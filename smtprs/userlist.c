////////////////////////////////////////////////////////////
// Userlist.c Copyright K.kawakami
////////////////////////////////////////////////////////////
#define UNICODE 1
#include "smtp.h"
#include <windows.h>
#include <lmcons.h>
#include <lmaccess.h>
#include <lmerr.h>
#include <lmapibuf.h>
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>
#include <locale.h>
#include "profile.h"
///////////////////////////
extern int errno;
extern BOOL bDebug;
#ifdef UPDATE_20080326 // ドメイン無しアカウントをローカルとしての利用有無選択機能追加
extern BOOL bHaveDomain;
#endif
#ifdef V3
extern BOOL  bUserMan;
typedef struct _User_DB {
    char        account[256];  // ユーザー名
    char        password[256]; // パスワード
	char        fullname[256]; // フルネーム
	char        domain[256];   // ドメイン名
	char        home[256];     // ホームフォルダ
};
#endif
///// debug ///////////////
extern BOOL  bAcceptLog;
extern char  mMailSpoolDir[];
extern CHAR  mMailBoxDir[];
///// LOG ////////////////
extern char  mLog[2048];
///////////////////////////
extern char mAuthDomain[];
extern int  nAddressFamily;
extern int   nConnectAF;
extern char mLocalDomain[];
extern DWORD nLocalDomain;
extern char mMailGroup[];
//extern BOOL  bPOPbeforeSMTP;
#ifdef UPDATE_20091026 // AD連携の問い合わせリトライ間隔を短く設定しなおした
extern DWORD  nLDAPRetryMSec;
extern DWORD  nADRetryMSec;
#endif
#ifdef UPDATE_20060606 // ドメインコントローラへの接続リトライ
extern DWORD  nADRetryTime;
#endif

#ifdef LDAP_ON         // LDAPへのログイン処理でSMTP認証の確認を行うオプション // //CRAM-MD5は複合できないので、これを利用する場合は、PLAINかLOGINのみとします。
extern BOOL   bLDAP;
extern BOOL   bLDAPLicense;
extern DWORD  nLDAPPort;
extern char   mLDAPAdminID[];
extern char   mLDAPAdminPW[];
extern char   mLDAPSchemaRDN[];
extern char   mLDAPSchemaDC[];
extern char   mLDAPSchemaFilter[];
extern char   mLDAPSchemaID[];
extern char   mLDAPSchemaHome[];
extern char   mLDAPSchemaName[];
extern char   mLDAPSchemaMember[];
int  LDAP_UserList(LDAPD *ldapd); // 一覧
#endif
#ifdef UPDATE_20070521 // OSの予約語対策
extern CHAR   mReservedWords[];
#endif
#ifdef ADD_IDCACHE
extern int  nIDCashLiveTime; // ADキャッシュ利用有効時間(秒単位)
#endif

#ifdef ACCEPT_LOG_LEVEL3
void LEVEL_3_ACCEPTLOG(PCLIENT_CONTEXT lpClientContext, char *mess);
#endif
NET_API_STATUS confirms_user(char *Domain, char *user, char *myaddr, struct _User_DB *userdb, DWORD *total);
//BOOL GetLockFile(char  *pUser, char *myaddr);

#ifdef V3
NET_API_STATUS UserHomeDir( CHAR *lpszContry,
					        CHAR *lpszDomain,
                            CHAR *lpszUser,
							CHAR *lpszUserDomain,
							CHAR *lpszHomeDir)
#else
NET_API_STATUS UserHomeDir( CHAR *lpszContry,
					        CHAR *lpszDomain,
                            CHAR *lpszUser,
							CHAR *lpszHomeDir)
#endif
{
#ifdef ACCEPT_LOG_LEVEL3
    char str[LOG_BUFFER];
#endif
    LPWSTR          lpwszPrimaryDC = NULL;
    NET_API_STATUS  err = 0;
    USER_INFO_11    *user_info11;
	LPWSTR          lpwszDomain;
    wchar_t         wszDomain[24], wszUser[24];
	NET_API_STATUS nSts;
    wchar_t wDom[65];
	LPBYTE  pbuff;
#ifdef V3
	struct _User_DB userdb;
	DWORD  totalentries;
#endif
#ifdef UPDATE_20060606 // ドメインコントローラへの接続リトライ
	int    nTry;
#endif
#ifdef LDAP_ON         // LDAPへのログイン処理でSMTP認証の確認を行うオプション // //CRAM-MD5は複合できないので、これを利用する場合は、PLAINかLOGINのみとします。
	CHAR   *p, mFilter[256], mScope[256], mNULL[] = "";
    CHAR   mKey[256], mDN[256];
    LDAPD  ldapd;
#endif
#ifdef ADD_IDCACHE
	BOOL   bHit = FALSE;
	BOOL   bCache = FALSE;
#endif

	*lpszHomeDir = '\x0'; // home dir 初期化
#ifdef UPDATE_20091014B // LDAPでHOMEディレクトリ参照時に問合せアカウントがNULLの場合、リトライがかかり応答に時間がかかる不具合
	if (!*lpszUser)
	{
	  return (err);
	}
#endif

#ifdef V3
  if (bUserMan) {
#endif
#ifdef LDAP_ON         // LDAPへのログイン処理でSMTP認証の確認を行うオプション // //CRAM-MD5は複合できないので、これを利用する場合は、PLAINかLOGINのみとします。
#ifdef LDAP_OPTION_LICENSE // LDAP Optionのライセンスチェックを行なう
	if (bLDAP && bLDAPLicense) 
#else
	if (bLDAP) 
#endif
	{
#ifdef ADD_IDCACHE
      if (nIDCashLiveTime > 0)  // 0 以上
	  {
        if ((bCache = GetIDCashList(lpszUser, lpszHomeDir, "hm", &bHit)))
		{
		  err = (bHit ? NERR_Success : 2221);
#ifdef ACCEPT_LOG_LEVEL3
          sprintf(str, "LogonUser(%s) Cache Success.", lpszUser);
		  LEVEL_3_ACCEPTLOG(NULL, str);
#endif
		}
	  }
	  if (!bCache)
#endif
	  {
#ifdef UPDATE_20070620 // ドメインごとにDNを設定可能にする
#ifdef UPDATE_20070521 // 予約語対策
      strcpy(mKey, lpszUserDomain);
      strtok(mKey, "@");
      if (ReservedWords(mKey)) {
        strcpy(mKey, mReservedWords);
        strcat(mKey, lpszUserDomain);
	  } else
#endif
	  {
        strcpy(mKey, lpszUserDomain);
	  }
 	  GetProfileStringEx((LPCTSTR)DOMAIN_BASEDN, (char *)mKey, "", mDN, sizeof(mDN)); // 対象ドメインのDN名
#ifdef UPDATE_20091014 // 深い階層のアカウントが読めるように対策
      strcpy(mScope, (mDN[0] ? mDN : mLDAPSchemaDC));
#else
      sprintf(mScope, "%s=%s,%s", mLDAPSchemaRDN, lpszUser, (mDN[0] ? mDN : mLDAPSchemaDC));
#endif
#else
      sprintf(mScope, "%s=%s,%s", mLDAPSchemaRDN, lpszUser, mLDAPSchemaDC);
#endif
	  ldapd.pHost = (mAuthDomain[0] ? mAuthDomain : "localhost");
	  ldapd.nPort = nLDAPPort;
      ldapd.pLogonDomain = mNULL;
	  ldapd.pLogonID = mLDAPAdminID;
	  ldapd.pLogonPW = mLDAPAdminPW;
	  ldapd.pScope = mScope;
	  strcpy(mFilter, mLDAPSchemaFilter);
      p = &mFilter[strlen(mFilter)-2];
	  if (!strcmp(p, "))")) {
		p++;
		*p = '\x0';
		sprintf(p, "(%s=%s))", mLDAPSchemaID, lpszUser);
	  }
	  ldapd.pRequest1 = mFilter; //lpszUser;
      ldapd.pRequest2 = mLDAPSchemaHome;
	  ldapd.pAnswer   = lpszHomeDir;
#ifdef UPDATE_20071016 // LDAPサーバーへの接続リトライ
	  nTry = 0;
	  do {
#ifdef UPDATE_20091014C // LDAPへのアカウント問合せ処理の高速化
		nSts = LDAP_UserList(&ldapd);
		if (nSts == 1)         // 接続成功、アカウント情報有り
		{ 
		  err = NERR_Success;
#ifdef ADD_IDCACHE
          if (nIDCashLiveTime > 0)  // 0 以上
		  {
		    SetIDCashList(lpszUser, lpszHomeDir, "hm");
		  }
#endif
		  break;
		} else if (nSts == 2)  // 接続成功、アカウント情報無し
		{ 
		  err = 2221;
		  break;
		} else {               // 接続失敗、リトライ対象
		  err = -1; 
		}
#else
        nSts = (LDAP_UserList(&ldapd) == 1 ? NERR_Success : -1);
#endif
	    if (++nTry >= nADRetryTime) {// １０回までトライを試みる
	      break;
		}
		if (nSts != NERR_Success)
#ifdef UPDATE_20091026 // AD連携の問い合わせリトライ間隔を短く設定しなおした
		  Sleep(nLDAPRetryMSec);
#else
		  _sleep(1000);
#endif
	  } while (nSts != NERR_Success);
#else
      err = LDAP_UserList(&ldapd);
#endif
	  }
	} else
#endif
	{
#ifdef ADD_IDCACHE
      if (nIDCashLiveTime > 0)  // 0 以上
	  {
        if ((bCache = GetIDCashList(lpszUser, lpszHomeDir, "hm", &bHit)))
		{
		  err = (bHit ? NERR_Success : 2221);
#ifdef ACCEPT_LOG_LEVEL3
          sprintf(str, "LogonUser(%s) Cache Success.", lpszUser);
		  LEVEL_3_ACCEPTLOG(NULL, str);
#endif
		}
	  }
	  if (!bCache)
#endif
	  {
      if (lpszContry)
        setlocale(LC_ALL, lpszContry);
      lpwszDomain = NULL;
      if (lpszDomain) {
        mbstowcs(wszDomain,     lpszDomain,     24);
        lpwszDomain = wszDomain;
	  } else if (mAuthDomain[0]) {
        mbstowcs( wDom, mAuthDomain, 65);
#ifdef UPDATE_20060606 // ドメインコントローラへの接続リトライ
	    nTry = 0;
	    do {
#endif
 	      nSts = NetGetAnyDCName(NULL, 
                             wDom,
                             &pbuff);
	      if (nSts == NERR_Success) {
	        wcscpy(wszDomain, (const wchar_t *)pbuff);
		    lpwszDomain = wszDomain;
            NetApiBufferFree(pbuff);
		  }
#ifdef UPDATE_20060606 // ドメインコントローラへの接続リトライ
#ifdef ACCEPT_LOG_LEVEL3
          else {
		    sprintf(str, "NetGetAnyDCName(%s / %s) = ", mAuthDomain, lpszUser);
		    switch (nSts) {
		      case ERROR_NO_LOGON_SERVERS:          strcat(str, "ERROR_NO_LOGON_SERVERS\n"); break;
		      case ERROR_NO_SUCH_DOMAIN:            strcat(str, "ERROR_NO_SUCH_DOMAIN\n"); break;
		      case ERROR_NO_TRUST_LSA_SECRET:       strcat(str, "ERROR_NO_TRUST_LSA_SECRET\n"); break;
		      case ERROR_NO_TRUST_SAM_ACCOUNT:      strcat(str, "ERROR_NO_TRUST_SAM_ACCOUNT\n"); break;
              case ERROR_DOMAIN_TRUST_INCONSISTENT: strcat(str, "ERROR_DOMAIN_TRUST_INCONSISTENT\n"); break;
		    //case NERR_Success:                    strcat(str, "NERR_Success\n"); break;
		      default: sprintf(str, "NERR_Other %d\n", nSts); break;
			}
            LEVEL_3_ACCEPTLOG(NULL, str);
		  }
#endif
		  if (++nTry >= nADRetryTime) {// １０回までトライを試みる
		    break;
		  }
		  if (nSts != ERROR_NO_LOGON_SERVERS &&
		      nSts != ERROR_NO_SUCH_DOMAIN &&
			  nSts != ERROR_NO_TRUST_LSA_SECRET &&
			  nSts != ERROR_NO_TRUST_SAM_ACCOUNT &&
			  nSts != ERROR_DOMAIN_TRUST_INCONSISTENT &&
		  	  nSts != NERR_Success)
#ifdef UPDATE_20091026 // AD連携の問い合わせリトライ間隔を短く設定しなおした
		  Sleep(nADRetryMSec);
#else
		  _sleep(1000);
#endif
		} while (nSts != ERROR_NO_LOGON_SERVERS &&
		         nSts != ERROR_NO_SUCH_DOMAIN &&
			     nSts != ERROR_NO_TRUST_LSA_SECRET &&
			     nSts != ERROR_NO_TRUST_SAM_ACCOUNT &&
			     nSts != ERROR_DOMAIN_TRUST_INCONSISTENT &&
			     nSts != NERR_Success);
#endif
	  }
      mbstowcs(wszUser,       lpszUser,       24);

// 最初にプライマリドメインコントローラの名前を取得。
// 必ず、戻されたバッファを解放すること
#ifdef UPDATE_20060606 // ドメインコントローラへの接続リトライ
      nTry = 0;
      do {
#endif
        err = NetUserGetInfo( lpwszDomain, 
                           wszUser,
                           (DWORD)11,
                           (LPBYTE *)&user_info11); 
        switch ( err ) {
          case 0:
            //printf("user deleted.\n");
		    wcstombs( lpszHomeDir, user_info11->usri11_home_dir, 256 );
		    NetApiBufferFree(user_info11);
#ifdef ADD_IDCACHE
            if (nIDCashLiveTime > 0)  // 0 以上
			{
			  SetIDCashList(lpszUser, lpszHomeDir, "hm");
			}
#endif
            break;
          default:
            //printf("user not found.\n");
            break;
		}
#ifdef UPDATE_20060606 // ドメインコントローラへの接続リトライ
#ifdef ACCEPT_LOG_LEVEL3
		{
	      sprintf(str, "NetGetAnyDCName(%s / %s) = ", mAuthDomain, lpszUser);
	      switch (err) {
	        case ERROR_ACCESS_DENIED:          strcat(str, "ERROR_ACCESS_DENIED\n"); break;
	        case NERR_InvalidComputer:            strcat(str, "NERR_InvalidComputer\n"); break;
		    case NERR_UserNotFound:       strcat(str, "NERR_UserNotFound\n"); break;
		    case NERR_Success:                    strcat(str, "NERR_Success\n"); break;
		    default: sprintf(str, "NERR_Other %d\n", err); break;
		  }
          LEVEL_3_ACCEPTLOG(NULL, str);
		}
#endif
	    if (++nTry >= nADRetryTime) {// １０回までトライを試みる
	      break;
		}
	    if (err != NERR_Success &&
	        err != ERROR_ACCESS_DENIED &&
		    err != NERR_InvalidComputer &&
		    err != NERR_UserNotFound)
#ifdef UPDATE_20091026 // AD連携の問い合わせリトライ間隔を短く設定しなおした
		  Sleep(nADRetryMSec);
#else
		  _sleep(1000);
#endif
	  } while (err != NERR_Success &&
	           err != ERROR_ACCESS_DENIED &&
		       err != NERR_InvalidComputer &&
		       err != NERR_UserNotFound);
#endif
    }
	}
#ifdef V3
  } else {
	  err = confirms_user( lpszUserDomain,
		                   lpszUser,
						   "", 
						   (struct _User_DB *)&userdb, 
						   (LPDWORD)&totalentries );
	  if (err == 0)
        strcpy(lpszHomeDir, userdb.home);
  }
#endif
    return( err );
}

BOOL CheckLocalServer(char *mDom, char *mTblDom) {
  BOOL sts = FALSE;
  char *pd;
  int  i, j;

  if (*mDom == '\x0' || *mTblDom == '\x0')
	return sts;

  i = strlen(mTblDom);
  j = strlen(mDom);
  if (i < j) {
    pd = (char *)(mDom + (j-i));
    if (stricmp(pd, mTblDom) == 0)
      sts = TRUE;
  }
  return sts;
}

#ifdef UPDATE_20070509 // エイリアス設定の実アドレスオプションを別データとして処理する
BOOL CheckUser(char *user, char *pOpt, char *myaddr, BOOL *bLocal, BOOL *bSubLocal, BOOL *bMList, BOOL *bOutSideAliases, LPBYTE fullname)
#else
BOOL CheckUser(char *user, char *myaddr, BOOL *bLocal, BOOL *bSubLocal, BOOL *bMList, BOOL *bOutSideAliases, LPBYTE fullname)
#endif
{
#ifdef ACCEPT_LOG_LEVEL3
    char str[LOG_BUFFER];
#endif
	NET_API_STATUS sts = 0;
	NET_API_STATUS nSts;
    DWORD entriesread, entries; 
	DWORD totalentries; 
    LPLOCALGROUP_USERS_INFO_0   user_info[1];
    LPUSER_INFO_10      uinfo[1];
	BOOL  bsts = FALSE, bdomsts = FALSE, bRequest = FALSE, bSubDom = FALSE;
	BOOL  bGroup = FALSE, bCheck = FALSE, bUser = FALSE;
	BOOL  bAliases = FALSE;
	LPBYTE  pbuff;
	wchar_t wDom[65], wAuthServer[65], wuser[21], wmember[21];
	char   member[256] = {""}, *pdom, *pldom, fname[256], mAuthServer[65];
	char   machine[256];
    PHOSTENT host[2] = {NULL, NULL};
	struct in_addr *inDom, *inlDom;
	char mDom[32], mlDom[32], *pDom, *pllDom; //, *qpdom = NULL;
	DWORD naliases;
#ifdef IPv6
	BOOL bhost1 = FALSE;
	char Out6[INET6_ADDRSTRLEN];
    int  Error;
    SOCKADDR_IN6 sin6;
    SOCKADDR_IN  sin;
    struct in_addr addr4;
    struct in6_addr addr6, *inl6Dom;
#endif
#ifdef V3
	struct _User_DB userdb;
#endif
#ifdef V4
	BOOL   bValidTime = TRUE;
#endif
	char   mIP[256];
#ifdef UPDATE_20060606 // ドメインコントローラへの接続リトライ
	int    nTry;
#endif
#ifndef VC6
  HOSTENT hst;
  struct addrinfo Hints, *AddrInfo;
#endif
#ifdef UPDATE_20070521 // 予約語対策
    CHAR mKey[256];
#endif
	
#ifdef K_SEARCH // K_SEARCH OEM 版
	  *bLocal = TRUE;  // '@'がない場合は内部ドメインと判定
	  return TRUE;
#endif
#ifdef LDAP_ON         // LDAPへのログイン処理でSMTP認証の確認を行うオプション // //CRAM-MD5は複合できないので、これを利用する場合は、PLAINかLOGINのみとします。
	CHAR   *p, mNULL[] = "";
	CHAR   mScope[256], mFilter[256];
    CHAR   mDN[256];
    LDAPD  ldapd;
#endif
#ifdef ADD_IDCACHE
	BOOL   bHit = FALSE;
	BOOL   bCache = FALSE;
    BOOL   bLock  = FALSE;
#endif

#ifdef UPDATE_200500902 // ドメインが空欄だと、メモリが初期化されないままドメインとなる
#ifdef V3
	userdb.account[0] = '\x0';
	userdb.domain[0] = '\x0';
	userdb.fullname[0] = '\x0';
	userdb.home[0] = '\x0';
	userdb.password[0] = '\x0';
#endif
#endif
    /// User Local Domain ? /////
	//printf("Check %s address = %s\n", (bSubLocal ? "RCPT TO" : "MAIL FROM"), user);
	sprintf(mLog, "Check %s address = %s", (bSubLocal ? "RCPT TO" : "MAIL FROM"), user);
	if (bDebug) printf("%s\n", mLog);
#ifdef ACCEPT_LOG_LEVEL3
    LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
    /// アカウントＤＢチェックより先にエイリアスチェック
	/* 
	// "postmaster"はデフォルトでは無効にする。
	pdom = strstr(user, "@");
	if (pdom)
	  *pdom = '\x0';
    bsts = GetPostMaster((char *)user, (char *)pdom);
	if (bsts) {
      if (bDebug) printf("\t-> postmaster = %s\n", user);
	} else {
	  if (pdom)
	    *pdom = '@';
	}
	*/
	pdom = strstr(user, "@");
	if (pdom) {
	  *pdom = '\x0';
	}
    //bsts = GetAliases((LPCTSTR)SOFT_ALIASES_REG, (char *)user, (char *)pdom);
#ifdef UPDATE_20070509 // エイリアス設定の実アドレスオプションを別データとして処理する
	bAliases = GetAliases((LPCTSTR)SOFT_ALIASES_REG, (char *)user, (char *)pdom, pOpt);
#else
	bAliases = GetAliases((LPCTSTR)SOFT_ALIASES_REG, (char *)user, (char *)pdom);
#endif
	//if (bsts) {
	if (bAliases) {
      //printf("\t-> aliases = %s\n", user);
      sprintf(mLog, "\t-> aliases = %s", user);
	  if (bDebug) printf("%s\n", mLog);
#ifdef ACCEPT_LOG_LEVEL3
      LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
	} else {
	  if (pdom)
	    *pdom = '@';
	}
	bsts = FALSE;
    ///////////////////////////////////////////////////
	pdom = strstr(user, "@");
	if (pdom) {
	  //strncpy((char *)dom, (char *)(pdom+1), 256);
	  *pdom = '\x0';
      pldom = mLocalDomain;
      _strlwr(pdom+1);
   	  do {
        _strlwr(pldom);
		if (bDebug) printf("domain check status = \"%s\" vs. \"%s\"", (pdom+1), pldom);
		/*
        ////// Accept Log の収集 ///////
	    if (bAcceptLog) {
   	      FILE   *Logfp;
          char   mAcptLogFn[256];
	      sprintf(mAcptLogFn, "%sacceptlog\\debug.log", mMailSpoolDir);
#ifdef UPDATE_20260603 // ログ出力の排他処理化を追加
          WaitForSingleObject(g_hMutexAcceptLog, INFINITE);  // 排他開始
#endif
          Logfp = fopen(mAcptLogFn, "at");
          if (Logfp) {
	          fprintf(Logfp, "domain check status = \"%s\" vs. \"%s\"\n", (pdom+1), pldom);
  		      fclose(Logfp);
		  }
#ifdef UPDATE_20260603 // ログ出力の排他処理化を追加
          ReleaseMutex(g_hMutexAcceptLog);  // 排他終了
#endif
		}
		*/
        //////////////////////////////////
		if (pldom[0] =='\x0') {
	      //printf(" unmatch\n");
          sprintf(mLog, " unmatch");
	      if (bDebug) printf("%s\n", mLog);
#ifdef ACCEPT_LOG_LEVEL3
          LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
		} else if(strcmp((pdom+1), pldom) == 0) {
#ifdef V3
	      GetProfileStringEx((LPCTSTR)DOMAIN_SMTPIP, (LPCTSTR)(pdom+1), (LPCTSTR)"", (LPTSTR)mIP, (DWORD)sizeof(mIP)); // 対象ドメインの応答ＩＰアドレス取出し
#ifdef UPDATE_20070521 // 予約語対策
          strcpy(mKey, pdom+1);
          strtok(mKey, "@");
          if (ReservedWords(mKey)) {
            strcpy(mKey, mReservedWords);
	        strcat(mKey, pdom+1);
	        GetProfileStringEx((LPCTSTR)DOMAIN_SMTPIP, (LPCTSTR)mKey, (LPCTSTR)"", (LPTSTR)mIP, (DWORD)sizeof(mIP)); // 対象ドメインの応答ＩＰアドレス取出し
		  }
#endif
          //printf("\nPermission address check=(\"%s:%s\" vs. \"%s\") ", (pdom+1), mIP, myaddr);
          sprintf(mLog, "Permission address check=(\"%s:%s\" vs. \"%s\") ", (pdom+1), mIP, myaddr);
	      if (bDebug) printf("%s\n", mLog);
#ifdef ACCEPT_LOG_LEVEL3
          LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
#ifdef CLUSTERING
	      if (mIP[0] != '\x0' && !strstr(mIP, myaddr)) // 対象のドメインは接続IPで許可されたものであるか？
#else
	      if (mIP[0] != '\x0' && strcmp(myaddr, mIP) != 0) // 対象のドメインは接続IPで許可されたものであるか？
#endif
		  {
	        //printf("unmach\n");
            sprintf(mLog, " unmach");
	        if (bDebug) printf("%s\n", mLog);
#ifdef ACCEPT_LOG_LEVEL3
            LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
	        break;
		  }
#endif
          *bLocal = TRUE;
	      //printf(" match!!\n");
          sprintf(mLog, " match!!");
	      if (bDebug) printf("%s\n", mLog);
#ifdef ACCEPT_LOG_LEVEL3
          LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
		  break;
		} else if (CheckLocalServer(pdom+1, pldom)) {
#ifdef IPv6
          Error = 0;
	      bhost1 = FALSE;
#ifdef VC6
          host[1] = getipnodebyname(pldom, (nConnectAF ? AF_INET6 : AF_INET),  (nConnectAF ? AI_ALL : 0), &Error);
#else
          host[1] = NULL;
          memset(&Hints, 0, sizeof(Hints));
		  Hints.ai_flags = AI_NUMERICHOST | AI_PASSIVE;
          Hints.ai_family =  (nConnectAF ? AF_INET6 : AF_INET);
          Hints.ai_socktype = SOCK_STREAM;
          if (getaddrinfo(pldom, NULL, &Hints, &AddrInfo) == 0) {
            hst.h_addr = (struct sockaddr_in *)(AddrInfo->ai_addr);
 	        host[1] = &hst;
          }
#endif

#else
          host[1] = gethostbyname(pldom);
#endif
 	      mlDom[0] = '\x0';
	      inlDom = NULL;
		  machine[0] ='\x0';
		  if (host[1]) {
#ifdef IPv6
            inl6Dom = NULL;
#endif
			inlDom = NULL;
			if (host[1]->h_addr) {
#ifdef IPv6
			  if (nConnectAF) {
                inl6Dom = (struct in6_addr *)host[1]->h_addr_list[0];
			  } else {
                inlDom = (struct in_addr *)host[1]->h_addr_list[0];
			  }
			  if (nConnectAF && inl6Dom ||
				  !nConnectAF && inlDom)
#else
              inlDom = (struct in_addr *)host[1]->h_addr; //h_addr_list[0];
			  if (inlDom)
#endif
			  {
				pllDom = NULL;
                //strcpy(mlDom, inet_ntoa(*inlDom));
#ifdef IPv6
			   //LocalFree(host[1]);
			   if (nConnectAF)
                 (const char *)pllDom = inet_ntop(AF_INET6, inl6Dom, Out6, INET6_ADDRSTRLEN); // IPv6
			   else
                 (const char *)pllDom = inet_ntop(AF_INET, inlDom, Out6, INET_ADDRSTRLEN);     // IPv4
#else
			    pllDom = inet_ntoa(*inlDom);
#endif
				if (pllDom)
                  strcpy(mlDom, pllDom);
			    //else
                  //mlDom[0] = '\x0';
#ifdef IPv6
                if (LocalFlags(host[1]) != LMEM_INVALID_HANDLE) // 解放可能かどうか確認
#ifdef VC6
			      LocalFree(host[1]);
#else
                  freeaddrinfo(AddrInfo);
#endif
                Error = 0;
  			    bhost1 = FALSE;
		        //if (nAddressFamily) {
				host[1] = NULL;
			    if (mlDom[0] != '\x0') {
				  if (nConnectAF) {
#ifdef VC6
	                inet6_addr(mlDom, &addr6);
#else
 		            inet_pton(AF_INET6, mlDom, &addr6);
#endif
                    memcpy(&sin6.sin6_addr, &addr6, sizeof(struct in6_addr));
#ifdef VC6
	                host[1] = getipnodebyaddr(&sin6.sin6_addr, sizeof(sin6.sin6_addr), AF_INET6, &Error);
#else
                    host[1] = NULL;
                    memset(&Hints, 0, sizeof(Hints));
					Hints.ai_flags = AI_NUMERICHOST | AI_PASSIVE;
                    Hints.ai_family = AF_INET6;
                    Hints.ai_socktype = SOCK_STREAM;
                    if (getaddrinfo(&sin6.sin6_addr, NULL, &Hints, &AddrInfo) == 0) {
                      hst.h_name = AddrInfo->ai_canonname;
			          host[1] = &hst;
                    }
#endif
				  } else {
	                *(unsigned long *)&addr4 = inet_addr(mlDom);
					memcpy(&sin.sin_addr, &addr4, sizeof(struct in_addr));
#ifdef VC6
                    host[1] = getipnodebyaddr(&sin.sin_addr, sizeof(sin.sin_addr), AF_INET, &Error);
#else
                    host[1] = NULL;
                    memset(&Hints, 0, sizeof(Hints));
					Hints.ai_flags = AI_NUMERICHOST | AI_PASSIVE;
                    Hints.ai_family = AF_INET;
                    Hints.ai_socktype = SOCK_STREAM;
                    if (getaddrinfo(&sin.sin_addr, NULL, &Hints, &AddrInfo) == 0) {
                      hst.h_name = AddrInfo->ai_canonname;
			          host[1] = &hst;
                    }
#endif
				  }
				}
#else
			    host[1] = gethostbyaddr(host[1]->h_addr, 4, PF_INET);
#endif
			    if (host[1]) { // 逆引きが出来ない場合どうするか？
				  if (host[1]->h_name) {
			        strcpy(machine,host[1]->h_name); 
				  }
#ifdef IPv6
				  bhost1 = TRUE;
#ifdef UPDATE_20060515 // SOCKET関連のメモリ開放抜け
                  if (host[1]) {
#ifdef VC6
                    freehostent(host[1]);
#else
                    freeaddrinfo(AddrInfo);
#endif
					host[1] = NULL;
				  }
#else
                  if (LocalFlags(host[1]) != LMEM_INVALID_HANDLE) // 解放可能かどうか確認
				    LocalFree(host[1]);
#endif
#endif
				}
			  }
			}
		  } 
#ifdef IPv6
		  if (bhost1)
#else
          if (!host[1])
#endif
		  {
	        //printf(" unmatch\n A host address wasn't found from DNS Server. (%s)\n", pldom);
            sprintf(mLog, " unmatch\n A host address wasn't found from DNS Server. (%s)", pldom);
	        if (bDebug) printf("%s\n", mLog);
#ifdef ACCEPT_LOG_LEVEL3
            LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
/*
		  } else if (bSubLocal) { // Recipientのみ確認
		  //} else if (host[0] && bSubLocal){ // Recipientのみ確認
 	        mDom[0] = '\x0';
		    inDom = NULL;
			host[0] = gethostbyname(pdom+1);
			if (host[0]) {
			  if (host[0]->h_addr) {
  	              inDom = (struct in_addr *)host[0]->h_addr; //h_addr_list[0];
			    if (inDom)  {
	              //strcpy(mDom, inet_ntoa(*inDom));
			      pDom = inet_ntoa(*inDom);
			      if (pDom)
                    strcpy(mDom, pDom);
			      else
                    mDom[0] = '\x0';
				}
			  }
			  if (inlDom && inDom) {
			    if (strcmp(mlDom, mDom) != 0) {
  			      bSubDom = *bSubLocal= TRUE; // サブドメインは中継可
                  bsts = TRUE;
				}
                *bLocal = TRUE;
			  }
			} else {
	          if (bDebug) printf(" unmatch\n A host address wasn't found from DNS Server. (%s)\n", pdom+1);
			}
*/
		  //} else if (strcmp(host[1]->h_name, pldom) == 0) {
		  //} else if (strcmp(machine, pldom) == 0) {
/*
		  } else if (stricmp(machine, pdom+1) == 0) {
            *bLocal = TRUE;
*/
		  } else {
 	        //mDom[0] = '\x0';
		    inDom = NULL;
#ifdef IPv6
            Error = 0;
#ifdef VC6
            host[0] = getipnodebyname(pdom+1, (nConnectAF ? AF_INET6 : AF_INET),  (nConnectAF ? AI_ALL : 0), &Error);
#else
            host[0] = NULL;
            memset(&Hints, 0, sizeof(Hints));
			Hints.ai_flags = AI_NUMERICHOST | AI_PASSIVE;
            Hints.ai_family =  (nConnectAF ? AF_INET6 : AF_INET);
            Hints.ai_socktype = SOCK_STREAM;
            if (getaddrinfo(pdom+1, NULL, &Hints, &AddrInfo) == 0) {
              hst.h_addr = (struct sockaddr_in *)(AddrInfo->ai_addr);
 	          host[0] = &hst;
           }
#endif
#else
			host[0] = gethostbyname(pdom+1);
#endif
			if (host[0]) {
			  naliases = 0; // IPアドレスが複数設定されている場合のチェック
			  while(host[0]->h_addr_list[naliases]) {
	            //printf("\n%s (", pdom+1);
                sprintf(mLog, "\n%s (", pdom+1);
	            if (bDebug) printf("%s\n", mLog);
#ifdef ACCEPT_LOG_LEVEL3
                LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
				mDom[0] = '\x0';
			    inDom = (struct in_addr *)host[0]->h_addr_list[naliases];
			    if (inDom)  {
#ifdef IPv6
                 (const char *)pDom = inet_ntop(AF_INET, inDom, Out6, INET_ADDRSTRLEN);     // IPv4
                 if (!pDom)
                   (const char *)pDom = inet_ntop(AF_INET6, inDom, Out6, INET6_ADDRSTRLEN); // IPv6
#else
			      pDom = inet_ntoa(*inDom);
#endif
				  if (pDom)
                    strcpy(mDom, pDom);
				}
                //printf("%s) ", mDom);
                sprintf(mLog, "%s) ", mDom);
	            if (bDebug) printf("%s\n", mLog);
#ifdef ACCEPT_LOG_LEVEL3
                LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
			    if (inlDom && inDom) {
				//if (mlDom[0] != '\x0' && mDom[0] != '\x0') {
				  if (!bSubLocal) {
			        if (stricmp(mlDom, mDom) == 0) { // 同一マシンのエイリアス
                      *bLocal = TRUE;
			  	      break;
					}
				  } else {
			        if (stricmp(mlDom, mDom) != 0) {
  			          bsts = bSubDom = *bSubLocal= TRUE; // サブドメインは中継可
					} else {
  			          bsts = bSubDom = *bSubLocal= FALSE; // サブドメインは中継可
                      *bLocal = TRUE;
					  break;
					}
				  }
				}
				naliases++;
			  }
#ifdef IPv6
#ifdef UPDATE_20060515 // SOCKET関連のメモリ開放抜け
              if (host[0]) {
#ifdef VC6
                freehostent(host[0]);
#else
                freeaddrinfo(AddrInfo);
#endif
				host[0] = NULL;
			  }
#else
              if (LocalFlags(host[0]) != LMEM_INVALID_HANDLE) // 解放可能かどうか確認
			    LocalFree(host[0]);
#endif
#endif
			} else {
              //printf("\n%s (\?\?\?.\?\?\?.\?\?\?.\?\?\?)", pdom+1);
              sprintf(mLog, "\n%s (\?\?\?.\?\?\?.\?\?\?.\?\?\?)", pdom+1);
	          if (bDebug) printf("%s\n", mLog);
#ifdef ACCEPT_LOG_LEVEL3
              LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
			}
            if (!*bLocal) {
			  //printf(" vs %s unmatch.\n", machine[0] != '\x0' ? machine : "(??? host name)");
              sprintf(mLog, " vs %s unmatch.\n", machine[0] != '\x0' ? machine : "(??? host name)");
	          if (bDebug) printf("%s\n", mLog);
#ifdef ACCEPT_LOG_LEVEL3
              LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
			}
		  }
		  if (*bLocal) {
		    //printf(" vs %s match!!\n", machine[0] != '\x0' ? machine : "(??? host name)");
            sprintf(mLog, " vs %s match!!", machine[0] != '\x0' ? machine : "(??? host name)");
	        if (bDebug) printf("%s\n", mLog);
#ifdef ACCEPT_LOG_LEVEL3
            LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
		    break;
		  }
		} else {
	      //printf(" unmatch\n");
          sprintf(mLog, " unmatch");
          if (bDebug) printf("%s\n", mLog);
#ifdef ACCEPT_LOG_LEVEL3
          LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
		}
        pldom += strlen(pldom);
		pldom++;
	  } while (strlen(pldom));
	} else {
	  *bLocal = TRUE;  // '@'がない場合は内部ドメインと判定
	}
    if (strlen(user) == 0) { // ユーザー名が空白なら、外部ドメインと判定
	  *bLocal = FALSE; 
	} 
	*fullname = '\x0';
	if (*bLocal && !bSubDom) {
	  entriesread =  0;
	  totalentries = 0;
#ifdef V3
	  if (bUserMan)
#endif
	  {
#ifdef LDAP_ON         // LDAPへのログイン処理でSMTP認証の確認を行うオプション // //CRAM-MD5は複合できないので、これを利用する場合は、PLAINかLOGINのみとします。
#ifdef LDAP_OPTION_LICENSE // LDAP Optionのライセンスチェックを行なう
	    if (bLDAP && bLDAPLicense) 
#else
	    if (bLDAP) 
#endif
	    {
#ifdef ADD_IDCACHE
	      if (nIDCashLiveTime > 0)  // 0 以上
		  {
            bCache = GetIDCashList(user, mMailGroup, "gp", &bHit);
		    sts = (bHit ? NERR_Success : 2221);
            bGroup = bHit;
		  }
	      if (!bCache)
#endif
		  {
#ifdef UPDATE_20070620 // ドメインごとにDNを設定可能にする
		  mDN[0] = '\x0';
		  if (pdom) {
#ifdef UPDATE_20070521 // 予約語対策
            strcpy(mKey, pdom+1);
            strtok(mKey, "@");
            if (ReservedWords(mKey)) {
              strcpy(mKey, mReservedWords);
              strcat(mKey, pdom+1);
			} else
#endif
			{
              strcpy(mKey, pdom+1);
			}
 	        GetProfileStringEx((LPCTSTR)DOMAIN_BASEDN, (char *)mKey, "", mDN, sizeof(mDN)); // 対象ドメインのDN名
		  }
#ifdef UPDATE_20091014 // 深い階層のアカウントが読めるように対策
          strcpy(mScope, (mDN[0] ? mDN : mLDAPSchemaDC));
#else
          sprintf(mScope, "%s=%s,%s", mLDAPSchemaRDN, user, (mDN[0] ? mDN : mLDAPSchemaDC));
#endif
#else
          sprintf(mScope, "%s=%s,%s", mLDAPSchemaRDN, user, mLDAPSchemaDC);
#endif
	      ldapd.pHost = (mAuthDomain[0] ? mAuthDomain : "localhost");
	      ldapd.nPort = nLDAPPort;
          ldapd.pLogonDomain = mNULL;
	      ldapd.pLogonID = mLDAPAdminID;
	      ldapd.pLogonPW = mLDAPAdminPW;
	      ldapd.pScope = mScope;
	      strcpy(mFilter, mLDAPSchemaFilter);
          p = &mFilter[strlen(mFilter)-2];
	      if (!strcmp(p, "))")) {
		    p++;
		    *p = '\x0';
		    sprintf(p, "(%s=%s))", mLDAPSchemaID, user);
		  }
	      ldapd.pRequest1 = mFilter; //lpszUser;
          ldapd.pRequest2 = NULL;
	      ldapd.pAnswer   = NULL;
#ifdef UPDATE_20071016 // LDAPサーバーへの接続リトライ
	      nTry = 0;
	      do {
#ifdef UPDATE_20091014C // LDAPへのアカウント問合せ処理の高速化
			nSts = LDAP_UserList(&ldapd);
			if (nSts == 1)         // 接続成功、アカウント情報有り
			{ 
			  sts = NERR_Success;
#ifdef ADD_IDCACHE
             if (nIDCashLiveTime > 0)  // 0 以上
			 {
		       SetIDCashList(user, mMailGroup, "gp");
			 }
#endif
			  break;
			} else if (nSts == 2)  // 接続成功、アカウント情報無し
			{ 
			  sts = 2221;
			  break;
			} else {               // 接続失敗、リトライ対象
			  sts = -1; 
			}
#else
            nSts = (LDAP_UserList(&ldapd) == 1 ? NERR_Success : -1);
#endif
		    if (++nTry >= nADRetryTime) {// １０回までトライを試みる
		      break;
			}
		    if (nSts != NERR_Success)
#ifdef UPDATE_20091026 // AD連携の問い合わせリトライ間隔を短く設定しなおした
		      Sleep(nLDAPRetryMSec);
#else
		      _sleep(1000);
#endif
		  } while (nSts != NERR_Success);
#else
          sts = (LDAP_UserList(&ldapd) == 1 ? NERR_Success : -1);
#endif
		  }
		} else
#endif
		{
#ifdef UPDATE_20050512
		  strcpy(userdb.domain , mLocalDomain);
#endif
#ifdef ADD_IDCACHE
	      if (nIDCashLiveTime > 0)  // 0 以上
		  {
            bCache = GetIDCashList(user, mMailGroup, "gp", &bHit);
		    sts = (bHit ? NERR_Success : 2221);
            bGroup = bHit;
		  }
	      if (!bCache)
#endif
		  {
	      if (mAuthDomain[0]) {
            mbstowcs( wDom, mAuthDomain, 65);
#ifdef UPDATE_20060606 // ドメインコントローラへの接続リトライ
	        nTry = 0;
	        do
#endif
			{
		      nSts = NetGetAnyDCName(NULL, 
                                 wDom,
                                 &pbuff);
	          if (nSts == NERR_Success) {
		        wcscpy(wAuthServer, (const wchar_t *)pbuff);
                NetApiBufferFree(pbuff);
			  }
#ifdef UPDATE_20060606 // ドメインコントローラへの接続リトライ
#ifdef ACCEPT_LOG_LEVEL3
              else {
		        sprintf(str, "NetGetAnyDCName(%s / %s) = ", mAuthDomain, user);
		        switch (nSts) {
		          case ERROR_NO_LOGON_SERVERS:          strcat(str, "ERROR_NO_LOGON_SERVERS\n"); break;
		          case ERROR_NO_SUCH_DOMAIN:            strcat(str, "ERROR_NO_SUCH_DOMAIN\n"); break;
		          case ERROR_NO_TRUST_LSA_SECRET:       strcat(str, "ERROR_NO_TRUST_LSA_SECRET\n"); break;
		          case ERROR_NO_TRUST_SAM_ACCOUNT:      strcat(str, "ERROR_NO_TRUST_SAM_ACCOUNT\n"); break;
                  case ERROR_DOMAIN_TRUST_INCONSISTENT: strcat(str, "ERROR_DOMAIN_TRUST_INCONSISTENT\n"); break;
		          //case NERR_Success:                    strcat(str, "NERR_Success\n"); break;
		          default: sprintf(str, "NERR_Other %d\n", nSts); break;
				}
                LEVEL_3_ACCEPTLOG(NULL, str);
#endif
			  }
		      if (++nTry >= nADRetryTime) {// １０回までトライを試みる
		        break;
			  }
		      if (nSts != ERROR_NO_LOGON_SERVERS &&
		          nSts != ERROR_NO_SUCH_DOMAIN &&
			      nSts != ERROR_NO_TRUST_LSA_SECRET &&
			      nSts != ERROR_NO_TRUST_SAM_ACCOUNT &&
			      nSts != ERROR_DOMAIN_TRUST_INCONSISTENT &&
			      nSts != NERR_Success)
#ifdef UPDATE_20091026 // AD連携の問い合わせリトライ間隔を短く設定しなおした
		        Sleep(nADRetryMSec);
#else
		        _sleep(1000);
#endif
			} while (nSts != ERROR_NO_LOGON_SERVERS &&
		             nSts != ERROR_NO_SUCH_DOMAIN &&
			         nSts != ERROR_NO_TRUST_LSA_SECRET &&
			         nSts != ERROR_NO_TRUST_SAM_ACCOUNT &&
			         nSts != ERROR_DOMAIN_TRUST_INCONSISTENT &&
			         nSts != NERR_Success);
#endif
		  }
          mbstowcs( wuser, user, 21);
          mbstowcs( wmember, mMailGroup, 21);
	      ///////////////////////////////////
          //setlocale( LC_ALL, "Japanese" );
          //setlocale( LC_ALL, NULL );
#ifdef UPDATE_20060606 // ドメインコントローラへの接続リトライ
	      nTry = 0;
	      do
#endif
		  {
#ifdef UPDATE_20060704 // ローカルマシンのWINDOWSアカウント参照でエラーになる
            sts = NetUserGetLocalGroups( (mAuthDomain[0] ? wAuthServer : NULL),
                                   wuser, 
                                   (DWORD) 0, 
                                   (DWORD) LG_INCLUDE_INDIRECT,
                                   (LPBYTE *) user_info, 
                                   (DWORD) 1024,   // サイズが小さいとグループ一覧が取り出せない
                                   (LPDWORD)&entriesread, 
                                   (LPDWORD)&totalentries 
                                   ); 
#else
            sts = NetUserGetLocalGroups( (mAuthDomain[0] ? wAuthServer : user),
                                   wuser, 
                                   (DWORD) 0, 
                                   (DWORD) LG_INCLUDE_INDIRECT,
                                   (LPBYTE *) user_info, 
                                   (DWORD) 1024,   // サイズが小さいとグループ一覧が取り出せない
                                   (LPDWORD)&entriesread, 
                                   (LPDWORD)&totalentries 
                                   ); 
#endif
#ifdef ADD_IDCACHE
		   if (sts == NERR_Success)
		   {
             if (nIDCashLiveTime > 0)  // 0 以上
			 {
		      SetIDCashList(user, mMailGroup, "gp");
			 }
		   }
#endif

#ifdef UPDATE_20060606 // ドメインコントローラへの接続リトライ
#ifdef ACCEPT_LOG_LEVEL3
            if (sts != NERR_Success) {
		      sprintf(str, "NetUserGetLocalGroups(%s / %s) = ", mAuthDomain, user);
		      switch (sts) {
		        case ERROR_ACCESS_DENIED:  strcat(str, "ERROR_ACCESS_DENIED\n"); break;
		        case ERROR_MORE_DATA:      strcat(str, "ERROR_MORE_DATA\n"); break;
		        case NERR_InvalidComputer: strcat(str, "NERR_InvalidComputer\n"); break;
		        case NERR_UserNotFound :   strcat(str, "NERR_UserNotFound\n"); break;
		        //case NERR_Success:         strcat(str, "NERR_Success\n"); break;
		        default: sprintf(str, "NERR_Other %d\n", sts); break;
			  }
              LEVEL_3_ACCEPTLOG(NULL, mLog);
			}
#endif
	        if (++nTry >= nADRetryTime) {// １０回までトライを試みる
		      break;
			}
		    if (sts != NERR_Success &&
		        sts != ERROR_ACCESS_DENIED &&
			    sts != ERROR_MORE_DATA &&
			    sts != NERR_InvalidComputer &&
			    sts != NERR_UserNotFound)
#ifdef UPDATE_20091026 // AD連携の問い合わせリトライ間隔を短く設定しなおした
		     Sleep(nADRetryMSec);
#else
 		     _sleep(1000);
#endif
		  } while (sts != NERR_Success &&
			       sts != ERROR_ACCESS_DENIED &&
				   sts != ERROR_MORE_DATA &&
				   sts != NERR_InvalidComputer &&
				   sts != NERR_UserNotFound);
#endif
        }
		}
	  }
#ifdef V3
	  else {
	    sts = confirms_user( (pdom ? (pdom+1) : NULL),
		                   user,
						   myaddr, 
						   (struct _User_DB *)&userdb, 
						   (LPDWORD)&totalentries );
	  }
#endif
      //printf("user check status(1) = %sfound local group (%d)\n", (sts ? "not " : ""), sts);
      sprintf(mLog, "user check status(1) = %sfound local group (%d)", (sts ? "not " : ""), sts);
      if (bDebug) printf("%s\n", mLog);
#ifdef ACCEPT_LOG_LEVEL3
      LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
      if (sts == NERR_Success) {
		bGroup = bCheck = FALSE;
#ifdef V3
	if (bUserMan)
#endif
	{
#ifdef LDAP_ON         // LDAPへのログイン処理でSMTP認証の確認を行うオプション // //CRAM-MD5は複合できないので、これを利用する場合は、PLAINかLOGINのみとします。
#ifdef LDAP_OPTION_LICENSE // LDAP Optionのライセンスチェックを行なう
	    if (bLDAP && bLDAPLicense) 
#else
	    if (bLDAP) 
#endif
	    {
          bGroup = TRUE;
		} else
#endif
		{
#ifdef ADD_IDCACHE
		  if (bCache)
		  {
	        if (bLock = CheckIDCashList(user))
			{
		      totalentries = CountIDCashList(user, member, "gp", mMailGroup, &bGroup, &bCheck);
			} else { // ロックできないとき
			  totalentries = 0;
			  member[0] = '\x0';
			  bCheck = FALSE;
              bGroup = FALSE;
			}
            ReleaseIDChche(user);
		  } else 
#endif
		  {
#ifdef ADD_IDCACHE
	      if (nIDCashLiveTime > 0 && !bCache)  // 0 以上
		  {
		    if (bLock = CheckIDCashList(user))
			{
              DelIDCashList(user, "gp");
			}
            ReleaseIDChche(user);
		  }
#endif
  	      for (entries = 0; entries < totalentries; entries++) {
		    member[0] = '\x0';
            wcstombs( member, (user_info[0]+entries)->lgrui0_name, 128 );
            //printf("\n%s's Local group check=(\"%s\" vs. \"%s\") ", user, mMailGroup, member);
            sprintf(mLog, "%s's Local group check=(\"%s\" vs. \"%s\") ", user, mMailGroup, member);
            if (bDebug) printf("%s\n", mLog);
#ifdef ACCEPT_LOG_LEVEL3
            LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
            //if (strcmp(member, "IMSUsers") == 0) {
            if (stricmp(member, mMailGroup) == 0) { // 2004.03.15 大小文字の違いなし
              bGroup = TRUE;
			}
		    if (strstr(member,".")) {
			  bCheck = TRUE;
			}
#ifdef ADD_IDCACHE
	        if (bLock)  // 0 以上
			{
		      AddIDCashList(user, member, "gp");
			}
#endif
		  }
#ifdef ADD_IDCACHE
          if (bLock)
		  {
			ReleaseIDChche(user);
		  }
#endif
		  }
		}
	}
#ifdef V3
	else {
      bGroup = TRUE;
      if (strstr(userdb.domain,".")) {
  	    bCheck = TRUE;
	  }
#endif
	}
	    bUser = TRUE;
	    if (bGroup && bCheck && pdom)  {
		  bUser = FALSE;
printf("\n");
  	      for (entries = 0; entries < totalentries; entries++) {
		     member[0] = '\x0';
#ifdef V3
	if (bUserMan)
#endif
	{
#ifdef LDAP_ON         // LDAPへのログイン処理でSMTP認証の確認を行うオプション // //CRAM-MD5は複合できないので、これを利用する場合は、PLAINかLOGINのみとします。
#ifdef LDAP_OPTION_LICENSE // LDAP Optionのライセンスチェックを行なう
	  if (bLDAP && bLDAPLicense) 
#else
	  if (bLDAP) 
#endif
	  {
        strcpy(member, "");
	  } else 
#endif
	  {
#ifdef ADD_IDCACHE
		if (bCache)
		{
	      if (bLock = CheckIDCashList(user))
		  {
		    GetNumIDCashList(user, member, "gp", entries);
		  } else { // ロックできないとき
			member[0] = '\x0';
		  }
		} else 
#endif
		{
          wcstombs( member, (user_info[0]+entries)->lgrui0_name, 128 );
		}
	  }
	}
#ifdef V3
	else {
             strcpy( member, userdb.domain);
	}
	GetProfileStringEx((LPCTSTR)DOMAIN_SMTPIP, (LPCTSTR)member, (LPCTSTR)"", (LPTSTR)mIP, (DWORD)sizeof(mIP)); // 対象ドメインの応答ＩＰアドレス取出し
#ifdef UPDATE_20070521 // 予約語対策
    strcpy(mKey, member);
    strtok(mKey, "@");
    if (ReservedWords(mKey)) {
      strcpy(mKey, mReservedWords);
      strcat(mKey, member);
#ifndef UPDATE_20071022A // 変数の初期値が不定のためにハングする可能性
      if (qpdom)
		strcat(mKey, qpdom);
#endif
      GetProfileStringEx((LPCTSTR)DOMAIN_SMTPIP, (LPCTSTR)mKey, (LPCTSTR)"", (LPTSTR)mIP, (DWORD)sizeof(mIP)); // 対象ドメインの応答ＩＰアドレス取出し
	}
#endif
    //printf("%s Permission address check=(\"%s:%s\" vs. \"%s\") ", user, member, mIP, myaddr);
    sprintf(mLog, "%s Permission address check=(\"%s:%s\" vs. \"%s\") ", user, member, mIP, myaddr);
    if (bDebug) printf("%s\n", mLog);
#ifdef ACCEPT_LOG_LEVEL3
    LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
#ifdef CLUSTERING
    if (mIP[0] != '\x0' && !strstr(mIP, myaddr)) // 対象のドメインは接続IPで許可されたものであるか？
#else
	if (mIP[0] != '\x0' && strcmp(myaddr, mIP) != 0) // 対象のドメインは接続IPで許可されたものであるか？
#endif
	{
	  //printf("unmach\n");
      sprintf(mLog, " unmach");
      if (bDebug) printf("%s\n", mLog);
#ifdef ACCEPT_LOG_LEVEL3
      LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
	  continue;
	}
	//////////////////////////////////////////////
	//printf("mach\n");
    sprintf(mLog, " mach!!");
    if (bDebug) printf("%s\n", mLog);
#ifdef ACCEPT_LOG_LEVEL3
    LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
#endif
             //printf("%s's Local group check=(\"%s\" vs. \"%s\")\n", user, (pdom+1), member);
             sprintf(mLog, "%s's Local group check=(\"%s\" vs. \"%s\")", user, (pdom+1), member);
             if (bDebug) printf("%s\n", mLog);
#ifdef ACCEPT_LOG_LEVEL3
             LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
			 if (stricmp(member, (pdom+1)) == 0) { // 2004.03.15 大小文字の違いなし
               bUser = TRUE;
			   break;
			 }
		  }
		}
		   if (bGroup && bUser) {
              bsts = TRUE;
#ifdef V3
	if (bUserMan)
#endif
	{
#ifdef LDAP_ON         // LDAPへのログイン処理でSMTP認証の確認を行うオプション // //CRAM-MD5は複合できないので、これを利用する場合は、PLAINかLOGINのみとします。
#ifdef LDAP_OPTION_LICENSE // LDAP Optionのライセンスチェックを行なう
	  if (bLDAP && bLDAPLicense) 
#else
	  if (bLDAP) 
#endif
	  {
        //strcpy(member, "");
	  } else 
#endif
	  {
#ifdef ADD_IDCACHE
	    if (nIDCashLiveTime > 0)  // 0 以上
		{
          if ((bCache = GetIDCashList(user, fname, "fn", &bHit)))
		  {
			if (fname[0] != '\x0')
			  sprintf(fullname, "<%s>", fname);
#ifdef ACCEPT_LOG_LEVEL3
            sprintf(str, "LogonUser(%s) Cache Success.", user);
		    LEVEL_3_ACCEPTLOG(NULL, str);
#endif
		  }
		}
	    if (!bCache)
#endif
		{
  			  fname[0] = *fullname = '\x0';
              //if (NetUserGetInfo( NULL, wuser, (DWORD)10, (LPBYTE *) uinfo) == NERR_Success) {
              if (NetUserGetInfo( (mAuthDomain[0] ? wAuthServer : NULL), wuser, (DWORD)10, (LPBYTE *) uinfo) == NERR_Success) {
                wcstombs( fname, uinfo[0]->usri10_full_name, 256 );
			    if (fname[0] != '\x0')
			      sprintf(fullname, "<%s>", fname);
                NetApiBufferFree(uinfo[0]);
#ifdef ADD_IDCACHE
                if (nIDCashLiveTime > 0)  // 0 以上
				{
		          SetIDCashList(user, fullname, "fn");
				}
#endif
			  }
		}
	  }
	}
#ifdef V3
	else {
             sprintf(fullname, "<%s>", userdb.fullname);
	}
#endif
              //printf("match!!\n",member);
              sprintf(mLog, "<%s> match!!", userdb.domain); //member);
              if (bDebug) printf("%s\n", mLog);
#ifdef ACCEPT_LOG_LEVEL3
              LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
#ifdef V4
	   //////////////////////////////////////////////
	   // 実ユーザ名はメールアドレスとして禁止する場合
#ifdef UPDATE_20070510 // 実ユーザ名でのメール送受信を禁止処理する処理
       if (!bAliases &&
		   GetSMTPOFFFile(user, (pdom ? pdom+1 : userdb.domain))) {
		 bsts = FALSE;
         sprintf(mLog, " prohibit");
         if (bDebug) printf("%s\n", mLog);
#ifdef ACCEPT_LOG_LEVEL3
         LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
	   }
#endif
		////// 送信有効期間の確認
		if (!bSubLocal)  // RCPT TO:以外の場合チェック
#ifdef UPDATE_20050512
		  bValidTime = GetUseTimeFile(user, (pdom ? pdom+1 : userdb.domain));
#else
		  bValidTime = GetUseTimeFile(user, (pdom ? pdom+1 : NULL));
#endif
#endif
/*
		///// Pop before SMTP を行うなら
		if (bPOPbeforeSMTP) {
		  if (bDebug) printf("checked Pop before Smtp:");
          bsts = GetLockFile(user);
        }
*/
        ////////////////////////////////
	          //break;
		   } else {
             //printf("unmatch\n",member);
             sprintf(mLog, "<%s> unmatch\n", userdb.domain); //member);
             if (bDebug) printf("%s\n", mLog);
#ifdef ACCEPT_LOG_LEVEL3
             LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
		   }
		//}
#ifdef V3
	if (bUserMan)
#endif
	{
#ifdef LDAP_ON         // LDAPへのログイン処理でSMTP認証の確認を行うオプション // //CRAM-MD5は複合できないので、これを利用する場合は、PLAINかLOGINのみとします。
#ifdef LDAP_OPTION_LICENSE // LDAP Optionのライセンスチェックを行なう
	  if (bLDAP && bLDAPLicense) 
#else
	  if (bLDAP) 
#endif
	  {
	  } else 
#endif
	  {
#ifdef ADD_IDCACHE
	    if (!bCache)
#endif
		{
          NetApiBufferFree(user_info[0]);
		}
	  }
	}

	  }
	  //} else if (sts == NERR_UserNotFound ) {
	  /*
      if (!bsts) {
        bsts = GetPostMaster((char *)user, (char *)pdom);
        if (bsts && strstr(user,"@"))
	      pdom = NULL;
	  }
	  */
	  //if (!bsts) {

/*
        bsts = GetAliases((LPCTSTR)SOFT_ALIASES_REG, (char *)user, (char *)pdom);
        if (bsts && strstr(user,"@"))
	      pdom = NULL;
*/
		/*
	    if (!bsts) {
          bsts = GetPostMaster((char *)user);
	      //if (bsts)
	        //pdom = NULL;
		}
		*/
	    if (!bsts) {
          //bsts = GetMLists((LPCTSTR)"SOFTWARE\\EMWAC\\IMS\\Lists", (char *)user, &bRequest);
#ifdef MULTI_ML
		  if (pdom) { /// 2002.09.04 ドメイン有りのMLチェック
	        *pdom = '@';
            bsts = GetMLists((LPCTSTR)SOFT_LISTS_REG, (char *)user, &bRequest);
		    if (!bsts) {
              if (pdom) { /// 2002.09.04 ドメイン無しのMLのチェック
	            *pdom = '\x0';
                bsts = GetMLists((LPCTSTR)SOFT_LISTS_REG, (char *)user, &bRequest);
			  }
			} 
		  } else 
#endif
          bsts = GetMLists((LPCTSTR)SOFT_LISTS_REG, (char *)user, &bRequest);
		  if (bMList && !bRequest) // NULLでないなら
	        *bMList = bsts;
		  if (bsts && !bMList) { // && !bRequest) { // MAIL FROM:での利用許可否
			if (GetMLSenderPermission(user))  // 送信者として無効の場合、"MAIL FROM:"のチェックでは無効
			  bsts = FALSE;
		  }
#ifdef MULTI_ML
          if (pdom) { /// 2002.09.04 名称を戻す
	        *pdom = '\x0';
		  }
#endif
		}
        //printf("user check status(2) = %sfound %s\n", (!bsts ? "not " : ""), (bMList ? "Lists" : "Aliases"));
        sprintf(mLog, "user check status(2) = %sfound %s", (!bsts ? "not " : ""), (bMList ? "Lists" : "Aliases"));
        if (bDebug) printf("%s\n", mLog);
#ifdef ACCEPT_LOG_LEVEL3
        LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
	  //}
	  if (bsts == FALSE && sts != NERR_Success) {
		strcpy(fullname, "");
        //printf("user check status(2) =");
        sprintf(mLog, "user check status(2) =");
        if (bDebug) printf("%s\n", mLog);
#ifdef ACCEPT_LOG_LEVEL3
        LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
		if (sts == NERR_UserNotFound) {
          //printf(" User not found.(%d)\n",sts);
          sprintf(mLog, " User not found.(%d)",sts);
		} else if (sts == ERROR_ACCESS_DENIED) {
          //printf(" Access denied.(%d)\n",sts);
          sprintf(mLog, " Access denied.(%d)",sts);
		} else {
          //printf(" Other error.(%d)\n",sts);
          sprintf(mLog, " Other error.(%d)",sts);
		}
        if (bDebug) printf("%s\n", mLog);
#ifdef ACCEPT_LOG_LEVEL3
        LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
	  }
	} else if (!*bLocal && !bSubDom) {
	  if (bOutSideAliases)
	    *bOutSideAliases = bAliases;
	  bsts = bAliases; // 外部ドメインでエイリアスの場合有効
	  if (bsts)
		*bLocal = TRUE;
	}
	if (pdom) {
	   *pdom = '@';
	}
#ifdef UPDATE_20050512
	else {  // ドメインが無いなら付け
#ifdef UPDATE_200500902 // ドメインが空欄だと、メモリが初期化されないままドメインとなる
	  if (user[0] != '\x0') {
	    if (userdb.domain[0]) {
          strcat(user, "@");
          strcat(user, userdb.domain);
		}
	  }
#else
      if (userdb.domain[0]) {
         strcat(user, "@");
         strcat(user, userdb.domain);
	  }
#endif
	}
#endif

#ifdef V4
#ifdef UPDATE_20080326 // ドメイン無しアカウントをローカルとしての利用有無選択機能追加
	return (bsts && *bLocal && bValidTime && (bHaveDomain || pdom) );
#else
    ////// 接続時間制限
	return (bsts && *bLocal && bValidTime);
#endif
#else
	return (bsts && *bLocal);
#endif

}
