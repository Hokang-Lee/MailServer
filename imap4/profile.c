////////////////////////////////////////////////////////////
// Profile.c Copyright K.kawakami
// Get profile key and data.
////////////////////////////////////////////////////////////
#include "imap4.h"
#include <windows.h>
#include <stdio.h>
#include <share.h>
#include <sys/types.h> 
#include <sys/stat.h>

//#include "profile.h"

//#define PROFILE_ROOT_TREE "SOFTWARE\\EMWAC\\%s\\"
#define PROFILE_ROOT_TREE "%s\\"
#define MAX_VALUE_NAME    128

#define MAX_REG_RCPT      384

#ifdef CLUSTERING
extern BOOL   nClustering;
#endif

extern DWORD nTMOut;
extern DWORD nSendTMO;
extern BOOL  bDebug;
extern char  mMailExtension[];
extern BOOL  bCountLock;
extern DWORD nMailInBoxSize;
extern CHAR  mMailBoxDir[];
extern BOOL  bListenMode;
extern char  mLocalDomain[];
extern char  mMailSpoolDir[128];
extern BOOL  bLog;
#ifdef Y2038_BUG
extern char mMonth[12][4];
extern char mWeek[7][4];
#endif
#ifdef USE_SSL
extern BOOL  bUsedSSL;
#endif
#ifdef V4
extern char  mUseTimeFile[];
#endif
#ifdef CLUSTERING
extern char   mComName[];
#endif
#ifdef UPDATE_20070521 // OSの予約語対策
extern CHAR   mReservedWords[];
#endif
extern BOOL   bServiceTerminating;
#ifdef UPDATE_20140610 // IDLEコマンド実行中の無通信タイムアウト時間をデフォルトで30分にした。
extern int nIDLETMOut; // IDLE実行中の無通信タイマーデフォルト30分
#endif
#ifdef UPDATE_20141220 // 操作フォルダをアクセス中のセッションに変化をリアルタイムに同報するための対策
extern unsigned int nPutLockCnt; // 同一フォルダの操作情報を接続中のセッションに同報する排他用フラグチェックのタイムアウト値　デフォルト 30秒
#endif
#ifdef ADD_IDCACHE
extern BOOL bServiceTerminating;
extern int  nIDCashLiveTime; // ADキャッシュ利用有効時間(秒単位)
extern char mComName[];
#endif
#ifdef UPDATE_20151225 // 試作：UID値によるファイル範囲指定のチューニング
extern int  bQueryFileArea; // UID値範囲指定時の処理方法選択 デフォルト FALSE:従来方式 TRUE:範囲指定方式
#endif
#ifdef UPDATE_20151226 // 当分封印：IDLE処理の修正 (同一フォルダにアクセス中のセッションに変化が伝わらない不具合)
extern CRITICAL_SECTION g_csIdleMess;
#endif
#ifdef UPDATE_20160113 // 認証時のロックアウト機能を追加
extern DWORD nAuthLockOut; // ロックアウトまでの回数 デフォルト 0:無効
extern char  mRejectPeerPFile[];
#endif
#ifdef UPDATE_20190216 // IP毎の同時接続ロックアウト機能を追加
extern DWORD nIPLockOut; // IPロックアウトまでの回数 デフォルト 0:無効
extern DWORD nIPLockOutTime; // ロックアウト時間 デフォルト 0:無限 分単位
#ifdef UPDATE_20190218 // IP毎の同時接続ロックアウト機能を強化（サンプル時間と拒絶時間の設定を追加）
extern DWORD nIPLockOutSPTime; // IPロックアウトまでのサンプリング時間 デフォルト 0:無限 秒単位
#endif
#endif
#ifdef UPDATE_20231211 // 通信エラーが事前の送信で発生してる場合送信を行わないでセッションを終了するようにした。
extern BOOL  bSendErrEscape; //送信エラー時中断 1:する(デフォルト) 0:しない
#endif
#ifdef UPDATE_20240728 // サブネットマスクの範囲指定の高速化
extern BOOL bIPcmpmaskengin;
#endif

#ifdef ACCEPT_LOG_LEVEL3
void LEVEL_3_ACCEPTLOG(PCLIENT_CONTEXT lpClientContext, char *mess);
#endif
#ifdef Y2038_BUG
void gettime(SYSTEMTIME *ltime, char *mt);
#else
void gettime(time_t *ltime, char *mt);
#endif
void translateUue2Base64(char *line, int inlen, char *newline);
int Base64DecodeLine(char * pszLine, int nLineLen, unsigned char * uchBuffer, int nBufferLen);
int WSAGetLastErrorName(void);
BOOL put_reply_flash(PCLIENT_CONTEXT lpClientContext, BOOL bOnLog);
int SPA_FileDecode(char *pSrc, char *pDest, char cMask);

#ifdef UPDATE_20190510 // NTFS以外のファイルシステムからのファイル一覧取得可能にする対策（一覧ソート）を追加
int fn_sort( const void * a , const void * b ) 
{
  WIN32_FIND_DATA *lpa, *lpb;
#ifdef UPDATE_20191002 // ソート対象日付を作成日付から更新日付に変更した。
  int nResult;
  ULARGE_INTEGER *ua, *ub;
#endif

  lpa = (WIN32_FIND_DATA *)a;
  lpb = (WIN32_FIND_DATA *)b;
  // 引数はvoid*型と規定されているのでint型にcastする 
#ifdef UPDATE_20191002 // ソート対象日付を作成日付から更新日付に変更した。
  ua = (ULARGE_INTEGER *)&lpa->ftLastWriteTime; //->ftCreationTime;
  ub = (ULARGE_INTEGER *)&lpb->ftLastWriteTime; //->ftCreationTime;
  nResult = (ua->QuadPart > ub->QuadPart ? 0 : (ua->QuadPart == ub->QuadPart ? stricmp(lpa->cFileName, lpb->cFileName) : 1));
//printf("%s(%I64u) / %s(%I64u) / %d\n", lpa->cFileName, ua->QuadPart, lpb->cFileName, ub->QuadPart, nResult);
  return nResult;
#else
  return stricmp(lpa->cFileName, lpb->cFileName);
#endif
}

WIN32_FIND_DATA * FindFirstFileSort(LPCTSTR pDir, WIN32_FIND_DATA* pFD, DWORD *pNo)
{
  WIN32_FIND_DATA  *pfd;
  HANDLE hSearch;
  LPTSTR lpBuff = NULL;
  DWORD  nSize;

  pfd=NULL;
  *pNo = 0;
  if ((hSearch = FindFirstFile(pDir, pFD)) != INVALID_HANDLE_VALUE)
  {
    do {
      if (pFD->dwFileAttributes & (FILE_ATTRIBUTE_DIRECTORY |
                                         FILE_ATTRIBUTE_SYSTEM |
                                         FILE_ATTRIBUTE_HIDDEN) ) {
            continue;   // ファイル以外はSkip non-normal files
      }
      if (strlen(pFD->cFileName) != 21) {
        continue;    // ファイル名の長さが違うものは無視
      }
      nSize = sizeof(WIN32_FIND_DATA)*(*pNo+1);
      if (*pNo == 0)
      {
        pfd=malloc(nSize);
      } else {
        pfd=realloc(pfd, nSize);
      }
      if (pfd) 
      {
        memcpy(pfd+*pNo, pFD, sizeof(WIN32_FIND_DATA));
        (*pNo)++;
      }
    } while (FindNextFile(hSearch, pFD));
    FindClose(hSearch);
    if (pfd)
    {
      qsort(( void * )pfd , *pNo,  sizeof( WIN32_FIND_DATA ), fn_sort );
      memcpy(pFD, pfd, sizeof(WIN32_FIND_DATA));
    }
  }
  return pfd;
}

BOOL FindNextFileSort(WIN32_FIND_DATA *pFD, WIN32_FIND_DATA *pfd)
{
   memcpy(pFD, (WIN32_FIND_DATA *)pfd, sizeof(WIN32_FIND_DATA));
}

void FindCloseSort(WIN32_FIND_DATA *pfd)
{ 
  if (pfd)
    free(pfd);
}
#endif

#if defined(USE_SSL) && defined(UPDATE_20190426) // 利用する暗号アルゴリズムの組み合わせオプション 
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
#ifdef UPDATE_20060116  // ユーザルートより上位のフォルダにアクセスできる脆弱性３
BOOL FolderRight2(PImap4Context pContext, char *pTarget) {
  char *o, *p, *q, mSel[256];
  int  nPtr[2], nDep, nCur, nBack;
  // ROOTフォルダ取得
  GetBaseDirectory(pContext->mRootDir, mMailBoxDir, pContext->mUSER, pContext->MyAddr);
  strncpy(mSel, pContext->mSelectDir, sizeof(mSel)-1);
  mSel[sizeof(mSel)-1] = '\x0';
  o = (char *)(mSel + strlen(pContext->mRootDir));
  p = mSel;
  while(*p) {
    if (*p == '\\')  // '\'を'/'に統一
	  *p = '/';
	p++;
  }
  nDep = nCur = nBack = 0; // 初期化
  while(*o) {
	if (*o == '.') {
	  if (*(o+1) == '/') { // 現階層
		o+=2;
        nCur++;
	  } else if (*(o+1) == '.') {
		if (*(o+2) == '/') {  // 前階層
		  o+=3;
          nBack++;
		}
	  } else {
	    o++;
	  }
	} else if (*o == '/') { // 次階層
	  o++;
      nDep++;
	} else {
	  o++;
      nCur++;
	}
  }
  nPtr[0] = nDep - nBack; // 階層位置
  ///////////////////////////////////////
  strncpy(mSel, pTarget, sizeof(mSel)-1);
  mSel[sizeof(mSel)-1] = '\x0';
  o = (char *)(mSel + strlen(pContext->mRootDir));
  p = mSel;
  while(*p) {
    if (*p == '\\')  // '\'を'/'に統一
	  *p = '/';
	p++;
  }
  nDep = nCur = nBack = 0; // 初期化
  while(*o) {
	if (*o == '.') {
	  if (*(o+1) == '/') { // 現階層
		o+=2;
        nCur++;
	  } else if (*(o+1) == '.') {
		if (*(o+2) == '/') {  // 前階層
		  o+=3;
          nBack++;
		}
#ifdef UPDATE_20061217 // 特殊なトークン".. *"でフォルダ階層指定を行うと処理から抜けられない不具合
		else
		{
		  o+=2;
          nBack++;
		}
#endif
	  } else {
	    o++;
	  }
	} else if (*o == '/') { // 次階層
	  o++;
      nDep++;
	} else {
	  o++;
      nCur++;
	}
  }
  nPtr[1] = nDep + nPtr[0] - nBack; // 階層位置
  ///////////////////////////////////////
  if (nPtr[1] < 0 || 
	  (nDep <= nBack && nBack > 0)) // ユーザルートより上位
	return FALSE;
  else                   // ユーザルート内
    return TRUE;
}
#endif

#ifdef UPDATE_20050530  // ユーザルートより上位のフォルダにアクセスできる脆弱性
BOOL FolderRight(PImap4Context pContext, char *pTarget) {
  char *o, *p, *q, mSel[256];
  // ROOTフォルダ取得
  GetBaseDirectory(pContext->mRootDir, mMailBoxDir, pContext->mUSER, pContext->MyAddr);
  strncpy(mSel, pTarget, sizeof(mSel)-1);
  mSel[sizeof(mSel)-1] = '\x0';
  o = (char *)(mSel + strlen(pContext->mRootDir));
  p = mSel;
  while(*p) {
    if (*p == '\\')  // '\'を'/'に統一
	  *p = '/';
	p++;
  }
  /////////////////////////////////////
#ifdef UPDATE_20050531_A // ユーザルートより上位のフォルダにアクセスできる脆弱性その２
  if (strstr(o, "../"))
	return FALSE;
#else
  p = o;
  while((q = strstr(p, "../")) ) {  /// 上位階層の指定をカウント
    if (q >= o)
	  return FALSE;
	p=q+3;
  }
#endif
  return TRUE;
}
#endif

BOOL put_reply(PCLIENT_CONTEXT lpClientContext, BOOL bFlash, BOOL bOnLog) {
  PImap4Context   pContext = &lpClientContext->Context;

  if ((sizeof(pContext->mCash)-1) > (strlen(pContext->mCash)+strlen(pContext->mMessages))) {
    strcat(pContext->mCash, pContext->mMessages);
#ifdef UPDATE_20150705 // 複数の命令が１度に受信された時、応答結果に無効なデータが含まれることがある
	pContext->mMessages[0] = '\x0';
#endif
    if (bFlash)
	{
	  return (put_reply_flash(lpClientContext, bOnLog)); // 残りも送信
	}
  } else {
    put_reply_flash(lpClientContext, bOnLog); // 溜まったデータ送信
    strcpy(pContext->mCash, pContext->mMessages);
#ifdef UPDATE_20150705 // 複数の命令が１度に受信された時、応答結果に無効なデータが含まれることがある
	pContext->mMessages[0] = '\x0';
#endif
    if (bFlash)
	{
	  return (put_reply_flash(lpClientContext, bOnLog)); // 残りも送信
	}
  }
  return TRUE;
}

BOOL put_reply_flash(PCLIENT_CONTEXT lpClientContext, BOOL bOnLog)
{
  char mec[128];
  int  err;
  PImap4Context   pContext = &lpClientContext->Context;
#ifdef UPDATE_20101224 // SOCKET送信でのERROR=183に対する対策
  int  i;
#endif

#ifdef UPDATE_20231211 // 通信エラーが事前の送信で発生してる場合送信を行わないでセッションを終了するようにした。
	if (lpClientContext->Context.LastError != 0 && bSendErrEscape)
	  return FALSE;
#endif
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
  	  err = SSL_write(lpClientContext->ssl, pContext->mCash, strlen(pContext->mCash));
//	  lpClientContext->ssl->rbio->bOnMemory  = 1; // TRUE;
  } else
#endif
#ifdef UPDATE_20101224 // SOCKET送信でのERROR=183に対する対策
  for (i = 0; i < 30; i++)
  {
#endif
    err = send(lpClientContext->Socket, pContext->mCash, strlen(pContext->mCash) /*strlen(pContext->mMessages)*/, 0 );
#ifdef UPDATE_20101224 // SOCKET送信でのERROR=183に対する対策
    if (err != 183 ||
		bServiceTerminating)
	{
	  break;
	}
	_sleep(10*i);
  } 
#endif
  if (bOnLog) {
    if (bDebug) printf("[%08x] SEND[%08x] -> [%s]\n", lpClientContext, lpClientContext->Socket, pContext->mCash /*pContext->mMessages*/);
#ifdef ACCEPT_LOG_LEVEL3
    LEVEL_3_ACCEPTLOG(lpClientContext, pContext->mCash );
#endif
  }
  pContext->mCash[0] = '\x0'; //キャッシュクリア
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
#ifdef UPDATE_20231211 // 通信エラーが事前の送信で発生してる場合送信を行わないでセッションを終了するようにした。
	lpClientContext->Context.LastError = err;
#endif
    return FALSE;
  }
#ifdef UPDATE_20231211 // 通信エラーが事前の送信で発生してる場合送信を行わないでセッションを終了するようにした。
  lpClientContext->Context.LastError = 0;
#endif
  return TRUE;
}

BOOL get_reply(PCLIENT_CONTEXT lpClientContext, BOOL bOnLog, CHAR *pCmd) {
  int  n, l, remain;
  char *p, *p2, *p3;
  char mec[128];
  int  err;
  DWORD nx;
  PImap4Context    pContext = &lpClientContext->Context;
//#ifdef UPADTE_20040202
   int    ns;
   fd_set ds;
   struct timeval tmo;
//#endif

    //if (bDebug) printf("[%08x] get_reply() Start\n", lpClientContext);
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
    //if (bDebug) printf("[%08x] get_reply():remain=%d\n", remain);
#ifdef UPDATE_20050518
#ifdef UPDATE_20140610 // IDLEコマンド実行中の無通信タイムアウト時間をデフォルトで30分にした。
 	  if (nTMOut || pContext->bIDLE && nIDLETMOut) // 0 ならタイムアウトしない。
#else
 	  if (nTMOut)  // 0 ならタイムアウトしない。
#endif
	  {
#ifdef UPDATE_20140610 // IDLEコマンド実行中の無通信タイムアウト時間をデフォルトで30分にした。
        tmo.tv_sec = (pContext->bIDLE ? nIDLETMOut : (int)nTMOut); // 1秒単位
#else
        tmo.tv_sec = (int)nTMOut; // 1秒単位
#endif
 	    tmo.tv_usec = 0;            // microseconds 
	    FD_ZERO(&ds);
	    FD_SET(lpClientContext->Socket, &ds);
#ifdef UPDATE_20060324 // Thunderbirdで添付を含むSSL送信で問題をおこす。
#ifdef USE_SSL
#ifdef USE_STARTTLS
	        if (( lpClientContext->Context.bUsedSSL || 
                 !bListenMode && bUsedSSL) &&
		          lpClientContext->ssl) 
#else
	        if (( bListenMode && lpClientContext->Context.bUsedSSL || 
                 !bListenMode && bUsedSSL) &&
		          lpClientContext->ssl) 
#endif
			{
			  // SSLでは、select処理させない
	          l = SSL_read(lpClientContext->ssl, &lpClientContext->Buffer[n], (remain >= 2048 ? 2048 : remain)); // opensslサイズの限界のため sizeof(lpClientContext->Buffer)-1-n);
		      if (l == -1)
		        l = 0;
			} else {
#endif
//if (bDebug) printf("[%08x] get_reply():select() Start\n", lpClientContext);
	          if ((ns = select(lpClientContext->Socket, &ds, (fd_set *)NULL, (fd_set *)NULL, &tmo)) > 0) {
//if (bDebug) printf("[%08x] get_reply():select()\n", lpClientContext);
                l = recv(lpClientContext->Socket, &lpClientContext->Buffer[n], remain /*sizeof(lpClientContext->Buffer)-1-n*/, 0);
//if (bDebug) printf("[%08x] get_reply():recv() l=%lu\n", lpClientContext, l);
			  } else {
                l = SOCKET_ERROR;
			  }
#ifdef USE_SSL
		  }
#endif
#else
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
		    if (l == -1)
		     l = 0;
		  } else
#endif
            l = recv(lpClientContext->Socket, &lpClientContext->Buffer[n], remain /*sizeof(lpClientContext->Buffer)-1-n*/, 0);
		} else {
          l = SOCKET_ERROR;
		}
#endif
	  } else {
#ifdef USE_SSL
#ifdef USE_STARTTLS
	    if (( lpClientContext->Context.bUsedSSL || 
             !bListenMode && bUsedSSL) &&
		     lpClientContext->ssl) 
#else
	    if (( bListenMode && lpClientContext->Context.bUsedSSL || 
             !bListenMode && bUsedSSL) &&
		     lpClientContext->ssl) 
#endif
		{
  	      l = SSL_read(lpClientContext->ssl, &lpClientContext->Buffer[n], (remain >= 2048 ? 2048 : remain)); // opensslサイズの限界のため sizeof(lpClientContext->Buffer)-1-n);
		  if (l == -1)
		    l = 0;
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
#ifdef UPDATE_20140610 // IDLEコマンド実行中の無通信タイムアウト時間をデフォルトで30分にした。
  	    if (nTMOut || pContext->bIDLE && nIDLETMOut) // 0 ならタイムアウトしない。
#else
 		if (nTMOut)  // 0 ならタイムアウトしない。
#endif
		{
#ifdef UPDATE_20140610 // IDLEコマンド実行中の無通信タイムアウト時間をデフォルトで30分にした。
          tmo.tv_sec = (pContext->bIDLE ? nIDLETMOut : (int)nTMOut); // 1秒単位
#else
          tmo.tv_sec = (int)nTMOut; // 1秒単位
#endif
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
	    break;      // これを有効にするとSSLでCPU負荷１００%になる場合がある。2004.01.21
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
    //if (bDebug) printf("[%08x] get_reply():lpClientContext->Buffer[%lu+%lu]\n", lpClientContext, n, l);

#ifdef UPDATE_20060117 // 強制切断でCPUが１００％状態になる
      if (!stricmp(pCmd, "append") &&
		  *lpClientContext->Buffer == '\x03') // ブレーク発生で終了
        return FALSE;
#endif
	//printf("lpClientContext->Buffer[%d+%d]=%s\n", n, l, lpClientContext->Buffer);
       p = strchr(lpClientContext->Buffer,'\n'); // ライン終了検出
	  if (p) {
		p2 = strchr(lpClientContext->Buffer,' '); // 命令検出
		if (p2) {
		  while (*p2 == ' ')
		    p2++;
		  if (strnicmp(p2, "append", 6) &&
			  stricmp(pCmd, "append") ) // APPEND以外は引き続きデータがある場合の対応をする。
		  {
			  //!strnicmp(p2, "search", 6) ||
			  //!strnicmp(p2, "uid search", 10)) {
			if ((p3 = strrchr(p2, '{'))) { // 引き続きデータがある場合
#ifdef UPDATE_20211027 // '{'が含まれたトークンを次通信にトークンが続く判定の改善 "<CMD> {size}<CR><LF>"
			  if (strstr((p3+1), "}\r\n") || strstr((p3+1), "}\r") || strstr((p3+1), "}\n"))
			  {
#endif
		    //if (*(p2-2) == '}') { // 引き続きデータがある場合
			  *p3 = '\x0';
			  p3++;
	  		  nx = atol(p3); // 受信サイズ
		      p = NULL;
              strtok(lpClientContext->Buffer, "\r\n");
		      if ((n+nx) < sizeof(lpClientContext->Buffer) ) {// 領域内なら
                strcpy(pContext->mMessages, "+ Ready for additional command text\r\n");
                put_reply(lpClientContext, TRUE, TRUE); // データFlash
			  } else {// 領域を越えた場合
			    sprintf(mec, "get_reply buffer size overflow. n=%u nx=%u", n, nx);
#ifdef ACCEPT_LOG_LEVEL3
	            LEVEL_3_ACCEPTLOG(lpClientContext, mec);
#endif
                //strcpy(pContext->mMessages, "* NO Buffer overflow\r\n");
                //put_reply(lpClientContext, TRUE, TRUE); // データFlash
		        return FALSE;
			  }
#ifdef UPDATE_20211027 // '{'が含まれたトークンを次通信にトークンが続く判定の改善 "<CMD> {size}<CR><LF>"
			  }
#endif
			}
		  }
		}
	  }
	} while(!p);
    if (bOnLog) {
      if (bDebug) printf("[%08x] RECV[%08x] <- [%s]\n", lpClientContext, lpClientContext->Socket, lpClientContext->Buffer);
#ifdef ACCEPT_LOG_LEVEL3
      LEVEL_3_ACCEPTLOG(lpClientContext, lpClientContext->Buffer);
#endif
	}
    //if (bDebug) printf("[%08x] get_reply() End\n", lpClientContext);
	if (l == 0)
	  return FALSE;  // 切断された為終了
	else
      return TRUE;
}

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

void SPA_EncodeA(char *pSrc, char *pDest) { // 記号化
   int i, n;
   char mEn[3];
   unsigned char cMask;
   
   cMask = *pSrc; // 記号化フラグ無し
   pSrc++;
   n = strlen(pSrc);
   for (i = 0; i < n; i++) {
     sprintf(mEn, "%02x", (unsigned char)((unsigned char)((*pSrc)^cMask)));
     strcat(pDest, mEn);
     pSrc++;
   }
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

BOOL SPA_CopyFileDecode(char *pSrc, char *pDest) {
  FILE *pSFn, *pDFn;
  CHAR cMask[2], mBuff[2][1024];
  BOOL bResult = FALSE;
  size_t nRead[2];

  if ((pSFn = fopen(pSrc, "rb"))) {
    nRead[0] = fread(cMask, 1, 2, pSFn);
	if (cMask[0] == '\xff') {  // 記号化されたファイル
	  if ((pDFn = fopen(pDest, "wb"))) {
		do {
          nRead[0] = fread(mBuff[0], 1, sizeof(mBuff[0])-2, pSFn);
		  mBuff[0][nRead[0]] = '\x0';
		  if (nRead[0]) {
	        nRead[1] = SPA_FileDecode(mBuff[0], mBuff[1], cMask[1]);
            fwrite(mBuff[1], 1, nRead[1], pDFn);
		  }
		} while (nRead [0] == sizeof(mBuff[0])-2);
		fclose(pDFn);
	    bResult = TRUE;
	  }
	}
    fclose(pSFn);
  }

  return bResult;
}

int SPA_FileDecode(char *pSrc, char *pDest, char cMask) { // 復号化
   int i, n, nByte;
   char mEn[3];

   nByte = 0;
   mEn[2] = '\x0';
   n = strlen(pSrc);
   for (i = 0; i < n; i += 2) {
     strncpy(mEn, (pSrc+i), 2);
     *pDest = (char)(tochar(mEn)^cMask);
     pDest++;
     nByte++;
   }
   *pDest = '\x0';
   return nByte;
}

BOOL CopyB64EncodeFile(char *pSrc, char *pDest) { //BASE64で記号化して保存します。
   FILE *fpSrc, *fpDest;
   char mLine[129], mMime[256], *p;
   BOOL bSts = FALSE;

   fpSrc = fopen(pSrc, "rt");
   if (fpSrc) {
     fpDest = fopen(pDest, "wt");
     if (fpDest) {
	   p = fgets(mLine, sizeof(mLine)-1, fpSrc);
	   while(p || !feof(fpSrc)) {
		 mLine[sizeof(mLine)-1] = '\x0';
	     translateUue2Base64(mLine, strlen(mLine), mMime);
		 fprintf(fpDest, "%s\n", mMime);
	     p = fgets(mLine, sizeof(mLine)-1, fpSrc);
	   }
	   bSts = TRUE;
	   fflush(fpDest);
	   fclose(fpDest);
	 }
	 fclose(fpSrc);
   }
   return bSts;
}

BOOL CopyB64DecodeFile(char *pSrc, char *pDest) { //BASE64から復号して保存します。
   FILE *fpSrc, *fpDest;
   char mLine[256], mDec[256], *p;
   BOOL bSts = FALSE;

   fpSrc = fopen(pSrc, "rt");
   if (fpSrc) {
     fpDest = fopen(pDest, "wt");
     if (fpDest) {
	   p = fgets(mLine, sizeof(mLine)-1, fpSrc);
	   while(p || !feof(fpSrc)) {
		 mLine[sizeof(mLine)-1] = '\x0';
		 strtok(mLine, "\n");
		 Base64DecodeLine(mLine, strlen(mLine), mDec, sizeof(mDec));
		 fputs(mDec, fpDest);
	     p = fgets(mLine, sizeof(mLine)-1, fpSrc);
	   }
	   bSts = TRUE;
	   fflush(fpDest);
	   fclose(fpDest);
	 }
	 fclose(fpSrc);
   }
   return bSts;
}

void GetDomainFromIP(char *myaddr, char *mydomain, DWORD nSize) {
  HKEY hKeyRoot = HKEY_LOCAL_MACHINE;
  HKEY hKey;
  CHAR mkey[256], mIP[256];
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
  sprintf(mkey,PROFILE_ROOT_TREE, DOMAIN_IMAP4IP); //DOMAIN_POP3IP);

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
	 } while (retCode != ERROR_NO_MORE_ITEMS);
  }
#ifdef REGTOFILE
  if (nClustering && !strnicmp(mkey, "software\\emwac", 14))
	CloseHandle(hFile);
  else
#endif
  RegCloseKey (hKey);
}

void GetMailBoxFolder(char *user, char *pMailBox) {
 CHAR mHome[256];
 CHAR mMIBD[256], *p1, *p2;
 BOOL bHome = FALSE;
 INT  nl;
#ifdef UPDATE_20070521 // 予約語対策
 CHAR mKey[256];
#endif

#ifdef V3
    BOOL bLink = FALSE;
    char mLink[256], *pdomain;

	mLink[0] = '\x0';
	pdomain = strstr(user, "@");
	if (!pdomain) // ドメイン区切りに@と%で可能に
	  pdomain = strstr(user, "%");
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
	}
#ifdef UPDATE_20040707  // ドメイン無しアドレスをデフォルトのドメインフォルダに入るようにする
    else {
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
	}
#endif
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
   GetProfileStringEx((LPCTSTR)SOFT_REG,(LPCTSTR)"MailInBoxDir", (LPCTSTR)MAIL_BOX, (LPTSTR)mMIBD, 256);
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
#ifdef UPDATE_20071205 // メールアドレス長が256Byteのときの対策
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
#endif

void imap4log(PCLIENT_CONTEXT lpClientContext, char *pSts) {
    PImap4Context pContext = &lpClientContext->Context;
	SYSTEMTIME ltime, lt;
	CHAR   *p, mtime[256];
#ifdef UPDATE_20141115 // // 独自アカウント運用時で最大文字数を128byteまで拡張可能にする対策をした。
	char   mdata[128], msts[256], mLogFn[256];
#else
	char   mdata[128], msts[128], mLogFn[256];
#endif
	FILE   *Logfp;
    int    nsts;

	///// pop3log //////////////////////
	if (bLog) {
	  //printf("(%08x):dot phase log start.\n", pContext);
      gettime(&ltime, mtime);
      _tzset();
	  SystemTimeToTzSpecificLocalTime(NULL, &ltime, &lt);
	  sprintf(mdata, "%02d%02d%02d", (lt.wYear%100), lt.wMonth, lt.wDay );
#ifdef REGTOFILE
      if (nClustering) {
	    sprintf(mLogFn, "%simap4log\\%s\\%s.log", mMailSpoolDir, mComName, mdata);
	  } else {
#endif
	  sprintf(mLogFn, "%simap4log\\%s.log", mMailSpoolDir, mdata);
#ifdef REGTOFILE
	  }
#endif
      sprintf( mdata, "%02d/%s/%04d:%02d:%02d:%02d",lt.wDay, mMonth[lt.wMonth-1], lt.wYear, lt.wHour, lt.wMinute, lt.wSecond);
	  msts[0] = '\x0';
      if (!stricmp(pContext->pCmd, "LOGIN") ){
        strcpy(msts, pContext->pToken);
		p = strrchr(msts, ' ');
		if (p) {
		  *p = '\x0';
		  strcat(p, " ****");
		} else {
		  strcat(msts, " ****");
		}
	  }
#ifdef UPDATE_20260606 // ログの書き込み信頼性向上対策
      WaitForSingleObject(g_hMutexImap4Log, INFINITE);  // 排他開始
      fpImap4Log = OpenCloseLog(fpImap4Log, mDTImap4Log, "imap4log", mComName, lt);
	  if (fpImap4Log) {
		 nsts = strnicmp(pContext->mToken,"pass", 4);
		 //fprintf(Logfp, "%s from %s %s%s%s [%s] %s\n",
		 fprintf(fpImap4Log, "[%s] %s from %s %s %s %s\n",
					 mdata,
                     (pContext->mUSER[0] ? pContext->mUSER : "-"),
					 pContext->PeerAddr,
				     pContext->pCmd,
					 (stricmp(pContext->pCmd, "LOGIN") ? (pContext->pToken ? pContext->pToken : "-") : msts),
					 pSts);
		 fflush(fpImap4Log);
	  }
#else
#ifdef UPDATE_20260603 // ログ出力の排他処理化を追加
      WaitForSingleObject(g_hMutexImap4Log, INFINITE);  // 排他開始
#endif
	  Logfp = _fsopen(mLogFn, "at", _SH_DENYNO);
	  if (Logfp) {
		 nsts = strnicmp(pContext->mToken,"pass", 4);
		 //fprintf(Logfp, "%s from %s %s%s%s [%s] %s\n",
		 fprintf(Logfp, "[%s] %s from %s %s %s %s\n",
					 mdata,
                     (pContext->mUSER[0] ? pContext->mUSER : "-"),
					 pContext->PeerAddr,
				     pContext->pCmd,
					 (stricmp(pContext->pCmd, "LOGIN") ? (pContext->pToken ? pContext->pToken : "-") : msts),
					 pSts);
		 fclose(Logfp);
	  }
#endif
#ifdef UPDATE_20260603 // ログ出力の排他処理化を追加
      ReleaseMutex(g_hMutexImap4Log);  // 排他終了
#endif
	}
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
  if (bDom) {
    pdom = strstr(user, "@");
    if (pdom)
      *pdom = '\x0';
  }
  /////////////////////////////////////
  mPersonFile[0] = '\x0';
  GetMailBoxFolder(user, mPersonFile);
  strcat(mPersonFile, mFileName);
  ///// LocalDomain 以外も設定可能とする
  if (bDom) {
    if (pdom)
      *pdom = '@';
  }
  /////////////////////////////////////
}

BOOL GetMailBoxSize(char *user, BOOL bLocal, BOOL bSubLocal) {
  CHAR mMBoxDir[_MAX_PATH], mFileGroup[_MAX_PATH];
  CHAR mTempDir[_MAX_PATH];
  CHAR *pdom;
  //INT  nl;
  BOOL bFile;
  HANDLE          hFindFile;
  WIN32_FIND_DATA FindFileData ;
  BOOL bBox = TRUE;
  DWORD nSize = 0, nBoxSize;

  //return (DWORD)0;
  if (bLocal && !bSubLocal) {
    nBoxSize = nMailInBoxSize;
    if (nBoxSize != 0) {
	  strcpy(mTempDir, mMailBoxDir);
      pdom = strstr(user, "@");
      if (pdom)
        *pdom = '\x0';
      mMBoxDir[0] = '\x0';
      GetMailBoxFolder(user, mMBoxDir);
      // sprintf(mFileGroup, "%s*.msg", mMBoxDir); // 2001.11.06 メールデータの拡張子を.MSG以外の指定可能にする。IIS SMTPの受信データに対応。
      sprintf(mFileGroup, "%s*.%s", mMBoxDir, mMailExtension);

      hFindFile = FindFirstFile( mFileGroup, &FindFileData);
      if (hFindFile != INVALID_HANDLE_VALUE) {
		bFile = TRUE;
        while (bFile) {
	      nSize += FindFileData.nFileSizeLow;
          bFile = FindNextFile( hFindFile, &FindFileData);
		}; 
        FindClose( hFindFile ); 
	  }
      if (pdom)
        *pdom = '@';
      if (nBoxSize < nSize)
        bBox = FALSE;
	}
  }
  return bBox;
}

BOOL MLUserCheck(char *mFrom, char *mRcpt) {
  HKEY hKeyRoot = HKEY_LOCAL_MACHINE;
  HKEY   hKey;
  CHAR  mkey[256];
  BOOL  sts = FALSE;
  DWORD  cbValueName = MAX_VALUE_NAME;
  DWORD  retCode;

  DWORD      dwIndex = 0;	         // index of subkey to enumerate 
  CHAR       mName[_MAX_PATH];	     // address of buffer for subkey name 
  DWORD      lpcbName = 0;
  int        i = 0;
#ifdef REGTOFILE
  HANDLE hFile;
#endif

   //sprintf(mkey,"SOFTWARE\\EMWAC\\IMS\\Lists\\%s", mRcpt);
   sprintf(mkey,"%s\\%s", SOFT_LISTS_REG, mRcpt);
   strtok(mkey,"@");
   strcat(mkey,"\\Members");
  // OPEN THE KEY.
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
     do {
#ifdef REGTOFILE
	 if (nClustering && !strnicmp(mkey, "software\\emwac", 14)) {
       retCode =
		 KeyFileEnumKey(mMailSpoolDir,
		                mkey,
						dwIndex, 
						(LPTSTR)mName, 
						(unsigned long *)&lpcbName);
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

BOOL ListsCheck(char *mFrom, char *mRcpt, char *mMlTkn, BOOL bCount) {
   CHAR  mkey[256], mtkn[64], mModerator[_MAX_PATH], *p;
   int   i, nketa;
   char  *pdom;
   DWORD nWhoCanSend, nMlId, nMax;
   BOOL  bsts = FALSE, bAliases = FALSE;

   pdom = strstr(mRcpt, "@");
   if (pdom)
	 *pdom = '\x0';
   //bAliases = GetAliases((LPCTSTR)"SOFTWARE\\EMWAC\\IMS\\Aliases", (char *)mRcpt, (char *)pdom);
   bAliases = GetAliases((LPCTSTR)SOFT_ALIASES_REG, (char *)mRcpt, (char *)pdom);
   if (pdom)
	 *pdom = '@';
   if (!bAliases) {
     //sprintf(mkey,"SOFTWARE\\EMWAC\\IMS\\Lists\\%s", mRcpt);
     sprintf(mkey,"%s\\%s", SOFT_LISTS_REG, mRcpt);
     strtok(mkey,"@");
     ////////////////////////////////////////////////////////////
     // WhoCanSend 0x1 = Member, 0x2 = Anyone, 0x3 = Moderator //
     ////////////////////////////////////////////////////////////
     nWhoCanSend = GetProfileIntEx(mkey, "WhoCanSend", (INT)-1);
     switch(nWhoCanSend) {
	    case 1: if (MLUserCheck(mFrom, mRcpt))
	 			  bsts = TRUE;
		        break;
	    case 2: bsts = TRUE;
		        break;
	    case 3: GetProfileStringEx(mkey,"Moderator", "", mModerator, sizeof(mModerator));
		        if (_stricmp(mFrom, mModerator) == 0)
				  bsts = TRUE;
		       break;
	 }
     if (bsts && bCount) {
       nMlId = GetProfileIntEx(mkey, "MlId", (INT)0) + 1;
	   GetProfileStringEx(mkey,"MlToken", "", mtkn, sizeof(mtkn));
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
	 }
   }
   return bsts;
}

BOOL GetAliases(LPCTSTR lpAppName, char *uid, char *dom) {
  HKEY hKeyRoot = HKEY_LOCAL_MACHINE;
  HKEY hKey;
  CHAR mkey[256];
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
  HANDLE hFile;
#endif

  // OPEN THE KEY.
  sprintf(mkey,PROFILE_ROOT_TREE,lpAppName);
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
  // READ THE KEY DATA.
    do {
#ifdef REGTOFILE
	 if (nClustering && !strnicmp(mkey, "software\\emwac", 14)) {
       retCode =
		 KeyFileEnumKey(mMailSpoolDir,
		                mkey,
						dwIndex, 
						(LPTSTR)lpName, 
						(unsigned long *)&lpcbName);
	 } else {
#endif
      retCode =
          RegEnumKey(hKey,	        // handle of key to query 
                     dwIndex,	    // index of subkey to query 
                     (LPTSTR)lpName, // address of buffer for subkey name  
                     (unsigned long)&lpcbName 	    // size of subkey buffer 
         );
#ifdef REGTOFILE
	 }
#endif
      if (retCode == ERROR_SUCCESS) {
		//sprintf(mAliasKey, "SOFTWARE\\EMWAC\\IMS\\Aliases\\%s", lpName);
		sprintf(mAliasKey, "%s\\%s", SOFT_ALIASES_REG, lpName);
        GetProfileStringEx(mAliasKey,"Alias", "", mAlias, sizeof(mAlias));
		_strlwr(mAlias);
		//////////////////////////////////////
		/////  Doamin Check
		pAdom = strstr(mAlias, "@");
	    stsDom = 1;
		if (pAdom)
          *pAdom = '\x0';
		if (dom && pAdom) {                // Domain check
		  stsDom = _stricmp(dom+1, pAdom+1);
		} else if (!dom && !pAdom) {
		  stsDom = 0;
		}
		//////////////////////////////////////
		/////  User Check
		pAlias = strstr(mAlias,"*");
		if (pAlias) {
          *pAlias = '\x0';
          if (*(pAlias+1) == '\x0')
			stsUid = 0;
		  else {
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
          pMapto = strstr(mMappedTo,"*");
		  if (pMapto) {
	        strcpy(mwork, &uid[strlen(mAlias)]);
			*pMapto = '\x0';
			//sprintf(uid, "%s%s%s", mMappedTo, uid, pMapto+1);
			sprintf(uid, "%s%s%s", mMappedTo, mwork, pMapto+1);
		  } else
	        strcpy(uid, mMappedTo);
		  break;
		}
	  }
      dwIndex++;
	} while (retCode == ERROR_SUCCESS);
#ifdef REGTOFILE
    if (nClustering && !strnicmp(mkey, "software\\emwac", 14))
	  CloseHandle(hFile);
    else
#endif
    RegCloseKey(hKey);
  }
  return sts;
}

BOOL GetPostMaster(char *uid) {
  CHAR   mMappedTo[_MAX_PATH];
  BOOL   sts = FALSE;

  // OPEN THE KEY.
  if (_stricmp(uid,"postmaster") == 0) {
    //GetProfileStringEx("SOFTWARE\\EMWAC\\IMS","PostMaster", "", mMappedTo, sizeof(mMappedTo));
    GetProfileStringEx(SOFT_REG,"PostMaster", "administrator", mMappedTo, sizeof(mMappedTo));
	if (mMappedTo[0] == '\x0')
	  strcpy(mMappedTo, "administrator");
    if (strlen(mMappedTo) > 0) {
	  //strcpy(uid, mMappedTo);
      sts = TRUE;
	}
  }
  return sts;
}

BOOL GetMLists(LPCTSTR lpAppName, char *uid, BOOL *bRequest) {
  HKEY hKeyRoot = HKEY_LOCAL_MACHINE;
  HKEY hKey;
  CHAR mkey[256];
  DWORD  cbValueName = MAX_VALUE_NAME;
  DWORD  retCode;
  DWORD  dwIndex = 0;	         // index of subkey to enumerate 
  CHAR   mName[_MAX_PATH];	 // address of buffer for subkey name 
  CHAR   mRequestName[_MAX_PATH];	// address of buffer for subkey name 
  DWORD  lpcbName = 0;
  BOOL   sts = FALSE;
  int    i = 0;
#ifdef REGTOFILE
  HANDLE hFile;
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
  // READ THE KEY DATA.
    do {
#ifdef REGTOFILE
	 if (nClustering && !strnicmp(mkey, "software\\emwac", 14)) {
       retCode =
		 KeyFileEnumKey(mMailSpoolDir,
		                mkey,
						dwIndex, 
						(LPTSTR)mName, 
						(unsigned long *)&lpcbName);
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
      if (retCode == ERROR_SUCCESS) {
	    sprintf(mRequestName,"%s-request", mName);
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
	  }
      dwIndex++;
	} while (retCode == ERROR_SUCCESS);
#ifdef REGTOFILE
    if (nClustering && !strnicmp(mkey, "software\\emwac", 14))
	  CloseHandle(hFile);
    else
#endif
    RegCloseKey(hKey);
  }
  return sts;
}

DWORD GetProfileIntEx(LPCTSTR lpAppName, LPCTSTR lpKeyName, INT nDefault) {
  HKEY hKeyRoot = HKEY_LOCAL_MACHINE;
  HKEY hKey;
  CHAR mkey[256];
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
	 } else {
#endif
    retCode =
	   RegQueryValueEx(hKey,            // handle of key to query 
                      (LPSTR)lpKeyName, // address of name of value to query 
                      0,                // reserved 
                      &dwType,  // address of buffer for value type 
                      (LPBYTE)&nReturned,  // address of data buffer 
                      &nSize  // address of data buffer size 
                      ); 
    RegCloseKey(hKey);
#ifdef REGTOFILE
	 }
#endif
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
  CHAR mkey[256];
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
	 } else {
#endif
    retCode =
	   RegQueryValueEx(hKey,            // handle of key to query 
                      (LPSTR)lpKeyName, // address of name of value to query 
                      0,                // reserved 
                      &dwType,  // address of buffer for value type 
                      (LPBYTE)lpReturnedString,  // address of data buffer 
                      &nSize  // address of data buffer size 
                      ); 
    RegCloseKey(hKey);
#ifdef REGTOFILE
	 }
#endif
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
  CHAR mkey[256];
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
	 } else {
#endif
    retCode =
	     RegQueryValueEx(hKey,            // handle of key to query 
                      (LPSTR)lpKeyName, // address of name of value to query 
                      0,                // reserved 
                      &dwType,  // address of buffer for value type 
                      lpReturnedString,  // address of data buffer 
                      &nSize  // address of data buffer size 
                      ); 
    RegCloseKey(hKey);
#ifdef REGTOFILE
	 }
#endif
	if (retCode != ERROR_SUCCESS)
#ifndef TUNING
      strcpy(lpReturnedString, "");
#else
	  *lpReturnedString = '\x0';
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

	 } else {
#endif
     retCode = 
      RegOpenKeyEx(hKeyRoot,    // Key handle at root level.
                   RegPath,     // Path name of child key.
                   0,           // Reserved.
                   KEY_WRITE,  // KEY_READ | KEY_WRITE, // Requesting read access.
                   &hKey);      // Address of key to be returned.
#ifdef REGTOFILE
	 }
#endif
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
	 } else {
#endif
      retCode =
         RegSetValueEx(hKey,	// handle of key to set value for  
                       ValuePath,	// address of value to set 
                       0,
                       REG_DWORD,	// type of value 
                       (CONST BYTE *)&lpData,	// address of value data 
                       (DWORD)cbData 	// size of value data 
                      );	
      RegCloseKey((HKEY)hKey);
#ifdef REGTOFILE
	 }
#endif
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

	 } else {
#endif
     retCode = 
      RegOpenKeyEx(hKeyRoot,    // Key handle at root level.
                   RegPath,     // Path name of child key.
                   0,           // Reserved.
                   KEY_WRITE,   // KEY_READ | KEY_WRITE, // Requesting read access.
                   &hKey);      // Address of key to be returned.
#ifdef REGTOFILE
	 }
#endif
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
	 } else {
#endif
      retCode =
         RegSetValueEx(hKey,	// handle of key to set value for  
                       ValuePath,	// address of value to set 
                       0,
                       REG_SZ,	// type of value 
                       (CONST BYTE *)lpData,	// address of value data 
                       (DWORD)cbData 	// size of value data 
                      );	
      RegCloseKey((HKEY)hKey);
#ifdef REGTOFILE
	 }
#endif
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

	sprintf(szRegPath, "%s%s", EVENT_REG, IMAP4_SERVICE);

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

#ifdef UPDATE_20141220 // 操作フォルダをアクセス中のセッションに変化をリアルタイムに同報するための対策

#ifdef UPDATE_20141221 // セッション内でのメッセージ送信の排他処理
void  put_reply_check(BOOL *pPutLock)
{
   if (bDebug) printf("[] put_reply_check()\n");
   while(*pPutLock) 
   {
     if (bServiceTerminating)
       break;
     _sleep(WAIT_TIME);
  }
  *pPutLock = TRUE;
}

#ifdef UPDATE_20141225 // セッション内でのメッセージ送信の排他処理ハング対策2
BOOL put_reply_check2(BOOL *pPutLock, CHAR *pFn)
#else
BOOL put_reply_check2(BOOL *pPutLock)
#endif
{
   unsigned int i = 0;
   BOOL bSts = FALSE;
#ifdef UPDATE_20141225 // セッション内でのメッセージ送信の排他処理ハング対策2
   FILE *fp;
#endif

   //if (bDebug) printf("[] put_reply_check2()\n");
   
   while(*pPutLock) 
   {
	  
	 if (i > nPutLockCnt || bServiceTerminating) 
	 {
       break;
	 }
	 i++;
	 
     _sleep(1000);
#ifdef UPDATE_20141225 // セッション内でのメッセージ送信の排他処理ハング対策2
     if (!(fp = fopen(pFn, "rt"))) 
	 { 
       break;
	 }
	 fclose(fp);
#endif
   }
   
#ifdef UPDATE_20141225 // セッション内でのメッセージ送信の排他処理ハング対策2
  if((fp = fopen(pFn, "rt"))) 
  { 
    fclose(fp);
    bSts = (*pPutLock ? FALSE: TRUE);
  }
  *pPutLock = bSts; //TRUE;
#else
  bSts = (*pPutLock ? FALSE: TRUE);
  *pPutLock = bSts; //TRUE;
#endif
  return bSts;
}
#endif

#ifdef UPDATE_20141225 // セッション内でのメッセージ送信の排他処理ハング対策2
void CheckBroadcastSession(CHAR *pFn)
{
   FILE *fp;
   CHAR mData[3];

   //if (bDebug) printf("[] CheckBroadcastSession(%s)\n", pFn);
   strcpy(mData, "0");
   do {
     if((fp = fopen(pFn, "rt"))) 
	 { 
       fgets(mData, 2, fp);
       fclose(fp);
	 }
	 if (mData[0] == '1')
	 {
	   _sleep(1000);
	 }
     if (bServiceTerminating)
       break;
   } while(mData[0] != '0');
}
#endif

void JoinBroadcastSession(PCLIENT_CONTEXT lpClientContext)
{
	CHAR   mFn[256];
	CHAR   *ptmp, mTempDir[_MAX_PATH], mMBDir[_MAX_PATH];
	FILE   *fp;
    HANDLE           hSearch;
    WIN32_FIND_DATA  FindData;
#ifdef UPDATE_20141222 // 同報セッション情報の保管先を接続ウント単位に保存する対策
    PImap4Context    pContext;
#endif

	//if (bDebug) printf("[%08x] JoinBroadcastSession()\n", lpClientContext);
	if (!lpClientContext || LocalFlags(lpClientContext) == LMEM_INVALID_HANDLE) // 有効なハンドルか否か確認
	{
	  return;
	}
#ifdef UPDATE_20151226 // 当分封印：IDLE処理の修正 (同一フォルダにアクセス中のセッションに変化が伝わらない不具合)
    EnterCriticalSection(&g_csIdleMess);
#endif
#ifdef UPDATE_20141222 // 同報セッション情報の保管先を接続ウント単位に保存する対策
	strcpy(mMBDir, mMailBoxDir);
	strtok(mMBDir, "%");
	if (mMBDir[strlen(mMBDir)-1] != '\\')
	{
	  strcat(mMBDir, "\\");
	}

	pContext = &lpClientContext->Context;
    sprintf(mFn, "%sreg\\" DOMAIN_IMAP4SESS "\\%s", mMailSpoolDir, &pContext->mRootDir[strlen(mMBDir)]);
    sprintf(mTempDir, "%sreg\\" DOMAIN_IMAP4SESS, mMailSpoolDir);
	if ((ptmp = &mFn[strlen(mTempDir)]))
	{
	  ptmp = strstr(ptmp+1,"\\");
      while(ptmp) {
        *ptmp = '\x0';
	    _mkdir(mFn);         // 処理用フォルダ作成
        *ptmp = '\\';
	    ptmp = strstr(ptmp+1,"\\");
	  }
	  //_mkdir(mFn);
	}
    sprintf(mFn, "%s\\reg\\" DOMAIN_IMAP4SESS "\\%s\\%lu", mMailSpoolDir, &pContext->mRootDir[strlen(mMBDir)], lpClientContext);
#else
    sprintf(mFn, "%s\\reg\\" DOMAIN_IMAP4SESS "\\%lu", mMailSpoolDir, lpClientContext);
#endif
    //if (bDebug) printf("[%08x] JoinBroadcastSession() DeleteFile(%s) Start\n", lpClientContext, mFn);
	if((fp = fopen(mFn, "wt"))) 
	{ 
      fputs("0\n", fp);
	  fclose(fp);
	}
    while ((hSearch = FindFirstFile((LPCTSTR)mFn, &FindData)) == INVALID_HANDLE_VALUE)
    {
	  _sleep(1000);
      if (bServiceTerminating)
       break;
    }
    FindClose(hSearch);
#ifdef UPDATE_20151226 // 当分封印：IDLE処理の修正 (同一フォルダにアクセス中のセッションに変化が伝わらない不具合)
    LeaveCriticalSection(&g_csIdleMess);
#endif

}

void LeaveBroadcastSession(PCLIENT_CONTEXT lpClientContext)
 {
	CHAR   mFn[256], mFlag[10];
	FILE   *fp;
    HANDLE           hSearch;
    WIN32_FIND_DATA  FindData;
#ifdef UPDATE_20141222 // 同報セッション情報の保管先を接続ウント単位に保存する対策
	CHAR   mMBDir[_MAX_PATH];
    PImap4Context    pContext;
#endif

#ifdef UPDATE_20151226 // 当分封印：IDLE処理の修正 (同一フォルダにアクセス中のセッションに変化が伝わらない不具合)
    EnterCriticalSection(&g_csIdleMess);
#endif
	//if (bDebug) printf("[%08x] LeaveBroadcastSession() Start\n", lpClientContext);
	if (lpClientContext && LocalFlags(lpClientContext) != LMEM_INVALID_HANDLE) // 有効なハンドルか否か確認
	{
#ifdef UPDATE_20141222 // 同報セッション情報の保管先を接続ウント単位に保存する対策
	  strcpy(mMBDir, mMailBoxDir);
	  strtok(mMBDir, "%");
	  if (mMBDir[strlen(mMBDir)-1] != '\\')
	  {
	    strcat(mMBDir, "\\");
	  }

	  pContext = &lpClientContext->Context;
      sprintf(mFn, "%sreg\\" DOMAIN_IMAP4SESS "\\%s%lu", mMailSpoolDir, &pContext->mRootDir[strlen(mMBDir)], lpClientContext);
#else
      sprintf(mFn, "%sreg\\" DOMAIN_IMAP4SESS "\\%lu", mMailSpoolDir, lpClientContext);
#endif
#ifdef UPDATE_20141225 // セッション内でのメッセージ送信の排他処理ハング対策2
      CheckBroadcastSession(mFn);
#endif

	  //if (bDebug) printf("[%08x] LeaveBroadcastSession() DeleteFile(%s) Start\n", lpClientContext, mFn);
      // セッションファイルの内容が'0'のとき削除可能
	  strcpy(mFlag, "0");
	  while(1) 
	  { 
	    if ((fp = fopen(mFn, "rt"))) 
		{
          fgets(mFlag, sizeof(mFlag)-1, fp);
          fclose(fp);
		}
		if (mFlag[0] == '0')
		{
		  break;
		}
		_sleep(1000);
        if (bServiceTerminating)
         break;
	  } 
      /////////////////////////////////////
	  DeleteFile(mFn);
      while ((hSearch = FindFirstFile((LPCTSTR)mFn, &FindData)) != INVALID_HANDLE_VALUE)
	  {
        FindClose(hSearch);
	    _sleep(1000);
		DeleteFile(mFn);
        if (bServiceTerminating)
          break;
	  }
	  //if (bDebug) printf("[%08x] LeaveBroadcastSession() DeleteFile(%s) End\n", lpClientContext, mFn);
	}
#ifdef UPDATE_20151226 // 当分封印：IDLE処理の修正 (同一フォルダにアクセス中のセッションに変化が伝わらない不具合)
    LeaveCriticalSection(&g_csIdleMess);
#endif
}

void PutBroadcastSession(PCLIENT_CONTEXT lpCC) //CHAR *pSelDir, CHAR *pMess)
{
	CHAR   mFn[256];
#ifdef UPDATE_20141222 // 同報セッション情報の保管先を接続ウント単位に保存する対策
	CHAR   mFn2[256];
	CHAR   mMBDir[_MAX_PATH];
	FILE *fp;
#endif
	CHAR   mFlag[3];
    HANDLE           hSearch, hS;
    WIN32_FIND_DATA  FindData, FD;
	PCLIENT_CONTEXT  lpClientContext;
	PImap4Context    pContext, pCC;
	BOOL bSts = FALSE;

	//if (bDebug) printf("[%08x] PutBroadcastSession():lpCC\n", lpCC);
	pCC = &lpCC->Context;
#ifdef UPDATE_20141222 // 同報セッション情報の保管先を接続アウント単位に保存する対策
    strcpy(mMBDir, mMailBoxDir);
    strtok(mMBDir, "%");
	if (mMBDir[strlen(mMBDir)-1] != '\\')
	{
	  strcat(mMBDir, "\\");
	}
    sprintf(mFn, "%sreg\\" DOMAIN_IMAP4SESS "\\%s*", mMailSpoolDir, &pCC->mRootDir[strlen(mMBDir)]);
#else
    sprintf(mFn, "%sreg\\" DOMAIN_IMAP4SESS "\\*", mMailSpoolDir);
#endif
#ifdef UPDATE_20151226 // 当分封印：IDLE処理の修正 (同一フォルダにアクセス中のセッションに変化が伝わらない不具合)
    EnterCriticalSection(&g_csIdleMess);
#endif

    if ((hSearch = FindFirstFile((LPCTSTR)mFn, &FindData)) != INVALID_HANDLE_VALUE)
	{
       do {
		  if (strcmp(FindData.cFileName, ".") &&
		      strcmp(FindData.cFileName, "..") )
		  {
            lpClientContext = (PCLIENT_CONTEXT)atol(FindData.cFileName);
			//if (bDebug) printf("[%08x] PutBroadcastSession():lpClientContext\n", lpClientContext);

            sprintf(mFn2, "%sreg\\" DOMAIN_IMAP4SESS "\\%s\\%s", mMailSpoolDir, &pCC->mRootDir[strlen(mMBDir)], FindData.cFileName);
            if ((hS = FindFirstFile((LPCTSTR)mFn2, &FD)) == INVALID_HANDLE_VALUE)
			{
			  if (bDebug) printf("[%08x] PutBroadcastSession():lpClientContext Skip\n", lpClientContext);
			  continue;
			}
			FindClose(hS);

	        if (lpClientContext && LocalFlags(lpClientContext) != LMEM_INVALID_HANDLE) // 解放可能かどうか確認
			{
			  //if (bDebug) printf("[%08x] PutBroadcastSession():lpClientContext->Context\n", lpClientContext);
		      pContext = &lpClientContext->Context;
		      if (!stricmp(pCC->mSelectDir, pContext->mSelectDir))
			  {
			    if (lpCC != lpClientContext) // 異なる場合はメッセージコピー
				{

#ifdef UPDATE_20141225 // セッション内でのメッセージ送信の排他処理ハング対策2
                  sprintf(mFn2, "%sreg\\" DOMAIN_IMAP4SESS "\\%s\\%s", mMailSpoolDir, &pCC->mRootDir[strlen(mMBDir)], FindData.cFileName);
				  //if (bDebug) printf("[%08x] PutBroadcastSession() put_reply_check2():Start %s\n", lpCC, mFn2);
                  CheckBroadcastSession(mFn2);
				  bSts = FALSE;
				  while(1)
				  {
	                if((fp = fopen(mFn2, "r+t"))) 
					{ 
					  fseek(fp, 0L, SEEK_SET);
	                  fputs("1", fp);
	                  fclose(fp);
				      bSts = TRUE;
					} else {
					  break;
					}
	                if((fp = fopen(mFn2, "rt"))) 
					{ 
	                  fgets(mFlag, 2, fp);
	                  fclose(fp);
					} 
					if (mFlag[0] == '1')
					{
					  break;
					}
					_sleep(1000);
                    if (bServiceTerminating)
                      break;
				  }
				  if (!bSts)
				  {
					continue;
				  }
#endif
#ifdef UPDATE_20141221 // セッション内でのメッセージ送信の排他処理
#ifdef UPDATE_20141225 // セッション内でのメッセージ送信の排他処理ハング対策2
                  if (put_reply_check2(&lpClientContext->bPutLock, mFn2))
#else
                  if (put_reply_check2(&lpClientContext->bPutLock))
#endif
#endif
				  {
                    strcpy(pContext->mCash, pCC->mMessages);
                    //if (lpClientContext->ssl || lpClientContext->Socket) // セッション内でのメッセージ送信の排他処理でのハング対策
					{
					  //if (bDebug) printf("[%08x] PutBroadcastSession() put_reply_flash():Start %s\n", lpCC, mFn2);
				      put_reply_flash(lpClientContext, TRUE);
					  //if (bDebug) printf("[%08x] PutBroadcastSession() put_reply_flash():End %s\n", lpCC, mFn2);
					}
#ifdef UPDATE_20141221 // セッション内でのメッセージ送信の排他処理
				    lpClientContext->bPutLock = FALSE;
#endif
				  }
#ifdef UPDATE_20141225 // セッション内でのメッセージ送信の排他処理ハング対策2
				  bSts = FALSE;
				  while(1)
				  {
	                if((fp = fopen(mFn2, "r+t"))) 
					{ 
					  fseek(fp, 0L, SEEK_SET);
	                  fputs("0", fp);
	                  fclose(fp);
				      bSts = TRUE;
					} else {
					  break;
					}
	                if((fp = fopen(mFn2, "rt"))) 
					{ 
	                  fgets(mFlag, 2, fp);
	                  fclose(fp);
					}
					if (mFlag[0] == '0')
					{
					  break;
					}
					_sleep(1000);
                    if (bServiceTerminating)
                      break;
				  }
#endif
				  //if (bDebug) printf("[%08x] PutBroadcastSession() put_reply_check2():End %s\n", lpCC, mFn2);
				}
			  }
			}
		  }
	   } while (FindNextFile(hSearch, &FindData));
	   FindClose(hSearch);
	}
#ifdef UPDATE_20151226 // 当分封印：IDLE処理の修正 (同一フォルダにアクセス中のセッションに変化が伝わらない不具合)
    LeaveCriticalSection(&g_csIdleMess);
#endif

}

#ifdef UPDATE_20141222 // 同報セッション情報の保管先を接続ウント単位に保存する対策
void InitBroadcastSession(CHAR *pFn)
#else
void InitBroadcastSession(void)
#endif
{
	CHAR   mFn[256], mFn2[256];
    HANDLE           hSearch;
    WIN32_FIND_DATA  FindData;
	PCLIENT_CONTEXT  lpClientContext;
	PImap4Context    pContext;

	//if (bDebug) printf("[] InitBroadcastSession()\n");
#ifdef UPDATE_20141222 // 同報セッション情報の保管先を接続ウント単位に保存する対策
	if (pFn)
	{
      sprintf(mFn, "%s\\*", pFn);
	}  else 
#endif
	{
      sprintf(mFn, "%s\\reg\\" DOMAIN_IMAP4SESS "\\*", mMailSpoolDir);
	}
#ifdef UPDATE_20151226 // 当分封印：IDLE処理の修正 (同一フォルダにアクセス中のセッションに変化が伝わらない不具合)
    EnterCriticalSection(&g_csIdleMess);
#endif
    if ((hSearch = FindFirstFile((LPCTSTR)mFn, &FindData)) != INVALID_HANDLE_VALUE)
	{
       do {
		 if (strcmp(FindData.cFileName, ".") &&
			 strcmp(FindData.cFileName, "..") )
		 {
	       if (pFn)
		   {
			 sprintf(mFn2, "%s\\%s", pFn, FindData.cFileName);
           } else {
			 sprintf(mFn2, "%s\\reg\\" DOMAIN_IMAP4SESS "\\%s", mMailSpoolDir, FindData.cFileName);
		   }
#ifdef UPDATE_20141222 // 同報セッション情報の保管先を接続ウント単位に保存する対策
           if (FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) 
		   {
			 InitBroadcastSession(mFn2);
	         RemoveDirectory(mFn2);
		   } else 
#endif
		   {
	         while(!DeleteFile(mFn2)) 
			 {
	           if (GetLastError() == ERROR_FILE_NOT_FOUND)
			   {
	             break;
			   }
	           _sleep(WAIT_TIME);
			 }
		   }
		 }
	   } while (FindNextFile(hSearch, &FindData));
	   FindClose(hSearch);
	}
#ifdef UPDATE_20151226 // 当分封印：IDLE処理の修正 (同一フォルダにアクセス中のセッションに変化が伝わらない不具合)
    LeaveCriticalSection(&g_csIdleMess);
#endif
}

#endif

#ifdef UPDATE_20151127 // リッスンＩＰ毎に証明書を選択できる対策(sslbyip.datファイルで指定)
BOOL IP_COMP(char *pRange, char *pAddr) {
  char *pAster, *pmask;
#ifdef UPDATE_20240728 // サブネットマスクの範囲指定の高速化
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
#ifdef UPDATE_20240728 // サブネットマスクの範囲指定の高速化
      if (bIPcmpmaskengin) // 新型
      {
        unsigned long nip;
        unsigned long sip;
        unsigned long eip;
        int  nmask;		

	    if ((mask = 32 - atoi(++pAster)) < 0)
 		  mask = 0;
        num = 1L << mask; // 2のべき乗範囲
		nmask = (-1L << (32-mask))-1;

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
	}
#ifdef UPDATE_20240728 // サブネットマスクの範囲指定の高速化
	}
#endif
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
    if (pAster && !strncmp(pAddr, pRange, strlen(pRange)) || //ワイルドカード有り
		!pAster && !strcmp(pAddr, pRange))   // ワイルドカードなし
	{
      bsts = TRUE;  // 一致するものがあった
	}
  } else { // ネットマスクの場合
	if (strlen(pAddr) > strlen(pRange))
	{
	  for ( i = 0; i < addr; i++)
	  {
	    *pAster = '\x0';
	    pmask = (pRange+strlen(pRange));
        n = atoi(pAddr+strlen(pRange)+1);
	    sprintf(pmask, ".%d", start + i);
        if ((start+i == n) &&
		    !strncmp(pAddr, pRange, strlen(pRange)))
		{
	      bsts = TRUE;  // 一致するものがあった
		}
	  }
	}
  }
  return bsts;
}
#endif

#ifdef ADD_IDCACHE

BOOL IDLiveTime(char *pFn) 
{
  SYSTEMTIME ltime;
  FILETIME   st, ft, ut;
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
     n2  = u2->QuadPart;// + (__int64)((__int64)nIDCashLiveTime*(__int64)10000000);
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

BOOL GetIDCashList(char *pID, char *pData, char *pName, BOOL *pResult)
{
#ifdef ACCEPT_LOG_LEVEL3
	char str[LOG_BUFFER];
#endif
  FILE    *fp;
  char    *fsts;
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
	   if (!strcmp(pName, "hm"))
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
   strtok(mID, ".");
   ReleaseIDChche(mID);
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

// ADキャッシュを全て削除する
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

#ifdef UPDATE_20151225 // 試作：UID値によるファイル範囲指定のチューニング
void GetQueryFileArea(CHAR *pMailDir, CHAR *pBaseFolder, BOOL bUID, BOOL bFromTo, DWORD nFrom, DWORD nTo)
{
  int i;
  CHAR mFrom[12];
  CHAR mTo[12];
  CHAR mQuery[12] = "??????????";

  if (bQueryFileArea && bUID & bFromTo)
  {
    sprintf(mFrom, "%010lu", nFrom);
    sprintf(mTo, "%010lu", nTo);
    for (i = 0; i < 10; i++)
	{
	  if (mFrom[i] == mTo[i])
	  {
	    mQuery[i] = mFrom[i];
	  } else {
		break;
	  }
	}
    sprintf(pMailDir, "%s\\%s-??????.MSG", pBaseFolder, mQuery); 
  } else {
    sprintf(pMailDir, "%s\\*-??????.MSG", pBaseFolder); 
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
      sprintf(Path, "%s%s.imap", pDir, mAddr); 
	}
	else 
#endif
	{
      sprintf(Path, "%s%s.imap", pDir, pAddr); 
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
        sprintf(Path, "%sauthlock\\%s.imap", mMailSpoolDir, pAddr);
#else
        sprintf(Path, "%s%s.imap", mMailBoxDir, pAddr); 
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
  BOOL bResult = FALSE;
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
  printf("%x\n",pip6.s6_addr[0]);
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

#ifdef ADD_XOAUTH2_D // OAUTH2の認可アプリの登録メールアドレスで一致するもののみの許可するようにした。
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
        //先頭カラムと最後尾カラムのワイルドカード指定があると処理できない
	if (k == 0) {
	  m = strlen((char *)(string1+1));
	}
	{
      if (strnicmp(string2, string1, k) == 0) {
		n += k;
	    if (q = strchr(p+1, '*')) {
	      *q = '\x0';
          l = strlen(p+1);
	      m = strlen(q+1);
              //最後尾カラムのワイルドカード指定があると処理できない
		} else if (*(p+1) == '\x0') {
		  return TRUE;
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

#ifdef UPDATE_20260603 // ログ出力の排他処理化を追加
HANDLE g_hMutexAcceptLog = NULL;
HANDLE g_hMutexImap4Log = NULL;

#ifdef UPDATE_20260606 // ログの書き込み信頼性向上対策
/*
  ログの書き込み用ファイルディスクプリタを新規で開いた後保持して
　一定時間間隔（１時間間隔）内の書込みは実行し時間を超えた時の書き
　込みはクローズ再オープンし、書き込み時は、MUTEXで排他する。
*/
FILE *fpAcceptLog = NULL;
FILE *fpImap4Log = NULL;

char mDTAcceptLog[16] = "";
char mDTImap4Log[16] = "";
#endif

/* 初期化：アプリ起動時に1回だけ呼ぶ */
void InitLogger()
{
    g_hMutexImap4Log = CreateMutex(NULL, FALSE, TEXT("Local\\MyLogMutexImap4log"));
    if (bDebug) printf("g_hMutexImap4Log = %08x\n", g_hMutexImap4Log);
    g_hMutexAcceptLog = CreateMutex(NULL, FALSE, TEXT("Local\\MyLogMutexAcceptlog"));
    if (bDebug) printf("g_hMutexAcceptLog = %08x\n", g_hMutexAcceptLog);
}

/* 終了処理：アプリ終了時に1回だけ呼ぶ */
void CloseLogger()
{
    if (g_hMutexImap4Log) {
        CloseHandle(g_hMutexImap4Log);
        g_hMutexImap4Log = NULL;
    }
    if (g_hMutexAcceptLog) {
        CloseHandle(g_hMutexAcceptLog);
        g_hMutexAcceptLog = NULL;
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

FILE *OpenCloseLog(FILE *pfp, char *pDT, char *plog, char *pCom, SYSTEMTIME lt)
{
   char mdata[16];
   char mFn[256];

   sprintf(mdata, "%02d%02d%02d%02d%02d", (lt.wYear%100), lt.wMonth, lt.wDay, lt.wHour, lt.wMinute);
   mdata[8] = '\x0'; // 10桁で分単位 9桁で10分 8桁で時間

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
#ifdef REGTOFILE
      if (nClustering) {
         sprintf(mFn, "%s\\%s\\%s\\%02d%02d%02d.log", mMailSpoolDir, plog, pCom, (lt.wYear%100), lt.wMonth, lt.wDay);
	  } else {
#endif
         sprintf(mFn, "%s\\%s\\%02d%02d%02d.log", mMailSpoolDir, plog, (lt.wYear%100), lt.wMonth, lt.wDay);
#ifdef REGTOFILE
	  }
#endif
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
   return pfp;
}
#endif
#endif