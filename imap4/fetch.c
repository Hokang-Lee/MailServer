////////////////////////////////////////////////////////////
// FETCH.C Copyright K.kawakami
////////////////////////////////////////////////////////////
#include "imap4.h"

extern BOOL bServiceTerminating;
extern BOOL  bDebug;
#ifdef UPDATE_20090130 // レシピエントファイル(RCP)があれば一緒にコピーさせる
extern BOOL    bIgnoreRCP; // TRUE: RCPファイル無視, FALSE:RCPファイル無視しない
#endif
#ifdef UPDATE_20151115 // SELECT時にメール一覧をキャッシュするためのポインタ
extern BOOL   bSELECT;   // SELECTによるファイル一覧キャッシュ化
#endif
#ifdef UPDATE_20190510 // NTFS以外のファイルシステムからのファイル一覧取得可能にする対策（一覧ソート）を追加
extern BOOL   bOtherFS; // TRUE:対応する FALSE:対応しない（旧仕様）
#endif
#ifdef UPDATE_20230706 // FETCH時のメールデータを事前に一括して読込み検索するフラグを追加
extern BOOL  bBulkFetch;
#endif
#ifdef UPDATE_20231003 // データ受信時のCPU負荷を下げる対策
extern DWORD nPeekCoolTime;   //0:無効 デフォルト 50 100 500 1000 5000 10000
#endif
#ifdef UPDATE_20230714E // Content-Disposition:ヘッダの"filename="の項目のみ出力するオプションを追加　デフォルト 0:filenameのみ 1:他含む
extern BOOL  bDisposition; // IDLE時の参加セッションへ同報するか否かの選択 デフォルト 0:同報しない 1:同報する
#endif
#ifdef UPDATE_20231209 // FETCH命令中に通信切断された場合に大きなサイズのデータ送信の中断を行えるようにした。
extern BOOL  bSendErrEscape; //送信エラー時中断 1:する(デフォルト) 0:しない
#endif

#ifdef ACCEPT_LOG_LEVEL3
void LEVEL_3_ACCEPTLOG(PCLIENT_CONTEXT lpClientContext, char *mess);
#endif
#ifdef UPDATE_20110404 // 選択フォルダ単位で排他する対策
HANDLE LockMailSelectDirectory(PCHAR  pszPath);
BOOL UnLockMailSelectDirectory(PCHAR  pszPath);
#endif

void maketime(SYSTEMTIME *ltime, char *mt);
void gettime(SYSTEMTIME *ltime, char *mt);
BOOL SPA_CopyFileDecode(char *pSrc, char *pDest); // 暗号化ファイル用

DWORD MSGIDDecipher(PCLIENT_CONTEXT lpClientContext, char *pRequest, char **pDec);
DWORD FLAGSDecipher(BOOL bUID, char *pRequest, char **pDec, BOOL bOK);
void FetchMSGByData(PCLIENT_CONTEXT lpClientContext, char *pNo, char **pDec);
//////////////////////////////////////////////////////////////////
void translateUue2Base64(char *line, int inlen, char *newline);
void X_StrCpy(char *pDest, char *pSrc, int nDest, int nSrc, BOOL bContinue);
BOOL Working_Fetch(PCLIENT_CONTEXT lpClientContext, char **pDec);
void fetch_flags(PCLIENT_CONTEXT lpClientContext);
void fetch_uid(PCLIENT_CONTEXT lpClientContext);
void fetch_internaldate(PCLIENT_CONTEXT lpClientContext);
void fetch_rfc822_size(PCLIENT_CONTEXT lpClientContext);
void make_message(PCLIENT_CONTEXT lpClientContext, char *pMess, char *pEnv);
void make_recode(char *pMess, char *pEnv);
void fetch_envelope(PCLIENT_CONTEXT lpClientContext);
#ifdef UPDATE_20231121 // HEADER.FIELDS命令の応答で範囲指定が指定できない不具合。
void fetch_rfc822_header(PCLIENT_CONTEXT lpClientContext, int nFlag, char **pHeader, char *pRange);
#else
void fetch_rfc822_header(PCLIENT_CONTEXT lpClientContext, int nFlag, char **pHeader);
#endif
void fetch_rfc822(PCLIENT_CONTEXT lpClientContext, int nType, char *pPart, char *pRange);
#ifdef UPDATE_20150630 // MESSAGE/RFC822の構造出力への対応
void fetch_bodystructure(PCLIENT_CONTEXT lpClientContext, BOOL bExtend, FILE *fp);
#else
void fetch_bodystructure(PCLIENT_CONTEXT lpClientContext, BOOL bExtend);
#endif
#ifdef UPDATE_20150710 // MESSAGE/RFC822の構造出力への対応(入れ子の解析)でTEXT以外構造が出力されなかった。
int multipart_header(PCLIENT_CONTEXT lpClientContext, int nPart0, PPartStruct PS0, PPartStruct PS, char *pLine, int nLineLen, char *pBound0, char *pBound, FILE *fp, BOOL bExtend);
#else
#ifdef UPDATE_20150708 // MIME構造で(multipart(text,multipart(html))のような構造で順番道理の結果が返せない不具合。
int multipart_header(PCLIENT_CONTEXT lpClientContext, PPartStruct PS0, PPartStruct PS, char *pLine, int nLineLen, char *pBound0, char *pBound, FILE *fp, BOOL bExtend);
#else
#ifdef UPDATE_20150620B // 多重入れ子構造がある場合に正しく抽出できない不具合
void multipart_header(PCLIENT_CONTEXT lpClientContext, PPartStruct PS, char *pLine, int nLineLen, char *pBound0, char *pBound, FILE *fp, BOOL bExtend);
#else
void multipart_header(PCLIENT_CONTEXT lpClientContext, PPartStruct PS, char *pLine, int nLineLen, char *pBound, FILE *fp, BOOL bExtend);
#endif
#endif
#endif
#ifdef UPDATE_20150712 // MIME構造の階層管理に親パート番号を設定できる項目を追加した。
void fetch_bodystructure_2(PCLIENT_CONTEXT lpClientContext, DWORD *pnPart, PartStruct *pPS, BOOL bExtend, FILE *fp);
#else
#ifdef UPDATE_20150701 // MESSAGE/RFC822の構造出力への対応(入れ子)
void fetch_bodystructure_2(PCLIENT_CONTEXT lpClientContext, PartStruct *pPS, BOOL bExtend, FILE *fp);
#endif
#endif
#ifdef UPDATE_20150712 // MIME構造の階層管理に親パート番号を設定できる項目を追加した。
int  message_bodystructure(PCLIENT_CONTEXT lpClientContext, DWORD *pnPart, PPartStruct pPS, char *pLine,  FILE *fp, char *pBound, BOOL bExtend);
#else
#ifdef UPDATE_20150711 // MESSAGE/RFC822の構造出力への対応(入れ子の解析)でTEXT以外構造が出力されなかった。
BOOL message_bodystructure(PCLIENT_CONTEXT lpClientContext,char *pLine,  FILE *fp, char *pBound, BOOL bExtend);
#else
#ifdef UPDATE_20150710 // MESSAGE/RFC822の構造出力への対応(入れ子の解析)でTEXT以外構造が出力されなかった。
void message_bodystructure(PCLIENT_CONTEXT lpClientContext,char *pLine,  FILE *fp, char *pBound, BOOL bExtend);
#else
#ifdef UPDATE_20150709 // MESSAGE/RFC822の構造出力への対応(入れ子の解析)でTEXT以外構造が出力されなかった。
void message_bodystructure(PCLIENT_CONTEXT lpClientContext,char *pLine,  FILE *fp, BOOL bExtend);
#endif
#endif
#endif
#endif
#ifdef UPDATE_20151225 // 試作：UID値によるファイル範囲指定のチューニング
void GetQueryFileArea(CHAR *pMailDir, CHAR *pBaseFolder, BOOL bUID, BOOL bFromTo, DWORD nFrom, DWORD nTo);
#endif
#ifdef UPDATE_20230706 // FETCH時のメールデータを事前に一括して読込み検索するフラグを追加
char *mgets(char *pline, unsigned long nlen, char **ptkn);
#endif

char * SKIP_SP_TAB(char *p) {
  while(*p == ' ' || *p == '\t'){ // <SPACE> or <TAB> 除去
    p++;
  }
  return p;
}

BOOL FETCHDispatch(PCLIENT_CONTEXT lpClientContext) {
  DWORD   nMESDec = 0;
  char    *pMESDec[MSGID_MAX_ATTRIBUTE];
  char    *p, *q, *pNo, *pFlags, *pEnd;
  char    *pDec[MAX_ATTRIBUTE];
  DWORD   nDec;
  PImap4Context pContext = &lpClientContext->Context;
  if (pContext->State == Imap4SelectFolder) {  // フォルダ選択済状態
	for (nMESDec = 0; nMESDec < MSGID_MAX_ATTRIBUTE; nMESDec++) // 構造初期化
	  pMESDec[nMESDec] = NULL;
	for (nDec = 0; nDec < MAX_ATTRIBUTE; nDec++) // 構造初期化
	  pDec[nDec] = NULL;
	pNo    = pContext->pToken;
	if ((pFlags = strstr(pContext->pToken, " "))) {
	  *pFlags = '\x0';
	  pFlags++;
	  if (*pFlags == '(')
	    pFlags++;
      if ((pEnd = strrchr(pFlags, ')')))
#ifdef UPDATE_20231121 // HEADER.FIELDS命令の応答で範囲指定が指定できない不具合。
		if (*(pEnd+1) == '\x0')
		  *pEnd = '\x0';
		else if ((pEnd = strrchr(pEnd+1, ')')))
		  *pEnd = '\x0';
#else
		*pEnd = '\x0';
#endif
	}
    if (pFlags) { // 実行内容の有無
      nDec = FLAGSDecipher(pContext->bUID, pFlags, pDec, TRUE);
      if (nDec >= MAX_ATTRIBUTE) { // 複雑すぎる構造
        sprintf(pContext->mMessages, "%s %s Excessively complex %s attribute list\r\n", pContext->pSquence, IMAP4_RESULT_RESPONSE, pContext->pCmd );
	  } else {
#ifdef UPDATE_20040428
	    q = pNo;
	    do {
		  if ((p = strstr(q, ","))) {
	        *p = '\x0';
		    p++;
		    if (p) {
			  FetchMSGByData(lpClientContext, q, pDec);
		      q = p;
			}
		  } else {
		    FetchMSGByData(lpClientContext, q, pDec); // 1つの場合
		  }
		} while(p);
#else
	    MSGIDDecipher(lpClientContext, pNo, pMESDec);
	    nMESDec = 0;
        while(pMESDec[nMESDec]) {
	      FetchMSGByData(lpClientContext, pMESDec[nMESDec], pDec);
	      nMESDec++;
	      if (nMESDec >= MSGID_MAX_ATTRIBUTE) // 制限領域オーバー
		    break;
		} 
#endif
        sprintf(pContext->mMessages, "%s %s %s completed\r\n", pContext->pSquence, IMAP4_GOOD_RESPONSE, pContext->pCmd );
	  }
    } else {
      sprintf(pContext->mMessages, "%s %s %s illegal argument\r\n", pContext->pSquence, IMAP4_BADTOKEN_RESPONSE, pContext->pCmd );
	}
  } else { // 状態が違う
    sprintf(pContext->mMessages, "%s %s %s invalid command\r\n", pContext->pSquence, IMAP4_RESULT_RESPONSE, pContext->pCmd );
  }
   return TRUE;
}

DWORD FLAGSDecipher(BOOL bUID, char *pRequest, char **pDec, BOOL bOK) {
  CHAR             *p, *p2, *p3;
  DWORD            nDec = 0, nTotal = 1;
  BOOL             bBrackets = FALSE, bOnUid = FALSE;
  CHAR             mStack[1024];
#ifdef UPDATE_20150702 // UID付きFETCHの応答でUID値を最後に出力していたのを最初に出力するように修正した。
  DWORD            n1, n2;      
  BOOL             bINUID = FALSE;
#endif    
#ifdef UPDATE_20151225A // FETCH命令のALL,FULL,FASTが小文字だと処理できない不具合
  CHAR *q, mRequest[1024 * BUFFER_ARREY];
#endif
    ////////////////////////////
#ifdef UPDATE_20151225A // FETCH命令のALL,FULL,FASTが小文字だと処理できない不具合
    strcpy(mRequest, pRequest);
	strupr(mRequest);
#ifdef UPDATE_20230713 // FETCH命令のHEADER.FIELDSの文字列の並びに"ALL/FULL/FAST"が含まれると、"FETCH ALL/FULL/FAST"として誤って処理してしまう不具合
	if (q = strstr(mRequest, "ALL"))
	  if (*(q+3) != ' ' && *(q+3) != '\x0')
		q= NULL;
	if (q) 
#else
	if ((q = strstr(mRequest, "ALL"))) 
#endif
#else
	if ((p = strstr(pRequest, "ALL"))) 
#endif
	{
#ifdef UPDATE_20151225A // FETCH命令のALL,FULL,FASTが小文字だと処理できない不具合
	  p = pRequest + (unsigned int)(q - mRequest);
#endif
	  strncpy(mStack, p+3, sizeof(mStack));
	  *p = '\x0';
      strcpy(pRequest,"FLAGS INTERNALDATE RFC822.SIZE ENVELOPE ");
	  strcat(pRequest, mStack);
	} else 
#ifdef UPDATE_20151225A // FETCH命令のALL,FULL,FASTが小文字だと処理できない不具合
#ifdef UPDATE_20230713 // FETCH命令のHEADER.FIELDSの文字列の並びに"ALL/FULL/FAST"が含まれると、"FETCH ALL/FULL/FAST"として誤って処理してしまう不具合
	if (q = strstr(mRequest, "FULL"))
	  if (*(q+4) != ' ' && *(q+4) != '\x0')
		q= NULL;
	if (q) 
#else
	if ((q = strstr(mRequest, "FULL"))) 
#endif
#else
	if ((p = strstr(pRequest, "FULL"))) 
#endif
	{
#ifdef UPDATE_20151225A // FETCH命令のALL,FULL,FASTが小文字だと処理できない不具合
	  p = pRequest + (unsigned int)(q - mRequest);
#endif
	  strncpy(mStack, p+4, sizeof(mStack));
	  *p = '\x0';
      strcpy(pRequest,"FLAGS INTERNALDATE RFC822.SIZE ENVELOPE BODY ");
	  strcat(pRequest, mStack);
	} else 
#ifdef UPDATE_20151225A // FETCH命令のALL,FULL,FASTが小文字だと処理できない不具合
#ifdef UPDATE_20230713 // FETCH命令のHEADER.FIELDSの文字列の並びに"ALL/FULL/FAST"が含まれると、"FETCH ALL/FULL/FAST"として誤って処理してしまう不具合
	if (q = strstr(mRequest, "FAST"))
	  if (*(q+4) != ' ' && *(q+4) != '\x0')
		q= NULL;
	if (q) 
#else
	if ((q = strstr(mRequest, "FAST"))) 
#endif
#else
	if ((p = strstr(pRequest, "FAST"))) 
#endif
	{
#ifdef UPDATE_20151225A // FETCH命令のALL,FULL,FASTが小文字だと処理できない不具合
	  p = pRequest + (unsigned int)(q - mRequest);
#endif
	  strncpy(mStack, p+4, sizeof(mStack));
	  *p = '\x0';
	  strcpy(pRequest,"FLAGS INTERNALDATE RFC822.SIZE ");
	  strcat(pRequest, mStack);
	}
    p3 = pRequest + strlen(pRequest) + 1; // 末尾
/*  //最後に実施
	if (!bOnUid && bUID && bOK) {// UIDが未設定なら命令追加
	  strcpy(p3, "UID");
	  pDec[0] = p3;
      nDec=1;
	}
*/
	pDec[nDec] = pRequest;
	do {
	  if (bBrackets)
	    p = strstr(pDec[nDec], "]");
	  else
	    p = strstr(pDec[nDec], " ");
	  if (p) {
		nTotal++;
		if (strnicmp(pDec[nDec], "not", 3) == 0) {
		  p2 = pDec[nDec] + 4;
	      p = strstr(p2, " ");
	      if (p) {
		    *p = '\x0';
		    p++;
		  }
		} else {
		  *p = '\x0';
	      if (!stricmp(pDec[nDec], "UID")) // UIDが設定済
	        bOnUid = TRUE;
		  if (strstr(pDec[nDec], "[") && !strstr(pDec[nDec], "]"))  // 鍵括弧があるなら
			bBrackets = TRUE;
		  else 
			bBrackets = FALSE;
		  p++;
		}
		if (p) {
		  pDec[++nDec] = p;
		}
	  } else {
        if (!stricmp(pDec[nDec], "UID")) // UIDが設定済
	      bOnUid = TRUE;
	  }
	} while(p && nTotal < MAX_ATTRIBUTE-1);
#ifdef UPDATE_20150702 // UID付きFETCHの応答でUID値を最後に出力していたのを最初に出力するように修正した。
	if (!bOnUid && bUID && bOK) {// UIDが未設定なら命令追加
	  for (n1 = 0; n1 < nTotal; n1++)
	  {
		if (!strcmp(pDec[n1], "uid"))
		{
		  bINUID = TRUE;
		  break;
		}
	  }
	  if (!bINUID)
	  {
	    for (n1 = nTotal; n1 > 0; n1--)
		{
		  pDec[n1] = pDec[n1-1];
		}
	    strcpy(p3, "UID");
	    pDec[n1] = p3;
		nTotal++;
	  }
	}
#else
	if (!bOnUid && bUID && bOK) {// UIDが未設定なら命令追加
	  strcpy(p3, "UID");
	  pDec[nTotal++] = p3;
	}
#endif
	if (p) {
	  nTotal++; 
	}

	return nTotal;
}

void FetchMSGByData(PCLIENT_CONTEXT lpClientContext, char *pNo, char **pDec) {
#ifdef UPDATE_20190510 // NTFS以外のファイルシステムからのファイル一覧取得可能にする対策（一覧ソート）を追加
  DWORD            nSearch, no;
  WIN32_FIND_DATA  *pfd;
#endif
  HANDLE           hSearch;
  PImap4Context    pContext = &lpClientContext->Context;
  char             *p2; 
  CHAR             MailDir[MAX_PATH], mMSG[MAX_PATH];
  CHAR             mSrc[MAX_PATH], mDest[MAX_PATH];
  CHAR             mUID[32];
  //CHAR             mFlags[7] = {"??????"};
  DWORD            nNo = 0, nMSG = 0, nDec = 0, nFrom = 0, nTo = 0;
  BOOL             bNOT = FALSE, bFromTo = FALSE;
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
#ifdef UPDATE_20230706 // FETCH時のメールデータを事前に一括して読込み検索するフラグを追加
  unsigned long    nfsize;
  char mFn[256];
#endif
#ifdef UPDATE_20240827 // FETCH命令で処理トーク解析処理の際のCPU負荷を下げる対策
  DWORD  nCloolStep = 0;;
#endif

	if (pNo) {
      bFromTo = TRUE;
	  if ((p2 = strstr(pNo, ":"))) {
        *p2 = '\x0';
	    p2++;
#ifdef UPDATE_20151218 // 範囲指定の*のとき処理の修正
	    if (*pNo == '*')
	 	  nFrom = (pContext->bUID ?  pContext->nUid-1 : pContext->nExists);
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
    mMSG[0] = '\x0';
#ifdef UPDATE_20110302A // フラグを変更対象のファイル名の一覧表を作成後にリネーム処理し、"~SG"が残されないようにする対策。
#ifdef UPDATE_20151115 // SELECT時にメール一覧をキャッシュするためのポインタ
   if (bSELECT && pContext->pFnLists)
   {
/*
     for (nNo = 1; nNo <= pContext->nExists; nNo++)
	 {
	   if (pContext->bUID) { // MSGのUID
 	     strcpy(mUID, (CHAR *)((SFL *)(pContext->pFnLists+(nNo-1))));
         strtok(mUID, "-");
	     nMSG = (DWORD) atol(mUID);
	   } else {
	     nMSG++; // MSGファイル数のカウント
	   }
  	   if ( (!bFromTo || (bFromTo && nFrom <= nMSG && (nTo == 0 || nTo >= nMSG) )) ) {
		 (SFL *)(pContext->pFnLists+(nNo-1))->bSel = TRUE;
*/
		   /*
	     sprintf(mSrc,  "%s\\%s", pContext->mSelectDir, (CHAR *)((SFL *)(pContext->pFnLists+(nNo-1))->mFn));
         fprintf(fp1, "%s\t%lu\n", (CHAR *)((SFL *)(pContext->pFnLists+(nNo-1))->mFn), nNo);
#ifdef UPDATE_20110405B // インデックス用ファイルに一覧書込みが完全に完了させる対策。
	        fflush(fp1);
#endif
			*/
/*
	   }
	 }
*/
   } else 
#endif
   {
    sprintf(mIdx, "%s\\fetch", pContext->mSelectDir);  // STROE一覧
	if ((fp1 = fopen(mIdx, "wt")))
	{
	  {
	    //メッセージの相対位置の取得が必要 20110203Aの変更してはいけない
#ifdef UPDATE_20151225 // 試作：UID値によるファイル範囲指定のチューニング
        GetQueryFileArea(MailDir, pContext->mSelectDir, pContext->bUID, bFromTo, nFrom, nTo);
#else
        sprintf(MailDir, "%s\\*-??????.MSG", pContext->mSelectDir);  // メッセージフォルダ取出し
#endif
#ifdef UPDATE_20190510 // NTFS以外のファイルシステムからのファイル一覧取得可能にする対策（一覧ソート）を追加
   if (bOtherFS) // TRUE:対応する FALSE:対応しない（旧仕様）
   {
     pfd = FindFirstFileSort((LPCTSTR)MailDir, &pContext->FindData, &nSearch);
/*
#ifdef ACCEPT_LOG_LEVEL3
     sprintf(mLog, "FindFirstFileSort() / pfd=%x / nSearch=%lu\n", pfd, nSearch);
     LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
#endif
*/
	 if (!pfd)
	 {
#ifdef UPDATE_20110404 // 選択フォルダ単位で排他する対策
       UnLockMailSelectDirectory(pContext->mSelectDir);
#endif
#ifdef UPDATE_20130529 // フォルダ作成直後に検査用一時ファイルが開放されずにフォルダが削除できなくなる不具合。
       fclose(fp1);
#endif
        return;
     }
     no = 0;
   } else {
     hSearch = FindFirstFile((LPCTSTR)MailDir, &pContext->FindData);
     if (hSearch == INVALID_HANDLE_VALUE)
 	 {
#ifdef UPDATE_20110404 // 選択フォルダ単位で排他する対策
       UnLockMailSelectDirectory(pContext->mSelectDir);
#endif
#ifdef UPDATE_20130529 // フォルダ作成直後に検査用一時ファイルが開放されずにフォルダが削除できなくなる不具合。
       fclose(fp1);
#endif
       return;
  	 }
   }
#else
        hSearch = FindFirstFile((LPCTSTR)MailDir, &pContext->FindData);
        if (hSearch == INVALID_HANDLE_VALUE)
		{
#ifdef UPDATE_20110404 // 選択フォルダ単位で排他する対策
          UnLockMailSelectDirectory(pContext->mSelectDir);
#endif
#ifdef UPDATE_20130529 // フォルダ作成直後に検査用一時ファイルが開放されずにフォルダが削除できなくなる不具合。
          fclose(fp1);
#endif
          return;
		}
#endif
        do {
/*
#ifdef ACCEPT_LOG_LEVEL3
          sprintf(mLog, "FetchMSGByData(%s) / nSearch=%lu / no=%lu\n", pContext->FindData.cFileName, nSearch, no);
          LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
#endif
*/
#ifdef UPDATE_20240830B // FETCH命令で処理トーク解析処理の際のCPU負荷を下げる対策
           nCloolStep++;
           if (nPeekCoolTime)
		   {
	         if ((nCloolStep % nPeekCoolTime) == 0)
			 {
#ifdef UPDATE_20240830A // FETCH命令で処理トーク解析処理の際のCPU負荷を下げる対策(UPDATE_20240827 でスリープする時間を0からPeekCoolTimeとした)
			   _sleep(nPeekCoolTime);
#else
	           _sleep(0);
#endif
			 }
		   }
#endif
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
	         sprintf(mSrc,  "%s\\%s", pContext->mSelectDir, pContext->FindData.cFileName);
             fprintf(fp1, "%s\t%lu\n", pContext->FindData.cFileName, nNo);
#ifdef UPDATE_20110405B // インデックス用ファイルに一覧書込みが完全に完了させる対策。
	         fflush(fp1);
#endif
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
#ifdef UPDATE_20230706_DEBUG // FETCH時のメールデータを事前に一括して読込み検索するフラグを追加
if (bDebug) printf("FetchMSGByData()  bOtherFS=%d / FindClose(%x)\n", bOtherFS, pfd);
#endif
      if (bOtherFS)
        FindCloseSort(pfd);
      else
  	    FindClose(hSearch);
#else
		} while (FindNextFile(hSearch, &pContext->FindData));
  	    FindClose(hSearch);
#endif
	  }

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
   }
	//メッセージの相対位置の取得が必要 20110203Aの変更してはいけない
#ifdef UPDATE_20110302A // フラグを変更対象のファイル名の一覧表を作成後にリネーム処理し、"~SG"が残されないようにする対策。
#ifdef UPDATE_20151115 // SELECT時にメール一覧をキャッシュするためのポインタ
   if (bSELECT && pContext->pFnLists)
   {
     for (nNo = 1; nNo <= pContext->nExists; nNo++)
	 {
	   if (pContext->bUID) { // MSGのUID
 	     strcpy(mUID, (CHAR *)((SFL *)(pContext->pFnLists+(nNo-1))));
         strtok(mUID, "-");
	     nMSG = (DWORD) atol(mUID);
	   } else {
	     nMSG++; // MSGファイル数のカウント
	   }
  	   if ( (!bFromTo || (bFromTo && nFrom <= nMSG && (nTo == 0 || nTo >= nMSG) )) ) 
	   {
	     strcpy(pContext->FindData.cFileName, (pContext->pFnLists+(nNo-1))->mFn);
		 goto sel_cache_1;
sel_cache_2:
		 printf("\n");
	   }
	 }
	 goto sel_cache_3;
   } else 
#endif
	//nNo = 0; 
	//nMSG = 0; 
    sprintf(mIdx, "%s\\fetch", pContext->mSelectDir);  // STROE一覧
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
		sprintf(mLog, "FetchMSGByData() index error / mList[%s]\n", mList);
        LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
#endif
		break;
	  } else 
#endif
		{
	    //strtok(pContext->FindData.cFileName, "\r\n");
	    nNo = atol(&mList[22]);
	    strtok(mList, "\t");
	    strcpy(pContext->FindData.cFileName, mList);
#else
#ifdef UPDATE_20151225 // 試作：UID値によるファイル範囲指定のチューニング
    GetQueryFileArea(MailDir, pContext->mSelectDir, pContext->bUID, bFromTo, nFrom, nTo);
#else
    sprintf(MailDir, "%s\\*-??????.MSG", pContext->mSelectDir);  // メッセージフォルダ取出し
#endif
#ifdef UPDATE_20190510 // NTFS以外のファイルシステムからのファイル一覧取得可能にする対策（一覧ソート）を追加
   pfd = FindFirstFileSort((LPCTSTR)MailDir, &pContext->FindData, &nSearch);
   if (!pfd)
   {
#ifdef UPDATE_20110404 // 選択フォルダ単位で排他する対策
      UnLockMailSelectDirectory(pContext->mSelectDir);
#endif
#ifdef UPDATE_20130529 // フォルダ作成直後に検査用一時ファイルが開放されずにフォルダが削除できなくなる不具合。
      unlink(mIdx);
      fclose(fp1);
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
#ifdef UPDATE_20130529 // フォルダ作成直後に検査用一時ファイルが開放されずにフォルダが削除できなくなる不具合。
      unlink(mIdx);
      fclose(fp1);
#endif
      return;
    }
#endif
    do {
#ifdef UPDATE_20240830B // FETCH命令で処理トーク解析処理の際のCPU負荷を下げる対策
           nCloolStep++;
           if (nPeekCoolTime)
		   {
	         if ((nCloolStep % nPeekCoolTime) == 0)
			 {
#ifdef UPDATE_20240830A // FETCH命令で処理トーク解析処理の際のCPU負荷を下げる対策(UPDATE_20240827 でスリープする時間を0からPeekCoolTimeとした)
			   _sleep(nPeekCoolTime);
#else
	           _sleep(0);
#endif
			 }
		   }
#endif
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
		if (pContext->bUID) { // MSGのUID
		  strcpy(mUID, pContext->FindData.cFileName);
          strtok(mUID, "-");
		  nMSG = (DWORD) atol(mUID);
		} else {
		  nMSG++; // MSGファイル数のカウント
		}
#ifndef UPDATE_20110302A // フラグを変更対象のファイル名の一覧表を作成後にリネーム処理し、"~SG"が残されないようにする対策。
		if ( (!bFromTo || (bFromTo && nFrom <= nMSG && (nTo == 0 || nTo >= nMSG) )) )
#endif
		{
#ifdef UPDATE_20151115 // SELECT時にメール一覧をキャッシュするためのポインタ
sel_cache_1:
#endif
		   /// ファイルの暗号化への対応
	       sprintf(mSrc,  "%s\\%s", pContext->mSelectDir, pContext->FindData.cFileName);
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
           strcpy(mDest, pContext->FindData.cFileName);
           strtok(mDest, ".");
           sprintf(pContext->mDecFileName, "%s_%lu.ENC", mDest, pContext->nLogInID);
           sprintf(mDest, "%s\\%s", pContext->mSelectDir, pContext->mDecFileName);
		   pContext->bEncMSG = SPA_CopyFileDecode(mSrc, mDest);
		   ///////////////////////////////
		   sprintf(pContext->mMessages, "* %u FETCH (", nNo); //nMSG); //hNo);
		   put_reply(lpClientContext, FALSE, TRUE);
		   *pContext->mMessages = '\x0';
#ifdef UPDATE_20230706 // FETCH時のメールデータを事前に一括して読込み検索するフラグを追加
           pContext->pBuff = NULL;
           if (bBulkFetch)
		   {
             sprintf(mFn, "%s/%s", pContext->mSelectDir, (pContext->bEncMSG ? pContext->mDecFileName : pContext->FindData.cFileName));
//printf("mFn[%s]\n", mFn);
             if ((fp = fopen(mFn, "rb"))) 
			 {
               nfsize = filelength(fileno(fp));
              if (pContext->pBuff = malloc(nfsize+1))
			  {
                //pfBufp = pContext->pBuff;
                fread(pContext->pBuff, 1, nfsize, fp);
                *(pContext->pBuff+nfsize)='\x0';
			  }
              fclose(fp);
			 }
		   }
		   pContext->pfBufp = pContext->pfBuff = pContext->pBuff;
#endif

#ifdef UPDATE_20240827 // FETCH命令で処理トーク解析処理の際のCPU負荷を下げる対策
           nCloolStep++;
           if (nPeekCoolTime)
		   {
	         if ((nCloolStep % nPeekCoolTime) == 0)
			 {
#ifdef UPDATE_20240830A // FETCH命令で処理トーク解析処理の際のCPU負荷を下げる対策(UPDATE_20240827 でスリープする時間を0からPeekCoolTimeとした)
			   _sleep(nPeekCoolTime);
#else
	           _sleep(0);
#endif
			 }
		   }
#endif
		   if (Working_Fetch(lpClientContext, pDec)) { // \Seen セットなら
		     sprintf(mSrc,  "%s\\%s", pContext->mSelectDir, pContext->FindData.cFileName);
             pContext->FindData.cFileName[UIDLEN+RECENT] = '0';
#ifdef UPDATE_20101224A // フラグを変更したファイルがファイル一覧取得時に反映されないようにする対策
#ifdef UPDATE_20110203A //フラグ変更中に新着の検出が発生したとき最後のUIDを正しく取得できないタイミングがある不具合
		     sprintf(mDest, "%s\\%s", pContext->mSelectDir, pContext->FindData.cFileName);
#ifndef UPDATE_20110302A // フラグを変更対象のファイル名の一覧表を作成後にリネーム処理し、"~SG"が残されないようにする対策。
			 mDest[strlen(mDest)-3] = '~';
#endif
#ifdef UPDATE_20240830 // FETCH命令でフラグ操作内容が現状と同じ場合はスキップする
			 if (!stricmp(mSrc, mDest))
			 {
#ifdef ACCEPT_LOG_LEVEL3
		     sprintf(mLog, "MoveFile(%s -> %s) skip.\n", mSrc, mDest);
             LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
#endif

			 } else {
#endif
#ifdef UPDATE_20110301A // FETCHコマンドでフラグ変更時に拡張子"~SG"のファイルが残ってしまうタイミングが無いようにする対策
		     //DeleteFile(mDest); // フラグ変更
		     rename(mSrc, mDest);
#else
		     MoveFileEx(mSrc, mDest, MOVEFILE_WRITE_THROUGH); // フラグ変更
#endif
#ifdef ACCEPT_LOG_LEVEL3
		     sprintf(mLog, "MoveFile(%s -> %s)\n", mSrc, mDest);
             LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
#endif
#else
		     sprintf(mDest, "%s\\~%s", pContext->mSelectDir, pContext->FindData.cFileName);
		     MoveFile(mSrc, mDest); // フラグ変更
#endif
			 while(!(fp = fopen(mDest, "rt")))
			 {
#ifdef UPDATE_20110301A // FETCHコマンドでフラグ変更時に拡張子"~SG"のファイルが残ってしまうタイミングが無いようにする対策
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
#ifdef UPDATE_20240830 // FETCH命令でフラグ操作内容が現状と同じ場合はスキップする
			 }
#endif
#else
		     sprintf(mDest, "%s\\%s", pContext->mSelectDir, pContext->FindData.cFileName);
		     MoveFile(mSrc, mDest); // フラグ変更
#endif
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
#ifdef UPDATE_20230706 // FETCH時のメールデータを事前に一括して読込み検索するフラグを追加
           if (bBulkFetch && pContext->pBuff)
             free(pContext->pBuff);
#endif
		   strcpy(pContext->mMessages, ")\r\n");
           put_reply(lpClientContext, TRUE, TRUE);
		   if (pContext->bEncMSG) {
             sprintf(mDest, "%s\\%s", pContext->mSelectDir, pContext->mDecFileName);
			 DeleteFile(mDest);
		   }
		}
#ifdef UPDATE_20110405B // インデックス用ファイルに一覧書込みが完全に完了させる対策。
	    memset(mList, 0, sizeof(mList));
#endif
		}
#ifdef UPDATE_20110302A // フラグを変更対象のファイル名の一覧表を作成後にリネーム処理し、"~SG"が残されないようにする対策。
#ifdef UPDATE_20151115 // SELECT時にメール一覧をキャッシュするためのポインタ
        if (bSELECT && pContext->pFnLists)
          goto sel_cache_2;
#endif
	  }
	  fclose(fp1);
    }
#else
#ifdef UPDATE_20190510 // NTFS以外のファイルシステムからのファイル一覧取得可能にする対策（一覧ソート）を追加
	  no++;
	  if (no < nSearch) {
	    FindNextFileSort(&pContext->FindData, pfd+no);
	  }
	} while(no < hSearch);
	FindCloseSort(pfd);
#else
    } while (FindNextFile(hSearch, &pContext->FindData));
	FindClose(hSearch);
#endif
#endif
	//////////////////////////////////////////
#ifdef UPDATE_20110302A // フラグを変更対象のファイル名の一覧表を作成後にリネーム処理し、"~SG"が残されないようにする対策。
	unlink(mIdx);
#else
#ifdef UPDATE_20101224A // フラグを変更したファイルがファイル一覧取得時に反映されないようにする対策
#ifdef UPDATE_20110203A //フラグ変更中に新着の検出が発生したとき最後のUIDを正しく取得できないタイミングがある不具合
    sprintf(MailDir, "%s\\*-??????.~SG", pContext->mSelectDir);  // メッセージフォルダ取出し
#else
    sprintf(MailDir, "%s\\~*-??????.MSG", pContext->mSelectDir);  // メッセージフォルダ取出し
#endif
#ifdef UPDATE_20190510 // NTFS以外のファイルシステムからのファイル一覧取得可能にする対策（一覧ソート）を追加
    pfd = FindFirstFileSort((LPCTSTR)MailDir, &pContext->FindData, &nSearch);
    if (!pfd) {
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
    do {
#ifdef UPDATE_20240830B // FETCH命令で処理トーク解析処理の際のCPU負荷を下げる対策
           nCloolStep++;
           if (nPeekCoolTime)
		   {
	         if ((nCloolStep % nPeekCoolTime) == 0)
			 {
#ifdef UPDATE_20240830A // FETCH命令で処理トーク解析処理の際のCPU負荷を下げる対策(UPDATE_20240827 でスリープする時間を0からPeekCoolTimeとした)
			   _sleep(nPeekCoolTime);
#else
	           _sleep(0);
#endif
			 }
		   }
#endif
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
 	    sprintf(mSrc, "%s\\%s", pContext->mSelectDir, pContext->FindData.cFileName);
#ifdef UPDATE_20110203A //フラグ変更中に新着の検出が発生したとき最後のUIDを正しく取得できないタイミングがある不具合
	    sprintf(mDest, "%s\\%s", pContext->mSelectDir, pContext->FindData.cFileName);
	    mDest[strlen(mDest)-3] = 'M';
#ifdef UPDATE_20110301A // FETCHコマンドでフラグ変更時に拡張子"~SG"のファイルが残ってしまうタイミングが無いようにする対策
		//DeleteFile(mDest); // フラグ変更
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
	    while(!(fp = fopen(mDest, "rt")))
		{
#ifdef UPDATE_20110301A // FETCHコマンドでフラグ変更時に拡張子"~SG"のファイルが残ってしまうタイミングが無いようにする対策
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
#ifdef UPDATE_20151115 // SELECT時にメール一覧をキャッシュするためのポインタ
sel_cache_3:
#endif
	//////////////////////////////////////////
#ifdef UPDATE_20110404 // 選択フォルダ単位で排他する対策
    UnLockMailSelectDirectory(pContext->mSelectDir);
#endif
}

void X_StrCpy(char *pDest, char *pSrc, int nDest, int nSrc, BOOL bContinue) {
   int  i, n;
   char *p, *pD;
   BOOL bTop = FALSE;

   strtok(pSrc, "\r\n");
   if (*pSrc == '\r' || *pSrc == '\n')
	 *pSrc = '\x0';
   if (bContinue) {
     pD = strchr(pDest, '\x0');
	 p = pSrc;
   } else {
     pD = pDest;
     p = strstr(pSrc, ":");
     nSrc -= (int) (p - pSrc);
     if (p) 
	   p++;
     else 
	   p = pSrc;
   }
   *pD = '\x0';
   n = strlen(pDest);
   for (i = 0; i < nSrc; i++) {
	 if (*p == '\x0') //データ終了
	   break;
#ifdef UPDATE_20231117 // FTECH ENVELOPEでFROMの定義可能な上限を増やした。
	 if (i < (nDest-1) ) 
#else
	 if (n+i < (nDest-1) ) 
#endif
	 {
	   if (bTop || !bTop && *p != ' ' && *p != '\t') { // 先頭の空白の読み飛ばし
         bTop = TRUE;
		 if (*p >= 0x20 && *p <= 0x7f && *p != '"' && *p != '\\') { // 有効文字のみ置換え
	       *pD = *p;
		   pD++;
		 }
         *pD = '\x0';
	   /*
	   if (bContinue) 
		 strncat(pDest, p, nDest-1);
	   else
	     strncpy(pDest, p, nDest-1);
	   break;
	   */
	   }
	 } else {  // サイズ超過
	   break;
	 }
	 p++;
   }
   //*pD = '\x0';
}

BOOL Working_Fetch(PCLIENT_CONTEXT lpClientContext, char **pDec)  {
  DWORD            nDec = 0, nHeader = 0, i;
  int              nType;
  char             *p, *p2, *pReq, *pEnd, *pRange;
  BOOL             bSeen = FALSE, bSet = FALSE;
  PImap4Context    pContext = &lpClientContext->Context;
  char             *pHeader[MAX_ATTRIBUTE];
#ifdef UPDATE_20091129D // CatchMe@Mailで添付きメールの表示が失敗する
  BOOL bBodyPeek;
  char *q;
#endif
#ifdef UPDATE_20150623 // 一括でのBODY情報取得時に2件目以降の応答内容がが欠けてしまう不具合。
  char *r;
#endif
#ifdef UPDATE_20150707 // BODY[]<x.n>で指定した応答でBODY[]<x>で応答していない不具合。
  char *pPeriod;
#endif
#ifdef UPDATE_20231119A // MIMEパートに対する、"HEADER.FIELDS""命令がが失敗する不具合。
  BOOL bEnd;
#endif

  nDec = 0;
  while(pDec[nDec]) {

    p = pDec[nDec];
	if (nDec == 0 && !bSet) {
	  *pContext->mMessages = '\x0';
	  bSet = TRUE;
	} else {
	  if (*p) {
	    strcpy(pContext->mMessages," ");
        put_reply(lpClientContext, FALSE, TRUE);
	  }
	}
	if (!stricmp(p,"UID")) {
	  fetch_uid(lpClientContext);
	  bSet = TRUE;
	} else
    if (!stricmp(p, "FLAGS")) {
	  fetch_flags(lpClientContext);
	} else if (!stricmp(p,"INTERNALDATE")) {
	  fetch_internaldate(lpClientContext);
    } else if (!stricmp(p,"RFC822.SIZE")) {
	  fetch_rfc822_size(lpClientContext);
    } else if (!stricmp(p,"ENVELOPE")) {
      fetch_envelope(lpClientContext);
    } else if (!stricmp(p,"BODY")) {
#ifdef UPDATE_20150630 // MESSAGE/RFC822の構造出力への対応
	  fetch_bodystructure(lpClientContext, FALSE, NULL); // 拡張なし
#else
	  fetch_bodystructure(lpClientContext, FALSE); // 拡張なし
#endif
    } else if (!stricmp(p,"BODYSTRUCTURE")) {
#ifdef UPDATE_20150630 // MESSAGE/RFC822の構造出力への対応
	  fetch_bodystructure(lpClientContext, TRUE, NULL); // 拡張
#else
      fetch_bodystructure(lpClientContext, TRUE); // 拡張
#endif
    } else if (!stricmp(p,"RFC822.PEEK") || 
		       !stricmp(p,"RFC822") || 
			   !stricmp(p,"BODY[]") || 
			   !stricmp(p,"BODY.PEEK[]") ||
               !strnicmp(p,"RFC822.PEEK<", 12) || 
		       !strnicmp(p,"RFC822<", 7) || 
			   !strnicmp(p,"BODY[]<", 7) || 
			   !strnicmp(p,"BODY.PEEK[]<", 12)
			   ) {
  	  pRange = strchr(p, '<'); // 範囲指定の有無
#ifdef UPDATE_20150707 // BODY[]<x.n>で指定した応答でBODY[]<x>で応答していない不具合。
	  if (pRange)
	  {
        pPeriod = strchr(pRange, '.');
		*pPeriod = '\x0';
	  } else {
		pPeriod = NULL;
	  }
	  if (!stricmp(p,"RFC822") || !stricmp(p,"RFC822.PEEK") || !strnicmp(p,"RFC822.PEEK<", 12) || !strnicmp(p,"RFC822<", 7))
		sprintf(pContext->mMessages, "RFC822%s%s {", (pRange ? pRange : ""), (pRange ? ">" : ""));
	  else
	    sprintf(pContext->mMessages, "BODY[]%s%s {", (pRange ? pRange : ""), (pRange ? ">" : ""));
	  if (pPeriod)
	  {
		*pPeriod = '.';
	  }
#else
	  if (!stricmp(p,"RFC822") || !stricmp(p,"RFC822.PEEK") || !strnicmp(p,"RFC822.PEEK<", 12) || !strnicmp(p,"RFC822<", 7))
		strcpy(pContext->mMessages, "RFC822 {");
	  else
	    strcpy(pContext->mMessages, "BODY[] {");
#endif
	  put_reply(lpClientContext, FALSE, TRUE);
      fetch_rfc822(lpClientContext, 0, "0", pRange);
	  if (stricmp(p,"RFC822.PEEK") && stricmp(p,"BODY.PEEK[]")) // BODY.PEEK[]以外なら
	    bSeen = TRUE;
    } else if (!stricmp(p,"RFC822.HEADER")) {
      strcpy(pContext->mMessages, "RFC822.HEADER {");
	  put_reply(lpClientContext, FALSE, TRUE);
#ifdef UPDATE_20231121 // HEADER.FIELDS命令の応答で範囲指定が指定できない不具合。
	  fetch_rfc822_header(lpClientContext, 0, NULL, NULL); // ヘッダ全て
#else
	  fetch_rfc822_header(lpClientContext, 0, NULL); // ヘッダ全て
#endif
    } else if (!strnicmp(p,"RFC822.TEXT.PEEK", 16) ||
		       !strnicmp(p,"RFC822.TEXT", 11) ||
			   !strnicmp(p,"BODY[TEXT]", 10) ||
			   !strnicmp(p,"BODY.PEEK[TEXT]", 15) ) {
  	  pRange = strchr(p, '<'); // 範囲指定の有無
#ifdef UPDATE_20150707 // BODY[]<x.n>で指定した応答でBODY[]<x>で応答していない不具合。
	  if (pRange)
	  {
        pPeriod = strchr(pRange, '.');
		*pPeriod = '\x0';
	  } else {
		pPeriod = NULL;
	  }
      sprintf(pContext->mMessages, "BODY[TEXT]%s%s {", (pRange ? pRange : ""), (pRange ? ">" : ""));
	  if (pPeriod)
	  {
		*pPeriod = '.';
	  }
#else
	  strcpy(pContext->mMessages, "BODY[TEXT] {");
#endif
	  put_reply(lpClientContext, FALSE, TRUE);
	  fetch_rfc822(lpClientContext, 1, "text", pRange);
	  if (strnicmp(p,"RFC822.TEXT.PEEK", 16) && strnicmp(p,"BODY.PEEK[TEXT]", 15)) // BODY.PEEK[]以外なら
	    bSeen = TRUE;
    } else if (!stricmp(p,"RFC822.HEADER.LINES")) {
      //f[k++] = fetch_rfc822_header_lines;
#ifdef UPDATE_20091129A // CatchMe@Mailで添付きメールの表示が失敗する
		strcpy(pContext->mMessages, "RFC822.HEADER.LINES (");
	    put_reply(lpClientContext, FALSE, TRUE);
		//////////////////////////////
        for (nHeader = 0; nHeader < MAX_ATTRIBUTE; nHeader++) //// 初期化
	      pHeader[nHeader] = NULL;
		nHeader = 0;
		while (p = pDec[++nDec])
		{
		  if (*p == '(')
		    p++;
          if ((pEnd = strrchr(p, ')')))
		    *pEnd = '\x0';
	      FLAGSDecipher(pContext->bUID, p, &pHeader[nHeader], FALSE);
		  nHeader++;
		}
		//////////////////////////////
        for (nHeader = 0; nHeader < MAX_ATTRIBUTE; nHeader++) { //
		  if (pHeader[nHeader] == NULL)
			break;
          sprintf(pContext->mMessages, "%s%s", (nHeader ? " " : "" ), pHeader[nHeader]);
	      put_reply(lpClientContext, FALSE, TRUE);
		}
        strcpy(pContext->mMessages, ") {");
	    put_reply(lpClientContext, FALSE, TRUE);
		//////////////////////////////
#ifdef UPDATE_20231121 // HEADER.FIELDS命令の応答で範囲指定が指定できない不具合。
	    fetch_rfc822_header(lpClientContext, 1, pHeader, NULL); // 指定したヘッダ
#else
	    fetch_rfc822_header(lpClientContext, 1, pHeader); // 指定したヘッダ
#endif
        /////////////////////////////
        //strcpy(pContext->mMessages, ")\r\n");
	    //put_reply(lpClientContext, TRUE, TRUE);
        /////////////////////////////
        for (nHeader = 1; nHeader < MAX_ATTRIBUTE; nHeader++) { //
		  if (pHeader[nHeader] == NULL)
			break;
          *(pHeader[nHeader]-1) = ' ';
		}
        /////////////////////////////
#endif
    } else if (!stricmp(p,"RFC822.HEADER.LINES.NOT")) {
      //f[k++] = fetch_rfc822_header_lines_not;
#ifdef UPDATE_20091129A // CatchMe@Mailで添付きメールの表示が失敗する
		strcpy(pContext->mMessages, "RFC822.HEADER.LINES.NOT (");
	    put_reply(lpClientContext, FALSE, TRUE);
		//////////////////////////////
        for (nHeader = 0; nHeader < MAX_ATTRIBUTE; nHeader++) //// 初期化
	      pHeader[nHeader] = NULL;
		nHeader = 0;
		while (p = pDec[++nDec])
		{
		  if (*p == '(')
		    p++;
          if ((pEnd = strrchr(p, ')')))
		    *pEnd = '\x0';
	      FLAGSDecipher(pContext->bUID, p, &pHeader[nHeader], FALSE);
		  nHeader++;
		}
		//////////////////////////////
        for (nHeader = 0; nHeader < MAX_ATTRIBUTE; nHeader++) { //
		  if (pHeader[nHeader] == NULL)
			break;
          sprintf(pContext->mMessages, "%s%s", (nHeader ? " " : "" ), pHeader[nHeader]);
	      put_reply(lpClientContext, FALSE, TRUE);
		}
        strcpy(pContext->mMessages, ") {");
	    put_reply(lpClientContext, FALSE, TRUE);
		//////////////////////////////
#ifdef UPDATE_20231121 // HEADER.FIELDS命令の応答で範囲指定が指定できない不具合。
	    fetch_rfc822_header(lpClientContext, 2, pHeader, NULL); // 指定以外のヘッダ
#else
	    fetch_rfc822_header(lpClientContext, 2, pHeader); // 指定以外のヘッダ
#endif
        /////////////////////////////
        //strcpy(pContext->mMessages, ")\r\n");
	    //put_reply(lpClientContext, TRUE, TRUE);
        /////////////////////////////
        for (nHeader = 1; nHeader < MAX_ATTRIBUTE; nHeader++) { //
		  if (pHeader[nHeader] == NULL)
			break;
          *(pHeader[nHeader]-1) = ' ';
		}
        /////////////////////////////
#endif
	} else if (!strnicmp(p,"BODY[",5) || !strnicmp(p,"BODY.PEEK[",10)) {
	  if (!stricmp(p,"BODY[HEADER]") ||
		  !stricmp(p,"BODY.PEEK[HEADER]")) {
#ifdef UPDATE_20150703A  // BODY[HEADER]のとき応答の先頭に余分なスペースを出力してしまっていた。
		strcpy(pContext->mMessages, "BODY[HEADER] {");
#else
		strcpy(pContext->mMessages, " BODY[HEADER] {");
#endif
	    put_reply(lpClientContext, FALSE, TRUE);
#ifdef UPDATE_20231121 // HEADER.FIELDS命令の応答で範囲指定が指定できない不具合。
	    fetch_rfc822_header(lpClientContext, 0, NULL, NULL); // ヘッダ全て
#else
	    fetch_rfc822_header(lpClientContext, 0, NULL); // ヘッダ全て
#endif
	  } else if (!stricmp(p,"BODY[HEADER.FIELDS") || !stricmp(p,"BODY.PEEK[HEADER.FIELDS")) {
		//////////////////////////////
		strcpy(pContext->mMessages, "BODY[HEADER.FIELDS (");
	    put_reply(lpClientContext, FALSE, TRUE);
		//////////////////////////////
		p = pDec[++nDec];
		if (*p == '(')
		  p++;
        if ((pEnd = strrchr(p, ')')))
		  *pEnd = '\x0';
        for (nHeader = 0; nHeader < MAX_ATTRIBUTE; nHeader++) //// 初期化
	      pHeader[nHeader] = NULL;
		//////////////////////////////
	    FLAGSDecipher(pContext->bUID, p, pHeader, FALSE);
		//////////////////////////////
        for (nHeader = 0; nHeader < MAX_ATTRIBUTE; nHeader++) { //
		  if (pHeader[nHeader] == NULL)
			break;
          sprintf(pContext->mMessages, "%s%s", (nHeader ? " " : "" ), pHeader[nHeader]);
	      put_reply(lpClientContext, FALSE, TRUE);
		}
#ifdef UPDATE_20231121 // HEADER.FIELDS命令の応答で範囲指定が指定できない不具合。
		pRange = NULL;
		if (*pDec[nDec+1] == '<')
		{
		  pRange = pDec[++nDec];
		  if (p = strchr(pRange, '.'))
			*p = '\x0';
		  sprintf(pContext->mMessages, ")]%s> {", pRange);
		  if (p)
			*p = '.';
		} else
#endif
          strcpy(pContext->mMessages, ")] {");
	    put_reply(lpClientContext, FALSE, TRUE);
		//////////////////////////////
#ifdef UPDATE_20231121 // HEADER.FIELDS命令の応答で範囲指定が指定できない不具合。
	    fetch_rfc822_header(lpClientContext, 1, pHeader, pRange); // 指定したヘッダ
#else
	    fetch_rfc822_header(lpClientContext, 1, pHeader); // 指定したヘッダ
#endif
        /////////////////////////////
        for (nHeader = 1; nHeader < MAX_ATTRIBUTE; nHeader++) { //
		  if (pHeader[nHeader] == NULL)
			break;
          *(pHeader[nHeader]-1) = ' ';
		}
        /////////////////////////////
	  } else if (!stricmp(p,"BODY[HEADER.FIELDS.NOT") || !stricmp(p,"BODY.PEEK[HEADER.FIELDS.NOT")) {
		//////////////////////////////
		strcpy(pContext->mMessages, "BODY[HEADER.FIELDS.NOT (");
	    put_reply(lpClientContext, FALSE, TRUE);
		//////////////////////////////
		p = pDec[++nDec];
		if (*p == '(')
		  p++;
        if ((pEnd = strrchr(p, ')')))
		  *pEnd = '\x0';
        for (nHeader = 0; nHeader < MAX_ATTRIBUTE; nHeader++) //// 初期化
	      pHeader[nHeader] = NULL;
		//////////////////////////////
	    FLAGSDecipher(pContext->bUID, p, pHeader, FALSE);
		//////////////////////////////
        for (nHeader = 0; nHeader < MAX_ATTRIBUTE; nHeader++) { //
		  if (pHeader[nHeader] == NULL)
			break;
          sprintf(pContext->mMessages, "%s%s", (nHeader ? " " : "" ), pHeader[nHeader]);
	      put_reply(lpClientContext, FALSE, TRUE);
		}
#ifdef UPDATE_20231121 // HEADER.FIELDS命令の応答で範囲指定が指定できない不具合。
		pRange = NULL;
		if (*pDec[nDec+1] == '<')
		{
		  pRange = pDec[++nDec];
		  if (p = strchr(pRange, '.'))
			*p = '\x0';
		  sprintf(pContext->mMessages, ")]%s> {", pRange);
		  if (p)
			*p = '.';
		} else
#endif
          strcpy(pContext->mMessages, ")] {");
	    put_reply(lpClientContext, FALSE, TRUE);
		//////////////////////////////
#ifdef UPDATE_20231121 // HEADER.FIELDS命令の応答で範囲指定が指定できない不具合。
	    fetch_rfc822_header(lpClientContext, 2, pHeader, pRange); // 指定以外のヘッダ
#else
	    fetch_rfc822_header(lpClientContext, 2, pHeader); // 指定以外のヘッダ
#endif
        /////////////////////////////
        for (nHeader = 1; nHeader < MAX_ATTRIBUTE; nHeader++) { //
		  if (pHeader[nHeader] == NULL)
			break;
          *(pHeader[nHeader]-1) = ' ';
		}
        /////////////////////////////
	  } else {
#ifdef UPDATE_20231119A // MIMEパートに対する、"HEADER.FIELDS""命令がが失敗する不具合。
        bEnd = FALSE;
#endif
		if ((pReq = strchr(p,'['))) {
#ifdef UPDATE_20231119A // MIMEパートに対する、"HEADER.FIELDS""命令がが失敗する不具合。
		  if (pDec[nDec+1])
		  {
		    if (*pDec[nDec+1] == '(')
            {
              *(pDec[nDec+1]-1) = ' ';
		      nDec++;
#ifdef UPDATE_20231121 // HEADER.FIELDS命令の応答で範囲指定が指定できない不具合。
			  if (*pDec[nDec+1] == '<')
			  {
                *(pDec[nDec+1]-1) = ']';
		        nDec++;
			  }
#endif
			  bEnd = TRUE;
            }
		  }
#endif

#ifdef UPDATE_20091129D // CatchMe@Mailで添付きメールの表示が失敗する
          bBodyPeek = FALSE;
		  if (!strnicmp(p, "body.peek", 9))
		  {
			p+=5;
			*p = 'B';
			*(p+1) = 'O';
			*(p+2) = 'D';
			*(p+3) = 'Y';
            bBodyPeek = TRUE;
		  }
		  if ((q = strchr(p, '<')))
		  {
			while(!(*q == '.' || *q == '\x0'))
			{
			  q++;
			}
			if (*q == '.')
			{
			  *q = '\x0';
              sprintf(pContext->mMessages, "%s> {", p);
			  *q = '.';
			} else {
             sprintf(pContext->mMessages, "%s {", p);
			}
		  } else
#endif
		  {
#ifdef UPDATE_20231120B //"HEADER.FIELDS""命令の応答で末尾のカッコが欠落する場合がある不具合。
 			 if (strchr(p, '(') && !strchr(p, ')')) 
			   sprintf(pContext->mMessages, "%s%s {", p, (bEnd ? ")]":""));
			 else
			   sprintf(pContext->mMessages, "%s%s {", p, (bEnd ? "]":""));
#else
#ifdef UPDATE_20231119A // MIMEパートに対する、"HEADER.FIELDS""命令がが失敗する不具合。
	        sprintf(pContext->mMessages, "%s%s {", p, (bEnd ? "]":""));
#else
            sprintf(pContext->mMessages, "%s {", p);
#endif
#endif
		  }
	      put_reply(lpClientContext, FALSE, TRUE);
#ifdef UPDATE_20091129D // CatchMe@Mailで添付きメールの表示が失敗する
          if (bBodyPeek)
		  {
			p-=5;
			*p = 'B';
			*(p+1) = 'O';
			*(p+2) = 'D';
			*(p+3) = 'Y';
			*(p+4) = '.';
			*(p+5) = 'P';
			*(p+6) = 'E';
			*(p+7) = 'E';
			*(p+8) = 'K';
		  }
#endif
		  pReq++;
		  i = atoi(pReq);
#ifdef UPDATE_20150623A // BODY[HEADER]<x.n>で指定したヘッダが出力されない不具合。
		  if (i > 0) { 
#else
		  if (i >= 0) { // TEXT/PLAIN part
#endif
            pRange = strchr(pReq, '.');
			nType = 1;   // BODY表示
 			if (pRange) {
#ifdef UPDATE_20231119A // MIMEパートに対する、"HEADER.FIELDS""命令がが失敗する不具合。
			  if (!(p2 = strpbrk(pRange, "]<\r\n")))
                p2 = pRange+strlen(pRange);
#else
			  p2 = strpbrk(pRange, "]<\r\n");
#endif
			  while (pRange < p2) {
			    pRange++;
                if (!strnicmp(pRange, "MIME", 4) || 
				    !strnicmp(pRange, "HEADER", 6)) 
				  nType = 2;  // HEADER 表示
				if (!(pRange = strchr(pRange, '.')))
				  break;
			  }
 			}
  	        pRange = strchr(p, '<'); // 範囲指定の有無
#ifdef UPDATE_20150623 // 一括でのBODY情報取得時に2件目以降の応答内容がが欠けてしまう不具合。
			r = strtok(pReq, "]");
#else
			strtok(pReq, "]");
#endif
			fetch_rfc822(lpClientContext, nType, pReq, pRange);
#ifdef UPDATE_20150623 // 一括でのBODY情報取得時に2件目以降の応答内容がが欠けてしまう不具合。
			if (r)
			{
			  *(r+strlen(r)) = ']'; // 復帰
			}
#endif
		  } else if (!strnicmp(pReq, "HEADER", 6)) 
		  {
  	        pRange = strchr(p, '<'); // 範囲指定の有無
#ifdef UPDATE_20150623 // 一括でのBODY情報取得時に2件目以降の応答内容がが欠けてしまう不具合。
			r = strtok(pReq, "]");
#else
			strtok(pReq, "]");
#endif
			nType = 2;  // HEADER 表示
			fetch_rfc822(lpClientContext, nType, pReq, pRange);
#ifdef UPDATE_20150623 // 一括でのBODY情報取得時に2件目以降の応答内容がが欠けてしまう不具合。
			if (r)
			{
			  *(r+strlen(r)) = ']'; // 復帰
			}
#endif
		  } else {                       // その他
	        strcpy(pContext->mMessages, "2}\r\n\r\n");
	        put_reply(lpClientContext, TRUE, TRUE);
		  }
		};
	  }
	  if (stricmp(p,"BODY.PEEK[")) // BODY.PEEK[]以外なら
	    bSeen = TRUE;
	}
	nDec++;
  } 
  return bSeen;
}

void fetch_flags(PCLIENT_CONTEXT lpClientContext)  {
  PImap4Context    pContext = &lpClientContext->Context;
  BOOL             bSet = FALSE;

  // フラグ設定
  strcpy(pContext->mMessages, "FLAGS ("); // output attribute
  if (pContext->FindData.cFileName[UIDLEN+RECENT] == '1') {
	if (bSet)
      strcat(pContext->mMessages, " ");
	strcat(pContext->mMessages, "\\Recent");
	bSet = TRUE;
  }
#ifdef UPDATE_20130625 // $Forwarded フラグを利用可能にした
  if (pContext->FindData.cFileName[UIDLEN+ANSWERED] == '1' || pContext->FindData.cFileName[UIDLEN+ANSWERED] == '3') {
	if (bSet)
      strcat(pContext->mMessages, " ");
	strcat(pContext->mMessages, "\\Answered");
	bSet = TRUE;
  }
  if (pContext->FindData.cFileName[UIDLEN+ANSWERED] == '2' || pContext->FindData.cFileName[UIDLEN+ANSWERED] == '3') {
	if (bSet)
      strcat(pContext->mMessages, " ");
	strcat(pContext->mMessages, "$Forwarded");
	bSet = TRUE;
  }
#else
  if (pContext->FindData.cFileName[UIDLEN+ANSWERED] == '1') {
	if (bSet)
      strcat(pContext->mMessages, " ");
	strcat(pContext->mMessages, "\\Answered");
	bSet = TRUE;
  }
#endif
  if (pContext->FindData.cFileName[UIDLEN+DELETED] == '1') {
	if (bSet)
      strcat(pContext->mMessages, " ");
	strcat(pContext->mMessages, "\\Deleted");
	bSet = TRUE;
  }
  if (pContext->FindData.cFileName[UIDLEN+FLAGGED] == '1') {
	if (bSet)
      strcat(pContext->mMessages, " ");
	strcat(pContext->mMessages, "\\Flagged");
	bSet = TRUE;
  }
  if (pContext->FindData.cFileName[UIDLEN+SEEN] == '1') {
	if (bSet)
      strcat(pContext->mMessages, " ");
	strcat(pContext->mMessages, "\\Seen");
	bSet = TRUE;
  }
  if (pContext->FindData.cFileName[UIDLEN+DRAFT] == '1') {
	if (bSet)
      strcat(pContext->mMessages, " ");
	strcat(pContext->mMessages, "\\Draft");
	bSet = TRUE;
  }
  strcat(pContext->mMessages, ")");
  put_reply(lpClientContext, FALSE, TRUE);
}

void fetch_uid(PCLIENT_CONTEXT lpClientContext)  {
  PImap4Context    pContext = &lpClientContext->Context;
  CHAR             mUID[11];
  int              nSize = 0;

  // フラグ設定
  strcpy(pContext->mMessages, "UID "); // output attribute
  nSize = strlen(pContext->mMessages);
  strncpy(mUID, pContext->FindData.cFileName, 10);
  mUID[10] = '\x0';
  for (nSize = 0; nSize < 10; nSize++){
	if (mUID[nSize] != '0')
	  break;
  }
  strcat(pContext->mMessages, &mUID[nSize]);
  put_reply(lpClientContext, FALSE, TRUE);
}

void fetch_internaldate(PCLIENT_CONTEXT lpClientContext) {
  PImap4Context    pContext = &lpClientContext->Context;
  HANDLE           hFile;
  SYSTEMTIME       ltime, lt;
  FILETIME         ft;
  char             mFn[MAX_PATH];
  Imap4Envelope    mEnv;

   mEnv.mDate[0] = '\x0';
   sprintf(mFn, "%s/%s", pContext->mSelectDir, pContext->FindData.cFileName);
   hFile = CreateFile( (LPCTSTR)mFn,
                       GENERIC_READ,
                       0,
                       NULL,
                       OPEN_EXISTING,
                       FILE_ATTRIBUTE_NORMAL, 
                       NULL);
   if (hFile != INVALID_HANDLE_VALUE) {
     GetFileTime(hFile, NULL, NULL, &ft); // 更新日時
     CloseHandle(hFile);
	 FileTimeToSystemTime(&ft, &ltime);
	 SystemTimeToTzSpecificLocalTime(NULL, &ltime, &lt);
	 maketime(&lt, mEnv.mDate);
	 if (bDebug) printf("Time:%s\n", mEnv.mDate);
   }

  if (mEnv.mDate[0])
    sprintf(pContext->mMessages, "INTERNALDATE \"%s\"", mEnv.mDate);    // Date
  else
    strcpy(pContext->mMessages, "INTERNALDATE NIL");    // Date NULL
  put_reply(lpClientContext, FALSE, TRUE);
}

void fetch_rfc822_size(PCLIENT_CONTEXT lpClientContext) {
  PImap4Context    pContext = &lpClientContext->Context;
  FILE             *fp;
  ULARGE_INTEGER   u1;
  char             *p, mSize[64], mFn[MAX_PATH], mLine[1024];

  u1.QuadPart = 0;
  sprintf(mFn, "%s/%s", pContext->mSelectDir, (pContext->bEncMSG ? pContext->mDecFileName : pContext->FindData.cFileName));
#ifdef UPDATE_20230706 // FETCH時のメールデータを事前に一括して読込み検索するフラグを追加
  fp = NULL;
  if (pContext->pBuff) // ファイルの先頭にポインタリセット
    pContext->pfBuff = pContext->pBuff;
  if (pContext->pBuff || !pContext->pBuff && (fp = fopen(mFn, "rb"))) 
  {
#else
  if ((fp = fopen(mFn, "rb"))) 
  {
#endif
#ifdef UPDATE_20230706 // FETCH時のメールデータを事前に一括して読込み検索するフラグを追加
	if (pContext->pfBufp)
	  p = mgets(mLine, sizeof(mLine)-1, &pContext->pfBuff);
	else
#endif
    p = fgets(mLine, sizeof(mLine), fp);
	do {
	  if (!strcmp(mLine, ".\r\n"))
	    break;
      u1.QuadPart += strlen(mLine);
#ifdef UPDATE_20110126 // １カラム目のピリオドは出力しない対策
	  if (mLine[0] == '.')
	  {
        u1.QuadPart--;
	  }
#endif
#ifdef UPDATE_20230706 // FETCH時のメールデータを事前に一括して読込み検索するフラグを追加
	  if (pContext->pfBufp)
	    p = mgets(mLine, sizeof(mLine)-1, &pContext->pfBuff);
	  else
#endif
      p = fgets(mLine, sizeof(mLine), fp);
#ifdef UPDATE_20230706 // FETCH時のメールデータを事前に一括して読込み検索するフラグを追加
	} while (p || (pContext->pfBuff ? *pContext->pfBuff : !feof(fp)));
#else
    } while(p || !feof(fp));
#endif
#ifdef UPDATE_20230706 // FETCH時のメールデータを事前に一括して読込み検索するフラグを追加
    if (!pContext->pBuff && fp)
	  fclose(fp);
#else
	fclose(fp);
#endif
  }
  sprintf(mSize , "%lu", u1.QuadPart );
  strcpy(pContext->mMessages, "RFC822.SIZE ");
  strcat(pContext->mMessages, mSize);
  put_reply(lpClientContext, FALSE, TRUE);
}

void fetch_envelope(PCLIENT_CONTEXT lpClientContext) {
  PImap4Context    pContext = &lpClientContext->Context;
  DWORD            nLen;
  int              i = 0;
  BOOL             bContinue;
  FILE             *fp;
  char             *p;
#ifdef UPDATE_20230707 // FTECH ENVELOPEでTO,CC,BCCのアドレス定義可能な上限を増やした。
  char             mFn[MAX_PATH], mLine[0x20000];
#else
  char             mFn[MAX_PATH], mLine[1024];
#endif
  Imap4Envelope    mEnv;

  ///////////////////////////
  /// テーブル初期化
  mEnv.mDate[0] = '\x0';
  mEnv.mSubject[0] = '\x0';
  mEnv.mFrom[0] = '\x0';
  mEnv.mSender[0] = '\x0';
  mEnv.mReplyto[0] = '\x0';
  mEnv.mTo[0] = '\x0';
  mEnv.mCc[0] = '\x0';
  mEnv.mBcc[0] = '\x0';
  mEnv.mInreplyto[0] = '\x0';
  mEnv.mMessgeid[0] = '\x0';
  ///////////////////////////
  strcpy(pContext->mMessages, "ENVELOPE (");
  put_reply(lpClientContext, FALSE, TRUE);

  sprintf(mFn, "%s/%s", pContext->mSelectDir, (pContext->bEncMSG ? pContext->mDecFileName : pContext->FindData.cFileName));
#ifdef UPDATE_20230706 // FETCH時のメールデータを事前に一括して読込み検索するフラグを追加
  fp = NULL;
  if (pContext->pBuff) // ファイルの先頭にポインタリセット
    pContext->pfBuff = pContext->pBuff;
  if (pContext->pBuff || !pContext->pBuff && (fp = fopen(mFn, "rb"))) 
#else
  if ((fp = fopen(mFn, "rb"))) 
#endif
  {
#ifdef UPDATE_20230706 // FETCH時のメールデータを事前に一括して読込み検索するフラグを追加
	if (pContext->pfBufp)
	  p = mgets(mLine, sizeof(mLine)-1, &pContext->pfBuff);
	else
#endif
#ifdef UPDATE_20230707 // FTECH ENVELOPEでTO,CC,BCCのアドレス定義可能な上限を増やした。
    p = fgets(mLine, sizeof(mLine)-1, fp);
#else
    p = fgets(mLine, sizeof(mLine), fp);
#endif
#ifdef UPDATE_20230706 // FETCH時のメールデータを事前に一括して読込み検索するフラグを追加
    while(strcmp(mLine, "\r\n") && (p || (pContext->pfBuff ? *pContext->pfBuff : !feof(fp))))
#else
	while(strcmp(mLine, "\r\n") && (p || !feof(fp))) 
#endif
	{
	  if (i > 0 && (mLine[0] == '\t' || mLine[0] == ' '))
	 	bContinue = TRUE;
      else
	 	bContinue = FALSE;
	  if (strnicmp(mLine, "Date:", 5) == 0 || (bContinue && i == 1)) {
		i = 1; // Date:
		nLen = sizeof(mEnv.mDate) - strlen(mEnv.mDate);
	    X_StrCpy(mEnv.mDate, mLine, nLen, sizeof(mLine), bContinue);
	  } else if (strnicmp(mLine, "Subject:", 8) == 0 || (bContinue && i == 2)) {
		i = 2; // Subject:
		nLen = sizeof(mEnv.mSubject) - strlen(mEnv.mSubject);
		X_StrCpy(mEnv.mSubject, mLine, nLen, sizeof(mLine), bContinue);
	  } else if (strnicmp(mLine, "From:", 5) == 0 || (bContinue && i == 3)) {
		i = 3; // From:
		nLen = sizeof(mEnv.mFrom) - strlen(mEnv.mFrom);
		X_StrCpy(mEnv.mFrom, mLine, nLen, sizeof(mLine), bContinue);
#ifdef UPDATE_20230618 // FETCH ENVELOPE命令の応答でSender又はReplytoへの回答がNILだと開封確認ダイアログが表示されない(Denbun対策)
        if (!mEnv.mSender[0])
          X_StrCpy(mEnv.mSender, mLine, nLen, sizeof(mLine), bContinue);
        if (!mEnv.mReplyto[0])
          X_StrCpy(mEnv.mReplyto, mLine, nLen, sizeof(mLine), bContinue);
#endif
	  } else if (strnicmp(mLine, "Sender:", 7) == 0 || (bContinue && i == 4)) {
#ifdef UPDATE_20230914 // Reply-to:,Sender:ヘッダの情報取得でメモリリークする場合がある
		if (strnicmp(mLine, "Sender:", 7) == 0)
		  mEnv.mSender[0]='\x0';
#endif
		i = 4; // Sender:
		nLen = sizeof(mEnv.mSender) - strlen(mEnv.mSender);
		X_StrCpy(mEnv.mSender, mLine, nLen, sizeof(mLine), bContinue);
	  } else if (strnicmp(mLine, "Reply-To:", 9) == 0 || (bContinue && i == 5)) {
#ifdef UPDATE_20230914 // Reply-to:,Sender:ヘッダの情報取得でメモリリークする場合がある
		if (strnicmp(mLine, "Reply-To:", 9) == 0)
		  mEnv.mReplyto[0]='\x0';
#endif
		i = 5; // Reply-To:
		if (bContinue)
		  strcat(mEnv.mReplyto, "\t");
		nLen = sizeof(mEnv.mReplyto) - strlen(mEnv.mReplyto);
		X_StrCpy(mEnv.mReplyto, mLine, nLen, sizeof(mLine), bContinue);
	  } else if (strnicmp(mLine, "To:", 3) == 0 || (bContinue && i == 6)) {
		i = 6; // To:
		if (bContinue)
		  strcat(mEnv.mTo, "\t");
		nLen = sizeof(mEnv.mTo) - strlen(mEnv.mTo);
		X_StrCpy(mEnv.mTo, mLine, nLen, sizeof(mLine), bContinue);
	  } else if (strnicmp(mLine, "Cc:", 3) == 0 || (bContinue && i == 7)) {
		i = 7; // Cc:
		if (bContinue)
		  strcat(mEnv.mCc, "\t");
		nLen = sizeof(mEnv.mCc) - strlen(mEnv.mCc);
		X_StrCpy(mEnv.mCc, mLine, nLen, sizeof(mLine), bContinue);
	  } else if (strnicmp(mLine, "Bcc:", 4) == 0 || (bContinue && i == 8)) {
		i = 8; // Bcc:
		if (bContinue)
		  strcat(mEnv.mBcc, "\t");
		nLen = sizeof(mEnv.mBcc) - strlen(mEnv.mBcc);
		X_StrCpy(mEnv.mBcc, mLine, nLen, sizeof(mLine), bContinue);
	  } else if (strnicmp(mLine, "In-Reply-To:", 12) == 0 || (bContinue && i == 9)) {
		i = 9; // In-Reply-To:
		nLen = sizeof(mEnv.mInreplyto) - strlen(mEnv.mInreplyto);
		X_StrCpy(mEnv.mInreplyto, mLine, nLen, sizeof(mLine), bContinue);
	  } else if (strnicmp(mLine, "Message-ID:", 11) == 0 || (bContinue && i == 10)) {
		i = 10; // Message-ID:
		nLen = sizeof(mEnv.mMessgeid) - strlen(mEnv.mMessgeid);
		X_StrCpy(mEnv.mMessgeid, mLine, nLen, sizeof(mLine), bContinue);
	  } else {
		i = 0;
	  }
#ifdef UPDATE_20230706 // FETCH時のメールデータを事前に一括して読込み検索するフラグを追加
	  if (pContext->pfBufp)
	    p = mgets(mLine, sizeof(mLine)-1, &pContext->pfBuff);
	  else
#endif
#ifdef UPDATE_20230707 // FTECH ENVELOPEでTO,CC,BCCのアドレス定義可能な上限を増やした。
      p = fgets(mLine, sizeof(mLine)-1, fp);
#else
      p = fgets(mLine, sizeof(mLine), fp);
#endif
	}
#ifdef UPDATE_20230706 // FETCH時のメールデータを事前に一括して読込み検索するフラグを追加
    if (!pContext->pBuff && fp)
	  fclose(fp);
#else
	fclose(fp);
#endif
  }
  //////////////////////////
  if (mEnv.mDate[0]) {
#ifdef UPDATE_20091129 // ENVELOPE応答で、日付に曜日が含まれないとCatchMe@Mailが正常に一覧表示しない
    sprintf(pContext->mMessages, "\"%s\" ", mEnv.mDate);    // Date
#else
	if ((p = strpbrk(mEnv.mDate, ",")))
	 p+=2;
    sprintf(pContext->mMessages, "\"%s\" ", (p ? p : mEnv.mDate));    // Date
#endif
  } else {
    strcpy(pContext->mMessages, "NIL ");    // Date NULL
  }
  put_reply(lpClientContext, FALSE, TRUE);
  //////////////////////////
  if (mEnv.mSubject[0]) {
	//Encoding_For_Token(mEnv.mSubject);
	if (strstr(mEnv.mSubject, "\"")) // トークンに'"'ダブルクォーテーションがある場合の処置
      sprintf(pContext->mMessages, "{%d}\r\n%s ", strlen(mEnv.mSubject), mEnv.mSubject); // Subject
	else
      sprintf(pContext->mMessages, "\"%s\" ", mEnv.mSubject); // Subject
  } else {
    strcpy(pContext->mMessages, "NIL ");    // Subject NULL
  }
  put_reply(lpClientContext, FALSE, TRUE);
  //////////////////////////
  make_message(lpClientContext, pContext->mMessages, mEnv.mFrom); // From
  make_message(lpClientContext, pContext->mMessages, mEnv.mSender); // Sender
  make_message(lpClientContext, pContext->mMessages, mEnv.mReplyto); // Reply-To
  make_message(lpClientContext, pContext->mMessages, mEnv.mTo); // To
  make_message(lpClientContext, pContext->mMessages, mEnv.mCc); // Cc
  make_message(lpClientContext, pContext->mMessages, mEnv.mBcc); // Bcc
  //////////////////////////
  if (mEnv.mInreplyto[0])
    sprintf(pContext->mMessages, "\"%s\" ", mEnv.mInreplyto); // // In-Reply-To
  else
    strcpy(pContext->mMessages, "NIL ");    // // In-Reply-To NULL
  put_reply(lpClientContext, FALSE, TRUE);
  //////////////////////////
  if (mEnv.mMessgeid[0])
    sprintf(pContext->mMessages, "\"%s\"", mEnv.mMessgeid); // Message-ID
  else
    strcpy(pContext->mMessages, "NIL");    // Message-ID NULL
  put_reply(lpClientContext, FALSE, TRUE);
  //////////////////////////
  strcpy(pContext->mMessages, ")");
  put_reply(lpClientContext, FALSE, TRUE);
}

char *StrNext(char *pSrc) {
   char *pNext;
   BOOL bName = FALSE;
   int i = 0;

	pNext = NULL;
	while (*(pSrc+i)) {
#ifdef UPDATE_20150708A // FETCH n ENVELOPEでメールアドレス内に不正な文字がある対策
	  if (*(pSrc+i) == '"' || (!bName && (*(pSrc+i) == '\x1b' || *(pSrc+i) == '<') )) 
#else
	  if (*(pSrc+i) == '"' || (!bName && *(pSrc+i) == '\x1b')) 
#endif
	  {
	    if (bName) // 名前の終わり
		  bName = FALSE;
	    else       // 名前の始まり
		  bName = TRUE;
	  } else {     
		if (!bName) {
		  if (*(pSrc+i) == ',' ||
			  *(pSrc+i) == ';' )
		  { // 次のアドレスの区切り
		    pNext = (pSrc+i);
  	        *pNext++ = '\x0';
		    break;
		  } else if (*(pSrc+i) == '\x0') {
		    pNext = (pSrc+i);
		    break;
		  }
		}
	  }
	  i++;
	}
	return pNext;
}

void make_message(PCLIENT_CONTEXT lpClientContext, char *pMess, char *pEnv) {
  BOOL bMulti = FALSE;
  char *pNext = NULL;

  if (*pEnv) {
    strcpy(pMess, "("); //Envelope Start
    put_reply(lpClientContext, FALSE, TRUE);
	do {
	  pNext = StrNext(pEnv);
	  if (pNext)
		bMulti = TRUE;
      make_recode(pMess, pEnv);
      put_reply(lpClientContext, FALSE, TRUE);
	  if (bMulti) {
		bMulti = FALSE;
	    if (!*pNext)
		  break;
	    pEnv = pNext; // 次のアドレス
		if (*pEnv) {
          strcpy(pMess, " "); //Envelope Start
          put_reply(lpClientContext, FALSE, TRUE);
		}
	  }
	} while (pNext);
    strcpy(pMess, ") "); //Envelope End
  } else {
    strcpy(pMess, "NIL "); //Envelope NULL End
  }
  put_reply(lpClientContext, FALSE, TRUE);
}

void make_recode(char *pMess, char *pEnv) {
  char  *pMB, *pDom;
  BOOL  bName = TRUE;
#ifdef UPDATE_20230801 // FETCH ENVELOPEでファイル読み込みでメモリのオーバーロードが発生してしまう。
  char  mName[0x20000];
#else
  char  mName[256];
#endif
#ifdef UPDATE_20230613 // FETCH ENVELOPEのFETCH命令でTO:CC:ヘッダに複数のアドレスが記載されている場合正しく応答されない不具合
  char *pNext;
#endif

  *pMess = '\x0';
#ifdef UPDATE_20230613 // FETCH ENVELOPEのFETCH命令でTO:CC:ヘッダに複数のアドレスが記載されている場合正しく応答されない不具合
   pNext = pEnv;
   while (*pEnv)
   {
     if (pNext=strpbrk(pEnv, ";,"))
	 {
       *pNext = '\x0';
	   pNext++;
	 }
#endif
  pMB = pDom = NULL; // クリア

  while (*pEnv  == '\t' || *pEnv == ' ') // タブか空白は読み飛ばし
	pEnv++;
  pMB = strpbrk(pEnv, "<");
  if (pMB) {
    *pMB = '\x0';
    pMB++;
    strtok(pMB, ">");
  } else {
    bName = FALSE;
    pMB = strpbrk(pEnv, "\"");
    if (pMB) {
      *pMB = '\x0';
      pMB++;
      strtok(pMB, "\"");
	} else {
      if (strpbrk(pEnv, "("))
	    bName = TRUE;
      else
		pMB = pEnv;
	}
  }
  if (pMB) {
    pDom = strstr(pMB, "@");
    if (pDom) {
      *pDom = '\x0';
      pDom++;
	}
  }
  strcat(pMess, "(");
  if (pEnv && bName) {     // 名前
    if (*pEnv) {
      if (*pEnv == '"' || *pEnv == '(') {
        pEnv++;
	  }
	  strtok(pEnv, "\")");
	  ////////////////////////////////////////
	  if (strlen(pEnv) > sizeof(mName)) // バッファフローしないような処置
		pEnv[sizeof(mName)-1] = '\x0';
	  if (strstr(pEnv, "\"")) // トークンに'"'ダブルクォーテーションがある場合の処置
        sprintf(mName, "{%d}\r\n%s ", strlen(pEnv), pEnv);
	  else
        sprintf(mName, "\"%s\" ", pEnv);
      strcat(pMess, mName);
	  ////////////////////////////////////////
	} else {
     strcat(pMess, "NIL ");
	}
  } else {
	strcat(pMess, "NIL ");
  }
  strcat(pMess, "NIL "); // ソースルート
  if (pMB) {            // メールボックス(ID)
    strcat(pMess, "\"");
    strcat(pMess, pMB);
    strcat(pMess, "\" ");
  } else {
    strcat(pMess, "NIL ");
  }
  if (pDom) {            // ドメイン
    strcat(pMess, "\"");
    strcat(pMess, pDom);
    strcat(pMess, "\")");
  } else {
    strcat(pMess, "NIL)");
  }
#ifdef UPDATE_20230613 // FETCH ENVELOPEのFETCH命令でTO:CC:ヘッダに複数のアドレスが記載されている場合正しく応答されない不具合
     if (pNext)
       pEnv=pNext;
	 else
	   break;
   }
#endif

}

#ifdef UPDATE_20231121 // HEADER.FIELDS命令の応答で範囲指定が指定できない不具合。
void fetch_rfc822_header(PCLIENT_CONTEXT lpClientContext, int nFlag, char **pHeader, char *pRange) 
#else
void fetch_rfc822_header(PCLIENT_CONTEXT lpClientContext, int nFlag, char **pHeader) 
#endif
{
  PImap4Context    pContext = &lpClientContext->Context;
  char             *p, *ph, mFn[MAX_PATH], mLine[1024];
  FILE             *fp;
  DWORD            i = 0, nHeader = 0, nH;
  ULARGE_INTEGER   uSize;
  BOOL             bMach;
#ifdef UPDATE_20231121 // HEADER.FIELDS命令の応答で範囲指定が指定できない不具合。
  DWORD            nCount = 0, nStart[2] = {0, 0}, nTotal[2] = {0, 0};
#endif

#ifdef UPDATE_20231121 // HEADER.FIELDS命令の応答で範囲指定が指定できない不具合。
  if (pRange) {
	pRange++;
	nStart[0] = nStart[1] = atol(pRange);   // 開始バイト位置
	p = strchr(pRange, '.');
	if (p) {                 // 送信サイズ合計
	  p++;
	  nTotal[0] = nTotal[1] = atol(p);
	}
	pRange--; // 元に戻す。
  }
#endif
  uSize.QuadPart = 0;
  sprintf(mFn, "%s/%s", pContext->mSelectDir, (pContext->bEncMSG ? pContext->mDecFileName : pContext->FindData.cFileName));
#ifdef UPDATE_20230706 // FETCH時のメールデータを事前に一括して読込み検索するフラグを追加
  fp = NULL;
  if (pContext->pBuff) // ファイルの先頭にポインタリセット
    pContext->pfBuff = pContext->pBuff;
  if (pContext->pBuff || !pContext->pBuff && (fp = fopen(mFn, "rb"))) 
#else
  if ((fp = fopen(mFn, "rb"))) 
#endif
  {
	for (i = 0; i <= 1; i++) {
#ifdef UPDATE_20230706 // FETCH時のメールデータを事前に一括して読込み検索するフラグを追加
	  if (pContext->pfBufp)
	    p = mgets(mLine, sizeof(mLine)-1, &pContext->pfBuff);
	  else
#endif
      p = fgets(mLine, sizeof(mLine), fp);
#ifdef UPDATE_20230706 // FETCH時のメールデータを事前に一括して読込み検索するフラグを追加
	  while (p || (pContext->pfBuff ? *pContext->pfBuff : !feof(fp)))
#else
      while (p || !feof(fp))
#endif
	  {
		if (nFlag >= 1 && strcmp(mLine, "\r\n") != 0) {  // 指定したヘッダ条件
          nHeader = 0;
		  while(pHeader[nHeader]) {
            ph = pHeader[nHeader];
			while (*ph == '"')
			 ph++;
			nH = 0;
			while(*(ph+nH) != '\x0' && *(ph+nH) != '"')
			  nH++;
	        //nH = strlen(ph);
			if (strnicmp(mLine, ph, nH) == 0) {   // 一致
              /*
			  nH+=2;
			  while(mLine[nH++] == ' ');
			  if (mLine[nH] == '\r' || mLine[nH] == '\n') {// ヘッダが空欄なら無効
			    strcpy(&mLine[nH-1], "<null>\r\n"); // 空欄ヘッダにはNULLトークンを追加
			  }
			  */
			  bMach = TRUE;
			  break;
			} else {
		      if (!(mLine[0] == '\t' || mLine[0] == ' ')) // 不一致
			    bMach = FALSE;
			}
            nHeader++;
		  }
		  if (nFlag == 1 && !bMach ||
			  nFlag == 2 &&  bMach ) {
#ifdef UPDATE_20230706 // FETCH時のメールデータを事前に一括して読込み検索するフラグを追加
	        if (pContext->pfBufp)
	          p = mgets(mLine, sizeof(mLine)-1, &pContext->pfBuff);
	        else
#endif
			p = fgets(mLine, sizeof(mLine), fp);
			continue;
		  }
		}
		if (i == 0) {
#ifdef UPDATE_20231121 // HEADER.FIELDS命令の応答で範囲指定が指定できない不具合。
		  if (!pRange)
		  {
			uSize.QuadPart += strlen(mLine);
		  } else  {
			if (nStart[i] >= strlen(mLine))
			{
			  nStart[i] -= strlen(mLine);
			} else {
			  if (strlen(&mLine[nStart[i]]) > nTotal[i])
			  {
			    mLine[nStart[i]+nTotal[i]]= '\x0';
			  }
              uSize.QuadPart += strlen(&mLine[nStart[i]]);
			  if (nTotal[i] > strlen(&mLine[nStart[i]]))
			    nTotal[i] -= strlen(&mLine[nStart[i]]);
              else
			    nTotal[i] = 0;
			  nStart[i] = 0;
			}
		  }
#else
          uSize.QuadPart += strlen(mLine);
#endif
		} else {
#ifdef UPDATE_20231121 // HEADER.FIELDS命令の応答で範囲指定が指定できない不具合。
		  if (!pRange)
		  {
            strcpy(pContext->mMessages, mLine);
		  } else {
		    if (nStart[i] >= strlen(mLine))
			{
			  nStart[i] -= strlen(mLine);
			} else {
			  if (strlen(&mLine[nStart[i]]) > nTotal[i])
			  {
			    mLine[nStart[i]+nTotal[i]]= '\x0';
			  }
			  strcpy(pContext->mMessages, &mLine[nStart[i]]);
			  if (nTotal[i] >  strlen(&mLine[nStart[i]]))
			    nTotal[i] -= strlen(&mLine[nStart[i]]);
              else
			    nTotal[i] = 0;
		      nStart[i] = 0;
			}
		  }
#else
          strcpy(pContext->mMessages, mLine);
#endif
          put_reply(lpClientContext, (!strcmp(mLine, "\r\n") ? TRUE : FALSE), TRUE);
		}
        if (strcmp(mLine, "\r\n") == 0) {
		  break;
		}
#ifdef UPDATE_20230706 // FETCH時のメールデータを事前に一括して読込み検索するフラグを追加
	    if (pContext->pfBufp)
	      p = mgets(mLine, sizeof(mLine)-1, &pContext->pfBuff);
	    else
#endif
        p = fgets(mLine, sizeof(mLine), fp);
	  }
	  //// ヘッダの終了ならば..
	  if (i == 0) {
		sprintf(pContext->mMessages, "%u}\r\n", (uSize.QuadPart == 0 ? 2: uSize.QuadPart));
        put_reply(lpClientContext, FALSE, TRUE); //TRUE, TRUE);
	  } else {
		//if (uSize.QuadPart == 0) {
	    //  strcpy(pContext->mMessages, "\r\n");
        //  put_reply(lpClientContext, TRUE, TRUE);
		//}
	  }
	  if (i == 0)
#ifdef UPDATE_20230706 // FETCH時のメールデータを事前に一括して読込み検索するフラグを追加
		if (pContext->pfBufp)
		  pContext->pfBuff = pContext->pfBufp;
        else
#endif
	    fseek(fp, 0L, SEEK_SET);
	}
#ifdef UPDATE_20230706 // FETCH時のメールデータを事前に一括して読込み検索するフラグを追加
    if (!pContext->pBuff && fp)
	  fclose(fp);
#else
	fclose(fp);
#endif
  }
}

///////////////////////////////////////
// 深い構造の情報を読み出す場合コールする
// nType 0:全て/1:ボディのみ/2:ヘッダのみ
// nPart 0:全て/n1目パートのボディ/ TEXT / MIME
///////////////////////////////////////
void multipart_fetch_rfc822(FILE *fp, DWORD *pnPart, DWORD nNow, DWORD nDepath, DWORD nBase, CHAR *pupsideBound, PCLIENT_CONTEXT lpClientContext, int nType, char *pPart, char *pRange) { 
  PImap4Context    pContext = &lpClientContext->Context;
  char             *p, *pCT,mLine[1024], mLine2[1024], mBound[256], mBound1[256], mupsideBound1[256];
  DWORD            i = 0, nSend = 0, nStart = 0, nTotal = 0, n;
  ULARGE_INTEGER   uSize;
  DWORD            nLen = 0, nP = 0; //nNow;
  BOOL             bHead = TRUE, bCT = FALSE, bBD = FALSE, bTEXT = FALSE;
#ifdef UPDATE_20110126 // １カラム目のピリオドは出力しない対策
  BOOL             bBodyEnd = FALSE;
#endif
#ifdef UPDATE_20180109 // BODY.PEEK[1.1] の応答が出来ない
  BOOL             bSep = FALSE;
#endif
#ifdef UPDATE_20231114 // Content-Type:ヘッダに"message/rfc822"が指定されると構造解析が失敗する不具合。
  BOOL      bCTRFC822 = FALSE;
  char      *pCTRFC822;
#endif
#ifdef UPDATE_20231119A // MIMEパートに対する、"HEADER.FIELDS""命令がが失敗する不具合。
  char      *pReq, *pReq1, *pReq2, *pReq3;
  char      cReq;
  BOOL      bReq, bHit, bMach;
  int       nReqLen;
#endif
#ifdef UPDATE_20231122 // HEADER.FIELDS命令の応答で範囲指定が正しくない不具合。
  BOOL      bHfields = FALSE;
#endif
#ifdef UPDATE_20240121 // FETCH BODY[x.x]<n.x> のの応答で範囲指定が正しくない不具合。
  BOOL      bBfields = FALSE;
#endif

  if (nDepath > 9)  // １０階層までで中止
	return;
  // メッセージかヘッダかの判断を入れる
  //  bTEXT = TRUE;
#ifdef UPDATE_20231119A // MIMEパートに対する、"HEADER.FIELDS""命令がが失敗する不具合。
  bReq = FALSE; // HEADER,MIMEのみ
  pReq = pPart; //p;
  pReq1 = pReq2 = NULL;
  if (pReq)
  {
    pReq1 = pReq;
    for (i = 0; i <= nDepath; i++)
	{
	  if (pReq2 = strchr(pReq1, '.'))
	    pReq1 = pReq2+1;
	  else
		pReq1 += strlen(pReq1);
	} 
	pReq = pReq1;
    pReq1 = pReq2 = NULL;
    if (!strnicmp(pReq, "HEADER.FIELDS", 13))
    {
#ifdef UPDATE_20231122 // HEADER.FIELDS命令の応答で範囲指定が正しくない不具合。
      bHfields = TRUE;
#endif
      if (strnicmp(pReq+14, "NOT", 3))
	    bReq = TRUE;
#ifdef UPDATE_20231122A // HEADER.FIELDS命令の項目指定が無いとハングする不具合。
	  if (pReq1 = strchr(pReq, '('))
#else
	  pReq1 = strchr(pReq, '(');
#endif
	  while(*pReq1 == '(' || *pReq1 == ' ' || *pReq1 == '\t') // 先頭トリム
	    pReq1++;
#ifdef UPDATE_20240121 // FETCH BODY[x.x]<n.x> のの応答で範囲指定が正しくない不具合。
	} else {
      bBfields = TRUE;
#endif
	}
  }
#endif
  if (pRange) {
	pRange++;
	nStart = atol(pRange);   // 開始バイト位置
	p = strchr(pRange, '.');
	if (p) {                 // 送信サイズ合計
	  p++;
	  nTotal = atol(p);
	}
	pRange--; // 元に戻す。
  }
  uSize.QuadPart = 0;
  mBound1[0] = mBound[0] = '\x0';
  sprintf(mupsideBound1, "%s--", pupsideBound);
  {
	//for (i = 0; i <= 1; i++) { // １回目サイズ計算、２回目にデータ出力
    for (i = 0; i <= 2; i++) { // １回目サイズ計算、２回目にデータ出力
#ifdef UPDATE_20240121 // FETCH BODY[x.x]<n.x> のの応答で範囲指定が正しくない不具合。
      bBfields = FALSE;
#endif
#ifdef UPDATE_20231119A // MIMEパートに対する、"HEADER.FIELDS""命令がが失敗する不具合。
      bHit = TRUE;
#endif
#ifdef UPDATE_20180109 // BODY.PEEK[1.1] の応答が出来ない
      bSep = FALSE;
#endif
#ifdef UPDATE_20110126 // １カラム目のピリオドは出力しない対策
      bBodyEnd = FALSE;
#endif
#ifdef UPDATE_20230706 // FETCH時のメールデータを事前に一括して読込み検索するフラグを追加
	  if (pContext->pfBufp)
	    p = mgets(mLine, sizeof(mLine)-1, &pContext->pfBuff);
	  else
#endif
      p = fgets(mLine, sizeof(mLine), fp);
      while(1) { //p || !feof(fp)) {
#ifdef UPDATE_20180109 // BODY.PEEK[1.1] の応答が出来ない
		if (bSep && (mLine[0] == '\r' || mLine[0] == '\n'))
		{
          bBodyEnd = TRUE;
		  nP++;
		}
#endif
#ifdef UPDATE_20110126 // １カラム目のピリオドは出力しない対策
	    if (mLine[0] == '.' && !bHead)
		{
		  if (!strcmp(mLine, ".\r\n"))
		  {
            bBodyEnd = TRUE;
		  }
	      memmove(mLine, &mLine[1], strlen(mLine));
		}
#endif
		nLen += strlen(mLine);
		strcpy(mLine2, mLine);
		strtok(mLine2, "\r\n");
		if (bHead && bCT && !bBD) { // Boundry検出
		  pCT = SKIP_SP_TAB(mLine);
#ifdef UPDATE_20150623B // BODY[n.MIME]で指定した情報が出力されない不具合。
		  if (!strnicmp(pCT, "boundary", 8) && 
			  (*(pCT+8) == '=' || *(pCT+8) == ' ' || *(pCT+8) == '\t') )
#else
		  if (!strnicmp(pCT, "boundary=", 9)) 
#endif
		  {
			bBD = TRUE;
			pCT+=9;
#ifdef UPDATE_20150623B // BODY[n.MIME]で指定した情報が出力されない不具合。
			while(*pCT == ' ' || *pCT == '\t' || *pCT == '=')
			{
			  pCT++;
			}
#endif
			if (*pCT == '"')
			  pCT++;
			sprintf(mBound, "--%s", pCT);
#ifdef UPDATE_20150630A // FETCH N BODY[1]でboundary=がダブルクォテーションで囲まれていないと正しく読み込めない不具合
			strtok(mBound, "\"\r\n");
#else
			strtok(mBound, "\"");
#endif
		    sprintf(mBound1, "%s--", mBound);
		  }
		}
		if (bHead && !bCT && !bBD && !strnicmp(mLine, "Content-Type:", 13) ) { // Boundry検出
#ifdef UPDATE_20231114 // Content-Type:ヘッダに"message/rfc822"が指定されると構造解析が失敗する不具合。
		  if (!bCTRFC822)
		  { 
		    pCTRFC822 = (bCT ? mLine : &mLine[13]);
		    while(*pCTRFC822 == ' ' || *pCTRFC822 == '\t')
			{
			  pCTRFC822++;
			}
		    bCTRFC822 = !strnicmp(pCTRFC822, "message/rfc822", 14);
		  }
#endif
#ifdef UPDATE_20231120A // "Conten-Type: message/rfc822"の子パートの取得に失敗する不具合。
		  if (!bCTRFC822)
            bCT = TRUE;
#else
          bCT = TRUE;
#endif
		  pCT = strstr(&mLine[13], ";");
		  if (pCT) {
#ifdef UPDATE_20150706 // Content-Type: 行に boundaryが含まれていると入れ子のbody[n.n]等が正しく抽出できない
			do 
			{
#endif
			  pCT++;
		      pCT = SKIP_SP_TAB(pCT);
#ifdef UPDATE_20150623B // BODY[n.MIME]で指定した情報が出力されない不具合。
		      if (!strnicmp(pCT, "boundary", 8) && 
			    (*(pCT+8) == '=' || *(pCT+8) == ' ' || *(pCT+8) == '\t') )
#else
		      if (!strnicmp(pCT, "boundary=", 9))
#endif
			  {
 			    bBD = TRUE;
			    pCT+=9;
#ifdef UPDATE_20150623B // BODY[n.MIME]で指定した情報が出力されない不具合。
			    while(*pCT == ' ' || *pCT == '\t' || *pCT == '=')
				{
			      pCT++;
				}
#endif
			    if (*pCT == '"')
			      pCT++;
			    sprintf(mBound, "--%s", pCT);
#ifdef UPDATE_20150630A // FETCH N BODY[1]でboundary=がダブルクォテーションで囲まれていないと正しく読み込めない不具合
			    strtok(mBound, "\"\r\n");
#else
			    strtok(mBound, "\"");
#endif
		        sprintf(mBound1, "%s--", mBound);
#ifdef UPDATE_20150706 // Content-Type: 行に boundaryが含まれていると入れ子のbody[n.n]等が正しく抽出できない
				break;
#endif
			  }
#ifdef UPDATE_20150706 // Content-Type: 行に boundaryが含まれていると入れ子のbody[n.n]等が正しく抽出できない
			} while (pCT = strstr(pCT, ";"));
#endif
		  }
		}
#ifdef UPDATE_20231114 // Content-Type:ヘッダに"message/rfc822"が指定されると構造解析が失敗する不具合。
		else if (bCTRFC822 && (mLine[0] == '\r' || mLine[0] == '\n') )
		{ 
#ifdef UPDATE_20230706 // FETCH時のメールデータを事前に一括して読込み検索するフラグを追加
	      if (pContext->pfBufp)
	        p = mgets(mLine, sizeof(mLine)-1, &pContext->pfBuff);
	      else
#endif
		  p = fgets(mLine, sizeof(mLine), fp);
		  bCTRFC822 = FALSE;
		  continue;
		}
#endif
#ifdef UPDATE_20180109 // BODY.PEEK[1.1] の応答が出来ない
		if (nType <= 1 && (bHead || ((int)*(pnPart+nDepath) > nP)) && strcmp(mLine, "\r\n") == 0) 
#else
		if (nType <= 1 && bHead && strcmp(mLine, "\r\n") == 0) 
#endif
		{ // ヘッダの終わり
		  bHead = FALSE;
		  if (nType == 1) {
	        if (i >= 1 && pRange && ((int)*(pnPart+nDepath) == nP))
#ifdef UPDATE_20240121 // FETCH BODY[x.x]<n.x> のの応答で範囲指定が正しくない不具合。
              if (!bHfields && !bBfields)
#endif
#ifdef UPDATE_20230706 // FETCH時のメールデータを事前に一括して読込み検索するフラグを追加
		      if (pContext->pfBufp)
#ifdef UPDATE_20231122 // HEADER.FIELDS命令の応答で範囲指定が正しくない不具合。
			  pContext->pfBuff += (long)(!bHfields ? nStart : 0);
#else
			  pContext->pfBuff += (long)nStart;
#endif
			  else
#endif
#ifdef UPDATE_20231122 // HEADER.FIELDS命令の応答で範囲指定が正しくない不具合。
	          fseek(fp, (!bHfields ? nStart : 0), SEEK_CUR); // 現在位置から開始バイト位置指定
#else
	          fseek(fp, nStart, SEEK_CUR); // 現在位置から開始バイト位置指定
#endif
		    if (!mBound[0])  // マルチパートでないなら
			{
			  nP++;
#ifdef UPDATE_20180109 // BODY.PEEK[1.1] の応答が出来ない
              if (((int)*(pnPart+nDepath) == nP))
			  {
				 bSep = TRUE;
			  }
              if (((int)*(pnPart+nDepath) >= nP))
			  {
#ifdef UPDATE_20230706 // FETCH時のメールデータを事前に一括して読込み検索するフラグを追加
	             if (pContext->pfBufp)
	               p = mgets(mLine, sizeof(mLine)-1, &pContext->pfBuff);
	             else
#endif
		         p = fgets(mLine, sizeof(mLine), fp);
		         continue;
			  } 
#endif
#ifdef UPDATE_20150702A // BODY[n.n]のように深い階層にデータがないと上位層のデータを応答してしまう不具合。
               uSize.QuadPart = 0;
			   break;
#endif
			}
#ifdef UPDATE_20230706 // FETCH時のメールデータを事前に一括して読込み検索するフラグを追加
	        if (pContext->pfBufp)
	          p = mgets(mLine, sizeof(mLine)-1, &pContext->pfBuff);
	        else
#endif
		    p = fgets(mLine, sizeof(mLine), fp);
		    continue;
		  }
		}
		if (nType > 0 && !bHead && !bTEXT && 
			(
			 ( mBound[0] && (!strcmp(mLine2, mBound) || !strcmp(mLine2, mBound1))) ||
			 (!mBound[0] && (!strcmp(mLine2, pupsideBound) || !strcmp(mLine2, mupsideBound1)))
			 )
			) { // パート区切り
          nP++;
		  bHead = TRUE;
		  if (i == 0 && nP == *(pnPart+nNow) && *(pnPart+nNow+1)) { // 深い構造を調査
		    multipart_fetch_rfc822(fp, pnPart, nNow+1, nDepath, nBase+nLen, mBound, lpClientContext, nType, pPart, pRange);
			return;
		  }
          if (i >= 1 && pRange && ((int)*(pnPart+nDepath) == nP))
#ifdef UPDATE_20240121 // FETCH BODY[x.x]<n.x> のの応答で範囲指定が正しくない不具合。
//printf("1:nType = %d /bBfields=%d /bHfields=%d / bHead=%d\nmLine=%s\n", nType, bBfields, bHfields, bHead, mLine);
            if (nType == 2 || nType == 1 && bHfields && bHead)
#endif
#ifdef UPDATE_20230706 // FETCH時のメールデータを事前に一括して読込み検索するフラグを追加
		    if (pContext->pfBufp)
#ifdef UPDATE_20231122 // HEADER.FIELDS命令の応答で範囲指定が正しくない不具合。
			   pContext->pfBuff += (long)(!bHfields ? nStart : 0);
#else
			   pContext->pfBuff += (long)nStart;
#endif
			else
#endif
#ifdef UPDATE_20231122 // HEADER.FIELDS命令の応答で範囲指定が正しくない不具合。
	        fseek(fp, (!bHfields ? nStart : 0), SEEK_CUR); // 現在位置から開始バイト位置指定
#else
	        fseek(fp, nStart, SEEK_CUR); // 現在位置から開始バイト位置指定
#endif
		} else {
/*
#ifdef UPDATE_20231119 // Content-Type:ヘッダに"message/rfc822"が指定されその内容がマルチ―パートの構造の場合に解析が失敗する不具合。
printf("nType=%d\n", nType);
printf("bHead=%d\n", bHead);
printf("bTEXT=%d\n", bTEXT);
printf("nP=%d\n", nP);
printf("*(pnPart+nNow)=%d\n", *(pnPart+nNow));
printf("*(pnPart+nDepath)=%d\n", *(pnPart+nDepath));
printf("nDepath=%d\n", nDepath);
printf("pupsideBound=%s\n", pupsideBound);
printf("mBound=%s\n", mBound);
printf("mupsideBound1=%s\n", mupsideBound1);
printf("mLine2=%s\n", mLine2);
if (strstr(mLine2, "Content-Type: message/DELIVERY-STATUS"))
  printf("hit\n");
#endif
*/
//if (strstr(mLine2, "Content-Type: text/plain; charset=UTF-8; 3.3.3.1"))
//  printf("hit\n");

          if ((!bHead && nType == 2 && (int)*(pnPart+nDepath) == 0) ||
#ifdef UPDATE_20110126 // １カラム目のピリオドは出力しない対策
             bBodyEnd ||
#else
             strcmp(mLine, ".\r\n") == 0 ||
#endif
#ifdef UPDATE_20231120 // multipart/alternativeパートの2パートの取得に失敗する不具合。
			 ((int)*(pnPart+nDepath) > 0 && (int)*(pnPart+nDepath) < nP && nDepath == nNow) ||
#else
#ifdef UPDATE_20231119 // Content-Type:ヘッダに"message/rfc822"が指定されその内容がマルチ―パートの構造の場合に解析が失敗する不具合。
			 ((int)*(pnPart+nDepath) > 0 && *(pnPart+(nDepath==0 ? nDepath:nDepath-1)) < nP) ||
#else
			 ((int)*(pnPart+nDepath) > 0 && (int)*(pnPart+nDepath) < nP) ||
#endif
#endif
			 !p ||
#ifdef UPDATE_20230706 // FETCH時のメールデータを事前に一括して読込み検索するフラグを追加
	          (pContext->pfBuff ? !(*pContext->pfBuff) : feof(fp)))
#else
			 feof(fp)) 
#endif
			 {
		    //if (i == 0) {
			if (i == 1) {
	          sprintf(pContext->mMessages, "%u}\r\n", (uSize.QuadPart == 0 ? 2: uSize.QuadPart));
              put_reply(lpClientContext, TRUE, TRUE);
			} 
	        break;
		  }
		  if (nType == 0 ||
			 (nType == 1 && !bHead  && ((int)*(pnPart+nDepath) == 0 || (int)*(pnPart+nDepath) > 0 && (int)*(pnPart+nDepath) == nP)) ||// 指定されたパートのみ処理
			 (nType == 2 && bHead && ((int)*(pnPart+nDepath) == 0 || (int)*(pnPart+nDepath) > 0 && (int)*(pnPart+nDepath) == nP))) 
		  {
#ifdef UPDATE_20231119A // MIMEパートに対する、"HEADER.FIELDS""命令がが失敗する不具合。
			if (nType == 2 && bHead && ((int)*(pnPart+nDepath) == 0 || (int)*(pnPart+nDepath) > 0 && (int)*(pnPart+nDepath) == nP))
			{
			  if (mLine[0] != ' ' && mLine[0] != '\t' && mLine[0] != '\r' && mLine[0] != '\n') // ヘッダの継続行であるかの判定
			  {
			    bHit = (bReq ? FALSE : TRUE); //NOT有無
			    if (pReq1)
			    {
				  bMach = FALSE;
			      pReq3 = pReq1;
                  do 
			      {
				    if (pReq2 = strpbrk(pReq3, " )"))
				    {
			          cReq = *pReq2;
				      *pReq2 = '\x0';
					}
					nReqLen = strlen(pReq3);
				    if (!strnicmp(mLine, pReq3, strlen(pReq3)))
			        {
				      if (mLine[strlen(pReq3)] == ':')
					  {
						bMach = TRUE;
			            bHit = (bReq ? TRUE : FALSE); //NOT有無
						if (pReq2)
						  *pReq2 = cReq;
					    break;
				      }
				    }
					if (pReq2)
				      *pReq2 = cReq;
					if (bMach)
					  break;
				      //pReq3 = pReq2+1;
					pReq3 += nReqLen;
					while(*pReq3 == ' ' || *pReq3 == '\t' || *pReq3 == ')')
					  pReq3++;
				  } while(*pReq3);
			    } else {
			      bHit = (bReq ? FALSE : TRUE); //NOT有無;
			    }
			  }
			} else {
			  bHit = TRUE;
			} 
			if (bHit)
			{
#endif
		      if (i == 0) {
                uSize.QuadPart += strlen(mLine); // 該当パートのサイズ計算
			  } else if (i == 1) {
#ifdef UPDATE_20240121 // FETCH BODY[x.x]<n.x> のの応答で範囲指定が正しくない不具合。
			    if (pRange && (nTotal <= uSize.QuadPart + strlen(mLine))) 
#else
			    if (pRange && (nTotal < uSize.QuadPart + strlen(mLine))) 
#endif
				{
			      uSize.QuadPart = nTotal;
	              sprintf(pContext->mMessages, "%u}\r\n", (uSize.QuadPart == 0 ? 2: uSize.QuadPart));
                  put_reply(lpClientContext, TRUE, TRUE);
			      break;
			    } else {
                  uSize.QuadPart += strlen(mLine);
			    }
			  } else {
			    if (nSend + strlen(mLine) < FETCH_BODY_BUFFER) {
			      nSend += strlen(mLine);
                  strcat(pContext->mMessages, mLine);
			    } else {
			      pContext->mMessages[FETCH_BODY_BUFFER] = '\x0';
			      n = strlen(pContext->mMessages);
			      if (pRange && (nTotal < n)) {
			        pContext->mMessages[nTotal] = '\x0'; // サイズを調整
			        n = strlen(pContext->mMessages);
				  }
#ifdef UPDATE_20231209 // FETCH命令中に通信切断された場合に大きなサイズのデータ送信の中断を行えるようにした。
                  if (!put_reply(lpClientContext, TRUE, FALSE) && bSendErrEscape) { //FALSE); //bSendErrEscape; //送信エラー時中断 1:する(デフォルト) 0:しない
					break;
				  }
#else
                  put_reply(lpClientContext, TRUE, FALSE); //FALSE);
#endif
				  if (pRange) {
				    nTotal -= n;
				    if (nTotal == 0) {
                      nSend = 0;
				      break;
				    }
				  }
				  nSend = strlen(mLine);
                  strcpy(pContext->mMessages, mLine);
			    }
			  }
#ifdef UPDATE_20231119A // MIMEパートに対する、"HEADER.FIELDS""命令がが失敗する不具合。
		    }
#endif
	      }
		}
		if (nType == 2 && bHead && strcmp(mLine, "\r\n") == 0) // ヘッダの終わり
		{
		  bHead = FALSE;
#ifdef UPDATE_20150702A // BODY[n.n]のように深い階層にデータがないと上位層のデータを応答してしまう不具合。
	      if (!mBound[0])
		  {
            uSize.QuadPart = 0;
			break;
		  }
#endif
		}
#ifdef UPDATE_20230706 // FETCH時のメールデータを事前に一括して読込み検索するフラグを追加
	    if (pContext->pfBufp)
	      p = mgets(mLine, sizeof(mLine)-1, &pContext->pfBuff);
	    else
#endif
        p = fgets(mLine, sizeof(mLine), fp);
	  }
	  //if (i == 0) {
	  if (i <= 1) {

#ifdef UPDATE_20150702A // BODY[n.n]のように深い階層にデータがないと上位層のデータを応答してしまう不具合。
#ifdef UPDATE_20240121 // FETCH BODY[x.x]<n.x> のの応答で範囲指定が正しくない不具合。
	    if (uSize.QuadPart == 0 ||
			bHfields && i == 0 && nStart > uSize.QuadPart)  // 開始サイズオーバー
#else
        if (uSize.QuadPart == 0 ||
			i == 0 && nStart > uSize.QuadPart)  // 開始サイズオーバー
#endif
#else
		if (i == 0 && nStart > uSize.QuadPart)  // 開始サイズオーバー
#endif
		{
#ifdef UPDATE_20231205 // FETCH命令で取得データが空白の場合、0バイトデータとして応答する。
          sprintf(pContext->mMessages, "0}\r\n");
#else
          sprintf(pContext->mMessages, "2}\r\n");
#endif
          put_reply(lpClientContext, TRUE, TRUE);
		  break;
		} else {
		  *pContext->mMessages = '\x0';
          bHead = TRUE;
		  nP = 0;
#ifdef UPDATE_20230706 // FETCH時のメールデータを事前に一括して読込み検索するフラグを追加
	      if (pContext->pfBufp)
		  {
#ifdef UPDATE_20231122 // HEADER.FIELDS命令の応答で範囲指定が正しくない不具合。
	        if ((nType == 0 || nType== 2) && pRange && !bHfields)
#else
	        if ((nType == 0 || nType== 2) && pRange)
#endif
	          pContext->pfBuff = pContext->pfBufp + (LONG)nBase + nStart; // 開始バイト位置指定
		    else
              pContext->pfBuff = pContext->pfBufp + (LONG)nBase;
		  } else {
#endif
#ifdef UPDATE_20231122 // HEADER.FIELDS命令の応答で範囲指定が正しくない不具合。
	        if ((nType == 0 || nType== 2) && pRange && !bHfields)
#else
	        if ((nType == 0 || nType== 2) && pRange)
#endif
	          fseek(fp, (LONG)nBase+nStart, SEEK_SET); // 開始バイト位置指定
		    else
              fseek(fp, (LONG)nBase, SEEK_SET);
#ifdef UPDATE_20230706 // FETCH時のメールデータを事前に一括して読込み検索するフラグを追加
		  }
#endif
		}
		if (i == 0)  // トータルサイズクリア
		  uSize.QuadPart = 0;
	  }
	}
    if (nSend > 0) { // 残りを送信
	  nSend = 0;
#ifdef UPDATE_20231122 // HEADER.FIELDS命令の応答で範囲指定が正しくない不具合。
	  n = strlen(&pContext->mMessages[(bHfields ? nStart : 0)]);
#else
	  n = strlen(pContext->mMessages);
#endif
	  if (pRange && (nTotal < n)) {
#ifdef UPDATE_20231122 // HEADER.FIELDS命令の応答で範囲指定が正しくない不具合。
	    pContext->mMessages[nTotal+(bHfields ? nStart : 0)] = '\x0'; // サイズを調整
#else
	    pContext->mMessages[nTotal] = '\x0'; // サイズを調整
#endif
	    n = strlen(pContext->mMessages);
	  }
#ifdef UPDATE_20231122 // HEADER.FIELDS命令の応答で範囲指定が正しくない不具合。
	  if (bHfields)
	    memcpy(pContext->mMessages, &pContext->mMessages[nStart], nTotal);
#endif
      put_reply(lpClientContext, TRUE, FALSE); //TRUE);
	  if (pRange)
        nTotal -= n;
	}
  }
}
///////////////////////////////////////
// nType 0:全て/1:ボディのみ/2:ヘッダのみ
// nPart 0:全て/n1目パートのボディ/ TEXT / MIME
///////////////////////////////////////
//void fetch_rfc822(PCLIENT_CONTEXT lpClientContext, int nType, int nPart, char *pRange) { 
void fetch_rfc822(PCLIENT_CONTEXT lpClientContext, int nType, char *pPart, char *pRange) { 
  PImap4Context    pContext = &lpClientContext->Context;
  char             *p, *pCT, mFn[MAX_PATH], mLine[1024], mLine2[1024], mBound[256], mBound1[256];
  FILE             *fp;
  DWORD            nDepath, i = 0, nSend = 0, nStart = 0, nTotal = 0, n;
  ULARGE_INTEGER   uSize;
  DWORD            nLen = 0, nP = 0, nPart[10];
  BOOL             bHead = TRUE, bCT = FALSE, bBD = FALSE, bTEXT = FALSE;
#ifdef UPDATE_20110126 // １カラム目のピリオドは出力しない対策
  BOOL             bBodyEnd = FALSE;
#endif
#ifdef UPDATE_20231003 // データ受信時のCPU負荷を下げる対策
  DWORD     nCloolStep;
#endif
#ifdef UPDATE_20231114 // Content-Type:ヘッダに"message/rfc822"が指定されると構造解析が失敗する不具合。
  BOOL      bCTRFC822 = FALSE;
  char      *pCTRFC822;
#endif
#ifdef UPDATE_20231119A // MIMEパートに対する、"HEADER.FIELDS""命令がが失敗する不具合。
  char      *pReq, *pReq1, *pReq2, *pReq3;
  char      cReq;
  BOOL      bReq, bHit, bMach;
  int       nReqLen;
#endif
#ifdef UPDATE_20231122 // HEADER.FIELDS命令の応答で範囲指定が正しくない不具合。
  BOOL      bHfields = FALSE;
#endif
#ifdef UPDATE_20240121 // FETCH BODY[x.x]<n.x> のの応答で範囲指定が正しくない不具合。
  BOOL      bBfields = FALSE;
#endif

  for (nDepath = 0; nDepath < 10; nDepath++) // パート番号をクリア
	nPart[nDepath] = 0;
  nDepath = 0;
  if (pPart) {
	p = pPart;
	while(p && nDepath < 10)
	{
	  if (*p) {
	    if (*p >= '0' && *p <= '9' )
	      nPart[nDepath] = atoi(p); // パート番号指定
	    else {
	      nPart[nDepath] = 0; //-1; // パート名称指定("text","mime"など)
		  if (nDepath == 0) // トップレベルのヘッダー
	        bTEXT = TRUE;
		  if (nDepath > 0)
			nDepath--;
		  break;
		}
        if (!bTEXT && (p = strpbrk(p, "."))) {
 	      p++;
		  nDepath++;
		}
	  } else {
		break;
	  }
	}
  }
#ifdef UPDATE_20231119A // MIMEパートに対する、"HEADER.FIELDS""命令がが失敗する不具合。
  bReq = FALSE; // HEADER,MIMEのみ
  pReq = p;
  pReq1 = pReq2 = NULL;
  if (pReq)
  {
    if (!strnicmp(pReq, "HEADER.FIELDS", 13))
    {
#ifdef UPDATE_20231122 // HEADER.FIELDS命令の応答で範囲指定が正しくない不具合。
      bHfields = TRUE;
#endif
      if (strnicmp(pReq+14, "NOT", 3))
	    bReq = TRUE;
#ifdef UPDATE_20231122A // HEADER.FIELDS命令の項目指定が無いとハングする不具合。
	  if (pReq1 = strchr(pReq, '('))
#else
	  pReq1 = strchr(pReq, '(');
#endif
	  while(*pReq1 == '(' || *pReq1 == ' ' || *pReq1 == '\t') // 先頭トリム
	    pReq1++;
#ifdef UPDATE_20240121 // FETCH BODY[x.x]<n.x> のの応答で範囲指定が正しくない不具合。
	} else {
      bBfields = TRUE;
#endif
	}
  }
#endif

  if (pRange) {
	pRange++;
	nStart = atol(pRange);   // 開始バイト位置
	p = strchr(pRange, '.');
	if (p) {                 // 送信サイズ合計
	  p++;
	  nTotal = atol(p);
	}
	pRange--; // 元に戻す
  }
  uSize.QuadPart = 0;
  mBound1[0] = mBound[0] = '\x0';
  sprintf(mFn, "%s/%s", pContext->mSelectDir, (pContext->bEncMSG ? pContext->mDecFileName : pContext->FindData.cFileName));
#ifdef UPDATE_20230706 // FETCH時のメールデータを事前に一括して読込み検索するフラグを追加
  fp = NULL;
  if (pContext->pBuff) // ファイルの先頭にポインタリセット
    pContext->pfBuff = pContext->pBuff;
  if (pContext->pBuff || !pContext->pBuff && (fp = fopen(mFn, "rb"))) 
#else
  if ((fp = fopen(mFn, "rb")))
#endif
  {
/* 一時
	if (nType == 0 && pRange) {
	  fseek(fp, nStart, SEEK_SET); // 開始バイト位置指定
	}
*/
	//for (i = 0; i <= 1; i++) { // １回目サイズ計算、２回目にデータ出力
    for (i = 0; i <= 2; i++) { // １回目サイズ計算、２回目にデータ出力
#ifdef UPDATE_20240121 // FETCH BODY[x.x]<n.x> のの応答で範囲指定が正しくない不具合。
      bBfields = FALSE;
#endif
#ifdef UPDATE_20231119A // MIMEパートに対する、"HEADER.FIELDS""命令がが失敗する不具合。
      bHit = TRUE;
#endif
#ifdef UPDATE_20231003 // データ受信時のCPU負荷を下げる対策
	  nCloolStep = 0;
#endif
	  bBodyEnd = FALSE;
#ifdef UPDATE_20230706 // FETCH時のメールデータを事前に一括して読込み検索するフラグを追加
	  if (pContext->pfBufp)
	    p = mgets(mLine, sizeof(mLine)-1, &pContext->pfBuff);
	  else
#endif
      p = fgets(mLine, sizeof(mLine), fp);
      while(1) { //p || !feof(fp)) {
#ifdef UPDATE_20110126 // １カラム目のピリオドは出力しない対策
	    if (mLine[0] == '.' && !bHead)
		{
		  if (!strcmp(mLine, ".\r\n"))
		  {
            bBodyEnd = TRUE;
		  }
	      memmove(mLine, &mLine[1], strlen(mLine));
		}
#endif
		nLen += strlen(mLine);
		strcpy(mLine2, mLine);
		strtok(mLine2, "\r\n");
		if (bHead && bCT && !bBD) { // Boundry検出
		  pCT = SKIP_SP_TAB(mLine);
#ifdef UPDATE_20150623B // BODY[n.MIME]で指定した情報が出力されない不具合。
		  if (!strnicmp(pCT, "boundary", 8) && 
			  (*(pCT+8) == '=' || *(pCT+8) == ' ' || *(pCT+8) == '\t') )
#else
		  if (!strnicmp(pCT, "boundary=", 9))
#endif
		  {
			bBD = TRUE;
			pCT+=9;
#ifdef UPDATE_20150623B // BODY[n.MIME]で指定した情報が出力されない不具合。
			while(*pCT == ' ' || *pCT == '\t' || *pCT == '=')
			{
			  pCT++;
			}
#endif
			if (*pCT == '"')
			  pCT++;
			sprintf(mBound, "--%s", pCT);
#ifdef UPDATE_20150630A // FETCH N BODY[1]でboundary=がダブルクォテーションで囲まれていないと正しく読み込めない不具合
			strtok(mBound, "\"\r\n");
#else
			strtok(mBound, "\"");
#endif
		    sprintf(mBound1, "%s--", mBound);
		  }
		}
		if (bHead && !bCT && !bBD && !strnicmp(mLine, "Content-Type:", 13) ) { // Boundry検出
#ifdef UPDATE_20231114 // Content-Type:ヘッダに"message/rfc822"が指定されると構造解析が失敗する不具合。
		  if (!bCTRFC822)
		  { 
		    pCTRFC822 = (bCT ? mLine : &mLine[13]);
		    while(*pCTRFC822 == ' ' || *pCTRFC822 == '\t')
			{
			  pCTRFC822++;
			}
		    bCTRFC822 = !strnicmp(pCTRFC822, "message/rfc822", 14);
		  }
#endif
#ifdef UPDATE_20231120A // "Conten-Type: message/rfc822"の子パートの取得に失敗する不具合。
		  if (!bCTRFC822)
            bCT = TRUE;
#else
          bCT = TRUE;
#endif
		  pCT = strstr(&mLine[13], ";");
		  if (pCT) {
#ifdef UPDATE_20150706 // Content-Type: 行に boundaryが含まれていると入れ子のbody[n.n]等が正しく抽出できない
			do 
			{
#endif
			  pCT++;
		      pCT = SKIP_SP_TAB(pCT);
#ifdef UPDATE_20150623B // BODY[n.MIME]で指定した情報が出力されない不具合。
		      if (!strnicmp(pCT, "boundary", 8) && 
			      (*(pCT+8) == '=' || *(pCT+8) == ' ' || *(pCT+8) == '\t') )
#else
		      if (!strnicmp(pCT, "boundary=", 9))
#endif
			  {
			    bBD = TRUE;
			    pCT+=9;
#ifdef UPDATE_20150623B // BODY[n.MIME]で指定した情報が出力されない不具合。
			    while(*pCT == ' ' || *pCT == '\t' || *pCT == '=')
				{
			      pCT++;
				}
#endif
			    if (*pCT == '"')
			      pCT++;
			    sprintf(mBound, "--%s", pCT);
#ifdef UPDATE_20150630A // FETCH N BODY[1]でboundary=がダブルクォテーションで囲まれていないと正しく読み込めない不具合
			    strtok(mBound, "\"\r\n");
#else
			    strtok(mBound, "\"");
#endif
			    sprintf(mBound1, "%s--", mBound);
#ifdef UPDATE_20091129A // CatchMe@Mailで添付きメールの表示が失敗する
			    strtok(mBound, "\r\n");
			    strtok(mBound1, "\r\n");
#endif
#ifdef UPDATE_20150706 // Content-Type: 行に boundaryが含まれていると入れ子のbody[n.n]等が正しく抽出できない
				break;
#endif
			  }
#ifdef UPDATE_20150706 // Content-Type: 行に boundaryが含まれていると入れ子のbody[n.n]等が正しく抽出できない
			} while (pCT = strstr(pCT, ";"));
#endif
		  }
		}
#ifdef UPDATE_20231114 // Content-Type:ヘッダに"message/rfc822"が指定されると構造解析が失敗する不具合。
		else if (bCTRFC822 && (mLine[0] == '\r' || mLine[0] == '\n'))
		{ 
#ifdef UPDATE_20230706 // FETCH時のメールデータを事前に一括して読込み検索するフラグを追加
	      if (pContext->pfBufp)
	        p = mgets(mLine, sizeof(mLine)-1, &pContext->pfBuff);
	      else
#endif
		  p = fgets(mLine, sizeof(mLine), fp);
		  bCTRFC822 = FALSE;
		  continue;
		}
#endif
		if (nType <= 1 && bHead && strcmp(mLine, "\r\n") == 0) { // ヘッダの終わり
		  bHead = FALSE;
		  if (nType == 1) {
	        if (i >= 1 && pRange && (nPart[0] == nP))
			{
#ifdef UPDATE_20240121 // FETCH BODY[x.x]<n.x> のの応答で範囲指定が正しくない不具合。
              if (!bHfields && !bBfields)
#endif
#ifdef UPDATE_20230706 // FETCH時のメールデータを事前に一括して読込み検索するフラグを追加
		      if (pContext->pfBufp)
#ifdef UPDATE_20231122 // HEADER.FIELDS命令の応答で範囲指定が正しくない不具合。
			    pContext->pfBuff += (long)(!bHfields ? nStart : 0);
#else
				pContext->pfBuff += (long)nStart;
#endif
			  else
#endif
#ifdef UPDATE_20231122 // HEADER.FIELDS命令の応答で範囲指定が正しくない不具合。
	          fseek(fp, (!bHfields ? nStart : 0), SEEK_CUR); // 現在位置から開始バイト位置指定
#else
	          fseek(fp, nStart, SEEK_CUR); // 現在位置から開始バイト位置指定
#endif
			}
		    if (!mBound[0])  // マルチパートでないなら
			{
			  nP++;
#ifdef UPDATE_20150702A // BODY[n.n]のように深い階層にデータがないと上位層のデータを応答してしまう不具合。
			  if (nDepath > 0)
			  {
                uSize.QuadPart = 0;
			    break;
			  }
#endif
#ifdef UPDATE_20240121 // FETCH BODY[x.x]<n.x> のの応答で範囲指定が正しくない不具合。
		      if (pContext->pfBufp)
			    pContext->pfBuff += (long)(!bHfields ? nStart : 0);
			  else
	            fseek(fp, (!bHfields ? nStart : 0), SEEK_CUR); // 現在位置から開始バイト位置指定
#endif
			}
#ifdef UPDATE_20230706 // FETCH時のメールデータを事前に一括して読込み検索するフラグを追加
	        if (pContext->pfBufp)
	          p = mgets(mLine, sizeof(mLine)-1, &pContext->pfBuff);
	        else
#endif
		    p = fgets(mLine, sizeof(mLine), fp);
		    continue;
		  }
		}
#ifdef UPDATE_20091129A // CatchMe@Mailで添付きメールの表示が失敗する
		if (nType > 0 && !bHead && !bTEXT && mBound[0] &&
			(!strncmp(mLine2, mBound, strlen(mBound)) || !strncmp(mLine2, mBound1, strlen(mBound1)) )
#else
		if (nType > 0 && !bHead && !bTEXT && mBound[0] &&
			(!strcmp(mLine2, mBound) || !strcmp(mLine2, mBound1))
#endif
			)
		{ // パート区切り
          nP++;
		  bHead = TRUE;
		  if (i == 0 && nP == nPart[0] && nPart[1]) { // 深い構造を調査
		    multipart_fetch_rfc822(fp, nPart, 1, nDepath, nLen, mBound, lpClientContext, nType, pPart, pRange);
#ifdef UPDATE_20230706 // FETCH時のメールデータを事前に一括して読込み検索するフラグを追加
            if (!pContext->pBuff && fp)
	          fclose(fp);
#else
#ifdef UPDATE_20091129G // MIMEパート取得でファイルクローズ抜け CatchMe@Mailで添付きメールの表示が失敗する
			fclose(fp);
#endif
#endif
			return;
		  }
#ifdef UPDATE_20240121 // FETCH BODY[x.x]<n.x> のの応答で範囲指定が正しくない不具合。
          if (i >= 1 && pRange && (nPart[0] == nP))
#else
#ifdef UPDATE_20091129F // CatchMe@Mailで添付きメールの表示が失敗する
          if (!bHead && i >= 1 && pRange && (nPart[0] == nP))
#else
          if (i >= 1 && pRange && (nPart[0] == nP))
#endif
#endif
		  {
#ifdef UPDATE_20240121 // FETCH BODY[x.x]<n.x> のの応答で範囲指定が正しくない不具合。
//printf("2:nType = %d /bBfields=%d /bHfields=%d / bHead=%d\nmLine=%s\n", nType, bBfields, bHfields, bHead, mLine);
            if (nType == 2 || nType == 1 && bHfields && bHead)
#endif
#ifdef UPDATE_20230706 // FETCH時のメールデータを事前に一括して読込み検索するフラグを追加
		    if (pContext->pfBufp)
#ifdef UPDATE_20231122 // HEADER.FIELDS命令の応答で範囲指定が正しくない不具合。
			  pContext->pfBuff += (long)(!bHfields ? nStart : 0);
#else
		      pContext->pfBuff+=(long)nStart;
#endif
		    else
#endif
#ifdef UPDATE_20231122 // HEADER.FIELDS命令の応答で範囲指定が正しくない不具合。
			fseek(fp, (!bHfields ? nStart : 0), SEEK_CUR); // 現在位置から開始バイト位置指定
#else
			fseek(fp, nStart, SEEK_CUR); // 現在位置から開始バイト位置指定
#endif
		  }
		} else {
          if ((!bHead && nType == 2 && nPart[0] == 0) ||
#ifdef UPDATE_20110126 // １カラム目のピリオドは出力しない対策
             bBodyEnd ||
#else
             strcmp(mLine, ".\r\n") == 0 ||
#endif
			 (nPart[0] > 0 && nPart[0] < nP) ||
			 !p ||
#ifdef UPDATE_20230706 // FETCH時のメールデータを事前に一括して読込み検索するフラグを追加
	          (pContext->pfBuff ? !(*pContext->pfBuff) : feof(fp)))
#else
			 feof(fp)) 
#endif
		  {
		    //if (i == 0) {
			if (i == 1) {
	          sprintf(pContext->mMessages, "%u}\r\n", (uSize.QuadPart == 0 ? 2: uSize.QuadPart));
              put_reply(lpClientContext, TRUE, TRUE);
			} 
	        break;
		  }
		  if (nType == 0 ||
			 (nType == 1 && !bHead  && (nPart[0] == 0 || nPart[0] > 0 && nPart[0] == nP)) ||// 指定されたパートのみ処理
			 (nType == 2 && bHead && (nPart[0] == 0 || nPart[0] > 0 && nPart[0] == nP))) {
#ifdef UPDATE_20231119A // MIMEパートに対する、"HEADER.FIELDS""命令がが失敗する不具合。
			if (nType == 2 && bHead && (nPart[0] == 0 || nPart[0] > 0 && nPart[0] == nP))
			{
			  if (mLine[0] != ' ' && mLine[0] != '\t' && mLine[0] != '\r' && mLine[0] != '\n') // ヘッダの継続行であるかの判定
			  {
			    bHit = (bReq ? FALSE : TRUE); //NOT有無
			    if (pReq1)
			    {
				  bMach = FALSE;
			      pReq3 = pReq1;
                  do 
			      {
				    if (pReq2 = strpbrk(pReq3, " )"))
				    {
			          cReq = *pReq2;
				      *pReq2 = '\x0';
					}
					nReqLen = strlen(pReq3);
				    if (!strnicmp(mLine, pReq3, strlen(pReq3)))
			        {
				      if (mLine[strlen(pReq3)] == ':')
					  {
						bMach = TRUE;
			            bHit = (bReq ? TRUE : FALSE); //NOT有無
						if (pReq2)
						  *pReq2 = cReq;
					    break;
				      }
				    }
					if (pReq2)
				      *pReq2 = cReq;
					if (bMach)
					  break;
				      //pReq3 = pReq2+1;
					pReq3 += nReqLen;
					while(*pReq3 == ' ' || *pReq3 == '\t' || *pReq3 == ')')
					  pReq3++;
				  } while(*pReq3);
			    } else {
			      bHit = (bReq ? FALSE : TRUE); //NOT有無;
			    }
			  }
			} else {
			  bHit = TRUE;
			} 
			if (bHit)
			{
#endif
			  if (i == 0) {
                uSize.QuadPart += strlen(mLine); // 該当パートのサイズ計算
			  } else if (i == 1) {
		        //if (pRange && (nTotal > uSize.QuadPart + strlen(mLine))) {
			    if (pRange && (nTotal < uSize.QuadPart + strlen(mLine))) {
			      uSize.QuadPart = nTotal;
	              sprintf(pContext->mMessages, "%u}\r\n", (uSize.QuadPart == 0 ? 2: uSize.QuadPart));
                  put_reply(lpClientContext, TRUE, TRUE);
			      break;
			    } else {
                  uSize.QuadPart += strlen(mLine);
			    }
			  } else {
			    if (nSend + strlen(mLine) < FETCH_BODY_BUFFER) 
			    {
			      nSend += strlen(mLine);
                  strcat(pContext->mMessages, mLine);
			    }
			    else 
			    {
			      pContext->mMessages[FETCH_BODY_BUFFER] = '\x0';
			      n = strlen(pContext->mMessages);
			      if (pRange && (nTotal < n)) {
			        pContext->mMessages[nTotal] = '\x0'; // サイズを調整
			        n = strlen(pContext->mMessages);
				  }
#ifdef UPDATE_20231209 // FETCH命令中に通信切断された場合に大きなサイズのデータ送信の中断を行えるようにした。
                  if (!put_reply(lpClientContext, TRUE, FALSE) && bSendErrEscape) { //FALSE); //bSendErrEscape; //送信エラー時中断 1:する(デフォルト) 0:しない
					break;
				  }
#else
                  put_reply(lpClientContext, TRUE, FALSE); //FALSE);
#endif
				  if (pRange) {
				    nTotal -= n;
				    if (nTotal == 0) {
                      nSend = 0;
				      break;
				    }
				  }
				  nSend = strlen(mLine);
                  strcpy(pContext->mMessages, mLine);
			    }
			  }
#ifdef UPDATE_20231119A // MIMEパートに対する、"HEADER.FIELDS""命令がが失敗する不具合。
		    }
#endif
		  }
		}
		if (nType == 2 && bHead && strcmp(mLine, "\r\n") == 0) // ヘッダの終わり
		{
		  bHead = FALSE;
#ifdef UPDATE_20150702A // BODY[n.n]のように深い階層にデータがないと上位層のデータを応答してしまう不具合。
	      if (!mBound[0] && nDepath > 0)
		  {
            uSize.QuadPart = 0;
			break;
		  }
#endif
		}
#ifdef UPDATE_20230706 // FETCH時のメールデータを事前に一括して読込み検索するフラグを追加
		if (pContext->pfBufp)
		  p = mgets(mLine, sizeof(mLine)-1, &pContext->pfBuff);
		 else
#endif
        p = fgets(mLine, sizeof(mLine), fp);
#ifdef UPDATE_20231003 // データ受信時のCPU負荷を下げる対策
        nCloolStep++;
		if (nPeekCoolTime)
		{
		  if (i == 2 && (nCloolStep % nPeekCoolTime) == 0)
		  {
#ifdef UPDATE_20240830A // FETCH命令で処理トーク解析処理の際のCPU負荷を下げる対策(UPDATE_20240827 でスリープする時間を0からPeekCoolTimeとした)
			_sleep(nPeekCoolTime);
#else
	        _sleep(0);
#endif
		  }
		}
#endif
	  }
	  //if (i == 0) {
#ifdef UPDATE_20150703  // BODY[]のときMIME構造があると出力サイズ値と実データが一致しない場合があった。
	  if (i == 0 && nType == 0) {
        uSize.QuadPart = nLen; // 該当パートのサイズ計算
	  }
      nLen = 0; // リセット
#endif
	  if (i <= 1) {
#ifdef UPDATE_20150702A // BODY[n.n]のように深い階層にデータがないと上位層のデータを応答してしまう不具合。
#ifdef UPDATE_20240121 // FETCH BODY[x.x]<n.x> のの応答で範囲指定が正しくない不具合。
	    if (uSize.QuadPart == 0 ||
			bHfields && i == 0 && nStart > uSize.QuadPart)  // 開始サイズオーバー
#else
		if (uSize.QuadPart == 0 ||
			i == 0 && nStart > uSize.QuadPart)  // 開始サイズオーバー
#endif
#else
		if (i == 0 && nStart > uSize.QuadPart)   // 開始サイズオーバー
#endif
		{
#ifdef UPDATE_20231205 // FETCH命令で取得データが空白の場合、0バイトデータとして応答する。
          sprintf(pContext->mMessages, "0}\r\n");
#else
          sprintf(pContext->mMessages, "2}\r\n");
#endif
          put_reply(lpClientContext, TRUE, TRUE);
		  break;
		} else {
		  *pContext->mMessages = '\x0';
          bHead = TRUE;
		  nP = 0;
#ifdef UPDATE_20230706 // FETCH時のメールデータを事前に一括して読込み検索するフラグを追加
		  if (pContext->pfBufp)
		  {
#ifdef UPDATE_20231122 // HEADER.FIELDS命令の応答で範囲指定が正しくない不具合。
	        if ((nType == 0 || nType == 2) && pRange && !bHfields)
#else
	        if ((nType == 0 || nType == 2) && pRange)
#endif
	          pContext->pfBuff = pContext->pfBufp + (long)nStart; // 開始バイト位置指定
		    else
              pContext->pfBuff = pContext->pfBufp;
		  } else {
#endif
#ifdef UPDATE_20231122 // HEADER.FIELDS命令の応答で範囲指定が正しくない不具合。
	      if ((nType == 0 || nType == 2) && pRange && !bHfields)
#else
	      if ((nType == 0 || nType == 2) && pRange)
#endif
	        fseek(fp, nStart, SEEK_SET); // 開始バイト位置指定
		  else
            fseek(fp, 0L, SEEK_SET);
#ifdef UPDATE_20230706 // FETCH時のメールデータを事前に一括して読込み検索するフラグを追加
		  }
#endif
		}
		if (i == 0)  // トータルサイズクリア
		  uSize.QuadPart = 0;
	  }
// 20231003 //大きなファイルが読み込みするとCPU負荷が上がるのと、同一フォルダのSELECTが完了まで待たされる
//#ifdef UPDATE_20231003 // データ受信時のCPU負荷を下げる対策
	  //_sleep(5000);
//#endif
	}
    if (nSend > 0) { // 残りを送信
	  nSend = 0;
#ifdef UPDATE_20231122 // HEADER.FIELDS命令の応答で範囲指定が正しくない不具合。
	  n = strlen(&pContext->mMessages[(bHfields ? nStart : 0)]);
#else
	  n = strlen(pContext->mMessages);
#endif
	  if (pRange && (nTotal < n)) {
#ifdef UPDATE_20231122 // HEADER.FIELDS命令の応答で範囲指定が正しくない不具合。
	    pContext->mMessages[nTotal+(bHfields ? nStart : 0)] = '\x0'; // サイズを調整
#else
	    pContext->mMessages[nTotal] = '\x0'; // サイズを調整
#endif
	    n = strlen(pContext->mMessages);
	  }
#ifdef UPDATE_20231122 // HEADER.FIELDS命令の応答で範囲指定が正しくない不具合。
	  if (bHfields)
	    memcpy(pContext->mMessages, &pContext->mMessages[nStart], nTotal);
#endif
      put_reply(lpClientContext, TRUE, FALSE); //TRUE);
	  if (pRange)
        nTotal -= n;
	}
#ifdef UPDATE_20230706 // FETCH時のメールデータを事前に一括して読込み検索するフラグを追加
    if (!pContext->pBuff && fp)
	  fclose(fp);
#else
	fclose(fp);
#endif
  }
}

#ifdef UPDATE_20150630 // MESSAGE/RFC822の構造出力への対応
int default_header(PCLIENT_CONTEXT lpClientContext, PPartStruct PS, char *pLine, int nLineLen, char *pBound, FILE *fp)
#else
void default_header(PCLIENT_CONTEXT lpClientContext, PPartStruct PS, char *pLine, int nLineLen, char *pBound, FILE *fp)
#endif
{
  PImap4Context    pContext = &lpClientContext->Context;
  char             *p, *p2, *p3;
  BOOL             bCT = FALSE;
#ifdef UPDATE_20150630 // MESSAGE/RFC822の構造出力への対応
  int              nCnt = 0;
#endif
#ifdef UPDATE_20150709 // MESSAGE/RFC822の構造出力への対応(入れ子の解析)でTEXT以外構造が出力されなかった。
  BOOL              bDi = FALSE;
  char              *p4, *p5;
#endif
#ifdef UPDATE_20231025 //Content-Type:ヘッダに続く次行が改行のみしか無い場合、添付ファイルと認識できない不具合
  char             *p6;
#endif

  *pBound = '\x0';
  strcpy(PS->mBEnc, "\"7bit\""); // デフォルト
#ifdef UPDATE_20230706 // FETCH時のメールデータを事前に一括して読込み検索するフラグを追加
  if (pContext->pfBufp)
    p = mgets(pLine, nLineLen, &pContext->pfBuff);
  else
#endif
  p = fgets(pLine, nLineLen, fp);
#ifdef UPDATE_20230706 // FETCH時のメールデータを事前に一括して読込み検索するフラグを追加
  while (strcmp(pLine, "\r\n") && (p || (pContext->pfBuff ? *pContext->pfBuff : !feof(fp))))
#else
  while(strcmp(pLine, "\r\n") && (p || !feof(fp)))
#endif
  { // ヘッダの最後まで
#ifdef UPDATE_20150630 // MESSAGE/RFC822の構造出力への対応
    nCnt++;
	PS->uHead.QuadPart += strlen(pLine);
#endif
#ifdef UPDATE_20110126 // １カラム目のピリオドは出力しない対策
    if (*pLine == '.')
	{
      memmove(pLine, pLine+1, strlen(pLine));
	}
#endif
	if (!(*pLine == ' ' || *pLine == '\t')) {  // 別のヘッダー
	  bCT = FALSE;
	}
#ifdef UPDATE_20150709 // MESSAGE/RFC822の構造出力への対応(入れ子の解析)でTEXT以外構造が出力されなかった。
#ifdef UPDATE_20230821 // "Content-Transfer-Encoding:"ヘッダが"Content-Disposition:"の次にあるとメール本文のエンコーディング情報を正しく返せない
	if (!bDi && !strnicmp(pLine, "Content-Disposition:", 20) || bDi && (*pLine==' ' || *pLine =='\t') )
#else
	if (!bDi && !strnicmp(pLine, "Content-Disposition:", 20) || bDi)
#endif
	{
	  if (bDi) {
        p2 = p;
	    p2 = SKIP_SP_TAB(p2);
		if (*p == ' ' || *p == '\t') { // 名称がまたがる場合の処理
		  if ((p4 = strstr(PS->mBDisp, "\")")))
			*p4 = '\x0';
		  else if ((p4 = strstr(PS->mBDisp, ")")))
			*p4 = '\x0';
		  if (p4) {
            strtok(p2, "\"\r\n");
#ifdef UPDATE_20091129E // CatchMe@Mailで添付きメールの表示が失敗する
			if (!strnicmp(p2, "filename*", 9))
			{
			  p2 = strchr(p2, '=');
			  p2++;
			}
			strtok(p2, ";\"\r\n");
#endif
#ifdef UPDATE_20150615A // BODYSTRUCTUREでヘッダ構造に誤った解析結果を出力する場合があった
			if (!strncmp(p2, "=?", 2))
			{
              sprintf(p4, " %s\"))", p2);
			} else
#endif
			{
              sprintf(p4, "%s\"))", p2);
			}
		  }
		} else {
		  bDi = FALSE;
		}
	  } else {
		bDi = TRUE;
	    p2 = pLine+20;
	    p2 = SKIP_SP_TAB(p2);
	    p3 = strchr(p2, ';');
	    if (p3) {
	      *p3++ = '\x0';
          sprintf(PS->mBDisp,"(\"%s\"", p2); // ボディ記述
#ifdef UPDATE_20150618  // HEADER(Content-Disposition:)構造のattachmentの続きに半角スペースだけあると正しく抽出できない不具合
          while (*p3 == ' ' || *p3 == '\t')
		  {
            p3++;
		  }
#endif
          if (*p3 == '\r' || *p3 == '\n') { // 次レコード読込
#ifdef UPDATE_20230706 // FETCH時のメールデータを事前に一括して読込み検索するフラグを追加
            if (pContext->pfBufp)
              p3 = mgets(pLine, nLineLen, &pContext->pfBuff);
            else
#endif
		    p3 = fgets(pLine, nLineLen, fp);
		    if (p3 == NULL ||
				!(*p3 == '\t' || *p3 == ' '))
			  break;
		  }
          p2 = p3;
		  p2 = SKIP_SP_TAB(p2);
		  p3 = strchr(p2, '=');
		  if (p3) {
		    *p3++ = '\x0';
#ifdef UPDATE_20150620A // HEADER(Content-Type:)構造でboundaryの文字列の後ろに空白がある場合に正しく抽出できない不具合
		    while(*p3 == ' ' || *p3 == '\t')
			{
			  p3++;
			}
#endif
		    if (p3) {
#ifdef UPDATE_20150618B  // HEADER(Content-Type:)構造でnameに続くファイル名が改行後に記述されている正しく抽出できない不具合
			  if (*p3 == '\r' || *p3 == '\n')
			  {
			    *p3 = '\x0';
			  }
#endif
              strtok(p3, ";\r\n");
		      /////////// ボディ記述
		      strcat(PS->mBDisp, " (\"");
#ifdef UPDATE_20091129E // CatchMe@Mailで添付きメールの表示が失敗する
			  if (!strnicmp(p2, "filename*", 9))
			  {
				*(p2+9) = '\x0';
			  }
			  strcat(PS->mBDisp, p2);
#else
			  strcat(PS->mBDisp, p2);
#endif
#ifdef UPDATE_20091129B // CatchMe@Mailで添付きメールの表示が失敗する
			  strcat(PS->mBDisp, "\"");
			  {
			    CHAR mLen[80];
			    if (strchr(p3+1, '"'))
				{
				  p3++;
				  strtok(p3,"\"");
				}
			    sprintf(mLen, "{%d}\r\n", strlen(p3));
			    strcat(PS->mBDisp, mLen);
			  }
#else
			  strcat(PS->mBDisp, "\" ");
			  if (!strchr(p3+1, '"'))
				strcat(p3, "\"");
#endif
#ifdef UPDATE_20091129E // CatchMe@Mailで添付きメールの表示が失敗する
			  if (*p3 != '"')
			  {
			    strcat(PS->mBDisp, "\"");
			  }
#endif
			  strcat(PS->mBDisp, p3);
			  strcat(PS->mBDisp, ")");
			  /////////////////////
			}
		  }
		  strcat(PS->mBDisp, ")");
		}
	  }
	} else
#endif	  
	if (!strnicmp(pLine, "Content-Transfer-Encoding:", 26)) { // 7bit
	  bCT = FALSE;
      p2 = pLine+26;
	  p2 = SKIP_SP_TAB(p2);
	  strtok(p2, "\r\n");
#ifdef UPDATE_20180109A //Content-Transfer-Encoding:ヘッダ情報が空白の場合の対策
	  if (*p2 == '\r' || *p2 == '\n')
	    *p2 = '\x0';
#endif
	  sprintf(PS->mBEnc, "\"%s\"", p2);
	} else if (!bCT && !strnicmp(pLine, "Content-Type:", 13) ||
		 bCT) {
#ifdef UPDATE_20231013 // Content-Type:ヘッダに続くデータが改行のみしか無い場合、添付ファイルと認識できない不具合
	  if (!strnicmp(pLine, "Content-Type:", 13))
	  {
		if (!strstr((pLine+13), ";"))
		{
#ifndef UPDATE_20231025 //Content-Type:ヘッダに続く次行が改行のみしか無い場合、添付ファイルと認識できない不具合
		  strtok(pLine, "\r\n");
#endif
#ifdef UPDATE_20231025 //Content-Type:ヘッダに続く次行が改行のみしか無い場合、添付ファイルと認識できない不具合
		  p6 = pLine+13;
		  while(*p6 == ' ' || *p6 == '\t')
		    p6++;
		  if (*p6 == '\r' || *p6 == '\n')
#endif
#ifdef UPDATE_20231024 //Content-Type:ヘッダに続く次行が改行のみしか無い場合、添付ファイルと認識できない不具合
		  {
	        long ctcur;
		    CHAR *pctcur;

			if (pContext->pfBufp)
			  pctcur = pContext->pfBuff;
			else
			  ctcur = ftell(fp);
#endif
#ifdef UPDATE_20231025 //Content-Type:ヘッダに続く次行が改行のみしか無い場合、添付ファイルと認識できない不具合
		  strtok(pLine, "\r\n");
#endif
#ifdef UPDATE_20230706 // FETCH時のメールデータを事前に一括して読込み検索するフラグを追加
          if (pContext->pfBufp)
	        p = mgets((pLine+strlen(pLine)), nLineLen, &pContext->pfBuff);
	      else
#endif
            p = fgets((pLine+strlen(pLine)), nLineLen, fp);
#ifdef UPDATE_20231024 //Content-Type:ヘッダに続く次行が改行のみしか無い場合、添付ファイルと認識できない不具合
		    if (p)
			{
			  if (*p == '\r' || *p == '\n')
			  {
		        if (pContext->pfBufp)
				  pContext->pfBuff = pctcur;
			    else
				  fseek(fp, ctcur, SEEK_SET); // 現在位置から開始バイト位置指定
			  }
			}
		  }
#endif
		}
	  }
#endif
	  if (bCT) {
        p2 = p;
	    p2 = SKIP_SP_TAB(p2);
	    p3 = strchr(p2, '=');
	    if (p3) {
	      *p3++ = '\x0';
#ifdef UPDATE_20150620A // HEADER(Content-Type:)構造でboundaryの文字列の後ろに空白がある場合に正しく抽出できない不具合
		  while(*p3 == ' ' || *p3 == '\t')
		  {
			p3++;
		  }
#endif
		  if (p3) {
            strtok(p3, ",;\r\n");
			//// "ダブルクォーテーションがある場合は一旦削除
		    if (*p3 == '"') {
		      p3++;
		      strtok(p3, "\"");
			}
#ifdef UPDATE_20150620A // HEADER(Content-Type:)構造でboundaryの文字列の後ろに空白がある場合に正しく抽出できない不具合
			strtok(p2, " \t");
#endif
			sprintf(PS->mBPara,"(\"%s\" \"%s\")", p2, p3); // ボディパラメータ括弧つきリスト
			if (!stricmp(p2, "boundary")) 
			{ // マルチパートなら処理
	          sprintf(pBound, "--%s", p3);
	          strtok(pBound, "\"");
			  bCT = FALSE;
			}
		  }
		}
	  } else {
		bCT = TRUE;
        p2 = pLine+13;
	    p2 = SKIP_SP_TAB(p2);
	    p3 = strchr(p2, '/');
	    if (p3) {
	      *p3++ = '\x0';
          sprintf(PS->mBType, "\"%s\"", p2); // ボディタイプ
          p2 = p3;
	      p3 = strchr(p2, ';');
#ifdef UPDATE_20150709 // MESSAGE/RFC822の構造出力への対応(入れ子の解析)でTEXT以外構造が出力されなかった。
		  if (!p3) // 行にセミコロンがないときの対処
		  {
	        p3 = strpbrk(p2, " \t\r\n");
		  }
#endif
	      if (p3) {
	        *p3++ = '\x0';
            sprintf(PS->mBSub,"\"%s\"", p2); // ボディサブタイプ
#ifdef UPDATE_20230706_DEBUG // FETCH時のメールデータを事前に一括して読込み検索するフラグを追加
if (bDebug) printf("PS->mBType=%s / PS->mBSub=%s\n", PS->mBType, PS->mBSub);
#endif
			/*
			p3 = SKIP_SP_TAB(p3);
            if (*p3 == '\r' || *p3 == '\n') { // 次レコード読込
              p3 = fgets(pLine, nLineLen, fp);
 			  if ((p3 == NULL) ||
				  !(*p3 == '\t' || *p3 == ' '))
			   break;
			}
			*/
            p2 = p3;
	        p2 = SKIP_SP_TAB(p2);
	        p3 = strchr(p2, '=');
	        if (p3) {
	          *p3++ = '\x0';
#ifdef UPDATE_20150620A // HEADER(Content-Type:)構造でboundaryの文字列の後ろに空白がある場合に正しく抽出できない不具合
		      while(*p3 == ' ' || *p3 == '\t')
			  {
			    p3++;
			  }
#endif
		      if (p3) {
#ifdef UPDATE_20150618A  // HEADER(Content-Type:)構造で改行されずにboundaryが記述されていると正しく抽出できない不具合
                if (p = strchr(p3, ';'))
				{
                  p++;
				}
#endif
                strtok(p3, ",;\r\n");
                //sprintf(PS->mBPara,"(\"%s\" %s)", p2, p3); // ボディパラメータ括弧つきリスト
				//// charset=に"ダブルクォーテーションがある場合は一旦削除
				if (*p3 == '"') {
				  p3++;
				  strtok(p3, "\"");
				}
#ifdef UPDATE_20150620A // HEADER(Content-Type:)構造でboundaryの文字列の後ろに空白がある場合に正しく抽出できない不具合
			    strtok(p2, " \t");
#endif
				sprintf(PS->mBPara,"(\"%s\" \"%s\")", p2, p3); // ボディパラメータ括弧つきリスト
			    if (!stricmp(p2, "boundary")) { // マルチパートなら処理
	              sprintf(pBound, "--%s", p3);
	              strtok(pBound, "\"");
				}
#ifdef UPDATE_20150618A  // HEADER(Content-Type:)構造で改行されずにboundaryが記述されていると正しく抽出できない不具合
#ifdef UPDATE_20150712C // Content-TypeにPGPが含まれている時正しい構造が生成できない不具合。
                while (p)
#else
				if (p)
#endif
				{
				  p2 = SKIP_SP_TAB(p);
	              p3 = strchr(p2, '=');
		          if (p3) {
#ifdef UPDATE_20150712C // Content-TypeにPGPが含まれている時正しい構造が生成できない不具合。
                    if (p = strchr(p3, ';'))
					{
                      p++;
					}
#endif
					*p3++ = '\x0';
#ifdef UPDATE_20150620A // HEADER(Content-Type:)構造でboundaryの文字列の後ろに空白がある場合に正しく抽出できない不具合
		            while(*p3 == ' ' || *p3 == '\t')
					{ 
			          p3++;
					}
#endif
				    if (*p3 == '"') {
				      p3++;
				      strtok(p3, "\"");
					}
#ifdef UPDATE_20150620A // HEADER(Content-Type:)構造でboundaryの文字列の後ろに空白がある場合に正しく抽出できない不具合
			        strtok(p2, " \t");
#endif
			        if (!stricmp(p2, "boundary")) { // マルチパートなら処理
	                  sprintf(pBound, "--%s", p3);
	                  strtok(pBound, "\"");
					}
				  }
#ifdef UPDATE_20150712C // Content-TypeにPGPが含まれている時正しい構造が生成できない不具合。
				  else 
				  {
					break;
				  }
#endif
				}
#endif
			  }
			}
		  }
		}
	  }
	}
#ifdef UPDATE_20230706 // FETCH時のメールデータを事前に一括して読込み検索するフラグを追加
    if (pContext->pfBufp)
      p = mgets(pLine, nLineLen, &pContext->pfBuff);
    else
#endif
    p = fgets(pLine, nLineLen, fp);
  }
#ifdef UPDATE_20150630 // MESSAGE/RFC822の構造出力への対応
  return nCnt;
#endif
}

void BODY_COUNTER(PCLIENT_CONTEXT lpClientContext, PPartStruct PS, char *pLine, int nLineLen, char *pBound, FILE *fp) 
{
  PImap4Context    pContext = &lpClientContext->Context;
  char             *p;
#ifdef UPDATE_20150712 // MIME構造の階層管理に親パート番号を設定できる項目を追加した。
  BOOL bEnd = FALSE;
#endif

  PS->uBody.QuadPart = PS->nLine = 0;
#ifdef UPDATE_20230706 // FETCH時のメールデータを事前に一括して読込み検索するフラグを追加
  if (pContext->pfBufp)
    p = mgets(pLine, nLineLen, &pContext->pfBuff);
  else
#endif
  p = fgets(pLine, nLineLen, fp);
  do {
#ifdef UPDATE_20230706_DEBUG // FETCH時のメールデータを事前に一括して読込み検索するフラグを追加
if (bDebug) printf("BODY_COUNTER(0) pLine=%s\n", pLine);
#endif
	if (!strcmp(pLine, ".\r\n"))
	  break;
	if (*pBound != '\x0' && !strnicmp(pLine, pBound, strlen(pBound))) // メッセージの終了
	{
	  break;
	}
	PS->nLine++;
	PS->uBody.QuadPart += strlen(pLine);
#ifdef UPDATE_20110126 // １カラム目のピリオドは出力しない対策
    if (*pLine == '.')
	{
      PS->uBody.QuadPart--;
	}
#endif
#ifdef UPDATE_20230706 // FETCH時のメールデータを事前に一括して読込み検索するフラグを追加
    if (pContext->pfBufp)
      p = mgets(pLine, nLineLen, &pContext->pfBuff);
    else
#endif
    p = fgets(pLine, nLineLen, fp);
#ifdef UPDATE_20230706 // FETCH時のメールデータを事前に一括して読込み検索するフラグを追加
  } while (p || (pContext->pfBuff ? *pContext->pfBuff : !feof(fp)));
#else
  } while(p || !feof(fp)); // マルチパートの始まりまでスキップ
#endif
}

#ifdef UPDATE_20150618C  // HEADER(Content-Type:)構造でmultipart/alternativeの入れ子構造が正しく抽出できない不具合

#ifdef UPDATE_20150709 // MESSAGE/RFC822の構造出力への対応(入れ子の解析)でTEXT以外構造が出力されなかった。
#ifdef UPDATE_20150710 // MESSAGE/RFC822の構造出力への対応(入れ子の解析)でTEXT以外構造が出力されなかった。
#ifdef UPDATE_20150711 // MESSAGE/RFC822の構造出力への対応(入れ子の解析)でTEXT以外構造が出力されなかった。
#ifdef UPDATE_20150712 // MIME構造の階層管理に親パート番号を設定できる項目を追加した。
int multipart_bodystructure(PCLIENT_CONTEXT lpClientContext, DWORD *pnPart, PPartStruct pPS, char *pLine,  FILE *fp, char *pBound, BOOL bExtend, BOOL bSet)
#else
int multipart_bodystructure(PCLIENT_CONTEXT lpClientContext,char *pLine,  FILE *fp, char *pBound, BOOL bExtend, BOOL bSet)
#endif
#else
void multipart_bodystructure(PCLIENT_CONTEXT lpClientContext,char *pLine,  FILE *fp, char *pBound, BOOL bExtend, BOOL bSet)
#endif
#else
void multipart_bodystructure(PCLIENT_CONTEXT lpClientContext,char *pLine,  FILE *fp, BOOL bExtend, BOOL bSet)
#endif
#else
void multipart_bodystructure(PCLIENT_CONTEXT lpClientContext,char *pLine,  FILE *fp, BOOL bExtend)
#endif
{
  PImap4Context    pContext = &lpClientContext->Context;
  char             mFn[MAX_PATH], mLine[1024], mBound[BODY_PART_LIMIT][256];
  char             mBody[32], mRec[32];
  DWORD            n, i, j[BODY_PART_LIMIT], nPart = 0, nDepath = 0;
  BOOL             bBody = FALSE;

#ifndef UPDATE_20150712 // MIME構造の階層管理に親パート番号を設定できる項目を追加した。
  PartStruct    PS[BODY_PART_LIMIT]; //
#else
  long cur;
  PPartStruct   PS = pPS;
#endif
#ifdef UPDATE_20150710 // MESSAGE/RFC822の構造出力への対応(入れ子の解析)でTEXT以外構造が出力されなかった。
  BOOL bBoundSet = FALSE;
  BOOL bEnd = FALSE;
  char mEndBound[256] = "";
#endif
#ifdef UPDATE_20150711 // MESSAGE/RFC822の構造出力への対応(入れ子の解析)でTEXT以外構造が出力されなかった。
  int nCloseType = 0;
#endif

#ifdef UPDATE_20150710 // MESSAGE/RFC822の構造出力への対応(入れ子の解析)でTEXT以外構造が出力されなかった。
  if (*pBound)
  {
    sprintf(mEndBound, "%s--", pBound);
  }
#endif

#ifndef UPDATE_20150712 // MIME構造の階層管理に親パート番号を設定できる項目を追加した。
#ifdef UPDATE_20150709 // MESSAGE/RFC822の構造出力への対応(入れ子の解析)でTEXT以外構造が出力されなかった。
  if (bSet)
#endif
  {
    sprintf(pContext->mMessages, "(" );
    put_reply(lpClientContext, FALSE, TRUE);
  }
#endif

#ifdef UPDATE_20150712 // MIME構造の階層管理に親パート番号を設定できる項目を追加した。
  nPart = *pnPart;//+1;
#else
  for (i = 0; i < BODY_PART_LIMIT; i++) {
#ifdef UPDATE_20150630 // MESSAGE/RFC822の構造出力への対応
   strcpy(PS[i].mBType, "\"TEXT\""); // ボディタイプ Content-Type: text/plain
   strcpy(PS[i].mBSub, "\"PLAIN\""); // ボディサブタイプ 
#else
    PS[i].mBType[0] = '\x0';
    PS[i].mBSub[0]  = '\x0';
#endif
    PS[i].mBPara[0] = '\x0';
    PS[i].mBID[0]   = '\x0';
    PS[i].mBDisp[0] = '\x0';
    PS[i].mBEnc[0]  = '\x0';
    PS[i].uBody.QuadPart = 0;
#ifdef UPDATE_20150630 // MESSAGE/RFC822の構造出力への対応
    PS[i].uHead.QuadPart = 0;
	PS[i].nHLine         = 0;
#endif
    PS[i].nLine          = 0;
#ifdef UPDATE_20150712      // MIME構造の階層管理に親パート番号を設定できる項目を追加した。
	PS[i].nPPart = 0;    // 親パート番号
#endif
#ifdef UPDATE_20150630 // MESSAGE/RFC822の構造出力への対応
	strcpy(PS[i].PSM.mBType, "\"TEXT\""); // ボディタイプ Content-Type: text/plain
    strcpy(PS[i].PSM.mBSub, "\"PLAIN\""); // ボディサブタイプ 
    PS[i].PSM.mBPara[0] = '\x0';          // ボディパラメータ括弧つきリスト ("name" "test.bin")
	PS[i].PSM.mBID[0] = '\x0';            // ボディ ID Content-ID: <Mm3V1n13r86nICg78>
    PS[i].PSM.mBDisp[0] = '\x0';          // ボディ記述 Content-Disposition: attachment;
    strcpy(PS[i].PSM.mBEnc, "\"7bit\"");  // ボディ符号化 Content-Encoding: US-ASCII,base64
    PS[i].PSM.uHead.QuadPart = 0;         // ヘッダサイズ
	PS[i].PSM.uBody.QuadPart = 0;         // ボディサイズ：
	PS[i].PSM.nHLine = 0;                 // ヘッダ部行数;
	PS[i].PSM.nLine = 0;                  // 本文行数;
#endif
#ifdef UPDATE_20150712      // MIME構造の階層管理に親パート番号を設定できる項目を追加した。
	PS[i].PSM.nPPart = 0;    // 親パート番号
#endif
    mBound[i][0] = '\x0';
	j[i] = 0;
  }
#endif
  //sprintf(mFn, "%s/%s", pContext->mSelectDir, (pContext->bEncMSG ? pContext->mDecFileName : pContext->FindData.cFileName));
  //strcpy(mLine, pLine);
#ifdef UPDATE_20230706 // FETCH時のメールデータを事前に一括して読込み検索するフラグを追加
  if (pContext->pBuff || !pContext->pBuff && fp)
#else
  if (fp) 
#endif
  {
#ifdef UPDATE_20150712 // MIME構造の階層管理に親パート番号を設定できる項目を追加した。
	(PS+nPart)->nPPart = *pnPart; // 親パートNO設定
#endif
	PS[nPart].nHLine = default_header(lpClientContext, &PS[nPart], mLine, sizeof(mLine), mBound[nDepath], fp); // 最初のヘッダ
#ifdef UPDATE_20150712 // MIME構造の階層管理に親パート番号を設定できる項目を追加した。
#ifdef UPDATE_20230706 // FETCH時のメールデータを事前に一括して読込み検索するフラグを追加
	if (pContext->pfBufp)
      cur = (long)(pContext->pfBuff - pContext->pfBufp);
	else
#endif
	cur = ftell(fp);
#endif
#ifdef UPDATE_20230706_DEBUG // FETCH時のメールデータを事前に一括して読込み検索するフラグを追加
if (bDebug) printf("multipart_bodystructure(0) cur=%lu\n", cur);
#endif
#ifdef UPDATE_20150711 // MESSAGE/RFC822の構造出力への対応(入れ子の解析)でTEXT以外構造が出力されなかった。
	if(!bSet && !stricmp(PS[nPart].mBType, "\"multipart\""))
	{
      sprintf(pContext->mMessages, "(" );
      put_reply(lpClientContext, FALSE, TRUE);
	}
#endif
#ifdef UPDATE_20150710 // MESSAGE/RFC822の構造出力への対応(入れ子の解析)でTEXT以外構造が出力されなかった。
    if (*pBound && !mBound[nDepath][0])
	{
      strcpy(mBound[nDepath], pBound);
	  bBoundSet = TRUE;
	}
#endif
	BODY_COUNTER(lpClientContext, &PS[nPart], mLine, sizeof(mLine), mBound[nDepath], fp);   // BODYSIZE
#ifdef UPDATE_20150710 // MESSAGE/RFC822の構造出力への対応(入れ子の解析)でTEXT以外構造が出力されなかった。
    if (bBoundSet)
	{
      mBound[nDepath][0] = '\x0';
	}
#endif
	if (mBound[0][0] != '\x0') { // マルチパートなら次の処理
#ifdef UPDATE_20150712 // MIME構造の階層管理に親パート番号を設定できる項目を追加した。
#ifdef UPDATE_20230706 // FETCH時のメールデータを事前に一括して読込み検索するフラグを追加
      if (pContext->pfBufp)
		pContext->pfBuff = pContext->pfBufp + (long)cur;
	  else
#endif
	  fseek(fp, cur, SEEK_SET);
#endif
      do {
	    nPart++;
		if (nPart >= BODY_PART_LIMIT)
		{
#ifdef UPDATE_20090325C // メッセージのBODYパート数の上限を超えた場合ハングする
		  nPart = BODY_PART_LIMIT-1;
#endif
		  break;
		}
		strcpy(PS[nPart].mBType, "end_multipart");

#ifdef UPDATE_20150712 // MIME構造の階層管理に親パート番号を設定できる項目を追加した。
        nPart = multipart_header(lpClientContext, nPart, PS, &PS[nPart], mLine, sizeof(mLine), mBound[nDepath], mBound[nDepath+1], fp , bExtend); // マルチパート
#else
#ifdef UPDATE_20150711 // MESSAGE/RFC822の構造出力への対応(入れ子の解析)でTEXT以外構造が出力されなかった。
        nCloseType = multipart_header(lpClientContext, nPart, PS, &PS[nPart], mLine, sizeof(mLine), mBound[nDepath], mBound[nDepath+1], fp , bExtend); // マルチパート
		if (nCloseType > 0)
		{
          nPart--;
		}
#else
#ifdef UPDATE_20150710 // MESSAGE/RFC822の構造出力への対応(入れ子の解析)でTEXT以外構造が出力されなかった。
        nPart += multipart_header(lpClientContext, nPart, PS, &PS[nPart], mLine, sizeof(mLine), mBound[nDepath], mBound[nDepath+1], fp , bExtend); // マルチパート
#else
#ifdef UPDATE_20150708 // MIME構造で(multipart(text,multipart(html))のような構造で順番道理の結果が返せない不具合。
        nPart += multipart_header(lpClientContext, (nPart > 0 ? &PS[nPart-1] : NULL), &PS[nPart], mLine, sizeof(mLine), mBound[nDepath], mBound[nDepath+1], fp , bExtend); // マルチパート
#else
#ifdef UPDATE_20150620B // 多重入れ子構造がある場合に正しく抽出できない不具合
        multipart_header(lpClientContext, &PS[nPart], mLine, sizeof(mLine), mBound[nDepath], mBound[nDepath+1], fp , bExtend); // マルチパート
#else
        multipart_header(lpClientContext, &PS[nPart], mLine, sizeof(mLine), mBound[nDepath+1], fp, bExtend); // マルチパート
#endif
#endif
#endif
#endif
#endif
#ifndef UPDATE_20150712 // MIME構造の階層管理に親パート番号を設定できる項目を追加した。
		if (mBound[nDepath+1][0] && strcmp(mBound[nDepath], mBound[nDepath+1]))
		  nDepath++;
#endif
	    BODY_COUNTER(lpClientContext, &PS[nPart], mLine, sizeof(mLine), mBound[nDepath], fp);   // BODYSIZE
		if (strstr(mLine, mBound[nDepath]) && strstr(&mLine[strlen(mBound[nDepath])],"--")) {
#ifdef UPDATE_20150712 // MIME構造の階層管理に親パート番号を設定できる項目を追加した。
	      nPart++;
		  strcpy(PS[nPart].mBType,"end_multipart");
#endif
		  if (nDepath > 0) {
            mBound[nDepath][0] = '\x0';
			nDepath--;
		  } else {
		    break;
		  }
		}
#ifdef UPDATE_20150710 // MESSAGE/RFC822の構造出力への対応(入れ子の解析)でTEXT以外構造が出力されなかった。
		if (mEndBound[0])
		{
		  bEnd = !strncmp(mLine, mEndBound, strlen(mEndBound)); 
		}
#endif
	  }
#ifdef UPDATE_20230706 // FETCH時のメールデータを事前に一括して読込み検索するフラグを追加
     while(strcmp(mLine, ".\r\n") && (pContext->pfBuff ? *pContext->pfBuff : !feof(fp)) && !bEnd );
#else
#ifdef UPDATE_20150710 // MESSAGE/RFC822の構造出力への対応(入れ子の解析)でTEXT以外構造が出力されなかった。
	  while (strcmp(mLine, ".\r\n") && !feof(fp) && !bEnd );
#else
#ifdef UPDATE_20150620B // 多重入れ子構造がある場合に正しく抽出できない不具合
	  while (strcmp(mLine, ".\r\n") && !feof(fp) );
#else
	  while (strcmp(mLine, ".\r\n") || !feof(fp) );
#endif
#endif
#endif
#ifdef UPDATE_20150712 // MIME構造の階層管理に親パート番号を設定できる項目を追加した。
	  if (strcmp(PS[nPart].mBType,"end_multipart"))
	  {
	    nPart++;
	    strcpy(PS[nPart].mBType,"end_multipart");
	  }
#endif

#ifndef UPDATE_20150712 // MIME構造の階層管理に親パート番号を設定できる項目を追加した。
	  //if (!strcmp(PS[0].mBType, "\"multipart\"")) {
	  { // 先頭のContent-Typeは読み飛ばす
	    n = 0;
	    for (i = 1; i <= nPart; i++) {
		  if (!strcmp(PS[i].mBType, "\"multipart\"")) {
		    n++;
		  } else if (!strcmp(PS[i].mBType, "end_multipart")) {
		    if (n == 0) {
              strcpy(pContext->mMessages, "");
	          PS[i].mBType[0] = '\x0';  // 最後を揃える
		      nPart = i;
		      break;
			} else
			  n--;
		  }
		}
	  }
#endif
	} 
  }

  //sprintf(pContext->mMessages, "%s %s", (bExtend ? "BODYSTRUCTURE" :"BODY") , (nPart == 0 ? "" : "("));
  //put_reply(lpClientContext, FALSE, TRUE);
  nDepath = 0;
#ifndef UPDATE_20150712 // MIME構造の階層管理に親パート番号を設定できる項目を追加した。
#ifdef UPDATE_20150711 // MESSAGE/RFC822の構造出力への対応(入れ子の解析)でTEXT以外構造が出力されなかった。
  if (mBound[nDepath][0])
  {
    nPart = 2;
  }
  for (i = (nPart == 0 ? 0 : (mBound[nDepath][0] ? nPart : 1)); i <= nPart; i++) 
#else
  for (i = (nPart == 0 ? 0 : 1); i <= nPart; i++) 
#endif
  {
	sprintf(mBody, "%lu", PS[i].uBody.QuadPart);
	sprintf(mRec, "%lu", PS[i].nLine);
#ifdef UPDATE_20150709 // MESSAGE/RFC822の構造出力への対応(入れ子の解析)でTEXT以外構造が出力されなかった。
	if (i==1 && !strcmp(PS[0].mBType, "\"multipart\"") && !strcmp(PS[0].mBSub, "\"mixed\""))
	{
	  sprintf(pContext->mMessages, "(");
	  put_reply(lpClientContext, FALSE, TRUE);
	}
#endif

#ifdef UPDATE_20150617A // Content-Type: multipart/alternative の箇所の構造を出力できない不具合
	if (!strcmp(PS[i].mBType, "\"multipart\"") && !strcmp(PS[i].mBSub, "\"mixed\"")) 
#else
	if (!strcmp(PS[i].mBType, "\"multipart\"")) 
#endif
	{
	   j[++nDepath] = i;
	   sprintf(pContext->mMessages, "(");
	} else if (!strcmp(PS[i].mBType, "end_multipart")) {
	  if (bExtend) // BODYSTRUCTURE
#ifdef UPDATE_20091129C // CatchMe@Mailで添付きメールの表示が失敗する
        sprintf(pContext->mMessages, "%s %s NIL NIL)",
#else
        sprintf(pContext->mMessages, "%s %s NIL NIL) ",
#endif
                                 (PS[j[nDepath]].mBSub[j[nDepath]] ? strupr(PS[j[nDepath]].mBSub): "NIL"),
                                 (PS[j[nDepath]].mBPara[j[nDepath]] ? PS[j[nDepath]].mBPara: "NIL"));
	  else         // BODY
        sprintf(pContext->mMessages, "%s) ", (PS[j[nDepath]].mBSub[j[nDepath]] ? strupr(PS[j[nDepath]].mBSub): "NIL"));
	  nDepath--;
	} else {
#ifdef UPDATE_20150620B // 多重入れ子構造がある場合に正しく抽出できない不具合
      if (PS[i].mBType[0] != '\x0' || mBound[0][0] == '\x0')
#endif
	  {
	    if (bExtend) // BODYSTRUCTURE
#ifdef UPDATE_20150617B // 応答に"NIL"追加
          sprintf(pContext->mMessages, "(%s %s %s %s %s %s %s %s%s %s %s %s)%s",
#else
          sprintf(pContext->mMessages, "(%s %s %s %s %s %s %s %s%s %s %s)%s",
#endif
                                    (PS[i].mBType[0] ? strupr(PS[i].mBType): "NIL"),
                                    (PS[i].mBSub[0] ? strupr(PS[i].mBSub): "NIL"),
                                    (PS[i].mBPara[0] ? PS[i].mBPara: "NIL"),
                                    (PS[i].mBID[0] ? PS[i].mBID: "NIL"),
                                    "NIL",
					  			    (PS[i].mBEnc[0] ? PS[i].mBEnc: "NIL"),
                                   (PS[i].uBody.QuadPart ? mBody: "NIL"),
                                   (!stricmp(PS[i].mBType, "\"text\"") ? (PS[i].nLine ? mRec: "NIL") : ""),
#ifdef UPDATE_20091129H // MESSAGE/RFC822の添付表示 CatchMe@Mailで添付きメールの表示が失敗する
                                   (!stricmp(PS[i].mBSub, "\"rfc822\"") ? "(NIL NIL NIL NIL NIL NIL NIL NIL NIL NIL) (NIL NIL NIL NIL NIL NIL NIL NIL NIL NIL NIL) NIL NIL" : (!stricmp(PS[i].mBType, "\"text\"") ? " NIL" : "NIL")),
                                   //(!stricmp(PS[i].mBSub, "\"rfc822\"") ? "(\"Sun, 29 Nov 2009 11:28:01 +0900 (JST)\" {34}\r\n=?ISO-2022-JP?B?GyRCRTpJVSMyGyhC?= ((NIL NIL \"user3\" \"test-sample01.jp\")) ((NIL NIL \"user3\" \"test-sample01.jp\")) ((NIL NIL \"user3\" \"test-sample01.jp\")) ((NIL NIL \"user3\" \"test-sample01.jp\")) NIL NIL NIL \"<28899428.1259461681687.JavaMail.user3@test-sample01.jp>\") (\"TEXT\" \"plain\" (\"charset\" \"ISO-2022-JP\") NIL NIL \"7bit\" 17 3 NIL NIL NIL) 18 NIL" : (!stricmp(PS[i].mBType, "\"text\"") ? " NIL" : "NIL")),
#else
                                   (!stricmp(PS[i].mBType, "\"text\"") ? " NIL" : "NIL"),
#endif
                                   (PS[i].mBDisp[0] ? PS[i].mBDisp: "NIL"),
								   "NIL",
#ifdef UPDATE_20150617B // 応答に"NIL"追加
                                   "NIL",
#endif
#ifdef UPDATE_20091129C // CatchMe@Mailで添付きメールの表示が失敗する
								   (i == 0 ? "" : ""));
#else
								   (i == 0 ? "" : " "));
#endif
	    else         // BODY
          sprintf(pContext->mMessages, "(%s %s %s %s %s %s %s %s)%s",
                                    (PS[i].mBType[0] ? strupr(PS[i].mBType): "NIL"),
                                    (PS[i].mBSub[0] ? strupr(PS[i].mBSub): "NIL"),
                                    (PS[i].mBPara[0] ? PS[i].mBPara: "NIL"),
                                    (PS[i].mBID[0] ? PS[i].mBID: "NIL"),
                                    "NIL",
					  			    (PS[i].mBEnc[0] ? PS[i].mBEnc: "NIL"),
                                   (PS[i].uBody.QuadPart ? mBody: "NIL"),
                                   (!stricmp(PS[i].mBType, "\"text\"") ? (PS[i].nLine ? mRec: "NIL") : ""),
                                   //(!stricmp(PS[i].mBType, "\"text\"") ? " NIL" : "NIL"),
                                   //(PS[i].mBDisp[0] ? PS[i].mBDisp: "NIL"),
#ifdef UPDATE_20091129C // CatchMe@Mailで添付きメールの表示が失敗する
								   (i == 0 ? "" : ""));
#else
								   (i == 0 ? "" : " "));
#endif
	  }
	}
    put_reply(lpClientContext, FALSE, TRUE);
  } 
  // マルチパートの場合はBoundaryを後から設定
  if (nPart > 0) {
    if (bExtend) // BODYSTRUCTURE
#ifdef UPDATE_20150617B // 応答に"NIL"追加
#ifdef UPDATE_20150616 // BODYSTRUCTUREのマルチパート構造の応答時に半角スペースの区切りが挿入されない箇所があった
      sprintf(pContext->mMessages, " %s %s NIL NIL NIL",
#else
      sprintf(pContext->mMessages, "%s %s NIL NIL NIL",
#endif
#else
#ifdef UPDATE_20150616 // BODYSTRUCTUREのマルチパート構造の応答時に半角スペースの区切りが挿入されない箇所があった
      sprintf(pContext->mMessages, " %s %s NIL NIL",
#else
      sprintf(pContext->mMessages, "%s %s NIL NIL",
#endif
#endif
                                 (PS[0].mBSub[0] ? strupr(PS[0].mBSub): "NIL"),
                                 (PS[0].mBPara[0] ? PS[0].mBPara: "NIL"));
     else         // BODY
#ifdef UPDATE_20150616 // BODYSTRUCTUREのマルチパート構造の応答時に半角スペースの区切りが挿入されない箇所があった
       sprintf(pContext->mMessages, " %s", (PS[0].mBSub[0] ? strupr(PS[0].mBSub): "NIL"));
#else
       sprintf(pContext->mMessages, "%s", (PS[0].mBSub[0] ? strupr(PS[0].mBSub): "NIL"));
#endif
    put_reply(lpClientContext, FALSE, TRUE);
#ifdef UPDATE_20150711 // MESSAGE/RFC822の構造出力への対応(入れ子の解析)でTEXT以外構造が出力されなかった。
    if (!stricmp(PS[0].mBSub, "\"mixed\""))
	{
	  nCloseType = nPart;
	}
#endif
  }
  ///////////////////////////////////////////
  if (nPart > 0) {
    strcpy(pContext->mMessages, ")");
    put_reply(lpClientContext, FALSE, TRUE);
  }
#ifdef UPDATE_20150711 // MESSAGE/RFC822の構造出力への対応(入れ子の解析)でTEXT以外構造が出力されなかった。
  return nCloseType;
#endif
#else
//#ifdef UPDATE_20150712 // MIME構造の階層管理に親パート番号を設定できる項目を追加した。
  *pnPart = nPart;
  return nPart;
#endif
}
#endif

#ifdef UPDATE_20150710 // MESSAGE/RFC822の構造出力への対応(入れ子の解析)でTEXT以外構造が出力されなかった。
int multipart_header(PCLIENT_CONTEXT lpClientContext, int nPart0, PPartStruct pPS0, PPartStruct PS, char *pLine, int nLineLen, char *pBound0, char *pBound, FILE *fp, BOOL bExtend)
#else
#ifdef UPDATE_20150708 // MIME構造で(multipart(text,multipart(html))のような構造で順番道理の結果が返せない不具合。
int multipart_header(PCLIENT_CONTEXT lpClientContext, PPartStruct PS0, PPartStruct PS, char *pLine, int nLineLen, char *pBound0, char *pBound, FILE *fp, BOOL bExtend)
#else
#ifdef UPDATE_20150620B // 多重入れ子構造がある場合に正しく抽出できない不具合
void multipart_header(PCLIENT_CONTEXT lpClientContext, PPartStruct PS, char *pLine, int nLineLen, char *pBound0, char *pBound, FILE *fp, BOOL bExtend)
#else
void multipart_header(PCLIENT_CONTEXT lpClientContext, PPartStruct PS, char *pLine, int nLineLen, char *pBound, FILE *fp, BOOL bExtend) 
#endif
#endif
#endif
{
  PImap4Context    pContext = &lpClientContext->Context;
  char             *p, *p2, *p3, *p4, *p5;
#ifdef UPDATE_20231025 //Content-Type:ヘッダに続く次行が改行のみしか無い場合、添付ファイルと認識できない不具合
  char             *p6;
#endif
  BOOL             bCT = FALSE, bDi = FALSE;
#ifdef UPDATE_20150630 // MESSAGE/RFC822の構造出力への対応
  //PartStruct       PSM;
  char             mMBound[256];
  long             cur; // ファイル現在位置
#endif
#ifdef UPDATE_20150630C // MESSAGE/RFC822の構造出力への対応
  int              bCType = FALSE;
#endif
#ifdef UPDATE_20150708 // MIME構造で(multipart(text,multipart(html))のような構造で順番道理の結果が返せない不具合。
  int              nPart = 0;
#endif
#ifdef UPDATE_20150709 // MESSAGE/RFC822の構造出力への対応(入れ子の解析)でTEXT以外構造が出力されなかった。
  long             cur2;
#endif
#ifdef UPDATE_20150710 // MESSAGE/RFC822の構造出力への対応(入れ子の解析)でTEXT以外構造が出力されなかった。
  int         n;
  PPartStruct PSN;
  PPartStruct PS0 = (nPart0 > 0 ? pPS0+nPart0-1 : pPS0);
#endif
#ifdef UPDATE_20150711 // MESSAGE/RFC822の構造出力への対応(入れ子の解析)でTEXT以外構造が出力されなかった。
  int nCloseType = 0;
#endif
#ifdef UPDATE_20230714 // Content-Disposition:ヘッダに"filename="に続き異なる情報が含まれると、結合されて表示されてしまう
  BOOL bna;
  char cn, *pn;
#endif
#ifdef UPDATE_20230714A // Content-Disposition:ヘッダに"filename="に続き異なる情報が含まれると、結合されて表示されてしまう
  BOOL bnb;
  char cnn;
#endif
#ifdef UPDATE_20230714D // Content-Disposition:ヘッダに"filename="に続き異なる情報が含まれると、結合されて表示されてしまう
  char *ps0, *ps1, *ps2, *ps3, *ps4;
#endif
#ifdef UPDATE_20150712 // MIME構造の階層管理に親パート番号を設定できる項目を追加した。
    PPartStruct PS1;

#ifdef UPDATE_20230706_DEBUG // FETCH時のメールデータを事前に一括して読込み検索するフラグを追加
if (bDebug)
{
  printf("nPart0=%d / pPS0=%x / PS=%x\n", nPart, pPS0, PS);
  printf("pLine=%s\n", pLine);
  printf("nLineLen=%d\n", nLineLen);
  printf("pBound0=%s\n", pBound0);
  printf("pBound=%d\n", pBound);
  printf("fp=%x / bExtend=%d\n", fp, bExtend);
}
#endif

    nPart = 0;
    for (PS1 = pPS0; PS1 < PS; PS1++)
	{
      nPart++;
	}
	PS->nPPart = nPart0;
#endif
  strcpy(PS->mBEnc, "\"7bit\""); // デフォルト
  do {  // ヘッダの最後まで
#ifdef UPDATE_20150630 // MESSAGE/RFC822の構造出力への対応
	PS->uHead.QuadPart += strlen(pLine);
#endif
	if (!(*pLine == ' ' || *pLine == '\t')) {  // 別のヘッダー
#ifdef UPDATE_20230714D // Content-Disposition:ヘッダに"filename="に続き異なる情報が含まれると、結合されて表示されてしまう
	  if (bDi)
	  {
//printf("PS->mBDisp=%s\n", PS->mBDisp);
	    ps4 = PS->mBDisp + strlen(PS->mBDisp)-2; // 最後尾 閉じ括弧手前まで
		if (ps0 = strrchr(PS->mBDisp, '(')) 
		{
		  if (ps1 = strstr(ps0, "\"filename"))
		  {
            if (ps2= strstr(ps1, "\" \""))
			{
              if (ps3= strchr(ps2+3, '"'))
			  {
#ifdef UPDATE_20230714E // Content-Disposition:ヘッダの"filename="の項目のみ出力するオプションを追加　デフォルト 0:filenameのみ 1:他含む
			    if (!bDisposition) {
				  *(ps3+1) = '\x0';
				  strcat(ps1, "))");
			      memcpy(ps0+1,(char *)(ps1), strlen(ps1)+1);
				} else 
#endif
				{
                  if (ps4 > (ps3+3))
				  {
				    strtok(ps3, ")");
				    *(ps3+1) = '\x0';
				    strcpy(ps4, " ");
				    strcat(ps4, ps1);
				    memcpy(ps1,(char *)(ps3+2), strlen(ps3+2)+1);
				    strcat(ps1, "))");
				  }
				}
			  }
			}
		  }
		}
	  }
#endif
	  bCT = bDi = FALSE;
	}
	if (!strnicmp(pLine, "Content-ID:", 11)) {
	  p2 = pLine+11;
	  p2 = SKIP_SP_TAB(p2);
	  p3 = strrchr(p2, '<');
	  if (p3) {
        strtok(p3, "\r\n");
#ifdef UPDATE_20180109A //Content-Transfer-Encoding:ヘッダ情報が空白の場合の対策
	    if (*p3 == '\r' || *p3 == '\n')
	      *p3 = '\x0';
#endif
        sprintf(PS->mBID, "\"%s\"", p3); // ボディタイプ
	  }
	} else 
#ifdef UPDATE_20230821 // "Content-Transfer-Encoding:"ヘッダが"Content-Disposition:"の次にあるとメール本文のエンコーディング情報を正しく返せない
	if (!bDi && !strnicmp(pLine, "Content-Disposition:", 20) || bDi && (*pLine==' ' || *pLine =='\t') )
#else
	if (!bDi && !strnicmp(pLine, "Content-Disposition:", 20) || bDi)
#endif
	{
	  if (bDi) {
#ifdef UPDATE_20230714A // Content-Disposition:ヘッダに"filename="に続き異なる情報が含まれると、結合されて表示されてしまう
		cnn = cn;
#endif
        p2 = p;
	    p2 = SKIP_SP_TAB(p2);
#ifdef UPDATE_20230714 // Content-Disposition:ヘッダに"filename="に続き異なる情報が含まれると、結合されて表示されてしまう
	    if (bna && strnicmp(p2, "filename*", 9))
		{
          if (pContext->pfBufp)
	        p = mgets(pLine, nLineLen, &pContext->pfBuff);
	      else
            p = fgets(pLine, nLineLen, fp);
	      continue;
		}
#endif
		if (*p == ' ' || *p == '\t') { // 名称がまたがる場合の処理
		  if ((p4 = strstr(PS->mBDisp, "\")")))
			*p4 = '\x0';
		  else if ((p4 = strstr(PS->mBDisp, ")")))
			*p4 = '\x0';
		  if (p4) {
#ifdef UPDATE_20230714 // Content-Disposition:ヘッダに"filename="に続き異なる情報が含まれると、結合されて表示されてしまう
#ifdef UPDATE_20230714A // Content-Disposition:ヘッダに"filename="に続き異なる情報が含まれると、結合されて表示されてしまう
		    cn = '\x0';
#endif
            if (pn = strpbrk(p2, ";\r\n"))
			{
#ifdef UPDATE_20230714A // Content-Disposition:ヘッダに"filename="に続き異なる情報が含まれると、結合されて表示されてしまう
		      cn = *pn;
			  *pn = '\x0';
			  if (!strnicmp(p2, "filename*", 9))
			  {
				if (!bna && !bnb)
				  cnn = cn;
				else
			      cnn = '\x0';
#ifdef UPDATE_20230714C // Content-Disposition:ヘッダに"filename="に続き異なる情報が含まれると、結合されて表示されてしまう
				if (!strnicmp(p2, "filename*0=", 10) || !strnicmp(p2, "filename*0*=", 11))
				{
				  cnn = '\x0';
				  bna = FALSE;
				  bnb = FALSE;
				} else {
#endif
				bna = FALSE;
				bnb = TRUE;
#ifdef UPDATE_20230714C // Content-Disposition:ヘッダに"filename="に続き異なる情報が含まれると、結合されて表示されてしまう
				}
#endif
			  } else if (strnicmp(p2, "filename", 8)) {
				bnb = FALSE;
			  }
#else
			  if (strnicmp(p2, "filename*", 9) && *pn == ';')
			    bDi = FALSE;
#endif
			}
#ifdef UPDATE_20230714A // Content-Disposition:ヘッダに"filename="に続き異なる情報が含まれると、結合されて表示されてしまう
			//if ((cnn == ';' || cnn == '\r' || cnn == '\n' || (!bnb && !strnicmp(p2, "filename", 8))))
			if (!bnb || bnb && cnn == ';') // && (cnn == ';' || cnn == '\r' || cnn == '\n'))
			{
              do
			  {
		        p2 = SKIP_SP_TAB(p2);
		        if (p3 = strchr(p2, '='))
				{
			      *p3 = '\x0';
				  if (!*p2)
				  {
					*p3 = '=';
			        strcat(PS->mBDisp, " ");
			        strtok(p3, "\"\r\n");
			        strcat(PS->mBDisp, p3);
					if (cn ==';')
                      strcat(PS->mBDisp, "\"");
				    p4 = p4+strlen(p3)+1;
				  } else {
			        p3++;
			        if (*p3=='"')
 			          p3++;
			        strtok(p3, "\"\r\n");
			        cn = '\x0';
			        if (p2) {
			          strcat(PS->mBDisp, "\" \"");
#ifdef UPDATE_20230714C // Content-Disposition:ヘッダに"filename="に続き異なる情報が含まれると、結合されて表示されてしまう
					  if (!strnicmp(p2, "filename*", 9))
//#ifdef UPDATE_20230714D // Content-Disposition:ヘッダに"filename="に続き異なる情報が含まれると、結合されて表示されてしまう
//						strcat(PS->mBDisp, "filename");
//#else
						strcat(PS->mBDisp, "filename*");
//#endif
					  else
#endif
		                strcat(PS->mBDisp, p2);
			          strcat(PS->mBDisp, "\" \"");
			          strtok(p3, "\"\r\n");
			          strcat(PS->mBDisp, p3);
			          //strcat(PS->mBDisp, "\"");
#ifdef UPDATE_20230714C // Content-Disposition:ヘッダに"filename="に続き異なる情報が含まれると、結合されて表示されてしまう
					  p4 = PS->mBDisp+strlen(PS->mBDisp);
#else
				      p4 = p4+strlen(p2)+strlen(p3)+6;
#endif
					}
				  }
				} else {
				  while (*p2 == ';' || *p2 == '\r' || *p2 == '\n')
				    p2++;
				  break;
				}
                if (!strnicmp(p2, "filename*", 9) || !strnicmp(p2, "filename", 8))
				{
		          pn = p2 = p3+strlen(p3);
				  cnn = '\x0';
				  bnb = TRUE;
				  if (!strnicmp(p2, "filename*", 9))
				  {
				    bna = TRUE;
				  }
                  //break;
				}
			    pn = p2 = p3+strlen(p3)+2;
				//pn++;
			  } while(strpbrk(pn, ";\r\n"));
			} else {
			  if (cn == ';' && bnb && strnicmp(p2, "filename*", 9))
				bnb = FALSE;
              cnn = cn;
			}
#endif
			if (strnicmp(p2, "filename*", 9))
#endif
              strtok(p2, "\"\r\n");
#ifdef UPDATE_20091129E // CatchMe@Mailで添付きメールの表示が失敗する
			if (!strnicmp(p2, "filename*", 9))
			{
			  p2 = strchr(p2, '=');
			  p2++;
#ifdef UPDATE_20230714 // Content-Disposition:ヘッダに"filename="に続き異なる情報が含まれると、結合されて表示されてしまう
			  while(*p2=='"')
				p2++;
#endif
			}
			strtok(p2, ";\"\r\n");
#endif
#ifdef UPDATE_20150615A // BODYSTRUCTUREでヘッダ構造に誤った解析結果を出力する場合があった
			if (!strncmp(p2, "=?", 2))
			{
              sprintf(p4, " %s\"))", p2);
			} else
#endif
			{
              sprintf(p4, "%s\"))", p2);
			}
		  }
		} else {
		  bDi = FALSE;
		}
	  } else {
#ifdef UPDATE_20230714 // Content-Disposition:ヘッダに"filename="に続き異なる情報が含まれると、結合されて表示されてしまう
        bna = FALSE;
#endif
#ifdef UPDATE_20230714A // Content-Disposition:ヘッダに"filename="に続き異なる情報が含まれると、結合されて表示されてしまう
		bnb = FALSE;
#endif
		bDi = TRUE;
	    p2 = pLine+20;
	    p2 = SKIP_SP_TAB(p2);
	    p3 = strchr(p2, ';');
	    if (p3) {
	      *p3++ = '\x0';
          sprintf(PS->mBDisp,"(\"%s\"", p2); // ボディ記述
#ifdef UPDATE_20150618  // HEADER(Content-Disposition:)構造のattachmentの続きに半角スペースだけあると正しく抽出できない不具合
          while (*p3 == ' ' || *p3 == '\t')
		  {
            p3++;
		  }
#endif
          if (*p3 == '\r' || *p3 == '\n') { // 次レコード読込
#ifdef UPDATE_20230706 // FETCH時のメールデータを事前に一括して読込み検索するフラグを追加
            if (pContext->pfBufp)
              p3 = mgets(pLine, nLineLen, &pContext->pfBuff);
            else
#endif
            p3 = fgets(pLine, nLineLen, fp);
		    if (p3 == NULL ||
				!(*p3 == '\t' || *p3 == ' '))
			  break;
		  }
          p2 = p3;
		  p2 = SKIP_SP_TAB(p2);
		  p3 = strchr(p2, '=');
		  if (p3) 
		  {
		    *p3++ = '\x0';
#ifdef UPDATE_20150620A // HEADER(Content-Type:)構造でboundaryの文字列の後ろに空白がある場合に正しく抽出できない不具合
		    while(*p3 == ' ' || *p3 == '\t')
			{
			  p3++;
			}
#endif
		    if (p3) 
			{
#ifdef UPDATE_20150618B  // HEADER(Content-Type:)構造でnameに続くファイル名が改行後に記述されている正しく抽出できない不具合
			  if (*p3 == '\r' || *p3 == '\n')
			  {
			    *p3 = '\x0';
			  }
#endif
#ifdef UPDATE_20230714 // Content-Disposition:ヘッダに"filename="に続き異なる情報が含まれると、結合されて表示されてしまう
			  cn = '\x0';
              if (pn = strpbrk(p3, ";\r\n"))
			  {
			    cn = *pn;
				*pn = '\x0';
#ifdef UPDATE_20230714A // Content-Disposition:ヘッダに"filename="に続き異なる情報が含まれると、結合されて表示されてしまう
				strtok(p3, "\"");
			    if (!strnicmp(p2, "filename*", 9) && cn == ';')
				  cn = '\x0';
#else
			    if (strnicmp(p2, "filename*", 9) && cn == ';')
				  bDi = FALSE;
#endif
			  }
#else
              strtok(p3, ";\r\n");
#endif
		      /////////// ボディ記述
		      strcat(PS->mBDisp, " (\"");
#ifdef UPDATE_20230714A // Content-Disposition:ヘッダに"filename="に続き異なる情報が含まれると、結合されて表示されてしまう
              while(pn && p3)
			  {
#endif
#ifdef UPDATE_20091129E // CatchMe@Mailで添付きメールの表示が失敗する
			  if (!strnicmp(p2, "filename*", 9))
			  {
				*(p2+9) = '\x0';
#ifdef UPDATE_20230714 // Content-Disposition:ヘッダに"filename="に続き異なる情報が含まれると、結合されて表示されてしまう
                bna = TRUE;
#endif
#ifdef UPDATE_20230714A // Content-Disposition:ヘッダに"filename="に続き異なる情報が含まれると、結合されて表示されてしまう
		        bnb = TRUE;
#endif
			  }
#ifdef UPDATE_20230714A // Content-Disposition:ヘッダに"filename="に続き異なる情報が含まれると、結合されて表示されてしまう
			  if (!strnicmp(p2, "filename", 8))
                bnb = TRUE;
#endif
//#ifdef UPDATE_20230714D // Content-Disposition:ヘッダに"filename="に続き異なる情報が含まれると、結合されて表示されてしまう
//			  if (!strnicmp(p2, "filename*", 9))
//				strcat(PS->mBDisp, "filename");
//			  else
//#endif
			    strcat(PS->mBDisp, p2);
#else
			  strcat(PS->mBDisp, p2);
#endif
#ifdef UPDATE_20091129B // CatchMe@Mailで添付きメールの表示が失敗する
			  strcat(PS->mBDisp, "\"");
			  {
			    CHAR mLen[80];
			    if (strchr(p3+1, '"'))
				{
				  p3++;
				  strtok(p3,"\"");
				}
			    sprintf(mLen, "{%d}\r\n", strlen(p3));
			    strcat(PS->mBDisp, mLen);
			  }
#else
			  strcat(PS->mBDisp, "\" ");
			  if (!strchr(p3+1, '"'))
				strcat(p3, "\"");
#endif
#ifdef UPDATE_20091129E // CatchMe@Mailで添付きメールの表示が失敗する
			  if (*p3 != '"')
			  {
			    strcat(PS->mBDisp, "\"");
			  }
#endif
			  strcat(PS->mBDisp, p3);
#ifdef UPDATE_20230714A // Content-Disposition:ヘッダに"filename="に続き異なる情報が含まれると、結合されて表示されてしまう
			  p2 = pn+1;
			  //pn = strchr(p2, ';');
#ifdef UPDATE_20230714B // Content-Disposition:ヘッダに"filename="に続き異なる情報が含まれると、結合されて表示されてしまう
              do
			  {
#endif
		        p2 = SKIP_SP_TAB(p2+1);
		        if (p3 = strchr(p2, '='))
				{
				  *p3 = '\x0';
#ifdef UPDATE_20230714B // Content-Disposition:ヘッダに"filename="に続き異なる情報が含まれると、結合されて表示されてしまう
				  if (!*p2)
				  {
					*p3 = '=';
			        strcat(PS->mBDisp, "\" \"");
			        strtok(p3, "\"\r\n");
			        strcat(PS->mBDisp, p3);
				    p4 = p4+strlen(p3)+3;
				  } else {
#endif
				    p3++;
				    if (*p3=='"')
				      p3++;
#ifdef UPDATE_20230714B // Content-Disposition:ヘッダに"filename="に続き異なる情報が含まれると、結合されて表示されてしまう
                    strtok(p3, "\"\r\n");
#endif
			        cn = '\x0';
#ifdef UPDATE_20230714B // Content-Disposition:ヘッダに"filename="に続き異なる情報が含まれると、結合されて表示されてしまう
			        if (p2) {
					  if (PS->mBDisp[strlen(PS->mBDisp)-1]== '"')
						strcat(PS->mBDisp, " \"");
					  else
 			            strcat(PS->mBDisp, "\" \"");
			          strcat(PS->mBDisp, p2);
                      if (strnicmp(p2, "filename", 8))
					  {
			            strcat(PS->mBDisp, "\" \"");
			            strtok(p3, "\"\r\n");
			            strcat(PS->mBDisp, p3);
			            //strcat(PS->mBDisp, "\"");
				        p4 = p4+strlen(p2)+strlen(p3)+6;
					  }
					} else {
			          while (*p2 == ';' || *p2 == '\r' || *p2 == '\n')
			            p2++;
			          break;
					}
				  }
#endif
                  if (pn = strpbrk(p3, ";\r\n"))
				  {
			        cn = *pn;
				    *pn = '\x0';
			        //if (strnicmp(p2, "filename*", 9) && cn == ';')
				    //  bDi = FALSE;
				    pn++;
				  }
				}
#ifdef UPDATE_20230714B // Content-Disposition:ヘッダに"filename="に続き異なる情報が含まれると、結合されて表示されてしまう
                if (!strnicmp(p2, "filename*", 9) || !strnicmp(p2, "filename", 8))
				{
		          pn = p2 = p3+strlen(p3);
			      cnn = '\x0';
			      bnb = TRUE;
			      if (!strnicmp(p2, "filename*", 9))
				  {
			        bna = TRUE;
				  }
                  break;
				}
				if (p3)
			      pn = p2 = p3+strlen(p3)+1;
				else
				  break;
			  } while(strpbrk(pn, ";\r\n"));
			  if (!bnb && p3)
#else
			  if (p3)
#endif
				strcat(PS->mBDisp, " \"");
			  }
#endif
#ifdef UPDATE_20230714B // Content-Disposition:ヘッダに"filename="に続き異なる情報が含まれると、結合されて表示されてしまう
			  if (PS->mBDisp[strlen(PS->mBDisp)-1] != '"')
			 	strcat(PS->mBDisp, "\"");
#endif
			  strcat(PS->mBDisp, ")");
			  /////////////////////
			}
		  }
		  strcat(PS->mBDisp, ")");
		}
	  }
	} else if (!strnicmp(pLine, "Content-Transfer-Encoding:", 26)) { // 7bit
      p2 = pLine+26;
	  p2 = SKIP_SP_TAB(p2);
	  strtok(p2, "\r\n");
#ifdef UPDATE_20180109A //Content-Transfer-Encoding:ヘッダ情報が空白の場合の対策
	  if (*p2 == '\r' || *p2 == '\n')
	    *p2 = '\x0';
#endif
  	  sprintf(PS->mBEnc, "\"%s\"", p2);
	} else if (!bCT && !strnicmp(pLine, "Content-Type:", 13) ||
		       bCT) {
#ifdef UPDATE_20231013 // Content-Type:ヘッダに続くデータが改行のみしか無い場合、添付ファイルと認識できない不具合
	  if (!strnicmp(pLine, "Content-Type:", 13))
	  {
		if (!strstr((pLine+13), ";"))
		{
#ifndef UPDATE_20231025 //Content-Type:ヘッダに続く次行が改行のみしか無い場合、添付ファイルと認識できない不具合
		  strtok(pLine, "\r\n");
#endif
#ifdef UPDATE_20231025 //Content-Type:ヘッダに続く次行が改行のみしか無い場合、添付ファイルと認識できない不具合
		  p6 = pLine+13;
		  while(*p6 == ' ' || *p6 == '\t')
		    p6++;
		  if (*p6 == '\r' || *p6 == '\n')
#endif
#ifdef UPDATE_20231024 //Content-Type:ヘッダに続く次行が改行のみしか無い場合、添付ファイルと認識できない不具合
		  {
	        long ctcur;
		    CHAR *pctcur;

			if (pContext->pfBufp)
			  pctcur = pContext->pfBuff;
			else
			  ctcur = ftell(fp);
#endif
#ifdef UPDATE_20231025 //Content-Type:ヘッダに続く次行が改行のみしか無い場合、添付ファイルと認識できない不具合
		  strtok(pLine, "\r\n");
#endif
#ifdef UPDATE_20230706 // FETCH時のメールデータを事前に一括して読込み検索するフラグを追加
          if (pContext->pfBufp)
	        p = mgets((pLine+strlen(pLine)), nLineLen, &pContext->pfBuff);
	      else
#endif
          p = fgets((pLine+strlen(pLine)), nLineLen, fp);
#ifdef UPDATE_20231024 //Content-Type:ヘッダに続く次行が改行のみしか無い場合、添付ファイルと認識できない不具合
		    if (p)
			{
			  if (*p == '\r' || *p == '\n')
			  {
		        if (pContext->pfBufp)
				  pContext->pfBuff = pctcur;
			    else
				  fseek(fp, ctcur, SEEK_SET); // 現在位置から開始バイト位置指定
			  }
			}
		  }
#endif
		}
	  }
#endif
	  if (bCT) 
	  {
        p2 = p;
	    p2 = SKIP_SP_TAB(p2);
        p3 = strchr(p2, '=');
#ifdef UPDATE_20150618B  // HEADER(Content-Type:)構造でnameに続くファイル名が改行後に記述されている正しく抽出できない不具合
		if (p3 && !(*p == ' ' || *p == '\t')) // || !strcmp(p2, "name=")))
#else
	    if (p3 && !(*p == '\x20' || *p == '\x08')) 
#endif
		{
	      *p3++ = '\x0';
#ifdef UPDATE_20150620A // HEADER(Content-Type:)構造でboundaryの文字列の後ろに空白がある場合に正しく抽出できない不具合
	      while(*p3 == ' ' || *p3 == '\t')
		  {
		    p3++;
		  }
#endif
		  if (p3) {
            strtok(p3, ",;\r\n");
			//// "ダブルクォーテーションがある場合は一旦削除
		    if (*p3 == '"') {
		      p3++;
		      strtok(p3, "\"");
			}
#ifdef UPDATE_20150620A // HEADER(Content-Type:)構造でboundaryの文字列の後ろに空白がある場合に正しく抽出できない不具合
			strtok(p2, " \t");
#endif
			sprintf(PS->mBPara,"(\"%s\" \"%s\")", p2, p3); // ボディパラメータ括弧つきリスト
			if (!stricmp(p2, "boundary")) { // マルチパートなら処理
	          sprintf(pBound, "--%s", p3);
	          strtok(pBound, "\"");
			  bCT = FALSE;
			}
		  }
		} else if (p3) {  // 名称がまたがる場合の処理
		  if ((p4 = strstr(PS->mBPara, "\")")))
			*p4 = '\x0';
		  else if ((p4 = strstr(PS->mBPara, ")")))
			*p4 = '\x0';
		  if (p4) {
            strtok(p3, "\"\r\n");
            sprintf(p4, "%s\")", p3);
		  }
		} 
	  } else {
#ifdef UPDATE_20150630C // MESSAGE/RFC822の構造出力への対応
        bCType = TRUE;
#endif
        bCT = TRUE;
        p2 = pLine+13;
	    p2 = SKIP_SP_TAB(p2);
	    p3 = strchr(p2, '/');
#ifdef UPDATE_20150618C  // HEADER(Content-Type:)構造でmultipart/alternativeの入れ子構造が正しく抽出できない不具合
		if (!p3)
		{
		  p3 = strchr(p2, ';');
		}
#endif
		if (p3) {
	      *p3++ = '\x0';
#ifdef UPDATE_20150618C  // HEADER(Content-Type:)構造でmultipart/alternativeの入れ子構造が正しく抽出できない不具合
#ifdef UPDATE_20150709 // MESSAGE/RFC822の構造出力への対応(入れ子の解析)でTEXT以外構造が出力されなかった。
#ifdef UPDATE_20150710 // MESSAGE/RFC822の構造出力への対応(入れ子の解析)でTEXT以外構造が出力されなかった。
#ifdef UPDATE_20230706 // FETCH時のメールデータを事前に一括して読込み検索するフラグを追加
		  if ((!stricmp(p2, "multipart") || (!stricmp(p2, "message") && !strnicmp(p3,"rfc822", 6))) && (p || (pContext->pfBuff ? *pContext->pfBuff : !feof(fp))))
#else
		  if ((!stricmp(p2, "multipart") || (!stricmp(p2, "message") && !strnicmp(p3,"rfc822", 6))) && !feof(fp))
#endif
#else
		  if ((!stricmp(p2, "multipart") || !stricmp(p2, "message")) && !feof(fp))
#endif
#else
#ifdef UPDATE_20150620B // 多重入れ子構造がある場合に正しく抽出できない不具合
		  if (!stricmp(p2, "multipart") && !feof(fp))
#else
		  if (!stricmp(p2, "multipart") && !strnicmp(p3, "alternative", 11))
#endif
#endif
		  {
#ifndef UPDATE_20150712 // MIME構造の階層管理に親パート番号を設定できる項目を追加した。
#ifdef UPDATE_20150708 // MIME構造で(multipart(text,multipart(html))のような構造で順番道理の結果が返せない不具合。
#ifdef UPDATE_20150709 // MESSAGE/RFC822の構造出力への対応(入れ子の解析)でTEXT以外構造が出力されなかった。
#ifdef UPDATE_20150710 // MESSAGE/RFC822の構造出力への対応(入れ子の解析)でTEXT以外構造が出力されなかった。
			  if (PS0->uBody.QuadPart+PS0->uHead.QuadPart > 0 && (!strnicmp(p3, "mixed", 5) || !stricmp(p2, "message")) )
#else
			  if (!stricmp(PS0->mBType, "\"text\"") && PS0->uBody.QuadPart+PS0->uHead.QuadPart > 0 && (!strnicmp(p3, "mixed", 5) || !stricmp(p2, "message")) )
#endif
#else
			if (!stricmp(PS0->mBType, "\"text\"") && PS0->uBody.QuadPart+PS0->uHead.QuadPart > 0 && !strnicmp(p3, "mixed", 5) )
#endif
			{
			  char  mBody[32], mRec[32];
#ifdef UPDATE_20150710 // MESSAGE/RFC822の構造出力への対応(入れ子の解析)でTEXT以外構造が出力されなかった。
               for (n = 1; n < nPart0; n++)
			   {
				   PSN = pPS0+n;
		  	  sprintf(mBody, "%lu", PSN->uBody.QuadPart);
	          sprintf(mRec, "%lu", PSN->nLine);

	          if (bExtend) // BODYSTRUCTURE
                sprintf(pContext->mMessages, "(%s %s %s %s %s %s %s %s%s %s %s %s)%s",
                                    (PSN->mBType[0] ? strupr(PSN->mBType): "NIL"),
                                    (PSN->mBSub[0] ? strupr(PSN->mBSub): "NIL"),
                                    (PSN->mBPara[0] ? PSN->mBPara: "NIL"),
                                    (PSN->mBID[0] ? PSN->mBID: "NIL"),
                                    "NIL",
					  			    (PSN->mBEnc[0] ? PSN->mBEnc: "NIL"),
                                   (PSN->uBody.QuadPart ? mBody: "NIL"),
                                   (!stricmp(PSN->mBType, "\"text\"") ? (PSN->nLine ? mRec: "NIL") : ""),
                                   (!stricmp(PSN->mBSub, "\"rfc822\"") ? "(NIL NIL NIL NIL NIL NIL NIL NIL NIL NIL) (NIL NIL NIL NIL NIL NIL NIL NIL NIL NIL NIL) NIL NIL" : (!stricmp(PSN->mBType, "\"text\"") ? " NIL" : "NIL")),
                                   (PSN->mBDisp[0] ? PSN->mBDisp: "NIL"),
								   "NIL",
                                   "NIL",
								   "");
	          else         // BODY
				sprintf(pContext->mMessages, "(%s %s %s %s %s %s %s %s)%s",
                                    (PSN->mBType[0] ? strupr(PSN->mBType): "NIL"),
                                    (PSN->mBSub[0] ? strupr(PSN->mBSub): "NIL"),
                                    (PSN->mBPara[0] ? PSN->mBPara: "NIL"),
                                    (PSN->mBID[0] ? PSN->mBID: "NIL"),
                                    "NIL",
					  			    (PSN->mBEnc[0] ? PSN->mBEnc: "NIL"),
                                   (PSN->uBody.QuadPart ? mBody: "NIL"),
                                   (!stricmp(PSN->mBType, "\"text\"") ? (PSN->nLine ? mRec: "NIL") : ""),
								   "");
              put_reply(lpClientContext, FALSE, TRUE);

			   }
#else
		  	  sprintf(mBody, "%lu", PS0->uBody.QuadPart);
	          sprintf(mRec, "%lu", PS0->nLine);

	          if (bExtend) // BODYSTRUCTURE
                sprintf(pContext->mMessages, "(%s %s %s %s %s %s %s %s%s %s %s %s)%s",
                                    (PS0->mBType[0] ? strupr(PS0->mBType): "NIL"),
                                    (PS0->mBSub[0] ? strupr(PS0->mBSub): "NIL"),
                                    (PS0->mBPara[0] ? PS0->mBPara: "NIL"),
                                    (PS0->mBID[0] ? PS0->mBID: "NIL"),
                                    "NIL",
					  			    (PS0->mBEnc[0] ? PS0->mBEnc: "NIL"),
                                   (PS0->uBody.QuadPart ? mBody: "NIL"),
                                   (!stricmp(PS0->mBType, "\"text\"") ? (PS0->nLine ? mRec: "NIL") : ""),
                                   (!stricmp(PS0->mBSub, "\"rfc822\"") ? "(NIL NIL NIL NIL NIL NIL NIL NIL NIL NIL) (NIL NIL NIL NIL NIL NIL NIL NIL NIL NIL NIL) NIL NIL" : (!stricmp(PS0->mBType, "\"text\"") ? " NIL" : "NIL")),
                                   (PS0->mBDisp[0] ? PS0->mBDisp: "NIL"),
								   "NIL",
                                   "NIL",
								   "");
	          else         // BODY
				sprintf(pContext->mMessages, "(%s %s %s %s %s %s %s %s)%s",
                                    (PS0->mBType[0] ? strupr(PS0->mBType): "NIL"),
                                    (PS0->mBSub[0] ? strupr(PS0->mBSub): "NIL"),
                                    (PS0->mBPara[0] ? PS0->mBPara: "NIL"),
                                    (PS0->mBID[0] ? PS0->mBID: "NIL"),
                                    "NIL",
					  			    (PS0->mBEnc[0] ? PS0->mBEnc: "NIL"),
                                   (PS0->uBody.QuadPart ? mBody: "NIL"),
                                   (!stricmp(PS0->mBType, "\"text\"") ? (PS0->nLine ? mRec: "NIL") : ""),
								   "");
              put_reply(lpClientContext, FALSE, TRUE);
			  //PS = PS0;
              nPart = -1;
#ifdef UPDATE_20150711 // MESSAGE/RFC822の構造出力への対応(入れ子の解析)でTEXT以外構造が出力されなかった。
			  nCloseType = 0;
#endif
#endif   
			}
#endif
#endif
			 // 入れ子処理
			 *(p3-1) = '/';
#ifdef UPDATE_20230706 // FETCH時のメールデータを事前に一括して読込み検索するフラグを追加
             if (pContext->pfBufp)
		       pContext->pfBuff -=(long)strlen(pLine);
	         else
#endif
			 fseek(fp, (long)(0-strlen(pLine)), SEEK_CUR); // １行手前にもどる
#ifdef UPDATE_20150709 // MESSAGE/RFC822の構造出力への対応(入れ子の解析)でTEXT以外構造が出力されなかった。
#ifdef UPDATE_20150712 // MIME構造の階層管理に親パート番号を設定できる項目を追加した。
			 if (!strnicmp(p2, "message/", 8) && !strnicmp(p3, "rfc822", 6) )
#else
			 if (nPart == -1 && !strnicmp(p2, "message/", 8) )
#endif
			 {
			   //if (!strnicmp(p3, "rfc822", 6))
			   {
#ifdef UPDATE_20150712 // MIME構造の階層管理に親パート番号を設定できる項目を追加した。
                 nPart = message_bodystructure(lpClientContext, &nPart, pPS0, pLine, fp, pBound0, bExtend);
#else
#ifdef UPDATE_20150711 // MESSAGE/RFC822の構造出力への対応(入れ子の解析)でTEXT以外構造が出力されなかった。
                 nCloseType = message_bodystructure(lpClientContext, pLine, fp, pBound0, bExtend);
#else
#ifdef UPDATE_20150710 // MESSAGE/RFC822の構造出力への対応(入れ子の解析)でTEXT以外構造が出力されなかった。
			     message_bodystructure(lpClientContext, pLine, fp, pBound0, bExtend);
#else
			     message_bodystructure(lpClientContext, pLine, fp, bExtend);
#endif
#endif
#endif
			   } 
#ifndef UPDATE_20150712 // MIME構造の階層管理に親パート番号を設定できる項目を追加した。
#ifdef UPDATE_20150711 // MESSAGE/RFC822の構造出力への対応(入れ子の解析)でTEXT以外構造が出力されなかった。
			   nPart = 0; 
#endif
#endif
			 } else 
#endif
#ifdef UPDATE_20150712 // MIME構造の階層管理に親パート番号を設定できる項目を追加した。
             nPart = multipart_bodystructure(lpClientContext, &nPart, pPS0, pLine, fp, pBound0, bExtend, TRUE);
#else
#ifdef UPDATE_20150710 // MESSAGE/RFC822の構造出力への対応(入れ子の解析)でTEXT以外構造が出力されなかった。
			 multipart_bodystructure(lpClientContext, pLine, fp, pBound0, bExtend, TRUE);
#else
#ifdef UPDATE_20150709 // MESSAGE/RFC822の構造出力への対応(入れ子の解析)でTEXT以外構造が出力されなかった。
			 multipart_bodystructure(lpClientContext, pLine, fp, bExtend, TRUE);
#else
			 multipart_bodystructure(lpClientContext, pLine, fp, bExtend);
#endif
#endif
#endif
			 do
			 {
#ifdef UPDATE_20230706 // FETCH時のメールデータを事前に一括して読込み検索するフラグを追加
               if (pContext->pfBufp)
                 p = mgets(pLine, nLineLen, &pContext->pfBuff);
               else
#endif
			   p = fgets(pLine, nLineLen, fp);
#ifdef UPDATE_20230706 // FETCH時のメールデータを事前に一括して読込み検索するフラグを追加
			 } while ((p || (pContext->pfBuff ? *pContext->pfBuff : !feof(fp))) && (*pLine == '\x0' || *pLine == '\r' || *pLine == '\n'));
#else
			 } while (!feof(fp) && (*pLine == '\x0' || *pLine == '\r' || *pLine == '\n'));
#endif
#ifdef UPDATE_20150620B // 多重入れ子構造がある場合に正しく抽出できない不具合
			 //if (strstr(pLine, pBound0) && strstr(pLine+strlen(pBound0),"--"))
			 if (!strncmp(pLine, pBound0, strlen(pBound0)) && !strncmp(pLine+strlen(pBound0), "--", 2))
			 {
#ifdef UPDATE_20230706 // FETCH時のメールデータを事前に一括して読込み検索するフラグを追加
               if (pContext->pfBufp)
                 pContext->pfBuff -= strlen(pLine); // １行手前にもどる
               else
#endif
               fseek(fp, (long)(0-strlen(pLine)), SEEK_CUR); // １行手前にもどる
			   break;
			 }
#endif
#ifdef UPDATE_20230706 // FETCH時のメールデータを事前に一括して読込み検索するフラグを追加
             if (pContext->pfBuff ? !(*pContext->pfBuff) : feof(fp))
#else
			 if (feof(fp))
#endif
			 {
			   break;
			 } else {
#ifdef UPDATE_20150712 // MIME構造の階層管理に親パート番号を設定できる項目を追加した。
               nPart++;
			   PS = pPS0+nPart;
#endif
			   continue;
			 }
			 //*(p3-1) = '\x0';
		  }
#endif
          sprintf(PS->mBType, "\"%s\"", p2); // ボディタイプ
          p2 = p3;
	      p3 = strchr(p2, ';');
#ifdef UPDATE_20150709 // MESSAGE/RFC822の構造出力への対応(入れ子の解析)でTEXT以外構造が出力されなかった。
		  if (!p3) // 行にセミコロンがないときの対処
		  {
	        p3 = strpbrk(p2, " \t\r\n");
		  }
#endif
	      if (p3) {
#ifdef UPDATE_20150617 // HEADER(Content-Type:)構造が１行内に複数の情報がセミコロンで区切られていると抽出できない不具合
		    if ((p = strchr(p3+1, ';')))
			{
			  if (strchr(p,'='))
			  {
			    p++;
			  } else {
			    *p = '\x0';
			  }
			}
#endif
	        *p3++ = '\x0';
#ifdef UPDATE_20150618B  // HEADER(Content-Type:)構造でnameに続くファイル名が改行後に記述されている正しく抽出できない不具合
			p3 = SKIP_SP_TAB(p3);
#endif
            sprintf(PS->mBSub,"\"%s\"", p2); // ボディサブタイプ
            if (*p3 == '\r' || *p3 == '\n') { // 次レコード読込
#ifdef UPDATE_20230706 // FETCH時のメールデータを事前に一括して読込み検索するフラグを追加
		      if (pContext->pfBufp)
		        p3 = mgets(pLine, nLineLen, &pContext->pfBuff);
		      else
#endif
              p3 = fgets(pLine, nLineLen, fp);
 			  if ((p3 == NULL) ||
				 !(*p3 == '\t' || *p3 == ' '))
#ifdef UPDATE_20150630B // FETCH N BODYSTRUCTでContent-Transfer-Encoding: base64の時の応答結果が"7bit"になってしまう場合がある不具合
               continue;
#else
			   break;
#endif
			}
            p2 = p3;
	        p2 = SKIP_SP_TAB(p2);
	        p3 = strchr(p2, '=');
	        if (p3) {
	          *p3++ = '\x0';
#ifdef UPDATE_20150620A // HEADER(Content-Type:)構造でboundaryの文字列の後ろに空白がある場合に正しく抽出できない不具合
		      while(*p3 == ' ' || *p3 == '\t')
			  {
			    p3++;
			  }
#endif
#ifdef UPDATE_20150620A // HEADER(Content-Type:)構造でboundaryの文字列の後ろに空白がある場合に正しく抽出できない不具合
			  strtok(p2, " \t");
#endif
		      if (p3) {
#ifdef UPDATE_20150618B  // HEADER(Content-Type:)構造でnameに続くファイル名が改行後に記述されている正しく抽出できない不具合
				if (*p3 == '\r' || *p3 == '\n')
				{
				   *p3 = '\x0';
				}
#endif
                strtok(p3, ";,\r\n");
                //sprintf(PS->mBPara,"(\"%s\" %s)", p2, p3); // ボディパラメータ括弧つきリスト
			    if (*p3 == '"') {
#ifdef UPDATE_20150620  // HEADER(Content-Type:)構造でname=""の場合に正しく抽出できない不具合
                  do
				  {
					p3++;
				  } while (*p3 == '"');
#else
			      p3++;
#endif
			      strtok(p3, "\"");
				}
#ifdef UPDATE_20091129B // CatchMe@Mailで添付きメールの表示が失敗する
                if (!strcmp(p2, "name"))
				{
			      sprintf(PS->mBPara,"(\"%s\" {%d}\r\n%s)", p2, strlen(p3), p3); // ボディパラメータ括弧つきリスト
				} else 
#endif
#ifdef UPDATE_20150604 // 添付ファイルの名称が複数行にまたがっている場合の処理
			    //if (!strcmp(p2, "name") ) //|| !strcmp(p2, "filename"))
				{
#ifdef UPDATE_20150618B  // HEADER(Content-Type:)構造でnameに続くファイル名が改行後に記述されている正しく抽出できない不具合
				  if (strlen(p3) == 0)
				  {
			        sprintf(PS->mBPara,"(\"%s\" \"" , p2); // ボディパラメータ括弧つきリスト
				  } else 
#endif
				  {
			        sprintf(PS->mBPara,"(\"%s\" \"%s", p2, p3); // ボディパラメータ括弧つきリスト
				  }
#ifdef UPDATE_20150617 // HEADER(Content-Type:)構造が１行内に複数の情報がセミコロンで区切られていると抽出できない不具合
                  if (!p)
				  {
#ifdef UPDATE_20230706 // FETCH時のメールデータを事前に一括して読込み検索するフラグを追加
		            if (pContext->pfBufp)
		              p = mgets(pLine, nLineLen, &pContext->pfBuff);
		            else
#endif
					p = fgets(pLine, nLineLen, fp);
				  } else {
					if (strchr(p, '='))
					{
					  memcpy(pLine, p, strlen(p)+1);
					  p = pLine;
					} else {
#ifdef UPDATE_20230706 // FETCH時のメールデータを事前に一括して読込み検索するフラグを追加
		            if (pContext->pfBufp)
		              p = mgets(pLine, nLineLen, &pContext->pfBuff);
		            else
#endif
					  p = fgets(pLine, nLineLen, fp);
					}
				  }
				  while(p)
#else
				  while((p = fgets(pLine, nLineLen, fp)))
#endif
				  {
	                if (!(*pLine == ' ' || *pLine == '\t')) {  // 別のヘッダー
				      break;
					}
		            p3 = p;
	                p3 = SKIP_SP_TAB(p3);
#ifdef UPDATE_20150615A // BODYSTRUCTUREでヘッダ構造に誤った解析結果を出力する場合があった
					p5 = strchr(p3+1, '=');
#ifdef UPDATE_20150618B  // HEADER(Content-Type:)構造でnameに続くファイル名が改行後に記述されている正しく抽出できない不具合
					if (!p5)
					{
		              strtok(p3, "\"\r\n");
					  strcat(PS->mBPara, p3);
					} else if (*p5 && !(*p3=='=' && *(p3+1)== '?') && *p3 != '"')
#else
				    if (*p5 && !(*p3=='=' && *(p3+1)== '?'))
#endif
					{
	                  *p5++ = '\x0';
		              if (p5) {
                        strtok(p5, ";,\r\n");
			            if (*p5 == '"') {
 			              p5++;
			              strtok(p5, "\"");
						}
						sprintf(&PS->mBPara[strlen(PS->mBPara)],"\" \"%s\" \"%s", p3, p5); // ボディパラメータ括弧つきリスト
			            if (*p5 == '"') {
			              p5++;
			              strtok(p5, "\"");
						}
					  }
					} else 
#endif
					{
		              //p3 = p;
	                  //p3 = SKIP_SP_TAB(p3);
			          if (*p3 == '"') {
 			            p3++;
			            strtok(p3, "\"");
					  }
					  strtok(p3, "\r\n");
#ifdef UPDATE_20150615A // BODYSTRUCTUREでヘッダ構造に誤った解析結果を出力する場合があった
					  if (*(p3+strlen(p3)-1) == '"')
					  {
						*(p3+strlen(p3)-1) = '\x0';
					  }
#ifdef UPDATE_20150618B  // HEADER(Content-Type:)構造でnameに続くファイル名が改行後に記述されている正しく抽出できない不具合
					  if (!strncmp(p3, "=?", 2) && PS->mBPara[strlen(PS->mBPara)-1] != '"')
#else
					  if (!strncmp(p3, "=?", 2))
#endif
					  {
                        strcat(PS->mBPara, " ");
					  }
#endif
                      strcat(PS->mBPara, p3);
					}
#ifdef UPDATE_20150617 // HEADER(Content-Type:)構造が１行内に複数の情報がセミコロンで区切られていると抽出できない不具合
#ifdef UPDATE_20230706 // FETCH時のメールデータを事前に一括して読込み検索するフラグを追加
		            if (pContext->pfBufp)
		              p = mgets(pLine, nLineLen, &pContext->pfBuff);
		            else
#endif
					p = fgets(pLine, nLineLen, fp);
#endif
				  }
				  strcat(PS->mBPara, "\")");
#ifdef UPDATE_20150630 // MESSAGE/RFC822の構造出力への対応
		  if (!strcmp(PS->mBType, "\"message\"") && !strcmp(PS->mBSub, "\"rfc822\""))
		  {
#ifdef UPDATE_20230706 // FETCH時のメールデータを事前に一括して読込み検索するフラグを追加
	        if (pContext->pfBufp)
              cur = (long)(pContext->pfBuff - pContext->pfBufp);
	        else
#endif
			cur = ftell(fp);
#ifdef UPDATE_20230706_DEBUG // FETCH時のメールデータを事前に一括して読込み検索するフラグを追加
if (bDebug) printf("multipart_header(0) cur=%lu\n", cur);
#endif
#ifdef UPDATE_20150630C // MESSAGE/RFC822の構造出力への対応
			do
			{
#ifdef UPDATE_20230706 // FETCH時のメールデータを事前に一括して読込み検索するフラグを追加
		      if (pContext->pfBufp)
		        p = mgets(pLine, nLineLen, &pContext->pfBuff);
		      else
#endif
			  p = fgets(pLine, nLineLen, fp);
#ifdef UPDATE_20230706 // FETCH時のメールデータを事前に一括して読込み検索するフラグを追加
			 } while ((p || (pContext->pfBuff ? *pContext->pfBuff : !feof(fp))) && !(*pLine == '\x0' || *pLine == '\r' || *pLine == '\n'));
#else
			}  while (!feof(fp) && !(*pLine == '\x0' || *pLine == '\r' || *pLine == '\n'));
#endif
#endif
			do
			{
#ifdef UPDATE_20230706 // FETCH時のメールデータを事前に一括して読込み検索するフラグを追加
		      if (pContext->pfBufp)
		        p = mgets(pLine, nLineLen, &pContext->pfBuff);
		      else
#endif
			  p = fgets(pLine, nLineLen, fp);
#ifdef UPDATE_20230706 // FETCH時のメールデータを事前に一括して読込み検索するフラグを追加
			} while ((p || (pContext->pfBuff ? *pContext->pfBuff : !feof(fp))) && (*pLine == '\x0' || *pLine == '\r' || *pLine == '\n'));
#else
			} while (!feof(fp) && (*pLine == '\x0' || *pLine == '\r' || *pLine == '\n'));
#endif
            ////////////////////
			// 初期化
			strcpy(PS->PSM.mBType, "\"TEXT\""); // ボディタイプ Content-Type: text/plain
            strcpy(PS->PSM.mBSub, "\"PLAIN\""); // ボディサブタイプ 
            PS->PSM.mBPara[0] = '\x0';          // ボディパラメータ括弧つきリスト ("name" "test.bin")
	        PS->PSM.mBID[0] = '\x0';            // ボディ ID Content-ID: <Mm3V1n13r86nICg78>
            PS->PSM.mBDisp[0] = '\x0';          // ボディ記述 Content-Disposition: attachment;
            strcpy(PS->PSM.mBEnc, "\"7bit\"");  // ボディ符号化 Content-Encoding: US-ASCII,base64
            PS->PSM.uHead.QuadPart = 0;         // ヘッダサイズ
	        PS->PSM.uBody.QuadPart = 0;         // ボディサイズ：
			PS->PSM.nHLine = 0;                 // ヘッダ部行数;
	        PS->PSM.nLine = 0;                  // 本文行数;
            ////////////////////
#ifdef UPDATE_20230706 // FETCH時のメールデータを事前に一括して読込み検索するフラグを追加
		    if (pContext->pfBufp)
               pContext->pfBuff-=(long)strlen(pLine);
            else
#endif
			fseek(fp, (long)(0-strlen(pLine)), SEEK_CUR); // １行手前にもどる
#ifdef UPDATE_20150701 // MESSAGE/RFC822の構造出力への対応(入れ子)
#ifdef UPDATE_20150712 // MIME構造の階層管理に親パート番号を設定できる項目を追加した。
            fetch_bodystructure_2(lpClientContext, &nPart0, pPS0, bExtend, fp);
#else
            fetch_bodystructure_2(lpClientContext, PS, bExtend, fp);
#endif
#else
			PS->PSM.nHLine = default_header(lpClientContext, &PS->PSM, pLine, nLineLen, mMBound, fp); // 最初のヘッダ
	        BODY_COUNTER(lpClientContext, &PS->PSM, pLine, nLineLen, mMBound, fp);   // BODYSIZE
			strcpy(PS->PSM.mBType, "\"TEXT\""); // ボディタイプ Content-Type: text/plain
            strcpy(PS->PSM.mBSub, "\"PLAIN\""); // ボディサブタイプ 
            PS->PSM.mBPara[0] = '\x0';          // ボディパラメータ括弧つきリスト ("name" "test.bin")
	        PS->PSM.mBID[0] = '\x0';            // ボディ ID Content-ID: <Mm3V1n13r86nICg78>
            PS->PSM.mBDisp[0] = '\x0';          // ボディ記述 Content-Disposition: attachment;
#endif
#ifdef UPDATE_20230706 // FETCH時のメールデータを事前に一括して読込み検索するフラグを追加
             if (pContext->pfBufp)
		       pContext->pfBuff = pContext->pfBufp + (long)cur;
	         else
#endif
			fseek(fp, cur, SEEK_SET); // 処理前の位置に戻る
		  }
#endif
				  continue;
				} //else 
#else
				{
			      sprintf(PS->mBPara,"(\"%s\" \"%s\")", p2, p3); // ボディパラメータ括弧つきリスト
				}
#endif
			    if (!stricmp(p2, "boundary")) { // マルチパートなら処理
	              sprintf(pBound, "--%s", p3);
	              strtok(pBound, "\"");
				}
			  }
			}
#ifdef UPDATE_20091129H // MESSAGE/RFC822の添付表示 CatchMe@Mailで添付きメールの表示が失敗する
		  } else {
			strtok(p2, "\r\n");
#ifdef UPDATE_20150618C  // HEADER(Content-Type:)構造でmultipart/alternativeの入れ子構造が正しく抽出できない不具合
            if (*p2 == '\x0' || *p2 == '\r' || *p2 == '\n')
			{
              sprintf(PS->mBSub,"\"UNKOWN\""); // ボディサブタイプ
			} else
#endif
			{
              sprintf(PS->mBSub,"\"%s\"", p2); // ボディサブタイプ
			}
#endif
		  }
#ifdef UPDATE_20150630 // MESSAGE/RFC822の構造出力への対応
		  if (!strcmp(PS->mBType, "\"message\"") && !strcmp(PS->mBSub, "\"rfc822\""))
		  {
#ifdef UPDATE_20150630C // MESSAGE/RFC822の構造出力への対応
			do
			{
#ifdef UPDATE_20230706 // FETCH時のメールデータを事前に一括して読込み検索するフラグを追加
		      if (pContext->pfBufp)
		        p = mgets(pLine, nLineLen, &pContext->pfBuff);
		      else
#endif
			  p = fgets(pLine, nLineLen, fp);
#ifdef UPDATE_20230706 // FETCH時のメールデータを事前に一括して読込み検索するフラグを追加
			} while ((p || (pContext->pfBuff ? *pContext->pfBuff : !feof(fp))) && !(*pLine == '\x0' || *pLine == '\r' || *pLine == '\n'));
#else
			}  while (!feof(fp) && !(*pLine == '\x0' || *pLine == '\r' || *pLine == '\n'));
#endif
#endif
			do
			{
#ifdef UPDATE_20230706 // FETCH時のメールデータを事前に一括して読込み検索するフラグを追加
		      if (pContext->pfBufp)
		        p = mgets(pLine, nLineLen, &pContext->pfBuff);
		      else
#endif
			  p = fgets(pLine, nLineLen, fp);
#ifdef UPDATE_20230706 // FETCH時のメールデータを事前に一括して読込み検索するフラグを追加
			} while ((p || (pContext->pfBuff ? *pContext->pfBuff : !feof(fp))) && (*pLine == '\x0' || *pLine == '\r' || *pLine == '\n'));
#else
			} while (!feof(fp) && (*pLine == '\x0' || *pLine == '\r' || *pLine == '\n'));
#endif
            ////////////////////
			// 初期化
			strcpy(PS->PSM.mBType, "\"TEXT\""); // ボディタイプ Content-Type: text/plain
            strcpy(PS->PSM.mBSub, "\"PLAIN\""); // ボディサブタイプ 
            PS->PSM.mBPara[0] = '\x0';          // ボディパラメータ括弧つきリスト ("name" "test.bin")
	        PS->PSM.mBID[0] = '\x0';            // ボディ ID Content-ID: <Mm3V1n13r86nICg78>
            PS->PSM.mBDisp[0] = '\x0';          // ボディ記述 Content-Disposition: attachment;
            strcpy(PS->PSM.mBEnc, "\"7bit\"");  // ボディ符号化 Content-Encoding: US-ASCII,base64
            PS->PSM.uHead.QuadPart = 0;         // ヘッダサイズ
	        PS->PSM.uBody.QuadPart = 0;         // ボディサイズ：
			PS->PSM.nHLine = 0;                 // ヘッダ部行数;
	        PS->PSM.nLine = 0;                  // 本文行数;
            ////////////////////
#ifdef UPDATE_20230706 // FETCH時のメールデータを事前に一括して読込み検索するフラグを追加
		    if (pContext->pfBufp)
		      pContext->pfBuff -= (long)strlen(pLine);
#endif
			fseek(fp, (long)(0-strlen(pLine)), SEEK_CUR); // １行手前にもどる
#ifdef UPDATE_20150701 // MESSAGE/RFC822の構造出力への対応(入れ子)
#ifdef UPDATE_20150712 // MIME構造の階層管理に親パート番号を設定できる項目を追加した。
            fetch_bodystructure_2(lpClientContext, &nPart0, pPS0, bExtend, fp);
#else
            fetch_bodystructure_2(lpClientContext, PS, bExtend, fp);
#endif
#else
			PS->PSM.nHLine = default_header(lpClientContext, &PS->PSM, pLine, nLineLen, mMBound, fp); // 最初のヘッダ
	        BODY_COUNTER(lpClientContext, &PS->PSM, pLine, nLineLen, mMBound, fp);   // BODYSIZE
			strcpy(PS->PSM.mBType, "\"TEXT\""); // ボディタイプ Content-Type: text/plain
            strcpy(PS->PSM.mBSub, "\"PLAIN\""); // ボディサブタイプ 
            PS->PSM.mBPara[0] = '\x0';          // ボディパラメータ括弧つきリスト ("name" "test.bin")
	        PS->PSM.mBID[0] = '\x0';            // ボディ ID Content-ID: <Mm3V1n13r86nICg78>
            PS->PSM.mBDisp[0] = '\x0';          // ボディ記述 Content-Disposition: attachment;
#endif
		  }
#endif
		}
	  }
	}
#ifdef UPDATE_20230706 // FETCH時のメールデータを事前に一括して読込み検索するフラグを追加
    if (pContext->pfBufp)
	  p = mgets(pLine, nLineLen, &pContext->pfBuff);
	else
#endif
    p = fgets(pLine, nLineLen, fp);
#ifdef UPDATE_20230706 // FETCH時のメールデータを事前に一括して読込み検索するフラグを追加
  } while(strcmp(pLine, "\r\n") && (p || (pContext->pfBuff ? *pContext->pfBuff : !feof(fp))));
#else
  } while(strcmp(pLine, "\r\n") && (p || !feof(fp)));
#endif
#ifdef UPDATE_20150630C // MESSAGE/RFC822の構造出力への対応
  if (!bCType && !stricmp(PS->mBType, "end_multipart"))
  {
    strcpy(PS->mBType, "\"TEXT\""); // ボディタイプ
#ifdef UPDATE_20150712 // MIME構造の階層管理に親パート番号を設定できる項目を追加した。
	nPart--;
#endif
#ifdef UPDATE_20150712B // MIMEヘッダが空の構造の時正しい構造が生成できない不具合。
	if (*pBound0 != '\x0')
	{
#ifdef UPDATE_20230706 // FETCH時のメールデータを事前に一括して読込み検索するフラグを追加
      if (pContext->pfBufp)
        cur = (long)(pContext->pfBuff - pContext->pfBufp);
	  else
#endif
      cur = ftell(fp);
#ifdef UPDATE_20230706_DEBUG // FETCH時のメールデータを事前に一括して読込み検索するフラグを追加
if (bDebug) printf("multipart_header(1) cur=%lu\n", cur);
#endif
#ifdef UPDATE_20230706 // FETCH時のメールデータを事前に一括して読込み検索するフラグを追加
      if (pContext->pfBufp)
	    p = mgets(pLine, nLineLen, &pContext->pfBuff);
	  else
		p = fgets(pLine, nLineLen, fp);
      if (p)
#else
      if (p = fgets(pLine, nLineLen, fp))
#endif
	  {
	    if (strnicmp(pLine, pBound0, strlen(pBound0))) // MIMEの終わりかどうかを確認
		{
	      nPart++;
		}
	  } // マルチパートの始まりまでスキップ
#ifdef UPDATE_20230706 // FETCH時のメールデータを事前に一括して読込み検索するフラグを追加
      if (pContext->pfBufp)
	    pContext->pfBuff = pContext->pfBufp + (long)cur; 
	  else
#endif
      fseek(fp, cur, SEEK_SET);
#ifdef UPDATE_20230706_DEBUG // FETCH時のメールデータを事前に一括して読込み検索するフラグを追加
if (bDebug) printf("multipart_header(1) SEEK_SET cur=%lu\n", cur);
#endif
	}
#endif
  }
#endif

#ifdef UPDATE_20150712 // MIME構造の階層管理に親パート番号を設定できる項目を追加した。
  return nPart;
#else
#ifdef UPDATE_20150711 // MESSAGE/RFC822の構造出力への対応(入れ子の解析)でTEXT以外構造が出力されなかった。
  return nCloseType;
#else
#ifdef UPDATE_20150708 // MIME構造で(multipart(text,multipart(html))のような構造で順番道理の結果が返せない不具合。
  return nPart;
#endif
#endif
#endif
}

#ifdef UPDATE_20150630 // MESSAGE/RFC822の構造出力への対応
void fetch_bodystructure(PCLIENT_CONTEXT lpClientContext, BOOL bExtend, FILE *fpx)
#else
void fetch_bodystructure(PCLIENT_CONTEXT lpClientContext, BOOL bExtend) 
#endif
{
  PImap4Context    pContext = &lpClientContext->Context;
  char             mFn[MAX_PATH], mLine[1024], mBound[BODY_PART_LIMIT][256];
  char             mBody[32], mRec[32];
  FILE             *fp;
  DWORD            n, i, j[BODY_PART_LIMIT], nPart = 0, nDepath = 0;
  BOOL             bBody = FALSE;

  PartStruct    PS[BODY_PART_LIMIT]; //
#ifdef UPDATE_20150630 // MESSAGE/RFC822の構造出力への対応
#ifdef UPDATE_20150708B // MESSAGE/RFC822の構造出力結果がバッファオーバフローする可能性のある不具合。
  char             mRFC822[1024];
#else
  char             mRFC822[512];
#endif
#endif
#ifdef UPDATE_20150708 // MIME構造で(multipart(text,multipart(html))のような構造で順番道理の結果が返せない不具合。
  int              nResult = 0;
#endif
#ifdef UPDATE_20150712      // MIME構造の階層管理に親パート番号を設定できる項目を追加した。
  DWORD            nCurrentNo = nPart;
  int              nMESSRFC822 = 0;
#endif
#ifdef UPDATE_20150712A // MIME構造の階層管理に親パート番号を設定できる項目を追加した。
  DWORD	           n2 = 0;
  long             cur;
#endif
#ifdef UPDATE_20230706 // FETCH時のメールデータを事前に一括して読込み検索するフラグを追加
  CHAR             *p;
#endif


#ifndef UPDATE_20150712 // MIME構造の階層管理に親パート番号を設定できる項目を追加した。
#ifdef UPDATE_20150618C  // HEADER(Content-Type:)構造でmultipart/alternativeの入れ子構造が正しく抽出できない不具合
#ifdef UPDATE_20150630 // MESSAGE/RFC822の構造出力への対応
  if (fpx)
    sprintf(pContext->mMessages, " (");
  else
#endif
    sprintf(pContext->mMessages, "%s %s", (bExtend ? "BODYSTRUCTURE" :"BODY") , (nPart == 0 ? "" : "("));
  put_reply(lpClientContext, FALSE, TRUE);
#endif  
#endif
  for (i = 0; i < BODY_PART_LIMIT; i++) {
#ifdef UPDATE_20150630 // MESSAGE/RFC822の構造出力への対応
   strcpy(PS[i].mBType, "\"TEXT\""); // ボディタイプ Content-Type: text/plain
   strcpy(PS[i].mBSub, "\"PLAIN\""); // ボディサブタイプ 
#else
    PS[i].mBType[0] = '\x0';
    PS[i].mBSub[0]  = '\x0';
#endif
    PS[i].mBPara[0] = '\x0';
    PS[i].mBID[0]   = '\x0';
    PS[i].mBDisp[0] = '\x0';
    PS[i].mBEnc[0]  = '\x0';
    PS[i].uBody.QuadPart = 0;
#ifdef UPDATE_20150630 // MESSAGE/RFC822の構造出力への対応
    PS[i].uHead.QuadPart = 0;
	PS[i].nHLine         = 0;
#endif
    PS[i].nLine          = 0;
#ifdef UPDATE_20150712      // MIME構造の階層管理に親パート番号を設定できる項目を追加した。
	PS[i].nPPart = 0;    // 親パート番号
#endif
#ifdef UPDATE_20150630 // MESSAGE/RFC822の構造出力への対応
	strcpy(PS[i].PSM.mBType, "\"TEXT\""); // ボディタイプ Content-Type: text/plain
    strcpy(PS[i].PSM.mBSub, "\"PLAIN\""); // ボディサブタイプ 
    PS[i].PSM.mBPara[0] = '\x0';          // ボディパラメータ括弧つきリスト ("name" "test.bin")
	PS[i].PSM.mBID[0] = '\x0';            // ボディ ID Content-ID: <Mm3V1n13r86nICg78>
    PS[i].PSM.mBDisp[0] = '\x0';          // ボディ記述 Content-Disposition: attachment;
    strcpy(PS[i].PSM.mBEnc, "\"7bit\"");  // ボディ符号化 Content-Encoding: US-ASCII,base64
    PS[i].PSM.uHead.QuadPart = 0;         // ヘッダサイズ
	PS[i].PSM.uBody.QuadPart = 0;         // ボディサイズ：
	PS[i].PSM.nHLine = 0;                 // ヘッダ部行数;
	PS[i].PSM.nLine = 0;                  // 本文行数;
#endif
#ifdef UPDATE_20150712      // MIME構造の階層管理に親パート番号を設定できる項目を追加した。
	PS[i].PSM.nPPart = 0;    // 親パート番号
#endif
    mBound[i][0] = '\x0';
	j[i] = 0;
  }
  sprintf(mFn, "%s/%s", pContext->mSelectDir, (pContext->bEncMSG ? pContext->mDecFileName : pContext->FindData.cFileName));
#ifdef UPDATE_20230706 // FETCH時のメールデータを事前に一括して読込み検索するフラグを追加
  fp = NULL;
  if (pContext->pBuff) // ファイルの先頭にポインタリセット
    pContext->pfBuff = pContext->pBuff;
  if (pContext->pBuff || !pContext->pBuff && (fp = fopen(mFn, "rb"))) 
#else
#ifdef UPDATE_20150630 // MESSAGE/RFC822の構造出力への対応
  if (fpx)
    fp = fpx;
  else 
    fp = fopen(mFn, "rb");
  if (fp)
#else
  if ((fp = fopen(mFn, "rb"))) 
#endif
#endif
  {
	default_header(lpClientContext, &PS[nPart], mLine, sizeof(mLine), mBound[nDepath], fp); // 最初のヘッダ
	BODY_COUNTER(lpClientContext, &PS[nPart], mLine, sizeof(mLine), mBound[nDepath], fp);   // BODYSIZE
	if (mBound[0][0] != '\x0') { // マルチパートなら次の処理
#ifndef UPDATE_20150712 // MIME構造の階層管理に親パート番号を設定できる項目を追加した。
#ifdef UPDATE_20150618C  // HEADER(Content-Type:)構造でmultipart/alternativeの入れ子構造が正しく抽出できない不具合
      sprintf(pContext->mMessages, "(");
	  put_reply(lpClientContext, FALSE, TRUE);
#endif
#endif
      do {
	    nPart++;
		if (nPart >= BODY_PART_LIMIT)
		{
#ifdef UPDATE_20090325C // メッセージのBODYパート数の上限を超えた場合ハングする
		  nPart = BODY_PART_LIMIT-1;
#endif
		  break;
		}
		strcpy(PS[nPart].mBType, "end_multipart");
#ifdef UPDATE_20150712      // MIME構造の階層管理に親パート番号を設定できる項目を追加した。
        nCurrentNo = nPart;
#endif
#ifdef UPDATE_20150710 // MESSAGE/RFC822の構造出力への対応(入れ子の解析)でTEXT以外構造が出力されなかった。
		nPart = multipart_header(lpClientContext, 0, PS, &PS[nPart], mLine, sizeof(mLine), mBound[nDepath], mBound[nDepath+1], fp , bExtend); // マルチパート
#else
#ifdef UPDATE_20150708 // MIME構造で(multipart(text,multipart(html))のような構造で順番道理の結果が返せない不具合。
        nResult = multipart_header(lpClientContext, (nPart > 0 ? &PS[nPart-1] : NULL), &PS[nPart], mLine, sizeof(mLine), mBound[nDepath], mBound[nDepath+1], fp , bExtend); // マルチパート
#else
#ifdef UPDATE_20150620B // 多重入れ子構造がある場合に正しく抽出できない不具合
        multipart_header(lpClientContext, &PS[nPart], mLine, sizeof(mLine), mBound[nDepath], mBound[nDepath+1], fp , bExtend); // マルチパート
#else
        multipart_header(lpClientContext, &PS[nPart], mLine, sizeof(mLine), mBound[nDepath+1], fp , bExtend); // マルチパート
#endif
#endif
#endif
		if (mBound[nDepath+1][0] && strcmp(mBound[nDepath], mBound[nDepath+1]))
		  nDepath++;
#ifdef UPDATE_20150712      // MIME構造の階層管理に親パート番号を設定できる項目を追加した。
#ifdef UPDATE_20150712A      // MIME構造の階層管理に親パート番号を設定できる項目を追加した。
#ifdef UPDATE_20230706 // FETCH時のメールデータを事前に一括して読込み検索するフラグを追加
		if (pContext->pfBufp)
          cur = (long)(pContext->pfBuff - pContext->pfBufp);
		else
#endif
		cur = ftell(fp);
#ifdef UPDATE_20230706_DEBUG // FETCH時のメールデータを事前に一括して読込み検索するフラグを追加
if (bDebug) printf("fetch_bodystructure(0) cur=%lu\n", cur);
#endif
		if (PS[nPart].nLine == 0)
		{
          BODY_COUNTER(lpClientContext, &PS[nPart], mLine, sizeof(mLine), mBound[nDepath], fp);   // BODYSIZE
		}
        if (nCurrentNo != nPart)
		{
#ifdef UPDATE_20230706 // FETCH時のメールデータを事前に一括して読込み検索するフラグを追加
		  if (pContext->pfBufp)
			pContext->pfBuff = pContext->pfBufp + (long)cur;
		  else
#endif
		  fseek(fp, cur, SEEK_SET);
#else
        if (nCurrentNo == nPart)
		{
	      BODY_COUNTER(lpClientContext, &PS[nPart], mLine, sizeof(mLine), mBound[nDepath], fp);   // BODYSIZE
		}
		else 
		{
#endif
#ifdef UPDATE_20230706 // FETCH時のメールデータを事前に一括して読込み検索するフラグを追加
		  if (pContext->pfBufp)
		    p = mgets(mLine, sizeof(mLine)-1, &pContext->pfBuff);
		  else
		    p = fgets(mLine, sizeof(mLine), fp);
		  do
#else
          while(fgets(mLine, sizeof(mLine), fp) || !feof(fp)) // マルチパートの始まりまでスキップ
#endif
		  {
	        if (!strcmp(mLine, ".\r\n"))
	          break;
	        if (mBound[nDepath][0] != '\x0' && !strnicmp(mLine, mBound[nDepath], strlen(mBound[nDepath]))) // メッセージの終了
			{
#ifndef UPDATE_20150712D // MIMEパートの終わり判定が抜ける対策
	          nPart++;
		      strcpy(PS[nPart].mBType,"end_multipart");
#endif
	          break;
			}
#ifdef UPDATE_20230706 // FETCH時のメールデータを事前に一括して読込み検索するフラグを追加
	        if (pContext->pfBufp)
	          p = mgets(mLine, sizeof(mLine)-1, &pContext->pfBuff);
			else
              p = fgets(mLine, sizeof(mLine), fp);
#ifdef UPDATE_20230706 // FETCH時のメールデータを事前に一括して読込み検索するフラグを追加
		  } while (p || (pContext->pfBuff ? *pContext->pfBuff : !feof(fp)));
#else
		  } while(p || !feof(fp));
#endif
#else
		  } 
#endif
		}
#else
	    BODY_COUNTER(lpClientContext, &PS[nPart], mLine, sizeof(mLine), mBound[nDepath], fp);   // BODYSIZE
		if (strstr(mLine, mBound[nDepath]) && strstr(&mLine[strlen(mBound[nDepath])],"--")) {
		  if (nDepath > 0) {
            mBound[nDepath][0] = '\x0';
			nDepath--;
		  } else
		    break;
		}
#endif
	    if (strstr(mLine, mBound[nDepath]) && strstr(&mLine[strlen(mBound[nDepath])],"--")) {
#ifdef UPDATE_20150712D // MIMEパートの終わり判定が抜ける対策
		  nPart++;
		  strcpy(PS[nPart].mBType,"end_multipart");
#endif
		  if (nDepath > 0) {
            mBound[nDepath][0] = '\x0';
		    nDepath--;
		  } else
		    break;
		}
#ifdef UPDATE_20230706 // FETCH時のメールデータを事前に一括して読込み検索するフラグを追加
	  } while (strcmp(mLine, ".\r\n") && (p || (pContext->pfBuff ? *pContext->pfBuff : !feof(fp))));
#else
#ifdef UPDATE_20150620B // 多重入れ子構造がある場合に正しく抽出できない不具合
	  } while (strcmp(mLine, ".\r\n") && !feof(fp) );
#else
	  } while (strcmp(mLine, ".\r\n") || !feof(fp) );
#endif
#endif
/*	  
	  if (nPart==1 && !strcmp(PS[nPart].mBType, "end_multipart")) { // マルチパートなのにBODYが見つからない時
		PS[nPart].mBType[0] = '\x0';
		//nPart--;
		//strcpy(PS[nPart].mBType, "end_multipart");
	  }
*/	  
#ifndef UPDATE_20150712 // MIME構造の階層管理に親パート番号を設定できる項目を追加した。
	  //if (!strcmp(PS[0].mBType, "\"multipart\"")) {
	  { // 先頭のContent-Typeは読み飛ばす
	    n = 0;
	    for (i = 1; i <= nPart; i++) {
		  if (!strcmp(PS[i].mBType, "\"multipart\"")) {
		    n++;
		  } else if (!strcmp(PS[i].mBType, "end_multipart")) {
		    if (n == 0) {
              strcpy(pContext->mMessages, "");
	          PS[i].mBType[0] = '\x0';  // 最後を揃える
		      nPart = i;
		      break;
			} else
			  n--;
		  }
		}
	  }
#endif
	  /*
	  if (n) {  //  multipart
	    for (i = 0; i <= nPart; i++) {
	      if (!strcmp(PS[i].mBType, "end_multipart")) {
		    if (n == 0) {               // multipart < end_multipartが合わない場合
		      PS[i].mBType[0] = '\x0';  // 最後を揃える
		      nPart = i;
		      break;
			} else 
			  n--;
		  }
		}
	  }
	  */
    /*
	  for (i = nPart; i > 0; i--) {
		if (!strcmp(PS[nPart].mBType, "end_multipart")) {
		  PS[1].mBType[0] = '\x0';
		  nPart = 1;
		  break;
		}
	  }
	*/
	} 
#ifdef UPDATE_20150630 // MESSAGE/RFC822の構造出力への対応
    if (!fpx)
#endif
#ifdef UPDATE_20230706 // FETCH時のメールデータを事前に一括して読込み検索するフラグを追加
      if (!pContext->pBuff && fp)
#endif
	    fclose(fp);
  }

#ifdef UPDATE_20150712 // MIME構造の階層管理に親パート番号を設定できる項目を追加した。
  sprintf(pContext->mMessages, "%s ", (bExtend ? "BODYSTRUCTURE" :"BODY"));
  put_reply(lpClientContext, FALSE, TRUE);
#else
#ifndef UPDATE_20150618C  // HEADER(Content-Type:)構造でmultipart/alternativeの入れ子構造が正しく抽出できない不具合
  sprintf(pContext->mMessages, "%s %s", (bExtend ? "BODYSTRUCTURE" :"BODY") , (nPart == 0 ? "" : "("));
  put_reply(lpClientContext, FALSE, TRUE);
#endif
#endif
#ifndef UPDATE_20150710 // MESSAGE/RFC822の構造出力への対応(入れ子の解析)でTEXT以外構造が出力されなかった。
#ifdef UPDATE_20150708 // MIME構造で(multipart(text,multipart(html))のような構造で順番道理の結果が返せない不具合。
  if (nResult == 0)
#endif
#endif
  {
#ifdef UPDATE_20150712A // MIME構造の階層管理に親パート番号を設定できる項目を追加した。
	n2 = 0;
#endif
    nDepath = 0;
#ifdef UPDATE_20150712 // MIME構造の階層管理に親パート番号を設定できる項目を追加した。
    for (i = 0; i <= nPart; i++) 
#else
#ifdef UPDATE_20150710 // MESSAGE/RFC822の構造出力への対応(入れ子の解析)でTEXT以外構造が出力されなかった。
    for (i = (nResult == -1 ? 2 : (nPart == 0 ? 0 : 1)); i <= nPart; i++) 
#else
    for (i = (nPart == 0 ? 0 : 1); i <= nPart; i++)
#endif
#endif
	{
	  sprintf(mBody, "%lu", PS[i].uBody.QuadPart);
	  sprintf(mRec, "%lu", PS[i].nLine);
#ifdef UPDATE_20150712 // MIME構造の階層管理に親パート番号を設定できる項目を追加した。
	  if (!stricmp(PS[i].mBType, "\"multipart\"")) 
#else
#ifdef UPDATE_20150617A // Content-Type: multipart/alternative の箇所の構造を出力できない不具合
	  if (!stricmp(PS[i].mBType, "\"multipart\"") && !stricmp(PS[i].mBSub, "\"mixed\"")) 
#else
	  if (!stricmp(PS[i].mBType, "\"multipart\"")) 
#endif
#endif
	  {
	     j[++nDepath] = i;
         sprintf(pContext->mMessages, "(");
	  } else if (!stricmp(PS[i].mBType, "end_multipart")) {
#ifdef UPDATE_20150712D // MIMEパートの終わり判定が抜ける対策
#ifdef UPDATE_20150712E // MIMEパートの区切り処理が抜ける対策
		if ((long)nDepath == 0 && i < nPart)
#else
		if ((long)nDepath == 0)
#endif
		{
		  continue;
		}
#endif
	    if (bExtend) // BODYSTRUCTURE
		{
#ifdef UPDATE_20150712 // MIME構造の階層管理に親パート番号を設定できる項目を追加した。
		  if (nMESSRFC822)
		  {
            if (!stricmp(PS[j[nDepath]-1].mBType, "\"message\"") &&
				!stricmp(PS[j[nDepath]-1].mBSub, "\"rfc822\""))
			{
			  if (nMESSRFC822 > 1 && !stricmp(PS[j[nDepath]+1].mBType, "\"message\""))
			  {
                sprintf(pContext->mMessages, " %lu NIL %s NIL NIL)",
				                   PS[i-2].nLine,
								   (PS[i-2].mBDisp[0] ? PS[i-2].mBDisp: "NIL"));
	             put_reply(lpClientContext, FALSE, TRUE);
			  }
			
			if (!stricmp(PS[j[nDepath]+1].mBType, "\"multipart\""))
			{
            sprintf(pContext->mMessages, " %s %s NIL NIL NIL)",
                                   (PS[j[nDepath]+1].mBSub[0] ? strupr(PS[j[nDepath]+1].mBSub): "NIL"),
                                   (PS[j[nDepath]+1].mBPara[0] ? PS[j[nDepath]+1].mBPara: "NIL"));
#ifdef UPDATE_20150712A // MIME構造の階層管理に親パート番号を設定できる項目を追加した。
				 if (!(!stricmp(PS[j[nDepath]].mBType, "\"message\"") &&
				       !stricmp(PS[j[nDepath]].mBSub, "\"rfc822\"")))
				 {
#ifdef UPDATE_20150712E // MIMEパートの区切り処理が抜ける対策
				   if (!stricmp(PS[i+1].mBType, "\"multipart\"") ||
					   !stricmp(PS[i+1].mBType, "end_multipart"))
#endif
				   {
	             put_reply(lpClientContext, FALSE, TRUE);
            sprintf(pContext->mMessages, " %s %s NIL NIL NIL) %lu NIL %s NIL NIL)",
                                   (PS[j[nDepath]].mBSub[0] ? strupr(PS[j[nDepath]].mBSub): "NIL"),
                                   (PS[j[nDepath]].mBPara[0] ? PS[j[nDepath]].mBPara: "NIL"),
								   PS[j[nDepath]-1].nLine,
                                   (PS[j[nDepath]-1].mBDisp[0] ? PS[j[nDepath]].mBDisp: "NIL"));
			       n2 = 1;
				   }
				 }
#endif
			} else {
            sprintf(pContext->mMessages, " %s %s NIL NIL NIL) %lu NIL %s %s %s)%s",
                                   (PS[j[nDepath]].mBSub[0] ? strupr(PS[j[nDepath]].mBSub): "NIL"),
                                   (PS[j[nDepath]].mBPara[0] ? PS[j[nDepath]].mBPara: "NIL"),
								   PS[j[nDepath]-1].nLine,
                                   (PS[j[nDepath]-1].mBDisp[0] ? PS[j[nDepath]-1].mBDisp: "NIL"),
								   "NIL",
                                   "NIL",
								   (i == 0 ? "" : ""));
			}
			} else {
            sprintf(pContext->mMessages, " %s %s NIL NIL NIL) %lu NIL %s %s %s)%s",
                                   (PS[j[nDepath]-1].mBSub[0] ? strupr(PS[j[nDepath]-1].mBSub): "NIL"),
                                   (PS[j[nDepath]-1].mBPara[0] ? PS[j[nDepath]-1].mBPara: "NIL"),
								   PS[j[nDepath]-1].nLine,
                                   (PS[j[nDepath]-1].mBDisp[0] ? PS[j[nDepath]-1].mBDisp: "NIL"),
								   "NIL",
                                   "NIL",
								   (i == 0 ? "" : ""));
#ifdef UPDATE_20150712E // MIMEパートの区切り処理が抜ける対策
	         if (!stricmp(PS[j[nDepath]].mBType, "\"message\"") ||
			     !stricmp(PS[j[nDepath]].mBType, "\"multipart\""))
			 {
               put_reply(lpClientContext, FALSE, TRUE);
               sprintf(pContext->mMessages, " %lu NIL %s NIL NIL)",
				     PS[j[nDepath]].nLine,
					(PS[j[nDepath]].mBDisp[0] ? PS[j[nDepath]].mBDisp: "NIL"));
			 }
#endif
			}
		  } else 
#endif
		  {
#ifdef UPDATE_20150712F // MIMEパートの区切り処理が抜ける対策
			if (nPart == i+1 && nDepath == 1 && !stricmp(PS[j[nDepath]+1].mBSub,"\"related\""))
			{
      sprintf(pContext->mMessages, " %s %s NIL NIL NIL) %I64u NIL %s NIL NIL)",
                                 (PS[j[nDepath]+1].mBSub[0] ? strupr(PS[j[nDepath]+1].mBSub): "NIL"),
                                 (PS[j[nDepath]+1].mBPara[0] ? PS[j[nDepath]+1].mBPara: "NIL"),
	                              PS[j[nDepath]+1].uHead.QuadPart+PS[j[nDepath]+1].uBody.QuadPart,
					              (PS[j[nDepath]+1].mBDisp[0] ? PS[j[nDepath]+1].mBDisp: "NIL"));
	  //strcp(pContext->mMessages, "\"RELATED\" (\"boundary\" \"----=_NextPart_000_0FC2_01D0ABF9.938BBF40\") NIL NIL NIL)"); 
			} else 
#endif
#ifdef UPDATE_20210412 // related後の続きでMIMEパートがある場合区切り処理が抜ける対策
			if (nPart >= i+1 && nDepath == 1 && stricmp(PS[j[nDepath]+1].mBSub,"\"mixed\""))
			{
			  int k = 2;
			  BOOL bkHit = TRUE;
			  while (stricmp(PS[j[nDepath]+k].mBSub,"\"mixed\""))
			  {
				 if (nPart > k)
				 {
				   k++;
				 } else {
				   bkHit = FALSE;
				   break;
				 }
			  }
			  if (bkHit)
			  {
      sprintf(pContext->mMessages, " %s %s NIL NIL NIL) %I64u NIL %s NIL NIL)",
                                 (PS[j[nDepath]+k].mBSub[0] ? strupr(PS[j[nDepath]+k].mBSub): "NIL"),
                                 (PS[j[nDepath]+k].mBPara[0] ? PS[j[nDepath]+k].mBPara: "NIL"),
	                              PS[j[nDepath]+k].uHead.QuadPart+PS[j[nDepath]+k].uBody.QuadPart,
					              (PS[j[nDepath]+k].mBDisp[0] ? PS[j[nDepath]+k].mBDisp: "NIL"));
			  } else {
      sprintf(pContext->mMessages, " %s %s NIL NIL NIL)",
                                 (PS[j[nDepath]].mBSub[0] ? strupr(PS[j[nDepath]].mBSub): "NIL"),
                                 (PS[j[nDepath]].mBPara[0] ? PS[j[nDepath]].mBPara: "NIL"));
			  }
			} else 
#endif

			{
#ifdef UPDATE_20150617B // 応答に"NIL"追加
#ifdef UPDATE_20150616 // BODYSTRUCTUREのマルチパート構造の応答時に半角スペースの区切りが挿入されない箇所があった
      sprintf(pContext->mMessages, " %s %s NIL NIL NIL)",
#else
      sprintf(pContext->mMessages, "%s %s NIL NIL NIL)",
#endif
#else
#ifdef UPDATE_20150616 // BODYSTRUCTUREのマルチパート構造の応答時に半角スペースの区切りが挿入されない箇所があった
      sprintf(pContext->mMessages, " %s %s NIL NIL)",
#else
      sprintf(pContext->mMessages, "%s %s NIL NIL)",
#endif
#endif
                                 (PS[j[nDepath]].mBSub[0] ? strupr(PS[j[nDepath]].mBSub): "NIL"),
                                 (PS[j[nDepath]].mBPara[0] ? PS[j[nDepath]].mBPara: "NIL"));
			}
#ifdef UPDATE_20150712E // MIMEパートの区切り処理が抜ける対策
	         if (i < nPart && 
				 nPart > 2 &&
				 !stricmp(PS[nPart-2].mBType, "end_multipart") &&
				 !stricmp(PS[nPart-1].mBType, "end_multipart") &&
				 !stricmp(PS[nPart].mBType, "end_multipart") &&
				 !stricmp(PS[j[nDepath]].mBType, "\"multipart\""))
			 {
			   if (j[nDepath] == 0)
			   {
                 put_reply(lpClientContext, FALSE, TRUE);
                 sprintf(pContext->mMessages, " %lu NIL %s NIL NIL)",
				     PS[j[nDepath]].nLine,
					(PS[j[nDepath]].mBDisp[0] ? PS[j[nDepath]].mBDisp: "NIL"));
			   }
			   else if (j[nDepath] > 0 && !stricmp(PS[j[nDepath]-1].mBType, "\"message\""))
			   {
                 put_reply(lpClientContext, FALSE, TRUE);
                 sprintf(pContext->mMessages, " %lu NIL %s NIL NIL)",
				     PS[j[nDepath]-1].nLine,
					(PS[j[nDepath]-1].mBDisp[0] ? PS[j[nDepath]-1].mBDisp: "NIL"));
			   }
			 }
#endif
		  }
		} else {        // BODY
#ifdef UPDATE_20150712 // MIME構造の階層管理に親パート番号を設定できる項目を追加した。
		  if (nMESSRFC822)
		  {
            if (!stricmp(PS[j[nDepath]-1].mBType, "\"message\"") &&
				!stricmp(PS[j[nDepath]-1].mBSub, "\"rfc822\""))
			{
			  if (nMESSRFC822 > 1 && !stricmp(PS[j[nDepath]+1].mBType, "\"message\""))
			  {
                sprintf(pContext->mMessages, " %lu NIL %s NIL NIL)",
				                   PS[i-2].nLine,
								   (PS[i-2].mBDisp[0] ? PS[i-2].mBDisp: "NIL"));
	             put_reply(lpClientContext, FALSE, TRUE);
			  }
			if (!stricmp(PS[j[nDepath]+1].mBType, "\"multipart\""))
			{
            sprintf(pContext->mMessages, " %s)",// %s NIL NIL NIL)",
                                   (PS[j[nDepath]+1].mBSub[0] ? strupr(PS[j[nDepath]+1].mBSub): "NIL"));
                                   //(PS[j[nDepath]+1].mBPara[0] ? PS[j[nDepath]+1].mBPara: "NIL"));
#ifdef UPDATE_20150712A // MIME構造の階層管理に親パート番号を設定できる項目を追加した。
				 if (!(!stricmp(PS[j[nDepath]].mBType, "\"message\"") &&
				       !stricmp(PS[j[nDepath]].mBSub, "\"rfc822\"")))
				 {
#ifdef UPDATE_20150712E // MIMEパートの区切り処理が抜ける対策
				   if (!stricmp(PS[i+1].mBType, "\"multipart\"") ||
					   !stricmp(PS[i+1].mBType, "end_multipart"))
#endif
				   {
	             put_reply(lpClientContext, FALSE, TRUE);
            sprintf(pContext->mMessages, " %s) %lu)",
                                   (PS[j[nDepath]].mBSub[0] ? strupr(PS[j[nDepath]].mBSub): "NIL"),
								   PS[j[nDepath]-1].nLine);
			       n2 = 1;
				   }
				 }
#endif
			} else {			
#ifdef UPDATE_20150712A // MIME構造の階層管理に親パート番号を設定できる項目を追加した。
            sprintf(pContext->mMessages, " %s) %lu)",
                                   (PS[j[nDepath]].mBSub[0] ? strupr(PS[j[nDepath]].mBSub): "NIL"),
								   PS[j[nDepath]-1].nLine);
#else
            sprintf(pContext->mMessages, " %s %s NIL NIL NIL) %lu NIL %s %s %s)%s",
                                   (PS[j[nDepath]].mBSub[0] ? strupr(PS[j[nDepath]].mBSub): "NIL"),
                                   (PS[j[nDepath]].mBPara[0] ? PS[j[nDepath]].mBPara: "NIL"),
								   PS[j[nDepath]].nLine,
                                   (PS[j[nDepath]].mBDisp[0] ? PS[j[nDepath]].mBDisp: "NIL"),
								   "NIL",
                                   "NIL",
								   (i == 0 ? "" : ""));
#endif
			}
			} else {
#ifdef UPDATE_20150712A // MIME構造の階層管理に親パート番号を設定できる項目を追加した。
            sprintf(pContext->mMessages, " %s) %lu)",
                                   (PS[j[nDepath]-1].mBSub[0] ? strupr(PS[j[nDepath]-1].mBSub): "NIL"),
								   PS[j[nDepath]-1].nLine);
#else
            sprintf(pContext->mMessages, " %s %s NIL NIL NIL) %lu NIL %s %s %s)%s",
                                   (PS[j[nDepath]-1].mBSub[0] ? strupr(PS[j[nDepath]-1].mBSub): "NIL"),
                                   (PS[j[nDepath]-1].mBPara[0] ? PS[j[nDepath]-1].mBPara: "NIL"),
								   PS[j[nDepath]-1].nLine,
                                   (PS[j[nDepath]-1].mBDisp[0] ? PS[j[nDepath]-1].mBDisp: "NIL"),
								   "NIL",
                                   "NIL",
								   (i == 0 ? "" : ""));
#endif
			}
		  } else
#endif
		  {
#ifdef UPDATE_20150712F // MIMEパートの区切り処理が抜ける対策
			if (nPart == i+1 && nDepath == 1 && !stricmp(PS[j[nDepath]+1].mBSub,"\"related\""))
			{
	  sprintf(pContext->mMessages, " %s) %I64u)",
	                    (PS[j[nDepath]+1].mBSub[0] ? strupr(PS[j[nDepath]+1].mBSub): "NIL"),
	                     PS[j[nDepath]+1].uHead.QuadPart+PS[j[nDepath]+1].uBody.QuadPart);
			} else 
#endif
			{
#ifdef UPDATE_20150712A // MIME構造の階層管理に親パート番号を設定できる項目を追加した。
       sprintf(pContext->mMessages, " %s)", (PS[j[nDepath]].mBSub[0] ? strupr(PS[j[nDepath]].mBSub): "NIL"));
#else
#ifdef UPDATE_20150616 // BODYSTRUCTUREのマルチパート構造の応答時に半角スペースの区切りが挿入されない箇所があった
       sprintf(pContext->mMessages, " %s", (PS[j[nDepath]].mBSub[0] ? strupr(PS[j[nDepath]].mBSub): "NIL"));
#else
       sprintf(pContext->mMessages, "%s", (PS[j[nDepath]].mBSub[0] ? strupr(PS[j[nDepath]].mBSub): "NIL"));
#endif
#endif
#ifdef UPDATE_20150712E // MIMEパートの区切り処理が抜ける対策
	         if (i < nPart && 
				 nPart > 2 &&
				 !stricmp(PS[nPart-2].mBType, "end_multipart") &&
				 !stricmp(PS[nPart-1].mBType, "end_multipart") &&
				 !stricmp(PS[nPart].mBType, "end_multipart") &&
				 !stricmp(PS[j[nDepath]].mBType, "\"multipart\""))
			 {
			   if (j[nDepath] == 0)
			   {
                 put_reply(lpClientContext, FALSE, TRUE);
                 sprintf(pContext->mMessages, " %lu)", PS[j[nDepath]].nLine);
			   }
			   else if (j[nDepath] > 0 && !stricmp(PS[j[nDepath]-1].mBType, "\"message\""))
			   {
                 put_reply(lpClientContext, FALSE, TRUE);
                 sprintf(pContext->mMessages, " %lu)", PS[j[nDepath]-1].nLine);
			   }
			 }
#endif
			}
		  }
		}
#ifdef UPDATE_20150712 // MIME構造の階層管理に親パート番号を設定できる項目を追加した。
		nMESSRFC822 = 0;
#endif
	    nDepath--;
	  } else {
#ifdef UPDATE_20150620B // 多重入れ子構造がある場合に正しく抽出できない不具合
      // 2015.06.30 修正 if (bExtend && PS[i].mBType[0] != '\x0' || mBound[0][0] == '\x0')
	    if (PS[i].mBType[0] != '\x0' || mBound[0][0] == '\x0')
#endif
		{
#ifdef UPDATE_20150712 // MIME構造の階層管理に親パート番号を設定できる項目を追加した。
		    sprintf(mRFC822, "(NIL NIL NIL NIL NIL NIL NIL NIL NIL NIL) (\"TEXT\" \"PLAIN\" NIL NIL NIL NIL NIL NIL NIL NIL NIL NIL) NIL NIL");
            if (!stricmp(PS[i].mBType, "\"message\"") && !stricmp(PS[i].mBSub, "\"rfc822\""))
			{
			  if (!stricmp(PS[i+1].mBType, "\"multipart\""))
			  {
#ifdef UPDATE_20180110 // rfc822構造添付ファイルに複雑なMIME構造の生成に失敗する場合があった
			    n = 2;
				if (!stricmp(PS[i+n+1].mBType, "\"multipart\"") && !stricmp(PS[i+n+1].mBSub, "\"alternative\""))
				{
				  //++nDepath;
	              j[++nDepath] = i+1;
	              j[++nDepath] = i+n;
	              j[++nDepath] = i+n+2;
				} else 
				{
	              j[++nDepath] = i+1;
				  //nPart--;
			      //n = 2;
				}
#else
				{
	              j[++nDepath] = i+1;
				  //nPart--;
			      n = 2;
				}
#endif
                nMESSRFC822 = 1;

				if (!stricmp(PS[i+n].mBType, "\"multipart\"")) // ALTERNATIVE
				{
			  sprintf(mRFC822, "(NIL NIL NIL NIL NIL NIL NIL NIL NIL NIL) (("); // 共通(BODY/BODYSTRUCTURE)
				} 
				else if (!stricmp(PS[i+n].mBType, "\"text\""))
				{
			  if (bExtend)
			  {
			  sprintf(mRFC822, "(NIL NIL NIL NIL NIL NIL NIL NIL NIL NIL) %s(%s %s %s %s %s %s %I64u %lu %s NIL NIL NIL)",
				                    ( (n == 1) ? "" : "("),
                                    (PS[i+n].mBType[0] ? strupr(PS[i+n].mBType): "NIL"),
                                    (PS[i+n].mBSub[0] ? strupr(PS[i+n].mBSub): "NIL"),
                                    (PS[i+n].mBPara[0] ? PS[i+n].mBPara: "NIL"),
                                    (PS[i+n].mBID[0] ? PS[i+n].mBID: "NIL"),
                                    "NIL",
					  			    (PS[i+n].mBEnc[0] ? PS[i+n].mBEnc: "NIL"),
									PS[i+n].uBody.QuadPart,
									PS[i+n].nLine,
									(PS[i+n].mBDisp[0] ? PS[i+n].mBDisp: "NIL"));
			  } else {
			  sprintf(mRFC822, "(NIL NIL NIL NIL NIL NIL NIL NIL NIL NIL) %s(%s %s %s %s %s %s %I64u %lu)",
				                    ( (n == 1) ? "" : "("),
                                    (PS[i+n].mBType[0] ? strupr(PS[i+n].mBType): "NIL"),
                                    (PS[i+n].mBSub[0] ? strupr(PS[i+n].mBSub): "NIL"),
                                    (PS[i+n].mBPara[0] ? PS[i+n].mBPara: "NIL"),
                                    (PS[i+n].mBID[0] ? PS[i+n].mBID: "NIL"),
                                    "NIL",
					  			    (PS[i+n].mBEnc[0] ? PS[i+n].mBEnc: "NIL"),
									PS[i+n].uBody.QuadPart,
									PS[i+n].nLine);
			  }
				}
				else if (!stricmp(PS[i+n].mBType, "\"message\"") && !stricmp(PS[i+n].mBSub, "\"rfc822\""))
				{
			  if (bExtend)
			  {
			  sprintf(mRFC822, "(NIL NIL NIL NIL NIL NIL NIL NIL NIL NIL) %s(%s %s %s %s %s %s %I64u (NIL NIL NIL NIL NIL NIL NIL NIL NIL NIL)",
				                    ( (n == 1) ? "" : "("),
                                    (PS[i+n].mBType[0] ? strupr(PS[i+n].mBType): "NIL"),
                                    (PS[i+n].mBSub[0] ? strupr(PS[i+n].mBSub): "NIL"),
                                    (PS[i+n].mBPara[0] ? PS[i+n].mBPara: "NIL"),
                                    (PS[i+n].mBID[0] ? PS[i+n].mBID: "NIL"),
                                    "NIL",
					  			    (PS[i+n].mBEnc[0] ? PS[i+n].mBEnc: "NIL"),
									PS[i+n].uBody.QuadPart);
			  } else {
			  sprintf(mRFC822, "(NIL NIL NIL NIL NIL NIL NIL NIL NIL NIL) %s(%s %s %s %s %s %s %I64u (NIL NIL NIL NIL NIL NIL NIL NIL NIL NIL)",
				                    ( (n == 1) ? "" : "("),
                                    (PS[i+n].mBType[0] ? strupr(PS[i+n].mBType): "NIL"),
                                    (PS[i+n].mBSub[0] ? strupr(PS[i+n].mBSub): "NIL"),
                                    (PS[i+n].mBPara[0] ? PS[i+n].mBPara: "NIL"),
                                    (PS[i+n].mBID[0] ? PS[i+n].mBID: "NIL"),
                                    "NIL",
					  			    (PS[i+n].mBEnc[0] ? PS[i+n].mBEnc: "NIL"),
									PS[i+n].uBody.QuadPart);
			  }
				} else {
			  if (bExtend)
			  {
			  sprintf(mRFC822, "(NIL NIL NIL NIL NIL NIL NIL NIL NIL NIL) %s(%s %s %s %s %s %s %I64u NIL %s NIL NIL)",
				                    ( (n == 1) ? "" : "("),
                                    (PS[i+n].mBType[0] ? strupr(PS[i+n].mBType): "NIL"),
                                    (PS[i+n].mBSub[0] ? strupr(PS[i+n].mBSub): "NIL"),
                                    (PS[i+n].mBPara[0] ? PS[i+n].mBPara: "NIL"),
                                    (PS[i+n].mBID[0] ? PS[i+n].mBID: "NIL"),
                                    "NIL",
					  			    (PS[i+n].mBEnc[0] ? PS[i+n].mBEnc: "NIL"),
									PS[i+n].uBody.QuadPart,
									(PS[i+n].mBDisp[0] ? PS[i+n].mBDisp: "NIL"));
			  } else {
			  sprintf(mRFC822, "(NIL NIL NIL NIL NIL NIL NIL NIL NIL NIL) %s(%s %s %s %s %s %s %I64u)",
				                    ( (n == 1) ? "" : "("),
                                    (PS[i+n].mBType[0] ? strupr(PS[i+n].mBType): "NIL"),
                                    (PS[i+n].mBSub[0] ? strupr(PS[i+n].mBSub): "NIL"),
                                    (PS[i+n].mBPara[0] ? PS[i+n].mBPara: "NIL"),
                                    (PS[i+n].mBID[0] ? PS[i+n].mBID: "NIL"),
                                    "NIL",
					  			    (PS[i+n].mBEnc[0] ? PS[i+n].mBEnc: "NIL"),
									PS[i+n].uBody.QuadPart);
			  }
				}
			  } else {
                n = 1;
			  if (bExtend)
			  {
			  sprintf(mRFC822, "(NIL NIL NIL NIL NIL NIL NIL NIL NIL NIL) %s(%s %s %s %s %s %s %I64u %lu NIL NIL NIL NIL) %lu NIL",
				                    ( (n == 1) ? "" : "("),
                                    (PS[i+n].mBType[0] ? strupr(PS[i+n].mBType): "NIL"),
                                    (PS[i+n].mBSub[0] ? strupr(PS[i+n].mBSub): "NIL"),
                                    (PS[i+n].mBPara[0] ? PS[i+n].mBPara: "NIL"),
                                    (PS[i+n].mBID[0] ? PS[i+n].mBID: "NIL"),
                                    "NIL",
					  			    (PS[i+n].mBEnc[0] ? PS[i+n].mBEnc: "NIL"),
									PS[i+n].uBody.QuadPart,
                                    PS[i+n].nLine,
									PS[i+n].nHLine+PS[i+n].nLine);
			  } else {
			  sprintf(mRFC822, "(NIL NIL NIL NIL NIL NIL NIL NIL NIL NIL) %s(%s %s %s %s %s %s %I64u %lu) %lu",
				                    ( (n == 1) ? "" : "("),
                                    (PS[i+n].mBType[0] ? strupr(PS[i+n].mBType): "NIL"),
                                    (PS[i+n].mBSub[0] ? strupr(PS[i+n].mBSub): "NIL"),
                                    (PS[i+n].mBPara[0] ? PS[i+n].mBPara: "NIL"),
                                    (PS[i+n].mBID[0] ? PS[i+n].mBID: "NIL"),
                                    "NIL",
					  			    (PS[i+n].mBEnc[0] ? PS[i+n].mBEnc: "NIL"),
									PS[i+n].uBody.QuadPart,
                                    PS[i+n].nLine,
									PS[i+n].nHLine+PS[i+n].nLine);
			  }
			  }
			  PS[i+n].uBody.QuadPart = PS[i+n].uHead.QuadPart+PS[i+n].uBody.QuadPart;
			  sprintf(mBody, "%lu", PS[i+n].uBody.QuadPart);
			}

#else
#ifdef UPDATE_20150630 // MESSAGE/RFC822の構造出力への対応
		    sprintf(mRFC822, "(NIL NIL NIL NIL NIL NIL NIL NIL NIL NIL) (\"TEXT\" \"PLAIN\" NIL NIL NIL NIL NIL NIL NIL NIL NIL) NIL NIL");
            if (!stricmp(PS[i].mBType, "\"message\"") && !stricmp(PS[i].mBSub, "\"rfc822\""))
			{
			  if (bExtend)
			  {
			  sprintf(mRFC822, "(NIL NIL NIL NIL NIL NIL NIL NIL NIL NIL) (%s %s %s %s %s %s %I64u %lu NIL NIL NIL) %lu NIL",
                                    (PS[i].PSM.mBType[0] ? strupr(PS[i].PSM.mBType): "NIL"),
                                    (PS[i].PSM.mBSub[0] ? strupr(PS[i].PSM.mBSub): "NIL"),
                                    (PS[i].PSM.mBPara[0] ? PS[i].PSM.mBPara: "NIL"),
                                    (PS[i].PSM.mBID[0] ? PS[i].PSM.mBID: "NIL"),
                                    "NIL",
					  			    (PS[i].PSM.mBEnc[0] ? PS[i].PSM.mBEnc: "NIL"),
									PS[i].PSM.uBody.QuadPart,
                                    PS[i].PSM.nLine,
									PS[i].PSM.nHLine+PS[i].PSM.nLine);
			                        PS[i].uBody.QuadPart = PS[i].PSM.uHead.QuadPart+PS[i].PSM.uBody.QuadPart;
									sprintf(mBody, "%lu", PS[i].uBody.QuadPart);
			  } else {
			  sprintf(mRFC822, "(NIL NIL NIL NIL NIL NIL NIL NIL NIL NIL) (%s %s %s %s %s %s %I64u %lu) %lu",
                                    (PS[i].PSM.mBType[0] ? strupr(PS[i].PSM.mBType): "NIL"),
                                    (PS[i].PSM.mBSub[0] ? strupr(PS[i].PSM.mBSub): "NIL"),
                                    (PS[i].PSM.mBPara[0] ? PS[i].PSM.mBPara: "NIL"),
                                    (PS[i].PSM.mBID[0] ? PS[i].PSM.mBID: "NIL"),
                                    "NIL",
					  			    (PS[i].PSM.mBEnc[0] ? PS[i].PSM.mBEnc: "NIL"),
									PS[i].PSM.uBody.QuadPart,
                                    PS[i].PSM.nLine,
									PS[i].PSM.nHLine+PS[i].PSM.nLine);
			                        PS[i].uBody.QuadPart = PS[i].PSM.uHead.QuadPart+PS[i].PSM.uBody.QuadPart;
									sprintf(mBody, "%lu", PS[i].uBody.QuadPart);
			  }
			}
#endif
#endif
	      if (bExtend) // BODYSTRUCTURE
		  {
#ifdef UPDATE_20150712 // MIME構造の階層管理に親パート番号を設定できる項目を追加した。
			if (nMESSRFC822==1)
			{
            sprintf(pContext->mMessages, "(%s %s %s %s %s %s %I64u %s%s",
                                    (PS[i].mBType[0] ? strupr(PS[i].mBType): "NIL"),
                                    (PS[i].mBSub[0] ? strupr(PS[i].mBSub): "NIL"),
                                    (PS[i].mBPara[0] ? PS[i].mBPara: "NIL"),
                                    (PS[i].mBID[0] ? PS[i].mBID: "NIL"),
                                    "NIL",
					  			    (PS[i].mBEnc[0] ? PS[i].mBEnc: "NIL"),
                                    PS[i].uBody.QuadPart,
                                   (!stricmp(PS[i].mBType, "\"text\"") ? (PS[i].nLine ? mRec: "0") : ""),
                                   (!stricmp(PS[i].mBSub, "\"rfc822\"") ? mRFC822 : (!stricmp(PS[i].mBType, "\"text\"") ? " NIL" : "NIL"))
                                   );
			  nMESSRFC822++;
			} else 
#endif
			{
#ifdef UPDATE_20150617B // 応答に"NIL"追加
            sprintf(pContext->mMessages, "(%s %s %s %s %s %s %I64u %s%s %s %s %s)%s",
#else
            sprintf(pContext->mMessages, "(%s %s %s %s %s %s %s %s%s %s %s)%s",
#endif
                                    (PS[i].mBType[0] ? strupr(PS[i].mBType): "NIL"),
                                    (PS[i].mBSub[0] ? strupr(PS[i].mBSub): "NIL"),
                                    (PS[i].mBPara[0] ? PS[i].mBPara: "NIL"),
                                    (PS[i].mBID[0] ? PS[i].mBID: "NIL"),
                                    "NIL",
					  			    (PS[i].mBEnc[0] ? PS[i].mBEnc: "NIL"),
#ifdef UPDATE_20150701A // 本文が0バイトの応答にNILを出力しないようにした。
                                   PS[i].uBody.QuadPart, // ? mBody: "0"),
                                   (!stricmp(PS[i].mBType, "\"text\"") ? (PS[i].nLine ? mRec: "0") : ""),
#else
                                   (PS[i].uBody.QuadPart ? mBody: "NIL"),
                                   (!stricmp(PS[i].mBType, "\"text\"") ? (PS[i].nLine ? mRec: "NIL") : ""),
#endif
#ifdef UPDATE_20150630 // MESSAGE/RFC822の構造出力への対応
                                   (!stricmp(PS[i].mBSub, "\"rfc822\"") ? mRFC822 : (!stricmp(PS[i].mBType, "\"text\"") ? " NIL" : "NIL")),
#else
#ifdef UPDATE_20091129H // MESSAGE/RFC822の添付表示 CatchMe@Mailで添付きメールの表示が失敗する
                                   (!stricmp(PS[i].mBSub, "\"rfc822\"") ? "(NIL NIL NIL NIL NIL NIL NIL NIL NIL NIL) (NIL NIL NIL NIL NIL NIL NIL NIL NIL NIL NIL) NIL NIL" : (!stricmp(PS[i].mBType, "\"text\"") ? " NIL" : "NIL")),
                                   //(!stricmp(PS[i].mBSub, "\"rfc822\"") ? "(\"Sun, 29 Nov 2009 11:28:01 +0900 (JST)\" {34}\r\n=?ISO-2022-JP?B?GyRCRTpJVSMyGyhC?= ((NIL NIL \"user3\" \"test-sample01.jp\")) ((NIL NIL \"user3\" \"test-sample01.jp\")) ((NIL NIL \"user3\" \"test-sample01.jp\")) ((NIL NIL \"user3\" \"test-sample01.jp\")) NIL NIL NIL \"<28899428.1259461681687.JavaMail.user3@test-sample01.jp>\") (\"TEXT\" \"plain\" (\"charset\" \"ISO-2022-JP\") NIL NIL \"7bit\" 17 3 NIL NIL NIL) 18 NIL" : (!stricmp(PS[i].mBType, "\"text\"") ? " NIL" : "NIL")),
#else
                                   (!stricmp(PS[i].mBType, "\"text\"") ? " NIL" : "NIL"),
#endif
#endif
                                   (PS[i].mBDisp[0] ? PS[i].mBDisp: "NIL"),
								   "NIL",
#ifdef UPDATE_20150617B // 応答に"NIL"追加
                                   "NIL",
#endif
#ifdef UPDATE_20091129C // CatchMe@Mailで添付きメールの表示が失敗する
								   (i == 0 ? "" : ""));
#else
								   (i == 0 ? "" : " "));
#endif
			}
		  } else {         // BODY
#ifdef UPDATE_20150712 // MIME構造の階層管理に親パート番号を設定できる項目を追加した。
			if (nMESSRFC822==1)
			{
            sprintf(pContext->mMessages, "(%s %s %s %s %s %s %I64u %s%s",
                                    (PS[i].mBType[0] ? strupr(PS[i].mBType): "NIL"),
                                    (PS[i].mBSub[0] ? strupr(PS[i].mBSub): "NIL"),
                                    (PS[i].mBPara[0] ? PS[i].mBPara: "NIL"),
                                    (PS[i].mBID[0] ? PS[i].mBID: "NIL"),
                                    "NIL",
					  			    (PS[i].mBEnc[0] ? PS[i].mBEnc: "NIL"),
                                    PS[i].uBody.QuadPart,
                                   (!stricmp(PS[i].mBType, "\"text\"") ? (PS[i].nLine ? mRec: "0") : ""),
                                   (!stricmp(PS[i].mBSub, "\"rfc822\"") ? mRFC822 : "")
                                   );
			  nMESSRFC822++;
			} else 
#endif
			{
            sprintf(pContext->mMessages, "(%s %s %s %s %s %s %I64u%s%s%s%s)%s",
                                    (PS[i].mBType[0] ? strupr(PS[i].mBType): "NIL"),
                                    (PS[i].mBSub[0] ? strupr(PS[i].mBSub): "NIL"),
                                    (PS[i].mBPara[0] ? PS[i].mBPara: "NIL"),
                                    (PS[i].mBID[0] ? PS[i].mBID: "NIL"),
                                    "NIL",
					  			    (PS[i].mBEnc[0] ? PS[i].mBEnc: "NIL"),
#ifdef UPDATE_20150701A // 本文が0バイトの応答にNILを出力しないようにした。
                                    PS[i].uBody.QuadPart, // ? mBody: "0"),
#ifdef UPDATE_20150712A // MIME構造の階層管理に親パート番号を設定できる項目を追加した。
								   (!stricmp(PS[i].mBType, "\"text\"") ? " " : ""),
#endif
                                   (!stricmp(PS[i].mBType, "\"text\"") ? (PS[i].nLine ? mRec: "0") : ""),
#else
                                   (PS[i].uBody.QuadPart ? mBody: "NIL"),
                                   (!stricmp(PS[i].mBType, "\"text\"") ? (PS[i].nLine ? mRec: "NIL") : ""),
#endif
								   (!stricmp(PS[i].mBSub, "\"rfc822\"") ? " " : ""),
								   (!stricmp(PS[i].mBSub, "\"rfc822\"") ? mRFC822 : ""),
                                   //(!stricmp(PS[i].mBType, "\"text\"") ? " NIL" : "NIL"),
                                   //(PS[i].mBDisp[0] ? PS[i].mBDisp: "NIL"),
#ifdef UPDATE_20091129C // CatchMe@Mailで添付きメールの表示が失敗する
								   (i == 0 ? "" : ""));
#else
								   (i == 0 ? "" : " "));
#endif
			}
		  }
		}
	  }
      put_reply(lpClientContext, FALSE, TRUE);
#ifdef UPDATE_20150712 // MIME構造の階層管理に親パート番号を設定できる項目を追加した。
      if (!stricmp(PS[i].mBType, "\"message\"") && !stricmp(PS[i].mBSub, "\"rfc822\""))
	  {
	    i += n;
#ifdef UPDATE_20180110 // rfc822構造添付ファイルに複雑なMIME構造の生成に失敗する場合があった
		if (!stricmp(PS[i+1].mBType, "\"multipart\"") && !stricmp(PS[i+1].mBSub, "\"alternative\""))
		{
		  i++;
		  //nDepath--;
		  sprintf(pContext->mMessages, "(");
          put_reply(lpClientContext, FALSE, TRUE);
		}
#endif
	  } 
#endif
#ifdef UPDATE_20150712A // MIME構造の階層管理に親パート番号を設定できる項目を追加した。
	  i += n2;
	  n2 = 0;
#endif
    }
  }

#ifdef UPDATE_20150712D // MIMEパートの終わり判定が抜ける対策
  if ((long)nDepath > 0) // || stricmp(PS[nPart].mBType, "end_multipart")) 
#else  
#ifdef UPDATE_20150712 // MIME構造の階層管理に親パート番号を設定できる項目を追加した。
  //余分に出力される問題あり
  if (nPart > 0 && !stricmp(PS[1].mBType, "\"multipart\"")) // || stricmp(PS[nPart].mBType, "end_multipart")) 
#else
  if (nPart > 0) 
#endif
#endif
  {
    if (bExtend) // BODYSTRUCTURE
#ifdef UPDATE_20150617B // 応答に"NIL"追加
#ifdef UPDATE_20150616 // BODYSTRUCTUREのマルチパート構造の応答時に半角スペースの区切りが挿入されない箇所があった
      sprintf(pContext->mMessages, " %s %s NIL NIL NIL",
#else
      sprintf(pContext->mMessages, "%s %s NIL NIL NIL",
#endif
#else
#ifdef UPDATE_20150616 // BODYSTRUCTUREのマルチパート構造の応答時に半角スペースの区切りが挿入されない箇所があった
      sprintf(pContext->mMessages, " %s %s NIL NIL",
#else
      sprintf(pContext->mMessages, "%s %s NIL NIL",
#endif
#endif
                                 (PS[0].mBSub[0] ? strupr(PS[0].mBSub): "NIL"),
                                 (PS[0].mBPara[0] ? PS[0].mBPara: "NIL"));
     else         // BODY
#ifdef UPDATE_20150616 // BODYSTRUCTUREのマルチパート構造の応答時に半角スペースの区切りが挿入されない箇所があった
       sprintf(pContext->mMessages, " %s", (PS[0].mBSub[0] ? strupr(PS[0].mBSub): "NIL"));
#else
       sprintf(pContext->mMessages, "%s", (PS[0].mBSub[0] ? strupr(PS[0].mBSub): "NIL"));
#endif
    put_reply(lpClientContext, FALSE, TRUE);
  }
  ///////////////////////////////////////////
#ifdef UPDATE_20150712D // MIMEパートの終わり判定が抜ける対策
  if ((long)nDepath > 0)
#else
  if (nPart > 0) 
#endif
  {
    strcpy(pContext->mMessages, ")");
    put_reply(lpClientContext, FALSE, TRUE);
  }

}

#ifdef UPDATE_20150701 // MESSAGE/RFC822の構造出力への対応(入れ子)
#ifdef UPDATE_20150712 // MIME構造の階層管理に親パート番号を設定できる項目を追加した。
void fetch_bodystructure_2(PCLIENT_CONTEXT lpClientContext, DWORD *pnPart, PartStruct *pPS, BOOL bExtend, FILE *fp)
#else
void fetch_bodystructure_2(PCLIENT_CONTEXT lpClientContext, PartStruct *pPS, BOOL bExtend, FILE *fp)
#endif
{
  PImap4Context    pContext = &lpClientContext->Context;
  char             mFn[MAX_PATH], mLine[1024], mBound[BODY_PART_LIMIT][256];
  char             mBody[32], mRec[32];
  DWORD            n, i, j[BODY_PART_LIMIT], nPart = 0, nDepath = 0;
  BOOL             bBody = FALSE;

#ifdef UPDATE_20150712 // MIME構造の階層管理に親パート番号を設定できる項目を追加した。
  PartStruct    PS[BODY_PART_LIMIT]; //
#else
  PPartStruct   PS = pPS;
#endif
#ifdef UPDATE_20150708B // MESSAGE/RFC822の構造出力結果がバッファオーバフローする可能性のある不具合。
  char             mRFC822[1024];
#else
  char             mRFC822[128];
#endif

  //sprintf(pContext->mMessages, " (");
  //put_reply(lpClientContext, FALSE, TRUE);
#ifdef UPDATE_20150712 // MIME構造の階層管理に親パート番号を設定できる項目を追加した。
  nPart = *pnPart+1;
#else
  for (i = 0; i < BODY_PART_LIMIT; i++) {
   strcpy(PS[i].mBType, "\"TEXT\""); // ボディタイプ Content-Type: text/plain
   strcpy(PS[i].mBSub, "\"PLAIN\""); // ボディサブタイプ 
    PS[i].mBPara[0] = '\x0';
    PS[i].mBID[0]   = '\x0';
    PS[i].mBDisp[0] = '\x0';
    PS[i].mBEnc[0]  = '\x0';
    PS[i].uBody.QuadPart = 0;
    PS[i].uHead.QuadPart = 0;
	PS[i].nHLine         = 0;
    PS[i].nLine          = 0;
#ifdef UPDATE_20150630 // MESSAGE/RFC822の構造出力への対応
	strcpy(PS[i].PSM.mBType, "\"TEXT\""); // ボディタイプ Content-Type: text/plain
    strcpy(PS[i].PSM.mBSub, "\"PLAIN\""); // ボディサブタイプ 
    PS[i].PSM.mBPara[0] = '\x0';          // ボディパラメータ括弧つきリスト ("name" "test.bin")
	PS[i].PSM.mBID[0] = '\x0';            // ボディ ID Content-ID: <Mm3V1n13r86nICg78>
    PS[i].PSM.mBDisp[0] = '\x0';          // ボディ記述 Content-Disposition: attachment;
    strcpy(PS[i].PSM.mBEnc, "\"7bit\"");  // ボディ符号化 Content-Encoding: US-ASCII,base64
    PS[i].PSM.uHead.QuadPart = 0;         // ヘッダサイズ
	PS[i].PSM.uBody.QuadPart = 0;         // ボディサイズ：
	PS[i].PSM.nHLine = 0;                 // ヘッダ部行数;
	PS[i].PSM.nLine = 0;                  // 本文行数;
#endif
    mBound[i][0] = '\x0';
	j[i] = 0;
  }
#endif
  sprintf(mFn, "%s/%s", pContext->mSelectDir, (pContext->bEncMSG ? pContext->mDecFileName : pContext->FindData.cFileName));
#ifdef UPDATE_20230706 // FETCH時のメールデータを事前に一括して読込み検索するフラグを追加
  fp = NULL;
  if (pContext->pBuff) // ファイルの先頭にポインタリセット
    pContext->pfBuff = pContext->pBuff;
  if (pContext->pBuff || !pContext->pBuff && (fp = fopen(mFn, "rb"))) 
#else
  if (fp)
#endif
  {
#ifdef UPDATE_20150712 // MIME構造の階層管理に親パート番号を設定できる項目を追加した。
	(PS+nPart)->nPPart = *pnPart; // 親パートNO設定
#endif
	PS->PSM.nHLine = default_header(lpClientContext, &pPS->PSM, mLine, sizeof(mLine), mBound[nDepath], fp); // 最初のヘッダ
	BODY_COUNTER(lpClientContext, &pPS->PSM, mLine, sizeof(mLine), mBound[nDepath], fp);   // BODYSIZE
	if (mBound[0][0] != '\x0') { // マルチパートなら次の処理
      //sprintf(pContext->mMessages, "(");
	  //put_reply(lpClientContext, FALSE, TRUE);
      do {
	    nPart++;
		if (nPart >= BODY_PART_LIMIT)
		{
		  nPart = BODY_PART_LIMIT-1;
		  break;
		}
		strcpy(PS[nPart].mBType, "end_multipart");
#ifdef UPDATE_20150712 // MIME構造の階層管理に親パート番号を設定できる項目を追加した。
        nPart += multipart_header(lpClientContext, nPart, PS, &PS[nPart], mLine, sizeof(mLine), mBound[nDepath], mBound[nDepath+1], fp , bExtend); // マルチパート
#else
#ifdef UPDATE_20150710 // MESSAGE/RFC822の構造出力への対応(入れ子の解析)でTEXT以外構造が出力されなかった。
        multipart_header(lpClientContext, nPart, PS, &PS[nPart], mLine, sizeof(mLine), mBound[nDepath], mBound[nDepath+1], fp , bExtend); // マルチパート
#else
#ifdef UPDATE_20150708 // MIME構造で(multipart(text,multipart(html))のような構造で順番道理の結果が返せない不具合。
        multipart_header(lpClientContext, (nPart > 0 ? &PS[nPart-1] : NULL), &PS[nPart], mLine, sizeof(mLine), mBound[nDepath], mBound[nDepath+1], fp , bExtend); // マルチパート
#else
        multipart_header(lpClientContext, &PS[nPart], mLine, sizeof(mLine), mBound[nDepath], mBound[nDepath+1], fp , bExtend); // マルチパート
#endif
#endif
#endif
		if (mBound[nDepath+1][0] && strcmp(mBound[nDepath], mBound[nDepath+1]))
		  nDepath++;
	    BODY_COUNTER(lpClientContext, &PS[nPart], mLine, sizeof(mLine), mBound[nDepath], fp);   // BODYSIZE
if (!strcmp(pPS->PSM.mBType, "\"multipart\"") && !strcmp(PS[nPart].mBType, "\"text\""))
{
    strcpy(pPS->PSM.mBType,  PS[nPart].mBType);
    strcpy(pPS->PSM.mBSub, PS[nPart].mBSub);
    strcpy(pPS->PSM.mBPara,  PS[nPart].mBPara);
    strcpy(pPS->PSM.mBID, PS[nPart].mBID);
    strcpy(pPS->PSM.mBDisp, PS[nPart].mBDisp);
    strcpy(pPS->PSM.mBEnc, PS[nPart].mBEnc);
    pPS->PSM.uBody.QuadPart = PS[nPart].uBody.QuadPart;
    pPS->PSM.uHead.QuadPart = PS[nPart].uHead.QuadPart;
	pPS->PSM.nHLine = PS[nPart].nHLine;
    pPS->PSM.nLine = PS[nPart].nLine;
}
		if (strstr(mLine, mBound[nDepath]) && strstr(&mLine[strlen(mBound[nDepath])],"--")) {
		  if (nDepath > 0) {
            mBound[nDepath][0] = '\x0';
			nDepath--;
		  } else
		    break;
		}
	  } while (strcmp(mLine, ".\r\n") && !feof(fp) );

	  { // 先頭のContent-Typeは読み飛ばす
	    n = 0;
	    for (i = 1; i <= nPart; i++) {
		  if (!strcmp(PS[i].mBType, "\"multipart\"")) {
		    n++;
		  } else if (!strcmp(PS[i].mBType, "end_multipart")) {
		    if (n == 0) {
              strcpy(pContext->mMessages, "");
	          PS[i].mBType[0] = '\x0';  // 最後を揃える
		      nPart = i;
		      break;
			} else
			  n--;
		  }
		}
	  }
	} 
  }
#ifdef UPDATE_20150712 // MIME構造の階層管理に親パート番号を設定できる項目を追加した。
  *pnPart =nPart;
#endif
}
#endif

#ifdef UPDATE_20150709 // MESSAGE/RFC822の構造出力への対応(入れ子の解析)でTEXT以外構造が出力されなかった。
#ifdef UPDATE_20150710 // MESSAGE/RFC822の構造出力への対応(入れ子の解析)でTEXT以外構造が出力されなかった。
#ifdef UPDATE_20150711 // MESSAGE/RFC822の構造出力への対応(入れ子の解析)でTEXT以外構造が出力されなかった。
#ifdef UPDATE_20150712 // MIME構造の階層管理に親パート番号を設定できる項目を追加した。
int  message_bodystructure(PCLIENT_CONTEXT lpClientContext, DWORD *pnPart, PPartStruct pPS, char *pLine,  FILE *fp, char *pBound, BOOL bExtend)
#else
int  message_bodystructure(PCLIENT_CONTEXT lpClientContext,char *pLine,  FILE *fp, char *pBound, BOOL bExtend)
#endif
#else
void message_bodystructure(PCLIENT_CONTEXT lpClientContext,char *pLine,  FILE *fp, char *pBound, BOOL bExtend)
#endif
#else
void message_bodystructure(PCLIENT_CONTEXT lpClientContext,char *pLine, FILE *fp, BOOL bExtend)
#endif
{
  PImap4Context    pContext = &lpClientContext->Context;
  char             mFn[MAX_PATH], mLine[1024], mBound[BODY_PART_LIMIT][256];
  char             mBody[32], mRec[32];
  DWORD            n, i, j[BODY_PART_LIMIT], nPart = 0, nDepath = 0;
  BOOL             bBody = FALSE;
  long             cur;
  char             *p;

#ifndef UPDATE_20150712 // MIME構造の階層管理に親パート番号を設定できる項目を追加した。
  PartStruct    PS[BODY_PART_LIMIT]; //
#else
  PPartStruct   PS = pPS; //
#endif
#ifdef UPDATE_20150710 // MESSAGE/RFC822の構造出力への対応(入れ子の解析)でTEXT以外構造が出力されなかった。
  long             cur2;
  BOOL bBoundSet = FALSE;
  BOOL bEnd = FALSE;
  char mEndBound[256] = "";
#endif
#ifdef UPDATE_20150711 // MESSAGE/RFC822の構造出力への対応(入れ子の解析)でTEXT以外構造が出力されなかった。
  int nCloseType = 0;
#endif

#ifdef UPDATE_20150710 // MESSAGE/RFC822の構造出力への対応(入れ子の解析)でTEXT以外構造が出力されなかった。
  if (*pBound)
  {
    sprintf(mEndBound, "%s--", pBound);
  }
#endif
 
  //sprintf(pContext->mMessages, "(" );
  //put_reply(lpClientContext, FALSE, TRUE);
#ifdef UPDATE_20150712 // MIME構造の階層管理に親パート番号を設定できる項目を追加した。
  nPart = *pnPart;
#else
  for (i = 0; i < BODY_PART_LIMIT; i++) {
#ifdef UPDATE_20150630 // MESSAGE/RFC822の構造出力への対応
   strcpy(PS[i].mBType, "\"TEXT\""); // ボディタイプ Content-Type: text/plain
   strcpy(PS[i].mBSub, "\"PLAIN\""); // ボディサブタイプ 
#else
    PS[i].mBType[0] = '\x0';
    PS[i].mBSub[0]  = '\x0';
#endif
    PS[i].mBPara[0] = '\x0';
    PS[i].mBID[0]   = '\x0';
    PS[i].mBDisp[0] = '\x0';
    PS[i].mBEnc[0]  = '\x0';
    PS[i].uBody.QuadPart = 0;
#ifdef UPDATE_20150630 // MESSAGE/RFC822の構造出力への対応
    PS[i].uHead.QuadPart = 0;
	PS[i].nHLine         = 0;
#endif
    PS[i].nLine          = 0;
#ifdef UPDATE_20150712      // MIME構造の階層管理に親パート番号を設定できる項目を追加した。
	PS[i].nPPart = 0;    // 親パート番号
#endif
#ifdef UPDATE_20150630 // MESSAGE/RFC822の構造出力への対応
	strcpy(PS[i].PSM.mBType, "\"TEXT\""); // ボディタイプ Content-Type: text/plain
    strcpy(PS[i].PSM.mBSub, "\"PLAIN\""); // ボディサブタイプ 
    PS[i].PSM.mBPara[0] = '\x0';          // ボディパラメータ括弧つきリスト ("name" "test.bin")
	PS[i].PSM.mBID[0] = '\x0';            // ボディ ID Content-ID: <Mm3V1n13r86nICg78>
    PS[i].PSM.mBDisp[0] = '\x0';          // ボディ記述 Content-Disposition: attachment;
    strcpy(PS[i].PSM.mBEnc, "\"7bit\"");  // ボディ符号化 Content-Encoding: US-ASCII,base64
    PS[i].PSM.uHead.QuadPart = 0;         // ヘッダサイズ
	PS[i].PSM.uBody.QuadPart = 0;         // ボディサイズ：
	PS[i].PSM.nHLine = 0;                 // ヘッダ部行数;
	PS[i].PSM.nLine = 0;                  // 本文行数;
#endif
#ifdef UPDATE_20150712      // MIME構造の階層管理に親パート番号を設定できる項目を追加した。
	PS[i].PSM.nPPart = 0;    // 親パート番号
#endif
    mBound[i][0] = '\x0';
	j[i] = 0;
  }
#endif
  //sprintf(mFn, "%s/%s", pContext->mSelectDir, (pContext->bEncMSG ? pContext->mDecFileName : pContext->FindData.cFileName));
  //strcpy(mLine, pLine);
#ifdef UPDATE_20230706 // FETCH時のメールデータを事前に一括して読込み検索するフラグを追加
  if (pContext->pBuff || !pContext->pBuff && fp)
#else
  if (fp) 
#endif
  {
#ifdef UPDATE_20230706 // FETCH時のメールデータを事前に一括して読込み検索するフラグを追加
    if (pContext->pfBufp)
	  cur = (long)(pContext->pfBuff-pContext->pfBufp);
	else
#endif
    cur = ftell(fp);
#ifdef UPDATE_20230706_DEBUG // FETCH時のメールデータを事前に一括して読込み検索するフラグを追加
if (bDebug) printf("message_bodystructure(0) cur=%lu\n", cur);
#endif
#ifdef UPDATE_20150712 // MIME構造の階層管理に親パート番号を設定できる項目を追加した。
	PS[nPart].nPPart = *pnPart;
#endif
#ifdef UPDATE_20150712      // MIME構造の階層管理に親パート番号を設定できる項目を追加した。
	PS[nPart].nHLine = default_header(lpClientContext, &PS[nPart], mLine, sizeof(mLine), mBound[nDepath], fp); // 最初のヘッダ
#else
	PS[nPart].PSM.nHLine = default_header(lpClientContext, &PS[nPart], mLine, sizeof(mLine), mBound[nDepath], fp); // 最初のヘッダ
#endif
#ifdef UPDATE_20150710 // MESSAGE/RFC822の構造出力への対応(入れ子の解析)でTEXT以外構造が出力されなかった。
    if (*pBound && !mBound[nDepath][0])
	{
      strcpy(mBound[nDepath], pBound);
	  bBoundSet = TRUE;
	}
#endif
	BODY_COUNTER(lpClientContext, &PS[nPart], mLine, sizeof(mLine), mBound[nDepath], fp);   // BODYSIZE
#ifdef UPDATE_20150710 // MESSAGE/RFC822の構造出力への対応(入れ子の解析)でTEXT以外構造が出力されなかった。
#ifdef UPDATE_20230706 // FETCH時のメールデータを事前に一括して読込み検索するフラグを追加
    if (pContext->pfBufp)
	  cur2 = (long)(pContext->pfBuff-pContext->pfBufp)-strlen(mLine);
	else
#endif
	cur2 = ftell(fp)-strlen(mLine);
#ifdef UPDATE_20230706_DEBUG // FETCH時のメールデータを事前に一括して読込み検索するフラグを追加
if (bDebug) printf("message_bodystructure() cur2=%lu\n", cur2);
#endif
    if (bBoundSet)
	{
      mBound[nDepath][0] = '\x0';
	}
#endif
	if (mBound[0][0] != '\x0')  // マルチパートなら次の処理
	{
      do {
	    nPart++;
		if (nPart >= BODY_PART_LIMIT)
		{
#ifdef UPDATE_20090325C // メッセージのBODYパート数の上限を超えた場合ハングする
		  nPart = BODY_PART_LIMIT-1;
#endif
		  break;
		}
		strcpy(PS[nPart].mBType, "end_multipart");
#ifdef UPDATE_20150710 // MESSAGE/RFC822の構造出力への対応(入れ子の解析)でTEXT以外構造が出力されなかった。
        nPart += multipart_header(lpClientContext, nPart, PS, &PS[nPart], mLine, sizeof(mLine), mBound[nDepath], mBound[nDepath+1], fp , bExtend); // マルチパート
#else
#ifdef UPDATE_20150708 // MIME構造で(multipart(text,multipart(html))のような構造で順番道理の結果が返せない不具合。
        nPart += multipart_header(lpClientContext, (nPart > 0 ? &PS[nPart-1] : NULL), &PS[nPart], mLine, sizeof(mLine), mBound[nDepath], mBound[nDepath+1], fp , bExtend); // マルチパート
#else
#ifdef UPDATE_20150620B // 多重入れ子構造がある場合に正しく抽出できない不具合
        multipart_header(lpClientContext, &PS[nPart], mLine, sizeof(mLine), mBound[nDepath], mBound[nDepath+1], fp , bExtend); // マルチパート
#else
        multipart_header(lpClientContext, &PS[nPart], mLine, sizeof(mLine), mBound[nDepath+1], fp, bExtend); // マルチパート
#endif
#endif
#endif
#ifndef UPDATE_20150712 // MIME構造の階層管理に親パート番号を設定できる項目を追加した。
		if (mBound[nDepath+1][0] && strcmp(mBound[nDepath], mBound[nDepath+1]))
		  nDepath++;
#endif
	    BODY_COUNTER(lpClientContext, &PS[nPart], mLine, sizeof(mLine), mBound[nDepath], fp);   // BODYSIZE
		if (strstr(mLine, mBound[nDepath]) && strstr(&mLine[strlen(mBound[nDepath])],"--")) {
#ifdef UPDATE_20150712 // MIME構造の階層管理に親パート番号を設定できる項目を追加した。
	      nPart++;
		  strcpy(PS[nPart].mBType,"end_multipart");
#endif
		  if (nDepath > 0) {
            mBound[nDepath][0] = '\x0';
			nDepath--;
		  } else
		    break;
		}
#ifdef UPDATE_20150710 // MESSAGE/RFC822の構造出力への対応(入れ子の解析)でTEXT以外構造が出力されなかった。
		if (mEndBound[0])
		{
		  bEnd = !strncmp(mLine, mEndBound, strlen(mEndBound)); 
		}
#endif
	  }
#ifdef UPDATE_20150710 // MESSAGE/RFC822の構造出力への対応(入れ子の解析)でTEXT以外構造が出力されなかった。
#ifdef UPDATE_20230706 // FETCH時のメールデータを事前に一括して読込み検索するフラグを追加
      while (strcmp(mLine, ".\r\n") && (p || (pContext->pfBuff ? *pContext->pfBuff : !feof(fp))) && !bEnd );
#else
	  while (strcmp(mLine, ".\r\n") && !feof(fp) && !bEnd );
#endif
#else
#ifdef UPDATE_20150620B // 多重入れ子構造がある場合に正しく抽出できない不具合
	 while (strcmp(mLine, ".\r\n") && !feof(fp) );
#else
	 while (strcmp(mLine, ".\r\n") || !feof(fp) );
#endif
#endif
	  //if (!strcmp(PS[0].mBType, "\"multipart\"")) {
	  { // 先頭のContent-Typeは読み飛ばす
	    n = 0;
	    for (i = 1; i <= nPart; i++) {
		  if (!strcmp(PS[i].mBType, "\"multipart\"")) {
		    n++;
		  } else if (!strcmp(PS[i].mBType, "end_multipart")) {
		    if (n == 0) {
              strcpy(pContext->mMessages, "");
	          PS[i].mBType[0] = '\x0';  // 最後を揃える
		      nPart = i;
		      break;
			} else
			  n--;
		  }
		}
	  }
	} 
  }

  //sprintf(pContext->mMessages, "%s %s", (bExtend ? "BODYSTRUCTURE" :"BODY") , (nPart == 0 ? "" : "("));
  //put_reply(lpClientContext, FALSE, TRUE);
  nDepath = 0;
  //for (i = (nPart == 0 ? 0 : 1); i <= nPart; i++) 
#ifdef UPDATE_20150712 // MIME構造の階層管理に親パート番号を設定できる項目を追加した。
  nPart++;
#else
  nPart = i = 0;
  {
	sprintf(mBody, "%lu", PS[i].uBody.QuadPart);
	sprintf(mRec, "%lu", PS[i].nLine);
	{
      if (PS[i].mBType[0] != '\x0' || mBound[0][0] == '\x0')
	  {
	    if (bExtend) // BODYSTRUCTURE
          sprintf(pContext->mMessages, " (%s %s %s %s %s %s %s %s%s ",
                                    (PS[i].mBType[0] ? strupr(PS[i].mBType): "NIL"),
                                    (PS[i].mBSub[0] ? strupr(PS[i].mBSub): "NIL"),
                                    (PS[i].mBPara[0] ? PS[i].mBPara: "NIL"),
                                    (PS[i].mBID[0] ? PS[i].mBID: "NIL"),
                                    "NIL",
					  			    (PS[i].mBEnc[0] ? PS[i].mBEnc: "NIL"),
                                   (PS[i].uBody.QuadPart ? mBody: "NIL"),
                                   (!stricmp(PS[i].mBType, "\"text\"") ? (PS[i].nLine ? mRec: "NIL") : ""),
                                   "(NIL NIL NIL NIL NIL NIL NIL NIL NIL NIL)");
	    else         // BODY
          sprintf(pContext->mMessages, " (%s %s %s %s %s %s %s %s)%s",
                                    (PS[i].mBType[0] ? strupr(PS[i].mBType): "NIL"),
                                    (PS[i].mBSub[0] ? strupr(PS[i].mBSub): "NIL"),
                                    (PS[i].mBPara[0] ? PS[i].mBPara: "NIL"),
                                    (PS[i].mBID[0] ? PS[i].mBID: "NIL"),
                                    "NIL",
					  			    (PS[i].mBEnc[0] ? PS[i].mBEnc: "NIL"),
                                   (PS[i].uBody.QuadPart ? mBody: "NIL"),
                                   "NIL",
								   (i == 0 ? "" : ""));
	  }
	}
    put_reply(lpClientContext, FALSE, TRUE);
  } 
#endif
  ////////////////////////
  // 位置あわせ
#ifdef UPDATE_20230706 // FETCH時のメールデータを事前に一括して読込み検索するフラグを追加
  if (pContext->pfBufp)
    pContext->pfBuff = pContext->pfBufp + (long)cur; 
  else
#endif
  fseek(fp, (long)cur, SEEK_SET); // 元に戻る
#ifdef UPDATE_20230706_DEBUG // FETCH時のメールデータを事前に一括して読込み検索するフラグを追加
if (bDebug) printf("message_bodystructure(0) SEEK_SET cur=%lu\n", cur);
#endif
  do
  {
#ifdef UPDATE_20230706 // FETCH時のメールデータを事前に一括して読込み検索するフラグを追加
     if (pContext->pfBufp)
	   p = mgets(mLine, sizeof(mLine)-1, &pContext->pfBuff);
	 else
#endif
     p = fgets(mLine, sizeof(mLine)-1, fp);
#ifdef UPDATE_20230706 // FETCH時のメールデータを事前に一括して読込み検索するフラグを追加
  } while ((p || (pContext->pfBuff ? *pContext->pfBuff : !feof(fp))) && !(mLine[0] == '\x0' || mLine[0] == '\r' || mLine[0] == '\n'));
#else
  }  while (!feof(fp) && !(mLine[0] == '\x0' || mLine[0] == '\r' || mLine[0] == '\n'));
#endif
  do
  {
#ifdef UPDATE_20230706 // FETCH時のメールデータを事前に一括して読込み検索するフラグを追加
     if (pContext->pfBufp)
	   p = mgets(mLine, sizeof(mLine)-1, &pContext->pfBuff);
	 else
#endif
     p = fgets(mLine, sizeof(mLine)-1, fp);
#ifdef UPDATE_20230706 // FETCH時のメールデータを事前に一括して読込み検索するフラグを追加
  } while ((p || (pContext->pfBuff ? *pContext->pfBuff : !feof(fp))) && (mLine[0] == '\x0' || mLine[0] == '\r' || mLine[0] == '\n'));
#else
  }  while (!feof(fp) && (mLine[0] == '\x0' || mLine[0] == '\r' || mLine[0] == '\n'));
#endif
#ifdef UPDATE_20230706 // FETCH時のメールデータを事前に一括して読込み検索するフラグを追加
  if (pContext->pfBufp)
    pContext->pfBuff -=(long)strlen(mLine); 
  else
#endif
  fseek(fp, (long)(0-strlen(mLine)), SEEK_CUR); // １行手前にもどる
#ifdef UPDATE_20230706 // FETCH時のメールデータを事前に一括して読込み検索するフラグを追加
if (bDebug) printf("message_bodystructure(1) SEEK_CUR strlen(mLine)=%lu\n", strlen(mLine));
#endif
  ////////////////////////
#ifdef UPDATE_20150712 // MIME構造の階層管理に親パート番号を設定できる項目を追加した。
   nPart = multipart_bodystructure(lpClientContext, &nPart, pPS, pLine, fp, pBound, bExtend, TRUE);
#else
#ifdef UPDATE_20150711 // MESSAGE/RFC822の構造出力への対応(入れ子の解析)でTEXT以外構造が出力されなかった。
  nCloseType = multipart_bodystructure(lpClientContext, *pLine, fp, pBound, bExtend, FALSE);
#else
#ifdef UPDATE_20150710 // MESSAGE/RFC822の構造出力への対応(入れ子の解析)でTEXT以外構造が出力されなかった。
  multipart_bodystructure(lpClientContext, *pLine, fp, pBound, bExtend, FALSE);
#else
  multipart_bodystructure(lpClientContext, *pLine, fp, bExtend, FALSE);
#endif
#endif
#endif
#ifndef UPDATE_20150712 // MIME構造の階層管理に親パート番号を設定できる項目を追加した。
  if (mBound[0][0] == '\x0')
  {
	 sprintf(pContext->mMessages, " %lu NIL", PS[i].PSM.nHLine+PS[i].PSM.nLine);
	 put_reply(lpClientContext, FALSE, TRUE);
  }
  if (bExtend) // BODYSTRUCTURE
  {
     sprintf(pContext->mMessages, " %s %s %s)%s",
                                   (PS[i].mBDisp[0] ? PS[i].mBDisp: "NIL"),
								   "NIL",
                                   "NIL",
								   "");
  } else {
    sprintf(pContext->mMessages, ")");
  }
  put_reply(lpClientContext, FALSE, TRUE);
#endif
#ifdef UPDATE_20150710 // MESSAGE/RFC822の構造出力への対応(入れ子の解析)でTEXT以外構造が出力されなかった。
  ////////////////////////
  // 位置あわせ
#ifdef UPDATE_20230706 // FETCH時のメールデータを事前に一括して読込み検索するフラグを追加
  if (pContext->pfBufp)
    pContext->pfBuff = pContext->pfBufp + (long)cur2;
  else
#endif
  fseek(fp, (long)cur2, SEEK_SET); // 元に戻る
#ifdef UPDATE_20230706_DEBUG // FETCH時のメールデータを事前に一括して読込み検索するフラグを追加
if (bDebug) printf("message_bodystructure(2) SEEK_SET cur2=%lu\n", cur2);
#endif
#endif
#ifdef UPDATE_20150712 // MIME構造の階層管理に親パート番号を設定できる項目を追加した。
  *pnPart = nPart;
  return nPart;
#else
#ifdef UPDATE_20150711 // MESSAGE/RFC822の構造出力への対応(入れ子の解析)でTEXT以外構造が出力されなかった。
  return nCloseType;
#endif
#endif
}
#endif


