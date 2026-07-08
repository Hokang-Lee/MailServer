////////////////////////////////////////////////////////////
// Vrfy.c Copyright K.kawakami
// メールアドレスからユーザーの出力命令
////////////////////////////////////////////////////////////
#include "smtp.h"

extern bVrfy;
#ifdef UPDATE_20070509 // エイリアス設定の実アドレスオプションを別データとして処理する
BOOL CheckUser(char *user, char *pOpt, char *myaddr, BOOL *bLocal, BOOL *bSubLocal, BOOL *bMList, BOOL *bOutSideAliases, LPBYTE fullname);
#else
BOOL CheckUser(char *user, char *myaddr, BOOL *bLocal, BOOL *bSubLocal, BOOL *bMList, BOOL *bOutSideAliases, LPBYTE fullname);
#endif

#ifdef BTHREAD
BOOL VrfyDispatch(PCLIENT_CONTEXT lpClientContext)
#else
SMTPRSDisposition VrfyDispatch(
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
    //pContext->State = SmtpAuthorization;
    // No file, but copy the banner string and return it.
    //*SendHandle = NULL;
	if (bVrfy) {
	  sprintf(pContext->mMessages, SMTP_UNKNOW_USER, ""); // ユーザーが存在しない場合
	} else {
#ifdef UPDATE_20071209B // VRFY,EXPN命令の非応答設定での応答コードを252に変更。(RFC2821に準拠)
      sprintf(pContext->mMessages, SMTP_HIDDEN_VRFY); //SMTP_BAD_ARGUMENT);
#else
      sprintf(pContext->mMessages, SMTP_ISNT_INPLEMENTED); //SMTP_BAD_ARGUMENT);
#endif
	}
	pContext->p = strstr(pContext->mToken, " ");
	if (pContext->p && bVrfy) {
	  strtok(pContext->p, "\r\n");
	  pContext->p++;
      if (strlen(pContext->p) > 0) {
        pContext->bDomainVRFY = FALSE;
	    strcpy(pContext->mUIDVRFY, pContext->p);
#ifdef UPDATE_20070509 // エイリアス設定の実アドレスオプションを別データとして処理する
        pContext->bVRFY = CheckUser( pContext->p,
			                         NULL,
			                         pContext->MyAddr,
			                        &pContext->bDomainVRFY,
									 NULL,
									 NULL,
									 NULL,
									 pContext->fullname);
#else
        pContext->bVRFY = CheckUser( pContext->p,
			                         pContext->MyAddr,
			                        &pContext->bDomainVRFY,
									 NULL,
									 NULL,
									 NULL,
									 pContext->fullname);
#endif
		//// エイリアスで書き換えられても元に戻す
	    strcpy(pContext->p, pContext->mUIDVRFY);
        /////////////////////////////////////////
        if (pContext->bVRFY && pContext->bDomainVRFY) {
#ifdef UPDATE_20071209A // VRFY命令でユーザ応答の形式の修正。
		  strtok(pContext->fullname, ">");
	      sprintf(pContext->mMessages, SMTP_GOOD_VRFY, &pContext->fullname[1], pContext->p); // ユーザーが存在する場合
#else
	      sprintf(pContext->mMessages, SMTP_GOOD_VRFY, pContext->p, pContext->fullname); // ユーザーが存在する場合
#endif
		} else {
		  sprintf(pContext->mMessages, SMTP_UNKNOW_USER, pContext->p); // ユーザーが存在しない場合
		}
	  }
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
