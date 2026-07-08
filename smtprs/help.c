////////////////////////////////////////////////////////////
// Help.c Copyright K.kawakami
////////////////////////////////////////////////////////////
#include "smtp.h"

extern BOOL bDebug;
extern char mdomain[];

#ifdef BTHREAD
BOOL HelpDispatch(PCLIENT_CONTEXT lpClientContext)
#else
SMTPRSDisposition HelpDispatch(
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
 	//printf("(%08x):HELP phase start.\n", pContext);
    if (!pContext ||
        (pContext->State == SmtpData) ) {
 	    pContext->mToken[0] = '\x0';
 	   //printf("(%08x):HELP phase exit(1).\n", pContext);
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
	  //printf("(%08x):HELP phase exit(2).\n", pContext);
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
    sprintf(pContext->mMessages, SMTP_UNKNOW_HELP); //, mdomain);
    pContext->mToken[0] = '\x0';
	if (bDebug) printf("(%08x):HELP phase exit(3).\n", pContext);
#ifdef BTHREAD
    return TRUE;
#else
    *OutputBuffer = pContext->mMessages;
    *OutputBufferLen = strlen(pContext->mMessages);
    return(SMTPRS_SendBuffer);
#endif
}
