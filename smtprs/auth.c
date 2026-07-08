////////////////////////////////////////////////////////////
// Helo.c Copyright K.kawakami
////////////////////////////////////////////////////////////
#include "smtp.h"
#include <time.h>

#ifdef ESMTP_AUTH
extern BOOL  bAcceptLog;
extern CHAR  mMailBoxDir[];
extern char  mMailSpoolDir[];
#ifdef Y2038_BUG
extern char mMonth[12][4];
extern char mWeek[7][4];
#endif
#ifdef CLUSTERING
extern BOOL   nClustering;
extern char   mComName[];
#endif
extern BOOL  bUserMan;
extern char mAuthDomain[];
#ifdef LDAP_ON         // LDAPへのログイン処理でSMTP認証の確認を行うオプション // //CRAM-MD5は複合できないので、これを利用する場合は、PLAINかLOGINのみとします。
extern BOOL  bLDAP;
extern DWORD  nLDAPPort;
#endif
#ifdef UPDATE_20140704 // SMTP認証方法の指定が選択されていない指定で認証が応答してしまう不具合
extern char  mSMTPAUTHMODE[];
#endif
#ifdef ADD_IDCACHE
extern int  nIDCashLiveTime; // ADキャッシュ利用有効時間(秒単位)
#endif
#ifdef UPDATE_20160113 // 認証時のロックアウト機能を追加
extern DWORD nAuthLockOut; // ロックアウトまでの回数 デフォルト 0:無効
#endif
#ifdef UPDATE_20241226 // AUTH LOGINでの応答時チャレンジ文字列の設定をレジストリから変更できるようにした。
extern CHAR mChallengeTokenLOGINUser[];
#endif

void GetBaseDirectory(CHAR *BaseDir, char *mMBdir, char *muser, char *myaddr);
#ifdef UPDATE_20050927 // SMTP AUTH のユーザＩＤ情報の格納
BOOL GetPasswordFile(char  *pDir, char *pUser, char *mpass, DWORD *nFSL);
#else
BOOL GetPasswordFile(char  *pDir, char *pUser, char *mpass);
#endif
int  Base64DecodeLine(char * pszLine, int nLineLen, unsigned char * uchBuffer, int nBufferLen);
void translateUue2Base64(char *line, int inlen, char *newline);
#ifdef Y2038_BUG
void gettime(SYSTEMTIME *ltime, char *mt);
#else
void gettime(time_t *ltime, char *mt);
#endif
#ifdef UPDATE_20160113 // 認証時のロックアウト機能を追加
#ifdef UPDATE_20180819B // 認証成功時にロックアウトカウンタのリセット機能を追加
void AuthLockOut(char *pAddr, char  *pDir, BOOL bFlg);
#else
void AuthLockOut(char *pAddr, char  *pDir);
#endif
#endif

#ifdef BTHREAD
BOOL AuthDispatch(PCLIENT_CONTEXT lpClientContext)
#else
SMTPRSDisposition AuthDispatch(
    PSmtpContext pContext,
    PUCHAR      InputBuffer,
    DWORD       InputBufferLen,
    PHANDLE     SendHandle,
    PUCHAR *    OutputBuffer,
    PDWORD      OutputBufferLen)
#endif
{
#ifdef BTHREAD
  PSmtpContext pContext = &lpClientContext->Context;
#endif

#ifdef Y2038_BUG
	SYSTEMTIME ltime;
    FILETIME ft;
#else
    time_t ltime;
#endif
#ifdef UPDATE_20071205 //RFC2821: メールアドレス長が255Byteのときの対策
	char  mUser[512], mPass[256];
	CHAR  Bdir[512], mToken[512];
#else
	char  mUser[256], mPass[256];
	CHAR  Bdir[MAX_PATH], mToken[MAX_PATH];
#endif
#ifdef LDAP_ON         // LDAPへのログイン処理でSMTP認証の確認を行うオプション // //CRAM-MD5は複合できないので、これを利用する場合は、PLAINかLOGINのみとします。
	CHAR   *p, mLog[512];
	CHAR   mNULL[] = "";
    LDAPD  ldapd;
//	HANDLE hToken;
#endif
#ifdef UPDATE_20140704 // SMTP認証方法の指定が選択されていない指定で認証が応答してしまう不具合
    char   mMode[64];
#endif
#ifdef ADD_IDCACHE
	BOOL   bHit = FALSE;
	BOOL   bCache = FALSE;
#endif
    // Verify context, and that the context says we can receive this command
    if (!pContext ||
        (pContext->State == SmtpData) ) {
 	    pContext->mToken[0] = '\x0';
#ifdef BTHREAD
       return TRUE;
#else
       return(SMTPRS_Discard);
#endif
    }

	if (pContext->State < SmtpEGreeting) {
      sprintf(pContext->mMessages, SMTP_AUTH_NEED);
      //*OutputBuffer = pContext->mMessages;
      //*OutputBufferLen = strlen(pContext->mMessages);
      //pContext->mToken[0] = '\x0';
      //return(SMTPRS_SendBuffer);
	}
	if (pContext->bAUTHSUCCESS) { // 認証完了なら
      sprintf(pContext->mMessages, SMTP_AUTH_DUPLICATE, pContext->LocalName);
	}

	if (pContext->State == SmtpEGreeting) 
	{
#ifdef UPDATE_20140704 // SMTP認証方法の指定が選択されていない指定で認証が応答してしまう不具合
      strcpy(mMode, mSMTPAUTHMODE);
	  strlwr(mMode);
	  if (strstr(mMode, "no")) 
	  {
	    sprintf(pContext->mMessages, SMTP_AUTH_NOT_ENABLED);
	  } 
	  else 
#endif
	  {
        sprintf(pContext->mMessages, SMTP_AUTH_BAD_TYPE);
	  }
      strtok(pContext->mToken, "\r\n\x08");
	  if (strlen(pContext->mToken) > 5) {
#ifdef Y2038_BUG
        GetSystemTime(&ltime);
	    SystemTimeToFileTime(&ltime, &ft);
        sprintf(pContext->mAUTHCODE, "<%06d.%d@%s>", rand(), ft.dwLowDateTime, pContext->LocalName);
#else
        time(&ltime); 
        sprintf(pContext->mAUTHCODE, "<%06d.%d@%s>", rand(), ltime, pContext->LocalName);
#endif
	    pContext->p = &pContext->mToken[5];
#ifdef UPDATE_20140704 // SMTP認証方法の指定が選択されていない指定で認証が応答してしまう不具合
        if (strnicmp(pContext->p, "PLAIN ", 6) == 0 && strstr(mMode, "plain"))  
#else
        if (strnicmp(pContext->p, "PLAIN ", 6) == 0) 
#endif
		{
          pContext->nAUTHMODE = 0; // PLAIN mode
          sprintf(pContext->mMessages, SMTP_PASS_FAIL);
	      Base64DecodeLine((pContext->p+6), strlen((pContext->p+6)), mToken, sizeof(mToken));
#ifdef UPDATE_20061218 //AUTH PLAINの初めの認可識別子を読み飛ばすように修正
		  pContext->p = &mToken[strlen(mToken)+1];
#else
          pContext->p = mToken;
		  if (*pContext->p == '\x0')
            pContext->p = &mToken[1];
#endif
		  strcpy(mUser, pContext->p);
#ifdef UPDATE_20050916 // SMTP AUTH のユーザＩＤ情報の格納
		  strncpy(pContext->mUSER, mUser, _MAX_PATH);
		  pContext->mUSER[_MAX_PATH-1] = '\x0';
#endif
		  pContext->p += strlen(mUser);
		  pContext->p++; // PASSWORD
          GetBaseDirectory(Bdir, mMailBoxDir, mUser, pContext->MyAddr);
#ifdef LDAP_ON         // LDAPへのログイン処理でSMTP認証の確認を行うオプション // //CRAM-MD5は複合できないので、これを利用する場合は、PLAINかLOGINのみとします。
#ifdef V3
         if (bUserMan && bLDAP) {
		   if ((p = strchr(mUser, '@')))
		  	 *p = '\x0';
  sprintf(mLog, "USER=[%s] PASS=[%s] / Domain=[%s]", mUser, pContext->p, mAuthDomain);
  LEVEL_3_ACCEPTLOG(NULL, mLog);
#ifdef UPDATE_20070620 // ドメインごとにDNを設定可能にする
	       if (!p) // ドメイン区切りに%も可能に
	         p = strstr(mUser, "%");
	       if (p) {
		     *p = 0;
             ldapd.pLogonDomain = p+1;
		   } else
#endif
		   {
             ldapd.pLogonDomain = mNULL;
		   }
           ldapd.pHost = (mAuthDomain[0] ? mAuthDomain : "localhost");
		   ldapd.nPort = nLDAPPort;
		   ldapd.pLogonID = mUser;
		   ldapd.pLogonPW = pContext->p;
#ifdef ADD_IDCACHE
	       if (nIDCashLiveTime > 0)  // 0 以上
		   {
             if ((bCache = GetIDCashList(mUser, pContext->p, "pw", &bHit)))
			 {
			   if (bHit)
			   {
		         pContext->bAUTHSUCCESS = TRUE;
                 pContext->State = SmtpGreeting;
	             sprintf(pContext->mMessages, SMTP_GOOD_PASS);
			   }
#ifdef ACCEPT_LOG_LEVEL3
               sprintf(mLog, "LogonUser(%s) Cache Success.", mUser);
		       LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
			 }
		   }
		   if (!bCache)
#endif
		   {
             if (LDAP_LOGON(&ldapd)) 
			 { 
		       pContext->bAUTHSUCCESS = TRUE;
               pContext->State = SmtpGreeting;
	           sprintf(pContext->mMessages, SMTP_GOOD_PASS);
#ifdef ADD_IDCACHE
               if (nIDCashLiveTime > 0)  // 0 以上
			   {
			     SetIDCashList(mUser, pContext->p, "pw");
			   }
#endif

/*
           if (LogonUserA(mUser,   // サービスとして動作させれば正常に働く
		             (mAuthDomain[0] ? mAuthDomain : "."), //".", //"KAWAKAMI", //NULL, //
					  pContext->p, 
		              LOGON32_LOGON_BATCH, //LOGON32_LOGON_INTERACTIVE, //LOGON32_LOGON_BATCH, 
		 		      LOGON32_PROVIDER_DEFAULT,
                     &hToken) ) {
              CloseHandle(hToken);
		      pContext->bAUTHSUCCESS = TRUE;
              pContext->State = SmtpGreeting;
	          sprintf(pContext->mMessages, SMTP_GOOD_PASS);
*/
			 }
		   }
		   if (p)
			 *p = '@';
		 } else 
#endif
#endif
		 {
#ifdef UPDATE_20050927 // SMTP AUTH のユーザＩＤ情報の格納 個人別対応
           if (GetPasswordFile(Bdir, mUser, mPass, &pContext->nFROMSecLevel))
#else
           if (GetPasswordFile(Bdir, mUser, mPass))
#endif
		   {
             if (strcmp(mPass, pContext->p) == 0) { // SUCCESS
		       pContext->bAUTHSUCCESS = TRUE;
               pContext->State = SmtpGreeting;
	           sprintf(pContext->mMessages, SMTP_GOOD_PASS);
			 }
		   }
		 }
          ////// Accept Log の収集 ///////
	      if (bAcceptLog) {
   	        gettime(&pContext->ltime, pContext->mtime);
#ifdef Y2038_BUG
            _tzset();
			SystemTimeToTzSpecificLocalTime(NULL, &pContext->ltime, &pContext->lt);
	        sprintf(pContext->mdata, "%02d%02d%02d", (pContext->lt.wYear%100), pContext->lt.wMonth, pContext->lt.wDay );
#else
            pContext->lt = *localtime(&pContext->ltime);
            strftime(pContext->mdata, 128, "%y%m%d", &pContext->lt );
#endif
#ifdef REGTOFILE
      if (nClustering) {
	    sprintf(pContext->mAcptLogFn, "%sacceptlog\\%s\\%s.log", mMailSpoolDir, mComName, pContext->mdata);
	  } else 
#endif
	  {
	        sprintf(pContext->mAcptLogFn, "%sacceptlog\\%s.log", mMailSpoolDir, pContext->mdata);
	  }
#ifdef Y2038_BUG
            sprintf(pContext->mdata, "%02d/%s/%04d:%02d:%02d:%02d",pContext->lt.wDay, mMonth[pContext->lt.wMonth-1], pContext->lt.wYear, pContext->lt.wHour, pContext->lt.wMinute, pContext->lt.wSecond);
#else
            strftime( pContext->mdata, 128, "%d/%b/%Y:%H:%M:%S", &pContext->lt );
#endif
		    sprintf(pContext->mAcptLogState, "[%s], (%s), %s[%s], AUTH-PLAIN:%s [%s]", 
                                     pContext->mdata,
                                     pContext->LocalName,
									 pContext->PeerName,
                                     pContext->PeerAddr,
									 mUser,
									 pContext->bAUTHSUCCESS ? "success":"failed");
		  }
        //////////////////////////////////
		}
#ifdef UPDATE_20171229 // rainloopでSMTP認証がしい敗しないようする対策 LOGINの処理
		else if (stricmp(pContext->p, "PLAIN") == 0 && strstr(mMode, "plain"))
		{
          pContext->nAUTHMODE = 4; // PLAIN USERID:PW無し mode;
          pContext->mUSER[0] = '\x0'; // Clear USER ID 
	      sprintf(pContext->mMessages, SMTP_GOOD_AUTH, "");
	      pContext->State = SmtpAuthentication;
		}
#endif
#ifdef UPDATE_20140704 // SMTP認証方法の指定が選択されていない指定で認証が応答してしまう不具合
		else if (strnicmp(pContext->p, "LOGIN ", 6) == 0 && strstr(mMode, "login"))
#else
		else if (strnicmp(pContext->p, "LOGIN ", 6) == 0)
#endif
        {
          pContext->nAUTHMODE = 1; // LOGIN USERID有り mode;
#ifdef UPDATE_20140709 // SMTP認証時のユーザ名取得でバッファオーバフローでハングする可能性
          strncpy(pContext->mUSER, (pContext->p+6), _MAX_PATH-1); // Get USER ID 
		  pContext->mUSER[_MAX_PATH-1] = '\x0';
#else
          strcpy(pContext->mUSER, (pContext->p+6)); // Get USER ID 
#endif
	      sprintf(pContext->mMessages, SMTP_GOOD_AUTH, "LOGIN");
	      pContext->State = SmtpAuthentication;
//printf("pContext->nAUTHMODE=%u\n", pContext->nAUTHMODE);
		}
#ifdef UPDATE_20140704 // SMTP認証方法の指定が選択されていない指定で認証が応答してしまう不具合
		else if (stricmp(pContext->p, "LOGIN") == 0 && strstr(mMode, "login"))
#else
		else if (stricmp(pContext->p, "LOGIN") == 0)
#endif
		{
          pContext->nAUTHMODE = 2; // LOGIN USERID無し mode;
          pContext->mUSER[0] = '\x0'; // Clear USER ID 
	      //sprintf(pContext->mMessages, SMTP_GOOD_AUTH, "username:");
#ifdef UPDATE_20241226 // AUTH LOGINでの応答時チャレンジ文字列の設定をレジストリから変更できるようにした。
		  strcpy(pContext->mAUTHCODE, mChallengeTokenLOGINUser);
#else
		  strcpy(pContext->mAUTHCODE, "username:");
#endif
		  translateUue2Base64(pContext->mAUTHCODE, strlen(pContext->mAUTHCODE), pContext->mAUTHCODEBASE64);
	      sprintf(pContext->mMessages, SMTP_GOOD_AUTH, pContext->mAUTHCODEBASE64);
	      pContext->State = SmtpAuthentication;
		}
#ifdef UPDATE_20140704 // SMTP認証方法の指定が選択されていない指定で認証が応答してしまう不具合
		else if (stricmp(pContext->p, "CRAM-MD5") == 0 && strstr(mMode, "cram-md5"))
#else
		else if (stricmp(pContext->p, "CRAM-MD5") == 0) 
#endif
		{
          pContext->nAUTHMODE = 3; // CRAM-MD5 mode
		  translateUue2Base64(pContext->mAUTHCODE, strlen(pContext->mAUTHCODE), pContext->mAUTHCODEBASE64);
	      sprintf(pContext->mMessages, SMTP_GOOD_AUTH, pContext->mAUTHCODEBASE64);
	      pContext->State = SmtpAuthentication;
		}
#ifdef ADD_XOAUTH2 // OAUTH2での認証方式を追加
#ifdef ADD_XOAUTH2_A // OAUTH2での認証方式を１行で指定した場合の対応
		else if (strnicmp(pContext->p, "XOAUTH2 ", 8) == 0 && strstr(mMode, "xoauth2"))
		{
          pContext->nAUTHMODE = 5; // XOAUTH2 mode
          pContext->mUSER[0] = '\x0'; // Clear USER ID 
	      sprintf(pContext->mMessages, SMTP_GOOD_AUTH, "");
	      pContext->State = SmtpAuthentication;
		  memcpy(pContext->mToken, pContext->p+8, strlen(pContext->p+8)+1);
		  PassDispatch(lpClientContext);
		  goto authend;
		}
#endif
		else if (stricmp(pContext->p, "XOAUTH2") == 0 && strstr(mMode, "xoauth2"))
		{
          pContext->nAUTHMODE = 5; // XOAUTH2 mode
          pContext->mUSER[0] = '\x0'; // Clear USER ID 
	      sprintf(pContext->mMessages, SMTP_GOOD_AUTH, "");
	      pContext->State = SmtpAuthentication;
		}
#endif
	  }
	}
    //*SendHandle = NULL;
#ifdef UPDATE_20180821 // LOGIN認証時のロックアウトのカウントが行えなかった
	if (strncmp(pContext->mMessages, "334 ", 4))
#endif
	{
#ifdef UPDATE_20160113 // 認証時のロックアウト機能を追加
      if (nAuthLockOut > 0 && !strcmp(pContext->mMessages, SMTP_PASS_FAIL))
	  {
        GetBaseDirectory(Bdir, mMailBoxDir, mUser, pContext->MyAddr);
#ifdef UPDATE_20180819B // 認証成功時にロックアウトカウンタのリセット機能を追加
	    AuthLockOut(pContext->PeerAddr, Bdir, TRUE);
#else
        AuthLockOut(pContext->PeerAddr, Bdir);
#endif
#ifdef UPDATE_20180819A // 認証セッション中にロックアウト回数に達したら強制切断する
	    if (HiddenMyIP(pContext->PeerAddr, 1))
		{
          pContext->mToken[0] = '\x0';
		  return FALSE;
		}
#endif
	  }
#ifdef UPDATE_20180819B // 認証成功時にロックアウトカウンタのリセット機能を追加
  	  else if (nAuthLockOut > 0) {
        GetBaseDirectory(Bdir, mMailBoxDir, mUser, pContext->MyAddr);
	    AuthLockOut(pContext->PeerAddr, Bdir, FALSE);
	  }
#endif
#endif
	}
#ifdef ADD_XOAUTH2_A // OAUTH2での認証方式を１行で指定した場合の対応
authend:
#endif
    pContext->mToken[0] = '\x0';
#ifdef BTHREAD
    return TRUE;
#else
    *OutputBuffer = pContext->mMessages;
    *OutputBufferLen = strlen(pContext->mMessages);
    return(SMTPRS_SendBuffer);
#endif
}

#endif
