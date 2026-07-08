////////////////////////////////////////////////////////////
// smtprs.c Copyright K.kawakami
////////////////////////////////////////////////////////////
#include "smtp.h"
#include "version.h"
#include "service.h"
#ifdef UPDATE_20080229 // 承認保留一覧に各メールの題名を表示する
#include "codepage.h"
#endif
#include <io.h>
#include <time.h>
#include <tchar.h>
#include <direct.h>
//#include <process.h>

#ifdef UPDATE_20181025 // 外部DLLの呼出しポインタをグローバル化
HMODULE hDLL;
#endif

#ifdef SOCKET_FAIL_TEST // 強制テスト：テスト的にエラーを発生 ソケット無応答状態の動作調査用
BOOL bFT = TRUE;
#endif

BOOL    bVLog; // イベントビューワにログ書込みエラーを表示する 0:しない　1:する

HANDLE  hServerStopEvent = NULL;

char *  SmtpCommands[] = {  SMTP_HELO,
#ifdef ESMTP_AUTH
                            SMTP_EHLO,
                            SMTP_AUTH,
                            SMTP_PASS,
#endif 
                            SMTP_MAIL,
                            SMTP_RCPT,
                            SMTP_DATA,
                            SMTP_DOT,
                            SMTP_RSET,
                            //SMTP_SEND,
                            //SMTP_SOML,
                            //SMTP_SAML,
                            SMTP_VRFY,
                            SMTP_EXPN,
                            SMTP_HELP,
                            SMTP_NOOP,
                            SMTP_QUIT
                            //SMTP_TURN,
                          };

#ifndef BTHREAD
SMTPRSDispatchFn  SMTPRSDispatchTable[] = {
                            HeloDispatch,
#ifdef ESMTP_AUTH
                            EhloDispatch,
                            AuthDispatch,
                            PassDispatch,
#endif 
                            MailDispatch,
                            RcptDispatch,
                            DataDispatch,
                            DotDispatch,
                            RsetDispatch,
                            //SendDispatch,
                            //SomlDispatch,
                            //SamlDispatch,
                            VrfyDispatch,
                            ExpnDispatch,
                            HelpDispatch,
                            NoopDispatch,
                            QuitDispatch,
                            //TurnDispatch,
                            BadDispatch
                         };
#endif

BOOL   gethostaddrname(char *hostname);
void   SendPipeLine(char *str);
BOOL   AcceptClients (HANDLE hCompletionPort);
void   SMTPRrvMain(int argc, char *argv[]);
HANDLE InitializeThreads (void);
#ifdef Y2038_BUG
BOOL GetTimeLimit(__int64 *ltime, char *mode, BOOL *bPassport);
#else
BOOL GetTimeLimit(time_t *ltbegin, char *mode, BOOL *bPassport);
#endif
/*
void hmac_md5(unsigned char* text, unsigned char* key, char *dest);
void MDString (char *string, char *dest);
void translateUue2Base64(char *line, int inlen, char *newline);
*/

#ifdef UPDATE_20080929A // ログのクリティカルセクション化
CRITICAL_SECTION g_csWriteReport;
#endif
#ifdef UPDATE_20091120 // ML保存時のindex.html作成のための排他処理、64bit長のファイルサイズに対応
CRITICAL_SECTION g_csMLIndex;
#endif

extern BOOL bDebug;

#ifdef Y2038_BUG
char mMonth[12][4] = {"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};
char mWeek[7][4]   = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
#endif

#ifdef V4
INT     nMAXUser;
#endif

#ifdef LGWAN_ON        // LGWAN機能拡張
BOOL   bLGWAN;
BOOL   bCHGHEADER;
#endif

#ifdef MILTER_ON // MILTERインターフェースを追加。
BOOL   bMILTER;         // MILTER機能拡張
BOOL   bMMACRO;         // MILTER MACRO追加
char   mMMCONNECT[256];
char   mMMHELO[256];
char   mMMENVFROM[256];
char   mMMENVRCPT[256];
char   mMMEOM[256];
char   mMILTERFile[256];
#endif
#ifdef UPDATE_20150212 // 接続元のアドレスによって受信メールを破棄するオプション
BOOL   bDISCARDMAIL;    // 受信メール破棄オプション
char   mDISCARDFile[256];
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

#ifdef UPDATE_20070407 // リンク先のドメイン名の調査する深さを調整可能に
DWORD   nURIRBLDepath; // URIBLのドメインの深さ制限　デフォルト 2;
#endif

#ifdef UPDATE_20220427 // リッスンしたIPアドレスのFQDNをホスト名として優先する。
BOOL bListenAddrPRI;
#endif

TMQueue mTMQueue;
DWORD   nTMOut;
//BOOL    bTMQWait;
//BOOL    bThreadWait;
BOOL    bVrfy;
int     nRecvBuf;
DWORD   nMailFiterUp; // メールフィルタ強化
#ifdef DOMAIN_AUTH_SPF    // 2006.04.10 ドメイン認証 SPF方式 追加
BOOL    bSPF;
CHAR   mDomainAUTHSPF[256];
CHAR   mNS[256];
CHAR   mNS1[256];
CHAR   mNS2[256];
CHAR   mNS3[256];
#endif
#ifdef DOMAIN_AUTH_DKIM    // 2013.01.17 ドメイン認証 DomainkeysとDKIM方式 追加
#ifdef UPDATE_20240118 // DKIM設定箇所の選択 bit0:(受信時付加)デフォルト bit1:上長承認メールへ付加 bit2:添付分離めーるへ付加
int     bDKIM;
#else
BOOL    bDKIM;
#endif
CHAR   mDomainAUTHDKIM[256];
#endif
#ifdef SENDERID
BOOL   bSenderID;
#endif
#ifdef EIGHT_BITMIME
BOOL   b8BITMIME;
#endif


#ifdef V3
BOOL    bUserMan;
#endif
int     nPriority;
int     nSendTMO;
DWORD   nAcceptCount;
DWORD   nAcceptLimit;
int     nAddressFamily;
#ifdef ADD_IDCACHE
int     nIDCashLiveTime; // ADキャッシュ利用有効時間(秒単位)
#endif
int     nConnectAF;
int     nport;
BOOL    bLog;
BOOL    bAcceptLog;
BOOL    bUsedSaveList;
BOOL    bConfirmReversDNS;
DWORD   nPatternOfTheOnce;
DWORD   nMailInMaxSize;
#ifdef UPDATE_ULONGLONG //64bit長のデータで取得可能にする
DWORD   nMailInBoxSizeHigh;
#endif
DWORD   nMailInBoxSize;
DWORD   nRCPTMaximum;
DWORD   nFROMSecLevel; // 認証ＩＤ一致レベル 0:SMTP-AUTH ID = 1:SMTP-AUTH ID And MAIL FROM: = 2: SMTP-AUTH ID and MAIL FROM: and HEADER FROM:
char    mLog[2048];
char    mAuthDomain[64];
char    mHostName[256];
char    mProgPath[256];
char    mdomain[256];
char    mMailGroup[256];
BOOL    bCountLock;
//BOOL    bCloseLock;
DWORD   nLastMsgId;
#ifdef UPDATE20240620 // effect.datでクラスB以上のサブネットマスクの指定に不具合
BOOL    bEffectmaskengin;
#endif
#ifdef UPDATE_20240219 // 付加表題付きのＭＬでの文字化け対策
char    mUTF8ToSJISTBL[0x4000]; // レジストリ：バイナリの限界値 [4096];
DWORD   nUTF8ToSJISTBL;
#endif
#ifdef UPDATE_20240228 // 付加表題もまとめてパックするオプション
BOOL    bMLtknInc;  // TRUE:まとめる FALSE:分離したまま(従来)
#endif
char    mLocalDomain[0x4000]; // レジストリ：バイナリの限界値 [4096];
DWORD   nLocalDomain;
char    mMailInCopy[4096];
DWORD   nMailInCopy;
char    mMailSpoolDir[128];
char    mMailQueueDir[128];
#ifdef K_SEARCH // K_SEARCH OEM 版
char    mMailQueueSubDir[256];
int     nQueueUnit; // 0:日単位 1:月単位 2:年単位
#endif
#ifdef UPDATE_20061102 // 受信メールのエンベロープと本文の任意のフォルダへのバックアップ機能の追加
BOOL    bMailBackup;
char    mMailBackupDir[256];
#ifdef UPDATE_20090205 // バックアップメッセージの拡張子を指定するオプション
char    mExtension[256];
#endif
#endif
#ifdef UPDATE_20070516 // 上長承認機能の追加
#ifdef UPDATE_20090213 // 上長承認メールのmailto:リンクの区切り記号を"?"以外に変更するオプションを追加
char    mMailApprovalMailTo[16];
char    mMailApprovalMID[16];
#endif
BOOL    bMailApproval;
BOOL    bMailApprovalLog;
#ifdef UPDATE_20231017 // 上長承認依頼メールの作成に失敗した場合の作成リトライ
int     nMailApprovalMessNum;    // BossCheck メッセージ作成失敗時のリトライ回数
#endif
#ifdef UPDATE_20140623B // // 上長承認機能の履歴作成時の詳細さ設定フラグを追加した（送信元IPや送信先を）0:含まない(デフォルト) 1:含む
int     nMailApprovalLogLevel;
#endif
char    mMailApprovalDir[256];
char    mMailApprovalManager[256];
#ifdef UPDATE_20090114 //BossCheck 承認者数の設定
DWORD   nMailApprovalNum;
#endif
#ifdef UPDATE_20080221 //BossCheck 承認者の代理人の代理人の深さの設定
DWORD   nMailApprovalDepath;
#endif
#ifdef UPDATE_20080225 // 部下に対する承認のため保留通知
BOOL    bMailApprovalRes;
#endif
#ifdef UPDATE_20161111 /// 外部からの送信元送信先制限(逆上長承認)
BOOL    bMailApprovalRejectRes;    // 却下通知の有効有無
#endif
#ifdef UPDATE_20080514 // 上長宛承認メールのmailto:で記載した返信先アドレスのドメインを強制変更する設定
char    mMailApprovalDomain[256];
#endif
#ifdef UPDATE_20080620 // TOアドレスが代理人になる機能追加
BOOL    bSetProxyUserType;
#endif
#endif
#ifdef UPDATE_20080326 // ドメイン無しアカウントをローカルとしての利用有無選択機能追加
BOOL bHaveDomain;
#endif
#ifdef UPDATE_20090910 // report-type="disposition-notification" なら添付無しと判定する
BOOL    bMailApprovalNotification;
#endif
#ifdef UPDATE_20091203 // 承認、否承認を一括処理するためのワイルドカード指定機能のON/OFFオプションを追加した。
BOOL    bMailApprovalWildcard;
#endif
#ifdef UPDATE_20141210 // 承認依頼メールのmailtoリンクの文字コード指定
INT     nMailApprovalCodepage;
#endif
#ifdef UPDATE_20150324 // エンベロープの送信先をファイルへの書込みエラーが発生した場合、リトライを最大５回行う
int     nRcptWRetry; // エンベロープの送信先のファイル書込みエラー時のリトライ回数
#endif
#ifdef UPDATE_20190329 // ML同報時の移動状態を詳細ログに追加
INT     nMovefileRetry; // MoveFileEx()での失敗時のリトライ上限 0=無制限
#endif
CHAR    mMailBoxDir[_MAX_PATH];
char    mLockFile[128];
char    mTempDir[128];
BOOL    bTrace;
char    mTraceFile[256];
FILE    *fTrace;
char    mORDBFile[256];
char    mPASSDBFile[256];
char    mPASSFilterFile[256];
char    mPASS3rdFile[256];
char    mPASSFTMFile[256];
char    mHiddenMyIPFile[256];
#ifdef UPDATE_20151122 // 特定接続元IPからの接続拒絶用テーブル追加(rejectsmtpip.dat)隠しオプション
char	mRejectPeerPFile[256];
#endif
#ifdef UPDATE_20211020 // 特定接続元IPからの複写転送先指定無効アドレス設定テーブル追加(ccdisableip.dat)
char    mCCDisableFile[256]; // 複写転送先指定無効アドレス設定テーブル
#endif
#ifdef UPDATE_20160113 // 認証時のロックアウト機能を追加
DWORD	nAuthLockOut; // ロックアウトまでの回数 デフォルト 0:無効
#endif
#ifdef UPDATE_20160114 // 接続拒絶用テーブルの指定に有効時間を設定可能にした。
DWORD	nAuthLockOutTime; // ロックアウト時間 デフォルト 0:無限 分単位
#endif
#ifdef UPDATE_20190216 // IP毎の同時接続ロックアウト機能を追加
DWORD	nIPLockOut; // IPロックアウトまでの回数 デフォルト 0:無効
DWORD	nIPLockOutTime; // IPロックアウト時間 デフォルト 0:無限 秒単位
#ifdef UPDATE_20190218 // IP毎の同時接続ロックアウト機能を強化（サンプル時間と拒絶時間の設定を追加）
DWORD	nIPLockOutSPTime; // IPロックアウトまでのサンプリング時間 デフォルト 0:無限 秒単位
#endif
#endif
#ifdef ADD_XOAUTH2 // OAUTH2での認証方式を追加
char	mOAuthFile[256];
#endif
#ifdef UPDATE20240728 // サブネットマスクの範囲指定の高速化
BOOL    bIPcmpmaskengin;
#endif
#ifdef FIRST_TIME_MAIL // 繰り返す同一内容のメール除去
BOOL	bFTM_ON;    // 同一メールの保管チェックの有無
DWORD	nFTMExpire; // 同一メールの保管期間　デフォルト９０日
DWORD   nFTMLength; // 比較トークンの長さ　　デフォルト１０バイト
DWORD   nFTMMargin; // 比較結果の許容範囲　　デフォルト８０％一致で同一と判断
#endif
//char    mORDBSite[5][128]; // Site list of "Open relay data base"
BOOL    bVirus;
char    mVirusFile[256];
char    mMailFile[256];
#ifdef V3
int     nVirusDoubtful;
DWORD   nViursMailSzie;
char    mVirus2File[256];
#endif
DWORD   nReceived;
//BOOL    bAnnounce;
//BOOL    bAnnLimit;
DWORD   nAnnounceMax;
#ifdef Y2038_BUG
__int64 ltbegin;
#else
time_t  ltbegin;
#endif
char    mDefaultCode[32];
int     nMixPriority;
//BOOL    bPOPbeforeSMTP;
int     nSMTPAUTHOnly;
char    mSMTPAUTHMODE[64];
char    mPasswordFile[128];
#ifdef V4
BOOL    bHide;
#ifdef UPDATE_20070510 // 実ユーザ名でのメール送受信を禁止処理する処理
char    mUseSMTPFile[128];
#endif
char    mUseTimeFile[128];
#ifdef UPDATE_20050924  // 送信元ユーザ別送信先制限
char    mSenderPermitFile[128];
#endif
#endif
#ifdef UPDATE_20190108 // 承認メールをHTMLメールにする機能を追加
BOOL    bMailApprovalHtml; // 承認メールをHTMLメールに
#endif
#ifdef UPDATE_20161111 /// 外部からの送信元送信先制限(逆上長承認)
BOOL    bMailApprovalRevers;
BOOL    bMailApprovalWebOnly; // Web管理のみでは承認依頼メールを作らない
char	mRecivePermitFile[128];
char    mRecivePermitKeyFile[128];
char    mRecivePermitBlackFile[128];
char    mRecivePermitWhiteFile[128];
char    mRecivePermitBlackWordFile[128];
char    mRecivePermitWhiteWordFile[128];
#endif
#ifdef UPDATE_20170930 // 逆上長承認で承認者への送信元エンベロープが不正アドレス等で送信先に受信拒絶されないようにする対策
int     nMailApprovalReversType; // 送信エラーの返信先 0:送信しない 1:送信元 2:送信先指定
char    mMailApprovalReversFrom[256];
#endif
BOOL    bListenMode;
char    mListenIP[1024];
char    mListenIP2[1024];
#ifdef USE_SSL
#ifdef USE_STARTTLS
BOOL    bUsedSTLS;
#endif
BOOL    bUsedSSL;
char    mCertificate[256];
char    mPrivatekey[256];
#endif
#ifdef THIRDPARTY
char    mThirdparty[256];
#ifdef UPDATE_20240523 // ウイルスチェック時のログ保存先が自動生成されない不具合
BOOL    b3rdViruslog;
#endif
#endif

#ifdef CLUSTERING
BOOL   nClustering;
char   mMsgIDFN[256];
char   mMsgIDFNLock[256];
char   mComName[256];
DWORD  nPipeCnt;
char   mMailTempDir[256]; // テンポラリパス
#endif
int    nReadyDriveTime;
#ifdef UPDATE_20051121 // メール連続受領に対する配送キュー調整
DWORD  nSpoolFsSync;
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
#ifdef  INCOM_SUB_FOLDER // incoming フォルダをスレッド別フォルダにして処理
BOOL    bIncomingSubFolder; // incomingフォルダにスレッド別フォルダを作成 TRUE:する FALSE:しない
BOOL	bThreadType;
DWORD   nMaxThread;
#endif
#ifdef SHORT_SURBL // 短縮URLのSURBL判定対策
CHAR     mShortDecodeFile[256];
#endif
#ifdef UPDATE_20240205 // 承認依頼メールで１行当たりの文字数を正規化
int     nNomalColumnMaxSize; //デフォルト 78byte/Line
#endif
#ifdef UPDATE_20241226 // AUTH LOGINでの応答時チャレンジ文字列の設定をレジストリから変更できるようにした。
CHAR mChallengeTokenLOGINUser[32];
CHAR mChallengeTokenLOGINPass[32];
#endif

SOCKET  sListener;
HANDLE  hCompletionPort;
WSADATA WsaData;
BOOL    bServiceTerminating = FALSE;
#ifdef UPDATE_20070509 // エイリアス設定の実アドレスオプションを別データとして処理する
BOOL    CheckUser(char *user, char *pOpt, char *myaddr, BOOL *bLocal, BOOL *bSubLocal, BOOL *bMList, BOOL *bOutSideAliases, LPBYTE fullname);
#else
BOOL    CheckUser(char *user, char *myaddr, BOOL *bLocal, BOOL *bSubLocal, BOOL *bMList, BOOL *bOutSideAliases, LPBYTE fullname);
#endif
#ifdef Y2038_BUG
BOOL    ReSetTimeLimit(__int64 *ltbegin);
#else
BOOL    ReSetTimeLimit(time_t *ltbegin);
#endif
#ifdef ACCEPT_LOG_LEVEL3
void LEVEL_3_ACCEPTLOG(PCLIENT_CONTEXT lpClientContext, char *mess);
#endif

//#define TRACE 1

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

//#ifndef BTHREAD
int SMTPRSCrackCommand(
    PSmtpContext  pSmtpContext,
    PUCHAR        Request,
    DWORD         RequestLen)
{
	int  nEnd;
	char *pEnd, mEnd[6];
      // DATA State -> .(dot) State
	if (pSmtpContext->State == SmtpDataError) {
	  if (strlen(pSmtpContext->mToken)+ RequestLen < sizeof(pSmtpContext->mToken)) {
        strncat(pSmtpContext->mToken, Request, RequestLen );
        if (!strstr(pSmtpContext->mToken,"\n")) {
//printf("%s\n", pSmtpContext->mToken);
		   return(-1);
		}
		if (strlen(pSmtpContext->mToken) > 6) {
		  strcpy(pSmtpContext->mToken, "QUIT\r\n");
#ifdef ESMTP_AUTH
          return (13); // QUIT;
#else
          return (10); // QUIT;
#endif
		}
//printf("found <LF>\n");
	  } else {
		if (bDebug) printf("Token overflows.\n");
		pSmtpContext->mToken[0] = '\x0';
 	    return(-1);
	  }
      for (pSmtpContext->i = 0; pSmtpContext->i < MAX_SMTP_COMMAND ; pSmtpContext->i++ ){
#ifdef ESMTP_AUTH
		if (_strnicmp((PCHAR) (pSmtpContext->State == SmtpAuthentication ? SMTP_PASS : pSmtpContext->mToken), SmtpCommands[pSmtpContext->i], 4) == 0)
#else
		if (_strnicmp((PCHAR) pSmtpContext->mToken, SmtpCommands[pSmtpContext->i], 4) == 0)
#endif
		{
		  if (strstr(pSmtpContext->mToken,"\n") ){
#if TRACE_MODE
            if (pSmtpContext)
	          if (bDebug) printf("[%s]\n",pSmtpContext->mToken);
#endif
            return(pSmtpContext->i);
		  } else {
            return(-1);
		  }
		}
	  //////////////////////////////////
	  }
      if (strstr(pSmtpContext->mToken,"\n"))
		pSmtpContext->mToken[0] = '\x0';
	  return (-1);
    } if (pSmtpContext->State == SmtpData) {
	  ///////////////////////////////////////
	  // データエンド検出 2002.05.16
	  if (pSmtpContext->bSave) {
	    nEnd = strlen(pSmtpContext->mEnd);
	    strcpy(mEnd, pSmtpContext->mEnd);
	    strncat(mEnd, Request, 5-nEnd);
	    mEnd[5] = '\x0';
		if (strcmp(mEnd, "\r\n.\r\n") == 0) {
		  pSmtpContext->bSave = FALSE;
		  RequestLen = 5 - nEnd;
		} else {
		  *(Request+RequestLen) = '\x0';
		  pEnd = strstr(Request, "\r\n.\r\n");
		  if (pEnd) {
            RequestLen = (pEnd+5) - Request;
		    pSmtpContext->bSave = FALSE;
		  }
		}
	    pSmtpContext->i = -1;
#ifndef TEST_MODE
#ifndef  DATA_CASH
#ifdef UPDATE_20060110 // 作成ファイルがクローズしてもクローズされないのでフラッシュしてみる
	    pSmtpContext->Crackfp = fopen(pSmtpContext->mFnData, "a+b");
#else
	    pSmtpContext->Crackfp = fopen(pSmtpContext->mFnData, "ab");
#endif
#endif
	    if (pSmtpContext->Crackfp) {
#if TRACE_MODE
          if (pSmtpContext)
            if (bDebug) printf("(%08x):Data Write [%d][%s]\n", pSmtpContext, RequestLen, Request);
#endif
#ifdef UPDATE_20060615 // データのディスク書込に異常が有った場合の対策
		  if (fwrite( Request, sizeof(char), RequestLen, pSmtpContext->Crackfp) < RequestLen)
            pSmtpContext->bDiskStatus = FALSE; // 異常
#else
		  fwrite( Request, sizeof(char), RequestLen, pSmtpContext->Crackfp);
#endif

#ifndef  DATA_CASH
#ifdef UPDATE_20060615 // データのディスク書込に異常が有った場合の対策
		  if (fflush(pSmtpContext->Crackfp) == EOF)
            pSmtpContext->bDiskStatus = FALSE; // 異常
#else
		   fflush(pSmtpContext->Crackfp);
#endif
		   fclose(pSmtpContext->Crackfp);
#endif
		} else {
#ifdef UPDATE_20060615 // データのディスク書込に異常が有った場合の対策
          pSmtpContext->bDiskStatus = FALSE; // 異常
#endif
#if TRACE_MODE
          if (pSmtpContext)
            if (bDebug) printf("(%08x):Data File don't open [%s]\n", pSmtpContext, pSmtpContext->mFnData);
#endif
		  return (-1);
		}
#endif
        pSmtpContext->nTotalData += RequestLen;
#ifdef TRACE
  fTrace = fopen(mTraceFile,"at");
  if (fTrace) {
#ifdef K_SEARCH // K_SEARCH OEM 版
     fprintf(fTrace, "<%s>, nTotalData=%lu, RequestLen=%lu, strlen(pSmtpContext->mEnd)=%lu\n",
	 			     pSmtpContext->mMsgId,
				     pSmtpContext->nTotalData,
				     RequestLen, 
				     strlen(pSmtpContext->mEnd));
#else
#ifdef UPDATE_20071204  // メッセージＩＤ採番処理を修正(Bym10id)
     fprintf(fTrace, "<%s>, nTotalData=%lu, RequestLen=%lu, strlen(pSmtpContext->mEnd)=%lu\n",
	 			     pSmtpContext->mMsgId,
				     pSmtpContext->nTotalData,
				     RequestLen, 
				     strlen(pSmtpContext->mEnd));
#else
     fprintf(fTrace, "<B%010lu>, nTotalData=%lu, RequestLen=%lu, strlen(pSmtpContext->mEnd)=%lu\n",
	 			     pSmtpContext->nMsgId,
				     pSmtpContext->nTotalData,
				     RequestLen, 
				     strlen(pSmtpContext->mEnd));
#endif
#endif // K_SEARCH OEM 版 //KTEC SMTP Archive Receiver
     fclose(fTrace);
  }
#endif
	  }

	  if (pSmtpContext->nTotalData <= 5) {
        pSmtpContext->mEnd[pSmtpContext->nTotalData] = '\x0';
	    strncat(pSmtpContext->mEnd, Request, RequestLen);
		switch (pSmtpContext->nTotalData) {
		   case 3: if (strcmp(pSmtpContext->mEnd, ".\r\n") == 0)
#ifdef ESMTP_AUTH
                     pSmtpContext->i = 7;
#else
                     pSmtpContext->i = 4;
#endif
				   break;
		   case 5: if (strcmp(pSmtpContext->mEnd, "\r\n.\r\n") == 0)
#ifdef ESMTP_AUTH
                     pSmtpContext->i = 7;
#else
                     pSmtpContext->i = 4;
#endif
		           break;
		}
      } else {
        pSmtpContext->m = (RequestLen < 5 ? RequestLen : 5) - (5 - strlen(pSmtpContext->mEnd));
		if (pSmtpContext->m > 0) {
	      for (pSmtpContext->n = 0; pSmtpContext->n < (5-pSmtpContext->m); pSmtpContext->n++) {
	        pSmtpContext->mEnd[pSmtpContext->n] = pSmtpContext->mEnd[pSmtpContext->n+pSmtpContext->m];
		  }
          pSmtpContext->mEnd[pSmtpContext->n] = '\x0';
		}
		strncat(pSmtpContext->mEnd, &Request[(RequestLen < 5 ? 0: RequestLen-5)], (RequestLen < 5 ? RequestLen : 5));
        if (strcmp(pSmtpContext->mEnd, "\r\n.\r\n") == 0)
#ifdef ESMTP_AUTH
          pSmtpContext->i = 7;
#else
          pSmtpContext->i = 4;
#endif
#ifdef TRACE
  fTrace = fopen(mTraceFile,"at");
  if (fTrace) {
#ifdef K_SEARCH // K_SEARCH OEM 版
     fprintf(fTrace, "<%s>, m=%lu, n=%lu, mEnd[0]=%02x, mEnd[1]=%02x, mEnd[2]=%02x, mEnd[3]=%02x, mEnd[4]=%02x\n",
				     pSmtpContext->mMsgId,
				     pSmtpContext->m,
				     pSmtpContext->n,
				     pSmtpContext->mEnd[0],
				     pSmtpContext->mEnd[1],
				     pSmtpContext->mEnd[2],
				     pSmtpContext->mEnd[3],
				     pSmtpContext->mEnd[4]);
#else
#ifdef UPDATE_20071204  // メッセージＩＤ採番処理を修正(Bym10id)
     fprintf(fTrace, "<%s>, m=%lu, n=%lu, mEnd[0]=%02x, mEnd[1]=%02x, mEnd[2]=%02x, mEnd[3]=%02x, mEnd[4]=%02x\n",
				     pSmtpContext->mMsgId,
				     pSmtpContext->m,
				     pSmtpContext->n,
				     pSmtpContext->mEnd[0],
				     pSmtpContext->mEnd[1],
				     pSmtpContext->mEnd[2],
				     pSmtpContext->mEnd[3],
				     pSmtpContext->mEnd[4]);
#else
     fprintf(fTrace, "<B%010lu>, m=%lu, n=%lu, mEnd[0]=%02x, mEnd[1]=%02x, mEnd[2]=%02x, mEnd[3]=%02x, mEnd[4]=%02x\n",
				     pSmtpContext->nMsgId,
				     pSmtpContext->m,
				     pSmtpContext->n,
				     pSmtpContext->mEnd[0],
				     pSmtpContext->mEnd[1],
				     pSmtpContext->mEnd[2],
				     pSmtpContext->mEnd[3],
				     pSmtpContext->mEnd[4]);
#endif
#endif // K_SEARCH OEM 版 //KTEC SMTP Archive Receiver
    fclose(fTrace);
  }
#endif
	  }	
#if TRACE_MODE
      if (pSmtpContext)
	    if (bDebug) printf("(%08x):mEnd size(%d):return %d\n",pSmtpContext, strlen(pSmtpContext->mEnd),pSmtpContext->i);
#endif
      return(pSmtpContext->i);
    } else {
      //if (strlen(Request) == 0)
		//return (-1);
      //printf("RequestLen = %d / strlen(pSmtpContext->mToken)+ RequestLen = %d\n",RequestLen, strlen(pSmtpContext->mToken)+ RequestLen);
	  if (strlen(pSmtpContext->mToken)+ RequestLen < sizeof(pSmtpContext->mToken)) {
        strncat(pSmtpContext->mToken, Request, RequestLen );
        if (strstr(pSmtpContext->mToken,"\n") == NULL) {
//printf("%s\n", pSmtpContext->mToken);
		   return(-1);
		}
//printf("found <LF>\n");
	  } else {
		if (bDebug) printf("Token overflows.\n");
		pSmtpContext->mToken[0] = '\x0';
 	    return(-1);
	  }
      for (pSmtpContext->i = 0; pSmtpContext->i < MAX_SMTP_COMMAND ; pSmtpContext->i++ ){
#ifdef ESMTP_AUTH
		if (_strnicmp((PCHAR) (pSmtpContext->State == SmtpAuthentication ? SMTP_PASS : pSmtpContext->mToken), SmtpCommands[pSmtpContext->i], 4) == 0)
#else
		if (_strnicmp((PCHAR) pSmtpContext->mToken, SmtpCommands[pSmtpContext->i], 4) == 0)
#endif
		{
		  if (strstr(pSmtpContext->mToken,"\n") ){
#if TRACE_MODE
            if (pSmtpContext)
	          if (bDebug) printf("[%s]\n",pSmtpContext->mToken);
#endif
			/*
			if () {
			  pContext->State = SmtpDataError;
              sprintf(pContext->mMessages, SMTP_NEED_COMMAND, "RCPT (recipient)");
			}
			*/
            return(pSmtpContext->i);
		  } else {
            return(-1);
		  }
		}
	  //////////////////////////////////
	  }
	}
    if (bDebug) printf("BAD Token Message\n");
	//if (pSmtpContext->State == SmtpDataError)
      //return(-1);
    return(MAX_SMTP_COMMAND); // BAD Token Message
}

#ifndef BTHREAD

SMTPRSDisposition SMTPRSDispatch(
    PVOID       pContext,
    PUCHAR      InputBuffer,
    DWORD       InputBufferLen,
    PHANDLE     SendHandle,
    PUCHAR *    OutputBuffer,
    PDWORD      OutputBufferLen
    )
{
    PSmtpContext pSmtpContext;
    //DWORD   i;

    pSmtpContext = (PSmtpContext) pContext;
    // Null terminate, so string functions won't get confused later.
    //InputBuffer[InputBufferLen] = L'\0';

    // Figure the command out
    pSmtpContext->Command = SMTPRSCrackCommand(pContext,InputBuffer, InputBufferLen);
    // If it didn't parse, throw it away
    if (pSmtpContext->Command == -1)
      return(SMTPRS_Discard);
    // Let the command handlers do their thing
    return(SMTPRSDispatchTable[pSmtpContext->Command](pSmtpContext,
		                                (pSmtpContext->State != SmtpData ? pSmtpContext->mToken : InputBuffer),
                                        (pSmtpContext->State != SmtpData ? (DWORD)strlen(pSmtpContext->mToken) : InputBufferLen),
                                        SendHandle,
                                        OutputBuffer,
                                        OutputBufferLen) );
}

#endif

PVOID CreateSMTPContext(void) {
    PSmtpContext pContext;

    //pContext = NULL;
    pContext = LocalAlloc(LPTR, sizeof(SmtpContext));
    if (pContext) {
 	  if (bDebug) printf("SmtpContext size=%08x\n",LocalSize(pContext));
      pContext->State = SmtpNegotiate;
      pContext->mToken[0] = '\x0';
        /*
		ReportServiceEvent(
            EVENTLOG_INFORMATION_TYPE,
            SMTPRSEVENT_USER_CONNECT,
            0, NULL, 0);
	    */
	}
    return(pContext);
}

#ifdef BTHREAD

BOOL GetData(PCLIENT_CONTEXT lpClientContext) {
  DWORD         l;
  int           err;
  PSmtpContext  pContext = &lpClientContext->Context;
  char          mEnd[6], mec[128];

//#ifdef UPADTE_20040202
   int    ns;
   fd_set ds;
   struct timeval tmo;
//#endif

#ifdef  DATA_CASH
#ifdef UPDATE_20060110 // 作成ファイルがクローズしてもクローズされないのでフラッシュしてみる
    pContext->Crackfp = fopen(pContext->mFnData, "a+b");
#else
    pContext->Crackfp = fopen(pContext->mFnData, "ab");
#endif
	setvbuf( pContext->Crackfp, pContext->mFwbuf, _IOFBF, sizeof(pContext->mFwbuf) );
#endif
    pContext->nTotalData = 0;
    do {
#ifdef UPDATE_20090225 // データ受信中プログラムのCPU使用率を下げる対策
        _sleep(WAIT_TIME); // 20090225
#endif
  	  mEnd[0] = '\x0';
#ifndef TUNING
      memset(lpClientContext->Buffer, 0, sizeof(lpClientContext->Buffer));
#else
      lpClientContext->Buffer[0] = '\x0';
#endif
	  //// 届いた分だけデータ読込み
#ifdef UPDATE_20050518
	  if (nTMOut) { // 0 ならタイムアウトしない。
        tmo.tv_sec = (int)nTMOut; // 1秒単位
 	    tmo.tv_usec = 0;            // microseconds 
	    FD_ZERO(&ds);
	    FD_SET(lpClientContext->Socket, &ds);
#ifdef UPDATE_20060324 // Thunderbirdで添付を含むSSL送信で問題をおこす。
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
			// SSLでは、select処理させない
            l = SSL_read(lpClientContext->ssl, lpClientContext->Buffer, 2048); // opensslサイズの限界のため sizeof(lpClientContext->Buffer)-1-n);
		    if (l == -1)
		      l = 0;
		    lpClientContext->Buffer[l] = '\x0';
		  } else {
#endif
	        if ((ns = select(lpClientContext->Socket, &ds, (fd_set *)NULL, (fd_set *)NULL, &tmo)) > 0) {
              l = recv(lpClientContext->Socket, lpClientContext->Buffer, sizeof(lpClientContext->Buffer), 0);
			} else {
              l = SOCKET_ERROR;
			}
#ifdef USE_SSL
		  }
#endif
#else
	    if ((ns = select(lpClientContext->Socket, &ds, (fd_set *)NULL, (fd_set *)NULL, &tmo)) > 0) {
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
            l = SSL_read(lpClientContext->ssl, lpClientContext->Buffer, 2048); // opensslサイズの限界のため sizeof(lpClientContext->Buffer)-1-n);
		    if (l == -1)
		      l = 0;
		    lpClientContext->Buffer[l] = '\x0';
		  } else
#endif
            l = recv(lpClientContext->Socket, lpClientContext->Buffer, sizeof(lpClientContext->Buffer), 0);
		} else {
          l = SOCKET_ERROR;
		}
#endif
	  } else {
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
          l = SSL_read(lpClientContext->ssl, lpClientContext->Buffer, 2048); // opensslサイズの限界のため sizeof(lpClientContext->Buffer)-1-n);
		  if (l == -1)
		    l = 0;
		 lpClientContext->Buffer[l] = '\x0';
	  } else
#endif
        l = recv(lpClientContext->Socket, lpClientContext->Buffer, sizeof(lpClientContext->Buffer), 0);
	  }
#else
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
        l = SSL_read(lpClientContext->ssl, lpClientContext->Buffer, 2048); // opensslサイズの限界のため sizeof(lpClientContext->Buffer)-1-n);
		if (l == -1)
		  l = 0;
		lpClientContext->Buffer[l] = '\x0';
	  } else
#endif
#ifdef UPDATE_20040202
 		if (nTMOut) { // 0 ならタイムアウトしない。
          tmo.tv_sec = (int)nTMOut; // 1秒単位
 	      tmo.tv_usec = 0;            // microseconds 
	      FD_ZERO(&ds);
	      FD_SET(lpClientContext->Socket, &ds);
	      if ((ns = select(lpClientContext->Socket, &ds, (fd_set *)NULL, (fd_set *)NULL, &tmo)) > 0) {
            l = recv(lpClientContext->Socket, lpClientContext->Buffer, sizeof(lpClientContext->Buffer), 0);
		  } else {
            l = SOCKET_ERROR;
		  }
		} else
#endif
          l = recv(lpClientContext->Socket, lpClientContext->Buffer, sizeof(lpClientContext->Buffer), 0);
#endif
	  if (l == 0) { // 0 = gracefully closed. // 丁寧な終了なら受信終了
	    break;
	  } else if (l == SOCKET_ERROR || 
	      strlen(lpClientContext->Buffer) == 0) {
	    err = WSAGetLastError();
	    switch (err) {
	      case WSANOTINITIALISED: sprintf(mec, "read() SOCKET_ERROR WSANOTINITIALISED=%d", err); break;
	  	  case WSAENETDOWN: sprintf(mec, "read() SOCKET_ERROR WSAENETDOWN=%d", err); break;
		  case WSAEFAULT: sprintf(mec, "read() SOCKET_ERROR WSAEFAULT=%d", err); break;
		  case WSAEINPROGRESS: sprintf(mec, "read() SOCKET_ERROR WSAEINPROGRESS=%d", err); break;
		  case WSAEINVAL: sprintf(mec, "read() SOCKET_ERROR WSAEINVAL=%d", err); break;
		  case WSAEMFILE: sprintf(mec, "read() SOCKET_ERROR WSAEMFILE=%d", err); break;
		  case WSAENOBUFS: sprintf(mec, "read() SOCKET_ERROR WSAENOBUFS=%d", err); break;
		  case WSAENOTSOCK: sprintf(mec, "read() SOCKET_ERROR WSAENOTSOCK=%d", err); break;
		  case WSAEOPNOTSUPP: sprintf(mec, "read() SOCKET_ERROR WSAEOPNOTSUPP=%d", err); break;
		  case WSAEWOULDBLOCK: sprintf(mec, "read() SOCKET_ERROR WSAEWOULDBLOCK=%d", err); break;
		  default:  sprintf(mec, "read() SOCKET_ERROR OTHER CODE=%d", err); break;
		}
        if (bDebug) printf("[%08x] RECV[%08x] ERROR<- [%s]\n", lpClientContext, lpClientContext->Socket, mec);
	    if (bDebug) printf("[%08x] %s\n",lpClientContext, mec);
#ifdef ACCEPT_LOG_LEVEL3
	    LEVEL_3_ACCEPTLOG(lpClientContext, mec);
#endif
          break;
	  } else if (l > 0) { // データがあるなら
        SMTPRSCrackCommand(pContext, lpClientContext->Buffer, l);
	  }
	  //_sleep(0); // 他スレッドに切替

	} while ( strcmp(pContext->mEnd, "\r\n.\r\n") && 
		      strcmp(pContext->mEnd, ".\r\n") ); // データエンドになるまで
#ifdef  DATA_CASH
	//fflush(pSmtpContext->Crackfp);
	fclose(pContext->Crackfp);
#endif
#if TRACE_MODE
    if (pContext)
       if (bDebug) printf("(%08x):mEnd size(%d):return %d\n",pContext, strlen(pContext->mEnd),pSmtpContext->i);
#endif
	if (!strcmp(pContext->mEnd, "\r\n.\r\n") ||
		!strcmp(pContext->mEnd, ".\r\n"))
      return TRUE;
	else
      return FALSE;
}

BOOL Smtp_Dispatch(PCLIENT_CONTEXT lpClientContext) {
  BOOL sts;
  PSmtpContext    pContext = &lpClientContext->Context;

  //do {
    pContext->pToken = NULL;
    pContext->pCmd = lpClientContext->Buffer;
    strncpy(pContext->mToken, lpClientContext->Buffer, sizeof(pContext->mToken)-1);
	pContext->mToken[sizeof(pContext->mToken)-1] = '\x0';
   	strtok(pContext->pCmd," \r\n");
	//strtok(pContext->mToken,"\r\n");
    if (pContext->pCmd) {
#ifdef USE_STARTTLS
#ifdef UPDATE_20151126 // リッスンＩＰ毎に証明書を選択できる対策
      if ((bUsedSTLS || pContext->bUsedSTLS) && stricmp(pContext->pCmd, SMTP_STLS) == 0) 
#else
      if (bUsedSTLS && stricmp(pContext->pCmd, SMTP_STLS) == 0) 
#endif
	  {
        sts = StlsDispatch(lpClientContext);
	  } else
#endif
      if (stricmp(pContext->pCmd, SMTP_HELO) ==0) {
        sts = HeloDispatch(lpClientContext);
#ifdef ESMTP_AUTH
	  } else if (stricmp(pContext->pCmd, SMTP_EHLO) == 0 ) {
        sts = EhloDispatch(lpClientContext);
	  } else if (stricmp(pContext->pCmd, SMTP_AUTH) == 0 ) {
        sts = AuthDispatch(lpClientContext);
	  } else if (stricmp(pContext->pCmd, SMTP_PASS) == 0 ||
		         pContext->State == SmtpAuthentication) {
        sts = PassDispatch(lpClientContext);
#endif
	  } else if (stricmp(pContext->pCmd, SMTP_MAIL) == 0) {
        sts = MailDispatch(lpClientContext);
	  } else if (stricmp(pContext->pCmd, SMTP_RCPT) == 0) {
        sts = RcptDispatch(lpClientContext);
	  } else if (stricmp(pContext->pCmd, SMTP_DATA) == 0) {
		sts = TRUE;
        if (DataDispatch(lpClientContext))   // DATA命令
		  if ((sts = GetData(lpClientContext))) {      // メッセージ取得
            if (DotDispatch(lpClientContext)) // 取得後処理
			   SendPipeLine("Quit");                // spadsへ通知
#ifdef UPDATE_20050921 // DATA受信中に強制切断されると　354 Data を再度送出してしまう不具合
		  } else {
			return sts;
#endif
		  }
        //printf("End DotDispatch");
	  } else if (stricmp(pContext->pCmd, SMTP_RSET) == 0) {
        sts = RsetDispatch(lpClientContext);
	  } else if (stricmp(pContext->pCmd, SMTP_VRFY) == 0) {
        sts = VrfyDispatch(lpClientContext);
	  } else if (stricmp(pContext->pCmd, SMTP_EXPN) == 0) {
        sts = ExpnDispatch(lpClientContext);
	  } else if (stricmp(pContext->pCmd, SMTP_HELP) == 0) {
        sts = HelpDispatch(lpClientContext);
	  } else if (stricmp(pContext->pCmd, SMTP_NOOP) == 0) {
        sts = NoopDispatch(lpClientContext);
	  } else if (stricmp(pContext->pCmd, SMTP_QUIT) == 0) {
        QuitDispatch(lpClientContext);
		sts = FALSE; // 終了
	  } else { // 間違った命令
        sts = BadDispatch(lpClientContext);
	  }
	} else { // 命令がNULL
      sts = BadDispatch(lpClientContext);
	}
#ifdef UPDATE_20041225_SPEEDUP
	if (!pContext->bAcptData)
#endif
    put_reply(lpClientContext, TRUE);
#ifdef USE_STARTTLS
		  // STARTTLSでSSLモードに切替
          if (!stricmp(pContext->pCmd, SMTP_STLS) && 
			  !strcmp(pContext->mMessages, SMTP_GOOD_STLS) &&
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
#ifdef UPDATE_20171211A // 使用可能な暗号化通信方式のバージョンを規定するオプション 0x1:SSL2禁止 0x2:SSL3禁止 0x4:TSL1禁止 0x8:TSL1.1禁止 0x10:TSL1.2禁止
              SSL_CTX_set_options(lpClientContext->ctx, (ULONGLONG)nSecuerLayOption);
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
#ifndef UPDATE_20170619 // 中間証明書の繋がりが検出できない不具合
	              SSL_CTX_use_certificate_chain_file(lpClientContext->ctx, pContext->mCertificate); // // SSL用中間証明書
#endif
				} else
#endif
				{
                  SSL_use_RSAPrivateKey_file(lpClientContext->ssl, mPrivatekey, SSL_FILETYPE_PEM); // 個人鍵を指定
                  SSL_use_certificate_file(lpClientContext->ssl, mCertificate, SSL_FILETYPE_PEM);   // 証明書を指定
#ifndef UPDATE_20170619 // 中間証明書の繋がりが検出できない不具合
	              SSL_CTX_use_certificate_chain_file(lpClientContext->ctx, mCertificate); // // SSL用中間証明書
#endif
				}
                if (SSL_accept(lpClientContext->ssl) == 1)
				{
				   pContext->bUsedSSL = TRUE;
//  printf("%s %s\n", SSL_get_version(lpClientContext->ssl), SSL_get_cipher(lpClientContext->ssl));
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
#endif  //USE_STARTTLS end.

  //} while(*lpClientContext->Queue);

  return sts;
}
#endif

BOOL SMTPRSMain(void) {
   hCompletionPort = InitializeThreads(); // Initialize the SMTPRSRV worker threads.
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
   if (bDebug) printf(SMTP_DEBUG_MESS, VERSION, mmode, mtime);
   if (bDebug) printf("\n");
}

void RecoverFolder(char *mExtend) {
  char mFileGroup[256], mLKFile[256];
  long               hFindFile;
  struct _finddata_t FindFileData ;
  BOOL bFile = TRUE;

#ifndef TUNING1
  sprintf(mFileGroup, "%s*.%s", mTempDir, mExtend); // RST,RCP,MSG
#else
  sprintf(mFileGroup, "%s%s*.%s",mMailSpoolDir,mMailQueueDir, mExtend); // RST,RC$,MS$
#endif

//  sprintf(mFileGroup, "%s\\%s\\*", mMailSpoolDir, mDir);
  hFindFile = _findfirst( mFileGroup, &FindFileData); //hFindFile = FindFirstFile( mFileGroup, &FindFileData);
  if (hFindFile != -1L) { //INVALID_HANDLE_VALUE) {
    while (bFile) {
	  if (!(strcmp(FindFileData.name,".") == 0 ||
		  strcmp(FindFileData.name,"..") == 0 ) ) {
#ifndef TUNING1
        sprintf(mLKFile, "%s%s", mTempDir, FindFileData.name); // RST,RCP,MSG
#else
        sprintf(mLKFile, "%s%s%s",mMailSpoolDir,mMailQueueDir, FindFileData.name); // RST,RC$,MS$
#endif
        _unlink(mLKFile); //DeleteFile(mLKFile);
	  }
      bFile = (_findnext(hFindFile, &FindFileData) == 0 ? TRUE : FALSE); //bFile = FindNextFile( hFindFile, &FindFileData);
	}; 
    _findclose(hFindFile); //FindClose( hFindFile ); 
  };
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
    LPTSTR                  lpszPipeName = TEXT("\\\\.\\pipe\\" TRADEMARK "SMTPRS");
    DWORD                   dwWait, dwc;
    //UINT                    ndx;
    int status, nl;             /* Status Code */
    WSADATA WSAData;
    char *tmp, *p, *pldom;
#ifdef Y2038_BUG
  ULARGE_INTEGER u1;
#endif
    char  mLicencekey[65];
#ifdef CLUSTERING
	//FILE *fp;
    HANDLE   hFile;
	DWORD    nComName;
#endif
#ifdef  INCOM_SUB_FOLDER // incoming フォルダをスレッド別フォルダにして処理
	DWORD    nNum;
	CHAR     mIncomingThread[_MAX_PATH+1];
#endif
	//char mver[128];
	//char dummy[] = "%SystemDrive%\\TEMP";
//BOOL btst=FALSE;
//char fullname[256];
// スレッドあたりのメモリ使用数
//printf("%lu\n", sizeof(SMTPRState));
//printf("%lu\n", sizeof(SmtpMessageHeader));
//printf("%lu\n", sizeof(SmtpMailDirectory));
//printf("%lu\n", sizeof(SmtpContext));
//printf("%lu\n", sizeof(CLIENT_CONTEXT));
	//////////////////////////////////////////////////////////////
#ifdef UPDATE_20080929A // ログのクリティカルセクション化
    //////////////////////////////////////////////
    // ログ出力の排他フラグ初期化
    InitializeCriticalSection(&g_csWriteReport);
#endif
#ifdef UPDATE_20091120 // ML保存時のindex.html作成のための排他処理、64bit長のファイルサイズに対応
    InitializeCriticalSection(&g_csMLIndex);
#endif

#ifdef UPDATE_20070405 // イベントログデータベースを追加。
    InitEventLog();
#endif
    // initalaize //////////////////
#ifdef V4
    _setmaxstdio(2048);
#endif
	/*
    char mD5[256];
	hmac_md5("<000041.980207109@ns1.kawa.or.jp>","ACCESS30", mD5);
	hmac_md5("<1896.697170952@postoffice.reston.mci.net>","tanstaaftanstaaf", mD5);
	*/
    _tzset();
	//////////////////////////////////////////////////////////////
#ifdef UPDATE_20181025 // 外部DLLの呼出しポインタをグローバル化
    hDLL = NULL;
#endif
	nAcceptCount = 0;
	bUsedSaveList = FALSE;
//	bTMQWait = FALSE;
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
#ifdef UPDATE_20070516 // 上長承認機能の追加
    bMailApproval = GetProfileIntEx(SOFT_REG, "MailApproval", (int) FALSE);    // 上長承認機能の有効有無
#endif
#ifdef UPDATE_20190108 // 承認メールをHTMLメールにする機能を追加
	bMailApprovalHtml = GetProfileIntEx(SOFT_REG, "MailApprovalHtml", (int) FALSE);    // 承認メールをHTMLメールに
#endif
#ifdef UPDATE_20161111 /// 外部からの送信元送信先制限(逆上長承認)
    bMailApprovalRevers = GetProfileIntEx(SOFT_REG, "MailApprovalRevers", (int) FALSE);    // 上長承認機能の有効有無
	bMailApprovalWebOnly = GetProfileIntEx(SOFT_REG, "MailApprovalWebOnly", (int) FALSE);   // Web管理のみでは承認依頼メールを作らない
#endif
#ifdef UPDATE_20170930 // 逆上長承認で承認者への送信元エンベロープが不正アドレス等で送信先に受信拒絶されないようにする対策
    nMailApprovalReversType = GetProfileIntEx(SOFT_REG, "MailApprovalReversType", (int) 0); // 送信エラーの返信先 0:送信しない 1:送信元 2:送信先指定
    GetProfileStringEx(SOFT_REG, "MailApprovalReversFrom", "", mMailApprovalReversFrom, sizeof(mMailApprovalReversFrom)); //送信先指定
#endif
#ifdef LDAP_ON         // LDAPへのログイン処理でSMTP認証の確認を行うオプション // //CRAM-MD5は複合できないので、これを利用する場合は、PLAINかLOGINのみとします。
    bLDAPOn = GetProfileIntEx(SOFT_REG, "LDAP", (int) FALSE);    // LDAP対応
#endif
#ifdef LGWAN_ON        // LGWAN機能拡張
	bLGWAN = GetProfileIntEx(SOFT_REG, "LGWAN", (int) FALSE);    // LGWAN拡張
	bCHGHEADER = GetProfileIntEx(SOFT_REG, "LGWANCHGHEADER", (int) TRUE);    // LGWAN拡張
#endif
#ifdef MILTER_ON // MILTERインターフェースを追加。
	bMILTER = GetProfileIntEx(SOFT_REG, "MILTER", (int) FALSE);    // MILTER拡張
	bMMACRO = GetProfileIntEx(SOFT_REG, "MILTERMACRO", (int) TRUE);    // MILTER MACRO追加
    GetProfileStringEx(SOFT_REG,"MILTERMACROSCONNECT", "j, {client_addr}", mMMCONNECT, sizeof(mMMCONNECT));
    GetProfileStringEx(SOFT_REG,"MILTERMACROSHELO", "j, {client_addr}", mMMHELO, sizeof(mMMHELO));
    GetProfileStringEx(SOFT_REG,"MILTERMACROSENVFROM", "{mail_host}, {mail_addr}, {client_addr}", mMMENVFROM, sizeof(mMMENVFROM));
    GetProfileStringEx(SOFT_REG,"MILTERMACROSENVRCPT", "{rcpt_host}, {rcpt_addr}, {client_addr}", mMMENVRCPT, sizeof(mMMENVRCPT));
    GetProfileStringEx(SOFT_REG,"MILTERMACROSEOM", "i, {client_addr}", mMMEOM, sizeof(mMMEOM));
#endif
#ifdef UPDATE_20150212 // 接続元のアドレスによって受信メールを破棄するオプション
    bDISCARDMAIL = GetProfileIntEx(SOFT_REG, "DISCARDMAIL", (int) FALSE);    // 受信メール破棄オプション
#endif
#ifdef UPDATE_20220427 // リッスン側のIPアドレスのFQDNをホスト名として優先する。
    bListenAddrPRI= GetProfileIntEx(SOFT_REG, "LISTENADDRPRI", (int) TRUE);    // 1:優先する 0:ローカルホスト名
#endif
#ifdef UPDATE_20240205 // 承認依頼メールで１行当たりの文字数を正規化
    nNomalColumnMaxSize = (bMailApprovalHtml ? GetProfileIntEx(SOFT_REG, "MailApprovalHtmlColumn", (int) 78) : 0); //デフォルト 78byte/Line
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
	nPipeCnt = 0;
#endif
#ifdef UPDATE_20070407 // リンク先のドメイン名の調査する深さを調整可能に
	nURIRBLDepath = GetProfileIntEx(SOFT_REG , "URIRBLDepath", (int)2); // URIBLのドメインの深さ制限　デフォルト 2;
#endif
	nMailFiterUp = GetProfileIntEx(SOFT_REG , "MailFilterUp", (int)0); // メールフィルタ機能強化 0:標準 1:強化
#ifdef K_SEARCH // K_SEARCH OEM 版
	nQueueUnit = GetProfileIntEx(SOFT_REG , "QueueUnit", (int)0); // メールの保管単位 0:日単位 1:月単位 2:年単位
#endif // K_SEARCH OEM 版 //KTEC SMTP Archive Receiver

#ifdef UPDATE_20060615 // データのディスク書込に異常が有った場合の対策
	bVLog = GetProfileIntEx(SOFT_REG , "VLog", (int)0); // イベントビューワにログ書込みエラーを表示する 0:しない　1:する
#endif
#ifdef UPDATE_20171211A // 使用可能な暗号化通信方式のバージョンを規定するオプション 0x1:SSL2禁止 0x2:SSL3禁止 0x4:TSL1禁止 0x8:TSL1.1禁止 0x10:TSL1.2禁止
    nSSLOpt = GetProfileIntEx(SYSTEM_REG , "SecureLayOption", (int)3); // 
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

#ifdef UPDATE_20051121 // メール連続受領に対する配送キュー調整
	nSpoolFsSync = GetProfileIntEx(SYSTEM_REG , "SpoolFsSync", (int)0); //2006.09.01 変更 1000 -> 0
#endif
#ifdef UPDATE_20241226 // AUTH LOGINでの応答時チャレンジ文字列の設定をレジストリから変更できるようにした。
    GetProfileStringEx(SYSTEM_REG, "ChallengeTokenLOGINUser", "Username:", mChallengeTokenLOGINUser, sizeof(mChallengeTokenLOGINUser));
    GetProfileStringEx(SYSTEM_REG, "ChallengeTokenLOGINPass", "Password:", mChallengeTokenLOGINPass, sizeof(mChallengeTokenLOGINPass));
#endif
	///// サードパーティーモジュール情報
#ifdef THIRDPARTY
    GetProfileStringEx(SYSTEM_REG, THIRDPARTYMODULE, "", mThirdparty, sizeof(mThirdparty));
#ifdef UPDATE_20240523 // ウイルスチェック時のログ保存先が自動生成されない不具合
    b3rdViruslog = GetProfileIntEx(SYSTEM_REG , "3rdViruslog", (INT)0);
#endif
#endif
	///// ライセンスキー情報
#ifdef K_SEARCH // K_SEARCH OEM 版 //KTEC SMTP Archive Receiver
    GetProfileStringEx(SOFT_REG, LIMITKEY, "", mLicencekey, sizeof(mLicencekey));
#else
    GetProfileStringEx(SYSTEM_REG, LIMITKEY, "", mLicencekey, sizeof(mLicencekey));
#endif // K_SEARCH OEM 版 //KTEC SMTP Archive Receiver
    //////////////////////
	//ltbegin = GetProfileIntEx("SYSTEM\\CurrentControlSet\\Services\\SPARS", "Begin", (int)0);
#ifdef K_SEARCH // K_SEARCH OEM 版 //KTEC SMTP Archive Receiver
	u1.LowPart = GetProfileIntEx(SOFT_REG , BEGINLOW, (int)0);
	u1.HighPart = GetProfileIntEx(SOFT_REG , BEGINHIGH, (int)0);
	ltbegin = u1.QuadPart;
#else
#ifdef Y2038_BUG
	u1.LowPart = GetProfileIntEx(SYSTEM_REG , BEGINLOW, (int)0);
	u1.HighPart = GetProfileIntEx(SYSTEM_REG , BEGINHIGH, (int)0);
	ltbegin = u1.QuadPart;
#else
	ltbegin = GetProfileIntEx(SYSTEM_REG , "Begin", (int)0);
#endif
#endif // K_SEARCH OEM 版 //KTEC SMTP Archive Receiver
    ReSetTimeLimit(&ltbegin);
	nPatternOfTheOnce = GetProfileIntEx(SYSTEM_REG, "PatternOfTheOnce", (int)500); // デフォルト 500パターンまでメモリに一括処理させる。
	nPriority = GetProfileIntEx(SYSTEM_REG, "Priority", (int)0); // -2:LOW-, -1:LOW, 0:NORMAL, 1:HIGH, 2:HIGH+
#ifdef K_SEARCH // K_SEARCH OEM 版 //KTEC SMTP Archive Receiver
	nTMOut = GetProfileIntEx(SOFT_REG, "Timeout", (int)300); // 60秒×5 = 5分
#else
	//nTMOut = GetProfileIntEx("SYSTEM\\CurrentControlSet\\Services\\SPARS", "Timeout", (int)1200); // 60秒×20 = 20分
#ifdef BTHREAD
	nTMOut = GetProfileIntEx(SYSTEM_REG, "Timeout", (int)300); // 60秒×5 = 5分
#else
	nTMOut = GetProfileIntEx(SYSTEM_REG, "Timeout", (int)1200); // 60秒×20 = 20分
#endif
#endif // K_SEARCH OEM 版 //KTEC SMTP Archive Receiver
	//nTMOut = 60;
    ////////////////////////////////
	bCountLock = FALSE;  // Counter Release
	nport = 25;
	bLog  = FALSE;
#ifdef K_SEARCH // K_SEARCH OEM 版 //KTEC SMTP Archive Receiver
    nport = GetProfileIntEx(SOFT_REG, "PortNo", (int)10025);
	bLog  = GetProfileIntEx(SOFT_REG, "MailInlogEnabled", (int)FALSE);
#else
#ifndef E_POST
    nport = GetProfileIntEx("SYSTEM\\CurrentControlSet\\Services\\SMTPRS\\Parameters", "PortNo", (int)25);
	bLog  = GetProfileIntEx("SYSTEM\\CurrentControlSet\\Services\\SMTPRS\\Parameters", "MailInlogEnabled", (int)FALSE);
#endif
	if (nport == 25)
      nport = GetProfileIntEx(SYSTEM_PARAM_REG, "PortNo", (int)25);
	if (!bLog)
	  bLog  = GetProfileIntEx(SYSTEM_PARAM_REG, "MailInlogEnabled", (int)FALSE);
#endif // K_SEARCH OEM 版 //KTEC SMTP Archive Receiver

#ifdef V3
#ifdef K_SEARCH // K_SEARCH OEM 版 //KTEC SMTP Archive Receiver
	bUserMan = GetProfileIntEx(SOFT_REG, "UserManager", (INT)FALSE);    // TRUE=NT使用,FALSE=SPA使用
#else
	bUserMan = GetProfileIntEx(SOFT_REG, "UserManager", (INT)TRUE);    // TRUE=NT使用,FALSE=SPA使用
#endif // K_SEARCH OEM 版 //KTEC SMTP Archive Receiver
#endif
	bVrfy = GetProfileIntEx(SOFT_REG, "Vrfy", (int)TRUE); // デフォルト VRFY/EXPN可能
    GetProfileStringEx(SOFT_REG, "Membership", "", mAuthDomain, sizeof(mAuthDomain)); // 親子関係　デフォルト NULL=workgroup(ローカルマシン)
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
    GetProfileStringEx(SOFT_REG, "LDAPSchemaFilter", "", mLDAPSchemaFilter, sizeof(mLDAPSchemaFilter)); // LDAP ユーザーID名称
    GetProfileStringEx(SOFT_REG, "LDAPSchemaID", "", mLDAPSchemaID, sizeof(mLDAPSchemaID)); // LDAP ユーザーID名称
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
#ifdef BTHREAD
	nSendTMO = GetProfileIntEx(SYSTEM_REG /*SOFT_REG*/, "SendDataTimeout", (INT)300); //60);     // send()送信タイムアウト時間:default 300秒
#else
	nSendTMO = GetProfileIntEx(SYSTEM_REG /*SOFT_REG*/, "SendDataTimeout", (INT)60); //60);     // send()送信タイムアウト時間:default 60秒
#endif
	nRecvBuf = GetProfileIntEx(SYSTEM_REG, "RecvDataBufferSize", (int)0); // socket 受信バッファサイズ
#ifdef UPDATE_20240219 // 付加表題付きのＭＬでの文字化け対策
    nUTF8ToSJISTBL = GetProfileBinaryEx(SYSTEM_REG,"UTF8ToSJISTBL", "\xE2\x80\x93\xEF\xBC\x8D\x20\xE2\x80\x94\xEF\xBC\x8D\x20\xE2\x88\x92\xEF\xBC\x8D", mUTF8ToSJISTBL, sizeof(mUTF8ToSJISTBL));
#endif
#ifdef UPDATE_20240228 // 付加表題もまとめてパックするオプション
	bMLtknInc = GetProfileIntEx(SYSTEM_REG, "MLTokenInclude", (int)FALSE); //TRUE 1:まとめる FALSE 0:分離したまま(従来)
#endif
	nAddressFamily = GetProfileIntEx(SOFT_REG, "AddressFamily", (INT)0);  //0:AF_INET only 1:AF_INET6 only 2:AF_INET&AF_INET6 
    GetProfileStringEx(SOFT_REG, "HostName", "", mHostName, sizeof(mHostName));
    //nLocalDomain = GetProfileBinaryEx("SOFTWARE\\EMWAC\\IMS","DomainNamesAreLocal", "", mLocalDomain, sizeof(mLocalDomain));
    nLocalDomain = GetProfileBinaryEx(SOFT_REG,"DomainNamesAreLocal", "", mLocalDomain, sizeof(mLocalDomain));
	//nMailInCopy = GetProfileBinaryEx("SOFTWARE\\EMWAC\\IMS","MailInCopy", "", mMailInCopy, sizeof(mMailInCopy));
	nMailInCopy = GetProfileBinaryEx(SOFT_REG,"MailInCopy", "", mMailInCopy, sizeof(mMailInCopy));
	//nMailInMaxSize = GetProfileIntEx("SOFTWARE\\EMWAC\\IMS", "MailInMaxSize", (INT)0);
	nMailInMaxSize = GetProfileIntEx(SOFT_REG, "MailInMaxSize", (INT)0);
#ifdef UPDATE_ULONGLONG //64bit長のデータで取得可能にする
	nMailInBoxSizeHigh = GetProfileIntEx(SOFT_REG, "MailInBoxSizeHigh", (INT)0);
#endif
	//nMailInBoxSize = GetProfileIntEx("SOFTWARE\\EMWAC\\IMS", "MailInBoxSize", (INT)0);
	nMailInBoxSize = GetProfileIntEx(SOFT_REG, "MailInBoxSize", (INT)0);
	nRCPTMaximum = GetProfileIntEx(SOFT_REG, "RCPTMaximum", (INT)0);
    //GetProfileStringEx("SOFTWARE\\EMWAC\\IMS","MailInBoxDir", "", mMailBoxDir, sizeof(mMailBoxDir));
    GetProfileStringEx(SOFT_REG, "MailInBoxDir", MAIL_BOX, mMailBoxDir, sizeof(mMailBoxDir));
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
	//GetProfileStringEx("SYSTEM\\CurrentControlSet\\Services\\SPARS","DefaultCode", "us-ascii", mDefaultCode, sizeof(mDefaultCode));
	GetProfileStringEx(SYSTEM_REG,"DefaultCode", "us-ascii", mDefaultCode, sizeof(mDefaultCode));
	strtok(mDefaultCode," ");
//CheckUser("kawa@kawa.co.jp", &btst, fullname);
//exit(0);
	//nLastMsgId = GetProfileIntEx("SOFTWARE\\EMWAC\\IMS", "LastMsgId", (int)0);
	nLastMsgId = GetProfileIntEx(SOFT_REG, "LastMsgId", (int)0);
	//GetProfileStringEx("SOFTWARE\\EMWAC\\IMS","MailGroup","IMSUsers", mMailGroup, sizeof(mMailGroup));
	GetProfileStringEx(SOFT_REG,"MailGroup","IMSUsers", mMailGroup, sizeof(mMailGroup));
	bServiceTerminating = FALSE;
	///// 受付け制限数
#ifdef K_SEARCH // K_SEARCH OEM 版 //KTEC SMTP Archive Receiver
    nAcceptLimit = GetProfileIntEx(SOFT_REG, "AcceptLimit", (int)0);
#else
    nAcceptLimit = GetProfileIntEx(SYSTEM_REG, "AcceptLimit", (int)0);
#endif // K_SEARCH OEM 版 //KTEC SMTP Archive Receiver
    ///// パスワード設定ファイル
    GetProfileStringEx(SYSTEM_REG,"PasswordFile","apop.dat", mPasswordFile, sizeof(mPasswordFile));
#ifdef V4
	///// グリーティングメッセージの扱い
	bHide = GetProfileIntEx(SYSTEM_REG , "HidesGreeting", (INT)FALSE);   // グリーティングメッセージを隠す。 TRUE:隠す FALSE:隠さない
#ifdef UPDATE_20070510 // 実ユーザ名でのメール送受信を禁止処理する処理
	///// 実ユーザ名でのメール送受信禁止許可フラグ
    GetProfileStringEx(SYSTEM_REG,"UseSMTPFile","$offsmtp", mUseSMTPFile, sizeof(mUseSMTPFile));
#endif
    ///// 有効時間設定ファイル
    GetProfileStringEx(SYSTEM_REG,"UseTimeFile","usetime.dat", mUseTimeFile, sizeof(mUseTimeFile));
#ifdef UPDATE_20050924  // 送信元ユーザ別送信先制限
	GetProfileStringEx(SYSTEM_REG,"SenderPermitFile", "sender.dat", mSenderPermitFile, sizeof(mSenderPermitFile));
#endif
#endif
    ///// リッスンIPアドレス制限用
#ifdef K_SEARCH // K_SEARCH OEM 版 //KTEC SMTP Archive Receiver
	bListenMode = GetProfileIntEx(SOFT_REG, "ListenMode", (int)0);  // 0:全てのIP許可, 1:指定IPのみ
#else
	bListenMode = GetProfileIntEx(SYSTEM_REG, "ListenMode", (int)0);  // 0:全てのIP許可, 1:指定IPのみ
#endif // K_SEARCH OEM 版 //KTEC SMTP Archive Receiver
#ifndef TUNING
	memset(mListenIP, 0, sizeof(mListenIP));
#else
    mListenIP[0] = '\x0';
#endif
#ifdef K_SEARCH // K_SEARCH OEM 版 //KTEC SMTP Archive Receiver
    dwc = GetProfileStringEx(SOFT_REG,"ListenIP","", mListenIP, sizeof(mListenIP));
#else
    dwc = GetProfileStringEx(SYSTEM_REG,"ListenIP","", mListenIP, sizeof(mListenIP));
#endif // K_SEARCH OEM 版 //KTEC SMTP Archive Receiver
#ifndef TUNING
    memset(&mListenIP[dwc], 0, (sizeof(mListenIP)-dwc));
#else
	mListenIP[dwc] = '\x0';
#endif
    memcpy(mListenIP2, mListenIP, sizeof(mListenIP));
	pldom = mListenIP2;
	while (strlen(pldom)){
      pldom += strlen(pldom);
 	  pldom++;
	  if (strlen(pldom))
	  	*(pldom-1) = ' ';
	};
	//////////////////////////////////////////////////////////////
	//bAcceptLog = GetProfileIntEx("SYSTEM\\CurrentControlSet\\Services\\SPARS", "AcceptlogEnabled", (int)FALSE);
#ifdef K_SEARCH // K_SEARCH OEM 版 //KTEC SMTP Archive Receiver
	bAcceptLog = GetProfileIntEx(SOFT_REG, "AcceptlogEnabled", (int)FALSE);
#else
	bAcceptLog = GetProfileIntEx(SYSTEM_REG, "AcceptlogEnabled", (int)FALSE);
#endif // K_SEARCH OEM 版 //KTEC SMTP Archive Receiver
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
#ifdef K_SEARCH // K_SEARCH OEM 版 //KTEC SMTP Archive Receiver
    GetProfileStringEx(SOFT_REG,"MailQueueDir", "incoming", mMailQueueDir, sizeof(mMailQueueDir));
#else
#ifndef E_POST
    GetProfileStringEx("SYSTEM\\CurrentControlSet\\Services\\SMTPRS\\Parameters","MailQueueDir", "incoming", mMailQueueDir, sizeof(mMailQueueDir));
#endif
	if (stricmp(mMailQueueDir, "incoming") == 0)
      GetProfileStringEx(SYSTEM_PARAM_REG,"MailQueueDir", "incoming", mMailQueueDir, sizeof(mMailQueueDir));
#endif // K_SEARCH OEM 版 //KTEC SMTP Archive Receiver
	nl = strlen(mMailQueueDir);
	if (nl != 0) {
	  nl--;
	  if (mMailQueueDir[nl] != '\\')
	    strcat(mMailQueueDir,"\\");
	}
	//////////////////////////////////////////////////////////////
	//bPOPbeforeSMTP = GetProfileIntEx(SYSTEM_REG, "POPbeforeSMTP", (int) FALSE);   // bPOPbeforeSMTP
//bPOPbeforeSMTP = TRUE;
#ifdef UPDATE_20061102 // 受信メールのエンベロープと本文の任意のフォルダへのバックアップ機能の追加
#ifdef UPDATE_20090205 // バックアップメッセージの拡張子を指定するオプション
    GetProfileStringEx(SYSTEM_REG,"MailBackupExtension", "EML", mExtension, sizeof(mExtension));
#endif
    bMailBackup = GetProfileIntEx(SYSTEM_REG, "MailBackup", (int) FALSE);    // TRUE:バックアップする, FALSE:バックアップしない
    GetProfileStringEx(SYSTEM_REG,"MailBackupDir", "", mMailBackupDir, sizeof(mMailBackupDir)); // フルパス
	nl = strlen(mMailBackupDir);
	if (nl != 0) {
	  nl--;
	  if (mMailBackupDir[nl] != '\\')
	    strcat(mMailBackupDir,"\\");
      strcpy(mTempDir, mMailBackupDir);
      tmp = strstr(mTempDir,":\\");
      if (tmp)
        tmp = strstr(tmp+2,"\\");
      while(tmp) {
        *tmp = '\x0';
	    _mkdir(mTempDir);         // 処理用フォルダ作成
        *tmp = '\\';
	    tmp = strstr(tmp+1,"\\");
	  }
	}
#endif
	
#ifdef UPDATE_20070516 // 上長承認機能の追加
#ifdef UPDATE_20161111 /// 外部からの送信元送信先制限(逆上長承認)
	GetProfileStringEx(SOFT_REG,"ReciverPermitFile", "reciver.dat", mRecivePermitFile, sizeof(mRecivePermitFile));
	GetProfileStringEx(SOFT_REG,"ReciverPermitKeyFile", "reciverkey.dat", mRecivePermitKeyFile, sizeof(mRecivePermitKeyFile));
	GetProfileStringEx(SOFT_REG,"ReciverPermitBlackFile", "reciverblackaddr.dat", mRecivePermitBlackFile, sizeof(mRecivePermitBlackFile));
	GetProfileStringEx(SOFT_REG,"ReciverPermitWhiteFile", "reciverwhiteaddr.dat", mRecivePermitWhiteFile, sizeof(mRecivePermitWhiteFile));
	GetProfileStringEx(SOFT_REG,"ReciverPermitBlackWordFile", "reciverblackword.dat", mRecivePermitBlackWordFile, sizeof(mRecivePermitBlackWordFile));
	GetProfileStringEx(SOFT_REG,"ReciverPermitWhiteWordFile", "reciverwhiteword.dat", mRecivePermitWhiteWordFile, sizeof(mRecivePermitWhiteWordFile));
#endif
#ifdef UPDATE_20090213 // 上長承認メールのmailto:リンクの区切り記号を"?"以外に変更するオプションを追加
    GetProfileStringEx(SOFT_REG,"MailApprovalMailTo", "?", mMailApprovalMailTo, sizeof(mMailApprovalMailTo));
	GetProfileStringEx(SOFT_REG,"MailApprovalMID", "@", mMailApprovalMID, sizeof(mMailApprovalMID));
#endif
#ifdef UPDATE_20231017 // 上長承認依頼メールの作成に失敗した場合の作成リトライ
	nMailApprovalMessNum = GetProfileIntEx(SOFT_REG, "MailApprovalMessNum", (int) 100);    // BossCheck メッセージ作成失敗時のリトライ回数
#endif
	bMailApprovalLog = GetProfileIntEx(SOFT_REG, "MailApprovalLog", (int) TRUE);    // 上長承認機能の履歴作成の有無
#ifdef UPDATE_20140623B // // 上長承認機能の履歴作成時の詳細さ設定フラグを追加した（送信元IPや送信先を）0:含まない(デフォルト) 1:含む
	nMailApprovalLogLevel = GetProfileIntEx(SOFT_REG, "MailApprovalLogLevel", (int) 1);  // 上長承認機能の履歴作成時の詳細さ（送信元IPや送信先を含むか否かの設定）0:含まない(デフォルト) 1:含む
#endif
    GetProfileStringEx(SOFT_REG,"MailApprovalDir", "approval", mMailApprovalDir, sizeof(mMailApprovalDir));
    GetProfileStringEx(SOFT_REG,"MailApprovalManager", "approval", mMailApprovalManager, sizeof(mMailApprovalManager));
	strlwr(mMailApprovalManager);
#ifdef UPDATE_20090114 //BossCheck 承認者数の設定
	nMailApprovalNum = GetProfileIntEx(SOFT_REG, "MailApprovalNum", (int) 2);    // BossCheck 承認者数
#endif
#ifdef UPDATE_20080221 //BossCheck 承認者の代理人の代理人の深さの設定
	nMailApprovalDepath = GetProfileIntEx(SOFT_REG, "MailApprovalDepath", (int) 1);    // 0:代理の代理承認なし 1～:回帰的に可能な代理承認の深さ
#endif
#ifdef UPDATE_20080225 // 部下に対する承認のため保留通知
    bMailApprovalRes = GetProfileIntEx(SOFT_REG, "MailApprovalRes", (int) TRUE);    // FALSE:通知なし, TRUE:部下に対する承認のため保留通知
#endif
#ifdef UPDATE_20161111 /// 外部からの送信元送信先制限(逆上長承認)
    bMailApprovalRejectRes = GetProfileIntEx(SOFT_REG, "MailApprovalRejectRes", (int) (bMailApprovalRevers ? TRUE : FALSE));    // 却下通知の有効有無
#endif

#ifdef UPDATE_20080514 // 上長宛承認メールのmailto:で記載した返信先アドレスのドメインを強制変更する設定
	GetProfileStringEx(SOFT_REG, "MailApprovalDomain", "", mMailApprovalDomain, sizeof(mMailApprovalDomain));
	if (bMailApprovalRevers)
	{
	   mMailApprovalDomain[0] = '\x0';
	}
#endif
#ifdef UPDATE_20080620 // 代理人設定追加動作オプション
    bSetProxyUserType = GetProfileIntEx(SOFT_REG, "SetProxyUserType", (int) TRUE);    // FALSE:オプション動作なし, TRUE:オプション動作あり
#endif
#endif
#ifdef UPDATE_20090910 // report-type="disposition-notification" なら添付無しと判定する
    bMailApprovalNotification = GetProfileIntEx(SOFT_REG, "MailApprovalNotification", (int) FALSE);    // FALSE:オプション動作なし, TRUE:オプション動作あり
#endif
#ifdef UPDATE_20091203 // 承認、否承認を一括処理するためのワイルドカード指定機能のON/OFFオプションを追加した。
    bMailApprovalWildcard = GetProfileIntEx(SOFT_REG, "MailApprovalWildcard", (int) FALSE);    // FALSE:ワイルドカード禁止, TRUE:ワイルドカード許可
#endif
#ifdef UPDATE_20141210 // 承認依頼メールのmailtoリンクの文字コード指定
    nMailApprovalCodepage = GetProfileIntEx(SOFT_REG, "MailApprovalCodepage", (INT) CP_JAPANESE);    // デフォルト CP_JAPANESE 
#endif
#ifdef UPDATE_20150324 // エンベロープの送信先をファイルへの書込みエラーが発生した場合、リトライを最大５回行う
    nRcptWRetry = GetProfileIntEx(SOFT_REG, "RcptWriteRetry", (int) 5); // エンベロープの送信先のファイル書込みエラー時のリトライ回数
#endif
#ifdef UPDATE_20080326 // ドメイン無しアカウントをローカルとしての利用有無選択機能追加
    bHaveDomain  = GetProfileIntEx(SOFT_REG, "HaveDomain", (int) TRUE);    // FALSE:無効, TRUE:有効
#endif
#ifdef UPDATE_20190329 // ML同報時の移動状態を詳細ログに追加
    nMovefileRetry = GetProfileIntEx(SYSTEM_REG, "MovefileRetry", (INT) 30000); // MoveFileEx()での失敗時のリトライ上限 (デフォルト30秒) 0=無制限
#endif
#ifdef REVERS_DNS_FAILED // 逆引きで応答がないものは拒否
	bConfirmReversDNS = GetProfileIntEx(SYSTEM_REG, "ConfirmReversDNS", (int) FALSE);    // TRUE:逆引きチェックする, FALSE:逆引きチェックしない
#endif
#ifdef DOMAIN_AUTH_SPF    // 2006.04.10 ドメイン認証 SPF方式 追加
	bSPF = GetProfileIntEx(SYSTEM_REG, "OnSPF", (int) FALSE);    // TRUE:SPF使用, FALSE:SPF不使用
    GetProfileStringEx(SYSTEM_REG, "DomainAUTHSPF", "", mDomainAUTHSPF, sizeof(mDomainAUTHSPF));
    GetProfileStringEx( "SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters", "NameServer", "", mNS, sizeof(mNS) );
//    GetProfileStringEx(SOFT_REG,   "NameServer", "",    mNS, sizeof(mNS) );
	strtok(mDomainAUTHSPF,"\r\n");
	strtok(mNS,"\r\n");
	strcpy(mNS1, mNS);  // DNSを３個に分解
	if ((p = strchr(mNS1, ' '))) {
	  *p = '\x0';
	  p++;
	  strcpy(mNS2, p);
	  if ((p = strchr(mNS2, ' '))) {
	    *p = '\x0';
	     p++;
	     strcpy(mNS3, p);
		 strtok(mNS3, " ");
	  }
	}
#endif
#ifdef DOMAIN_AUTH_DKIM    // 2013.01.17 ドメイン認証 DomainkeysとDKIM方式 追加
    bDKIM = GetProfileIntEx(SYSTEM_REG, "OnDKIM", (int) FALSE);    // TRUE:DKIM使用, FALSE:DKIM不使用
    GetProfileStringEx(SYSTEM_REG, "DomainAUTHDKIM", "", mDomainAUTHDKIM, sizeof(mDomainAUTHDKIM));
#endif
#ifdef SENDERID
	bSenderID = GetProfileIntEx(SYSTEM_REG, "SenderID", (int) FALSE);    // TRUE:SenderID使用, FALSE:SenderID不使用
#endif
#ifdef EIGHT_BITMIME
    b8BITMIME= GetProfileIntEx(SYSTEM_REG, "8BITMIME", (int) FALSE);    // TRUE:8BITMIME使用, FALSE:8BITMIME不使用
#endif

#ifdef ESMTP_AUTH
	nSMTPAUTHOnly = GetProfileIntEx(SYSTEM_REG, "SMTPAUTHOnly", (int) 0);    // 0:両方 1:認証専用 2:認証ﾌｧｲﾙ毎
	nFROMSecLevel = GetProfileIntEx(SYSTEM_REG, "FROMSecLevel", (int) 0);    //  認証ＩＤ一致レベル 0:SMTP-AUTH ID = 1:SMTP-AUTH ID And MAIL FROM: = 2: SMTP-AUTH ID and MAIL FROM: and HEADER FROM:
//SMTPAUTH = TRUE;
    //GetProfileStringEx(SYSTEM_REG, "SMTPAUTHMode", "PLAIN CRAM-MD5", mSMTPAUTHMODE, sizeof(mSMTPAUTHMODE)); //mSMTPAUTHMODE
    GetProfileStringEx(SYSTEM_REG, "SMTPAUTHMode", "NO", mSMTPAUTHMODE, sizeof(mSMTPAUTHMODE)); //mSMTPAUTHMODE
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
#endif
#ifdef ADD_IDCACHE
	nIDCashLiveTime = GetProfileIntEx(SYSTEM_REG, "ADCashLiveTime", (INT) 0); // MXキャッシュ利用有効時間(秒単位) デフォルト 0:無効
#endif
	nMixPriority = GetProfileIntEx(SYSTEM_REG, "MixPriority", (DWORD) 1);   //優先される添付者(受信者優先)
	//bTrace = GetProfileIntEx("SYSTEM\\CurrentControlSet\\Services\\SPARS", "Trace", (int)FALSE);
	bTrace = GetProfileIntEx(SYSTEM_REG, "Trace", (int)FALSE);
	sprintf(mTraceFile,"%strace.dat", mProgPath);
	//bVirus = GetProfileIntEx("SYSTEM\\CurrentControlSet\\Services\\SPARS", "VirusCheck", (int)FALSE);
	bVirus = GetProfileIntEx(SYSTEM_REG, "VirusCheck", (int)FALSE);
	sprintf(mVirusFile,"%svirus.dat", mProgPath);
	sprintf(mMailFile,"%smail.dat", mProgPath); 
#ifdef MILTER_ON // MILTERインターフェースを追加。
	sprintf(mMILTERFile,"%smilter.ini", mProgPath); 
#endif
#ifdef UPDATE_20150212 // 接続元のアドレスによって受信メールを破棄するオプション
	sprintf(mDISCARDFile,"%sdiscardmail.ini", mProgPath); 
#endif

#ifdef V3
	nVirusDoubtful = GetProfileIntEx(SYSTEM_REG, "VirusDoubtfulCheck", (int)0); // 0:OFF 1:ON記録有り 2:ON記録無し
	nViursMailSzie = GetProfileIntEx(SYSTEM_REG, "VirusMailSize", (int)1024000); // ウイルスメールチェック対象の上限サイズ デフォルト 1MByte
	sprintf(mVirus2File,"%svirus+.dat", mProgPath); 
#endif
	//////////////////////////////////////////////
	sprintf(mORDBFile,"%sordb.dat", mProgPath); // ORDB.DAT　問合せサイトテーブル
	sprintf(mPASSDBFile,"%spassdb.dat", mProgPath); // ORDB非問合せIPテーブル
    //////////////////////////////////////////////
#ifdef FILTER_PASS
	sprintf(mPASSFilterFile,"%spassfilter.dat", mProgPath); // フィルタ非問合せIPテーブル
#endif
#ifdef UPDATE_20050329
	sprintf(mPASS3rdFile,"%spass3rd.dat", mProgPath); // 3rd-partyDB非問合せIPテーブル
#endif
#ifdef UPDATE_20060315 // 隠すIPアドレス
	sprintf(mHiddenMyIPFile,"%shiddensmtpip.dat", mProgPath); 
#endif
#ifdef UPDATE_20151122 // 特定接続元IPからの接続拒絶用テーブル追加(rejectsmtpip.dat)隠しオプション
	sprintf(mRejectPeerPFile,"%srejectsmtpip.dat", mProgPath); 
#endif
#ifdef UPDATE_20211020 // 特定接続元IPからの複写転送先指定無効アドレス設定テーブル追加(ccdisableip.dat)
	sprintf(mCCDisableFile,"%sccdisableip.dat", mProgPath); // 複写転送先指定禁止設定テーブル
#endif
#ifdef UPDATE_20160113 // 認証時のロックアウト機能を追加
	nAuthLockOut = GetProfileIntEx(SYSTEM_REG, "AuthLockOut", (int)0); // ロックアウトまでの回数 デフォルト 0:無効
#endif
#ifdef UPDATE_20160114 // 接続拒絶用テーブルの指定に有効時間を設定可能にした。
	nAuthLockOutTime = GetProfileIntEx(SYSTEM_REG, "AuthLockOutTime", (INT)0); // ロックアウト時間 デフォルト 0:無限 分単位
#endif
#ifdef UPDATE_20190216 // IP毎の同時接続ロックアウト機能を追加
    nIPLockOut = GetProfileIntEx(SYSTEM_REG, "IPLockOut", (INT)0); // ロックアウトまでの回数 デフォルト 0:無効
    nIPLockOutTime = GetProfileIntEx(SYSTEM_REG, "IPLockOutTime", (INT)0); // ロックアウト時間 デフォルト 0:無限 秒単位
#ifdef UPDATE_20190218 // IP毎の同時接続ロックアウト機能を強化（サンプル時間と拒絶時間の設定を追加）
	nIPLockOutSPTime = GetProfileIntEx(SYSTEM_REG, "IPLockOutSPTime", (INT)0); // IPロックアウトまでのサンプリング時間 デフォルト 0:無限 秒単位
#endif
#endif
#ifdef FIRST_TIME_MAIL // 繰り返す同一内容のメール除去
	sprintf(mPASSFTMFile,"%spassftm.dat", mProgPath); // 繰り返す同一内容のメール除去非問合せIPテーブル
	bFTM_ON = GetProfileIntEx(SYSTEM_REG, "FTM", (BOOL)FALSE);// 同一メールの保管チェックの有無
	nFTMExpire = GetProfileIntEx(SYSTEM_REG, "FTMExpire", (int)10); // 同一メールの保管期間　デフォルト１０日
	nFTMLength = GetProfileIntEx(SYSTEM_REG, "FTMLength", (int)16); // 比較トークンの長さ　　デフォルト１６バイト
    nFTMMargin= GetProfileIntEx(SYSTEM_REG, "FTMMargin", (int)95);  // 比較結果の許容範囲　　デフォルト９５％一致で同一と判断
#endif
#ifdef SHORT_SURBL // 短縮URLのSURBL判定対策
    sprintf(mShortDecodeFile,"%sshorturl.dat", mProgPath); //  短縮URLの変換テーブル
#endif

    //////////////////////////////////////////////
	//nReceived = GetProfileIntEx("SYSTEM\\CurrentControlSet\\Services\\SPARS", "Received", (int)20); //FALSE);
	nReceived = GetProfileIntEx(SYSTEM_REG, "Received", (int)20); //FALSE);
    //bAnnounce = GetProfileIntEx("SYSTEM\\CurrentControlSet\\Services\\SPARS", "Announce", (int)FALSE);
//    bAnnounce = GetProfileIntEx(SYSTEM_REG, "Announce", (int)FALSE);
	//bAnnLimit = GetProfileIntEx("SYSTEM\\CurrentControlSet\\Services\\SPARS", "AnnLimit", (int)TRUE);
//	bAnnLimit = GetProfileIntEx(SYSTEM_REG, "AnnLimit", (int)TRUE);
	//////////////////////////////////////////////////////////////
#ifdef UPDATE20240620 // effect.datでクラスB以上のサブネットマスクの指定に不具合
     bEffectmaskengin = GetProfileIntEx(SYSTEM_REG, "Effectmaskengin", (int)TRUE); // ネットマスクの処理方式 デフォルト TRUE:新型 FALSE:旧型
#endif
#ifdef UPDATE20240728 // サブネットマスクの範囲指定の高速化
     bIPcmpmaskengin = GetProfileIntEx(SYSTEM_REG, "IPcmpmaskengin", (int)TRUE); // ネットマスクの処理方式 デフォルト TRUE:新型 FALSE:旧型
#endif
#ifdef ADD_XOAUTH2 // OAUTH2での認証方式を追加
	sprintf(mOAuthFile,"%soauth2.dat", mProgPath);
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
  	  sprintf(mTempDir, "%sinlog", mMailSpoolDir);
	  _mkdir(mTempDir);         // inlogフォルダ作成
#ifdef REGTOFILE
      if (nClustering) {
  	    sprintf(mTempDir, "%sinlog\\%s", mMailSpoolDir, mComName);
	    _mkdir(mTempDir);         // inlogフォルダ作成
	  }
#endif
	}
    if (bAcceptLog) {
      sprintf(mTempDir, "%sacceptlog", mMailSpoolDir);
  	  _mkdir(mTempDir);         // 接続ログフォルダ作成
#ifdef REGTOFILE
      if (nClustering) {
  	    sprintf(mTempDir, "%sacceptlog\\%s", mMailSpoolDir, mComName);
	    _mkdir(mTempDir);         // inlogフォルダ作成
	    sprintf(mTempDir, "%sreceivelog\\%s", mMailSpoolDir, mComName);
	    _mkdir(mTempDir);         // receivelogフォルダ作成
	  }
#endif
	}
#ifdef V3
#ifdef UPDATE_20240523 // ウイルスチェック時のログ保存先が自動生成されない不具合
    if (bVirus || (b3rdViruslog && mThirdparty[0]) || nVirusDoubtful)
#else
    if (bVirus || nVirusDoubtful)
#endif
#else
    if (bVirus)
#endif
	{
#ifdef UPDATE_20240523 // ウイルスチェック時のログ保存先が自動生成されない不具合
	  if (mMailSpoolDir[0] == '\\' && (b3rdViruslog && mThirdparty[0])) { // UNC接続の場合
        GetProfileStringEx(SOFT_REG,"MailTempDir", mProgPath, mMailTempDir, sizeof(mMailTempDir));
        sprintf(mTempDir, "%sviruslog", mMailTempDir);     // 実行マシン上に置く処理速度の向上のためテンポラリフォルダ
        _mkdir(mTempDir);
	  }
#endif
      sprintf(mTempDir, "%sviruslog", mMailSpoolDir);
  	  _mkdir(mTempDir);         // 接続ログフォルダ作成
	}
/*
	if (bAnnounce) {
      sprintf(mTempDir, "%sattach", mMailSpoolDir);
	  _mkdir(mTempDir);         // 添付用フォルダ作成
	}
*/
#ifdef CLUSTERING
	mMsgIDFN[0] = mMsgIDFNLock[0] = '\x0';
	if (nClustering) {
      sprintf(mTempDir, "%scluster", mMailSpoolDir);
	  _mkdir(mTempDir);         // 処理用フォルダ作成
	  sprintf(mMsgIDFN,"%s\\msg.id", mTempDir);  // メッセージＩＤカウンタファイル。
      sprintf(mMsgIDFNLock, "%s\\msg.lck", mTempDir);  // メッセージＩＤカウンタ排他ファイル。
      nLastMsgId = ReadLastMsgIdForAscii(&hFile, nLastMsgId);
	  if (hFile)
        CloseHandle(hFile);
	}
#endif

#ifdef UPDATE_20070516 // 上長承認機能の追加
#ifdef UPDATE_20161112 //題名ヘッダでのブラック＆ホワイトリストテーブル
    if (bMailApproval || bMailApprovalRevers)
#else
	if (bMailApproval) 
#endif
	{
      sprintf(mTempDir, "%s%s", mMailSpoolDir, mMailApprovalDir);
      _mkdir(mTempDir);         // 接続ログフォルダ作成
      sprintf(mTempDir, "%s%s\\log", mMailSpoolDir, mMailApprovalDir);
      _mkdir(mTempDir);         // 接続ログフォルダ作成
	}
#endif

#ifdef FIRST_TIME_MAIL // 繰り返す同一内容のメール除去
    if (bFTM_ON) {
      sprintf(mTempDir, "%smaildb", mMailSpoolDir);
  	  _mkdir(mTempDir);         // データベースフォルダ作成共有
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

#ifdef CLUSTERING
	if (mMailSpoolDir[0] == '\\') { // UNC接続の場合
      GetProfileStringEx(SOFT_REG,"MailTempDir", mProgPath, mMailTempDir, sizeof(mMailTempDir));
      sprintf(mTempDir, "%stemp", mMailTempDir);     // 実行マシン上に置く処理速度の向上のためテンポラリフォルダ
	} else 
#endif
    sprintf(mTempDir, "%stemp", mMailSpoolDir); // テンポラリフォルダ
	_mkdir(mTempDir);         // 処理用フォルダ作成
	strcat(mTempDir, "\\");
	//////////////////////////////////////////////////////////////
#ifdef  INCOM_SUB_FOLDER // incoming フォルダをスレッド別フォルダにして処理
    bIncomingSubFolder = GetProfileIntEx(SYSTEM_REG_DS, "IncomingSubFolder", (int) FALSE); // incomingフォルダにスレッド別フォルダを作成 TRUE:する FALSE:しない
	bThreadType = GetProfileIntEx(SYSTEM_REG_DS, "ThreadType", (int) TRUE); // デフォルトはマルチスレッド
	nMaxThread  = GetProfileIntEx(SYSTEM_REG_DS, "MaxThread", (int) 2);     // デフォルトはスレッド数1
    if (bIncomingSubFolder) // incomingフォルダにスレッド別フォルダを作成 TRUE:する FALSE:しない
	{
	  for (nNum = 0; nNum < nMaxThread; nNum++)
	  {
		sprintf(mIncomingThread, "%s%s\\%05d", mMailSpoolDir, mMailQueueDir, nNum);
		_mkdir(mIncomingThread);
	  }
	}
#endif
	//////////////////////////////////////////////////////////////
	//// レジストリ内容の表示
	//////////////////////////////////////////////////////////////
#ifdef ACCEPT_LOG_LEVEL3
    LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
	//printf("---- regestry ----\n");
	sprintf(mLog, "---- regestry ----");
	printf("%s\n", mLog);
#ifdef ACCEPT_LOG_LEVEL3
    LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
	sprintf(mLog, "Licence key          %s (len=%d)", mLicencekey, strlen(mLicencekey));
	printf("%s\n", mLog);
#ifdef ACCEPT_LOG_LEVEL3
    LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
#ifndef K_SEARCH // K_SEARCH OEM 版
	//printf("Priority             %d\n", nPriority);
	sprintf(mLog, "Priority             %d", nPriority);
	printf("%s\n", mLog);
#ifdef ACCEPT_LOG_LEVEL3
    LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
#endif
#ifdef V3
	//printf("User Manager         %s\n", bUserMan ? "NT use":"SPA use");
#ifdef UPDATE_20070516 // 上長承認機能の追加
	sprintf(mLog, "Mail Approval (out)  %s", bMailApproval ? "on" : "off");
	printf("%s\n", mLog);
#ifdef ACCEPT_LOG_LEVEL3
    LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
    // 逆上長承認機能の追加
	sprintf(mLog, "Mail Approval (in)   %s", bMailApprovalRevers ? "on" : "off");
	if (bDebug) printf("%s\n", mLog);
#ifdef ACCEPT_LOG_LEVEL3
    LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
#endif
#ifndef K_SEARCH // K_SEARCH OEM 版
#ifdef LDAP_ON
	sprintf(mLog, "User Manager         %s", bUserMan ? (bLDAP ? "LDAP use" : "NT use") : "SPA use");
#else
	sprintf(mLog, "User Manager         %s", bUserMan ? "NT use" : "SPA use");
#endif
	printf("%s\n", mLog);
#ifdef ACCEPT_LOG_LEVEL3
    LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
	if (bUserMan) {
	  //printf("Membership           %s\n", (mAuthDomain[0] ? mAuthDomain : "workgroup"));
	  sprintf(mLog, "Membership           %s", (mAuthDomain[0] ? mAuthDomain : "workgroup"));
	} else {
	  //printf("Account folder       %s\n", (mAuthDomain[0] ? mAuthDomain : "current"));
	  sprintf(mLog, "Account folder       %s", (mAuthDomain[0] ? mAuthDomain : "current"));
	}
    printf("%s\n", mLog);
#ifdef ACCEPT_LOG_LEVEL3
    LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
#endif //#ifndef K_SEARCH // K_SEARCH OEM 版
#else
	//printf("Membership           %s\n", (mAuthDomain[0] ? mAuthDomain : "workgroup"));
    sprintf(mLog, "Membership           %s", (mAuthDomain[0] ? mAuthDomain : "workgroup"));
    printf("%s\n", mLog);
#ifdef ACCEPT_LOG_LEVEL3
    LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
#endif
#ifndef K_SEARCH // K_SEARCH OEM 版
#ifdef UPDATE_20060606 // ドメインコントローラへの接続リトライ
#ifdef UPDATE_20091103 // LDAP連携専用のリトライ回数指定の変数に変更
	sprintf(mLog, "AD query retry       %lu(ms) x %lu(time)", nADRetryMSec, nADRetryTime);
#else
	sprintf(mLog, "AD query retry time  %lu", nADRetryTime);
#endif
	printf("%s\n", mLog);
#ifdef ACCEPT_LOG_LEVEL3
    LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
#endif
#ifdef ADD_IDCACHE
	sprintf(mLog, "AD Cache live time   %lu(s)", nIDCashLiveTime);
	printf("%s\n", mLog);
#ifdef ACCEPT_LOG_LEVEL3
    LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
#endif

#ifdef UPDATE_20091103 // LDAP連携専用のリトライ回数指定の変数に変更
	if (bLDAPOn)
	{
	  sprintf(mLog, "LDAP query retry     %lu(ms) x %lu(time)", nLDAPRetryMSec, nLDAPRetryTime);
	  printf("%s\n", mLog);
#ifdef ACCEPT_LOG_LEVEL3
      LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
	}
#endif

#ifdef USE_SSL
	//printf("Smtp over SSL        %s\n", (bUsedSSL ? "yes" : "no"));
    sprintf(mLog, "Smtp over SSL        %s", (bUsedSSL ? "yes" : "no"));
    printf("%s\n", mLog);
#ifdef ACCEPT_LOG_LEVEL3
    LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
#ifdef USE_STARTTLS
    sprintf(mLog, "STARTTLS             %s", (bUsedSTLS ? "on" : "off"));
    printf("%s\n", mLog);
#ifdef ACCEPT_LOG_LEVEL3
    LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
#endif
	//printf("Certificate          %s\n", mCertificate);
    sprintf(mLog, "Certificate          %s", mCertificate);
    printf("%s\n", mLog);
#ifdef ACCEPT_LOG_LEVEL3
    LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
	//printf("Private-Key          %s\n", mPrivatekey);
    sprintf(mLog, "Private-Key          %s", mPrivatekey);
    printf("%s\n", mLog);
#ifdef ACCEPT_LOG_LEVEL3
    LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
#endif
#ifdef IPv6
	//printf("Host IP version      %s\n", nAddressFamily ? (nAddressFamily == 2 ? "IPv4/6" : "IPv6") :"IPv4");
    sprintf(mLog, "Host IP version      %s", nAddressFamily ? (nAddressFamily == 2 ? "IPv4/6" : "IPv6") :"IPv4");
    printf("%s\n", mLog);
#ifdef ACCEPT_LOG_LEVEL3
    LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
	//printf("Host Name(IPv6)      %s\n", mHostName);
    sprintf(mLog, "Host Name(IPv6)      %s", mHostName);
    printf("%s\n", mLog);
#ifdef ACCEPT_LOG_LEVEL3
    LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
#endif
#endif // #ifndef K_SEARCH // K_SEARCH OEM 版
#ifdef TMQ_ON
#ifndef BTHREAD
	//printf("Accept limit         %ld%s\n", nAcceptLimit, (nAcceptLimit ? "":"(Unlimited)") );
    sprintf(mLog, "Accept limit         %ld%s", nAcceptLimit, (nAcceptLimit ? "":"(Unlimited)") );
    printf("%s\n", mLog);
#ifdef ACCEPT_LOG_LEVEL3
    LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
#endif
	//printf("Timer                %d(sec)\n", nTMOut);
    sprintf(mLog, "Timer                %d(sec)", nTMOut);
    printf("%s\n", mLog);
#ifdef ACCEPT_LOG_LEVEL3
    LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
#endif
#ifdef BTHREAD
	sprintf(mLog, "Accept limit         %ld%s", nAcceptLimit, (nAcceptLimit ? "":"(Unlimited)") );
    printf("%s\n", mLog);
#ifdef ACCEPT_LOG_LEVEL3
    LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
#ifndef K_SEARCH // K_SEARCH OEM 版
	if (nRecvBuf > 0)
	  sprintf(mLog, "Recv socket buff     %ld", nRecvBuf);
	else
	  sprintf(mLog, "Recv socket buff     default");
    printf("%s\n", mLog);
#ifdef ACCEPT_LOG_LEVEL3
    LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
	sprintf(mLog, "Recv Data Timer      %s %d(s)", (nTMOut ? "on" : "off"), nTMOut);
    printf("%s\n", mLog);
#ifdef ACCEPT_LOG_LEVEL3
    LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
	//printf("Send Data Timer      %s %d(s)\n", (nSendTMO ? "on" : "off"), nSendTMO);
    sprintf(mLog, "Send Data Timer      %s %d(s)", (nSendTMO ? "on" : "off"), nSendTMO);
    printf("%s\n", mLog);
#ifdef ACCEPT_LOG_LEVEL3
    LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
	//printf("Trace Mode           %s\n", bTrace ? "on":"off");
    sprintf(mLog, "Trace Mode           %s", bTrace ? "on":"off");
    printf("%s\n", mLog);
#ifdef ACCEPT_LOG_LEVEL3
    LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
#endif
#endif //#ifndef K_SEARCH // K_SEARCH OEM 版

#ifdef REVERS_DNS_FAILED // 逆引きで応答がないものは拒否
    sprintf(mLog, "Confirm revers DNS   %s", (bConfirmReversDNS ? "on" : "off"));
    printf("%s\n", mLog);
#ifdef ACCEPT_LOG_LEVEL3
    LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
#endif
#ifndef K_SEARCH // K_SEARCH OEM 版
	//printf("VirusCheck           %s\n", bVirus ? "on":"off");
    sprintf(mLog, "Mail Filter          %s", bVirus ? "on":"off");
    printf("%s\n", mLog);
#ifdef ACCEPT_LOG_LEVEL3
    LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
#ifdef V3
    sprintf(mLog, "VirusDoubtfulCheck   %s %s", nVirusDoubtful ? "on":"off", nVirusDoubtful == 1 ? "recode" : "");
    printf("%s\n", mLog);
#ifdef ACCEPT_LOG_LEVEL3
    LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
    sprintf(mLog, "VirusMailSize        less than %lu bytes", nViursMailSzie);
    printf("%s\n", mLog);
#ifdef ACCEPT_LOG_LEVEL3
    LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
#endif
	//printf("Vrfy/Expn            %s\n", bVrfy ? "enable":"disable");
    sprintf(mLog, "Vrfy/Expn            %s", bVrfy ? "enable":"disable");
    printf("%s\n", mLog);
#ifdef ACCEPT_LOG_LEVEL3
    LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
#endif
#ifndef K_SEARCH // K_SEARCH OEM 版
#ifdef DOMAIN_AUTH_SPF    // 2006.04.10 ドメイン認証 SPF方式 追加
    sprintf(mLog, "Domain AUTH SPF      %s %s", (bSPF ? "enable":"disable"), mDomainAUTHSPF);
	if (bDebug) printf("%s\n", mLog);
#ifdef ACCEPT_LOG_LEVEL3
    LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
#ifdef DOMAIN_AUTH_DKIM    // 2013.01.17 ドメイン認証 DomainkeysとDKIM方式 追加
    sprintf(mLog, "Domain AUTH DKIM     %s %s", (bDKIM ? "enable":"disable"), mDomainAUTHDKIM);
	if (bDebug) printf("%s\n", mLog);
#ifdef ACCEPT_LOG_LEVEL3
    LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
#endif
#endif

#ifdef SENDERID
    sprintf(mLog, "Sender ID            %s", bSenderID ? "enable":"disable");
    printf("%s\n", mLog);
#ifdef ACCEPT_LOG_LEVEL3
    LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
#endif

#ifdef EIGHT_BITMIME
    sprintf(mLog, "8bit MIME            %s", b8BITMIME ? "on" : "off");
    printf("%s\n", mLog);
#ifdef ACCEPT_LOG_LEVEL3
    LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
#endif

#ifdef ESMTP_AUTH
	//printf("SMTP AUTH Mode       %s\n", mSMTPAUTHMODE);
    sprintf(mLog, "SMTP AUTH Mode       %s", mSMTPAUTHMODE);
    printf("%s\n", mLog);
#ifdef ACCEPT_LOG_LEVEL3
    LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
	//printf("SMTP AUTH Sec Level  %s\n", nSMTPAUTHOnly == 0 ? "Or non-AUTH":(nSMTPAUTHOnly == 1 ? "Only the AUTH" :"AUTH file"));
    sprintf(mLog, "SMTP AUTH Sec Level  %s", nSMTPAUTHOnly == 0 ? "Or non-AUTH":(nSMTPAUTHOnly == 1 ? "Only the AUTH" :"AUTH file"));
    printf("%s\n", mLog);
#ifdef ACCEPT_LOG_LEVEL3
    LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
    sprintf(mLog, "FROM Addr Sec Level  %d", nFROMSecLevel);
    printf("%s\n", mLog);
#ifdef ACCEPT_LOG_LEVEL3
    LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
#endif
#endif //#ifndef K_SEARCH // K_SEARCH OEM 版
	//printf("Smtp IP              permit");
    sprintf(mLog, "Smtp IP              permit");
    printf("%s\n", mLog);
#ifdef ACCEPT_LOG_LEVEL3
    LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
	if (bListenMode) {
	  pldom = mListenIP;
	  while (strlen(pldom)){
	    //printf(" \"%s\"", pldom);
        sprintf(mLog, " \"%s\"", pldom);
		printf("%s\n", mLog);
#ifdef ACCEPT_LOG_LEVEL3
    LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
        pldom += strlen(pldom);
 	    pldom++;
	  };
	  //printf("\n");
      sprintf(mLog, "");
      printf("%s\n", mLog);
#ifdef ACCEPT_LOG_LEVEL3
    LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
	} else {
      //printf(" all\n");
      sprintf(mLog, " all\n");
      printf("%s\n", mLog);
#ifdef ACCEPT_LOG_LEVEL3
    LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
	}
	//printf("Smtp Port            %d\n", nport);
    sprintf(mLog, "Smtp Port            %d", nport);
    printf("%s\n", mLog);
#ifdef ACCEPT_LOG_LEVEL3
    LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
#ifndef K_SEARCH // K_SEARCH OEM 版
	//printf("MailGroup            \"%s\"\n",mMailGroup);
    sprintf(mLog, "MailGroup            \"%s\"",mMailGroup);
    printf("%s\n", mLog);
#ifdef ACCEPT_LOG_LEVEL3
    LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
    /////////////////////////////////
#ifdef  INCOM_SUB_FOLDER // incoming フォルダをスレッド別フォルダにして処理
	printf("Thread Sub Folder    %s\n", bIncomingSubFolder ? "yes":"no");
	printf("Thread Type          %s=%d\n", bThreadType ? "multi":"single", bThreadType ? nMaxThread : 1);
    printf("%s\n", mLog);
#ifdef ACCEPT_LOG_LEVEL3
    LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
#endif
    /////////////////////////////////
	//printf("LocalDomain List    ");
    sprintf(mLog, "LocalDomain List    ");
    printf("%s\n", mLog);
#ifdef ACCEPT_LOG_LEVEL3
    LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
	pldom = mLocalDomain;
	while (strlen(pldom)){
	  //printf(" \"%s\"", pldom);
      sprintf(mLog, " \"%s\"", pldom);
      printf("%s\n", mLog);
#ifdef ACCEPT_LOG_LEVEL3
    LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
      pldom += strlen(pldom);
 	  pldom++;
	};
	//printf("\n");
    sprintf(mLog, "");
    printf("%s\n", mLog);
#ifdef ACCEPT_LOG_LEVEL3
    LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
	//printf("Carbon Copy List ");
    sprintf(mLog, "Carbon Copy List ");
    printf("%s\n", mLog);
#ifdef ACCEPT_LOG_LEVEL3
    LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
	pldom = mMailInCopy;
 	while (strlen(pldom)){
	  //printf(" \"%s\"", pldom);
      sprintf(mLog, " \"%s\"", pldom);
      printf("%s\n", mLog);
#ifdef ACCEPT_LOG_LEVEL3
    LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
      pldom += strlen(pldom);
 	  pldom++;
	};
	//printf("\n");
    sprintf(mLog, "");
    printf("%s\n", mLog);
#ifdef ACCEPT_LOG_LEVEL3
    LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
	//printf("MailInMaxSize        ");
    sprintf(mLog, "MailInMaxSize        ");
    printf("%s\n", mLog);
#ifdef ACCEPT_LOG_LEVEL3
    LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
	if (nMailInMaxSize == 0) {
	  //printf("unlimited\n");
      sprintf(mLog, "unlimited");
	} else {
      //printf("%d\n", nMailInMaxSize);
      sprintf(mLog, "%d", nMailInMaxSize);
	}
    printf("%s\n", mLog);
#ifdef ACCEPT_LOG_LEVEL3
    LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
    //printf("MailInBoxSize        ");
    sprintf(mLog, "MailInBoxSize        ");
    printf("%s\n", mLog);
#ifdef ACCEPT_LOG_LEVEL3
    LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
#ifdef UPDATE_ULONGLONG //64bit長のデータで取得可能にする
    if (nMailInBoxSizeHigh == 0 && nMailInBoxSize == 0)
#else
	if (nMailInBoxSize == 0)
#endif
	{
	  //printf("unlimited\n");
      sprintf(mLog, "unlimited");
	} else {
      //printf("%d\n", nMailInBoxSize);
#ifdef UPDATE_ULONGLONG //64bit長のデータで取得可能にする
      ULARGE_INTEGER u;
	  u.LowPart = nMailInBoxSize;
      u.HighPart = nMailInBoxSizeHigh;
      sprintf(mLog, "%I64u\n", u.QuadPart);
#else
      sprintf(mLog, "%ul\n", nMailInBoxSize);
#endif
	}
    printf("%s\n", mLog);
#ifdef ACCEPT_LOG_LEVEL3
    LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
    //printf("RCPTMaximum          ");
    sprintf(mLog, "RCPTMaximum          ");
    printf("%s\n", mLog);
#ifdef ACCEPT_LOG_LEVEL3
    LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
	if (nRCPTMaximum == 0) {
	  //printf("unlimited\n");
      sprintf(mLog, "unlimited");
	} else {
      //printf("%d\n", nRCPTMaximum);
      sprintf(mLog, "%d", nRCPTMaximum);
	}
    printf("%s\n", mLog);
#ifdef ACCEPT_LOG_LEVEL3
    LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
    //printf("ReceivedHeader       %d\n", nReceived);
    sprintf(mLog, "ReceivedHeader       %d", nReceived);
    printf("%s\n", mLog);
#ifdef ACCEPT_LOG_LEVEL3
    LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
	// メールフィルタ機能強化 FALSE:なし TRUE:あり
    sprintf(mLog, "FiterClass           %d", nMailFiterUp);
    printf("%s\n", mLog);
#ifdef ACCEPT_LOG_LEVEL3
    LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
#endif //#ifndef K_SEARCH // K_SEARCH OEM 版
	//printf("LastMsgId            %010lu\n", nLastMsgId);
    sprintf(mLog, "LastMsgId            %010lu", nLastMsgId);
    printf("%s\n", mLog);
#ifdef ACCEPT_LOG_LEVEL3
    LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif

#ifdef CLUSTERING
	if (nClustering) {
      sprintf(mLog, "Clustering           on");
      printf("%s\n", mLog);
#ifdef ACCEPT_LOG_LEVEL3
      LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
	}
#endif

	//printf("MailQueueDir         %s\n", mMailQueueDir);
    sprintf(mLog, "MailQueueDir         %s", mMailQueueDir);
    printf("%s\n", mLog);
#ifdef ACCEPT_LOG_LEVEL3
    LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
	//printf("MailSpoolDir         %s\n", mMailSpoolDir);
    sprintf(mLog, "MailSpoolDir         %s", mMailSpoolDir);
    printf("%s\n", mLog);
#ifdef ACCEPT_LOG_LEVEL3
    LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
#ifndef K_SEARCH // K_SEARCH OEM 版
	//printf("MailBoxDir           %s\n", mMailBoxDir);
    sprintf(mLog, "MailBoxDir           %s", mMailBoxDir);
    printf("%s\n", mLog);
#ifdef ACCEPT_LOG_LEVEL3
    LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
#endif
    //printf("Program Path         %s\n", mProgPath);
    sprintf(mLog, "Program Path         %s", mProgPath);
    printf("%s\n", mLog);
#ifdef ACCEPT_LOG_LEVEL3
    LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
#ifndef K_SEARCH // K_SEARCH OEM 版
    //printf("Password File        %s\n", mPasswordFile);
    sprintf(mLog, "Password File        %s", mPasswordFile);
    printf("%s\n", mLog);
#ifdef ACCEPT_LOG_LEVEL3
    LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
#ifdef V4
    sprintf(mLog, "Greeting messages    %s", bHide ? "hide" : "show");
    printf("%s\n", mLog);
#ifdef ACCEPT_LOG_LEVEL3
    LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
    sprintf(mLog, "Use time File        %s", mUseTimeFile);
    printf("%s\n", mLog);
#ifdef ACCEPT_LOG_LEVEL3
    LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
#ifdef UPDATE_20050924  // 送信元ユーザ別送信先制限
    sprintf(mLog, "Sender permit File   %s", mSenderPermitFile);
    printf("%s\n", mLog);
#ifdef ACCEPT_LOG_LEVEL3
    LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
#endif
#endif
#endif //#ifndef K_SEARCH // K_SEARCH OEM 版
	//printf("AcceptLog            %s\n", bAcceptLog ? "on":"off");
    sprintf(mLog, "AcceptLog            %s", bAcceptLog ? "on":"off");
    printf("%s\n", mLog);
#ifdef ACCEPT_LOG_LEVEL3
    LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
	///printf("inLog                %s\n", bLog ? "on":"off");
    sprintf(mLog, "inLog                %s", bLog ? "on":"off");
    printf("%s\n", mLog);
#ifdef ACCEPT_LOG_LEVEL3
    LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
#ifndef K_SEARCH // K_SEARCH OEM 版
#ifdef THIRDPARTY
    GetProfileStringEx(SYSTEM_REG, THIRDPARTYMODULE, "", mThirdparty, sizeof(mThirdparty));
    sprintf(mLog, "Thirdparty           %s", mThirdparty[0] ? mThirdparty : "off");
    printf("%s\n", mLog);
#ifdef ACCEPT_LOG_LEVEL3
    LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
#endif
#endif //#ifndef K_SEARCH // K_SEARCH OEM 版

#ifdef FIRST_TIME_MAIL // 繰り返す同一内容のメール除去
    if (bFTM_ON) {
      sprintf(mLog, "FTM DB Expire        %d days", nFTMExpire); // 同一メールの保管期間　デフォルト９０日
      printf("%s\n", mLog);
#ifdef ACCEPT_LOG_LEVEL3
    LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
      sprintf(mLog, "FTM DB Level         %d bytes", nFTMLength); // 比較トークンの長さ　　デフォルト１０バイト
      printf("%s\n", mLog);
#ifdef ACCEPT_LOG_LEVEL3
    LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
      sprintf(mLog, "FTM DB Margin        %d %%", nFTMMargin);  // 比較結果の許容範囲　　デフォルト８０％一致で同一と判断
      printf("%s\n", mLog);
#ifdef ACCEPT_LOG_LEVEL3
    LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif

	}
#endif
#ifdef MILTER_ON
    sprintf(mLog, "MILTER               %s", bMILTER ? "on":"off");
    printf("%s\n", mLog);
#ifdef ACCEPT_LOG_LEVEL3
    LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
#endif
#ifdef UPDATE_20150212 // 接続元のアドレスによって受信メールを破棄するオプション
    sprintf(mLog, "DISCARDMAIL          %s", bDISCARDMAIL ? "on":"off");
    printf("%s\n", mLog);
#ifdef ACCEPT_LOG_LEVEL3
    LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
#endif
	//printf("-------------------\n");
    sprintf(mLog, "-------------------");
    printf("%s\n", mLog);
#ifdef ACCEPT_LOG_LEVEL3
    LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
	//////////////////////////////////////////////////////////////
	//FolderCleanup  // 作業フォルダのクリーニング
#ifndef TUNING1
	RecoverFolder("RST");
	RecoverFolder("MSG");
	RecoverFolder("RCP");
	RecoverFolder("TMP");
#else
	RecoverFolder("RST");
	RecoverFolder("MS$");
	RecoverFolder("RC$");
	RecoverFolder("TM$");
#endif
#ifdef UPDATE_20090306 // TEMPフォルダのデータ残骸起動時クリア
	RecoverFolder("RCX");
	RecoverFolder("RCB");
	RecoverFolder("MSB");
#endif
#ifdef MILTER_ON // MILTERインターフェースを追加。
	RecoverFolder("MFG");
	RecoverFolder("MEG");
	RecoverFolder("MHG");
	RecoverFolder("MMG");
	RecoverFolder("RHP");
	RecoverFolder("RRP");
	RecoverFolder("RFP");
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

    AddToMessageLog(TEXT(SMTP_NAME), 0, TEXT(VERSION), (WORD)EVENTLOG_INFORMATION_TYPE);
    // init the overlapped structure
    //
    memset( &os, 0, sizeof(OVERLAPPED) );
    os.hEvent = hEvents[1];
    ResetEvent( hEvents[1] );
	dwc = 0;
    _tzset();
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
		  if (SMTPRSMain() == FALSE) {
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
#ifdef UPDATE_20091120 // ML保存時のindex.html作成のための排他処理、64bit長のファイルサイズに対応
    DeleteCriticalSection(&g_csMLIndex);
#endif
#ifdef UPDATE_20080929A // ログのクリティカルセクション化
    //////////////////////////////////////////////
    // ログ出力の排他フラグ削除
    DeleteCriticalSection(&g_csWriteReport);
#endif

}

BOOL FillAddr(PSOCKADDR_IN psin, char *mSVR) {
   PHOSTENT phe = NULL;
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
 	 if (bDebug) printf("not found '%s'.(Code %d)\n", mSVR, WSAGetLastError());
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

	//ReportStatusToSCMgr( SERVICE_STOP_PENDING, NO_ERROR, 5000);
    if ( hServerStopEvent )
      SetEvent(hServerStopEvent);
	bServiceTerminating = TRUE;
	memset(mListenIP, 0, sizeof(mListenIP));
#ifdef K_SEARCH // K_SEARCH OEM 版 //KTEC SMTP Archive Receiver
    dwc = GetProfileStringEx(SOFT_REG,"ListenIP","", mListenIP, sizeof(mListenIP));
#else
    dwc = GetProfileStringEx(SYSTEM_REG,"ListenIP","", mListenIP, sizeof(mListenIP));
#endif // K_SEARCH OEM 版 //KTEC SMTP Archive Receiver
    memset(&mListenIP[dwc], 0, (sizeof(mListenIP)-dwc));
	nport = 25;
#ifdef K_SEARCH // K_SEARCH OEM 版 //KTEC SMTP Archive Receiver
    nport = GetProfileIntEx(SOFT_REG, "PortNo", (int)25);
#else
#ifndef E_POST
    nport = GetProfileIntEx("SYSTEM\\CurrentControlSet\\Services\\SMTPRS\\Parameters", "PortNo", (int)25);
#endif
	if (nport == 25)
      nport = GetProfileIntEx(SYSTEM_PARAM_REG, "PortNo", (int)25);
#endif
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
	  //itoa(nport, mport, 10);
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
    if (bDebug) printf("sock connected.\n");
#endif

    return;
}
