////////////////////////////////////////////////////////////
// NAMESPACE.C Copyright K.kawakami
////////////////////////////////////////////////////////////
#include "imap4.h"

BOOL NAMESPACEDispatch(PCLIENT_CONTEXT lpClientContext) {
  PImap4Context pContext = &lpClientContext->Context;
  CHAR mSH[32];

  if (pContext->State >= Imap4Authorization) {
	//strcpy(pContext->mMessages, "* NAMESPACE ((\"INBOX.\" \".\")) NIL NIL\r\n");
	//strcpy(pContext->mMessages, "* NAMESPACE ((\"\" \"/\")) NIL NIL\r\n");
	sprintf(mSH, "((\"%s\" \"/\"))", IMAP4_SHARED_NAME);
	strcpy(pContext->mMessages, "* NAMESPACE ((\"\" \"/\")) NIL ");
	strcat(pContext->mMessages, (pContext->mSharedRoot[0] ? mSH : "NIL"));
	strcat(pContext->mMessages, "\r\n");
    put_reply(lpClientContext, TRUE, TRUE);
    sprintf(pContext->mMessages, "%s %s %s completed\r\n", pContext->pSquence, IMAP4_GOOD_RESPONSE, pContext->pCmd );
  } else { // ó‘Ô‚ªˆá‚¤
    sprintf(pContext->mMessages, "%s %s Command unrecognized: %s\r\n", pContext->pSquence, IMAP4_RESULT_RESPONSE, pContext->pCmd );
  }
//* BAD Command line too long
  return TRUE;
}