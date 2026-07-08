////////////////////////////////////////////////////////////
// Profile.h Copyright K.kawakami
//   Get profile key and data header.
////////////////////////////////////////////////////////////

#ifdef UPDATE_20211210_IPV6PREFIX // IPv6アドレスプレフィックスに対応した。
BOOL Compear_IPv6_Addr(char *pSrcAddr, char *pCompAddr, char *pPrefix);
#endif

#ifdef FIRST_TIME_MAIL // 繰り返す同一内容のメール除去
BOOL FirstTimeMail(char *pMSG);
BOOL FirstTimeMail_PASSIP(char *pAddr);
#endif

#ifdef UPDATE_20190426 // 利用する暗号アルゴリズムの組み合わせオプション 
void Print_SSL_CIPHER_names(const SSL_CTX *ctx);
#endif
#ifdef V4
#ifdef UPDATE_20070510 // 実ユーザ名でのメール送受信を禁止処理する処理
BOOL GetSMTPOFFFile(char *pUser, char *pDom);
#endif
BOOL GetUseTimeFile(char *pUser, char *pDom);
#ifdef UPDATE_20050924  // 送信元ユーザ別送信先制限
#ifdef UPDATE_20070516 // 上長承認機能の追加
#ifdef UPDATE_20070607 // 上長承認機能にSubject:キーワードも可能にする
#ifdef UPDATE_20080617 // 上長承認機能で添付ファイルの拡張子による承認条件追加
#ifdef UPDATE_20161111 /// 外部からの送信元送信先制限(逆上長承認)
BOOL GetSenderPermitFile(CHAR *pFrom, CHAR *pTo, DWORD *pOption, CHAR *pMIME, CHAR *pSubject, CHAR *pBoss, BOOL bType);
#else
BOOL GetSenderPermitFile(CHAR *pFrom, CHAR *pTo, DWORD *pOption, CHAR *pMIME, CHAR *pSubject, CHAR *pBoss);
#endif
#else
BOOL GetSenderPermitFile(CHAR *pFrom, CHAR *pTo, DWORD *pOption,CHAR *pSubject, CHAR *pBoss);
#endif
#else
BOOL GetSenderPermitFile(CHAR *pFrom, CHAR *pTo, DWORD *pOption, CHAR *pBoss);
#endif
#else
BOOL GetSenderPermitFile(CHAR *pFrom, CHAR *pTo);
#endif
#endif
#endif
BOOL put_reply(PCLIENT_CONTEXT lpClientContext, BOOL bOnLog);
BOOL get_reply(PCLIENT_CONTEXT lpClientContext, BOOL bOnLog);
#ifdef V3
NET_API_STATUS UserHomeDir( CHAR *lpszContry, CHAR *lpszDomain, CHAR *lpszUser, CHAR *lpszUserDomain, CHAR *lpszHomeDir);
#else
NET_API_STATUS UserHomeDir( CHAR *lpszContry, CHAR *lpszDomain, CHAR *lpszUser, CHAR *lpszHomeDir);
#endif
BOOL  GetMailBoxSize(char *user, BOOL bLocal, BOOL bSubLocal);
#ifdef UPDATE_20070509 // エイリアス設定の実アドレスオプションを別データとして処理する
BOOL  IP_COMP(char *pRange, char *pAddr);
BOOL  GetAliases(LPCTSTR lpAppName, char *uid, char *dom, char *pOpt);
#else
BOOL  GetAliases(LPCTSTR lpAppName, char *uid, char *dom);
#endif
BOOL  GetMLSenderPermission(char *uid);
BOOL  QueryMLists(LPCTSTR lpAppName, char *uid);
BOOL  GetMLists(LPCTSTR lpAppName, char *uid, BOOL *bRequest);
BOOL  GetPostMaster(char *uid, char *dom);
DWORD GetProfileIntEx(LPCTSTR lpAppName, LPCTSTR lpKeyName, INT nDefault);
DWORD GetProfileStringEx(LPCTSTR lpAppName, LPCTSTR lpKeyName, LPCTSTR lpDefault, LPTSTR lpReturnedString, DWORD nSize);
DWORD GetProfileBinaryEx(LPCTSTR lpAppName, LPCTSTR lpKeyName, LPCTSTR lpDefault, LPTSTR lpReturnedString, DWORD nSize);
void  WriteProfileIntEx(CHAR * KeyPath, CHAR * ValuePath, DWORD ValueInt);
void  WriteProfileStringEx(CHAR * KeyPath, CHAR * ValuePath, LPCTSTR ValueString);

#ifdef CLUSTERING
DWORD ReadLastMsgIdForAscii(HANDLE *hFile, DWORD nMsgId);
void WriteLastMsgIdForAscii(HANDLE *hFile, DWORD nMsgId);
#endif

#ifdef REGTOFILE
void  ReadyDrive(char *pStr);
DWORD FileCreateKey(char *pKeyRoot, char *pKey);
DWORD OpenKeyFile(char *pKeyRoot, char *pKey, char *pValue, HANDLE *hFile);
DWORD KeyFileQueryValueEx(char *pKeyRoot, char *pKey, char *pValue, HANDLE hFile, DWORD dwType, LPBYTE pRet, DWORD *nSize);
DWORD KeyFileSetValueEx(char *pKeyRoot, char *pKey, char *pValue, HANDLE hFile, DWORD  dwType, LPBYTE pData, DWORD nSize);
DWORD KeyFileEnumValue(char *pKeyRoot, char *pKey, DWORD nIndex, char *pValue, DWORD  dwType, LPBYTE pRet, DWORD *nSize);
DWORD KeyFileEnumKeyEx(HANDLE *hF, char *pKeyRoot, char *pKey, char *pValue, DWORD *nSize);
DWORD KeyFileEnumKey(char *pKeyRoot, char *pKey, DWORD nIndex, char *pValue, DWORD *nSize);
DWORD KeyFileDeleteValue(char *pKeyRoot, char *pKey, char *pValue);
#endif

#ifdef UPDATE_20260603 // ログ出力の排他処理化を追加
//#ifndef LOGGER_H
//#define LOGGER_H
extern HANDLE g_hMutexAcceptLog;
extern HANDLE g_hMutexInLog;
extern HANDLE g_hMutexRecivedLog;
extern HANDLE g_hMutexMaildbLog;
extern HANDLE g_hMutexApprovalLog;

void InitLogger();
void CloseLogger();
//#endif

#ifdef UPDATE_20260606 // ログの書き込み信頼性向上対策
/*
  ログの書き込み用ファイルディスクプリタを新規で開いた後保持して
　一定時間間隔（１時間間隔）内の書込みは実行し時間を超えた時の書き
　込みはクローズ再オープンし、書き込み時は、MUTEXで排他する。
*/
extern FILE *fpAcceptLog;
extern FILE *fpInLog;
extern FILE *fpRecivedLog;
extern FILE *fpMaildbLog;
extern FILE *fpApprovalLog;

extern char mDTAcceptLog[];
extern char mDTInLog[];
extern char mDTRecivedLog[];
extern char mDTMaildbLog[];
extern char mDTApprovalLog[];

FILE *OpenCloseLog(FILE *pfp, char *pDT, char *plog, char *pCom, SYSTEMTIME lt);

#endif
#endif
