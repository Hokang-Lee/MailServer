////////////////////////////////////////////////////////////
// SUBSCRIBE.C Copyright K.kawakami
////////////////////////////////////////////////////////////
#include "imap4.h"

extern CHAR    mInboxAlias[];

BOOL SUBSCRIBEDispatch(PCLIENT_CONTEXT lpClientContext) {
  char    mTempDir[MAX_PATH], *p, *q;
  PImap4Context pContext = &lpClientContext->Context;
  DWORD nAttrib;
  BOOL             bShared = FALSE;

  if (pContext->State >= Imap4Authorization) {
	///////////////////////////////////////
	/// フォルダを作成
	p = pContext->pToken;
	if (*p == '"') {
	   p++;
	   strtok(p, "\"");
       if (*p == '"')
         *p = '\x0';
#ifdef UPDATE_20050531  // 命令にバッファフローの脆弱性
	  if (strlen(p) > MAXIMAPFOLDER) // フォルダ名が長い場合の処置
		*(p+MAXIMAPFOLDER) = '\x0';
#endif
	}

	if (p) {
	  if (!strncmp(p, mInboxAlias, strlen(mInboxAlias))) {// 受信トレイの別名称の場合
	    q = strpbrk(p, "/\\");
        sprintf(mTempDir,"%sINBOX%s",pContext->mRootDir, (q ? q : ""));
	  } else if (pContext->mSharedRoot[0] && !strnicmp(p, IMAP4_SHARED_NAME, 8)) { // 共通フォルダの指定
        q = strpbrk(p, "/\\");
        sprintf(mTempDir,"%s%s",pContext->mSharedRoot, ++p);
 	    bShared = TRUE;
	  } else {
#ifdef UPDATE_20050531  // 命令にバッファフローの脆弱性
	    strcpy(mTempDir, pContext->mRootDir);
	    strncat(mTempDir, p, MAX_PATH-strlen(pContext->mRootDir)-1);
	    mTempDir[MAX_PATH-1] = '\x0';
#else
        sprintf(mTempDir,"%s%s",pContext->mRootDir, p);
#endif
	  }
      if (!bShared || bShared && pContext->bSharedRW) {
        nAttrib = GetFileAttributes(mTempDir);
	    nAttrib = FILE_ATTRIBUTE_NORMAL; // 隠しファイル解除（購読対象として扱う）
        if (!SetFileAttributes(mTempDir, nAttrib)) // 失敗
		  sprintf(pContext->mMessages, "%s %s %s failed\r\n", pContext->pSquence, IMAP4_RESULT_RESPONSE, pContext->pCmd );
		else
          sprintf(pContext->mMessages, "%s %s %s completed\r\n", pContext->pSquence, IMAP4_GOOD_RESPONSE, pContext->pCmd );
	  } else {
        sprintf(pContext->mMessages, "%s %s %s doesn't have right\r\n", pContext->pSquence, IMAP4_RESULT_RESPONSE, pContext->pCmd );
	  }
	} else { // フォルダ指定なし
      sprintf(pContext->mMessages, "%s %s %s illegal argument\r\n", pContext->pSquence, IMAP4_BADTOKEN_RESPONSE, pContext->pCmd );
	}
  } else { // 状態が違う
    sprintf(pContext->mMessages, "%s %s Command unrecognized: %s\r\n", pContext->pSquence, IMAP4_RESULT_RESPONSE, pContext->pCmd );
  }
  return TRUE;
}