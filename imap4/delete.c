////////////////////////////////////////////////////////////
// DELETE.C Copyright K.kawakami
////////////////////////////////////////////////////////////
#include "imap4.h"

#ifdef UPDATE_20170223 // フォルダ削除でのメール一括削除オプション追加
extern BOOL   bAutoSubDirAndMsg;

BOOL DeleteSubDirAndMessage(char *pDir);
#endif

BOOL DELETEDispatch(PCLIENT_CONTEXT lpClientContext) {
  char          mUID[MAX_PATH*2], mTempDir[MAX_PATH], *p, *q;
  PImap4Context pContext = &lpClientContext->Context;
  BOOL             bShared = FALSE;
#ifdef UPDATE_20110406 // フォルダ削除時にrid,store,fetch処理用のインデックスファイルを削除する。
  char          mIdx[MAX_PATH*2];
#endif

  if (pContext->State >= Imap4Authorization) {
	///////////////////////////////////////
	/// フォルダを削除
	p = pContext->pToken;
	if (*p == '"') {
	   p++;
	   strtok(p, "\"");
       if (*p == '"')
         *p = '\x0';
#ifdef UPDATE_20050531  // 命令にバッファフローの脆弱性
	  if (strlen(p) > MAXIMAPFOLDER) // フォルダ名が長い場合の処置
		*(p+MAXIMAPFOLDER) = '\x0';
#endif
	}
	if (p) {
	  if (stricmp(p, "inbox")) { // INBOXでないなら削除許可
		if (pContext->mSharedRoot[0] && !strnicmp(p, IMAP4_SHARED_NAME, 8)) {// 共通フォルダの指定
	      q = strpbrk(p, "/\\");
	      sprintf(mTempDir,"%s%s",pContext->mSharedRoot, ++q);
		  bShared = TRUE;
		} else {
#ifdef UPDATE_20050531  // 命令にバッファフローの脆弱性
	      strcpy(mTempDir, pContext->mRootDir);
		  strncat(mTempDir, p, MAX_PATH-strlen(pContext->mRootDir)-1);
		  mTempDir[MAX_PATH-1] = '\x0';
#else
          sprintf(mTempDir,"%s%s",pContext->mRootDir, p);
#endif
		}
		sprintf(mUID, "%s\\uid", mTempDir);
	    if (!bShared || bShared && pContext->bSharedRW) {
#ifdef UPDATE_20050530  // ユーザルートより上位のフォルダにアクセスできる脆弱性
	      if (FolderRight(pContext, (char *)mTempDir)) {
#endif
		    DeleteFile(mUID); // UIDインデックスを削除
#ifdef UPDATE_20110406 // フォルダ削除時にrid,store,fetch処理用のインデックスファイルを削除する。
			//printf("GetLastError()=%d\n", GetLastError());
		    sprintf(mIdx, "%s\\rid", mTempDir);
			DeleteFile(mIdx); // RIDインデックスを削除
			//printf("GetLastError()=%d\n", GetLastError());
		    sprintf(mIdx, "%s\\store", mTempDir);
			DeleteFile(mIdx); // STOREインデックスを削除
		    sprintf(mIdx, "%s\\fetch", mTempDir);
			DeleteFile(mIdx); // FETCHインデックスを削除
#endif
#ifdef UPDATE_20170223 // フォルダ削除でのメール一括削除オプション追加
            if (!bAutoSubDirAndMsg || bAutoSubDirAndMsg && DeleteSubDirAndMessage(mTempDir))
#endif
			{
  	          if (_rmdir(mTempDir) != 0)  // 削除失敗
                sprintf(pContext->mMessages, "%s %s %s failed\r\n", pContext->pSquence, IMAP4_RESULT_RESPONSE, pContext->pCmd );
              else
                sprintf(pContext->mMessages, "%s %s %s successful\r\n", pContext->pSquence, IMAP4_GOOD_RESPONSE, pContext->pCmd );
			}
#ifdef UPDATE_20170223 // フォルダ削除でのメール一括削除オプション追加
			 else {
              sprintf(pContext->mMessages, "%s %s %s failed\r\n", pContext->pSquence, IMAP4_RESULT_RESPONSE, pContext->pCmd );
			}
#endif
#ifdef UPDATE_20050530  // ユーザルートより上位のフォルダにアクセスできる脆弱性
		  } else {
            sprintf(pContext->mMessages, "%s %s %s failed\r\n", pContext->pSquence, IMAP4_RESULT_RESPONSE, pContext->pCmd );
		  }
#endif
		} else
          sprintf(pContext->mMessages, "%s %s %s \"%s\" doesn't have right\r\n", pContext->pSquence, IMAP4_RESULT_RESPONSE, pContext->pCmd, p );
	  } else {
        sprintf(pContext->mMessages, "%s %s %s failed\r\n", pContext->pSquence, IMAP4_RESULT_RESPONSE, pContext->pCmd );
	  }
	} else { // フォルダ指定なし
      sprintf(pContext->mMessages, "%s %s %s illegal argument\r\n", pContext->pSquence, IMAP4_BADTOKEN_RESPONSE, pContext->pCmd );
	}
  } else { // 状態が違う
    sprintf(pContext->mMessages, "%s %s Command unrecognized: %s\r\n", pContext->pSquence, IMAP4_RESULT_RESPONSE, pContext->pCmd );
  }
  return TRUE;
}

#ifdef UPDATE_20170223 // フォルダ削除でのメール一括削除オプション追加
BOOL DeleteSubDirAndMessage(char *pDir)
{
  HANDLE           hSearch;
  WIN32_FIND_DATA  FindData;
  BOOL bsts = TRUE;
  CHAR mDir[MAX_PATH];
  CHAR mWork[MAX_PATH];

  sprintf(mDir, "%s\\*", pDir);
  hSearch = FindFirstFile((LPCTSTR)mDir, &FindData);
  if (hSearch != INVALID_HANDLE_VALUE) 
  {
    do {
	  if (strcmp(FindData.cFileName, ".") &&
	 	  strcmp(FindData.cFileName, "..") )
	  {
	    sprintf(mWork, "%s\\%s", pDir, FindData.cFileName);
        if (FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) // サブフォルダの中身を除去
		{
		   bsts = DeleteSubDirAndMessage(mWork);
		   if (_rmdir(mWork) != 0)  // 削除失敗
		   {
			 bsts = FALSE;
			 break;
		   }
		   continue;   
		}
		if (!bsts) {
		  break;
		}
	    if (_unlink(mWork) != 0)
		{
		   bsts = FALSE;
		   break;
		}
	  }
    } while (FindNextFile(hSearch, &FindData));
	FindClose(hSearch);
  }
  return bsts;
}
#endif