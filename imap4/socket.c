////////////////////////////////////////////////////////////
// Socket.c Copyright K.kawakami
////////////////////////////////////////////////////////////
#include "imap4.h"
#include "version.h"
#include <process.h>

#pragma hdrstop

#ifdef UPDATE_20171211A // 使用可能な暗号化通信方式のバージョンを規定するオプション 0x1:SSL2禁止 0x2:SSL3禁止 0x4:TSL1禁止 0x8:TSL1.1禁止 0x10:TSL1.2禁止
extern unsigned long nSecuerLayOption;
#endif
#ifdef UPDATE_20190426 // 利用する暗号アルゴリズムの組み合わせオプション 
extern char mSecuerLayCipher[];
#endif
#ifdef UPDATE_20240420 // 暗号化通信方式のセキュリティレベルの設定
extern int  nSSLSecureLevel; // デフォルト=0
#endif
#ifdef LDAP_ON         // LDAPへのログイン処理でSMTP認証の確認を行うオプション // //CRAM-MD5は複合できないので、これを利用する場合は、PLAINかLOGINのみとします。
extern BOOL   bLDAP;
extern BOOL   bLDAPLicense;
#endif
#ifdef SOCKET_FAIL_TEST // 強制テスト：テスト的にエラーを発生 ソケット無応答状態の動作調査用
extern BOOL  bFT;
#endif
extern INT   nMAXUser;
extern int   nSendBuf;
extern BOOL  bDebug;
extern CHAR  mSharedRoot[];
extern BOOL  bSharedRW;
extern BOOL  bSequenceOK; // シーケンス実行許可
extern int   nSendTMO;
extern DWORD nAcceptCount;
extern DWORD nAcceptLimit;
extern int   nAddressFamily;
extern int   nConnectAF;
extern char  mHostName[];
extern BOOL bServiceTerminating;
extern SOCKET sListener;
extern SOCKADDR_IN sinListener;
extern int nport, nsinListener;
extern char mdomain[];
extern char mMailSpoolDir[];
extern BOOL  bListenMode;
extern char  mListenIP[];
extern char  mListenIP2[];
extern DWORD nLastMsgId;
extern BOOL  bServiceTerminating;
extern BOOL  bCountLock;
extern BOOL  bAcceptLog;
extern BOOL  bTMQWait;
extern DWORD nTMOut;
extern BOOL  bThreadWait;
extern char  mLocalDomain[];
#ifdef USE_SSL
extern BOOL  bUsedSSL;
extern char    mCertificate[256];
extern char    mPrivatekey[256];
#endif
#ifdef UPDATE_20060315 // 隠すIPアドレス
extern char    mHiddenMyIPFile[];
#endif
#ifdef UPDATE_20151122 // 特定接続元IPからの接続拒絶用テーブル追加(rejectimap4ip.dat)隠しオプション
extern char  mRejectPeerPFile[];
#endif
#ifdef UPDATE_20151127 // リッスンＩＰ毎に証明書を選択できる対策(sslbyip.datファイルで指定)
extern char    mProgPath[];
#endif

#ifdef V4
extern BOOL    bHide;
#endif
#ifdef UPDATE_20151115 // SELECT時にメール一覧をキャッシュするためのポインタ
extern BOOL    bSELECT;   // SELECTによるファイル一覧キャッシュ化
#endif
#ifdef UPDATE_20160114 // 接続拒絶用テーブルの指定に有効時間を設定可能にした。
extern DWORD	nAuthLockOutTime; // ロックアウト時間 デフォルト 0:無限 分単位
#endif
#ifdef UPDATE_20190216 // IP毎の同時接続ロックアウト機能を追加
extern DWORD	nIPLockOut; // IPロックアウトまでの回数 デフォルト 0:無効
extern DWORD	nIPLockOutTime; // ロックアウト時間 デフォルト 0:無限 分単位
#ifdef UPDATE_20190218 // IP毎の同時接続ロックアウト機能を強化（サンプル時間と拒絶時間の設定を追加）
extern DWORD	nIPLockOutSPTime; // IPロックアウトまでのサンプリング時間 デフォルト 0:無限 秒単位
#endif
#endif
char    mTraceFile[256];
FILE    *fTrace;
#ifdef Y2038_BUG
extern  __int64  ltbegin;
#else
extern  time_t  ltbegin;
#endif
char    uapoppass[201];
BOOL    bLimit;
#ifdef IPv6
SOCKET  sLsn[2];
#endif
SOCKET  *psLsn;
//int         nsinListener;
//#define TRACE 1

#ifdef Y2038_BUG
void gettime(SYSTEMTIME *ltime, char *mt);
#else
void gettime(time_t *ltime, char *mt);
#endif
//PVOID CreatePOP3SContext(void);
//DWORD WorkerThread (LPVOID WorkContext);
#ifdef Y2038_BUG
BOOL GetTimeLimit(__int64 *ltbegin, char *mode, BOOL *bPassport);
#else
BOOL GetTimeLimit(time_t *ltbegin, char *mode, BOOL *bPassport);
#endif
void MDString (char *string, char *dest);
void NormalString (char *MDstring, char *dest);
VOID  CloseClient (PCLIENT_CONTEXT lpClientContext,BOOL bGraceful, BOOL bTMQ);
int WSAGetLastErrorName(void);
#ifdef ACCEPT_LOG_LEVEL3
void LEVEL_3_ACCEPTLOG(PCLIENT_CONTEXT lpClientContext, char *mess);
#endif
BOOL IMAP4Main(PCLIENT_CONTEXT lpClientContext);
BOOL Imap4_Dispatch(PCLIENT_CONTEXT lpClientContext);
#ifdef LDAP_OPTION_LICENSE // LDAP Optionのライセンスチェックを行なう
BOOL LDAP_Option_Check(void);
#endif
#ifdef UPDATE_20141220 // 操作フォルダをアクセス中のセッションに変化をリアルタイムに同報するための対策
extern BOOL  bBroadcastSession; // 同一フォルダの操作情報を接続中のセッションに同報するオプション TRUE:有効 FALSE:無効
void JoinBroadcastSession(PCLIENT_CONTEXT lpClientContext);
void LeaveBroadcastSession(PCLIENT_CONTEXT lpClientContext);
#endif
#ifdef UPDATE_20141221 // セッション内でのメッセージ送信の排他処理
void  put_reply_check(BOOL *pPutLock);
#endif

#ifdef UPDATE_20151127 // リッスンＩＰ毎に証明書を選択できる対策(sslbyip.datファイルで指定)
BOOL GetSSLbyIP(CHAR *pMyAddr, int nType, CHAR *pCertificate, CHAR *pPrivatekey)
{
   FILE *fp;
   CHAR mFn[256];
   CHAR *p, *q, mData[1024];
   BOOL bSts = FALSE;

   *pCertificate = *pPrivatekey = '\x0';
   sprintf(mFn,"%ssslbyip.dat", mProgPath); // ORDB.DAT　問合せサイトテーブル
   if ((fp = fopen(mFn, "rt"))) 
   {
	 while(fgets(mData, sizeof(mData), fp))
	 {
	   	strtok(mData, "\r\n");
		if (mData[0] != '\x0' &&
		    mData[0] != ' ' &&
		    mData[0] != '\'') {  // コメントチェック
		}
		//if (!strnicmp(mData, pMyAddr, strlen(pMyAddr)))
        if ((p = strchr(mData, ',')))
		{
		  *p = '\x0';
		}
	    if (IP_COMP(mData, pMyAddr))
		{
           if (p) //if ((p = strchr(mData, ',')))
		   {
			 bSts = (BOOL)atoi((CHAR *)(p+(nType*2)+1));
			 p+=7; // プロトコルによる有効無効
			 if ((q = strchr(p, ',')))
			 {
			    *q = '\x0';
				q++;
				while(*q == '"')
				{
				  q++;
				}
			    strcpy(pPrivatekey, q);
				strtok(pPrivatekey, "\"");
			 }
			 while(*p == '"')
			 {
			   p++;
			 }
			 strcpy(pCertificate, p);
			 strtok(pCertificate, "\"");
		   }
		   break;
		}
	 }
	 fclose(fp);
   }
   return bSts;
}
#endif

#ifdef UPDATE_20060315 // 隠すIPアドレス
#ifdef UPDATE_20151122 // 特定接続元IPからの接続拒絶用テーブル追加(rejectimap4ip.dat)隠しオプション
BOOL HiddenMyIP(char *pAddr, int nType)
#else
BOOL HiddenMyIP(char *pAddr)
#endif
{
  FILE *fp;
  int  i, n, mask, num, addr, start, dot;
  char *p, *pwild, *pmask, mwork[128];
  BOOL bsts = FALSE; // 一致するものが無い
#ifdef UPDATE_20160114 // 接続拒絶用テーブルの指定に有効時間を設定可能にした。
  char *q = NULL;
  SYSTEMTIME st;
  FILETIME   ft;
  ULARGE_INTEGER *u1;
  unsigned __int64 ltn, lts;
#ifdef UPDATE_20190216 // IP毎の同時接続ロックアウト機能を追加
  unsigned __int64 lttm;
  char *r = NULL;
#else
  unsigned __int64 lttm = (unsigned __int64)600000000 * (unsigned __int64)nAuthLockOutTime; // 分単位
#endif
#endif
#ifdef UPDATE_20160830 // 接続拒絶用テーブルの指定にホワイトリストの指定を追加した。
  BOOL bReject = TRUE;
#endif
#ifdef UPDATE_20190216 // IP毎の同時接続ロックアウト機能を追加
  lttm = (unsigned __int64)(nType == 1 ? 600000000 : 10000000) * (unsigned __int64)(nType == 1 ? nAuthLockOutTime : nIPLockOutTime); // nAuthLockOutTime(分単位) nIPLockOutTime(秒単位)
#endif
#ifdef UPDATE_20151122 // 特定接続元IPからの接続拒絶用テーブル追加(rejectimap4ip.dat)隠しオプション
  if (bDebug) printf("HiddenMyIP(%s, %d, %s)\n", pAddr, nType, (nType == 0 ? mHiddenMyIPFile : mRejectPeerPFile));
  if ((fp = fopen((nType == 0 ? mHiddenMyIPFile : mRejectPeerPFile) , "rt"))) 
#else
  if ((fp = fopen(mHiddenMyIPFile, "rt"))) 
#endif
  {
#ifdef UPDATE_20160114 // 接続拒絶用テーブルの指定に有効時間を設定可能にした。
    if (nType)
	{
      GetSystemTime(&st);
      SystemTimeToFileTime(&st, &ft);
      u1 = (ULARGE_INTEGER *)&ft;
      ltn = u1->QuadPart;
	}
#endif
	p = fgets(mwork, sizeof(mwork), fp); // dummy
	while(p || !feof(fp)) {
	  p = strpbrk(mwork, "' \r\n");
	  if (p)
		*p = '\x0';
#ifdef UPDATE_20160114 // 接続拒絶用テーブルの指定に有効時間を設定可能にした。
	  if (nType)
	  {
#ifdef UPDATE_20160830 // 接続拒絶用テーブルの指定にホワイトリストの指定を追加した。
        bReject = TRUE;
#endif
	    q = strpbrk(mwork, "\t");
	    if (q)
		{
		  *q = '\x0';
		  q++;
#ifdef UPDATE_20160830 // 接続拒絶用テーブルの指定にホワイトリストの指定を追加した。
          if (!stricmp(q, "true"))
		  {
			bReject = FALSE;
		  } 
#endif
		  lts = (unsigned __int64)_atoi64(q);
#ifdef UPDATE_20190216 // IP毎の同時接続ロックアウト機能を追加
		  if (r = strpbrk(q, "\t"))
		  {
            if (r && nType == 1)
			{
              mwork[0] = '\''; // コメント扱い
			}
		  } 
#endif
		}
	  }
#endif
	  if (mwork[0] != '\x0' &&
	      mwork[0] != ' ' &&
	      mwork[0] != '\'') {  // コメントチェック
#ifdef UPDATE_20160114 // 接続拒絶用テーブルの指定に有効時間を設定可能にした。
	    if (nType)
		{
	      if (q)
		  {
		    if (lttm == 0 || lts == 0 || ltn < (unsigned __int64)lts+lttm)
			{
	          if ((bsts = IP_COMP(mwork, pAddr))) // 一致するものがあった
			  {
#ifdef UPDATE_20160830 // 接続拒絶用テーブルの指定にホワイトリストの指定を追加した。
                bsts &= bReject; // true指定があれば拒否しない
#endif
	 	        break;
			  }
			}
		  } else {
	        if ((bsts = IP_COMP(mwork, pAddr))) // 一致するものがあった
	 	      break;
		  }
		}
	    else 
		{
	      if ((bsts = IP_COMP(mwork, pAddr))) // 一致するものがあった
	 	   break;
		}
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

#ifdef IPv6
void
DumpHostEntry(struct hostent *HostEntry)
{
    char **Item;
    int Count;
    char Buffer[INET6_ADDRSTRLEN];  // INET6_ADDRSTRLEN > INET_ADDRSTRLEN.

    if (HostEntry != NULL) {
        if (bDebug) printf("HostEntry->h_name = %s\n", HostEntry->h_name);
        for (Item = HostEntry->h_aliases, Count = 0; *Item != NULL; Item++) {
            if (bDebug) printf("HostEntry->h_aliases[%d] = %s\n", Count++, *Item);
        }
        if (bDebug) printf("HostEntry->h_addrtype = %u\n", HostEntry->h_addrtype);
        if (bDebug) printf("HostEntry->h_length = %u\n", HostEntry->h_length);
        for (Item = HostEntry->h_addr_list, Count = 0; *Item != NULL; Item++) {
            if (bDebug) printf("HostEntry->h_addr_list[%d] = %s\n", Count++,
                   inet_ntop(HostEntry->h_addrtype, *Item, Buffer,
                             sizeof Buffer));
        }

    } else {
        if (bDebug) printf("HostEntry = (null)\n");
    }
}
#endif

BOOL gethostaddrname(char *hostname) {
  char szHostName[256];
  int  iResult;
#ifdef IPv6
//  char Buffer[INET6_ADDRSTRLEN];
//  char **Item;
  PHOSTENT host = NULL;
  int Error;
#endif
  HOSTENT FAR * lphostent = NULL;
#ifndef VC6
  HOSTENT hst;
  struct addrinfo Hints, *AddrInfo;
#endif

  strcpy(hostname,"");
  if (mHostName[0] == '\x0') {
    iResult = gethostname(szHostName, sizeof(szHostName));
    if (iResult != 0)
	  return FALSE;
  } else {
	strcpy(szHostName, mHostName);
  }
  if (lstrcmp(szHostName,"") == 0)
	return FALSE;
#ifdef IPv6
  Error = 0;
#ifdef VC6
  lphostent = getipnodebyname(szHostName, (nAddressFamily == 1 ? AF_INET6 : AF_INET),  (nAddressFamily ? AI_ALL : 0), &Error);
#else
  lphostent = NULL;
  memset(&Hints, 0, sizeof(Hints));
  Hints.ai_flags = AI_CANONNAME;
  Hints.ai_family =  (nAddressFamily == 1 ? AF_INET6 : AF_INET);
  Hints.ai_socktype = SOCK_STREAM;
  if (getaddrinfo(szHostName, NULL, &Hints, &AddrInfo) == 0) {
    hst.h_name = AddrInfo->ai_canonname;
 	lphostent = &hst;
  }
#endif
  if (!lphostent) {
    gethostname(szHostName, sizeof(szHostName));
#ifdef VC6
    lphostent = getipnodebyname(szHostName, (nAddressFamily == 1 ? AF_INET6 : AF_INET),  (nAddressFamily ? AI_ALL : 0), &Error);
#else
    lphostent = NULL;
    memset(&Hints, 0, sizeof(Hints));
    Hints.ai_flags = AI_CANONNAME;
    Hints.ai_family =  (nAddressFamily == 1 ? AF_INET6 : AF_INET);
    Hints.ai_socktype = SOCK_STREAM;
    if (getaddrinfo(szHostName, NULL, &Hints, &AddrInfo) == 0) {
      hst.h_name = AddrInfo->ai_canonname;
      lphostent = &hst;
	}
#endif
    if (!lphostent)
	  return FALSE;
  } 
  strcpy(hostname, lphostent->h_name);
#ifdef UPDATE_20060515 // SOCKET関連のメモリ開放抜け
   if (lphostent)
#ifdef VC6
     freehostent(lphostent);
#else
     freeaddrinfo(AddrInfo);
#endif
#else
  if (LocalFlags(lphostent) != LMEM_INVALID_HANDLE) // 解放可能かどうか確認
    LocalFree(lphostent);
#endif
  return TRUE;
#else
  lphostent = gethostbyname(szHostName);
  if (!lphostent) {
    gethostname(szHostName, sizeof(szHostName));
    lphostent = gethostbyname(szHostName);
    if (!lphostent)
	  return FALSE;
  }
  strcpy(hostname, lphostent->h_name);
  return TRUE;
#endif
}

BOOL AcceptClients (HANDLE hCompletionPort) {
  	char  mSts[128];
    PCLIENT_CONTEXT lpClientContext;
	BOOL   bPassport = FALSE;
    SOCKET s;
    int    nSndsize, nl;
#ifdef IPv6
    char *Address = NULL;
    struct addrinfo Hints, *AddrInfo, *AI;
	SOCKADDR_IN  sinm, sinListener;
	SOCKADDR_IN6 sinm6, sinListener6;
	char Out6[INET6_ADDRSTRLEN];
	char mport[16];
	int i, j, k, l;
#else
    SOCKADDR_IN sin, sinListener;
	int i;
#endif
    int   nwsaerr;
    char  mWSAMean[256];
    int   timeout;
    int   err; //, nsinListener;
	int   nIPv4, nIPv6;
	char  *pldom, *lport;
	char  mmode[64];
    char  mess[64]; 
    PImap4Context pContext;
    int   Error;
	char  *pnet;

    fd_set fds;
#ifdef SEND_SELCET
    fd_set fdw;
#endif
#ifndef VC6
    HOSTENT hst;
#endif
#ifdef UPDATE_20151028 // セッションスレッドの作成に失敗した時の対策
	char  mec[128];
#endif
#ifndef UPDATE_20151127 // リッスンＩＰ毎に証明書を選択できる対策(sslbyip.datファイルで指定)
#ifdef UPDATE_20151126 // リッスンＩＰ毎に証明書を選択できる対策
	CHAR  mSSLKey[256];
	CHAR  mSSLKeyName[256];
#endif
#endif

    if (bListenMode) {  // 特定IPのみ
  	  nIPv4 = nIPv6 = 0;
      pldom = mListenIP;
      while (strlen(pldom)){
	    if (strstr(pldom,".")) {
          nIPv4++;
		} else if (strstr(pldom,":")) {
          nIPv6++;
		}
        pldom += strlen(pldom);
        pldom++;
	  };
	} else {  // 全IP
  	  nIPv4 = nIPv6 = 1;
	}
#ifdef IPv6
	switch (nAddressFamily) {
	  case 0: // IPv4 only
              psLsn = LocalAlloc( LPTR, sizeof(SOCKET)*nIPv4 );
			  k = nIPv4;
			  break;
	  case 1: // IPv6 only
	          psLsn = LocalAlloc( LPTR, sizeof(SOCKET)*nIPv6 );
			  k = nIPv6;
			  break;
	  case 2: // IPv4/6
              psLsn = LocalAlloc( LPTR, sizeof(SOCKET)*(nIPv4+nIPv6) );
			  k = nIPv4;
			  break;
	}
#else
    psLsn = LocalAlloc( LPTR, sizeof(SOCKET)*nIPv4 );
#endif

#ifdef IPv6
	l = 0;
	for (i = 1; i <= (nAddressFamily == 2 ? 2 : 1); i++) {
      pldom = mListenIP;
	  for (j = 0; j < (i == 1 ? k : nIPv6) ; j++) {
        memset(&Hints, 0, sizeof(Hints));
        Hints.ai_family = (i == 1 ? ((nAddressFamily == 0 || nAddressFamily == 2) ? AF_INET : AF_INET6) : AF_INET6);
        Hints.ai_socktype = SOCK_STREAM;
        Hints.ai_flags = AI_NUMERICHOST | AI_PASSIVE;
	    itoa(nport, mport, 10);
        if (getaddrinfo(Address, mport, &Hints, &AddrInfo) != 0) {
          if (bDebug) printf("getaddrinfo failed with error\n");
          return FALSE;
		}
	    AI = AddrInfo;
        psLsn[l] = socket(AI->ai_family, AI->ai_socktype, AI->ai_protocol);
        if ( psLsn[l] == INVALID_SOCKET )
          return FALSE;
        /////////////////////////////////////////////////////
        if (bListenMode) {  // 特定IPのみ
	      if (Hints.ai_family == AF_INET) {
            while (strlen(pldom)){
              if (strstr(pldom,"."))
                break;
              pldom += strlen(pldom);
              pldom++;
			};
	        if (strstr(pldom,".")) {
		      memcpy(&sinm, AI->ai_addr, sizeof(SOCKADDR_IN));
	          lport = strstr(pldom, " ");
	          if (lport) {
	            *lport = '\x0';
		        lport++;
                sinm.sin_port = htons( (u_short) atol(lport) );
#ifdef UPDATE_20061011 // ソケット処理のselect()がエラーとして返えった後、以降のソケットのリッスンが出来なくなる可能性の対策。
                *(lport-1) = ' '; // 復帰
#endif
			  } else
                sinm.sin_port = htons( (u_short) nport );
#ifdef CLUSTERING
			  if (*pldom == '*')
                sinm.sin_addr.s_addr = INADDR_ANY;
			  else
#endif
              sinm.sin_addr.s_addr = inet_addr(pldom);
		      memcpy(AI->ai_addr, &sinm, sizeof(SOCKADDR_IN));
			}
		  } else {
            while (strlen(pldom)){
              if (strstr(pldom,":"))
                break;
              pldom += strlen(pldom);
              pldom++;
			};
	        if (strstr(pldom,":")) {
		      memcpy(&sinm6, AI->ai_addr, sizeof(SOCKADDR_IN6));
	          lport = strstr(pldom, " ");
	          if (lport) {
	            *lport = '\x0';
		        lport++;
                sinm6.sin6_port = htons( (u_short) atol(lport) );
#ifdef UPDATE_20061011 // ソケット処理のselect()がエラーとして返えった後、以降のソケットのリッスンが出来なくなる可能性の対策。
                *(lport-1) = ' '; // 復帰
#endif
			  } else
                sinm6.sin6_port = htons( (u_short) nport );
#ifdef UPDATE_20211213 // 複数のIPv6アドレスが指定されているとサービス起動に失敗する
			  {
				  char *psp = strchr(pldom, ' ');
				  if (psp)
				    *psp='\x0';
#endif
#ifdef VC6
		      inet6_addr(pldom, &sinm6.sin6_addr);
#else
		      inet_pton(AF_INET6, pldom, &sinm6.sin6_addr);
#endif
		      memcpy(AI->ai_addr, &sinm6, sizeof(SOCKADDR_IN6));
#ifdef UPDATE_20211213 // 複数のIPv6アドレスが指定されているとサービス起動に失敗する
				  if (psp)
				    *psp=' ';
			  }
#endif
			}
		  }
	      pldom += strlen(pldom);
          pldom++;
		}
        /////////////////////////////////////////////////////
        if (bind(psLsn[l], AI->ai_addr, AI->ai_addrlen) == SOCKET_ERROR) {
          if (bDebug) printf("bind() failed with error\n");
          closesocket( psLsn[l] );
          return FALSE;
		} 
		l++;
	  }
	}
#else
    pldom = mListenIP;
	for (i = 0; i < nIPv4; i++) {
      psLsn[i] = socket( AF_INET, SOCK_STREAM, 0 );
      if ( psLsn[i] == INVALID_SOCKET )
        return FALSE;
      sin.sin_family = AF_INET;
      /////////////////////////////////////////////////////
      if (bListenMode) {  // 特定IPのみ
        //pldom = mListenIP;
        while (strlen(pldom)){
          if (strstr(pldom,"."))
            break;
          pldom += strlen(pldom);
          pldom++;
		};
	    if (strstr(pldom,".")) {
	      lport = strstr(pldom, " ");
	      if (lport) {
	        *lport = '\x0';
		    lport++;
            sin.sin_port = htons( (u_short) atol(lport) );
#ifdef UPDATE_20061011 // ソケット処理のselect()がエラーとして返えった後、以降のソケットのリッスンが出来なくなる可能性の対策。
            *(lport-1) = ' '; // 復帰
#endif
		  } else
            sin.sin_port = htons( (u_short) nport );
          sin.sin_addr.s_addr = inet_addr(pldom);
		}
	    pldom += strlen(pldom);
        pldom++;
	  } else {            // 全てのアサインされたIP
        sin.sin_port = htons( (u_short) nport );
        sin.sin_addr.s_addr = INADDR_ANY;
	  }
      /////////////////////////////////////////////////////
      //err = bind( sListener, (LPSOCKADDR)&sin, sizeof(sin) );
      err = bind( psLsn[i], (LPSOCKADDR)&sin, sizeof(sin) );
      if ( err == SOCKET_ERROR ) {
        closesocket( sListener );
        return FALSE;
	  }
	}
#endif

    // Listen for incoming connections on the socket.
#ifdef IPv6
    switch (nAddressFamily) {
      case 0: // IPv4 Only
	         for (i = 0; i < nIPv4; i++) {
               err = listen( *(psLsn+i), SOMAXCONN); //0);
               if ( err == SOCKET_ERROR ) {
                 closesocket( sListener );
                return FALSE;
			   }
			 }
			 break;
      case 1: // IPv6 Only
	         for (i = 0; i < nIPv6; i++) {
               err = listen( *(psLsn+i), SOMAXCONN); //0);
               if ( err == SOCKET_ERROR ) {
                 closesocket( sListener );
                return FALSE;
			   }
			 }
			 break;
  	  case 2: // IPv4/6
	         for (i = 0; i < nIPv4+nIPv6; i++) {
               err = listen( *(psLsn+i), SOMAXCONN); //0);
               if ( err == SOCKET_ERROR ) {
                 closesocket( sListener );
                return FALSE;
			   }
			 }
			 break;
	}
#else
	for (i = 0; i < nIPv4; i++) {
      err = listen( *(psLsn+i), SOMAXCONN); //0); //5 );
      if ( err == SOCKET_ERROR ) {
        closesocket( sListener );
        return FALSE;
	  }
	}
#endif
    gethostaddrname(mdomain);
    ///////////////////////////////////
	// ドメインが無いときに最初の管理ドメイン名を付加
	if (!strchr(mdomain, '.')) {
	  strcat(mdomain, ".");
	  strcat(mdomain, mLocalDomain);
	}
    ///////////////////////////////////
	printf("host.domain=[%s]\n", mdomain);
    // Loop forever accepting connections from clients.
	bThreadWait = bTMQWait = FALSE;
	srand( (unsigned)time( NULL ) );
    while ( TRUE ) {
#ifdef IPv6
	    /////////////////////////
        //fd_set fds;
	    i = nAddressFamily;
        FD_ZERO(&fds);
		for (j = 0; j < (i == 0 ? nIPv4 : (i == 1 ? nIPv6 : nIPv4+nIPv6 ) ); j++) {
          if (FD_ISSET(*(psLsn+j), &fds))
	        break;
		}
		for (j = 0; j < (i == 0 ? nIPv4 : (i == 1 ? nIPv6 : nIPv4+nIPv6 ) ); j++)
          FD_SET(*(psLsn+j), &fds);
	    if (bDebug) printf("wait select()\n");
#ifdef ACCEPT_LOG_LEVEL3
          LEVEL_3_ACCEPTLOG(NULL, "wait select()");
#endif
        if (select(0, &fds, 0, 0, 0) == SOCKET_ERROR) {
		  nwsaerr = WSAGetLastError();
		  strcpy(mWSAMean, "select() failed. error = ");
		  switch(nwsaerr) {
	        case WSANOTINITIALISED: strcat(mWSAMean, "WSANOTINITIALISED");
				                    break;
	  	    case WSAEFAULT:         strcat(mWSAMean, "WSAEFAULT");
				                    break;
	  	    case WSAENETDOWN:       strcat(mWSAMean, "WSAENETDOWN");
				                    break;
	  	    case WSAEINVAL:         strcat(mWSAMean, "WSAEINVAL");
				                    break;
	  	    case WSAEINTR:          strcat(mWSAMean, "WSAEINTR");
				                    break;
	  	    case WSAEINPROGRESS:    strcat(mWSAMean, "WSAEINPROGRESS");
				                    break;
	  	    case WSAENOTSOCK:       strcat(mWSAMean, "WSAENOTSOCK");
				                    break;
            default: strcat(mWSAMean, "WSA OTHER");
				     break;
		  }
#ifdef E_POST
   	      AddToMessageLog(TEXT(mWSAMean), 100, TEXT("EPSTIMAP4S"), EVENTLOG_WARNING_TYPE );
#endif
          if (bDebug) printf("%s\n", mWSAMean);
#ifdef ACCEPT_LOG_LEVEL3
          LEVEL_3_ACCEPTLOG(NULL, mWSAMean);
#endif
/*
          if (bDebug) printf("select() failed with error %d.\n", WSAGetLastError());
#ifdef ACCEPT_LOG_LEVEL3
          sprintf(mSts, "select() failed with error %d.", WSAGetLastError());
          LEVEL_3_ACCEPTLOG(NULL, mSts);
#endif
*/
#ifdef UPDATE_20061011 // ソケット処理のselect()がエラーとして返えった後、以降のソケットのリッスンが出来なくなる可能性の対策。
          for (j = 0; j < (i == 0 ? nIPv4 : (i == 1 ? nIPv6 : nIPv4+nIPv6 ) ); j++) {
            shutdown( *(psLsn+j), 2 );
	        closesocket( *(psLsn+j) );
		  }
          return TRUE;
#else
          continue;
#endif
		}
#ifdef ACCEPT_LOG_LEVEL3
        LEVEL_3_ACCEPTLOG(NULL, "select() success.");
#endif
#ifdef SOCKET_FAIL_TEST // 強制テスト：テスト的にエラーを発生 ソケット無応答状態の動作調査用
	   if (bFT) {
         for (j = 0; j < (i == 0 ? nIPv4 : (i == 1 ? nIPv6 : nIPv4+nIPv6 ) ); j++) {
           if (FD_ISSET(*(psLsn+j), &fds)) {
             shutdown( *(psLsn+j), 2 );
	         closesocket( *(psLsn+j) );
             break;
		   }
		 }
		 bFT = FALSE;
         continue;
	   }
#endif
		for (j = 0; j < (i == 0 ? nIPv4 : (i == 1 ? nIPv6 : nIPv4+nIPv6 ) ); j++) {
          if (FD_ISSET(*(psLsn+j), &fds)) {
            sListener = *(psLsn+j);
            FD_CLR(*(psLsn+j), &fds);
            break;
          }
		}
        /////////////////////////
		nsinListener = (i ? INET6_ADDRSTRLEN : sizeof(SOCKADDR));
		nConnectAF = i;
		if (bDebug) printf("wait accept()\n");
#ifdef ACCEPT_LOG_LEVEL3
        LEVEL_3_ACCEPTLOG(NULL, "wait accept()");
#endif
		if (nConnectAF) {
          s = accept( sListener, (LPSOCKADDR)&sinListener6, &nsinListener);
		  if (sinListener6.sin6_family == AF_INET) { // IPv4からの着信
		    memcpy(&sinListener, &sinListener6, sizeof(SOCKADDR));
		    nConnectAF = 0;
		  }
		} else 
          s = accept( sListener, (LPSOCKADDR)&sinListener, &nsinListener);
#else
	    /////////////////////////
        //fd_set fds;
        FD_ZERO(&fds);
	    for (i = 0; i < nIPv4; i++) {
          if (FD_ISSET(*(psLsn+i), &fds))
	        break;
		}
        for (i = 0; i < nIPv4; i++)
          FD_SET(*(psLsn+i), &fds);
	    if (bDebug) printf("wait select()\n");
        if (select(0, &fds, 0, 0, 0) == SOCKET_ERROR) {
          if (bDebug) printf("select() failed with error %d.\n", WSAGetLastError());
#ifdef UPDATE_20061011 // ソケット処理のselect()がエラーとして返えった後、以降のソケットのリッスンが出来なくなる可能性の対策。
          for (j = 0; j < (i == 0 ? nIPv4 : (i == 1 ? nIPv6 : nIPv4+nIPv6 ) ); j++) {
            shutdown( *(psLsn+j), 2 );
	        closesocket( *(psLsn+j) );
		  }
          return TRUE;
#else
          continue;
#endif
		}
        for (i = 0; i < nIPv4; i++){
          if (FD_ISSET(*(psLsn+i), &fds)) {
            sListener = *(psLsn+i);
            FD_CLR(*(psLsn+i), &fds);
            break;
          }
		}
        /////////////////////////
	    nsinListener = sizeof(SOCKADDR);
		if (bDebug) printf("wait accept()\n");
#ifdef ACCEPT_LOG_LEVEL3
        LEVEL_3_ACCEPTLOG(NULL, "wait accept()");
#endif
        s = accept( sListener, (LPSOCKADDR)&sinListener, &nsinListener);
#endif
		if (bDebug) printf("start accept()\n");
#ifdef ACCEPT_LOG_LEVEL3
        LEVEL_3_ACCEPTLOG(NULL, "start accept()");
#endif
        if ( s == INVALID_SOCKET ) {
		  err = WSAGetLastError();
		  if (bDebug) {
		    switch (err) {
		      case WSANOTINITIALISED: printf("WSANOTINITIALISED\n"); break;
			  case WSAENETDOWN: printf("WSAENETDOWN\n"); break;
			  case WSAEFAULT: printf("WSAEFAULT\n"); break;
			  case WSAEINPROGRESS: printf("WSAEINPROGRESS\n"); break;
			  case WSAEINVAL: printf("WSAEINVAL\n"); break;
			  case WSAEMFILE: printf("WSAEMFILE\n"); break;
			  case WSAENOBUFS: printf("WSAENOBUFS\n"); break;
			  case WSAENOTSOCK: printf("WSAENOTSOCK\n"); break;
			  case WSAEOPNOTSUPP: printf("WSAEOPNOTSUPP\n"); break;
			  case WSAEWOULDBLOCK: printf("WSAEWOULDBLOCK\n"); break;
			  default:  printf("UNKNOWN CODE\n"); break;
			}
		    printf("invalid socket\n");
		  }
          continue;
		}
 		if (bDebug) printf("Accept Client sokect.(%08x)(%08x)\n", s, sListener);
#ifdef ACCEPT_LOG_LEVEL3
		sprintf(mSts, "Accept Client sokect.(%08x)(%08x)", s, sListener);
        LEVEL_3_ACCEPTLOG(NULL, mSts);
#endif
		/////////////////////////////////////
#ifdef LDAP_OPTION_LICENSE // LDAP Optionのライセンスチェックを行なう
		if (bLDAP)
		{
          bLDAPLicense = LDAP_Option_Check();
		}
#endif
		/////////////////////////////////////
#ifdef V3
        if (!GetTimeLimit(&ltbegin, mmode, &bPassport)) {
#ifdef FREE_LICENCE
		  nMAXUser = 1; // １ユーザ有効
#else
  		  sprintf(mess, IMAP4_START_FAIL, "Exceeded an expiration date.");
          //sprintf(mess, IMAP4_START_FAIL, "exceeded the period of the trying-out.");
          send(s, mess, strlen(mess), 0 );
          shutdown( s, 2 );
          closesocket( s );
          return FALSE;
#endif
		}
		bLimit = TRUE;
#else
		bLimit = GetTimeLimit(&ltbegin, mmode, &bPassport);
#endif
        // If the service if terminating, exit this thread.
        if ( bServiceTerminating || !bPassport) {
  		  if (bDebug) printf("Service terminating.(%08x)(%08x)\n", s, sListener);
#ifdef ACCEPT_LOG_LEVEL3
		sprintf(mSts, "Service terminating.(%08x)(%08x)", s, sListener);
        LEVEL_3_ACCEPTLOG(NULL, mSts);
#endif
          shutdown( s, 2 );
          closesocket( s );
          shutdown( sListener, 2 );
          closesocket( sListener );
          return FALSE;
        }
        // クライアントとの実行領域を確保.
        lpClientContext = LocalAlloc( LPTR, sizeof(CLIENT_CONTEXT) );
        if (bDebug) printf("[%08x] memory alloc()\n", lpClientContext);
#ifdef ACCEPT_LOG_LEVEL3
        LEVEL_3_ACCEPTLOG(lpClientContext, "memory alloc()");
#endif
        if ( lpClientContext == NULL ) {
          shutdown( s, 2 );
          closesocket( s );
          shutdown( sListener, 2 );
          closesocket( sListener );
          continue;
        }
        lpClientContext->Socket = s;
#ifdef V4
		if (bHide)
          strcpy(lpClientContext->Context.mMessages, IMAP4_START_MESS_HIDE);
		else
#endif
		sprintf(lpClientContext->Context.mMessages, IMAP4_START_MESS, mmode);
		lpClientContext->Context.mCash[0] = '\x0';
		lpClientContext->Queue[0] = '\x0';
       // lpClientContext->LastClientIo = ClientIoRead;
       // lpClientContext->BytesReadSoFar = 0;
		lpClientContext->bUsed = FALSE;
#ifdef USE_SSL
		lpClientContext->bThreadEnd = TRUE;
#endif
		//////////////////////////////////
		//printf("lpClientContext size=%08x\n",LocalSize(lpClientContext));
        // Initialize the context structure.
		pContext = &lpClientContext->Context;
        pContext->State = Imap4Negotiate;
        pContext->mToken[0] = '\x0';
        pContext->LocalName[0] ='\x0';
#ifdef UPDATE_20140528 // IDLEコマンド実装
	    pContext->bIDLE = FALSE; // IDLE状態フラグ TRUE:IDLE起動中 FALSE:IDLE停止中
        pContext->hIDLEThread = NULL; // IDLEスレッドハンドル初期化
#endif
#ifdef UPDATE_20151115 // SELECT時にメール一覧をキャッシュするためのポインタ
		if (bSELECT)
		{
	      pContext->pFnLists = NULL;
		}
#endif
#ifdef UPDATE_20231211 // 通信エラーが事前の送信で発生してる場合送信を行わないでセッションを終了するようにした。
	    lpClientContext->Context.LastError = 0; 
#endif

#ifdef UPDATE_20110414 // NOOPタイマー（一定時間新着がないNOOPでセッション強制切断する対策（負荷軽減対策）
        GetLocalTime(&pContext->nNOOPst);
#endif
		lpClientContext->nsin1 = sizeof(lpClientContext->sin1);
#ifdef IPv6
		//if (nAddressFamily) {
        if (nConnectAF) {
		  lpClientContext->nsin1 = sizeof(lpClientContext->sin61);
		  err = getsockname(lpClientContext->Socket, (LPSOCKADDR)&lpClientContext->sin61, &lpClientContext->nsin1);
		} else {
		  lpClientContext->nsin1 = sizeof(lpClientContext->sin1);
		  err = getsockname(lpClientContext->Socket, (LPSOCKADDR)&lpClientContext->sin1, &lpClientContext->nsin1);
		}
#else
		lpClientContext->nsin1 = sizeof(SOCKADDR);
		err = getsockname(lpClientContext->Socket, (LPSOCKADDR)&lpClientContext->sin1, &lpClientContext->nsin1);
		//getsockname(lpClientContext->Socket, (LPSOCKADDR)&lpClientContext->sin1, &lpClientContext->nsin1);
#endif
        if ( err == SOCKET_ERROR ) {
		  if (bDebug) printf("getsockname error\n");
          shutdown( lpClientContext->Socket, 2 );
          closesocket(lpClientContext->Socket);
		  lpClientContext->Socket = 0;
#ifdef UPDATE_20060515 // SOCKET関連メモリ開放抜け
#ifdef IPv6
#ifdef VC6
	      freehostent(lpClientContext->phsin1);
#else
          freeaddrinfo(lpClientContext->AI);
#endif
		  lpClientContext->phsin1 = NULL;
#endif
#endif
		  //LocalFree(lpClientContext->hCompletionPort);
		  if (LocalFlags(lpClientContext) != LMEM_INVALID_HANDLE) // 解放可能かどうか確認
		    LocalFree(lpClientContext);
          continue;
        }
	    strcpy(pContext->LocalName, mdomain);
#ifdef IPv6
        Error = 0;
		//if (nAddressFamily) {
        if (nConnectAF) {
#ifdef VC6
          lpClientContext->phsin1 = getipnodebyaddr((const char FAR *)&lpClientContext->sin61.sin6_addr,
 			                                        sizeof(lpClientContext->sin61.sin6_addr),
												    AF_INET6,
												    &Error);
#else
          lpClientContext->phsin1 = NULL;
           memset(&Hints, 0, sizeof(Hints));
           Hints.ai_flags = AI_CANONNAME;
           Hints.ai_family = AF_INET6;
           Hints.ai_socktype = SOCK_STREAM;
           if (getaddrinfo(&lpClientContext->sin61.sin6_addr, NULL, &Hints, &AddrInfo) == 0) {
			 lpClientContext->AI = AddrInfo;
             hst.h_name = AddrInfo->ai_canonname;
			 lpClientContext->phsin1 = &hst;
           }
#endif
#ifdef UPDATE_20110525 // IPv6でのIP単位での接続設定のとき内部ドメイン宛の接続を拒否してしまう不具合
          (const char *)pnet = inet_ntop(AF_INET6, &lpClientContext->sin61.sin6_addr, Out6, INET6_ADDRSTRLEN);
#else
          (const char *)pnet = inet_ntop(AF_INET6, &sinListener6.sin6_addr, Out6, INET6_ADDRSTRLEN);
#endif
#ifdef UPDATE_20201117 // IPv6で接続時の受信ポートが正しく取得できていない不具合
          pContext->nConnectPort = ntohs(lpClientContext->sin61.sin6_port); // 2020.11.17 update
#else
          pContext->nConnectPort = ntohs(lpClientContext->sin1.sin_port);
#endif
		} else {
#ifdef VC6
          lpClientContext->phsin1 = getipnodebyaddr((const char FAR *)&lpClientContext->sin1.sin_addr,
 			                                        sizeof(lpClientContext->sin1.sin_addr),
												    AF_INET,
												    &Error);
#else
          lpClientContext->phsin1 = NULL;
          memset(&Hints, 0, sizeof(Hints));
          Hints.ai_flags = AI_CANONNAME;
          Hints.ai_family = AF_INET;
          Hints.ai_socktype = SOCK_STREAM;
          if (getaddrinfo(&lpClientContext->sin61.sin6_addr, NULL, &Hints, &AddrInfo) == 0) {
		    lpClientContext->AI = AddrInfo;
            hst.h_name = AddrInfo->ai_canonname;
		    lpClientContext->phsin1 = &hst;
          }
#endif
          (const char *)pnet = inet_ntop(AF_INET, &lpClientContext->sin1.sin_addr, Out6, INET_ADDRSTRLEN);
          pContext->nConnectPort = ntohs(lpClientContext->sin1.sin_port);
		}
#else
		lpClientContext->phsin1 = gethostbyaddr((const char FAR *)&lpClientContext->sin1.sin_addr, sizeof(lpClientContext->sin1.sin_addr), PF_INET);
		(const char *)pnet = inet_ntoa(lpClientContext->sin1.sin_addr);
        pContext->nConnectPort = ntohs(lpClientContext->sin1.sin_port);
#endif
		strcpy(pContext->MyAddr, "");
		if (pnet) {
          strcpy(pContext->MyAddr, pnet);
		}
#ifdef UPDATE_20060315 // 隠すIPアドレス
#ifdef UPDATE_20151122 // 特定接続元IPからの接続拒絶用テーブル追加(rejectimap4ip.dat)隠しオプション
		if (HiddenMyIP(pContext->MyAddr, 0))
#else
		if (HiddenMyIP(pContext->MyAddr))
#endif
		{
          shutdown( lpClientContext->Socket, 2 );
          closesocket(lpClientContext->Socket);
          lpClientContext->Socket = 0;
#ifdef UPDATE_20060515 // SOCKET関連メモリ開放抜け
#ifdef IPv6
#ifdef VC6
	      freehostent(lpClientContext->phsin1);
#else
          freeaddrinfo(lpClientContext->AI);
#endif
		  lpClientContext->phsin1 = NULL;
#endif
#endif
		  //LocalFree(lpClientContext->hCompletionPort);
		  if (LocalFlags(lpClientContext) != LMEM_INVALID_HANDLE) // 解放可能かどうか確認
		    LocalFree(lpClientContext);
		  continue;
		}
#endif
		if (lpClientContext->phsin1) {
		  if (strstr((lpClientContext->phsin1)->h_name,".")) {
		    strcpy(pContext->LocalName, (lpClientContext->phsin1)->h_name);
		  }
#ifdef IPv6
#ifdef UPDATE_20060515 // SOCKET関連メモリ開放抜け
#ifdef VC6
		  freehostent(lpClientContext->phsin1);
#else
          freeaddrinfo(lpClientContext->AI);
#ifdef UPDATE_20140723 // 64bit版での送信セッション中断でハングする
		  lpClientContext->AI = NULL;
#endif
#endif
#else
          //if (nAddressFamily) 
          if (nConnectAF)
		    if (LocalFlags(lpClientContext->phsin1) != LMEM_INVALID_HANDLE) // 解放可能かどうか確認
  	          LocalFree(lpClientContext->phsin1);
#endif
#endif
		  lpClientContext->phsin1 = NULL;
		}
#ifdef IPv6
		pnet = NULL;
		//if (nAddressFamily) {
        if (nConnectAF) {
          (const char *)pnet = inet_ntop(AF_INET6, &sinListener6.sin6_addr, Out6, INET6_ADDRSTRLEN);
		} else {
          (const char *)pnet = inet_ntop(AF_INET, &sinListener.sin_addr, Out6, INET_ADDRSTRLEN);
		}
#else
		pnet = inet_ntoa(sinListener.sin_addr);
#endif
		if (!pnet) {
#ifdef IPv6
		//if (nAddressFamily) {
        if (nConnectAF) {
		  if (bDebug) printf("inet_ntop(sinListener6.sin6_addr) = NULL\n");
		} else {
		  if (bDebug) printf("inet_ntop(sinListener.sin_addr) = NULL\n");
		}
#else
  		  if (bDebug) printf("inet_ntoa(sinListener.sin_addr) = NULL\n");
#endif
          shutdown( lpClientContext->Socket, 2 );
          closesocket(lpClientContext->Socket);
          lpClientContext->Socket = 0;
#ifdef UPDATE_20060515 // SOCKET関連メモリ開放抜け
#ifdef IPv6
#ifdef VC6
	      freehostent(lpClientContext->phsin1);
#else
          freeaddrinfo(lpClientContext->AI);

#endif
		  lpClientContext->phsin1 = NULL;
#endif
#endif
		  //LocalFree(lpClientContext->hCompletionPort);
		  if (LocalFlags(lpClientContext) != LMEM_INVALID_HANDLE) // 解放可能かどうか確認
		    LocalFree(lpClientContext);
		  continue;
		}
	    strcpy(pContext->PeerAddr, pnet);
/////////////////////////////////////
#ifdef UPDATE_20151122 // 特定接続元IPからの接続拒絶用テーブル追加(rejectimap4ip.dat)隠しオプション
		if (HiddenMyIP(pContext->PeerAddr, 1))
		{
          sprintf(mec, "reject address = [%s]", pContext->PeerAddr);
 		  if (bDebug) printf("[%08x] %s\n", lpClientContext, mec);
#ifdef ACCEPT_LOG_LEVEL3
		  LEVEL_3_ACCEPTLOG(lpClientContext, mec);
#endif
          shutdown( lpClientContext->Socket, 2 );
          closesocket(lpClientContext->Socket);
          lpClientContext->Socket = 0;
#ifdef UPDATE_20060515 // SOCKET関連メモリ開放抜け
#ifdef IPv6
#ifdef VC6
	      freehostent(lpClientContext->phsin1);
#else
          freeaddrinfo(lpClientContext->AI);
#endif
		  lpClientContext->phsin1 = NULL;
#endif
#endif
		  //LocalFree(lpClientContext->hCompletionPort);
          if (LocalFlags(lpClientContext) != LMEM_INVALID_HANDLE) // 解放可能かどうか確認
		    LocalFree(lpClientContext);
		  continue;
		}
#endif
#ifdef UPDATE_20151126 // リッスンＩＰ毎に証明書を選択できる対策
#ifdef UPDATE_20151127 // リッスンＩＰ毎に証明書を選択できる対策(sslbyip.datファイルで指定)
        pContext->bUsedSTLS = GetSSLbyIP(pContext->MyAddr, 2 /*imap*/, pContext->mCertificate, pContext->mPrivatekey);
#else 
		pContext->bUsedSTLS = FALSE;
        pContext->mCertificate[0] = '\x0';
		pContext->mPrivatekey[0] = '\x0';
		//if (bUsedSSL)
		{
          sprintf(mSSLKeyName, "UsedSTARTTLS[%s]", pContext->MyAddr);
		  if (GetProfileIntEx(SYSTEM_REG, mSSLKeyName, (int) TRUE))
		  { 
            sprintf(mSSLKeyName, "Certificate[%s]", pContext->MyAddr);
			mSSLKey[0] = '\x0';
            GetProfileStringEx(SYSTEM_REG, mSSLKeyName, "", mSSLKey, sizeof(mSSLKey)); // SSL/TSL 証明書
	        strcpy(pContext->mCertificate, mSSLKey);
            sprintf(mSSLKeyName, "Private-Key[%s]", pContext->MyAddr);
			mSSLKey[0] = '\x0';
            GetProfileStringEx(SYSTEM_REG, mSSLKeyName, "", mSSLKey, sizeof(mSSLKey)); // SSL/TSL 個人鍵
	        strcpy(pContext->mPrivatekey, mSSLKey);
		    pContext->bUsedSTLS = ((pContext->mCertificate[0] && pContext->mPrivatekey[0]) ? TRUE : FALSE);
		  }
		}
#endif
#endif
/////////////////////////////////////
 	    strcpy(pContext->PeerName, pContext->PeerAddr);
		if (nSendTMO) { // 0 ならタイムアウトしない。
          timeout = 1000 * (int)nSendTMO; // 1秒単位
          setsockopt(lpClientContext->Socket, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout));
		}
		if (nTMOut) { // 0 ならタイムアウトしない。
          timeout = 1000 * (int)nTMOut; // 1秒単位
          setsockopt(lpClientContext->Socket, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout));
		}
	    nl = sizeof(int);
 	    nSndsize = nSendBuf;
		if (nSndsize > 0) {
          err = setsockopt(lpClientContext->Socket, SOL_SOCKET, SO_SNDBUF, (char *)&nSndsize, nl);
          if ( err == SOCKET_ERROR ) {
		    if (bDebug) printf("setsockopt error(SO_SNDBUF)\n");
            shutdown( lpClientContext->Socket, 2 );
            closesocket(lpClientContext->Socket);
		    lpClientContext->Socket = 0;
#ifdef UPDATE_20060515 // SOCKET関連メモリ開放抜け
#ifdef IPv6
#ifdef VC6
	        freehostent(lpClientContext->phsin1);
#else
			freeaddrinfo(lpClientContext->AI);
#endif
		    lpClientContext->phsin1 = NULL;
#endif
#endif
		    //LocalFree(lpClientContext->hCompletionPort);
		    if (LocalFlags(lpClientContext) != LMEM_INVALID_HANDLE) // 解放可能かどうか確認
		      LocalFree(lpClientContext);
		    continue;
		  }
          getsockopt( lpClientContext->Socket, SOL_SOCKET, SO_SNDBUF, (char *)&nSndsize, &nl);
          if (bDebug)
            printf("Set Send Buffer size = %d\n", nSndsize);
		}
		/////////////////////////////////////
		//// IMAP4制御をスレッドへ渡す
 		if (bDebug) printf("[%08x] SOCKET[%08x] Create thread.\n", lpClientContext, s);
#ifdef UPDATE_20151028 // セッションスレッドの作成に失敗した時の対策
		if (_beginthread( IMAP4Main, sizeof(SOCKET), lpClientContext) == -1L)
		{ // 失敗
	      switch (errno)
		  {
		    case EAGAIN: sprintf(mec, "Start Thread() ERROR EAGAIN=%d", errno); break;
		    case EINVAL: sprintf(mec, "Start Thread() ERROR EINVAL=%d", errno); break;
            case ENOMEM: sprintf(mec, "Start Thread() ERROR ENOMEM=%d", errno); break;
		    default: sprintf(mec, "Start Thread() ERROR CODE=%d", errno); break;
		  }
 		  if (bDebug) printf("[%08x] %s.\n", lpClientContext, mec);
#ifdef ACCEPT_LOG_LEVEL3
		  LEVEL_3_ACCEPTLOG(lpClientContext, mec);
#endif
          shutdown( lpClientContext->Socket, 2 );
          closesocket(lpClientContext->Socket);
          lpClientContext->Socket = 0;
#ifdef UPDATE_20060515 // SOCKET関連メモリ開放抜け
#ifdef IPv6
#ifdef VC6
	      freehostent(lpClientContext->phsin1);
#else
          freeaddrinfo(lpClientContext->AI);
#endif
		  lpClientContext->phsin1 = NULL;
#endif
#endif
		  //LocalFree(lpClientContext->hCompletionPort);
		  if (LocalFlags(lpClientContext) != LMEM_INVALID_HANDLE) // 解放可能かどうか確認
		    LocalFree(lpClientContext);
		}
#else
		_beginthread( IMAP4Main, sizeof(SOCKET), lpClientContext);
#endif
		/////////////////////////////////////
	}
}

BOOL IMAP4Main(PCLIENT_CONTEXT lpClientContext) { //SOCKET *sk) { //struct _IMAP4S *mImap4s) {
#ifdef USE_SSL
	char    mCheck[INET6_ADDRSTRLEN+20];
    SSL_CTX *ctx;
    //SSL     *ssl;
    int     ret;
    unsigned long err_ssl;
#endif
	//SOCKET  s = *sk;
#ifdef IPv6
    char *Address = NULL;
	//SOCKADDR_IN  sinListener;
	//SOCKADDR_IN6 sinListener6;
	//char Out6[INET6_ADDRSTRLEN];
    //int Error;
#else
    SOCKADDR_IN sin, sinListener;
	int i;
#endif
    int  err; //, nsinListener;
    PImap4Context pContext = &lpClientContext->Context;
    //PCLIENT_CONTEXT lpClientContext;
	//char  *pnet;
#ifdef Y2038_BUG
	//SYSTEMTIME ltime;
    //FILETIME ft;
#else
    time_t ltime;
#endif
	char  mec[128];
    //char  mtime[256]; 
    //int   timeout;
	BOOL  sts;
#ifdef UPDATE_20190216 // IP毎の同時接続ロックアウト機能を追加
	char  mIPdir[256];
#endif
#ifdef UPDATE_20240420 // 暗号化通信方式のセキュリティレベルの設定
	char  mLog[256];
#endif
		////////////////////////////////////////////////
#ifdef SEND_SELCET
        FD_ZERO(&fdw);
        FD_SET(lpClientContext->Socket, &fdw);
#ifdef ACCEPT_LOG_LEVEL3
        LEVEL_3_ACCEPTLOG(lpClientContext, "wait select()");
#endif
        select(0, 0, &fdw, 0, 0);// 使用可能になるまで待つ
        FD_CLR(lpClientContext->Socket, &fdw); // 解除
#endif
		////////////////////////////////////////////////
#ifdef USE_SSL
		sprintf(mCheck, "%s %d*", pContext->MyAddr, pContext->nConnectPort);
        pContext->bUsedSSL = FALSE;
		if (strstr(mListenIP2 ,mCheck))
          pContext->bUsedSSL = TRUE;
#ifdef CLUSTERING
		else {
		  sprintf(mCheck, "*.*.*.* %d*", pContext->nConnectPort);
		  if (strstr(mListenIP2 ,mCheck))
            pContext->bUsedSSL = TRUE;
		}
#endif
		if ( bListenMode && pContext->bUsedSSL ||
			!bListenMode && bUsedSSL)  // SSLを使う場合は証明書と個人鍵を設定する。
		{
#ifndef UPDATE_20060512 // SSLの初期化はサービス開始で１回のみ
#ifdef UPDATE_20060322 // SSLの初期化に失敗すると以降でハングする
		  SSL_library_init();
#endif
          SSLeay_add_ssl_algorithms();
          SSL_load_error_strings();
#endif
#ifdef UPDATE_OPESSL1_1_1 // openssl1.1.1系に置換える場合
          ctx = SSL_CTX_new(TLS_server_method()); // SSL2を廃止、TLS1.3を追加
#else
          ctx = SSL_CTX_new(SSLv23_server_method()); // SSL2 or SSL3を使用
#endif
#ifdef ACCEPT_LOG_LEVEL3
          sprintf(mec, "SSL_CTX_new() = %d", ctx);
          LEVEL_3_ACCEPTLOG(lpClientContext, mec);
#endif
#ifdef UPDATE_20060329 // SSLの初期化に失敗すると以降でハングする
          if (ctx == NULL)  {
#ifndef UPDATE_20060512 // SSLの初期化はサービス開始で１回のみ
#ifndef UPDATE_OPESSL1_1_1 // openssl1.1.1系に置換える場合
            ERR_free_strings();
#endif
#endif
            shutdown( lpClientContext->Socket, 2 );
            closesocket( lpClientContext->Socket );
#ifdef ACCEPT_LOG_LEVEL3
            LEVEL_3_ACCEPTLOG(lpClientContext, "SSL_CTX_new() failed");
#endif
            return FALSE;
		  }
#endif
#ifdef UPDATE_20240420 // 暗号化通信方式のセキュリティレベルの設定
          //if (bDebug) printf("SSL security level =%d\n", SSL_CTX_get_security_level(ctx));
          SSL_CTX_set_security_level(ctx, nSSLSecureLevel);
#ifdef ACCEPT_LOG_LEVEL3
          sprintf(mLog, "SSL_CTX_get_security_level() = %d", SSL_CTX_get_security_level(ctx));
          LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
#endif
#endif
#ifdef UPDATE_20171211A // 使用可能な暗号化通信方式のバージョンを規定するオプション 0x1:SSL2禁止 0x2:SSL3禁止 0x4:TSL1禁止 0x8:TSL1.1禁止 0x10:TSL1.2禁止
          SSL_CTX_set_options(ctx, nSecuerLayOption);
#endif
#ifdef UPDATE_20190426 // 利用する暗号アルゴリズムの組み合わせオプション 
          if (mSecuerLayCipher[0])
	 	    SSL_CTX_set_cipher_list(ctx, mSecuerLayCipher);
		  if (bDebug)
		    Print_SSL_CIPHER_names(ctx);
#endif
          lpClientContext->ctx = ctx;
#ifdef UPDATE_20170619 // 中間証明書の繋がりが反映されていなかった
	      if (pContext->bUsedSTLS)
		  {
            SSL_CTX_use_certificate_chain_file(lpClientContext->ctx, pContext->mCertificate); // // SSL用中間証明書
		  } else {
	        SSL_CTX_use_certificate_chain_file(lpClientContext->ctx, mCertificate); // // SSL用中間証明書
		  }
#endif
          lpClientContext->ssl = SSL_new(ctx); // SSL構造作成
#ifdef UPDATE_20060322 // SSLの初期化に失敗すると以降でハングする
          if (lpClientContext->ssl == NULL)  {
		    SSL_CTX_free(lpClientContext->ctx);
#ifndef UPDATE_20060512 // SSLの初期化はサービス開始で１回のみ
#ifndef UPDATE_OPESSL1_1_1 // openssl1.1.1系に置換える場合
            ERR_free_strings();
#endif
#endif
            shutdown( lpClientContext->Socket, 2 );
            closesocket( lpClientContext->Socket );
#ifdef UPDATE_20060515 // SOCKET関連メモリ開放抜け
			lpClientContext->Socket = 0;
#ifdef IPv6
#ifdef VC6
	        freehostent(lpClientContext->phsin1);
#else
			freeaddrinfo(lpClientContext->AI);
#endif
		    lpClientContext->phsin1 = NULL;
#endif
		    if (LocalFlags(lpClientContext) != LMEM_INVALID_HANDLE) // 解放可能かどうか確認
		      LocalFree(lpClientContext);
#endif
            return FALSE;
		  }
#endif
          SSL_set_fd(lpClientContext->ssl, lpClientContext->Socket); 
//		  if (lpClientContext->ssl->rbio)
//		    lpClientContext->ssl->rbio->bOnMemory = 0; // デフォルト FALSE
#ifdef UPDATE_20151126 // リッスンＩＰ毎に証明書を選択できる対策
          if (pContext->bUsedSTLS)
		  {
            SSL_use_RSAPrivateKey_file(lpClientContext->ssl, pContext->mPrivatekey, SSL_FILETYPE_PEM); // 個人鍵を指定
            SSL_use_certificate_file(lpClientContext->ssl, pContext->mCertificate, SSL_FILETYPE_PEM);   // 証明書を指定
#ifndef UPDATE_20170619 // 中間証明書の繋がりが反映されていなかった
	        SSL_CTX_use_certificate_chain_file(lpClientContext->ctx, pContext->mCertificate); // // SSL用中間証明書
#endif
		  } else
#endif
		  {
            SSL_use_RSAPrivateKey_file(lpClientContext->ssl, mPrivatekey, SSL_FILETYPE_PEM); // 個人鍵を指定
            SSL_use_certificate_file(lpClientContext->ssl, mCertificate, SSL_FILETYPE_PEM);   // 証明書を指定
#ifndef UPDATE_20170619 // 中間証明書の繋がりが反映されていなかった
#ifdef UPDATE_20090108 // SSL用中間証明書
		    SSL_CTX_use_certificate_chain_file(lpClientContext->ctx, mCertificate);
#endif
#endif
		  }
          ret = SSL_accept(lpClientContext->ssl);
#ifdef UPDATE_20060605 // SSL_acceptの応答コードが０の時に正常としていた不具合
          if (ret <= 0)
#else
          if (ret < 0)
#endif
		  {
            err_ssl = ERR_get_error();
		    SSL_CTX_free(lpClientContext->ctx);
#ifdef UPDATE_20090113 // SSLでのメモリ開放
            SSL_shutdown(lpClientContext->ssl);
#endif
            SSL_free(lpClientContext->ssl);
#ifndef UPDATE_20060512 // SSLの初期化はサービス開始で１回のみ
#ifndef UPDATE_OPESSL1_1_1 // openssl1.1.1系に置換える場合
            ERR_free_strings();
#endif
#endif
            shutdown( lpClientContext->Socket, 2 );
            closesocket( lpClientContext->Socket );
#ifdef UPDATE_20060515 // SOCKET関連メモリ開放抜け
			lpClientContext->Socket = 0;
#ifdef IPv6
#ifdef VC6
	        freehostent(lpClientContext->phsin1);
#else
			freeaddrinfo(lpClientContext->AI);
#endif
		    lpClientContext->phsin1 = NULL;
#endif
		    if (LocalFlags(lpClientContext) != LMEM_INVALID_HANDLE) // 解放可能かどうか確認
		      LocalFree(lpClientContext);
#endif
            return FALSE;
		  }
		}
#endif

////// 特定IPの応答禁止対策 ///////
///////////////////////////////////

/*
        // Send the welcome banner.
        gettime(&ltime, mtime);
#ifdef Y2038_BUG
	    SystemTimeToFileTime(&ltime, &ft);
        sprintf(pContext->mAPOPCODE, "<%06d.%d@%s>", rand(), ft.dwLowDateTime, pContext->LocalName);
#else
        sprintf(pContext->mAPOPCODE, "<%06d.%d@%s>", rand(), ltime, pContext->LocalName);
#endif
*/
		//ltoa(ltime, mMD5, 10);
		//mMD5[0] = '\x0';
        //MDString (mtime, pContext->mAPOPCODE); 
        //MDString (mMD5, pContext->mAPOPCODE); 
        //sprintf(pContext->mMessages, IMAP4_START_MESS, mmode); // pContext->mAPOPCODE); //, pContext->LocalName);
#ifdef TMQ_ON
#ifdef UPDATE_20190216 // IP毎の同時接続ロックアウト機能を追加
        if(nTMOut && nAcceptLimit || nIPLockOut) // ロックアウトまでの回数 デフォルト 0:無効
#else
		if (nTMOut && nAcceptLimit) 
#endif
		{
#ifdef UPDATE_20190216 // IP毎の同時接続ロックアウト機能を追加
		  if (nIPLockOut) {
			sprintf(mIPdir, "%siplock\\", mMailSpoolDir);
            AuthLockOut(pContext->PeerAddr, mIPdir, TRUE);
		  }
          if (nAcceptLimit > 0 && nAcceptCount >= nAcceptLimit || nIPLockOut && HiddenMyIP(pContext->PeerAddr, 2)) 
#else
		  if (nAcceptCount >= nAcceptLimit)
#endif
		  {
            if (bDebug) printf("exceeds an accepting limit. count=(%lu)\n", nAcceptLimit);
            sprintf(pContext->mMessages, IMAP4_START_FAIL, "exceeds an accepting limit.");
#ifdef USE_SSL
#ifdef USE_STARTTLS
		if ((lpClientContext->Context.bUsedSSL || 
             !bListenMode && bUsedSSL) &&
		    lpClientContext->ssl) 
#else
		if (( bListenMode && lpClientContext->Context.bUsedSSL || 
             !bListenMode && bUsedSSL) &&
		    lpClientContext->ssl) 
#endif
		{
	  	  SSL_write(lpClientContext->ssl, pContext->mMessages, strlen(pContext->mMessages));
	      SSL_CTX_free(lpClientContext->ctx);
#ifdef UPDATE_20090113 // SSLでのメモリ開放
          SSL_shutdown(lpClientContext->ssl);
#endif
          SSL_free(lpClientContext->ssl);
#ifndef UPDATE_20060512 // SSLの初期化はサービス開始で１回のみ
#ifndef UPDATE_OPESSL1_1_1 // openssl1.1.1系に置換える場合
          ERR_free_strings();
#endif
#endif
		} else
#endif
            send(lpClientContext->Socket, pContext->mMessages, strlen(pContext->mMessages), 0 );
            shutdown( lpClientContext->Socket, 2 );
            closesocket(lpClientContext->Socket);
		    lpClientContext->Socket = 0;
#ifdef UPDATE_20060515 // SOCKET関連メモリ開放抜け
#ifdef IPv6
#ifdef VC6
	        freehostent(lpClientContext->phsin1);
#else
			freeaddrinfo(lpClientContext->AI);
#endif
		    lpClientContext->phsin1 = NULL;
#endif
#endif
		    //LocalFree(lpClientContext->hCompletionPort);
		    if (LocalFlags(lpClientContext) != LMEM_INVALID_HANDLE) // 解放可能かどうか確認
		      LocalFree(lpClientContext);
            return FALSE;
		  }
		}
#endif
		////////////////////////////////////////////////
#ifdef USE_SSL
#ifdef USE_STARTTLS
		if ((lpClientContext->Context.bUsedSSL || 
             !bListenMode && bUsedSSL) &&
		    lpClientContext->ssl)
#else
		if (( bListenMode && lpClientContext->Context.bUsedSSL || 
             !bListenMode && bUsedSSL) &&
		    lpClientContext->ssl)
#endif
		{
	  	  err = SSL_write(lpClientContext->ssl, pContext->mMessages, strlen(pContext->mMessages));
//		  lpClientContext->ssl->rbio->bOnMemory  = 1; // TRUE;
		} else
#endif
        err = send(lpClientContext->Socket, pContext->mMessages, strlen(pContext->mMessages), 0 );
#ifdef ACCEPT_LOG_LEVEL3
        LEVEL_3_ACCEPTLOG(lpClientContext, pContext->mMessages);
#endif
        if ( err == SOCKET_ERROR ) {
#ifdef ACCEPT_LOG_LEVEL3
          //LEVEL_3_ACCEPTLOG(lpClientContext, "send() SOCKET_ERROR");
		  err = WSAGetLastError();
		  switch (err) {
		    case WSANOTINITIALISED: sprintf(mec, "send() SOCKET_ERROR WSANOTINITIALISED=%d", err); break;
			case WSAENETDOWN: sprintf(mec, "send() SOCKET_ERROR WSAENETDOWN=%d", err); break;
			case WSAEFAULT: sprintf(mec, "send() SOCKET_ERROR WSAEFAULT=%d", err); break;
			case WSAEINPROGRESS: sprintf(mec, "send() SOCKET_ERROR WSAEINPROGRESS=%d", err); break;
			case WSAEINVAL: sprintf(mec, "send() SOCKET_ERROR WSAEINVAL=%d", err); break;
			case WSAEMFILE: sprintf(mec, "send() SOCKET_ERROR WSAEMFILE=%d", err); break;
			case WSAENOBUFS: sprintf(mec, "send() SOCKET_ERROR WSAENOBUFS=%d", err); break;
			case WSAENOTSOCK: sprintf(mec, "send() SOCKET_ERROR WSAENOTSOCK=%d", err); break;
			case WSAEOPNOTSUPP: sprintf(mec, "send() SOCKET_ERROR WSAEOPNOTSUPP=%d", err); break;
			case WSAEWOULDBLOCK: sprintf(mec, "send() SOCKET_ERROR WSAEWOULDBLOCK=%d", err); break;
			default:  sprintf(mec, "send() SOCKET_ERROR OTHER CODE=%d", err); break;
		  }
		  LEVEL_3_ACCEPTLOG(lpClientContext, mec);
#endif
#ifdef USE_SSL
#ifdef USE_STARTTLS
		  if ((lpClientContext->Context.bUsedSSL || 
			   !bListenMode && bUsedSSL) &&
			   lpClientContext->ssl)
#else
		  if (( bListenMode && lpClientContext->Context.bUsedSSL || 
			   !bListenMode && bUsedSSL) &&
			   lpClientContext->ssl)
#endif
		  {
		    SSL_CTX_free(lpClientContext->ctx);
#ifdef UPDATE_20090113 // SSLでのメモリ開放
            SSL_shutdown(lpClientContext->ssl);
#endif
            SSL_free(lpClientContext->ssl);
#ifndef UPDATE_20060512 // SSLの初期化はサービス開始で１回のみ
#ifndef UPDATE_OPESSL1_1_1 // openssl1.1.1系に置換える場合
            ERR_free_strings();
#endif
#endif
		  }
#endif
          shutdown( lpClientContext->Socket, 2 );
		  closesocket(lpClientContext->Socket);
		  lpClientContext->Socket = 0;
#ifdef UPDATE_20060515 // SOCKET関連メモリ開放抜け
#ifdef IPv6
#ifdef VC6
	      freehostent(lpClientContext->phsin1);
#else
		  freeaddrinfo(lpClientContext->AI);
#endif
		  lpClientContext->phsin1 = NULL;
#endif
#endif

		  if (LocalFlags(lpClientContext) != LMEM_INVALID_HANDLE) // 解放可能かどうか確認
		    LocalFree(lpClientContext);
          return FALSE;
        }
		/*
#ifdef UPDATE_20141220 // 操作フォルダをアクセス中のセッションに変化をリアルタイムに同報するための対策
		if (bBroadcastSession) // 同一フォルダの操作情報を接続中のセッションに同報するオプション TRUE:有効 FALSE:無効
		{
          JoinBroadcastSession(lpClientContext);
		}
#endif
		*/
#ifdef UPDATE_20141221 // セッション内でのメッセージ送信の排他処理
        lpClientContext->bPutLock = FALSE;
#endif
        /////////////////////////////////////////
        //lpClientContext->LastClientIo = ClientIoRead;
        //lpClientContext->BytesReadSoFar = 0;
		lpClientContext->bUsed = FALSE;
		nAcceptCount++;
        strcpy(pContext->mSharedRoot, mSharedRoot);
        pContext->bSharedRW = bSharedRW;
#ifdef TMQ_ON
#ifndef BTHREAD
		if (nTMOut)
          AddTMQueue(lpClientContext, FALSE);
#endif
#endif
	    if (bDebug) printf("[%08x] START[%08x]\n", lpClientContext, lpClientContext->Socket);

		if (lpClientContext->Socket) {
		  while (get_reply(lpClientContext, TRUE, "DISPATCH")) {
	        //if (bDebug) printf("[%08x] get_reply()\n", lpClientContext);
#ifdef UPDATE_20141221 // セッション内でのメッセージ送信の排他処理
		    if (bBroadcastSession) // 同一フォルダの操作情報を接続中のセッションに同報するオプション TRUE:有効 FALSE:無効
			{
              put_reply_check(&lpClientContext->bPutLock);
			}
#endif
	        //if (bDebug) printf("[%08x] Imap4_Dispatch() Start\n", lpClientContext);
            sts = Imap4_Dispatch(lpClientContext); // 受信コマンド解析＆応答作成
	        //if (bDebug) printf("[%08x] Imap4_Dispatch() End\n", lpClientContext);
#ifdef UPDATE_20141221 // セッション内でのメッセージ送信の排他処理
		    if (bBroadcastSession) // 同一フォルダの操作情報を接続中のセッションに同報するオプション TRUE:有効 FALSE:無効
			{
              lpClientContext->bPutLock = FALSE;
			}
#endif
	        //if (bDebug) printf("[%08x] Imap4_Dispatch() End2\n", lpClientContext);
			if (!sts) {// QUITなら終了
			  break;
			}
		  }
          CloseClient( lpClientContext, FALSE, FALSE);
		} 
   return TRUE;
} 

VOID CloseClient (PCLIENT_CONTEXT lpClientContext, BOOL bGraceful, BOOL bTMQ) {
    LINGER lingerStruct;
    PImap4Context pContext = &lpClientContext->Context;
    //HLOCAL hClient;
	int    nsts;
	///////////////////////////////////
    if (bDebug) printf("[%08x] CloseClient Started\n", lpClientContext);
#ifdef ACCEPT_LOG_LEVEL3
	LEVEL_3_ACCEPTLOG(lpClientContext, "CloseClient Started.:SOCKET.C");
#endif
	if (!lpClientContext && !LocalHandle(lpClientContext)) {// Null ポインタなら処理しない。
	  if (bDebug) printf("CloseClient lpClientContext = Null!!\n");
	  return;
	}
#ifdef UPDATE_20141220 // 操作フォルダをアクセス中のセッションに変化をリアルタイムに同報するための対策
	if (bBroadcastSession) // 同一フォルダの操作情報を接続中のセッションに同報するオプション TRUE:有効 FALSE:無効
	{
      LeaveBroadcastSession(lpClientContext);
	}
#endif

#ifdef TMQ_ON
#ifndef BTHREAD
printf("[%08x]CloseClient  DelTMQueue()\n", lpClientContext);
    if (nTMOut)
      DelTMQueue(lpClientContext, bTMQ); //TRUE); //
#endif
#endif
	///////////////////////////////////
    // If we're supposed to abort the connection, set the linger value
    // on the socket to 0.
    {
#ifdef USE_SSL
#ifdef USE_STARTTLS
 	  if ((lpClientContext->Context.bUsedSSL || 
           !bListenMode && bUsedSSL) &&
		   lpClientContext->ssl) 
#else
 	  if (( bListenMode && lpClientContext->Context.bUsedSSL || 
           !bListenMode && bUsedSSL) &&
		   lpClientContext->ssl) 
#endif
	  {
	    SSL_CTX_free(lpClientContext->ctx);
#ifdef UPDATE_20090113 // SSLでのメモリ開放
        SSL_shutdown(lpClientContext->ssl);
#endif
        SSL_free(lpClientContext->ssl);
#ifndef UPDATE_20060512 // SSLの初期化はサービス開始で１回のみ
#ifndef UPDATE_OPESSL1_1_1 // openssl1.1.1系に置換える場合
        ERR_free_strings();
#endif
#endif
	  }
#endif
	  if (lpClientContext->Socket) {
        if (bDebug) printf("[%08x] lpClientContext->Socket = %08x\n", lpClientContext, lpClientContext->Socket);
        if ( !bGraceful ) {
          lingerStruct.l_onoff = 1;
          lingerStruct.l_linger = 0;
          setsockopt( lpClientContext->Socket, SOL_SOCKET, SO_LINGER,
                      (char *)&lingerStruct, sizeof(lingerStruct) );
		}
        shutdown( lpClientContext->Socket, 2 );
        nsts = closesocket(lpClientContext->Socket);
#ifdef ACCEPT_LOG_LEVEL3
		LEVEL_3_ACCEPTLOG(lpClientContext, "closesocket():SOCKET.C");
#endif
        lpClientContext->Socket = 0;
	  }
#ifdef UPDATE_20060515 // SOCKET関連メモリ開放抜け
#ifdef IPv6
	  if (lpClientContext->phsin1) {
#ifdef VC6
		freehostent(lpClientContext->phsin1);
#else
        freeaddrinfo(lpClientContext->AI);
#endif
		lpClientContext->phsin1 = NULL;
	  }
#endif
#endif
	  
#ifdef UPDATE_20140528 // IDLEコマンド実装
	  // IDLE スレッドが終わるまで待機
#ifdef ACCEPT_LOG_LEVEL3
      LEVEL_3_ACCEPTLOG(lpClientContext, "IDLEThraed term check start");
#endif
#ifdef UPDATE_20161221 // セッションクローズ処理でIDLEフラグポインタが不定値で判定している不具合
	  pContext = &lpClientContext->Context;
#endif
	  //if (bDebug) printf("[%08x] IDLEThraed term check start\n", lpClientContext);
      while(pContext->bIDLE || pContext->bIDLEThread)
	  {
	    pContext->bIDLE = FALSE;
	    Sleep(5);
	  }
#ifdef UPDATE_20141018 // IDLEスレッドの起動に失敗しハングする可能性の対策
#ifdef UPDATE_20141025 // DONEコマンド受信時にIDLEスレッドのハンドル値をリセットしてメモリ開放に失敗していた
	  if (pContext->hIDLEThread != NULL && pContext->hIDLEThread != -1L)
#else
	  if (pContext->hIDLEThread != -1L)
#endif
	  {
	    WaitForSingleObject(pContext->hIDLEThread, INFINITE);
        CloseHandle(pContext->hIDLEThread);
#ifdef UPDATE_20141025 // DONEコマンド受信時にIDLEスレッドのハンドル値をリセットしてメモリ開放に失敗していた
		pContext->hIDLEThread = NULL;
#else
		pContext->hIDLEThread = -1L;
#endif
	  }
#endif
#ifdef ACCEPT_LOG_LEVEL3
      LEVEL_3_ACCEPTLOG(lpClientContext, "IDLEThraed term check end");
#endif
	  //if (bDebug) printf("[%08x] IDLEThraed term check end\n", lpClientContext);
	  /*
	  if (pContext->hIDLEThread)
	  {
	    TerminateThread(pContext->hIDLEThread, 0);   // スレッドのハンドル
	  }
	  */
#endif
	  
	  lpClientContext->bUsed = FALSE;
      // Free context structuyres.
	  lpClientContext->Buffer[0] ='\x0';
	  ///// removed recive data /////////
	  pContext = &lpClientContext->Context;
#ifdef UPDATE_20151115 // SELECT時にメール一覧をキャッシュするためのポインタ
	  if (bSELECT && pContext->pFnLists)
	  {
        LocalFree(pContext->pFnLists);
	    pContext->pFnLists = NULL;
	  }
#endif
      if (LocalHandle(lpClientContext))
#ifdef UPDATE_20061031 // 開放されていないメモリをリセットしてしまう可能性
		if (LocalFlags(lpClientContext) != LMEM_INVALID_HANDLE) {// 解放可能かどうか確認
          LocalFree( lpClientContext );
	      lpClientContext = NULL;
		}
#else
	    if (LocalFlags(lpClientContext) != LMEM_INVALID_HANDLE) // 解放可能かどうか確認
	      LocalFree(lpClientContext);
	  lpClientContext = NULL;
#endif
	}
	if (nAcceptCount > 0)
	  nAcceptCount--;
    return;
} // CloseClient

int WSAGetLastErrorName(void)
{
  int nsts = WSAGetLastError();
/*
  FILE *fp;
  fp = fopen("sock-pop3.log", "at");
  if (fp) {
   switch (nsts) {
     case WSANOTINITIALISED: fprintf(fp, "WSANOTINITIALISE\n"); break;
     case WSAENETDOWN: fprintf(fp, "WSAENETDOWN\n"); break;
     case WSAEACCES: fprintf(fp, "WSAEACCES\n"); break;
     case WSAEINTR: fprintf(fp, "WSAEINTR\n"); break;
     case WSAEINPROGRESS: fprintf(fp, "WSAEINPROGRESS\n"); break;
     case WSAEFAULT: fprintf(fp, "WSAEFAULT\n"); break;
     case WSAENETRESET: fprintf(fp, "WSAENETRESET\n"); break;
     case WSAENOBUFS: fprintf(fp, "WSAENOBUFS\n"); break;
     case WSAENOTCONN: fprintf(fp, "WSAENOTCONN\n"); break;
     case WSAENOTSOCK: fprintf(fp, "WSAENOTSOCK\n"); break;
     case WSAEOPNOTSUPP: fprintf(fp, "WSAEOPNOTSUPP\n"); break;
     case WSAESHUTDOWN: fprintf(fp, "WSAESHUTDOWN\n"); break;
     case WSAEWOULDBLOCK: fprintf(fp, "WSAEWOULDBLOCK\n"); break;
     case WSAEMSGSIZE: fprintf(fp, "WSAEMSGSIZE\n"); break;
     case WSAEHOSTUNREACH: fprintf(fp, "WSAEHOSTUNREACH\n"); break;
     case WSAEINVAL: fprintf(fp, "WSAEINVAL\n"); break;
     case WSAECONNABORTED: fprintf(fp, "WSAECONNABORTED\n"); break;
     case WSAECONNRESET: fprintf(fp, "WSAECONNRESET\n"); break;
     case WSAETIMEDOUT: fprintf(fp, "WSAETIMEDOUT\n"); break;
	 default: fprintf(fp, "OTHER=%d\n", nsts); break;
   }
   fclose(fp);
  }
*/
  if (bDebug) {
    switch (nsts) {
      case WSANOTINITIALISED: printf("WSANOTINITIALISE\n"); break;
      case WSAENETDOWN: printf("WSAENETDOWN\n"); break;
      case WSAEACCES: printf("WSAEACCES\n"); break;
      case WSAEINTR: printf("WSAEINTR\n"); break;
      case WSAEINPROGRESS: printf("WSAEINPROGRESS\n"); break;
      case WSAEFAULT: printf("WSAEFAULT\n"); break;
      case WSAENETRESET: printf("WSAENETRESET\n"); break;
      case WSAENOBUFS: printf("WSAENOBUFS\n"); break;
      case WSAENOTCONN: printf("WSAENOTCONN\n"); break;
      case WSAENOTSOCK: printf("WSAENOTSOCK\n"); break;
      case WSAEOPNOTSUPP: printf("WSAEOPNOTSUPP\n"); break;
      case WSAESHUTDOWN: printf("WSAESHUTDOWN\n"); break;
      case WSAEWOULDBLOCK: printf("WSAEWOULDBLOCK\n"); break;
      case WSAEMSGSIZE: printf("WSAEMSGSIZE\n"); break;
      case WSAEHOSTUNREACH: printf("WSAEHOSTUNREACH\n"); break;
      case WSAEINVAL: printf("WSAEINVAL\n"); break;
      case WSAECONNABORTED: printf("WSAECONNABORTED\n"); break;
      case WSAECONNRESET: printf("WSAECONNRESET\n"); break;
      case WSAETIMEDOUT: printf("WSAETIMEDOUT\n"); break;
	  default: printf("OTHER=%d\n", nsts); break;
    }
  }
   return nsts;
}