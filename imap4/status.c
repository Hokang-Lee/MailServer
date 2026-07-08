////////////////////////////////////////////////////////////
// STATUS.C Copyright K.kawakami
////////////////////////////////////////////////////////////
#include "imap4.h"

extern CHAR    mInboxAlias[];

void GetMSGFlags(PCLIENT_CONTEXT lpClientContext);
void MoveMSGFile(PCLIENT_CONTEXT lpClientContext);
DWORD GetFolderUID(char *pFolder, DWORD nUID);
BOOL  MakeUIDVALIDITY(PImap4Context pContext);

BOOL STATUSDispatch(PCLIENT_CONTEXT lpClientContext) {
  char    mTempDir[MAX_PATH], *pCurrent, *pRequest, *q;
  char    mWork[256];
  PImap4Context pContext = &lpClientContext->Context;
  BOOL    bAnswer = FALSE;
  DWORD   nUID = 1;       // ユニークID
  DWORD   nUidValidity[2];   // ユニーク識別子有効値
  BOOL    bShared = FALSE;
#ifdef UPDATE_20130531 // フォルダ名に半角スペースがあると処理が失敗することがある不具合
  char *pDest3 = NULL;
  char c = '\x0';
#endif

  if (pContext->State >= Imap4Authorization) {
	pCurrent    = pContext->pToken;
	pRequest    = strstr(pContext->pToken, " ");
#ifdef UPDATE_20130531 // フォルダ名に半角スペースがあると処理が失敗することがある不具合
	if (*pContext->pToken == '"')
	{
	  if ((pDest3 = strstr(pContext->pToken+1, "\"")))
	  {
	    pRequest = strstr(pDest3, " ");
	  }
	}
#endif
	if (pRequest) {
	  *pRequest = '\x0';
	  pRequest++;
	  if (*pRequest == '(') {
	    pRequest++;
	    strtok(pRequest, ")");
	  }
	} else {
      sprintf(pContext->mMessages, "%s %s illegal argument.(%s)\r\n", pContext->pSquence, IMAP4_BADTOKEN_RESPONSE, (pContext->pCmd ? _strupr(pContext->pCmd) : "NULL"));
	  return TRUE;
	}
	if (*pCurrent == '"') {
#ifdef UPDATE_20130531 // フォルダ名に半角スペースがあると処理が失敗することがある不具合
	   c = '"';
#endif
	   pCurrent++;
	   strtok(pCurrent, "\"");
       if (*pCurrent == '"')
         *pCurrent = '\x0';
#ifdef UPDATE_20050531  // 命令にバッファフローの脆弱性
	  if (strlen(pCurrent) > MAXIMAPFOLDER) // フォルダ名が長い場合の処置
		*(pCurrent+MAXIMAPFOLDER) = '\x0';
#endif
	}
	strcpy(mTempDir, pContext->mSelectDir); // バックアップ
    if (!strncmp(pCurrent, mInboxAlias, strlen(mInboxAlias))) {// 受信トレイの別名称の場合
      q = strpbrk(pCurrent, "/\\");
      sprintf(pContext->mSelectDir,"%sINBOX%s", pContext->mRootDir, (q ? q : ""));
	} else if (pContext->mSharedRoot[0] && !strnicmp(pCurrent, IMAP4_SHARED_NAME, 8)) { // 共通フォルダの指定
	  q = strpbrk(pCurrent, "/\\");
	  sprintf(pContext->mSelectDir,"%s%s",pContext->mSharedRoot, ++q);
	  bShared = TRUE;
	} else {
#ifdef UPDATE_20050531  // 命令にバッファフローの脆弱性
	  strcpy(pContext->mSelectDir, pContext->mRootDir);
	  strncat(pContext->mSelectDir, pCurrent, sizeof(pContext->mSelectDir)-strlen(pContext->mRootDir)-1);
	  pContext->mSelectDir[sizeof(pContext->mSelectDir)-1] = '\x0';
#else
      sprintf(pContext->mSelectDir,"%s%s",pContext->mRootDir, pCurrent);
#endif
	}

#ifdef UPDATE_20110405A // UID値の排他処理強化
    nUID = CriticalUID(lpClientContext, pCurrent);
#else
#ifdef UPDATE_20060807 // 選択中のフォルダのuid 値をINBOXフォルダのUIDと置き換わる可能性のある不具合
    pContext->nUid = GetFolderUID(pContext->mSelectDir, pContext->nUid);
#endif
    if (!stricmp(pCurrent, "inbox")) // INBOXフォルダ選択なら
      MoveMSGFile(lpClientContext);
	GetMSGFlags(lpClientContext); // フラグ集計
	nUID = GetFolderUID(pContext->mSelectDir, nUID); // 指定フォルダのUID値
#endif
	//////////////////////////////////////////
	// 指定フォルダごとのユニーク識別子有効値定義
	nUidValidity[0] = pContext->nUidValidity; // デフォルト値保存
    MakeUIDVALIDITY(pContext);
    nUidValidity[1] = pContext->nUidValidity; // 指定フォルダのユニーク識別子有効値
    pContext->nUidValidity = nUidValidity[0]; // デフォルト値復帰
	//////////////////////////////////////////
	strcpy(pContext->mSelectDir, mTempDir); // リストア
    //sprintf(pContext->mMessages, "%s %s %s %s (", pContext->pSquence, IMAP4_GOOD_RESPONSE, pContext->pCmd , pCurrent);
#ifdef UPDATE_20130531 // フォルダ名に半角スペースがあると処理が失敗することがある不具合
    sprintf(pContext->mMessages, "* %s %s%s%s (", pContext->pCmd , (c ? "\"" : "") , pCurrent, (c ? "\"" : ""));
#else
    sprintf(pContext->mMessages, "* %s %s (", pContext->pCmd , pCurrent);
#endif
	if (strstr(pRequest,"MESSAGES") || strstr(pRequest,"messages") ) { // メールボックスのメッセージの数。 
	  sprintf(mWork,"%sMESSAGES %lu", (bAnswer ? " " : ""), pContext->nExists); 
	  strcat(pContext->mMessages, mWork);
	  bAnswer = TRUE;
	}
	if (strstr(pRequest,"RECENT") || strstr(pRequest,"recent")) {   // \Recent フラグがセットされたメッセージの数。 
	  sprintf(mWork,"%sRECENT %lu", (bAnswer ? " " : ""), pContext->nRecent); 
	  strcat(pContext->mMessages, mWork);
	  bAnswer = TRUE;
	}
	if (strstr(pRequest,"UIDNEXT") || strstr(pRequest,"uidnext") ) {  // そのメールボックスで新規メッセージに割り当てられるであろう次の UID 値。 この値は新規メッセージがそのメールボックスに追加されない限り、変わらず、 そして、新規メッセージが追加されたとき、 もしそれらの新規メッセージがその後消去されたとしても、変わる ことが保証される。 
	  sprintf(mWork,"%sUIDNEXT %lu", (bAnswer ? " " : ""), nUID); 
	  strcat(pContext->mMessages, mWork);
	  bAnswer = TRUE;
	}
	if (strstr(pRequest,"UIDVALIDITY") || strstr(pRequest,"uidvalidity") ) { //そのメールボックスのユニーク識別子有効値。 
	  sprintf(mWork,"%sUIDVALIDITY %lu", (bAnswer ? " " : ""), nUidValidity[1]); 
	  strcat(pContext->mMessages, mWork);
	  bAnswer = TRUE;
	}
    if (strstr(pRequest,"UNSEEN") || strstr(pRequest,"unseen") ) { // \Seen フラグがセットされていないメッセージの数
	  sprintf(mWork,"%sUNSEEN %lu", (bAnswer ? " " : ""), pContext->nUnseen); 
	  strcat(pContext->mMessages, mWork);
	  bAnswer = TRUE;
	}
    strcat(pContext->mMessages, ")\r\n");
    sprintf(mWork, "%s %s %s completed\r\n", pContext->pSquence, IMAP4_GOOD_RESPONSE, pContext->pCmd);
    strcat(pContext->mMessages, mWork);
  } else { // 状態が違う
    sprintf(pContext->mMessages, "%s %s Command unrecognized: %s\r\n", pContext->pSquence, IMAP4_RESULT_RESPONSE, pContext->pCmd );
  }
  return TRUE;
}