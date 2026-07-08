////////////////////////////////////////////////////////////
// Imap4Type.h Copyright K.kawakami
////////////////////////////////////////////////////////////
#include <windows.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
//#include "debug.h"

#ifdef USE_SSL
#define NO_SNPRINTF
#define NO_SYSLOG
#define NO_FORK
#define NO_SETHOSTENT
#define NO_ALRM
#define NO_SETUID
#define NO_CHROOT
//#define ValidSocket(sd)		((sd) != INVALID_SOCKET)
//#undef EINTR
//#define EINTR	WSAEINTR
#define NO_BCOPY
#define bzero(b,n)	memset(b,0,n)
#endif

#define MAX_POP3_COMMAND  13 //14 //13
#define APPLICATION_NAME  TEXT(POP3_NAME)

#define POP3_MESSAGE_DELETE     0x00000001  // Message has been "deleted"

#define BUFFER_ARREY 4
#ifdef UPDATE_20230707 // FTECH ENVELOPEでTO,CC,BCCのアドレス定義可能な上限を増やした。
#define FETCH_BODY_BUFFER 1024*8+0x20000*3 //8192
#else
#define FETCH_BODY_BUFFER 1024*8 //8192
#endif

#define UIDLEN      10      // UID名長さ
#define RECENT       1      // 新着
#define ANSWERED     2      // 返信済み
#define FLAGGED      3      // 重要度
#define DELETED      4      // 削除対象
#define SEEN         5      // 読込済み
#define DRAFT        6      // 草稿


typedef enum _LAST_CLIENT_IO {
    ClientIoRead,
    ClientIoWrite,
    ClientIoTransmitFile
} LAST_CLIENT_IO, *PLAST_CLIENT_IO;

typedef enum _IMAP4State {
    Imap4Negotiate,                     // 非認証状態
    Imap4Authorization,                 // 認証済状態
    Imap4SelectFolder,                  // フォルダ選択済状態
} IMAP4State, *PIMAP4State;

#ifdef UPDATE_20151115 // SELECT時にメール一覧をキャッシュするためのポインタ
typedef struct _SELECT_FILE_LIST {
  //BOOL bSel;
  CHAR mFn[22];
} SFL;
#endif

typedef struct  _IMAP4_SEARCH_TOKEN {
#ifdef UPDATE_20150607 // 検索階層カウンタを追加
	int   nGrp;           // 階層
	int   nGrp2;          // 階層 バックアップ
    int   nGrpOR;         // 上位階層の検索文字列の種別(FALSE:AND/TRUE:OR)
    BOOL  bOR2;           // 検索文字列の種別(FALSE:AND/TRUE:OR) バックアップ
#endif
	BOOL  bResult;        // 検索結果(FALSE:無効/TRUE:有効)
	BOOL  bUnrelated;     // 大小文字無関係(FALSE:関係なし/TRUE:関係あり)
	BOOL  bNOT;           // 検索文字列の種別(FALSE:一致/TRUE:不一致)
    BOOL  bOR;            // 検索文字列の種別(FALSE:AND/TRUE:OR)
	INT   nMode;          // 0:FLAG/1:番号/2:内部日付/3:ファイルサイズ/4:メッセージ/5:その他
	INT   nUid;           // 0:連番/1:UID番号
	INT   nFlag;          // フラグ番号ビット位置(1:RECENT/2:ANSWERED/3:FLAGGED/4:DELETED/5:SEEN/6:DRAFT)
	INT   nFileSizeType;  // ファイルサイズ条件(0:LARGER/1:SMALLER)
	INT   nDType;         // 日付検索時の(0:BEFORE/1:ON/2:SINCE)
    INT   nArea;          // 検索文字列の検索領域(0:TEXT/1:BODY/2:HEADER)
	DWORD nFileSize;      // ファイルサイズ(byte)
	DWORD nFrom;          // 開始番号
	DWORD nTo;            // 終了番号
#ifdef UPDATE_20091129I // SEARCH CHRSETのUTF-8対応 CatchMe@Mailで添付きメールの表示が失敗する
	CHAR  mCharSet[32];   // キャラクタセット
#endif
    CHAR  mKeyword[64];   // 検索文字列キー(Bcc:/Cc:など)
    CHAR  mToken[64];     // 検索文字列
#ifdef UPDATE_20230624A // SEARCH BODY命令で検索速度の向上対策:検索キーワード側を文字コード別に準備して比較する
    CHAR  mTokenUTF8[64];    // 検索文字列
    CHAR  mTokenSJIS[64];    // 検索文字列
    CHAR  mTokenJIS[64];     // 検索文字列
    CHAR  mTokenEUC[64];     // 検索文字列
#endif
#ifdef UPDATE_20230626 //QuotedPrintableの比較の追加
    CHAR  mTokenQUTF8[256];    // 検索文字列
    CHAR  mTokenQSJIS[256];    // 検索文字列
    CHAR  mTokenQJIS[256];     // 検索文字列
    CHAR  mTokenQEUC[256];     // 検索文字列
#endif
} IMAP4STK, *PIMAP4STK;

typedef struct _Imap4Envelope {
	CHAR mDate[64]; // eg. "Thu, 23 Dec 1999 09:08:29 +0900 (JPT)"
#ifdef UPDATE_20110124A // Subject:のFetchで途中で文字が切れるないように変数領域を増やした。
	CHAR mSubject[2048];
#else
	CHAR mSubject[256]; 
#endif
#ifdef UPDATE_20231117 // FTECH ENVELOPEでFROMの定義可能な上限を増やした。
	CHAR mFrom[2048]; 
#else
#ifdef UPDATE_20071205 // 外部メールアドレス長が256Byteのときの対策
	CHAR mFrom[256]; 
#else
	CHAR mFrom[128]; 
#endif
#endif
#ifdef UPDATE_20230914 // Reply-to:,Sender:ヘッダの情報取得でメモリリークする場合がある
	CHAR mSender[256]; 
	CHAR mReplyto[256];
#else
	CHAR mSender[128]; 
	CHAR mReplyto[128];
#endif	
#ifdef UPDATE_20230707 // FTECH ENVELOPEでTO,CC,BCCのアドレス定義可能な上限を増やした。
	CHAR mTo[0x20000]; 
	CHAR mCc[0x20000]; 
	CHAR mBcc[0x20000]; 
#else
	CHAR mTo[1024]; 
	CHAR mCc[1024]; 
	CHAR mBcc[1024]; 
#endif
	CHAR mInreplyto[128]; 
	CHAR mMessgeid[128]; 
} Imap4Envelope, * PImap4Envelope;

#ifdef UPDATE_20150630 // MESSAGE/RFC822の構造出力への対応
typedef struct _PARTSTRUCTURE_M {
#ifdef UPDATE_20150712      // MIME構造の階層管理に親パート番号を設定できる項目を追加した。
	DWORD nPPart;            // 親パート番号
#endif
    CHAR  mBType[32];       // ボディタイプ Content-Type: text/plain
#ifdef UPDATE_20150601 // MS-OFFICEファイルのMIMEタイプが長すぎて途中で切られてしまう
    CHAR  mBSub[128];        // ボディサブタイプ Content-Type: (charset)/(BOUNDARY --ASW????....)
#else
    CHAR  mBSub[32];        // ボディサブタイプ Content-Type: (charset)/(BOUNDARY --ASW????....)
#endif
#ifdef UPDATE_20230929 // 256byteを越えるファイル名がMIME-Qエンコードで指定されていると正しいファイルが取得できない
    CHAR  mBPara[1024];       // ボディパラメータ括弧つきリスト ("name" "test.bin")
#else
    CHAR  mBPara[256];       // ボディパラメータ括弧つきリスト ("name" "test.bin")
#endif
	CHAR  mBID[256];         // ボディ ID Content-ID: <Mm3V1n13r86nICg78>
#ifdef UPDATE_20150617C // HEADER(Content-Disposition:)構造にfilename*N*=が複数行にわたり記載されていると正しく抽出できない不具合
    CHAR  mBDisp[1024];       // ボディ記述 Content-Disposition: attachment;
#else
    CHAR  mBDisp[256];       // ボディ記述 Content-Disposition: attachment;
#endif
    CHAR  mBEnc[256];        // ボディ符号化 Content-Encoding: US-ASCII,base64
	ULARGE_INTEGER   uHead;  // ヘッダサイズ：
	ULARGE_INTEGER   uBody;  // ボディサイズ：
	DWORD nHLine;            // ヘッダ部行数;
	DWORD nLine;             // 本文行数;
} PartStruct_M, *PPartStruct_M;
#endif

typedef struct _PARTSTRUCTURE {
#ifdef UPDATE_20150712      // MIME構造の階層管理に親パート番号を設定できる項目を追加した。
	DWORD nPPart;            // 親パート番号
#endif
    CHAR  mBType[32];       // ボディタイプ Content-Type: text/plain
#ifdef UPDATE_20150601 // MS-OFFICEファイルのMIMEタイプが長すぎて途中で切られてしまう
    CHAR  mBSub[128];        // ボディサブタイプ Content-Type: (charset)/(BOUNDARY --ASW????....)
#else
    CHAR  mBSub[32];        // ボディサブタイプ Content-Type: (charset)/(BOUNDARY --ASW????....)
#endif
#ifdef UPDATE_20230929 // 256byteを越えるファイル名がMIME-Qエンコードで指定されていると正しいファイルが取得できない
    CHAR  mBPara[1024];       // ボディパラメータ括弧つきリスト ("name" "test.bin")
#else
    CHAR  mBPara[256];       // ボディパラメータ括弧つきリスト ("name" "test.bin")
#endif
	CHAR  mBID[256];         // ボディ ID Content-ID: <Mm3V1n13r86nICg78>
#ifdef UPDATE_20150617C // HEADER(Content-Disposition:)構造にfilename*N*=が複数行にわたり記載されていると正しく抽出できない不具合
    CHAR  mBDisp[1024];       // ボディ記述 Content-Disposition: attachment;
#else
    CHAR  mBDisp[256];       // ボディ記述 Content-Disposition: attachment;
#endif
    CHAR  mBEnc[256];        // ボディ符号化 Content-Encoding: US-ASCII,base64
#ifdef UPDATE_20150630 // MESSAGE/RFC822の構造出力への対応
	ULARGE_INTEGER   uHead;  // ヘッダサイズ：
#endif
	ULARGE_INTEGER   uBody;  // ボディサイズ：
#ifdef UPDATE_20150630 // MESSAGE/RFC822の構造出力への対応
	DWORD nHLine;            // ヘッダ部行数;
#endif
	DWORD nLine;             // 本文行数;
#ifdef UPDATE_20150630 // MESSAGE/RFC822の構造出力への対応
	PartStruct_M  PSM;
#endif
} PartStruct, *PPartStruct;

typedef struct _Imap4Context {
    IMAP4State          State;          // State of the connection
    DWORD               Command;
	HLOCAL              hContext;
	BOOL                bCountLock;
    DWORD               LastError;      // Last error occurred
    DWORD               RetryCount;     // Number of retries
#ifdef Y2038_BUG
	SYSTEMTIME          ltime;          // inlog time
#else
	time_t              ltime;          // inlog time
#endif
    CHAR                LocalName[256]; // Local Host Name
#ifdef IPv6
	CHAR                MyAddr[INET6_ADDRSTRLEN]; // My Host Address IPv6
	CHAR                PeerAddr[INET6_ADDRSTRLEN]; // Peer Host Address IPv6
#else
	CHAR                MyAddr[21];     // My Host Address xxx.xxx.xxx.xxx
	CHAR                PeerAddr[21];   // Peer Host Address xxx.xxx.xxx.xxx
#endif
#ifdef UPDATE_20151126 // リッスンＩＰ毎に証明書を選択できる対策
    BOOL                bUsedSTLS;
    CHAR                mCertificate[256];
    CHAR                mPrivatekey[256];
#endif
	INT                 nConnectPort;
	BOOL                bUsedSSL;
    CHAR                PeerName[256];  // Peer Host Name
    CHAR                PeerHelo[256];  // Peer Host Helo Token
	CHAR                mToken[1024 * BUFFER_ARREY]; //2048];   // Command work
	CHAR                mMessages[FETCH_BODY_BUFFER+1]; // Answer Messages
	CHAR                mCash[FETCH_BODY_BUFFER+1];// キャッシュ Answer Messages
#ifdef UPDATE_20151115 // SELECT時にメール一覧をキャッシュするためのポインタ
	SFL                 *pFnLists;
#endif
	DWORD               nExists;        // MSG 総数
	DWORD               nRecent;        // MSG 新着
	DWORD               nUnseen;        // MSG 未読
	DWORD               nUid;           // フォルダ毎のユニーク識別番号
	DWORD               nUidValidity;   // ユニーク識別子有効値
#ifdef UPDATE_20110414 // NOOPタイマー（一定時間新着がないNOOPでセッション強制切断する対策（負荷軽減対策）
    SYSTEMTIME          nNOOPst;
#endif
	CHAR                fullname[256];
	CHAR                mUSER[_MAX_PATH];  // User ID
	CHAR                mPASS[_MAX_PATH];  // User PASSWORD
	CHAR                mAUTHCODE[256];
	CHAR                mAUTHCODEBASE64[256];
	CHAR                mAUTHUser[256];
	CHAR                mAUTHPass[256];
	CHAR                mRootDir[_MAX_PATH];    // Imap root folder(= POP3 INBOX)
	CHAR                mSelectDir[_MAX_PATH];  // Imap select folder
    CHAR                mSharedRoot[_MAX_PATH];
    CHAR                mLinkRoot[_MAX_PATH];
    BOOL                bShared;
    BOOL                bSharedRW;
	CHAR                *pSquence;      // シーケンス番号
	CHAR                *pCmd;          // 命令
	CHAR                *pToken;        // 命令内容
#ifdef UPDATE_20230617 // SEARCH命令でOR指定が含まれると正しく検索結果が出力されない不具合
	INT                 nMode;          // 0:FLAG/1:番号/2:内部日付/3:ファイルサイズ/4:メッセージ/5:その他
#endif
	BOOL                bUID;           // UIDによる命令
#ifdef Y2038_BUG
	SYSTEMTIME          lt;          // inlog time
#else
    struct tm           lt;
#endif
	CHAR                *p;
    WIN32_FIND_DATA     FindData;
	DWORD               nLogInID; // ID設定
	BOOL                bEncMSG; // MSGが暗号化されている場合 TRUE
	CHAR                mDecFileName[MAX_PATH]; // MSGが暗号化されている場合の複号化されたファイル
#ifdef UPDATE_20140528 // IDLEコマンド実装
	BOOL                bIDLE; // IDLE状態フラグ TRUE:IDLE起動中 FALSE:IDLE停止中
	BOOL                bIDLEThread; // IDLEスレッド実行中 TRUE:起動中 FALSE:停止中
	HANDLE              hIDLEThread; // IDLEスレッドハンドル
	CHAR                mIDLESquence[80];      // シーケンス番号
	DWORD               nIDLEExists;        // MSG 総数
	DWORD               nIDLERecent;        // MSG 新着
#endif
#ifdef UPDATE_20230627A // UPDATE_20230624のメールデータを事前に一括して読込み検索するフラグを追加
	CHAR                *pBuff;
#endif
#ifdef UPDATE_20230706 // FETCH時のメールデータを事前に一括して読込み検索するフラグを追加
	CHAR                *pfBufp;
	CHAR                *pfBuff;
#endif
} Imap4Context, * PImap4Context;

typedef struct _CLIENT_CONTEXT {
	BOOL   bUsed;
    HANDLE hCompletionPort;
#ifdef USE_SSL
    SSL_CTX *ctx;
    SSL    *ssl;
	BOOL   bThreadEnd;
#endif
    SOCKET Socket;
#ifdef IPv6
    SOCKADDR_IN6 sin61;
#endif
    SOCKADDR_IN sin1;
    int         nsin1;
    PHOSTENT     phsin1;
	PHOSTENT     phsin2;
#ifndef VC6
	struct addrinfo *AI;
#endif
#ifdef UPDATE_20141221 // セッション内でのメッセージ送信の排他処理
	BOOL        bPutLock; // 同報メッセージ送信用排他フラグ
#endif
    //PVOID Context;
	Imap4Context Context;
	Imap4Envelope Envelope;
	//CHAR SndBuf[1024 * BUFFER_ARREY];
	//CHAR RcvBuf[1024 * BUFFER_ARREY]; //16];
    CHAR Buffer[1024 * BUFFER_ARREY]; //16];
	CHAR Queue[1024 * BUFFER_ARREY]; //16];
    CHAR SSLBuffer[1024 * BUFFER_ARREY]; //16];
} CLIENT_CONTEXT, *PCLIENT_CONTEXT;

typedef struct _TMQueue {
  PCLIENT_CONTEXT pCurrent;
  BOOL        bwait; // waiting flag
  BOOL        btrans; // Data Transmit mode flag
#ifdef Y2038_BUG
  FILETIME    ltime; // last in time
#else
  time_t      ltime; // last in time
#endif
  VOID        *pNext;
} TMQueue;

typedef struct ldap_data
{
  char *pHost;
  DWORD nPort;
  char *pLogonDomain;
  char *pLogonID;
  char *pLogonPW;
  char *pScope;
  char *pMailAddr;
  char *pRequest1;
  char *pRequest2;
  char *pAnswer;
} LDAPD;

