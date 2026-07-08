////////////////////////////////////////////////////////////
// APPEND.C Copyright K.kawakami
////////////////////////////////////////////////////////////
#include "imap4.h"

extern BOOL bServiceTerminating;
extern CHAR    mInboxAlias[];

BOOL AppendMessage(PCLIENT_CONTEXT lpClientContext, char *pFn, DWORD nLUID, DWORD nSize, char *pFlags);
DWORD GetFolderUID(char *pFolder, DWORD nUID);
#ifdef UPDATE_20160823 // APPEND命令で存在しない追加フォルダを指定すると処理が終了できなくなる不具合
BOOL PutFolderUID(char *pFolder, DWORD nUID);
#else
void PutFolderUID(char *pFolder, DWORD nUID);
#endif
#ifdef UPDATE_20110404 // 選択フォルダ単位で排他する対策
HANDLE LockMailSelectDirectory(PCHAR  pszPath);
BOOL UnLockMailSelectDirectory(PCHAR  pszPath);
#endif

BOOL APPENDDispatch(PCLIENT_CONTEXT lpClientContext) {
  BOOL    bsts = FALSE, bAns = FALSE;
  char    *pCurrent, *pRequest, *pSize, *q;
  char    mSquence[32], mTempDir[MAX_PATH], mToken[1024];
  DWORD   nSize = 0, nLUID = 1;
  PImap4Context pContext = &lpClientContext->Context;
  BOOL             bShared = FALSE;
#ifdef UPDATE_20160823 // APPEND命令で存在しない追加フォルダを指定すると処理が終了できなくなる不具合
  BOOL  bput = TRUE;
#endif

  if (pContext->State >= Imap4Authorization) {
    strncpy(mSquence, pContext->pSquence, sizeof(mSquence));
    mSquence[sizeof(mSquence)-1] = '\x0';
	strncpy(mToken, pContext->pToken, sizeof(mToken)-1); //　受信があるので命令をバックアップ
	mToken[sizeof(mToken)-1] = '\x0';
	pCurrent    = mToken;
#ifdef UPDATE_20140523 // APPEND元のフォルダ名に空白が含まれていると実行が失敗する
	pRequest    = strstr(mToken, " (");
#else
	pRequest    = strpbrk(mToken, " (");//strstr(mToken, " ");
#endif
	pSize       = strstr(mToken, "{");
	if (lpClientContext->Queue[0] == '\x0') //　データも転送されている
	  bAns = TRUE;
	if (pSize) {
	  *pSize = '\x0';
	  pSize++;
	  strtok(pSize, "{");
	  nSize = atol(pSize);
	}
	if (pRequest) {
	  *pRequest = '\x0';
	  pRequest++;
	  if (*pRequest == '(') {
	    pRequest++;
	    strtok(pRequest, ")");
	  }
	}
	if (*pCurrent == '"') {
	  pCurrent++;
	  strtok(pCurrent, "\"");
      if (*pCurrent == '"')
	    *pCurrent = '\x0';
#ifdef UPDATE_20050531  // 命令にバッファフローの脆弱性
	  if (strlen(pCurrent) > MAXIMAPFOLDER) // フォルダ名が長い場合の処置
		*(pCurrent+MAXIMAPFOLDER) = '\x0';
#endif
	}
	if (!strncmp(pCurrent, mInboxAlias, strlen(mInboxAlias))) {// 受信トレイの別名称の場合
	  q = strpbrk(pCurrent, "/\\");
      sprintf(mTempDir,"%sINBOX%s",pContext->mRootDir, (q ? q : ""));
	} else if (pContext->mSharedRoot[0] && !strnicmp(pCurrent, IMAP4_SHARED_NAME, 8)) { // 共通フォルダの指定
      q = strpbrk(pCurrent, "/\\");
	  sprintf(mTempDir,"%s%s",pContext->mSharedRoot, ++q);
      bShared = TRUE;
	} else {
#ifdef UPDATE_20050531  // 命令にバッファフローの脆弱性
	  strcpy(mTempDir, pContext->mRootDir);
	  strncat(mTempDir, pCurrent, MAX_PATH-strlen(pContext->mRootDir)-1);
	  mTempDir[MAX_PATH-1] = '\x0';
#else
      sprintf(mTempDir,"%s%s",pContext->mRootDir, pCurrent);
#endif
	}
#ifdef UPDATE_20060116  // ユーザルートより上位のフォルダにアクセスできる脆弱性３
    if (!strcmp(pContext->mRootDir, mTempDir) ||
		!FolderRight2(pContext, (char *)mTempDir)) {
      sprintf(pContext->mMessages, "%s %s %s doesn't have right\r\n", pContext->pSquence, IMAP4_RESULT_RESPONSE, pContext->pCmd );
      return TRUE;
	}
#endif
    if (!bShared || bShared && pContext->bSharedRW) {
	  if (nSize > 0) {
#ifdef UPDATE_20110404 // 選択フォルダ単位で排他する対策
        while(!LockMailSelectDirectory(mTempDir)) // Lockファイルがある場合は何もしない。
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
        nLUID = GetFolderUID(mTempDir, nLUID);
	    if (bAns) {
          strcpy(pContext->mMessages, "+ Ready for literal data\r\n");
          put_reply(lpClientContext, TRUE, TRUE);
		}
	    AppendMessage(lpClientContext, mTempDir, nLUID, nSize, pRequest); // メッセージ追加
	    nLUID++; //UID値カウントアップ
	    if (nLUID == 0)
		  nLUID = 1;
#ifdef UPDATE_20050531  // 命令にバッファフローの脆弱性
	    strcpy(mTempDir, pContext->mRootDir);
	    strncat(mTempDir, pCurrent, MAX_PATH-strlen(pContext->mRootDir)-1);
	    mTempDir[MAX_PATH-1] = '\x0';
#else
        sprintf(mTempDir,"%s%s",pContext->mRootDir, pCurrent);
#endif
#ifdef UPDATE_20160823 // APPEND命令で存在しない追加フォルダを指定すると処理が終了できなくなる不具合
        bput = PutFolderUID(mTempDir, nLUID);
#else
        PutFolderUID(mTempDir, nLUID);
#endif
#ifdef UPDATE_20110404 // 選択フォルダ単位で排他する対策
        UnLockMailSelectDirectory(mTempDir);
#endif
#ifdef UPDATE_20160823 // APPEND命令で存在しない追加フォルダを指定すると処理が終了できなくなる不具合
        if (!bput) // フォルダが見つからないとき
		{
          sprintf(pContext->mMessages, "%s %s %s doesn't have right\r\n", pContext->pSquence, IMAP4_RESULT_RESPONSE, pContext->pCmd );
		} else
#endif
		{
          sprintf(pContext->mMessages, "%s %s APPEND completed\r\n", mSquence, IMAP4_GOOD_RESPONSE);
		}
	    ///////////////////////////////
	  } else { // 保存サイズが未指定
        sprintf(pContext->mMessages, "%s %s %s illegal argument\r\n", pContext->pSquence, IMAP4_BADTOKEN_RESPONSE, pContext->pCmd );
	  }
	} else {
      sprintf(pContext->mMessages, "%s %s %s doesn't have right\r\n", pContext->pSquence, IMAP4_RESULT_RESPONSE, pContext->pCmd );
	}
  } else { // 状態が違う
    sprintf(pContext->mMessages, "%s %s Command unrecognized: %s\r\n", pContext->pSquence, IMAP4_RESULT_RESPONSE, pContext->pCmd );
  }
  return TRUE;
}

BOOL AppendMessage(PCLIENT_CONTEXT lpClientContext, char *pFn, DWORD nLUID, DWORD nSize, char *pFlags) {
  BOOL    bsts = FALSE;
  FILE    *fp;
  char    mFn[MAX_PATH];
  PImap4Context pContext = &lpClientContext->Context;
  DWORD   nTotal = 0;
#ifdef UPDATE_20110126 // １カラム目のピリオドは出力しない対策
  BOOL    bRead = FALSE;
  FILE    *fp2;
  char    mFn2[MAX_PATH];
  char    mLine[2048];
#endif

#ifdef UPDATE_20110303 // UID値が0x7FFFFFFF(2147483647)を超えるとマイナス値で表示されてしまう不具合。
  sprintf(mFn, "\\%010lu-100000.MSG", nLUID);
#else
  sprintf(mFn, "\\%010ld-100000.MSG", nLUID);
#endif
#ifdef UPDATE_20140523A // APPEND実行時のフラグが未指定の時ハングする
  if (pFlags)
#endif
  {
    if (strstr(pFlags, "\\Answered")) {
      mFn[UIDLEN+ANSWERED+1] = '1';
	}
    if (strstr(pFlags, "\\Flagged")) {
      mFn[UIDLEN+FLAGGED+1] = '1';
	}
    if (strstr(pFlags, "\\Deleted")) {
      mFn[UIDLEN+DELETED+1] = '1';
	}
    if (strstr(pFlags, "\\Seen")) {
      mFn[UIDLEN+SEEN+1] = '1';
	}
    if (strstr(pFlags, "\\Draft")) {
      mFn[UIDLEN+DRAFT+1] = '1';
	}
  }
#ifdef UPDATE_20110126 // １カラム目のピリオドは出力しない対策
  sprintf(mFn2, "%s%s", pFn, mFn);
  mFn2[strlen(mFn2)-3] = '$'; // .%SG
#endif
  strcat(pFn, mFn);

  fp = fopen(pFn, "wb");
  if (fp) {
	if (lpClientContext->Queue[0] != '\x0') {//　データも転送されている
	  bsts = TRUE;
	  nTotal += strlen(lpClientContext->Queue);
	  fputs(lpClientContext->Queue, fp);
      lpClientContext->Queue[0] = '\x0';
	}
	while (nTotal < nSize+2) {
	  bsts = get_reply(lpClientContext, FALSE, "APPEND"); //TRUE);
#ifdef UPDATE_20060117 // 強制切断でCPUが１００％状態になる
	  if (!bsts)
		break;
#endif
      nTotal += strlen(lpClientContext->Buffer);
      fputs(lpClientContext->Buffer, fp);
	}
	//fputs("\r\n.\r\n", fp); // 終了コード保存
	fclose(fp);
  }
#ifdef UPDATE_20110126 // １カラム目のピリオドは出力しない対策
  if (bsts) // 成功
  {
    while(!(fp = fopen(pFn, "rb")))
    {
      if (bServiceTerminating)
  	    break;
	   _sleep(WAIT_TIME);
	}
	if (fp)
	{
	  fclose(fp);
	}
    if ((fp = fopen(pFn, "rb")))
	{
      if ((fp2 = fopen(mFn2, "wb")))
	  {
	    bRead = TRUE;
		while(fgets(mLine, sizeof(mLine)-1, fp))
		{
		  if (mLine[0] == '.')
		  {
			fputc('.', fp2);
		  }
		  fputs(mLine, fp2);
		}
	    fclose(fp2);
        while(!(fp2 = fopen(mFn2, "rb")))
		{
          if (bServiceTerminating)
  	        break;
	      _sleep(WAIT_TIME);
		}
		if (fp2)
		{
	      fclose(fp2);
		}
	  }
	  fclose(fp);
	}
	if (bRead)
	{
      CopyFile(mFn2, pFn, FALSE);
	  DeleteFile(mFn2);
	}
  }
#endif
  if (!bsts) { // 失敗
	DeleteFile(pFn);
  }
  return bsts;
}