////////////////////////////////////////////////////////////
// Expn.c Copyright K.kawakami
// メーリングリスト名から参加者アドレスの出力命令
////////////////////////////////////////////////////////////
#include "smtp.h"
extern bVrfy;
extern char   mMailSpoolDir[];
#ifdef CLUSTERING
extern BOOL   nClustering;
#endif
#ifdef UPDATE_20070521 // OSの予約語対策
extern CHAR   mReservedWords[];
#endif
#ifdef UPDATE_20070509 // エイリアス設定の実アドレスオプションを別データとして処理する
BOOL CheckUser(char *user, char *pOpt, char *myaddr, BOOL *bLocal, BOOL *bSubLocal, BOOL *bMList, BOOL *bOutSideAliases, LPBYTE fullname);
#else
BOOL CheckUser(char *user, char *myaddr, BOOL *bLocal, BOOL *bSubLocal, BOOL *bMList, BOOL *bOutSideAliases, LPBYTE fullname);
#endif
#ifdef UPDATE_20071209 // EXPN命令でメーリングリストメンバが正しく応答されないことがある不具合。
DWORD GetMLMembers(PCLIENT_CONTEXT lpClientContext,BOOL bView, DWORD nTotal, char *pML);
#endif
#ifdef BTHREAD
BOOL ExpnDispatch(PCLIENT_CONTEXT lpClientContext)
#else
SMTPRSDisposition ExpnDispatch(
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
#ifdef UPDATE_20071209 // EXPN命令でメーリングリストメンバが正しく応答されないことがある不具合。
  DWORD  nTotal = 0;
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
      sprintf(pContext->mMessages, SMTP_ISNT_INPLEMENTED); //SMTP_BAD_ARGUMENT);
	}
	pContext->p = strstr(pContext->mToken, " ");
	if (pContext->p && bVrfy) {
	  strtok(pContext->p, "\r\n");
	  pContext->p++;
      if (strlen(pContext->p) > 0) {
        pContext->bDomainVRFY = FALSE;
#ifdef UPDATE_20071209 // EXPN命令でメーリングリストメンバが正しく応答されないことがある不具合。
         nTotal = GetMLMembers(NULL, FALSE, 0, pContext->p);
		 if (nTotal > 0) { // メンバが存在する
           GetMLMembers(lpClientContext, TRUE, nTotal, pContext->p);
		 } else { // メンバが存在しない、またはリスト名が無い
		   sprintf(pContext->mMessages, SMTP_BAD_EXPN, pContext->p); // ユーザーが存在しない場合
		 }

#else
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
		//pContext->p = strstr(pContext->mToken, " ");
 	    //strtok(pContext->p, "\r\n");
	    //pContext->p++;
        /////////////////////////////////////////
        if (pContext->bVRFY && pContext->bDomainVRFY)
	      sprintf(pContext->mMessages, SMTP_GOOD_VRFY, pContext->p, pContext->fullname); // ユーザーが存在する場合
		else
		  sprintf(pContext->mMessages, SMTP_UNKNOW_USER, pContext->p); // ユーザーが存在しない場合
#endif
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

#ifdef UPDATE_20071209 // EXPN命令でメーリングリストメンバが正しく応答されないことがある不具合。
DWORD GetMLMembers(PCLIENT_CONTEXT lpClientContext,BOOL bView, DWORD nTotal, char *pML)
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
#ifdef BTHREAD
  PSmtpContext pContext = &lpClientContext->Context;
#endif
#ifdef UPDATE_20071210A // ML参加アドレスを含む256Byte超のフォルダ名になったと場合の対策
  FILE *fp;
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
	fp = fopen(mMLFn, "rt");
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
	    if (fp && retCode != ERROR_SUCCESS) {
		  lpName[0] = '\x0';
		  fgets(lpName, sizeof(lpName), fp);
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
		if (bView) {
		  if (nTotal > dwIndex ) {
		    sprintf(pContext->mMessages, SMTP_GOOD_EXPN2, lpName);
            send(lpClientContext->Socket, pContext->mMessages, strlen(pContext->mMessages), 0 );
		  } else {
		    sprintf(pContext->mMessages, SMTP_GOOD_EXPN, lpName);
			// 最後のメッセージは戻って送信
		  }
		}
	  }
	} while (retCode == ERROR_SUCCESS);
#ifdef UPDATE_20071210A // ML参加アドレスを含む256Byte超のフォルダ名になったと場合の対策
	if (fp) {
      fclose(fp);
	}
#endif
#ifdef REGTOFILE
   if (nClustering && !strnicmp(mkey, "software\\emwac", 14))
 	 CloseHandle(hFile);
   else
#endif
    RegCloseKey(hKey);
  }
  return dwIndex;
}
#endif