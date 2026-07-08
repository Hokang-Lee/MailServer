////////////////////////////////////////////////////////////
// Noop.c Copyright K.kawakami
////////////////////////////////////////////////////////////
#include "smtp.h"

#ifdef BTHREAD
BOOL NoopDispatch(PCLIENT_CONTEXT lpClientContext)
#else
SMTPRSDisposition NoopDispatch(
    PSmtpContext pContext,
    PUCHAR      InputBuffer,
    DWORD       InputBufferLen,
    PHANDLE     SendHandle,
    PUCHAR *    OutputBuffer,
    PDWORD      OutputBufferLen)
#endif
{
#ifdef BTHREAD
  PSmtpContext pContext = &lpClientContext->Context;
#endif

    // Verify context, and that the context says we can receive this command
    if (!pContext ||
        (pContext->State == SmtpData) ) {
 	    pContext->mToken[0] = '\x0';
#ifdef BTHREAD
       return TRUE;
#else
       return(SMTPRS_Discard);
#endif
    }

	if (strlen(pContext->mToken) != 6) {
      strtok(pContext->mToken, "\r\n\x08");
      sprintf(pContext->mMessages, SMTP_UNKNOW_MESS, pContext->mToken);
      pContext->mToken[0] = '\x0';
#ifdef BTHREAD
      return TRUE;
#else
      *OutputBuffer = pContext->mMessages;
      *OutputBufferLen = strlen(pContext->mMessages);
      return(SMTPRS_SendBuffer);
#endif
	}
    // Put us into Authorization state
    //pContext->State = SmtpAuthorization;
    // No file, but copy the banner string and return it.
    //*SendHandle = NULL;
    sprintf(pContext->mMessages, SMTP_GOOD_MESS, "");
    pContext->mToken[0] = '\x0';
#ifdef BTHREAD
    return TRUE;
#else
    *OutputBuffer = pContext->mMessages;
    *OutputBufferLen = strlen(pContext->mMessages);
    return(SMTPRS_SendBuffer);
#endif
}
