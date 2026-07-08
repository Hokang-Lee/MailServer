////////////////////////////////////////////////////////////
// EXAMINE.C Copyright K.kawakami
////////////////////////////////////////////////////////////
#include "imap4.h"

/*
extern CHAR    mInboxAlias[];

BOOL MakeUIDVALIDITY(PImap4Context pContext);
void MoveMSGFile(PCLIENT_CONTEXT lpClientContext);
void GetMSGFlags(PCLIENT_CONTEXT lpClientContext);
DWORD GetFolderUID(char *pFolder, DWORD nUID);
#ifdef UPDATE_20160823 // APPEND命令で存在しない追加フォルダを指定すると処理が終了できなくなる不具合
BOOL PutFolderUID(char *pFolder, DWORD nUID);
#else
void PutFolderUID(char *pFolder, DWORD nUID);
#endif

BOOL EXAMINEDispatch(PCLIENT_CONTEXT lpClientContext) {
  char *p, *q;
  PImap4Context pContext = &lpClientContext->Context;
  BOOL bShared = FALSE;

  if (pContext->State >= Imap4Authorization) {
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
#ifdef UPDATE_20141220 // 操作フォルダをアクセス中のセッションに変化をリアルタイムに同報するための対策
    if (bBroadcastSession) // 同一フォルダの操作情報を接続中のセッションに同報するオプション TRUE:有効 FALSE:無効
	{
      LeaveBroadcastSession(lpClientContext);
	}
#endif
    pContext->bShared = FALSE;
	if (!strncmp(p, mInboxAlias, strlen(mInboxAlias))) {// 受信トレイの別名称の場合
	  q = strpbrk(p, "/\\");
	  sprintf(pContext->mSelectDir,"%sINBOX%s", pContext->mRootDir, (q ? q : ""));
	} else if (pContext->mSharedRoot[0] && !strnicmp(p, IMAP4_SHARED_NAME, 8)) { // 共通フォルダの指定
      q = strpbrk(p, "/\\");
	  sprintf(pContext->mSelectDir,"%s%s", pContext->mSharedRoot, ++q);
	  pContext->bShared = TRUE;
	} else {
#ifdef UPDATE_20050531  // 命令にバッファフローの脆弱性
	  strcpy(pContext->mSelectDir, pContext->mRootDir);
	  strncat(pContext->mSelectDir, p, sizeof(pContext->mSelectDir)-strlen(pContext->mRootDir)-1);
	  pContext->mSelectDir[sizeof(pContext->mSelectDir)-1] = '\x0';
#else
	  sprintf(pContext->mSelectDir,"%s%s", pContext->mRootDir, p);
#endif
	}
	if (MakeUIDVALIDITY(pContext)) { // フォルダの存在有無とユニーク識別子有効値生成
#ifdef UPDATE_20110405A // UID値の排他処理強化
      CriticalUID(lpClientContext, p);
#else
      pContext->nUid = GetFolderUID(pContext->mSelectDir, pContext->nUid);
      if (!stricmp(p, "inbox")) // INBOXフォルダ選択なら
        MoveMSGFile(lpClientContext);
	  GetMSGFlags(lpClientContext); // フラグ集計
#endif
	  /////////////////////////
	  //メッセージ総数
      sprintf(pContext->mMessages, "* %lu EXISTS\r\n", pContext->nExists);
      put_reply(lpClientContext, TRUE, TRUE);
	  /////////////////////////
	  //新着メッセージ数
      sprintf(pContext->mMessages, "* %lu RECENT\r\n", pContext->nRecent);
      put_reply(lpClientContext, TRUE, TRUE);
	  /////////////////////////
	  //未読メッセージ数
      sprintf(pContext->mMessages, "* %s [UNSEEN %lu] Message %lu is first unseen\r\n", IMAP4_GOOD_RESPONSE, pContext->nUnseen, pContext->nUnseen); //\Seen フラグがセットされていないメッセージの数。 
      put_reply(lpClientContext, TRUE, TRUE);
	  /////////////////////////////
	  //新着メッセージのUID番号一覧(10進数の数字が続き、ユニーク識別子有効値を示す。)
      sprintf(pContext->mMessages, "* %s [UIDVALIDITY %lu] UIDs valid\r\n", IMAP4_GOOD_RESPONSE, pContext->nUidValidity); //メッセージユニーク識別子
      put_reply(lpClientContext, TRUE, TRUE);
	  /////////////////////////////
	  //次に割り当てられるUID番号
      sprintf(pContext->mMessages, "* %s [UIDNEXT %lu] Predicted next UID\r\n", IMAP4_GOOD_RESPONSE, pContext->nUid); // 次のUID値
      put_reply(lpClientContext, TRUE, TRUE);
	  /////////////////////////////
	  //メールボックスで定義されるフラグ一覧
      sprintf(pContext->mMessages, "* FLAGS (\\Answered \\Flagged \\Deleted \\Seen \\Draft)\r\n"); 
      put_reply(lpClientContext, TRUE, TRUE);
	  /////////////////////////////
	  //利用可能なフラグ
      sprintf(pContext->mMessages, "* %s [PERMANENTFLAGS ()] Limited\r\n", IMAP4_GOOD_RESPONSE); //フラグ括弧つき一覧 
      put_reply(lpClientContext, TRUE, TRUE);
	  /////////////////////////////
	  //メールボックス選択結果の応答
	  if (pContext->bShared && !pContext->bSharedRW)
        sprintf(pContext->mMessages, "%s %s [READ-ONLY] %s completed\r\n", pContext->pSquence, IMAP4_GOOD_RESPONSE, pContext->pCmd );
	  else
        sprintf(pContext->mMessages, "%s %s [READ-ONLY] %s completed\r\n", pContext->pSquence, IMAP4_GOOD_RESPONSE, pContext->pCmd );
      pContext->State = Imap4SelectFolder;                  // フォルダ選択済状態
	} else { // フォルダが存在しない場合カレント非選択後エラー回答
  	  sprintf(pContext->mSelectDir,"%s", pContext->mRootDir);
      sprintf(pContext->mMessages, "%s %s No such mailbox\r\n", pContext->pSquence, IMAP4_RESULT_RESPONSE);
	}
  } else { // 状態が違う
    sprintf(pContext->mMessages, "%s %s Command unrecognized: %s\r\n", pContext->pSquence, IMAP4_RESULT_RESPONSE, pContext->pCmd );
  }
  return TRUE;
}

*/

extern BOOL  bServiceTerminating;
extern CHAR    mInboxAlias[];
extern CHAR    mMailBoxDir[];

#ifdef UPDATE_20110222 //UPDATE_20110202でパフォーマンス改善の対策。
extern BOOL   bUIDRecover;
#endif
#ifdef UPDATE_20110202 //IMAPフォルダ内の連番構造で重複ファイル名が発生した場合の回復機能の追加
unsigned int getLastNo(char *pFolder);
void recoveid(char *pFolder, unsigned int nLastNo);
#endif
#ifdef UPDATE_20110304 // v4.34(フラグ変更の為の拡張子変更)使用している場合のバージョン差替え時の拡張子修復対策。
void recover_Fast_SELECT(char *pFolder);
#endif
#ifdef UPDATE_20110404 // 選択フォルダ単位で排他する対策
HANDLE LockMailSelectDirectory(PCHAR  pszPath);
BOOL UnLockMailSelectDirectory(PCHAR  pszPath);
#endif

BOOL MakeUIDVALIDITY(PImap4Context pContext);
void MoveMSGFile(PCLIENT_CONTEXT lpClientContext);
void GetMSGFlags(PCLIENT_CONTEXT lpClientContext);
DWORD GetFolderUID(char *pFolder, DWORD nUID);
#ifdef UPDATE_20160823 // APPEND命令で存在しない追加フォルダを指定すると処理が終了できなくなる不具合
BOOL PutFolderUID(char *pFolder, DWORD nUID);
#else
void PutFolderUID(char *pFolder, DWORD nUID);
#endif
void LEVEL_3_ACCEPTLOG(PCLIENT_CONTEXT lpClientContext, char *mess);
#ifdef UPDATE_20141220 // 操作フォルダをアクセス中のセッションに変化をリアルタイムに同報するための対策
extern BOOL  bBroadcastSession; // 同一フォルダの操作情報を接続中のセッションに同報するオプション TRUE:有効 FALSE:無効
void JoinBroadcastSession(PCLIENT_CONTEXT lpClientContext);
void LeaveBroadcastSession(PCLIENT_CONTEXT lpClientContext);
void PutBroadcastSession(PCLIENT_CONTEXT lpClientContext);
#endif


#ifdef UPDATE_20101221 // UID値ファイルのクリティカルセクション化
extern CRITICAL_SECTION g_csWriteUID;
#endif

BOOL EXAMINEDispatch(PCLIENT_CONTEXT lpClientContext) {
  char *p, *q;
  PImap4Context pContext = &lpClientContext->Context;
  BOOL bShared = FALSE;
#ifdef LINK_FOLDER
  FILE *fp;
#endif
#ifdef UPDATE_20110202 //IMAPフォルダ内の連番構造で重複ファイル名が発生した場合の回復機能の追加
  unsigned int nLastNo;
#endif
#ifdef UPDATE_20171230 // EXAMINE命令のフォルダ名に末尾に半角スペースが含まれると、フォルダ移動が永久ループする不具合
  int  i, j;
#endif

  if (pContext->State >= Imap4Authorization) {
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
#ifdef UPDATE_20141220 // 操作フォルダをアクセス中のセッションに変化をリアルタイムに同報するための対策
    if (bBroadcastSession) // 同一フォルダの操作情報を接続中のセッションに同報するオプション TRUE:有効 FALSE:無効
	{
      LeaveBroadcastSession(lpClientContext);
	}
#endif
    pContext->bShared = FALSE;
	if (!strncmp(p, mInboxAlias, strlen(mInboxAlias))) {// 受信トレイの別名称の場合
	  q = strpbrk(p, "/\\");
	  sprintf(pContext->mSelectDir,"%sINBOX%s", pContext->mRootDir, (q ? q : ""));
	} else if (pContext->mSharedRoot[0] && !strnicmp(p, IMAP4_SHARED_NAME, 8)) { // 共通フォルダの指定
      q = strpbrk(p, "/\\");
	  sprintf(pContext->mSelectDir,"%s%s", pContext->mSharedRoot, ++q);
	  pContext->bShared = TRUE;
	} else {
#ifdef UPDATE_20171230 // EXAMINE命令のフォルダ名に末尾に半角スペースが含まれると、フォルダ移動が永久ループする不具合
	  i = strlen(p);
#ifdef UPDATE_20210604 // 指定フォルダがフルパスで230byteを超えているとハングする不具合。
	  if (i+ strlen(pContext->mRootDir) > 230)
	  {
	    *(p+(230-strlen(pContext->mRootDir))) = '\x0';
	    i = strlen(p);
	  }
#endif
	  for (j = i-1; j >= 0 ; j--)
	  {
		if (*(p+j) == ' ')
		{
		  *(p+j) = '\x0';
		} else {
		  break;
		}
	  }
#endif
	  sprintf(pContext->mSelectDir,"%s%s", pContext->mRootDir, p);
#ifdef LINK_FOLDER
	  if ((fp = fopen(pContext->mSelectDir, "rt"))) { // リンクが貼られている場合
		fgets(pContext->mSelectDir, sizeof(pContext->mSelectDir), fp);
		strtok(pContext->mSelectDir, "\n");
		strcat(pContext->mSelectDir, p);
		fclose(fp);
	  }
#endif
	}
#ifdef UPDATE_20141225 // フォルダ切り替えで排他フラグがリセットさせる対策
    lpClientContext->bPutLock = FALSE;
#endif
#ifdef UPDATE_20110202 //IMAPフォルダ内の連番構造で重複ファイル名が発生した場合の回復機能の追加
#ifdef UPDATE_20110222 //UPDATE_20110202でパフォーマンス改善の対策。
#ifdef UPDATE_20110302A // フラグを変更対象のファイル名の一覧表を作成後にリネーム処理し、"~SG"が残されないようにする対策。
	if (bUIDRecover)
#else
#ifndef UPDATE_20110228 // サービス停止のタイミングでフラグ変更中のファイルの復旧対策
	if (bUIDRecover)
#endif
#endif
#endif
	{
//printf("Command(1) = [%s]\n", pContext->pCmd);
#ifdef UPDATE_20110204 // ファイル操作のファイルのクリティカルセクション化
#ifdef UPDATE_20110404 // 選択フォルダ単位で排他する対策
        while(!LockMailSelectDirectory(pContext->mSelectDir)) // Lockファイルがある場合は何もしない。
#else
        while(!LockMailDirectory(pContext->mUSER, pContext->MyAddr)) // Lockファイルがある場合は何もしない。
#endif
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
#ifdef UPDATE_20110326 // サービス起動時最初のフォルダ選択について判定し最初ならリカバリ処理を実施する
        if (Start_for_Fast_SELECT(pContext->mSelectDir))
#endif
		{
#ifdef UPDATE_20110325 // 回復機能をデフォルトに変更
          recover_Fast_SELECT(pContext->mSelectDir);
#endif
		}
//printf("Command(2.1) = [%s]\n", pContext->pCmd);
        nLastNo = getLastNo(pContext->mSelectDir) + 1;
//printf("Command(2.2) = [%s]\n", pContext->pCmd);
        recoveid(pContext->mSelectDir, nLastNo);
//printf("Command(2.3) = [%s]\n", pContext->pCmd);
#ifdef UPDATE_20110204 // ファイル操作のファイルのクリティカルセクション化
#ifdef UPDATE_20110404 // 選択フォルダ単位で排他する対策
        UnLockMailSelectDirectory(pContext->mSelectDir);
#else
        UnLockMailDirectory(pContext->mUSER, pContext->MyAddr);
#endif
#endif
#ifdef UPDATE_20110304 // v4.34(フラグ変更の為の拡張子変更)使用している場合のバージョン差替え時の拡張子修復対策。
	} else {
#ifdef UPDATE_20110326 // サービス起動時最初のフォルダ選択について判定し最初ならリカバリ処理を実施する
      if (Start_for_Fast_SELECT(pContext->mSelectDir))
#endif
	  {
#ifdef UPDATE_20110326 // サービス起動時最初のフォルダ選択について判定し最初ならリカバリ処理を実施する
#ifdef UPDATE_20110204 // ファイル操作のファイルのクリティカルセクション化
#ifdef UPDATE_20110404 // 選択フォルダ単位で排他する対策
        while(!LockMailSelectDirectory(pContext->mSelectDir)) // Lockファイルがある場合は何もしない。
#else
        while(!LockMailDirectory(pContext->mUSER, pContext->MyAddr)) // Lockファイルがある場合は何もしない。
#endif
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
        recover_Fast_SELECT(pContext->mSelectDir);
//printf("Command(2.1) = [%s]\n", pContext->pCmd);
        nLastNo = getLastNo(pContext->mSelectDir) + 1;
//printf("Command(2.2) = [%s]\n", pContext->pCmd);
        recoveid(pContext->mSelectDir, nLastNo);
//printf("Command(2.3) = [%s]\n", pContext->pCmd);
#ifdef UPDATE_20110204 // ファイル操作のファイルのクリティカルセクション化
#ifdef UPDATE_20110404 // 選択フォルダ単位で排他する対策
        UnLockMailSelectDirectory(pContext->mSelectDir);
#else
        UnLockMailDirectory(pContext->mUSER, pContext->MyAddr);
#endif
#endif
#endif
	  }
#endif
	}
#endif
//printf("Command(3) = [%s]\n", pContext->pCmd);
	if (MakeUIDVALIDITY(pContext)) { // フォルダの存在有無とユニーク識別子有効値生成
#ifdef UPDATE_20110405A // UID値の排他処理強化
      CriticalUID(lpClientContext, p);
#else
#ifdef UPDATE_20110404 // 選択フォルダ単位で排他する対策
      while(!LockMailSelectDirectory(pContext->mSelectDir)) // Lockファイルがある場合は何もしない。
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
      pContext->nUid = GetFolderUID(pContext->mSelectDir, pContext->nUid);
#ifdef UPDATE_20110404 // 選択フォルダ単位で排他する対策
      UnLockMailSelectDirectory(pContext->mSelectDir);
#endif
      if (!stricmp(p, "inbox")) // INBOXフォルダ選択なら
        MoveMSGFile(lpClientContext);
	  GetMSGFlags(lpClientContext); // フラグ集計
#endif
#ifdef UPDATE_20140528 // IDLEコマンド実装
	  pContext->bIDLE = FALSE; //IDLEスレッド一旦破棄
#ifdef UPDATE_20141025 // DONEコマンド受信時にIDLEスレッドのハンドル値をリセットしてメモリ開放に失敗していた
	  Sleep(1);
	  if (pContext->hIDLEThread != NULL && pContext->hIDLEThread != -1L)
	  {
	    WaitForSingleObject(pContext->hIDLEThread, INFINITE);
        CloseHandle(pContext->hIDLEThread);
		pContext->hIDLEThread = NULL;
	    pContext->mIDLESquence[0] = '\x0';
	  }
#endif
	  pContext->nIDLEExists = pContext->nExists;
      pContext->nIDLERecent = pContext->nRecent;
	  /*
	  if (pContext->hIDLEThread)
	  {
	    TerminateThread(pContext->hIDLEThread, 0);   // スレッドのハンドル
	  }
	  */
#endif
	  /////////////////////////
	  //メッセージ総数
      sprintf(pContext->mMessages, "* %lu EXISTS\r\n", pContext->nExists);
      put_reply(lpClientContext, TRUE, TRUE);
	  /////////////////////////
	  //新着メッセージ数
      sprintf(pContext->mMessages, "* %lu RECENT\r\n", pContext->nRecent);
      put_reply(lpClientContext, TRUE, TRUE);
	  /////////////////////////
	  //未読メッセージ数
      sprintf(pContext->mMessages, "* %s [UNSEEN %lu] Message %lu is first unseen\r\n", IMAP4_GOOD_RESPONSE, pContext->nUnseen, pContext->nUnseen); //\Seen フラグがセットされていないメッセージの数。 
      put_reply(lpClientContext, TRUE, TRUE);
	  /////////////////////////////
	  //新着メッセージのUID番号一覧(10進数の数字が続き、ユニーク識別子有効値を示す。)
      sprintf(pContext->mMessages, "* %s [UIDVALIDITY %lu] UIDs valid\r\n", IMAP4_GOOD_RESPONSE, pContext->nUidValidity); //メッセージユニーク識別子
      put_reply(lpClientContext, TRUE, TRUE);
	  /////////////////////////////
	  //次に割り当てられるUID番号
      sprintf(pContext->mMessages, "* %s [UIDNEXT %lu] Predicted next UID\r\n", IMAP4_GOOD_RESPONSE, pContext->nUid); // 次のUID値
      put_reply(lpClientContext, TRUE, TRUE);
	  /////////////////////////////
	  //メールボックスで定義されるフラグ一覧
      sprintf(pContext->mMessages, "* FLAGS (\\Answered \\Flagged \\Deleted \\Seen \\Draft)\r\n"); 
      put_reply(lpClientContext, TRUE, TRUE);
	  /////////////////////////////
	  //利用可能なフラグ
#ifdef ADD_UNLIMITED
      sprintf(pContext->mMessages, "* %s [PERMANENTFLAGS (\\Answered \\Flagged \\Deleted \\Seen \\Draft \\*)] Unlimited\r\n", IMAP4_GOOD_RESPONSE); //フラグ括弧つき一覧 
#else
#ifdef UPDATE_20130625 // $Forwarded フラグを利用可能にしたのとThunderbardでAnsweredフラグがセットされない対策
      sprintf(pContext->mMessages, "* %s [PERMANENTFLAGS (\\Answered \\Flagged \\Deleted \\Seen \\Draft \\*)] Limited\r\n", IMAP4_GOOD_RESPONSE); //フラグ括弧つき一覧 
#else
      sprintf(pContext->mMessages, "* %s [PERMANENTFLAGS (\\Deleted \\Seen \\*)] Limited\r\n", IMAP4_GOOD_RESPONSE); //フラグ括弧つき一覧 
#endif
#endif
      put_reply(lpClientContext, TRUE, TRUE);
	  /////////////////////////////
	  //メールボックス選択結果の応答
	  if (pContext->bShared && !pContext->bSharedRW)
        sprintf(pContext->mMessages, "%s %s [READ-ONLY] %s completed\r\n", pContext->pSquence, IMAP4_GOOD_RESPONSE, pContext->pCmd );
	  else
        sprintf(pContext->mMessages, "%s %s [READ-ONLY] %s completed\r\n", pContext->pSquence, IMAP4_GOOD_RESPONSE, pContext->pCmd );
      pContext->State = Imap4SelectFolder;                  // フォルダ選択済状態
	} else { // フォルダが存在しない場合カレント非選択後エラー回答
  	  sprintf(pContext->mSelectDir,"%s", pContext->mRootDir);
      sprintf(pContext->mMessages, "%s %s No such mailbox\r\n", pContext->pSquence, IMAP4_RESULT_RESPONSE);
	}
  } else { // 状態が違う
    sprintf(pContext->mMessages, "%s %s Command unrecognized: %s\r\n", pContext->pSquence, IMAP4_RESULT_RESPONSE, pContext->pCmd );
  }
  return TRUE;
}
