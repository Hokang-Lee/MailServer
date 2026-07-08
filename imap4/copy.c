////////////////////////////////////////////////////////////
// COPY.C Copyright K.kawakami
////////////////////////////////////////////////////////////
#include "imap4.h"

extern BOOL bServiceTerminating;
extern BOOL  bDebug;
extern CHAR  mInboxAlias[];
#ifdef UPDATE_20090130 // レシピエントファイル(RCP)があれば一緒にコピーさせる
extern BOOL    bIgnoreRCP; // TRUE: RCPファイル無視, FALSE:RCPファイル無視しない
#endif
#ifdef UPDATE_20190510 // NTFS以外のファイルシステムからのファイル一覧取得可能にする対策（一覧ソート）を追加
extern BOOL   bOtherFS; // TRUE:対応する FALSE:対応しない（旧仕様）
#endif

DWORD MSGIDDecipher(PCLIENT_CONTEXT lpClientContext, char *pRequest, char **pDec);
BOOL  CopyMSGByFiles(PCLIENT_CONTEXT lpClientContext, char *pControl, char *pDest1, char *pDest2);
DWORD GetFolderUID(char *pFolder, DWORD nUID);
#ifdef UPDATE_20160823 // APPEND命令で存在しない追加フォルダを指定すると処理が終了できなくなる不具合
BOOL PutFolderUID(char *pFolder, DWORD nUID);
#else
void PutFolderUID(char *pFolder, DWORD nUID);
#endif
#ifdef UPDATE_20110404 // 選択フォルダ単位で排他する対策
HANDLE LockMailSelectDirectory(PCHAR  pszPath);
BOOL UnLockMailSelectDirectory(PCHAR  pszPath);
#endif
#ifdef UPDATE_20151225 // 試作：UID値によるファイル範囲指定のチューニング
void GetQueryFileArea(CHAR *pMailDir, CHAR *pBaseFolder, BOOL bUID, BOOL bFromTo, DWORD nFrom, DWORD nTo);
#endif

BOOL COPYDispatch(PCLIENT_CONTEXT lpClientContext) {
  DWORD   nMESDec = 0;
  char    *pMESDec[MSGID_MAX_ATTRIBUTE];
  char    *p, *q, *pControl, *pDest1, *pDest2 = NULL;
  PImap4Context pContext = &lpClientContext->Context;
  BOOL    bCopy = TRUE;

  if (pContext->State == Imap4SelectFolder) {  // フォルダ選択済状態
	for (nMESDec = 0; nMESDec < MSGID_MAX_ATTRIBUTE; nMESDec++) // 構造初期化
	  pMESDec[nMESDec] = NULL;
	pControl    = pContext->pToken;
	pDest1      = strstr(pContext->pToken, " ");
	if (pDest1) {
	  *pDest1 = '\x0';
	  pDest1++;
	  if (*pDest1 == '"') {
	    pDest1++;
	    strtok(pDest1, "\"");
        if (*pDest1 == '"')
	      *pDest1 = '\x0';
	  }
#ifdef UPDATE_20050531  // 命令にバッファフローの脆弱性
	  if (strlen(pDest1) > MAXIMAPFOLDER) // フォルダ名が長い場合の処置
		*(pDest1+MAXIMAPFOLDER) = '\x0';
#endif
	  if (!strncmp(pDest1, mInboxAlias, strlen(mInboxAlias))) {// 受信トレイの別名称の場合
	    pDest2 = strpbrk(pDest1, "/\\");
	  }
	}
	if (*pControl == '"') {
	   pControl++;
	   strtok(pControl, "\"");
       if (*pControl == '"')
         *pControl = '\x0';
	}
#ifdef UPDATE_20040428
	q = pControl;
	do {
	  if ((p = strstr(q, ","))) {
	    *p = '\x0';
		p++;
		if (p) {
          bCopy = CopyMSGByFiles(lpClientContext, q, pDest1, pDest2);
		  q = p;
	      if (!bCopy)
		    break;
		}
	  } else {
        bCopy = CopyMSGByFiles(lpClientContext, q, pDest1, pDest2);
	  }
	} while(p);
#else
    MSGIDDecipher(lpClientContext, pControl, pMESDec);
	nMESDec = 0;
    while(pMESDec[nMESDec]) {
      bCopy = CopyMSGByFiles(lpClientContext, pMESDec[nMESDec], pDest1, pDest2);
	  if (!bCopy)
		break;
      nMESDec++;
	  if (nMESDec >= MSGID_MAX_ATTRIBUTE) // 制限領域オーバー
		break;
	}
#endif	
	if (bCopy)
      sprintf(pContext->mMessages, "%s %s %s completed\r\n", pContext->pSquence, IMAP4_GOOD_RESPONSE, pContext->pCmd );
	else
      sprintf(pContext->mMessages, "%s %s %s doesn't have right\r\n", pContext->pSquence, IMAP4_RESULT_RESPONSE, pContext->pCmd );
  } else { // 状態が違う
    sprintf(pContext->mMessages, "%s %s Command unrecognized: %s\r\n", pContext->pSquence, IMAP4_RESULT_RESPONSE, pContext->pCmd );
  }
  return TRUE;
}

BOOL CopyMSGByFiles(PCLIENT_CONTEXT lpClientContext, char *pControl, char *pDest1, char *pDest2) {
#ifdef UPDATE_20190510 // NTFS以外のファイルシステムからのファイル一覧取得可能にする対策（一覧ソート）を追加
  DWORD            nSearch, no;
  WIN32_FIND_DATA  *pfd;
#endif
  HANDLE           hSearch;
  PImap4Context    pContext = &lpClientContext->Context;
  CHAR             MailDir[MAX_PATH];
  CHAR             mSrc[MAX_PATH], mDest[MAX_PATH];
#ifdef UPDATE_20090130 // レシピエントファイル(RCP)があれば一緒にコピーさせる
  CHAR             *pRcp, mSrcRCP[MAX_PATH], mDestRCP[MAX_PATH];
#endif
  CHAR             mUID[32];
  //CHAR             mFlags[7] = {"??????"};
  CHAR             *q, *p, *p2;
  DWORD            nNo = 0, nMSG = 0, nFrom = 0, nTo = 0, nUID = 1;
  BOOL             bSilent = FALSE, bSet = FALSE;
  BOOL   	       bFromTo = FALSE;
  BOOL             bRecent = FALSE, bFlagged = FALSE, bAnswered = FALSE;
  BOOL             bDeleted = FALSE, bDraft = FALSE, bSeen = FALSE;
  BOOL             bShared = FALSE;
  BOOL             bSts = TRUE;
  FILE             *fp;

#ifdef UPDATE_20110228B // 誤った構造のCOPYコマンドを送るとハングする
   if (!pDest1)
   {
     return FALSE;
   }
#endif
    p = pControl;
    bFromTo = TRUE;
    if ((p2 = strstr(p, ":"))) {
      *p2 = '\x0';
      p2++;
#ifdef UPDATE_20151218 // 範囲指定の*のとき処理の修正
	  if (*p == '*')
	    nFrom = (pContext->bUID ?  pContext->nUid-1 : pContext->nExists);
	  else
#endif
        nFrom = atol(p);
#ifdef UPDATE_20151218 // 範囲指定の*のとき処理の修正
	  if (*p2 == '*')
	    nTo = (pContext->bUID ?  pContext->nUid-1 : pContext->nExists);
	  else
#endif
        nTo = atol(p2);
    } else {
#ifdef UPDATE_20151218 // 範囲指定の*のとき処理の修正
	   if (*p == '*')
	 	 nFrom = nTo = (pContext->bUID ?  pContext->nUid-1 : pContext->nExists);
	   else
#endif
 	     nFrom = nTo = atol(p);
    }
    //if (nFrom == nTo && pContext->bUID)  // １件だけ
    //  sprintf(MailDir, "%s\\%010u-??????.MSG", pContext->mSelectDir, nFrom);  // メッセージフォルダ取出し
	//else               // 複数件
#ifdef UPDATE_20151225 // 試作：UID値によるファイル範囲指定のチューニング
    GetQueryFileArea(MailDir, pContext->mSelectDir, pContext->bUID, bFromTo, nFrom, nTo);
#else
    sprintf(MailDir, "%s\\*.MSG", pContext->mSelectDir);  // メッセージフォルダ取出し
#endif
#ifdef UPDATE_20190510 // NTFS以外のファイルシステムからのファイル一覧取得可能にする対策（一覧ソート）を追加
   if (bOtherFS) // TRUE:対応する FALSE:対応しない（旧仕様）
   {
     pfd = FindFirstFileSort((LPCTSTR)MailDir, &pContext->FindData, &nSearch);
     if (!pfd)
     {
       return bSts;
     }
     no = 0;
   } else {
     hSearch = FindFirstFile((LPCTSTR)MailDir, &pContext->FindData);
     if (hSearch == INVALID_HANDLE_VALUE){
       return bSts;
     }
   }
#else
    hSearch = FindFirstFile((LPCTSTR)MailDir, &pContext->FindData);
    if (hSearch == INVALID_HANDLE_VALUE){
      return bSts;
    }
#endif
#ifdef UPDATE_20110404 // 選択フォルダ単位で排他する対策
    if (!strncmp(pDest1, mInboxAlias, strlen(mInboxAlias))) {
      sprintf(mDest, "%sINBOX%s", pContext->mRootDir, (pDest2 ? pDest2 : ""));
	} else if (pContext->mSharedRoot[0] && !strnicmp(pDest1, IMAP4_SHARED_NAME, 8)) { // 共通フォルダの指定
      q = strpbrk(pDest1,"/\\");
	  sprintf(mDest, "%s%s", pContext->mSharedRoot, ++q);
	} else {
#ifdef UPDATE_20050531  // 命令にバッファフローの脆弱性
      strcpy(mDest, pContext->mRootDir);
      strncat(mDest, pDest1, MAX_PATH-strlen(pContext->mRootDir)-1);
      mDest[MAX_PATH-1] = '\x0';
#else
      sprintf(mDest, "%s%s", pContext->mRootDir, pDest1);
#endif
    }
    while(!LockMailSelectDirectory(mDest)) // Lockファイルがある場合は何もしない。
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
    do {
        if (pContext->FindData.dwFileAttributes & (FILE_ATTRIBUTE_DIRECTORY |
                                         FILE_ATTRIBUTE_SYSTEM |
                                         FILE_ATTRIBUTE_HIDDEN) ) {
            continue;   // ファイル以外はSkip non-normal files
        }
		if (strlen(pContext->FindData.cFileName) != 21) {
		   continue;    // ファイル名の長さが違うものは無視
		}
		nNo++;
		if (pContext->bUID) { // MSGのUID
		  strcpy(mUID, pContext->FindData.cFileName);
          strtok(mUID, "-");
		  nMSG = (DWORD) atol(mUID);
		} else {
		  nMSG++; // MSGファイル数のカウント
		}
#ifdef UPDATE_20151217 // 範囲指定に0を指定するとワイルドカード処理されてしまう不具合
		if (bFromTo && nFrom <= nMSG && nTo >= nMSG) 
#else
		if ( (!bFromTo || (bFromTo && nFrom <= nMSG && (nTo == 0 || nTo >= nMSG) )) ) 
#endif
		{
		  sprintf(mSrc, "%s\\%s", pContext->mSelectDir, pContext->FindData.cFileName);
#ifdef UPDATE_20090130 // レシピエントファイル(RCP)があれば一緒にコピーさせる
          if (!bIgnoreRCP) // TRUE: RCPファイル無視, FALSE:RCPファイル無視しない
		  {
		    strcpy(mSrcRCP, mSrc);
            if (pRcp = strrchr(mSrcRCP, '.'))
			{
			  *pRcp = '\x0';
			  strcat(mSrcRCP, ".RCP");
			} else {
			  mSrcRCP[0] = '\x0';
			}
		  }
#endif
          if (!strncmp(pDest1, mInboxAlias, strlen(mInboxAlias))) {
		    sprintf(mDest, "%sINBOX%s", pContext->mRootDir, (pDest2 ? pDest2 : ""));
		  } else if (pContext->mSharedRoot[0] && !strnicmp(pDest1, IMAP4_SHARED_NAME, 8)) { // 共通フォルダの指定
            q = strpbrk(pDest1,"/\\");
		    sprintf(mDest, "%s%s", pContext->mSharedRoot, ++q);
		    bShared = TRUE;
		  } else {
#ifdef UPDATE_20050531  // 命令にバッファフローの脆弱性
	        strcpy(mDest, pContext->mRootDir);
	        strncat(mDest, pDest1, MAX_PATH-strlen(pContext->mRootDir)-1);
	        mDest[MAX_PATH-1] = '\x0';
#else
		    sprintf(mDest, "%s%s", pContext->mRootDir, pDest1);
#endif
		  }
#ifdef UPDATE_20060116  // ユーザルートより上位のフォルダにアクセスできる脆弱性３
          if (!FolderRight2(pContext, (char *)mDest)) {
#ifdef UPDATE_20110404 // 選択フォルダ単位で排他する対策
            UnLockMailSelectDirectory(mDest);
#endif
	        return FALSE;
		  }
#endif
          if (!bShared || bShared && pContext->bSharedRW) {
/*
#ifdef UPDATE_20110404 // 選択フォルダ単位で排他する対策
            while(!LockMailSelectDirectory(mDest)) // Lockファイルがある場合は何もしない。
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
            nUID = GetFolderUID(mDest, nUID);
/*
#ifdef UPDATE_20110404 // 選択フォルダ単位で排他する対策
            UnLockMailSelectDirectory(mDest);
#endif
*/
            if (!strncmp(pDest1, mInboxAlias, strlen(mInboxAlias))) {
#ifdef UPDATE_20110303 // UID値が0x7FFFFFFF(2147483647)を超えるとマイナス値で表示されてしまう不具合。
		      sprintf(mDest, "%sINBOX%s\\%010lu-%s", pContext->mRootDir, (pDest2 ? pDest2 : ""), nUID, &pContext->FindData.cFileName[UIDLEN+RECENT]); // フラグも踏襲
#else
		      sprintf(mDest, "%sINBOX%s\\%010ld-%s", pContext->mRootDir, (pDest2 ? pDest2 : ""), nUID, &pContext->FindData.cFileName[UIDLEN+RECENT]); // フラグも踏襲
#endif
			} else if (pContext->mSharedRoot[0] && !strnicmp(pDest1, IMAP4_SHARED_NAME, 8)) { // 共通フォルダの指定
              q = strpbrk(pDest1,"/\\");
#ifdef UPDATE_20110303 // UID値が0x7FFFFFFF(2147483647)を超えるとマイナス値で表示されてしまう不具合。
		      sprintf(mDest, "%s%s\\%010lu-%s",  pContext->mSharedRoot, q, nUID, &pContext->FindData.cFileName[UIDLEN+RECENT]); // フラグも踏襲
#else
		      sprintf(mDest, "%s%s\\%010ld-%s",  pContext->mSharedRoot, q, nUID, &pContext->FindData.cFileName[UIDLEN+RECENT]); // フラグも踏襲
#endif
		      bShared = TRUE;
			} else {
#ifdef UPDATE_20110303 // UID値が0x7FFFFFFF(2147483647)を超えるとマイナス値で表示されてしまう不具合。
		      sprintf(mDest, "%s%s\\%010lu-%s", pContext->mRootDir, pDest1, nUID, &pContext->FindData.cFileName[UIDLEN+RECENT]); // フラグも踏襲
#else
		      sprintf(mDest, "%s%s\\%010ld-%s", pContext->mRootDir, pDest1, nUID, &pContext->FindData.cFileName[UIDLEN+RECENT]); // フラグも踏襲
#endif
			}
#ifdef UPDATE_20090130 // レシピエントファイル(RCP)があれば一緒にコピーさせる
            if (!bIgnoreRCP) // TRUE: RCPファイル無視, FALSE:RCPファイル無視しない
			{
              strcpy(mDestRCP, mDest);
              if (pRcp = strrchr(mDestRCP, '.'))
			  {
			    *pRcp = '\x0';
			    strcat(mDestRCP, ".RCP");
			  } else {
			    mDestRCP[0] = '\x0';
			  }
			}
#endif
		    if (bDebug) printf("CopyFile(%s -> %s)\n", mSrc, mDest);
			if (CopyFile(mSrc, mDest, TRUE)) { // ファイルコピー
#ifdef UPDATE_20090130 // レシピエントファイル(RCP)があれば一緒にコピーさせる
              if (!bIgnoreRCP) // TRUE: RCPファイル無視, FALSE:RCPファイル無視しない
			  {
			    if (mSrcRCP[0] && mDestRCP)
				{
                  CopyFile(mSrcRCP, mDestRCP, TRUE);
				}
			  }
#endif
			  /////////////////////////////////////////////
			  // 成功なら
			  // コピーファイルが実体化されるまでウェイト
              while(!(fp = fopen(mDest, "rt"))) { 
#ifdef UPDATE_20110302B // ログイン時のファイル移行でループする可能性を除去する対策。
		        if (CopyFile(mSrc, mDest, TRUE) == 0) // リトライ
				{
			      break;
				}
#endif
                if (bServiceTerminating)
  	              break;
	            _sleep(WAIT_TIME);
			  }
			  if (fp)
			  {
			    fclose(fp);
			  }
			  /////////////////////////////////////////////
	          nUID++; //UID値カウントアップ
	          if (nUID == 0)
		        nUID = 1;
              if (!strncmp(pDest1, mInboxAlias, strlen(mInboxAlias))) {
			    sprintf(mDest, "%sINBOX%s", pContext->mRootDir, (pDest2 ? pDest2 : ""));
			  } else if (pContext->mSharedRoot[0] && !strnicmp(pDest1, IMAP4_SHARED_NAME, 8)) { // 共通フォルダの指定
                q = strpbrk(pDest1, "/\\");
		        sprintf(mDest, "%s%s", pContext->mSharedRoot, ++q);
		        bShared = TRUE;
			  } else {
#ifdef UPDATE_20050531  // 命令にバッファフローの脆弱性
	            strcpy(mDest, pContext->mRootDir);
	            strncat(mDest, pDest1, MAX_PATH-strlen(pContext->mRootDir)-1);
	            mDest[MAX_PATH-1] = '\x0';
#else
		        sprintf(mDest, "%s%s", pContext->mRootDir, pDest1);
#endif
			  }
              PutFolderUID(mDest, nUID);
#ifdef UPDATE_20110404 // 選択フォルダ単位で排他する対策
              UnLockMailSelectDirectory(mDest);
#endif
			} else {
              bSts = FALSE;
			}
		  } else {
            bSts = FALSE;
		  }
		}
#ifdef UPDATE_20190510 // NTFS以外のファイルシステムからのファイル一覧取得可能にする対策（一覧ソート）を追加
        if (bOtherFS) // TRUE:対応する FALSE:対応しない（旧仕様）
  	    {
		  no++;
		  if (no < nSearch) {
		    FindNextFileSort(&pContext->FindData, pfd+no);
	      }
	    }
    } while(bOtherFS ? (no < nSearch) : FindNextFile(hSearch, &pContext->FindData) );
    if (bOtherFS)
      FindCloseSort(pfd);
    else
  	  FindClose(hSearch);
#else
    } while (FindNextFile(hSearch, &pContext->FindData));
	FindClose(hSearch);
#endif
#ifdef UPDATE_20110404 // 選択フォルダ単位で排他する対策
    UnLockMailSelectDirectory(mDest);
#endif
	//////////////////////////////////////////
	return bSts;
}