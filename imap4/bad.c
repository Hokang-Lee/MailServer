////////////////////////////////////////////////////////////
// BAD.C Copyright K.kawakami
////////////////////////////////////////////////////////////
#include "imap4.h"

BOOL BADDispatch(PCLIENT_CONTEXT lpClientContext) {
  
  PImap4Context pContext = &lpClientContext->Context;

  if (pContext->pToken) { // オプション指定
    sprintf(pContext->mMessages, "%s %s unkown command.(%s)\r\n",pContext->pSquence, IMAP4_BADTOKEN_RESPONSE, (pContext->pCmd ? _strupr(pContext->pCmd) : "NULL"));
  } else {
    sprintf(pContext->mMessages, "%s %s illegal argument.(%s)\r\n", pContext->pSquence, IMAP4_BADTOKEN_RESPONSE, (pContext->pCmd ? _strupr(pContext->pCmd) : "NULL"));
  }
// BAD Command line too long
  return TRUE;
}