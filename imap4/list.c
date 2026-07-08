////////////////////////////////////////////////////////////
// LIST.C Copyright K.kawakami
////////////////////////////////////////////////////////////
#include "imap4.h"

extern BOOL  bDebug;

void ListDir(PCLIENT_CONTEXT lpClientContext, char *pRoot, char *pCurrent, BOOL bHidden);

BOOL LISTDispatch(PCLIENT_CONTEXT lpClientContext) {
  char    *pRoot, *pCurrent;
  PImap4Context pContext = &lpClientContext->Context;

   if (bDebug) printf("[%08x]  pList: state = %d;\n", lpClientContext, pContext->State);

  if (pContext->State >= Imap4Authorization) {
	///////////////////////////////////////
	/// フォルダを作成
	pRoot    = pContext->pToken;
	pCurrent = strstr(pContext->pToken, " ");
	if (pCurrent) {
	  *pCurrent = '\x0';
	  pCurrent++;
	  if (*pCurrent == '"') {
	    pCurrent++;
	    strtok(pCurrent, "\"");
	    if (*pCurrent == '"')
		  *pCurrent = '\x0';
	  }
	}
	if (*pRoot == '"') {
	   pRoot++;
	   strtok(pRoot, "\"");
	   if (*pRoot == '"')
		 *pRoot = '\x0';
	}
	if (pRoot && pCurrent) {
	  pContext->mLinkRoot[0] = '\x0';
	  ListDir(lpClientContext, pRoot, pCurrent, TRUE);
      sprintf(pContext->mMessages, "%s %s %s completed\r\n", pContext->pSquence, IMAP4_GOOD_RESPONSE, pContext->pCmd );
	} else { // フォルダ指定なし
      sprintf(pContext->mMessages, "%s %s %s illegal argument\r\n", pContext->pSquence, IMAP4_BADTOKEN_RESPONSE, pContext->pCmd );
	}
  } else { // 状態が違う
    sprintf(pContext->mMessages, "%s %s Command unrecognized: %s\r\n", pContext->pSquence, IMAP4_RESULT_RESPONSE, pContext->pCmd );
  }
  return TRUE;
}

void ListDir(PCLIENT_CONTEXT lpClientContext, char *pRoot, char *pCurrent, BOOL bHidden) {
  HANDLE           hSearch;
  WIN32_FIND_DATA  FindData;
  PImap4Context    pContext = &lpClientContext->Context;
  char             *p, *q, mTempDir[MAX_PATH], mCurrent[MAX_PATH], mCurrent2[MAX_PATH], mCurrent3[MAX_PATH];
  BOOL             bPercent = FALSE;
  BOOL             bShared = FALSE;
#ifdef LINK_FOLDER
  FILE *fp;
  BOOL bLink;
  char *pLink, mLink[MAX_PATH];
#endif

    strncpy(mCurrent2, pCurrent, MAX_PATH-1);
	mCurrent2[MAX_PATH-1] = '\x0';
	mCurrent3[0] = '\x0';
    //p = strstr(pCurrent, "%");
	if ((p = strpbrk(pCurrent, "*%"))) {
	  if (*p == '%') { // 最初に'%'が見つかる場合
	    *p = '*';
	    bPercent = TRUE;
	  } else if (*p == '*') {
	    p++;
	    if (strpbrk(p, "/"))
          strcpy(mCurrent3, p); // 残りの階層があればとりあえずコピー
	    *p = '\x0';
	  } else { // "*"が無い場合
	    bPercent = TRUE;
	  }
	} else {
      bPercent = TRUE;
	}
    if (*pRoot) {
	  if (!strcmp(pRoot, "/")) { // ルートなら
		*pRoot = '\x0';
		sprintf(mTempDir,"%s%s%s%s",pContext->mRootDir, ++pRoot, (*pRoot ? "/" : ""), pCurrent);
	  } else {
		if (pContext->mSharedRoot[0] && !strnicmp(pRoot, IMAP4_SHARED_NAME, 8)) {// 共通フォルダの指定
          sprintf(mTempDir,"%s%s",pContext->mSharedRoot, pCurrent);
          bShared = TRUE;
		} else {
#ifdef LINK_FOLDER
		  if (pContext->mLinkRoot[0]) {
#ifdef UPDATE_20050531  // 命令にバッファフローの脆弱性
	        strcpy(mTempDir, pContext->mLinkRoot);
	        strncat(mTempDir, pCurrent, MAX_PATH-strlen(pContext->mLinkRoot)-1);
	        mTempDir[MAX_PATH-1] = '\x0';
#else
            sprintf(mTempDir,"%s%s",pContext->mLinkRoot, pCurrent);
#endif
		  } else {
#ifdef UPDATE_20050531  // 命令にバッファフローの脆弱性
	        strcpy(mTempDir, pContext->mRootDir);
	        strncat(mTempDir, pRoot, MAX_PATH-strlen(pContext->mRootDir)-1);
	        mTempDir[MAX_PATH-1] = '\x0';
	        strncat(mTempDir, pCurrent, MAX_PATH-strlen(pContext->mRootDir)-1);
	        mTempDir[MAX_PATH-1] = '\x0';
#else
            sprintf(mTempDir,"%s%s%s",pContext->mRootDir, pRoot, pCurrent);
#endif
		  }
#else
          sprintf(mTempDir,"%s%s%s",pContext->mRootDir, pRoot, pCurrent);
#endif
#ifdef UPDATE_20050530  // ユーザルートより上位のフォルダにアクセスできる脆弱性
          if (!FolderRight(pContext, (char *)mTempDir)) {
#ifdef UPDATE_20050531  // 命令にバッファフローの脆弱性
	        strcpy(mTempDir, pContext->mRootDir);
	        strncat(mTempDir, pCurrent, MAX_PATH-strlen(pContext->mLinkRoot)-1);
	        mTempDir[MAX_PATH-1] = '\x0';
#else
            sprintf(mTempDir,"%s%s",pContext->mRootDir, pCurrent);
#endif
		  }
#endif

		}
	  }
    } else {
	   /*
      if (*pCurrent)
        sprintf(mTempDir,"%s%s",pContext->mSelectDir, pCurrent);
      else
        sprintf(mTempDir,"%s",pContext->mSelectDir);
	  */
	  if (*pCurrent) {
		if (pContext->mSharedRoot[0] && !strnicmp(pCurrent, IMAP4_SHARED_NAME, 8)) {// 共通フォルダの指定
	      q = strpbrk(pCurrent, "/\\");
	      sprintf(mTempDir,"%s%s",pContext->mSharedRoot, ++q);
          bShared = TRUE;
		} else {
#ifdef LINK_FOLDER
		  if (pContext->mLinkRoot[0]) {
#ifdef UPDATE_20050531  // 命令にバッファフローの脆弱性
	        strcpy(mTempDir, pContext->mLinkRoot);
	        strncat(mTempDir, pCurrent, MAX_PATH-strlen(pContext->mLinkRoot)-1);
	        mTempDir[MAX_PATH-1] = '\x0';
#else
            sprintf(mTempDir,"%s%s",pContext->mLinkRoot, pCurrent);
#endif
		  } else {
#ifdef UPDATE_20050531  // 命令にバッファフローの脆弱性
	        strcpy(mTempDir, pContext->mRootDir);
	        strncat(mTempDir, pCurrent, MAX_PATH-strlen(pContext->mLinkRoot)-1);
	        mTempDir[MAX_PATH-1] = '\x0';
#else
            sprintf(mTempDir,"%s%s",pContext->mRootDir, pCurrent);
#endif
		  }
#else
          sprintf(mTempDir,"%s%s",pContext->mRootDir, pCurrent);
#endif
		}
      } else {
        sprintf(mTempDir,"%s",pContext->mRootDir);
}
	    //sprintf(mTempDir,"%s*",pContext->mSelectDir);
    }

	if (!*pCurrent) { // 空 ("" 文字列) メールボックス名の場合階層区切りを返す
      sprintf(pContext->mMessages, "* %s () \"/\" \"\"\r\n", pContext->pCmd);
      put_reply(lpClientContext, TRUE, TRUE);
	  return;
	}

#ifdef UPDATE_20060116  // ユーザルートより上位のフォルダにアクセスできる脆弱性３
    if (!FolderRight2(pContext, (char *)mTempDir)) {
	  return;
	}
#endif

    hSearch = FindFirstFile(mTempDir, &FindData);
    if (hSearch != INVALID_HANDLE_VALUE) {
      do {
#ifdef LINK_FOLDER
		 bLink = FALSE;
		 if ((pLink = strrchr(FindData.cFileName, '.'))) // ショートカットか確認
		   if (!stricmp(pLink, ".lnk"))
			 bLink = TRUE;
		 if (FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY || bLink)
#else
		 if (FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) //nAttribute) { // (FILE_ATTRIBUTE_DIRECTORY /FILE_ATTRIBUTE_HIDDEN)
#endif
		 {
		   if (!bHidden && FindData.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) { // LSUBで隠しファイルは無視
			 continue;
		   }
		   if (strcmp((const char *)FindData.cFileName,".") == 0 || 
			   strcmp((const char *)FindData.cFileName,"..") == 0) {
		     continue;
		   }
		   if (!stricmp((const char *)FindData.cFileName, "INBOX")) // INBOXは大文字に置き換え
			 _strupr(FindData.cFileName);

		   strcpy(mCurrent, pCurrent);
		   if (*pCurrent == '*') {
		     sprintf(pContext->mMessages, "* %s (%s) \"%s\" \"%s%s\"\r\n", 
				              pContext->pCmd, 
							  "", //(FindData.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN ? "\\Noselect" : ""), 
							  "/", //(*pRoot=='\x0' ? "/" : pRoot), 
                              (bShared ? IMAP4_SHARED_NAME: ""),
							  (const char *)FindData.cFileName);
		   } else {
			 if (( p = strpbrk(pCurrent, "*"))) 
			   *p = '\x0';
			 else if ( (p = strrchr(pCurrent, '/'))) {
			   p++;
			   *p = '\x0';
			 }
             sprintf(pContext->mMessages, "* %s (%s) \"%s\" \"%s%s\"\r\n",
				              pContext->pCmd, 
							  "", //(FindData.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN ? "\\Noselect" : ""), 
                              "/", //(*pRoot=='\x0' ? "/" : pRoot), 
							  (p ? (strstr(pCurrent, "/") ? pCurrent : "") : ""), 
							  (const char *)FindData.cFileName);
			 if (p) 
			   *p = '*';
		   }
           put_reply(lpClientContext, TRUE, TRUE);
		   if (mCurrent[0] == '*') {
		     p = mCurrent;
		     *p = '\x0';
		   } else {
		     p = strstr(mCurrent, "*");
		     if (p)
		       *p = '\x0';
		   }
		   if (strpbrk(mCurrent, "/"))
             strcat(mCurrent, (const char *)FindData.cFileName);
		   else
             strcpy(mCurrent, (const char *)FindData.cFileName);
#ifdef LINK_FOLDER
		   if (bLink) {
		     strcpy(mLink, mTempDir);
		     strtok(mLink, "*%");
		     strcat(mLink, mCurrent);
	         if ((fp = fopen(mLink, "rt"))) { // リンクが貼られている場合
			   bLink = TRUE;
			   strcpy(mLink, pContext->mLinkRoot);
		       fgets(pContext->mLinkRoot, sizeof(pContext->mLinkRoot), fp);
		       strtok(pContext->mLinkRoot, "\n");
		       fclose(fp);
			 }
		   }
#endif
		   if (!mCurrent3[0]) {
		     if (p)
		       strcat(mCurrent, "/*");
		   } else {
  	         strcat(mCurrent, mCurrent3);
		   }
           if (!bPercent) { // '%'での検索の場合は子タスクは検索しない
		     ListDir(lpClientContext, pRoot, mCurrent, bHidden);
#ifdef LINK_FOLDER
			 if (bLink)
               strcpy(pContext->mLinkRoot, mLink); // リンクルートフォルダを復帰
#endif
             strcpy(pCurrent, mCurrent2);
		   }
		 }
	  } while (FindNextFile(hSearch, &FindData));
	  FindClose(hSearch);
	} /*else {
	  strcpy(mCurrent, pCurrent);
	  strtok(mCurrent, "*%");
      sprintf(pContext->mMessages, "* %s () \"/\" \"%s\"\r\n", pContext->pCmd, mCurrent);
      put_reply(lpClientContext, TRUE, TRUE);
	}*/
}