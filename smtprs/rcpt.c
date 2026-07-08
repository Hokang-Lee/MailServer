////////////////////////////////////////////////////////////
// Rcpt.c Copyright K.kawakami
////////////////////////////////////////////////////////////
#include "smtp.h"

extern BOOL bDebug;
extern char  mLocalDomain[];
extern DWORD nLocalDomain;
extern DWORD nRCPTMaximum;
#ifdef LGWAN_ON        // LGWAN機能拡張
extern BOOL   bLGWAN;
#endif
#ifdef UPDATE_20150324 // エンベロープの送信先をファイルへの書込みエラーが発生した場合、リトライを最大５回行う
extern int    nRcptWRetry; // エンベロープの送信先のファイル書込みエラー時のリトライ回数
#endif
#ifdef UPDATE_20161111 /// 外部からの送信元送信先制限(逆上長承認)
extern BOOL   bMailApprovalRevers;
#endif

#ifdef UPDATE_20070509 // エイリアス設定の実アドレスオプションを別データとして処理する
BOOL CheckUser(char *user, char *pOpt, char *myaddr, BOOL *bLocal, BOOL *bSubLocal, BOOL *bMList, BOOL *bOutSideAliases, LPBYTE fullname);
#else
BOOL CheckUser(char *user, char *myaddr, BOOL *bLocal, BOOL *bSubLocal, BOOL *bMList, BOOL *bOutSideAliases, LPBYTE fullname);
#endif
void GetListsPass(char *mRcpt, char *mMlPass);
void GetBaseDirectory(CHAR *BaseDir, char *mMBdir, char *muser, char *myaddr);
#ifdef MILTER_ON // MILTERインターフェースを追加。
extern BOOL   bMILTER;         // MILTER機能拡張
BOOL MLT_TO(PCLIENT_CONTEXT lpClientContext);
#endif
#ifdef UPDATE_20150319 // エンベロープの送信元によりメール受信の許可をする場合
BOOL Effective_Address(PSmtpContext pContext);
#endif

#ifdef BTHREAD
BOOL RcptDispatch(PCLIENT_CONTEXT lpClientContext)
#else
SMTPRSDisposition RcptDispatch(
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
	BOOL b[3];
	CHAR mMLPass[256] = {'\x0'};
#ifdef UPDATE_20050924  // 送信元ユーザ別送信先制限
#ifdef UPDATE_20071205 //RFC2821: メールアドレス長が255Byteのときの対策
    CHAR mUser[512];
	CHAR Bdir[512];
#else
    CHAR mUser[256];
	CHAR Bdir[MAX_PATH];
#endif
#endif
	int  sts;
#ifdef UPDATE_20060718 // エイリアスアカウントと実アカウントのドメインが異なる場合は、実アカウントに変換する。
	char *pA, *pM;
	BOOL bDiffAliases = FALSE;
#endif
#ifdef UPDATE_20060718 // エイリアスアカウントと実アカウントのドメインが異なる場合は、実アカウントに変換する。
	CHAR mOption[256];
#endif
#ifdef UPDATE_20070614 // RCPT TO:で最初に一致した上長承認条件設定を優先
	DWORD  nOption;             // 承認条件 0:不要,1:全て,2:添付があるとき
	CHAR   mBOSSSubject[_MAX_PATH*2];  // 上長アドレス
	CHAR   mBOSS[_MAX_PATH*4];  // 上長アドレス
#endif
#ifdef UPDATE_20150324 // エンベロープの送信先をファイルへの書込みエラーが発生した場合、リトライを最大５回行う
	int nRetry;
#endif

#ifdef UPDATE_20080617 // 上長承認機能で添付ファイルの拡張子による承認条件追加
	pContext->mMIME[0] = '\x0';
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
    ///////////////////////////////////////////////
    ///////////////////////////////////////////////
#ifdef UPDATE_20060615 // データのディスク書込に異常が有った場合の対策
	if (!pContext->bDiskStatus) { // デスクへの書込み異常があるなら
      sprintf(pContext->mMessages, SMTP_DEVICE_FULL);
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
    //*SendHandle = NULL;
    if (_strnicmp((PCHAR) &pContext->mToken[4], " TO:", 4) != 0 ){
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
	if (pContext->State < SmtpMailFrom) {
      sprintf(pContext->mMessages, SMTP_RCPT_NEED);
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
    // No file, but copy the banner string and return it.
    strtok(pContext->mToken, "\r\n\x08");
	pContext->p = strstr(pContext->mToken, "<");
	if (!pContext->p)
	  pContext->p = strstr(&pContext->mToken[8], " ");
	if (!pContext->p)
	  pContext->p = strstr(pContext->mToken, ":");
	if (pContext->p) {
	  strtok(pContext->p, ">\r\n");
	  if (*pContext->p == '<')
        pContext->p++;
	  else
        while(*pContext->p == ' ' || *pContext->p == ':') // ホワイトスペース削除
	      pContext->p++;
#ifdef UPDATE_20070614 // RCPT TO:で最初に一致した上長承認条件設定を優先
	  nOption = 0;
	  mBOSSSubject[0] = '\x0';
	  mBOSS[0] = '\x0';
#else
#ifdef UPDATE_20070516 // 上長承認機能の追加
	  pContext->nOption = 0;
	  pContext->mBOSSSubject[0] = '\x0';
	  pContext->mBOSS[0] = '\x0';
#endif
#endif

#ifdef UPDATE_20071210 // エンベロープの送信元が255Byte(RFC2821規定)を超えた場合、拒否する。
	  if (strlen(pContext->p) > 255) {
        sprintf(pContext->mMessages, SMTP_LONG_ADDRESS, pContext->p);
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

#ifdef UPDATE_20150319 // エンベロープの送信元によりメール受信の許可をする場合
	  if (!Effective_Address(pContext)) 
	  {
		if(!pContext->bebp)
		{
          sprintf(pContext->mMessages, SMTP_BAD_IP, pContext->p);
          pContext->mToken[0] = '\x0';
#ifdef BTHREAD
          return TRUE;
#else
          *OutputBuffer = pContext->mMessages;
          *OutputBufferLen = strlen(pContext->mMessages);
          return(SMTPRS_SendBuffer);
#endif
		}
	  }
#endif


#ifndef K_SEARCH // K_SEARCH OEM 版 //KTEC SMTP Archive Receiver
      if (strlen(pContext->p) > 0)
#endif
	  {
#ifdef UPDATE_20050924  // 送信元ユーザ別送信先制限
		//////////////////////////////////////
		///// Checked Sender permit List /////
		///// pContext->mUIDFROM 送信元エンベロープ
        ///// pContext->p        送信先
		if (!strchr(pContext->mUIDFROM, '@') ||
			CheckDomain(pContext->mUIDFROM)) { // 内部ドメインのユーザのみチェック
#ifdef UPDATE_20070614 // RCPT TO:で最初に一致した上長承認条件設定を優先
#ifdef UPDATE_20080617 // 上長承認機能で添付ファイルの拡張子による承認条件追加
#ifdef UPDATE_20161111 /// 外部からの送信元送信先制限(逆上長承認)
	      if (!GetSenderPermitFile(pContext->mUIDFROM, pContext->p, &nOption, pContext->mMIME, mBOSSSubject, mBOSS, TRUE))
#else
	      if (!GetSenderPermitFile(pContext->mUIDFROM, pContext->p, &nOption, pContext->mMIME, mBOSSSubject, mBOSS))
#endif
#else
	      if (!GetSenderPermitFile(pContext->mUIDFROM, pContext->p, &nOption, mBOSSSubject, mBOSS))
#endif
#else
#ifdef UPDATE_20070516 // 上長承認機能の追加
#ifdef UPDATE_20070607 // 上長承認機能にSubject:キーワードも可能にする
	      if (!GetSenderPermitFile(pContext->mUIDFROM, pContext->p, &pContext->nOption, pContext->mBOSSSubject, pContext->mBOSS))
#else
	      if (!GetSenderPermitFile(pContext->mUIDFROM, pContext->p, &pContext->nOption, pContext->mBOSS))
#endif
#else
	      if (!GetSenderPermitFile(pContext->mUIDFROM, pContext->p))
#endif
#endif
		  { // 許可されていない送り先の場合
#ifdef UPDATE_20200131 // セキュアハンドラのブラックリストで拒絶した場合のコードを追加
            sprintf(pContext->mMessages, SMTP_BAD_FROM_APPROVAL, pContext->mUIDFROM);
#else
            sprintf(pContext->mMessages, SMTP_BAD_RCPT);
#endif
            pContext->mToken[0] = '\x0';
#ifdef BTHREAD
            return TRUE;
#else
            *OutputBuffer = pContext->mMessages;
            *OutputBufferLen = strlen(pContext->mMessages);
            return(SMTPRS_SendBuffer);
#endif
		  }
		}
#endif	

#ifdef UPDATE_20161111 /// 外部からの送信元送信先制限(逆上長承認)
		if (bMailApprovalRevers)
		{
		  if (pContext->mUIDFROM[0] =='\x0' ||
			  strchr(pContext->mUIDFROM, '@') && !CheckDomain(pContext->mUIDFROM)) { // 外部ドメインのユーザのみチェック
		    if (!GetSenderPermitFile(pContext->mUIDFROM, pContext->p, &nOption, pContext->mMIME, mBOSSSubject, mBOSS, FALSE))
			{ // 許可されていない送り先の場合
#ifdef UPDATE_20200131 // セキュアハンドラのブラックリストで拒絶した場合のコードを追加
              sprintf(pContext->mMessages, SMTP_BAD_FROM_APPROVAL, pContext->mUIDFROM);
#else
              sprintf(pContext->mMessages, SMTP_BAD_RCPT);
#endif
              pContext->mToken[0] = '\x0';
#ifdef BTHREAD
              return TRUE;
#else
              *OutputBuffer = pContext->mMessages;
              *OutputBufferLen = strlen(pContext->mMessages);
              return(SMTPRS_SendBuffer);
#endif
			}
		  }
		}
#endif

#ifdef UPDATE_20070614 // RCPT TO:で最初に一致した上長承認条件設定を優先
		if (pContext->bTopRcpto && nOption >= 1) 
		{ // RCPT TO:で最初に一致した上長承認条件設定を優先
          pContext->bTopRcpto = FALSE; // 最初の処理完了
	      pContext->nOption = nOption;
	      strcpy(pContext->mBOSSSubject, mBOSSSubject);
	      strcpy(pContext->mBOSS, mBOSS);
		}
#endif
		//////////////////////////////////
		///// Checked Aileases and   /////
		/////  sender Local Domain ? /////
#ifdef UPDATE_20060627 // メーリングリストアドレス判断のリセット
		pContext->bMList = FALSE;
#endif
	    pContext->bRCPT = pContext->bDomainRCPT = pContext->bSubDomainRCPT = FALSE;
        strncpy(pContext->muid, pContext->p, _MAX_PATH-1);
		if (!pContext->bMList)
		  strcpy(pContext->mUIDRCPT, pContext->muid);
#ifdef UPDATE_20070509 // エイリアス設定の実アドレスオプションを別データとして処理する
#ifdef UPDATE_20121010 // RCPT TO:処理中の初期化されないコードによりハングする
		memset(mOption, 0, sizeof(mOption));
#else
		mOption[0] = '\x0';
#endif
        pContext->bRCPT = CheckUser( pContext->muid,
			                         mOption,
			                         pContext->MyAddr,
									&pContext->bDomainRCPT,
									&pContext->bSubDomainRCPT,
									&pContext->bMList,
									 NULL,
									 pContext->fullname);
#else
        pContext->bRCPT = CheckUser( pContext->muid,
			                         pContext->MyAddr,
									&pContext->bDomainRCPT,
									&pContext->bSubDomainRCPT,
									&pContext->bMList,
									 NULL,
									 pContext->fullname);
#endif
#ifdef UPDATE_20060718 // エイリアスアカウントと実アカウントのドメインが異なる場合は、実アカウントに変換する。
    pA = strchr(pContext->mUIDRCPT, '@');
    if ((pM = strchr(pContext->muid, '@'))) {
#ifdef UPDATE_20070509 // エイリアス設定の実アドレスオプションを別データとして処理する
#ifdef LGWAN_ON        // LGWAN機能拡張
	  if (mOption[0] == '0' || bLGWAN && mOption[0] == '2') {// 末尾に '0' または '2'
#ifdef UPDATE_20080531 // LGWAN使用時の振分け機能の強化
		if (mOption[1] == '\x0')
		{
          bDiffAliases = FALSE;
		} else {
		  if (IP_COMP(&mOption[2], pContext->PeerAddr))  // IPアドレスを確認
		  {
            bDiffAliases = FALSE;
		  }
		}
#else
        bDiffAliases = FALSE;
#endif
	  } else if (mOption[0] == '1' || bLGWAN && mOption[0] == '3') { // 末尾に '1' または '3'
#ifdef UPDATE_20080531 // LGWAN使用時の振分け機能の強化
		if (mOption[1] == '\x0')
		{
          bDiffAliases = TRUE;
		} else {
		  if (IP_COMP(&mOption[2], pContext->PeerAddr))  // IPアドレスを確認
		  {
            bDiffAliases = TRUE;
		  }
		}
#else
        bDiffAliases = TRUE;
#endif
	  } else if (pA && pM) { // ドメインが含まれていてドメイン不一致なら
#ifdef UPDATE_20080531 // LGWAN使用時の振分け機能の強化
		if (mOption[1] == '\x0')
		{
	      bDiffAliases = (strcmp(pA, pM) != 0 ? TRUE : FALSE);
		} else {
		  if (IP_COMP(&mOption[2], pContext->PeerAddr))  // IPアドレスを確認
		  {
            bDiffAliases = (strcmp(pA, pM) != 0 ? TRUE : FALSE);
		  }
		}
#else
	    bDiffAliases = (strcmp(pA, pM) != 0 ? TRUE : FALSE);
#endif
	  }
#else
	  if (mOption[0] == '0' || mOption[0] == '2') {// 末尾に '0' または '2'
        bDiffAliases = FALSE;
	  } else if (mOption[0] == '1' || mOption[0] == '3') { // 末尾に '1' または '3'
        bDiffAliases = TRUE;
	  } else if (pA && pM) { // ドメインが含まれていてドメイン不一致なら
	    bDiffAliases = (strcmp(pA, pM) != 0 ? TRUE : FALSE);
	  }
#endif
#else
	  if (*(pM+strlen(pM)-1) == '0') {// 末尾に '0'
        bDiffAliases = FALSE;
		*(pM+strlen(pM)-2) = '\x0';
	  } else if (*(pM+strlen(pM)-1) == '1') { // 末尾に '1'
        bDiffAliases = TRUE;
		*(pM+strlen(pM)-2) = '\x0';
	  } else if (pA && pM) { // ドメインが含まれていてドメイン不一致なら
	     bDiffAliases = (strcmp(pA, pM) != 0 ? TRUE : FALSE);
	  }
#endif
	}
#endif

	   /////////////////////////////////////////////////////////
	   /////////////////////////////////////////////////////////
	   // メーリングリストへの投稿の時でSMTP認証を
	   // 必要とした時認証接続していない場合の処理
	   if (pContext->bMList) { // メッセージがメーリングリストへの投稿の時
	     GetListsPass(pContext->mUIDRCPT, mMLPass);
	     if (!(mMLPass[0] == '\x0' ||
		       mMLPass[0] != '\x0' && pContext->bAUTHSUCCESS)) {
			pContext->bMList = FALSE;
		    if (bDebug) printf("List Group (%s) is Non-authentication.\n", pContext->mUIDRCPT);
            sprintf(pContext->mMessages, SMTP_BAD_NOAUTHML);
            pContext->mToken[0] = '\x0';
#ifdef BTHREAD
            return TRUE;
#else
            *OutputBuffer = pContext->mMessages;
            *OutputBufferLen = strlen(pContext->mMessages);
            return(SMTPRS_SendBuffer);
#endif
		 }
	   }
	///////////////////////////////
		b[0] = b[1] = b[2] = FALSE;
		if (pContext->bonly && pContext->bFROM && pContext->bebp &&
			 ( (!pContext->bRCPT && !pContext->bDomainRCPT) ||
			   ( pContext->bRCPT && pContext->bDomainRCPT) ) )
		  b[0] = TRUE;

        if( pContext->bonly && !pContext->bFROM &&
			 ( pContext->bRCPT && pContext->bDomainRCPT) )
		  b[1] = TRUE;  

		if (!pContext->bonly && (
			   (pContext->bFROM && pContext->bRCPT) ||
#ifdef UPDATE_20060618 // 内部宛にしか受け付けないオプションをeffect.datに追加
			   (pContext->bFROM && (!pContext->bRCPT && !pContext->bDomainRCPT && !pContext->bnorelay) ) ||
#else
			   (pContext->bFROM && (!pContext->bRCPT && !pContext->bDomainRCPT) ) ||
#endif
		       (pContext->bRCPT && (!pContext->bFROM && !pContext->bDomainFROM) ) ) )
           b[2] = TRUE;

#ifdef UPDATE_20060618 // 内部宛にしか受け付けないオプションをeffect.datに追加
#ifdef UPDATE_20070525 // SMTP認証が成功している場合で存在しない内部ユーザは拒否するようにする
        if ( b[0] || b[1] || b[2] || (!pContext->bnoauth && pContext->bAUTHSUCCESS && ( pContext->bRCPT && pContext->bDomainRCPT)))
#else
        if ( b[0] || b[1] || b[2] || (!pContext->bnoauth && pContext->bAUTHSUCCESS))
#endif
#else
        if ( b[0] || b[1] || b[2])
#endif
		{
			/*
		if ( ( pContext->bonly && 
			    (pContext->bFROM && pContext->bebp &&
			      (!pContext->bRCPT && !pContext->bDomainRCPT) ||
			      ( pContext->bRCPT && pContext->bDomainRCPT) ) ) ||
              ( pContext->bonly && 
			    (!pContext->bFROM &&
			      ( pContext->bRCPT && pContext->bDomainRCPT) ) ) ||
			 (!pContext->bonly && (
			   (pContext->bFROM && pContext->bRCPT) ||
			   (pContext->bFROM && (!pContext->bRCPT && !pContext->bDomainRCPT) ) ||
		       (pContext->bRCPT && (!pContext->bFROM && !pContext->bDomainFROM) ) ) )
		    ) {
			*/
		//if (pContext->bFROM || pContext->bRCPT) {
		  if (strlen(pContext->mFnHead) > 0) {
			if (nRCPTMaximum == 0 ||
				pContext->nRCPTCount < nRCPTMaximum) {
			  if ( GetMailBoxSize(pContext->muid, pContext->bDomainRCPT, pContext->bSubDomainRCPT) ) {
                pContext->State = SmtpRecpient;
			    pContext->nRCPTCount++;
                sprintf(pContext->mMessages, SMTP_GOOD_RCPT, pContext->p);
#ifdef MILTER_ON // MILTERインターフェースを追加。
                if (bMILTER)
				{
                  if (!MLT_TO(lpClientContext))
				  {
                    pContext->State = SmtpMailFrom;
                    pContext->mToken[0] = '\x0';
#ifdef BTHREAD
                    return TRUE;
#else
                    *OutputBuffer = pContext->mMessages;
                    *OutputBufferLen = strlen(pContext->mMessages);
                    return(SMTPRS_SendBuffer);
#endif
				  }
				}
#endif
                ///////////////////////////////////////
#ifndef TEST_MODE
	            pContext->Rcptfp = fopen(pContext->mFnHead, "at");
	            if (pContext->Rcptfp) {
                  if (pContext->pdom)
		            strcat(pContext->muid, pContext->pdom);
#ifndef DATA_CASH
#ifdef UPDATE_20060616 // エイリアスの場合実アカウントに変換されてしまう。
#ifdef UPDATE_20060718 // エイリアスアカウントと実アカウントのドメインが異なる場合は、実アカウントに変換する。
#ifdef UPDATE_20150324 // エンベロープの送信先をファイルへの書込みエラーが発生した場合、リトライを行う
				  nRetry = 0;
				  while(fprintf(pContext->Rcptfp, "Recipient: %s\n", (bDiffAliases ? pContext->muid : pContext->mUIDRCPT)) < 0)
				  {
                    Sleep(100);
					if (++nRetry >= nRcptWRetry)
					{
					  break;
					}
				  }
#else
				  fprintf(pContext->Rcptfp, "Recipient: %s\n", (bDiffAliases ? pContext->muid : pContext->mUIDRCPT));
#endif
#else
		          fprintf(pContext->Rcptfp, "Recipient: %s\n", pContext->mUIDRCPT);
#endif
#else
		          fprintf(pContext->Rcptfp, "Recipient: %s\n", pContext->muid);
#endif
			      sts = fflush(pContext->Rcptfp);
#else
			      fputs("Recipient: ", pContext->Rcptfp);
#ifdef UPDATE_20060616 // エイリアスの場合実アカウントに変換されてしまう。
#ifdef UPDATE_20060718 // エイリアスアカウントと実アカウントのドメインが異なる場合は、実アカウントに変換する。
			      fputs((bDiffAliases ? pContext->muid : pContext->mUIDRCPT), pContext->Rcptfp);
#else
			      fputs(pContext->mUIDRCPT, pContext->Rcptfp);
#endif
#else
			      fputs(pContext->muid, pContext->Rcptfp);
#endif
			      fputs("\n", pContext->Rcptfp);
#endif
#ifdef UPDATE_20060110 // 作成ファイルがクローズしてもクローズされないのでフラッシュしてみる
				  fflush(pContext->Rcptfp);
#endif
		          fclose(pContext->Rcptfp);
#ifdef UPDATE_20060615 // データのディスク書込に異常が有った場合の対策
			      if (sts == EOF) {
                    pContext->bDiskStatus = FALSE; // 異常
                    sprintf(pContext->mMessages, SMTP_DEVICE_FULL);
				  }
#endif
				}
#endif
			  } else {
			    sprintf(pContext->mMessages, SMTP_FULL_RCPT);
			  }
			} else {
			  sprintf(pContext->mMessages, SMTP_MAX_RCPT);
			}
		  } else {
			sprintf(pContext->mMessages, SMTP_BAD_RCPT);
		  }
		} else  {
		  if (pContext->bonly && pContext->bRCPT)
            sprintf(pContext->mMessages, SMTP_BAD_ADDR, pContext->mUIDRCPT);
#ifdef UPDATE_20070525 // SMTP認証が成功している場合で存在しない内部ユーザは拒否するようにする
		  else if (!pContext->bRCPT && pContext->bDomainRCPT)
            sprintf(pContext->mMessages, SMTP_UNKNOW_USER, pContext->mUIDRCPT);
		  else if (pContext->bDomainFROM && pContext->bDomainRCPT)
            sprintf(pContext->mMessages, SMTP_BAD_RCPT);
#else
		  else if (pContext->bDomainFROM && pContext->bDomainRCPT)
            sprintf(pContext->mMessages, SMTP_BAD_RCPT);
		  else if (!pContext->bRCPT && pContext->bDomainRCPT)
            sprintf(pContext->mMessages, SMTP_UNKNOW_USER, pContext->mUIDRCPT);
#endif
		  else 
            sprintf(pContext->mMessages, SMTP_BAD_ADDR, pContext->mUIDRCPT);
		}
#ifndef K_SEARCH // K_SEARCH OEM 版 //KTEC SMTP Archive Receiver
	  } else {
        sprintf(pContext->mMessages, SMTP_BAD_RCPT);
#endif
	  }
	} else {
      sprintf(pContext->mMessages, SMTP_BAD_ARGUMENT);
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
