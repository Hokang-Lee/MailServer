////////////////////////////////////////////////////////////
// LOGOUT.C Copyright K.kawakami
////////////////////////////////////////////////////////////
#include "imap4.h"

#ifdef V4
extern BOOL    bHide;
#endif

void DeleteMessage(PCLIENT_CONTEXT lpClientContext, BOOL bRes);

BOOL LOGOUTDispatch(PCLIENT_CONTEXT lpClientContext) {
   
  PImap4Context pContext = &lpClientContext->Context;

  //if (pContext->State == Imap4SelectFolder)
   // DeleteMessage(lpClientContext, FALSE); // メッセージ削除
#ifdef V4
  if (bHide)
    strcpy(pContext->mMessages, "* BYE terminating connection\r\n");
  else
#endif
  sprintf(pContext->mMessages, "* BYE %s terminating connection\r\n", IMAP4_NAME);
  put_reply(lpClientContext, TRUE, TRUE);
  sprintf(pContext->mMessages, "%s %s %s completed\r\n", pContext->pSquence, IMAP4_GOOD_RESPONSE, pContext->pCmd );
  pContext->State = Imap4Negotiate; // 非認証状態に戻す
 
  return FALSE;
}