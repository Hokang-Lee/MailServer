////////////////////////////////////////////////////////////
// Helo.c Copyright K.kawakami
////////////////////////////////////////////////////////////
#include "smtp.h"
#include <time.h>

extern BOOL bDebug;
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
extern char  mAuthDomain[];
#ifdef LDAP_ON         // LDAPへのログイン処理でSMTP認証の確認を行うオプション // //CRAM-MD5は複合できないので、これを利用する場合は、PLAINかLOGINのみとします。
extern BOOL  bLDAP;
extern DWORD nLDAPPort;
#endif
#ifdef ADD_IDCACHE
extern int  nIDCashLiveTime; // ADキャッシュ利用有効時間(秒単位)
#endif
#ifdef UPDATE_20160113 // 認証時のロックアウト機能を追加
extern DWORD nAuthLockOut; // ロックアウトまでの回数 デフォルト 0:無効
#endif
#ifdef ADD_XOAUTH2 // OAUTH2での認証方式を追加
extern char	mOAuthFile[];
#endif
#ifdef UPDATE_20241226 // AUTH LOGINでの応答時チャレンジ文字列の設定をレジストリから変更できるようにした。
extern CHAR mChallengeTokenLOGINPass[];
#endif

void GetBaseDirectory(CHAR *BaseDir, char *mMBdir, char *muser,char *myaddr);
#ifdef UPDATE_20050927 // SMTP AUTH のユーザＩＤ情報の格納 個人別対応
BOOL GetPasswordFile(char  *pDir, char *pUser, char *mpass, DWORD *nFSL);
#else
BOOL GetPasswordFile(char  *pDir, char *pUser, char *mpass);
#endif
int  Base64DecodeLine(char * pszLine, int nLineLen, unsigned char * uchBuffer, int nBufferLen);
void hmac_md5(unsigned char* text, unsigned char* key, char *dest);
void MDString (char *string, char *dest);
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
#ifdef UPDATE_20241209 // 外部アカウントでのXOAUTH2での認証方式用にユーザ別認証コード保管ファイル(oauth2.dat)を各アカウントフォルダに設定可能にした
char * GetXOAUTH2Path(char *pDir, char *pUser, char *pFn);
BOOL CheckDomain(char *mID);
#endif

#ifdef BTHREAD
BOOL PassDispatch(PCLIENT_CONTEXT lpClientContext)
#else
SMTPRSDisposition PassDispatch(
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
	INT   nMode = -1;
#ifdef UPDATE_20071205A //RFC2821: メールアドレス長が255Byteのときの対策
	char  mUser[512], mPass[256];
	CHAR  Bdir[512], mToken[512];
#else
	char  mUser[256], mPass[256];
	CHAR  Bdir[MAX_PATH], mToken[MAX_PATH];
#endif
#ifdef LDAP_ON         // LDAPへのログイン処理でSMTP認証の確認を行うオプション // //CRAM-MD5は複合できないので、これを利用する場合は、PLAINかLOGINのみとします。
#ifdef ADD_XOAUTH2_D // OAUTH2の認可アプリの登録メールアドレスで一致するもののみの許可するようにした。
	CHAR   *p, mLog[4096];
#else
	CHAR   *p, mLog[512];
#endif
	CHAR   mNULL[] = "";
    LDAPD  ldapd;
	//HANDLE hToken;
#endif
#ifdef ADD_IDCACHE
	BOOL   bHit = FALSE;
	BOOL   bCache = FALSE;
#endif
#ifdef ADD_XOAUTH2 // OAUTH2での認証方式を追加
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
  FILE *fp, *pp;
  int  n;
  CHAR mRes[4096], mReq[4096], mAns[4096];
  CHAR mPAuthFile[256];
  CHAR *q, *r, *s;
#endif
#ifdef ADD_XOAUTH2_D // OAUTH2の認可アプリの登録メールアドレスで一致するもののみの許可するようにした。
  BOOL bDbl;
  BOOL bOAuth2Addr;
  CHAR mOAuth2Addr[256];
#endif
#ifdef UPDATE_20241209_PIPE // 外部アカウントがらの内部アドレスにパイプ設定
  char *t; // 内部アドレスにパイプされている場合のポインタ
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

	if (pContext->State < SmtpAuthentication) {
      sprintf(pContext->mMessages, SMTP_AUTH_NEED);
      pContext->mToken[0] = '\x0';
#ifdef BTHREAD
      return TRUE;
#else
      *OutputBuffer = pContext->mMessages;
      *OutputBufferLen = strlen(pContext->mMessages);
      return(SMTPRS_SendBuffer);
#endif
	}

    sprintf(pContext->mMessages, SMTP_PASS_FAIL);
	if (pContext->State == SmtpAuthentication) {
      strtok(pContext->mToken, "\r\n\x08");
	  if (pContext->mToken[0] == '*') {
		sprintf(pContext->mMessages, SMTP_AUTH_CANCEL);
		pContext->State--;
	  } else {
        pContext->State = SmtpEGreeting;

#ifdef UPDATE_20140709 // SMTP認証時のユーザ名取得でバッファオーバフローでハングする可能性
        mUser[0] = mPass[0] = mToken[0] ='\x0';
#endif
//	    if (strlen(pContext->mToken) > 5) {
	      pContext->State = SmtpGreeting;
#ifdef ADD_XOAUTH2 // OAUTH2での認証方式を追加
		  if (pContext->nAUTHMODE > 0 && pContext->nAUTHMODE != 5) // PLAIN OR XOAUTH2 以外なら
#else
		  if (pContext->nAUTHMODE > 0) // PLAIN 以外なら
#endif
	        Base64DecodeLine(pContext->mToken, strlen(pContext->mToken), mToken, sizeof(mToken));
	      switch(pContext->nAUTHMODE) {
		     //case 0: // PLAIN
 	        case 1: // LOGIN USERID有り
	                Base64DecodeLine(pContext->mUSER, strlen(pContext->mUSER), mUser, sizeof(mUser));
		            pContext->p = mToken;
			        break;
		    case 2: // LOGIN USERID無し
			        pContext->State = SmtpAuthentication;
	                strcpy(pContext->mUSER, pContext->mToken);
	                Base64DecodeLine(pContext->mUSER, strlen(pContext->mUSER), mUser, sizeof(mUser));
				    //sprintf(pContext->mMessages, SMTP_GOOD_AUTH, "PASSWORD"); //mUser);
#ifdef UPDATE_20241226 // AUTH LOGINでの応答時チャレンジ文字列の設定をレジストリから変更できるようにした。
		            strcpy(pContext->mAUTHCODE, mChallengeTokenLOGINPass);
#else
		            strcpy(pContext->mAUTHCODE, "password:");
#endif
		            translateUue2Base64(pContext->mAUTHCODE, strlen(pContext->mAUTHCODE), pContext->mAUTHCODEBASE64);
	                sprintf(pContext->mMessages, SMTP_GOOD_AUTH, pContext->mAUTHCODEBASE64);
				    break;
		    case 3: // CRAM-MD5
		            pContext->p = strstr(mToken, " ");
		            if (pContext->p) {
		              *pContext->p = '\x0';
#ifdef UPDATE_20140709 // SMTP認証時のユーザ名取得でバッファオーバフローでハングする可能性
                      strncpy(mUser, mToken, _MAX_PATH-1); // Get USER ID 
		              mUser[_MAX_PATH-1] = '\x0';
#else
		              strcpy(mUser, mToken);
#endif
                      pContext->p++; // PASSWORDへ移動
					}
				    break;
#ifdef UPDATE_20171229 // rainloopでSMTP認証がしい敗しないようする対策 LOGINの処理
		    case 4: // PLAIN USERID:PW無し
			        //pContext->State = SmtpAuthentication;
	                Base64DecodeLine(pContext->mToken, strlen(pContext->mToken), mToken, sizeof(mToken));
                    strcpy(mUser, &mToken[strlen(mToken)+1]);//mToken+1);
#ifdef UPDATE_20251218 // SMTP認証 PLAIN方式で 先頭文字列が'\0'が含まれたユーザアカウント＆パスワード情報のBase64エンコードデータのデコード後の位置取りがずれて認証に失敗する
                    pContext->p = mToken+strlen(mToken)+strlen(mUser)+2;
#else
                    pContext->p = mToken+strlen(mUser)+2;
#endif
	                sprintf(pContext->mMessages, SMTP_GOOD_AUTH, "");
					break;
#endif
#ifdef ADD_XOAUTH2 // OAUTH2での認証方式を追加
             case 5: // XOAUTH2
#ifdef ADD_XOAUTH2_D // OAUTH2の認可アプリの登録メールアドレスで一致するもののみの許可するようにした。
                     mOAuth2Addr[0]='\x0';
#endif
		             mRes[0] = '\x0';
					 memset(mRes, 0, sizeof(mRes));
		             n = Base64DecodeLine(pContext->mToken, strlen(pContext->mToken), mRes, sizeof(mRes)-1);
		             if (n > 0)
					 {
		               q = mRes;
			           mUser[0] = '\x0';
			           if (q = strchr(q, '\x01'))
					   { 
			             *q = '\x0';
			             q++;
			             if (r = strchr(mRes, '='))
						 {
                           strncpy(mUser, r+1, _MAX_PATH-1);
	                       mUser[_MAX_PATH] = '\x0';
						   strcpy(pContext->mUSER, mUser);
						 }
#ifdef UPDATE_20241209 // 外部アカウントでのXOAUTH2での認証方式用にユーザ別認証コード保管ファイル(oauth2.dat)を各アカウントフォルダに設定可能にした
						 if (!CheckDomain(mUser))
						   sprintf(Bdir, GetXOAUTH2Path(mMailSpoolDir, mUser, ""));
						 else
#endif
	                       GetBaseDirectory(Bdir, mMailBoxDir, mUser, pContext->MyAddr);
			             if (q = strchr(q, ' '))
						 {
				           while(*q == ' ')
				             q++;
				           strtok(q, "\x01");
						   {
				             // アカウントまたはドメインをキーにしてトークン問合せ先テーブルファイルからQueryを行う
							 sprintf(mPAuthFile, "%soauth2.dat", Bdir);
				             if (!(fp = fopen(mPAuthFile, "rt")))  // 個人別トークン問合せ先
				               fp = fopen(mOAuthFile, "rt"); //無ければ全体用トークン問合せ先
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
				  	               if (strstr(strlwr(mAns), "smtp") || mAns[0] == '*')
								   {
#ifdef ADD_XOAUTH2_D // OAUTH2の認可アプリの登録メールアドレスで一致するもののみの許可するようにした。
						             if (*s == '"') {
							           bDbl = TRUE;
						               sprintf(mReq, s+1);
									 } else {
							           bDbl = FALSE;
					                   sprintf(mReq, s);
									 }
#else
					                 sprintf(mReq, s);
#endif
						             strcpy(mAns, mReq);
#ifdef ADD_XOAUTH2_D // OAUTH2の認可アプリの登録メールアドレスで一致するもののみの許可するようにした。
									 if (bDbl)
                                       s = strchr(mAns, '"');
									 else
                                       s = strpbrk(mAns, "\t= ");
									 if (s)
#else
				                     if (s = strpbrk(mAns, "\t= "))
#endif
									 {
					                   *s = '\x0';
#ifdef UPDATE_20241209_PIPE // 外部アカウントがらの内部アドレスにパイプ設定
							           if (t = strpbrk(mAns, "|")) // 内部アドレスにパイプされている場合
									   {
							             *t = '\x0';
							             t++;
									   }
#endif
#ifdef ADD_XOAUTH2_D // OAUTH2の認可アプリの登録メールアドレスで一致するもののみの許可するようにした。
                                       sprintf(mOAuth2Addr, "*\"%s\"*", mAns);
#endif
									   s++;
						               while(*s == ' ' || *s == '\t' ||*s == '='|| *s == '"')
						                 s++;
									 }
#ifdef ADD_XOAUTH2_D // OAUTH2の認可アプリの登録メールアドレスで一致するもののみの許可するようにした。
				  	                 if (s)
#else
				  	                 if (mAns[0] && strstr(mUser, mAns))
#endif
									 {
					                   sprintf(mReq, s, q);
									   //strtok(mReq, "\"");
#ifdef UPDATE_20241209_PIPE // 外部アカウントがらの内部アドレスにパイプ設定
									   if (t) {
							             strcpy(mUser, t);// 内部アドレスにパイプされている場合
										 GetBaseDirectory(Bdir, mMailBoxDir, mUser, pContext->MyAddr);
									   }
#endif
					                   break;
									 }
					                 break;
								   }
								 }
							   }
			  	               fclose(fp);
				               //sprintf(mReq, "curl http://10.0.1.248/oauth2/resource.php -s -d \"access_token=%s\"", q);
				               mAns[0]="";
                               //printf("mReq size=%d\n", strlen(mReq));
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
                               CreateProcess(NULL, mReq, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi);
                               // Close pipes we do not need.
							   if (bOut)
                                 CloseHandle(hStdOutPipeWrite);
							   if (bIn)
                                 CloseHandle(hStdInPipeRead);

                               if (ReadFile(hStdOutPipeRead, mAns, sizeof(mAns)-1, &dwRead, NULL))
	                           {
                                 //mAns[dwRead] = '\0';
                                 n = dwRead;
					             mAns[n]='\x0';
				                 while(ReadFile(hStdOutPipeRead, &mAns[n], sizeof(mAns)-strlen(mAns)-1, &dwRead, NULL))
								 {
                                   n += dwRead;
								 }
				                 mAns[n] = '\0';
#else
				               if (pp = _popen(mReq, "r"))
							   {
				                 //fgets(mAns, sizeof(mAns)-1, pp);
                                 n = 0;
                                 mAns[n]='\x0';
				                 while(fgets(&mAns[n], sizeof(mAns)-strlen(mAns)-1, pp))
								 {
                                   n = strlen(mAns);
								 }
					             mAns[n] = '\0';
				                 _pclose(pp);
#endif
								 //LEVEL_3_ACCEPTLOG(NULL, mAns);
				                 _strlwr(mAns);
				                 //if (strstr(mAns, "success"))
#ifdef ADD_XOAUTH2_D // OAUTH2の認可アプリの登録メールアドレスで一致するもののみの許可するようにした。
								 sprintf(mLog, "mOAuth2Addr=[%s], mAns=[%s]", mOAuth2Addr, mAns);
                                 if (bDebug)
								   printf("%s\n", mLog);
#ifdef ACCEPT_LOG_LEVEL3
                                 LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
				                 bOAuth2Addr = wildcard_stricmp(mOAuth2Addr, mAns);
				                 if (mAns[0] == '{' && !strstr(mAns, "\"error\"") && bOAuth2Addr)
#else
								 if (mAns[0] == '{' && !strstr(mAns, "\"error\""))
#endif
								 {
				                   //nSts = 0;
                                   //pContext->nAUTHMODE = 3; // LOGIN USERID有り mode;
	                               mUser[_MAX_PATH-1] = '\x0';
	                               sprintf(pContext->mMessages, SMTP_GOOD_AUTH, "");
/*
#ifdef UPDATE_20241209 // 外部アカウントでのXOAUTH2での認証方式用にユーザ別認証コード保管ファイル(oauth2.dat)を各アカウントフォルダに設定可能にした
			                       if (!CheckDomain(mUser))
								   {
			                         strncpy(pContext->mPASS, mAns, _MAX_PATH-1);
									 pContext->mPASS[_MAX_PATH-1] ='\x0';
									 strcpy(pContext->p, pContext->mPASS);
								   } else
								   {
#endif
*/
#ifdef UPDATE_20050927 // SMTP AUTH のユーザＩＤ情報の格納 個人別対応
                                     if (GetPasswordFile(Bdir, mUser, pContext->mPASS, &pContext->nFROMSecLevel))
#else
                                     if (GetPasswordFile(Bdir, mUser, pContext->mPASS))
#endif
				                       strcpy(pContext->p, pContext->mPASS);
/*
#ifdef UPDATE_20241209 // 外部アカウントでのXOAUTH2での認証方式用にユーザ別認証コード保管ファイル(oauth2.dat)を各アカウントフォルダに設定可能にした
								   }
#endif
*/
								 }
							   }
#ifdef WINDOWS
							   if (bOut)
                                 CloseHandle(hStdOutPipeRead);
							   if (bIn)
                                 CloseHandle(hStdInPipeWrite);
#endif
#ifdef ADD_XOAUTH2_LOG // OAUTH2での認証方式を追加
							   {
								   char mLog[256];
LEVEL_3_ACCEPTLOG(NULL, mReq);
sprintf(mLog, "pp=%x / errno=%d", pp, errno);
LEVEL_3_ACCEPTLOG(NULL, mLog);
							   }
#endif
							 }
						   }
		                   //bResult = OAuth2Dispatch(lpClientContext);
						 }
					   }
					 }
			         break;
#endif
		  } 
		  if (pContext->nAUTHMODE == 2) { // LOGIN USERID無し
		    pContext->nAUTHMODE = 1;
		  } else {
#ifdef UPDATE_20050916 // SMTP AUTH のユーザＩＤ情報の格納
			strncpy(pContext->mUSER, mUser, _MAX_PATH);
			pContext->mUSER[_MAX_PATH-1] = '\x0';
#endif
/*
#ifdef UPDATE_20241209 // 外部アカウントでのXOAUTH2での認証方式用にユーザ別認証コード保管ファイル(oauth2.dat)を各アカウントフォルダに設定可能にした
			if (pContext->nAUTHMODE == 5 && !CheckDomain(mUser))
			  sprintf(Bdir, GetXOAUTH2Path(mMailSpoolDir, mUser, ""));
			else
#endif
*/
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
#ifdef UPDATE_20180515 // LADP利用時にCRAM-MD5を有効にする
#ifdef UPDATE_20050927 // SMTP AUTH のユーザＩＤ情報の格納 個人別対応
                if (GetPasswordFile(Bdir, mUser, pContext->mPASS, &pContext->nFROMSecLevel))
#else
                if (GetPasswordFile(Bdir, mUser, pContext->mPASS))
#endif
				{
 	              switch(pContext->nAUTHMODE) 
				  {
 	                //case 0: // PLAIN
   	                case 1: // LOGIN
 			              strcpy(mPass, pContext->mPASS); 
                          break;
		            case 3: // CRAM-MD5
#ifdef UPDATE_20231221 //SMTP認証で条件によりユーザIDがNULLで認証が成功してしまう不具合
                          mPass[0] = '\x0';
					      if (pContext->mPASS[0])
#endif
	                      hmac_md5(pContext->mAUTHCODE,pContext->mPASS,mPass);
                          break;
#ifdef UPDATE_20171229 // rainloopでSMTP認証がしい敗しないようする対策 LOGINの処理
 	                case 4: // PLAIN
 			              strcpy(mPass, pContext->mPASS); 
                          break;
#endif
#ifdef ADD_XOAUTH2 // OAUTH2での認証方式を追加
 	                case 5: // XOAUTH2
						  strcpy(mPass, pContext->mPASS); 
					      break;
#endif
				  }
		          if (strcmp(mPass, pContext->p) == 0 ) { // SUCCESS
#ifdef ADD_XOAUTH2 // OAUTH2での認証方式を追加
				    if (pContext->nAUTHMODE == 3 || pContext->nAUTHMODE == 5) // CRAM-MD5 OR XAOUTH2
#else
				    if (pContext->nAUTHMODE == 3) // CRAM-MD5
#endif
					{
	                  strcpy(pContext->p, pContext->mPASS); 
					};
#endif
                    if (LDAP_LOGON(&ldapd)) 
					{ 
				      pContext->bAUTHSUCCESS = TRUE;
				      //nMode = pContext->nAUTHMODE;
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
				//nMode = pContext->nAUTHMODE;
                sprintf(pContext->mMessages, SMTP_GOOD_PASS);
			  */
					}
#ifdef UPDATE_20180515 // LADP利用時にCRAM-MD5を有効にする
				  }
				} 
#endif
			  }
			  if (p)
				*p = '@';
			} else 
#endif
#endif
			{
/*
#ifdef UPDATE_20241209 // 外部アカウントでのXOAUTH2での認証方式用にユーザ別認証コード保管ファイル(oauth2.dat)を各アカウントフォルダに設定可能にした
			  if (pContext->nAUTHMODE == 5 && !CheckDomain(mUser) ||
                  GetPasswordFile(Bdir, mUser, pContext->mPASS, &pContext->nFROMSecLevel))
#else
*/
#ifdef UPDATE_20050927 // SMTP AUTH のユーザＩＤ情報の格納 個人別対応
              if (GetPasswordFile(Bdir, mUser, pContext->mPASS, &pContext->nFROMSecLevel))
#else
              if (GetPasswordFile(Bdir, mUser, pContext->mPASS))
#endif
//#endif
			  {
 	            switch(pContext->nAUTHMODE) {
 	              //case 0: // PLAIN
   	              case 1: // LOGIN
 			              strcpy(mPass, pContext->mPASS); 
                          break;
		          case 3: // CRAM-MD5
#ifdef UPDATE_20231221 //SMTP認証で条件によりユーザIDがNULLで認証が成功してしまう不具合
                          mPass[0] = '\x0';
					      if (pContext->mPASS[0])
#endif
	                        hmac_md5(pContext->mAUTHCODE,pContext->mPASS,mPass);
                          break;
#ifdef UPDATE_20171229 // rainloopでSMTP認証がしい敗しないようする対策 LOGINの処理
 	              case 4: // PLAIN
 			              strcpy(mPass, pContext->mPASS); 
#ifdef UPDATE_20180821 // LOGIN認証時のロックアウトのカウントが行えなかった
                          sprintf(pContext->mMessages, SMTP_PASS_FAIL);
#endif
                          break;
#endif
#ifdef ADD_XOAUTH2 // OAUTH2での認証方式を追加
 	              case 5: // XOAUTH2
						  strcpy(mPass, pContext->mPASS); 
					      break;
#endif
				}
#ifdef UPDATE_20231221 //SMTP認証で条件によりユーザIDがNULLで認証が成功してしまう不具合
		        if (mPass[0] && *pContext->p && strcmp(mPass, pContext->p) == 0 ) 
#else
		        if (strcmp(mPass, pContext->p) == 0 ) 
#endif
				{ // SUCCESS
				  pContext->bAUTHSUCCESS = TRUE;
				  //nMode = pContext->nAUTHMODE;
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
		  sprintf(pContext->mAcptLogState, "[%s], (%s), %s[%s], AUTH-%s:%s [%s]", 
                                     pContext->mdata,
                                     pContext->LocalName,
									 pContext->PeerName,
                                     pContext->PeerAddr,
#ifdef UPDATE_20241210 // acceptlogのSMTP認証の方式の記録が"LOGIN"か"CRAM-MD5"しか記録されなかった不具合
									 (pContext->nAUTHMODE == 0 || pContext->nAUTHMODE == 4 ? "PLAIN" : 
									 (pContext->nAUTHMODE == 1 || pContext->nAUTHMODE == 2 ? "LOGIN" : 
		                             (pContext->nAUTHMODE == 3 ? "CRAM-MD5" : 
									 (pContext->nAUTHMODE == 5 ? "XAOUTH2" : "unknown")))), 
#else
									 pContext->nAUTHMODE == 1 ? "LOGIN" : "CRAM-MD5", 
#endif
									 mUser,
									 pContext->bAUTHSUCCESS ? "success":"failed");
		}
        //////////////////////////////////
		  }
		}
	  //} 
	}
    //*SendHandle = NULL;
	//pContext->nAUTHMODE = nMode; // 認証結果
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
