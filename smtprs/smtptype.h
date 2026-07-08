////////////////////////////////////////////////////////////
// SmtpType.h Copyright K.kawakami
////////////////////////////////////////////////////////////
#ifndef  _SMTPTYPE_H
#define  _SMTPTYPE_H
#include <windows.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
//#include "debug.h"

#ifdef MILTER_ON // MILTERインターフェースを追加。
#define M_VERSION   6L

// メールサーバ側ID
#define M_NEGO     'O' // Ver Act Step Macro           | ネゴシェーション
#define M_CONNECT  'C' // {HOST}{0}{Family}{Addr}{Port}| 接続
#define M_HELO     'H' // FQDN                         | HELO/EHLO(FQDN)
#define M_FROM     'M' // ENVELOPE FROM                | MAIL FROM
#define M_RCPT     'R' // ENVELOPE RECIPIENT           | RCPT TO
#define M_DATA     'T' // メール本体                   | DATA
#define M_HEADER   'L' // ヘッダ行   　                | HEADER LINE
#define M_EOH      'N' // ヘッダ終了                   | HEADER END
#define M_BODY     'B' // 本文行                       | BODY LINE
#define M_EOM      'E' // 本文終了                     | BODY END
#define M_DISCONN  'Q' //                              | 切断
#define M_ABORT    'A' //                              | 切断
#define M_MACRO	   'D' // Define macro 

// メールフィルタ側ID
#define F_RCPTADD  '+' // 宛先追加: 宛先を追加する応答。複数の宛先を追加するときは複数回送る。
#define F_RCPTDEL  '-' // 宛先削除: 宛先を削除する応答。「何番目の宛先」という形式で指定する。
#define F_RCPTADD2 '2' // パラメーター付きで宛先追加: <rcpt-parameters>付きで宛先を追加する。
#define F_OK       'a' // 受理: このメールは受理するという応答。milterプロトコルでのメールトランザクションの処理はここで終了し、これ以降MTAからコマンドは送られてこない。
#define F_DATAMOD  'b' // 本文置換: 本文を変更する応答。変更後の本文が大きい場合は複数のチャンクに分けて応答する。
#define F_NEXT     'c' // 継続: 次のコマンドへいってくれという応答。基本はこれを返す。
#define F_DISCON   'd' // 破棄: 処理中のメッセージを破棄する。SMTPでのメールトランザクションの処理はここで終了するので、milterプロトコルでのメールトランザクションも終了する。
#define F_FROMREP  'e' // 差出人変更: 差出人を変更する応答。
#define F_HEADADD  'h' // ヘッダー追加: ヘッダーを追加する応答。複数のヘッダーを追加するときは複数回送る。ヘッダーは末尾に追加される*7。
#define F_HEADINS  'i' // ヘッダー挿入: 指定した位置にヘッダーを挿入する応答。
#define F_HEADMOD  'm' // ヘッダー変更: 指定した位置のヘッダーを変更する。
#define F_EXECUTE  'p' // 処理中: milterの処理に時間がかかっていることをMTAに知らせる応答。MTA側のタイムアウト時間を伸ばせる。
#define F_ISOLATE  'q' // 隔離: このメールを隔離するという応答。
#define F_REJECT   'r' // 拒否: このメールの受信を拒否するという応答。SMTPでは5XX系のレスポンスになる。
#define F_SKIP     's' // スキップ: このメールトランザクションの処理を途中でやめるという応答。
#define F_TEMP     't' // 一時拒否: このメールの受信を一時的に拒否するという応答。SMTPでは4XX系のレスポンスになる。
#define F_RESPONSE 'y' // SMTPレスポンス設定: SMTPレスポンスのコードとメッセージを設定する。

// ACTフラグ
#define ACT_ADDHEADER 0x00000001L // ヘッダーを追加できる
#define ACT_MODBODY   0x00000002L // 本文を変更できる
#define ACT_ADDTO     0x00000004L // 宛先を追加できる
#define ACT_DELLTO    0x00000008L // 宛先を削除できる
#define ACT_MODHEADER 0x00000010L // ヘッダーを変更できる
#define ACT_HOLD      0x00000020L // 隔離（配送せずにholdキューに入れる）できる
#define ACT_MODFROM   0x00000040L // 差出人を変更できる
#define ACT_ADDTOPARA 0x00000080L // パラメーター付きで宛先を追加できる（RCPT TO:<forward-path> [ SP <rcpt-parameters> ] <CRLF>の<rcpt-parameters>を使うかどうか）
#define ACT_MODMACRO  0x00000100L // メールフィルタ側がマクロ定義を上書きできるか（詳細は後述するマクロリストを参照）

// STEP フラグ
#define STEP_MTA_NOCONN   0x00000001L // MTAが接続コマンドを送らない
#define STEP_MTA_NOHELO   0x00000002L // MTAがHELOコマンドを送らない
#define STEP_MTA_NOFROM   0x00000004L // MTAがMAIL FROMコマンドを送らない
#define STEP_MTA_NOTO     0x00000008L // MTAがRCPT TOコマンドを送らない
#define STEP_MTA_NOBODY   0x00000010L // MTAが本文チャンクコマンドを送らない
#define STEP_MTA_NOHEAD   0x00000020L // MTAがヘッダーコマンドを送らない
#define STEP_MTA_NOEOH    0x00000040L // MTAがヘッダー終了コマンドを送らない
#define STEP_MFT_NOHEADER 0x00000080L // メールフィルタ側がヘッダーコマンドに応答しない
#define STEP_MTA_NOUNKOWN 0x00000100L // MTAが未知コマンドを送らない
#define STEP_MTA_NODATA   0x00000200L // MTAがDATAコマンドを送らない
#define STEP_MTA_SKIP     0x00000400L // MTAがスキップ応答（後述）をサポートしているかどうか
#define STEP_MTA_REJTO    0x00000800L // MTAが拒否した宛先もメールフィルタ側に送るかどうか
#define STEP_MFT_NOCONN   0x00001000L // メールフィルタ側が接続コマンドに応答しない
#define STEP_MFT_NOHELO   0x00002000L // メールフィルタ側がHELOコマンドに応答しない
#define STEP_MFT_NOFROM   0x00004000L // メールフィルタ側がMAIL FROMコマンドに応答しない
#define STEP_MFT_NOTO     0x00008000L // メールフィルタ側がRCPT TOコマンドに応答しない
#define STEP_MFT_NODATA   0x00010000L // メールフィルタ側がDATAコマンドに応答しない
#define STEP_MFT_NOUNKOWN 0x00020000L // メールフィルタ側が未知コマンドに応答しない
#define STEP_MFT_NOEOH    0x00040000L // メールフィルタ側がヘッダー終了コマンドに応答しない
#define STEP_MFT_NOBODY   0x00080000L // メールフィルタ側が本文チャンクコマンドに応答しない
#define STEP_MTA_NOWSP    0x00100000L // MTAがヘッダーの値の先頭の空白を削除しない。「Subject: xxx」とあった場合、先頭の空白を削除しないで「 xxx」をメールフィルタ側に送るということ。このフラグを落とすと先頭の空白を削除して「xxx」をメールフィルタ側に送る。

// マクロフラグ
#define MA_CONNECT 0
#define MA_HELO    1
#define MA_MAIL    2
#define MA_RCPT    3
#define MA_DATA    4
#define MA_EOM     5

typedef struct _NEGODATA {
unsigned int sz; // data size
char id;          // id
char data[16];    // ver, act, step, macro
//unsigned int ver; // version
//unsigned int act; // action
//unsigned int step; // step
//unsigned int macro; // macro
} ND, *PND;

typedef struct _FILTERDATA {
unsigned int sz; // data size
char id;          // id
char *pData;      // Data
} FD, *PFD;
#endif

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

#ifdef ESMTP_AUTH
#ifdef USE_STARTTLS
  #define MAX_SMTP_COMMAND  15
#else
  #define MAX_SMTP_COMMAND  14
#endif
#else
  #define MAX_SMTP_COMMAND  11
#endif
#define APPLICATION_NAME  TEXT(SMTP_NAME)

typedef enum _LAST_CLIENT_IO {
    ClientIoRead,
    ClientIoWrite,
    ClientIoTransmitFile
} LAST_CLIENT_IO, *PLAST_CLIENT_IO;

typedef enum _SMTPRState {
    SmtpNegotiate,
#ifdef ESMTP_AUTH
    SmtpEGreeting,                       // EHLO phase
#ifdef USE_STARTTLS
	SmtpEGreetingTLS,                    // STARTTLS
#endif
    SmtpAuthentication,                  // AUTH phase
//    SmtpPassword,                        // PASS word phase
#endif
    SmtpGreeting,                        // HELO phase
	SmtpMailFrom,
	SmtpRecpient,
    SmtpDataError,                       // DATA error state
    SmtpData,                            // DATA state
    SmtpShutdown                        // Rundown state
} SMTPRState, * PSMTPRState;

typedef struct _SmtpMessageHeader {
    DWORD       Flags;                  // Flags about this message
    DWORD       Size;                   // Size of this message
    PWSTR       pszFileName;            // File name containing message
} SmtpMessageHeader, * PSmtpMessageHeader;

typedef struct _SmtpMailDirectory {
    DWORD               Flags;          // Flags about this directory
    DWORD               cMessages;      // Number of messages
    DWORD               NextMessage;    // Next message number
    DWORD               TotalSize;      // Total size of directory, in bytes
    DWORD               cAvailMessages; // Number of messages not deleted
    DWORD               AvailSize;      // Size of available messages
    PWSTR               pBaseDir;       // Base directory
    PSmtpMessageHeader   Messages;       // Array of message headers
} SmtpMailDirectory, * PSmtpMailDirectory;

typedef struct _SmtpContext {
    SMTPRState          State;          // State of the connection
    DWORD               Command;
	HLOCAL              hContext;
	BOOL                bCountLock;
    DWORD               LastError;      // Last error occurred
    DWORD               RetryCount;     // Number of retries
    //PSmtpMailDirectory  pDirectory;     // Directory for retrieval
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
	INT                 nConnectPort;
	BOOL                bUsedSSL;
    CHAR                mmode[64];
    CHAR                PeerName[256];  // Peer Host Name
    CHAR                PeerHelo[256];  // Peer Host Helo Token
#ifdef ADD_XOAUTH2 // OAUTH2での認証方式を追加
	CHAR                mToken[4096];   // Command work
#else
	CHAR                mToken[2048];   // Command work
#endif
	CHAR                mMessages[640]; //[512; // Answer Messages
	DWORD               nMsgId;         // Message ID
#ifdef UPDATE_20260610B // 本文情報がブラックリストにヒットした場合はinlogへの記録はしない
	DWORD                nMsgId2; // Answer Messages
#endif
#ifdef K_SEARCH // K_SEARCH OEM 版
	CHAR                mMsgId[256]; // Answer Messages
#ifdef UPDATE_20260610B // 本文情報がブラックリストにヒットした場合はinlogへの記録はしない
	CHAR                mMsgId2[256]; // Answer Messages
#endif
#endif
#ifdef UPDATE_20071204  // メッセージＩＤ採番処理を修正(Bym10id)
	CHAR                mMsgId[256];
#endif
#ifdef UPDATE_20260610B // 本文情報がブラックリストにヒットした場合はinlogへの記録はしない
	CHAR                mMsgId2[256]; // Answer Messages
#endif
	CHAR                fullname[256];
#ifdef UPDATE_20220728 // RFC5831(821/2821) でエンベロープFROMの書式違反の判定フラグの追加
    BOOL                bRFC3Dot3Sec;        // 3.3 Mail Transactions 規約違反  正しい書式は半角スペースが含まれない=>"MAIL FROM:<reverse-path> [SP <mail-parameters> ] <CRLF>"
#endif
	BOOL                bBlankFROM;     // if FROM Address <> Then TRUE
	BOOL                bBlankFROMPattan;// if FROM Address <> Then TRUE
	BOOL                bBlankFROMNumber; // if FROM Address xxx@xxx.xxx Then TRUE
	BOOL                bBlankFROMNoDomain;// if FROM Address xxx@xxx.xxx Then TRUE
	BOOL                bBlankFROMHost;  // if FROM Address xxx@xxx Then TRUE
	BOOL                bFROM;           // if FROM Address LocalDomain User then TRUE
	BOOL                bDomainFROM;
	BOOL                bOutSideAliases;
#ifdef UPDATE_20070516 // 上長承認機能の追加
	BOOL                bTopRcpto;            // 同報時の最初の送信先か判定用 Yes:TRUE No:FALSE
	DWORD               nOption;             // 承認条件 0:不要,1:全て,2:添付があるとき
	CHAR                mBOSSSubject[_MAX_PATH*2];  // 上長アドレス
	CHAR                mBOSS[_MAX_PATH*4];  // 上長アドレス
#ifdef UPDATE_20080617 // 上長承認機能で添付ファイルの拡張子による承認条件追加
	CHAR                mMIME[_MAX_PATH];   // 添付ファイル拡張子条件
#endif
#endif
#ifdef SENDERID
	CHAR                mSUBMITTER[_MAX_PATH];  // Submitter
#endif
	CHAR                mUIDFROM[_MAX_PATH*2];  // FROM User ID
	BOOL                bRCPT;
	BOOL                bDomainRCPT;
	BOOL                bSubDomainRCPT;
	CHAR                mUIDRCPT[_MAX_PATH*2];  // RCPT User ID
    DWORD               nRCPTCount;
	BOOL                bRCPTReset;
	BOOL                bVRFY;
	BOOL                bDomainVRFY;
	CHAR                mUIDVRFY[_MAX_PATH*2];  // VRFY User ID
	BOOL                bMList;         // Mailling List address flag
	CHAR                mFnRset[256];   // Header Rest work file name
	CHAR                mFnRCP[256];    // RCP work file name
	CHAR                mFnHead[256];   // Header work file name
	CHAR                mFnData[256];   // Data work file name
	CHAR                mFnMSG[256];    // MSG temp file name
	CHAR                mFnTemp[256];   // Data temp file name
    BOOL                bSave;
	CHAR                mEnd[6];        // Data end marck check
	CHAR                mtkn[16];
#ifdef UPADTE_20031120
    CHAR                mtkn2[256];
#endif
	CHAR                mLogFn[256];
	CHAR                mAcptLogFn[256];
	CHAR                mAcptLogState[512];
	BOOL                bAcptData;
	CHAR                mtime[256];
	CHAR                mdata[1024]; //[256];
	CHAR                mhdat[256];
#ifdef ESMTP_AUTH
	CHAR                mUSER[_MAX_PATH];  // User ID
	CHAR                mPASS[_MAX_PATH];  // User PASSWORD
	BOOL                bAUTHSUCCESS;
	INT                 nAUTHMODE; // 0=PLAIN 1=LOGIN1 2=LOGIN2 3=CRAM-MD5
#ifdef UPDATE_20050927 // SMTP AUTH のユーザＩＤ情報の格納 個人別対応
	DWORD               nFROMSecLevel;
#endif
#ifdef UPDATE_20151126 // リッスンＩＰ毎に証明書を選択できる対策
    BOOL                bUsedSTLS;
    CHAR                mCertificate[256];
    CHAR                mPrivatekey[256];
#endif
	CHAR                mAUTHCODE[512];
	CHAR                mAUTHCODEBASE64[512];
	CHAR                mAUTHUser[256];
	CHAR                mAUTHPass[256];
#endif
#ifdef UPDATE_20220603 // 接続時のTLSバージョンとChiperの表記を追加した。
	CHAR                mTLSInfo[256];
#endif
	CHAR                *pCmd;          // 命令
	CHAR                *pToken;        // 命令内容
#ifdef Y2038_BUG
	SYSTEMTIME          lt;          // inlog time
#else
    struct tm           lt;
#endif
	CHAR                *p;
	CHAR                *pdom;
	CHAR                *pldom;
    CHAR                muid[_MAX_PATH*2];
#ifdef UPDATE_20090818 // 付加表題のデータサイズを256Byteまで有効にする対策
	CHAR                mMLtkn[512];
#else
	CHAR                mMLtkn[128];
#endif
	DWORD               nWhoCanSend;
	PHOSTENT            phe;
	BOOL                bFL;
	BOOL                bHead;
	BOOL                bID;
	BOOL                bDate;
	BOOL                bReplyTo;
	BOOL                bFrom;
	BOOL                bSubject;
#ifdef UPDATE_20230620 // 受信メールに任意のヘッダを追加するオプション
	BOOL                bSender;
	BOOL                bReturnPath;
	CHAR                mReplyTo[256];
	CHAR                mSender[256];
	CHAR                mReturnPath[256];
#endif
	DWORD               nReceived;
	DWORD               nTotalData;
#ifdef UPDATE_20050214
	CHAR                mMessIDD[256];
#endif
    DWORD               i;
    DWORD               n;
    DWORD               m;
	DWORD               nbtm[2];
	DWORD               nmask;
#ifdef UPDATE_20150319 // エンベロープの送信元によりメール受信の許可をする場合
    CHAR                meffect[1024];
#else
    CHAR                meffect[256];
#endif
	CHAR                mip[40]; //mip[21];
	CHAR                eaddr[256];
    CHAR                *ep;
    CHAR                *ebp;
	CHAR                *mp;
    BOOL                bebp;
    BOOL                bonly;
	BOOL                bdefault;
	BOOL                bnorelay;
	BOOL                bnoauth;
    BOOL                sts;
    BOOL                bmask;
    HANDLE              hFindFile;
    WIN32_FIND_DATA     FindFileData ;
    BOOL                bDiskStatus;
	FILE                *Crackfp;       // SMTPRSCrackCommand File Pointer
	FILE                *Mailfp;        // MAIL FROM File Pointer
	FILE                *Effefp;        // Effective File Pointer
	FILE                *Rsetfp;        // RSET File Pointer
	FILE                *Rcptfp;        // RCPT File Pointer
	FILE                *Headfp;        // HEAD File Pointer
	FILE                *Datafp;        // DATA File Pointer
	FILE                *Tempfp;        // TEMP File Pointer
	FILE                *Lockfp;        // LOCK File Pointer
	FILE                *Logfp;         // LOG File Pointer
	FILE                *RCPfp;         // RCP  File Pointer
#ifdef DATA_CASH
	char                mFrbuf[0x4000]; //0x7ffe];  // ファイルアクセスバッファの
	char                mFsbuf[0x4000]; //0x7ffe];  // ファイルアクセスバッファの
	char                mFwbuf[0x4000]; //0x7ffe];  // 有効範囲は 2 < size < 32768 
#endif
} SmtpContext, * PSmtpContext;

#ifdef MILTER_ON // MILTERインターフェースを追加。
struct _MILTER
{
  SOCKET s;
  HANDLE h;
  int    sts;
  unsigned int macrosz;
  int    nMLTLen; // Command length
  char   *pType;
  char   *pPort;
  char   *pAddr;
  char   *pAction;
  char   *pCTimer;
  char   *pSTimer;
  char   *pRTimer;
  char   *pETimer;
  struct _NEGODATA mFnd;
  char   mType[80];
  char   mPort[12];
  char   mAddr[80];
  char   mAction[16];
  char   mCTimer[16];
  char   mSTimer[16];
  char   mRTimer[16];
  char   mETimer[16];
  CHAR   mMLTToken[1024*16]; //[2048];   // Command work
  CHAR   mMLTMessages[2048]; //[512; // Answer Messages
};
#endif

typedef struct _CLIENT_CONTEXT {
	BOOL   bUsed;
    HANDLE hCompletionPort;
	DWORD  dwThreadId;
#ifdef USE_SSL
    SSL_CTX *ctx;
    SSL    *ssl;
#endif
    SOCKET Socket;
#ifdef IPv6
    SOCKADDR_IN6 sin61;
#endif
//#else
    SOCKADDR_IN sin1;
//#endif
    int         nsin1;
    PHOSTENT     phsin1;
	PHOSTENT     phsin2;
#ifndef VC6
	struct addrinfo *AI;
#endif
#ifdef MILTER_ON // MILTERインターフェースを追加。
	int nMilter; // milter数
    struct _MILTER *pMLT;
#ifdef IPv6
    SOCKADDR_IN6 **pMsock6;
#endif
#endif
    //PVOID Context;
	SmtpContext Context;
    LAST_CLIENT_IO LastClientIo;
    DWORD BytesReadSoFar;
	DWORD dwBytesRead;
    OVERLAPPED Overlapped;
	//CHAR SndBuf[1024];
	//CHAR RcvBuf[1024*16];
    CHAR Buffer[1024*16];
    CHAR SSLBuffer[1024*16];
} CLIENT_CONTEXT, *PCLIENT_CONTEXT;

typedef struct _TMQueue {
  PCLIENT_CONTEXT pCurrent;
  BOOL        bwait; // waiting flag
#ifdef Y2038_BUG
  FILETIME    ltime; // last in time
#else
  time_t      ltime; // last in time
#endif
  VOID        *pNext;
} TMQueue;

typedef enum _SMTPRSDisposition {
    SMTPRS_Discard,                       // Discard the request
    SMTPRS_SendError,                     // Send the error string
    SMTPRS_SendBuffer,                    // Send the buffer returned
    SMTPRS_SendFile,                      // Send the File returned
    SMTPRS_SendBufferThenFile,            // Send the buffer, then the file
    SMTPRS_SendFileThenBuffer,            // Send the file, then the buffer
    SMTPRS_Quit                           // Quit the request
} SMTPRSDisposition;

typedef SMTPRSDisposition
(* SMTPRSDispatchFn)(
    PSmtpContext pContext,               // Client's connection context
    PUCHAR      InputBuffer,            // Buffer sent from client
    DWORD       InputBufferLen,         // Size of buffer
    PHANDLE     SendHandle,             // Handle of file to send
    PUCHAR *    OutputBuffer,           // Output buffer to send
    PDWORD      OutputBufferLen         // Size of output buffer
    );

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

#endif