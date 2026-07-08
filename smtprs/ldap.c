/////////////////////////////////////////////////////////////////////////////////
// ldap.c
//
// int  LDAP_UserList(LDAPD *ldapd); // 一覧
//      <ldap server> <auth id> <pw> <account dn> <filter> <account cn>
//
// int  LDAP_UserAdd(LDAPD *ldapd);  // アカウント追加
//      <ldap server> <auth id> <pw> <account dn> <account>
//
// int  LDAP_GroupAdd(LDAPD *ldapd); // グループ追加
//      <ldap server> <auth id> <pw> <group dn> <account>
//
// int  LDAP_PwdAdd(LDAPD *ldapd); // パスワード設定
//      <ldap server> <auth id> <pw> <account dn> <account pw>
//
// int  LDAP_UserDel(LDAPD *ldapd); // 削除
//      <ldap server> <auth id> <pw> <account dn>
//
// int  LDAP_UserSAMRename(LDAPD *ldapd); // SAMAccountName値変更
//      <ldap server> <auth id> <pw> <account dn> <new account>
//
// int  LDAP_UserRename(LDAPD *ldapd); // cn値変更
//      <ldap server> <auth id> <pw> <account dn> <new account>
//
// BOOL LDAP_LOGON(LDAPD *ldapd); // ユーザー認証
//      <ldap server> <auth id> <pw>
//
/////////////////////////////////////////////////////////////////////////////////
#include "smtp.h"

#ifdef LDAP_ON         // LDAPへのログイン処理でSMTP認証の確認を行うオプション // //CRAM-MD5は複合できないので、これを利用する場合は、PLAINかLOGINのみとします。
//#include <Winber.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <winldap.h>
#include <Iads.h>
#include <Adshlp.h>

extern char mAuthDomain[];

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

extern BOOL bDebug;

#ifdef UPDATE_20091026 // AD連携の問い合わせリトライ間隔を短く設定しなおした
extern DWORD  nLDAPRetryMSec;
#endif
#ifdef UPDATE_20060615 // LDAPサーバーへの接続リトライ
#ifdef UPDATE_20091103 // LDAP連携専用のリトライ回数指定の変数に変更
extern DWORD  nLDAPRetryTime;
#else
extern DWORD  nADRetryTime;
#endif
#endif
#ifdef UPDATE_20070521 // OSの予約語対策
extern CHAR   mReservedWords[];
#endif

typedef struct berval LDAP_BERVAL, *PLDAP_BERVAL, BERVAL, *PBERVAL;

/*
typedef struct ldap_data
{
  char *pHost;
  DWORD nPort;
  char *pLogonDomain;
  char *pLogonID;
  char *pLogonPW;
  char *pScope;
  char *pMailAddr;
  char *pRequest1;
  char *pRequest2;
} LDAPD;
*/

void translateUue2Base64(char *line, int inlen, char *newline);

LDAP *ConnetLDAP(LDAP *ld, LDAPD* ldapd); //LDAPへの接続
int  LDAP_UserList(LDAPD *ldapd); // 一覧
int  LDAP_UserAdd(LDAPD *ldapd);  // 追加
int  LDAP_GroupAdd(LDAPD *ldapd); // グループ追加
int  LDAP_PwdAdd(LDAPD *ldapd); // パスワード設定
int  LDAP_UserDel(LDAPD *ldapd); // 削除
int  LDAP_UserSAMRename(LDAPD *ldapd); // SAMAccountName値変更
int  LDAP_UserRename(LDAPD *ldapd); // cn値変更
BOOL LDAP_LOGON(LDAPD *ldapd); // ユーザー認証
#ifdef UPDATE_20091014D // LDAPへのアカウント認証時、ドメインに含まれないアカウントの認証も成功してしまう不具合
void LDAP_GetUserDN(CHAR *lpszUser, CHAR *lpszUserDomain, CHAR *pDN);
#endif

/*
int main( int cArgs, char  *pArgs[] )
{
   LDAP *ld = NULL;
   LDAPD  ldapd;
   LDAPMessage *results = NULL;
   ULONG version = LDAP_VERSION3;
   char *p, *q, mNULL[] = "";
   char mDomain[2][256], mMailAddr[256];

   ///////////////////////////////////////////
   /// 初期化
   ldapd.pHost        = mNULL;
   ldapd.pLogonDomain = mNULL;
   ldapd.pLogonID     = mNULL;
   ldapd.pLogonPW     = mNULL;
   ldapd.pScope       = mNULL;
   ldapd.pMailAddr    = mNULL;
   ldapd.pRequest1    = mNULL;
   ldapd.pRequest2    = mNULL;
   mDomain[1][0]      = '\x0';
   mMailAddr[0]       = '\x0';
   ///////////////////////////////////////////
   // 因数設定
   if (bDebug) printf( "\n" );
   if (cArgs > 1)
     ldapd.pHost     = pArgs[1];  // host名
   if (cArgs > 2)
	 ldapd.pLogonID  = pArgs[2];  // 認証アカウント
   if (cArgs > 3)
     ldapd.pLogonPW  = pArgs[3];  // パスワード
   if (cArgs > 4) {
	 ldapd.pScope    = pArgs[4]; // cn=<username>,cn=users,dc=ktinc,dc=jp
	 strcpy(mDomain[0], pArgs[4]);
	 strlwr(mDomain[0]);
	 p = mDomain[0];
	 while((q = strstr(p, "dc="))) {
	   if (p = strchr(q, ',')) {
		 *p = '\x0';
	   }
	   strcat(mDomain[1], q+3);
	   if (p) {
	     strcat(mDomain[1], ".");
	     p++;
	   } else {
		 break;
	   }
	 }
   }
   if (cArgs > 5) {
     ldapd.pRequest1 = pArgs[5]; // ユーザー名等任意
	 sprintf(mMailAddr, "%s@%s", pArgs[6], mDomain[1]);
	 ldapd.pMailAddr = mMailAddr;
   }
   if (cArgs > 6) 
     ldapd.pRequest2 = pArgs[6]; // ユーザー名等任意
   ///////////////////////////////////////////
   if (bDebug) printf("LDAP Port = %d\n", LDAP_PORT);

   // 操作試験
   //LDAP_LOGON(&ldapd); // ユーザー認証
   //LDAP_UserList(&ldapd); // ユーザ一覧
   LDAP_UserAdd(&ldapd);   //ユーザ作成
   //LDAP_GroupAdd(&ldapd);  // グループへユーザ設定
   //LDAP_PwdAdd(&ldapd);    // ユーザパスワード設定
   //LDAP_UserSAMRename(&ldapd); // SAMAccountname 変更
   //LDAP_UserRename(&ldapd); // cn 変更
   //LDAP_UserDel(&ldapd); // 削除

   return 0;
}   // main
*/

LDAP * ConnetLDAP(LDAP *ld, LDAPD* ldapd) {
  ULONG version = LDAP_VERSION3;
  ULONG AuthMethod = LDAP_AUTH_NTLM; // Set up AuthId for NTLM authentication
  //ULONG AuthMethod = LDAP_AUTH_SIMPLE;
  BOOL bsts = FALSE;
  INT err;
  SEC_WINNT_AUTH_IDENTITY AuthId;
  CHAR mDN[256];
#ifdef UPDATE_20060615 // LDAPサーバーへの接続リトライ
  int    nTry;
#endif
#ifdef UPDATE_20091014D // LDAPへのアカウント認証時、ドメインに含まれないアカウントの認証も成功してしまう不具合
  CHAR mUserDN[256];
#endif
#ifdef UPDATE_20070620 // ドメインごとにDNを設定可能にする
#ifdef UPDATE_20070521 // 予約語対策
  CHAR mKey[256], mBDN[256];
  strcpy(mKey, ldapd->pLogonDomain);
  strtok(mKey, "@");
  if (ReservedWords(mKey)) {
    strcpy(mKey, mReservedWords);
    strcat(mKey, ldapd->pLogonDomain);
  } else
#endif
  {
    strcpy(mKey, ldapd->pLogonDomain);
  }
  GetProfileStringEx((LPCTSTR)DOMAIN_BASEDN, (char *)mKey, "", mBDN, sizeof(mBDN)); // 対象ドメインのDN名
#endif

  // Set up AuthId for NTLM authentication
  AuthId.User = strlen(ldapd->pLogonID) ? ldapd->pLogonID :  NULL;
  AuthId.UserLength = strlen(ldapd->pLogonID);
  AuthId.Domain = strlen(ldapd->pLogonDomain) ? ldapd->pLogonDomain :  NULL;
  AuthId.DomainLength = strlen(ldapd->pLogonDomain);
  AuthId.Password = strlen(ldapd->pLogonPW) ? ldapd->pLogonPW :  NULL;
  AuthId.PasswordLength = strlen(ldapd->pLogonPW);
  #ifdef UNICODE
  AuthId.Flags = SEC_WINNT_AUTH_IDENTITY_UNICODE;
  #else
  AuthId.Flags = SEC_WINNT_AUTH_IDENTITY_ANSI;
  #endif

  if (AuthId.User)
  {
    // セッションハンドルの取得 
    if (!strnicmp(AuthId.User, "cn=", 3))
	{
      strcpy(mDN, AuthId.User);
    } else {
#ifdef UPDATE_20091014D // LDAPへのアカウント認証時、ドメインに含まれないアカウントの認証も成功してしまう不具合
	  mDN[0] = '\x0';
      LDAP_GetUserDN(AuthId.User, AuthId.Domain, mUserDN);
	  if (mUserDN[0])
	  {
		strcpy(mDN, mUserDN);
	  } else {
	    sprintf(mDN, "%s=%s,%s", mLDAPSchemaRDN, AuthId.User, (mBDN[0] ? mBDN : mLDAPSchemaDC));
	  }
#else
#ifdef UPDATE_20070620 // ドメインごとにDNを設定可能にする
	  sprintf(mDN, "%s=%s,%s", mLDAPSchemaRDN, AuthId.User, (mBDN[0] ? mBDN : mLDAPSchemaDC));
#else
      sprintf(mDN, "%s=%s,%s", mLDAPSchemaRDN, AuthId.User, mLDAPSchemaDC);
#endif
#endif
	}
  } else {
#ifdef UPDATE_20070620 // ドメインごとにDNを設定可能にする
	sprintf(mDN, "%s=%s,%s", mLDAPSchemaRDN, AuthId.User, (mBDN[0] ? mBDN : mLDAPSchemaDC));
#else
    sprintf(mDN, "%s=%s,%s", mLDAPSchemaRDN, AuthId.User, mLDAPSchemaDC);
#endif
  }
  if (bDebug) printf( "Connecting to server %s....\n", ldapd->pHost );
  ld = ldap_sslinit( ldapd->pHost, ldapd->nPort, 0 ); 
  if (ld) {
    ld->ld_lberoptions = 0;
    err = ldap_set_option( ld, LDAP_OPT_VERSION, (void *)&version );
    if (err == LDAP_SUCCESS) {
#ifdef UPDATE_20060615 // LDAPサーバーへの接続リトライ
	  nTry = 0;
	  do {
	    err = ldap_simple_bind_s(ld, (PCHAR)mDN , (PCHAR)AuthId.Password) ;
        if (err == LDAP_SUCCESS) {
	      bsts = TRUE;
		} else if (err == LDAP_TIMEOUT ||    // 0x55
		        err == LDAP_UNAVAILABLE ||   // 0x34
                err == LDAP_SERVER_DOWN)     // 0x51
		{
#ifdef UPDATE_20170207 //LDAP接続失敗時のリトライ処理の修正
          if (bDebug) printf( "ldap_simple_bind_s()=%d %s:%d (%s)\n", err, ldapd->pHost, ldapd->nPort, mDN);
          ldap_unbind_s( ld );
          ld = NULL;
          ld = ldap_sslinit( ldapd->pHost, ldapd->nPort, 0 ); 
	      ldap_set_option( ld, LDAP_OPT_VERSION, (void *)&version );
#endif
#ifdef UPDATE_20091026 // AD連携の問い合わせリトライ間隔を短く設定しなおした
          Sleep(nLDAPRetryMSec);
#else
		  Sleep(6000);
#endif
		}
	  } while ((err == LDAP_TIMEOUT ||
		        err == LDAP_UNAVAILABLE ||
                err == LDAP_SERVER_DOWN) &&
#ifdef UPDATE_20091103 // LDAP連携専用のリトライ回数指定の変数に変更
		        ++nTry < nLDAPRetryTime);
#else
		        ++nTry < nADRetryTime);
#endif
      if (bDebug) printf( "ldap_simple_bind_s() bind returned 0x%x\n", err );
#else
	  err = ldap_simple_bind_s(ld, (PCHAR)mDN , (PCHAR)AuthId.Password) ;
#endif
      if (err == LDAP_SUCCESS) {
		bsts = TRUE;
#ifdef UPDATE_20091014D // LDAPへのアカウント認証時、ドメインに含まれないアカウントの認証も成功してしまう不具合
	  } else if (!strnicmp(AuthId.User, "cn=", 3)) {
#else
	  } else {
#endif
        err = ldap_bind_s( ld, NULL, (TCHAR *) &AuthId, AuthMethod );
        if (bDebug) printf( "ldap_bind_s() bind returned 0x%x\n", err );
        if (err == LDAP_SUCCESS) {
		  bsts = TRUE;
		} else {
          ldap_unbind_s( ld );
          ld = NULL;
		}
#ifdef UPDATE_20091014D // LDAPへのアカウント認証時、ドメインに含まれないアカウントの認証も成功してしまう不具合
	  } else {
        ldap_unbind_s( ld );
        ld = NULL;
#endif
	  }
	} else {
      ldap_unbind_s( ld );
      ld = NULL;
      if (bDebug) printf( "ldap_set_option() returned 0x%x\n", err );
	}
  } else {
    if (bDebug) printf( "ld failed with 0x%x.\n", LdapGetLastError() );
  }
  return ld;
}

#ifdef UPDATE_20091014D // LDAPへのアカウント認証時、ドメインに含まれないアカウントの認証も成功してしまう不具合
void LDAP_GetUserDN(CHAR *lpszUser, CHAR *lpszUserDomain, CHAR *pDN)
{
	LDAPD ldapd;
    LDAP *ld = NULL;
    int status = 0;

    LDAPMessage *result, *e;
    BerElement *ber;
    char *a, *dn;
    char **vals;
    int i;
	CHAR mFilter[256], mScope[256], mNULL[] = "";
    CHAR *p, mKey[256] = "", mDN[256], mAnser[256]; 

#ifdef UPDATE_20070521 // 予約語対策
    strcpy(mKey, (lpszUserDomain ? lpszUserDomain : ""));
    strtok(mKey, "@");
    if (ReservedWords(mKey)) {
      strcpy(mKey, mReservedWords);
      strcat(mKey, (lpszUserDomain ? lpszUserDomain : ""));
	} else
#endif
	{
      strcpy(mKey, (lpszUserDomain ? lpszUserDomain : ""));
	}
 	GetProfileStringEx((LPCTSTR)DOMAIN_BASEDN, (char *)mKey, "", mDN, sizeof(mDN)); // 対象ドメインのDN名
    strcpy(mScope, (mDN[0] ? mDN : mLDAPSchemaDC));
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
	ldapd.pAnswer   = mAnser;
    ////////////////////////////////////////////////////
	// LDAPへ接続
    if ((ld = ConnetLDAP(ld, &ldapd)) == NULL) {
      return;// 0;
	}
    ////////////////////////////////////////////////////
    // エントリを検索する
    status = ldap_search_s(ld,
		           mScope,
			       LDAP_SCOPE_SUBTREE,
				   mFilter , 
				   NULL, 0, 
			       &result);

    if (status == LDAP_SUCCESS)
	{
      // エントリのＤＮを取得
      if (e = ldap_first_entry(ld, result))
	  {
	    dn = ldap_get_dn(ld, e);        // DN の取得 
	    if (dn != NULL) 
		{
	      if (bDebug) printf("dn: %s\n", dn);
	      strcpy(pDN, dn);
	      ldap_memfree(dn);
		}
	  }
      ldap_msgfree(result);               // 検索結果の解放 
	} else {
      if (bDebug) printf( "ldap_search_ext_s() failed with 0x%x.\n", LdapGetLastError() );
    } 

    if (ld)
	{
      ldap_unbind_s( ld );
	}

    return;// status;
}

#endif

int LDAP_UserList(LDAPD *ldapd) // 検索
{
    LDAP *ld = NULL;
    int status = 0;

    LDAPMessage *result, *e;
    BerElement *ber;
    char *a, *dn;
    char **vals;
    int i;
	char mFilter[256];
    
    ////////////////////////////////////////////////////
	// LDAPへ接続
    if ((ld = ConnetLDAP(ld, ldapd)) == NULL) {
      return 0;
	}
    ////////////////////////////////////////////////////
    // エントリを検索する
    if (bDebug) printf("user list...\n");
	sprintf(mFilter, "%s", ldapd->pRequest1);
    status = ldap_search_s(ld,
		           ldapd->pScope,
			       LDAP_SCOPE_SUBTREE,
				   mFilter , 
				   NULL, 0, 
			       &result);

    if (status != LDAP_SUCCESS) {
      if (bDebug) printf( "ldap_search_ext_s() failed with 0x%x.\n", LdapGetLastError() );
      goto FatalExit0;
    } else {
#ifdef UPDATE_20091014C // LDAPへのアカウント問合せ処理の高速化
	  status = 2;
#else
	  status = 1;
      if (ldapd->pRequest2 == NULL)
        goto FatalExit0;
#endif
	}

    if (bDebug) printf("\n");

    /* 全エントリについて名前と内容(属性と値)を印刷する */
    for (e = ldap_first_entry(ld, result);
	     e != NULL;
 	     e = ldap_next_entry(ld, e)) {  /* エントリを順番に取り出す */
#ifdef UPDATE_20091014C // LDAPへのアカウント問合せ処理の高速化
	  status = 1;
#endif
	   dn = ldap_get_dn(ld, e);        /* DN の取得 */
	   if (dn != NULL) {
	      if (bDebug) printf("dn: %s\n", dn);
	      ldap_memfree(dn);
	   }
#ifdef UPDATE_20091014C // LDAPへのアカウント問合せ処理の高速化
      if (ldapd->pRequest2 == NULL)
	  {
        break;
	  }
#endif
	for (a = ldap_first_attribute(ld, e, &ber); a != NULL;
	     a = ldap_next_attribute(ld, e, ber)) { // 属性名を順番に取り出す
	    vals = ldap_get_values(ld, e, a);   // 属性値を取り出す
	    if (vals != NULL) {
	      for (i = 0; vals[i] != NULL; i++) {
		    if (!stricmp(a, ldapd->pRequest2) || *(ldapd->pRequest2) == '\x0')
			{
			  if (ldapd->pAnswer)
			    sprintf(ldapd->pAnswer, vals[i]);
		      if (bDebug) printf("%s: %s\n", a, vals[i]);
			  break;
			} else {
			  break;
			}
		  }
		  ldap_value_free(vals);  // 取り出した属性値の解放
	    }
	    ldap_memfree(a);            // 属性名の解放 
	}

	//if (ber != NULL) {
	//  ber_free(ber, 0);
	//}

	  if (bDebug) printf("\n");
	}
    ldap_msgfree(result);               /* 検索結果の解放 */

FatalExit0:
    if (ld)
     ldap_unbind_s( ld );

    return status;
}

int LDAP_UserAdd(LDAPD *ldapd) // 検索
{
    LDAP *ld = NULL;
    int status, i;

    LDAPMod LP[6];
    LDAPMod *NewEntry[7];

    ////////////////////////////////////////////////////
	// アカウント情報設定
	// pMailUser = xxxxx
    char *pLp0[2] = { (char *)ldapd->pRequest1, NULL}; //cn
    char *pLp1[2] = { (char *)ldapd->pRequest2, NULL}; //samaccountname
	char *pLp2[2] = { (char *)ldapd->pRequest1, NULL}; //displayName: = fullname
	char *pLp3[2] = { (char *)ldapd->pMailAddr, NULL}; //mail
    char *pLp4[2] = { "user", NULL};// objectclass
	char *pLp5[2] = { "66080", NULL};// userAccountControl アカウントの有効化 + パスワード無期限
	//////////////////////////////////////////////////////////
	// userAccountControl フラグ定数 0x10220
    //int UF_ACCOUNTDISABLE     = 0x0002;   // アカウントを無効状態に
    //int UF_PASSWD_NOTREQD     = 0x0020;   // パスワードは必要なし
    //int UF_NORMAL_ACCOUNT     = 0x0200;   // 通常のユーザアカウント
    //int UF_DONT_EXPIRE_PASSWD = 0x10000;  // パスワードを無期限にする
	//////////////////////////////////////////////////////////
    LP[0].mod_op = LDAP_MOD_ADD; LP[0].mod_type = "cn";             LP[0].mod_values = pLp0;
    LP[1].mod_op = LDAP_MOD_ADD; LP[1].mod_type = "samaccountname"; LP[1].mod_values = pLp1;
    LP[2].mod_op = LDAP_MOD_ADD; LP[2].mod_type = "displayname";   LP[2].mod_values = pLp2;
    LP[3].mod_op = LDAP_MOD_ADD; LP[3].mod_type = "mail";           LP[3].mod_values = pLp3;
    LP[4].mod_op = LDAP_MOD_ADD; LP[4].mod_type = "objectclass";    LP[4].mod_values = pLp4;
    LP[5].mod_op = LDAP_MOD_ADD; LP[5].mod_type = "useraccountcontrol";  LP[5].mod_values = pLp5;
	for (i = 0; i < 6; i++)
      NewEntry[i] = &LP[i];
     NewEntry[i] = NULL;
    ////////////////////////////////////////////////////
	// LDAPへ接続
    if ((ld = ConnetLDAP(ld, ldapd)) == NULL) {
      return -1;
	}

    //add the entry
	printf("pScope=%s\n", ldapd->pScope);
    status = ldap_add_s( ld, ldapd->pScope, NewEntry);

    if (status != LDAP_SUCCESS) {
      if (bDebug) printf( "ldap_add_s() failed with 0x%x.\n", LdapGetLastError() );
    }

    if (ld)
      ldap_unbind( ld );
    return 0;
}

int LDAP_UserDel(LDAPD *ldapd) // 削除
{
    LDAP *ld = NULL;
    int status;

    ////////////////////////////////////////////////////
	// LDAPへ接続
    if ((ld = ConnetLDAP(ld, ldapd)) == NULL) {
      return -1;
	}

    // 削除操作
    status = ldap_delete_s(ld, ldapd->pScope);
    if (status != LDAP_SUCCESS) {
      if (bDebug) printf( "ldap_delete_s() failed with 0x%x.\n", LdapGetLastError() );
   }

    if (ld)
      ldap_unbind( ld );

    return 0;
}

int LDAP_UserSAMRename(LDAPD *ldapd) // 値変更
{
    LDAP *ld = NULL;
    int status, i;

    LDAPMod LP[4];
    LDAPMod *NewEntry[5];

    ////////////////////////////////////////////////////
	// アカウント情報設定
	// ldapd->pRequest1 = xxxxx
    char *pLp0[2] = { (char *)ldapd->pRequest1, NULL}; //samaccountname
	//////////////////////////////////////////////////////////
    LP[0].mod_op = LDAP_MOD_REPLACE; LP[0].mod_type = "samaccountname"; LP[0].mod_values = pLp0;
	for (i = 0; i < 1; i++)
      NewEntry[i] = &LP[i];
     NewEntry[i] = NULL;
    ////////////////////////////////////////////////////
	// LDAPへ接続
    if ((ld = ConnetLDAP(ld, ldapd)) == NULL) {
      return -1;
	}

    //add the entry
	printf("pScope=%s\n", ldapd->pScope);
    status = ldap_modify_s( ld, ldapd->pScope, NewEntry);

    if (status != LDAP_SUCCESS) {
      if (bDebug) printf( "ldap_add_s() failed with 0x%x.\n", LdapGetLastError() );
    }

    if (ld)
      ldap_unbind( ld );
    return 0;
}

int LDAP_UserRename(LDAPD *ldapd) // 値変更
{
    LDAP *ld = NULL;
    int status, i;

    LDAPMod LP[4];
    LDAPMod *NewEntry[5];

    ////////////////////////////////////////////////////
	// アカウント情報設定
	// pMailUser = xxxxx
    char *pLp0[2] = { (char *)ldapd->pRequest1, NULL}; //cn
	//////////////////////////////////////////////////////////
    LP[0].mod_op = LDAP_MOD_REPLACE; LP[0].mod_type = "cn"; LP[0].mod_values = pLp0;
	for (i = 0; i < 1; i++)
      NewEntry[i] = &LP[i];
     NewEntry[i] = NULL;
    ////////////////////////////////////////////////////
	// LDAPへ接続
    if ((ld = ConnetLDAP(ld, ldapd)) == NULL) {
      return -1;
	}

    //add the entry
	printf("pScope=%s\n", ldapd->pScope);
	status = ldap_modrdn_s(ld, ldapd->pScope, ldapd->pRequest1);
		                //"cn=0004,cn=users,dc=ktinc,dc=jp", OldName
                        //"cn=0005,cn=users,dc=ktinc,dc=jp"); NewName

    if (status != LDAP_SUCCESS) {
      if (bDebug) printf( "ldap_modrdn_s() failed with 0x%x.\n", LdapGetLastError() );
    }

    if (ld)
      ldap_unbind( ld );
    return 0;
}
///////////////////////////////////////////
// アカウント認証
// 戻り値
// 認証成功 : TRUE
// 認証失敗 : FALSE
///////////////////////////////////////////
BOOL LDAP_LOGON(LDAPD *ldapd) // ユーザー認証
{
    LDAP *ld = NULL;
	BOOL bsts = FALSE;

    ////////////////////////////////////////////////////
	// LDAPへ接続
    if ((ld = ConnetLDAP(ld, ldapd)) != NULL) {
	  if (bDebug) printf("id=<%s> ... OK.\n", ldapd->pLogonID);
      ldap_unbind( ld );
	  bsts = TRUE;
	} else {
	  if (bDebug) printf("id=<%s> pw=<%s>... NG.\n", ldapd->pLogonID, ldapd->pLogonPW);
	}

    return bsts;
}

///////////////////////////////////////////
// 既存のアカウントにグループを追加する
///////////////////////////////////////////
int LDAP_GroupAdd(LDAPD *ldapd) // 検索
{
    LDAP *ld = NULL;
    int status, i;

    LDAPMod LP[1];
    LDAPMod *NewEntry[2];
	char mJOIN[256];

    ////////////////////////////////////////////////////
	// アカウント情報設定
	// ldapd->pRequest1 = xxxxx
    char *pLp0[2] = {mJOIN, NULL}; //cn
	sprintf(mJOIN,"CN=%s,CN=Users,DC=ktinc,DC=jp", ldapd->pRequest1);

    ////////////////////////////////////////////////////
    LP[0].mod_op = LDAP_MOD_ADD; LP[0].mod_type = "member";             LP[0].mod_values = pLp0;

	for (i = 0; i < 1; i++)
      NewEntry[i] = &LP[i];
     NewEntry[i] = NULL;
    ////////////////////////////////////////////////////
	// LDAPへ接続
    if ((ld = ConnetLDAP(ld, ldapd)) == NULL) {
      return -1;
	}

    //add the entry
	printf("pScope=%s\n", ldapd->pScope);
    status = ldap_modify_s(ld, ldapd->pScope, NewEntry);

    if (status != LDAP_SUCCESS) {
      if (bDebug) printf( "ldap_add_s() failed with 0x%x.\n", LdapGetLastError() );
    }

    if (ld)
      ldap_unbind( ld );
    return 0;
}

///////////////////////////////////////////
// 既存のアカウントにパスワードを設定する
///////////////////////////////////////////
int LDAP_PwdAdd(LDAPD *ldapd) // 検索
{
    LDAP *ld = NULL;
    int status, i;

    LDAPMod LP[1];
    LDAPMod *NewEntry[2];
	BERVAL pwdBerVal;
	BERVAL *pwd_attr[2];

	CHAR    mPwd[128];
	CHAR    mPwdB64[4096];
	WCHAR   wPwd[256];
    ////////////////////////////////////////////////////
	// アカウント情報設定
    char *pLp0[2] = {(char *)mPwdB64, NULL}; //cn
    ////////////////////////////////////////////////////
    LP[0].mod_op = LDAP_MOD_REPLACE | LDAP_MOD_BVALUES;
	LP[0].mod_type = "userPassword"; //"unicodepwd";
	LP[0].mod_values = pwd_attr;
    ////////////////////////////////////////////////////
	sprintf(mPwd, "\"%s\"",ldapd->pRequest1); // User Password;
    ////////////////////////////////////////////////////
	memset(wPwd, 0, sizeof(wPwd) * sizeof(WCHAR));
	i = mbstowcs( wPwd, mPwd, strlen(mPwd));
    ////////////////////////////////////////////////////
	//Base64に変換
    translateUue2Base64(wPwd, (wcslen(wPwd) * sizeof(WCHAR)), mPwdB64);
	//translateUue2Base64(mPwd, strlen(mPwd), mPwdB64);
    ////////////////////////////////////////////////////
	pwd_attr[0] = &pwdBerVal;
	pwd_attr[1]= NULL;
	pwdBerVal.bv_len = strlen(mPwdB64); //wcslen(wPwd) * sizeof(WCHAR);
	pwdBerVal.bv_val = (char*)mPwdB64; //wPwd;
    ////////////////////////////////////////////////////

	for (i = 0; i < 1; i++)
      NewEntry[i] = &LP[i];
     NewEntry[i] = NULL;
    ////////////////////////////////////////////////////
	// LDAPへ接続
    if ((ld = ConnetLDAP(ld, ldapd)) == NULL) {
      return -1;
	}

    //add the entry
	if (bDebug) printf("pScope=%s\n", ldapd->pScope);
    status = ldap_modify_s(ld, ldapd->pScope, NewEntry);

    if (status != LDAP_SUCCESS) {
      if (bDebug) printf( "ldap_add_s() failed with 0x%x.\n", LdapGetLastError() );
    }

    if (ld)
      ldap_unbind( ld );
    return 0;
}
#endif