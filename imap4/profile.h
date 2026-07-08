////////////////////////////////////////////////////////////
// Profile.h Copyright K.kawakami
//   Get profile key and data header.
////////////////////////////////////////////////////////////
#ifndef _PROFILE_H
#define _PROFILE_H

#ifdef UPDATE_20211210_IPV6PREFIX // IPv6アドレスプレフィックスに対応した。
BOOL Compear_IPv6_Addr(char *pSrcAddr, char *pCompAddr, char *pPrefix);
#endif
#ifdef UPDATE_20190510 // NTFS以外のファイルシステムからのファイル一覧取得可能にする対策（一覧ソート）を追加
int fn_sort( const void * a , const void * b );
WIN32_FIND_DATA * FindFirstFileSort(LPCTSTR pDir, WIN32_FIND_DATA* pFD, DWORD *pNo);
BOOL FindNextFileSort(WIN32_FIND_DATA *pFD, WIN32_FIND_DATA *pfd);
void FindCloseSort(WIN32_FIND_DATA *pfd);
#endif
#if defined(USE_SSL) && defined(UPDATE_20190426) // 利用する暗号アルゴリズムの組み合わせオプション 
void Print_SSL_CIPHER_names(const SSL_CTX *ctx);
#endif
#ifdef V4
BOOL GetUseTimeFile(char *pUser, char *pDom);
#endif
#ifdef UPDATE_20151127 // リッスンＩＰ毎に証明書を選択できる対策(sslbyip.datファイルで指定)
BOOL IP_COMP(char *pRange, char *pAddr);
#endif
BOOL put_reply(PCLIENT_CONTEXT lpClientContext, BOOL bFlash, BOOL bOnLog);
BOOL get_reply(PCLIENT_CONTEXT lpClientContext, BOOL bOnLog, CHAR *pCmd);
#ifdef V3
NET_API_STATUS UserHomeDir( CHAR *lpszContry, CHAR *lpszDomain, CHAR *lpszUser, CHAR *lpszUserDomain, CHAR *lpszHomeDir);
#else
NET_API_STATUS UserHomeDir( CHAR *lpszContry, CHAR *lpszDomain, CHAR *lpszUser, CHAR *lpszHomeDir);
#endif
BOOL  GetMailBoxSize(char *user, BOOL bLocal, BOOL bSubLocal);
BOOL  GetAliases(LPCTSTR lpAppName, char *uid, char *dom);
BOOL  GetMLists(LPCTSTR lpAppName, char *uid, BOOL *bRequest);
BOOL  GetPostMaster(char *uid);
DWORD GetProfileIntEx(LPCTSTR lpAppName, LPCTSTR lpKeyName, INT nDefault);
DWORD GetProfileStringEx(LPCTSTR lpAppName, LPCTSTR lpKeyName, LPCTSTR lpDefault, LPTSTR lpReturnedString, DWORD nSize);
DWORD GetProfileBinaryEx(LPCTSTR lpAppName, LPCTSTR lpKeyName, LPCTSTR lpDefault, LPTSTR lpReturnedString, DWORD nSize);
void  WriteProfileIntEx(CHAR * KeyPath, CHAR * ValuePath, DWORD ValueInt);
void  WriteProfileStringEx(CHAR * KeyPath, CHAR * ValuePath, LPCTSTR ValueString);

void imap4logimap4log(PCLIENT_CONTEXT lpClientContext);
void GetBaseDirectory(CHAR *BaseDir, char *mMBdir, char *muser, char *myaddr);
#ifdef UPDATE_20160113 // 認証時のロックアウト機能を追加
DWORD CheckUser(char *user, char *pass, char *myaddr, char *pPeerAddr);
#else
DWORD CheckUser(char *user, char *pass, char *myaddr);
#endif
BOOL GetPasswordFile(char  *pDir, char *pUser, char *mpass);
//UnlockMailDirectory(HANDLE hLockFile);
BOOL   UnLockMailDirectory(PCHAR  pszPath, PCHAR pszMyAddr);
HANDLE LockMailDirectory(PCHAR  pszPath, PCHAR pszMyAddr);
void SPA_EncodeA(char *pSrc, char *pDest); // 本文終了データ記号化用
void SPA_Encode(char *pSrc, char *pDest);
void SPA_Decode(char *pSrc, char *pDest);

#ifdef REGTOFILE
void  ReadyDrive(char *pStr);
DWORD FileCreateKey(char *pKeyRoot, char *pKey);
DWORD OpenKeyFile(char *pKeyRoot, char *pKey, char *pValue, HANDLE *hFile);
DWORD KeyFileQueryValueEx(char *pKeyRoot, char *pKey, char *pValue, HANDLE hFile, DWORD dwType, LPBYTE pRet, DWORD *nSize);
DWORD KeyFileSetValueEx(char *pKeyRoot, char *pKey, char *pValue, HANDLE hFile, DWORD  dwType, LPBYTE pData, DWORD nSize);
DWORD KeyFileEnumValue(char *pKeyRoot, char *pKey, DWORD nIndex, char *pValue, DWORD  dwType, LPBYTE pRet, DWORD *nSize);
DWORD KeyFileEnumKey(char *pKeyRoot, char *pKey, DWORD nIndex, char *pValue, DWORD *nSize);
DWORD KeyFileDeleteValue(char *pKeyRoot, char *pKey, char *pValue);
#endif

#ifdef UPDATE_20060116  // ユーザルートより上位のフォルダにアクセスできる脆弱性３
BOOL FolderRight2(PImap4Context pContext, char *pTarget);
#endif

#ifdef UPDATE_20050530  // ユーザルートより上位のフォルダにアクセスできる脆弱性
BOOL FolderRight(PImap4Context pContext, char *pTarget);
#endif

#ifdef UPDATE_20070425 // MSCSのスタンバイ側に対応
BOOL QueryDrive(char *pStr);
#endif UPDATE_20070425 // MSCSのスタンバイ側に対応

#ifdef UPDATE_20260603 // ログ出力の排他処理化を追加
//#ifndef LOGGER_H
//#define LOGGER_H
extern HANDLE g_hMutexImap4Log;
extern HANDLE g_hMutexAcceptLog;

void InitLogger();
void CloseLogger();
//#endif
#endif
#ifdef UPDATE_20260606 // ログの書き込み信頼性向上対策
/*
  ログの書き込み用ファイルディスクプリタを新規で開いた後保持して
　一定時間間隔（１時間間隔）内の書込みは実行し時間を超えた時の書き
　込みはクローズ再オープンし、書き込み時は、MUTEXで排他する。
*/
extern FILE *fpImap4Log;
extern FILE *fpAcceptLog;

extern char mDTImap4Log[];
extern char mDTAcceptLog[];

FILE *OpenCloseLog(FILE *pfp, char *pDT, char *plog, char *pCom, SYSTEMTIME lt);

#endif

#endif