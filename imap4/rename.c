////////////////////////////////////////////////////////////
// RENAME.C Copyright K.kawakami
////////////////////////////////////////////////////////////
#include "imap4.h"

BOOL RENAMEDispatch(PCLIENT_CONTEXT lpClientContext) {
  char    mOld[MAX_PATH], mNew[MAX_PATH], *q, *pOld, *pNew;
  PImap4Context pContext = &lpClientContext->Context;
  DWORD            nAttrib;
  BOOL             bShared = FALSE;
#ifdef UPDATE_20130531 // フォルダ名に半角スペースがあると処理が失敗することがある不具合
  char *pDest3 = NULL;
#endif

  if (pContext->State >= Imap4Authorization) {
	///////////////////////////////////////
	/// フォルダを削除
	pOld = pContext->pToken;
	pNew = strstr(pContext->pToken, " ");
#ifdef UPDATE_20130531 // フォルダ名に半角スペースがあると処理が失敗することがある不具合
	if (*pContext->pToken == '"')
	{
	  if ((pDest3 = strstr(pContext->pToken+1, "\"")))
	  {
	    pNew = strstr(pDest3, " ");
	  }
	}
#endif
	if (pNew) {
	  *pNew = '\x0';
	  pNew++;
	  if (*pNew == '"') {
	    pNew++;
	    strtok(pNew, "\"");
        if (*pNew == '"')
          *pNew = '\x0';
	  }
#ifdef UPDATE_20050531  // 命令にバッファフローの脆弱性
	  if (strlen(pNew) > MAXIMAPFOLDER) // フォルダ名が長い場合の処置
		*(pNew+MAXIMAPFOLDER) = '\x0';
#endif
	}
	if (*pOld == '"') {
	   pOld++;
	   strtok(pOld, "\"");
       if (*pOld == '"')
         *pOld = '\x0';
#ifdef UPDATE_20050531  // 命令にバッファフローの脆弱性
	  if (strlen(pOld) > MAXIMAPFOLDER) // フォルダ名が長い場合の処置
		*(pOld+MAXIMAPFOLDER) = '\x0';
#endif
	}
	if (pOld && pNew) {
	  if (pContext->mSharedRoot[0] && !strnicmp(pOld, IMAP4_SHARED_NAME, 8)) { // 共通フォルダの指定
	    q = strpbrk(pOld, "/\\");
        sprintf(mOld,"%s%s",pContext->mSharedRoot, ++q);
		if (!strnicmp(pNew, IMAP4_SHARED_NAME, 8)) {
	      q = strpbrk(pNew, "/\\");
          sprintf(mNew,"%s%s",pContext->mSharedRoot, ++q);
		}
 	    bShared = TRUE;
	  } else {
#ifdef UPDATE_20050531  // 命令にバッファフローの脆弱性
	    strcpy(mOld, pContext->mRootDir);
		strncat(mOld, pOld, MAX_PATH-strlen(pContext->mRootDir)-1);
		mOld[MAX_PATH-1] = '\x0';
	    strcpy(mNew, pContext->mRootDir);
		strncat(mNew, pNew, MAX_PATH-strlen(pContext->mRootDir)-1);
		mNew[MAX_PATH-1] = '\x0';
#else
        sprintf(mOld,"%s%s",pContext->mRootDir, pOld);
        sprintf(mNew,"%s%s",pContext->mRootDir, pNew);
#endif
	  }
      if (!bShared || bShared && pContext->bSharedRW) {
#ifdef UPDATE_20050530  // ユーザルートより上位のフォルダにアクセスできる脆弱性
#ifdef UPDATE_20060116  // ユーザルートより上位のフォルダにアクセスできる脆弱性３
	    if (FolderRight(pContext, (char *)mOld) &&
			FolderRight2(pContext, (char *)mNew))
#else
	    if (FolderRight(pContext, (char *)mOld))
#endif
#endif
		{
  	      if (rename(mOld, mNew) != 0)  // 変更失敗
            sprintf(pContext->mMessages, "%s %s %s fails\r\n", pContext->pSquence, IMAP4_RESULT_RESPONSE, pContext->pCmd );
          else {                       // 変更成功
		    // 一旦隠しファイルにする
	        nAttrib = GetFileAttributes(mNew);
		    //nAttrib |= FILE_ATTRIBUTE_HIDDEN; // 隠しファイルとして初期設定（未購読として扱う）
            SetFileAttributes(mNew, nAttrib);
		    if (stricmp(pOld, "inbox") ==0 ) { // inboxなら再作成
		     _mkdir(mOld);
			}
            sprintf(pContext->mMessages, "%s %s %s successful\r\n", pContext->pSquence, IMAP4_GOOD_RESPONSE, pContext->pCmd );
		  }
#ifdef UPDATE_20050530  // ユーザルートより上位のフォルダにアクセスできる脆弱性
		} else {
          sprintf(pContext->mMessages, "%s %s %s fails\r\n", pContext->pSquence, IMAP4_RESULT_RESPONSE, pContext->pCmd );
		}
#endif
	  } else
        sprintf(pContext->mMessages, "%s %s %s doesn't have right\r\n", pContext->pSquence, IMAP4_RESULT_RESPONSE, pContext->pCmd );
	} else { // フォルダ指定なし
      sprintf(pContext->mMessages, "%s %s %s illegal argument\r\n", pContext->pSquence, IMAP4_BADTOKEN_RESPONSE, pContext->pCmd );
	}
  } else { // 状態が違う
    sprintf(pContext->mMessages, "%s %s Command unrecognized: %s\r\n", pContext->pSquence, IMAP4_RESULT_RESPONSE, pContext->pCmd );
  }
  return TRUE;
}