////////////////////////////////////////////////////////////
// EXPUNGE.C Copyright K.kawakami
////////////////////////////////////////////////////////////
#include "imap4.h"

void DeleteMessage(PCLIENT_CONTEXT lpClientContext, BOOL bRes);

BOOL EXPUNGEDispatch(PCLIENT_CONTEXT lpClientContext) {
   
  PImap4Context pContext = &lpClientContext->Context;

  if (pContext->State == Imap4SelectFolder) {  // フォルダ選択済状態
    DeleteMessage(lpClientContext, TRUE); // メッセージ削除
    sprintf(pContext->mMessages, "%s %s %s completed\r\n", pContext->pSquence, IMAP4_GOOD_RESPONSE, pContext->pCmd );
  } else { // 状態が違う
    sprintf(pContext->mMessages, "%s %s Command unrecognized: %s\r\n", pContext->pSquence, IMAP4_RESULT_RESPONSE, pContext->pCmd );
  }
  return TRUE;
}