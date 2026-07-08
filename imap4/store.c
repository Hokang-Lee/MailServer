////////////////////////////////////////////////////////////
// STORE.C Copyright K.kawakami
////////////////////////////////////////////////////////////
#include "imap4.h"
/*
FLAGS <flag list> メッセージのフラグを引数で置き換える。フラグの新しい値がそれらのフラグの FETCH が行われたかように返される。 
FLAGS.SILENT <flag list> FLAGS と等価であるが、新しい値を返さない。 
+FLAGS <flag list> 引数をメッセージのフラグに追加する。フラグの新しい値がそれらのフラグの FETCH が行われたかのように返される。 
+FLAGS.SILENT <flag list> +FLAGS と等価であるが、新しい値を返さない。 
-FLAGS <flag list> 引数をメッセージのフラグから取り除く。フラグの新しい値がそれらのフラグの FETCH が行われたかのように返される。 
-FLAGS.SILENT <flag list> -FLAGS と等価であるが、新しい値を返さない。 
*/

extern BOOL bServiceTerminating;
extern BOOL  bDebug;
#ifdef UPDATE_20090130 // レシピエントファイル(RCP)があれば一緒にコピーさせる
extern BOOL  bIgnoreRCP; // TRUE: RCPファイル無視, FALSE:RCPファイル無視しない
#endif
#ifdef UPDATE_20190510 // NTFS以外のファイルシステムからのファイル一覧取得可能にする対策（一覧ソート）を追加
extern BOOL   bOtherFS; // TRUE:対応する FALSE:対応しない（旧仕様）
#endif

#ifdef ACCEPT_LOG_LEVEL3
void LEVEL_3_ACCEPTLOG(PCLIENT_CONTEXT lpClientContext, char *mess);
#endif

DWORD MSGIDDecipher(PCLIENT_CONTEXT lpClientContext, char *pRequest, char **pDec);
void StoreMSGByCondition(PCLIENT_CONTEXT lpClientContext, char *pNo, char *pFlags);
#ifdef UPDATE_20110404 // 選択フォルダ単位で排他する対策
HANDLE LockMailSelectDirectory(PCHAR  pszPath);
BOOL UnLockMailSelectDirectory(PCHAR  pszPath);
#endif
#ifdef UPDATE_20141220 // 操作フォルダをアクセス中のセッションに変化をリアルタイムに同報するための対策
extern BOOL  bBroadcastSession; // 同一フォルダの操作情報を接続中のセッションに同報するオプション TRUE:有効 FALSE:無効
void PutBroadcastSession(PCLIENT_CONTEXT lpClientContext);
#endif
#ifdef UPDATE_20151225 // 試作：UID値によるファイル範囲指定のチューニング
void GetQueryFileArea(CHAR *pMailDir, CHAR *pBaseFolder, BOOL bUID, BOOL bFromTo, DWORD nFrom, DWORD nTo);
#endif

BOOL STOREDispatch(PCLIENT_CONTEXT lpClientContext) {
  char    *pNo, *pFlags;
  DWORD   nMESDec = 0;
  char    *p, *q, *pMESDec[MSGID_MAX_ATTRIBUTE];
  PImap4Context pContext = &lpClientContext->Context;

//printf("STORE Command(1) = [%s]\n", pContext->pCmd);
//printf("STORE Command(2) = [%s]\n", pContext->pToken);
  if (pContext->State == Imap4SelectFolder) {  // フォルダ選択済状態
	for (nMESDec = 0; nMESDec < MSGID_MAX_ATTRIBUTE; nMESDec++) // 構造初期化
	  pMESDec[nMESDec] = NULL;
	pNo    = pContext->pToken;
	pFlags = strstr(pContext->pToken, " ");
	if (pFlags) {
	  *pFlags = '\x0';
	  pFlags++;
	  if (*pFlags == '(') {
	    pFlags++;
	    strtok(pFlags, ")");
	  }
	}
#ifdef UPDATE_20040428
	q = pNo;
	do {
	  if ((p = strstr(q, ","))) {
	    *p = '\x0';
		p++;
		if (p) {
		  StoreMSGByCondition(lpClientContext, q, pFlags);
		  q = p;
		}
	  } else {
	    StoreMSGByCondition(lpClientContext, q, pFlags);
	  }
	} while(p);
#else
	MSGIDDecipher(lpClientContext, pNo, pMESDec);
	nMESDec = 0;
    while(pMESDec[nMESDec]) {
      StoreMSGByCondition(lpClientContext, pMESDec[nMESDec], pFlags);
	  nMESDec++;
	  if (nMESDec >= MSGID_MAX_ATTRIBUTE) // 制限領域オーバー
		break;
	} 
#endif
    sprintf(pContext->mMessages, "%s %s %s completed\r\n", pContext->pSquence, IMAP4_GOOD_RESPONSE, pContext->pCmd );
  } else { // 状態が違う
    sprintf(pContext->mMessages, "%s %s Command unrecognized: %s\r\n", pContext->pSquence, IMAP4_RESULT_RESPONSE, pContext->pCmd );
  }
//printf("STORE Command(9)\n");
  return TRUE;
}

DWORD MSGIDDecipher(PCLIENT_CONTEXT lpClientContext, char *pRequest, char **pDec) {
  PImap4Context    pContext = &lpClientContext->Context;
  CHAR             *p;
  DWORD            nDec = 0, nTotal = 0;

	pDec[nDec] = pRequest;
	do {
	  p = strstr(pDec[nDec], ",");
	  if (p) {
		nTotal++;
	    *p = '\x0';
		p++;
		if (p)
		  pDec[++nDec] = p;
	  }
	} while(p && nTotal < MSGID_MAX_ATTRIBUTE);

	if (p) {
	  nTotal++; 
	}

	return nTotal;
}

void StoreMSGByCondition(PCLIENT_CONTEXT lpClientContext, char *pNo, char *pFlags) {
#ifdef UPDATE_20190510 // NTFS以外のファイルシステムからのファイル一覧取得可能にする対策（一覧ソート）を追加
  DWORD            nSearch, no;
  WIN32_FIND_DATA  *pfd;
#endif
  HANDLE           hSearch;
  PImap4Context    pContext = &lpClientContext->Context;
  CHAR             MailDir[MAX_PATH];
  CHAR             mFn[MAX_PATH], mSrc[MAX_PATH], mDest[MAX_PATH];
  CHAR             mUID[32];
  CHAR             *p, *p2, mFlags[7] = {"??????"};
  DWORD            nNo = 0, nMSG = 0, nUID, nFrom = 0, nTo = 0;
  BOOL             bSilent = FALSE, bSet = FALSE, bOn = FALSE;
  BOOL   	       bFromTo = FALSE;
  BOOL             bRecent = FALSE, bFlagged = FALSE, bAnswered = FALSE;
  BOOL             bDeleted = FALSE, bDraft = FALSE, bSeen = FALSE;
#ifdef UPDATE_20130625 // $Forwarded フラグを利用可能にした
  BOOL             bForwarded = FALSE;
#endif
#ifdef UPDATE_20090130 // レシピエントファイル(RCP)があれば一緒にコピーさせる
  CHAR             *pRcp, mRCP[MAX_PATH], mRCPDest[MAX_PATH];
#endif
#ifdef ACCEPT_LOG_LEVEL3
  char mLog[512];
#endif
#ifdef UPDATE_20101224A // フラグを変更したファイルがファイル一覧取得時に反映されないようにする対策
  FILE *fp;
#endif
#ifdef UPDATE_20110302A // フラグを変更対象のファイル名の一覧表を作成後にリネーム処理し、"~SG"が残されないようにする対策。
  FILE *fp1;
  CHAR mIdx[MAX_PATH];
  CHAR mList[MAX_PATH*2];
#endif
#ifdef UPDATE_20130625 // $Forwarded フラグを利用可能にした
  char *pflg;
#endif

//printf("STORE Command(3) = [%s]\n", pNo);
   if (pNo) { // MSG No 設定
     bFromTo = TRUE;
     if ((p2 = strstr(pNo, ":"))) {
       *p2 = '\x0';
	   p2++;
#ifdef UPDATE_20151218 // 範囲指定の*のとき処理の修正
	   if (*pNo == '*')
		   nFrom = (pContext->bUID ? pContext->nUid-1 : pContext->nExists);
	   else
#endif
	     nFrom = atol(pNo);
	   if (*p2 == '*')
#ifdef UPDATE_20151218 // 範囲指定の*のとき処理の修正
		 nTo = (pContext->bUID ?  pContext->nUid-1 : pContext->nExists);
#else
		 nTo = 0;
#endif
	   else
	     nTo = atol(p2);
	   strtok(pNo, " ");
	 } else {
#ifdef UPDATE_20151218 // 範囲指定の*のとき処理の修正
	   if (*pNo == '*')
	 	 nFrom = nTo = (pContext->bUID ?  pContext->nUid-1 : pContext->nExists);
	   else
#endif
	     nFrom = nTo = atol(pNo);
	 }
   }
#ifdef UPDATE_20110228A // 誤った構造のSTOREコマンドを送るとハングする
   if (!pFlags)
   {
	 p = NULL;
   } else 
#endif
   {
#ifdef UPDATE_20110124 // STORE N FLAGSでフラグがリセットされる不具合
     if (*pFlags != '-') // フラグセット
 	   bSet = TRUE;
#else
     if (*pFlags == '+') // フラグセット
 	   bSet = TRUE;
#endif
     p = strstr(pFlags, ".");
     if (p) {
	   if (strnicmp(p, ".SILENT", 7) == 0) // 無応答
	     bSilent = TRUE;
	 }
     p = strstr(pFlags, "(");
   } 
//printf("STORE Command(4) p = %x\n", p);
#ifdef UPDATE_20110125 // 誤った構造のSTOREコマンドを送るとハングする
   if (p)
   {
#endif
#ifdef UPDATE_20130625 // $Forwarded フラグを利用可能にした
   while ((p = strpbrk(p, "\\$"))) 
#else
   while ((p = strstr(p, "\\"))) 
#endif
   {
     if (strnicmp(p, "\\Recent", 7) == 0) {
       bRecent = TRUE;
       mFlags[RECENT-1] = (bSet ? '1' : '0');
	 } else if (strnicmp(p, "\\Answered", 9) == 0) {
       bAnswered= TRUE;
       mFlags[ANSWERED-1] = (bSet ? '1' : '0');
#ifdef UPDATE_20130625 // $Forwarded フラグを利用可能にした
	 } else if (strnicmp(p, "$Forwarded", 10) == 0) {
       bForwarded= TRUE;
       mFlags[ANSWERED-1] = (bSet ? '2' : '0');
#endif
	 } else if (strnicmp(p, "\\Flagged", 8) == 0) {
       bFlagged = TRUE;
       mFlags[FLAGGED-1] = (bSet ? '1' : '0');
	 } else if (strnicmp(p, "\\Deleted", 8) == 0) {
       bDeleted = TRUE;
       mFlags[DELETED-1] = (bSet ? '1' : '0');
	 } else if (strnicmp(p, "\\Seen", 5) == 0) {
       bSeen = TRUE;
       mFlags[SEEN-1] = (bSet ? '1' : '0');
	 } else if (strnicmp(p, "\\Draft", 6) == 0) {
       bDraft = TRUE;
       mFlags[DRAFT-1] = (bSet ? '1' : '0');
	 }
	 p++;
   }
#ifdef UPDATE_20110125 // 誤った構造のSTOREコマンドを送るとハングする
   } else {
	 return;
   }
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
//printf("STORE Command(5)\n");
#ifdef UPDATE_20110302A // フラグを変更対象のファイル名の一覧表を作成後にリネーム処理し、"~SG"が残されないようにする対策。
    sprintf(mIdx, "%s\\store", pContext->mSelectDir);  // STROE一覧
	if ((fp1 = fopen(mIdx, "wt")))
	{
#ifdef UPDATE_20151225 // 試作：UID値によるファイル範囲指定のチューニング
      GetQueryFileArea(MailDir, pContext->mSelectDir, pContext->bUID, bFromTo, nFrom, nTo);
#else
      sprintf(MailDir, "%s\\*-??????.MSG", pContext->mSelectDir);  // メッセージフォルダ取出し
#endif
#ifdef UPDATE_20190510 // NTFS以外のファイルシステムからのファイル一覧取得可能にする対策（一覧ソート）を追加
     if (bOtherFS) // TRUE:対応する FALSE:対応しない（旧仕様）
	 {
       pfd = FindFirstFileSort((LPCTSTR)MailDir, &pContext->FindData, &nSearch);
       if (!pfd){
#ifdef UPDATE_20110404 // 選択フォルダ単位で排他する対策
          UnLockMailSelectDirectory(pContext->mSelectDir);
#endif
#ifdef UPDATE_20141118 // カラのフォルダに対しSTORE命令実行すると一部のメモリ開放が抜ける不具合。
	      fclose(fp1);
#endif
          return;
	   }
       no = 0;
	 } else {
       hSearch = FindFirstFile((LPCTSTR)MailDir, &pContext->FindData);
       if (hSearch == INVALID_HANDLE_VALUE){
#ifdef UPDATE_20110404 // 選択フォルダ単位で排他する対策
         UnLockMailSelectDirectory(pContext->mSelectDir);
#endif
#ifdef UPDATE_20141118 // カラのフォルダに対しSTORE命令実行すると一部のメモリ開放が抜ける不具合。
	     fclose(fp1);
#endif
         return;
	   }
	 }
#else
      hSearch = FindFirstFile((LPCTSTR)MailDir, &pContext->FindData);
      if (hSearch == INVALID_HANDLE_VALUE){
#ifdef UPDATE_20110404 // 選択フォルダ単位で排他する対策
        UnLockMailSelectDirectory(pContext->mSelectDir);
#endif
#ifdef UPDATE_20141118 // カラのフォルダに対しSTORE命令実行すると一部のメモリ開放が抜ける不具合。
	    fclose(fp1);
#endif
        return;
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
 	    strcpy(mUID, pContext->FindData.cFileName);
        strtok(mUID, "-");
	    nUID = (DWORD) atol(mUID);
        nMSG = (pContext->bUID ? nUID : ++nMSG);
#ifdef UPDATE_20151217 // 範囲指定に0を指定するとワイルドカード処理されてしまう不具合
		if (bFromTo && nFrom <= nMSG && nTo >= nMSG) 
#else
		if ( (!bFromTo || (bFromTo && nFrom <= nMSG && (nTo == 0 || nTo >= nMSG) )) ) 
#endif
		{
		  //strcpy(mFn, pContext->FindData.cFileName);
		  //sprintf(mSrc, "%s\\%s", pContext->mSelectDir, mFn);
          if (!pContext->bShared || pContext->bShared && pContext->bSharedRW) {
			fprintf(fp1, "%s\t%lu\n", pContext->FindData.cFileName, nNo);
#ifdef UPDATE_20110405B // インデックス用ファイルに一覧書込みが完全に完了させる対策。
	        fflush(fp1);
#endif
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
#ifdef UPDATE_20110405B // インデックス用ファイルに一覧書込みが完全に完了させる対策。
	  fflush(fp1);
#endif
	  fclose(fp1);
#ifdef UPDATE_20110405B // インデックス用ファイルに一覧書込みが完全に完了させる対策。
	  while(!(fp1 = fopen(mIdx, "rt")))
	  {
        if (bServiceTerminating)
  	      break;
	    _sleep(WAIT_TIME);
	  }
#ifdef UPDATE_20180827 //ファイルオープン中でサービス停止時が行われたときにハングしない対策
	  if (fp1)
#endif
	  {
	    fclose(fp1);
	  }
      //_sleep(1000);
#endif
	}
#endif
    //if (nFrom == nTo && pContext->bUID)  // １件だけ
    //  sprintf(MailDir, "%s\\%010u-??????.MSG", pContext->mSelectDir, nFrom);  // メッセージフォルダ取出し
	//else               // 複数件
	// 20110203Aの変更してはいけない
#ifdef UPDATE_20110302A // フラグを変更対象のファイル名の一覧表を作成後にリネーム処理し、"~SG"が残されないようにする対策。
  sprintf(mIdx, "%s\\store", pContext->mSelectDir);  // STROE一覧
  if ((fp1 = fopen(mIdx, "rt")))
  {
#ifdef UPDATE_20110405B // インデックス用ファイルに一覧書込みが完全に完了させる対策。
	memset(mList, 0, sizeof(mList));
#endif
	while (fgets(mList, sizeof(mList), fp1))
	{
#ifdef UPDATE_20110405B // インデックス用ファイルに一覧書込みが完全に完了させる対策。
	  if (!strchr(mList, '\n'))
	  {
#ifdef ACCEPT_LOG_LEVEL3
		sprintf(mLog, "StoreMSGByCondition() index error / mList[%s]\n", mList);
        LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
#endif
//exit(0);
		break;
	  } else 
#endif
	  {
	    //strtok(pContext->FindData.cFileName, "\r\n");
	  nNo = atol(&mList[22]);
	  strtok(mList, "\t");
	  strcpy(pContext->FindData.cFileName, mList);
#else
    sprintf(MailDir, "%s\\*-??????.MSG", pContext->mSelectDir);  // メッセージフォルダ取出し
#ifdef UPDATE_20190510 // NTFS以外のファイルシステムからのファイル一覧取得可能にする対策（一覧ソート）を追加
    pfd = FindFirstFileSort((LPCTSTR)MailDir, &pContext->FindData, &nSearch);
    if (!pfd){
#ifdef UPDATE_20110404 // 選択フォルダ単位で排他する対策
      UnLockMailSelectDirectory(pContext->mSelectDir);
#endif
      return;
	}
    no = 0;
#else
    hSearch = FindFirstFile((LPCTSTR)MailDir, &pContext->FindData);
    if (hSearch == INVALID_HANDLE_VALUE){
#ifdef UPDATE_20110404 // 選択フォルダ単位で排他する対策
      UnLockMailSelectDirectory(pContext->mSelectDir);
#endif
      return;
    }
#endif
//printf("STORE Command(5.1)\n");
    do 
	{
        if (pContext->FindData.dwFileAttributes & (FILE_ATTRIBUTE_DIRECTORY |
                                         FILE_ATTRIBUTE_SYSTEM |
                                         FILE_ATTRIBUTE_HIDDEN) ) {
            continue;   // ファイル以外はSkip non-normal files
        }
		if (strlen(pContext->FindData.cFileName) != 21) {
		   continue;    // ファイル名の長さが違うものは無視
		}
		nNo++;
#endif
		bOn = FALSE;
 	    strcpy(mUID, pContext->FindData.cFileName);
        strtok(mUID, "-");
	    nUID = (DWORD) atol(mUID);
//printf("STORE Command(5.2) mUID = [%s]\n", mUID);
		/*
#ifdef ACCEPT_LOG_LEVEL3
		sprintf(mLog, "%s / %s / nUID=%lu", pContext->FindData.cFileName, mUID, nUID);
        LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
#endif
		*/
#ifndef UPDATE_20110302A // フラグを変更対象のファイル名の一覧表を作成後にリネーム処理し、"~SG"が残されないようにする対策。
        nMSG = (pContext->bUID ? nUID : ++nMSG);
		if ( (!bFromTo || (bFromTo && nFrom <= nMSG && (nTo == 0 || nTo >= nMSG) )) ) 
#endif
		{
		  strcpy(mFn, pContext->FindData.cFileName);
		  sprintf(mSrc, "%s\\%s", pContext->mSelectDir, mFn);
#ifdef UPDATE_20090130 // レシピエントファイル(RCP)があれば一緒にコピーさせる
          if (!bIgnoreRCP) // TRUE: RCPファイル無視, FALSE:RCPファイル無視しない
		  {
		    strcpy(mRCP, mSrc);
            if (pRcp = strrchr(mRCP, '.'))
			{
			  *pRcp = '\x0';
			  strcat(mRCP, ".RCP");
			}
		  }
#endif
          if (!pContext->bShared || pContext->bShared && pContext->bSharedRW) {
		    if (bRecent)
			  mFn[UIDLEN+RECENT] = mFlags[RECENT-1];
#ifdef UPDATE_20130625 // $Forwarded フラグを利用可能にした
		    if (bAnswered || bForwarded)
			{
			  if ((pflg = strrchr(pContext->FindData.cFileName, '-')))
			  {
				 pflg+=ANSWERED;
				 if (*pflg == '0')
				 {
                   mFn[UIDLEN+ANSWERED] = mFlags[ANSWERED-1];
				 } else if (*pflg!=mFlags[ANSWERED-1]) {
                   mFn[UIDLEN+ANSWERED] = '3';
				 }
			  }
			}
#else
		    if (bAnswered)
			  mFn[UIDLEN+ANSWERED] = mFlags[ANSWERED-1];
#endif
		    if (bFlagged)
			  mFn[UIDLEN+FLAGGED] = mFlags[FLAGGED-1];
	        if (bDeleted)
			  mFn[UIDLEN+DELETED] = mFlags[DELETED-1];
		    if (bSeen)
			  mFn[UIDLEN+SEEN] = mFlags[SEEN-1];
		    if (bDraft)
			  mFn[UIDLEN+DRAFT] = mFlags[DRAFT-1];
//printf("STORE Command(5.3)\n");
#ifdef UPDATE_20101224A // フラグを変更したファイルがファイル一覧取得時に反映されないようにする対策
#ifdef UPDATE_20110203A //フラグ変更中に新着の検出が発生したとき最後のUIDを正しく取得できないタイミングがある不具合
		    sprintf(mDest, "%s\\%s", pContext->mSelectDir, mFn);
#ifndef UPDATE_20110302A // フラグを変更対象のファイル名の一覧表を作成後にリネーム処理し、"~SG"が残されないようにする対策。
		    mDest[strlen(mDest)-3] = '~';
#endif
#ifdef UPDATE_20110301A // FETCHコマンドでフラグ変更時に拡張子"~SG"のファイルが残ってしまうタイミングが無いようにする対策
		    //DeleteFile(mDest); // フラグ変更
//printf("STORE Command(5.3.1) MoveFileEx(mSrc, mDest)\n", mSrc, mDest);
		    rename(mSrc, mDest); // フラグ変更
#else
		    MoveFileEx(mSrc, mDest, MOVEFILE_WRITE_THROUGH); // フラグ変更
#endif
#else
		    sprintf(mDest, "%s\\~%s", pContext->mSelectDir, mFn);
		    MoveFile(mSrc, mDest); // フラグ変更
#endif
//printf("STORE Command(5.4)\n");
#ifdef UPDATE_20110203A //フラグ変更中に新着の検出が発生したとき最後のUIDを正しく取得できないタイミングがある不具合
#ifdef ACCEPT_LOG_LEVEL3
		   sprintf(mLog, "MoveFile(%s -> %s)\n", mSrc, mDest);
           LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
#endif
#else
		   if (bDebug) printf("MoveFile(%s -> %s)\n", mSrc, mDest);
#endif
//printf("STORE Command(5.4.1) mSrc = [%s], mDest = [%s]\n", mSrc, mDest);
			while(!(fp = fopen(mDest, "rt")))
			{
#ifdef UPDATE_20110301 // STROEコマンドでフラグ変更時に拡張子"~SG"のファイルが残ってしまうタイミングが無いようにする対策
		      if (rename(mSrc, mDest) != 0) // フラグ変更
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
#else
		    sprintf(mDest, "%s\\%s", pContext->mSelectDir, mFn);
		    if (bDebug) printf("MoveFile(%s -> %s)\n", mSrc, mDest);
		    MoveFile(mSrc, mDest); // フラグ変更
#endif
//printf("STORE Command(5.4.2)\n");
#ifdef UPDATE_20090130 // レシピエントファイル(RCP)があれば一緒にコピーさせる
            if (!bIgnoreRCP) // TRUE: RCPファイル無視, FALSE:RCPファイル無視しない
			{
		      strcpy(mRCPDest, mDest);
              if (pRcp = strrchr(mRCPDest, '.'))
			  {
			    *pRcp = '\x0';
			    strcat(mRCPDest, ".RCP");
			  }
			  MoveFile(mRCP, mRCPDest);
			}
#endif
		  }
//printf("STORE Command(5.5)\n");
		  if (!bSilent) { // 応答する場合
			sprintf(pContext->mMessages, "* %d FETCH (FLAGS (", nNo);
			////////////////////////////////////////////
			if (mFn[UIDLEN+RECENT] == '1') {
			  strcat(pContext->mMessages, "\\Recent");
			  bOn = TRUE;
			}
#ifdef UPDATE_20130625 // $Forwarded フラグを利用可能にした
			if (mFn[UIDLEN+ANSWERED] == '1' || mFn[UIDLEN+ANSWERED] == '3') 
#else
			if (mFn[UIDLEN+ANSWERED] == '1') 
#endif
			{
			  if (bOn)
				strcat(pContext->mMessages, " ");
			  strcat(pContext->mMessages, "\\Answered");
			  bOn = TRUE;
			}
			if (mFn[UIDLEN+FLAGGED] == '1') {
			  if (bOn)
				strcat(pContext->mMessages, " ");
			  strcat(pContext->mMessages, "\\Flaged");
			  bOn = TRUE;
			}
			if (mFn[UIDLEN+DELETED] == '1') {
			  if (bOn)
				strcat(pContext->mMessages, " ");
			  strcat(pContext->mMessages, "\\Deleted");
			  bOn = TRUE;
			}
			if (mFn[UIDLEN+SEEN] == '1') {
			  if (bOn)
				strcat(pContext->mMessages, " ");
			  strcat(pContext->mMessages, "\\Seen");
			  bOn = TRUE;
			}
			if (mFn[UIDLEN+DRAFT] == '1') {
			  if (bOn)
				strcat(pContext->mMessages, " ");
			  strcat(pContext->mMessages, "\\Draft");
			  bOn = TRUE;
			}
#ifdef UPDATE_20130625 // $Forwarded フラグを利用可能にした
			if (mFn[UIDLEN+ANSWERED] == '2' || mFn[UIDLEN+ANSWERED] == '3') 
			{
			  if (bOn)
				strcat(pContext->mMessages, " ");
			  strcat(pContext->mMessages, "$Forwarded");
			  bOn = TRUE;
			}

#endif
            ////////////////////////////////////////////
			sprintf(mUID, "%d", nUID);
		    strcat(pContext->mMessages, ") UID ");
		    strcat(pContext->mMessages, mUID);
		    strcat(pContext->mMessages, ")\r\n");
            //put_reply(lpClientContext, TRUE, TRUE);
#ifdef UPDATE_20141220 // 操作フォルダをアクセス中のセッションに変化をリアルタイムに同報するための対策
            put_reply(lpClientContext, TRUE, TRUE);
		    if (bBroadcastSession) // 同一フォルダの操作情報を接続中のセッションに同報するオプション TRUE:有効 FALSE:無効
			{
              PutBroadcastSession(lpClientContext); //pContext->mSelectDir, pContext->mMessages);
			}
#else
            put_reply(lpClientContext, TRUE, TRUE);
#endif
		  }
		}
#ifdef UPDATE_20110405B // インデックス用ファイルに一覧書込みが完全に完了させる対策。
	    memset(mList, 0, sizeof(mList));
#endif
	  }
#ifdef UPDATE_20110302A // フラグを変更対象のファイル名の一覧表を作成後にリネーム処理し、"~SG"が残されないようにする対策。
	}
	fclose(fp1);
  }
#else
    } while (FindNextFile(hSearch, &pContext->FindData));
	FindClose(hSearch);
#endif
/////////////////////
#ifdef UPDATE_20110302A // フラグを変更対象のファイル名の一覧表を作成後にリネーム処理し、"~SG"が残されないようにする対策。
	unlink(mIdx);
#else
//printf("STORE Command(6)\n");
#ifdef UPDATE_20101224A // フラグを変更したファイルがファイル一覧取得時に反映されないようにする対策
#ifdef UPDATE_20110203A //フラグ変更中に新着の検出が発生したとき最後のUIDを正しく取得できないタイミングがある不具合
    sprintf(MailDir, "%s\\*-??????.~SG", pContext->mSelectDir);  // メッセージフォルダ取出し
#else
    sprintf(MailDir, "%s\\~*-??????.MSG", pContext->mSelectDir);  // メッセージフォルダ取出し
#endif
    hSearch = FindFirstFile((LPCTSTR)MailDir, &pContext->FindData);
    if (hSearch == INVALID_HANDLE_VALUE){
#ifdef UPDATE_20110404 // 選択フォルダ単位で排他する対策
      UnLockMailSelectDirectory(pContext->mSelectDir);
#endif
      return;
    }
//printf("STORE Command(6.1)\n");
    do {
        if (pContext->FindData.dwFileAttributes & (FILE_ATTRIBUTE_DIRECTORY |
                                         FILE_ATTRIBUTE_SYSTEM |
                                         FILE_ATTRIBUTE_HIDDEN) ) {
            continue;   // ファイル以外はSkip non-normal files
        }
#ifdef UPDATE_20110203A //フラグ変更中に新着の検出が発生したとき最後のUIDを正しく取得できないタイミングがある不具合
		if (strlen(pContext->FindData.cFileName) != 21) 
#else
		if (strlen(pContext->FindData.cFileName) != 22)
#endif
		{
		   continue;    // ファイル名の長さが違うものは無視
		}
//printf("STORE Command(6.2)\n");
 	    sprintf(mSrc, "%s\\%s", pContext->mSelectDir, pContext->FindData.cFileName);
#ifdef UPDATE_20110203A //フラグ変更中に新着の検出が発生したとき最後のUIDを正しく取得できないタイミングがある不具合
	    sprintf(mDest, "%s\\%s", pContext->mSelectDir, pContext->FindData.cFileName);
	    mDest[strlen(mDest)-3] = 'M';
#ifdef UPDATE_20110301A // FETCHコマンドでフラグ変更時に拡張子"~SG"のファイルが残ってしまうタイミングが無いようにする対策
		//DeleteFile(mDest); // フラグ変更
//printf("STORE Command(6.3) rename(%s, %s)\n", mSrc, mDest);
		rename(mSrc, mDest); // フラグ変更
#else
		MoveFileEx(mSrc, mDest, MOVEFILE_WRITE_THROUGH); // フラグ変更
#endif
#else
	    sprintf(mDest, "%s\\%s", pContext->mSelectDir, &pContext->FindData.cFileName[1]);
	    MoveFile(mSrc, mDest); // フラグ変更
#endif
#ifdef ACCEPT_LOG_LEVEL3
		sprintf(mLog, "MoveFile(%s -> %s)\n", mSrc, mDest);
        LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
#endif
//printf("STORE Command(6.4)\n");
	    while(!(fp = fopen(mDest, "rt")))
		{
#ifdef UPDATE_20110301 // STROEコマンドでフラグ変更時に拡張子"~SG"のファイルが残ってしまうタイミングが無いようにする対策
		  if (rename(mSrc, mDest) != 0) // フラグ変更
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
#ifdef UPDATE_20190510 // NTFS以外のファイルシステムからのファイル一覧取得可能にする対策（一覧ソート）を追加
		no++;
		if (no < nSearch) {
		  FindNextFileSort(&pContext->FindData, pfd+no);
		}
	} while(no < nSearch);
	FindCloseSort(pfd);
#else
    } while (FindNextFile(hSearch, &pContext->FindData));
	FindClose(hSearch);
#endif
#endif
#endif //20110302A
//printf("STORE Command(7)\n");
	//////////////////////////////////////////
#ifdef UPDATE_20110404 // 選択フォルダ単位で排他する対策
    UnLockMailSelectDirectory(pContext->mSelectDir);
#endif
}