////////////////////////////////////////////////////////////
// LOGIN.C Copyright K.kawakami
////////////////////////////////////////////////////////////
#include "imap4.h"

extern BOOL  bServiceTerminating;
extern BOOL  bDebug;
extern DWORD nLogInID;
extern BOOL  bOFFPOP3;  // POP3にデータをさない
extern CHAR  mMailBoxDir[];
extern char  mMailExtension[];
#ifdef UPDATE_20090130 // レシピエントファイル(RCP)があれば一緒にコピーさせる
extern BOOL  bIgnoreRCP; // TRUE: RCPファイル無視, FALSE:RCPファイル無視しない
#endif
#ifdef UPDATE_20091110 // POP3メールボックスフォルダ位置を変更するオプション
extern BOOL  bPOP3Share;
#endif
#ifdef UPDATE_20091116 // CAPABILITY 命令で応答する認証方法を設定変更可能にした。
extern int nAuthType; // 3:CRAM-MD5&LOGIN,2:CRAM-MD5,1:LOGIN
#endif
#ifdef UPDATE_20151215 // 高負荷かつ同一アカウントへ複数ログイン時にIMAP用INBOXへのメール確認で重複メールが作成される可能性がある不具合
extern int nIMAPIndexLiveTime; // imap.idx有効時間
#endif
#ifdef UPDATE_20151215A // 高負荷かつ同一アカウントへ複数ログイン時にIMAP用INBOXへのメール確認で重複メールが作成される可能性がある不具合
extern CRITICAL_SECTION g_csMoveMess;
#endif
#ifdef UPDATE_20161013 // ログイン成功時にルートフォルダが存在しない場合、自動作成を試みるようにした。
extern BOOL   bAutoMakeDir;
#endif

#ifdef ACCEPT_LOG_LEVEL3
void LEVEL_3_ACCEPTLOG(PCLIENT_CONTEXT lpClientContext, char *mess);
#endif
void imap4log(PCLIENT_CONTEXT lpClientContext, char *bsts);
void SetSharedUserFolder(PCLIENT_CONTEXT lpClientContext);
BOOL IMAP4OffUser(PCLIENT_CONTEXT lpClientContext);
//void MoveMSGFile(PCLIENT_CONTEXT lpClientContext);
DWORD GetFolderUID(char *pFolder, DWORD nUID);
#ifdef UPDATE_20160823 // APPEND命令で存在しない追加フォルダを指定すると処理が終了できなくなる不具合
BOOL PutFolderUID(char *pFolder, DWORD nUID);
#else
void PutFolderUID(char *pFolder, DWORD nUID);
#endif
void GetBaseDirectory(CHAR *BaseDir, char *mMBdir, char *muser, char *myaddr);

BOOL LOGINDispatch(PCLIENT_CONTEXT lpClientContext) {
   
  PImap4Context pContext = &lpClientContext->Context;
  DWORD nSts = 0;
  char *p;
#ifdef UPDATE_20101229 // ユーザ名＋ドメイン付きの長さも３０バイトまでで切られてしまう不具合
  char *q;
  int  n;
#endif
#ifdef UPDATE_20110208 // フォルダ作成が完了するまでウエイトする。
  int              i;
  BOOL             bSuccess = FALSE;
  HANDLE           hF;
  WIN32_FIND_DATA  FD;
#endif
#ifdef ACCEPT_LOG_LEVEL3
  char mLog[512];
#endif
#ifdef UPDATE_20151111 // ログインパスワードにダブルクォーテーションが含まれていると失敗する //NG メーラによっては、ダブルクォーテーションが括りに使われるため
  char *s, mPS[128];
#endif
   /*
#ifdef ACCEPT_LOG_LEVEL3
  sprintf(mLog, "LOGIN:Imap4Negotiate=%d\n", Imap4Negotiate);
  LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
  LEVEL_3_ACCEPTLOG(lpClientContext, pContext->pToken);
#endif
  */
  if (pContext->State == Imap4Negotiate) { // 非認証状態
    if (pContext->pToken) {
      p = strstr(pContext->pToken, " ");
      if (p) {
	    *p = '\x0';
		/* 不要
#ifdef UPDATE_20091116 // CAPABILITY 命令で応答する認証方法を設定変更可能にした。
	    if (nAuthType == 2) // 3:CRAM-MD5&LOGIN,2:CRAM-MD5,1:LOGIN
		{
          sprintf(pContext->mMessages, "%s %s LOGIN %s isn't supported\r\n", pContext->pSquence, IMAP4_RESULT_RESPONSE, pContext->pToken);
          imap4log(lpClientContext, "failed");
          return TRUE;
		}
#endif
		*/
	    p++;
		if (*pContext->pToken == '"')
		  pContext->pToken++;
        //strcpy(pContext->mUSER, pContext->pToken);
#ifdef UPDATE_20040707
		strncpy(pContext->mUSER, pContext->pToken, sizeof(pContext->mUSER)-1 /*30*/); // ユーザーアカウントはMAX20文字ダミーで30文字まで _MAX_PATH-1);
		pContext->mUSER[sizeof(pContext->mUSER)-1] = '\x0';
#else
#ifdef UPDATE_20101229 // ユーザ名＋ドメイン付きの長さも３０バイトまでで切られてしまう不具合
		if ((q = strchr(pContext->pToken, '@')))
		{
		  *q = '\x0';
		  strncpy(pContext->mUSER, pContext->pToken, MAX_ACCOUNT_LENGTH); // ユーザーアカウントはMAX20文字ダミーで30文字まで _MAX_PATH-1);
		  pContext->mUSER[MAX_ACCOUNT_LENGTH] = '\x0';
          n = strlen(pContext->mUSER);
		  *q = '@';
		  strncat(pContext->mUSER, q, sizeof(pContext->mUSER)-1-n); // ユーザーアカウントはMAX20文字ダミーで30文字まで _MAX_PATH-1);
		  pContext->mUSER[sizeof(pContext->mUSER)-1] = '\x0';
		} else { // ユーザアカウントのみの場合
		  strncpy(pContext->mUSER, pContext->pToken, MAX_ACCOUNT_LENGTH); // ユーザーアカウントはMAX20文字ダミーで30文字まで _MAX_PATH-1);
		  pContext->mUSER[MAX_ACCOUNT_LENGTH] = '\x0';
		}
#else
		strncpy(pContext->mUSER, pContext->pToken, MAX_ACCOUNT_LENGTH); // ユーザーアカウントはMAX20文字ダミーで30文字まで _MAX_PATH-1);
		pContext->mUSER[MAX_ACCOUNT_LENGTH] = '\x0';
#endif
#endif
		strtok(pContext->mUSER, "\"");
		if (*p == '"')
		  p++;
#ifdef UPDATE_20151111 // ログインパスワードにダブルクォーテーションか円マークが含まれていると失敗する
		if (*(p+strlen(p)-1) == '"')
		{
		   *(p+strlen(p)-1) = '\x0';
		}
		s = mPS;
		while(*p)
		{
		  if (*p == '\\')
		  {
            *p++;
		  }
		  *s++ = *p++;
#ifdef UPDATE_20220317 // LOGIN命令時の受信するパスワード長を変更した(MAX 64byte)
		  if (s == &mPS[64]) // 64文字を超えない対策
#else
		  if (s == &mPS[60]) // 60文字を超えない対策
#endif
		  {
			break;
		  }
		}
		*s= '\x0';
#ifdef UPDATE_20220317 // 受信するパスワード長を変更した(MAX 64byte)
		strncpy(pContext->mPASS, mPS, 64); // ユーザーパスワードはMAX14文字ダミーで30文字まで _MAX_PATH-1);
	    pContext->mPASS[64] = '\x0';//_MAX_PATH] = '\x0';
#else
		strncpy(pContext->mPASS, mPS, 30); // ユーザーパスワードはMAX14文字ダミーで30文字まで _MAX_PATH-1);
	    pContext->mPASS[30] = '\x0';//_MAX_PATH] = '\x0';
#endif
#else
        //strcpy(pContext->mPASS, p);
		strncpy(pContext->mPASS, p, 30); // ユーザーパスワードはMAX14文字ダミーで30文字まで _MAX_PATH-1);
	    pContext->mPASS[30] = '\x0';//_MAX_PATH] = '\x0';
		strtok(pContext->mPASS, "\"");
#endif
	    //////////////////////////////////////////
 	    // 認証チェック
#ifdef UPDATE_20160113 // 認証時のロックアウト機能を追加
        nSts = CheckUser(pContext->mUSER, pContext->mPASS, pContext->MyAddr,  pContext->PeerAddr);
#else
   	    nSts = CheckUser(pContext->mUSER, pContext->mPASS, pContext->MyAddr);
#endif
        if (bDebug) printf("CheckUser()=%d [%s] [%s] [%s]\n", nSts, pContext->mUSER, pContext->mPASS, pContext->MyAddr);
		/*
#ifdef ACCEPT_LOG_LEVEL3
		sprintf(mLog, "LOGIN:CheckUser()=%d [%s] [%s] [%s]\n", nSts, pContext->mUSER, pContext->mPASS, pContext->MyAddr);
        LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
#endif
		*/
	    if (IMAP4OffUser(lpClientContext) ||
			nSts != 0) {  // 認証失敗
          sprintf(pContext->mMessages, "%s %s %s fails\r\n", pContext->pSquence, IMAP4_RESULT_RESPONSE, pContext->pCmd);
	      imap4log(lpClientContext, "failed");
		} else {           // 認証成功
          sprintf(pContext->mMessages, "%s %s %s completed\r\n", pContext->pSquence, IMAP4_GOOD_RESPONSE, pContext->pCmd);
          pContext->State = Imap4Authorization;  // 認証済状態
 	      if (bDebug) printf("[%08x]  pContext[%08x]->State = Imap4Authorization;\n", lpClientContext, pContext);
		  ////////////////////////////////
		  // ROOTフォルダ取得
	      GetBaseDirectory(pContext->mRootDir, mMailBoxDir, pContext->mUSER, pContext->MyAddr);
		  SetSharedUserFolder(lpClientContext); // 個別の共有フォルダの設定
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
		  imap4log(lpClientContext, "Success");
		  pContext->nLogInID = ++nLogInID; // ID設定
		  ////////////////////////////////
		}
	  } else { // ID または PASSWORD なし
        sprintf(pContext->mMessages, "%s %s %s illegal argument\r\n", pContext->pSquence, IMAP4_BADTOKEN_RESPONSE, pContext->pCmd );
	    imap4log(lpClientContext, "failed");
	  }
	} else {
      sprintf(pContext->mMessages, "%s %s %s illegal argument\r\n", pContext->pSquence, IMAP4_BADTOKEN_RESPONSE, pContext->pCmd );
      imap4log(lpClientContext, "failed");
	}
  } else { // 認証済み
    sprintf(pContext->mMessages, "%s %s already logged in\r\n", pContext->pSquence, IMAP4_RESULT_RESPONSE);
    imap4log(lpClientContext, "failed");
  }
#ifdef UPDATE_20180819A // 認証セッション中にロックアウト回数に達したら強制切断する
  if (nSts == (DWORD)-2L)
    return FALSE;
  else
    return TRUE;
#else
  return TRUE;
#endif
}

void SetSharedUserFolder(PCLIENT_CONTEXT lpClientContext) { // 個別の共有フォルダ設定
  FILE             *fp;
  CHAR             MailDir[MAX_PATH], mRW[8];
  PImap4Context    pContext = &lpClientContext->Context;

	sprintf(MailDir, "%s%s", pContext->mRootDir, IMAP4_SHARED_FILE);
	if ((fp = fopen(MailDir, "rt"))) { // 共有フォルダ設定ファイルに定義あればセット
	  pContext->mSharedRoot[0] = mRW[0] = '\x0';
      pContext->bSharedRW = FALSE;
      fscanf(fp, "%s", pContext->mSharedRoot);
      fscanf(fp, "%s", mRW);
	  if (!stricmp(mRW, "w"))
        pContext->bSharedRW = TRUE;
	  fclose(fp);
	}
}

BOOL IMAP4OffUser(PCLIENT_CONTEXT lpClientContext) {
  BOOL             bOFFIMAP4Person = FALSE; // 個々のIMAP4に対する対応 TRUE:IMAP4未使用, FALSE:IMAP4使用
  FILE             *fp;
  CHAR             BaseDirectory[MAX_PATH];
  CHAR             MailDir[MAX_PATH];
  PImap4Context    pContext = &lpClientContext->Context;

	GetBaseDirectory(BaseDirectory, mMailBoxDir, pContext->mUSER, pContext->MyAddr);
	sprintf(MailDir, "%s%s", BaseDirectory, IMAP4_OFF_IMAP4);
	if ((fp = fopen(MailDir, "rt"))) { // フラグがあればIMAP4は使用禁止。
	  bOFFIMAP4Person = TRUE;
	  fclose(fp);
	}
	return bOFFIMAP4Person;
}

#ifdef UPDATE_20151215 // 高負荷かつ同一アカウントへ複数ログイン時にIMAP用INBOXへのメール確認で重複メールが作成される可能性がある不具合
BOOL CheckCopyList(CHAR *pFn, CHAR *pSrc)
{
/*
#ifdef ACCEPT_LOG_LEVEL3
  char mLog[512];
#endif
*/
  FILE *fp;
  BOOL bSts = FALSE;
  CHAR mList[MAX_PATH] = "";

   if ((fp = fopen(pFn , "rt")))
   {
	 while(fgets(mList, sizeof(mList)-1, fp))
	 {
	   strtok(mList, "\r\n");
	   if (!strcmp(mList, pSrc))
	   {
/*
#ifdef ACCEPT_LOG_LEVEL3
		 sprintf(mLog, "CheckCopyList() Hit [%s]\n", pSrc);
         LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
*/
		 bSts = TRUE;
		 break;
	   }
	 }
	 fclose(fp);
   }
   return bSts;
}
#endif

void MoveMSGFile(PCLIENT_CONTEXT lpClientContext) 
{
#ifdef UPDATE_20151215 // 高負荷かつ同一アカウントへ複数ログイン時にIMAP用INBOXへのメール確認で重複メールが作成される可能性がある不具合
#ifdef ACCEPT_LOG_LEVEL3
  char mLog[512];
#endif
#endif
  BOOL             bOFFPOP3Person = FALSE; // 個々のPOP3に対する対応 TRUE:POP3未使用, FALSE:POP3使用
  FILE             *fp, *fp2;
  HANDLE           hSearch, hIdx;
  FILETIME         ftidx;
  ULARGE_INTEGER   *uidx, *usearch;
  WIN32_FIND_DATA  FindData;
  SYSTEMTIME       sidx, ssearch;
  PImap4Context    pContext = &lpClientContext->Context;
  CHAR             BaseDirectory[MAX_PATH];
  CHAR             mIMAPIDX[MAX_PATH], MailDir[MAX_PATH];
  CHAR             mSrc[MAX_PATH], mDest[MAX_PATH];
#ifdef UPDATE_20090130 // レシピエントファイル(RCP)があれば一緒にコピーさせる
  CHAR             *pRcp, mSrcRCP[MAX_PATH], mDestRCP[MAX_PATH];
#endif
#ifdef UPDATE_20110303 // UID値が0x7FFFFFFF(2147483647)を超えるとマイナス値で表示されてしまう不具合。
  int i;
#endif
#ifdef UPDATE_20151215 // 高負荷かつ同一アカウントへ複数ログイン時にIMAP用INBOXへのメール確認で重複メールが作成される可能性がある不具合
  unsigned __int64  uIdx64;
  CHAR mIMAPIDX2[MAX_PATH];
  CHAR mIMAPIDX3[MAX_PATH];
  FILETIME fc, fu;
  ULARGE_INTEGER   *ufc, *ufu;
#endif

#ifndef UPDATE_20110405A // UID値の排他処理強化
    if (!LockMailDirectory(pContext->mUSER, pContext->MyAddr)) // Lockファイルがある場合は何もしない。
	  return;
#endif
#ifdef UPDATE_20151215A // 高負荷かつ同一アカウントへ複数ログイン時にIMAP用INBOXへのメール確認で重複メールが作成される可能性がある不具合
    EnterCriticalSection(&g_csMoveMess);
	pContext->nUid = GetFolderUID(pContext->mSelectDir, pContext->nUid);
#endif
	////////////////////
	// フォルダをロック
	GetBaseDirectory(BaseDirectory, mMailBoxDir, pContext->mUSER, pContext->MyAddr);
	sprintf(MailDir, "%s%s", BaseDirectory, IMAP4_OFF_POP3);
	if ((fp = fopen(MailDir, "rt"))) { // フラグがあればPOP3は使用しない。
	  bOFFPOP3Person = TRUE;
	  fclose(fp);
	}
#ifdef UPDATE_20091110 // POP3メールボックスフォルダ位置を変更するオプション
    if (bPOP3Share)
	{
	  bOFFPOP3Person = TRUE;
	} else {
	  sprintf(MailDir, "%s%s", BaseDirectory, POP3_SHARE);
	  if ((fp = fopen(MailDir, "rt"))) { // フラグがあればPOP3のデフォルトフォルダは使用しない。
	    bOFFPOP3Person = TRUE;
	    fclose(fp);
	  }
	}
#endif
	//sprintf(MailDir, "%s%s", BaseDirectory, IMAP4_LOCKFILE);
	//fp = fopen(MailDir, "wt");
	//////////////////////////
	// imap.idxの作成日時を取得
#ifdef UPDATE_20151215 // 高負荷かつ同一アカウントへ複数ログイン時にIMAP用INBOXへのメール確認で重複メールが作成される可能性がある不具合
	usearch = NULL;
#endif
	ftidx.dwLowDateTime = ftidx.dwHighDateTime = 0; // 初期化
    sprintf(mIMAPIDX, "%simap.idx", pContext->mRootDir);  // IMAP4のコピーindex
#ifdef UPDATE_20151215 // 高負荷かつ同一アカウントへ複数ログイン時にIMAP用INBOXへのメール確認で重複メールが作成される可能性がある不具合
    sprintf(mIMAPIDX2, "%simap.$dx", pContext->mRootDir);  // IMAP4のコピーindex
    sprintf(mIMAPIDX3, "%simap.!dx", pContext->mRootDir);  // IMAP4のコピーindex
#endif
	{
      hIdx = FindFirstFile((LPCTSTR)mIMAPIDX, &FindData);
      if (hIdx != INVALID_HANDLE_VALUE) {
        GetFileTime(hIdx, NULL, &FindData.ftLastWriteTime, NULL); // アクセス日時を取得
	    ftidx.dwHighDateTime = FindData.ftLastWriteTime.dwHighDateTime;
	    ftidx.dwLowDateTime  = FindData.ftLastWriteTime.dwLowDateTime;
	    FindClose(hIdx);
	  } 
	  //// imap.idx 更新
#ifdef UPDATE_20151215 // 高負荷かつ同一アカウントへ複数ログイン時にIMAP用INBOXへのメール確認で重複メールが作成される可能性がある不具合
	  //// imap.idx 更新
	  if (!bOFFPOP3 && !bOFFPOP3Person) 
	  {
        // 今回コピーしたファイル名一覧を次回チェック用ファイルに移行
        CopyFile(mIMAPIDX2, mIMAPIDX3, FALSE);
        if ((hIdx = FindFirstFile((LPCTSTR)mIMAPIDX2, &FindData)) != INVALID_HANDLE_VALUE) 
		{
          GetFileTime(hIdx, &FindData.ftCreationTime, NULL, &FindData.ftLastWriteTime); // アクセス日時を取得
          ufc = (ULARGE_INTEGER *)&FindData.ftCreationTime;
		  ufu = (ULARGE_INTEGER *)&FindData.ftLastWriteTime;
		  if ((unsigned __int64)ufu->QuadPart > (unsigned __int64)ufc->QuadPart +(unsigned __int64)nIMAPIndexLiveTime*(unsigned __int64)10000000)
		  {
            if ((fp2 = fopen(mIMAPIDX, "wt"))) 
  	          fclose(fp2);
#ifdef UPDATE_20161006 // フォルダ選択時にインデックス情報(imap.$dx)が残っているとハンドルのクローズが抜ける
		    FindClose(hIdx);
#endif
            _unlink(mIMAPIDX2);
		  } 
#ifdef UPDATE_20161006 // フォルダ選択時にインデックス情報(imap.$dx)が残っているとハンドルのクローズが抜ける
		  else
		  {
		    FindClose(hIdx);
		  }
#endif
		}
	  } else {
        if ((fp2 = fopen(mIMAPIDX, "wt"))) 
  	      fclose(fp2);
	  }
#endif
#ifndef UPDATE_20151215 // 高負荷かつ同一アカウントへ複数ログイン時にIMAP用INBOXへのメール確認で重複メールが作成される可能性がある不具合
      if ((fp2 = fopen(mIMAPIDX, "wt"))) 
  	    fclose(fp2);
#endif
	}
	{
	//////////////////////////////////
    FileTimeToSystemTime(&ftidx, &sidx);
    if (bDebug) printf("imap.idx [%u/%u/%u %u:%u:%u] (UTC)\n", sidx.wYear, sidx.wMonth, sidx.wDay, sidx.wHour, sidx.wMinute, sidx.wSecond);
	//////////////////////////
    sprintf(MailDir, "%s\\*.%s", pContext->mRootDir, mMailExtension);  // メッセージフォルダ取出し
    hSearch = FindFirstFile((LPCTSTR)MailDir, &FindData);
    if (hSearch != INVALID_HANDLE_VALUE){
      do {
        if (FindData.dwFileAttributes & (FILE_ATTRIBUTE_DIRECTORY |
                                         FILE_ATTRIBUTE_SYSTEM |
                                         FILE_ATTRIBUTE_HIDDEN) ) {
            continue;   // ファイル以外はSkip non-normal files
        }
        if (stricmp((const char *)FindData.cFileName,(const char *)IMAP4_LOCKFILE) == 0) {
            break;      // Exit search files
		}
		sprintf(mSrc, "%s%s", pContext->mRootDir, (const char *)FindData.cFileName);
#ifdef UPDATE_20110303 // UID値が0x7FFFFFFF(2147483647)を超えるとマイナス値で表示されてしまう不具合。
		sprintf(mDest, "%s\\%010lu-100000.%s", pContext->mSelectDir, pContext->nUid, mMailExtension); // UID番号をファイル名に置き換え
#else
		sprintf(mDest, "%s\\%010ld-100000.%s", pContext->mSelectDir, pContext->nUid, mMailExtension); // UID番号をファイル名に置き換え
#endif
#ifdef UPDATE_20090130 // レシピエントファイル(RCP)があれば一緒にコピーさせる
        if (!bIgnoreRCP) // TRUE: RCPファイル無視, FALSE:RCPファイル無視しない
		{
		  strcpy(mSrcRCP, mSrc);
          if (pRcp = strrchr(mSrcRCP, '.'))
		  {
		    *pRcp = '\x0';
		    strcat(mSrcRCP, ".RCP");
		  } else {
		    mSrcRCP[0] = '\x0';
		  }
          strcpy(mDestRCP, mDest);
          if (pRcp = strrchr(mDestRCP, '.'))
		  {
		    *pRcp = '\x0';
		    strcat(mDestRCP, ".RCP");
		  } else {
		    mDestRCP[0] = '\x0';
		  }
		}
#endif
		uidx    = (ULARGE_INTEGER *)&ftidx;
        usearch = (ULARGE_INTEGER *)&FindData.ftCreationTime;
        FileTimeToSystemTime(&FindData.ftCreationTime, &ssearch);
        ///////////////////////////////////
        /// 最新メール取得
//printf("%s [%u/%u/%u %u:%u:%u] (UTC)\n",FindData.cFileName, ssearch.wYear, ssearch.wMonth, ssearch.wDay, ssearch.wHour, ssearch.wMinute, ssearch.wSecond);
#ifndef UPDATE_20110303A // POP3使用しない設定でIMAPフォルダへのメール移動処理の改修
        if (uidx->QuadPart <= usearch->QuadPart)
#endif
		{
	      if (bOFFPOP3 || bOFFPOP3Person) {  /// POP3に残さない
#ifdef UPDATE_20251105 // POP3に残さない指定時に場合に受任ボックスにファイルが実在していることを確認後にファイル移動するようにした。
			{
			  int nc = 0;
		      while(!(fp = fopen(mSrc, "rt")))
			  {
                _sleep(1);
			    if (nc++ > 10) // リトライ回数
				  break;
			  }
              fclose(fp);
			}
#endif
//printf("MoveFileEx(%s -> %s);\n",mSrc, mDest);
#ifdef UPDATE_20110301A // FETCHコマンドでフラグ変更時に拡張子"~SG"のファイルが残ってしまうタイミングが無いようにする対策
#ifdef UPDATE_20170114A // INBOXフォルダへ移動失敗時の対策を追加
		    if (rename(mSrc, mDest) == 0)
#else
		    rename(mSrc, mDest);
#endif
			{
              while(!(fp = fopen(mDest, "rt"))) { 
//if (bDebug) printf("(1)rename(%s -> %s) errno=%d\n",mSrc, mDest, errno);
#ifdef UPDATE_20110302B // ログイン時のファイル移行でループする可能性を除去する対策。
		        if (rename(mSrc, mDest) != 0) // リトライ
			    {
			      break;
			    }
#endif
                if (bServiceTerminating)
  	              break;
	             _sleep(WAIT_TIME);
			  }
			  if (fp)
			  {
			    fclose(fp);
			  }
#ifdef UPDATE_20170114A // INBOXフォルダへ移動失敗時の対策を追加
			} else {
#ifdef ACCEPT_LOG_LEVEL3
		      sprintf(mLog, "LOGIN:MoveMSGFile()=rename([%s], [%s]) Success.\n", mSrc, mDest);
              LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
#endif
#endif
			}
#else
		    MoveFileEx(mSrc, mDest, MOVEFILE_WRITE_THROUGH);       // メッセージファイル移動
#endif
#ifdef UPDATE_20090130 // レシピエントファイル(RCP)があれば一緒にコピーさせる
            if (!bIgnoreRCP) // TRUE: RCPファイル無視, FALSE:RCPファイル無視しない
			{
			  if (mSrcRCP[0] && mDestRCP)
			  {
#ifdef UPDATE_20110301A // FETCHコマンドでフラグ変更時に拡張子"~SG"のファイルが残ってしまうタイミングが無いようにする対策
#ifdef UPDATE_20170114A // INBOXフォルダへ移動失敗時の対策を追加
		        if (rename(mSrcRCP, mDestRCP) == 0)
#else
				rename(mSrcRCP, mDestRCP);
#endif
				{
                  while(!(fp = fopen(mDestRCP, "rt"))) { 
//if (bDebug) printf("(2)rename(%s -> %s) errno=%d\n",mSrcRCP, mDestRCP, errno);
#ifdef UPDATE_20110302B // ログイン時のファイル移行でループする可能性を除去する対策。
		            if (rename(mSrcRCP, mDestRCP) != 0) // リトライ
				    {
			          break;
				    }
#endif
                    if (bServiceTerminating)
  	                  break;
	                _sleep(WAIT_TIME);
				  }
			      if (fp)
				  { 
			        fclose(fp);
				  }
#ifdef UPDATE_20170114A // INBOXフォルダへ移動失敗時の対策を追加
			    } else {
#ifdef ACCEPT_LOG_LEVEL3
		          sprintf(mLog, "LOGIN:MoveMSGFile()=rename([%s], [%s]) Success.\n", mSrcRCP, mDestRCP);
                  LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
#endif
#endif
				}
#else
		        MoveFileEx(mSrcRCP, mDestRCP, MOVEFILE_WRITE_THROUGH);       // メッセージファイル移動
#endif
			  }
			}
#endif
#ifdef UPDATE_20110303A // POP3使用しない設定でIMAPフォルダへのメール移動処理の改修
		    pContext->nUid++; //UID値カウントアップ
		    if (pContext->nUid == 0)
			  pContext->nUid = 1;
#endif
		  } else {          /// POP3に残す
/*
#ifdef UPDATE_20151215 // 高負荷かつ同一アカウントへ複数ログイン時にIMAP用INBOXへのメール確認で重複メールが作成される可能性がある不具合
#ifdef ACCEPT_LOG_LEVEL3
		   sprintf(mLog, "LOGIN:MoveMSGFile(%I64u)=uidx->QuadPart(%I64u)<=usearch->QuadPart(%I64u[%s]) (CheckCopyList()=%s)\n", uIdx64, uidx->QuadPart, usearch->QuadPart, FindData.cFileName, (CheckCopyList(mIMAPIDX3, FindData.cFileName) ? "TRUE" : "FALSE"));
           LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
#endif
#endif
*/
#ifdef UPDATE_20110303A // POP3使用しない設定でIMAPフォルダへのメール移動処理の改修
#ifdef UPDATE_20151215 // 高負荷かつ同一アカウントへ複数ログイン時にIMAP用INBOXへのメール確認で重複メールが作成される可能性がある不具合
            if (uidx->QuadPart <= usearch->QuadPart && !CheckCopyList(mIMAPIDX3, FindData.cFileName))
//            if (uidx->QuadPart < usearch->QuadPart || 
//				uidx->QuadPart == usearch->QuadPart && !CheckCopyList(mIMAPIDX3, FindData.cFileName))
#else
            if (uidx->QuadPart <= usearch->QuadPart)
#endif
			{
#endif
//if (bDebug) printf("CopyFile(%s -> %s);\n",mSrc, mDest);
			  if (CopyFile(mSrc, mDest, TRUE))  // メッセージファイルコピー（POP3でも利用可）
			  {
//if (bDebug) printf("(1)CopyFile(%s -> %s) GetLastError()=%d\n",mSrc, mDest, GetLastError());
#ifdef UPDATE_20151215 // 高負荷かつ同一アカウントへ複数ログイン時にIMAP用INBOXへのメール確認で重複メールが作成される可能性がある不具合
			    if ((fp2 = fopen(mIMAPIDX2, "at"))) // 今回コピーしたファイル一覧を記録
				{
	              fprintf(fp2, "%s\n", FindData.cFileName);
                  fclose(fp2);
				}
#ifdef ACCEPT_LOG_LEVEL3
		      sprintf(mLog, "LOGIN:MoveMSGFile()=CopyFile([%s], [%s]) Success.\n", mSrc, mDest);
              LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
#endif
#endif
#ifdef UPDATE_20090130 // レシピエントファイル(RCP)があれば一緒にコピーさせる
                if (!bIgnoreRCP) // TRUE: RCPファイル無視, FALSE:RCPファイル無視しない
				{
			      if (mSrcRCP[0] && mDestRCP)
				  {
                    CopyFile(mSrcRCP, mDestRCP, TRUE);
//if (bDebug) printf("(2)CopyFile(%s -> %s) GetLastError()=%d\n",mSrcRCP, mDestRCP, GetLastError());
				  }
				}
#endif
			    /////////////////////////////////////////////
			    // 成功なら
			    // コピーファイルが実体化されるまでウェイト
                while(!(fp = fopen(mDest, "rt"))) { 
#ifdef UPDATE_20110302B // ログイン時のファイル移行でループする可能性を除去する対策。
		          if (CopyFile(mSrc, mDest, TRUE) == 0) // リトライ
				  {
			        break;
				  }
#endif
                  if (bServiceTerminating)
    	            break;
	              _sleep(WAIT_TIME);
				}
			    if (fp)
				{
			      fclose(fp);
				}
			    /////////////////////////////////////////////
#ifdef UPDATE_20151215 // 高負荷かつ同一アカウントへ複数ログイン時にIMAP用INBOXへのメール確認で重複メールが作成される可能性がある不具合
			  } else {
#ifdef ACCEPT_LOG_LEVEL3
		        sprintf(mLog, "LOGIN:MoveMSGFile(%I64u<=%I64u):CopyFile(%s, %s) Fail. GetLastError()=%d\n", uidx->QuadPart, usearch->QuadPart, mSrc, mDest, GetLastError());
                LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
#endif
#endif
			  }
#ifdef UPDATE_20110303A // POP3使用しない設定でIMAPフォルダへのメール移動処理の改修
		      pContext->nUid++; //UID値カウントアップ
		      if (pContext->nUid == 0)
			    pContext->nUid = 1;
#endif
			}
#ifndef UPDATE_20110303A // POP3使用しない設定でIMAPフォルダへのメール移動処理の改修
		    pContext->nUid++; //UID値カウントアップ
		    if (pContext->nUid == 0)
			  pContext->nUid = 1;
#endif
		  }
#ifndef UPDATE_20110303A // POP3使用しない設定でIMAPフォルダへのメール移動処理の改修
		} else {
		  if (bOFFPOP3 || bOFFPOP3Person) {  /// POP3に残さない
		    DeleteFile(mSrc);       // 古いファイルは削除
#ifdef UPDATE_20090130 // レシピエントファイル(RCP)があれば一緒にコピーさせる
            if (!bIgnoreRCP) // TRUE: RCPファイル無視, FALSE:RCPファイル無視しない
			{
			  if (mSrcRCP)
			  {
		        DeleteFile(mSrcRCP);       // 古いファイルは削除
			  }
			}
#endif
		  }
#endif
		}
	  } while (FindNextFile(hSearch, &FindData));
	  FindClose(hSearch);
/*
#ifdef UPDATE_20151215 // 高負荷かつ同一アカウントへ複数ログイン時にIMAP用INBOXへのメール確認で重複メールが作成される可能性がある不具合
	  //// imap.idx 更新
	  if (usearch) // NULLでないとき
	  {
		if (!bOFFPOP3 && !bOFFPOP3Person) 
		{
          // 今回コピーしたファイル名一覧を次回チェック用ファイルに移行
          CopyFile(mIMAPIDX2, mIMAPIDX3, FALSE);
          if ((hIdx = FindFirstFile((LPCTSTR)mIMAPIDX2, &FindData)) != INVALID_HANDLE_VALUE) 
		  {
            GetFileTime(hIdx, &FindData.ftCreationTime, NULL, &FindData.ftLastWriteTime); // アクセス日時を取得
            ufc = (ULARGE_INTEGER *)&FindData.ftCreationTime;
		    ufu = (ULARGE_INTEGER *)&FindData.ftLastWriteTime;
			if ((unsigned __int64)ufu->QuadPart > (unsigned __int64)ufc->QuadPart +(unsigned __int64)nIMAPIndexLiveTime*(unsigned __int64)10000000)
			{
              if ((fp2 = fopen(mIMAPIDX, "wt"))) 
  	            fclose(fp2);
              _unlink(mIMAPIDX2);
			}
		  } 
		}
	  }
#endif
*/
	  ///////////////////////////////
	  /// UID設定
      PutFolderUID(pContext->mSelectDir, pContext->nUid);
/*
	} else {
#ifdef UPDATE_20151215 // 高負荷かつ同一アカウントへ複数ログイン時にIMAP用INBOXへのメール確認で重複メールが作成される可能性がある不具合
#ifdef ACCEPT_LOG_LEVEL3
	  sprintf(mLog, "LOGIN:MoveMSGFile()=[%s] using.\n", mIMAPIDX);
      LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
#endif
#endif
*/
	}
	}
	///////////////////////////////
	/// ロック解除
	//if (fp)
	//  fclose(fp);
#ifdef UPDATE_20151215A // 高負荷かつ同一アカウントへ複数ログイン時にIMAP用INBOXへのメール確認で重複メールが作成される可能性がある不具合
    LeaveCriticalSection(&g_csMoveMess);
#endif
#ifndef UPDATE_20110405A // UID値の排他処理強化
    UnLockMailDirectory(pContext->mUSER, pContext->MyAddr);
#endif
}
