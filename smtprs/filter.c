////////////////////////////////////////////////////////////
// Virus Filter.c Copyright K.kawakami  //update 2003.10.20 
////////////////////////////////////////////////////////////

#include "smtp.h"

extern char mProgPath[];
extern CHAR mMailBoxDir[];
extern char mMailSpoolDir[];
extern char mPASSFilterFile[];
extern BOOL bDebug;
extern DWORD nFROMSecLevel;
extern BOOL bServiceTerminating;
extern DWORD nMailFiterUp; // メールフィルタ機能強化 0:標準 1:強化
#ifdef UPDATE_20070407 // リンク先のドメイン名の調査する深さを調整可能に
extern DWORD nURIRBLDepath; // URIBLのドメインの深さ制限　デフォルト 2;
#endif
#ifdef UPDATE_20070516 // 上長承認機能の追加
extern BOOL   bMailApproval;
#endif
#ifdef UPDATE_20161111 /// 外部からの送信元送信先制限(逆上長承認)
extern BOOL   bMailApprovalRevers;
#endif

typedef struct VIRUS_TABLE {
 char mVirus[80];
 char mReceived[256];
 char mSubject[256];
 char mFrom[256];
 char mTo[256];
 char mContentType[256];
 char mContentTransferEncoding[256];
 char mXMailer[256];
 char mXUIDL[256];
 char mBody[5][80];
 char mFolder[256];
 BOOL bReceived[2];
 BOOL bSubject[2];
 BOOL bFrom[2];
 BOOL bTo[2];
 BOOL bContentType[2];
 BOOL bContentTransferEncoding[2];
 BOOL bXMailer[2];
 BOOL bXUIDL[2];
 BOOL bBody[2];
 int  nBody[2]; // 追加
 int  nLevel;
 char mWarning[5][80];
#ifdef ADDED_FILTER_TAG // 一致したメールのSubject:にタグを追加する [SPAM !!]
 char mTag[80];
#endif
#ifdef ADDED_FILTER_RBL   // 本文内のURLを指定したRBLへ問い合わせるオプションを追加する 例 [SPAM !!]
 BOOL bRBL[2];
 char mRBL[256];
#endif
#ifdef ADDED_FILTER_FORWARD // 一致したメールを転送する。
 char mForward[280];
#endif
#ifdef ADDED_FILTER_UNIQUE   // 予約済み以外のヘッダのチェックオプションを追加する
 BOOL bUNIQUE[2];
 char mUNIQUEHEADER[280];
 char mUNIQUE[280];
#endif
#ifdef ADDED_WILDCARD_SEARCH // 大小文字＆ワイルドカード指定でトークン比較する機能 0:無効 1:有効
 int  nComp2[2];
#endif
} ;

extern BOOL    bVirus;
extern char    mVirusFile[];
extern char    mMailFile[];
extern DWORD   nPatternOfTheOnce;
#ifdef SHORT_SURBL // 短縮URLのSURBL判定対策
extern CHAR    mShortDecodeFile[];
#endif

BOOL CheckB64Line(char *p);
#ifdef ACCEPT_LOG_LEVEL3
void LEVEL_3_ACCEPTLOG(PCLIENT_CONTEXT lpClientContext, char *mess);
#endif
DWORD VirusCheck(PSmtpContext pContext, struct VIRUS_TABLE *Virus_Table, DWORD nVirus);
void  GetBaseDirectory(CHAR *BaseDir, char *mMBdir, char *muser,char *myaddr);
BOOL  CheckDomain(char *mID);
void  SPA_Decode(char *pSrc, char *pDest);
#ifdef UPDATE_20050107
void  LineConv(char *dest, INT ndest, char *src, char *pack);
#else
void  LineConv(char *dest, char *src, char *pack);
#endif
#ifdef UPDATE_20151111 // URIBL問合せのホワイトリストテーブルを追加
BOOL  wildcard_stricmp(const char *string1, const char *string2);
#endif

#ifdef ADDED_FILTER_RBL   // 本文内のURLを指定したRBLへ問い合わせるオプションを追加する 例 [SPAM !!]
int GetQueryHostName(char *pAddr, char *pHostName);

#ifdef UPDATE_20070205 // URIの検出時に"www=2exxx=2exxx"を"www.xxx.xxx"に変換する処理
//////////////////////////////////////////////////////////////
// URIのデータ正規化処理の追加
//////////////////////////////////////////////////////////////

void URI_Encode(char *pSrc, int nSize) {
  int  i = 0;
  char c, *p1, *p2, *pend;

  p1 = p2 = pSrc;
  while(*p1 && i < nSize) {
    if (!strnicmp(p1, "=2e", 3)) {
      *p2 = '.'; // ピリオドに置き換え
  	  p1 += 3; // "=2e"をスキップ
      i += 3;
	} else if (*p1 == '%') { // 16進文字列をASCIIに変換
 	  c = (char)strtoul( p1+1, &pend, 16);
	  if (c >= 0x20 && c < 0x7f) {
	    *p2 = c;
  	    p1 += 3; // "%XX"をスキップ
	    i += 3;
	  } else {
		p1++; // 英数字以外ならスキップ
	    i++;
	  }
	//} else if (*p1 == '*' || *p1 == '!') { // 16進文字列をASCIIに変換
	//  p1++; // 英数字以外ならスキップ
	//  i++;
	} else {
      *p2 = *p1;
	  p1++;
      i++;
	}
    p2++;
  }
  *p2 = 0;
}
#endif

#ifdef UPDATE_20151111 // URIBL問合せのホワイトリストテーブルを追加
BOOL White_URIBL(char *pRBL, char *pFnRBL)
{
   BOOL bSts = FALSE;
   FILE * fp;
   char mLine[1024];

   if (*pFnRBL)
   {
     if (fp = fopen(pFnRBL, "rb"))
	 {
	   while(fgets(mLine, sizeof(mLine)-1, fp))
	   {
	     strtok(mLine, "\r\n");
	     if (wildcard_stricmp(mLine, pRBL))
		 {
		   bSts = TRUE;
		   break;
		 }
	   }
	   fclose(fp);
	 }
   }
   return bSts;
}
#endif

#ifdef UPDATE_20141023  // URIBL問合せの効率化
#ifdef UPDATE_20161014 // メールフィルタ条件でRBLへの重複問合せがキャッシュ設定により失敗する
BOOL OverlapIgnore_URIBL(char *pRBL, char *pFnRBL, BOOL *pbsts, BOOL bWrite)
#else
BOOL OverlapIgnore_URIBL(char *pRBL, char *pFnRBL)
#endif
{
   BOOL bSts = FALSE;
   FILE *fp;
   char mLine[1024];
#ifdef UPDATE_20161014 // メールフィルタ条件でRBLへの重複問合せがキャッシュ設定により失敗する
   char *p;
#endif

   if (fp = fopen(pFnRBL, "rb"))
   {
	  while(fgets(mLine, sizeof(mLine)-1, fp))
	  {
#ifdef UPDATE_20161014 // メールフィルタ条件でRBLへの重複問合せがキャッシュ設定により失敗する
         if ((p = strrchr(mLine, '\t')))
		 {
		   *p = '\x0';
		 }
#endif
		 strtok(mLine, "\r\n");
		 if (!strcmp(mLine, pRBL))
		 {
#ifdef UPDATE_20161014 // メールフィルタ条件でRBLへの重複問合せがキャッシュ設定により失敗する
		   *pbsts = FALSE;
		   if (*(p+1)== '1')
		   {
			 *pbsts = TRUE;
		   }
#endif
		   bSts = TRUE;
		   break;
		 }
	  }
	  fclose(fp);
   }
#ifdef UPDATE_20161014 // メールフィルタ条件でRBLへの重複問合せがキャッシュ設定により失敗する
   if (bWrite)
#endif
   {
     if (!bSts) // 一致するURIBLがないなら保存
	 {
       if (fp = fopen(pFnRBL, "ab"))
	   {
#ifdef UPDATE_20161014 // メールフィルタ条件でRBLへの重複問合せがキャッシュ設定により失敗する
		 fprintf(fp, "%s\t%c\n", pRBL, (*pbsts ? '1' : '0'));
#else
	     fprintf(fp, "%s\n", pRBL);
#endif
	     fclose(fp);
	   }
	 }
   }
   return bSts;
}
#endif

#ifdef UPDATE_20141023  // URIBL問合せの効率化
BOOL Query_FILTER_RBL(char *pRBL, char *pData, char *pFnRBL)
#else
BOOL Query_FILTER_RBL(char *pRBL, char *pData)
#endif
{
	char mLog[512];
	BOOL sts = FALSE; // ブラックリストに載っていない
#ifdef UPDATE_20241022 // メールフィルタによるRBL問合せリンク長が長すぎるとハングする不具合
	char mHostName[512], mURL[256], mIP[256];
#else
	char mHostName[128], mURL[256], mIP[256];
#endif
	HOSTENT *phost;
	char    *phttp, *pno, *pSite, *pAscii;
	int     nIP, i;
#ifdef IPv6
	int    nErr = 0;
	CHAR   mPaddr[INET6_ADDRSTRLEN]; // Peer Host Address IPv6
#else
	CHAR   mPaddr[21];   // Peer Host Address xxx.xxx.xxx.xxx
#endif
#ifndef VC6
    HOSTENT hst;
    struct addrinfo Hints, *AddrInfo;
#endif
	BOOL   bAlpha = FALSE;
	int    nNUM = 0;
#ifdef UPDATE_20070205 // URIのデータ正規化処理の追加
	char   *phttp2;
#endif
#ifdef DNS_QUERY
    int  nDepath = nURIRBLDepath;
	CHAR *pDNS = NULL, *pDepath = NULL;
	CHAR mRBL[256];
#ifdef UPDATE_20080703 // メールフィルタのRBL参照DNS指定で、キャッシュ有効無効のオプションを追加
	BOOL bQuery = TRUE;
	CHAR *pQuery;
#endif
#endif
#ifdef SHORT_SURBL // 短縮URLのSURBL判定対策
	CHAR *pAscii2, mDecURL[256];
#endif
#ifdef UPDATE_20151111 // URIBL問合せのホワイトリストテーブルを追加
    CHAR mWRBL[256];
#endif
/*	
#ifdef ACCEPT_LOG_LEVEL3
	sprintf(mLog, "Start GetQueryHostName()");
    LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
*/
#ifdef DNS_QUERY
	strcpy(mRBL, pRBL);
#ifdef UPDATE_20151111 // URIBL問合せのホワイトリストテーブルを追加
	mWRBL[0] = '\x0';
    if (pDepath = strchr(mRBL, '"'))
	{
	  *pDepath = '\x0';
      strcpy(mWRBL, pDepath+1);
	  strtok(mWRBL, "\"");
	}
#endif
    if ((pDepath = strrchr(mRBL, ','))) // フィルターテーブル毎に変更可能に
	{
	  *pDepath = '\x0';
	  nDepath = atoi(pDepath+1);
#ifdef UPDATE_20080703 // メールフィルタのRBL参照DNS指定で、キャッシュ有効無効のオプションを追加
	  pQuery = pDepath + 1;
      if (*pQuery == 'N' || *pQuery == 'n')
	  {
	    bQuery = FALSE;  // DNSキャシュ無効
	  } else {
	    bQuery = TRUE;  // DNSキャシュ有効
	  }
#endif
	}
#endif
#ifdef UPDATE_20070205 // URIのデータ正規化処理の追加
    URI_Encode(pData, strlen(pData));
#endif

	phttp = strstr(pData, "://"); // URL
    if (!phttp) // URLが見つからない
	{
#ifdef UPDATE_20070910 // URIの追加判定条件
      phttp = strstr(pData, "www"); // URL
      if (!phttp) // URLが見つからない
	  {
        phttp = strstr(pData, "WWW"); // URL
        if (!phttp) // URLが見つからない
		{
          phttp = strstr(pData, "@"); // URL
          if (!phttp) // URLが見つからない
		  {
            return sts;
		  } else {
			if (!strstr(phttp, ".")) // ピリオドない場所はNG
              return sts;
		  }
		} else {
		  if (!strstr(phttp, ".")) // ピリオドない場所はNG
            return sts;
		}
	  } else {
	    if (!strstr(phttp, ".")) // ピリオドない場所はNG
          return sts;
	  }
#else
      return sts;
#endif
	} 
#ifdef UPDATE_20070910 // URIの追加判定条件
	//if (*phttp == 'w' || *phttp == 'W') { // 不要
	if (*phttp == ':') {
      phttp += 3;
	} else if (*phttp == '@') {
      phttp += 1;
	}
#else
    if (strnicmp(phttp-4, "http", 4) != 0)
      return sts;
    phttp += 3;
#endif
#ifdef UPDATE_20061002 // ドメイン名の抽出が出来ないURIBL処理の対策
	while(*phttp== '=') {
	  phttp++;
	}
#endif
#ifdef UPDATE_20070910 // URIの追加判定条件
	if ((phttp2 = strstr(phttp,"://"))) {
	  phttp = phttp2 + 3;
	} else if ((phttp2 = strstr(phttp,"www"))) {
	  phttp = phttp2;
	} else if ((phttp2 = strstr(phttp,"@"))) {
  	  phttp = phttp2 + 1;
	}
#else
#ifdef UPDATE_20070205 // URIのデータ正規化処理の追加
    if ((phttp2 = strstr(phttp, "://"))) { // URL
	  if (strnicmp(phttp2-4, "http", 4) == 0) {
		phttp = phttp2 + 3;
	  }
	}
#endif
#endif
	strncpy(mURL, phttp, sizeof(mURL));

	mURL[255] = '\x0';
#ifdef SHORT_SURBL // 短縮URLのSURBL判定対策
	mDecURL[0] = '\x0';
#endif

#ifdef UPDATE_20070910 // URIの追加判定条件
	///////////////////////////////////////////
	// ドメイン名の抽出 '-','a-z',"A-Z'のみ有効
	pAscii = mURL;
    while(*pAscii) {
	  if (!(*pAscii == '-' ||
		    *pAscii == '.' ||
		   (*pAscii >= '0' && *pAscii <= '9') ||
		   (*pAscii >= 'a' && *pAscii <= 'z') ||
		   (*pAscii >= 'A' && *pAscii <= 'Z')) ) {
#ifdef SHORT_SURBL // 短縮URLのSURBL判定対策
		pAscii2 = pAscii;
		if (*pAscii)
		{
		  pAscii2++;
		} 
        while(*pAscii2)
		{
	      if (!(*pAscii2 == '-' ||
		        *pAscii2 == '.' ||
		       (*pAscii2 >= '0' && *pAscii2 <= '9') ||
		       (*pAscii2 >= 'a' && *pAscii2 <= 'z') ||
		       (*pAscii2 >= 'A' && *pAscii2 <= 'Z')) ) 
		  {
		    *pAscii2 = '\x0';
		  }
		  pAscii2++;
		}
        strtok(pAscii, "/\r\n");
        if (Decode_Short_URL(mURL, mDecURL)) // 短縮URLの復号
		{
		  strcpy(mURL, mDecURL);
		} else 
#endif
		{
          *pAscii = '\x0';
		}
		break;
	  }
	  pAscii++;
	}
    if (!mURL[0])
      return sts;
	////////////////////////////////////
	pAscii = mURL;
    while(*pAscii) {
	  if (*pAscii < '\x20' || *pAscii > '\x7e' )
        return sts;
	  pAscii++;
	}
	
#else
    strtok(mURL, " =/?>\xff\x1b\"\t\r\n"); // サイト名を取得
#endif
	phttp = mURL;

	pno = phttp;
	{
	  if (strstr(pno, ".")) { // IPv4でIPアドレスかサイト名か確認
	    while(*pno) {
		  if (!(*pno >= (char)'0' && *pno <= (char)'9' ||
			    *pno == (char)'.') ) {
		    bAlpha = TRUE;
		    break;
		  }
		  pno++;
		}
	    strtok(phttp, ":"); // ポート指定を除去
#ifdef IPv6
	  } else if (strstr(pno, ":")) { // IPv6でIPアドレスかサイト名か確認
	    while(*pno) {
		  if (!(*pno >= (char)'0' && *pno <= (char)'9' ||
			    *pno >= (char)'a' && *pno <= (char)'z' ||
			    *pno >= (char)'A' && *pno <= (char)'Z' ||
			    *pno == (char)':' ||
			    *pno == (char)'%') ) {
		    bAlpha = TRUE;
		    break;
		  }
		  pno++;
		}
	  } else { // 上記以外
#endif
        return sts;
	  }
	}
#ifdef UPDATE_20151111 // URIBL問合せのホワイトリストテーブルを追加
	if (White_URIBL(phttp, mWRBL))
	{
      return sts; // 既出のチェックなので問合せしない。
	}
#endif
#ifdef UPDATE_20141023  // URIBL問合せの効率化
#ifdef UPDATE_20161014 // メールフィルタ条件でRBLへの重複問合せがキャッシュ設定により失敗する
    if (OverlapIgnore_URIBL(phttp, pFnRBL, &sts, FALSE))
#else
    if (OverlapIgnore_URIBL(phttp, pFnRBL))
#endif
	{
      return sts; // 既出のチェックなので問合せしない。
	}
#endif
	//strcpy(mPaddr, "::203.65.95.66"); 
	//strcpy(mPaddr, "203.65.95.66"); // test black list IP
	if (!bAlpha) {
      //bNUM = TRUE;
	  strcpy(mIP, phttp);
	  nIP = GetQueryHostName(phttp, mHostName);
	  strcpy(phttp, mIP);
	} else {
	  nIP = 1; // IPv4で確認
#ifdef DNS_QUERY
      if (pDepath && nDepath == 0) // フィルターテーブル毎に変更可能に
	  {
        sprintf(mHostName, "%s.", phttp);
	  } else 
#endif
	  {
	  ///////////////////////////////
#ifdef UPDATE_20061002 // ドメイン名の抽出が出来ないURIBL処理の対策
        pno = phttp+strlen(phttp)-1;
#else
        pno = phttp+strlen(pno)-1;
#endif
	    while(*pno != '.') {
          if (pno == phttp)
		    break;
		  pno--;
		}
	  
	    if (pno > phttp && *pno == '.') {
		  pno--;
		  while(*pno != '.') {
            if (pno == phttp)
			  break;
	        pno--;
		  }
	      if (*pno == '.')
		    pno++;
		} //else
          //return sts;
	    ///////////////////////////////
	    sprintf(mHostName, "%s.", pno); //phttp);
	  }
	}
#ifdef UPDATE_20071129 // 確認名称の先頭がピリオド若しくは連続するピリオドが含まれる対象はスキップする
    if (mHostName[0] == '.' || strstr(mHostName, "..")) {
      return sts;
    }
#endif

	pSite = mHostName + strlen(mHostName); // サイト名の加算位置
	while (1) {
#ifdef DNS_QUERY
      if (pDepath && nNUM && nDepath == 0)  // １回といあわせで終了
	  {
		break;
	  }
	  strcpy(pSite, mRBL);
#else
	  strcpy(pSite, pRBL);
#endif

#ifdef ACCEPT_LOG_LEVEL3
#ifdef UPDATE_20151111 // URIBL問合せのホワイトリストテーブルを追加
 	   sprintf(mLog, "Query_FILTER_RBL(%s%s%s%s)", mHostName, (mWRBL[0] ? "\"" : ""), (mWRBL[0]? mWRBL : ""), (mWRBL[0] ? "\"" : ""));
#else
 	   sprintf(mLog, "Query_FILTER_RBL(%s)", mHostName);
#endif
       LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
#ifdef UPDATE_20151111 // URIBL問合せのホワイトリストテーブルを追加
 	   if (bDebug) printf("Query_FILTER_RBL(%s%s%s%s)\n", mHostName, (mWRBL[0] ? "\"" : ""), (mWRBL[0]? mWRBL : ""), (mWRBL[0] ? "\"" : ""));
#else
       if (bDebug) printf("Query_FILTER_RBL(%s)\n", mHostName);
#endif

#ifdef IPv6
      if (nIP == 2) { // IPv6 address
#ifdef VC6
	    phost = getipnodebyname(mHostName, AF_INET6, AI_ALL, &nErr); //ORDBにレコードがあるか？
		if (phost) {
		  freehostent(phost);
          sts = TRUE; // ブラックリストに掲載
		  break;
		}
#else
		phost = NULL;
        memset(&Hints, 0, sizeof(Hints));
		//Hints.ai_flags = AI_NUMERICHOST | AI_PASSIVE; 数値表記のみになってしまう
        Hints.ai_family =  AF_INET6;
        Hints.ai_socktype = SOCK_STREAM;
		if (getaddrinfo(mHostName, NULL, &Hints, &AddrInfo) == 0) {
          freeaddrinfo(AddrInfo);
          sts = TRUE; // ブラックリストに掲載
		  break;
        }
#endif
	  } else if (nIP == 1) { // IPv4 address
#ifdef DNS_QUERY
	   if ((pDNS = strrchr(pSite, '\t'))) 
	   {
		 *pDNS = '\x0';
		 pDNS++;
#ifdef UPDATE_20080703 // メールフィルタのRBL参照DNS指定で、キャッシュ有効無効のオプションを追加
	     if (IPQuery(AF_INET, pDNS, bQuery, mHostName))
#else
	     if (IPQuery(AF_INET, pDNS, mHostName))
#endif
		 {
	       sts = TRUE; // ブラックリストに掲載
	       break; 
		 }
	   } else 
#endif
#ifdef VC6
	   {
 	      phost = getipnodebyname(mHostName, AF_INET, 0, &nErr); //ORDBにレコードがあるか？
		  if (phost) {
		    freehostent(phost);
	        sts = TRUE; // ブラックリストに掲載
	        break; 
		  }
	   }
#else
		phost = NULL;
        memset(&Hints, 0, sizeof(Hints));
		//Hints.ai_flags = AI_NUMERICHOST | AI_PASSIVE; 数値表記のみになってしまう
        Hints.ai_family =  AF_INET;
        Hints.ai_socktype = SOCK_STREAM;
		if (getaddrinfo(mHostName, NULL, &Hints, &AddrInfo) == 0) {
          freeaddrinfo(AddrInfo);
	      sts = TRUE; // ブラックリストに掲載
	      break; 
        }
#endif
	  }
#else
	  phost = gethostbyname(mHostName); //ORDBにレコードがあるか？
	  if (phost) {
        sts = TRUE; // ブラックリストに掲載
	    break; 
	  }
#endif
#ifdef DNS_QUERY
	  if (bAlpha && nNUM <= nDepath) // フィルターテーブル毎に変更可能に
#else
#ifdef UPDATE_20070407 // リンク先のドメイン名の調査する深さを調整可能に
	  if (bAlpha && nNUM <= nURIRBLDepath)
#else
	  if (bAlpha && nNUM == 0) // && nNUM <= 1) {
#endif
#endif
	  {
		nNUM++;
	    while(*pno != '.') {
          if (pno == phttp)
		    break;
		  pno--;
		}
 	    if (pno > phttp && *pno == '.') {
		  pno--;
		  while(*pno != '.') {
            if (pno == phttp)
			  break;
	        pno--;
		  }
	      if (*pno == '.')
		    pno++;
		  else
		  {
#ifndef UPDATE_20070407 // リンク先のドメイン名の調査する深さを調整可能に
			break;
#endif
		  }
		} else 
		  break;
	    //phttp = strstr(phttp, ".");
		//if (!phttp) // 区切りがない
		  //break;
        //phttp++;
 	    sprintf(mHostName, "%s.", pno); //phttp);
  	    pSite = mHostName + strlen(mHostName); // サイト名の加算位置
#ifdef UPDATE_20071129 // 確認名称の先頭がピリオド若しくは連続するピリオドが含まれる対象はスキップする
        if (mHostName[0] == '.' || strstr(mHostName, "..")) {
          break;
		}
#endif
		continue;
#ifdef UPDATE_20070407 // リンク先のドメイン名の調査する深さを調整可能に
	  }
#ifdef DNS_QUERY
	  else if (!bAlpha && nNUM <= nDepath) // フィルターテーブル毎に変更可能に
#else
	  else if (!bAlpha && nNUM <= nURIRBLDepath)
#endif
	  { // 数値のURLをサイト名として再チェック
		nNUM++;
	    nIP = 1; // IPv4で確認
	    sprintf(mHostName, "%s.", phttp);
  	    pSite = mHostName + strlen(mHostName); // サイト名の加算位置
#ifdef UPDATE_20071129 // 確認名称の先頭がピリオド若しくは連続するピリオドが含まれる対象はスキップする
        if (mHostName[0] == '.' || strstr(mHostName, "..")) {
          break;
		}
#endif
		continue;
#else
	  } else if (!bAlpha && nNUM == 0) { // 数値のURLをサイト名として再チェック
		nNUM++;
	    nIP = 1; // IPv4で確認
	    sprintf(mHostName, "%s.", phttp);
  	    pSite = mHostName + strlen(mHostName); // サイト名の加算位置
#ifdef UPDATE_20071129 // 確認名称の先頭がピリオド若しくは連続するピリオドが含まれる対象はスキップする
        if (mHostName[0] == '.' || strstr(mHostName, "..")) {
          break;
		}
#endif
		continue;
#endif
	  } else {
		break;
	  }
	}
#ifdef ACCEPT_LOG_LEVEL3
#ifdef DNS_QUERY
	strcpy(mRBL, pRBL);
#ifdef UPDATE_20151111 // URIBL問合せのホワイトリストテーブルを追加
	sprintf(mLog, "Query_FILTER_RBL(%s%s%s%s%s%s%s)=%s", mHostName, (pDNS ? " " : ""), (pDNS ? pDNS : ""), (pDepath ? pDepath : ""), (mWRBL[0] ? "\"" : ""), (mWRBL[0]? mWRBL : ""), (mWRBL[0] ? "\"" : ""), (sts ? "found list": "not found list"));
#else
	sprintf(mLog, "Query_FILTER_RBL(%s%s%s%s)=%s", mHostName, (pDNS ? " " : ""), (pDNS ? pDNS : ""), (pDepath ? pDepath : ""), (sts ? "found list": "not found list"));
#endif
    LEVEL_3_ACCEPTLOG(NULL, mLog);
#else
	sprintf(mLog, "Query_FILTER_RBL(%s)=%s", mHostName, (sts ? "found list": "not found list"));
    LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
#endif
#ifdef UPDATE_20161014 // メールフィルタ条件でRBLへの重複問合せがキャッシュ設定により失敗する
    OverlapIgnore_URIBL(phttp, pFnRBL, &sts, TRUE); // 結果の登録
#endif
    if (bDebug) printf("%s\n", (sts ? "found list": "not found list"));
    return sts;
}
#endif

#ifdef FILTER_PASS
BOOL FilterProcess_PASSIP(char *pAddr) {
  FILE *fp;
  int  i, n, mask, num, addr, start, dot;
  char *p, *pwild, *pmask, mwork[128];
  BOOL bsts = FALSE; // 一致するものが無い

  if ((fp = fopen(mPASSFilterFile, "rt"))) {
	p = fgets(mwork, sizeof(mwork), fp); // dummy
	while(p || !feof(fp)) {
	  p = strpbrk(mwork, "\r\n");
	  if (p)
		*p = '\x0';
	  if (mwork[0] != '\x0' &&
	      mwork[0] != ' ' &&
	      mwork[0] != '\'') {  // コメントチェック
#ifdef UPDATE_20070511 // IPアドレス確認ロジックの共有化
	   if ((bsts = IP_COMP(mwork, pAddr)))
	 	 break;
#else
		num = 1; // 比較アドレス数
		if ((pwild = strchr(mwork, '*'))) { // ワイルドカードでのマスク
		  *pwild = '\x0';
		} else if (strchr(mwork, '.')) { // IPv4のみ
		  if ((pwild = strchr(mwork, '/'))) {// ネットマスク
		    *pwild = '\x0';
		    mask = 32 - atoi(++pwild);
		    num = 1 << mask; // 2のべき乗範囲 num <<= mask; // 2のべき乗範囲
			dot = ((mask-1) / 8) + 1;
			if (dot > 1)
			  addr = num / (256*(dot-1));
			 else
			   addr = num;
			for (i = 1; i <= dot; i++) {
			  if ((pwild = strrchr(mwork, '.')))
				*pwild = '\x0';
			}
			start = atoi(pwild + 1);
		  }
		}
		if (num == 1) {
          if (!strncmp(pAddr, mwork, strlen(mwork))) {
		    bsts = TRUE;  // 一致するものがあった
		    break;
		  }
		} else { // ネットマスクの場合
	      for ( i = 0; i < addr; i++) {
			*pwild = '\x0';
			pmask = &mwork[strlen(mwork)];
		    sprintf(pmask, ".%d", start + i);
            if (!strncmp(pAddr, mwork, strlen(mwork))) {
		      bsts = TRUE;  // 一致するものがあった
			  break;
			}
		  }
		  if (bsts)
			break;
		}
#endif
	  }
	  mwork[0] ='\x0';
	  p = fgets(mwork, sizeof(mwork), fp); // dummy
	}
	fclose(fp);
  }
  return bsts;
}
#endif

#ifdef UPDATE_20050916 // SMTP AUTH のユーザＩＤ情報の格納
BOOL Check_FROM_Header(PSmtpContext pContext) {// FROM:ヘッダーがSMTP-AUTH ID と一致しているか否か
  BOOL bsts = FALSE;
  BOOL bhead = TRUE;
  DWORD nHeadNo = 0;
  char  c, *p, *q, mwork[2048];
  FILE *fdata;

#ifdef UPDATE_20070516 // 上長承認機能の追加
  DWORD nErr;
  CHAR mData[256];
#ifdef UPDATE_20161112 //題名ヘッダでのブラック＆ホワイトリストテーブル
  if ((bMailApproval || bMailApprovalRevers) && !pContext->mBOSS[0]) 
#else
  if (bMailApproval && !pContext->mBOSS[0]) 
#endif
  { // BOSSがいないとき
    strcpy(mData, pContext->mFnData);
    mData[strlen(mData)-1] = '$';
    if (!(fdata = fopen(mData, "rt"))) // ファイルが無い場合
      fdata = fopen(pContext->mFnData, "rt");
  } else {
    fdata = fopen(pContext->mFnData, "rt");
  }
#else
  fdata = fopen(pContext->mFnData, "rt");
#endif
  if (fdata) {
#ifdef DATA_CASH
    setvbuf( fdata, pContext->mFrbuf, _IOFBF, sizeof(pContext->mFrbuf) );
#endif
    while(!feof(fdata)) {
	  mwork[0] ='\x0';
  	  if (fgets( mwork, sizeof(mwork), fdata ) == NULL)
		break;
	  if (bhead)
        if (!(mwork[0] == ' ' || mwork[0] == '\t')) //新しいヘッダキーなら
		  nHeadNo = 0;
	  ///////////////////////////////////////////////////////////////////
	  if ( !_strnicmp(mwork,"from:",5) ||
		   (nHeadNo == 1 && (mwork[0] == ' ' || mwork[0] == '\t')) ) {
#ifdef UPDATE_20051004 // SMTP認証ID比較でFROM:が2行以上にまたがる対策
		if (nHeadNo == 0) // 始まり
		  p = &mwork[5];
		else
		  p = mwork;
#else
		p = &mwork[5];
#endif
	    nHeadNo = 1;
	    _strlwr(mwork);
		while (*p) {
		  if ((q = strstr(p, "<")))
            q++;
          else
		    q = p;
		  if (q) {
	        while(*q == ' ' || *q == '\t') // WSPがなくなるまで
		      q++;
		  }
		  if ((p = strpbrk(q, ",;> \r\n\t"))) {
		    *p = '\x0';
		    p++;
		  }
          if (!strnicmp(q, pContext->mUSER, strlen(pContext->mUSER))) {
		     bsts = TRUE;
		     break;
		  } 
#ifdef UPDATE_200691215 // エイリアスで送信者信頼度のFromヘッダ一致も可能にする
		  else
		  {
			char  *pdom, mAlias[512];
            strcpy(mAlias, q);
	        if ((pdom = strstr(mAlias, "@")))
			{
	          *pdom = '\x0';
			}
	        if (!GetAliases((LPCTSTR)SOFT_ALIASES_REG, (char *)mAlias, (char *)pdom, NULL))
			{
	          if (pdom)
			  {
	            *pdom = '@';
			  }
			} else { // 実アドレスが見つかった場合
              if (!strnicmp(mAlias, pContext->mUSER, strlen(pContext->mUSER))) {
		        bsts = TRUE;
		        break;
			  } 
			}
		  }
#endif
		}
	  }
	  //////////////////////////////////////////////////////////////////
	  // ヘッダの終了
	  if (bhead && (strlen(mwork) == 1 && mwork[0] == '\n') ||
		  (strlen(mwork) == 2 && mwork[0] == '\r' && mwork[1] == '\n')) {
	    break;
	  }
	}
	fclose(fdata);
  }
#ifndef UPDATE_20070613 // 上長承認機能追加 BXXXXXXXXX.MS$ファイル削除
#ifdef UPDATE_20070516 // 上長承認機能の追加
#ifdef UPDATE_20161112 //題名ヘッダでのブラック＆ホワイトリストテーブル
  if ((bMailApproval || bMailApprovalRevers) && !pContext->mBOSS[0]) 
#else
  if (bMailApproval && !pContext->mBOSS[0])  
#endif
  { // BOSSがいないとき
	while(!DeleteFile(mData)) {
	  nErr = GetLastError();
	  if (nErr == ERROR_FILE_NOT_FOUND)
		break;
	  _sleep(100);
	}
  }
#endif
#endif
  return bsts;
}
#endif

BOOL Filter(PSmtpContext pContext, char *pRCPTO) {
  BOOL  bsts = TRUE, bMess = TRUE;
  BOOL  bsharp = FALSE;
  FILE  *fdata, *fVirus, *ftmp;
  char  mData[512];
  char  mwork[256], mSub[256], mPack[512], *pw, *pl;
  char  mTemp[256], mL[512], *tmp;
  int   i, ni;
  struct VIRUS_TABLE Virus_Table;
///////
  DWORD  nVirus[2];
  HLOCAL hVirus = NULL;
#ifdef UPDATE_20050528_1
  HLOCAL hVirus2;
#endif
  struct VIRUS_TABLE *pVirus_Table = NULL;
//////
  CHAR  Bdir[MAX_PATH];
  char  mPVFn[512];
  int   nv;
  int   nLevel;
#ifdef UPADTE_20040518
  char *pat;
#endif
#ifdef ADDED_FILTER_TAG // 一致したメールのSubject:にタグを追加する [SPAM !!]
  char msub[512];
  int  nAction;
#endif

#ifdef UPDATE_20050916 // SMTP AUTH のユーザＩＤ情報の格納
#ifdef UPDATE_20050927 // SMTP AUTH のユーザＩＤ情報の格納 個人別対応
  if (pContext->nFROMSecLevel >= 2 && pContext->bAUTHSUCCESS) // Level 2 以上なら
#else
  if (nFROMSecLevel >= 2 && pContext->bAUTHSUCCESS) // Level 2 以上なら
#endif
  {
	if (!Check_FROM_Header(pContext)) // FROM:ヘッダーがSMTP-AUTH ID と一致しているか否か
      return FALSE;
  }
#endif

#ifdef FILTER_PASS
  if (bVirus && !FilterProcess_PASSIP(pContext->PeerAddr))
#else
  if (bVirus)
#endif
  {
	///////// フィルタ件数を確認
	fVirus = NULL;
	nVirus[0] = -1;
	nVirus[1] = 0;
	hVirus = LocalAlloc( LHND, sizeof(struct VIRUS_TABLE));
	nv = 0;
#ifdef ACCEPT_LOG_LEVEL3
	sprintf(mL, "Check Filter(%d) = %s", nv, mVirusFile);
    LEVEL_3_ACCEPTLOG(NULL, mL);
#endif
#ifdef UPDATE_20220728A // メールフィルタのファイルにバイナリコードが含まれていると処理がスキップしてしまう不具合
    fVirus = fopen(mVirusFile, "rb"); // デフォルトテーブル適用
#else
    fVirus = fopen(mVirusFile, "rt"); // デフォルトテーブル適用
#endif
DATA_NEXT:
	if (!fVirus) {
	  if (nv < 3)
        goto NEXT_FILTER_FILENAME;
	}
	if (fVirus) {
#ifdef DATA_CASH
      setvbuf( fVirus, pContext->mFrbuf, _IOFBF, sizeof(pContext->mFrbuf) );
#endif
	  while(!feof(fVirus)) {
		mwork[0] ='\x0';
  	    if (fgets( mwork, sizeof(mwork), fVirus ) == NULL)
		  break;
        strtok( mwork, "\r\n");
		//pw = strstr(mwork, ":");
		pw = strpbrk(mwork, ":");
		if (pw) {
		  *pw = '\x0';
          bsharp = FALSE;
		  if (strlen((pw+1)) > 1) {
			if (strncmp((pw+1), "##", 2) == 0)  {
              bsharp = TRUE;
              pw++;
			}
		  }
		  if (strcmp(mwork,"Virus") == 0) {
            strncpy(Virus_Table.mVirus, (pw+1), sizeof(Virus_Table.mVirus));
			Virus_Table.mVirus[sizeof(Virus_Table.mVirus)-1] = '\x0';
            Virus_Table.mReceived[0] = '\x0';
            Virus_Table.mSubject[0] = '\x0';
            Virus_Table.mFrom[0] = '\x0';
            Virus_Table.mTo[0] = '\x0';
			Virus_Table.mContentType[0] = '\x0';
			Virus_Table.mContentTransferEncoding[0] = '\x0';
			Virus_Table.mXMailer[0] = '\x0';
			Virus_Table.mXUIDL[0] = '\x0';
			Virus_Table.mFolder[0] = '\x0';
            Virus_Table.bReceived[0] = FALSE;
			Virus_Table.bSubject[0] = FALSE;
			Virus_Table.bFrom[0] = FALSE;
			Virus_Table.bTo[0] = FALSE;
			Virus_Table.bContentType[0] = FALSE;
			Virus_Table.bContentTransferEncoding[0] = FALSE;
			Virus_Table.bXMailer[0] = FALSE;
			Virus_Table.bXUIDL[0] = FALSE;
			Virus_Table.bBody[0] = FALSE;
			Virus_Table.nBody[0] = 0;
			Virus_Table.nBody[1] = 0;
			for (i = 0; i < 5; i++)
              Virus_Table.mBody[i][0] = '\x0';
            Virus_Table.nLevel = 0;
 			for (i = 0; i < 5; i++)
              Virus_Table.mWarning[i][0] = '\x0';
#ifdef ADDED_FILTER_TAG // 一致したメールのSubject:にタグを追加する [SPAM !!]
            Virus_Table.mTag[0] = '\x0';
#endif
#ifdef ADDED_FILTER_RBL   // 本文内のURLを指定したRBLへ問い合わせるオプションを追加する 例 [SPAM !!]
			Virus_Table.bRBL[0] = FALSE;
            Virus_Table.mRBL[0] = '\x0';
#endif
#ifdef ADDED_FILTER_FORWARD // 一致したメールを転送する。
            Virus_Table.mForward[0] = '\x0';
#endif
#ifdef ADDED_FILTER_UNIQUE   // 予約済み以外のヘッダのチェックオプションを追加する
            Virus_Table.bUNIQUE[0] = FALSE;
			Virus_Table.mUNIQUEHEADER[0] = '\x0';
            Virus_Table.mUNIQUE[0] = '\x0';
#endif
#ifdef ADDED_WILDCARD_SEARCH // 大小文字＆ワイルドカード指定でトークン比較する機能 0:無効 1:有効
            Virus_Table.nComp2[0] = 0;
#endif
          ////////////////////////////////////////////////
		  } else if (strcmp(mwork,"Received") == 0) {
            Virus_Table.bReceived[0] = TRUE;
			if (!bsharp && stricmp((pw+1), "#blank") == 0) {// 2001.09.14
              *(pw+1) = '\x0'; // 空白設定
			}

            strncpy(Virus_Table.mReceived, (pw+1), sizeof(Virus_Table.mReceived));
			Virus_Table.mReceived[sizeof(Virus_Table.mReceived)-1] = '\x0';
		  } else if (strcmp(mwork,"Subject") == 0) {
            Virus_Table.bSubject[0] = TRUE;
			if (!bsharp && stricmp((pw+1), "#blank") == 0) {// 2001.09.14
              *(pw+1) = '\x0'; // 空白設定
			}
#ifdef UPDATE_20050516
			pl = pw; // 退避
#ifdef UPDATE_20050107
		    LineConv(mSub, sizeof(mSub), (pw+1) , mPack);
#else
			LineConv(mSub, pw , mPack);
#endif
			if (mPack[0]) {
			  if ((pl = strchr(mSub, '\x1b'))) {
			    pw = pl+2;
#ifdef UPDATE_20240412 // メールフィルタ指定でJIS形式のMIME-Bエンコード指定で全角と半角混在文字列の場合、全角と半角区切りで残りの検索文字列が消えてしまう。
			    if (pl = strrchr(pw, '\x1b'))
				  *pl = '\x0';
#else
			    strtok(pw, "\x1b");
#endif
#ifdef UPDATE_20091210 // MIMEエンコードの指定でハングすることがある
			  } else {
				pl = &mSub;
				pw = pl;
				pw--;
#endif
			  } 
			}
#endif
            strncpy(Virus_Table.mSubject, (pw+1), sizeof(Virus_Table.mSubject));
			Virus_Table.mSubject[sizeof(Virus_Table.mSubject)-1] = '\x0';
#ifdef UPDATE_20050516
			pw = pl; // 復帰
#endif
		  } else if (strcmp(mwork,"From") == 0) {
            Virus_Table.bFrom[0] = TRUE;
			if (!bsharp && stricmp((pw+1), "#blank") == 0) {// 2001.09.14
              *(pw+1) = '\x0'; // 空白設定
			}
#ifdef UPDATE_20050528
			pl = pw; // 退避
		    LineConv(mSub, sizeof(mSub), (pw+1) , mPack);
			if (mPack[0]) {
			  if ((pl = strchr(mSub, '\x1b'))) {
			    pw = pl+2;
#ifdef UPDATE_20240412 // メールフィルタ指定でJIS形式のMIME-Bエンコード指定で全角と半角混在文字列の場合、全角と半角区切りで残りの検索文字列が消えてしまう。
			    if (pl = strrchr(pw, '\x1b'))
				  *pl = '\x0';
#else
			    strtok(pw, "\x1b");
#endif
#ifdef UPDATE_20091210 // MIMEエンコードの指定でハングすることがある
			  } else {
				pl = &mSub;
				pw = pl;
				pw--;
#endif
			  }
			}
#endif
            strncpy(Virus_Table.mFrom, (pw+1), sizeof(Virus_Table.mFrom));
			Virus_Table.mFrom[sizeof(Virus_Table.mFrom)-1] = '\x0';
#ifdef UPDATE_20050528
			pw = pl; // 復帰
#endif
		  } else if (strcmp(mwork,"To") == 0) {
            Virus_Table.bTo[0] = TRUE;
			if (!bsharp && stricmp((pw+1), "#blank") == 0) {// 2001.09.14
              *(pw+1) = '\x0'; // 空白設定
			}

            strncpy(Virus_Table.mTo, (pw+1), sizeof(Virus_Table.mTo));
			Virus_Table.mTo[sizeof(Virus_Table.mTo)-1] = '\x0';
		  } else if (strcmp(mwork,"Content-Type") == 0) {
            Virus_Table.bContentType[0] = TRUE;
			if (!bsharp && stricmp((pw+1), "#blank") == 0) {// 2001.09.14
              *(pw+1) = '\x0'; // 空白設定
			}

            strncpy(Virus_Table.mContentType, (pw+1), sizeof(Virus_Table.mContentType));
			Virus_Table.mContentType[sizeof(Virus_Table.mContentType)-1] = '\x0';
		  } else if (strcmp(mwork,"Content-Transfer-Encoding") == 0) {
            Virus_Table.bContentTransferEncoding[0] = TRUE;
			if (!bsharp && stricmp((pw+1), "#blank") == 0) {// 2001.09.14
              *(pw+1) = '\x0'; // 空白設定
			}

            strncpy(Virus_Table.mContentTransferEncoding, (pw+1), sizeof(Virus_Table.mContentTransferEncoding));
			Virus_Table.mContentTransferEncoding[sizeof(Virus_Table.mContentTransferEncoding)-1] = '\x0';
		  } else if (strcmp(mwork,"X-Mailer") == 0) {
            Virus_Table.bXMailer[0] = TRUE;
			if (!bsharp && stricmp((pw+1), "#blank") == 0) {// 2001.09.14
              *(pw+1) = '\x0'; // 空白設定
			}

            strncpy(Virus_Table.mXMailer, (pw+1), sizeof(Virus_Table.mXMailer));
			Virus_Table.mXMailer[sizeof(Virus_Table.mXMailer)-1] = '\x0';
		  } else if (strcmp(mwork,"X-UIDL") == 0) {
            Virus_Table.bXUIDL[0] = TRUE;
			if (!bsharp && stricmp((pw+1), "#blank") == 0) {// 2001.09.14
              *(pw+1) = '\x0'; // 空白設定
			}

            strncpy(Virus_Table.mXUIDL, (pw+1), sizeof(Virus_Table.mXUIDL));
			Virus_Table.mXUIDL[sizeof(Virus_Table.mXUIDL)-1] = '\x0';
#ifdef ADDED_FILTER_UNIQUE   // 予約済み以外のヘッダのチェックオプションを追加する
		  } else if (strcmp(mwork,"Unique-Header") == 0) {
            Virus_Table.bUNIQUE[0] = TRUE;
			if ((pw = strchr(pw+1, ':')))
			{
			  if (!bsharp && stricmp((pw+1), "#blank") == 0) {// 2001.09.14
                *(pw+1) = '\x0'; // 空白設定
			  }

              strncpy(Virus_Table.mUNIQUEHEADER, &mwork[14], 280);
			  strtok(Virus_Table.mUNIQUEHEADER, ":");
              strncpy(Virus_Table.mUNIQUE, (pw+1), sizeof(Virus_Table.mUNIQUE));
			  Virus_Table.mUNIQUE[sizeof(Virus_Table.mUNIQUE)-1] = '\x0';
			}
#endif
		  } else if (strcmp(mwork,"Body") == 0) {
            Virus_Table.bBody[0] = TRUE;
			i = 0;
			while(i < 5) {
		      fgets( mwork, sizeof(mwork), fVirus );
			  if (mwork[0] == '\r' || mwork[0] == '\n')
				mwork[0] = '\x0';
			  else
                strtok(mwork, "\r\n");
			  if (strcmp(mwork,"BodyEnd:") == 0)
				break;
		      strncpy(Virus_Table.mBody[i], mwork, sizeof(Virus_Table.mBody[0]));
			  Virus_Table.mBody[i][sizeof(Virus_Table.mBody[0])-1] = '\x0';
			  i++;
			}
			Virus_Table.nBody[0] = i;
		  } else if (strcmp(mwork,"EncBody") == 0) {
            Virus_Table.bBody[0] = TRUE;
			i = 0;
			while(i < 5) {
			  strcpy(mwork, "\xff");
		      fgets( &mwork[1], sizeof(mwork)-1, fVirus );
			  if (mwork[0] == '\r' || mwork[0] == '\n')
				mwork[0] = '\x0';
			  else
                strtok(mwork, "\r\n");
			  if (strcmp(&mwork[1],"EncBodyEnd:") == 0)
				break;
              SPA_Decode(mwork, Virus_Table.mBody[i]); // 複号化
		      //strncpy(Virus_Table.mBody[i], mwork, sizeof(Virus_Table.mBody[0]));
			  //Virus_Table.mBody[i][sizeof(Virus_Table.mBody)-1] = '\x0';
			  i++;
			}
			Virus_Table.nBody[0] = i;
		  } else if (strcmp(mwork,"Level") == 0) {
		    Virus_Table.nLevel = atoi(pw+1);
		  } else if (strcmp(mwork,"Warning") == 0) {
			i = 0;
			while(i < 5) {
		      fgets( mwork, sizeof(mwork), fVirus );
              strtok(mwork, "\r\n");
			  if (strcmp(mwork,"WarningEnd:") == 0)
				break;
		      strncpy(Virus_Table.mWarning[i], mwork, sizeof(Virus_Table.mWarning[0]));
			  Virus_Table.mWarning[i][sizeof(Virus_Table.mWarning[0])-1] = '\x0';
			  i++;
			}
		  } else if (strcmp(mwork,"Folder") == 0) {
            strncpy(Virus_Table.mFolder, (pw+1), sizeof(Virus_Table.mFolder));
			Virus_Table.mFolder[sizeof(Virus_Table.mFolder)-1] = '\x0';
#ifdef ADDED_FILTER_TAG // 一致したメールのSubject:にタグを追加する [SPAM !!]
		  } else if (strcmp(mwork,"Tag") == 0) {
            strncpy(Virus_Table.mTag, (pw+1), sizeof(Virus_Table.mTag));
			Virus_Table.mTag[sizeof(Virus_Table.mTag)-1] = '\x0';
#endif
#ifdef ADDED_FILTER_RBL   // 本文内のURLを指定したRBLへ問い合わせるオプションを追加する 例 [SPAM !!]
		  } else if (strcmp(mwork,"RBL") == 0) {
			Virus_Table.bRBL[0] = TRUE;
            strncpy(Virus_Table.mRBL, (pw+1), sizeof(Virus_Table.mRBL));
			Virus_Table.mRBL[sizeof(Virus_Table.mRBL)-1] = '\x0';
#endif
#ifdef ADDED_FILTER_FORWARD // 一致したメールを転送する。
		  } else if (strcmp(mwork,"Forward") == 0) {
            strncpy(Virus_Table.mForward, (pw+1), sizeof(Virus_Table.mForward));
			Virus_Table.mForward[sizeof(Virus_Table.mForward)-1] = '\x0';
#endif
#ifdef ADDED_WILDCARD_SEARCH // 大小文字＆ワイルドカード指定でトークン比較する機能 0:無効 1:有効
		  } else if (nMailFiterUp > 0 && strcmp(mwork,"Comp") == 0) { // メールフィルタ機能強化
		    Virus_Table.nComp2[0] = atoi(pw+1);
#endif
		  } else if (strcmp(mwork,"VirusEnd") == 0) {
//////////
#ifdef UPDATE_20050528_1
		    if ((hVirus2 = LocalReAlloc( hVirus, sizeof(struct VIRUS_TABLE) * (nVirus[1]+1), LMEM_ZEROINIT))) {
			hVirus = hVirus2;
#else
            hVirus = LocalReAlloc( hVirus, sizeof(struct VIRUS_TABLE) * (nVirus[1]+1), LMEM_ZEROINIT);
#endif
			pVirus_Table = LocalLock(hVirus);
			{
			  ///////////////////////////////////////////////////////////////////////////
			  // メモリ上にフィルタ情報全てコピー
 			  strcpy((pVirus_Table+nVirus[1])->mVirus, Virus_Table.mVirus);
 			  strcpy((pVirus_Table+nVirus[1])->mReceived, Virus_Table.mReceived);
 			  strcpy((pVirus_Table+nVirus[1])->mSubject, Virus_Table.mSubject);
 			  strcpy((pVirus_Table+nVirus[1])->mFrom, Virus_Table.mFrom);
 			  strcpy((pVirus_Table+nVirus[1])->mTo, Virus_Table.mTo);
 			  strcpy((pVirus_Table+nVirus[1])->mContentType, Virus_Table.mContentType);
 			  strcpy((pVirus_Table+nVirus[1])->mContentTransferEncoding, Virus_Table.mContentTransferEncoding);
			  strcpy((pVirus_Table+nVirus[1])->mXMailer, Virus_Table.mXMailer);
 			  strcpy((pVirus_Table+nVirus[1])->mXUIDL, Virus_Table.mXUIDL);
#ifdef ADDED_FILTER_UNIQUE   // 予約済み以外のヘッダのチェックオプションを追加する
 			  strcpy((pVirus_Table+nVirus[1])->mUNIQUEHEADER, Virus_Table.mUNIQUEHEADER);
 			  strcpy((pVirus_Table+nVirus[1])->mUNIQUE, Virus_Table.mUNIQUE);
#endif
 			  for (i = 0; i < Virus_Table.nBody[0]; i++)
 			    strcpy((pVirus_Table+nVirus[1])->mBody[i], Virus_Table.mBody[i]);
 			  (pVirus_Table+nVirus[1])->bReceived[0] = Virus_Table.bReceived[0];
 			  (pVirus_Table+nVirus[1])->bSubject[0] = Virus_Table.bSubject[0];
 			  (pVirus_Table+nVirus[1])->bFrom[0] = Virus_Table.bFrom[0];
 			  (pVirus_Table+nVirus[1])->bTo[0] = Virus_Table.bTo[0];
 			  (pVirus_Table+nVirus[1])->bContentType[0] = Virus_Table.bContentType[0];
 			  (pVirus_Table+nVirus[1])->bContentTransferEncoding[0] = Virus_Table.bContentTransferEncoding[0];
 			  (pVirus_Table+nVirus[1])->bXMailer[0] = Virus_Table.bXMailer[0];
 			  (pVirus_Table+nVirus[1])->bXUIDL[0] = Virus_Table.bXUIDL[0];
#ifdef ADDED_FILTER_UNIQUE   // 予約済み以外のヘッダのチェックオプションを追加する
 			  (pVirus_Table+nVirus[1])->bUNIQUE[0] = Virus_Table.bUNIQUE[0];
#endif
 			  (pVirus_Table+nVirus[1])->bBody[0] = Virus_Table.bBody[0];
 			  (pVirus_Table+nVirus[1])->nBody[0] = Virus_Table.nBody[0];
 			  (pVirus_Table+nVirus[1])->nBody[1] = 0; //Virus_Table.nBody;
 			  (pVirus_Table+nVirus[1])->nLevel = Virus_Table.nLevel;
 			  for (i = 0; i < 5; i++)
 			    strcpy((pVirus_Table+nVirus[1])->mWarning[i], Virus_Table.mWarning[i]);
#ifdef ADDED_FILTER_TAG // 一致したメールのSubject:にタグを追加する [SPAM !!]
 			  strcpy((pVirus_Table+nVirus[1])->mTag, Virus_Table.mTag);
#endif
#ifdef ADDED_FILTER_RBL   // 本文内のURLを指定したRBLへ問い合わせるオプションを追加する 例 [SPAM !!]
 			  (pVirus_Table+nVirus[1])->bRBL[0] = Virus_Table.bRBL[0];
 			  strcpy((pVirus_Table+nVirus[1])->mRBL, Virus_Table.mRBL);
#endif
#ifdef ADDED_FILTER_FORWARD // 一致したメールを転送する。
			  strcpy((pVirus_Table+nVirus[1])->mForward, Virus_Table.mForward);
#endif
#ifdef ADDED_WILDCARD_SEARCH // 大小文字＆ワイルドカード指定でトークン比較する機能 0:無効 1:有効
 			  (pVirus_Table+nVirus[1])->nComp2[0] = Virus_Table.nComp2[0];
#endif
 			  strcpy((pVirus_Table+nVirus[1])->mFolder, Virus_Table.mFolder);
			  ///////////////////////////////////////////////////////////////////////////
	          nVirus[1]++;
			}
			LocalUnlock(hVirus);
#ifdef UPDATE_20050528_1
			} else {
#ifdef ACCEPT_LOG_LEVEL3
	       sprintf(mL, "Read Filter(%d) = LocalReAlloc() failed. (code=%lu) / exceeded lists. (no=%lu)", nv, GetLastError(), nVirus[i]+1);
           LEVEL_3_ACCEPTLOG(NULL, mL);
#endif
			}
#endif
		    ///////////
		  }
		  *pw = ':';
		}
		if (nVirus[1] >= nPatternOfTheOnce) {
		  pVirus_Table = LocalLock(hVirus);
		  nVirus[0] = VirusCheck(pContext, pVirus_Table, nVirus[1]);
          if (nVirus[0] == -1) {
		    nVirus[1] = 0;     // hitしてないなら他のデータ読込み
			LocalUnlock(hVirus);
		  } else {
			break;  // hitしたならチェック終了
		  }
		}
	  }
	  fclose(fVirus);
	  //////////////////////////
NEXT_FILTER_FILENAME:
	  if (nVirus[0] == -1) {      // フィルタチェックで一致していないなら
	    if (nv == 0) {            // ２回目 全体のフィルタテーブルの読込み
           nv++;
#ifdef ACCEPT_LOG_LEVEL3
	       sprintf(mL, "Check Filter(%d) = %s", nv, mMailFile);
           LEVEL_3_ACCEPTLOG(NULL, mL);
#endif
#ifdef UPDATE_20220728A // メールフィルタのファイルにバイナリコードが含まれていると処理がスキップしてしまう不具合
           fVirus = fopen(mMailFile, "rb"); // 個人のフィルタテーブルがあるならば
#else
           fVirus = fopen(mMailFile, "rt"); // 個人のフィルタテーブルがあるならば
#endif
		   if (fVirus)
		     goto DATA_NEXT;
		}
		if (nv == 1 || nv == 2) {
	      if (CheckDomain(pRCPTO)) { // 内部ユーザーのみ個人フィルタの確認
#ifndef V3
            pat = strchr(pRCPTO, '@'); // ドメイン以降が消えない対策(v2,v1)で
            GetBaseDirectory(Bdir, mMailBoxDir, pRCPTO, pContext->MyAddr); // ユーザ別のフィルタ
			if (pat)
			  *pat = '@';
#else
            GetBaseDirectory(Bdir, mMailBoxDir, pRCPTO, pContext->MyAddr); // ユーザ別のフィルタ
#endif
			// ２回目 個人のウイルステーブル(viurs.dat)の読込み
			// ３回目 個人のフィルタステーブル(mail.dat)の読込み
			if (nv == 1) {
			  sprintf(mPVFn, "%svirus.dat", Bdir);
              nv++;
              if (mPVFn[0]) {    
#ifdef ACCEPT_LOG_LEVEL3
	            sprintf(mL, "Check Filter(%d) = %s", nv, mPVFn);
                LEVEL_3_ACCEPTLOG(NULL, mL);
#endif
#ifdef UPDATE_20220728A // メールフィルタのファイルにバイナリコードが含まれていると処理がスキップしてしまう不具合
	            fVirus = fopen(mPVFn, "rb"); // 個人のフィルタテーブルがあるならば
#else
	            fVirus = fopen(mPVFn, "rt"); // 個人のフィルタテーブルがあるならば
#endif
			    if (fVirus)
		          goto DATA_NEXT;
			  }
			} 
			if (nv == 2) {
			  sprintf(mPVFn, "%smail.dat", Bdir);
              nv++;
              if (mPVFn[0]) {    
#ifdef ACCEPT_LOG_LEVEL3
	            sprintf(mL, "Check Filter(%d) = %s", nv, mPVFn);
                LEVEL_3_ACCEPTLOG(NULL, mL);
#endif
#ifdef UPDATE_20220728A // メールフィルタのファイルにバイナリコードが含まれていると処理がスキップしてしまう不具合
	            fVirus = fopen(mPVFn, "rb"); // 個人のフィルタテーブルがあるならば
#else
	            fVirus = fopen(mPVFn, "rt"); // 個人のフィルタテーブルがあるならば
#endif
			    if (fVirus)
		          goto DATA_NEXT;
			  }
			} 
		  }
		}
	  } 
	  //////////////////////////
	  ///// フィルタ情報を全て読み込んでから一括チェックする
	  if (nVirus[0] == -1) {
	    if (nVirus[1] > 0 && nVirus[1] < nPatternOfTheOnce) {
		  pVirus_Table = LocalLock(hVirus);
		  nVirus[0] = VirusCheck(pContext, pVirus_Table, nVirus[1]);
		}
	  }
	  if (nVirus[0] != -1) {
		if ((pVirus_Table+nVirus[0])->nLevel >= 100) { // 100以上なら接続IP拒否設定、Effect.datに IP false
	 	  nLevel = (pVirus_Table+nVirus[0])->nLevel / 10;
		  if ((pVirus_Table+nVirus[0])->nLevel - (nLevel*10) > 0 ) { // 下一桁が0以外なら
		    // Effect.DatにIP追加
		    sprintf(pContext->meffect,"%seffect.dat", mProgPath);
            if ((pContext->Effefp = fopen(pContext->meffect,"at"))) {
		      fprintf(pContext->Effefp, "\n%s false\n", pContext->PeerAddr);
			  fclose(pContext->Effefp);
			}
		  }
	   	  (pVirus_Table+nVirus[0])->nLevel = nLevel; // ２桁に調整
		}
		nAction = (pVirus_Table+nVirus[0])->nLevel / 10;
	    switch((pVirus_Table+nVirus[0])->nLevel / 10) {
	      case 0: bsts = TRUE; 
	 	          break;
		  case 1: bsts = FALSE;
		 		  break;
#ifdef ADDED_FILTER_HEADER_AND_FORWARD   // 予約済み以外のヘッダのチェックオプションを追加する
		  case 7: 
#endif
#ifdef ADDED_FILTER_SUBJECT_AND_FORWARD   // 予約済み以外のヘッダのチェックオプションを追加する
		  case 6: 
#endif
#ifdef ADDED_FILTER_FORWARD // 一致したメールを転送する。
		  case 5: 
#ifdef UPDATE_20071204  // メッセージＩＤ採番処理を修正(Bym10id)
		          sprintf(mTemp, "%stemp\\%s.~CP", mMailSpoolDir, pContext->mMsgId);
#else
		          sprintf(mTemp, "%stemp\\B%010lu.~CP", mMailSpoolDir, pContext->nMsgId);
#endif
		 	      fdata = fopen(pContext->mFnHead, "rt");
 	              if (fdata) {
#ifdef DATA_CASH
                    setvbuf( fdata, pContext->mFrbuf, _IOFBF, sizeof(pContext->mFrbuf) );
#endif
#ifdef ACCEPT_LOG_LEVEL3
	                sprintf(mL, "Filter() Tag: fopen(%s)", mTemp);
                    LEVEL_3_ACCEPTLOG(NULL, mL);
#endif
				    ftmp = fopen(mTemp, "wt");
				    if (ftmp) {
	                  fgets( mData, sizeof(mData), fdata ); //Message-ID:
					  fputs(mData, ftmp );
					  fgets( mData, sizeof(mData), fdata ); //Return-path: 
					  fputs(mData, ftmp );
					  fprintf(ftmp, "Recipient: %s\n", (pVirus_Table+nVirus[0])->mForward); //Recipient: 
					  strcpy(pRCPTO, (pVirus_Table+nVirus[0])->mForward);
					  fclose(ftmp);
					}
					fclose(fdata);
					// RCPはアクセス中なのでここでコピーは出来ない。
                    fclose(pContext->RCPfp); // 一旦クローズ
					// RCPを書換え
					for (ni = 0; ni < 10; ni++) {
					  if (MoveFileEx(mTemp, pContext->mFnHead, MOVEFILE_COPY_ALLOWED | MOVEFILE_WRITE_THROUGH) != 0) { // 移動成功
						break;
					  }
					  else
					  {        // 移動失敗リトライ
#ifdef ACCEPT_LOG_LEVEL3
	                    sprintf(mL, "Filter() Tag: MoveFileEx(%s) err=(%d)", mTemp, GetLastError());
                        LEVEL_3_ACCEPTLOG(NULL, mL);
#endif
						_sleep(100*ni);
			            DeleteFile(pContext->mFnHead);
					  }
					}
					////////////////////////////////
					// 再オープン
					while(!(pContext->RCPfp = fopen(pContext->mFnHead, "rt")))
					{
                      if (bServiceTerminating)
		                break;
	                  _sleep(WAIT_TIME);
					}
					if (pContext->RCPfp){
	                  fgets( mData, sizeof(mData), pContext->RCPfp ); //Message-ID:
					  fgets( mData, sizeof(mData), pContext->RCPfp ); //Return-path: 
					  fgets( mData, sizeof(mData), pContext->RCPfp ); //Recipient:
					  //fclose(pContext->RCPfp); クローズ不要
					}
				  }
#ifdef ADDED_FILTER_HEADER_AND_FORWARD   // 予約済み以外のヘッダのチェックオプションを追加する
				  if (nAction == 5)
#endif
				  {
				    break;
				  }
#endif

#ifdef ADDED_FILTER_HEADER // 一致したメールのSヘッダとして追加する。
		  case 4: // ヘッダとして追加するオプション
#endif
		  case 2: 
		  case 3: bsts = TRUE; 
#ifdef UPDATE_20071204  // メッセージＩＤ採番処理を修正(Bym10id)
		          sprintf(mTemp, "%stemp\\%s.~SG", mMailSpoolDir, pContext->mMsgId);
#else
		          sprintf(mTemp, "%stemp\\B%010lu.~SG", mMailSpoolDir, pContext->nMsgId);
#endif
		 	      fdata = fopen(pContext->mFnData, "rt");
 	              if (fdata) {
#ifdef DATA_CASH
                    setvbuf( fdata, pContext->mFrbuf, _IOFBF, sizeof(pContext->mFrbuf) );
#endif
#ifdef ACCEPT_LOG_LEVEL3
	                sprintf(mL, "Filter() Tag: fopen(%s)", mTemp);
                    LEVEL_3_ACCEPTLOG(NULL, mL);
#endif
				    ftmp = fopen(mTemp, "wt");
				    if (ftmp) {
#ifdef DATA_CASH
                      setvbuf( ftmp, pContext->mFwbuf, _IOFBF, sizeof(pContext->mFwbuf) );
#endif
#ifdef UPDATE_20061225 // "拡張子が".~SG"のファイルがTEMPフォルダに削除されずに残されることがある
	                  while(fgets( mwork, sizeof(mwork), fdata ))
#else
	                  fgets( mwork, sizeof(mwork), fdata );
	                  while(!feof(fdata))
#endif
					  {
#ifdef ADDED_FILTER_TAG // 一致したメールのSubject:にタグを追加する [SPAM !!]
	                    if (nAction == 2 && bMess && ( (strlen(mwork) == 1 && mwork[0] == '\n') ||
							(strlen(mwork) == 2 && mwork[0] == '\r' && mwork[1] == '\n') ) )
#else
	                    if (bMess && ( (strlen(mwork) == 1 && mwork[0] == '\n') ||
							(strlen(mwork) == 2 && mwork[0] == '\r' && mwork[1] == '\n') ) )
#endif
						{
		                  fputs( mwork, ftmp );
						  for (i = 0; i < 5; i++) {
						    strtok((pVirus_Table+nVirus[0])->mWarning[i], "\r\n");
						    fprintf(ftmp, "%s\r\n", (pVirus_Table+nVirus[0])->mWarning[i]);
						  }
						  bMess = FALSE;
						} else {
#ifdef ADDED_FILTER_TAG // 一致したメールのSubject:にタグを追加する [SPAM !!]
						  if (bMess && !strnicmp(mwork, "subject:", 8) &&
							  (nAction == 3 || nAction == 6) )
						  {
#ifdef UPDATE_20060506 // メールフィルタで同じTagをを重複追加しないようにする。
						    if (!strstr(&mwork[8], (pVirus_Table+nVirus[0])->mTag)) // 指定タグが見つからないときは追加する
#endif
							{
							  mwork[7] = '\x0';
                              sprintf(msub, "%s: %s %s", mwork, (pVirus_Table+nVirus[0])->mTag, &mwork[8]);
                              strncpy(mwork, msub, min(255, strlen(msub)));
							  mwork[min(255, strlen(msub))] = '\x0';
							}
						    bMess = FALSE; // TAGが有効ならWarningは記述しない
						  }
#ifdef ADDED_FILTER_HEADER // 一致したメールのヘッダとして追加する。
						  else if (bMess && 
							       (nAction == 4 || nAction == 7 ) )
						  {
#ifdef UPDATE_20080628 // メールフィルタのヘッダ挿入機能で、':'の区切りがない場合強制的に挿入する
							if (!strchr((pVirus_Table+nVirus[0])->mTag, ':')) {
							  fprintf(ftmp, "%s:\n", (pVirus_Table+nVirus[0])->mTag);
							} else
#endif
							{
							  fprintf(ftmp, "%s\n", (pVirus_Table+nVirus[0])->mTag);
							}
						    bMess = FALSE; // TAGが有効ならWarningは記述しない
						  }
#endif
#endif
		                  fputs( mwork, ftmp );
						}
#ifndef UPDATE_20061225 // "拡張子が".~SG"のファイルがTEMPフォルダに削除されずに残されることがある
		                fgets( mwork, sizeof(mwork), fdata );
#endif
					  }
#ifdef UPDATE_20061213 // "拡張子が".~SG"のファイルがTEMPフォルダに削除されずに残されることがある
					  fflush(ftmp);
#endif
					  fclose(ftmp);
#ifdef ACCEPT_LOG_LEVEL3
	                  sprintf(mL, "Filter() Tag: fclose(%s)", mTemp);
                      LEVEL_3_ACCEPTLOG(NULL, mL);
#endif
					}
					fclose(fdata);
#ifndef TUNING1
			        DeleteFile(pContext->mFnData);
#ifdef UPDATE_20070607A // "拡張子が".~SG"のファイルがTEMPフォルダに削除されずに残されることがある
					while((ftmp = fopen(pContext->mFnData, "rt"))) {
					  fclose(ftmp);
                      DeleteFile(pContext->mFnData);
                      if (bServiceTerminating)
		                break;
	                  _sleep(WAIT_TIME);
					}
#endif
					////////////////////////////
#ifdef UPDATE_20070423 // "拡張子が".~SG"のファイルがTEMPフォルダに削除されずに残されることがある
					while(!(ftmp = fopen(mTemp, "rt"))) {
                      if (bServiceTerminating)
		                break;
	                  _sleep(WAIT_TIME);
					}
					fclose(ftmp);
#endif
					////////////////////////////
#ifdef UPDATE_20061213 // "拡張子が".~SG"のファイルがTEMPフォルダに削除されずに残されることがある
#ifdef UPDATE_20061225 // "拡張子が".~SG"のファイルがTEMPフォルダに削除されずに残されることがある
					for (ni = 0; ni < 10; ni++) {
					  if (MoveFileEx(mTemp, pContext->mFnData, MOVEFILE_COPY_ALLOWED | MOVEFILE_WRITE_THROUGH) != 0) { // 移動成功
						break;
					  }
					  else
					  {        // 移動失敗リトライ
#ifdef ACCEPT_LOG_LEVEL3
	                    sprintf(mL, "Filter() Tag: MoveFileEx(%s) err=(%d)", mTemp, GetLastError());
                        LEVEL_3_ACCEPTLOG(NULL, mL);
#endif
						_sleep(100*ni);
			            DeleteFile(pContext->mFnData);
					  }
					}
#ifdef UPDATE_20070204 // TEMPフォルダ内".MSG"のファイルが読み込み可能か確認
					while(!(fdata = fopen(pContext->mFnData, "rt"))) {
                      if (bServiceTerminating)
		                break;
	                  _sleep(WAIT_TIME);
					}
					fclose(fdata);
#endif
					DeleteFile(mTemp);
#else
					MoveFileEx(mTemp, pContext->mFnData, MOVEFILE_WRITE_THROUGH);
#endif
#else
                    MoveFile( mTemp, pContext->mFnData );
#endif
#else
			        _unlink(pContext->mFnData);
                    rename( mTemp, pContext->mFnData );
#endif
				  }
				  break;
		}
	    if (((pVirus_Table+nVirus[0])->nLevel % 10) == 1) {
	 	  // ヒットしたフィルタ条件別のフォルダへの保存の場合
		  if ((pVirus_Table+nVirus[0])->mFolder[0]) { // 
			strcpy(mTemp, (pVirus_Table+nVirus[0])->mFolder); 
	        tmp = strstr(mTemp,":\\");
	        if (tmp)
	          tmp = strstr(tmp+2,"\\");
#ifdef REGTOFILE
            else if ((tmp = strstr(mTemp,"\\\\"))) {
               if ((tmp = strstr(tmp+2,"\\")))
                 tmp = strstr(tmp+1,"\\");
			}
#endif
            while(tmp) {
              *tmp = '\x0';
	          _mkdir(mTemp);         // 処理用フォルダ作成
              *tmp = '\\';
	          tmp = strstr(tmp+1,"\\");
			}
            _mkdir(mTemp);         // 処理用フォルダ作成
#ifdef UPDATE_20071204  // メッセージＩＤ採番処理を修正(Bym10id)
		    sprintf(mTemp, "%s\\%s.MSG", (pVirus_Table+nVirus[0])->mFolder, pContext->mMsgId);
            CopyFile( pContext->mFnData, mTemp, TRUE);
		    sprintf(mTemp, "%s\\%s.RCP", (pVirus_Table+nVirus[0])->mFolder, pContext->mMsgId);
            CopyFile( pContext->mFnHead, mTemp, TRUE);
#else
		    sprintf(mTemp, "%s\\B%010lu.MSG", (pVirus_Table+nVirus[0])->mFolder, pContext->nMsgId);
            CopyFile( pContext->mFnData, mTemp, TRUE);
		    sprintf(mTemp, "%s\\B%010lu.RCP", (pVirus_Table+nVirus[0])->mFolder, pContext->nMsgId);
            CopyFile( pContext->mFnHead, mTemp, TRUE);
#endif
		  } else {
	 	    // デフォルト(viruslog)へのデータバックアップ
#ifdef UPDATE_20071204  // メッセージＩＤ採番処理を修正(Bym10id)
		    sprintf(mTemp, "%sviruslog\\%s.MSG", mMailSpoolDir, pContext->mMsgId);
            CopyFile( pContext->mFnData, mTemp, TRUE);
		    sprintf(mTemp, "%sviruslog\\%s.RCP", mMailSpoolDir, pContext->mMsgId);
            CopyFile( pContext->mFnHead, mTemp, TRUE);
#else
		    sprintf(mTemp, "%sviruslog\\B%010lu.MSG", mMailSpoolDir, pContext->nMsgId);
            CopyFile( pContext->mFnData, mTemp, TRUE);
		    sprintf(mTemp, "%sviruslog\\B%010lu.RCP", mMailSpoolDir, pContext->nMsgId);
            CopyFile( pContext->mFnHead, mTemp, TRUE);
#endif
		  }
		  ftmp = fopen(mTemp, "at");
		  if (ftmp) {
		    fprintf(ftmp, "\n");
		    for (i = 0; i < 5; i++) {
		      strtok((pVirus_Table+nVirus[0])->mWarning[i], "\r\n");
		      fprintf(ftmp, "%s\n", (pVirus_Table+nVirus[0])->mWarning[i]);
			 }
		     fclose(ftmp);
		  }
		}
	  }
	  LocalUnlock(hVirus);
	}
    if (LocalFlags(pVirus_Table) != LMEM_INVALID_HANDLE) // 解放可能かどうか確認
	{
#ifdef UPDATE_20080929 // メールフィルタ未定義の時に取得メモリ領域の開放抜け
	  if (hVirus)
	  {
	    LocalFree(hVirus);
	  }
#else
      LocalFree(pVirus_Table);
#endif
	}
#ifdef UPDATE_20080929 // メールフィルタ未定義の時に取得メモリ領域の開放抜け
	else {
	  if (hVirus)
	  {
	    LocalFree(hVirus);
	  }
	}
#endif

  }
  return bsts;
}

DWORD VirusCheck(PSmtpContext pContext, struct VIRUS_TABLE *Virus_Table, DWORD nVirus) {
  FILE *fdata;
#ifdef UPDATE_20050914  // base64メールの解凍
  BOOL b64 = FALSE;
  char mb64[2048];
#endif
  char mwork[2048], mSub[2048], mPack[512], *pw, *pt, *pm, *pl;
  BOOL bhead = TRUE;
  BOOL bDot[2];
  int  nbody = 0;
  DWORD i, nHit = -1;
  DWORD nHeadNo = 0; // 1:Received, 2:Subject, 3:From, 4:To, 5:Content-Type, 6:X-Mailer, 7:X-UIDL
#ifdef UPDATE_20050819
  BOOL bReceived = FALSE;
  BOOL bSubject = FALSE;
  BOOL bFrom = FALSE;
  BOOL bTo = FALSE;
  BOOL bContentType = FALSE;
  BOOL bContentTransferEncoding = FALSE;
  BOOL bXMailer = FALSE;
  BOOL bXUIDL = FALSE;
#ifdef ADDED_FILTER_UNIQUE   // 予約済み以外のヘッダのチェックオプションを追加する
  BOOL bUNIQUE= FALSE;
#endif
  BOOL bBody = FALSE;
#endif
#ifdef UPDATE_20051002  // WSP(ヘッダーのホワイトスペース）除去
  char *pwsp;
#endif
#ifdef ADDED_FILTER_RBL   // 本文内のURLを指定したRBLへ問い合わせるオプションを追加する 例 [SPAM !!]
  char mFLine[4096];   // 前の行のバックアップ
#ifdef UPDATE_20121212 // メールフィルタのRBL問合せで"Content-Type:"ヘッダの条件と組合せ可能にした。
  char mFLine2[4096];  
  BOOL bTextType = TRUE;
#endif
#endif
#ifdef UPDATE_20080423  // 対象ヘッダが複数行にまたがる#blank処理の判定
  BOOL bEnd = FALSE;
#endif
#ifdef UPDATE_20141023  // URIBL問合せの効率化
  DWORD  nErr;
  char mFnURIBL[_MAX_PATH+1];
  //char mLog[1024];
#endif
#ifdef UPDATE_20220706 // 本文にTO:トークンに続くメールアドレスがある場合、FROMヘッダと一致するか否かの判定（EMOTET対策フィルタ）
  BOOL bEmoChk = FALSE;
  char *pd, mwork2[512], mFROM[1024];
#endif

#ifdef ADDED_FILTER_RBL   // 本文内のURLを指定したRBLへ問い合わせるオプションを追加する 例 [SPAM !!]
   mFLine[0] = '\x0';   // 前の行のバックアップ
#ifdef UPDATE_20121212 // メールフィルタのRBL問合せで"Content-Type:"ヘッダの条件と組合せ可能にした。
   mFLine2[0] = '\x0';  
#endif
#endif

  if (bDebug) printf("start Filter Check\n");
  for (i = 0; i < nVirus; i++) {
    (Virus_Table+i)->bReceived[1]    = FALSE;
    (Virus_Table+i)->bSubject[1]     = FALSE;
    (Virus_Table+i)->bFrom[1]        = FALSE;
    (Virus_Table+i)->bTo[1]          = FALSE;
	(Virus_Table+i)->bContentType[1] = FALSE;
	(Virus_Table+i)->bContentTransferEncoding[1] = FALSE;
    (Virus_Table+i)->bXMailer[1]     = FALSE;
    (Virus_Table+i)->bXUIDL[1]       = FALSE;
#ifdef ADDED_FILTER_UNIQUE   // 予約済み以外のヘッダのチェックオプションを追加する
    (Virus_Table+i)->bUNIQUE[1]      = FALSE;
#endif
    (Virus_Table+i)->bBody[1]        = FALSE;
#ifdef ADDED_FILTER_RBL   // 本文内のURLを指定したRBLへ問い合わせるオプションを追加する 例 [SPAM !!]
    (Virus_Table+i)->bRBL[1]         = FALSE;
#endif
#ifdef ADDED_WILDCARD_SEARCH // 大小文字＆ワイルドカード指定でトークン比較する機能 0:無効 1:有効
    (Virus_Table+i)->nComp2[1]       = 0;
#endif
  }
#ifdef UPDATE_20141023  // URIBL問合せの効率化
  sprintf(mFnURIBL, "%s.uri", pContext->mFnData);
  DeleteFile(mFnURIBL);
#endif

  fdata = fopen(pContext->mFnData, "rt");
  if (fdata) {
#ifdef DATA_CASH
    setvbuf( fdata, pContext->mFrbuf, _IOFBF, sizeof(pContext->mFrbuf) );
#endif
    while(!feof(fdata)) {
	  mwork[0] ='\x0';
  	  if (fgets( mwork, sizeof(mwork), fdata ) == NULL)
		break;
#ifdef UPDATE_20180529A // Comp:1オプションで最後尾カラムのワイルドカード指定があると処理できない
	  strtok(mwork, "\r\n");
#endif
	  if (bhead)
		if (!(mwork[0] == ' ' || mwork[0] == '\t')) { //新しいヘッダキーなら
		  nHeadNo = 0;
#ifdef UPDATE_20080423  // 対象ヘッダが複数行にまたがる#blank処理の判定
        //bEnd = FALSE;
        for (i = 0; i < nVirus; i++) { // 全ての比較後、一致のフィルタ名を検出フィルタ順の若い方が優先度高くなる
	      if (i < nVirus) {
            if ((!(Virus_Table+i)->bReceived[0]   || (Virus_Table+i)->bReceived[1]) &&
               (!(Virus_Table+i)->bSubject[0]     || (Virus_Table+i)->bSubject[1]) &&
               (!(Virus_Table+i)->bFrom[0]        || (Virus_Table+i)->bFrom[1]) &&
               (!(Virus_Table+i)->bTo[0]          || (Virus_Table+i)->bTo[1]) &&
               (!(Virus_Table+i)->bContentType[0] || (Virus_Table+i)->bContentType[1]) &&
               (!(Virus_Table+i)->bContentTransferEncoding[0] || (Virus_Table+i)->bContentTransferEncoding[1]) &&
               (!(Virus_Table+i)->bXMailer[0]     || (Virus_Table+i)->bXMailer[1]) &&
               (!(Virus_Table+i)->bXUIDL[0]       || (Virus_Table+i)->bXUIDL[1]) &&
#ifdef ADDED_FILTER_RBL   // 本文内のURLを指定したRBLへ問い合わせるオプションを追加する 例 [SPAM !!]
               (!(Virus_Table+i)->bRBL[0]         || (Virus_Table+i)->bRBL[1]) &&
#endif
#ifdef ADDED_FILTER_UNIQUE   // 予約済み以外のヘッダのチェックオプションを追加する
               (!(Virus_Table+i)->bUNIQUE[0]      || (Virus_Table+i)->bUNIQUE[1]) &&
#endif
	           (!(Virus_Table+i)->bBody[0]        || (Virus_Table+i)->bBody[1]) ) {
              if (bDebug) printf("Filter check data '%s' is matched.\n", (Virus_Table+i)->mVirus);
              nHit = i;
              //bEnd = TRUE;
 	          break;
			}
		  }
		}
		//if (bEnd) {
		  //break;
		//}
#endif
		}
	  ///////////////////////////////////////////////////////////////////
	  /// ヘッダ部から本文への区切りで対象ヘッダ有無のチェック
#ifdef UPDATE_20050819
	  if (mwork[0] == '.') {
		if (mwork[1] == '\r' || mwork[1] == '\n') {
          for (i = 0; i < nVirus; i++) {
            if (!bReceived &&
               ((Virus_Table+i)->bReceived[0] && (Virus_Table+i)->mReceived[0] == '\x0') ) {
              (Virus_Table+i)->bReceived[1] = TRUE; // ヘッダ無し条件に一致
			}
            if (!bSubject &&
               ((Virus_Table+i)->bSubject[0] && (Virus_Table+i)->mSubject[0] == '\x0') ) {
              (Virus_Table+i)->bSubject[1] = TRUE; // ヘッダ無し条件に一致
			}
            if (!bFrom  &&
               ((Virus_Table+i)->bFrom[0] && (Virus_Table+i)->mFrom[0] == '\x0') ) {
              (Virus_Table+i)->bFrom[1] = TRUE; // ヘッダ無し条件に一致
			}
            if (!bTo  &&
               ((Virus_Table+i)->bTo[0] && (Virus_Table+i)->mTo[0] == '\x0') ) {
              (Virus_Table+i)->bTo[1] = TRUE; // ヘッダ無し条件に一致
			}
            if (!bContentType  &&
               ((Virus_Table+i)->bContentType[0] && (Virus_Table+i)->mContentType[0] == '\x0') ) {
              (Virus_Table+i)->bContentType[1] = TRUE; // ヘッダ無し条件に一致
			}
            if (!bContentTransferEncoding  &&
               ((Virus_Table+i)->bContentTransferEncoding[0] && (Virus_Table+i)->mContentTransferEncoding[0] == '\x0') ) {
              (Virus_Table+i)->bContentTransferEncoding[1] = TRUE; // ヘッダ無し条件に一致
			}
            if (!bXMailer  &&
               ((Virus_Table+i)->bXMailer[0] && (Virus_Table+i)->mXMailer[0] == '\x0') ) {
              (Virus_Table+i)->bXMailer[1] = TRUE; // ヘッダ無し条件に一致
			}
            if (!bXUIDL  &&
               ((Virus_Table+i)->bXUIDL[0] && (Virus_Table+i)->mXUIDL[0] == '\x0') ) {
              (Virus_Table+i)->bXUIDL[1] = TRUE; // ヘッダ無し条件に一致
			}
#ifdef ADDED_FILTER_UNIQUE   // 予約済み以外のヘッダのチェックオプションを追加する
            if (!bUNIQUE  &&
               ((Virus_Table+i)->bUNIQUE[0] && (Virus_Table+i)->mUNIQUE[0] == '\x0') ) {
              (Virus_Table+i)->bUNIQUE[1] = TRUE; // ヘッダ無し条件に一致
			}
#endif
		  }
		}
	  }
#endif
	  if (bhead && (strlen(mwork) == 1 && mwork[0] == '\n') ||
		  (strlen(mwork) == 2 && mwork[0] == '\r' && mwork[1] == '\n')) {
	    nHeadNo = 0;
        bhead = FALSE;
	  }
	  // 本文チェックがいらないなら終了 2001.09.13
	  //if (!Virus_Table.bBody && !bhead)  // チェック処理の高速化
		//break;
	  /////////////////////////////////////////////
//printf("%s\n", mwork);
      for (i = 0; i < nVirus; i++) {
//printf("[%d] ", i);
        if (bhead) {
		  if (mwork[0] == ' ' || mwork[0] == '\t')
			pw = &mwork[0];
		  else
	        pw = strstr(mwork,":");
	      if (pw) {
			if (*pw == ':') {
#ifdef UPDATE_20051002  // WSP(ヘッダーのホワイトスペース）除去
              pwsp = pw;
#endif
		      *pw = '\x0';
			  pw++;
			}
		    if ((_stricmp(mwork,"received") == 0 || (nHeadNo == 1 && (mwork[0] == ' ' || mwork[0] == '\t')) ) &&
			    ( (Virus_Table+i)->bReceived[0] || (Virus_Table+i)->mReceived[0] != '\x0') ) {
#ifdef UPDATE_20080423  // 対象ヘッダが複数行にまたがる#blank処理の判定
			  if ((Virus_Table+i)->mReceived[0] == '\x0') {
                (Virus_Table+i)->bReceived[1] = FALSE;   // データ空白なら拒否対象
			  }
#endif
#ifdef UPDATE_20051002  // WSP(ヘッダーのホワイトスペース）除去
		      while(*pw == ' ' || *pw == '\t') // ホワイトスペース除去
			    pw++;
#endif
#ifdef UPDATE_20050819
              bReceived = TRUE;
#endif
			  nHeadNo = 1;
		      //(Virus_Table+i)->bReceived[1] = FALSE;
#ifdef UPDATE_20050214
			  if (strstr((Virus_Table+i)->mReceived, "#M-ID#")) {
				pt = pw;
				while(*pt == ' ' || *pt == '\t') // ホワイトスペース除去
				  pt++;
				if (!strnicmp(pt, "from ", 5)) {
				  pt+=5;
				  if ((pm = strchr(pt, ' ')))
					*pm = '\x0';
				  bDot[0] = bDot[1] = FALSE;
				  if (strchr(pt, '.'))
					bDot[0] = TRUE;
				  if (strchr(pContext->mMessIDD, '.'))
					bDot[1] = TRUE;
				  if (pt[0] && 
					  pContext->mMessIDD[0] &&
					  bDot[0] == bDot[1] &&
					           (strstr(pContext->mMessIDD, pt) ||
					            strstr(pt, pContext->mMessIDD)) ) {
			        (Virus_Table+i)->bReceived[1] = TRUE; // データ一致なら処理対象
				  } 
				  if (pm)
					*pm = ' '; // 元に戻す
				}
			  } else
#endif
			  {
			    if ((Virus_Table+i)->mReceived[0] != '\x0') {
#ifdef ADDED_WILDCARD_SEARCH // 大小文字＆ワイルドカード指定でトークン比較する機能 0:無効 1:有効
				  if ((Virus_Table+i)->nComp2[0] == 1) 
	 		        pt = wildcard_stricmp((Virus_Table+i)->mReceived, pw);
				   else
#endif
	 		      pt = strstr(pw, (Virus_Table+i)->mReceived);
			      if (pt)
			        (Virus_Table+i)->bReceived[1] = TRUE; // データ一致なら拒否対象
				} else if (*pw == '\x0' || *pw == '\r' || *pw == '\n') {
		          (Virus_Table+i)->bReceived[1] = TRUE;   // データ空白なら拒否対象
				}
			  }
			} else if ((_stricmp(mwork,"subject") == 0 || (nHeadNo == 2 && (mwork[0] == ' ' || mwork[0] == '\t')) ) &&
			           ( (Virus_Table+i)->bSubject[0] || (Virus_Table+i)->mSubject[0] != '\x0') ) {
#ifdef UPDATE_20080423  // 対象ヘッダが複数行にまたがる#blank処理の判定
			  if ((Virus_Table+i)->mSubject[0] == '\x0') {
                (Virus_Table+i)->bSubject[1] = FALSE;   // データ空白なら拒否対象
			  }
#endif
#ifdef UPDATE_20051002  // WSP(ヘッダーのホワイトスペース）除去
		      while(*pw == ' ' || *pw == '\t') // ホワイトスペース除去
			    pw++;
#endif
#ifdef UPDATE_20050819
              bSubject = TRUE;
#endif
			  nHeadNo = 2;
		      //(Virus_Table+i)->bSubject[1] = FALSE;
			  if ((Virus_Table+i)->mSubject[0] != '\x0') {
#ifdef UPDATE_20050516
				pl = pw; // 退避
#ifdef UPDATE_20050107
				LineConv(mSub, sizeof(mSub), pw , mPack);
#else
				LineConv(mSub, pw , mPack);
#endif
				if (mPack[0])
				  pw = mSub;
#endif
#ifdef ADDED_WILDCARD_SEARCH // 大小文字＆ワイルドカード指定でトークン比較する機能 0:無効 1:有効
			    if ((Virus_Table+i)->nComp2[0] == 1) 
	 		      pt = wildcard_stricmp((Virus_Table+i)->mSubject, pw);
				 else
#endif
			    pt = strstr(pw, (Virus_Table+i)->mSubject);
			    if (pt)
			      (Virus_Table+i)->bSubject[1] = TRUE; // データ一致なら拒否対象
#ifdef UPDATE_20050516
				pw = pl; // 復帰
#endif
			  } else if (*pw == '\x0' || *pw == '\r' || *pw == '\n') {
		        (Virus_Table+i)->bSubject[1] = TRUE;   // データ空白なら拒否対象
			  }
			} else if ((_stricmp(mwork,"from") == 0 || (nHeadNo == 3 && (mwork[0] == ' ' || mwork[0] == '\t')) ) &&
			           ( (Virus_Table+i)->bFrom[0] || (Virus_Table+i)->mFrom[0] != '\x0') ) {
#ifdef UPDATE_20080423  // 対象ヘッダが複数行にまたがる#blank処理の判定
			  if ((Virus_Table+i)->mFrom[0] == '\x0') {
                (Virus_Table+i)->bFrom[1] = FALSE;   // データ空白なら拒否対象
			  }
#endif
#ifdef UPDATE_20051002  // WSP(ヘッダーのホワイトスペース）除去
		      while(*pw == ' ' || *pw == '\t') // ホワイトスペース除去
			    pw++;
#endif
#ifdef UPDATE_20050819
              bFrom = TRUE;
#endif
			  nHeadNo = 3;
#ifdef UPDATE_20050214
			  if (strstr((Virus_Table+i)->mFrom, "#M-ID#")) {
				if ((pt = strchr(pw, '@')))
                  pt++;
				if (pt) {
				  if ((pm = strchr(pt, '>')))
					*pm = '\x0';
				  bDot[0] = bDot[1] = FALSE;
				  if (strchr(pt, '.'))
					bDot[0] = TRUE;
				  if (strchr(pContext->mMessIDD, '.'))
					bDot[1] = TRUE;
			      if (pt[0] &&
					  pContext->mMessIDD[0] &&
					  bDot[0] == bDot[1] &&
					         (strstr(pContext->mMessIDD, pt) ||
				              strstr(pt, pContext->mMessIDD)) ) {
			        (Virus_Table+i)->bFrom[1] = TRUE; // データ一致なら処理対象
				  }
				  if (pm)
					*pm = '>';
				}
			  } else
#endif
			  {
			    if ((Virus_Table+i)->mFrom[0] != '\x0') {
#ifdef UPDATE_20050528
				pl = pw; // 退避
				LineConv(mSub, sizeof(mSub), pw , mPack);
				if (mPack[0])
				  pw = mSub;
#endif
#ifdef ADDED_ENVELOPE_UNMATCH // From:ヘッダの記載アドレスにエンベロープの送信元が含まれているか確認（メールフィルタ用）
				if (!strcmp((Virus_Table+i)->mFrom, "#ENVELOPE_UNMATCH#")) {
				   pt = !strstr(pw, pContext->mUIDFROM);
				} else
#endif
#ifdef ADDED_WILDCARD_SEARCH // 大小文字＆ワイルドカード指定でトークン比較する機能 0:無効 1:有効
			    if ((Virus_Table+i)->nComp2[0] == 1) 
	 		      pt = wildcard_stricmp((Virus_Table+i)->mFrom, pw);
				else
#endif
			      pt = strstr(pw, (Virus_Table+i)->mFrom);
			    if (pt)
			      (Virus_Table+i)->bFrom[1] = TRUE; // データ一致なら拒否対象
#ifdef UPDATE_20220706 // 本文にTO:トークンに続くメールアドレスがある場合、FROMヘッダと一致するか否かの判定（EMOTET対策フィルタ）
				 if (pd = strchr(pw, '<'))
                   strcpy(mFROM, pd+1);
				 else 
				 {
				   pd = pw;
				   while(*pd == ' ' || *pd == '\t')
				   {
					 pd++;
				   }
				   strcpy(mFROM, pd);
				 }
				 strtok(mFROM, "\r\n\t>");
#endif
#ifdef UPDATE_20050528
				pw = pl; // 復帰
#endif
				} else if (*pw == '\x0' || *pw == '\r' || *pw == '\n') {
		          (Virus_Table+i)->bFrom[1] = TRUE;   // データ空白なら拒否対象
				}
			  }
			} else if ((_stricmp(mwork,"to") == 0 || (nHeadNo == 4 && (mwork[0] == ' ' || mwork[0] == '\t')) ) &&
			           ( (Virus_Table+i)->bTo[0] || (Virus_Table+i)->mTo[0] != '\x0') ) {
#ifdef UPDATE_20080423  // 対象ヘッダが複数行にまたがる#blank処理の判定
			  if ((Virus_Table+i)->mTo[0] == '\x0') {
                (Virus_Table+i)->bTo[1] = FALSE;   // データ空白なら拒否対象
			  }
#endif
#ifdef UPDATE_20051002  // WSP(ヘッダーのホワイトスペース）除去
		      while(*pw == ' ' || *pw == '\t') // ホワイトスペース除去
			    pw++;
#endif
#ifdef UPDATE_20050819
              bTo = TRUE;
#endif
			  nHeadNo = 4;
		      //(Virus_Table+i)->bTo[1] = FALSE;
			  if ((Virus_Table+i)->mTo[0] != '\x0') {
#ifdef ADDED_WILDCARD_SEARCH // 大小文字＆ワイルドカード指定でトークン比較する機能 0:無効 1:有効
			    if ((Virus_Table+i)->nComp2[0] == 1) 
	 		      pt = wildcard_stricmp((Virus_Table+i)->mTo, pw);
				 else
#endif
			    pt = strstr(pw, (Virus_Table+i)->mTo);
			    if (pt)
			      (Virus_Table+i)->bTo[1] = TRUE; // データ一致なら拒否対象
			  } else if (*pw == '\x0' || *pw == '\r' || *pw == '\n') {
		        (Virus_Table+i)->bTo[1] = TRUE;   // データ空白なら拒否対象
			  }
			} else if ((_stricmp(mwork,"content-type") == 0 || (nHeadNo == 5 && (mwork[0] == ' ' || mwork[0] == '\t')) ) &&
			           ( (Virus_Table+i)->bContentType[0] || (Virus_Table+i)->mContentType[0] != '\x0') ) {
#ifdef UPDATE_20080423  // 対象ヘッダが複数行にまたがる#blank処理の判定
			  if ((Virus_Table+i)->mContentType[0] == '\x0') {
                (Virus_Table+i)->bContentType[1] = FALSE;   // データ空白なら拒否対象
			  }
#endif
#ifdef UPDATE_20051002  // WSP(ヘッダーのホワイトスペース）除去
		      while(*pw == ' ' || *pw == '\t') // ホワイトスペース除去
			    pw++;
#endif
#ifdef UPDATE_20050819
              bContentType = TRUE;
#endif
			  nHeadNo = 5;
		      //(Virus_Table+i)->bContentType[1] = FALSE;
			  if ((Virus_Table+i)->mContentType[0] != '\x0') {
#ifdef ADDED_WILDCARD_SEARCH // 大小文字＆ワイルドカード指定でトークン比較する機能 0:無効 1:有効
			    if ((Virus_Table+i)->nComp2[0] == 1) 
	 		      pt = wildcard_stricmp((Virus_Table+i)->mContentType, pw);
				 else
#endif
			    pt = strstr(pw, (Virus_Table+i)->mContentType);
			    if (pt)
			      (Virus_Table+i)->bContentType[1] = TRUE; // データ一致なら拒否対象
			  } else if (*pw == '\x0' || *pw == '\r' || *pw == '\n') {
		        (Virus_Table+i)->bContentType[1] = TRUE;   // データ空白なら拒否対象
			  }
			} else if ((_stricmp(mwork,"x-mailer") == 0 || (nHeadNo == 6 && (mwork[0] == ' ' || mwork[0] == '\t')) ) &&
			           ( (Virus_Table+i)->bXMailer[0] || (Virus_Table+i)->mXMailer[0] != '\x0') ) {
#ifdef UPDATE_20080423  // 対象ヘッダが複数行にまたがる#blank処理の判定
			  if ((Virus_Table+i)->mXMailer[0] == '\x0') {
                (Virus_Table+i)->bXMailer[1] = FALSE;   // データ空白なら拒否対象
			  }
#endif
#ifdef UPDATE_20051002  // WSP(ヘッダーのホワイトスペース）除去
		      while(*pw == ' ' || *pw == '\t') // ホワイトスペース除去
			    pw++;
#endif
#ifdef UPDATE_20050819
              bXMailer = TRUE;
#endif
			  nHeadNo = 6;
		      //(Virus_Table+i)->bXMailer[1] = FALSE;
			  if ((Virus_Table+i)->mXMailer[0] != '\x0') {
#ifdef ADDED_WILDCARD_SEARCH // 大小文字＆ワイルドカード指定でトークン比較する機能 0:無効 1:有効
			    if ((Virus_Table+i)->nComp2[0] == 1) 
	 		      pt = wildcard_stricmp((Virus_Table+i)->mXMailer, pw);
				 else
#endif
			    pt = strstr(pw, (Virus_Table+i)->mXMailer);
			    if (pt)
			      (Virus_Table+i)->bXMailer[1] = TRUE; // データ一致なら拒否対象
			  } else if (*pw == '\x0' || *pw == '\r' || *pw == '\n') {
		        (Virus_Table+i)->bXMailer[1] = TRUE;   // データ空白なら拒否対象
			  }
			} else if ((_stricmp(mwork,"x-uidl") == 0 || (nHeadNo == 7 && (mwork[0] == ' ' || mwork[0] == '\t')) ) &&
			           ( (Virus_Table+i)->bXUIDL[0] || (Virus_Table+i)->mXUIDL[0] != '\x0') ) {
#ifdef UPDATE_20080423  // 対象ヘッダが複数行にまたがる#blank処理の判定
			  if ((Virus_Table+i)->mXUIDL[0] == '\x0') {
                (Virus_Table+i)->bXUIDL[1] = FALSE;   // データ空白なら拒否対象
			  }
#endif
#ifdef UPDATE_20051002  // WSP(ヘッダーのホワイトスペース）除去
		      while(*pw == ' ' || *pw == '\t') // ホワイトスペース除去
			    pw++;
#endif
#ifdef UPDATE_20050819
              bXUIDL = TRUE;
#endif
			  nHeadNo = 7;
		      //(Virus_Table+i)->bXUIDL[1] = FALSE;
			  if ((Virus_Table+i)->mXUIDL[0] != '\x0') {
#ifdef ADDED_WILDCARD_SEARCH // 大小文字＆ワイルドカード指定でトークン比較する機能 0:無効 1:有効
			    if ((Virus_Table+i)->nComp2[0] == 1) 
	 		      pt = wildcard_stricmp((Virus_Table+i)->mXUIDL, pw);
				 else
#endif
			    pt = strstr(pw, (Virus_Table+i)->mXUIDL);
			    if (pt)
			      (Virus_Table+i)->bXUIDL[1] = TRUE; // データ一致なら拒否対象
			  } else if (*pw == '\x0' || *pw == '\r' || *pw == '\n') {
		        (Virus_Table+i)->bXUIDL[1] = TRUE;   // データ空白なら拒否対象
			  }
			} else if ((_stricmp(mwork,"content-transfer-encoding") == 0 || (nHeadNo == 8 && (mwork[0] == ' ' || mwork[0] == '\t')) ) &&
			           ( (Virus_Table+i)->bContentTransferEncoding[0] || (Virus_Table+i)->mContentTransferEncoding[0] != '\x0') ) {
#ifdef UPDATE_20080423  // 対象ヘッダが複数行にまたがる#blank処理の判定
			  if ((Virus_Table+i)->mContentTransferEncoding[0] == '\x0') {
                (Virus_Table+i)->bContentTransferEncoding[1] = FALSE;   // データ空白なら拒否対象
			  }
#endif
#ifdef UPDATE_20051002  // WSP(ヘッダーのホワイトスペース）除去
		      while(*pw == ' ' || *pw == '\t') // ホワイトスペース除去
			    pw++;
#endif
#ifdef UPDATE_20050819
              bContentTransferEncoding = TRUE;
#endif
			  nHeadNo = 8;
		      //(Virus_Table+i)->bContentTransferEncoding[1] = FALSE;
			  if ((Virus_Table+i)->mContentTransferEncoding[0] != '\x0') {
#ifdef ADDED_WILDCARD_SEARCH // 大小文字＆ワイルドカード指定でトークン比較する機能 0:無効 1:有効
			    if ((Virus_Table+i)->nComp2[0] == 1) 
	 		      pt = wildcard_stricmp((Virus_Table+i)->mContentTransferEncoding, pw);
				 else
#endif
			    pt = strstr(pw, (Virus_Table+i)->mContentTransferEncoding);
			    if (pt)
			      (Virus_Table+i)->bContentTransferEncoding[1] = TRUE; // データ一致なら拒否対象
			  } else if (*pw == '\x0' || *pw == '\r' || *pw == '\n') {
		        (Virus_Table+i)->bContentTransferEncoding[1] = TRUE;   // データ空白なら拒否対象
			  }
#ifdef ADDED_FILTER_UNIQUE   // 予約済み以外のヘッダのチェックオプションを追加する
			} else if ((_stricmp(mwork, (Virus_Table+i)->mUNIQUEHEADER) == 0 || (nHeadNo == 9 && (mwork[0] == ' ' || mwork[0] == '\t')) ) &&
			           ( (Virus_Table+i)->mUNIQUE[0] || (Virus_Table+i)->mUNIQUE[0] != '\x0') ) {
#ifdef UPDATE_20080423  // 対象ヘッダが複数行にまたがる#blank処理の判定
			  if ((Virus_Table+i)->mUNIQUE[0] == '\x0') {
                (Virus_Table+i)->bUNIQUE[1] = FALSE;   // データ空白なら拒否対象
			  }
#endif
#ifdef UPDATE_20051002  // WSP(ヘッダーのホワイトスペース）除去
		      while(*pw == ' ' || *pw == '\t') // ホワイトスペース除去
			    pw++;
#endif
#ifdef UPDATE_20050819
              bUNIQUE = TRUE;
#endif
			  nHeadNo = 9;
		      //(Virus_Table+i)->bContentTransferEncoding[1] = FALSE;
			  if ((Virus_Table+i)->mUNIQUE[0]!= '\x0') {
#ifdef ADDED_WILDCARD_SEARCH // 大小文字＆ワイルドカード指定でトークン比較する機能 0:無効 1:有効
			    if ((Virus_Table+i)->nComp2[0] == 1) 
#ifdef UPDATE_20180529 // メールフィルタ機能強化オプション使用時でユニークヘッダにワイルドカード指定を行うとハングする不具合。
	 		      pt = wildcard_stricmp((Virus_Table+i)->mUNIQUE, pw);
#else
	 		      pt = wildcard_stricmp((Virus_Table+i)->mUNIQUE[0], pw);
#endif
				 else
#endif
			    pt = strstr(pw, (Virus_Table+i)->mUNIQUE);
			    if (pt)
			      (Virus_Table+i)->bUNIQUE[1] = TRUE; // データ一致なら拒否対象
			  } else if (*pw == '\x0' || *pw == '\r' || *pw == '\n') {
		        (Virus_Table+i)->bUNIQUE[1] = TRUE;   // データ空白なら拒否対象
			  }
#endif
			}
			if (mwork[0] != ' ' && mwork[0] != '\t') {
#ifdef UPDATE_20051002  // WSP(ヘッダーのホワイトスペース）除去
              *pwsp = ':';
#else
		      *(pw-1) = ':';
#endif
			}
		  }
		} else {
#ifdef UPDATE_20050819
          bBody = TRUE;
#endif
//body:
		  ////////////////////////////////////////////////
	      // データの改行を取除く
		  if (mwork[0] == '\r' || mwork[0] == '\n')
		    mwork[0] = '\x0';
		  else {
		    strtok(mwork,"\r\n");
#ifdef UPDATE_20050914  // base64メールの解凍
			mb64[0] = '\x0';
			b64 = CheckB64Line(mwork); //B64コードであることをチェック
			if (b64) {//B64コードであることをチェック
	          Base64DecodeLine(mwork, strlen(mwork), mb64, sizeof(mb64));
              //if (bDebug) printf("[base64] \"%s\"\n", mb64);
			}
#endif
		  }
#ifdef ADDED_FILTER_RBL   // 本文内のURLを指定したRBLへ問い合わせるオプションを追加する 例 [SPAM !!]
		  if (i == 0)
            strcat(mFLine, (b64 ? mb64 : mwork));   // 前の行のバックアップ
#endif
		  ////////////////////////////////////////////////
	      // 本文チェックがいらないなら次のフィルタチェックへ
#ifdef ADDED_FILTER_RBL   // 本文内のURLを指定したRBLへ問い合わせるオプションを追加する 例 [SPAM !!]
		  if (!((Virus_Table+i)->bBody[0] || (Virus_Table+i)->bRBL[0])) // チェック処理の高速化
#else
		  if (!(Virus_Table+i)->bBody[0]) // チェック処理の高速化
#endif
		  {
		    continue;
		  }
          //(Virus_Table+i)->bBody[1] = FALSE;
  		  //if ((Virus_Table+i)->mBody[(Virus_Table+i)->nBody[1]][0] != '\x0') {
#ifdef ADDED_FILTER_RBL   // 本文内のURLを指定したRBLへ問い合わせるオプションを追加する 例 [SPAM !!]
		  if ((Virus_Table+i)->mRBL[0]) {
#ifdef UPDATE_20121212 // メールフィルタのRBL問合せで"Content-Type:"ヘッダの条件と組合せ可能にした。
			if ((Virus_Table+i)->mContentType[0])
			{
              strncpy(mFLine2, mFLine, sizeof(mFLine2)-1);
			  mFLine2[sizeof(mFLine2)-1] = '\x0';
			  strlwr(mFLine2);
			  if (!strnicmp(mFLine2, "content-type:", 13))
			  {
			    if (strstr(&mFLine2[13], (Virus_Table+i)->mContentType))
				{
			      bTextType = TRUE;
#ifdef UPDATE_20130221 // メールフィルタのRBL問合せ時にContent-typeを指定するとでMIME形式の時の処理が正常になっていない
				  (Virus_Table+i)->bContentType[1] = TRUE;
#endif
				} else {
				  bTextType = FALSE;
				}
			  }
			}
			if (bTextType)
#endif
			{
#ifdef UPDATE_20141023  // URIBL問合せの効率化
		      if (Query_FILTER_RBL((Virus_Table+i)->mRBL, mFLine, mFnURIBL)) // RBLに存在するなら
#else
		      if (Query_FILTER_RBL((Virus_Table+i)->mRBL, mFLine )) // RBLに存在するなら
#endif
			  {
 	            (Virus_Table+i)->bRBL[1] = TRUE;
			  }
			}
		  }
		  if ((Virus_Table+i)->bBody[0]) {
#endif
#ifdef ADDED_WILDCARD_SEARCH // 大小文字＆ワイルドカード指定でトークン比較する機能 0:無効 1:有効
			if ((Virus_Table+i)->nComp2[0] == 1) {
#ifdef UPDATE_20220706 // 本文にTO:トークンに続くメールアドレスがある場合、FROMヘッダと一致するか否かの判定（EMOTET対策フィルタ）
			  if (!strcmp((Virus_Table+i)->mBody[(Virus_Table+i)->nBody[1]], "#COMPARE_FROM_VS_BODY_EMAIL#")) 
			  {
				if (!bEmoChk && _stristr(mwork,"@"))
				{
				  //bEmoChk = TRUE;
				  //if (pd = strchr(pContext->mUIDFROM, '@'))
				  strcpy(mwork2, mFROM);
				  /*
				  if (pd = strchr(mFROM, '@'))
				  {
					strcpy(mwork2, pd+1);
				  } else {
				    strcpy(mwork2, mFROM);
				  }
				  */
				  if (mwork2[0] =='\x0' || _stristr( mwork, mwork2)) // 送信元エンベロープが空白または、本文の最初に見つかるメールアドレスと一致する場合。
				  {
				    (Virus_Table+i)->nBody[1] = 0;
					(Virus_Table+i)->bBody[1] = FALSE;
					nHit = -1L;
					bEmoChk = TRUE;
				  } else {
		            (Virus_Table+i)->nBody[1] = 1;
 		            (Virus_Table+i)->bBody[1] = TRUE;
				  } 
				}
			  } else
#endif
			  {
	            if ((mwork[0] == '\x0' && (Virus_Table+i)->mBody[(Virus_Table+i)->nBody[1]][0] == '\x0')  ||
		            (wildcard_stricmp((Virus_Table+i)->mBody[(Virus_Table+i)->nBody[1]], mwork) && (Virus_Table+i)->mBody[(Virus_Table+i)->nBody[1]][0] != '\x0') ||
#ifdef UPDATE_20050914  // base64メールの解凍
  		            (b64 && wildcard_stricmp((Virus_Table+i)->mBody[(Virus_Table+i)->nBody[1]], mb64) && (Virus_Table+i)->mBody[(Virus_Table+i)->nBody[1]][0] != '\x0') 
#endif
		           ) {
		          (Virus_Table+i)->nBody[1]++;
		          if ((Virus_Table+i)->nBody[0] == (Virus_Table+i)->nBody[1]) {
 		            (Virus_Table+i)->bBody[1] = TRUE;
				  } 
				} else {
		          (Virus_Table+i)->nBody[1] = 0;
				}
			  }
			} else
#endif
			{
	          if ((mwork[0] == '\x0' && (Virus_Table+i)->mBody[(Virus_Table+i)->nBody[1]][0] == '\x0')  ||
		          (strstr(mwork, (Virus_Table+i)->mBody[(Virus_Table+i)->nBody[1]]) && (Virus_Table+i)->mBody[(Virus_Table+i)->nBody[1]][0] != '\x0') ||
#ifdef UPDATE_20050914  // base64メールの解凍
  		          (b64 && strstr(mb64, (Virus_Table+i)->mBody[(Virus_Table+i)->nBody[1]]) && (Virus_Table+i)->mBody[(Virus_Table+i)->nBody[1]][0] != '\x0') 
#endif
		         ) {
		        (Virus_Table+i)->nBody[1]++;
		        if ((Virus_Table+i)->nBody[0] == (Virus_Table+i)->nBody[1]) {
 		          (Virus_Table+i)->bBody[1] = TRUE;
				} 
			  } else {
		        (Virus_Table+i)->nBody[1] = 0;
			  }
			}
#ifdef ADDED_FILTER_RBL   // 本文内のURLを指定したRBLへ問い合わせるオプションを追加する 例 [SPAM !!]
		  }
#endif
		}
		/*  全ての条件チェックさせるため速度向上処理はしない
        if ( (!(Virus_Table+i)->bReceived[0]    || (Virus_Table+i)->bReceived[1]) &&
             (!(Virus_Table+i)->bSubject[0]     || (Virus_Table+i)->bSubject[1]) &&
             (!(Virus_Table+i)->bFrom[0]        || (Virus_Table+i)->bFrom[1]) &&
             (!(Virus_Table+i)->bTo[0]          || (Virus_Table+i)->bTo[1]) &&
             (!(Virus_Table+i)->bContentType[0] || (Virus_Table+i)->bContentType[1]) &&
             (!(Virus_Table+i)->bXMailer[0]     || (Virus_Table+i)->bXMailer[1]) &&
             (!(Virus_Table+i)->bXUIDL[0]       || (Virus_Table+i)->bXUIDL[1]) &&
	         (!(Virus_Table+i)->bBody[0]        || (Virus_Table+i)->bBody[1]) )
		   break;
		*/
	  }
#ifdef ADDED_FILTER_RBL   // 本文内のURLを指定したRBLへ問い合わせるオプションを追加する 例 [SPAM !!]
      strcpy(mFLine, (b64 ? mb64 : mwork));   // 前の行のバックアップ
#endif
#ifdef UPDATE_20070910 // URIの追加判定条件 //評価中
	  if (strlen(mFLine))
		if (mFLine[strlen(mFLine)-1] == '=')
		  mFLine[strlen(mFLine)-1] = '\x0';
#ifdef UPDATE_20070914 // URIの追加判定条件 末尾に'='がある場合のみ改行を外す。
		else {
	      strcat(mFLine, "\r\n");
		}
#endif

#endif
#ifdef UPDATE_20080423  // 対象ヘッダが複数行にまたがる#blank処理の判定
	  if (!bhead) 
#endif
	  {
        for (i = 0; i < nVirus; i++) { // 全ての比較後、一致のフィルタ名を検出フィルタ順の若い方が優先度高くなる
	      if (i < nVirus) {
            if ( (!(Virus_Table+i)->bReceived[0]    || (Virus_Table+i)->bReceived[1]) &&
               (!(Virus_Table+i)->bSubject[0]     || (Virus_Table+i)->bSubject[1]) &&
               (!(Virus_Table+i)->bFrom[0]        || (Virus_Table+i)->bFrom[1]) &&
               (!(Virus_Table+i)->bTo[0]          || (Virus_Table+i)->bTo[1]) &&
               (!(Virus_Table+i)->bContentType[0] || (Virus_Table+i)->bContentType[1]) &&
               (!(Virus_Table+i)->bContentTransferEncoding[0] || (Virus_Table+i)->bContentTransferEncoding[1]) &&
               (!(Virus_Table+i)->bXMailer[0]     || (Virus_Table+i)->bXMailer[1]) &&
               (!(Virus_Table+i)->bXUIDL[0]       || (Virus_Table+i)->bXUIDL[1]) &&
#ifdef ADDED_FILTER_RBL   // 本文内のURLを指定したRBLへ問い合わせるオプションを追加する 例 [SPAM !!]
               (!(Virus_Table+i)->bRBL[0]         || (Virus_Table+i)->bRBL[1]) &&
#endif
#ifdef ADDED_FILTER_UNIQUE   // 予約済み以外のヘッダのチェックオプションを追加する
               (!(Virus_Table+i)->bUNIQUE[0]      || (Virus_Table+i)->bUNIQUE[1]) &&
#endif
	           (!(Virus_Table+i)->bBody[0]        || (Virus_Table+i)->bBody[1]) ) {
              if (bDebug) printf("Filter check data '%s' is matched.\n", (Virus_Table+i)->mVirus);
              nHit = i;
 	          break;
			}
		  }
		}
	  }
#ifdef UPDATE_20071024 // メールフィルタ処理中にCPU100%にならない対策
	  _sleep(0);
#endif
	}
	fclose(fdata);
  } 
#ifdef UPDATE_20141023  // URIBL問合せの効率化
  while(!DeleteFile(mFnURIBL)) {
	 nErr = GetLastError();
	 if (nErr == ERROR_FILE_NOT_FOUND)
	   break;
     //sprintf(mLog, "DeleteFile(%s) Fail. code=(%d) retry delete.", mFnURIBL, nErr);
     //LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
 	 _sleep(100);
  }
#endif
  if (bDebug) printf("end Filter Check\n");
  return nHit;
}

#ifdef SHORT_SURBL // 短縮URLのSURBL判定対策
// 短縮URLの復号
BOOL Decode_Short_URL(CHAR *pLink, CHAR *pURI)
{
   BOOL bsts = FALSE;
   FILE *fp;
   CHAR *p, mURL[256], mData[512];
   CHAR mMes[512];

   if (mShortDecodeFile[0]) // ファイルの指定がある場合
   {
	 strcpy(mURL, pLink);
     if ((fp = fopen(mShortDecodeFile, "rt")))
	 {
	   while(fgets(mData, sizeof(mData), fp))
	   {
	     if (mData[0] != '\x0' &&
		     mData[0] != '\'' &&
		     mData[0] != '"' &&
		     mData[0] != ' ' &&
		     mData[0] != '\r' &&
		     mData[0] != '\n')
		 {
           strtok(mData, "\r\n");
		   if (!strnicmp(mData, mURL, strlen(mURL)))
		   {
			 strcpy(pURI, &mData[strlen(mURL)+1]);
#ifdef ACCEPT_LOG_LEVEL3
             sprintf(mMes, "Hit. Short_URL %s (%s)\n", mURL, pURI);
             LEVEL_3_ACCEPTLOG(NULL, mMes);
#endif
			 bsts = TRUE;
			 break;
		   }
		 }
	   }
	   fclose(fp);
	 }
   }
   return bsts;
}
#endif