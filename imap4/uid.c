////////////////////////////////////////////////////////////
// UID.C Copyright K.kawakami
////////////////////////////////////////////////////////////
#include "imap4.h"

extern BOOL    bServiceTerminating;

BOOL SEARCHDispatch(PCLIENT_CONTEXT lpClientContext);
BOOL FETCHDispatch(PCLIENT_CONTEXT lpClientContext);
BOOL STOREDispatch(PCLIENT_CONTEXT lpClientContext);
BOOL COPYDispatch(PCLIENT_CONTEXT lpClientContext);
BOOL BADDispatch(PCLIENT_CONTEXT lpClientContext);

BOOL UIDDispatch(PCLIENT_CONTEXT lpClientContext) {
  char    *p;
  BOOL    bsts = FALSE;
  PImap4Context pContext = &lpClientContext->Context;

  if (pContext->State == Imap4SelectFolder) {  // フォルダ選択済状態
	pContext->bUID = TRUE;
    pContext->pCmd = pContext->pToken;
    p = strstr(pContext->pCmd, " "); // 命令
	if (p) {
	  *p = '\x0';
	  pContext->pToken = ++p;
	  p = strstr(pContext->pToken, "\r\n"); // // 命令内容
	  if (p) {
	    *p = '\x0';
	  } else {
        strtok(pContext->pToken,"\r\n");
	  }
	} else {
      strtok(pContext->pCmd,"\r\n");
	}

	if (stricmp(pContext->pCmd, "COPY") == 0 ){
#ifndef UPDATE_20110404 // 選択フォルダ単位で排他する対策
#ifdef UPDATE_20110204 // ファイル操作のファイルのクリティカルセクション化
      while(!LockMailDirectory(pContext->mUSER, pContext->MyAddr)) // Lockファイルがある場合は何もしない。
	  {
#ifdef UPDATE_20170114 // UID値書き込み用のスペースに空きが無いときロックする不具合
        if (bServiceTerminating || GetLastError() == ERROR_DISK_FULL)
#else
        if (bServiceTerminating)
#endif
  	      break;
	     _sleep(WAIT_TIME);
	  }
#endif
#endif
      bsts = COPYDispatch(lpClientContext);
#ifndef UPDATE_20110404 // 選択フォルダ単位で排他する対策
#ifdef UPDATE_20110204 // ファイル操作のファイルのクリティカルセクション化
      UnLockMailDirectory(pContext->mUSER, pContext->MyAddr);
#endif
#endif
	} else if (stricmp(pContext->pCmd, "FETCH") == 0 ){
#ifndef UPDATE_20110404 // 選択フォルダ単位で排他する対策
#ifdef UPDATE_20110204 // ファイル操作のファイルのクリティカルセクション化
      while(!LockMailDirectory(pContext->mUSER, pContext->MyAddr)) // Lockファイルがある場合は何もしない。
	  {
#ifdef UPDATE_20170114 // UID値書き込み用のスペースに空きが無いときロックする不具合
        if (bServiceTerminating || GetLastError() == ERROR_DISK_FULL)
#else
        if (bServiceTerminating)
#endif
  	      break;
	     _sleep(WAIT_TIME);
	  }
#endif
#endif
      bsts = FETCHDispatch(lpClientContext);
#ifndef UPDATE_20110404 // 選択フォルダ単位で排他する対策
#ifdef UPDATE_20110204 // ファイル操作のファイルのクリティカルセクション化
      UnLockMailDirectory(pContext->mUSER, pContext->MyAddr);
#endif
#endif
	} else if (stricmp(pContext->pCmd, "STORE") == 0 ){
#ifndef UPDATE_20110404 // 選択フォルダ単位で排他する対策
#ifdef UPDATE_20110204 // ファイル操作のファイルのクリティカルセクション化
      while(!LockMailDirectory(pContext->mUSER, pContext->MyAddr)) // Lockファイルがある場合は何もしない。
	  {
#ifdef UPDATE_20170114 // UID値書き込み用のスペースに空きが無いときロックする不具合
        if (bServiceTerminating || GetLastError() == ERROR_DISK_FULL)
#else
        if (bServiceTerminating)
#endif
  	      break;
	     _sleep(WAIT_TIME);
	  }
#endif
#endif
      bsts = STOREDispatch(lpClientContext);
#ifndef UPDATE_20110404 // 選択フォルダ単位で排他する対策
#ifdef UPDATE_20110204 // ファイル操作のファイルのクリティカルセクション化
      UnLockMailDirectory(pContext->mUSER, pContext->MyAddr);
#endif
#endif
	} else if (stricmp(pContext->pCmd, "SEARCH") == 0 ) {
	  /*
#ifdef UPDATE_20110204 // ファイル操作のファイルのクリティカルセクション化
      while(!LockMailDirectory(pContext->mUSER, pContext->MyAddr)) // Lockファイルがある場合は何もしない。
	  {
#ifdef UPDATE_20170114 // UID値書き込み用のスペースに空きが無いときロックする不具合
        if (bServiceTerminating || GetLastError() == ERROR_DISK_FULL)
#else
        if (bServiceTerminating)
#endif
  	      break;
	     _sleep(WAIT_TIME);
	  }
#endif
	  */
      bsts = SEARCHDispatch(lpClientContext);
	  /*
#ifdef UPDATE_20110204 // ファイル操作のファイルのクリティカルセクション化
      UnLockMailDirectory(pContext->mUSER, pContext->MyAddr);
#endif
	  */
	} else { // 間違った命令
	  bsts = TRUE;
      sprintf(pContext->mMessages, "%s %s illegal argument.(%s)\r\n", pContext->pSquence, IMAP4_BADTOKEN_RESPONSE, (pContext->pCmd ? _strupr(pContext->pCmd) : "NULL"));
	}
  } else { // 状態が違う
#ifdef UPDATE_20090320 // フォルダ未選択でUIDリクエストを受けるとサービス停止してしまう
	bsts = TRUE;
#endif
    sprintf(pContext->mMessages, "%s %s Command unrecognized: %s\r\n", pContext->pSquence, IMAP4_RESULT_RESPONSE, pContext->pCmd );
  }
#ifdef UPDATE_20090320 // フォルダ未選択でUIDリクエストを受けるとサービス停止してしまう
  return TRUE;
#else
  return bsts;
#endif
}