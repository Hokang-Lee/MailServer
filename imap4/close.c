////////////////////////////////////////////////////////////
// CLOSE.C Copyright K.kawakami
////////////////////////////////////////////////////////////
#include "imap4.h"

#ifdef UPDATE_20090130 // レシピエントファイル(RCP)があれば一緒にコピーさせる
extern BOOL    bIgnoreRCP; // TRUE: RCPファイル無視, FALSE:RCPファイル無視しない
#endif
extern BOOL bServiceTerminating;
#ifdef UPDATE_20190510 // NTFS以外のファイルシステムからのファイル一覧取得可能にする対策（一覧ソート）を追加
extern BOOL   bOtherFS; // TRUE:対応する FALSE:対応しない（旧仕様）
#endif

#ifdef UPDATE_20110404 // 選択フォルダ単位で排他する対策
HANDLE LockMailSelectDirectory(PCHAR  pszPath);
BOOL UnLockMailSelectDirectory(PCHAR  pszPath);
#endif

void DeleteMessage(PCLIENT_CONTEXT lpClientContext, BOOL bRes);

#ifdef UPDATE_20141220 // 操作フォルダをアクセス中のセッションに変化をリアルタイムに同報するための対策
extern BOOL  bBroadcastSession; // 同一フォルダの操作情報を接続中のセッションに同報するオプション TRUE:有効 FALSE:無効
void PutBroadcastSession(PCLIENT_CONTEXT lpClientContext);
#endif

BOOL CLOSEDispatch(PCLIENT_CONTEXT lpClientContext) {

  PImap4Context pContext = &lpClientContext->Context;

  if (pContext->State == Imap4SelectFolder) {  // フォルダ選択済状態
    DeleteMessage(lpClientContext, FALSE); // メッセージ削除

	pContext->State = Imap4Authorization; // 認証済状態へ遷移
    sprintf(pContext->mMessages, "%s %s %s completed\r\n", pContext->pSquence, IMAP4_GOOD_RESPONSE, pContext->pCmd );
  } else { // 状態が違う
    sprintf(pContext->mMessages, "%s %s Command unrecognized: %s\r\n", pContext->pSquence, IMAP4_RESULT_RESPONSE, pContext->pCmd );
  }
  return TRUE;
}

void DeleteMessage(PCLIENT_CONTEXT lpClientContext, BOOL bRes) {
#ifdef UPDATE_20190510 // NTFS以外のファイルシステムからのファイル一覧取得可能にする対策（一覧ソート）を追加
  DWORD            nSearch, no;
  WIN32_FIND_DATA  *pfd;
#endif
  HANDLE           hSearch;
  WIN32_FIND_DATA  FindData;
  PImap4Context    pContext = &lpClientContext->Context;
  CHAR             mUID[32];
  CHAR             MailDir[MAX_PATH], mWork[MAX_PATH];
  DWORD            nMSG = 0;
#ifdef UPDATE_20090130 // レシピエントファイル(RCP)があれば一緒にコピーさせる
  CHAR             *pRcp, mRCP[MAX_PATH];
#endif

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

#ifdef UPDATE_20110302A // フラグを変更対象のファイル名の一覧表を作成後にリネーム処理し、"~SG"が残されないようにする対策。
    sprintf(MailDir, "%s\\*-??????.MSG", pContext->mSelectDir);  // メッセージフォルダ取出し
#else
#ifdef UPDATE_20110203A //フラグ変更中に新着の検出が発生したとき最後のUIDを正しく取得できないタイミングがある不具合
    sprintf(MailDir, "%s\\*-??????.?SG", pContext->mSelectDir);  // メッセージフォルダ取出し
#else
    sprintf(MailDir, "%s\\*-??????.MSG", pContext->mSelectDir);  // メッセージフォルダ取出し
#endif
#endif
#ifdef UPDATE_20190510 // NTFS以外のファイルシステムからのファイル一覧取得可能にする対策（一覧ソート）を追加
   if (bOtherFS) // TRUE:対応する FALSE:対応しない（旧仕様）
   {
     pfd = FindFirstFileSort((LPCTSTR)MailDir, &FindData, &nSearch);
     if (!pfd)
     {
#ifdef UPDATE_20110404 // 選択フォルダ単位で排他する対策
        UnLockMailSelectDirectory(pContext->mSelectDir);
#endif
        return;
     }
     no = 0;
   } else {
     hSearch = FindFirstFile((LPCTSTR)MailDir, &FindData);
     if (hSearch == INVALID_HANDLE_VALUE){
#ifdef UPDATE_20110404 // 選択フォルダ単位で排他する対策
       UnLockMailSelectDirectory(pContext->mSelectDir);
#endif
       return;
     }
   }
#else
    hSearch = FindFirstFile((LPCTSTR)MailDir, &FindData);
    if (hSearch == INVALID_HANDLE_VALUE){
#ifdef UPDATE_20110404 // 選択フォルダ単位で排他する対策
      UnLockMailSelectDirectory(pContext->mSelectDir);
#endif
      return;
    }
#endif
    do {
        if (FindData.dwFileAttributes & (FILE_ATTRIBUTE_DIRECTORY |
                                         FILE_ATTRIBUTE_SYSTEM |
                                         FILE_ATTRIBUTE_HIDDEN) ) {
            continue;   // ファイル以外はSkip non-normal files
        }
		if (strlen(FindData.cFileName) != 21) {
		   continue;    // ファイル名の長さが違うものは無視
		}
		nMSG++; // MSGファイル数のカウント
		if (FindData.cFileName[UIDLEN+DELETED] == '1') {// 削除フラグセットなら
          sprintf(mWork, "%s\\%s", pContext->mSelectDir, FindData.cFileName);
#ifdef UPDATE_20090130 // レシピエントファイル(RCP)があれば一緒にコピーさせる
         if (!bIgnoreRCP) // TRUE: RCPファイル無視, FALSE:RCPファイル無視しない
		 {
		    strcpy(mRCP, mWork);
            if (pRcp = strrchr(mRCP, '.'))
			{
			  *pRcp = '\x0';
			  strcat(mRCP, ".RCP");
              DeleteFile(mRCP);
			}
		  }
#endif
		  DeleteFile(mWork);
		  if (bRes) {  // 削除を通知する
		    strcpy(mUID, FindData.cFileName);
            strtok(mUID, "-");
            sprintf(pContext->mMessages, "* %lu EXPUNGE\r\n", nMSG);
#ifdef UPDATE_20141220 // 操作フォルダをアクセス中のセッションに変化をリアルタイムに同報するための対策
            put_reply(lpClientContext, TRUE, TRUE);
		    if (bBroadcastSession) // 同一フォルダの操作情報を接続中のセッションに同報するオプション TRUE:有効 FALSE:無効
			{
              PutBroadcastSession(lpClientContext);
			}
#else
            put_reply(lpClientContext, TRUE, TRUE);
#endif
		  }
		  nMSG--;    // MSGファイル数をデクリメント
		  pContext->nExists--;
		}
#ifdef UPDATE_20190510 // NTFS以外のファイルシステムからのファイル一覧取得可能にする対策（一覧ソート）を追加
        if (bOtherFS) // TRUE:対応する FALSE:対応しない（旧仕様）
  	    {
		  no++;
		  if (no < nSearch) {
		    FindNextFileSort(&FindData, pfd+no); // &pContext->FindData, pfd+no);
	      }
	    }
    } while(bOtherFS ? (no < nSearch) : FindNextFile(hSearch, &FindData) ); //&pContext->FindData) );
    if (bOtherFS)
      FindCloseSort(pfd);
    else
  	  FindClose(hSearch);
#else
    } while (FindNextFile(hSearch, &FindData));
	FindClose(hSearch);
#endif
#ifdef UPDATE_20110404 // 選択フォルダ単位で排他する対策
    UnLockMailSelectDirectory(pContext->mSelectDir);
#endif

}
