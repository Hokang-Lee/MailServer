////////////////////////////////////////////////////////////
// Mail.c Copyright K.kawakami
////////////////////////////////////////////////////////////
#include "smtp.h"

#ifdef DOMAIN_AUTH_SPF    // 2006.04.10 ドメイン認証 SPF方式 追加
extern BOOL  bSPF;
extern CHAR  mDomainAUTHSPF[];
extern CHAR  mNS[];
extern CHAR  mNS1[];
extern CHAR  mNS2[];
extern CHAR  mNS3[];
#endif

#ifdef SENDERID
extern BOOL  bSenderID;
#endif
#ifdef UPDATE_20080326 // ドメイン無しアカウントをローカルとしての利用有無選択機能追加
extern BOOL bHaveDomain;
#endif

extern BOOL bDebug;
extern DWORD nLastMsgId;
extern char  mLocalDomain[];
extern char  mSMTPAUTHMODE[];
extern DWORD nFROMSecLevel;
extern char  mTempDir[];
extern DWORD nLocalDomain;
extern BOOL  bCountLock;
extern BOOL  bAcceptLog;
extern CHAR  mMailBoxDir[];
extern char  mMailSpoolDir[];
extern char  mMailQueueDir[];
#ifdef ESMTP_AUTH
  extern BOOL  nSMTPAUTHOnly;
#endif
#ifdef Y2038_BUG
extern char mMonth[12][4];
extern char mWeek[7][4];
#endif
#ifdef CLUSTERING
extern BOOL   nClustering;
extern char   mComName[];
#endif

#ifdef LGWAN_ON        // LGWAN機能拡張
extern BOOL   bLGWAN;
#endif

#ifdef ACCEPT_LOG_LEVEL3
void LEVEL_3_ACCEPTLOG(PCLIENT_CONTEXT lpClientContext, char *mess);
#endif

BOOL Effective_Address(PSmtpContext pContext);
#ifdef UPDATE_20070509 // エイリアス設定の実アドレスオプションを別データとして処理する
BOOL CheckUser(char *user, char *pOpt, char *myaddr, BOOL *bLocal, BOOL *bSubLocal, BOOL *bMList, BOOL *bOutSideAliases, LPBYTE fullname);
#else
BOOL CheckUser(char *user, char *myaddr, BOOL *bLocal, BOOL *bSubLocal, BOOL *bMList, BOOL *bOutSideAliases, LPBYTE fullname);
#endif
#ifdef Y2038_BUG
void gettime(SYSTEMTIME *ltime, char *mt);
#else
void gettime(time_t *ltime, char *mt);
#endif
void GetBaseDirectory(CHAR *BaseDir, char *mMBdir, char *muser, char *myaddr);
#ifdef UPDATE_20050927 // SMTP AUTH のユーザＩＤ情報の格納 個人別対応
BOOL GetPasswordFile(char  *pDir, char *pUser, char *mpass, DWORD *nFSL);
#else
BOOL GetPasswordFile(char  *pDir, char *pUser, char *mpass);
#endif
#ifdef MILTER_ON // MILTERインターフェースを追加。
extern BOOL   bMILTER;         // MILTER機能拡張
BOOL MLT_FROM(PCLIENT_CONTEXT lpClientContext);
#endif

#ifdef BTHREAD
BOOL MailDispatch(PCLIENT_CONTEXT lpClientContext)
#else
SMTPRSDisposition MailDispatch(
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
	char  mLog[1024];
	char  *p, mDA[512];
#ifdef DATA_CASH
	char  mWork[256];
#endif
#ifdef UPDATE_20071205 //RFC2821: メールアドレス長が255Byteのときの対策
	char  mUser[512], mPass[256];
	char  *pdom, mAlias[512];
#else
	char  mUser[256], mPass[256];
	char  *pdom, mAlias[256];
#endif
	CHAR  Bdir[MAX_PATH];
#ifdef SENDERID
	INT   i;
	CHAR  mSubmitter[2048];
#endif
#ifdef CLUSTERING
	HANDLE hFile;
#endif
#ifdef DOMAIN_AUTH_SPF    // 2006.04.10 ドメイン認証 SPF方式 追加
	FILE *fp;
	CHAR mMess[256];
#endif
    int  sts;
    char *pd;
#ifdef UPDATE_20071204 // メッセージＩＤ採番処理を修正(Bym10id)
   	SYSTEMTIME lt;
#endif
#ifdef K_SEARCH // K_SEARCH OEM 版
   	SYSTEMTIME lt;
#endif
#ifdef UPDATE_20070509 // エイリアス設定の実アドレスオプションを別データとして処理する
	char mOption[256];
#endif
#ifdef UPDATE_20080326 // ドメイン無しアカウントをローカルとしての利用有無選択機能追加
	BOOL bAtMark = TRUE; // ドメイン有りか無しか
#endif
#ifdef UPDATE_20200202 // 子プロセスへの引数が空白になるデータは""（ダブルクォーテーション）で括るようにした。
	CHAR mArgv[_MAX_PATH*2];
	CHAR mArgv2[_MAX_PATH*2];
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
    ///////////////////////////////////////////////
#ifdef UPDATE_20120919 // セッションを切らずに連続して別のメール送信が発生すると、前の上長承認状態がリセットされない不具合。
	pContext->bTopRcpto = TRUE; // 最初のRCPT TO:
	pContext->nOption = 0;
	pContext->mBOSSSubject[0] = '\x0';
	pContext->mBOSS[0] = '\x0';
#endif
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
    if (_strnicmp((PCHAR) &pContext->mToken[4], " FROM:", 6) != 0 ){
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
	if (pContext->State >= SmtpMailFrom) {
      sprintf(pContext->mMessages, SMTP_MAIL_DUPLICATE);
      pContext->mToken[0] = '\x0';
#ifdef BTHREAD
      return TRUE;
#else
      *OutputBuffer = pContext->mMessages;
      *OutputBufferLen = strlen(pContext->mMessages);
      return(SMTPRS_SendBuffer);
#endif
	}
	/////////////////////////////////
#ifdef ESMTP_AUTH
		  ///// SMTP AUTH で成功したもののみ送信可能にする。
	if (!pContext->bAUTHSUCCESS &&
        (nSMTPAUTHOnly == 1 || pContext->nAUTHMODE != -1) ) {
	  sprintf(pContext->mMessages, SMTP_MAIL_NEED);
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

    // No file, but copy the banner string and return it.
    //printf("(%08x):MAIL phase [%s]\n", pContext, pContext->mToken);
    strtok(pContext->mToken, "\r\n\x08");
#ifdef SENDERID
	if (bSenderID) {
      strcpy(mSubmitter, pContext->mToken);
	  strlwr(mSubmitter);
	  if ((pContext->p = strstr(mSubmitter, "submitter="))) {
		i = (int)(pContext->p - (char *)&mSubmitter);
	    strncpy(pContext->mSUBMITTER, &pContext->mToken[i+10], sizeof(pContext->mSUBMITTER));
	    pContext->mSUBMITTER[sizeof(pContext->mSUBMITTER)-1]= '\x0';
	  }
	}
#endif

	pContext->p = strstr(pContext->mToken, "<");
	pContext->bBlankFROM = FALSE;
#ifdef UPDATE_20220728 // RFC5831(821/2821) でエンベロープFROMの書式違反の判定フラグの追加
    pContext->bRFC3Dot3Sec = FALSE;  // 3.3 Mail Transactions 規約違反  正しい書式は半角スペースが含まれない=>"MAIL FROM:<reverse-path> [SP <mail-parameters> ] <CRLF>"
#endif
	if (pContext->p) {
	  pContext->bBlankFROM = (strncmp(pContext->p, "<>", 2) == 0 ? TRUE : FALSE);
#ifdef UPDATE_20220728 // RFC5831(821/2821) でエンベロープFROMの書式違反の判定フラグの追加
      pContext->bRFC3Dot3Sec = (*(pContext->p-1)==':' ? TRUE : FALSE);;  // 3.3 Mail Transactions 規約違反  正しい書式は半角スペースが含まれない=>"MAIL FROM:<reverse-path> [SP <mail-parameters> ] <CRLF>"
#endif
	}
	if (!pContext->p)
	  pContext->p = strstr(&pContext->mToken[10], " ");
	if (!pContext->p)
	  pContext->p = strstr(pContext->mToken, ":");
	if (pContext->p) {
	  strtok(pContext->p, ">\r\n");
	  if (strlen(pContext->p) > 0) {
		if (*pContext->p == '<')
          pContext->p++;
		else
	      while(*pContext->p == ' ' || *pContext->p == ':') // ホワイトスペース削除
	        pContext->p++;
#ifndef K_SEARCH // K_SEARCH OEM 版
	  } else {
        sprintf(pContext->mMessages, SMTP_BAD_ARGUMENT);
#endif
	  }

//      if (strlen(pContext->p) > 0) {
      if (pContext->p) {
		///// Sender Machine check /////////
		pContext->phe = 0;
        strncpy(pContext->muid, pContext->p, _MAX_PATH-1);
		strcpy(pContext->mUIDFROM, pContext->muid);
#ifdef UPDATE_20080326 // ドメイン無しアカウントをローカルとしての利用有無選択機能追加
		bAtMark = strchr(pContext->muid, '@'); // ドメイン有りか無しか
#endif
		pContext->bAcptData = FALSE;  // 中継結果フラグ
#ifdef ESMTP_AUTH
        if (pContext->nAUTHMODE == -1)
#endif
		{
          ////// Accept Log の収集 ///////
	      if (bAcceptLog) {
   	        gettime(&pContext->ltime, pContext->mtime);
#ifdef Y2038_BUG
            _tzset();
			SystemTimeToTzSpecificLocalTime(NULL, &pContext->ltime, &pContext->lt);
            sprintf(pContext->mdata, "%02d%02d%02d", (pContext->lt.wYear%100), pContext->lt.wMonth, pContext->lt.wDay );
#else
            pContext->lt = *localtime(&pContext->ltime);
            strftime(pContext->mdata, 128, "%y%m%d", &pContext->lt );
#endif
#ifdef REGTOFILE
      if (nClustering) {
	    sprintf(pContext->mAcptLogFn, "%sacceptlog\\%s\\%s.log", mMailSpoolDir, mComName, pContext->mdata);
	  } else
#endif
	  {
	    sprintf(pContext->mAcptLogFn, "%sacceptlog\\%s.log", mMailSpoolDir, pContext->mdata);
	  }
#ifdef Y2038_BUG
            sprintf(pContext->mdata, "%02d/%s/%04d:%02d:%02d:%02d",pContext->lt.wDay, mMonth[pContext->lt.wMonth-1], pContext->lt.wYear, pContext->lt.wHour, pContext->lt.wMinute, pContext->lt.wSecond);
#else
            strftime( pContext->mdata, 128, "%d/%b/%Y:%H:%M:%S", &pContext->lt );
#endif
		    sprintf(pContext->mAcptLogState, "[%s], (%s), %s[%s], %s", 
                                     pContext->mdata,
                                     pContext->LocalName,
									 pContext->PeerName,
                                     pContext->PeerAddr,
									 pContext->mUIDFROM);
		  }
          //////////////////////////////////
		}
#ifdef UPDATE_20071210 // エンベロープの送信元が255Byte(RFC2821規定)を超えた場合、拒否する。
		if (strlen(pContext->mUIDFROM) > 255) {
           sprintf(pContext->mMessages, SMTP_LONG_ADDRESS, pContext->mUIDFROM);
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

#ifdef K_SEARCH // K_SEARCH OEM 版
	    pContext->bFROM = TRUE;   // 無条件許可許可されたIPアドレス
	    pContext->bDomainFROM = TRUE;
#else

#ifdef UPDATE_20060617 // エイリアスで送信者信頼度も可能にする
      strcpy(mAlias, pContext->mUIDFROM);
	  if ((pdom = strstr(mAlias, "@")))
	    *pdom = '\x0';
#ifdef UPDATE_20070509 // エイリアス設定の実アドレスオプションを別データとして処理する
	  if (!GetAliases((LPCTSTR)SOFT_ALIASES_REG, (char *)mAlias, (char *)pdom, NULL))
#else
	  if (!GetAliases((LPCTSTR)SOFT_ALIASES_REG, (char *)mAlias, (char *)pdom))
#endif
	  {
	    if (pdom)
	      *pdom = '@';
	  }
#endif

#ifdef UPDATE_20050916 // SMTP AUTH のユーザＩＤ情報の格納
		// 認証ＩＤと送信元エンベロープ不一致なら
#ifdef UPDATE_20050927 // SMTP AUTH のユーザＩＤ情報の格納 個人別対応
        if (pContext->nFROMSecLevel && // Level 1 以上なら
#else
	    if (nFROMSecLevel && // Level 1 以上なら
#endif
#ifdef UPDATE_20060617 // エイリアスで送信者信頼度も可能にする
		   strnicmp(mAlias, pContext->mUSER, strlen(pContext->mUSER)))
#else
		   strnicmp(pContext->mUIDFROM, pContext->mUSER, strlen(pContext->mUSER)))
#endif
		{
           sprintf(pContext->mMessages, SMTP_BAD_FROM);
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

#ifdef DOMAIN_AUTH_SPF    // 2006.04.10 ドメイン認証 SPF方式 追加
		if (bSPF && mDomainAUTHSPF[0]) { // 2006.04.10
#ifdef UPDATE_20200202 // 子プロセスへの引数が空白になるデータは""（ダブルクォーテーション）で括るようにした。
		{
			{
		      if (!(p = strchr(pContext->mUIDFROM, '@')))
			  {
				p = pContext->mUIDFROM;
			  }
#else
		  p = strchr(pContext->mUIDFROM, '@');
		  if (p) {
			p++;
			if (*p) { // ブランクでない場合
#endif
#ifdef UPDATE_20060509  // 送信元エンベロープでのSPFのスキップチェックできるようオプションとして追加
#ifdef UPDATE_20200202 // 子プロセスへの引数が空白になるデータは""（ダブルクォーテーション）で括るようにした。
			  sprintf(mDA, "%s %s \"%s\" \"%s\" %s %s", mDomainAUTHSPF, (bDebug ? "1" : "0"), p, pContext->mUIDFROM, pContext->PeerAddr, mNS);
#else
			  sprintf(mDA, "%s %s %s %s %s %s", mDomainAUTHSPF, (bDebug ? "1" : "0"), p, pContext->mUIDFROM, pContext->PeerAddr, mNS);
#endif
#else
			  sprintf(mDA, "%s %s %s %s %s", mDomainAUTHSPF, (bDebug ? "1" : "0"), p, pContext->PeerAddr, mNS);
#endif
		      sprintf(mLog, "start _spawnl(%s)", mDA);
              if (bDebug) printf("%s\n", mLog );
#ifdef ACCEPT_LOG_LEVEL3
              LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
#endif
#ifdef UPDATE_20060509  // 送信元エンベロープでのSPFのスキップチェックできるようオプションとして追加
#ifdef UPDATE_20200202 // 子プロセスへの引数が空白になるデータは""（ダブルクォーテーション）で括るようにした。
			  sprintf(mArgv, "\"%s\"", p);
			  sprintf(mArgv2, "\"%s\"", pContext->mUIDFROM);
#ifdef UPDATE_20240122 // SPF/DKIM/DMARC/ARC実行パスを半角スペースを含むロングネームでも指定可能にする対策
			  {
	          char mCmdl[256];

	          sprintf(mCmdl, "\"%s\"", mDomainAUTHSPF);
			  if (_spawnl(_P_WAIT, mDomainAUTHSPF, mCmdl, (bDebug ? "1" : "0"), mArgv, mArgv2, pContext->PeerAddr, mNS1, mNS2, mNS3, NULL) == 0)
#else
			  if (_spawnl(_P_WAIT, mDomainAUTHSPF, mDomainAUTHSPF, (bDebug ? "1" : "0"), mArgv, mArgv2, pContext->PeerAddr, mNS1, mNS2, mNS3, NULL) == 0)
#endif
#else
			  if (_spawnl(_P_WAIT, mDomainAUTHSPF, mDomainAUTHSPF, (bDebug ? "1" : "0"), p, pContext->mUIDFROM, pContext->PeerAddr, mNS1, mNS2, mNS3, NULL) == 0)
#endif
#else
			  if (_spawnl(_P_WAIT, mDomainAUTHSPF, mDomainAUTHSPF, (bDebug ? "1" : "0"), p, pContext->PeerAddr, mNS1, mNS2, mNS3, NULL) == 0)
#endif
			  {
                strcpy(mLog, "_spawnl() 'Reject (SPF).'");
              if (bDebug) printf("%s\n", mLog );
#ifdef ACCEPT_LOG_LEVEL3
                LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
#endif
                 strnicmp(pContext->mUIDFROM, pContext->mUSER, strlen(pContext->mUSER));
                 sprintf(pContext->mMessages, SMTP_BAD_SPF);
                 pContext->mToken[0] = '\x0';
#ifdef BTHREAD
                 return TRUE;
#else
                 *OutputBuffer = pContext->mMessages;
                 *OutputBufferLen = strlen(pContext->mMessages);
                 return(SMTPRS_SendBuffer);
#endif
			  }
#ifdef UPDATE_20240122 // SPF/DKIM/DMARC/ARC実行パスを半角スペースを含むロングネームでも指定可能にする対策
			  }
#endif
              strcat(mLog, "_spawnl() 'Pass (SPF).'");
              if (bDebug) printf("%s\n", mLog );
#ifdef ACCEPT_LOG_LEVEL3
              LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
#endif
			}
		  }
		}
#endif

		pContext->bFROM = FALSE; // ステータスリセット
		if (!Effective_Address(pContext)) {
  		  ///// Domain check /////////
		  if (pContext->bebp || pContext->bonly) {
            //strncpy(pContext->muid, pContext->p, _MAX_PATH-1);
		    pContext->bDomainFROM = pContext->bMList = pContext->bOutSideAliases = FALSE;
#ifdef UPDATE_20070509 // エイリアス設定の実アドレスオプションを別データとして処理する
#ifdef UPDATE_20121010 // RCPT TO:処理中の初期化されないコードによりハングする
		    memset(mOption, 0, sizeof(mOption));
#else
            mOption[0]= '\x0';
#endif
            pContext->bFROM = CheckUser( pContext->muid,
				                         mOption,
			                             pContext->MyAddr,
									    &pContext->bDomainFROM,
										 NULL,
										 NULL,
										&pContext->bOutSideAliases,
										 pContext->fullname);
#else
            pContext->bFROM = CheckUser( pContext->muid,
			                             pContext->MyAddr,
									    &pContext->bDomainFROM,
										 NULL,
										 NULL,
										&pContext->bOutSideAliases,
										 pContext->fullname);
#endif

#ifdef UPDATE_20050512
#ifdef UPDATE_20060616 // エイリアスの場合実アカウントに変換されてしまう。
		  if (!strstr(pContext->mUIDFROM, "@")) // ドメイン無いもののみ書き換える
			if (pd = strrchr(pContext->muid, '@'))
			  strcat(pContext->mUIDFROM, pd);
#else
			strcpy(pContext->mUIDFROM, pContext->muid);
#endif
#endif

#ifndef UPDATE_20080531 // LGWAN使用時の振分け機能の強化
#ifdef UPDATE_20070509 // エイリアス設定の実アドレスオプションを別データとして処理する
#ifdef LGWAN_ON        // LGWAN機能拡張
	  if (bLGWAN)
#endif
	  {
	    if (mOption[0] == '2' || mOption[0] == '3') { // 末尾に '2'　または '3
		  if (mOption[1] == ',') { // 特定IPの場合
		    if (IP_COMP(&mOption[2], pContext->PeerAddr)) {  // IPアドレスを確認
		    //if (!strcmp(&mOption[2], pContext->PeerAddr)) {
		      strcpy(pContext->mUIDFROM, pContext->muid); // 実アドレスに変換
			}
		  } else {
	        strcpy(pContext->mUIDFROM, pContext->muid); // 実アドレスに変換
		  }
		}
	  }
#endif
#endif

#ifdef ESMTP_AUTH
		   ///// SMTP AUTH で成功したローカルユーザののみ送信可能にする。
			if (nSMTPAUTHOnly == 2 &&
				stricmp(mSMTPAUTHMODE, "no") != 0 &&
				strlen(mSMTPAUTHMODE) > 0) {
			  strcpy(mUser, pContext->muid);
              GetBaseDirectory(Bdir, mMailBoxDir, mUser, pContext->MyAddr);

              // (2006/03/14) 外部ドメインのSMTP認証対象のメールボックス先指定を無効
			  //if (pContext->bOutSideAliases) { //if (!pContext->bOutSideAliases)
			  //  strtok(mUser, "@");
              //  GetBaseDirectory(Bdir, mMailBoxDir, mUser, pContext->MyAddr);
			  //}

#ifdef UPDATE_20050927 // SMTP AUTH のユーザＩＤ情報の格納 個人別対応
              if (GetPasswordFile(Bdir, mUser, mPass, &pContext->nFROMSecLevel) &&
			      !pContext->bAUTHSUCCESS &&
				  pContext->bFROM)
#else
              if (GetPasswordFile(Bdir, mUser, mPass) &&
			      !pContext->bAUTHSUCCESS &&
				  pContext->bFROM)
#endif
			  {
	            sprintf(pContext->mMessages, SMTP_AUTH_NEED);
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
	        if (!pContext->bFROM && !pContext->bDomainFROM ||
			   pContext->bFROM  && pContext->bonly )
	          pContext->phe = (PHOSTENT)1;
		  }/* else if (pContext->bonly) {
	        pContext->phe = (PHOSTENT)1;
		  }*/
		} else {
		  pContext->bDomainFROM = FALSE; 
#ifdef UPDATE_20070509 // エイリアス設定の実アドレスオプションを別データとして処理する
#ifdef UPDATE_20121010 // RECT TO:処理中の初期化されないコードによりハングする
		  memset(mOption, 0, sizeof(mOption));
#else
          mOption[0]= '\x0';
#endif
          pContext->bFROM = CheckUser( pContext->muid,
			                           mOption,
			                           pContext->MyAddr,
									  &pContext->bDomainFROM,
			  						   NULL,
									   NULL,
									   NULL,
									   pContext->fullname);
#else
          pContext->bFROM = CheckUser( pContext->muid,
			                           pContext->MyAddr,
									  &pContext->bDomainFROM,
			  						   NULL,
									   NULL,
									   NULL,
									   pContext->fullname);
#endif
#ifdef UPDATE_20050512
#ifdef UPDATE_20060616 // エイリアスの場合実アカウントに変換されてしまう。
		  if (!strstr(pContext->mUIDFROM, "@")) // ドメイン無いもののみ書き換える
			if (pd = strrchr(pContext->muid, '@'))
			  strcat(pContext->mUIDFROM, pd);
#else
		  strcpy(pContext->mUIDFROM, pContext->muid);
#endif
#endif
#ifndef UPDATE_20080531 // LGWAN使用時の振分け機能の強化
#ifdef UPDATE_20070509 // エイリアス設定の実アドレスオプションを別データとして処理する
#ifdef LGWAN_ON        // LGWAN機能拡張
	     if (bLGWAN)
#endif
		 {
	       if (mOption[0] == '2' || mOption[0] == '3') { // 末尾に '2'　または '3
		     if (mOption[1] == ',') { // 特定IPの場合
		       if (!strcmp(&mOption[2], pContext->PeerAddr)) {
		         strcat(pContext->mUIDFROM, pContext->muid); // 実アドレスに変換
			   }
			 } else {
	           strcat(pContext->mUIDFROM, pContext->muid); // 実アドレスに変換
			 }
		   }
		 }
#endif
#endif
          /////////////////////////
		  if (pContext->bonly) {
		    if (!pContext->bFROM && !pContext->bDomainFROM)
 	          pContext->phe = (PHOSTENT)1;
		  } else {
		    pContext->bFROM = TRUE;   // 受信許可されたIPアドレス
		    pContext->bDomainFROM = TRUE;
		  }
		}
#endif // #ifdef K_SEARCH
        ///////////////////////////////
        if (pContext->bFROM || pContext->phe) {
#ifdef MILTER_ON // MILTERインターフェースを追加。
          if (bMILTER)
		  {
            if (!MLT_FROM(lpClientContext))
			{
              pContext->State = SmtpGreeting;
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
        ///////////////////////////////
          //nLastMsgId = GetProfileIntEx("SOFTWARE\\EMWAC\\IMS", "LastMsgId", 0);
          //nLastMsgId = GetProfileIntEx(SOFT_REG, "LastMsgId", 0);
#ifdef CLUSTERING
		  ///////////////
		  // カウンタ値を任意のファイルに保存(クラスタ対応)
		  hFile = NULL;
		  nLastMsgId = ReadLastMsgIdForAscii(&hFile, nLastMsgId);
		  if (!pContext->bRCPTReset) { // RCPT->REST後の再送でないならカウントアップ
	        pContext->nMsgId = ++nLastMsgId;
		    WriteLastMsgIdForAscii(&hFile, nLastMsgId);
		  } else {
	        if (hFile)
		      CloseHandle(hFile); // 排他解除
		  }
		  ///////////////
#else
		  ///////////////
		  if (!pContext->bRCPTReset) // RCPT->REST後の再送でないならカウントアップ
	        pContext->nMsgId = ++nLastMsgId;
#endif
#ifdef UPDATE_20071204 // メッセージＩＤ採番処理を修正(Bym10id)
		  // ユニークなメッセージＩＤ作成
	      GetLocalTime(&lt);
          sprintf(pContext->mMsgId, 
				  "B%x%x%010d",
                  lt.wYear%10,
                  lt.wMonth,
				  pContext->nMsgId);
		  /*
          sprintf(pContext->mMsgId, 
				  "B%04d%02d%02d%02d%02d%02d%03d%010d",
                  lt.wYear,
                  lt.wMonth,
                  lt.wDay,
                  lt.wHour,
                  lt.wMinute,
                  lt.wSecond,
                  lt.wMilliseconds,
				  pContext->nMsgId);
		  */
#endif

#ifdef K_SEARCH // K_SEARCH OEM 版
		  // ユニークなメッセージＩＤ作成
	      GetLocalTime(&lt);
		  //srand((unsigned)time( NULL ));
          sprintf(pContext->mMsgId, 
				  "B%x%x%010d",
                  lt.wYear%10,
                  lt.wMonth,
				  pContext->nMsgId);
		  /*
          sprintf(pContext->mMsgId, 
			      //"%04d%02d%02d%02d%02d%02d%03d.%06d.%08d",
				  "%04d%02d%02d%02d%02d%02d%03d.%08d",
                  lt.wYear,
                  lt.wMonth,
                  lt.wDay,
                  lt.wHour,
                  lt.wMinute,
                  lt.wSecond,
                  lt.wMilliseconds,
                  //rand(),
				  pContext->nMsgId);
		  */
#endif
#ifdef UPDATE_20260610B // 本文情報がブラックリストにヒットした場合はinlogへの記録はしない
		  pContext->nMsgId2 = 0;
		  pContext->mMsgId2[0] ='\x0';
#endif
		  pContext->bRCPTReset = FALSE;  // RCPT->REST後フラグ初期化
          //WriteProfileIntEx("SOFTWARE\\EMWAC\\IMS", "LastMsgId", (DWORD)nLastMsgId);
          WriteProfileIntEx(SOFT_REG, "LastMsgId", (DWORD)nLastMsgId);
		  bCountLock = FALSE;// Counter release
		  ///////////////////////////////////////////////////////
#ifndef TUNING1
		  //sprintf(pContext->mFnRset, "%sB%010lu.RST", mTempDir, pContext->nMsgId);
	      //sprintf(pContext->mFnHead, "%sB%010lu.RCP", mTempDir, pContext->nMsgId);
#ifdef UPDATE_20041224_FILESPEED
		  sprintf(pContext->mFnRset, "%s%stemp_B%010lu.RST", mMailSpoolDir,mMailQueueDir, pContext->nMsgId);
	      sprintf(pContext->mFnHead, "%s%stemp_B%010lu.RC$", mMailSpoolDir,mMailQueueDir, pContext->nMsgId);
		  sprintf(pContext->mFnData, "%s%stemp_B%010lu.MSG", mMailSpoolDir,mMailQueueDir, pContext->nMsgId);
#else
#ifdef UPDATE_20071204  // メッセージＩＤ採番処理を修正(Bym10id)
		  sprintf(pContext->mFnRset, "%s%s.RST", mTempDir, pContext->mMsgId);
	      sprintf(pContext->mFnHead, "%s%s.RCP", mTempDir, pContext->mMsgId);
	      sprintf(pContext->mFnData, "%s%s.MSG", mTempDir, pContext->mMsgId);
#else
		  sprintf(pContext->mFnRset, "%sB%010lu.RST", mTempDir, pContext->nMsgId);
	      sprintf(pContext->mFnHead, "%sB%010lu.RCP", mTempDir, pContext->nMsgId);
	      sprintf(pContext->mFnData, "%sB%010lu.MSG", mTempDir, pContext->nMsgId);
#endif
#endif
#else
		  sprintf(pContext->mFnRset, "%s%sB%010lu.RST",mMailSpoolDir,mMailQueueDir,pContext->nMsgId);
	      sprintf(pContext->mFnHead, "%s%sB%010lu.RC$",mMailSpoolDir,mMailQueueDir,pContext->nMsgId);
	      sprintf(pContext->mFnData, "%s%sB%010lu.MS$",mMailSpoolDir,mMailQueueDir,pContext->nMsgId);
#endif
	      _unlink(pContext->mFnHead);
 	      _unlink(pContext->mFnData);
          ///////////////////////////////
		  if (strlen(pContext->mFnHead) > 0) {
#ifndef TEST_MODE
            if (pContext->State != SmtpMailFrom)
	          pContext->Mailfp = fopen(pContext->mFnHead, "wt");
		    else
	          pContext->Mailfp = fopen(pContext->mFnHead, "at");
	        if (pContext->Mailfp) {
#ifndef DATA_CASH
#ifdef K_SEARCH // K_SEARCH OEM 版
		      fprintf(pContext->Mailfp, "Message-ID: <%s@%s>\n", pContext->mMsgId, pContext->LocalName);
#else
#ifdef UPDATE_20071204  // メッセージＩＤ採番処理を修正(Bym10id)
		      fprintf(pContext->Mailfp, "Message-ID: <%s@%s>\n", pContext->mMsgId, pContext->LocalName);
#else
		      fprintf(pContext->Mailfp, "Message-ID: <B%010lu@%s>\n", pContext->nMsgId, pContext->LocalName);
#endif
#endif
			  fprintf(pContext->Mailfp, "Return-path: %s\n", pContext->mUIDFROM);
			  sts = fflush(pContext->Mailfp);
#else
#ifdef K_SEARCH // K_SEARCH OEM 版
		      sprintf(mWork, "Message-ID: <%s@", pContext->mMsgId);
#else
#ifdef UPDATE_20071204  // メッセージＩＤ採番処理を修正(Bym10id)
		      sprintf(mWork, "Message-ID: <%s@", pContext->mMsgId);
#else
		      sprintf(mWork, "Message-ID: <B%010lu@", pContext->nMsgId);
#endif
#endif
			  fputs(mWork, pContext->Mailfp);
			  fputs(pContext->LocalName, pContext->Mailfp);
			  fputs(">\n", pContext->Mailfp);
			  fputs("Return-path: ", pContext->Mailfp);
			  fputs(pContext->mUIDFROM, pContext->Mailfp);
			  fputs("\n", pContext->Mailfp);
#endif
		      fclose(pContext->Mailfp);
#ifdef UPDATE_20060615 // データのディスク書込に異常が有った場合の対策
			  if (sts == EOF) {
                pContext->bDiskStatus = FALSE; // 異常
                sprintf(pContext->mMessages, SMTP_DEVICE_FULL);
			  } else 
#endif
			  {
		      // Put us into Authorization state
              pContext->State = SmtpMailFrom;
			  pContext->nRCPTCount = 0; // Count Clear
              sprintf(pContext->mMessages, SMTP_GOOD_MAIL, pContext->p);
			  }
			} else {
              sprintf(pContext->mMessages, SMTP_BAD_FROM);
			}
#else
            pContext->State = SmtpMailFrom;
		    pContext->nRCPTCount = 0; // Count Clear
			sprintf(pContext->mMessages, SMTP_GOOD_MAIL, pContext->p);
#endif
		  } else {
            sprintf(pContext->mMessages, SMTP_BAD_FROM);
		  }
		} else {
#ifdef UPDATE_20080326 // ドメイン無しアカウントをローカルとしての利用有無選択機能追加
	      if (!pContext->bFROM && pContext->bDomainFROM && (bHaveDomain || bAtMark))
#else
	      if (!pContext->bFROM && pContext->bDomainFROM)
#endif
            sprintf(pContext->mMessages, SMTP_BAD_FROM);
	      else if (!pContext->bebp)
            sprintf(pContext->mMessages, SMTP_BAD_IP);
		  else 
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
	  } else {
        sprintf(pContext->mMessages, SMTP_BAD_ARGUMENT);
 	    pContext->mToken[0] = '\x0';
#ifdef BTHREAD
        return TRUE;
#else
        *OutputBuffer = pContext->mMessages;
        *OutputBufferLen = strlen(pContext->mMessages);
        return(SMTPRS_SendBuffer);
#endif
	  }
	} else {
      sprintf(pContext->mMessages, SMTP_BAD_ARGUMENT);
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
