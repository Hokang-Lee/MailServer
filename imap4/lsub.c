////////////////////////////////////////////////////////////
// LSUB.C Copyright K.kawakami
////////////////////////////////////////////////////////////
#include "imap4.h"

void ListDir(PCLIENT_CONTEXT lpClientContext, char *pRoot, char *pCurrent, BOOL bHidden);

BOOL LSUBDispatch(PCLIENT_CONTEXT lpClientContext) {
  char    *pRoot, *pCurrent;
  PImap4Context pContext = &lpClientContext->Context;

  if (pContext->State >= Imap4Authorization) {
	///////////////////////////////////////
	/// フォルダを作成
	pRoot    = pContext->pToken;
	pCurrent = strstr(pContext->pToken, " ");
	if (pCurrent) {
	  *pCurrent = '\x0';
	  pCurrent++;
	  if (*pCurrent == '"') {
	    pCurrent++;
	    strtok(pCurrent, "\"");
        if (*pCurrent == '"')
          *pCurrent = '\x0';
	  }
#ifdef UPDATE_20050531  // 命令にバッファフローの脆弱性
	  if (strlen(pCurrent) > MAXIMAPFOLDER) // フォルダ名が長い場合の処置
		*(pCurrent+MAXIMAPFOLDER) = '\x0';
#endif
	}
	if (*pRoot == '"') {
	   pRoot++;
	   strtok(pRoot, "\"");
       if (*pRoot == '"')
         *pRoot = '\x0';
#ifdef UPDATE_20050531  // 命令にバッファフローの脆弱性
	  if (strlen(pRoot) > MAXIMAPFOLDER) // フォルダ名が長い場合の処置
		*(pRoot+MAXIMAPFOLDER) = '\x0';
#endif
	}
	if (pRoot && pCurrent) {
	  pContext->mLinkRoot[0] = '\x0';
	  ListDir(lpClientContext, pRoot, pCurrent, FALSE); // 購読済みのみフォルダ
      sprintf(pContext->mMessages, "%s %s %s completed\r\n", pContext->pSquence, IMAP4_GOOD_RESPONSE, pContext->pCmd );
	} else { // フォルダ指定なし
      sprintf(pContext->mMessages, "%s %s %s illegal argument\r\n", pContext->pSquence, IMAP4_BADTOKEN_RESPONSE, pContext->pCmd );
	}
  } else { // 状態が違う
    sprintf(pContext->mMessages, "%s %s Command unrecognized: %s\r\n", pContext->pSquence, IMAP4_RESULT_RESPONSE, pContext->pCmd );
  }
  return TRUE;
}
