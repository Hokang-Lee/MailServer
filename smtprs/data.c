////////////////////////////////////////////////////////////
// Data.c Copyright K.kawakami
////////////////////////////////////////////////////////////
#include "smtp.h"
#include "version.h"

#ifdef K_SEARCH // K_SEARCH OEM 版
char    mMailQueueSubDir[256];
int     nQueueUnit; // 0:日単位 1:月単位 2:年単位
#endif
extern BOOL bDebug;

#ifdef Y2038_BUG
void gettime(SYSTEMTIME *ltime, char *mt);
#else
void gettime(time_t *ltime, char *mt);
#endif
void CarbonCopy(PSmtpContext pContext);
#ifdef V3
BOOL Make_Received_Header(PSmtpContext pContext);
#endif

#ifdef BTHREAD
BOOL DataDispatch(PCLIENT_CONTEXT lpClientContext)
#else
SMTPRSDisposition DataDispatch(
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
#ifdef UPDATE_20121001 // DATA<WSP><CRLF>を許可するようにした。
  int i;
  BOOL bPass = TRUE;
#endif
    // Verify context, and that the context says we can receive this command
    if (!pContext ||
        (pContext->State == SmtpData) ) {
 	    pContext->mToken[0] = '\x0';
#ifdef BTHREAD
       return FALSE;
#else
       return(SMTPRS_Discard);
#endif
    }

	if (strlen(pContext->mToken) != 6) {
      //strtok(pContext->mToken, "\r\n\x08");
#ifdef UPDATE_20121001 // DATA命令の続きに半角空白+<CRLF>の場合も命令として許可するようにした。
	  for (i = 4; i < strlen(pContext->mToken); i++)
	  {
		if (pContext->mToken[i] != ' ' &&
			//pContext->mToken[i] != '\t' &&
			pContext->mToken[i] != '\r' &&
			pContext->mToken[i] != '\n')
		{
		  bPass = FALSE;
		}
	  }
	  if (!bPass)
#endif
	  {
        sprintf(pContext->mMessages, SMTP_UNKNOW_MESS, pContext->mToken);
        pContext->mToken[0] = '\x0';
#ifdef BTHREAD
        return FALSE;
#else
        *OutputBuffer = pContext->mMessages;
        *OutputBufferLen = strlen(pContext->mMessages);
        return(SMTPRS_SendBuffer);
#endif
	  }
	}
	if (pContext->State >= SmtpRecpient && 
		pContext->State != SmtpDataError &&
		pContext->mFnData[0] != '\x0') {
#ifndef TEST_MODE
 	  if (pContext->State != SmtpData) {
#ifdef UPDATE_20060110 // 作成ファイルがクローズしてもクローズされないのでフラッシュしてみる
        pContext->Datafp = fopen(pContext->mFnData, "w+b");
#else
        pContext->Datafp = fopen(pContext->mFnData, "wb");
#endif
	  } else {
#ifdef UPDATE_20060110 // 作成ファイルがクローズしてもクローズされないのでフラッシュしてみる
	    pContext->Datafp = fopen(pContext->mFnData, "a+b");
#else
	    pContext->Datafp = fopen(pContext->mFnData, "ab");
#endif
	  }
      // Put us into Authorization state
      pContext->State = SmtpData;
	  memset(pContext->mEnd,0,6);
	  pContext->nTotalData = 0;
	  pContext->bSave = TRUE; // 2002.05.16
      if (pContext->Datafp) {
#ifndef K_SEARCH // K_SEARCH OEM 版
#ifndef E_POST
	    if (bDebug) printf("Add Received: header (%s %s)\n", ESMTP_NAME, VERSION);
#endif
#ifdef V3
#ifdef UPDATE_20220603 // 接続時のTLSバージョンとChiperの表記を追加した。
        pContext->mTLSInfo[0] = '\x0';
        if (lpClientContext->ssl)
		{
	      sprintf(pContext->mTLSInfo, "(version=%s,cipher=%s)", SSL_get_version(lpClientContext->ssl), SSL_get_cipher(lpClientContext->ssl));
	    }
#endif
		if (!Make_Received_Header(pContext)) {
#endif
	      sprintf(pContext->mdata,"Received: from %s (unverified [%s]) by %s\r\n", pContext->PeerHelo, pContext->PeerAddr, pContext->LocalName);
	      fputs(pContext->mdata, pContext->Datafp);
#ifdef ESMTP_AUTH
#ifdef K_SEARCH // K_SEARCH OEM 版
#ifdef UPDATE_20220603 // 接続時のTLSバージョンとChiperの表記を追加した。
          if (lpClientContext->ssl)
	        sprintf(pContext->mdata,"\t(%s %s) with ESMTP\r\n\t%s\r\n\tid <%s@%s>;\r\n",ESMTP_NAME, VERSION, pContext->mTLSInfo, pContext->mMsgId, pContext->LocalName);
          else
	        sprintf(pContext->mdata,"\t(%s %s) with ESMTP id <%s@%s>;\r\n",ESMTP_NAME, VERSION, pContext->mMsgId, pContext->LocalName);
#else
	      sprintf(pContext->mdata,"\t(%s %s) with ESMTP id <%s@%s>;\r\n",ESMTP_NAME, VERSION, pContext->mMsgId, pContext->LocalName);
#endif
#else
#ifdef UPDATE_20071204  // メッセージＩＤ採番処理を修正(Bym10id)
#ifdef UPDATE_20220603 // 接続時のTLSバージョンとChiperの表記を追加した。
          if (lpClientContext->ssl)
	        sprintf(pContext->mdata,"\t(%s %s) with ESMTP\r\n\t%s\r\n\tid <%s@%s>;\r\n",ESMTP_NAME, VERSION, pContext->mTLSInfo, pContext->mMsgId, pContext->LocalName);
          else
	        sprintf(pContext->mdata,"\t(%s %s) with ESMTP id <%s@%s>;\r\n",ESMTP_NAME, VERSION, pContext->mMsgId, pContext->LocalName);
#endif
#else
#ifdef UPDATE_20220603 // 接続時のTLSバージョンとChiperの表記を追加した。
          if (lpClientContext->ssl)
	        sprintf(pContext->mdata,"\t(%s %s) with ESMTP\r\n\t%s\r\n\tid <B%010lu@%s>;\r\n",ESMTP_NAME, VERSION, pContext->mTLSInfo, pContext->nMsgId, pContext->LocalName);
		  else
	        sprintf(pContext->mdata,"\t(%s %s) with ESMTP id <B%010lu@%s>;\r\n",ESMTP_NAME, VERSION, pContext->nMsgId, pContext->LocalName);
#else
	      sprintf(pContext->mdata,"\t(%s %s) with ESMTP id <B%010lu@%s>;\r\n",ESMTP_NAME, VERSION, pContext->nMsgId, pContext->LocalName);
#endif
#endif
#endif
#else
#ifdef K_SEARCH // K_SEARCH OEM 版
#ifdef UPDATE_20220603 // 接続時のTLSバージョンとChiperの表記を追加した。
          if (lpClientContext->ssl)
	        sprintf(pContext->mdata,"\t(%s %s) with SMTP\r\n\t%s\r\n\tid <%s@%s>;\r\n",ESMTP_NAME, VERSION, pContext->mTLSInfo, pContext->mMsgId, pContext->LocalName);
		  else
	        sprintf(pContext->mdata,"\t(%s %s) with SMTP id <%s@%s>;\r\n",ESMTP_NAME, VERSION, pContext->mMsgId, pContext->LocalName);
#else
	      sprintf(pContext->mdata,"\t(%s %s) with SMTP id <%s@%s>;\r\n",ESMTP_NAME, VERSION, pContext->mMsgId, pContext->LocalName);
#endif
#else
#ifdef UPDATE_20071204  // メッセージＩＤ採番処理を修正(Bym10id)
#ifdef UPDATE_20220603 // 接続時のTLSバージョンとChiperの表記を追加した。
          if (lpClientContext->ssl)
	        sprintf(pContext->mdata,"\t(%s %s) with SMTP\r\n\t%s\r\n\tid <%s@%s>;\r\n",ESMTP_NAME, VERSION, pContext->mTLSInfo, pContext->mMsgId, pContext->LocalName);
		  else
	        sprintf(pContext->mdata,"\t(%s %s) with SMTP id <%s@%s>;\r\n",ESMTP_NAME, VERSION, pContext->mMsgId, pContext->LocalName);
#else
	      sprintf(pContext->mdata,"\t(%s %s) with SMTP id <%s@%s>;\r\n",ESMTP_NAME, VERSION, pContext->mMsgId, pContext->LocalName);
#endif
#else
#ifdef UPDATE_20220603 // 接続時のTLSバージョンとChiperの表記を追加した。
          if (lpClientContext->ssl)
	        sprintf(pContext->mdata,"\t(%s %s) with SMTP\r\n\t%s\r\n\tid <B%010lu@%s>;\r\n",ESMTP_NAME, VERSION, pContext->mTLSInfo, pContext->nMsgId, pContext->LocalName);
		  else
	        sprintf(pContext->mdata,"\t(%s %s) with SMTP id <B%010lu@%s>;\r\n",ESMTP_NAME, VERSION, pContext->nMsgId, pContext->LocalName);
#else
	      sprintf(pContext->mdata,"\t(%s %s) with SMTP id <B%010lu@%s>;\r\n",ESMTP_NAME, VERSION, pContext->nMsgId, pContext->LocalName);
#endif
#endif
#endif
#endif
	      fputs(pContext->mdata, pContext->Datafp);
  	      gettime(&pContext->ltime, pContext->mtime);
	      sprintf(pContext->mdata,"\t%s\r\n", pContext->mtime);
	      fputs(pContext->mdata, pContext->Datafp);
#ifdef V3
		}
#endif
#ifdef UPDATE_20060110 // 作成ファイルがクローズしてもクローズされないのでフラッシュしてみる
	    fflush(pContext->Datafp);
#endif
#endif
        fclose(pContext->Datafp);
	  }
      // No file, but copy the banner string and return it.
      //*SendHandle = NULL;
      CarbonCopy(pContext);
#else // TEST_MODE
      pContext->State = SmtpData;
	  memset(pContext->mEnd,0,6);
	  pContext->nTotalData = 0;
	  pContext->bSave = TRUE; // 2002.05.16
#endif
#ifdef K_SEARCH // K_SEARCH OEM 版
      sprintf(pContext->mMessages, SMTP_DATA_MESS2, pContext->mMsgId, pContext->LocalName);
#else
#ifdef UPDATE_20071204  // メッセージＩＤ採番処理を修正(Bym10id)
      sprintf(pContext->mMessages, SMTP_DATA_MESS2, pContext->mMsgId, pContext->LocalName);
#else
      sprintf(pContext->mMessages, SMTP_DATA_MESS, pContext->nMsgId);
#endif
#endif
      pContext->mToken[0] = '\x0';
	} else {
      pContext->State = SmtpDataError;
      sprintf(pContext->mMessages, SMTP_NEED_COMMAND, "RCPT (recipient)");
      pContext->mToken[0] = '\x0';
#ifdef BTHREAD
      return FALSE;
#endif
	}

	pContext->mEnd[0] = '\x0';
#ifdef BTHREAD
    put_reply(lpClientContext, TRUE);
    return TRUE;
#else
    *OutputBuffer = pContext->mMessages;
    *OutputBufferLen = strlen(pContext->mMessages);
    return(SMTPRS_SendBuffer);
#endif
}
