////////////////////////////////////////////////////////////
// Stls.c STARTTLS Copyright K.kawakami
////////////////////////////////////////////////////////////
#include "smtp.h"

#ifdef BTHREAD
BOOL StlsDispatch(PCLIENT_CONTEXT lpClientContext)
#else
SMTPRSDisposition StlsDispatch(
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
    
    if (!pContext ||
        (pContext->State == SmtpData) ) {
 	    pContext->mToken[0] = '\x0';
#ifdef BTHREAD
       return TRUE;
#else
       return(SMTPRS_Discard);
#endif
    }

	if (pContext->State == SmtpEGreeting && !pContext->bUsedSSL) 
	{
      pContext->State = SmtpNegotiate;
	  sprintf(pContext->mMessages, SMTP_GOOD_STLS, pContext->LocalName, pContext->PeerAddr);
	} else {
      sprintf(pContext->mMessages, SMTP_ERROR_STLS, pContext->LocalName);
	}
    
    pContext->mToken[0] = '\x0';
#ifdef BTHREAD
    return TRUE;
#else
    *OutputBuffer = pContext->mMessages;
    *OutputBufferLen = strlen(pContext->mMessages);
    return(SMTPRS_SendBuffer);
#endif
}
