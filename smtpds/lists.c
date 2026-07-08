#include "smtp.h"
#include <windows.h>       /* required for all Windows applications */
#include <winsock.h>
#include <stdio.h>         /* for sprintf                           */
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <direct.h>
#include "profile.h"

extern LCID    nLCID;
extern BOOL    bServiceTerminating;
extern char    mMailSpoolDir[];
extern char    mLocalDomain[];
extern BOOL    bDebug;
//extern char    mListsLockFn[];
extern char    mArticleLockFn[];

#ifdef CLUSTERING
extern BOOL   nClustering;
#endif

#ifdef UPDATE_20070521 // OSの予約語対策
extern CHAR   mReservedWords[];
#endif
#ifdef UPDATE_20240127 // ＭＬ制御応答＆送信エラー通知メールにもDKIMサインを追加可能にするオプション
// DKIM設定箇所の選択 bit0:(受信時付加)デフォルト bit1:上長承認メールへ付加 bit2:添付分離めーるへ付加
extern int    bDKIM;
extern CHAR   mDomainAUTHDKIM[];
#endif
#ifdef UPDATE_20240127C // DKIMサイン実施無効フラグを追加 1:無効(bit0:送信エラー bit1:ＭＬ制御応答 bit2:転送メール bit:3自動応答)
extern int    nOnDKIM;
extern CHAR   mDomainAUTHARC[];
#endif

#ifdef Y2038_BUG
void gettime(SYSTEMTIME *ltime, char *mt);
#else
void gettime(time_t *ltime, char *mt);
#endif
int SendGlobalMail(struct _RCP *rcp);
BOOL SetRCPFile(struct _RCP *rcp, char *mRCP, BOOL bmod, DWORD *no);
DWORD GetMLMembers(FILE *fp, char *pML);
void GetML_ARTICLE_INDEX(FILE *fp, char *pML, char *pWhere);
void GetML_ARTICLE_LIST(struct _RCP *rcp, FILE *fp, char *mFolder, char *mFileNo, char *pML, char *pWhere);
void GetML_ARTICLE_DELETE(FILE *fp, char *pML, char *pWhere);
#ifdef UPDATE_20050123
BOOL ScrambleBegin(HANDLE *hFile, char *pFn, BOOL bType);
#endif
void translateUue2Base64(char *line, int inlen, char *newline);
int Base64DecodeLine(char * pszLine, int nLineLen, unsigned char * uchBuffer, int nBufferLen);


BOOL GetToken(char *ptkn, char **pML, char **pAddr){
  BOOL sts = FALSE;

   *pML = strpbrk(ptkn, " ");
   if (*pML) {
	 **pML = '\x0';
	 *pAddr = strpbrk(++*pML, " ");
	 if (*pAddr) {
       **pAddr = '\x0';
       ++*pAddr;
       sts = TRUE;
	   while (**pAddr == ' ') {
	 //if (*pAddr) {
	     ++*pAddr;
	   }
	 }
   }
   if (*pML)
     if (strlen(*pML) == 0)
	   *pML = NULL;
   return sts;
}

int QueryLists(BOOL bChk, char *pML, char *pAddr) {
  DWORD  retCode;
  HKEY   hKeyRoot = HKEY_LOCAL_MACHINE;
  HKEY   hKey;
  CHAR   SubKey[512], SubKey2[512];
  int    nsts = -1, i;
  BOOL   bOK = FALSE;
  char   *pAtmark, *pAt = NULL;
#ifdef REGTOFILE
  HANDLE hFile;
#endif
#ifdef UPDATE_20071210A // ML参加アドレスを含む256Byte超のフォルダ名になったと場合の対策
  FILE *fp;
  char mMLFn[256], mMLAddr[260];
#endif
#ifdef UPDATE_20070521 // 予約語対策
  char *q, mUID[512], mKEY[512];
  BOOL bReservedWords1 = FALSE;
  BOOL bReservedWords2 = FALSE;

  if (nClustering){
	strcpy(mUID, pML);
    strtok(mUID, "@");
    if (ReservedWords(mUID)) {
      if ((q = strstr(pML, mUID))) {
        strcpy(mUID, mReservedWords);
        strcat(mUID, q);
  	    bReservedWords1 = TRUE;
	  }
	}
	strcpy(mKEY, pAddr);
    strtok(mKEY, "@");
    if (ReservedWords(mKEY)) {
      if ((q = strstr(pAddr, mKEY))) {
        strcpy(mKEY, mReservedWords);
        strcat(mKEY, q);
  	    bReservedWords2 = TRUE;
	  }
	}
  }
#endif

   if (pAddr == NULL)
	 return nsts;
   if (*pAddr == '\x0')
	 return nsts;
#ifdef UPDATE_20060716A // ML制御の結果メールにML名に付加されているドメイン名が削除されてしまう。
#ifdef UPDATE_20070521 // 予約語対策
  	if (bReservedWords1)
      pAt = strchr(&mUID[strlen(mReservedWords)], '@');
    else
#endif
    pAt = strchr(pML, '@');
#endif
   sprintf(SubKey, "%s\\%s", SOFT_LISTS_REG, pML);
#ifdef UPDATE_20070521 // 予約語対策
  	if (bReservedWords1)
     sprintf(SubKey, "%s\\%s", SOFT_LISTS_REG, mUID);
#endif
#ifdef MULTI_ML
   if (!QueryMLists((LPCTSTR)SOFT_LISTS_REG, pML)) {// ドメイン付MLが無い場合
     strtok(SubKey, "@");
     strtok(pML, "@");
   }
#else
   strtok(SubKey, "@");
#endif

#ifdef REGTOFILE
	 if (nClustering && !strnicmp(SubKey, "software\\emwac", 14)) {
#ifdef UPDATE_20060716 // 存在するMLの先頭文字が一致してしまうと、存在しないML名が登録できてしまう。
	   strcat(SubKey, "\\");
#endif

       retCode = 
         OpenKeyFile(mMailSpoolDir,
		             SubKey,
					 (char *)"",
					 &hFile);
       if (retCode == ERROR_SUCCESS)
	     CloseHandle(hFile);
	 } else {
#endif
   retCode = RegOpenKey(hKeyRoot, SubKey, &hKey); 
   RegCloseKey((HKEY)hKey);
#ifdef REGTOFILE
	 }
#endif
   if (retCode == ERROR_SUCCESS) {
      sprintf(SubKey2, "%s\\Members\\%s", SubKey, pAddr);
#ifdef REGTOFILE
	 if (nClustering && !strnicmp(SubKey2, "software\\emwac", 14)) {
#ifdef UPDATE_20070521 // 予約語対策
  	   if (bReservedWords2)
         sprintf(SubKey2, "%s\\Members\\%s", SubKey, mKEY);
#endif
	   strcat(SubKey2, "\\");
       retCode = 
         OpenKeyFile(mMailSpoolDir,
		             SubKey2,
					 (char *)"",
					 &hFile);
	   if (retCode == ERROR_SUCCESS)
	     CloseHandle(hFile);
#ifdef UPDATE_20071210A // ML参加アドレスを含む256Byte超のフォルダ名になったと場合の対策
       if (retCode != ERROR_SUCCESS) {
         sprintf(mMLFn, "%sreg\\%s\\Members\\ext.dat", mMailSpoolDir, SubKey);
		 if ((fp = fopen(mMLFn, "rt"))) {
		   while (fgets(mMLAddr, sizeof(mMLAddr), fp)) {
			 strtok(mMLAddr, "\r\n");
			 if (!stricmp(mMLAddr, pAddr)) {
			   retCode = ERROR_SUCCESS;
			   break;
			 }
		   }
           fclose(fp);
		 }
  	   }
#endif
	 } else {
#endif
      retCode = RegOpenKey(hKeyRoot, SubKey2, &hKey); 
      RegCloseKey((HKEY)hKey);
#ifdef REGTOFILE
	 }
#endif
      if (retCode == ERROR_SUCCESS)
	    nsts = 1;
	  else
	    nsts = 0;
   }

   if (bChk) { // アドレスキャラクタチェック有効
	 pAtmark = strchr(pAddr, '@');
	 if (pAtmark) {
	   *pAtmark = '\x0';
       for (i = 0; i < (int)strlen(pAddr); i++) {
	     bOK = FALSE;
	     if (0x20 <= *(pAddr+i) && *(pAddr+i) <= 0x7f)
	       bOK = TRUE;
	     else
	       break;
	   }
	   *pAtmark = '@';
       for (i = 0; i < (int)strlen(pAtmark+1); i++) {
	     bOK = FALSE;
	     if (0x20 < *(pAtmark+1+i) && *(pAtmark+1+i) <= 0x7f)
	       bOK = TRUE;
	     else
	       break;
	   }
	 } else
 	   bOK = FALSE;
   if (!bOK)
     nsts = 2; // メールアドレスの文字に規定外の文字を発見
   }
#ifdef UPDATE_20060716A // ML制御の結果メールにML名に付加されているドメイン名が削除されてしまう。
   if (pAt)
     *pAt = '@';
#endif

   return nsts;
}

void ModeratorRequest(struct _RCP *rcp, char *mMDR, char *mtkn, char *mt, int mode) {
  char  mwork[512], mRCP[512], mMSG[512], *p;
  FILE  *fp;
  struct _RCP r;
#ifdef UPDATE_20070521 // 予約語対策
  char  mUID[512];
#endif

  memcpy(&r, rcp, sizeof(struct _RCP));
  strcpy(mwork, r.mTo);

#ifdef UPDATE_20070521 // 予約語対策
  if (nClustering) {
	strcpy(mUID, r.mTo);
    strtok(mUID, "@");
    if (ReservedWords(mUID)) {
      strcpy(mwork, mReservedWords);
      strcat(mwork, r.mTo);
	}
  }
#endif

  //strtok(mwork, "@");
  sprintf(mRCP, "%slists\\%s\\%s-T0001.RCP", mMailSpoolDir, mwork, r.mMNo);
  sprintf(mMSG, "%slists\\%s\\%s-T0001.MSG", mMailSpoolDir, mwork, r.mMNo);
  if ((fp = fopen(mMSG, "wt"))) {
#ifdef DATA_CASH
    setvbuf(fp, rcp->mFwbuf, _IOFBF, sizeof(rcp->mFwbuf) );
#endif

    fprintf(fp, "Subject: %s Request\n", (mode ? "LEAVE" : "JOIN"));
	if (!strstr(mwork, "@"))
      fprintf(fp, "From: %s@%s\n", mwork, mLocalDomain); //rcp->mSmtp);
	else
      fprintf(fp, "From: %s\n", mwork); //rcp->mSmtp);
    fprintf(fp, "To: %s\n", mMDR);
	fprintf(fp, "Message-Id: <%s>\n", r.mMIDNo);
    fprintf(fp, "Date: %s\n", mt);
    fprintf(fp, "\n");
    fprintf(fp, "The following request from '%s' has been received\n", r.mFrom);
    fprintf(fp, "by the %sMailing List Server at %s.\n", TRADEMARK, rcp->mDomain); //(mLocalDomain ? mLocalDomain : "-")); //rcp->mSmtp);
    fprintf(fp, "\n");
#ifdef UPDATE_20050526  // 管理人宛へのMLアドレスが"-"の含まれるドメインだとおかしくなる。
    p = mwork;
	while (*p && strnicmp(p, "-request", 8)) 
	  p++;
#else
    p = strrchr(mwork, '-');
#endif
	if (p) {
	  *p = '\x0';
#ifdef UPDATE_20050526  // 管理人宛へのMLアドレスが"-"の含まれるドメインだとおかしくなる。
	  if (p = strchr(p+1, '@'))
		strcat(mwork, p); // ドメインがあれば結合
#endif
#ifdef UPDATE_20060317 // 管理人通知のメッセージで余分なメッセージが追加される
      fprintf(fp, "\n%s %s %s\n.\n", (mode ? "LEAVE" : "JOIN"), mwork, r.mFrom);
#else
      fprintf(fp, "\n%s %s %s\n.\n", mtkn, mwork, r.mFrom);
#endif
	} else {
      fprintf(fp, "\n%s\n.\n", mtkn);
	}
	fflush(fp);
	fclose(fp);
	// 送信ファイル、送信元、送信先を入れ替え
    strcpy(mwork, r.mMsg);
    strcpy(r.mMsg, mMSG);
    strcpy(mMSG, mwork);
    strcpy(mwork, r.mFrom);
	////////////
	if (!strstr(r.mTo, "@"))
      sprintf(r.mFrom, "%s@%s", r.mTo, mLocalDomain);
	else
      strcpy(r.mFrom, r.mTo);
	////////////
    strcpy(r.mTo, mMDR);
	// メール送信
    SetRCPFile(&r, mRCP, FALSE, NULL);
  }
}

void ListControl(struct _RCP *rcp, char *mUser, DWORD n) {
  char  mwork[512], mwk[512], mML[512], mL[512], *pL;
  char  mMSG[512], mRCP[512], mt[128];
  char  *p, *pML, *pAddr, *pAt;
  FILE  *fp, *fpd;
  DWORD retCode;
  HKEY   hKeyRoot = HKEY_LOCAL_MACHINE;
  HKEY   hKey;
  CHAR   SubKey[512], SubKey2[512], mMDR[512];
  BOOL   bDom = TRUE; // ドメイン有りML
#ifdef Y2038_BUG
   SYSTEMTIME ltime;
#else
   time_t ltime;
#endif
  BOOL   bMsg = FALSE;
  int    nsts;
  BOOL   bMCJ, bMCL, bNoReply;
  DWORD  nBounceAction; 
  CHAR   mCharSet[128];
#ifdef UPADTE_20040315
  BOOL   bsts = FALSE;
  char   *pdom;
  CHAR   mName[_MAX_PATH];	 // address of buffer for subkey name 
#endif  
#ifdef UPDATE_20050123
  HANDLE hFile, hFLis;
#endif
#ifdef UPDATE_20060513 // ＭＬ管理メールにキーコードを付加
  time_t lts, ltn;
  DWORD  nPWChk = 0;
  BOOL   bPWMach = FALSE;
  char   *pcp, mPwd[512] = "", mCPwd[512] = "";
  char   *pWSP, mSub[1024] = "";
#endif
  char  mB64[512] = "", mB641[512] = "";
#ifdef UPDATE_20070521 // 予約語対策
  char mKEY[512], mUID[512];
  BOOL bReservedWords = FALSE;
  BOOL bReservedWords2 = FALSE;
#endif
#ifdef UPDATE_20071210A // ML参加アドレスを含む256Byte超のフォルダ名になったと場合の対策
  FILE *fp1, *fp2;
  BOOL bNewML;
  char mMLFn[256], mMLFn2[256], mMLAddr[260];
#endif
#ifdef UPDATE_20090925 // メーリングリスト宛の制御メールの本文がBase64にエンコードされる場合の対策
  BOOL bCType = TRUE; // Content-Type:
  BOOL bB64 = FALSE;   // Content-Transfer-Encoding:
  CHAR mB642[512];
#endif
#ifdef UPDATE_20130819 // ML名を指定した制御命令を無効にするオプションを追加
  BOOL bMCM = TRUE;
  CHAR *pL2;
#endif
  //////////////////////////////////////////////
#ifdef UPDATE_20050123
  ScrambleBegin(&hFile, mArticleLockFn, FALSE);
#endif
  //////////////////////////////////////////////
  sprintf(SubKey, "%s\\%s", SOFT_LISTS_REG, mUser);
  strcpy(mL, mUser);
#ifdef MULTI_ML
  if (!QueryMLists((LPCTSTR)SOFT_LISTS_REG, mUser)) { // ドメイン付MLが無い場合
     strtok(SubKey, "@");
	 bDom = FALSE; // ドメイン無しML
  }
#else
   strtok(SubKey, "@");
   bDom = FALSE; // ドメイン無しML
#endif
#ifdef UPDATE_20130819 // ML名を指定した制御命令を無効にするオプションを追加
  bMCM = GetProfileIntEx(SubKey, "ListControlNameMach", (int)TRUE);
#endif
  bNoReply = GetProfileIntEx(SubKey, "NoReply", (int)FALSE);
  bMCJ = GetProfileIntEx(SubKey, "ModeratorControlJoin", (int)FALSE);
  bMCL = GetProfileIntEx(SubKey, "ModeratorControlLeave", (int)FALSE);
  GetProfileStringEx(SubKey,"Moderator", "", mMDR, sizeof(mMDR));
  GetProfileStringEx(SubKey,"CharSet", "", mCharSet, sizeof(mCharSet));
#ifdef UPDATE_20060513 // ＭＬ管理メールにキーコードを付加
  nPWChk = 0;
  if (bMCJ || bMCL) {
    nPWChk = GetProfileIntEx(SubKey, "ModeratorKey", (int)0); // 投稿コード有効期間分単位
    if (nPWChk != 0) {
	  time(&lts);
	  sprintf(mPwd, "%s:%lu:%lu", mMDR, lts, nPWChk);
	  SPA_Encode(mPwd, mCPwd); // 記号化
	  translateUue2Base64(mCPwd, strlen(mCPwd), mB64);
	}
  }
#endif
  //////////////////////////////////////////////
  strcpy(mwork, rcp->mTo);
  //sprintf(mwork, "%s-%s", rcp->mMNo, rcp->mTo);
  //strtok(mwork, "@");
  sprintf(mwk, "%slists\\%s", mMailSpoolDir, mwork);
#ifdef UPDATE_20070521 // 予約語対策
  if (nClustering){
	strcpy(mKEY, mwork);
    strtok(mKEY, "@");
    if (ReservedWords(mKEY)) {
      strcpy(mKEY, mReservedWords);
      strcat(mKEY, mwork);
      sprintf(mwk, "%slists\\%s", mMailSpoolDir, mKEY);
	  bReservedWords = TRUE;
	}
  }
  if (!strnicmp(mUser, mReservedWords, strlen(mReservedWords))) {
    bReservedWords2 = TRUE;
  }
#endif
  _mkdir(mwk);
  sprintf(mMSG, "%s\\%s-L%03d.MSG", mwk, rcp->mMNo, n);
  sprintf(mRCP, "%s\\%s-L%03d.RCP", mwk, rcp->mMNo, n);
  gettime(&ltime, mt);
  if ((fpd = fopen(mMSG, "wt"))) {
#ifdef DATA_CASH
    setvbuf(fpd, rcp->mFwbuf, _IOFBF, sizeof(rcp->mFwbuf) );
#endif
    fprintf(fpd, "Subject: Journal\n");
#ifdef UPDATE_20070521 // 予約語対策
	if ((p = strrchr((bReservedWords2 ? &mUser[strlen(mReservedWords)] : mUser), '@')))
	{
	  *p = '\x0';
      fprintf(fpd, "From: %s-request@%s\n", (bReservedWords2 ? &mUser[strlen(mReservedWords)] : mUser), rcp->mDomain); //rcp->mSmtp);
	  *p = '@';
	} else {
      fprintf(fpd, "From: %s-request@%s\n", (bReservedWords2 ? &mUser[strlen(mReservedWords)] : mUser), rcp->mDomain); //rcp->mSmtp);
	}
#else
	if ((p = strstr(mUser, "@")))
	{
	  *p = '\x0';
      fprintf(fpd, "From: %s-request@%s\n", mUser, rcp->mDomain); //rcp->mSmtp);
	  *p = '@';
	} else {
      fprintf(fpd, "From: %s-request@%s\n", mUser, rcp->mDomain); //rcp->mSmtp);
	}
#endif
    fprintf(fpd, "To: %s\n", rcp->mFrom);
	if (mCharSet[0])
      fprintf(fpd, "Content-Type: text/plain; charset=\"%s\"\n", mCharSet); 
	else if (nLCID == 1041) // 日本語環境
      fprintf(fpd, "Content-Type: text/plain; charset=\"iso-2022-jp\"\n"); 
	fprintf(fpd, "Message-Id: <%s>\n", rcp->mMIDNo);
    fprintf(fpd, "Date: %s\n", mt);
    fprintf(fpd, "\n");
    fprintf(fpd, "Welcome to the %sMailing List Server at %s.\n", TRADEMARK, rcp->mDomain); //rcp->mSmtp);
    fprintf(fpd, "\n");
    while(!(fp = fopen(rcp->mMsg, "rt"))) {
      if (bServiceTerminating)
		break;
	  _sleep(WAIT_TIME);
	}
    if (fp) {
#ifdef DATA_CASH
	  setvbuf(fp, rcp->mFrbuf, _IOFBF, sizeof(rcp->mFrbuf) );
#endif
	  mwork[0] = '\x0';
	  p = fgets(mwork, sizeof(mwork), fp);
	  while(p || !feof(fp)) {
		pML = pAddr = NULL;
        strtok(mwork,"\n");
        if (!bMsg && mwork[0] == '\n')
		  bMsg = TRUE;
#ifdef UPDATE_20060513 // ＭＬ管理メールにキーコードを付加
		if (!bMsg) { // ヘッダチェック
#ifdef UPDATE_20090925 // メーリングリスト宛の制御メールの本文がBase64にエンコードされる場合の対策
		  if (!strnicmp(mwork, "content-type:", 13))
		  {
			p = &mwork[13];
            while(*p == ' ' || *p == '\t')
			{
			  p++;
			}
			if (strnicmp(p, "text/", 5)) // 本文がテキスト以外
			{
			  bCType = FALSE;
			}
		  }	
		  if (!strnicmp(mwork, "content-transfer-encoding:", 26))
		  {
			p = &mwork[26];
            while(*p == ' ' || *p == '\t')
			{
			  p++;
			}
			if (!strnicmp(p, "base64", 6)) // 本文がBASE64
			{
			  bB64 = TRUE;
			}
		  }
#endif
          if (nPWChk != 0) {
		    if (!strnicmp(mwork, "subject:", 8)) {
	          strtok(mwork,"\r\n");
              strcat(mSub, mwork);
			  while((p = fgets(mwork, sizeof(mwork), fp))) {
	            strtok(mwork,"\r\n");
				if (mwork[0] == ' ' || mwork[0] == '\t') {
				  pWSP = mwork;
				  while(*pWSP == ' ' || *pWSP == '\t')
					pWSP++;
				  strcat(mSub, pWSP);
				} else {
				  break;
				}
			  }

			  if ((pcp = strstr(&mSub[8], "[no:"))) {
				mB64[0] = '\x0'; // 制御コードつきのメールには応答メールにコードは記載しない
				pcp+=4;
				strtok(pcp,"]");
				Base64DecodeLine(pcp, strlen(pcp), (unsigned char *)mB641, sizeof(mB641));
                SPA_Decode(mB641, mCPwd); // 管理者カウント復号化
				if ((pcp = strchr(mCPwd, ':'))) {
				  *pcp = '\x0';
				  if (!strcmp(mCPwd, mMDR)) { //管理者アカウントと比較
	                time(&ltn);
					pcp++;
                    lts  = atol(pcp);    // 開始時間
				    if ((pcp = strchr(pcp, ':'))) {
				      pcp++;
                      lts += (DWORD)(atol(pcp) * 60); // 分単位 60秒ゲタ
                      if (lts >= ltn) // 有効期限内なら
				        bPWMach = TRUE;
					}
				  }
				}
			  }
              continue;
			}
		  }
		} else
#else
		if (bMsg)
#endif
		{
#ifdef UPDATE_20090925 // メーリングリスト宛の制御メールの本文がBase64にエンコードされる場合の対策
          if (bCType && bB64)
		  {
			Base64DecodeLine(mwork, strlen(mwork), mB642, sizeof(mB642));
			strcpy(mwork, mB642);
		  }
#endif
		  if (strlen(mwork) < 4) {
#ifdef UPDATE_20070608 // メッセージ配送バッファのクリア 最後の行に同じメッセージが出力されてしまう不具合
		   if (mwork[0] == '\r' || mwork[0] == '\n')
             fprintf(fpd, "%s", mwork);
		   else
#endif
            fprintf(fpd, "%s\n", mwork);
            //fputs(mwork, fpd);
		  } else {
            fprintf(fpd, "> %s\n", mwork);
			strcpy(mwk, mwork);
		    GetToken(mwork, &pML, &pAddr);
#ifdef UPDATE_20130819 // ML名を指定した制御命令を無効にするオプションを追加
			if (pML &&
				(stricmp(mwork, "JOIN") == 0 ||
				 stricmp(mwork, "LEAVE") == 0 ||
                 stricmp(mwork, "SUBSCRIBE") == 0 ||
                 stricmp(mwork, "UNSUBSCRIBE") == 0) )
			{
              if (bMCM && strlen(pML) > 0)
			  {
			    strcpy(mML, rcp->mTo);
				if ((pL = strstr(mML, "@")))
				{
				  *pL = '\x0';
				}
				if ((pL2 = strstr(pML, "@")))
				{
				  *pL2 = '\x0';
				}
			    if (strlen(mML) > 8) 
				{
			 	  mML[strlen(mML)-8] = '\x0';
				  if (stricmp(pML, mML))
				  {
				    fprintf(fpd, "Sorry. Invalid listname.\n\n");
					if (pL2)
					{
					  *pL2 = '@';
					}
				    break;
				  }
				}
				if (pL2)
				{
				  *pL2 = '@';
				}
			  }
			}
#endif
	        if (stricmp(mwork, "HELP") == 0) {
		      fprintf(fpd, "\n");
              fprintf(fpd, "This mailing list processor understands the following commands:\n\n");
              fprintf(fpd, "HELP\nProduces this message\n\n");
#ifdef UPDATE_20060513 // ＭＬ管理メールにキーコードを付加
			  fprintf(fpd, "KEYGEN\nThe key generation of the 'join(subscribe)' and the 'leave(unsubscribe)' for the moderator.\n(moderator only).\n\n");
#endif
              fprintf(fpd, "JOIN [listname [address]]\nSubscribe to mailing list\n\n");
              fprintf(fpd, "LEAVE [listname [address]]\nUnsubscribe from mailing list\n\n");
              fprintf(fpd, "STOP\nStop processing commands (eg to avoid processing your signature)\n\n");
              fprintf(fpd, "SUBSCRIBE [listname [address]]\nSubscribe to mailing list\n\n");
              fprintf(fpd, "UNSUBSCRIBE [listname [address]]\nUnsubscribe from mailing list\n\n");
              fprintf(fpd, "LIST\nRequires a member list(moderator only).\n\n");
#ifdef V3
              fprintf(fpd, "ARTICLE [index [ALL or begin(.-end no.)]]\nGet article index.\n\n");
              fprintf(fpd, "ARTICLE [list [ALL or begin(-end no.)]]\nGet article list.\n\n");
              fprintf(fpd, "ARTICLE [delete [ALL begin(-end no.)]]\nDelete article.(moderator only).\n\n");
#endif
              fprintf(fpd, "Commands should be placed in the body of the mail message.\n\n");
              fprintf(fpd, "If you have problems with this mailing list processor, you\n");
			  if (mMDR[0] != '\x0')
			    fprintf(fpd, "may obtain help from a human by mailing %s\n", mMDR);
			  else
                fprintf(fpd, "may obtain help from a human by mailing postmaster@%s\n",rcp->mDomain); //rcp->mSmtp);
			} else if (stricmp(mwork, "STOP") == 0) {
              fprintf(fpd, "End of commands detected\n");
			  break;
			}
#ifdef UPDATE_20060513 // ＭＬ管理メールにキーコードを付加
			else if (stricmp(mwork, "KEYGEN") == 0) {
		      if (stricmp(mMDR, rcp->mFrom) == 0) {
                fprintf(fpd, "\nEnter the following code in the subject and send a command mail.\n[no:%s]\n", mB64);
	            if ((p = strrchr((bReservedWords2 ? &mUser[strlen(mReservedWords)] : mUser), '@')))
	              *p = '\x0';
			    fprintf(fpd, "\nOr, \nclick the following mailto code and send a command mail.\nmailto:%s-request@%s?subject=[no:%s]\n", (bReservedWords2 ? &mUser[strlen(mReservedWords)] : mUser), rcp->mDomain, mB64);
				if (p)
	              *p = '@';
			  } else {
 	            fprintf(fpd, "\nKEYGEN command is moderator only.\n\n");
			  }
			  break;
			}
#endif
			else if (stricmp(mwork, "JOIN") == 0 ||
                       stricmp(mwork, "SUBSCRIBE") == 0) {

			  if (!pML) {
			    strcpy(mML, rcp->mTo);
                //sprintf(mML, "%s-%s", rcp->mMNo, rcp->mTo);
				pL = strstr(mML, "@");
				if (pL) {
				  *pL = '\x0';
				  pL++;
				}
			    //strtok(mML, "@");
			    if (strlen(mML) > 8) {
			  	  mML[strlen(mML)-8] = '\x0';
				  pML = mML;
				}
				strcpy(mL, pML);
				if (pL) {
				  strcat(mL, "@");
				  strcat(mL, pL);
				}
			  } else {
			    if (strlen(pML) == 0)
				{
			      strcpy(mML, rcp->mTo);
				  //sprintf(mML, "%s-%s", rcp->mMNo, rcp->mTo);
				  pL = strstr(mML, "@");
				  if (pL) {
				    *pL = '\x0';
				    pL++;
				  }
			      //strtok(mML, "@");
			      if (strlen(mML) > 8) {
			 	    mML[strlen(mML)-8] = '\x0';
				    pML = mML;
				  }
				  strcpy(mL, pML);
				  if (pL) {
				    strcat(mL, "@");
				    strcat(mL, pL);
				  }
				} else {
				  strcpy(mL, pML);
				}
			  }
			  pML = mL;
			  if (!strstr(pML, "@")) {
				strcat(pML, "@");
				strcat(pML, rcp->mDomain);
			  }
			  if (!pAddr) {
			    pAddr = rcp->mFrom;
			  }
#ifdef UPDATE_20060513 // ＭＬ管理メールにキーコードを付加
			  if (!bMCJ || 
				  (bMCJ && stricmp(mMDR, rcp->mFrom) == 0 && (nPWChk == 0 || (nPWChk != 0 && bPWMach)))
				  ) 
#else
			  if (!bMCJ || 
				  (bMCJ && stricmp(mMDR, rcp->mFrom) == 0)
				  ) 
#endif
			  {
			    nsts = QueryLists(TRUE, pML, pAddr); // ドメイン付で確認
				if (nsts == -1) {                    // 見つからない場合は、
#ifdef UPDATE_20060716A // ML制御の結果メールにML名に付加されているドメイン名が削除されてしまう。
				  if ((pAt = strchr(pML, '@')))
					*pAt = '\x0';
#else
				  strtok(pML, "@");
#endif
			      nsts = QueryLists(TRUE, pML, pAddr); // ドメイン無しで確認
#ifdef UPDATE_20060716A // ML制御の結果メールにML名に付加されているドメイン名が削除されてしまう。
				  if (pAt)
					*pAt = '@';
#endif
				}
			    switch (nsts) {
			      case 0: sprintf(SubKey2, "%s\\%s\\Members\\%s", SOFT_LISTS_REG, pML, pAddr);
					      //sprintf(SubKey2, "%s\\Members\\%s", SubKey, pAddr);
#ifdef REGTOFILE
					      if (nClustering && !strnicmp(SubKey2, "software\\emwac", 14)) {
#ifdef UPDATE_20070521 // 予約語対策
                             strcpy(mML, pAddr);
                             strtok(mML, "@");
                             if (ReservedWords(mML)) {
                               strcpy(mML, mReservedWords);
                               strcat(mML, pAddr);
		                       sprintf(SubKey2, "%s\\%s\\Members\\%s", SOFT_LISTS_REG, pML, mML);
							 }
#endif
						    retCode = FileCreateKey(mMailSpoolDir, SubKey2);
						  } else
#endif
						  {
                          retCode = RegCreateKey(hKeyRoot, SubKey2, &hKey); 
 		                  RegCloseKey((HKEY)hKey);
						  }
#ifdef UPDATE_20071210A // ML参加アドレスを含む256Byte超のフォルダ名になったと場合の対策
						   if (strlen(pAddr) < 256) {
                             if (!QueryLists(FALSE, pML, pAddr)) {
						       bNewML = TRUE;
							   sprintf(mMLFn, "%sreg\\%s\\%s\\Members\\ext.dat", mMailSpoolDir, SOFT_LISTS_REG, pML);
							   /// アドレスが登録済みか確認
		                       if ((fp1 = fopen(mMLFn, "rt"))) {
		                         while (fgets(mMLAddr, sizeof(mMLAddr), fp1)) {
			                       strtok(mMLAddr, "\r\n");
			                       if (!stricmp(mMLAddr, pAddr)) {
							         bNewML = FALSE;
			                         break;
								   }
								 }
                                 fclose(fp1);
							   }
							   if (bNewML) {
				                 retCode = ERROR_SUCCESS;
		                         if ((fp1 = fopen(mMLFn, "at"))) {
							       fprintf(fp1, "%s\n", pAddr);
                                   fclose(fp1);
								 }
							   }
							 }
						   }
						   if (strlen(pAddr) >= 256)
							 fprintf(fpd, "'%s' is too long (255 bytes max).\n\n", pAddr);
		                   else if (retCode == ERROR_SUCCESS)
                             fprintf(fpd, "'%s' has been added to the %s mailing list.\n\n", pAddr, pML);
		                   else
		                     fprintf(fpd, "\nThere is no mailing list called %s.\n\n", pML);
						   break;
#else
		                   if (retCode == ERROR_SUCCESS)
                             fprintf(fpd, "'%s' has been added to the %s mailing list.\n\n", pAddr, pML);
		                   else
		                     fprintf(fpd, "\nThere is no mailing list called %s.\n\n", pML);
						   break;
#endif
                  case 1:  fprintf(fpd, "'%s' is already a member of the %s mailing list.\n\n", pAddr, pML);
				  	       break;
				  case 2:  fprintf(fpd, "\nIt found an invalid character in address '%s'.\n\n", pAddr);
						   break;
				  default: fprintf(fpd, "\nThere is no mailing list called %s.\n\n", pML);
					       break;
				}
			  } else {
#ifdef UPDATE_20060513 // ＭＬ管理メールにキーコードを付加
				if (!stricmp(mMDR, rcp->mFrom)) {
				  fprintf(fpd, "\nThere is not a 'KEYGEN'.\n\n");
				} else
#endif
				{
			      fprintf(fpd, "\nYour request to added '%s' to the %s list\n", pAddr, pML);
			      fprintf(fpd, "has been forwarded to the list moderator.\n\n");
				  ModeratorRequest(rcp, mMDR, mwk, mt, 0);
				}
			  }
			} else if (stricmp(mwork, "LEAVE") == 0 ||
		               stricmp(mwork, "UNSUBSCRIBE") == 0) {
			  if (!pML) {
			    strcpy(mML, rcp->mTo);
				//sprintf(mML, "%s-%s", rcp->mMNo, rcp->mTo);
				pL = strstr(mML, "@");
				if (pL) {
				  *pL = '\x0';
				  pL++;
				}
			    //strtok(mML, "@");
			    if (strlen(mML) > 8) {
			  	  mML[strlen(mML)-8] = '\x0';
				  pML = mML;
				}
				strcpy(mL, pML);
				if (pL) {
				  strcat(mL, "@");
				  strcat(mL, pL);
				}
			  } else {
			    if (strlen(pML) == 0) {
			      strcpy(mML, rcp->mTo);
				  //sprintf(mML, "%s-%s", rcp->mMNo, rcp->mTo);
				  pL = strstr(mML, "@");
				  if (pL) {
				    *pL = '\x0';
				    pL++;
				  }
			      //strtok(mML, "@");
			      if (strlen(mML) > 8) {
			 	    mML[strlen(mML)-8] = '\x0';
				    pML = mML;
				  }
				  strcpy(mL, pML);
				  if (pL) {
				    strcat(mL, "@");
				    strcat(mL, pL);
				  }
				} else {
				  strcpy(mL, pML);
				}
			  }
			  pML = mL;
			  if (!strstr(pML, "@")) {
				strcat(pML, "@");
				strcat(pML, rcp->mDomain);
			  }
			  if (!pAddr) {
			    pAddr = rcp->mFrom;
			  }
#ifdef UPDATE_20060513 // ＭＬ管理メールにキーコードを付加
			  if (!bMCJ || 
				  (bMCJ && stricmp(mMDR, rcp->mFrom) == 0 && (nPWChk == 0 || (nPWChk != 0 && bPWMach)))
				  )
#else
			  if (!bMCL || 
				  (bMCL && stricmp(mMDR, rcp->mFrom) == 0)
				  )
#endif
			  {
			    nsts = QueryLists(FALSE, pML, pAddr);
				if (nsts == -1) {                    // 見つからない場合は、
#ifdef UPDATE_20060716A // ML制御の結果メールにML名に付加されているドメイン名が削除されてしまう。
				  if ((pAt = strchr(pML, '@')))
					*pAt = '\x0';
#else
				  strtok(pML, "@");
#endif
			      nsts = QueryLists(TRUE, pML, pAddr); // ドメイン無しで確認
#ifdef UPDATE_20060716A // ML制御の結果メールにML名に付加されているドメイン名が削除されてしまう。
				  if (pAt)
					*pAt = '@';
#endif
				}
			    switch (nsts) {
                  case 0: fprintf(fpd, "'%s' is not a member of the '%s' mailing list.\n\n", pAddr, pML);
					      break;
			      case 1: sprintf(SubKey2, "%s\\%s\\Members\\%s", SOFT_LISTS_REG, pML, pAddr);
					      //sprintf(SubKey2, "%s\\Members\\%s", SubKey, pAddr);
#ifdef REGTOFILE
					      if (nClustering && !strnicmp(SubKey2, "software\\emwac", 14)) {
#ifdef UPDATE_20070521 // 予約語対策
                             strcpy(mML, pAddr);
                             strtok(mML, "@");
                             if (ReservedWords(mML)) {
                               strcpy(mML, mReservedWords);
                               strcat(mML, pAddr);
		                       sprintf(SubKey2, "%s\\%s\\Members\\%s", SOFT_LISTS_REG, pML, mML);
							 }
#endif
						    retCode = KeyFileDeleteKey(mMailSpoolDir, SubKey2);
						  } else
#endif
						  {
                          retCode = RegDeleteKey(hKeyRoot, SubKey2); 
						  }
 		                  //RegCloseKey((HKEY)hKey);
#ifdef UPDATE_20071210A // ML参加アドレスを含む256Byte超のフォルダ名になったと場合の対策
						  if (strlen(pAddr) < 256) {
                            if (QueryLists(FALSE, pML, pAddr)) {
							  bNewML = TRUE;
							  sprintf(mMLFn, "%sreg\\%s\\%s\\Members\\ext.dat", mMailSpoolDir, SOFT_LISTS_REG, pML);
							  sprintf(mMLFn2, "%sreg\\%s\\%s\\Members\\~ext.dat", mMailSpoolDir, SOFT_LISTS_REG, pML);
							  /// アドレスが登録済みか確認
		                      if ((fp1 = fopen(mMLFn, "rt"))) {
		                        if ((fp2 = fopen(mMLFn2, "wt"))) {
		                          while (fgets(mMLAddr, sizeof(mMLAddr), fp1)) {
			                        strtok(mMLAddr, "\r\n");
			                        if (stricmp(mMLAddr, pAddr)) {
								      fprintf(fp2, "%s\n", mMLAddr);
									}
								  }
								  fflush(fp2);
                                  fclose(fp2);
								}
                                fclose(fp1);
							    CopyFile(mMLFn2, mMLFn, FALSE);
							    DeleteFile(mMLFn2);
							  }
							}
			                retCode = ERROR_SUCCESS;
						  }
						  if (strlen(pAddr) >= 256)
							 fprintf(fpd, "'%s' is too long (255 bytes max).\n\n", pAddr);
		                  else if (retCode == ERROR_SUCCESS)
                            fprintf(fpd, "'%s' has been a removed from the '%s' mailing list.\n\n", pAddr, pML);
		                  else
		                    fprintf(fpd, "\nThere is no mailing list called '%s'.\n\n", pML);
						  break;
#else
		                  if (retCode == ERROR_SUCCESS)
                            fprintf(fpd, "'%s' has been a removed from the '%s' mailing list.\n\n", pAddr, pML);
		                  else
		                    fprintf(fpd, "\nThere is no mailing list called '%s'.\n\n", pML);
						  break;
#endif
				   //case 2:fprintf(fpd, "\nIt found an invalid character in address '%s'.\n\n", pAddr);
						  break;
				 default: fprintf(fpd, "\nThere is no mailing list called %s.\n\n", pML);
				 	      break;
				}
			  } else {
#ifdef UPDATE_20060513 // ＭＬ管理メールにキーコードを付加
				if (!stricmp(mMDR, rcp->mFrom)) {
				  fprintf(fpd, "\nThere is not a 'KEYGEN'.\n\n");
				} else 
#endif
				{
			      fprintf(fpd, "\nYour request to remove '%s' from the %s list\n", pAddr, pML);
			      fprintf(fpd, "has been forwarded to the list moderator.\n\n");
				  ModeratorRequest(rcp, mMDR, mwk, mt, 1);
				}
			  }
			} else if (stricmp(mwork, "LIST") == 0) {
#ifdef UPDATE_20060513 // ＭＬ管理メールにキーコードを付加
			  if ((stricmp(mMDR, rcp->mFrom) == 0 && (nPWChk == 0 || (nPWChk != 0 && bPWMach))))
#else
			  if (stricmp(mMDR, rcp->mFrom) == 0)
#endif
			  {
			    if (!pML) {
			      strcpy(mML, rcp->mTo);
				  //sprintf(mML, "%s-%s", rcp->mMNo, rcp->mTo);
				  pL = strstr(mML, "@");
				  if (pL) {
				    *pL = '\x0';
				    pL++;
				  }
			      //strtok(mML, "@");
			      if (strlen(mML) > 8) {
			  	    mML[strlen(mML)-8] = '\x0';
				    pML = mML;
				  }
				  strcpy(mL, pML);
				  if (pL) {
				    strcat(mL, "@");
				    strcat(mL, pL);
				  }
				} else {
			      if (strlen(pML) == 0) {
 			        strcpy(mML, rcp->mTo);
				    //sprintf(mML, "%s-%s", rcp->mMNo, rcp->mTo);
				    pL = strstr(mML, "@");
				    if (pL) {
				      *pL = '\x0';
				      pL++;
					}
			        //strtok(mML, "@");
			        if (strlen(mML) > 8) {
			 	      mML[strlen(mML)-8] = '\x0';
				      pML = mML;
					}
				    strcpy(mL, pML);
				    if (pL) {
				      strcat(mL, "@");
				      strcat(mL, pL);
					}
				  } else {
				    strcpy(mL, pML);
				  }
				}
			    pML = mL;
			    if (!strstr(pML, "@")) {
				  strcat(pML, "@");
				  strcat(pML, rcp->mDomain);
				}
			    if (!pAddr) {
			      pAddr = rcp->mFrom;
				}
 	            fprintf(fpd, "\nThe list of '%s'.\n\n", pML);
	            GetMLMembers(fpd, pML);
			  } else {
#ifdef UPDATE_20060513 // ＭＬ管理メールにキーコードを付加
				if (!stricmp(mMDR, rcp->mFrom)) {
				  fprintf(fpd, "\nThere is not a 'KEYGEN'.\n\n");
				} else
#endif
				{
				  fprintf(fpd, "\nLIST command is moderator only.\n\n", mwork);
				}
			  }
#ifdef V3
			} else if (stricmp(mwork, "ARTICLE") == 0) { // 記事の取り出し
			  if (pML) {
			    strcpy(mML, rcp->mTo);
				//sprintf(mML, "%s-%s", rcp->mMNo, rcp->mTo);
				pL = strstr(mML, "@");
				if (pL) {
				  *pL = '\x0';
				  pL++;
				}
			    if (strlen(mML) > 8)
			      mML[strlen(mML)-8] = '\x0';
				if (pL) {
				  strcat(mML, "@");
				  strcat(mML, pL);
				}
			    if (stricmp(pML, "index") == 0) {
			      if (QueryLists(TRUE, mML, rcp->mFrom) == 1 ||
					  stricmp(mMDR, rcp->mFrom) == 0){ // メンバーチェック
			        GetML_ARTICLE_INDEX(fpd, mML, pAddr);
 	                fprintf(fpd, "ARTICLE INDEX command completed.\n\n");
				  } else {
 	                fprintf(fpd, "\nARTICLE LIST command is member or moderator only.\n\n");
				  }
				} else if (stricmp(pML, "list") == 0) {
			      if (QueryLists(TRUE, mML, rcp->mFrom) == 1 ||
					stricmp(mMDR, rcp->mFrom) == 0){ // メンバーチェック
                    sprintf(mwk, "%slists\\%s", mMailSpoolDir, rcp->mTo);
					//sprintf(mwk, "%slists\\%s-%s", mMailSpoolDir, rcp->mMNo, rcp->mTo);
                    //sprintf(mwk, "%slists\\%s-request", mMailSpoolDir, mML);
#ifdef UPDATE_20070521 // 予約語対策
				    if (nClustering) {
                      strcpy(mUID, rcp->mTo);
                      strtok(mUID, "@");
                      if (ReservedWords(mUID)) {
                        strcpy(mUID, mReservedWords);
                        strcat(mUID, rcp->mTo);
                        sprintf(mwk, "%slists\\%s", mMailSpoolDir, mUID);
					  }
					}
#endif
   	                GetML_ARTICLE_LIST(rcp, fpd, mwk, rcp->mMNo, mML, pAddr);
 	                fprintf(fpd, "ARTICLE LIST command completed.\n\n");
				  } else {
 	                fprintf(fpd, "\nARTICLE LIST command is member or moderator only.\n\n");
				  }
				} else if (stricmp(pML, "delete") == 0) {
#ifdef UPDATE_20060513 // ＭＬ管理メールにキーコードを付加
			      if ((stricmp(mMDR, rcp->mFrom) == 0 && (nPWChk == 0 || (nPWChk != 0 && bPWMach))))
#else
			      if (stricmp(mMDR, rcp->mFrom) == 0)
#endif
				  {
			        GetML_ARTICLE_DELETE(fpd, mML, pAddr);
 	                fprintf(fpd, "ARTICLE DELETE command completed.\n\n");
				  } else {
#ifdef UPDATE_20060513 // ＭＬ管理メールにキーコードを付加
				    if (!stricmp(mMDR, rcp->mFrom)) {
				      fprintf(fpd, "\nThere is not a 'KEYGEN'.\n\n");
					} else
#endif
					{
 	                  fprintf(fpd, "\nARTICLE DELETE command is moderator only.\n\n");
					}
				  }
				} else {
 	             fprintf(fpd, "\nCommand %s not recognised\n\n", mwork);
				}
			  } else {
                fprintf(fpd, "\nARTICLE direction is the following form.\n\n", mwork);
                fprintf(fpd, "ARTICLE [index [ALL or begin(.-end no.)]]\nGet article index.\n\n");
                fprintf(fpd, "ARTICLE [list [ALL or begin(-end no.)]]\nGet article list.\n\n");
                fprintf(fpd, "ARTICLE [delete [ALL begin(-end no.)]]\nDelete article.(moderator only).\n\n");
			  }
#endif
			} else {
 	          fprintf(fpd, "\nCommand %s not recognised\n\n", mwork);
			}
		  }
		}
	    mwork[0] = '\x0';
 	    p = fgets(mwork, sizeof(mwork), fp);
	  }
	  fclose(fp);
	  //fprintf(fpd, "\n");
	}
    fflush(fpd);
    fclose(fpd);
  }
#ifdef UPDATE_20240127 // ＭＬ制御応答＆送信エラー通知メールにもDKIMサインを追加可能にするオプション
#ifdef UPDATE_20240127C // DKIMサイン実施無効フラグを追加 1:無効(bit0:送信エラー bit1:ＭＬ制御応答 bit2:転送メール bit:3自動応答)
  if (bDKIM && (nOnDKIM & 0x2)) // ＭＬ制御応答
#else
  if (bDKIM)
#endif
    AddDKIMHeader(mMSG, rcp->mDomain);
#endif

  strcpy(mwork, rcp->mTo);
  strcpy(rcp->mTo, rcp->mFrom);
  //strcpy(rcp->mFrom, mwork); エンベローブがrequestで送信先エラーだとメールループする。
  ///// 返却メールのアドレス指定
  //sprintf(SubKey, "%s\\%s", SOFT_LISTS_REG, mUser); 指定済み
  nBounceAction = GetProfileIntEx(SubKey, "BounceAction", (INT)0);    // default Discard
  switch (nBounceAction) {
    case 0: // Discard
 	      strcpy(rcp->mFrom, "");
		  break;
    case 1: // Return to Sender
	      break;
    case 2: // Send to
	      GetProfileStringEx(SubKey,"BouncesTo", "", rcp->mFrom, sizeof(rcp->mFrom));
		  break;
  }
  if (bNoReply) { // 結果応答はしない場合。
    _unlink(mMSG); //DeleteFile(mMSG);
  } else {
#ifndef UPDATE_20060616 // エイリアスの場合実アカウントに変換されてしまう。
#ifdef UPADTE_20040315 // エイリアスに関する応答
	strcpy(mName, rcp->mTo);
	pdom = strstr(mName, "@");
	if ((pdom = strstr(mName, "@")))
	  *pdom = '\x0';
#ifdef UPDATE_20070509 // エイリアス設定の実アドレスオプションを別データとして処理する
	if ((bsts = GetAliases((LPCTSTR)SOFT_ALIASES_REG, (char *)mName, (char *)pdom, NULL)))
#else
	if ((bsts = GetAliases((LPCTSTR)SOFT_ALIASES_REG, (char *)mName, (char *)pdom)))
#endif
	{
	  if (bDebug)
         printf("\t-> aliases = %s\n", mName);
	  strcpy(rcp->mTo, mName); // 置換え
	} else {
	  if (pdom)
	   *pdom = '@';
	}
#endif
#endif
    SetRCPFile(rcp, mRCP, FALSE, NULL);
  }
#ifdef UPDATE_20050123
  CloseHandle(hFile);
#endif
  //SendGlobalMail(rcp);
}

DWORD GetMLMembers(FILE *fp, char *pML)
{
  HKEY hKeyRoot = HKEY_LOCAL_MACHINE;
  HKEY hKey;
  CHAR mkey[256];
  DWORD  cbValueName = 128;
  DWORD  retCode;
  DWORD  dwIndex = 0;	         // index of subkey to enumerate 
  CHAR   lpName[_MAX_PATH];	 // address of buffer for subkey name 
  DWORD  lpcbName = 0;
  int    i = 0;
#ifdef REGTOFILE
  HANDLE hFile;
#endif
#ifdef UPDATE_20070521 // 予約語対策
  char *p, *q, mML[512];
#endif
#ifdef UPDATE_20071210A // ML参加アドレスを含む256Byte超のフォルダ名になったと場合の対策
  FILE *fp1;
  char mMLFn[256];
#endif

   // OPEN THE KEY.
//  sprintf(mkey,PROFILE_ROOT_TREE,lpAppName);
  sprintf(mkey, "%s\\%s\\Members", SOFT_LISTS_REG, pML);
   do {
	 if (i > 0)
	  _sleep(0);
#ifdef REGTOFILE
	 if (nClustering && !strnicmp(mkey, "software\\emwac", 14)) {
#ifdef UPDATE_20070521 // 予約語対策
       strcpy(mML, pML);
       strtok(mML, "@");
       if (ReservedWords(mML)) {
         strcpy(mML, mReservedWords);
         strcat(mML, pML);
         sprintf(mkey, "%s\\%s\\Members", SOFT_LISTS_REG, mML);
	   }
#endif
	   strcat(mkey, "\\");
       retCode = 
         OpenKeyFile(mMailSpoolDir,
		             mkey,
					 "",
					 &hFile);

	 } else {
#endif
    retCode = 
      RegOpenKeyEx(hKeyRoot,       // Key handle at root level.
                   (LPCTSTR)mkey,  // Path name of child key.
                   0,              // Reserved.
                   KEY_READ,       // Requesting read access.
                   &hKey);         // Address of key to be returned.
#ifdef REGTOFILE
	 }
#endif
  } while(retCode == ERROR_INVALID_HANDLE && i++ < 3);  // ERROR_INVALID_HANDLE = 6
 
  if (retCode != ERROR_SUCCESS && strstr(mkey, "@")) { // ドメイン付が無い場合
	strtok(mkey, "@");
	strcat(mkey, "\\Members");
    do {
	  if (i > 0)
	    _sleep(0);
#ifdef REGTOFILE
	 if (nClustering && !strnicmp(mkey, "software\\emwac", 14)) {
	   strcat(mkey, "\\");
       retCode = 
         OpenKeyFile(mMailSpoolDir,
		             mkey,
					 "",
					 &hFile);

	 } else {
#endif
      retCode = 
        RegOpenKeyEx(hKeyRoot,       // Key handle at root level.
                   (LPCTSTR)mkey,  // Path name of child key.
                   0,              // Reserved.
                   KEY_READ,       // Requesting read access.
                   &hKey);         // Address of key to be returned.
#ifdef REGTOFILE
	 }
#endif
    } while(retCode == ERROR_INVALID_HANDLE && i++ < 3);  // ERROR_INVALID_HANDLE = 6
  }
  if (retCode == ERROR_SUCCESS) {
#ifdef UPDATE_20071210A // ML参加アドレスを含む256Byte超のフォルダ名になったと場合の対策
    sprintf(mMLFn, "%sreg\\%sext.dat", mMailSpoolDir, mkey);
	fp1 = fopen(mMLFn, "rt");
#endif
  // READ THE KEY DATA.
    do {
#ifdef REGTOFILE
	 if (nClustering && !strnicmp(mkey, "software\\emwac", 14)) {
       retCode =
		 KeyFileEnumKey(mMailSpoolDir,
		                mkey,
						dwIndex, 
						(LPTSTR)lpName, 
						(unsigned long *)&lpcbName);
#ifdef UPDATE_20071210A // ML参加アドレスを含む256Byte超のフォルダ名になったと場合の対策
	    if (fp1 && retCode != ERROR_SUCCESS) {
		  lpName[0] = '\x0';
		  fgets(lpName, sizeof(lpName), fp1);
          if (lpName[0] && lpName[0] != '\r' && lpName[0] != '\n') {
			strtok(lpName, "\r\n");
		    retCode = ERROR_SUCCESS;
		  }
		}
#endif
#ifdef UPDATE_20070521 // 予約語対策
        if (retCode == ERROR_SUCCESS) {
          strcpy(mML, lpName);
          strtok(mML, "@");
          if (!strnicmp(mML, mReservedWords, strlen(mReservedWords))
		      && ReservedWords(&mML[strlen(mReservedWords)])) {
            if ((q = strchr(lpName, '@'))) {
	          strcpy(mML, lpName+strlen(mReservedWords));
		      strcpy(lpName, mML);
			}
		  }
		}
#endif
	 } else {
#endif
      retCode =
          RegEnumKey(hKey,	        // handle of key to query 
                     dwIndex,	    // index of subkey to query 
                     (LPTSTR)lpName, // address of buffer for subkey name  
                     (unsigned long)&lpcbName 	    // size of subkey buffer 
         );
#ifdef REGTOFILE
	 }
#endif
      if (retCode == ERROR_SUCCESS) {
        dwIndex++;
		if (fp)
		  fprintf(fp, "%s\n", lpName);
	  }
	} while (retCode == ERROR_SUCCESS);
#ifdef UPDATE_20071210A // ML参加アドレスを含む256Byte超のフォルダ名になったと場合の対策
	if (fp1) {
      fclose(fp1);
	}
#endif
#ifdef REGTOFILE
   if (nClustering && !strnicmp(mkey, "software\\emwac", 14))
 	 CloseHandle(hFile);
   else
#endif
    RegCloseKey(hKey);
  }
  fprintf(fp, "\nIt was %ld in amount.\n", dwIndex);
  return dwIndex;
}

void GetML_ARTICLE_INDEX(FILE *fp, char *pML, char *pWhere) {
  char mFirst[256], mEnd[256];
  char mKey[256], mDir[256], mTkn[256], *p, *ps;
  FILE *fpidx;
  BOOL bAll = FALSE;
  BOOL bGet = FALSE;
#ifdef UPADTE_20040422
  DWORD dwStart = 0, dwEnd = 0, dwCurrent = 0;
  char  *q;
#endif
#ifdef UPDATE_20070521 // 予約語対策
  char mML[512];
#endif
#ifdef UPDATE_20210303A // 保存する拡張子の指定
  char mExte[16];
#endif

    sprintf(mKey,"%s\\%s", SOFT_LISTS_REG, pML);
#ifdef UPDATE_20070521 // 予約語対策
    strcpy(mML, pML);
    strtok(mML, "@");
    if (ReservedWords(mML)) {
      strcpy(mML, mReservedWords);
      strcat(mML, pML);
      sprintf(mKey,"%s\\%s", SOFT_LISTS_REG, mML);
	}
#endif

#ifdef MULTI_ML
   if (!QueryMLists((LPCTSTR)SOFT_LISTS_REG, pML)) // ドメイン付MLが無い場合
     strtok(mKey,"@");
#else
   strtok(mKey,"@");
#endif

#ifdef UPDATE_20210303A // メーリングリストで投稿内容保存ファイルの拡張子の指定を可能にするオプションを追加した。
   mExte[0] =' .';
   GetProfileStringEx(mKey,"ListSaveExtension", "TXT", &mExte[1], sizeof(mExte)-2);
   strcat(mExte, ":");
#endif
   GetProfileStringEx(mKey,"ListSaveFolder", "", mDir, sizeof(mDir));
   if (mDir[0] == '\x0'||                                // 記事フォルダーが指定されていない場合。
	   !GetProfileIntEx(mKey, "ListSave", (INT)FALSE)) { // 記事の記録をしていない場合。
     fprintf(fp, "\nArticle is empty.\n");
	 return;
   }
   GetProfileStringEx(mKey, "MlToken", "", mTkn, sizeof(mTkn));

   if (!pWhere) { // NULLならALL
	 bAll = TRUE;
   } else if (stricmp(pWhere,"ALL") == 0) {
	 bAll = TRUE;
   } else if (strstr(pWhere,"-")) {
	 p = strstr(pWhere,"-");
	 *p = '\x0';
	 sprintf(mFirst, "%s:", pWhere);
#ifdef UPADTE_20040422
	 dwStart = atol(pWhere);
#endif
     p++;
	 mEnd[0] = '\x0';
	 if (*p != '\x0') {
	   sprintf(mEnd, "%s:", p);
#ifdef UPADTE_20040422
	   dwEnd = atol(p);
#endif
	 }
   } else {
	 sprintf(mFirst, "%s:", pWhere);
	 strcpy(mEnd, mFirst);
#ifdef UPADTE_20040422
	 dwStart = dwEnd = atol(pWhere);
#endif
   }
   fprintf(fp, "\nArticle index.\n");
   //strcat(mDir, "index.html");
   strcat(mDir, "index.tbl");
   fpidx = fopen(mDir, "rt");
   if (fpidx) {
	  p = fgets(mTkn, sizeof(mTkn), fpidx);
	  while(p || !feof(fpidx)) {
#ifdef UPADTE_20040422
#ifdef UPDATE_20210303A // メーリングリストで投稿内容保存ファイルの拡張子の指定を可能にするオプションを追加した。
	    if ((q = strstr(mTkn, mExte))) 
#else
		if ((q = strstr(mTkn, ".TXT:"))) 
#endif
		{
		  q += 5;
		  while(*q == ' ')  // Noの先頭
		    q++;
          dwCurrent = atol(q); // 現行の番号取得
		}
#endif
	  	if (!bGet) {
#ifdef UPADTE_20040422
		  if (bAll || dwCurrent >= dwStart)
#else
		  if (bAll || strstr(mTkn, mFirst))
#endif
		    bGet = TRUE;   // 出力開始
		}
		if (mEnd[0] != '\x0') {
#ifdef UPADTE_20040422
		  if (!bAll && dwCurrent > dwEnd)
#else
		  if (!bAll && strstr(mTkn, mEnd))
#endif
		    bGet = FALSE;  // 出力終了
		}
	    if (bGet) {
		  ps = strstr(mTkn,":");
#ifdef UPDATE_20210303 // メーリングリストの投稿内容保存時、題名が複数行にまたがるとインデックス取得でハングする
		  if (ps) 
		  {
#else
		  if (ps)
#endif
		    ps++;
		  while(*ps == ' ')
			ps++;
		  fprintf(fp, " no.%s", ps); // メールに書込み
#ifdef UPDATE_20210303 // メーリングリストの投稿内容保存時、題名が複数行にまたがるとインデックス取得でハングする
		  } else {
            fprintf(fp, "%s", mTkn); // メールに書込み
		  }
#endif

		}
	    p = fgets(mTkn, sizeof(mTkn), fpidx);
	  }
	  fclose(fpidx);
   }
   fprintf(fp, "\nSend article index.\n");
}

void GetML_ARTICLE_LIST(struct _RCP *rcp, FILE *fp, char *mFolder, char *mFileNo, char *pML, char *pWhere) {
  char  mFirst[256], mEnd[256];
  char  mKey[256], mDir[256], mIndex[256], mTkn[256], *p, *ps;
  char  mMSGSrc[256], mMSGDest[256];
  char  mRCPSrc[256], mRCPDest[256];
  char  mwork[256], mLKFile[256];
  DWORD n;
  FILE *fpidx, *fpms, *fpmd, *fplk;
  BOOL  bAll = FALSE;
  BOOL  bGet = FALSE;
#ifdef UPADTE_20040422
  DWORD dwStart = 0, dwEnd = 0, dwCurrent = 0;
  char  *q;
#endif
#ifdef REGTOFILE
   HANDLE hFile;
#endif
#ifdef UPDATE_20060904 // ロックファイルの作成確認
  long               hFindFile;
  struct _finddata_t FindFileData;
#endif
#ifdef UPDATE_20070521 // 予約語対策
  char mML[512];
#endif
#ifdef UPDATE_20210303A // 保存する拡張子の指定
  char mExte[16];
#endif

    sprintf(mKey,"%s\\%s", SOFT_LISTS_REG, pML);
#ifdef UPDATE_20070521 // 予約語対策
    strcpy(mML, pML);
    strtok(mML, "@");
    if (ReservedWords(mML)) {
      strcpy(mML, mReservedWords);
      strcat(mML, pML);
      sprintf(mKey,"%s\\%s", SOFT_LISTS_REG, mML);
	}
#endif

#ifdef MULTI_ML
   if (!QueryMLists((LPCTSTR)SOFT_LISTS_REG, pML)) // ドメイン付MLが無い場合
     strtok(mKey,"@");
#else
   strtok(mKey,"@");
#endif

#ifdef UPDATE_20210303A // メーリングリストで投稿内容保存ファイルの拡張子の指定を可能にするオプションを追加した。
   mExte[0] =' .';
   GetProfileStringEx(mKey,"ListSaveExtension", "TXT", &mExte[1], sizeof(mExte)-2);
   strcat(mExte, ":");
#endif
   GetProfileStringEx(mKey,"ListSaveFolder", "", mDir, sizeof(mDir));
   if (mDir[0] == '\x0'||                                // 記事フォルダーが指定されていない場合。
	   !GetProfileIntEx(mKey, "ListSave", (INT)FALSE)) { // 記事の記録をしていない場合。
     fprintf(fp, "\nArticle is empty.\n");
	 return;
   }
   GetProfileStringEx(mKey, "MlToken", "", mTkn, sizeof(mTkn));

   if (!pWhere) { // NULLならALL
	 bAll = TRUE;
   } else if (stricmp(pWhere,"ALL") == 0) {
	 bAll = TRUE;
   } else if (strstr(pWhere,"-")) {
	 p = strstr(pWhere,"-");
	 *p = '\x0';
	 sprintf(mFirst, "%s:", pWhere);
#ifdef UPADTE_20040422
	 dwStart = atol(pWhere);
#endif
     p++;
	 mEnd[0] = '\x0';
	 if (*p != '\x0') {
	   sprintf(mEnd, "%s:", p);
#ifdef UPADTE_20040422
	   dwEnd = atol(p);
#endif
	 }
   } else {
	 sprintf(mFirst, "%s:", pWhere);
	 strcpy(mEnd, mFirst);
#ifdef UPADTE_20040422
	 dwStart = dwEnd = atol(pWhere);
#endif
   }
   //sprintf(mIndex, "%sindex.html", mDir);
   sprintf(mIndex, "%sindex.tbl", mDir);
   n = 0;
   /////////////////////////////////////////////////
   // ロックファイル
#ifdef REGTOFILE
   sprintf(mLKFile, "%s\\%s", mFolder, LOCKFILE); 
   while ((hFile = CreateFile((LPCTSTR)mLKFile,
                        GENERIC_WRITE,
                        0,   // 排他アクセス = 0
                        NULL,
                        CREATE_ALWAYS,
                        FILE_ATTRIBUTE_NORMAL, 
                        NULL)) == INVALID_HANDLE_VALUE) {
      if (bServiceTerminating) 
        break;
       _sleep(WAIT_TIME);
   } 
#ifdef UPDATE_20060904 // ロックファイルの作成確認
   while ((hFindFile = _findfirst( mLKFile, &FindFileData)) == -1L) {
     if (bServiceTerminating) 
       break;
       _sleep(WAIT_TIME);
   }
   _findclose(hFindFile);
#endif
#else
#ifndef UPDATE_20050123
   sprintf(mLKFile, "%s\\%s", mFolder, LOCKFILE); 
   while((fplk = fopen(mLKFile, "rt"))) { // 対象ML展開フォルダをロック
     fclose(fplk);
     if (bServiceTerminating)
       break;
     _sleep(WAIT_TIME);
   }
   if ((fplk = fopen(mLKFile, "wt")))
     fclose(fplk);
   while(!(fplk = fopen(mLKFile, "rt"))) { // 対象ML展開フォルダをロック
     if (bServiceTerminating)
       break;
     _sleep(WAIT_TIME);
   }
   fclose(fplk);
#endif
#endif
   /////////////////////////////////////////////////
   sprintf(mRCPSrc, "%s\\%s.$CP", mFolder, mFileNo); // 配送先
   ////////////////////////////// 送信元、送信先入れ替え
   strcpy(mwork, rcp->mTo);
   strcpy(rcp->mTo, rcp->mFrom);
   strcpy(rcp->mFrom, mwork);
   //////////////////////////////
   SetRCPFile(rcp, mRCPSrc, FALSE, NULL);
   ////////////////////////////// 元に戻す
   strcpy(rcp->mFrom, rcp->mTo);
   strcpy(rcp->mTo, mwork);
   //////////////////////////////
   fpidx = fopen(mIndex, "rt");
   if (fpidx) {
#ifdef DATA_CASH
	  setvbuf(fpidx, rcp->mFsbuf, _IOFBF, sizeof(rcp->mFsbuf) );
#endif
	  p = fgets(mTkn, sizeof(mTkn), fpidx);
	  while(p || !feof(fpidx)) {
#ifdef UPADTE_20040422
#ifdef UPDATE_20210303A // メーリングリストで投稿内容保存ファイルの拡張子の指定を可能にするオプションを追加した。
	    if ((q = strstr(mTkn, mExte))) 
#else
		if ((q = strstr(mTkn, ".TXT:"))) 
#endif
		{
		  q += 5;
		  while(*q == ' ')  // Noの先頭
		    q++;
          dwCurrent = atol(q); // 現行の番号取得
		}
#endif
	  	if (!bGet) {
#ifdef UPADTE_20040422
		  if (bAll || dwCurrent >= dwStart)
#else
		  if (bAll || strstr(mTkn, mFirst))
#endif
		    bGet = TRUE;   // 出力開始
		}
		if (mEnd[0] != '\x0') {
#ifdef UPADTE_20040422
		  if (!bAll && dwCurrent > dwEnd)
#else
		  if (!bAll && strstr(mTkn, mEnd))
#endif
		    bGet = FALSE;  // 出力終了
		}
	    if (bGet) {
		  ps = strstr(mTkn,":");
		  if (ps) {
		    *ps = '\x0';
            sprintf(mMSGSrc, "%s\\%s", mDir, mTkn);
		    n++;
            sprintf(mMSGDest, "%s\\%s-A%04ld.MSG", mFolder, mFileNo, n);
            sprintf(mRCPDest, "%s\\%s-A%04ld.RCP", mFolder, mFileNo, n);
		    fpms = fopen(mMSGSrc, "rt");
		    if (fpms) {
#ifdef DATA_CASH
 	          setvbuf(fpms, rcp->mFrbuf, _IOFBF, sizeof(rcp->mFrbuf) );
#endif
			  if ((fpmd = fopen(mMSGDest, "wt"))) {
#ifdef DATA_CASH
 	            setvbuf(fpms, rcp->mFwbuf, _IOFBF, sizeof(rcp->mFwbuf) );
#endif
			    p = fgets(mwork, sizeof(mwork), fpms);
			    while(p || !feof(fpms)) {
			      fputs(mwork, fpmd);
			      p = fgets(mwork, sizeof(mwork), fpms);
				}
			    fflush(fpmd);
				fclose(fpmd);
			  }
              fclose(fpms);
  		      X_CopyFile(mRCPSrc, mRCPDest, FALSE);
			} // MSGが存在しないものはRCP作成しない
  		    //X_CopyFile(mRCPSrc, mRCPDest, FALSE);
			*ps = ':';
		  }
		}
	    p = fgets(mTkn, sizeof(mTkn), fpidx);
	  }
	  fclose(fpidx);
   }
   _unlink(mRCPSrc); //DeleteFile(mRCPSrc);
#ifdef REGTOFILE
   CloseHandle(hFile);
#endif
#ifndef UPDATE_20050123
   _unlink(mLKFile); //DeleteFile(mLKFile);
#endif
   fprintf(fp, "\nSend article list.\n");
}

void GetML_ARTICLE_DELETE(FILE *fp, char *pML, char *pWhere) {
  char mFirst[256], mEnd[256];
  char mKey[256], mDir[256], mIndex[256], mIdx2[256], mTkn[256]; //, mTkn2[256];
  char *p;
  char  mMSG[256];
  FILE *fpidx, *fpid2;
  BOOL bAll = FALSE;
  BOOL bGet = FALSE;
  BOOL bDel = FALSE;
#ifdef UPADTE_20040422
  DWORD dwStart = 0, dwEnd = 0, dwCurrent = 0;
  char  *q;
#endif
#ifdef UPDATE_20070521 // 予約語対策
  char mML[512];
#endif
#ifdef UPDATE_20210303A // 保存する拡張子の指定
  char mExte[16];
#endif

    sprintf(mKey,"%s\\%s", SOFT_LISTS_REG, pML);
#ifdef UPDATE_20070521 // 予約語対策
    strcpy(mML, pML);
    strtok(mML, "@");
    if (ReservedWords(mML)) {
      strcpy(mML, mReservedWords);
      strcat(mML, pML);
      sprintf(mKey,"%s\\%s", SOFT_LISTS_REG, mML);
	}
#endif
#ifdef MULTI_ML
   if (!QueryMLists((LPCTSTR)SOFT_LISTS_REG, pML)) // ドメイン付MLが無い場合
     strtok(mKey,"@");
#else
   strtok(mKey,"@");
#endif

#ifdef UPDATE_20210303A // メーリングリストで投稿内容保存ファイルの拡張子の指定を可能にするオプションを追加した。
   mExte[0] =' .';
   GetProfileStringEx(mKey,"ListSaveExtension", "TXT", &mExte[1], sizeof(mExte)-2);
   strcat(mExte, ":");
#endif
   GetProfileStringEx(mKey,"ListSaveFolder", "", mDir, sizeof(mDir));
   if (mDir[0] == '\x0'||                                // 記事フォルダーが指定されていない場合。
	   !GetProfileIntEx(mKey, "ListSave", (INT)FALSE)) { // 記事の記録をしていない場合。
     fprintf(fp, "\nArticle is empty.\n");
	 return;
   }
   GetProfileStringEx(mKey, "MlToken", "", mTkn, sizeof(mTkn));

   if (!pWhere) { // NULLならALL
	 bAll = TRUE;
   } else if (stricmp(pWhere,"ALL") == 0) {
	 bAll = TRUE;
   } else if (strstr(pWhere,"-")) {
	 p = strstr(pWhere,"-");
	 *p = '\x0';
	 sprintf(mFirst, "%s:", pWhere);
#ifdef UPADTE_20040422
	 dwStart = atol(pWhere);
#endif
     p++;
	 mEnd[0] = '\x0';
	 if (*p != '\x0') {
	   sprintf(mEnd, "%s:", p);
#ifdef UPADTE_20040422
	   dwEnd = atol(p);
#endif
	 }
   } else {
	 sprintf(mFirst, "%s:", pWhere);
	 strcpy(mEnd, mFirst);
#ifdef UPADTE_20040422
	 dwStart = dwEnd = atol(pWhere);
#endif
   }
   sprintf(mIndex, "%sindex.tbl", mDir);
   sprintf(mIdx2, "%sindex.~tbl", mDir);
   fpidx = fopen(mIndex, "rt");
   if (fpidx) {
      fpid2 = fopen(mIdx2, "wt");
	  if (fpid2) {
	    p = fgets(mTkn, sizeof(mTkn), fpidx);
	    while(p || !feof(fpidx)) {
#ifdef UPADTE_20040422
#ifdef UPDATE_20210303A // メーリングリストで投稿内容保存ファイルの拡張子の指定を可能にするオプションを追加した。
		  if ((q = strstr(mTkn, mExte))) 
#else
		  if ((q = strstr(mTkn, ".TXT:"))) 
#endif
		  {
		    q += 5;
		    while(*q == ' ')  // Noの先頭
		      q++;
            dwCurrent = atol(q); // 現行の番号取得
		  }
#endif
	  	  if (!bGet) {
#ifdef UPADTE_20040422
			if (bAll || dwCurrent >= dwStart)
#else
			if (bAll || strstr(mTkn, mFirst))
#endif
			{
		      bGet = TRUE;   // 出力開始
			  strcpy(mFirst, mTkn);
			  strtok(mFirst, ":");
			}
		  }
		  if (mEnd[0] != '\x0') {
#ifdef UPADTE_20040422
		    if (!bAll && dwCurrent > dwEnd)
#else
			if (!bAll && strstr(mTkn, mEnd))
#endif
		      bGet = FALSE;  // 出力終了
		  }
		  if (bGet) {
			p = strstr(mTkn, ":");
			if (p)
			  *p = '\x0';
			bDel = TRUE;
            sprintf(mMSG, "%s\\%s", mDir, mTkn);
  			if (p)
			  *p = ':';
			_unlink(mMSG); //DeleteFile(mMSG);
		  } else {
	        fputs(mTkn, fpid2);
		  }
	      p = fgets(mTkn, sizeof(mTkn), fpidx);
		}
	    fflush(fpid2);
		fclose(fpid2);
	  }
	  fclose(fpidx);
   }
   if (bDel) {
	 //X_CopyFile(mIdx2, mIndex, FALSE); // こちらの関数だとサイズが異なるファイルコピーが変になる。
	 CopyFile(mIdx2, mIndex, FALSE); // 上書きコピー
	 _unlink(mIdx2); //DeleteFile(mIdx2);
   }
/* htmlの行削除はしない。
   if (bDel) {
	 X_CopyFile(mIdx2, mIndex, FALSE); // 上書きコピー
	 _unlink(mIdx2); //DeleteFile(mIdx2);
	 bGet = FALSE;
	 /////////////////////
     sprintf(mIndex, "%sindex.html", mDir);
     sprintf(mIdx2, "%sindex.~tml", mDir);
     fpidx = fopen(mIndex, "rt");
     if (fpidx) {
	   if ((fpid2 = fopen(mIdx2, "wt"))) {
	     p = fgets(mTkn, sizeof(mTkn), fpidx);
	     while(p || !feof(fpidx)) {
	       if (strstr(mTkn, ".txt") ||
               strstr(mTkn, ".TXT") ) {
		     strcpy(mTkn2, mTkn);
	  	     if (!bGet) {
#ifdef UPADTE_20040422
		       if (bAll || dwCurrent >= dwStart && dwCurrent <= dwEnd)
#else
		       if (bAll || strstr(mTkn, mFirst))
#endif
		         bGet = TRUE;   // 出力禁止
			 }
		     if (bGet) {
		       if (mEnd[0] != '\x0')
#ifdef UPADTE_20040422
			     if (!bAll && dwCurrent > dwEnd) {
#else
				 if (!bAll && strstr(mTkn, mEnd)) {
#endif
		           bGet = FALSE;  // 出力再開 {
	               fputs(mTkn, fpid2);
				 }
			 } else {
	           fputs(mTkn, fpid2);
			 }
		   } else {
             fputs(mTkn, fpid2);
		   }
	       p = fgets(mTkn, sizeof(mTkn), fpidx);
		 }
 	     fflush(fpid2);
		 fclose(fpid2);
	   }
	   fclose(fpidx);
	 }
	 X_CopyFile(mIdx2, mIndex, FALSE); // 上書きコピー
	 _unlink(mIdx2); //DeleteFile(mIdx2);
   } else {
	 _unlink(mIdx2); //DeleteFile(mIdx2);
   }
*/
   fprintf(fp, "\nDeleted article.\n");
}
