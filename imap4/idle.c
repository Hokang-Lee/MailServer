////////////////////////////////////////////////////////////
// NOOP.C Copyright K.kawakami
////////////////////////////////////////////////////////////
#include "imap4.h"
#include <process.h>
#include <winbase.h>

#ifdef UPDATE_20140528 // IDLEコマンド実装
#ifdef UPDATE_20250918 // IDLEポーリングベース間隔(ミリ秒）の調整
extern int  nIDLEMSLoop; // IDLEベースタイマー（一定時間新着チェック）
#endif
// IDLEタイマー（一定時間新着チェック）
extern int nIDLELoop;
//extern BOOL  bDebug;

#ifdef ACCEPT_LOG_LEVEL3
void LEVEL_3_ACCEPTLOG(PCLIENT_CONTEXT lpClientContext, char *mess);
#endif
#ifdef UPDATE_20141018 // IDLEスレッドの起動に失敗しハングする可能性の対策
unsigned __stdcall IDLE_Thread(PCLIENT_CONTEXT lpClientContext);
#else
void IDLE_Thread(PCLIENT_CONTEXT lpClientContext);
#endif
#ifdef UPDATE_20141220 // 操作フォルダをアクセス中のセッションに変化をリアルタイムに同報するための対策
extern BOOL  bBroadcastSession; // 同一フォルダの操作情報を接続中のセッションに同報するオプション TRUE:有効 FALSE:無効
void PutBroadcastSession(PCLIENT_CONTEXT lpClientContext);
#endif

BOOL IDLEDispatch(PCLIENT_CONTEXT lpClientContext) {

  PImap4Context pContext = &lpClientContext->Context;
  HANDLE hCompletionPort = lpClientContext;
  DWORD dwThreadId;
  BOOL  bOK = TRUE;
  DWORD dwStart = GetTickCount();

#ifdef UPDATE_20141027 // ログインされていないセッションでIDLE命令が有効になってしまう不具合
  if (pContext->State < Imap4Authorization)
  {
    bOK = FALSE;
  }
#endif
  /////////////////////////////////////
  // IDLEスレッドは１セッションで一つのみ実行
  while(pContext->bIDLE || pContext->bIDLEThread)
  {
    Sleep(1);
    if (GetTickCount() - dwStart > (DWORD)10000)
	{
	  bOK = FALSE;
	  break;
	}
  }
  if (bOK)
  {
    pContext->bIDLE = TRUE;
    pContext->mIDLESquence[0] = '\x0';
    /////////////////////////////////////
    sprintf(pContext->mMessages, "+ idling\r\n");
    strcpy(pContext->mIDLESquence, pContext->pSquence);
	//if (bDebug) printf("[%08x] 0:_beginthread(IDLE_Thread)\n", lpClientContext);
#ifdef UPDATE_20141018 // IDLEスレッドの起動に失敗しハングする可能性の対策
#ifdef UPDATE_20141025 // DONEコマンド受信時にIDLEスレッドのハンドル値をリセットしてメモリ開放に失敗していた
	if ((pContext->hIDLEThread = _beginthreadex(NULL, 0, IDLE_Thread, lpClientContext, 0, NULL)) == 0)
#else
	if ((pContext->hIDLEThread = _beginthreadex(NULL, 0, IDLE_Thread, lpClientContext, 0, NULL)) == -1L)
#endif
	{
	   pContext->bIDLE = FALSE;
	}
#else
    _beginthread( IDLE_Thread, sizeof(SOCKET), lpClientContext);
#endif
  } else {
    sprintf(pContext->mMessages, "%s %s IDLE failure\r\n", pContext->pSquence, IMAP4_RESULT_RESPONSE);
  }
  return TRUE;
}

BOOL DONEDispatch(PCLIENT_CONTEXT lpClientContext) {

  PImap4Context pContext = &lpClientContext->Context;

  if (pContext->bIDLE)
  {
    pContext->bIDLE = FALSE;
	Sleep(1);
#ifdef UPDATE_20141025 // DONEコマンド受信時にIDLEスレッドのハンドル値をリセットしてメモリ開放に失敗していた
    if (pContext->hIDLEThread != NULL && pContext->hIDLEThread != -1L)
	{
	  WaitForSingleObject(pContext->hIDLEThread, INFINITE);
      CloseHandle(pContext->hIDLEThread);
	  pContext->hIDLEThread = NULL;
	}
#endif
    sprintf(pContext->mMessages, "%s %s IDLE terminated\r\n", pContext->mIDLESquence, IMAP4_GOOD_RESPONSE);
    pContext->mIDLESquence[0] = '\x0';
#ifndef UPDATE_20141025 // DONEコマンド受信時にIDLEスレッドのハンドル値をリセットしてメモリ開放に失敗していた
	pContext->hIDLEThread = NULL;
#endif
  } else {
	sprintf(pContext->mMessages, "%s %s illegal argument.(%s)\r\n", pContext->pSquence, IMAP4_BADTOKEN_RESPONSE, (pContext->pCmd ? _strupr(pContext->pCmd) : "NULL"));
  }
  return TRUE;
}

#ifdef UPDATE_20141018 // IDLEスレッドの起動に失敗しハングする可能性の対策
unsigned __stdcall IDLE_Thread(PCLIENT_CONTEXT lpClientContext) 
#else
void IDLE_Thread(PCLIENT_CONTEXT lpClientContext) 
#endif
{
  char *p;
  char mec[256];

  PImap4Context pContext =  &lpClientContext->Context;
  DWORD dwStart = GetTickCount();

  //if (bDebug) printf("[%08x] 0:IDLE_Thread()\n", lpClientContext);
#ifdef ACCEPT_LOG_LEVEL3
  LEVEL_3_ACCEPTLOG(lpClientContext, "IDLE_Thread():Start.");
#endif

  pContext->bIDLEThread = TRUE;
  //Sleep(5*1000); // 2秒単位
  while(pContext->bIDLE)
  {
#ifdef UPDATE_20250918 // IDLEポーリングベース間隔(ミリ秒）の調整
    if (GetTickCount() - dwStart > (DWORD)(nIDLELoop*nIDLEMSLoop))
#else
    if (GetTickCount() - dwStart > (DWORD)(nIDLELoop*1000))
#endif
	{
       //if (bDebug) printf("[%08x] 1:IDLE_Thread()\n", lpClientContext);
	   dwStart = GetTickCount();
#ifdef UPDATE_20110405A // UID値の排他処理強化
#ifdef UPDATE_20110406A// 他の命令でフォルダ処理中はNOOPでフォルダチェックしない
/*		
#ifdef ACCEPT_LOG_LEVEL3
		LEVEL_3_ACCEPTLOG(lpClientContext, "IDLE_Thread():LockMailSelectDirectory()");
#endif
*/		
      if (LockMailSelectDirectory(pContext->mSelectDir)) // Lockファイルがある場合は何もしない。
	  {
/*		  
#ifdef ACCEPT_LOG_LEVEL3
	    LEVEL_3_ACCEPTLOG(lpClientContext, "IDLE_Thread():GetFolderUID()");
#endif
*/
//pContext = NULL; //99dd		  
        pContext->nUid = GetFolderUID(pContext->mSelectDir, pContext->nUid);
//pContext = NULL; //9a01
	    p = &pContext->mSelectDir[strlen(pContext->mRootDir)];
        if (!stricmp(p, "inbox")) // INBOXフォルダ選択なら
		{
          MoveMSGFile(lpClientContext);
          //printf("************ Wait IDLE_Thread() 60sec.\n");
          //Sleep(60000); //1min待たせる
		}
/*		
#ifdef ACCEPT_LOG_LEVEL3
	    LEVEL_3_ACCEPTLOG(lpClientContext, "IDLE_Thread():GetMSGFlags()");
#endif
*/		
#ifdef UPDATE_20141017 // IMAPフォルダ指定が空欄の場合はフラグ集計しない
		if (*p) // フォルダ名が存在する場合のみ
#endif
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
		if (pContext->nExists != pContext->nIDLEExists )
		{
	      /////////////////////////
	      //メッセージ総数
          pContext->nIDLEExists = pContext->nExists;
          sprintf(pContext->mMessages, "* %lu EXISTS\r\n", pContext->nExists);
/*		  
#ifdef ACCEPT_LOG_LEVEL3
	      LEVEL_3_ACCEPTLOG(lpClientContext, "IDLE_Thread():put_reply():Exists");
#endif
*/		  
#ifdef UPDATE_20150107 // 操作フォルダをアクセス中のセッションに変化をリアルタイムに同報するための対策(IDLE/NOOP)
          put_reply(lpClientContext, TRUE, TRUE);
	      if (bBroadcastSession) // 同一フォルダの操作情報を接続中のセッションに同報するオプション TRUE:有効 FALSE:無効
		  {
            PutBroadcastSession(lpClientContext); //pContext->mSelectDir, pContext->mMessages);
		  }
#else
          put_reply(lpClientContext, TRUE, TRUE);
#endif
		}
		if (pContext->nRecent != pContext->nIDLERecent)
		{
	      /////////////////////////
	      //新着メッセージ数
          pContext->nIDLERecent = pContext->nRecent;
          sprintf(pContext->mMessages, "* %lu RECENT\r\n", pContext->nRecent);
/*		  
#ifdef ACCEPT_LOG_LEVEL3
	      LEVEL_3_ACCEPTLOG(lpClientContext, "IDLE_Thread():put_reply():Recent");
#endif
*/		  
#ifdef UPDATE_20150107 // 操作フォルダをアクセス中のセッションに変化をリアルタイムに同報するための対策(IDLE/NOOP)
          put_reply(lpClientContext, TRUE, TRUE);
	      if (bBroadcastSession) // 同一フォルダの操作情報を接続中のセッションに同報するオプション TRUE:有効 FALSE:無効
		  {
            PutBroadcastSession(lpClientContext); //pContext->mSelectDir, pContext->mMessages);
		  }
#else
          put_reply(lpClientContext, TRUE, TRUE);
#endif
		}
#ifdef UPDATE_20110406A// 他の命令でフォルダ処理中はNOOPでフォルダチェックしない
	/*	
#ifdef ACCEPT_LOG_LEVEL3
		LEVEL_3_ACCEPTLOG(lpClientContext, "IDLE_Thread():UnLockMailSelectDirectory()");
#endif
	*/	
	    UnLockMailSelectDirectory(pContext->mSelectDir);
	  }
#endif
	}
	/*
#ifdef ACCEPT_LOG_LEVEL3
	LEVEL_3_ACCEPTLOG(lpClientContext, "IDLE_Thread():Sleep()");
#endif
	*/
    Sleep(1);
    //Sleep(nIDLELoop*1000); // １秒単位
  }
  pContext->bIDLEThread = FALSE;
#ifdef ACCEPT_LOG_LEVEL3
  LEVEL_3_ACCEPTLOG(lpClientContext, "IDLE_Thread():Exit.");
#endif
  //printf("[%08x] 2:IDLE_Thread():Exit.\n", lpClientContext);
#ifdef UPDATE_20141018 // IDLEスレッドの起動に失敗しハングする可能性の対策
#ifndef UPDATE_20141025 // DONEコマンド受信時にIDLEスレッドのハンドル値をリセットしてメモリ開放に失敗していた
  _endthreadex( 0 );
#endif
  return 0;
#endif
}
#endif