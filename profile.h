////////////////////////////////////////////////////////////
// Profile.h Copyright K.kawakami
//   Get profile key and data header.
////////////////////////////////////////////////////////////
#ifdef UPDATE_20231103 // エラーメール生成時にメールヘッダの情報を利用するオプション機能を追加。
void GetInfo(_TCHAR *pTkn, _TCHAR *pFn, _TCHAR *pData, DWORD nMaxLen);
void GetEnvelope(char *pEnveopes, char *pMSG, char *pEnv);
#endif
#ifdef UPDATE_20211210_IPV6PREFIX // IPv6アドレスプレフィックスに対応した。
BOOL Compear_IPv6_Addr(char *pSrcAddr, char *pCompAddr, char *pPrefix);
#endif
#ifdef UPDATE_20190426 // 利用する暗号アルゴリズムの組み合わせオプション 
void Print_SSL_CIPHER_names(const SSL_CTX *ctx);
#endif
#ifdef UPDATE_20060529 // 設定ドメイン毎のＩＰに送信元ＩＰを切替える設定
void GetMTAIP(char *pDom, char *pMTAIP);
#endif
#ifdef ML_ASTER_OPTION
DWORD SetListsUserAster(char *pName, struct _RCP *rcp, char *mRCP, DWORD nMLDomDiv, DWORD nNoOfDiv, DWORD *nListNo);
#endif
#ifdef V3
NET_API_STATUS UserHomeDir( CHAR *lpszContry, CHAR *lpszDomain, CHAR *lpszUser, CHAR *lpszUserDomain, CHAR *lpszHomeDir);
#else
NET_API_STATUS UserHomeDir( CHAR *lpszContry, CHAR *lpszDomain, CHAR *lpszUser, CHAR *lpszHomeDir);
#endif
BOOL CheckDomain(char *mID);
void  GetMailBoxFolder(char *user, char *pMailBox);
BOOL  GetMailBoxSize(char *user, BOOL bLocal, BOOL bSubLocal);
#ifdef UPDATE_20070509 // エイリアス設定の実アドレスオプションを別データとして処理する
BOOL GetAliases(LPCTSTR lpAppName, char *uid, char *dom, char *pOpt);
#else
BOOL GetAliases(LPCTSTR lpAppName, char *uid, char *dom);
#endif
BOOL  GetMLists(LPCTSTR lpAppName, char *uid, BOOL *bRequest);
BOOL  CheckLists(LPCTSTR lpAppName, char *uid);
BOOL  MLUserCheck(char *mFrom, char *mRcpt);
BOOL  GetPostMaster(char *uid);
BOOL  QueryMLists(LPCTSTR lpAppName, char *uid);
DWORD GetProfileIntEx(LPCTSTR lpAppName, LPCTSTR lpKeyName, INT nDefault);
DWORD GetProfileStringEx(LPCTSTR lpAppName, LPCTSTR lpKeyName, LPCTSTR lpDefault, LPTSTR lpReturnedString, DWORD nSize);
DWORD GetProfileBinaryEx(LPCTSTR lpAppName, LPCTSTR lpKeyName, LPCTSTR lpDefault, LPTSTR lpReturnedString, DWORD nSize);
void  WriteProfileIntEx(CHAR * KeyPath, CHAR * ValuePath, DWORD ValueInt);
void  WriteProfileStringEx(CHAR * KeyPath, CHAR * ValuePath, LPCTSTR ValueString);
void  SPA_Encode(char *pSrc, char *pDest); // 記号化
void  SPA_Decode(char *pSrc, char *pDest); // 復号化
void  SPA_FileEncode(char *pSrc, char *pDest, char cMask); // 記号化
void  CopyFile_Encode(char *pSrc, char *pDest); // データファイルを記号化暗号化
DWORD X_CopyFile(char *pSrc, char *pDest, BOOL bType);
void DelMXCashList(char *pDom);
#ifdef UPDATE_20070423 // MXキャッシュを全て削除する
void CleanMXCashList(void);
#endif
#ifdef UPDATE_20150108 // 送信許可時間の設定機能を追加
BOOL GetUseTimeFile(char *pUserFrom, char *pUserTo, char *pDom);
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
extern HANDLE g_hMutexOutLog;
extern HANDLE g_hMutexOutLocalLog;
extern HANDLE g_hMutexOutFailLog;
extern HANDLE g_hMutexOutSenderLog;
void InitLogger();
void CloseLogger();
//#endif

#ifdef UPDATE_20260606 // ログの書き込み信頼性向上対策
/*
  ログの書き込み用ファイルディスクプリタを新規で開いた後保持して
　一定時間間隔（１時間間隔）内の書込みは実行し時間を超えた時の書き
　込みはクローズ再オープンし、書き込み時は、MUTEXで排他する。
*/
extern FILE *fpOutLog;
extern FILE *fpOutLocalLog;
extern FILE *fpOutFailLog;
extern FILE *fpOutSenderLog;

extern char mDTOutLog[];
extern char mDTOutLocalLog[];
extern char mDTOutFailLog[];
extern char mDTOutSenderLog[];

FILE *OpenCloseLog(FILE *pfp, char *pDT, char *plog, char *pCom, SYSTEMTIME lt);

#endif
#endif