////////////////////////////////////////////////////////////
// SETMETADATA.C Copyright K.kawakami
////////////////////////////////////////////////////////////
#include "imap4.h"

#ifdef ADD_METADATA
BOOL SETMETADATADispatch(PCLIENT_CONTEXT lpClientContext) {
   
  PImap4Context pContext = &lpClientContext->Context;

  sprintf(pContext->mMessages, "%s %s %s completed\r\n", pContext->pSquence, IMAP4_GOOD_RESPONSE, pContext->pCmd );

  return TRUE;
}
#endif