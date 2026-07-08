////////////////////////////////////////////////////////////
// Userlist.c Copyright K.kawakami
////////////////////////////////////////////////////////////
#define UNICODE 1
#include "imap4.h"
#include <windows.h>
#include <lmcons.h>
#include <lmaccess.h>
#include <lmerr.h>
#include <lmapibuf.h>
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>
#include <locale.h>
  
///////////////////////////
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
///////////////////////////
extern CHAR mMailBoxDir[];
extern char mAuthDomain[];
extern char mLocalDomain[];
extern DWORD nLocalDomain;
extern char mMailGroup[];
extern BOOL bDebug;
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
#endif
#ifdef UPDATE_20070521 // OSの予約語対策
extern CHAR   mReservedWords[];
#endif
#ifdef ADD_IDCACHE
extern int  nIDCashLiveTime; // ADキャッシュ利用有効時間(秒単位)
#endif
#ifdef UPDATE_20160113 // 認証時のロックアウト機能を追加
extern DWORD nAuthLockOut; // ロックアウトまでの回数 デフォルト 0:無効
#endif

NET_API_STATUS confirms_user(char *Domain, char *user, char *myaddr, struct _User_DB *userdb, DWORD *total);
void GetDomainFromIP(char *myaddr, char *mydomain, DWORD nSize);
#ifdef UPDATE_20160113 // 認証時のロックアウト機能を追加
#ifdef UPDATE_20180819B // 認証成功時にロックアウトカウンタのリセット機能を追加
void AuthLockOut(char *pAddr, char  *pDir, BOOL bFlg);
#else
void AuthLockOut(char *pAddr, char  *pDir);
#endif
#endif

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

#ifdef V3
  if (bUserMan) {
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
		    SetIDCashList(lpszUser, lpszHomeDir,  "hm");
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
      LDAP_UserList(&ldapd);
#endif
	  }
	} else
#endif
	{
#endif
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
		    //_sleep(1000);
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
	      sprintf(str, "NetUserGetInfo(%s / %s) = ", mAuthDomain, lpszUser);
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

#ifdef UPDATE_20160113 // 認証時のロックアウト機能を追加
DWORD CheckUser(char *user, char *pass, char *myaddr, char *pPeerAddr)
#else
DWORD CheckUser(char *user, char *pass, char *myaddr)
#endif
{
#ifdef ADD_IDCACHE
#ifdef ACCEPT_LOG_LEVEL3
    char str[LOG_BUFFER];
#endif
#endif	
	DWORD  nError = 0;
//#ifndef _DEBUG
	HANDLE hToken;
#ifdef V3
	char  *p;
	CHAR  Bdir[MAX_PATH];
	CHAR  mDom[256];
    DWORD totalentries; 
	struct _User_DB userdb;
#endif
#ifdef V4
	BOOL   bValidTime = TRUE;
#endif
#ifdef LDAP_ON         // LDAPへのログイン処理でSMTP認証の確認を行うオプション // //CRAM-MD5は複合できないので、これを利用する場合は、PLAINかLOGINのみとします。
	CHAR   mNULL[] = "";
	CHAR   mScope[256];
    LDAPD  ldapd;
#endif
#ifdef ADD_IDCACHE
	BOOL   bHit = FALSE;
	BOOL   bCache = FALSE;
#endif

#ifdef V3
    memset(mDom, 0, sizeof(mDom));
	if (bUserMan) {
#ifdef LDAP_ON         // LDAPへのログイン処理でSMTP認証の確認を行うオプション // //CRAM-MD5は複合できないので、これを利用する場合は、PLAINかLOGINのみとします。
#ifdef LDAP_OPTION_LICENSE // LDAP Optionのライセンスチェックを行なう
	  if (bLDAP && bLDAPLicense) 
#else
	  if (bLDAP) 
#endif
	  {
		if ((p = strchr(user, '@')))
		  *p = '\x0';
        ldapd.pHost = (mAuthDomain[0] ? mAuthDomain : "localhost");
	    ldapd.nPort = nLDAPPort;
#ifdef UPDATE_20070620 // ドメインごとにDNを設定可能にする
		if (!p)
		  p = strstr(user, "%");
	    if (p) {
		  *p = 0;
		  strcpy(mDom, p+1);
		} else {
	      GetDomainFromIP(myaddr, mDom, sizeof(mDom));
		}
        ldapd.pLogonDomain = mDom;
#else
        ldapd.pLogonDomain = mNULL;
#endif
	    ldapd.pLogonID = user;
		ldapd.pLogonPW = pass;
#ifdef ADD_IDCACHE
	    if (nIDCashLiveTime > 0)  // 0 以上
		{
          if ((bCache = GetIDCashList(user, pass, "pw", &bHit)))
		  {
			nError = (bHit ? 0 : -1);
#ifdef ACCEPT_LOG_LEVEL3
            sprintf(str, "LogonUser(%s) Cache Success.", user);
		    LEVEL_3_ACCEPTLOG(NULL, str);
#endif
		  }
		}
		if (!bCache)
#endif
		{
          if (!LDAP_LOGON(&ldapd)) { 
            nError = -1;
		  } else {
#ifdef ADD_IDCACHE
            if (nIDCashLiveTime > 0)  // 0 以上
			{
			  SetIDCashList(user, pass, "pw");
			}
#endif
		  }
		}
		if (p)
		  *p = '@';
	  } else
#endif
	  {
#endif
#ifdef ADD_IDCACHE
	    if (nIDCashLiveTime > 0)  // 0 以上
		{
          if ((bCache = GetIDCashList(user, pass, "pw", &bHit)))
		  {
			nError = (bHit ? 0 : -1);
#ifdef ACCEPT_LOG_LEVEL3
            sprintf(str, "LogonUser(%s) Cache Success.", user);
		    LEVEL_3_ACCEPTLOG(NULL, str);
#endif
		  }
		}
		if (!bCache)
#endif
		{
          if (!bDebug) {
            if (LogonUserA(user,   // サービスとして動作させれば正常に働く
		                (mAuthDomain[0] ? mAuthDomain : "."), //".", //"KAWAKAMI", //NULL, //
		             //(mAuthDomain[0] ? "" : "."), //".", //"KAWAKAMI", //NULL, //
					    pass, 
		                LOGON32_LOGON_BATCH, //LOGON32_LOGON_INTERACTIVE, //LOGON32_LOGON_BATCH, 
		 		        LOGON32_PROVIDER_DEFAULT,
                        &hToken) ) {
              CloseHandle(hToken);
#ifdef ADD_IDCACHE
#ifdef ACCEPT_LOG_LEVEL3
               sprintf(str, "LogonUser(%s) Success.", user);
		       LEVEL_3_ACCEPTLOG(NULL, str);
#endif
	          if (nIDCashLiveTime > 0)  // 0 以上
			  {
				SetIDCashList(user, pass, "pw");
			  }
#endif
			} else {
              nError = GetLastError();
#ifdef ADD_IDCACHE
#ifdef ACCEPT_LOG_LEVEL3
              sprintf(str, "LogonUser(%s) Fail. GetLastError() = %d", user, nError);
		      LEVEL_3_ACCEPTLOG(NULL, str);
#endif
#endif
		      if (nError == 0)
		        nError = -1;
			}
		  } else {
#ifdef ADD_IDCACHE
            if (nIDCashLiveTime > 0)  // 0 以上 // デバッグモードのとき
			{
			  SetIDCashList(user, pass, "pw");
			}
#endif
		  }
		}
	  }
#ifdef V3
	} else {
	  //memset(mDom, 0, sizeof(mDom));
	  nError = -1;
	  p = strstr(user, "@");
	  if (!p) // ドメイン区切りに%も可能に
		p = strstr(user, "%");
	  if (p) {
		*p = 0;
		p++;
		strcpy(mDom, p);
	  } else {
//printf("GetDomainFromIP(%s, %s, %d)\n", myaddr, mDom, sizeof(mDom));
	    GetDomainFromIP(myaddr, mDom, sizeof(mDom));
	  }
printf("confirms_user(mDom = %s, user = %s)\n", mDom, user);
	  if (confirms_user( mDom, //(mAuthDomain[0] ? wAuthServer : NULL),
		                 user,
#ifdef UPDATE_20050514
						 myaddr, //"",
#else
						 "",
#endif
						 (struct _User_DB *)&userdb, 
						 (LPDWORD)&totalentries ) == NERR_Success) {
		strcat(user, "@"); strcat(user, mDom);
	    GetBaseDirectory(Bdir, mMailBoxDir, user, myaddr);
		//if (GetPasswordFile(Bdir, mpass)) {
        if (strcmp(pass, userdb.password) == 0)
		  nError = 0;
		//}
	  }
	}
#endif
#ifdef UPDATE_20160113 // 認証時のロックアウト機能を追加
    if (nAuthLockOut > 0 && nError == -1)
	{
      GetBaseDirectory(Bdir, mMailBoxDir, user, myaddr);
#ifdef UPDATE_20180819B // 認証成功時にロックアウトカウンタのリセット機能を追加
	  AuthLockOut(pPeerAddr, Bdir, TRUE);
#else
      AuthLockOut(pPeerAddr, Bdir);
#endif
#ifdef UPDATE_20180819A // 認証セッション中にロックアウト回数に達したら強制切断する
	  if (HiddenMyIP(pPeerAddr, 1))
	  {
		nError = -2;
	  }
#endif
	}
#ifdef UPDATE_20180819B // 認証成功時にロックアウトカウンタのリセット機能を追加
  	  else if (nAuthLockOut > 0) {
      GetBaseDirectory(Bdir, mMailBoxDir, user, myaddr);
	  AuthLockOut(pPeerAddr, Bdir, FALSE);
	}
#endif
#endif

#ifdef V4
	////// 送信有効期間の確認
    if (!GetUseTimeFile(user, mDom)) //NULL))
#ifdef UPDATE_20180819A // 認証セッション中にロックアウト回数に達したら強制切断する
	  if (nError != -2)
#endif
        nError = -1;
#endif
	return nError;
}
