////////////////////////////////////////////////////////////
// Helo.c Copyright K.kawakami
////////////////////////////////////////////////////////////
#include "smtp.h"

#ifdef ESMTP_AUTH
extern DWORD   nMailInMaxSize;
extern char    mSMTPAUTHMODE[64];
#ifdef SENDERID
extern BOOL     bSenderID;
#endif
extern BOOL  bListenMode;
#ifdef USE_SSL
extern BOOL  bUsedSSL;
#ifdef USE_STARTTLS
extern BOOL    bUsedSTLS;
#endif
#endif

#ifdef EIGHT_BITMIME
extern BOOL   b8BITMIME;
#endif

#ifdef MILTER_ON // MILTERインターフェースを追加。
extern BOOL   bMILTER;         // MILTER機能拡張
BOOL MLT_HELO(PCLIENT_CONTEXT lpClientContext);
#endif

#ifdef BTHREAD
BOOL EhloDispatch(PCLIENT_CONTEXT lpClientContext)
#else
SMTPRSDisposition EhloDispatch(
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
#ifdef USE_STARTTLS
  BOOL bSTLSView = FALSE;
#endif
  CHAR mEhlo[80];
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

#ifdef UPDATE_20130304 // HELO/EHLOのトークンが所得されなくなった不具合
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
      pContext->State = SmtpEGreeting;
	  /////////////////////////////////////////////////////
	  //sprintf(pContext->mMessages, SMTP_GOOD_EHLO, pContext->LocalName, pContext->PeerAddr, mSMTPAUTHMODE);
	  sprintf(pContext->mMessages, SMTP_GOOD_EHLO, pContext->LocalName, pContext->PeerAddr);
#ifdef EIGHT_BITMIME
#ifdef USE_STARTTLS
#ifdef UPDATE_20151126 // リッスンＩＰ毎に証明書を選択できる対策
	  if (!lpClientContext->ssl &&
           (pContext->bUsedSTLS || 
             (bUsedSTLS &&
               (!(pContext->bUsedSSL ||
                 (!bListenMode && bUsedSSL))))))
#else
	  if (bUsedSTLS &&
          (!(pContext->bUsedSSL ||
             (!bListenMode && bUsedSSL))))
#endif
	  {
		bSTLSView = TRUE;
//        pContext->mMessages[3] = '-';
	  } else {
        bSTLSView = FALSE;
	  }
#ifdef SENDERID
	  if ( b8BITMIME || bSTLSView || nMailInMaxSize > 0 || (stricmp(mSMTPAUTHMODE, "no") != 0 && strlen(mSMTPAUTHMODE) > 0) || bSenderID)
        pContext->mMessages[3] = '-';
#else
	  if ( b8BITMIME || bSTLSView || nMailInMaxSize > 0 || (stricmp(mSMTPAUTHMODE, "no") != 0 && strlen(mSMTPAUTHMODE) > 0) )
        pContext->mMessages[3] = '-';
#endif
#else
#ifdef SENDERID
	  if ( b8BITMIME || nMailInMaxSize > 0 || (stricmp(mSMTPAUTHMODE, "no") != 0 && strlen(mSMTPAUTHMODE) > 0) || bSenderID)
        pContext->mMessages[3] = '-';
#else
	  if ( b8BITMIME || nMailInMaxSize > 0 || (stricmp(mSMTPAUTHMODE, "no") != 0 && strlen(mSMTPAUTHMODE) > 0) )
        pContext->mMessages[3] = '-';
#endif
#endif
#else
#ifdef USE_STARTTLS
#ifdef UPDATE_20151126 // リッスンＩＰ毎に証明書を選択できる対策
	  if (!lpClientContext->ssl &&
           (pContext->bUsedSTLS || 
             (bUsedSTLS &&
               (!(pContext->bUsedSSL ||
                 (!bListenMode && bUsedSSL))))))
#else
	  if (bUsedSTLS &&
          (!(pContext->bUsedSSL ||
             (!bListenMode && bUsedSSL))))
#endif
	  {
		bSTLSView = TRUE;
//        pContext->mMessages[3] = '-';
	  } else {
        bSTLSView = FALSE;
	  }
#ifdef SENDERID
	  if ( bSTLSView || nMailInMaxSize > 0 || (stricmp(mSMTPAUTHMODE, "no") != 0 && strlen(mSMTPAUTHMODE) > 0) || bSenderID)
        pContext->mMessages[3] = '-';
#else
	  if ( bSTLSView || nMailInMaxSize > 0 || (stricmp(mSMTPAUTHMODE, "no") != 0 && strlen(mSMTPAUTHMODE) > 0) )
        pContext->mMessages[3] = '-';
#endif
#else
#ifdef SENDERID
	  if ( nMailInMaxSize > 0 || (stricmp(mSMTPAUTHMODE, "no") != 0 && strlen(mSMTPAUTHMODE) > 0) || bSenderID)
        pContext->mMessages[3] = '-';
#else
	  if ( nMailInMaxSize > 0 || (stricmp(mSMTPAUTHMODE, "no") != 0 && strlen(mSMTPAUTHMODE) > 0) )
        pContext->mMessages[3] = '-';
#endif
#endif
#endif

#ifdef EIGHT_BITMIME
	  if ( b8BITMIME )
	  {
		strcpy(mEhlo, SMTP_GOOD_EHLO_8BITMIME);
#ifdef SENDERID
		if ( bSTLSView || nMailInMaxSize > 0 || (stricmp(mSMTPAUTHMODE, "no") != 0 && strlen(mSMTPAUTHMODE) > 0) || bSenderID)
		  mEhlo[3] = '-';
#else
		if ( bSTLSView || nMailInMaxSize > 0 || (stricmp(mSMTPAUTHMODE, "no") != 0 && strlen(mSMTPAUTHMODE) > 0) )
		  mEhlo[3] = '-';
#endif
		strcat(pContext->mMessages, mEhlo);
	  }
#endif

	  if (nMailInMaxSize != 0) {
		sprintf(mEhlo, SMTP_GOOD_EHLO_SIZE, nMailInMaxSize);
#ifdef USE_STARTTLS
#ifdef SENDERID
		if ( bSTLSView || (stricmp(mSMTPAUTHMODE, "no") != 0 && strlen(mSMTPAUTHMODE) > 0) || bSenderID)
		  mEhlo[3] = '-';
#else
		if ( bSTLSView || (stricmp(mSMTPAUTHMODE, "no") != 0 && strlen(mSMTPAUTHMODE) > 0) )
		  mEhlo[3] = '-';
#endif
#else
#ifdef SENDERID
		if (stricmp(mSMTPAUTHMODE, "no") != 0 && strlen(mSMTPAUTHMODE) > 0 || bSenderID)
		  mEhlo[3] = '-';
#else
		if (stricmp(mSMTPAUTHMODE, "no") != 0 && strlen(mSMTPAUTHMODE) > 0)
		  mEhlo[3] = '-';
#endif
#endif
		strcat(pContext->mMessages, mEhlo);
	  }
#ifdef USE_XCHATMAIL
	  strcpy(mEhlo, "250 X-CHATMAIL\r\n"); // X-CHATMAIL
	  if (stricmp(mSMTPAUTHMODE, "no") != 0 && strlen(mSMTPAUTHMODE) > 0)
	    mEhlo[3] = '-';
	  strcat(pContext->mMessages, mEhlo);
#endif
#ifdef SENDERID
	  if (bSenderID) {
		strcpy(mEhlo, SMTP_GOOD_EHLO_SUBMITTER);
#ifdef USE_STARTTLS
		if ( bSTLSView || (stricmp(mSMTPAUTHMODE, "no") != 0 && strlen(mSMTPAUTHMODE) > 0))
		  mEhlo[3] = '-';
#else
		if (stricmp(mSMTPAUTHMODE, "no") != 0 && strlen(mSMTPAUTHMODE) > 0)
		  mEhlo[3] = '-';
#endif
		strcat(pContext->mMessages, mEhlo);
	  }
#endif
#ifdef USE_STARTTLS
	  if (bSTLSView) // STARTTLSとAUTHは排他表示
	  {
		strcpy(mEhlo, SMTP_GOOD_EHLO_STLS); // STARTTLS
		if (stricmp(mSMTPAUTHMODE, "no") != 0 && strlen(mSMTPAUTHMODE) > 0)
		  mEhlo[3] = '-';
		strcat(pContext->mMessages, mEhlo);
        if (stricmp(mSMTPAUTHMODE, "no") != 0 && strlen(mSMTPAUTHMODE) > 0) 
		{
	      sprintf(mEhlo, SMTP_GOOD_EHLO_AUTH, mSMTPAUTHMODE);
	      strcat(pContext->mMessages, mEhlo);
		}
	  } else 
#endif
	  {
        if (stricmp(mSMTPAUTHMODE, "no") != 0 && strlen(mSMTPAUTHMODE) > 0) 
		{
	      sprintf(mEhlo, SMTP_GOOD_EHLO_AUTH, mSMTPAUTHMODE);
	      strcat(pContext->mMessages, mEhlo);
		}
	  }
	  /////////////////////////////////////////////////////
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
      sprintf(pContext->mMessages, SMTP_EHLO_DUPLICATE, pContext->LocalName);
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

#endif
