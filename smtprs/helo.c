////////////////////////////////////////////////////////////
// Helo.c Copyright K.kawakami
////////////////////////////////////////////////////////////
#include "smtp.h"

#ifdef MILTER_ON // MILTERインターフェースを追加。
extern BOOL   bMILTER;         // MILTER機能拡張
BOOL MLT_HELO(PCLIENT_CONTEXT lpClientContext);
#endif

#ifdef BTHREAD
BOOL HeloDispatch(PCLIENT_CONTEXT lpClientContext)
#else
SMTPRSDisposition HeloDispatch(
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
#ifdef UPDATE_20130304 // HELO/EHLOのトークンが取得されなくなった不具合
    pContext->nRCPTCount = 0;     // 送信先を全てクリア。RFC821規定
    _unlink(pContext->mFnRset);
    _unlink(pContext->mFnHead);
	if (pContext->State > SmtpNegotiate) {
	  if (pContext->State == SmtpMailFrom || pContext->State == SmtpRecpient) {
	    pContext->bRCPTReset = TRUE;  // RCPT->REST後フラグセット
	  }
	}
    pContext->State = SmtpNegotiate;
#else
#ifdef UPDATE_20121206 // EHLOが重複した場合はRSET処理が行われる用に修正
    RsetDispatch(lpClientContext);
#endif
#endif
	if (pContext->State == SmtpNegotiate) {
	  pContext->PeerHelo[0] = '\x0';
      pContext->State = SmtpGreeting;
	  sprintf(pContext->mMessages, SMTP_GOOD_HELO, pContext->LocalName, pContext->PeerAddr);
	  pContext->p = strstr(pContext->mToken, " ");
	  if (pContext->p) {
		pContext->p++;
		strtok(pContext->p,"\r\n ");
		if (*pContext->p == '\r' || *pContext->p == '\n' || *pContext->p == ' ')
		  *pContext->p = '\x0';
		strncpy(pContext->PeerHelo, pContext->p, sizeof(pContext->PeerHelo)-1);
		pContext->PeerHelo[sizeof(pContext->PeerHelo)-1] = '\x0';
		//if (strlen(pContext->PeerHelo)) {
          //sprintf(pContext->mMessages, SMTP_GOOD_HELO, pContext->LocalName, pContext->PeerAddr);
		//}
	  }
#ifdef MILTER_ON // MILTERインターフェースを追加。
	  if (bMILTER)
	  {
	    if (!MLT_HELO(lpClientContext))
		{
          pContext->State = SmtpNegotiate;
		}
	  }
#endif
	} else {
      sprintf(pContext->mMessages, SMTP_HELO_DUPLICATE, pContext->LocalName);
	}
    //*SendHandle = NULL;
    pContext->mToken[0] = '\x0';
#ifdef BTHREAD
    return TRUE;
#else
    *OutputBuffer = pContext->mMessages;
    *OutputBufferLen = strlen(pContext->mMessages);
    return(SMTPRS_SendBuffer);
#endif
}
