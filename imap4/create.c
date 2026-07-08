////////////////////////////////////////////////////////////
// CREATE.C Copyright K.kawakami
////////////////////////////////////////////////////////////
#include "imap4.h"

extern BOOL    bServiceTerminating;
extern CHAR    mInboxAlias[];

BOOL CREATEDispatch(PCLIENT_CONTEXT lpClientContext) {
  char             mTempDir[MAX_PATH], *p, *q;
  PImap4Context    pContext = &lpClientContext->Context;
  BOOL             bSts = TRUE;
  DWORD            nAttrib;
  BOOL             bShared = FALSE;
#ifdef UPDATE_20110208 // フォルダ作成が完了するまでウエイトする。
  int              i;
  BOOL             bSuccess = FALSE;
  HANDLE           hF;
  WIN32_FIND_DATA  FD;
#endif

  if (pContext->State >= Imap4Authorization) {
	///////////////////////////////////////
	/// フォルダを作成
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
	  if (!strncmp(p, mInboxAlias, strlen(mInboxAlias))) {// 受信トレイの別名称の場合
	    q = strpbrk(p, "/\\");
	    sprintf(mTempDir,"%sINBOX%s", pContext->mRootDir, (q ? q : ""));
	  } else if (pContext->mSharedRoot[0] && !strnicmp(p, IMAP4_SHARED_NAME, 8)) { // 共通フォルダの指定
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
      bSts = FALSE;
	  if (!bShared || bShared && pContext->bSharedRW) {
#ifdef UPDATE_20050530  // ユーザルートより上位のフォルダにアクセスできる脆弱性
	    if (FolderRight(pContext, (char *)mTempDir)) {
#endif
#ifdef UPDATE_20110208 // フォルダ作成が完了するまでウエイトする。
		  bSuccess = FALSE;
		  for (i = 0; i < 5; i++)
		  {
			if (_mkdir(mTempDir) == 0)
			{
			  bSuccess = TRUE;
			  break;
			}
	        _sleep(300*i);
		  }
		  if (bSuccess)
#else
          if (_mkdir(mTempDir) == 0) 
#endif
	      { // 処理用フォルダ作成成功
#ifdef UPDATE_20110208 // フォルダ作成が完了するまでウエイトする。
			if (mTempDir[strlen(mTempDir)-1] == '/')
			{
			  mTempDir[strlen(mTempDir)-1] = '\x0';
			}
	        while ((hF = FindFirstFile(mTempDir, &FD)) == INVALID_HANDLE_VALUE) 
			{
              if (bServiceTerminating)
  	            break;
			   _mkdir(mTempDir);
	           _sleep(WAIT_TIME);
			}
			if (hF) {
              FindClose( hF ); 
			}
#endif
	        nAttrib = GetFileAttributes(mTempDir);
            SetFileAttributes(mTempDir, nAttrib);
            sprintf(pContext->mMessages, "%s %s %s completed\r\n", pContext->pSquence, IMAP4_GOOD_RESPONSE, pContext->pCmd );
		  } else {   // フォルダ作成失敗
            sprintf(pContext->mMessages, "%s %s [TRYCREATE] %s failed\r\n", pContext->pSquence, IMAP4_RESULT_RESPONSE, p );
		  }
#ifdef UPDATE_20050530  // ユーザルートより上位のフォルダにアクセスできる脆弱性
		} else {
          sprintf(pContext->mMessages, "%s %s [TRYCREATE] %s failed\r\n", pContext->pSquence, IMAP4_RESULT_RESPONSE, p );
		}
#endif
	  } else {  // フォルダ作成不可
        sprintf(pContext->mMessages, "%s %s %s \"%s\" doesn't have right\r\n", pContext->pSquence, IMAP4_RESULT_RESPONSE, pContext->pCmd, p );
	  }
	} else { // フォルダ指定なし
      sprintf(pContext->mMessages, "%s %s %s illegal argument\r\n", pContext->pSquence, IMAP4_BADTOKEN_RESPONSE, pContext->pCmd );
	}
  } else { // 状態が違う
    sprintf(pContext->mMessages, "%s %s Command unrecognized: %s\r\n", pContext->pSquence, IMAP4_RESULT_RESPONSE, pContext->pCmd );
  }
  return TRUE;
}