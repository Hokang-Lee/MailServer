////////////////////////////////////////////////////////////
// Stls.C STARTTLS Copyright K.kawakami
////////////////////////////////////////////////////////////
#include "imap4.h"

BOOL StlsDispatch(PCLIENT_CONTEXT lpClientContext) {
  PImap4Context pContext = &lpClientContext->Context;

  if (pContext->State != Imap4Negotiate || pContext->bUsedSSL) 
  {
    sprintf(pContext->mMessages, "%s %s\r\n", pContext->pSquence, IMAP4_BAD_STARTTLS, pContext->pCmd);
  } else {
    sprintf(pContext->mMessages, "%s %s\r\n",pContext->pSquence, IMAP4_OK_STARTTLS, pContext->pCmd);
  } 

  return TRUE;
}