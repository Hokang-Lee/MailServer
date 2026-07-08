/****************************************************************************\
*  sendmail.c -- Mail program Windows Sockets APIs.
*
****************************************************************************/
#include "smtp.h"
#include <winbase.h>
#include <windows.h>       /* required for all Windows applications */
#include <winsock.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/utime.h>
#include <stdio.h>         /* for sprintf                           */
#include <fcntl.h>
#include <io.h>
#include <time.h>
#include <direct.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <memory.h>
#include <process.h>       /* for _beginthread                      */
#include <errno.h>
#include "profile.h"
#include "version.h"

#define SIO_UDP_CONNRESET _WSAIOW(IOC_VENDOR,12) 
#define MAXMAILDATA  1024 //1KB
//#define WAIT_TIME      10

#ifdef UPDATE_20220725 // "MAIL From:","RCPT To:"のコマンド出力を"MAIL FROM:","RCPT TO:"に変更するようにした。
extern int nSelectcase; // 動詞部分の大小文字の選択: 0:従来 1:小文字 2:大文字（デフォルト）
#endif
#ifdef UPDATE_20171211A // 使用可能な暗号化通信方式のバージョンを規定するオプション 0x1:SSL2禁止 0x2:SSL3禁止 0x4:TSL1禁止 0x8:TSL1.1禁止 0x10:TSL1.2禁止
extern unsigned long nSecuerLayOption;
#endif
#ifdef UPDATE_20190426 // 利用する暗号アルゴリズムの組み合わせオプション 
extern char mSecuerLayCipher[];
#endif
#ifdef UPDATE_20240420 // 暗号化通信方式のセキュリティレベルの設定
extern int  nSSLSecureLevel;
#endif
#ifdef  CS_RETURNMAIL // ReturnMail()中の割り込み処理禁止
extern CRITICAL_SECTION g_csReturnMail;
#endif
#ifdef  INCOM_SUB_FOLDER // incoming フォルダをスレッド別フォルダにして処理
extern CRITICAL_SECTION g_csWriteReport;
extern BOOL    bIncomingSubFolder; // incomingフォルダにスレッド別フォルダを作成 TRUE:する FALSE:しない
#endif
#ifdef UPDATE_20070521 // OSの予約語対策
extern CHAR    mReservedWords[];
#endif
#ifdef UPDATE_20140711 // エラーメール生成時の"multipart/report"に対応するオプションを追加した。
extern char    mHostName[];
extern char    mMyFDQN[];
#endif
#ifdef UPDATE_20070130 // 配送不能メールの送信者アカウントの指定をオプションを追加
extern char    mMailDaemonName[];
#endif
extern BOOL    bOldRetryMode;
extern BOOL    bSaveDead;  // deadフォルダに保管しないオプション
extern BOOL    bCompl;// 強制吐き出しオプション
extern BOOL    bVLog; // イベントビューワにログ書込みエラーを表示する 0:しない　1:する
extern BOOL    bMTAIP;             // MTAのIPを送信時設定するか否か
#ifdef UPDATE_20050901
#ifdef UPDATE_20140711 // エラーメール生成時の"multipart/report"に対応するオプションを追加した。
extern int     nReturnMailForm;
#else
extern BOOL    bReturnMailForm;
#endif
#endif
#ifdef UPDATE_20061118 // エラーメールのエンベロープのFROM:を空白にするオプション
extern BOOL    bReturnMailEnvelope;
#endif
#ifdef UPDATE_20231103 // エラーメール生成時にメールヘッダの情報を利用するオプション機能を追加。
extern char    mMailEnvTo[]; //[512];// エラーメールのエンベローブの送信元として利用するヘッダ
extern char    mMailEnvFrom[]; //[512]; // エラーメールのエンベローブの送信先として利用するヘッダ
#endif

extern BOOL    bDebug;
//extern int errno; // グローバル変数
extern DWORD   nGetRCPWait;        // 処理スレッドがRCPファイル取得時にどれだけ待つか
extern DWORD   nMaxDivide;         // ＭＬアドレスを指定数のメールとしてふりわける。起動スレッド数とは別。
extern DWORD   nDomainDivide;
extern DWORD   nMaxThread;
extern DWORD   nRunThread;
extern BOOL    bServiceTerminating;
extern int     nport;            // smtp port
extern BOOL    bLog;
extern BOOL    bFailLog;
extern BOOL    bSendLocalLog;
extern BOOL    bCheckLocalDomainIP;
//extern DWORD   nThreadNo;

char DebugMode = 'N';
char prgver[6]  = VERSION;
char sender[80] = SMTP_NAME;

#ifdef LGWAN_ON        // LGWAN機能拡張
extern BOOL bLGWAN;
extern BOOL bCHGHEADER;
#endif

#ifdef CLUSTERING
extern BOOL   nClustering;
extern char   mComName[];
#endif

#ifdef V3
extern BOOL    bInboxEnc;
#endif
extern int     nAddressFamily;
extern int     nChkTM;
extern DWORD   nSendRefusalTime;  // 受信拒絶(5xx)時のトライ回数
extern DWORD   nSendOtherTime;    // 無通信時のトライ時間
extern DWORD   nSendOther;        // 無通信時のトライ回数
extern DWORD   nSendMaxRetryTime; // 回線異常+4xxのトライ時間
extern DWORD   nSendMaxRetry;     // 回線異常+4xx送信トライ回数
extern ULONG   hTd;
extern BOOL    bTd;
extern int     nSenderLog;
#ifdef UPDATE_20060613 // senderlogでプロトコルログ のみ出力するオプション追加
extern int     nSender2Log;
extern int     nSenderlogUnit; // ログするファイルの単位指定 0:１日 1:１時間
#endif
//u_short portno  = 25;       // smtp tcp port 
extern BOOL    bOverlapFile;
extern BOOL    bRetryRule;
extern int     nSendBuf;
extern DWORD   nSendTMO;
extern DWORD   nRecvTMO;
extern DWORD   nGreetingTMO;
extern BOOL    bMRICounter;
extern BOOL    bOutlog;
extern BOOL    bOutLocallog;
extern BOOL    bFailOutlog;
extern BOOL    bSenderlog;
extern BOOL    bThread;
extern BOOL    bThreadRetry;
extern BOOL    bThreadRetry2;
extern BOOL    bThreadDomain;
extern BOOL    bThreadLists;
extern BOOL    bThreadType;
extern char    mRCPLockFn[];
extern char    mArticleLockFn[];
extern char    mRetryLockFn[];
extern char    mRetryDomainLockFn[];
extern char    mRetryListsLockFn[];
extern char    mThreadLockFn[];
extern char    mDomainsLockFn[];
extern char    mListsLockFn[];
extern char    mProgPath[];
extern char    mMailSpoolDir[];
extern char    mMailQueueDir[];
extern char    mLocalDomain[];
extern DWORD   nLocalDomain;
extern CHAR    mMailBoxDir[];
extern char    mMailHoldDir[];
extern char    mMailDomainDir[];
extern char    mPostmaster[];
extern BOOL    bMailForward;
extern BOOL    bAutoCreateInbox;
extern BOOL    bSendPostMaster;
extern char    mGetRCP[];
extern char    mWaxPipe[];
extern char    mSMTPAUTHID[];
extern char    mSMTPAUTHPASS[];
extern char    mSMTPAUTHMODE[];
extern BOOL    bAUTHTYPE[];
#ifdef UPDATE_20241217 // gateway.datにSMTP認証の指定がある時は優先するようにした。
extern BOOL    bGWAUTHTYPE[]; // gateway.datでのSMTP認証情報
#endif
extern BOOL    bESMTP;
#ifdef UPDATE_20150108 // 送信許可時間の設定機能を追加
extern int  nUseTime;
extern char mUseTimeFile[];
extern char mProgPath[];
#endif
#ifdef Y2038_BUG
extern char mMonth[12][4];
extern char mWeek[7][4];
#endif
#ifdef USE_SSL
#ifdef USE_STARTTLS
extern BOOL    bUsedSTLS;
#endif
extern BOOL    bUsedSSL;
int sendEx(SOCKET msock, SSL *ssl, char *mMess);
#endif

#ifdef UPDATE_20220414 // ゲートウェイ先ごとにSMTP認証ID,PWを規定する場合
extern char   *pAuthON;
extern char   *pAuthID;
extern char   *pAuthPW;
extern char   mGWDAT[];
extern char   mauthB64[];
#endif

#ifdef UPDATE_20230310 // SMTPゲートウェイ欄にゲートウェイ先明記ならセッションを分離しない
extern char   mSendGateway[];
extern BOOL   bIncludeForward;
#endif

#ifdef ADD_XOAUTH2_A // OAUTH2での認証方式を追加
extern char	mOAuthFile[];
#endif
#ifdef UPDATE_20240127 // ＭＬ制御応答＆送信エラー通知メールにもDKIMサインを追加可能にするオプション
// DKIM設定箇所の選択 bit0:(受信時付加)デフォルト bit1:上長承認メールへ付加 bit2:添付分離めーるへ付加
extern int    bDKIM;
extern CHAR   mDomainAUTHDKIM[];
#endif
#ifdef UPDATE_20240127C // DKIMサイン実施無効フラグを追加 1:無効(bit0:送信エラー bit1:ＭＬ制御応答 bit2:転送メール bit:3自動応答)
extern int    nOnDKIM;
#endif

#ifdef V4
BOOL QueryPermitFrom(char *pFrom, char *pFn);
#endif

#ifdef Y2038_BUG
void gettime(SYSTEMTIME *ltime, char *mt);
#else
void gettime(time_t *ltime, char *mt);
#endif

#ifdef UPDATE_20050121
void   StopLog(DWORD nCode);
#endif

#ifdef LGWAN_ON        // LGWAN機能拡張
#ifdef UPDATE_20241217_RCP // gateway.datにSMTP認証の指定が別のセッションで上書きされない対策
void LGWANControl(CHAR *pFrom, CHAR *pTo, int *nGateAuthType);
#else
void LGWANControl(CHAR *pFrom, CHAR *pTo);
#endif
#endif

DWORD GetTryServer(char *mDomain, char *mMsg, DWORD *nErrCode, BOOL bmode);
#ifdef UPDATE_20241217_RCP // gateway.datにSMTP認証の指定が別のセッションで上書きされない対策
DWORD GetSMTPServer(char *mNS, char *mDomain, char *pFrom, char *pTo, char *mMsg, char *mSMTPServer, int *nPort, BOOL *bSSL, BOOL *bNotNS, BOOL *bNotMX, int *nGateAuthType);
#else
#ifdef UPDATE_20241128 // GATEWAY.DATにエンベロープの送信元の条件も追加可能に拡張した。
DWORD GetSMTPServer(char *mNS, char *mDomain, char *pFrom, char *pTo, char *mMsg, char *mSMTPServer, int *nPort, BOOL *bSSL, BOOL *bNotNS, BOOL *bNotMX);
#else
#ifdef UPDATE_20141023 // メールアドレス単位でもGatway.datでフォワード指定可能にする対策
DWORD GetSMTPServer(char *mNS, char *mDomain, char *pTo, char *mMsg, char *mSMTPServer, int *nPort, BOOL *bSSL, BOOL *bNotNS, BOOL *bNotMX);
#else
DWORD GetSMTPServer(char *mNS, char *mDomain, char *mMsg, char *mSMTPServer, int *nPort, BOOL *bSSL, BOOL *bNotNS, BOOL *bNotMX);
#endif
#endif
#endif
BOOL GetMailControl(struct _MAIL_CTL *mc, char *mCtlFn);
BOOL CheckUser(char *mFrom, char *mRcpt, char *mAnswer, BOOL *bML, BOOL *bNoReply);
BOOL CheckLocalServer(char *mDomain);
void SendLocalMail(char *mNS, struct _RCP *rcp, char *mfn, DWORD i);
void faillog(struct _RCP *rcp);
void outlog(struct _RCP *rcp);
void outlocallog(struct _RCP *rcp);
void SetRetry(struct _RCP *rcp, BOOL bCount, DWORD nErrCode, int nCC); //BOOL bINIT);
BOOL RetryStart(char *mNS, char *mMQ, char *mDir, BOOL bmod);
BOOL CheckLists(LPCTSTR lpAppName, char *uid);
DWORD SetListsUser(LPCTSTR lpAppName, struct _RCP *rcp, char *mRCPDest, DWORD nMLDomDiv, DWORD nNoOfDiv);
void RestorRCPFile(struct _RCP *rcp, char *mRCPSrc, char *mRCPDest);
BOOL RestorMLRCPFile(struct _RCP *rcp, char *mUser, DWORD n);
void ListControl(struct _RCP *rcp, char *mUser, DWORD n);
BOOL AutoProcess(struct _MAIL_CTL *mc, char *mMBox, char *mSrc);
BOOL MailDataDiscard(struct _MAIL_CTL *mc, char *mSrcfn, char *mDestfn);
void AutoTransfar(char *mNS, struct _RCP *rcp, struct _MAIL_CTL *mc, char *mfn, DWORD n);
BOOL NoReply(struct _RCP *rcp, char *mfn);
BOOL YesReply(struct _RCP *rcp, char *mfn, char *mkey);
void AutoReply(char *mNS, struct _RCP *rcp, struct _MAIL_CTL *mc, char *mrep, BOOL bmode, DWORD n);
void RecordReplies(char *mRep, char *mRepFn, BOOL bRecordDate);
int Base64DecodeLine(char * pszLine, int nLineLen, unsigned char * uchBuffer, int nBufferLen);
void translateUue2Base64(char *line, int inlen, char *newline);
void hmac_md5(unsigned char* text, unsigned char* key, char *dest);
#ifdef UPDATE_20241217_RCP // gateway.datにSMTP認証の指定が別のセッションで上書きされない対策
BOOL GetGatewayList(char *mDom, char *pFrom, char *pTo, char *mGate, int *nPort, BOOL *bSSL, int *nGateAuthType);
#else
#ifdef UPDATE_20241128 // GATEWAY.DATにエンベロープの送信元の条件も追加可能に拡張した。
BOOL GetGatewayList(char *mDom, char *pFrom, char *pTo, char *mGate, int *nPort, BOOL *bSSL);
#else
#ifdef UPDATE_20141023 // メールアドレス単位でもGatway.datでフォワード指定可能にする対策
BOOL GetGatewayList(char *mDom, char *pTo, char *mGate, int *nPort, BOOL *bSSL);
#else
BOOL GetGatewayList(char *mDom, char *mGate, int *nPort, BOOL *bSSL);
#endif
#endif
#endif
#ifdef UPDATE_20240127 // ＭＬ制御応答＆送信エラー通知メールにもDKIMサインを追加可能にするオプション
void AddDKIMHeader(char *pMsg, char *pDomain);
#endif
#ifdef UPDATE_20241124 // OAUTH2での認証方式用にユーザ別認証コード保管ファイル(xoauth2_code.dat)を各アカウントフォルダに設定可能にした
char * GetXOAUTH2Path(char *pDir, char *pUser, char *pFn);
BOOL GetXOAUTH2CODEFile(char  *pDir, char *pUser, char *mpass);
#endif
#ifdef WAX_ON
// Send Pipe Line Messages
void SendPipeLine(char *str) {
  char    inbuf[80];
  char    outbuf[80];
  LPTSTR  lpszPipeName; //= TEXT("\\\\.\\pipe\\waxsvc");
  DWORD   bytesRead;

  // WAX 受信部に通信する。
  if (strlen(mWaxPipe) > 0) {
    lpszPipeName = (LPTSTR)mWaxPipe;
    strncpy( outbuf, str, 80); outbuf[79] = '\x0';
    CallNamedPipe(lpszPipeName,
                outbuf, sizeof(outbuf),
                inbuf, sizeof(inbuf),
                &bytesRead, NMPWAIT_NOWAIT);
  }
}
#endif

void printfTrace(char *str) 
{
#ifdef Y2038_BUG
	SYSTEMTIME lt;
#else
	time_t ltime;
    struct tm lt;
#endif
  char   mdata[128], fn[256];
  FILE   *fp;

#ifdef  INCOM_SUB_FOLDER // incoming フォルダをスレッド別フォルダにして処理
  EnterCriticalSection(&g_csWriteReport);
#endif
  while(bSenderlog) {
    if (bServiceTerminating)
      return;
    _sleep(WAIT_TIME);
  }
  bSenderlog = TRUE;

#ifdef Y2038_BUG
  GetLocalTime(&lt); 
#ifdef UPDATE_20060613 // senderlogでプロトコルログ のみ出力するオプション追加
  if (nSenderlogUnit)
    sprintf(mdata, "%02d%02d%02d_%02d", (lt.wYear%100), lt.wMonth, lt.wDay, lt.wHour);
  else
#endif
  sprintf(mdata, "%02d%02d%02d", (lt.wYear%100), lt.wMonth, lt.wDay );
#else
  time( &ltime );                	 
  lt = *localtime(&ltime);
#ifdef UPDATE_20060613 // senderlogでプロトコルログ のみ出力するオプション追加
  if (nSenderlogUnit)
    strftime( mdata, sizeof(mdata), "%y%m%d_%H", &lt );
  else
#endif
  strftime( mdata, sizeof(mdata), "%y%m%d", &lt );
#endif
#ifdef UPDATE_20260606 // ログの書き込み信頼性向上対策
      WaitForSingleObject(g_hMutexOutSenderLog, INFINITE);  // 排他開始
      fpOutSenderLog = OpenCloseLog(fpOutSenderLog, mDTOutSenderLog, "senderlog", mComName, lt);
  if (fpOutSenderLog) {
    //fprintf(fp, str); // これだと、%文字があるとハングする。
#ifdef Y2038_BUG
    fprintf(fpOutSenderLog, "[%02d:%02d:%02d:%03d] %s", lt.wHour, lt.wMinute, lt.wSecond, lt.wMilliseconds, str);
#else
    fprintf(fpOutSenderLog, "[%02d:%02d:%02d] %s", lt.tm_hour, lt.tm_min, lt.tm_sec, str);
#endif
#ifdef UPDATE_20060615 // データのディスク書込に異常が有った場合の対策
    if (fflush(fpOutSenderLog) == EOF)  // 書込み異常はイベントビューワに
	  if (bVLog)
	    AddToMessageLog(TEXT("senderlog write fail."), 102, TEXT(SMTP_SERVICE), EVENTLOG_WARNING_TYPE );
#endif
  }
#else
#ifdef REGTOFILE
      if (nClustering) {
	    sprintf(fn, "%s\\senderlog\\%s\\%s.log", mMailSpoolDir, mComName, mdata);
	  } else {
#endif
  sprintf(fn, "%s\\senderlog\\%s.log", mMailSpoolDir, mdata);
#ifdef REGTOFILE
	  }
#endif
#ifdef UPDATE_20260603 // ログ出力の排他処理化を追加
  WaitForSingleObject(g_hMutexOutSenderLog, INFINITE);  // 排他開始
#endif
  fp = fopen(fn, "at");
#ifdef UPDATE_20260605 // 詳細ログファイルのオープン失敗リトライ処理の追加
  if (!fp) 
  {
    Sleep(200); //200ms
    fp = fopen(fn, "at");
    if (!fp) 
    {
      Sleep(200); //200ms
      fp = fopen(fn, "at");
    }
  }
#endif
  if (fp) {
    //fprintf(fp, str); // これだと、%文字があるとハングする。
#ifdef Y2038_BUG
    fprintf(fp, "[%02d:%02d:%02d:%03d] %s", lt.wHour, lt.wMinute, lt.wSecond, lt.wMilliseconds, str);
#else
    fprintf(fp, "[%02d:%02d:%02d] %s", lt.tm_hour, lt.tm_min, lt.tm_sec, str);
#endif
#ifdef UPDATE_20060615 // データのディスク書込に異常が有った場合の対策
    if (fflush(fp) == EOF)  // 書込み異常はイベントビューワに
	  if (bVLog)
	    AddToMessageLog(TEXT("senderlog write fail."), 102, TEXT(SMTP_SERVICE), EVENTLOG_WARNING_TYPE );
#endif
    fclose(fp);
  }
#endif
#ifdef UPDATE_20260603 // ログ出力の排他処理化を追加
  ReleaseMutex(g_hMutexOutSenderLog);  // 排他終了
#endif
  bSenderlog = FALSE;
#ifdef  INCOM_SUB_FOLDER // incoming フォルダをスレッド別フォルダにして処理
  LeaveCriticalSection(&g_csWriteReport);
#endif
}

DWORD X_CopyFile(char *pSrc, char *pDest, BOOL bType) {
  //bType TRUE:ファイルがあると書き込まない。
  //bType FALSE:ファイルへ上書き。
#ifdef DATA_CASH
  int hds, hdd, n;
  FILE *fds, *fdd;
  struct _stat st; 
  struct _utimbuf ut;
  char mBuff[0x8000]; //4096];
  DWORD nSts = 1;
#else 
  int hds, hdd, n;
  struct _stat st; 
  struct _utimbuf ut;
  char mBuff[0x8000]; //4096];
  DWORD nSts = 1;
#endif

  //rename(pSrc, pDest);
  while((hds = _open( pSrc, _O_RDONLY | _O_BINARY)) == -1) {
    if (bServiceTerminating)
  	  break;
	  _sleep(WAIT_TIME);
  }
#ifdef DATA_CASH
  if (hds) {
	_fstat( hds,  &st );
    if ((fds = _fdopen( hds, "r" ))){
	  if ((fdd = fopen( pDest, "wb"))) {
		while((n = fread( mBuff, sizeof(char), sizeof(mBuff), fds)))
          fwrite( mBuff, sizeof(char), n , fdd);
		fclose(fdd);
	    ut.actime  = st.st_atime;
        ut.modtime = st.st_mtime;
        _utime(pDest, &ut);
	  } else {
	   if (!bType) // 上書きできない
	     nSts = 0;
	  }
	  fclose(fds);
	}
  } else {
    if (!bType) // 上書きできない
	  nSts = 0;
  }
#else
  if (hds) {
	_fstat( hds,  &st );
    if ((hdd = _open( pDest, _O_WRONLY | _O_BINARY | (bType ? _O_CREAT | _O_EXCL : _O_CREAT ), _S_IREAD | _S_IWRITE)) != -1) {
      while((n = _read( hds, mBuff, sizeof(mBuff))))
	    _write( hdd, mBuff, n );
	  _close(hdd);
      ut.actime  = st.st_atime;
      ut.modtime = st.st_mtime;
      _utime(pDest, &ut);
	} else {
	  if (!bType) // 上書きできない
	    nSts = 0;
	}
	_close(hds);
  } else {
    if (!bType) // 上書きできない
	  nSts = 0;
  }
#endif
  return nSts;
}

DWORD X_MoveFile(char *pSrc, char *pDest) {
#ifdef TRACE_MODE
   char str[LOG_BUFFER];
#endif
 DWORD dErr;
 FILE  *fp;

  dErr = 0;

    while(rename(pSrc, pDest)) { //while(!MoveFile(pSrc, pDest)) { // 転送先 RCPファイル
       if (bServiceTerminating)
	     break;
	   dErr = GetLastError();
	   if (bDebug)
         printf("Failed - MoveFile(%s, %s) Failed. Status = %d\n", pSrc, pDest, dErr);
#ifdef TRACE_MODE
       if (nSenderLog) {
         sprintf(str, ":X_MoveFile(%s, %s) Failed. Status = %d\n", pSrc, pDest, dErr);
         printfTrace(str);
	   }
#endif
#ifdef UPDATE_20061128 // リネーム要求が繰り返された場合もウエイトしない
	   if (dErr == (DWORD)183 ||
		   dErr == ERROR_ALREADY_EXISTS ||
		   dErr == ERROR_FILE_NOT_FOUND ||
		   dErr == ERROR_SHARING_VIOLATION)
	     break;
#else
	   if (dErr == ERROR_FILE_NOT_FOUND ||
		   dErr == ERROR_SHARING_VIOLATION)
	     break;
#endif
       _sleep(WAIT_TIME);
	   dErr = 0;
	}
#ifdef UPDATE_20050122
    //// リネームが正常に完了するまでウエイト
    if (nSenderLog) {
      sprintf(str, ":X_MoveFile(%s, %s) Complete check.\n", pSrc, pDest, (dErr == ERROR_SUCCESS ? "Success" : "Failed"));
      printfTrace(str);
	}
	/*
    while(!(fp = fopen(pDest, "rt"))) { 
      if (bServiceTerminating)
       break;
       _sleep(WAIT_TIME);
	}
    fclose(fp);
	*/
#endif
#ifdef TRACE_MODE
    if (nSenderLog) {
      sprintf(str, ":X_MoveFile(%s, %s) Success.\n", pSrc, pDest, (dErr == ERROR_SUCCESS ? "Success" : "Failed"));
      printfTrace(str);
	}
#endif

  return dErr;
}

BOOL SendGlobalMailStatus(char *mDomain, char *mMsg, BOOL sts, BOOL btyp) {
  DWORD nTry, nMaxTry, nCount;
  BOOL bsts, bMxmode;
  char str[LOG_BUFFER];
  char *pMsg, mMsgNo[256];
#ifndef UPDATE_20061129 // 同報処理内での転送エラーメール発生時にエラーメールのファイル名が重複する不具合。
#ifdef UPDATE_20040525
  char mDom[256];

  sprintf(mDom, "@%s", mDomain);
  if (CheckDomain(mDom)) { // ローカルドメインのメールがエラーなら即エラーリターン
	 return FALSE;
  }
#endif
#endif
#ifdef UPDATE_20220414 // ゲートウェイ先ごとにSMTP認証ID,PWを規定する場合
   pAuthON = NULL;
   pAuthID = NULL;
   pAuthPW = NULL;
#endif

  //nTry = (DWORD)(btyp ? 10 : nSendMaxRetry);
  //nTry = (DWORD)(btyp ? 9 : nSendMaxRetry-1);
  nTry = (DWORD)(btyp ? nSendRefusalTime : nSendMaxRetry-1);
  nMaxTry = GetTryServer(mDomain, mMsg, NULL, TRUE);
  nCount  = GetTryServer(mDomain, mMsg, NULL, FALSE);
#ifdef UPDATE_20071207 // 受信拒否時のリトライ回数が多くなる
  bMxmode = (sts >=500 ? TRUE: (nCount % 2 ?  FALSE : TRUE));
#else
  bMxmode = (nCount % 2 ?  FALSE : TRUE);
#endif
  //if (sts >= 505 && sts != 554 && nMaxTry < (DWORD)(btyp ? 10 : nSendMaxRetry) || // 中継拒否の場合10回までリトライさせる。
  //if (sts >= 505 && sts != 554 && nMaxTry < (DWORD)(btyp ? 2 : nSendMaxRetry) || // 中継拒否の場合10回までリトライさせる。
#ifdef UPDATE_20060629 // 400番台のリトライが500番台に切替えられるてしまう。
   if (bOldRetryMode) {
#endif
     if (sts >= 505 && sts != 554 && nMaxTry < nTry || // 中継拒否の場合10回までリトライさせる。
	    !bMxmode ||
        sts == (int)INVALID_SOCKET ||
	    sts >= 400 && sts < 500 || //回線異常
	    //sts == (int) 421 || // docomo 回線異常
        //sts == (int) 451 || // @nifty 回線異常
        sts == (int) FALSE)
       bsts = TRUE;
     else
 	   bsts = FALSE;
#ifdef UPDATE_20060629 // 400番台のリトライが500番台に切替えられるてしまう。
   } else {
     if (sts >= 500 && nMaxTry < nSendRefusalTime ||                                  // 500番台 (中継拒否)
	     sts >= 400 && sts < 500 && nMaxTry < nSendMaxRetry ||                        // 400番台 (回線異常)
		 (sts == (int)INVALID_SOCKET || sts == (int)FALSE) && nMaxTry < nSendOther || //（無通信）
         !bMxmode )
       bsts = TRUE;
     else
 	   bsts = FALSE;
   }
#endif

  pMsg = strrchr( mMsg, '\\');
  if (pMsg) 
    strcpy(mMsgNo, ++pMsg);
  else
    strcpy(mMsgNo, mMsg);
  ///////////////////////////// 2001.12.30
  //strtok(mMsgNo, ".");
  pMsg = strrchr(mMsgNo, '.');
  if (pMsg)
    *pMsg = '\x0';
  /////////////////////////////////////////

  if (bDebug)
    printf("[%s] SendGlobalMailStatus:%s:return=%d\n", mMsgNo, mDomain, bsts);
#ifdef TRACE_MODE
#ifdef UPDATE_20060904A // ロックファイルの作成確認
if (nSenderLog || nSender2Log)
#else
if (nSenderLog)
#endif
{
  sprintf(str, "[%s] SendGlobalMailStatus:%s:return=%d\n", mMsgNo, mDomain, bsts);
  printfTrace(str);
}
#endif
  return bsts;
}

void strSkipWSP(char *pstr) {
  char *p;

    p = pstr;
	while (*p == ' ' || *p == '\t')
	  p++;
	if (p != pstr)
	  strcpy(pstr, p);
}

void strword(char *pstr, char *c1, char *c2) {
   char *p, *q;

#ifdef UPDATE_20070319C // データが空ならキャラクタ検索で見つからない場合ハングする
   if (!pstr)
	 return;
   if (*pstr == '\x0')
	 return;
#endif
    p = strpbrk(pstr, c1);
	if (p) {
	  q = strpbrk(++p, c2);
	  if (q) 
		*q = '\x0';
	  //strtok(++p, c2);
	  strcpy(pstr, p);
	} else {
	  q = strpbrk(pstr, c2);
	  if (q)
		*q = '\x0';
	}
}

void InitRCP(struct _RCP *rcp) {
  *rcp->mMID       = '\x0';
  *rcp->mMIDNo     = '\x0';
  *rcp->mMNo       = '\x0';
#ifdef SENDERID
  *rcp->mSubmitter = '\x0';
#endif  
  *rcp->mFrom      = '\x0';
  *rcp->mTo        = '\x0';
  *rcp->mCurrentTo = '\x0';
  *rcp->mDomain    = '\x0';
  *rcp->mSmtp      = '\x0';
  *rcp->mSmtpIP    = '\x0';
#ifdef UPDATE_20170215 // 配送スレッドの変数初期化抜けの修正
  *rcp->mBSmtp     = '\x0';
  *rcp->mBSmtpIP   = '\x0';
#endif
  *rcp->mMsg       = '\x0';
  *rcp->mRcp       = '\x0';
#ifdef UPDATE_20170215 // 配送スレッドの変数初期化抜けの修正
  *rcp->mToOK      = '\x0';
  *rcp->mToNG      = '\x0';
#endif
  rcp->nPort       = nport;
#ifdef UPDATE_20170215 // 配送スレッドの変数初期化抜けの修正
  rcp->phe         = NULL;
  rcp->bUseTime    = FALSE;
#endif
#ifdef USE_SSL
  rcp->bUsedSSL    = FALSE;
  rcp->ssl         = NULL;
#endif
#ifdef UPDATE_20060725 // 配送ログへ出力フラグの初期化
  rcp->nToOK        = 0;
  rcp->nToNG        = 0;
#endif
#ifdef UPDATE_20170215 // 配送スレッドの変数初期化抜けの修正
  *rcp->mLogTo     = '\x0';
  rcp->bHaed       = FALSE;
  rcp->bGreeting   = FALSE;
  rcp->bNego       = FALSE;
  *rcp->mAnswer    = '\x0';
  *rcp->mRCPAnswer = '\x0';
  rcp->nConnectRetry = 0;
  rcp->grst.dwLowDateTime  = 0;
  rcp->grst.dwHighDateTime = 0;
  rcp->gret.dwLowDateTime  = 0;
  rcp->gret.dwHighDateTime = 0;
  *rcp->mSetRetryMode = '\x0';
#endif
}

// get a response from the mail server. Return true if it's in the
// range 200-399 (good returns), otherwise return false, and set $status
// to 0.
#ifdef USE_SSL
BOOL get_reply(SOCKET msock, SSL *ssl, char *junk)
#else
BOOL get_reply(SOCKET msock, char *junk)
#endif
{
#ifdef TRACE_MODE
   char str[LOG_BUFFER];
#endif
  int  retcode, i, n, cnt, l, err;
  char buffer[2048];
  char *o, *p; //, *q;
#ifdef UPADTE_20040202
   int    ns;
   fd_set ds;
   struct timeval tmo;
#endif

#ifdef UPDATE_20170214 // データ受信前に以前の応答データがクリアされるようにした
  if (junk)
  {
    *junk = '\x0';
  }
#endif
#ifndef TUNING 
  memset(buffer, 0, sizeof(buffer));
#else
  buffer[0] = '\x0';
#endif
  i = 0;
  l = n = 0;
  cnt = 0;
  o = buffer;
  do {
	if (bServiceTerminating) {
	  if (junk)
        sprintf(junk, "451 Requested action aborted: stoped in processing.");
      if (bDebug)
        printf("451 Requested action aborted: stoped in processing.\n");
      return FALSE;
	}
    n += l; //strlen(buffer);
    if (n >= ((sizeof(buffer)/2)-1)) {
	  if (junk)
        sprintf(junk, "451 Requested action aborted: overflow of token. (size=%d)", n);
      if (bDebug)
        printf("451 Requested action aborted: overflow of token.\n");
      return FALSE;
	}
	l = 0;
#ifdef UPDATE_20050518
     if (nRecvTMO) {
       tmo.tv_sec = (int)nRecvTMO; // 1秒単位
 	   tmo.tv_usec = 0;            // microseconds 
	   FD_ZERO(&ds);
	   FD_SET(msock, &ds);
	   if ((ns = select(msock, &ds, (fd_set *)NULL, (fd_set *)NULL, &tmo)) > 0) {
#ifdef USE_SSL
		 if (ssl) {
           l = SSL_read(ssl, &buffer[n], (sizeof(buffer)/2)-1-n);
		   if (l == -1)
		     l = 0;
		 } else
#endif
           l = recv(msock, &buffer[n], (sizeof(buffer)/2)-1-n, 0);
	   } else {
         l = SOCKET_ERROR;
	   }
	 } else {
#ifdef USE_SSL
	  if (ssl) {
        l = SSL_read(ssl, &buffer[n], (sizeof(buffer)/2)-1-n);
	    if (l == -1)
		  l = 0;
	  } else
#endif
       l = recv(msock, &buffer[n], (sizeof(buffer)/2)-1-n, 0);
	 }
#else
#ifdef USE_SSL
	if (ssl)
      l = SSL_read(ssl, &buffer[n], (sizeof(buffer)/2)-1-n);
	else
#endif
#ifdef UPADTE_20040202
     if (nRecvTMO) {
       tmo.tv_sec = (int)nRecvTMO; // 1秒単位
 	   tmo.tv_usec = 0;            // microseconds 
	   FD_ZERO(&ds);
	   FD_SET(msock, &ds);
	   if ((ns = select(msock, &ds, (fd_set *)NULL, (fd_set *)NULL, &tmo)) > 0) {
#ifdef UPDATE_20050121
		 if (ns == SOCKET_ERROR) {
		   l = SOCKET_ERROR;
		   StopLog(GetLastError());
		 } else
#endif
           l = recv(msock, &buffer[n], (sizeof(buffer)/2)-1-n, 0);
		   
	   } else {
         l = SOCKET_ERROR;
	   }
	 } else
#endif
       l = recv(msock, &buffer[n], (sizeof(buffer)/2)-1-n, 0);
#endif
#ifdef UPDATE_20230927 // 受信直後バッファ内容ののトレース
#ifdef TRACE_MODE
if (nSenderLog || nSender2Log) {
	sprintf(str, "%d:get_reply():size=%d:token=%s\n", msock, l, &buffer[n]);
  printfTrace(str);
}
#endif
#endif
// // 以下の指定だとハングする？でも正しい....
    if (l == SOCKET_ERROR || 
		l == 0 ) {
      err = WSAGetLastError();
	  if (junk)
        sprintf(junk, "451 Requested action aborted: error in processing.(code=%d/%x)\n", err, l);
#ifdef TRACE_MODE
      sprintf(str, "451 Requested action aborted: error in processing. (code=%d/%x)\n", err, l);
      if (bDebug)
        printf("%s", str);
      if (nSenderLog || nSender2Log) {
        printfTrace(str);
	  }
#endif
      return FALSE;
	} 
	buffer[n+l] = '\x0';
    if (bDebug)
      printf("%s", &buffer[n]);
    p = strchr(buffer,'\n');
#ifdef ESMTP_ON
	// ESMTP 対応  /////////////////////////////
	if (p) {
      while(1) {
		if (strlen(o) <= 4) {
          p = strchr(o,'\n');
          break;
		}
        if (*(o+3) == ' ') { 
          p = strchr(o,'\n');
		  break;
		} else {
	      o = p + 1;
          p = strchr(o,'\n');
		  if (!p)
			break;
		}
	  }
	}
	///////////////////////////////////////////
#endif
  } while(!p);
  
  if (junk)
    strcpy(junk, buffer);
  strtok(buffer," \r\n");
  retcode = atoi(buffer);

  if ((retcode >= 200) && (retcode < 400))
    return TRUE;
  else
    return FALSE;
}

BOOL FillAddr(PSOCKADDR_IN psin, struct _RCP *rcp) {
#ifdef TRACE_MODE
   char str[LOG_BUFFER];
#endif
   //PHOSTENT phe;
   unsigned long inaddr;
   char *paddr;

   psin->sin_family = AF_INET;
   if (bDebug)
     printf("gethostbyname(%s)", rcp->mSmtp);
#ifdef TRACE_MODE
if (nSenderLog || nSender2Log) {
  sprintf(str, "%s:FillAddr:domain=%s\n", rcp->mMNo, rcp->mSmtp);
  printfTrace(str);
}
#endif
   if (!rcp->phe) {
     inaddr = inet_addr(rcp->mSmtp);
     if (inaddr != INADDR_NONE) { // = (unsigned long)0xffffffff) {
       memcpy((char FAR *)&(psin->sin_addr), &inaddr, 4);
	   psin->sin_port = htons((u_short)rcp->nPort);        // Convert to network ordering 
       paddr = inet_ntoa(psin->sin_addr);
	   if (paddr) {
         strcpy(rcp->mSmtpIP, paddr);
	   } else {
		 if (bDebug) {
           printf("Make sure '%s' is listed in the hosts file.\n", rcp->mDomain); //rcp->mSmtp);
           printf("inet_ntoa() failed.\n");
		 }
         return FALSE;
	   }
	 } else { // 最初の場合
       rcp->phe = gethostbyname(rcp->mSmtp);
       if (rcp->phe) {
         memcpy((char FAR *)&(psin->sin_addr), rcp->phe->h_addr, rcp->phe->h_length);
         psin->sin_port = htons((u_short)rcp->nPort);        // Convert to network ordering 
         paddr = inet_ntoa(psin->sin_addr);
  	     if (paddr) {
           strcpy(rcp->mSmtpIP, paddr);
		 } else {
		   if (bDebug) {
             printf("Make sure '%s' is listed in the hosts file.\n", rcp->mDomain); //rcp->mSmtp);
             printf("inet_ntoa() failed.\n");
		   }
           return FALSE;
		 }
	   } else {
		 if (bDebug) {
		   printf("%d is the error. Make sure '%s' is listed in the hosts file.\n", WSAGetLastError(), rcp->mSmtp);
           printf("gethostbyname() failed.\n");
		 }
         return FALSE;
	   }
	 }
   } else { // ラウンドロビン対策、別のIPがある場合
     memcpy((char FAR *)&(psin->sin_addr), rcp->phe->h_addr, rcp->phe->h_length);
     psin->sin_port = htons((u_short)rcp->nPort);        // Convert to network ordering 
     paddr = inet_ntoa(psin->sin_addr);
  	 if (paddr) {
       strcpy(rcp->mSmtpIP, paddr);
	 } else {
	   if (bDebug) {
		 printf("Make sure '%s' is listed in the hosts file.\n", rcp->mDomain); //rcp->mSmtp);
         printf("inet_ntoa() failed.\n");
	   }
       return FALSE;
	 }
   }
   if (bDebug)
     printf(" / IP=%s\n", rcp->mSmtpIP);
   return TRUE;
}

SOCKET ConnectHost(struct _RCP *rcp) {
#ifdef TRACE_MODE
   char str[LOG_BUFFER];
#endif
   SOCKET msock;
   int    nSockErr;
   int    nSndsize, nl;
#ifdef IPv6
    struct addrinfo Hints, *AddrInfo, *AI, *AI2;
	char   mport[8];
	int    nAF, RetVal;
#else
   SOCKADDR_IN  dest_sin;  /* DESTination Socket INternet */
   short        h_length;
#endif
   int timeout;// = 60000 * 60; // 受信タイムアウトは60分;
   char mNo[256], *pNo;
   //BOOL bKeepAlive;
#ifdef USE_SSL
    SSL_CTX *ctx;
#endif
#ifdef UPDATE_20060529 // 設定ドメイン毎のＩＰに送信元ＩＰを切替える設定
#ifdef IPv6
   struct addrinfo *LocalAI;
#ifdef UPDATE_20070122 // 設定ドメイン毎のＩＰに送信元ＩＰを切替える設定の修正
   short  *pPort;
   BOOL    on = 1;
#endif
#else
   SOCKADDR_IN sin; 
#endif
   char mIP[64] = "", *pdom;
#endif   

#ifdef DNS_TEST
       return INVALID_SOCKET;
#endif
#ifdef UPDATE_20070521 // 予約語対策
 CHAR mKey[256];
#endif

#ifdef UPDATE_20060609 // ソケット接続失敗時のエラーメールに記載するメッセージ
	      strcpy(rcp->mAnswer, "451 socket connect failed.");
#endif
#ifdef USE_SSL
	    rcp->ssl   = NULL;
		if (bUsedSSL || rcp->bUsedSSL) {  // SSLを使う場合は証明書と個人鍵を設定する。
		  if (rcp->bUsedSSL) {
#ifdef UPDATE_OPESSL1_1_1 // openssl1.1.1系に置換えた（SSL2を廃止し、TSL1.3を使用可能にする為）
            ctx = SSL_CTX_new(TLS_client_method()); // SSL2 or SSL3を使用
#else
#ifndef UPDATE_20060512 // SSLの初期化はサービス開始で１回のみ
#ifdef UPDATE_20060322 // SSLの初期化に失敗すると以降でハングする
		    SSL_library_init();
#endif
            SSLeay_add_ssl_algorithms();
            SSL_load_error_strings();
            ctx = SSL_CTX_new(SSLv23_client_method()); // SSL2 or SSL3を使用
#endif
#endif
#ifdef UPDATE_20060329 // SSLの初期化に失敗すると以降でハングする
            if (ctx == NULL)  {
              if (bDebug)
                printf("SSL_CTX_new() failed SMTP=[%s] PORT=[%d]\n", rcp->mSmtp, rcp->nPort);
#ifdef TRACE_MODE
if (nSenderLog || nSender2Log) {
  sprintf(str, "ConnectHost():SSL_CTX_new() failed SMTP=[%s] PORT=[%d]\n", rcp->mSmtp, rcp->nPort);
  printfTrace(str);
}
#endif
			}
#endif
#ifdef TRACE_MODE
if (nSenderLog || nSender2Log) {
                sprintf(str, "ConnectHost():SSL_CTX_new() = %d\n", ctx);
                printfTrace(str);
}
#endif
#ifdef UPDATE_20240420 // 暗号化通信方式のセキュリティレベルの設定
          // if (bDebug) printf("SSL security level =%d\n", SSL_CTX_get_security_level(ctx));
          SSL_CTX_set_security_level(ctx, nSSLSecureLevel);
#ifdef TRACE_MODE
if (nSenderLog || nSender2Log) {
  sprintf(str, "ConnectHost():SSL_CTX_get_security_level() = %d\n", SSL_CTX_get_security_level(ctx));
  printfTrace(str);
}
#endif
#endif
#ifdef UPDATE_20171211A // 使用可能な暗号化通信方式のバージョンを規定するオプション 0x1:SSL2禁止 0x2:SSL3禁止 0x4:TSL1禁止 0x8:TSL1.1禁止 0x10:TSL1.2禁止
            SSL_CTX_set_options(ctx, nSecuerLayOption);
#endif
#ifdef UPDATE_20190426 // 利用する暗号アルゴリズムの組み合わせオプション 
            if (mSecuerLayCipher[0])
			  SSL_CTX_set_cipher_list(ctx, mSecuerLayCipher);
		    if (bDebug)
			  Print_SSL_CIPHER_names(ctx);
#endif
            rcp->ctx = ctx;
            rcp->ssl = SSL_new(ctx); // SSL構造作成
#ifdef UPDATE_20060322 // SSLの初期化に失敗した場合のメッセージ
			if (rcp->ssl ==  NULL) {
              if (bDebug)
                printf("SSL_new() failed SMTP=[%s] PORT=[%d]\n", rcp->mSmtp, rcp->nPort);
#ifdef TRACE_MODE
if (nSenderLog || nSender2Log) {
  sprintf(str, "ConnectHost():SSL_new() failed SMTP=[%s] PORT=[%d]\n", rcp->mSmtp, rcp->nPort);
  printfTrace(str);
}
#endif
			}
#endif
		  }
		} 
#endif
  /////////////////////////////////////////
  strcpy(mNo, rcp->mMsg);
  //strtok(mNo, ".");
  pNo = strrchr(mNo, '.');
  if (pNo)
	*pNo = '\x0';
  pNo = strrchr(mNo, '\\');
  if (pNo)
	pNo++;
  else
	pNo = mNo;
  /////////////////////////////////////////

#ifdef TRACE_MODE
if (nSenderLog) {
  sprintf(str, "[%s] ConnectHost:domain=%s\n", pNo, rcp->mTo);
  printfTrace(str);
}
#endif

#ifdef IPv6
    nAF = nAddressFamily;
#ifdef UPDATE_20121105 // IPv4/v6併用で相手FQDNにIPv6が有効なのにIPv4のみでSMTPサーバが接続可能な場合接続できない不具合
retry_ipv6nextv4:
#endif
    memset(&Hints, 0, sizeof(Hints));
	//不要 Hints.ai_flags = AI_NUMERICHOST | AI_PASSIVE;
    Hints.ai_family =  (nAF ? AF_INET6 : AF_INET);
    Hints.ai_socktype = SOCK_STREAM;
	//itoa(nport, mport, 10);
	itoa(rcp->nPort, mport, 10);
    RetVal = getaddrinfo(rcp->mSmtp, mport, &Hints, &AddrInfo);
    if (RetVal != 0) {
	  nAF =	(nAddressFamily ? 0 : 1); // AddressFamiry を切替える
      memset(&Hints, 0, sizeof(Hints));
	  //不要 Hints.ai_flags = AI_NUMERICHOST | AI_PASSIVE;
      Hints.ai_family =  (nAF ? AF_INET6 : AF_INET);
      Hints.ai_socktype = SOCK_STREAM;
	  //itoa(nport, mport, 10);
      itoa(rcp->nPort, mport, 10);
      RetVal = getaddrinfo(rcp->mSmtp, mport, &Hints, &AddrInfo);
      if (RetVal != 0) {
        if (bDebug)
          printf("Cannot resolve address [%s] and port [%s], error %d: %s\n", rcp->mSmtp, mport, RetVal, gai_strerror(RetVal));
        //WSACleanup();
        return -1; // INVALID_SOCKET
	  }
    }
	AI = AddrInfo;
#ifdef TRACE_MODE
if (nSenderLog) {
  sprintf(str, "[%s] getaddrinfo:domain=%s\n", pNo, rcp->mSmtp);
  printfTrace(str);
}
#endif
    msock = socket((nAF ? AF_INET6 : AF_INET), SOCK_STREAM, 0);
    if (msock != INVALID_SOCKET) {
#ifdef UPDATE_20060529 // 設定ドメイン毎のＩＰに送信元ＩＰを切替える設定
	  if (bMTAIP) {             // MTAのIPを送信時設定するか否か
	    if ((pdom = strrchr(rcp->mFrom, '@'))) {
          GetMTAIP((LPCTSTR)(pdom+1), (LPTSTR)mIP);
		  if (bDebug) printf("GetMTAIP [%s] [%s] len=%d \n", (pdom+1), mIP, strlen(mIP));
#ifdef TRACE_MODE
          if (nSenderLog) {
            sprintf(str, "GetMTAIP [%s] [%s] len=%d \n", (pdom+1), mIP, strlen(mIP));
            printfTrace(str);
		  }
#endif
		  if (!mIP[0]) {
            GetProfileStringEx((LPCTSTR)DOMAIN_SMTPIP,(LPCTSTR)(pdom+1),(LPCTSTR)"",(LPTSTR)mIP, sizeof(mIP));
#ifdef UPDATE_20070521 // 予約語対策
            strcpy(mKey, pdom+1);
            strtok(mKey, "@");
            if (ReservedWords(mKey)) {
              strcpy(mKey, mReservedWords);
	          strcat(mKey, pdom+1);
	          GetProfileStringEx((LPCTSTR)DOMAIN_SMTPIP, (LPCTSTR)mKey, (LPCTSTR)"", (LPTSTR)mIP, (DWORD)sizeof(mIP)); // 対象ドメインの応答ＩＰアドレス取出し
			}
#endif
		    strtok(mIP, " ");
		    if (bDebug) printf("Get DOMAIN_SMTPIP [%s] [%s] len=%d\n", (pdom+1), mIP, strlen(mIP));
#ifdef TRACE_MODE
            if (nSenderLog) {
              sprintf(str, "Get DOMAIN_SMTPIP [%s] [%s] len=%d\n", (pdom+1), mIP, strlen(mIP));
              printfTrace(str);
			}
#endif
		  }
		  if (mIP[0]) {
            memset(&Hints, 0, sizeof(Hints));
			Hints.ai_flags = AI_NUMERICHOST | AI_PASSIVE;
            Hints.ai_family =  (nAF ? AF_INET6 : AF_INET);
            Hints.ai_socktype = SOCK_STREAM;
			itoa(rcp->nPort, mport, 10);
            RetVal = getaddrinfo(mIP, mport, &Hints, &LocalAI);
            if (RetVal == 0) {
              if (bDebug) printf("Resolve address [%s] and port [%d], code %d\n", mIP, rcp->nPort, RetVal);
#ifdef TRACE_MODE
              if (nSenderLog) {
                sprintf(str, "Resolve address [%s] and port [%d], code %d\n", mIP, rcp->nPort, RetVal);
                printfTrace(str);
			  }
#endif
#ifdef UPDATE_20070122 // 設定ドメイン毎のＩＰに送信元ＩＰを切替える設定の修正
			  /// getaddrinf()で設定したポートのバイト位置がおかしい為。
			  pPort = (int *)(LocalAI->ai_addr)->sa_data;
			  *pPort = rcp->nPort;
              /////////////////////////////////////////////////////
		      //bindがエラーにならない対策
              setsockopt(msock, SOL_SOCKET, SO_REUSEADDR, (const char *)&on, sizeof(on));
              /////////////////////////////////////////////////////
#endif
	          if (bind(msock, LocalAI->ai_addr, LocalAI->ai_addrlen) == SOCKET_ERROR) {
		        if (bDebug) printf("bind local adder fail. (%s)\n", mIP);
#ifdef TRACE_MODE
                if (nSenderLog) {
                  sprintf(str, "bind local adder fail. (%s)\n", mIP);
                  printfTrace(str);
				}
#endif
			  }
		      freeaddrinfo(LocalAI);
			} else {
              if (bDebug) printf("Resolve address [%s] and port [%d], code %d: %s\n", mIP, rcp->nPort, RetVal, gai_strerror(RetVal));
#ifdef TRACE_MODE
              if (nSenderLog) {
                sprintf(str, "Resolve address [%s] and port [%d], code %d: %s\n", mIP, rcp->nPort, RetVal, gai_strerror(RetVal));
                printfTrace(str);
			  }
#endif
			}
		  }
		}
	  }
#endif
	  //bKeepAlive = TRUE;
	  //if (setsockopt( msock, SOL_SOCKET, SO_KEEPALIVE, (char *)&bKeepAlive, sizeof(bKeepAlive)) != SOCKET_ERROR)
		//printf("Set KeepAlive success\n");
	  nl = sizeof(int);
	  nSndsize = nSendBuf;
	  if (nSndsize > 0) {
        if (setsockopt( msock, SOL_SOCKET, SO_SNDBUF, (char *)&nSndsize, nl) != SOCKET_ERROR) {
	      if (getsockopt( msock, SOL_SOCKET, SO_SNDBUF, (char *)&nSndsize, &nl) != SOCKET_ERROR) {
            if (bDebug)
              printf("Set Send Buffer size = %d\n", nSndsize);
#ifdef TRACE_MODE
if (nSenderLog) {
  sprintf(str, "[%s] Set Send Buffer size = %d\n", pNo, nSndsize);
  printfTrace(str);
}
#endif
		  }
		}
	  }

      if (nRecvTMO) {
        timeout = 1000 * (int)nRecvTMO; // 1秒単位
        if (setsockopt( msock, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) != SOCKET_ERROR)
          if (bDebug)
		    printf("Set SO_RCVTIMEO success\n");
	  }
      if (nSendTMO) {
        timeout = 1000 * (int)nSendTMO; // 1秒単位
        if (setsockopt( msock, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout)) != SOCKET_ERROR)
          if (bDebug)
		    printf("Set SO_SNDTIMEO success\n");
	  } 
	  rcp->nConnectRetry = 0;
	  AI2 = AI;
round_robin:
      if (connect(msock, AI->ai_addr, AI->ai_addrlen) == SOCKET_ERROR) {// < 0) {
        nSockErr = WSAGetLastError();
		// ラウンドロビン対策
		if (AI->ai_next) { // 他にIPが定義されているならそのIPで接続リトライ
          AI = AI->ai_next;
		  goto round_robin;
		} else {
		  AI = AI2; // 先頭に戻す。
#ifdef UPDATE_20121105 // IPv4/v6併用で相手FQDNにIPv6が有効なのにIPv4のみでSMTPサーバが接続可能な場合接続できない不具合
          if (nAF == 2)
		  {
            closesocket( msock );
	        freeaddrinfo(AddrInfo);
            nAF = 0; // IPv6で失敗知るならIPv4で接続にトライする。
            goto retry_ipv6nextv4;
		  }
#endif
		}
#ifdef UPDATE_20070122A // 接続エラー（10055）で接続をリトライする処理を追加
		if (rcp->nConnectRetry < 30 && (nSockErr == 10055 || nSockErr == 10061))  // 接続が強制的に拒絶されました。
#else
		if (rcp->nConnectRetry < 30 && nSockErr == 10061)  // 接続が強制的に拒絶されました。
#endif
		{
		  _sleep(0);
	      rcp->nConnectRetry++;
          goto round_robin;
		}
#ifdef USE_SSL
		if (rcp->ssl) {
		  SSL_CTX_free(rcp->ctx);
          SSL_free(rcp->ssl);
#ifndef UPDATE_20060512 // SSLの初期化はサービス開始で１回のみ
          ERR_free_strings();
#endif
		  rcp->ssl = NULL;
		}
#endif
        closesocket( msock );
		msock = INVALID_SOCKET;
        //if (bDebug)
          printf("winsock connect failed. %s (socket error code=%d)\n", pNo, nSockErr);
#ifdef TRACE_MODE
if (nSenderLog || nSender2Log) {
  sprintf(str, "[%s] ConnectHost: winsock connect failed. (socket error code=%d)\n", pNo, nSockErr);
  printfTrace(str);
}
#endif
	  } else {
        if (bDebug)
          printf("winsock connect success. %s\n", pNo);
#ifdef TRACE_MODE
if (nSenderLog) {
  sprintf(str, "[%s] ConnectHost: winsock connect success.\n", pNo);
  printfTrace(str);
}
#endif
#ifdef USE_SSL
  	    if (rcp->ssl) {  // SSLを使う場合は証明書と個人鍵を設定する。
          SSL_set_fd(rcp->ssl, msock); 
//	      if (rcp->ssl->rbio)
//	        rcp->ssl->rbio->bOnMemory = 0; // デフォルト FALSE
          SSL_connect(rcp->ssl);
		}
#endif
	  }
	} else {
      closesocket( msock );
	  msock = INVALID_SOCKET;
#ifdef TRACE_MODE
if (nSenderLog || nSender2Log) {
  sprintf(str, "[%s] ConnectHost: winsock socket()=INVALID_SOCKET.\n", pNo);
  printfTrace(str);
}
#endif
	}
	freeaddrinfo(AddrInfo);
#else
   msock = socket( AF_INET, SOCK_STREAM, 0);
   if (msock != INVALID_SOCKET) {
#ifdef UPDATE_20060529 // 設定ドメイン毎のＩＰに送信元ＩＰを切替える設定
// DOMAIN_SMTPSENDIP NULL:しない あり:ドメイン別に設定されたIPから送信秒単位でローテーションあり
//
      if (bMTAIP) {             // MTAのIPを送信時設定するか否か
	    if ((pdom = strrchr(rcp->mFrom, '@'))) {
          GetMTAIP((LPCTSTR)(pdom+1), (LPTSTR)mIP);
#ifdef TRACE_MODE
          if (nSenderLog) {
            sprintf(str, "[%s] GetMTAIP [%s]\n", pNo, (pdom+1));
            printfTrace(str);
		  }
#endif
		  if (!mIP[0]) {
            GetProfileStringEx((LPCTSTR)DOMAIN_SMTPIP,(LPCTSTR)(pdom+1),(LPCTSTR)"",(LPTSTR)mIP, sizeof(mIP));
#ifdef UPDATE_20070521 // 予約語対策
            strcpy(mKey, pdom+1);
            strtok(mKey, "@");
            if (ReservedWords(mKey)) {
              strcpy(mKey, mReservedWords);
	          strcat(mKey, pdom+1);
	          GetProfileStringEx((LPCTSTR)DOMAIN_SMTPIP, (LPCTSTR)mKey, (LPCTSTR)"", (LPTSTR)mIP, (DWORD)sizeof(mIP)); // 対象ドメインの応答ＩＰアドレス取出し
			}
#endif
		    strtok(mIP, " ");
		  }
		  if (mIP[0]) {
	        sin.sin_family = AF_INET;
		    sin.sin_addr.s_addr = inet_addr(mIP);
            sin.sin_port = rcp->nPort;        /* Convert to network ordering */
#ifdef UPDATE_20070122 // 設定ドメイン毎のＩＰに送信元ＩＰを切替える設定の修正
            /////////////////////////////////////////////////////
		    //bindがエラーにならない対策
            setsockopt(msock, SOL_SOCKET, SO_REUSEADDR, (const char *)&on, sizeof(on));
            /////////////////////////////////////////////////////
#endif
            if (bind(msock,(LPSOCKADDR)&sin, sizeof(sin)) == SOCKET_ERROR) {
		      if (bDebug) printf("bind local adder fail. (%s)\n", mIP);
			}
		  }
		}
	  }
#endif
	 //bKeepAlive = TRUE;
     //if (setsockopt( msock, SOL_SOCKET, SO_KEEPALIVE, (char *)&bKeepAlive, sizeof(bKeepAlive)))
		//printf("Set KeepAlive success\n");
     if (nRecvTMO) {
       timeout = 1000 * (int)nRecvTMO; // 1秒単位
       setsockopt( msock, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout));
	 }
     if (nSendTMO) {
       timeout = 1000 * (int)nSendTMO; // 1秒単位
       setsockopt( msock, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout));
	 }
	 rcp->phe = NULL;
	 rcp->nConnectRetry = 0;
	 h_length = 0;
round_robin:
     if (FillAddr(&dest_sin, rcp)) {
       if (connect( msock, (PSOCKADDR) &dest_sin, sizeof( dest_sin)) == SOCKET_ERROR) {
        nSockErr = WSAGetLastError();
		// ラウンドロビン対策
		if (rcp->phe) {
		  if (rcp->phe->h_addr + rcp->phe->h_length < rcp->phe->h_name) { // 他にIPが定義されているならそのIPで接続リトライ
      	    rcp->phe->h_addr += rcp->phe->h_length;
		    h_length += rcp->phe->h_length;
		    goto round_robin;
		  } else {
      	    rcp->phe->h_addr -= h_length; // 先頭に戻す
		  }
		}
#ifdef UPDATE_20070122A // 接続エラー（10055）で接続をリトライする処理を追加
		if (rcp->nConnectRetry < 30 && (nSockErr == 10055 || nSockErr == 10061))  // 接続が強制的に拒絶されました。
#else
		if (rcp->nConnectRetry < 30 && nSockErr == 10061) { // 接続が強制的に拒絶されました。
#endif
		  _sleep(0);
	      rcp->nConnectRetry++;
          goto round_robin;
		}
#ifdef USE_SSL
		 if (rcp->ssl) {
		   SSL_CTX_free(rcp->ctx);
           SSL_free(rcp->ssl);
#ifndef UPDATE_20060512 // SSLの初期化はサービス開始で１回のみ
#ifndef UPDATE_OPESSL1_1_1 // openssl1.1.1系に置換えた（SSL2を廃止し、TSL1.3を使用可能にする為）
           ERR_free_strings();
#endif
#endif
   		   rcp->ssl = NULL;
		 }
#endif
         closesocket( msock );
		 msock = INVALID_SOCKET;
         //if (bDebug)
           printf("winsock connect failed. %s (socket error code=%d)\n", pNo, nSockErr);
#ifdef TRACE_MODE
if (nSenderLog || nSender2Log) {
  sprintf(str, "[%s] ConnectHost: winsock connect failed. (socket error code=%d)\n", pNo, nSockErr);
  printfTrace(str);
}
#endif
	   } else {
         if (bDebug)
           printf("winsock connect success. %s\n", pNo);
#ifdef TRACE_MODE
if (nSenderLog) {
  sprintf(str, "[%s] ConnectHost: winsock connect success.\n", pNo);
  printfTrace(str);
}
#endif
#ifdef USE_SSL
		if (rcp->ssl) {  // SSLを使う場合は証明書と個人鍵を設定する。
          SSL_set_fd(rcp->ssl, msock); 
//	      if (rcp->ssl->rbio)
//	        rcp->ssl->rbio->bOnMemory = 0; // デフォルト FALSE
          SSL_connect(rcp->ssl);
		}
#endif
	   }
	 } else {
       closesocket( msock );
	   msock = INVALID_SOCKET;
#ifdef TRACE_MODE
if (nSenderLog || nSender2Log) {
  sprintf(str, "[%s] ConnectHost: winsock FillAddr error.\n", pNo);
  printfTrace(str);
}
#endif
	 }
   } else {
     msock = INVALID_SOCKET;
     if (bDebug)
       printf("socket() failed Error\n");
#ifdef TRACE_MODE
if (nSenderLog || nSender2Log) {
  sprintf(str, "[%s] ConnectHost: winsock socket()=INVALID_SOCKET.\n", pNo);
  printfTrace(str);
}
#endif
   }
#endif

#ifdef TRACE_MODE
if (nSenderLog) {
  sprintf(str, "[%s] ConnectHost: retrun mscok (%x) / WSAGetLastError() = (%d);\n", pNo, msock, WSAGetLastError());
  printfTrace(str);
}
#endif
   return msock;
  
}

#ifdef USE_SSL
BOOL SetSendData(char *pNo, SOCKET msock, SSL *ssl, char *fn)
#else
BOOL SetSendData(char *pNo, SOCKET msock, char *fn)
#endif
{
#ifdef TRACE_MODE
   char str[LOG_BUFFER];
#endif
  FILE *src;
  int  sts = 0;
#ifdef UPDATE_20200609 // メールヘッダの1行がが252byteを以上で不正な改行が不可される
  char *fsts, *bN, Mes[1056];
#else
  char *fsts, *bN, Mes[256];
#endif
  BOOL bTerm = FALSE;
#ifdef V4
  int  nLen = 0;
  char mSend[256*64];

    mSend[0] = '\x0';
#endif

#ifdef V4
#ifdef UPDATE_20140809 //転送処理時のファイル読込みでバイナリデータが含まれていてもデータが切れてしまわない対策を追加。
	if ((src = fopen( fn, "rb" )))
#else
	if ((src = fopen( fn, "rt" )))
#endif
#else
#ifdef UPDATE_20140809 //転送処理時のファイル読込みでバイナリデータが含まれていてもデータが切れてしまわない対策を追加。
	if ((src = fopen( fn, "rb" )) != NULL )
#else
	if ((src = fopen( fn, "rt" )) != NULL )
#endif
#endif
	{
#ifdef UPDATE_20070608 // メッセージ配送バッファのクリア 最後の行に同じメッセージが出力されてしまう不具合
	 Mes[0] = '\x0'; // 
#endif
	 fsts = fgets(Mes,sizeof(Mes)-3,src);
     while(fsts || !feof(src)) {
	   if (fsts &&
		    (strcmp(Mes, ".\n") == 0 || strcmp(Mes, ".\r\n") == 0 ) )
		 bTerm = TRUE;
	   //_sleep(0);  // 他スレッドに切替
	   bN = strpbrk(Mes,"\r\n");
	   if (bN)
		 *bN = '\x0';
	   //strtok(Mes,"\r\n");
       if(Mes[0] == '\n' || Mes[0] == '\r') {
          Mes[0] = '\0';
		  bN = Mes;
	   }
	   if (bN)
 	     strcat(Mes,"\r\n");

#ifdef V4
#ifdef USE_SSL
#ifdef UPDATE_20060323 // 2048byte越えのSSL送信で終了しない
       if ((INT)(ssl ? 0x0800 : 0x4000) > (INT)(nLen +strlen(Mes))) // 16KByte以内{
#else
       if ((INT)0x4000 > (INT)(nLen +strlen(Mes))) // 16KByte以内{
#endif
	   {
		 nLen += strlen(Mes);
		 strcat(mSend, Mes);
	   } 
#else
       if ((INT)0x4000 > (INT)(nLen +strlen(Mes))) { // 16KByte以内{
		 nLen += strlen(Mes);
		 strcat(mSend, Mes);
	   } 
#endif
	   else {
#ifdef USE_SSL
         sts = sendEx(msock, ssl, mSend);
#else
         sts = send(msock, mSend, nLen, 0 );
#endif
	     if (sts == SOCKET_ERROR) {
#ifdef TRACE_MODE
           if (nSenderLog || nSender2Log) {
             sprintf(str, "[%s] SendData (socket error code=%d)\n", pNo, WSAGetLastError());
             printfTrace(str);
		   }
#endif
		 }
         ////////////////////////////
		 nLen = strlen(Mes);
		 strcpy(mSend, Mes);
	   }
       ////////////////////////////
#else  /// V4 
       ////////////////////////////
#ifdef USE_SSL
       sts = sendEx(msock, ssl, Mes);
#else
       sts = send(msock, Mes, strlen(Mes), 0 );
#endif

#ifdef TRACE_MODE
       if (nSenderLog == 2) { // Level 2
         sprintf(str, "[%s] SendMailMess: size=%4d Message > ", pNo, strlen(Mes));
         printfTrace(str);
         printfTrace(Mes);
	   }
#endif
	   if (sts == SOCKET_ERROR) {
#ifdef TRACE_MODE
if (nSenderLog || nSender2Log) {
  sprintf(str, "[%s] SendData (socket error code=%d)\n", pNo, WSAGetLastError());
  printfTrace(str);
}
#endif
//		 break;
	   }
       if (bDebug)
		 printf("%s", Mes);
#endif  /// V4
#ifdef UPDATE_20070608 // メッセージ配送バッファのクリア 最後の行に同じメッセージが出力されてしまう不具合
	   Mes[0] = '\x0'; // 
#endif
	   fsts = fgets(Mes,sizeof(Mes)-3,src);
     }
////////////////////// キャッシュにあるものを送出
#ifdef V4
	 if (nLen) { // 16KByte以内{
#ifdef USE_SSL
       sts = sendEx(msock, ssl, mSend);
#else
       sts = send(msock, mSend, nLen, 0 );
#endif
	 }
	 if (sts == SOCKET_ERROR) {
#ifdef TRACE_MODE
       if (nSenderLog || nSender2Log) {
         sprintf(str, "[%s] SendData (socket error code=%d)\n", pNo, WSAGetLastError());
         printfTrace(str);
	   }
#endif
	 }
#endif /// V4 
//////////////////////
	 if (fsts && sts != SOCKET_ERROR) {
	   if (strcmp(Mes, ".\n") == 0 || strcmp(Mes, ".\r\n") == 0 )
		 bTerm = TRUE;
	   bN = strpbrk(Mes,"\r\n");
	   if (bN)
		 *bN = '\x0';
	   //bN = strtok(Mes,"\r\n");
       if(Mes[0] == '\n' || Mes[0] == '\r') {
          Mes[0] = '\0';
		  bN = Mes;
	   }
	   if (bN)
 	     strcat(Mes,"\r\n");
#ifdef USE_SSL
       sts = sendEx(msock, ssl, Mes);
#else
       sts = send(msock, Mes, strlen(Mes), 0 );
#endif
#ifndef V4
#ifdef TRACE_MODE
       if (nSenderLog == 2) { // Level 2
         sprintf(str, "[%s] SendMailMess: size=%4d Message > ", pNo, strlen(Mes));
         printfTrace(str);
         printfTrace(Mes);
	   }
#endif
#endif
	   if (sts == SOCKET_ERROR) {
#ifdef TRACE_MODE
         if (nSenderLog || nSender2Log) {
           sprintf(str, "[%s] SendData (socket error code=%d)\n", pNo, WSAGetLastError());
           printfTrace(str);
		 }
#endif
	   }
	 }
     fclose(src);
   }
   if (!bTerm && sts != SOCKET_ERROR) {  // 終了ピリオドが見つからない時の処理。
   //if (!bTerm) {  // 終了ピリオドが見つからない時の処理。
     strcat(Mes,"\r\n.\r\n");
#ifdef USE_SSL
     sts = sendEx(msock, ssl, Mes);
#else
     sts = send(msock, Mes, strlen(Mes), 0 );
#endif
#ifndef V4
#ifdef TRACE_MODE
     if (nSenderLog == 2) { // Level 2
       sprintf(str, "[%s] SendMailMess: size=%4d Message > ", pNo, strlen(Mes));
       printfTrace(str);
       printfTrace(Mes);
	 }
#endif
#endif
	 if (sts == SOCKET_ERROR) {
#ifdef TRACE_MODE
       if (nSenderLog || nSender2Log) {
         sprintf(str, "[%s] SendData (socket error code=%d)\n", pNo, WSAGetLastError());
         printfTrace(str);
	   }
#endif
	 }
   }

   Mes[0] = '\x0';
#ifdef USE_SSL
   //sts = sendEx(msock, ssl, Mes);
#else
   sts = send(msock, Mes, 0, 0 );
#endif
   if (sts != SOCKET_ERROR) {
#ifdef TRACE_MODE
     if (nSenderLog || nSender2Log) {
       sprintf(str, "[%s] SendData complete.\n", pNo);
       printfTrace(str);
	 }
#endif
   }
   return (sts != SOCKET_ERROR ? TRUE : FALSE);
}

void AfterCutting(struct _RCP *rcp) { // 強制切断後の処理
  FILE *fpng;
  char mWKTo[256];
#ifdef TRACE_MODE
  char str[1024];
#endif

   rcp->nToNG++;
   if ((fpng = fopen(rcp->mToNG, "wt"))) {// 配送先ファイルが作成できないならエラー
     fprintf(fpng, "%s\n", rcp->mTo);
     fflush(fpng);
     fclose(fpng);
#ifdef TRACE_MODE
if (nSenderLog || nSender2Log) 
{
  sprintf(str, "[%s] AfterCutting(%d) write success. %s\n", rcp->mMNo, rcp->nToNG, rcp->mToNG);
  printfTrace(str);
}
#endif
     while(!(fpng = fopen(rcp->mToNG, "rt"))) {
       if (bServiceTerminating)
	 	 break;
	   _sleep(WAIT_TIME);
	 }
     fclose(fpng);
   } else {
#ifdef TRACE_MODE
if (nSenderLog || nSender2Log)
{
  sprintf(str, "[%s] AfterCutting(%d) write fail. %s\n", rcp->mMNo, rcp->nToNG, rcp->mToNG);
  printfTrace(str);
}
#endif
   }
   if (rcp->fp) { // ファイルポインタがNULLでないなら
	 strcpy(mWKTo, rcp->mTo);
	 /* 2002.03.07
	 strncpy(mWKTo, rcp->mTo, sizeof(mWKTo));
     mWKTo[sizeof(mWKTo)-1] = '\x0';
	 */
     rcp->fsts = fgets(rcp->mTo, sizeof(rcp->mTo), rcp->fp);
     //if (rcp->fsts && !feof(rcp->fp) ) {
#ifdef UPDATE_20050428
	 if ((rcp->fsts || !feof(rcp->fp)) && strchr(rcp->mTo,'@'))
#else
     if (rcp->fsts && !feof(rcp->fp) && strchr(rcp->mTo,'@'))
#endif
	 {
#ifdef UPDATE_20050428
	   if (strnicmp(rcp->mTo, "recipient:", 10))
		 strtok(rcp->mTo, "\r\n");
	   else
#endif
         strword(rcp->mTo, " ", "\r\n");
       strword(rcp->mTo, "<", ">");
       strcpy(rcp->mDomain, rcp->mTo);
       strword(rcp->mDomain, "@\r\n", ">\r\n");
#ifdef UPDATE_20050428
	   if (strnicmp(rcp->mTo, "recipient:", 10))
		 strtok(rcp->mTo, "\r\n");
	   else
#endif
         strword(rcp->mTo, " ", "\r\n");
       strword(rcp->mTo, "<", ">");
	 } else {
	   strcpy(rcp->mTo, mWKTo);
	   /* 2002.03.07
	   strncpy(rcp->mTo, mWKTo, sizeof(rcp->mTo));
	   rcp->mTo[sizeof(rcp->mTo)-1] = '\x0';
	   */
	 }
   }
}

#ifdef USE_SSL
int sendEx(SOCKET msock, SSL *ssl, char *mMess) {
  if (ssl) 
    return (SSL_write(ssl,  mMess, strlen(mMess)));
  else
    return (send(msock, mMess, strlen(mMess), 0 ));
}
#endif

BOOL SendMailMess(SOCKET msock, struct _RCP *rcp, BOOL *pbGreeting, BOOL *pbNego) {
#ifdef TRACE_MODE
#ifdef UPDATE_20241114_BUF // XOAUTH2のアクセストークン長いとログ用バッファオーバフローが発生する。
   char str[LOG_BUFFER*4];
#else
   char str[LOG_BUFFER];
#endif
#endif
   BOOL sts = FALSE, bTo = FALSE, bDATA = TRUE;
   char *pd;
#ifdef ADD_XOAUTH2 // OAUTH2での認証方式を追加
#ifdef UPDATE_20241114 // XOAUTH2のIDとアクセストークンのエンコードに余分なデータが出力される。
   char mCode[3][8192]; // outlookだとサイズが小さい
#else
   char mCode[3][4096];
#endif
#else
   char mCode[3][256];
#endif
#ifdef UPDATE_20241114 // XOAUTH2のIDとアクセストークンのエンコードに余分なデータが出力される。
   char Mes[8192];
#else
   char Mes[256];
#endif
   char mWKTo[256], mNGAddr[2048], *pNGAddrTop, *pNGAddrAT; //, junk[256];
   char mNo[256], *pNo, *psts;
   FILE *fpok, *fpng;
#ifdef V4
#ifdef UPDATE_20050124
   SYSTEMTIME lt;
#endif
#endif
#ifdef UPDATE_20070621 // ユーザ毎のSMTP認証ID/PWを設定可能にする
#ifdef ADD_XOAUTH2 // OAUTH2での認証方式を追加
   char mID[256], mDir[256], mAUTHPW[3072];
#else
   char mID[256], mDir[256], mAUTHPW[256];
#endif
#endif
#ifdef UPDATE_20220414 // ゲートウェイ先ごとにSMTP認証ID,PWを規定する場合
#ifdef ADD_XOAUTH2 // OAUTH2での認証方式を追加
   char mGWON[2], mGWID[256],  mGWPW[3072];
#else
   char mGWON[2], mGWID[256],  mGWPW[256];
#endif
#endif
#ifdef UPDATE_20230425 // SMTPゲートウェイ欄にゲートウェイ先明記ならセッションを分離しないとき、内部ドメインも含まれてしまう不具合
   int  i;
   BOOL bInLocal;
#ifdef UPDATE_20230425A // SMTPゲートウェイ欄にゲートウェイ先明記ならセッションを分離しないとき、内部ドメインも含まれてしまう不具合
   FILE *fplc;
   char mToLC[512];
   char mToTkn[512];
#endif
#endif
#ifdef ADD_XOAUTH2_A
   FILE *fp, *pp;
   int  n;
   CHAR *p, *q, *s, *t;
   CHAR mUser[512];
#ifdef UPDATE_20241114_BUF // XOAUTH2のアクセストークン長いとログ用バッファオーバフローが発生する。
   CHAR mRes[8192], mReq[8192], mAns[8192];
   CHAR mPAuthFile[_MAX_PATH];
   CHAR mAccessToken[8192], mRefreshToken[8192];
#else
   CHAR mRes[4096], mReq[4096], mAns[4096];
   CHAR mPAuthFile[_MAX_PATH];
   CHAR mAccessToken[4096], mRefreshToken[4096];
#endif
#endif
#ifdef ADD_XOAUTH2
#ifdef WINDOWS
	BOOL   bIn, bOut;
    HANDLE hStdInPipeRead = NULL;
    HANDLE hStdInPipeWrite = NULL;
    HANDLE hStdOutPipeRead = NULL;
    HANDLE hStdOutPipeWrite = NULL;
    STARTUPINFO si;
    PROCESS_INFORMATION pi ;
    SECURITY_ATTRIBUTES sa = { sizeof(SECURITY_ATTRIBUTES), NULL, TRUE };
    DWORD dwRead = 0;
    DWORD dwAvail = 0;
#endif
#endif
#ifdef ADD_XOAUTH2_C // GATEWAY.DATにOAUTH2のURLを指定可能に
	char *r, c;
#endif
   //int timeout;// = 60000 * 60; // 受信タイムアウトは60分;
   //int err;

#ifdef UPDATE_20230425A // SMTPゲートウェイ欄にゲートウェイ先明記ならセッションを分離しないとき、内部ドメインも含まれてしまう不具合
   strcpy(mToLC, rcp->mToNG);
   strcpy(&mToLC[strlen(mToLC)-2], "LC");
   bInLocal = FALSE;
#endif

#ifdef UPDATE_20220414 // ゲートウェイ先ごとにSMTP認証ID,PWを規定する場合
   mGWON[0] = '\x0';
   mGWID[0] = '\x0';
   mGWPW[0] = '\x0';
   if (pAuthON)
	 strcpy(mGWON, pAuthON);
   if (pAuthID)
	 strcpy(mGWID, pAuthID);
   if (pAuthPW)
     strcpy(mGWPW, pAuthPW);
#endif
  /////////////////////////////////////////
  strcpy(mNo, rcp->mMsg);
  ///////////////////////////// 2001.12.30
  //strtok(mNo, ".");
  pNo = strrchr(mNo, '.');
  if (pNo)
    *pNo = '\x0';
  /////////////////////////////////////////
  pNo = strrchr(mNo, '\\');
  if (pNo)
	pNo++;
  else
	pNo = mNo;
  /////////////////////////////////////////
#ifdef TRACE_MODE
if (nSenderLog) {
  sprintf(str, "[%s] SendMailMess\n", pNo);
  printfTrace(str);
}
#endif
/*
   if (nRecvTMO) {
     timeout = 1000 * (int)nRecvTMO; // 1秒単位
     err = setsockopt( msock, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout));
   }
   if (nSendTMO) {
     timeout = 1000 * (int)nSendTMO; // 1秒単位
     err = setsockopt( msock, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout));
   }
*/
   //*pbGreeting = *pbNego = TRUE;
   *pbGreeting = TRUE; *pbNego = FALSE;
   rcp->nToOK = rcp->nToNG = 0;
#ifdef UPDATE_20100118 // 過去の同報送信での送信エラー情報がクリアされずに新たなセッションでの送信エラーにその情報が反映されてしまう可能性があった。
//strcpy(rcp->mRCPAnswer,"550 Unknown user xxxxxx@docomo.ne.jp");
   rcp->mRCPAnswer[0] = '\x0';
#endif
#ifdef USE_SSL
   if (get_reply(msock, rcp->ssl, rcp->mAnswer) )
#else
   if (get_reply(msock, rcp->mAnswer) )
#endif
   {
#ifdef V4
#ifdef UPDATE_20050124
   GetSystemTime(&lt);
   SystemTimeToFileTime(&lt, &rcp->gret); // 接続開始時間
#endif
#endif
#ifdef USE_STARTTLS
RE_EHLO:
#endif
     *pbGreeting = TRUE;
#ifdef ESMTP_ON
	 if (bESMTP) {
       sprintf(Mes, "EHLO %s\r\n", rcp->mMID);
       if (bDebug)
		 printf("EHLO %s\n", rcp->mMID);
	 } else {
       sprintf(Mes, "HELO %s\r\n", rcp->mMID);
	   if (bDebug)
         printf("HELO %s\n", rcp->mMID);
	 }
#else
     sprintf(Mes, "HELO %s\r\n", rcp->mMID);
	 if (bDebug)
       printf("HELO %s\n", rcp->mMID);
#endif
#ifdef TRACE_MODE
if (nSenderLog || nSender2Log) {
  sprintf(str, "[%s] SendMailMess:", pNo);
  printfTrace(str);
  printfTrace(rcp->mAnswer);
  sprintf(str, "[%s] SendMailMess:%s", pNo, Mes);
  printfTrace(str);
}
#endif
#ifdef USE_SSL
     if (sendEx(msock, rcp->ssl, Mes) != SOCKET_ERROR)
#else
	 if (send(msock, Mes, strlen(Mes), 0 ) != SOCKET_ERROR)
#endif
	 {
#ifdef USE_SSL
       if (get_reply(msock, rcp->ssl, rcp->mAnswer) )
#else
       if (get_reply(msock, rcp->mAnswer) )
#endif
	   {
#ifdef USE_STARTTLS
		 strlwr(rcp->mAnswer);

#ifdef UPDATE_20231007 // STARTTLSで暗号化ネゴシェーションに失敗するドメインリストの作成と失敗先へのプレーン送信への切替対策
		 if (GetSTLSCheck(rcp->mDomain) && !rcp->bUsedSSL && bUsedSTLS && strstr(rcp->mAnswer, "starttls"))
#else
#ifdef UPDATE_20160113 // STARTTLS接続後の接続相手のグリーティングにSTARTTLSが含まれると正常に動作しない不具合
		 if (!rcp->bUsedSSL && bUsedSTLS && strstr(rcp->mAnswer, "starttls"))
#else
		 if (bUsedSTLS && strstr(rcp->mAnswer, "starttls"))
#endif
#endif
		 {
           sprintf(Mes, "STARTTLS\r\n");
 		   if (bDebug)
             printf("STARTTLS\n");
#ifdef UPDATE_20151126 // 送信の詳細ログにSTARTTLS実行時の記録が残るようにした。
#ifdef TRACE_MODE
if (nSenderLog || nSender2Log) {
  sprintf(str, "[%s] SendMailMess:", pNo);
  printfTrace(str);
  printfTrace(rcp->mAnswer);
  sprintf(str, "[%s] SendMailMess:%s", pNo, Mes);
  printfTrace(str);
}
#endif
#endif
#ifdef USE_SSL
           if (sendEx(msock, rcp->ssl, Mes) != SOCKET_ERROR)
#else
	       if (send(msock, Mes, strlen(Mes), 0 ) != SOCKET_ERROR)
#endif
		   {
#ifdef USE_SSL
		     if (!get_reply(msock, rcp->ssl, rcp->mAnswer) )
#else
			 if (!get_reply(msock, rcp->mAnswer) )
#endif
			 {
#ifdef UPDATE_20231007 // STARTTLSで暗号化ネゴシェーションに失敗するドメインリストの作成と失敗先へのプレーン送信への切替対策
	           SetSTLSCheck(rcp->mDomain);
#endif
#ifdef UPDATE_20090731 // STARTTLS有効でリクエストに対する応答が拒否の場合、平文で送信継続する
	           strcpy(Mes, "RSET\r\n");
               if (bDebug)
		         printf("RSET\n");
#ifdef UPDATE_20151126 // 送信の詳細ログにSTARTTLS実行時の記録が残るようにした。
#ifdef TRACE_MODE
if (nSenderLog || nSender2Log) {
  sprintf(str, "[%s] SendMailMess:", pNo);
  printfTrace(str);
  printfTrace(rcp->mAnswer);
  sprintf(str, "[%s] SendMailMess:%s", pNo, Mes);
  printfTrace(str);
}
#endif
#endif
#ifdef USE_SSL
               if (sendEx(msock, rcp->ssl, Mes) != SOCKET_ERROR)
#else
	           if (send(msock, Mes, strlen(Mes), 0 ) != SOCKET_ERROR)
#endif
			   {

#ifdef USE_SSL
		         if (!get_reply(msock, rcp->ssl, rcp->mAnswer) )
#else
			     if (!get_reply(msock, rcp->mAnswer) )
#endif
				 {
                   AfterCutting(rcp);
 	               bDATA = FALSE;
	               goto quit;
				 }
                 sprintf(Mes, "EHLO %s\r\n", rcp->mMID);
                 if (bDebug)
		           printf("EHLO %s\n", rcp->mMID);
#ifdef UPDATE_20151126 // 送信の詳細ログにSTARTTLS実行時の記録が残るようにした。
#ifdef TRACE_MODE
if (nSenderLog || nSender2Log) {
  sprintf(str, "[%s] SendMailMess:", pNo);
  printfTrace(str);
  printfTrace(rcp->mAnswer);
  sprintf(str, "[%s] SendMailMess:%s", pNo, Mes);
  printfTrace(str);
}
#endif
#endif
#ifdef USE_SSL
                 if (sendEx(msock, rcp->ssl, Mes) != SOCKET_ERROR)
#else
	             if (send(msock, Mes, strlen(Mes), 0 ) != SOCKET_ERROR)
#endif
				 {

#ifdef USE_SSL
		           if (!get_reply(msock, rcp->ssl, rcp->mAnswer) )
#else
			       if (!get_reply(msock, rcp->mAnswer) )
#endif
				   {
                     AfterCutting(rcp);
 	                 bDATA = FALSE;
	                 goto quit;
				   }
				   goto smtpauth;
				 } else { // 強制切断の場合
                   AfterCutting(rcp);
 	               bDATA = FALSE;
	               goto quit;
			     }
			   } else { // 強制切断の場合
                 AfterCutting(rcp);
 	             bDATA = FALSE;
	             goto quit;
			   }
#else
               AfterCutting(rcp);
 	           bDATA = FALSE;
	           goto quit;
#endif
			 }
#ifdef UPDATE_OPESSL1_1_1 // openssl1.1.1系に置換えた（SSL2を廃止し、TSL1.3を使用可能にする為）
             if ((rcp->ctx = SSL_CTX_new(TLS_client_method()))) // SSL3以降を使用
#else
             if ((rcp->ctx = SSL_CTX_new(SSLv23_client_method()))) // SSL2 or SSL3を使用
#endif
			 {
#ifdef TRACE_MODE
if (nSenderLog || nSender2Log) {
                sprintf(str, "SendMailMess():SSL_CTX_new() = %d\n", rcp->ctx);
                printfTrace(str);
}
#endif
#ifdef UPDATE_20240420 // 暗号化通信方式のセキュリティレベルの設定
          // if (bDebug) printf("SSL security level =%d\n", SSL_CTX_get_security_level(ctx));
          SSL_CTX_set_security_level(rcp->ctx, nSSLSecureLevel);
#ifdef TRACE_MODE
if (nSenderLog || nSender2Log) {
  sprintf(str, "SendMailMess():SSL_CTX_get_security_level() = %d\n", SSL_CTX_get_security_level(rcp->ctx));
  printfTrace(str);
}
#endif
#endif
#ifdef UPDATE_20171211A // 使用可能な暗号化通信方式のバージョンを規定するオプション 0x1:SSL2禁止 0x2:SSL3禁止 0x4:TSL1禁止 0x8:TSL1.1禁止 0x10:TSL1.2禁止
               SSL_CTX_set_options(rcp->ctx, nSecuerLayOption);
#endif
#ifdef UPDATE_20190426 // 利用する暗号アルゴリズムの組み合わせオプション 
			   if (mSecuerLayCipher[0])
			     SSL_CTX_set_cipher_list(rcp->ctx, mSecuerLayCipher);
		       if (bDebug)
			     Print_SSL_CIPHER_names(rcp->ctx);
#endif
		       if ((rcp->ssl = SSL_new(rcp->ctx))) // SSL構造作成
			   {
	             SSL_set_fd(rcp->ssl, msock);
                 if (SSL_connect(rcp->ssl) == 1)
				 {
                   rcp->bUsedSSL = TRUE;
				   goto RE_EHLO;
				 } else {
                   SSL_CTX_free(rcp->ctx);
#ifdef UPDATE_20160224 // STARTTLSで初期化失敗時にハングする
                   SSL_free(rcp->ssl);
   		           rcp->ssl = NULL;
#endif
                   if (bDebug)
                     printf("SSL_connect() failed SMTP=[%s] PORT=[%d]\n", rcp->mSmtp, rcp->nPort);
#ifdef TRACE_MODE
if (nSenderLog || nSender2Log) {
  sprintf(str, "SendMailMess():SSL_connect() failed SMTP=[%s] PORT=[%d]\n", rcp->mSmtp, rcp->nPort);
  printfTrace(str);
}
#endif
				 }
			   } else {
                 SSL_CTX_free(rcp->ctx);
                 if (bDebug)
                   printf("SSL_new() failed SMTP=[%s] PORT=[%d]\n", rcp->mSmtp, rcp->nPort);
#ifdef TRACE_MODE
if (nSenderLog || nSender2Log) {
  sprintf(str, "SendMailMess:SSL_new() failed SMTP=[%s] PORT=[%d]\n", rcp->mSmtp, rcp->nPort);
  printfTrace(str);
}
#endif
			   }
			 } else {
               if (bDebug)
                 printf("SSL_CTX_new() failed SMTP=[%s] PORT=[%d]\n", rcp->mSmtp, rcp->nPort);
#ifdef TRACE_MODE
if (nSenderLog || nSender2Log) {
  sprintf(str, "SendMailMess:SSL_CTX_new() failed SMTP=[%s] PORT=[%d]\n", rcp->mSmtp, rcp->nPort);
  printfTrace(str);
}
#endif
			 }
#ifdef UPDATE_20231007 // STARTTLSで暗号化ネゴシェーションに失敗するドメインリストの作成と失敗先へのプレーン送信への切替対策
	         SetSTLSCheck(rcp->mDomain);
#endif
#ifdef UPDATE_20241110 // XOAUTH2のトークン取得に失敗する不具合
             goto RE_EHLO;
#endif
		   } else { // 強制切断の場合
             AfterCutting(rcp);
 	         bDATA = FALSE;
	         goto quit;
		   }
		 } 
#endif
#ifdef UPDATE_20090731 // STARTTLS有効でリクエストに対する応答が拒否の場合、平文で送信継続する
smtpauth:
#endif
         *pbGreeting = FALSE; *pbNego = TRUE;
		  // 認証が必要なESMTPであるか否か ///////
#ifdef UPDATE_20070621 // ユーザ毎のSMTP認証ID/PWを設定可能にする
		  if (bESMTP) {
            mDir[0] = mAUTHPW[0] = '\x0';
			strcpy(mID, rcp->mFrom);
		    GetMailBoxFolder(mID, mDir);
			strcpy(mID, rcp->mFrom);
            GetPasswordFile(mDir, mID, mAUTHPW);
		  }
#endif

#ifdef UPDATE_20241217_RCP // gateway.datにSMTP認証の指定が別のセッションで上書きされない対策
		  if (bESMTP &&
			  mGWON[0] != '0' &&
			  mSMTPAUTHMODE[0] != '\x0' &&
			  (stricmp(mSMTPAUTHMODE,"no") != 0 || rcp->nGateAuthType))
#else
#ifdef UPDATE_20241217 // gateway.datにSMTP認証の指定がある時は優先するようにした。
		  if (bESMTP &&
			  mGWON[0] != '0' &&
			  mSMTPAUTHMODE[0] != '\x0' &&
			  (stricmp(mSMTPAUTHMODE,"no") != 0 || bGWAUTHTYPE[0] || bGWAUTHTYPE[1] || bGWAUTHTYPE[2] || bGWAUTHTYPE[3]))
#else
#ifdef UPDATE_20220414 // ゲートウェイ先ごとにSMTP認証ID,PWを規定する場合
		  if (bESMTP &&
			  mGWON[0] != '0' &&
			  mSMTPAUTHMODE[0] != '\x0' &&
			  stricmp(mSMTPAUTHMODE,"no") != 0)
#else
		  if (bESMTP && 
			  mSMTPAUTHMODE[0] != '\x0' &&
			  stricmp(mSMTPAUTHMODE,"no") != 0)
#endif
#endif
#endif
		  {
			  _strupr(rcp->mAnswer);
              /////////////// OAUTH2 認証 //////////////
#ifdef ADD_XOAUTH2 // OAUTH2での認証方式を追加
#ifdef UPDATE_20241217_RCP // gateway.datにSMTP認証の指定が別のセッションで上書きされない対策
			  if ((bAUTHTYPE[3] || (rcp->nGateAuthType & 0x8)) && strstr(rcp->mAnswer,"XOAUTH2")) {
#else
#ifdef UPDATE_20241217 // gateway.datにSMTP認証の指定がある時は優先するようにした。
			  if ((bAUTHTYPE[3] || bGWAUTHTYPE[3]) && strstr(rcp->mAnswer,"XOAUTH2")) {
#else
			  if (bAUTHTYPE[3] && strstr(rcp->mAnswer,"XOAUTH2")) {
#endif
#endif
#ifdef ADD_XOAUTH2_A
                 // 認証サーバへアクセストークンとリフレッシュトークンの取得処理
                 // 1.リフレッシュトークンがあるなら、リフレッシュしてアクセストークンとリフレッシュトークンを取得
                 // 1.リフレッシュトークンがないなら認証トークンからアクセストークンとリフレッシュトークンを取得
	             // 送信元エンベロープのユーザアカウントフォルダに"refresh_token"ファイルがあるか確認
#ifdef UPDATE_20241124_BOX // OAUTH2での認証方式用にアカウント毎にアクセストークンとリフレッシュトークンを格納するようにした
#ifdef UPDATE_20241127_BOX // OAUTH2でのアクセストークンとリフレッシュトークンの保存先を[メール作業フォルダ/oauth2/メールアドレス]に変更するようにした。
#ifdef UPDATE_20250601 // OAUTH2でのアクセストークンとリフレッシュトークンを特定のメールアドレスを参照先とする機能を追加
				 GetXOAUTH2CODEFile(mMailSpoolDir, (pAuthPW == NULL ? rcp->mFrom : pAuthPW), mAUTHPW);
#else
				 GetXOAUTH2CODEFile(mMailSpoolDir, rcp->mFrom, mAUTHPW);
#endif
#else
				 GetXOAUTH2CODEFile(mDir, mID, mAUTHPW);
#endif
#else
#ifdef ADD_XOAUTH2_B // アクセストークンの保存と再使用
				 sprintf(mDir, "%soauth2", mMailSpoolDir);
				 _mkdir(mDir);
#ifdef WINDOWS
				 strcat(mDir, "\\");
#else
				 strcat(mDir, "/");
#endif
				 strcat(mDir, rcp->mSmtp);
				 _mkdir(mDir);
#ifdef WINDOWS
				 strcat(mDir, "\\");
#else
				 strcat(mDir, "/");
#endif
#else
			     GetMailBoxFolder((mSMTPAUTHID[0] ? mSMTPAUTHID : mID), mDir);
#endif
#endif
				 strcpy(mID, rcp->mFrom);
                 mAccessToken[0]='\x0';
	             mRefreshToken[0] = '\x0';
#ifdef ADD_XOAUTH2_C // GATEWAY.DATにOAUTH2のURLを指定可能に
				 mReq[0] = '\x0';
                 if (pAuthID && *pAuthID)
				 {
	               strcpy(mReq, pAuthID);
                   if (p = strstr(mReq, "client_id="))
				   {
	                 if (r = strpbrk(p+10, " \"\r\n"))
					 {
                       c = *r;
					   *r = '\x0';
					   strcpy(mID, p+10);
					   *r = c;
					 }
				   }
				 }
#endif
#ifdef ADD_XOAUTH2_B // アクセストークンの保存と再使用
#ifdef UPDATE_20241127_BOX // OAUTH2でのアクセストークンとリフレッシュトークンの保存先を[メール作業フォルダ/oauth2/メールアドレス]に変更するようにした。
#ifdef UPDATE_20250601 // OAUTH2でのアクセストークンとリフレッシュトークンを特定のメールアドレスを参照先とする機能を追加
				 strcpy(mPAuthFile, GetXOAUTH2Path(mMailSpoolDir, (pAuthPW == NULL ? rcp->mFrom : pAuthPW), "access_token.dat"));
#else
			     strcpy(mPAuthFile, GetXOAUTH2Path(mMailSpoolDir, rcp->mFrom, "access_token.dat"));
#endif
#else
			     sprintf(mPAuthFile, "%saccess_token.dat", mDir);
#endif
				 if (bDebug) printf("Read %s\n", mPAuthFile);
                 if (fp = fopen(mPAuthFile, "rt"))  // Gateway.dat別トークン問合せ先
				 {
                   fgets(mAccessToken, sizeof(mAccessToken)-1, fp);
                   strtok(mAccessToken, "\r\n");
                   fclose(fp);
				 }
				 if (bDebug) printf("AccessToken=[%s]\n",  mAccessToken);
#ifdef UPDATE_20241110 // XOAUTH2のトークン取得に失敗する不具合
				 if (!fp || !mAccessToken[0])
#else
				 if (!fp)
#endif
				 {
#endif
#ifdef UPDATE_20241127_BOX // OAUTH2でのアクセストークンとリフレッシュトークンの保存先を[メール作業フォルダ/oauth2/メールアドレス]に変更するようにした。
#ifdef UPDATE_20250601 // OAUTH2でのアクセストークンとリフレッシュトークンを特定のメールアドレスを参照先とする機能を追加
				   strcpy(mPAuthFile, GetXOAUTH2Path(mMailSpoolDir, (pAuthPW == NULL ? rcp->mFrom : pAuthPW), "refresh_token.dat"));
#else
			       strcpy(mPAuthFile, GetXOAUTH2Path(mMailSpoolDir, rcp->mFrom, "refresh_token.dat"));
#endif
#else
			       sprintf(mPAuthFile, "%srefresh_token.dat", mDir);
#endif
				   if (bDebug) printf("Read %s\n", mPAuthFile);
                   if (fp = fopen(mPAuthFile, "rt"))  // Gateway.dat別トークン問合せ先
				   {
                     fgets(mRefreshToken, sizeof(mRefreshToken)-1, fp);
                     strtok(mRefreshToken, "\r\n");
                     fclose(fp);
				   }
				   if (bDebug) printf("RefreshToken=[%s]\n",mRefreshToken);
                   // ユーザアカウントフォルダから"oauth2.dat"を参照し認証サーバへ認証を行う
	               //sprintf(mOAuthFile,"%soauth2.dat", mProgPath); // OAUTH2での認証方式を追加(全体用)
#ifdef ADD_XOAUTH2_C // GATEWAY.DATにOAUTH2のURLを指定可能に
				  mReq[0] = '\x0';
                  if (pAuthID && *pAuthID)
				  {
	                strcpy(mReq, pAuthID);
                    if (mRefreshToken[0])
					{
                      if (t = strstr(mReq, "grant_type=")) // refresh_tokenがあればURLの"grant_type="以降をrefresh_tokenに置換えアクセストークンとリフレッシュ取得
					  {
                        *t='\x0';
                        sprintf(&mReq[strlen(mReq)], "grant_type=refresh_token&refresh_token=%s\"", mRefreshToken);
					  }
				    }
				    fp = NULL;
				  }
#endif
			       sprintf(mPAuthFile, "%soauth2.dat", mDir);
				   mAns[0]='\x0';
#ifdef ADD_XOAUTH2_C // GATEWAY.DATにOAUTH2のURLを指定可能に
				   //if (!mReq[0])
				   //{
					  //GetMailBoxFolder((mSMTPAUTHID[0] ? mSMTPAUTHID : mID), mDir);
					  //strcpy(mID, rcp->mFrom);
					  //sprintf(mPAuthFile, "%soauth2.dat", mDir);
#endif
                     // fp = fopen(mPAuthFile, "rt");  // 個人別トークン問合せ先
 			          // fp = fopen(mOAuthFile, "rt"); //無ければ全体用トークン問合せ先
				   if (bDebug) printf("mReq=[%s]\n",mReq);
#ifdef ADD_XOAUTH2_C // GATEWAY.DATにOAUTH2のURLを指定可能に
				   //}
				   //if (mReq[0] || fp)
				   if (mReq[0])
				   {
#endif
					   /*
			         if (fp)
					 {
			           while(fgets(mAns, sizeof(mAns)-1, fp))
					   {
				         strtok(mAns, "\r\n");
				         if (s = strpbrk(mAns, "\t= "))
						 {
				           *s = '\x0';
				           s++;
				           while(*s == ' ' || *s == '\t' ||*s == '=')
 				             s++;
				           if (strstr(strlwr(mAns), "smtpd"))
						   {
 					         sprintf(mReq, s);
					         strcpy(mAns, mReq);
				             if (s = strpbrk(mAns, "\t= "))
							 {
 					           *s = '\x0';
					           s++;
					           while(*s == ' ' || *s == '\t' ||*s == '='|| *s == '"')
 					             s++;
							 }
				  	         if (mAns[0] && strstr((mSMTPAUTHID[0] ? mSMTPAUTHID : mID), mAns))
							 {
                               if (mRefreshToken[0])
							   {
                                 if (t = strstr(s, "grant_type=")) // refresh_tokenがあればURLの"grant_type="以降をrefresh_tokenに置換えアクセストークンとリフレッシュ取得
                                   *t='\x0';
                                 sprintf(&mReq[strlen(mReq)], "grant_type=refresh_token&refresh_token=%s\"", mRefreshToken);
                                 if (p = strstr(mReq, "client_id="))
								 {
	                               if (r = strpbrk(p+10, " \"\r\n"))
								   {
                                     c = *r;
						             *r = '\x0';
						             strcpy(mID, p+10);
						             *r = c;
								   }
								 }
							   }
	                           break;
							 }
					         break;
						   }
						 }
					   }
			           fclose(fp);
#ifdef ADD_XOAUTH2_C // GATEWAY.DATにOAUTH2のURLを指定可能に
					 }
#endif
                     mAns[0]="";
					 */
#ifdef UPDATE_20241124 // OAUTH2での認証方式用にユーザ別認証コード保管ファイル(xoauth2_code.dat)を各アカウントフォルダに設定可能にした
#ifdef UPDATE_20241126 // XOAUTH2で失敗以外の応答コード200番台以外が返ってきた場合の対策
                    if (!strstr(mReq, "refresh_token"))
					{
#endif
					  if (!strnicmp(mAUTHPW, "code=", 5))
					  {
					    char *pcd;
					    if (pcd = strstr(mReq, "\"code=")) //"code="が設定済の場合
						{
					      strcpy(pcd+1, mAUTHPW); // アカウント毎による認可グラントコードの設定
					      strcat(pcd, "\"");
						} else {
					      strcat(mReq, " -d \""); ////"code="が未設定の場合
					      strcat(mReq, mAUTHPW); // アカウント毎による認可グラントコードの設定
					      strcat(mReq, "\"");
						}
					  }
#ifdef UPDATE_20241126 // XOAUTH2で失敗以外の応答コード200番台以外が返ってきた場合の対策
					}
#endif
#endif
                     sprintf(Mes, "%s\n", mReq);
 				     if (bDebug)
                       printf("%s\n", mReq);
#ifdef TRACE_MODE
if (nSenderLog || nSender2Log) {
  sprintf(str, "[%s] SendMailMess:", pNo);
  printfTrace(str);
  printfTrace(rcp->mAnswer);
  sprintf(str, "[%s] SendMailMess:%s", pNo, Mes);
  printfTrace(str);
}
#endif
#ifdef WINDOWS
                     bIn = CreatePipe(&hStdInPipeRead, &hStdInPipeWrite, &sa, 0);
                     bOut = CreatePipe(&hStdOutPipeRead, &hStdOutPipeWrite, &sa, 0);
                     // Create the process.
					 ZeroMemory( &si, sizeof(si) );
                     si.cb = sizeof(STARTUPINFO);
					 ZeroMemory( &pi, sizeof(pi) );
                     si.dwFlags = STARTF_USESTDHANDLES;
                     //si.hStdError = hStdOutPipeWrite;
                     si.hStdOutput = hStdOutPipeWrite;
                     si.hStdInput = hStdInPipeRead;
                     //CreateProcess(NULL, mReq, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi);
//printf("mReq size=%d\n", strlen(mReq));
#ifdef TRACE_MODE
if (nSenderLog || nSender2Log) {
  sprintf(str, "[%s] SendMailMess:", pNo);
  printfTrace(str);
  printfTrace(mReq);
}
#endif
                     if (!CreateProcess(NULL, mReq, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi))
					 {
                        //if (bDebug)
						  //printf("プロセスを開始できません. (%d)\n", WSAGetLastError());
#ifdef TRACE_MODE
if (nSenderLog || nSender2Log) {
  sprintf(str, "プロセスを開始できません. (%d)\n", WSAGetLastError());
  printfTrace(str);
}
#endif
					 } 
                     // Close pipes we do not need.
				     if (bOut)
                       CloseHandle(hStdOutPipeWrite);
				     if (bIn)
                       CloseHandle(hStdInPipeRead);

                     if (ReadFile(hStdOutPipeRead, mAns, sizeof(mAns)-1, &dwRead, NULL))
					 {
                       n = dwRead;
					   mAns[n]='\x0';
				       while(ReadFile(hStdOutPipeRead, &mAns[n], sizeof(mAns)-strlen(mAns)-1, &dwRead, NULL))
					   {
						 if (dwRead == 0)
						   break;
                         n += dwRead;
					   }
				       mAns[n] = '\0';
#else
                     if (pp = _popen(mReq, "r")) // 認証サーバに確認
					 {
                       n = 0;
					   mAns[n]='\x0';
				       while(fgets(&mAns[n], sizeof(mAns)-strlen(mAns)-1, pp))
					   {
                         n = strlen(mAns);
					   }
				       _pclose(pp);
#endif
                       sprintf(Mes, "%s\n", mAns);;
 				       if (bDebug)
                         printf("%s\n", mAns);
#ifdef TRACE_MODE
if (nSenderLog || nSender2Log) {
  sprintf(str, "[%s] SendMailMess:", pNo);
  printfTrace(str);
  printfTrace(rcp->mAnswer);
  sprintf(str, "[%s] SendMailMess:%s", pNo, Mes);
  printfTrace(str);
}
#endif
                     //_strlwr(mAns);
                       if (p=strstr(mAns, "access_token\":")) // アクセストークン
					   {
                         if (q = strchr(p+14, '"'))
						 {
                           *q='\x0';
#ifdef UPDATE_20241110 // XOAUTH2のトークン取得に失敗する不具合
                           strncpy(mAccessToken, q+1, sizeof(mAccessToken)-1);
						   mAccessToken[sizeof(mAccessToken)-1]='\x0';
						   strtok(mAccessToken, "\"\r\n");
#else
                           strcpy(mAccessToken, p+15);
#endif
                           *q='"';
#ifdef ADD_XOAUTH2_B // アクセストークンの保存と再使用
#ifdef UPDATE_20241127_BOX // OAUTH2でのアクセストークンとリフレッシュトークンの保存先を[メール作業フォルダ/oauth2/メールアドレス]に変更するようにした。
#ifdef UPDATE_20250601 // OAUTH2でのアクセストークンとリフレッシュトークンを特定のメールアドレスを参照先とする機能を追加
						   strcpy(mPAuthFile, GetXOAUTH2Path(mMailSpoolDir, (pAuthPW == NULL ? rcp->mFrom : pAuthPW), "access_token.dat"));
#else
			               strcpy(mPAuthFile, GetXOAUTH2Path(mMailSpoolDir, rcp->mFrom, "access_token.dat"));
#endif
#else
					       sprintf(mPAuthFile, "%saccess_token.dat", mDir);
#endif
                           if (fp = fopen(mPAuthFile, "wt"))
						   { 
                             fputs(mAccessToken, fp);
                             fclose(fp);
						   }
#endif
						 }
					   } 
#ifdef ADD_XOAUTH2_B // アクセストークンの保存と再使用
					   else {
#ifdef UPDATE_20241127_BOX // OAUTH2でのアクセストークンとリフレッシュトークンの保存先を[メール作業フォルダ/oauth2/メールアドレス]に変更するようにした。
#ifdef UPDATE_20250601 // OAUTH2でのアクセストークンとリフレッシュトークンを特定のメールアドレスを参照先とする機能を追加
						 strcpy(mPAuthFile, GetXOAUTH2Path(mMailSpoolDir, (pAuthPW == NULL ? rcp->mFrom : pAuthPW), "access_token.dat"));
#else
			             strcpy(mPAuthFile, GetXOAUTH2Path(mMailSpoolDir, rcp->mFrom, "access_token.dat"));
#endif
#else
                         sprintf(mPAuthFile, "%saccess_token.dat", mDir);
#endif
                         unlink(mPAuthFile);
					   }
#endif
				       if (p=strstr(mAns, "refresh_token\":")) // リフレッシュトークン
					   {
                         if (q = strchr(p+15, '"'))
						 {
                           *q='\x0';
#ifdef UPDATE_20241110 // XOAUTH2のトークン取得に失敗する不具合
                           strncpy(mRefreshToken, q+1, sizeof(mRefreshToken)-1);
						   mRefreshToken[sizeof(mRefreshToken)-1]='\x0';
						   strtok(mRefreshToken, "\"\r\n");
#else
                           strcpy(mRefreshToken, p+16);
#endif
                           *q='"';
#ifdef UPDATE_20241127_BOX // OAUTH2でのアクセストークンとリフレッシュトークンの保存先を[メール作業フォルダ/oauth2/メールアドレス]に変更するようにした。
#ifdef UPDATE_20250601 // OAUTH2でのアクセストークンとリフレッシュトークンを特定のメールアドレスを参照先とする機能を追加
						   strcpy(mPAuthFile, GetXOAUTH2Path(mMailSpoolDir, (pAuthPW == NULL ? rcp->mFrom : pAuthPW), "refresh_token.dat"));
#else
			               strcpy(mPAuthFile, GetXOAUTH2Path(mMailSpoolDir, rcp->mFrom, "refresh_token.dat"));
#endif
#else
					       sprintf(mPAuthFile, "%srefresh_token.dat", mDir);
#endif
                           if (fp = fopen(mPAuthFile, "wt"))
						   { 
                             fputs(mRefreshToken, fp);
                             fclose(fp);
						   }
						 }
					   }
#ifdef ADD_XOAUTH2_B // アクセストークンの保存と再使用
#ifndef UPDATE_20241126_NODEL // XOAUTH2で失敗時のアクセストークン再取得時は、リフレッシュトークンファイルは消さずに残し再利用する。
					   else {
#ifdef UPDATE_20241127_BOX // OAUTH2でのアクセストークンとリフレッシュトークンの保存先を[メール作業フォルダ/oauth2/メールアドレス]に変更するようにした。
#ifdef UPDATE_20250601 // OAUTH2でのアクセストークンとリフレッシュトークンを特定のメールアドレスを参照先とする機能を追加
						 strcpy(mPAuthFile, GetXOAUTH2Path(mMailSpoolDir, (pAuthPW == NULL ? rcp->mFrom : pAuthPW), "refresh_token.dat"));
#else
			             strcpy(mPAuthFile, GetXOAUTH2Path(mMailSpoolDir, rcp->mFrom, "refresh_token.dat"));
#endif
#else
                         sprintf(mPAuthFile, "%srefresh_token.dat", mDir);
#endif
                         unlink(mPAuthFile);
					   }
#endif
#endif
					 }
#ifdef WINDOWS
			         if (bOut)
                       CloseHandle(hStdOutPipeRead);
				     if (bIn)
                       CloseHandle(hStdInPipeWrite);
#endif
					} 
				 }
#ifdef ADD_XOAUTH2_B // アクセストークンの保存と再使用
			   } 
#endif
#ifdef UPDATE_20241217_RCP // gateway.datにSMTP認証の指定が別のセッションで上書きされない対策
			   if ((bAUTHTYPE[3] || (rcp->nGateAuthType & 0x8)) && mAccessToken[0] && strstr(rcp->mAnswer,"XOAUTH2")) 
#else
#ifdef UPDATE_20241217 // gateway.datにSMTP認証の指定がある時は優先するようにした。
			   if ((bAUTHTYPE[3] || bGWAUTHTYPE[3]) && mAccessToken[0] && strstr(rcp->mAnswer,"XOAUTH2")) 
#else
			   if (bAUTHTYPE[3] && mAccessToken[0] && strstr(rcp->mAnswer,"XOAUTH2")) 
#endif
#endif
			   {
#endif
                sprintf(Mes, "AUTH XOAUTH2\r\n");
				if (bDebug)
                  printf("AUTH XOAUTH2\n");
#ifdef TRACE_MODE
if (nSenderLog || nSender2Log) {
  sprintf(str, "[%s] SendMailMess:", pNo);
  printfTrace(str);
  printfTrace(rcp->mAnswer);
  sprintf(str, "[%s] SendMailMess:%s", pNo, Mes);
  printfTrace(str);
}
#endif
///////////////////////////////////////////////////////////////////////////////////
#ifdef USE_SSL
                if (sendEx(msock, rcp->ssl, Mes) != SOCKET_ERROR)
#else
	            if (send(msock, Mes, strlen(Mes), 0 ) != SOCKET_ERROR)
#endif
				{
#ifdef USE_SSL
				  if (!get_reply(msock, rcp->ssl, rcp->mAnswer) )
#else
				  if (!get_reply(msock, rcp->mAnswer) )
#endif
				  {
                    AfterCutting(rcp);
 	                bDATA = FALSE;
	                goto quit;
                  } else {
#ifdef UPDATE_20241104 // XOAUTH2でのBearer用のコード生成で余計な文字が含まれている不具合
#ifdef UPDATE_20241110 // XOAUTH2のトークン取得に失敗する不具合
#ifdef UPDATE_20250601 // OAUTH2でのアクセストークンとリフレッシュトークンを特定のメールアドレスを参照先とする機能を追加
					sprintf(mCode[0], "user=%s\1auth=Bearer %s\1\1", (pAuthPW == NULL ? rcp->mFrom : pAuthPW), mAccessToken);
#else
					sprintf(mCode[0], "user=%s\1auth=Bearer %s\1\1", rcp->mFrom, mAccessToken);
#endif
#else
                    sprintf(mCode[0], "user=%s\1auth=Bearer %s\1\1", mID, mAccessToken);
#endif
#else
                    sprintf(mCode[0], "user=%s\1Aauth=Bearer %s\1\1", mID, mAccessToken);
#endif
#ifdef UPDATE_20241114 // XOAUTH2のIDとアクセストークンのエンコードに余分なデータが出力される。
		            translateUue2Base64(mCode[0], strlen(mCode[0]), mCode[1]);
#else
		            translateUue2Base64(mCode[0], strlen(mID)+strlen(mAccessToken)+21, mCode[1]);
#endif
                    sprintf(Mes, "%s\r\n", mCode[1]);
					if (bDebug)
                      printf("%s\n", mCode[1]);
#ifdef TRACE_MODE
if (nSenderLog || nSender2Log) {
  sprintf(str, "[%s] SendMailMess:", pNo);
  printfTrace(str);
  printfTrace(rcp->mAnswer);
  sprintf(str, "[%s] SendMailMess:%s", pNo, Mes);
  printfTrace(str);
}
#endif
#ifdef USE_SSL
                    if (sendEx(msock, rcp->ssl, Mes) != SOCKET_ERROR)
#else
	                if (send(msock, Mes, strlen(Mes), 0 ) != SOCKET_ERROR)
#endif
					{
#ifdef USE_SSL
					  if (!get_reply(msock, rcp->ssl, rcp->mAnswer) )
#else
					  if (!get_reply(msock, rcp->mAnswer) )
#endif
					  {
#ifdef ADD_XOAUTH2_B // アクセストークンの保存と再使用
#ifdef UPDATE_20241127_BOX // OAUTH2でのアクセストークンとリフレッシュトークンの保存先を[メール作業フォルダ/oauth2/メールアドレス]に変更するようにした。
#ifdef UPDATE_20250601 // OAUTH2でのアクセストークンとリフレッシュトークンを特定のメールアドレスを参照先とする機能を追加
						strcpy(mPAuthFile, GetXOAUTH2Path(mMailSpoolDir, (pAuthPW == NULL ? rcp->mFrom : pAuthPW), "access_token.dat"));
#else
			            strcpy(mPAuthFile, GetXOAUTH2Path(mMailSpoolDir, rcp->mFrom, "access_token.dat"));
#endif
#else
                        sprintf(mPAuthFile, "%saccess_token.dat", mDir);
#endif
                        unlink(mPAuthFile);
#endif
                        AfterCutting(rcp);
 	                    bDATA = FALSE;
				        goto quit;
					  } 
#ifdef UPDATE_20241126 // XOAUTH2で失敗以外の応答コード200番台以外が返ってきた場合の対策
					  else if (rcp->mAnswer[0] != '2')
					  {
#ifdef ADD_XOAUTH2_B // アクセストークンの保存と再使用
#ifdef UPDATE_20241127_BOX // OAUTH2でのアクセストークンとリフレッシュトークンの保存先を[メール作業フォルダ/oauth2/メールアドレス]に変更するようにした。
#ifdef UPDATE_20250601 // OAUTH2でのアクセストークンとリフレッシュトークンを特定のメールアドレスを参照先とする機能を追加
						strcpy(mPAuthFile, GetXOAUTH2Path(mMailSpoolDir, (pAuthPW == NULL ? rcp->mFrom : pAuthPW), "access_token.dat"));
#else
			            strcpy(mPAuthFile, GetXOAUTH2Path(mMailSpoolDir, rcp->mFrom, "access_token.dat"));
#endif
#else
                        sprintf(mPAuthFile, "%saccess_token.dat", mDir);
#endif
                        unlink(mPAuthFile);
#endif
                        AfterCutting(rcp);
 	                    bDATA = FALSE;
#ifdef UPDATE_20241127 // XOAUTH2で失敗以外の応答コード300番台以外が返ってきた場合の対策
						if (rcp->mAnswer[0] == '3')
							rcp->mAnswer[0] = '5';
#endif
				        goto quit;
					  }
#endif
					}
				  }
				}
			  } else // XOAUTH2失敗したら、他の認証で試す。
#endif
              /////////////// CRAM-MD5 認証 //////////////
#ifdef UPDATE_20241217_RCP // gateway.datにSMTP認証の指定が別のセッションで上書きされない対策
		      if ((bAUTHTYPE[0] || (rcp->nGateAuthType & 0x4)) && strstr(rcp->mAnswer,"CRAM-MD5")) {
#else
#ifdef UPDATE_20241217 // gateway.datにSMTP認証の指定がある時は優先するようにした。
			  if ((bAUTHTYPE[0] || bGWAUTHTYPE[0]) && strstr(rcp->mAnswer,"CRAM-MD5")) {
#else
              if (bAUTHTYPE[0] && strstr(rcp->mAnswer,"CRAM-MD5")) {
#endif
#endif
                sprintf(Mes, "AUTH CRAM-MD5\r\n");
				if (bDebug)
                  printf("AUTH CRAM-MD5\n");
#ifdef TRACE_MODE
if (nSenderLog || nSender2Log) {
  sprintf(str, "[%s] SendMailMess:", pNo);
  printfTrace(str);
  printfTrace(rcp->mAnswer);
  sprintf(str, "[%s] SendMailMess:%s", pNo, Mes);
  printfTrace(str);
}
#endif
#ifdef USE_SSL
                if (sendEx(msock, rcp->ssl, Mes) != SOCKET_ERROR)
#else
	            if (send(msock, Mes, strlen(Mes), 0 ) != SOCKET_ERROR)
#endif
				{
#ifdef USE_SSL
				  if (!get_reply(msock, rcp->ssl, rcp->mAnswer) )
#else
				  if (!get_reply(msock, rcp->mAnswer) )
#endif
				  {
                    AfterCutting(rcp);
 	                bDATA = FALSE;
	                goto quit;
                  } else {
                    Base64DecodeLine(&rcp->mAnswer[4], strlen(&rcp->mAnswer[4]), mCode[0], sizeof(mCode[0]));
#ifdef UPDATE_20220414 // ゲートウェイ先ごとにSMTP認証ID,PWを規定する場合
					hmac_md5(mCode[0],(pAuthPW ? mGWPW :(mSMTPAUTHPASS[0] ? mSMTPAUTHPASS : mAUTHPW)),mCode[1]);
					sprintf(mCode[2], "%s %s", (pAuthID ? mGWID : (mSMTPAUTHID[0] ? mSMTPAUTHID : mID)), mCode[1]);
#else
#ifdef UPDATE_20070621 // ユーザ毎のSMTP認証ID/PWを設定可能にする
					hmac_md5(mCode[0],(mSMTPAUTHPASS[0] ? mSMTPAUTHPASS : mAUTHPW),mCode[1]);
					sprintf(mCode[2], "%s %s", (mSMTPAUTHID[0] ? mSMTPAUTHID : mID), mCode[1]);
#else
					hmac_md5(mCode[0],mSMTPAUTHPASS,mCode[1]);
					sprintf(mCode[2], "%s %s", mSMTPAUTHID, mCode[1]);
#endif
#endif
					//[a2F3YSA2YWJiNzY2MDdkOTIzNTQ5MzZjMDRkYWJmN2ZmNDI3MA==
		            translateUue2Base64(mCode[2], strlen(mCode[2]), mCode[1]);
                    sprintf(Mes, "%s\r\n", mCode[1]);
					if (bDebug)
                      printf("%s\n", mCode[1]);
#ifdef TRACE_MODE
if (nSenderLog || nSender2Log) {
  sprintf(str, "[%s] SendMailMess:", pNo);
  printfTrace(str);
  printfTrace(rcp->mAnswer);
  sprintf(str, "[%s] SendMailMess:%s", pNo, Mes);
  printfTrace(str);
}
#endif
#ifdef USE_SSL
                    if (sendEx(msock, rcp->ssl, Mes) != SOCKET_ERROR)
#else
	                if (send(msock, Mes, strlen(Mes), 0 ) != SOCKET_ERROR)
#endif
					{
#ifdef USE_SSL
					  if (!get_reply(msock, rcp->ssl, rcp->mAnswer) )
#else
					  if (!get_reply(msock, rcp->mAnswer) )
#endif
					  {
                        AfterCutting(rcp);
 	                    bDATA = FALSE;
				        goto quit;
					  }
					}
				  }
				}
              /////////////// LOGIN 認証 //////////////
#ifdef UPDATE_20241217_RCP // gateway.datにSMTP認証の指定が別のセッションで上書きされない対策
			  } else if ((bAUTHTYPE[1] || (rcp->nGateAuthType & 0x1)) && strstr(rcp->mAnswer,"LOGIN")) {
#else
#ifdef UPDATE_20241217 // gateway.datにSMTP認証の指定がある時は優先するようにした。
			  } else if ((bAUTHTYPE[1] || bGWAUTHTYPE[1]) && strstr(rcp->mAnswer,"LOGIN")) {
#else
			  } else if (bAUTHTYPE[1] && strstr(rcp->mAnswer,"LOGIN")) {
#endif
#endif
                sprintf(Mes, "AUTH LOGIN\r\n");
				if (bDebug)
                  printf("AUTH LOGIN\n");
#ifdef USE_SSL
                if (sendEx(msock, rcp->ssl, Mes) != SOCKET_ERROR)
#else
	            if (send(msock, Mes, strlen(Mes), 0 ) != SOCKET_ERROR)
#endif
				{
#ifdef USE_SSL
				  if (!get_reply(msock, rcp->ssl, rcp->mAnswer) )
#else
				  if (!get_reply(msock, rcp->mAnswer) )
#endif
				  {
					AfterCutting(rcp);
 	                bDATA = FALSE;
	                goto quit;
				  } else { // 認証ID
#ifdef UPDATE_20220414 // ゲートウェイ先ごとにSMTP認証ID,PWを規定する場合
					translateUue2Base64((pAuthID ? mGWID :(mSMTPAUTHID[0] ? mSMTPAUTHID : mID)), strlen((pAuthID ? mGWID :(mSMTPAUTHID[0] ? mSMTPAUTHID : mID))), mCode[0]);
#else
#ifdef UPDATE_20070621 // ユーザ毎のSMTP認証ID/PWを設定可能にする
		            translateUue2Base64((mSMTPAUTHID[0] ? mSMTPAUTHID : mID), strlen((mSMTPAUTHID[0] ? mSMTPAUTHID : mID)), mCode[0]);
#else
		            translateUue2Base64(mSMTPAUTHID, strlen(mSMTPAUTHID), mCode[0]);
#endif
#endif
                    sprintf(Mes, "%s\r\n", mCode[0]);
					if (bDebug)
                      printf("%s\n", mCode[0]);
#ifdef TRACE_MODE
if (nSenderLog || nSender2Log) {
  sprintf(str, "[%s] SendMailMess:", pNo);
  printfTrace(str);
  printfTrace(rcp->mAnswer);
  sprintf(str, "[%s] SendMailMess:%s", pNo, Mes);
  printfTrace(str);
}
#endif
#ifdef USE_SSL
                    if (sendEx(msock, rcp->ssl, Mes) != SOCKET_ERROR)
#else
	                if (send(msock, Mes, strlen(Mes), 0 ) != SOCKET_ERROR)
#endif
					{
#ifdef USE_SSL
					  if (!get_reply(msock, rcp->ssl, rcp->mAnswer) )
#else
					  if (!get_reply(msock, rcp->mAnswer) )
#endif
					  {
                        AfterCutting(rcp);
 	                    bDATA = FALSE;
				        goto quit;
					  } else {
#ifdef UPDATE_20220414 // ゲートウェイ先ごとにSMTP認証ID,PWを規定する場合
						translateUue2Base64((pAuthPW ? mGWPW : (mSMTPAUTHPASS[0] ? mSMTPAUTHPASS : mAUTHPW)), strlen((pAuthPW ? mGWPW : (mSMTPAUTHPASS[0] ? mSMTPAUTHPASS : mAUTHPW))), mCode[0]);
#else
#ifdef UPDATE_20070621 // ユーザ毎のSMTP認証ID/PWを設定可能にする
		                translateUue2Base64((mSMTPAUTHPASS[0] ? mSMTPAUTHPASS : mAUTHPW), strlen((mSMTPAUTHPASS[0] ? mSMTPAUTHPASS : mAUTHPW)), mCode[0]);
#else
		                translateUue2Base64(mSMTPAUTHPASS, strlen(mSMTPAUTHPASS), mCode[0]);
#endif
#endif
                        sprintf(Mes, "%s\r\n", mCode[0]);
						if (bDebug)
                          printf("%s\n", mCode[0]);
#ifdef TRACE_MODE
if (nSenderLog || nSender2Log) {
  sprintf(str, "[%s] SendMailMess:", pNo);
  printfTrace(str);
  printfTrace(rcp->mAnswer);
  sprintf(str, "[%s] SendMailMess:%s", pNo, Mes);
  printfTrace(str);
}
#endif
#ifdef USE_SSL
                        if (sendEx(msock, rcp->ssl, Mes) != SOCKET_ERROR)
#else
	                    if (send(msock, Mes, strlen(Mes), 0 ) != SOCKET_ERROR)
#endif
						{
#ifdef USE_SSL
						  if (!get_reply(msock, rcp->ssl, rcp->mAnswer) )
#else
						  if (!get_reply(msock, rcp->mAnswer) )
#endif
						  {

                            AfterCutting(rcp);
 	                        bDATA = FALSE;
				            goto quit;
						  }
						}
					  }
					}
				  }
				}
              /////////////// PLAIN 認証 //////////////
#ifdef UPDATE_20241217_RCP // gateway.datにSMTP認証の指定が別のセッションで上書きされない対策
			  } else if ((bAUTHTYPE[2] || (rcp->nGateAuthType & 0x2)) && strstr(rcp->mAnswer,"PLAIN")) {
#else
#ifdef UPDATE_20241217 // gateway.datにSMTP認証の指定がある時は優先するようにした。
			  } else if ((bAUTHTYPE[2] || bGWAUTHTYPE[2]) && strstr(rcp->mAnswer,"PLAIN")) {
#else
			  } else if (bAUTHTYPE[2] && strstr(rcp->mAnswer,"PLAIN")) {
#endif
#endif
				mCode[0][0] = '\x0';
#ifdef UPDATE_20220414 // ゲートウェイ先ごとにSMTP認証ID,PWを規定する場合
				strcpy(&mCode[0][1], (pAuthID ? mGWID : (mSMTPAUTHID[0] ? mSMTPAUTHID : mID)));
				strcpy(&mCode[0][strlen((pAuthID ? mGWID : (mSMTPAUTHID[0] ? mSMTPAUTHID : mID)))+2], (pAuthPW ? mGWPW : (mSMTPAUTHPASS[0] ? mSMTPAUTHPASS : mAUTHPW)));
		        translateUue2Base64(mCode[0], strlen((pAuthID ? mGWID : (mSMTPAUTHID[0] ? mSMTPAUTHID : mID)))+strlen((pAuthPW ? mGWPW : (mSMTPAUTHPASS[0] ? mSMTPAUTHPASS : mAUTHPW)))+2, mCode[1]);
#else
#ifdef UPDATE_20070621 // ユーザ毎のSMTP認証ID/PWを設定可能にする
				strcpy(&mCode[0][1], (mSMTPAUTHID[0] ? mSMTPAUTHID : mID));
				strcpy(&mCode[0][strlen((mSMTPAUTHID[0] ? mSMTPAUTHID : mID))+2], (mSMTPAUTHPASS[0] ? mSMTPAUTHPASS : mAUTHPW));
		        translateUue2Base64(mCode[0], strlen((mSMTPAUTHID[0] ? mSMTPAUTHID : mID))+strlen((mSMTPAUTHPASS[0] ? mSMTPAUTHPASS : mAUTHPW))+2, mCode[1]);
#else
				strcpy(&mCode[0][1], mSMTPAUTHID);
				strcpy(&mCode[0][strlen(mSMTPAUTHID)+2], mSMTPAUTHPASS);
		        translateUue2Base64(mCode[0], strlen(mSMTPAUTHID)+strlen(mSMTPAUTHPASS)+2, mCode[1]);
#endif
#endif
                sprintf(Mes, "AUTH PLAIN %s\r\n", mCode[1]);
				if (bDebug)
                  printf("AUTH PLAIN %s\n", mCode[1]);
#ifdef TRACE_MODE
if (nSenderLog || nSender2Log) {
  sprintf(str, "[%s] SendMailMess:", pNo);
  printfTrace(str);
  printfTrace(rcp->mAnswer);
  sprintf(str, "[%s] SendMailMess:%s", pNo, Mes);
  printfTrace(str);
}
#endif
#ifdef USE_SSL
                if (sendEx(msock, rcp->ssl, Mes) != SOCKET_ERROR)
#else
	            if (send(msock, Mes, strlen(Mes), 0 ) != SOCKET_ERROR)
#endif
				{
#ifdef USE_SSL
				  if (!get_reply(msock, rcp->ssl, rcp->mAnswer) )
#else
				  if (!get_reply(msock, rcp->mAnswer) )
#endif
				  {

                    AfterCutting(rcp);
 	                bDATA = FALSE;
				    goto quit;
				  }
				}
			  }
		  }
		  ////////////////////////////////////////
mailfrom:
#ifdef SENDERID
		 _strlwr(rcp->mAnswer);
		 if (strstr(rcp->mAnswer, "250 submitter") ||
			 strstr(rcp->mAnswer, "250-submitter")) {
#ifdef UPDATE_20220725 // "MAIL From:","RCPT To:"のコマンド出力を"MAIL FROM:","RCPT TO:"に変更するようにした。
		   sprintf(Mes, "MAIL %s:<%s> SUBMITTER=%s\r\n", (nSelectcase==0 ? "From":(nSelectcase == 1? "from": "FROM")), rcp->mFrom, (rcp->mSubmitter[0] ? rcp->mSubmitter : rcp->mFrom));
		   if (bDebug)
             printf("MAIL %s:<%s> SUBMITTER=%s\n", (nSelectcase==0 ? "From":(nSelectcase == 1? "from": "FROM")), rcp->mFrom, (rcp->mSubmitter[0] ? rcp->mSubmitter : rcp->mFrom));
#else
		   sprintf(Mes, "MAIL From: <%s> SUBMITTER=%s\r\n", rcp->mFrom, (rcp->mSubmitter[0] ? rcp->mSubmitter : rcp->mFrom));
		   if (bDebug)
             printf("MAIL From: <%s> SUBMITTER=%s\n", rcp->mFrom, (rcp->mSubmitter[0] ? rcp->mSubmitter : rcp->mFrom));
#endif
		 } else {
#ifdef UPDATE_20220725 // "MAIL From:","RCPT To:"のコマンド出力を"MAIL FROM:","RCPT TO:"に変更するようにした。
           sprintf(Mes, "MAIL %s:<%s>\r\n", (nSelectcase==0 ? "From":(nSelectcase == 1? "from": "FROM")), rcp->mFrom);
		   if (bDebug)
             printf("MAIL %s:<%s>\n", (nSelectcase==0 ? "From":(nSelectcase == 1? "from": "FROM")), rcp->mFrom);
#else
           sprintf(Mes, "MAIL From: <%s>\r\n", rcp->mFrom);
		   if (bDebug)
             printf("MAIL From: <%s>\n", rcp->mFrom);
#endif
		 }
#else
#ifdef UPDATE_20220725 // "MAIL From:","RCPT To:"のコマンド出力を"MAIL FROM:","RCPT TO:"に変更するようにした。
         sprintf(Mes, "MAIL %s:<%s>\r\n", (nSelectcase==0 ? "From":(nSelectcase == 1? "from": "FROM")), rcp->mFrom);
		 if (bDebug)
           printf("MAIL %s:<%s>\n", (nSelectcase==0 ? "From":(nSelectcase == 1? "from": "FROM")), rcp->mFrom);
#else
         sprintf(Mes, "MAIL From: <%s>\r\n", rcp->mFrom);
		 if (bDebug)
           printf("MAIL From: <%s>\n", rcp->mFrom);
#endif
#endif
#ifdef TRACE_MODE
if (nSenderLog || nSender2Log) {
  sprintf(str, "[%s] SendMailMess:", pNo);
  printfTrace(str);
  printfTrace(rcp->mAnswer);
  sprintf(str, "[%s] SendMailMess:%s", pNo, Mes);
  printfTrace(str);
}
#endif
#ifdef USE_SSL
         if (sendEx(msock, rcp->ssl, Mes) != SOCKET_ERROR)
#else
 	     if (send(msock, Mes, strlen(Mes), 0 ) != SOCKET_ERROR)
#endif
		 {
#ifdef USE_SSL
           if (!get_reply(msock, rcp->ssl, rcp->mAnswer) )
#else
           if (!get_reply(msock, rcp->mAnswer) )
#endif
		   {
			 AfterCutting(rcp);
			 bDATA = FALSE;
             goto quit;
		   } else { 
			 if (!(fpok = fopen(rcp->mToOK, "wt"))) {// 配送先ファイルが作成できないならエラー
               sprintf(rcp->mAnswer, "551 error in the OK file system.\r\n"); 
		       return sts;
			 }
			 if (!(fpng = fopen(rcp->mToNG, "wt"))) {// 配送先ファイルが作成できないならエラー
               sprintf(rcp->mAnswer, "551 error in the NG file system.\r\n"); 
			   fclose(fpok);
		       return sts;
			 }
		     strcpy(rcp->mLogTo, "");
#ifdef UPDATE_20071208 // 同報メールで送信成功が１つでもあるとすぐにエラーアドレスがエラー生成してしまう
			 strcpy(rcp->mRCPAnswer, "");
#endif
NEXT_RCPTO:
#ifdef UPDATE_20220725 // "MAIL From:","RCPT To:"のコマンド出力を"MAIL FROM:","RCPT TO:"に変更するようにした。
             sprintf(Mes, "RCPT %s:<%s>\r\n", (nSelectcase==0 ? "To":(nSelectcase == 1? "to": "TO")), rcp->mTo);
			 if (bDebug)
               printf("RCPT %s:<%s>\n", (nSelectcase==0 ? "To":(nSelectcase == 1? "to": "TO")), rcp->mTo);
#else
             sprintf(Mes, "RCPT To: <%s>\r\n", rcp->mTo);
			 if (bDebug)
               printf("RCPT To: <%s>\n", rcp->mTo);
#endif
#ifdef TRACE_MODE
if (nSenderLog || nSender2Log) {
  sprintf(str, "[%s] SendMailMess:", pNo);
  printfTrace(str);
  printfTrace(rcp->mAnswer);
  sprintf(str, "[%s] SendMailMess:%s", pNo, Mes);
  printfTrace(str);
}
#endif
#ifdef USE_SSL
             if (sendEx(msock, rcp->ssl, Mes) != SOCKET_ERROR)
#else
		     if (send(msock, Mes, strlen(Mes), 0 ) != SOCKET_ERROR)
#endif
			 {
#ifdef USE_SSL
               bTo = get_reply(msock, rcp->ssl, rcp->mAnswer);
#else
			   bTo = get_reply(msock, rcp->mAnswer);
#endif
		       ///////////////////////////////////////////////////
			   if (bTo) {
   			     rcp->nToOK++;
		         fprintf(fpok, "%s\n", rcp->mTo);
			     if ((strlen(rcp->mLogTo)+strlen(rcp->mTo)+1) < sizeof(rcp->mLogTo) ) {
			       strcat(rcp->mLogTo, rcp->mTo);
			       strcat(rcp->mLogTo, " ");
				 }
               } else {
#ifdef UPDATE_20071208 // 同報メールで送信成功が１つでもあるとすぐにエラーアドレスがエラー生成してしまう
				 if (!rcp->mRCPAnswer[0]) {
			       strcpy(rcp->mRCPAnswer, rcp->mAnswer);
				 }
#endif
   			     rcp->nToNG++;
		         fprintf(fpng, "%s\n", rcp->mTo);
			   }
		       ///////////////////////////////////////////////////
			   ///// 同報処理
			   ///////////////////////////////////////////////////
#ifdef UPDATE_20230425 // SMTPゲートウェイ欄にゲートウェイ先明記ならセッションを分離しないとき、内部ドメインも含まれてしまう不具合
			   i = 1;
#ifndef UPDATE_20230425A // SMTPゲートウェイ欄にゲートウェイ先明記ならセッションを分離しないとき、内部ドメインも含まれてしまう不具合
			   bInLocal = FALSE;
#endif
NEXT_RCPTO2:
#endif
               if (rcp->fp) { // ファイルポインタがNULLでないなら
				 strcpy(mWKTo, rcp->mTo);
                 rcp->fsts = fgets(rcp->mTo, sizeof(rcp->mTo), rcp->fp);
#ifdef UPDATE_20050428
	             if (rcp->fsts || !feof(rcp->fp))
#else
	             if (rcp->fsts && !feof(rcp->fp))
#endif
				 {
#ifdef UPDATE_20050428
	               if (strnicmp(rcp->mTo, "recipient:", 10))
		             strtok(rcp->mTo, "\r\n");
	               else
#endif
                     strword(rcp->mTo, " ", "\r\n");
                   strword(rcp->mTo, "<", ">");
	               pd = strstr(rcp->mTo,"@");
	               if (pd) {
#ifdef UPDATE_20230310 // SMTPゲートウェイ欄にゲートウェイ先明記ならセッションを分離しない
#ifdef UPDATE_20230425 // SMTPゲートウェイ欄にゲートウェイ先明記ならセッションを分離しないとき、内部ドメインも含まれてしまう不具合
					 if (*pd =='@')
					   pd++;
	                 if (mSendGateway[0] && bIncludeForward || stricmp(rcp->mDomain, pd) == 0)
#else
	                 if (mSendGateway[0] && bIncludeForward || stricmp(rcp->mDomain, ++pd) == 0)
#endif
#else
	                 if (stricmp(rcp->mDomain, ++pd) == 0) 
#endif
					 {
#ifdef UPDATE_20230425 // SMTPゲートウェイ欄にゲートウェイ先明記ならセッションを分離しないとき、内部ドメインも含まれてしまう不具合
	                   if (!bMailForward &&  (CheckLocalServer(pd) ||
                           QueryMLists((LPCTSTR)SOFT_LISTS_REG, rcp->mTo)))
					   {
						 bInLocal = TRUE;
   			             rcp->nToNG++;
#ifdef UPDATE_20230425A // SMTPゲートウェイ欄にゲートウェイ先明記ならセッションを分離しないとき、内部ドメインも含まれてしまう不具合
						 if ((fplc=fopen(mToLC, "at")))
						 {
		                   fprintf(fplc, "%s\n", rcp->mTo);
						   fflush(fplc);
						   fclose(fplc);
						 }
#else
		                 fprintf(fpng, "%s\n", rcp->mTo);
#endif
						 goto NEXT_RCPTO2;
					   }
					   goto NEXT_RCPTO; 
#else
                       goto NEXT_RCPTO; /// 同一ドメインなら同報処理
#endif
					 } else {
                       strcpy(rcp->mDomain, rcp->mTo);
	                   //strword(rcp->mTo, " ", "\r\n");
		               //strword(rcp->mTo, "<", ">");
		               strword(rcp->mDomain, "@\r\n", ">\r\n");
					 }
				   }
				 } else {
				   strcpy(rcp->mTo, mWKTo);
				 }
			   }
			   fclose(fpok);
			   fclose(fpng);
			   while(!(fpok = fopen(rcp->mToOK, "rt"))) {
                 if (bServiceTerminating)
		           break;
				 _sleep(WAIT_TIME);
			   }
			   fclose(fpok);
			   while(!(fpng = fopen(rcp->mToNG, "rt"))) {
                 if (bServiceTerminating)
		           break;
				 _sleep(WAIT_TIME);
			   }
			   fclose(fpng);
			   if (rcp->nToOK==0) {
				 _unlink(rcp->mToOK); //DeleteFile(rcp->mToOK);
			   }
			   if (rcp->nToNG==0) {
				 _unlink(rcp->mToNG); //DeleteFile(rcp->mToNG);
			   }
			   ///////////////////////////////////////////////////
               if (!(bTo || rcp->nToOK)) {
			     bDATA = FALSE;
                 goto quit;
               } else {
			   //if (bTo || rcp->nToOK) {
                 *pbNego = FALSE;
                 sprintf(Mes, "DATA\r\n");
				 if (bDebug)
                   printf("DATA\n");
#ifdef TRACE_MODE
if (nSenderLog || nSender2Log) {
  sprintf(str, "[%s] SendMailMess:", pNo);
  printfTrace(str);
  printfTrace(rcp->mAnswer);
  sprintf(str, "[%s] SendMailMess:%s", pNo, Mes);
  printfTrace(str);
}
#endif
#ifdef USE_SSL
                 if (sendEx(msock, rcp->ssl, Mes) != SOCKET_ERROR)
#else
		         if (send(msock, Mes, strlen(Mes), 0 ) != SOCKET_ERROR)
#endif
				 {
#ifdef USE_SSL
                   if (get_reply(msock, rcp->ssl, rcp->mAnswer) )
#else
                   if (get_reply(msock, rcp->mAnswer) )
#endif
				   {
#ifdef TRACE_MODE
if (nSenderLog || nSender2Log) {
  sprintf(str, "[%s] SendMailMess:", pNo);
  printfTrace(str);
  printfTrace(rcp->mAnswer);
}
#endif
					 strcat(rcp->mAnswer, "> While delivering, the stopping happened.");
#ifdef USE_SSL
                     if (SetSendData(pNo, msock, rcp->ssl, rcp->mMsg))
#else
			         if (SetSendData(pNo, msock, rcp->mMsg))
#endif
					 {
#ifdef USE_SSL
                       if (get_reply(msock, rcp->ssl, rcp->mAnswer) )
#else
                       if (get_reply(msock, rcp->mAnswer) )
#endif
					   {
quit:
                         sprintf(Mes, "QUIT\r\n");
						 if (bDebug)
                           printf("QUIT\n");
#ifdef TRACE_MODE
if (nSenderLog || nSender2Log) {
  sprintf(str, "[%s] SendMailMess:", pNo);
  printfTrace(str);
  printfTrace(rcp->mAnswer);
  sprintf(str, "[%s] SendMailMess:%s", pNo, Mes);
  printfTrace(str);
}
#endif
#ifdef USE_SSL
                         if (sendEx(msock, rcp->ssl, Mes) != SOCKET_ERROR)
#else
			             if (send(msock, Mes, strlen(Mes), 0 ) != SOCKET_ERROR)
#endif
						 {
						   if (bDATA) {
#ifdef USE_SSL
                             get_reply(msock, rcp->ssl, rcp->mAnswer);
#else
                             get_reply(msock, rcp->mAnswer);
#endif
			 				 _unlink(rcp->mToOK); //DeleteFile(rcp->mToOK);
						     if (rcp->nToNG == 0)
                               sts = TRUE;
						   } else
#ifdef USE_SSL
                             get_reply(msock, rcp->ssl, NULL);
#else
                             get_reply(msock, NULL);
#endif
						 }
					   } else { // DATA送信でエラーのとき
senddata_err:
						 strcpy(mNGAddr, rcp->mAnswer);
#ifdef UPDATE_20080414 // 送信元アドレスが記載されている場合は無視する
						 if (strstr(mNGAddr, rcp->mFrom))
						 {
						   pNGAddrAT = NULL;
						 } else 
#endif
						 {
						   pNGAddrAT = strpbrk(mNGAddr, "@");
						 }
						 if (pNGAddrAT)
						 {
						   *pNGAddrAT = '\x0';
						   pNGAddrTop = strrchr(mNGAddr, ' ');
						   *pNGAddrAT = '@';
						   if (pNGAddrTop) {
							 pNGAddrTop++;
#ifdef UPDATE_20071129 //DATA送信後の受信拒否メッセージに送信先アドレスが含まれている場合正しくアドレス抽出できない場合がある。
							 while(*pNGAddrTop == ' ' ||
                                   *pNGAddrTop == '<' ||
								   *pNGAddrTop == '(') {
                                pNGAddrTop++;
							 }
                             strtok(pNGAddrTop, " )>\r\n");
#else
                             strtok(pNGAddrTop, " \r\n");
#endif
   			                 rcp->nToNG++;
	                         fpng = fopen(rcp->mToNG, "at");
			                 if (fpng) {
                               fprintf(fpng, "%s\n", pNGAddrTop);
							   fflush(fpng);
			                   fclose(fpng);
			                   while(!(fpng = fopen(rcp->mToNG, "rt"))) {
                                 if (bServiceTerminating)
		                           break;
				                 _sleep(WAIT_TIME);
							   }
			                   fclose(fpng);
							 }
							 rcp->nToOK = 0;
				             _unlink(rcp->mToOK); //DeleteFile(rcp->mToOK);
						   }
						 } else {
		                   rcp->nToNG = rcp->nToOK;
						   fpok = fopen(rcp->mToOK, "rt");
						   if (fpok) {
	                         fpng = fopen(rcp->mToNG, "at");
			                 if (fpng) {
                               psts = fgets(mNGAddr, sizeof(mNGAddr), fpok);
							   while(psts || !feof(fpok)) {
                                 fputs(mNGAddr, fpng);
                                 psts = fgets(mNGAddr, sizeof(mNGAddr), fpok);
							   }
							   fclose(fpok);
							   fflush(fpng);
			                   fclose(fpng);
						       rcp->nToOK = 0;
				               _unlink(rcp->mToOK); //DeleteFile(rcp->mToOK);
			                   while(!(fpng = fopen(rcp->mToNG, "rt"))) {
                                 if (bServiceTerminating)
		                           break;
				                 _sleep(WAIT_TIME);
							   }
			                   fclose(fpng);
							 }
						   }
						 }
						 bDATA = FALSE;
                         goto quit;
					   } 
					 } else {
					   goto senddata_err;
					 }
				 } else {
			        goto senddata_err;
				 }
				 } else {
			       goto senddata_err;
				 }
			   } 
			 } else { //RCPT TO Send err
			 ///////////////////////////////////
			   fflush(fpok);
			   fclose(fpok);
			   fflush(fpng);
			   fclose(fpng);
			   while(!(fpok = fopen(rcp->mToOK, "rt"))) {
                 if (bServiceTerminating)
		           break;
				 _sleep(WAIT_TIME);
			   }
			   fclose(fpok);
			   while(!(fpng = fopen(rcp->mToNG, "rt"))) {
                 if (bServiceTerminating)
		           break;
				 _sleep(WAIT_TIME);
			   }
			   fclose(fpng);
			   if (rcp->nToOK==0) {
				 _unlink(rcp->mToOK); //DeleteFile(rcp->mToOK);
			   }
			   if (rcp->nToNG==0) {
				 _unlink(rcp->mToNG); //DeleteFile(rcp->mToNG);
			   }
			 ///////////////////////////////////
			 }
           }
         }
#ifdef ESMTP_ON
       } else if (bESMTP) { // 拡張SMTPでない場合
	     sprintf(Mes, "HELO %s\r\n", rcp->mMID);
		 if (bDebug)
           printf("HELO %s\n", rcp->mMID);
#ifdef TRACE_MODE
if (nSenderLog || nSender2Log) {
  sprintf(str, "[%s] SendMailMess:", pNo);
  printfTrace(str);
  printfTrace(rcp->mAnswer);
  sprintf(str, "[%s] SendMailMess:%s", pNo, Mes);
  printfTrace(str);
}
#endif
#ifdef USE_SSL
         if (sendEx(msock, rcp->ssl, Mes) != SOCKET_ERROR)
#else
	     if (send(msock, Mes, strlen(Mes), 0 ) != SOCKET_ERROR)
#endif
		 {
#ifdef USE_SSL
           if (!get_reply(msock, rcp->ssl, rcp->mAnswer) )
#else
		   if (!get_reply(msock, rcp->mAnswer) ) // 強制切断の場合
#endif
		   {
			 AfterCutting(rcp);
			 bDATA = FALSE;
		     goto quit;
		   } else {
             *pbGreeting = FALSE; *pbNego = TRUE;
		     goto mailfrom;
		   }
#ifdef UPDATE_20250418 // 初回送信がESMTPで送信拒絶等によるソケットが相手側から切断されている状態になった場合で切断されていると認識出来ずに再送処理が行われ重複メールが発生する不具合
		 } else {
			 AfterCutting(rcp);
			 bDATA = FALSE;
		     goto quit;
#endif
		 }
#endif
	   } else { // 強制切断の場合
		 AfterCutting(rcp);
		 bDATA = FALSE;
	     goto quit;
	   }
     } else { // 強制切断の場合
       AfterCutting(rcp);
 	   bDATA = FALSE;
	   goto quit;
	 }
   } else {  // 強制切断の場合
     AfterCutting(rcp);
     bDATA = FALSE;
     goto quit;
   }
   if (!sts && rcp->nToOK == 0 && rcp->nToNG == 0) {
     if ((fpng = fopen(rcp->mToNG, "wt"))) {
       fprintf(fpng, "%s\n", rcp->mTo);
       fflush(fpng);
       fclose(fpng);
       while(!(fpng = fopen(rcp->mToNG, "rt"))) {
         if (bServiceTerminating)
           break;
	     _sleep(WAIT_TIME);
	   }
	   fclose(fpng);
	 }
   }
#ifdef UPDATE_20071208 // 同報メールで送信成功が１つでもあるとすぐにエラーアドレスがエラー生成してしまう
   if (rcp->mRCPAnswer[0]) {  // レシピエントにエラーがある場合
	 sts = atoi(rcp->mRCPAnswer);
	 strcpy(rcp->mAnswer, rcp->mRCPAnswer);
   }
#endif

#ifdef TRACE_MODE
if (nSenderLog) {
  sprintf(str, "[%s] SendMailMess:", pNo);
  printfTrace(str);
  printfTrace(rcp->mAnswer);
  sprintf(str, "[%s] SendMailMess:return=%d\n", pNo, sts);
  printfTrace(str);
}
#endif
#ifdef UPDATE_20230425 // SMTPゲートウェイ欄にゲートウェイ先明記ならセッションを分離しないとき、内部ドメインも含まれてしまう不具合
#ifdef UPDATE_20230425A // SMTPゲートウェイ欄にゲートウェイ先明記ならセッションを分離しないとき、内部ドメインも含まれてしまう不具合
   if (!bMailForward && bInLocal)
#else
   if (!bMailForward && rcp->nToNG > 0)
#endif
   {
#ifdef UPDATE_20230425A // SMTPゲートウェイ欄にゲートウェイ先明記ならセッションを分離しないとき、内部ドメインも含まれてしまう不具合
	 if ((fplc=fopen(mToLC, "at")))
	 {
       if (fpng = fopen(rcp->mToNG, "rt")) 
	   {
         while(fgets(mToTkn, sizeof(mToTkn)-1, fpng))
		 {
		   fputs(mToTkn, fplc);
		 }
         fclose(fpng);
	   }
	   fflush(fplc);
       fclose(fplc);
       if (fpng = fopen(rcp->mToNG, "wt"))
	   {
	     if (fplc=fopen(mToLC, "rt"))
		 {
           while(fgets(mToTkn, sizeof(mToTkn)-1, fplc))
		   {
		     fputs(mToTkn, fpng);
		   }
           fclose(fplc);
		 }
	     fflush(fpng);
         fclose(fpng);
	   }
       _unlink(mToLC); 
       while((fplc = fopen(mToLC, "rt"))) 
	   {
		 fclose(fplc);
         if (bServiceTerminating)
           break;
		 _unlink(mToLC); 
		 _sleep(WAIT_TIME);
	   }
	 }
#endif
	 SetRetry(rcp, TRUE, (DWORD) -1, i);
   }
#endif
#ifdef UPDATE_20230926 // 次のメールに暗号化通信フラグが一旦初期化されずに、SMTP Over SSLとして処理される場合がある不具合
   rcp->bUsedSSL = FALSE;
#endif
   return sts;
}

// int main(int argc, char *argv[]) {
int SendGlobalMail(struct _RCP *rcp) {
#ifdef TRACE_MODE
   char str[LOG_BUFFER];
#endif
   SOCKET  msock;
   int     sts = (int)FALSE;
   BOOL    bGreeting = TRUE, bNego = TRUE;
   //WSADATA WSAData;
   char mcode[4];
   LINGER lingerStruct;
   FILE   *fpng;
   char   mNo[256], mWKTo[256], *pNo;
   DWORD  dwBytesReturned = 0;
   BOOL   bNewBehavior = FALSE;
#ifdef V4
#ifdef UPDATE_20050124
   SYSTEMTIME lt;
   ULARGE_INTEGER *us, *ue;
#endif
#endif
#ifdef UPDATE_20060904C // 送信先回線エラーで対象ドメインの保存（MXキャッシュ削除用）
   char mSendDom[256];
#endif
   //DWORD  status;
   //DWORD  dwErr;
   // WINSOCK Start up
  /////////////////////////////////////////
  strcpy(mNo, rcp->mMsg);
  ///////////////////////////// 2001.12.30
  //strtok(mNo, ".");
  pNo = strrchr(mNo, '.');
  if (pNo)
    *pNo = '\x0';
  /////////////////////////////////////////
  pNo = strrchr(mNo, '\\');
  if (pNo)
	pNo++;
  else
	pNo = mNo;
  /////////////////////////////////////////
#ifdef TRACE_MODE
  if (nSenderLog) {
    sprintf(str, "[%s] SendGlbalMail:domain=%s\n", pNo, rcp->mTo);
    printfTrace(str);
  }
#endif
   rcp->bGreeting = rcp->bNego = TRUE;
   if (bDebug)
     printf("SendGlbalMail\n");
/*
#ifdef IPv6
   if (WSAStartup(MAKEWORD(2,2), &WSAData) == 0) {
#else
   if (WSAStartup(MAKEWORD(1,1), &WSAData) == 0) {
#endif
*/
#ifdef V4
#ifdef UPDATE_20050124
   GetSystemTime(&lt);
   SystemTimeToFileTime(&lt, &rcp->grst); // 接続開始時間
#endif
#endif
#ifdef TRACE_MODE
if (nSenderLog || nSender2Log) {
  sprintf(str, "[%s] ConnectHost(%s)\n", pNo, rcp->mSmtp);
  printfTrace(str);
}
#endif
   {
#ifdef UPDATE_20150108 // 送信許可時間の設定機能を追加
     if (!rcp->bUseTime)
	 {
       //strcpy(rcp->mAnswer, "451 Can not be sent in overtime.");
	   rcp->mAnswer[0] = '\x0';
	   goto NO_CONNECT;
	 }
#endif
	 if ( (msock = ConnectHost(rcp)) != INVALID_SOCKET)
	 {
/*
#ifdef IPv6
	   ////////////////////////////////////////////////////
	   // WIN2Kにて送信時にエラーコード10054が出ないようにする対策
       // disable  new behavior using
       // IOCTL: SIO_UDP_CONNRESET
       status = WSAIoctl(msock, SIO_UDP_CONNRESET,
                         &bNewBehavior, sizeof(bNewBehavior),
                         NULL, 0, &dwBytesReturned,
                         NULL, NULL);
if (SOCKET_ERROR == status) {
  dwErr = WSAGetLastError();
  if (WSAEWOULDBLOCK == dwErr) {
    // nothing to do
    //return(FALSE);
  } else {
    printf("WSAIoctl(SIO_UDP_CONNRESET) Error: %d\n", dwErr);
  }
}
	   /////////////////////////////////////////////////////
#endif
*/
       //sts = (int)SendMailMess(msock, rcp, &bGreeting, &bNego);   // SEND MAIL MESSAGES
	   /*
	   { // SOCKET Close
         lingerStruct.l_onoff = 1;
         lingerStruct.l_linger = 0;
         setsockopt(msock, SOL_SOCKET, SO_LINGER,
                    (char *)&lingerStruct, sizeof(lingerStruct) );
	   }
	   */
#ifdef V4
#ifdef UPDATE_20060904C // 送信先回線エラーで対象ドメインの保存（MXキャッシュ削除用）
       strcpy(mSendDom, rcp->mDomain);  // MXキャッシュ削除
#endif
#endif
	   sts = (int)SendMailMess(msock, rcp, &rcp->bGreeting, &rcp->bNego);   // SEND MAIL MESSAGES
#ifdef UPDATE_20181101 // 接続成功してもどこにも送信されない場合は、MXキャッシュをリセットするようにした。
	   if (!sts || sts && rcp->nToOK == 0) 
#else
	   if (!sts) 
#endif
	   {
#ifdef V4
#ifdef UPDATE_20060904C // 送信先回線エラーで対象ドメインの保存（MXキャッシュ削除用）
         DelMXCashList(mSendDom);  // MXキャッシュ削除
#else
         DelMXCashList(rcp->mDomain);  // MXキャッシュ削除
#endif
#endif
		 strncpy(mcode, rcp->mAnswer, 3);
		 mcode[3] = '\x0';
         sts = atoi(mcode);
#ifdef UPDATE_20060629 // 400番台のリトライが500番台に切替えられるてしまう。
         if (bOldRetryMode)
#endif
		 {
		   if (rcp->bGreeting && sts != 220) // 接続確立が成功コード以外なら接続できない対象と判定
             sts = FALSE;
		   //if (rcp->bNego && sts == 554)
		   if (rcp->bNego && sts == 554 || rcp->nToNG && sts < 500) // 同報で配送失敗が含まれるなら。
		     sts = 553;
		   //if (bNego && sts >= 400)
		     //sts = FALSE;
		 }
	   }
#ifdef V4
#ifdef UPDATE_20050124
	   us = (ULARGE_INTEGER *)&rcp->grst;
	   ue = (ULARGE_INTEGER *)&rcp->gret;
	   if ((__int64)(ue->QuadPart - us->QuadPart) > (__int64)(nGreetingTMO * 10000000))
         DelMXCashList(rcp->mDomain);  // MXキャッシュ削除
#endif
#endif
	   { // SOCKET Close
#ifdef USE_SSL
		 if (rcp->ssl) {
		   SSL_CTX_free(rcp->ctx);
           SSL_free(rcp->ssl);
#ifndef UPDATE_20060512 // SSLの初期化はサービス開始で１回のみ
#ifndef UPDATE_OPESSL1_1_1 // openssl1.1.1系に置換えた（SSL2を廃止し、TSL1.3を使用可能にする為）
           ERR_free_strings();
#endif
#endif
		   rcp->ssl = NULL;
		 }
#endif
         lingerStruct.l_onoff = 1;
         lingerStruct.l_linger = 0;
         setsockopt(msock, SOL_SOCKET, SO_LINGER,
                    (char *)&lingerStruct, sizeof(lingerStruct) );
         shutdown(msock, 2 );
	     closesocket(msock);
	   }
	 } else {
#ifdef UPDATE_20150108 // 送信許可時間の設定機能を追加
NO_CONNECT:
#endif
#ifdef V4
       DelMXCashList(rcp->mDomain);  // MXキャッシュ削除
#endif
	   if (!(fpng = fopen(rcp->mToNG, "at")))
	     fpng = fopen(rcp->mToNG, "wt");
	   if (fpng) {
		 strcpy(rcp->mCurrentTo, rcp->mTo);
	     fprintf(fpng, "%s\n", rcp->mTo);
	     fflush(fpng);
	     fclose(fpng);
		 while(!(fpng = fopen(rcp->mToNG, "rt"))) {
           if (bServiceTerminating)
             break;
		   _sleep(WAIT_TIME);
		 }
		 fclose(fpng);
	   }
	   ///////////////////
	   if (rcp->fp) {
	     strcpy(mWKTo, rcp->mTo);
         rcp->fsts = fgets(rcp->mTo, sizeof(rcp->mTo), rcp->fp);
#ifdef UPDATE_20050428
         if (rcp->fsts || !feof(rcp->fp))
#else
         if (rcp->fsts && !feof(rcp->fp))
#endif
		 {
#ifdef UPDATE_20050428
	       if (strnicmp(rcp->mTo, "recipient:", 10))
		     strtok(rcp->mTo, "\r\n");
	       else
#endif
              strword(rcp->mTo, " ", "\r\n");
           strword(rcp->mTo, "<", ">");
           strcpy(rcp->mDomain, rcp->mTo);
#ifdef UPDATE_20050428
	       if (strnicmp(rcp->mTo, "recipient:", 10))
		     strtok(rcp->mTo, "\r\n");
	       else
#endif
             strword(rcp->mTo, " ", "\r\n");
	       strword(rcp->mTo, "<", ">");
	       strword(rcp->mDomain, "@\r\n", ">\r\n");
		 } else {
	      strcpy(rcp->mTo, mWKTo);
		 }
	   }
	   ///////////////////
	   sts = (int) FALSE; //msock;
	 }
     //WSACleanup();
   }
   if (bDebug)
     printf("[%s] SendGlbalMail:return code=%d\n", pNo, sts);
#ifdef TRACE_MODE
   if (nSenderLog) { //20070320 test
     sprintf(str, "[%s] SendGlbalMail:return code=%d\n", pNo, sts);
     printfTrace(str);
   }
#endif
   return sts;
}

#ifdef UPDATE_20050901
void BoundGen(char *pCode, char * bound) {
  char mt[128], mb[1024];
  SYSTEMTIME ltime;

     gettime(&ltime, mt);
	 strcat(mt, pCode);
	 translateUue2Base64(mt, strlen(mt), mb);
	 sprintf(bound, "----%s", mb);
}
#endif

void ReturnMail(char *mNS, struct _RCP *rcp, DWORD no) {
#ifdef TRACE_MODE
   char str[LOG_BUFFER];
   char mMID[512];
#endif
   BOOL bAlias = FALSE;
   struct _RCP r;
#ifdef UPDATE_20071205 // メールアドレス長が255Byteのときの対策
   char mFrom[512], mTo[512], mrcpTo[512], *pdom;
   char mwork[512], mMSG[512], mPM[512], mt[128], *p;
   char mUserRcp[2][512], mUserMsg[512];
   char mPostRcp[2][512], mPostMsg[512];
#else
   char mFrom[256], mTo[256], mrcpTo[256], *pdom;
   char mwork[256], mMSG[256], mPM[256], mt[128], *p;
   char mUserRcp[2][256], mUserMsg[256];
   char mPostRcp[2][256], mPostMsg[256];
#endif
   char mNo[80];
#ifdef Y2038_BUG
   SYSTEMTIME ltime;
#else
   time_t ltime;
#endif
   FILE *fpsrc, *fpd; //, *fp;
   int  i, sts = 0;
   DWORD nMaxTry; //, nSMTPType;
   BOOL bNotNS = FALSE;
   BOOL bNotMX = FALSE;
   BOOL bEnd = FALSE;
#ifdef UPDATE_20050901
   char mbound[256];
#endif
#ifdef LGWAN_ON        // LGWAN機能拡張
   CHAR mOpt[128];
#endif
#ifdef UPDATE_20140711 // エラーメール生成時の"multipart/report"に対応するオプションを追加した。
   CHAR mStscd[6] = "5.0.0";
#endif

#ifdef UPDATE_0x15639_HUNG_DEBUG_A // 0x15639ハング調査用(これを入れないとなぜかハングする)
  if (p = strrchr(rcp->mMsg, '\\'))
  {
	strcpy(mMID, p+1);
  } else {
	strcpy(mMID, rcp->mMsg);
  }
  strtok(mMID, ".");
  sprintf(str, "[%s] [%03d]:ReturnMail() start. mNS=%s\n", mMID, no, mNS);
  if (nSenderLog) //if (nSenderLog || nSender2Log) 
  {
    printfTrace(str);
  }
  if (bDebug)
  {
	printf("%s", str);
  }
#endif

   memcpy(&r, rcp, sizeof(struct _RCP));
//printf("r.mTo size=%d", strlen(r.mTo));
   // リターンメールの再エラーの場合はリターンさせない。
   if (strstr(r.mMsg, "-E")) {
	 ////////////////////////////////////////
	 // MSGファイルを移動
#ifdef UPDATE_20060628 // deadフォルダに保管しないオプション
	 if (bSaveDead)
#endif
	 {
	   p = strrchr(r.mMsg,'\\');
	   if (p)
         sprintf(mwork, "%sdead%s", mMailSpoolDir, p);
	   else
         sprintf(mwork, "%sdead%s", mMailSpoolDir, r.mMsg);
       X_CopyFile(r.mMsg, mwork, TRUE);
	 }
	 return;
   }

#ifdef LGWAN_ON        // LGWAN機能拡張
#ifdef UPDATE_20241217_RCP // gateway.datにSMTP認証の指定が別のセッションで上書きされない対策
   LGWANControl(r.mTo, r.mFrom, &r.nGateAuthType);
#else
   LGWANControl(r.mTo, r.mFrom);
#endif
#endif
   r.nToNG = r.nToOK = 0;
   strcpy(mFrom, r.mFrom);
   strcpy(mrcpTo, r.mTo);
   strcpy(mPM, r.mTo);
   strcpy(mTo, r.mTo);
   strtok(mTo, "@");
   //strword(rcp->mDomain, "@", "\r\n");
   strword(mPM, "@", "\r\n");
   strcpy(r.mTo, r.mFrom);
#ifdef TRACE_MODE
if (nSenderLog || nSender2Log)
{
#ifdef UPDATE_0x15639_HUNG_DEBUG_A // 0x15639ハング調査用(これを入れないとなぜかハングする)
  sprintf(str, "[%s] [%03d]:ReturnMail Start:RCPT To=%s\n", mMID, no, r.mTo);
#else
  sprintf(str, "[%s-%03d]:ReturnMail Start:RCPT To=%s\n", r.mMNo, no, r.mTo);
#endif
  printfTrace(str);
}
#endif
   if (strstr(r.mAnswer, "moderator"))
     sprintf(r.mFrom, "%s-request@%s", mTo, r.mDomain); //r.mMID);
   else {
	 if (strstr(mPostmaster,"@"))
       sprintf(r.mFrom, "%s", mPostmaster);
	 else
       sprintf(r.mFrom, "%s@%s", mPostmaster, r.mDomain); //r.mMID); //mPM);
   }
   //// 2000.12.7
   strcpy(r.mDomain, r.mTo);
   strword(r.mDomain, "@", "\r\n");
   /////////////////////////////////

   if (strstr(r.mFrom, "-request@") &&  // 両方ともリクエストアドレスなら管理者へ返信
	   strstr(r.mTo, "-request@") ) {
	 if (strstr(mPostmaster,"@"))
       sprintf(r.mTo, "%s", mPostmaster);
	 else
       sprintf(r.mTo, "%s@%s", mPostmaster, r.mMID); //mPM);
   }
#ifdef UPDATE_20091210 // 送信エラーの返信メール送信時にハングする可能性
   if (!mNS)
   {
     bNotNS = TRUE;
	 r.mSmtp[0] == '\x0';
   } else
#endif
   {
//if (mNS != 0x13ecb8)
#ifdef UPDATE_0x15639_HUNG_DEBUG
printf("[%s-%03d]:ReturnMail mNS=%x\n", r.mMNo, no, mNS);
if (mNS > 0x0a000000)
{
  if (nSenderLog || nSender2Log) 
  {
#ifdef UPDATE_0x15639_HUNG_DEBUG_A // 0x15639ハング調査用(これを入れないとなぜかハングする)
    sprintf(str, "[%s] [%03d]:ReturnMail mNS%x\n", mMID, no, mNS);
#else
    sprintf(str, "[%s-%03d]:ReturnMail mNS%x\n", r.mMNo, no, mNS);
#endif
    printfTrace(str);
  }
  return;
}
#endif
//printf("[%s-%03d]:ReturnMail mNS=%s\n", r.mMNo, no, mNS);
     strcpy(mwork, mNS); // 0x15639 ハングの理由
     if (strcmp(r.mDomain, " ") != 0 && strcmp(r.mDomain, "\n") != 0 && strlen(r.mDomain) )
	 {
       //if (!GetSMTPServer(mNS, r.mDomain, r.mMsg, r.mSmtp, &bNotNS, &bNotMX)) {
#ifdef UPDATE_20241217_RCP // gateway.datにSMTP認証の指定が別のセッションで上書きされない対策
	   if (!GetSMTPServer(mNS, r.mDomain, r.mFrom, r.mTo, r.mMsg, r.mSmtp, &r.nPort, &r.bUsedSSL, &bNotNS, &bNotMX, &r.nGateAuthType))
#else
#ifdef UPDATE_20241128 // GATEWAY.DATにエンベロープの送信元の条件も追加可能に拡張した。
	   if (!GetSMTPServer(mNS, r.mDomain, r.mFrom, r.mTo, r.mMsg, r.mSmtp, &r.nPort, &r.bUsedSSL, &bNotNS, &bNotMX))
#else
#ifdef UPDATE_20141023 // メールアドレス単位でもGatway.datでフォワード指定可能にする対策
	   if (!GetSMTPServer(mNS, r.mDomain, r.mTo, r.mMsg, r.mSmtp, &r.nPort, &r.bUsedSSL, &bNotNS, &bNotMX))
#else
	   if (!GetSMTPServer(mNS, r.mDomain, r.mMsg, r.mSmtp, &r.nPort, &r.bUsedSSL, &bNotNS, &bNotMX))
#endif
#endif
#endif
	   {
 	     strcpy(r.mSmtp, r.mDomain);
	   }
	    nMaxTry = GetTryServer(r.mDomain, r.mMsg, NULL, TRUE);
	   if (nMaxTry < nSendMaxRetry && bNotNS == FALSE)
 	     bNotNS = TRUE;
	   if (nMaxTry < nSendMaxRetry && bNotMX == FALSE)
 	     bNotMX = TRUE;
	 } else
 	   bNotNS = TRUE;
   }
   if (bNotNS) {
     if (r.mSmtp[0] == '\x0')
 	   strcpy(r.mSmtp, r.mDomain);
#ifdef UPDATE_20091210 // 送信エラーの返信メール送信時にハングする可能性
     if (mNS)
	 {
       strcpy(mNS, mwork);
	 }
#else
     strcpy(mNS, mwork);
#endif
#ifdef TRACE_MODE
if (nSenderLog || nSender2Log) 
{
#ifdef UPDATE_0x15639_HUNG_DEBUG_A // 0x15639ハング調査用(これを入れないとなぜかハングする)
  sprintf(str, "[%s] [%03d]:ReturnMail Start:domain:[%s] smtp server:[%s]\n", mMID, no, r.mDomain, r.mSmtp);
#else
  sprintf(str, "[%s-%03d]:ReturnMail Start:domain:[%s] smtp server:[%s]\n", r.mMNo, no, r.mDomain, r.mSmtp);
#endif
  printfTrace(str);
}
#endif
	 if (bDebug)
       printf("domain:[%s] smtp server:[%s]\n", r.mDomain, r.mSmtp);
     strcpy(mMSG, r.mMsg);
     ///////////////////////////// 2001.12.30
     p = strrchr(r.mMsg, '.');
     if (p)
       *p = '\x0';
     ///////////////////////////// 2002.03.08
     p = strrchr(r.mRcp, '.');
     if (p)
       *p = '\x0';
     p = strrchr(r.mToOK, '.');
     if (p)
       *p = '\x0';
     p = strrchr(r.mToNG, '.');
     if (p)
       *p = '\x0';
     /////////////////////////////////////////
	 sprintf(mNo,"%03d", no);
	 
	 strcpy(mUserMsg, r.mMsg);
 	 strcpy(mUserRcp[0], r.mRcp);
 	 strcpy(mUserRcp[1], r.mRcp);
#ifdef UPDATE_20061129 // 同報処理内での転送エラーメール発生時にエラーメールのファイル名が重複する不具合。
     if (rcp->mSetRetryMode[0]) {
	   strcat(mUserMsg, rcp->mSetRetryMode);
 	   strcat(mUserRcp[0], rcp->mSetRetryMode);
 	   strcat(mUserRcp[1], rcp->mSetRetryMode);
	 }
#endif
	 strcat(mUserMsg, "-E");  strcat(mUserMsg, mNo);  strcat(mUserMsg,".MSG");
	 strcat(mUserRcp[0], "-E");  strcat(mUserRcp[0], mNo);  strcat(mUserRcp[0],".$CP");
	 strcat(mUserRcp[1], "-E");  strcat(mUserRcp[1], mNo);  strcat(mUserRcp[1],".RCP");
	 strcpy(mPostMsg, r.mMsg);
 	 strcpy(mPostRcp[0], r.mRcp);
 	 strcpy(mPostRcp[1], r.mRcp);
#ifdef UPDATE_20061129 // 同報処理内での転送エラーメール発生時にエラーメールのファイル名が重複する不具合。
     if (rcp->mSetRetryMode[0]) {
	   strcat(mPostMsg, rcp->mSetRetryMode);
 	   strcat(mPostRcp[0], rcp->mSetRetryMode);
 	   strcat(mPostRcp[1], rcp->mSetRetryMode);
	 }
#endif
	 strcat(mPostMsg, "-EP");  strcat(mPostMsg, mNo);  strcat(mPostMsg,".MSG");
	 strcat(mPostRcp[0], "-EP");  strcat(mPostRcp[0], mNo);  strcat(mPostRcp[0],".$CP");
	 strcat(mPostRcp[1], "-EP");  strcat(mPostRcp[1], mNo);  strcat(mPostRcp[1],".RCP");
#ifdef UPDATE_20061129 // 同報処理内での転送エラーメール発生時にエラーメールのファイル名が重複する不具合。
     if (rcp->mSetRetryMode[0]) {
	   strcat(r.mMsg, rcp->mSetRetryMode);
	   strcat(r.mRcp, rcp->mSetRetryMode);
	   strcat(r.mToOK, rcp->mSetRetryMode);
	   strcat(r.mToNG, rcp->mSetRetryMode);
	 }
#endif
	 strcat(r.mMsg, "-E");  strcat(r.mMsg, mNo);  strcat(r.mMsg,".MSG"); //tmp");
	 //strcat(r.mRcp, "-EP");  strcat(r.mRcp, mNo);  strcat(r.mRcp,".$CP");
	 strcat(r.mRcp, "-EP");  strcat(r.mRcp, mNo);  strcat(r.mRcp,".$CP");
	 strcat(r.mToOK, "-E"); strcat(r.mToOK, mNo); strcat(r.mToOK,".$OK");
	 strcat(r.mToNG, "-E"); strcat(r.mToNG, mNo); strcat(r.mToNG,".$NG");
     /////////////////////////////////////////
     gettime(&ltime, mt);
#ifdef TRACE_MODE
if (nSenderLog) {
#ifdef UPDATE_0x15639_HUNG_DEBUG_A // 0x15639ハング調査用(これを入れないとなぜかハングする)
  sprintf(str, "[%s] [%03d]:ReturnMail Start:fopen(mMSG[%s])\n", mMID, no, mMSG);
#else
  sprintf(str, "[%s-%03d]:ReturnMail Start:fopen(mMSG[%s])\n", r.mMNo, no, mMSG);
#endif
  printfTrace(str);
}
#endif
     while(!(fpsrc = fopen(mMSG, "rt"))) {
       if (bServiceTerminating)
	     break;
	   _sleep(WAIT_TIME);
	 }
     if (fpsrc) {
	   if (bDebug)
	     printf("fopen(%s, \"wt\");\n", r.mMsg);
       if ((fpd = fopen(r.mMsg, "wt"))) {
#ifdef DATA_CASH
         setvbuf(fpd, rcp->mFwbuf, _IOFBF, sizeof(rcp->mFwbuf) );
#endif
         if (strstr(r.mAnswer, "moderator"))
	       fprintf(fpd, "Subject: Your message to the %s mailing list\n", mTo);
	     else
	       fprintf(fpd, "Subject: Delivery failure\n");
	     if (strstr(r.mAnswer, "moderator")) {
           fprintf(fpd, "From: %s-request@%s\n", mTo, mPM);
		   if (strcmp(mFrom, " ") != 0 && strcmp(mFrom, "\n") != 0 && strlen(mFrom))
	         fprintf(fpd, "To: %s\n", mFrom);
		 } else {
#ifdef UPDATE_20070130 // 配送不能メールの送信者アカウントの指定をオプションを追加
		   if (r.mDomain[0] == '\x0' || strchr(mMailDaemonName, '@'))
			 fprintf(fpd, "From: %s\n", mMailDaemonName);
		   else
	         fprintf(fpd, "From: %s@%s\n", mMailDaemonName, r.mDomain);
#else
		   if (r.mDomain[0] == '\x0')
			 fprintf(fpd, "From: postmaster\n");
		   else
	         fprintf(fpd, "From: postmaster@%s\n", r.mDomain);
#endif
		   if (strcmp(mFrom, " ") != 0 && strcmp(mFrom, "\n") != 0 && strlen(mFrom))
	         fprintf(fpd, "To: %s\n", mFrom);
		 }
	     fprintf(fpd, "Date: %s\n", mt);
	     fprintf(fpd, "Message-Id: <%s>\n", r.mMIDNo);
#ifdef UPDATE_20050901
#ifdef UPDATE_20140711 // エラーメール生成時の"multipart/report"に対応するオプションを追加した。
         if (nReturnMailForm) 
#else
         if (bReturnMailForm) 
#endif
		 {
           BoundGen(r.mDomain, mbound);
#ifdef UPDATE_20140711 // エラーメール生成時の"multipart/report"に対応するオプションを追加した。
	       if (nReturnMailForm == 2)
		   {
             fprintf(fpd, "Content-Type: multipart/report; report-type=delivery-status;\n\tboundary=\"%s\"\n", mbound);
		   }
		   else 
#endif
		   {
             fprintf(fpd, "Content-Type: multipart/mixed;\n\tboundary=\"%s\"\n", mbound);
		   }
		 }
#endif
	     fprintf(fpd, "\n");
         if (strstr(rcp->mAnswer, "moderator")) {
#ifdef UPDATE_20050901
#ifdef UPDATE_20140711 // エラーメール生成時の"multipart/report"に対応するオプションを追加した。
           if (nReturnMailForm) 
#else
           if (bReturnMailForm) 
#endif
		   {
             fprintf(fpd, "\n--%s\n", mbound);
             fprintf(fpd, "Content-Type: text/plain\n\n"); 
		   } 
#endif
	       fprintf(fpd, "%s\n", r.mAnswer);
#ifdef UPDATE_20050901
#ifdef UPDATE_20140711 // エラーメール生成時の"multipart/report"に対応するオプションを追加した。
           if (nReturnMailForm) 
#else
           if (bReturnMailForm) 
#endif
		   {
#ifdef UPDATE_20140711 // エラーメール生成時の"multipart/report"に対応するオプションを追加した。
	         if (nReturnMailForm == 2)
			 {
			   char *s = NULL, *ps = NULL, *e = NULL;
			   int nl = strlen(r.mAnswer);
			   if ((r.mAnswer[0] >= '0' && r.mAnswer[0] <= '5'))
			   {
				 s = r.mAnswer;
				 /*
			   } else {
                 if ((s = strstr(r.mAnswer, "> ")))
				 {
				   s+=2;
				 } else {
				   s = r.mAnswer;
				 }
				 if ((e = strchr(s, '\n')))
				 {
				   *e = '\x0';
				 }
				 s = NULL;
				 */
			   }
               fprintf(fpd, "\n--%s\n", mbound);
               fprintf(fpd, "Content-Description: Delivery report\n");
               fprintf(fpd, "Content-type: message/delivery-status\n\n");
			   /*
			   fprintf(fpd, "Received-From-MTA: dns;こちら側のメールサーバ名
 			   fprintf(fpd, "Final-Recipient: 相手側の転送先メールアドレス
			   */
#ifdef UPDATE_20220508 // 設定ドメイン毎のＩＰに送信元ＩＰを切替える設定有効時に"Reporting-MTA:"ヘッダを変更するようにした。
			   fprintf(fpd, "Reporting-MTA: dns; %s\n", (bMTAIP ? rcp->mMID : mMyFDQN));
#else
		       fprintf(fpd, "Reporting-MTA: dns; %s\n", mMyFDQN);
#endif
               fprintf(fpd, "X-Epost-Queue-ID: %s\n", mMID);
               fprintf(fpd, "X-Epost-Sender: rfc822; %s\n", rcp->mFrom);
			   // 受領日取得
			   { 
			     HANDLE             hF;
                 WIN32_FIND_DATA    FD;
				 SYSTEMTIME ltime;
				 char   mt[256];
                 if ((hF = FindFirstFile(r.mMsg, &FD)) != INVALID_HANDLE_VALUE) 
			     {
                   FileTimeToSystemTime(&FD.ftCreationTime, &ltime);
                   gettime(&ltime, &mt);
                   FindClose(hF); 
			       fprintf(fpd, "Arrival-Date: %s\n", mt); // 受領日
			     }
			   }
               fprintf(fpd, "\n");

               //fprintf(fpd, "X-E-Post-Sender: rfc822; %s\n", rcp->mFrom);
               fprintf(fpd, "Final-Recipient: rfc822; %s\n", rcp->mTo);
			   fprintf(fpd, "Original-Recipient: rfc822; %s\n", rcp->mTo);
               fprintf(fpd, "Action: failed\n");
			   // Status 生成
			   if (s)
			   {
				   /*
				 if (e)
				 {
			       if (!strncmp(e+3, " 4.", 3) ||
				       !strncmp(e+3, " 5.", 3) )
				   {
				     strncpy(mStscd, e+4, 5);
			         mStscd[5] = '\x0';
				   } 
				 } else
					 */
				 {
			       if (!strncmp(s+3, " 4.", 3) ||
				       !strncmp(s+3, " 5.", 3) )
				   {
				     strncpy(mStscd, s+4, 5);
			         mStscd[5] = '\x0';
				   } 
				 }
			   } else {
				 strcpy(mStscd, "5.2.4");
			   }
			   fprintf(fpd, "Status: %s\n", mStscd); // 550 5.2.4
			   //fprintf(fpd, "Status: %s\n", (s ? mStscd : "5.2.4")); // 550 5.2.4
               fprintf(fpd, "Remote-MTA: dns; %s\n",rcp->mSmtp); 
			   /*
			   if (s)
			   {
                 fprintf(fpd, "Diagnostic-Code: smtp; %s\n", s);
			   } else {
                 fprintf(fpd, "Diagnostic-Code: smtp; 550 %s\n", r.mAnswer);
			   }
			   */
			   if (s)
			   {
   			     fprintf(fpd, "Diagnostic-Code: smtp; %s\n", s);
			   } else {
   			     fprintf(fpd, "Diagnostic-Code: X-Epost;");
			   	 ps = r.mAnswer;
				 while((e = strchr(ps, '\n')))
				 {
				   *e = '\x0';
				   fprintf(fpd, " %s\n", ps);
				   *e = '\n';
				   ps = e+1;
				 }
			   }
               /*
			   if (e)
			   {
				 *e = '\n';
			   }
			   */
			 }
#endif
             fprintf(fpd, "\n--%s\n", mbound);
             fprintf(fpd, "Content-Type: message/rfc822\n"); 
		   }
#endif
		 } else {
#ifdef UPDATE_20050901
#ifdef UPDATE_20140711 // エラーメール生成時の"multipart/report"に対応するオプションを追加した。
           if (nReturnMailForm) 
#else
           if (bReturnMailForm) 
#endif
		   {
             fprintf(fpd, "\n--%s\n", mbound);
             fprintf(fpd, "Content-Type: text/plain\n\n"); 
		   } 
#endif
	       fprintf(fpd, "Your message has encountered delivery problems to the following recipients:\n");
	       fprintf(fpd, "%s\n", mrcpTo);
	       fprintf(fpd, "\n");
	       fprintf(fpd, "> %s\n", r.mAnswer);
	       fprintf(fpd, "\n");
#ifdef UPDATE_20050901
#ifdef UPDATE_20140711 // エラーメール生成時の"multipart/report"に対応するオプションを追加した。
           if (nReturnMailForm) 
#else
           if (bReturnMailForm) 
#endif
		   {
#ifdef UPDATE_20140711 // エラーメール生成時の"multipart/report"に対応するオプションを追加した。
	         if (nReturnMailForm == 2)
			 {
//Unable to deliver to destination domain\n> 550 5.4.4 Cannot resolve %s\n", rcp.mDomain);
			   char *s = NULL; char *ps = NULL, *e = NULL;
			   int nl = strlen(r.mAnswer);
			   if ((r.mAnswer[0] >= '0' && r.mAnswer[0] <= '5'))
			   {
				 s = r.mAnswer;
				   /*
			   } else {
                 if ((s = strstr(r.mAnswer, "> Cannot resolve")))
				 {
				   s+=2;
				 } else {
				   s = r.mAnswer;
				 }
				 if ((e = strchr(s, '\n')))
				 {
				   *e = '\x0';
				 }
				 */
			   }
               fprintf(fpd, "\n--%s\n", mbound);
               fprintf(fpd, "Content-Description: Delivery report\n");
               fprintf(fpd, "Content-type: message/delivery-status\n\n");
			   /*
			   fprintf(fpd, "Received-From-MTA: dns;こちら側のメールサーバ名
 			   fprintf(fpd, "Final-Recipient: 相手側の転送先メールアドレス
			   */
#ifdef UPDATE_20220508 // 設定ドメイン毎のＩＰに送信元ＩＰを切替える設定有効時に"Reporting-MTA:"ヘッダを変更するようにした。
			   fprintf(fpd, "Reporting-MTA: dns; %s\n", (bMTAIP ? rcp->mMID : mMyFDQN));
#else
		       fprintf(fpd, "Reporting-MTA: dns; %s\n", mMyFDQN);
#endif
               fprintf(fpd, "X-Epost-Queue-ID: %s\n", mMID);

               fprintf(fpd, "X-Epost-Sender: rfc822; %s\n", rcp->mFrom);
			   // 受領日取得
			   { 
			     HANDLE             hF;
                 WIN32_FIND_DATA    FD;
				 SYSTEMTIME ltime;
				 char   mt[256];
                 if ((hF = FindFirstFile(r.mMsg, &FD)) != INVALID_HANDLE_VALUE) 
			     {
                   FileTimeToSystemTime(&FD.ftCreationTime, &ltime);
                   gettime(&ltime, &mt);
                   FindClose(hF); 
			       fprintf(fpd, "Arrival-Date: %s\n", mt); // 受領日
			     }
			   }
               fprintf(fpd, "\n");

               fprintf(fpd, "Final-Recipient: rfc822; %s\n", rcp->mTo);
			   fprintf(fpd, "Original-Recipient: rfc822; %s\n", rcp->mTo);
               fprintf(fpd, "Action: failed\n");
			   // Status 生成
			   if (s)
			   {
				  /*
				 if (e)
				 {
			       if (!strncmp(e+3, " 4.", 3) ||
				       !strncmp(e+3, " 5.", 3) )
				   {
				     strncpy(mStscd, e+4, 5);
			         mStscd[5] = '\x0';
				   } 
				 } else 
                 */
				 {
			       if (!strncmp(s+3, " 4.", 3) ||
				       !strncmp(s+3, " 5.", 3) )
				   {
				     strncpy(mStscd, s+4, 5);
			         mStscd[5] = '\x0';
				   } 
				 }
			   } else {
				 strcpy(mStscd, "5.4.4");
			   }
			   fprintf(fpd, "Status: %s\n", mStscd);
               fprintf(fpd, "Remote-MTA: dns; %s\n",rcp->mSmtp); 
			   //fprintf(fpd, "Diagnostic-Code: smtp; %s\n", s);
			   if (s)
			   {
   			     fprintf(fpd, "Diagnostic-Code: smtp; %s\n", s);
			   } else {
   			     fprintf(fpd, "Diagnostic-Code: X-Epost;");
			   	 ps = r.mAnswer;
				 while((e = strchr(ps, '\n')))
				 {
				   *e = '\x0';
				   fprintf(fpd, " %s\n", ps);
				   *e = '\n';
				   ps = e+1;
				 }
			   }
			   /*
			   if (e)
			   {
				 *e = '\n';
			   }
			   */
			 }
#endif
             fprintf(fpd, "\n--%s\n", mbound);
             fprintf(fpd, "Content-Type: message/rfc822\n"); 
		   } else 
#endif
	       fprintf(fpd, "Your message reads (in part):\n");
		 }
	     fprintf(fpd, "\n");

	     i = 0;
	     p = fgets(mwork, sizeof(mwork), fpsrc);
	     while (p || !feof(fpsrc)) {
		   i++;
#ifdef UPDATE_20050901
           if (mwork[0] == '.' && mwork[1] == '\n') {
#ifdef UPDATE_20140711 // エラーメール生成時の"multipart/report"に対応するオプションを追加した。
             if (nReturnMailForm == 0) 
#else
			 if (!bReturnMailForm) 
#endif
			 {
		       bEnd = TRUE;  // ".\n"終了があるか？
	           fputs(mwork, fpd);
			 }
		   } else {
             fputs(mwork, fpd);
		   }
#else			 
	       if (mwork[0] == '.' && mwork[1] == '\n')
		     bEnd = TRUE;  // ".\n"終了があるか？
	       fputs(mwork, fpd);
#endif
	       if (i > 150)
	 	     break;
	       p = fgets(mwork, sizeof(mwork), fpsrc);
		 }
#ifdef UPDATE_20050901
#ifdef UPDATE_20140711 // エラーメール生成時の"multipart/report"に対応するオプションを追加した。
         if (nReturnMailForm) 
#else
         if (bReturnMailForm) 
#endif
		 {
           fprintf(fpd, "\n--%s--\n", mbound);
		 }
#endif
	     if (!bEnd)
           fprintf(fpd, "\n.\n");
         fflush(fpd);
	     fclose(fpd);
	   }
	   fclose(fpsrc);
	 }
     sts = TRUE;
#ifdef UPDATE_20240127 // ＭＬ制御応答＆送信エラー通知メールにもDKIMサインを追加可能にするオプション
#ifdef UPDATE_20240127C // DKIMサイン実施無効フラグを追加 1:無効(bit0:送信エラー bit1:ＭＬ制御応答 bit2:転送メール bit:3自動応答)
     if (bDKIM && (nOnDKIM & 0x1)) // 送信エラー
#else
     if (bDKIM)
#endif
       AddDKIMHeader(r.mMsg, r.mMID);
#endif
	 if (strlen(r.mTo)) { // 返信先が空白で無いならエラーメール送信
#ifndef NEW_SENDER
       if (strcmp(r.mDomain, " ") != 0 && strcmp(r.mDomain, "\n") != 0 && strlen(r.mDomain) ) {
	     r.fp = NULL;
#ifdef TRACE_MODE
         if (nSenderLog) {
#ifdef UPDATE_0x15639_HUNG_DEBUG_A // 0x15639ハング調査用(これを入れないとなぜかハングする)
           sprintf(str, "[%s] [%03d]:ReturnMail SendGlobalMail:RCPT To=%s\n", mMID, no, r.mTo);
#else
           sprintf(str, "[%s-%03d]:ReturnMail SendGlobalMail:RCPT To=%s\n", r.mMNo, no, r.mTo);
#endif
           printfTrace(str);
		 }
#endif
         sts = SendGlobalMail(&r);
	     if (r.nToOK)
	       outlog(&r);  // 送信成功なら;
	   } else
	     sts = FALSE;
#else
	   strcpy(mrcpTo, r.mTo);
	   pdom = strstr(mrcpTo, "@");
       if (pdom)
         *pdom = '\x0';
#ifdef UPDATE_20070509 // エイリアス設定の実アドレスオプションを別データとして処理する
#ifdef LGWAN_ON        // LGWAN機能拡張
	   bAlias = GetAliases((LPCTSTR)SOFT_ALIASES_REG, (char *)mrcpTo, (char *)pdom, mOpt);
       if (!(mOpt[0] == '\x0' || mOpt[0] == '0' || mOpt[0] == '1'))
	   {
         bAlias = FALSE;
	   }
#else
	   bAlias = GetAliases((LPCTSTR)SOFT_ALIASES_REG, (char *)mrcpTo, (char *)pdom, NULL);
#endif
#else
	   bAlias = GetAliases((LPCTSTR)SOFT_ALIASES_REG, (char *)mrcpTo, (char *)pdom);
#endif
#ifdef TRACE_MODE
       if (nSenderLog) {
         if (bAlias) {
#ifdef UPDATE_0x15639_HUNG_DEBUG_A // 0x15639ハング調査用(これを入れないとなぜかハングする)
           sprintf(str, "[%s] GetRCPFile:GetAliases(%s -> %s)\n", mMID, r.mTo, mrcpTo);
#else
           sprintf(str, "[%s] GetRCPFile:GetAliases(%s -> %s)\n", r.mMNo, r.mTo, mrcpTo);
#endif
		 } else {
#ifdef UPDATE_0x15639_HUNG_DEBUG_A // 0x15639ハング調査用(これを入れないとなぜかハングする)
           sprintf(str, "[%s] GetRCPFile:GetAliases(%s -> isn't alias.)\n", mMID, r.mTo);
#else
           sprintf(str, "[%s] GetRCPFile:GetAliases(%s -> isn't alias.)\n", r.mMNo, r.mTo);
#endif
		 }
         printfTrace(str);
	   }
#endif
       if ((r.fp =  fopen(mUserRcp[0], "wt"))) {
         fprintf(r.fp,"Message-ID: <%s>\n",r.mMIDNo);
#ifdef UPDATE_20231103 // エラーメール生成時にメールヘッダの情報を利用するオプション機能を追加。
		 GetEnvelope(mMailEnvFrom, mMSG, r.mFrom);
         fprintf(r.fp, "Return-path: %s\n", (bReturnMailEnvelope ? "" : r.mFrom));
		 GetEnvelope(mMailEnvTo, mMSG, r.mTo);
         fprintf(r.fp, "Recipient: %s\n", (bAlias ? mrcpTo : r.mTo));
#else
#ifdef UPDATE_20061118 // エラーメールのエンベロープのFROM:を空白にするオプション
         fprintf(r.fp, "Return-path: %s\n", (bReturnMailEnvelope ? "" : r.mFrom));
#else
         fprintf(r.fp, "Return-path: %s\n",r.mFrom);
#endif
         fprintf(r.fp, "Recipient: %s\n", (bAlias ? mrcpTo : r.mTo));
#endif
	     fflush(r.fp);
	     fclose(r.fp);
	   }
#endif
	 }
   }

#ifndef NEW_SENDER
     if (sts != TRUE && 
		 SendGlobalMailStatus(r.mDomain, r.mMsg, sts, FALSE) &&
		 bNotNS) {
#ifdef UPDATE_20061129 // 同報処理内での転送エラーメール発生時にエラーメールのファイル名が重複する不具合。
	   strcpy(r.mSetRetryMode, "");
#endif
       SetRetry(&r, TRUE, (DWORD) sts, 1); //TRUE); // ドメインが応答なしならDomainフォルダとHoldフォルダに振分け保存
#ifdef TRACE_MODE
       if (nSenderLog) {
#ifdef UPDATE_0x15639_HUNG_DEBUG_A // 0x15639ハング調査用(これを入れないとなぜかハングする)
         sprintf(str, "[%s] [%03d]:ReturnMail SetRetry:RCPT To=%s\n", mMID, no, r.mTo);
#else
         sprintf(str, "[%s-%03d]:ReturnMail SetRetry:RCPT To=%s\n", r.MMNo, no, r.mTo);
#endif
         printfTrace(str);
	   }
#endif
     } else if (sts != TRUE || !bNotNS || !bNotMX) { // 送信失敗ならdeadフォルダに移動。
	   if (bDebug)
         printf("Return Messages %s not send.\n", r.mDomain);
	   ////////////////////////////////////////
	   // MSGファイルを移動
#ifdef UPDATE_20060628 // deadフォルダに保管しないオプション
	   if (bSaveDead) {
#endif
         sprintf(mwork, "%sdead\\%s-%s.MSG", mMailSpoolDir, r.mMNo, mNo);
         X_CopyFile(r.mMsg, mwork, TRUE);
#ifdef UPDATE_20060628 // deadフォルダに保管しないオプション
	   }
#endif
	   _unlink(r.mToNG); //DeleteFile(r.mToNG);
	 } 
#endif
   if (bSendPostMaster) { //postmasterにも送信なら
	 strcpy(mrcpTo, mPostmaster);
	 pdom = strstr(mrcpTo, "@");
     if (pdom)
       *pdom = '\x0';
#ifdef UPDATE_20070509 // エイリアス設定の実アドレスオプションを別データとして処理する
#ifdef LGWAN_ON        // LGWAN機能拡張
	 bAlias = GetAliases((LPCTSTR)SOFT_ALIASES_REG, (char *)mrcpTo, (char *)pdom, mOpt);
     if (!(mOpt[0] == '\x0' || mOpt[0] == '0' || mOpt[0] == '1'))
	 {
       bAlias = FALSE;
	 }
#else
	 bAlias = GetAliases((LPCTSTR)SOFT_ALIASES_REG, (char *)mrcpTo, (char *)pdom, NULL);
#endif
#else
	 bAlias = GetAliases((LPCTSTR)SOFT_ALIASES_REG, (char *)mrcpTo, (char *)pdom);
#endif
#ifdef TRACE_MODE
if (nSenderLog) {
     if (bAlias) {
#ifdef UPDATE_0x15639_HUNG_DEBUG_A // 0x15639ハング調査用(これを入れないとなぜかハングする)
       sprintf(str, "[%s] GetRCPFile:GetAliases(%s -> %s)\n", mMID, mPostmaster, mrcpTo);
#else
       sprintf(str, "[%s] GetRCPFile:GetAliases(%s -> %s)\n", r.mMNo, mPostmaster, mrcpTo);
#endif
	 } else {
#ifdef UPDATE_0x15639_HUNG_DEBUG_A // 0x15639ハング調査用(これを入れないとなぜかハングする)
       sprintf(str, "[%s] GetRCPFile:GetAliases(%s -> isn't alias.)\n", mMID, mPostmaster);
#else
       sprintf(str, "[%s] GetRCPFile:GetAliases(%s -> isn't alias.)\n", r.mMNo, mPostmaster);
#endif
	 }
     printfTrace(str);
}
#endif
#ifndef NEW_SENDER
     if ((r.fp =  fopen(r.mRcp, "wt")))
#else
     if ((r.fp =  fopen(mPostRcp[0], "wt")))
#endif
	 {
       fprintf(r.fp,"Message-ID: <%s>\n",r.mMIDNo);
       fprintf(r.fp, "Return-path: %s\n",r.mFrom);
       fprintf(r.fp, "Recipient: %s\n", (bAlias ? mrcpTo : mPostmaster));
	   fflush(r.fp);
	   fclose(r.fp);
	 }
     X_CopyFile(r.mMsg, mPostMsg, TRUE); // Postmasterへの送信 MSGファイル
   } else {
#ifndef NEW_SENDER
     _unlink(r.mMsg); //DeleteFile(r.mMsg);  // Bxxxxxxxx.msgを削除
#endif
   }
#ifdef NEW_SENDER
   if (strlen(r.mTo)) { // 返信先がある場合
	 X_MoveFile(mUserRcp[0], mUserRcp[1]); // 送信元への返信 RCPファイル
   } else {               // 返信先がない場合
     _unlink(r.mMsg); //DeleteFile(r.mMsg);  // Bxxxxxxxx.msgを削除
   }
   if (bSendPostMaster) {// 管理者への返信する場合
	 X_MoveFile(mPostRcp[0], mPostRcp[1]);// Postmasterへの送信 RCPファイル
   }
#endif

#ifdef TRACE_MODE
  if (nSenderLog || nSender2Log) {
#ifdef UPDATE_0x15639_HUNG_DEBUG_A // 0x15639ハング調査用(これを入れないとなぜかハングする)
    sprintf(str, "[%s] [%03d]:ReturnMail Completed.\n", mMID, no);
#else
    sprintf(str, "[%s-%03d]:ReturnMail Completed.\n", r.mMNo, no);
#endif
    printfTrace(str);
  }
#endif
}

void SendLocalMail(char *mNS, struct _RCP *rcp, char *mfn, DWORD i) {
#ifdef TRACE_MODE
   char str[LOG_BUFFER];
#endif
  struct _MAIL_CTL mc;
#ifdef UPDATE_20141115 // 独自アカウント運用時で最大文字数を128byteまで拡張可能にする対策をした。
  char mMB[256], mMail[256], mNoMail[256], mCtl[256], mTmp[256];
  char mUser[256], mUser2[256];
  char mMsg[256], mMwk[256], *tmp, *p;
  char mFrom[256], mTo[256], mNo[32];
#ifdef V4
  char mPermitFromFile[256];
#endif
#ifdef E_POST
  char mYesMail[256];
  char mKey[256];
#endif
#else
  char mMB[256], mMail[256], mNoMail[256], mCtl[256], mTmp[256];
  char mUser[128], mUser2[128];
  char mMsg[128], mMwk[256], *tmp, *p;
  char mFrom[256], mTo[256], mNo[32];
#ifdef V4
  char mPermitFromFile[256];
#endif
#ifdef E_POST
  char mYesMail[256];
  char mKey[256];
#endif
#endif
  FILE *fp;

#ifdef TRACE_MODE
#ifdef UPDATE_20060904A // ロックファイルの作成確認
if (nSenderLog || nSender2Log)
#else
if (nSenderLog)
#endif
{
  sprintf(str, "[%s]:SendLocalMail:domain=%s\n", rcp->mMNo, rcp->mTo);
  printfTrace(str);
}
#endif
    strcpy(mUser, rcp->mTo);
	strcpy(mUser2, rcp->mTo);
#ifndef V3
	strtok(mUser, "@");
#else
	/////////////////////////////////////////////////
	// 選択ドメイン別に保存先メールボックスを代える為
	// ドメインを加工しないままフォルダー位置取得
	/////////////////////////////////////////////////
#endif
	strcpy(mMsg, mfn);
	///////////////////////////// 2001.12.30
    //strtok(mMsg,".");
    p = strrchr(mMsg, '.');
    if (p)
      *p = '\x0';
    /////////////////////////////////////////
//	mMsg[strlen(mMsg)-4] = '\x0'; // 5.18
	strcat(mMsg,".MSG");
	//strcpy(mMB,  mMailBoxDir);
    GetMailBoxFolder(mUser, mMB);  // 2000.06.17 %HOME%が使えるように
#ifdef MULTI_ML
   //if (!QueryMLists((LPCTSTR)SOFT_LISTS_REG, mUser2)) // ドメイン付MLが無い場合
   //  strtok(mUser2,"@");
#else
   strtok(mUser2,"@");
#endif
	if (CheckLists(SOFT_LISTS_REG, mUser2)) { // メーリングリストかどうか？
	  if (bDebug)
	    printf("This account mailing list\n");
	  strcpy(mFrom, rcp->mFrom);
	  strcpy(mTo, rcp->mTo);
///////////////////////////////
	  if (!RestorMLRCPFile(rcp, mUser2, i)) {
	    sprintf(rcp->mAnswer,"It wasn't possible to deliver to the list user(It wasn't possible to have taken out a list).\n", mUser);
#ifdef UPDATE_20061129 // 同報処理内での転送エラーメール発生時にエラーメールのファイル名が重複する不具合。
        strcpy(rcp->mSetRetryMode, "");
#endif
#ifdef UPDATE_0x15639_HUNG_DEBUG
printf("[%s-%03d]:SendLocalMail(1) mNS=%x\n", rcp->mMNo, i, mNS);
if (mNS > 0x0a000000)
{
  if (nSenderLog || nSender2Log) 
  {
    sprintf(str, "[%s-%03d]:SendLocalMail(1) mNS=%x\n", rcp->mMNo, i, mNS);
    printfTrace(str);
  }
  exit(0);
}
if (i > 10000)
{
  exit(0);
}
#endif
#ifdef  CS_RETURNMAIL // ReturnMail()中の割り込み処理禁止
   EnterCriticalSection(&g_csReturnMail);
#endif
	    ReturnMail(mNS, rcp, i);
#ifdef  CS_RETURNMAIL // ReturnMail()中の割り込み処理禁止
  LeaveCriticalSection(&g_csReturnMail);
#endif
        faillog(rcp);               // 配送エラーログ
	  }
#ifdef TRACE_MODE
#ifdef UPDATE_20060904A // ロックファイルの作成確認ログ出力用
      if (nSenderLog || nSender2Log)
#else
      if (nSenderLog == 2)
#endif
	  {
        sprintf(str, "[%s] SendLocalMail() bThreadLists = %d\n", rcp->mMNo, bThreadLists);
        printfTrace(str);
	  }
#endif
#ifdef NEW_SENDER
#ifdef UPDATE_20060904B // bThreadLists = TRUE;が利いたままになる
	  //{
#else
      if (!bThreadLists) 
#endif
	  {
	    bThreadLists = TRUE;
#ifndef V4
#ifdef TRACE_MODE
#ifdef UPDATE_20060904A // ロックファイルの作成確認
       if (nSenderLog || nSender2Log)
#else
       if (nSenderLog == 2)
#endif
	   {
          sprintf(str, "[%s] start RetryStart(%s, NULL, \"lists\", FALSE) SendLocalMail()\n",rcp->mMNo, mNS);
          printfTrace(str);
		}
#endif
#endif
	    RetryStart(mNS, NULL, "lists", FALSE); // メーリングリストなら
	    bThreadLists = FALSE;
	  }
#endif
	  strcpy(rcp->mFrom, mFrom);
	  strcpy(rcp->mTo, mTo);
	} else {
	  if (bDebug)
        printf("This account local user\n");
	  sprintf(mCtl,"%sIMS.CTL", mMB); // 2000.06.17 %HOME%が使えるように
      if (!GetMailControl(&mc, mCtl)) {
		if (bAutoCreateInbox) {
	      sprintf(mMail,"%s", mMB); // 2000.06.17 %HOME%が使えるように
		  //_mkdir(mMail);
	      tmp = strstr(mMail,":\\");
	      if (tmp)
	        tmp = strstr(tmp+2,"\\");
#ifdef REGTOFILE
          else if ((tmp = strstr(mMail,"\\\\"))) {
            if ((tmp = strstr(tmp+2,"\\")))
              tmp = strstr(tmp+1,"\\");
		  }
#endif
		  else
	        tmp = strstr(mMail,"\\");
          while(tmp) {
            *tmp = '\x0';
			if (mMail[0] != '\x0')
	          _mkdir(mMail);         // 処理用フォルダ作成
            *tmp = '\\';
	        tmp = strstr(tmp+1,"\\");
		  }
	      _mkdir(mMail);         // 処理用フォルダ作成
		}
#ifdef UPDATE_20170304_FILENAME_ADD_DATE // 保存ファイルに日付情報付加
		{
          time_t lt;
		  char *pext;
			  
          if (pext= strrchr(mMsg, '.'))
		  {
			*pext = '\x0';
            time(&lt);
            sprintf(mMail,"%s%s_%lu.%s", mMB, mMsg, lt, pext+1); // 2000.06.17 %HOME%が使えるように

		  } else {
            sprintf(mMail,"%s%s", mMB, mMsg); // 2000.06.17 %HOME%が使えるように
		  }
		}
#else
        sprintf(mMail,"%s%s", mMB, mMsg); // 2000.06.17 %HOME%が使えるように
#endif
#ifdef UPDATE_20230105 // X-UIDLヘッダを追加するオプション
        AddUIDLHeader(rcp->mMsg, mMail, TRUE);
#endif
#ifdef V3                                 // 2002.09.01
		if (bInboxEnc)
		  CopyFile_Encode(rcp->mMsg, mMail);
		else
          X_CopyFile(rcp->mMsg, mMail, bOverlapFile); //TRUE);
#else
        X_CopyFile(rcp->mMsg, mMail, bOverlapFile); //TRUE);
#endif

#ifdef UPDATE_20230105 // X-UIDLヘッダを追加するオプション
	   AddUIDLHeader(rcp->mMsg, mMail, FALSE);
#endif
        if (bDebug)
	      printf("CopyFile(%s, %s)\n",rcp->mMsg, mMail);
#ifdef WAX_ON
		SendPipeLine("Request Waxsv"); // Waxに受信チェックするよう通知。
#endif
	  } else { // MAIL CNTROL ファイルがあるなら処理する。
		////////////// バックアップ
		strcpy(mFrom, rcp->mFrom);
		strcpy(mTo, rcp->mTo);
		fp = rcp->fp; // ファイルから送信先処理しないようにする。
		rcp->fp = NULL;
		///////////////////////////
#ifdef AUTOPROCESS
		if (mc.bAutoProcess) {  // アドインプロセス処理
          AutoProcess(&mc, mMB, rcp->mMsg);
		}
#endif
		strcpy(mMwk, rcp->mMsg); // バックアップ
	    if (mc.nMode == 1) {  // 自動転送
		  if (mc.bDataDiscard) {   // 2001.12.30 転送時に添付ファイルを破棄するか否か、0(no):破棄しない、1(yes):破棄する
            strcpy(mMail, rcp->mMsg); // 2000.06.17 %HOME%が使えるように
			p = strrchr(mMail,'.');
            if (p)
	          *p = '\x0';
			sprintf(mNo, "-C%08lu", i);
			strcat(mMail, mNo);
			strcat(mMail, ".MSG");
			MailDataDiscard(&mc, rcp->mMsg, mMail);
            strcpy(mTmp, rcp->mMsg);
		    strcpy(rcp->mMsg, mMail);
		  }
#ifdef UPDATE_20080626 // 転送先アドレスの先頭に全角空白や半角空白が含まれている場合は、転送処理をスキップするようにした。
		  if (mc.ForwardTo[0] != '\x0' &&
			  mc.ForwardTo[0] != ' ' &&
			  strnicmp(mc.ForwardTo, "　", 2))
#else
		  if (mc.ForwardTo[0] != '\x0')
#endif
		  { // 転送先アドレスが指定されていれば実行
#ifdef V4
	        sprintf(mPermitFromFile,"%spermitfrom.txt", mMB); // 許可送信元
			if (QueryPermitFrom(mFrom, mPermitFromFile))      // 転送指定よい送信元かチェック
#endif
            AutoTransfar(mNS, rcp, &mc, mfn, i);
		  }
		  if (mc.bDataDiscard) {    // 2001.12.30 
            strcpy(rcp->mMsg, mTmp); // 元に戻す
            _unlink(mMail); //DeleteFile(mMail);       // 作成ファイル削除
		  }
		} else if (mc.nMode == 2) { // 自動応答
#ifdef E_POST
          //sprintf(mYesMail,"%sattach\\REG_USER.TXT", mMailSpoolDir); // 2000.06.17 %HOME%が使えるように
          sprintf(mYesMail,"%sYESREPLY.TXT", mMB); // 2000.06.17 %HOME%が使えるように
          sprintf(mKey,"%sKEY.TXT", mMB);          // 2000.06.17 %HOME%が使えるように
	      if (!YesReply(rcp, mYesMail, mKey)) {  // Reply してよいメールアドレスか否かチェック
#endif
		    if (mc.bRecordReplies) {
              sprintf(mCtl,"%sREPLIED.TXT", mMB); // 2000.06.17 %HOME%が使えるように
			  RecordReplies(rcp->mFrom, mCtl, mc.bRecordDate);
			}
            sprintf(mNoMail,"%sNOREPLY.TXT", mMB); // 2000.06.17 %HOME%が使えるように
		    if (!NoReply(rcp, mNoMail)) {  // Reply しないメールアドレスか否かチェック
              sprintf(mMail, "%sAUTORPLY.TXT", mMB); // 2000.06.17 %HOME%が使えるように
              AutoReply(mNS, rcp, &mc, mMail, FALSE, i);
			  if (mc.RecordForward[0] != '\x0') {   // UPDATE 2000.09.21
                sprintf(mMail, "%sRPLYUSER.TXT", mMB);
                AutoReply(mNS, rcp, &mc, mMail, TRUE, i);
			  }
			}
			// NOREPLY.TXTへのコピーは１回目自動応答後セットする 2003.02.27
			if (mc.bCopyReplies) { // 2001.12.30 自動応答メールアドレスをNOREPLY.TXTにもコピーする
              sprintf(mCtl,"%sNOREPLY.TXT", mMB); // 2000.06.17 %HOME%が使えるように
			  RecordReplies(rcp->mFrom, mCtl, FALSE);
			}

#ifdef E_POST
		  } else {
            sprintf(mNoMail,"%sNOREPLY.TXT", mMB); // 2000.06.17 %HOME%が使えるように
		    if (!NoReply(rcp, mNoMail)) {  // Reply しないメールアドレスか否かチェック
              sprintf(mMail, "%sAUTOFAIL.TXT", mMB); // 2000.06.17 %HOME%が使えるように
	          if (strstr(mPostmaster,"@"))
                sprintf(rcp->mTo, "%s", mPostmaster);
	          else
  	            sprintf(rcp->mTo, "%s@%s", mPostmaster, rcp->mDomain); //rcp->mSmtp); // エラーメールとして返信
              AutoReply(mNS, rcp, &mc, mMail, FALSE, i);
			  if (mc.RecordForward[0] != '\x0') { // UPDATE 2000.09.21
                sprintf(mMail, "%sRPLYFAIL.TXT", mMB); 
                AutoReply(mNS, rcp, &mc, mMail, TRUE, i);
			  }
			}
		  }
#endif
		}
		///////////////////////////
		strcpy(rcp->mMsg, mMwk); // 戻す
        if (mc.bDiscard) {  // ファイルを残す
#ifdef UPDATE_20170304_FILENAME_ADD_DATE // 保存ファイルに日付情報付加
		{
           time_t lt;
		   char *pext;
			  
           if (pext= strrchr(mMsg, '.'))
		   {
		 	 *pext = '\x0';
             time(&lt);
             sprintf(mMail,"%s%s_%lu.%s", mMB, mMsg, lt, pext+1); // 2000.06.17 %HOME%が使えるように
		   } else {
             sprintf(mMail,"%s%s", mMB, mMsg); // 2000.06.17 %HOME%が使えるように
		   }
		}
#else
          sprintf(mMail,"%s%s", mMB, mMsg); // 2000.06.17 %HOME%が使えるように
#endif
#ifdef UPDATE_20230105 // X-UIDLヘッダを追加するオプション
          AddUIDLHeader(rcp->mMsg, mMail, TRUE);
#endif
#ifdef V3
		  if (bInboxEnc)                    // 2002.09.01
			CopyFile_Encode(rcp->mMsg, mMail);
		  else
            X_CopyFile(rcp->mMsg, mMail, bOverlapFile); //TRUE);
#else
          X_CopyFile(rcp->mMsg, mMail, bOverlapFile); //TRUE);
#endif
#ifdef UPDATE_20230105 // X-UIDLヘッダを追加するオプション
	      AddUIDLHeader(rcp->mMsg, mMail, FALSE);
#endif
		  if (bDebug)
            printf("CopyFile(%s, %s)\n",rcp->mMsg, mMail);
		}
		////////////// 元に戻す
		strcpy(rcp->mFrom, mFrom);
		strcpy(rcp->mTo, mTo);
		rcp->fp = fp;// ファイルポインタ復帰
		////////////////////////
	  }
	}
}

BOOL SetRecpient(struct _RCP *rcp, FILE *fp) {
  FILE *fpp;
#ifdef UPDATE_20071205 // メールアドレス長が255Byteのときの対策
  char mwork[512], *p;
#else
  char mwork[256], *p;
#endif
  BOOL bin = FALSE;

  //// 送信先受入ファイルが残っていればデータ取り出し
  fpp = fopen(rcp->mToOK, "rt");
  if (fpp) {
#ifdef DATA_CASH
    setvbuf(fpp, rcp->mFrbuf, _IOFBF, sizeof(rcp->mFrbuf) );
#endif
    do {
	  p = fgets(mwork, sizeof(mwork), fpp);
	  if (p) {
		strtok(mwork, "\n");
		if (mwork[0])
          fprintf(fp, "Recipient: %s\n", mwork);
	  }
	} while (p);
    fclose(fpp);
	bin = TRUE;
	_unlink(rcp->mToOK); //DeleteFile(rcp->mToOK);
  }
  //// 送信先拒否ファイルが残っていればデータ取り出し
  fpp = fopen(rcp->mToNG, "rt");
  if (fpp) {
#ifdef DATA_CASH
    setvbuf(fpp, rcp->mFrbuf, _IOFBF, sizeof(rcp->mFrbuf) );
#endif
    do {
	  p = fgets(mwork, sizeof(mwork), fpp);
	  if (p) {
		strtok(mwork, "\n");
		if (mwork[0])
          fprintf(fp, "Recipient: %s\n", mwork);
	  }
	} while (p);
    fclose(fpp);
    bin = TRUE;
	_unlink(rcp->mToNG); //DeleteFile(rcp->mToNG);
  }
  return bin;
}

BOOL SetRCPFile(struct _RCP *rcp, char *mRCP, BOOL bmod, DWORD *no) {
#ifdef TRACE_MODE
   char str[LOG_BUFFER];
#endif
  FILE *fp;
  char mwork[256], *pm, *tmp;
  BOOL bNew = TRUE, bsts = FALSE;
  long               hFindFile;
  struct _finddata_t FindFileData ;
//  int  i;
//  for (i = 0; i < 5; i++) { // bsts = FALSE だったらリトライ

#ifdef TRACE_MODE
if (nSenderLog) { // 20070320test
  sprintf(str, "[%s]:Start SetRCPFile:%s, bmod=%d\n", rcp->mMNo, mRCP, bmod);
  printfTrace(str);
}
#endif
  // フォルダが消去されていた場合にフォルダを再作成する
    if (strstr(mRCP,"\\domains\\") ||
		strstr(mRCP,"\\lists\\")) {
	  tmp = strstr(mRCP,":\\");
	  if (tmp)
	    tmp = strstr(tmp+2,"\\");
#ifdef REGTOFILE
      else if ((tmp = strstr(mRCP,"\\\\"))) {
        if ((tmp = strstr(tmp+2,"\\")))
          tmp = strstr(tmp+1,"\\");
	  }
#endif
      while(tmp) {
        *tmp = '\x0';
	    if (strstr(mRCP,"\\domains\\") ||
			strstr(mRCP,"\\lists\\")) {
	      _mkdir(mRCP);         // 処理用フォルダ作成
#ifdef TRACE_MODE
if (nSenderLog) {
  sprintf(str, "[%s]:SetRCPFile:Make folder. %s\n", rcp->mMNo, mRCP);
  printfTrace(str);
}
#endif
		  //while ((hFindFile = FindFirstFile( mRCP, &FindFileData)) == INVALID_HANDLE_VALUE) {
          while ((hFindFile = _findfirst( mRCP, &FindFileData)) == -1L) {
            if (bServiceTerminating)
		      break;
			_sleep(WAIT_TIME);
		  }
          _findclose(hFindFile); //FindClose( hFindFile ); 
		}
        *tmp = '\\';
	    tmp = strstr(tmp+1,"\\");
	  }
	}
    /////////////////////////////////////////////////////
#ifdef TRACE_MODE
if (nSenderLog) {
  sprintf(str, "[%s]:SetRCPFile:Read start. %s\n", rcp->mMNo, mRCP);
  printfTrace(str);
}
#endif
    fp = fopen(mRCP, "rt");
    if (fp) {
#ifdef DATA_CASH
      setvbuf(fp, rcp->mFrbuf, _IOFBF, sizeof(rcp->mFrbuf) );
#endif
#ifdef TRACE_MODE
if (nSenderLog) {
  sprintf(str, "[%s]:SetRCPFile:Read open success. %s\n", rcp->mMNo, mRCP);
  printfTrace(str);
}
#endif
	  fgets(mwork, sizeof(mwork), fp);
	  fgets(mwork, sizeof(mwork), fp);
      mwork[0] = '\x0';
	  do {
        fgets(mwork, sizeof(mwork), fp);
	    pm = strstr(mwork, " ");
	    if (pm) {
		  pm++;
		  strtok(pm, "\r\n");
		  if (stricmp(pm, rcp->mTo) == 0) {
#ifdef TRACE_MODE
if (nSenderLog) {
  sprintf(str, "[%s]:SetRCPFile:Mach addr. %s:%s\n", rcp->mMNo, pm, rcp->mTo);
  printfTrace(str);
}
#endif
		    bNew = FALSE;
		    break;
		  }
		}
	  } while (!feof(fp));
      fclose(fp);
#ifdef TRACE_MODE
if (nSenderLog) {
  sprintf(str, "[%s]:SetRCPFile:Read close.\n", rcp->mMNo);
  printfTrace(str);
}
#endif
	  if (bNew) {
		//for (i = 0; i < 5; i++) { // ファイルがあるのに書き込みできないならリトライしてみる。
		  if ((fp = fopen(mRCP, "at"))) {
#ifdef DATA_CASH
            setvbuf(fp, rcp->mFwbuf, _IOFBF, sizeof(rcp->mFwbuf) );
#endif
#ifdef TRACE_MODE
if (nSenderLog) {
  sprintf(str, "[%s]:SetRCPFile:Append recipient. %s\n", rcp->mMNo, rcp->mTo);
  printfTrace(str);
}
#endif
 		    bsts = TRUE;
	        if (bmod)
	          bsts = SetRecpient(rcp, fp); 
	        else {
              if (strstr(rcp->mTo, "\n"))
                fputs(rcp->mTo, fp);
	          else
                fprintf(fp, "Recipient: %s\n",rcp->mTo);
			}
	        fflush(fp);
            fclose(fp);
			//break;
		  }
		//}
	  }
	} else {
      if ((fp = fopen(mRCP, "wt"))) {
#ifdef TRACE_MODE
if (nSenderLog) {
  sprintf(str, "[%s]:SetRCPFile:Write open success. %s\n", rcp->mMNo, mRCP);
  printfTrace(str);
}
#endif
	    bsts = TRUE;
	    if (strstr(rcp->mMID, "\n"))
          fputs(rcp->mMID, fp); 
	    else
          fprintf(fp, "Message-ID: <%s>\n",rcp->mMIDNo);
	    if (strstr(rcp->mFrom, "\n"))
          fputs(rcp->mFrom, fp);
	    else
          fprintf(fp, "Return-path: %s\n",rcp->mFrom);
	    if (bmod)
	      bsts = SetRecpient(rcp, fp); 
	    else {
          if (strstr(rcp->mTo, "\n"))
            fputs(rcp->mTo, fp);
  	      else
            fprintf(fp, "Recipient: %s\n",rcp->mTo);
		}
	    fflush(fp);
        fclose(fp);
		if (no)
		  (*no)++;
	  } else {
#ifdef TRACE_MODE
if (nSenderLog) {
  sprintf(str, "[%s]:SetRCPFile:Write open failed. %s\n", rcp->mMNo, mRCP);
  printfTrace(str);
}
#endif
	  }
	}
#ifdef TRACE_MODE
if (nSenderLog) {
  sprintf(str, "[%s]:End SetRCPFile:%s\n", rcp->mMNo, (bsts ? "Success" : "Failed"));
  printfTrace(str);
}
#endif
    if (bsts) {
	  if (bDebug)
  	    printf("SetRCPFile(%s) Success.\n", mRCP);
	} else {
	  if (bDebug)
	    printf("SetRCPFile(%s) Failed.\n", mRCP);
	}
	//////////////////////////////////////
#ifdef UPDATE_20070319D // RCPファイルの作成完了を確認するまでウエイトする
	while(!(fp = fopen(mRCP, "rt"))) {
      if (bServiceTerminating)
		break;
	  _sleep(WAIT_TIME);
	}
	if (fp) 
	  fclose(fp);
#endif
	//////////////////////////////////////
  return bsts;
}

BOOL RestorMLRCPFile(struct _RCP *rcp, char *mUser, DWORD n) {
#ifdef TRACE_MODE
   char str[LOG_BUFFER];
#endif
  char mAppName[256], mwk[256], mDir[256], mLKFile[256];
  char mRCPSrc[256], mRCPDest[256], mRCPDestA[256], mRCPDestB[256], mRCPDestC[256];
  char mMSGSrc[256], mMSGDest[256], mMSGDestA[256], mMSGDestB[256];
  char *pMNo, mMNo[256], mINo[256], mN[256];
  char mkey[256];
  DWORD nBounceAction, nN; 
  char  mwork[256], *p, *pL, *pH; //, *tmp;
  FILE  *fpsrc, *fpdest, *fmes;
  BOOL  bMsg = FALSE, bFound = FALSE;
  BOOL  bExtendHeader = TRUE;
  DWORD nMLDomDiv;   // ドメイン同報分割
  DWORD nNoOfDiv;    // 分割数
  DWORD nsts = 0;
  BOOL  bCR  = TRUE; // 2000.12.28
  DWORD i, j, k, nL, dErr;
  struct _MAIL_CTL mc;
  BOOL            bFile;
  long               hF;
  struct _finddata_t FD;
  char            mRCP[256];
#ifdef REGTOFILE
  HANDLE hFile;
#endif
#ifdef UPDATE_20060904 // ロックファイルの作成確認
  long               hFindFile;
  struct _finddata_t FindFileData;
#endif
#ifdef UPDATE_20070521 // 予約語対策
  char *q, mUID[512], mKEY[512];
  BOOL bReservedWords1 = FALSE;
  BOOL bReservedWords2 = FALSE;
#endif
#ifdef  INCOM_SUB_FOLDER // incoming フォルダをスレッド別フォルダにして処理
  CHAR  *ps, mSubDir[512];
#endif
#ifdef UPDATE_20210321 // 添付分離時にMLメンバーが無い場合のエラーメール生成を抑止する
  BOOL  bListSaveOrign;
#endif

#ifdef TRACE_MODE
#ifdef UPDATE_20060904A // ロックファイルの作成確認
   if (nSenderLog || nSender2Log)
#else
   if (nSenderLog)
#endif
   {
     sprintf(str, "[%s-%03d]:Start RestorMLRCPFile:%s\n", rcp->mMNo, n, mUser);
     printfTrace(str);
   }
#endif
#ifdef UPDATE_20070521 // 予約語対策
  if (nClustering){
	strcpy(mUID, rcp->mTo);
    strtok(mUID, "@");
    if (ReservedWords(mUID)) {
      if ((q = strstr(rcp->mTo, mUID))) {
        strcpy(mUID, mReservedWords);
        strcat(mUID, q);
  	    bReservedWords1 = TRUE;
	  }
	}
	strcpy(mKEY, mUser);
    strtok(mKEY, "@");
    if (ReservedWords(mKEY)) {
      if ((q = strstr(mUser, mKEY))) {
        strcpy(mKEY, mReservedWords);
        strcat(mKEY, q);
  	    bReservedWords2 = TRUE;
	  }
	}
  }
  if (bReservedWords1)
   strcpy(mwk, mUID);
  else
#endif
   strcpy(mwk, rcp->mTo);
#ifdef MULTI_ML
   if (!QueryMLists((LPCTSTR)SOFT_LISTS_REG, mwk)) // ドメイン付MLが無い場合
     strtok(mwk, "@");
#else
   strtok(mwk, "@");
#endif
#ifdef UPDATE_20070521 // 予約語対策
   if ((pL = strstr(((bReservedWords1 || bReservedWords2) ? &mKEY[strlen(mReservedWords)] : mUser),"@"))) { // ドメイン付のままなら
	 *pL = '\x0';
     nL = strlen(((bReservedWords1 || bReservedWords2) ? mKEY : mUser));
	 *pL = '@';
   } else {
     nL = strlen(((bReservedWords1 || bReservedWords2) ? mKEY : mUser));
   }
#else
   if ((pL = strstr(mUser,"@"))) { // ドメイン付のままなら
	 *pL = '\x0';
     nL = strlen(mUser);
	 *pL = '@';
   } else {
     nL = strlen(mUser);
   }
#endif
   //if (strnicmp(&mwk[strlen(mUser)], "-request", 8) == 0) {
   if (strnicmp(&mwk[nL], "-request", 8) == 0) {
	 ListControl(rcp, mUser, n);
	 //bsts = TRUE;
	 nsts = 1;
   } else {
     sprintf(mkey,"%s\\%s", SOFT_LISTS_REG, mwk);
#ifdef UPDATE_20210321 // 添付分離時にMLメンバーが無い場合のエラーメール生成を抑止する
     bListSaveOrign = GetProfileIntEx(mkey, "ListSaveOrign", (INT)FALSE);
#endif
	 nMLDomDiv     = GetProfileIntEx(mkey, "MaxDivide", (INT)nMaxDivide);          // default ドメイン同報として分割
	 nNoOfDiv      = GetProfileIntEx(mkey, "DomainDivide", (INT)nDomainDivide);    // 分割数
	 bExtendHeader = GetProfileIntEx(mkey, "ExtendHeader", (INT)TRUE); // default Added
     nBounceAction = GetProfileIntEx(mkey, "BounceAction", (INT)0);    // default Discard
     GetProfileStringEx(mkey,"AutoModule", "", mc.Module, sizeof(mc.Module)); // AutoProcess module
	 GetProfileStringEx(mkey,"ModuleCharSet", "", mc.CharSet, sizeof(mc.Module)); // AutoProcess charset
     switch (nBounceAction) {
	    case 0: // Discard
	 	      strcpy(rcp->mFrom, ""); //strcpy(rcp->mFrom, " "); //sprintf(rcp->mFrom, "postmaster@%s", rcp->mSmtp); 
			  break;
	    case 1: // Return to Sender
		      break;
	    case 2: // Send to
		      //GetProfileStringEx(mkey,"BouncesTo", " ", rcp->mFrom, sizeof(rcp->mFrom));
		      GetProfileStringEx(mkey,"BouncesTo", "", rcp->mFrom, sizeof(rcp->mFrom));
			  break;
	 }
//（あ）     sprintf(mDir, "%slists\\%s", mMailSpoolDir, mwk);
     //_mkdir(mDir);         // listsグループフォルダ作成
     //sprintf(mRCP, "%s\\%s.RCP", mDir, rcp->mMNo);
	 /////////////////////////////////////////////////////////
	 //if (!(pMNo = strrchr(rcp->mMsg, '~')))
     pMNo = strrchr(rcp->mMsg, '\\');
	 if (!pMNo)
	   strcpy(mMNo, rcp->mMsg);
	 else
	   strcpy(mMNo, (pMNo+1));
	 ///////////////////////////////////////
	 //pINo = strrchr(rcp->mMsg, '\\');
	 //if (!pINo)
	 //  strcpy(mINo, rcp->mMsg);
	 //else
	 //  strcpy(mINo, (pINo+1));
     ///////////////////////////// 2001.12.30
     //strtok(mMNo, ".");
     p = strrchr(mMNo, '.');
     if (p)
       *p = '\x0';
     //p = strrchr(mINo, '.');
     //if (p)
       //*p = '\x0';

	 strcpy(mINo, mMNo);
	 p = strstr(mINo, "-"); 
	 if (p) {
	   *p = '\x0';
	   p++;
	   while(!(*p == '0' || *p == '1' || *p == '2' || *p == '3' || *p == '4' ||
		       *p == '5' || *p == '6' || *p == '7' || *p == '8' || *p == '9' ||
			   *p == '\x0')) {
		 p++;
	   }
	   nN = atoi(p);
	   _itoa( nN, mN, 10);
       strcat(mINo, mN);
	 }
     /////////////////////////////////////////
	 /////////////////////////////////////////////////////////
	 mRCPDest[0] = '\x0';
     sprintf(mDir, "%slists\\%s-%s", mMailSpoolDir, mwk, mINo); //mMNo); //（あ）の代わり
     sprintf(mRCP, "%s\\*.RCP", mDir); 
     sprintf(mRCPSrc, "%s\\%s.$CP", mDir, mMNo); //rcp->mMNo);
     sprintf(mRCPDestA, "%s\\%s-M%%08x.RCP", mDir, mINo); //mMNo); //rcp->mMNo);
     if (nMLDomDiv == 0) // 強制分割で無い場合はドメイン分割
	   sprintf(mRCPDestB, "%s\\%s-M%%s-%%03lu.$CP", mDir, mINo); //mMNo); //rcp->mMNo);
	 else
       sprintf(mRCPDestB, "%s\\%s-M%%08x.$CP", mDir, mINo); //mMNo); //rcp->mMNo);
#ifdef  INCOM_SUB_FOLDER // incoming フォルダをスレッド別フォルダにして処理
	 sprintf(mSubDir, rcp->mMsg);
	 if (ps = strrchr(mSubDir, '\\'))
	 {
	   *ps = '\x0';
       sprintf(mMSGSrc, "%s\\%s.MSG", mSubDir, mMNo); //rcp->mMNo);
	 } else 
#endif
	 {
       sprintf(mMSGSrc, "%sincoming\\%s.MSG", mMailSpoolDir, mMNo); //rcp->mMNo);
	 }
     sprintf(mMSGDestA, "%s\\%s-M%%08x.MSG", mDir, mINo); //mMNo); //rcp->mMNo);
     sprintf(mAppName, "%s\\%s\\Members", SOFT_LISTS_REG, mwk);
	 bFound = FALSE;
     hF = _findfirst( mRCP, &FD); //hF = FindFirstFile( mRCP, &FD);
     if (hF != -1L) { //INVALID_HANDLE_VALUE) { // 既にファイルが存在しているかチェック
		bFound = TRUE;
        _findclose(hF); //FindClose( hF ); 
	 }
     /*
	 for (i = 1; i <= nMaxThread; i++) { // 既にファイルが存在しているかチェック
       sprintf(mRCPDest, mRCPDestA, 1);
       if ((fpdest = fopen(mRCPDest, "rt"))) {
		 fclose(fpdest);
		 bFound = TRUE;
         break;
	   }
	 }
	 */
	 if (!bFound) {
	   strcpy(mLKFile, mRCPDestB);
	   p = strrchr(mLKFile, '\\');
	   if (p) {
	     p++;
	     *p = '\x0';
	     strcat(p, LOCKFILE);
	     p = strstr(mLKFile,":\\");
	     if (p)
	       p = strstr(p+2,"\\");
#ifdef REGTOFILE
         else if ((p = strstr(mLKFile,"\\\\"))) {
           if ((p = strstr(p+2,"\\")))
             p = strstr(p+1,"\\");
		 }
#endif
         while(p) {
           *p = '\x0';
	       if (strstr(mLKFile,"\\domains\\") ||
		       strstr(mLKFile,"\\lists\\"))
	          _mkdir(mLKFile);         // 処理用フォルダ作成
           *p = '\\';
	       p = strstr(p+1,"\\");
		 }
#ifdef REGTOFILE
         while ((hFile = CreateFile((LPCTSTR)mLKFile,
                        GENERIC_WRITE,
                        0,   // 排他アクセス = 0
                        NULL,
                        CREATE_ALWAYS,
                        FILE_ATTRIBUTE_NORMAL, 
                        NULL)) == INVALID_HANDLE_VALUE) {
            if (bServiceTerminating) 
              break;
            _sleep(WAIT_TIME);
#ifdef UPDATE_20101126 // ML配送用作業フォルダが作成できない場合に作成リトライを行なう処理を追加
	        if ((p = strrchr(mLKFile, '\\')))
			{
              while(p) 
			  {
                *p = '\x0';
	            if (strstr(mLKFile,"\\domains\\") ||
		            strstr(mLKFile,"\\lists\\"))
	              _mkdir(mLKFile);         // 処理用フォルダ作成
                *p = '\\';
	            p = strstr(p+1,"\\");
			  }
			}
#endif
		 } 
#ifdef UPDATE_20060904 // ロックファイルの作成確認
		 while ((hFindFile = _findfirst( mLKFile, &FindFileData)) == -1L)
		 {
            if (bServiceTerminating) 
              break;
            _sleep(WAIT_TIME);
		 }
         _findclose(hFindFile);
#endif
#else
		 if ((fpsrc = fopen(mLKFile, "wt")))
	       fclose(fpsrc);
         while(!(fpsrc = fopen(mLKFile, "rt"))) { // 対象ML展開フォルダをロック
           if (bServiceTerminating)
		     break;
	       _sleep(WAIT_TIME);
		 }
	     fclose(fpsrc);
#endif
	   }
#ifdef UPDATE_20210321 // 添付分離時にMLメンバーが無い場合のエラーメール生成を抑止する
       if (bListSaveOrign)
		 nsts = 1;
	   else
#endif
	   nsts = SetListsUser(mAppName, rcp, mRCPDestB, nMLDomDiv, nNoOfDiv); //nMaxDivide); //nMaxThread);
#ifndef V4
#ifdef TRACE_MODE
   if (nSenderLog) {
     sprintf(str, "[%s-%03d]:End SetListsUser:%d\n", rcp->mMNo, nsts);
     printfTrace(str);
   }
#endif
#endif
	 }
     if (!bFound && nsts) {
       j = 0;
//ReadRetry:
       fpsrc = fopen(mMSGSrc, "rt");
       if (fpsrc) {
#ifdef DATA_CASH
         setvbuf(fpsrc, rcp->mFrbuf, _IOFBF, sizeof(rcp->mFrbuf) );
#endif
//@@@@@ #ifndef V4
		 if (bDebug)
   	       printf("RestorMLRCPFile(%s) Read Open Success. \n", mMSGSrc);
#ifdef TRACE_MODE
#ifdef UPDATE_20060904A // ロックファイルの作成確認
if (nSenderLog || nSender2Log)
#else
if (nSenderLog)
#endif
{
  sprintf(str, "[%s-%03d]:RestorMLRCPFile: Read Open Success(%s)\n", rcp->mMNo, n, mMSGSrc);
  printfTrace(str);
}
#endif
//#endif
		 sprintf(mMSGDest, mMSGDestA, 1);
         k = 0;
         /////////////////////////////////////////////////////
         if ((fpdest = fopen(mMSGDest, "wt"))) {
#ifdef DATA_CASH
           setvbuf(fpdest, rcp->mFwbuf, _IOFBF, sizeof(rcp->mFwbuf) );
#endif
///@@@@ #ifndef V4
		   if (bDebug)
   	         printf("RestorMLRCPFile(%s) Write Open Success. \n", mMSGDest);
#ifdef TRACE_MODE
if (nSenderLog) {
  sprintf(str, "[%s-%03d]:RestorMLRCPFile: Write Open Success(%s)\n", rcp->mMNo, n, mMSGDest);
  printfTrace(str);
}
#endif
//#endif
	       p = fgets(mwork, sizeof(mwork), fpsrc);
	       while (p || !feof(fpsrc)) {
		     //if (!bMsg && mwork[0] == '\n') {
			 if (!bMsg && mwork[0] == '\n' && bCR) { // 2000.12.28
  		       bMsg = TRUE;
			   if (bExtendHeader) { // ＭＬ用に拡張ヘッダを挿入する場合：デフォルト
#ifdef UPDATE_20070521 // 予約語対策
				 if (bReservedWords1)
				   pH = strstr(&mwk[strlen(mReservedWords)], "@");
				 else
#endif
				   pH = strstr(mwk, "@");
				 if (pH) 
				   *pH = '\x0'; // 一旦ドメインを消去
#ifdef UPDATE_20070521 // 予約語対策
				 fprintf(fpdest, "Sender: %s-request@%s\n", (bReservedWords1 ? &mwk[strlen(mReservedWords)] : mwk), rcp->mDomain); //rcp->mSmtp);
	             fprintf(fpdest, "Resent-Message-Id: <%s>\n", rcp->mMIDNo);
	             fprintf(fpdest, "Resent-From: %s@%s\n", (bReservedWords1 ? &mwk[strlen(mReservedWords)] : mwk), rcp->mDomain); //rcp->mSmtp);
	             fprintf(fpdest, "X-Unsub: To leave, send text 'LEAVE' to <%s-request@%s>\n", (bReservedWords1 ? &mwk[strlen(mReservedWords)] : mwk), rcp->mDomain); //rcp->mSmtp);
#else
	             fprintf(fpdest, "Sender: %s-request@%s\n", mwk, rcp->mDomain); //rcp->mSmtp);
	             fprintf(fpdest, "Resent-Message-Id: <%s>\n", rcp->mMIDNo);
	             fprintf(fpdest, "Resent-From: %s@%s\n", mwk, rcp->mDomain); //rcp->mSmtp);
	             fprintf(fpdest, "X-Unsub: To leave, send text 'LEAVE' to <%s-request@%s>\n", mwk, rcp->mDomain); //rcp->mSmtp);
#endif
				 if (pH) 
				   *pH = '@'; // 戻す
			   }
	           fputs(mwork, fpdest);
			 } else {
			   if (!(!bMsg &&
				     (strnicmp(mwork, "Sender:", 7) == 0  ||
				      strnicmp(mwork, "Resent-Message-Id:", 18) == 0  ||
				      strnicmp(mwork, "Resent-From:", 12) == 0  ||
				      strnicmp(mwork, "X-Unsub:", 8) == 0) ) )
                 fputs(mwork, fpdest);
			   /// 2000.12.28
			   if (strpbrk(mwork, "\n"))
				 bCR  = TRUE;
			   else
				 bCR  = FALSE;
			   /////////////////////
			 }
	         p = fgets(mwork, sizeof(mwork), fpsrc);
		   }
		   fflush(fpdest);
	       fclose(fpdest);
#ifdef AUTOPROCESS
		   if (mc.Module[0]) {  // アドインプロセス処理
             AutoProcess(&mc, mDir, mMSGDest);
		   }
#endif
           //// listフォルダ内の.$CP->RCPに戻し処理を有効にする。
           while(!(fpdest = fopen(mMSGDest, "rt"))) {
             if (bServiceTerminating)
		       break;
	         _sleep(WAIT_TIME);
		   }
		   fclose(fpdest);
#ifndef NEW_SENDER
           _sleep(WAIT_TIME*100);
#endif
#ifndef V4
		   if (bDebug)
   	         printf("RestorMLRCPFile(%s) Write Open Complete. \n", mMSGDest);
#ifdef TRACE_MODE
if (nSenderLog) {
  sprintf(str, "[%s-%03d]:RestorMLRCPFile: Write Open Complete(%s)\n", rcp->mMNo, n, mMSGDest);
  printfTrace(str);
}
#endif
#endif
		   //// 2003.3.3
		   if (nMLDomDiv == 0) {  // 強制分割で無い場合はドメイン分割
		     i = 0;
		     sprintf(mRCP, "%s\\*.$CP", mDir); 
	         hF = _findfirst( mRCP, &FD); //hF = FindFirstFile( mRCP, &FD);
             if (hF != -1L) { //INVALID_HANDLE_VALUE) {
	           bFile = TRUE;
               while (bFile) {
                if (bServiceTerminating)
	              break;
			    i++;
		        sprintf(mMSGDestB, mMSGDestA, i);
#ifdef TRACE_MODE
		        sprintf(str, "[%s-%03d]:RestorMLRCPFile:Start X_CopyFile(%s, %s)\n", rcp->mMNo, n, mMSGDest, mMSGDestB);
			    if (bDebug)
			      printf("%s", str);
                if (nSenderLog) {
                  printfTrace(str);
				}
#endif
			    X_CopyFile(mMSGDest, mMSGDestB, FALSE);
#ifdef TRACE_MODE
		        sprintf(str, "[%s-%03d]:RestorMLRCPFile:End X_CopyFile(%s, %s)\n", rcp->mMNo, n, mMSGDest, mMSGDestB);
			    if (bDebug)
			      printf("%s", str);
                if (nSenderLog) {
                  printfTrace(str);
				}
#endif
   		        sprintf(mRCPDest,  mRCPDestA, i);
			    sprintf(mRCPDestC, "%s\\%s", mDir, FD.name); //cFileName); 
			    while(!(fmes = fopen(mMSGDestB, "rt"))) {// コピー完了待ち
                  if (bServiceTerminating)
		            break;
			      _sleep(WAIT_TIME);
#ifdef TRACE_MODE
		        sprintf(str, "[%s-%03d]:RestorMLRCPFile:%s wait loop(%s)\n", rcp->mMNo, n, mMSGDestB);
			    if (bDebug)
			      printf("%s", str);
                if (nSenderLog) {
                  printfTrace(str);
				}
#endif
				}
			    fclose(fmes);
 	 		    dErr = X_MoveFile(mRCPDestC, mRCPDest);
#ifndef V4
#ifdef TRACE_MODE
		        sprintf(str, "[%s-%03d]:RestorMLRCPFile:%s MoveFile(%s, %s):dErr=%ld\n", rcp->mMNo, n, (dErr == ERROR_SUCCESS ? "Success" : "Fail"), mRCPDestC, mRCPDest, dErr);
			    if (bDebug)
			      printf("%s", str);
                if (nSenderLog) {
                  printfTrace(str);
				}
#endif
#endif
		        bFile = (_findnext(hF, &FD) == 0 ? TRUE : FALSE); //bFile = FindNextFile( hF, &FD);
			   }; 
              _findclose(hF); //FindClose( hF ); 
			 }
		   } else { // 2003.3.3
		     for (i = 2; i <= nsts; i++) {
		       sprintf(mMSGDestB, mMSGDestA, i);
			   X_CopyFile(mMSGDest, mMSGDestB, FALSE);
   		       sprintf(mRCPDest,  mRCPDestA, i);
   		       sprintf(mRCPDestC, mRCPDestB, i);
			   while(!(fmes = fopen(mMSGDestB, "rt"))) {// コピー完了待ち
                 if (bServiceTerminating)
		           break;
			     _sleep(WAIT_TIME);
			   }
			   fclose(fmes);
 	 		   dErr = X_MoveFile(mRCPDestC, mRCPDest);
#ifndef V4
#ifdef TRACE_MODE
		       sprintf(str, "[%s-%03d]:RestorMLRCPFile:%s MoveFile(%s, %s):dErr=%ld\n", rcp->mMNo, n, (dErr == ERROR_SUCCESS ? "Success" : "Fail"), mRCPDestC, mRCPDest, dErr);
			   if (bDebug)
			     printf("%s", str);
               if (nSenderLog) {
                 printfTrace(str);
			   }
#endif
#endif
			 }
             //// 複写元を.$CP->RCPに戻し処理を有効にする。
   		     sprintf(mRCPDest,  mRCPDestA, 1);
   		     sprintf(mRCPDestC, mRCPDestB, 1);
 	 	     dErr = X_MoveFile(mRCPDestC, mRCPDest);
		     if (dErr != ERROR_SUCCESS)
               _unlink(mMSGDest); //DeleteFile(mMSGDest);
#ifndef V4
#ifdef TRACE_MODE
             sprintf(str, "[%s-%03d]:RestorMLRCPFile:Success MoveFile(%s, %s)\n", rcp->mMNo, n, mRCPDestC, mRCPDest);
		     if (bDebug)
		       printf("%s", str);
             if (nSenderLog]) {
               printfTrace(str);
			 }
#endif
#endif
		   }
           ////////////////////////////////////////////////////
		 } else {
#ifndef V4
		   if (bDebug)
   	         printf("RestorMLRCPFile(%s) Write Open Failed.(%d)\n", mMSGDest, k);
#ifdef TRACE_MODE
           if (nSenderLog) {
             sprintf(str, "[%s-%03d]:RestorMLRCPFile: Write Open Failed. (%d)(%s)\n", rcp->mMNo, n, k, mMSGDest);
             printfTrace(str);
		   }
#endif
#endif
		   fclose(fpsrc);
		   _unlink(mRCPSrc); //DeleteFile(mRCPSrc);
#ifdef UPDATE_20170222 // 送信エラー時の送信アドレス管理ファイルのクローズ抜け
#ifdef REGTOFILE
           CloseHandle(hFile);
#endif
		   if (!bFound)
		     _unlink(mLKFile);
#endif
		   return FALSE;
		   //exit(1);
		 }
	     fclose(fpsrc);
	   } else {
#ifndef V4
		 if (bDebug)
   	       printf("RestorMLRCPFile(%s) Read Open Failed.(%d) \n", mMSGSrc, j);
#ifdef TRACE_MODE
         if (nSenderLog) {
           sprintf(str, "[%s-%03d]:RestorMLRCPFile: Read Open Failed.(%d)(%s)\n", rcp->mMNo, n, j, mMSGSrc);
           printfTrace(str);
		 }
#endif
#endif
		 //return bsts;
	   }
	 } 
#ifdef REGTOFILE
     CloseHandle(hFile);
#endif
	 if (!bFound)
       _unlink(mLKFile); //DeleteFile(mLKFile);
   }

#ifdef TRACE_MODE
#ifdef UPDATE_20060904A // ロックファイルの作成確認
   if (nSenderLog || nSender2Log)
#else
   if (nSenderLog)
#endif
   {
     sprintf(str, "[%s-%03d]:End RestorMLRCPFile:nsts=%d\n", rcp->mMNo, n, nsts);
     printfTrace(str);
   }
#endif
   return (nsts ? TRUE : (bFound ? TRUE : FALSE));
}

void RestorRCPFile(struct _RCP *rcp, char *mRCPSrc, char *mRCPDest) {
  FILE *fp;
  char *fsts;

  fp = fopen(mRCPSrc, "rt");
  if (fp) {
#ifdef DATA_CASH
     setvbuf(fp, rcp->mFrbuf, _IOFBF, sizeof(rcp->mFrbuf) );
#endif
	fgets(rcp->mMID, sizeof(rcp->mMID), fp);
	fgets(rcp->mFrom, sizeof(rcp->mFrom), fp);
    rcp->mTo[0] = '\x0';
    fsts = fgets(rcp->mTo, sizeof(rcp->mTo), fp);
    while (fsts || !feof(fp)) {
	  if (rcp->mTo[0] != '\x0')
	    SetRCPFile(rcp, mRCPDest, FALSE, NULL);
	  rcp->mTo[0] = '\x0';
      fsts = fgets(rcp->mTo, sizeof(rcp->mTo), fp);
	}
    fclose(fp);
  }
}

DWORD GetTryServer(char *mDomain, char *mMsg, DWORD *nErrCode, BOOL bmode) {
#ifdef TRACE_MODE
  char str[LOG_BUFFER];
#endif
  char mDir[256], mMail[256], mcnt[24];
  char mNo[256], *pNo;
  FILE *fp;
  struct _MRI mri;
  DWORD  i = 0;
#ifdef Y2038_BUG
   SYSTEMTIME lt;
   FILETIME   ltime64, otime64;
   __int64    ltime, otime = 0;
   HANDLE hFile;
   ULARGE_INTEGER *u1, *u2;
   DWORD      ld = 0;
#else
  time_t ltime, otime = 0;
  int fh;
  struct _stat buf;
#endif
  //BOOL bsts = TRUE;
  //// リトライ回数から、MXレコード優先か@ドメインを優先して接続するか交互に選択。
  //sprintf(mDir, "%sholding", mMailSpoolDir);
#ifdef Y2038_BUG
  GetSystemTime(&lt);
  SystemTimeToFileTime(&lt, &ltime64);
  u1 = (ULARGE_INTEGER *)&ltime64;
  ltime = (__int64)(u1->QuadPart/10000000);
  //ltime = (__int64)(u1->QuadPart/10000);
#else
  time( &ltime );
#endif
  /////////////////////////////////////////
  strcpy(mNo, mMsg);
  ///////////////////////////// 2001.12.30
  //strtok(mNo, ".");
  pNo = strrchr(mNo, '.');
  if (pNo)
    *pNo = '\x0';
  /////////////////////////////////////////
  pNo = strrchr(mNo, '\\');
  if (pNo)
	pNo++;
  else
	pNo = mNo;
  /////////////////////////////////////////
#ifdef UPDATE_20080213 // ドメイン無しアドレスの外部転送で転送できない場合にbaddomainフォルダ内のメールがリトライし続ける
  if (mDomain[0] != '\x0') {
    sprintf(mDir, "%sdomains\\%s-%s", mMailSpoolDir, mDomain, pNo);
  } else if (bMailForward) { // ドメインが空白で全受信メールを転送する場合
    sprintf(mDir, "%sdomains\\baddomain-%s", mMailSpoolDir, pNo);
  } else {
	return 0;
  }
#else
  if (mDomain[0] != '\x0')
    sprintf(mDir, "%sdomains\\%s-%s", mMailSpoolDir, mDomain, pNo);
  else
	return 0;
#endif
    //sprintf(mDir, "%sdomains\\localhost", mMailSpoolDir);
  sprintf(mMail,"%s\\Domain.mri", mDir);
  if (nErrCode)
    *nErrCode = 0;
 ///////////////////////////////////////////////////
#ifdef TRACE_MODE
    if (nSenderLog || nSender2Log) {
 	  sprintf(str, "[%s] [%s] GetTryServer:()\n", pNo, mDomain);
      printfTrace(str);
	}
#endif
 ///////////////////////////////////////////////////
      fp = fopen(mMail, "rt");
      if (fp) {
        fgets(mri.mDom, sizeof(mri.mDom), fp);
        fgets(mri.mLastTry, sizeof(mri.mLastTry), fp);
#ifdef Y2038_BUG
        otime = _atoi64(&mri.mLastTry[12]);
#else
        otime = atol(&mri.mLastTry[12]);
#endif
        fgets(mri.mTryCount, sizeof(mri.mTryCount), fp);
        fgets(mri.mErrCode, sizeof(mri.mErrCode), fp);
        strcpy(mcnt, mri.mTryCount);
        strword(mcnt, " ", "\n");
        i = atoi(mcnt);
#ifndef UPDATE_20060621 // リトライ間隔のままのリトライがスレッドが生きている間リトライされ続けてしまう対策
        if (bmode)
          sprintf(mri.mTryCount,"Try-number: %d\n", ++i);
#endif
        if (nErrCode) {
          strcpy(mcnt, mri.mErrCode);
          strword(mcnt, " ", "\n");
          *nErrCode = atoi(mcnt);
		}
        fclose(fp);
#ifdef UPDATE_20060624 // holding内のMSGファイルもチェック
	  } else {
#ifdef TRACE_MODE
#ifdef UPDATE_20060904A // ロックファイルの作成確認
       if (nSenderLog || nSender2Log)
#else
       if (nSenderLog)
#endif
	   {
 	      sprintf(str, "[%s] GetTryServer(): %s not found\n", pNo, mMail);
          printfTrace(str);
		}
		//if (strstr(pNo, "-R"))
		  //_exit(0);
#endif
#endif
	  }
  ///////////////////////////////////////////////////
  if (mMsg) { // MSGファイルの作成日付取得
	if (strlen(mMsg)) {
#ifdef Y2038_BUG
   hFile = CreateFile( (LPCTSTR)mMsg,
                        GENERIC_READ,
                        0,
                        NULL,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL, 
                        NULL);
   if (hFile != INVALID_HANDLE_VALUE) {
     //GetFileTime(hFile, &otime64, NULL, NULL);
	 GetFileTime(hFile, NULL, NULL, &otime64);
     CloseHandle(hFile);
   }
   u2 = (ULARGE_INTEGER *)&otime64;
   otime = (__int64)(u2->QuadPart/10000000);
   //otime = (__int64)(u2->QuadPart/10000);
   if (otime > ltime)
	 otime = ltime;
   ld = (DWORD)((ltime-otime)/(nChkTM ? nChkTM : 1));
#else
      fh = _open( mMsg, _O_RDONLY);
      if( fh != -1 ) {
        if ( _fstat( fh, &buf ) == 0 )
          otime = buf.st_mtime; //.st_ctime;
        _close( fh );
	  }
      if (otime > ltime)
	    otime = ltime;
#endif
	}
  }
  ///////////////////////////////////////////////////
  if (bmode) {
#ifdef TRACE_MODE
    if (nSenderLog || nSender2Log) {
#ifdef Y2038_BUG
 	  sprintf(str, "[%s] [%s] GetTryServer:(ltime(%I64u)-otime(%I64u))/%d=(%I64u)\n", pNo, mDomain, ltime, otime, nSendMaxRetry, (ltime-otime)/(nChkTM ? nChkTM : 1) );
#else
      sprintf(str, "[%s] [%s] GetTryServer:(ltime(%u)-otime(%u))/%d=(%u)\n", pNo, mDomain, ltime, otime, nSendMaxRetry, (ltime-otime)/(nChkTM ? nChkTM : 1));
#endif
      printfTrace(str);
	}
#endif
//	return  min((otime ? (DWORD)((ltime-otime)/nSendMaxRetry) : otime), i) ;
    bMRICounter = FALSE;
#ifdef Y2038_BUG
	return  min((otime ? ld : (DWORD)0), i) ;
#else
	return  min((otime ? (DWORD)((ltime-otime)/(nChkTM ? nChkTM : 1)) : (DWORD)0), i) ;
#endif
  } else {
#ifdef TRACE_MODE
    if (nSenderLog) {
      sprintf(str, "[%s] [%s] GetTryServer:i=%d\n", pNo, mDomain, i);
      printfTrace(str);
	}
#endif
    return  i;
  }
}

#ifdef UPDATE_20060621 // リトライ間隔のままのリトライがスレッドが生きていり間リトライされ続けてしまう対策
void Inc_MRI_TryCount2(char *pNo, char *pMRI) {
#ifdef TRACE_MODE
  char str[LOG_BUFFER];
#endif
  char mcnt[24];
  FILE *fp;
  struct _MRI mri;
  int i;

  if ((fp = fopen(pMRI, "rt"))) {
	fgets(mri.mDom, sizeof(mri.mDom), fp);
	fgets(mri.mLastTry, sizeof(mri.mLastTry), fp);
	fgets(mri.mTryCount, sizeof(mri.mTryCount), fp);
	strcpy(mcnt, mri.mTryCount);
    strword(mcnt, " ", "\n");
    fgets(mri.mErrCode, sizeof(mri.mErrCode), fp);
    fclose(fp);
    i = atoi(mcnt) + 1;
	sprintf(mri.mTryCount,"Try-number: %d\n", i);
#ifdef TRACE_MODE
    if (nSenderLog) { //20070320test
      sprintf(str, "[%s] Inc_MRI_TryCount2():%s\n", pNo, mri.mTryCount);
      printfTrace(str);
	}
#endif
    if ((fp = fopen(pMRI, "wt"))) {
	  fprintf(fp, "%s", mri.mDom);
	  fprintf(fp, "%s", mri.mLastTry);
	  fprintf(fp, "%s", mri.mTryCount);
	  fprintf(fp, "%s", mri.mErrCode);
      fflush(fp);
	  fclose(fp);
	}
#ifdef UPDATE_20070320A  // Domain.mriの再書込みで実体化せずハングする
    while(!(fp = fopen(pMRI, "rt"))) { 
      if (bServiceTerminating)
       break;
       _sleep(WAIT_TIME);
	}
    fclose(fp);
#endif
  }
}
#endif

void Inc_MRI_TryCount(char *mID, char *mDomain) {
#ifdef TRACE_MODE
  char str[LOG_BUFFER];
#endif
  char mMRI[256], mcnt[24], mNo[256], *pNo;
  FILE *fp;
  struct _MRI mri;
  int i;

  strcpy(mNo, mDomain);
  if (mNo[strlen(mNo)-1] == '\\')
	mNo[strlen(mNo)-1] = '\x0';
  pNo = strrchr(mNo, '\\');
  if (pNo)
	pNo++;
  else
	pNo = mNo;
  /////////////////////////////////////////
  sprintf(mMRI,"%s\\Domain.mri", mDomain);
  fp = fopen(mMRI, "rt");
  if (fp) {
	fgets(mri.mDom, sizeof(mri.mDom), fp);
	fgets(mri.mLastTry, sizeof(mri.mLastTry), fp);
	fgets(mri.mTryCount, sizeof(mri.mTryCount), fp);
	strcpy(mcnt, mri.mTryCount);
    strword(mcnt, " ", "\n");
    fgets(mri.mErrCode, sizeof(mri.mErrCode), fp);
    fclose(fp);
    if (bMRICounter)
  	  i = atoi(mcnt);
	else
      i = atoi(mcnt) + 1;
	sprintf(mri.mTryCount,"Try-number: %d\n", i);
#ifdef TRACE_MODE
if (nSenderLog) {
  sprintf(str, "[%s] Inc_MRI_TryCount():Try-number: %d -> %s\n", pNo, i, mDomain);
  printfTrace(str);
}
#endif
      if ((fp = fopen(mMRI, "wt"))) {
	    fprintf(fp, "%s", mri.mDom);
	    fprintf(fp, "%s", mri.mLastTry);
	    fprintf(fp, "%s", mri.mTryCount);
	    fprintf(fp, "%s", mri.mErrCode);
        fflush(fp);
	    fclose(fp);
	  }
	}
}

BOOL BadCharcter(char *src) {
  int i, j;
  char mBadChar[12] = "\\/:,;*?\"<>|"; // 使用不可のキャラクタ

  //// ドメインフォルダ作成に有効なキャラクタであるかチェック
  for (i = 0; i < (int)strlen(src); i++) {
	for (j = 0; j < (int)strlen(mBadChar); j++) {
	  if (*(src+i) == mBadChar[j]) {
	    return FALSE;
	  }
	}
  }
  return TRUE;
}

void SetRetry(struct _RCP *rcp, BOOL bCount, DWORD nErrCode, int nCC) {
#ifdef TRACE_MODE
  char str[LOG_BUFFER];
#endif
#ifdef UPDATE_20071205 // メールアドレス長が255Byteのときの対策
  char mDir[512], mMail[512], mNo[512], mpNo[512];
#else
  char mDir[256], mMail[256], mNo[256], mpNo[256];
#endif
#ifdef UPDATE_20071205A // リトライキュー保管時にドメイン名が32バイト以上の場合MD5のハッシュ値に変換したフォルダ名にする
  char mDomDir[128];
#endif
#ifdef UPDATE_20050122
  char mMRCP[256];
#endif
  char mBadChar[12] = "\\/:,;*?\"<>|";
  char *pNo, *p;
  FILE *fp;
  struct _MRI mri;
#ifdef Y2038_BUG
   SYSTEMTIME lt;
   FILETIME   ltime64;
   DWORD      ltime;
   ULARGE_INTEGER *u1;
#else
  time_t ltime, otime = 0;
#endif
  BOOL   bGoodDomain = TRUE;
  HANDLE             hF;

#ifdef UPDATE_20050122
  while (bThreadDomain) {
    if (bServiceTerminating)
      return;
	_sleep(WAIT_TIME);
  }
  bThreadDomain = TRUE;
#endif
  /////////////////////////////////////////
  strcpy(mNo, rcp->mMsg);
  ///////////////////////////// 2001.12.30
  //strtok(mNo, ".");
  p = strrchr(mNo, '.');
  if (p)
	*p = '\x0';
  /////////////////////////////////////////
  pNo = strrchr(mNo, '\\');
  if (pNo)
	pNo++;
  else
	pNo = mNo;
  /////////////////////////////////////////
  sprintf(mDir, "%sholding", mMailSpoolDir);
  /////////////////////////////////////////////////////
  // 初めてのリトライ対象であれば番号追加
  //GetSystemTime(&lt);
  //SystemTimeToFileTime(&lt, &ltime64);
#ifdef UPDATE_20061129 // 同報処理内での転送エラーメール発生時にエラーメールのファイル名が重複する不具合。
  if (rcp->mSetRetryMode[0]) {
    sprintf(mpNo, "%s%s", pNo, rcp->mSetRetryMode);
  } else
#endif
  strcpy(mpNo, pNo);
  if (!strstr(pNo, "-R")) {
#ifdef UPDATE_20061129 // 同報処理内での転送エラーメール発生時にエラーメールのファイル名が重複する不具合。
	if (rcp->mSetRetryMode[0]) {
      sprintf(mpNo, "%s%s-R%04d", pNo, rcp->mSetRetryMode, nCC);
	} else
#endif
    sprintf(mpNo, "%s-R%04d", pNo, nCC);
  }
	//sprintf(mpNo, "%s-R%lu", pNo, ltime64.dwLowDateTime+ltime64.dwHighDateTime);
  /////////////////////////////////////////////////////
  sprintf(mMail,"%s\\%s.MSG", mDir, mpNo);
  /////////////////////////////////////////////////////
  if (bDebug)
    printf("SetRetry:%s->%s\n", rcp->mMsg, mMail);
#ifdef TRACE_MODE
if (nSenderLog) { // 20070320test
  sprintf(str, "[%s] SetRetry:%s->%s\n", pNo, rcp->mMsg, mMail);
  printfTrace(str);
}
#endif
  if (stricmp(rcp->mMsg, mMail) != 0) {
	if (X_CopyFile(rcp->mMsg, mMail, TRUE)) {
	  while(!(fp = fopen(mMail, "rt"))) {
		if (bServiceTerminating) {
#ifdef UPDATE_20050122
          bThreadDomain = FALSE;
#endif
	      return;
		}
		_sleep(WAIT_TIME);
	  }
	  fclose(fp);
	  if (bDebug)
	    printf("Copy File Success. SetRetry(1) %s -> %s\n", rcp->mMsg, mMail);
	} else {
	  if (bDebug)
	    printf("Copy File Fail.  SetRetry(1) %s -> %s\n", rcp->mMsg, mMail);
	}
	/////////////////////////////////
#ifdef UPDATE_20080213 // ドメイン無しアドレスの外部転送で転送できない場合にbaddomainフォルダ内のメールがリトライし続ける
    sprintf(mDir, "%sdomains\\baddomain-%s", mMailSpoolDir, mpNo);
#else
	sprintf(mDir, "%sdomains\\baddomain", mMailSpoolDir);
#endif
	if (rcp->mDomain[0] != '\x0')
		if (BadCharcter(rcp->mDomain))   //// ドメインフォルダ作成に有効なキャラクタであるかチェック
		{
#ifdef UPDATE_20071205A // リトライキュー保管時にドメイン名が32バイト以上の場合MD5のハッシュ値に変換したフォルダ名にする
		  if (strlen(rcp->mDomain)>64) {
		    hmac_md5(rcp->mDomain,"0",mDomDir);
		  } else {
		    strcpy(mDomDir, rcp->mDomain);
		  }
          sprintf(mDir, "%sdomains\\%s-%s", mMailSpoolDir, mDomDir, mpNo);
#else
          sprintf(mDir, "%sdomains\\%s-%s", mMailSpoolDir, rcp->mDomain, mpNo);
#endif
		}
	/////////////////////////////////
	_mkdir(mDir);
#ifdef UPDATE_20050122
    sprintf(mMail,"%s\\%s.$$P", mDir, mpNo);
    sprintf(mMRCP,"%s\\%s.RCP", mDir, mpNo);
    /////////////////////////////////////////
	//// RCPファイルが存在するなら削除
    if ((fp = fopen(mMRCP, "rt"))) {
	  fclose(fp);
	  _unlink(mMRCP);
	}
#else
    sprintf(mMail,"%s\\%s.RCP", mDir, mpNo);
#endif
	if (bDebug)
      printf("SetRetry:%s->%s\n", rcp->mRcp, mMail);
	if (!SetRCPFile(rcp, mMail, TRUE, NULL)) { // RCPファイルが空なら
#ifndef UPDATE_20050122
      // 何もしない
	  return;
#endif
	}
#ifdef UPDATE_20050122
    while(!(fp = fopen(mMail, "rt"))) {
  	  if (bServiceTerminating) {
        bThreadDomain = FALSE;
	    return;
	  }
	  _sleep(WAIT_TIME);
	}
	fclose(fp);
	X_MoveFile(mMail, mMRCP);
#endif
  }
#ifdef Y2038_BUG
  GetSystemTime(&lt);
  SystemTimeToFileTime(&lt, &ltime64);
  u1 = (ULARGE_INTEGER *)&ltime64;
  ltime = (DWORD)(u1->QuadPart/10000000);
#else
  time( &ltime );
#endif
  sprintf(mMail,"%s\\Domain.mri", mDir);
  fp = fopen(mMail, "rt");
  if (fp) {
#ifdef DATA_CASH
    setvbuf(fp, rcp->mFrbuf, _IOFBF, sizeof(rcp->mFrbuf) );
#endif
	fgets(mri.mDom, sizeof(mri.mDom), fp);
	fgets(mri.mLastTry, sizeof(mri.mLastTry), fp);
	fgets(mri.mTryCount, sizeof(mri.mTryCount), fp);
	fclose(fp);
  } else {
	sprintf(mri.mDom, "Domain-name: %s\n", rcp->mSmtp); //rcp->mDomain);
	sprintf(mri.mLastTry, "Last-tried: %u\n", ltime);
	sprintf(mri.mTryCount,"Try-number: %d\n", 0); //(bCount ? 1 : 0));
  }
  sprintf(mri.mErrCode,"Err-code: %d\n", nErrCode); // 前回エラーコード
#ifdef TRACE_MODE
if (nSenderLog) {
  sprintf(str, "[%s] SetRetry():Try-number: %d -> %s\n", pNo, atoi(mri.mTryCount), mDir);
  printfTrace(str);
}
#endif
      if ((fp = fopen(mMail, "wt"))) {
	    fprintf(fp, "%s", mri.mDom);
	    fprintf(fp, "%s", mri.mLastTry);
	    fprintf(fp, "%s", mri.mTryCount);
	    fprintf(fp, "%s", mri.mErrCode);
        fflush(fp);
	    fclose(fp);
	  }
#ifdef UPDATE_20050122
    bThreadDomain = FALSE;
#endif
}

void outlocallog(struct _RCP *rcp) {
#ifdef TRACE_MODE
   char str[LOG_BUFFER];
#endif
#ifdef Y2038_BUG
	SYSTEMTIME ltime, lt;
#else
	time_t ltime;
    struct tm lt;
#endif
	CHAR   mtime[256];
	char   mdata[128], mLogFn[256];
	FILE   *Logfp;
        
	///// outlog //////////////////////
	if (bSendLocalLog) {
#ifdef NEW_SENDER
      while(bOutLocallog) {
        if (bServiceTerminating)
	      return;
	    _sleep(WAIT_TIME);
	  }
      bOutLocallog = TRUE;
#ifdef TRACE_MODE
      sprintf(str, "[%s] outlocallog() thread=%d.\n", rcp->mMNo, nRunThread);
	  if (bDebug)
        printf("%s", str);
      if (nSenderLog) {
        printfTrace(str);
	  }
#endif
#endif
      gettime(&ltime, mtime);
#ifdef Y2038_BUG
	  _tzset();
	  SystemTimeToTzSpecificLocalTime(NULL, &ltime, &lt);
	  sprintf(mdata, "%02d%02d%02d", (lt.wYear%100), lt.wMonth, lt.wDay );
#else
      lt = *localtime(&ltime);
      strftime( mdata, sizeof(mdata), "%y%m%d", &lt );
#endif
#ifdef UPDATE_20260606 // ログの書き込み信頼性向上対策
      WaitForSingleObject(g_hMutexOutLocalLog, INFINITE);  // 排他開始
      fpOutLocalLog = OpenCloseLog(fpOutLocalLog, mDTOutLocalLog, "outlocallog", mComName, lt);
#endif
#ifdef Y2038_BUG
      sprintf(mdata, "%02d/%s/%04d:%02d:%02d:%02d",lt.wDay, mMonth[lt.wMonth-1], lt.wYear, lt.wHour, lt.wMinute, lt.wSecond);
#else
      strftime( mdata, sizeof(mdata), "%d/%b/%Y:%H:%M:%S", &lt );
#endif
#ifdef UPDATE_20260606 // ログの書き込み信頼性向上対策
	  if (fpOutLocalLog) {
		fprintf(fpOutLocalLog, "%s sent on [%s] to %s %s from %s to %s\n",
                     rcp->mMIDNo,
                     mdata,
                     rcp->mSmtpIP,
                     rcp->mSmtp,
                     ((rcp->mFrom[0] == '\x0' || rcp->mFrom[0] == '\r' || rcp->mFrom[0] == '\n') ? "-" : rcp->mFrom),
                     //rcp->mFrom,
                     rcp->mTo);
#ifdef UPDATE_20060615 // データのディスク書込に異常が有った場合の対策
		 if (fflush(fpOutLocalLog) == EOF)  // 書込み異常はイベントビューワに
		   if (bVLog)
             AddToMessageLog(TEXT("outlocallog write fail."), 103, TEXT(SMTP_SERVICE), EVENTLOG_WARNING_TYPE );
#endif
	  }
#else
#ifdef REGTOFILE
      if (nClustering) {
	    sprintf(mLogFn, "%soutlocallog\\%s\\%s.log", mMailSpoolDir, mComName, mdata);
	  } else {
#endif
	  sprintf(mLogFn, "%soutlocallog\\%s.log", mMailSpoolDir, mdata);
#ifdef REGTOFILE
	  }
#endif
#ifdef UPDATE_20260603 // ログ出力の排他処理化を追加
      WaitForSingleObject(g_hMutexOutLocalLog, INFINITE);  // 排他開始
#endif
	  Logfp = fopen(mLogFn, "at");
	  if (Logfp) {
		fprintf(Logfp, "%s sent on [%s] to %s %s from %s to %s\n",
                     rcp->mMIDNo,
                     mdata,
                     rcp->mSmtpIP,
                     rcp->mSmtp,
                     ((rcp->mFrom[0] == '\x0' || rcp->mFrom[0] == '\r' || rcp->mFrom[0] == '\n') ? "-" : rcp->mFrom),
                     //rcp->mFrom,
                     rcp->mTo);
#ifdef UPDATE_20060615 // データのディスク書込に異常が有った場合の対策
		 if (fflush(Logfp) == EOF)  // 書込み異常はイベントビューワに
		   if (bVLog)
             AddToMessageLog(TEXT("outlocallog write fail."), 103, TEXT(SMTP_SERVICE), EVENTLOG_WARNING_TYPE );
#endif
		 fclose(Logfp);
	  }
#endif
#ifdef UPDATE_20260603 // ログ出力の排他処理化を追加
      ReleaseMutex(g_hMutexOutLocalLog);  // 排他終了
#endif
	  bOutLocallog = FALSE;
	}
}

void outlog(struct _RCP *rcp) {
#ifdef TRACE_MODE
   char str[LOG_BUFFER];
#endif
#ifdef Y2038_BUG
	SYSTEMTIME ltime, lt;
#else
	time_t ltime;
    struct tm lt;
#endif
	CHAR   mtime[256];
	char   mdata[128], mLogFn[256];
	FILE   *Logfp;
        
	///// outlog //////////////////////
	if (bLog) {
#ifdef NEW_SENDER
      while(bOutlog) {
        if (bServiceTerminating)
	      return;
	    _sleep(WAIT_TIME);
	  }
      bOutlog = TRUE;
#ifdef TRACE_MODE
      sprintf(str, "[%s] outlog() thread=%d.\n", rcp->mMNo, nRunThread);
	  if (bDebug)
        printf("%s", str);
      if (nSenderLog) {
        printfTrace(str);
	  }
#endif
#endif
      gettime(&ltime, mtime);
#ifdef Y2038_BUG
	  _tzset();
	  SystemTimeToTzSpecificLocalTime(NULL, &ltime, &lt);
	  sprintf(mdata, "%02d%02d%02d", (lt.wYear%100), lt.wMonth, lt.wDay );
#else
      lt = *localtime(&ltime);
      strftime( mdata, sizeof(mdata), "%y%m%d", &lt );
#endif
#ifdef UPDATE_20260606 // ログの書き込み信頼性向上対策
      WaitForSingleObject(g_hMutexOutLog, INFINITE);  // 排他開始
      fpOutLog = OpenCloseLog(fpOutLog, mDTOutLog, "outlog", mComName, lt);
#endif
#ifdef Y2038_BUG
      sprintf(mdata, "%02d/%s/%04d:%02d:%02d:%02d",lt.wDay, mMonth[lt.wMonth-1], lt.wYear, lt.wHour, lt.wMinute, lt.wSecond);
#else
      strftime( mdata, sizeof(mdata), "%d/%b/%Y:%H:%M:%S", &lt );
#endif
#ifdef UPDATE_20260606 // ログの書き込み信頼性向上対策
	  if (fpOutLog) {
		fprintf(fpOutLog, "%s sent on [%s] to %s %s from %s to %s\n",
                     rcp->mMIDNo,
                     mdata,
                     rcp->mSmtpIP,
                     rcp->mSmtp,
                     ((rcp->mFrom[0] == '\x0' || rcp->mFrom[0] == '\r' || rcp->mFrom[0] == '\n') ? "-" : rcp->mFrom),
                     //rcp->mFrom,
                     rcp->mLogTo); //rcp->mTo);
#ifdef UPDATE_20060615 // データのディスク書込に異常が有った場合の対策
		 if (fflush(fpOutLog) == EOF)  // 書込み異常はイベントビューワに
		   if (bVLog)
             AddToMessageLog(TEXT("outllog write fail."), 104, TEXT(SMTP_SERVICE), EVENTLOG_WARNING_TYPE );
#endif
	  }
#else
#ifdef REGTOFILE
      if (nClustering) {
	    sprintf(mLogFn, "%soutlog\\%s\\%s.log", mMailSpoolDir, mComName, mdata);
	  } else {
#endif
	  sprintf(mLogFn, "%soutlog\\%s.log", mMailSpoolDir, mdata);
#ifdef REGTOFILE
	  }
#endif
#ifdef UPDATE_20260603 // ログ出力の排他処理化を追加
      WaitForSingleObject(g_hMutexOutLog, INFINITE);  // 排他開始
#endif
	  Logfp = fopen(mLogFn, "at");
	  if (Logfp) {
		fprintf(Logfp, "%s sent on [%s] to %s %s from %s to %s\n",
                     rcp->mMIDNo,
                     mdata,
                     rcp->mSmtpIP,
                     rcp->mSmtp,
                     ((rcp->mFrom[0] == '\x0' || rcp->mFrom[0] == '\r' || rcp->mFrom[0] == '\n') ? "-" : rcp->mFrom),
                     //rcp->mFrom,
                     rcp->mLogTo); //rcp->mTo);
#ifdef UPDATE_20060615 // データのディスク書込に異常が有った場合の対策
		 if (fflush(Logfp) == EOF)  // 書込み異常はイベントビューワに
		   if (bVLog)
             AddToMessageLog(TEXT("outllog write fail."), 104, TEXT(SMTP_SERVICE), EVENTLOG_WARNING_TYPE );
#endif
		 fclose(Logfp);
	  }
#endif
#ifdef UPDATE_20260603 // ログ出力の排他処理化を追加
      ReleaseMutex(g_hMutexOutLog);  // 排他終了
#endif
	  bOutlog = FALSE;
	}
}

void faillog(struct _RCP *rcp) {
#ifdef TRACE_MODE
   char str[LOG_BUFFER];
#endif
#ifdef Y2038_BUG
	SYSTEMTIME ltime, lt;
#else
	time_t ltime;
    struct tm lt;
#endif
	CHAR   mtime[256];
	char   mdata[128], mLogFn[256];
	FILE   *Logfp;
	int    sts;
        
	///// inlog //////////////////////
	if (bFailLog) {
#ifdef NEW_SENDER
      while(bFailOutlog) {
        if (bServiceTerminating)
	      return;
	    _sleep(WAIT_TIME);
	  }
      bFailOutlog = TRUE;
#ifdef TRACE_MODE
      sprintf(str, "[%s] faillog() thread=%d.\n", rcp->mMNo, nRunThread);
	  if (bDebug)
        printf("%s", str);
      if (nSenderLog) {
        printfTrace(str);
	  }
#endif
#endif
      gettime(&ltime, mtime);
#ifdef Y2038_BUG
	  _tzset();
	  SystemTimeToTzSpecificLocalTime(NULL, &ltime, &lt);
	  sprintf(mdata, "%02d%02d%02d", (lt.wYear%100), lt.wMonth, lt.wDay );
#else
      lt = *localtime(&ltime);
      strftime( mdata, sizeof(mdata), "%y%m%d", &lt );
#endif
#ifdef UPDATE_20260606 // ログの書き込み信頼性向上対策
     WaitForSingleObject(g_hMutexOutFailLog, INFINITE);  // 排他開始
     fpOutFailLog = OpenCloseLog(fpOutFailLog, mDTOutFailLog, "faillog", mComName, lt);
#endif
#ifdef Y2038_BUG
      sprintf(mdata, "%02d/%s/%04d:%02d:%02d:%02d",lt.wDay, mMonth[lt.wMonth-1], lt.wYear, lt.wHour, lt.wMinute, lt.wSecond);
#else
      strftime( mdata, sizeof(mdata), "%d/%b/%Y:%H:%M:%S", &lt );
#endif
#ifdef UPDATE_20260606 // ログの書き込み信頼性向上対策
	  if (fpOutFailLog) {
#ifdef UPDATE_20070907 // faillogにエラーコード記載
		 if (rcp->mAnswer[0] == '\r' || rcp->mAnswer[0] == '\n')
		   rcp->mAnswer[0] = '\x0';
		 else
		   strtok(rcp->mAnswer, "\r\n");
		 fprintf(fpOutFailLog, "%s send fail on [%s] from %s to %s [%s]\n",
                     rcp->mMIDNo,
                     mdata,
					 rcp->mFrom,
                     rcp->mTo,
                     rcp->mAnswer);
#else
		 fprintf(fpOutFailLog, "%s send fail on [%s] from %s to %s\n",
                     rcp->mMIDNo,
                     mdata,
					 rcp->mFrom,
                     rcp->mTo,
                     rcp->mLogTo);
#endif
#ifdef UPDATE_20060615 // データのディスク書込に異常が有った場合の対策
		 if (fflush(fpOutFailLog) == EOF) // 書込み異常はイベントビューワに
		   if (bVLog)
             AddToMessageLog(TEXT("faillog write fail."), 105, TEXT(SMTP_SERVICE), EVENTLOG_WARNING_TYPE );
#endif
	  }
#else
#ifdef REGTOFILE
      if (nClustering) {
	    sprintf(mLogFn, "%sfaillog\\%s\\%s.log", mMailSpoolDir, mComName, mdata);
	  } else {
#endif
	  sprintf(mLogFn, "%sfaillog\\%s.log", mMailSpoolDir, mdata);
#ifdef REGTOFILE
	  }
#endif
#ifdef UPDATE_20260603 // ログ出力の排他処理化を追加
     WaitForSingleObject(g_hMutexOutFailLog, INFINITE);  // 排他開始
#endif
	  Logfp = fopen(mLogFn, "at");
	  if (Logfp) {
#ifdef UPDATE_20070907 // faillogにエラーコード記載
		 if (rcp->mAnswer[0] == '\r' || rcp->mAnswer[0] == '\n')
		   rcp->mAnswer[0] = '\x0';
		 else
		   strtok(rcp->mAnswer, "\r\n");
		 fprintf(Logfp, "%s send fail on [%s] from %s to %s [%s]\n",
                     rcp->mMIDNo,
                     mdata,
					 rcp->mFrom,
                     rcp->mTo,
                     rcp->mAnswer);
#else
		 fprintf(Logfp, "%s send fail on [%s] from %s to %s\n",
                     rcp->mMIDNo,
                     mdata,
					 rcp->mFrom,
                     rcp->mTo,
                     rcp->mLogTo);
#endif
#ifdef UPDATE_20060615 // データのディスク書込に異常が有った場合の対策
		 if (fflush(Logfp) == EOF) // 書込み異常はイベントビューワに
		   if (bVLog)
             AddToMessageLog(TEXT("faillog write fail."), 105, TEXT(SMTP_SERVICE), EVENTLOG_WARNING_TYPE );
#endif
		 fclose(Logfp);
	  }
#endif
#ifdef UPDATE_20260603 // ログ出力の排他処理化を追加
      ReleaseMutex(g_hMutexOutFailLog);  // 排他終了
#endif
	  bFailOutlog = FALSE;
	}
}

BOOL CheckLocalServer(char *mDomain) {
  BOOL sts = FALSE;
  char *pldom, *pd;
  int  i, j;
  PHOSTENT host;
  struct in_addr *inDom, *inlDom;
#ifdef IPv6
  char mDom[INET6_ADDRSTRLEN], mlDom[INET6_ADDRSTRLEN];
#else
  char mDom[16], mlDom[16];
#endif
  char *pDom, *plDom;
  DWORD naliases;
#ifdef IPv6
  //struct in6_addr inDom6, inlDom6;
  char Out6[INET6_ADDRSTRLEN];
  int  Error;
#endif
#ifndef VC6
  int     n;
  HOSTENT hst;
  struct addrinfo Hints, *AddrInfo, *AI;
#endif

  if (*mDomain == '\x0')
	return TRUE;

  pldom = mLocalDomain;
  while (strlen(pldom)){
	if (bDebug)
      printf("domain checked \"%s\" vs \"%s\"\n", mDomain, pldom);
	if (stricmp(mDomain, pldom) == 0) {
      sts = TRUE;
      break;
	}
	if (bCheckLocalDomainIP) {
	  i = strlen(pldom);
	  j = strlen(mDomain);
	  if (i <= j) {
	    pd = (char *)(mDomain + (j-i));
	    if (stricmp(pd, pldom) == 0) {
#ifdef IPv6
          Error = 0;
#ifdef VC6
          host = getipnodebyname(pldom, (nAddressFamily ? AF_INET6 : AF_INET),  (nAddressFamily ? AI_ALL : 0), &Error);
#else
		  host = NULL;
          memset(&Hints, 0, sizeof(Hints));
		  Hints.ai_flags = AI_NUMERICHOST | AI_PASSIVE;
          Hints.ai_family =  (nAddressFamily ? AF_INET6 : AF_INET);
          Hints.ai_socktype = SOCK_STREAM;
		  if (getaddrinfo(pldom, NULL, &Hints, &AddrInfo) == 0) {
#ifdef UPDATE_20181004 // ドメイン名からの順引きが成功したときのアドレス取得でハングすることがあった。
            hst.h_addr_list = (struct sockaddr_in *)(AddrInfo->ai_addr);
#else
            hst.h_addr = (struct sockaddr_in *)(AddrInfo->ai_addr);
#endif
			host = &hst;
          }
#endif
#else
          host = gethostbyname(pldom);
#endif
   	      mlDom[0] = '\x0';
	      inlDom = NULL;
	      if (host) {
            inlDom = (struct in_addr *)host->h_addr; //h_addr_list[0];
		    if (inlDom) {
              //strcpy(mlDom, inet_ntoa(*inlDom));
#ifdef IPv6
		      if (nAddressFamily) {
                (const char *)plDom = inet_ntop(AF_INET6, inlDom, Out6, INET6_ADDRSTRLEN);
			  } else {
                (const char *)plDom = inet_ntop(AF_INET, inlDom, Out6, INET_ADDRSTRLEN);
			  }
#else
			  plDom = inet_ntoa(*inlDom);
#endif
 			  if (plDom)
                strcpy(mlDom, plDom);
			  else 
			     mlDom[0] = '\x0';
#ifdef IPv6
#ifdef VC6
			  if (host)
                freehostent(host);
               //freehostent(host); 同じ
#else  
              if (host)
 	  	        freeaddrinfo(AddrInfo);

#endif
#endif
			}
            mDom[0] = '\x0';
		    inDom = NULL;
#ifdef IPv6
            Error = 0;
#ifdef VC6
            host = getipnodebyname(mDomain, (nAddressFamily ? AF_INET6 : AF_INET),  (nAddressFamily ? AI_ALL : 0), &Error);
#else
		    host = NULL;
            memset(&Hints, 0, sizeof(Hints));
			Hints.ai_flags = AI_NUMERICHOST | AI_PASSIVE;
            Hints.ai_family =  (nAddressFamily ? AF_INET6 : AF_INET);
            Hints.ai_socktype = SOCK_STREAM;
		    if (getaddrinfo(mDomain, NULL, &Hints, &AddrInfo) == 0) {
			  AI = AddrInfo;
			  n = 0;
			  do {
                hst.h_addr_list[n] = (struct sockaddr_in *)(AI->ai_addr);
				AI = AI->ai_next;
			    n++;
			  } while (AI);
			  host = &hst;
           }
#endif
#else
		    host = gethostbyname(mDomain);
#endif
		    if (host) {
		      naliases = 0; // IPアドレスが複数設定されている場合のチェック
			  while(host->h_addr_list[naliases]) {
			    inDom = (struct in_addr *)host->h_addr_list[naliases];
  	            //inDom = (struct in_addr *)host->h_addr; //h_addr_list[0];
			    if (inDom) {
#ifdef IPv6
		          if (nAddressFamily) {
                    (const char *)pDom = inet_ntop(AF_INET6, inDom, Out6, INET6_ADDRSTRLEN);
				  } else {
                    (const char *)pDom = inet_ntop(AF_INET, inDom, Out6, INET_ADDRSTRLEN);
				  }
#else
			      pDom = inet_ntoa(*inDom);
#endif
			      if (pDom)
                    strcpy(mDom, pDom);
			      else
			        mDom[0] = '\x0';
				}
			    if (inlDom && inDom) {
			      if (strcmp(mlDom, mDom) == 0) {
                    sts = TRUE;
			        break;
				  }
				}
			    naliases++;
			  }
#ifdef IPv6
#ifdef VC6
			  if (host)
                freehostent(host);
#else  
              if (host)
 		        freeaddrinfo(AddrInfo);

#endif
#endif
			}
		  }
		  // break;
		}
	  }
	}
    pldom += strlen(pldom);
    pldom++;
  };
  
  return sts;
}

int RetryDomain(char *mMsg) { // char *mMRI, char *mSmtp) {
   char str[LOG_BUFFER], *pMsg, mM[64];
#ifdef Y2038_BUG
   SYSTEMTIME ltime;
   FILETIME   ft, nlast, nacc, nupdate;
   ULARGE_INTEGER *u1, *u2, *u3, *u4, ut;
   //ULARGE_INTEGER *u1, *u2, ut;
   HANDLE hFile;
#else
   time_t ltime;
   u_long nlast = 0;
   struct _stat buf;
   int fh;
#endif
   int  sts = 1, ncnt = 0;

#ifdef Y2038_BUG
   u1 = (ULARGE_INTEGER *)&ft;
   u2 = (ULARGE_INTEGER *)&nlast;
   u3 = (ULARGE_INTEGER *)&nacc;
   u4 = (ULARGE_INTEGER *)&nupdate;
   ut.QuadPart = (__int64)(36000000000*nSendOtherTime);
//   ut.QuadPart = (__int64)(36000000000*nSendMaxRetryTime*3);
//     ut.QuadPart = (__int64)(36000000*nSendMaxRetryTime*3);
   GetSystemTime(&ltime);
   SystemTimeToFileTime(&ltime, &ft);
   nlast = ft;
   hFile = CreateFile( (LPCTSTR)mMsg,
                        GENERIC_READ,
                        0,
                        NULL,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL, 
                        NULL);
   if (hFile != INVALID_HANDLE_VALUE) {
	 //GetFileTime(hFile, NULL, NULL, &nlast);
     //GetFileTime(hFile, &nlast, NULL, NULL);
     //GetFileTime(hFile, &nlast, &nacc, &nupdate);
	 GetFileTime(hFile, &nupdate, &nacc, &nlast);
     CloseHandle(hFile);
   }
   if (u2->QuadPart < u1->QuadPart)
     if (u1->QuadPart - u2->QuadPart > ut.QuadPart)
       sts = -1;   // リトライ時間 1時間毎
   ///////////////////////////////////////////////////
#else
   time( &ltime );
   nlast = ltime;
  ///////////////////////////////////////////////////
  if (mMsg) { // MSGファイルの作成日付取得
	if (strlen(mMsg)) {
      fh = _open( mMsg, _O_RDONLY);
      if( fh != -1 ) {
        if ( _fstat( fh, &buf ) == 0 )
          nlast = buf.st_mtime;
        _close( fh );
	  }
	}
  }
  if (nlast < ltime)
    if (ltime - nlast > 3600*nSendOtherTime) //
      sts = -1;   // リトライ時間 1時間毎
    //if (ltime - nlast > 60*60*nSendMaxRetryTime*3) //
    //  sts = -1;   // リトライ時間 1時間毎
  ///////////////////////////////////////////////////
#endif
#ifdef TRACE_MODE
if (nSenderLog) {
  pMsg = strrchr(mMsg,'\\');
  if (pMsg) {
	pMsg++;
    strncpy(mM, pMsg, sizeof(mM));
	///////////////////////////// 2001.12.30
    //strtok(mM, ".");
    pMsg = strrchr(mM, '.');
    if (pMsg)
     *pMsg = '\x0';
    /////////////////////////////////////////
  }
#ifdef Y2038_BUG
  sprintf(str, "[%s] RetryDomain:(ltime(%I64u)-nlast(%I64u))/Status=%d\n", (pMsg ? mM : mMsg), u1->QuadPart, u2->QuadPart, sts);
  //sprintf(str, "[%s] RetryDomain:(ltime(%I64u)-nlast(%I64u)/nacc(%I64u)/nupdate(%I64u))/Status=%d\n", (pMsg ? mM : mMsg), u1->QuadPart, u2->QuadPart, u3->QuadPart, u4->QuadPart, sts);
#else
  sprintf(str, "[%s] RetryDomain:(ltime(%u)-nlast(%u))/Status=%d\n", (pMsg ? mM : mMsg), ltime, nlast, sts);
#endif

  printfTrace(str);
}
#endif
   return sts;
}

//////// v2.01 2001.7.8 追加 /////////////
void RecoverOKNGFile(char *pBoxDir) {
  char mFileGroup[256], mfsrc[256];
  long               hFindFile;
  struct _finddata_t FindFileData ;
  BOOL bFile = TRUE;

  sprintf(mFileGroup, "%s\\*.$OK", pBoxDir);
  hFindFile = _findfirst( mFileGroup, &FindFileData); //hFindFile = FindFirstFile( mFileGroup, &FindFileData);
  if (hFindFile != -1L) { //INVALID_HANDLE_VALUE) {
	bFile = TRUE;
    while (bFile) {
      if (bServiceTerminating)
	    break;
	  sprintf(mfsrc, "%s\\%s", pBoxDir, FindFileData.name); //cFileName);
       _unlink(mfsrc); //DeleteFile(mfsrc);
	  bFile = (_findnext(hFindFile, &FindFileData) == 0 ? TRUE : FALSE); //bFile = FindNextFile( hFindFile, &FindFileData);
	}; 
    _findclose(hFindFile); //FindClose( hFindFile ); 
  }
  sprintf(mFileGroup, "%s\\*.$NG", pBoxDir);
  hFindFile = _findfirst( mFileGroup, &FindFileData); //hFindFile = FindFirstFile( mFileGroup, &FindFileData);
  if (hFindFile != -1L) { //INVALID_HANDLE_VALUE) {
	bFile = TRUE;
    while (bFile) {
      if (bServiceTerminating)
	    break;
	  sprintf(mfsrc, "%s\\%s", pBoxDir, FindFileData.name); //cFileName);
       _unlink(mfsrc); //DeleteFile(mfsrc);
	  bFile = (_findnext(hFindFile, &FindFileData) == 0 ? TRUE : FALSE); //bFile = FindNextFile( hFindFile, &FindFileData);
	}; 
    _findclose(hFindFile); //FindClose( hFindFile ); 
  }
}

void RecoverDomainMSGFile(char *pBoxDir) {
  char mFileGroup[256], mfsrc[256], mf[2][256];
  long               hFindFile, hF;
  struct _finddata_t FindFileData , Fd;
  BOOL bFile = TRUE;

  sprintf(mFileGroup, "%sholding\\*.MSG", pBoxDir);
  hFindFile = _findfirst( mFileGroup, &FindFileData); //hFindFile = FindFirstFile( mFileGroup, &FindFileData);
  if (hFindFile != -1L) { //INVALID_HANDLE_VALUE) {
	bFile = TRUE;
    while (bFile) {
      if (bServiceTerminating)
	    break;
	  sprintf(mfsrc, "%sholding\\%s", pBoxDir, FindFileData.name); //cFileName);
      sprintf(mf[0], "%sdomains\\*-%s", pBoxDir, FindFileData.name); //cFileName); 
	  mf[0][strlen(mf[0])-4] = '\x0';
      hF = _findfirst(mf[0], &Fd); //hF = FindFirstFile(mf[0], &Fd);
      if (hF != -1L) { //INVALID_HANDLE_VALUE) {
        //sprintf(mf[1], "%sdomains\\%s\\%s", pBoxDir, Fd.name, FindFileData.name); 
		sprintf(mf[1], "%sdomains\\%s\\%s", pBoxDir, Fd.name, FindFileData.name);
        mf[1][strlen(mf[1])-4] = '\x0';
	    strcat(mf[1], ".RCP");
		_findclose(hF); //FindClose(hF); 
        hF = _findfirst(mf[1], &Fd); //hF = FindFirstFile(mf[1], &Fd);
		if (hF != -1L) { //INVALID_HANDLE_VALUE) { // .RCPがあるか
		  _findclose(hF); //FindClose(hF); 
		  mf[1][strlen(mf[1])-3] = '$';
          hF = _findfirst(mf[1], &Fd); //hF = FindFirstFile(mf[1], &Fd);
		  if (hF != -1L) { //INVALID_HANDLE_VALUE) { // .$CPもあるか
		    _findclose(hF); //FindClose(hF); 
		    if (bDebug)
              printf("Delete overlap file.  %s\n", mf[1]);
            _unlink(mf[1]); //DeleteFile(mf[1]);              // 両方の場合は.$CPを削除
		  }
		} else {
          sprintf(mf[1], "%s\\Domain.mri", mf[0]); 
		  if (bDebug)
            printf("Delete alone file.  %s\n", mf[1]);
          _unlink(mf[1]); //DeleteFile(mf[1]);
		  if (bDebug)
            printf("Delete alone file.  %s\n", mfsrc);
          _unlink(mfsrc); //DeleteFile(mfsrc);
		}
	  } else {
		if (bDebug)
          printf("Delete alone file.  %s\n", mfsrc);
		_unlink(mfsrc); //DeleteFile(mfsrc);
	  }
	  bFile = (_findnext(hFindFile, &FindFileData) == 0 ? TRUE : FALSE); //bFile = FindNextFile( hFindFile, &FindFileData);
	}; 
    _findclose(hFindFile); //FindClose( hFindFile ); 
  }
}

void RecoverListsMSGFile(char *pBoxDir) {
  char mFileGroup[256], mfsrc[256], mfdest[256];
  long               hFindFile, hFMSG, hFRCP, hF;
  struct _finddata_t FindFileData, FDMSG, FD;
  BOOL bFile = TRUE, bFMSG, bFRCP;

  sprintf(mFileGroup, "%s%s\\*", mMailSpoolDir, pBoxDir);
  hFindFile = _findfirst( mFileGroup, &FindFileData); //hFindFile = FindFirstFile( mFileGroup, &FindFileData);
  if (hFindFile != -1L) { //INVALID_HANDLE_VALUE) {
	bFile = TRUE;
    while (bFile) {
      if (bServiceTerminating)
	    break;
	  if (!(strcmp(FindFileData.name,".") == 0 ||
		  strcmp(FindFileData.name,"..") == 0 ) ) {
	    sprintf(mfsrc, "%s%s\\%s\\%s", mMailSpoolDir, pBoxDir, FindFileData.name, LOCKFILE);
	    hFMSG = _findfirst(mfsrc, &FDMSG); //hFMSG = FindFirstFile(mfsrc, &FDMSG);
        if (hFMSG != -1L) { //INVALID_HANDLE_VALUE) {
		  _findclose(hFMSG); //FindClose( hFMSG );  // LOCKFILE があれば何もしない
		} else {
		  //// 孤児のMSG
		  bFMSG = TRUE;
	      sprintf(mfsrc, "%s%s\\%s\\*.MSG", mMailSpoolDir, pBoxDir, FindFileData.name);
	      hFMSG = _findfirst(mfsrc, &FDMSG); //hFMSG = FindFirstFile(mfsrc, &FDMSG);
          if (hFMSG != -1L) { //INVALID_HANDLE_VALUE) {
		    while (bFMSG) {
             if (bServiceTerminating)
	           break;
 	          sprintf(mfdest, "%s%s\\%s\\%s", mMailSpoolDir, pBoxDir, FindFileData.name, FDMSG.name);
	          mfdest[strlen(mfdest)-4] = '\x0';
	          strcat(mfdest, ".RCP");
	          hF = _findfirst(mfdest, &FD); //hF = FindFirstFile(mfdest, &FD);
              if (hF != -1L) { //INVALID_HANDLE_VALUE) {
		        _findclose(hF); //FindClose(hF); 
			  } else {
 	            sprintf(mfdest, "%s%s\\%s\\%s", mMailSpoolDir, pBoxDir, FindFileData.name, FDMSG.name);
				if (bDebug)
                  printf("Delete alone file.  %s\n", mfdest);
		        _unlink(mfdest); //DeleteFile(mfdest);
			  }
			  bFMSG = (_findnext(hFMSG, &FDMSG) == 0 ? TRUE : FALSE); //bFMSG = FindNextFile( hFMSG, &FDMSG);
			}
		    _findclose(hFMSG); //FindClose( hFMSG ); 
		  }
		  //// 孤児のRCP
		  bFRCP = TRUE;
	      sprintf(mfsrc, "%s%s\\%s\\*.RCP", mMailSpoolDir, pBoxDir, FindFileData.name);
	      hFRCP = _findfirst(mfsrc, &FDMSG); //hFMSG = FindFirstFile(mfsrc, &FDMSG);
          if (hFRCP != -1L) { //INVALID_HANDLE_VALUE) {
		    while (bFRCP) {
             if (bServiceTerminating)
	           break;
 	          sprintf(mfdest, "%s%s\\%s\\%s", mMailSpoolDir, pBoxDir, FindFileData.name, FDMSG.name);
	          mfdest[strlen(mfdest)-4] = '\x0';
	          strcat(mfdest, ".MSG");
	          hF = _findfirst(mfdest, &FD); //hF = FindFirstFile(mfdest, &FD);
              if (hF != -1L) { //INVALID_HANDLE_VALUE) {
		        _findclose(hF); //FindClose(hF); 
			  } else {
 	            sprintf(mfdest, "%s%s\\%s\\%s", mMailSpoolDir, pBoxDir, FindFileData.name, FDMSG.name);
				if (bDebug)
                  printf("Delete alone file.  %s\n", mfdest);
		        _unlink(mfdest); //DeleteFile(mfdest);
			  }
			  bFRCP = (_findnext(hFRCP, &FDMSG) == 0 ? TRUE : FALSE); //bFMSG = FindNextFile( hFMSG, &FDMSG);
			}
		    _findclose(hFRCP); //FindClose( hFMSG ); 
		  }

		}
	  }
	  bFile = (_findnext(hFindFile, &FindFileData) == 0 ? TRUE : FALSE); //bFile = FindNextFile( hFindFile, &FindFileData);
	}
    _findclose(hFindFile); //FindClose( hFindFile ); 
  }
}

//////// v1.21 2000.8.17 追加 /////////////
void RecoverRCPFile(char *pBoxDir) {
  char mFileGroup[256], mfsrc[256], mfdest[256], mfmsg[256];
  long               hFindFile, hF, hFM;
  struct _finddata_t FindFileData, FD, FDM;
  BOOL bFile = TRUE;

  sprintf(mFileGroup, "%s\\*.$CP", pBoxDir);
  hFindFile = _findfirst( mFileGroup, &FindFileData); //hFindFile = FindFirstFile( mFileGroup, &FindFileData);
  if (hFindFile != -1L) { //INVALID_HANDLE_VALUE) {
	bFile = TRUE;
    while (bFile) {
      if (bServiceTerminating)
	    break;
	  sprintf(mfsrc, "%s\\%s", pBoxDir, FindFileData.name);
      sprintf(mfdest, "%s\\%s", pBoxDir, FindFileData.name); 
	  mfdest[strlen(mfdest)-4] = '\x0';
	  strcpy(mfmsg, mfdest); 
	  strcat(mfdest, ".RCP");
	  strcat(mfmsg, ".MSG");
      hF = _findfirst( mfdest, &FD); //hF = FindFirstFile( mfdest, &FD);
      if (hF != -1L) { //INVALID_HANDLE_VALUE) {
		_findclose(hF); //FindClose( hF ); 
		_unlink(mfdest); //DeleteFile(mfdest);
	  } /*else*/
      hFM = _findfirst( mfmsg, &FDM); //hFM = FindFirstFile( mfmsg, &FDM);
	  if (hFM != -1L) { //INVALID_HANDLE_VALUE) {
		_findclose(hFM); //FindClose(hFM);
	    if (X_MoveFile(mfsrc, mfdest) == ERROR_SUCCESS)
		  if (bDebug)
	        printf("Moved File Success. %s -> %s\n", mfsrc, mfdest);
	    else
		  if (bDebug)
            printf("Moved File Failed(%ld).  %s -> %s\n", GetLastError(), mfsrc, mfdest);
	  } else {
		_unlink(mfsrc); //DeleteFile(mfsrc); // MSGファイルが無いなら削除
	  }
      bFile = (_findnext(hFindFile, &FindFileData) == 0 ? TRUE : FALSE); //bFile = FindNextFile( hFindFile, &FindFileData);
	}; 
    _findclose(hFindFile); //FindClose( hFindFile ); 
  }
}

void RecoverFolder(char *mDir) {
  char mFileGroup[256], mRCPDir[256], mLKFile[256];
  long               hFindFile;
  struct _finddata_t FindFileData ;
  BOOL bFile = TRUE;

  sprintf(mFileGroup, "%s\\%s\\*", mMailSpoolDir, mDir);
  hFindFile = _findfirst( mFileGroup, &FindFileData); //hFindFile = FindFirstFile( mFileGroup, &FindFileData);
  if (hFindFile != -1L) { //INVALID_HANDLE_VALUE) {
    while (bFile) {
	  if (!(strcmp(FindFileData.name,".") == 0 ||
		  strcmp(FindFileData.name,"..") == 0 ) ) {
        sprintf(mLKFile, "%s%s\\%s\\%s", mMailSpoolDir, mDir, FindFileData.name, LOCKFILE);
        _unlink(mLKFile); //DeleteFile(mLKFile);
        sprintf(mRCPDir, "%s%s\\%s", mMailSpoolDir, mDir, FindFileData.name);
		RecoverRCPFile(mRCPDir);
		if (!strcmp(mDir, "lists")) {
		  RecoverListsMSGFile(mDir);
		}
	  }
      bFile = (_findnext(hFindFile, &FindFileData) == 0 ? TRUE : FALSE); //bFile = FindNextFile( hFindFile, &FindFileData);
	}; 
    _findclose(hFindFile); //FindClose( hFindFile ); 
  };
}

//////// v1.21 2000.8.17 追加 /////////////
DWORD NumberRCPFile(char *pBoxDir) {
  char mFileGroup[256];
  long               hFindFile;
  struct _finddata_t FindFileData ;
  BOOL bFile = TRUE;
  DWORD nNum = 0;

  sprintf(mFileGroup, "%s\\*.RCP", pBoxDir);
  hFindFile = _findfirst( mFileGroup, &FindFileData); //hFindFile = FindFirstFile( mFileGroup, &FindFileData);
  if (hFindFile != -1L) { //INVALID_HANDLE_VALUE) {
	bFile = TRUE;
    while (bFile) {
      if (bServiceTerminating)
	    break;
	  nNum++;
      bFile = (_findnext(hFindFile, &FindFileData) == 0 ? TRUE : FALSE); //bFile = FindNextFile( hFindFile, &FindFileData);
	}; 
    _findclose(hFindFile); //FindClose( hFindFile ); 
  }
  return nNum;
}

BOOL HaveRCPFile(char *pBoxDir) {
  char mFileGroup[256];
  long               hFindFile;
  struct _finddata_t FindFileData ;
  BOOL bFile = FALSE;
  DWORD nNum = 0;

  sprintf(mFileGroup, "%s\\*.RCP", pBoxDir);
  hFindFile = _findfirst( mFileGroup, &FindFileData); //hFindFile = FindFirstFile( mFileGroup, &FindFileData);
  if (hFindFile != -1L) { //INVALID_HANDLE_VALUE) {
	bFile = TRUE;
    _findclose(hFindFile); //FindClose( hFindFile ); 
  }
  return bFile;
}


void GetReturnAddress(char *pNS, struct _RCP *rcp, int no) {
#ifdef TRACE_MODE
   char str[LOG_BUFFER];
#endif
   FILE *fpsrc; //, *fpdest;
   char *fsts, mWork[256];///, mNewToNG[256];
   BOOL bOK = FALSE;

#ifdef TRACE_MODE
   if (nSenderLog) {
     sprintf(str, "[%s]:GetReturnAddress(no=%03d):Start=%s\n", rcp->mMNo, no, rcp->mToNG);
     printfTrace(str);
   }
#endif
     //sprintf(mNewToNG, "%s.tmp", rcp->mToNG);
   fpsrc = fopen(rcp->mToNG, "rt");
   if (fpsrc) {
#ifdef DATA_CASH
      setvbuf(fpsrc, rcp->mFrbuf, _IOFBF, sizeof(rcp->mFrbuf) );
#endif
#ifdef TRACE_MODE
     if (nSenderLog) {
       sprintf(str, "[%s]:GetReturnAddress(no=%03d):Read open=%s\n", rcp->mMNo, no, rcp->mToNG);
       printfTrace(str);
	 }
#endif
	 bOK = TRUE;
     fsts = fgets(mWork, sizeof(mWork), fpsrc);
	 while(fsts || !feof(fpsrc)) {
	   strtok(mWork, "\n");
	   strcpy(rcp->mTo, mWork);
#ifdef TRACE_MODE
   if (nSenderLog) {
     sprintf(str, "[%s]:GetReturnAddress(no=%03d):rcp->mTo=%s\n", rcp->mMNo, no, rcp->mTo);
     printfTrace(str);
   }
#endif
#ifdef UPDATE_20061129 // 同報処理内での転送エラーメール発生時にエラーメールのファイル名が重複する不具合。
       strcpy(rcp->mSetRetryMode, "");
#endif
#ifdef UPDATE_0x15639_HUNG_DEBUG
printf("[%s-%03d]:GetReturnAddress(1) mNS=%x\n", rcp->mMNo, no, pNS);
if (pNS > 0x0a000000)
{
  if (nSenderLog || nSender2Log) 
  {
    sprintf(str, "[%s-%03d]:GetReturnAddress(1) mNS=%x\n", rcp->mMNo, no, pNS);
    printfTrace(str);
  }
  exit(0);
}
if (no > 10000)
{
  exit(0);
}
#endif
#ifdef  CS_RETURNMAIL // ReturnMail()中の割り込み処理禁止
   EnterCriticalSection(&g_csReturnMail);
#endif
       ReturnMail(pNS, rcp, no++); // 送信失敗ならメールを戻す。
#ifdef  CS_RETURNMAIL // ReturnMail()中の割り込み処理禁止
   LeaveCriticalSection(&g_csReturnMail);
#endif
       faillog(rcp);               // 配送エラーログ
	   if (rcp->nToNG > 0)
	     rcp->nToNG--;
       fsts = fgets(mWork, sizeof(mWork), fpsrc);
	 }
	 fclose(fpsrc);
     _unlink(rcp->mToNG); //DeleteFile(rcp->mToNG);
   } else {
#ifdef TRACE_MODE
     if (nSenderLog) {
       sprintf(str, "[%s]:GetReturnAddress(no=%03d):Dosen't open=%s\n", rcp->mMNo, no, rcp->mToNG);
       printfTrace(str);
	 }
#endif
   }
   //_flushall();  //を行うと他のファイルポインタに影響がでる。
}

#ifdef UPDATE_20050123
BOOL ScrambleRCP(HANDLE *hFile)
#else
BOOL ScrambleRCP(void)
#endif
{
#ifdef TRACE_MODE
   char str[LOG_BUFFER];
#endif
   DWORD n = 0;

#ifdef UPDATE_20050123
   // sprintf(mRCPLockFn, "%srcp.lck", mMailSpoolDir); グローバルで初期定義
   while ((*hFile = CreateFile( (LPCTSTR)mRCPLockFn,
                        GENERIC_WRITE,
                        0,   // 排他アクセス = 0
                        NULL,
                        CREATE_ALWAYS,
                        FILE_ATTRIBUTE_NORMAL, 
                        NULL)) == INVALID_HANDLE_VALUE) {
   //while (!(fp = fopen(mRCPLockFn, "wt"))) {
     if (bServiceTerminating)
       return FALSE;
	 if (nGetRCPWait > 0 && n++ >= nGetRCPWait)
	   return FALSE;
#ifdef TRACE_MODE
     sprintf(str, "[           ] ScrambleRCP() / thread (%d) / retry (%lu).\n", nRunThread, n);
     if (bDebug)
	   printf("%s", str);
     if (nSenderLog) {
       printfTrace(str);
	 }
#endif
	 _sleep(WAIT_TIME);
   } 
#ifdef UPDATE_20231027 // 送信ファイルのファイルシステムからの一覧取得時に最新の状態を取得する対策
   FlushFileBuffers(hFile);
#endif
#ifdef TRACE_MODE
   if (nSenderLog) { // 20070320test
     sprintf(str, "[           ] ScrambleRCP() Success. / thread (%d) / retry (%lu).\n", nRunThread, n);
     printfTrace(str);
   }
#endif
   /// fclose(fp)　は、.RCPファイル取得完了後
   //bThread = TRUE;
#else
   while(bThread) { // 
     if (bServiceTerminating)
       return FALSE;
	 if (nGetRCPWait > 0 && n++ >= nGetRCPWait)
	   return FALSE;
#ifdef TRACE_MODE
     sprintf(str, "[           ] ScrambleRCP() / thread (%d).\n", nRunThread);
     if (bDebug)
	   printf("%s", str);
     if (nSenderLog) {
       printfTrace(str);
	 }
#endif
#ifdef ENTERPRISE
	  _sleep(WAIT_TIME);
#else
	  _sleep(WAIT_TIME);
#endif
   }
   bThread = TRUE;
#endif

   return TRUE;
}

void GetRCPFile(char *mNS, char *pBoxDir, char *pMsgDir ,DWORD dwWait, int nType) {
#ifdef TRACE_MODE
   char str[LOG_BUFFER];
#endif
  struct _RCP rcp;
  BOOL bRetryOK;
  //char mRcpt[256];
  char mFileGroup[256], mMSG[256], mfn[256], mwork[256], *fsts;
  char mNo[256], *pNo, *p; //, *pdom;
  char mMRI[256], mDm[2][256];
  long                 hFindFile, hFindFileMSG;
  struct _finddata_t   FindFileData, FindMSG;
  BOOL bFile;
  int  sts, mode = 0; //, i = 0;
  DWORD i = 0; //, nTR;
  DWORD nMaxTry, nErr, nNowTry;
  DWORD nTryCnt;
#ifdef UPDATE_20071217A // 「リトライ間隔のまま」オフでもリトライ間隔で処理される不具合
  DWORD n, nRetryTime[14] = {0,2,5,6,14,15,29,30,59,60,119,120,239,240};
#else
  DWORD n, nRetryTime[18] = {0,2,4,5,7,8,10,11,15,16,29,30,59,60,119,120,239,240};
#endif
  BOOL bNotNS = FALSE;
  BOOL bNotMX = FALSE;
  BOOL bGoodDomain = FALSE;
  BOOL bAlias = FALSE;
  char *pMNo, mMNo[256], mWTo[256];
  BOOL bML = FALSE, bNoReply;
  FILE *fpng; //, *fprcp;
#ifdef UPDATE_20050122
  long                 hF;
  struct _finddata_t   FF;
  BOOL   bNGFn, bRCPFn, bMSGFn;
#endif
#ifdef UPDATE_20050123
  HANDLE hFile;
#endif
/*
#ifdef ENTERPRISE
  FILE *fp;
#endif
*/
  InitRCP(&rcp);
#ifdef V4
  strcpy(mFileGroup, pBoxDir);
  if (dwWait == WAIT_TIMEOUT)
	strcat(mFileGroup, "*.rcp");   // リトライ時
  else
	strcat(mFileGroup, "B??????????.rcp");   // リトライ時
#else  // V4
  if (dwWait == WAIT_TIMEOUT)
    sprintf(mFileGroup, "%s*.rcp", pBoxDir);           // リトライ時
  else
    sprintf(mFileGroup, "%sB??????????.rcp", pBoxDir); // メール着信時
#endif // V4

ReGetRCP:
// incomingから配送ファイルが無くなるまでループする仕様
#ifdef UPDATE_20050123
  // incoming 走査権を取得
  if (!ScrambleRCP(&hFile))
    return;
#endif
  hFindFile = _findfirst(mFileGroup, &FindFileData); //hFindFile = FindFirstFile( mFileGroup, &FindFileData);
  if (hFindFile != -1L) { //INVALID_HANDLE_VALUE) {
	bFile = TRUE;
    while (bFile) {
	  if (bServiceTerminating)
		break;
#ifdef V4
	  strcpy(mfn, pBoxDir); strcat(mfn, FindFileData.name);
	  strcpy(rcp.mRcp, mfn);
#else // V4
	  /////////////////////
	  if (bDebug)
	    printf("Find File Name (type(%d)) = [%s]\n", nType, FindFileData.name); //cFileName);
	  sprintf(mfn, "%s%s", pBoxDir, FindFileData.name);
      sprintf(rcp.mRcp, "%s%s", pBoxDir, FindFileData.name); // v1.21 2000.8.17 追加
#endif // V4
	  ////////////////////////////// 2001.12.30
	  if ((p = strrchr(rcp.mRcp, '.')))
		*p = '\x0';
	  //////////////////////////////////////////
#ifdef V4
      strcpy(mMSG, rcp.mRcp); strcat(mMSG,".MSG");
      strcpy(rcp.mToOK, rcp.mRcp); strcat(rcp.mToOK,".$OK");
      strcpy(rcp.mToNG, rcp.mRcp); strcat(rcp.mToNG,".$NG");
	  strcat(rcp.mRcp, ".$CP");
#else  //V4
      sprintf(mMSG, "%s.MSG", rcp.mRcp);
	  sprintf(rcp.mToOK, "%s.$OK", rcp.mRcp);
	  sprintf(rcp.mToNG, "%s.$NG", rcp.mRcp);
	  strcat(rcp.mRcp, ".$CP");
#endif // V4
	  //////////////////////////////////////////
#ifdef NEW_SENDER
#ifndef UPDATE_20050123
      if (!ScrambleRCP())
        return;
#endif
#ifdef UPDATE_20070319A // ファイル確認でハングさせない対策
__try
{
      hFindFileMSG = _findfirst(rcp.mRcp, &FindMSG); //hFindFileMSG = FindFirstFile(rcp.mRcp, &FindMSG); // $CPがあれば読み飛ばし
      if (hFindFileMSG != -1L) { //INVALID_HANDLE_VALUE) {// RCPファイルに対応するMSGファイルが出来ているか?
		_findclose( hFindFileMSG ); //FindClose( hFindFileMSG ); 
		_findclose( hFindFile ); //FindClose( hFindFile ); 
#ifdef UPDATE_20050123
        CloseHandle(hFile);
#else
	    bThread = FALSE;
#endif
		goto ReGetRCP;
        //bFile = (_findnext(hFindFile, &FindFileData) == 0 ? TRUE : FALSE); //bFile = FindNextFile( hFindFile, &FindFileData);
		//continue;
	  }
}
__except(hFile)
{
#ifdef UPDATE_20050123
        CloseHandle(hFile);
#else
	    bThread = FALSE;
#endif
	goto ReGetRCP;
}
#else //UPDATE_20070319A // ファイル確認でハングさせない対策
      // .$CPファイルが出来てしまっているか？＝先に取得のスレッドがあるか？
      hFindFileMSG = _findfirst(rcp.mRcp, &FindMSG); //hFindFileMSG = FindFirstFile(rcp.mRcp, &FindMSG); // $CPがあれば読み飛ばし
      if (hFindFileMSG != -1L) { //INVALID_HANDLE_VALUE) {// RCPファイルに対応するMSGファイルが出来ているか?
		_findclose( hFindFileMSG ); //FindClose( hFindFileMSG ); 
#ifdef UPDATE_20071115 //配送スレッドで取得したファイル名が変更されるまで確認待ちする。
 	    bFile = (_findnext(hFindFile, &FindFileData) == 0 ? TRUE : FALSE); 
		if (bFile) {
          continue; // 次を探す
		} else {
		  _findclose( hFindFile ); //FindClose( hFindFile ); 
          CloseHandle(hFile);
		  goto ReGetRCP;
		}
#else
		_findclose( hFindFile ); //FindClose( hFindFile ); 
#ifdef UPDATE_20050123
        CloseHandle(hFile);
#else
	    bThread = FALSE;
#endif
		goto ReGetRCP;
#endif
        //bFile = (_findnext(hFindFile, &FindFileData) == 0 ? TRUE : FALSE); //bFile = FindNextFile( hFindFile, &FindFileData);
		//continue;
	  }
#endif //UPDATE_20070319A // ファイル確認でハングさせない対策

#ifdef TRACE_MODE
      if (nSenderLog) { // 20070320test
        sprintf(str, "[%s][%d] _findfirst()=%s\n", rcp.mMNo, nRunThread, rcp.mRcp);
        printfTrace(str);
	  }
#endif
      // .RCPを.$CPへリネームし使用中にする
	  if (X_MoveFile(mfn, rcp.mRcp) != 0) { // 書換え
#ifdef UPDATE_20071115 //配送スレッドで取得したファイル名が変更されるまで確認待ちする。
 	    bFile = (_findnext(hFindFile, &FindFileData) == 0 ? TRUE : FALSE); 
		if (bFile) {
          continue; // 次を探す
		} else {
		  _findclose( hFindFile ); //FindClose( hFindFile ); 
          CloseHandle(hFile);
		  goto ReGetRCP;
		}
#else
		_findclose( hFindFile ); //FindClose( hFindFile ); 
#ifdef UPDATE_20050123
        CloseHandle(hFile);
#else
	    bThread = FALSE;
#endif
		goto ReGetRCP;
#endif
        //bFile = (_findnext(hFindFile, &FindFileData) == 0 ? TRUE : FALSE); //bFile = FindNextFile( hFindFile, &FindFileData);
		//continue;
	  }
#ifdef UPDATE_20071115 //配送スレッドで取得したファイル名が変更されるまで確認待ちする。
      // .RCPが消滅したことを確認する
      while((hFindFileMSG = _findfirst(mfn, &FindMSG)) != -1L) { // ファイルが削除されていること
		_findclose( hFindFileMSG );
		DeleteFile(mfn);
        if (bServiceTerminating) {
          CloseHandle(hFile);
          return;
		}
	    _sleep(WAIT_TIME);
	  }
#endif
	  ////////////////////////////////////////
	  // incoming 走査権を開放
#ifdef UPDATE_20231027 // 送信ファイルのファイルシステムからの一覧取得時に最新の状態を取得する対策
	  FlushFileBuffers(hFile);
#endif
#ifdef UPDATE_20050123
      CloseHandle(hFile);
#else
	  bThread = FALSE;
#endif
	  ////////////////////////////////////////
#endif
      hFindFileMSG = _findfirst(mMSG, &FindMSG); //hFindFileMSG = FindFirstFile(mMSG, &FindMSG);
      if (hFindFileMSG != -1L) { //INVALID_HANDLE_VALUE) {// RCPファイルに対応するMSGファイルが出来ているか?
		_findclose(hFindFileMSG); //FindClose( hFindFileMSG ); 
	  } else {
		if (bDebug)
		  printf("Not found MSG File=[%s]\n",mMSG);
		_findclose( hFindFile ); //FindClose( hFindFile ); 
#ifdef UPDATE_20210321 // 添付分離時にMLメンバーが無い場合のエラーメール生成を抑止する
		while(unlink(rcp.mRcp) != 0)
		{
	      if (errno == ENOENT) //ファイルが無いとき
		    break;
		  if (bDebug)
		    printf("Delete File Failed. (ReGetRCP) %s\n", rcp.mRcp);
#ifdef TRACE_MODE
          if (nSenderLog)
	      {
            sprintf(str, "Delete File Failed. (ReGetRCP) errno(%d) %s\n", errno, rcp.mRcp);
            printfTrace(str);
	      }
#endif
		   _sleep(WAIT_TIME);
		}
#endif
		goto ReGetRCP;
		//bFile = (_findnext(hFindFile, &FindFileData) == 0 ? TRUE : FALSE); //bFile = FindNextFile( hFindFile, &FindFileData);
		//continue;
	  }
#ifndef NEW_SENDER
	  if (!X_CopyFile(mfn, rcp.mRcp, TRUE)) {
		if (bDebug)
	      printf("Copy File Failed. GetRCPFile(1) %s -> %s\n", mfn, rcp.mRcp);
		_findclose( hFindFile ); //FindClose( hFindFile ); 
		goto ReGetRCP;
		bFile = (_findnext(hFindFile, &FindFileData) == 0 ? TRUE : FALSE); //bFile = FindNextFile( hFindFile, &FindFileData);
	    continue;
	  } else {
		if (bDebug)
	      printf("Copy File Success. GetRCPFile(1) %s -> %s\n", mfn, rcp.mRcp);
	  }
	  if (!_unlink(mfn)) //if (DeleteFile(mfn))
        if (bDebug)
		  printf("DeleteFile Success. type(%d) [%s]\n", nType, FindFileData.name);
	  else {
        if (bDebug)
	  	  printf("DeleteFile Failed. type(%d)  [%s]\n", nType, FindFileData.name);
		_unlink(rcp.mRcp); //DeleteFile(rcp.mRcp);
		_findclose( hFindFile ); //FindClose( hFindFile ); 
		goto ReGetRCP;
		bFile = (_findnext(hFindFile, &FindFileData) == 0 ? TRUE : FALSE); //bFile = FindNextFile( hFindFile, &FindFileData);
	    continue;
	  }
#endif
	  ///////////////
	  strcpy(mfn, rcp.mRcp); 
	  //////////////////////////////////////////
	  rcp.mMID[0] = '\x0';
	  rcp.mFrom[0] = '\x0';
	  rcp.mTo[0] ='\x0';
#ifndef V4
#ifdef TRACE_MODE
      if (nSenderLog) {
        printfTrace("------------------------------------------\n");
        sprintf(str, "[           ] %s:GetRCPFile:RCP fopen()\n", mfn);
        printfTrace(str);
	  }
#endif
#endif
	  while(!(rcp.fp = fopen(mfn, "rt"))) {
        if (bServiceTerminating)
		  break;
		 _sleep(WAIT_TIME);
	  }
	  if (rcp.fp) {
		strcpy(rcp.mRcp, mfn); 
		strcpy(rcp.mMsg, mMSG);
		fgets(rcp.mMID, sizeof(rcp.mMID), rcp.fp);
		strcpy(rcp.mMIDNo, rcp.mMID);
		strword(rcp.mMID, "@", ">\r\n");
		strword(rcp.mMIDNo, "<", ">\r\n");
		//strncpy(rcp.mMNo, rcp.mMIDNo, 11); rcp.mMNo[11] = '\x0';
		strcpy(rcp.mMNo, rcp.mMIDNo); // 5.18
		strtok(rcp.mMNo, "@");        // 5.18
		fgets(rcp.mFrom, sizeof(rcp.mFrom), rcp.fp);
		strword(rcp.mFrom, " ", " \r\n");
		i = 0;
		rcp.mTo[0] = '\x0';
 	    rcp.fsts = fgets(rcp.mTo, sizeof(rcp.mTo), rcp.fp);
        while (rcp.fsts || !feof(rcp.fp)) {
		  i++;
//#ifndef V4
#ifdef TRACE_MODE
         if (nSenderLog) {
           sprintf(str, "[%s] GetRCPFile:while(i=%03d):To=%s\n", rcp.mMNo, i, rcp.mTo);
           printfTrace(str);
		 }
#endif
//#endif
		////////////////////////////////////////////
#ifdef UPADTE_20040518
		  if (!strnicmp("Recipient: ", rcp.mTo, 10) || strchr(rcp.mTo, '@') )
		    strcpy(rcp.mDomain, rcp.mTo);
		  else
		    rcp.mDomain[0] = '\x0';
#else
	      strcpy(rcp.mDomain, rcp.mTo);
#endif    
#ifdef UPDATE_20050428
	      if (strnicmp(rcp.mTo, "recipient:", 10))
		    strtok(rcp.mTo, "\r\n");
	      else
#endif
		    strword(rcp.mTo, " ", "\r\n");
		  strword(rcp.mTo, "<", ">");
		  strword(rcp.mDomain, "@\r\n", "> \r\n\a\b\f\t\v\x0b");
		  if (!rcp.mTo[0]) {
#ifdef UPDATE_20050428
 	        rcp.fsts = fgets(rcp.mTo, sizeof(rcp.mTo), rcp.fp);
			continue;
#else
			break;
#endif
		  }
#ifndef V4
#ifdef TRACE_MODE
         if (nSenderLog) {
           sprintf(str, "[%s] GetRCPFile:while():To=%s\n", rcp.mMNo, rcp.mTo);
           printfTrace(str);
		 }
#endif
#endif
          bGoodDomain = BadCharcter(rcp.mDomain);
          if (bGoodDomain) {
		    //if (rcp.mDomain[0] == '\x0')
			  //strcpy(rcp.mDomain, mLocalDomain);
            strcpy(mwork, mNS);
 		    //////////////// 外部ドメインへの送信タイムアウトチェック
            /////////////////////////////////////////
            strcpy(mNo, rcp.mMsg);
            ///////////////////////////// 2001.12.30
            //strtok(mNo, ".");
            pNo = strrchr(mNo, '.');
            if (pNo)
              *pNo = '\x0';
            /////////////////////////////////////////
            pNo = strrchr(mNo, '\\');
            if (pNo)
	          pNo++;
            else
	          pNo = mNo;
#ifdef UPDATE_20080213 // ドメイン無しアドレスの外部転送で転送できない場合にbaddomainフォルダ内のメールがリトライし続ける
			if (!rcp.mDomain[0] && bMailForward) {
	          sprintf(mMRI, "%s\\domains\\baddomain-%s\\domain.mri", mMailSpoolDir, pNo);
			} else
#endif
			{
	          sprintf(mMRI, "%s\\domains\\%s-%s\\domain.mri", mMailSpoolDir, rcp.mDomain, pNo);
			}
            /////////////////////////////////////////
		    rcp.mSmtp[0] = '\x0';
            if (!bThreadType && i >= 20) { // シングルスレッドでメーリングリスト配送先が指定数以上なら一旦保留して他のメール遅配が無いように処理させる。
	          if (!(fpng = fopen(rcp.mToNG, "at")))
	            fpng = fopen(rcp.mToNG, "wt");
	          if (fpng) {
	            fprintf(fpng, "%s\n", rcp.mTo);
                fflush(fpng);
	            fclose(fpng);
			    while(!(fpng = fopen(rcp.mToNG, "rt"))) {
                  if (bServiceTerminating)
		            break;
				  _sleep(WAIT_TIME);
				}
			    fclose(fpng);
			  }
			  strcpy(rcp.mSmtp, rcp.mDomain);
#ifdef UPDATE_20061129 // 同報処理内での転送エラーメール発生時にエラーメールのファイル名が重複する不具合。
	          strcpy(rcp.mSetRetryMode, "");
#endif
              SetRetry(&rcp, FALSE, (DWORD) 0, i); //FALSE);
		      rcp.mTo[0] ='\x0';
	          fsts = fgets(rcp.mTo, sizeof(rcp.mTo), rcp.fp);
			  continue;
			}
			nMaxTry = GetTryServer(rcp.mDomain, rcp.mMsg, &nErr, TRUE);
#ifndef V4
#ifdef TRACE_MODE
#ifdef UPDATE_20060904A // ロックファイルの作成確認
           if (nSenderLog || nSender2Log)
#else
           if (nSenderLog)
#endif
		   {
              sprintf(str, "[%s] GetRCPFile:nMaxTry = %d, code= %d\n", rcp.mMNo, nMaxTry, nErr);
              printfTrace(str);
			}
#endif
#endif


#ifdef UPDATE_20060621 // リトライ間隔のままのリトライがスレッドが生きていり間リトライされ続けてしまう対策
		    //if (!bRetryRule ||
                //bRetryRule && 
		  if (!bCompl)
#else
		  if (bRetryRule && 
				((nErr != 0 && nMaxTry < nRetryTime[7]) ||
				 (nErr == 0 && nMaxTry < nRetryTime[17])) )  // リトライ送信間隔の制御を有効にする
#endif
		  {
			  bRetryOK = FALSE;
#ifdef UPDATE_20060621 // リトライ間隔のままのリトライがスレッドが生きていり間リトライされ続けてしまう対策
		      nNowTry = GetTryServer(rcp.mDomain, rcp.mMsg, NULL, FALSE);
#ifdef TRACE_MODE
			  if (nSenderLog) { //|| nSender2Log) {
                  sprintf(str, "[%s] bRetryRule=%d / (nMaxTry(%d) == nNowTry(%d) ||  nMaxTry=(%d) >= nSendRefusalTime=(%d))\n", rcp.mMNo, bRetryRule, nMaxTry, nNowTry, nMaxTry, nSendRefusalTime);
                  printfTrace(str);
			  }
#endif
			  if (!bRetryRule) {
#ifdef UPDATE_20060629 // 400番台のリトライが500番台に切替えられるてしまう。
               if (nMaxTry == nNowTry ||
				   nErr >= 500 && nMaxTry >= nSendRefusalTime ||             // 500番台 (中継拒否)
	               nErr >= 400 && nErr < 500 && nMaxTry >= nSendMaxRetry ||  // 400番台 (回線異常)
#ifdef UPDATE_20120308 // 300番台(354)で切断された時もリトライを行う
	               nErr >= 300 && nErr < 400 && nMaxTry >= nSendMaxRetry ||  // 300番台 (回線異常)
#endif
		           nErr == 0   && nMaxTry >= nSendOther)                     //（無通信）
#else
				if (nMaxTry == nNowTry || nMaxTry >= nSendRefusalTime)
#endif
                  bRetryOK = TRUE;
			  } else {
#ifdef UPDATE_20060629 // 400番台のリトライが500番台に切替えられるてしまう。
               if (nMaxTry == nNowTry ||
				   nErr >= 500 && nMaxTry >= nSendRefusalTime ||             // 500番台 (中継拒否)リトライ時間を超えた場合
	               nErr >= 400 && nErr < 500 && nMaxTry >= nSendMaxRetry ||   // 400番台 (回線異常)リトライ時間を超えた場合
#ifdef UPDATE_20120308 // 300番台(354)で切断された時もリトライを行う
	               nErr >= 300 && nErr < 400 && nMaxTry >= nSendMaxRetry ||  // 300番台 (回線異常)
#endif
				   nErr == 0   && nMaxTry >= nSendOther)                   //（無通信）リトライ時間を超えた場合
#else
			    if (nMaxTry == nNowTry || nMaxTry >= nSendRefusalTime)
#endif
				{
#endif
#ifdef UPDATE_20071217A // 「リトライ間隔のまま」オフでもリトライ間隔で処理される不具合
			      for (n = 0; n < 14; n++)
#else
			      for (n = 0; n < 16; n++)
#endif
				  {
#ifdef UPDATE_20060621 // リトライ間隔のままのリトライがスレッドが生きている間リトライされ続けてしまう対策
#ifdef UPDATE_20071217A // 「リトライ間隔のまま」オフでもリトライ間隔で処理される不具合
				   if (nErr >= 500 && nMaxTry >= nSendRefusalTime ||             // 500番台 (中継拒否)リトライ時間を超えた場合
	                   nErr >= 400 && nErr < 500 && nMaxTry >= nSendMaxRetry ||   // 400番台 (回線異常)リトライ時間を超えた場合
					   nErr == 0   && nMaxTry >= nSendOther)                    //（無通信）リトライ時間を超えた場合
				   { // 時間を越えたら強制リトライ
				       nMaxTry = n;
				       bRetryOK = TRUE;
					   break;
				   } else if (nMaxTry%240 == nRetryTime[n])
#else
			        if (nMaxTry%240 == nRetryTime[n] || nMaxTry >= nSendRefusalTime)
#endif
#else
			        if (nMaxTry == nRetryTime[n])
#endif
                    {
#ifdef UPDATE_20060621 // リトライ間隔のままのリトライがスレッドが生きている間リトライされ続けてしまう対策
#else
				       nMaxTry = n;
#endif
				       bRetryOK = TRUE;
				       break;
					}
				  }
#ifdef UPDATE_20060621 // リトライ間隔のままのリトライがスレッドが生きていり間リトライされ続けてしまう対策
				  if (!bRetryOK)
                    Inc_MRI_TryCount2(rcp.mMNo, mMRI);
				}
			  }
#endif
			  if (!bRetryOK)
			  { // リトライの時間でない場合は、domains,holdingフォルダに戻す。
	            if (!(fpng = fopen(rcp.mToNG, "at")))
	              fpng = fopen(rcp.mToNG, "wt");
	            if (fpng) {
	              fprintf(fpng, "%s\n", rcp.mTo);
                  fflush(fpng);
	              fclose(fpng);
			      while(!(fpng = fopen(rcp.mToNG, "rt"))) {
                    if (bServiceTerminating)
		              break;
				    _sleep(WAIT_TIME);
				  }
			      fclose(fpng);
				}
#ifdef UPDATE_20061129 // 同報処理内での転送エラーメール発生時にエラーメールのファイル名が重複する不具合。
 	            strcpy(rcp.mSetRetryMode, "");
#endif
                SetRetry(&rcp, TRUE, nErr, i); //FALSE); // ドメインが応答なしならDomainフォルダとHoldフォルダに振分け保存
 		        rcp.mTo[0] ='\x0';
	            rcp.fsts = fgets(rcp.mTo, sizeof(rcp.mTo), rcp.fp);
		        if (bServiceTerminating)
			      break;
#ifndef V4
#ifdef TRACE_MODE
                if (nSenderLog) {
                  sprintf(str, "[%s] GetRCPFile:It is not the time of the retry. nMaxTry=%d\n", rcp.mMNo, nMaxTry);
                  printfTrace(str);
				}
#endif
#endif
			    continue;
			  }
			}
#ifdef UPDATE_20060621 // リトライ間隔のままのリトライがスレッドが生きていり間リトライされ続けてしまう対策
            Inc_MRI_TryCount2(rcp.mMNo, mMRI);
#endif
			////////////////////////////
		    mode = RetryDomain(rcp.mMsg); //mMRI, rcp.mSmtp); // 対象ドメインのリトライ状況チェック
	        bNotNS = FALSE;
			if (bDebug)
			  printf("GetRCPFile():GetSMTPServer(%s, %s)\n", mNS, rcp.mDomain);
#ifdef TRACE_MODE
            if (nSenderLog) { //20070320test
              sprintf(str, "[%s] GetRCPFile:GetSMTPServer(%s, %s)\n", rcp.mMNo, mNS, rcp.mDomain);
              printfTrace(str);
			}
#endif
			if (bDebug)
	          printf("call GetSMTPServer() rcp.mMNo=[%s]\n", rcp.mMNo);
		    //if (!GetSMTPServer(mNS, rcp.mDomain, rcp.mMsg, rcp.mSmtp, &bNotNS, &bNotMX) &&
#ifdef UPDATE_20071217A // 「リトライ間隔のまま」オフでもリトライ間隔で処理される不具合
#ifdef UPDATE_20241217_RCP // gateway.datにSMTP認証の指定が別のセッションで上書きされない対策
            if (!GetSMTPServer(mNS, rcp.mDomain, rcp.mFrom, rcp.mTo, rcp.mMsg, rcp.mSmtp, &rcp.nPort, &rcp.bUsedSSL, &bNotNS, &bNotMX, &rcp.nGateAuthType) &&
				nNowTry % 2 == 1) 
#else
#ifdef UPDATE_20241128 // GATEWAY.DATにエンベロープの送信元の条件も追加可能に拡張した。
            if (!GetSMTPServer(mNS, rcp.mDomain, rcp.mFrom, rcp.mTo, rcp.mMsg, rcp.mSmtp, &rcp.nPort, &rcp.bUsedSSL, &bNotNS, &bNotMX) &&
				nNowTry % 2 == 1) 
#else
#ifdef UPDATE_20141023 // メールアドレス単位でもGatway.datでフォワード指定可能にする対策
            if (!GetSMTPServer(mNS, rcp.mDomain, rcp.mTo, rcp.mMsg, rcp.mSmtp, &rcp.nPort, &rcp.bUsedSSL, &bNotNS, &bNotMX) &&
				nNowTry % 2 == 1) 
#else
            if (!GetSMTPServer(mNS, rcp.mDomain, rcp.mMsg, rcp.mSmtp, &rcp.nPort, &rcp.bUsedSSL, &bNotNS, &bNotMX) &&
				nNowTry % 2 == 1) 
#endif
#endif
#endif
#else
#ifdef UPDATE_20241217_RCP // gateway.datにSMTP認証の指定が別のセッションで上書きされない対策
            if (!GetSMTPServer(mNS, rcp.mDomain, rcp.mFrom, rcp.mTo, rcp.mMsg, rcp.mSmtp, &rcp.nPort, &rcp.bUsedSSL, &bNotNS, &bNotMX, &r.nGateAuthType) &&
				nMaxTry % 2 == 1) 
#else
#ifdef UPDATE_20241128 // GATEWAY.DATにエンベロープの送信元の条件も追加可能に拡張した。
            if (!GetSMTPServer(mNS, rcp.mDomain, rcp.mFrom, rcp.mTo, rcp.mMsg, rcp.mSmtp, &rcp.nPort, &rcp.bUsedSSL, &bNotNS, &bNotMX) &&
				nMaxTry % 2 == 1) 
#else
#ifdef UPDATE_20141023 // メールアドレス単位でもGatway.datでフォワード指定可能にする対策
            if (!GetSMTPServer(mNS, rcp.mDomain, rcp.mTo, rcp.mMsg, rcp.mSmtp, &rcp.nPort, &rcp.bUsedSSL, &bNotNS, &bNotMX) &&
				nMaxTry % 2 == 1) 
#else
            if (!GetSMTPServer(mNS, rcp.mDomain, rcp.mMsg, rcp.mSmtp, &rcp.nPort, &rcp.bUsedSSL, &bNotNS, &bNotMX) &&
				nMaxTry % 2 == 1) 
#endif
#endif
#endif
#endif
			{
		      strcpy(rcp.mSmtp, rcp.mDomain);
			} else {
			////////////////////////////
			  if (nMaxTry < nSendMaxRetry && bNotMX == FALSE) {// 対象ドメインのMXレコードが見つからない
			    bNotNS = TRUE;
		    //if (nMaxTry < nSendMaxRetry && bNotMX == FALSE) // 対象ドメインのMXレコードが見つからない
			    bNotMX = TRUE;
			  }
			}
		    if (bNotNS) {
		      if (rcp.mSmtp[0] == '\x0')
		        strcpy(rcp.mSmtp, rcp.mDomain); //"mxin1.so-net.ne.jp"); //
		      strcpy(mNS, mwork);
			  if (bDebug)
		        printf("domain:[%s] smtp server:[%s]\n", (rcp.mDomain[0] != '\x0'? rcp.mDomain : "Local"), (rcp.mSmtp[0] != '\x0' ? rcp.mSmtp : "Local"));
		      //////////////// 外部ドメインへの送信タイムアウトチェック
	          //sprintf(mMRI, "%s\\domains\\%s\\domain.mri", mMailSpoolDir, rcp.mDomain);
		      //mode = RetryDomain(mMRI); // 対象ドメインのリトライ状況チェック
			}
#ifdef UPDATE_20150108 // 送信許可時間の設定機能を追加
		    rcp.bUseTime = TRUE;
            if (nUseTime) // GATEWAY単位
			{
              rcp.bUseTime = GetUseTimeFile(rcp.mFrom, rcp.mTo, rcp.mSmtp); // 有効時間であるか確認
			}
#endif
		  }
#ifndef V4
#ifdef TRACE_MODE
          if (nSenderLog) { // 20070320test
            sprintf(str, "[%s] GetRCPFile:Result mode=%d, bNotNS=%d, bNotMX=%d, bGoodDomain=%d, nMaxTry=%d\n", rcp.mMNo, mode,bNotNS,bNotMX,bGoodDomain,nMaxTry);
            printfTrace(str);
		  }
#endif
#endif
          if (mode == -1 || !bNotNS || !bNotMX || !bGoodDomain) {// リトライ時間オーバーなら
		    //sprintf(rcp.mMsg, "%s\\holding\\%s.MSG", mMailSpoolDir, rcp.mMNo);
		    sprintf(rcp.mAnswer,"Unable to deliver to destination domain\n> Cannot resolve %s\n", rcp.mDomain);
#ifdef TRACE_MODE
        if (nSenderLog) {
          sprintf(str, "[%s] GetRCPFile:rcp.mAnswer(%s)\n", rcp.mMNo, rcp.mAnswer);
          printfTrace(str);
		}
#endif
#ifdef UPDATE_20061129 // 同報処理内での転送エラーメール発生時にエラーメールのファイル名が重複する不具合。
            strcpy(rcp.mSetRetryMode, "");
#endif
#ifdef UPDATE_0x15639_HUNG_DEBUG
printf("[%s-%03d]:GetRCPFile(1) mNS=%x\n", rcp.mMNo, i, mNS);
if (mNS > 0x0a000000)
{
  if (nSenderLog || nSender2Log) 
  {
    sprintf(str, "[%s-%03d]:GetRCPFile(1) mNS=%x\n", rcp.mMNo, i, mNS);
    printfTrace(str);
  }
  exit(0);
}
if (i > 10000)
{
  exit(0);
}
#endif
#ifdef  CS_RETURNMAIL // ReturnMail()中の割り込み処理禁止
   EnterCriticalSection(&g_csReturnMail);
#endif
		    ReturnMail(mNS, &rcp, i);
#ifdef  CS_RETURNMAIL // ReturnMail()中の割り込み処理禁止
   LeaveCriticalSection(&g_csReturnMail);
#endif
#ifdef TRACE_MODE
        if (nSenderLog) { // 20070320test
          sprintf(str, "[%s] GetRCPFile:ReturnMail(1)\n", rcp.mMNo);
          printfTrace(str);
		}
#endif
            faillog(&rcp);               // 配送エラーログ
		  //////////////////////////////////////////////////////////
		  } else if (!(rcp.mDomain[0] != '\x0' && rcp.mSmtp[0] == '\x0')) {
/* // エイリアスで変換されたアドレスを再度エイリアス変換してしまうのでＮＧ。
			strcpy(mRcpt, rcp.mTo);
	        pdom = strstr(mRcpt, "@");
            if (pdom)
              *pdom = '\x0';
	        bAlias = GetAliases((LPCTSTR)SOFT_ALIASES_REG, (char *)mRcpt, (char *)pdom);
#ifdef TRACE_MODE
if (nSenderLog || nSender2Log) {
  if (bAlias) {
    sprintf(str, "[%s] GetRCPFile:GetAliases(%s -> %s)\n", rcp.mMNo, rcp.mTo, mRcpt);
  } else {
    sprintf(str, "[%s] GetRCPFile:GetAliases(%s -> isn't alias.)\n", rcp.mMNo, rcp.mTo);
  }
  printfTrace(str);
}
#endif
*/
#ifdef V3
#ifdef UPDATE_20150108 // 送信許可時間の設定機能を追加
			if (rcp.bUseTime && 
				(!bMailForward && 
				 (CheckLocalServer(rcp.mDomain) ||
				 QueryMLists((LPCTSTR)SOFT_LISTS_REG, rcp.mTo))) ) // V3では管理ドメイン以外のMLグループの配送も可能にした。
#else
			if (/* !bAlias && */
				!bMailForward && 
				(CheckLocalServer(rcp.mDomain) ||
				QueryMLists((LPCTSTR)SOFT_LISTS_REG, rcp.mTo)) ) // V3では管理ドメイン以外のMLグループの配送も可能にした。
#endif
#else
			if (/* !bAlias && */
				!bMailForward && 
				CheckLocalServer(rcp.mDomain))
#endif
			{
			  bNoReply = FALSE; // 2002.05.15 通常結果の返信を行う。
			  if (CheckUser(rcp.mFrom, rcp.mTo, rcp.mAnswer, &bML, &bNoReply)) {
                if (strstr(rcp.mFrom, "-request@") &&  // 両方ともリクエストアドレスなら管理者へ返信
                    strstr(rcp.mTo, "-request@") ) {
				  if (strstr(mPostmaster,"@"))
                    sprintf(rcp.mTo, "%s", mPostmaster);
				  else
                    sprintf(rcp.mTo, "%s@%s", mPostmaster, rcp.mMID);
				}
	            /////////////////////////////////////////////////////////
				/// MLにMLアドレスが見つかったらグローバル送信
				//////////////////////////////////////
                pMNo = strrchr(rcp.mMsg, '\\');
	            if (!pMNo)
	              strcpy(mMNo, rcp.mMsg);
	            else
	              strcpy(mMNo, (pMNo+1));
                ///////////////////////////// 2001.12.30
                //strtok(mMNo, ".");
                pMNo = strrchr(mMNo, '.');
                if (pMNo)
                  *pMNo = '\x0';
                /////////////////////////////////////////
				//////////////////////////////////////
				/*
				if (bML && strlen(mMNo) == 21)
                  if (mMNo[11] == '-' && mMNo[12] == 'M')
				    goto GlobalMail;
				*/
				if (bML)
                  if (strstr(mMNo,"-M") || strstr(mMNo,"-m"))
				    goto GlobalMail;
	            /////////////////////////////////////////////////////////
#ifdef UPDATE_20080704 // 転送指定があると、SMTP情報が書き換わってoutlocallogに記載されてしまう
                 strcpy(rcp.mBSmtp, rcp.mSmtp);
                 strcpy(rcp.mBSmtpIP, rcp.mSmtpIP);
#endif
 		        SendLocalMail(mNS, &rcp, FindFileData.name, i); //cFileName, i);
#ifdef UPDATE_20080704 // 転送指定があると、SMTP情報が書き換わってoutlocallogに記載されてしまう
                 strcpy(rcp.mSmtp, rcp.mBSmtp);
                 strcpy(rcp.mSmtpIP, rcp.mBSmtpIP);
#endif
	            outlocallog(&rcp);  // 内部メールボックス送信ログ;
			  } else {
				if (!bNoReply) { // 結果応答はしない場合。
#ifdef UPDATE_20061129 // 同報処理内での転送エラーメール発生時にエラーメールのファイル名が重複する不具合。
                  strcpy(rcp.mSetRetryMode, "");
#endif
#ifdef UPDATE_0x15639_HUNG_DEBUG
printf("[%s-%03d]:GetRCPFile(2) mNS=%x\n", rcp.mMNo, i, mNS);
if (mNS > 0x0a000000)
{
  if (nSenderLog || nSender2Log) 
  {
    sprintf(str, "[%s-%03d]:GetRCPFile(2) mNS=%x\n", rcp.mMNo, i, mNS);
    printfTrace(str);
  }
  exit(0);
}
if (i > 10000)
{
  exit(0);
}
#endif
#ifdef  CS_RETURNMAIL // ReturnMail()中の割り込み処理禁止
   EnterCriticalSection(&g_csReturnMail);
#endif
				  ReturnMail(mNS, &rcp, i);
#ifdef  CS_RETURNMAIL // ReturnMail()中の割り込み処理禁止
   LeaveCriticalSection(&g_csReturnMail);
#endif
#ifdef UPDATE_20121122 // メーリングリスト配送先が存在しないローカルユーザ宛の時の配送失敗ログを記録するようにした。
                  faillog(&rcp);               // 配送エラーログ
#endif
#ifdef TRACE_MODE
        if (nSenderLog) {
          sprintf(str, "[%s] GetRCPFile:ReturnMail(2)\n", rcp.mMNo);
          printfTrace(str);
		}
#endif
				}
			  }
			} else {
GlobalMail:
			  strcpy(mDm[0], rcp.mDomain);
			  strcpy(rcp.mCurrentTo, rcp.mTo);
              sts = SendGlobalMail(&rcp);
			  strcpy(mDm[1], rcp.mDomain);
			  if (rcp.nToOK)
	            outlog(&rcp);  // 送信成功なら;
			  rcp.nToOK = 0;
	          if (sts == (int)TRUE) {
	            //outlog(&rcp);  // 送信成功なら;
				if (bDebug)
	              printf(" Messages %s send.\n", rcp.mDomain);
		        if (bServiceTerminating)
			      break;
			    continue;
			  } else{
				strcpy(rcp.mDomain, mDm[0]);
				if (SendGlobalMailStatus(rcp.mDomain, rcp.mMsg, sts, TRUE)) {
#ifdef UPDATE_20061129 // 同報処理内での転送エラーメール発生時にエラーメールのファイル名が重複する不具合。
 	              strcpy(rcp.mSetRetryMode, "");
#endif
                  SetRetry(&rcp, TRUE, (DWORD) sts, i); //FALSE); // ドメインが応答なしならDomainフォルダとHoldフォルダに振分け保存
				} else {
				  _unlink(rcp.mToOK); //DeleteFile(rcp.mToOK);           // 同一ドメイン送信失敗ならOKファイル削除
				  strcpy(mWTo, rcp.mTo);           // 次のアドレスバックアップ
				  strcpy(rcp.mTo, rcp.mCurrentTo); // エラーアドレス
				  GetReturnAddress(mNS, &rcp, i);
				  strcpy(rcp.mCurrentTo, rcp.mTo); // エラーアドレス
				  strcpy(rcp.mTo, mWTo);           // 元に戻す
				}
				if (bDebug)
		          printf(" Messages %s not send.\n", rcp.mDomain);
				strcpy(rcp.mDomain, mDm[1]);
				continue;
			  }
			}
		  } else {
#ifdef UPDATE_20061129 // 同報処理内での転送エラーメール発生時にエラーメールのファイル名が重複する不具合。
            strcpy(rcp.mSetRetryMode, "");
#endif
            SetRetry(&rcp, TRUE, (DWORD) -1, i); //FALSE); // ドメインが応答なしならDomainフォルダとHoldフォルダに振分け保存
		  }
 		  rcp.mTo[0] ='\x0';
	      rcp.fsts = fgets(rcp.mTo, sizeof(rcp.mTo), rcp.fp);
		  if (bServiceTerminating)
			break;
		}
		//////////////////
#ifdef UPDATE_20210704 //配送処理後の"$CP"が削除されずに残ラ内容にする対策
		while(unlink(rcp.mToNG) != 0)
		{
	      if (errno == ENOENT) //ファイルが無いとき
		    break;
		  if (bDebug)
		    printf("Delete File Failed.  type(%d) %s\n", nType, rcp.mToNG);
#ifdef TRACE_MODE
          if (nSenderLog)
	      {
            sprintf(str, "Delete File Failed.  errno(%d) type(%d) %s\n", errno, nType, rcp.mToNG);
            printfTrace(str);
	      }
#endif
		    _sleep(WAIT_TIME);
		}
		if (bDebug)
		  printf("Delete File Success. type(%d) %s\n", nType, rcp.mToNG);
#ifdef TRACE_MODE
        if (nSenderLog)
	    {
          sprintf(str, "Delete File Success. type(%d) %s\n", nType, rcp.mToNG);
          printfTrace(str);
	    }
#endif
#else
		if (!_unlink(rcp.mToNG)) { //if (DeleteFile(rcp.mToNG))
#ifdef UPDATE_20050122
		  bNGFn = TRUE;
#endif
		  if (bDebug)
		    printf("Delete File Success. type(%d) %s\n", nType, rcp.mToNG);
		} else {
#ifdef UPDATE_20050122
		  bNGFn = FALSE;
#endif
		  if (bDebug)
		    printf("Delete File Failed.  type(%d) %s\n", nType, rcp.mToNG);
		}
#endif
		///////////////
	    fclose(rcp.fp);
#ifndef V4
#ifdef TRACE_MODE
        if (nSenderLog) {
          sprintf(str, "[%s] GetRCPFile:RCP %s fclose()\n", rcp.mMNo, rcp.mRcp);
          printfTrace(str);
		}
#endif
#endif
#ifdef UPDATE_20210704 //配送処理後の"$CP"が削除されずに残ラ内容にする対策
		  while(unlink(rcp.mMsg) != 0)
		  {
	        if (errno == ENOENT) //ファイルが無いとき
		      break;
			if (bDebug)
		      printf("Delete File Failed. type(%d) %s\n", nType, rcp.mMsg);
#ifdef TRACE_MODE
            if (nSenderLog)
	        {
              sprintf(str, "Delete File Failed.  errno(%d) type(%d) %s\n", errno, nType, rcp.mMsg);
              printfTrace(str);
	        }
#endif
		     _sleep(WAIT_TIME);
		  }
		  if (bDebug)
		    printf("Delete File Success. type(%d) %s\n", nType, rcp.mMsg);
#ifdef TRACE_MODE
          if (nSenderLog)
	      {
            sprintf(str, "Delete File Success. type(%d) %s\n", nType, rcp.mMsg);
            printfTrace(str);
	      }
#endif
#else
		//if (pBoxDir == pMsgDir) {
		  if (!_unlink(rcp.mMsg)) { //if (DeleteFile(rcp.mMsg))
#ifdef UPDATE_20050122
		    bMSGFn = TRUE;
#endif
		    if (bDebug)
			  printf("Delete File Failed. type(%d) %s\n", nType, rcp.mMsg);
		  } else {
#ifdef UPDATE_20050122
		    bMSGFn = FALSE;
#endif
		    if (bDebug)
			  printf("Delete File Success. type(%d) %s\n", nType, rcp.mMsg);
		  }
#endif
#ifdef UPDATE_20210704 //配送処理後の"$CP"が削除されずに残ラ内容にする対策
		  while(unlink(rcp.mRcp) != 0)
		  {
	        if (errno == ENOENT) //ファイルが無いとき
		      break;
			if (bDebug)
		      printf("Delete File Failed.  type(%d) %s\n", nType, rcp.mRcp);
#ifdef TRACE_MODE
            if (nSenderLog)
	        {
              sprintf(str, "Delete File Failed.  errno(%d) type(%d) %s\n", errno, nType, rcp.mRcp);
              printfTrace(str);
	        }
#endif
		     _sleep(WAIT_TIME);
		  }
		  if (bDebug)
		    printf("Delete File Success. type(%d) %s\n", nType, rcp.mRcp);
#ifdef TRACE_MODE
          if (nSenderLog)
	      {
            sprintf(str, "Delete File Success. type(%d) %s\n", nType, rcp.mRcp);
            printfTrace(str);
	      }
#endif
#else
		  if (!_unlink(rcp.mRcp)) { //if (DeleteFile(rcp.mRcp))
#ifdef UPDATE_20050122
			bRCPFn = TRUE;
#endif
			if (bDebug)
		      printf("Delete File Success. type(%d) %s\n", nType, rcp.mRcp);
		  } else {
#ifdef UPDATE_20050122
			bRCPFn = FALSE;
#endif
			if (bDebug)
		      printf("Delete File Failed.  type(%d) %s\n", nType, rcp.mRcp);
		  }
#endif
#ifdef UPDATE_20050122
/// 削除確認されるまでウエイト　rcp.mToNG, rcp.mRcp, rcp.mMsg の３ファイル
		  if (bNGFn && !bServiceTerminating) 
		  {
            while ((hF = _findfirst(rcp.mToNG, &FF)) != -1L) {
	          if (bServiceTerminating)
		        break;
              _findclose(hF);
			  _unlink(rcp.mToNG);
	          _sleep(WAIT_TIME);
			}
		  }
		  if (bRCPFn && !bServiceTerminating)
		  {
            while ((hF = _findfirst(rcp.mRcp, &FF)) != -1L) {
	          if (bServiceTerminating)
 		        break;
              _findclose(hF);
			  _unlink(rcp.mRcp);
	          _sleep(WAIT_TIME);
			}
		  }
		  if (bMSGFn && !bServiceTerminating) {
            while ((hF = _findfirst(rcp.mMsg, &FF)) != -1L) {
	          if (bServiceTerminating)
		        break;
              _findclose(hF);
			  _unlink(rcp.mMsg);
	          _sleep(WAIT_TIME);
			}
		  }
#endif
		//}
	  } 
#ifdef UPDATE_20050123
     // ハングするからFINDNEXTしないであたらしくはじめる
     // でも、送信速度が遅くなるかも？
	  if (bServiceTerminating)
		break;
      _findclose( hFindFile ); 
	  goto ReGetRCP;
#else
 	  bFile = (_findnext(hFindFile, &FindFileData) == 0 ? TRUE : FALSE); // bFile = FindNextFile( hFindFile, &FindFileData);
#endif
	}
    _findclose( hFindFile ); //FindClose( hFindFile ); 
////////////////////////////////////
#ifdef UPDATE_20050123
  } else {
    CloseHandle(hFile);
#endif
////////////////////////////////////
  }
}

void GetRetryRCPFile(char *mNS, char *mDomQ, char *mMSGDir, char *mInQ) {
#ifdef TRACE_MODE
   char str[LOG_BUFFER];
#endif
  struct _RCP rcp;
  FILE *fp;
  char mLKFile[256];
  char mFileGroup[256], mRCPSrc[256], mRCPDestA[256], mRCPDest[256], mRCPDestB[256];
  char mID[256], mMSGSrc[256], mMSGDest[256], mMri[256], *p;
  //char md[256];
  long               hF, hFindFile, hFindFile$CP, hFindFileMSG;
  struct _finddata_t FD, FindFileData, Find$CP, FindMSG;
  BOOL bFile = TRUE, bM[2];
  BOOL bMSGCpy;
  FILE *fmes;
#ifdef REGTOFILE
   HANDLE hFile;
#endif
  ////////////////////////////////////////
  sprintf(mLKFile, "%s%s", mDomQ, LOCKFILE);
#ifdef REGTOFILE
#ifdef TRACE_MODE
#ifdef UPDATE_20060904A // ロックファイルの作成確認
       if (nSenderLog || nSender2Log)
#else
       if (nSenderLog)
#endif
	   {
          sprintf(str, ":GetRetryRCPFile:CreateFile(%s)\n", mLKFile);
          printfTrace(str);
	   }
#endif
         while ((hFile = CreateFile((LPCTSTR)mLKFile,
                        GENERIC_WRITE,
                        0,   // 排他アクセス = 0
                        NULL,
                        CREATE_ALWAYS,
                        FILE_ATTRIBUTE_NORMAL, 
                        NULL)) == INVALID_HANDLE_VALUE) {
#ifdef UPDATE_20070208 // domainsフォルダ内の$LockFileを一旦クリアし対象の処理をスキップ
			{ //if (strstr(mDomQ,"\\domains")) {
              _unlink(mLKFile);
			  return;
			}
#endif
////////////////////////////////////////////////////////////////////////
// もし、Domains内フォルダのLockFileなら、これをスキップさせる処理が必要
// 2007.02.08
////////////////////////////////////////////////////////////////////////
            if (bServiceTerminating) 
              break;
            _sleep(WAIT_TIME);
		 } 
#ifdef UPDATE_20060904 // ロックファイルの作成確認
		 while ((hFindFile = _findfirst( mLKFile, &FindFileData)) == 1L) {
            if (bServiceTerminating) 
              break;
            _sleep(WAIT_TIME);
		 }
         _findclose(hFindFile);
#endif
#else
#ifdef UPDATE_20050123
  //if (ScrambleBegin(&hFile, mLKFile, FALSE)) {
#else
  if ((fp = fopen(mLKFile, "rt"))) {
	fclose(fp);
	return;
  }
  if ((fp = fopen(mLKFile, "wt")))
    fclose(fp);
  while(!(fp = fopen(mLKFile, "rt"))) { // 対象ML展開フォルダをロック
    if (bServiceTerminating)
	  break;
	  _sleep(WAIT_TIME);
  }
  fclose(fp);
#endif
#ifdef UPDATE_20050609  // ML/Domainsフォルダのアクセス競合対策（フォルダ内で先データ配送が始まってしまうため）
  if ((fp = fopen(mLKFile, "rt"))) {
	fclose(fp);
	return;
  }
  if ((fp = fopen(mLKFile, "wt")))
    fclose(fp);
  while(!(fp = fopen(mLKFile, "rt"))) { // 対象ML展開フォルダをロック
    if (bServiceTerminating)
	  break;
	  _sleep(WAIT_TIME);
  }
  fclose(fp);
#endif
#endif

  ////////////////////////////////////////
#ifdef TRACE_MODE
#ifdef UPDATE_20060904A // ロックファイルの作成確認
  if (nSenderLog || nSender2Log)
#else
  if (nSenderLog)
#endif
  {
    sprintf(str, ":GetRetryRCPFile:InitRCP(%s)\n", mLKFile);
    printfTrace(str);
  }
#endif
  InitRCP(&rcp);
  sprintf(mFileGroup, "%s*.rcp", mDomQ);
  hFindFile = _findfirst( mFileGroup, &FindFileData); //hFindFile = FindFirstFile( mFileGroup, &FindFileData);
  if (hFindFile != -1L) { //INVALID_HANDLE_VALUE) {
    while (bFile) {
	  sprintf(mRCPSrc, "%s%s", mDomQ, FindFileData.name);
	  sprintf(mRCPDestA, "%s%s", mDomQ, FindFileData.name);
	  mRCPDestA[strlen(mRCPDestA)-3] = '\x0'; 
	  strcat(mRCPDestA,"$cp");
      hF = _findfirst(mRCPDestA, &FD); //hF = FindFirstFile( mfdest, &FD);
      if (hF != -1L) { //INVALID_HANDLE_VALUE) {
		_findclose(hF); //FindClose( hF ); 
		if (!_unlink(mRCPDestA)) //DeleteFile(mfdest); 削除が成功した場合
	      X_MoveFile(mRCPSrc, mRCPDestA); //処理中にする。
	  } else {
	    X_MoveFile(mRCPSrc, mRCPDestA); //処理中にする。
	  }
      strcpy(mID, FindFileData.name);
#ifdef TRACE_MODE
#ifdef UPDATE_20060904A // ロックファイルの作成確認
      if (nSenderLog || nSender2Log)
#else
      if (nSenderLog)
#endif
	  {
        sprintf(str, ":GetRetryRCPFile:found = %s\n", mID);
        printfTrace(str);
	  }
#endif
      ///////////////////////////// 2001.12.30
      p = strrchr(mID, '.');
      if (p)
        *p = '\x0';
      /////////////////////////////////////////
	  sprintf(mRCPDest, "%s%s", mInQ, FindFileData.name);
	  mRCPDest[strlen(mRCPDest)-3] = '\x0'; 
	  strcat(mRCPDest,"$CP");
      sprintf(mMSGSrc, "%s%s.MSG", mMSGDir, mID);
      sprintf(mMSGDest,"%s%s.MSG", mInQ, mID);
	  hFindFile$CP = _findfirst(mRCPDest, &Find$CP); //hFindFile$CP =FindFirstFile(mRCPDest, &Find$CP);
      if (hFindFile$CP != -1L) { //INVALID_HANDLE_VALUE) {// incomingにRCPファイルあれば、次の処理
		_findclose(hFindFile$CP); //FindClose( hFindFile$CP );
#ifdef TRACE_MODE
        if (nSenderLog) {
          sprintf(str, ":GetRetryRCPFile:found = %s\n", mRCPDest);
          printfTrace(str);
		}
#endif
	    hFindFileMSG = _findfirst(mMSGSrc, &FindMSG); //hFindFileMSG =FindFirstFile(mMSGSrc, &FindMSG);
        if (hFindFileMSG != -1L) { //INVALID_HANDLE_VALUE) {// incomingにMSGファイルあれば、次の処理
		  _findclose(hFindFileMSG); //FindClose( hFindFileMSG ); 
  		  //DeleteFile(mMSGSrc); // 2001.12.31 incomingにあるのでholdingフォルダのMSGを削除
		} else {
		  if (X_CopyFile(mMSGDest, mMSGSrc, TRUE) ) {
		    while(!(fmes = fopen(mMSGSrc, "rt"))) { // コピー完了待ち
              if (bServiceTerminating)
		        break;
		      _sleep(WAIT_TIME);
			}
		    fclose(fmes);
			if (bDebug)
	          printf("Recover File.  GetRetryRCPFile(2) %s -> %s\n", mMSGDest, mMSGSrc);
		  }
		}
#ifdef UPDATE_20050126
        X_MoveFile(mRCPDestA, mRCPSrc); // $CPを元に戻す。
#endif
		break; //return;
      } else {
#ifdef TRACE_MODE
if (nSenderLog) {
  sprintf(str, ":GetRetryRCPFile:not found = %s\n", mRCPDest);
  printfTrace(str);
}
#endif
        if (bDebug)
		 printf("Not found executing RCP File.  GetRetryRCPFile(1) %s\n", mRCPDest);
	    hFindFileMSG = _findfirst(mMSGDest, &FindMSG); //hFindFileMSG =FindFirstFile(mMSGDest, &FindMSG);
        if (hFindFileMSG != -1L) { //INVALID_HANDLE_VALUE) {// incomingにMSGファイルあれば、次の処理
		  _findclose(hFindFileMSG); //FindClose( hFindFileMSG ); 
		  bMSGCpy = FALSE;
		} else {
#ifdef UPDATE_20071217 // domainsフォルダにRCPだけ残骸として残ってリトライ処理が進まない対策
		  if (X_MoveFile(mMSGSrc, mMSGDest) == ERROR_FILE_NOT_FOUND) { //順次送信先を移動
		    bMSGCpy = FALSE;
		  } else {
		    bMSGCpy = TRUE;
		  }
#else
		  X_MoveFile(mMSGSrc, mMSGDest); //順次送信先を移動
		  bMSGCpy = TRUE;
#endif
		}
        if (bMSGCpy) {
		  while(!(fmes = fopen(mMSGDest, "rt"))) { // コピー完了待ち
            if (bServiceTerminating)
		      break;
		    _sleep(WAIT_TIME);
		  }
		  fclose(fmes);
		  if (bDebug)
	        printf("Move File Success. GetRetryRCPFile(2) %s -> %s\n", mMSGSrc, mMSGDest);
#ifdef TRACE_MODE
          if (nSenderLog) {
            sprintf(str, ":GetRetryRCPFile:MoveFile(%s,%s) Success.\n", mMSGSrc, mMSGDest);
            printfTrace(str);
		  }
#endif
		  ///////// 移動成功時のみ処理を行う
          sprintf(mRCPDestB, "%s%s", mInQ, FindFileData.name);
#ifdef TRACE_MODE
          if (nSenderLog) {
            sprintf(str, ":GetRetryRCPFile:RestorRCPFile(%s,%s)\n", mRCPSrc, mRCPDest);
            printfTrace(str);
		  }
#endif
		  X_MoveFile(mRCPDestA, mRCPDestB); //順次送信先を移動
          ///////////////////////////////////
		} else {
		  if (bDebug)
	        printf("Move File Failed.  GetRetryRCPFile(2) %s -> %s\n", mMSGSrc, mMSGDest);
#ifdef TRACE_MODE
          if (nSenderLog) {
            sprintf(str, ":GetRetryRCPFile:MoveFile(%s,%s) Failed.\n", mMSGSrc, mMSGDest);
            printfTrace(str);
		  }
#endif
/////////////////////////// 手直し
          bM[0] = bM[1] = FALSE;
	      hFindFileMSG = _findfirst(mMSGSrc, &FindMSG); //hFindFileMSG =FindFirstFile(mMSGSrc, &FindMSG);
          if (hFindFileMSG != -1L) { //INVALID_HANDLE_VALUE) {// incomingにMSGファイルあれば、次の処理
		    bM[0] = TRUE;
		    _findclose(hFindFileMSG); //FindClose( hFindFileMSG ); 
		  }
	      hFindFileMSG = _findfirst(mMSGDest, &FindMSG); //hFindFileMSG =FindFirstFile(mMSGDest, &FindMSG);
          if (hFindFileMSG != -1L) { //INVALID_HANDLE_VALUE) {// incomingにMSGファイルあれば、次の処理
		    bM[1] = TRUE;
		    _findclose(hFindFileMSG); //FindClose( hFindFileMSG ); 
		  }
		  if (bM[0] || bM[1]) { // どちらかにMSGがある場合
		    if (!bM[0] && bM[1])
              X_CopyFile(mMSGDest, mMSGSrc, TRUE);
		    _unlink(mMSGDest); //DeleteFile(mMSGDest);
		  } else {
            _unlink(mRCPDestA); //DeleteFile(mRCPDestA);
            _unlink(mRCPSrc); //DeleteFile(mRCPSrc);
	        sprintf(mMri, "%s\\domain.mri", mDomQ);
            _unlink(mMri); //DeleteFile(mMri);
#ifdef TRACE_MODE
			if (nSenderLog) { // || nSender2Log) {
      sprintf(str, ":GetRetryRCPFile():delete %s file.\n", mMri);
      printfTrace(str);
	}
#endif
		  }
		}
	  } 
      bFile = (_findnext(hFindFile, &FindFileData) == 0 ? TRUE : FALSE); //bFile = FindNextFile( hFindFile, &FindFileData);
	}; 
    _findclose(hFindFile); //FindClose( hFindFile ); 
  }
#ifdef REGTOFILE
   CloseHandle(hFile);
   _unlink(mLKFile);
#else
#ifdef UPDATE_20050609  // ML/Domainsフォルダのアクセス競合対策（フォルダ内で先データ配送が始まってしまうため）
   _unlink(mLKFile);
#endif
#endif
#ifdef UPDATE_20050123
  //CloseHandle(hFile);
  //_unlink(mLKFile); // 削除されないタイミングがある？
  //}
#else
  _unlink(mLKFile);
#endif
}

BOOL RCPFileEmpty(char *mDomQ) {
  char *p, mFileGroup[256], mFN[2][256];
  long               hFindFile;
  struct _finddata_t FindFileData ;
  BOOL bsts = FALSE;
	  
  if (strstr(mDomQ, "\\domains"))
    sprintf(mFileGroup, "%s\\*.rcp", mDomQ);
  else
    sprintf(mFileGroup, "%s.rcp", mDomQ);

  hFindFile = _findfirst( mFileGroup, &FindFileData); //hFindFile = FindFirstFile( mFileGroup, &FindFileData);
  if (hFindFile != -1L) { //INVALID_HANDLE_VALUE) {
    _findclose(hFindFile); //FindClose( hFindFile ); 
	bsts = TRUE;
  }
#ifdef NEW_SENDER
  if (!strstr(mDomQ, "\\domains") && !bsts) {
    sprintf(mFileGroup, "%s.$cp", mDomQ);
    hFindFile = _findfirst( mFileGroup, &FindFileData); //hFindFile = FindFirstFile( mFileGroup, &FindFileData);
    if (hFindFile != -1L) { //INVALID_HANDLE_VALUE) {
      _findclose(hFindFile); //FindClose( hFindFile ); 
	  bsts = TRUE;
	}
#ifdef UPDATE_20060624 // holding内のMSGファイルもチェック
	if (!bsts) {
      strcpy(mFileGroup, mDomQ);
      if ((p = strstr(mFileGroup, "\\incoming"))) {
		strcpy(p, "\\holding");
	    if ((p = strstr(mDomQ, "\\incoming"))) {
		  strcat(mFileGroup, p + 9);
		  strcat(mFileGroup, ".msg");
          hFindFile = _findfirst( mFileGroup, &FindFileData); //hFindFile = FindFirstFile( mFileGroup, &FindFileData);
          if (hFindFile != -1L) { //INVALID_HANDLE_VALUE) {
            _findclose(hFindFile); //FindClose( hFindFile ); 
	        bsts = TRUE;
		  }
		}
	  }
	}
  } else if (!bsts) {
    sprintf(mFileGroup, "%s\\*.$cp", mDomQ);
    hFindFile = _findfirst( mFileGroup, &FindFileData); //hFindFile = FindFirstFile( mFileGroup, &FindFileData);
    if (hFindFile != -1L) { //INVALID_HANDLE_VALUE) {
      _findclose(hFindFile); //FindClose( hFindFile ); 
	  sprintf(mFN[0], "%s\\%s", mDomQ, FindFileData.name );
	  strcpy(mFN[1], mFN[0]);
	  mFN[1][strlen(mFN[1])-3] = 'r';
	  rename(mFN[0], mFN[1]);
	  bsts = TRUE;
	}
#endif
  }
#endif
  return bsts;
}

BOOL RetryEnd(char *mNS, char *mMQ, char *mDir) {
#ifdef TRACE_MODE
   char str[LOG_BUFFER];
#endif
  char mFileGroup[256], mRCPDir[256], mMri[256];
  char *p, mID[256];
  long               hFindFile;
  struct _finddata_t FindFileData ;
  BOOL bFile = TRUE;

  sprintf(mFileGroup, "%s\\%s\\*", mMailSpoolDir, mDir);

  hFindFile = _findfirst( mFileGroup, &FindFileData); //hFindFile = FindFirstFile( mFileGroup, &FindFileData);
  if (hFindFile != -1L) { //INVALID_HANDLE_VALUE) {
    while (bFile) {
      if (bServiceTerminating)
	    break;
	  if (!(strcmp(FindFileData.name,".") == 0 ||
		  strcmp(FindFileData.name,"..") == 0 ) ) {
		strcpy(mID, FindFileData.name);
        sprintf(mRCPDir, "%s\\%s\\%s", mMailSpoolDir, mDir, mID);
		if (!RCPFileEmpty(mRCPDir)) {
		  // ドメイン名の途中に"-"がある場合の対策。
		  p = strrchr(mID, '.');
		  if (p) 
			p = strchr(p, '-');
		  else 
		    p = strchr(mID, '-');
		  if (p)
			p++;
		  else
			p = mID;
		  sprintf(mRCPDir, "%s\\incoming\\%s", mMailSpoolDir, p);
	      if (!RCPFileEmpty(mRCPDir)) { // ドメインフォルダが空なら削除
	        sprintf(mMri, "%s\\%s\\%s\\domain.mri", mMailSpoolDir, mDir, mID);
            _unlink(mMri); //DeleteFile(mMri);
#ifdef TRACE_MODE
			if (nSenderLog) { // || nSender2Log) {
      sprintf(str, ":RetryEnd():delete %s file.\n", mMri);
      printfTrace(str);
	}
#endif
			sprintf(mRCPDir, "%s\\%s\\%s", mMailSpoolDir, mDir, mID);
		    RemoveDirectory(mRCPDir);
		  }
		}
	  }
      bFile = (_findnext(hFindFile, &FindFileData) == 0 ? TRUE : FALSE); //bFile = FindNextFile( hFindFile, &FindFileData);
	}; 
    _findclose(hFindFile); //FindClose( hFindFile ); 
  };

  return TRUE;
}

BOOL RetryStart(char *mNS, char *mMQ, char *mDir, BOOL bmod) {
#ifdef TRACE_MODE
   char str[LOG_BUFFER];
#endif
  char mFileGroup[256], mRCPDir[256], mMSGDir[256], mInDir[256], mLKFile[256];
  char mID[256];
  long               hFindFile, hFL;
  struct _finddata_t FindFileData, FDL;
  BOOL bFile = TRUE;
  //FILE  *fp;
#ifdef UPDATE_20050123
   HANDLE hFile;
#endif
#ifdef  INCOM_SUB_FOLDER // incoming フォルダをスレッド別フォルダにして処理
    DWORD nQueue = 0;
#endif
  ////////////////////////////////////////
#ifdef UPDATE_20050123
#ifdef UPDATE_20060905 // 対象フォルダごとにロックファイルを設定する
  if (ScrambleBegin(&hFile, (!stricmp(mDir, "domains") ? mRetryDomainLockFn : (!stricmp(mDir, "lists") ? mRetryListsLockFn : mRetryLockFn)), TRUE))
#else
  if (ScrambleBegin(&hFile, mRetryLockFn, TRUE))
#endif
#endif
  {
  //sprintf(mFileGroup, "%s\\domains\\*", mMailSpoolDir);
  //sprintf(mFileGroup, "%s\\%s\\*", mMailSpoolDir, mDir);
  sprintf(mFileGroup, "%s\\%s\\*.*", mMailSpoolDir, mDir);
  hFindFile = _findfirst( mFileGroup, &FindFileData); //hFindFile = FindFirstFile( mFileGroup, &FindFileData);
  if (hFindFile != -1L) { //INVALID_HANDLE_VALUE) {
    while (bFile) {
	  if (bServiceTerminating)
		break;
	  if (!(strcmp(FindFileData.name,".") == 0 ||
		  strcmp(FindFileData.name,"..") == 0 ) &&
		  FindFileData.attrib & _A_SUBDIR) { // サブフォルダのみ
		strcpy(mID, FindFileData.name);
#ifdef  INCOM_SUB_FOLDER // incoming フォルダをスレッド別フォルダにして処理
        sprintf(mInDir, "%s%s\\%05d\\", mMailSpoolDir, mMailQueueDir, nQueue%nMaxThread); // リトライフォルダを最後の番号にする
		nQueue++;
#else
        sprintf(mInDir, "%s%s", mMailSpoolDir, mMailQueueDir);
#endif
        sprintf(mRCPDir, "%s%s\\%s\\", mMailSpoolDir, mDir, mID);
		if (strcmp(mDir, "domains") == 0)
          sprintf(mMSGDir, "%sholding\\", mMailSpoolDir);
		else
		  strcpy(mMSGDir, mRCPDir);
#ifdef TRACE_MODE
#ifdef UPDATE_20060904A // ロックファイルの作成確認
if (nSenderLog || nSender2Log)
#else
if (nSenderLog)
#endif
{
  sprintf(str, ":GetRetryRCPFile() Start.: out=%s / inRCP=%s / inMSG=%s\n", mInDir, mRCPDir, mMSGDir);
  printfTrace(str);
}
#endif

#ifndef UPDATE_20050123
 	    //sprintf(mLKFile, "%s\\%s", mMSGDir, LOCKFILE);// 2003.09.18
		sprintf(mLKFile, "%s\\%s", mRCPDir, LOCKFILE);
        hFL = _findfirst( mLKFile, &FDL); //hFL = FindFirstFile( mLKFile, &FDL);
        if (hFL != -1L) { //INVALID_HANDLE_VALUE) {
		  _findclose(hFL); //FindClose( hFL );  // LOCKFILE があれば読み飛ばし
		} else
#endif
		{
          GetRetryRCPFile(mNS, mRCPDir, mMSGDir, mInDir);
		  if (stricmp(mDir, "domains") == 0) {
#ifndef UPDATE_20060621 // リトライ間隔のままのリトライがスレッドが生きていり間リトライされ続けてしまう対策
            Inc_MRI_TryCount(mID, mRCPDir); // MRIのカウントアップ
#endif
		  }
		}
	  }
      bFile = (_findnext(hFindFile, &FindFileData) == 0 ? TRUE : FALSE); //bFile = FindNextFile( hFindFile, &FindFileData);
	}; 
    _findclose(hFindFile); //FindClose( hFindFile ); 
  };

  if (stricmp(mDir,"domains") == 0)
    bMRICounter = TRUE;
  if (!bServiceTerminating) {
#ifndef NEW_SENDER
      if (stricmp(mDir, "domains") == 0) {
        GetRCPFile(mNS, mMQ, mMQ, (DWORD)WAIT_TIMEOUT, 2); // 送信トライ
	  }
#endif
	if (bmod) { 
	  if (!stricmp(mDir, "domains")) {
        RetryEnd(mNS, mMQ, mDir);        // フォルダ後処理
	  } else {
        RetryEnd(mNS, mMQ, mDir);        // フォルダ後処理
        RecoverListsMSGFile(mDir);
	  }
	}
  }
#ifdef UPDATE_20050123
    CloseHandle(hFile);
#endif
  }
  return TRUE;
}

BOOL SMTPDSMain(struct _SMTPDS *mSmtpds) {
#ifdef TRACE_MODE
   char str[LOG_BUFFER];
#endif

  /////////////////////////////
  if (bDebug) {
    printf("start SMTPDSMain\n");
    printf("start GetRCPFile(%s, %s) SMTPDSMain() nRunThread=%d\n", mSmtpds->mNS, mSmtpds->mMQ, nRunThread);
  }
#ifndef V4
#ifdef TRACE_MODE
    if (nSenderLog == 2) {
      sprintf(str, "[           ] start GetRCPFile(%s, %s) SMTPDSMain() nRunThread=%d\n", mSmtpds->mNS, mSmtpds->mMQ, nRunThread);
      printfTrace(str);
	}
#endif
#endif
  if (bDebug) {
    printf("Thread      = %s\n", bThread ? "TRUE":"FALSE");
    printf("ThreadRetry = %s\n", bThreadRetry ? "TRUE":"FALSE");
  }
  GetRCPFile(mSmtpds->mNS, mSmtpds->mMQ, mSmtpds->mMQ, mSmtpds->dwWait, 0); //Set Option & Send mail
  if (!bThreadRetry && !bThreadRetry2) {
	bThreadRetry = TRUE;
	if (bDebug)
	{
#ifdef  INCOM_SUB_FOLDER // incoming フォルダをスレッド別フォルダにして処理
      printf("[%05d] start RetryStart(\"lists\") bThreadRetry(1)\n", mSmtpds->dwNo);
#else
      printf("start RetryStart(\"lists\") bThreadRetry(1)\n");
#endif  INCOM_SUB_FOLDER // incoming フォルダをスレッド別フォルダにして処理
	}
#ifndef V4
#ifdef TRACE_MODE
    if (nSenderLog == 2) {
#ifdef  INCOM_SUB_FOLDER // incoming フォルダをスレッド別フォルダにして処理
      sprintf(str, "[%05d] start RetryStart(\"lists\") bThreadRetry(1)\n", mSmtpds->dwNo);
#else
      sprintf(str, "[           ] start RetryStart(\"lists\") bThreadRetry(1)\n");
#endif
      printfTrace(str);
	}
#endif
#endif
    RetryStart(mSmtpds->mNS, mSmtpds->mMQ, "lists", TRUE);    // メーリングリスト
	if (bDebug)
	{
#ifdef  INCOM_SUB_FOLDER // incoming フォルダをスレッド別フォルダにして処理
      printf("[%05d] start RetryStart(\"domains\") bThreadRetry(1)\n", mSmtpds->dwNo);
#else
      printf("start RetryStart(\"domains\") bThreadRetry(1)\n");
#endif //  INCOM_SUB_FOLDER // incoming フォルダをスレッド別フォルダにして処理
	}
#ifndef V4
#ifdef TRACE_MODE
    if (nSenderLog == 2) {
#ifdef  INCOM_SUB_FOLDER // incoming フォルダをスレッド別フォルダにして処理
      sprintf(str, "[%05d] start RetryStart(\"domains\") bThreadRetry(1)\n", mSmtpds->dwNo);
#else
      sprintf(str, "[           ] start RetryStart(\"domains\") bThreadRetry(1)\n");
#endif  //INCOM_SUB_FOLDER // incoming フォルダをスレッド別フォルダにして処理
      printfTrace(str);
	}
#endif
#endif
    RetryStart(mSmtpds->mNS, mSmtpds->mMQ, "domains", TRUE);  // ドメイン
	if (bDebug)
	{
#ifdef  INCOM_SUB_FOLDER // incoming フォルダをスレッド別フォルダにして処理
      printf("[%05d] start RetryStart(\"lists\") bThreadRetry(2)\n", mSmtpds->dwNo);
#else
      printf("start RetryStart(\"lists\") bThreadRetry(2)\n");
#endif //INCOM_SUB_FOLDER // incoming フォルダをスレッド別フォルダにして処理

	}
#ifndef V4
#ifdef TRACE_MODE
    if (nSenderLog == 2) {
#ifdef  INCOM_SUB_FOLDER // incoming フォルダをスレッド別フォルダにして処理
      sprintf("[%05d] start RetryStart(\"lists\") bThreadRetry(2)\n", mSmtpds->dwNo);
#else
      sprintf(str, "[           ] start RetryStart(\"lists\") bThreadRetry(2)\n");
#endif  //INCOM_SUB_FOLDER // incoming フォルダをスレッド別フォルダにして処理
      printfTrace(str);
	}
#endif
#endif
    RetryStart(mSmtpds->mNS, mSmtpds->mMQ, "lists", TRUE);    // メーリングリスト
    if (bThreadType) {
	  if (bDebug)
        printf("start GetRCPFile(%s, %s, %s)\n", mSmtpds->mNS, mSmtpds->mMQ, mSmtpds->mMQ);
      GetRCPFile(mSmtpds->mNS, mSmtpds->mMQ, mSmtpds->mMQ, (DWORD)WAIT_TIMEOUT, 1);    // メーリングリスト再処理
	}
    bThreadRetry = FALSE;
  } else if (!bThreadRetry2) {
	bThreadRetry2 = TRUE;
	if (bDebug)
	{
#ifdef  INCOM_SUB_FOLDER // incoming フォルダをスレッド別フォルダにして処理
      printf("[%05d] start RetryStart(\"lists\") bThreadRetry2(1)\n", mSmtpds->dwNo);
#else
      printf("start RetryStart(\"lists\") bThreadRetry2(1)\n");
#endif //INCOM_SUB_FOLDER // incoming フォルダをスレッド別フォルダにして処理
	}
#ifndef V4
#ifdef TRACE_MODE
    if (nSenderLog == 2) {
#ifdef  INCOM_SUB_FOLDER // incoming フォルダをスレッド別フォルダにして処理
      sprintf(str, "[%05d] start RetryStart(\"lists\") bThreadRetry2(1)\n", mSmtpds->dwNo);
#else
      sprintf(str, "[           ] start RetryStart(\"lists\") bThreadRetry2(1)\n");
#endif //INCOM_SUB_FOLDER // incoming フォルダをスレッド別フォルダにして処理
      printfTrace(str);
	}
#endif
#endif
    RetryStart(mSmtpds->mNS, mSmtpds->mMQ, "lists", TRUE);    // メーリングリスト
	if (bDebug)
	{
#ifdef  INCOM_SUB_FOLDER // incoming フォルダをスレッド別フォルダにして処理
      printf("[%05d] start RetryStart(\"domains\") bThreadRetry2(1)\n", mSmtpds->dwNo);
#else
      printf("start RetryStart(\"domains\") bThreadRetry2(1)\n");
#endif  //INCOM_SUB_FOLDER // incoming フォルダをスレッド別フォルダにして処理
	}
#ifndef V4
#ifdef TRACE_MODE
    if (nSenderLog == 2) {
#ifdef  INCOM_SUB_FOLDER // incoming フォルダをスレッド別フォルダにして処理
      sprintf(str, "[%05d] start RetryStart(\"domains\") bThreadRetry2(1)\n", mSmtpds->dwNo);
#else
      sprintf(str, "[           ] start RetryStart(\"domains\") bThreadRetry2(1)\n");
#endif  //INCOM_SUB_FOLDER // incoming フォルダをスレッド別フォルダにして処理
      printfTrace(str);
	}
#endif
#endif
    RetryStart(mSmtpds->mNS, mSmtpds->mMQ, "domains", FALSE);  // ドメイン
    bThreadRetry2 = FALSE;
  }
//FIN:
  if (bDebug)
    printf("end SMTPDSMain\n");
  if (bThreadType) {
	if (nRunThread > 0)
      nRunThread--;
	if (bDebug) printf("SMTPDSMain() in nRunThread = %d\n", nRunThread);
    _endthread();
  }
  return TRUE;
}

BOOL SMTPDSDomains(struct _SMTPDS *mSmtpds) {
#ifdef TRACE_MODE
   char str[LOG_BUFFER];
#endif

  if (!bThreadDomain) {
    bThreadDomain = TRUE;
	if (bDebug) {
#ifdef  INCOM_SUB_FOLDER // incoming フォルダをスレッド別フォルダにして処理
      printf("[%05d] start SMTPDSDomains\n", mSmtpds->dwNo); 
      printf("[%05d] start RetryStart(\"domains\", %s, %s) SMTPDSDomains()\n", mSmtpds->dwNo, mSmtpds->mNS, mSmtpds->mMQ);
#else
      printf("start SMTPDSDomains\n"); 
      printf("start RetryStart(\"domains\", %s, %s) SMTPDSDomains()\n", mSmtpds->mNS, mSmtpds->mMQ);
#endif  //  INCOM_SUB_FOLDER // incoming フォルダをスレッド別フォルダにして処理
	}
#ifndef V4
#ifdef TRACE_MODE
    if (nSenderLog == 2) {
#ifdef  INCOM_SUB_FOLDER // incoming フォルダをスレッド別フォルダにして処理
      sprintf(str, "[%05d] start RetryStart(\"domains\", %s, %s) SMTPDSDomains()\n", mSmtpds->dwNo, mSmtpds->mNS, mSmtpds->mMQ);
#else
      sprintf(str, "[           ] start RetryStart(\"domains\", %s, %s) SMTPDSDomains()\n", mSmtpds->mNS, mSmtpds->mMQ);
#endif  //  INCOM_SUB_FOLDER // incoming フォルダをスレッド別フォルダにして処理
      printfTrace(str);
	}
#endif
#endif
    RetryStart(mSmtpds->mNS, mSmtpds->mMQ, "domains", TRUE);  // ドメイン
	if (bDebug)
	{
#ifdef  INCOM_SUB_FOLDER // incoming フォルダをスレッド別フォルダにして処理
      printf(str, "[%05d] end SMTPDSDomains\n", mSmtpds->dwNo);
#else
      printf("end SMTPDSDomains\n");
#endif //INCOM_SUB_FOLDER // incoming フォルダをスレッド別フォルダにして処理
	}
	bThreadDomain = FALSE;
  }
  return TRUE;
}

BOOL SMTPDSLists(struct _SMTPDS *mSmtpds) {
#ifdef TRACE_MODE
   char str[LOG_BUFFER];
#endif

#ifdef TRACE_MODE
#ifdef UPDATE_20060904A // ロックファイルの作成確認ログ出力用
   if (nSenderLog || nSender2Log)
#else
   if (nSenderLog == 2)
#endif
   {
#ifdef  INCOM_SUB_FOLDER // incoming フォルダをスレッド別フォルダにして処理
     sprintf(str, "[%05d] SMTPDSLists() bThreadLists = %d / %s\n", mSmtpds->dwNo, bThreadLists, mSmtpds->mMQ);
#else
     sprintf(str, "[           ] SMTPDSLists() bThreadLists = %d / %s\n", bThreadLists, mSmtpds->mMQ);
#endif //INCOM_SUB_FOLDER // incoming フォルダをスレッド別フォルダにして処理
     printfTrace(str);
   }
#endif

#ifdef UPDATE_20060904B // bThreadLists = TRUE;が利いたままになる
//   {
#else
   if (!bThreadLists)
#endif
   {
	 bThreadLists = TRUE;
	 if (bDebug) {
#ifdef  INCOM_SUB_FOLDER // incoming フォルダをスレッド別フォルダにして処理
       printf("[%05d] start SMTPDSLists\n", mSmtpds->dwNo );
       printf("[%05d] start RetryStart(\"lists\", %s, %s) SMTPDSDomains()\n", mSmtpds->dwNo, mSmtpds->mNS, mSmtpds->mMQ);
#else
       printf("start SMTPDSLists\n");
       printf("start RetryStart(\"lists\", %s ,%s) SMTPDSLists()\n", mSmtpds->mNS, mSmtpds->mMQ);
#endif  //INCOM_SUB_FOLDER // incoming フォルダをスレッド別フォルダにして処理
	 }
#ifndef V4
#ifdef TRACE_MODE
     if (nSenderLog == 2) {
#ifdef  INCOM_SUB_FOLDER // incoming フォルダをスレッド別フォルダにして処理
       sprintf(str, "[%05d] start RetryStart(%s, %s, \"lists\", TRUE) SMTPDSLists()\n", mSmtpds->dwNo, mSmtpds->mNS, mSmtpds->mMQ);
#else
       sprintf(str, "[           ] start RetryStart(%s, %s, \"lists\", TRUE) SMTPDSLists()\n", mSmtpds->mNS, mSmtpds->mMQ);
#endif
       printfTrace(str);
	 }
#endif
#endif
     RetryStart(mSmtpds->mNS, mSmtpds->mMQ, "lists", TRUE);    // メーリングリスト
	 if (bDebug)
	 {
#ifdef  INCOM_SUB_FOLDER // incoming フォルダをスレッド別フォルダにして処理
       printf("[%05d] end SMTPDSLists\n", mSmtpds->dwNo );
#else
       printf("end SMTPDSLists\n");
#endif  //INCOM_SUB_FOLDER // incoming フォルダをスレッド別フォルダにして処理
	 }
	 bThreadLists = FALSE;
  }
  return TRUE;
}

BOOL SMTPDSIncoming(struct _SMTPDS *mSmtpds) {
#ifdef TRACE_MODE
   char str[LOG_BUFFER];
#endif

#ifdef UPDATE_20091225 // スレッド数を超えて起動してしまう不具合
   if (nRunThread >= nMaxThread)
   {
     if (nSenderLog || nSender2Log)
	 {
       sprintf(str, "SMTPDSIncoming() nRunThread=%d / MaxThread=%d / thread over.\n", nRunThread, nMaxThread);
       printfTrace(str);
	 }
     nRunThread--;
	 return TRUE;
   }
#endif
  if (bDebug) {
	printf("start SMTPDSIncoming\n");
    printf("start GetRCPFile(%s, %s) SMTPDSIncoming() nRunThread=%d\n", mSmtpds->mNS, mSmtpds->mMQ, nRunThread);
  }
#ifndef V4
#ifdef TRACE_MODE
    if (nSenderLog == 2) {
      sprintf(str, "[           ] start GetRCPFile(%s, %s) SMTPDSIncoming() nRunThread=%d\n", mSmtpds->mNS, mSmtpds->mMQ, nRunThread);
      printfTrace(str);
	}
#endif
#endif
  GetRCPFile(mSmtpds->mNS, mSmtpds->mMQ, mSmtpds->mMQ, mSmtpds->dwWait, 0); //Set Option & Send mail
  if (bDebug)
    printf("end SMTPDSIncoming\n");
//#ifndef UPDATE_20051115 //ここでスレッドダウンしない
  if (nRunThread > 0)
    nRunThread--;
  if (bDebug) printf("SMTPDSIncoming() in nRunThread = %d\n", nRunThread);
//#endif
  return TRUE;
}


#ifdef UPDATE_20050123
BOOL ScrambleBegin(HANDLE *hFile, char *pFn, BOOL bType) {
#ifdef TRACE_MODE
   char str[LOG_BUFFER];
#endif
   DWORD n = 0;
   
#ifdef TRACE_MODE
#ifdef UPDATE_20060904A // ロックファイルの作成確認
if (nSenderLog || nSender2Log)
#else
if (nSenderLog) 
#endif
{
  sprintf(str, "[           ] ScrambleBegin(%s) start.\n", pFn);
  printfTrace(str);
}
#endif
   while ((*hFile = CreateFile( (LPCTSTR)pFn, //mThreadLockFn,
                        GENERIC_WRITE,
                        0,   // 排他アクセス = 0
                        NULL,
                        CREATE_ALWAYS,
                        FILE_ATTRIBUTE_NORMAL, 
                        NULL)) == INVALID_HANDLE_VALUE) {
	 if (bType)
	   return FALSE;
     if (bServiceTerminating)
       return FALSE;
  	 if (nGetRCPWait > 0 && n++ >= nGetRCPWait)
	   return FALSE;
#ifdef TRACE_MODE
     if (nSenderLog) {
       sprintf(str, "[           ] ScrambleBegin(%s) wait.\n", pFn);
       printfTrace(str);
	 }
#endif
	 _sleep(WAIT_TIME);
   } 
#ifdef TRACE_MODE
#ifdef UPDATE_20060904A // ロックファイルの作成確認
   if (nSenderLog || nSender2Log)
#else
   if (nSenderLog)
#endif
   {
     sprintf(str, "[           ] ScrambleBegin(%s) Success.\n", pFn);
     printfTrace(str);
   }
#endif
   return TRUE;
}

#endif

BOOL SMTPDSIncomingA(struct _SMTPDS *mSmtpds) {
#ifdef UPDATE_20050123
   HANDLE hFile, hFile2;
#endif
#ifdef TRACE_MODE
   char str[LOG_BUFFER];
#endif
   DWORD  nNum, nCnt;

#ifdef UPDATE_20091225 // スレッド数を超えて起動してしまう不具合
   if (nRunThread >= nMaxThread)
   {
     if (nSenderLog || nSender2Log)
	 {
       sprintf(str, "SMTPDSIncomingA(1) nRunThread=%d / MaxThread=%d / thread over.\n", nRunThread, nMaxThread);
       printfTrace(str);
	 }
	 nRunThread--;
     return TRUE;
   }
#endif

  if (bDebug) {
#ifdef  INCOM_SUB_FOLDER // incoming フォルダをスレッド別フォルダにして処理
    printf("[%05d] start SMTPDSIncomingA\n", mSmtpds->dwNo);
	printf("[%05d] start GetRCPFile(%s, %s) SMTPDSIncomingA() nRunThread=%d\n", mSmtpds->dwNo, mSmtpds->mNS, mSmtpds->mMQ);
#else
    printf("start SMTPDSIncomingA\n");
    printf("start GetRCPFile(%s, %s) SMTPDSIncomingA() nRunThread=%d\n", mSmtpds->mNS, mSmtpds->mMQ, nRunThread);
#endif
  }
#ifndef V4
#ifdef TRACE_MODE
    if (nSenderLog == 2) {
#ifdef  INCOM_SUB_FOLDER // incoming フォルダをスレッド別フォルダにして処理
      sprintf(str, "[           ] [%05d] start GetRCPFile(%s, %s) SMTPDSIncomingA()\n", mSmtpds->dwNo, mSmtpds->mNS, mSmtpds->mMQ);
#else
      sprintf(str, "[           ] start GetRCPFile(%s, %s) SMTPDSIncomingA() nRunThread=%d\n", mSmtpds->mNS, mSmtpds->mMQ, nRunThread);
#endif
      printfTrace(str);
	}
#endif
#endif
///////////////////////////////
#ifndef  INCOM_SUB_FOLDER // incoming フォルダをスレッド別フォルダにして処理
#ifdef UPDATE_20050123
   if (ScrambleBegin(&hFile, mThreadLockFn, FALSE)) {
#endif
     nNum = NumberRCPFile(mSmtpds->mMQ);
     if (nNum > 0) {
       if (bThreadType) {
         //nCnt = __min(nMaxThread , nRunThread+nNum);
         //while (nRunThread < nCnt && nNum) { //nMaxThread && nNum) { //RCPファイルがある場合スレッド開始 //do {
         while (nRunThread < nMaxThread) { //RCPファイルがある場合スレッド開始 //do {
	       nRunThread++;
	       mSmtpds->dwNo = nRunThread;
#ifdef TRACE_MODE
           if (nSenderLog == 2) {
             sprintf(str, "[           ] start _beginthread() nRunThread=%d\n", nRunThread);
             printfTrace(str);
		   }
#endif
#ifdef UPDATE_20091225A // スレッド作成エラーに対する対策
		   if (_beginthread( SMTPDSIncoming, sizeof(struct _SMTPDS) , mSmtpds) == -1L)
		   {
			 nRunThread++;
			 return TRUE;
		   }
#else
	       _beginthread( SMTPDSIncoming, sizeof(struct _SMTPDS) , mSmtpds);
#endif
		 }
	   }
	 }
#ifdef UPDATE_20050123
     CloseHandle(hFile);
   }
#endif
#endif // INCOM_SUB_FOLDER // incoming フォルダをスレッド別フォルダにして処理
#ifdef UPDATE_20091225 // スレッド数を超えて起動してしまう不具合
   if (nRunThread >= nMaxThread)
   {
     if (nSenderLog || nSender2Log)
	 {
       sprintf(str, "SMTPDSIncomingA(2) nRunThread=%d / MaxThread=%d / thread over.\n", nRunThread, nMaxThread);
       printfTrace(str);
	 }
	 nRunThread--;
     return TRUE;
   }
#endif
   GetRCPFile(mSmtpds->mNS, mSmtpds->mMQ, mSmtpds->mMQ, mSmtpds->dwWait, 0); //Set Option & Send mail
#ifdef UPDATE_20050123
   if (ScrambleBegin(&hFile, mListsLockFn, TRUE)) {
	 if (ScrambleBegin(&hFile2, mArticleLockFn, TRUE)) {
#endif
       SMTPDSLists(mSmtpds);
#ifdef UPDATE_20050123
       CloseHandle(hFile2);
	 }
     CloseHandle(hFile);
   }
#endif

#ifdef UPDATE_20050123
   if (ScrambleBegin(&hFile, mDomainsLockFn, TRUE)) {
#endif
     SMTPDSDomains(mSmtpds);
#ifdef UPDATE_20050123
     CloseHandle(hFile);
   }
#endif
///////////////////////////////
#ifdef  INCOM_SUB_FOLDER // incoming フォルダをスレッド別フォルダにして処理
   GetRCPFile(mSmtpds->mNS, mSmtpds->mMQ, mSmtpds->mMQ, mSmtpds->dwWait, 0); //Set Option & Send mail
#else
#ifdef UPDATE_20050123
   if (ScrambleBegin(&hFile, mThreadLockFn, FALSE)) {
#endif
/////////////////////////////////////////
      nNum = NumberRCPFile(mSmtpds->mMQ);
      if (nNum > 0) {
        if (bThreadType) {
  	      nCnt = __min(nMaxThread , nRunThread+nNum);
	      //while (nRunThread < nCnt && nNum) { //nMaxThread && nNum) { //RCPファイルがある場合スレッド開始 //do {
	      while (nRunThread < nMaxThread) { //RCPファイルがある場合スレッド開始 //do {
	        nRunThread++;
	        mSmtpds->dwNo = nRunThread;
#ifdef TRACE_MODE
            if (nSenderLog == 2) {
              sprintf(str, "[           ] start _beginthread() nRunThread=%d\n", nRunThread);
              printfTrace(str);
			}
#endif
	        _beginthread( SMTPDSIncoming, sizeof(struct _SMTPDS) , mSmtpds);
		  } // while (nCnt < (nNum <= nMaxThread ? (nNum == 0 ? 1: nNum+1) : nMaxThread));
		} else {
          SMTPDSIncoming(mSmtpds);
		}
	  }
//Aend:
      if (bDebug)
        printf("end SMTPDSIncomingA\n");
      if (nRunThread > 0)
        nRunThread--;
      if (bDebug) printf("0:SMTPDSIncomingA() in nRunThread = %d\n", nRunThread);
///////////////////////////
#ifdef UPDATE_20050123
      CloseHandle(hFile);
   } else {
#ifdef UPDATE_20050623
   if (nRunThread > 0)
      nRunThread--;
   if (bDebug) printf("1:SMTPDSIncoming() in nRunThread = %d\n", nRunThread);
#endif
   }
#endif
#endif //INCOM_SUB_FOLDER // incoming フォルダをスレッド別フォルダにして処理

  return TRUE;
}

#ifdef LGWAN_ON        // LGWAN機能拡張
BOOL IP_COMP(char *pRange, char *pAddr) {
  char *pAster, *pmask;
  int  i, n, mask, num, addr, start, dot;
  BOOL bsts = FALSE;

  num = 1; // 比較アドレス数
  if ((pAster = strchr(pRange, '*'))) { // ワイルドカードでのマスク
	*pAster = '\x0';
  } else if (strchr(pRange, '.')) { // IPv4のみ
    if ((pAster = strchr(pRange, '/'))) {// ネットマスク
	  *pAster = '\x0';
	  mask = 32 - atoi(++pAster);
	  num = 1 << mask; // 2のべき乗範囲 num <<= mask; // 2のべき乗範囲
	  dot = ((mask-1) / 8) + 1;
	  if (dot > 1)
	    addr = num / (256*(dot-1));
	  else
	    addr = num;
	  for (i = 1; i <= dot; i++) {
	    if ((pAster = strrchr(pRange, '.')))
		  *pAster = '\x0';
	  }
  	  start = atoi(pAster + 1);
	}
  }
#ifdef UPDATE_20211210_IPV6PREFIX // IPv6アドレスプレフィックスに対応した。
  else if (strchr(pRange, ':')) { // IPv6
     if ((pAster = strchr(pRange, '/'))) {// ネットマスク
       num = 0;
	   *pAster = '\x0';
	   bsts = Compear_IPv6_Addr(pAddr, pRange, pAster);
    }
  }
#endif
  if (num == 1) {
    if (pAster && !strncmp(pAddr, pRange, strlen(pRange)) || //ワイルドカード有り
		!pAster && !strcmp(pAddr, pRange))   // ワイルドカードなし
	{
      bsts = TRUE;  // 一致するものがあった
	}
  } else { // ネットマスクの場合
	for ( i = 0; i < addr; i++) {
	  pAster = '\x0';
	  pmask = (pRange+strlen(pRange));
	  sprintf(pmask, ".%d", start + i);
      if (!strncmp(pAddr, pRange, strlen(pRange))) {
	    bsts = TRUE;  // 一致するものがあった
	  }
	}
  }
  return bsts;
}

#ifdef UPDATE_20241217_RCP // gateway.datにSMTP認証の指定が別のセッションで上書きされない対策
void LGWANControl(CHAR *pFrom, CHAR *pTo, int *nGateAuthType) //CHAR *pMSG, CHAR *pRCP)
#else
void LGWANControl(CHAR *pFrom, CHAR *pTo) //CHAR *pMSG, CHAR *pRCP)
#endif
{  
  BOOL bLocal, bIn = FALSE;
  BOOL bSSL = FALSE, bAliases /* = FALSE*/, bAls, bSet, bHit = FALSE, bReWrite = FALSE;  // FROMを書き換え
  INT  nPort = 25;
  CHAR *pdom, *prcptdom, mOpt[128], mGate[128], mUser[512], mRCPT[512], mData[512];

  if (bLGWAN) {
    bReWrite = FALSE;  // MAIL FROMを書き換え
	bHit = FALSE;
	bAliases = FALSE;
    strcpy(mUser, pFrom); //&mData[13]);
    if ((pdom = strstr(mUser, "@")))
	{
	  *pdom = '\x0';
      bAliases = GetAliases((LPCTSTR)SOFT_ALIASES_REG, (char *)mUser, (char *)pdom, mOpt);
	}
    if (bAliases && mOpt[1] == ',')
    {
	  strcpy(mRCPT, pTo);
	  strcpy(mData, pTo);
      if ((prcptdom = strstr(mRCPT, "@")))
	  {
	    *prcptdom = '\x0';
	    bAls = GetAliases((LPCTSTR)SOFT_ALIASES_REG, (char *)mRCPT, (char *)prcptdom, NULL);
	  }
	  if (!bAls || (bAls && !CheckDomain(mRCPT))) //グローバルドメインであることを確認
	  {
        if ((pdom = strstr(mData, "@")))
		{
	      *pdom = '\x0';
		  pdom++;
#ifdef UPDATE_20241217_RCP // gateway.datにSMTP認証の指定が別のセッションで上書きされない対策
          if (GetGatewayList(pdom, pFrom, pTo, mGate, &nPort, &bSSL, nGateAuthType))
#else
#ifdef UPDATE_20241128 // GATEWAY.DATにエンベロープの送信元の条件も追加可能に拡張した。
          if (GetGatewayList(pdom, pFrom, pTo, mGate, &nPort, &bSSL))
#else
#ifdef UPDATE_20141023 // メールアドレス単位でもGatway.datでフォワード指定可能にする対策
          if (GetGatewayList(pdom, pTo, mGate, &nPort, &bSSL))
#else
          if (GetGatewayList(pdom, mGate, &nPort, &bSSL))
#endif
#endif
#endif
		  {
		    if (IP_COMP(&mOpt[2], mGate))  // IPアドレスを確認
			{
              bHit = TRUE;  // MAIL FROMを書き換え
			}
          }
		}
	  }
	}
	else 
	{
#ifdef UPDATE_20080618 // LGWAN機能が有効なとき、ドメインなしアカウントがあるとハングする不具合
	  if (pdom)
#endif
	  {  // NULLでないとき 20080618
	    *pdom = '@';
	  }
      if (!CheckDomain(mUser))
	  {
	    strcpy(mRCPT, pTo);
		strcpy(mData, pTo);
        if ((prcptdom = strstr(mRCPT, "@")))
		{
	      *prcptdom = '\x0';
	      bAls = GetAliases((LPCTSTR)SOFT_ALIASES_REG, (char *)mRCPT, (char *)prcptdom, (char *)mOpt);
		}
	    if (bAls && CheckDomain(mRCPT)) //ローカルドメインであることを確認
		{
	      if (mOpt[0] == '0' && mOpt[1] == ',')
		  {
            bIn = bReWrite = TRUE;  // RCPT TOを書き換え
		  }
		}
	  }
	}
  }
  if (bAliases)
  {
	if (mOpt[0] == '0') 
	{
      bReWrite = !bHit;  // RCPT TOを書き換え
	} else
	if (mOpt[0] == '2')
	{
      bReWrite = bHit;  // MAIL FROMを書き換え
	}
  }
  if (bReWrite) // MAIL FROMを書換え
  {
	strcpy(pFrom, mUser);
	bLocal = CheckDomain(mUser);
    if (bLocal || !bLocal && bIn) {
  	  strcpy(mRCPT, pTo);
      if ((prcptdom = strstr(mRCPT, "@")))
	  {
	    *prcptdom = '\x0';
	    bAls = GetAliases((LPCTSTR)SOFT_ALIASES_REG, (char *)mRCPT, (char *)prcptdom, mOpt);
	    if (bAls && (mOpt[0] == '0') && CheckDomain(mRCPT)) //グローバルドメインであることを確認
		{
		  strcpy(pTo, mRCPT);
		}
	  }
	}
  }
}

#endif

