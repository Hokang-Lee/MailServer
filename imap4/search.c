////////////////////////////////////////////////////////////
// SEARCH.C Copyright K.kawakami
////////////////////////////////////////////////////////////
#include "imap4.h"
#include "codepage.h"
#include "utf8.h"
/*
<message set>  指定されたメッセージ連番集合に対応するメッセージ連番を持つメッセージ。 
ALL            メールボックスの全てのメッセージ; AND するためのデフォルト初期キー。 

UID <message set> 指定されたユニーク識別子集合に対応するユニーク識別子を持つメッセージ。 

TEXT <string> 指定された文字列をメッセージのヘッダまたはボディに含むメッセージ。 
HEADER <field-name> <string> 指定された fielld-name ([RFC-822] で定義されている)で、指定された文字列を [RFC-822] field-body に含むヘッダーを持つメッセージ。 
BODY <string>  指定された文字列をメッセージのボディに含むメッセージ。 

BCC <string>   指定された文字列をエンベロープ構造の BCC フィールドに含むメッセージ。 
CC <string>    指定された文字列をエンベロープ構造の CC フィールドに含むメッセージ。 
FROM <string>  指定された文字列をエンベロープ構造の FROM フィールドに含むメッセージ。 
SUBJECT <string>  指定された文字列をエンベロープ構造の SUBJECT フィールドに含むメッセージ。 
TO <string> 指定された文字列をエンベロープ構造の TO フィールドに含むメッセージ。 

SENTBEFORE <date> [RFC-822] Date: ヘッダーが指定された日付よりも早いメッセージ。 
SENTON <date>  [RFC-822] Date: ヘッダーが指定された日付中のメッセージ。 
SENTSINCE <date>  [RFC-822] Date: ヘッダーが指定された日付中かそれより後のメッセージ。 

BEFORE <date>  内部日付が指定された日付よりも早いメッセージ。 
ON <date>      内部日付が指定された日付中のメッセージ。 
SINCE <date>  内部日付が指定した日付内またはそれより後のメッセージ。 

KEYWORD <flag> 指定されたキーワードが設定されたメッセージ。 
UNKEYWORD <flag> 指定されたキーワードが設定されていないメッセージ。 

LARGER <n>     [RFC-822] サイズが指定されたオクテット数よりも大きいメッセージ。 
SMALLER <n> [RFC-822] サイズが指定されたオクテット数よりも小さいメッセージ。 

NOT <search-key>  指定された検索キーにマッチしないメッセージ。 
OR <search-key1> <search-key2>  いずれかの検索キーにマッチするメッセージ。 

*/
extern char mMonth[12][4];
#ifdef UPDATE_20150608 // 検索時の日付をUTC日付かローカル日付かに変更できるオプションを追加した
extern BOOL	bSearchTime;
#endif
#ifdef UPDATE_20150609 // フォルダへのコピー時に受信日付がコピー日付になってしまった不具合 0:作成日時 1:更新日時 2:アクセス日時
extern int  nSearchType;
#endif
#ifdef UPDATE_20190510 // NTFS以外のファイルシステムからのファイル一覧取得可能にする対策（一覧ソート）を追加
extern BOOL   bOtherFS; // TRUE:対応する FALSE:対応しない（旧仕様）
#endif
#ifdef UPDATE_20230624A // SEARCH BODY命令で検索速度の向上対策:検索キーワード側を文字コード別に準備して比較する
extern BOOL  bDebug;
#endif
#ifdef UPDATE_20230627  // UPDATE_20230624の有効無効フラグを追加
extern BOOL  bBulkSearch;
#endif
#ifdef UPDATE_20231003 // データ受信時のCPU負荷を下げる対策
extern DWORD nPeekCoolTime;   //0:無効 デフォルト 50 100 500 1000 5000 10000
#endif

int CODEPAGE2UTF8N(INT copdepage, const char * ssrc,char * sdst,int idstsz);
int  Base64DecodeLine(char * pszLine, int nLineLen, unsigned char * uchBuffer, int nBufferLen);
DWORD MSGDecipher(PCLIENT_CONTEXT lpClientContext, char *pRequest, char **pDec);
#ifdef SEARCH_NO_RESS_TEST // 使用不可のオプションでNOを返す
int SearchMSGByCondition(PCLIENT_CONTEXT lpClientContext, char **pDec);
#else
void  SearchMSGByCondition(PCLIENT_CONTEXT lpClientContext, char **pDec);
#endif
void  search_falgs(PImap4Context pContext, DWORD nToken, IMAP4STK *stk);
void  search_number(PImap4Context pContext, DWORD nToken, DWORD nMSG, IMAP4STK *stk);
void  search_internaldate(PImap4Context pContext, DWORD nToken, IMAP4STK *stk);
void  search_size(PImap4Context pContext, DWORD nToken, IMAP4STK *stk);
void  search_rfc822(PImap4Context pContext, DWORD nToken, IMAP4STK *stk);
#ifdef UPDATE_20171230A // Quoted-Printableのメールの検索への対応
void QuotedPrintableDecodeLine(unsigned char * pSrc, unsigned char * pDest);
#endif
#ifdef UPDATE_20230626 //QuotedPrintableの比較の追加
void QuotedPrintableEncodeLine(const char *pSrc, char *pDest);
#endif

BOOL SEARCHDispatch(PCLIENT_CONTEXT lpClientContext) {
  char    *pDec[MAX_ATTRIBUTE];
  DWORD   nDec;
  PImap4Context pContext = &lpClientContext->Context;

  if (pContext->State == Imap4SelectFolder) {  // フォルダ選択済状態
	for (nDec = 0; nDec < MAX_ATTRIBUTE; nDec++) // 構造初期化
	  pDec[nDec] = NULL;
    nDec = MSGDecipher(lpClientContext, pContext->pToken, pDec);
	if (nDec >= MAX_ATTRIBUTE) { // 複雑すぎる構造
      sprintf(pContext->mMessages, "%s %s Excessively complex %s attribute list\r\n", pContext->pSquence, IMAP4_RESULT_RESPONSE, pContext->pCmd );
	} else {
#ifdef SEARCH_NO_RESS_TEST // 使用不可のオプションでNOを返す
	  if (SearchMSGByCondition(lpClientContext, pDec))
	  {
        sprintf(pContext->mMessages, "%s %s %s completed\r\n", pContext->pSquence, IMAP4_GOOD_RESPONSE, pContext->pCmd );
	  } else {
        sprintf(pContext->mMessages, "%s %s %s completed\r\n", pContext->pSquence, IMAP4_RESULT_RESPONSE, pContext->pCmd );
	  }
#else
	  SearchMSGByCondition(lpClientContext, pDec);
      sprintf(pContext->mMessages, "%s %s %s completed\r\n", pContext->pSquence, IMAP4_GOOD_RESPONSE, pContext->pCmd );
#endif
	}
  } else { // 状態が違う
    sprintf(pContext->mMessages, "%s %s Command unrecognized: %s\r\n", pContext->pSquence, IMAP4_RESULT_RESPONSE, pContext->pCmd );
  }
  return TRUE;
}

DWORD MSGDecipher(PCLIENT_CONTEXT lpClientContext, char *pRequest, char **pDec) {
  PImap4Context    pContext = &lpClientContext->Context;
  CHAR             *p, *p2;
  DWORD            nDec = 0, nTotal = 0;
#ifdef UPDATE_20230711 // 検索文字列をダブルクォーテーションで囲うと検索に失敗する
  char             *q;
#endif

#ifdef UPDATE_20230619 // SEARCH命令でOR指定の前に括弧が指定されていなと正しく検索されない不具合
	// 括弧を除去して正規化
	p = pRequest;
	while (*p)
	{
	  if (*p=='(' || *p==')')
	  {
#ifdef UPDATE_20230620 // SEARCH命令でOR指定の前に括弧が指定されていなと正しく検索されない不具合
		*p = ' ';
#else
		p2 = p;
		do
		{
		  *p2 = *(p2+1);
		  p2++;
		} while(*p2);
		*p2= '\x0';
#endif		
	  } else {
	    p++;
	  }
	}
#endif
#ifdef UPDATE_20230620 // SEARCH命令でOR指定の前に括弧が指定されていなと正しく検索されない不具合
    p = pRequest+strlen(pRequest)-1;
    while(*p==' ') // 末尾の空白除去
	{
	  *p = '\x0';
	  p--;
	  if (pRequest == p)
	    break;
	}
#endif

#ifdef UPDATE_20091201 // SEARCH命令に余分なスペースがある場合の対策
    while (*pRequest == ' ')
	{
      pRequest++;
	}
#endif
	pDec[nDec] = pRequest;
#ifdef UPDATE_20230711 // 検索文字列をダブルクォーテーションで囲うと検索に失敗する
	p = q = NULL;
#endif
	do {
#ifdef UPDATE_20230711 // 検索文字列をダブルクォーテーションで囲うと検索に失敗する
      if (q && (p = strchr(pDec[nDec], '"')))
	  {
	    *p='\x0';
	    p++;
		if (p = strpbrk(p, "\" "))
		{
		  //pDec[++nDec] = p;
		  while (*p == ' ')
		  {
		    p++;
		  }
	      pDec[++nDec] = p;
		  p = strpbrk(pDec[nDec], "\" ");
		}
	  } else 
        p = strpbrk(pDec[nDec], "\" ");
	  q = NULL;
#else
	  p = strstr(pDec[nDec], " ");
#endif
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
		  p++;
		}
		if (p)
		{
#ifdef UPDATE_20091201 // SEARCH命令に余分なスペースがある場合の対策
#ifdef UPDATE_20230711 // 検索文字列をダブルクォーテーションで囲うと検索に失敗する
          while (*p == ' ' || *p == '"')
#else
          while (*p == ' ')
#endif
		  {
#ifdef UPDATE_20230711 // 検索文字列をダブルクォーテーションで囲うと検索に失敗する
			if (*p == '"')
			  q = p;
#endif
            p++;
#ifdef UPDATE_20231130 //  検索文字列が空欄("")だと検索出来ない不具合。
			if (q == (p-1) && *p == '"')
              break;
#endif
		  }
#endif
		  pDec[++nDec] = p;
#ifdef UPDATE_20231130 //  検索文字列が空欄("")だと検索出来ない不具合。
		  if (q == (p-1) && *p == '"') {
			*p = '\x0';
			p++;
		  } 
#endif
		}
	  } 
	} while(p && nTotal < MAX_ATTRIBUTE-1);

	if (p) {
	  nTotal++; 
	}

	return nTotal;
}

#ifdef UPDATE_20150612 // SEARCH命令の構造解析見直し
BOOL SearchCalc(BOOL bOR, BOOL bItemA, BOOL bItemB)
{
   //printf("SearchCalc(%d %s %d)\n", bItemA, (bOR ? "OR" : "AND"), bItemB);
   if (bOR)
   {
	  return bItemA | bItemB;
   } else {
	  return bItemA & bItemB;
   }
}

#ifdef UPDATE_20230624A // SEARCH BODY命令で検索速度の向上対策:検索キーワード側を文字コード別に準備して比較する
void ConvertSearchKey(IMAP4STK *pstk)
{
#ifdef UPDATE_20230626 //QuotedPrintableの比較の追加
	char *p;
#endif

	pstk->mTokenUTF8[0]=pstk->mTokenSJIS[0]=pstk->mTokenJIS[0]=pstk->mTokenEUC[0]='\x0';
#ifdef UPDATE_20230626 //QuotedPrintableの比較の追加
	pstk->mTokenQUTF8[0]=pstk->mTokenQSJIS[0]=pstk->mTokenQJIS[0]=pstk->mTokenQEUC[0]='\x0';
#endif
	if (!stricmp(pstk->mCharSet, "utf8") ||
		!stricmp(pstk->mCharSet, "\"utf8\"") ||
		!stricmp(pstk->mCharSet, "utf-8") ||
		!stricmp(pstk->mCharSet, "\"utf-8\""))
	{
	  strcpy(pstk->mTokenUTF8, pstk->mToken);
	  // UTF-8 to S-JIS
	  UTF8N2CODEPAGE(CP_JAPANESE, pstk->mToken, pstk->mTokenSJIS, sizeof(pstk->mTokenSJIS));
	  // UTF-8 to JIS
	  UTF8N2CODEPAGE(CP_ISO_JAPANESE, pstk->mToken, pstk->mTokenJIS, sizeof(pstk->mTokenJIS));
	  // UTF-8 to EUC
	  UTF8N2CODEPAGE(CP_EUC_JAPANESE_MS, pstk->mToken, pstk->mTokenEUC, sizeof(pstk->mTokenEUC));
//printf("UTF-8 to EUC [%s]\n", pstk->mTokenEUC);
	} else if (!stricmp(pstk->mCharSet, "s-jis") ||
	 	       !stricmp(pstk->mCharSet, "\"s-jis\"") ||
			   !stricmp(pstk->mCharSet, "shift-jis") ||
		 	   !stricmp(pstk->mCharSet, "\"shift-jis\""))
	{
      CODEPAGE2UTF8N(CP_JAPANESE, pstk->mToken, pstk->mTokenUTF8, sizeof(pstk->mTokenUTF8));
	  // UTF-8 to S-JIS
	  strcpy(pstk->mTokenSJIS, pstk->mToken);
	  // UTF-8 to JIS
	  UTF8N2CODEPAGE(CP_ISO_JAPANESE, pstk->mTokenUTF8, pstk->mTokenJIS, sizeof(pstk->mTokenJIS));
	  // UTF-8 to EUC
	  UTF8N2CODEPAGE(CP_EUC_JAPANESE_MS, pstk->mTokenUTF8, pstk->mTokenEUC, sizeof(pstk->mTokenEUC));
//printf("UTF-8 to EUC [%s]\n", pstk->mTokenEUC);
	} else if (!stricmp(pstk->mCharSet, "eucjp") ||
		       !stricmp(pstk->mCharSet, "\"eucjp\"") ||
			   !stricmp(pstk->mCharSet, "euc-jp") ||
		       !stricmp(pstk->mCharSet, "\"euc-jp\"") ||
			   !stricmp(pstk->mCharSet, "euc") ||
		       !stricmp(pstk->mCharSet, "\"euc\""))
	{
      CODEPAGE2UTF8N(CP_EUC_JAPANESE_MS, pstk->mToken, pstk->mTokenUTF8, sizeof(pstk->mTokenUTF8));
	  // UTF-8 to S-JIS
	  UTF8N2CODEPAGE(CP_JAPANESE, pstk->mTokenUTF8, pstk->mTokenSJIS, sizeof(pstk->mTokenSJIS));
	  // UTF-8 to JIS
	  UTF8N2CODEPAGE(CP_ISO_JAPANESE, pstk->mTokenUTF8, pstk->mTokenJIS, sizeof(pstk->mTokenJIS));
	  // UTF-8 to EUC
	  strcpy(pstk->mTokenEUC, pstk->mToken);
	} else if (!stricmp(pstk->mCharSet, "iso2022jp") ||
		       !stricmp(pstk->mCharSet, "\"iso2022jp\"") ||
		       !stricmp(pstk->mCharSet, "iso-2022-jp") ||
		       !stricmp(pstk->mCharSet, "\"iso-2022-jp\"") ||
		       !stricmp(pstk->mCharSet, "jis") ||
		       !stricmp(pstk->mCharSet, "\"jis\""))
	{
      CODEPAGE2UTF8N(CP_ISO_JAPANESE, pstk->mToken, pstk->mTokenUTF8, sizeof(pstk->mTokenUTF8));
	  // UTF-8 to S-JIS
	  UTF8N2CODEPAGE(CP_JAPANESE, pstk->mTokenUTF8, pstk->mTokenJIS, sizeof(pstk->mTokenJIS));
	  // UTF-8 to JIS
	  strcpy(pstk->mTokenJIS, pstk->mToken);
	  // UTF-8 to EUC
	  UTF8N2CODEPAGE(CP_EUC_JAPANESE_MS, pstk->mTokenJIS, pstk->mTokenEUC, sizeof(pstk->mTokenEUC));
	} else { // UTF-8
	  strcpy(pstk->mTokenUTF8, pstk->mToken);
	  // UTF-8 to S-JIS
	  UTF8N2CODEPAGE(CP_JAPANESE, pstk->mToken, pstk->mTokenSJIS, sizeof(pstk->mTokenSJIS));
	  // UTF-8 to JIS
	  UTF8N2CODEPAGE(CP_ISO_JAPANESE, pstk->mToken, pstk->mTokenJIS, sizeof(pstk->mTokenJIS));
	  // UTF-8 to EUC
	  UTF8N2CODEPAGE(CP_EUC_JAPANESE_MS, pstk->mTokenJIS, pstk->mTokenEUC, sizeof(pstk->mTokenEUC));
	}

#ifdef UPDATE_20230626 //QuotedPrintableの比較の追加
	if (strlen(pstk->mTokenJIS)>6)
	{
	  p = pstk->mTokenJIS+3;
	  //pstk->mTokenJIS[strlen(pstk->mTokenJIS)-3] = '\x0';
	  while(*p)
	  {
	    *(p-3) = *p;
	    p++;
	  }
	  pstk->mTokenJIS[strlen(pstk->mTokenJIS)-6] = '\x0';
	}
    QuotedPrintableEncodeLine(pstk->mTokenUTF8, pstk->mTokenQUTF8);
    QuotedPrintableEncodeLine(pstk->mTokenSJIS, pstk->mTokenQSJIS);
    QuotedPrintableEncodeLine(pstk->mTokenJIS, pstk->mTokenQJIS);
    QuotedPrintableEncodeLine(pstk->mTokenEUC, pstk->mTokenQEUC);
#endif
/*
    if (bDebug) 
	{
	  printf("pstk->mCharSet=[%s]\n", pstk->mCharSet);
	  printf("pstk->mToken=[%s]\n", pstk->mToken);
	  printf("pstk->mTokenUTF8=[%s]\n", pstk->mTokenUTF8);
      printf("pstk->mTokenSJIS=[%s]\n", pstk->mTokenSJIS);
      printf("pstk->mTokenJIS=[%s]\n", pstk->mTokenJIS);
      printf("pstk->mTokenEUC=[%s]\n", pstk->mTokenEUC);
#ifdef UPDATE_20230626 //QuotedPrintableの比較の追加
	  printf("pstk->mTokenQUTF8=[%s]\n", pstk->mTokenQUTF8);
      printf("pstk->mTokenQSJIS=[%s]\n", pstk->mTokenQSJIS);
      printf("pstk->mTokenQJIS=[%s]\n", pstk->mTokenQJIS);
      printf("pstk->mTokenQEUC=[%s]\n", pstk->mTokenQEUC);
#endif
	}
*/
}
#endif

BOOL SearchCMD(PImap4Context pContext, DWORD nMSG, char *pCharSet, char **pDec, int *pnDec)
{
   DWORD nToken = 0;
   BOOL  bNOT = FALSE;
   BOOL  bUID = FALSE;
#ifdef UPDATE_20150624 // SEARCH命令で検索対象NOを複数羅列するとハングする不具合
   IMAP4STK  stk[MAX_ATTRIBUTE];  // 検索文字列構造体
#else
   IMAP4STK  stk[3];  // 検索文字列構造体
#endif
   CHAR *q;
   CHAR *p = pDec[*pnDec];
   CHAR *p2, *p3;
   int n;

   //printf("SearchCMD(%d, %s %s)\n", *pnDec, p, pDec[*pnDec+1]);
   //// 初期設定
   stk[nToken].nGrp = FALSE;           // 階層
   stk[nToken].bResult = FALSE;
   stk[nToken].bUnrelated = FALSE; // 関係有り
   stk[nToken].bNOT = bNOT;
   stk[nToken].bOR = FALSE;
   stk[nToken].nMode = 1;
   stk[nToken].nUid  = (bUID ? 1 : 0);  // UID番号 : 連番
   ///////////////////
   // SEARCH CHRSETのUTF-8対応 CatchMe@Mailで添付きメールの表示が失敗する
   strcpy(stk[0].mCharSet, pCharSet); //stk[0].mCharSet[0] = '\x0';
   // "UID SEARCH"での番号指定での誤った応答結果を返してしまう不具合。
   stk[nToken].nFrom = stk[nToken].nTo = 0;
   stk[nToken].mToken[0] = '\x0';
search_retry:
   {
     bNOT = FALSE;
	 if (!strnicmp(p, "uid ", 4) || !stricmp(p, "uid")) 
	 {
#ifdef UPDATE_20140620 // "UID SEARCH"での番号指定での誤った応答結果を返してしまう不具合。
         if (nToken == 0)
		 {
	       ////////////////////////////
	       /// 初期設定（共通）
	       stk[nToken].nGrp = FALSE;           // 階層
           stk[nToken].bResult = FALSE;
	       stk[nToken].bUnrelated = FALSE; // 関係有り
	       stk[nToken].bNOT = bNOT;
	       stk[nToken].bOR = FALSE;
           stk[nToken].nMode = 1;
	       stk[nToken].nUid  = (bUID ? 1 : 0);  // UID番号 : 連番
#ifndef UPDATE_20150626A // SEARCH命令で検索対象NOをUID値で指定を行うと正しい結果が得られない不具合
		   nToken++;
#endif
		 }
#endif
		 bUID = TRUE;
	     p = pDec[++(*pnDec)];
	   } else {
	     bUID = FALSE;
	   }
   	   if (p) // SEARCH命令に続くUIDトークンの引数が不足しているとハングする不具合
	   {
	     if (!strnicmp(p, "NOT ", 4) || !stricmp(p, "not"))
		 {
	       bNOT = TRUE;
	       p +=4;
		 } else {
	       bNOT = FALSE;
		 }
	   }
	 }
	 ////////////////////////////
	 /// 初期設定（共通）
     stk[nToken].nGrp = FALSE;           // 階層
     stk[nToken].bResult = FALSE;
	 stk[nToken].bUnrelated = FALSE; // 関係有り
	 stk[nToken].bNOT = bNOT;
	 stk[nToken].bOR = FALSE;
     stk[nToken].nMode = 1;
	 stk[nToken].nUid  = (bUID ? 1 : 0);  // UID番号 : 連番
   /*
	 if (!p) // 複雑なSEARCH命令の解読でハングすることがある。
	 {
	   (*pnDec)++;
       nToken++;
	   continue;
	 }
	 */
#ifdef UPDATE_20151220 // SEARCH命令でハングする可能性
	 if (!p)
	 {
	   return stk[nToken].bResult;
	 }
#endif
	 /////////////////////////////////////////////
	 /// 連番 又は UID番号
	 //if (nDec == 0 && (p2 = strpbrk(p, ",:"))) {
	 if ((p2 = strpbrk(p, ",:"))) 
	 {
       //bFromTo = TRUE;
	   while(1) {
	     ////////////////////////////
	     /// 初期設定（共通）
         //stk[nToken].nGrp = nGrp;           // 階層
         stk[nToken].bResult = FALSE;
	     stk[nToken].bUnrelated = FALSE; // 関係有り
	     stk[nToken].bNOT = bNOT;
	     stk[nToken].bOR = FALSE;
         stk[nToken].nMode = 1;
		 stk[nToken].nUid  = (bUID ? 1 : 0);  // UID番号 : 連番
         stk[nToken].mToken[0] = '\x0';
         if (p2) {
		   if (*p2 == ':') {
             *p2 = '\x0';
	         p2++;
#ifdef UPDATE_20151218 // 範囲指定の*のとき処理の修正
	         if (*p == '*')
		       stk[nToken].nFrom  = (stk[nToken].nUid ? pContext->nUid-1 : pContext->nExists);
	         else
#endif
               stk[nToken].nFrom = atol(p);
		     if (*p2 == '*')
#ifdef UPDATE_20151218 // 範囲指定の*のとき処理の修正
		       stk[nToken].nTo  = (stk[nToken].nUid ? pContext->nUid-1 : pContext->nExists);
#else
		       stk[nToken].nTo = 0;     // '*'なら0に置換え
#endif
		     else
	           stk[nToken].nTo = atol(p2);
		   } else {
#ifdef UPDATE_20151218 // 範囲指定の*のとき処理の修正
	         if (*p == '*')
	 	       stk[nToken].nFrom = stk[nToken].nTo = (stk[nToken].nUid ?  pContext->nUid-1 : pContext->nExists);
	         else
#endif
	 	       stk[nToken].nFrom = stk[nToken].nTo = atol(p);
		   }
#ifdef UPDATE_20150625 // SEARCH命令で検索対象NOを複数羅列すると正しい結果が得られない不具合
		   if (strpbrk(p2, ","))
		   {
			 p2 = strpbrk(p2, ",");
		   }
#endif
		 } else {
#ifdef UPDATE_20151218 // 範囲指定の*のとき処理の修正
	       if (*p == '*')
	 	     stk[nToken].nFrom = stk[nToken].nTo = (stk[nToken].nUid ?  pContext->nUid-1 : pContext->nExists);
	       else
#endif
	 	     stk[nToken].nFrom = stk[nToken].nTo = atol(p);
		   break;
		 }
		 if (*p2 == ',') { // 続きの番号アリ
		   p = ++p2;
		   p2 = strpbrk(p, ",:");
		   nToken++;
		 } else {
		   break;
		 }
	   }
     /// "UID SEARCH"での番号指定での誤った応答結果を返してしまう不具合。
#ifdef UPDATE_20151218 // 範囲指定の*のとき処理の修正
	 } else if (*p == '*') {
       stk[nToken].nFrom = stk[nToken].nTo = (stk[nToken].nUid ? pContext->nUid-1 : pContext->nExists);
#endif
	 } else if (stricmp(p, "UID") == 0 ||
		        stricmp(p, "ALL") == 0) {   // デフォルトの為無視
#ifdef UPDATE_20151218 // 範囲指定の*のとき処理の修正
       stk[nToken].nFrom = 0;
	   stk[nToken].nTo = (stk[nToken].nUid ? pContext->nUid-1 : pContext->nExists);;
#else
       stk[nToken].nFrom = stk[nToken].nTo = 0;
#endif
       //nToken++;
	   //++(*pnDec);
	   //continue;
	 } else if (stricmp(p, "RECENT") == 0) {
       stk[nToken].nMode = 0;
	   stk[nToken].bNOT = (bNOT ? TRUE : FALSE);
	   stk[nToken].nFlag = (0x1 << RECENT);
	 } else if (stricmp(p, "NEW") == 0) { // =(RECENT UNSEEN)
       stk[nToken].nMode = 0;
	   stk[nToken].bNOT = (bNOT ? TRUE : FALSE);
	   stk[nToken].nFlag = (0x1 << RECENT);
	   nToken++;
       stk[nToken].nMode = 0;
	   stk[nToken].bNOT = (bNOT ? FALSE : TRUE);
	   stk[nToken].nFlag = (0x1 << SEEN);
	 } else if (stricmp(p, "OLD") == 0) {
       stk[nToken].nMode = 0;
	   stk[nToken].bNOT = (bNOT ? FALSE : TRUE);
	   stk[nToken].nFlag = (0x1 << RECENT);
	 } else if (stricmp(p, "FLAGGED") == 0) {
       stk[nToken].nMode = 0;
	   stk[nToken].bNOT = (bNOT ? TRUE : FALSE);
	   stk[nToken].nFlag = (0x1 << FLAGGED);
	 } else if (stricmp(p, "UNFLAGGED") == 0) {
       stk[nToken].nMode = 0;
	   stk[nToken].bNOT = (bNOT ? FALSE : TRUE);
	   stk[nToken].nFlag = (0x1 << FLAGGED);
	 } else if (stricmp(p, "ANSWERED") == 0) {
       stk[nToken].nMode = 0;
	   stk[nToken].bNOT = (bNOT ? TRUE : FALSE);
	   stk[nToken].nFlag = (0x1 << ANSWERED);
	 } else if (stricmp(p, "UNANSWERED") == 0) {
       stk[nToken].nMode = 0;
	   stk[nToken].bNOT = (bNOT ? FALSE : TRUE);
	   stk[nToken].nFlag = (0x1 << ANSWERED);
	 } else if (stricmp(p, "DELETED") == 0) {
       stk[nToken].nMode = 0;
	   stk[nToken].bNOT = (bNOT ? TRUE : FALSE);
	   stk[nToken].nFlag = (0x1 << DELETED);
	 } else if (stricmp(p, "UNDELETED") == 0) {
       stk[nToken].nMode = 0;
	   stk[nToken].bNOT = (bNOT ? FALSE : TRUE);
	   stk[nToken].nFlag = (0x1 << DELETED);
	 } else if (stricmp(p, "DRAFT") == 0) {
       stk[nToken].nMode = 0;
	   stk[nToken].bNOT = (bNOT ? TRUE : FALSE);
	   stk[nToken].nFlag = (0x1 << DRAFT);
	 } else if (stricmp(p, "UNDRAFT") == 0) {
       stk[nToken].nMode = 0;
	   stk[nToken].bNOT = (bNOT ? FALSE : TRUE);
	   stk[nToken].nFlag = (0x1 << DRAFT);
	 } else if (stricmp(p, "SEEN") == 0) {
       stk[nToken].nMode = 0;
	   stk[nToken].bNOT = (bNOT ? TRUE : FALSE);
	   stk[nToken].nFlag = (0x1 << SEEN);
	 } else if (stricmp(p, "UNSEEN") == 0) {
       stk[nToken].nMode = 0;
	   stk[nToken].bNOT = (bNOT ? FALSE : TRUE);
	   stk[nToken].nFlag = (0x1 << SEEN);
	 /////////////////////////////////////////////
	 /// 文字列検索
	 } else if (stricmp(p, "TEXT") == 0) {
       stk[nToken].nMode = 4;
	   stk[nToken].nArea = 0;                    // TEXTキー
       strcpy(stk[nToken].mKeyword, "");         // TEXTキー
       // 検索条件が長いと途中で切ってしまう不具合
	   if (pDec[*pnDec+1])
	     (*pnDec)++;
	   if (pDec[*pnDec][0] == '"') {
         strcpy(stk[nToken].mToken, &pDec[*pnDec][1]);
	     strtok(stk[nToken].mToken, "\"");
	   } else if (pDec[*pnDec][0] == '{') {
         p3 = strpbrk(pDec[*pnDec], "}");
		 n = atol(&pDec[*pnDec][1]);
		 strncpy(stk[nToken].mToken, ++p3, n);
		 stk[nToken].mToken[n] = '\x0';
	   } else {
         strcpy(stk[nToken].mToken, pDec[*pnDec]);
	   }
#ifdef UPDATE_20230624A // SEARCH BODY命令で検索速度の向上対策:検索キーワード側を文字コード別に準備して比較する
       ConvertSearchKey((IMAP4STK *)&stk[nToken]);
#endif
	 } else if (stricmp(p, "BODY") == 0) {
       // SEARCH CHRSETのUTF-8対応 CatchMe@Mailで添付きメールの表示が失敗する
	   if (nToken > 0)
	   {
         strcpy(stk[nToken].mCharSet, stk[nToken-1].mCharSet);
       }
       stk[nToken].nMode = 4;
	   stk[nToken].nArea = 1;               // BODYキー
       strcpy(stk[nToken].mKeyword, "");         // BODYキー
       // 検索条件が長いと途中で切ってしまう不具合
	   if (pDec[*pnDec+1])
	     (*pnDec)++;
	   if (pDec[*pnDec][0] == '"') {
         strcpy(stk[nToken].mToken, &pDec[*pnDec][1]);
	     strtok(stk[nToken].mToken, "\"");
	   } else if (pDec[*pnDec][0] == '{') {
         p3 = strpbrk(pDec[*pnDec], "}");
		 n = atol(&pDec[*pnDec][1]);
		 strncpy(stk[nToken].mToken, ++p3, n);
		 stk[nToken].mToken[n-1] = '\x0';
	   } else {
         strcpy(stk[nToken].mToken, pDec[*pnDec]);
	   }
#ifdef UPDATE_20230624A // SEARCH BODY命令で検索速度の向上対策:検索キーワード側を文字コード別に準備して比較する
       ConvertSearchKey((IMAP4STK *)&stk[nToken]);
#endif
/*
	 } else if (stricmp(p, "CHARSET") == 0) { // 検索キーのキャラクタセット
	   ++(*pnDec);
#ifdef SEARCH_NO_RESS_TEST // 使用不可のオプションでNOを返す
	   if (strcmp(pDec[*pnDec], "iso-2022-jp"))
	   {
         //nResult = FALSE;
		 return FALSE; //nResult;
	   }
#endif

       // SEARCH CHRSETのUTF-8対応 CatchMe@Mailで添付きメールの表示が失敗する
       strcpy(stk[nToken].mCharSet, pDec[*pnDec]);
       //nToken++;
	   ++(*pnDec);
	   p = pDec[*pnDec];
	   //goto search_retry; 
	   return TRUE;
	   //continue;  // 無視
	   stk[nToken].bUnrelated = TRUE;
       stk[nToken].nMode = 4;
	   stk[nToken].nArea = 2;                 // HEADERキー
	   strcpy(stk[nToken].mKeyword, "Content-Type:"); // HEADERキー
#ifdef UPDATE_20150606B // 検索条件が長いと途中で切ってしまう不具合
	   if (pDec[*pnDec+1])
	     (*pnDec)++;
#else
	   if (pDec[nDec+1])
  	     pDec[++(*pnDec)][sizeof(stk[0].mToken)-1] = '\x0'; // バッファフローしないように
#endif
	   if (pDec[*pnDec][0] == '"') {
         strcpy(stk[nToken].mToken, &pDec[*pnDec][1]);
	     strtok(stk[nToken].mToken, "\"");
	   } else if (pDec[*pnDec][0] == '{') {
         p3 = strpbrk(pDec[*pnDec], "}");
		 n = atol(&pDec[*pnDec][1]);
		 strncpy(stk[nToken].mToken, ++p3, n);
		 stk[nToken].mToken[n-1] = '\x0';
	   } else {
         strcpy(stk[nToken].mToken, pDec[*pnDec]);
	   }
*/
	 } else if (stricmp(p, "HEADER") == 0) {
       stk[nToken].nMode = 4;
	   stk[nToken].nArea = 2;                 // HEADERキー
       // 検索条件が長いと途中で切ってしまう不具合
	   if (pDec[*pnDec+1])
	     (*pnDec)++;
#ifdef UPDATE_20150615 // 検索条件でHEADER <KEY>をダブルクォーテーション囲まれていると失敗する
	   // 先頭のスペース、ダブルクォーテーション、閉じ括弧除去
	   q = pDec[*pnDec];
       while(*q == '"') 
	   {
	     q++;
	   }
       strtok(q, "\"");
       strcpy(stk[nToken].mKeyword, q); // HEADERキー
#else
       strcpy(stk[nToken].mKeyword, pDec[*pnDec]); // HEADERキー
#endif
	   if (stk[nToken].mKeyword[strlen(stk[nToken].mKeyword)-1] != ':')
		 strcat(stk[nToken].mKeyword, ":");
	   if (pDec[*pnDec+1])
	     (*pnDec)++;
	   if (pDec[*pnDec][0] == '"') {
         strcpy(stk[nToken].mToken, &pDec[*pnDec][1]);
	     strtok(stk[nToken].mToken, "\"");
	   } else if (pDec[*pnDec][0] == '{') {
         p3 = strpbrk(pDec[*pnDec], "}");
		 n = atol(&pDec[*pnDec][1]);
		 strncpy(stk[nToken].mToken, ++p3, n);
		 stk[nToken].mToken[n-1] = '\x0';
	   } else {
         strcpy(stk[nToken].mToken, pDec[*pnDec]);
	   }
#ifdef UPDATE_20230624A // SEARCH BODY命令で検索速度の向上対策:検索キーワード側を文字コード別に準備して比較する
       ConvertSearchKey((IMAP4STK *)&stk[nToken]);
#endif
	 } else if (stricmp(p, "BCC") == 0 ||
		        stricmp(p, "CC") == 0 ||
		        stricmp(p, "FROM") == 0 ||
		        stricmp(p, "SUBJECT") == 0 ||
		        stricmp(p, "TO") == 0 ) {
       // SEARCH CHRSETのUTF-8対応 CatchMe@Mailで添付きメールの表示が失敗する
	   if (nToken > 1)
	   {
         strcpy(stk[nToken].mCharSet, stk[nToken-1].mCharSet);
       }
       stk[nToken].nMode = 4;
	   stk[nToken].nArea = 2;                      // HEADERキー
       sprintf(stk[nToken].mKeyword, "%s:", p);    // HEADERキー
       // 検索条件が長いと途中で切ってしまう不具合
	   if (pDec[*pnDec+1])
	     (*pnDec)++;
	   if (pDec[*pnDec][0] == '"') {
         strcpy(stk[nToken].mToken, &pDec[*pnDec][1]);
	     strtok(stk[nToken].mToken, "\"");
	   } else if (pDec[*pnDec][0] == '{') {
         p3 = strpbrk(pDec[*pnDec], "}");
		 n = atol(&pDec[*pnDec][1]);
		 strncpy(stk[nToken].mToken, ++p3, n);
		 stk[nToken].mToken[n-1] = '\x0';
	   } else {
         strcpy(stk[nToken].mToken, pDec[*pnDec]);
	   }
#ifdef UPDATE_20230624A // SEARCH BODY命令で検索速度の向上対策:検索キーワード側を文字コード別に準備して比較する
       ConvertSearchKey((IMAP4STK *)&stk[nToken]);
#endif
	 } else if (stricmp(p, "SENTBEFORE") == 0 ||
		        stricmp(p, "SENTON") == 0 ||
		        stricmp(p, "SENTSINCE") == 0) {
       stk[nToken].nMode = 4;
	   if (stricmp(p, "SENTBEFORE") == 0)
		 stk[nToken].nDType = 0; // BEFORE
	   else if (stricmp(p, "SENTON") == 0)
		 stk[nToken].nDType = 1; // ON
	   else if (stricmp(p, "SENTSINCE") == 0)
		 stk[nToken].nDType = 2; // SINCE
	   stk[nToken].nArea = 2;                        // HEADERキー
       sprintf(stk[nToken].mKeyword, "DATE:", p);    // HEADERキー
       // 検索条件が長いと途中で切ってしまう不具合
	   if (pDec[*pnDec+1])
	     (*pnDec)++;
       strcpy(stk[nToken].mToken, pDec[*pnDec]);
	 /////////////////////////////////////////////
	 /// ファイルサイズ
	 } else if (stricmp(p, "LARGER") == 0) {
       stk[nToken].nMode = 3;
       stk[nToken].nFileSizeType  = 0;
	   if (pDec[*pnDec+1])
         stk[nToken].nFileSize = atol(pDec[++(*pnDec)]);
	 } else if (stricmp(p, "SMALLER") == 0) {
       stk[nToken].nMode = 3;
       stk[nToken].nFileSizeType  = 1;
	   if (pDec[*pnDec+1])
         stk[nToken].nFileSize = atol(pDec[++(*pnDec)]);
	 /////////////////////////////////////////////
	 /// 内部日付
	 } else if (stricmp(p, "BEFORE") == 0) {
       stk[nToken].nMode = 2;
       stk[nToken].nDType  = 0;
	   if (pDec[*pnDec+1])
         strcpy(stk[nToken].mToken, pDec[++(*pnDec)]);
	 } else if (stricmp(p, "ON") == 0) {
       stk[nToken].nMode = 2;
       stk[nToken].nDType  = 1;
	   if (pDec[*pnDec+1])
         strcpy(stk[nToken].mToken, pDec[++(*pnDec)]);
	 } else if (stricmp(p, "SINCE") == 0) {
       stk[nToken].nMode = 2;
       stk[nToken].nDType  = 2;
	   if (pDec[*pnDec+1])
         strcpy(stk[nToken].mToken, pDec[++(*pnDec)]);
	 /////////////////////////////////////////////
     // SEARCHの番号指定で結果フラグが初期化されていなかった。
	 } else {
       stk[nToken].nFrom = stk[nToken].nTo = atol(p);
	 }

	 ////////////////////////
	 // 先頭のスペース、ダブルクォーテーション、閉じ括弧除去
	 q = stk[nToken].mToken;
     while(*q == ' ' || *q == '"' ) 
	 {
	   q++;
	 }
     strtok(q, ")\"");
     ////////////////////////
	 {
       search_falgs(pContext, nToken+1, stk);
       search_number(pContext, nToken+1, nMSG, stk);
       search_internaldate(pContext, nToken+1, stk);
       search_size(pContext, nToken+1, stk);
       search_rfc822(pContext, nToken+1, stk);
	 }
#ifdef UPDATE_20230617 // SEARCH命令でOR指定が含まれると正しく検索結果が出力されない不具合
	 pContext->nMode = stk[nToken].nMode;          // 0:FLAG/1:番号/2:内部日付/3:ファイルサイズ/4:メッセージ/5:その他
#endif
	 return stk[nToken].bResult;
}

#ifdef UPDATE_20230617 // SEARCH命令でOR指定が含まれると正しく検索結果が出力されない不具合
BOOL SearchFrame(PImap4Context pContext, BOOL bORU, DWORD nMSG, char *pCharSet, char **pDec, int *pnDec)
#else
BOOL SearchFrame(PImap4Context pContext, DWORD nMSG, char *pCharSet, char **pDec, int *pnDec)
#endif
{
  BOOL  bResult[2] = {FALSE, FALSE};
  BOOL  bFrame = FALSE;
#ifdef UPDATE_20230617 // SEARCH命令でOR指定が含まれると正しく検索結果が出力されない不具合
  BOOL  bOR = bORU;
#else
  BOOL  bOR = FALSE;
#endif
  DWORD nToken = 0;                    // 検索文字列数
  int   nDec = 0;
  int   i, nLoop = 0;
  char  *p;
#ifdef UPDATE_20230619 // SEARCH命令でOR指定の前に括弧が指定されていなと正しく検索されない不具合
  char *p2;
  BOOL bOR2 = FALSE;
#endif

  /////////////////////////////
  // SEARCH CHRSETのUTF-8対応 CatchMe@Mailで添付きメールの表示が失敗する
  if (stricmp(pDec[*pnDec], "CHARSET") == 0) // 検索キーのキャラクタセット
  {
    ++(*pnDec);
    strcpy(pCharSet, pDec[*pnDec]);
    ++(*pnDec);
  }
#ifdef UPDATE_20180107 // SEARCH命令がCHARSETのみしかないとハングする不具合
  if (!pDec[*pnDec])
  {
     return bResult[0];
  }
#endif
  /////////////////////////////
#ifdef UPDATE_20230619 // SEARCH命令でOR指定の前に括弧が指定されていなと正しく検索されない不具合
  p2 = pDec[*pnDec];
  while(*p2 == ' ')
  {
	p2++;
  }
  if (*p2 == '(')
#else
  if (strchr(pDec[*pnDec], '('))
#endif
  {
	bFrame = TRUE;
	nDec = *pnDec;
	pDec[*pnDec]++; 
#ifdef UPDATE_20230617 // SEARCH命令でOR指定が含まれると正しく検索結果が出力されない不具合
	bResult[0] = SearchFrame(pContext, bOR, nMSG, pCharSet, pDec, pnDec);
#else
	bResult[0] = SearchFrame(pContext, nMSG, pCharSet, pDec, pnDec);
#endif
	pDec[nDec]--;
	return bResult[0];
	//p = pDec[*pnDec];
  } else
  {
    if ((p=!stricmp(pDec[*pnDec], "OR")))
	{
#ifdef UPDATE_20230619 // SEARCH命令でOR指定の前に括弧が指定されていなと正しく検索されない不具合
	  bOR = bORU+1;
#else
	  bOR = TRUE;
#endif
	  ++(*pnDec);
#ifdef UPDATE_20230619 // SEARCH命令でOR指定の前に括弧が指定されていなと正しく検索されない不具合
	  p = pDec[*pnDec];
	  while (*p == ' ')
	  {
		p++;
	  }
	  if (*p == '(' || !strnicmp(p, "OR",2))
#else
      if ((p=strchr(pDec[*pnDec], '(')))
#endif
	  {
		nLoop = 0;
		while(*(pDec[*pnDec]+nLoop) == '(') // 連続括弧 (((... の処理
		{
		  nLoop++;
		}
#ifdef UPDATE_20230617 // SEARCH命令でOR指定が含まれると正しく検索結果が出力されない不具合
	    bResult[0] = SearchFrame(pContext, bOR, nMSG, pCharSet, pDec, pnDec);
#else
	    bResult[0] = SearchFrame(pContext, nMSG, pCharSet, pDec, pnDec);
#endif
		for (i = 1; i < nLoop; i++)
		{
#ifdef UPDATE_20230617 // SEARCH命令でOR指定が含まれると正しく検索結果が出力されない不具合
		  bResult[0] &= SearchFrame(pContext, bOR, nMSG, pCharSet, pDec, pnDec);
#else
		  bResult[0] &= SearchFrame(pContext, nMSG, pCharSet, pDec, pnDec);
#endif
		}
		p = pDec[*pnDec];
#ifdef UPDATE_20230619 // SEARCH命令でOR指定の前に括弧が指定されていなと正しく検索されない不具合
		bOR2 = TRUE;
#endif
		goto s_second;
	  }
	}
	p = pDec[*pnDec];
	if (!p)
	{
	  return  bResult[0];
	}
	// 区切り除去
    while(*p == ' ' || *p == '"' ) 
	{
	  p++;
	}
    strtok(p, "\"");
	////////////////////////////////////
	bResult[0] = SearchCMD(pContext, nMSG, pCharSet, pDec, pnDec);
	p = pDec[++(*pnDec)];
  }
s_second:
#ifdef UPDATE_20230619 // SEARCH命令でOR指定の前に括弧が指定されていなと正しく検索されない不具合
  if (p && bOR)
#else
  if (p)
#endif
  {
    /////////////////////////////
    // SEARCH CHRSETのUTF-8対応 CatchMe@Mailで添付きメールの表示が失敗する
    if (stricmp(pDec[*pnDec], "CHARSET") == 0) // 検索キーのキャラクタセット
	{
      ++(*pnDec);
      strcpy(pCharSet, pDec[*pnDec]);
      ++(*pnDec);
	}
    /////////////////////////////
#ifdef UPDATE_20230619 // SEARCH命令でOR指定の前に括弧が指定されていなと正しく検索されない不具合
    p2 = pDec[*pnDec];
    while(*p2 == ' ')
	{
	  p2++;
	}
    if (*p2 == '(')
#else
    if (strchr(pDec[*pnDec], '(')) // || !stricmp(pDec[*pnDec], "OR"))
#endif
	{
	  bFrame = TRUE;
	  nDec = *pnDec;
	  nLoop = 0;
	  while(*(pDec[*pnDec]+nLoop) == '(') // 連続括弧 (((... の処理
	  {
	    nLoop++;
	  }
#ifdef UPDATE_20230619 // SEARCH命令でOR指定の前に括弧が指定されていなと正しく検索されない不具合
      p2 = pDec[*pnDec];
      while(*p2 == ' ')
	  {
	    p2++;
	  }
      if (*p2 == '(')
#else
	  if (strchr(pDec[*pnDec], '('))
#endif
	  {
	    pDec[*pnDec]++; 
	  }
#ifdef UPDATE_20230617 // SEARCH命令でOR指定が含まれると正しく検索結果が出力されない不具合
	  bResult[1] = SearchFrame(pContext, bOR, nMSG, pCharSet, pDec, pnDec);
#else
	  bResult[1] = SearchFrame(pContext, nMSG, pCharSet, pDec, pnDec);
#endif
	  for (i = 1; i < nLoop; i++)
	  {
#ifdef UPDATE_20230617 // SEARCH命令でOR指定が含まれると正しく検索結果が出力されない不具合
	    bResult[1] &= SearchFrame(pContext, bOR, nMSG, pCharSet, pDec, pnDec);
#else
	    bResult[1] &= SearchFrame(pContext, nMSG, pCharSet, pDec, pnDec);
#endif
	  }
	  if (pDec[*pnDec])
	  {
#ifdef UPDATE_20230619 // SEARCH命令でOR指定の前に括弧が指定されていなと正しく検索されない不具合
        p2 = pDec[*pnDec];
        while(*p2 == ' ')
		{
	      p2++;
		}
        if (*p2 == '(')
#else
	    if (strchr(pDec[*pnDec], '('))
#endif
		{
	      pDec[nDec]--;
		}
	  }
	  //return bResult[1];
	  //p = pDec[*pnDec];
	} else
	{
      // 区切り除去
      while(*p == ' ' || *p == '"' ) 
	  {
	    p++;
	  }
      strtok(p, "\"");
      if (!stricmp(p, "OR"))
	  {
	    //bOR = TRUE;
#ifdef UPDATE_20230617 // SEARCH命令でOR指定が含まれると正しく検索結果が出力されない不具合
		bResult[1] = SearchFrame(pContext, bOR, nMSG, pCharSet, pDec, pnDec);
#else
		bResult[1] = SearchFrame(pContext, nMSG, pCharSet, pDec, pnDec);
#endif
        //(*pnDec)++;
#ifdef UPDATE_20230619 // SEARCH命令でOR指定の前に括弧が指定されていなと正しく検索されない不具合
        if (bOR > 1 && !bOR2)
		  bOR--;
#endif
		goto s_last;
	  }
      /////////////////////////////
      p = pDec[*pnDec];
	  if (p)
	  {
        // 区切り除去
        while(*p == ' ' || *p == '"' ) 
		{
	      p++;
		}
        strtok(p, "\"");
        bResult[1] = SearchCMD(pContext, nMSG, pCharSet, pDec, pnDec);
        (*pnDec)++;
#ifdef UPDATE_20230619 // SEARCH命令でOR指定の前に括弧が指定されていなと正しく検索されない不具合
        if (bOR > 1 && !bOR2)
		  bOR--;
#endif
	  }
	}
  } else {
#ifdef UPDATE_20230617 // SEARCH命令でOR指定が含まれると正しく検索結果が出力されない不具合
    if (!bOR || pContext->nMode == 0)
	  bResult[1] = TRUE;
#else
    bResult[1] = TRUE;
#endif
  }
s_last:
  return SearchCalc(bOR, bResult[0], bResult[1]);
}

#ifdef SEARCH_NO_RESS_TEST // 使用不可のオプションでNOを返す
int SearchMSGByCondition(PCLIENT_CONTEXT lpClientContext, char **pDec)
#else
void SearchMSGByCondition(PCLIENT_CONTEXT lpClientContext, char **pDec)
#endif
{
  PImap4Context    pContext = &lpClientContext->Context;
#ifdef UPDATE_20190510 // NTFS以外のファイルシステムからのファイル一覧取得可能にする対策（一覧ソート）を追加
  DWORD            nSearch, no;
  WIN32_FIND_DATA  *pfd;
#endif
  HANDLE           hSearch;
  CHAR             MailDir[MAX_PATH], mWork[MAX_PATH];
  CHAR             mUID[32];
  DWORD nDec = 0;
  DWORD nMSG = 0;
  BOOL  bResult = FALSE;
  BOOL  bSend = FALSE;
#ifdef SEARCH_NO_RESS_TEST // 使用不可のオプションでNOを返す
  int   nResult = TRUE;
#endif
  int   i, j, k, l;
  char  mCharSet[40] = "";
  char  *pDecBackup[MAX_ATTRIBUTE];
  char  mDec[MAX_ATTRIBUTE][80];
#ifdef UPDATE_20230619 // SEARCH命令でOR指定の前に括弧が指定されていなと正しく検索されない不具合
  char  *p2;
#endif
#ifdef UPDATE_20230627A // UPDATE_20230624のメールデータを事前に一括して読込み検索するフラグを追加
  FILE *fp;
  char *pfBufp;
  unsigned long    nfsize;
  char mFn[256];
#endif
#ifdef UPDATE_20240830B // FETCH命令で処理トーク解析処理の際のCPU負荷を下げる対策
  DWORD  nCloolStep = 0;
#endif

    // 検索条件バックアップと括弧の正規化
//printf("Backup\n");
    j = 0;
    while(pDec[j])
    {
	  pDecBackup[j] = pDec[j];
//printf("[%d] [%s]\n", j, pDec[j]);
	  k = 1;
	  //if (!strcmp(pDec[j], "("))
	  if (pDec[j][strlen(pDec[j])-1] == '(')
	  {
	    while (pDec[j+k])
		{
		  strcat(pDec[j] , pDec[j+k]);
		  k++;
		  if (pDec[j][strlen(pDec[j])-1] != '(') // && *pDec[j+k] != '(')
		  {
			break;
		  }
		  /*
		  if (*pDec[j] == '(' && *pDec[j+k] == '(')
		  {
		    strcat(pDec[j] , pDec[j+1]);
		    k++;
		  } else {
			break;
		  }
		  */
		}
		/// 移動
		if (k > 1)
		{
          l = 1;
		  do
		  {
		    pDec[j+l] = pDec[j+k+l-1];
		    l++;
		  } while(pDec[j+k+l-1]);
		  pDec[j+l] = NULL;
		}
	  } else if (pDec[j][strlen(pDec[j])-1] == ')')
	  {
	    while (pDec[j+k])
		{
#ifdef UPDATE_20230619 // SEARCH命令でOR指定の前に括弧が指定されていなと正しく検索されない不具合
          p2 = pDec[j];
          while(*p2 == ' ')
		  {
	        p2++;
		  }
          if (*p2 == ')' && *pDec[j+k] == ')')
#else
		  if (strchr(pDec[j], ')') && *pDec[j+k] == ')')
#endif
		  {
		    strcat(pDec[j] , pDec[j+k]);
		    k++;
		  } else {
			break;
		  }
		}
		/// 移動
		if (k > 1)
		{
          l = 1;
		  do
		  {
		    pDec[j+l] = pDec[j+k+l-1];
		    l++;
		  } while(pDec[j+k+l-1]);
		  pDec[j+l] = NULL;
		}
	  }
	  strncpy(mDec[j], pDec[j], 79);
	  mDec[j][79] = '\x0';
      j++;	  
	};
	/// 項目数の最カウント
	j = 0;
    while(pDec[j])
    {
	  j++;
	}
    // フラグを変更対象のファイル名の一覧表を作成後にリネーム処理し、"~SG"が残されないようにする対策。
    sprintf(MailDir, "%s\\*-??????.MSG", pContext->mSelectDir);  // メッセージフォルダ取出し
#ifdef UPDATE_20190510 // NTFS以外のファイルシステムからのファイル一覧取得可能にする対策（一覧ソート）を追加
   if (bOtherFS) // TRUE:対応する FALSE:対応しない（旧仕様）
   {
     pfd = FindFirstFileSort((LPCTSTR)MailDir, &pContext->FindData, &nSearch);
     no = 0;
   } else {
	 hSearch = FindFirstFile((LPCTSTR)MailDir, &pContext->FindData);
   }
   if (bOtherFS && pfd || !bOtherFS && hSearch != INVALID_HANDLE_VALUE)
   {
#else
    hSearch = FindFirstFile((LPCTSTR)MailDir, &pContext->FindData);
    if (hSearch != INVALID_HANDLE_VALUE)
	{
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

        // 検索条件リストア
//printf("Restore\n");
		mCharSet[0] = '\x0';
		for (i = 0; i < j; i++)
		{
		  pDec[i] = pDecBackup[i];
	      strcpy(pDec[i], mDec[i]);
//printf("[%d] [%s]\n", i, pDec[i]);
		};

        // "UID SEARCH"での番号指定での誤った応答結果を返してしまう不具合。
		nDec = 0;
	    nMSG++; // MSGファイル数のカウント
#ifdef UPDATE_20230627A // UPDATE_20230624のメールデータを事前に一括して読込み検索するフラグを追加
        pContext->pBuff = NULL;
        if (bBulkSearch)
		{
          sprintf(mFn, "%s/%s", pContext->mSelectDir, pContext->FindData.cFileName);
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
#endif
#ifdef UPDATE_20230701 // 検索中にMIME-Bエンコードの行で区切りコードが抜けていると処理が終わらない不具合
        if (bDebug) printf("SearchFrame(%s) Sstart\n", mFn);
#endif
#ifdef UPDATE_20230617 // SEARCH命令でOR指定が含まれると正しく検索結果が出力されない不具合
        bResult = SearchFrame(pContext, FALSE, nMSG, mCharSet, pDec, &nDec);
#else
        bResult = SearchFrame(pContext, nMSG, mCharSet, pDec, &nDec);
#endif
		while(nDec < j)
		{
#ifdef UPDATE_20230617 // SEARCH命令でOR指定が含まれると正しく検索結果が出力されない不具合
          bResult &= SearchFrame(pContext, FALSE, nMSG, mCharSet, pDec, &nDec);
#else
          bResult &= SearchFrame(pContext, nMSG, mCharSet, pDec, &nDec);
#endif
		}
#ifdef UPDATE_20230627A // UPDATE_20230624のメールデータを事前に一括して読込み検索するフラグを追加
        if (bBulkSearch && pContext->pBuff)
          free(pContext->pBuff);
#endif
//printf("--------------\n");
		if (bResult) 
		{
		  if (!bSend) 
		  {
            sprintf(pContext->mMessages, "* SEARCH");
            put_reply(lpClientContext, FALSE, TRUE);
			bSend = TRUE;
		  }
          // "UID SEARCH"での番号指定での誤った応答結果を返してしまう不具合。
		  sprintf(mWork, " %u", (pContext->bUID == 0 ? nMSG : atol(pContext->FindData.cFileName)));
          strcpy(pContext->mMessages, mWork);
          put_reply(lpClientContext, FALSE, TRUE);
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
	  //////////////////////////////////////////
    }
    if (bSend) { // 結果がある場合のみ
      strcpy(pContext->mMessages, "\r\n");
      put_reply(lpClientContext, TRUE, TRUE); // データFlash
	}
#ifdef SEARCH_NO_RESS_TEST // 使用不可のオプションでNOを返す
    return nResult;
#endif
}

#else

#ifdef SEARCH_NO_RESS_TEST // 使用不可のオプションでNOを返す
int SearchMSGByCondition(PCLIENT_CONTEXT lpClientContext, char **pDec)
#else
void SearchMSGByCondition(PCLIENT_CONTEXT lpClientContext, char **pDec)
#endif
{
#ifdef SEARCH_NO_RESS_TEST // 使用不可のオプションでNOを返す
  int nResult = TRUE;
#endif
#ifdef UPDATE_20190510 // NTFS以外のファイルシステムからのファイル一覧取得可能にする対策（一覧ソート）を追加
  DWORD           hSearch, no;
  WIN32_FIND_DATA  *pfd;
#else
  HANDLE           hSearch;
#endif
  //WIN32_FIND_DATA  FindData;
  PImap4Context    pContext = &lpClientContext->Context;
  CHAR             MailDir[MAX_PATH], mWork[MAX_PATH];
  CHAR             mUID[32];
  CHAR             *p, *p2, *p3;
  //////////////////////////////////////////
  DWORD            nToken = 0;                    // 検索文字列数
  DWORD            nToken2 = 0;                    // 検索文字列数
  IMAP4STK         stk[MAX_ATTRIBUTE];            // 検索文字列構造体
  //////////////////////////////////////////
  DWORD            i, j, k, l, n, nMSG = 0, nDec = 0;
  BOOL             bSend = FALSE;
  BOOL             bResult, bUID = FALSE;
  BOOL             bNOT = FALSE, bOR = FALSE;
#ifdef UPDATE_20060418 // カッコつきの処理
  BOOL             bGrp = FALSE;
#endif
#ifdef UPDATE_20150606 // 先頭にORがある時、検索結果が正しく出力されない
  int             nORStart = 0;
  int             nAnd = 0;
#endif
#ifdef UPDATE_20150606A // 先頭にORがある時、検索結果が正しく出力されない
  int             nGrp = 0;
#endif
#ifdef UPDATE_20150607 // 検索階層カウンタを追加
  int             nNowGrp = 0;
  int             nLastGrp = 0;
  int             nGroup = 0;
  BOOL            bGrpOR = FALSE;
  BOOL            bTopGrp = FALSE;
#endif
//	INT   nMode; 
 // 0:FLAG/1:NO/2:内部日付/3:ファイルサイズ/4:メッセージ
#ifdef UPDATE_20150607 // 検索階層カウンタを追加
  stk[0].nGrpOR = -1;
  nGrp++;
#endif
#ifdef UPDATE_20091129I // SEARCH CHRSETのUTF-8対応 CatchMe@Mailで添付きメールの表示が失敗する
  stk[0].mCharSet[0] = '\x0';
#endif
#ifdef UPDATE_20140620 // "UID SEARCH"での番号指定での誤った応答結果を返してしまう不具合。
  stk[nToken].nFrom = stk[nToken].nTo = 0;
#endif
  while(pDec[nDec]) {
#ifdef UPDATE_20150607 // 検索階層カウンタを追加
	 //bOR = FALSE;
     stk[nToken].nGrpOR = (int)-1;
#endif
	 if (!*pDec[nDec]) // データがないなら
	   break;
	 p = pDec[nDec];
#ifdef UPDATE_20060418 // カッコつきの処理
	 if (*p == '(') {
#ifdef UPDATE_20150607 // 検索階層カウンタを追加
	   stk[nToken].nGrpOR = (int)bOR;
       nGroup = 0;
#endif
	   bGrp = TRUE;
	   p++;
#ifdef UPDATE_20150606A // 先頭にORがある時、検索結果が正しく出力されない
       while(*p == ' ')
	   {
		 p++;
	   }
       nGrp++;
#endif
	 } else {
	   bGrp = TRUE;
       //nGrp++;
	 }
	 if (*p == ')') {
#ifdef UPDATE_20150607 // 検索階層カウンタを追加
	   bOR = FALSE;
       //nGroup = 0;
#endif
#ifdef UPDATE_20150606A // 先頭にORがある時、検索結果が正しく出力されない
       nGrp--;
	   if (nGrp == 0)
#endif
	   {
	     bGrp = FALSE;
	   }
	   p++;
#ifdef UPDATE_20150606A // 先頭にORがある時、検索結果が正しく出力されない
       while(*p == ' ')
	   {
		 p++;
	   }
	   if (!*p)
	   {
	     nDec++;
         //nToken++;
	     continue;
	   }
#endif
	 }
	 if (!bGrp)
	   bNOT = bOR = FALSE;
//#ifdef UPDATE_20150606 // 先頭にORがある時、検索結果が正しく出力されない
//	 if (!strnicmp(p, "or ", 3) || !stricmp(p, "or") || bOR && bGrp) || nORStart == 1)
//#else
	 if (!strnicmp(p, "or ", 3) || !stricmp(p, "or") || bOR && bGrp)
//#endif
#else
 	 bNOT = bOR = FALSE;
	 if (!strnicmp(p, "or ", 3) || !stricmp(p, "or"))
#endif
	 {
	   bOR = TRUE;
#ifdef UPDATE_20150607 // 検索階層カウンタを追加
	   if (!strnicmp(p, "or ", 3) || !stricmp(p, "or"))
	   {
	     nGroup = 0;
		 if (*(p-1) != '(')
           nGrp++;
	   }
#endif
	   /*
#ifdef UPDATE_20150606 // 先頭にORがある時、検索結果が正しく出力されない
	   if (!bGrp)
	   {
         if (nORStart > 0)//if (nORStart > 2)
		 {
           nORStart = 0; // リセット
		 } else {
	       nORStart++;
		 }
	   } else {
         nORStart = 0;
	   }
#endif
	   */

#ifdef UPDATE_20090325B // SEARCH命令で複雑な入れ子の命令が処理されない。
	   if (!strnicmp(p, "or ", 3) || !stricmp(p, "or"))
#endif
	   {
	     p = pDec[++nDec];
	   }
	 } else {
	   bOR = FALSE;
#ifdef UPDATE_20150606 // 先頭にORがある時、検索結果が正しく出力されない
       //nORStart = 0; // リセット
#endif
	 }

#ifdef UPDATE_20150607 // 検索階層カウンタを追加
	 if (*p == '(') {
#ifdef UPDATE_20150607 // 検索階層カウンタを追加
	   stk[nToken].nGrpOR = (int)bOR;
	   bGrpOR = bOR;
       nGroup = 0;
#endif
       nGrp++;
	   bGrp = TRUE;
	   p++;
	   if (!strnicmp(p, "or ", 3) || !stricmp(p, "or"))
	   {
	     bOR = TRUE;
	     p = pDec[++nDec];
	   } else {
	     bOR = FALSE;
	   }
	 } 
	 if (*p == ')') 
	 {
#ifdef UPDATE_20150607 // 検索階層カウンタを追加
	   bOR = FALSE;
       //nGroup = 0;
#endif
       nGrp--;
	   if (nGrp == 0)
	   {
	     bGrp = FALSE;
	   }
	   p++;
	 }
#endif

#ifdef UPDATE_20090325 // 複雑なSEARCH命令の解読でハングすることがある。
	 if (!p)
	 {
	     bNOT = FALSE;
		 bUID = FALSE;
	 } else 
#endif
	 {
       bNOT = FALSE;
	   if (!strnicmp(p, "uid ", 4) || !stricmp(p, "uid")) {
#ifdef UPDATE_20140620 // "UID SEARCH"での番号指定での誤った応答結果を返してしまう不具合。
         if (nToken == 0)
		 {
	       ////////////////////////////
	       /// 初期設定（共通）
#ifdef UPDATE_20150607 // 検索階層カウンタを追加
	       stk[nToken].nGrp = nGrp;           // 階層
#endif
           stk[nToken].bResult = FALSE;
	       stk[nToken].bUnrelated = FALSE; // 関係有り
	       stk[nToken].bNOT = bNOT;
	       stk[nToken].bOR = bOR;
#ifdef UPDATE_20091203 // SEARCHの番号指定で結果フラグが初期化されていなかった。
           stk[nToken].nMode = 1;
	       stk[nToken].nUid  = (bUID ? 1 : 0);  // UID番号 : 連番
#endif
		   nToken++;
		 }
#endif
	     bUID = TRUE;
	     p = pDec[++nDec];
	   } else {
	     bUID = FALSE;
	   }
#ifdef UPDATE_20140620A // SEARCH命令に続くUIDトークンの引数が不足しているとハングする不具合
   	   if (p)
#endif
	   {

#ifdef UPDATE_20060418 // カッコつきの処理
	     if (!strnicmp(p, "NOT ", 4) || !stricmp(p, "not") || bNOT && bGrp)
#else
	     if (!strnicmp(p, "NOT ", 4) || !stricmp(p, "not"))
#endif
		 {
	       bNOT = TRUE;
	       p +=4;
		 } else {
	       bNOT = FALSE;
		 }
	   }
	 }
	 ////////////////////////////
	 /// 初期設定（共通）
#ifdef UPDATE_20150607 // 検索階層カウンタを追加
     stk[nToken].nGrp = nGrp;           // 階層
#endif
     stk[nToken].bResult = FALSE;
	 stk[nToken].bUnrelated = FALSE; // 関係有り
	 stk[nToken].bNOT = bNOT;
	 stk[nToken].bOR = bOR;
#ifdef UPDATE_20091203 // SEARCHの番号指定で結果フラグが初期化されていなかった。
     stk[nToken].nMode = 1;
	 stk[nToken].nUid  = (bUID ? 1 : 0);  // UID番号 : 連番
#endif
#ifdef UPDATE_20090325 // 複雑なSEARCH命令の解読でハングすることがある。
	 if (!p)
	 {
	   nDec++;
       nToken++;
	   continue;
	 }
#endif
	 /////////////////////////////////////////////
	 /// 連番 又は UID番号
	 //if (nDec == 0 && (p2 = strpbrk(p, ",:"))) {
	 if ((p2 = strpbrk(p, ",:"))) {
       //bFromTo = TRUE;
	   while(1) {
	     ////////////////////////////
	     /// 初期設定（共通）
#ifdef UPDATE_20150607 // 検索階層カウンタを追加
         stk[nToken].nGrp = nGrp;           // 階層
#endif
         stk[nToken].bResult = FALSE;
	     stk[nToken].bUnrelated = FALSE; // 関係有り
	     stk[nToken].bNOT = bNOT;
	     stk[nToken].bOR = FALSE;
         stk[nToken].nMode = 1;
		 stk[nToken].nUid  = (bUID ? 1 : 0);  // UID番号 : 連番
         if (p2) {
		   if (*p2 == ':') {
             *p2 = '\x0';
	         p2++;
	         stk[nToken].nFrom = atol(p);
		     if (*p2 == '*')
		       stk[nToken].nTo = 0;     // '*'なら0に置換え
		     else
	           stk[nToken].nTo = atol(p2);
		   } else {
	 	     stk[nToken].nFrom = stk[nToken].nTo = atol(p);
		   }
		 } else {
	 	   stk[nToken].nFrom = stk[nToken].nTo = atol(p);
		   break;
		 }
		 if (*p2 == ',') { // 続きの番号アリ
		   p = ++p2;
		   p2 = strpbrk(p, ",:");
		   nToken++;
		 } else {
		   break;
		 }
	   }
#ifdef UPDATE_20140620 // "UID SEARCH"での番号指定での誤った応答結果を返してしまう不具合。
	 } else if (stricmp(p, "UID") == 0 ||
		        stricmp(p, "ALL") == 0) {   // デフォルトの為無視
       stk[nToken].nFrom = stk[nToken].nTo = 0;
       nToken++;
#else
	 } else if (stricmp(p, "ALL") == 0) {   // デフォルトの為無視
#endif
	   ++nDec;
	   continue;
	 } else if (stricmp(p, "RECENT") == 0) {
       stk[nToken].nMode = 0;
	   stk[nToken].bNOT = (bNOT ? TRUE : FALSE);
	   stk[nToken].nFlag = (0x1 << RECENT);
	 } else if (stricmp(p, "NEW") == 0) { // =(RECENT UNSEEN)
       stk[nToken].nMode = 0;
	   stk[nToken].bNOT = (bNOT ? TRUE : FALSE);
	   stk[nToken].nFlag = (0x1 << RECENT);
	   nToken++;
       stk[nToken].nMode = 0;
	   stk[nToken].bNOT = (bNOT ? FALSE : TRUE);
	   stk[nToken].nFlag = (0x1 << SEEN);
	 } else if (stricmp(p, "OLD") == 0) {
       stk[nToken].nMode = 0;
	   stk[nToken].bNOT = (bNOT ? FALSE : TRUE);
	   stk[nToken].nFlag = (0x1 << RECENT);
	 } else if (stricmp(p, "FLAGGED") == 0) {
       stk[nToken].nMode = 0;
	   stk[nToken].bNOT = (bNOT ? TRUE : FALSE);
	   stk[nToken].nFlag = (0x1 << FLAGGED);
	 } else if (stricmp(p, "UNFLAGGED") == 0) {
       stk[nToken].nMode = 0;
	   stk[nToken].bNOT = (bNOT ? FALSE : TRUE);
	   stk[nToken].nFlag = (0x1 << FLAGGED);
	 } else if (stricmp(p, "ANSWERED") == 0) {
       stk[nToken].nMode = 0;
	   stk[nToken].bNOT = (bNOT ? TRUE : FALSE);
	   stk[nToken].nFlag = (0x1 << ANSWERED);
	 } else if (stricmp(p, "UNANSWERED") == 0) {
       stk[nToken].nMode = 0;
	   stk[nToken].bNOT = (bNOT ? FALSE : TRUE);
	   stk[nToken].nFlag = (0x1 << ANSWERED);
	 } else if (stricmp(p, "DELETED") == 0) {
       stk[nToken].nMode = 0;
	   stk[nToken].bNOT = (bNOT ? TRUE : FALSE);
	   stk[nToken].nFlag = (0x1 << DELETED);
	 } else if (stricmp(p, "UNDELETED") == 0) {
       stk[nToken].nMode = 0;
	   stk[nToken].bNOT = (bNOT ? FALSE : TRUE);
	   stk[nToken].nFlag = (0x1 << DELETED);
	 } else if (stricmp(p, "DRAFT") == 0) {
       stk[nToken].nMode = 0;
	   stk[nToken].bNOT = (bNOT ? TRUE : FALSE);
	   stk[nToken].nFlag = (0x1 << DRAFT);
	 } else if (stricmp(p, "UNDRAFT") == 0) {
       stk[nToken].nMode = 0;
	   stk[nToken].bNOT = (bNOT ? FALSE : TRUE);
	   stk[nToken].nFlag = (0x1 << DRAFT);
	 } else if (stricmp(p, "SEEN") == 0) {
       stk[nToken].nMode = 0;
	   stk[nToken].bNOT = (bNOT ? TRUE : FALSE);
	   stk[nToken].nFlag = (0x1 << SEEN);
	 } else if (stricmp(p, "UNSEEN") == 0) {
       stk[nToken].nMode = 0;
	   stk[nToken].bNOT = (bNOT ? FALSE : TRUE);
	   stk[nToken].nFlag = (0x1 << SEEN);
	 /////////////////////////////////////////////
	 /// 文字列検索
	 } else if (stricmp(p, "TEXT") == 0) {
       stk[nToken].nMode = 4;
	   stk[nToken].nArea = 0;                    // TEXTキー
       strcpy(stk[nToken].mKeyword, "");         // TEXTキー
#ifdef UPDATE_20150606B // 検索条件が長いと途中で切ってしまう不具合
	   if (pDec[nDec+1])
	     nDec++;
#else
	   if (pDec[nDec+1])
	     pDec[++nDec][sizeof(stk[0].mToken)-1] = '\x0'; // バッファフローしないように
#endif
	   if (pDec[nDec][0] == '"') {
         strcpy(stk[nToken].mToken, &pDec[nDec][1]);
	     strtok(stk[nToken].mToken, "\"");
	   } else if (pDec[nDec][0] == '{') {
         p3 = strpbrk(pDec[nDec], "}");
		 n = atol(&pDec[nDec][1]);
		 strncpy(stk[nToken].mToken, ++p3, n);
		 stk[nToken].mToken[n] = '\x0';
	   } else {
         strcpy(stk[nToken].mToken, pDec[nDec]);
	   }
#ifdef UPDATE_20230624A2 // SEARCH BODY命令で検索速度の向上対策:検索キーワード側を文字コード別に準備して比較する
       ConvertSearchKey((IMAP4STK *)&stk[nToken]);
#endif
	 } else if (stricmp(p, "BODY") == 0) {
#ifdef UPDATE_20091129I // SEARCH CHRSETのUTF-8対応 CatchMe@Mailで添付きメールの表示が失敗する
	   if (nToken > 0)
	   {
         strcpy(stk[nToken].mCharSet, stk[nToken-1].mCharSet);
       }
#endif
       stk[nToken].nMode = 4;
	   stk[nToken].nArea = 1;               // BODYキー
       strcpy(stk[nToken].mKeyword, "");         // BODYキー
#ifdef UPDATE_20150606B // 検索条件が長いと途中で切ってしまう不具合
	   if (pDec[nDec+1])
	     nDec++;
#else
	   if (pDec[nDec+1])
	     pDec[++nDec][sizeof(stk[0].mToken)-1] = '\x0'; // バッファフローしないように
#endif
	   if (pDec[nDec][0] == '"') {
         strcpy(stk[nToken].mToken, &pDec[nDec][1]);
	     strtok(stk[nToken].mToken, "\"");
	   } else if (pDec[nDec][0] == '{') {
         p3 = strpbrk(pDec[nDec], "}");
		 n = atol(&pDec[nDec][1]);
		 strncpy(stk[nToken].mToken, ++p3, n);
		 stk[nToken].mToken[n-1] = '\x0';
	   } else {
         strcpy(stk[nToken].mToken, pDec[nDec]);
	   }
#ifdef UPDATE_20230624A2 // SEARCH BODY命令で検索速度の向上対策:検索キーワード側を文字コード別に準備して比較する
       ConvertSearchKey((IMAP4STK *)&stk[nToken]);
#endif
	 } else if (stricmp(p, "CHARSET") == 0) { // 検索キーのキャラクタセット
	   ++nDec;
#ifdef SEARCH_NO_RESS_TEST // 使用不可のオプションでNOを返す
	   if (strcmp(pDec[nDec], "iso-2022-jp"))
	   {
         nResult = FALSE;
		 return nResult;
	   }
#endif

#ifdef UPDATE_20091129I // SEARCH CHRSETのUTF-8対応 CatchMe@Mailで添付きメールの表示が失敗する
       strcpy(stk[nToken].mCharSet, pDec[nDec]);
       //nToken++;
#endif
	   ++nDec;
	   continue;  // 無視
	   stk[nToken].bUnrelated = TRUE;
       stk[nToken].nMode = 4;
	   stk[nToken].nArea = 2;                 // HEADERキー
	   strcpy(stk[nToken].mKeyword, "Content-Type:"); // HEADERキー
#ifdef UPDATE_20150606B // 検索条件が長いと途中で切ってしまう不具合
	   if (pDec[nDec+1])
	     nDec++;
#else
	   if (pDec[nDec+1])
  	     pDec[++nDec][sizeof(stk[0].mToken)-1] = '\x0'; // バッファフローしないように
#endif
	   if (pDec[nDec][0] == '"') {
         strcpy(stk[nToken].mToken, &pDec[nDec][1]);
	     strtok(stk[nToken].mToken, "\"");
	   } else if (pDec[nDec][0] == '{') {
         p3 = strpbrk(pDec[nDec], "}");
		 n = atol(&pDec[nDec][1]);
		 strncpy(stk[nToken].mToken, ++p3, n);
		 stk[nToken].mToken[n-1] = '\x0';
	   } else {
         strcpy(stk[nToken].mToken, pDec[nDec]);
	   }
	 } else if (stricmp(p, "HEADER") == 0) {
       stk[nToken].nMode = 4;
	   stk[nToken].nArea = 2;                 // HEADERキー
#ifdef UPDATE_20150606B // 検索条件が長いと途中で切ってしまう不具合
	   if (pDec[nDec+1])
	     nDec++;
#else
	   if (pDec[nDec+1])
	     pDec[++nDec][sizeof(stk[0].mToken)-2] = '\x0'; // バッファフローしないように
#endif
       strcpy(stk[nToken].mKeyword, pDec[nDec]); // HEADERキー
	   if (stk[nToken].mKeyword[strlen(stk[nToken].mKeyword)-1] != ':')
		 strcat(stk[nToken].mKeyword, ":");
#ifdef UPDATE_20150606B // 検索条件が長いと途中で切ってしまう不具合
	   if (pDec[nDec+1])
	     nDec++;
#else
	   if (pDec[nDec+1])
	     pDec[++nDec][sizeof(stk[0].mToken)-1] = '\x0'; // バッファフローしないように
#endif
	   if (pDec[nDec][0] == '"') {
         strcpy(stk[nToken].mToken, &pDec[nDec][1]);
	     strtok(stk[nToken].mToken, "\"");
	   } else if (pDec[nDec][0] == '{') {
         p3 = strpbrk(pDec[nDec], "}");
		 n = atol(&pDec[nDec][1]);
		 strncpy(stk[nToken].mToken, ++p3, n);
		 stk[nToken].mToken[n-1] = '\x0';
	   } else {
         strcpy(stk[nToken].mToken, pDec[nDec]);
	   }
#ifdef UPDATE_20230624A2 // SEARCH BODY命令で検索速度の向上対策:検索キーワード側を文字コード別に準備して比較する
       ConvertSearchKey((IMAP4STK *)&stk[nToken]);
#endif
	 } else if (stricmp(p, "BCC") == 0 ||
		        stricmp(p, "CC") == 0 ||
		        stricmp(p, "FROM") == 0 ||
		        stricmp(p, "SUBJECT") == 0 ||
		        stricmp(p, "TO") == 0 ) {
#ifdef UPDATE_20091129I // SEARCH CHRSETのUTF-8対応 CatchMe@Mailで添付きメールの表示が失敗する
	   if (nToken > 1)
	   {
         strcpy(stk[nToken].mCharSet, stk[nToken-1].mCharSet);
       }
#endif
       stk[nToken].nMode = 4;
	   stk[nToken].nArea = 2;                      // HEADERキー
       sprintf(stk[nToken].mKeyword, "%s:", p);    // HEADERキー
#ifdef UPDATE_20150606B // 検索条件が長いと途中で切ってしまう不具合
	   if (pDec[nDec+1])
	     nDec++;
#else
	   if (pDec[nDec+1])
	     pDec[++nDec][sizeof(stk[0].mToken)-1] = '\x0'; // バッファフローしないように
#endif
	   if (pDec[nDec][0] == '"') {
         strcpy(stk[nToken].mToken, &pDec[nDec][1]);
	     strtok(stk[nToken].mToken, "\"");
	   } else if (pDec[nDec][0] == '{') {
         p3 = strpbrk(pDec[nDec], "}");
		 n = atol(&pDec[nDec][1]);
		 strncpy(stk[nToken].mToken, ++p3, n);
		 stk[nToken].mToken[n-1] = '\x0';
	   } else {
         strcpy(stk[nToken].mToken, pDec[nDec]);
	   }
#ifdef UPDATE_20230624A2 // SEARCH BODY命令で検索速度の向上対策:検索キーワード側を文字コード別に準備して比較する
       ConvertSearchKey((IMAP4STK *)&stk[nToken]);
#endif
	 } else if (stricmp(p, "SENTBEFORE") == 0 ||
		        stricmp(p, "SENTON") == 0 ||
		        stricmp(p, "SENTSINCE") == 0) {
       stk[nToken].nMode = 4;
	   if (stricmp(p, "SENTBEFORE") == 0)
		 stk[nToken].nDType = 0; // BEFORE
	   else if (stricmp(p, "SENTON") == 0)
		 stk[nToken].nDType = 1; // ON
	   else if (stricmp(p, "SENTSINCE") == 0)
		 stk[nToken].nDType = 2; // SINCE
	   stk[nToken].nArea = 2;                        // HEADERキー
       sprintf(stk[nToken].mKeyword, "DATE:", p);    // HEADERキー
#ifdef UPDATE_20150606B // 検索条件が長いと途中で切ってしまう不具合
	   if (pDec[nDec+1])
	     nDec++;
#else
	   if (pDec[nDec+1])
	     pDec[++nDec][sizeof(stk[0].mToken)-1] = '\x0'; // バッファフローしないように
#endif
       strcpy(stk[nToken].mToken, pDec[nDec]);
	 /////////////////////////////////////////////
	 /// ファイルサイズ
	 } else if (stricmp(p, "LARGER") == 0) {
       stk[nToken].nMode = 3;
       stk[nToken].nFileSizeType  = 0;
	   if (pDec[nDec+1])
         stk[nToken].nFileSize = atol(pDec[++nDec]);
	 } else if (stricmp(p, "SMALLER") == 0) {
       stk[nToken].nMode = 3;
       stk[nToken].nFileSizeType  = 1;
	   if (pDec[nDec+1])
         stk[nToken].nFileSize = atol(pDec[++nDec]);
	 /////////////////////////////////////////////
	 /// 内部日付
	 } else if (stricmp(p, "BEFORE") == 0) {
       stk[nToken].nMode = 2;
       stk[nToken].nDType  = 0;
	   if (pDec[nDec+1])
         strcpy(stk[nToken].mToken, pDec[++nDec]);
	 } else if (stricmp(p, "ON") == 0) {
       stk[nToken].nMode = 2;
       stk[nToken].nDType  = 1;
	   if (pDec[nDec+1])
         strcpy(stk[nToken].mToken, pDec[++nDec]);
	 } else if (stricmp(p, "SINCE") == 0) {
       stk[nToken].nMode = 2;
       stk[nToken].nDType  = 2;
	   if (pDec[nDec+1])
         strcpy(stk[nToken].mToken, pDec[++nDec]);
	 /////////////////////////////////////////////
#ifdef UPDATE_20150606A // 先頭にORがある時、検索結果が正しく出力されない
	 } else if (stricmp(p, "(OR") == 0 || stricmp(p, "( OR") == 0) {
       continue;
#endif
#ifdef UPDATE_20091203 // SEARCHの番号指定で結果フラグが初期化されていなかった。
	 } else {
       stk[nToken].nFrom = stk[nToken].nTo = atol(p);
#endif
	 }
#ifdef UPDATE_20150607 // 検索階層カウンタを追加
	 nGroup++;
	 while (bGrp && (stk[nToken].mToken[strlen(stk[nToken].mToken)-1] == ')' || nGroup == 2 /*|| nGrp == 2*/))
#else
	 while (bGrp && (stk[nToken].mToken[strlen(stk[nToken].mToken)-1] == ')'))
#endif
#ifdef UPDATE_20150606 // 先頭にORがある時、検索結果が正しく出力されない
	 //while (bGrp && (stk[nToken].mToken[strlen(stk[nToken].mToken)-1] == ')' || nGroup == 2))
	 {
#ifdef UPDATE_20150607 // 検索階層カウンタを追加
	   bOR &= bGrpOR;
	   stk[nToken].bOR = bOR;
	   nGroup = 0;
	   if (stk[nToken].mToken[strlen(stk[nToken].mToken)-1] == ')')
#endif
	   {
	     stk[nToken].mToken[strlen(stk[nToken].mToken)-1] = '\x0'; // 括弧除去
	   }
#ifdef UPDATE_20150606A // 先頭にORがある時、検索結果が正しく出力されない
       nGrp--;
	   if (nGrp == 1)//if (nGrp == 0)
#endif
	   {
	     bGrp = FALSE;
		 bOR = FALSE;
	     stk[nToken].bOR = bOR;
	   }
	 }
#endif
	 nDec++;
     nToken++;
  } 

    //mMSG[0] = '\x0';
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
   if (pfd = FindFirstFileSort((LPCTSTR)MailDir, &pContext->FindData, &hSearch))
   {
     no = 0;
#else
    hSearch = FindFirstFile((LPCTSTR)MailDir, &pContext->FindData);
    if (hSearch != INVALID_HANDLE_VALUE){
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
#ifdef UPDATE_20140620 // "UID SEARCH"での番号指定での誤った応答結果を返してしまう不具合。
	    nMSG++; // MSGファイル数のカウント
#else
		if (pContext->bUID) { // MSGのUID
		  strcpy(mUID, pContext->FindData.cFileName);
          strtok(mUID, "-");
		  nMSG = (DWORD) atol(mUID);
		} else {
		  nMSG++; // MSGファイル数のカウント
		}
#endif
        search_falgs(pContext, nToken, stk);
        search_number(pContext, nToken, nMSG, stk);
        search_internaldate(pContext, nToken, stk);
        search_size(pContext, nToken, stk);
        search_rfc822(pContext, nToken, stk);

#ifdef UPDATE_20060418 // カッコつきの処理
		bResult = TRUE;
#ifdef UPDATE_20150606 // 先頭にORがある時、検索結果が正しく出力されない
	    nAnd = -1;
#endif
#ifdef UPDATE_20150607 // 検索階層カウンタを追加
	    bTopGrp = FALSE;
		for (i = 0; i < nToken; i++) // 最後の階層値を取得
		{
//printf("[%s] [%d] nGrpOR=%d, nGrp=%d, bOR=%d, bResult=%d\n", pContext->FindData.cFileName, i, stk[i].nGrpOR, stk[i].nGrp, stk[i].bOR, stk[i].bResult);
           if (stk[i].nGrp == 1)
		   {
			 bTopGrp = TRUE;
		   }
           if (stk[i].nGrp == 0)
		   {
			 stk[i].nGrp = 1;
		   }
           if (stk[i].nGrp>nLastGrp)
		   {
			 nLastGrp = stk[i].nGrp;
		   }
           stk[i].nGrp2 = stk[i].nGrp; // バックアップ
           stk[i].bOR2 = stk[i].bOR; // バックアップ
		}
		
		// 階層を正規化
		while(!bTopGrp)
		{
		  for (i = 0; i < nToken; i++) 
		  {
            stk[i].nGrp--;
            if (stk[i].nGrp == 1)
			{
			  bTopGrp = TRUE;
			}
		  }
		}
		//nToken2 = nToken;
		for (nNowGrp = nLastGrp; nNowGrp >= 0; nNowGrp--) // 階層毎に演算
		{
//printf("[NowGrp] = %d\n", nNowGrp);
		  do
		  {
	        k = 0;
		    for (i = 1; i < nToken; i++) 
			{
			  //if (nNowGrp == stk[i-1].nGrp && (stk[0].bOR || !stk[0].bOR && nNowGrp == stk[i].nGrp))
			  if (nNowGrp == stk[i-1].nGrp && nNowGrp == stk[i].nGrp)
			  {
//printf("[%s] [i-1=%d] [i=%d]  stk[i-1].bOR=%d stk[i-1].bResult=%d stk[i].bResult=%d\n", pContext->FindData.cFileName, i-1, i, stk[i-1].bOR, stk[i-1].bResult, stk[i].bResult);
			    if (stk[i-1].bOR) // OR
				{
				  stk[i-1].bResult |= stk[i].bResult;
				} else { // AND
				  stk[i-1].bResult &= stk[i].bResult;
				}
//printf("stk[i-1].bOR=%d stk[i-1].bResult=%d stk[i].bResult=%d\n", stk[i-1].bOR, stk[i-1].bResult, stk[i].bResult);
		        //stk[i-1].bOR = stk[i].bOR; // 2015.06.10　追加
                //if (stk[i-1].nGrp > stk[i].nGrp)
                if (nNowGrp > 1)
		          stk[i-1].nGrp--;
				stk[i].nGrp = -1;
			    k = 1;
				//nToken2--;
			    break;
			  }
			}
			
            // 最後の項目処理の場合は移動処理しない。
			if (i < nToken-1)
			{
			  stk[i-1].bOR = stk[i].bOR;
		      for (j = i+1; j < nToken; j++)
			  {
				if (i > 0)
				{
				  stk[j-1].nGrp = stk[j].nGrp;
				}
				if (j-1 > 0)
				{
                  stk[j-1].bResult = stk[j].bResult;
				}
                stk[j-1].bOR = stk[j].bOR;
                stk[j-1].nGrpOR = stk[j].nGrpOR;
				stk[j].nGrp = -1;
			  }
			  	/*		  
		      for (i = 0; i < nToken; i++) 
			  {
			    if (i == 0 && stk[0].bOR || i > 0) // || stk[i].nGrp == -1)
				{
			      for (j = i+1; j < nToken; j++)
				  {
				    if (i > 0)
					{
				      stk[j-1].nGrp = stk[j].nGrp;
					}
				    stk[j].nGrp = -1;
				    if (j-1 > 0)
					{
                      stk[j-1].bResult = stk[j].bResult;
					}
                    stk[j-1].bOR = stk[j].bOR;
                    stk[j-1].nGrpOR = stk[j].nGrpOR;
				  }
				}
			  }
			 */ 
			}
			
		  } while(k & nToken > 2);
		}
//printf("[%s] [%d] nGrpOR=%d, nGrp=%d, bOR=%d, bResult=%d\n", pContext->FindData.cFileName, 0, stk[0].nGrpOR, stk[0].nGrp, stk[0].bOR, stk[0].bResult);
	    for (i = 0; i < nToken; i++) 
		{
//printf("[%s] [%d] nGrpOR=%d, nGrp=%d, bOR=%d, bResult=%d\n", pContext->FindData.cFileName, i, stk[i].nGrpOR, stk[i].nGrp, stk[i].bOR, stk[i].bResult);
		  //bResult &= stk[i].bResult;
          stk[i].nGrp = stk[i].nGrp2; // リストア
          stk[i].bOR = stk[i].bOR2; // リストア
		}
		bResult &= stk[0].bResult;
//printf("bResult=%d\n", bResult);
#else
		for (i = 0; i < nToken; i++) 
		{
#ifndef UPDATE_20091203 // SEARCHの番号指定で結果フラグが初期化されていなかった。
		  if (stk[i].nMode == 1)  // 番号なら読み飛ばし
			continue;
#endif
/*
printf("[%s] stk[%d].nGrp=%d, stk[%d].bOR=%d, stk[%d].bResult=%d\n", pContext->FindData.cFileName, i, stk[i].nGrp, i, stk[i].bOR, i, stk[i].bResult);
if (!stricmp(pContext->FindData.cFileName, "0000001482-000000.MSG"))
{
  printf("hit!!\n");
}
*/
#ifdef UPDATE_20090325B // SEARCH命令で複雑な入れ子の命令が処理されない。
		  if (stk[i].bOR) {
			if (i >= 1)
			{
#ifdef UPDATE_20150606 // 先頭にORがある時、検索結果が正しく出力されない
			  if (nAnd == -1)
			  {
			    bResult |=  stk[i].bResult;
			  } else {
		        if (nAnd == i - 1)
				{
			      bResult =  stk[nAnd].bResult & stk[i].bResult;
				} else {
			      bResult |=  stk[nAnd].bResult & stk[i].bResult;
				}
			  }
#else
			  bResult |=  stk[i].bResult;
#endif
			} else {
			  bResult = stk[i].bResult;
			}
		  } else {
#ifdef UPDATE_20150606 // 先頭にORがある時、検索結果が正しく出力されない
			nAnd = i;
#endif
			if (i >= 1)
			  bResult &= stk[i].bResult;
			else
			  bResult = stk[i].bResult;
		  }
#else
		  if (stk[i].bOR) {
			if (i == 1)
			  bResult = FALSE | stk[i].bResult;
			else
			  bResult |= stk[i].bResult;
		  } else {
			if (i == 1)
			  bResult = TRUE & stk[i].bResult;
			else
			  bResult &= stk[i].bResult;
		  }
#endif
		}
#endif // UPDATE_20150607
#else
		bResult = TRUE;
		for (i = 0; i < nToken; i++) {
		  if (stk[i].nMode == 1)  // 番号なら読み飛ばし
			continue;
		  if (!stk[i].bResult) { // 不一致の条件がある場合は読み飛ばす。
            bResult = FALSE;
			break;
		  } else {
			if (stk[i].bOR && bResult) // OR 条件なら
			  break;
		  }
		}
#endif
		if (bResult) {
		  if (!bSend) {
            sprintf(pContext->mMessages, "* SEARCH");
            put_reply(lpClientContext, FALSE, TRUE);
			bSend = TRUE;
		  }
#ifdef UPDATE_20140620 // "UID SEARCH"での番号指定での誤った応答結果を返してしまう不具合。
		  sprintf(mWork, " %u", (pContext->bUID == 0 ? nMSG : atol(pContext->FindData.cFileName)));
#else
	      sprintf(mWork, " %u", nMSG);
#endif
          strcpy(pContext->mMessages, mWork);
          put_reply(lpClientContext, FALSE, TRUE);
		}
/*
		if ( (!bFromTo || (bFromTo && nFrom <= nMSG && (nTo == 0 || nTo >= nMSG) ) ) ) {
		   //// ファイルサイズ検索
		   if (bFileSize && search_size(pContext, nFileSizeType, nFileSize) ||
               !bFileSize ) {
			 //// 内部日付検索
		     if ( bInternalDate && search_internaldate(pContext, nInternalType, mDate) ||
                 !bInternalDate) {
			   //// メッセージ内のトークン検索
               if (bToken && search_rfc822(pContext, nToken, stk) ||
				   !bToken) {
	             sprintf(mWork, " %u", nMSG);
		         strcat(mMSG, mWork);
			   }
			 }
		   }
		}
*/
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
	  //////////////////////////////////////////
    }
    if (bSend) { // 結果がある場合のみ
      strcpy(pContext->mMessages, "\r\n");
      put_reply(lpClientContext, TRUE, TRUE); // データFlash
	}
#ifdef SEARCH_NO_RESS_TEST // 使用不可のオプションでNOを返す
    return nResult;
#endif
}
#endif

void search_falgs(PImap4Context pContext, DWORD nToken, IMAP4STK *stk) {
  DWORD            i = 0;
  CHAR             *p;

  p = &pContext->FindData.cFileName[UIDLEN]; // フラグ位置
  for (i = 0; i < nToken; i++) { // 結果の初期設定
    if (stk[i].nMode != 0)  //フラグに関する処理のみ
	  continue;
	if (stk[i].nFlag & (0x1 << RECENT)) {
	  if (!stk[i].bNOT && *(p+RECENT) == '1' ||
		   stk[i].bNOT && *(p+RECENT) == '0' ) {
		stk[i].bResult = TRUE;
	  } else {
		stk[i].bResult = FALSE;
	  }
	}
	if (stk[i].nFlag & (0x1 << ANSWERED)) {
	  if (!stk[i].bNOT && *(p+ANSWERED) == '1' ||
		   stk[i].bNOT && *(p+ANSWERED) == '0' ) {
		stk[i].bResult = TRUE;
	  } else {
		stk[i].bResult = FALSE;
	  }
	}
	if (stk[i].nFlag & (0x1 << FLAGGED)) {
	  if (!stk[i].bNOT && *(p+FLAGGED) == '1' ||
		   stk[i].bNOT && *(p+FLAGGED) == '0' ) {
		stk[i].bResult = TRUE;
	  } else {
		stk[i].bResult = FALSE;
	  }
	}
	if (stk[i].nFlag & (0x1 << DELETED)) {
	  if (!stk[i].bNOT && *(p+DELETED) == '1' ||
		   stk[i].bNOT && *(p+DELETED) == '0' ) {
		stk[i].bResult = TRUE;
	  } else {
		stk[i].bResult = FALSE;
	  }
	}
	if (stk[i].nFlag & (0x1 << SEEN)) {
	  if (!stk[i].bNOT && *(p+SEEN) == '1' ||
		   stk[i].bNOT && *(p+SEEN) == '0' ) {
		stk[i].bResult = TRUE;
	  } else {
		stk[i].bResult = FALSE;
	  }
	}
	if (stk[i].nFlag & (0x1 << DRAFT)) {
	  if (!stk[i].bNOT && *(p+DRAFT) == '1' ||
		   stk[i].bNOT && *(p+DRAFT) == '0' ) {
		stk[i].bResult = TRUE;
	  } else {
		stk[i].bResult = FALSE;
	  }
	}
  }
}

void search_number(PImap4Context pContext, DWORD nToken, DWORD nMSG, IMAP4STK *stk) {
  DWORD            i = 0;
  DWORD            nNo;
#ifdef UPDATE_20150625 // SEARCH命令で検索対象NOを複数羅列すると正しい結果が得られない不具合
#ifdef UPDATE_20150626 // SEARCH命令で検索対象NOにNOT指定を行うと正しい結果が得られない不具合
  BOOL             bResult = stk[0].bNOT;
  BOOL             bHit = stk[0].bNOT; 
  int              nHit = (stk[0].bNOT ? 0: -1);
#else
  BOOL             bResult = FALSE;
  BOOL             bHit = FALSE;
  int              nHit = -1;
#endif
  int              n[2] = {-1, -1};
#endif

#ifdef UPDATE_20150625 // SEARCH命令で検索対象NOを複数羅列すると正しい結果が得られない不具合
  for (i = 0; i < nToken; i++) { // 結果の初期設定
    if (stk[i].nMode != 1)  //番号に関する処理のみ
	  continue;
	if (n[0] == -1)
	{
	  n[0] = i;
	}
#else
  for (i = 0; i < nToken; i++) { // 結果の初期設定
    if (stk[i].nMode != 1)  //番号に関する処理のみ
	  continue;
#endif
#ifdef UPDATE_20150626A // SEARCH命令で検索対象NOをUID値で指定を行うと正しい結果が得られない不具合
	if (stk[i].nUid == 0)  // 連番
	  nNo = nMSG;
	else                   // UID番号
	  nNo = atol(pContext->FindData.cFileName);
#else
#ifdef UPDATE_20140620 // "UID SEARCH"での番号指定での誤った応答結果を返してしまう不具合。
	if (i == 0) { // 連番
	  nNo = nMSG;
	} else {                   // UID番号
	  if (stk[i].nUid == 0)  // 連番
	    nNo = nMSG;
	  else                   // UID番号
	    nNo = atol(pContext->FindData.cFileName);
	}
#else
	if (stk[i].nUid == 0)  // 連番
	  nNo = nMSG;
	else                   // UID番号
	  nNo = atol(pContext->FindData.cFileName);
#endif
#endif
#ifdef UPDATE_20091203 // SEARCHの番号指定で結果フラグが初期化されていなかった。
#ifdef UPDATE_20150626 // // SEARCH命令で検索対象NOにNOT指定を行うと正しい結果が得られない不具合
	stk[i].bResult = stk[0].bNOT;
#else
	stk[i].bResult = FALSE;
#endif
#endif
#ifdef UPDATE_20151217 // 範囲指定に0を指定するとワイルドカード処理されてしまう不具合
	if (nNo >= stk[i].nFrom && nNo <= stk[i].nTo)
#else
#ifdef UPDATE_20140620 // "UID SEARCH"での番号指定での誤った応答結果を返してしまう不具合。
	if (nNo >= stk[i].nFrom && (nNo <= stk[i].nTo || stk[i].nTo == 0))
#else
	if (nNo >= stk[i].nFrom && nNo <= stk[i].nTo)
#endif
#endif
	{
      if (!stk[i].bNOT)
        stk[i].bResult = TRUE;
      else
        stk[i].bResult = FALSE;
#ifdef UPDATE_20150625 // SEARCH命令で検索対象NOを複数羅列すると正しい結果が得られない不具合
#ifdef UPDATE_20150626A // SEARCH命令で検索対象NOをUID値で指定を行うと正しい結果が得られない不具合
	  if((stk[i].bNOT ? (nHit == 0): (nHit == -1)))
#else
	  if (nHit == -1)
#endif
	  {
        bHit = TRUE;
	    nHit = i;
	  }
#endif
	} 
  }
#ifdef UPDATE_20150625 // SEARCH命令で検索対象NOを複数羅列すると正しい結果が得られない不具合
  if (nHit >= 0)
  {
    if (n[1] == -1)
	{
	  n[1] = i;
	}
	bResult = stk[nHit].bResult;
    for (i = n[0]; i < n[1]; i++) // 結果の設定
	{
	  stk[i].bResult = bResult;
	}
  }
#endif
}

void Serach_Date(char *pDate, DWORD *nDate) {
   SYSTEMTIME lt;
   int i, j;
   char *p, *p2;

   lt.wYear = lt.wMonth = lt.wDayOfWeek = lt.wDay = 0;
   lt.wHour = lt.wMinute = lt.wSecond = lt.wMilliseconds = 0;
   // eg. 17-Jul-1996 02:44:25 -0700
   i = 0;
   p = pDate;
   while (*p) {
	 switch (i) {
 	   case 0: lt.wDay = atoi(p); break;
	   case 1: for (j = 0; j < 12; j++) {
		         if (!strnicmp(p, mMonth[j], 3)) {
				   lt.wMonth = j+1;
				   break;
				 }
			   }
		       break;
 	   case 2: lt.wYear = atoi(p); break;
	 }
	 p2 = strpbrk(p, " -");
	 if (p2)
	   p = ++p2;
	 else
	   break;
	 i++;
   }
   *nDate = lt.wYear*10000 + lt.wMonth*100 + lt.wDay;
}

void search_internaldate(PImap4Context pContext, DWORD nToken, IMAP4STK *stk) {
  DWORD            i = 0;
  HANDLE           hFile;
  FILETIME         ft;
#ifdef UPDATE_20150608 // 検索時の日付をUTC日付かローカル日付かに変更できるオプションを追加した TRUE:local FALSE:UTC
  FILETIME         flt;
#endif
  SYSTEMTIME       lt;
  DWORD            nDate[2];
  char             mFn[MAX_PATH];

  for (i = 0; i < nToken; i++) { // 結果の初期設定
    if (stk[i].nMode != 2)  // 内部日付に関する処理のみ
	  continue;
    if (!stk[i].bNOT)
      stk[i].bResult = FALSE;
    else
      stk[i].bResult = TRUE;
  }
   sprintf(mFn, "%s/%s", pContext->mSelectDir, pContext->FindData.cFileName);
   hFile = CreateFile( (LPCTSTR)mFn,
                       GENERIC_READ,
                       0,
                       NULL,
                       OPEN_EXISTING,
                       FILE_ATTRIBUTE_NORMAL, 
                       NULL);
   if (hFile != INVALID_HANDLE_VALUE) {
#ifdef UPDATE_20150609 // IMAPフォルダへのコピー時に受信日付が変わらないようにする
	if (nSearchType == 0) //0:更新日時 1:アクセス日時 2:作成日時 
	{
      GetFileTime(hFile, NULL, NULL, &ft); // 更新日時
	} else if (nSearchType == 1)
	{
      GetFileTime(hFile, NULL, &ft, NULL); // 最終アクセス日時
	} else 
	{
      GetFileTime(hFile, &ft, NULL, NULL); // 作成日時
	}
#else
     GetFileTime(hFile, &ft, NULL, NULL); // 作成日時
#endif
     CloseHandle(hFile);
	 //FileTimeToLocalFileTime(&ft, &lt); // ローカルタイム基準
#ifdef UPDATE_20150608 // 検索時の日付をUTC日付かローカル日付かに変更できるオプションを追加した TRUE:local FALSE:UTC
     if (bSearchTime)
	 {
       FileTimeToLocalFileTime(&ft, &flt); //日付検索がUTC基準になっていた
       FileTimeToSystemTime(&flt, &lt); //日付検索がUTC基準
	 } else 
#endif
	 {
       FileTimeToSystemTime(&ft, &lt); //日付検索がUTC基準
	 }
     nDate[1] = lt.wYear*10000 + lt.wMonth*100 + lt.wDay; // データ
     for (i = 0; i < nToken; i++) { // 結果の初期設定
       if (stk[i].nMode != 2)  // 内部日付に関する処理のみ
	     continue;
       Serach_Date(stk[i].mToken, &nDate[0]);             // 条件
	   switch (stk[i].nDType) {
	     case 0: if (nDate[0] > nDate[1]) // 以前
                   if (!stk[i].bNOT)
                     stk[i].bResult = TRUE;
                   else
                     stk[i].bResult = FALSE;
		           break;
	     case 1: if (nDate[0] == nDate[1]) // 当日
                   if (!stk[i].bNOT)
                     stk[i].bResult = TRUE;
                   else
                     stk[i].bResult = FALSE;
	             break;
	     case 2: if (nDate[0] <= nDate[1]) // 日中以降
                   if (!stk[i].bNOT)
                     stk[i].bResult = TRUE;
                   else
                     stk[i].bResult = FALSE;
		          break;
	   }
	 }
   }
}

void search_size(PImap4Context pContext, DWORD nToken, IMAP4STK *stk) {
  DWORD            i = 0;
  ULARGE_INTEGER   u[2];
  HANDLE           hFile;
  char             mFn[MAX_PATH];

  for (i = 0; i < nToken; i++) { // 結果の初期設定
    if (stk[i].nMode != 3)  // ファイルサイズに関する処理のみ
	  continue;
    if (!stk[i].bNOT)
      stk[i].bResult = FALSE;
    else
      stk[i].bResult = TRUE;
  }
   sprintf(mFn, "%s/%s", pContext->mSelectDir, pContext->FindData.cFileName);
   hFile = CreateFile( (LPCTSTR)mFn,
                       GENERIC_READ,
                       0,
                       NULL,
                       OPEN_EXISTING,
                       FILE_ATTRIBUTE_NORMAL, 
                       NULL);
   if (hFile != INVALID_HANDLE_VALUE) {
	 u[0].LowPart = GetFileSize(hFile, &u[0].HighPart);
     CloseHandle(hFile);
     for (i = 0; i < nToken; i++) { // 結果の初期設定
       if (stk[i].nMode != 3)  // ファイルサイズに関する処理のみ
	     continue;
	   u[1].LowPart = stk[i].nFileSize;
	   u[1].HighPart = 0;
	   switch (stk[i].nFileSizeType) {
	     case  0: if (u[0].QuadPart >= u[1].QuadPart) // サイズ以上
                   if (!stk[i].bNOT)
                     stk[i].bResult = TRUE;
                   else
                     stk[i].bResult = FALSE;
		          break;
	     case  1: if (u[0].QuadPart <= u[1].QuadPart) // サイズ以下
                   if (!stk[i].bNOT)
                     stk[i].bResult = TRUE;
                   else
                     stk[i].bResult = FALSE;
		          break;
	   }
	 }
   }
}

#ifdef UPDATE_20230624 // SEARCH BODY命令で検索速度の向上対策
char *mgets(char *pline, unsigned long nlen, char **ptkn)
{
 //改行ごとに取り出す。
	char c, c1, *p;
	unsigned int  n;

	*pline = '\x0';
	p = strpbrk(*ptkn, "\r\n");
	if (p ||
		**ptkn == '\r' ||
		**ptkn == '\n')
	{
      if (**ptkn == '\r' || **ptkn =='\n')
	    p = *ptkn;
	  c1 = *p;
	  p++;
	  while (*p=='\r'|| *p=='\n')
	  {
		if (*p == c1)
		  break;
		else
	     p++;
	  }
	  c = *p;
	  *p = '\x0';
	  n = min(strlen(*ptkn), nlen-1);
	  memcpy(pline, *ptkn, n);
	  *p =c;
	  //strncpy(pline, *ptkn, n);
	  *(pline+n)='\x0';
	  *(*ptkn+n)=c;
	  *ptkn+=n;
	}
#ifdef UPDATE_20230717 // BulkFetchフラグ有効時にファイルの最終行に改行"\r\n"がないとデータ取得が終わらない
	else {
	  *ptkn+=strlen(*ptkn);
	}
#endif
#ifdef UPDATE_20230706 // FETCH時のメールデータを事前に一括して読込み検索するフラグを追加
    if (p)
	  p=pline;
#endif
	return p;
}
#endif

void search_rfc822(PImap4Context pContext, DWORD nToken, IMAP4STK *stk) {
  char             *p, *pDate, mHead[128], mFn[MAX_PATH];
  char             *ps, *pl, mLine[1024], mDec[1024];
  FILE             *fp;
  DWORD            i = 0, n[2], nDate[2];
  BOOL             bHead = TRUE; //, bMach = FALSE;
#ifdef UPDATE_20091129I // SEARCH CHRSETのUTF-8対応 CatchMe@Mailで添付きメールの表示が失敗する
  CHAR             mEUC[2048];
  CHAR             mSJIS[2048];
  CHAR             mJIS[2048];
  CHAR             mMime[3][2048];
#endif
#ifdef UPDATE_20171230A // Quoted-Printableのメールの検索への対応
  CHAR             mQP[1024];
#endif
#ifdef UPDATE_20230624 // SEARCH BODY命令で検索速度の向上対策
  unsigned long    nfsize;
  CHAR             *pfBuff, *pfBufp;
#endif
#ifdef UPDATE_20230624B // SEARCH 変数を重複してカウントしている
  DWORD            j;
#endif
#ifdef UPDATE_20230626A //1行にMIME-Bエンコードの区切りが複数あると、1つ目しか展開できない不具合
  char             *q, *qs, *pb, *pb2;
  CHAR             mNewLine[1024];
  CHAR             mSubLine[1024];
#endif

  mHead[0] = '\x0';
  for (i = 0; i < nToken; i++) { // 結果の初期設定
    if (stk[i].nMode != 4)  // メッセージに関する処理のみ
	  continue;
    if (!stk[i].bNOT)
      stk[i].bResult = FALSE;
    else
      stk[i].bResult = TRUE;
  }
  sprintf(mFn, "%s/%s", pContext->mSelectDir, pContext->FindData.cFileName);
#ifdef UPDATE_20230627A // UPDATE_20230624のメールデータを事前に一括して読込み検索するフラグを追加
  pfBufp = pfBuff = pContext->pBuff;
  fp = NULL;
  if (pContext->pBuff || !pContext->pBuff && (fp = fopen(mFn, "rb"))) 
  {
#else
  if ((fp = fopen(mFn, "rb"))) 
  {
#ifdef UPDATE_20230624 // SEARCH BODY命令で検索速度の向上対策
    nfsize = filelength(fileno(fp));
    if (pfBuff = malloc(nfsize+1))
	{
	  pfBufp = pfBuff;
	  fread(pfBuff, 1, nfsize, fp);
	  *(pfBuff+nfsize)='\x0';
//if (bDebug) printf("%s malloc %lu:%lu size=%lu\n", mFn, pfBuff, pfBufp, nfsize);
	} else {
	  pfBufp = NULL;
//if (bDebug) printf("malloc fail\n");
	}
#endif
#endif
#ifdef UPDATE_20230624B // SEARCH 変数を重複してカウントしている
	for (j = 0; j <= 1; j++) 
#else
	for (i = 0; i <= 1; i++) 
#endif
	{
#ifdef UPDATE_20230624 // SEARCH BODY命令で検索速度の向上対策
     if (pfBufp) {
	   p = mgets(mLine, sizeof(mLine), &pfBuff);
	 } else
	   p = fgets(mLine, sizeof(mLine), fp);
     while(p || (!pfBuff && !feof(fp)))
#else
      p = fgets(mLine, sizeof(mLine), fp);
      while(p || !feof(fp))
#endif
	  {
#ifdef UPDATE_20171230A // Quoted-Printableのメールの検索への対応
		 mQP[0] = '\x0';
#endif
		if (bHead && strcmp(mLine, "\r\n") == 0) { // ヘッダの終わり
		  bHead = FALSE;
		}
#ifdef UPDATE_20230624B // SEARCH 変数を重複してカウントしている
        if ((stk[0].nArea == 2 && !bHead) ||      // ヘッダのみの検索
#else
        if ((stk[i].nArea == 2 && !bHead) ||      // ヘッダのみの検索
#endif
			strcmp(mLine, ".\r\n") == 0) { // ボディを含む検索
	      break;
		}
#ifdef UPDATE_20110126 // １カラム目のピリオドは出力しない対策
        if (mLine[0] == '.' && !bHead)
		{
          memmove(mLine, &mLine[1], strlen(mLine));
		}
#endif
		/// 検索
		for (i = 0; i < nToken; i++) {
	      if (stk[i].nMode != 4)  // メッセージに関する処理のみ
			continue;
		  if (stk[i].nArea == 0 ||              // 全て
              stk[i].nArea == 1 && !bHead) {    // BODY
			n[0] = 0; // 文字開始位置
		    if (bHead) { // Headerの場合
#ifdef UPDATE_20230626B //1行にMIME-Bエンコードの区切りが小文字があるとループする不具合
			  if ((ps = strstr(mLine, "=?")) && ((pl = strstr(mLine, "?B?")) ||
				                                 (pl = strstr(mLine, "?b?")) ||
				                                 (pl = strstr(mLine, "?Q?")) ||
				                                 (pl = strstr(mLine, "?q?")))) { // BASE64/MIME化
#else
#ifdef UPDATE_20171230A // Quoted-Printableのメールの検索への対応
			  if ((ps = strstr(mLine, "=?")) && ((pl = strstr(mLine, "?B?")) || (pl = strstr(mLine, "?Q?")))) { // BASE64/MIME化
#else
			  if ((ps = strstr(mLine, "=?")) && (pl = strstr(mLine, "?B?"))) { // BASE64/MIME化
#endif
#endif
				if (!ps)
				  ps = mLine;
			    if (pl) {
			   	  pl += 3;
				  strcpy(mDec, pl);
				  pl = strstr(mDec, "?=");
				  if (pl)
				    *pl = '\x0';
#ifdef UPDATE_20171230A // Quoted-Printableのメールの検索への対応
			      mQP[0] = '\x0';
#ifdef UPDATE_20230626B //1行にMIME-Bエンコードの区切りが小文字があるとループする不具合
				  if (strstr(mLine, "?Q?") || strstr(mLine, "?q?") )
#else
				  if (strstr(mLine, "?Q?"))
#endif
				  {
                    QuotedPrintableDecodeLine(&mLine[n[0]], mQP);
				  } else 
#endif
				  {
                    //Base64DecodeLine(mDec, strlen(mDec), mLine, sizeof(mLine));
				    Base64DecodeLine(mDec, strlen(mDec), ps, sizeof(mLine)-(ps-mLine));
				  }
#ifdef UPDATE_20230626B //1行にMIME-Bエンコードの区切りが小文字があるとループする不具合
				  pb = mLine;
				  while(pb2 = strchr(pb, '\x1b'))
				  {
					while(*(pb2+2)) // 詰める
					{
					  *pb2 = *(pb2+2);
					  pb2++;
					}
#ifdef UPDATE_20230701 // 検索中にMIME-Bエンコードの行で区切りコードが抜けていると処理が終わらない不具合
				    *(pb2-2)='x\0';
#endif
				  }
#else
				  if (mLine[0] == '\x1b') {// ESCコードなら２バイトコード系
			        n[0] = 3;
				    if (mLine[1] == '$' && mLine[2] == ')')
				      n[0]++;
				      strtok(mLine, "\x1b");  // 末尾のESCコード以降削除
				  }
#endif
				}
			  }
			}  
#ifdef UPDATE_20171230A // Quoted-Printableのメールの検索への対応
			else {
	          mQP[0] = '\x0';
              QuotedPrintableDecodeLine(&mLine[n[0]], mQP);
			  if (!strcmp(&mLine[n[0]], mQP))
			  {
				mQP[0]='\x0';
			  }
			}
#endif
			n[1] = 0; // 文字開始位置
			if (stk[i].mToken[0] == '\x1b') {// ESCコードなら２バイトコード系
		      n[1] = 3;
			  if (stk[i].mToken[1] == '$' && stk[i].mToken[2] == ')')
				n[1]++;
			  strtok(stk[i].mToken, "\x1b");  // 末尾のESCコード以降削除
			}
#ifdef UPDATE_20230624A // SEARCH BODY命令で検索速度の向上対策:検索キーワード側を文字コード別に準備して比較する
			if (stk[i].bUnrelated) 
			{ // 大小文字関係ない検索の場合
			  _strlwr(stk[i].mToken);
			  _strlwr(mQP);
			}
//printf("stk[i].bNOT=%d\n", stk[i].bNOT);
//printf("stk[i].bResult=%d\n", stk[i].bResult);
//printf("strstr(mLine[%s],mToken[%s])\n", mLine, &stk[i].mToken[n[1]]);
//printf("strstr(mLine[%s],mTokenSJIS[%s])\n", mLine, &stk[i].mTokenSJIS[n[1]]);
//printf("strstr(mLine[%s],mTokenJIS[%s])\n", mLine, &stk[i].mTokenJIS[n[1]]);
//printf("strstr(mLine[%s],mTokenEUC[%s])\n", mLine, &stk[i].mTokenEUC[n[1]]);
//printf("strstr(mLine[%s],mTokenUTF8[%s])\n", mLine, &stk[i].mTokenUTF8[n[1]]);
//printf("&stk[i].mToken[n[1]]=%s\n", &stk[i].mToken[n[1]]);
//printf("&stk[i].mTokenSJIS[n[1]]=%s\n", &stk[i].mTokenSJIS[n[1]]);
//printf("&stk[i].mTokenJIS[n[1]]=%s\n", &stk[i].mTokenJIS[n[1]]);
//printf("&stk[i].mTokenEUC[n[1]]=%s\n", &stk[i].mTokenEUC[n[1]]);
//printf("&stk[i].mTokenUTF8[n[1]]=%s\n", &stk[i].mTokenUTF8[n[1]]);
//printf("strstr('%s', '%s'=%d\n", mQP,&stk[i].mToken[n[1]], strstr(mQP, &stk[i].mToken[n[1]]));
			if (strstr(mLine, &stk[i].mToken[n[1]]) ||
			    strstr(mLine, &stk[i].mTokenSJIS[n[1]]) ||
			    strstr(mLine, &stk[i].mTokenJIS[n[1]]) ||
			    strstr(mLine, &stk[i].mTokenEUC[n[1]]) ||
			    strstr(mLine, &stk[i].mTokenUTF8[n[1]]) ||
#ifdef UPDATE_20230626 //QuotedPrintableの比較の追加
			    strstr(mLine, &stk[i].mTokenQSJIS[n[1]]) ||
			    strstr(mLine, &stk[i].mTokenQJIS[n[1]]) ||
			    strstr(mLine, &stk[i].mTokenQEUC[n[1]]) ||
			    strstr(mLine, &stk[i].mTokenQUTF8[n[1]]) ||
#endif
			    strstr(mQP, &stk[i].mToken[n[1]]) ||
			    strstr(mQP, &stk[i].mTokenSJIS[n[1]]) ||
			    strstr(mQP, &stk[i].mTokenJIS[n[1]]) ||
			    strstr(mQP, &stk[i].mTokenEUC[n[1]]) ||
			    strstr(mQP, &stk[i].mTokenUTF8[n[1]]) ) 
			{
			  if (!stk[i].bNOT)
			    stk[i].bResult = TRUE;
			  else
			    stk[i].bResult = FALSE;
			}
//printf("stk[i].bResult=%d\n", stk[i].bResult);
#else
#ifdef UPDATE_20091129I // SEARCH CHRSETのUTF-8対応 CatchMe@Mailで添付きメールの表示が失敗する
			if (!stricmp(stk[i].mCharSet, "utf-8") ||
				!stricmp(stk[i].mCharSet, "\"utf-8\""))
			{
#ifdef UPDATE_20171230A // Quoted-Printableのメールの検索への対応
			  if (mQP[0])
			  {
			    // S-JIS to UTF-8
	            CODEPAGE2UTF8N(CP_JAPANESE, mQP, mSJIS, sizeof(mSJIS));
			    // JIS to UTF-8
			    CODEPAGE2UTF8N(CP_ISO_JAPANESE, mQP, mJIS, sizeof(mJIS));
			    // EUC to UTF-8
	            CODEPAGE2UTF8N(CP_EUC_JAPANESE, mQP, mEUC, sizeof(mEUC));
			  } else 
#endif
			  {
			    // S-JIS to UTF-8
	            CODEPAGE2UTF8N(CP_JAPANESE, &mLine[n[0]], mSJIS, sizeof(mSJIS));
			    // JIS to UTF-8
			    CODEPAGE2UTF8N(CP_ISO_JAPANESE, &mLine[n[0]], mJIS, sizeof(mJIS));
			    // EUC to UTF-8
	            CODEPAGE2UTF8N(CP_EUC_JAPANESE, &mLine[n[0]], mEUC, sizeof(mEUC));
			  }
#ifndef UPDATE_20090325A // SEARCH命令で指定文字列の大文字小文字の区別がされていた。
			  if (stk[i].bUnrelated) 
#endif
			  { // 大小文字関係ない検索の場合
			    _strlwr(mSJIS);
			    _strlwr(mJIS);
			    _strlwr(mEUC);
			    _strlwr(stk[i].mToken);
#ifdef UPDATE_20171230A // Quoted-Printableのメールの検索への対応
			    _strlwr(mQP);
#endif
			  }
#ifdef UPDATE_20171230A // Quoted-Printableのメールの検索への対応
			  if (strstr(mLine, &stk[i].mToken[n[1]]) ||
				  strstr(mQP, &stk[i].mToken[n[1]]) ||
			      strstr(mSJIS, &stk[i].mToken[n[1]]) ||
				  strstr(mJIS, &stk[i].mToken[n[1]]) ||
				  strstr(mEUC, &stk[i].mToken[n[1]])) 
#else
			  if (strstr(mSJIS, &stk[i].mToken[n[1]]) ||
				  strstr(mJIS, &stk[i].mToken[n[1]]) ||
				  strstr(mEUC, &stk[i].mToken[n[1]])) 
#endif
			  {
			    if (!stk[i].bNOT)
			      stk[i].bResult = TRUE;
			    else
			      stk[i].bResult = FALSE;
			  }
			} else
#endif
			{
#ifdef UPDATE_20091129I // SEARCH CHRSETのUTF-8対応 CatchMe@Mailで添付きメールの表示が失敗する
			  // S-JIS to JIS
	          CODEPAGE2UTF8N(CP_JAPANESE, &mLine[n[0]], mMime, sizeof(mMime));
			  UTF8N2CODEPAGE(CP_JAPANESE, mMime, mSJIS, sizeof(mSJIS));
			  // JIS to JIS
			  CODEPAGE2UTF8N(CP_ISO_JAPANESE, &mLine[n[0]], mMime, sizeof(mMime));
			  UTF8N2CODEPAGE(CP_JAPANESE, mMime, mJIS, sizeof(mJIS));
			  // EUC to JIS
	          CODEPAGE2UTF8N(CP_EUC_JAPANESE, &mLine[n[0]], mMime, sizeof(mMime));
			  UTF8N2CODEPAGE(CP_ISO_JAPANESE, mMime, mEUC, sizeof(mEUC));
#endif
#ifndef UPDATE_20090325A // SEARCH命令で指定文字列の大文字小文字の区別がされていた。
			  if (stk[i].bUnrelated) 
#endif
			  { // 大小文字関係ない検索の場合
#ifdef UPDATE_20091129I // SEARCH CHRSETのUTF-8対応 CatchMe@Mailで添付きメールの表示が失敗する
			    _strlwr(mSJIS);
			    _strlwr(mJIS);
			    _strlwr(mEUC);
#else
			    _strlwr(mLine);
#endif
			    _strlwr(stk[i].mToken);
#ifdef UPDATE_20171230A // Quoted-Printableのメールの検索への対応
			    _strlwr(mQP);
#endif
			  }
#ifdef UPDATE_20091129I // SEARCH CHRSETのUTF-8対応 CatchMe@Mailで添付きメールの表示が失敗する
#ifdef UPDATE_20171230A // Quoted-Printableのメールの検索への対応
			  if (strstr(mLine, &stk[i].mToken[n[1]]) ||
				  strstr(mQP, &stk[i].mToken[n[1]]) ||
			      strstr(mSJIS, &stk[i].mToken[n[1]]) ||
				  strstr(mJIS, &stk[i].mToken[n[1]]) ||
				  strstr(mEUC, &stk[i].mToken[n[1]])) 
#else
			  if (strstr(mSJIS, &stk[i].mToken[n[1]]) ||
				  strstr(mJIS, &stk[i].mToken[n[1]]) ||
				  strstr(mEUC, &stk[i].mToken[n[1]])) 
#endif
#else
		      if (strstr(&mLine[n[0]], &stk[i].mToken[n[1]]))
#endif
			  {
			  //if (strstr(mLine, stk[i].mToken)) {
			    if (!stk[i].bNOT)
			      stk[i].bResult = TRUE;
			    else
			      stk[i].bResult = FALSE;
			  }
			}
#endif //UPDATE_20230624A // SEARCH BODY命令で検索速度の向上対策:検索キーワード側を文字コード別に準備して比較する
          } else if ( stk[i].nArea == 2 && bHead ) {   // HEADER
		    if (!strnicmp(mLine, stk[i].mKeyword, strlen(stk[i].mKeyword)) ||
				!stricmp(mHead, stk[i].mKeyword) && (mLine[0] == ' ' || mLine[0] == '\t')) {
			  if (!(mLine[0] == ' ' || mLine[0] == '\t')) {
			    strncpy(mHead, mLine, strlen(stk[i].mKeyword));
			    mHead[strlen(stk[i].mKeyword)] = '\x0';
			  }
			  if (stricmp(stk[i].mKeyword, "DATE:")) { // Date:ヘッダ以外の場合
				n[0] = strlen(stk[i].mKeyword); // 文字開始位置
#ifdef UPDATE_20230626A //1行にMIME-Bエンコードの区切りが複数あると、1つ目しか展開できない不具合
				mNewLine[0] = mSubLine[0] = '\x0';
			    q = mLine;
				if (!strstr(q, "=?")) {
				  strcpy(mNewLine, q);
				}
				while(qs=strstr(q, "=?"))
				{ 
				  *qs='\x0';
				  strcat(mNewLine, q);
				  *qs = '=';
#ifdef UPDATE_20230626B //1行にMIME-Bエンコードの区切りが小文字があるとループする不具合
			      if ((ps = strstr(qs, "=?")) && ((pl = strstr(qs, "?B?")) ||
					                              (pl = strstr(qs, "?b?")) ||
												  (pl = strstr(qs, "?Q?")) ||
												  (pl = strstr(qs, "?q?")))) 
#else
			      if ((ps = strstr(qs, "=?")) && ((pl = strstr(qs, "?B?")) || (pl = strstr(qs, "?Q?")))) 
#endif
				  { // BASE64/MIME化
				    if (!ps)
				      ps = qs;
				    if (pl) {
					  pl += 3;
					  strcpy(mDec, pl);
					  pl = strstr(mDec, "?=");
					  if (pl)
					    *pl = '\x0';
#ifdef UPDATE_20230628 // MIME-Bエンコードの区切りコードが含まれないとハングする
					  else
						pl = &mDec[strlen(mDec)-2];
#endif
					  q = pl+2;
			          mQP[0] = '\x0';
#ifdef UPDATE_20230626B //1行にMIME-Bエンコードの区切りが小文字があるとループする不具合
				      if (strstr(qs, "?Q?") || strstr(qs, "?q?"))
#else
				      if (strstr(qs, "?Q?"))
#endif
					  {
                        QuotedPrintableDecodeLine(mDec, mSubLine);
					  } else 
					  {
                        Base64DecodeLine(mDec, strlen(mDec), mSubLine, sizeof(mSubLine));
					  }
#ifdef UPDATE_20230626B //1行にMIME-Bエンコードの区切りが小文字があるとループする不具合
					  pb = mSubLine;
					  while(pb2 = strchr(pb, '\x1b'))
					  {
						while(*(pb2+2)) // 詰める
						{
						  *pb2 = *(pb2+2);
						  pb2++;
						}
#ifdef UPDATE_20230701 // 検索中にMIME-Bエンコードの行で区切りコードが抜けていると処理が終わらない不具合
						*(pb2-2)='x\0';
#endif
					  }
#else
				      if (mSubLine[0] == '\x1b') {// ESCコードなら２バイトコード系
			            n[0] = 3;
				        if (mSubLine[1] == '$' && mSubLine[2] == ')')
					      n[0]++;
				          strtok(mSubLine, "\x1b");  // 末尾のESCコード以降削除
					  }
#endif
					}
				  } else {
#ifdef UPDATE_20230701 // 検索中にMIME-Bエンコードの行で区切りコードが抜けていると処理が終わらない不具合
					q = qs+2;
					strcat(mNewLine, "=?");
#else
					strcpy(mNewLine, q);
#endif
				  }
#ifdef UPDATE_20230626B //1行にMIME-Bエンコードの区切りが小文字があるとループする不具合
				  strcat(mNewLine, mSubLine);
#else
				  strcat(mNewLine, (mSubLine[0] == '\x1b' ? &mSubLine[n[0]] : mSubLine) );
#endif
				  if (!strstr(q, "=?"))
				    strcat(mNewLine, q);
				} 
				strcpy(mLine, mNewLine);
#else
#ifdef UPDATE_20171230A // Quoted-Printableのメールの検索への対応
			    if ((ps = strstr(mLine, "=?")) && ((pl = strstr(mLine, "?B?")) || (pl = strstr(mLine, "?Q?")))) { // BASE64/MIME化
#else
				if ((ps = strstr(mLine, "=?")) && (pl = strstr(mLine, "?B?"))) { // BASE64/MIME化
#endif
				  if (!ps)
				    ps = mLine;
				  if (pl) {
					pl += 3;
					strcpy(mDec, pl);
					pl = strstr(mDec, "?=");
					if (pl)
					  *pl = '\x0';
#ifdef UPDATE_20171230A // Quoted-Printableのメールの検索への対応
			        mQP[0] = '\x0';
				    if (strstr(mLine, "?Q?"))
					{
                      QuotedPrintableDecodeLine(&mLine[n[0]], mQP);
					} else 
#endif
					{
                      //Base64DecodeLine(mDec, strlen(mDec), mLine, sizeof(mLine));
				      Base64DecodeLine(mDec, strlen(mDec), ps, sizeof(mLine)-(ps-mLine));
					}
				    if (mLine[0] == '\x1b') {// ESCコードなら２バイトコード系
			          n[0] = 3;
				      if (mLine[1] == '$' && mLine[2] == ')')
					    n[0]++;
				        strtok(mLine, "\x1b");  // 末尾のESCコード以降削除
					}
#ifdef UPDATE_20090326 // デコードの為に区切った残りの文字列を復帰
					if (pl)
					{
					  strcat(mLine, pl+1);
					}
#endif
				  }
				}
#endif //1行にMIME-Bエンコードの区切りが複数あると、1つ目しか展開できない不具合
				n[1] = 0; // 文字開始位置
				if (stk[i].mToken[0] == '\x1b') {// ESCコードなら２バイトコード系
			      n[1] = 3;
				  if (stk[i].mToken[1] == '$' && stk[i].mToken[2] == ')')
					n[1]++;
				  strtok(stk[i].mToken, "\x1b");  // 末尾のESCコード以降削除
				}

#ifdef UPDATE_20230624A // SEARCH BODY命令で検索速度の向上対策:検索キーワード側を文字コード別に準備して比較する
			    if (stk[i].bUnrelated) 
				{ // 大小文字関係ない検索の場合
			      _strlwr(stk[i].mToken);
			      _strlwr(mQP);
				}
//printf("stk[i].bNOT=%d\n", stk[i].bNOT);
//printf("stk[i].bResult=%d\n", stk[i].bResult);
//printf("strstr(mLine[%s],mToken[%s])\n", mLine, &stk[i].mToken[n[1]]);
//printf("strstr(mLine[%s],mTokenSJIS[%s])\n", mLine, &stk[i].mTokenSJIS[n[1]]);
//printf("strstr(mLine[%s],mTokenJIS[%s])\n", mLine, &stk[i].mTokenJIS[n[1]]);
//printf("strstr(mLine[%s],mTokenEUC[%s])\n", mLine, &stk[i].mTokenEUC[n[1]]);
//printf("strstr(mLine[%s],mTokenUTF8[%s])\n", mLine, &stk[i].mTokenUTF8[n[1]]);
//printf("&stk[i].mToken[n[1]]=%s\n", &stk[i].mToken[n[1]]);
//printf("&stk[i].mTokenSJIS[n[1]]=%s\n", &stk[i].mTokenSJIS[n[1]]);
//printf("&stk[i].mTokenJIS[n[1]]=%s\n", &stk[i].mTokenJIS[n[1]]);
//printf("&stk[i].mTokenEUC[n[1]]=%s\n", &stk[i].mTokenEUC[n[1]]);
//printf("&stk[i].mTokenUTF8[n[1]]=%s\n", &stk[i].mTokenUTF8[n[1]]);
/*
printf("strstr('%s', '%s')=%d\n", mLine,&stk[i].mToken[n[1]], strstr(mLine, &stk[i].mToken[n[1]]));
printf("strstr('%s', '%s')=%d\n", mLine,&stk[i].mTokenSJIS[n[1]], strstr(mLine, &stk[i].mTokenSJIS[n[1]]));
printf("strstr('%s', '%s')=%d\n", mLine,&stk[i].mTokenJIS[n[1]], strstr(mLine, &stk[i].mTokenJIS[n[1]]));
printf("strstr('%s', '%s')=%d\n", mLine,&stk[i].mTokenEUC[n[1]], strstr(mLine, &stk[i].mTokenEUC[n[1]]));
printf("strstr('%s', '%s')=%d\n", mLine,&stk[i].mTokenUTF8[n[1]], strstr(mLine, &stk[i].mTokenUTF8[n[1]]));
printf("strstr('%s', '%s')=%d\n", mLine,&stk[i].mTokenQSJIS[n[1]], strstr(mLine, &stk[i].mTokenQSJIS[n[1]]));
printf("strstr('%s', '%s')=%d\n", mLine,&stk[i].mTokenQJIS[n[1]], strstr(mLine, &stk[i].mTokenQJIS[n[1]]));
printf("strstr('%s', '%s')=%d\n", mLine,&stk[i].mTokenQEUC[n[1]], strstr(mLine, &stk[i].mTokenQEUC[n[1]]));
printf("strstr('%s', '%s')=%d\n", mLine,&stk[i].mTokenQUTF8[n[1]], strstr(mLine, &stk[i].mTokenQUTF8[n[1]]));
printf("strstr('%s', '%s')=%d\n", mQP,&stk[i].mToken[n[1]], strstr(mQP, &stk[i].mToken[n[1]]));
printf("strstr('%s', '%s')=%d\n", mQP,&stk[i].mTokenSJIS[n[1]], strstr(mQP, &stk[i].mTokenSJIS[n[1]]));
printf("strstr('%s', '%s')=%d\n", mQP,&stk[i].mTokenJIS[n[1]], strstr(mQP, &stk[i].mTokenJIS[n[1]]));
printf("strstr('%s', '%s')=%d\n", mQP,&stk[i].mTokenEUC[n[1]], strstr(mQP, &stk[i].mTokenEUC[n[1]]));
printf("strstr('%s', '%s')=%d\n", mQP,&stk[i].mTokenUTF8[n[1]], strstr(mQP, &stk[i].mTokenUTF8[n[1]]));
*/
			    if (strstr(mLine, &stk[i].mToken[n[1]]) ||
			        strstr(mLine, &stk[i].mTokenSJIS[n[1]]) ||
			        strstr(mLine, &stk[i].mTokenJIS[n[1]]) ||
			        strstr(mLine, &stk[i].mTokenEUC[n[1]]) ||
			        strstr(mLine, &stk[i].mTokenUTF8[n[1]]) ||
#ifdef UPDATE_20230626 //QuotedPrintableの比較の追加
			        strstr(mLine, &stk[i].mTokenQSJIS[n[1]]) ||
			        strstr(mLine, &stk[i].mTokenQJIS[n[1]]) ||
			        strstr(mLine, &stk[i].mTokenQEUC[n[1]]) ||
			        strstr(mLine, &stk[i].mTokenQUTF8[n[1]]) ||
#endif
			        strstr(mQP, &stk[i].mToken[n[1]]) ||
			        strstr(mQP, &stk[i].mTokenSJIS[n[1]]) ||
			        strstr(mQP, &stk[i].mTokenJIS[n[1]]) ||
			        strstr(mQP, &stk[i].mTokenEUC[n[1]]) ||
			        strstr(mQP, &stk[i].mTokenUTF8[n[1]]) ) 
			    {
			      if (!stk[i].bNOT)
			        stk[i].bResult = TRUE;
			      else
			        stk[i].bResult = FALSE;
			    }
//printf("stk[i].bResult=%d\n", stk[i].bResult);
#else

#ifdef UPDATE_20091129I // SEARCH CHRSETのUTF-8対応 CatchMe@Mailで添付きメールの表示が失敗する
				if (!stricmp(stk[i].mCharSet, "utf-8") ||
					!stricmp(stk[i].mCharSet, "\"utf-8\""))
				{
#ifdef UPDATE_20171230A // Quoted-Printableのメールの検索への対応
				  if (mQP[0])
				  {
			        // S-JIS to UTF-8
	                CODEPAGE2UTF8N(CP_JAPANESE, mQP, mSJIS, sizeof(mSJIS));
			        // JIS to UTF-8
			        CODEPAGE2UTF8N(CP_ISO_JAPANESE, mQP, mJIS, sizeof(mJIS));
			        // EUC to UTF-8
	                CODEPAGE2UTF8N(CP_EUC_JAPANESE, mQP, mEUC, sizeof(mEUC));
				  } else 
#endif
				  {
			        // S-JIS to UTF-8
	                CODEPAGE2UTF8N(CP_JAPANESE, &mLine[n[0]], mSJIS, sizeof(mSJIS));
			        // JIS to UTF-8
			        CODEPAGE2UTF8N(CP_ISO_JAPANESE, &mLine[n[0]], mJIS, sizeof(mJIS));
			        // EUC to UTF-8
	                CODEPAGE2UTF8N(CP_EUC_JAPANESE, &mLine[n[0]], mEUC, sizeof(mEUC));
				  }
#ifndef UPDATE_20090325A // SEARCH命令で指定文字列の大文字小文字の区別がされていた。
			      if (stk[i].bUnrelated) 
#endif
				  { // 大小文字関係ない検索の場合
			        _strlwr(mSJIS);
			        _strlwr(mJIS);
			        _strlwr(mEUC);
			        _strlwr(stk[i].mToken);
#ifdef UPDATE_20171230A // Quoted-Printableのメールの検索への対応
			        _strlwr(mQP);
#endif
				  }
#ifdef UPDATE_20171230A // Quoted-Printableのメールの検索への対応
                  if (strstr(mLine, &stk[i].mToken[n[1]]) ||
			          strstr(mQP, &stk[i].mToken[n[1]]) ||
			          strstr(mSJIS, &stk[i].mToken[n[1]]) ||
				      strstr(mJIS, &stk[i].mToken[n[1]]) ||
				      strstr(mEUC, &stk[i].mToken[n[1]])) 
#else
			      if (strstr(mSJIS, &stk[i].mToken[n[1]]) ||
				      strstr(mJIS, &stk[i].mToken[n[1]]) ||
				      strstr(mEUC, &stk[i].mToken[n[1]])) 
#endif
				  {
			        if (!stk[i].bNOT)
			          stk[i].bResult = TRUE;
			        else
			          stk[i].bResult = FALSE;
				  }
				} else
#endif
				{
#ifdef UPDATE_20091129I // SEARCH CHRSETのUTF-8対応 CatchMe@Mailで添付きメールの表示が失敗する
			      // S-JIS to JIS
	              CODEPAGE2UTF8N(CP_JAPANESE, &mLine[n[0]], mMime, sizeof(mMime));
			      UTF8N2CODEPAGE(CP_JAPANESE, mMime, mSJIS, sizeof(mSJIS));
			      // JIS to JIS
			      CODEPAGE2UTF8N(CP_ISO_JAPANESE, &mLine[n[0]], mMime, sizeof(mMime));
			      UTF8N2CODEPAGE(CP_JAPANESE, mMime, mJIS, sizeof(mJIS));
			      // EUC to JIS
	              CODEPAGE2UTF8N(CP_EUC_JAPANESE, &mLine[n[0]], mMime, sizeof(mMime));
			      UTF8N2CODEPAGE(CP_ISO_JAPANESE, mMime, mEUC, sizeof(mEUC));
#endif
#ifndef UPDATE_20090325A // SEARCH命令で指定文字列の大文字小文字の区別がされていた。
				  if (stk[i].bUnrelated) 
#endif
				  { // 大小文字関係ない検索の場合
#ifdef UPDATE_20091129I // SEARCH CHRSETのUTF-8対応 CatchMe@Mailで添付きメールの表示が失敗する
			        _strlwr(mSJIS);
			        _strlwr(mJIS);
			        _strlwr(mEUC);
#else
				    _strlwr(mLine);
#endif
				    _strlwr(stk[i].mToken);
#ifdef UPDATE_20171230A // Quoted-Printableのメールの検索への対応
			        _strlwr(mQP);
#endif
				  }
#ifdef UPDATE_20091129I // SEARCH CHRSETのUTF-8対応 CatchMe@Mailで添付きメールの表示が失敗する
#ifdef UPDATE_20171230A // Quoted-Printableのメールの検索への対応
                  if (strstr(mLine, &stk[i].mToken[n[1]]) ||
			          strstr(mQP, &stk[i].mToken[n[1]]) ||
			          strstr(mSJIS, &stk[i].mToken[n[1]]) ||
				      strstr(mJIS, &stk[i].mToken[n[1]]) ||
				      strstr(mEUC, &stk[i].mToken[n[1]])) 
#else
			      if (strstr(mSJIS, &stk[i].mToken[n[1]]) ||
				      strstr(mJIS, &stk[i].mToken[n[1]]) ||
				      strstr(mEUC, &stk[i].mToken[n[1]])) 
#endif
#else
			      if (strstr(&mLine[n[0]], &stk[i].mToken[n[1]]))
#endif
				  {
			        if (!stk[i].bNOT)
			          stk[i].bResult = TRUE;
			        else
			          stk[i].bResult = FALSE;
//printf("o %d:[%s]\n", stk[i].bResult, &mLine[n[0]]);
//				  } else {
//printf("x %d:[%s]\n", stk[i].bResult, &mLine[n[0]]);
				  }
				}
#endif// UPDATE_20230624A // SEARCH BODY命令で検索速度の向上対策:検索キーワード側を文字コード別に準備して比較する
			  } else {  // Date:ヘッダの場合
				mHead[0] = '\x0';
			    Serach_Date(stk[i].mToken,&nDate[0]);    // 条件
				nDate[1] = 0;
				if ((pDate = strpbrk((mLine[9] == ',' ? &mLine[6] : mLine), " "))) {
				  Serach_Date(++pDate,&nDate[1]);        // データ
	              switch (stk[i].nDType) {
	                case 0: if (nDate[0] > nDate[1]) // 以前
			                  if (!stk[i].bNOT)
			                    stk[i].bResult = TRUE;
			                  else
			                    stk[i].bResult = FALSE;
		                    break;
	                case 1: if (nDate[0] == nDate[1]) // 当日
			                  if (!stk[i].bNOT)
			                    stk[i].bResult = TRUE;
			                  else
			                    stk[i].bResult = FALSE;
		                    break;
	                case 2: if (nDate[0] <= nDate[1]) // 日中以降
			                  if (!stk[i].bNOT)
			                    stk[i].bResult = TRUE;
			                  else
			                    stk[i].bResult = FALSE;
		                    break;
				  }
				}
			  }
			  ////////////////////////////////////////////
			} else {
			  mHead[0] = '\x0';
			}
		  }
		}
#ifdef UPDATE_20230624 // SEARCH BODY命令で検索速度の向上対策
		if (pfBufp){
		  p = mgets(mLine, sizeof(mLine), &pfBuff);
		} else
	      p = fgets(mLine, sizeof(mLine), fp);
#else
        p = fgets(mLine, sizeof(mLine), fp);
#endif
	  }
	}
#ifdef UPDATE_20230627A // UPDATE_20230624のメールデータを事前に一括して読込み検索するフラグを追加
    if (!pContext->pBuff && fp)
	  fclose(fp);
#else
#ifdef UPDATE_20230624 // SEARCH BODY命令で検索速度の向上対策
	if (pfBufp)
	{
//if (bDebug) printf("free %lu\n", pfBufp);
      free(pfBufp);
	}
#endif
	fclose(fp);
#endif
  }
}

#ifdef UPDATE_20171230A // Quoted-Printableのメールの検索への対応
void QuotedPrintableDecodeLine(unsigned char * pSrc, unsigned char * pDest) 
{
	unsigned char *o;
	unsigned char *p = pSrc;
	unsigned char *q = pDest;
	short    n[3];
	BOOL     bMach = FALSE;
	
	*pDest = '\x0';
	if (!p)
	  return;
	while (*p) {
	  //if (!strcmp((const char *)p, "?="))  // =? 終了
	    //break;
	  if (*p == '&' && *(p+1) == '#' && *(p+4) == ';' && strlen((const char *)p) > 4) { // &#??; のパターン
		n[0] = *(p+2) - 0x30;
		n[1] = *(p+3) - 0x30;
        p+=5;
        *q = (char)(n[0] * 10 + n[1]);
	  } else if (*p == '&' && *(p+1) == '#' && *(p+5) == ';' && strlen((const char *)p) > 5) { // &#???; のパターン
		n[0] = *(p+2) - 0x30;
		n[1] = *(p+3) - 0x30;
		n[2] = *(p+4) - 0x30;
        p+=6;
        *q = (char)(n[0] * 100 + n[1] * 10 + n[2]);
	  } else if (*p == '=') { // 8Bit
		p++;
		n[0] = n[1] = 0;
		if (*p >= 0x30 && *p <= 0x39) {
		  n[0] = *p - 0x30;
		  bMach = TRUE;
		} else if (*p >= 0x41 && *p <= 0x46) {
		  n[0] = *p - 0x37;
		  bMach = TRUE;
		} else if (*p >= 0x61 && *p <= 0x66) {
		  n[0] = *p - 0x57;
		  bMach = TRUE;
		} else {
		  p--;
		  bMach = FALSE;
		}
		if (bMach) {
          p++;
		  if (*p >= 0x30 && *p <= 0x39) {
		    n[1] = *p - 0x30;
		  } else if (*p >= 0x41 && *p <= 0x46) {
		    n[1] = *p - 0x37;
		  } else if (*p >= 0x61 && *p <= 0x66) {
		    n[1] = *p - 0x57;
		  } else {
            p -= 2;
		    bMach = FALSE;
            *q = *p;
	        p++;
		  }
		  if (bMach) {
            p++;
		    *q = (char)(n[0] * 16 + n[1]);
		  }
		} else {
          *q = *p;
	      p++;
		}
	  } else {         // ASCII
		if (*p == '_')
          *q = ' ';
		else
          *q = *p;
	    p++;
	  }
	  q++;
	}
	*q = '\x0';
    // Quoted-Printableのデコードで文字が欠ける場合がある
	if (strlen(pDest) >= 2)
	{
      if ((o = strstr((char *)(pDest+strlen(pDest)-2), "?="))) { // パックの終わりを除去
        *o = '\x0';
	  }
	} 
}
#endif

#ifdef UPDATE_20230626 //QuotedPrintableの比較の追加
void QuotedPrintableEncodeLine(const char *pSrc, char *pDest)
{
  int n;
  char m[4];
  char *s = pSrc;

  *pDest ='\x0';
  for (n = 0; *s; s++) {
	if (n >= 73 && *s != 10 && *s != 13) 
	{
	  strcpy(m,"=\r\n");
	  strcat(pDest, m); 
	  n = 0;
	}

    if (*s == 10 || *s == 13) {
	  m[0] = *s; m[1] = '\x0';
	  strcat(pDest, m); 
	  n = 0;
	}
    else if (*s<32 || *s==61 || *s>126) 
	{
	  n += sprintf(m,"=%02X", (unsigned char)*s);
	  strcat(pDest, m); 
	}
    else if (*s != 32 || (*(s+1) != 10 && *(s+1) != 13)) 
	{
	  m[0] = *s; m[1] = '\x0';
	  strcat(pDest, m); 
	  n++;
	}
    else 
	{
	  n += 3;
	  strcat(pDest, "=20"); 
	}
  }
}
#endif