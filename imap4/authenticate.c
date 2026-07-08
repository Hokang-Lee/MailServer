////////////////////////////////////////////////////////////
// AUTHENTICATE.C Copyright K.kawakami
////////////////////////////////////////////////////////////
#include "imap4.h"
#include <process.h>

extern BOOL  bServiceTerminating;
extern BOOL  bDebug;
extern DWORD nLogInID;
extern CHAR mMailBoxDir[];
extern char mMailExtension[];
#ifdef UPDATE_20091116 // CAPABILITY 命令で応答する認証方法を設定変更可能にした。
extern int nAuthType; // 3:CRAM-MD5&LOGIN,2:CRAM-MD5,1:LOGIN
#endif
#ifdef UPDATE_20161013 // ログイン成功時にルートフォルダが存在しない場合、自動作成を試みるようにした。
extern BOOL   bAutoMakeDir;
#endif
#ifdef UPDATE_20210224 // ADアカウントまたは、WINDOWSアカウント運用時のAUTHENTICATE LOGIN/PLAINでのパスワードファイルの参照を無視する
extern BOOL  bUserMan;
extern BOOL  bSimpleAuth;
#endif
#ifdef ADD_XOAUTH2 // OAUTH2での認証方式を追加
extern char	mOAuthFile[];
#endif
#ifdef UPDATE_20241209 // 外部アカウントでのXOAUTH2での認証方式用にユーザ別認証コード保管ファイル(oauth2.dat)を各アカウントフォルダに設定可能にした
extern char mMailSpoolDir[];
#endif

void imap4log(PCLIENT_CONTEXT lpClientContext, char *bsts);
void GetBaseDirectory(CHAR *BaseDir, char *mMBdir, char *muser, char *myaddr);
BOOL GetPasswordFile(char  *pDir, char *pUser, char *mpass);
int  Base64DecodeLine(char * pszLine, int nLineLen, unsigned char * uchBuffer, int nBufferLen);
void translateUue2Base64(char *line, int inlen, char *newline);
void hmac_md5(unsigned char* text, unsigned char* key, char *dest);
void MDString (char *string, char *dest);

BOOL LOGINDispatch(PCLIENT_CONTEXT lpClientContext);
void SetSharedUserFolder(PCLIENT_CONTEXT lpClientContext);
BOOL IMAP4OffUser(PCLIENT_CONTEXT lpClientContext);
//void MoveMSGFile(PCLIENT_CONTEXT lpClientContext);
void CmdRestor(PImap4Context pContext, char *pSquence, char *pCmd, char *pToken);
DWORD GetFolderUID(char *pFolder, DWORD nUID);
#ifdef UPDATE_20160823 // APPEND命令で存在しない追加フォルダを指定すると処理が終了できなくなる不具合
BOOL PutFolderUID(char *pFolder, DWORD nUID);
#else
void PutFolderUID(char *pFolder, DWORD nUID);
#endif
#ifdef ADD_XOAUTH2_D // OAUTH2の認可アプリの登録メールアドレスで一致するもののみの許可するようにした。
BOOL  wildcard_stricmp(const char *string1, const char *string2);
#endif
#ifdef UPDATE_20241209 // 外部アカウントでのXOAUTH2での認証方式用にユーザ別認証コード保管ファイル(oauth2.dat)を各アカウントフォルダに設定可能にした
char * GetXOAUTH2Path(char *pDir, char *pUser, char *pFn);
BOOL CheckDomain(char *mID);
#endif

BOOL AUTHENTICATEDispatch(PCLIENT_CONTEXT lpClientContext) {
  SYSTEMTIME ltime;
  FILETIME ft;
  BOOL    bsts = FALSE;
  char    *p, mSquence[32], mCmd[32], mToken[32], mPass[256];
  PImap4Context pContext = &lpClientContext->Context;
#ifdef UPDATE_20110208 // フォルダ作成が完了するまでウエイトする。
  int              i;
  BOOL             bSuccess = FALSE;
  HANDLE           hF;
  WIN32_FIND_DATA  FD;
#endif
#ifdef UPDATE_20180820 // 暗号認証でロックアウトの処理が出来ない
  DWORD nSts = 0;
#endif
#ifdef UPDATE_20180820A // SASLにPLAIN方式を追加
  int  n;
#ifdef ADD_XOAUTH2_B // AUTHENTICATE XOAUTH2で改行した場合のトークン入力町を可能に
  char *q, mRes[4096];
#else
  char *q, mRes[1024];
#endif
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
  CHAR *r, *s;
  CHAR mReq[4096], mAns[4096];
  CHAR mPAuthFile[256];
#endif
#ifdef ADD_XOAUTH2_D // OAUTH2の認可アプリの登録メールアドレスで一致するもののみの許可するようにした。
  BOOL bDbl;
  BOOL bOAuth2Addr;
  CHAR mOAuth2Addr[256];
  CHAR str[4096];
#endif
#ifdef UPDATE_20241209_PIPE // 外部アカウントがらの内部アドレスにパイプ設定
  char *t; // 内部アドレスにパイプされている場合のポインタ
#endif

  if (pContext->State == Imap4Negotiate) { // 非認証状態
    strncpy(mSquence, pContext->pSquence, sizeof(mSquence));
    strncpy(mCmd, pContext->pCmd, sizeof(mCmd));
	if (!pContext->pToken) { // 認証方式がない
      strtok(pContext->pToken, "\r\n");
      sprintf(pContext->mMessages, "%s %s The method is an undefined in AUTHENTICATE.\r\n", mSquence, IMAP4_RESULT_RESPONSE, pContext->pToken);
	  CmdRestor(pContext, mSquence, mCmd, "");
      imap4log(lpClientContext, "failed");
	  return TRUE;
	}
	strncpy(mToken, pContext->pToken, sizeof(mToken)-1);
    mToken[sizeof(mToken)-1] = '\x0';
    mSquence[sizeof(mSquence)-1] = '\x0';
#ifdef UPDATE_20180820A // SASLにPLAIN方式を追加
	if (!strnicmp(pContext->pToken, "PLAIN", 5) && (nAuthType & 1)) // 3:CRAM-MD5&LOGIN,2:CRAM-MD5,1:LOGIN
	{
	  n = 0;
	  q = strchr(pContext->pToken, ' ');
	  if (q)
	  {
		*q = '\x0';
		q++;
		mRes[0] = '\x0';
		n = Base64DecodeLine(q, strlen(q), mRes, sizeof(mRes)-1);
	  } else {
        strcpy(pContext->mAUTHCODE, pContext->pToken);
        if (bDebug) printf("AUTHCODE: SEND %s\n", pContext->mAUTHCODE);
        translateUue2Base64(pContext->mAUTHCODE, strlen(pContext->mAUTHCODE), pContext->mAUTHCODEBASE64);
        sprintf(pContext->mMessages, "+ %s\r\n", pContext->mAUTHCODEBASE64);
	    put_reply(lpClientContext, TRUE, TRUE);
        if ((bsts = get_reply(lpClientContext, TRUE, "AUTHENTICATE"))) 
		{
	      strtok(lpClientContext->Buffer, "\r\n");
		  mRes[0] = '\x0';
          n = Base64DecodeLine(lpClientContext->Buffer, strlen(lpClientContext->Buffer), mRes, sizeof(mRes)-1);
		}
	  }
	  if (n > 0)
	  {
	    q = mRes;
	    q += strlen(q)+1;
		strncpy(pContext->mUSER, q, _MAX_PATH-1);
        pContext->mUSER[_MAX_PATH] = '\x0';
		q += strlen(q)+1;
		strcpy(pContext->mToken, q);
	  }
      GetBaseDirectory(pContext->mRootDir, mMailBoxDir, pContext->mUSER, pContext->MyAddr);
#ifdef UPDATE_20230722 // AUTHENTICATE命令でユーザ名をNULLで返答すると、ログイン成功の応答をしてしまう可能性
      if (pContext->mUSER[0] && GetPasswordFile(pContext->mRootDir, pContext->mUSER, pContext->mPASS)) 
#else
      if (GetPasswordFile(pContext->mRootDir, pContext->mUSER, pContext->mPASS)) 
#endif
	  {
	    SetSharedUserFolder(lpClientContext); // 個別の共有フォルダの設定
        if (bDebug) printf("[C:Code] %s\n[S:Code] %s\n", pContext->mToken, pContext->mPASS);
#ifdef UPDATE_20180820 // 暗号認証でロックアウトの処理が出来ない
        nSts = CheckUser(pContext->mUSER, pContext->mToken, pContext->MyAddr,  pContext->PeerAddr);
#endif
        if (!IMAP4OffUser(lpClientContext) &&
#ifdef UPDATE_20210224 // ADアカウントまたは、WINDOWSアカウント運用時のAUTHENTICATE LOGIN/PLAINでのパスワードファイルの参照を無視する
            (bSimpleAuth && nSts == 0 || !bSimpleAuth && strcmp(pContext->mToken, pContext->mPASS) == 0)
#else
		    strcmp(pContext->mToken, pContext->mPASS) == 0 
#endif
			) { // パスワード確認 SUCCESS
          pContext->State = Imap4Authorization; // 認証済状態
 	      if (bDebug) printf("[%08x]  pContext[%08x]->State = Imap4Authorization;\n", lpClientContext, pContext);
		    ////////////////////////////
#ifdef UPDATE_20161013 // ログイン時にルートフォルダが存在しない場合、自動作成を試みるようにした。
          if (bAutoMakeDir) // ログイン成功時にルートフォルダが存在しない場合、自動作成を試みるようにした。
		    _mkdir(pContext->mRootDir);
#endif
          sprintf(pContext->mSelectDir,"%sINBOX", pContext->mRootDir);
#ifdef UPDATE_20110208 // フォルダ作成が完了するまでウエイトする。
		  bSuccess = FALSE;
		  for (i = 0; i < 5; i++)
		  {
		    if (_mkdir(pContext->mSelectDir) == 0)
			{
			  bSuccess = TRUE;
			  break;
#ifdef UPDATE_20130528 // ログイン時に作成済みのINBOXフォルダがあると、無駄に作成リトライ待ちが発生する不具合
			} else if (errno == EEXIST) 
			{
			  bSuccess = TRUE;
			  break;
#endif
			}
	        _sleep(300*i);
		  }
		  if (bSuccess)
		  {
		    while ((hF = FindFirstFile(pContext->mSelectDir, &FD)) == INVALID_HANDLE_VALUE) 
			{
              if (bServiceTerminating)
  	            break;
			  _mkdir(pContext->mSelectDir);
	          _sleep(WAIT_TIME);
			} 
		    if (hF) {
              FindClose( hF ); 
			}
		  }
#else
		  _mkdir(pContext->mSelectDir); // エラーチェックしない
#endif
#ifndef UPDATE_20110405 // LOGIN,AUTHENTICATED命令でUID値取得をしないようにした。（排他処理対策）
		  pContext->nUid = GetFolderUID(pContext->mSelectDir, pContext->nUid);
#endif
		  //MoveMSGFile(lpClientContext); // SELECT INBOX実施後に移動させるように // INBOXフォルダへMSGファイル移動;
          strcpy(pContext->mSelectDir, pContext->mRootDir); // ルートに初期化
          sprintf(pContext->mMessages, "%s %s AUTHENTICATE completed\r\n", mSquence, IMAP4_GOOD_RESPONSE);
		  CmdRestor(pContext, mSquence, mCmd, mToken);
		  strtok(pContext->pToken, " ");
          imap4log(lpClientContext, "Success");
		  pContext->nLogInID = ++nLogInID; // ID設定
		    ////////////////////////////////
		} else { // 失敗
          sprintf(pContext->mMessages, "%s %s AUTHENTICATE fails\r\n", mSquence, IMAP4_RESULT_RESPONSE);
		  CmdRestor(pContext, mSquence, mCmd, mToken);
          imap4log(lpClientContext, "failed");
		}
	  } else { // 失敗
        sprintf(pContext->mMessages, "%s %s AUTHENTICATE fails\r\n", mSquence, IMAP4_RESULT_RESPONSE);
        sprintf(pContext->mMessages, "%s %s AUTHENTICATE illegal argument\r\n", mSquence, IMAP4_BADTOKEN_RESPONSE);
	    CmdRestor(pContext, mSquence, mCmd, mToken);
        imap4log(lpClientContext, "failed");
	  }
	} else
#endif
#ifdef UPDATE_20180820A // SASLにPLAIN方式を追加
	if (!strnicmp(pContext->pToken, "LOGIN", 5) && (nAuthType & 2)) // 3:CRAM-MD5&LOGIN,2:CRAM-MD5,1:LOGIN
#else
#ifdef UPDATE_20091116 // CAPABILITY 命令で応答する認証方法を設定変更可能にした。
	if (!strnicmp(pContext->pToken, "LOGIN", 5) && nAuthType != 2) // 3:CRAM-MD5&LOGIN,2:CRAM-MD5,1:LOGIN
#else
    if (!strnicmp(pContext->pToken, "LOGIN", 5)) 
#endif
#endif
	{
#ifdef UPDATE_20230722A // "AUTHENTICATE LOGIN"命令に引数を続けると認証に失敗する不具合
	  if (q = strchr(pContext->pToken, ' '))
	  {
		q++;
		while(*q == ' ')
		  q++;
		if (*q != '\x0' && *q != '\r' && *q != '\n')
		{
          Base64DecodeLine(q, strlen(q), pContext->mToken, sizeof(pContext->mToken));
          if (bDebug) printf("AUTHCODE RECV: %s\n", pContext->mToken);
		  goto passwordfage;
		}
	  }
#endif
      strcpy(pContext->mAUTHCODE, pContext->pToken);
      if (bDebug) printf("AUTHCODE: SEND %s\n", pContext->mAUTHCODE);
      translateUue2Base64(pContext->mAUTHCODE, strlen(pContext->mAUTHCODE), pContext->mAUTHCODEBASE64);
      sprintf(pContext->mMessages, "+ %s\r\n", pContext->mAUTHCODEBASE64);
	  put_reply(lpClientContext, TRUE, TRUE);
      if ((bsts = get_reply(lpClientContext, TRUE, "AUTHENTICATE"))) {
	    strtok(lpClientContext->Buffer, "\r\n");
#ifndef UPDATE_20180820 // 暗号認証でロックアウトの処理が出来ない
		if (strstr(lpClientContext->Buffer, " ") ){ // 区切りがあるなら
          pContext->pCmd = pContext->pToken = NULL;
          pContext->pSquence = lpClientContext->Buffer;
          //p = strstr(pContext->pSquence, " "); // シーケンス番号
          if ((p = strstr(pContext->pSquence, " "))) { // 命令だけ切りだす
	        *p = '\x0';
	        pContext->pCmd = ++p;
	        //p = strstr(pContext->pCmd, " "); // 命令
	        if ((p = strstr(pContext->pCmd, " "))) {
	          *p = '\x0';
	          pContext->pToken = ++p;
	          p = strstr(pContext->pToken, "\r\n"); // // 命令内容
	          if ((p = strstr(pContext->pToken, "\r\n"))) {
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
          LOGINDispatch(lpClientContext);
		} else 
#endif
		{
          Base64DecodeLine(lpClientContext->Buffer, strlen(lpClientContext->Buffer), pContext->mToken, sizeof(pContext->mToken));
          if (bDebug) printf("AUTHCODE RECV: %s\n", pContext->mToken);
#ifdef UPDATE_20230722A // "AUTHENTICATE LOGIN"命令に引数を続けると認証に失敗する不具合
passwordfage:
#endif
          strcpy(pContext->mUSER, pContext->mToken);
		  strcpy(pContext->mAUTHCODE, "PASSWORD");
          translateUue2Base64(pContext->mAUTHCODE, strlen(pContext->mAUTHCODE), pContext->mAUTHCODEBASE64);
          sprintf(pContext->mMessages, "+ %s\r\n", pContext->mAUTHCODEBASE64);
	      put_reply(lpClientContext, TRUE, TRUE);
          if ((bsts = get_reply(lpClientContext, TRUE, "AUTHENTICATE"))) {
	        strtok(lpClientContext->Buffer, "\r\n");
            Base64DecodeLine(lpClientContext->Buffer, strlen(lpClientContext->Buffer), pContext->mToken, sizeof(pContext->mToken));
            if (bDebug) printf("AUTHCODE RECV: %s\n", pContext->mToken);
            GetBaseDirectory(pContext->mRootDir, mMailBoxDir, pContext->mUSER, pContext->MyAddr);
#ifdef UPDATE_20230722 // AUTHENTICATE命令でユーザ名をNULLで返答すると、ログイン成功の応答をしてしまう可能性
            if (pContext->mUSER[0] && GetPasswordFile(pContext->mRootDir, pContext->mUSER, pContext->mPASS)) 
#else
            if (GetPasswordFile(pContext->mRootDir, pContext->mUSER, pContext->mPASS)) 
#endif
			{
			  SetSharedUserFolder(lpClientContext); // 個別の共有フォルダの設定
              if (bDebug) printf("[C:Code] %s\n[S:Code] %s\n", pContext->mToken, pContext->mPASS);
#ifdef UPDATE_20180820 // 暗号認証でロックアウトの処理が出来ない
              nSts = CheckUser(pContext->mUSER, pContext->mToken, pContext->MyAddr,  pContext->PeerAddr);
#endif
              if (!IMAP4OffUser(lpClientContext) &&
#ifdef UPDATE_20210224 // ADアカウントまたは、WINDOWSアカウント運用時のAUTHENTICATE LOGIN/PLAINでのパスワードファイルの参照を無視する
                 (bSimpleAuth && nSts == 0 || !bSimpleAuth && strcmp(pContext->mToken, pContext->mPASS) == 0)
#else
				  strcmp(pContext->mToken, pContext->mPASS) == 0
#endif
			  ) { // パスワード確認 SUCCESS

                pContext->State = Imap4Authorization; // 認証済状態
 	            if (bDebug) printf("[%08x]  pContext[%08x]->State = Imap4Authorization;\n", lpClientContext, pContext);
			    ////////////////////////////
#ifdef UPDATE_20161013 // ログイン時にルートフォルダが存在しない場合、自動作成を試みるようにした。
                if (bAutoMakeDir) // ログイン成功時にルートフォルダが存在しない場合、自動作成を試みるようにした。
				  _mkdir(pContext->mRootDir);
#endif
                sprintf(pContext->mSelectDir,"%sINBOX", pContext->mRootDir);
#ifdef UPDATE_20110208 // フォルダ作成が完了するまでウエイトする。
		         bSuccess = FALSE;
		         for (i = 0; i < 5; i++)
				 {
			       if (_mkdir(pContext->mSelectDir) == 0)
				   {
			         bSuccess = TRUE;
			         break;
#ifdef UPDATE_20130528 // ログイン時に作成済みのINBOXフォルダがあると、無駄に作成リトライ待ちが発生する不具合
				   }
			       else if (errno == EEXIST) 
				   {
			         bSuccess = TRUE;
			         break;
#endif
				   }
	               _sleep(300*i);
				 }
		         if (bSuccess)
				 {
				   while ((hF = FindFirstFile(pContext->mSelectDir, &FD)) == INVALID_HANDLE_VALUE) 
				   {
                     if (bServiceTerminating)
  	                   break;
			          _mkdir(pContext->mSelectDir);
	                  _sleep(WAIT_TIME);
				   } 
			       if (hF) {
                     FindClose( hF ); 
				   }
				 }
#else
		        _mkdir(pContext->mSelectDir); // エラーチェックしない
#endif
#ifndef UPDATE_20110405 // LOGIN,AUTHENTICATED命令でUID値取得をしないようにした。（排他処理対策）
		        pContext->nUid = GetFolderUID(pContext->mSelectDir, pContext->nUid);
#endif
		        //MoveMSGFile(lpClientContext); // SELECT INBOX実施後に移動させるように // INBOXフォルダへMSGファイル移動;
                strcpy(pContext->mSelectDir, pContext->mRootDir); // ルートに初期化
                sprintf(pContext->mMessages, "%s %s AUTHENTICATE completed\r\n", mSquence, IMAP4_GOOD_RESPONSE);
			    CmdRestor(pContext, mSquence, mCmd, mToken);
                imap4log(lpClientContext, "Success");
			    pContext->nLogInID = ++nLogInID; // ID設定
		        ////////////////////////////////
			  } else { // 失敗
                sprintf(pContext->mMessages, "%s %s AUTHENTICATE fails\r\n", mSquence, IMAP4_RESULT_RESPONSE);
			    CmdRestor(pContext, mSquence, mCmd, mToken);
                imap4log(lpClientContext, "failed");
			  }
			} else { // 失敗
              sprintf(pContext->mMessages, "%s %s AUTHENTICATE fails\r\n", mSquence, IMAP4_RESULT_RESPONSE);
			  CmdRestor(pContext, mSquence, mCmd, mToken);
              imap4log(lpClientContext, "failed");
			} 
		  } else { // 失敗
           sprintf(pContext->mMessages, "%s %s AUTHENTICATE illegal argument\r\n", mSquence, IMAP4_BADTOKEN_RESPONSE);
		   CmdRestor(pContext, mSquence, mCmd, mToken);
           imap4log(lpClientContext, "failed");
		  }
		}
	  } else { // 失敗
        sprintf(pContext->mMessages, "%s %s AUTHENTICATE illegal argument\r\n", pContext->pSquence, IMAP4_BADTOKEN_RESPONSE);
	    CmdRestor(pContext, mSquence, mCmd, mToken);
        imap4log(lpClientContext, "failed");
	  }
	}
#ifdef UPDATE_20180820A // SASLにPLAIN方式を追加
	else if (!strnicmp(pContext->pToken, "CRAM-MD5", 8) && (nAuthType & 4)) // 3:CRAM-MD5&LOGIN,2:CRAM-MD5,1:LOGIN
#else
#ifdef UPDATE_20091116 // CAPABILITY 命令で応答する認証方法を設定変更可能にした。
	else if (!strnicmp(pContext->pToken, "CRAM-MD5", 8) && nAuthType >= 2) // 3:CRAM-MD5&LOGIN,2:CRAM-MD5,1:LOGIN
#else
	else if (!strnicmp(pContext->pToken, "CRAM-MD5", 8))
#endif
#endif
	{
      GetSystemTime(&ltime);
      SystemTimeToFileTime(&ltime, &ft);
      sprintf(pContext->mAUTHCODE, "<%lu.%lu@%s>", _getpid(), ft.dwLowDateTime, pContext->LocalName);
      if (bDebug) printf("AUTHCODE: SEND %s\n", pContext->mAUTHCODE);
      translateUue2Base64(pContext->mAUTHCODE, strlen(pContext->mAUTHCODE), pContext->mAUTHCODEBASE64);
      sprintf(pContext->mMessages, "+ %s\r\n", pContext->mAUTHCODEBASE64);
      put_reply(lpClientContext, TRUE, TRUE);
      if ((bsts = get_reply(lpClientContext, TRUE, "AUTHENTICATE"))) {
	    strtok(lpClientContext->Buffer, "\r\n");
        Base64DecodeLine(lpClientContext->Buffer, strlen(lpClientContext->Buffer), pContext->mToken, sizeof(pContext->mToken));
        if (bDebug) printf("AUTHCODE RECV: %s\n", pContext->mToken);
        pContext->p = strrchr(pContext->mToken, ' ');
        if (pContext->p) {
          *pContext->p++ = '\x0'; // IDとPASSWORDに分解
          strcpy(pContext->mUSER, pContext->mToken);
          GetBaseDirectory(pContext->mRootDir, mMailBoxDir, pContext->mUSER, pContext->MyAddr);
#ifdef UPDATE_20230722 // AUTHENTICATE命令でユーザ名をNULLで返答すると、ログイン成功の応答をしてしまう可能性
          if (pContext->mUSER[0] && GetPasswordFile(pContext->mRootDir, pContext->mUSER, pContext->mPASS))
#else
          if (GetPasswordFile(pContext->mRootDir, pContext->mUSER, pContext->mPASS))
#endif
		  {
		    SetSharedUserFolder(lpClientContext); // 個別の共有フォルダの設定
            hmac_md5(pContext->mAUTHCODE,pContext->mPASS,mPass); // CRAM-MD5
            if (bDebug) printf("[C:Code] %s\n[S:Code] %s\n", pContext->p, mPass);
#ifdef UPDATE_20180820 // 暗号認証でロックアウトの処理が出来ない
			if (strcmp(mPass, pContext->p))
			{
			  pContext->mPASS[0] = '\x0';
			}
            nSts = CheckUser(pContext->mUSER, pContext->mPASS, pContext->MyAddr,  pContext->PeerAddr);
#endif
            if (!IMAP4OffUser(lpClientContext) &&
				strcmp(mPass, pContext->p) == 0 ) { // パスワード確認  SUCCESS
              pContext->State = Imap4Authorization; // 認証済状態
 	          if (bDebug) printf("[%08x]  pContext[%08x]->State = Imap4Authorization;\n", lpClientContext, pContext);
			  ////////////////////////////
#ifdef UPDATE_20161013 // ログイン時にルートフォルダが存在しない場合、自動作成を試みるようにした。
              if (bAutoMakeDir) // ログイン成功時にルートフォルダが存在しない場合、自動作成を試みるようにした。
			    _mkdir(pContext->mRootDir);
#endif
              sprintf(pContext->mSelectDir,"%sINBOX", pContext->mRootDir);
#ifdef UPDATE_20110208 // フォルダ作成が完了するまでウエイトする。
		      bSuccess = FALSE;
		      for (i = 0; i < 5; i++)
			  {
			    if (_mkdir(pContext->mSelectDir) == 0)
				{
			       bSuccess = TRUE;
			       break;
#ifdef UPDATE_20130528 // ログイン時に作成済みのINBOXフォルダがあると、無駄に作成リトライ待ちが発生する不具合
				}
			    else if (errno == EEXIST) 
				{
			      bSuccess = TRUE;
			      break;
#endif
				}
	             _sleep(300*i);
			  }
		      if (bSuccess)
			  {
			    while ((hF = FindFirstFile(pContext->mSelectDir, &FD)) == INVALID_HANDLE_VALUE) 
				{
                  if (bServiceTerminating)
  	                break;
			       _mkdir(pContext->mSelectDir);
	               _sleep(WAIT_TIME);
				} 
			    if (hF) {
                  FindClose( hF ); 
				}
			  }
#else
		      _mkdir(pContext->mSelectDir); // エラーチェックしない
#endif
#ifndef UPDATE_20110405 // LOGIN,AUTHENTICATED命令でUID値取得をしないようにした。（排他処理対策）
		      pContext->nUid = GetFolderUID(pContext->mSelectDir, pContext->nUid);
#endif
		      //MoveMSGFile(lpClientContext); // SELECT INBOX実施後に移動させるように // INBOXフォルダへMSGファイル移動;
              strcpy(pContext->mSelectDir, pContext->mRootDir); // ルートに初期化
              sprintf(pContext->mMessages, "%s %s AUTHENTICATE completed\r\n", mSquence, IMAP4_GOOD_RESPONSE);
			  CmdRestor(pContext, mSquence, mCmd, mToken);
              imap4log(lpClientContext, "Success");
			  pContext->nLogInID = ++nLogInID; // ID設定
		      ////////////////////////////////
			} else { // 失敗
              sprintf(pContext->mMessages, "%s %s AUTHENTICATE fails\r\n", mSquence, IMAP4_RESULT_RESPONSE);
			  CmdRestor(pContext, mSquence, mCmd, mToken);
              imap4log(lpClientContext, "failed");
			}
		  } else { // 失敗
            sprintf(pContext->mMessages, "%s %s AUTHENTICATE fails\r\n", mSquence, IMAP4_RESULT_RESPONSE);
		    CmdRestor(pContext, mSquence, mCmd, mToken);
            imap4log(lpClientContext, "failed");
		  } 
		} else { // 失敗
          sprintf(pContext->mMessages, "%s %s AUTHENTICATE challenge/responce err\r\n", mSquence, IMAP4_BADTOKEN_RESPONSE);
		  CmdRestor(pContext, mSquence, mCmd, mToken);
          imap4log(lpClientContext, "failed");
		}
	  } else { // 失敗
        sprintf(pContext->mMessages, "%s %s AUTHENTICATE challenge/responce err\r\n", pContext->pSquence, IMAP4_BADTOKEN_RESPONSE);
	    CmdRestor(pContext, mSquence, mCmd, mToken);
        imap4log(lpClientContext, "failed");
	  }
#ifdef ADD_XOAUTH2 // OAUTH2での認証方式を追加
	}
	else if (!strnicmp(pContext->pToken, "XOAUTH2", 7) && (nAuthType & 8)) // 8:3:CRAM-MD5&LOGIN,2:CRAM-MD5,1:LOGIN
	{
#ifdef ADD_XOAUTH2_B // AUTHENTICATE XOAUTH2で改行した場合のトークン入力町を可能に
	  if (!(q = strchr(pContext->pToken, ' ')))
	  {
        //strcpy(pContext->mAUTHCODE, pContext->pToken);
        //if (bDebug) printf("AUTHCODE: SEND %s\n", pContext->mAUTHCODE);
        //translateUue2Base64(pContext->mAUTHCODE, strlen(pContext->mAUTHCODE), pContext->mAUTHCODEBASE64);
        sprintf(pContext->mMessages, "+\r\n");
	    put_reply(lpClientContext, TRUE, TRUE);
        if ((bsts = get_reply(lpClientContext, TRUE, "AUTHENTICATE"))) 
		{
		  q = lpClientContext->Buffer;
		  //q--;
		}
	  }
	  if (q)
#else
	  if (q = strchr(pContext->pToken, ' '))
#endif
	  {
#ifdef ADD_XOAUTH2_B // AUTHENTICATE XOAUTH2で改行した場合のトークン入力町を可能に
		if (*q == ' ')
		{
#endif
		  *q = '\x0';
		  q++;
#ifdef ADD_XOAUTH2_B // AUTHENTICATE XOAUTH2で改行した場合のトークン入力町を可能に
		}
#endif
        strtok(lpClientContext->Buffer, "\r\n");
#ifdef ADD_XOAUTH2_D // OAUTH2の認可アプリの登録メールアドレスで一致するもののみの許可するようにした。
        mOAuth2Addr[0]='\x0';
#endif
		mRes[0] = '\x0';
	    nSts = -1;
		memset(mRes, 0, sizeof(mRes));
        n = Base64DecodeLine(q, strlen(q), mRes, sizeof(mRes)-1);
		if (n > 0)
		{
		  q = mRes;
		  pContext->mUSER[0] = '\x0';
		  if (q = strchr(q, '\x01'))
		  { 
		    *q = '\x0';
		    q++;
		    if (r = strchr(mRes, '='))
			{
              strncpy(pContext->mUSER, r+1, _MAX_PATH-1);
	          pContext->mUSER[_MAX_PATH] = '\x0';
			}
#ifdef UPDATE_20241209 // 外部アカウントでのXOAUTH2での認証方式用にユーザ別認証コード保管ファイル(oauth2.dat)を各アカウントフォルダに設定可能にした
		    if (!CheckDomain(pContext->mUSER))
			   sprintf(pContext->mRootDir, GetXOAUTH2Path(mMailSpoolDir, pContext->mUSER, ""));
			else
#endif
            GetBaseDirectory(pContext->mRootDir, mMailBoxDir, pContext->mUSER, pContext->MyAddr);
	        if (q = strchr(q, ' '))
			{
			  while(*q == ' ')
			    q++;
			  strtok(q, "\x01");
			  ///////////////////////////////////
			  {
				// アカウントまたはドメインをキーにしてトークン問合せ先テーブルファイルからQueryを行う
				sprintf(mPAuthFile, "%soauth2.dat", pContext->mRootDir);
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
				  	  if (strstr(strlwr(mAns), "imap") || mAns[0] == '*')
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
				  	    if (mAns[0] && strstr(pContext->mUSER, mAns))
#endif
						{
					      sprintf(mReq, s, q);
						  //strtok(mReq, "\"");
#ifdef UPDATE_20241209_PIPE // 外部アカウントがらの内部アドレスにパイプ設定
						  if (t)
						  {
						    strcpy(pContext->mUSER, t);// 内部アドレスにパイプされている場合
 						    GetBaseDirectory(pContext->mRootDir, mMailBoxDir, pContext->mUSER, pContext->MyAddr);
						  }
#endif
					      break;
						}
					  }
					}
				  }
			  	  fclose(fp);
				  mAns[0]="";
                  //printf("mReq size=%d\n", strlen(mReq));
				  //printf("mReq=%s\n", mReq);
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
				    _strlwr(mAns);
                    //printf("mAns=%s\n", mAns);
				    //if (strstr(mAns, "success"))
#ifdef ADD_XOAUTH2_D // OAUTH2の認可アプリの登録メールアドレスで一致するもののみの許可するようにした。
					sprintf(str, "mOAuth2Addr=[%s], mAns=[%s]", mOAuth2Addr, mAns);
                    if (bDebug)
					  printf("%s\n", str);
#ifdef ACCEPT_LOG_LEVEL3
		            LEVEL_3_ACCEPTLOG(NULL, str);
#endif
				    bOAuth2Addr = wildcard_stricmp(mOAuth2Addr, mAns);
				    if (mAns[0] == '{' && !strstr(mAns, "\"error\"") && bOAuth2Addr)
#else
                    if (mAns[0] == '{' && !strstr(mAns, "\"error\""))
#endif
				      nSts = 0;
				  }
#ifdef WINDOWS
			      if (bOut)
                    CloseHandle(hStdOutPipeRead);
				  if (bIn)
                    CloseHandle(hStdInPipeWrite);
#endif
				}
			  }
		      //bResult = OAuth2Dispatch(lpClientContext);
			}
		  }
		}
        if (!IMAP4OffUser(lpClientContext) &&
			nSts == 0 ) // トークン確認  SUCCESS
		{
          pContext->State = Imap4Authorization; // 認証済状態
 	      if (bDebug) printf("[%08x]  pContext[%08x]->State = Imap4Authorization;\n", lpClientContext, pContext);
			  ////////////////////////////
#ifdef UPDATE_20161013 // ログイン時にルートフォルダが存在しない場合、自動作成を試みるようにした。
          if (bAutoMakeDir) // ログイン成功時にルートフォルダが存在しない場合、自動作成を試みるようにした。
            _mkdir(pContext->mRootDir);
#endif
          sprintf(pContext->mSelectDir,"%sINBOX", pContext->mRootDir);
#ifdef UPDATE_20110208 // フォルダ作成が完了するまでウエイトする。
		  bSuccess = FALSE;
		  for (i = 0; i < 5; i++)
		  {
		    if (_mkdir(pContext->mSelectDir) == 0)
			{
		      bSuccess = TRUE;
		      break;
#ifdef UPDATE_20130528 // ログイン時に作成済みのINBOXフォルダがあると、無駄に作成リトライ待ちが発生する不具合
			}
		    else if (errno == EEXIST) 
			{
		      bSuccess = TRUE;
		      break;
#endif
			}
	        _sleep(300*i);
		  }
		  if (bSuccess)
		  {
		    while ((hF = FindFirstFile(pContext->mSelectDir, &FD)) == INVALID_HANDLE_VALUE) 
			{
              if (bServiceTerminating)
  	            break;
		       _mkdir(pContext->mSelectDir);
	           _sleep(WAIT_TIME);
			} 
            if (hF) {
              FindClose( hF ); 
			}
		  }
#else
		  _mkdir(pContext->mSelectDir); // エラーチェックしない
#endif
#ifndef UPDATE_20110405 // LOGIN,AUTHENTICATED命令でUID値取得をしないようにした。（排他処理対策）
		  pContext->nUid = GetFolderUID(pContext->mSelectDir, pContext->nUid);
#endif
		  //MoveMSGFile(lpClientContext); // SELECT INBOX実施後に移動させるように // INBOXフォルダへMSGファイル移動;
          strcpy(pContext->mSelectDir, pContext->mRootDir); // ルートに初期化
          sprintf(pContext->mMessages, "%s %s AUTHENTICATE completed\r\n", mSquence, IMAP4_GOOD_RESPONSE);
		  CmdRestor(pContext, mSquence, mCmd, mToken);
          imap4log(lpClientContext, "Success");
		  pContext->nLogInID = ++nLogInID; // ID設定
		  ////////////////////////////////
		} else {
          sprintf(pContext->mMessages, "%s %s AUTHENTICATE fails\r\n", mSquence, IMAP4_RESULT_RESPONSE);
		  CmdRestor(pContext, mSquence, mCmd, mToken);
          imap4log(lpClientContext, "failed");
		}
	  } else { // 失敗
        sprintf(pContext->mMessages, "%s %s AUTHENTICATE illegal argument\r\n", mSquence, IMAP4_BADTOKEN_RESPONSE);
 	    CmdRestor(pContext, mSquence, mCmd, mToken);
        imap4log(lpClientContext, "failed");
	  }
#endif
	} else { // 未対応の認証方式
      strtok(pContext->pToken, "\r\n");
      sprintf(pContext->mMessages, "%s %s AUTHENTICATE %s isn't supported\r\n", mSquence, IMAP4_RESULT_RESPONSE, pContext->pToken);
	  CmdRestor(pContext, mSquence, mCmd, mToken);
      imap4log(lpClientContext, "failed");
	}
  } else { // 認証済み
    sprintf(pContext->mMessages, "%s %s already logged in\r\n", pContext->pSquence, IMAP4_RESULT_RESPONSE);
    CmdRestor(pContext, mSquence, mCmd, mToken);
    imap4log(lpClientContext, "failed");
  }

#ifdef UPDATE_20180820 // 暗号認証でロックアウトの処理が出来ない
  if (nSts == (DWORD)-2L)
    return FALSE;
  else
    return TRUE;
#else
  return TRUE;
#endif
}

void CmdRestor(PImap4Context pContext, char *pSquence, char *pCmd, char *pToken) {
  strcpy(pContext->pSquence, pSquence);
  strcpy(pContext->pCmd, pCmd);
  if (pContext->pToken)
    strcpy(pContext->pToken, pToken);
}