////////////////////////////////////////////////////////////
// Threads.c Copyright K.kawakami
////////////////////////////////////////////////////////////
#include "smtp.h"
#include "smtptype.h"
#include "service.h"
#include <share.h>
#pragma hdrstop

#ifdef UPDATE_20080929A // ログのクリティカルセクション化
extern CRITICAL_SECTION g_csWriteReport;
#endif
extern BOOL bVLog; // イベントビューワにログ書込みエラーを表示する 0:しない　1:する
extern int  nPriority;
extern char mMailSpoolDir[];
extern char mProgPath[];
extern int  nAddressFamily;
extern int  nport;
extern BOOL bListenMode;
extern BOOL bServiceTerminating;
extern BOOL bTrace;
extern char mTraceFile[];
extern FILE *fTrace;
extern TMQueue mTMQueue;
extern DWORD nTMOut;
//extern BOOL  bTMQWait;
extern BOOL  bThreadWait;
extern BOOL  bAcceptLog;
#ifdef Y2038_BUG
extern char mMonth[12][4];
extern char mWeek[7][4];
#endif
#ifdef USE_SSL
extern BOOL  bUsedSSL;
#endif
#ifdef CLUSTERING
extern BOOL   nClustering;
extern char   mComName[];
extern DWORD  nPipeCnt;
#endif

//OSVERSIONINFO VersionInformation;
//SYSTEM_INFO   systemInfo;
//#define TRACE 1

#ifdef Y2038_BUG
void gettime(SYSTEMTIME *ltime, char *mt);
#else
void gettime(time_t *ltime, char *mt);
#endif
DWORD WorkerThread (LPVOID WorkContext);
VOID  CloseClient (PCLIENT_CONTEXT lpClientContext,BOOL bGraceful, BOOL bTMQ);
int WSAGetLastErrorName(char *pErr);

#ifndef BTHREAD
SMTPRSDisposition SMTPRSDispatch(
    PVOID       pContext,
    PUCHAR      InputBuffer,
    DWORD       InputBufferLen,
    PHANDLE     SendHandle,
    PUCHAR *    OutputBuffer,
    PDWORD      OutputBufferLen
    );
#endif
/////////////////////////////////////////////////
// Send Pipe Line Messages
void SendPipeLine(char *str) {
#ifdef K_SEARCH // K_SEARCH OEM 版
#else
  char    inbuf[80];
  char    outbuf[80];
  DWORD   bytesRead;
  HANDLE  hPipe = NULL;
  OVERLAPPED Overlapped;
#ifdef CLUSTERING
  LPTSTR  lpszPipeName = TEXT("\\\\%s\\pipe\\" TRADEMARK "SMTPDS");
  FILE    *fp;
  CHAR    *p, mMachineName[256], mMMList[256], mPipeName[256];

  sprintf(mMMList,"%smmlist.dat", mProgPath); 
  fp = fopen(mMMList, "rt");
  strcpy(mMachineName, "."); // ローカルマシン
  do {
    sprintf(mPipeName, lpszPipeName , mMachineName); // 通知先マシン名
    hPipe = CreateFile((LPTSTR)mPipeName,  // ファイル名
                     /*GENERIC_READ | */GENERIC_WRITE, // アクセスモード
                     FILE_SHARE_READ | FILE_SHARE_WRITE, // 共有モード
                     NULL, // セキュリティ記述子
                     OPEN_EXISTING,                // 作成方法
                     FILE_ATTRIBUTE_TEMPORARY, //FILE_ATTRIBUTE_NORMAL,                 // ファイル属性
                     NULL);                        // テンプレートファイルのハンドル
#else
  LPTSTR  lpszPipeName = TEXT("\\\\.\\pipe\\" TRADEMARK "SMTPDS");

  hPipe = CreateFile(lpszPipeName,  // ファイル名
                     /*GENERIC_READ | */GENERIC_WRITE, // アクセスモード
                     FILE_SHARE_READ | FILE_SHARE_WRITE, // 共有モード
                     NULL, // セキュリティ記述子
                     OPEN_EXISTING,                // 作成方法
                     FILE_ATTRIBUTE_TEMPORARY, //FILE_ATTRIBUTE_NORMAL,                 // ファイル属性
                     NULL);                        // テンプレートファイルのハンドル
#endif
  if (hPipe != INVALID_HANDLE_VALUE) {
    // SMTP 送信部に通信する。
#ifndef UPDATE_20041208 // ファイルのオープンだけで通知完了。データ書き込みは不要
    strncpy( outbuf, str, 80); outbuf[79] = '\x0';
    TransactNamedPipe(hPipe,  // 名前付きパイプのハンドル
                     outbuf, sizeof(outbuf),
                     inbuf, sizeof(inbuf),
                     &bytesRead,
					 &Overlapped);
#endif
	CloseHandle(hPipe);
  }
#ifdef CLUSTERING
	nPipeCnt++;
    if (nPipeCnt%50 != 1) // 初回を含み、特定するリクエスト回数で他のマシンにも要求をかける。
	  break;
	nPipeCnt = 1;
    p = NULL;
    if (fp) {
	  p = fgets(mMachineName, sizeof(mMachineName), fp);
	  if (mMachineName[0] == '\n') {
		mMachineName[0] = '\x0';
        p = NULL;
	  }
	  strtok(mMachineName, "\n"); 
	}
  } while (p);
  if (fp)
	fclose(fp);
#endif
#endif
}

HANDLE InitializeThreads (void) {
    SOCKET s;
    //DWORD i;
    HANDLE hCompletionPort;
    HANDLE hThreadHandle;
    DWORD dwThreadId;
    OSVERSIONINFO VersionInformation;
    SYSTEM_INFO   systemInfo;

#ifdef IPv6
    char *Address = NULL;
    struct addrinfo Hints, *AddrInfo, *AI;
	char mport[16];

    memset(&Hints, 0, sizeof(Hints));
    Hints.ai_family = (nAddressFamily ? AF_INET6 : AF_INET);
    Hints.ai_socktype = SOCK_STREAM;
    Hints.ai_flags = AI_NUMERICHOST | AI_PASSIVE;
	itoa(nport, mport, 10);
    if (getaddrinfo(Address, mport, &Hints, &AddrInfo) != 0) {
        printf("getaddrinfo failed with error\n");
        //WSACleanup();
        return NULL;
    }
    AI = AddrInfo;
    s = socket(AI->ai_family, AI->ai_socktype, AI->ai_protocol);
    if ( s == INVALID_SOCKET )
      return NULL;
#else
    //s = socket( AF_INET, SOCK_DGRAM, 0 );
    s = socket( AF_INET, SOCK_STREAM, 0 );
    if ( s == INVALID_SOCKET )
        return NULL;
#endif
    // Create the completion port that will be used by all the worker threads.
    hCompletionPort = CreateIoCompletionPort( (HANDLE)s, NULL, 0, 0 );
    if ( hCompletionPort == NULL ) {
        closesocket( s );
        return NULL;
    }
    closesocket( s );
	VersionInformation.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    GetVersionEx( &VersionInformation);
    printf("windows %ld.%ld Build %ld (%s)\n",
		            VersionInformation.dwMajorVersion,
		            VersionInformation.dwMinorVersion,
		            VersionInformation.dwBuildNumber,
					VersionInformation.szCSDVersion);
    GetSystemInfo( &systemInfo );
	if (systemInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL)
	  printf("Intel");
	else if (systemInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_MIPS)
	  printf("Mips");
	else if (systemInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_ALPHA)
	  printf("Alpha");
	else 
	  printf("Unknown");
	printf(" %ld processor in the system.\n", systemInfo.dwNumberOfProcessors );
#ifndef BTHREAD
    //for ( i = 0; i < systemInfo.dwNumberOfProcessors*2; i++ ) {
	// スレッドを複数にするとタイムアウト処理で同期が
	// とれなくタイミングによりハングするのでシングル
	// スレッドにする。 2001.06.25 v1.45
        hThreadHandle = CreateThread(
                            (LPSECURITY_ATTRIBUTES)NULL,
                            0,
                            (LPTHREAD_START_ROUTINE) WorkerThread,
                            (LPVOID) hCompletionPort,
                            0,
                            &dwThreadId
                            );
        if ( hThreadHandle == NULL ) {
            CloseHandle( hCompletionPort );
            return NULL;
        }
        printf("GetPriorityClass=%d\n", GetPriorityClass(hThreadHandle));
		//if (nPriority != 0)
        SetThreadPriority(hThreadHandle, nPriority); // スレッドの相対優先順位値
        printf("GetThreadPriority=%d\n",GetThreadPriority(hThreadHandle));
        CloseHandle( hThreadHandle );
    //}
#endif
    return hCompletionPort;
} // InitializeThreads

#ifdef ACCEPT_LOG_LEVEL3
void LEVEL_3_ACCEPTLOG(PCLIENT_CONTEXT lpClientContext, char *mess)
{
    FILE *fp;
#ifdef UPDATE_20080929A // ログのクリティカルセクション化
    EnterCriticalSection(&g_csWriteReport);
#endif
    if (bAcceptLog) {
	  CHAR   mtime[256];
      char   mdata[256], mAcptLogFn[256];
#ifdef Y2038_BUG
      SYSTEMTIME ltime, lt;
#else
      time_t ltime;
   	  struct tm lt;
#endif
      gettime(&ltime, mtime);
	  //time(&ltime); 
#ifdef Y2038_BUG
      _tzset();
	  SystemTimeToTzSpecificLocalTime(NULL, &ltime, &lt);
      sprintf(mdata, "%02d%02d%02d", (lt.wYear%100), lt.wMonth, lt.wDay );
#else
      lt = *localtime(&ltime);
      strftime(mdata, 128, "%y%m%d", &lt );
#endif

#ifdef REGTOFILE
      if (nClustering) {
	    sprintf(mAcptLogFn, "%sreceivelog\\%s\\%s.log", mMailSpoolDir, mComName, mdata);
	  } else {
#endif
      sprintf(mAcptLogFn, "%sreceivelog\\%s.log", mMailSpoolDir, mdata);
#ifdef REGTOFILE
	  }
#endif
 	  if (*mAcptLogFn != '\x0') {
#ifdef UPDATE_20260606 // ログの書き込み信頼性向上対策
      WaitForSingleObject(g_hMutexRecivedLog, INFINITE);  // 排他開始
      fpRecivedLog = OpenCloseLog(fpRecivedLog, mDTRecivedLog, "receivelog", mComName, lt);
	    if (fpRecivedLog) {
		   //if (LocalHandle(lpClientContext))
#ifdef Y2038_BUG
             sprintf(mdata, "%02d/%s/%04d:%02d:%02d:%02d",lt.wDay, mMonth[lt.wMonth-1], lt.wYear, lt.wHour, lt.wMinute, lt.wSecond);
#else
	         strftime( mdata, 128, "%d/%b/%Y:%H:%M:%S", &lt );
#endif
	         fprintf(fpRecivedLog, "[%s], %08x, %s\n",
									   mdata,
									   lpClientContext,
									   mess);
		   //else
	         //fprintf(fp, "[%s], %08x, %s\n",
			//						   mdata,
			//						   0,
			//						   mess);

#ifdef UPDATE_20060615 // データのディスク書込に異常が有った場合の対策
		   if (fflush(fpRecivedLog) == EOF)
			 if (bVLog)
               AddToMessageLog(TEXT("receivelog write fail."), 115, TEXT(SMTP_SERVICE), EVENTLOG_WARNING_TYPE);
#endif
		}
#else
#ifdef UPDATE_20260603 // ログ出力の排他処理化を追加
        WaitForSingleObject(g_hMutexRecivedLog, INFINITE);  // 排他開始
#endif
        fp = _fsopen(mAcptLogFn, "at", _SH_DENYNO); //receivelog
#ifdef UPDATE_20260605 // 詳細ログファイルのオープン失敗リトライ処理の追加
	    if (!fp) 
		{
		  Sleep(200); //200ms
		  fp = _fsopen(mAcptLogFn, "at", _SH_DENYNO); //receivelog
	      if (!fp) 
		  {
		    Sleep(200); //200ms
		    fp = _fsopen(mAcptLogFn, "at", _SH_DENYNO); //receivelog
		  }
        }
#endif
	    if (fp) {
		   //if (LocalHandle(lpClientContext))
#ifdef Y2038_BUG
             sprintf(mdata, "%02d/%s/%04d:%02d:%02d:%02d",lt.wDay, mMonth[lt.wMonth-1], lt.wYear, lt.wHour, lt.wMinute, lt.wSecond);
#else
	         strftime( mdata, 128, "%d/%b/%Y:%H:%M:%S", &lt );
#endif
	         fprintf(fp, "[%s], %08x, %s\n",
									   mdata,
									   lpClientContext,
									   mess);
		   //else
	         //fprintf(fp, "[%s], %08x, %s\n",
			//						   mdata,
			//						   0,
			//						   mess);

#ifdef UPDATE_20060615 // データのディスク書込に異常が有った場合の対策
		   if (fflush(fp) == EOF)
			 if (bVLog)
               AddToMessageLog(TEXT("receivelog write fail."), 115, TEXT(SMTP_SERVICE), EVENTLOG_WARNING_TYPE);
#endif
  		   fclose(fp);
		}
#endif
#ifdef UPDATE_20260603 // ログ出力の排他処理化を追加
        ReleaseMutex(g_hMutexRecivedLog);  // 排他終了
#endif
	  }
	}
#ifdef UPDATE_20080929A // ログのクリティカルセクション化
    LeaveCriticalSection(&g_csWriteReport);
#endif
}
#endif

#ifndef BTHREAD
DWORD WorkerThread (LPVOID WorkContext) {
    HANDLE hCompletionPort = WorkContext;
    BOOL bSuccess;
	int  nStatus;
    DWORD dwIoSize, nTMO, dwErr;
    LPOVERLAPPED lpOverlapped;
    PCLIENT_CONTEXT lpClientContext;
    SMTPRSDisposition Disposition;
    HANDLE hFile = NULL;
    CHAR * OutputBuffer;
    DWORD OutputBufferLen;
	char  mErr[256];
	//int i;

	bThreadWait = FALSE;
	lpClientContext = NULL;
	nTMO = (nTMOut ? (DWORD)nTMOut*1000 : (DWORD)INFINITE); //60000ms=１分 //(DWORD)INFINITE
    while ( TRUE ) {
		/*
        while(bThreadWait) {
          printf("[%08x] WorkerThread = wait loop\n", lpClientContext);
          if ( bServiceTerminating )
            break;
          _sleep(SLEEP_TIME);
		};
		bThreadWait = TRUE;
		*/
#ifdef TMQ_ON
#ifndef BTHREAD
	    if (nTMOut) {
		  if (bDebug) printf("[%08x] Start Check Timer Queue\n", lpClientContext);
#ifdef ACCEPT_LOG_LEVEL3
        LEVEL_3_ACCEPTLOG(lpClientContext, "Start Check Timer Queue");
#endif
		if (CheckTMQueue(lpClientContext, TRUE, FALSE)) {// 対象ハンドルは待ち状態
		  if (bDebug) printf("[%08x] Start UpDate Timer Queue\n", lpClientContext);
#ifdef ACCEPT_LOG_LEVEL3
        LEVEL_3_ACCEPTLOG(lpClientContext, "Start UpDate Timer Queue");
#endif
           UpDateTMQueue(lpClientContext, FALSE);        // 対象ハンドルが存在すればタイマーリセット
		}
		if (bDebug) printf("[%08x] Start TimeOut Timer Queue\n", lpClientContext);
#ifdef ACCEPT_LOG_LEVEL3
        LEVEL_3_ACCEPTLOG(lpClientContext, "Start TimeOut Timer Queue");
#endif
        TimeOutTMQueue(lpClientContext, FALSE); //タイムアウトしたハンドルをチェックし開放する。
		if (bDebug) printf("[%08x] End Timer Queue\n", lpClientContext);
#ifdef ACCEPT_LOG_LEVEL3
        LEVEL_3_ACCEPTLOG(lpClientContext, "End Timer Queue");
#endif
		}
#endif
#endif
/*
#ifdef TMQ_ON
	    if (nTMOut) {
          while(bTMQWait) {
            if ( bServiceTerminating )
              break;
            _sleep(SLEEP_TIME);
		  };
          bTMQWait = TRUE;
          //TimeOutTMQueue(lpClientContext, TRUE); //タイムアウトしたハンドルをチェックし開放する。
          UpDateTMQueue(lpClientContext, TRUE);        // 対象ハンドルが存在すればタイマーリセット
          CheckTMQueue(lpClientContext, TRUE, TRUE); // 対象ハンドルは待ち状態
          bTMQWait = FALSE;
		}
#endif
*/
//GET_COMPLETED_IO:
        // Get a completed IO request.
		dwIoSize = 0;
		lpClientContext = NULL;
		lpOverlapped = NULL;
		////// Timeout timer.
		if (bDebug) printf("[%08x] Start GetQueuedCompletionStatus hCompletionPort=%08x\n", lpClientContext, hCompletionPort);
#ifdef ACCEPT_LOG_LEVEL3
        LEVEL_3_ACCEPTLOG(lpClientContext, "Start GetQueuedCompletionStatus");
#endif
        bSuccess = GetQueuedCompletionStatus(
                       hCompletionPort,
                       &dwIoSize,
                       (LPDWORD)&lpClientContext,
                       &lpOverlapped,
#ifdef TMQ_ON
					   nTMO
#else
                       (DWORD)INFINITE
#endif
                       );
/*
#ifdef TMQ_ON
        TimeOutTMQueue(lpClientContext, FALSE); //タイムアウトしたハンドルをチェックし開放する。
#endif
*/
		if (bDebug) printf("[%08x] GetQueuedCompletionStatus bSuccess = %d\n", (bSuccess ? lpClientContext : 0), bSuccess);
	//		bThreadWait = FALSE;
		if (!bSuccess) {
		  if (WAIT_TIMEOUT == GetLastError()) {
		    if (bDebug) printf("[%08x] End GetQueuedCompletionStatus(Timeout)\n", lpClientContext);
#ifdef ACCEPT_LOG_LEVEL3
            LEVEL_3_ACCEPTLOG(lpClientContext, "*** End GetQueuedCompletionStatus(Timeout)");
#endif
		    lpClientContext = NULL;
			continue;
		  }
		}
		if (bDebug) printf("[%08x]End GetQueuedCompletionStatus\n", lpClientContext);
#ifdef ACCEPT_LOG_LEVEL3
        LEVEL_3_ACCEPTLOG(lpClientContext, "End GetQueuedCompletionStatus");
#endif
        // If the service is terminating, exit this thread.
        if ( bServiceTerminating )
          return 0;
		
		if (bTrace) {
		   fTrace = fopen(mTraceFile,"at");
		   if (fTrace) {
		     fprintf(fTrace, "bSuccess=%s, dwIoSize=%04x, lpClientContext=%08x, lpOverlapped=%08x\n",
				(bSuccess ? "true" : "false"),
                dwIoSize,
			    lpClientContext,
		        lpOverlapped
			   );
			 fclose(fTrace);
		   }
		}
		
		if (!lpClientContext) {
		  if (bDebug) printf("[00000000]lpClientContext == NULL !!\n");
#ifdef ACCEPT_LOG_LEVEL3
          LEVEL_3_ACCEPTLOG(lpClientContext, "*** lpClientContext == NULL !!");
#endif
		  //goto GET_COMPLETED_IO;
		  continue;
		}
 	    if (bDebug) printf("[%08x]lpClientContext\n", lpClientContext);
////////////////////////////////////////
#ifdef TMQ_ON
#ifndef BTHREAD
		if (nTMOut) {
		  if (!CheckTMQueue(lpClientContext, FALSE, FALSE) ) {  // 対象ハンドルは動作中
		    if (bDebug) printf("[%08x] CheckTMQueue(lpClientContext) == NULL !!\n", lpClientContext);
#ifdef ACCEPT_LOG_LEVEL3
            LEVEL_3_ACCEPTLOG(lpClientContext, "*** CheckTMQueue(lpClientContext) == NULL !!");
#endif
		    lpClientContext = NULL;
			//goto GET_COMPLETED_IO;
            continue;
		  }
          //TimeOutTMQueue(lpClientContext, FALSE); //タイムアウトしたハンドルをチェックし開放する。
		}
#endif
#endif
////////////////////////////////////////
		if (!bSuccess) {
          if (bDebug) printf("[%08x] bSuccess = zero.\n", lpClientContext);
#ifdef ACCEPT_LOG_LEVEL3
          LEVEL_3_ACCEPTLOG(lpClientContext, "*** bSuccess = zero.");
#endif
          CloseClient( lpClientContext, FALSE, FALSE);
		  continue;
		}
		if (dwIoSize == 0) {
          if (bDebug) printf("[%08x] dwIoSize = zero.\n", lpClientContext);
#ifdef ACCEPT_LOG_LEVEL3
          LEVEL_3_ACCEPTLOG(lpClientContext, "*** dwIoSize = zero.");
#endif
          CloseClient( lpClientContext, FALSE, FALSE);
		  continue;
		} else {
#ifdef USE_SSL
#ifdef USE_STARTTLS
		  if ((lpClientContext->Context.bUsedSSL || 
			   !bListenMode && bUsedSSL) &&
			   lpClientContext->ssl &&
			   bSuccess) 
#else
		  if (( bListenMode && lpClientContext->Context.bUsedSSL || 
			   !bListenMode && bUsedSSL) &&
			  lpClientContext->ssl &&
			  bSuccess) 
#endif
		  {
#ifndef TUNING
			memset(lpClientContext->SSLBuffer, 0, sizeof(lpClientContext->SSLBuffer));
			memset(lpClientContext->Buffer, 0, sizeof(lpClientContext->Buffer));
#else
			lpClientContext->SSLBuffer[0] = '\x0';
			lpClientContext->Buffer[0] = '\x0';
#endif
			memcpy(lpClientContext->SSLBuffer, lpClientContext->Buffer, dwIoSize);
			dwIoSize = SSL_read(lpClientContext->ssl, lpClientContext->Buffer, sizeof(lpClientContext->Buffer));
			if (dwIoSize == -1)
			  dwIoSize = 0;
			lpClientContext->Buffer[dwIoSize] = '\x0';
//printf("size=%d:%s", dwIoSize, lpClientContext->Buffer);
		  } 
#endif
          // If the request was a read, process the client request.
          if ( lpClientContext->LastClientIo == ClientIoRead &&
			   !lpClientContext->bUsed) {
		    lpClientContext->bUsed = TRUE;
                Disposition = SMTPRSDispatch(
                                &lpClientContext->Context,
                                lpClientContext->Buffer,
                                dwIoSize, //lpOverlapped->InternalHigh,
                                &hFile,
                                &OutputBuffer,
                                &OutputBufferLen
                                );
			if (bDebug) printf("[%08x] Disposition = %d\n", lpClientContext, Disposition);
            // Act based on the Disposition.
            switch ( Disposition ) {
              case SMTPRS_Discard:
                  break;

              case SMTPRS_SendError:
              case SMTPRS_SendBuffer:
              case SMTPRS_Quit:
                  lpClientContext->LastClientIo = ClientIoWrite;
				  /* 1byte毎の転送試験用
				  for (i = 0; i < (int)OutputBufferLen; i++)
				    nStatus = send(lpClientContext->Socket, &OutputBuffer[i], 1, 0 );
				  */
#ifdef USE_SSL
#ifdef USE_STARTTLS
		          if ((lpClientContext->Context.bUsedSSL || 
			           !bListenMode && bUsedSSL) &&
			           lpClientContext->ssl)
#else
		          if (( bListenMode && lpClientContext->Context.bUsedSSL || 
			           !bListenMode && bUsedSSL) &&
			           lpClientContext->ssl)
#endif
					nStatus = SSL_write(lpClientContext->ssl, OutputBuffer, OutputBufferLen);
				  else
#endif
				  nStatus = send(lpClientContext->Socket, OutputBuffer, OutputBufferLen, 0 );
#ifdef ACCEPT_LOG_LEVEL3
                  LEVEL_3_ACCEPTLOG(lpClientContext, OutputBuffer);
#endif
				  if (nStatus == SOCKET_ERROR) {
/*
#ifdef ACCEPT_LOG_LEVEL3
                     LEVEL_3_ACCEPTLOG(lpClientContext, "send() SOCKET_ERROR");
#endif
*/
					 /* 2002.04.13
					 if (WSAGetLastErrorName(mErr) == WSAECONNRESET) {
                       Disposition = SMTPRS_Quit; // 送信エラー発生でSOCKET　クローズ
					 }
					 */
                     Disposition = SMTPRS_Quit; // 送信エラー発生でSOCKET　クローズ
					 WSAGetLastErrorName(mErr);
					 strcat(mErr, " = send() SOCKET_ERROR");
#ifdef ACCEPT_LOG_LEVEL3
                     LEVEL_3_ACCEPTLOG(lpClientContext, mErr);
#endif

					 //Disposition = SMTPRS_Quit; // 送信エラー発生でSOCKET　クローズ
/*

					//if (WSAGetLastErrorName() == WSAECONNRESET) {
				      CloseClient( lpClientContext, FALSE, FALSE);
					  continue;
					//}
*/
				  }
#ifdef ACCEPT_LOG_LEVEL2
    if (bAcceptLog) {
      char   mdata[256], mAcptLogFn[256];
#ifdef Y2038_BUG
	  SYSTEMTIME ltime;
      GetLocalTime(&ltime);
      sprintf(mdata, "%02d%02d%02d", (ltime.wYear%100), ltime.wMonth, ltime.wDay );
#else
      time_t ltime;
   	  struct tm lt;
	  time(&ltime); 
      lt = *localtime(&ltime);
      strftime(mdata, 128, "%y%m%d", &lt );
#endif
#ifdef UPDATE_20260606 // ログの書き込み信頼性向上対策
      WaitForSingleObject(g_hMutexAcceptLog, INFINITE);  // 排他開始
      fpAcceptLog = OpenCloseLog(fpAcceptLog, mDTAcceptLog, "acceptlog", mComName, lt);
#endif
#ifdef REGTOFILE
      if (nClustering) {
	    sprintf(mAcptLogFn, "%sacceptlog\\%s\\%s.log", mMailSpoolDir, mComName, mdata);
	  } else {
#endif
      sprintf(mAcptLogFn, "%sacceptlog\\%s.log", mMailSpoolDir, mdata);
#ifdef REGTOFILE
	  }
#endif
 	  if (*mAcptLogFn != '\x0') {
#ifdef UPDATE_20260606 // ログの書き込み信頼性向上対策
	    if (fpAcceptLog) {
		   WSAGetLastErrorName(mdata);
		   //fprintf((lpClientContext->Context).Logfp, "%d:%s\n", OutputBufferLen, OutputBuffer);
		   /*
		   if (OutputBufferLen) {  // 
		     strtok(OutputBuffer,"\r\n");　これするとハングる
           }*/
#ifdef K_SEARCH // K_SEARCH OEM 版
	       fprintf((fpAcceptLog, " <%s>, [%s], %s\n",
                                      (lpClientContext->Context).mMsgId, 
									  mdata,
									  OutputBuffer);
#else
#ifdef UPDATE_20071204  // メッセージＩＤ採番処理を修正(Bym10id)
	       fprintf(fpAcceptLog, " <%s>, [%s], %s\n",
                                      (lpClientContext->Context).mMsgId, 
									  mdata,
									  OutputBuffer);
#else
	       fprintf(fpAcceptLog, " <B%010lu>, [%s], %s\n",
                                      (lpClientContext->Context).nMsgId, 
									  mdata,
									  OutputBuffer);
#endif
#endif
  		   fflush(fpAcceptLog);
		}
#else
#ifdef UPDATE_20260603 // ログ出力の排他処理化を追加
        WaitForSingleObject(g_hMutexAcceptLog, INFINITE);  // 排他開始
#endif
        (lpClientContext->Context).Logfp = fopen(mAcptLogFn, "at");
	    if ((lpClientContext->Context).Logfp) {
		   WSAGetLastErrorName(mdata);
		   //fprintf((lpClientContext->Context).Logfp, "%d:%s\n", OutputBufferLen, OutputBuffer);
		   /*
		   if (OutputBufferLen) {  // 
		     strtok(OutputBuffer,"\r\n");　これするとハングる
           }*/
#ifdef K_SEARCH // K_SEARCH OEM 版
	       fprintf((lpClientContext->Context).Logfp, " <%s>, [%s], %s\n",
                                      (lpClientContext->Context).mMsgId, 
									  mdata,
									  OutputBuffer);
#else
#ifdef UPDATE_20071204  // メッセージＩＤ採番処理を修正(Bym10id)
	       fprintf((lpClientContext->Context).Logfp, " <%s>, [%s], %s\n",
                                      (lpClientContext->Context).mMsgId, 
									  mdata,
									  OutputBuffer);
#else
	       fprintf((lpClientContext->Context).Logfp, " <B%010lu>, [%s], %s\n",
                                      (lpClientContext->Context).nMsgId, 
									  mdata,
									  OutputBuffer);
#endif
#endif
  		   fclose((lpClientContext->Context).Logfp);
		}
#endif
#ifdef UPDATE_20260603 // ログ出力の排他処理化を追加
        ReleaseMutex(g_hMutexAcceptLog);  // 排他終了
#endif
	  }
	}
#endif
				  /*
				  if (OutputBuffer)
                    if (bDebug) printf("[%s]", OutputBuffer);
				  */
                  if (bDebug) printf("\n");
				  OutputBuffer = '\x0';
  				  if (Disposition == SMTPRS_Quit) {
	                lpClientContext->bUsed = FALSE;
		            if (bTrace) {
                      fTrace = fopen(mTraceFile,"at");
                      if (fTrace) {
#ifdef K_SEARCH // K_SEARCH OEM 版
                        fprintf(fTrace, "<%s>, CloseClient\n",(lpClientContext->Context).mMsgId);
#else
#ifdef UPDATE_20071204  // メッセージＩＤ採番処理を修正(Bym10id)
                        fprintf(fTrace, "<%s>, CloseClient\n",(lpClientContext->Context).mMsgId);
#else
                        fprintf(fTrace, "<B%010lu>, CloseClient\n",(lpClientContext->Context).nMsgId);
#endif
#endif
                        fclose(fTrace);
					  }
					}
                    if (bDebug) printf("[%08x] Close Client Success.\n", lpClientContext);
#ifdef ACCEPT_LOG_LEVEL3
                    LEVEL_3_ACCEPTLOG(lpClientContext, "Close Client Success.");
#endif
				    CloseClient( lpClientContext, FALSE, FALSE);
#ifdef ACCEPT_LOG_LEVEL3
                    LEVEL_3_ACCEPTLOG(NULL, "Send Quit Start");
#endif
					SendPipeLine("Quit");
#ifdef ACCEPT_LOG_LEVEL3
                    LEVEL_3_ACCEPTLOG(NULL, "Send Quit End.");
#endif
					continue;
				  }
				  break;
			  default:
  				  break;
			}
		    lpClientContext->bUsed = FALSE;
		  } 
		}
        lpClientContext->LastClientIo = ClientIoRead;
        lpClientContext->BytesReadSoFar = 0;
#ifndef TUNING
        memset(lpClientContext->Buffer,0,sizeof(lpClientContext->Buffer));
#else
        lpClientContext->Buffer[0] = '\x0';
#endif
		if(lpClientContext->Socket) {
		  lpClientContext->Overlapped.Internal = 0;
          lpClientContext->Overlapped.InternalHigh = 0;
          lpClientContext->Overlapped.Offset = 0;
          lpClientContext->Overlapped.OffsetHigh = 0;
          lpClientContext->Overlapped.hEvent = NULL;
#ifdef USE_SSL
#ifdef USE_STARTTLS
        if ((lpClientContext->Context.bUsedSSL || 
             !bListenMode && bUsedSSL) &&
		    lpClientContext->ssl)
#else
        if (( bListenMode && lpClientContext->Context.bUsedSSL || 
             !bListenMode && bUsedSSL) &&
		    lpClientContext->ssl)
#endif
		{
	      lpClientContext->ssl->rbio->nOnMemory = 1; // 欲しいサイズ問合せ
	      SSL_read(lpClientContext->ssl,lpClientContext->Buffer,sizeof(lpClientContext->Buffer));
          if (bDebug) printf("nOnOut %d\n", lpClientContext->ssl->rbio->nOnOut);
		}
#endif
		  if (bDebug) printf("[%08x] Start ReadFile():THREADS.C\n", lpClientContext);
#ifdef ACCEPT_LOG_LEVEL3
          LEVEL_3_ACCEPTLOG(lpClientContext, "Start ReadFile():THREADS.C");
#endif

          bSuccess = ReadFile(
                       (HANDLE)lpClientContext->Socket,
                       lpClientContext->Buffer,
#ifdef USE_SSL
					   (lpClientContext->ssl ? lpClientContext->ssl->rbio->nOnOut : sizeof(lpClientContext->Buffer)),
#else
                       sizeof(lpClientContext->Buffer),
#endif
                       NULL, //&lpClientContext->dwBytesRead,
                       &lpClientContext->Overlapped
                       );
            dwErr = GetLastError();
//		bThreadWait = FALSE;
#ifdef ACCEPT_LOG_LEVEL2
    if (bAcceptLog) {
      char   mdata[256], mAcptLogFn[256];
#ifdef Y2038_BUG
	  SYSTEMTIME ltime;
      GetLocalTime(&ltime);
      sprintf(mdata, "%02d%02d%02d", (ltime.wYear%100), ltime.wMonth, ltime.wDay );
#else
      time_t ltime;
   	  struct tm lt;
	  time(&ltime); 
      lt = *localtime(&ltime);
      strftime(mdata, 128, "%y%m%d", &lt );
#endif
#ifdef UPDATE_20260606 // ログの書き込み信頼性向上対策
      WaitForSingleObject(g_hMutexAcceptLog, INFINITE);  // 排他開始
      fpAcceptLog = OpenCloseLog(fpAcceptLog, mDTAcceptLog, "acceptlog", mComName, lt);
#endif
#ifdef REGTOFILE
      if (nClustering) {
	    sprintf(mAcptLogFn, "%sacceptlog\\%s\\%s.log", mMailSpoolDir, mComName, mdata);
	  } else {
#endif
      sprintf(mAcptLogFn, "%sacceptlog\\%s.log", mMailSpoolDir, mdata);
#ifdef REGTOFILE
	  }
#endif
 	  if (*mAcptLogFn != '\x0') {
#ifdef UPDATE_20260606 // ログの書き込み信頼性向上対策
	    if (fpAcceptLog) {
		   strtok(OutputBuffer,"\r\n");
#ifdef K_SEARCH // K_SEARCH OEM 版
	       fprintf(fpAcceptLog, " <%s>, [Read %sstatus = %d]\n",
                                      (lpClientContext->Context).mMsgId, 
									  (bSuccess ? "" : "error "),
									  dwErr);
#else
#ifdef UPDATE_20071204  // メッセージＩＤ採番処理を修正(Bym10id)
	       fprintf(fpAcceptLog, " <%s>, [Read %sstatus = %d]\n",
                                      (lpClientContext->Context).mMsgId, 
									  (bSuccess ? "" : "error "),
									  dwErr);
#else
	       fprintf(fpAcceptLog, " <B%010lu>, [Read %sstatus = %d]\n",
                                      (lpClientContext->Context).nMsgId, 
									  (bSuccess ? "" : "error "),
									  dwErr);
#endif
#endif
  		   fflush(fpAcceptLog);
		}
#else
#ifdef UPDATE_20260603 // ログ出力の排他処理化を追加
        WaitForSingleObject(g_hMutexAcceptLog, INFINITE);  // 排他開始
#endif
        (lpClientContext->Context).Logfp = fopen(mAcptLogFn, "at");
	    if ((lpClientContext->Context).Logfp) {
		   strtok(OutputBuffer,"\r\n");
#ifdef K_SEARCH // K_SEARCH OEM 版
	       fprintf((lpClientContext->Context).Logfp, " <%s>, [Read %sstatus = %d]\n",
                                      (lpClientContext->Context).mMsgId, 
									  (bSuccess ? "" : "error "),
									  dwErr);
#else
#ifdef UPDATE_20071204  // メッセージＩＤ採番処理を修正(Bym10id)
	       fprintf((lpClientContext->Context).Logfp, " <%s>, [Read %sstatus = %d]\n",
                                      (lpClientContext->Context).mMsgId, 
									  (bSuccess ? "" : "error "),
									  dwErr);
#else
	       fprintf((lpClientContext->Context).Logfp, " <B%010lu>, [Read %sstatus = %d]\n",
                                      (lpClientContext->Context).nMsgId, 
									  (bSuccess ? "" : "error "),
									  dwErr);
#endif
#endif
  		   fclose((lpClientContext->Context).Logfp);
		}
#endif
#ifdef UPDATE_20260603 // ログ出力の排他処理化を追加
        ReleaseMutex(g_hMutexAcceptLog);  // 排他終了
#endif
	  }
	}
#endif
          if (!bSuccess) {
            //dwErr = GetLastError();
		    //if ( !bSuccess && GetLastError() != ERROR_IO_PENDING ) {
			if (dwErr != ERROR_IO_PENDING &&
				dwErr != ERROR_NETNAME_DELETED ) {
			  if (dwErr == ERROR_INVALID_USER_BUFFER)
		        if (bDebug) printf("[%08x] Read File error (ERROR_INVALID_USER_BUFFER)\n", lpClientContext);
			  else if (dwErr == ERROR_NOT_ENOUGH_MEMORY)
		        if (bDebug) printf("[%08x] Read File error (ERROR_NOT_ENOUGH_MEMORY)\n", lpClientContext);
			  else
		        if (bDebug) printf("[%08x] Read File error (OHTER=%ld)\n", lpClientContext, dwErr);
#ifdef ACCEPT_LOG_LEVEL3
              LEVEL_3_ACCEPTLOG(lpClientContext, "End ReadFile() Error:THREADS.C");
#endif
              CloseClient( lpClientContext, FALSE, FALSE);
              continue;
			}
		  }
#ifdef ACCEPT_LOG_LEVEL3
          LEVEL_3_ACCEPTLOG(lpClientContext, "End ReadFile() Success:THREADS.C");
#endif
		}
    }
    return 0;
} // WorkThread
#endif