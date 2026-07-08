////////////////////////////////////////////////////////////
// Threads.c Copyright K.kawakami
////////////////////////////////////////////////////////////
#include "imap4.h"
#include "service.h"
#include <process.h>       /* for _beginthread                      */
#include <share.h>
#pragma hdrstop

extern BOOL  bDebug;
extern int  nPriority;
extern char mMailSpoolDir[];
extern int  nAddressFamily;
extern int  nport;
extern BOOL  bListenMode;
extern BOOL bServiceTerminating;
extern BOOL bTrace;
extern char mTraceFile[];
extern FILE *fTrace;
extern TMQueue mTMQueue;
extern DWORD nTMOut;
extern BOOL    bTMQWait;
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
#endif
//#define TRACE 1
//#define DEBUG_THREAD 1

#ifdef Y2038_BUG
void gettime(SYSTEMTIME *ltime, char *mt);
#else
void gettime(time_t *ltime, char *mt);
#endif
//DWORD WorkerThread (LPVOID WorkContext);
VOID  CloseClient (PCLIENT_CONTEXT lpClientContext,BOOL bGraceful, BOOL bTMQ);
int   WSAGetLastErrorName(void);

/*
POP3SDisposition POP3SDispatch(
    PVOID       pContext,
    PUCHAR      InputBuffer,
    DWORD       InputBufferLen,
    PHANDLE     SendHandle,
    PUCHAR *    OutputBuffer,
    PDWORD      OutputBufferLen
    );
*/

/////////////////////////////////////////////////
// Send Pipe Line Messages
void SendPipeLine(char *str) {
  char    inbuf[80];
  char    outbuf[80];
  LPSTR   lpszPipeName = TEXT("\\\\.\\pipe\\SPA IMAP4S");
  DWORD   bytesRead;

  strncpy( outbuf, str, 80); outbuf[79] = '\x0';
  CallNamedPipe(lpszPipeName,
                outbuf, sizeof(outbuf),
                inbuf, sizeof(inbuf),
               &bytesRead, NMPWAIT_NOWAIT);
}

HANDLE InitializeThreads (void) {
    SOCKET s;
    HANDLE hCompletionPort;
    //HANDLE hThreadHandle;
    //DWORD dwThreadId;
    OSVERSIONINFO VersionInformation;
    SYSTEM_INFO   systemInfo;
    //PSID pSIDAliasAdmins = NULL; 
    //static SID_IDENTIFIER_AUTHORITY siaNTAuthority = SECURITY_NT_AUTHORITY; 
    //SECURITY_DESCRIPTOR  SD[SECURITY_DESCRIPTOR_MIN_LENGTH];
    //SECURITY_ATTRIBUTES  sa;
    //DWORD dwRevision;
    //BOOL bsa;

    ///////セキュリティ設定
    //bsa = InitializeSecurityDescriptor( &SD, SECURITY_DESCRIPTOR_REVISION);
    //bsa = SetSecurityDescriptorDacl(&SD, TRUE, (PACL) NULL, FALSE);
    //bsa = AllocateAndInitializeSid(&siaNTAuthority, 
    //                         2,
	//		                 SECURITY_BUILTIN_DOMAIN_RID, 
    //                         DOMAIN_ALIAS_RID_ADMINS, 
    //                         0, 0, 0, 0, 0, 0, 
    //                         &pSIDAliasAdmins);
    //bsa = SetSecurityDescriptorOwner(&SD, pSIDAliasAdmins, FALSE);

    //sa.nLength = sizeof(sa);
    //sa.lpSecurityDescriptor = &SD;
    //sa.bInheritHandle = TRUE;
    /////////////////////////////
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
        if (bDebug) printf("getaddrinfo failed with error\n");
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
    //for ( i = 0; i < systemInfo.dwNumberOfProcessors*2; i++ ) { 
	// スレッドを複数にするとタイムアウト処理で同期が
	// とれなくタイミングによりハングするのでシングル
	// スレッドにする。 2001.06.25 v1.21
	    /*
        hThreadHandle = CreateThread(
                            (LPSECURITY_ATTRIBUTES)NULL, //&sa,
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
		*/
    //}
    return hCompletionPort;
} // InitializeThreads

#ifdef ACCEPT_LOG_LEVEL3
void LEVEL_3_ACCEPTLOG(PCLIENT_CONTEXT lpClientContext, char *mess) {
	char *p;
    FILE *fp;
    if (bAcceptLog) {
	  CHAR   mtime[256];
      char   mdata[256], mAcptLogFn[256];
#ifdef Y2038_BUG
      SYSTEMTIME ltime, lt;
#else
      time_t ltime;
   	  struct tm lt;
#endif
	  p = strrchr(mess, '\r');
	  if (p) {
		*p = '\x0';
	  }
      gettime(&ltime, mtime);
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
	    sprintf(mAcptLogFn, "%sreceiveimap4\\%s\\%s.log", mMailSpoolDir, mComName, mdata);
	  } else {
#endif
      sprintf(mAcptLogFn, "%sreceiveimap4\\%s.log", mMailSpoolDir, mdata);
#ifdef REGTOFILE
	  }
#endif
 	  if (*mAcptLogFn != '\x0') {
#ifdef UPDATE_20260606 // ログの書き込み信頼性向上対策
      WaitForSingleObject(g_hMutexAcceptLog, INFINITE);  // 排他開始
      fpAcceptLog = OpenCloseLog(fpAcceptLog, mDTAcceptLog, "receiveimap4", mComName, lt);
	    if (fpAcceptLog) {
		   //if (LocalHandle(lpClientContext))
#ifdef Y2038_BUG
             sprintf(mdata, "%02d/%s/%04d:%02d:%02d:%02d",lt.wDay, mMonth[lt.wMonth-1], lt.wYear, lt.wHour, lt.wMinute, lt.wSecond);
#else
	         strftime( mdata, 128, "%d/%b/%Y:%H:%M:%S", &lt );
#endif
	         fprintf(fpAcceptLog, "[%s], %08x, %s\n",
									   mdata,
									   lpClientContext,
									   mess);
		   //else
	         //fprintf(fp, "[%s], %08x, %s\n",
			//						   mdata,
			//						   0,
			//						   mess);
           fflush(fpAcceptLog);
		}
#else
#ifdef UPDATE_20260603 // ログ出力の排他処理化を追加
        WaitForSingleObject(g_hMutexAcceptLog, INFINITE);  // 排他開始
#endif
        fp = _fsopen(mAcptLogFn, "at", _SH_DENYNO);
#ifdef UPDATE_20260605 // 詳細ログファイルのオープン失敗リトライ処理の追加
        if (!fp) 
        {
          Sleep(200); //200ms
          fp = _fsopen(mAcptLogFn, "at", _SH_DENYNO);
          if (!fp) 
          {
            Sleep(200); //200ms
            fp = _fsopen(mAcptLogFn, "at", _SH_DENYNO);
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

  		   fclose(fp);
		}
#endif
#ifdef UPDATE_20260603 // ログ出力の排他処理化を追加
       ReleaseMutex(g_hMutexAcceptLog);  // 排他終了
#endif
	  }
	  if (p) // 元に戻す
	    *p = '\r';

	}
}
#endif

