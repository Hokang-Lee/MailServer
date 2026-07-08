////////////////////////////////////////////////////////////
// imap4.c Copyright K.kawakami
////////////////////////////////////////////////////////////
#include "imap4.h"
#include "version.h"
#include "service.h"
#include <time.h>
#include <tchar.h>
#include <direct.h>

#ifdef SOCKET_FAIL_TEST // 強制テスト：テスト的にエラーを発生 ソケット無応答状態の動作調査用
BOOL bFT = TRUE;
#endif

HANDLE  hServerStopEvent = NULL;

// 非認証状態コマンド
char *  NoAuthCommands[] = { IMAP4_CAPABILITY,
							 IMAP4_NOOP,
							 IMAP4_LOGOUT,
							 IMAP4_AUTHENTICATE, 
							 IMAP4_LOGIN
							};

// 認証済状態コマンド
char *  OnAuthCommands[] = { IMAP4_CAPABILITY, 
						 	 IMAP4_NOOP,
							 IMAP4_LOGOUT,
							 IMAP4_SELECT, 
							 IMAP4_EXAMINE, 
							 IMAP4_CREATE,
							 IMAP4_DELETE,
							 IMAP4_RENAME,
							 IMAP4_SUBSCRIBE,
							 IMAP4_UNSUBSCRIBE,
							 IMAP4_LIST,
							 IMAP4_LSUB,
							 IMAP4_STATUS,
#ifdef ADD_ID_XDELTAPUSH
						     IMAP4_ID,
#endif
#ifdef ADD_METADATA
						     IMAP4_SETMETADATA,
							 IMAP4_GETMETADATA,
#endif
 							 IMAP4_APPEND
						   };

// 選択済状態コマンド
char *  OnFolderCommands[] = { IMAP4_CAPABILITY, 
						 	   IMAP4_NOOP,
							   IMAP4_LOGOUT,
							   IMAP4_CHECK,
							   IMAP4_CLOSE,
							   IMAP4_EXPUNGE,
							   IMAP4_SEARCH,
							   IMAP4_FETCH,
							   IMAP4_STORE,
							   IMAP4_COPY,
							   IMAP4_UID
							  };

BOOL   IMAP4SMain(void);
//////////////////////////////////////////////////////////
BOOL CAPABILITYDispatch(PCLIENT_CONTEXT lpClientContext);
BOOL NOOPDispatch(PCLIENT_CONTEXT lpClientContext);
BOOL LOGOUTDispatch(PCLIENT_CONTEXT lpClientContext);
BOOL AUTHENTICATEDispatch(PCLIENT_CONTEXT lpClientContext);
BOOL LOGINDispatch(PCLIENT_CONTEXT lpClientContext);
BOOL SELECTDispatch(PCLIENT_CONTEXT lpClientContext);
BOOL EXAMINEDispatch(PCLIENT_CONTEXT lpClientContext);
BOOL CREATEDispatch(PCLIENT_CONTEXT lpClientContext);
BOOL DELETEDispatch(PCLIENT_CONTEXT lpClientContext);
BOOL RENAMEDispatch(PCLIENT_CONTEXT lpClientContext);
BOOL SUBSCRIBEDispatch(PCLIENT_CONTEXT lpClientContext);
BOOL UNSUBSCRIBEDispatch(PCLIENT_CONTEXT lpClientContext);
BOOL LISTDispatch(PCLIENT_CONTEXT lpClientContext);
BOOL LSUBDispatch(PCLIENT_CONTEXT lpClientContext);
BOOL STATUSDispatch(PCLIENT_CONTEXT lpClientContext);
BOOL APPENDDispatch(PCLIENT_CONTEXT lpClientContext);
BOOL NAMESPACEDispatch(PCLIENT_CONTEXT lpClientContext);
BOOL CHECKDispatch(PCLIENT_CONTEXT lpClientContext);
BOOL CLOSEDispatch(PCLIENT_CONTEXT lpClientContext);
BOOL EXPUNGEDispatch(PCLIENT_CONTEXT lpClientContext);
BOOL SEARCHDispatch(PCLIENT_CONTEXT lpClientContext);
BOOL FETCHDispatch(PCLIENT_CONTEXT lpClientContext);
BOOL STOREDispatch(PCLIENT_CONTEXT lpClientContext);
BOOL COPYDispatch(PCLIENT_CONTEXT lpClientContext);
BOOL UIDDispatch(PCLIENT_CONTEXT lpClientContext);
BOOL BADDispatch(PCLIENT_CONTEXT lpClientContext);
#ifdef ADD_ID_XDELTAPUSH
BOOL IDDispatch(PCLIENT_CONTEXT lpClientContext);
#endif
#ifdef ADD_METADATA
BOOL SETMETADATADispatch(PCLIENT_CONTEXT lpClientContext);
BOOL GETMETADATADispatch(PCLIENT_CONTEXT lpClientContext);
#endif
//////////////////////////////////////////////////////////
void   imap4log(PCLIENT_CONTEXT lpClientContext, char *bsts);
BOOL   AcceptClients (HANDLE hCompletionPort);
HANDLE InitializeThreads (void);
#ifdef Y2038_BUG
BOOL GetTimeLimit(__int64 *ltbegin, char *mode, BOOL *bPassport);
#else
BOOL GetTimeLimit(time_t *ltbegin, char *mode, BOOL *bPassport);
#endif

extern BOOL  bDebug;

#ifdef CLUSTERING
BOOL   nClustering;
char   mComName[256];
#endif
int    nReadyDriveTime;

#ifdef Y2038_BUG
char mMonth[12][4] = {"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};
char mWeek[7][4]   = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
#endif
BOOL    bSequenceOK; // シーケンス実行許可

TMQueue mTMQueue;
DWORD   nTMOut;
DWORD   nTransTMOut;
#ifdef UPDATE_20140522 // IMAPでフォルダを選択した時、排他処理フラグが同期されずに以降の処理をロックさせてしまう場合がある
DWORD   nLockTMOut;
#endif
#ifdef UPDATE_20150608 // 検索時の日付をUTC日付かローカル日付かに変更できるオプションを追加した
BOOL	bSearchTime;
#endif
#ifdef UPDATE_20150609 // フォルダへのコピー時に受信日付がコピー日付になってしまった不具合 0:作成日時 1:更新日時 2:アクセス日時
int	    nSearchType;
#endif

BOOL    bTMQWait;
BOOL    bThreadWait;
int     nSendBuf;

CHAR    mInboxAlias[MAX_PATH];
#ifdef V4
INT     nMAXUser;
#endif
#ifdef LDAP_ON         // LDAPへのログイン処理でSMTP認証の確認を行うオプション // //CRAM-MD5は複合できないので、これを利用する場合は、PLAINかLOGINのみとします。
BOOL   bLDAPOn;
BOOL   bLDAP;
BOOL   bLDAPLicense;
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
#ifdef UPDATE_20170223 // フォルダ削除でのメール一括削除オプション追加
BOOL   bAutoSubDirAndMsg;
#endif
#ifdef UPDATE_20161013 // ログイン成功時にルートフォルダが存在しない場合、自動作成を試みるようにした。
BOOL   bAutoMakeDir;
#endif
#ifdef V3
BOOL    bUserMan;
#endif
DWORD   nLogInID;
#ifdef UPDATE_20090130 // レシピエントファイル(RCP)があれば一緒にコピーさせる
BOOL    bIgnoreRCP; // TRUE: RCPファイル無視, FALSE:RCPファイル無視しない
#endif
#ifdef UPDATE_20091110 // POP3メールボックスフォルダ位置を変更するオプション
BOOL    bPOP3Share;
#endif
BOOL    bOFFPOP3;  // POP3にデータをさない
char    mMailExtension[32];
int     nPriority;
int     nSendTMO;
DWORD   nAcceptCount;
DWORD   nAcceptLimit;
int     nAddressFamily;
#ifdef ADD_IDCACHE
int     nIDCashLiveTime; // ADキャッシュ利用有効時間(秒単位)
#endif
#ifdef UPDATE_20151215 // 高負荷かつ同一アカウントへ複数ログイン時にIMAP用INBOXへのメール確認で重複メールが作成される可能性がある不具合
int 	nIMAPIndexLiveTime; // imap.idx有効時間
#endif
#ifdef UPDATE_20151225 // 試作：UID値によるファイル範囲指定のチューニング
int     bQueryFileArea; // UID値範囲指定時の処理方法選択 デフォルト FALSE:従来方式 TRUE:範囲指定方式
#endif
#ifdef UPDATE_20151226 // IDLE処理の修正
int     bIDLEBroadcast; // IDLE時の参加セッションへ同報するか否かの選択 デフォルト 0:同報しない 1:同報する
#endif
#ifdef UPDATE_20230714E // Content-Disposition:ヘッダの"filename="の項目のみ出力するオプションを追加　デフォルト 0:filenameのみ 1:他含む
BOOL    bDisposition; // Content-Disposition:ヘッダの"filename="の項目のみ出力するオプションを追加　デフォルト 0:filenameのみ 1:他含む
#endif

int     nConnectAF;
int     nport;
BOOL    bLog;
BOOL    bAcceptLog;
//BOOL    bExApop;  //排他的APOP認証フラグ
//BOOL    bAcceptLog;
DWORD   nMailInMaxSize;
DWORD   nMailInBoxSize;
char    mProgPath[256];
char    mdomain[256];
char    mMailGroup[128];
BOOL    bCountLock;
BOOL    bCloseLock;
DWORD   nLastMsgId;
char    mLog[2048];
char    mAuthDomain[64];
char    mHostName[256];
char    mLocalDomain[0x4000]; // レジストリ：バイナリの限界値 [4096];
DWORD   nLocalDomain;
char    mMailInCopy[4096];
DWORD   nMailInCopy;
char    mMailSpoolDir[128];
char    mMailQueueDir[128];
CHAR    mMailBoxDir[_MAX_PATH];
CHAR    mSharedRoot[_MAX_PATH];
BOOL    bSharedRW;
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
char    mPasswordFile[128];
#ifdef UPDATE_20210224 // ADアカウントまたは、WINDOWSアカウント運用時のAUTHENTICATE LOGIN/PLAINでのパスワードファイルの参照を無視する
BOOL    bSimpleAuth;
#endif
#ifdef V4
BOOL    bHide;
char    mUseTimeFile[128];
#endif
BOOL    bListenMode;
char    mListenIP[1024];
char    mListenIP2[1024];

#ifdef UPDATE_20110301B // サービス停止で残ったユーザ毎のロックファイルをサービス再起動後に一旦削除する対策
CHAR   mImapStartFile[256];
#endif
#ifdef UPDATE_20110222 //UPDATE_20110202でパフォーマンス改善の対策。
BOOL   bUIDRecover;
#endif
#ifdef UPDATE_20190510 // NTFS以外のファイルシステムからのファイル一覧取得可能にする対策（一覧ソート）を追加
BOOL   bOtherFS; // TRUE:対応する FALSE:対応しない（旧仕様）
#endif
#if defined(USE_SSL) && defined(UPDATE_20171211A) // 使用可能な暗号化通信方式のバージョンを規定するオプション 0x1:SSL2禁止 0x2:SSL3禁止 0x4:TSL1禁止 0x8:TSL1.1禁止 0x10:TSL1.2禁止
int nSSLOpt;
unsigned long nSecuerLayOption;
#endif
#ifdef UPDATE_20190426 // 利用する暗号アルゴリズムの組み合わせオプション 
int nSSLCipher;
char mSecuerLayCipher[1024];
#endif
#ifdef UPDATE_20240420 // 暗号化通信方式のセキュリティレベルの設定
int   nSSLSecureLevel; // デフォルト=0
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
#ifdef UPDATE_20070521 // OSの予約語対策
CHAR   mReservedWords[256];
#endif
#ifdef UPDATE_20091116 // CAPABILITY 命令で応答する認証方法を設定変更可能にした。
int    nAuthType; // 3:CRAM-MD5&LOGIN,2:CRAM-MD5,1:LOGIN
CHAR   mImapAuthType[256];
#endif
#ifdef UPDATE_20110414 // NOOPタイマー（一定時間新着がないNOOPでセッション強制切断する対策（負荷軽減対策）
int nIDELTimeOut;
#endif

#ifdef USE_SSL
#ifdef USE_STARTTLS
BOOL    bUsedSTLS;
#endif
BOOL    bUsedSSL;
char    mCertificate[256];
char    mPrivatekey[256];
#else
BOOL    bUsedSTLS = FALSE;
#endif
char    mHiddenMyIPFile[256];
#ifdef UPDATE_20151122 // 特定接続元IPからの接続拒絶用テーブル追加(rejectimap4ip.dat)隠しオプション
char	mRejectPeerPFile[256];
#endif
#ifdef UPDATE_20160113 // 認証時のロックアウト機能を追加
DWORD	nAuthLockOut; // ロックアウトまでの回数 デフォルト 0:無効
#endif
#ifdef UPDATE_20160114 // 接続拒絶用テーブルの指定に有効時間を設定可能にした。
DWORD	nAuthLockOutTime; // ロックアウト時間 デフォルト 0:無限 分単位
#endif
#ifdef UPDATE_20190216 // IP毎の同時接続ロックアウト機能を追加
DWORD	nIPLockOut; // IPロックアウトまでの回数 デフォルト 0:無効
DWORD	nIPLockOutTime; // ロックアウト時間 デフォルト 0:無限 分単位
#ifdef UPDATE_20190218 // IP毎の同時接続ロックアウト機能を強化（サンプル時間と拒絶時間の設定を追加）
DWORD	nIPLockOutSPTime; // IPロックアウトまでのサンプリング時間 デフォルト 0:無限 秒単位
#endif
#endif
#ifdef ADD_XOAUTH2 // OAUTH2での認証方式を追加
char   mOAuthFile[256];
#endif
#ifdef UPDATE_20240728 // サブネットマスクの範囲指定の高速化
BOOL   bIPcmpmaskengin;
#endif

SOCKET  sListener;
SOCKADDR_IN sinListener;
int     nsinListener;
HANDLE  hCompletionPort;
WSADATA WsaData;
BOOL    bServiceTerminating = FALSE;

#ifdef UPDATE_20151115 // SELECT時にメール一覧をキャッシュするためのポインタ
BOOL bSELECT;   // SELECTによるファイル一覧キャッシュ化
#endif
#ifdef UPDATE_20140530 // // NOOPによるポーリング結果送信有無
BOOL bNOOPPoll;   // NOOPによるポーリング結果送信有無
#endif
#ifdef UPDATE_20140528 // IDLEコマンド実装
BOOL bIDLECmd;    // IDLEコマンド有効有無
#ifdef UPDATE_20250918 // IDLEポーリングベース間隔(ミリ秒）の調整
int  nIDLEMSLoop; // IDLEベースタイマー（一定時間新着チェック）
#endif
int  nIDLELoop; // IDLEタイマー（一定時間新着チェック）
#ifdef UPDATE_20140610 // IDLEコマンド実行中の無通信タイムアウト時間をデフォルトで30分にした。
int  nIDLETMOut; // IDLE実行中の無通信タイマーデフォルト30分
#endif
#endif
#ifdef UPDATE_20230627 // UPDATE_20230624のメールデータを事前に一括して読込み検索するフラグを追加
BOOL bBulkSearch;
#endif
#ifdef UPDATE_20230706 // FETCH時のメールデータを事前に一括して読込み検索するフラグを追加
BOOL bBulkFetch;
#endif
#ifdef UPDATE_20231003 // データ受信時のCPU負荷を下げる対策
DWORD nPeekCoolTime;   //0:無効 デフォルト 50 100 500 1000 5000 10000
#endif
#ifdef UPDATE_20231209 // FETCH命令中に通信切断された場合に大きなサイズのデータ送信の中断を行えるようにした。
BOOL bSendErrEscape;
#endif

#ifdef UPDATE_20090322 // ログイン時のロックファイルのクリティカルセクション化
CRITICAL_SECTION g_csLogin;
#endif
#ifdef UPDATE_20101221 // UID値ファイルのクリティカルセクション化
CRITICAL_SECTION g_csWriteUID;
#endif
#ifdef UPDATE_20151215A // 高負荷かつ同一アカウントへ複数ログイン時にIMAP用INBOXへのメール確認で重複メールが作成される可能性がある不具合
CRITICAL_SECTION g_csMoveMess;
#endif
#ifdef UPDATE_20151226 // 当分封印：IDLE処理の修正 (同一フォルダにアクセス中のセッションに変化が伝わらない不具合)
CRITICAL_SECTION g_csIdleMess;
#endif

#ifdef Y2038_BUG
BOOL    ReSetTimeLimit(__int64 *ltbegin);
#else
BOOL    ReSetTimeLimit(time_t *ltbegin);
#endif
#ifdef ACCEPT_LOG_LEVEL3
void LEVEL_3_ACCEPTLOG(PCLIENT_CONTEXT lpClientContext, char *mess);
#endif
#ifdef UPDATE_20141220 // 操作フォルダをアクセス中のセッションに変化をリアルタイムに同報するための対策
unsigned int nPutLockCnt; // 同一フォルダの操作情報を接続中のセッションに同報する排他用フラグチェックのタイムアウト値　デフォルト 30秒
BOOL  bBroadcastSession; // 同一フォルダの操作情報を接続中のセッションに同報するオプション TRUE:有効 FALSE:無効
#ifdef UPDATE_20141222 // 同報セッション情報の保管先を接続ウント単位に保存する対策
void InitBroadcastSession(CHAR *pFn);
#else
void InitBroadcastSession(void);
#endif
#endif
//#define TRACE 1

void maketime(SYSTEMTIME *lt, char *mt) {
  char   mzone[6];
  int    off ,i;

   *mt = '\x0';
   _tzset();
   off = (_timezone / 60) * -1;
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
   sprintf(mt, "%02d-%3s-%04d %02d:%02d:%02d %s", lt->wDay, mMonth[lt->wMonth-1], lt->wYear, lt->wHour, lt->wMinute, lt->wSecond, mzone);
}

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

BOOL Imap4_Dispatch(PCLIENT_CONTEXT lpClientContext) {
  BOOL sts;
  char *p;
  PImap4Context    pContext = &lpClientContext->Context;
#ifdef UPDATE_20151122A // STARTTLSで暗号化通信に切り替わらない不具合
  BOOL bStartTSL = FALSE;
#endif

  do {
    if (*lpClientContext->Queue) { // キューに命令があるとき
      if (bDebug) printf("[%08x] RECV QUEUE [%08x] <- [%s]\n", lpClientContext, lpClientContext->Socket, lpClientContext->Buffer);
#ifdef ACCEPT_LOG_LEVEL3
      LEVEL_3_ACCEPTLOG(lpClientContext, lpClientContext->Queue);
#endif
      strcpy(lpClientContext->Buffer, lpClientContext->Queue);
	  lpClientContext->Queue[0] = '\x0'; // コピーしたら一旦クリア
	}
    p = strchr(lpClientContext->Buffer,'\n'); // ライン終了検出
    if (p) {
      p++;
      if (*p != '\x0') {  // 一括して命令がきた場合、他はキューに保存
        strcpy(lpClientContext->Queue, p);
        *p = '\x0';
	  }
	}
    pContext->pCmd = pContext->pToken = NULL;
    pContext->pSquence = lpClientContext->Buffer;
    p = strstr(pContext->pSquence, " "); // シーケンス番号
    if (p) { // 命令だけ切りだす
	  *p = '\x0';
	  pContext->pCmd = ++p;
	  p = strstr(pContext->pCmd, " "); // 命令
	  if (p) {
	    *p = '\x0';
	    pContext->pToken = ++p;
	    p = strstr(pContext->pToken, "\r\n"); // // 命令内容
	    if (p) {
	      *p = '\x0';
		} else {
          strtok(pContext->pToken,"\r\n");
		}
	  } else {
        strtok(pContext->pCmd,"\r\n");
	  }
	} else {
      strtok(pContext->pSquence,"\r\n");
	}
    if (pContext->pCmd) {
#ifdef UPDATE_20110414 // NOOPタイマー（一定時間新着がないNOOPでセッション強制切断する対策（負荷軽減対策）
      if (stricmp(pContext->pCmd, IMAP4_NOOP) != 0 ) {
		GetLocalTime(&pContext->nNOOPst); // NOOP以外のコマンドで時間をリセットする
	  }
#endif
  	  if (!(stricmp(pContext->pCmd, IMAP4_LOGIN) == 0 ||
	  	    stricmp(pContext->pCmd, IMAP4_AUTHENTICATE) == 0) )
        imap4log(lpClientContext, "");
	  pContext->bUID = FALSE; // UID付き命令クリア
#ifdef USE_STARTTLS
#ifdef UPDATE_20151126 // リッスンＩＰ毎に証明書を選択できる対策
      if ((bUsedSTLS || pContext->bUsedSTLS) && stricmp(pContext->pCmd, IMAP4_STARTTLS) == 0)
#else
      if (bUsedSTLS && stricmp(pContext->pCmd, IMAP4_STARTTLS) == 0)
#endif
	  {
        sts = StlsDispatch(lpClientContext);
	  } else
#endif
      if (stricmp(pContext->pCmd, IMAP4_CAPABILITY) ==0) {
        sts = CAPABILITYDispatch(lpClientContext);
	  } else if (stricmp(pContext->pCmd, IMAP4_NOOP) ==0 ) {
        sts = NOOPDispatch(lpClientContext);
	  } else if (stricmp(pContext->pCmd, IMAP4_LOGOUT) ==0 ) {
        sts = LOGOUTDispatch(lpClientContext);
	  } else if (stricmp(pContext->pCmd, IMAP4_AUTHENTICATE) ==0 ) {
        sts = AUTHENTICATEDispatch(lpClientContext);
	  } else if (stricmp(pContext->pCmd, IMAP4_LOGIN) ==0 && pContext->pToken) {
        sts = LOGINDispatch(lpClientContext);
	  } else if (stricmp(pContext->pCmd, IMAP4_SELECT) ==0 && pContext->pToken) {
        sts = SELECTDispatch(lpClientContext);
	  } else if (stricmp(pContext->pCmd, IMAP4_EXAMINE) ==0 && pContext->pToken) {
        sts = EXAMINEDispatch(lpClientContext);
	  } else if (stricmp(pContext->pCmd, IMAP4_CREATE) ==0 && pContext->pToken) {
#ifndef UPDATE_20110404 // 選択フォルダ単位で排他する対策
#ifdef UPDATE_20110204 // ファイル操作のファイルのクリティカルセクション化
        while(!LockMailDirectory(pContext->mUSER, pContext->MyAddr)) // Lockファイルがある場合は何もしない。
		{
#ifdef UPDATE_20170114 // UID値書き込み用のスペースに空きが無いときロックする不具合
          if (bServiceTerminating || GetLastError() == ERROR_DISK_FULL)
#else
          if (bServiceTerminating)
#endif
  	        break;
	      _sleep(WAIT_TIME);
		}
#endif
#endif
        sts = CREATEDispatch(lpClientContext);
#ifndef UPDATE_20110404 // 選択フォルダ単位で排他する対策
#ifdef UPDATE_20110204 // ファイル操作のファイルのクリティカルセクション化
        UnLockMailDirectory(pContext->mUSER, pContext->MyAddr);
#endif
#endif
	  } else if (stricmp(pContext->pCmd, IMAP4_DELETE) ==0 && pContext->pToken) {
#ifdef UPDATE_20110204 // ファイル操作のファイルのクリティカルセクション化
        while(!LockMailDirectory(pContext->mUSER, pContext->MyAddr)) // Lockファイルがある場合は何もしない。
		{
#ifdef UPDATE_20170114 // UID値書き込み用のスペースに空きが無いときロックする不具合
          if (bServiceTerminating || GetLastError() == ERROR_DISK_FULL)
#else
          if (bServiceTerminating)
#endif
  	        break;
	      _sleep(WAIT_TIME);
		}
#endif
        sts = DELETEDispatch(lpClientContext);
#ifdef UPDATE_20110204 // ファイル操作のファイルのクリティカルセクション化
        UnLockMailDirectory(pContext->mUSER, pContext->MyAddr);
#endif
	  } else if (stricmp(pContext->pCmd, IMAP4_RENAME) ==0 && pContext->pToken) {
#ifdef UPDATE_20110204 // ファイル操作のファイルのクリティカルセクション化
        while(!LockMailDirectory(pContext->mUSER, pContext->MyAddr)) // Lockファイルがある場合は何もしない。
		{
#ifdef UPDATE_20170114 // UID値書き込み用のスペースに空きが無いときロックする不具合
          if (bServiceTerminating || GetLastError() == ERROR_DISK_FULL)
#else
          if (bServiceTerminating)
#endif
  	        break;
	      _sleep(WAIT_TIME);
		}
#endif
        sts = RENAMEDispatch(lpClientContext);
#ifdef UPDATE_20110204 // ファイル操作のファイルのクリティカルセクション化
        UnLockMailDirectory(pContext->mUSER, pContext->MyAddr);
#endif
	  } else if (stricmp(pContext->pCmd, IMAP4_SUBSCRIBE) ==0 && pContext->pToken) {
        sts = SUBSCRIBEDispatch(lpClientContext);
	  } else if (stricmp(pContext->pCmd, IMAP4_UNSUBSCRIBE) ==0 && pContext->pToken) {
        sts = UNSUBSCRIBEDispatch(lpClientContext);
	  } else if (stricmp(pContext->pCmd, IMAP4_LIST) ==0 && pContext->pToken) {
        sts = LISTDispatch(lpClientContext);
	  } else if (stricmp(pContext->pCmd, IMAP4_LSUB) ==0 && pContext->pToken) {
        sts = LSUBDispatch(lpClientContext);
	  } else if (stricmp(pContext->pCmd, IMAP4_STATUS) ==0 && pContext->pToken) {
        sts = STATUSDispatch(lpClientContext);
	  } else if (stricmp(pContext->pCmd, IMAP4_APPEND) ==0 && pContext->pToken) {
#ifndef UPDATE_20110404 // 選択フォルダ単位で排他する対策
#ifdef UPDATE_20110204 // ファイル操作のファイルのクリティカルセクション化
        while(!LockMailDirectory(pContext->mUSER, pContext->MyAddr)) // Lockファイルがある場合は何もしない。
		{
#ifdef UPDATE_20170114 // UID値書き込み用のスペースに空きが無いときロックする不具合
          if (bServiceTerminating || GetLastError() == ERROR_DISK_FULL)
#else
          if (bServiceTerminating)
#endif
  	        break;
	      _sleep(WAIT_TIME);
		}
#endif
#endif
        sts = APPENDDispatch(lpClientContext);
#ifndef UPDATE_20110404 // 選択フォルダ単位で排他する対策
#ifdef UPDATE_20110204 // ファイル操作のファイルのクリティカルセクション化
        UnLockMailDirectory(pContext->mUSER, pContext->MyAddr);
#endif
#endif
	  } else if (stricmp(pContext->pCmd, IMAP4_NAMESPACE) ==0) {
        sts = NAMESPACEDispatch(lpClientContext);
#ifdef UPDATE_20140528 // IDLEコマンド実装
	  } else if (stricmp(pContext->pCmd, IMAP4_IDLE) ==0 && bIDLECmd) {
        sts = IDLEDispatch(lpClientContext);
#endif
	  } else if (stricmp(pContext->pCmd, IMAP4_CHECK) ==0 ) {
        sts = CHECKDispatch(lpClientContext);
	  } else if (stricmp(pContext->pCmd, IMAP4_CLOSE) ==0 ) {
        sts = CLOSEDispatch(lpClientContext);
	  } else if (stricmp(pContext->pCmd, IMAP4_EXPUNGE) ==0 ) {
#ifndef UPDATE_20110404 // 選択フォルダ単位で排他する対策
#ifdef UPDATE_20110204 // ファイル操作のファイルのクリティカルセクション化
        while(!LockMailDirectory(pContext->mUSER, pContext->MyAddr)) // Lockファイルがある場合は何もしない。
		{
#ifdef UPDATE_20170114 // UID値書き込み用のスペースに空きが無いときロックする不具合
          if (bServiceTerminating || GetLastError() == ERROR_DISK_FULL)
#else
          if (bServiceTerminating)
#endif
  	        break;
	      _sleep(WAIT_TIME);
		}
#endif
#endif
        sts = EXPUNGEDispatch(lpClientContext);
#ifndef UPDATE_20110404 // 選択フォルダ単位で排他する対策
#ifdef UPDATE_20110204 // ファイル操作のファイルのクリティカルセクション化
        UnLockMailDirectory(pContext->mUSER, pContext->MyAddr);
#endif
#endif
	  } else if (stricmp(pContext->pCmd, IMAP4_SEARCH) ==0 && pContext->pToken) {
        sts = SEARCHDispatch(lpClientContext);
	  } else if (stricmp(pContext->pCmd, IMAP4_FETCH) ==0 && pContext->pToken) {
#ifndef UPDATE_20110404 // 選択フォルダ単位で排他する対策
#ifdef UPDATE_20110204 // ファイル操作のファイルのクリティカルセクション化
        while(!LockMailDirectory(pContext->mUSER, pContext->MyAddr)) // Lockファイルがある場合は何もしない。
		{
#ifdef UPDATE_20170114 // UID値書き込み用のスペースに空きが無いときロックする不具合
          if (bServiceTerminating || GetLastError() == ERROR_DISK_FULL)
#else
          if (bServiceTerminating)
#endif
  	        break;
	      _sleep(WAIT_TIME);
		}
#endif
#endif
        sts = FETCHDispatch(lpClientContext);
#ifndef UPDATE_20110404 // 選択フォルダ単位で排他する対策
#ifdef UPDATE_20110204 // ファイル操作のファイルのクリティカルセクション化
        UnLockMailDirectory(pContext->mUSER, pContext->MyAddr);
#endif
#endif
	  } else if (stricmp(pContext->pCmd, IMAP4_STORE) ==0 && pContext->pToken) {
#ifndef UPDATE_20110404 // 選択フォルダ単位で排他する対策
#ifdef UPDATE_20110204 // ファイル操作のファイルのクリティカルセクション化
        while(!LockMailDirectory(pContext->mUSER, pContext->MyAddr)) // Lockファイルがある場合は何もしない。
		{
#ifdef UPDATE_20170114 // UID値書き込み用のスペースに空きが無いときロックする不具合
          if (bServiceTerminating || GetLastError() == ERROR_DISK_FULL)
#else
          if (bServiceTerminating)
#endif
  	        break;
	      _sleep(WAIT_TIME);
		}
#endif
#endif
        sts = STOREDispatch(lpClientContext);
#ifndef UPDATE_20110404 // 選択フォルダ単位で排他する対策
#ifdef UPDATE_20110204 // ファイル操作のファイルのクリティカルセクション化
        UnLockMailDirectory(pContext->mUSER, pContext->MyAddr);
#endif
#endif
	  } else if (stricmp(pContext->pCmd, IMAP4_COPY) ==0 && pContext->pToken) {
#ifndef UPDATE_20110404 // 選択フォルダ単位で排他する対策
#ifdef UPDATE_20110204 // ファイル操作のファイルのクリティカルセクション化
        while(!LockMailDirectory(pContext->mUSER, pContext->MyAddr)) // Lockファイルがある場合は何もしない。
		{
#ifdef UPDATE_20170114 // UID値書き込み用のスペースに空きが無いときロックする不具合
          if (bServiceTerminating || GetLastError() == ERROR_DISK_FULL)
#else
          if (bServiceTerminating)
#endif
  	        break;
	      _sleep(WAIT_TIME);
		}
#endif
#endif
        sts = COPYDispatch(lpClientContext);
#ifndef UPDATE_20110404 // 選択フォルダ単位で排他する対策
#ifdef UPDATE_20110204 // ファイル操作のファイルのクリティカルセクション化
        UnLockMailDirectory(pContext->mUSER, pContext->MyAddr);
#endif
#endif
	  } else if (stricmp(pContext->pCmd, IMAP4_UID) ==0 && pContext->pToken) {
        sts = UIDDispatch(lpClientContext);
#ifdef ADD_ID_XDELTAPUSH
	  } else if (stricmp(pContext->pCmd, IMAP4_ID) ==0 && pContext->pToken) {
        sts = IDDispatch(lpClientContext);
#endif
#ifdef ADD_METADATA
	  } else if (stricmp(pContext->pCmd, IMAP4_SETMETADATA) ==0 && pContext->pToken) {
        sts = SETMETADATADispatch(lpClientContext);
	  } else if (stricmp(pContext->pCmd, IMAP4_GETMETADATA) ==0 && pContext->pToken) {
        sts = GETMETADATADispatch(lpClientContext);
#endif
	  } else { // 間違った命令
        sts = BADDispatch(lpClientContext);
	  }
	} else { // 命令がNULL
#ifdef UPDATE_20140528 // IDLEコマンド実装
	  if (stricmp(pContext->pSquence, IMAP4_DONE) ==0 && pContext->bIDLE)
	  {
        sts = DONEDispatch(lpClientContext);
	  } else 
#endif
	  {
        sts = BADDispatch(lpClientContext);
	  }
	}
#ifdef UPDATE_20151122A // STARTTLSで暗号化通信に切り替わらない不具合
	if (strstr(pContext->mMessages, IMAP4_OK_STARTTLS))
	{
	  bStartTSL = TRUE;
	}
#endif
#ifdef UPDATE_20151226 // IDLE処理の修正
    if (!bIDLEBroadcast) // IDLE時の参加セッションへメッセージ出力を、自分のセッションより先に行うか否かの選択 デフォルト 0:後に送信 1:先に送信
#endif
	{
      put_reply(lpClientContext, TRUE, TRUE);
	}
#ifdef UPDATE_20141220 // 操作フォルダをアクセス中のセッションに変化をリアルタイムに同報するための対策
    if (bBroadcastSession) // 同一フォルダの操作情報を接続中のセッションに同報するオプション TRUE:有効 FALSE:無効
	{
	  if (pContext->pCmd && pContext->mMessages)
	  {
#ifdef UPDATE_20151226 // IDLE処理の修正
        if ((stricmp(pContext->pCmd, IMAP4_SELECT) == 0 || stricmp(pContext->pCmd, IMAP4_EXAMINE) == 0) &&
		    (strstr(pContext->mMessages, "OK [READ-WRITE]") || strstr(pContext->mMessages, "OK [READ-ONLY]")) )
#else
        if ((stricmp(pContext->pCmd, IMAP4_SELECT) == 0 || stricmp(pContext->pCmd, IMAP4_EXAMINE) == 0) &&
			(strstr(pContext->mMessages, "[READ-WRITE]") || strstr(pContext->mMessages, "[READ-ONLY]")) )
#endif
	{
          JoinBroadcastSession(lpClientContext);
		}
	  }
	}
#endif
#ifdef UPDATE_20151226 // IDLE処理の修正
    if (bIDLEBroadcast) // IDLE時の参加セッションへメッセージ出力を、自分のセッションより先に行うか否かの選択 デフォルト 0:後に送信 1:先に送信
	{
      put_reply(lpClientContext, TRUE, TRUE);
	}
#endif

#ifdef USE_STARTTLS
    // STARTTLSでSSLモードに切替
	if (pContext->pCmd)
	{
      if (!stricmp(pContext->pCmd, IMAP4_STARTTLS) &&
#ifdef UPDATE_20151122A // STARTTLSで暗号化通信に切り替わらない不具合
	      bStartTSL &&
#else
		  strstr(pContext->mMessages, IMAP4_OK_STARTTLS) &&
#endif
#ifdef UPDATE_20151126 // リッスンＩＰ毎に証明書を選択できる対策
		  !pContext->bUsedSSL && (bUsedSTLS || pContext->bUsedSTLS))
#else
		  !pContext->bUsedSSL && mPrivatekey[0] && mCertificate[0])
#endif
	  {
#ifdef UPDATE_OPESSL1_1_1 // openssl1.1.1系に置換える場合
        lpClientContext->ctx = SSL_CTX_new(TLS_server_method()); // SSL3以降を使用
#else
        lpClientContext->ctx = SSL_CTX_new(SSLv23_server_method()); // SSL2 or SSL3を使用
#endif
#ifdef ACCEPT_LOG_LEVEL3
        sprintf(mLog, "SSL_CTX_new() = %d", lpClientContext->ctx);
        LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
#endif
        if (lpClientContext->ctx)
		{
#ifdef UPDATE_20240420 // 暗号化通信方式のセキュリティレベルの設定
              //if (bDebug) printf("SSL security level =%d\n", SSL_CTX_get_security_level(lpClientContext->ctx));
          SSL_CTX_set_security_level(lpClientContext->ctx, nSSLSecureLevel);
#ifdef ACCEPT_LOG_LEVEL3
          sprintf(mLog, "SSL_CTX_get_security_level() = %d", SSL_CTX_get_security_level(lpClientContext->ctx));
          LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
#endif
#endif
#if defined(USE_SSL) && defined(UPDATE_20171211A) // 使用可能な暗号化通信方式のバージョンを規定するオプション 0x1:SSL2禁止 0x2:SSL3禁止 0x4:TSL1禁止 0x8:TSL1.1禁止 0x10:TSL1.2禁止
          SSL_CTX_set_options(lpClientContext->ctx, nSecuerLayOption);
#endif
#ifdef UPDATE_20190426 // 利用する暗号アルゴリズムの組み合わせオプション 
          if (mSecuerLayCipher[0])
		    SSL_CTX_set_cipher_list(lpClientContext->ctx, mSecuerLayCipher);
		  if (bDebug)
		    Print_SSL_CIPHER_names(lpClientContext->ctx);
#endif
#ifdef UPDATE_20170619 // 中間証明書の繋がりが反映されていなかった
	      if (pContext->bUsedSTLS)
		  {
            SSL_CTX_use_certificate_chain_file(lpClientContext->ctx, pContext->mCertificate); // // SSL用中間証明書
		  } else {
	        SSL_CTX_use_certificate_chain_file(lpClientContext->ctx, mCertificate); // // SSL用中間証明書
		  }
#endif
          lpClientContext->ssl = SSL_new(lpClientContext->ctx); // SSL構造作成
          if (lpClientContext->ssl)
		  {
            SSL_set_fd(lpClientContext->ssl, lpClientContext->Socket); 
#ifdef UPDATE_20151126 // リッスンＩＰ毎に証明書を選択できる対策
            if (pContext->bUsedSTLS)
			{
              SSL_use_RSAPrivateKey_file(lpClientContext->ssl, pContext->mPrivatekey, SSL_FILETYPE_PEM); // 個人鍵を指定
              SSL_use_certificate_file(lpClientContext->ssl, pContext->mCertificate, SSL_FILETYPE_PEM);   // 証明書を指定
#ifndef UPDATE_20170619 // 中間証明書の繋がりが反映されていなかった
	          SSL_CTX_use_certificate_chain_file(lpClientContext->ctx, pContext->mCertificate); // // SSL用中間証明書
#endif
			} else
#endif
			{
              SSL_use_RSAPrivateKey_file(lpClientContext->ssl, mPrivatekey, SSL_FILETYPE_PEM); // 個人鍵を指定
              SSL_use_certificate_file(lpClientContext->ssl, mCertificate, SSL_FILETYPE_PEM);   // 証明書を指定
#ifndef UPDATE_20170619 // 中間証明書の繋がりが反映されていなかった
	          SSL_CTX_use_certificate_chain_file(lpClientContext->ctx, mCertificate); // // SSL用中間証明書
#endif
			}
            if (SSL_accept(lpClientContext->ssl) == 1)
			{
			  pContext->bUsedSSL = TRUE;
			} else {
              //err_ssl = ERR_get_error();
              SSL_CTX_free(lpClientContext->ctx);
              SSL_shutdown(lpClientContext->ssl);
              SSL_free(lpClientContext->ssl);
			}
		  } else {
	        SSL_CTX_free(lpClientContext->ctx);
		  }
		} else {
#ifdef ACCEPT_LOG_LEVEL3
          LEVEL_3_ACCEPTLOG(lpClientContext, "SSL_CTX_new() failed");
#endif
		} 
	  }
	}
#endif  //USE_STARTTLS end.
  } while(*lpClientContext->Queue);

  return sts;
}

BOOL IMAP4SMain(void) {
   hCompletionPort = InitializeThreads(); // Initialize the IMAP4SRV worker threads.
   if ( hCompletionPort == NULL )
    if (bDebug) printf( "it failed.\n" );
   return(AcceptClients( hCompletionPort )); // Start accepting and processing clients.
}

void Copyright(void) {
   BOOL   bPassport = FALSE;
   char   mtime[256];
   char   mmode[64]; //[10];
#ifdef Y2038_BUG
   SYSTEMTIME ltime;
#else
   time_t ltime;
#endif
   gettime(&ltime, mtime);
   GetTimeLimit(&ltbegin, mmode, &bPassport);
   if (bDebug) printf(IMAP4_DEBUG_MESS, VERSION, mmode, mtime);
   if (bDebug) printf("\n");
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
    HANDLE                  hPipe = INVALID_HANDLE_VALUE;
    HANDLE                  hEvents[2] = {NULL,NULL};
    OVERLAPPED              os;
    PSECURITY_DESCRIPTOR    pSD = NULL;
    SECURITY_ATTRIBUTES     sa;
    LPTSTR                  lpszPipeName = TEXT("\\\\.\\pipe\\" TRADEMARK "IMAP4S");
    DWORD                   dwWait, dwc;
   // UINT                    ndx;
    int status, nl;             /* Status Code */
    WSADATA WSAData;
    char *tmp, *p, *pldom;
#ifdef Y2038_BUG
  ULARGE_INTEGER u1;
#endif
    char  mLicencekey[65];
#ifdef CLUSTERING
	DWORD    nComName;
#endif
#ifdef UPDATE_20091116 // CAPABILITY 命令で応答する認証方法を設定変更可能にした。
    CHAR mAuth[256];
#endif
#ifdef UPDATE_20110301B // サービス停止で残ったユーザ毎のロックファイルをサービス再起動後に一旦削除する対策
	FILE *fp;
#endif

//BOOL btst=FALSE;
//char fullname[256];
// スレッドあたりのメモリ使用数
//printf("%lu\n", sizeof(IMAP4STK));
//printf("%lu\n", sizeof(Imap4Envelope));
//printf("%lu\n", sizeof(PartStruct));
printf("%lu\n", sizeof(Imap4Context));
printf("%lu\n", sizeof(CLIENT_CONTEXT));
	//////////////////////////////////////////////////////////////
#ifdef UPDATE_20101221 // UID値ファイルのクリティカルセクション化
    InitializeCriticalSection(&g_csWriteUID);
#endif
#ifdef UPDATE_20090322 // ログイン時のロックファイルのクリティカルセクション化
    //////////////////////////////////////////////
    // ログイン時のロックファイルフラグ初期化
    InitializeCriticalSection(&g_csLogin);
#endif
#ifdef UPDATE_20151215A // 高負荷かつ同一アカウントへ複数ログイン時にIMAP用INBOXへのメール確認で重複メールが作成される可能性がある不具合
    InitializeCriticalSection(&g_csMoveMess);
#endif
#ifdef UPDATE_20151226 // 当分封印：IDLE処理の修正 (同一フォルダにアクセス中のセッションに変化が伝わらない不具合)
    InitializeCriticalSection(&g_csIdleMess);
#endif

#ifdef UPDATE_20070405 // イベントログデータベースを追加。
    InitEventLog();
#endif
    // initalaize //////////////////
#ifdef V4
    _setmaxstdio(2048);
#endif
    nAcceptCount = 0;
    nLogInID = 0;
    bSequenceOK = TRUE;
	bTMQWait = FALSE;
	mTMQueue.pCurrent = NULL;
	mTMQueue.pNext = NULL;

#ifdef CLUSTERING
	///// コンピュータ名を取得
	nComName = sizeof(mComName);
	GetComputerName(mComName, &nComName);
	///// UNC接続可能になるまでのリトライ時間
	nReadyDriveTime = GetProfileIntEx(SOFT_REG, "ReadyDriveTime", 5);
	///// スプール先はレジストリから取得
    GetProfileStringEx(SOFT_REG,"MailSpoolDir", MAIL_SPOOL, mMailSpoolDir, sizeof(mMailSpoolDir));
#ifdef LDAP_ON         // LDAPへのログイン処理でSMTP認証の確認を行うオプション // //CRAM-MD5は複合できないので、これを利用する場合は、PLAINかLOGINのみとします。
    bLDAPOn = GetProfileIntEx(SOFT_REG, "LDAP", (int) FALSE);    // LDAP対応
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
#ifdef UPDATE_20190510 // NTFS以外のファイルシステムからのファイル一覧取得可能にする対策（一覧ソート）を追加
    bOtherFS = GetProfileIntEx(SYSTEM_REG , "AnyOtherFS", FALSE); // TRUE:対応する FALSE:対応しない（旧仕様）
#endif
#if defined(USE_SSL) && defined(UPDATE_20171211A) // 使用可能な暗号化通信方式のバージョンを規定するオプション 0x1:SSL2禁止 0x2:SSL3禁止 0x4:TSL1禁止 0x8:TSL1.1禁止 0x10:TSL1.2禁止
    nSSLOpt = GetProfileIntEx(SYSTEM_REG , "SecureLayOption", (int)3); // 0x1:SSL2禁止 0x2:SSL3禁止 0x4:TSL1禁止 0x8:TSL1.1禁止 0x10:TSL1.2禁止
    nSecuerLayOption = 0;
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
#ifdef UPDATE_20110222 //UPDATE_20110202でパフォーマンス改善の対策。
#ifdef UPDATE_20110325 // 回復機能をデフォルトに変更
	bUIDRecover = GetProfileIntEx(SYSTEM_REG , "UIDRecover", (int)1);
#else
	bUIDRecover = GetProfileIntEx(SYSTEM_REG , "UIDRecover", (int)0);
#endif
#endif
	///// ライセンスキー情報
    GetProfileStringEx(SYSTEM_REG_POP3, LIMITKEY, "", mLicencekey, sizeof(mLicencekey));
    if (!mLicencekey[0]) // なければIMAP4のキーにて確認
      GetProfileStringEx(SYSTEM_REG, LIMITKEY, "", mLicencekey, sizeof(mLicencekey));
    //////////////////////
#ifdef Y2038_BUG
	u1.LowPart = GetProfileIntEx(SYSTEM_REG , BEGINLOW, (int)0);
	u1.HighPart = GetProfileIntEx(SYSTEM_REG , BEGINHIGH, (int)0);
	ltbegin = u1.QuadPart;
#else
	ltbegin = GetProfileIntEx(SYSTEM_REG, "Begin", (int)0);
#endif
    ReSetTimeLimit(&ltbegin);
#ifdef UPDATE_20141220 // 操作フォルダをアクセス中のセッションに変化をリアルタイムに同報するための対策
	nPutLockCnt = GetProfileIntEx(SYSTEM_REG , "PutLockCount", (int)3); // 同一フォルダの操作情報を接続中のセッションに同報する排他用フラグチェックのタイムアウト値　デフォルト 3秒
    bBroadcastSession = GetProfileIntEx(SYSTEM_REG , "Broadcast", (int)TRUE); // 同一フォルダの操作情報を接続中のセッションに同報するオプション TRUE:有効 FALSE:無効
    if (bBroadcastSession)
	{
#ifdef UPDATE_20141222 // 同報セッション情報の保管先を接続ウント単位に保存する対策
      InitBroadcastSession(NULL);
#else
      InitBroadcastSession();
#endif
	}
#endif
#ifdef UPDATE_20151115 // SELECT時にメール一覧をキャッシュするためのポインタ
    bSELECT = GetProfileIntEx(SYSTEM_REG , "SelectBoxChace", (int)FALSE); // SELECTによるファイル一覧キャッシュ有無 TRUE:有効 FALSE:無効
#endif
#ifdef UPDATE_20140528 // IDLEコマンド実装
#ifdef UPDATE_20141025A // IDLEコマンドデフォルトで無効にした
    bIDLECmd = GetProfileIntEx(SYSTEM_REG , "IDLECmd", (int)FALSE); // IDLEコマンド有効有無 TRUE:有効 FALSE:無効
#else
    bIDLECmd = GetProfileIntEx(SYSTEM_REG , "IDLECmd", (int)TRUE); // IDLEコマンド有効有無 TRUE:有効 FALSE:無効
#endif
#ifdef UPDATE_20250918 // IDLEポーリングベース間隔(ミリ秒）の調整
    nIDLEMSLoop = GetProfileIntEx(SYSTEM_REG , "IDLEMSLoop", (int)1000); // IDLEベースタイマー（ミリ秒 一定時間新着チェック）
	if (nIDLEMSLoop == 0)
	  nIDLEMSLoop = 1;
#endif
    nIDLELoop = GetProfileIntEx(SYSTEM_REG , "IDLELoop", (int)5); // IDLEタイマー（一定時間新着チェック）
#ifdef UPDATE_20140610 // IDLEコマンド実行中の無通信タイムアウト時間をデフォルトで30分にした。
    nIDLETMOut = GetProfileIntEx(SYSTEM_REG , "IDLETMOut", (int)1800); // IDLE実行中の無通信タイマーデフォルト30分
#endif
#endif
#ifdef UPDATE_20140530 // NOOPによるポーリング結果送信有無
#ifdef UPDATE_20140606 // NOOPポーリング結果でフォルダ内に変化があった場合のみ結果応答をしデフォルトで有効にする
    bNOOPPoll= GetProfileIntEx(SYSTEM_REG , "NOOPPoll", (int)TRUE); // NOOPによるポーリング結果送信有無 TRUE:有効 FALSE:無効   
#else
    bNOOPPoll= GetProfileIntEx(SYSTEM_REG , "NOOPPoll", (int)FALSE); // NOOPによるポーリング結果送信有無 TRUE:有効 FALSE:無効   
#endif
#endif
#ifdef UPDATE_20110414 // NOOPタイマー（一定時間新着がないNOOPでセッション強制切断する対策（負荷軽減対策）
	nIDELTimeOut = GetProfileIntEx(SYSTEM_REG, "IDLETimeout", (int)1800); // 60秒×30 = 30分
#endif
	nPriority = GetProfileIntEx(SYSTEM_REG, "Priority", (int)0); // -2:LOW-, -1:LOW, 0:NORMAL, 1:HIGH, 2:HIGH+
#ifdef UPDATE_20140610A // 実行中の受信側無通信タイムアウト時間をデフォルトで30分に変更した。
	nTMOut = GetProfileIntEx(SYSTEM_REG, "Timeout", (int)1800); // 60秒×30 = 30分
#else
	nTMOut = GetProfileIntEx(SYSTEM_REG, "Timeout", (int)300); // 60秒×5 = 5分
#endif
	nTransTMOut = GetProfileIntEx(SYSTEM_REG, "TransmitTimeout", (int)0); // 無制限 3600); // 60秒×60 = 60分
#ifdef UPDATE_20140522 // IMAPでフォルダを選択した時、排他処理フラグが同期されずに以降の処理をロックさせてしまう場合がある
#ifdef UPDATE_20240827A // IMAPフォルダの排他フラグの有効時間をデフォルト30秒に変更した
	nLockTMOut = GetProfileIntEx(SYSTEM_REG, "LockTimeout", (int)30); // 30秒
#else
	nLockTMOut = GetProfileIntEx(SYSTEM_REG, "LockTimeout", (int)300); // 60秒×5 = 5分
#endif
#endif

	//nTMOut = 60;
	//bExApop = GetProfileIntEx(SYSTEM_PARAM_REG, "ExclusiveAPOP", (int)FALSE);
    ///// 共有フォルダのRoot名
    GetProfileStringEx(SYSTEM_REG,"SharedRoot","", mSharedRoot, sizeof(mSharedRoot));
	bSharedRW = GetProfileIntEx(SYSTEM_REG, "SharedMode", (int)FALSE); // FALSE:Read Only, TRUE:Read Write
    ///// INBOXの別名設定ファイル
    GetProfileStringEx(SYSTEM_REG,"InboxAlias","&U9dP4TDIMOwwpA-", mInboxAlias, sizeof(mInboxAlias));
	///// 受付け制限数
    nAcceptLimit = GetProfileIntEx(SYSTEM_REG, "AcceptLimit", (int)0);
    ///// パスワード設定ファイル
    GetProfileStringEx(SYSTEM_REG,"PasswordFile","apop.dat", mPasswordFile, sizeof(mPasswordFile));
#ifdef UPDATE_20210224 // ADアカウントまたは、WINDOWSアカウント運用時のAUTHENTICATE LOGIN/PLAINでのパスワードファイルの参照を無視する
    bSimpleAuth = GetProfileIntEx(SYSTEM_REG, "SimpleAuth", (int)0);
#endif
#ifdef V4
	///// グリーティングメッセージの扱い
	bHide = GetProfileIntEx(SYSTEM_REG , "HidesGreeting", (INT)FALSE);   // グリーティングメッセージを隠す。 TRUE:隠す FALSE:隠さない
    ///// 有効時間設定ファイル
    GetProfileStringEx(SYSTEM_REG,"UseTimeFile","usetime.dat", mUseTimeFile, sizeof(mUseTimeFile));
#endif
    ///// IMAP4受信詳細ログ
	bAcceptLog = GetProfileIntEx(SYSTEM_REG, "AcceptlogEnabled", (int)FALSE);
    ///// メールデータ拡張子を.MSG以外でも指定。
	GetProfileStringEx(SYSTEM_REG, "MailExtension", "MSG", mMailExtension, sizeof(mMailExtension)); // メールデータの拡張子、デフォルト".MSG"
    ///// リッスンIPアドレス制限用
	bListenMode = GetProfileIntEx(SYSTEM_REG, "ListenMode", (int)0); // 0:全てのIP許可, 1:指定IPのみ
	memset(mListenIP, 0, sizeof(mListenIP));
    dwc = GetProfileStringEx(SYSTEM_REG,"ListenIP","", mListenIP, sizeof(mListenIP));
    memset(&mListenIP[dwc], 0, (sizeof(mListenIP)-dwc));
    memcpy(mListenIP2, mListenIP, sizeof(mListenIP));
	pldom = mListenIP2;
	while (strlen(pldom)){
      pldom += strlen(pldom);
 	  pldom++;
	  if (strlen(pldom))
	  	*(pldom-1) = ' ';
	};
    ////////////////////////////////
	bCountLock = FALSE;  // Counter Release
	nport = DEFAULT_PORT;
	bLog  = FALSE;
#ifndef E_POST
    nport = GetProfileIntEx("SYSTEM\\CurrentControlSet\\Services\\IMAP4S\\Parameters", "PortNo", (int)DEFAULT_PORT);
	bLog  = GetProfileIntEx("SYSTEM\\CurrentControlSet\\Services\\IMAP4S\\Parameters", "Imap4LogEnabled", (int)FALSE);
#endif
	if (nport == DEFAULT_PORT)
      nport = GetProfileIntEx(SYSTEM_PARAM_REG, "PortNo", (int)DEFAULT_PORT);
	if (!bLog)
	  bLog  = GetProfileIntEx(SYSTEM_PARAM_REG, "Imap4LogEnabled", (int)FALSE);

#ifdef UPDATE_20170223 // フォルダ削除でのメール一括削除オプション追加
    bAutoSubDirAndMsg = GetProfileIntEx(SOFT_REG, "AutoSubDirAndMsg", (INT)TRUE);    // TRUE=削除する ,FALSE=削除しない
#endif
#ifdef UPDATE_20161013 // ログイン成功時にルートフォルダが存在しない場合、自動作成を試みるようにした。
	bAutoMakeDir = GetProfileIntEx(SOFT_REG, "AutoCreateFolder", (INT)TRUE);    // TRUE=作成する,FALSE=作成しない
#endif
#ifdef V3
	bUserMan = GetProfileIntEx(SOFT_REG, "UserManager", (INT)TRUE);    // TRUE=NT使用,FALSE=SPA使用
//	bInboxEnc = GetProfileIntEx(SOFT_REG, "InboxData", (INT)FALSE);    // TRUE=記号化する,FALSE=記号化しない
#else
//    bInboxEnc = FALSE; // V1,V2は無効
#endif
	nSendBuf = GetProfileIntEx(SYSTEM_REG, "SendDataBufferSize", (int)0); // socket 送信バッファサイズ
	nSendTMO = GetProfileIntEx(SYSTEM_REG, "SendDataTimeout", (INT)300);     // send()送信タイムアウト時間:default 60秒
#ifdef UPDATE_20090130 // レシピエントファイル(RCP)があれば一緒にコピーさせる
	bIgnoreRCP = GetProfileIntEx(SYSTEM_REG, "IgnoreRCP", (int)TRUE); // TRUE: RCPファイル無視, FALSE:RCPファイル無視しない
#endif
#ifdef UPDATE_20091110 // POP3メールボックスフォルダ位置を変更するオプション
    bPOP3Share = GetProfileIntEx(SOFT_REG, "POP3Share", (int)0);  // TRUE:有効 FALSE:無効
#endif
	bOFFPOP3 = GetProfileIntEx(SOFT_REG, "POP3IsNotUsed", (INT)FALSE);  //FALSE:残す TRUE:残さない
	nAddressFamily = GetProfileIntEx(SOFT_REG, "AddressFamily", (INT)0);  //0:AF_INET only 1:AF_INET6 only 2:AF_INET&AF_INET6 
    GetProfileStringEx(SOFT_REG, "Membership", "", mAuthDomain, sizeof(mAuthDomain)); // 認証サーバー選択　デフォルト NULL=ローカルマシン
#ifdef UPDATE_20180820A // SASLにPLAIN方式を追加
    GetProfileStringEx(SYSTEM_REG, "ImapAuthType", "CRAM-MD5,LOGIN,PLAIN", mImapAuthType, sizeof(mImapAuthType)); // 認証可能方式の提示　AUTH=CRAM-MD5 AUTH=LOGIN AUTH=PLAIN
    strcpy(mAuth, mImapAuthType);
    ///////////////////////////////
#ifdef ADD_XOAUTH2 // OAUTH2での認証方式を追加
	//F:XOAUTH2&CRAM-MD5&LOGIN&PLAIN
	//E:XOAUTH2&CRAM-MD5&LOGIN
	//D:XOAUTH2&CRAM-MD5&PLAIN
	//C:XOAUTH2&CRAM-MD5
	//B:XOAUTH2&LOGIN&PLAIN
	//A:XOAUTH2&LOGIN
	//9:XOAUTH2&PLAIN
	//8:XOAUTH2
#endif
	//7:CRAM-MD5&LOGIN&PLAIN
	//6:CRAM-MD5&LOGIN
	//5:CRAM-MD5&PLAIN
	//4:CRAM-MD5
	//3:LOGIN&PLAIN
	//2:LOGIN
	//1:PLAIN
    ///////////////////////////////
	_strupr(mAuth);
	nAuthType = 0;
#ifdef ADD_XOAUTH2 // OAUTH2での認証方式を追加
	if (strstr(mAuth, "XOAUTH2"))
      nAuthType |= 8;
#endif
	if (strstr(mAuth, "CRAM-MD5"))
      nAuthType |= 4;
	if (strstr(mAuth, "LOGIN"))
      nAuthType |= 2;
	if (strstr(mAuth, "PLAIN"))
      nAuthType |= 1;
#else
#ifdef UPDATE_20091116 // CAPABILITY 命令で応答する認証方法を設定変更可能にした。
    GetProfileStringEx(SYSTEM_REG, "ImapAuthType", "CRAM-MD5,LOGIN", mImapAuthType, sizeof(mImapAuthType)); // 認証可能方式の提示　AUTH=CRAM-MD5 AUTH=LOGIN
    strcpy(mAuth, mImapAuthType);
    nAuthType = 1; // 3:CRAM-MD5&LOGIN,2:CRAM-MD5,1:LOGIN
	if (!stricmp(mAuth, "cram-md5")) {
      nAuthType = 2; // 3:CRAM-MD5&LOGIN,2:CRAM-MD5,1:LOGIN
    } else if (strpbrk(mAuth, " ,\t")) {
      nAuthType = 3; // 3:CRAM-MD5&LOGIN,2:CRAM-MD5,1:LOGIN
	}
#endif
#endif
#ifdef LDAP_ON         // LDAPへのログイン処理でSMTP認証の確認を行うオプション // //CRAM-MD5は複合できないので、これを利用する場合は、PLAINかLOGINのみとします。
#ifdef LDAP_OPTION_LICENSE // LDAP Optionのライセンスチェックを行なう
	bLDAPLicense = FALSE;
#endif
    bLDAP = bLDAPOn && GetProfileIntEx(SOFT_REG, "LDAP", (int) FALSE);    // LDAP対応
	mLDAPAdminID[0] = '\x0';
    nLDAPPort = GetProfileIntEx(SOFT_REG, "LDAPPort", (int) 389);    // LDAP対応
    GetProfileStringEx(SOFT_REG, "LDAPAdminID", "", mLDAPAdminID, sizeof(mLDAPAdminID)); // LDAP管理者ID
    GetProfileStringEx(SOFT_REG, "LDAPAdminPW", "", mLDAPAdminPW, sizeof(mLDAPAdminPW)); // LDAP管理者パスワード
    GetProfileStringEx(SOFT_REG, "LDAPSchemaRDN", "cn", mLDAPSchemaRDN, sizeof(mLDAPSchemaRDN)); // LDAP RDN
    GetProfileStringEx(SOFT_REG, "LDAPSchemaDC", "", mLDAPSchemaDC, sizeof(mLDAPSchemaDC)); // LDAP DC（ドメインコンポーネント）ディレクトリのスキーマ名称
#ifdef UPDATE_20091013 // mLDAPSchemaID 
    GetProfileStringEx(SOFT_REG, "LDAPSchemaID", "", mLDAPSchemaID, sizeof(mLDAPSchemaID)); // LDAP ユーザーID名称
#endif
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
#ifdef UPDATE_20150608 // 検索時の日付をUTC日付かローカル日付かに変更できるオプションを追加した
	bSearchTime = GetProfileIntEx(SOFT_REG, "SearchLocalTime", (int)TRUE); // TRUE:ローカル日付 FALSE:UTC日付
#endif
#ifdef UPDATE_20150609 // フォルダへのコピー時に受信日付がコピー日付になってしまった不具合 0:更新日時 1:アクセス日時 2:作成日時 
    nSearchType = GetProfileIntEx(SOFT_REG, "SearchAccessType", (int)0); //  0:更新日時 1:アクセス日時 2:作成日時 
#endif
    GetProfileStringEx(SOFT_REG, "HostName", "", mHostName, sizeof(mHostName));
    //nLocalDomain = GetProfileBinaryEx("SOFTWARE\\EMWAC\\IMS","DomainNamesAreLocal", "", mLocalDomain, sizeof(mLocalDomain));
    nLocalDomain = GetProfileBinaryEx(SOFT_REG,"DomainNamesAreLocal", "", mLocalDomain, sizeof(mLocalDomain));
	//nMailInCopy = GetProfileBinaryEx("SOFTWARE\\EMWAC\\IMS","MailInCopy", "", mMailInCopy, sizeof(mMailInCopy));
	nMailInCopy = GetProfileBinaryEx(SOFT_REG,"MailInCopy", "", mMailInCopy, sizeof(mMailInCopy));
	//nMailInMaxSize = GetProfileIntEx("SOFTWARE\\EMWAC\\IMS", "MailInMaxSize", (INT)0);
	nMailInMaxSize = GetProfileIntEx(SOFT_REG, "MailInMaxSize", (INT)0);
	//nMailInBoxSize = GetProfileIntEx("SOFTWARE\\EMWAC\\IMS", "MailInBoxSize", (INT)0);
	nMailInBoxSize = GetProfileIntEx(SOFT_REG, "MailInBoxSize", (INT)0);
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

//CheckUser("kawa@kawa.co.jp", &btst, fullname);
//exit(0);
	//nLastMsgId = GetProfileIntEx("SOFTWARE\\EMWAC\\IMS", "LastMsgId", (int)0);
	nLastMsgId = GetProfileIntEx(SOFT_REG, "LastMsgId", (int)0);
	//GetProfileStringEx("SOFTWARE\\EMWAC\\IMS","MailGroup","IMSUsers", mMailGroup, sizeof(mMailGroup));
	GetProfileStringEx(SOFT_REG,"MailGroup","IMSUsers", mMailGroup, sizeof(mMailGroup));
	bServiceTerminating = FALSE;
#ifdef USE_SSL
	bUsedSSL = GetProfileIntEx(SYSTEM_REG, "UsedSSL", (int) FALSE);    // TRUE:SSL使用, FALSE:SSL不使用
    GetProfileStringEx(SYSTEM_REG, "Certificate", "", mCertificate, sizeof(mCertificate)); // SSL/TSL 証明書
    GetProfileStringEx(SYSTEM_REG, "Private-Key", "", mPrivatekey, sizeof(mPrivatekey)); // SSL/TSL 個人鍵
#ifdef USE_STARTTLS
	if (mCertificate[0] && mPrivatekey[0])
	{
	  bUsedSTLS = GetProfileIntEx(SYSTEM_REG, "UsedSTARTTLS", (int) TRUE);  // TRUE:STARTTLS使用, FALSE:STARTTLS不使用
	} else {
	  bUsedSTLS = FALSE;
	}
#endif
#endif
#ifdef ADD_IDCACHE
	nIDCashLiveTime = GetProfileIntEx(SYSTEM_REG, "ADCashLiveTime", (INT) 0); // MXキャッシュ利用有効時間(秒単位) デフォルト 0:無効
#endif
#ifdef UPDATE_20151215 // 高負荷かつ同一アカウントへ複数ログイン時にIMAP用INBOXへのメール確認で重複メールが作成される可能性がある不具合
	nIMAPIndexLiveTime = GetProfileIntEx(SYSTEM_REG, "IMAPIndexLiveTime", (INT) 30); // MXキャッシュ利用有効時間(秒単位) デフォルト5秒 0:無効
#endif
#ifdef UPDATE_20151225 // 試作：UID値によるファイル範囲指定のチューニング
	bQueryFileArea = GetProfileIntEx(SYSTEM_REG, "QueryFileArea", FALSE); // UID値範囲指定時の処理方法選択 デフォルト FALSE:従来方式 TRUE:範囲指定方式
#endif
#ifdef UPDATE_20151226 // IDLE処理の修正
    bIDLEBroadcast = GetProfileIntEx(SYSTEM_REG, "IDLEBroadcast", FALSE); // IDLE時の参加セッションへメッセージ出力を、自分のセッションより先に行うか否かの選択 デフォルト 0:後に送信 1:先に送信
#endif
#ifdef UPDATE_20230627 // UPDATE_20230624の有効無効フラグを追加
    bBulkSearch = GetProfileIntEx(SYSTEM_REG, "BulkSearch", TRUE); // メールデータを事前に一括して読込み検索するフラグを追加 デフォルト 1:する 0:しない
#endif
#ifdef UPDATE_20230706 // FETCH時のメールデータを事前に一括して読込み検索するフラグを追加
    bBulkFetch = GetProfileIntEx(SYSTEM_REG, "BulkFetch", FALSE); // FETCH時のメールデータを事前に一括して読込み検索するフラグを追加 デフォルト 1:する 0:しない
#endif
#ifdef UPDATE_20231003 // データ受信時のCPU負荷を下げる対策
    nPeekCoolTime= GetProfileIntEx(SYSTEM_REG, "BodyPeekCoolTime", (INT) 0);  //0:無効 デフォルト 50 100 500 1000 5000 10000
#endif
#ifdef UPDATE_20231209 // FETCH命令中に通信切断された場合に大きなサイズのデータ送信の中断を行えるようにした。
    bSendErrEscape = GetProfileIntEx(SYSTEM_REG, "SendErrEscape", TRUE); //送信エラー時中断 1:する(デフォルト) 0:しない
#endif
#ifdef UPDATE_20230714E // Content-Disposition:ヘッダの"filename="の項目のみ出力するオプションを追加　デフォルト 0:filenameのみ 1:他含む
    bDisposition = GetProfileIntEx(SYSTEM_REG, "FetchDisposition", FALSE);  // IDLE時の参加セッションへ同報するか否かの選択 デフォルト 0:同報しない 1:同報する
#endif

	//////////////////////////////////////////////////////////////
    GetProfileStringEx(SYSTEM_REG,"ImagePath", "", mProgPath, sizeof(mProgPath));
    for (nl = strlen(mProgPath)-1; nl > 0; nl--) {
	  if (mProgPath[nl] == '\\') {
		mProgPath[nl+1] = '\x0';
		break;
	  }
	}
	//////////////////////////////////////////////////////////////
	strcpy(mMailQueueDir, "incoming");
#ifndef E_POST
    GetProfileStringEx("SYSTEM\\CurrentControlSet\\Services\\SMTPRS\\Parameters","MailQueueDir", "incoming", mMailQueueDir, sizeof(mMailQueueDir));
#endif
	if (stricmp(mMailQueueDir, "incoming") == 0)
      GetProfileStringEx(SYSTEM_PARAM_REG,"MailQueueDir", "incoming", mMailQueueDir, sizeof(mMailQueueDir));

	nl = strlen(mMailQueueDir);
	if (nl != 0) {
	  nl--;
	  if (mMailQueueDir[nl] != '\\')
	    strcat(mMailQueueDir,"\\");
	}

#ifdef UPDATE_20110301B // サービス停止で残ったユーザ毎のロックファイルをサービス再起動後に一旦削除する対策
	sprintf(mImapStartFile,"%simapstart", mProgPath); // 非応答IPテーブル
	///////////////////////////////////////
	// 古いファイルを削除
	while((fp = fopen(mImapStartFile, "rt")))
	{
	  fclose(fp);
	  _unlink(mImapStartFile);
      if (bServiceTerminating)
        break;
      _sleep(WAIT_TIME);
	}
	///////////////////////////////////////
    if ((fp = fopen(mImapStartFile, "wt")))
	{
	  fclose(fp);
	}
#endif
	//////////////////////////////////////////////////////////////
#ifdef UPDATE_20060315 // 隠すIPアドレス
	sprintf(mHiddenMyIPFile,"%shiddenimapip.dat", mProgPath); // 非応答IPテーブル
#endif
#ifdef UPDATE_20151122 // 特定接続元IPからの接続拒絶用テーブル追加(rejectimap4ip.dat)隠しオプション
	sprintf(mRejectPeerPFile,"%srejectimapip.dat", mProgPath); 
#endif
#ifdef UPDATE_20160113 // 認証時のロックアウト機能を追加
	nAuthLockOut = GetProfileIntEx(SYSTEM_REG, "AuthLockOut", (int)0); // ロックアウトまでの回数 デフォルト 0:無効
#endif
#ifdef UPDATE_20160114 // 接続拒絶用テーブルの指定に有効時間を設定可能にした。
	nAuthLockOutTime = GetProfileIntEx(SYSTEM_REG, "AuthLockOutTime", (INT)0); // ロックアウト時間 デフォルト 0:無限 分単位
#endif
#ifdef UPDATE_20190216 // IP毎の同時接続ロックアウト機能を追加
    nIPLockOut = GetProfileIntEx(SYSTEM_REG, "IPLockOut", (INT)0); // ロックアウトまでの回数 デフォルト 0:無効
    nIPLockOutTime = GetProfileIntEx(SYSTEM_REG, "IPLockOutTime", (INT)0); // ロックアウト時間 デフォルト 0:無限 分単位
#ifdef UPDATE_20190218 // IP毎の同時接続ロックアウト機能を強化（サンプル時間と拒絶時間の設定を追加）
	nIPLockOutSPTime = GetProfileIntEx(SYSTEM_REG, "IPLockOutSPTime", (INT)0); // IPロックアウトまでのサンプリング時間 デフォルト 0:無限 秒単位
#endif
#endif
	//////////////////////////////////////////////////////////////
	bTrace = GetProfileIntEx(SYSTEM_REG, "Trace", (int)FALSE);
	sprintf(mTraceFile,"%strace.dat", mProgPath);
	//////////////////////////////////////////////////////////////
#ifdef UPDATE_20240728 // サブネットマスクの範囲指定の高速化
     bIPcmpmaskengin = GetProfileIntEx(SYSTEM_REG, "IPcmpmaskengin", (int)TRUE); // ネットマスクの処理方式 デフォルト TRUE:新型 FALSE:旧型
#endif
#ifdef ADD_XOAUTH2 // OAUTH2での認証方式を追加
	sprintf(mOAuthFile, "%soauth2.dat", mProgPath);
#endif
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
    sprintf(mTempDir, "%s%s", mMailSpoolDir, mMailQueueDir);
	_mkdir(mTempDir);         // incomingフォルダ作成
	sprintf(mLockFile, "%s%s", mTempDir, "$LockFile");
	if (bLog) {
  	  sprintf(mTempDir, "%simap4log", mMailSpoolDir);
	  _mkdir(mTempDir);         // inlogフォルダ作成
#ifdef REGTOFILE
      if (nClustering) {
  	    sprintf(mTempDir, "%simap4log\\%s", mMailSpoolDir, mComName);
	    _mkdir(mTempDir);         // inlogフォルダ作成
	  }
#endif
	}
#ifdef REGTOFILE
    if (bAcceptLog) {
      if (nClustering) {
	    sprintf(mTempDir, "%sreceiveimap4\\%s", mMailSpoolDir, mComName);
	    _mkdir(mTempDir);         // receivepop3フォルダ作成
	  }
	}
#endif
	/*
    if (bVirus) {
      sprintf(mTempDir, "%sviruslog", mMailSpoolDir);
  	  _mkdir(mTempDir);         // 接続ログフォルダ作成
	}
	if (bAnnounce) {
      sprintf(mTempDir, "%sattach", mMailSpoolDir);
	  _mkdir(mTempDir);         // 広告用フォルダ作成
	}
	*/
#ifdef CLUSTERING
	if (nClustering) {
      sprintf(mTempDir, "%scluster", mMailSpoolDir);
	  _mkdir(mTempDir);         // 処理用フォルダ作成
	}
#endif
#ifdef UPDATE_20141220 // 操作フォルダをアクセス中のセッションに変化をリアルタイムに同報するための対策
    if (bBroadcastSession)
	{
      sprintf(mTempDir, "%sreg\\" DOMAIN_IMAP4SESS, mMailSpoolDir);
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
	  _mkdir(mTempDir);         // 処理用フォルダ作成
	}
#endif
#ifdef UPDATE_20160128 // 存在しないアカウントへの認証失敗ロックアウト管理用フォルダを作成
    sprintf(mTempDir, "%sauthlock", mMailSpoolDir);
	_mkdir(mTempDir);
#endif
#ifdef UPDATE_20190216 // IP毎の同時接続ロックアウト機能を追加
    sprintf(mTempDir, "%siplock", mMailSpoolDir);
	_mkdir(mTempDir);
#endif
    sprintf(mTempDir, "%stemp", mMailSpoolDir);
	_mkdir(mTempDir);         // 処理用フォルダ作成
	strcat(mTempDir, "\\");
	//////////////////////////////////////////////////////////////
	//// レジストリ内容の表示
	//////////////////////////////////////////////////////////////
	sprintf(mLog, "---- regestry ----");
	printf("%s\n", mLog);
#ifdef ACCEPT_LOG_LEVEL3
    LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
	sprintf(mLog, "Licence key         %s (len=%d)", mLicencekey, strlen(mLicencekey));
	printf("%s\n", mLog);
#ifdef ACCEPT_LOG_LEVEL3
    LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
	sprintf(mLog, "Priority            %d", nPriority);
	printf("%s\n", mLog);
#ifdef ACCEPT_LOG_LEVEL3
    LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
#ifdef V3
	#ifdef LDAP_ON
	sprintf(mLog, "User Manager        %s", bUserMan ? (bLDAP ? "LDAP use" : "NT use") : "SPA use");
#else
	sprintf(mLog, "User Manager        %s", bUserMan ? "NT use" : "SPA use");
#endif
	printf("%s\n", mLog);
#ifdef ACCEPT_LOG_LEVEL3
    LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
	if (bUserMan)
	  sprintf(mLog, "Membership          %s", (mAuthDomain[0] ? mAuthDomain : "workgroup"));
	else
	  sprintf(mLog, "Account folder      %s", (mAuthDomain[0] ? mAuthDomain : "current"));
	printf("%s\n", mLog);
#ifdef ACCEPT_LOG_LEVEL3
    LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
#else
	sprintf(mLog, "Membership          %s", (mAuthDomain[0] ? mAuthDomain : "workgroup"));
	printf("%s\n", mLog);
#ifdef ACCEPT_LOG_LEVEL3
    LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
#endif
#ifdef UPDATE_20091116 // CAPABILITY 命令で応答する認証方法を設定変更可能にした。
	sprintf(mLog, "Auth type           %s", mImapAuthType);
	printf("%s\n", mLog);
#ifdef ACCEPT_LOG_LEVEL3
    LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
#endif

#ifdef UPDATE_20060606 // ドメインコントローラへの接続リトライ
#ifdef UPDATE_20091103 // LDAP連携専用のリトライ回数指定の変数に変更
    sprintf(mLog, "AD query retry      %lu(ms) x %lu(time)", nADRetryMSec, nADRetryTime);
#else
	sprintf(mLog, "AD query retry time %lu", nADRetryTime);
#endif
	printf("%s\n", mLog);
#ifdef ACCEPT_LOG_LEVEL3
    LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
#endif
#ifdef ADD_IDCACHE
	sprintf(mLog, "AD Cache live time  %lu(s)", nIDCashLiveTime);
	printf("%s\n", mLog);
#ifdef ACCEPT_LOG_LEVEL3
    LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
#endif

#if defined(LDAP_ON) && defined(UPDATE_20091103) // LDAP連携専用のリトライ回数指定の変数に変更
	if (bLDAPOn)
	{
	  sprintf(mLog, "LDAP query retry    %lu(ms) x %lu(time)", nLDAPRetryMSec, nLDAPRetryTime);
	  printf("%s\n", mLog);
#ifdef ACCEPT_LOG_LEVEL3
      LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
	}
#endif

#ifdef USE_SSL
	sprintf(mLog, "IMAP4 over SSL      %s", (bUsedSSL ? "yes" : "no"));
	printf("%s\n", mLog);
#ifdef ACCEPT_LOG_LEVEL3
    LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
#ifdef USE_STARTTLS
    sprintf(mLog, "STARTTLS            %s", (bUsedSTLS ? "on" : "off"));
    printf("%s\n", mLog);
#ifdef ACCEPT_LOG_LEVEL3
    LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
#endif
	sprintf(mLog, "Certificate         %s", mCertificate);
	printf("%s\n", mLog);
#ifdef ACCEPT_LOG_LEVEL3
    LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
	sprintf(mLog, "Private-Key         %s", mPrivatekey);
	printf("%s\n", mLog);
#ifdef ACCEPT_LOG_LEVEL3
    LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
#endif
#ifdef IPv6
	sprintf(mLog, "Host IP version     %s", nAddressFamily ? (nAddressFamily == 2 ? "IPv4/6" : "IPv6") :"IPv4");
	printf("%s\n", mLog);
#ifdef ACCEPT_LOG_LEVEL3
    LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
	sprintf(mLog, "Host Name(IPv6)     %s", mHostName);
	printf("%s\n", mLog);
#ifdef ACCEPT_LOG_LEVEL3
    LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
#endif
	sprintf(mLog, "Accept limit        %lu%s", nAcceptLimit, (nAcceptLimit ? "":"(Unlimited)") );
	printf("%s\n", mLog);
#ifdef ACCEPT_LOG_LEVEL3
    LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
	sprintf(mLog, "Recv Data Timer     %s %d(s)", (nTMOut ? "on" : "off"), nTMOut);
	printf("%s\n", mLog);
#ifdef ACCEPT_LOG_LEVEL3
    LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
	sprintf(mLog, "Send Data Timer     %s %d(s)", (nSendTMO ? "on" : "off"), nSendTMO);
	printf("%s\n", mLog);
#ifdef ACCEPT_LOG_LEVEL3
    LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
    sprintf(mLog, "Send socket buff    ");
	if (nSendBuf > 0)
	  sprintf(mLog, "%d", nSendBuf);
	else
	  sprintf(mLog, "default");
	printf("%s\n", mLog);
#ifdef ACCEPT_LOG_LEVEL3
    LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
	sprintf(mLog, "Imap4 IP            permit");
	if (bListenMode) {
	  pldom = mListenIP;
	  while (strlen(pldom)){
	    sprintf(mLog, " \"%s\"", pldom);
	    printf("%s\n", mLog);
#ifdef ACCEPT_LOG_LEVEL3
        LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
        pldom += strlen(pldom);
 	    pldom++;
	  };
	  sprintf(mLog, "");
	  printf("%s\n", mLog);
#ifdef ACCEPT_LOG_LEVEL3
      LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
	} else {
      sprintf(mLog, " all");
	  printf("%s\n", mLog);
#ifdef ACCEPT_LOG_LEVEL3
      LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
	}
	sprintf(mLog, "Imap4 Port          %d", nport);
	printf("%s\n", mLog);
#ifdef ACCEPT_LOG_LEVEL3
    LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
#ifdef UPDATE_20190510 // NTFS以外のファイルシステムからのファイル一覧取得可能にする対策（一覧ソート）を追加
    sprintf(mLog, "FileSystem          \"%s\"", (bOtherFS ? "Any Other" : "NTFS Only"));
	printf("%s\n", mLog);
#ifdef ACCEPT_LOG_LEVEL3
    LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
#endif
	sprintf(mLog, "MailGroup           \"%s\"",mMailGroup);
	printf("%s\n", mLog);
#ifdef ACCEPT_LOG_LEVEL3
    LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
	sprintf(mLog, "MailSpoolDir        %s", mMailSpoolDir);
	printf("%s\n", mLog);
#ifdef ACCEPT_LOG_LEVEL3
    LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
	sprintf(mLog, "MailBoxDir          %s", mMailBoxDir);
	printf("%s\n", mLog);
#ifdef ACCEPT_LOG_LEVEL3
    LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
	sprintf(mLog, "MailDataExtension   *.%s", mMailExtension);
	printf("%s\n", mLog);
#ifdef ACCEPT_LOG_LEVEL3
    LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
	sprintf(mLog, "Shared Root Dir     %s %s", (mSharedRoot[0] ? mSharedRoot : "NULL"), (mSharedRoot[0] ? (bSharedRW ? "rw" : "r") : ""));
	printf("%s\n", mLog);
#ifdef ACCEPT_LOG_LEVEL3
    LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
    sprintf(mLog, "Program Path        %s", mProgPath);
	printf("%s\n", mLog);
#ifdef ACCEPT_LOG_LEVEL3
    LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
    sprintf(mLog, "Imap Password File  %s", mPasswordFile);
	printf("%s\n", mLog);
#ifdef ACCEPT_LOG_LEVEL3
    LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
#ifdef V4
    sprintf(mLog, "Greeting messages   %s", bHide ? "hide" : "show");
	printf("%s\n", mLog);
#ifdef ACCEPT_LOG_LEVEL3
    LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
    sprintf(mLog, "Use time File       %s", mUseTimeFile);
	printf("%s\n", mLog);
#ifdef ACCEPT_LOG_LEVEL3
    LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
#ifdef UPDATE_20110222 //UPDATE_20110202でパフォーマンス改善の対策。
    sprintf(mLog, "UID Recover         %s", bUIDRecover ? "on" : "off");
	printf("%s\n", mLog);
#ifdef ACCEPT_LOG_LEVEL3
    LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
#endif
#endif
	sprintf(mLog, "Pop3 isn't using    %s", bOFFPOP3 ? "on":"off");
	printf("%s\n", mLog);
#ifdef ACCEPT_LOG_LEVEL3
    LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
	sprintf(mLog, "AcceptImap4 Log     %s", bAcceptLog ? "on":"off");
	printf("%s\n", mLog);
#ifdef ACCEPT_LOG_LEVEL3
    LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
	sprintf(mLog, "Imap4Log            %s", bLog ? "on":"off");
	printf("%s\n", mLog);
#ifdef ACCEPT_LOG_LEVEL3
    LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif

#ifdef UPDATE_20151115 // SELECT時にメール一覧をキャッシュするためのポインタ
	sprintf(mLog, "Select box cache    %s", bSELECT ? "on":"off");
	printf("%s\n", mLog);
#ifdef ACCEPT_LOG_LEVEL3
    LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
#endif

#ifdef UPDATE_20140528 // IDLEコマンド実装
	sprintf(mLog, "IDLE Command        %s", bIDLECmd ? "on":"off"); // IDLEコマンド有効有無 TRUE:有効 FALSE:無効
	printf("%s\n", mLog);
#ifdef ACCEPT_LOG_LEVEL3
    LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
	sprintf(mLog, "IDLE Loop timer     %d * %d (ms)", nIDLELoop, nIDLEMSLoop); // IDLEタイマー（一定時間新着チェック）
	printf("%s\n", mLog);
#ifdef ACCEPT_LOG_LEVEL3
    LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
#ifdef UPDATE_20140610 // IDLEコマンド実行中の無通信タイムアウト時間をデフォルトで30分にした。
	sprintf(mLog, "IDLE Timeout timer  %d(s)", nIDLETMOut); // IDLE実行中の無通信タイマーデフォルト30分
	printf("%s\n", mLog);
#ifdef ACCEPT_LOG_LEVEL3
    LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
#endif
#endif
#ifdef UPDATE_20140530 // // NOOPによるポーリング結果送信有無
	sprintf(mLog, "NOOP Polling        %s", bNOOPPoll ? "on":"off"); // NOOPによるポーリング結果送信有無 TRUE:有効 FALSE:無効   
	printf("%s\n", mLog);
#ifdef ACCEPT_LOG_LEVEL3
    LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
#endif
#ifdef UPDATE_20141220 // 操作フォルダをアクセス中のセッションに変化をリアルタイムに同報するための対策
    //if (bBroadcastSession)
	sprintf(mLog, "Broadcast Session   %s", bBroadcastSession ? "on":"off"); // 操作フォルダをアクセス中のセッションに変化をリアルタイムに同報する TRUE:有効 FALSE:無効   
	printf("%s\n", mLog);
#ifdef ACCEPT_LOG_LEVEL3
    LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
#endif
#ifdef UPDATE_20151226 // IDLE処理の修正
    sprintf(mLog, "Broadcast Idle      %s", (bIDLEBroadcast ? "on" : "off")); // IDLE時の参加セッションへ同報するか否かの選択 デフォルト 0:同報しない 1:同報する
	printf("%s\n", mLog);
#ifdef ACCEPT_LOG_LEVEL3
    LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
#endif
	sprintf(mLog, "-------------------");
	printf("%s\n", mLog);
#ifdef ACCEPT_LOG_LEVEL3
    LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
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
                    PIPE_ACCESS_DUPLEX,     // pipe open mode
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

    AddToMessageLog(TEXT(IMAP4_NAME), 0, TEXT(VERSION), (WORD)EVENTLOG_INFORMATION_TYPE);
    // init the overlapped structure
    //
    memset( &os, 0, sizeof(OVERLAPPED) );
    os.hEvent = hEvents[1];
    ResetEvent( hEvents[1] );
	dwc = 0;
#ifdef USE_SSL
#ifdef UPDATE_20060512 // SSLの初期化はサービス開始で１回のみ
#ifndef UPDATE_OPESSL1_1_1 // openssl1.1.1系に置換える場合
    SSL_library_init();
    SSLeay_add_ssl_algorithms();
    SSL_load_error_strings();
#endif
#endif
#endif
#ifdef SOCKET_FAIL_TEST // 強制テスト：テスト的にエラーを発生 ソケット無応答状態の動作調査用
	bFT = TRUE;
#endif
    while ( 1 )
    {
        ConnectNamedPipe(hPipe, &os);
        if ( GetLastError() == ERROR_IO_PENDING )
        {
          dwWait = WaitForMultipleObjects( 2, hEvents, FALSE, 1000);
          if ( dwWait == 0 )
		    break; 
          Copyright();
		  if (IMAP4SMain() == FALSE) {
			printf("(error) accept clients.\n");
			break;
		  }
		}
        // drop the connection...
        DisconnectNamedPipe(hPipe);
    }
#ifdef USE_SSL
#ifdef UPDATE_20060512 // SSLの初期化はサービス開始で１回のみ
#ifndef UPDATE_OPESSL1_1_1 // openssl1.1.1系に置換える場合
    ERR_free_strings();
#endif
#endif
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

#ifdef UPDATE_20151226 // 当分封印：IDLE処理の修正 (同一フォルダにアクセス中のセッションに変化が伝わらない不具合)
    DeleteCriticalSection(&g_csIdleMess);
#endif
#ifdef UPDATE_20151215A // 高負荷かつ同一アカウントへ複数ログイン時にIMAP用INBOXへのメール確認で重複メールが作成される可能性がある不具合
    DeleteCriticalSection(&g_csMoveMess);
#endif
#ifdef UPDATE_20090322 // ログイン時のロックファイルのクリティカルセクション化
    //////////////////////////////////////////////
    // ログイン時のロックファイルフラグ削除
    DeleteCriticalSection(&g_csLogin);
#endif
#ifdef UPDATE_20101221 // UID値ファイルのクリティカルセクション化
    DeleteCriticalSection(&g_csWriteUID);
#endif

}

BOOL FillAddr(PSOCKADDR_IN psin, char *mSVR) {
   PHOSTENT phe;
   unsigned long inaddr;
   BOOL bAddr = FALSE;

   psin->sin_family = AF_INET;
   if (mSVR[0] == '*')
	 strcpy(mSVR, "localhost");
   if (strlen(mSVR) > 0) {
     inaddr = inet_addr(mSVR);
  	 if (inaddr != INADDR_NONE) { // = (unsigned long)0xffffffff) {
	    bAddr = TRUE;
	    memcpy((char FAR *)&(psin->sin_addr), &inaddr, 4);
     } else {
        phe = gethostbyname(mSVR);
	    if (phe) {
	      bAddr = TRUE;
          memcpy((char FAR *)&(psin->sin_addr), phe->h_addr, phe->h_length);
		}
	 }
   } 
   if (!bAddr) {
 	 printf("not found '%s'.(Code %d)\n", mSVR, WSAGetLastError());
     return FALSE;
   }
   /* actual port number entered */
   psin->sin_port = htons((USHORT)nport);        /* Convert to network ordering */

   return TRUE;
}

VOID ServiceStop()
{
#ifdef IPv6
#ifdef UPDATE_20071121 //IPv6で全てのIPアドレスに応答のとき停止が出来ない
	CHAR mV6LocalHost[] = "::1";
    char *Address = mV6LocalHost;
#else
    char *Address = NULL;
#endif
    struct addrinfo Hints, *AddrInfo, *AI;
	int  i;
    SOCKET msock[2];
#else
    SOCKET msock;
#endif
    SOCKADDR_IN dest_sin;  /* DESTination Socket INternet */
	DWORD  dwc;
    char   mport[16], szHostName[256], *p;

printf("Service Stoped.\n");

    //ReportStatusToSCMgr(SERVICE_STOP_PENDING, NO_ERROR, 5000);
    if ( hServerStopEvent )
        SetEvent(hServerStopEvent);
	bServiceTerminating = TRUE;
	memset(mListenIP, 0, sizeof(mListenIP));
    dwc = GetProfileStringEx(SYSTEM_REG,"ListenIP","", mListenIP, sizeof(mListenIP));
    memset(&mListenIP[dwc], 0, (sizeof(mListenIP)-dwc));
	nport = DEFAULT_PORT;
#ifndef E_POST
    nport = GetProfileIntEx("SYSTEM\\CurrentControlSet\\Services\\IMAP4S\\Parameters", "PortNo", (int)DEFAULT_PORT);
#endif
	if (nport == DEFAULT_PORT)
      nport = GetProfileIntEx(SYSTEM_PARAM_REG, "PortNo", (int)DEFAULT_PORT);
#ifdef IPv6
	i = 0;
	// IPv4 only 又は、IPv4/6混在のとき
	if (nAddressFamily == 0 ||
		nAddressFamily == 2 ) {
      msock[i] = socket( AF_INET, SOCK_STREAM, 0);
      if (msock[i] == INVALID_SOCKET) {
        if (bDebug) printf("socket() failed Error.\n");
		if (nAddressFamily == 0)
          return;
	  }
	  // gethostname(szHostName, sizeof(szHostName));
	  if (bListenMode) { // 1:特定のIPの場合
		p = mListenIP;
		while (*p) {
	      strcpy(szHostName, p);
		  if (strstr(szHostName, ".")) {
			p = strstr(p, " ");
			p++;
	        strcpy(mport, p);
			nport = atoi(mport);
			strtok(szHostName, " ");
			break;
		  }
		  p += strlen(p);
		  p++;
		}
	  }
	  gethostaddrname(mdomain);
      if (!FillAddr( &dest_sin, (bListenMode ? szHostName: mdomain))) {
        closesocket( msock[i] );
		if (nAddressFamily == 0)
          return;
	  }
      if (connect( msock[i], (PSOCKADDR) &dest_sin, sizeof( dest_sin)) < 0) {
        closesocket( msock[i] );
        if (bDebug) printf("sock connect failed Error.\n");
		if (nAddressFamily == 0)
          return;
	  }
	}
	// IPv6 only 又は、IPv4/6混在のとき
	i = 1;
	if (nAddressFamily == 1 ||
		nAddressFamily == 2 ) {
      memset(&Hints, 0, sizeof(Hints));
      //Hints.ai_family = (nAddressFamily ? AF_INET6 : AF_INET);
	  Hints.ai_flags = AI_NUMERICHOST | AI_PASSIVE;
      Hints.ai_family = AF_INET6;
      Hints.ai_socktype = SOCK_STREAM;
	  if (!bListenMode) { // 0:全てのIPの場合
	    itoa(nport, mport, 10);
	  } else { // 1:特定のIPの場合
		p = mListenIP;
		while (*p) {
	      strcpy(szHostName, p);
		  if (strstr(szHostName, ":")) {
			p = strstr(p, " ");
			Address = szHostName;
			p++;
	        strcpy(mport, p);
			strtok(Address, " ");
			break;
		  }
		  p += strlen(p);
		  p++;
		}
	  }
      if (getaddrinfo(Address, mport, &Hints, &AddrInfo) != 0) {
        if (bDebug) printf("getaddrinfo failed with error\n");
        //WSACleanup();
        return;
	  }
	  AI = AddrInfo;
      msock[i] = socket(AI->ai_family, AI->ai_socktype, AI->ai_protocol);
      if ( msock[i] == INVALID_SOCKET ) {
        return;
	  }
      if (connect(msock[i], AI->ai_addr, AI->ai_addrlen) < 0) {
        closesocket( msock[i] );
        if (bDebug) printf("sock connect failed Error.\n");
	    return;
	  }
	}
#else
    msock = socket( AF_INET, SOCK_STREAM, 0);
    if (msock == INVALID_SOCKET) {
      if (bDebug) printf("socket() failed Error.\n");
      return;
    }
	if (bListenMode) { // 1:特定のIPの場合
	  p = mListenIP;
	  while (*p) {
	    strcpy(szHostName, p);
	    if (strstr(szHostName, ".")) {
	   	  p = strstr(p, " ");
		  p++;
	      strcpy(mport, p);
		  nport = atoi(mport);
		  strtok(szHostName, " ");
	 	  break;
		}
		p += strlen(p);
		p++;
	  }
	}
	gethostaddrname(mdomain);
	if (!FillAddr( &dest_sin, (bListenMode ? szHostName : mdomain))) {
      closesocket( msock );
      return;
    }
    if (connect( msock, (PSOCKADDR) &dest_sin, sizeof( dest_sin)) < 0) {
      closesocket( msock );
      if (bDebug) printf("sock connect failed Error.\n");
      return;
    }
#endif
    if (bDebug) printf("sock connected.\n");
    return;
}
