////////////////////////////////////////////////////////////
// NOOP.C Copyright K.kawakami
////////////////////////////////////////////////////////////
#include "imap4.h"

#ifdef UPDATE_20110414 // NOOPタイマー（一定時間新着がないNOOPでセッション強制切断する対策（負荷軽減対策）
extern int nIDELTimeOut;
#endif
#ifdef UPDATE_20140530 // // NOOPによるポーリング結果送信有無
extern BOOL bNOOPPoll;   // NOOPによるポーリング結果送信有無
#endif
#ifdef UPDATE_20141220 // 操作フォルダをアクセス中のセッションに変化をリアルタイムに同報するための対策
extern BOOL  bBroadcastSession; // 同一フォルダの操作情報を接続中のセッションに同報するオプション TRUE:有効 FALSE:無効
void PutBroadcastSession(PCLIENT_CONTEXT lpClientContext);
#endif

BOOL NOOPDispatch(PCLIENT_CONTEXT lpClientContext) {
#ifdef UPDATE_20060303 // NOOPで状態応答
  char *p;
#endif   
  char mec[256];
#ifdef UPDATE_20110414 // NOOPタイマー（一定時間新着がないNOOPでセッション強制切断する対策（負荷軽減対策）
  SYSTEMTIME st, sst; // 現在時刻
  FILETIME ft[3];
  ULARGE_INTEGER *u[3], ut;
  BOOL bON = TRUE;
#endif


  PImap4Context pContext = &lpClientContext->Context;
#ifdef UPDATE_20140530 // // NOOPによるポーリング結果送信有無
  if (bNOOPPoll)
#endif
  {
#ifdef UPDATE_20060303 // NOOPで状態応答
	if (MakeUIDVALIDITY(pContext)) { // フォルダの存在有無とユニーク識別子有効値生成
#ifdef UPDATE_20110405A // UID値の排他処理強化
#ifdef UPDATE_20110406A// 他の命令でフォルダ処理中はNOOPでフォルダチェックしない
      if (LockMailSelectDirectory(pContext->mSelectDir)) // Lockファイルがある場合は何もしない。
	  {
        pContext->nUid = GetFolderUID(pContext->mSelectDir, pContext->nUid);
	    p = &pContext->mSelectDir[strlen(pContext->mRootDir)];
        if (!stricmp(p, "inbox")) // INBOXフォルダ選択なら
          MoveMSGFile(lpClientContext);
	    GetMSGFlags(lpClientContext); // フラグ集計
#else
	  p = &pContext->mSelectDir[strlen(pContext->mRootDir)];
      CriticalUID(lpClientContext, p);
#endif
#else
      pContext->nUid = GetFolderUID(pContext->mSelectDir, pContext->nUid);
	  p = &pContext->mSelectDir[strlen(pContext->mRootDir)];
      if (!stricmp(p, "inbox")) // INBOXフォルダ選択なら
        MoveMSGFile(lpClientContext);
	  GetMSGFlags(lpClientContext); // フラグ集計
#endif
	  /////////////////////////
	  //メッセージ総数
#ifdef UPDATE_20140606 // NOOPポーリング結果でフォルダ内に変化があった場合のみ結果応答をする
	  if (pContext->nExists != pContext->nIDLEExists )
	  {
        pContext->nIDLEExists = pContext->nExists;
#endif
        sprintf(pContext->mMessages, "* %lu EXISTS\r\n", pContext->nExists);
#ifdef UPDATE_20150107 // 操作フォルダをアクセス中のセッションに変化をリアルタイムに同報するための対策(IDLE/NOOP)
        put_reply(lpClientContext, TRUE, TRUE);
	    if (bBroadcastSession) // 同一フォルダの操作情報を接続中のセッションに同報するオプション TRUE:有効 FALSE:無効
		{
          PutBroadcastSession(lpClientContext); //pContext->mSelectDir, pContext->mMessages);
		}
#else
        put_reply(lpClientContext, TRUE, TRUE);
#endif
#ifdef UPDATE_20140606 // NOOPポーリング結果でフォルダ内に変化があった場合のみ結果応答をする
	  }
#endif
	  /////////////////////////
	  //新着メッセージ数
#ifdef UPDATE_20140606 // NOOPポーリング結果でフォルダ内に変化があった場合のみ結果応答をする
	  if (pContext->nRecent != pContext->nIDLERecent)
	  {
        pContext->nIDLERecent = pContext->nRecent;
#endif
        sprintf(pContext->mMessages, "* %lu RECENT\r\n", pContext->nRecent);
#ifdef UPDATE_20150107 // 操作フォルダをアクセス中のセッションに変化をリアルタイムに同報するための対策(IDLE/NOOP)
        put_reply(lpClientContext, TRUE, TRUE);
	    if (bBroadcastSession) // 同一フォルダの操作情報を接続中のセッションに同報するオプション TRUE:有効 FALSE:無効
		{
          PutBroadcastSession(lpClientContext); //pContext->mSelectDir, pContext->mMessages);
		}
#else
        put_reply(lpClientContext, TRUE, TRUE);
#endif
#ifdef UPDATE_20140606 // NOOPポーリング結果でフォルダ内に変化があった場合のみ結果応答をする
	  }
#endif
#ifdef UPDATE_20110414 // NOOPタイマー（一定時間新着がないNOOPでセッション強制切断する対策（負荷軽減対策）
	  if (pContext->nRecent == 0 && nIDELTimeOut > 0)  // 新着チェック前のNOOP値
	  {
        GetLocalTime(&st); // 現在時刻
        SystemTimeToFileTime(&st, &ft[0]);  // 現在時刻
        SystemTimeToFileTime(&pContext->nNOOPst, &ft[1]); // 新着未発生時刻
	    u[0] = (ULARGE_INTEGER *)&ft[0];
        u[1] = (ULARGE_INTEGER *)&ft[1];
		// nIDELTimeOut 秒経過しても新着が無いときセッション切断
		ut.QuadPart = (__int64)((__int64)10000000*(__int64)nIDELTimeOut);
        u[2] = (ULARGE_INTEGER *)&ft[2];
		u[2]->QuadPart = u[1]->QuadPart+ut.QuadPart;
		FileTimeToSystemTime(&ft[2], &sst);
        printf("IDLE time: %02d:%02d:%02d\n", pContext->nNOOPst.wHour, pContext->nNOOPst.wMinute, pContext->nNOOPst.wSecond);
        printf("Kill time: %02d:%02d:%02d\n", sst.wHour, sst.wMinute, sst.wSecond);
		if (u[0]->QuadPart > u[1]->QuadPart+ut.QuadPart)
		{
		  bON = FALSE;
		}
	  } else {
		GetLocalTime(&pContext->nNOOPst); // 新着があるなら更新
	  }
#endif
#ifdef UPDATE_20110406A// 他の命令でフォルダ処理中はNOOPでフォルダチェックしない
	    UnLockMailSelectDirectory(pContext->mSelectDir);
	  }
#endif
	}
#ifdef UPDATE_20110414 // NOOPタイマー（一定時間新着がないNOOPでセッション強制切断する対策（負荷軽減対策）
	else { // 不正なIMAPフォルダ位置でNOOPしている場合の対策
	  if (nIDELTimeOut > 0)
	  {
        GetLocalTime(&st); // 現在時刻
        //printf("%02d:%02d:%02d vs %02d:%02d:%02d\n", pContext->nNOOPst.wHour, pContext->nNOOPst.wMinute, pContext->nNOOPst.wSecond, st.wHour, st.wMinute, st.wSecond);
        SystemTimeToFileTime(&st, &ft[0]);  // 現在時刻
        SystemTimeToFileTime(&pContext->nNOOPst, &ft[1]); // 新着未発生時刻
        u[0] = (ULARGE_INTEGER *)&ft[0];
        u[1] = (ULARGE_INTEGER *)&ft[1];
  	    // nIDELTimeOut 秒経過しても新着が無いときセッション切断
		ut.QuadPart = (__int64)((__int64)10000000*(__int64)nIDELTimeOut);
        u[2] = (ULARGE_INTEGER *)&ft[2];
		u[2]->QuadPart = u[1]->QuadPart+ut.QuadPart;
		FileTimeToSystemTime(&ft[2], &sst);
        printf("IDLE time: %02d:%02d:%02d\n", pContext->nNOOPst.wHour, pContext->nNOOPst.wMinute, pContext->nNOOPst.wSecond);
        printf("Kill time: %02d:%02d:%02d\n", sst.wHour, sst.wMinute, sst.wSecond);
	    if (u[0]->QuadPart > u[1]->QuadPart+ut.QuadPart)
		{
	      bON = FALSE;
		}
	  }
	}
#endif
#endif
  }
#ifdef UPDATE_20140530 // // NOOPによるポーリング結果送信有無
  else {
	  if (nIDELTimeOut > 0)
	  {
        GetLocalTime(&st); // 現在時刻
        //printf("%02d:%02d:%02d vs %02d:%02d:%02d\n", pContext->nNOOPst.wHour, pContext->nNOOPst.wMinute, pContext->nNOOPst.wSecond, st.wHour, st.wMinute, st.wSecond);
        SystemTimeToFileTime(&st, &ft[0]);  // 現在時刻
        SystemTimeToFileTime(&pContext->nNOOPst, &ft[1]); // 新着未発生時刻
        u[0] = (ULARGE_INTEGER *)&ft[0];
        u[1] = (ULARGE_INTEGER *)&ft[1];
  	    // nIDELTimeOut 秒経過しても新着が無いときセッション切断
		ut.QuadPart = (__int64)((__int64)10000000*(__int64)nIDELTimeOut);
        u[2] = (ULARGE_INTEGER *)&ft[2];
		u[2]->QuadPart = u[1]->QuadPart+ut.QuadPart;
		FileTimeToSystemTime(&ft[2], &sst);
        printf("IDLE time: %02d:%02d:%02d\n", pContext->nNOOPst.wHour, pContext->nNOOPst.wMinute, pContext->nNOOPst.wSecond);
        printf("Kill time: %02d:%02d:%02d\n", sst.wHour, sst.wMinute, sst.wSecond);
	    if (u[0]->QuadPart > u[1]->QuadPart+ut.QuadPart)
		{
	      bON = FALSE;
		}
	  }
  }
#endif
  sprintf(pContext->mMessages, "%s %s %s completed\r\n", pContext->pSquence, IMAP4_GOOD_RESPONSE, pContext->pCmd );
#ifdef UPDATE_20110414 // NOOPタイマー（一定時間新着がないNOOPでセッション強制切断する対策（負荷軽減対策）
  return bON;
#else
  return TRUE;
#endif
}