////////////////////////////////////////////////////////////
// Profile.c Copyright K.kawakami
// Get profile key and data.
////////////////////////////////////////////////////////////
#include "smtp.h"
#include <windows.h>
#include <stdio.h>
#include <io.h>
#include <windns.h>
#ifdef ADD_IDCACHE
#include <share.h>
#endif
#include <sys/types.h> 
#include <sys/stat.h>
//#include "profile.h"

#ifdef CLUSTERING
extern BOOL nClustering;
#endif

#ifdef V4
extern INT  nMAXUser;
#endif
#ifdef UPDATE_20070521 // OSの予約語対策
extern CHAR   mReservedWords[];
#endif

//#define PROFILE_ROOT_TREE "SOFTWARE\\EMWAC\\%s\\"
#define PROFILE_ROOT_TREE "%s\\"
#define MAX_VALUE_NAME    128
#define POP3_LOCKFILE     TEXT("$LockFile") 

#define MAX_REG_RCPT      512 //384

extern DWORD nTMOut;
extern BOOL  bUsedSaveList;
extern BOOL  bDebug;
extern BOOL  bCountLock;
#ifdef UPDATE_ULONGLONG //64bit長のデータで取得可能にする
extern DWORD nMailInBoxSizeHigh;
#endif
extern DWORD nMailInBoxSize;
extern DWORD nMailInMaxSize;
extern CHAR  mMailBoxDir[];
extern BOOL  bListenMode;
extern BOOL  bUsedSSL;
extern char mProgPath[];
extern char mMailSpoolDir[];
extern char mMailQueueDir[];
extern char  mLocalDomain[];
extern char  mPasswordFile[];
#ifdef V4
#ifdef UPDATE_20070510 // 実ユーザ名でのメール送受信を禁止処理する処理
extern char  mUseSMTPFile[];
#endif
extern char  mUseTimeFile[];
#ifdef UPDATE_20050924  // 送信元ユーザ別送信先制限
extern char  mSenderPermitFile[];
#endif
#endif
extern BOOL  bServiceTerminating;
extern DWORD nFROMSecLevel;
#ifdef UPDATE_20051121 // メール連続受領に対する配送キュー調整
extern DWORD  nSpoolFsSync;
#endif
#ifdef UPDATE_20091120 // ML保存時のindex.html作成のための排他処理、64bit長のファイルサイズに対応
extern CRITICAL_SECTION g_csMLIndex;
#endif
#ifdef ADD_IDCACHE
extern BOOL bServiceTerminating;
extern int  nIDCashLiveTime; // ADキャッシュ利用有効時間(秒単位)
extern char mComName[];
#endif
#ifdef UPDATE_20160113 // 認証時のロックアウト機能を追加
extern DWORD nAuthLockOut; // ロックアウトまでの回数 デフォルト 0:無効
extern char  mRejectPeerPFile[];
#endif
#ifdef UPDATE_20190216 // IP毎の同時接続ロックアウト機能を追加
extern DWORD nIPLockOut; // IPロックアウトまでの回数 デフォルト 0:無効
extern DWORD nIPLockOutTime; // ロックアウト時間 デフォルト 0:無限 分単位
#ifdef UPDATE_20190218 // IP毎の同時接続ロックアウト機能を強化（サンプル時間と拒絶時間の設定を追加）
extern DWORD	nIPLockOutSPTime; // IPロックアウトまでのサンプリング時間 デフォルト 0:無限 秒単位
#endif
#endif
#ifdef UPDATE_20161111 /// 外部からの送信元送信先制限(逆上長承認)
extern char	mRecivePermitFile[];
#endif
#ifdef UPDATE20240728 // サブネットマスクの範囲指定の高速化
extern BOOL bIPcmpmaskengin;
#endif

#ifdef Y2038_BUG
void gettime(SYSTEMTIME *ltime, char *mt);
#else
void gettime(time_t *ltime, char *mt);
#endif

#ifdef ACCEPT_LOG_LEVEL3
void LEVEL_3_ACCEPTLOG(PCLIENT_CONTEXT lpClientContext, char *mess);
#endif

#ifdef UPDATE_20240325 // 承認依頼のメールアドレスが、MLアドレスの場合、付加表題やMLカウント、メール保存を実施可能なようにした。
// 検討中
void MLAction(char *pUser, char *pMLtkn)
{
	BOOL bReq = FALSE;
	BOOL bMList = FALSE;
	char *pdom = NULL;

    if ((pdom = strstr(mUser, "@"))) { /// 2002.09.04 ドメイン有りのMLチェック
      if (!(bMList = GetMLists((LPCTSTR)SOFT_LISTS_REG, (char *)mUser, &bReq)))
	  {
        *pdom = '\x0';
        bMList = GetMLists((LPCTSTR)SOFT_LISTS_REG, (char *)mUser, &bReq);
 	    *pdom = '@';
	  }
	} else {
	  bMList = GetMLists((LPCTSTR)SOFT_LISTS_REG, (char *)mUser, &bReq);
	}
    if (bMList)
	{
      if (ListsCheck(pContext->mUIDFROM, &mRCPTO[11], mMLtkn, NULL, NULL, FALSE))
	  { //2001.12.06
	  //Subject: ヘッダなら
              SubjectConv(pContext->Tempfp, 
			 	          mMLWord,
						  pContext->mMLtkn, 
						  mSubject, 
						  mDBSubject,
						  mPack);
	}
    ListsSave(&mRCPTO[11], mDBSubject, nMLNo, pContext->mMsgId, mListDir);  // 保存フラグが指定されていればしてフォルダへメッセージ保存
	}
#endif

#ifdef UPDATE_20190426 // 利用する暗号アルゴリズムの組み合わせオプション 
void Print_SSL_CIPHER_names(const SSL_CTX *ctx)
{
#ifdef VC6
  const _STACK *ciphers;
#else
  STACK_OF(SSL_CIPHER) *ciphers;
#endif
  const SSL_CIPHER *cipher;
  int i, num;

  if (ciphers = SSL_CTX_get_ciphers(ctx))
  {
#ifdef VC6
	 num = (int)((ciphers) ? (ciphers)->num:-1); 
#else
	 num = sk_SSL_CIPHER_num(ciphers);
#endif
     for(i = 0; i < num; i++)
	 {
#ifdef VC6
	   cipher = ((ciphers) ? (ciphers)->data[i] : NULL); 
#else
	   cipher = sk_SSL_CIPHER_value(ciphers, i);
#endif
       printf("%s\n", SSL_CIPHER_get_name(cipher));
     }
#ifdef VC6
	 /*
     if (ciphers == NULL)
       return;
     for (i = 0; i < num; i++)
       if (ciphers->data[i] != NULL)
         OPENSSL_free((void *)ciphers->data[i]); // ハングする
     OPENSSL_free(ciphers);
	 */
	 //sk_SSL_CIPHER_free(ciphers);
#else
     //sk_SSL_CIPHER_free(ciphers);
#endif
  }
}
#endif
#ifdef DNS_QUERY
#ifdef UPDATE_20080703 // メールフィルタのRBL参照DNS指定で、キャッシュ有効無効のオプションを追加
CHAR *IPQuery(INT nAF, CHAR *pDNS, BOOL bQuery, CHAR *pDomain)
#else
CHAR *IPQuery(INT nAF, CHAR *pDNS, CHAR *pDomain)
#endif
{
  BOOL nSts = FALSE;
  DNS_STATUS status;               //Return value of  DnsQuery_A() function.
  PDNS_RECORD pDnsRecord;          //Pointer to DNS_RECORD structure.
  PIP4_ARRAY pSrvList = NULL;      //Pointer to IP4_ARRAY structure.
  CHAR DnsServIp[255];             //DNS server ip address.
  DNS_FREE_TYPE freetype ;
  freetype =  DnsFreeRecordListDeep;
   ///////////////////////////////////
   // MXレコード
   if (pDNS) 
   {
     if((pSrvList = (PIP4_ARRAY) LocalAlloc(LPTR,sizeof(IP4_ARRAY))))
	 {
       strcpy(DnsServIp, pDNS);
       pSrvList->AddrCount = 1;
       pSrvList->AddrArray[0] = inet_addr(DnsServIp); //DNS server IP address
	 }
   }
   ///////////////////////////////////
   status = DnsQuery(pDomain,       //Pointer to OwnerName. 
                     DNS_TYPE_A,             //Type of the record to be queried.
#ifdef UPDATE_20080703 // メールフィルタのRBL参照DNS指定で、キャッシュ有効無効のオプションを追加
                     (bQuery ? DNS_QUERY_STANDARD : DNS_QUERY_BYPASS_CACHE), //DNS_QUERY_STANDARD,     // Bypasses the resolver cache on the lookup. 
#else
                     DNS_QUERY_STANDARD, // DNS_QUERY_BYPASS_CACHE, //DNS_QUERY_STANDARD,     // Bypasses the resolver cache on the lookup. 
#endif
                     pSrvList,               //Contains DNS server IP address.
                     &pDnsRecord,            //Resource record that contains the response.
                     NULL);                  //Reserved for future use.
   if (!status)
   {      // 見つかった
	 nSts = TRUE;
	 DnsRecordListFree(pDnsRecord, freetype);
   }
   LocalFree(pSrvList);
   return nSts;
}
#endif

#ifdef UPDATE_20070301 // 対象アドレスの比較 *ワイルドカード有効にする 2007.03.01
char * _stristr( const char *string1, const char *string2 ) {
   int  n;
   char *p, *q, *r;
   char c[2];

   r = p = string1;
   q = string2;
   n = strlen(string2);
   if (n == 0)
	 return NULL;

   while(*p) {
	 if (*p >= 0x41 && *p <= 0x5a) // アルファベットが大文字なら小文字に変換
	   c[0] = *p + 0x20;
      else	 
	   c[0] = *p;
	 if (*q >= 0x41 && *q <= 0x5a) // アルファベットが大文字なら小文字に変換
	   c[1] = *q + 0x20;
      else	 
	   c[1] = *q;

      p++;
	  if (c[0] == c[1]) {
		q++;
		n--;
		if (n == 0)
		  break;
	  } else {  // はじめからチェック
		r = p;
	    q = string2;
        n = strlen(string2);
	  }
   }

   if (n != 0)
	 return NULL;
   else
	 return r;

}

BOOL  wildcard_stricmp(const char *string1, const char *string2) {
  char *p, *q, *r;
  int  k, l, m, n = 0;

  if (p = strchr(string1,'*')) {
    *p = '\x0';
    k = strlen(string1);
    l = strlen(p+1);
    *p = '*';
	m = 0;
#ifdef UPDATE_20070922 // Comp:1オプションで先頭カラムと最後尾カラムのワイルドカード指定があると処理できない
	if (k == 0) {
	  m = strlen((char *)(string1+1));
	}
#endif
    //if (k+l <= (int)strlen(string2))
	{
      if (strnicmp(string2, string1, k) == 0) {
		n += k;
	    if (q = strchr(p+1, '*')) {
	      *q = '\x0';
          l = strlen(p+1);
	      m = strlen(q+1);
#ifdef UPDATE_20180529A // Comp:1オプションで最後尾カラムのワイルドカード指定があると処理できない
		} else if (*(p+1) == '\x0') {
		  return TRUE;
#endif
		}
		//if ((r = strstr(string2+n, p+1))) {
		r = _stristr(string2+n, p+1);
		if (q)
		  *q = '*';
		if (r) { // 大小文字
          if (m > 0) {
            return wildcard_stricmp(p+1, r);
		  } else {
			return ((strlen(p+1) == strlen(r) || q) ? TRUE : FALSE);
		    //return TRUE;
		  }
        } else {
		  return (q && r ? TRUE : FALSE);
		}
      } else {
		return FALSE;
	  }
    //} else {
  	//  return FALSE;
	}
  } else {
	return (stricmp(string1, string2)== 0 ? TRUE : FALSE);
  }
}
#endif

#ifdef UPDATE_20070509 // エイリアス設定の実アドレスオプションを別データとして処理する
BOOL IP_COMP(char *pRange, char *pAddr) {
  char *pAster, *pmask;
#ifdef UPDATE20240728 // サブネットマスクの範囲指定の高速化
  unsigned long mask, num;
  int  i, n, addr, start, dot;
#else
  int  i, n, mask, num, addr, start, dot;
#endif
  BOOL bsts = FALSE;

  num = 1; // 比較アドレス数
  if ((pAster = strchr(pRange, '*'))) { // ワイルドカードでのマスク
	*pAster = '\x0';
  } else if (strchr(pRange, '.')) { // IPv4のみ
    if ((pAster = strchr(pRange, '/'))) {// ネットマスク
	  *pAster = '\x0';
#ifdef UPDATE20240728 // サブネットマスクの範囲指定の高速化
      if (bIPcmpmaskengin) // 新型
      {
        unsigned long nip;
        unsigned long sip;
        unsigned long eip;
		int  nmask;
		
	    if ((mask = 32 - atoi(++pAster)) < 0)
 		  mask = 0;
        num = 1L << mask; // 2のべき乗範囲
		nmask  = (-1L << (32-mask))-1;

        sip = inet_addr(pAddr); //inet_pton(AF_INET, eaddr, &sip);
        sip = htonl(sip);
		sip = (sip & (num*-1L));
		eip = (sip + num);
		nip = inet_addr(pRange); //inet_pton(AF_INET, pContext->mip, &nip);
        nip = htonl(nip);
        if (nip >= sip && nip < eip)
        {
          bsts = TRUE;  // 一致するものがあった
//          break;
        }
      } else {
#endif
	  mask = 32 - atoi(++pAster);
	  num = 1 << mask; // 2のべき乗範囲 num <<= mask; // 2のべき乗範囲
	  dot = ((mask-1) / 8) + 1;
	  if (dot > 1)
	    addr = num / (256*(dot-1));
	  else
	    addr = num;
	  for (i = 1; i <= dot; i++) {
	    if ((pAster = strrchr(pRange, '.')))
		  *pAster = '\x0';
	  }
  	  start = atoi(pAster + 1);
#ifdef UPDATE20240728 // サブネットマスクの範囲指定の高速化
	  }
#endif
	}
  }
#ifdef UPDATE_20211210_IPV6PREFIX // IPv6アドレスプレフィックスに対応した。
  else if (strchr(pRange, ':')) { // IPv6
     if ((pAster = strchr(pRange, '/'))) {// ネットマスク
       num = 0;
	   *pAster = '\x0';
	   bsts = Compear_IPv6_Addr(pAddr, pRange, pAster);
    }
  }
#endif
  if (num == 1) {
#ifdef UPDATE_20071022 // IP比較で、完全一致しない
    if (pAster && !strncmp(pAddr, pRange, strlen(pRange)) || //ワイルドカード有り
		!pAster && !strcmp(pAddr, pRange))   // ワイルドカードなし
	{
      bsts = TRUE;  // 一致するものがあった
	}
#else
    if (!strncmp(pAddr, pRange, strlen(pRange))) 
	{
      bsts = TRUE;  // 一致するものがあった
	}
#endif
  } else { // ネットマスクの場合
#ifdef UPDATE_20081111 // IP_COMP()内のネットマスクのチェック範囲に誤りがあった
	if (strlen(pAddr) > strlen(pRange))
#endif
	{
	  for ( i = 0; i < addr; i++)
	  {
#ifdef UPDATE_20081111 // IP_COMP()内のネットマスクのチェック範囲に誤りがあった
	    *pAster = '\x0';
	    pmask = (pRange+strlen(pRange));
        n = atoi(pAddr+strlen(pRange)+1);
	    sprintf(pmask, ".%d", start + i);
        if ((start+i == n) &&
		    !strncmp(pAddr, pRange, strlen(pRange)))
#else
	    pAster = '\x0';
	    pmask = (pRange+strlen(pRange));
	    sprintf(pmask, ".%d", start + i);
        if (!strncmp(pAddr, pRange, strlen(pRange)))
#endif
		{
	      bsts = TRUE;  // 一致するものがあった
		}
	  }
	}
  }
  return bsts;
}
#endif

#ifdef UPDATE_20061118 // フォルダで使用不可のキャラクタ
BOOL BadCharcter(char *src) {
  int i, j;
  char mBadChar[12] = "\\/:*?\"<>|"; // フォルダで使用不可のキャラクタ

  //// ドメインフォルダ作成に有効なキャラクタであるかチェック
  for (i = 0; i < (int)strlen(src); i++) {
	for (j = 0; j < (int)strlen(mBadChar); j++) {
	  if (*(src+i) == mBadChar[j]) {
	    return FALSE;
	  }
	}
  }
  return TRUE;
}
#endif

#ifdef BTHREAD
BOOL put_reply(PCLIENT_CONTEXT lpClientContext, BOOL bOnLog) {
  char mec[128];
  int  err;
  PSmtpContext   pContext = &lpClientContext->Context;

#ifdef USE_SSL
#ifdef USE_STARTTLS
  if (( pContext->bUsedSSL || 
       !bListenMode && bUsedSSL) &&
       lpClientContext->ssl) 
#else
  if (( bListenMode && pContext->bUsedSSL || 
       !bListenMode && bUsedSSL) &&
       lpClientContext->ssl) 
#endif
  {
  	  err = SSL_write(lpClientContext->ssl, pContext->mMessages, strlen(pContext->mMessages));
//	  lpClientContext->ssl->rbio->bOnMemory  = 1; // TRUE;
  } else
#endif
  err = send(lpClientContext->Socket, pContext->mMessages, strlen(pContext->mMessages), 0 );
  if (bOnLog) {
    if (bDebug) printf("[%08x] SEND[%08x] -> [%s]\n", lpClientContext, lpClientContext->Socket, pContext->mMessages);
#ifdef ACCEPT_LOG_LEVEL3
    LEVEL_3_ACCEPTLOG(lpClientContext, pContext->mMessages );
#endif
  }
  if ( err == SOCKET_ERROR ) {
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
    if (bDebug) printf("[%08x] SEND[%08x] ERROR -> [%s]\n", lpClientContext, lpClientContext->Socket, mec);
#ifdef ACCEPT_LOG_LEVEL3
	LEVEL_3_ACCEPTLOG(lpClientContext, mec);
#endif
    return FALSE;
  }
  return TRUE;
}

BOOL get_reply(PCLIENT_CONTEXT lpClientContext, BOOL bOnLog) {
  int  n, l, remain;
  char *p;
  char mec[128];
  int  err;
  PSmtpContext    pContext = &lpClientContext->Context;
//#ifdef UPADTE_20040202
   int    ns;
   fd_set ds;
   struct timeval tmo;
//#endif

#ifndef TUNING 
    memset(lpClientContext->Buffer, 0, sizeof(lpClientContext->Buffer));
#else
	lpClientContext->Buffer[0] = '\x0';
#endif
    n = 0;
    do {
	  l = 0;
      n = strlen(lpClientContext->Buffer);
      if (n >= (sizeof(lpClientContext->Buffer)-1)) {
		sprintf(mec, "get_reply() ERROR = overflow of token.");
	    if (bDebug) printf("[%08x] %s\n",lpClientContext, mec);
#ifdef ACCEPT_LOG_LEVEL3
	   LEVEL_3_ACCEPTLOG(lpClientContext, mec);
#endif
         return FALSE;
	  }
	  remain = sizeof(lpClientContext->Buffer) - 1 - n;
#ifdef UPDATE_20050518
	  if (nTMOut) { // 0 ならタイムアウトしない。
        tmo.tv_sec = (int)nTMOut; // 1秒単位
 	    tmo.tv_usec = 0;            // microseconds 
	    FD_ZERO(&ds);
	    FD_SET(lpClientContext->Socket, &ds);
	    if ((ns = select(lpClientContext->Socket, &ds, (fd_set *)NULL, (fd_set *)NULL, &tmo)) > 0) {
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
		    l = SSL_read(lpClientContext->ssl, &lpClientContext->Buffer[n], (remain >= 2048 ? 2048 : remain)); // opensslサイズの限界のため sizeof(lpClientContext->Buffer)-1-n);
		  } else
#endif
            l = recv(lpClientContext->Socket, &lpClientContext->Buffer[n], remain /*sizeof(lpClientContext->Buffer)-1-n*/, 0);
		} else {
          l = SOCKET_ERROR;
		}
	  } else {
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
		  l = SSL_read(lpClientContext->ssl, &lpClientContext->Buffer[n], (remain >= 2048 ? 2048 : remain)); // opensslサイズの限界のため sizeof(lpClientContext->Buffer)-1-n);
		} else
#endif
          l = recv(lpClientContext->Socket, &lpClientContext->Buffer[n], remain /*sizeof(lpClientContext->Buffer)-1-n*/, 0);
	  }

#else
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
		l = SSL_read(lpClientContext->ssl, &lpClientContext->Buffer[n], (remain >= 2048 ? 2048 : remain)); // opensslサイズの限界のため sizeof(lpClientContext->Buffer)-1-n);
	  } else
#endif
#ifdef UPDATE_20040202
 		if (nTMOut) { // 0 ならタイムアウトしない。
          tmo.tv_sec = (int)nTMOut; // 1秒単位
 	      tmo.tv_usec = 0;            // microseconds 
	      FD_ZERO(&ds);
	      FD_SET(lpClientContext->Socket, &ds);
	      if ((ns = select(lpClientContext->Socket, &ds, (fd_set *)NULL, (fd_set *)NULL, &tmo)) > 0) {
            l = recv(lpClientContext->Socket, &lpClientContext->Buffer[n], remain /*sizeof(lpClientContext->Buffer)-1-n*/, 0);
		  } else {
            l = SOCKET_ERROR;
		  }
		} else
#endif
          l = recv(lpClientContext->Socket, &lpClientContext->Buffer[n], remain /*sizeof(lpClientContext->Buffer)-1-n*/, 0);
#endif
	  if (l == 0) { // 0 = gracefully closed. // 丁寧な終了なら受信終了
	    break; // これを有効にするとCPU負荷１００%になる場合がある。2004.01.21
	  } else
	  if (l == SOCKET_ERROR || 
	      strlen(&lpClientContext->Buffer[n]) == 0) {
	    err = WSAGetLastError();
	    switch (err) {
	      case WSANOTINITIALISED: sprintf(mec, "read() SOCKET_ERROR WSANOTINITIALISED=%d", err); break;
	  	  case WSAENETDOWN: sprintf(mec, "read() SOCKET_ERROR WSAENETDOWN=%d", err); break;
		  case WSAEFAULT: sprintf(mec, "read() SOCKET_ERROR WSAEFAULT=%d", err); break;
		  case WSAEINPROGRESS: sprintf(mec, "read() SOCKET_ERROR WSAEINPROGRESS=%d", err); break;
		  case WSAEINVAL: sprintf(mec, "read() SOCKET_ERROR WSAEINVAL=%d", err); break;
		  case WSAEMFILE: sprintf(mec, "read() SOCKET_ERROR WSAEMFILE=%d", err); break;
		  case WSAENOBUFS: sprintf(mec, "read() SOCKET_ERROR WSAENOBUFS=%d", err); break;
		  case WSAENOTSOCK: sprintf(mec, "read() SOCKET_ERROR WSAENOTSOCK=%d", err); break;
		  case WSAEOPNOTSUPP: sprintf(mec, "read() SOCKET_ERROR WSAEOPNOTSUPP=%d", err); break;
		  case WSAEWOULDBLOCK: sprintf(mec, "read() SOCKET_ERROR WSAEWOULDBLOCK=%d", err); break;
		  default:  sprintf(mec, "get_reply() SOCKET_ERROR OTHER CODE=%d", err); break;
		}
        if (bDebug) printf("[%08x] RECV[%08x] ERROR<- [%s]\n", lpClientContext, lpClientContext->Socket, mec);
	    if (bDebug) printf("[%08x] %s\n",lpClientContext, mec);
#ifdef ACCEPT_LOG_LEVEL3
	   LEVEL_3_ACCEPTLOG(lpClientContext, mec);
#endif
        return FALSE;
	  } 
	  lpClientContext->Buffer[n+l] = '\x0';
	//printf("lpClientContext->Buffer[%d+%d]=%s\n", n, l, lpClientContext->Buffer);
      p = strchr(lpClientContext->Buffer,'\n'); // ライン終了検出
	  if (!p)
		_sleep(0); // 他スレッドに切替
	} while(!p);
    if (bOnLog) {
      if (bDebug) printf("[%08x] RECV[%08x] <- [%s]\n", lpClientContext, lpClientContext->Socket, lpClientContext->Buffer);
#ifdef ACCEPT_LOG_LEVEL3
      LEVEL_3_ACCEPTLOG(lpClientContext, lpClientContext->Buffer);
#endif
	}
	if (l == 0)
	  return FALSE;  // 切断された為終了
	else
      return TRUE;
}
#endif

char tochar(char *p) {
  int  i;
  char c = 0x00;
 
  for (i = 0; i < 2; i++) {
	switch (*(p+i)) {
	case '0': c |= 0x00 >> 4*i; break;
	case '1': c |= 0x10 >> 4*i; break;
	case '2': c |= 0x20 >> 4*i; break;
	case '3': c |= 0x30 >> 4*i; break;
	case '4': c |= 0x40 >> 4*i; break;
	case '5': c |= 0x50 >> 4*i; break;
	case '6': c |= 0x60 >> 4*i; break;
	case '7': c |= 0x70 >> 4*i; break;
	case '8': c |= 0x80 >> 4*i; break;
	case '9': c |= 0x90 >> 4*i; break;
	case 'a': 
	case 'A': c |= 0xA0 >> 4*i; break;
	case 'b': 
	case 'B': c |= 0xB0 >> 4*i; break;
	case 'c': 
	case 'C': c |= 0xC0 >> 4*i; break;
	case 'd': 
	case 'D': c |= 0xD0 >> 4*i; break;
	case 'e': 
	case 'E': c |= 0xE0 >> 4*i; break;
	case 'f': 
	case 'F': c |= 0xF0 >> 4*i; break;
	}
  }
  return c;
}

void SPA_Encode(char *pSrc, char *pDest) { // 記号化
   int i, n;
   char mEn[3];
   SYSTEMTIME lt;
   unsigned char cMask;
   
   if (*pSrc != '\xff' && strlen(pSrc)) {// 記号化フラグ無し
     GetSystemTime(&lt);
     //cMask = (char)(lt.wMilliseconds % 0xfe + 1);
	 cMask = (char)(lt.wMilliseconds % 0xef + 0x10);
     sprintf(pDest, "\xff%c", cMask); // 記号化フラグセット
     n = strlen(pSrc);
     for (i = 0; i < n; i++) {
	   //sprintf(mEn, "%02x", __toascii(*pSrc));
       sprintf(mEn, "%02x", (unsigned char)((unsigned char)((*pSrc)^cMask)));
       strcat(pDest, mEn);
	   pSrc++;
	 }
   } else {
     strcpy(pDest, pSrc); // そのまま
   }
}

void SPA_Decode(char *pSrc, char *pDest) { // 復号化
   int i, n;
   char mEn[3];
   unsigned char cMask;

   mEn[2] = '\x0';
   if (*pSrc == '\xff') {// 記号化フラグ有り
	 cMask = *(pSrc+1);
	 pSrc+=2;
     n = strlen(pSrc);
	 for (i = 0; i < n; i += 2) {
       strncpy(mEn, (pSrc+i), 2);
       *pDest = (char)(tochar(mEn)^cMask);
	   pDest++;
	 }
	 *pDest = '\x0';
   } else { // 記号化フラグ無し
	 strcpy(pDest, pSrc);
   }
}

void GetListsPass(char *mRcpt, char *mMlPass) {
  //// 投稿パスワード取出し
  CHAR mkey[MAX_REG_RCPT];
  char  *pdom;
  BOOL  bAliases = FALSE;
#ifdef UPDATE_20041208
#ifdef UPDATE_20071205 //RFC2821: メールアドレス長が255Byteのときの対策
   CHAR  mListRcpt[512];
#else
   CHAR  mListRcpt[256];
#endif
#endif
#ifdef UPDATE_20070521 // 予約語対策
  char *q;
#endif

  if (mMlPass) {
	 *mMlPass = '\x0';
#ifdef UPDATE_20041208
#ifdef UPDATE_20071205 // RFC2821:メールアドレス長が255Byteのときの対策
     strncpy(mListRcpt, mRcpt, 255);
     mListRcpt[255] = '\x0';
     pdom = strstr(mListRcpt, "@");
#else
     strncpy(mListRcpt, mRcpt, 255);
     mListRcpt[255] = '\x0';
     pdom = strstr(mListRcpt, "@");
#endif
#else
     pdom = strstr(mRcpt, "@");
#endif
     if (pdom)
	   *pdom = '\x0';
#ifdef UPDATE_20041208
#ifdef UPDATE_20070509 // エイリアス設定の実アドレスオプションを別データとして処理する
     bAliases = GetAliases((LPCTSTR)SOFT_ALIASES_REG, (char *)mListRcpt, (char *)pdom, NULL);
#else
     bAliases = GetAliases((LPCTSTR)SOFT_ALIASES_REG, (char *)mListRcpt, (char *)pdom);
#endif
#else
#ifdef UPDATE_20070509 // エイリアス設定の実アドレスオプションを別データとして処理する
     bAliases = GetAliases((LPCTSTR)SOFT_ALIASES_REG, (char *)mRcpt, (char *)pdom, NULL);
#else
     bAliases = GetAliases((LPCTSTR)SOFT_ALIASES_REG, (char *)mRcpt, (char *)pdom);
#endif
#endif
     if (pdom)
       *pdom = '@';
     if (!bAliases) {
#ifdef UPDATE_20041208
       sprintf(mkey,"%s\\%s", SOFT_LISTS_REG, mListRcpt);
#ifdef UPDATE_20070521 // 予約語対策
       strcpy(mListRcpt, mRcpt);
       strtok(mListRcpt, "@");
       if (ReservedWords(mListRcpt)) {
         if ((q = strstr(mRcpt, mListRcpt))) {
           strcpy(mListRcpt, mReservedWords);
           strcat(mListRcpt, q);
           sprintf(mkey,"%s\\%s", SOFT_LISTS_REG, mListRcpt);
		 }
	   }
#endif

#else
       sprintf(mkey,"%s\\%s", SOFT_LISTS_REG, mRcpt);
#endif
#ifdef MULTI_ML
     if (!QueryMLists((LPCTSTR)SOFT_LISTS_REG, mRcpt)) // ドメイン付MLが無い場合
       strtok(mkey,"@");
#else
       strtok(mkey,"@");
#endif
       GetProfileStringEx(mkey, "ListPassword", "", mMlPass, 256);
	 }
  }
}

#ifdef UPDATE_ULONGLONG //64bit長のデータで取得可能にする
ULONGLONG GetMailBoxLimit(char *pMailBox)
#else
DWORD GetMailBoxLimit(char *pMailBox)
#endif
{
 CHAR mLimit[256];
 FILE *fp;
#ifdef UPDATE_ULONGLONG //64bit長のデータで取得可能にする
 ULONGLONG nLimit = 0; // デフォルト無限
#else
 DWORD nLimit = 0; // デフォルト無限
#endif
   sprintf(mLimit, "%ssize.ctl", pMailBox);
   fp = fopen(mLimit, "rt");
   if (fp) {
#ifdef UPDATE_ULONGLONG //64bit長のデータで取得可能にする
	 fscanf(fp, "%I64u", &nLimit); //64bit整数
#else
	 fscanf(fp, "%lu", &nLimit);
#endif
	 fclose(fp);
   }
   return nLimit;
}

void GetMailBoxFolder(char *user, char *pMailBox)
{
 CHAR mHome[256];
 CHAR mMIBD[256], *p1, *p2;
 BOOL bHome = FALSE;
 INT  nl;
#ifdef UPDATE_20070521 // 予約語対策
#ifdef UPDATE_20071205 //RFC2821: メールアドレス長が255Byteのときの対策
 CHAR mKey[MAX_REG_RCPT];
#else
 CHAR mKey[256];
#endif
#endif

#ifdef V3
    BOOL bLink = FALSE;
    char mLink[256], *pdomain;

   mLink[0] = '\x0';
   pdomain = strstr(user, "@");
   if (pdomain) {
     *pdomain = '\x0';
	 pdomain++;
#ifdef UPDATE_20070521 // 予約語対策
     strcpy(mKey, pdomain);
     strtok(mKey, "@");
     if (ReservedWords(mKey)) {
       strcpy(mKey, mReservedWords);
       strcat(mKey, pdomain);
	 } else
	 {
       strcpy(mKey, pdomain);
	 }
     GetProfileStringEx(DOMAIN_FOLDER, mKey, "", mLink, sizeof(mLink)); // 対象ドメインの応答ＩＰアドレス取出し
#else
     GetProfileStringEx(DOMAIN_FOLDER, pdomain, "", mLink, sizeof(mLink)); // 対象ドメインの応答ＩＰアドレス取出し
#endif
#ifdef UPDATE_20040707  // ドメイン無しアドレスをデフォルトのドメインフォルダに入るようにする
   } else {
#ifdef UPDATE_20070521 // 予約語対策
     strcpy(mKey, mLocalDomain);
     strtok(mKey, "@");
     if (ReservedWords(mKey)) {
       strcpy(mKey, mReservedWords);
       strcat(mKey, mLocalDomain);
	 } else
	 {
       strcpy(mKey, mLocalDomain);
	 }
     GetProfileStringEx(DOMAIN_FOLDER, mKey, "", mLink, sizeof(mLink)); // 対象ドメインの応答ＩＰアドレス取出し
#else
     GetProfileStringEx(DOMAIN_FOLDER, mLocalDomain, "", mLink, sizeof(mLink)); // 対象ドメインの応答ＩＰアドレス取出し
#endif
#endif
   }
   if (mLink[0] != '\x0') {
     bLink = TRUE;
#ifdef UPDATE_20070521 // 予約語対策
     strcpy(mKey, mLink);
     strtok(mKey, "@");
     if (ReservedWords(mKey)) {
       strcpy(mKey, mReservedWords);
	   strcat(mKey, mLink);
       strcpy(mLink, mKey);
	 }
#endif
   }   
#ifdef UPDATE_20091101 // HOMEディレクトリタイプで無いならホームフォルダの検索をしないようにした
   mHome[0] = '\x0';
   if (!strnicmp(mMailBoxDir, "%home%", 6))
#endif
   {
#ifdef UPDATE_20091014A // ドメイン無しアカウントでホームディレクトリ参照時ハングする可能性がある
	 UserHomeDir( NULL, NULL, user, (pdomain ? pdomain : mLocalDomain), mHome);
#else
     UserHomeDir( NULL, NULL, user, pdomain, mHome);
#endif
#else
     UserHomeDir( NULL, NULL, user, mHome);
#endif
   }
   *pMailBox = '\x0';
   /*
   GetProfileStringEx((LPCTSTR)SOFT_REG,(LPCTSTR)"MailInBoxDir", (LPCTSTR) MAIL_BOX, (LPTSTR)mMIBD, 256);
   if (mMIBD[0] == '\x0')
	 strcpy(mMIBD, MAIL_BOX);
   */
   strcpy(mMIBD, mMailBoxDir);
   if (strnicmp(mMIBD, "%home%", 6) == 0) {
	 strcat(pMailBox, mHome);
	 bHome = TRUE;
   }
   p1 = strstr(mMIBD, "%USERNAME%");
   if (!p1)
     p1 = strstr(mMIBD, "%username%");
   if (p1) {
	 *p1 = '\x0';
     p2 = p1 + 10;
     if (bHome)
	   strcat(pMailBox, &mMIBD[6]);
	 else
	   strcat(pMailBox, mMIBD);
#ifdef V3
	 if (bLink) {
       strcat(pMailBox, mLink);
       strcat(pMailBox, "\\");
	 }
#endif
#ifdef UPDATE_20070521 // 予約語対策
	 if (ReservedWords(user))
	   strcat(pMailBox, mReservedWords);
#endif
#ifdef UPDATE_20060116 // バッファオーバーフロー対策
	 strncat(pMailBox, user, 250-(strlen(pMailBox)+strlen(p2)));
	 *(pMailBox+250) = '\x0';
#else
	 strcat(pMailBox, user);
#endif
	 strcat(pMailBox, p2);
   } else {
	 if (bHome) // %HOME% があるとき
	   strcat(pMailBox, &mMIBD[6]); 
	 else
	   strcat(pMailBox, mMIBD);     // %HOME% , %USERNAME%が共にない場合、メールボックスフォルダに固定 2003/08/17
   }
   // 末尾に "\"マークをなければ追加
   nl = strlen(pMailBox);
   if (nl != 0) {
     nl--;
     if (pMailBox[nl] != '\\')
      strcat(pMailBox,"\\");
   }
}
#ifdef UPDATE_20070521 // 予約語対策
BOOL ReservedWords(char *pUser) {
    BOOL bsts = FALSE;
    int i, n[2];
#ifdef UPDATE_20071205 //RFC2821: メールアドレス長が255Byteのときの対策
	CHAR mData[512];
#else
	CHAR mData[256];
#endif
	CHAR mWords[25][7] = {"CON", "PRN", "AUX", "CLOCK$", "NUL",
                          "COM0", "COM1", "COM2", "COM3", "COM4", "COM5", "COM6", "COM7", "COM8", "COM9",
                          "LPT0", "LPT1", "LPT2", "LPT3", "LPT4", "LPT5", "LPT6", "LPT7", "LPT8", "LPT9"};

	strcpy(mData, pUser);
	strtok(mData, "@");
    for(i = 0; i < 25; i++) {
	   n[0] = strlen(mData);
	   n[1] = strlen(mWords[i]);
	   if ( !stricmp(mData, mWords[i]) ||
           (!strnicmp(mData, mWords[i], n[1]) && *(pUser+n[1]) == '.') ||
		   (!strnicmp(mData, mWords[i], n[1]) && n[0] == n[1]+1 && *(pUser+n[1]) == ' ') ) {
		 bsts = TRUE;
	   }
	}
	return bsts;
}
#endif

#ifdef V4

#ifdef UPDATE_20070510 // 実ユーザ名でのメール送受信を禁止処理する処理
BOOL GetSMTPOFFFile(char *pUser, char *pDom) {
  FILE   *fp;
  CHAR   *p, Path[MAX_PATH];
  BOOL   bsts = FALSE;
  CHAR   mUser[1024], mMBoxDir[_MAX_PATH];

    if (pDom && !strchr(pUser, '@'))
       sprintf(mUser, "%s@%s", pUser, pDom);
    else
	  strcpy(mUser, pUser);
    GetMailBoxFolder(mUser, mMBoxDir);
    sprintf(Path, "%s%s", mMBoxDir, mUseSMTPFile);
	if ((fp = fopen(Path, "rt"))) {
	  fclose(fp);
	  bsts = TRUE;
      if (bDebug) printf(" -> found Valid user smtp file (%s)\n", Path);
	} else {
      if (bDebug) printf(" -> not found Valid user smtp file (%s)\n", Path);
	}
    return bsts;
}

#endif

BOOL GetUseTimeFile(char *pUser, char *pDom) {
  FILE   *fp;
  CHAR   *p, *pWeek, Path[MAX_PATH];
  BOOL   bsts = FALSE;
  SYSTEMTIME lt;
  CHAR   mUser[1024], mMBoxDir[_MAX_PATH], mt[256];
  CHAR   *pl, mWeek[4], mSchedule[256];
  int    nFrom, nTo, nNow;

    if (pDom && !strchr(pUser, '@'))
       sprintf(mUser, "%s@%s", pUser, pDom);
    else
	  strcpy(mUser, pUser);
    GetMailBoxFolder(mUser, mMBoxDir);
    sprintf(Path, "%s%s", mMBoxDir, mUseTimeFile);
    if (bDebug) printf("Valid time file name = %s\n", Path);
    fp = fopen(Path, "rt");
	if (fp) {
	  gettime(&lt, mt); // 現在時刻取り出し
	  pWeek = mt;      mt[3] = '\x0';
	  nNow = atoi(&mt[17]) * 60 + atoi(&mt[20]);
      //sprintf(mt, "%3s, %02d %3s %04d %02d:%02d:%02d %s", mWeek[lt.wDayOfWeek], lt.wDay, mMonth[lt.wMonth-1], lt.wYear, lt.wHour, lt.wMinute, lt.wSecond, mzone);
      p = fgets(mSchedule, sizeof(mSchedule), fp);
	  while(p || !feof(fp)) {
		mSchedule[3] = '\x0'; strcpy(mWeek, mSchedule);
        if ((pl = strpbrk(&mSchedule[4], ":"))) {
		  *pl++;
		  nTo = nFrom = atoi(&mSchedule[4]) * 60 + atoi(pl);
		  nTo++;
          if ((pl = strpbrk(pl, "-"))) {
			pl++;
		    nTo = atoi(pl) * 60;
            if ((pl = strpbrk(pl, ":"))) {
			  pl++;
		      nTo += atoi(pl);
			}
		  }
		  if (!stricmp(mWeek, pWeek) && nFrom <= nNow && nTo >= nNow) {  // 曜日＆時間範囲一致で許可
	        bsts = TRUE;
			break;
		  }
		} else {
		  if (!stricmp(mWeek, pWeek)) {  // 曜日一致で許可
	        bsts = TRUE;
			break;
		  }
		}
        p = fgets(mSchedule, sizeof(mSchedule), fp);
	  }
	  fclose(fp);
      if (bDebug) printf(" -> found Valid time file (%s)\n", Path);
	} else {
	  bsts = TRUE;
      if (bDebug) printf(" -> not found Valid time file (%s)\n", Path);
	}
    return bsts;
}

#ifdef UPDATE_20050924  // 送信元ユーザ別送信先制限
#ifdef UPDATE_20070516 // 上長承認機能の追加
#ifdef UPDATE_20070607 // 上長承認機能にSubject:キーワードも可能にする
#ifdef UPDATE_20080617 // 上長承認機能で添付ファイルの拡張子による承認条件追加
#ifdef UPDATE_20161111 /// 外部からの送信元送信先制限(逆上長承認)
BOOL GetSenderPermitFile(CHAR *pFrom, CHAR *pTo, DWORD *pOption, CHAR *pMIME, CHAR *pSubject, CHAR *pBoss, BOOL bType)
#else
BOOL GetSenderPermitFile(CHAR *pFrom, CHAR *pTo, DWORD *pOption, CHAR *pMIME, CHAR *pSubject, CHAR *pBoss)
#endif
#else
BOOL GetSenderPermitFile(CHAR *pFrom, CHAR *pTo, DWORD *pOption, CHAR *pSubject, CHAR *pBoss)
#endif
#else
BOOL GetSenderPermitFile(CHAR *pFrom, CHAR *pTo, DWORD *pOption, CHAR *pBoss)
#endif
#else
BOOL GetSenderPermitFile(CHAR *pFrom, CHAR *pTo)
#endif
{
  FILE   *fp;
  CHAR   *p, Path[MAX_PATH];
  CHAR   mTo[_MAX_PATH]; 
#ifdef UPDATE_20161111 /// 外部からの送信元送信先制限(逆上長承認)
  CHAR   mFrom[_MAX_PATH]; 
#endif
  BOOL   bsts = FALSE, bData = FALSE, bHit = FALSE;
#ifdef UPDATE_20250620 // 送信元ドメイン単位での送信先制限も可能に
  BOOL   bFound = FALSE;
#endif
  CHAR   mUser[1024], mMBoxDir[_MAX_PATH];
#ifdef UPDATE_20070516 // 上長承認機能の追加
  CHAR   *pAddr, mSender[_MAX_PATH*4];
#else
  CHAR   mSender[256];
#endif
  char   *pwild, *pwild2, *pat, *pat2;
  DWORD  i, n, k, l, m;
#ifdef UPDATE_20080617 // 上長承認機能で添付ファイルの拡張子による承認条件追加
  CHAR   c, *pMM;
#endif
#ifdef UPDATE_20161113 /// 対象アドレス欄にファイル指定を可能にする機能の追加
  FILE   *fps;
  CHAR   *pS, mSFn[MAX_PATH];
#endif

#ifdef UPDATE_20060710 // ワイルドカードの処理強化
    strncpy(mTo, pTo, MAX_PATH);
    mTo[MAX_PATH-1] = '\x0';
	strlwr(mTo);
#endif
#ifdef UPDATE_20161111 /// 外部からの送信元送信先制限(逆上長承認)
    strncpy(mFrom, pFrom, MAX_PATH);
    mFrom[MAX_PATH-1] = '\x0';
	strlwr(mFrom);
#endif
#ifdef UPDATE_20060711 // 全体の送信先制限も可能に
#ifdef UPDATE_20250620 // 送信元ドメイン単位での送信先制限も可能に
    for (i = 0; i < 3; i++)
#else
    for (i = 0; i < 2; i++)
#endif
#endif
	{
#ifdef UPDATE_20060711 // 全体の送信先制限も可能に
#ifdef UPDATE_20250620 // 送信元ドメイン単位での送信先制限も可能に
	  if (!bFound)
#endif
	  bsts = FALSE;
	  if (i == 0) {// 全体用
#ifdef UPDATE_20161111 /// 外部からの送信元送信先制限(逆上長承認)
		sprintf(Path, "%s%s", mProgPath, (bType ? mSenderPermitFile : mRecivePermitFile));
#else
        sprintf(Path, "%s%s", mProgPath, mSenderPermitFile);
#endif
	  } else
#ifdef UPDATE_20250620 // 送信元ドメイン単位での送信先制限も可能に
	  if (i == 1) {// 送信元ドメイン単位用
		sprintf(Path, "%spermit\\%s\\%s", mMailSpoolDir, (strchr(pFrom, '@') ? (strchr(pFrom, '@')+1): mLocalDomain), (bType ? mSenderPermitFile : mRecivePermitFile));
	  } else
#endif
#endif
	  {
        if (!strchr(pFrom, '@')) // ドメインがない場合はトップのドメインを付加
          sprintf(mUser, "%s@%s", pFrom, mLocalDomain);
        else
	      strcpy(mUser, pFrom);
        GetMailBoxFolder(mUser, mMBoxDir);
#ifdef UPDATE_20161111 /// 外部からの送信元送信先制限(逆上長承認)
		sprintf(Path, "%s%s", mMBoxDir, (bType ? mSenderPermitFile : mRecivePermitFile));
#else
        sprintf(Path, "%s%s", mMBoxDir, mSenderPermitFile);
#endif
	  }
#ifdef UPDATE_20161111 /// 外部からの送信元送信先制限(逆上長承認)
      if (bDebug) printf("%s permit file = %s\n",  (bType ? "Sender" : "Reciver") , Path);
#else
      if (bDebug) printf("Sender permit file = %s\n", Path);
#endif
	  if ((fp = fopen(Path, "rt"))) {
#ifdef UPDATE_20250620 // 送信元ドメイン単位での送信先制限も可能に
        bFound = TRUE;
#endif
	    while((p = fgets(mSender, sizeof(mSender), fp)))
		{
#ifdef UPDATE_20161113 /// 対象アドレス欄にファイル指定を可能にする機能の追加
		mSFn[0] = '\x0';
		fps = pS = NULL;
		if (!strnicmp(mSender, "file=", 5) && strlen(mSender) > 8)
		{
		  strtok(mSender, "\r\n");
		  strcpy(mSFn, &mSender[5]);
		  if ((pS = strchr(&mSFn[2], ':')))
		  {
			*pS = '\x0';
			pS++;
		  }
		  if (!(fps = fopen(mSFn, "rt")))
		  {
			continue;
		  }
		}
		if (fps)
		{
		  if (!fgets(mSender, sizeof(mSender)-1, fps))
		  {
			fclose(fps);
			continue;
		  }
		  strtok(mSender, "\r\n");
		  if (!strchr(mSender, ':'))
		  {
			if (pS)
			{
			  strcat(mSender, ":");
              strcat(mSender, pS);
			}
		  }
		}
		do 
		{
#endif
		//p = fgets(mSender, sizeof(mSender), fp);
	    //while(p || !feof(fp)) {
	      if (mSender[0] != '\x0' &&
	          mSender[0] != ' ' &&
	          mSender[0] != '\'') {  // コメントチェック
		    bData = TRUE;
		    strtok(mSender, "\r\n");
#ifdef UPDATE_20070516 // 上長承認機能の追加
		    *pOption = 0;
		    *pBoss = '\x0';
            if ((pAddr = strpbrk(mSender, ",:"))) { // 承認条件
              *pAddr = '\x0';
			  pAddr++;
			  *pOption = atoi(pAddr);
#ifdef UPDATE_20080617 // 上長承認機能で添付ファイルの拡張子による承認条件追加
			  if (pMM = strchr(pAddr, '='))
			  {
				pMM++;
				if (*pMM == '"') // ダブルコーテーションで括られている場合、フルパスのファイル名として認識
				{
				  pMM++;
				  if (strchr(pMM, '"'))
				  {
					pAddr = strchr(pMM, '"');
					*pAddr = '\x0';
					pAddr++;
				  }
                  strcpy(pMIME, pMM);
				} else {
				  if (pAddr = strpbrk(pAddr, ",:"))
				  {
					c = *pAddr;
					*pAddr = '\x0';
                    strcpy(pMIME, pMM);
					*pAddr = c;
				  } else {
                    strcpy(pMIME, pMM);
				  }
				}
				strtok(pMIME, "\r\n");
			  }
#endif
#ifdef UPDATE_20070607 // 上長承認機能にSubject:キーワードも可能にする
              if ((pAddr = strpbrk(pAddr, ",:"))) { // Subjectキーワード
                *pAddr = '\x0';
                pAddr++; // 上長アドレス
			    strncpy(pSubject, pAddr, 256);
			    *(pSubject+256) = '\x0'; // 以前 'x\0'になっていた
			    if (*pSubject == ':')
                  *pSubject = '\x0';
				else {
#ifdef UPDATE_20161112 //日付時差が一致していないとチェック対象にする条件
		         if ((!strnicmp(pSubject, "envf0=", 6) ||
					  !strnicmp(pSubject, "envf1=", 6)) &&
					 strlen(pSubject) >= 6) {
			        strtok(pSubject+6, ",:");
					pAddr += 6; 
				 } 
		         else 
			     if ((!strnicmp(pSubject, "sub0=", 5) ||
					  !strnicmp(pSubject, "sub1=", 5)) &&
					 strlen(pSubject) >= 5) {
			        strtok(pSubject+5, ",:");
					pAddr += 5; 
				 } 
		         else 
		         if ((!strnicmp(pSubject, "from0=", 6) ||
					  !strnicmp(pSubject, "from1=", 6)) &&
					 strlen(pSubject) >= 6) {
			        strtok(pSubject+6, ",:");
					pAddr += 6; 
				 } 
		         else 
			     if ((!strnicmp(pSubject, "date0=", 6) ||
					  !strnicmp(pSubject, "date1=", 6)) &&
					 strlen(pSubject) >= 6) {
			        strtok(pSubject+6, ",:");
					pAddr += 6; 
				 } 
		         else 
#endif
#ifdef UPDATE_20160415 //送信先に指定のアドレスまたはドメインが同報として含まれていないとチェック対象にする条件
		         if ((!strnicmp(pSubject, "to0=", 4) ||
					  !strnicmp(pSubject, "to1=", 4) ||
					  !strnicmp(pSubject, "cc0=", 4) || 
					  !strnicmp(pSubject, "cc1=", 4)) &&
					 strlen(pSubject) >= 4) {
			        strtok(pSubject+4, ",:");
					pAddr += 4; 
				 } 
		         else 
#endif
				  if (!strnicmp(pSubject, "file=", 5)  && strlen(pSubject) >= 7) {
			        strtok(pSubject+7, ",:");
					pAddr += 7; 
				  } else {
			        strtok(pSubject, ",:");
				  }
				}
                if ((pAddr = strpbrk(pAddr, ",:"))) { // 承認者
                  *pAddr = '\x0';
                  pAddr++; // 上長アドレス
			      strcpy(pBoss, pAddr);
				}
			  }
#else
              if ((pAddr = strpbrk(pAddr, ",:"))) { // 承認者
                *pAddr = '\x0';
                pAddr++; // 上長アドレス
			    strcpy(pBoss, pAddr);
			  }
#endif
			}
#endif

#ifdef UPDATE_20060710 // ワイルドカードの処理強化
#ifdef UPDATE_20161111 /// 外部からの送信元送信先制限(逆上長承認)
      if (bDebug) printf("%s permit file = %s\n",  (bType ? "Sender" : "Reciver") , Path);
            if (!strstr(mSender, "@") ) {
				pat = strstr((bType ? mTo : mFrom), "@");
			  if (pat)
			    pat++;
			  else
			   pat = (bType ? mTo : mFrom);
			} else {
              pat = (bType ? mTo : mFrom);
			}
#else
            if (!strstr(mSender, "@") ) {
		      pat = strstr(mTo, "@");
			  if (pat)
			    pat++;
			  else
			   pat = mTo;
			} else {
              pat = mTo;
			}
#endif
#ifdef UPDATE_20211231 // 承認アドレス条件に複数のワイルドカード指定を可能にした。
            if (wildcard_stricmp(mSender, pat))
#else
		    if (!stricmp(mSender, pat)) 
#endif
			{  // 許可ユーザーとする
#ifdef UPDATE_20070610 // 拒否指定オプションの追加
              if (*pOption != -1L)
#endif
	            bsts = TRUE;
#ifdef UPDATE_20060711 // 全体の送信先制限も可能に
			  bHit = TRUE;
#endif
		      break;
			} else if ((pwild = strchr(mSender, '*'))) { // ワイルドカードでのマスク
		      *pwild = '\x0';
			   k = strlen(mSender);
			   l = strlen(pwild+1);
			   if ((pwild2 = strstr(pwild+1,"*"))) {
			     *pwild2 = '\x0';
			     if ((pat2 = strstr(pat, pwild+1))) {
			       l = strlen(pwild+1);
			       m = strlen(pwild2+1);
			       if (k+l <= (int)strlen(pat) && l+m <= (int)strlen(pat2)) {
			         if (strnicmp(pat, mSender, k) == 0 &&
			             strnicmp(pat2, pwild+1, l) == 0 &&
					     strnicmp((pat2+(strlen(pat2)-m)), pwild2+1, m) == 0) {
#ifdef UPDATE_20070610 // 拒否指定オプションの追加
                       if (*pOption != -1L)
#endif
 	                     bsts = TRUE;
#ifdef UPDATE_20060711 // 全体の送信先制限も可能に
			           bHit = TRUE;
#endif
				       break;
					 }
				   }
				 }
			   } else {
			     if (k+l <= (int)strlen(pat)) {
			       if (strnicmp(pat, mSender, k) == 0 &&
				       strnicmp((pat+(strlen(pat)-l)), pwild+1, l) == 0) {
#ifdef UPDATE_20070610 // 拒否指定オプションの追加
                     if (*pOption != -1L)
#endif
 	                   bsts = TRUE;
#ifdef UPDATE_20060711 // 全体の送信先制限も可能に
			         bHit = TRUE;
#endif
				     break;
				   }
				 }
			   }
			}
#else
		    if (!stricmp(mSender, pTo)) {  // 許可ユーザーとする
#ifdef UPDATE_20070610 // 拒否指定オプションの追加
              if (*pOption != -1L)
#endif
	            bsts = TRUE;
#ifdef UPDATE_20060711 // 全体の送信先制限も可能に
			  bHit = TRUE;
#endif
		      break;
			} else if ((pwild = strchr(mSender, '*'))) { // ワイルドカードでのマスク
 		      *pwild = '\x0';
		      pwild++;
		      n = strlen(pTo)-strlen(pwild);
		      if (n >= 0) {
		        if (!strnicmp(pTo, mSender, strlen(mSender)) && // 前半部
			        !stricmp(&pTo[n], pwild)) { // 後半部が一致
#ifdef UPDATE_20070610 // 拒否指定オプションの追加
                  if (*pOption != -1L)
#endif
	                bsts = TRUE;
#ifdef UPDATE_20060711 // 全体の送信先制限も可能に
			      bHit = TRUE;
#endif
		          break;
				}
			  }
			}
#endif
		  }
          //p = fgets(mSender, sizeof(mSender), fp);
#ifdef UPDATE_20161113 /// 対象アドレス欄にファイル指定を可能にする機能の追加
		} while(fps && fgets(mSender, sizeof(mSender)-1, fps));
		if (fps)
		{
		  fclose(fps);
		}
        if (bHit)
		  break;
#endif
		}
	    fclose(fp);
#ifdef UPDATE_20161111 /// 外部からの送信元送信先制限(逆上長承認)
        if (bDebug) printf(" -> found %s permit file (%s)\n",  (bType ? "Sender" : "Reciver") , Path);
#else
        if (bDebug) printf(" -> found Sender permit file (%s)\n", Path);
#endif
	  } else {
#ifdef UPDATE_20250620 // 送信元ドメイン単位での送信先制限も可能に
		if (!bFound)
#endif
	    bsts = TRUE;
#ifdef UPDATE_20161111 /// 外部からの送信元送信先制限(逆上長承認)
        if (bDebug) printf(" -> not found %s permit file (%s)\n",  (bType ? "Sender" : "Reciver") , Path);
#else
        if (bDebug) printf(" -> not found Sender permit file  (%s)\n", Path);
#endif
	  }
#ifdef UPDATE_20060711 // 全体の送信先制限も可能に
      if (bHit)
		break;
#endif
    }
    return (bData ? bsts : TRUE); // テーブルが空なら条件無効
}
#endif
#endif

void GetDomainFromIP(char *myaddr, char *mydomain, DWORD nSize) {
  HKEY   hKeyRoot = HKEY_LOCAL_MACHINE;
  HKEY   hKey;
  CHAR   mkey[256], mIP[256];
  CHAR   mName[_MAX_PATH];	     // address of buffer for subkey name 
  DWORD  cbValueName = MAX_VALUE_NAME;
  DWORD  retCode;
  DWORD  dwIndex = 0;	         // index of subkey to enumerate 
  DWORD  lpcbName = 0;
  DWORD  dValueName;
  DWORD  dwType;
  DWORD  dSize;
  int    i = 0;
#ifdef REGTOFILE
  HANDLE hFile;
#endif

  // READ THE KEY DATA.

  memset(mydomain, 0, nSize); // クリアしておかないとＮＧ
  sprintf(mkey,PROFILE_ROOT_TREE, DOMAIN_SMTPIP);

   do {
	if (i > 0)
	  _sleep(0);
#ifdef REGTOFILE
	 if (nClustering && !strnicmp(mkey, "software\\emwac", 14)) {
       retCode = 
         OpenKeyFile(mMailSpoolDir,
		             mkey,
					 "",
					 &hFile);

	 } else
#endif
	 {
    retCode = 
      RegOpenKeyEx(hKeyRoot,       // Key handle at root level.
                   (LPCTSTR)mkey,  // Path name of child key.
                   0,              // Reserved.
                   KEY_READ,       // Requesting read access.
                   &hKey);         // Address of key to be returned.
	 }
  } while(retCode == ERROR_INVALID_HANDLE && i++ < 3);  // ERROR_INVALID_HANDLE = 6

  if (retCode == ERROR_SUCCESS) {
     do {
	   dwType = REG_SZ;
	   dValueName = sizeof(mName);
	   dSize = nSize;
#ifdef REGTOFILE
	 if (nClustering && !strnicmp(mkey, "software\\emwac", 14)) {
       retCode =
		 KeyFileEnumValue(mMailSpoolDir,
		                  mkey,
						  dwIndex,
						  (char *)&mName,
						  dwType,
						  (LPBYTE)mIP,
						  &dSize);

	 } else
#endif
	 {
       retCode = RegEnumValue(hKey,             // 問い合わせ対象のキーのハンドル
                              dwIndex,         // 取得するべきレジストリエントリのインデックス番号
                              (LPTSTR)mName,    // レジストリエントリ名が格納されるバッファ
                              &dValueName,  // レジストリエントリ名バッファのサイズ
                              NULL,    // 予約済み
                              &dwType,        // レジストリエントリのデータのタイプ
                              (LPBYTE)mIP,         // レジストリエントリのデータが格納されるバッファ
                              &dSize       // データバッファのサイズ
                              );

	 }
        if (retCode == ERROR_SUCCESS)
#ifdef CLUSTERING
		  if (strstr(mIP, myaddr))
#else
		  if (strcmp(myaddr, mIP) == 0)
#endif
		  {
            strcpy(mydomain, mName);
			break;
		  }
        dwIndex++;
	 } while (retCode == ERROR_SUCCESS);
  }
#ifdef REGTOFILE
  if (nClustering && !strnicmp(mkey, "software\\emwac", 14))
	CloseHandle(hFile);
  else
#endif
  RegCloseKey (hKey);
}

#ifdef UPDATE_20241209 // 外部アカウントでのXOAUTH2での認証方式用にユーザ別認証コード保管ファイル(oauth2.dat)を各アカウントフォルダに設定可能にした
char * GetXOAUTH2Path(char *pDir, char *pUser, char *pFn)
{
  char c, *p, *q;
  char Path[MAX_PATH];

    if (p = strchr(pUser, '@')) // ドメインの区切り
	{
	   *p = '\x0';
#ifdef WINDOWS
       sprintf(Path, "%soauth2\\%s\\%s\\%s", pDir, p+1, pUser, pFn);
#else
       sprintf(Path, "%soauth2/%s/%s/%s", pDir, p+1, pUser);
#endif
	   *p = '@';
	} else {
#ifdef WINDOWS
       sprintf(Path, "%soauth2\\%s\\%s", pDir, pUser, pFn);
#else
       sprintf(Path, "%soauth2/%s/%s", pDir, pUser);
#endif
	}
	if (p = strstr(Path, "oauth2"))
	{
	  while ((q = strchr(p, '\\')) || (q = strchr(p, '/')) )
	  {
		 c = *q;
		 *q = '\x0';
		 _mkdir(Path);
		 *q = c;
		 p = q+1;
	  }
	}
	return Path;
}
#endif

void GetBaseDirectory(CHAR *BaseDir, char *mMBdir, char *muser, char *myaddr) {
  char mTempDir[MAX_PATH];
  char mMBoxDir[MAX_PATH];
#ifdef V3
#ifdef UPDATE_20071205 //RFC2821: メールアドレス長が255Byteのときの対策
  char mduser[512];
  char mydomain[512];
#else
  char mduser[256];
  char mydomain[256];
#endif
    if (!strstr(muser, "@"))  // ドメイン指定をしていない場合。
	  GetDomainFromIP(myaddr, mydomain, sizeof(mydomain));
#endif
	mTempDir[0] = '\x0';
	strcpy(mMBoxDir, mMBdir);
#ifdef V3
	memset(mduser, 0, sizeof(mduser)); // ０クリア必須：削除するとユーザーチェックでハングする
    if (!strstr(muser, "@"))
	  sprintf(mduser,"%s@%s", muser, mydomain);
	else
	  strcpy(mduser, muser);
	GetMailBoxFolder(mduser, BaseDir);
#else
	strtok(muser, "@");
	GetMailBoxFolder(muser, BaseDir);
#endif
}

/*
BOOL GetLockFile(char  *pUser, char *myaddr) {
  CHAR   mBase[MAX_PATH], mFile[MAX_PATH];
  HANDLE          hFindFile;
  WIN32_FIND_DATA FindFileData ;
  BOOL   bsts = FALSE;

    GetBaseDirectory(mBase, mMailBoxDir, pUser, myaddr);
	strtok(mBase, "@");
    sprintf(mFile, "%s%s", mBase, POP3_LOCKFILE);
    hFindFile = FindFirstFile( mFile, &FindFileData);
    if (hFindFile != INVALID_HANDLE_VALUE) {
	  bsts = TRUE;
      FindClose( hFindFile ); 
	}
    if (bDebug) printf("%s %s.\n", mBase, bsts ? "True" : "False");
    return bsts;
}
*/

#ifdef UPDATE_20050927 // SMTP AUTH のユーザＩＤ情報の格納 個人別対応
BOOL GetPasswordFile(char  *pDir, char *pUser, char *mpass, DWORD *nFSL)
#else
BOOL GetPasswordFile(char  *pDir, char *pUser, char *mpass)
#endif
{
  FILE   *fp;
#ifdef UPDATE_20071205 //RFC2821: メールアドレス長が255Byteのときの対策
  CHAR   Path[512];
#else
  CHAR   Path[MAX_PATH];
#endif
  BOOL   bsts = FALSE;
  char   mPwd[256];
#ifdef UPDATE_20050927 // SMTP AUTH のユーザＩＤ情報の格納 個人別対応
  char   mLevel[16];
#endif
    //sprintf(Path, "%sapop.dat", pDir);
    sprintf(Path, "%s%s", pDir, mPasswordFile);
    if (bDebug) printf("Authentication file name = %s\n", Path);
    fp = fopen(Path, "rt");
	if (fp) {
      fgets(mpass, _MAX_PATH, fp);
	  strtok(mpass, "\n");
#ifdef UPDATE_20050927 // SMTP AUTH のユーザＩＤ情報の格納 個人別対応
	  mLevel[0] = '\x0'; 
      fgets(mLevel, 16, fp);
	  mLevel[1] = '\x0';
	  if (mLevel[0] == '\n' || mLevel[0] == '\r')
		mLevel[0] = '\x0';
	  *nFSL = (mLevel[0] ? atoi(mLevel) : nFROMSecLevel);
#endif
	  fclose(fp);
      SPA_Decode(mpass, mPwd); // 2002.08.30
	  strcpy(mpass, mPwd);     // 2002.08.30
	  bsts = TRUE;
      if (bDebug) printf(" -> found Authentication file name (mailbox)\n");
	} else {
	  mpass[0] = '\x0';
#ifdef UPDATE_20061118 // フォルダで使用不可のキャラクタ
      if (BadCharcter(pUser))
#endif
	  {
        GetListsPass(pUser, mpass);
	    if (*mpass != '\x0') {
	      bsts = TRUE;
          if (bDebug) printf(" -> found Authentication file name (ML Password)\n");
		} else {
          if (bDebug) printf(" -> not found Authentication file name\n");
		}
	  } 
#ifdef UPDATE_20061118 // フォルダで使用不可のキャラクタ
	  else {
        if (bDebug) printf(" -> not found Authentication file name\n");
	  }
#endif
	}
    return bsts;
}

BOOL CheckDomain(char *mID) {
  char *pldom, *pid;
  BOOL sts = FALSE; // Global

  pldom = mLocalDomain;
  pid  = strstr(mID, "@");
  if (pid) {
	pid++;
    while (strlen(pldom)){
      //printf(" \"%s\"", pldom);
	  if (stricmp(pldom, pid) == 0) {
		 sts = TRUE; //Local
		 break;
	  }
      pldom += strlen(pldom);
      pldom++;
	};
  } else
	sts = TRUE;  //Local
  return sts;
}

void GetPersonInfo(char *user, CHAR *mFileName, CHAR *mPersonFile) { 
 CHAR mTempDir[_MAX_PATH];
 CHAR *pdom;
 //INT  nl;
 BOOL bDom;

  strcpy(mTempDir, mMailBoxDir);
  ///// LocalDomain 以外も設定可能とする
  bDom = CheckDomain(user);
#ifndef V3
    if (bDom) {
      pdom = strstr(user, "@");
      if (pdom)
        *pdom = '\x0';
	}
#else
    if (bDom) {
      pdom = strstr(user, "@");
	}
	/////////////////////////////////////////////////
	// 選択ドメイン別に保存先メールボックスを代える為
	// ドメインを加工しないままフォルダー位置取得
	/////////////////////////////////////////////////
#endif
  /////////////////////////////////////
  mPersonFile[0] = '\x0';
  GetMailBoxFolder(user, mPersonFile); // 2000.06.17 %HOME%が使えるように
  strcat(mPersonFile, mFileName);
  ///// LocalDomain 以外も設定可能とする
  if (bDom) {
    if (pdom)
      *pdom = '@';
  }
  /////////////////////////////////////
}

/* //ユーザ別の１メール制限は同報があったら行えない
#ifdef V4
DWORD GetMailInLimit(char *pMailBox) {
 CHAR mLimit[256];
 FILE *fp;
 DWORD nLimit = 0; // デフォルト無限
   sprintf(mLimit, "%spsize.ctl", pMailBox);
   fp = fopen(mLimit, "rt");
   if (fp) {
	 fscanf(fp, "%lu", &nLimit);
	 fclose(fp);
   }
   return nLimit;
}

DWORD GetMailInMaxSize(char *user, BOOL bLocal, BOOL bSubLocal) {
  CHAR mMBoxDir[_MAX_PATH];
  CHAR mUser[_MAX_PATH];
  BOOL bBox = TRUE;
  DWORD nSize = 0;

  //return (DWORD)0;
  if (bLocal && !bSubLocal) {
	strcpy(mUser, user);
	mMBoxDir[0] = '\x0';
    GetMailBoxFolder(mUser, mMBoxDir);  // 2000.06.17 %HOME%が使えるように
	nSize = GetMailInLimit(mMBoxDir); // 個別の１メールあたり制限
	if (nSize == 0)
      nSize = nMailInMaxSize;
  }
  return nSize;
}
#endif
*/

#ifdef UPDATE_20051121 // メール連続受領に対する配送キュー調整
void IncomingForFsSync(void) {
  CHAR mFileGroup[_MAX_PATH];
  BOOL bFile;
  HANDLE          hFindFile;
  WIN32_FIND_DATA FindFileData ;
  BOOL bBox = TRUE;
  DWORD n;

  if (nSpoolFsSync == 0)
	return;

  sprintf(mFileGroup, "%s%s*.rcp",mMailSpoolDir,mMailQueueDir);
  do {
	n = 0;
    hFindFile = FindFirstFile( mFileGroup, &FindFileData);
    if (hFindFile != INVALID_HANDLE_VALUE) {
	  bFile = TRUE;
      while (bFile) {
       if (bServiceTerminating)
         break;
        n++; // フォルダに溜まったファイル総数
        bFile = FindNextFile( hFindFile, &FindFileData);
	  }
      FindClose( hFindFile ); 
	  //SendPipeLine("Go"); // spadsへ通知
	}
	SendPipeLine("Go"); // spadsへ通知
    if (bServiceTerminating)
      break;
	if (nSpoolFsSync < n) // フォルダにファイル溜まり過ぎた
	  _sleep(n);
  } while (nSpoolFsSync < n);
}
#endif

#ifdef UPDATE_20090625 // IMAP4使用対策（サブフォルダのファイルサイズのチェック）
#ifdef UPDATE_ULONGLONG //64bit長のデータで取得可能にする
void GetSubMailBoxSize(CHAR *pDir, ULONGLONG *pnSize, ULONGLONG nBoxSize)
#else
void GetSubMailBoxSize(CHAR *pDir, DWORD *pnSize, DWORD nBoxSize)
#endif
{
  BOOL bFile;
  HANDLE          hF;
  WIN32_FIND_DATA FD;
  //DWORD nSize = 0;
  CHAR  mDir[_MAX_PATH], mSubDir[_MAX_PATH];

  sprintf(mDir, "%s*.msg", pDir);
  hF = FindFirstFile( mDir, &FD);
  if (hF != INVALID_HANDLE_VALUE) 
  {
 	 bFile = TRUE;
     while (bFile)
	 {
#ifdef UPDATE_ULONGLONG //64bit長のデータで取得可能にする
       ULARGE_INTEGER u;
	   u.LowPart = FD.nFileSizeLow;
       u.HighPart = FD.nFileSizeHigh;
	   *pnSize += (ULONGLONG)u.QuadPart;
#else
	   *pnSize += FD.nFileSizeLow;
#endif
       if (nBoxSize < *pnSize)
	   {
	     break;
	   }
       bFile = FindNextFile( hF, &FD);
	 }; 
     FindClose( hF ); 
  }
  // 深い階層の確認
  if (nBoxSize > *pnSize)
  {
    sprintf(mDir, "%s*", pDir);
    hF = FindFirstFile( mDir, &FD);
    if (hF != INVALID_HANDLE_VALUE) 
	{
 	  bFile = TRUE;
      while (bFile)
	  {
	    if (strcmp(FD.cFileName, ".") && strcmp(FD.cFileName, "..") )
		{
		  if (FD.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		  {
            sprintf(mSubDir, "%s%s\\", pDir, FD.cFileName);
            GetSubMailBoxSize(mSubDir, pnSize, nBoxSize);
            if (nBoxSize < *pnSize)
			{
		 	  break;
			}
		  }
		}
        bFile = FindNextFile( hF, &FD);
	  }; 
      FindClose( hF ); 
	}
  }
}
#endif

BOOL GetMailBoxSize(char *user, BOOL bLocal, BOOL bSubLocal) 
{
  CHAR mMBoxDir[_MAX_PATH], mFileGroup[_MAX_PATH];
  CHAR mTempDir[_MAX_PATH], mUser[_MAX_PATH];
  CHAR *pdom;
  //INT  nl;
  BOOL bFile;
  HANDLE          hFindFile;
  WIN32_FIND_DATA FindFileData ;
  BOOL bBox = TRUE;
#ifdef UPDATE_ULONGLONG //64bit長のデータで取得可能にする
  ULONGLONG nSize = 0, nBoxSize;
#else
  DWORD nSize = 0, nBoxSize;
#endif
#ifdef UPDATE_20090625 // IMAP4使用対策（サブフォルダのファイルサイズのチェック）
  CHAR mSubDir[_MAX_PATH];
#endif
  //return (DWORD)0;
  if (bLocal && !bSubLocal) {
	strcpy(mUser, user);
	mMBoxDir[0] = '\x0';
    GetMailBoxFolder(mUser, mMBoxDir);  // 2000.06.17 %HOME%が使えるように
	nBoxSize = GetMailBoxLimit(mMBoxDir); // 個別のメールボックス制限
	if (nBoxSize == 0)
	{
#ifdef UPDATE_ULONGLONG //64bit長のデータで取得可能にする
      ULARGE_INTEGER u;
	  u.LowPart = nMailInBoxSize;
      u.HighPart = nMailInBoxSizeHigh;
      nBoxSize = u.QuadPart;
#else
      nBoxSize = nMailInBoxSize;
#endif
	}
    if (nBoxSize != 0) {
#ifdef UPDATE_ULONGLONG //64bit長のデータで取得可能にする
      if (bDebug) printf("Start GetMailBoxSize = %I64u byte.\n", nBoxSize);
#else
      if (bDebug) printf("Start GetMailBoxSize = %ld byte.\n", nBoxSize);
#endif
	  strcpy(mTempDir, mMailBoxDir);
#ifndef V3
      pdom = strstr(user, "@");
      if (pdom)
        *pdom = '\x0';
#else
      pdom = strstr(user, "@");
	/////////////////////////////////////////////////
	// 選択ドメイン別に保存先メールボックスを代える為
	// ドメインを加工しないままフォルダー位置取得
	/////////////////////////////////////////////////
#endif
      //mMBoxDir[0] = '\x0';
	  //GetMailBoxFolder(user, mMBoxDir);  // 2000.06.17 %HOME%が使えるように
      sprintf(mFileGroup, "%s*.msg", mMBoxDir);
      hFindFile = FindFirstFile( mFileGroup, &FindFileData);
      if (hFindFile != INVALID_HANDLE_VALUE) {
		bFile = TRUE;
        while (bFile) {
#ifdef UPDATE_ULONGLONG //64bit長のデータで取得可能にする
          ULARGE_INTEGER u;
	      u.LowPart = FindFileData.nFileSizeLow;
          u.HighPart = FindFileData.nFileSizeHigh;
		  nSize += (ULONGLONG)u.QuadPart;
#else
	      nSize += FindFileData.nFileSizeLow;
#endif
#ifdef UPDATE_20090625 // IMAP4使用対策（サブフォルダのファイルサイズのチェック）
          if (nBoxSize < nSize)
		  {
		    break;
		  }
#endif
          bFile = FindNextFile( hFindFile, &FindFileData);
		}; 
        FindClose( hFindFile ); 
	  }
#ifdef UPDATE_20090625 // IMAP4使用対策（サブフォルダのファイルサイズのチェック）
      // 深い階層の確認
      if (nBoxSize > nSize)
	  {
        sprintf(mFileGroup, "%s*", mMBoxDir);
        hFindFile = FindFirstFile( mFileGroup, &FindFileData);
        if (hFindFile != INVALID_HANDLE_VALUE) 
		{
 	      bFile = TRUE;
          while (bFile)
		  {
	        if (strcmp(FindFileData.cFileName, ".") && strcmp(FindFileData.cFileName, "..") )
			{
		      if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			  {
                sprintf(mSubDir, "%s%s\\", mMBoxDir, FindFileData.cFileName);
                GetSubMailBoxSize(mSubDir, &nSize, nBoxSize);
                if (nBoxSize < nSize)
				{
				  break;
				}
			  }
			}
            bFile = FindNextFile( hFindFile, &FindFileData);
		  }; 
          FindClose( hFindFile ); 
		}
	  }
#endif
      if (pdom)
	  {
        *pdom = '@';
	  }
      if (nBoxSize < nSize)
	  {
        bBox = FALSE;
	  }
	}
  }
  return bBox;
}

BOOL MLUserCheck(char *mFrom, char *mRcpt) {
  HKEY hKeyRoot = HKEY_LOCAL_MACHINE;
  HKEY   hKey;
  CHAR  mkey[MAX_REG_RCPT];
  BOOL  sts = FALSE;
  DWORD  cbValueName = MAX_VALUE_NAME;
  DWORD  retCode;
  DWORD      dwIndex = 0;	         // index of subkey to enumerate 
  CHAR       mName[_MAX_PATH];	     // address of buffer for subkey name 
  DWORD      lpcbName = 0;
  int    i = 0;
#ifdef REGTOFILE
  HANDLE hFile;
#endif
#ifdef UPDATE_20070521 // 予約語対策
  char *q, mUID[512], mKEY[512];
#endif
#ifdef UPDATE_20071210A // ML参加アドレスを含む256Byte超のフォルダ名になったと場合の対策
  FILE *fp;
  char mMLFn[256];
#endif

   //sprintf(mkey,"SOFTWARE\\EMWAC\\IMS\\Lists\\%s", mRcpt);
   sprintf(mkey,"%s\\%s", SOFT_LISTS_REG, mRcpt);
#ifdef UPDATE_20070521 // 予約語対策
   strcpy(mUID, mRcpt);
   strtok(mUID, "@");
   if (ReservedWords(mUID)) {
     if ((q = strstr(mRcpt, mUID))) {
       strcpy(mUID, mReservedWords);
       strcat(mUID, q);
       sprintf(mkey,"%s\\%s", SOFT_LISTS_REG, mUID);
	 }
   }
#endif
#ifdef MULTI_ML
   if (!QueryMLists((LPCTSTR)SOFT_LISTS_REG, mRcpt)) // ドメイン付MLが無い場合
     strtok(mkey,"@");
#else
   strtok(mkey,"@");
#endif
   strcat(mkey,"\\Members");
  // OPEN THE KEY.
   do {
	 if (i > 0)
	  _sleep(0);
#ifdef REGTOFILE
	 if (nClustering && !strnicmp(mkey, "software\\emwac", 14)) {
	   strcat(mkey, "\\");
       retCode = 
         OpenKeyFile(mMailSpoolDir,
		             mkey,
					 "",
					 &hFile);

	 } else {
#endif
     retCode = RegOpenKeyEx(hKeyRoot,    // Key handle at root level.
                          (LPCTSTR)mkey,     // Path name of child key.
                          0,           // Reserved.
                          KEY_READ, // Requesting read access.
                          &hKey);      // Address of key to be returned.
#ifdef REGTOFILE
	 }
#endif
  } while(retCode == ERROR_INVALID_HANDLE && i++ < 3);  // ERROR_INVALID_HANDLE = 6

   if (retCode == ERROR_SUCCESS) {
     // READ THE KEY DATA.
#ifdef UPDATE_20071210A // ML参加アドレスを含む256Byte超のフォルダ名になったと場合の対策
     sprintf(mMLFn, "%sreg\\%sext.dat", mMailSpoolDir, mkey);
	 fp = fopen(mMLFn, "rt");
#endif
     do {
#ifdef REGTOFILE
	 if (nClustering && !strnicmp(mkey, "software\\emwac", 14)) {
       retCode =
		 KeyFileEnumKey(mMailSpoolDir,
		                mkey,
						dwIndex, 
						(LPTSTR)mName, 
						(unsigned long *)&lpcbName);
#ifdef UPDATE_20071210A // ML参加アドレスを含む256Byte超のフォルダ名になったと場合の対策
	  if (fp && retCode != ERROR_SUCCESS) {
		mName[0] = '\x0';
		fgets(mName, sizeof(mName), fp);
		if (mName[0] && mName[0] != '\r' && mName[0] != '\n') {
		  strtok(mName, "\r\n");
		  retCode = ERROR_SUCCESS;
		}
	  }
#endif
#ifdef UPDATE_20070521 // 予約語対策
		if (!strnicmp(mName, mReservedWords, strlen(mReservedWords))) {
          strcpy(mKEY, &mName[strlen(mReservedWords)]);
		  strcpy(mName, mKEY);
		}
#endif
	 } else {
#endif
       retCode =
        RegEnumKey(hKey,	        // handle of key to query 
                   dwIndex,	    // index of subkey to query 
                   (LPTSTR)mName, // address of buffer for subkey name  
                   (unsigned long)&lpcbName 	    // size of subkey buffer 
        );
#ifdef REGTOFILE
	 }
#endif
        if (retCode == ERROR_SUCCESS)
		  if (_stricmp(mName, mFrom) == 0) {
            sts = TRUE;
			break;
		  }
        dwIndex++;
	 } while (retCode == ERROR_SUCCESS);
#ifdef UPDATE_20071210A // ML参加アドレスを含む256Byte超のフォルダ名になったと場合の対策
	 if (fp) {
       fclose(fp);
	 }
#endif
   }
   //RegFlushKey(hKey);
#ifdef REGTOFILE
  if (nClustering && !strnicmp(mkey, "software\\emwac", 14))
	CloseHandle(hFile);
  else
#endif
   RegCloseKey (hKey);
   return sts;
}

BOOL ListsReplyCheck(char *mRcpt) {
#ifdef UPDATE_20071205 //RFC2821: メールアドレス長が255Byteのときの対策
   CHAR  mkey[MAX_REG_RCPT], mListRcpt[MAX_REG_RCPT], *pdom;
#else
   CHAR  mkey[256], mListRcpt[256], *pdom;
#endif
   BOOL bsts = FALSE, bAliases = FALSE;
#ifdef UPDATE_20070521 // 予約語対策
  char *q;
#endif

#ifdef UPDATE_20071205 //RFC2821: メールアドレス長が255Byteのときの対策
   strncpy(mListRcpt, mRcpt, 255);
   mListRcpt[255] = '\x0';
   pdom = strstr(mListRcpt, "@");
#else
   strncpy(mListRcpt, mRcpt, 255);
   mListRcpt[255] = '\x0';
   pdom = strstr(mListRcpt, "@");
#endif
   if (pdom)
	 *pdom = '\x0';
#ifdef UPDATE_20070509 // エイリアス設定の実アドレスオプションを別データとして処理する
   bAliases = GetAliases((LPCTSTR)SOFT_ALIASES_REG, (char *)mListRcpt, (char *)pdom, NULL);
#else
   bAliases = GetAliases((LPCTSTR)SOFT_ALIASES_REG, (char *)mListRcpt, (char *)pdom);
#endif
   if (pdom)
	   *pdom = '@';
   if (!bAliases) {
     sprintf(mkey,"%s\\%s", SOFT_LISTS_REG, mListRcpt);
#ifdef UPDATE_20070521 // 予約語対策
     strcpy(mListRcpt, mRcpt);
     strtok(mListRcpt, "@");
     if (ReservedWords(mListRcpt)) {
       if ((q = strstr(mRcpt, mListRcpt))) {
         strcpy(mListRcpt, mReservedWords);
         strcat(mListRcpt, q);
         sprintf(mkey,"%s\\%s", SOFT_LISTS_REG, mListRcpt);
	   }
	 }
#endif
#ifdef MULTI_ML
     if (!QueryMLists((LPCTSTR)SOFT_LISTS_REG, mRcpt)) // ドメイン付MLが無い場合
       strtok(mkey,"@");
#else
     strtok(mkey,"@");
#endif
	 bsts = GetProfileIntEx(mkey, "ReplyTo", (INT)TRUE);
   }
   return bsts;
}

BOOL ListsCheck(char *mFrom, char *mRcpt, char *mMlTkn, char *mMlWord, DWORD *nMLNo, BOOL bCount) {
#ifdef UPDATE_20090818 // 付加表題のデータサイズを256Byteまで有効にする対策
   CHAR  mkey[MAX_REG_RCPT], mtkn[257], mModerator[_MAX_PATH], *p;
#else
#ifdef UPDATE_20071205 //RFC2821: メールアドレス長が255Byteのときの対策
   CHAR  mkey[MAX_REG_RCPT], mtkn[64], mModerator[_MAX_PATH], *p;
#else
   CHAR  mkey[256], mtkn[64], mModerator[_MAX_PATH], *p;
#endif
#endif
   int   i, nketa;
   char  *pdom;
   DWORD nWhoCanSend, nMlId, nMax;
   BOOL  bsts = FALSE, bAliases = FALSE;
#ifdef CLUSTERING
   HANDLE hF;
#endif
#ifdef UPDATE_20060627 // メーリングリストアドレス判断のリセット
   CHAR  mAlfrom[256];
#endif
#ifdef UPDATE_20070521 // 予約語対策
  char *q;
#endif
#ifdef UPDATE_20041208
#ifdef UPDATE_20071205 //RFC2821: メールアドレス長が255Byteのときの対策
   CHAR  mListRcpt[512];

   strncpy(mListRcpt, mRcpt, 255);
   mListRcpt[255] = '\x0';
   pdom = strstr(mListRcpt, "@");
#else
   CHAR  mListRcpt[256];

   strncpy(mListRcpt, mRcpt, 255);
   mListRcpt[255] = '\x0';
   pdom = strstr(mListRcpt, "@");
#endif
#else
   pdom = strstr(mRcpt, "@");
#endif
   if (pdom)
	 *pdom = '\x0';
   //bAliases = GetAliases((LPCTSTR)"SOFTWARE\\EMWAC\\IMS\\Aliases", (char *)mRcpt, (char *)pdom);
#ifdef UPDATE_20041208
#ifdef UPDATE_20070509 // エイリアス設定の実アドレスオプションを別データとして処理する
   bAliases = GetAliases((LPCTSTR)SOFT_ALIASES_REG, (char *)mListRcpt, (char *)pdom, NULL);
#else
   bAliases = GetAliases((LPCTSTR)SOFT_ALIASES_REG, (char *)mListRcpt, (char *)pdom);
#endif
#else
#ifdef UPDATE_20070509 // エイリアス設定の実アドレスオプションを別データとして処理する
   bAliases = GetAliases((LPCTSTR)SOFT_ALIASES_REG, (char *)mRcpt, (char *)pdom, NULL);
#else
   bAliases = GetAliases((LPCTSTR)SOFT_ALIASES_REG, (char *)mRcpt, (char *)pdom);
#endif
#endif
   if (pdom)
	 *pdom = '@';
   if (!bAliases) {
     //sprintf(mkey,"SOFTWARE\\EMWAC\\IMS\\Lists\\%s", mRcpt);
#ifdef UPDATE_20041208
     sprintf(mkey,"%s\\%s", SOFT_LISTS_REG, mListRcpt);
#ifdef UPDATE_20070521 // 予約語対策
     strcpy(mListRcpt, mRcpt);
     strtok(mListRcpt, "@");
     if (ReservedWords(mListRcpt)) {
       if ((q = strstr(mRcpt, mListRcpt))) {
         strcpy(mListRcpt, mReservedWords);
         strcat(mListRcpt, q);
         sprintf(mkey,"%s\\%s", SOFT_LISTS_REG, mListRcpt);
	   }
	 }
#endif
#else
     sprintf(mkey,"%s\\%s", SOFT_LISTS_REG, mRcpt);
#endif
#ifdef MULTI_ML
     if (!QueryMLists((LPCTSTR)SOFT_LISTS_REG, mRcpt)) // ドメイン付MLが無い場合
       strtok(mkey,"@");
#else
     strtok(mkey,"@");
#endif
     ////////////////////////////////////////////////////////////
     // WhoCanSend 0x1 = Member, 0x2 = Anyone, 0x3 = Moderator //
     ////////////////////////////////////////////////////////////
#ifdef UPDATE_20060627 // メーリングリストアドレス判断のリセット
   strcpy(mAlfrom, mFrom);
   if ((pdom = strstr(mAlfrom, "@")))
	 *pdom = '\x0';
#ifdef UPDATE_20070509 // エイリアス設定の実アドレスオプションを別データとして処理する
   GetAliases((LPCTSTR)SOFT_ALIASES_REG, (char *)mAlfrom, (char *)pdom, NULL);
#else
   GetAliases((LPCTSTR)SOFT_ALIASES_REG, (char *)mAlfrom, (char *)pdom);
#endif
#endif
     nWhoCanSend = GetProfileIntEx(mkey, "WhoCanSend", (INT)-1);
     switch(nWhoCanSend) {
#ifdef UPDATE_20060627 // メーリングリストアドレス判断のリセット
	    case 1: if (MLUserCheck(mFrom, mRcpt) || MLUserCheck(mAlfrom, mRcpt))
#else
	    case 1: if (MLUserCheck(mFrom, mRcpt))
#endif
	 			  bsts = TRUE;
		        break;
	    case 2: bsts = TRUE;
		        break;
	    case 3: GetProfileStringEx(mkey,"Moderator", "", mModerator, sizeof(mModerator));
#ifdef UPDATE_20060627 // メーリングリストアドレス判断のリセット
		        if (_stricmp(mFrom, mModerator) == 0 || _stricmp(mAlfrom, mModerator) == 0)
#else
		        if (_stricmp(mFrom, mModerator) == 0)
#endif
				  bsts = TRUE;
		       break;
	 }
     if (bsts && bCount) {
#ifdef CLUSTERING
	   hF = NULL;
       OpenKeyFile(mMailSpoolDir, mkey, "MlId_RW", &hF);
#endif
       nMlId = GetProfileIntEx(mkey, "MlId", (INT)0) + 1;
	   if (nMLNo)
		 *nMLNo = (DWORD) nMlId;
#ifdef UPDATE_20090818 // 付加表題のデータサイズを256Byteまで有効にする対策
	   GetProfileStringEx(mkey,"MlToken", "", mtkn, sizeof(mtkn)-1);
#else
	   GetProfileStringEx(mkey,"MlToken", "", mtkn, sizeof(mtkn));
#endif
	   if (mMlWord) // NULL ポインタでなければ
	     strcpy(mMlWord, mtkn);
	   nketa = 4; // デフォルト４桁
       p = strstr(mtkn, "%");
	   if (p) {
         nketa = atoi(p+1);
	   }
	   nMax = 1;
	   for (i = 1; i <= nketa; i++)
	     nMax *= 10;
       if (nMlId >= nMax)
	     nMlId = 0;
	   sprintf(mMlTkn, mtkn, nMlId);
	   WriteProfileIntEx(mkey, "MlId", (DWORD)nMlId);
#ifdef CLUSTERING
	   if (hF)
         CloseHandle(hF);
#endif

	 }
   }
   return bsts;
}

#ifdef UPADTE_20031120
BOOL ListsDeletesAttachment(char *mRcpt) {
   CHAR  mkey[MAX_REG_RCPT];
   BOOL  bDelAttach = FALSE;
#ifdef UPDATE_20070521 // 予約語対策
  char *q, mUID[512];
#endif

  sprintf(mkey,"%s\\%s", SOFT_LISTS_REG, mRcpt);
#ifdef UPDATE_20070521 // 予約語対策
  strcpy(mUID, mRcpt);
  strtok(mUID, "@");
  if (ReservedWords(mUID)) {
    if ((q = strstr(mRcpt, mUID))) {
      strcpy(mUID, mReservedWords);
      strcat(mUID, q);
      sprintf(mkey,"%s\\%s", SOFT_LISTS_REG, mUID);
	}
  }
#endif
#ifdef MULTI_ML
   if (!QueryMLists((LPCTSTR)SOFT_LISTS_REG, mRcpt)) // ドメイン付MLが無い場合
     strtok(mkey,"@");
#else
   strtok(mkey,"@");
#endif
   bDelAttach = GetProfileIntEx(mkey, "DeletesAttachment", (INT)FALSE);
   return bDelAttach;
}
#endif

#ifdef UPDATE_20071204  // メッセージＩＤ採番処理を修正(Bym10id)
BOOL ListsSave(char *mRcpt, char *mSubject, DWORD nMLNo, char *mID, char *mFolder)
#else
#ifdef UPDATE_20210304 // メーリングリストで投稿内容保存ファイルの原本を保存するオプションを追加した。
BOOL ListsSave(char *mRcpt, char *mSubject, DWORD nMLNo, DWORD mID, char *mFolder, BOOL *pbListSaveOrign)
#else
BOOL ListsSave(char *mRcpt, char *mSubject, DWORD nMLNo, DWORD mID, char *mFolder)
#endif
#endif
{
   CHAR  mkey[MAX_REG_RCPT], mdir[256], mIdx[256], tmp[256], mold[256], wk[256];
   BOOL  bListSave = FALSE, bhtml = FALSE;
#ifdef UPDATE_20210305 // メーリングリストで投稿内容保存ファイルのHTML形式のインデックスファイルの作成を禁止するオプション追加した。
   BOOL  bListSaveHTMLIndex = TRUE;
#endif
   FILE  *fp, *fpd;
#ifdef UPDATE_20091120 // 64bit長のファイルサイズに対応
   HANDLE hFindFile;
   WIN32_FIND_DATA FindFileData;
#else
   long   hFindFile;
   struct _finddata_t FindFileData;
#endif
   //CHAR   mFileGroup[_MAX_PATH];
#ifdef UPDATE_20070521 // 予約語対策
  char *q, mUID[512];
#endif
#ifdef UPDATE_20210303A // 保存する拡張子の指定
  char mExte[16];
#endif

   sprintf(mkey,"%s\\%s", SOFT_LISTS_REG, mRcpt);
#ifdef UPDATE_20070521 // 予約語対策
   strcpy(mUID, mRcpt);
   strtok(mUID, "@");
   if (ReservedWords(mUID)) {
     if ((q = strstr(mRcpt, mUID))) {
       strcpy(mUID, mReservedWords);
       strcat(mUID, q);
       sprintf(mkey,"%s\\%s", SOFT_LISTS_REG, mUID);
	 }
   }
#endif
#ifdef MULTI_ML
   if (!QueryMLists((LPCTSTR)SOFT_LISTS_REG, mRcpt)) // ドメイン付MLが無い場合
     strtok(mkey,"@");
#else
   strtok(mkey,"@");
#endif
#ifdef UPDATE_20091120 // ML保存時のindex.html作成のための排他処理、64bit長のファイルサイズに対応
   EnterCriticalSection(&g_csMLIndex);
#endif
   while (bUsedSaveList) { // 作成中ならウエイト
     if (bServiceTerminating)
	   break;
	 _sleep(100);
   }
   bUsedSaveList = TRUE;
   bListSave = GetProfileIntEx(mkey, "ListSave", (INT)FALSE);
   GetProfileStringEx(mkey,"ListSaveFolder", "", mdir, 240);
   if (bListSave && mdir[0] != '\x0') {
#ifdef UPDATE_20210303A // メーリングリストで投稿内容保存ファイルの拡張子の指定を可能にするオプションを追加した。
   GetProfileStringEx(mkey,"ListSaveExtension", "TXT", mExte, sizeof(mExte)-1);
   sprintf(mFolder, "%sB%010lu.%s", mdir, mID, mExte);
#else
#ifdef UPDATE_20071204  // メッセージＩＤ採番処理を修正(Bym10id)
     sprintf(mFolder, "%s%s.TXT", mdir, mID);
#else
     sprintf(mFolder, "%sB%010lu.TXT", mdir, mID);
#endif
#endif
#ifdef UPDATE_20210305 // メーリングリストで投稿内容保存ファイルのHTML形式のインデックスファイルの作成を禁止するオプション追加した。
	bListSaveHTMLIndex = GetProfileIntEx(mkey, "ListSaveHTMLIndex", (INT)TRUE);
    if (bListSaveHTMLIndex) 
    {
#endif

     sprintf(mIdx, "%sindex.html", mdir);
	 ///////////////////////////////
	 // index.htmlが100KByte 超えたら別ファイルに分離する
	 mold[0] = '\x0';
#ifdef UPDATE_20091120 // ML保存時のindex.html作成のための排他処理、64bit長のファイルサイズに対応
     hFindFile = FindFirstFile(mIdx, &FindFileData);
     if (hFindFile != INVALID_HANDLE_VALUE) 
	 {
       ULARGE_INTEGER u;
	   u.LowPart = FindFileData.nFileSizeLow;
       u.HighPart = FindFileData.nFileSizeHigh;
       FindClose( hFindFile ); 
	   if (u.QuadPart > (unsigned __int64) 102400)
	   { 
		 sprintf(mold, "index%lu.html", mdir, u.QuadPart);
		 sprintf(tmp, "%s%s", mdir, mold);
	     while(!DeleteFile(tmp))
		 {
           if (GetLastError() == ERROR_FILE_NOT_FOUND)
		   {
		     break;
		   }
		   _sleep(WAIT_TIME);
		 }
		 MoveFileEx(mIdx, tmp, MOVEFILE_COPY_ALLOWED | MOVEFILE_WRITE_THROUGH);
	     while(!(fp = fopen(tmp, "rt")))
		 {
           _sleep(WAIT_TIME);
		 }
		 fclose(fp);
	   }
	 }
#else
     hFindFile = _findfirst(mIdx, &FindFileData);
     if (hFindFile != -1L) {
	  if (FindFileData.size > 102400) { 
		 sprintf(mold, "index%lu.html", mdir, FindFileData.time_create);
		 sprintf(tmp, "%s%s", mdir, mold);
		 rename(mIdx, tmp);
	   }
       _findclose(hFindFile);
	 }
#endif
     ///////////////////////////////
     sprintf(tmp, "%sindex.tmp", mdir);
     strcpy(mkey, mRcpt);
     strtok(mkey,"@");
     ///////////////////////////////
	 fp = fopen(mIdx, "rt");
	 if (fp) {
	   fpd = fopen(tmp, "wt");
       if (fpd) {
		 do {
		   fgets(wk, 256, fp);
		   strtok(wk, "\r\n");
		   if (stricmp(wk, "</body>") == 0 || stricmp(wk, "</html>") == 0) {
			 bhtml = TRUE;
#ifdef UPDATE_20210303A // メーリングリストで投稿内容保存ファイルの拡張子の指定を可能にするオプションを追加した。
		     fprintf(fpd, "No.%010lu:<A HREF=\"B%010lu.%s\">%s</A><BR>\n", mID, mID, mExte, mSubject);
#else
#ifdef UPDATE_20071204  // メッセージＩＤ採番処理を修正(Bym10id)
		     fprintf(fpd, "No.%s:<A HREF=\"%s.TXT\">%s</A><BR>\n", mID, mID, mSubject);
#else
		     fprintf(fpd, "No.%010lu:<A HREF=\"B%010lu.TXT\">%s</A><BR>\n", mID, mID, mSubject);
#endif
#endif
			 break;
		   }
	       fprintf(fpd, "%s\n", wk);
		 } while(!feof(fp));
		 if (!bhtml) {
#ifdef UPDATE_20210303A // メーリングリストで投稿内容保存ファイルの拡張子の指定を可能にするオプションを追加した。
	       fprintf(fpd, "No.%010lu:<A HREF=\"B%010lu.%s\">%s</A><BR>\n", mID, mID, mExte, mSubject);
#else
#ifdef UPDATE_20071204  // メッセージＩＤ採番処理を修正(Bym10id)
	       fprintf(fpd, "No.%s:<A HREF=\"%s.TXT\">%s</A><BR>\n", mID, mID, mSubject);
#else
	       fprintf(fpd, "No.%010lu:<A HREF=\"B%010lu.TXT\">%s</A><BR>\n", mID, mID, mSubject);
#endif
#endif
		 }
		 fprintf(fpd, "</BODY>\n</HTML>\n");
	     fclose(fpd);
	   }
	   fclose(fp);
#ifdef UPDATE_20091120 // ML保存時のindex.html作成のための排他処理、64bit長のファイルサイズに対応
	   while(!(fp = fopen(tmp, "rt")))
	   {
		 _sleep(100);
	   }
	   fclose(fp);
	   while(!DeleteFile(mIdx))
	   {
         if (GetLastError() == ERROR_FILE_NOT_FOUND)
		 {
		   break;
		 }
		 _sleep(WAIT_TIME);
	   }
#endif
	   CopyFile(tmp, mIdx, FALSE);
#ifdef UPDATE_20091120 // ML保存時のindex.html作成のための排他処理、64bit長のファイルサイズに対応
	   while(!(fp = fopen(mIdx, "rt")))
	   {
		 CopyFile(tmp, mIdx, FALSE);
		 _sleep(100);
	   }
	   fclose(fp);
#endif
#ifndef TUNING1
	   DeleteFile(tmp);
#else
	   _unlink(tmp);
#endif
	 } else {
	   fp = fopen(mIdx, "wt");
	   if (fp) {
		 fprintf(fp, "<HTML>\n<HEAD>\n<TITLE>%s lists group</TITLE>\n</HEAD>\n<BODY>\n", mkey);
		 if (mold[0]){
	       fprintf(fp, "<A HREF=\"%s\">previous</A><BR><BR>\n", mold);
		 }
#ifdef UPDATE_20210303A // メーリングリストで投稿内容保存ファイルの拡張子の指定を可能にするオプションを追加した。
	     fprintf(fp, "No.%010lu:<A HREF=\"B%010lu.%s\">%s</A><BR>\n", mID, mID, mExte, mSubject);
#else
#ifdef UPDATE_20071204  // メッセージＩＤ採番処理を修正(Bym10id)
	     fprintf(fp, "No.%s:<A HREF=\"%s.TXT\">%s</A><BR>\n", mID, mID, mSubject);
#else
	     fprintf(fp, "No.%010lu:<A HREF=\"B%010lu.TXT\">%s</A><BR>\n", mID, mID, mSubject);
#endif
#endif
		 fprintf(fp, "</BODY>\n</HTML>\n");
	     fclose(fp);
	   }
	 }
#ifdef UPDATE_20210305 // メーリングリストで投稿内容保存ファイルのインデックスHTMLファイルの作成を禁止するオプション追加した。
     }
#endif

     sprintf(mIdx, "%sindex.tbl", mdir);
     fp = fopen(mIdx, "a+t");
	 if (fp) {
#ifdef UPDATE_20210303A // メーリングリストで投稿内容保存ファイルの拡張子の指定を可能にするオプションを追加した。
       fprintf(fp, "B%010lu.%s:%10lu: %s\n", mID, mExte, nMLNo, mSubject);
#else
#ifdef UPDATE_20071204  // メッセージＩＤ採番処理を修正(Bym10id)
       fprintf(fp, "%s.TXT:%10lu: %s\n", mID, nMLNo, mSubject);
#else
       fprintf(fp, "B%010lu.TXT:%10lu: %s\n", mID, nMLNo, mSubject);
#endif
#endif
	   fclose(fp);
	 }
   } else {
	 *mFolder = '\x0';
   }
   bUsedSaveList = FALSE;
#ifdef UPDATE_20091120 // ML保存時のindex.html作成のための排他処理、64bit長のファイルサイズに対応
   LeaveCriticalSection(&g_csMLIndex);
#endif

   return bListSave;
}

#ifdef UPDATE_20070509 // エイリアス設定の実アドレスオプションを別データとして処理する
BOOL GetAliases(LPCTSTR lpAppName, char *uid, char *dom, char *pOpt)
#else
BOOL GetAliases(LPCTSTR lpAppName, char *uid, char *dom)
#endif
{
  HKEY hKeyRoot = HKEY_LOCAL_MACHINE;
  HKEY hKey;
#ifdef UPDATE_20071205 //RFC2821: メールアドレス長が255Byteのときの対策
  CHAR  mkey[MAX_REG_RCPT];
#else
  CHAR mkey[256];
#endif
  DWORD  cbValueName = MAX_VALUE_NAME;
  DWORD  retCode;
  DWORD  dwIndex = 0;	         // index of subkey to enumerate 
  CHAR   lpName[_MAX_PATH];	 // address of buffer for subkey name 
  DWORD  lpcbName = 0;
  CHAR   mAliasKey[_MAX_PATH];
  CHAR   mAlias[_MAX_PATH];
  CHAR   *pAlias, *pAdom, *pMapto;
  CHAR   mMappedTo[_MAX_PATH], mwork[_MAX_PATH];
  BOOL   sts = FALSE;
  DWORD  stsDom = 0, stsUid = 1, nLeft, nRight, nUid;
  int    i = 0;
#ifdef REGTOFILE
  HANDLE hFile, hF = NULL;
#endif

#ifdef V4
  int  nUN = nMAXUser;  // 最大ユーザー数
#else
  int  nUN = USER_LIMIT; // 最大ユーザー数
#endif

  // OPEN THE KEY.
  sprintf(mkey,PROFILE_ROOT_TREE,lpAppName);
  if (bDebug) printf("Get Aliases List = %s\n", mkey);
   do {
	 if (i > 0)
	  _sleep(0);
#ifdef REGTOFILE
	 if (nClustering && !strnicmp(mkey, "software\\emwac", 14)) {
       retCode = 
         OpenKeyFile(mMailSpoolDir,
		             mkey,
					 "",
					 &hFile);

	 } else
#endif
	 {
     retCode = 
      RegOpenKeyEx(hKeyRoot,       // Key handle at root level.
                   (LPCTSTR)mkey,  // Path name of child key.
                   0,              // Reserved.
                   KEY_READ,       // Requesting read access.
                   &hKey);         // Address of key to be returned.
	 }
  } while(retCode == ERROR_INVALID_HANDLE && i++ < 3);  // ERROR_INVALID_HANDLE = 6
  if (retCode == ERROR_SUCCESS) {
  // READ THE KEY DATA.
    do {
#ifdef REGTOFILE
	 if (nClustering && !strnicmp(mkey, "software\\emwac", 14)) {
       retCode =
		 KeyFileEnumKeyEx(&hF,
		                mMailSpoolDir,
		                mkey,
						//dwIndex, 
						(LPTSTR)lpName, 
						(unsigned long *)&lpcbName);
	 } else 
#endif
	 {
      retCode =
          RegEnumKey(hKey,	        // handle of key to query 
                     dwIndex,	    // index of subkey to query 
                     (LPTSTR)lpName, // address of buffer for subkey name  
                     (unsigned long)&lpcbName 	    // size of subkey buffer 
         );
	 }
      if (bDebug) printf("Get Aliases List Data = ");
      if (retCode == ERROR_SUCCESS) {
		//sprintf(mAliasKey, "SOFTWARE\\EMWAC\\IMS\\Aliases\\%s", lpName);
		sprintf(mAliasKey, "%s\\%s", SOFT_ALIASES_REG , lpName);
        GetProfileStringEx(mAliasKey,"Alias", "", mAlias, sizeof(mAlias));
        if (bDebug) printf("Success. Aliases Name = %s", mAlias);
		_strlwr(mAlias);
		//////////////////////////////////////
		/////  Doamin Check
		pAdom = strstr(mAlias, "@");
	    stsDom = 1;
		if (pAdom)
          *pAdom = '\x0';
		if (dom && pAdom) {                // Domain check
#ifdef UPDATE_20070607 // ドメイン部のワイルドカードを許可
		  if (*(pAdom+1) == '*')
			stsDom = 0;
		  else
#endif
		  stsDom = _stricmp(dom+1, pAdom+1);
		} else if (!dom && !pAdom) {
		  stsDom = 0;
		}
		//////////////////////////////////////
		/////  User Check
		pAlias = strstr(mAlias,"*");
		if (pAlias) {
          *pAlias = '\x0';
          /*if (*(pAlias+1) == '\x0')
			stsUid = 0;
		  else */ {
             nLeft  = strlen(mAlias);
			 stsUid = _strnicmp(mAlias, uid, nLeft);
             nRight = strlen(pAlias+1);
			 if (nRight) {
			   nUid   = strlen(uid);
			   if (nUid < nLeft+nRight) {
 			     stsUid = -1;
			   } else {
			     if (stsUid || _strnicmp(pAlias+1, &uid[nUid-nRight], nRight))
				   stsUid = -1;
			   }
			 }
			 
		  }
		} else {
		  stsUid = _stricmp(mAlias, uid);
		}
		//////////////////////////////////////
        if (stsDom == 0 && stsUid == 0) {
		  sts = TRUE;
          GetProfileStringEx(mAliasKey,"MappedTo", "", mMappedTo, sizeof(mMappedTo));
          if (bDebug) printf(" -> %s is hit.\n", mMappedTo);
#ifdef UPDATE_20070509 // エイリアス設定の実アドレスオプションを別データとして処理する
		  if (pOpt) {  // NULLでないなら
			if ((pMapto = strchr(mMappedTo, ','))) {
			  *pMapto = '\x0';  // オプション削除
			  pMapto++;         // オプション情報
			  strcpy(pOpt, pMapto);
			}
		  } else {
			strtok(mMappedTo, ","); // オプション削除
		  }
#endif
          pMapto = strstr(mMappedTo,"*");
		  if (pMapto) {
			//strcpy(mwork, uid);
	        strcpy(mwork, &uid[strlen(mAlias)]);
			*pMapto = '\x0';
			//sprintf(uid, "%s%s%s", mMappedTo, uid, pMapto+1);
			sprintf(uid, "%s%s%s", mMappedTo, mwork, pMapto+1);
		  } else
	        strcpy(uid, mMappedTo);
		  break;
		} else 
          if (bDebug) printf("\n");
	  } else
        if (bDebug) printf("end.\n");
      dwIndex++;
#ifdef HOME_VERSION
      nUN--; // 最大ユーザー数
#else
#ifdef V4
       if (nMAXUser) // 無制限でないなら
	     nUN--; // 最大ユーザー数
#endif
#endif
	}
#ifdef HOME_VERSION
	while(retCode == ERROR_SUCCESS && nUN); // 人数制限ないなら繰り返し
#else
#ifdef V4
	while(retCode == ERROR_SUCCESS && (nMAXUser ? nUN : 1)); // 人数制限ないなら繰り返し
#else
    while (retCode == ERROR_SUCCESS);
#endif
#endif

#ifdef REGTOFILE
    if (nClustering && !strnicmp(mkey, "software\\emwac", 14)) {
	  if (hF)
	    FindClose(hF); 
	  CloseHandle(hFile);
    } else
#endif
    RegCloseKey(hKey);
  }
  return sts;
}

BOOL GetPostMaster(char *uid, char *dom) {
  CHAR   mMappedTo[_MAX_PATH], *pMapdom;
  BOOL   sts = FALSE;

  // OPEN THE KEY.
  if (_stricmp(uid,"postmaster") == 0) {
    //GetProfileStringEx("SOFTWARE\\EMWAC\\IMS","PostMaster", "", mMappedTo, sizeof(mMappedTo));
    GetProfileStringEx(SOFT_REG,"PostMaster", "administrator", mMappedTo, sizeof(mMappedTo));
    if (strlen(mMappedTo) > 0) {
	  pMapdom = strpbrk(mMappedTo, "@");
	  if (!pMapdom && dom) {
		strcat(mMappedTo, "@");
	    strcat(mMappedTo, dom+1);
	  }
      strcpy(uid, mMappedTo);
      sts = TRUE;
	}
  }
  return sts;
}

BOOL GetMLSenderPermission(char *uid) {
  CHAR mkey[MAX_REG_RCPT];
  CHAR *p, mRequestName[_MAX_PATH];	// address of buffer for subkey name 
#ifdef UPDATE_20070521 // 予約語対策
  char *q, mUID[512];
  BOOL bReservedWords = FALSE;

  strcpy(mUID, uid);
  strtok(mUID, "@");
  if (ReservedWords(mUID)) {
    if ((q = strstr(uid, mUID))) {
      strcpy(mUID, mReservedWords);
      strcat(mUID, q);
  	  bReservedWords = TRUE;
	}
  }
#endif


#ifdef UPDATE_20070521 // 予約語対策
   if (bReservedWords) {
     strcpy(mRequestName, mUID);
   } else
#endif
   {
     strcpy(mRequestName, uid);
   } 
   p = strstr(mRequestName, "-request@");
   if (p) {
     *p = '\x0';
     sprintf(mkey,"%s\\%s%s", SOFT_LISTS_REG, mRequestName, (p+8));
   } else {
     p = strstr(mRequestName, "-request");
     if (p) {
       *p = '\x0';
       sprintf(mkey,"%s\\%s", SOFT_LISTS_REG, mRequestName);
	 } else {
#ifdef UPDATE_20070521 // 予約語対策
       if (bReservedWords) {
         sprintf(mkey,"%s\\%s", SOFT_LISTS_REG, mUID);
	   } else
#endif
	   {
         sprintf(mkey,"%s\\%s", SOFT_LISTS_REG, uid);
	   }
	 }
   }
   return (GetProfileIntEx(mkey, "NoFrom", (INT)FALSE)); // (Default)FALSE:許可 TRUE:拒否 
}

BOOL QueryMLists(LPCTSTR lpAppName, char *uid) {
  HKEY hKeyRoot = HKEY_LOCAL_MACHINE;
  HKEY hKey;
  CHAR mkey[MAX_REG_RCPT]; //[256];
  DWORD  retCode;
  BOOL   sts = FALSE;
  int    i = 0;
#ifdef REGTOFILE
  HANDLE hFile;
#endif

  // OPEN THE KEY.
#ifndef TUNING
  sprintf(mkey,PROFILE_ROOT_TREE,lpAppName);
  strcat(mkey, uid);
#else
  strcpy(mkey, lpAppName);
  strcat(mkey, "\\");
  strcat(mkey, uid);
#endif
  if (bDebug) printf("Get Mailing List = %s\n", mkey);
   do {
	 if (i > 0)
	  _sleep(0);
#ifdef REGTOFILE
	 if (nClustering && !strnicmp(mkey, "software\\emwac", 14)) {
       retCode = 
         OpenKeyFile(mMailSpoolDir,
		             mkey,
					 "",
					 &hFile);

	 } else {
#endif
     retCode = 
      RegOpenKeyEx(hKeyRoot,       // Key handle at root level.
                   (LPCTSTR)mkey,  // Path name of child key.
                   0,              // Reserved.
                   KEY_READ,       // Requesting read access.
                   &hKey);         // Address of key to be returned.
#ifdef REGTOFILE
	 }
#endif
  } while(retCode == ERROR_INVALID_HANDLE && i++ < 3);  // ERROR_INVALID_HANDLE = 6
  if (retCode == ERROR_SUCCESS) {
#ifdef REGTOFILE
    if (nClustering && !strnicmp(mkey, "software\\emwac", 14))
	  CloseHandle(hFile);
    else
#endif
    RegCloseKey(hKey);
	sts = TRUE;
  }

  return sts;
}

BOOL GetMLists(LPCTSTR lpAppName, char *uid, BOOL *bRequest) {
  HKEY hKeyRoot = HKEY_LOCAL_MACHINE;
  HKEY hKey;
#ifdef UPDATE_20071205 //RFC2821: メールアドレス長が255Byteのときの対策
  CHAR mkey[MAX_REG_RCPT];
#else
  CHAR mkey[256];
#endif
  DWORD  cbValueName = MAX_VALUE_NAME;
  DWORD  retCode;
  DWORD  dwIndex = 0;	         // index of subkey to enumerate 
  CHAR   mName[_MAX_PATH];	 // address of buffer for subkey name 
  CHAR   mRequestName[_MAX_PATH];	// address of buffer for subkey name 
  DWORD  lpcbName = 0;
  BOOL   sts = FALSE;
  char   *p;
  int    i = 0;
#ifdef REGTOFILE
  HANDLE hFile, hF = NULL;
#endif
#ifdef V4
  int  nUN = nMAXUser;  // 最大ユーザー数
#else
  int  nUN = USER_LIMIT; // 最大ユーザー数
#endif
#ifdef UPDATE_20070521 // 予約語対策
  char *q, mUID[512];
  BOOL bReservedWords = FALSE;

  strcpy(mUID, uid);
  strtok(mUID, "@");
  if (ReservedWords(mUID)) {
    if ((q = strstr(uid, mUID))) {
      strcpy(mUID, mReservedWords);
      strcat(mUID, q);
  	  bReservedWords = TRUE;
	}
  }
#endif
  // OPEN THE KEY.
  sprintf(mkey,PROFILE_ROOT_TREE,lpAppName);
  if (bDebug) printf("Get Mailing List = %s\n", mkey);
  do {
	if (i > 0)
	  _sleep(0);
#ifdef REGTOFILE
	 if (nClustering && !strnicmp(mkey, "software\\emwac", 14)) {
       retCode = 
         OpenKeyFile(mMailSpoolDir,
		             mkey,
					 "",
					 &hFile);

	 } else
#endif
	 { 
    retCode = 
      RegOpenKeyEx(hKeyRoot,       // Key handle at root level.
                   (LPCTSTR)mkey,  // Path name of child key.
                   0,              // Reserved.
                   KEY_READ,       // Requesting read access.
                   &hKey);         // Address of key to be returned.
	 }
  } while(retCode == ERROR_INVALID_HANDLE && i++ < 3);  // ERROR_INVALID_HANDLE = 6
  if (retCode == ERROR_SUCCESS) {
  // READ THE KEY DATA.
    do {
#ifdef REGTOFILE
	 if (nClustering && !strnicmp(mkey, "software\\emwac", 14)) {
       retCode =
		 KeyFileEnumKeyEx(&hF,
		                mMailSpoolDir,
		                mkey,
						//dwIndex, 
						(LPTSTR)mName, 
						(unsigned long *)&lpcbName);
	 } else
#endif
	 {
      retCode =
          RegEnumKey(hKey,	        // handle of key to query 
                     dwIndex,	    // index of subkey to query 
                     (LPTSTR)mName, // address of buffer for subkey name  
                     (unsigned long)&lpcbName 	    // size of subkey buffer 
         );
	 }
      if (bDebug) printf("Get Mailing List Data = ");
      if (retCode == ERROR_SUCCESS) {
        if (bDebug) printf("Success. List Name = %s\n", mName);
#ifdef MULTI_ML
		// 2002.09.04 ドメイン付なら@マーク以降を削除
		  strcpy(mRequestName, mName);
		  strtok(mRequestName, "@");
		  strcat(mRequestName, "-request");
#ifdef UPDATE_20070521 // 予約語対策
	    if (!strnicmp(mName, mReservedWords, strlen(mReservedWords))) {
		  p = strstr(&mName[strlen(mReservedWords)], "@");
		} else
#endif
		{
		  p = strstr(mName, "@");
		}
		if (p) {
		  strcat(mRequestName, p);
		}
#else
	    sprintf(mRequestName,"%s-request", mName);
#endif
#ifdef UPDATE_20070521 // 予約語対策
		if (bReservedWords) {
          if (_stricmp(mName, mUID) == 0 ||
		      _stricmp(mRequestName, mUID) == 0) {
		    sts = TRUE;
		    if (_stricmp(mRequestName, mUID) == 0)
			  *bRequest = TRUE;
		    break;
		  }
        } else
#endif
		{
          if (_stricmp(mName, uid) == 0 ||
		      _stricmp(mRequestName, uid) == 0) {
		    sts = TRUE;
		    if (_stricmp(mRequestName, uid) == 0)
			  *bRequest = TRUE;
		    break;
		  }
		}
	  } else
        if (bDebug) printf("end.\n");

      dwIndex++;
#ifdef HOME_VERSION
      nUN--; // 最大ユーザー数
#else
#ifdef V4
       if (nMAXUser) // 無制限でないなら
	     nUN--; // 最大ユーザー数
#endif
#endif
	}
#ifdef HOME_VERSION
	while(retCode == ERROR_SUCCESS && nUN); // 人数制限ないなら繰り返し
#else
#ifdef V4
	while(retCode == ERROR_SUCCESS && (nMAXUser ? nUN : 1)); // 人数制限ないなら繰り返し
#else
    while (retCode == ERROR_SUCCESS);
#endif
#endif
#ifdef REGTOFILE
    if (nClustering && !strnicmp(mkey, "software\\emwac", 14)) {
	  if (hF)
	    FindClose(hF); 
	  CloseHandle(hFile);
    } else
#endif
    RegCloseKey(hKey);
  }
  return sts;
}

DWORD GetProfileIntEx(LPCTSTR lpAppName, LPCTSTR lpKeyName, INT nDefault) {
  HKEY hKeyRoot = HKEY_LOCAL_MACHINE;
  HKEY hKey;
#ifdef UPDATE_20071205 //RFC2821: メールアドレス長が255Byteのときの対策
  CHAR mkey[MAX_REG_RCPT];
#else
  CHAR mkey[256];
#endif
  DWORD  cbValueName = MAX_VALUE_NAME;
  DWORD  retCode;
  DWORD  dwIndex = 0;	         // index of subkey to enumerate 
  DWORD  dwType;
  DWORD  nReturned;
  DWORD  nSize = sizeof(DWORD);
  int    i = 0;
#ifdef REGTOFILE
  HANDLE hFile;
#endif
  // READ THE KEY DATA.
  nReturned = (DWORD)nDefault;
  sprintf(mkey,PROFILE_ROOT_TREE,lpAppName);
   do {
	 if (i > 0)
	  _sleep(0);
#ifdef REGTOFILE
    if (nClustering && !strnicmp(lpAppName, "software\\emwac", 14)) {
#ifdef READING_ASYNCHRONOUS
       retCode = 
         OpenKeyFile(mMailSpoolDir,
		             mkey,
					 (char *)lpKeyName,
					 &hFile);
#else
	   retCode = ERROR_SUCCESS;
#endif
	 } else
#endif
	{
     retCode = 
      RegOpenKeyEx(hKeyRoot,       // Key handle at root level.
                   (LPCTSTR)mkey,  // Path name of child key.
                   0,              // Reserved.
                   KEY_READ,       // Requesting read access.
                   &hKey);         // Address of key to be returned.
	 }
  } while(retCode == ERROR_INVALID_HANDLE && i++ < 3);  // ERROR_INVALID_HANDLE = 6
  if (retCode == ERROR_SUCCESS) {
	dwType = REG_DWORD;
#ifdef REGTOFILE
    if (nClustering && !strnicmp(lpAppName, "software\\emwac", 14)) {
       retCode =
		 KeyFileQueryValueEx(mMailSpoolDir,
		                     mkey,
					         (char *)lpKeyName,
							 &hFile,
							 dwType,
							 (LPBYTE)&nReturned,
							 &nSize);
#ifdef READING_ASYNCHRONOUS
	     CloseHandle(hFile);
#endif
	 } else
#endif
	{
    retCode =
	   RegQueryValueEx(hKey,            // handle of key to query 
                      (LPSTR)lpKeyName, // address of name of value to query 
                      0,                // reserved 
                      &dwType,  // address of buffer for value type 
                      (LPBYTE)&nReturned,  // address of data buffer 
                      &nSize  // address of data buffer size 
                      ); 
    RegCloseKey(hKey);
	 }
	if (retCode) {
      nReturned = (DWORD)nDefault;
	}
  } else 
    nReturned = (DWORD)nDefault;
  return nReturned;
} 

DWORD GetProfileStringEx(LPCTSTR lpAppName, LPCTSTR lpKeyName, LPCTSTR lpDefault, LPTSTR lpReturnedString, DWORD nSize) {
  HKEY hKeyRoot = HKEY_LOCAL_MACHINE;
  HKEY hKey;
#ifdef UPDATE_20071205 //RFC2821: メールアドレス長が255Byteのときの対策
  CHAR mkey[MAX_REG_RCPT];
#else
  CHAR mkey[256];
#endif
  DWORD  cbValueName = MAX_VALUE_NAME;
  DWORD  retCode;
  DWORD  dwIndex = 0;	         // index of subkey to enumerate 
  DWORD  dwType;
  int    i = 0;
#ifdef REGTOFILE
  HANDLE hFile;
#endif
  // READ THE KEY DATA.
#ifndef TUNING
  memset(lpReturnedString, 0, nSize);
  strcpy(lpReturnedString, lpDefault);
#else
  *lpReturnedString = '\x0';
#endif
  sprintf(mkey,PROFILE_ROOT_TREE,lpAppName);
   do {
	 if (i > 0)
	  _sleep(0);
#ifdef REGTOFILE
    if (nClustering && !strnicmp(lpAppName, "software\\emwac", 14)) {
#ifdef READING_ASYNCHRONOUS
       retCode = 
         OpenKeyFile(mMailSpoolDir,
		             mkey,
					 (char *)lpKeyName,
					 &hFile);
#else
	   retCode = ERROR_SUCCESS;
#endif
	 } else
#endif
	{
     retCode = 
      RegOpenKeyEx(hKeyRoot,       // Key handle at root level.
                   (LPCTSTR)mkey,  // Path name of child key.
                   0,              // Reserved.
                   KEY_READ,       // Requesting read access.
                   &hKey);         // Address of key to be returned.
	 }
  } while(retCode == ERROR_INVALID_HANDLE && i++ < 3);  // ERROR_INVALID_HANDLE = 6
 
  if (retCode == ERROR_SUCCESS) {
	dwType = REG_SZ;
#ifdef REGTOFILE
    if (nClustering && !strnicmp(lpAppName, "software\\emwac", 14)) {
       retCode =
		 KeyFileQueryValueEx(mMailSpoolDir,
		                     mkey,
					         (char *)lpKeyName,
							 &hFile,
							 dwType,
							 (LPBYTE)lpReturnedString,
							 &nSize);
#ifdef READING_ASYNCHRONOUS
	     CloseHandle(hFile);
#endif
	 } else
#endif
	{
    retCode =
	   RegQueryValueEx(hKey,            // handle of key to query 
                      (LPSTR)lpKeyName, // address of name of value to query 
                      0,                // reserved 
                      &dwType,  // address of buffer for value type 
                      (LPBYTE)lpReturnedString,  // address of data buffer 
                      &nSize  // address of data buffer size 
                      ); 
    RegCloseKey(hKey);
	 }
	if (retCode) {
      strcpy(lpReturnedString, lpDefault);
	}
  } else
    strcpy(lpReturnedString, lpDefault);
  return nSize;
}

DWORD GetProfileBinaryEx(LPCTSTR lpAppName, LPCTSTR lpKeyName, LPCTSTR lpDefault, LPTSTR lpReturnedString, DWORD nSize) {
  HKEY hKeyRoot = HKEY_LOCAL_MACHINE;
  HKEY hKey;
#ifdef UPDATE_20071205 //RFC2821: メールアドレス長が255Byteのときの対策
  CHAR mkey[MAX_REG_RCPT];
#else
  CHAR mkey[256];
#endif
  DWORD  cbValueName = MAX_VALUE_NAME;
  DWORD  retCode;
  DWORD  dwIndex = 0;	         // index of subkey to enumerate 
  DWORD  dwType;
  int    i = 0;
#ifdef REGTOFILE
  HANDLE hFile;
#endif

  // READ THE KEY DATA.
#ifndef TUNING
  memset(lpReturnedString, 0, nSize);
  strcpy(lpReturnedString, lpDefault);
#else
  *lpReturnedString = '\x0';
#endif
  sprintf(mkey,PROFILE_ROOT_TREE,lpAppName);
   do {
	 if (i > 0)
	  _sleep(0);
#ifdef REGTOFILE
    if (nClustering && !strnicmp(lpAppName, "software\\emwac", 14)) {
#ifdef READING_ASYNCHRONOUS
       retCode = 
         OpenKeyFile(mMailSpoolDir,
		             mkey,
					 (char *)lpKeyName,
					 &hFile);
#else
	   retCode = ERROR_SUCCESS;
#endif
	 } else
#endif
	{
     retCode = 
      RegOpenKeyEx(hKeyRoot,       // Key handle at root level.
                   (LPCTSTR)mkey,  // Path name of child key.
                   0,              // Reserved.
                   KEY_READ,       // Requesting read access.
                   &hKey);         // Address of key to be returned.
	 }
  } while(retCode == ERROR_INVALID_HANDLE && i++ < 3);  // ERROR_INVALID_HANDLE = 6
  
  if (retCode == ERROR_SUCCESS) {
	dwType = REG_BINARY;
#ifdef REGTOFILE
    if (nClustering && !strnicmp(lpAppName, "software\\emwac", 14)) {
       retCode =
		 KeyFileQueryValueEx(mMailSpoolDir,
		                     mkey,
					         (char *)lpKeyName,
							 &hFile,
							 dwType,
							 (LPBYTE)lpReturnedString,
							 &nSize);
#ifdef READING_ASYNCHRONOUS
	     CloseHandle(hFile);
#endif
	 } else 
#endif
	{
    retCode =
	     RegQueryValueEx(hKey,            // handle of key to query 
                      (LPSTR)lpKeyName, // address of name of value to query 
                      0,                // reserved 
                      &dwType,  // address of buffer for value type 
                      lpReturnedString,  // address of data buffer 
                      &nSize  // address of data buffer size 
                      ); 
    RegCloseKey(hKey);
	 }
#ifndef UPDATE_20240219 // 付加表題付きのＭＬでの文字化け対策
	if (retCode != ERROR_SUCCESS)
#ifndef TUNING
      strcpy(lpReturnedString, "");
#else
	  *lpReturnedString = '\x0';
#endif
#endif
  } else
    strcpy(lpReturnedString, lpDefault);
  return nSize;
}

void WriteProfileIntEx(CHAR * KeyPath, CHAR * ValuePath, DWORD ValueInt) {
  HKEY   hKeyRoot = HKEY_LOCAL_MACHINE;
  HKEY   hKey;
  DWORD  retCode;
  int    mach = 1;
  DWORD  cbValueName = 80;
  DWORD  dwIndex = 0;	         // index of subkey to enumerate 
  DWORD  lpData = 0;
  DWORD  cbData = 80;
#ifdef UPDATE_20110909 // ML名+ドメイン名が53byteを超えるとハングする
  CHAR   RegPath[512];
#else
  CHAR   RegPath[80];
#endif
  int    i = 0;
#ifdef REGTOFILE
  HANDLE hFile;
#endif

  sprintf(RegPath, PROFILE_ROOT_TREE, KeyPath);
  lpData = (unsigned long)ValueInt;
  cbData = sizeof(DWORD);

   do {
	 if (i > 0)
	  _sleep(0);
#ifdef REGTOFILE
     if (nClustering && !strnicmp(KeyPath, "software\\emwac", 14)) {
       retCode = 
         OpenKeyFile(mMailSpoolDir,
		             RegPath,
					 (char *)ValuePath,
					 &hFile);

	 } else
#endif
	 {
     retCode = 
      RegOpenKeyEx(hKeyRoot,    // Key handle at root level.
                   RegPath,     // Path name of child key.
                   0,           // Reserved.
                   KEY_WRITE,  // KEY_READ | KEY_WRITE, // Requesting read access.
                   &hKey);      // Address of key to be returned.
	 }
  } while(retCode == ERROR_INVALID_HANDLE && i++ < 3);  // ERROR_INVALID_HANDLE = 6
	if (retCode == ERROR_SUCCESS) {
#ifdef REGTOFILE
     if (nClustering && !strnicmp(KeyPath, "software\\emwac", 14)) {
       retCode =
		 KeyFileSetValueEx(mMailSpoolDir,
		                   RegPath,
					       ValuePath,
						   &hFile,
						   REG_DWORD,
						   (LPBYTE)&lpData,
						   (DWORD)cbData);
	     CloseHandle(hFile);
	 } else
#endif
	 {
      retCode =
         RegSetValueEx(hKey,	// handle of key to set value for  
                       ValuePath,	// address of value to set 
                       0,
                       REG_DWORD,	// type of value 
                       (CONST BYTE *)&lpData,	// address of value data 
                       (DWORD)cbData 	// size of value data 
                      );	
      RegCloseKey((HKEY)hKey);
	 }
	}
}

void WriteProfileStringEx(CHAR * KeyPath, CHAR * ValuePath, LPCTSTR ValueString) {
  HKEY   hKeyRoot = HKEY_LOCAL_MACHINE;
  HKEY   hKey;
  DWORD  retCode;
  int    mach = 1;
  DWORD  cbValueName = 80;
  DWORD  dwIndex = 0;	         // index of subkey to enumerate 
  DWORD  lpData = 0;
  DWORD  cbData = 80;
#ifdef UPDATE_20110909 // ML名+ドメイン名が53byteを超えるとハングする
  CHAR   RegPath[512];
#else
  CHAR   RegPath[80];
#endif
  int    i = 0;
#ifdef REGTOFILE
  HANDLE hFile;
#endif

  sprintf(RegPath, PROFILE_ROOT_TREE, KeyPath);
  lpData = (unsigned long)ValueString;
  cbData = strlen(ValueString);

   do {
	 if (i > 0)
	  _sleep(0);
#ifdef REGTOFILE
     if (nClustering && !strnicmp(KeyPath, "software\\emwac", 14)) {
       retCode = 
         OpenKeyFile(mMailSpoolDir,
		             RegPath,
					 ValuePath,
					 &hFile);

	 } else
#endif
	 {
     retCode = 
      RegOpenKeyEx(hKeyRoot,    // Key handle at root level.
                   RegPath,     // Path name of child key.
                   0,           // Reserved.
                   KEY_WRITE,   // KEY_READ | KEY_WRITE, // Requesting read access.
                   &hKey);      // Address of key to be returned.
	 }
  } while(retCode == ERROR_INVALID_HANDLE && i++ < 3);  // ERROR_INVALID_HANDLE = 6
	if (retCode == ERROR_SUCCESS) {
#ifdef REGTOFILE
     if (nClustering && !strnicmp(KeyPath, "software\\emwac", 14)) {
       retCode =
		 KeyFileSetValueEx(mMailSpoolDir,
		                     RegPath,
					         ValuePath,
							 &hFile,
							 REG_SZ,
							 (LPBYTE)lpData,
							 (DWORD)cbData);
	     CloseHandle(hFile);
	 } else
#endif
	 {
      retCode =
         RegSetValueEx(hKey,	// handle of key to set value for  
                       ValuePath,	// address of value to set 
                       0,
                       REG_SZ,	// type of value 
                       (CONST BYTE *)lpData,	// address of value data 
                       (DWORD)cbData 	// size of value data 
                      );	
      RegCloseKey((HKEY)hKey);
	 }
	}
}

#ifdef UPDATE_20070405 // イベントログデータベースを追加。
///////////////////////////////////////////////////////////////////////////
// [形    式]    InitEventLog
// [引    数]    なし
// [説    明]    イベントログを出力可能にするための初期設定
// [戻 り 値]    正常終了  ERROR_SUCCESS
//               異常終了  エラーコード
///////////////////////////////////////////////////////////////////////////
LRESULT InitEventLog(VOID)
{
	HKEY	hKey;
	DWORD	dwType = EVENTLOG_ERROR_TYPE
					| EVENTLOG_WARNING_TYPE
					| EVENTLOG_INFORMATION_TYPE;
	CHAR	szRegPath[MAX_PATH];
	CHAR	szFilePath[2][MAX_PATH];
	LRESULT lResult = ERROR_CALL_NOT_IMPLEMENTED;
	CHAR    *p;

	sprintf(szRegPath, "%s%s", EVENT_REG, SMTP_SERVICE);

	// レジストリキーが既に存在するなら処理しない
	if ((lResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE, (LPCTSTR)szRegPath, 0, KEY_READ, &hKey)) == ERROR_SUCCESS) {
	  goto LabelExit;
	}

	// レジストリキー作成
	if((lResult = RegCreateKey(
			HKEY_LOCAL_MACHINE, szRegPath, &hKey)) != ERROR_SUCCESS){
		return lResult;
	}

	// サポートタイプ設定
	if((lResult = RegSetValueEx(hKey, "TypesSupported",
			0, REG_DWORD, (LPBYTE)&dwType, sizeof(DWORD))) != ERROR_SUCCESS){
		goto LabelExit;
	}

	// メッセージファイルパス設定
	if(GetModuleFileName(NULL, szFilePath[0], sizeof(szFilePath[0])) == 0){
		lResult = GetLastError();
		goto LabelExit;
	}
    GetLongPathName(szFilePath[0], szFilePath[1], sizeof(szFilePath[1]));

	if ((p = strrchr(szFilePath[0], '\\'))) {
	  *p = '\x0';
	  if ((p = strrchr(szFilePath[1], '\\'))) {
	    strcat(szFilePath[0], p);
	    if ((p = strrchr(szFilePath[0], '.'))) {
	      *p = '\x0';
	      strcat(szFilePath[0], ".dll");
 	      lResult = RegSetValueEx(hKey, "EventMessageFile",
			   0, REG_EXPAND_SZ, (LPBYTE)szFilePath[0], lstrlen(szFilePath[0]) + 1);
		}
	  }
	}

LabelExit:
	RegCloseKey(hKey);
	return lResult;
}
#endif

#ifdef ADD_IDCACHE

BOOL IDLiveTime(char *pFn) 
{
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
	 //GetFileTime(hFile, &ft, &st, NULL);
     GetFileTime(hFile, NULL, &st, &ft);
     CloseHandle(hFile);
     u2 = (ULARGE_INTEGER *)&ft;
     n2  = u2->QuadPart;
     if (n1 < (n2 + (__int64)nIDCashLiveTime*10000000) )
	   return TRUE;    // キャッシュ有効
   }
   return FALSE;   // キャッシュ無効
}

BOOL ReserveIDChche(char *pID)
{
  FILE    *fp;
  char    mIDCash[512];
  BOOL    bSts = TRUE;

#ifdef REGTOFILE
   sprintf(mIDCash,"%s\\" ADCACHE, mMailSpoolDir);
   _mkdir(mIDCash);         // cashフォルダ作成
   if (nClustering) {
     sprintf(mIDCash,"%s\\" ADCACHE "\\%s\\", mMailSpoolDir, mComName);
   } else {
#endif
     sprintf(mIDCash,"%s\\" ADCACHE "\\", mMailSpoolDir);
#ifdef REGTOFILE
   }
#endif

   _mkdir(mIDCash);         // cashフォルダ作成
   strcat(mIDCash, pID);
   strcat(mIDCash, ".lck");

     while (!(fp = _fsopen(mIDCash, "wb", _SH_DENYWR))) 
	 {
       if (bServiceTerminating)
         break;
	   _sleep(WAIT_TIME);
	 }
     if (fp)
	 { 
       fclose(fp);
       while (!(fp = _fsopen(mIDCash, "rb", _SH_DENYWR))) 
	   {
         if (bServiceTerminating)
           break;
	     _sleep(WAIT_TIME);
         // IDロックファイルが作成されない場合の対策
         if ((fp = _fsopen(mIDCash, "wb", _SH_DENYWR))) 
		 { 
           fclose(fp);
		 }
	   }
       fclose(fp);
     } else {
       bSts = FALSE;
     }
     return bSts;
}

void ReleaseIDChche(char *pID)
{
  FILE    *fp;
  char    mIDCash[512];
  BOOL    bSts = TRUE;

#ifdef REGTOFILE
      if (nClustering) {
        sprintf(mIDCash,"%s\\" ADCACHE "\\%s\\%s.lck", mMailSpoolDir, mComName, pID);  // cashフォルダ
	  } else {
#endif
   sprintf(mIDCash,"%s\\" ADCACHE "\\%s.lck", mMailSpoolDir, pID);  // cashフォルダ
#ifdef REGTOFILE
	  }
#endif
     _unlink(mIDCash);
     while ((fp = _fsopen(mIDCash, "rb", _SH_DENYWR))) {
	  fclose(fp);
      if (bServiceTerminating)
        break;
	   _sleep(WAIT_TIME);
       // IDロックファイルが削除されない場合の対策
       _unlink(mIDCash);
	}
}

void SetIDCashList(char *pID, char *pData, char *pName)
{
#ifdef ACCEPT_LOG_LEVEL3
	char str[LOG_BUFFER];
#endif
  FILE    *fpr, *fpw;
  char    mIDCash[512];
  char    mIDB[256];
  char    mID[512];

   if (nIDCashLiveTime == 0) // 更新時間が設定されていない場合は設定しない
	 return;
   if (strlen(pID)==0) 
     return;
   strcpy(mIDB, pID);
   translateUue2Base64(mIDB, strlen(mIDB), mID);
   if (!ReserveIDChche(mID)) // ロックファイルがないなら処理可能
   {
	 return;
   }
   strcat(mID, ".");
   strcat(mID, pName);

#ifdef ACCEPT_LOG_LEVEL3
   sprintf(str, "[          ] [%s] SetIDCashList=[%s]\n", pID, pName);
   LEVEL_3_ACCEPTLOG(NULL, str);
#endif

#ifdef REGTOFILE
   sprintf(mIDCash,"%s\\" ADCACHE, mMailSpoolDir);
   _mkdir(mIDCash);         // cashフォルダ作成
   if (nClustering) {
     sprintf(mIDCash,"%s\\" ADCACHE "\\%s", mMailSpoolDir, mComName);
     _mkdir(mIDCash);         // cashフォルダ作成
     sprintf(mIDCash,"%s\\" ADCACHE "\\%s\\", mMailSpoolDir, mComName);
   } else {
#endif
     sprintf(mIDCash,"%s\\" ADCACHE "\\", mMailSpoolDir);
#ifdef REGTOFILE
   }
#endif

   _mkdir(mIDCash);         // cashフォルダ作成
   strcat(mIDCash, mID);
   //if (!(fpr = _fsopen(mIDCash, "rb", _SH_DENYWR))) 
   {
     while (!(fpw = _fsopen(mIDCash, "wb", _SH_DENYWR))) 
	 {
       if (bServiceTerminating)
         break;
	   _sleep(WAIT_TIME);
	 }
	 if (fpw)
	 {
	   SPA_Encode(pData, mIDB);
       fprintf(fpw, "%s", mIDB);
	   fclose(fpw);
	 }
     // IDキャッシュデータの削除でハングする
     while (!(fpw = _fsopen(mIDCash, "rb", _SH_DENYWR))) {
       if (bServiceTerminating)
         break;
	    _sleep(WAIT_TIME);
	 }
     fclose(fpw);
	/*
   } else {
     fclose(fpr);
	 */
   }
   strtok(mID, ".");
   ReleaseIDChche(mID);
}

BOOL CheckIDCashList(char *pID)
{
  char    mIDB[256];
  char    mID[512];
  BOOL    bSts = FALSE;

   if (strlen(pID)==0) 
     return bSts;
   strcpy(mIDB, pID);
   translateUue2Base64(mIDB, strlen(mIDB), mID);
   if (!ReserveIDChche(mID)) // ロックファイルがないなら処理可能
   {
	 return bSts;
   } 
   return TRUE;
}

void AddIDCashList(char *pID, char *pData, char *pName)
{
#ifdef ACCEPT_LOG_LEVEL3
	char str[LOG_BUFFER];
#endif
  FILE    *fpr, *fpw;
  char    mIDCash[512];
  char    mIDB[256], mIDB2[256];
  char    mID[512];

   if (nIDCashLiveTime == 0) // 更新時間が設定されていない場合は設定しない
	 return;
   if (strlen(pID)==0) 
     return;
   strcpy(mIDB, pID);
   translateUue2Base64(mIDB, strlen(mIDB), mID);
   strcat(mID, ".");
   strcat(mID, pName);

#ifdef ACCEPT_LOG_LEVEL3
   sprintf(str, "[          ] [%s] AddIDCashList=[%s]\n", pID, pName);
   LEVEL_3_ACCEPTLOG(NULL, str);
#endif

#ifdef REGTOFILE
   sprintf(mIDCash,"%s\\" ADCACHE "\\", mMailSpoolDir);
   _mkdir(mIDCash);         // cashフォルダ作成
   if (nClustering) {
     sprintf(mIDCash,"%s\\" ADCACHE "\\%s\\", mMailSpoolDir, mComName);
   } else {
#endif
     sprintf(mIDCash,"%s\\" ADCACHE "\\", mMailSpoolDir);
#ifdef REGTOFILE
   }
#endif

   _mkdir(mIDCash);         // cashフォルダ作成
   strcat(mIDCash, mID);
   {
     while (!(fpw = _fsopen(mIDCash, "ab", _SH_DENYWR))) 
	 {
       if (bServiceTerminating)
         break;
	   _sleep(WAIT_TIME);
	 }
	 if (fpw)
	 {
	   SPA_Encode(pData, mIDB);
       fprintf(fpw, "%s\n", mIDB);
	   fclose(fpw);
       // IDキャッシュデータの削除でハングする
       while (!(fpw = _fsopen(mIDCash, "rb", _SH_DENYWR))) {
         if (bServiceTerminating)
           break;
	     _sleep(WAIT_TIME);
	   }
	   fclose(fpw);
	 }
   }
}

int CountIDCashList(char *pID, char *pData, char *pName, char *pMGroup, BOOL *pbGroup, BOOL *pbResult)
{
#ifdef ACCEPT_LOG_LEVEL3
	char str[LOG_BUFFER];
#endif
  FILE    *fpr, *fpw;
  char    mIDCash[512];
  char    mIDB[256];
  char    mID[512];
  int     nCnt = 0;

   *pbResult = FALSE;
   *pbGroup = FALSE;
   if (nIDCashLiveTime == 0) // 更新時間が設定されていない場合は設定しない
	 return nCnt;
   if (strlen(pID)==0) 
     return nCnt;
   strcpy(mIDB, pID);
   translateUue2Base64(mIDB, strlen(mIDB), mID);
   strcat(mID, ".");
   strcat(mID, pName);

#ifdef ACCEPT_LOG_LEVEL3
   sprintf(str, "[          ] [%s] CountIDCashList=[%s]\n", pID, pName);
   LEVEL_3_ACCEPTLOG(NULL, str);
#endif

#ifdef REGTOFILE
   sprintf(mIDCash,"%s\\" ADCACHE "\\", mMailSpoolDir);
   _mkdir(mIDCash);         // cashフォルダ作成
   if (nClustering) {
     sprintf(mIDCash,"%s\\" ADCACHE "\\%s\\", mMailSpoolDir, mComName);
   } else {
#endif
     sprintf(mIDCash,"%s\\" ADCACHE "\\", mMailSpoolDir);
#ifdef REGTOFILE
   }
#endif

   _mkdir(mIDCash);         // cashフォルダ作成
   strcat(mIDCash, mID);

   if ((fpw = _fsopen(mIDCash, "rb", _SH_DENYWR))) {
     while((fgets(mIDB, sizeof(mIDB), fpw)))
	 {
	   strtok(mIDB, "\n");
	   SPA_Decode(mIDB, pData);
       sprintf(str, "%s's Local group check. (Cache)=(\"%s\" vs. \"%s\") ", pID, pMGroup, pData);
       if (bDebug) printf("%s\n", str);
#ifdef ACCEPT_LOG_LEVEL3
       LEVEL_3_ACCEPTLOG(NULL, str);
#endif
       if (stricmp(pData, pMGroup) == 0) { // 2004.03.15 大小文字の違いなし
          *pbGroup = TRUE;
	   }
	   if (strstr(pData,".")) {
		 *pbResult = TRUE;
	   }
       nCnt++;
	 }
	 fclose(fpw);
   }
   strtok(mID, ".");
   ReleaseIDChche(mID);
   return nCnt;
}

void GetNumIDCashList(char *pID, char *pData, char *pName, int nEnt)
{
#ifdef ACCEPT_LOG_LEVEL3
	char str[LOG_BUFFER];
#endif
  FILE    *fpr, *fpw;
  char    *p;
  char    mIDCash[512];
  char    mIDB[256];
  char    mID[512];
  char    mGrp[256];
  int     nCnt = 0;

   if (nIDCashLiveTime == 0) // 更新時間が設定されていない場合は設定しない
	 return;
   if (strlen(pID)==0) 
     return;
   strcpy(mIDB, pID);
   translateUue2Base64(mIDB, strlen(mIDB), mID);
   strcat(mID, ".");
   strcat(mID, pName);

#ifdef ACCEPT_LOG_LEVEL3
   sprintf(str, "[          ] [%s] CountIDCashList=[%s]\n", pID, pName);
   LEVEL_3_ACCEPTLOG(NULL, str);
#endif

#ifdef REGTOFILE
   sprintf(mIDCash,"%s\\" ADCACHE "\\", mMailSpoolDir);
   _mkdir(mIDCash);         // cashフォルダ作成
   if (nClustering) {
     sprintf(mIDCash,"%s\\" ADCACHE "\\%s\\", mMailSpoolDir, mComName);
   } else {
#endif
     sprintf(mIDCash,"%s\\" ADCACHE "\\", mMailSpoolDir);
#ifdef REGTOFILE
   }
#endif

   _mkdir(mIDCash);         // cashフォルダ作成
   strcat(mIDCash, mID);

   mGrp[0] = '\x0';
   if ((fpw = _fsopen(mIDCash, "rb", _SH_DENYWR))) {
	 for (nCnt = 0; nCnt <= nEnt; nCnt++)
	 {
       if (fgets(mID, sizeof(mID), fpw))
	   {
	     strtok(mID, "\n");
	     SPA_Decode(mID, mGrp);
	   }
	 }
	 strcpy(pData, mGrp);
	 fclose(fpw);
   }
   if ((p = strrchr(mIDCash, '\\')))
   {
     strtok(p++, ".");
     ReleaseIDChche(p);
   }
}

BOOL GetIDCashList(char *pID, char *pData, char *pName, BOOL *pResult)
{
#ifdef ACCEPT_LOG_LEVEL3
	char str[LOG_BUFFER];
#endif
  FILE    *fp;
  char    *fsts;
  char    *p;
  char    mIDCash[512];
  char    mIDB[256];
  char    mID[512];
  char    mData[512] = "";
  BOOL    bSts = FALSE; //ゲートウェイリストに載っているか否か

   if (strlen(pID)==0) 
     return bSts;
   strcpy(mIDB, pID);
   translateUue2Base64(mIDB, strlen(mIDB), mID);
   if (!ReserveIDChche(mID)) // ロックファイルがないなら処理可能
   {
	 return bSts;
   }
   strcat(mID, ".");
   strcat(mID, pName);

#ifdef REGTOFILE
      if (nClustering) {
        sprintf(mIDCash,"%s\\" ADCACHE "\\%s\\%s", mMailSpoolDir, mComName, mID);  // cashフォルダ
	  } else {
#endif
   sprintf(mIDCash,"%s\\" ADCACHE "\\%s", mMailSpoolDir, mID);  // cashフォルダ
#ifdef REGTOFILE
	  }
#endif
   if ((bSts = IDLiveTime(mIDCash))) {
     if ((fp = _fsopen(mIDCash, "rb", _SH_DENYWR))) {
	   mID[0] = '\x0';
       if ((fsts = fgets(mID, sizeof(mID)-1, fp)))
         strtok(mID, "\n");
	   fclose(fp);
   	   SPA_Decode(mID, mData);
	   if (!strcmp(pName, "hm") ||
		   //!strcmp(pName, "gp") ||
		   !strcmp(pName, "fn") )
	   {
		 *pResult = (mData[0]  ? TRUE : FALSE);
		 strcpy(pData, (mData[0]  ? mData : ""));
	   } else {
	     *pResult = !strcmp(mData, pData);
	   }
	   bSts = TRUE;
	 } else {
	   bSts = FALSE;
	 }
	 /*
   } else {
	 _unlink(mIDCash);
     // IDキャッシュデータの削除でハングする
     while ((fp = _fsopen(mIDCash, "rb", _SH_DENYWR))) {
	   fclose(fp);
       if (bServiceTerminating)
         break;
	   _sleep(WAIT_TIME);
	 }
	*/
   }
   if ((p = strrchr(mIDCash, '\\')))
   {
     strtok(p++, ".");
     ReleaseIDChche(p);
   } 
#ifdef ACCEPT_LOG_LEVEL3
   sprintf(str, "[          ] [%s] GetIDCashList=[%s] status=%s\n", pID, pName, (bSts ? "Hit" : "Fail"));
   LEVEL_3_ACCEPTLOG(NULL, str);
#endif
   return bSts;
}

void DelIDCashList(char *pID, char *pName)
{
#ifdef ACCEPT_LOG_LEVEL3
	char str[LOG_BUFFER];
#endif
  char    mIDCash[512];
  char    mIDB[256];
  char    mID[512];
  FILE    *fp;

   if (strlen(pID)==0) 
     return;
   strcpy(mIDB, pID);
   translateUue2Base64(mIDB, strlen(mIDB), mID);
   strcat(mID, ".");
   strcat(mID, pName);

#ifdef REGTOFILE
      if (nClustering) {
        sprintf(mIDCash,"%s\\" ADCACHE "\\%s\\%s", mMailSpoolDir, mComName, mID);  // cashフォルダ
	  } else {
#endif
   sprintf(mIDCash,"%s\\" ADCACHE "\\%s", mMailSpoolDir, mID);  // cashフォルダ
#ifdef REGTOFILE
	  }
#endif
/////////////////////////////////////////////////////////////////
   _unlink(mIDCash);
    // ADキャッシュデータの削除でハングする
    while ((fp = _fsopen(mIDCash, "rb", _SH_DENYWR))) {
	  fclose(fp);
      if (bServiceTerminating)
        break;
	   _sleep(WAIT_TIME);
#ifdef UPDATE_20071129A // MXキャッシュファイルが削除されない場合の対策
       _unlink(mIDCash);
#endif
	}
/////////////////////////////////////////////////////////////////
#ifdef ACCEPT_LOG_LEVEL3
   sprintf(str, "[          ] [%s] DelIDCashList()\n", pID);
   LEVEL_3_ACCEPTLOG(NULL, str);
#endif
}

//ADキャッシュを全て削除する
void CleanIDCashList(void) {
#ifdef ACCEPT_LOG_LEVEL3
	char str[LOG_BUFFER];
#endif
  HANDLE             hF, hFile;
  WIN32_FIND_DATA    FD;
  BOOL               bFile = TRUE;
  char    mIDCash[512], mCash[512];
  FILE    *fp;

#ifdef REGTOFILE
      if (nClustering) {
        sprintf(mIDCash,"%s\\" ADCACHE "\\%s\\*", mMailSpoolDir, mComName);  // cashフォルダ
	  } else {
#endif
   sprintf(mIDCash,"%s\\" ADCACHE "\\*", mMailSpoolDir);  // cashフォルダ
#ifdef REGTOFILE
	  }
#endif
     /////////////////////////////////////////////////////////////////
     hF = FindFirstFile(mIDCash, &FD);
     if (hF != INVALID_HANDLE_VALUE) {
       bFile = TRUE;
       while (bFile) {
	     if (!(!stricmp(FD.cFileName, ".") ||
	           !stricmp(FD.cFileName, ".."))) {
#ifdef REGTOFILE
           if (nClustering) {
             sprintf(mCash,"%s\\" ADCACHE "\\%s\\%s", mMailSpoolDir, mComName, FD.cFileName);  // cashフォルダ
		   } else
#endif
		   {
             sprintf(mCash,"%s\\" ADCACHE "\\%s", mMailSpoolDir, FD.cFileName);  // cashフォルダ
		   }
           _unlink(mCash);
#ifdef ACCEPT_LOG_LEVEL3
           sprintf(str, "[          ] [%s] CleanIDCashList()\n", mCash);
           LEVEL_3_ACCEPTLOG(NULL, str);
#endif
		 }
         bFile = FindNextFile( hF, &FD);
	   }; 
       FindClose( hF ); 
	 }
}
#endif

#ifdef UPDATE_20160113 // 認証時のロックアウト機能を追加
#ifdef UPDATE_20180819B // 認証成功時にロックアウトカウンタのリセット機能を追加
void AuthLockOut(char *pAddr, char  *pDir, BOOL bFlg) //, char *pUser)
#else
void AuthLockOut(char *pAddr, char  *pDir) //, char *pUser)
#endif
{
  FILE   *fp;
  CHAR   Path[512];
  BOOL   bUser = FALSE;
  unsigned int    i, nCnt = 0;
#ifdef UPDATE_20160114 // 接続拒絶用テーブルの指定に有効時間を設定可能にした。
  SYSTEMTIME st;
  FILETIME   ft;
  ULARGE_INTEGER *u1;
#endif
#ifdef UPDATE_20180817 // IPv6での接続時にロックアウトのカウントが出来るようにした。
  char *p, *q;
#ifdef IPv6
	CHAR    mAddr[INET6_ADDRSTRLEN]; // My Host Address IPv6
#else
	CHAR    mAddr[21];     // My Host Address xxx.xxx.xxx.xxx
#endif
#endif
#ifdef UPDATE_20190216 // IP毎の同時接続ロックアウト機能を追加
   struct _stati64 sfn;
   HANDLE          hFs;
   SYSTEMTIME      lts;
   FILETIME        fts;
   ULARGE_INTEGER *u2;
   BOOL     bRset = FALSE;
#endif

#ifdef UPDATE_20180817 // IPv6での接続時にロックアウトのカウントが出来るようにした。
	if (strchr(pAddr, ':'))
	{
	  strcpy(mAddr, pAddr);
	  p = mAddr;
      while ((q = strchr(p, ':')))
	  {
	    *q = '$';  // ':'を'$'に置換えてファイル名にする
	    p = q + 1;
	  }
      sprintf(Path, "%s%s.smtp", pDir, mAddr); 
	}
	else 
#endif
	{
      sprintf(Path, "%s%s.smtp", pDir, pAddr); 
	}
#ifdef UPDATE_20190216 // IP毎の同時接続ロックアウト機能を追加
      GetSystemTime(&st);
      SystemTimeToFileTime(&st, &ft);
      u1 = (ULARGE_INTEGER *)&ft;
      ///////////////
	  if (strstr(pDir, "iplock"))
	  {
	    u2 = (ULARGE_INTEGER *)&fts;
        hFs = CreateFile( (LPCTSTR)Path,
                          GENERIC_READ,
                          0,
                          NULL,
                          OPEN_EXISTING,
                          FILE_ATTRIBUTE_NORMAL, 
                          NULL);
        if (hFs != INVALID_HANDLE_VALUE) {
          GetFileTime(hFs, NULL, NULL, &fts); // 更新日時
          CloseHandle(hFs);
	      FileTimeToSystemTime(&fts, &lts);
#ifdef UPDATE_20190218 // IP毎の同時接続ロックアウト機能を強化（サンプル時間と拒絶時間の設定を追加）
          u2->QuadPart+=(unsigned __int64)(nIPLockOutSPTime * 10000000); // 秒単位で経過
#else
          u2->QuadPart+=(unsigned __int64)(nIPLockOutTime * 10000000); // 秒単位で経過
#endif
	      if ((unsigned __int64)u2->QuadPart < (unsigned __int64)u1->QuadPart)
		  {
		    bRset = TRUE;
		  }
		}
	  }
#endif
	for (i = 0; i < 2; i++)
	{
      if ((fp = fopen(Path, "rt")) )
	  {
        fscanf(fp, "%d", &nCnt);
        fclose(fp);
	    bUser = TRUE;
	  }
#ifdef UPDATE_20180819B // 認証成功時にロックアウトカウンタのリセット機能を追加
	  if (bFlg)
#endif
	    nCnt++;
#ifdef UPDATE_20190216 // IP毎の同時接続ロックアウト機能を追加
      if (bRset)
		nCnt = 0;
	  if (nCnt >= (strstr(pDir, "iplock") ? nIPLockOut : nAuthLockOut))
#else
      if (nCnt >= nAuthLockOut)
#endif
	  {
        if ((fp = fopen(mRejectPeerPFile, "at")))
		{
#ifdef UPDATE_20160114 // 接続拒絶用テーブルの指定に有効時間を設定可能にした。
          GetSystemTime(&st);
          SystemTimeToFileTime(&st, &ft);
          u1 = (ULARGE_INTEGER *)&ft;
#ifdef UPDATE_20190216 // IP毎の同時接続ロックアウト機能を追加
		  if (strstr(pDir, "iplock"))
		  {
            fprintf(fp, "%s\t%I64u\tiplock\n", pAddr, (unsigned __int64)u1->QuadPart);
		  } else
#endif
            fprintf(fp, "%s\t%I64u\n", pAddr, (unsigned __int64)u1->QuadPart);
#else
          fprintf(fp, "%s\n", pAddr);
#endif
          fclose(fp);
	      _unlink(Path);
		}
	  } else {
#ifdef UPDATE_20180819B // 認証成功時にロックアウトカウンタのリセット機能を追加
		if (bFlg)
		{
          if ((fp = fopen(Path, "wt")))
		  { 
            fprintf(fp, "%d", nCnt);
            fclose(fp);
	        bUser = TRUE;
		  }
		} else {
	      _unlink(Path);
	      bUser = TRUE;
		} 
#else
        if ((fp = fopen(Path, "wt")))
		{ 
          fprintf(fp, "%d", nCnt);
          fclose(fp);
	      bUser = TRUE;
		}
#endif
	  }
	  if (!bUser)
	  {
#ifdef UPDATE_20160128 // 存在しないアカウントへの認証失敗ロックアウト管理用フォルダを作成
        sprintf(Path, "%sauthlock\\%s.smtp", mMailSpoolDir, pAddr);
#else
        sprintf(Path, "%s%s.smtp", mMailBoxDir, pAddr); 
#endif
		nCnt = 0;
	  } else {
		break;
	  }
    } 
}
#endif

#ifdef UPDATE_20211210_IPV6PREFIX // IPv6アドレスプレフィックスに対応した。
BOOL Compear_IPv6_Addr(char *pSrcAddr, char *pCompAddr, char *pPrefix)
{
  int      mask;
  BOOL     bobf;
  char     *pmp;
  BOOL     bResult = FALSE;
  DWORD    slit=0, slitbit=0;
  struct in6_addr pip6, dip6, rip6;
  unsigned __int64 deip6[2], dsip6[2], ddip6[2];
			   
#ifdef VC6
  inet6_addr(pSrcAddr, &pip6);
  inet6_addr(pCompAddr, &dip6);
#else
  inet_pton(AF_INET6, pSrcAddr, &pip6);
  inet_pton(AF_INET6, pCompAddr, &dip6);
#endif
  memcpy((void *)&rip6, (void *)&dip6, sizeof(struct in6_addr));
  //接続元アドレス
  deip6[0] = (unsigned __int64)((unsigned __int64)pip6.s6_addr[0]*0x100000000000000+
                                (unsigned __int64)pip6.s6_addr[1]*0x1000000000000+
                                (unsigned __int64)pip6.s6_addr[2]*0x10000000000+
                                (unsigned __int64)pip6.s6_addr[3]*0x100000000+
                                (unsigned __int64)pip6.s6_addr[4]*0x1000000+
                                (unsigned __int64)pip6.s6_addr[5]*0x10000+
                                (unsigned __int64)pip6.s6_addr[6]*0x100+
                                (unsigned __int64)pip6.s6_addr[7]);
  deip6[1] = (unsigned __int64)((unsigned __int64)pip6.s6_addr[8]*0x100000000000000+
                                (unsigned __int64)pip6.s6_addr[9]*0x1000000000000+
                                (unsigned __int64)pip6.s6_addr[10]*0x10000000000+
                                (unsigned __int64)pip6.s6_addr[11]*0x100000000+
                                (unsigned __int64)pip6.s6_addr[12]*0x1000000+
                                (unsigned __int64)pip6.s6_addr[13]*0x10000+
                                (unsigned __int64)pip6.s6_addr[14]*0x100+
                                (unsigned __int64)pip6.s6_addr[15]);
  //下限アドレス
  dsip6[0] = (unsigned __int64)((unsigned __int64)dip6.s6_addr[0]*0x100000000000000+
                                (unsigned __int64)dip6.s6_addr[1]*0x1000000000000+
                                (unsigned __int64)dip6.s6_addr[2]*0x10000000000+
                                (unsigned __int64)dip6.s6_addr[3]*0x100000000+
                                (unsigned __int64)dip6.s6_addr[4]*0x1000000+
                                (unsigned __int64)dip6.s6_addr[5]*0x10000+
                                (unsigned __int64)dip6.s6_addr[6]*0x100+
                                (unsigned __int64)dip6.s6_addr[7]);
  dsip6[1] = (unsigned __int64)((unsigned __int64)dip6.s6_addr[8]*0x100000000000000+
                                (unsigned __int64)dip6.s6_addr[9]*0x1000000000000+
                                (unsigned __int64)dip6.s6_addr[10]*0x10000000000+
                                (unsigned __int64)dip6.s6_addr[11]*0x100000000+
                                (unsigned __int64)dip6.s6_addr[12]*0x1000000+
                                (unsigned __int64)dip6.s6_addr[13]*0x10000+
                                (unsigned __int64)dip6.s6_addr[14]*0x100+
                                (unsigned __int64)dip6.s6_addr[15]);
  ///上限アドレス
  ddip6[0] = (unsigned __int64)((unsigned __int64)rip6.s6_addr[0]*0x100000000000000+
                                (unsigned __int64)rip6.s6_addr[1]*0x1000000000000+
                                (unsigned __int64)rip6.s6_addr[2]*0x10000000000+
                                (unsigned __int64)rip6.s6_addr[3]*0x100000000+
                                (unsigned __int64)rip6.s6_addr[4]*0x1000000+
                                (unsigned __int64)rip6.s6_addr[5]*0x10000+
                                (unsigned __int64)rip6.s6_addr[6]*0x100+
                                (unsigned __int64)rip6.s6_addr[7]);
  ddip6[1] = (unsigned __int64)((unsigned __int64)rip6.s6_addr[8]*0x100000000000000+
                                (unsigned __int64)rip6.s6_addr[9]*0x1000000000000+
                                (unsigned __int64)rip6.s6_addr[10]*0x10000000000+
                                (unsigned __int64)rip6.s6_addr[11]*0x100000000+
                                (unsigned __int64)rip6.s6_addr[12]*0x1000000+
                                (unsigned __int64)rip6.s6_addr[13]*0x10000+
                                (unsigned __int64)rip6.s6_addr[14]*0x100+
                                (unsigned __int64)rip6.s6_addr[15]);
  // 上限アドレス算出
  bobf = FALSE;
  if ((mask = 128 - atoi((char *)(pPrefix+1))) < 0)
    mask = 0;
  if (mask < 64)
  {
    if ((unsigned __int64)(ddip6[1] +(unsigned __int64)((unsigned __int64)1<<mask)) < (unsigned __int64)ddip6[1])
	{
	  bobf = TRUE;
      ddip6[0]++; // オーバフロー
	}
	if (mask < 64)
      ddip6[1] +=(unsigned __int64)((unsigned __int64)1<<mask);
  } else{ // 64以上
	bobf = TRUE;
    ddip6[0] +=(unsigned __int64)((unsigned __int64)1<<(64-mask));
  }
  // 範囲内か比較
  if (dsip6[0] == deip6[0]) // 上位桁への繰上りが無いとき。
  {
    if (dsip6[1] <= deip6[1] && ((!bobf && ddip6[1] > deip6[1]) || bobf))
      bResult = TRUE;  // 範囲内と判定した。
  } else if (ddip6[0] == deip6[0]) // 上位桁への繰上りが有るとき。
  {
    if (ddip6[1] > deip6[1])
      bResult = TRUE;  // 範囲内と判定した。
  }

  return bResult;
}
#endif

#ifdef UPDATE_20240205 // 承認依頼メールで１行当たりの文字数を正規化
void LineNormalization(FILE *fp, int nMax, char *pLine)
{
   int n1, n2, n3;
   char c;

   n1 = strlen(pLine) / nMax;
   n2 = nMax * n1;
   for (n3 = 1; n3 <= n1; n3++) // 一定カラムごとに改行(CLRF)を追加
   {
     c = (char) *(pLine+(nMax*n3));
     *(pLine+(nMax*n3)) = '\x0';
     fprintf(fp, "%s\n", (char *)(pLine+(nMax*(n3-1))));
	 *(pLine+(nMax*n3)) = c;
   }
   if (strlen(pLine) % nMax) // 残りに改行(CLRF)
     fprintf(fp, "%s", (char *)(pLine+(nMax*(n3-1))));
}
#endif

#ifdef UPDATE_20260603 // ログ出力の排他処理化を追加
HANDLE g_hMutexAcceptLog = NULL;
HANDLE g_hMutexInLog = NULL;
HANDLE g_hMutexRecivedLog = NULL;
HANDLE g_hMutexMaildbLog = NULL;
HANDLE g_hMutexApprovalLog = NULL;

#ifdef UPDATE_20260606 // ログの書き込み信頼性向上対策
/*
  ログの書き込み用ファイルディスクプリタを新規で開いた後保持して
　一定時間間隔（１時間間隔）内の書込みは実行し時間を超えた時の書き
　込みはクローズ再オープンし、書き込み時は、MUTEXで排他する。
*/
FILE *fpAcceptLog = NULL;
FILE *fpInLog = NULL;
FILE *fpRecivedLog = NULL;
FILE *fpMaildbLog = NULL;
FILE *fpApprovalLog = NULL;

char mDTAcceptLog[16] = "";
char mDTInLog[16] = "";
char mDTRecivedLog[16] = "";
char mDTMaildbLog[16] = "";
char mDTApprovalLog[16] = "";
#endif

/* 初期化：アプリ起動時に1回だけ呼ぶ */
void InitLogger()
{
    g_hMutexAcceptLog = CreateMutex(NULL, FALSE, TEXT("Local\\MyLogMutexAcceptlog"));
    if (bDebug) printf("g_hMutexAcceptLog = %08x\n", g_hMutexAcceptLog);
    g_hMutexInLog = CreateMutex(NULL, FALSE, TEXT("Local\\MyLogMutexInlog"));
    if (bDebug) printf("g_hMutexInLog = %08x\n", g_hMutexInLog);
    g_hMutexRecivedLog = CreateMutex(NULL, FALSE, TEXT("Local\\MyLogMutexRecivedlog"));
    if (bDebug) printf("g_hMutexRecivedLog = %08x\n", g_hMutexRecivedLog);
	g_hMutexMaildbLog = CreateMutex(NULL, FALSE, TEXT("Local\\MyLogMutexMaildblog"));
    if (bDebug) printf("g_hMutexMaildbLog = %08x\n", g_hMutexMaildbLog);
	g_hMutexApprovalLog = CreateMutex(NULL, FALSE, TEXT("Local\\MyLogMutexApprovallog"));
    if (bDebug) printf("g_hMutexApprovalLog = %08x\n", g_hMutexApprovalLog);
}

/* 終了処理：アプリ終了時に1回だけ呼ぶ */
void CloseLogger()
{
    if (g_hMutexAcceptLog) {
        CloseHandle(g_hMutexAcceptLog);
        g_hMutexAcceptLog = NULL;
    }
    if (g_hMutexInLog) {
        CloseHandle(g_hMutexInLog);
        g_hMutexInLog = NULL;
    }
    if (g_hMutexRecivedLog) {
        CloseHandle(g_hMutexRecivedLog);
        g_hMutexRecivedLog = NULL;
    }
    if (g_hMutexMaildbLog) {
        CloseHandle(g_hMutexMaildbLog);
        g_hMutexMaildbLog = NULL;
    }
    if (g_hMutexApprovalLog) {
        CloseHandle(g_hMutexApprovalLog);
        g_hMutexApprovalLog = NULL;
    }
}

/* 排他付きログ出力 サンプル*/
/*
void WriteLog(const char* fmt, ...)
{
    WaitForSingleObject(g_hMutexLog, INFINITE);  // 排他開始

    FILE* fp = fopen("app.log", "a");
    if (fp) {
        va_list args;
        va_start(args, fmt);
        vfprintf(fp, fmt, args);
        fprintf(fp, "\n");
        va_end(args);
        fclose(fp);
    }

    ReleaseMutex(g_hMutexLog);  // 排他終了
}
*/

#ifdef UPDATE_20260606 // ログの書き込み信頼性向上対策
extern char   mMailApprovalDir[];

FILE *OpenCloseLog(FILE *pfp, char *pDT, char *plog, char *pCom, SYSTEMTIME lt)
{
   char mdata[16];
   char mFn[256];

   sprintf(mdata, "%02d%02d%02d%02d%02d", (lt.wYear%100), lt.wMonth, lt.wDay, lt.wHour, lt.wMinute);
   mdata[9] = '\x0'; // 10桁で分単位 9桁で10分 8桁で時間
/*
if (bDebug && !strcmp(plog, "inlog")) 
{
	printf("-------\nOpenCloseLog(), start\n-------\n");
    printf("pfp=%08x\n", pfp);
    printf("OpenCloseLog(), start pfp=%08x\n", pfp);
    printf("pDT=%s\n", pDT);
	printf("plog=%s\n", plog);
	printf("pCom=%s\n", pCom);
    printf("mdata=%s\n", mdata);
}
*/
   if (*pDT == 0 || strcmp(mdata, pDT)) // 不一致なら
   {
	 int retry = 0;
	 if (pfp) 
	 {
	   fflush(pfp);
	   fclose(pfp);
 	   pfp = NULL;
	 }
	 /////
 	   if (!strcmp(plog, "log"))
	   {
         sprintf(mFn, "%s\\%s\\%02d%02d%02d.txt", mMailSpoolDir, mMailApprovalDir, (lt.wYear%100), lt.wMonth, lt.wDay);
	   } else {
#ifdef REGTOFILE
         if (nClustering) {
           sprintf(mFn, "%s\\%s\\%s\\%02d%02d%02d.log", mMailSpoolDir, plog, pCom, (lt.wYear%100), lt.wMonth, lt.wDay);
		 } else {
#endif
           sprintf(mFn, "%s\\%s\\%02d%02d%02d.log", mMailSpoolDir, plog, (lt.wYear%100), lt.wMonth, lt.wDay);
#ifdef REGTOFILE
	  }
#endif

       }
       while (pfp == NULL && retry < 3) {
         pfp = fopen(mFn, "at");
		 //pfp = _fsopen(mFn, "at", _SH_DENYNO);
         if (pfp)
			break;
		 Sleep(10); // AVスキャン完了を少し待つ
         retry++;
	   }
	   if (pfp) // 開けたらな日付をコピー
	   {
		 strcpy(pDT, mdata);
	   }
	 ////
   }
/*
if (bDebug && !strcmp(plog, "inlog")) 
{
    printf("pfp=%08x\n", pfp);
	printf("OpenCloseLog(), end\n-------\n");
}
*/
   return pfp;
}
#endif
#endif