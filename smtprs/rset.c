////////////////////////////////////////////////////////////
// Rset.c Copyright K.kawakami
////////////////////////////////////////////////////////////
#include "smtp.h"

#ifdef BTHREAD
BOOL RsetDispatch(PCLIENT_CONTEXT lpClientContext) 
#else
SMTPRSDisposition RsetDispatch(
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

    // Put us into Authorization state
	if (pContext->State > SmtpNegotiate) {
	  if (pContext->State == SmtpMailFrom) {
		if (pContext->mFnHead[0] != '\x0')
          _unlink(pContext->mFnHead);
        pContext->State--;            // ステータスを１つ前に戻す
		pContext->bRCPTReset = TRUE;  // RCPT->REST後フラグセット
	  } else if (pContext->State == SmtpRecpient) {
        //pContext->nRCPTCount--;
		//if (pContext->nRCPTCount == 0)
          //pContext->State--;
		// 2001.11.13 /////////////////////////////////////////
		pContext->nRCPTCount = 0;     // 送信先を全てクリア。RFC821規定
        pContext->State--;            // ステータスを１つ前に戻す
		pContext->bRCPTReset = TRUE;  // RCPT->REST後フラグセット
		///////////////////////////////////////////////////////
		if (pContext->mFnRset[0] != '\x0' && pContext->mFnHead[0] != '\x0') {
 	      pContext->Rsetfp = fopen(pContext->mFnRset, "wt");
          pContext->Rcptfp = fopen(pContext->mFnHead, "rt");
          if (pContext->Rcptfp && pContext->Rsetfp) {
#ifdef DATA_CASH
            setvbuf( pContext->Rsetfp, pContext->mFwbuf, _IOFBF, sizeof(pContext->mFwbuf) );
            setvbuf( pContext->Rcptfp, pContext->mFrbuf, _IOFBF, sizeof(pContext->mFrbuf) );
#endif
		    //for (pContext->i = 0; pContext->i < pContext->nRCPTCount+2;  pContext->i++) {
		    for (pContext->i = 0; pContext->i < pContext->nRCPTCount+1;  pContext->i++) {
		      fgets(pContext->mToken, sizeof(pContext->mToken), pContext->Rcptfp);
		      fputs(pContext->mToken, pContext->Rsetfp);
			}
		    fclose(pContext->Rsetfp);
		    fclose(pContext->Rcptfp);
		    _unlink(pContext->mFnHead);
		    rename(pContext->mFnRset, pContext->mFnHead);
		  } else {
		    if (pContext->Rsetfp)
		      fclose(pContext->Rsetfp);
		    if (pContext->Rcptfp)
 		      fclose(pContext->Rcptfp);
		  }
		}
#ifdef ESMTP_AUTH
	  } if (pContext->State == SmtpGreeting && 
		    pContext->nAUTHMODE != 0) {
        pContext->State = SmtpEGreeting;
	  } if (pContext->State == SmtpGreeting ||
		    pContext->State == SmtpEGreeting) { //||
		    //pContext->State == SmtpAuthentication) {
        pContext->State = SmtpNegotiate;
#endif
	  } else
        pContext->State--;     // ステータスを１つ前に戻す
	}
#ifdef UPDATE_20071227 // RSETコマンドでRCPファイルを削除
    _unlink(pContext->mFnHead);
#endif
#ifdef UPDATE_20230124 // RSETコマンドでRSTファイルを削除
    _unlink(pContext->mFnRset);
#endif
    // No file, but copy the banner string and return it.
    //*SendHandle = NULL;
    sprintf(pContext->mMessages, SMTP_GOOD_RSET);
    pContext->mToken[0] = '\x0';
#ifdef BTHREAD
    return TRUE;
#else
    *OutputBuffer = pContext->mMessages;
    *OutputBufferLen = strlen(pContext->mMessages);
    return(SMTPRS_SendBuffer);
#endif
}
