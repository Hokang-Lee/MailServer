#include "smtp.h"
#include <direct.h>
#include <windows.h>
#include <winsock.h>
#include <string.h>
#include <sys/types.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <arpa/inet.h>
#include <arpa/nameser.h>
#include <resolv.h>
#include <windns.h>


#ifdef CLUSTERING
extern BOOL   nClustering;
extern char   mComName[];
#endif
#ifdef UPDATE_20241217 // gateway.datにSMTP認証の指定がある時は優先するようにした。
extern BOOL   bAUTHTYPE[];
extern BOOL   bGWAUTHTYPE[]; // gateway.datでのSMTP認証情報
#endif

void printfTrace(char *str);
DWORD GetTryServer(char *mDomain, char *mMsg, DWORD *nErrCode, BOOL bmode);
void nsError();
BOOL LocalNameServers();
BOOL findNameServers();
BOOL queryMXRecode();
void returnCodeError();
int  skipToData();
int  skipName();
#ifdef NEW_MXQUERY
BOOL queryMXRecode2(char *mMsgNo, char *pDomain, char *pNS, char *pMX, int *mxNum);
#endif

HANDLE	hReadWriteEvent;

#ifdef V4
extern int   nMXCashLiveTime; // MXキャッシュ利用有効時間(秒単位)
#endif
extern char  mMailSpoolDir[];
extern BOOL  bDebug;
extern int   nAddressFamily;
extern BOOL  bServiceTerminating;
extern BOOL  bWaitDNS;
extern char  mSendGateway[];
extern int   nport;            // smtp port
extern int   nSenderLog;
extern BOOL  bServerType;
extern char  mProgPath[];
#ifdef USE_SSL
extern BOOL  bUsedSSL;
#endif
#ifdef UPDATE_20070321 //DNSへの問合せテーブル初期化(初期化が無いと問合せで不定期にハングする可能性有り)
extern char  mDNSMXLockFn[];
#endif
#ifdef UPDATE_20141105 // MXレコード参照失敗した受信をSMTP Gatewayへ転送
extern BOOL  bMailForward2;
#endif

#ifdef UPDATE_20220414 // ゲートウェイ先ごとにSMTP認証ID,PWを規定する場合
char   *pAuthON, *pAuthID, *pAuthPW;
#ifdef ADD_XOAUTH2_C // GATEWAY.DATにOAUTH2のURLを指定可能に
char   mGWDAT[4096];
char   mauthB64[4096];
#ifdef UPDATE_20220418 // ゲートウェイ先ごとにSMTP認証ID,PWを規定する場合にデータを暗号化する
char   mauthSPA[4096];
#endif
#else
char   mGWDAT[1024];
char   mauthB64[1024];
#ifdef UPDATE_20220418 // ゲートウェイ先ごとにSMTP認証ID,PWを規定する場合にデータを暗号化する
char   mauthSPA[1024];
#endif
#endif
#endif

#ifdef UPDATE_20220416 // ゲートウェイ先ごとにSMTP認証ID,PWを規定する場合を有効にするオプションフラグ
extern BOOL  bExGateway;
#endif

#define NSLIMIT 20
#define MXLIMIT 20

struct _MX_RECODE {
  u_short  npreference;
  char     List[MAXDNAME];
};

#ifdef V4
BOOL MXLiveTime(char *pFn) {
  SYSTEMTIME ltime;
  FILETIME   st, ft;
  ULARGE_INTEGER *u1, *u2;
  __int64    n1, n2;
  HANDLE hFile;

   /// 現在時間
   GetSystemTime(&ltime);
   SystemTimeToFileTime(&ltime, &st);
   u1 = (ULARGE_INTEGER *)&st;
   n2 = n1  = u1->QuadPart;
   /// キャッシュ作成時間
   hFile = CreateFile( (LPCTSTR)pFn,
                        GENERIC_READ,
                        0,
                        NULL,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL, 
                        NULL);
   if (hFile != INVALID_HANDLE_VALUE) {
#ifdef ADD_IDCACHE     // Windows,AD,LDAP連携で問合せキャッシュ機能を追加。(問合せ負荷の軽減策)
	 GetFileTime(hFile, NULL, &st, &ft);
#else
	 GetFileTime(hFile, &ft, &st, NULL);
#endif
     CloseHandle(hFile);
     u2 = (ULARGE_INTEGER *)&ft;
     n2  = u2->QuadPart;
   }
   if (n1 < (n2 + (__int64)nMXCashLiveTime*10000000) )
	 return TRUE;    // キャッシュ有効
   else
	 return FALSE;   // キャッシュ無効
}

void SetMXCashList(char *pDom, char *pMX) {
#ifdef TRACE_MODE
	char str[LOG_BUFFER];
#endif
  FILE    *fpr, *fpw;
#ifdef UPDATE_20071205 // メールアドレス長が255Byteのときの対策
  char    mMXCash[512];
#else
  char    mMXCash[256];
#endif

#ifdef UPDATE_20070426 // 更新時間が設定されていない場合は設定しない
   if (nMXCashLiveTime == 0) // 更新時間が設定されていない場合は設定しない
	 return;
#endif

#ifdef TRACE_MODE
if (nSenderLog) {
   sprintf(str, "[          ] [%s] SetMXCashList=[%s]\n", pDom, pMX);
   printfTrace(str);
}
#endif

#ifdef UPDATE_20071205 // メールアドレス長が255Byteのときの対策
// ドメインが６４バイト以上なら、ＭＸキャッシュを作成しないで終了する。
// キャッシュファイル名の長に限界があるためが作成できずループしないようにする為。
   if (strlen(pDom) > 64) {  
	 return;
   }
#endif

#ifdef REGTOFILE
   sprintf(mMXCash,"%s\\mxcash\\", mMailSpoolDir);
   _mkdir(mMXCash);         // cashフォルダ作成
   if (nClustering) {
     sprintf(mMXCash,"%s\\mxcash\\%s\\", mMailSpoolDir, mComName);
   } else {
#endif
     sprintf(mMXCash,"%s\\mxcash\\", mMailSpoolDir);
#ifdef REGTOFILE
   }
#endif

   _mkdir(mMXCash);         // cashフォルダ作成
   strcat(mMXCash, pDom);
   strcat(mMXCash, ".mx");
   if (!(fpr = fopen(mMXCash, "rt"))) {
     if ((fpw = fopen(mMXCash, "wt"))) {
       fprintf(fpw, "%s", pMX);
	   fclose(fpw);
	 }
#ifdef UPDATE_20070320 // MXキャッシュデータの削除でハングする
     while (!(fpw = fopen(mMXCash, "rt"))) {
       if (bServiceTerminating)
         break;
	    _sleep(WAIT_TIME);
	 }
     fclose(fpw);
#endif
   } else {
     fclose(fpr);
   }
}

BOOL GetMXCashList(char *pDom, char *pGate) {
#ifdef TRACE_MODE
	char str[LOG_BUFFER];
#endif
  FILE    *fp;
  char    *fsts;
#ifdef UPDATE_20071205 // メールアドレス長が255Byteのときの対策
  char    mMXCash[512];
#else
  char    mMXCash[256];
#endif
  BOOL    bSts = FALSE; //ゲートウェイリストに載っているか否か

#ifdef REGTOFILE
      if (nClustering) {
        sprintf(mMXCash,"%s\\mxcash\\%s\\%s.mx", mMailSpoolDir, mComName, pDom);  // cashフォルダ
	  } else {
#endif
   sprintf(mMXCash,"%s\\mxcash\\%s.mx", mMailSpoolDir, pDom);  // cashフォルダ
#ifdef REGTOFILE
	  }
#endif
   if ((bSts = MXLiveTime(mMXCash))) {
     if ((fp = fopen(mMXCash, "rt"))) {
       if ((fsts = fgets(pGate, 256, fp)))
         strtok(pGate, "\n");
	   fclose(fp);
	 } else {
	   bSts = FALSE;
	 }
   } else {
	 _unlink(mMXCash);
#ifdef UPDATE_20070320 // MXキャッシュデータの削除でハングする
     while ((fp = fopen(mMXCash, "rt"))) {
	   fclose(fp);
       if (bServiceTerminating)
         break;
	   _sleep(WAIT_TIME);
	 }
#endif // UPDATE_20070320 // MXキャッシュデータの削除でハングする
   }
#ifdef TRACE_MODE
if (nSenderLog) {
   sprintf(str, "[          ] [%s] GetMXCashList=[%s] status=%s\n", pDom, pGate, (bSts ? "Hit" : "Fail"));
   printfTrace(str);
}
#endif
   return bSts;
}

void DelMXCashList(char *pDom) {
#ifdef TRACE_MODE
	char str[LOG_BUFFER];
#endif
#ifdef UPDATE_20071205 // メールアドレス長が255Byteのときの対策
  char    mMXCash[512];
#else
  char    mMXCash[256];
#endif
#ifdef UPDATE_20070320 // MXキャッシュデータの削除でハングする
  FILE    *fp;
#endif

#ifdef REGTOFILE
      if (nClustering) {
        sprintf(mMXCash,"%s\\mxcash\\%s\\%s.mx", mMailSpoolDir, mComName, pDom);  // cashフォルダ
	  } else {
#endif
   sprintf(mMXCash,"%s\\mxcash\\%s.mx", mMailSpoolDir, pDom);  // cashフォルダ
#ifdef REGTOFILE
	  }
#endif
/////////////////////////////////////////////////////////////////
   _unlink(mMXCash);
#ifdef UPDATE_20070320 // MXキャッシュデータの削除でハングする
    while ((fp = fopen(mMXCash, "rt"))) {
	  fclose(fp);
      if (bServiceTerminating)
        break;
	   _sleep(WAIT_TIME);
#ifdef UPDATE_20071129A // MXキャッシュファイルが削除されない場合の対策
       _unlink(mMXCash);
#endif
	}
#endif // UPDATE_20070320 // MXキャッシュデータの削除でハングする
/////////////////////////////////////////////////////////////////
#ifdef TRACE_MODE
if (nSenderLog) {
   sprintf(str, "[          ] [%s] DelMXCashList()\n", pDom);
   printfTrace(str);
}
#endif
}

#ifdef UPDATE_20070423 // MXキャッシュを全て削除する
void CleanMXCashList(void) {
#ifdef TRACE_MODE
	char str[LOG_BUFFER];
#endif
  HANDLE             hF, hFile;
  WIN32_FIND_DATA    FD;
  BOOL               bFile = TRUE;
#ifdef UPDATE_20071205 // メールアドレス長が255Byteのときの対策
  char    mMXCash[512], mCash[512];
#else
  char    mMXCash[256], mCash[256];
#endif
#ifdef UPDATE_20070320 // MXキャッシュデータの削除でハングする
  FILE    *fp;
#endif

#ifdef REGTOFILE
      if (nClustering) {
        sprintf(mMXCash,"%s\\mxcash\\%s\\*.mx", mMailSpoolDir, mComName);  // cashフォルダ
	  } else {
#endif
   sprintf(mMXCash,"%s\\mxcash\\*.mx", mMailSpoolDir);  // cashフォルダ
#ifdef REGTOFILE
	  }
#endif
     /////////////////////////////////////////////////////////////////
     hF = FindFirstFile(mMXCash, &FD);
     if (hF != INVALID_HANDLE_VALUE) {
       bFile = TRUE;
       while (bFile) {
	     if (!(!stricmp(FD.cFileName, ".") ||
	           !stricmp(FD.cFileName, ".."))) {
#ifdef REGTOFILE
           if (nClustering) {
             sprintf(mCash,"%s\\mxcash\\%s\\%s", mMailSpoolDir, mComName, FD.cFileName);  // cashフォルダ
		   } else
#endif
		   {
             sprintf(mCash,"%s\\mxcash\\%s", mMailSpoolDir, FD.cFileName);  // cashフォルダ
		   }
           _unlink(mCash);
#ifdef TRACE_MODE
           if (nSenderLog) {
             sprintf(str, "[          ] [%s] CleanMXCashList()\n", mCash);
             printfTrace(str);
		   }
#endif
		 }
         bFile = FindNextFile( hF, &FD);
	   }; 
       FindClose( hF ); 
	 }
}
#endif UPDATE_20070423 // MXキャッシュを全て削除する

#endif

#ifdef UPDATE_20241217_RCP // gateway.datにSMTP認証の指定が別のセッションで上書きされない対策
BOOL GetGatewayList(char *mDom, char *pFrom, char *pTo, char *mGate, int *nPort, BOOL *bSSL, int *nGateAuthType) 
#else
#ifdef UPDATE_20241128 // GATEWAY.DATにエンベロープの送信元の条件も追加可能に拡張した。
BOOL GetGatewayList(char *mDom, char *pFrom, char *pTo, char *mGate, int *nPort, BOOL *bSSL)
#else
#ifdef UPDATE_20141023 // メールアドレス単位でもGatway.datでフォワード指定可能にする対策
BOOL GetGatewayList(char *mDom, char *pTo, char *mGate, int *nPort, BOOL *bSSL)
#else
BOOL GetGatewayList(char *mDom, char *mGate, int *nPort, BOOL *bSSL)
#endif
#endif
#endif
{
  FILE    *fpSSL;
#ifdef UPDATE_20241128 // GATEWAY.DATにエンベロープの送信元の条件も追加可能に拡張した。
  char    *pFM;
#endif
#ifdef ADD_XOAUTH2_C // GATEWAY.DATにOAUTH2のURLを指定可能に
  char    *fsts, *pgate, *pport, *pssl, mSSL[4096];
#else
  char    *fsts, *pgate, *pport, *pssl, mSSL[1024];
#endif
  BOOL    bSts = FALSE; //ゲートウエィリストに載っているか否か
#ifdef UPDATE_20041126
  DWORD   stsDom = 1, nLeft, nRight, nDom;
  char    *pSSL;
#endif
#ifdef UPDATE_20220414 // ゲートウェイ先ごとにSMTP認証ID,PWを規定する場合
  char *plauthon, *plauthid, *plauthpw;
  //char mauthB64[1024];
#endif
#ifdef UPDATE_20071123 // 同一宛先の複数ゲートウェイ先（負荷分散）指定対応
  CHAR    *pg[3], *pp[3], *ps[3];
  int     np[3];
  CHAR    *pSep;
  SYSTEMTIME lt;
  INT     nSel = 0, nRoot = 0, nPt = *nPort;
#ifdef UPDATE_20141023 // メールアドレス単位でもGatway.datでフォワード指定可能にする対策
  BOOL    bAddr = FALSE;
#endif

  GetSystemTime(&lt);
#endif

#ifdef UPDATE_20220414 // ゲートウェイ先ごとにSMTP認証ID,PWを規定する場合
   plauthon = NULL;
   plauthid = NULL;
   plauthpw = NULL;
#endif

   sprintf(mSSL,"%sgateway.dat", mProgPath);
   if (bDebug) printf("GetGatewayList():[%s]\n", mSSL);
   fpSSL = fopen(mSSL, "rt");
   if (fpSSL) 
   {
     fsts = fgets(mSSL,sizeof(mSSL), fpSSL);
     while(fsts || !feof(fpSSL)) {
	   if (mSSL[0] != '\n' && mSSL[0] != '\'' && mSSL[0] != ' ') 
	   {
	     pssl   = pport = NULL;
         *nPort = nport; // SMTP over SSL default port
         *bSSL  = FALSE;
 	     strtok(mSSL, "\n");
	     pgate = strstr(mSSL, ",");
if (bDebug) printf("gateway.dat:%s\n", mSSL);
	     if (pgate) {
	 	   *pgate = '\x0';
		   pgate++;
#ifdef UPDATE_20241128 // GATEWAY.DATにエンベロープの送信元の条件も追加可能に拡張した。
		   if (pFM = strpbrk(mSSL, "|\t"))
		   {
			 *pFM = '\x0';
			 pFM++;
			 // エンベロープ送信元と比較
			 if (*pFrom == '\x0' ||  // エンベロープ送信元が空欄
				 !wildcard_stricmp(pFM, pFrom)) {  // 不一致していたら次のテーブル
			   fsts = fgets(mSSL,sizeof(mSSL), fpSSL);
			   continue;
			 }
		   }
#endif
#ifdef UPDATE_20071123 // 同一宛先の複数ゲートウェイ先（負荷分散）指定対応
		   nRoot = 0;
		   do {
  		     np[nRoot] = 0;
	         ps[nRoot] = NULL;
		     pg[nRoot] = pgate;
			 if ((pSep = strchr(pgate, '\t'))) {
			   *pSep = '\x0';
			   pgate = pSep+1;
			 }
#ifdef UPDATE_20110525 // gateway.datにIPv6アドレスを設定するとアドレスが正しく認識されない
			 if (pg[nRoot][0] == '[')
			 {
               char *pv6 = strpbrk(pg[nRoot], "]");
			   if (pv6)
			   {
	             pp[nRoot] = strpbrk(pv6, ",:");
			   }
			 } else 
#endif
			 {
	           pp[nRoot] = strpbrk(pg[nRoot], ",:");
			 }
	         if (pp[nRoot]) {
#ifdef ADD_XOAUTH2_C // GATEWAY.DATにOAUTH2のURLを指定可能に
			   if (strncmp(pp[nRoot] , "://", 3))
			   {
#endif
	 	         *pp[nRoot] = '\x0';
		         pp[nRoot]++;
  		         np[nRoot] = atoi(pp[nRoot]);
	             ps[nRoot] = strstr(pp[nRoot], "*");
#ifdef ADD_XOAUTH2_C // GATEWAY.DATにOAUTH2のURLを指定可能に
#ifdef UPDATE_20241110 // XOAUTH2のトークン取得に失敗する不具合
			   } else {
                 pp[nRoot] = strpbrk(pg[nRoot], ",");
	 	         *pp[nRoot] = '\x0';
		         pp[nRoot]++;
  		         np[nRoot] = atoi(pp[nRoot]);
	             ps[nRoot] = strstr(pp[nRoot], "*");
#endif
			   }
#endif
			 }
		     nRoot++;
		   } while (pSep && (nRoot < 3)); // 3箇所まで
		   nSel = (lt.wSecond/10)%nRoot;  // 負荷分散１０秒単位
#ifdef UPDATE_20220414 // ゲートウェイ先ごとにSMTP認証ID,PWを規定する場合
           strcpy(mGWDAT, pg[nSel]);
		   if (plauthon=strchr(pg[nSel], '|'))
		   {
             *plauthon = '\x0';
			 plauthon++;
			 mGWDAT[0] = '\x0';
			 strcpy(&mGWDAT[1], plauthon);
             plauthon = &mGWDAT[1];
			 if (*plauthon == '0')  // 強制的にSMTP-AUTHなし
			 {
               plauthid = mGWDAT;
               plauthpw = mGWDAT;
			 } else {
               if (plauthid=strchr(plauthon, '|'))
			   {  
                 *plauthid = '\x0';
			     plauthid++;
#ifdef UPDATE_20241217 // gateway.datにSMTP認証の指定がある時は優先するようにした。
				 strupr(plauthon);
				 if (strstr(plauthon+1, "CRAM-MD5"))
#ifdef UPDATE_20241217_RCP // gateway.datにSMTP認証の指定が別のセッションで上書きされない対策
                   *nGateAuthType  |= 0x4;
#else
                   bGWAUTHTYPE[0] = TRUE;
#endif
				 else if (strstr(plauthon+1, "LOGIN"))
#ifdef UPDATE_20241217_RCP // gateway.datにSMTP認証の指定が別のセッションで上書きされない対策
                   *nGateAuthType  |= 0x1;
#else
                   bGWAUTHTYPE[1] = TRUE;
#endif
				 else if (strstr(plauthon+1, "PLAIN"))
#ifdef UPDATE_20241217_RCP // gateway.datにSMTP認証の指定が別のセッションで上書きされない対策
                   *nGateAuthType  |= 0x2;
#else
                   bGWAUTHTYPE[2] = TRUE;
#endif
				 else if (strstr(plauthon+1, "XOAUTH2"))
#ifdef UPDATE_20241217_RCP // gateway.datにSMTP認証の指定が別のセッションで上書きされない対策
                   *nGateAuthType  |= 0x8;
#else
                   bGWAUTHTYPE[3] = TRUE;
#endif
				 *(plauthon+1)='\x0';
#endif
if (bDebug) printf("GetGatewayList(): *plauthon=%c\n", *plauthon);
			     if (*plauthon == '2')  // BASE64で保存
				 {
			       Base64DecodeLine(plauthid, strlen(plauthid), (unsigned char *)mauthB64, sizeof(mauthB64));
                   plauthid = mauthB64;
				 } 
#ifdef UPDATE_20220418 // ゲートウェイ先ごとにSMTP認証ID,PWを規定する場合にデータを暗号化する
				 else if (*plauthon == '3')  // BAS64でデコードされた独自暗号化データで保存
				 {
			       Base64DecodeLine(plauthid, strlen(plauthid), (unsigned char *)mauthSPA, sizeof(mauthSPA));
				   SPA_Decode(mauthSPA, mauthB64);
                   plauthid = mauthB64;
				 }
#endif
if (bDebug) printf("GetGatewayList(): plauthid=%s\n", plauthid);
			     if (plauthpw=strchr(plauthid, '|'))
				 {
                   *plauthpw = '\x0';
			       plauthpw++;
				 } else {
			       plauthpw = NULL;
				 }
			   } else {
			     plauthid = NULL;
			   }
			 }
		   } else {
			 plauthon = NULL;
		   }
#endif
#ifdef UPDATE_20110525 // gateway.datにIPv6アドレスを設定するとアドレスが正しく認識されない
		   if (pg[nSel][0] == '[')
		   {
             strcpy(mGate, &pg[nSel][1]);
			 strtok(mGate, "]");
		   } else 
#endif
		   {
		     strcpy(mGate, pg[nSel]);
		   }
		   *nPort = (np[nSel] ? np[nSel] : nPt);
	       pssl = ps[nSel];
#else
		   strcpy(mGate, pgate);
		   strtok(mGate, ",");
	       pport = strstr(pgate, ",");
	       if (pport) {
	 	     *pport = '\x0';
		     pport++;
  		     *nPort = atoi(pport);
	         pssl = strstr(pport, "*");
		   }
#endif
		 }
#ifdef UPDATE_20141023 // メールアドレス単位でもGatway.datでフォワード指定可能にする対策
         bAddr = (strchr(mSSL,'@') ? TRUE : FALSE); // メールアドレスフォワードか否か
#endif
#ifdef UPDATE_20041126
		 if ((pSSL = strstr(mSSL,"*")))
		 {
           *pSSL = '\x0';
           nLeft  = strlen(mSSL);
		   if (nLeft == 0)
			 stsDom = 0;
		   else
#ifdef UPDATE_20141023 // メールアドレス単位でもGatway.datでフォワード指定可能にする対策
			 stsDom = _strnicmp(mSSL, (bAddr ? pTo : mDom), nLeft);
#else
		     stsDom = _strnicmp(mSSL, mDom, nLeft);
#endif
           nRight = strlen(pSSL+1);
		   if (nRight) {
#ifdef UPDATE_20141023 // メールアドレス単位でもGatway.datでフォワード指定可能にする対策
		     nDom   = strlen(bAddr ? pTo : mDom);
#else
		     nDom   = strlen(mDom);
#endif
		     if (nDom < nLeft+nRight) {
 		       stsDom = -1;
			 } else {
#ifdef UPDATE_20141023 // メールアドレス単位でもGatway.datでフォワード指定可能にする対策
			   if (stsDom || _strnicmp(pSSL+1, (bAddr ? &pTo[nDom-nRight] :  &mDom[nDom-nRight] ), nRight))
#else
			   if (stsDom || _strnicmp(pSSL+1, &mDom[nDom-nRight], nRight))
#endif
			     stsDom = -1;
			 }
		   }
		 } else {
#ifdef UPDATE_20141023 // メールアドレス単位でもGatway.datでフォワード指定可能にする対策
		   stsDom = _stricmp(mSSL, (bAddr ? pTo : mDom));
#else
		   stsDom = _stricmp(mSSL, mDom);
#endif
		 }
		 if (stsDom == 0)
#else
#ifdef UPDATE_20141023 // メールアドレス単位でもGatway.datでフォワード指定可能にする対策
	     if (strcmp(mSSL,"*") == 0 || // '*'なら全てのサーバーに対して
	       stricmp((bAddr ? pTo : mDom), mSSL) == 0)
#else
	     if (strcmp(mSSL,"*") == 0 || // '*'なら全てのサーバーに対して
	       stricmp(mDom, mSSL) == 0)
#endif
#endif
		 {
#ifdef UPDATE_20241217_RCP // gateway.datにSMTP認証の指定が別のセッションで上書きされない対策
	      pAuthON = NULL;
	      pAuthID = NULL;
	      pAuthPW = NULL;
#endif
#ifdef UPDATE_20220414 // ゲートウェイ先ごとにSMTP認証ID,PWを規定する場合
          if (plauthon && plauthid && plauthpw)
		  {
		    pAuthON = plauthon;
			pAuthID = plauthid;
#ifdef UPDATE_20250601 // OAUTH2でのアクセストークンとリフレッシュトークンを特定のメールアドレスを参照先とする機能を追加
            if (!stricmp(plauthpw, "NULL"))
			  pAuthPW = NULL;
			else
#endif
		    pAuthPW = plauthpw;
		  }
#ifdef UPDATE_20220416 // ゲートウェイ先ごとにSMTP認証ID,PWを規定する場合を有効にするオプションフラグ
          if (!bExGateway)
		  {
	        pAuthON = NULL;
	        pAuthID = NULL;
	        pAuthPW = NULL;
		  }
#endif
#endif
		   bSts = TRUE;
	 	   *bSSL = FALSE;
		   if (pssl) {
		     if (*pssl == '*') // Non SSL Server
		       *bSSL = TRUE;
		   } 
		   break;
		 }
	   }
	   fsts = fgets(mSSL,sizeof(mSSL), fpSSL);
	 }			 
	 fclose(fpSSL);
   }
   if (!bSts) { // デフォルトに戻す
	 *mGate = '\x0';
     *nPort = nport; // SMTP over SSL default port
   }
   //if (bDebug) printf("pAuthON=[%s] / pAuthID=[%s] / pAuthPW=[%s]\n", (pAuthON ? pAuthON : ""), (pAuthID ? pAuthID : ""), (pAuthPW ? pAuthPW :""));

   return bSts;
}

#ifdef UPDATE_20241217_RCP // gateway.datにSMTP認証の指定が別のセッションで上書きされない対策
DWORD GetSMTPServer(char *mNS, char *mDomain, char *pFrom, char *pTo, char *mMsg, char *mSMTPServer, int *nPort, BOOL *bSSL, BOOL *bNotNS, BOOL *bNotMX, int *nGateAuthType) 
#else
#ifdef UPDATE_20241128 // GATEWAY.DATにエンベロープの送信元の条件も追加可能に拡張した。
DWORD GetSMTPServer(char *mNS, char *mDomain, char *pFrom, char *pTo, char *mMsg, char *mSMTPServer, int *nPort, BOOL *bSSL, BOOL *bNotNS, BOOL *bNotMX) 
#else
#ifdef UPDATE_20141023 // メールアドレス単位でもGatway.datでフォワード指定可能にする対策
DWORD GetSMTPServer(char *mNS, char *mDomain, char *pTo, char *mMsg, char *mSMTPServer, int *nPort, BOOL *bSSL, BOOL *bNotNS, BOOL *bNotMX) 
#else
DWORD GetSMTPServer(char *mNS, char *mDomain, char *mMsg, char *mSMTPServer, int *nPort, BOOL *bSSL, BOOL *bNotNS, BOOL *bNotMX) 
#endif
#endif
#endif
{
//void main(int argc, char **argv) {
#ifdef TRACE_MODE
	char str[LOG_BUFFER];
#endif
	DWORD  sts = 0; //FALSE; 0=not,1=Direct,2=MX,3=Gateway
	char mNSwork[MAXDNAME];
	struct _MX_RECODE mx[MXLIMIT];
	u_short nwork;
	char mwork[MAXDNAME];
    char nsList[NSLIMIT][MAXDNAME];
    char *pnsList[NSLIMIT], *pList[MXLIMIT] ;
	char *pns[3], *pdom;
	char *pMsg, mMsgNo[256];
	int  nsNum = 0;
	int  mxNum = 0;
	int  i, j, k; //, l;
    struct in_addr a[3];
// 	WSADATA WSAData;
//	BOOL    bOKSock;
    PHOSTENT phe;
/*
#ifdef IPv6
	SOCKADDR_IN6 sin;
#else
	SOCKADDR_IN sin;
#endif
*/
//	SOCKET msock;
	//int    nsock;
	DWORD  nTry, nErrCode;
#ifdef IPv6
    char   *Address = NULL;
    struct addrinfo Hints, *AddrInfo = NULL;
	char   mport[16];
	int    Error;
	BOOL   bv6 = FALSE;
#endif
#ifndef VC6
  HOSTENT hst;
#endif
#ifdef UPDATE_20041008
    BOOL   bOK = TRUE; //ドメイン無しなら許可
#endif
#ifdef UPDATE_20070321 //DNSへの問合せテーブル初期化(初期化が無いと問合せで不定期にハングする可能性有り)
    HANDLE hFileDNS;
#endif

if (bDebug)
{
  printf("in GetSMTPServer() bAUTHTYPE[0]=%d\n",  bAUTHTYPE[0]);
  printf("in GetSMTPServer() bAUTHTYPE[1]=%d\n",  bAUTHTYPE[1]);
  printf("in GetSMTPServer() bAUTHTYPE[2]=%d\n",  bAUTHTYPE[2]);
  printf("in GetSMTPServer() bAUTHTYPE[3]=%d\n",  bAUTHTYPE[3]);
  printf("in GetSMTPServer() nGateAuthType=%x\n", nGateAuthType);
  printf("in GetSMTPServer() *nGateAuthType=%d\n", *nGateAuthType);
}

#ifdef UPDATE_20241217_RCP // gateway.datにSMTP認証の指定が別のセッションで上書きされない対策
	*nGateAuthType = (bAUTHTYPE[0] ? 0x4 : 0) | (bAUTHTYPE[1] ? 0x1 : 0) | (bAUTHTYPE[2] ? 0x2 : 0) | (bAUTHTYPE[3] ? 0x8 : 0);
#else
#ifdef UPDATE_20241217 // gateway.datにSMTP認証の指定がある時は優先するようにした。
    bGWAUTHTYPE[0] = bAUTHTYPE[0]; // CRAM-MD5
    bGWAUTHTYPE[1] = bAUTHTYPE[1]; // LOGIN
    bGWAUTHTYPE[2] = bAUTHTYPE[2]; // PLAIN
    bGWAUTHTYPE[3] = bAUTHTYPE[3]; // XOAUTH2
#endif
#endif
if (bDebug)
  printf("in GetSMTPServer() *nGateAuthType=%d\n", *nGateAuthType);

	pMsg = strrchr( mMsg, '\\');
	if (pMsg) 
	  strcpy(mMsgNo, ++pMsg);
	else
	  strcpy(mMsgNo, mMsg);
    ////////////////////////////// 2001.12.30
	//strtok(mMsgNo, ".");
	pMsg = strrchr(mMsgNo, '.');
	if (pMsg)
	  *pMsg = '\x0';
    /////////////////////////////////////////

    *bNotMX = FALSE;  // MXレコード有りとする
    *nPort = nport;   // Default port
	if (bDebug)
	  printf("\n");
	i = MAXDNAME;
	mSMTPServer[0] = '\x0';
#ifdef UPDATE_20041008
    // 全角ドメインチェック
    for (i = 0; i < (int)strlen(mDomain); i++) {
	  bOK = FALSE;
	  if (0x20 <= *(mDomain+i) && *(mDomain+i) <= 0x7f)
	    bOK = TRUE;
	  else
	    break;
	}

	if (!bOK) { //
	  sts = 0; // MXレコードなし
      *bNotNS = FALSE;  // DNSが存在する時
      *bNotMX = TRUE;  // MXレコード有りとする
	  if (bDebug)
        printf("[%s] [%s] illegal domain\n", mMsgNo, mDomain);
#ifdef TRACE_MODE
if (nSenderLog) {
  sprintf(str, "[%s] [%s] illegal domain\n", mMsgNo, mDomain);
  printfTrace(str);
}
#endif
      return sts;

	} 
#ifdef UPDATE_20241217_RCP // gateway.datにSMTP認証の指定が別のセッションで上書きされない対策
	else if (GetGatewayList(mDomain, pFrom, pTo, mSMTPServer, nPort, bSSL, nGateAuthType))
#else
#ifdef UPDATE_20241128 // GATEWAY.DATにエンベロープの送信元の条件も追加可能に拡張した。
	else if (GetGatewayList(mDomain, pFrom, pTo, mSMTPServer, nPort, bSSL))
#else
#ifdef UPDATE_20141023 // メールアドレス単位でもGatway.datでフォワード指定可能にする対策
	else if (GetGatewayList(mDomain, pTo, mSMTPServer, nPort, bSSL))
#else
	else if (GetGatewayList(mDomain, mSMTPServer, nPort, bSSL))
#endif
#endif
#endif
#else
#ifdef UPDATE_20241217_RCP // gateway.datにSMTP認証の指定が別のセッションで上書きされない対策
	if (GetGatewayList(mDomain, pFrom, pTo, mSMTPServer, nPort, bSSL, nGateAuthType))
#else
#ifdef UPDATE_20241128 // GATEWAY.DATにエンベロープの送信元の条件も追加可能に拡張した。
	if (GetGatewayList(mDomain, pFrom, pTo, mSMTPServer, nPort, bSSL))
#else
#ifdef UPDATE_20141023 // メールアドレス単位でもGatway.datでフォワード指定可能にする対策
	if (GetGatewayList(mDomain, pTo, mSMTPServer, nPort, bSSL))
#else
    if (GetGatewayList(mDomain, mSMTPServer, nPort, bSSL))
#endif
#endif
#endif
#endif
	{
         *bNotNS = TRUE;  // DNSが存在する時
         *bNotMX = TRUE;  // MXレコード有りとする
	     sts = 3; //TRUE;
         if (bDebug)
		   printf("[%s] GetSMTPServer = SMTP GateWay(%s:%d%s)\n", mMsgNo, mSMTPServer, *nPort, (*bSSL ? "/SSL" : ""));
#ifdef TRACE_MODE
if (nSenderLog) {
  sprintf(str, "[%s] GetSMTPServer = SMTP GateWay(%s:%d%s)\n", mMsgNo, mSMTPServer, *nPort, (*bSSL ? "/SSL" : ""));
  printfTrace(str);
}
#endif
	   return sts;
	} else  if (mSendGateway[0] != '\x0') {  // 中継用ゲートウェイを指定
#ifdef UPDATE_20141105 // MXレコード参照失敗した受信をSMTP Gatewayへ転送
	  if (!bMailForward2)
#endif
	  {
         strcpy(mSMTPServer, mSendGateway);
         *bNotNS = TRUE;  // DNSが存在する時
         *bNotMX = TRUE;  // MXレコード有りとする
	     sts = 3; //TRUE;
         if (bDebug)
	       printf("[%s] GetSMTPServer = SMTP GateWay(%s:%d)\n", mMsgNo, mSMTPServer, *nPort);
#ifdef TRACE_MODE
if (nSenderLog) {
  sprintf(str, "[%s] GetSMTPServer = SMTP GateWay(%s:%d)\n", mMsgNo, mSMTPServer, *nPort);
  printfTrace(str);
}
#endif
	     return sts;
	  }
	}
#ifdef V4
	if (nMXCashLiveTime > 0) { // 0 以上
	  if (GetMXCashList(mDomain, mSMTPServer)) {
        *bNotNS = TRUE;  // DNSキャッシュが存在する時
        *bNotMX = TRUE;  // MXレコード有りとする
	    sts = 3; //TRUE;
        if (bDebug)
	      printf("[%s] GetSMTPServer = SMTP MX Chash(%s=%s)\n", mMsgNo, mDomain, mSMTPServer);
#ifdef TRACE_MODE
        if (nSenderLog) {
          sprintf(str, "[%s] GetSMTPServer = SMTP MX Chash(%s=%s)\n", mMsgNo, mDomain, mSMTPServer);
          printfTrace(str);
		}
#endif
	    return sts;
	  }
	} else {
      if (bDebug)
	    printf("[%s] GetSMTPServer = SMTP MX Chash Time = Zero.\n", mMsgNo);
#ifdef TRACE_MODE
      if (nSenderLog) {
        sprintf(str, "[%s] GetSMTPServer = SMTP MX Chash Time = Zero.\n", mMsgNo);
        printfTrace(str);
	  }
#endif
	}
#endif

	//nTry = GetTryServer(mDomain, NULL, &nErrCode, FALSE);
	nTry = GetTryServer(mDomain, mMsg, &nErrCode, FALSE);
#ifdef IPv6
    //nTry=1;
#endif
	if (nTry % 2 == 1) { //0) { //
	//if (nTry % 2 == 0) { //
	//if (nTry == 0) {
#ifdef IPv6
      Error = 0;
#ifdef VC6
      phe = getipnodebyname(mDomain, (nAddressFamily ? AF_INET6 : AF_INET),  (nAddressFamily ? AI_ALL : 0), &Error);
#else
	  phe = NULL;
      memset(&Hints, 0, sizeof(Hints));
	  Hints.ai_flags = AI_NUMERICHOST | AI_PASSIVE;
      Hints.ai_family =   (nAddressFamily ? AF_INET6 : AF_INET);
      Hints.ai_socktype = SOCK_STREAM;
	  //itoa(*nPort, mport, 10);
	  // v4.63 x64 Addr=000000000000bdfc このあたりでハングする事例があった。解決 2018.10.04 
	  if (getaddrinfo(mDomain, NULL, &Hints, &AddrInfo) == 0) {
#ifdef UPDATE_20181004 // ドメイン名からの順引きが成功したときのアドレス取得でハングすることがあった。
        hst.h_addr_list = (struct sockaddr_in *)(AddrInfo->ai_addr);
#else
        hst.h_addr = (struct sockaddr_in *)(AddrInfo->ai_addr);
#endif
		phe = &hst;
      }
#endif
#else
      phe = gethostbyname(mDomain);
#endif
      if (phe) {        // @以降のドメインでマシンが直接引けるならSMTPサーバーと解釈
        strcpy(mSMTPServer, mDomain);
  	    *bNotNS = TRUE;  // DNSが存在する時
        *bNotMX = TRUE;  // MXレコード有りとする
	    sts = 1; //TRUE;
        if (bDebug)
	      printf("[%s] GetSMTPServer = SMTP Direct Domain\n", mMsgNo);
#ifdef TRACE_MODE
if (nSenderLog) {
  sprintf(str, "[%s] GetSMTPServer = SMTP Direct Domain\n", mMsgNo);
  printfTrace(str);
}
#endif
#ifdef IPv6
#ifdef VC6
        if (phe)
          freehostent(phe);
#else  
        if (phe)
 		  freeaddrinfo(AddrInfo);
#endif
#endif
		return sts;
	  }
	}
    if (bDebug)
      printf("[%s] GetSMTPServer = SMTP Search MX Recode\n", mMsgNo);
#ifdef TRACE_MODE
if (nSenderLog) {
  sprintf(str, "[%s] GetSMTPServer = SMTP Search MX Recode\n", mMsgNo);
  printfTrace(str);
}
#endif

	///// DNSへの問合せ実行中かチェック
#ifdef UPDATE_20070321 //DNSへの問合せテーブル初期化(初期化が無いと問合せで不定期にハングする可能性有り)
    while ((hFileDNS = CreateFile((LPCTSTR)mDNSMXLockFn,
                        GENERIC_WRITE,
                        0,   // 排他アクセス = 0
                        NULL,
                        CREATE_ALWAYS,
                        FILE_ATTRIBUTE_NORMAL, 
                        NULL)) == INVALID_HANDLE_VALUE) {
      if (bServiceTerminating) 
        return sts;
      _sleep(WAIT_TIME);
	} 
 	//res_init();
#else
    while(bWaitDNS) {    // DNS問合せはシングルスレッドで処理
      if (bServiceTerminating)
        return sts;
#ifndef NEW_SEND
      _sleep(WAIT_TIME*100);
#else
      _sleep(WAIT_TIME);
#endif
	}
#endif
	bWaitDNS = TRUE;

	for (i = 0; i < NSLIMIT; i++)
	 pnsList[i] = (char *)nsList[i];

	for (i = 0; i < MXLIMIT; i++)
	 pList[i] = (char *)mx[i].List;

/*
 	bOKSock = FALSE;
#ifdef IPv6
    if (WSAStartup(MAKEWORD(2,2), &WSAData) == 0)
	  bOKSock = TRUE;
#else
    if (WSAStartup(MAKEWORD(1,1), &WSAData) == 0) 
	  bOKSock = TRUE;
#endif
*/
    _res.options |= RES_INIT;
#ifdef UPDATE_20140805 // MX問合せ失敗があった場合の対策。
	_res.options |= RES_IGNTC;
#endif
	//if (*mNS != '\x0' && bOKSock) { // ネームサーバーが指定されていないなら中止
	if (*mNS != '\x0') { // ネームサーバーが指定されていないなら中止
	  *mSMTPServer = '\x0';
	  pdom = mDomain;
      //res_init();
	  while(pdom && !sts) {
	    if (bServiceTerminating)    // if server stop signaled then exit.
		  break;
		if (!strpbrk(pdom, "."))
          break;
		if (bDebug)
          printf("[%s] [%s] start findNameServers\n", mMsgNo, pdom);
#ifdef TRACE_MODE
if (nSenderLog) { // 20070320test 1
  sprintf(str, "[%s] [%s] start findNameServers\n", mMsgNo, pdom);
  printfTrace(str);
}
#endif
          //res_init();
	      strcpy(mNSwork, mNS);
	      pns[0] = strtok(mNSwork, " ");               // 最初のネームサーバー
          _res.nscount = 1;
          pns[1] = strtok(NULL, " ");  // 次のネームサーバー
	      if (pns[1])
            _res.nscount++;
          pns[2] = strtok(NULL, " ");  // 次のネームサーバー
	      if (pns[2])
            _res.nscount++;
          _res.retry = 4; // 検索のリトライ回数
	      for (i = 0; i < _res.nscount; i++) {
            if (bDebug)
		      printf("NS[%d] = %s\n", i, pns[i]);
#ifdef IPv6
            bv6 = FALSE;
            if (strstr(pns[i],":")) { // IPv6
			  // BIND 9　以前ではIPv6でアドレス引きはできない為、
			  // DNSはIPv4上にある前提で処理される。
			  bv6 = TRUE;
              memset(&Hints, 0, sizeof(Hints));
			  Hints.ai_flags = AI_NUMERICHOST | AI_PASSIVE;
              Hints.ai_family = AF_INET6;
              Hints.ai_socktype = SOCK_DGRAM;
	          itoa(NAMESERVER_PORT, mport, 10);
              if (getaddrinfo(pns[i], mport, &Hints, &AddrInfo) !=0) {
				if (bDebug)
                  printf("getaddrinfo failed with error\n");
                continue;
			  }
 	          memcpy(&_res.nsaddr6_list[i], AddrInfo, sizeof(struct addrinfo));
		  	  //freeaddrinfo(AddrInfo);
			} else {              // IPv4
              inet_pton(AF_INET, pns[i], &a[i]);
              strcpy(_res.defdname, pdom);//mDomain);
	          _res.nsaddr_list[i].sin_addr = a[i];
              _res.nsaddr_list[i].sin_family = AF_INET;
              _res.nsaddr_list[i].sin_port = htons(NAMESERVER_PORT);
			}
#else
  	        inet_aton(pns[i], &a[i]);
            strcpy(_res.defdname, pdom);//mDomain);
	        _res.nsaddr_list[i].sin_addr = a[i];
            _res.nsaddr_list[i].sin_family = AF_INET;
            _res.nsaddr_list[i].sin_port = htons(NAMESERVER_PORT);
#endif
		  }
		  /*
		  if (bServerType)
            *bNotNS = findNameServers(pdom, pnsList, &nsNum);
		  else
            *bNotNS = LocalNameServers(&pnsList, pns, &nsNum);
		  */
		  *bNotNS = LocalNameServers(&pnsList, pns, &nsNum);
		  if (*bNotNS) {
			for (i = 0; i < nsNum; i++) {
			  if (bDebug)
				printf("[%s] (%s) %sNameServers\n", mMsgNo, pnsList[i] , (bServerType?"find":"Local"));
#ifdef TRACE_MODE
if (nSenderLog) { // 20070320test 2
  sprintf(str, "[%s] (%s) %sNameServers\n", mMsgNo, pnsList[i], (bServerType?"find":"Local"));
  printfTrace(str);
}
#endif
			}
			if (bDebug)
              printf("[%s] start queryMXRecode\n", mDomain);
			// MXレコード最初の問い合わせは、"mDomain"が正解
	        if (queryMXRecode(mMsgNo, mDomain, pList, mx, &mxNum, pnsList, nsNum)) {
#ifdef IPv6
			  if (bv6)
				freeaddrinfo(AddrInfo);
#endif
		      k = 0;
              for (i = mxNum-k-1; i >= 0; i--) {  // Sort する。
		        strcpy(mwork, mx[i].List);
 	            for (j = mxNum-k-2; j >= 0; j--) {
 	              if (mx[i].npreference < mx[j].npreference) {
 				    nwork = mx[j].npreference;
                    strcpy(mwork, mx[j].List);
				    mx[j].npreference = mx[i].npreference;
                    strcpy(mx[j].List, mx[i].List);
				    mx[i].npreference = nwork;
                    strcpy(mx[i].List, mwork);
				  }
				}
				if (bDebug)
                  printf("[%s] [%s] [%d] IN MX %d %s\n", mMsgNo, mDomain, i+1, mx[i].npreference, mx[i].List);
#ifdef TRACE_MODE
if (nSenderLog) { // 20070320test
  sprintf(str, "[%s] queryMXRecode:[%s] [%d] IN MX %d %s\n", mMsgNo, mDomain, i+1, mx[i].npreference, mx[i].List);
  printfTrace(str);
}
#endif
			    k++;
			  }
			  if (mxNum > 0) { 
#ifdef TRACE_MODE
if (nSenderLog) {
  sprintf(str, "[%s] nErrCode:[%d] nTry:[%d] %% mxNum:[%d] = [%s]\n", mMsgNo, nErrCode, nTry/2, mxNum, mx[(nTry/2) % mxNum].List);
  printfTrace(str);
}
#endif
#ifdef UPDATE_20060929 // 接続不能時に別のMXレコード選択
				if (nTry > 0 ||
					nErrCode == INVALID_SOCKET )// 接続不能だった時MXレコードの記述された次のSMTPサーバーを選択する。
#else
				if (nErrCode == 0 && nTry > 0 ||
					nErrCode == INVALID_SOCKET )// 接続不能だった時MXレコードの記述された次のSMTPサーバーを選択する。
#endif
				{
	              //strcpy(mSMTPServer, mx[nTry % mxNum].List);
	              strcpy(mSMTPServer, mx[(nTry/2) % mxNum].List);
				} else {
	              strcpy(mSMTPServer, mx[0].List);
				}
#ifdef TRACE_MODE
if (nSenderLog) { // 20070320test 3
  sprintf(str, "[%s] [%s] SMTPServer=[%s]\n", mMsgNo, mDomain, mSMTPServer);
  printfTrace(str);
}
#endif
#ifdef V4
                SetMXCashList(mDomain, mSMTPServer);
#endif
                *bNotMX = TRUE;  // MXレコード有り
		        sts = 2; //TRUE;
			  } else
#ifdef NEW_MXQUERY
			  {
                if (queryMXRecode2(mMsgNo, mDomain, mNS, mSMTPServer, &mxNum) )
				{
#ifdef V4
                  SetMXCashList(mDomain, mSMTPServer);
#endif
                  *bNotMX = TRUE;  // MXレコード有り
		          sts = 2; //TRUE;
				} else
#endif
				{
#ifdef UPDATE_20071129A // MXキャッシュファイルが削除されない場合の対策
				DelMXCashList(mDomain);  // MXキャッシュ削除
#else
#ifdef V4
                SetMXCashList(mDomain, mDomain);
#endif
#endif
#ifdef NEW_MXQUERY
				}
#endif
			  }
			  if (!sts) {
				if (bDebug)
                  printf("[%s] [%s] not found queryMXRecode\n", mMsgNo, mDomain);
#ifdef TRACE_MODE
if (nSenderLog) { // 20070320test 4
  sprintf(str, "[%s] [%s] not found queryMXRecode\n", mMsgNo, mDomain);
  printfTrace(str);
}
#endif
				break;   //ネームサーバが見つかっているならMXレコード検出は失敗終了する。
			  }
			} else {
#ifdef IPv6
			  if (bv6)
				freeaddrinfo(AddrInfo);
#endif
			  if (bDebug)
                printf("[%s] [%s] error queryMXRecode\n", mMsgNo, mDomain);
#ifdef TRACE_MODE
if (nSenderLog) { // 20070320test 5
  sprintf(str, "[%s] [%s] error queryMXRecode\n", mMsgNo, mDomain);
  printfTrace(str);
}
#endif
			  break;   //ネームサーバが見つかっているならMXレコード検出は失敗終了する。
			}
		  } else {
#ifdef IPv6
			  if (bv6)
				freeaddrinfo(AddrInfo);
#endif
			if (bDebug)
 	          printf("[%s] [%s] not found findNameServers\n",mMsgNo, pdom);
#ifdef TRACE_MODE
if (nSenderLog) { // 20070320test 6
  sprintf(str, "[%s] [%s] not found findNameServers\n",mMsgNo, pdom);
  printfTrace(str);
}
#endif
		  }
		if (sts) {
		  break;
		} else {
		  pdom = strpbrk(pdom, ".");
		  if (pdom)
		    pdom++;
		}
	  }
	}
#ifdef UPDATE_20070321 //DNSへの問合せテーブル初期化(初期化が無いと問合せで不定期にハングする可能性有り)
    CloseHandle(hFileDNS);
#else
	bWaitDNS = FALSE;
#endif
#ifdef UPDATE_20141105 // MXレコード参照失敗した受信をSMTP Gatewayへ転送
	if (sts == 0 && bMailForward2 && mSendGateway[0] != '\x0') // 中継用ゲートウェイを指定
	{
      strcpy(mSMTPServer, mSendGateway);
      *bNotNS = TRUE;  // DNSが存在する時
      *bNotMX = TRUE;  // MXレコード有りとする
	  sts = 3; //TRUE;
      if (bDebug)
	    printf("[%s] GetSMTPServer = SMTP GateWay(%s:%d)\n", mMsgNo, mSMTPServer, *nPort);
#ifdef TRACE_MODE
if (nSenderLog) {
  sprintf(str, "[%s] GetSMTPServer = SMTP GateWay(%s:%d)\n", mMsgNo, mSMTPServer, *nPort);
  printfTrace(str);
}
#endif
	}
#endif

	return sts;
}

BOOL LocalNameServers(char *pnsList[], char *pns[], int *nsNum) {
  //自ネームサーバーを指定
  BOOL sts = TRUE;
    *nsNum = 0;
    pnsList[0] = pns[0];
	(*nsNum)++;
    if (pns[1]) {
      pnsList[1] = pns[1];
	  (*nsNum)++;
	}
	if (pns[2]) {
      pnsList[2] = pns[2];
	  (*nsNum)++;
	}
    return sts;
}

BOOL findNameServers(char *domain, char *pnsList[], int *nsNum) {
#ifdef TRACE_MODE
	char str[LOG_BUFFER];
#endif
	BOOL sts = TRUE;
	union {
		HEADER hdr;
		u_char buf[PACKETSZ*2];
	} response;
	int responseLen;

	u_char  *cp;
	u_char  *endOfMsg;
	u_short clss;
	u_short type;
	u_long  ttl;
	u_short dlen;
	int i, count, dup, n;
#ifdef TRACE_MODE
if (nSenderLog) {
  sprintf(str, "[%s] findNameServers:res_query\n", domain);
//  sprintf(str, "findNameServers:res_search:domain=%s\n", domain);
  printfTrace(str);
}
#endif
//    responseLen = res_search(domain,
    responseLen = res_query(domain,
                          C_IN,
						  T_NS,
						  (u_char *) &response,
						  sizeof(response));
    if (responseLen < 0) { 
      nsError(h_errno, domain);
      sts = FALSE;
      return sts; //exit(1);
	}
	endOfMsg = response.buf + responseLen;
	cp =  response.buf + sizeof(HEADER);
	cp += skipName(cp, endOfMsg) + QFIXEDSZ;
	count = ntohs((u_short)response.hdr.ancount) +
		    ntohs((u_short)response.hdr.nscount);
#ifdef TRACE_MODE
if (nSenderLog) {
  sprintf(str, "[%s] findNameServers:while():*nsNum=%d\n", domain, *nsNum);
  printfTrace(str);
}
#endif
	while( (--count >= 0) &&
		   (cp < endOfMsg) &&
		   (*nsNum < NSLIMIT) ) {
	  n = skipToData(cp, &type, &clss, &ttl, &dlen, endOfMsg);
	  if (n < 0) {
        sts = FALSE;
		break;
	  }
	  cp += n; //skipToData(cp, &type, &clss, &ttl, &dlen, endOfMsg);
	  if (type == T_NS) {
        //nsList[*nsNum] = (char *) malloc(MAXDNAME);
		/*
		if (nsList[*nsNum] == NULL) {
          printf("malloc failed\n");
	      sts = FALSE;
		  break; //exit(1);
		}
		*/
#ifdef TRACE_MODE
if (nSenderLog) {
  sprintf(str, "[%s] findNameServers:while():dn_expand\n", domain);
  printfTrace(str);
}
#endif
		if (dn_expand(response.buf, 
					  endOfMsg,
					  cp,
					  (u_char *)pnsList[*nsNum],
					  MAXDNAME)
					  < 0) {
		  if (bDebug)
            printf("dn_expand failed\n");
	      sts = FALSE;
		  break; //exit(1);
		}
		if (bDebug)
		  printf("Name Server(%d): %s\n", *nsNum, pnsList[*nsNum]);
		for (i = 0, dup = 0; (i < *nsNum) && !dup; i++)
		  dup = !strcasecmp(pnsList[i], pnsList[*nsNum]);
		if (!dup)
		  (*nsNum)++;
		/*
 	    if (dup)
		  free (pnsList[*nsNum]);
		else
		  (*nsNum)++;
		*/
		cp += dlen;
	  }
	}
#ifdef TRACE_MODE
if (nSenderLog) {
  sprintf(str, "[%s] findNameServers:sts=%s\n", domain, (sts ? "FALSE":"TRUE"));
  printfTrace(str);
}
#endif
    return sts;
}

BOOL queryMXRecode(char *mMsgNo, char *domain, char *pList[], struct _MX_RECODE mx[], int *mxNum, char *nsList[], int nsNum) {
#ifdef IPv6
	//int  Error;
	BOOL bAF;
#endif
#ifdef TRACE_MODE
	char str[LOG_BUFFER];
#endif
    BOOL sts = TRUE;
	union {
		HEADER hdr;
		u_char buf[PACKETSZ*4];
	} query, response;
	int responseLen, queryLen, nsN, n;

	u_char  *cp;
	u_char  *endOfMsg;
	u_short clss;
	u_short type;
	u_long  ttl;
	u_short dlen;

	//WSADATA  WSAData;
	//unsigned long ns;
	struct in_addr saveNsAddr[MAXNS];
	int nsCount;
	struct hostent *host;
	int i, count;//, dup;
    unsigned long inaddr;

//
/*
*mxNum = 1;
strcpy(mx[0].List, "mx.kawa.org");
return TRUE;
*/
////
     //nsCount = _res.nscount;
     nsCount = nsNum; //_res.nscount;
	 for (i = 0; i < nsCount; i++)
	   saveNsAddr[i] = _res.nsaddr_list[i].sin_addr;
     for (i = 0; i < nsCount; i++)
 	   _res.nsaddr_list[i].sin_addr = saveNsAddr[i];
     _res.options &= ~(RES_DNSRCH | RES_DEFNAMES);
	 for (nsN = 0; nsN < nsNum; nsN++) {
	   sts =TRUE;
       _res.options |= RES_RECURSE;
	   _res.retry = 4;
	   _res.nscount = nsCount;
#ifdef IPv6
	   if (bDebug)
	     printf("Query (%s:%s)\n",nsList[nsN], domain);

	   bAF = (strstr(nsList[nsN],":") ? 1 : 0);
       if (!bAF) {
         inaddr = inet_addr(nsList[nsN]); //IPアドレスかマシンアドレスかをチェック
         if (inaddr != INADDR_NONE) { // = (unsigned long)0xffffffff) {
           memcpy((void *)&_res.nsaddr_list[0].sin_addr, &inaddr, 4);
		 } else {
	       host = gethostbyname(nsList[nsN]);
           if (!host) {
			 if (bDebug)
               printf("There is no addres for %s\n", (const char FAR * )nsList[nsN]);
	         continue;
		   }
	       memcpy((void *)&_res.nsaddr_list[0].sin_addr,
		         (void *)host->h_addr_list[0], (size_t)host->h_length);
		 }
	   }
#else
       inaddr = inet_addr(nsList[nsN]); // IPアドレスかマシンアドレスかをチェック
       if (inaddr != INADDR_NONE) { // = (unsigned long)0xffffffff) {
         memcpy((void *)&_res.nsaddr_list[0].sin_addr,
	            &inaddr, 4);
	   } else {
	     host = gethostbyname(nsList[nsN]);
         if (!host) {
		   if (bDebug)
             printf("There is no addres for %s\n", (const char FAR * )nsList[nsN]);
	       continue;
		 }
	     memcpy((void *)&_res.nsaddr_list[0].sin_addr,
		       (void *)host->h_addr_list[0], (size_t)host->h_length);
	   }
#endif
#ifdef IPv6
  	   // LocalFree(host);
#endif
	   //_res.nscount = 1;
//	   _res.options &= ~RES_RECURSE; // 再帰的な問合せを有効にする。
	   _res.retry = 4; //2;
/*
printf("id =%d\n", query.hdr.id);
printf("rd =%d\n", query.hdr.rd);
printf("tc =%d\n", query.hdr.tc);
printf("aa =%d\n", query.hdr.aa);
printf("opcode =%d\n", query.hdr.opcode);
printf("qr =%d\n", query.hdr.qr);
printf("rcode =%d\n", query.hdr.rcode);
printf("cd =%d\n", query.hdr.cd);
printf("ad =%d\n", query.hdr.ad);
printf("unused =%d\n", query.hdr.unused);
printf("ra =%d\n", query.hdr.ra);
printf("qdcount =%d\n", query.hdr.qdcount);
printf("ancount =%d\n", query.hdr.ancount);
printf("nscount =%d\n", query.hdr.nscount);
printf("arcount =%d\n", query.hdr.arcount);
*/
#ifdef TRACE_MODE
if (nSenderLog) {
  sprintf(str, "[%s] (%s)->[%s] queryMXRecode:resomake query\n", mMsgNo, nsList[nsN], domain);
  printfTrace(str);
}
#endif
#ifdef UPDATE_20070319 // MXレコード参照でハングさせない対策
__try
{
	   queryLen = res_mkquery(QUERY,
		                      domain,
							  C_IN,
							  T_MX,
							  (char *)NULL,
							  0,
							  (struct rrec *)NULL,
							  (char *)&query,
							  sizeof(query));
}
__except(queryLen)
{
       queryLen = -1;
}
#else //UPDATE_20070319 // MXレコード参照でハングさせない対策
	   queryLen = res_mkquery(QUERY,
		                      domain,
							  C_IN,
							  T_MX,
							  (char *)NULL,
							  0,
							  (struct rrec *)NULL,
							  (char *)&query,
							  sizeof(query));
#endif
	   if (bDebug)
         printf("queryLen=%d\n", queryLen);
#ifdef TRACE_MODE
if (nSenderLog) {
  sprintf(str, "[%s] (%s)->[%s] queryMXRecode:res_send:Len=%d\n", mMsgNo, nsList[nsN], domain, queryLen);
  printfTrace(str);
}
#endif

	   //errno = 0;
#ifdef UPDATE_20070319 // MXレコード参照でハングさせない対策
#ifdef IPv6
__try
{
	if (queryLen > 0) {
	   responseLen = res_send((char *)&query,
		                            queryLen,
								   (char *)&response,
								   sizeof(response),
								   bAF);
	} else {
     responseLen = -1;
	}
}
__except(responseLen)
{
       responseLen = -1;
}
#else
__try
{
	if (queryLen > 0) {
	   responseLen = res_send((char *)&query,
		                            queryLen,
								   (char *)&response,
								   sizeof(response),
								   0);
	} else {
     responseLen = -1;
	}
}
__except(responseLen)
{
       responseLen = -1;
}
#endif //IPv6
#else  //UPDATE_20070319 // MXレコード参照でハングさせない対策
#ifdef IPv6
	   responseLen = res_send((char *)&query,
		                            queryLen,
								   (char *)&response,
								   sizeof(response),
								   bAF);
#else
	   responseLen = res_send((char *)&query,
		                            queryLen,
								   (char *)&response,
								   sizeof(response),
								   0);
#endif // IPv6
#endif // UPDATE_20070319
/*
printf("id =%d\n", response.hdr.id);
printf("rd =%d\n", response.hdr.rd);
printf("tc =%d\n", response.hdr.tc);
printf("aa =%d\n", response.hdr.aa);
printf("opcode =%d\n", response.hdr.opcode);
printf("qr =%d\n", response.hdr.qr);
printf("rcode =%d\n", response.hdr.rcode);
printf("cd =%d\n", response.hdr.cd);
printf("ad =%d\n", response.hdr.ad);
printf("unused =%d\n", response.hdr.unused);
printf("ra =%d\n", response.hdr.ra);
printf("qdcount =%d\n", response.hdr.qdcount);
printf("ancount =%d\n", response.hdr.ancount);
printf("nscount =%d\n", response.hdr.nscount);
printf("arcount =%d\n", response.hdr.arcount);
*/
	  //////////////////////////////
      if (responseLen < 0) {
		 if (errno == ECONNREFUSED) {
		   if (bDebug)
             printf("There is no name server running on %s\n", nsList[nsN]);
		 } else {
		   if (bDebug)
             printf("There was no response from %s\n", nsList[nsN]);
		 }
         continue;
	   }
	   //////////////////////////////
	   if (bDebug)
	     printf("responseLen=%d\n", responseLen);
       endOfMsg = response.buf + responseLen;
	   cp = response.buf + sizeof(HEADER);
	   if (response.hdr.rcode != NOERROR) {
		 returnCodeError((int)response.hdr.rcode, nsList[nsN]);
		 continue;
	   }
	   if (!response.hdr.aa) {
		 if (bDebug)
           printf("%s is not authpritaive for %s\n", nsList[nsN], domain);
		 //continue; 
	   }
	   cp += skipName(cp, endOfMsg) + QFIXEDSZ;
	   count = ntohs((u_short)response.hdr.ancount);
#ifdef TRACE_MODE
if (nSenderLog) {
  sprintf(str, "[%s] (%s)->[%s] queryMXRecode:while():(%sauthpritaive)/*mxNum=%d\n", mMsgNo, nsList[nsN], domain, (response.hdr.aa ? "" : "not "), *mxNum);
  printfTrace(str);
}
#endif
	   while( (--count >= 0) &&
	 	      (cp < endOfMsg) &&
		      (*mxNum < MXLIMIT) ) {
		 n = skipToData(cp, &type, &clss, &ttl, &dlen, endOfMsg);
		 if (bDebug)
           printf("skipToData=%d\n", n);
		 if (n < 0) {
     	   sts = FALSE;
		   break;
		 }
	     cp += n; //skipToData(cp, &type, &clss, &ttl, &dlen, endOfMsg);
	     if (type == T_MX) {
		   mx[*mxNum].npreference = _getshort(cp);
		   cp += 2; //sizeof(u_short);
           //mx[*mxNum].List= (char *) malloc(MAXDNAME);
		   /*
		   if (mx[*mxNum].List == NULL) {
             printf("malloc failed\n");
	         sts = FALSE;
 		     break; //exit(1);
		   }
		   */
#ifdef TRACE_MODE
if (nSenderLog) {
  sprintf(str, "[%s] (%s)->[%s] queryMXRecode:dn_expand:*mxNum=%d\n", mMsgNo, nsList[nsN], domain, *mxNum);
  printfTrace(str);
}
#endif
		   if ((n = dn_expand(response.buf, 
		  			     endOfMsg,
					     cp,
					     (u_char *)pList[*mxNum], //mx[*mxNum].List,
					     MAXDNAME) )
					     < 0) {
			 if (bDebug)
               printf("dn_expand failed\n");
       	     sts = FALSE;
		     break; //exit(1);
		   }
		   cp += n;
		   (*mxNum)++;
/*
		   for (i = 0, dup = 0; (i < *mxNum) && !dup; i++)
		     dup = !strcasecmp(mx[i].List, mx[*mxNum].List);
		   if (!dup)
 		     (*mxNum)++;
 	       //if (dup)
		     //free (mx[*mxNum].List);
		   //else {
             ///printf("%s has MX Recode %s\n", nsList[nsN], mx[*mxNum].List);
 		     //(*mxNum)++;
		   //}
		   cp += dlen;
		   
  //printf("queryMXRecode:res_send:domain=%s/queryLen=%d\n", domain, queryLen);
#ifdef TRACE_MODE
if (nSenderLog) {
  sprintf(str, "(%s)->[%s] queryMXRecode:res_send:queryLen=%d\n", nsList[nsN], domain, queryLen);
  printfTrace(str);
}
#endif
#ifdef IPv6
	       responseLen = res_send((char *)&query,
		                            queryLen,
								   (char *)&response,
								   sizeof(response),
								   bAF);
#else
	       responseLen = res_send((char *)&query,
		                            queryLen,
								   (char *)&response,
								   sizeof(response),
								   0);
#endif
           endOfMsg = response.buf + responseLen;
	       cp = response.buf + sizeof(HEADER);
	       cp += skipName(cp, endOfMsg) + QFIXEDSZ;
*/
           //printf("[%d]\n", count);
		 }
	   }
	   if (*mxNum) // MXレコードが検出されたなら次のNSへの問合せはしない。
	     break;
	 }
	 //printf("queryMXRecode:sts=%s\n", (sts ? "TRUE" : "FALSE"));
#ifdef TRACE_MODE
if (nSenderLog) {
  sprintf(str, "[%s] [%s] queryMXRecode:sts=%s\n", mMsgNo, domain, (sts ? "TRUE" : "FALSE"));
  printfTrace(str);
}
#endif
	 return sts;
}

#ifdef NEW_MXQUERY // MX問合せのリトライ処理を追加
BOOL queryMXRecode2(char *mMsgNo, char *pDomain, char *pNS, char *pMX, int *mxNum) 
{
  BOOL bsts = FALSE;
  int  i = 0;
  CHAR mQNS[MAXDNAME] = "";
  char *p, *q;
  char DnsServIp[255];             //DNS server ip address.
  DNS_STATUS status;               //Return value of  DnsQuery_A() function.
  PDNS_RECORD pDnsRecord, pDns;          //Pointer to DNS_RECORD structure.
  PIP4_ARRAY pSrvList = NULL;      //Pointer to IP4_ARRAY structure.
  DNS_FREE_TYPE freetype =  DnsFreeRecordListDeep;
#ifdef TRACE_MODE
	char str[LOG_BUFFER];
#endif
   ///////////////////////////////////
   // MXレコード
   *pMX = NULL;
   if (*pNS)
   {
     // Allocate memory for IP4_ARRAY structure.
     strcpy(mQNS, pNS);
     p = mQNS;
     if ((pSrvList = (PIP4_ARRAY) LocalAlloc(LPTR, sizeof(IP4_ARRAY))))
	 {
       //strcpy(DnsServIp, pNS);
	   //strtok(DnsServIp, "\n\r ");
       pSrvList->AddrCount = 1;
       //pSrvList->AddrArray[0] = inet_addr(DnsServIp); //DNS server IP address
	   do {
         if ((q = strpbrk(p, "\r\n ")))
		 {
	       *q = '\x0';
		 }
	     pSrvList->AddrArray[0] = inet_addr(p); //DNS server IP address
         ///////////////
         if (!(status = DnsQuery(pDomain,       //Pointer to OwnerName. 
                      DNS_TYPE_MX,                      //Type of the record to be queried.
                      DNS_QUERY_BYPASS_CACHE, //DNS_QUERY_STANDARD,     // Bypasses the resolver cache on the lookup. 
                      pSrvList,                   //Contains DNS server IP address.
                      &pDnsRecord,                //Resource record that contains the response.
                      NULL)))
		 {      // 見つかった
	       pDns = pDnsRecord;
	       while(pDns)
		   {
	         if (pDns->wType == DNS_TYPE_MX) {
 		       *mxNum++;
	           strcpy(pMX, (const char *)pDns->Data.MX.pNameExchange);
		       break; // 最初に見つかったレコード
			 }
             pDns = pDns->pNext;
		   }
	       DnsRecordListFree(pDnsRecord, freetype);
		 }
		 // NEXT
	     if (q)
		 {
	       p = q+1;
		 } else {
		   *p = '\x0';
		 }
	   } while(*p);
       LocalFree(pSrvList);
	 }
   }
#ifdef TRACE_MODE
if (nSenderLog) {
  sprintf(str, "[%s] [%s] queryMXRecode2:sts=%s\n", mMsgNo, pDomain, (*pMX ? pMX : FALSE));
  printfTrace(str);
}
#endif
   return (*pMX ? TRUE : FALSE);
}
#endif

int skipName(u_char *cp, u_char *endOfMsg) {
   int n;

   if ( (n = dn_skipname(cp, endOfMsg) ) < 0 ) {
	 if (bDebug)
       printf("dn_skipname failed\n");
	 //exit(1);
   }
   return (n);
}

int skipToData(u_char *cp, u_short *type, u_short *clss, u_long *ttl, u_short *dlen, u_char *endOfMsg) {
   u_char *tmp_cp = cp;
   int    n;

   n = skipName(tmp_cp, endOfMsg);
   if (n < 0)
	 return -1;

   tmp_cp += n; //skipName(tmp_cp, endOfMsg);

   *type = _getshort(tmp_cp);
   tmp_cp += 2; //sizeof(u_short);
   *clss = _getshort(tmp_cp);
   tmp_cp += 2; //sizeof(u_short);
   *ttl  = _getlong(tmp_cp);
   tmp_cp += 4; //sizeof(u_long);
   *dlen = _getshort(tmp_cp);
   tmp_cp += 2; //sizeof(u_short);
   
   return (tmp_cp - cp);
}

void nsError(int error, char *domain) {
  char str[LOG_BUFFER];

  switch (error) {
    case HOST_NOT_FOUND: sprintf(str, "[%s] Unknown domain\n", domain); break;
    case NO_DATA:        sprintf(str, "[%s] No NS records\n", domain); break;
    case TRY_AGAIN:      sprintf(str, "[%s] Bo response for NS query\n", domain); break;
    default:             sprintf(str, "[%s] Unexpected error\n", domain); break;
  }
  if (bDebug)
    printf("%s", str);
#ifdef TRACE_MODE
if (nSenderLog) {
  printfTrace(str);
}
#endif
}
       
void returnCodeError(int rcode, char *nameserver) {

  if (bDebug)
    printf( "%s: ", nameserver);
  switch (rcode) {
    case FORMERR:  if (bDebug) printf("FORMERR response\n"); break;
	case SERVFAIL: if (bDebug) printf("SERVFAIL response\n"); break;
	case NXDOMAIN: if (bDebug) printf("NXDOMAIN response\n"); break;
	case NOTIMP:   if (bDebug) printf("NOTIMP response\n"); break; 
	case REFUSED:  if (bDebug) printf("REFUSED response\n"); break; 
    default:       if (bDebug) printf("Unexpected error\n"); break;
  }
}