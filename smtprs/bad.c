////////////////////////////////////////////////////////////
// Bad.c Copyright K.kawakami
////////////////////////////////////////////////////////////
#include "smtp.h"
extern int nport;
extern char mdomain[];

#ifdef BTHREAD
BOOL BadDispatch(PCLIENT_CONTEXT lpClientContext)
#else
SMTPRSDisposition BadDispatch(
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
	char mwk[81];
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

    // Put us into Authorization state
    //pContext->State = SmtpAuthorization;
    // No file, but copy the banner string and return it.
    //*SendHandle = NULL;
	strtok(pContext->mToken,"\r\n");
	if (pContext->mToken[0] == '\r' || pContext->mToken[0] == '\n')
	  pContext->mToken[0] = '\x0';
	strncpy(mwk, pContext->mToken, sizeof(mwk));
	mwk[sizeof(mwk)-1] = '\x0';
	sprintf(pContext->mMessages, SMTP_UNKNOW_MESS, mwk);
    //sprintf(pContext->mMessages, SMTP_UNKNOW_MESS, pContext->mToken);
    pContext->mToken[0] = '\x0';
#ifdef BTHREAD
    return TRUE;
#else
    *OutputBuffer = pContext->mMessages;
    *OutputBufferLen = strlen(pContext->mMessages);
    return(SMTPRS_SendBuffer);
#endif
}
