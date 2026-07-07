////////////////////////////////////////////////////////////
// Userlist.c Copyright K.kawakami
////////////////////////////////////////////////////////////
//#define UNICODE 1
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
extern int  nSenderLog;
extern BOOL  bDebug;
extern char mProgPath[];
extern char mAuthDomain[];
extern char mLocalDomain[];
extern DWORD nLocalDomain;
extern char mMailGroup[];
#ifdef UPDATE_20091026 // AD連携の問い合わせリトライ間隔を短く設定しなおした
extern DWORD  nLDAPRetryMSec;
extern DWORD  nADRetryMSec;
#endif
#ifdef UPDATE_20060606 // ドメインコントローラへの接続リトライ
extern DWORD  nADRetryTime;
#endif
#ifdef LDAP_ON         // LDAPへのログイン処理でSMTP認証の確認を行うオプション // //CRAM-MD5は複合できないので、これを利用する場合は、PLAINかLOGINのみとします。
extern BOOL   bLDAP;
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
#ifdef CLUSTERING
extern BOOL   nClustering;
#endif

#ifdef UPDATE_20070521 // OSの予約語対策
extern CHAR   mReservedWords[];
#endif
#ifdef ADD_IDCACHE
extern int  nIDCashLiveTime; // ADキャッシュ利用有効時間(秒単位)
#endif

void printfTrace(char *str);
int Base64DecodeLine(char * pszLine, int nLineLen, unsigned char * uchBuffer, int nBufferLen);
void SetRCPFile(struct _RCP *rcp, char *mRCP, BOOL bmod, DWORD *no);
NET_API_STATUS confirms_user(char *Domain, char *user, char *myaddr, struct _User_DB *userdb, DWORD *total);

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
#ifdef TRACE_MODE
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
	if (bLDAP) {
#ifdef ADD_IDCACHE
      if (nIDCashLiveTime > 0)  // 0 以上
	  {
        if ((bCache = GetIDCashList(lpszUser, lpszHomeDir, "hm", &bHit)))
		{
		  err = (bHit ? NERR_Success : 2221);
#ifdef TRACE_MODE
          if (nSenderLog) {
            sprintf(str, "LogonUser(%s) Cache Success.", lpszUser);
            printfTrace(str);
		  }
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
      sprintf(mScope, "%s=%s,%s",  mLDAPSchemaRDN, lpszUser, mLDAPSchemaDC);
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
#ifdef TRACE_MODE
          if (nSenderLog) {
            sprintf(str, "LogonUser(%s) Cache Success.", lpszUser);
            printfTrace(str);
		  }
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
#ifdef TRACE_MODE
          if (nSenderLog) {
		    sprintf(str, "NetGetAnyDCName(%s / %s) = ", mAuthDomain, lpszUser);
		    switch (nSts) {
		      case ERROR_NO_LOGON_SERVERS:          strcat(str, "ERROR_NO_LOGON_SERVERS\n"); break;
		      case ERROR_NO_SUCH_DOMAIN:            strcat(str, "ERROR_NO_SUCH_DOMAIN\n"); break;
		      case ERROR_NO_TRUST_LSA_SECRET:       strcat(str, "ERROR_NO_TRUST_LSA_SECRET\n"); break;
		      case ERROR_NO_TRUST_SAM_ACCOUNT:      strcat(str, "ERROR_NO_TRUST_SAM_ACCOUNT\n"); break;
              case ERROR_DOMAIN_TRUST_INCONSISTENT: strcat(str, "ERROR_DOMAIN_TRUST_INCONSISTENT\n"); break;
		      case NERR_Success:                    strcat(str, "NERR_Success\n"); break;
		      default: strcat(str, "NERR_Other\n"); break;
			}
            printfTrace(str);
		  }
#endif
		  if (++nTry >= nADRetryTime) {// １０回までトライを試みる
		    break;
		  }
	      if (nSts != NERR_Success &&
	          nSts != ERROR_NO_LOGON_SERVERS &&
		      nSts != ERROR_NO_SUCH_DOMAIN &&
		      nSts != ERROR_NO_TRUST_LSA_SECRET &&
		      nSts != ERROR_NO_TRUST_SAM_ACCOUNT &&
		      nSts != ERROR_DOMAIN_TRUST_INCONSISTENT)
#ifdef UPDATE_20091026 // AD連携の問い合わせリトライ間隔を短く設定しなおした
		    Sleep(nADRetryMSec);
#else
		    _sleep(1000);
#endif
		} while (nSts != NERR_Success &&
	         nSts != ERROR_NO_LOGON_SERVERS &&
	         nSts != ERROR_NO_SUCH_DOMAIN &&
	         nSts != ERROR_NO_TRUST_LSA_SECRET &&
	         nSts != ERROR_NO_TRUST_SAM_ACCOUNT &&
	         nSts != ERROR_DOMAIN_TRUST_INCONSISTENT);
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
		    wcstombs( lpszHomeDir, user_info11->usri11_home_dir, 256 );
  		    NetApiBufferFree(user_info11);
#ifdef ADD_IDCACHE
            if (nIDCashLiveTime > 0)  // 0 以上
			{
		      SetIDCashList(lpszUser, lpszHomeDir,  "hm");
			}
#endif
            break;
        default:
            //printf("user not found.\n");
           break;
		}
#ifdef UPDATE_20060606 // ドメインコントローラへの接続リトライ
#ifdef TRACE_MODE
		{
	      sprintf(str, "NetUserGetInfo(%s / %s) = ", mAuthDomain, lpszUser);
	      switch (err) {
	        case ERROR_ACCESS_DENIED:          strcat(str, "ERROR_ACCESS_DENIED\n"); break;
	        case NERR_InvalidComputer:            strcat(str, "NERR_InvalidComputer\n"); break;
		    case NERR_UserNotFound:       strcat(str, "NERR_UserNotFound\n"); break;
		    case NERR_Success:                    strcat(str, "NERR_Success\n"); break;
		    default: sprintf(str, "NERR_Other %d\n", err); break;
		  }
          printfTrace(str);
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
	  err = confirms_user( lpszUserDomain, // 対象アカウントのドメイン
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

BOOL CheckUser(char *mFrom, char *mRcpt, char *mAnswer, BOOL *bML, BOOL *bNoReply)
{
#ifdef TRACE_MODE
   char str[LOG_BUFFER];
#endif
	NET_API_STATUS sts = 0;
	NET_API_STATUS nSts;
    DWORD entriesread, entries; 
	DWORD totalentries; 
    LPLOCALGROUP_USERS_INFO_0   user_info[1];
    //LPUSER_INFO_10      uinfo[1];
	BOOL  bsts = FALSE, bdomsts = FALSE, bRequest = FALSE, bSubDom = FALSE;
	BOOL  bGroup = FALSE, bCheck = FALSE, bUser = FALSE;
	BOOL  bMList = FALSE, bReq = FALSE;
	LPBYTE  pbuff;
	wchar_t wDom[65], wAuthServer[65], wuser[21], wmember[21];
	char   member[256], *pdom, *pldom;//, fname[256]; //*dom[256]
	CHAR  mkey[256], mModerator[_MAX_PATH], ml[256];
	DWORD nWhoCanSend;
#ifdef V3
	struct _User_DB userdb;
#endif
#ifdef UPDATE_20060606 // ドメインコントローラへの接続リトライ
	int    nTry;
#endif
#ifdef LDAP_ON         // LDAPへのログイン処理でSMTP認証の確認を行うオプション // //CRAM-MD5は複合できないので、これを利用する場合は、PLAINかLOGINのみとします。
	CHAR   mNULL[] = "";
	CHAR   mScope[256], mFilter[256];
	CHAR   mKey[256], mDN[256];
    LDAPD  ldapd;
#endif
#ifdef UPDATE_20070521 // 予約語対策
  char *p, *q, mML[512];
#endif
#ifdef ADD_IDCACHE
  BOOL  bHit = FALSE;
  BOOL  bCache = FALSE;
  BOOL  bLock  = FALSE;
#endif

    //PHOSTENT host[2];
	//struct in_addr *inDom, *inlDom;
	//char mDom[16], mlDom[16];

	*mAnswer = '\x0';
	*bML   = FALSE;
    /// User Local Domain ? /////
	if (bDebug)
	  printf("Check To: address = %s\n", mRcpt);
#ifdef TRACE_MODE
    if (nSenderLog) {
	  sprintf(str, "Check To: address = %s\n", mRcpt);
      printfTrace(str);
	}
#endif

	pdom = strstr(mRcpt, "@");
	if (pdom)
	  *pdom = '\x0';
#ifdef UPDATE_20070509 // エイリアス設定の実アドレスオプションを別データとして処理する
    bsts = GetAliases((LPCTSTR)SOFT_ALIASES_REG, (char *)mRcpt, (char *)pdom, NULL);
#else
    bsts = GetAliases((LPCTSTR)SOFT_ALIASES_REG, (char *)mRcpt, (char *)pdom);
#endif
	if (bsts) {
      printf("\t-> aliases = %s\n", mRcpt);
	} else {
	  if (pdom)
	    *pdom = '@';
	}
	///////////////
	pdom = strstr(mRcpt, "@");
	if (pdom) {
	  //strncpy((char *)dom, (char *)(pdom+1), 256);
	  *pdom = '\x0';
      pldom = mLocalDomain;
	}
	if (!bSubDom) {
	  entriesread =  0;
	  totalentries = 0;
#ifdef V3
	if (bUserMan) {
#endif
#ifdef LDAP_ON         // LDAPへのログイン処理でSMTP認証の確認を行うオプション // //CRAM-MD5は複合できないので、これを利用する場合は、PLAINかLOGINのみとします。
	  if (bLDAP) {
#ifdef ADD_IDCACHE
	      if (nIDCashLiveTime > 0)  // 0 以上
		  {
            bCache = GetIDCashList(mRcpt, mMailGroup, "gp", &bHit);
		    sts = (bHit ? NERR_Success : 2221);
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
        sprintf(mScope, "%s=%s,%s", mLDAPSchemaRDN, mRcpt, (mDN[0] ? mDN : mLDAPSchemaDC));
#endif
#else
        sprintf(mScope, "%s=%s,%s",  mLDAPSchemaRDN, mRcpt, mLDAPSchemaDC);
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
		  sprintf(p, "(%s=%s))", mLDAPSchemaID, mRcpt);
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
		      if (sts == NERR_Success)
			  {
                if (nIDCashLiveTime > 0)  // 0 以上
				{
		          SetIDCashList(mRcpt, mMailGroup, "gp");
				}
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
        sts = (LDAP_UserList(&ldapd) ? NERR_Success : -1);
#endif
		  }
	  } else
#endif
	  {
#ifdef ADD_IDCACHE
	      if (nIDCashLiveTime > 0)  // 0 以上
		  {
            bCache = GetIDCashList(mRcpt, mMailGroup, "gp", &bHit);
		    sts = (bHit ? NERR_Success : 2221);
		  }
	      if (!bCache)
#endif
		  {
	    if (mAuthDomain[0]) {
          mbstowcs( wDom, mAuthDomain, 65);
#ifdef UPDATE_20060606 // ドメインコントローラへの接続リトライ
		  nTry = 0;
		  do {
#endif
		    nSts = NetGetAnyDCName(NULL, 
                               wDom,
                               &pbuff);
	        if (nSts == NERR_Success) {
		      wcscpy(wAuthServer, (const wchar_t *)pbuff);
	          //wcstombs(mAuthServer, (const wchar_t *)pbuff, 65);
              NetApiBufferFree(pbuff);
			}
#ifdef UPDATE_20060606 // ドメインコントローラへの接続リトライ
#ifdef TRACE_MODE
            if (nSenderLog) {
		      sprintf(str, "NetGetAnyDCName(%s / %s) = ", mAuthDomain, mRcpt);
		      switch (nSts) {
		        case ERROR_NO_LOGON_SERVERS:          strcat(str, "ERROR_NO_LOGON_SERVERS\n"); break;
		        case ERROR_NO_SUCH_DOMAIN:            strcat(str, "ERROR_NO_SUCH_DOMAIN\n"); break;
		        case ERROR_NO_TRUST_LSA_SECRET:       strcat(str, "ERROR_NO_TRUST_LSA_SECRET\n"); break;
		        case ERROR_NO_TRUST_SAM_ACCOUNT:      strcat(str, "ERROR_NO_TRUST_SAM_ACCOUNT\n"); break;
                case ERROR_DOMAIN_TRUST_INCONSISTENT: strcat(str, "ERROR_DOMAIN_TRUST_INCONSISTENT\n"); break;
		        case NERR_Success:                    strcat(str, "NERR_Success\n"); break;
		        default: sprintf(str, "NERR_Other %d\n", nSts); break;
			  }
              printfTrace(str);
			}
#endif
		    if (++nTry >= nADRetryTime) {// １０回までトライを試みる
			  break;
			}
	        if (nSts != NERR_Success &&
	            nSts != ERROR_NO_LOGON_SERVERS &&
		        nSts != ERROR_NO_SUCH_DOMAIN &&
		        nSts != ERROR_NO_TRUST_LSA_SECRET &&
		        nSts != ERROR_NO_TRUST_SAM_ACCOUNT &&
		        nSts != ERROR_DOMAIN_TRUST_INCONSISTENT)
#ifdef UPDATE_20091026 // AD連携の問い合わせリトライ間隔を短く設定しなおした
		    Sleep(nADRetryMSec);
#else
		    _sleep(1000);
#endif
		  } while (nSts != NERR_Success &&
	               nSts != ERROR_NO_LOGON_SERVERS &&
		           nSts != ERROR_NO_SUCH_DOMAIN &&
		           nSts != ERROR_NO_TRUST_LSA_SECRET &&
		           nSts != ERROR_NO_TRUST_SAM_ACCOUNT &&
		           nSts != ERROR_DOMAIN_TRUST_INCONSISTENT);
#endif
		}
        mbstowcs( wuser, mRcpt, 21);
        mbstowcs( wmember, mMailGroup, 21); //mbstowcs( wmember, "IMSUsers", 20);
	    ///////////////////////////////////
        //setlocale( LC_ALL, "Japanese" );
        //setlocale( LC_ALL, NULL );
	    //entriesread =  0;
	    //totalentries = 0;
#ifdef UPDATE_20060606 // ドメインコントローラへの接続リトライ
	    nTry = 0;
	    do {
#endif
          sts = NetUserGetLocalGroups( (mAuthDomain[0] ? wAuthServer : NULL),
                                   wuser, 
                                   (DWORD) 0, 
                                   (DWORD) LG_INCLUDE_INDIRECT,
                                   (LPBYTE *) user_info, 
                                   (DWORD) 1024,   // サイズが小さいとグループ一覧が取り出せない
                                   (LPDWORD)&entriesread, 
                                   (LPDWORD)&totalentries 
                                   ); 
#ifdef ADD_IDCACHE
		   if (sts == NERR_Success)
		   {
             if (nIDCashLiveTime > 0)  // 0 以上
			 {
		      SetIDCashList(mRcpt, mMailGroup, "gp");
			 }
		   }
#endif

#ifdef UPDATE_20060606 // ドメインコントローラへの接続リトライ
#ifdef TRACE_MODE
          if (nSenderLog) {
		    sprintf(str, "NetUserGetLocalGroups(%s / %s) = ", mAuthDomain, mRcpt);
		    switch (sts) {
		      case ERROR_ACCESS_DENIED:  strcat(str, "ERROR_ACCESS_DENIED\n"); break;
		      case ERROR_MORE_DATA:      strcat(str, "ERROR_MORE_DATA\n"); break;
		      case NERR_InvalidComputer: strcat(str, "NERR_InvalidComputer\n"); break;
		      case NERR_UserNotFound :   strcat(str, "NERR_UserNotFound\n"); break;
		      case NERR_Success:         strcat(str, "NERR_Success\n"); break;
		      default: sprintf(str, "NERR_Other %d\n", sts); break;
			}
            printfTrace(str);
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
#ifdef V3
	} else {
	  sts = confirms_user((pdom ? (pdom+1) : NULL),
		                   mRcpt,
						   "",
						   (struct _User_DB *)&userdb, 
						   (LPDWORD)&totalentries );
	}
#endif
      if (bDebug)
		printf("user check status(1) = %sfound local group (%d)\n", (sts ? "not " : ""), sts);
#ifdef TRACE_MODE
      if (nSenderLog) {
		sprintf(str, "user check status(1) = %sfound local group (%d)\n", (sts ? "not " : ""), sts);
        printfTrace(str);
	  }
#endif

      if (sts == NERR_Success) {
		bGroup = bCheck = FALSE;
#ifdef V3
	if (bUserMan) {
#endif
#ifdef LDAP_ON         // LDAPへのログイン処理でSMTP認証の確認を行うオプション // //CRAM-MD5は複合できないので、これを利用する場合は、PLAINかLOGINのみとします。
	    if (bLDAP) {
          bGroup = TRUE;
		} else
#endif
		{
#ifdef ADD_IDCACHE
		  if (bCache)
		  {
	        if (bLock = CheckIDCashList(mRcpt))
			{
		      totalentries = CountIDCashList(mRcpt, member, "gp", mMailGroup, &bGroup, &bCheck);
			} else { // ロックできないとき
			  totalentries = 0;
			  member[0] = '\x0';
			  bCheck = FALSE;
              bGroup = FALSE;
			}
            ReleaseIDChche(mRcpt);
		  } else 
#endif
		  {
#ifdef ADD_IDCACHE
	        if (nIDCashLiveTime > 0 && !bCache)  // 0 以上
			{
		      if (bLock = CheckIDCashList(mRcpt))
			  {
                DelIDCashList(mRcpt, "gp");
			  }
              ReleaseIDChche(mRcpt);
			}
#endif
  	        for (entries = 0; entries < totalentries; entries++) {
		     member[0] = '\x0';
             wcstombs( member, (user_info[0]+entries)->lgrui0_name, 128 );
		     if (bDebug)
               printf("%s's Local group check=(\"%s\" vs. \"%s\")\n", mRcpt, mMailGroup, member);
             //if (strcmp(member, "IMSUsers") == 0) {
             if (strcmp(member, mMailGroup) == 0) {
               bGroup = TRUE;
			   if (bDebug)
                 printf("match!!\n",member);
			 } else
			   if (bDebug)
                 printf("unmatch\n",member);
		     if (strstr(member,".")) {
			   bCheck = TRUE;
			 }
#ifdef ADD_IDCACHE
	        if (bLock)  // 0 以上
			{
	          AddIDCashList(mRcpt, member, "gp");
			}
#endif
			}
#ifdef ADD_IDCACHE
            if (bLock) 
			{
			  ReleaseIDChche(mRcpt);
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
  	      for (entries = 0; entries < totalentries; entries++) {
		     member[0] = '\x0';
#ifdef V3
	if (bUserMan) {
#endif
#ifdef LDAP_ON         // LDAPへのログイン処理でSMTP認証の確認を行うオプション // //CRAM-MD5は複合できないので、これを利用する場合は、PLAINかLOGINのみとします。
	  if (bLDAP) {
        strcpy(member, "");
	  } else 
#endif
	  {
#ifdef ADD_IDCACHE
		if (bCache)
		{
	      if (bLock = CheckIDCashList(mRcpt))
		  {
		    GetNumIDCashList(mRcpt, member, "gp", entries);
		  } else { // ロックできないとき
			member[0] = '\x0';
		  }
		} else 
#endif
		{
          wcstombs( member, (user_info[0]+entries)->lgrui0_name, 128 );
		}
	  }
#ifdef V3
	} else {
      strcpy( member, userdb.domain);
	}
#endif
	         if (bDebug)
               printf("%s's Local group check=(\"%s\" vs. \"%s\")\n", mRcpt, (pdom+1), member);
			 if (stricmp(member, (pdom+1)) == 0) {
               bUser = TRUE;
			   break;
			 }
		  }
		}
	    if (bGroup && bUser) {
          bsts = TRUE;
		  if (bDebug)
            printf("match!!\n",member);
		} else
          if (bDebug)
			printf("unmatch\n",member);
#ifdef V3
	if (bUserMan) {
#endif
#ifdef LDAP_ON         // LDAPへのログイン処理でSMTP認証の確認を行うオプション // //CRAM-MD5は複合できないので、これを利用する場合は、PLAINかLOGINのみとします。
	  if (bLDAP) {
        //strcpy(member, "");
	  } else 
#endif
	  {
		NetApiBufferFree(user_info[0]);
	  }
#ifdef V3
	}
#endif
	  }
	  /*
      if (!bsts) {
        bsts = GetPostMaster((char *)mRcpt);
	  }
	  */
	  if (!bsts) {
        ////////////////////////
/*
        if (strstr(mRcpt,"@"))
	      *pdom = '\x0';
		GetAliases((LPCTSTR)SOFT_ALIASES_REG, (char *)mRcpt, (char *)pdom);
*/
        //if (pdom)
          //*pdom = '@';
        ////////////////////////
	    if (!bsts) {
#ifdef MULTI_ML
		  if (pdom) { /// 2002.09.04 ドメイン有りのMLチェック
	        *pdom = '@';
            bsts = GetMLists((LPCTSTR)SOFT_LISTS_REG, (char *)mRcpt, &bReq);
		    if (!bsts) {
              if (pdom) { /// 2002.09.04 ドメイン無しのMLのチェック
	            *pdom = '\x0';
                bsts = GetMLists((LPCTSTR)SOFT_LISTS_REG, (char *)mRcpt, &bReq);
			  }
			} 
		  } else 
#endif
          bsts = GetMLists((LPCTSTR)SOFT_LISTS_REG, (char *)mRcpt, &bReq);
		  //if (GetMLists((LPCTSTR)SOFT_LISTS_REG, (char *)mRcpt, &bReq)) {
          if (bsts) {
			if (!bReq) { // メーリングリストの制御コマンドでない
			  *bML   = TRUE;
              sprintf(mkey,"%s\\%s", SOFT_LISTS_REG, mRcpt);
#ifdef REGTOFILE
			  if (nClustering) {
#ifdef UPDATE_20070521 // 予約語対策
                strcpy(mML, mRcpt);
                strtok(mML, "@");
                if (ReservedWords(mML)) {
                  strcpy(mML, mReservedWords);
                  strcat(mML, mRcpt);
				  sprintf(mkey,"%s\\%s", SOFT_LISTS_REG, mML);
				}
#endif
			  }
#endif
#ifdef MULTI_ML
              if (!QueryMLists((LPCTSTR)SOFT_LISTS_REG, mRcpt)) // ドメイン付MLが無い場合
                strtok(mkey,"@");
#else
              strtok(mkey,"@");
#endif
			  strcpy(ml, mRcpt);
              //strtok(ml,"@");
			  if (bNoReply)
			    *bNoReply = GetProfileIntEx((LPCTSTR)mkey, "NoReply", (int)FALSE);
              ////////////////////////////////////////////////////////////
              // WhoCanSend 0x1 = Member, 0x2 = Anyone, 0x3 = Moderator //
              ////////////////////////////////////////////////////////////
              nWhoCanSend = GetProfileIntEx((LPCTSTR)mkey, (LPCTSTR)"WhoCanSend", (INT)1); // Default Member
              switch(nWhoCanSend) {
                case 1: if (MLUserCheck(mFrom, mRcpt))
 			              bsts = TRUE;
					    else {
						  bsts = FALSE;
                          sprintf(mAnswer, "'%s' is not a member or moderator of the '%s' list.\nSorry, only list members and the moderator may send mail to this list.\nYou can join the list by sending a reply to this message, with \nSUBSCRIBE %s in the message body.\n", mFrom, ml, ml);
						}
	                    break;
	            case 2: bsts = TRUE;
	                    break;
	            case 3: GetProfileStringEx(mkey,"Moderator", "", mModerator, sizeof(mModerator));
	                    if (_stricmp(mFrom, mModerator) == 0)
 			              bsts = TRUE;
						else {
						  bsts = FALSE;
                          sprintf(mAnswer, "'%s' is not the moderator of the '%s' list.\nSorry, only the moderator may send mail to this list.\nYour message has been forwarded to the moderator.\n", mFrom, ml);
						}
						break;
			  }
			} else
			 bsts = TRUE;
		  }
		}
        //printf("user check status(2) = %sfound %s\n", (!bsts ? "not " : ""), (bMList ? "Lists" : "Aliases"));
		if (bDebug)
          printf("user check status(2) = %sfound Lists\n", (!bsts ? "not " : "") );
	  }
	}
	if (pdom) {
	   *pdom = '@';
	}
	if (!bsts && *mAnswer == '\x0')
      sprintf(mAnswer, " Your message has encountered delivery problems to local user.\n> (Originally addressed to %s)\n", mRcpt);
	return (bsts);
}

#ifdef ML_ASTER_OPTION
////////////////////////////////////////////////////////
// 内部ユーザー一覧取り出しＭＬ設定用 //2003.9.1 検討中

DWORD MLToALLUSER(char *pdom, struct _RCP *rcp, char *mRCP, DWORD nLNo, DWORD nMLDomDiv, DWORD nNoOfDiv, DWORD *nNo) {
#ifdef TRACE_MODE
   char str[LOG_BUFFER];
#endif
  DWORD sts = 0;
  char   mRCPDest[_MAX_PATH];
  DWORD  nListNo = nLNo; //nDiv, 
  //long               hF;
  //struct _finddata_t FD;
  //time_t  u0, u1, u2;

	if (nMLDomDiv == 0) { // 強制分割で無い場合はドメイン分割
	  sprintf(mRCPDest, mRCP, (pdom ? pdom : "@localhost"), (*nNo%nNoOfDiv+1)); // 配送先ドメインで分離
	  *nNo+=1;
///////////////// 入替え 2003.3.3
#ifndef V4
#ifdef TRACE_MODE
      if (nSenderLog) {
        sprintf(str, "%s:MLToALLUSER->SetRCPFile:%s\n", rcp->mMNo, mRCPDest);
        printfTrace(str);
	  }
#endif
#endif
      SetRCPFile(rcp, mRCPDest, FALSE, &nListNo);
	  sts = *nNo;
	} else {
	  sprintf(mRCPDest, mRCP, (nListNo%nMLDomDiv+1));
	  nLNo++;
#ifndef V4
#ifdef TRACE_MODE
      if (nSenderLog) {
        sprintf(str, "%s:MLToALLUSER->SetRCPFile:%s\n", rcp->mMNo, mRCPDest);
        printfTrace(str);
	  }
#endif
#endif
      SetRCPFile(rcp, mRCPDest, FALSE, NULL);
	  sts = nLNo; //nMLDomDiv;
	  //if (nLNo < (DWORD) nMLDomDiv)
	    //sts = nLNo;
	}
	return sts;
}

BOOL CheckUserGroups(char *pPDC, char *pDomain, char *pAccount) {
#ifdef TRACE_MODE
   char str[LOG_BUFFER];
#endif
    LPLOCALGROUP_USERS_INFO_0   user_info[1];
	NET_API_STATUS sts;
	BOOL   nSts = FALSE;
    DWORD i, ent, entriesread; 
	DWORD totalentries; 
	//LPBYTE  pbuff;
	wchar_t  /*wDom[65], */ wAuthServer[65], wuser[256];
	char     mDomain[256];
	BOOL     bCheck = FALSE;
#ifdef UPDATE_20060606 // ドメインコントローラへの接続リトライ
	int    nTry;
#endif

    if (*pPDC) { // pPDCは\\マシン名で受取る
	  mbstowcs( wAuthServer, pPDC, 65);
	}

    mbstowcs( wuser, pAccount, 256);
    ent = entriesread = totalentries = 0;
	do {
#ifdef UPDATE_20060606 // ドメインコントローラへの接続リトライ
	  nTry = 0;
	  do {
#endif
        sts = NetUserGetLocalGroups( (*pPDC ? wAuthServer : NULL),
                                   wuser, 
                                   (DWORD) 0, 
                                   (DWORD) LG_INCLUDE_INDIRECT,
                                   (LPBYTE *) user_info, 
                                   (DWORD) 1024,   // サイズが小さいとグループ一覧が取り出せない
                                   (LPDWORD)&entriesread, 
                                   (LPDWORD)&totalentries 
                                   ); 
#ifdef UPDATE_20060606 // ドメインコントローラへの接続リトライ
#ifdef TRACE_MODE
        if (nSenderLog) {
		  sprintf(str, "NetUserGetLocalGroups(%s / %s) = ", pDomain, pAccount);
		  switch (nSts) {
		    case ERROR_ACCESS_DENIED:  strcat(str, "ERROR_ACCESS_DENIED\n"); break;
		    case ERROR_MORE_DATA:      strcat(str, "ERROR_MORE_DATA\n"); break;
		    case NERR_InvalidComputer: strcat(str, "NERR_InvalidComputer\n"); break;
		    case NERR_UserNotFound :   strcat(str, "NERR_UserNotFound\n"); break;
		    case NERR_Success:         strcat(str, "NERR_Success\n"); break;
		    default: sprintf(str, "NERR_Other %d\n", sts); break;
		  }
          printfTrace(str);
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
 	  if (sts == NERR_Success) {
        ent += entriesread;
        for (i = 0; i < entriesread; i++) {
	       mDomain[0] = '\x0';
           wcstombs( mDomain, (user_info[0]+i)->lgrui0_name, 128 );
		   if (strstr(mDomain,".")) {
		     bCheck = TRUE;
		   }
		   if (bCheck && stricmp(mDomain, pDomain) == 0) {
             nSts = TRUE;
		     break;
		   } 
		}
		NetApiBufferFree(*user_info);
	  } else {
		break;
	  }
	} while(entriesread != totalentries);

    if (!bCheck && totalentries > 0) {
	  *pDomain = '\x0';
      nSts = TRUE;
	}

    return nSts;
}

NET_API_STATUS GetWinUserList(char *pName, char *pDom, struct _RCP *rcp, char *mRCP, DWORD nMLDomDiv, DWORD nNoOfDiv, DWORD *nListNo) {
#ifdef TRACE_MODE
   char str[LOG_BUFFER];
#endif
    DWORD i; 
    LPLOCALGROUP_MEMBERS_INFO_1 user_info[1];
    //LPLOCALGROUP_MEMBERS_INFO_3 user_info[1];
	wchar_t wDom[65], wAuthServer[65], wmember[256];
    NET_API_STATUS nSts;
	LPBYTE  pbuff;
	BOOL    bMach;
	char    *pAster, *pAfter, *pAcount;
	char    maccount[256], mdomain[256], mServer[64]; 
	DWORD   nr;
	DWORD   nNo, total, ent, entr, enttotal, rh;
#ifdef UPDATE_20060606 // ドメインコントローラへの接続リトライ
	int    nTry;
#endif

	/////////////////////////////////
    pAster = pAfter= NULL;
	if (*pName != '*') {
#ifdef UPDATE_20070319B // キャラクタ検索で見つからない場合ハングする
	  if ((pAster = strchr(pName, '*'))) {
  	    *pAster = '\x0';
	    pAfter = pAster+1;
	  }
#else
	  pAster = strchr(pName, '*');
  	  *pAster = '\x0';
	  pAfter = pAster+1;
#endif
	}
	/////////////////////////////////
	nNo = total = ent = entr = enttotal = rh = 0;
	mServer[0] = '\x0';
    if (mAuthDomain[0]){
      mbstowcs( wDom, mAuthDomain, 65);
#ifdef UPDATE_20060606 // ドメインコントローラへの接続リトライ
	  nTry = 0;
	  do {
#endif
	    nSts = NetGetAnyDCName(NULL, wDom, &pbuff);
	    if (nSts == NERR_Success) {
	      wcscpy(wAuthServer, (const wchar_t *)pbuff);
		  wcstombs(mServer, wAuthServer, sizeof(mServer));
          NetApiBufferFree(pbuff);
		}
#ifdef UPDATE_20060606 // ドメインコントローラへの接続リトライ
#ifdef TRACE_MODE
        if (nSenderLog) {
		  sprintf(str, "NetGetAnyDCName(%s / %s) = ", mAuthDomain, pName);
		  switch (nSts) {
		    case ERROR_NO_LOGON_SERVERS:          strcat(str, "ERROR_NO_LOGON_SERVERS\n"); break;
		    case ERROR_NO_SUCH_DOMAIN:            strcat(str, "ERROR_NO_SUCH_DOMAIN\n"); break;
		    case ERROR_NO_TRUST_LSA_SECRET:       strcat(str, "ERROR_NO_TRUST_LSA_SECRET\n"); break;
		    case ERROR_NO_TRUST_SAM_ACCOUNT:      strcat(str, "ERROR_NO_TRUST_SAM_ACCOUNT\n"); break;
            case ERROR_DOMAIN_TRUST_INCONSISTENT: strcat(str, "ERROR_DOMAIN_TRUST_INCONSISTENT\n"); break;
		    case NERR_Success:                    strcat(str, "NERR_Success\n"); break;
		    default: strcat(str, "NERR_Other\n"); break;
		  }
          printfTrace(str);
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
    mbstowcs( wmember, mMailGroup, 256);
	do {
#ifdef UPDATE_20060606 // ドメインコントローラへの接続リトライ
	  nTry = 0;
	  do {
#endif
	    nSts = NetLocalGroupGetMembers((mAuthDomain[0] ? wAuthServer : NULL),
                                        wmember,
										1, //3,
										(LPBYTE *) user_info,
										2048, //(*resumehandle == 0 ? 1024 : 256),// 2048, //8192,
                                        (LPDWORD)&nr, 
                                        (LPDWORD)&total,
										&rh);
#ifdef UPDATE_20060606 // ドメインコントローラへの接続リトライ
#ifdef TRACE_MODE
        if (nSenderLog) {
		  sprintf(str, "NetLocalGroupGetMembers(%s / %s) = ", mAuthDomain, pName);
		  switch (nSts) {
		    case ERROR_ACCESS_DENIED:          strcat(str, "ERROR_ACCESS_DENIED\n"); break;
		    case NERR_InvalidComputer:            strcat(str, "NERR_InvalidComputer\n"); break;
		    case ERROR_MORE_DATA:       strcat(str, "ERROR_MORE_DATA\n"); break;
		    case ERROR_NO_SUCH_ALIAS:      strcat(str, "ERROR_NO_SUCH_ALIAS\n"); break;
		    case NERR_Success:                    strcat(str, "NERR_Success\n"); break;
		    default: strcat(str, "NERR_Other\n"); break;
		  }
          printfTrace(str);
		}
#endif
		if (++nTry >= nADRetryTime) {// １０回までトライを試みる
		  break;
		}
		if (nSts != ERROR_ACCESS_DENIED &&
		    nSts != NERR_InvalidComputer &&
			nSts != ERROR_MORE_DATA &&
			nSts != ERROR_NO_SUCH_ALIAS &&
			nSts != NERR_Success)
#ifdef UPDATE_20091026 // AD連携の問い合わせリトライ間隔を短く設定しなおした
		  Sleep(nADRetryMSec);
#else
		  _sleep(1000);
#endif
	  } while (nSts != ERROR_ACCESS_DENIED &&
		       nSts != NERR_InvalidComputer &&
			   nSts != ERROR_MORE_DATA &&
			   nSts != ERROR_NO_SUCH_ALIAS &&
			   nSts != NERR_Success);
#endif
	   for (i = 0; i < nr; i++) {
	 	  maccount[0] = '\x0';
	      wcstombs( maccount,(user_info[0]+i)->lgrmi1_name, 256 );
		  bMach = FALSE;
		  if (!pAster)
	        bMach = TRUE;
		  else if (pAster && !strncmp(maccount, pName, strlen(pName))) {
			pAcount = &maccount[strlen(pName)];
			if ((!*pAcount && !*pAfter) || strstr(pAcount, pAfter))
		      bMach = TRUE;
		  }
		  if (bMach) {
		    strcpy(mdomain, pDom);
		    if (CheckUserGroups(mServer, mdomain, maccount)) {
			  // ユーザー設定
	          sprintf(rcp->mTo, "%s@%s", maccount, pDom);
			  *nListNo = MLToALLUSER(pDom-1, rcp, mRCP, *nListNo, nMLDomDiv, nNoOfDiv, &nNo);
			}
		  }
	   }
	 } while(rh);
    return *nListNo;
}

NET_API_STATUS GetSpaUserList(char *pName, char *pDom, struct _RCP *rcp, char *mRCP, DWORD nMLDomDiv, DWORD nNoOfDiv, DWORD *nListNo) {
    NET_API_STATUS    nSts = 0;
	char    mDB[256];
	FILE    *fp;
    char    mDec[1024], mUser[1024];
	BOOL    bMach;
	char    *p, *pAster, *pAfter, *pAcount;
	DWORD   nNo = 0;

	/////////////////////////////////
    pAster = pAfter= NULL;
	if (*pName != '*') {
	  pAster = strchr(pName, '*');
	  *pAster = '\x0';
	  pAfter = pAster+1;
	}
	/////////////////////////////////
	if (mAuthDomain[0] == '\x0') // アカウントフォルダが空白なら実行パスをアカウントフォルダにする。
      sprintf(mDB,"%s%s.idx", mProgPath, pDom);
    else
      sprintf(mDB,"%s%s.idx", mAuthDomain, pDom);
	/////////////////////////
    if ((fp = fopen(mDB, "rt" ))) {
#ifndef TUNING
      memset(mDec, 0, sizeof(mDec));
#else
	  mDec[0] = '\x0';
#endif
      p = fgets(mDec, sizeof(mDec), fp);
      while(p || !feof(fp)) {
	    strtok(mDec, " \n");
	    if (mDec[0] == '\x0' || mDec[0] == '\n')
		  break;
		Base64DecodeLine(mDec, strlen(mDec), (unsigned char *)mUser, sizeof(mUser));
        //pDoc->DecodeLine(mDec, strlen(mDec), (unsigned char *)mUser, sizeof(mUser));
        strtok(mUser, "\t");
        if (strlen(mUser)) {
		  // ユーザー設定
		  bMach = FALSE;
		  if (!pAster)
	        bMach = TRUE;
		  else if (pAster && !strncmp(mUser, pName, strlen(pName))) {
			pAcount = &mUser[strlen(pName)];
			if ((!*pAcount && !*pAfter) || strstr(pAcount, pAfter))
		      bMach = TRUE;
		  }
		  if (bMach) {
	        sprintf(rcp->mTo, "%s@%s", mUser, pDom);
		    *nListNo = MLToALLUSER(pDom-1, rcp, mRCP, *nListNo, nMLDomDiv, nNoOfDiv, &nNo);
		  }
		}
#ifndef TUNING
        memset(mDec, 0, sizeof(mDec));
#else
 	    mDec[0] = '\x0';
#endif
        p = fgets(mDec, sizeof(mDec), fp);
	  } 
      fclose(fp);
	}
    return *nListNo;
}

#ifdef LDAP_ON         // LDAPへのログイン処理でSMTP認証の確認を行うオプション // //CRAM-MD5は複合できないので、これを利用する場合は、PLAINかLOGINのみとします。
// 作成途中
NET_API_STATUS GetLDAPUserList(char *pName, char *pDom, struct _RCP *rcp, char *mRCP, DWORD nMLDomDiv, DWORD nNoOfDiv, DWORD *nListNo) {
    NET_API_STATUS    nSts = 0;
	char    mDB[256];
	FILE    *fp;
    char    mUser[256];
	BOOL    bMach;
	char    *p, *pAster, *pAfter, *pAcount;
	DWORD   nNo = 0;
	/////////////////////////////////
    LDAP *ld = NULL;
    int status = 0;
    LDAPMessage *result, *e;
    BerElement *ber;
    char *a, *dn;
    char **vals;
    int i;
	CHAR mScope[256], mNULL[] = "";
	CHAR mFilter[256], mKey[] = "*";
	LDAPD ldapd;
	/////////////////////////////////
    pAster = pAfter= NULL;
	if (*pName != '*') {
	  pAster = strchr(pName, '*');
	  *pAster = '\x0';
	  pAfter = pAster+1;
	}
	/////////////////////////////////
    // メンバリスト一覧
    // LDAPに問合せメンバの取得
    sprintf(mScope, "cn=%s,%s", mMailGroup, mLDAPSchemaDC);
    ldapd.pHost = (mAuthDomain[0] ? mAuthDomain : "localhost");
    ldapd.nPort = nLDAPPort;
    ldapd.pLogonDomain = mNULL;
    ldapd.pLogonID = mLDAPAdminID; 
    ldapd.pLogonPW = mLDAPAdminPW;
    ldapd.pScope = mScope;            //"cn=imsusers,cn=users,dc=ktinc,dc=jp"
    ldapd.pRequest1 = mKey;               //"*"
    ldapd.pRequest2 = mLDAPSchemaMember;  //"member"
    ldapd.pAnswer   = mUser;
    ////////////////////////////////////////////////////
	// LDAPへ接続
    if ((ld = ConnetLDAP(ld, &ldapd)) == NULL) {
      return 0;
	}
    ////////////////////////////////////////////////////
    // エントリを検索する
    printf("user list...\n");
	sprintf(mFilter, "%s=%s", mLDAPSchemaRDN, ldapd.pRequest1);
    status = ldap_search_s(ld,
		           ldapd.pScope,
			       LDAP_SCOPE_SUBTREE,
				   mFilter , 
				   NULL, 0, 
			       &result);

    if (status != LDAP_SUCCESS) {
      printf( "ldap_search_ext_s() failed with 0x%x.\n", LdapGetLastError() );
      goto FatalExit0;
    }

    //printf("\n");

    /* 全エントリについて名前と内容(属性と値)を印刷する */
    for (e = ldap_first_entry(ld, result);
	     e != NULL;
 	     e = ldap_next_entry(ld, e)) {  /* エントリを順番に取り出す */

	   dn = ldap_get_dn(ld, e);        /* DN の取得 */
	   if (dn != NULL) {
	      //printf("dn: %s\n", dn);
	      ldap_memfree(dn);
	   }

	   for (a = ldap_first_attribute(ld, e, &ber); a != NULL;
	        a = ldap_next_attribute(ld, e, ber)) { // 属性名を順番に取り出す
	     vals = ldap_get_values(ld, e, a);   // 属性値を取り出す
	     if (vals != NULL) {
		   for (i = 0; vals[i] != NULL; i++) {
		 	 if (!stricmp(a, ldapd.pRequest2) || *(ldapd.pRequest2) == '\x0') {
			   status = 1;
			   if ((p = strchr(vals[i], '='))) {
		 	     sprintf(mUser, p+1);
				 strtok(mUser, ",");
		         //printf("%s\n", mUser);
			   } else {
				 continue;
			   }
////////////////////////////
               {
		       // ユーザー設定
		         bMach = FALSE;
		         if (!pAster)
	               bMach = TRUE;
		         else if (pAster && !strncmp(mUser, pName, strlen(pName))) {
			       pAcount = &mUser[strlen(pName)];
			     if ((!*pAcount && !*pAfter) || strstr(pAcount, pAfter))
		            bMach = TRUE;
				 }
		         if (bMach) {
	               sprintf(rcp->mTo, "%s@%s", mUser, pDom);
		           *nListNo = MLToALLUSER(pDom-1, rcp, mRCP, *nListNo, nMLDomDiv, nNoOfDiv, &nNo);
				 }
			   }
/////////////////////
			 }
		   }
		   ldap_value_free(vals);  // 取り出した属性値の解放
		 }
	     ldap_memfree(a);            // 属性名の解放 
		}
	    //printf("\n");
	}
    ldap_msgfree(result);               /* 検索結果の解放 */

FatalExit0:
    if (ld)
     ldap_unbind_s( ld );


    return *nListNo;
}
#endif

#endif