////////////////////////////////////////////////////////////
// Quit.c Copyright K.kawakami
////////////////////////////////////////////////////////////
#include "smtp.h"

#ifdef BTHREAD
BOOL QuitDispatch(PCLIENT_CONTEXT lpClientContext)
#else
SMTPRSDisposition QuitDispatch(
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
 	//printf("(%08x):QUIT phase start.\n", pContext);
    if (!pContext ||
         pContext->State == SmtpData ) {
 	  pContext->mToken[0] = '\x0';
   	  //printf("(%08x):QUIT phase exit(1).\n", pContext);
#ifdef BTHREAD
      return TRUE;
#else
      return(SMTPRS_Discard);
#endif
    }
	
  	if ( pContext->State == SmtpShutdown ) {
 	  //printf("(%08x):Duplicate QUIT\n", pContext);
      pContext->mToken[0] = '\x0';
   	  //printf("(%08x):QUIT phase exit(2).\n", pContext);
#ifdef BTHREAD
      return TRUE;
#else
      return(SMTPRS_Discard);
#endif
    }

	if (!(pContext->mToken[4] == '\r' && pContext->mToken[5] == '\n'||
		  pContext->mToken[4] == '\n')) {
      strtok(pContext->mToken, "\r\n\x08");
      sprintf(pContext->mMessages, SMTP_UNKNOW_MESS, pContext->mToken);
      pContext->mToken[0] = '\x0';
   	  //printf("(%08x):QUIT phase exit(3).\n", pContext);
#ifdef BTHREAD
      return TRUE;
#else
      *OutputBuffer = pContext->mMessages;
      *OutputBufferLen = strlen(pContext->mMessages);
      return(SMTPRS_SendBuffer);
#endif
	}

    // Put us into Authorization state
    pContext->State = SmtpShutdown;
    // No file, but copy the banner string and return it.
    sprintf(pContext->mMessages, SMTP_QUIT_MESS, pContext->LocalName);
    pContext->mToken[0] = '\x0';
    //printf("(%08x):QUIT phase exit(3).\n", pContext);
#ifdef BTHREAD
    return TRUE;
#else
    *OutputBuffer = pContext->mMessages;
    *OutputBufferLen = strlen(pContext->mMessages);
    return(SMTPRS_Quit);
#endif
}
