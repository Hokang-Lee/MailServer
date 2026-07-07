////////////////////////////////////////////////////////////
// smtpds.c Copyright K.kawakami
////////////////////////////////////////////////////////////
#include "smtp.h"
#include "version.h"
#include "service.h"
#include <time.h>
#include <tchar.h>
#include <direct.h>
#include <process.h>

//#define WAIT_TIME      10
extern BOOL    bDebug;

BOOL   bOldRetryMode; //４００番台と５００番台リトライをシンプルにする。
BOOL   bSaveDead; // deadフォルダに保管しないオプション
BOOL   bCompl;// 強制吐き出しオプション
BOOL   bVLog; // イベントビューワにログ書込みエラーを表示する 0:しない　1:する

#ifdef CLUSTERING
BOOL   nClustering;
char   mComName[256];
char   mLogicalIP[256]; // 論理IP
#endif

#ifdef SENDERID
BOOL   bSenderID;
#endif

HANDLE  hServerStopEvent = NULL;

#ifdef UPDATE_20050121
void   StopLog(DWORD nCode);
#endif
BOOL   SMTPDSMain(struct _SMTPDS *mSmtpds);
BOOL   SMTPDSIncomingA(struct _SMTPDS *mSmtpds);
BOOL   SMTPDSIncoming(struct _SMTPDS *mSmtpds);
BOOL   SMTPDSDomains(struct _SMTPDS *mSmtpds);
BOOL   SMTPDSLists(struct _SMTPDS *mSmtpds);
//BOOL   SMTPDSMain(DWORD status, char *mNS, char *mMQ);
void   RecoverDomainMSGFile(char *pBoxDir);
void   RecoverRCPFile(char *pBoxDir);
void   RecoverFolder(char *mDir);
void   RecoverOKNGFile(char *pBoxDir);
DWORD  NumberRCPFile(char *pBoxDir);
//BOOL   AcceptClients (HANDLE hCompletionPort);
//void   SMTPRrvMain(int argc, char *argv[]);
//HANDLE InitializeThreads (void);
#ifdef Y2038_BUG
BOOL    GetTimeLimit(__int64 *ltbegin, char *mode, BOOL *bPassport);
BOOL    ReSetTimeLimit(__int64 *ltbegin);
#else
BOOL    GetTimeLimit(time_t *ltbegin, char *mode, BOOL *bPassport);
BOOL    ReSetTimeLimit(time_t *ltbegin);
#endif
#ifdef CLUSTERING
BOOL HotStandby(char *pMacine, char *pLogicalIP);
#endif;
#ifdef UPDATE_20140711 // エラーメール生成時の"multipart/report"に対応するオプションを追加した。
BOOL gethostaddrname(char *hostname);
#endif

#ifdef Y2038_BUG
char mMonth[12][4] = {"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};
char mWeek[7][4]   = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
#endif
#ifdef UPDATE_20230105 // X-UIDLヘッダを追加するオプション
BOOL    bADDXUIDL; // X-UIDLヘッダを追加するか否か
#endif
BOOL    bMTAIP;             // MTAのIPを送信時設定するか否か
DWORD   nGetRCPWait;        // 処理スレッドがRCPファイル取得時にどれだけ待つか
int     nTMOut;
DWORD   nSendRefusalTime;   // 受信拒絶時のトライ回数
DWORD   nSendOtherTime;     // 無通信時のトライ（時間）
DWORD   nSendOther;         // 無通信時のトライ回数
DWORD   nSendMaxRetryTime;  // 相手無応答時のトライ回数
DWORD   nSendMaxRetry;      // 送信トライ最大回数(時間)
BOOL    bTMQWait;
DWORD   nThreadNo;
ULONG   hTd;
BOOL    bTd;
int     nAddressFamily;
#ifdef ADD_IDCACHE
int     nIDCashLiveTime; // ADキャッシュ利用有効時間(秒単位)
#endif
#ifdef V4
int     nMXCashLiveTime; // MXキャッシュ利用有効時間(秒単位)
#endif
#ifdef V4
INT     nMAXUser;
#endif
#ifdef LGWAN_ON        // LGWAN機能拡張
BOOL   bLGWAN;
BOOL   bCHGHEADER;
#endif
#ifdef LDAP_ON         // LDAPへのログイン処理でSMTP認証の確認を行うオプション // //CRAM-MD5は複合できないので、これを利用する場合は、PLAINかLOGINのみとします。
BOOL   bLDAPOn;
BOOL   bLDAP;
DWORD  nLDAPPort;
char   mLDAPAdminID[256];
char   mLDAPAdminPW[256];
char   mLDAPSchemaRDN[256];
char   mLDAPSchemaDC[256];
char   mLDAPSchemaFilter[256];
char   mLDAPSchemaID[256];
char   mLDAPSchemaHome[256];
char   mLDAPSchemaName[256];
char   mLDAPSchemaMember[256];
#endif
#ifdef V3
BOOL    bUserMan;
BOOL    bInboxEnc;
#endif
BOOL    bRetryRule;
int     nSendBuf;
DWORD   nSendTMO;
DWORD   nRecvTMO;
DWORD   nGreetingTMO;
BOOL    bOverlapFile;
BOOL    bWaitDNS;
BOOL    bMRICounter;
BOOL    bOutlog;
BOOL    bOutLocallog;
BOOL    bFailOutlog;
BOOL    bSenderlog;
BOOL    bThread;
BOOL    bThreadRetry;
BOOL    bThreadRetry2;
BOOL    bThreadDomain;
BOOL    bThreadLists;
int     nChkTM;
int     nport;
BOOL    bLog;
BOOL    bSendLocalLog;
BOOL    bFailLog;
int     nSenderLog;
#ifdef UPDATE_20060613 // senderlogでプロトコルログ のみ出力するオプション追加
int     nSender2Log;
int     nSenderlogUnit; // ログするファイルの単位指定 0:１日 1:１時間
#endif
BOOL    bSendPostMaster;
BOOL    bAutoCreateInbox;
BOOL    bThreadType;
DWORD   nFolderWoker;
DWORD   nMaxDivide;         // ＭＬアドレスを指定数のメールとしてふりわける。起動スレッド数とは別。
DWORD   nDomainDivide;      // ＭＬアドレスのドメインでの分割単位。
DWORD   nMaxThread;
DWORD   nRunThread;
DWORD   nMailInMaxSize;
DWORD   nMailInBoxSize;
char    mAuthDomain[64];
char    mProgPath[256];
char    mdomain[256];
char    mMailGroup[128];
char    mRCPLockFn[256];
char    mRetryLockFn[256];
char    mRetryDomainLockFn[256];
char    mRetryListsLockFn[256];
char    mThreadLockFn[256];
char    mDomainsLockFn[256];
char    mListsLockFn[256];
char    mArticleLockFn[256];
#ifdef UPDATE_20070321 //DNSへの問合せテーブル初期化(初期化が無いと問合せで不定期にハングする可能性有り)
char    mDNSMXLockFn[256];
#endif
BOOL    bCountLock;
BOOL    bCloseLock;
DWORD   nLastMsgId;
BOOL    bCheckLocalDomainIP;
char    mLocalDomain[0x4000]; // レジストリ：バイナリの限界値 [4096];
DWORD   nLocalDomain;
char    mMailInCopy[4096];
DWORD   nMailInCopy;
char    mMailSpoolDir[128];
char    mMailQueueDir[128];
CHAR    mMailBoxDir[_MAX_PATH];
char    mMailHoldDir[128];
char    mMailDomainDir[128];
char    mLockFile[128];
char    mTempDir[128];
BOOL    bTrace;
char    mTraceFile[256];
FILE    *fTrace;
BOOL    bVirus;
char    mVirusFile[256];
DWORD   nReceived;
BOOL    bAnnounce;
BOOL    bAnnLimit;
DWORD   nAnnounceMax;
#ifdef Y2038_BUG
__int64 ltbegin;
#else
time_t  ltbegin;
#endif
char    mDefaultCode[32];
char    mPostmaster[256];
char    mSendGateway[256];
#ifdef UPDATE_20230310 // SMTPゲートウェイ欄にゲートウェイ先明記ならセッションを分離しない
BOOL    bIncludeForward;
#endif
BOOL    bMailForward;
#ifdef UPDATE_20141105 // MXレコード参照失敗した受信をSMTP Gatewayへ転送
BOOL    bMailForward2;
#endif
char    mGetRCP[256];
char    mWaxPipe[256];
char    mSMTPAUTHID[64];
#ifdef UPDATE_20210127 // SMTP認証時のパスワード領域上限数を、63byteから127byteに拡張した。(SendGrid SMTP認証対策)
char    mSMTPAUTHPASS[128];
#else
char    mSMTPAUTHPASS[64];
#endif
char    mSMTPAUTHMODE[64];
char    mPasswordFile[128];
#ifdef ADD_XOAUTH2 // OAUTH2での認証方式を追加
BOOL    bAUTHTYPE[4]; // 0=CRAM-MD5,1=LOGIN,2=PLAIN,3=XOAUTH2
#else
BOOL    bAUTHTYPE[3]; // 0=CRAM-MD5,1=LOGIN,2=PLAIN
#endif
#ifdef UPDATE_20241217 // gateway.datにSMTP認証の指定がある時は優先するようにした。
BOOL    bGWAUTHTYPE[4]; // gateway.datでのSMTP認証情報
#endif
BOOL    bESMTP;
BOOL    bServerType;
#ifdef USE_SSL
#ifdef USE_STARTTLS
BOOL    bUsedSTLS;
#endif
BOOL    bUsedSSL;
#endif
int     nReadyDriveTime;
SOCKET  sListener;
HANDLE  hCompletionPort;
WSADATA WsaData;
BOOL    bServiceTerminating; // = FALSE;
LCID    nLCID;
#ifdef UPDATE_20050901
#ifdef UPDATE_20140711 // エラーメール生成時の"multipart/report"に対応するオプションを追加した。
int     nReturnMailForm;
#else
BOOL    bReturnMailForm;
#endif
#endif
#ifdef UPDATE_20061118 // エラーメールのエンベロープのFROM:を空白にするオプション
BOOL    bReturnMailEnvelope;
#endif
#ifdef UPDATE_20231103 // エラーメール生成時にメールヘッダの情報を利用するオプション機能を追加。
char    mMailEnvTo[512];// エラーメールのエンベローブの送信元として利用するヘッダ
char    mMailEnvFrom[512]; // エラーメールのエンベローブの送信先として利用するヘッダ
#endif
#ifdef UPDATE_20231104 // 自動応答生成時にメールヘッダの情報を利用するオプション機能を追加。
char    mMailReplyTo[512];// 自動応答のエンベローブの送信元として利用するヘッダ
char    mMailReplyFrom[512];// 自動応答のエンベローブの送信先として利用するヘッダ
#endif
#ifdef UPDATE_20220416 // ゲートウェイ先ごとにSMTP認証ID,PWを規定する場合を有効にするオプションフラグ
BOOL    bExGateway;
#endif
#ifdef UPDATE_20231007 // STARTTLSで暗号化ネゴシェーションに失敗するドメインリストの作成と失敗先へのプレーン送信への切替対策
BOOL    bBadSTLSDom; // STARTTLSで暗号ネゴシェーションに失敗するサイトの記録 1:する。（デフォルト）/ 0:しない。
#endif
#ifdef UPDATE_20171211A // 使用可能な暗号化通信方式のバージョンを規定するオプション 0x1:SSL2禁止 0x2:SSL3禁止 0x4:TSL1禁止 0x8:TSL1.1禁止 0x10:TSL1.2禁止
int nSSLOpt;
unsigned long nSecuerLayOption;
#endif
#ifdef UPDATE_20190426 // 利用する暗号アルゴリズムの組み合わせオプション 
int nSSLCipher;
char mSecuerLayCipher[1024];
#endif
#ifdef UPDATE_20240420 // 暗号化通信方式のセキュリティレベルの設定
int  nSSLSecureLevel;
#endif
#ifdef UPDATE_20091103 // LDAP連携専用のリトライ回数指定の変数に変更
DWORD  nLDAPRetryTime;
#endif
#ifdef UPDATE_20091026 // AD連携の問い合わせリトライ間隔を短く設定しなおした
DWORD  nLDAPRetryMSec;
DWORD  nADRetryMSec;
#endif
#ifdef UPDATE_20060606 // ドメインコントローラへの接続リトライ
DWORD  nADRetryTime;
#endif
#ifdef UPDATE_20140711 // エラーメール生成時の"multipart/report"に対応するオプションを追加した。
char    mHostName[256];
char    mMyFDQN[256];
#endif
#ifdef UPDATE_20070130 // 配送不能メールの送信者アカウントの指定をオプションを追加
char  mMailDaemonName[256];
#endif
#ifdef UPDATE_20070423 // MXキャッシュを全て削除する
BOOL  bMXAutoClean;    // MXキャッシュを起動時にクリアするオプション　TRUE:クリアする FALSE:クリアしない
#endif
#ifdef UPDATE_20070521 // OSの予約語対策
CHAR   mReservedWords[256];
#endif
#ifdef  INCOM_SUB_FOLDER // incoming フォルダをスレッド別フォルダにして処理
BOOL    bIncomingSubFolder; // incomingフォルダにスレッド別フォルダを作成 TRUE:する FALSE:しない
#endif
#ifdef UPDATE_20150108 // 送信許可時間の設定機能を追加
int     nUseTime;
char    mUseTimeFile[128];
#endif
#ifdef UPDATE_20220725 // "MAIL From:","RCPT To:"のコマンド出力を"MAIL FROM:","RCPT TO:"に変更するようにした。
int     nSelectcase; // 動詞部分の大小文字の選択: 0:従来 1:小文字 2:大文字（デフォルト）
#endif
//#define TRACE 1
#ifdef ADD_XOAUTH2_A // OAUTH2での認証方式を追加
char	mOAuthFile[256];
#endif
#ifdef UPDATE_20240127 // ＭＬ制御応答＆送信エラー通知メールにもDKIMサインを追加可能にするオプション
// DKIM設定箇所の選択 bit0:(受信時付加)デフォルト bit1:上長承認メールへ付加 bit2:添付分離めーるへ付加
int    bDKIM;
CHAR   mDomainAUTHDKIM[256];
#endif
#ifdef UPDATE_20240127C // DKIMサイン実施無効フラグを追加 (bit0:送信エラー bit1:ＭＬ制御応答 bit2:転送メール bit:3自動応答)
int    nOnDKIM;
CHAR   mDomainAUTHARC[256];
#endif

#ifdef Y2038_BUG
void gettime(SYSTEMTIME *ltime, char *mt)
#else
void gettime(time_t *ltime, char *mt)
#endif
{
	TIME_ZONE_INFORMATION tzi;
#ifdef Y2038_BUG
	FILETIME   ft;
    ULARGE_INTEGER *u1;
	SYSTEMTIME lt;
#else
	struct tm gmt;
	struct tm lt;
#endif
	char   mzone[6];
	int    off, i;

	*mt = '\x0';
    _tzset();
    off = (_timezone / 60) * -1;
#ifdef Y2038_BUG
	GetSystemTime(ltime);
#ifdef SUMMER_TIME_OFF // 夏時間はSystemTimeToTzSpecificLocalTimeで自動調整する
    SystemTimeToFileTime(ltime, &ft);
	u1 = (ULARGE_INTEGER *)&ft;
    if (_daylight)
      if (GetTimeZoneInformation(&tzi) == TIME_ZONE_ID_DAYLIGHT)
	  u1->QuadPart -= (__int64)36000000000; // 夏時間調整
    FileTimeToSystemTime(&ft, ltime);
#endif
	SystemTimeToTzSpecificLocalTime(NULL, ltime, &lt);
#else
    time(ltime);
    if (_daylight)
      if (GetTimeZoneInformation(&tzi) == TIME_ZONE_ID_DAYLIGHT)
	    *ltime -= 3600; // 夏時間調整
    lt = *localtime(ltime);
	gmt = *gmtime(ltime);
#endif
	///////////////////////
	  if (off == 0) {
	    strcpy(mzone, "GMT");
	  } else {
	    sprintf(mzone, "%s", (off < 0 ? "-":"+"));
		if (off < 0)
		  off *= -1; // 絶対値に修正
 	    if (off >= 24*60)		/* should be impossible */
  	      off = 23*60+59;		/* if not, insert silly value */
	    i = strlen(mzone);
	    mzone[i++] = (off / 600) + '0';
	    mzone[i++] = (off / 60) % 10 + '0';
	    off %= 60;
	    mzone[i++] = (off / 10) + '0';
	    mzone[i++] = (off % 10) + '0';
	    mzone[i] = '\0';
	  }
#ifdef Y2038_BUG
      sprintf(mt, "%3s, %02d %3s %04d %02d:%02d:%02d %s", mWeek[lt.wDayOfWeek], lt.wDay, mMonth[lt.wMonth-1], lt.wYear, lt.wHour, lt.wMinute, lt.wSecond, mzone);
#else
      strftime( mt, 128, "%a, %d %b %Y %H:%M:%S ", &lt );
      strcat(mt, mzone);
#endif
}

void Copyright(BOOL *bPassport) {
   OSVERSIONINFO VersionInformation;
   SYSTEM_INFO   systemInfo;
   char   mtime[256];
   char   mmode[64];//[10];
#ifdef Y2038_BUG
   SYSTEMTIME ltime;
#else
   time_t ltime;
#endif
   gettime(&ltime, mtime);
   GetTimeLimit(&ltbegin, mmode, bPassport);
   printf(SMTP_DEBUG_MESS, VERSION, mmode, mtime);
   printf("\n");
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

}

#ifdef UPDATE_20070425 // MSCSのスタンバイ側に対応
///////////////////////////////
// Service initialization
///////////////////////////////
BOOL ServiceInit(HANDLE  *phServerStopEvent, HANDLE *phEvents0, HANDLE *phEvents1, PSECURITY_DESCRIPTOR pSD, SECURITY_ATTRIBUTES *psa, HANDLE *phPipe, LPTSTR lpszPipeName) {
    if (!ReportStatusToSCMgr(
        SERVICE_START_PENDING, // service state
        NO_ERROR,              // exit code
        3000))                 // wait hint
        return FALSE;
    // create the event object. The control handler function signals
    // this event when it receives the "stop" control code.
    *phServerStopEvent = CreateEvent(
        NULL,    // no security attributes
        TRUE,    // manual reset event
        FALSE,   // not-signalled
        NULL);   // no name
    if ( *phServerStopEvent == NULL)
        return FALSE;
    *phEvents0 = hServerStopEvent;
    // report the status to the service control manager.
    //
    if (!ReportStatusToSCMgr(
        SERVICE_START_PENDING, // service state
        NO_ERROR,              // exit code
        3000))                 // wait hint
        return FALSE;
    // create the event object object use in overlapped i/o
    //
    *phEvents1 = CreateEvent(
        NULL,    // no security attributes
        TRUE,    // manual reset event
        FALSE,   // not-signalled
        NULL);   // no name
    if ( *phEvents1 == NULL)
        return FALSE;
    // report the status to the service control manager.
    //
    if (!ReportStatusToSCMgr(
        SERVICE_START_PENDING, // service state
        NO_ERROR,              // exit code
        3000))                 // wait hint
        return FALSE;
    // create a security descriptor that allows anyone to write to
    //  the pipe...
    //
    pSD = (PSECURITY_DESCRIPTOR) malloc( SECURITY_DESCRIPTOR_MIN_LENGTH );
    if (pSD == NULL)
        return FALSE;
    if (!InitializeSecurityDescriptor(pSD, SECURITY_DESCRIPTOR_REVISION))
        return FALSE;
    // add a NULL disc. ACL to the security descriptor.
    if (!SetSecurityDescriptorDacl(pSD, TRUE, (PACL) NULL, FALSE))
        return FALSE;

    psa->nLength = sizeof(SECURITY_ATTRIBUTES);
    psa->lpSecurityDescriptor = pSD;
    psa->bInheritHandle = TRUE;
    // report the status to the service control manager.
    //
    if (!ReportStatusToSCMgr(
        SERVICE_START_PENDING, // service state
        NO_ERROR,              // exit code
        3000))                 // wait hint
        return FALSE;
    // open our named pipe...
    //
    *phPipe = CreateNamedPipe(
                    lpszPipeName         ,  // name of pipe
                    FILE_FLAG_OVERLAPPED |
                    PIPE_ACCESS_DUPLEX,     // pipe open mode
                    PIPE_TYPE_MESSAGE |
                    PIPE_READMODE_MESSAGE |
                    PIPE_WAIT,              // pipe IO type
                    1,                      // number of instances
                    0,                      // size of outbuf (0 == allocate as necessary)
                    0,                      // size of inbuf
                    1000,                   // default time-out value
                    psa);                   // security attributes

    if (*phPipe == INVALID_HANDLE_VALUE) {
        AddToMessageLog(TEXT("The named pipe could not be made."), 202, TEXT("%s error: %d"), EVENTLOG_ERROR_TYPE);
        return FALSE;
    }
    
    // report the status to the service control manager.
    //
    if (!ReportStatusToSCMgr(
        SERVICE_RUNNING,       // service state
        NO_ERROR,              // exit code
        0))                    // wait hint
        return FALSE;

   return TRUE;
}
#endif UPDATE_20070425 // MSCSのスタンバイ側に対応

//void main (int argc, char *argv[])
VOID ServiceStart (DWORD dwArgc, LPTSTR *lpszArgv)
{
	BOOL                    bPassport = FALSE;
    HANDLE                  hPipe = INVALID_HANDLE_VALUE;
    HANDLE                  hEvents[3] = {NULL,NULL,NULL};
    OVERLAPPED              os;
    PSECURITY_DESCRIPTOR    pSD = NULL;
    SECURITY_ATTRIBUTES     sa;
    LPTSTR                  lpszPipeName = TEXT("\\\\.\\pipe\\" TRADEMARK "SMTPDS");
    DWORD                   dwWait, dwc, dwerr;
    //UINT                    ndx;
    int status, nl;             /* Status Code */
    WSADATA WSAData;
    char *tmp, *p, *pldom;
    char mNS[MAXDNAME], mMQ[MAXDNAME];
	//FILE *fp;
    struct _SMTPDS mSmtpds;
#ifdef  INCOM_SUB_FOLDER // incoming フォルダをスレッド別フォルダにして処理
    struct _SMTPDS *pSmtpds;
#endif
	DWORD  nNum, nCnt;
#ifdef Y2038_BUG
  ULARGE_INTEGER u1;
#endif
    char  mLicencekey[65];
    char   mmode[64];//[10];
#ifdef CLUSTERING
	DWORD    nComName;
	HANDLE   hF;
#endif
#ifdef  INCOM_SUB_FOLDER // incoming フォルダをスレッド別フォルダにして処理
	DWORD    nSubNum;
	CHAR     mIncomingThread[_MAX_PATH+1];
#endif
// スレッドあたりのメモリ使用数
//printf("%lu\n", sizeof(struct _RCP));
//printf("%lu\n", sizeof(struct _MRI));
//printf("%lu\n", sizeof(struct _MAIL_CTL));
//printf("%lu\n", sizeof(struct _SMTPDS));
//printf("%lu\n", sizeof(struct sockaddr));
	//////////////////////////////////////////////////////////////
#ifdef UPDATE_20070405 // イベントログデータベースを追加。
    InitEventLog();
#endif
    // initalaize //////////////////
#ifdef V4
    _setmaxstdio(2048);
#endif
	nLCID = GetSystemDefaultLCID();
	//bTMQWait = FALSE;
	mGetRCP[0] = '\x0';
    nChkTM  = 120;
	//nThreadNo = 0;
	//////////////////////////////////////////////////////////////
#ifdef CLUSTERING
	///// コンピュータ名を取得
	nComName = sizeof(mComName);
	GetComputerName(mComName, &nComName);
	///// 論理IP指定がある場合はホットスタンバイ
    GetProfileStringEx(SYSTEM_REG,"LogicalIP", "", mLogicalIP, sizeof(mLogicalIP));
	///// UNC接続可能になるまでのリトライ時間
	nReadyDriveTime = GetProfileIntEx(SOFT_REG, "ReadyDriveTime", 5);
	///// スプール先はレジストリから取得
    GetProfileStringEx(SOFT_REG,"MailSpoolDir", MAIL_SPOOL, mMailSpoolDir, sizeof(mMailSpoolDir));
#ifdef LDAP_ON         // LDAPへのログイン処理でSMTP認証の確認を行うオプション // //CRAM-MD5は複合できないので、これを利用する場合は、PLAINかLOGINのみとします。
    bLDAPOn = GetProfileIntEx(SOFT_REG, "LDAP", (int) FALSE);    // LDAP対応
#endif
#ifdef LGWAN_ON        // LGWAN機能拡張
	bLGWAN = GetProfileIntEx(SOFT_REG, "LGWAN", (int) FALSE);    // LGWAN拡張
	bCHGHEADER = GetProfileIntEx(SOFT_REG, "LGWANCHGHEADER", (int) TRUE);    // LGWAN拡張
#endif
#ifdef UPDATE_20070425 // MSCSのスタンバイ側に対応
    if (!ServiceInit(&hServerStopEvent, &hEvents[0], &hEvents[1], pSD, &sa, &hPipe, lpszPipeName))
      goto cleanup;
    memset( &os, 0, sizeof(OVERLAPPED) );
    os.hEvent = hEvents[1];
    ResetEvent( hEvents[1] );
	printf("Query work folder = [%s]\n", mMailSpoolDir);
    while (!QueryDrive(mMailSpoolDir))
    {
        ConnectNamedPipe(hPipe, &os);
        if ( GetLastError() == ERROR_IO_PENDING )
        {
          dwWait = WaitForMultipleObjects( 2, hEvents, FALSE, 1000);
          if ( dwWait == 0 )
		    goto cleanup;
		}
        // drop the connection...
        DisconnectNamedPipe(hPipe);
    }
	printf("Ready work folder = [%s]\n", mMailSpoolDir);
#else
	ReadyDrive(mMailSpoolDir); // DISKが使用状態になるまで待つ
#endif UPDATE_20070425 // MSCSのスタンバイ側に対応
	///// クラスタ対応モードはレジストリから取得
	nClustering = GetProfileIntEx(SOFT_REG, "Clustering", (int)0);
#endif
	///// ライセンスキー情報
    GetProfileStringEx(SYSTEM_REG_RS, LIMITKEY, "", mLicencekey, sizeof(mLicencekey));
    //////////////////////
#ifndef E_POST
	nChkTM  = GetProfileIntEx("SYSTEM\\CurrentControlSet\\Services\\SMTPDS\\Parameters", "Timeout", (int) 120);
#endif
	if (nChkTM == 120)
	  nChkTM  = GetProfileIntEx(SYSTEM_PARAM_REG, "Timeout", (int) 120);

#ifdef UPDATE_20070423 // MXキャッシュを全て削除する
    bMXAutoClean = GetProfileIntEx(SYSTEM_REG , "MXAutoClean", (int)FALSE);     // MXキャッシュを起動時にクリアするオプション。　TRUE:クリアする FALSE:クリアしない
#endif
#ifdef UPDATE_20060629 // 400番台のリトライが500番台に切替えられるてしまう。
    bOldRetryMode = GetProfileIntEx(SYSTEM_REG , "OldRetryMode", (int)FALSE); //４００番台と５００番台リトライをシンプルにする。　FALSE:シンプル TRUE:旧方式
#endif
#ifdef UPDATE_20060628 // deadフォルダに保管しないオプション
	bSaveDead = GetProfileIntEx(SYSTEM_REG , "SaveDead", (int)TRUE); // deadフォルダに保管しないオプション TRUE:保管する FALSE:保管しない
#endif
#ifdef UPDATE_20060621 // リトライ間隔のままのリトライがスレッドが生きていり間リトライされ続けてしまう対策
	bCompl = GetProfileIntEx(SYSTEM_REG , "Compulsion", (int)FALSE); // 強制吐き出しオプション TRUE:強制 FALSE:しない
#endif
#ifdef UPDATE_20060615 // データのディスク書込に異常が有った場合の対策
	bVLog = GetProfileIntEx(SOFT_REG , "VLog", (int)0); // イベントビューワにログ書込みエラーを表示する 0:しない　1:する
#endif
#ifdef UPDATE_20231007 // STARTTLSで暗号化ネゴシェーションに失敗するドメインリストの作成と失敗先へのプレーン送信への切替対策
    bBadSTLSDom = GetProfileIntEx(SYSTEM_REG , "RecodeBadSTLSDomain", (int)1); // STARTTLSで暗号ネゴシェーションに失敗するサイトの記録 1:する。（デフォルト）/ 0:しない。
#endif
#ifdef UPDATE_20171211A // 使用可能な暗号化通信方式のバージョンを規定するオプション 0x1:SSL2禁止 0x2:SSL3禁止 0x4:TSL1禁止 0x8:TSL1.1禁止 0x10:TSL1.2禁止
    nSSLOpt = GetProfileIntEx(SYSTEM_REG , "SecureLayOption", (int)3); // 0x1:SSL2禁止 0x2:SSL3禁止 0x4:TSL1禁止 0x8:TSL1.1禁止 0x10:TSL1.2禁止 0x20:TSL1.3禁止
//#ifdef UPDATE_20231006 // Openssl 3系でdocomoのSMTPに繋がらない対策
    //nSecuerLayOption = 0x4; // SSL_OP_LEGACY_SERVER_CONNECT=0x4
//	nSecuerLayOption = 0x20000000L; //SSL_OP_NO_TLSv1_3
//#else
	nSecuerLayOption = 0;
//#endif
    if (nSSLOpt & 0x1)
      nSecuerLayOption |= SSL_OP_NO_SSLv2;
    if (nSSLOpt & 0x2)
      nSecuerLayOption |= SSL_OP_NO_SSLv3;
    if (nSSLOpt & 0x4)
      nSecuerLayOption |= SSL_OP_NO_TLSv1;
    if (nSSLOpt & 0x8)
      nSecuerLayOption |= SSL_OP_NO_TLSv1_1;
    if (nSSLOpt & 0x10)
      nSecuerLayOption |= SSL_OP_NO_TLSv1_2;
//#ifdef UPDATE_20231006 // Openssl 3系でdocomoのSMTPに繋がらない対策
//    if (nSSLOpt & 0x20)
//      nSecuerLayOption |= 0x20000000L; //SSL_OP_NO_TLSv1_3
//#endif
#endif
#ifdef UPDATE_20190426 // 利用する暗号アルゴリズムの組み合わせオプション 
	 GetProfileStringEx(SYSTEM_REG, "SecuerLayCipher", "", mSecuerLayCipher, sizeof(mSecuerLayCipher));
#endif
#ifdef UPDATE_20240420 // 暗号化通信方式のセキュリティレベルの設定
     nSSLSecureLevel = GetProfileIntEx(SYSTEM_REG , "SecureLevel", (int)0); // 
#endif
#ifdef UPDATE_20091103 // LDAP連携専用のリトライ回数指定の変数に変更
	nLDAPRetryTime = GetProfileIntEx(SYSTEM_REG , "LDAPRetryTime", (int)100);
#endif
#ifdef UPDATE_20091026 // AD連携の問い合わせリトライ間隔を短く設定しなおした
    nLDAPRetryMSec = GetProfileIntEx(SYSTEM_REG , "LDAPRetryMSec", (int)6000);
    nADRetryMSec = GetProfileIntEx(SYSTEM_REG , "ADRetryMSec", (int)1000);
#endif
#ifdef UPDATE_20060606 // ドメインコントローラへの接続リトライ
	nADRetryTime = GetProfileIntEx(SYSTEM_REG , "ADRetryTime", (int)10);
#endif
#ifdef UPDATE_20230105 // X-UIDLヘッダを追加するオプション
    bADDXUIDL = GetProfileIntEx(SYSTEM_REG, "XUIDL", (int)FALSE);   // X-UIDLヘッダを追加するか否か
#endif
    bMTAIP = GetProfileIntEx(SYSTEM_REG, "MTAIPOn", (int)FALSE);   // MTAのIPを送信時設定するか否か
	bServerType = GetProfileIntEx(SYSTEM_REG, "ServerType", (int)FALSE); // Localを使用
	nSendRefusalTime = GetProfileIntEx(SYSTEM_REG, "SendRefusalTime", (int)9);   // 受信拒絶時のリトライ回数
	nSendMaxRetryTime = GetProfileIntEx(SYSTEM_REG, "SendMaxRetryTime", (int)8); // 無応答時のトライ回数
	nSendOtherTime = GetProfileIntEx(SYSTEM_REG, "SendOtherTime", (int)nSendMaxRetryTime*3);    // その他の受信拒絶時のリトライ回数
	nSendOther    = nSendOtherTime * 3600 / (nChkTM ? nChkTM : 1);  // / 120; // 無通信時のトライ回数
    nSendMaxRetry = nSendMaxRetryTime * 3600 / (nChkTM ? nChkTM : 1);  // / 120; // default 8 時間
	nTMOut = GetProfileIntEx(SYSTEM_REG, "Timeout", (int)1); //14400); // 60秒×60×4 = 4時間
	bRetryRule = GetProfileIntEx(SYSTEM_REG, "RetryRule", (int)TRUE); // リトライ送信の間隔制御を有効にする。
	nSendBuf = GetProfileIntEx(SYSTEM_REG, "SendDataBufferSize", (int)0); // socket 送信バッファサイズ
	nSendTMO = GetProfileIntEx(SYSTEM_REG, "SendDataTimeout", (int)1200); // 60秒×10×1 = 10分
    nRecvTMO = GetProfileIntEx(SYSTEM_REG, "RecvDataTimeout", (int)1200); // 60秒×20×1 = 20分
	nGreetingTMO = GetProfileIntEx(SYSTEM_REG, "GreetingTimeout", (int)10); // 10秒
	bThreadType = GetProfileIntEx(SYSTEM_REG, "ThreadType", (int) TRUE); // デフォルトはマルチスレッド
	nMaxThread  = GetProfileIntEx(SYSTEM_REG, "MaxThread", (int) 2);     // デフォルトはスレッド数1
#ifdef UPDATE_20050901
#ifdef E_POST
#ifdef UPDATE_20140711 // エラーメール生成時の"multipart/report"に対応するオプションを追加した。
	nReturnMailForm = GetProfileIntEx(SYSTEM_REG, "ReturnMailForm", (int) 2); // 0:テキスト, 1:添付ファイル 2:マルチパートレポート
#else
	bReturnMailForm = GetProfileIntEx(SYSTEM_REG, "ReturnMailForm", (int) TRUE); // FALSE:テキスト, TRUE:添付ファイル
#endif
#else
	bReturnMailForm = GetProfileIntEx(SYSTEM_REG, "ReturnMailForm", (int) FALSE); // FALSE:テキスト, TRUE:添付ファイル
#endif
#endif
#ifdef UPDATE_20061118 // エラーメールのエンベロープのFROM:を空白にするオプション
    bReturnMailEnvelope = GetProfileIntEx(SYSTEM_REG, "ReturnMailEnvelope", (int) TRUE); // FALSE:アドレス指定, TRUE:空白アドレス
#endif
#ifdef UPDATE_20231103 // エラーメール生成時にメールヘッダの情報を利用するオプション機能を追加。
    GetProfileStringEx(SYSTEM_REG, "ReturnMailEnvelopeTo", "" /*"Return-Path From Sender Reply-To Return-Receipt-To Errors-To Resent-Sender Resent-From Resent-Reply-To"*/, mMailEnvTo, sizeof(mMailEnvTo)); // エラーメールのエンベローブの送信元として利用するヘッダ
    GetProfileStringEx(SYSTEM_REG, "ReturnMailEnvelopeFrom", "" /*"To Cc Bcc Apparently-To Resent-To Resent-Cc Resent-Bcc"*/, mMailEnvFrom, sizeof(mMailEnvFrom)); // エラーメールのエンベローブの送信先として利用するヘッダ
if (bDebug) printf("ReturnMailForm = [%d]\nReturnMailEnvelopeTo = [%s]\nReturnMailEnvelopeFrom = [%s]\n", bReturnMailEnvelope, mMailEnvTo, mMailEnvFrom);
#endif
#ifdef UPDATE_20231104 // 自動応答生成時にメールヘッダの情報を利用するオプション機能を追加。
    GetProfileStringEx(SYSTEM_REG, "ReplyMailEnvelopeTo", "" /*"Return-Path From Sender Reply-To Return-Receipt-To Errors-To Resent-Sender Resent-From Resent-Reply-To"*/, mMailReplyTo, sizeof(mMailReplyTo)); // 自動応答のエンベローブの送信元として利用するヘッダ
    GetProfileStringEx(SYSTEM_REG, "ReplyMailEnvelopeFrom", "" /*"To Cc Bcc Apparently-To Resent-To Resent-Cc Resent-Bcc"*/, mMailReplyFrom, sizeof(mMailReplyFrom)); // 自動応答のエンベローブの送信先として利用するヘッダ
if (bDebug) printf("ReplyMailEnvelopeTo = [%s]\nReplyMailEnvelopeFrom = [%s]\n", mMailReplyTo, mMailReplyFrom);
#endif
#ifdef UPDATE_20220416 // ゲートウェイ先ごとにSMTP認証ID,PWを規定する場合を有効にするオプションフラグ
#ifdef UPDATE_20241217
    bExGateway = GetProfileIntEx(SYSTEM_REG, "ExpansionGatewayTable", (int) TRUE); // FALSE:無効, TRUE:有効
#else
    bExGateway = GetProfileIntEx(SYSTEM_REG, "ExpansionGatewayTable", (int) FALSE); // FALSE:無効, TRUE:有効
#endif
#endif

#ifdef HOME_VERSION
	if (nMaxThread > 10)
	  nMaxThread = 10;
#endif
	nMaxDivide  = GetProfileIntEx(SYSTEM_REG, "MaxDivide", (int) 0);     // デフォルトはドメイン分割 (bThreadType ? nMaxThread : 1)); // デフォルトはマルチスレッド数
	nDomainDivide = GetProfileIntEx(SYSTEM_REG, "DomainDivide", (int) 2); // デフォルトは2分割
#ifdef ENTERPRISE
    nGetRCPWait = GetProfileIntEx(SYSTEM_REG, "GetRCPWait", (int) 10);     // エンタープライズは無制限
#else
    nGetRCPWait = GetProfileIntEx(SYSTEM_REG, "GetRCPWait", (int) 10);    // デフォルトは10回待ち
#endif
	nRunThread  = 0;
#ifdef Y2038_BUG
	u1.LowPart = GetProfileIntEx(SYSTEM_REG_RS , BEGINLOW, (int)0);
	u1.HighPart = GetProfileIntEx(SYSTEM_REG_RS , BEGINHIGH, (int)0);
	ltbegin = u1.QuadPart;
#else
	ltbegin = GetProfileIntEx(SYSTEM_REG_RS, "Begin", (int)0);
#endif
    ReSetTimeLimit(&ltbegin);
    ////////////////////////////////
	bCountLock = FALSE;  // Counter Release
	nport = 25;
	bLog  = FALSE;
#ifndef E_POST
    nport = GetProfileIntEx("SYSTEM\\CurrentControlSet\\Services\\SMTPDS\\Parameters", "PortNo", (int)25);
	bLog  = GetProfileIntEx("SYSTEM\\CurrentControlSet\\Services\\SMTPDS\\Parameters", "MailOutlogEnabled", (int)FALSE);
#endif
	if (nport == 25)
      nport = GetProfileIntEx(SYSTEM_PARAM_REG, "PortNo", (int)25);
	if (!bLog)
	  bLog  = GetProfileIntEx(SYSTEM_PARAM_REG, "MailOutlogEnabled", (int)FALSE);
#ifdef UPDATE_20140711 // エラーメール生成時の"multipart/report"に対応するオプションを追加した。
    GetProfileStringEx(SOFT_REG, "HostName", "", mHostName, sizeof(mHostName));
#endif
#ifdef UPDATE_20070130 // 配送不能メールの送信者アカウントの指定をオプションを追加
    GetProfileStringEx(SOFT_REG, "MailDaemonName", "postmaster", mMailDaemonName, sizeof(mMailDaemonName)); // 親子関係　デフォルト NULL=workgroup(ローカルマシン)
#endif
#ifdef V3
	bUserMan = GetProfileIntEx(SOFT_REG, "UserManager", (INT)TRUE);    // TRUE=NT使用,FALSE=SPA使用
	bInboxEnc = GetProfileIntEx(SOFT_REG, "InboxData", (INT)FALSE);    // TRUE=記号化する,FALSE=記号化しない
#endif
	bOverlapFile = GetProfileIntEx(SOFT_REG, "OverlapFile", (int)FALSE);
    GetProfileStringEx(SOFT_REG, "Membership", "", mAuthDomain, sizeof(mAuthDomain)); // 親子関係　デフォルト NULL=workgroup(ローカルマシン)
#ifdef LDAP_ON         // LDAPへのログイン処理でSMTP認証の確認を行うオプション // //CRAM-MD5は複合できないので、これを利用する場合は、PLAINかLOGINのみとします。
    bLDAP = bLDAPOn && GetProfileIntEx(SOFT_REG, "LDAP", (int) FALSE);    // LDAP対応
	mLDAPAdminID[0] = '\x0';
    nLDAPPort = GetProfileIntEx(SOFT_REG, "LDAPPort", (int) 389);    // LDAP対応
    GetProfileStringEx(SOFT_REG, "LDAPAdminID", "", mLDAPAdminID, sizeof(mLDAPAdminID)); // LDAP管理者ID
    GetProfileStringEx(SOFT_REG, "LDAPAdminPW", "", mLDAPAdminPW, sizeof(mLDAPAdminPW)); // LDAP管理者パスワード
    GetProfileStringEx(SOFT_REG, "LDAPSchemaRDN", "cn", mLDAPSchemaRDN, sizeof(mLDAPSchemaRDN)); // LDAP RDN
    GetProfileStringEx(SOFT_REG, "LDAPSchemaDC", "", mLDAPSchemaDC, sizeof(mLDAPSchemaDC)); // LDAP DC（ドメインコンポーネント）ディレクトリのスキーマ名称
    GetProfileStringEx(SOFT_REG, "LDAPSchemaID", "", mLDAPSchemaID, sizeof(mLDAPSchemaFilter)); // LDAP ユーザーID名称
    GetProfileStringEx(SOFT_REG, "LDAPSchemaFilter", "", mLDAPSchemaFilter, sizeof(mLDAPSchemaFilter)); // LDAP ユーザーID名称
    GetProfileStringEx(SOFT_REG, "LDAPSchemaHome", "", mLDAPSchemaHome, sizeof(mLDAPSchemaHome)); // LDAP Homeディレクトリのスキーマ名称
    GetProfileStringEx(SOFT_REG, "LDAPSchemaName", "", mLDAPSchemaName, sizeof(mLDAPSchemaName)); // LDAP 名称（フルネーム）のスキーマ名称
    GetProfileStringEx(SOFT_REG, "LDAPSchemaMember", "", mLDAPSchemaMember, sizeof(mLDAPSchemaMember)); // LDAP Memberのスキーマ名称
#endif
#ifdef UPDATE_20070521 // OSの予約語対策
    GetProfileStringEx(SOFT_REG, "ReservedWords", "@", mReservedWords, sizeof(mReservedWords)); // LDAP Memberのスキーマ名称
#endif
#ifdef REGTOFILE
	if (!bUserMan) // 独自アカウントなら
	  ReadyDrive(mAuthDomain); // DISKが使用状態になるまで待つ
#endif
	nAddressFamily = GetProfileIntEx(SOFT_REG, "AddressFamily", (INT)0); //0:AF_INET only 1:AF_INET6 only 2:AF_INET&AF_INET6 
	//bAutoCreateInbox = GetProfileIntEx("SOFTWARE\\EMWAC\\IMS", "AutoCreateInboxDir", (int)FALSE);
	bAutoCreateInbox = GetProfileIntEx(SOFT_REG, "AutoCreateInboxDir", (int)FALSE);
	//bSendPostMaster = GetProfileIntEx("SOFTWARE\\EMWAC\\IMS", "LocalFailuresToPostmaster", (INT) FALSE);
	bSendPostMaster = GetProfileIntEx(SOFT_REG, "LocalFailuresToPostmaster", (INT) FALSE);
	bCheckLocalDomainIP = GetProfileIntEx(SOFT_REG, "CheckLocalDomainIP", (INT) TRUE);
    //nLocalDomain = GetProfileBinaryEx("SOFTWARE\\EMWAC\\IMS","DomainNamesAreLocal", "", mLocalDomain, sizeof(mLocalDomain));
    nLocalDomain = GetProfileBinaryEx(SOFT_REG,"DomainNamesAreLocal", "", mLocalDomain, sizeof(mLocalDomain));
	//nMailInCopy = GetProfileBinaryEx("SOFTWARE\\EMWAC\\IMS","MailInCopy", "", mMailInCopy, sizeof(mMailInCopy));
	nMailInCopy = GetProfileBinaryEx(SOFT_REG,"MailInCopy", "", mMailInCopy, sizeof(mMailInCopy));
	//nMailInMaxSize = GetProfileIntEx("SOFTWARE\\EMWAC\\IMS", "MailInMaxSize", (INT)0);
	nMailInMaxSize = GetProfileIntEx(SOFT_REG, "MailInMaxSize", (INT)0);
	//nMailInBoxSize = GetProfileIntEx("SOFTWARE\\EMWAC\\IMS", "MailInBoxSize", (INT)0);
	nMailInBoxSize = GetProfileIntEx(SOFT_REG, "MailInBoxSize", (INT)0);
    //GetProfileStringEx("SOFTWARE\\EMWAC\\IMS","PostMaster", "", mPostmaster, sizeof(mPostmaster));
    GetProfileStringEx(SOFT_REG,"PostMaster", "administrator", mPostmaster, sizeof(mPostmaster));
    if (mPostmaster[0] == '\x0')
 	  strcpy(mPostmaster, "administrator");
	//// Postmasterのドメイン無しアカウントの場合、運用ドメインより先頭のドメインを付加。
	if (mLocalDomain[0] != '\x0' && 
		!strstr(mPostmaster,"@")) {
		strcat(mPostmaster, "@");
		strcat(mPostmaster, mLocalDomain);
	}
    //GetProfileStringEx("SOFTWARE\\EMWAC\\IMS","MailInBoxDir", "", mMailBoxDir, sizeof(mMailBoxDir));
    GetProfileStringEx(SOFT_REG,"MailInBoxDir", MAIL_BOX, mMailBoxDir, sizeof(mMailBoxDir));
    if (mMailBoxDir[0] == '\x0')
	  strcpy(mMailBoxDir, MAIL_BOX);
	else if (!strstr(mMailBoxDir,"%")) {
	  strcat(mMailBoxDir, "%USERNAME%");
	}
#ifdef REGTOFILE
#ifdef UPDATE_20090610 // %HOME%が設定されているとまともに動かない
	if (mMailBoxDir[0] != '%')
#endif
	{
	  ReadyDrive(mMailBoxDir); // DISKが使用状態になるまで待つ
	}
#endif
	//GetProfileStringEx("SYSTEM\\CurrentControlSet\\Services\\SPADS","SendGateway", "", mSendGateway, sizeof(mSendGateway));
	GetProfileStringEx(SYSTEM_REG,"SendGateway", "", mSendGateway, sizeof(mSendGateway));
#ifdef UPDATE_20230310 // SMTPゲートウェイ欄にゲートウェイ先明記ならセッションを分離しない
    bIncludeForward= GetProfileIntEx(SYSTEM_REG, "IncludeForward", (INT)TRUE);  // SMTP GateWayへ転送ドメイン毎にセッションを分離しない。 TRUE:分離しない FALSE:分離する
#endif
	bMailForward = GetProfileIntEx(SYSTEM_REG, "MailForward", (INT)FALSE);  // 全受信をSMTP GateWayへ転送。
#ifdef UPDATE_20141105 // MXレコード参照失敗した受信をSMTP Gatewayへ転送
	bMailForward2 = GetProfileIntEx(SYSTEM_REG, "MailForward2", (INT)FALSE);  // MXレコード参照失敗した受信をSMTP Gatewayへ転送
#endif
	//GetProfileStringEx("SYSTEM\\CurrentControlSet\\Services\\SPARS","DefaultCode", "us-ascii", mDefaultCode, sizeof(mDefaultCode));
	GetProfileStringEx(SYSTEM_REG_RS,"DefaultCode", "us-ascii", mDefaultCode, sizeof(mDefaultCode));
	strtok(mDefaultCode," ");
#ifdef ESMTP_ON
	GetProfileStringEx(SYSTEM_REG, "SMTPAUTHId", "", mSMTPAUTHID, sizeof(mSMTPAUTHID));
	GetProfileStringEx(SYSTEM_REG, "SMTPAUTHPass", "", mSMTPAUTHPASS, sizeof(mSMTPAUTHPASS));
#ifdef ADD_XOAUTH2 // OAUTH2での認証方式を追加
    bAUTHTYPE[0] = bAUTHTYPE[1] = bAUTHTYPE[2] = bAUTHTYPE[3] = FALSE;
#else
    bAUTHTYPE[0] = bAUTHTYPE[1] = bAUTHTYPE[2] = FALSE;
#endif
	GetProfileStringEx(SYSTEM_REG, "SMTPAUTHMode", "", mSMTPAUTHMODE, sizeof(mSMTPAUTHMODE)); //mSMTPAUTHMODE
    _strupr(mSMTPAUTHMODE);
    if (strstr(mSMTPAUTHMODE, "CRAM-MD5"))
	  bAUTHTYPE[0] = TRUE;
    if (strstr(mSMTPAUTHMODE, "LOGIN"))
	  bAUTHTYPE[1] = TRUE;
    if (strstr(mSMTPAUTHMODE, "PLAIN"))
	  bAUTHTYPE[2] = TRUE;
#ifdef ADD_XOAUTH2 // OAUTH2での認証方式を追加
    if (strstr(mSMTPAUTHMODE, "XOAUTH2"))
	  bAUTHTYPE[3] = TRUE;
#endif
	bESMTP = GetProfileIntEx(SYSTEM_REG, "ESMTP", (INT)FALSE);  // Greeting を EHLO で行うか否か。
#ifdef SENDERID
	bSenderID = GetProfileIntEx(SYSTEM_REG, "SenderID", (int) FALSE);    // TRUE:SenderID使用, FALSE:SenderID不使用
#endif
    ////////////////////////////////
// test
//strcpy(mSMTPAUTHID,"kawa");
//strcpy(mSMTPAUTHPASS,"AXCEwQCEEeEWSWSeE2E2230");
//strcpy(mSMTPAUTHMODE,"CRAM-MD5");
//bAUTHTYPE[0] = TRUE;
//bESMTP = TRUE;
/////////
#endif
//CheckUser("kawa@kawa.co.jp", &btst, fullname, NULL);
//exit(0);
	//nLastMsgId = GetProfileIntEx("SOFTWARE\\EMWAC\\IMS", "LastMsgId", (int)0);
	nLastMsgId = GetProfileIntEx(SOFT_REG, "LastMsgId", (int)0);
	//GetProfileStringEx("SOFTWARE\\EMWAC\\IMS","MailGroup","IMSUsers", mMailGroup, sizeof(mMailGroup));
	GetProfileStringEx(SOFT_REG,"MailGroup","IMSUsers", mMailGroup, sizeof(mMailGroup));
	//////////////////////
    GetProfileStringEx(SYSTEM_REG_RS,"PasswordFile","apop.dat", mPasswordFile, sizeof(mPasswordFile));
	///////////////////////
	bServiceTerminating = FALSE;
    //// Wax連動 ////
#ifdef WAX_ON
	GetProfileStringEx(SYSTEM_REG,"WaxPipe", "\\\\.\\pipe\\waxsvc", mWaxPipe, sizeof(mWaxPipe));
#endif
#ifdef UPDATE_20220725 // "MAIL From:","RCPT To:"のコマンド出力を"MAIL FROM:","RCPT TO:"に変更するようにした。
    nSelectcase = GetProfileIntEx(SYSTEM_REG, "SelectCase", (int) 2); // 動詞部分の大小文字の選択: 0:従来 1:小文字 2:大文字（デフォルト）
#endif
#ifdef USE_SSL
#ifdef USE_STARTTLS
	bUsedSTLS = GetProfileIntEx(SYSTEM_REG, "UsedSTARTTLS", (int) TRUE);  // TRUE:STARTTLS使用, FALSE:STARTTLS不使用
#endif
	bUsedSSL = GetProfileIntEx(SYSTEM_REG, "UsedSSL", (int) FALSE);    // TRUE:SSL使用, FALSE:SSL不使用
#endif
#ifdef V4
	nMXCashLiveTime = GetProfileIntEx(SYSTEM_REG, "MXCashLiveTime", (INT) 86400*10); // MXキャッシュ利用有効時間(秒単位) デフォルト24時間×10日
#endif
#ifdef ADD_IDCACHE
	nIDCashLiveTime = GetProfileIntEx(SYSTEM_REG, "ADCashLiveTime", (INT) 0); // MXキャッシュ利用有効時間(秒単位) デフォルト0:無効
#endif
	//////////////////////////////////////////////////////////////
    bSendLocalLog  = GetProfileIntEx(SYSTEM_REG, "MailSendLocallogEnabled", (int)FALSE);
    bFailLog  = GetProfileIntEx(SYSTEM_REG, "MailFaillogEnabled", (int)FALSE);
	//bAcceptLog = GetProfileIntEx("SYSTEM\\CurrentControlSet\\Services\\SPARS", "AcceptlogEnabled", (int)FALSE);
	nSenderLog = GetProfileIntEx(SYSTEM_REG, "SenderlogEnabled", (int)0); // 1:Level1, 2:Level2
#ifdef UPDATE_20060613 // senderlogでプロトコルログ のみ出力するオプション追加
    nSender2Log = GetProfileIntEx(SYSTEM_REG, "Sender2logEnabled", (int)0); // 1:Level1
    nSenderlogUnit = GetProfileIntEx(SYSTEM_REG, "SenderlogUnit", (int)0);  // ログするファイルの量指定 0:１日 1:１時間
#endif
#ifdef UPDATE_20150108 // 送信許可時間の設定機能を追加
	///// 送信許可時間設定の有効有無
	nUseTime = GetProfileIntEx(SYSTEM_REG, "UseTimeFileEnabled", (int)0); // 0:禁止 1:許可 2:許可(プラス MAIL FROM)
    ///// 送信許可時間設定ファイル
    GetProfileStringEx(SYSTEM_REG,"UseTimeFile","usetime.dat", mUseTimeFile, sizeof(mUseTimeFile));
#endif
#ifdef UPDATE_20240127 // ＭＬ制御応答＆送信エラー通知メールにもDKIMサインを追加可能にするオプション
    bDKIM = GetProfileIntEx(SYSTEM_REG_RS, "OnDKIM", (int) FALSE);    // TRUE:DKIM使用, FALSE:DKIM不使用
    GetProfileStringEx(SYSTEM_REG_RS, "DomainAUTHDKIM", "", mDomainAUTHDKIM, sizeof(mDomainAUTHDKIM));
#endif
#ifdef UPDATE_20240127C // DKIMサイン実施無効フラグを追加 1:無効(bit0:送信エラー bit1:ＭＬ制御応答 bit2:転送メール bit:3自動応答)
    nOnDKIM = GetProfileIntEx(SYSTEM_REG, "OnDKIM", (int) 15);    // (bit0:送信エラー bit1:ＭＬ制御応答 bit2:転送メール bit:3自動応答)
    GetProfileStringEx(SYSTEM_REG, "DomainAUTHARC", "", mDomainAUTHARC, sizeof(mDomainAUTHARC));
#endif

    //GetProfileStringEx("SYSTEM\\CurrentControlSet\\Services\\SPARS","ImagePath", "", mProgPath, sizeof(mProgPath));
    GetProfileStringEx(SYSTEM_REG,"ImagePath", "", mProgPath, sizeof(mProgPath));
    for (nl = strlen(mProgPath)-1; nl > 0; nl--) {
	  if (mProgPath[nl] == '\\') {
		mProgPath[nl+1] = '\x0';
		break;
	  }
	}
	//////////////////////////////////////////////////////////////
	strcpy(mMailQueueDir, "incoming");
/*
#ifndef E_POST
    GetProfileStringEx("SYSTEM\\CurrentControlSet\\Services\\SMTPRS\\Parameters","MailQueueDir", "incoming", mMailQueueDir, sizeof(mMailQueueDir));
#endif
	if (stricmp(mMailQueueDir, "incoming") == 0)
      GetProfileStringEx(SYSTEM_PARAM_REG_RS,"MailQueueDir", "incoming", mMailQueueDir, sizeof(mMailQueueDir));
*/
	nl = strlen(mMailQueueDir);
	if (nl != 0) {
	  nl--;
	  if (mMailQueueDir[nl] != '\\')
	    strcat(mMailQueueDir,"\\");
	}
	//////////////////////////////////////////////////////////////
	//bTrace = GetProfileIntEx("SYSTEM\\CurrentControlSet\\Services\\SPARS", "Trace", (int)FALSE);
	bTrace = GetProfileIntEx(SYSTEM_REG_RS , "Trace", (int)FALSE);
	sprintf(mTraceFile,"%strace.dat", mProgPath);
	//bVirus = GetProfileIntEx("SYSTEM\\CurrentControlSet\\Services\\SPARS", "VirusCheck", (int)FALSE);
	bVirus = GetProfileIntEx(SYSTEM_REG_RS, "VirusCheck", (int)FALSE);
	sprintf(mVirusFile,"%svirus.dat", mProgPath);
	//nReceived = GetProfileIntEx("SYSTEM\\CurrentControlSet\\Services\\SPARS", "Received", (int)20); //FALSE);
	nReceived = GetProfileIntEx(SYSTEM_REG_RS, "Received", (int)20); //FALSE);
    //bAnnounce = GetProfileIntEx("SYSTEM\\CurrentControlSet\\Services\\SPARS", "Announce", (int)FALSE);
    bAnnounce = GetProfileIntEx(SYSTEM_REG_RS, "Announce", (int)FALSE);
	//bAnnLimit = GetProfileIntEx("SYSTEM\\CurrentControlSet\\Services\\SPARS", "AnnLimit", (int)TRUE);
	bAnnLimit = GetProfileIntEx(SYSTEM_REG_RS, "AnnLimit", (int)TRUE);
	//////////////////////////////////////////////////////////////
    GetProfileStringEx( "SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters", "NameServer", "", mNS, sizeof(mNS) );
	//////////////////////////////////////////////////////////////
#ifndef CLUSTERING
    //GetProfileStringEx("SOFTWARE\\EMWAC\\IMS","MailSpoolDir", "", mMailSpoolDir, sizeof(mMailSpoolDir));
    GetProfileStringEx(SOFT_REG,"MailSpoolDir", MAIL_SPOOL, mMailSpoolDir, sizeof(mMailSpoolDir));
#endif
	if (mMailSpoolDir[0] == '\x0')
	  strcpy(mMailSpoolDir, MAIL_SPOOL);
	tmp = mMailSpoolDir;
	mTempDir[0] = '\x0';
	if (tmp) {
	  while (*(tmp) =='%') {
		p = tmp+1;
		tmp = strstr(p, "%");
		*(tmp) = '\x0';
		tmp++;
	    strcat(mTempDir, getenv(p));
	  }
	  strcat(mTempDir, tmp);
	  nl = strlen(mTempDir);
	  if (nl != 0) {
	    nl--;
	    if (mTempDir[nl] != '\\')
	      strcat(mTempDir,"\\");
	  }
	}
	strcpy(mMailSpoolDir, mTempDir);
	tmp = strstr(mTempDir,":\\");
	if (tmp)
	  tmp = strstr(tmp+2,"\\");
#ifdef REGTOFILE
     else if ((tmp = strstr(mTempDir,"\\\\"))) {
       if ((tmp = strstr(tmp+2,"\\")))
         tmp = strstr(tmp+1,"\\");
	 }
#endif
    while(tmp) {
      *tmp = '\x0';
	  _mkdir(mTempDir);         // 処理用フォルダ作成
      *tmp = '\\';
	  tmp = strstr(tmp+1,"\\");
	}
#ifdef UPDATE_20050123
	sprintf(mTempDir, "%sexclusive", mMailSpoolDir);
	_mkdir(mTempDir);         // inlogフォルダ作成
    sprintf(mRCPLockFn, "%sexclusive\\rcp.lck", mMailSpoolDir);
	sprintf(mRetryLockFn, "%sexclusive\\retry.lck", mMailSpoolDir);
	sprintf(mRetryDomainLockFn, "%sexclusive\\retrydomain.lck", mMailSpoolDir);
	sprintf(mRetryListsLockFn, "%sexclusive\\retrylists.lck", mMailSpoolDir);
	sprintf(mThreadLockFn, "%sexclusive\\thread.lck", mMailSpoolDir);
	sprintf(mDomainsLockFn, "%sexclusive\\domain.lck", mMailSpoolDir);
	sprintf(mListsLockFn, "%sexclusive\\lists.lck", mMailSpoolDir);
    sprintf(mArticleLockFn, "%sexclusive\\article.lck", mMailSpoolDir);
#ifdef UPDATE_20070321 //DNSへの問合せテーブル初期化(初期化が無いと問合せで不定期にハングする可能性有り)
    sprintf(mDNSMXLockFn, "%sexclusive\\dnsmx.lck", mMailSpoolDir);
#endif
#endif
    sprintf(mTempDir, "%s%s", mMailSpoolDir, mMailQueueDir);
	_mkdir(mTempDir);         // incomingフォルダ作成
	sprintf(mLockFile, "%s%s", mTempDir, LOCKFILE);
	if (bLog) {
  	  sprintf(mTempDir, "%soutlog", mMailSpoolDir);
	  _mkdir(mTempDir);         // inlogフォルダ作成
#ifdef REGTOFILE
      if (nClustering) {
  	    sprintf(mTempDir, "%soutlog\\%s", mMailSpoolDir, mComName);
	    _mkdir(mTempDir);         // inlogフォルダ作成
	  }
#endif
	}
	if (bSendLocalLog) {
  	  sprintf(mTempDir, "%soutlocallog", mMailSpoolDir);
	  _mkdir(mTempDir);         // outlocallog"フォルダ作成
#ifdef REGTOFILE
      if (nClustering) {
  	    sprintf(mTempDir, "%soutlocallog\\%s", mMailSpoolDir, mComName);
	    _mkdir(mTempDir);         // inlogフォルダ作成
	  }
#endif
	}
	if (bFailLog) {
  	  sprintf(mTempDir, "%sfaillog", mMailSpoolDir);
	  _mkdir(mTempDir);         // inlogフォルダ作成
#ifdef REGTOFILE
      if (nClustering) {
  	    sprintf(mTempDir, "%sfaillog\\%s", mMailSpoolDir, mComName);
	    _mkdir(mTempDir);         // inlogフォルダ作成
	  }
#endif
	}
    if (nSenderLog || nSender2Log) {
      sprintf(mTempDir, "%ssenderlog", mMailSpoolDir);
  	  _mkdir(mTempDir);         // 接続ログフォルダ作成
#ifdef REGTOFILE
      if (nClustering) {
  	    sprintf(mTempDir, "%ssenderlog\\%s", mMailSpoolDir, mComName);
	    _mkdir(mTempDir);         // inlogフォルダ作成
	  }
#endif
	}
    if (bVirus) {
      sprintf(mTempDir, "%sviruslog", mMailSpoolDir);
  	  _mkdir(mTempDir);         // 接続ログフォルダ作成
#ifdef REGTOFILE
      if (nClustering) {
  	    sprintf(mTempDir, "%sviruslog\\%s", mMailSpoolDir, mComName);
	    _mkdir(mTempDir);         // inlogフォルダ作成
	  }
#endif
	}
	if (bAnnounce) {
      sprintf(mTempDir, "%sattach", mMailSpoolDir);
	  _mkdir(mTempDir);         // 広告用フォルダ作成
	}
#ifdef CLUSTERING
	if (nClustering) {
      sprintf(mTempDir, "%scluster", mMailSpoolDir);
	  _mkdir(mTempDir);         // 処理用フォルダ作成
	}
#endif
    sprintf(mTempDir, "%sdomains", mMailSpoolDir);
    _mkdir(mTempDir);         // domainフォルダ作成
    sprintf(mTempDir, "%sholding", mMailSpoolDir);
    _mkdir(mTempDir);         // holdフォルダ作成
    sprintf(mTempDir, "%slists", mMailSpoolDir);
    _mkdir(mTempDir);         // listsフォルダ作成
    sprintf(mTempDir, "%sdead", mMailSpoolDir);
    _mkdir(mTempDir);         // deadフォルダ作成
    sprintf(mTempDir, "%stemp", mMailSpoolDir);
	_mkdir(mTempDir);         // 処理用フォルダ作成
	strcat(mTempDir, "\\");
	//////////////////////////////////////////////////////////////
#ifdef  INCOM_SUB_FOLDER // incoming フォルダをスレッド別フォルダにして処理
    bIncomingSubFolder = GetProfileIntEx(SYSTEM_REG, "IncomingSubFolder", (int) FALSE); // incomingフォルダにスレッド別フォルダを作成 TRUE:する FALSE:しない
    if (bIncomingSubFolder) // incomingフォルダにスレッド別フォルダを作成 TRUE:する FALSE:しない
	{
	  for (nSubNum = 0; nSubNum < nMaxThread; nSubNum++)
	  {
		sprintf(mIncomingThread, "%s%s\\%05d", mMailSpoolDir, mMailQueueDir, nSubNum);
		_mkdir(mIncomingThread);
	  }
	}
#endif
	//////////////////////////////////////////////////////////////
	//// レジストリ内容の表示
	//////////////////////////////////////////////////////////////
	printf("---- regestry ----\n");
	printf("Licence key         %s (len=%d)\n", mLicencekey, strlen(mLicencekey));
#ifdef V3
	printf("User Manager        %s\n", bUserMan ? (bLDAP ? "LDAP use" : "NT use") : "SPA use");
	if (bUserMan)
	  printf("Membership          %s\n", (mAuthDomain[0] ? mAuthDomain : "workgroup"));
	else
	  printf("Account folder      %s\n", (mAuthDomain[0] ? mAuthDomain : "current"));
#else
	printf("Membership          %s\n", (mAuthDomain[0] ? mAuthDomain : "workgroup"));
#endif
#ifdef UPDATE_20060606 // ドメインコントローラへの接続リトライ
#ifdef UPDATE_20091103 // LDAP連携専用のリトライ回数指定の変数に変更
	printf("AD query retry      %lu(ms) x %lu(time)\n", nADRetryMSec, nADRetryTime);
#else
	printf("AD query retry time %lu\n", nADRetryTime);
#endif
#endif
#ifdef ADD_IDCACHE
	printf("AD Cache live time  %lu(s)\n", nIDCashLiveTime);
#endif
#ifdef UPDATE_20091103 // LDAP連携専用のリトライ回数指定の変数に変更
	if (bLDAPOn)
	{
	  printf("LDAP query retry    %lu(ms) x %lu(time)\n", nLDAPRetryMSec, nLDAPRetryTime);
	}
#endif

#ifdef V4
	printf("MX Cache live time  %lu(s)\n", nMXCashLiveTime);
#endif
#ifdef USE_SSL
	printf("Smtp over SSL       %s\n", (bUsedSSL ? "yes" : "no"));
#ifdef USE_STARTTLS
    printf("STARTTLS            %s\n", (bUsedSTLS ? "on" : "off"));
#endif
#endif
#ifdef IPv6
	printf("Host IP version     %s\n", nAddressFamily ? (nAddressFamily == 2 ? "IPv4/6" : "IPv6") :"IPv4");
#endif
#ifdef V3
	printf("Inbox MSG Encode    %s\n", (bInboxEnc ? "Encode" : "Plain"));
#endif
	printf("ESMTP               %s\n", (bESMTP ? "on" : "off"));
#ifdef SENDERID
    printf("Sender ID           %s\n", bSenderID ? "enable":"disable");
#endif
	printf("SMTP AUTH           %s\n", mSMTPAUTHMODE[0] == '\x0' ? "No" : mSMTPAUTHMODE);
	printf("SMTP AUTH ID        %s\n", mSMTPAUTHID[0] == '\x0' ? "(null)" : mSMTPAUTHID);
	printf("SMTP AUTH PASS      %s\n", mSMTPAUTHPASS[0] == '\x0' ? "(null)" : "**********"); //mSMTPAUTHPASS);
	printf("Resending interval  %d(s)\n", nChkTM);
	printf("Resending rule      %s\n", (bRetryRule ? "multiple":"constant"));
	printf("Resending refusal   %d(times)\n", nSendRefusalTime);
	printf("Resending abnormal  %d(h)\n", nSendMaxRetryTime);
	printf("Resending non-res   %d(h)\n", nSendOtherTime);
    printf("Send socket buff    ");
	if (nSendBuf > 0)
	  printf("%d\n", nSendBuf);
	else
	  printf("default\n");
	printf("Send Data Timer     %s %d(s)\n", (nSendTMO ? "on" : "off"), nSendTMO);
	printf("Recv Data Timer     %s %d(s)\n", (nRecvTMO ? "on" : "off"), nRecvTMO);
	//printf("Trace Mode        %s\n", bTrace ? "on":"off");
	//printf("VirusCheck        %s\n", bVirus ? "on":"off");
	printf("SMTP Gateway        %s\n", mSendGateway); 
	printf("Fowarding all mail to SMTP Gateway  %s\n", bMailForward ? "on":"off");
#ifdef UPDATE_20141105 // MXレコード参照失敗した受信をSMTP Gatewayへ転送
	printf("SMTP Gateway after DNS query  %s\n", bMailForward2 ? "on":"off");
#endif
	printf("Postmaster          %s\n", mPostmaster);
	printf("Fail reports        %s\n", bSendPostMaster ? "on":"off");
	printf("Smtp Port           %d\n", nport);
	printf("MailGroup           \"%s\"\n",mMailGroup);
	printf("LocalDomain List ");
	pldom = mLocalDomain;
	while (strlen(pldom)){
	  printf(" \"%s\"", pldom);
      pldom += strlen(pldom);
 	  pldom++;
	};
	printf("\n");
	printf("Carbon Copy List ");
	pldom = mMailInCopy;
 	while (strlen(pldom)){
	  printf(" \"%s\"", pldom);
      pldom += strlen(pldom);
 	  pldom++;
	};
	printf("\n");
#ifdef  INCOM_SUB_FOLDER // incoming フォルダをスレッド別フォルダにして処理
	printf("Thread Sub Folder   %s\n", bIncomingSubFolder ? "yes":"no");
#endif
	printf("Thread Type         %s=%d\n", bThreadType ? "multi":"single", bThreadType ? nMaxThread : 1);
	printf("ML max of divide    %d\n", nMaxDivide);
	printf("domain of divide    %d\n", nDomainDivide);
	printf("MailInMaxSize       %d\n", nMailInMaxSize);
    printf("MailInBoxSize       %d\n", nMailInBoxSize);
    printf("ReceivedHeader      %d\n", nReceived);
	printf("LastMsgId           %010lu\n", nLastMsgId);
	printf("MailQueueDir        %s\n", mMailQueueDir);
	printf("MailSpoolDir        %s\n", mMailSpoolDir);
	printf("MailBoxDir          %s\n", mMailBoxDir);
    printf("Program Path        %s\n", mProgPath);
    printf("NameServer          %s\n", mNS);
	printf("OutLog              %s\n", bLog ? "on":"off");
	printf("OutLocalLog         %s\n", bSendLocalLog ? "on":"off");
	printf("FailLog             %s\n", bFailLog ? "on":"off");
    printf("SenderLog           ");
	if (nSenderLog > 0)
	  printf("Level %d\n", nSenderLog);
	else
	  printf("off\n");
#ifdef WAX_ON
	printf("WaxPipe             %s\n", mWaxPipe);
#endif
#ifdef UPDATE_20150108 // 送信許可時間の設定機能を追加
	printf("UseTimeFileEnabled  %s\n", nUseTime == 0 ? "off" : (nUseTime == 1 ? "on(TO)" : "on(FROM & TO)"));
	printf("UseTimeFile         %s\n", mUseTimeFile);
#endif
	printf("-------------------\n");
	//////////////////////////////////////////////////////////////
#ifndef UPDATE_20070425 // MSCSのスタンバイ側に対応
    ////////////////////////////////
    // Service initialization
    if (!ReportStatusToSCMgr(
        SERVICE_START_PENDING, // service state
        NO_ERROR,              // exit code
        3000))                 // wait hint
        goto cleanup;
    // create the event object. The control handler function signals
    // this event when it receives the "stop" control code.
    hServerStopEvent = CreateEvent(
        NULL,    // no security attributes
        TRUE,    // manual reset event
        FALSE,   // not-signalled
        NULL);   // no name
    if ( hServerStopEvent == NULL)
        goto cleanup;
    hEvents[0] = hServerStopEvent;
    // report the status to the service control manager.
    //
    if (!ReportStatusToSCMgr(
        SERVICE_START_PENDING, // service state
        NO_ERROR,              // exit code
        3000))                 // wait hint
        goto cleanup;
    // create the event object object use in overlapped i/o
    //
    hEvents[1] = CreateEvent(
        NULL,    // no security attributes
        TRUE,    // manual reset event
        FALSE,   // not-signalled
        NULL);   // no name
    if ( hEvents[1] == NULL)
        goto cleanup;
    // report the status to the service control manager.
    //
    if (!ReportStatusToSCMgr(
        SERVICE_START_PENDING, // service state
        NO_ERROR,              // exit code
        3000))                 // wait hint
        goto cleanup;
    // create a security descriptor that allows anyone to write to
    //  the pipe...
    //
    pSD = (PSECURITY_DESCRIPTOR) malloc( SECURITY_DESCRIPTOR_MIN_LENGTH );
    if (pSD == NULL)
        goto cleanup;
    if (!InitializeSecurityDescriptor(pSD, SECURITY_DESCRIPTOR_REVISION))
        goto cleanup;
    // add a NULL disc. ACL to the security descriptor.
    if (!SetSecurityDescriptorDacl(pSD, TRUE, (PACL) NULL, FALSE))
        goto cleanup;

    sa.nLength = sizeof(sa);
    sa.lpSecurityDescriptor = pSD;
    sa.bInheritHandle = TRUE;
    // report the status to the service control manager.
    //
    if (!ReportStatusToSCMgr(
        SERVICE_START_PENDING, // service state
        NO_ERROR,              // exit code
        3000))                 // wait hint
        goto cleanup;
/*
    // allow user tp define pipe name
    for ( ndx = 1; ndx < dwArgc-1; ndx++ )
    {

        if ( ( (*(lpszArgv[ndx]) == TEXT('-')) ||
               (*(lpszArgv[ndx]) == TEXT('/')) ) &&
             _tcsicmp( TEXT("pipe"), lpszArgv[ndx]+1 ) == 0 )
        {
            lpszPipeName = lpszArgv[++ndx];
        }

    }
*/
    // open our named pipe...
    //
    hPipe = CreateNamedPipe(
                    lpszPipeName         ,  // name of pipe
                    FILE_FLAG_OVERLAPPED |
                    PIPE_ACCESS_INBOUND, //PIPE_ACCESS_DUPLEX,     // pipe open mode
                    PIPE_TYPE_MESSAGE |
                    PIPE_READMODE_MESSAGE |
                    PIPE_WAIT,              // pipe IO type
                    1,                      // number of instances
                    0,                      // size of outbuf (0 == allocate as necessary)
                    0,                      // size of inbuf
                    1000,                   // default time-out value
                    &sa);                   // security attributes

    if (hPipe == INVALID_HANDLE_VALUE) {
        AddToMessageLog(TEXT("The named pipe could not be made."), 202, TEXT("%s error: %d"), EVENTLOG_ERROR_TYPE);
        goto cleanup;
    }
    
    // report the status to the service control manager.
    //
    if (!ReportStatusToSCMgr(
        SERVICE_RUNNING,       // service state
        NO_ERROR,              // exit code
        0))                    // wait hint
        goto cleanup;

    //
    // End of initialization
    //
    ////////////////////////////////////////////////////////
#endif UPDATE_20070425 //#ifndef MSCSのスタンバイ側に対応

    ////////////////////////////////////////////////////////
    //
    // Service is now running, perform work until shutdown
    //

    // WINSOCK Start up
#ifdef IPv6
    if ((status = WSAStartup(MAKEWORD(2,2), &WSAData)) != 0)
       goto cleanup;
#else
    if ((status = WSAStartup(MAKEWORD(1,1), &WSAData)) != 0)
       goto cleanup;
#endif

    AddToMessageLog(TEXT(SMTP_NAME), 0, TEXT(VERSION), (WORD)EVENTLOG_INFORMATION_TYPE);
    // init the overlapped structure
    //
	dwc = 0;
	dwWait = 1;
	nFolderWoker = 0;
	bWaitDNS = bMRICounter = bOutlog = bOutLocallog = bFailOutlog = bSenderlog = bThread = bThreadRetry = bThreadRetry2 =bThreadDomain = bThreadLists = FALSE;
    sprintf(mMQ, "%s%s", mMailSpoolDir, mMailQueueDir);
    Copyright(&bPassport);
    memset( &os, 0, sizeof(OVERLAPPED) );
    os.hEvent = hEvents[1];
    ResetEvent( hEvents[1] );
#ifdef CLUSTERING
	hF = NULL;
    while ((hF = CreateFile( (LPCTSTR)mDomainsLockFn,
                        GENERIC_WRITE,
                        0,   // 排他アクセス = 0
                        NULL,
                        CREATE_ALWAYS,
                        FILE_ATTRIBUTE_NORMAL, 
                        NULL)) == INVALID_HANDLE_VALUE) {
     if (bServiceTerminating)
       break;
	 _sleep(WAIT_TIME);
   } 
#endif
#ifdef UPDATE_20060901 // listsフォルダのクリアが出来ない
#ifdef CLUSTERING // ホットスタンバイモードの処理
    if (HotStandby(mComName, mLogicalIP)) {
#endif
      RecoverFolder("domains");
      RecoverFolder("lists");
#ifdef CLUSTERING // ホットスタンバイモードの処理
	}
#endif
#else
#ifdef CLUSTERING // ホットスタンバイモードの処理
    if (HotStandby(mComName, mLogicalIP))
#endif
      RecoverFolder("domains");
    //RecoverFolder("lists");
#endif
#ifdef CLUSTERING
	if (hF)
	  CloseHandle(hF);
	hF = NULL;
    while ((hF = CreateFile( (LPCTSTR)mRCPLockFn,
                        GENERIC_WRITE,
                        0,   // 排他アクセス = 0
                        NULL,
                        CREATE_ALWAYS,
                        FILE_ATTRIBUTE_NORMAL, 
                        NULL)) == INVALID_HANDLE_VALUE) {
     if (bServiceTerminating)
       break;
	 _sleep(WAIT_TIME);
   } 
#endif

#ifdef CLUSTERING // ホットスタンバイモードの処理
    if (HotStandby(mComName, mLogicalIP)) 
	{
#endif
#ifdef  INCOM_SUB_FOLDER // incoming フォルダをスレッド別フォルダにして処理
	if (bIncomingSubFolder)
	{
	  for (nSubNum = 0; nSubNum < nMaxThread; nSubNum++)
	  {
		sprintf(mIncomingThread, "%s%05d", mMQ, nSubNum);
	    RecoverRCPFile(mIncomingThread);
	    RecoverOKNGFile(mIncomingThread);
	  }
	}
#endif
	RecoverRCPFile(mMQ);
	RecoverOKNGFile(mMQ);
#ifdef CLUSTERING // ホットスタンバイモードの処理
    }
#endif

#ifdef CLUSTERING
	if (hF)
	  CloseHandle(hF);
	hF = NULL;
    while ((hF = CreateFile( (LPCTSTR)mDomainsLockFn,
                        GENERIC_WRITE,
                        0,   // 排他アクセス = 0
                        NULL,
                        CREATE_ALWAYS,
                        FILE_ATTRIBUTE_NORMAL, 
                        NULL)) == INVALID_HANDLE_VALUE) {
     if (bServiceTerminating)
       break;
	 _sleep(WAIT_TIME);
   } 
#endif
#ifdef CLUSTERING // ホットスタンバイモードの処理
    if (HotStandby(mComName, mLogicalIP))
#endif
     RecoverDomainMSGFile(mMailSpoolDir);
#ifdef CLUSTERING
	if (hF)
	  CloseHandle(hF);
#endif
	strcpy(mSmtpds.mNS, mNS);
	strcpy(mSmtpds.mMQ, mMQ);

#ifdef USE_SSL
#ifdef UPDATE_20060512 // SSLの初期化はサービス開始で１回のみ
#ifndef UPDATE_OPESSL1_1_1 // openssl1.1.1系に置換えた（SSL2を廃止し、TSL1.3を使用可能にする為）
    SSL_library_init();
    SSLeay_add_ssl_algorithms();
    SSL_load_error_strings();
#endif
#endif
#endif

#ifdef UPDATE_20070423 // MXキャッシュを全て削除する
    if (bMXAutoClean)
      CleanMXCashList();
#endif
#ifdef UPDATE_20231007 // STARTTLSで暗号化ネゴシェーションに失敗するドメインリストの作成と失敗先へのプレーン送信への切替対策
    if (bBadSTLSDom)
      CleanSTLSCheck();
#endif
//bDebug = FALSE;
#ifdef  INCOM_SUB_FOLDER // incoming フォルダをスレッド別フォルダにして処理
    if (bIncomingSubFolder)
    {
	  if (!(pSmtpds = LocalAlloc( LPTR, sizeof(struct _SMTPDS)*(nMaxThread+1))))
	  {
	    if (bDebug) printf("memory allocation fail.\n");
	  }
    }
#endif
#ifdef UPDATE_20140711 // エラーメール生成時の"multipart/report"に対応するオプションを追加した。
    gethostaddrname(mMyFDQN);
	// ドメインが無いときに最初の管理ドメイン名を付加
	if (!strchr(mMyFDQN, '.')) {
	  if (strchr(mHostName, '.'))
	  {
	    strcpy(mMyFDQN, mHostName);
	  } else {
	    strcat(mMyFDQN, ".");
	    strcat(mMyFDQN, mLocalDomain);
	  }
	}
#endif

#ifdef  INCOM_SUB_FOLDER // incoming フォルダをスレッド別フォルダにして処理
    while (pSmtpds)
#else
    while ( 1 )
#endif
    {
#ifdef UPDATE_20080111 // MSCSの時、フェールオーバー遷移中のディスクアクセス処理の修正
	  if (QueryDrive(mMailSpoolDir) &&
		  HotStandby(mComName, mLogicalIP))
#else
#ifdef CLUSTERING // ホットスタンバイモードの処理
	  if (HotStandby(mComName, mLogicalIP))
#endif
#endif
	  {
        if (!GetTimeLimit(&ltbegin, mmode, &bPassport)) { // 使用期限が切れたまま利用ならスレッド数を２以内にする
		  if (bDebug)
			printf("%s\n", mmode);
		  nMaxThread = min(2, nMaxThread);
		}
	    mSmtpds.dwWait = dwWait;
		if (bThreadType)
		{
#ifdef  INCOM_SUB_FOLDER // incoming フォルダをスレッド別フォルダにして処理
	      if (bIncomingSubFolder)
		  {
			////////////////////////////////////////
            strcpy(pSmtpds->mMQ, mMQ);
            nRunThread = nMaxThread;
			pSmtpds->dwNo = 0;
            if (bDebug) printf("[%05d]:beginthread()->SMTPDSIncomingA = %s\n", pSmtpds->dwNo, pSmtpds->mMQ);
	        hTd = _beginthread( SMTPDSIncomingA, sizeof(struct _SMTPDS), (struct _SMTPDS *)(pSmtpds));
			if (bDebug) printf("[%05d]:beginthread()->SMTPDSIncomingA %s.\n", pSmtpds->dwNo, (hTd == -1L ? "fail" :  "success"));
            (pSmtpds+0)->dwWait = WAIT_TIMEOUT;
			////////////////////////////////////////
		    for (nCnt = 0; nCnt < nMaxThread; nCnt++)
			{
		      sprintf(mIncomingThread, "%s%05d\\", mMQ, nCnt);
	          strcpy((pSmtpds+nCnt)->mMQ, mIncomingThread);
	          nRunThread = nMaxThread;
			  (pSmtpds+nCnt)->dwNo = nCnt;
              if (bDebug) printf("[%05d]:beginthread()->SMTPDSIncomingA = %s\n", (pSmtpds+nCnt)->dwNo, (pSmtpds+nCnt)->mMQ);
	          hTd = _beginthread( SMTPDSIncomingA, sizeof(struct _SMTPDS), (struct _SMTPDS *)(pSmtpds+nCnt));
			  if (bDebug) printf("[%05d]:beginthread()->SMTPDSIncomingA %s.\n", (pSmtpds+nCnt)->dwNo, (hTd == -1L ? "fail" :  "success"));
              (pSmtpds+nCnt)->dwWait = WAIT_TIMEOUT;
			}
		  } else 
#endif
		  {
if (bDebug) printf("0:nRunThread = %d < nMaxThread = %d\n", nRunThread, nMaxThread);
#ifdef NEW_SENDER
#ifdef UPDATE_20050123
		    if (nRunThread < nMaxThread)
			{
#endif
#ifdef  INCOM_SUB_FOLDER // incoming フォルダをスレッド別フォルダにして処理
			  mSmtpds.dwNo = nRunThread;
#endif
	          nRunThread++;
	          hTd = _beginthread( SMTPDSIncomingA, sizeof(struct _SMTPDS), (struct _SMTPDS *)&mSmtpds);
              mSmtpds.dwWait = WAIT_TIMEOUT;
#ifdef UPDATE_20050123
			}
#endif
		  }
#else
		  nNum = NumberRCPFile(mMQ);
		  for (nCnt = nRunThread; nCnt < (nNum <= nMaxThread ? (nNum == 0 ? 1: nNum+1) : nMaxThread) ; nCnt++) {// incomingの処理スレッド数を決定
            bTd = FALSE;
			hTd = 0;
			nRunThread++;
			hTd = _beginthread( SMTPDSMain, sizeof(struct _SMTPDS) /*0*/, (struct _SMTPDS *)&mSmtpds);
            mSmtpds.dwWait = WAIT_TIMEOUT;
		  }
#endif
		} else {
#ifdef NEW_SENDER
		  SMTPDSIncomingA((struct _SMTPDS *)&mSmtpds);
#else
		  SMTPDSMain((struct _SMTPDS *)&mSmtpds);
#endif
		}
#ifdef CLUSTERING // ホットスタンバイモードの処理の終わり
	  } 
#endif
	    if (bServiceTerminating || !bPassport)    // if server stop signaled then exit.
		  break;
		os.Internal = 0; 
        os.InternalHigh = 0;
        os.Offset = 0; 
        os.OffsetHigh = 0; 
		ConnectNamedPipe(hPipe, &os);
        if ( GetLastError() == ERROR_IO_PENDING ) 
		{
		  ResetEvent( hEvents[0] );
		  ResetEvent( hEvents[1] );
		  //ResetEvent( hEvents[2] );
          dwWait = WaitForMultipleObjects( 2, hEvents, FALSE, nChkTM*1000);
          if ( dwWait == 0 ) {     // if server stop signaled then exit.
#ifdef UPDATE_20050121
			dwerr = GetLastError();
			StopLog(dwerr);
			if (dwerr == ERROR_OPERATION_ABORTED)
#endif
              break;
		  }
		}  
		ResetEvent(os.hEvent);
        // drop the connection...
        DisconnectNamedPipe(hPipe);
	}
#ifdef USE_SSL
#ifdef UPDATE_20060512 // SSLの初期化はサービス開始で１回のみ
#ifndef UPDATE_OPESSL1_1_1 // openssl1.1.1系に置換えた（SSL2を廃止し、TSL1.3を使用可能にする為）
        ERR_free_strings();
#endif
#endif
#endif
#ifdef  INCOM_SUB_FOLDER // incoming フォルダをスレッド別フォルダにして処理
    if (pSmtpds)
    {
	  LocalFree(pSmtpds);
	}
#endif
    WSACleanup();

  cleanup:

    if (hPipe != INVALID_HANDLE_VALUE )
        CloseHandle(hPipe);

    if (hServerStopEvent)
        CloseHandle(hServerStopEvent);

    if (hEvents[1]) // overlapped i/o event
        CloseHandle(hEvents[1]);

    if ( pSD )
        free( pSD );
}

#ifdef UPDATE_20050121

void StopLog(DWORD nCode) {
  SYSTEMTIME lt;
  char   mdata[128], fn[256];
  FILE   *fp;

  GetLocalTime(&lt); 
  sprintf(mdata, "%02d%02d%02d", (lt.wYear%100), lt.wMonth, lt.wDay );

#ifdef REGTOFILE
      if (nClustering) {
	    sprintf(fn, "%s\\stop\\%s\\%s.log", mMailSpoolDir, mComName, mdata);
	  } else {
#endif
  sprintf(fn, "%s\\stop\\%s.log", mMailSpoolDir, mdata);
#ifdef REGTOFILE
	  }
#endif
  fp = fopen(fn, "at");
  if (fp) {
    fprintf(fp, "[%02d:%02d:%02d] Code = %d\n", lt.wHour, lt.wMinute, lt.wSecond, nCode);
    fclose(fp);
  }

}

#endif

#ifdef CLUSTERING
BOOL HotStandby(char *pMacine, char *pLogicalIP) {
  char  mTkn[256], *pDom;
  struct in_addr *inDom;
  int    i = 0;
  BOOL   nsts = FALSE;
  struct hostent *ph;

  if (*pLogicalIP == '\x0') // ＩＰ指定がないならチェックしない
	return TRUE;
  /////////////////////////////////////
  // 相手マシンの実IPアドレスが生きているか確認
  ph = gethostbyname(pMacine);
  if (ph) {
    while((inDom = (struct in_addr *)*(ph->h_addr_list+i))) {
      //inDom = (struct in_addr *)*(ph->h_addr_list+i);
      if (inDom) {
        pDom = inet_ntoa(*inDom);
		if (!strcmp(pLogicalIP, pDom)) {
		  nsts = TRUE;
		  break;
		}
	  }
	  i++;
	}
  }

  return nsts;
}
#endif

VOID ServiceStop()
{
    char mMQ[MAXDNAME];
#ifdef CLUSTERING
	HANDLE hF;
#endif

	bServiceTerminating = TRUE;
    if ( hServerStopEvent )
        SetEvent(hServerStopEvent);
    fcloseall();
    sprintf(mMQ, "%s%s", mMailSpoolDir, mMailQueueDir);
#ifdef CLUSTERING
	hF = NULL;
    while ((hF = CreateFile( (LPCTSTR)mDomainsLockFn,
                        GENERIC_WRITE,
                        0,   // 排他アクセス = 0
                        NULL,
                        CREATE_ALWAYS,
                        FILE_ATTRIBUTE_NORMAL, 
                        NULL)) == INVALID_HANDLE_VALUE) {
     if (bServiceTerminating)
       break;
	 _sleep(WAIT_TIME);
   } 
#endif
    RecoverFolder("domains");
#ifdef CLUSTERING
	if (hF)
	  CloseHandle(hF);
	hF = NULL;
    while ((hF = CreateFile( (LPCTSTR)mListsLockFn,
                        GENERIC_WRITE,
                        0,   // 排他アクセス = 0
                        NULL,
                        CREATE_ALWAYS,
                        FILE_ATTRIBUTE_NORMAL, 
                        NULL)) == INVALID_HANDLE_VALUE) {
     if (bServiceTerminating)
       break;
	 _sleep(WAIT_TIME);
   } 
#endif
    RecoverFolder("lists");
#ifdef CLUSTERING
	if (hF)
	  CloseHandle(hF);
	hF = NULL;
    while ((hF = CreateFile( (LPCTSTR)mRCPLockFn,
                        GENERIC_WRITE,
                        0,   // 排他アクセス = 0
                        NULL,
                        CREATE_ALWAYS,
                        FILE_ATTRIBUTE_NORMAL, 
                        NULL)) == INVALID_HANDLE_VALUE) {
     if (bServiceTerminating)
       break;
	 _sleep(WAIT_TIME);
   } 
#endif
	RecoverRCPFile(mMQ);
	RecoverOKNGFile(mMQ);
#ifdef CLUSTERING
	if (hF)
	  CloseHandle(hF);
	hF = NULL;
    while ((hF = CreateFile( (LPCTSTR)mDomainsLockFn,
                        GENERIC_WRITE,
                        0,   // 排他アクセス = 0
                        NULL,
                        CREATE_ALWAYS,
                        FILE_ATTRIBUTE_NORMAL, 
                        NULL)) == INVALID_HANDLE_VALUE) {
     if (bServiceTerminating)
       break;
	 _sleep(WAIT_TIME);
   } 
#endif
    RecoverDomainMSGFile(mMailSpoolDir);
#ifdef CLUSTERING
	if (hF)
	  CloseHandle(hF);
#endif
    return;
}

#ifdef UPDATE_20140711 // エラーメール生成時の"multipart/report"に対応するオプションを追加した。
BOOL gethostaddrname(char *hostname) 
{
  char szHostName[256];
  int  iResult;
#ifdef IPv6
  char Buffer[INET6_ADDRSTRLEN];
  char **Item;
  PHOSTENT host = NULL;
  int Error;
#endif
  HOSTENT FAR * lphostent = NULL;
#ifndef VC6
  HOSTENT hst;
  struct addrinfo Hints, *AddrInfo;
#endif

  strcpy(hostname,"");
  if (mHostName[0] == '\x0') {
    iResult = gethostname(szHostName, sizeof(szHostName));
    if (iResult != 0)
	  return FALSE;
  } else {
	strcpy(szHostName, mHostName);
  }
  if (lstrcmp(szHostName,"") == 0)
	return FALSE;
#ifdef IPv6
  Error = 0;
#ifdef VC6
  lphostent = getipnodebyname(szHostName, (nAddressFamily == 1 ? AF_INET6 : AF_INET),  (nAddressFamily ? AI_ALL : 0), &Error);
#else
  lphostent = NULL;
  memset(&Hints, 0, sizeof(Hints));
  Hints.ai_flags = AI_CANONNAME;
  Hints.ai_family =  (nAddressFamily == 1 ? AF_INET6 : AF_INET);
  Hints.ai_socktype = SOCK_STREAM;
  if (getaddrinfo(szHostName, NULL, &Hints, &AddrInfo) == 0) {
    hst.h_name = AddrInfo->ai_canonname;
 	lphostent = &hst;
  }
#endif

  if (!lphostent) {
    gethostname(szHostName, sizeof(szHostName));
#ifdef VC6
    lphostent = getipnodebyname(szHostName, (nAddressFamily == 1 ? AF_INET6 : AF_INET),  (nAddressFamily ? AI_ALL : 0), &Error);
#else
    lphostent = NULL;
     memset(&Hints, 0, sizeof(Hints));
     Hints.ai_flags = AI_CANONNAME;
     Hints.ai_family =  (nAddressFamily == 1 ? AF_INET6 : AF_INET);
     Hints.ai_socktype = SOCK_STREAM;
    if (getaddrinfo(szHostName, NULL, &Hints, &AddrInfo) == 0) {
      hst.h_name = AddrInfo->ai_canonname;
 	  lphostent = &hst;
	}
#endif
    if (!lphostent)
	  return FALSE;
  } 
  strcpy(hostname, lphostent->h_name);
//#ifdef UPDATE_20060515 // SOCKET関連のメモリ開放抜け
   if (lphostent)
#ifdef VC6
     freehostent(lphostent);
#else
     freeaddrinfo(AddrInfo);
#endif
   /*
#else
  if (LocalFlags(lphostent) != LMEM_INVALID_HANDLE) // 解放可能かどうか確認
    LocalFree(lphostent);
#endif
*/
  return TRUE;
#else
  lphostent = gethostbyname(szHostName);
  if (!lphostent) {
    gethostname(szHostName, sizeof(szHostName));
    lphostent = gethostbyname(szHostName);
    if (!lphostent)
	  return FALSE;
  }//else {
	strcpy(hostname, lphostent->h_name);
	return TRUE;
  //}
#endif
}
#endif