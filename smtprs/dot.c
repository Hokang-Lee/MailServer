////////////////////////////////////////////////////////////
// dot.c Copyright K.kawakami
////////////////////////////////////////////////////////////
#include "smtp.h"
#include <share.h>
#ifdef UPDATE_20080229 // 承認保留一覧に各メールの題名を表示する
#include "codepage.h"
#endif

#ifdef LGWAN_ON        // LGWAN機能拡張
extern BOOL bLGWAN;
extern BOOL bCHGHEADER;
#endif
#ifdef FIRST_TIME_MAIL // 繰り返す同一内容のメール除去
extern BOOL	bFTM_ON;    // 同一メールの保管チェックの有無
#endif

extern BOOL bDebug;
extern BOOL bVLog; // イベントビューワにログ書込みエラーを表示する 0:しない　1:する
extern BOOL bServiceTerminating;
extern BOOL bLog;
extern CHAR  mMailBoxDir[];
extern char mMailSpoolDir[];
extern char mMailQueueDir[];
extern char mProgPath[];
#ifdef K_SEARCH // K_SEARCH OEM 版
extern char mMailQueueSubDir[];
#endif
#ifdef UPDATE_20061102 // 受信メールのエンベロープと本文の任意のフォルダへのバックアップ機能の追加
extern BOOL bMailBackup;
extern char mMailBackupDir[];
#ifdef UPDATE_20090205 // バックアップメッセージの拡張子を指定するオプション
extern char   mExtension[256];
#endif
#endif
extern char mTempDir[];
extern char mLockFile[];
extern DWORD   nMailInMaxSize;
extern DWORD   nReceived;
char    mTraceFile[256];
FILE    *fTrace;
#ifdef Y2038_BUG
extern char mMonth[12][4];
extern char mWeek[7][4];
#endif
#ifdef CLUSTERING
extern BOOL   nClustering;
extern char   mComName[];
#endif
#ifdef UPDATE_20070516 // 上長承認機能の追加
#ifdef UPDATE_20090213 // 上長承認メールのmailto:リンクの区切り記号を"?"以外に変更するオプションを追加
extern char   mMailApprovalMailTo[];
extern char   mMailApprovalMID[];
#endif
extern BOOL   bMailApproval;
extern BOOL   bMailApprovalLog;
#ifdef UPDATE_20231017 // 上長承認依頼メールの作成に失敗した場合の作成リトライ
extern int    nMailApprovalMessNum;    // BossCheck メッセージ作成失敗時のリトライ回数
#endif
#ifdef UPDATE_20140623B // // 上長承認機能の履歴作成時の詳細さ設定フラグを追加した（送信元IPや送信先を）0:含まない(デフォルト) 1:含む
extern int    nMailApprovalLogLevel;
#endif
extern char   mMailApprovalDir[];
extern char   mMailApprovalManager[];
#ifdef UPDATE_20090114 //BossCheck 承認者数の設定
extern DWORD   nMailApprovalNum;
#endif
#ifdef UPDATE_20080221 //BossCheck 承認者の代理人の代理人の深さの設定
extern DWORD  nMailApprovalDepath;
#endif
#ifdef UPDATE_20080225 // 部下に対する承認のため保留通知
extern BOOL   bMailApprovalRes;
#endif
#ifdef UPDATE_20161111 /// 外部からの送信元送信先制限(逆上長承認)
extern BOOL   bMailApprovalRejectRes;    // 却下通知の有効有無
#endif
#ifdef UPDATE_20080514 // 上長宛承認メールのmailto:で記載した返信先アドレスのドメインを強制変更する設定
extern char   mMailApprovalDomain[];
#endif
#ifdef UPDATE_20080620 // TOアドレスが代理人になる機能追加
extern BOOL   bSetProxyUserType;
#endif
#ifdef  INCOM_SUB_FOLDER // incoming フォルダをスレッド別フォルダにして処理
extern BOOL   bIncomingSubFolder; // incomingフォルダにスレッド別フォルダを作成 TRUE:する FALSE:しない
extern BOOL	  bThreadType;
extern DWORD  nMaxThread;
#endif
#ifdef UPDATE_20090910 // report-type="disposition-notification" なら添付無しと判定する
extern BOOL   bMailApprovalNotification;
#endif
#ifdef UPDATE_20091203 // 承認、否承認を一括処理するためのワイルドカード指定機能のON/OFFオプションを追加した。
extern BOOL   bMailApprovalWildcard;
#endif
#ifdef UPDATE_20141210 // 承認依頼メールのmailtoリンクの文字コード指定
extern INT     nMailApprovalCodepage;
#endif
#ifdef DOMAIN_AUTH_DKIM    // 2013.01.17 ドメイン認証 DomainkeysとDKIM方式 追加
#ifdef UPDATE_20240118 // DKIM設定箇所の選択 bit0:(受信時付加)デフォルト bit1:上長承認メールへ付加 bit2:添付分離メールへ付加
extern int   bDKIM;
#else
extern BOOL   bDKIM;
#endif
extern CHAR   mDomainAUTHDKIM[];
#endif
#ifdef UPDATE_20150212 // 接続元のアドレスによって受信メールを破棄するオプション
extern BOOL   bDISCARDMAIL;    // 受信メール破棄オプション
extern char   mDISCARDFile[];
#endif
#ifdef UPDATE_20190108 // 承認メールをHTMLメールにする機能を追加
extern BOOL bMailApprovalHtml; // 承認メールをHTMLメールに
#endif
#ifdef UPDATE_20161112 // 送信先制限にエンベロープの送信元と日付ヘッダ中のトークンを指定可能にした。
extern BOOL bMailApprovalRevers;
extern BOOL bMailApprovalWebOnly; // Web管理のみでは承認依頼メールを作らない
extern char mRecivePermitKeyFile[];
extern char mRecivePermitBlackFile[];
extern char mRecivePermitWhiteFile[];
extern char mRecivePermitBlackWordFile[];
extern char mRecivePermitWhiteWordFile[];
#endif
#ifdef UPDATE_20170930 // 逆上長承認で承認者への送信元エンベロープが不正アドレス等で送信先に受信拒絶されないようにする対策
extern int  nMailApprovalReversType; // 送信エラーの返信先 0:送信しない 1:送信元 2:送信先指定
extern char mMailApprovalReversFrom[];
#endif
#ifdef UPDATE_20190329 // ML同報時の移動状態を詳細ログに追加
extern INT  nMovefileRetry; // MoveFileEx()での失敗時のリトライ上限 0=無制限
#endif
#ifdef UPDATE_20240205 // 承認依頼メールで１行当たりの文字数を正規化
extern int nNomalColumnMaxSize; //デフォルト 78byte/Line
#endif
#ifdef UPDATE_20240221 // 文字コード区切りでないデータへの対応
extern DWORD   nUTF8ToSJISTBL;
#endif
#ifdef UPDATE_20260610A // 1セッションで繰返し送信があるとacceptlogに結果の記録が抜ける
extern BOOL  bAcceptLog;
#endif
#ifdef LGWAN_ON        // LGWAN機能拡張
BOOL IP_COMP(char *pRange, char *pAddr);
void LGWANControl(PSmtpContext pContext);
BOOL CheckDomain(char *mID);
void ReplaceMSGHeader(PSmtpContext pContext, CHAR *pFrom, CHAR *pFromDest, CHAR *pTo, CHAR *pToDest);
#endif
void MailToBoss(CHAR *pHostName, CHAR *pMSGID, CHAR *pMSGSource, CHAR *pMSG, CHAR *pRCP, CHAR *pFrom, CHAR *pTos, CHAR *pB64Tos, CHAR *pSubject);
#ifdef UPDATE_20080225 // 部下に対する承認のため保留通知
void MailToStaff(CHAR *pHostName, CHAR *pMSGID, CHAR *pMSGSource, CHAR *pMSG, CHAR *pRCP, CHAR *pFrom, CHAR *pTos, CHAR *pB64Tos, CHAR *pSubject);
void MoveStaffMail(CHAR *pMSGID, CHAR *pRCP);
void DeleteStaffMail(CHAR *pMSGID, CHAR *pRCP);
#endif
#ifdef UPDATE_20080226 // 部下に対する承認のため保留中のリスト
void WriteWaitListsForStaff(char *pFrom, char *pFn, char *pmtB64, FILE *fp);
#endif
#ifdef UPDATE_20080229 // 承認保留一覧に各メールの題名を表示する
void GetInfo(_TCHAR *pTkn, _TCHAR *pFn, _TCHAR *pData, DWORD nMaxLen);
#endif
#ifdef UPDATE_20080711 // 承認したことを、他の上長にも通知
void AddToBossAddrress(CHAR *pFrom, CHAR *pBoss, CHAR *pRCPFn);
#endif

#ifdef UPDATE_20070713 // 承認者の代理人を設定するオプション
#ifdef UPDATE_20080220 // 承認者の代理人の代理人を設定する
#ifdef UPDATE_20080221 //BossCheck 承認者の代理人の代理人の深さの設定
void GetProxyUser(CHAR *pDir, CHAR *pBoss, CHAR *pAddr, DWORD nDepath); // 代理人ファイルから読出し
#else
void GetProxyUser(CHAR *pDir, CHAR *pBoss, CHAR *pAddr); // 代理人ファイルから読出し
#endif
#else
void GetProxyUser(CHAR *pDir, CHAR *pBoss); // 代理人ファイルから読出し
#endif
#ifdef UPDATE_20090701 // 代理承認処理メールの挿入文の書換えを可能に
void SetProxyUser(CHAR *pDir, CHAR *pAddr, DWORD nDepath, CHAR *pProxyAddr);
#else
#ifdef UPDATE_20080221 //BossCheck 承認者の代理人の代理人の深さの設定
void SetProxyUser(CHAR *pDir, CHAR *pAddr, DWORD nDepath);
#else
void SetProxyUser(CHAR *pDir, CHAR *pAddr);
#endif
#endif

#ifdef UPDATE_20140623A // 上長承認結果ログに送信元IPの記録を追加した。
void MailToApproval(CHAR *pBDir, CHAR *pIP, CHAR *pMSG, CHAR *pRCP, CHAR *pFrom, CHAR *pTo, CHAR *pCc, CHAR *pSub);
#else
#ifdef UPDATE_20080620 // TOアドレスが代理人になる機能追加
void MailToApproval(CHAR *pBDir, CHAR *pMSG, CHAR *pRCP, CHAR *pFrom, CHAR *pTo, CHAR *pCc, CHAR *pSub);
#else
void MailToApproval(CHAR *pBdir, CHAR *pMSG, CHAR *pRCP, CHAR *pFrom, CHAR *pSub);
#endif
#endif
#else
void MailToApproval(CHAR *pMSG, CHAR *pRCP, CHAR *pFrom, CHAR *pSub);
#endif
#ifdef UPDATE_20161112
void SetMailApprovalReversAddr(char *pAddr, char *pFn, BOOL bMode);
#endif
void MakeEML(char *pFrom, char *pAppMSG, char *pMSGSrc, char *pMSGDest, char *pMSGID, int nAapproval);
void WriteWaitLists(char *pFrom, char *pFn, FILE *fp);
#ifdef UPDATE_20140623A // 上長承認結果ログに送信元IPの記録を追加した。
void ApprovalLog(BOOL bAction, CHAR *pIP, CHAR *pFrom, CHAR *pMail);
#else
void ApprovalLog(BOOL bAction, CHAR *pFrom, CHAR *pMail);
#endif
#endif
#ifdef ACCEPT_LOG_LEVEL3
void LEVEL_3_ACCEPTLOG(PCLIENT_CONTEXT lpClientContext, char *mess);
#endif

#ifdef Y2038_BUG
void gettime(SYSTEMTIME *ltime, char *mt);
#else
void gettime(time_t *ltime, char *mt);
#endif
#ifdef UPADTE_20031120
BOOL ListsDeletesAttachment(char *mRcpt);
#endif
BOOL  ListsReplyCheck(char *mRcpt);
BOOL  ListsCheck(char *mFrom, char *mRcpt, char *mMlTkn, char *MlWord, DWORD *nMLNo, BOOL bCount);
void  CarbonCopy(PSmtpContext pContext);
//BOOL  Filter(PSmtpContext pContext);
BOOL  Filter(PSmtpContext pContext, char *pRCPTO);
BOOL  Check_Of_MIME(PSmtpContext pContext);
#ifdef UPDATE_20071204  // メッセージＩＤ採番処理を修正(Bym10id)
BOOL ListsSave(char *mRcpt, char *mSubject, DWORD nMLNo, char *mID, char *mFolder);
#else
#ifdef UPDATE_20210304 // メーリングリストで投稿内容保存ファイルの原本を保存するオプションを追加した。
BOOL ListsSave(char *mRcpt, char *mSubject, DWORD nMLNo, DWORD mID, char *mFolder, BOOL *pbListSaveOrign);
#else
BOOL ListsSave(char *mRcpt, char *mSubject, DWORD nMLNo, DWORD mID, char *mFolder);
#endif
#endif
#ifdef UPDATE_20050107
void  LineConv(char *dest, INT ndest, char *src, char *pack);
#else
void  LineConv(char *dest, char *src, char *pack);
#endif
void  translateUue2Base64(char *line, int inlen, char *newline);
void  SubjectConv(FILE *Tempfp, char *mMLWord, char *mMLtkn, char *mSubject, char *mDBSubject, char *mPack);
void  LineConv2(char *dest, INT ndest, char *src, char *pack);

#ifdef V4
BOOL GetMailInMaxSize(char *user, BOOL bLocal, BOOL bSubLocal);
#endif
#ifdef MILTER_ON // MILTERインターフェースを追加。
extern BOOL   bMILTER;         // MILTER機能拡張
BOOL MLT_DATA(PCLIENT_CONTEXT lpClientContext);
#endif
#ifdef UPDATE_20150212 // 接続元のアドレスによって受信メールを破棄するオプション
BOOL DiscardMail(CHAR *pPeerAddr, CHAR *pFrom);
#endif
#ifdef UPDATE_20160707 // 上長承認をUTF-8で処理する際、全角ハイフン「－」が内部処理で文字化けする
void UTF8_Rewrite_Value(CHAR *pSrc);
#endif
#ifdef ANNOUNCE
VOID Announce(PSmtpContext pContext);
#endif
#ifdef UPDATE_20240205 // 承認依頼メールで１行当たりの文字数を正規化
void LineNormalization(FILE *fp, int nMax, char *pLine);
#endif

#ifdef BTHREAD
BOOL DotDispatch(PCLIENT_CONTEXT lpClientContext)
#else
SMTPRSDisposition DotDispatch(
    PSmtpContext pContext,
    PUCHAR      InputBuffer,
    DWORD       InputBufferLen,
    PHANDLE     SendHandle,
    PUCHAR *    OutputBuffer,
    PDWORD      OutputBufferLen)
#endif
{
#ifdef BTHREAD
  PSmtpContext pContext = &lpClientContext->Context;
#endif
	CHAR       mMLWord[128], mListDir[256];
	CHAR       mSubject[512], mDBSubject[512], mPack[512];
#ifdef UPDATE_20210303 // メーリングリストに付加表題が無いとき題名を再構築できない不具合
    CHAR       mDBSubject2[512];
#endif
#ifdef UPDATE_20071205A //RFC2821: メールアドレス長が255Byteのときの対策
    CHAR       mMES[512], mRET[512], mRCPTO[512]; // 255だとオーバーフローする為変更
	CHAR       mFnRCP[256], mUser[260];
#else
    CHAR       mMES[200], mRET[200], mRCPTO[200]; // 255だとオーバーフローする為変更
	CHAR       mFnRCP[256], mUser[256];
#endif
	//CHAR       mMES[256], mRET[256], mRCPTO[256], mFnRCP[256], mUser[256];
	BOOL       bList, bSub, bReq, bNL, bML = FALSE, b1Byte;
	DWORD      n, nRCP, nMLNo, nErr;
	FILE       *fpRcp;
	char       *pdom, *pCrLf, *pWSP[2];
	int        nWSP;
#ifdef UPADTE_20031120
	BOOL       bDelAttch, bTEXT, bFirst;
	CHAR       *pBoundary, mBoundary[256];
#endif
#ifdef UPADTE_20040428
    BOOL       bHitaFilter;
#endif
#ifdef UPDATE_20050214 
    char       *pIDD;
#endif
#ifdef UPDATE_20050606 // RCPの再作成でロストしないように待つ
    FILE       *fp1;
#endif
	char       mLog[1024];
#ifdef UPDATE_20060306 // ヘッダ区切りのない本文への対応
    BOOL       bBODY = FALSE;
#endif
#ifdef UPDATE_20061102 // 受信メールのエンベロープと本文の任意のフォルダへのバックアップ機能の追加
    CHAR       mBackEML[256], mBackRCP[256], mBackEML2[256], mBackRCP2[256];
#endif	
#ifdef UPDATE_20070516 // 上長承認機能の追加
	FILE       *fp;
    BOOL       bDeleApprovalMSG = TRUE;
	BOOL       bSub2 = FALSE;
	BOOL       bAttach = FALSE; // 添付つきか否か
	CHAR       *p, *q, *r;
#ifdef UPDATE_20241111 // 題名がMIME-Qで記載されていると上長承認題名チェックの題名長さの範囲が短くなってしまう。
	CHAR       mSub[512];
#else
	CHAR       mSub[256];
#endif
#ifdef UPDATE_20150807 // BOSSCheckで生成したSubject:ヘッダが１行結合されて出力しないようにした。
	CHAR       mRawSub[1024];
#endif
	int        nb[2];
	CHAR       mData[256];
	CHAR       mBoss[2048], mApprEML[256], mApprRCP[256], mMSGID[256], mBossB64[1024];
    CHAR       *itmv[4], itm[4][1024 /*MAXMAILDATA*/];
#endif
#ifdef UPDATE_20080501 // uuencodeでの添付ファイル有無を認識する対策
	BOOL       bUueAttach = FALSE; // 添付つきか否か
#endif
#ifdef UPDATE_20070713 // 承認者の代理人を設定するオプション
    CHAR  Bdir[MAX_PATH];
#endif
#ifdef UPDATE_20080507 // 添付付きの判定方法の改善
	BOOL       bCT = FALSE;
    BOOL       bMIMEHead = FALSE;
	CHAR       *pBd, mBd[256];
#endif
#ifdef UPDATE_20230302 // 署名鍵入りメールの添付分離を行わないようにする
	BOOL	   bSIG = FALSE;
#endif
#ifdef UPDATE_20080516 // 複数承認者中に代理承認者設定があった場合の対策
	CHAR       mBoss1[256], mBoss2[1024];
#endif
#ifdef UPDATE_20080617 // 上長承認機能で添付ファイルの拡張子による承認条件追加
	BOOL       bHit = FALSE, bFName = FALSE;
	CHAR       *pm, mFName[512], mMIME[512] = "\x0";
#endif
#ifdef UPDATE_20080620 // TOアドレスが代理人になる機能追加
	CHAR mAPTo[512] = "\x0", mAPCc[512]  = "\x0";
#ifdef UPDATE_20080710 // TOアドレスのホワイトスペース除去
	CHAR *pAPTo, *pAPCc;
#endif
#endif
#ifdef UPDATE_20080629 // 承認者は２人までに制限(approvalフォルダに保管するファイル名が、ファイル名の長さの限界があるため)
    int  nBoss = 0;
#endif
#ifdef UPDATE_20090121 // 上長承認保留ファイル名称の生成ルール変更
	CHAR       mApprADR[256]; // 常駐アドレス保管ファイル
	CHAR       mft[32];
	SYSTEMTIME ltime;
	FILETIME ft;
#endif	
#ifdef UPDATE_20090910 // report-type="disposition-notification" なら添付無しと判定する
	BOOL bReport = FALSE;
	BOOL bCtype = FALSE;
#endif
#ifdef UPDATE_20100129 // メールヘッダの区切りにホワイトスペースが無いとヘッダを認識しない
    CHAR *pTerm;
#endif
#ifdef UPDATE_20151118 // パックされている言語の末尾のホワイトスペースを除去する
	CHAR *pWBSP[2];
#endif
#ifdef UPDATE_20160418 // 拡張ファイルにも併記可能にする。送信先に指定のアドレスまたはドメインが同報として含まれていないとチェック対象にする条件
	FILE *fpExt;
    BOOL bExtHit = FALSE;
	CHAR mBossExt[2048];
#endif
#ifdef UPDATE_20161112 //日付時差が一致していないとチェック対象にする条件
	CHAR mDate[512] = "";
	CHAR mHFrom[512] = "";
#endif
#ifdef UPDATE_20161112 // 送信先制限にエンベロープの送信元と日付ヘッダ中のトークンを指定可能にした。
    BOOL bHitaApprovalRev = FALSE;
#endif
#ifdef UPDATE_20170512 // ヘッダ一行の文字数が受信バッファを超えている場合、空欄行が含まれてしまう不具合
	int nTerm;
#endif
#ifdef UPDATE_20181026 // ヒットしたブラックリストファイルの文字列位置情報を追加表示する。
	DWORD nAPNo;
#endif
#ifdef UPDATE_20190329 // ML同報時の移動状態を詳細ログに追加
    INT  nMRty; // MoveFileEx()での失敗時のリトライ上限 0=無制限
#endif
#ifdef UPDATE_20200202 // 子プロセスへの引数が空白になるデータは""（ダブルクォーテーション）で括るようにした。
	CHAR mArgv[_MAX_PATH*2];
#endif
#ifdef UPDATE_20210304 // メーリングリストで投稿内容保存ファイルの原本を保存するオプションを追加した。
    BOOL bListSaveOrign = FALSE;
	CHAR mALTBoundary[256] = "";
#endif
#ifdef UPDATE_20210314 // MLで添付削除後に続く同報先がMLで添付削除指定があると、MIME構想が狂う
    BOOL bSkipDelAttach;
#endif
#ifdef UPDATE_20210317 // メーリングリストあてで添付削除指定の際、メールヘッダの構造が"multipart/alternative"のとき、2番目のMIME構造が添付と判定され削除されないようにした。
    BOOL bHeadAlt;
#endif
#ifdef UPDATE_20210322 // 添付にTEXTやHTMLファイルがあるとメッセージが複数挿入される不具合
    BOOL bInsert;
#endif
#ifdef UPDATE_20210403 // 上長承認が有効だと添付ファイル分離機能が無効状態にならないようにした。
#ifdef UPDATE_20210706 // 上長承認フォルダに作成した"$SG"ファイルが削除されないようにした。
	CHAR mApprtmpEML[256] = "", mApprtmpRCP[256] = "";
#else
	CHAR mApprtmpEML[256], mApprtmpRCP[256];
#endif
	BOOL bMailApprovalMove = FALSE; 
#endif
#ifdef UPDATE_20210427 // 本文のCharsetに合わせてリンク挿入できるようにした。
	CHAR *pcs;
	CHAR mcs[256] = "";
#endif
#ifdef UPDATE_20240112 // MIMEパートが添付のみだと分離されない
    unsigned long npos;
	BOOL bDTXT;
#endif
#ifdef UPDATE_20240113 // MIMEパート無しの添付メールだと分離されない
	BOOL bAttachOnly; // MIMEパート無しの添付
#endif
#ifdef UPDATE_20240116 // メール本体の文字エンコードがbase64の場合の添付分離時のメールに、Content-Transfer-Encoding:ヘッダが重複してしまう不具合。
	BOOL bHitCT;
	int  nCEnc;
	CHAR mCEnc[256] = "";
#endif

    // Verify context, and that the context says we can receive this command
#ifdef UPDATE_20210722_LOG
    //sprintf(mLog, "Start DotDispatch", );
    LEVEL_3_ACCEPTLOG(lpClientContext, "Start DotDispatch");
#endif

    if (!pContext ||
        (pContext->State != SmtpData) ) {
      sprintf(pContext->mMessages, SMTP_UNKNOW_MESS, ".");
      pContext->mToken[0] = '\x0';
#ifdef BTHREAD
      return TRUE;
#else
      *OutputBuffer = pContext->mMessages;
      *OutputBufferLen = strlen(pContext->mMessages);
      return(SMTPRS_SendBuffer);
#endif
    }

    // Put us into Authorization state
    pContext->State  = SmtpGreeting;
#ifdef UPDATE_20210722_LOG
    sprintf(mLog, "pContext->State=%d", pContext->State);
    LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
#endif

#ifdef UPDATE_20060615 // データのディスク書込に異常が有った場合の対策
	if (!pContext->bDiskStatus) { // デスクへの書込み異常があるなら
      sprintf(pContext->mMessages, SMTP_DEVICE_FULL);
      pContext->mToken[0] = '\x0';
#ifdef BTHREAD
      return TRUE;
#else
      *OutputBuffer = pContext->mMessages;
      *OutputBufferLen = strlen(pContext->mMessages);
      return(SMTPRS_SendBuffer);
#endif
	}
#endif

#ifdef UPDATE_20210722_LOG
    sprintf(mLog, "pContext->bDiskStatus=%d", pContext->bDiskStatus);
    LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
#endif

#ifdef MILTER_ON // MILTERインターフェースを追加。
	if (bMILTER)
	{
      if (!MLT_DATA(lpClientContext))
	  {
        pContext->mToken[0] = '\x0';
#ifdef BTHREAD
        return TRUE;
#else
        *OutputBuffer = pContext->mMessages;
        *OutputBufferLen = strlen(pContext->mMessages);
        return(SMTPRS_SendBuffer);
#endif
	  }
	}
#endif

    // No file, but copy the banner string and return it.
    //*SendHandle = NULL;
    pContext->mToken[0] = '\x0';
	mSubject[0] = '\x0';
	bSub = FALSE;
#ifdef UPDATE_20070516 // 上長承認機能の追加
    bDeleApprovalMSG = TRUE;
	bSub2 = FALSE;
	mSub[0] = '\x0';
#endif	
#ifdef UPDATE_20150807 // BOSSCheckで生成したSubject:ヘッダが１行結合されて出力しないようにした。
	mRawSub[0] = '\x0';
#endif
#ifdef UPDATE_20080507 // 添付付きの判定方法の改善
    bCT = FALSE;
    bMIMEHead = FALSE;
	mBd[0] = '\x0';
	pBd = NULL;
#endif
#ifdef UPDATE_20230302 // 署名鍵入りメールの添付分離を行わないようにする
	bSIG = FALSE;
#endif
#ifdef UPDATE_20210314 // MLで添付削除後に続く同報先がMLで添付削除指定があると、MIME構想が狂う
    bSkipDelAttach = FALSE;
#endif
#ifdef UPDATE_20210317 // メーリングリストあてで添付削除指定の際、メールヘッダの構造が"multipart/alternative"のとき、2番目のMIME構造が添付と判定され削除されないようにした。
    bHeadAlt = FALSE;
#endif
#ifdef UPDATE_20210322 // 添付にTEXTやHTMLファイルがあるとメッセージが複数挿入される不具合
    bInsert = FALSE;
#endif

///// 1000通１分２０秒
//#ifdef SPEED_TEST
//pContext->bAcptData = TRUE;
//sprintf(pContext->mMessages, SMTP_GOOD_MESS, "Message received");
//return TRUE;
//#endif
////////////////
#ifdef UPDATE_20080531 // LGWAN使用時の振分け機能の強化
#ifdef LGWAN_ON        // LGWAN機能拡張
    LGWANControl(pContext);
#endif
#endif
#ifdef UPDATE_20210722_LOG
    sprintf(mLog, "bDKIM=%d", bDKIM);
    LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
#endif

#ifdef DOMAIN_AUTH_DKIM    // 2013.01.17 ドメイン認証 DomainkeysとDKIM方式 追加
	if (bDKIM)
	{
#ifdef UPDATE_20240122 // SPF/DKIM/DMARC/ARC実行パスを半角スペースを含むロングネームでも指定可能にする対策
	  char mCmdl[256];

	  sprintf(mCmdl, "\"%s\"", mDomainAUTHDKIM);
	  sprintf(mArgv, "\"%s\"", pContext->mUIDFROM);
      _spawnl(_P_WAIT, mDomainAUTHDKIM, mCmdl, pContext->PeerAddr, mArgv, pContext->mFnData, pContext->LocalName, NULL);
#else
#ifdef UPDATE_20200202 // 子プロセスへの引数が空白になるデータは""（ダブルクォーテーション）で括るようにした。
	  sprintf(mArgv, "\"%s\"", pContext->mUIDFROM);
      _spawnl(_P_WAIT, mDomainAUTHDKIM, mDomainAUTHDKIM, pContext->PeerAddr, mArgv, pContext->mFnData, pContext->LocalName, NULL);
#else
      _spawnl(_P_WAIT, mDomainAUTHDKIM, mDomainAUTHDKIM, pContext->PeerAddr, pContext->mUIDFROM, pContext->mFnData, pContext->LocalName, NULL);
#endif
#endif
	}
#endif
	///// Get Messgae-ID info //////
#ifdef UPDATE_20050214
    pContext->mMessIDD[0] = '\x0';
	pContext->bHead = FALSE;
    pContext->Datafp = fopen(pContext->mFnData, "rb");
#ifdef UPDATE_20210722_LOG
    sprintf(mLog, "fopen(%s, rb)=%08x", pContext->mFnData, pContext->Datafp);
    LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
#endif

	if (pContext->Datafp) {
#ifdef DATA_CASH
	  setvbuf(pContext->Datafp, pContext->mFrbuf, _IOFBF, sizeof(pContext->mFrbuf) );
#endif
#ifdef UPDATE_20040720_LOG
      sprintf(mLog, " Get Messgae-ID: fopen(%s, wb)", pContext->mFnData);
      LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
#endif
       //////////////////////////////////////////////
      fgets(pContext->mdata, sizeof(pContext->mdata), pContext->Datafp);
      while(!feof(pContext->Datafp)){ // ヘッダのみ確認
#ifdef UPDATE_20090910 // report-type="disposition-notification" なら添付無しと判定する
		if (bMailApprovalNotification)
		{
		  if (!pContext->bHead &&
		      (bCtype || !_strnicmp(pContext->mdata, "content-type:", 13)))
		  {
		    bCtype = TRUE;
		    if (bCtype)
			{ 
		      if (!_strnicmp(pContext->mdata, "content-type:", 13) ||
			  	  pContext->mdata[0] == ' ' ||
				  pContext->mdata[0] == '\t')
			  {
			    CHAR mLine[1024];
			    strcpy(mLine, pContext->mdata);
			    _strlwr(mLine);
		        if (strstr(mLine, "report-type=") &&
				    strstr(mLine, "disposition-notification") )
				{
		          bReport = TRUE;
				} 
			  } else {
		        bCtype = FALSE;
			  }
			}
		  }
		}
#endif
#ifdef UPDATE_20060306 // ヘッダ区切りのない本文への対応
	    if ( !pContext->bHead &&
			  ( !(strpbrk(pContext->mdata, ":") ||
			      pContext->mdata[0] == '\t' ||
			      pContext->mdata[0] == ' ')    ||
			   !strcmp(pContext->mdata, "\r\n") ||
			   !strcmp(pContext->mdata, "\n")   ||
		       !strcmp(pContext->mdata,".\r\n") ||
			   !strcmp(pContext->mdata,".\n"))  )
#else
	    if ( !pContext->bHead &&
			  (!strcmp(pContext->mdata, "\r\n") ||
			   !strcmp(pContext->mdata, "\n")   ||
		       !strcmp(pContext->mdata,".\r\n") ||
			   !strcmp(pContext->mdata,".\n"))  )
#endif
		{

#ifdef UPDATE_20080501 // uuencodeでの添付ファイル有無を認識する対策
		  // 本文内
#ifdef UPDATE_20080617 // 上長承認機能で添付ファイルの拡張子による承認条件追加
		  if (bHit) // 添付ありならチェック不要
			break;
#else
		  if (bAttach) // 添付ありならチェック不要
			break;
#endif
		  if (bUueAttach && !_strnicmp(pContext->mdata, "end", 3) && 
			  (pContext->mdata[3] == '\r' || pContext->mdata[3] == '\n')) {
#ifdef UPDATE_20080617 // 上長承認機能で添付ファイルの拡張子による承認条件追加
			if (pContext->mMIME[0]) { // MIME定義ファイルがあるとき
			  if (!bHit)
			  {
                bAttach = QueryMIMEType(pContext->mMIME, mMIME, &bHit); // 指定ファイルに拡張子が一致する場合のみ上長承認処理
			  }
			} else {
	 	      bAttach = TRUE; // 添付付きとして強制判定
			}
#else
	 	    bAttach = TRUE; // 添付付きとして強制判定
#endif
			break;
		  }
		  if (!_strnicmp(pContext->mdata, "begin ", 6)) {
			if (strchr((char *)&pContext->mdata[6], ' ')) {
	          bUueAttach = TRUE; // 添付つきの可能性
#ifdef UPDATE_20080617 // 上長承認機能で添付ファイルの拡張子による承認条件追加
			  if (pContext->mMIME[0]) { // MIME定義ファイルがあるとき
			    mMIME[0] = '\x0';
	            if ((pm = strrchr((char *)&pContext->mdata[6], '.')))
				{
				  strncpy(mMIME, pm, 5);
				  mMIME[5] = '\x0';
				  strtok(mMIME, "\r\n");
				}
			  }
#endif
			}
		  }
#ifdef UPDATE_20080507 // 添付付きの判定方法の改善
		  strtok(pContext->mdata, "\r\n");
		  if (!strcmp(pContext->mdata,mBd))  // ヘッダとして判定し直し
		  {
			pContext->bHead = FALSE;
			pBd = NULL;
		  }
#endif
#else
          //pContext->bHead = TRUE;
	      break;
#endif
		} 
		if (!pContext->bHead) {
	      strncpy(pContext->mtkn, pContext->mdata, 12);
		  pContext->mtkn[12] = '\x0';
		  strtok(pContext->mtkn," \r\n");
		  if (!_stricmp(pContext->mtkn, "message-id:")) {
		    pContext->mMessIDD[0] = '\x0';
		    if ((pIDD = strrchr(pContext->mdata, '@')))
		      strcpy(pContext->mMessIDD, ++pIDD);
		    strtok(pContext->mMessIDD, ">\r\n");
		  }
#ifdef UPDATE_20070516 // 上長承認機能の追加
		  strtok(pContext->mdata, "\r\n");
#ifdef UPDATE_20161112 //日付時差が一致していないとチェック対象にする条件
	      if (!mDate[0] && !_strnicmp(pContext->mdata, "date:", 5))
		  {
		     p = &pContext->mdata[5];
		     while (*p == '\t' || *p == ' ')
		       p++;
			 strcpy(mDate, p);
		  } else 
	      if (!mDate[0] && !_strnicmp(pContext->mdata, "from:", 5))
		  {
		     p = &pContext->mdata[5];
		     while (*p == '\t' || *p == ' ')
		       p++;
			 strcpy(mHFrom, p);
		  } else 
#endif

#ifdef UPDATE_20080517 // 上長宛承認メールの承認否認メールに対象メールの題名情報などを記録する
		  if (!mSub[0] && !_strnicmp(pContext->mdata, "subject:", 8))
#else
          if (!_strnicmp(pContext->mdata, "subject:", 8))
#endif
          {
		     bSub2 = TRUE;
		     p = &pContext->mdata[8];
		     while (*p == '\t' || *p == ' ')
		       p++;
#ifdef UPDATE_20150807 // BOSSCheckで生成したSubject:ヘッダが１行結合されて出力しないようにした。
			  if (sizeof(mRawSub)-strlen(mRawSub) > strlen(p)+2) 
			  {
#ifdef UPDATE_20170112 // データ行に改行だけの場合も除去するようにする
				if (*p)
#endif
				{
                  strcpy(mRawSub, p);
			      strcat(mRawSub,"\n");
				}
			  }
#endif
#ifdef UPDATE_20241111 // 題名がMIME-Qで記載されていると上長承認題名チェックの題名長さの範囲が短くなってしまう。
			 if (sizeof(mSub) > strlen(p))
#else
		     if (256 > strlen(p))
#endif
			 {
		       strcpy(mSub, p);
			 }
		  } else if (bSub2) {
			if (pContext->mdata[0] == '\t' || pContext->mdata[0] == ' ') {
		      p = pContext->mdata;
#ifdef UPDATE_20150807 // BOSSCheckで生成したSubject:ヘッダが１行結合されて出力しないようにした。
			  if (sizeof(mRawSub)-strlen(mRawSub) > strlen(p)+2) 
			  {
	            strcat(mRawSub, p);
			    strcat(mRawSub,"\n");
			  }
#endif
		      while (*p == '\t' || *p == ' ')
		        p++;
#ifdef UPDATE_20241111 // 題名がMIME-Qで記載されていると上長承認題名チェックの題名長さの範囲が短くなってしまう。
			  if (sizeof(mSub)-strlen(mSub) > strlen(p)) 
#else
		      if (256-strlen(mSub) > strlen(p)) 
#endif
			  {
		        strcat(mSub, p);
			  }
			} else {
			  bSub2 = FALSE;
			}
		  }
#ifndef UPDATE_20080501 // 上長承認機能でSubjectヘッダの次にContent-Typeヘッダが続くと添付の判定に失敗する
		  else 
		  if (!_strnicmp(pContext->mdata, "content-type:", 13))
#else
#ifdef UPDATE_20080507 // 添付付きの判定方法の改善
		  if (!bSub2 && (bCT || !_strnicmp(pContext->mdata, "content-type:", 13)))
#else
		  if (!bSub2 && !_strnicmp(pContext->mdata, "content-type:", 13))
#endif
#endif
		  {
#ifdef UPDATE_20080507 // 添付付きの判定方法の改善
			strncpy(pContext->mtkn2, pContext->mdata, 128);
			pContext->mtkn2[128] = '\x0';
			strlwr(pContext->mtkn2);
		    strtok(pContext->mtkn2,"\r\n");
		    strlwr(pContext->mtkn2);
			if (bCT && (pContext->mtkn2[0] != '\t' && pContext->mtkn2[0] != ' ')) {
			  bCT = FALSE;
			}

#ifdef UPDATE_20080617 // 上長承認機能で添付ファイルの拡張子による承認条件追加
			if (!_strnicmp(pContext->mtkn2, "content-type:", 13)) {
			  bFName = FALSE;
#ifdef UPDATE_20230302 // 署名鍵入りメールの添付分離を行わないようにする
			  if (strstr(pContext->mtkn2, "multipart/signed"))
			  {
				bSIG = TRUE;
			  }
#endif
			  if (pContext->mMIME[0]) { // MIME定義ファイルがあるとき
			    bCT= TRUE;
				/*
			    mMIME[0] = '\x0';
				pm = &pContext->mtkn2[13];
				while(*pm == ' ' || *pm == '\t ')
				{
				  pm++;
				}
				strcpy(mMIME, pm);
				strtok(mMIME, ";\r\n");
				if (!bHit)
				{
                  bAttach = QueryMIMEType(pContext->mMIME, mMIME, &bHit); // 指定ファイルに拡張子が一致する場合のみ上長承認処理
				}
				*/
			  } else { // MIME定義ファイルが無いとき
                if (strstr(pContext->mtkn2, "multipart/alternative"))
				{
			      bCT = TRUE;
				} else if (!pBd && !bCT && !strstr(pContext->mtkn2, "text/"))
				{
		          bAttach = TRUE; // 添付付き
				}
			  }
			}
#else
#ifdef UPDATE_20080616 // 上長承認メールで添付ファイルとして拡張子がHTMLのファイルを添付された場合にスルーしてしまう対策
            if (strstr(pContext->mtkn2, "multipart/alternative"))
#else
            if (strstr(pContext->mtkn2, "multipart"))
#endif
			{
			  bCT = TRUE;
			} else if (!pBd && !bCT && !strstr(pContext->mtkn2, "text/")) 
            {
		      bAttach = TRUE; // 添付付き
			}
#endif
            if (bCT) {
			  if ((pBd = strstr(pContext->mtkn2, "boundary")))
			  {
			    pBd += 8;
			    strncpy(pContext->mtkn2, pContext->mdata, 128);
			    pContext->mtkn2[128] = '\x0';
#ifdef UPDATE_20080513 // boundaryのデータに"ダブルクォーテーションが無いとハングする
				if (!strchr(pBd, '"'))  // 区切りがないなら
				{
				  pBd++;
				} else
#endif
				{
			      while(*pBd!='"') // トークン開始カラムを探す
			        pBd++;
			      pBd++;          // 見つけた次のカラムからトークンを取り出す。
				}
			    sprintf(mBd, "--%s", pBd);
			    strtok(mBd, "\"\r\n");
			  }
#ifdef UPDATE_20080617 // 上長承認機能で添付ファイルの拡張子による承認条件追加
			  mMIME[0] = '\x0';
			  if (bFName || (pm= strstr(pContext->mtkn2, "name=")))
			  {
				if (bFName)
				{
				  pm = pContext->mtkn2;
				} else {
			      bFName = TRUE;
				  pm+=5;
				  if (*pm == '"')
				  {
					pm++;
				  }
				}
			    strncpy(pContext->mtkn2, pContext->mdata, 128);
			    pContext->mtkn2[128] = '\x0';
				strtok(pm, ";\"\r\n");
				mFName[0] = '\x0';
		        LineConv2(mFName, sizeof(mFName), pm, mPack);
	            if ((pm = strrchr(mFName, '.')))
				{
				  strncpy(mMIME, pm, 5);
				  mMIME[5] = '\x0';
				}
				if (!bHit)
				{
                  bAttach = QueryMIMEType(pContext->mMIME, mMIME, &bHit); // 指定ファイルに拡張子が一致する場合のみ上長承認処理
				}
			  }
			  
#endif
			}
#else
		    strlwr(pContext->mdata);
			if (strstr(pContext->mdata, "multipart")) {
#ifdef UPDATE_20080617 // 上長承認機能で添付ファイルの拡張子による承認条件追加
			  if (!bHit)
			  {
                bAttach = QueryMIMEType(pContext->mMIME, mMIME, &bHit); // 指定ファイルに拡張子が一致する場合のみ上長承認処理
			  }
#else
			  bAttach = TRUE; // 添付付き
#endif
			}
#ifdef UPDATE_20080218 // 分割メールでの添付判定
		    else if (strstr(pContext->mdata, "message/partial"))
			{
#ifdef UPDATE_20080617 // 上長承認機能で添付ファイルの拡張子による承認条件追加
			  if (!bHit)
			  {
                bAttach = QueryMIMEType(pContext->mMIME, mMIME, &bHit); // 指定ファイルに拡張子が一致する場合のみ上長承認処理
			  }
#else
			  bAttach = TRUE; // 添付付きとして強制判定
#endif
		      //bSepMail = TRUE;
			}
#endif
#endif
		  }
#endif
#ifndef UPDATE_20080812 // 送信元と送信先情報の取得
#ifdef UPDATE_20080620 // TOアドレスが代理人になる機能追加
          if (!pContext->bHead) {
		    if (mAPTo[0] == '\x0' && !_strnicmp(pContext->mdata, "to:", 3))
			{
#ifdef UPDATE_20080710 // TOアドレスのホワイトスペース除去
	          pAPTo = &pContext->mdata[3];
	          while(*pAPTo == ' ' || *pAPTo == '\t')
			  {
	            *pAPTo++;
			  }
		 	  strcpy(mAPTo, pAPTo);
#else
		 	  strcpy(mAPTo, &pContext->mdata[3]);
#endif
			}
		    if (mAPCc[0] == '\x0' && !_strnicmp(pContext->mdata, "cc:", 3))
			{
#ifdef UPDATE_20080710 // TOアドレスのホワイトスペース除去
	          pAPCc = &pContext->mdata[3];
	          while(*pAPCc == ' ' || *pAPCc == '\t')
			  {
	            *pAPCc++;
			  }
			  strcpy(mAPCc, pAPCc);
#else
			  strcpy(mAPCc, &pContext->mdata[3]);
#endif
			}
		  }
#endif
#endif
		}
        fgets(pContext->mdata, sizeof(pContext->mdata), pContext->Datafp);
	  }
	  fclose(pContext->Datafp);
	}
#endif

#ifdef UPDATE_20080812 // 送信元と送信先情報の取得
#ifdef UPDATE_20080620 // TOアドレスが代理人になる機能追加
    if ((pContext->Datafp = fopen(pContext->mFnHead, "rt"))) 
    {
	 mAPTo[0] = mAPCc[0] = '\x0';
     fgets(pContext->mdata, sizeof(pContext->mdata), pContext->Datafp);
     fgets(pContext->mdata, sizeof(pContext->mdata), pContext->Datafp);
	 pContext->mdata[0] = '\x0';
     fgets(pContext->mdata, sizeof(pContext->mdata), pContext->Datafp);
	 strtok(pContext->mdata, "\r\n");
	 if (strlen(pContext->mdata) > 11)
	 {
	   strcpy(mAPTo, &pContext->mdata[11]);
	 }
	 pContext->mdata[0] = '\x0';
     fgets(pContext->mdata, sizeof(pContext->mdata), pContext->Datafp);
	 strtok(pContext->mdata, "\r\n");
	 if (strlen(pContext->mdata) > 11)
	 {
	   strcpy(mAPCc, &pContext->mdata[11]);
	 }
     fclose(pContext->Datafp);
   }
#endif
#endif

	//////////////////////////////////
#ifdef UPDATE_20080317 // メッセージＩＤ用変数が初期化されない不具合
#ifdef UPDATE_20071204  // メッセージＩＤ採番処理を修正(Bym10id)
    strcpy(mMSGID, pContext->mMsgId);
#else
    sprintf(mMSGID, "B%010lu", pContext->nMsgId);
#endif
#endif
#ifdef UPDATE_20061102 // 受信メールのエンベロープと本文の任意のフォルダへのバックアップ機能の追加
    if (bMailBackup) {
#ifdef UPDATE_20071204  // メッセージＩＤ採番処理を修正(Bym10id)
#ifdef UPDATE_20090227 // データ削除の完了確認対策
	  sprintf(mBackEML, "%s%s.MSB", mTempDir, pContext->mMsgId);
      while(!CopyFile(pContext->mFnData, mBackEML, TRUE)) 
	  {
        nErr = GetLastError();
	    if (nErr == ERROR_FILE_NOT_FOUND)
	      break;
        sprintf(mLog, "CopyFile(%s, %s) Fail. code=(%d) retry copy.", pContext->mFnData, mBackEML, nErr);
        LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
        _sleep(100);
	  }
	  sprintf(mBackRCP, "%s%s.RCB", mTempDir, pContext->mMsgId);
      while(!CopyFile(pContext->mFnHead, mBackRCP, TRUE);) 
	  {
        nErr = GetLastError();
	    if (nErr == ERROR_FILE_NOT_FOUND)
	      break;
        sprintf(mLog, "CopyFile(%s, %s) Fail. code=(%d) retry copy.", pContext->mFnHead, mBackRCP, nErr);
        LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
        _sleep(100);
	  }
#else
	  sprintf(mBackEML, "%s%s.MSB", mTempDir, pContext->mMsgId);
      CopyFile(pContext->mFnData, mBackEML, TRUE);
	  sprintf(mBackRCP, "%s%s.RCB", mTempDir, pContext->mMsgId);
      CopyFile(pContext->mFnHead, mBackRCP, TRUE);
#endif
#else
#ifdef UPDATE_20090227 // データコピーの完了確認対策
	  sprintf(mBackEML, "%sB%010lu.MSB", mTempDir, pContext->nMsgId);
      while(!CopyFile(pContext->mFnData, mBackEML, TRUE)) 
	  {
        nErr = GetLastError();
	    if (nErr == ERROR_FILE_NOT_FOUND)
	      break;
        sprintf(mLog, "CopyFile(%s, %s) Fail. code=(%d) retry copy.", pContext->mFnData, mBackEML, nErr);
        LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
        _sleep(100);
	  }
	  sprintf(mBackRCP, "%sB%010lu.RCB", mTempDir, pContext->nMsgId);
      while(!CopyFile(pContext->mFnHead, mBackRCP, TRUE)) 
	  {
        nErr = GetLastError();
	    if (nErr == ERROR_FILE_NOT_FOUND)
	      break;
        sprintf(mLog, "CopyFile(%s, %s) Fail. code=(%d) retry copy.", pContext->mFnHead, mBackRCP, nErr);
        LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
        _sleep(100);
	  }
#else
	  sprintf(mBackEML, "%sB%010lu.MSB", mTempDir, pContext->nMsgId);
      CopyFile(pContext->mFnData, mBackEML, TRUE);
	  sprintf(mBackRCP, "%sB%010lu.RCB", mTempDir, pContext->nMsgId);
      CopyFile(pContext->mFnHead, mBackRCP, TRUE);
#endif
#endif
#ifdef UPDATE_20090227 // データ削除の完了確認対策
      while(!(fp = fopen(mBackEML, "rt")))
	  {
        if (bServiceTerminating)
	      break;
        nErr = GetLastError();
        sprintf(mLog, "fopen(%s, \"rt\") Fail. code=(%d) retry.", mBackEML, nErr);
        LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
	    _sleep(WAIT_TIME);
	  }
	  fclose(fp);
      while(!(fp = fopen(mBackRCP, "rt"))) 
	  {
         if (bServiceTerminating)
	       break;
         nErr = GetLastError();
         sprintf(mLog, "fopen(%s, \"rt\") Fail. code=(%d) retry.", mBackEML, nErr);
         LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
	     _sleep(WAIT_TIME);
	  }
	  fclose(fp);
#endif
	}
#endif
#ifdef UPDATE_20070516 // 上長承認機能の追加
    bDeleApprovalMSG = FALSE;
#ifdef UPDATE_20161112 //題名ヘッダでのブラック＆ホワイトリストテーブル
    if (bMailApproval || bMailApprovalRevers)
#else
	if (bMailApproval) 
#endif
	{
#ifdef UPDATE_20070607 // 上長承認機能にSubject:キーワードも可能にする
	  LineConv(itm[2], sizeof(itm[2]), mSub, mPack);
      if (pContext->mBOSSSubject[0]) {
        //////コード変換 (EUC, JIS) -> S-JIS /////////////
        strcpy(itm[0], "");           itmv[0] = itm[0];
        strcpy(itm[1], "-s");         itmv[1] = itm[1];
        /*strcpy(itm[2], mSubject);*/ itmv[2] = itm[2];
        QuickConvertBuff(3, (char **)&itmv, (char *)itm[2]);
#ifdef UPDATE_20161112 //日付時差が一致していないとチェック対象にする条件
		//エンベロープの送信元でチェック対象にする条件
        if ((!strnicmp(pContext->mBOSSSubject, "envf0=", 6) ||
		     !strnicmp(pContext->mBOSSSubject, "envf1=", 6)) &&
		    strlen(pContext->mBOSSSubject) >= 6) {
		  if (!strnicmp(pContext->mBOSSSubject, "envf0=", 6))  // 一致する日付時差が含まれないとき承認依頼
		  {
		    bSub2 =  (wildcard_stricmp(&pContext->mBOSSSubject[6], pContext->mUIDFROM) ? FALSE : TRUE);
		  } else {
			bSub2 =  (wildcard_stricmp(&pContext->mBOSSSubject[6], pContext->mUIDFROM) ? TRUE : FALSE);
		  }
		}
		else 
		//Subject:ヘッダでチェック対象にする条件
        if ((!strnicmp(pContext->mBOSSSubject, "sub0=", 5) ||
		     !strnicmp(pContext->mBOSSSubject, "sub1=", 5)) &&
		    strlen(pContext->mBOSSSubject) >= 5) {
		  if (!strnicmp(pContext->mBOSSSubject, "sub0=", 5))  // 一致する日付時差が含まれないとき承認依頼
		  {
		    bSub2 =  (wildcard_stricmp(&pContext->mBOSSSubject[5], itm[2]) ? FALSE : TRUE);
		  } else {
			bSub2 =  (wildcard_stricmp(&pContext->mBOSSSubject[5], itm[2]) ? TRUE : FALSE);
		  }
		}
		else 
		//From:ヘッダでチェック対象にする条件
        if ((!strnicmp(pContext->mBOSSSubject, "from0=", 6) ||
		     !strnicmp(pContext->mBOSSSubject, "from1=", 6)) &&
		    strlen(pContext->mBOSSSubject) >= 6) {
		  if (!strnicmp(pContext->mBOSSSubject, "from0=", 6))  // 一致する日付時差が含まれないとき承認依頼
		  {
		    bSub2 =  (wildcard_stricmp(&pContext->mBOSSSubject[6], mHFrom) ? FALSE : TRUE);
		  } else {
			bSub2 =  (wildcard_stricmp(&pContext->mBOSSSubject[6], mHFrom) ? TRUE : FALSE);
		  }
		}
		else 
		//Date:ヘッダの文字列でチェック対象にする条件
        if ((!strnicmp(pContext->mBOSSSubject, "date0=", 6) ||
		     !strnicmp(pContext->mBOSSSubject, "date1=", 6)) &&
		    strlen(pContext->mBOSSSubject) >= 6) {
		  if (!strnicmp(pContext->mBOSSSubject, "date0=", 6))  // 一致する日付時差が含まれないとき承認依頼
		  {
		    bSub2 =  (wildcard_stricmp(&pContext->mBOSSSubject[6], mDate) ? FALSE: TRUE);
		  } else {
			bSub2 =  (wildcard_stricmp(&pContext->mBOSSSubject[6], mDate) ? TRUE : FALSE);
		  }
		}
		else 
#endif
#ifdef UPDATE_20160415 //送信先に指定のアドレスまたはドメインが同報として含まれていないとチェック対象にする条件
		if ((!strnicmp(pContext->mBOSSSubject, "to0=", 4) || 
			 !strnicmp(pContext->mBOSSSubject, "to1=", 4) ||
			 !strnicmp(pContext->mBOSSSubject, "cc0=", 4) || 
			 !strnicmp(pContext->mBOSSSubject, "cc1=", 4)) &&
			strlen(pContext->mBOSSSubject) >= 4) {
		  if (!strnicmp(pContext->mBOSSSubject, "to0=", 4) || // 一致するアドレスが含まれないとき承認依頼
			  !strnicmp(pContext->mBOSSSubject, "cc0=", 4) )  // 一致するCCアドレスが含まれないとき承認依頼
		  {
		    bSub2 =  TRUE;
		  } else {
		    bSub2 =  FALSE;
		  }
		  if ((fp = fopen(pContext->mFnHead, "rt"))) { // エンベロープの送信先をチェック
			if (!strnicmp(pContext->mBOSSSubject, "cc0=", 4) ||
				!strnicmp(pContext->mBOSSSubject, "cc1=", 4))
			{
	          while((p = fgets(mBoss, sizeof(mBoss), fp))) 
			  {
			    if (!strnicmp(mBoss, "recipient:", 10))
				{
				  break;
				}
			  }
			}
	        while((p = fgets(mBoss, sizeof(mBoss), fp))) 
			{
			  strtok(mBoss, "\r\n");
			  if (!strnicmp(mBoss, "recipient:", 10))
			  {
				if (!strnicmp(pContext->mBOSSSubject, "to0=", 4) || // 一致するアドレスが含まれないとき承認依頼
					!strnicmp(pContext->mBOSSSubject, "cc0=", 4) )  // 一致するCCアドレスが含まれないとき承認依頼
				{
                  if (wildcard_stricmp(&pContext->mBOSSSubject[4], &mBoss[11])) // 一致した
				  {
                    bSub2 =  FALSE;
				    break;
				  }
				} else { // 一致するアドレスが含まれるとき承認依頼
				  if (wildcard_stricmp(&pContext->mBOSSSubject[4], &mBoss[11])) // 一致した
				  {
                    bSub2 =  TRUE;
				    break;
				  }
				}
			  }
			}
			fclose(fp);
		  }
		} 
		else 
#endif
		if (!strnicmp(pContext->mBOSSSubject, "file=", 5) && strlen(pContext->mBOSSSubject) >= 7) {
		  bSub2 =  FALSE;
	      if ((fp = fopen(&pContext->mBOSSSubject[5], "rt"))) {
	        while((p = fgets(mBoss, sizeof(mBoss), fp))) {
			  strtok(mBoss, "\r\n");
#ifdef UPDATE_20161112 //題名ヘッダでのブラック＆ホワイトリストテーブル
              if (bMailApprovalRevers &&
				  (!strnicmp(mBoss, "black=", 6) ||
                   !strnicmp(mBoss, "white=", 6)) &&
				  strlen(mBoss) >= 6) {
				FILE *fpsub;
			    CHAR mSubFn[256] = "";
				CHAR mToken[512] = "";
                
                if ((r = strpbrk(&mBoss[8], ",:"))) {
				  *r = '\x0';
				}
				sprintf(mSubFn, &mBoss[6]);
				if ((fpsub = fopen(mSubFn, "rt")))
				{
#ifdef UPDATE_20181026 // ヒットしたブラックリストファイルの文字列位置情報を追加表示する。
	              nAPNo = 0;
#endif
				  while(fgets(mToken, sizeof(mToken)-1, fpsub))
				  {
#ifdef UPDATE_20181026 // ヒットしたブラックリストファイルの文字列位置情報を追加表示する。
	                nAPNo++;
#endif
					strtok(mToken, "\r\n");
				    if (!strnicmp(mBoss, "black=", 6))
					{
 		              bSub2 =  (wildcard_stricmp(mToken, itm[2]) ? TRUE: FALSE);
					  if (bSub2) {
#ifdef UPDATE_20181026 // ヒットしたブラックリストファイルの文字列位置情報を追加表示する。
						sprintf(pContext->mMessages, SMTP_BAD_APPROVAL, nAPNo);
#else
						sprintf(pContext->mMessages, SMTP_BAD_APPROVAL);
#endif
                        bHitaApprovalRev = TRUE;
#ifdef UPDATE_20260610 // 本文情報がブラックリストにヒットした場合はinlogへの記録はしない
                        bHitaFilter = TRUE;
#endif
						pContext->mBOSS[0]  = '\x0'; // 承認不要
					  }
					} else {
					  bSub2 =  (wildcard_stricmp(mToken, itm[2]) ? TRUE : FALSE);
					  if (bSub2) {
                        pContext->mBOSS[0] = '\x0'; // 承認不要
					  }
					}
				    if (bSub2)
				      break;
				  }
				  fclose(fpsub);
				}
				if (bSub2)
				  break;
			  } else 
			  {
#ifdef UPDATE_20070616 // キーワードで、承認先上長を変更
                if ((r = strpbrk(mBoss, ",:"))) {
				  *r = '\x0';
				}
#endif
			  }
			  //エンベロープの送信元でチェック対象にする条件
              if ((!strnicmp(mBoss, "envf0=", 6) ||
                   !strnicmp(mBoss, "envf1=", 6)) &&
                    strlen(mBoss) >= 6) {
		        if (!strnicmp(mBoss, "envf0=", 6))  // 一致する日付時差が含まれないとき承認依頼
				{
		          bSub2 =  (wildcard_stricmp(&mBoss[6], pContext->mUIDFROM) ? FALSE: TRUE);
				} else {
			      bSub2 =  (wildcard_stricmp(&mBoss[6], pContext->mUIDFROM) ? TRUE : FALSE);
				}
				if (bSub2)
				{
				  if (r) {
			        strcpy(pContext->mBOSS, r+1);
				  }
				  break;
				}
			  }
		      else
			  //Subject:ヘッダでチェック対象にする条件
              if ((!strnicmp(mBoss, "sub0=", 5) ||
                   !strnicmp(mBoss, "sub1=", 5)) &&
                    strlen(mBoss) >= 5) {
		        if (!strnicmp(mBoss, "sub0=", 5))  // 一致する日付時差が含まれないとき承認依頼
				{
		          bSub2 =  (wildcard_stricmp(&mBoss[5], itm[2]) ? FALSE: TRUE);
				} else {
			      bSub2 =  (wildcard_stricmp(&mBoss[5], itm[2]) ? TRUE : FALSE);
				}
				if (bSub2)
				{
				  if (r) {
			        strcpy(pContext->mBOSS, r+1);
				  }
				  break;
				}
			  }
		      else
			  //From:ヘッダでチェック対象にする条件
              if ((!strnicmp(mBoss, "from0=", 6) ||
                   !strnicmp(mBoss, "from1=", 6)) &&
                    strlen(mBoss) >= 6) {
		        if (!strnicmp(mBoss, "from0=", 6))  // 一致する日付時差が含まれないとき承認依頼
				{
		          bSub2 =  (wildcard_stricmp(&mBoss[6], mHFrom) ? FALSE: TRUE);
				} else {
			      bSub2 =  (wildcard_stricmp(&mBoss[6], mHFrom) ? TRUE : FALSE);
				}
				if (bSub2)
				{
				  if (r) {
			        strcpy(pContext->mBOSS, r+1);
				  }
				  break;
				}
			  }
		      else
			  //Date:ヘッダの文字列でチェック対象にする条件
              if ((!strnicmp(mBoss, "date0=", 6) ||
                   !strnicmp(mBoss, "date1=", 6)) &&
                    strlen(mBoss) >= 6) {
		        if (!strnicmp(mBoss, "date0=", 6))  // 一致する日付時差が含まれないとき承認依頼
				{
		          bSub2 =  (wildcard_stricmp(&mBoss[6], mDate) ? FALSE: TRUE);
				} else {
			      bSub2 =  (wildcard_stricmp(&mBoss[6], mDate) ? TRUE : FALSE);
				}
				if (bSub2)
				{
				  if (r) {
			        strcpy(pContext->mBOSS, r+1);
				  }
				  break;
				}
			  }
		      else 
#endif
#ifdef UPDATE_20160418 // 拡張ファイルにも併記可能にする。送信先に指定のアドレスまたはドメインが同報として含まれていないとチェック対象にする条件
		      if ((!strnicmp(mBoss, "to0=", 4) || 
			       !strnicmp(mBoss, "to1=", 4) ||
			       !strnicmp(mBoss, "cc0=", 4) || 
			       !strnicmp(mBoss, "cc1=", 4)) &&
			        strlen(mBoss) >= 4) {
			    bExtHit = FALSE;
		        if (!strnicmp(mBoss, "to0=", 4) || // 一致するアドレスが含まれないとき承認依頼
			        !strnicmp(mBoss, "cc0=", 4) )  // 一致するCCアドレスが含まれないとき承認依頼
				{
		          bSub2 =  TRUE;
				} else {
		          bSub2 =  FALSE;
				}
		        if ((fpExt = fopen(pContext->mFnHead, "rt"))) { // エンベロープの送信先をチェック
			      if (!strnicmp(mBoss, "cc0=", 4) ||
				      !strnicmp(mBoss, "cc1=", 4))
				  {
	                while((p = fgets(mBossExt, sizeof(mBossExt), fpExt))) 
					{
			          if (!strnicmp(mBossExt, "recipient:", 10))
					  {
				        break;
					  }
					}
				  }
	              while((p = fgets(mBossExt, sizeof(mBossExt), fpExt))) 
				  {
			        strtok(mBossExt, "\r\n");
			        if (!strnicmp(mBossExt, "recipient:", 10))
					{
				      if (!strnicmp(mBoss, "to0=", 4) || // 一致するアドレスが含まれないとき承認依頼
					      !strnicmp(mBoss, "cc0=", 4) )  // 一致するCCアドレスが含まれないとき承認依頼
					  {
                        if (wildcard_stricmp(&mBoss[4], &mBossExt[11])) // 一致した
						{
                          bSub2 =  FALSE;
						  bExtHit = TRUE;
				          break;
						}
					  } else { // 一致するアドレスが含まれるとき承認依頼
				        if (wildcard_stricmp(&mBoss[4], &mBossExt[11])) // 一致した
						{
                          bSub2 =  TRUE;
						  bExtHit = TRUE;
				          break;
						}
					  }
					}
				  }
			      fclose(fpExt);
				}
#ifdef UPDATE_20160419 // アドレス条件で、承認先上長を変更
				if (bSub2)
				{
				  if (r) {
			        strcpy(pContext->mBOSS, r+1);
				  }
				  break;
				}
#else
				if (bExtHit)
				{
				  break;
				}
#endif
			  } 
		      else 
#endif
			  if (strstr(itm[2], mBoss))
			  {
#ifdef UPDATE_20070616 // キーワードで、承認先上長を変更
				if (r) {
			      strcpy(pContext->mBOSS, r+1);
				}
#endif
				bSub2 = TRUE;
				break;
			  }
#ifdef UPDATE_20180324 // 承認先上長の変更時に変数が初期化後再読み込みするように変更
              memset(mBoss, 0, sizeof(mBoss)); // 初期化
#endif
			}
			fclose(fp);
		  }
		} else {
		  bSub2 = (strstr(itm[2], pContext->mBOSSSubject) ? TRUE : FALSE);
		}
	  } else {
		bSub2 = TRUE;
	  }
#ifdef UPDATE_20111125 // 題名または添付のいづれかの一致で上長承認できる機能を追加
	  if (pContext->mBOSS[0] && (((pContext->nOption == 1 || pContext->nOption == 3) && bSub2) ||
		                          (pContext->nOption == 2 && bSub2 && bAttach && !bReport) ||
								  (pContext->nOption == 3 && bAttach && !bReport) 
								  )) 
#else
#ifdef UPDATE_20090910 // report-type="disposition-notification" なら添付無しと判定する
	  if (pContext->mBOSS[0] && ((pContext->nOption == 1 && bSub2) || (pContext->nOption == 2 && bSub2 && bAttach && !bReport))) 
#else
	  if (pContext->mBOSS[0] && ((pContext->nOption == 1 && bSub2) || (pContext->nOption == 2 && bSub2 && bAttach))) 
#endif
#endif
#else
	  if (pContext->mBOSS[0] && (pContext->nOption == 1 || (pContext->nOption == 2 && bAttach))) 
#endif
	  {
#ifdef UPDATE_20080629 // 承認者は２人までに制限(approvalフォルダに保管するファイル名が、ファイル名の長さの限界があるため)
        nBoss = 0;
#endif	
#ifdef UPDATE_20070713 // 承認者の代理人を設定するオプション
#ifdef UPDATE_20080516 // 複数承認者中に代理承認者設定があった場合の対策
		mBoss2[0] = '\x0';
        q = pContext->mBOSS;
	    while((p = strchr(q, ',')))
		{
	      *p = '\x0';
#ifdef UPDATE_20080629 // 承認者は２人までに制限(approvalフォルダに保管するファイル名が、ファイル名の長さの限界があるため)
#ifdef UPDATE_20090114 //BossCheck 承認者数の設定
		  if (nBoss++ > nMailApprovalNum - 2)
#else
		  if (nBoss++ > 0)
#endif
		  {
			break;
		  }
#endif	
		  strcpy(mBoss1, q);
          GetBaseDirectory(Bdir, mMailBoxDir, mBoss1, pContext->MyAddr); // ユーザ別のフィルタ
          GetProxyUser(Bdir, mBoss1, pContext->MyAddr, 0); // 代理人ファイルから読出し
		  strcat(mBoss2, mBoss1);
		  strcat(mBoss2, ",");
	      q = p + 1;
		} 
		strcpy(mBoss1, q);
        GetBaseDirectory(Bdir, mMailBoxDir, mBoss1, pContext->MyAddr); // ユーザ別のフィルタ
        GetProxyUser(Bdir, mBoss1, pContext->MyAddr, 0); // 代理人ファイルから読出し
		strcat(mBoss2, mBoss1);
		strcpy(pContext->mBOSS, mBoss2);
#else
        GetBaseDirectory(Bdir, mMailBoxDir, pContext->mBOSS, pContext->MyAddr); // ユーザ別のフィルタ
#ifdef UPDATE_20080220 // 承認者の代理人の代理人を設定する
#ifdef UPDATE_20080221 //BossCheck 承認者の代理人の代理人の深さの設定
        GetProxyUser(Bdir, pContext->mBOSS, pContext->MyAddr, 0); // 代理人ファイルから読出し
#else
		GetProxyUser(Bdir, pContext->mBOSS, pContext->MyAddr); // 代理人ファイルから読出し
#endif
#else
		GetProxyUser(Bdir, pContext->mBOSS); // 代理人ファイルから読出し
#endif
#endif
#endif
		strcpy(mBoss, pContext->mBOSS);
		nb[0] = strlen(mBoss);
		for (nb[1] = 0; nb[1] < (6 - nb[0] % 6); nb[1]++)
		  strcat(mBoss, " ");
		translateUue2Base64(mBoss, strlen(mBoss), mBossB64);
	    //translateUue2Base64(pContext->mBOSS, strlen(pContext->mBOSS), mBossB64);
/////////////////////////////////////////
#ifdef UPDATE_20090121 // 上長承認保留ファイル名称の生成ルール変更
		GetLocalTime(&ltime);
		SystemTimeToFileTime(&ltime, &ft);
		sprintf(mft, "%08x%08x", ft.dwHighDateTime, ft.dwLowDateTime);
#ifdef UPDATE_20071204  // メッセージＩＤ採番処理を修正(Bym10id)
#ifdef UPDATE_20090213 // 上長承認メールのmailto:リンクの区切り記号を"?"以外に変更するオプションを追加
		sprintf(mApprADR, "%s%s\\%s%c%s.ADR", mMailSpoolDir, mMailApprovalDir, pContext->mMsgId, mMailApprovalMID[0], mft);
#else
		sprintf(mApprADR, "%s%s\\%s@%s.ADR", mMailSpoolDir, mMailApprovalDir, pContext->mMsgId, mft);
#endif
#else
#ifdef UPDATE_20090213 // 上長承認メールのmailto:リンクの区切り記号を"?"以外に変更するオプションを追加
		sprintf(mApprADR, "%s%s\\B%010lu%c%s.ADR", mMailSpoolDir, mMailApprovalDir, pContext->nMsgId, mMailApprovalMID[0], mft);
#else
		sprintf(mApprADR, "%s%s\\B%010lu@%s.ADR", mMailSpoolDir, mMailApprovalDir, pContext->nMsgId, mft);
#endif
#endif
		if ((fp = fopen(mApprADR, "wt")))
		{
		  fprintf(fp, "%s", mBossB64); // 上長承認者アドレスを取得
		  fclose(fp);
        }
#endif
/////////////////////////////////////////
#ifdef UPDATE_20071204  // メッセージＩＤ採番処理を修正(Bym10id)
#ifdef UPDATE_20090121 // 上長承認保留ファイル名称の生成ルール変更
#ifdef UPDATE_20090213 // 上長承認メールのmailto:リンクの区切り記号を"?"以外に変更するオプションを追加
	    sprintf(mApprEML, "%s%s\\%s%c%s.MSG", mMailSpoolDir, mMailApprovalDir, pContext->mMsgId, mMailApprovalMID[0], mft);
#else
		sprintf(mApprEML, "%s%s\\%s@%s.MSG", mMailSpoolDir, mMailApprovalDir, pContext->mMsgId, mft);
#endif
#else
#ifdef UPDATE_20090213 // 上長承認メールのmailto:リンクの区切り記号を"?"以外に変更するオプションを追加
		sprintf(mApprEML, "%s%s\\%s%c%s.MSG", mMailSpoolDir, mMailApprovalDir, pContext->mMsgId, mMailApprovalMID[0], mBossB64);
#else
		sprintf(mApprEML, "%s%s\\%s@%s.MSG", mMailSpoolDir, mMailApprovalDir, pContext->mMsgId, mBossB64);
#endif
#endif
#else
#ifdef UPDATE_20090121 // 上長承認保留ファイル名称の生成ルール変更
#ifdef UPDATE_20090213 // 上長承認メールのmailto:リンクの区切り記号を"?"以外に変更するオプションを追加
		sprintf(mApprEML, "%s%s\\B%010lu%c%s.MSG", mMailSpoolDir, mMailApprovalDir, pContext->nMsgId, mMailApprovalMID[0], mft);
#else
		sprintf(mApprEML, "%s%s\\B%010lu@%s.MSG", mMailSpoolDir, mMailApprovalDir, pContext->nMsgId, mft);
#endif
#else
#ifdef UPDATE_20090213 // 上長承認メールのmailto:リンクの区切り記号を"?"以外に変更するオプションを追加
		sprintf(mApprEML, "%s%s\\B%010lu%c%s.MSG", mMailSpoolDir, mMailApprovalDir, pContext->nMsgId, mMailApprovalMID[0], mBossB64);
#ifdef UPDATE_20210403 // 上長承認が有効だと添付ファイル分離機能が無効状態にならないようにした。
		sprintf(mApprtmpEML, "%s%s\\B%010lu%c%s.MS$", mMailSpoolDir, mMailApprovalDir, pContext->nMsgId, mMailApprovalMID[0], mBossB64);
#endif
#else
		sprintf(mApprEML, "%s%s\\B%010lu@%s.MSG", mMailSpoolDir, mMailApprovalDir, pContext->nMsgId, mBossB64);
#endif
#endif
#endif
#ifdef UPDATE_20210403 // 上長承認が有効だと添付ファイル分離機能が無効状態にならないようにした。
	    bMailApprovalMove = TRUE; 
#endif
        MoveFileEx(pContext->mFnData, mApprEML, MOVEFILE_COPY_ALLOWED | MOVEFILE_WRITE_THROUGH);
//if (bDebug) printf("MoveFileEx(%s, %s) start\n", pContext->mFnData, mApprEML);
		while(!(fp = fopen(mApprEML, "rt"))) {
          if (bServiceTerminating)
		    break;
	        _sleep(WAIT_TIME);
		  MoveFileEx(pContext->mFnData, mApprEML, MOVEFILE_COPY_ALLOWED | MOVEFILE_WRITE_THROUGH);
        }
		fclose(fp);
//if (bDebug) printf("MoveFileEx(%s, %s) end\n", pContext->mFnData, mApprEML);
#ifdef UPDATE_20071204  // メッセージＩＤ採番処理を修正(Bym10id)
#ifdef UPDATE_20090121 // 上長承認保留ファイル名称の生成ルール変更
#ifdef UPDATE_20090213 // 上長承認メールのmailto:リンクの区切り記号を"?"以外に変更するオプションを追加
		sprintf(mApprRCP, "%s%s\\%s%c%s.RCP", mMailSpoolDir, mMailApprovalDir, pContext->mMsgId, mMailApprovalMID[0], mft);
#else
		sprintf(mApprRCP, "%s%s\\%s@%s.RCP", mMailSpoolDir, mMailApprovalDir, pContext->mMsgId, mft);
#endif
#else
#ifdef UPDATE_20090213 // 上長承認メールのmailto:リンクの区切り記号を"?"以外に変更するオプションを追加
		sprintf(mApprRCP, "%s%s\\%s%c%s.RCP", mMailSpoolDir, mMailApprovalDir, pContext->mMsgId, mMailApprovalMID[0], mBossB64);
#else
	    sprintf(mApprRCP, "%s%s\\%s@%s.RCP", mMailSpoolDir, mMailApprovalDir, pContext->mMsgId, mBossB64);
#endif
#endif
#else
#ifdef UPDATE_20090121 // 上長承認保留ファイル名称の生成ルール変更
#ifdef UPDATE_20090213 // 上長承認メールのmailto:リンクの区切り記号を"?"以外に変更するオプションを追加
		sprintf(mApprRCP, "%s%s\\B%010lu%c%s.RCP", mMailSpoolDir, mMailApprovalDir, pContext->nMsgId, mMailApprovalMID[0], mft);
#else
		sprintf(mApprRCP, "%s%s\\B%010lu@%s.RCP", mMailSpoolDir, mMailApprovalDir, pContext->nMsgId, mft);
#endif
#else
#ifdef UPDATE_20090213 // 上長承認メールのmailto:リンクの区切り記号を"?"以外に変更するオプションを追加
		sprintf(mApprRCP, "%s%s\\B%010lu%c%s.RCP", mMailSpoolDir, mMailApprovalDir, pContext->nMsgId, mMailApprovalMID[0], mBossB64);
#ifdef UPDATE_20210403 // 上長承認が有効だと添付ファイル分離機能が無効状態にならないようにした。
		sprintf(mApprtmpRCP, "%s%s\\B%010lu%c%s.RC$", mMailSpoolDir, mMailApprovalDir, pContext->nMsgId, mMailApprovalMID[0], mBossB64);
#endif
#else
	    sprintf(mApprRCP, "%s%s\\B%010lu@%s.RCP", mMailSpoolDir, mMailApprovalDir, pContext->nMsgId, mBossB64);
#endif
#endif
#endif
        MoveFileEx(pContext->mFnHead, mApprRCP, MOVEFILE_COPY_ALLOWED | MOVEFILE_WRITE_THROUGH);
//if (bDebug) printf("MoveFileEx(%s, %s) start\n", pContext->mFnHead, mApprRCP);
		while(!(fp = fopen(mApprRCP, "rt"))) {
          if (bServiceTerminating)
		    break;
	        _sleep(WAIT_TIME);
		  MoveFileEx(pContext->mFnHead, mApprRCP, MOVEFILE_COPY_ALLOWED | MOVEFILE_WRITE_THROUGH);
        }
		fclose(fp);
//if (bDebug) printf("MoveFileEx(%s, %s) end\n", pContext->mFnHead, mApprRCP);
#ifdef UPDATE_20071204  // メッセージＩＤ採番処理を修正(Bym10id)
	    strcpy(mMSGID, pContext->mMsgId);
#else
	    sprintf(mMSGID, "B%010lu", pContext->nMsgId);
#endif
        /////////////////////////
        sprintf(mLog, "MailToBoss() Start: mMSGID=%s, pContext->mFnData=%s ,pContext->mUIDFROM =%s", mMSGID, pContext->mFnData, pContext->mUIDFROM);
        LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
        /////////////////////////
#ifdef UPDATE_20150807 // BOSSCheckで生成したSubject:ヘッダが１行結合されて出力しないようにした。
        MailToBoss(pContext->LocalName, mMSGID, mApprEML, pContext->mFnData, pContext->mFnHead, pContext->mUIDFROM, pContext->mBOSS, mBossB64, mRawSub);
#else
#ifdef UPDATE_20090121 // 上長承認保留ファイル名称の生成ルール変更
        MailToBoss(pContext->LocalName, mMSGID, mApprEML, pContext->mFnData, pContext->mFnHead, pContext->mUIDFROM, pContext->mBOSS, mft, mSub);
#else
        MailToBoss(pContext->LocalName, mMSGID, mApprEML, pContext->mFnData, pContext->mFnHead, pContext->mUIDFROM, pContext->mBOSS, mBossB64, mSub);
#endif
#endif
#ifdef UPDATE_20240117 // 上長承認通知メールへのドメイン認証ヘッダ挿入対策
#ifdef DOMAIN_AUTH_DKIM    // 2013.01.17 ドメイン認証 DomainkeysとDKIM方式 追加
#ifdef UPDATE_20240118 // DKIM設定箇所の選択 bit0:(受信時付加)デフォルト bit1:上長承認メールへ付加 bit2:添付分離メールへ付加
	   if (bDKIM & 0x2)
#else
	   if (bDKIM)
#endif
	   {
#ifdef UPDATE_20240122 // SPF/DKIM/DMARC/ARC実行パスを半角スペースを含むロングネームでも指定可能にする対策
	     char mCmdl[256];

	     sprintf(mCmdl, "\"%s\"", mDomainAUTHDKIM);
#ifdef UPDATE_20240129 // 承認依頼メールにDKIMサインが追加されない
	     sprintf(mArgv, "\"postmaster@%s\"", pContext->LocalName);
#else
	     sprintf(mArgv, "\"%s\"", pContext->mUIDFROM);
#endif
         _spawnl(_P_WAIT, mDomainAUTHDKIM, mCmdl, pContext->PeerAddr, mArgv, pContext->mFnData, pContext->LocalName, NULL);
#else
	     sprintf(mArgv, "\"%s\"", pContext->mUIDFROM);
         _spawnl(_P_WAIT, mDomainAUTHDKIM, mDomainAUTHDKIM, pContext->PeerAddr, mArgv, pContext->mFnData, pContext->LocalName, NULL);
#endif
	   }
#endif
#endif
#ifdef UPDATE_20080225 // 部下に対する承認のため保留通知
        if (bMailApprovalRes && (!bMailApprovalRevers || bMailApprovalRevers && CheckDomain(pContext->mUIDFROM))) {
#ifdef UPDATE_20231017 // 上長承認依頼メールの作成に失敗した場合の作成リトライ
          sprintf(mLog, "MailToStaff() Start: mMSGID=%s, pContext->mFnData=%s ,pContext->mUIDFROM =%s", mMSGID, pContext->mFnData, pContext->mUIDFROM);
          LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
#endif
#ifdef UPDATE_20150807 // BOSSCheckで生成したSubject:ヘッダが１行結合されて出力しないようにした。
          MailToStaff(pContext->LocalName, mMSGID, mApprEML, pContext->mFnData, pContext->mFnHead, pContext->mBOSS, pContext->mUIDFROM, mBossB64, mRawSub);
#else
#ifdef UPDATE_20090121 // 上長承認保留ファイル名称の生成ルール変更
          MailToStaff(pContext->LocalName, mMSGID, mApprEML, pContext->mFnData, pContext->mFnHead, pContext->mBOSS, pContext->mUIDFROM, mft, mSub);
#else
          MailToStaff(pContext->LocalName, mMSGID, mApprEML, pContext->mFnData, pContext->mFnHead, pContext->mBOSS, pContext->mUIDFROM, mBossB64, mSub);
#endif
#endif
#ifdef UPDATE_20240117 // 上長承認通知メールへのドメイン認証ヘッダ挿入対策
#ifdef DOMAIN_AUTH_DKIM    // 2013.01.17 ドメイン認証 DomainkeysとDKIM方式 追加
#ifdef UPDATE_20240118 // DKIM設定箇所の選択 bit0:(受信時付加)デフォルト bit1:上長承認メールへ付加 bit2:添付分離メールへ付加
	      if (bDKIM & 0x2)
#else
	      if (bDKIM)
#endif
		  {
			CHAR  mSignMSG[256];
#ifdef UPDATE_20240206 // SPF/DKIM/DMARC/ARC実行パスを半角スペースを含むロングネームでも指定可能にする対策
	        char mCmdl[256];

	        sprintf(mCmdl, "\"%s\"", mDomainAUTHDKIM);
#endif
			strcpy(mSignMSG, pContext->mFnData);
			strtok(mSignMSG, ".");
			strcat(mSignMSG, "-S0001.MSG");
#ifdef UPDATE_20240129 // 承認依頼メールにDKIMサインが追加されない
	        sprintf(mArgv, "\"postmaster@%s\"", pContext->LocalName);
#else
	        sprintf(mArgv, "\"%s\"", pContext->mUIDFROM);
#endif
#ifdef UPDATE_20240206 // SPF/DKIM/DMARC/ARC実行パスを半角スペースを含むロングネームでも指定可能にする対策
            _spawnl(_P_WAIT, mDomainAUTHDKIM, mCmdl, pContext->PeerAddr, mArgv, mSignMSG, pContext->LocalName, NULL);
#else
            _spawnl(_P_WAIT, mDomainAUTHDKIM, mDomainAUTHDKIM, pContext->PeerAddr, mArgv, mSignMSG, pContext->LocalName, NULL);
#endif
		  }
#endif
#endif
		}
#endif
	  } else {
#ifdef UPDATE_20070713 // 承認者の代理人を設定するオプション
        GetBaseDirectory(Bdir, mMailBoxDir, pContext->mUIDFROM, pContext->MyAddr); // ユーザ別のフィルタ
        sprintf(mLog, "MailToApproval() Start: pContext->mFnData=%s, pContext->mUIDFROM=%s", pContext->mFnData, pContext->mUIDFROM);
        LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
#ifdef UPDATE_20140623A // 上長承認結果ログに送信元IPの記録を追加した。
        MailToApproval(Bdir, pContext->PeerAddr, pContext->mFnData, pContext->mFnHead,  pContext->mUIDFROM, mAPTo, mAPCc, mSub);
#else
#ifdef UPDATE_20080620 // TOアドレスが代理人になる機能追加
        MailToApproval(Bdir, pContext->mFnData, pContext->mFnHead,  pContext->mUIDFROM, mAPTo, mAPCc, mSub);
#else
	    MailToApproval(Bdir, pContext->mFnData, pContext->mFnHead,  pContext->mUIDFROM, mSub);
#endif
#endif
#else
	    MailToApproval(pContext->mFnData, pContext->mFnHead,  pContext->mUIDFROM, mSub);
#endif
        bDeleApprovalMSG = TRUE;
	  }
	}
#endif
	///// rename massage data //////
#ifdef UPDATE_20161112 //題名ヘッダでのブラック＆ホワイトリストテーブル
    if (bMailApprovalRevers)
    {
      bHitaFilter = bHitaApprovalRev;
    } else 
#endif
    {
#ifdef UPADTE_20040428
      bHitaFilter = FALSE;
#endif
    }
	bNL = pContext->bFL = FALSE;
	nRCP = 1;
#ifdef UPDATE_20040720_LOG
    sprintf(mLog, "fopen(%s)", pContext->mFnHead);
    LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
#endif
#ifndef TEST_MODE
#ifdef UPDATE_20050606 // RCPの再作成でロストしないように待つ
#ifdef UPDATE_20210403 // 上長承認が有効だと添付ファイル分離機能が無効状態にならないようにした。
	while(!(pContext->RCPfp = fopen((bMailApprovalMove ? mApprRCP : pContext->mFnHead), "rt")))
#else
	while(!(pContext->RCPfp = fopen(pContext->mFnHead, "rt")))
#endif
	  _sleep(0);
#else
	pContext->RCPfp = fopen(pContext->mFnHead, "rt");
#endif
#endif
	if (pContext->RCPfp) {
#ifdef DATA_CASH
	  setvbuf(pContext->RCPfp, pContext->mFsbuf, _IOFBF, sizeof(pContext->mFsbuf) );
#endif
	  mMES[0] = '\x0';
#ifdef UPDATE_20071205 //RFC2821: メールアドレス長が255Byteのときの対策
	  mMES[sizeof(mMES)-1] = '\x0';
	  fgets(mMES, sizeof(mMES)-1, pContext->RCPfp); // Message-ID
#else
	  fgets(mMES, sizeof(mMES), pContext->RCPfp); // Message-ID
#endif
	  mRET[0] = '\x0';
#ifdef UPDATE_20071205A //RFC2821: メールアドレス長が255Byteのときの対策
	  mRET[269] = '\x0';
	  fgets(mRET, 269, pContext->RCPfp); // Return-path
#else
	  fgets(mRET, sizeof(mRET), pContext->RCPfp); // Return-path
#endif
	  mRCPTO[0] = '\x0';
#ifdef UPDATE_20071205A //RFC2821: メールアドレス長が255Byteのときの対策
	  mRCPTO[267] = '\x0';
	  fgets(mRCPTO, 267, pContext->RCPfp); // Recipient-1
#else
	  fgets(mRCPTO, sizeof(mRCPTO), pContext->RCPfp); // Recipient-1
#endif
      while(!feof(pContext->RCPfp)) {
        strtok(mRCPTO,"\n");
		if (strnicmp(mRCPTO, "recipient:", 10)) { // recipient:がないなら読飛ばし
	      mRCPTO[0] = '\x0';
#ifdef UPDATE_20071205A //RFC2821: メールアドレス長が255Byteのときの対策
	      mRCPTO[267] = '\x0';
	      fgets(mRCPTO, 267, pContext->RCPfp); // Recipient-1
#else
	      fgets(mRCPTO, sizeof(mRCPTO), pContext->RCPfp); // Recipient-1
#endif
		  continue;
		}
#ifdef UPADTE_20040428
	    if (!Filter(pContext, &mRCPTO[11]))
		  bHitaFilter = TRUE;   // フィルタに一致した
#endif
#ifdef UPADTE_20031120
		mBoundary[0] = '\x0';
        bTEXT = TRUE;
		bFirst = FALSE;
#ifdef UPDATE_20210406 //メーリングリストアドレスが複数同報されている場合添付分離後のメッセージを複写する。
		bInsert = FALSE;
#endif
#ifdef UPDATE_20210314 // MLで添付削除後に続く同報先がMLで添付削除指定があると、MIME構想が狂う
	    bDelAttch = ListsDeletesAttachment(&mRCPTO[11]) && !bSkipDelAttach;
#else
		bDelAttch = ListsDeletesAttachment(&mRCPTO[11]);
#endif
#endif
    /////////////////////////////////////
#ifdef UPDATE_20230620 // 受信メールに任意のヘッダを追加するオプション
	pContext->bReturnPath = pContext->bSender = pContext->bHead = pContext->bID = pContext->bDate = pContext->bReplyTo = FALSE;
#else
	pContext->bHead = pContext->bID = pContext->bDate = pContext->bReplyTo = FALSE;
#endif

//	pContext->bReplyTo = !ListsReplyCheck(pContext->mUIDRCPT); // ヘッダの付加条件
if (bDebug) printf("Start ListsReplyCheck\n");
	pContext->bReplyTo = !ListsReplyCheck(&mRCPTO[11]); // 2001.12.06 ヘッダの付加条件
if (bDebug) printf("End ListsReplyCheck\n");
	/////////////////////////////////////
	// クロスポストなら、RCPを１件ずつに分解する。
	/////////////////////////////////////
	bReq = FALSE;
	strcpy(mUser, &mRCPTO[11]);
#ifdef MULTI_ML
	// strtok(mUser,"@"); 2002.09.04 コメント
	pdom = strstr(mUser, "@");
#else
	strtok(mUser,"@"); 
#endif

#ifdef ANNOUNCE
	if (nRCP > 0)
	  pContext->bMList = TRUE; // 同報ならすべて１アドレスごとに分解
	bReq = FALSE;
#else
#ifdef MULTI_ML // 2002.09.24 ドメイン有りのMLチェック抜け
	if (pdom) { /// 2002.09.04 ドメイン有りのMLチェック
	  *pdom = '@';
      pContext->bMList = GetMLists((LPCTSTR)SOFT_LISTS_REG, (char *)mUser, &bReq);
	  if (!pContext->bMList) {
        if (pdom) { /// 2002.09.04 ドメイン無しのMLのチェック
	      *pdom = '\x0';
          pContext->bMList = GetMLists((LPCTSTR)SOFT_LISTS_REG, (char *)mUser, &bReq);
		}
	  } 
	} else 
#endif
	pContext->bMList = GetMLists((LPCTSTR)SOFT_LISTS_REG, (char *)mUser, &bReq);
#endif
#ifdef UPDATE_20231031 // 上長承認が内部のメーリングリスト宛だと、承認メールが作成されない不具合
    sprintf(mLog, "pContext->bMList=%d / bReq=%d", pContext->bMList, bReq);
    LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
#else
#ifdef UPDATE_20210722_LOG
    sprintf(mLog, "pContext->bMList=%d / bReq=%d", pContext->bMList, bReq);
    LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
#endif
#endif
	if (pContext->bMList && !bReq) {// MLなら
#ifndef TUNING1
#ifdef UPDATE_20041224_FILESPEED
	  sprintf(mFnRCP, "%s%stemp_B%010lu-%03lu.RCX",mMailSpoolDir, mMailQueueDir, pContext->nMsgId, nRCP);
      sprintf(pContext->mFnTemp, "%s%stemp_B%010lu-%03lu.TMP",mMailSpoolDir, mMailQueueDir, pContext->nMsgId, nRCP);
#else
#ifdef UPDATE_20071204  // メッセージＩＤ採番処理を修正(Bym10id)
	  sprintf(mFnRCP, "%s%s-%03lu.RCP",mTempDir,pContext->mMsgId, nRCP);
      sprintf(pContext->mFnTemp, "%s%s-%03lu.TMP",mTempDir,pContext->mMsgId, nRCP);
#else
	  sprintf(mFnRCP, "%sB%010lu-%03lu.RCP",mTempDir,pContext->nMsgId, nRCP);
      sprintf(pContext->mFnTemp, "%sB%010lu-%03lu.TMP",mTempDir,pContext->nMsgId, nRCP);
#endif
#endif
#else
	  sprintf(mFnRCP, "%s%sB%010lu-%03lu.RC$",mMailSpoolDir,mMailQueueDir,pContext->nMsgId, nRCP);
      sprintf(pContext->mFnTemp, "%s%sB%010lu-%03lu.TM$",mMailSpoolDir,mMailQueueDir,pContext->nMsgId, nRCP);
#endif
	  nRCP++;
      fpRcp = fopen(mFnRCP, "wt");
      if (fpRcp) {
#ifdef UPDATE_20071205A //メールアドレス長がバッファ長以上のときの対処
	    strtok(mMES, "\r\n");
	    fprintf(fpRcp, "%s\n", mMES);
	    strtok(mRET , "\r\n");
	    fprintf(fpRcp, "%s\n", mRET);
#else
	    fputs(mMES ,fpRcp);
	    fputs(mRET ,fpRcp);
#endif
#ifdef UPDATE_20210403 // 上長承認が有効だと添付ファイル分離機能が無効状態にならないようにした。
		fprintf(fpRcp, "%s%s\n", (bMailApprovalMove ? "Recipient: " : ""), (bMailApprovalMove ? pContext->mBOSS : mRCPTO));
#else
	    fprintf(fpRcp, "%s\n", mRCPTO);
#endif
	    fclose(fpRcp);
	  }
	} else { // MLでない場合
	  bNL = TRUE;
#ifndef TUNING1
#ifdef UPDATE_20041224_FILESPEED
	  sprintf(mFnRCP, "%s%stemp_B%010lu.RCX",mMailSpoolDir, mMailQueueDir, pContext->nMsgId);
      sprintf(pContext->mFnTemp, "%s%stemp_B%010lu.TMP",mMailSpoolDir, mMailQueueDir, pContext->nMsgId);
#else
#ifdef UPDATE_20071204  // メッセージＩＤ採番処理を修正(Bym10id)
	  sprintf(mFnRCP, "%s%s.RCX",mTempDir,pContext->mMsgId);
      sprintf(pContext->mFnTemp, "%s%s.TMP",mTempDir,pContext->mMsgId);
#else
	  sprintf(mFnRCP, "%sB%010lu.RCX",mTempDir,pContext->nMsgId);
      sprintf(pContext->mFnTemp, "%sB%010lu.TMP",mTempDir,pContext->nMsgId);
#endif
#endif
#else
      sprintf(mFnRCP, "%s%sB%010lu.RCX",mMailSpoolDir,mMailQueueDir,pContext->nMsgId);
      sprintf(pContext->mFnTemp, "%s%sB%010lu.TM$",mMailSpoolDir,mMailQueueDir,pContext->nMsgId);
#endif
      fpRcp = fopen(mFnRCP, "rt");
#ifdef UPDATE_20210722_LOG
    sprintf(mLog, "fopen(%s, rt)=%08x", mFnRCP, fpRcp);
    LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
#endif

      if (fpRcp) { // 追加なら
        fclose(fpRcp);
        fpRcp = fopen(mFnRCP, "at");
        if (fpRcp) {
          fprintf(fpRcp, "%s\n", mRCPTO);
          fclose(fpRcp);
		}
	    mRCPTO[0] = '\x0';
	    fgets(mRCPTO, sizeof(mRCPTO), pContext->RCPfp); // Recipient-1
        continue;
	  } else {     // 新規なら
        fpRcp = fopen(mFnRCP, "wt");
        if (fpRcp) {
#ifdef UPDATE_20071205A //メールアドレス長がバッファ長以上のときの対処
	      strtok(mMES, "\r\n");
	      fprintf(fpRcp, "%s\n", mMES);
	      strtok(mRET , "\r\n");
	      fprintf(fpRcp, "%s\n", mRET);
#else
	      fputs(mMES ,fpRcp);
	      fputs(mRET ,fpRcp);
#endif
	      fprintf(fpRcp, "%s\n", mRCPTO);
	      fclose(fpRcp);
		}
	  }
	}
  	/////////////////////////////////////
#ifdef UPADTE_20040428
#ifdef V3
//printf("bHitaFilter = %d", bHitaFilter);
//printf("Check_Of_MIME(pContext)\n");
    if (!bHitaFilter && !Check_Of_MIME(pContext))
      bHitaFilter = TRUE;
#endif
#endif
#ifdef FIRST_TIME_MAIL // 繰り返す同一内容のメール除去
    if (!bHitaFilter && bFTM_ON) // 同一メールの保管チェックの有無
      if (!FirstTimeMail_PASSIP(pContext->PeerAddr))
        if (FirstTimeMail(pContext->mFnData)) 
	      bHitaFilter = TRUE; // 同一あて先の同一内容メールが届いた
#endif

#ifdef FOR_LGWAN
//pContext->mUIDFROM
//pContext->m
#endif
/*
#ifdef UPDATE_20070516 // 上長承認機能の追加
	if (bMailApproval) {
	  if (pContext->mBOSS[0]) {
	    translateUue2Base64(pContext->mBOSS, strlen(pContext->mBOSS), mBossB64);
	    sprintf(mBackEML, "%s%s\\B%010lu@%s.MSG", mMailSpoolDir, mMailApprovalDir, pContext->nMsgId, mBossB64);
        MoveFileEx(pContext->mFnData, mBackEML, MOVEFILE_COPY_ALLOWED | MOVEFILE_WRITE_THROUGH);
	    sprintf(mBackRCP, "%s%s\\B%010lu@%s.RCP", mMailSpoolDir, mMailApprovalDir, pContext->nMsgId, mBossB64);
        MoveFileEx(mFnRCP, mBackRCP, MOVEFILE_COPY_ALLOWED | MOVEFILE_WRITE_THROUGH);
	    sprintf(mMSGID, "B%010lu", pContext->nMsgId);
        MailToBoss(pContext->LocalName, mMSGID, mBackEML, pContext->mFnData, mFnRCP, pContext->mUIDFROM, pContext->mBOSS, mBossB64);
	  } else {
	    MailToApproval(pContext->mFnData, mFnRCP,  pContext->mUIDFROM);
	  }
	}
#endif
*/
  	/////////////////////////////////////
    pContext->nReceived = 0;
#ifdef UPDATE_20070202 // pContext->Datafpのオープン失敗を回復
#ifdef UPDATE_20210403 // 上長承認が有効だと添付ファイル分離機能が無効状態にならないようにした。
	while (!(pContext->Datafp = fopen((bMailApprovalMove ? mApprEML : pContext->mFnData), "rb"))) 
#else
	 while (!(pContext->Datafp = fopen(pContext->mFnData, "rb"))) 
#endif
	 {
#ifdef UPDATE_20210403_20210701 // 上長承認が有効だと添付ファイル分離機能が無効状態にならないようにした。
	   sprintf(mLog, "Don't fopen(%s, %d, rb).", (bMailApprovalMove ? mApprEML : pContext->mFnData), bMailApprovalMove);
#else
       sprintf(mLog, "Don't fopen(%s, rb).", pContext->mFnData);
#endif
       LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
       if (bServiceTerminating)
         break;
	   _sleep(100);
	 }
#else
    pContext->Datafp = fopen(pContext->mFnData, "rb");
#endif
	if (pContext->Datafp) {
#ifdef DATA_CASH
	  setvbuf(pContext->Datafp, pContext->mFrbuf, _IOFBF, sizeof(pContext->mFrbuf) );
#endif
#ifdef UPDATE_20040720_LOG
      sprintf(mLog, "fopen(%s, wb)", pContext->mFnTemp);
      LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
#endif
#ifdef UPDATE_20240112 // MIMEパートが添付のみだと分離されない
	  bDTXT = FALSE;
#endif
#ifdef UPDATE_20240113 // MIMEパート無しの添付メールだと分離されない
	  bAttachOnly = FALSE; // MIMEパート無しの添付
#endif
#ifdef UPDATE_20240116 // メール本体の文字エンコードがbase64の場合の添付分離時のメールに、Content-Transfer-Encoding:ヘッダが重複してしまう不具合。
	  bHitCT = FALSE;
      nCEnc = 0;
	  mCEnc[0]='\x0';
#endif

#ifdef UPDATE_20070202 // pContext->Datafpのオープン失敗を回復
#ifdef UPDATE_20210403 // 上長承認が有効だと添付ファイル分離機能が無効状態にならないようにした。
	while (!(pContext->Tempfp = fopen((bMailApprovalMove ? mApprtmpEML : pContext->mFnTemp), "wb"))) 
#else
	 while (!(pContext->Tempfp = fopen(pContext->mFnTemp, "wb"))) 
#endif
	 {
#ifdef UPDATE_20210403_20210701 // 上長承認が有効だと添付ファイル分離機能が無効状態にならないようにした。
	   sprintf(mLog, "Don't fopen(%s, %d, wb).", (bMailApprovalMove ? mApprtmpEML : pContext->mFnTemp), bMailApprovalMove);
#else
       sprintf(mLog, "Don't fopen(%s, wb).", pContext->mFnTemp);
#endif
       LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
       if (bServiceTerminating)
         break;
	   _sleep(100);
	 }
#else
      pContext->Tempfp = fopen(pContext->mFnTemp, "wb");
#endif
	  if (pContext->Tempfp) {
#ifdef DATA_CASH
		setvbuf( pContext->Tempfp, pContext->mFwbuf, _IOFBF, sizeof(pContext->mFwbuf) );
#endif
		pContext->bFL = TRUE;
#ifdef UPDATE_20170512 // ヘッダ一行の文字数が受信バッファを超えている場合、空欄行が含まれてしまう不具合
	    fgets(pContext->mdata, RFC2822_LINE_LENGTH_LIMIT+1, pContext->Datafp);
		*(pContext->mdata+RFC2822_LINE_LENGTH_LIMIT+1)='\x0';
#else
	    fgets(pContext->mdata, sizeof(pContext->mdata), pContext->Datafp);
#endif
	    while(!feof(pContext->Datafp)){
		  //// 改行コードが<CR>のみ又は<CR><CR><LF>のデータは<CR><LF>に置換え
		  if ((pCrLf = strstr(pContext->mdata, "\r\r\n"))) {
		    *pCrLf = '\x0';
			strcat(pCrLf, "\r\n");
		  } else {
		    pCrLf = strstr(pContext->mdata, "\r\n");
		    if (!pCrLf) {
			  pCrLf = strstr(pContext->mdata, "\r");
			  if (!pCrLf) {
			    pCrLf = strstr(pContext->mdata, "\n");
				if (!pCrLf) 
			      pCrLf = pContext->mdata;
#ifdef UPDATE_20170512 // ヘッダ一行の文字数が受信バッファを超えている場合、空欄行が含まれてしまう不具合
			  } else {
				if ((nTerm = fgetc(pContext->Datafp)) != '\n')
				{
				  fseek(pContext->Datafp, -1L, SEEK_CUR);
				}
#endif
			  }
#ifdef UPDATE_20170512 // ヘッダ一行の文字数が受信バッファを超えている場合、空欄行が含まれてしまう不具合
              if (pCrLf != pContext->mdata || pCrLf == pContext->mdata && strlen(pContext->mdata) < RFC2822_LINE_LENGTH_LIMIT)
#endif
			  {
		        *pCrLf = '\x0';
		        strcat(pCrLf, "\r\n");
			  }
			}
		  }
		  //////////////////////////////////////////////
		  if (!strrchr(pContext->mdata, '\r')) {
			if (!strchr(pContext->mdata, '\n') && *pContext->mdata != '\n')
#ifdef UPDATE_20170512 // ヘッダ一行の文字数が受信バッファを超えている場合、空欄行が含まれてしまう不具合
              if (strlen(pContext->mdata) < sizeof(pContext->mdata)-RFC2822_LINE_LENGTH_LIMIT)
#endif
			    strcat(pContext->mdata, "\n");
		  }
		  //////////////////////////////////////////////
#ifdef UPDATE_20060306 // ヘッダ区切りのない本文への対応
		  if( bBODY ||
			  (!pContext->bHead &&
			   (strcmp(pContext->mdata, "\r\n") == 0 ||
			    strcmp(pContext->mdata, "\n") == 0   ||
		        strcmp(pContext->mdata,".\r\n") == 0 ||
			    strcmp(pContext->mdata,".\n") == 0)) )
#else
		  if( !pContext->bHead &&
			  (strcmp(pContext->mdata, "\r\n") == 0 ||
			   strcmp(pContext->mdata, "\n") == 0   ||
		       strcmp(pContext->mdata,".\r\n") == 0 ||
			   strcmp(pContext->mdata,".\n") == 0)  )
#endif
		  {
 		    // end header
#ifdef UPADTE_20040107
			////////////////////////////////////////////
			// 本文が空の場合、手前に空白行を追加する。
			// OEで読み込める用に対策
		    if (!strcmp(pContext->mdata,".\r\n") ||
			    !strcmp(pContext->mdata,".\n")) {
				strcpy(pContext->mdata, "\r\n.\r\n");
			}
#endif
            pContext->bHead = TRUE;
			if (bSub) {
              SubjectConv(pContext->Tempfp, 
			 	          mMLWord,
						  pContext->mMLtkn, 
						  mSubject, 
						  mDBSubject,
						  mPack);
		      bSub = FALSE;
			}
			if (!pContext->bID) {
#ifdef K_SEARCH // K_SEARCH OEM 版
			  sprintf(pContext->mhdat, "Message-Id: <%s@%s>\r\n",
                     pContext->mMsgId,
                     pContext->LocalName);
#else
#ifdef UPDATE_20071204  // メッセージＩＤ採番処理を修正(Bym10id)
			  sprintf(pContext->mhdat, "Message-Id: <%s@%s>\r\n",
                     pContext->mMsgId,
                     pContext->LocalName);
#else
			  sprintf(pContext->mhdat, "Message-Id: <B%010lu@%s>\r\n",
                     pContext->nMsgId,
                     pContext->LocalName);
#endif
#endif
              fputs(pContext->mhdat, pContext->Tempfp);
			} 
			if (!pContext->bSubject) {
			  if (ListsCheck(pContext->mUIDFROM, &mRCPTO[11], pContext->mMLtkn, NULL, NULL, TRUE)) { //2001.12.06
                sprintf(pContext->mhdat,"Subject: %s\r\n", pContext->mMLtkn);
                fputs(pContext->mhdat, pContext->Tempfp);
			  }
			}
		    if (!pContext->bDate) {
    	      gettime(&pContext->ltime, pContext->mtime);
	          sprintf(pContext->mhdat,"Date: %s\r\n", pContext->mtime);
              fputs(pContext->mhdat, pContext->Tempfp);
			}
			if (pContext->bMList && !pContext->bFrom) {
              sprintf(pContext->mhdat,"From: %s\r\n", pContext->mUIDFROM);
              fputs(pContext->mhdat, pContext->Tempfp);
			}
#ifdef UPDATE_20230620 // 受信メールに任意のヘッダを追加するオプション
		    if (!pContext->bSender && pContext->mSender[0]) {
	          sprintf(pContext->mhdat,"%s\r\n", pContext->mSender);
              fputs(pContext->mhdat, pContext->Tempfp);
			}
		    if (!pContext->bSender && pContext->mReturnPath[0]) {
	          sprintf(pContext->mhdat,"%s\r\n", pContext->mReturnPath);
              fputs(pContext->mhdat, pContext->Tempfp);
			}
			if (!pContext->bReplyTo) {
              if (pContext->bMList)
			  {
			    if (ListsCheck(pContext->mUIDFROM, &mRCPTO[11], pContext->mMLtkn, NULL, NULL, FALSE)) { //2001.12.06
				  sprintf(pContext->mhdat,"Reply-To: %s\r\n", &mRCPTO[11]); // 2001.12.06
                  fputs(pContext->mhdat, pContext->Tempfp);
				} else if (pContext->mReplyTo[0]) {
	              sprintf(pContext->mhdat,"%s\r\n", pContext->mReplyTo);
                  fputs(pContext->mhdat, pContext->Tempfp);
				}
			  } else if (pContext->mReplyTo[0]) {
	            sprintf(pContext->mhdat,"%s\r\n", pContext->mReplyTo);
                fputs(pContext->mhdat, pContext->Tempfp);
			  }
			}
#else
			if (pContext->bMList && !pContext->bReplyTo) {
			  if (ListsCheck(pContext->mUIDFROM, &mRCPTO[11], pContext->mMLtkn, NULL, NULL, FALSE)) { //2001.12.06
				sprintf(pContext->mhdat,"Reply-To: %s\r\n", &mRCPTO[11]); // 2001.12.06
                fputs(pContext->mhdat, pContext->Tempfp);
			  } 
			}
#endif
#ifdef UPDATE_20060306 // ヘッダ区切りのない本文への対応
			if (!pContext->bMList && !pContext->bFrom) {
              sprintf(pContext->mhdat,"From: %s\r\n", pContext->mUIDFROM);
              fputs(pContext->mhdat, pContext->Tempfp);
			}
			if (!pContext->bFrom) {
              strcpy(pContext->mhdat,"To: undisclosed-recipients:;\r\n");
              fputs(pContext->mhdat, pContext->Tempfp);
			}
#endif
		  }
		  if (!pContext->bHead) {
#ifdef UPDATE_20060306 // ヘッダ区切りのない本文への対応
			if (!(strpbrk(pContext->mdata, ":") ||
		          pContext->mdata[0] == '\t' ||
				  pContext->mdata[0] == ' ')) {
              bBODY = TRUE;
			  continue;
			}
#endif
		    strncpy(pContext->mtkn, pContext->mdata, 12);
			pContext->mtkn[12] = '\x0';
		    strtok(pContext->mtkn," \r\n");
#ifdef UPDATE_20100129 // メールヘッダの区切りにホワイトスペースが無いとヘッダを認識しない
		    if (pTerm = strchr(pContext->mtkn, ':'))
			{
			  *(pTerm+1)= '\x0';
			}
#endif
#ifdef UPDATE_20240127 // メーリングリスト題名付加時メールへのドメイン認証ヘッダ挿入対策
			if (pContext->bMList && mMLWord[0])
			{
			  strncpy(pContext->mtkn2, pContext->mdata, 128);
			  pContext->mtkn2[128] = '\x0';
			  strlwr(pContext->mtkn2);
		      strtok(pContext->mtkn2,"\r\n");
#ifdef UPDATE_20240118 // DKIM設定箇所の選択 bit0:(受信時付加)デフォルト bit1:上長承認メールへ付加 bit2:添付分離メールへ付加
	          if ((bDKIM & 0x4) &&
#else
			  if (bDKIM &&
#endif
				  (!strnicmp(pContext->mtkn2, "dkim-signature:", 15) ||
                   !strnicmp(pContext->mtkn2, "arc-authentication-results:", 27) ||
                   !strnicmp(pContext->mtkn2, "arc-message-signature:", 22) ||
			       !strnicmp(pContext->mtkn2, "arc-seal:", 9) ||
				   !strnicmp(pContext->mtkn2, "authentication-results:", 23)))
			  {
			    // ５ヘッダは除去
                if (bDebug) printf("%s\n", pContext->mtkn2);
	            while(fgets(pContext->mdata, RFC2822_LINE_LENGTH_LIMIT+1, pContext->Datafp)) // 読み飛ばし
				{
				  if (!(pContext->mdata[0] == ' ' || pContext->mdata[0] == '\t'))
				  {
				    break;
				  }
				}
			    if (!(pContext->mdata[0] == ' ' || pContext->mdata[0] == '\t'))
			      continue;
			  }
			}
#endif
#ifdef UPADTE_20031120
			if (pContext->bMList && bDelAttch) { // MLの場合のみチェック
			  strncpy(pContext->mtkn2, pContext->mdata, 128);
			  pContext->mtkn2[128] = '\x0';
			  strlwr(pContext->mtkn2);
		      strtok(pContext->mtkn2,"\r\n");
#ifdef UPDATE_20240117A // メーリングリスト＆添付分離メールへのドメイン認証ヘッダ挿入対策
#ifdef UPDATE_20240118 // DKIM設定箇所の選択 bit0:(受信時付加)デフォルト bit1:上長承認メールへ付加 bit2:添付分離メールへ付加
	          if ((bDKIM & 0x4) &&
#else
			  if (bDKIM &&
#endif
				  (!strnicmp(pContext->mtkn2, "dkim-signature:", 15) ||
                   !strnicmp(pContext->mtkn2, "arc-authentication-results:", 27) ||
                   !strnicmp(pContext->mtkn2, "arc-message-signature:", 22) ||
			       !strnicmp(pContext->mtkn2, "arc-seal:", 9) ||
				   !strnicmp(pContext->mtkn2, "authentication-results:", 23)))
			  {
			    // ５ヘッダは除去
                if (bDebug) printf("%s\n", pContext->mtkn2);
	            while(fgets(pContext->mdata, RFC2822_LINE_LENGTH_LIMIT+1, pContext->Datafp)) // 読み飛ばし
				{
				  if (!(pContext->mdata[0] == ' ' || pContext->mdata[0] == '\t'))
				  {
				    break;
				  }
				}
			    if (!(pContext->mdata[0] == ' ' || pContext->mdata[0] == '\t'))
			      continue;
			  }
#endif
#ifdef UPDATE_20240116 // メール本体の文字エンコードがbase64の場合の添付分離時のメールに、Content-Transfer-Encoding:ヘッダが重複してしまう不具合。
              if ((!bHitCT || bAttachOnly) && !strnicmp(pContext->mtkn2, "content-transfer-encoding:", 26))
			  {
				if (!bAttachOnly)
				{
				  nCEnc = 1;
				  strcpy(mCEnc, pContext->mdata);
				} else {
	              while(fgets(pContext->mdata, RFC2822_LINE_LENGTH_LIMIT+1, pContext->Datafp)) // 読み飛ばし
				  {
				    if (!(pContext->mdata[0] == ' ' || pContext->mdata[0] == '\t'))
					{
					  break;
					}
				  }
				  if (!(pContext->mdata[0] == ' ' || pContext->mdata[0] == '\t'))
				    continue;
				}
			  }
#endif
#ifdef UPDATE_20240113 // MIMEパート無しの添付メールだと分離されない
#ifdef UPDATE_20240116 // メール本体の文字エンコードがbase64の場合の添付分離時のメールに、Content-Transfer-Encoding:ヘッダが重複してしまう不具合。
              if (!strnicmp(pContext->mtkn2, "content-type:", 13))
			  {
				bHitCT = TRUE;
				if ((!strstr(pContext->mtkn2, "text/") && !strstr(pContext->mtkn2, "multipart/")))
				{
				  bDTXT = bFirst = bTEXT = bAttachOnly = TRUE;
				  pContext->bFrom = TRUE;
	              while(fgets(pContext->mdata, RFC2822_LINE_LENGTH_LIMIT+1, pContext->Datafp)) // 読み飛ばし
				  {
				    if (!(pContext->mdata[0] == ' ' || pContext->mdata[0] == '\t'))
					{
					  break;
					}
				  }
				  if (!(pContext->mdata[0] == ' ' || pContext->mdata[0] == '\t'))
				    continue;
				} else if (mCEnc[0]) {
				  fputs(mCEnc, pContext->Tempfp);
				}
			  }
			  if (bAttachOnly && !strnicmp(pContext->mdata, "content-disposition:", 20)) // 読み飛ばし
			  {
	            while(fgets(pContext->mdata, RFC2822_LINE_LENGTH_LIMIT+1, pContext->Datafp))
				{
				  if (!(pContext->mdata[0] == ' ' || pContext->mdata[0] == '\t'))
				  {
                    //fseek(pContext->Datafp, (unsigned long)(0-strlen(pContext->mdata)), SEEK_CUR);
				    break;
				  }
				}
				if (!(pContext->mdata[0] == ' ' || pContext->mdata[0] == '\t'))
				  continue;
			  }
#else
              if (!strnicmp(pContext->mtkn2, "content-type:", 13) && (!strstr(pContext->mtkn2, "text/") && !strstr(pContext->mtkn2, "multipart/")))
			  {
				 bDTXT = bFirst = bTEXT = bAttachOnly = TRUE;
			  }
#endif
#endif
#ifdef UPDATE_20210317 // メーリングリストあてで添付削除指定の際、メールヘッダの構造が"multipart/alternative"のとき、2番目のMIME構造が添付と判定され削除されないようにした。
              if (strstr(pContext->mtkn2, "multipart/alternative"))
			  {
				 bHeadAlt = TRUE;
			  }
			  if (!bHeadAlt && (pBoundary = strstr(pContext->mtkn2, "boundary"))) 
#else
			  if ((pBoundary = strstr(pContext->mtkn2, "boundary"))) 
#endif
			  {
			    pBoundary += 8;
#ifdef UPDATE_20080513 // boundaryのデータに"ダブルクォーテーションが無いとハングする
				if (!strchr(pBoundary, '"'))  // 区切りがないなら
				{
				  pBoundary++;
				} else
#endif
				{
			      while(*pBoundary!='"') // トークン開始カラムを探す
			        pBoundary++;
			      pBoundary++;          // 見つけた次のカラムからトークンを取り出す。
				}
			    strncpy(pContext->mtkn2, pContext->mdata, 128);
			    pContext->mtkn2[128] = '\x0';
		        strtok(pContext->mtkn2,"\r\n");
			    strcpy(mBoundary, pBoundary);
			    strtok(mBoundary, "\"\r\n");
			  }
			}
#endif
#ifndef K_SEARCH // K_SEARCH OEM 版
			if (_stricmp(pContext->mtkn, "received:") == 0) {
			  pContext->nReceived++;
			}
#endif //#ifndef K_SEARCH // K_SEARCH OEM 版
			if (_stricmp(pContext->mtkn, "message-id:") == 0) {
		      pContext->bID = TRUE;
			}
			if (_stricmp(pContext->mtkn, "date:") == 0) {
		      pContext->bDate = TRUE;
			} 
			if (_stricmp(pContext->mtkn, "reply-to:") == 0) {
			  pContext->bReplyTo = TRUE;
			}
#ifdef UPDATE_20230620 // 受信メールに任意のヘッダを追加するオプション
			if (_stricmp(pContext->mtkn, "sender:") == 0) {
			  pContext->bSender = TRUE;
			}
			if (_stricmp(pContext->mtkn, "return-path:") == 0) {
			  pContext->bReturnPath = TRUE;
			}
#endif
			if (_stricmp(pContext->mtkn, "from:") == 0) {
			  pContext->bFrom = TRUE;
			}
            if (_stricmp(pContext->mtkn, "subject:") == 0 ||
				(bSub && (pContext->mtkn[0] == '\x09' || 
				          pContext->mtkn[0] == '\x20' )) ) {
			  if (!bSub) {
			    bML = bList = ListsCheck(pContext->mUIDFROM, &mRCPTO[11], pContext->mMLtkn, mMLWord, &nMLNo, TRUE);
 	            mDBSubject[0] = '\x0';
#ifdef UPDATE_20210303 // メーリングリストに付加表題が無いとき題名を再構築できない不具合
			    mDBSubject2[0] = '\x0';
#endif
				if (bList)
				  bSub = TRUE;
			  } else {
//strcat(mDBSubject, "\r\n ");
				bList = FALSE;
			  }
#ifndef UPDATE_20080219 // メーリングリストの保存で付加表題がNULLの場合題名が記載されない
#ifdef UPDATE_20070928 // メーリングリスト MIME-Qへの対応と付加へ空なら処理しない
              if (!mMLWord[0]) {
				bSub = bList = FALSE;
			  }
#endif
#endif
			  if (bList || bSub) {
 				if (bList) {
				  fputs(pContext->mtkn, pContext->Tempfp);
			      fputs(" ", pContext->Tempfp);
				}
			    strtok(pContext->mdata,"\r\n");
#ifdef UPDATE_20151118 // パックされている言語の末尾のホワイトスペースを除去する
				if ((pWBSP[0] = strstr(pContext->mdata,"=?")))
				{
				  while(pWBSP[0])
				  {
#ifdef UPDATE_20241116_ML // MLへの付加表題がMIMEエンコードで記述されていると、付加表題の付け替えに失敗する
				    if (pWBSP[1] = strstr(pWBSP[0]+2, "=?")) // MIMEエンコード間にACSCIIコードの並びがあるならホワイトスペース処理はしない。
					{
					  if (pWBSP[0] = strstr(pWBSP[0], "?=")) // MIMEの末尾
					  {
					    do
						{
					      pWBSP[1]--;
						  if (*pWBSP[1] != '\x20' && *pWBSP[1] != '\x09')
						    break;
						} while (pWBSP[1] > pWBSP[0]+1);
					    if (pWBSP[1] > pWBSP[0]+2)
						{
					      pWBSP[0] = strstr(pWBSP[0]+2,"=?");
						}
					  }
					}
#endif
				    if ((pWBSP[1] = strstr(pWBSP[0], "?=\x09")) ||
					    (pWBSP[1] = strstr(pWBSP[0], "?=\x20")) )
					{
					  pWBSP[1]+=2;
					  pWBSP[0] = pWBSP[1];
				      while((*pWBSP[1] == '\x09' || *pWBSP[1] == '\x20')) {
					    *pWBSP[1] = '\x0';
				        pWBSP[1]++;   // ホワイトスペースを除去
					  }
					  strcat(pContext->mdata, pWBSP[1]);
					  pWBSP[0] = strstr(pWBSP[0],"=?");
					} else {
					  break;
					}
				  }
				}
#endif
#ifdef UPDATE_20240221 // 文字コード区切りでないデータへの対応
#ifdef UPDATE_20240229 // 付加表題付きのＭＬでない場合に処理不用
				if (mMLWord[0] && nUTF8ToSJISTBL != 0)
#else
				if (nUTF8ToSJISTBL != 0)
#endif
				{
				  //BOOL bSkip = FALSE;
				  char *pWS;
				  char mSubj[1024];
				  //unsigned long npt = ftell(pContext->Datafp);
				  while (fgets(mSubj, sizeof(mSubj), pContext->Datafp))
				  {
					if (mSubj[0] != ' ' && mSubj[0] != '\t')
					{
					  strtok(pContext->mdata, "\r\n");
					  fseek(pContext->Datafp, (long)(0-strlen(mSubj)), SEEK_CUR);
					  break;
					}
					//strtok(mSubj, "\r\n");
					pWS = mSubj;
					while(*pWS == ' ' || *pWS =='\t')
					  pWS++;
					if (strlen(pContext->mdata)+strlen(pWS) < sizeof(mSubj))
					{
					  //bSkip = TRUE;
					  strtok(pContext->mdata, "\r\n");
					  strcat(pContext->mdata, pWS);
					}
				  }
				}
#endif
#ifdef UPDATE_20050107
				LineConv(mSubject, sizeof(mSubject), pContext->mdata , mPack);
#else
				LineConv(mSubject, pContext->mdata , mPack);
#endif
				if (bList) {
				  if ((pWSP[0] = strpbrk(pContext->mdata,":")))
					pWSP[0]++;
				  if ((pWSP[1] = strpbrk(mSubject,":")))
					pWSP[1]++;
// v4.94 00005d4c ハング箇所周辺
//pWSP[1]=NULL; // この設定で 00005d50でハング
////////////////////////////
				} else {
				  pWSP[0] = pContext->mdata;
				  pWSP[1] = mSubject;
				} 
				// ホワイトスペースを除去
				while((*pWSP[0] == '\x09' || *pWSP[0] == '\x20')) {
				  pWSP[0]++;   
				}
				while((*pWSP[1] == '\x09' || *pWSP[1] == '\x20')) {
				  pWSP[1]++;   // ホワイトスペースを除去
				}
			    // 1Byteコードか2Byteコードか判断
				if (!strncmp(pWSP[0], "=?",2) || *pWSP[0] == '\x1b') {
#ifdef UPDATE_20050111
				  if (sizeof(mDBSubject) > strlen(mDBSubject) + 1 )
				    if (!bList && b1Byte)
				      strcat(mDBSubject, " ");
#else
				  if (!bList && b1Byte)
				    strcat(mDBSubject, " ");
#endif
				} else {
#ifdef UPDATE_20050111
				  if (sizeof(mDBSubject) > strlen(mDBSubject) + 1 )
				    if (!bList) // && !b1Byte) // 1Byteコードは常に1SPあける
				      strcat(mDBSubject, " ");
#else
				  if (!bList) // && !b1Byte) // 1Byteコードは常に1SPあける
				    strcat(mDBSubject, " ");
#endif
			  	  //b1Byte = TRUE;
				}
				///////////////////////////
			    /// 末尾の判定
			    b1Byte = TRUE;    // 1Byteコードで終了
				if ((nWSP = strlen(pWSP[0])) >= 2)
					if (!strcmp(&pWSP[0][nWSP-2], "?="))
				    b1Byte = FALSE;   // 2Byteコードで終了
				///////////////////////////
#ifdef UPDATE_20170923 // メーリングリストに付加表題が無いとき題名を再構築しない
				if (sizeof(mDBSubject) > strlen(mDBSubject) + strlen(mMLWord[0] ? pWSP[1] : pWSP[0]) )
				{
                    if (!mMLWord[0] && mDBSubject[0])
					{
					  strcat(mDBSubject, " ");
					}
					strcat(mDBSubject, mMLWord[0] ? pWSP[1] : pWSP[0]);
#ifdef UPDATE_20210303 // メーリングリストに付加表題が無いとき題名を再構築できない不具合
			        strcat(mDBSubject2, pWSP[1]);
#endif
					if (!mMLWord[0])
					{
					  strcat(mDBSubject, "\r\n");
					}
				}
#else
#ifdef UPDATE_20050111
				if (sizeof(mDBSubject) > strlen(mDBSubject) + strlen(pWSP[1]) )
				  strcat(mDBSubject, pWSP[1]);
#else
				  strncat(mDBSubject, pWSP[1] /*(bList ? &mSubject[9] : pWSP)*/, 128-strlen(mDBSubject));
#endif
#endif
//printf("mDBSubject Len = %d \n", strlen(mDBSubject));
				strcat(pContext->mdata,"\r\n");
			  }
			  pContext->bSubject = TRUE;
			} else {
			  if (bSub) {
                SubjectConv(pContext->Tempfp, 
					        mMLWord,
							pContext->mMLtkn, 
							mSubject, 
							mDBSubject,
							mPack);
 			    bSub = FALSE;
			  }
			}
#ifdef UPADTE_20031120
		  } else {  // 本文チェック
#ifdef UPDATE_20240113 // MIMEパート無しの添付メールだと分離されない
		    if ((bAttachOnly || mBoundary[0]) && bDelAttch && !bSIG) 
#else
#ifdef UPDATE_20230302 // 署名鍵入りメールの添付分離を行わないようにする
		    if (mBoundary[0] && bDelAttch && !bSIG) 
#else
		    if (mBoundary[0] && bDelAttch) 
#endif
#endif
			{  // MLへの添付ファイル削除の指定がある場合の処理
#ifdef UPDATE_20210427 // 本文のCharsetに合わせてリンク挿入できるようにした。
				if (!mcs[0])
				{
				  if (pcs = strstr(pContext->mdata, "charset="))
				  {
				    pcs += 8;
				    while(*pcs == ' ' || *pcs == '\t' || *pcs == '"')
  					  pcs++;
				    strcpy(mcs, pcs);
				    strtok(mcs, " ;\"\r\n\t");
				    strlwr(mcs);
				  }
				}
#endif
#ifdef UPDATE_20210304 // メーリングリストで投稿内容保存ファイルの原本を保存するオプションを追加した。
#ifdef UPDATE_20210321 // Content-Typeがmultipart/related;の時の処理
                    if (!mALTBoundary[0] &&
						(strstr(pContext->mtkn2, "multipart/related") || strstr(pContext->mtkn2, "multipart/alternative")) &&
#else
				  	if (strstr(pContext->mtkn2, "multipart/alternative") &&
#endif
 			            (pBoundary = strstr(pContext->mdata, "boundary")))
					{
			          pBoundary += 8;
				      if (!strchr(pBoundary, '"'))  // 区切りがないなら
					  {
				        pBoundary++;
					  } else
					  {
			          while(*pBoundary!='"') // トークン開始カラムを探す
			            pBoundary++;
			          pBoundary++;          // 見つけた次のカラムからトークンを取り出す。
					  }
			          strcpy(mALTBoundary, pBoundary);
			          strtok(mALTBoundary, "\"\r\n");
#ifndef UPDATE_20210304 // メーリングリストで投稿内容保存ファイルの原本を保存するオプションを追加した。
					  bFirst = FALSE;
#endif
					}
#ifdef UPDATE_20240113 // MIMEパート無しの添付メールだと分離されない
			  if (!mALTBoundary[0] && (bAttachOnly || !strncmp(&pContext->mdata[2], mBoundary, strlen(mBoundary))) ||
				  mALTBoundary[0] && !strncmp(&pContext->mdata[2], mALTBoundary, strlen(mALTBoundary))) // 完全一致であること
#else
			  if (!mALTBoundary[0] && !strncmp(&pContext->mdata[2], mBoundary, strlen(mBoundary)) ||
				  mALTBoundary[0] && !strncmp(&pContext->mdata[2], mALTBoundary, strlen(mALTBoundary))) // 完全一致であること
#endif
#else
			  if (!strncmp(&pContext->mdata[2], mBoundary, strlen(mBoundary)))  // 完全一致であること
#endif
			  {
#ifdef UPDATE_20240112 // MIMEパートが添付のみだと分離されない
			    npos = ftell(pContext->Datafp);
#endif
			    bTEXT = TRUE;
#ifdef UPDATE_20210304 // メーリングリストで投稿内容保存ファイルの原本を保存するオプションを追加した。
#ifdef UPADTE_20210312 // リンク挿入時にMIMEで分割した。
#ifdef UPDATE_20240113 // MIMEパート無しの添付メールだと分離されない
				if (!mALTBoundary[0] && (bAttachOnly || strlen(&pContext->mdata[2])-2 == strlen(mBoundary)) ||
                    (mALTBoundary[0] && strlen(&pContext->mdata[2])-4 == strlen(mALTBoundary) && !strncmp(&pContext->mdata[strlen(mALTBoundary)+2], "--", 2)))
#else
				if (!mALTBoundary[0] && strlen(&pContext->mdata[2])-2 == strlen(mBoundary) ||
                    (mALTBoundary[0] && strlen(&pContext->mdata[2])-4 == strlen(mALTBoundary) && !strncmp(&pContext->mdata[strlen(mALTBoundary)+2], "--", 2)))
#endif
#else
				if (!mALTBoundary[0] && strlen(&pContext->mdata[2])-2 == strlen(mBoundary) ||
                    (mALTBoundary[0] && strlen(&pContext->mdata[2])-4 == strlen(mALTBoundary) && !strncmp(&pContext->mdata[strlen(mALTBoundary)+2], "--", 2)) ||
					mALTBoundary[0] && strlen(&pContext->mdata[2])-2 == strlen(mALTBoundary))
#endif
#else
				if (strlen(&pContext->mdata[2])-2 == strlen(mBoundary))
#endif
				{
#ifdef UPDATE_20210304 // メーリングリストで投稿内容保存ファイルの原本を保存するオプションを追加した。
#ifdef UPDATE_20210306 // メーリングリストの添付削除でContent-TypeがTEXT/PLAINかTEXT/HTML以外のTEXTが含まれないようにした。
					if (bFirst && 
#ifdef UPADTE_20210312 // リンク挿入時にMIMEで分割した。
#ifdef UPDATE_20210321 // Content-Typeがmultipart/related;の時の処理
#ifdef UPDATE_20210322 // 添付にTEXTやHTMLファイルがあるとメッセージが複数挿入される不具合
#ifdef UPDATE_20240112 // MIMEパートが添付のみだと分離されない
#ifdef UPDATE_20240113 // MIMEパート無しの添付メールだと分離されない
						((!bInsert && (bAttachOnly || bDTXT || strstr(pContext->mtkn2, "text/plain") || strstr(pContext->mtkn2, "text/html"))) || (mALTBoundary[0] && (strstr(pContext->mtkn2, "multipart/related") || strstr(pContext->mtkn2, "multipart/alternative"))))
#else
						((!bInsert && (bDTXT || strstr(pContext->mtkn2, "text/plain") || strstr(pContext->mtkn2, "text/html"))) || (mALTBoundary[0] && (strstr(pContext->mtkn2, "multipart/related") || strstr(pContext->mtkn2, "multipart/alternative"))))
#endif
#else
						((!bInsert && (strstr(pContext->mtkn2, "text/plain") || strstr(pContext->mtkn2, "text/html"))) || (mALTBoundary[0] && (strstr(pContext->mtkn2, "multipart/related") || strstr(pContext->mtkn2, "multipart/alternative"))))
#endif
#else
						(strstr(pContext->mtkn2, "text/plain") || strstr(pContext->mtkn2, "text/html") || (mALTBoundary[0] && (strstr(pContext->mtkn2, "multipart/related") || strstr(pContext->mtkn2, "multipart/alternative"))))
#endif
#else
						(strstr(pContext->mtkn2, "text/plain") || strstr(pContext->mtkn2, "text/html") || (mALTBoundary[0] && strstr(pContext->mtkn2, "multipart/alternative")))
#endif
#else
						(strstr(pContext->mtkn2, "text/plain") || strstr(pContext->mtkn2, "text/html"))
#endif
					    )
#else
					if (bFirst)
#endif
				  // メッセージの挿入
				  //ListsAddMessage(pContext->Tempfp, pContext->nMsgId, mListDir);
					{
					   int nUrl;
					   FILE *fpMes;
					   CHAR mAMFn[256], mAMDIR[256], mAM[256], mAMURL[256];
					   CHAR mAMkey[512]; //MAX_REG_RCPT];

					   sprintf(mAMkey,"%s\\%s", SOFT_LISTS_REG, &mRCPTO[11]);
				       GetProfileStringEx(mAMkey,"ListSaveFolder", "", mAMDIR, sizeof(mAMDIR)-1);
				       GetProfileStringEx(mAMkey,"ListSaveURL", "", mAMURL, sizeof(mAMURL)-1);
					   bListSaveOrign = GetProfileIntEx(mAMkey, "ListSaveOrign", (INT)FALSE);

                       //GetProfileStringEx(mkey,"ListSaveExtension", "TXT", mExte, sizeof(mExte)-1);
                       //sprintf(mAMFn, "%sB%010lu.%s", mdir, nMLNo, mExte);
#ifdef UPADTE_20210312 // リンク挿入時にMIMEで分割した。
					   if ((mALTBoundary[0] && strlen(&pContext->mdata[2])-4 == strlen(mALTBoundary) && !strncmp(&pContext->mdata[strlen(mALTBoundary)+2], "--", 2)))
					   {
#ifdef UPDATE_20240112 // MIMEパートが添付のみだと分離されない
						 if (!bDTXT)
#endif
						 {
                           fputs(pContext->mdata, pContext->Tempfp);
						   fputs("\n", pContext->Tempfp);
						 }
					   }
#ifdef UPDATE_20210322 // 添付にTEXTやHTMLファイルがあるとメッセージが複数挿入される不具合
						bInsert = TRUE;
#endif
					   ////////////////////
#ifdef UPDATE_20210313 // MLで添付削除指定で分離保存しない設定の時添付がMIME構想が狂う
					   if (bListSaveOrign)
#endif
					   {
#ifdef UPDATE_20240112 // MIMEパートが添付のみだと分離されない
						 if (bDTXT) 
						   bDTXT = FALSE;
                          else
#endif
						 {
                            fprintf(pContext->Tempfp, "--%s\r\n", mBoundary);
						 }
#ifndef UPDATE_20210320 // リンクメッセージをattachmentからinlineに変更すした
                          fprintf(pContext->Tempfp, "Conten-Type: text/plain; charset=\"utf-8\";\r\n\tname=\"url.txt\"\r\n");
						  fprintf(pContext->Tempfp, "Content-Transfer-Encoding: 8bit\r\n");
						  fprintf(pContext->Tempfp, "Content-Disposition: attachment;\r\n\tfilename=\"url.txt\"\r\n\r\n");
#endif
						  {
#ifdef UPDATE_20210427 // 本文のCharsetに合わせてリンク挿入できるようにした。
							if (mcs[0])
							{
							  sprintf(mAMFn, "%smessage%05d.txt", mAMDIR, GetCodePage(mcs, ""));
					          fpMes = fopen(mAMFn, "rb");
							  if (!fpMes)
							  {
					            sprintf(mAMFn, "%smessage.txt", mAMDIR);
					            fpMes = fopen(mAMFn, "rb");
							  }
							} else {
                              sprintf(mAMFn, "%smessage.txt", mAMDIR);
							  fpMes = fopen(mAMFn, "rb");
							}
							if (fpMes)
#else
					        sprintf(mAMFn, "%smessage.txt", mAMDIR);
					        if ((fpMes = fopen(mAMFn, "rb")))
#endif
							{
						      while((nUrl = fread(mAM, sizeof(char), sizeof(mAM), fpMes)))
							  {
						        fwrite(mAM, sizeof(char), nUrl, pContext->Tempfp);
							  }
						      fclose(fpMes);
#ifdef UPDATE_20210320 // リンクメッセージをattachmentからinlineに変更すした
							} else {
                              fprintf(pContext->Tempfp, "Conten-Type: text/plain; charset=\"utf-8\"\r\n");
						      fprintf(pContext->Tempfp, "Content-Transfer-Encoding: 8bit\r\n");
							  fprintf(pContext->Tempfp, "Content-ID: <url.txt>\r\n");
#ifdef UPDATE_20210322 // 添付にTEXTやHTMLファイルがあるとメッセージが複数挿入される不具合
						      fprintf(pContext->Tempfp, "Content-Disposition: inline;\r\n");
                              fprintf(pContext->Tempfp, "	filename=\"=?utf-8?B?5re75LuY44OV44Kh44Kk44Or44Gu44OA44Km44Oz44Ot44O844OJ5YWI44Gv?=\r\n");
                              fprintf(pContext->Tempfp, "	=?utf-8?B?44GT44Gu44OV44Kh44Kk44Or44Gr6KiY6LyJ44GV44KM44Gm44GE44G+44GZ44CCLnR4dA==?=\"\r\n\r\n");
#else
						      fprintf(pContext->Tempfp, "Content-Disposition: inline\r\n\r\n");
#endif								  
#endif
							}
						  }
						  fputs("\r\n", pContext->Tempfp);
						  fprintf(pContext->Tempfp, "%sB%010lu.EML\r\n\r\n", mAMURL, pContext->nMsgId);
					   }
#else
					   fprintf(pContext->Tempfp, "%sB%010lu.EML\n\n", mAMURL, pContext->nMsgId);
#endif
					   if (mALTBoundary[0])
					     bFirst = FALSE;
					}
					if ((mALTBoundary[0] && strlen(&pContext->mdata[2])-4 == strlen(mALTBoundary) && !strncmp(&pContext->mdata[strlen(mALTBoundary)+2], "--", 2)))
					{
                      mALTBoundary[0] = '\x0'; //alternativeパートの終わり
#ifdef UPADTE_20210312 // リンク挿入時にMIMEで分割した。
					  pContext->mdata[0] = '\x0';
#endif
					}

				  if (!bFirst)
#endif
                    fputs(pContext->mdata, pContext->Tempfp);
				  bTEXT = FALSE;
				  do {
#ifdef UPDATE_20170512 // ヘッダ一行の文字数が受信バッファを超えている場合、空欄行が含まれてしまう不具合
	                fgets(pContext->mdata, RFC2822_LINE_LENGTH_LIMIT+1, pContext->Datafp);
		            *(pContext->mdata+RFC2822_LINE_LENGTH_LIMIT+1)='\x0';
#else 	         
			        fgets(pContext->mdata, sizeof(pContext->mdata), pContext->Datafp);
#endif
#ifdef UPDATE_20210427A // 本文のCharsetに合わせてリンク挿入できるようにした。
				    if (!mcs[0])
					{
				      if (pcs = strstr(pContext->mdata, "charset="))
					  {
				        pcs += 8;
				        while(*pcs == ' ' || *pcs == '\t' || *pcs == '"')
  					      pcs++;
				        strcpy(mcs, pcs);
				        strtok(mcs, " ;\"\r\n\t");
				        strlwr(mcs);
					  }
					}
#endif
		            strncpy(pContext->mtkn2, pContext->mdata, 128);
			        pContext->mtkn2[128] = '\x0';
		            strtok(pContext->mtkn2,"\r\n");
			        if (!_strnicmp(pContext->mtkn2, "content-type:", 13)) 
					{
				      strlwr(pContext->mtkn2);
#ifdef UPDATE_20240112 // MIMEパートが添付のみだと分離されない
				      if (!bInsert && !bFirst && !strstr(pContext->mtkn2, "text/") && !strstr(pContext->mtkn2, "multipart/"))
					  {
				        //fprintf(pContext->Tempfp, "Conten-type: text/plain\r\n\r\n");
						//fprintf(pContext->Tempfp, "--%s\r\n", mBoundary);
#ifdef UPDATE_20240116A // MIMEヘッダに、Content-Type:ヘッダが重複してしまう不具合。
                        strcpy(pContext->mdata, "\r\n");
#endif
						fseek(pContext->Datafp, npos-(strlen(mBoundary)+4), SEEK_SET);
                        bDTXT = bFirst = bTEXT = TRUE;
						break;
					  }
#endif
#ifdef UPDATE_20110119 // メーリングリストの添付削除でHTMLメールに添付があると正しく削除されない
#ifdef UPDATE_20210321 // Content-Typeがmultipart/related;の時の処理
#ifdef UPDATE_20210322 // 添付にTEXTやHTMLファイルがあるとメッセージが複数挿入される不具合
					  if (!bInsert &&
					      (strstr(pContext->mtkn2, "text/") || strstr(pContext->mtkn2, "multipart/related") || strstr(pContext->mtkn2, "multipart/alternative") ) && 
#else
					  if ((strstr(pContext->mtkn2, "text/") || strstr(pContext->mtkn2, "multipart/related") || strstr(pContext->mtkn2, "multipart/alternative") ) && 
#endif
#else
					  if ((strstr(pContext->mtkn2, "text/") || strstr(pContext->mtkn2, "multipart/alternative") ) && 
#endif
						  !bFirst) // 最初のパートのみ許可
#else
					  if (strstr(pContext->mtkn2, "text/") && !bFirst) // 最初のTEXTパートのみ許可
#endif
					  {
#ifdef UPADTE_20210312A // Content-Type:に改行されずにboundaryが含まれると正しく添付隔離に失敗した。
                        if (pBoundary = strstr(pContext->mdata, "boundary"))
						{
			              pBoundary += 8;
				          if (!strchr(pBoundary, '"'))  // 区切りがないなら
						  {
				            pBoundary++;
						  }
						  else
						  {
			                while(*pBoundary!='"') // トークン開始カラムを探す
			                  pBoundary++;
			                pBoundary++;          // 見つけた次のカラムからトークンを取り出す。
						  }
			              strcpy(mALTBoundary, pBoundary);
			              strtok(mALTBoundary, "\"\r\n");
						}
#endif
					    bFirst = bTEXT = TRUE;
					  } else { // 添付の場合
#ifndef UPDATE_20210314 // MLで添付削除後に続く同報先がMLで添付削除指定があると、MIME構想が狂う
#ifdef UPDATE_20210304 // メーリングリストで投稿内容保存ファイルの原本を保存するオプションを追加した。
						if (!bListSaveOrign)
#endif
                          fputs(pContext->mdata, pContext->Tempfp);
#endif
					    fputs("\r\n", pContext->Tempfp);
					  }
				      break;
#ifdef UPDATE_20210304 // メーリングリストで投稿内容保存ファイルの原本を保存するオプションを追加した。
#ifdef UPDATE_20211229 // 添付分離で'Content-Type'ヘッダより先に'Content-Transfer-Encoding'ヘッダがあると'Content-Transfer-Encoding'ヘッダが無視される不具合。
					} else if (!bFirst && !_strnicmp(pContext->mtkn2, "content-transfer-encoding:", 26)) {
#ifdef UPDATE_20240114 // MIMEパート無しの添付メールで添付分離で'Content-Type'ヘッダより先に'Content-Transfer-Encoding'ヘッダがあると2重定義してしまう不具合
						{
						  BOOL bSet2 = TRUE;
						  CHAR  mdata2[1024];
						  unsigned long nops2 = ftell(pContext->Datafp);

						  while(fgets(mdata2, sizeof(mdata2)-1, pContext->Datafp))
						  {
							if (!strcmp(mdata2, "\r\n"))
							  break;
							strlwr(mdata2);
							if (strstr(mdata2, "content-type:") && !strstr(mdata2, "text/"))
							{
							  bSet2 = FALSE;
							  break;
							}
						  }
						  fseek(pContext->Datafp, nops2, SEEK_SET);
						  if (bSet2)
							fputs(pContext->mdata, pContext->Tempfp);
						}
#else
					  fputs(pContext->mdata, pContext->Tempfp);
#endif
#endif

#else
					} else {
                      fputs(pContext->mdata, pContext->Tempfp);
#endif
					}
				  } while(!feof(pContext->Datafp));
				}
			  }
			}
#endif
		  }

#ifdef UPDATE_20060306 // ヘッダ区切りのない本文への対応
		  if (bBODY) {    // 空白改行挿入
		    bBODY = FALSE;
            fputs("\r\n", pContext->Tempfp);
		  }
#endif

#ifdef UPDATE_20240116 // メール本体の文字エンコードがbase64の場合の添付分離時のメールに、Content-Transfer-Encoding:ヘッダが重複してしまう不具合。
		  if (nCEnc == 1)
			nCEnc++;
		  else 
		  {
#endif
#ifdef UPADTE_20031120
#ifdef UPDATE_20240112 // MIMEパートが添付のみだと分離されない
#ifdef UPDATE_20240116 // メール本体の文字エンコードがbase64の場合の添付分離時のメールに、Content-Type:ヘッダが重複してしまう不具合。
		    if (!bSub && bTEXT && !(bDTXT && (bBODY ||(!bBODY && pContext->mdata[0] == '\r' || pContext->mdata[0] =='\n'))))
#else
		    if (!bSub && bTEXT && !bDTXT)
#endif
#else
		    if (!bSub && bTEXT)
#endif
              fputs(pContext->mdata, pContext->Tempfp);
#else
		    if (!bSub)
              fputs(pContext->mdata, pContext->Tempfp);
#endif
            pContext->p = strstr(pContext->mdata,"\r\n");
#ifdef UPDATE_20240116 // メール本体の文字エンコードがbase64の場合の添付分離時のメールに、Content-Transfer-Encoding:ヘッダが重複してしまう不具合。
		  }
#endif
		  if (!pContext->p) {
		    if (*pContext->mdata == '\r' && *(pContext->mdata+1) == '\n')
              pContext->p = pContext->mdata;
		  }
		  if (!pContext->p && !feof(pContext->Datafp)) { // １行がバッファ長の2倍以上の場合の処理
		    //// 改行がでるまで読込み
	        do {
#ifdef UPDATE_20170512 // ヘッダ一行の文字数が受信バッファを超えている場合、空欄行が含まれてしまう不具合
	          fgets(pContext->mdata, RFC2822_LINE_LENGTH_LIMIT+1, pContext->Datafp);
		      *(pContext->mdata+RFC2822_LINE_LENGTH_LIMIT+1)='\x0';
#else
	          fgets(pContext->mdata, sizeof(pContext->mdata), pContext->Datafp);
#endif
		      //// 改行コードが<CR>のみ又は<CR><CR><LF>のデータは<CR><LF>に置換え
		      if ((pCrLf = strstr(pContext->mdata, "\r\r\n"))) {
		        *pCrLf = '\x0';
			    strcat(pCrLf, "\r\n");
			  } else {
		        pCrLf = strstr(pContext->mdata, "\r\n");
		        if (!pCrLf) {
			      pCrLf = strstr(pContext->mdata, "\r");
			      if (!pCrLf) {
			        pCrLf = strstr(pContext->mdata, "\n");
			        if (!pCrLf)
			          pCrLf = pContext->mdata;
#ifdef UPDATE_20170512 // ヘッダ一行の文字数が受信バッファを超えている場合、空欄行が含まれてしまう不具合
				  } else {
				    if ((nTerm = fgetc(pContext->Datafp)) != '\n')
					{
				      fseek(pContext->Datafp, -1L, SEEK_CUR);
					}
#endif
				  }
#ifdef UPDATE_20170512 // ヘッダ一行の文字数が受信バッファを超えている場合、空欄行が含まれてしまう不具合
                  if (pCrLf != pContext->mdata || pCrLf == pContext->mdata && strlen(pContext->mdata) < RFC2822_LINE_LENGTH_LIMIT)
#endif
				  {
		            *pCrLf = '\x0';
		             strcat(pCrLf, "\r\n");
				  }
				}
			  }
		      //////////////////////////////////////////////
	          fputs(pContext->mdata, pContext->Tempfp);
              pContext->p = strstr(pContext->mdata,"\r\n");
			  if (!pContext->p) {
		        if (*pContext->mdata == '\r' && *(pContext->mdata+1) == '\n')
#ifdef UPDATE_20170512 // ヘッダ一行の文字数が受信バッファを超えている場合、空欄行が含まれてしまう不具合
                  if (strlen(pContext->mdata) < sizeof(pContext->mdata)-RFC2822_LINE_LENGTH_LIMIT)
#endif
                     pContext->p = pContext->mdata;
			  }
			} while(!pContext->p && !feof(pContext->Datafp));
		  }
#ifdef UPDATE_20170512 // ヘッダ一行の文字数が受信バッファを超えている場合、空欄行が含まれてしまう不具合
	      fgets(pContext->mdata, RFC2822_LINE_LENGTH_LIMIT+1, pContext->Datafp);
		  *(pContext->mdata+RFC2822_LINE_LENGTH_LIMIT+1)='\x0';
#else
  	      fgets(pContext->mdata, sizeof(pContext->mdata), pContext->Datafp);
#endif
//printf("fgets\n%s\n", pContext->mdata);
		}
	    fclose(pContext->Tempfp);
#ifdef UPDATE_20240117A // メーリングリスト＆添付分離メールへのドメイン認証ヘッダ挿入対策
#ifdef DOMAIN_AUTH_DKIM    // 2013.01.17 ドメイン認証 DomainkeysとDKIM方式 追加
#ifdef UPDATE_20240118 // DKIM設定箇所の選択 bit0:(受信時付加)デフォルト bit1:上長承認メールへ付加 bit2:添付分離メールへ付加
#ifdef UPDATE_20240127 // メーリングリスト題名付加時メールへのドメイン認証ヘッダ挿入対策
	   if ((bDKIM & 0x4) && ((bML && bDelAttch) || (pContext->bMList && mMLWord[0])))
#else
	   if ((bDKIM & 0x4) && bML && bDelAttch)
#endif
#else
	   if (bDKIM && bML && bDelAttch)
#endif
	   {
#ifdef UPDATE_20240122 // SPF/DKIM/DMARC/ARC実行パスを半角スペースを含むロングネームでも指定可能にする対策
	     char mCmdl[256];

	     sprintf(mCmdl, "\"%s\"", mDomainAUTHDKIM);
#ifdef UPDATE_20240129 // 承認依頼メールにDKIMサインが追加されない
	     sprintf(mArgv, "\"postmaster@%s\"", pContext->LocalName);
#else
	     sprintf(mArgv, "\"%s\"", pContext->mUIDFROM);
#endif
         _spawnl(_P_WAIT, mDomainAUTHDKIM, mCmdl, pContext->PeerAddr, mArgv, (bMailApprovalMove ? mApprtmpEML : pContext->mFnTemp), pContext->LocalName, NULL);
#else
	     sprintf(mArgv, "\"%s\"", pContext->mUIDFROM);
         _spawnl(_P_WAIT, mDomainAUTHDKIM, mDomainAUTHDKIM, pContext->PeerAddr, mArgv, (bMailApprovalMove ? mApprtmpEML : pContext->mFnTemp), pContext->LocalName, NULL);
#endif
	   }
#endif
#endif
	  }
	  fclose(pContext->Datafp);
	}
#ifdef UPDATE_20070202 // pContext->Datafpのオープン失敗を回復
    if (pContext->Tempfp) { // fopenが成功しているときのみ処理する
#ifdef UPDATE_20210403 // 上長承認が有効だと添付ファイル分離機能が無効状態にならないようにした。
       while (!(pContext->Tempfp = fopen((bMailApprovalMove ? mApprtmpEML : pContext->mFnTemp), "rt"))) 
#else
	   while (!(pContext->Tempfp = fopen(pContext->mFnTemp, "rt")))
#endif
	   {
#ifdef UPDATE_20210403_20210701 // 上長承認が有効だと添付ファイル分離機能が無効状態にならないようにした。
	     sprintf(mLog, "Don't finised to fclose(%s).", (bMailApprovalMove ? mApprtmpEML : pContext->mFnTemp));
#else
         sprintf(mLog, "Don't finised to fclose(%s).", pContext->mFnTemp);
#endif
         LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
         if (bServiceTerminating)
           break;
	     _sleep(100);
	   }
	   fclose(pContext->Tempfp);
	}
#else
#ifdef UPDATE_20050606 // RCPの再作成でロストしないように待つ
	 while (!(pContext->Tempfp = fopen(pContext->mFnTemp, "rt"))) {
       sprintf(mLog, "Don't finised to fclose(%s).", pContext->mFnTemp);
       LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
       if (bServiceTerminating)
         break;
	   _sleep(100);
	 }
	 fclose(pContext->Tempfp);
#endif
#endif
	if (pContext->bMList && bML) { //bList) { // メッセージがメーリングリストへの投稿の時
#ifdef UPDATE_20071204  // メッセージＩＤ採番処理を修正(Bym10id)
      if (ListsSave(&mRCPTO[11], mDBSubject, nMLNo, pContext->mMsgId, mListDir))  // 保存フラグが指定されていればしてフォルダへメッセージ保存
#else
#ifdef UPDATE_20210303 // メーリングリストに付加表題が無いとき題名を再構築できない不具合
#ifdef UPDATE_20210304 // メーリングリストで投稿内容保存ファイルの原本を保存するオプションを追加した。
      if (ListsSave(&mRCPTO[11], mMLWord[0] ? mDBSubject : mDBSubject2, nMLNo, pContext->nMsgId, mListDir, &bListSaveOrign))  // 保存フラグが指定されていればしてフォルダへメッセージ保存
#else
      if (ListsSave(&mRCPTO[11], mMLWord[0] ? mDBSubject : mDBSubject2, nMLNo, pContext->nMsgId, mListDir))  // 保存フラグが指定されていればしてフォルダへメッセージ保存
#endif
#else
      //if (ListsSave(pContext->mUIDRCPT, mDBSubject, pContext->nMsgId, mListDir)) { // 保存フラグが指定されていればしてフォルダへメッセージ保存
      if (ListsSave(&mRCPTO[11], mDBSubject, nMLNo, pContext->nMsgId, mListDir))  // 保存フラグが指定されていればしてフォルダへメッセージ保存
#endif
#endif
	  {
	    if (mListDir[0] != '\x0')
#ifdef UPDATE_20210304 // メーリングリストで投稿内容保存ファイルの原本を保存するオプションを追加した。
		  if (bListSaveOrign)
		  {
#ifdef UPDATE_20210403 // 上長承認が有効だと添付ファイル分離機能が無効状態にならないようにした。
            if (bMailApprovalMove) 
			{
		      CopyFile(mApprEML, mListDir, TRUE);
			  if (mBoundary[0])
			    CopyFile(mApprtmpEML, mApprEML, FALSE);
			  CopyFile(pContext->mFnData, pContext->mFnTemp, FALSE);
			  //DeleteFile(mApprtmpEML);
			}
			else 
#endif
			{
		      CopyFile(pContext->mFnData, mListDir, TRUE);
			  CopyFile(pContext->mFnTemp, pContext->mFnData, FALSE);
#ifdef UPDATE_20210406 //メーリングリストアドレスが複数同報されている場合添付分離後のメッセージを複写する。
			  {
			     BOOL bBody;
				 int na, nz;
				 char *pna;
				 CHAR mTmpFn[256], mTmpFn2[256];
				 CHAR mTmpLine[1024];

				 na = 0;
                 if (pna = strrchr(pContext->mFnTemp, '-'))
				 {
				   na = atoi(pna+1);
				 } 
#ifdef UPDATE_20210722_LOG
          sprintf(mLog, "LINE=2681 / nRCP=%d / bHitaApprovalRev=%d / bHitaFilter=%d", nRCP, bHitaApprovalRev, bHitaFilter);
          LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
#endif
                 for (nz = 1; nz < nRCP; nz++)
				 {
				   if (na != nz)
				   {
					 sprintf(mTmpFn, "%sB%010lu-%03lu.TMP",mTempDir,pContext->nMsgId, nz);
					 sprintf(mTmpFn2, "%sB%010lu-%03lu.TM$",mTempDir,pContext->nMsgId, nz);
		             {
			           FILE *fps, *fpd;
		               if (fpd = fopen(mTmpFn2, "wt")) 
					   {
					     // ヘッダ抽出
		                 if (fps = fopen(mTmpFn, "rt")) 
					     {
						   while(fgets(mTmpLine, sizeof(mTmpLine)-1, fps))
						   {
							 fputs(mTmpLine, fpd);
						     if (mTmpLine[0] == '\r' || mTmpLine[0] == '\n')
							  break;
						   }
                           fclose(fps);
                         }
						 DeleteFile(mTmpFn);
					     // 本文抽出
						 bBody = FALSE;
		                 if (fps = fopen(pContext->mFnTemp, "rt")) 
					     {
						   while(fgets(mTmpLine, sizeof(mTmpLine)-1, fps))
						   {
						     if (bBody)
                               fputs(mTmpLine, fpd);
						     if (!bBody && (mTmpLine[0] == '\r' || mTmpLine[0] == '\n'))
							   bBody = TRUE;
						   }
                           fclose(fps);
                         }
                         fclose(fpd);
                       }
#ifdef UPDATE_20210704 //メーリングリストアドレスが複数同報されている場合添付分離後のメッセージを複写後に不要な".TM$"ファイルファイルが残されないようにする。
		               while (!(fps = fopen(mTmpFn, "rt"))) 
					   {
                          DeleteFile(mTmpFn);
		                  _sleep(0);
                          MoveFileEx(mTmpFn2, mTmpFn, MOVEFILE_COPY_ALLOWED | MOVEFILE_WRITE_THROUGH);
					   }
	                   fclose(fps);
		               while ((fps = fopen(mTmpFn2, "rt"))) { // TM$ファイルが削除されていない場合に補助的に削除
		                 fclose(fps);
		                 DeleteFile(mTmpFn2);
		                 _sleep(100);
					   }
#else
					   MoveFileEx(mTmpFn2, mTmpFn, MOVEFILE_COPY_ALLOWED | MOVEFILE_WRITE_THROUGH);
#endif
			         }
				     //CopyFile(pContext->mFnTemp, mTmpFn, FALSE);
				   }
				 }
			  }
#endif
			}

			sprintf(pContext->mFnTemp, "%sB%010lu.TMP",mTempDir,pContext->nMsgId);
			CopyFile(pContext->mFnData, pContext->mFnTemp, FALSE);
#ifdef UPDATE_20210314 // MLで添付削除後に続く同報先がMLで添付削除指定があると、MIME構想が狂う
            bSkipDelAttach = TRUE;
#endif
          } else
#endif
#ifdef UPDATE_20210706 // 上長承認フォルダに作成した"$SG"ファイルが削除されないようにした。
		    CopyFile((bMailApprovalMove ? mApprtmpEML : pContext->mFnTemp), mListDir, TRUE);
#else
		    CopyFile(pContext->mFnTemp, mListDir, TRUE);
#endif
#ifndef UPDATE_20210707 // 上長承認フォルダに作成した"$SG"ファイルが削除されないことがあった。
#ifdef UPDATE_20210403 // 上長承認が有効だと添付ファイル分離機能が無効状態にならないようにした。
#ifdef UPDATE_20210706 // 上長承認フォルダに作成した"$SG"ファイルが削除されないようにした。
         if (mApprtmpEML[0])
         {
	       while(!DeleteFile(mApprtmpEML)) 
		   {
	         if (GetLastError() == ERROR_FILE_NOT_FOUND)
		       break;
	         _sleep(100);
		   }
		 }
#else
	      DeleteFile(mApprtmpEML);
#endif
#endif
#endif
	  }
	}
    /////////////////////////////////////
	    mRCPTO[0] = '\x0';
	    fgets(mRCPTO, sizeof(mRCPTO), pContext->RCPfp); // Recipient-1
	  }
      fclose(pContext->RCPfp);
#ifdef UPDATE_20040720_LOG
      sprintf(mLog, "fclose(%s)", pContext->mFnHead);
      LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
#endif
#ifdef UPDATE_20210707 // 上長承認フォルダに作成した"$SG"ファイルが削除されないことがあった。
      if (mApprtmpEML[0])
      {
        while(!DeleteFile(mApprtmpEML)) 
		{
	      if (GetLastError() == ERROR_FILE_NOT_FOUND)
		    break;
	       _sleep(100);
		}
	  }
#endif
#ifdef UPDATE_20210722_LOG
    sprintf(mLog, "bMailApproval=%d / bMailApprovalRevers=%d / pContext->mBOSS[0]=%c / bDeleApprovalMSG=%d", bMailApproval,  bMailApprovalRevers, pContext->mBOSS[0], bDeleApprovalMSG);
    LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
#endif

#ifdef UPDATE_20070613 // 上長承認機能追加 BXXXXXXXXX.MS$ファイル削除
#ifdef UPDATE_20161112 //題名ヘッダでのブラック＆ホワイトリストテーブル
    if ((bMailApproval || bMailApprovalRevers) && !pContext->mBOSS[0] && bDeleApprovalMSG) 
#else
    if (bMailApproval && !pContext->mBOSS[0] && bDeleApprovalMSG)  
#endif
	  { // BOSSがいないとき
        strcpy(mData, pContext->mFnData);
        mData[strlen(mData)-1] = '$';
	    while(!DeleteFile(mData)) {
          nErr = GetLastError();
	      if (nErr == ERROR_FILE_NOT_FOUND)
	        break;
          sprintf(mLog, "DeleteFile(%s) Fail. code=(%d) retry delete.", mData, nErr);
          LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
	      _sleep(100);
		}
	  }
#endif
	}
    /////////////////////////////////////

	if (pContext->bFL) {
#ifndef TUNING1
#ifdef UPDATE_20041224_FILESPEED
	  sprintf(mFnRCP, "%s%stemp_B%010lu.RCX",mMailSpoolDir, mMailQueueDir, pContext->nMsgId);
      sprintf(pContext->mFnTemp, "%s%stemp_B%010lu.TMP",mMailSpoolDir, mMailQueueDir, pContext->nMsgId);
#else
#ifdef UPDATE_20071204  // メッセージＩＤ採番処理を修正(Bym10id)
	  sprintf(mFnRCP, "%s%s.RCX",mTempDir,pContext->mMsgId);
      sprintf(pContext->mFnTemp, "%s%s.TMP",mTempDir,pContext->mMsgId);
#else
	  sprintf(mFnRCP, "%sB%010lu.RCX",mTempDir,pContext->nMsgId);
      sprintf(pContext->mFnTemp, "%sB%010lu.TMP",mTempDir,pContext->nMsgId);
#endif
#endif
#else
      sprintf(mFnRCP, "%s%sB%010lu.RCX",mMailSpoolDir,mMailQueueDir,pContext->nMsgId);
      sprintf(pContext->mFnTemp, "%s%sB%010lu.TM$",mMailSpoolDir,mMailQueueDir,pContext->nMsgId);
#endif
////////////////////////////////////////////////////////////
#ifdef UPDATE_20060627A // ML宛てで同報があるとinlogに記録できない
	          ///// start inlog //////////////////////
#ifdef UPDATE_20260610 // 本文情報がブラックリストにヒットした場合はinlogへの記録はしない
	          if (bLog && !bHitaFilter) // == 0 && !strnicmp(pContext->mMessages,"354 ", 4))
#else
	          if (bLog)
#endif
			  {
#ifdef Y2038_BUG
	           // GetLocalTime(&pContext->lt);
                sprintf(pContext->mdata, "%02d/%s/%04d:%02d:%02d:%02d",pContext->lt.wDay, mMonth[pContext->lt.wMonth-1], pContext->lt.wYear, pContext->lt.wHour, pContext->lt.wMinute, pContext->lt.wSecond);
#else
	           // pContext->lt = *localtime(&pContext->ltime);
                strftime( pContext->mdata, 128, "%d/%b/%Y:%H:%M:%S", &pContext->lt );
#endif
#ifdef UPDATE_20260606 // ログの書き込み信頼性向上対策
                WaitForSingleObject(g_hMutexInLog, INFINITE);  // 排他開始
                fpInLog = OpenCloseLog(fpInLog, mDTInLog, "inlog", mComName, pContext->lt);
                if (fpInLog)
				{
#ifdef K_SEARCH // K_SEARCH OEM 版
		          fprintf(fpInLog, "<%s@%s> [%s] %s %s",
                     pContext->mMsgId,
                     pContext->LocalName,
                     pContext->mdata,
                     pContext->PeerAddr,
                     pContext->PeerHelo);
#else
#ifdef UPDATE_20071204  // メッセージＩＤ採番処理を修正(Bym10id)
		          fprintf(fpInLog, "<%s@%s> [%s] %s %s",
                     pContext->mMsgId,
                     pContext->LocalName,
                     pContext->mdata,
                     pContext->PeerAddr,
                     pContext->PeerHelo);
#else
		          fprintf(fpInLog, "<B%010lu@%s> [%s] %s %s",
                     pContext->nMsgId,
                     pContext->LocalName,
                     pContext->mdata,
                     pContext->PeerAddr,
                     pContext->PeerHelo);
#endif
#endif
                  pContext->Headfp = fopen(pContext->mFnHead, "rt");
		          if (pContext->Headfp) {
		             fgets(pContext->mdata, sizeof(pContext->mdata), pContext->Headfp); // Messages-ID
	                 fgets(pContext->mdata, sizeof(pContext->mdata), pContext->Headfp); // mail from address
		             while(!feof(pContext->Headfp)) {
		               strtok(pContext->mdata,"\r\n");
		               pContext->p = strstr(pContext->mdata," ");
		               if (pContext->p)
		                 fprintf(fpInLog, "%s", pContext->p);
		               fgets(pContext->mdata, sizeof(pContext->mdata), pContext->Headfp); // rcpt to address
					 }
		             fclose(pContext->Headfp);
				  }
                  fprintf(fpInLog, "\n");
#ifdef UPDATE_20060615 // データのディスク書込に異常が有った場合の対策
				  if (fflush(fpInLog) == EOF)
				    if (bVLog)
                      AddToMessageLog(TEXT("inlog write fail."), 102, TEXT(SMTP_SERVICE), EVENTLOG_WARNING_TYPE );
#endif
				}
#else
#ifdef REGTOFILE
      if (nClustering) {
	    sprintf(pContext->mLogFn, "%sinlog\\%s\\%s.log", mMailSpoolDir, mComName, pContext->mdata);
	  } else
#endif
	  {
	    sprintf(pContext->mLogFn, "%sinlog\\%s.log", mMailSpoolDir, pContext->mdata);
	  }
#ifdef Y2038_BUG
                sprintf(pContext->mdata, "%02d/%s/%04d:%02d:%02d:%02d",pContext->lt.wDay, mMonth[pContext->lt.wMonth-1], pContext->lt.wYear, pContext->lt.wHour, pContext->lt.wMinute, pContext->lt.wSecond);
#else
                strftime( pContext->mdata, 128, "%d/%b/%Y:%H:%M:%S", &pContext->lt );
#endif
#ifdef UPDATE_20260603 // ログ出力の排他処理化を追加
                WaitForSingleObject(g_hMutexInLog, INFINITE);  // 排他開始
#endif
	            pContext->Logfp = _fsopen(pContext->mLogFn, "at", _SH_DENYNO);
	            if (pContext->Logfp) 
				{
#ifdef K_SEARCH // K_SEARCH OEM 版
		          fprintf(pContext->Logfp, "<%s@%s> [%s] %s %s",
                     pContext->mMsgId,
                     pContext->LocalName,
                     pContext->mdata,
                     pContext->PeerAddr,
                     pContext->PeerHelo);
#else
#ifdef UPDATE_20071204  // メッセージＩＤ採番処理を修正(Bym10id)
		          fprintf(pContext->Logfp, "<%s@%s> [%s] %s %s",
                     pContext->mMsgId,
                     pContext->LocalName,
                     pContext->mdata,
                     pContext->PeerAddr,
                     pContext->PeerHelo);
#else
		          fprintf(pContext->Logfp, "<B%010lu@%s> [%s] %s %s",
                     pContext->nMsgId,
                     pContext->LocalName,
                     pContext->mdata,
                     pContext->PeerAddr,
                     pContext->PeerHelo);
#endif
#endif
                  pContext->Headfp = fopen(pContext->mFnHead, "rt");
		          if (pContext->Headfp) {
		             fgets(pContext->mdata, sizeof(pContext->mdata), pContext->Headfp); // Messages-ID
	                 fgets(pContext->mdata, sizeof(pContext->mdata), pContext->Headfp); // mail from address
		             while(!feof(pContext->Headfp)) {
		               strtok(pContext->mdata,"\r\n");
		               pContext->p = strstr(pContext->mdata," ");
		               if (pContext->p)
		                 fprintf(pContext->Logfp, "%s", pContext->p);
		               fgets(pContext->mdata, sizeof(pContext->mdata), pContext->Headfp); // rcpt to address
					 }
		             fclose(pContext->Headfp);
				  }
                  fprintf(pContext->Logfp, "\n");
#ifdef UPDATE_20060615 // データのディスク書込に異常が有った場合の対策
				  if (fflush(pContext->Logfp) == EOF)
				    if (bVLog)
                      AddToMessageLog(TEXT("inlog write fail."), 102, TEXT(SMTP_SERVICE), EVENTLOG_WARNING_TYPE );
#endif
		          fclose(pContext->Logfp);
				}
#endif
#ifdef UPDATE_20260603 // ログ出力の排他処理化を追加
                ReleaseMutex(g_hMutexInLog);  // 排他終了
#endif
			  }
	          ///// end inlog //////////////////////
#endif
////////////////////////////////////////////////////////////
		if (bNL || nRCP == 2) 
		{
#ifndef TUNING1
#ifdef UPDATE_20210403 // 上長承認が有効だと添付ファイル分離機能が無効状態にならないようにした。
        //if (!bListSaveOrign && bMailApprovalMove)
	    if (bMailApprovalMove)
		{
		  CopyFile(pContext->mFnHead, mFnRCP, FALSE);
		}
#endif
#ifdef UPDATE_20040721
		while(!DeleteFile(pContext->mFnHead)) {
		  nErr = GetLastError();
		  if (nErr == ERROR_FILE_NOT_FOUND)
			break;
          sprintf(mLog, "DeleteFile(%s) Fail. code=(%d) retry delete.", pContext->mFnHead, nErr);
          LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
		  _sleep(100);
		}
#else
		DeleteFile(pContext->mFnHead);
#endif
#ifdef UPDATE_20210403 // 上長承認が有効だと添付ファイル分離機能が無効状態にならないようにした。
        //if (!bListSaveOrign && bMailApprovalMove)
		if (bMailApprovalMove)
		{
#ifdef UPDATE_20210405 // 添付無しのマルチパート指定のメールだと処理がロックする
		  if (!mBoundary[0] && (nRCP == 2 || !bListSaveOrign) || mBoundary[0])
#else
		  if (!mBoundary[0] && (nRCP == 2 || !bListSaveOrign))
#endif
		  {
			  /*
		    {
		      FILE *fpt;
              if (fpt = fopen(pContext->mFnTemp, "w"))
              {
           	    fflush(fpt);
           	    fclose(fpt);
              }
		    }
			  */
#ifdef UPDATE_20231026 // 上長承認依頼メールの作成に失敗した場合の作成リトライ
            {
			  int i = 0;
              sprintf(mLog, "CopyFile(%s, %s) start.", pContext->mFnData, pContext->mFnTemp);
              LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
		      CopyFile(pContext->mFnData, pContext->mFnTemp, FALSE); // 上書きコピー
	          while(!(fp1 = fopen(pContext->mFnTemp, "rt")))
	          {
                if (bServiceTerminating || i > nMailApprovalMessNum)
	              break;
                i++;
	            _sleep(WAIT_TIME+i);
			    CopyFile(pContext->mFnData, pContext->mFnTemp, FALSE); // 上書きコピー
	          }
			  if (fp1)
			  {
                sprintf(mLog, "CopyFile(%s) success. retry=%d", pContext->mFnTemp, i);
	            fclose(fp1);
			  } else { // ファイル作成できないとき
                sprintf(mLog, "CopyFile(%s) fail. retry=%d, errno=%d", pContext->mFnTemp, i-1, GetLastError());
			  }
              LEVEL_3_ACCEPTLOG(NULL, mLog);
		    }
#else
		    CopyFile(pContext->mFnData, pContext->mFnTemp, FALSE);
#endif
		  } else {
#ifdef UPDATE_20231026 // 上長承認依頼メールの作成に失敗した場合の作成リトライ
            {
              int i = 0;
              sprintf(mLog, "DeleteFile(%s) start.", pContext->mFnTemp);
              LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
		      DeleteFile(pContext->mFnTemp);
	          while((fp1 = fopen(pContext->mFnTemp, "rt")))
	          {
			    fclose(fp1);
                if (bServiceTerminating || i > nMailApprovalMessNum)
	              break;
                i++;
	            _sleep(WAIT_TIME+i);
			     DeleteFile(pContext->mFnTemp);
	          }
              sprintf(mLog, "DeleteFile(%s) success. retry=%d", pContext->mFnTemp, i-1);
              LEVEL_3_ACCEPTLOG(NULL, mLog);
		    }
#else
		    DeleteFile(pContext->mFnTemp);
#endif
		  }
		}
#endif
#ifdef UPDATE_20070105 // TEMPフォルダ内のMSGファイルの削除に失敗してファイルが残ってしまうことがある
#ifdef UPDATE_20231026 // 上長承認依頼メールの作成に失敗した場合の作成リトライ
        sprintf(mLog, "DeleteFile(%s) start.", pContext->mFnData);
        LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
#endif
		while(!DeleteFile(pContext->mFnData)) {
		  nErr = GetLastError();
		  if (nErr == ERROR_FILE_NOT_FOUND)
			break;
          sprintf(mLog, "DeleteFile(%s) Fail. code=(%d) retry delete.", pContext->mFnData, nErr);
          LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
		  _sleep(100);
		}

#ifdef UPDATE_20231026 // 上長承認依頼メールの作成に失敗した場合の作成リトライ
        sprintf(mLog, "DeleteFile(%s) success.", pContext->mFnData);
        LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
#endif
#else
        DeleteFile(pContext->mFnData); // MSGファイルを削除
#endif
#else
        _unlink(pContext->mFnHead);
        _unlink(pContext->mFnData);
#endif
#ifdef UPDATE_20040720_LOG
        sprintf(mLog, "DeleteFile(%s)", pContext->mFnHead);
        LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
        sprintf(mLog, "DeleteFile(%s)", pContext->mFnData);
        LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
#endif
	  }
  	  pContext->hFindFile = FindFirstFile( mFnRCP, &pContext->FindFileData);
	  if (nRCP == 2 && pContext->hFindFile == INVALID_HANDLE_VALUE) {
	    nRCP--;
#ifdef UPDATE_20040720_LOG
        sprintf(mLog, "DeleteFile(%s)", mFnRCP);
        LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
        sprintf(mLog, "DeleteFile(%s)", pContext->mFnTemp);
        LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
#endif
#ifndef TUNING1
	    DeleteFile(mFnRCP);
        DeleteFile(pContext->mFnTemp);
#ifdef UPDATE_20041224_FILESPEED
	    sprintf(mFnRCP, "%s%stemp_B%010lu-%03lu.RCX",mMailSpoolDir,mMailQueueDir,pContext->nMsgId, nRCP);
        sprintf(pContext->mFnTemp, "%s%stemp_B%010lu-%03lu.TMP",mMailSpoolDir,mMailQueueDir,pContext->nMsgId, nRCP);
#else
#ifdef UPDATE_20071204  // メッセージＩＤ採番処理を修正(Bym10id)
	    sprintf(mFnRCP, "%s%s-%03lu.RCP",mTempDir,pContext->mMsgId, nRCP);
        sprintf(pContext->mFnTemp, "%s%s-%03lu.TMP",mTempDir,pContext->mMsgId, nRCP);
#else
	    sprintf(mFnRCP, "%sB%010lu-%03lu.RCP",mTempDir,pContext->nMsgId, nRCP);
        sprintf(pContext->mFnTemp, "%sB%010lu-%03lu.TMP",mTempDir,pContext->nMsgId, nRCP);
#endif
#endif
#else
	    _unlink(mFnRCP);
        _unlink(pContext->mFnTemp);
        sprintf(mFnRCP, "%s%sB%010lu-%03lu.RC$",mMailSpoolDir,mMailQueueDir,pContext->nMsgId, nRCP);
        sprintf(pContext->mFnTemp, "%s%sB%010lu-%03lu.TM$",mMailSpoolDir,mMailQueueDir,pContext->nMsgId, nRCP);
#endif
#ifdef UPDATE_20040720_LOG
        sprintf(mLog, "MoveFileEx(%s, %s)", mFnRCP, pContext->mFnHead);
        LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
        sprintf(mLog, "MoveFileEx(%s, %s)", pContext->mFnTemp, pContext->mFnData);
        LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
#endif
#ifdef UPDATE_20050606 // RCPの再作成でロストしないように待つ
        MoveFileEx(mFnRCP, pContext->mFnHead, MOVEFILE_WRITE_THROUGH);
#ifdef UPDATE_20051027 // RCPの再作成でロストしないように待つ
		while (!(fp1 = fopen(pContext->mFnHead, "rt"))) {
		  _sleep(0);
          MoveFileEx(mFnRCP, pContext->mFnHead, MOVEFILE_WRITE_THROUGH);
		}
	    fclose(fp1);
		while ((fp1 = fopen(mFnRCP, "rt"))) { // TMPファイルが削除されていない場合に補助的に削除
		  fclose(fp1);
		  DeleteFile(mFnRCP);
		  _sleep(100);
		}
#else
	    while ((fp1 = fopen(mFnRCP, "rt"))) {
		  fclose(fp1);
		  _sleep(0);
          MoveFileEx(mFnRCP, pContext->mFnHead, MOVEFILE_WRITE_THROUGH);
		}
#endif
        MoveFileEx( pContext->mFnTemp, pContext->mFnData, MOVEFILE_WRITE_THROUGH);
#ifdef UPDATE_20051018 // RCPの再作成でロストしないように待つ
		while (!(fp1 = fopen(pContext->mFnData, "rt"))) {
		  _sleep(0);
          MoveFileEx( pContext->mFnTemp, pContext->mFnData, MOVEFILE_WRITE_THROUGH);
		}
	    fclose(fp1);
		while ((fp1 = fopen(pContext->mFnTemp, "rt"))) { // TMPファイルが削除されていない場合に補助的に削除
		  fclose(fp1);
		  DeleteFile(pContext->mFnTemp);
		  _sleep(100);
		}
#else
	    while ((fp1 = fopen(pContext->mFnTemp, "rt"))) {
		  fclose(fp1);
		  _sleep(0);
          MoveFileEx( pContext->mFnTemp, pContext->mFnData, MOVEFILE_WRITE_THROUGH);
		}
#endif
#else
        MoveFileEx(mFnRCP, pContext->mFnHead, MOVEFILE_WRITE_THROUGH);
        MoveFileEx( pContext->mFnTemp, pContext->mFnData, MOVEFILE_WRITE_THROUGH);
#endif
	  } else {
#ifdef UPDATE_20190410 // 複数ML宛のみの同報メールで処理がロックする
        if (pContext->hFindFile != INVALID_HANDLE_VALUE)
#endif
		  FindClose(pContext->hFindFile);
        //MoveFile( pContext->mFnTemp, pContext->mFnData );
#ifdef UPDATE_20040720_LOG
        sprintf(mLog, "MoveFileEx(%s, %s)", mFnRCP, pContext->mFnHead);
        LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
        sprintf(mLog, "MoveFileEx(%s, %s)", pContext->mFnTemp, pContext->mFnData);
        LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
#endif
#ifdef UPDATE_20050606 // RCPの再作成でロストしないように待つ
#ifdef UPDATE_20210714 // セキュアハンドラ処理で承認依頼メール用のファイルが作成に失敗することがある不具合。
        while(!MoveFileEx(mFnRCP, pContext->mFnHead, MOVEFILE_COPY_ALLOWED | MOVEFILE_WRITE_THROUGH)) 
		{
		  nErr = GetLastError();
	      if (bServiceTerminating || nErr == ERROR_FILE_NOT_FOUND)
	        break;
          _sleep(0);
		}
#else
        MoveFileEx(mFnRCP, pContext->mFnHead, MOVEFILE_WRITE_THROUGH);
#endif
#ifdef UPDATE_20051027 // RCPの再作成でロストしないように待つ
#ifndef UPDATE_20210714 // セキュアハンドラ処理で承認依頼メール用のファイルが作成に失敗することがある不具合。
		while (!(fp1 = fopen(pContext->mFnHead, "rt"))) {
		  _sleep(0);
#ifdef UPDATE_20060106 // 作成完了しているがファイル記録が削除されないのタイミングで移動待ちにならないための処理
		  MoveFile(mFnRCP, pContext->mFnHead);
#else
          MoveFileEx(mFnRCP, pContext->mFnHead, MOVEFILE_WRITE_THROUGH);
#endif
		}
	    fclose(fp1);
#endif
		while ((fp1 = fopen(mFnRCP, "rt"))) { // TMPファイルが削除されていない場合に補助的に削除
		  fclose(fp1);
		  DeleteFile(mFnRCP);
		  _sleep(100);
		}
#else
	    while ((fp1 = fopen(mFnRCP, "rt"))) {
		  fclose(fp1);
		  _sleep(0);
          MoveFileEx(mFnRCP, pContext->mFnHead, MOVEFILE_WRITE_THROUGH);
		}
#endif
#ifdef UPDATE_20210722_LOG | UPDATE_20231026 // 上長承認依頼メールの作成に失敗した場合の作成リトライ
    sprintf(mLog, "bListSaveOrign=%d / bMailApprovalMove=%d", bListSaveOrign, bMailApprovalMove);
    LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
#endif
#ifdef UPDATE_20210403 // 上長承認が有効だと添付ファイル分離機能が無効状態にならないようにした。
        if (!bListSaveOrign || !bMailApprovalMove)
#endif
        {
#ifdef UPDATE_20231026 // 上長承認依頼メールの作成に失敗した場合の作成リトライ
          sprintf(mLog, "CopyFile(%s, %s) start.", pContext->mFnTemp, pContext->mFnData);
          LEVEL_3_ACCEPTLOG(NULL, mLog);
          //MoveFileEx( pContext->mFnTemp, pContext->mFnData, MOVEFILE_COPY_ALLOWED | MOVEFILE_WRITE_THROUGH);
		  ///////////////////////////////////////////
		  /*
          DeleteFile(pContext->mFnData);
		  {
		    FILE *fpt;
            if (fpt = fopen(pContext->mFnData, "w"))
            {
              fflush(fpt);
              fclose(fpt);
            }
		  }
		  */
if (bDebug) printf("CopyFile(%s, %s);\n", pContext->mFnTemp, pContext->mFnData);
          {
            int i = 0;
		    CopyFile( pContext->mFnTemp, pContext->mFnData,  FALSE); // 上書きコピー
	        while(!(fp1 = fopen(pContext->mFnData, "rt")))
	        {
              if (bServiceTerminating || i > nMailApprovalMessNum)
	            break;
              i++;
	          _sleep(WAIT_TIME+i);
			  CopyFile( pContext->mFnTemp, pContext->mFnData, FALSE);
	        }
			if (fp1)
			{
              sprintf(mLog, "CopyFile(%s) success. retry=%d", pContext->mFnData, i);
	          fclose(fp1);
			} else { // ファイル作成できないとき
              sprintf(mLog, "CopyFile(%s) fail. retry=%d, errno=%d", pContext->mFnData, i-1, GetLastError());
			}
            LEVEL_3_ACCEPTLOG(NULL, mLog);
		  }
if (bDebug) printf("DeleteFile(%s);\n", pContext->mFnTemp);
          {
            int i = 0;
		    DeleteFile(pContext->mFnTemp);
	        while((fp1 = fopen(pContext->mFnTemp, "rt")))
	        {
			  fclose(fp1);
              if (bServiceTerminating || i > nMailApprovalMessNum)
	            break;
              i++;
	          _sleep(WAIT_TIME+i);
			   DeleteFile(pContext->mFnTemp);
	        }
            sprintf(mLog, "DeleteFile(%s) success. retry=%d", pContext->mFnTemp, i-1);
            LEVEL_3_ACCEPTLOG(NULL, mLog);
		  }
		  ///////////////////////////////////////////
#else
          MoveFileEx( pContext->mFnTemp, pContext->mFnData, MOVEFILE_WRITE_THROUGH);

#ifdef UPDATE_20051018 // RCPの再作成でロストしないように待つ
		  while (!(fp1 = fopen(pContext->mFnData, "rt"))) {
#ifdef UPDATE_20231026 // 上長承認依頼メールの作成に失敗した場合の作成リトライ
		    if (fp1 = fopen(pContext->mFnTemp, "rt"))

			{
			  fclose(fp1);
			}
	        if (bServiceTerminating || !fp1)
	           break;
#endif
		    _sleep(0);
#ifdef UPDATE_20060106 // 作成完了しているがファイル記録が削除されないのタイミングで移動待ちにならないための処理
		    MoveFile(pContext->mFnTemp, pContext->mFnData);
#else
#ifdef UPDATE_20231026 // 上長承認依頼メールの作成に失敗した場合の作成リトライ
            MoveFileEx( pContext->mFnTemp, pContext->mFnData, MOVEFILE_COPY_ALLOWED | MOVEFILE_WRITE_THROUGH);
#else
            MoveFileEx( pContext->mFnTemp, pContext->mFnData, MOVEFILE_WRITE_THROUGH);
#endif
#endif
		  }
	      fclose(fp1);
#ifdef UPDATE_20231026 // 上長承認依頼メールの作成に失敗した場合の作成リトライ
          sprintf(mLog, "AccessCheck(%s) ok.", pContext->mFnData);
          LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
		  while ((fp1 = fopen(pContext->mFnTemp, "rt"))) { // TMPファイルが削除されていない場合に補助的に削除
		    fclose(fp1);
		    DeleteFile(pContext->mFnTemp);
		    _sleep(100);
		  }
#else
	      while ((fp1 = fopen(pContext->mFnTemp, "rt"))) {
		    fclose(fp1);
		    _sleep(0);
            MoveFileEx( pContext->mFnTemp, pContext->mFnData, MOVEFILE_WRITE_THROUGH);
		  }
#endif
#endif
#else
          MoveFileEx(mFnRCP, pContext->mFnHead, MOVEFILE_WRITE_THROUGH);
          MoveFileEx( pContext->mFnTemp, pContext->mFnData, MOVEFILE_WRITE_THROUGH);
#endif

		}
	  }
	}
	///// moved incoming folder //////
#ifdef K_SEARCH // K_SEARCH OEM 版
    sprintf(pContext->mFnMSG, "%s%s%s@%s.EML",mMailSpoolDir,mMailQueueSubDir,pContext->mMsgId, pContext->LocalName);
    sprintf(pContext->mFnRCP, "%s%s%s@%s.RCP",mMailSpoolDir,mMailQueueSubDir,pContext->mMsgId, pContext->LocalName);
#else
#ifdef UPDATE_20071204  // メッセージＩＤ採番処理を修正(Bym10id)
#ifdef  INCOM_SUB_FOLDER // incoming フォルダをスレッド別フォルダにして処理
    if (bIncomingSubFolder) // incomingフォルダにスレッド別フォルダを作成 TRUE:する FALSE:しない
	{
    sprintf(pContext->mFnMSG, "%s%s%05d\\%s.MSG",mMailSpoolDir,mMailQueueDir,(pContext->nMsgId%nMaxThread), pContext->mMsgId);
    sprintf(pContext->mFnRCP, "%s%s%05d\\%s.RCP",mMailSpoolDir,mMailQueueDir,(pContext->nMsgId%nMaxThread), pContext->mMsgId);
	} else 
#endif
	{
    sprintf(pContext->mFnMSG, "%s%s%s.MSG",mMailSpoolDir,mMailQueueDir,pContext->mMsgId);
    sprintf(pContext->mFnRCP, "%s%s%s.RCP",mMailSpoolDir,mMailQueueDir,pContext->mMsgId);
	}
#else
#ifdef  INCOM_SUB_FOLDER // incoming フォルダをスレッド別フォルダにして処理
    if (bIncomingSubFolder) // incomingフォルダにスレッド別フォルダを作成 TRUE:する FALSE:しない
	{
    sprintf(pContext->mFnMSG, "%s%s%05d\\B%010lu.MSG",mMailSpoolDir,mMailQueueDir,(pContext->nMsgId%nMaxThread), pContext->nMsgId);
    sprintf(pContext->mFnRCP, "%s%s%05d\\B%010lu.RCP",mMailSpoolDir,mMailQueueDir,(pContext->nMsgId%nMaxThread), pContext->nMsgId);
	} else 
#endif
	{
    sprintf(pContext->mFnMSG, "%s%sB%010lu.MSG",mMailSpoolDir,mMailQueueDir,pContext->nMsgId);
    sprintf(pContext->mFnRCP, "%s%sB%010lu.RCP",mMailSpoolDir,mMailQueueDir,pContext->nMsgId);
	}
#endif
#endif

#ifndef TUNING1
    DeleteFile(pContext->mFnMSG);
    DeleteFile(pContext->mFnRCP);
#else
    _unlink(pContext->mFnMSG);
    _unlink(pContext->mFnRCP);
#endif
#ifdef UPDATE_20040720_LOG
    sprintf(mLog, "DeleteFile(%s)", pContext->mFnMSG);
    LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
    sprintf(mLog, "DeleteFile(%s)", pContext->mFnRCP);
    LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
#endif

////// ここまでで１０００通２分
//#ifdef SPEED_TEST
//pContext->bAcptData = TRUE;
//sprintf(pContext->mMessages, SMTP_GOOD_MESS, "Message received");
//return TRUE;
//#endif

#ifdef UPDATE_20231026 // 上長承認依頼メールの作成に失敗した場合の作成リトライ
    sprintf(mLog, "Check Received heder(%s) limit=%d num=%d", pContext->mFnData, nReceived, pContext->nReceived);
    LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
	if (pContext->nReceived <= nReceived) {
#ifdef UPDATE_20231026 // 上長承認依頼メールの作成に失敗した場合の作成リトライ
      sprintf(mLog, "Check (%s) MailInMaxSize=%d", pContext->mFnData, nMailInMaxSize);
      LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
      if (nMailInMaxSize != 0 ) {
#ifdef UPDATE_20210725 // セキュアハンドラ処理でブラックリスト対象且つ添付分離機能が有効時かつ1メールのサイズ上限が設定されているとメール作業フォルダ内の".TMP"ファイルが削除されないことがあった。
        pContext->hFindFile = FindFirstFile( (bMailApprovalMove ? mApprEML : pContext->mFnData), &pContext->FindFileData);
#else
        pContext->hFindFile = FindFirstFile( pContext->mFnData, &pContext->FindFileData);
#endif
        if (pContext->hFindFile != INVALID_HANDLE_VALUE) {
	 	  //if (nMailInMaxSize >= (pContext->FindFileData.nFileSizeHigh * (MAXDWORD+1)) + pContext->FindFileData.nFileSizeLow) {
          if (nMailInMaxSize >= pContext->FindFileData.nFileSizeLow) {
#ifdef UPADTE_20040428
			if (!bHitaFilter) // フィルタにヒットしたものがない時のみ処理
#else
#ifdef V3
		    if (Filter(pContext, &mRCPTO[11]) && Check_Of_MIME(pContext))
#else
		    if (Filter(pContext, &mRCPTO[11]))
#endif
#endif
			{
#ifndef UPDATE_20060627A // ML宛てで同報があるとinlogに記録できない
	          ///// start inlog //////////////////////
#ifdef UPDATE_20260610 // 本文情報がブラックリストにヒットした場合はinlogへの記録はしない
	          if (bLog && !bHitaFilter) // == 0 && !strnicmp(pContext->mMessages,""354 ", 4))
#else
	          if (bLog)
#endif
			  {
#ifdef Y2038_BUG
	           // GetLocalTime(&pContext->lt);
                sprintf(pContext->mdata, "%02d/%s/%04d:%02d:%02d:%02d",pContext->lt.wDay, mMonth[pContext->lt.wMonth-1], pContext->lt.wYear, pContext->lt.wHour, pContext->lt.wMinute, pContext->lt.wSecond);
#else
	           // pContext->lt = *localtime(&pContext->ltime);
                strftime( pContext->mdata, 128, "%d/%b/%Y:%H:%M:%S", &pContext->lt );
#endif
#ifdef UPDATE_20260606 // ログの書き込み信頼性向上対策
                WaitForSingleObject(g_hMutexInLog, INFINITE);  // 排他開始
                fpInLog = OpenCloseLog(fpInLog, mDTInLog, "inlog", mComName, pContext->lt);
	            if (fpInLog) 
				{
#ifdef K_SEARCH // K_SEARCH OEM 版
		          fprintf(fpInLog, "<%s@%s> [%s] %s %s",
                     pContext->mMsgId,
                     pContext->LocalName,
                     pContext->mdata,
                     pContext->PeerAddr,
                     pContext->PeerHelo);
#else
#ifdef UPDATE_20071204  // メッセージＩＤ採番処理を修正(Bym10id)
		          fprintf(fpInLog, "<%s@%s> [%s] %s %s",
                     pContext->mMsgId,
                     pContext->LocalName,
                     pContext->mdata,
                     pContext->PeerAddr,
                     pContext->PeerHelo);
#else
		          fprintf(fpInLog, "<B%010lu@%s> [%s] %s %s",
                     pContext->nMsgId,
                     pContext->LocalName,
                     pContext->mdata,
                     pContext->PeerAddr,
                     pContext->PeerHelo);
#endif
#endif
                  pContext->Headfp = fopen(pContext->mFnHead, "rt");
		          if (pContext->Headfp) {
		             fgets(pContext->mdata, sizeof(pContext->mdata), pContext->Headfp); // Messages-ID
	                 fgets(pContext->mdata, sizeof(pContext->mdata), pContext->Headfp); // mail from address
		             while(!feof(pContext->Headfp)) {
		               strtok(pContext->mdata,"\r\n");
		               pContext->p = strstr(pContext->mdata," ");
		               if (pContext->p)
		                 fprintf(fpInLog, "%s", pContext->p);
		               fgets(pContext->mdata, sizeof(pContext->mdata), pContext->Headfp); // rcpt to address
					 }
		             fclose(pContext->Headfp);
				  }
		          fprintf(fpInLog, "\n");
#ifdef UPDATE_20060615 // データのディスク書込に異常が有った場合の対策
				  if (fflush(fpInLog) == EOF)
				    if (bVLog)
                      AddToMessageLog(TEXT("inlog write fail."), 103, TEXT(SMTP_SERVICE), EVENTLOG_WARNING_TYPE );
#endif
				}
#else
#ifdef REGTOFILE
      if (nClustering) {
	    sprintf(pContext->mLogFn, "%sinlog\\%s\\%s.log", mMailSpoolDir, mComName, pContext->mdata);
	  } else 
#endif
	  {
        sprintf(pContext->mLogFn, "%sinlog\\%s.log", mMailSpoolDir, pContext->mdata);
	  }
#ifdef Y2038_BUG
                sprintf(pContext->mdata, "%02d/%s/%04d:%02d:%02d:%02d",pContext->lt.wDay, mMonth[pContext->lt.wMonth-1], pContext->lt.wYear, pContext->lt.wHour, pContext->lt.wMinute, pContext->lt.wSecond);
#else
                strftime( pContext->mdata, 128, "%d/%b/%Y:%H:%M:%S", &pContext->lt );
#endif
#ifdef UPDATE_20260603 // ログ出力の排他処理化を追加
                WaitForSingleObject(g_hMutexInLog, INFINITE);  // 排他開始
#endif
	            pContext->Logfp = _fsopen(pContext->mLogFn, "at", _SH_DENYNO);
	            if (pContext->Logfp) 
				{
#ifdef K_SEARCH // K_SEARCH OEM 版
		          fprintf(pContext->Logfp, "<%s@%s> [%s] %s %s",
                     pContext->mMsgId,
                     pContext->LocalName,
                     pContext->mdata,
                     pContext->PeerAddr,
                     pContext->PeerHelo);
#else
#ifdef UPDATE_20071204  // メッセージＩＤ採番処理を修正(Bym10id)
		          fprintf(pContext->Logfp, "<%s@%s> [%s] %s %s",
                     pContext->mMsgId,
                     pContext->LocalName,
                     pContext->mdata,
                     pContext->PeerAddr,
                     pContext->PeerHelo);
#else
		          fprintf(pContext->Logfp, "<B%010lu@%s> [%s] %s %s",
                     pContext->nMsgId,
                     pContext->LocalName,
                     pContext->mdata,
                     pContext->PeerAddr,
                     pContext->PeerHelo);
#endif
#endif
                  pContext->Headfp = fopen(pContext->mFnHead, "rt");
		          if (pContext->Headfp) {
		             fgets(pContext->mdata, sizeof(pContext->mdata), pContext->Headfp); // Messages-ID
	                 fgets(pContext->mdata, sizeof(pContext->mdata), pContext->Headfp); // mail from address
		             while(!feof(pContext->Headfp)) {
		               strtok(pContext->mdata,"\r\n");
		               pContext->p = strstr(pContext->mdata," ");
		               if (pContext->p)
		                 fprintf(pContext->Logfp, "%s", pContext->p);
		               fgets(pContext->mdata, sizeof(pContext->mdata), pContext->Headfp); // rcpt to address
					 }
		             fclose(pContext->Headfp);
				  }
                  fprintf(pContext->Logfp, "\n");
#ifdef UPDATE_20060615 // データのディスク書込に異常が有った場合の対策
				  if (fflush(pContext->Logfp) == EOF)
				    if (bVLog)
                      AddToMessageLog(TEXT("inlog write fail."), 103, TEXT(SMTP_SERVICE), EVENTLOG_WARNING_TYPE );
#endif
		          fclose(pContext->Logfp);
				}
#endif
#ifdef UPDATE_20260603 // ログ出力の排他処理化を追加
                ReleaseMutex(g_hMutexInLog);  // 排他終了
#endif
			  }
	          ///// end inlog //////////////////////
#endif
////////////////////////////////////////////////////////////
#ifdef UPDATE_20231031 // 上長承認が内部のメーリングリスト宛だと、承認メールが作成されない不具合
		      if (!pContext->mBOSS[0] && !bNL && nRCP > 1) // ML以外のアドレス送信が無いなら。
#else
			  if (!bNL && nRCP > 1) // ML以外のアドレス送信が無いなら。
#endif
			  {
#ifndef TUNING1
			    DeleteFile(pContext->mFnHead);
                DeleteFile(pContext->mFnData);
		        DeleteFile(pContext->mFnTemp);
#else
			    _unlink(pContext->mFnHead);
                _unlink(pContext->mFnData);
		        _unlink(pContext->mFnTemp);
#endif
#ifdef UPDATE_20040720_LOG
                sprintf(mLog, "DeleteFile(%s)", pContext->mFnHead);
                LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
                sprintf(mLog, "DeleteFile(%s)", pContext->mFnData);
                LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
                sprintf(mLog, "DeleteFile(%s)", pContext->mFnTemp);
                LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
#endif
			  }
 		      //// 広告添付サービス ////
#ifdef ANNOUNCE
			  //Announce(pContext);
#ifdef UPDATE_20210722_LOG
          sprintf(mLog, "LINE=3307 / nRCP=%d / bHitaApprovalRev=%d / bHitaFilter=%d", nRCP, bHitaApprovalRev, bHitaFilter);
          LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
#endif
			  for (n = 0; n < nRCP; n++) {
				if (n == 0) {
                  sprintf(mFnRCP, "%sB%010lu.RCP",mTempDir,pContext->nMsgId);
				} else {
	              sprintf(mFnRCP, "%sB%010lu-%03lu.RCP",mTempDir,pContext->nMsgId, n);
                  strcpy(pContext->mFnTemp, pContext->mFnData);
                  sprintf(pContext->mFnData, "%sB%010lu-%03lu.TMP",mTempDir,pContext->nMsgId, n);
				}
                pContext->RCPfp = fopen(mFnRCP, "rt");
	            if (pContext->RCPfp) {
	              mMES[0] = mRET[0] = mRCPTO[0] = '\x0';
	              fgets(mMES, sizeof(mMES), pContext->RCPfp); // Message-ID
	              fgets(mRET, sizeof(mRET), pContext->RCPfp); // Return-path
 	              fgets(mRCPTO, sizeof(mRCPTO), pContext->RCPfp); // Recipient-1
                  fclose(pContext->RCPfp);
				  strtok(mRCPTO,"\n");
				  strcpy(pContext->mUIDRCPT, &mRCPTO[11]);
				}
 			    Announce(pContext);
				if (n > 0) {
                  strcpy(pContext->mFnData, pContext->mFnTemp);
				}
			  }
#endif
 		      //// incoming Folder Lock ////
#ifdef UPDATE_20051108 // RCP,MSGの移動ロストしないように待つ
              while(!(pContext->Lockfp = fopen(mLockFile, "wt"))) { // 対象ML展開フォルダをロック
                if (bServiceTerminating)
		          break;
	             _sleep(WAIT_TIME);
			  }
              //fclose(pContext->Lockfp); クローズしない
#else
#ifndef DATA_CASH
              pContext->Lockfp = fopen(mLockFile, "at");
		      if (pContext->Lockfp)
		        fclose(pContext->Lockfp);
#ifdef UPDATE_20051104 // RCP,MSGの移動ロストしないように待つ
              while(!(pContext->Lockfp = fopen(mLockFile, "rt"))) { // 対象ML展開フォルダをロック
                if (bServiceTerminating)
		          break;
	             _sleep(WAIT_TIME);
			  }
              fclose(pContext->Lockfp);
#endif
#endif
#endif
		      //////////////////////////////
#ifndef TEST_MODE
#ifdef UPDATE_20210726 // 1メールのサイズ上限が設定されているとMILTER有効時に接続元のアドレスによって受信メールを破棄するオプションがスキップしてしまう。
#ifdef UPDATE_20150212 // MILTER有効時に接続元のアドレスによって受信メールを破棄するオプション
		      if (bMILTER && 
				  DiscardMail(pContext->PeerAddr, pContext->mUIDFROM))
			  {
	            while(!DeleteFile(pContext->mFnData)) // MSGファイルを破棄（削除）
				{
	              if (bServiceTerminating ||
                      GetLastError() == ERROR_FILE_NOT_FOUND)
		            break;
                  _sleep(100);
				}
#ifdef UPDATE_20040720_LOG
                sprintf(mLog, "DiscardFile(%s)", pContext->mFnData);
                LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
#endif
	            while(!DeleteFile(pContext->mFnHead)) // RCPファイルを破棄（削除）
				{
	              if (bServiceTerminating ||
                      GetLastError() == ERROR_FILE_NOT_FOUND)
		            break;
                  _sleep(100);
				}
#ifdef UPDATE_20040720_LOG
                sprintf(mLog, "DiscardFile(%s)", pContext->mFnHead);
                LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
#endif
			  } else 
#endif
			  {
#endif //UPDATE_20210726
#ifdef UPDATE_20210722_LOG
          sprintf(mLog, "LINE=3360 / nRCP=%d / bHitaApprovalRev=%d / bHitaFilter=%d", nRCP, bHitaApprovalRev, bHitaFilter);
          LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
#endif
			  for (n = 0; n < nRCP; n++) {
				if (n == 0) {
              //MoveFile( pContext->mFnData, pContext->mFnMSG );
              //MoveFile( pContext->mFnHead, pContext->mFnRCP );
#ifdef UPDATE_20040720_LOG
                  sprintf(mLog, "MoveFileEx(%s, %s)", pContext->mFnData, pContext->mFnMSG);
                  LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
                  sprintf(mLog, "MoveFileEx(%s, %s)", pContext->mFnHead, pContext->mFnRCP);
                  LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
#endif

#ifdef UPDATE_20050606 // RCPの再作成でロストしないように待つ
#ifdef UPDATE_20210725 // セキュアハンドラ処理でブラックリスト対象且つ添付分離機能が有効時かつ1メールのサイズ上限が設定されているとメール作業フォルダ内の".TMP"ファイルが削除されないことがあった。
#ifdef UPDATE_20210405A // 上長承認有効で添付ファイル分離が行われると、余分なRCPファイルが残ることがある。
#ifdef UPDATE_20231025 // 上長承認依頼メールの作成に失敗した場合の作成リトライ
				{
                  int i = 0;
	              while(!(fp1 = fopen(pContext->mFnData, "rt")))
	              {
                    if (bServiceTerminating || i > nMailApprovalMessNum)
	                  break;
                    i++;
	                _sleep(WAIT_TIME+i);
	              }
#ifdef UPDATE_20231026 // 上長承認依頼メールの作成に失敗した場合の作成リトライ
				  if (fp1)
				  {
                    sprintf(mLog, "AccessCheck(%s) Open file success. retry=%d", pContext->mFnData, i);
				  } else { // ファイル作成できないとき
                    sprintf(mLog, "AccessCheck(%s) Open file fail. retry=%d, errno=%d", pContext->mFnData, i-1, GetLastError());
				  }
                  LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
				}
#ifndef UPDATE_20231026 // 上長承認依頼メールの作成に失敗した場合の作成リトライ
	            if (!fp1) // ファイル作成できないとき
				{
                  sprintf(mLog, "[%s] Open file fail. retry=%d , errno=%d", pContext->mFnData, nMailApprovalMessNum, GetLastError());
                  LEVEL_3_ACCEPTLOG(NULL, mLog);
				}
#endif
				if (fp1)
#else
				 if (fp1 = fopen(pContext->mFnData, "rt"))
#endif
                 {
                   fclose(fp1);
#endif
#ifdef UPDATE_20210721_LOG
        sprintf(mLog, "Line=3417 / nRCP=%d /  MoveFileEx(%s, %s)", nRCP, pContext->mFnData, pContext->mFnMSG);
        LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
#endif
#endif
#ifdef UPDATE_20100902 // メール受信サイズ制限有効時に共有DISKを作業フォルダにすると通信がロックする不具合
                  MoveFileEx( pContext->mFnData, pContext->mFnMSG , MOVEFILE_COPY_ALLOWED | MOVEFILE_WRITE_THROUGH);
#else
                  MoveFileEx( pContext->mFnData, pContext->mFnMSG , MOVEFILE_WRITE_THROUGH);
#endif
	              while ((fp1 = fopen(pContext->mFnData, "rt"))) 
				  {
		            fclose(fp1);
		            _sleep(0);
#ifdef UPDATE_20100902 // メール受信サイズ制限有効時に共有DISKを作業フォルダにすると通信がロックする不具合
                    MoveFileEx( pContext->mFnData, pContext->mFnMSG , MOVEFILE_COPY_ALLOWED | MOVEFILE_WRITE_THROUGH);
#else
                    MoveFileEx( pContext->mFnData, pContext->mFnMSG , MOVEFILE_WRITE_THROUGH);
#endif
				  }
#ifdef UPDATE_20231025 // 上長承認依頼メールの作成に失敗した場合の作成リトライ
                  sprintf(mLog, "MoveFileEx(%s, %s) is successed.", pContext->mFnData, pContext->mFnMSG);
                  LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
#endif
#ifdef UPDATE_20100902 // メール受信サイズ制限有効時に共有DISKを作業フォルダにすると通信がロックする不具合
                  MoveFileEx( pContext->mFnHead, pContext->mFnRCP , MOVEFILE_COPY_ALLOWED | MOVEFILE_WRITE_THROUGH);
#else
                  MoveFileEx( pContext->mFnHead, pContext->mFnRCP , MOVEFILE_WRITE_THROUGH);
#endif
	              while ((fp1 = fopen(pContext->mFnHead, "rt"))) 
				  {
		            fclose(fp1);
		            _sleep(0);
#ifdef UPDATE_20100902 // メール受信サイズ制限有効時に共有DISKを作業フォルダにすると通信がロックする不具合
                    MoveFileEx( pContext->mFnHead, pContext->mFnRCP , MOVEFILE_COPY_ALLOWED | MOVEFILE_WRITE_THROUGH);
#else
                    MoveFileEx( pContext->mFnHead, pContext->mFnRCP , MOVEFILE_WRITE_THROUGH);
#endif
				  }
                  sprintf(mLog, "MoveFileEx(%s, %s) is successed.", pContext->mFnHead, pContext->mFnRCP);
                  LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
#else
                  MoveFileEx( pContext->mFnData, pContext->mFnMSG , MOVEFILE_WRITE_THROUGH);
				  MoveFileEx( pContext->mFnHead, pContext->mFnRCP , MOVEFILE_WRITE_THROUGH);
#endif
#ifdef UPDATE_20210725 // セキュアハンドラ処理でブラックリスト対象且つ添付分離機能が有効時かつ1メールのサイズ上限が設定されているとメール作業フォルダ内の".TMP"ファイルが削除されないことがあった。
#ifdef UPDATE_20210405A // 上長承認有効で添付ファイル分離が行われると、余分なRCPファイルが残ることがある。
                  } else {
#ifdef UPDATE_20210721_LOG
        sprintf(mLog, "nRCP=%d / DeleteFile(%s)", nRCP, pContext->mFnHead);
        LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
#endif
                    DeleteFile(pContext->mFnHead);
#ifdef UPDATE_20210721_LOG
        sprintf(mLog, "nRCP=%d / DeleteFile(%s)", nRCP, pContext->mFnTemp);
        LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
#endif
                    DeleteFile(pContext->mFnTemp);
                  }
#endif
#endif //UPDATE_20210725
				} else {
#ifdef UPDATE_20071204  // メッセージＩＤ採番処理を修正(Bym10id)
                  sprintf(pContext->mFnMSG, "%s%s%s-%03lu.MSG",mMailSpoolDir,mMailQueueDir,pContext->mMsgId, n);
                  sprintf(pContext->mFnRCP, "%s%s%s-%03lu.RCP",mMailSpoolDir,mMailQueueDir,pContext->mMsgId, n);
#else
                  sprintf(pContext->mFnMSG, "%s%sB%010lu-%03lu.MSG",mMailSpoolDir,mMailQueueDir,pContext->nMsgId, n);
                  sprintf(pContext->mFnRCP, "%s%sB%010lu-%03lu.RCP",mMailSpoolDir,mMailQueueDir,pContext->nMsgId, n);
#endif
#ifndef TUNING1
#ifdef UPDATE_20071204  // メッセージＩＤ採番処理を修正(Bym10id)
	              sprintf(mFnRCP, "%s%s-%03lu.RCP",mTempDir,pContext->mMsgId, n);
                  sprintf(pContext->mFnTemp, "%s%s-%03lu.TMP",mTempDir,pContext->mMsgId, n);
#else
	              sprintf(mFnRCP, "%sB%010lu-%03lu.RCP",mTempDir,pContext->nMsgId, n);
                  sprintf(pContext->mFnTemp, "%sB%010lu-%03lu.TMP",mTempDir,pContext->nMsgId, n);
#endif
#else
	              sprintf(mFnRCP, "%s%sB%010lu-%03lu.RC$",mMailSpoolDir,mMailQueueDir,pContext->nMsgId, n);
                  sprintf(pContext->mFnTemp, "%s%sB%010lu-%03lu.TM$",mMailSpoolDir,mMailQueueDir,pContext->nMsgId, n);
#endif
#ifdef UPDATE_20040720_LOG
                  sprintf(mLog, "MoveFileEx(%s, %s)", pContext->mFnTemp, pContext->mFnMSG);
                  LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
                  sprintf(mLog, "MoveFileEx(%s, %s)", mFnRCP, pContext->mFnRCP);
                  LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
#endif

#ifdef UPDATE_20050606 // RCPの再作成でロストしないように待つ
                  MoveFileEx( pContext->mFnTemp, pContext->mFnMSG , MOVEFILE_WRITE_THROUGH);
	              while ((fp1 = fopen(pContext->mFnTemp, "rt"))) {
		            fclose(fp1);
		            _sleep(0);
                    MoveFileEx( pContext->mFnTemp, pContext->mFnMSG , MOVEFILE_WRITE_THROUGH);
				  }
                  MoveFileEx( mFnRCP, pContext->mFnRCP , MOVEFILE_WRITE_THROUGH);
	              while ((fp1 = fopen(mFnRCP, "rt"))) {
		            fclose(fp1);
		            _sleep(0);
                    MoveFileEx( mFnRCP, pContext->mFnRCP , MOVEFILE_WRITE_THROUGH);
				  }
                  sprintf(mLog, "MoveFileEx(%s, %s) is successed.", mFnRCP, pContext->mFnRCP);
                  LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
#else
                  MoveFileEx( pContext->mFnTemp, pContext->mFnMSG , MOVEFILE_WRITE_THROUGH);
                  MoveFileEx( mFnRCP, pContext->mFnRCP , MOVEFILE_WRITE_THROUGH);
#endif
#ifdef UPDATE_20210726 // 1メールのサイズ上限が設定されているとMILTER有効時に接続元のアドレスによって受信メールを破棄するオプションがスキップしてしまう。
#ifdef UPDATE_20210405A // 上長承認有効で添付ファイル分離が行われると、余分なRCPファイルが残ることがある。
                    if (fp1 = fopen(pContext->mFnTemp, "rt"))
                    {
                      fclose(fp1);
#endif
#ifdef UPDATE_20210721_LOG
        sprintf(mLog, "nRCP=%d / MoveFileEx(%s, %s)", nRCP, pContext->mFnTemp, pContext->mFnMSG);
        LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
#endif
#ifdef UPDATE_20060721 // 共有DISK方式で複数ファイルの移動があると失敗する。
                      MoveFileEx( pContext->mFnTemp, pContext->mFnMSG , MOVEFILE_COPY_ALLOWED | MOVEFILE_WRITE_THROUGH);
#else
                      MoveFileEx( pContext->mFnTemp, pContext->mFnMSG , MOVEFILE_WRITE_THROUGH);
#endif
#ifdef UPDATE_20051101 // RCP,MSGの移動ロストしないように待つ
                      while (!(fp1 = fopen(pContext->mFnMSG, "rt"))) { // 移動先にファイルが作成されたことを確認
                        if (bServiceTerminating)
		                  break;
		                _sleep(0);
#ifdef UPDATE_20060721 // 共有DISK方式で複数ファイルの移動があると失敗する。
                        MoveFileEx( pContext->mFnTemp, pContext->mFnMSG , MOVEFILE_COPY_ALLOWED | MOVEFILE_WRITE_THROUGH);
#else
					    MoveFileEx( pContext->mFnTemp, pContext->mFnMSG , MOVEFILE_WRITE_THROUGH);
#endif
					  }
	                  fclose(fp1);
		              while ((fp1 = fopen(pContext->mFnTemp, "rt"))) { // MSGファイルが削除されていない場合に補助的に削除
                        if (bServiceTerminating)
 		                  break;
		                fclose(fp1);
		                DeleteFile(pContext->mFnTemp);
		                _sleep(0);
					  }
#else
#ifdef UPDATE_20190329 // ML同報時の移動状態を詳細ログに追加
                      nMRty = nMovefileRetry; // MoveFileEx()での失敗時のリトライ上限 0=無制限
#endif
	                  while ((fp1 = fopen(pContext->mFnTemp, "rt"))) {
		                fclose(fp1);
                        if (bServiceTerminating)
 		                  break;
#ifdef UPDATE_20190329 // ML同報時の移動状態を詳細ログに追加
					    if (nMovefileRetry > 0 && nMRty == 0)
					    {
                          sprintf(mLog, "MoveFileEx(%s, %s) fail. retry times %d.", pContext->mFnTemp, pContext->mFnMSG, nMovefileRetry);
                          LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
					      break;
				        }
					    _sleep(1);
                        CopyFile(pContext->mFnTemp, pContext->mFnMSG , TRUE);
					    DeleteFile(pContext->mFnTemp);
					    nMRty--;
#else
#ifdef UPDATE_20060721 // 共有DISK方式で複数ファイルの移動があると失敗する。
                        MoveFileEx( pContext->mFnTemp, pContext->mFnMSG , MOVEFILE_COPY_ALLOWED | MOVEFILE_WRITE_THROUGH);
#else
                        MoveFileEx( pContext->mFnTemp, pContext->mFnMSG , MOVEFILE_WRITE_THROUGH);
#endif
#endif
					  }
#endif
#ifdef UPDATE_20060721 // 共有DISK方式で複数ファイルの移動があると失敗する。
                      MoveFileEx( mFnRCP, pContext->mFnRCP , MOVEFILE_COPY_ALLOWED | MOVEFILE_WRITE_THROUGH);
#else
                      MoveFileEx( mFnRCP, pContext->mFnRCP , MOVEFILE_WRITE_THROUGH);
#endif
#ifdef UPDATE_20190329 // ML同報時の移動状態を詳細ログに追加
                      nMRty = nMovefileRetry; // MoveFileEx()での失敗時のリトライ上限 0=無制限
#endif
				      while ((fp1 = fopen(mFnRCP, "rt"))) {
		              fclose(fp1);
                        if (bServiceTerminating)
   		                  break;
#ifdef UPDATE_20190329 // ML同報時の移動状態を詳細ログに追加
					    if (nMovefileRetry > 0 && nMRty == 0)
					    {
                          sprintf(mLog, "MoveFileEx(%s, %s) fail. retry times %d.", mFnRCP, pContext->mFnRCP, nMovefileRetry);
                          LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
					      break;
				        }
		                _sleep(1);
                        CopyFile(mFnRCP, pContext->mFnRCP , TRUE);
					    DeleteFile(mFnRCP);
					    nMRty--;
#else
#ifdef UPDATE_20060721 // 共有DISK方式で複数ファイルの移動があると失敗する。
                        MoveFileEx( mFnRCP, pContext->mFnRCP , MOVEFILE_COPY_ALLOWED | MOVEFILE_WRITE_THROUGH);
#else
                        MoveFileEx( mFnRCP, pContext->mFnRCP , MOVEFILE_WRITE_THROUGH);
#endif
#endif
					  }
                      sprintf(mLog, "MoveFileEx(%s, %s) is successed.", mFnRCP, pContext->mFnRCP );
                      LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
#endif // UPDATE_20210726
#ifdef UPDATE_20210726 // 1メールのサイズ上限が設定されているとMILTER有効時に接続元のアドレスによって受信メールを破棄するオプションがスキップしてしまう。
#ifdef UPDATE_20210405A // 上長承認有効で添付ファイル分離が行われると、余分なRCPファイルが残ることがある。
                } else {
                  DeleteFile(mFnRCP);
                  DeleteFile(pContext->mFnTemp);
                }
#endif
#endif // UPDATE_20210726
				}
			  }
#endif
#ifdef UPDATE_20080225 // 部下に対する承認のため保留通知
              if (bMailApprovalRes) {
			    MoveStaffMail(mMSGID, pContext->mFnHead);
			  }
#endif
#ifdef UPDATE_20210726 // 1メールのサイズ上限が設定されているとMILTER有効時に接続元のアドレスによって受信メールを破棄するオプションがスキップしてしまう。
#ifdef UPDATE_20150212 // MILTER有効時に接続元のアドレスによって受信メールを破棄するオプション
			 } // UPDATE_20150212 // 接続元のアドレスによって受信メールを破棄するオプション
#endif
#endif
		      //// incoming Folder UnLock ////
#ifdef UPDATE_20051108 // RCP,MSGの移動ロストしないように待つ
              fclose(pContext->Lockfp); // 作成クローズ
		      if (pContext->Lockfp)
                _unlink( mLockFile );
#else
#ifndef DATA_CASH
#ifndef TUNING1
		      if (pContext->Lockfp)
                DeleteFile( mLockFile );
#else
		      if (pContext->Lockfp)
                _unlink( mLockFile );
#endif
#endif
#endif
	          //// test 用 /////////////////
#ifdef TEST_MODE
#ifdef UPDATE_20210722_LOG
          sprintf(mLog, "LINE=3483 / nRCP=%d / bHitaApprovalRev=%d / bHitaFilter=%d", nRCP, bHitaApprovalRev, bHitaFilter);
          LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
#endif
			  for (n = 0; n < nRCP; n++) {
				if (n == 0) {
#ifndef TUNING1
				  DeleteFile(pContext->mFnData);
                  DeleteFile(pContext->mFnHead);
#else
				  _unlink(pContext->mFnData);
                  _unlink(pContext->mFnHead);
#endif
#ifdef UPDATE_20040720_LOG
                  sprintf(mLog, "DeleteFile(%s)", pContext->mFnData);
                  LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
                  sprintf(mLog, "DeleteFile(%s)", pContext->mFnHead);
                  LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
#endif
				} else {
#ifndef TUNING1
#ifdef UPDATE_20071204  // メッセージＩＤ採番処理を修正(Bym10id)
	              sprintf(mFnRCP, "%s%s-%03lu.RCP",mTempDir,pContext->mMsgId, n);
                  sprintf(pContext->mFnTemp, "%s%s-%03lu.TMP",mTempDir,pContext->mMsgId, n);
#else
	              sprintf(mFnRCP, "%sB%010lu-%03lu.RCP",mTempDir,pContext->nMsgId, n);
                  sprintf(pContext->mFnTemp, "%sB%010lu-%03lu.TMP",mTempDir,pContext->nMsgId, n);
#endif
                  DeleteFile(pContext->mFnTemp);
                  DeleteFile(mFnRCP);
#else
	              sprintf(mFnRCP, "%s%sB%010lu-%03lu.RC$",mMailSpoolDir,mMailQueueDir,pContext->nMsgId, n);
                  sprintf(pContext->mFnTemp, "%s%sB%010lu-%03lu.TM$",mMailSpoolDir,mMailQueueDir,pContext->nMsgId, n);
                  _unlink(pContext->mFnTemp);
                  _unlink(mFnRCP);
#endif
#ifdef UPDATE_20040720_LOG
                  sprintf(mLog, "DeleteFile(%s)", pContext->mFnTemp);
                  LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
                  sprintf(mLog, "DeleteFile(%s)", mFnRCP);
                  LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
#endif
				}
			  }
#endif
			  pContext->bAcptData = TRUE;
              sprintf(pContext->mMessages, SMTP_GOOD_MESS, "Message received");
			} else {
#ifdef UPDATE_20210722_LOG
          sprintf(mLog, "LINE=3531 / nRCP=%d / bHitaApprovalRev=%d / bHitaFilter=%d", nRCP, bHitaApprovalRev, bHitaFilter);
          LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
#endif
			  for (n = 0; n < nRCP; n++) {
				if (n == 0) {
#ifndef TUNING1
                  DeleteFile(pContext->mFnData);
                  DeleteFile(pContext->mFnHead);
#else
                  _unlink(pContext->mFnData);
                  _unlink(pContext->mFnHead);
#endif
#ifdef UPDATE_20040720_LOG
                  sprintf(mLog, "DeleteFile(%s)", pContext->mFnData);
                  LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
                  sprintf(mLog, "DeleteFile(%s)", pContext->mFnHead);
                  LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
#endif
#ifdef UPDATE_20210725 // セキュアハンドラ処理でブラックリスト対象且つ添付分離機能が有効時かつ1メールのサイズ上限が設定されているとメール作業フォルダ内の".TMP"ファイルが削除されないことがあった。
			      DeleteFile(pContext->mFnTemp);
#endif // UPDATE_20210725
				} else {
#ifndef TUNING1
#ifdef UPDATE_20071204  // メッセージＩＤ採番処理を修正(Bym10id)
	              sprintf(mFnRCP, "%s%s-%03lu.RCP",mTempDir,pContext->mMsgId, n);
                  sprintf(pContext->mFnTemp, "%s%s-%03lu.TMP",mTempDir,pContext->mMsgId, n);
#else
	              sprintf(mFnRCP, "%sB%010lu-%03lu.RCP",mTempDir,pContext->nMsgId, n);
                  sprintf(pContext->mFnTemp, "%sB%010lu-%03lu.TMP",mTempDir,pContext->nMsgId, n);
#endif
                  DeleteFile(pContext->mFnTemp);
                  DeleteFile(mFnRCP);
#else
	              sprintf(mFnRCP, "%s%sB%010lu-%03lu.RC$",mMailSpoolDir,mMailQueueDir,pContext->nMsgId, n);
                  sprintf(pContext->mFnTemp, "%s%sB%010lu-%03lu.TM$",mMailSpoolDir,mMailQueueDir,pContext->nMsgId, n);
                  _unlink(pContext->mFnTemp);
                  _unlink(mFnRCP);
#endif
#ifdef UPDATE_20040720_LOG
                  sprintf(mLog, "DeleteFile(%s)", pContext->mFnTemp);
                  LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
                  sprintf(mLog, "DeleteFile(%s)", mFnRCP);
                  LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
#endif
				}
			  }
#ifdef UPDATE_20061102 // 受信メールのエンベロープと本文の任意のフォルダへのバックアップ機能の追加
              if (bMailBackup) {
#ifdef UPDATE_20090227 // データ削除の完了確認対策
	            while(!DeleteFile(mBackRCP)) 
				{
	              nErr = GetLastError();
	              if (nErr == ERROR_FILE_NOT_FOUND)
		            break;
                  sprintf(mLog, "DeleteFile(%s) Fail. code=(%d) retry delete.", mBackRCP, nErr);
                  LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
	              _sleep(100);
				}
	            while(!DeleteFile(mBackEML)) 
				{
	              nErr = GetLastError();
	              if (nErr == ERROR_FILE_NOT_FOUND)
		            break;
                  sprintf(mLog, "DeleteFile(%s) Fail. code=(%d) retry delete.", mBackEML, nErr);
                  LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
	              _sleep(100);
				}
#else
                DeleteFile(mBackRCP);
                DeleteFile(mBackEML);
#endif
			  }
#endif
#ifdef UPDATE_20161112 //題名ヘッダでのブラック＆ホワイトリストテーブル
#ifdef UPDATE_20170309 // 長承認または、セキュアハンドラが有効時にウイルスチェックでウイルス発見された時のクライアントへ正しくない応答メッセージを送信してしまう
              if (!strnicmp(pContext->mMessages, "354 ", 4))
#else
              if (!bMailApprovalRevers && !bHitaApprovalRev)
#endif
#endif
			  {
                sprintf(pContext->mMessages, SMTP_BAD_DATAVIRUS);
			  }
			}
		  } else {
#ifdef UPDATE_20210722_LOG
          sprintf(mLog, "LINE=3614 / nRCP=%d / bHitaApprovalRev=%d / bHitaFilter=%d", nRCP, bHitaApprovalRev, bHitaFilter);
          LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
#endif
			for (n = 0; n < nRCP; n++) {
			  if (n == 0) {
#ifndef TUNING1
				DeleteFile(pContext->mFnData);
                DeleteFile(pContext->mFnHead);
#else
				_unlink(pContext->mFnData);
                _unlink(pContext->mFnHead);
#endif
#ifdef UPDATE_20040720_LOG
                sprintf(mLog, "DeleteFile(%s)", pContext->mFnData);
                LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
                sprintf(mLog, "DeleteFile(%s)", pContext->mFnHead);
                LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
#endif
#ifdef UPDATE_20210725 // セキュアハンドラ処理でブラックリスト対象且つ添付分離機能が有効時かつ1メールのサイズ上限が設定されているとメール作業フォルダ内の".TMP"ファイルが削除されないことがあった。
			    DeleteFile(pContext->mFnTemp);
#endif // UPDATE_20210725
			  } else {
#ifndef TUNING1
#ifdef UPDATE_20071204  // メッセージＩＤ採番処理を修正(Bym10id)
	            sprintf(mFnRCP, "%s%s-%03lu.RCP",mTempDir,pContext->mMsgId, n);
                sprintf(pContext->mFnTemp, "%s%s-%03lu.TMP",mTempDir,pContext->mMsgId, n);
#else
	            sprintf(mFnRCP, "%sB%010lu-%03lu.RCP",mTempDir,pContext->nMsgId, n);
                sprintf(pContext->mFnTemp, "%sB%010lu-%03lu.TMP",mTempDir,pContext->nMsgId, n);
#endif
                DeleteFile(pContext->mFnTemp);
                DeleteFile(mFnRCP);
#else
	            sprintf(mFnRCP, "%s%sB%010lu-%03lu.RC$",mMailSpoolDir,mMailQueueDir,pContext->nMsgId, n);
                sprintf(pContext->mFnTemp, "%s%sB%010lu-%03lu.TM$",mMailSpoolDir,mMailQueueDir,pContext->nMsgId, n);
                _unlink(pContext->mFnTemp);
                _unlink(mFnRCP);
#endif
#ifdef UPDATE_20040720_LOG
                sprintf(mLog, "DeleteFile(%s)", pContext->mFnTemp);
                LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
                sprintf(mLog, "DeleteFile(%s)", mFnRCP);
                LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
#endif
			  }
			}
#ifdef UPDATE_20061102 // 受信メールのエンベロープと本文の任意のフォルダへのバックアップ機能の追加
            if (bMailBackup) {
#ifdef UPDATE_20090227 // データ削除の完了確認対策
	          while(!DeleteFile(mBackRCP)) 
			  {
	            nErr = GetLastError();
	            if (nErr == ERROR_FILE_NOT_FOUND)
		          break;
                sprintf(mLog, "DeleteFile(%s) Fail. code=(%d) retry delete.", mBackRCP, nErr);
                LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
	            _sleep(100);
			  }
	          while(!DeleteFile(mBackEML)) 
			  {
	            nErr = GetLastError();
	            if (nErr == ERROR_FILE_NOT_FOUND)
		          break;
                sprintf(mLog, "DeleteFile(%s) Fail. code=(%d) retry delete.", mBackEML, nErr);
                LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
	            _sleep(100);
			  }
#else
              DeleteFile(mBackRCP);
              DeleteFile(mBackEML);
#endif
			}
#endif
		    sprintf(pContext->mMessages, SMTP_BAD_DATASIZE);
		  }
		  FindClose(pContext->hFindFile);
		}
	  } else {
#ifdef UPADTE_20040428
		if (!bHitaFilter) // フィルタにヒットしたものがない時のみ処理
#else
#ifdef V3
	    if (Filter(pContext, &mRCPTO[11]) && Check_Of_MIME(pContext))
#else
	    if (Filter(pContext, &mRCPTO[11]))
#endif
#endif
		{
	      //CarbonCopy(pContext);
		  ////////////////////////////////////
		  //// クライアントへは先に通知
#ifdef UPDATE_20041225_SPEEDUP
          sprintf(pContext->mMessages, SMTP_GOOD_MESS, "Message received");
          put_reply(lpClientContext, TRUE);
#endif
#ifndef UPDATE_20060627A // ML宛てで同報があるとinlogに記録できない
	      ///// start inlog //////////////////////
#ifdef UPDATE_20260610 // 本文情報がブラックリストにヒットした場合はinlogへの記録はしない
          if (bLog && !bHitaFilter) // == 0 && !strnicmp(pContext->mMessages,""354 ", 4))
#else
          if (bLog)
#endif
		  {
#ifdef Y2038_BUG
	        //GetLocalTime(&pContext->lt);
            sprintf(pContext->mdata, "%02d/%s/%04d:%02d:%02d:%02d",pContext->lt.wDay, mMonth[pContext->lt.wMonth-1], pContext->lt.wYear, pContext->lt.wHour, pContext->lt.wMinute, pContext->lt.wSecond);
#else
	        //pContext->lt = *localtime(&pContext->ltime);
            strftime( pContext->mdata, 128, "%d/%b/%Y:%H:%M:%S", &pContext->lt );
#endif
#ifdef UPDATE_20260606 // ログの書き込み信頼性向上対策
            WaitForSingleObject(g_hMutexInLog, INFINITE);  // 排他開始
            fpInLog = OpenCloseLog(fpInLog, mDTInLog, "inlog", mComName, pContext->lt);
	        if (fpInLog)
			{
#ifdef K_SEARCH // K_SEARCH OEM 版
	          fprintf(pContext->Logfp, "<%s@%s> [%s] %s %s",
                   pContext->mMsgId,
                   pContext->LocalName,
                   pContext->mdata,
                   pContext->PeerAddr,
                   pContext->PeerHelo);
#else
#ifdef UPDATE_20071204  // メッセージＩＤ採番処理を修正(Bym10id)
	          fprintf(fpInLog, "<%s@%s> [%s] %s %s",
                   pContext->mMsgId,
                   pContext->LocalName,
                   pContext->mdata,
                   pContext->PeerAddr,
                   pContext->PeerHelo);
#else
		      fprintf(fpInLog, "<B%010lu@%s> [%s] %s %s",
                   pContext->nMsgId,
                   pContext->LocalName,
                   pContext->mdata,
                   pContext->PeerAddr,
                   pContext->PeerHelo);
#endif
#endif
               pContext->Headfp = fopen(pContext->mFnHead, "rt");
		       if (pContext->Headfp) {
		         fgets(pContext->mdata, sizeof(pContext->mdata), pContext->Headfp); // Messages-ID
	             fgets(pContext->mdata, sizeof(pContext->mdata), pContext->Headfp); // mail from address
		         while(!feof(pContext->Headfp)) {
		           strtok(pContext->mdata,"\r\n");
		           pContext->p = strstr(pContext->mdata," ");
		           if (pContext->p)
		             fprintf(pContext->Logfp, "%s", pContext->p);
		           fgets(pContext->mdata, sizeof(pContext->mdata), pContext->Headfp); // rcpt to address
				 }
		         fclose(pContext->Headfp);
			   }
               fprintf(fpInLog, "\n");
#ifdef UPDATE_20060615 // データのディスク書込に異常が有った場合の対策
			   if (fflush(fpInLog) == EOF)
				 if (bVLog)
                   AddToMessageLog(TEXT("inlog write fail."), 104, TEXT(SMTP_SERVICE), EVENTLOG_WARNING_TYPE );
#endif
			}
#else
#ifdef REGTOFILE
      if (nClustering) {
	    sprintf(pContext->mLogFn, "%sinlog\\%s\\%s.log", mMailSpoolDir, mComName, pContext->mdata);
	  } else
#endif
	  {
	    sprintf(pContext->mLogFn, "%sinlog\\%s.log", mMailSpoolDir, pContext->mdata);
	  }
#ifdef Y2038_BUG
            sprintf(pContext->mdata, "%02d/%s/%04d:%02d:%02d:%02d",pContext->lt.wDay, mMonth[pContext->lt.wMonth-1], pContext->lt.wYear, pContext->lt.wHour, pContext->lt.wMinute, pContext->lt.wSecond);
#else
            strftime( pContext->mdata, 128, "%d/%b/%Y:%H:%M:%S", &pContext->lt );
#endif
#ifdef UPDATE_20260603 // ログ出力の排他処理化を追加
            WaitForSingleObject(g_hMutexInLog, INFINITE);  // 排他開始
#endif
	        pContext->Logfp = _fsopen(pContext->mLogFn, "at", _SH_DENYNO);
	        if (pContext->Logfp)
			{
#ifdef K_SEARCH // K_SEARCH OEM 版
	          fprintf(pContext->Logfp, "<%s@%s> [%s] %s %s",
                   pContext->mMsgId,
                   pContext->LocalName,
                   pContext->mdata,
                   pContext->PeerAddr,
                   pContext->PeerHelo);
#else
#ifdef UPDATE_20071204  // メッセージＩＤ採番処理を修正(Bym10id)
	          fprintf(pContext->Logfp, "<%s@%s> [%s] %s %s",
                   pContext->mMsgId,
                   pContext->LocalName,
                   pContext->mdata,
                   pContext->PeerAddr,
                   pContext->PeerHelo);
#else
		      fprintf(pContext->Logfp, "<B%010lu@%s> [%s] %s %s",
                   pContext->nMsgId,
                   pContext->LocalName,
                   pContext->mdata,
                   pContext->PeerAddr,
                   pContext->PeerHelo);
#endif
#endif
               pContext->Headfp = fopen(pContext->mFnHead, "rt");
		       if (pContext->Headfp) {
		         fgets(pContext->mdata, sizeof(pContext->mdata), pContext->Headfp); // Messages-ID
	             fgets(pContext->mdata, sizeof(pContext->mdata), pContext->Headfp); // mail from address
		         while(!feof(pContext->Headfp)) {
		           strtok(pContext->mdata,"\r\n");
		           pContext->p = strstr(pContext->mdata," ");
		           if (pContext->p)
		             fprintf(pContext->Logfp, "%s", pContext->p);
		           fgets(pContext->mdata, sizeof(pContext->mdata), pContext->Headfp); // rcpt to address
				 }
		         fclose(pContext->Headfp);
			   }
               fprintf(pContext->Logfp, "\n");
#ifdef UPDATE_20060615 // データのディスク書込に異常が有った場合の対策
			   if (fflush(pContext->Logfp) == EOF)
				 if (bVLog)
                   AddToMessageLog(TEXT("inlog write fail."), 104, TEXT(SMTP_SERVICE), EVENTLOG_WARNING_TYPE );
#endif
		       fclose(pContext->Logfp);
			}
#endif
#ifdef UPDATE_20260603 // ログ出力の排他処理化を追加
            ReleaseMutex(g_hMutexInLog);  // 排他終了
#endif
		  }
	      ///// end inlog //////////////////////
#endif
#ifdef UPDATE_20231031 // 上長承認が内部のメーリングリスト宛だと、承認メールが作成されない不具合
		  if (!pContext->mBOSS[0] && !bNL && nRCP > 1) // ML以外のアドレス送信が無いなら。
#else
		  if (!bNL && nRCP > 1) // ML以外のアドレス送信が無いなら。
#endif
		  {
#ifndef TUNING1
		    DeleteFile(pContext->mFnHead);
            DeleteFile(pContext->mFnData);
		    DeleteFile(pContext->mFnTemp);
#else
		    _unlink(pContext->mFnHead);
            _unlink(pContext->mFnData);
		    _unlink(pContext->mFnTemp);
#endif
#ifdef UPDATE_20040720_LOG
            sprintf(mLog, "DeleteFile(%s)", pContext->mFnHead);
            LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
            sprintf(mLog, "DeleteFile(%s)", pContext->mFnData);
            LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
            sprintf(mLog, "DeleteFile(%s)", pContext->mFnTemp);
            LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
#endif
		  }
	      //// 広告添付サービス ////
#ifdef ANNOUNCE
		  //Announce(pContext);
#ifdef UPDATE_20210722_LOG
          sprintf(mLog, "LINE=3802 / nRCP=%d / bHitaApprovalRev=%d / bHitaFilter=%d", nRCP, bHitaApprovalRev, bHitaFilter);
          LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
#endif
		  for (n = 0; n < nRCP; n++) {
			if (n == 0) {
#ifndef TUNING1
#ifdef UPDATE_20071204  // メッセージＩＤ採番処理を修正(Bym10id)
              sprintf(mFnRCP, "%s%s.RCP",mTempDir,pContext->mMsgId);
#else
              sprintf(mFnRCP, "%sB%010lu.RCP",mTempDir,pContext->nMsgId);
#endif
#else
              sprintf(mFnRCP, "%s%sB%010lu.RC$",mMailSpoolDir,mMailQueueDir,pContext->nMsgId);
#endif
			} else {
#ifndef TUNING1
#ifdef UPDATE_20071204  // メッセージＩＤ採番処理を修正(Bym10id)
	          sprintf(mFnRCP, "%s%s-%03lu.RCP",mTempDir,pContext->mMsgId, n);
              strcpy(pContext->mFnTemp, pContext->mFnData);
              sprintf(pContext->mFnData, "%s%s-%03lu.TMP",mTempDir,pContext->mMsgId, n);
#else
	          sprintf(mFnRCP, "%sB%010lu-%03lu.RCP",mTempDir,pContext->nMsgId, n);
              strcpy(pContext->mFnTemp, pContext->mFnData);
              sprintf(pContext->mFnData, "%sB%010lu-%03lu.TMP",mTempDir,pContext->nMsgId, n);
#endif
#else
              sprintf(mFnRCP, "%s%sB%010lu-%03lu.RC$",mMailSpoolDir,mMailQueueDir,pContext->nMsgId, n);
              strcpy(pContext->mFnTemp, pContext->mFnData);
              sprintf(pContext->mFnData, "%s%sB%010lu-%03lu.TM$",mMailSpoolDir,mMailQueueDir,pContext->nMsgId, n);
#endif
			}
            pContext->RCPfp = fopen(mFnRCP, "rt");
	        if (pContext->RCPfp) {
	          mMES[0] = mRET[0] = mRCPTO[0] = '\x0';
	          fgets(mMES, sizeof(mMES), pContext->RCPfp); // Message-ID
	          fgets(mRET, sizeof(mRET), pContext->RCPfp); // Return-path
 	          fgets(mRCPTO, sizeof(mRCPTO), pContext->RCPfp); // Recipient-1
              fclose(pContext->RCPfp);
			  strtok(mRCPTO,"\n");
			  strcpy(pContext->mUIDRCPT, &mRCPTO[11]);
			}
 			Announce(pContext);
			if (n > 0) {
              strcpy(pContext->mFnData, pContext->mFnTemp);
			}
		  }
#endif
  	      //// incoming Folder Lock ////
#ifdef UPDATE_20051108 // RCP,MSGの移動ロストしないように待つ
              while(!(pContext->Lockfp = fopen(mLockFile, "wt"))) { // 対象ML展開フォルダをロック
                if (bServiceTerminating)
		          break;
	             _sleep(WAIT_TIME);
			  }
              //fclose(pContext->Lockfp); クローズしない
#else
#ifndef DATA_CASH
          pContext->Lockfp = fopen(mLockFile, "at");
		  if (pContext->Lockfp)
	        fclose(pContext->Lockfp);
#ifdef UPDATE_20051104 // RCP,MSGの移動ロストしないように待つ
          while(!(pContext->Lockfp = fopen(mLockFile, "rt"))) { // 対象ML展開フォルダをロック
            if (bServiceTerminating)
		      break;
	         _sleep(WAIT_TIME);
		  }
          fclose(pContext->Lockfp);
#endif
#endif
#endif
	      //////////////////////////////
#ifndef TEST_MODE
#ifdef UPDATE_20150212 // MILTER有効時に接続元のアドレスによって受信メールを破棄するオプション
		      if (bMILTER && 
				  DiscardMail(pContext->PeerAddr, pContext->mUIDFROM))
			  {
	            while(!DeleteFile(pContext->mFnData)) // MSGファイルを破棄（削除）
				{
	              if (bServiceTerminating ||
                      GetLastError() == ERROR_FILE_NOT_FOUND)
		            break;
                  _sleep(100);
				}
#ifdef UPDATE_20040720_LOG
                sprintf(mLog, "DiscardFile(%s)", pContext->mFnData);
                LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
#endif
	            while(!DeleteFile(pContext->mFnHead)) // RCPファイルを破棄（削除）
				{
	              if (bServiceTerminating ||
                      GetLastError() == ERROR_FILE_NOT_FOUND)
		            break;
                  _sleep(100);
				}
#ifdef UPDATE_20040720_LOG
                sprintf(mLog, "DiscardFile(%s)", pContext->mFnHead);
                LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
#endif
			  } else 
#endif
			  {
#ifdef UPDATE_20210722_LOG
          sprintf(mLog, "LINE=3904 / nRCP=%d / bHitaApprovalRev=%d / bHitaFilter=%d", nRCP, bHitaApprovalRev, bHitaFilter);
          LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
#endif
			    for (n = 0; n < nRCP; n++) {
				  if (n == 0) {
          //MoveFile( pContext->mFnData, pContext->mFnMSG );
          //MoveFile( pContext->mFnHead, pContext->mFnRCP );
#ifdef UPDATE_20040720_LOG
                    sprintf(mLog, "MoveFileEx(%s, %s)", pContext->mFnData, pContext->mFnMSG);
                    LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
                    sprintf(mLog, "MoveFileEx(%s, %s)", pContext->mFnHead, pContext->mFnRCP);
                    LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
#endif
				  ////////////////////////////////
#ifdef UPDATE_20041224_FILESPEED
                    MoveFile( pContext->mFnData, pContext->mFnMSG);
                    MoveFileEx( pContext->mFnHead, pContext->mFnRCP, MOVEFILE_WRITE_THROUGH);
#else
////CCCC
#ifdef UPDATE_20050606 // RCPの再作成でロストしないように待つ
#ifdef UPDATE_20210405A // 上長承認有効で添付ファイル分離が行われると、余分なRCPファイルが残ることがある。
#ifdef UPDATE_20231025 // 上長承認依頼メールの作成に失敗した場合の作成リトライ
					{
                      int i = 0;
	                  while(!(fp1 = fopen(pContext->mFnData, "rt")))
	                  {
                        if (bServiceTerminating || i > nMailApprovalMessNum)
	                      break;
                        i++;
	                    _sleep(WAIT_TIME+i);
	                  }
#ifdef UPDATE_20231026 // 上長承認依頼メールの作成に失敗した場合の作成リトライ
				      if (fp1)
				      {
                        sprintf(mLog, "AccessCheck(%s) Open file success. retry=%d", pContext->mFnData, i);
				      } else { // ファイル作成できないとき
                        sprintf(mLog, "AccessCheck(%s) Open file fail. retry=%d, errno=%d", pContext->mFnData, i-1, GetLastError());
				      }
                      LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
					}
#ifndef UPDATE_20231026 // 上長承認依頼メールの作成に失敗した場合の作成リトライ
	                if (!fp1) // ファイル作成できないとき
					{
                      sprintf(mLog, "[%s] Open file fail. retry=%d , errno=%d", pContext->mFnData, nMailApprovalMessNum, GetLastError());
                      LEVEL_3_ACCEPTLOG(NULL, mLog);
					}
#endif
					if (fp1)
#else
                    if (fp1 = fopen(pContext->mFnData, "rt"))
#endif
                    {
                      fclose(fp1);
#endif
#ifdef UPDATE_20210721_LOG
        sprintf(mLog, "nRCP=%d /  MoveFileEx(%s, %s)", nRCP, pContext->mFnData, pContext->mFnMSG);
        LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
#endif
                      MoveFileEx( pContext->mFnData, pContext->mFnMSG , MOVEFILE_COPY_ALLOWED | MOVEFILE_WRITE_THROUGH);
#ifdef UPDATE_20051101 // RCP,MSGの移動ロストしないように待つ
                      while (!(fp1 = fopen(pContext->mFnMSG, "rt"))) { // 移動先にファイルが作成されたことを確認
                        if (bServiceTerminating)
		                  break;
		                _sleep(0);
#ifdef UPDATE_20060721 // 共有DISK方式で複数ファイルの移動があると失敗する。
				        MoveFileEx( pContext->mFnData, pContext->mFnMSG , MOVEFILE_COPY_ALLOWED | MOVEFILE_WRITE_THROUGH);
#else
				        MoveFileEx( pContext->mFnData, pContext->mFnMSG , MOVEFILE_WRITE_THROUGH);
#endif
					  }
	                  fclose(fp1);
		              while ((fp1 = fopen(pContext->mFnData, "rt"))) { // MSGファイルが削除されていない場合に補助的に削除
                        if (bServiceTerminating)
  		                  break;
		                fclose(fp1);
		                DeleteFile(pContext->mFnData);
		                _sleep(0);
					  }
#else
#ifdef UPDATE_20190329 // ML同報時の移動状態を詳細ログに追加
                      nMRty = nMovefileRetry; // MoveFileEx()での失敗時のリトライ上限 0=無制限
#endif
	                  while ((fp1 = fopen(pContext->mFnData, "rt"))) {
		                fclose(fp1);
                        if (bServiceTerminating)
 		                  break;
#ifdef UPDATE_20190329 // ML同報時の移動状態を詳細ログに追加
					    if (nMovefileRetry > 0 && nMRty == 0)
					    {
                          sprintf(mLog, "MoveFileEx(%s, %s) fail. retry times %d.", pContext->mFnData, pContext->mFnMSG, nMovefileRetry);
                          LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
					      break;
					    }
		                _sleep(1);
                        CopyFile(pContext->mFnData, pContext->mFnMSG , TRUE);
					    DeleteFile(pContext->mFnData);
					    nMRty--;
#else
                        MoveFileEx( pContext->mFnData, pContext->mFnMSG , MOVEFILE_COPY_ALLOWED | MOVEFILE_WRITE_THROUGH);
#endif
					  }
#endif
#ifdef UPDATE_20231025 // 上長承認依頼メールの作成に失敗した場合の作成リトライ
                      sprintf(mLog, "MoveFileEx(%s, %s) is successed.", pContext->mFnData, pContext->mFnMSG);
                      LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
#endif

                      MoveFileEx( pContext->mFnHead, pContext->mFnRCP , MOVEFILE_COPY_ALLOWED | MOVEFILE_WRITE_THROUGH);
#ifdef UPDATE_20190329 // ML同報時の移動状態を詳細ログに追加
                      nMRty = nMovefileRetry; // MoveFileEx()での失敗時のリトライ上限 0=無制限
#endif
	                  while ((fp1 = fopen(pContext->mFnHead, "rt"))) {
		                fclose(fp1);
                        if (bServiceTerminating)
 		                  break;
#ifdef UPDATE_20190329 // ML同報時の移動状態を詳細ログに追加
					    if (nMovefileRetry > 0 && nMRty == 0)
					    {
                          sprintf(mLog, "MoveFileEx(%s, %s) fail. retry times %d.", pContext->mFnHead, pContext->mFnRCP, nMovefileRetry);
                          LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
					      break;
				        }
		                _sleep(1);
                        CopyFile(pContext->mFnHead, pContext->mFnRCP , TRUE);
					    DeleteFile(pContext->mFnHead);
					    nMRty--;
#else
                        MoveFileEx( pContext->mFnHead, pContext->mFnRCP , MOVEFILE_COPY_ALLOWED | MOVEFILE_WRITE_THROUGH);
#endif
					  }
                      sprintf(mLog, "MoveFileEx(%s, %s) is successed.", pContext->mFnHead, pContext->mFnRCP );
                      LEVEL_3_ACCEPTLOG(lpClientContext, mLog);

#ifdef UPDATE_20210405A // 上長承認有効で添付ファイル分離が行われると、余分なRCPファイルが残ることがある。
                    } else {
#ifdef UPDATE_20210721_LOG
        sprintf(mLog, "nRCP=%d / DeleteFile(%s)", nRCP, pContext->mFnHead);
        LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
#endif
                      DeleteFile(pContext->mFnHead);
#ifdef UPDATE_20210721_LOG
        sprintf(mLog, "nRCP=%d / DeleteFile(%s)", nRCP, pContext->mFnTemp);
        LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
#endif
                      DeleteFile(pContext->mFnTemp);
                    }
#endif
#else
#ifdef UPDATE_20060721 // 共有DISK方式で複数ファイルの移動があると失敗する。
                    MoveFileEx( pContext->mFnData, pContext->mFnMSG , MOVEFILE_COPY_ALLOWED | MOVEFILE_WRITE_THROUGH);
                    MoveFileEx( pContext->mFnHead, pContext->mFnRCP , MOVEFILE_COPY_ALLOWED | MOVEFILE_WRITE_THROUGH);
#else
                    MoveFileEx( pContext->mFnData, pContext->mFnMSG , MOVEFILE_WRITE_THROUGH);
                    MoveFileEx( pContext->mFnHead, pContext->mFnRCP , MOVEFILE_WRITE_THROUGH);
#endif
#endif
#endif
				  } else {
#ifdef UPDATE_20071204  // メッセージＩＤ採番処理を修正(Bym10id)
                    sprintf(pContext->mFnMSG, "%s%s%s-%03lu.MSG",mMailSpoolDir,mMailQueueDir,pContext->mMsgId, n);
                    sprintf(pContext->mFnRCP, "%s%s%s-%03lu.RCP",mMailSpoolDir,mMailQueueDir,pContext->mMsgId, n);
#else
                    sprintf(pContext->mFnMSG, "%s%sB%010lu-%03lu.MSG",mMailSpoolDir,mMailQueueDir,pContext->nMsgId, n);
                    sprintf(pContext->mFnRCP, "%s%sB%010lu-%03lu.RCP",mMailSpoolDir,mMailQueueDir,pContext->nMsgId, n);
#endif
#ifndef TUNING1
#ifdef UPDATE_20041224_FILESPEED
	                sprintf(mFnRCP, "%s%stemp_B%010lu-%03lu.RCX",mMailSpoolDir,mMailQueueDir,pContext->nMsgId, n);
                    sprintf(pContext->mFnTemp, "%s%stemp_B%010lu-%03lu.TMP",mMailSpoolDir,mMailQueueDir,pContext->nMsgId, n);
#else
#ifdef UPDATE_20071204  // メッセージＩＤ採番処理を修正(Bym10id)
	                sprintf(mFnRCP, "%s%s-%03lu.RCP",mTempDir,pContext->mMsgId, n);
                    sprintf(pContext->mFnTemp, "%s%s-%03lu.TMP",mTempDir,pContext->mMsgId, n);
#else
	                sprintf(mFnRCP, "%sB%010lu-%03lu.RCP",mTempDir,pContext->nMsgId, n);
                    sprintf(pContext->mFnTemp, "%sB%010lu-%03lu.TMP",mTempDir,pContext->nMsgId, n);
#endif
#endif
#else
	                sprintf(mFnRCP, "%s%sB%010lu-%03lu.RC$",mMailSpoolDir,mMailQueueDir,pContext->nMsgId, n);
                    sprintf(pContext->mFnTemp, "%s%sB%010lu-%03lu.TM$",mMailSpoolDir,mMailQueueDir,pContext->nMsgId, n);
#endif
#ifdef UPDATE_20040720_LOG
                    sprintf(mLog, "MoveFileEx(%s, %s)", pContext->mFnTemp, pContext->mFnMSG);
                    LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
                    sprintf(mLog, "MoveFileEx(%s, %s)", mFnRCP, pContext->mFnRCP);
                    LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
#endif
				  ////////////////////////////////
#ifdef UPDATE_20041224_FILESPEED
                    MoveFile( pContext->mFnTemp, pContext->mFnMSG);
                    MoveFileEx( mFnRCP, pContext->mFnRCP , MOVEFILE_WRITE_THROUGH);
#else
////CCCC
#ifdef UPDATE_20050606 // RCPの再作成でロストしないように待つ
#ifdef UPDATE_20210405A // 上長承認有効で添付ファイル分離が行われると、余分なRCPファイルが残ることがある。
                    if (fp1 = fopen(pContext->mFnTemp, "rt"))
                    {
                      fclose(fp1);
#endif
#ifdef UPDATE_20210721_LOG
        sprintf(mLog, "nRCP=%d / MoveFileEx(%s, %s)", nRCP, pContext->mFnTemp, pContext->mFnMSG);
        LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
#endif
#ifdef UPDATE_20060721 // 共有DISK方式で複数ファイルの移動があると失敗する。
                      MoveFileEx( pContext->mFnTemp, pContext->mFnMSG , MOVEFILE_COPY_ALLOWED | MOVEFILE_WRITE_THROUGH);
#else
                      MoveFileEx( pContext->mFnTemp, pContext->mFnMSG , MOVEFILE_WRITE_THROUGH);
#endif
#ifdef UPDATE_20051101 // RCP,MSGの移動ロストしないように待つ
                      while (!(fp1 = fopen(pContext->mFnMSG, "rt"))) { // 移動先にファイルが作成されたことを確認
                        if (bServiceTerminating)
		                  break;
		                _sleep(0);
#ifdef UPDATE_20060721 // 共有DISK方式で複数ファイルの移動があると失敗する。
                        MoveFileEx( pContext->mFnTemp, pContext->mFnMSG , MOVEFILE_COPY_ALLOWED | MOVEFILE_WRITE_THROUGH);
#else
					    MoveFileEx( pContext->mFnTemp, pContext->mFnMSG , MOVEFILE_WRITE_THROUGH);
#endif
					  }
	                  fclose(fp1);
		              while ((fp1 = fopen(pContext->mFnTemp, "rt"))) { // MSGファイルが削除されていない場合に補助的に削除
                        if (bServiceTerminating)
 		                  break;
		                fclose(fp1);
		                DeleteFile(pContext->mFnTemp);
		                _sleep(0);
					  }
#else
#ifdef UPDATE_20190329 // ML同報時の移動状態を詳細ログに追加
                      nMRty = nMovefileRetry; // MoveFileEx()での失敗時のリトライ上限 0=無制限
#endif
	                  while ((fp1 = fopen(pContext->mFnTemp, "rt"))) {
		                fclose(fp1);
                        if (bServiceTerminating)
 		                  break;
#ifdef UPDATE_20190329 // ML同報時の移動状態を詳細ログに追加
					    if (nMovefileRetry > 0 && nMRty == 0)
					    {
                          sprintf(mLog, "MoveFileEx(%s, %s) fail. retry times %d.", pContext->mFnTemp, pContext->mFnMSG, nMovefileRetry);
                          LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
					      break;
				        }
					    _sleep(1);
                        CopyFile(pContext->mFnTemp, pContext->mFnMSG , TRUE);
					    DeleteFile(pContext->mFnTemp);
					    nMRty--;
#else
#ifdef UPDATE_20060721 // 共有DISK方式で複数ファイルの移動があると失敗する。
                        MoveFileEx( pContext->mFnTemp, pContext->mFnMSG , MOVEFILE_COPY_ALLOWED | MOVEFILE_WRITE_THROUGH);
#else
                        MoveFileEx( pContext->mFnTemp, pContext->mFnMSG , MOVEFILE_WRITE_THROUGH);
#endif
#endif
					  }
#endif
#ifdef UPDATE_20060721 // 共有DISK方式で複数ファイルの移動があると失敗する。
                      MoveFileEx( mFnRCP, pContext->mFnRCP , MOVEFILE_COPY_ALLOWED | MOVEFILE_WRITE_THROUGH);
#else
                      MoveFileEx( mFnRCP, pContext->mFnRCP , MOVEFILE_WRITE_THROUGH);
#endif
#ifdef UPDATE_20190329 // ML同報時の移動状態を詳細ログに追加
                      nMRty = nMovefileRetry; // MoveFileEx()での失敗時のリトライ上限 0=無制限
#endif
				      while ((fp1 = fopen(mFnRCP, "rt"))) {
		              fclose(fp1);
                        if (bServiceTerminating)
   		                  break;
#ifdef UPDATE_20190329 // ML同報時の移動状態を詳細ログに追加
					    if (nMovefileRetry > 0 && nMRty == 0)
					    {
                          sprintf(mLog, "MoveFileEx(%s, %s) fail. retry times %d.", mFnRCP, pContext->mFnRCP, nMovefileRetry);
                          LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
					      break;
				        }
		                _sleep(1);
                        CopyFile(mFnRCP, pContext->mFnRCP , TRUE);
					    DeleteFile(mFnRCP);
					    nMRty--;
#else
#ifdef UPDATE_20060721 // 共有DISK方式で複数ファイルの移動があると失敗する。
                        MoveFileEx( mFnRCP, pContext->mFnRCP , MOVEFILE_COPY_ALLOWED | MOVEFILE_WRITE_THROUGH);
#else
                        MoveFileEx( mFnRCP, pContext->mFnRCP , MOVEFILE_WRITE_THROUGH);
#endif
#endif
					  }
                      sprintf(mLog, "MoveFileEx(%s, %s) is successed.", mFnRCP, pContext->mFnRCP );
                      LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
#ifdef UPDATE_20210405A // 上長承認有効で添付ファイル分離が行われると、余分なRCPファイルが残ることがある。
                    } else {
                      DeleteFile(mFnRCP);
                      DeleteFile(pContext->mFnTemp);
                    }
#endif

#else
#ifdef UPDATE_20060721 // 共有DISK方式で複数ファイルの移動があると失敗する。
                    MoveFileEx( pContext->mFnTemp, pContext->mFnMSG , MOVEFILE_COPY_ALLOWED | MOVEFILE_WRITE_THROUGH);
                    MoveFileEx( mFnRCP, pContext->mFnRCP , MOVEFILE_COPY_ALLOWED | MOVEFILE_WRITE_THROUGH);
#else
                    MoveFileEx( pContext->mFnTemp, pContext->mFnMSG , MOVEFILE_WRITE_THROUGH);
                    MoveFileEx( mFnRCP, pContext->mFnRCP , MOVEFILE_WRITE_THROUGH);
#endif
#endif
#endif
				  }
				}
#endif
#ifdef UPDATE_20080225 // 部下に対する承認のため保留通知
                if (bMailApprovalRes) {
		          MoveStaffMail(mMSGID, pContext->mFnHead);
				}
#endif
              } // UPDATE_20150212 // 接続元のアドレスによって受信メールを破棄するオプション
	      //// incoming Folder UnLock ////
#ifdef UPDATE_20051108 // RCP,MSGの移動ロストしないように待つ
              fclose(pContext->Lockfp); // 作成クローズ
		      if (pContext->Lockfp)
                _unlink( mLockFile );
#else
#ifndef DATA_CASH
#ifndef TUNING1
		  if (pContext->Lockfp)
            DeleteFile( mLockFile );
#else
		  if (pContext->Lockfp)
            _unlink( mLockFile );
#endif
#endif
#endif
	      //// test 用 /////////////////
#ifdef TEST_MODE
#ifdef UPDATE_20210722_LOG
          sprintf(mLog, "LINE=4204 / nRCP=%d / bHitaApprovalRev=%d / bHitaFilter=%d", nRCP, bHitaApprovalRev, bHitaFilter);
          LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
#endif
			  for (n = 0; n < nRCP; n++) {
				if (n == 0) {
#ifndef TUNING1
                  DeleteFile(pContext->mFnData);
                  DeleteFile(pContext->mFnHead);
#else
                  _unlink(pContext->mFnData);
                  _unlink(pContext->mFnHead);
#endif
#ifdef UPDATE_20040720_LOG
                  sprintf(mLog, "DeleteFile(%s)", pContext->mFnData);
                  LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
                  sprintf(mLog, "DeleteFile(%s)", pContext->mFnHead);
                  LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
#endif
				} else {
#ifndef TUNING1
#ifdef UPDATE_20041224_FILESPEED
	              sprintf(mFnRCP, "%s%stemp_B%010lu-%03lu.RCX",mMailSpoolDir,mMailQueueDir,pContext->nMsgId, n);
                  sprintf(pContext->mFnTemp, "%s%stemp_B%010lu-%03lu.TMP",mMailSpoolDir,mMailQueueDir,pContext->nMsgId, n);
#else
#ifdef UPDATE_20071204  // メッセージＩＤ採番処理を修正(Bym10id)
	              sprintf(mFnRCP, "%s%s-%03lu.RCP",mTempDir,pContext->mMsgId, n);
                  sprintf(pContext->mFnTemp, "%s%s-%03lu.TMP",mTempDir,pContext->mMsgId, n);
#else
	              sprintf(mFnRCP, "%sB%010lu-%03lu.RCP",mTempDir,pContext->nMsgId, n);
                  sprintf(pContext->mFnTemp, "%sB%010lu-%03lu.TMP",mTempDir,pContext->nMsgId, n);
#endif
#endif
                  DeleteFile(pContext->mFnTemp);
                  DeleteFile(mFnRCP);
#else
	              sprintf(mFnRCP, "%s%sB%010lu-%03lu.RC$",mMailSpoolDir,mMailQueueDir,pContext->nMsgId, n);
                  sprintf(pContext->mFnTemp, "%s%sB%010lu-%03lu.TM$",mMailSpoolDir,mMailQueueDir,pContext->nMsgId, n);
                  _unlink(pContext->mFnTemp);
                  _unlink(mFnRCP);
#endif
#ifdef UPDATE_20040720_LOG
                  sprintf(mLog, "DeleteFile(%s)", pContext->mFnTemp);
                  LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
                  sprintf(mLog, "DeleteFile(%s)", mFnRCP);
                  LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
#endif
				}
			  }
#endif
	      //////////////////////////////
		  pContext->bAcptData = TRUE;
          sprintf(pContext->mMessages, SMTP_GOOD_MESS, "Message received");
		} else {
#ifdef UPDATE_20210722_LOG
          sprintf(mLog, "LINE=4226 / nRCP=%d / bHitaApprovalRev=%d / bHitaFilter=%d", nRCP, bHitaApprovalRev, bHitaFilter);
          LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
#endif
			  for (n = 0; n < nRCP; n++) {
				if (n == 0) {
#ifndef TUNING1
                  DeleteFile(pContext->mFnData);
                  DeleteFile(pContext->mFnHead);
#else
                  _unlink(pContext->mFnData);
                  _unlink(pContext->mFnHead);
#endif
#ifdef UPDATE_20040720_LOG
                  sprintf(mLog, "DeleteFile(%s)", pContext->mFnData);
                  LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
                  sprintf(mLog, "DeleteFile(%s)", pContext->mFnHead);
                  LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
#endif
#ifdef UPDATE_20210725 // セキュアハンドラ処理でブラックリスト対象且つ添付分離機能が有効時かつ1メールのサイズ上限が設定されているとメール作業フォルダ内の".TMP"ファイルが削除されないことがあった。
			      DeleteFile(pContext->mFnTemp);
#endif // UPDATE_20210725
				} else {
#ifndef TUNING1
#ifdef UPDATE_20071204  // メッセージＩＤ採番処理を修正(Bym10id)
	              sprintf(mFnRCP, "%s%s-%03lu.RCP",mTempDir,pContext->mMsgId, n);
                  sprintf(pContext->mFnTemp, "%s%s-%03lu.TMP",mTempDir,pContext->mMsgId, n);
#else
	              sprintf(mFnRCP, "%sB%010lu-%03lu.RCP",mTempDir,pContext->nMsgId, n);
                  sprintf(pContext->mFnTemp, "%sB%010lu-%03lu.TMP",mTempDir,pContext->nMsgId, n);
#endif
#ifdef UPDATE_20090227 // データ削除の完了確認対策
	              while(!DeleteFile(pContext->mFnTemp)) 
				  {
	                nErr = GetLastError();
	                if (nErr == ERROR_FILE_NOT_FOUND)
		              break;
                    sprintf(mLog, "DeleteFile(%s) Fail. code=(%d) retry delete.", pContext->mFnTemp, nErr);
                    LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
	                _sleep(100);
				  }
	              while(!DeleteFile(mFnRCP)) 
				  {
	                nErr = GetLastError();
	                if (nErr == ERROR_FILE_NOT_FOUND)
		              break;
                    sprintf(mLog, "DeleteFile(%s) Fail. code=(%d) retry delete.", mFnRCP, nErr);
                    LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
	                _sleep(100);
				  }
#else
                  DeleteFile(pContext->mFnTemp);
                  DeleteFile(mFnRCP);
#endif
#else
	              sprintf(mFnRCP, "%s%sB%010lu-%03lu.RC$",mMailSpoolDir,mMailQueueDir,pContext->nMsgId, n);
                  sprintf(pContext->mFnTemp, "%s%sB%010lu-%03lu.TM$",mMailSpoolDir,mMailQueueDir,pContext->nMsgId, n);
                  _unlink(pContext->mFnTemp);
                  _unlink(mFnRCP);
#endif
#ifdef UPDATE_20040720_LOG
                  sprintf(mLog, "DeleteFile(%s)", pContext->mFnTemp);
                  LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
                  sprintf(mLog, "DeleteFile(%s)", mFnRCP);
                  LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
#endif
				}
			  }
#ifdef UPDATE_20061102 // 受信メールのエンベロープと本文の任意のフォルダへのバックアップ機能の追加
              if (bMailBackup) {
#ifdef UPDATE_20090227 // データ削除の完了確認対策
	            while(!DeleteFile(mBackRCP)) 
				{
	              nErr = GetLastError();
	              if (nErr == ERROR_FILE_NOT_FOUND)
		            break;
                  sprintf(mLog, "DeleteFile(%s) Fail. code=(%d) retry delete.", mBackRCP, nErr);
                  LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
	              _sleep(100);
				}
	            while(!DeleteFile(mBackEML)) 
				{
	              nErr = GetLastError();
	              if (nErr == ERROR_FILE_NOT_FOUND)
		            break;
                  sprintf(mLog, "DeleteFile(%s) Fail. code=(%d) retry delete.", mBackEML, nErr);
                  LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
	              _sleep(100);
				}
#else
                DeleteFile(mBackRCP);
                DeleteFile(mBackEML);
#endif
			  }
#endif
#ifdef UPDATE_20161112 //題名ヘッダでのブラック＆ホワイトリストテーブル
#ifdef UPDATE_20170309 // 長承認または、セキュアハンドラが有効時にウイルスチェックでウイルス発見された時のクライアントへ正しくない応答メッセージを送信してしまう
              if (!strnicmp(pContext->mMessages, "354 ", 4))
#else
              if (!bMailApprovalRevers && !bHitaApprovalRev)
#endif
#endif
			  {
                sprintf(pContext->mMessages, SMTP_BAD_DATAVIRUS);
			  }
		}
	  }
	} else {
#ifdef UPDATE_20210722_LOG
          sprintf(mLog, "LINE=4331 / nRCP=%d / bHitaApprovalRev=%d / bHitaFilter=%d", nRCP, bHitaApprovalRev, bHitaFilter);
          LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
#endif
      for (n = 0; n < nRCP; n++) {
	    if (n == 0) {
#ifndef TUNING1
		  DeleteFile(pContext->mFnData);
          DeleteFile(pContext->mFnHead);
#else
		  _unlink(pContext->mFnData);
          _unlink(pContext->mFnHead);
#endif
#ifdef UPDATE_20040720_LOG
          sprintf(mLog, "DeleteFile(%s)", pContext->mFnData);
          LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
          sprintf(mLog, "DeleteFile(%s)", pContext->mFnHead);
          LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
#endif
		} else {
#ifndef TUNING1
#ifdef UPDATE_20071204  // メッセージＩＤ採番処理を修正(Bym10id)
          sprintf(mFnRCP, "%s%s-%03lu.RCP",mTempDir,pContext->mMsgId, n);
          sprintf(pContext->mFnTemp, "%s%s-%03lu.TMP",mTempDir,pContext->mMsgId, n);
#else
          sprintf(mFnRCP, "%sB%010lu-%03lu.RCP",mTempDir,pContext->nMsgId, n);
          sprintf(pContext->mFnTemp, "%sB%010lu-%03lu.TMP",mTempDir,pContext->nMsgId, n);
#endif
#ifdef UPDATE_20090227 // データ削除の完了確認対策
	      while(!DeleteFile(pContext->mFnTemp)) 
		  {
	        nErr = GetLastError();
	        if (nErr == ERROR_FILE_NOT_FOUND)
		      break;
            sprintf(mLog, "DeleteFile(%s) Fail. code=(%d) retry delete.", pContext->mFnTemp, nErr);
            LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
	        _sleep(100);
		  }
	      while(!DeleteFile(mFnRCP)) 
		  {
	        nErr = GetLastError();
	        if (nErr == ERROR_FILE_NOT_FOUND)
		      break;
            sprintf(mLog, "DeleteFile(%s) Fail. code=(%d) retry delete.", mFnRCP, nErr);
            LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
	        _sleep(100);
		  }
#else
          DeleteFile(pContext->mFnTemp);
          DeleteFile(mFnRCP);
#endif
#else
          sprintf(mFnRCP, "%s%sB%010lu-%03lu.RC$",mMailSpoolDir,mMailQueueDir,pContext->nMsgId, n);
          sprintf(pContext->mFnTemp, "%s%sB%010lu-%03lu.TM$",mMailSpoolDir,mMailQueueDir,pContext->nMsgId, n);
          _unlink(pContext->mFnTemp);
          _unlink(mFnRCP);
#endif
#ifdef UPDATE_20040720_LOG
          sprintf(mLog, "DeleteFile(%s)", pContext->mFnTemp);
          LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
          sprintf(mLog, "DeleteFile(%s)", mFnRCP);
          LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
#endif
		}
	  }
#ifdef UPDATE_20061102 // 受信メールのエンベロープと本文の任意のフォルダへのバックアップ機能の追加
      if (bMailBackup) {
#ifdef UPDATE_20090227 // データ削除の完了確認対策
        while(!DeleteFile(mBackRCP)) 
		{
	      nErr = GetLastError();
	      if (nErr == ERROR_FILE_NOT_FOUND)
		    break;
          sprintf(mLog, "DeleteFile(%s) Fail. code=(%d) retry delete.", mBackRCP, nErr);
          LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
          _sleep(100);
		}
	    while(!DeleteFile(mBackEML)) 
		{
	      nErr = GetLastError();
	      if (nErr == ERROR_FILE_NOT_FOUND)
		    break;
          sprintf(mLog, "DeleteFile(%s) Fail. code=(%d) retry delete.", mBackEML, nErr);
          LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
          _sleep(100);
		}
#else
        DeleteFile(mBackRCP);
        DeleteFile(mBackEML);
#endif
	  }
#endif
      sprintf(pContext->mMessages, SMTP_BAD_DATARECEIVED);
	}
	//////////////////////////////////
#ifdef UPDATE_20080225 // 部下に対する承認のため保留通知
    if (bMailApprovalRes) {
	  DeleteStaffMail(mMSGID, pContext->mFnHead); // 保留通知を削除
	}
#endif
#ifdef UPDATE_20061102 // 受信メールのエンベロープと本文の任意のフォルダへのバックアップ機能の追加
    if (bMailBackup) {
#ifdef UPDATE_20070316 // メールバックアップ先が異なるボリュームの場合移動できない不具合
#ifdef UPDATE_20071204  // メッセージＩＤ採番処理を修正(Bym10id)
#ifdef UPDATE_20090205 // バックアップメッセージの拡張子を指定するオプション
      sprintf(mBackEML2, "%s%s.%s", mMailBackupDir, pContext->mMsgId, mExtension);
#else
      sprintf(mBackEML2, "%s%s.EML", mMailBackupDir, pContext->mMsgId);
#endif
#ifdef UPDATE_20090227 // データ削除の完了確認対策
      while(!MoveFileEx(mBackEML, mBackEML2, MOVEFILE_COPY_ALLOWED | MOVEFILE_WRITE_THROUGH)) 
	  {
	    nErr = GetLastError();
	    if (nErr == ERROR_FILE_NOT_FOUND)
	      break;
        sprintf(mLog, "MoveFileEx(%s, %s) Fail. code=(%d) retry move.", mBackEML, mBackEML2, nErr);
        LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
        _sleep(100);
	  }
	  while(!DeleteFile(mBackEML)) 
	  {
	    nErr = GetLastError();
	    if (nErr == ERROR_FILE_NOT_FOUND)
	      break;
        sprintf(mLog, "DeleteFile(%s) Fail. code=(%d) retry delete.", mBackEML, nErr);
        LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
        _sleep(100);
	  }
#else
      MoveFileEx(mBackEML, mBackEML2, MOVEFILE_COPY_ALLOWED | MOVEFILE_WRITE_THROUGH);
#endif
      sprintf(mBackRCP2, "%s%s.RCP", mMailBackupDir, pContext->mMsgId);
#ifdef UPDATE_20090227 // データ削除の完了確認対策
      while(!MoveFileEx(mBackRCP, mBackRCP2, MOVEFILE_COPY_ALLOWED | MOVEFILE_WRITE_THROUGH)) 
	  {
	    nErr = GetLastError();
	    if (nErr == ERROR_FILE_NOT_FOUND)
	      break;
        sprintf(mLog, "MoveFileEx(%s, %s) Fail. code=(%d) retry move.", mBackEML, mBackEML2, nErr);
        LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
        _sleep(100);
	  }
	  while(!DeleteFile(mBackRCP)) 
	  {
	    nErr = GetLastError();
	    if (nErr == ERROR_FILE_NOT_FOUND)
	      break;
        sprintf(mLog, "DeleteFile(%s) Fail. code=(%d) retry delete.", mBackRCP, nErr);
        LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
        _sleep(100);
	  }
#else
      MoveFileEx(mBackRCP, mBackRCP2, MOVEFILE_COPY_ALLOWED | MOVEFILE_WRITE_THROUGH);
#endif
#else
#ifdef UPDATE_20090205 // バックアップメッセージの拡張子を指定するオプション
	  sprintf(mBackEML2, "%s%010lu.%s", mMailBackupDir, pContext->nMsgId, mExtension);
#else
      sprintf(mBackEML2, "%s%010lu.EML", mMailBackupDir, pContext->nMsgId);
#endif
#ifdef UPDATE_20090227 // データ削除の完了確認対策
      while(!MoveFileEx(mBackEML, mBackEML2, MOVEFILE_COPY_ALLOWED | MOVEFILE_WRITE_THROUGH)) 
	  {
	    nErr = GetLastError();
	    if (nErr == ERROR_FILE_NOT_FOUND)
	      break;
        sprintf(mLog, "MoveFileEx(%s, %s) Fail. code=(%d) retry move.", mBackEML, mBackEML2, nErr);
        LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
        _sleep(100);
	  }
	  while(!DeleteFile(mBackEML)) 
	  {
	    nErr = GetLastError();
	    if (nErr == ERROR_FILE_NOT_FOUND)
	      break;
        sprintf(mLog, "DeleteFile(%s) Fail. code=(%d) retry delete.", mBackEML, nErr);
        LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
        _sleep(100);
	  }
#else
      MoveFileEx(mBackEML, mBackEML2, MOVEFILE_COPY_ALLOWED | MOVEFILE_WRITE_THROUGH);
#endif
      sprintf(mBackRCP2, "%s%010lu.RCP", mMailBackupDir, pContext->nMsgId);
#ifdef UPDATE_20090227 // データ削除の完了確認対策
      while(!MoveFileEx(mBackRCP, mBackRCP2, MOVEFILE_COPY_ALLOWED | MOVEFILE_WRITE_THROUGH)) 
	  {
	    nErr = GetLastError();
	    if (nErr == ERROR_FILE_NOT_FOUND)
	      break;
        sprintf(mLog, "MoveFileEx(%s, %s) Fail. code=(%d) retry move.", mBackEML, mBackEML2, nErr);
        LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
        _sleep(100);
	  }
	  while(!DeleteFile(mBackRCP)) 
	  {
	    nErr = GetLastError();
	    if (nErr == ERROR_FILE_NOT_FOUND)
	      break;
        sprintf(mLog, "DeleteFile(%s) Fail. code=(%d) retry delete.", mBackRCP, nErr);
        LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
        _sleep(100);
	  }
#else
      MoveFileEx(mBackRCP, mBackRCP2, MOVEFILE_COPY_ALLOWED | MOVEFILE_WRITE_THROUGH);
#endif
#endif
#else
#ifdef UPDATE_20090205 // バックアップメッセージの拡張子を指定するオプション
	  sprintf(mBackEML2, "%s%010lu.%s", mMailBackupDir, pContext->nMsgId, mExtension);
#else
      sprintf(mBackEML2, "%s%010lu.EML", mMailBackupDir, pContext->nMsgId);
#endif
      MoveFileEx(mBackEML, mBackEML2, MOVEFILE_WRITE_THROUGH);
      sprintf(mBackRCP2, "%s%010lu.RCP", mMailBackupDir, pContext->nMsgId);
      MoveFileEx(mBackRCP, mBackRCP2, MOVEFILE_WRITE_THROUGH);
#endif
	}
#endif
///// start acceptlog //////////////////////
#ifdef UPDATE_20260610A // 本文情報がブラックリストにヒットした場合はinlogへの記録はしない
#ifdef UPDATE_20260610B // 本文情報がブラックリストにヒットした場合はinlogへの記録はしない
	  pContext->nMsgId2 = pContext->nMsgId;
  	  //strcpy(pContext->mMsgId2, pContext->mMsgId);
#endif
      if (bAcceptLog && pContext) {
        if (bDebug) printf("[%08x] AcceptLog\n", lpClientContext);
		if(*pContext->mAcptLogFn != '\x0') {
#ifdef UPDATE_20260606 // ログの書き込み信頼性向上対策
   	      SYSTEMTIME lt;
          _tzset();
          GetLocalTime(&lt);
          WaitForSingleObject(g_hMutexAcceptLog, INFINITE);  // 排他開始
          fpAcceptLog = OpenCloseLog(fpAcceptLog, mDTAcceptLog, "acceptlog", mComName, lt);
	      if (fpAcceptLog) {
#ifdef K_SEARCH // K_SEARCH OEM 版
	        fprintf(fpAcceptLog, "%s,<%s>,%s\n",
			                          pContext->mAcptLogState,
                                      pContext->mMsgId,
									  (pContext->bAcptData ? "ok" : "abort"));
#else
#ifdef UPDATE_20071204  // メッセージＩＤ採番処理を修正(Bym10id)
	        fprintf(fpAcceptLog, "%s,<%s>,%s\n",
			                          pContext->mAcptLogState,
                                      pContext->mMsgId,
									  (pContext->bAcptData ? "ok" : "abort"));
#else
	        fprintf(fpAcceptLog, "%s,<B%010lu>,%s\n",
			                          pContext->mAcptLogState,
                                      pContext->nMsgId,
									  (pContext->bAcptData ? "ok" : "abort"));

#endif
#endif
#ifdef UPDATE_20060615 // データのディスク書込に異常が有った場合の対策
		    if (fflush(fpAcceptLog) == EOF)
			  if (bVLog)
                AddToMessageLog(TEXT("acceptlog write fail."), 113, TEXT(SMTP_SERVICE), EVENTLOG_WARNING_TYPE);
#endif
  		    fflush(fpAcceptLog);
		  }
#endif
		}
	  }
#endif // end UPDATE_20260610A
#ifdef BTHREAD
    return TRUE;
#else
    *OutputBuffer = pContext->mMessages;
    *OutputBufferLen = strlen(pContext->mMessages);
    return(SMTPRS_SendBuffer);
#endif
}

#ifdef UPDATE_20070516 // 上長承認機能の追加

#ifdef UPDATE_20070713 // 承認者の代理人を設定するオプション
#ifdef UPDATE_20080220 // 承認者の代理人の代理人を設定する
#ifdef UPDATE_20080221 //BossCheck 承認者の代理人の代理人の深さの設定
void GetProxyUser(CHAR *pDir, CHAR *pBoss, CHAR *pAddr, DWORD nDepath)
#else
void GetProxyUser(CHAR *pDir, CHAR *pBoss, CHAR *pAddr)
#endif
#else
void GetProxyUser(CHAR *pDir, CHAR *pBoss)
#endif
{ // 代理人ファイルから読出し
  FILE *fp;
  CHAR mFn[256], mData[1024];

   mData[0] = '\x0';
   sprintf(mFn, "%sproxyuser.dat", pDir);
   if ((fp = fopen(mFn, "rt"))) {
	 fgets(mData, sizeof(mData), fp);
	 if (mData[0] == '\r' || mData[0] == '\n')
	   mData[0] = '\x0';
	 else
	   strtok(mData, "\r\n");
	 fclose(fp);
   }
   if (mData[0]) 
   {
#ifdef UPDATE_20080220 // 承認者の代理人の代理人を設定する
#ifdef UPDATE_20080221 //BossCheck 承認者の代理人の代理人の深さの設定
	 if (nMailApprovalDepath > nDepath)
#else
	 if (*pAddr)
#endif
	 { // NULLでないなら
       GetBaseDirectory(pDir, mMailBoxDir, mData, pAddr); // ユーザ別のフィルタ
#ifdef UPDATE_20080221 //BossCheck 承認者の代理人の代理人の深さの設定
	   GetProxyUser(pDir, mData, pAddr, nDepath+1); // 代理人ファイルから読出し
#else
	   GetProxyUser(pDir, mData, pAddr); // 代理人ファイルから読出し
#endif
	 }
#endif
	 strcpy(pBoss, mData);
   }
}
#ifdef UPDATE_20090701 // 代理承認処理メールの挿入文の書換えを可能に
void SetProxyUser(CHAR *pDir, CHAR *pAddr, DWORD nDepath, CHAR *pProxyAddr)
#else
#ifdef UPDATE_20080221 //BossCheck 承認者の代理人の代理人の深さの設定
void SetProxyUser(CHAR *pDir, CHAR *pAddr, DWORD nDepath)
#else
void SetProxyUser(CHAR *pDir, CHAR *pAddr)
#endif
#endif
{
  FILE *fp;
  CHAR mFn[256], mData[1024] = "";


   sprintf(mFn, "%sproxyuser.dat", pDir);
#ifdef UPDATE_20080221 //BossCheck 承認者の代理人の代理人の深さの設定
   if (nMailApprovalDepath > nDepath) {
     if ((fp = fopen(mFn, "rt"))) {
	   fgets(mData, sizeof(mData), fp);
	   if (mData[0] == '\r' || mData[0] == '\n')
	     mData[0] = '\x0';
	   else
	     strtok(mData, "\r\n");
	   fclose(fp);
       GetBaseDirectory(pDir, mMailBoxDir, mData, ""); // ユーザ別のフィルタ
#ifdef UPDATE_20090701 // 代理承認処理メールの挿入文の書換えを可能に
	   SetProxyUser(pDir, "", nDepath+1, pProxyAddr); // 代理人ファイルへ書込み
#else
	   SetProxyUser(pDir, "", nDepath+1); // 代理人ファイルへ書込み
#endif
	 }
   }
#endif
#ifdef UPDATE_20090701 // 代理承認処理メールの挿入文の書換えを可能に
   if ((fp = fopen(mFn, "rt"))) 
   {
	 fgets(mData, sizeof(mData), fp);
	 strtok(mData, "\r\n");
	 if (*pProxyAddr)
	 {
	   strcat(pProxyAddr, ",");
	 }
	 strcat(pProxyAddr, mData);
	 fclose(fp);
   }
#endif
   if ((fp = fopen(mFn, "wt"))) 
   {
	 fprintf(fp, "%s", pAddr);
	 fclose(fp);
   }
}

#endif

#ifdef UPDATE_20080225 // 部下に対する承認のため保留通知
void MailToStaff(CHAR *pHostName, CHAR *pMSGID, CHAR *pMSGSource, CHAR *pMSG, CHAR *pRCP, CHAR *pFrom, CHAR *pTos, CHAR *pB64Tos, CHAR *pSubject)
{
#ifdef UPDATE_20231017 // 上長承認依頼メールの作成に失敗した場合の作成リトライ
   char mLog[1024];
#endif
   char *p, *q, mt[128], mwork[256], mMSG[256], mRCP[256];
   char mMSG1[256], mRCP1[256];
   BOOL bHead = TRUE, bSub = FALSE;
   SYSTEMTIME ltime;
   FILE *fpsrc, *fpd, *fp;
   int i;
#ifdef UPDATE_20151118B // 上長承認対象メールのSubjectヘッダがEUC_JPでパックされていると文字化けする。
    CHAR *pSubjectCode;
#endif
#ifdef UPDATE_20080630 // In-Reply-Toへの対応
   CHAR  mID[512];
#endif
#ifdef UPDATE_20080903 // １メール送信ごとにセッションを切らずに連続送信すると２通目以降は片方の承認者のみにしか送信できなくなる不具合
   CHAR  *pf = strchr(pFrom, ',');
    if (pf)
    {
	  *pf = '\x0';
    }
#endif

#ifdef UPDATE_20080630 // In-Reply-Toへの対応
     mID[0] = '\x0';
     GetInfo(_T("message-id:"), pMSGSource, mID, sizeof(mID));
#endif
     gettime
		 (&ltime, mt);
     strcpy(mRCP, pRCP);
     strtok(mRCP, ".");
     strcpy(mMSG, mRCP);
     strcat(mRCP, "-S0001.RC$");
     strcat(mMSG, "-S0001.MSG");
#ifdef UPDATE_20231017 // 上長承認依頼メールの作成に失敗した場合の作成リトライ
     i = 0;
	 while(!(fpd = fopen(mMSG, "wt")))
	 {
       if (bServiceTerminating || i > nMailApprovalMessNum)
	     break;
       i++;
	   _sleep(WAIT_TIME+i);
	 }
#ifdef UPDATE_20231026 // 上長承認依頼メールの作成に失敗した場合の作成リトライ
     if (fpd)
	 {
       sprintf(mLog, "MailToStaff() [%s] Make file success. retry=%d", mMSG, i);
	 } else { // ファイル作成できないとき
       sprintf(mLog, "MailToStaff() [%s] Make file fail. retry=%d, errno=%d", mMSG, i-1, GetLastError());
	 }
	 LEVEL_3_ACCEPTLOG(NULL, mLog);
	 if (fpd)
#else
	 if (!fpd) // ファイル作成できないとき
	 {
       sprintf(mLog, "MailToStaff() [%s] make file fail. retry=%d , errno=%d", mMSG, nMailApprovalMessNum, GetLastError());
       LEVEL_3_ACCEPTLOG(NULL, mLog);
	 } else
#endif
#else
     if ((fpd = fopen(mMSG, "wt"))) 
#endif
	 {
#ifdef UPDATE_20080630 // In-Reply-Toへの対応
       if (mID[0])
	   {
	     fprintf(fpd, "In-Reply-To: %s\n", mID); // 対応するメッセージIDをIn-Reply-To:ヘッダへ記載
         fprintf(fpd, "References: %s\n", mID); // 対応するメッセージIDをIn-Reply-To:ヘッダへ記載
	   }
#endif     	 
#ifdef UPDATE_20151118B // 上長承認対象メールのSubjectヘッダがEUC_JPでパックされていると文字化けする。
       if (!(pSubjectCode = strstr(pSubject, "=?")))
	   {
		 pSubjectCode = pSubject;
	   }
#endif

#ifdef UPDATE_20080204A // 題名がUNICODE utf-7/utf-8でのBossCheckで承認依頼メールの題名が文字化けする
#ifdef UPDATE_20151118B // 上長承認対象メールのSubjectヘッダがEUC_JPでパックされていると文字化けする。
	   if (!strnicmp(pSubjectCode, "=?utf-7", 7)) 
#else
	   if (!strnicmp(pSubject, "=?utf-7", 7)) 
#endif
	   { 
		 fprintf(fpd, "Subject: =?utf-7?B?K1Q5MTFXWkFhZCtVLSA=?=\n"); // 承認保留通知
	   } 
#ifdef UPDATE_20151118B // 上長承認対象メールのSubjectヘッダがEUC_JPでパックされていると文字化けする。
	   else if (!strnicmp(pSubjectCode, "=?utf-8", 7)) 
#else
	   else if (!strnicmp(pSubject, "=?utf-8", 7))
#endif
	   { 
		 fprintf(fpd, "Subject: =?utf-8?B?5L+d55WZ6YCa55+lIA==?=\n"); // 承認保留通知
	   } else
#endif
#ifdef UPDATE_20151118B // 上長承認対象メールのSubjectヘッダがEUC_JPでパックされていると文字化けする。
	   if (!strnicmp(pSubjectCode, "=?euc-jp", 8)) 
	   { 
         fprintf(fpd, "Subject: =?euc-jp?B?yt3OscTMw84=?=\n"); // 承認保留通知
	   }
	   else if (!strnicmp(pSubjectCode, "=?shift_jis", 11))
	   { 
         fprintf(fpd, "Subject: =?shift_jis?B?lduXr5LKkm0=?=\n"); // 承認保留通知
	   } else
#endif
	   {
         //fprintf(fpd, "Subject: =?iso-2022-jp?B?GyRCSl1OMURMQ04bKEI=?=\n"); // 承認保留通知
		 fprintf(fpd, "Subject: =?iso-2022-jp?B?GyRCSl1OMURMQ04bKEIg?=\n"); // 承認保留通知
	   }
#ifdef UPDATE_20150807 // BOSSCheckで生成したSubject:ヘッダが１行結合されて出力しないようにした。
	   fprintf(fpd, "\t%s", pSubject);
#else
	   fprintf(fpd, "\t %s\n", pSubject);
#endif
#ifdef UPDATE_20240502 // 送信元が無い（NULL）場合の承認依頼通知メールのFrom:ヘッダにPostmasterアカウントを疑似的に追加する
	   fprintf(fpd, "From: %%s\n", (*pFrom ? pFrom: "postmaster@"), (*pFrom ? "": pHostName));
#else
       fprintf(fpd, "From: %s\n", pFrom);
#endif
       fprintf(fpd, "To: %s\n", pTos);
       fprintf(fpd, "Date: %s\n", mt);
       fprintf(fpd, "Message-Id: <%s.2@%s>\n", pMSGID, pHostName); //pB64Tos);
       fprintf(fpd, "Content-Type: multipart/mixed;\n\tboundary=\"----%s%s\"\n", pMSGID, pB64Tos);
       fprintf(fpd, "\n");
       fprintf(fpd, "\n------%s%s\n", pMSGID, pB64Tos);
       fprintf(fpd, "Content-Type: text/plain; charset=\"ISO-2022-JP\"\n\n"); 
	   ////////////////////////////////////////////////////////
	   // メッセージ挿入
	   fprintf(fpd, "\n");
       fprintf(fpd, "\x1B$B$3$N%%a!<%%k$O!\"!J\x1B(B%s\x1B$B!K$N>5G'$,I,MW$J0Y!\"Aw?.$,J]N1$5$l$^$7$?!#\x1B(B\n", pFrom);
	   fprintf(fpd, "\n\n");
	   ////////////////////////////////////////////////////////
       fprintf(fpd, "\n------%s%s\n", pMSGID, pB64Tos);
       fprintf(fpd, "Content-Type: message/rfc822\n\n"); 
#ifdef UPDATE_20230602 // 上長承認、セキュアハンドラへの通知メールに添付する元メールに特殊なバイナリコードが含まれていると正しく添付が出来ない不具合。
       if ((fpsrc = fopen(pMSGSource, "rb"))) 
#else
       if ((fpsrc = fopen(pMSGSource, "rt"))) 
#endif
	   {
         while (p = fgets(mwork, sizeof(mwork), fpsrc)) {
#ifdef UPDATE_20230602 // 上長承認、セキュアハンドラへの通知メールに添付する元メールに特殊なバイナリコードが含まれていると正しく添付が出来ない不具合。
           if (mwork[0] != '.' && (mwork[1] != '\r' || mwork[1] != '\n'))
		   {
			 if (mwork[0] == '\r' ||  mwork[0] == '\n') {
			   fputs("\n", fpd);
			 } else if (strtok(mwork, "\r\n")) {
			   fputs(mwork, fpd); fputs("\n", fpd);
			 } else {
               fputs(mwork, fpd);
			 }
		   }
#else
           if (mwork[0] != '.' && mwork[1] != '\n')
             fputs(mwork, fpd);
#endif
		 }
#ifdef UPDATE_20231017A // 上長承認依頼メールの作成に失敗した場合のファイルクローズ不具合
         fclose(fpsrc);
#endif
	   }
       fprintf(fpd, "\n------%s%s--\n", pMSGID, pB64Tos);
       fprintf(fpd, "\n.\n");
       fflush(fpd);
       fclose(fpd);
#ifndef UPDATE_20231017A // 上長承認依頼メールの作成に失敗した場合のファイルクローズ不具合
       fclose(fpsrc);
#endif
	 }
#ifdef UPDATE_20231017 // 上長承認依頼メールの作成に失敗した場合の作成リトライ
     i = 0;
	 while(!(fpd = fopen(mRCP, "wt")))
	 {
       if (bServiceTerminating || i > nMailApprovalMessNum)
	     break;
       i++;
	   _sleep(WAIT_TIME+i);
	 }
#ifdef UPDATE_20231026 // 上長承認依頼メールの作成に失敗した場合の作成リトライ
     if (fpd)
	 {
       sprintf(mLog, "MailToStaff() [%s] Make file success. retry=%d", mRCP, i);
	 } else { // ファイル作成できないとき
       sprintf(mLog, "MailToStaff() [%s] Make file fail. retry=%d, errno=%d", mRCP, i-1, GetLastError());
	 }
	 LEVEL_3_ACCEPTLOG(NULL, mLog);
	 if (fpd)
#else
	 if (!fpd) // ファイル作成できないとき
	 {
       sprintf(mLog, "MailToStaff() [%s] make file fail. retry=%d , errno=%d", mRCP, nMailApprovalMessNum, GetLastError());
       LEVEL_3_ACCEPTLOG(NULL, mLog);
	 } else
#endif
#else
     if ((fpd =  fopen(mRCP, "wt"))) 
#endif
	 {
       fprintf(fpd,"Message-ID: <%s@%s>\n",pMSGID, pHostName);
       fprintf(fpd, "Return-path: %s\n",pFrom);
       p = pTos;
       while(*p) {
         if ((q = strchr(p, ','))) {
           *q = '\x0';
		 } 
         fprintf(fpd, "Recipient: %s\n", p);
#ifdef UPDATE_20231017 // 上長承認依頼メールの作成に失敗した場合の作成リトライ
         sprintf(mLog, "MailToStaff() RCP File Set: pMSGID=%s, To Staff=%s", pMSGID, p);
         LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
         if (q)
           p = q + 1;
	     else
		   break;
	   }
	   fflush(fpd);
	   fclose(fpd);
	 }
     i = 0;
     while(!(fp = fopen(mMSG, "rt"))) {
#ifdef UPDATE_20231017 // 上長承認依頼メールの作成に失敗した場合の作成リトライ
       if (bServiceTerminating || i > nMailApprovalMessNum)
         break;
       i++;
	   _sleep(WAIT_TIME+i);
#else
       if (bServiceTerminating || i > 30)
         break;
       i++;
	   _sleep(WAIT_TIME);
#endif
     }
#ifdef UPDATE_20231026 // 上長承認依頼メールの作成に失敗した場合の作成リトライ
	 if (fp) {
       sprintf(mLog, "MailToStaff() [%s] Open file success. retry=%d", mMSG, i);
       fclose(fp);
	 } else {
       sprintf(mLog, "MailToStaff() [%s] Open file fail. retry=%d, errno=%d", mMSG, i-1, GetLastError());
	 }
     LEVEL_3_ACCEPTLOG(NULL, mLog);
#else
     if (fp) 
       fclose(fp);
#endif
     i = 0;
     while(!(fp = fopen(mRCP, "rt"))) {
#ifdef UPDATE_20231017 // 上長承認依頼メールの作成に失敗した場合の作成リトライ
       if (bServiceTerminating || i > nMailApprovalMessNum)
         break;
       i++;
	   _sleep(WAIT_TIME+i);
#else
       if (bServiceTerminating || i > 30)
	     break;
       i++;
	   _sleep(WAIT_TIME);
#endif
	 }
#ifdef UPDATE_20231026 // 上長承認依頼メールの作成に失敗した場合の作成リトライ
	 if (fp) {
       sprintf(mLog, "[%s] Open file success. retry=%d", mRCP, i);
       fclose(fp);
	 } else {
       sprintf(mLog, "[%s] Open file fail. retry=%d, errno=%d", mRCP, i-1, GetLastError());
	 }
     LEVEL_3_ACCEPTLOG(NULL, mLog);
#else
     if (fp) 
       fclose(fp);
#endif
#ifdef UPDATE_20080903 // １メール送信ごとにセッションを切らずに連続送信すると２通目以降は片方の承認者のみにしか送信できなくなる不具合
    if(pf)
    {
      *pf = ',';
    }
#endif

	 /*
     sprintf(mMSG1, "%s%s%s-S0001.MSG",mMailSpoolDir,mMailQueueDir,pMSGID);
     MoveFileEx(mMSG, mMSG1,  MOVEFILE_WRITE_THROUGH);
     sprintf(mRCP1, "%s%s%s-S0001.RCP",mMailSpoolDir,mMailQueueDir,pMSGID);
     MoveFileEx(mRCP, mRCP1,  MOVEFILE_WRITE_THROUGH);
	 */
}

// データをincomingフォルダに移動
void MoveStaffMail(CHAR *pMSGID, CHAR *pRCP)
{
   CHAR mMSG[256], mRCP[256];
   CHAR mMSG1[256], mRCP1[256];

    strcpy(mRCP, pRCP);
    strtok(mRCP, ".");
    strcpy(mMSG, mRCP);
    strcat(mRCP, "-S0001.RC$");
    strcat(mMSG, "-S0001.MSG");
    sprintf(mMSG1, "%s%s%s-S0001.MSG",mMailSpoolDir,mMailQueueDir,pMSGID);
#ifdef UPDATE_201012140 // 外部DISKの場合承認のため保留通知の移動に失敗してロックする
    MoveFileEx(mMSG, mMSG1,  MOVEFILE_COPY_ALLOWED | MOVEFILE_WRITE_THROUGH);
#else
    MoveFileEx(mMSG, mMSG1,  MOVEFILE_WRITE_THROUGH);
#endif
    sprintf(mRCP1, "%s%s%s-S0001.RCP",mMailSpoolDir,mMailQueueDir,pMSGID);
#ifdef UPDATE_201012140 // 外部DISKの場合承認のため保留通知の移動に失敗してロックする
    MoveFileEx(mRCP, mRCP1,   MOVEFILE_COPY_ALLOWED | MOVEFILE_WRITE_THROUGH);
#else
    MoveFileEx(mRCP, mRCP1,  MOVEFILE_WRITE_THROUGH);
#endif
}

// データをincomingフォルダに移動
void DeleteStaffMail(CHAR *pMSGID, CHAR *pRCP)
{
   CHAR mMSG[256], mRCP[256];
   CHAR mMSG1[256], mRCP1[256];

    strcpy(mRCP, pRCP);
    strtok(mRCP, ".");
    strcpy(mMSG, mRCP);
    strcat(mRCP, "-S0001.RC$");
    strcat(mMSG, "-S0001.MSG");
	DeleteFile(mRCP);
	DeleteFile(mMSG);
}
#endif

void MailToBoss(CHAR *pHostName, CHAR *pMSGID, CHAR *pMSGSource, CHAR *pMSG, CHAR *pRCP, CHAR *pFrom, CHAR *pTos, CHAR *pB64Tos, CHAR *pSubject)
{
#ifdef UPDATE_20080530 // 上長承認で複数上長宛の指定が一部宛先がロスとしてしまう
   char mLog[1024];
#endif
   char *p, *q, mt[128], itm[512], mwork[512], mRCP[256];
   BOOL bHead = TRUE, bSub = FALSE;
   SYSTEMTIME ltime;
   FILE *fpsrc, *fpd, *fp;
   int i;
#ifdef UPDATE_20080627 // 題名付加で承認否認メールの作成でハングする可能性
   CHAR mInfo[3][1024];  // 承認対象メール情報
#else
#ifdef UPDATE_20080517 // 上長宛承認メールの承認否認メールに対象メールの題名情報などを記録する
   CHAR mInfo[3][256];  // 承認対象メール情報
#endif
#endif
#ifdef UPDATE_20080514 // 上長宛承認メールのmailto:で記載した返信先アドレスのドメインを強制変更する設定
   char *r1, *r2, mAFrom[512] = "";
#endif
#ifdef UPDATE_20080630 // In-Reply-To,Referencesへの対応
  CHAR mID[512];
#endif
#ifdef UPDATE_20151118B // 上長承認対象メールのSubjectヘッダがEUC_JPでパックされていると文字化けする。
  CHAR *pSubjectCode;
#endif
#ifdef UPDATE_20240205 // 承認依頼メールで１行当たりの文字数を正規化
  int  n1, n2, n3;
  CHAR c1, mLine[8192];
#endif

#ifdef UPDATE_20080509 // 送信先アドレスを表示
   char mRcpt[512] = "";

   strcpy(mRcpt, pMSGSource);
   if (p = strrchr(mRcpt, '.')) {
	 *p ='\x0';
   }
   strcat(mRcpt, ".RCP");
#endif
#ifdef UPDATE_20080514 // 上長宛承認メールのmailto:で記載した返信先アドレスのドメインを強制変更する設定
   if (mMailApprovalDomain[0])
   {
     strcpy(mAFrom, pFrom);
     if ((r1 = strchr(mAFrom, '@'))) {
       r1++;
	   sprintf(r1, "%s.", mMailApprovalDomain);
	 }
	 if ((r2 = strchr(pFrom, '@'))) {
	   r2++;
	   strcat(mAFrom, r2);
	 }
   }
#endif

#ifdef UPDATE_20080630 // In-Reply-Toへの対応
     mID[0] = '\x0';
     GetInfo(_T("message-id:"), pMSGSource, mID, sizeof(mID));
#endif
#ifdef UPDATE_20080517 // 上長宛承認メールの承認否認メールに対象メールの題名情報などを記録する
     mInfo[0][0] = mInfo[1][0] = mInfo[2][0] = '\x0';
     GetInfo(_T("date:"), pMSGSource, mwork, sizeof(mwork));
     CODEPAGE2UTF8N(CP_ISO_JAPANESE, mwork, itm, sizeof(itm));
#ifdef UPDATE_20190109 // 承認メールをHTMLメールにしたときutf-8に変換
     if (bMailApprovalHtml)
     {
       EncodeString(itm, strlen(itm), mInfo[0]);
	 } else 
#endif
	 {
#ifdef UPDATE_20141210 // 承認依頼メールのmailtoリンクの文字コード指定
       UTF8N2CODEPAGE(nMailApprovalCodepage, itm, mwork, sizeof(mwork)); // iso-2022-jpに変換
#else
       UTF8N2CODEPAGE(CP_JAPANESE, itm, mwork, sizeof(mwork)); // iso-2022-jpに変換
#endif
       EncodeString(mwork, strlen(mwork), mInfo[0]);
	 }
     GetInfo(_T("from:"), pMSGSource, mwork, sizeof(mwork));
     CODEPAGE2UTF8N(CP_ISO_JAPANESE, mwork, itm, sizeof(itm));
#ifdef UPDATE_20190109 // 承認メールをHTMLメールにしたときutf-8に変換
     if (bMailApprovalHtml)
     {
       EncodeString(itm, strlen(itm), mInfo[1]);
	 } else 
#endif
	 {
#ifdef UPDATE_20141210 // 承認依頼メールのmailtoリンクの文字コード指定
       UTF8N2CODEPAGE(nMailApprovalCodepage, itm, mwork, sizeof(mwork)); // iso-2022-jpに変換
#else
       UTF8N2CODEPAGE(CP_JAPANESE, itm, mwork, sizeof(mwork)); // iso-2022-jpに変換
#endif
       EncodeString(mwork, strlen(mwork), mInfo[1]);
	 }
     GetInfo(_T("subject:"), pMSGSource, mwork, sizeof(mwork));
     CODEPAGE2UTF8N(CP_ISO_JAPANESE, mwork, itm, sizeof(itm));
#ifdef UPDATE_20190109 // 承認メールをHTMLメールにしたときutf-8に変換
     if (bMailApprovalHtml)
     {
       EncodeString(itm, strlen(itm), mInfo[2]);
	 } else 
#endif
	 {
#ifdef UPDATE_20141210 // 承認依頼メールのmailtoリンクの文字コード指定
       UTF8N2CODEPAGE(nMailApprovalCodepage, itm, mwork, sizeof(mwork)); // iso-2022-jpに変換
#else
       UTF8N2CODEPAGE(CP_JAPANESE, itm, mwork, sizeof(mwork)); // iso-2022-jpに変換
#endif
       EncodeString(mwork, strlen(mwork), mInfo[2]);
	 }
#endif
     gettime(&ltime, mt);
     strcpy(mRCP, pRCP);
     strtok(mRCP, ".");
     strcat(mRCP, ".RC$");
#ifdef UPDATE_20231017 // 上長承認依頼メールの作成に失敗した場合の作成リトライ
     i = 0;
	 while(!(fpd = fopen(pMSG, "wt")))
	 {
       if (bServiceTerminating || i > nMailApprovalMessNum)
	     break;
       i++;
	   _sleep(WAIT_TIME+i);
	 }
#ifdef UPDATE_20231026 // 上長承認依頼メールの作成に失敗した場合の作成リトライ
     if (fpd)
	 {
       sprintf(mLog, "MailToBoss() [%s] Make file success. retry=%d", pMSG, i);
	 } else { // ファイル作成できないとき
       sprintf(mLog, "MailToBoss() [%s] Make file fail. retry=%d, errno=%d", pMSG, i-1, GetLastError());
	 }
	 LEVEL_3_ACCEPTLOG(NULL, mLog);
	 if (fpd)
#else
	 if (!fpd) // ファイル作成できないとき
	 {
       sprintf(mLog, "MailToBoss() [%s] make file fail. retry=%d , errno=%d", pMSG, nMailApprovalMessNum, GetLastError());
       LEVEL_3_ACCEPTLOG(NULL, mLog);
	 } else
#endif
#else
     if ((fpd = fopen(pMSG, "wt"))) 
#endif
	 {
#ifdef UPDATE_20080630 // In-Reply-Toへの対応
       if (mID[0])
	   {
	     fprintf(fpd, "In-Reply-To: %s\n", mID); // 対応するメッセージIDをIn-Reply-To:ヘッダへ記載
         fprintf(fpd, "References: %s\n", mID); // 対応するメッセージIDをIn-Reply-To:ヘッダへ記載
	   }
#endif     	 
#ifdef UPDATE_20151118B // 上長承認対象メールのSubjectヘッダがEUC_JPでパックされていると文字化けする。
       if (!(pSubjectCode = strstr(pSubject, "=?")))
	   {
		 pSubjectCode = pSubject;
	   }
#endif
#ifdef UPDATE_20080204A // 題名がUNICODE utf-7/utf-8でのBossCheckで承認依頼メールの題名が文字化けする
#ifdef UPDATE_20151118B // 上長承認対象メールのSubjectヘッダがEUC_JPでパックされていると文字化けする。
	   if (!strnicmp(pSubjectCode, "=?utf-7", 7)) 
#else
	   if (!strnicmp(pSubject, "=?utf-7", 7)) 
#endif
	   { 
         fprintf(fpd, "Subject: =?utf-7?B?K1luK0tqVStkbUR3LQ==?=(%s)\n", pFrom); // 承認依頼
	   } 
#ifdef UPDATE_20151118B // 上長承認対象メールのSubjectヘッダがEUC_JPでパックされていると文字化けする。
	   else if (!strnicmp(pSubjectCode, "=?utf-8", 7)) 
#else
	   else if (!strnicmp(pSubject, "=?utf-8", 7))
#endif
	   { 
         fprintf(fpd, "Subject: =?utf-8?B?5om/6KqN5L6d6aC8?=(%s)\n", pFrom); // 承認依頼
	   } else
#endif
#ifdef UPDATE_20151118B // 上長承認対象メールのSubjectヘッダがEUC_JPでパックされていると文字化けする。
	   if (!strnicmp(pSubjectCode, "=?euc-jp", 8)) 
	   { 
         fprintf(fpd, "Subject: =?euc-jp?B?vrXHp7DNzeo=?=(%s)\n", pFrom); // 承認依頼
	   } 
	   else if (!strnicmp(pSubjectCode, "=?shift_jis", 11))
	   { 
         fprintf(fpd, "Subject: =?shift_jis?B?j7OURojLl4o=?=(%s)\n", pFrom); // 承認依頼
	   } else
#endif
	   {
         fprintf(fpd, "Subject: =?iso-2022-jp?B?GyRCPjVHJzBNTWobKEI=?=(%s)\n", pFrom); // 承認依頼
	   }
#ifdef UPDATE_20150807 // BOSSCheckで生成したSubject:ヘッダが１行結合されて出力しないようにした。
	   fprintf(fpd, "\t%s", pSubject);
#else
	   fprintf(fpd, "\t %s\n", pSubject);
#endif
#ifdef UPDATE_20240502 // 送信元が無い（NULL）場合の承認依頼通知メールのFrom:ヘッダにPostmasterアカウントを疑似的に追加する
	   fprintf(fpd, "From: %s%s\n", (*pFrom ? pFrom: "postmaster@"), (*pFrom ? "": pHostName));
#else
       fprintf(fpd, "From: %s\n", pFrom);
#endif
       fprintf(fpd, "To: %s\n", pTos);
       fprintf(fpd, "Date: %s\n", mt);
       fprintf(fpd, "Message-Id: <%s.1@%s>\n", pMSGID, pHostName); //pB64Tos);
       fprintf(fpd, "Content-Type: multipart/mixed;\n\tboundary=\"----%s%s\"\n", pMSGID, pB64Tos);
       fprintf(fpd, "\n");
       fprintf(fpd, "\n------%s%s\n", pMSGID, pB64Tos);
#ifdef UPDATE_20190108 // 承認メールをHTMLメールにする機能を追加
#ifdef UPDATE_20190109 // 承認メールをHTMLメールにしたときutf-8に変換
       fprintf(fpd, "Content-Type: text/%s; charset=\"%s\"\n\n%s", (bMailApprovalHtml ? "html" : "plain"), (bMailApprovalHtml ? "UTF-8" : "ISO-2022-JP"), (bMailApprovalHtml ? "<html><body>\n" : "")); 
#else
	   fprintf(fpd, "Content-Type: text/%s; charset=\"ISO-2022-JP\"\n\n%s", bMailApprovalHtml ? "html" : "plain", bMailApprovalHtml ? "<html><body>\n" : ""); 
#endif
#else
       fprintf(fpd, "Content-Type: text/plain; charset=\"ISO-2022-JP\"\n\n"); 
#endif
	   ////////////////////////////////////////////////////////
	   // メッセージ挿入
#ifdef UPDATE_20170125 // セキュアハンドラでのアウトバウンドメールの受信承認メッセージの修正
       if (bMailApprovalRevers && !CheckDomain(pFrom))
#ifdef UPDATE_20190108 // 承認メールをHTMLメールにする機能を追加
#ifdef UPDATE_20190109 // 承認メールをHTMLメールにしたときutf-8に変換
	     sprintf(mwork, "\x1B$B$3$N%%a!<%%k$O!\"E:IU%%U%%!%%$%%k$NFbMF$K$D$$$F<u?.>5G'$N2DH]\x1B(B%s\n", bMailApprovalHtml ? "<br>" : "");
#else
	     fprintf(fpd, "\x1B$B$3$N%%a!<%%k$O!\"E:IU%%U%%!%%$%%k$NFbMF$K$D$$$F<u?.>5G'$N2DH]\x1B(B%s\n", bMailApprovalHtml ? "<br>" : "");
#endif
#else
	     fprintf(fpd, "\x1B$B$3$N%%a!<%%k$O!\"E:IU%%U%%!%%$%%k$NFbMF$K$D$$$F<u?.>5G'$N2DH]\x1B(B\n");
#endif
	   else
#endif
#ifdef UPDATE_20190108 // 承認メールをHTMLメールにする機能を追加
#ifdef UPDATE_20190109 // 承認メールをHTMLメールにしたときutf-8に変換
		 sprintf(mwork, "\x1B$B$3$N%%a!<%%k$O!\"E:IU%%U%%!%%$%%k$NFbMF$K$D$$$FAw?.>5G'$N2DH]\x1B(B%s\n", (bMailApprovalHtml ? "<br>" : ""));
       if (bMailApprovalHtml)
	   {
         CODEPAGE2UTF8N(CP_ISO_JAPANESE, mwork, itm, sizeof(itm)); // utf-8に変換
#ifdef UPDATE_20240205 // 承認依頼メールで１行当たりの文字数を正規化
		 if (nNomalColumnMaxSize != 0)
           LineNormalization(fpd, nNomalColumnMaxSize, itm);
		 else 
#endif
       	 fputs(itm, fpd);
       } else {
       	 fputs(mwork, fpd); // JISのまま
       }
#else
	   fprintf(fpd, "\x1B$B$3$N%%a!<%%k$O!\"E:IU%%U%%!%%$%%k$NFbMF$K$D$$$FAw?.>5G'$N2DH]\x1B(B%s\n", (bMailApprovalHtml ? "<br>" : ""));
#endif
#ifdef UPDATE_20190109 // 承認メールをHTMLメールにしたときutf-8に変換
	   sprintf(mwork, "\x1B$B$rMW5a$9$k$b$N$G$9!#\x1B(B%s\n", bMailApprovalHtml ? "<br>" : "");
       if (bMailApprovalHtml)
	   {
         CODEPAGE2UTF8N(CP_ISO_JAPANESE, mwork, itm, sizeof(itm)); // utf-8に変換
#ifdef UPDATE_20240205 // 承認依頼メールで１行当たりの文字数を正規化
		 if (nNomalColumnMaxSize != 0)
           LineNormalization(fpd, nNomalColumnMaxSize, itm);
		 else 
#endif
       	 fputs(itm, fpd);
       } else {
       	 fputs(mwork, fpd); // JISのまま
       }
#else
	   fprintf(fpd, "\x1B$B$rMW5a$9$k$b$N$G$9!#\x1B(B%s\n", bMailApprovalHtml ? "<br>" : "");
#endif
#ifdef UPDATE_20190109 // 承認メールをHTMLメールにしたときutf-8に変換
	   sprintf(mwork, "\x1B$B2DH]$N%%\"%%/%%7%%g%%s$O0J2<$N%%j%%s%%/$r%%/%%j%%C%%/$7!\"I=<($5$l$?\x1B(B%s\n", (bMailApprovalHtml ? "<br>" : ""));
       if (bMailApprovalHtml)
	   {
         CODEPAGE2UTF8N(CP_ISO_JAPANESE, mwork, itm, sizeof(itm)); // utf-8に変換
#ifdef UPDATE_20240205 // 承認依頼メールで１行当たりの文字数を正規化
		 if (nNomalColumnMaxSize != 0)
           LineNormalization(fpd, nNomalColumnMaxSize, itm);
		 else 
#endif
       	 fputs(itm, fpd);
       } else {
       	 fputs(mwork, fpd); // JISのまま
       }
#else
	   fprintf(fpd, "\x1B$B2DH]$N%%\"%%/%%7%%g%%s$O0J2<$N%%j%%s%%/$r%%/%%j%%C%%/$7!\"I=<($5$l$?\x1B(B%s\n", (bMailApprovalHtml ? "<br>" : ""));
#endif
#ifdef UPDATE_20190109 // 承認メールをHTMLメールにしたときutf-8に変換
	   sprintf(mwork, "\x1B$B%%a!<%%k$rAw?.$9$k$3$H$G40N;$7$^$9!#\x1B(B%s\n", bMailApprovalHtml ? "<br>" : "");
       if (bMailApprovalHtml)
	   {
         CODEPAGE2UTF8N(CP_ISO_JAPANESE, mwork, itm, sizeof(itm)); // utf-8に変換
#ifdef UPDATE_20240205 // 承認依頼メールで１行当たりの文字数を正規化
		 if (nNomalColumnMaxSize != 0)
           LineNormalization(fpd, nNomalColumnMaxSize, itm);
		 else 
#endif
       	 fputs(itm, fpd);
       } else {
       	 fputs(mwork, fpd); // JISのまま
       }
#else
	   fprintf(fpd, "\x1B$B%%a!<%%k$rAw?.$9$k$3$H$G40N;$7$^$9!#\x1B(B%s\n", bMailApprovalHtml ? "<br>" : "");
#endif
#ifdef UPDATE_20190109 // 承認メールをHTMLメールにしたときutf-8に変換
	   sprintf(mwork, "%s\n", bMailApprovalHtml ? "<br>" : "");
       if (bMailApprovalHtml)
	   {
         CODEPAGE2UTF8N(CP_ISO_JAPANESE, mwork, itm, sizeof(itm)); // utf-8に変換
#ifdef UPDATE_20240205 // 承認依頼メールで１行当たりの文字数を正規化
		 if (nNomalColumnMaxSize != 0)
           LineNormalization(fpd, nNomalColumnMaxSize, itm);
		 else 
#endif
       	 fputs(itm, fpd);
       } else {
       	 fputs(mwork, fpd); // JISのまま
       }
#else
	   fprintf(fpd, "%s\n", bMailApprovalHtml ? "<br>" : "");
#endif
#else
       fprintf(fpd, "\x1B$B$3$N%%a!<%%k$O!\"E:IU%%U%%!%%$%%k$NFbMF$K$D$$$FAw?.>5G'$N2DH]\x1B(B\n");
	   fprintf(fpd, "\x1B$B$rMW5a$9$k$b$N$G$9!#\x1B(B\n");
	   fprintf(fpd, "\x1B$B2DH]$N%%\"%%/%%7%%g%%s$O0J2<$N%%j%%s%%/$r%%/%%j%%C%%/$7!\"I=<($5$l$?\x1B(B\n");
	   fprintf(fpd, "\x1B$B%%a!<%%k$rAw?.$9$k$3$H$G40N;$7$^$9!#\x1B(B\n");
	   fprintf(fpd, "\n");
#endif
#ifdef UPDATE_20190108 // 承認メールをHTMLメールにする機能を追加
#ifdef UPDATE_20190109 // 承認メールをHTMLメールにしたときutf-8に変換
	   sprintf(mwork, "[\x1B$B>5G'\x1B(B] %smailto:", (bMailApprovalHtml ? "<a href=\"" : "")); //[承認]
       if (bMailApprovalHtml)
	   {
         CODEPAGE2UTF8N(CP_ISO_JAPANESE, mwork, itm, sizeof(itm)); // utf-8に変換
#ifdef UPDATE_20240205 // 承認依頼メールで１行当たりの文字数を正規化
		 if (nNomalColumnMaxSize != 0)
           strcpy(mLine, itm);
		 else
#endif
       	 fputs(itm, fpd);
       } else {
       	 fputs(mwork, fpd); // JISのまま
       }
#else
	   fprintf(fpd, "[\x1B$B>5G'\x1B(B] %smailto:", (bMailApprovalHtml ? "<a href=\"" : "")); //[承認]
#endif
#ifdef UPDATE_20240205 // 承認依頼メールで１行当たりの文字数を正規化
	   if (nNomalColumnMaxSize != 0)
	   {
         sprintf(&mLine[strlen(mLine)], (mMailApprovalDomain[0] ? mAFrom : (bMailApprovalRevers  && !*pFrom ? pTos : pFrom)));
         sprintf(&mLine[strlen(mLine)], "%csubject=approval_", mMailApprovalMailTo[0]);//fprintf(fpd, "?subject=approval+");
	     sprintf(&mLine[strlen(mLine)], "%s", pMSGID);
	     sprintf(&mLine[strlen(mLine)], "%c", mMailApprovalMID[0]);
         sprintf(&mLine[strlen(mLine)], "%s", pB64Tos);
         sprintf(&mLine[strlen(mLine)], "&body=Date:%%20%s%%0AFrom:%%20%s%%0ASubject:%%20%s%s", mInfo[0], mInfo[1], mInfo[2], (bMailApprovalHtml ? "\">" : ""));
         LineNormalization(fpd, nNomalColumnMaxSize, mLine);
//printf("size(%d):%s", strlen(mLine), mLine);
	   } else {
#endif
       fprintf(fpd, (mMailApprovalDomain[0] ? mAFrom : (bMailApprovalRevers  && !*pFrom ? pTos : pFrom)));
       fprintf(fpd, "%csubject=approval_", mMailApprovalMailTo[0]);//fprintf(fpd, "?subject=approval+");
	   fprintf(fpd, pMSGID);
	   fprintf(fpd, "%c", mMailApprovalMID[0]);
       fprintf(fpd, pB64Tos);
       fprintf(fpd, "&body=Date:%%20%s%%0AFrom:%%20%s%%0ASubject:%%20%s%s", mInfo[0], mInfo[1], mInfo[2], (bMailApprovalHtml ? "\">" : ""));
#ifdef UPDATE_20240205 // 承認依頼メールで１行当たりの文字数を正規化
	   }
#endif
       ///////
	   if (bMailApprovalHtml)
	   {
#ifdef UPDATE_20240205 // 承認依頼メールで１行当たりの文字数を正規化
	   if (nNomalColumnMaxSize != 0)
	   {
		 fprintf(fpd, "\n");
	     sprintf(mLine, "mailto:"); //[承認]
         sprintf(&mLine[strlen(mLine)], (mMailApprovalDomain[0] ? mAFrom : (bMailApprovalRevers  && !*pFrom ? pTos : pFrom)));
         sprintf(&mLine[strlen(mLine)], "%csubject=approval_", mMailApprovalMailTo[0]);//fprintf(fpd, "?subject=approval+");
	     sprintf(&mLine[strlen(mLine)], "%s", pMSGID);
	     sprintf(&mLine[strlen(mLine)], "%c", mMailApprovalMID[0]);
         sprintf(&mLine[strlen(mLine)], "%s", pB64Tos);
         sprintf(&mLine[strlen(mLine)], "&body=Date:%%20%s%%0AFrom:%%20%s%%0ASubject:%%20%s</a><br>", mInfo[0], mInfo[1], mInfo[2]);
	     sprintf(&mLine[strlen(mLine)], "<br>\n<br>\n");
         LineNormalization(fpd, nNomalColumnMaxSize, mLine);
//printf("size(%d):%s", strlen(mLine), mLine);
	   } else {
#endif
	     fprintf(fpd, "mailto:"); //[承認]
         fprintf(fpd, (mMailApprovalDomain[0] ? mAFrom : (bMailApprovalRevers  && !*pFrom ? pTos : pFrom)));
         fprintf(fpd, "%csubject=approval_", mMailApprovalMailTo[0]);//fprintf(fpd, "?subject=approval+");
	     fprintf(fpd, pMSGID);
	     fprintf(fpd, "%c", mMailApprovalMID[0]);
         fprintf(fpd, pB64Tos);
         fprintf(fpd, "&body=Date:%%20%s%%0AFrom:%%20%s%%0ASubject:%%20%s</a><br>", mInfo[0], mInfo[1], mInfo[2]);
	     fprintf(fpd, "<br>\n<br>\n");
#ifdef UPDATE_20240205 // 承認依頼メールで１行当たりの文字数を正規化
	   }
#endif
	   } else {
	     fprintf(fpd, "\n\n");
	   }
       ///////
       if (bMailApprovalHtml)
	   {
#ifdef UPDATE_20190109 // 承認メールをHTMLメールにしたときutf-8に変換
	   sprintf(mwork, (bMailApprovalRevers && !CheckDomain(pFrom) ? "[\x1B$B5qH]\x1B(B] <a href=\"mailto:" : "[\x1B$B5Q2<\x1B(B] <a href=\"mailto:")); //[拒否] [却下]
       if (bMailApprovalHtml)
	   {
         CODEPAGE2UTF8N(CP_ISO_JAPANESE, mwork, itm, sizeof(itm)); // utf-8に変換
#ifdef UPDATE_20240205 // 承認依頼メールで１行当たりの文字数を正規化
		 if (nNomalColumnMaxSize != 0)
           strcpy(mLine, itm);
		 else
#endif
       	 fputs(itm, fpd);
       } else {
       	 fputs(mwork, fpd); // JISのまま
       }
#else
#ifdef UPDATE_20240205 // 承認依頼メールで１行当たりの文字数を正規化
	   if (nNomalColumnMaxSize != 0)
	     sprintf(mLine, (bMailApprovalRevers && !CheckDomain(pFrom) ? "[\x1B$B5qH]\x1B(B] <a href=\"mailto:" : "[\x1B$B5Q2<\x1B(B] <a href=\"mailto:")); //[拒否] [却下]
	   else
#endif
	   fprintf(fpd, (bMailApprovalRevers && !CheckDomain(pFrom) ? "[\x1B$B5qH]\x1B(B] <a href=\"mailto:" : "[\x1B$B5Q2<\x1B(B] <a href=\"mailto:")); //[拒否] [却下]
#endif
	   } else {
#ifdef UPDATE_20240205 // 承認依頼メールで１行当たりの文字数を正規化
	   if (nNomalColumnMaxSize != 0)
	     sprintf(mLine, (bMailApprovalRevers && !CheckDomain(pFrom) ? "[\x1B$B5qH]\x1B(B] mailto:" : "[\x1B$B5Q2<\x1B(B] mailto:")); //[拒否] [却下]
	   else
#endif
	     fprintf(fpd, (bMailApprovalRevers && !CheckDomain(pFrom) ? "[\x1B$B5qH]\x1B(B] mailto:" : "[\x1B$B5Q2<\x1B(B] mailto:")); //[拒否] [却下]
	   }
#ifdef UPDATE_20240205 // 承認依頼メールで１行当たりの文字数を正規化
	   if (nNomalColumnMaxSize != 0)
	   {
         sprintf(&mLine[strlen(mLine)], (mMailApprovalDomain[0] ? mAFrom : (bMailApprovalRevers  && !*pFrom ? pTos : pFrom)));
	     sprintf(&mLine[strlen(mLine)], "%csubject=reject_", mMailApprovalMailTo[0]); //fprintf(fpd, "?subject=reject+");
	     sprintf(&mLine[strlen(mLine)], "%s", pMSGID);
	     sprintf(&mLine[strlen(mLine)], "%c", mMailApprovalMID[0]);
         sprintf(&mLine[strlen(mLine)], "%s", pB64Tos);
         sprintf(&mLine[strlen(mLine)], "&body=Date:%%20%s%%0AFrom:%%20%s%%0ASubject:%%20%s%s", mInfo[0], mInfo[1], mInfo[2], (bMailApprovalHtml ? "\">" : ""));
         LineNormalization(fpd, nNomalColumnMaxSize, mLine);
//printf("size(%d):%s", strlen(mLine), mLine);
	   } else {
#endif
       fprintf(fpd, (mMailApprovalDomain[0] ? mAFrom : (bMailApprovalRevers  && !*pFrom ? pTos : pFrom)));
	   fprintf(fpd, "%csubject=reject_", mMailApprovalMailTo[0]); //fprintf(fpd, "?subject=reject+");
	   fprintf(fpd, pMSGID);
	   fprintf(fpd, "%c", mMailApprovalMID[0]);
       fprintf(fpd, pB64Tos);
       fprintf(fpd, "&body=Date:%%20%s%%0AFrom:%%20%s%%0ASubject:%%20%s%s", mInfo[0], mInfo[1], mInfo[2], (bMailApprovalHtml ? "\">" : ""));
#ifdef UPDATE_20240205 // 承認依頼メールで１行当たりの文字数を正規化
	   }
#endif
       ///////
	   if (bMailApprovalHtml)
	   {
#ifdef UPDATE_20240205 // 承認依頼メールで１行当たりの文字数を正規化
	   if (nNomalColumnMaxSize != 0)
	   {
		 fprintf(fpd, "\n");
	     sprintf(mLine, "mailto:"); //[拒否] [却下]
         sprintf(&mLine[strlen(mLine)], (mMailApprovalDomain[0] ? mAFrom : (bMailApprovalRevers  && !*pFrom ? pTos : pFrom)));
	     sprintf(&mLine[strlen(mLine)], "%csubject=reject_", mMailApprovalMailTo[0]); //fprintf(fpd, "?subject=reject+");
	     sprintf(&mLine[strlen(mLine)], "%s", pMSGID);
	     sprintf(&mLine[strlen(mLine)], "%c", mMailApprovalMID[0]);
         sprintf(&mLine[strlen(mLine)], "%s", pB64Tos);
         sprintf(&mLine[strlen(mLine)], "&body=Date:%%20%s%%0AFrom:%%20%s%%0ASubject:%%20%s</a><br>", mInfo[0], mInfo[1], mInfo[2]);
	     sprintf(&mLine[strlen(mLine)], "<br>\n<br>\n");
         LineNormalization(fpd, nNomalColumnMaxSize, mLine);
//printf("size(%d):%s", strlen(mLine), mLine);
	   } else {
#endif
	     fprintf(fpd, "mailto:"); //[拒否] [却下]
         fprintf(fpd, (mMailApprovalDomain[0] ? mAFrom : (bMailApprovalRevers  && !*pFrom ? pTos : pFrom)));
	     fprintf(fpd, "%csubject=reject_", mMailApprovalMailTo[0]); //fprintf(fpd, "?subject=reject+");
	     fprintf(fpd, pMSGID);
	     fprintf(fpd, "%c", mMailApprovalMID[0]);
         fprintf(fpd, pB64Tos);
         fprintf(fpd, "&body=Date:%%20%s%%0AFrom:%%20%s%%0ASubject:%%20%s</a><br>", mInfo[0], mInfo[1], mInfo[2]);
	     fprintf(fpd, "<br>\n<br>\n");
#ifdef UPDATE_20240205 // 承認依頼メールで１行当たりの文字数を正規化
	   }
#endif
	   } else {
	     fprintf(fpd, "\n\n");
	   }
#else
	   fprintf(fpd, "[\x1B$B>5G'\x1B(B] mailto:"); //[承認]
#ifdef UPDATE_20080514 // 上長宛承認メールのmailto:で記載した返信先アドレスのドメインを強制変更する設定
#ifdef UPDATE_20161112 // 送信先制限にエンベロープの送信元と日付ヘッダ中のトークンを指定可能にした。
       fprintf(fpd, (mMailApprovalDomain[0] ? mAFrom : (bMailApprovalRevers  && !*pFrom ? pTos : pFrom)));
#else
       fprintf(fpd, (mMailApprovalDomain[0] ? mAFrom : pFrom));
#endif
#else
	   fprintf(fpd, pFrom);
#endif
#ifdef UPDATE_20090213 // 上長承認メールのmailto:リンクの区切り記号を"?"以外に変更するオプションを追加
       fprintf(fpd, "%csubject=approval_", mMailApprovalMailTo[0]);//fprintf(fpd, "?subject=approval+");
#else
	   fprintf(fpd, "?subject=approval_");//fprintf(fpd, "?subject=approval+");
#endif
	   fprintf(fpd, pMSGID);
#ifdef UPDATE_20090213 // 上長承認メールのmailto:リンクの区切り記号を"?"以外に変更するオプションを追加
	   fprintf(fpd, "%c", mMailApprovalMID[0]);
#else
	   fprintf(fpd, "@");
#endif
       fprintf(fpd, pB64Tos);
#ifdef UPDATE_20080517 // 上長宛承認メールの承認否認メールに対象メールの題名情報などを記録する
       fprintf(fpd, "&body=Date:%%20%s%%0AFrom:%%20%s%%0ASubject:%%20%s", mInfo[0], mInfo[1], mInfo[2]);
#endif
	   fprintf(fpd, "\n\n");
	   fprintf(fpd, (bMailApprovalRevers && !CheckDomain(pFrom) ? "[\x1B$B5qH]\x1B(B] mailto:" : "[\x1B$B5Q2<\x1B(B] mailto:")); //[拒否] [却下]
#ifdef UPDATE_20080514 // 上長宛承認メールのmailto:で記載した返信先アドレスのドメインを強制変更する設定
#ifdef UPDATE_20161112 // 送信先制限にエンベロープの送信元と日付ヘッダ中のトークンを指定可能にした。
       fprintf(fpd, (mMailApprovalDomain[0] ? mAFrom : (bMailApprovalRevers  && !*pFrom ? pTos : pFrom)));
#else
       fprintf(fpd, (mMailApprovalDomain[0] ? mAFrom : pFrom));
#endif
#else
	   fprintf(fpd, pFrom);
#endif
#ifdef UPDATE_20090213 // 上長承認メールのmailto:リンクの区切り記号を"?"以外に変更するオプションを追加
	   fprintf(fpd, "%csubject=reject_", mMailApprovalMailTo[0]); //fprintf(fpd, "?subject=reject+");
#else
	   fprintf(fpd, "?subject=reject_"); //fprintf(fpd, "?subject=reject+");
#endif
	   fprintf(fpd, pMSGID);
#ifdef UPDATE_20090213 // 上長承認メールのmailto:リンクの区切り記号を"?"以外に変更するオプションを追加
	   fprintf(fpd, "%c", mMailApprovalMID[0]);
#else
	   fprintf(fpd, "@");
#endif
       fprintf(fpd, pB64Tos);
#ifdef UPDATE_20080517 // 上長宛承認メールの承認否認メールに対象メールの題名情報などを記録する
       fprintf(fpd, "&body=Date:%%20%s%%0AFrom:%%20%s%%0ASubject:%%20%s", mInfo[0], mInfo[1], mInfo[2]);
#endif
	   fprintf(fpd, "\n\n");
#endif
	   ////////////////////////////////////////////////////////
#ifdef UPDATE_20080509 // 送信先アドレスを表示
#ifdef UPDATE_20190108 // 承認メールをHTMLメールにする機能を追加
#ifdef UPDATE_20190109 // 承認メールをHTMLメールにしたときutf-8に変換
	   sprintf(mwork, "[\x1B$BAw?.@h%%\"%%I%%l%%9\x1B(B]%s\n", bMailApprovalHtml ? "<br>" : ""); //[送信先アドレス]
       if (bMailApprovalHtml)
	   {
         CODEPAGE2UTF8N(CP_ISO_JAPANESE, mwork, itm, sizeof(itm)); // utf-8に変換
       	 fputs(itm, fpd);
       } else {
       	 fputs(mwork, fpd); // JISのまま
       }
#else
       fprintf(fpd, "[\x1B$BAw?.@h%%\"%%I%%l%%9\x1B(B]%s\n", bMailApprovalHtml ? "<br>" : ""); //[送信先アドレス]
#endif
#else
       fprintf(fpd, "[\x1B$BAw?.@h%%\"%%I%%l%%9\x1B(B]\n"); //[送信先アドレス]
#endif
	   while(!(fpsrc = fopen(mRcpt, "rt"))) {
         if (bServiceTerminating)
		   break;
         _sleep(WAIT_TIME);
	   }
       if (fpsrc) {
		 fgets(mwork, sizeof(mwork), fpsrc);
		 fgets(mwork, sizeof(mwork), fpsrc);
         while (p = fgets(mwork, sizeof(mwork), fpsrc)) {
	       fprintf(fpd, "%s", &mwork[11]); // エンベロープの送信先
		 }
         fclose(fpsrc);
	   }
#ifdef UPDATE_20190108 // 承認メールをHTMLメールにする機能を追加
       fprintf(fpd, "%s\n\n", (bMailApprovalHtml ? "<br>\n</body></html>" : "")); 
#else
	   fprintf(fpd, "\n\n");
#endif
#endif
	   ////////////////////////////////////////////////////////
       fprintf(fpd, "\n------%s%s\n", pMSGID, pB64Tos);
       fprintf(fpd, "Content-Type: message/rfc822\n\n"); 
#ifdef UPDATE_20230602 // 上長承認、セキュアハンドラへの通知メールに添付する元メールに特殊なバイナリコードが含まれていると正しく添付が出来ない不具合。
       if ((fpsrc = fopen(pMSGSource, "rb"))) 
#else
       if ((fpsrc = fopen(pMSGSource, "rt"))) 
#endif
	   {
         while (p = fgets(mwork, sizeof(mwork), fpsrc)) 
		 {
#ifdef UPDATE_20230602 // 上長承認、セキュアハンドラへの通知メールに添付する元メールに特殊なバイナリコードが含まれていると正しく添付が出来ない不具合。
           if (mwork[0] != '.' && (mwork[1] != '\r' || mwork[1] != '\n'))
		   {
			 if (mwork[0] == '\r' ||  mwork[0] == '\n') {
			   fputs("\n", fpd);
			 } else if (strtok(mwork, "\r\n")) {
			   fputs(mwork, fpd); fputs("\n", fpd);
			 } else {
               fputs(mwork, fpd);
			 }
		   }
#else
           if (mwork[0] != '.' && mwork[1] != '\n')
             fputs(mwork, fpd);
#endif
		 }
#ifdef UPDATE_20231017A // 上長承認依頼メールの作成に失敗した場合のファイルクローズ不具合
         fclose(fpsrc);
#endif
	   }
       fprintf(fpd, "\n------%s%s--\n", pMSGID, pB64Tos);
       fprintf(fpd, "\n.\n");
       fflush(fpd);
       fclose(fpd);
#ifndef UPDATE_20231017A // 上長承認依頼メールの作成に失敗した場合のファイルクローズ不具合
       fclose(fpsrc);
#endif
	 }
#ifdef UPDATE_20231017 // 上長承認依頼メールの作成に失敗した場合の作成リトライ
     i = 0;
	 while(!(fpd = fopen(mRCP, "wt")))
	 {
       if (bServiceTerminating || i > nMailApprovalMessNum)
	     break;
       i++;
	   _sleep(WAIT_TIME+i);
	 }
#ifdef UPDATE_20231026 // 上長承認依頼メールの作成に失敗した場合の作成リトライ
     if (fpd)
	 {
       sprintf(mLog, "MailToBoss() [%s] Make file success. retry=%d", mRCP, i);
	 } else { // ファイル作成できないとき
       sprintf(mLog, "MailToBoss() [%s] Make file fail. retry=%d , errno=%d", mRCP, i-1, GetLastError());
	 }
	 LEVEL_3_ACCEPTLOG(NULL, mLog);
	 if (fpd)
#else
	 if (!fpd) // ファイル作成できないとき
	 {
       sprintf(mLog, "MailToBoss() [%s] make file fail. retry=%d , errno=%d", mRCP, nMailApprovalMessNum, GetLastError());
       LEVEL_3_ACCEPTLOG(NULL, mLog);
	 } else
#endif
#else
     if ((fpd =  fopen(mRCP, "wt"))) 
#endif
	 {
       fprintf(fpd,"Message-ID: <%s@%s>\n",pMSGID, pHostName);
#ifdef UPDATE_20170930 // 逆上長承認で承認者への送信元エンベロープが不正アドレス等で送信先に受信拒絶されないようにする対策
	   if (!bMailApprovalWebOnly && bMailApprovalRevers)
	   {
         fprintf(fpd, "Return-path: %s\n", (nMailApprovalReversType == 0 ?  "" : (nMailApprovalReversType == 1 ? pFrom : mMailApprovalReversFrom)));
	   } else 
#endif
	   {
	     fprintf(fpd, "Return-path: %s\n", (bMailApprovalWebOnly ? "" : pFrom));
	   }
       p = pTos;
       while(*p) {
         if ((q = strchr(p, ','))) {
           *q = '\x0';
		 } 
         fprintf(fpd, "Recipient: %s\n", (bMailApprovalWebOnly ? "" : p));
#ifdef UPDATE_20080530 // 上長承認で複数上長宛の指定が一部宛先がロスとしてしまう
         sprintf(mLog, "MailToBoss() RCP File Set: pMSGID=%s, To Boss=%s", pMSGID, p);
         LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
         if (q) {
#ifdef UPDATE_20080903 // １メール送信ごとにセッションを切らずに連続送信すると２通目以降は片方の承認者のみにしか送信できなくなる不具合
           *q = ',';
#endif
           p = q + 1;
		 } else {
		   break;
		 }
	   }
	   fflush(fpd);
	   fclose(fpd);
	 }
     i = 0;
     while(!(fp = fopen(pMSG, "rt"))) {
#ifdef UPDATE_20231017 // 上長承認依頼メールの作成に失敗した場合の作成リトライ
       if (bServiceTerminating || i > nMailApprovalMessNum)
         break;
       i++;
	   _sleep(WAIT_TIME+i);
#else
#ifdef UPDATE_20080530 // 上長承認で複数上長宛の指定が一部宛先がロスとしてしまう
       if (bServiceTerminating)
	     break;
#else
       if (bServiceTerminating || i > 30)
         break;
       i++;
#endif
	   _sleep(WAIT_TIME);
#endif
     }
#ifdef UPDATE_20231026 // 上長承認依頼メールの作成に失敗した場合の作成リトライ
	 if (fp) {
       sprintf(mLog, "MailToBoss() [%s] Open file success. retry=%d", pMSG, i);
       fclose(fp);
	 } else {
       sprintf(mLog, "MailToBoss() [%s] Open file fail. retry=%d , errno=%d", pMSG, i-1, GetLastError());
	 }
     LEVEL_3_ACCEPTLOG(NULL, mLog);
#else
     if (fp) 
       fclose(fp);
#endif
     i = 0;
     while(!(fp = fopen(mRCP, "rt"))) {
#ifdef UPDATE_20231017 // 上長承認依頼メールの作成に失敗した場合の作成リトライ
       if (bServiceTerminating || i > nMailApprovalMessNum)
         break;
       i++;
	   _sleep(WAIT_TIME+i);
#else
#ifdef UPDATE_20080530 // 上長承認で複数上長宛の指定が一部宛先がロスとしてしまう
       if (bServiceTerminating)
	     break;
#else
       if (bServiceTerminating || i > 30)
	     break;
       i++;
#endif
	   _sleep(WAIT_TIME);
#endif
	 }
#ifdef UPDATE_20231026 // 上長承認依頼メールの作成に失敗した場合の作成リトライ
	 if (fp) {
       sprintf(mLog, "MailToBoss() [%s] Open file success. retry=%d", mRCP, i);
       fclose(fp);
	 } else {
       sprintf(mLog, "MailToBoss() [%s] Open file fail. retry=%d, errno=%d", mRCP, i-1, GetLastError());
	 }
     LEVEL_3_ACCEPTLOG(NULL, mLog);
#else
     if (fp) 
       fclose(fp);
#endif
#ifdef UPDATE_20080530 // 上長承認で複数上長宛の指定が一部宛先がロスとしてしまう
     MoveFileEx(mRCP, pRCP,  MOVEFILE_COPY_ALLOWED | MOVEFILE_WRITE_THROUGH);
#else
     MoveFileEx(mRCP, pRCP,  MOVEFILE_WRITE_THROUGH);
#endif
#ifdef UPDATE_20231017 // 上長承認依頼メールの作成に失敗した場合の作成リトライ
	 i = 0;
     while(!(fp = fopen(pRCP, "rt"))) { // 移動が成功しているか確認
       if (bServiceTerminating || i > nMailApprovalMessNum)
         break;
       i++;
	   _sleep(WAIT_TIME+i);
	 }
	 if (fp) {
       sprintf(mLog, "MailToBoss() [%s] Open file success. retry=%d", pRCP, i);
       fclose(fp);
	 } else {
       sprintf(mLog, "MailToBoss() [%s] Open file fail. retry=%d, errno=%d", pRCP, i-1, GetLastError());
	 }
     LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
}

#ifdef UPDATE_20140623A // 上長承認結果ログに送信元IPの記録を追加した。
void MailToApproval(CHAR *pBDir, CHAR *pIP, CHAR *pMSG, CHAR *pRCP, CHAR *pFrom, CHAR *pTo, CHAR *pCc, CHAR *pSub)
#else
#ifdef UPDATE_20080620 // TOアドレスが代理人になる機能追加
void MailToApproval(CHAR *pBDir, CHAR *pMSG, CHAR *pRCP, CHAR *pFrom, CHAR *pTo, CHAR *pCc, CHAR *pSub)
#else
#ifdef UPDATE_20070713 // 承認者の代理人を設定するオプション
void MailToApproval(CHAR *pBDir, CHAR *pMSG, CHAR *pRCP, CHAR *pFrom, CHAR *pSub)
#else
void MailToApproval(CHAR *pMSG, CHAR *pRCP, CHAR *pFrom, CHAR *pSub)
#endif
#endif
#endif
{
   CHAR *p, *q;
   CHAR mSub[512], mwork[512], mFrom[256];
   CHAR mMSGFn[256], mRCPFn[256];
   CHAR mMSGDestFn[256], mRCPDestFn[256];
   CHAR mBoss[1024];
   CHAR mPack[512];
#ifdef UPDATE_20080620 // TOアドレスが代理人になる機能追加
   BOOL bChange = FALSE;
   CHAR *pAPTo, mTo[512];
   CHAR *pAPCc, mCc[512];
#endif
   BOOL bSub = FALSE;
   SYSTEMTIME ltime;
   FILE *fpsrc, *fpd, *fp;
   int i;
#ifdef UPDATE_20090121 // 上長承認保留ファイル名称の生成ルール変更
   CHAR mADRFn[256]; // 常駐アドレス保管ファイル
   CHAR mData[1024] = "";
#endif
   CHAR mLog[1024];
   DWORD nErr;
#ifdef UPDATE_20090701 // 代理承認処理メールの挿入文の書換えを可能に
   CHAR mProxyAddr[1024] = "";
#endif
#ifdef UPDATE_20091126 // 承認、否承認を一括処理するためのワイルドカード指定を可能にした。
  HANDLE          hF;
  WIN32_FIND_DATA FD;
  BOOL bFile = TRUE;
  CHAR mFn[256], mFn2[256];
#endif

   LineConv(mSub, sizeof(mSub), pSub , mPack);
   //strcpy(mSub, pSub);
   strtok(mSub, "\r\n");
   if (!strnicmp(mSub, "approval_", 9)) {//if (!strncmp(mSub, "approval+", 9)) {
#ifdef UPDATE_20091126 // 承認、否承認を一括処理するためのワイルドカード指定を可能にした。
     sprintf(mFn, "%s%s\\%s.MSG", mMailSpoolDir, mMailApprovalDir, &mSub[9]);
	 hF = FindFirstFile(mFn, &FD);
     if (hF != INVALID_HANDLE_VALUE) 
	 {
       while (bFile) 
	   {
         strcpy(mFn2, FD.cFileName);
		 mFn2[strlen(mFn2)-4] = '\x0';
#endif
#ifdef UPDATE_20090121 // 上長承認保留ファイル名称の生成ルール変更
#ifdef UPDATE_20091203 // 承認、否承認を一括処理するためのワイルドカード指定機能のON/OFFオプションを追加した。
     sprintf(mADRFn, "%s%s\\%s.ADR", mMailSpoolDir, mMailApprovalDir, (bMailApprovalWildcard ? mFn2 : &mSub[9]));
#else
#ifdef UPDATE_20091126 // 承認、否承認を一括処理するためのワイルドカード指定を可能にした。
     sprintf(mADRFn, "%s%s\\%s.ADR", mMailSpoolDir, mMailApprovalDir, mFn2);
#else
     sprintf(mADRFn, "%s%s\\%s.ADR", mMailSpoolDir, mMailApprovalDir, &mSub[9]);
#endif
#endif
	 if ((fp = fopen(mADRFn, "rt")))
	 {
	   fgets(mData, sizeof(mData), fp);
	   strtok(mData, "\r\n");
	   fclose(fp);
	 } else {
	   mData[0] = '\x0';
	 }
#endif
#ifdef UPDATE_20091203 // 承認、否承認を一括処理するためのワイルドカード指定機能のON/OFFオプションを追加した。
     sprintf(mMSGFn, "%s%s\\%s.MSG", mMailSpoolDir, mMailApprovalDir,(bMailApprovalWildcard ? mFn2 : &mSub[9]));
#else
#ifdef UPDATE_20091126 // 承認、否承認を一括処理するためのワイルドカード指定を可能にした。
     sprintf(mMSGFn, "%s%s\\%s.MSG", mMailSpoolDir, mMailApprovalDir, mFn2);
#else
     sprintf(mMSGFn, "%s%s\\%s.MSG", mMailSpoolDir, mMailApprovalDir, &mSub[9]);
#endif
#endif
	 if ((fp = fopen(mMSGFn, "rt")))
	 {
	   fclose(fp);
#ifdef UPDATE_20091203 // 承認、否承認を一括処理するためのワイルドカード指定機能のON/OFFオプションを追加した。
       sprintf(mRCPFn, "%s%s\\%s.RCP", mMailSpoolDir, mMailApprovalDir, (bMailApprovalWildcard ? mFn2 : &mSub[9]));
#else
#ifdef UPDATE_20091126 // 承認、否承認を一括処理するためのワイルドカード指定を可能にした。
       sprintf(mRCPFn, "%s%s\\%s.RCP", mMailSpoolDir, mMailApprovalDir, mFn2);
       if ((p = strchr(mFn2, mMailApprovalMID[0]))) 
#else
	   sprintf(mRCPFn, "%s%s\\%s.RCP", mMailSpoolDir, mMailApprovalDir, &mSub[9]);
       if ((p = strchr(&mSub[9], '@'))) 
#endif
#endif
	   {
	     p++;
#ifdef UPDATE_20090121 // 上長承認保留ファイル名称の生成ルール変更
		 if (mData[0])
		 {
		   p = mData;
		 }
#endif
         Base64DecodeLine(p, strlen(p), (unsigned char *)mBoss, sizeof(mBoss));
		 strtok(mBoss, " ");
	     strlwr(mBoss);
	     strcpy(mFrom, pFrom);
	     strlwr(mFrom);

	     if (strstr(mBoss, mFrom) || strstr(mMailApprovalManager, mFrom)) 
		 {
#ifdef UPDATE_20161112 // 送信先制限にエンベロープの送信元と日付ヘッダ中のトークンを指定可能にした。
	       if (bMailApprovalRevers)
		   {
	  	     SetMailApprovalReversAddr(pTo, mMSGFn, TRUE);
             if(bMailApprovalRejectRes)  // 却下通知の有効有無
			 {
		       if ((fp = fopen(pRCP, "wt"))) // 送信先を空にする
			   {
				 fprintf(fp, "Message-ID: <%s>\n", &mSub[9]);
                 fprintf(fp, "Return-path: \n");
                 fprintf(fp, "Recipient: \n");
		         fclose(fp);
			   }
			 }
			 //ApprovalLog(TRUE, pIP, mFrom, &mSub[9]); // 上長承認結果ログに送信元IPの記録を追加した。
		   } 
#endif
		   {
#ifdef UPDATE_20091203 // 承認、否承認を一括処理するためのワイルドカード指定機能のON/OFFオプションを追加した。
             sprintf(mMSGDestFn, "%stemp\\%s.MS$", mMailSpoolDir, (bMailApprovalWildcard ? mFn2 : &mSub[9]));
             MakeEML(pFrom, mMSGFn, pMSG, mMSGDestFn, (bMailApprovalWildcard ? mFn2 : &mSub[9]), 1);
#else
#ifdef UPDATE_20091126 // 承認、否承認を一括処理するためのワイルドカード指定を可能にした。
             sprintf(mMSGDestFn, "%stemp\\%s.MS$", mMailSpoolDir, mFn2);
             MakeEML(pFrom, mMSGFn, pMSG, mMSGDestFn, mFn2, 1);
#else
             sprintf(mMSGDestFn, "%stemp\\%s.MS$", mMailSpoolDir, &mSub[9]);
             MakeEML(pFrom, mMSGFn, pMSG, mMSGDestFn, &mSub[9], 1);
#endif
#endif
             CopyFile(mMSGDestFn, pMSG, FALSE);
		     while(!(fp = fopen(pMSG, "rt"))) {
               if (bServiceTerminating)
		         break;
	           _sleep(WAIT_TIME);
			 }
		     fclose(fp);
	         //DeleteFile(mMSGDestFn);
		     while(!DeleteFile(mMSGDestFn)) 
			 {
	           nErr = GetLastError();
	           if (nErr == ERROR_FILE_NOT_FOUND)
		         break;
               sprintf(mLog, "DeleteFile(%s) Fail. code=(%d) retry delete.", mMSGDestFn, nErr);
               LEVEL_3_ACCEPTLOG(NULL, mLog);
		       _sleep(WAIT_TIME);
			 }
#ifdef UPDATE_20091203 // 承認、否承認を一括処理するためのワイルドカード指定機能のON/OFFオプションを追加した。
             sprintf(mMSGDestFn, "%s%s%s.MSG", mMailSpoolDir, mMailQueueDir, (bMailApprovalWildcard ? mFn2 : &mSub[9]));
 	         sprintf(mRCPDestFn, "%s%s%s.RCP", mMailSpoolDir, mMailQueueDir, (bMailApprovalWildcard ? mFn2 : &mSub[9]));
#else
#ifdef UPDATE_20091126 // 承認、否承認を一括処理するためのワイルドカード指定を可能にした。
             sprintf(mMSGDestFn, "%s%s%s.MSG", mMailSpoolDir, mMailQueueDir, mFn2);
 	         sprintf(mRCPDestFn, "%s%s%s.RCP", mMailSpoolDir, mMailQueueDir, mFn2);
#else
             sprintf(mMSGDestFn, "%s%s%s.MSG", mMailSpoolDir, mMailQueueDir, &mSub[9]);
 	         sprintf(mRCPDestFn, "%s%s%s.RCP", mMailSpoolDir, mMailQueueDir, &mSub[9]);
#endif
#endif
	         // メールを戻す。
             CopyFile(pMSG, mMSGDestFn, FALSE);
		     while(!(fp = fopen(mMSGDestFn, "rt"))) {
               if (bServiceTerminating)
 		         break;
	           _sleep(WAIT_TIME);
			 }
		     fclose(fp);
#ifdef UPDATE_20080711 // 承認したことを、他の上長にも通知
		     AddToBossAddrress(mFrom, mBoss, pRCP);
#endif
             CopyFile(pRCP, mRCPDestFn, FALSE);
		     while(!(fp = fopen(mRCPDestFn, "rt")))
			 {
               if (bServiceTerminating)
		         break;
	           _sleep(WAIT_TIME);
			 }
		     fclose(fp);
   		     X_DateChengeFile(mMSGFn, pMSG); // date:ヘッダを変更する
		     while(!(fp = fopen(pMSG, "rt"))) {
               if (bServiceTerminating)
		         break;
	           _sleep(WAIT_TIME);
			 }
		     fclose(fp);
             while(!CopyFile(mRCPFn, pRCP, FALSE)) {
               if (bServiceTerminating)
		         break;
	           _sleep(WAIT_TIME);
			 }
		     while(!(fp = fopen(pRCP, "rt"))) {
               if (bServiceTerminating)
 		         break;
	           _sleep(WAIT_TIME);
			 }
		     fclose(fp);
#ifdef UPDATE_20090121 // 上長承認保留ファイル名称の生成ルール変更
		     while(!DeleteFile(mADRFn)) {
	           nErr = GetLastError();
	           if (nErr == ERROR_FILE_NOT_FOUND)
		          break;
               sprintf(mLog, "DeleteFile(%s) Fail. code=(%d) retry delete.", mADRFn, nErr);
               LEVEL_3_ACCEPTLOG(NULL, mLog);
		       _sleep(WAIT_TIME);
			 }
#endif
#ifdef UPDATE_20140623 // 上長承認結果ログに送信先アドレスの記録を追加した。
		     if (bMailApprovalLog)
			 {
#ifdef UPDATE_20091203 // 承認、否承認を一括処理するためのワイルドカード指定機能のON/OFFオプションを追加した。
               ApprovalLog(TRUE, mFrom, (bMailApprovalWildcard ? mFn2 : &mSub[9]));
#else
#ifdef UPDATE_20091126 // 承認、否承認を一括処理するためのワイルドカード指定を可能にした。
               ApprovalLog(TRUE, mFrom, mFn2);
#else
#ifdef UPDATE_20140623A // 上長承認結果ログに送信元IPの記録を追加した。
			   ApprovalLog(TRUE, pIP, mFrom, &mSub[9]);
#else
               ApprovalLog(TRUE, mFrom, &mSub[9]);
#endif
#endif
#endif
			 }
#endif
		   }
#ifdef UPDATE_20071220A // 承認処理済で削除がされないことがある。
		   while(!DeleteFile(mMSGFn)) {
	         nErr = GetLastError();
	         if (nErr == ERROR_FILE_NOT_FOUND)
		        break;
             sprintf(mLog, "DeleteFile(%s) Fail. code=(%d) retry delete.", mMSGFn, nErr);
             LEVEL_3_ACCEPTLOG(NULL, mLog);
		     _sleep(WAIT_TIME);
		   }
		   while(!DeleteFile(mRCPFn)) {
	         nErr = GetLastError();
	         if (nErr == ERROR_FILE_NOT_FOUND)
		        break;
             sprintf(mLog, "DeleteFile(%s) Fail. code=(%d) retry delete.", mRCPFn, nErr);
             LEVEL_3_ACCEPTLOG(NULL, mLog);
		     _sleep(WAIT_TIME);
		   }
#else
	       DeleteFile(mMSGFn);
	       DeleteFile(mRCPFn);
#endif
#ifndef UPDATE_20140623 // 上長承認結果ログに送信先アドレスの記録を追加した。
		   if (bMailApprovalLog)
		   {
#ifdef UPDATE_20091203 // 承認、否承認を一括処理するためのワイルドカード指定機能のON/OFFオプションを追加した。
             ApprovalLog(TRUE, mFrom, (bMailApprovalWildcard ? mFn2 : &mSub[9]));
#else
#ifdef UPDATE_20091126 // 承認、否承認を一括処理するためのワイルドカード指定を可能にした。
             ApprovalLog(TRUE, mFrom, mFn2);
#else
#ifdef UPDATE_20140623A // 上長承認結果ログに送信元IPの記録を追加した。
             ApprovalLog(TRUE, pIP, mFrom,  &mSub[9]);
#else
             ApprovalLog(TRUE, mFrom, &mSub[9]);
#endif
#endif
#endif
		   }
#endif
		 }
	   }
	 } else {
#ifdef UPDATE_20080620 // TOアドレスが代理人になる機能追加
       X_ReturnCTLMail(0, pFrom, NULL, pMSG, NULL);
#else
       X_ReturnCTLMail(0, pFrom, pMSG, NULL);
#endif
	 }
#ifdef UPDATE_20091126 // 承認、否承認を一括処理するためのワイルドカード指定を可能にした。
          bFile = FindNextFile( hF, &FD);
        }
        FindClose( hF ); 
     }
#endif
   } else if (!strnicmp(mSub, "reject_", 7)) { //} else if (!strncmp(mSub, "reject+", 7)) {
#ifdef UPDATE_20091126 // 承認、否承認を一括処理するためのワイルドカード指定を可能にした。
     sprintf(mFn, "%s%s\\%s.MSG", mMailSpoolDir, mMailApprovalDir, &mSub[7]);
	 hF = FindFirstFile(mFn, &FD);
     if (hF != INVALID_HANDLE_VALUE) 
	 {
       while (bFile) 
	   {
         strcpy(mFn2, FD.cFileName);
		 mFn2[strlen(mFn2)-4] = '\x0';
#endif
#ifdef UPDATE_20090121 // 上長承認保留ファイル名称の生成ルール変更
#ifdef UPDATE_20091203 // 承認、否承認を一括処理するためのワイルドカード指定機能のON/OFFオプションを追加した。
     sprintf(mADRFn, "%s%s\\%s.ADR", mMailSpoolDir, mMailApprovalDir, (bMailApprovalWildcard ? mFn2 : &mSub[7]));
#else
#ifdef UPDATE_20091126 // 承認、否承認を一括処理するためのワイルドカード指定を可能にした。
     sprintf(mADRFn, "%s%s\\%s.ADR", mMailSpoolDir, mMailApprovalDir, mFn2);
#else
     sprintf(mADRFn, "%s%s\\%s.ADR", mMailSpoolDir, mMailApprovalDir, &mSub[7]);
#endif
#endif
	 if ((fp = fopen(mADRFn, "rt")))
	 {
	   fgets(mData, sizeof(mData), fp);
	   strtok(mData, "\r\n");
	   fclose(fp);
	 } else {
	   mData[0] = '\x0';
	 }
#endif
#ifdef UPDATE_20091203 // 承認、否承認を一括処理するためのワイルドカード指定機能のON/OFFオプションを追加した。
	 if ((p = strchr((bMailApprovalWildcard ? mFn2 : &mSub[7]), mMailApprovalMID[0]))) 
#else
#ifdef UPDATE_20091126 // 承認、否承認を一括処理するためのワイルドカード指定を可能にした。
	 if ((p = strchr(mFn2, mMailApprovalMID[0]))) 
#else
	 if ((p = strchr(&mSub[7], '@'))) 
#endif
#endif
	 {
	   p++;
#ifdef UPDATE_20090121 // 上長承認保留ファイル名称の生成ルール変更
	   if (mData[0])
	   {
	     p = mData;
	   }
#endif
       Base64DecodeLine(p, strlen(p), (unsigned char *)mBoss, sizeof(mBoss));
	   strtok(mBoss, " ");
	   strlwr(mBoss);
	   strcpy(mFrom, pFrom);
	   strlwr(mFrom);
       if (strstr(mBoss, mFrom) || strstr(mMailApprovalManager, mFrom)) {
#ifdef UPDATE_20091203 // 承認、否承認を一括処理するためのワイルドカード指定機能のON/OFFオプションを追加した。
           sprintf(mMSGFn, "%s%s\\%s.MSG", mMailSpoolDir, mMailApprovalDir, (bMailApprovalWildcard ? mFn2 : &mSub[7]));
           sprintf(mRCPFn, "%s%s\\%s.RCP", mMailSpoolDir, mMailApprovalDir, (bMailApprovalWildcard ? mFn2 : &mSub[7]));
#else
#ifdef UPDATE_20091126 // 承認、否承認を一括処理するためのワイルドカード指定を可能にした。
           sprintf(mMSGFn, "%s%s\\%s.MSG", mMailSpoolDir, mMailApprovalDir, mFn2);
           sprintf(mRCPFn, "%s%s\\%s.RCP", mMailSpoolDir, mMailApprovalDir, mFn2);
#else
           sprintf(mMSGFn, "%s%s\\%s.MSG", mMailSpoolDir, mMailApprovalDir, &mSub[7]);
           sprintf(mRCPFn, "%s%s\\%s.RCP", mMailSpoolDir, mMailApprovalDir, &mSub[7]);
#endif
#endif
#ifdef UPDATE_20161112 // 送信先制限にエンベロープの送信元と日付ヘッダ中のトークンを指定可能にした。
	     if (bMailApprovalRevers)
		 {
	       SetMailApprovalReversAddr(pTo, mMSGFn, FALSE);
           if(bMailApprovalRejectRes)  // 却下通知の有効有無
		   {
		     if ((fp = fopen(pRCP, "wt"))) // 送信先を空にする
			 {
			   fprintf(fp, "Message-ID: <%s>\n", &mSub[9]);
               fprintf(fp, "Return-path: \n");
               fprintf(fp, "Recipient: \n");
		       fclose(fp);
			 }
		   }
		   //ApprovalLog(FALSE, pIP, mFrom, &mSub[7]); // 上長承認結果ログに送信元IPの記録を追加した。
		 }
#endif
		 {
	       if ((fp = fopen(mMSGFn, "rt"))) {
		     fclose(fp);
#ifdef UPDATE_20091203 // 承認、否承認を一括処理するためのワイルドカード指定機能のON/OFFオプションを追加した。
	         sprintf(mRCPFn, "%s%s\\%s.RCP", mMailSpoolDir, mMailApprovalDir, (bMailApprovalWildcard ? mFn2 : &mSub[7]));
             sprintf(mMSGDestFn, "%stemp\\%s.MS$", mMailSpoolDir, (bMailApprovalWildcard ? mFn2 : &mSub[7]));
             MakeEML(pFrom, mMSGFn, pMSG, mMSGDestFn, (bMailApprovalWildcard ? mFn2 : &mSub[7]), 2);
#else
#ifdef UPDATE_20091126 // 承認、否承認を一括処理するためのワイルドカード指定を可能にした。
	         sprintf(mRCPFn, "%s%s\\%s.RCP", mMailSpoolDir, mMailApprovalDir, mFn2);
             sprintf(mMSGDestFn, "%stemp\\%s.MS$", mMailSpoolDir, mFn2);
             MakeEML(pFrom, mMSGFn, pMSG, mMSGDestFn, mFn2, 2);
#else
	         sprintf(mRCPFn, "%s%s\\%s.RCP", mMailSpoolDir, mMailApprovalDir, &mSub[7]);
             sprintf(mMSGDestFn, "%stemp\\%s.MS$", mMailSpoolDir, &mSub[7]);
             MakeEML(pFrom, mMSGFn, pMSG, mMSGDestFn, &mSub[7], 2);
#endif
#endif
             CopyFile(mMSGDestFn, pMSG, FALSE);
		     while(!(fp = fopen(mMSGDestFn, "rt"))) {
               if (bServiceTerminating)
		         break;
	           _sleep(WAIT_TIME);
			 }
		     fclose(fp);
             //DeleteFile(mMSGDestFn);
		     while(!DeleteFile(mMSGDestFn)) {
	           nErr = GetLastError();
	           if (nErr == ERROR_FILE_NOT_FOUND)
		          break;
               sprintf(mLog, "DeleteFile(%s) Fail. code=(%d) retry delete.", mMSGDestFn, nErr);
               LEVEL_3_ACCEPTLOG(NULL, mLog);
		       _sleep(WAIT_TIME);
			 }
#ifdef UPDATE_20080711 // 承認したことを、他の上長にも通知
		     if (strchr(mBoss, ','))  // 複数上長が設定されている場合のみ
			 {
#ifdef UPDATE_20091203 // 承認、否承認を一括処理するためのワイルドカード指定機能のON/OFFオプションを追加した。
               sprintf(mMSGDestFn, "%s%s\\%s.MSG", mMailSpoolDir, mMailQueueDir, (bMailApprovalWildcard ? mFn2 : &mSub[7]));
	           sprintf(mRCPFn, "%stemp\\%s.RCP", mMailSpoolDir, (bMailApprovalWildcard ? mFn2 : &mSub[7]));
 	           sprintf(mRCPDestFn, "%s%s%s.RCP", mMailSpoolDir, mMailQueueDir, (bMailApprovalWildcard ? mFn2 : &mSub[7]));
#else
#ifdef UPDATE_20091126 // 承認、否承認を一括処理するためのワイルドカード指定を可能にした。
               sprintf(mMSGDestFn, "%s%s\\%s.MSG", mMailSpoolDir, mMailQueueDir, mFn2);
	           sprintf(mRCPFn, "%stemp\\%s.RCP", mMailSpoolDir, mFn2);
 	           sprintf(mRCPDestFn, "%s%s%s.RCP", mMailSpoolDir, mMailQueueDir, mFn2);
#else
               sprintf(mMSGDestFn, "%s%s\\%s.MSG", mMailSpoolDir, mMailQueueDir, &mSub[7]);
	           sprintf(mRCPFn, "%stemp\\%s.RCP", mMailSpoolDir, &mSub[7]);
 	           sprintf(mRCPDestFn, "%s%s%s.RCP", mMailSpoolDir, mMailQueueDir, &mSub[7]);
#endif
#endif
		       CopyFile(pMSG, mMSGDestFn, FALSE);
		       while(!(fp = fopen(mMSGDestFn, "rt"))) {
                 if (bServiceTerminating)
  		           break;
	             _sleep(WAIT_TIME);
			   }
		       fclose(fp);
		       if ((fp = fopen(pRCP, "rt"))) 
			   {
	             fgets(mwork, sizeof(mwork), fp);
	             fclose(fp);           
			   }
		       if ((fp = fopen(mRCPFn, "wt"))) 
			   {
	             fprintf(fp, "%s", mwork);
			     fprintf(fp, "Return-path: %s\n", mFrom);
	             fclose(fp);           
			   }
		       AddToBossAddrress(mFrom, mBoss, mRCPFn);
		       while(!(fp = fopen(mRCPFn, "rt")))
			   {
                 if (bServiceTerminating)
	               _sleep(WAIT_TIME);
			   }
		       fclose(fp);
		       CopyFile(mRCPFn, mRCPDestFn, FALSE);
		       while(!DeleteFile(mRCPFn)) {
	             nErr = GetLastError();
	             if (nErr == ERROR_FILE_NOT_FOUND)
		           break;
                 sprintf(mLog, "DeleteFile(%s) Fail. code=(%d) retry delete.", mRCPFn, nErr);
                 LEVEL_3_ACCEPTLOG(NULL, mLog);
		         _sleep(WAIT_TIME);
			   }
		       /// 元に戻す。
#ifdef UPDATE_20091203 // 承認、否承認を一括処理するためのワイルドカード指定機能のON/OFFオプションを追加した。
               sprintf(mRCPFn, "%s%s\\%s.RCP", mMailSpoolDir, mMailApprovalDir, (bMailApprovalWildcard ? mFn2 : &mSub[7]));
#else
#ifdef UPDATE_20091126 // 承認、否承認を一括処理するためのワイルドカード指定を可能にした。
               sprintf(mRCPFn, "%s%s\\%s.RCP", mMailSpoolDir, mMailApprovalDir, mFn2);
#else
               sprintf(mRCPFn, "%s%s\\%s.RCP", mMailSpoolDir, mMailApprovalDir, &mSub[7]);
#endif
#endif
			 }
#endif
		     if (bMailApprovalLog)
			 {
#ifdef UPDATE_20091203 // 承認、否承認を一括処理するためのワイルドカード指定機能のON/OFFオプションを追加した。
               ApprovalLog(FALSE, mFrom, (bMailApprovalWildcard ? mFn2 : &mSub[7]));
#else
#ifdef UPDATE_20091126 // 承認、否承認を一括処理するためのワイルドカード指定を可能にした。
               ApprovalLog(FALSE, mFrom, mFn2);
#else
#ifdef UPDATE_20140623A // 上長承認結果ログに送信元IPの記録を追加した。
               ApprovalLog(FALSE, pIP, mFrom, &mSub[7]);
#else
               ApprovalLog(FALSE, mFrom, &mSub[7]);
#endif
#endif
#endif
			 }
		   } else {
#ifdef UPDATE_20080620 // TOアドレスが代理人になる機能追加
             X_ReturnCTLMail(0, pFrom, NULL, pMSG, NULL);
#else
             X_ReturnCTLMail(0, pFrom, pMSG, NULL);
#endif
		   }
		 }
		 // メールを削除する。
#ifdef UPDATE_20090121 // 上長承認保留ファイル名称の生成ルール変更
		 while(!DeleteFile(mADRFn)) {
	       nErr = GetLastError();
	       if (nErr == ERROR_FILE_NOT_FOUND)
		     break;
           sprintf(mLog, "DeleteFile(%s) Fail. code=(%d) retry delete.", mADRFn, nErr);
           LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
		   _sleep(WAIT_TIME);
		 }
#endif
#ifdef UPDATE_20071220A // 承認処理済で削除がされないことがある。
		 while(!DeleteFile(mMSGFn)) {
	       nErr = GetLastError();
	       if (nErr == ERROR_FILE_NOT_FOUND)
		     break;
#ifdef UPDATE_20091203A // 承認、否承認のリクエストにワイルドカード指定があると対象のセッションがロックする。
		   if (strpbrk(mMSGFn, "?*"))
		   {
			 break;
		   }
#endif
           sprintf(mLog, "DeleteFile(%s) Fail. code=(%d) retry delete.", mMSGFn, nErr);
           LEVEL_3_ACCEPTLOG(NULL, mLog);
		   _sleep(WAIT_TIME);
		 }
		 while(!DeleteFile(mRCPFn)) {
	       nErr = GetLastError();
	       if (nErr == ERROR_FILE_NOT_FOUND)
		     break;
#ifdef UPDATE_20091203A // 承認、否承認のリクエストにワイルドカード指定があると対象のセッションがロックする。
		   if (strpbrk(mRCPFn, "?*"))
		   {
			 break;
		   }
#endif
           sprintf(mLog, "DeleteFile(%s) Fail. code=(%d) retry delete.", mRCPFn, nErr);
           LEVEL_3_ACCEPTLOG(NULL, mLog);
		   _sleep(WAIT_TIME);
		 }
#else
		 DeleteFile(mMSGFn);
		 DeleteFile(mRCPFn);
#endif
	   }
	 }
#ifdef UPDATE_20091126 // 承認、否承認を一括処理するためのワイルドカード指定を可能にした。
          bFile = FindNextFile( hF, &FD);
        }
        FindClose( hF ); 
     }
#endif
   } else if (!strncmp(mSub, "waitlist_", 9)) { //} else if (!strncmp(mSub, "waitlist+", 9)) {
     sprintf(mMSGFn, "%s%s\\%s.MSG", mMailSpoolDir, mMailApprovalDir, (mSub[9] ? &mSub[9] : "*"));
     sprintf(mRCPFn, "%s%s\\%s.RCP", mMailSpoolDir, mMailApprovalDir, (mSub[9] ? &mSub[9] : "*"));
     strcpy(mMSGDestFn, pMSG);
	 mMSGDestFn[strlen(mMSGDestFn)-1] = '$';
     MakeEML(pFrom, mMSGFn, pMSG, mMSGDestFn, "", 3);
     CopyFile(mMSGDestFn, pMSG, FALSE);
     //DeleteFile(mMSGDestFn);
     while(!DeleteFile(mMSGDestFn)) {
       nErr = GetLastError();
	   if (nErr == ERROR_FILE_NOT_FOUND)
	     break;
       sprintf(mLog, "DeleteFile(%s) Fail. code=(%d) retry delete.", mMSGDestFn, nErr);
       LEVEL_3_ACCEPTLOG(NULL, mLog);
	   _sleep(WAIT_TIME);
	 }
   } else if (bMailApprovalLog && !strnicmp(mSub, "approvalgetlog_", 15)) {
     strcpy(mFrom, pFrom);
     strlwr(mFrom);
     if (strstr(mMailApprovalManager, mFrom)) { // 管理者アドレス一致
#ifdef UPDATE_20080620 // TOアドレスが代理人になる機能追加
       X_ReturnCTLMail(2, pFrom, NULL, pMSG, &mSub[15]);
#else
       X_ReturnCTLMail(2, pFrom, pMSG, &mSub[15]);
#endif
	 } else {
#ifdef UPDATE_20080620 // TOアドレスが代理人になる機能追加
       X_ReturnCTLMail(1, pFrom, NULL, pMSG, NULL);
#else
       X_ReturnCTLMail(1, pFrom, pMSG, NULL);
#endif
	 }
   } else if (bMailApprovalLog && !strnicmp(mSub, "approvaldellog_", 15)) {
     strcpy(mFrom, pFrom);
     strlwr(mFrom);
     if (strstr(mMailApprovalManager, mFrom)) { // 管理者アドレス一致
#ifdef UPDATE_20080620 // TOアドレスが代理人になる機能追加
       X_ReturnCTLMail(3, pFrom, NULL, pMSG, &mSub[15]);
#else
       X_ReturnCTLMail(3, pFrom, pMSG, &mSub[15]);
#endif
	 } else {
#ifdef UPDATE_20080620 // TOアドレスが代理人になる機能追加
       X_ReturnCTLMail(1, pFrom, NULL, pMSG, NULL);
#else
       X_ReturnCTLMail(1, pFrom, pMSG, NULL);
#endif
	 }
#ifdef UPDATE_20070713 // 承認者の代理人を設定するオプション
   } else if (!strncmp(mSub, "setproxyuser_", 13)) { //代理人を設定する
#ifdef UPDATE_20080620 // TOアドレスが代理人になる機能追加
	 bChange = FALSE;
	 if (bSetProxyUserType)
	 {
	   strtok(pTo, "\r\n>");
	   if (pAPTo = strpbrk(pTo, "<")) 
	   {
         pAPTo++;
	   } else {
	     pAPTo = pTo;
	   }
	   while (pAPTo == ' ' || pAPTo == '\t')
	   {
	     pAPTo++;
	   }
	   if (mSub[13] == '\x0' && stricmp(pFrom, pAPTo)) // アドレス無効なら
	   {
	     bChange = TRUE;
	     strcpy(&mSub[13], pAPTo);
	   } 
	   strtok(pCc, "\r\n>");
	   if (pAPCc = strpbrk(pCc, "<")) 
	   {
         pAPCc++;
	   } else {
	     pAPCc = pCc;
	   }
	   while (pAPCc == ' ' || pAPCc == '\t')
	   {
	     pAPCc++;
	   }
	   if (mSub[13] == '\x0' && *pAPCc) // アドレス無効なら
	   {
	     bChange = TRUE;
	   }
	 }
#endif
#ifdef UPDATE_20090701 // 代理承認処理メールの挿入文の書換えを可能に
	 mProxyAddr[0] = '\x0';
     SetProxyUser(pBDir, &mSub[13], 0, mProxyAddr);
#else
#ifdef UPDATE_20080221 //BossCheck 承認者の代理人の代理人の深さの設定
     SetProxyUser(pBDir, &mSub[13], 0);
#else
     SetProxyUser(pBDir, &mSub[13]);
#endif
#endif
#ifdef UPDATE_20080620 // TOアドレスが代理人になる機能追加
#ifdef UPDATE_20090701 // 代理承認処理メールの挿入文の書換えを可能に
     X_ReturnCTLMail(4, pFrom, (bChange ? (*pAPCc ? pAPCc : pAPTo) : (mSub[13] ? &mSub[13] : mProxyAddr)), pMSG, &mSub[13]);
#else
     X_ReturnCTLMail(4, pFrom, (bChange ? (*pAPCc ? pAPCc : pAPTo) : &mSub[13]), pMSG, &mSub[13]);
#endif
#else
     X_ReturnCTLMail(4, pFrom, pMSG, &mSub[13]);
#endif
   } else if (!strncmp(mSub, "getproxyuser_", 13)) { //代理人を確認する
#ifdef UPDATE_20080220 // 承認者の代理人の代理人を設定する
#ifdef UPDATE_20080221 //BossCheck 承認者の代理人の代理人の深さの設定
     GetProxyUser(pBDir, &mSub[13], "", 0);
#else
     GetProxyUser(pBDir, &mSub[13], NULL);
#endif
#else
     GetProxyUser(pBDir, &mSub[13]);
#endif
#ifdef UPDATE_20080620 // TOアドレスが代理人になる機能追加
     X_ReturnCTLMail(5, pFrom, NULL, pMSG, &mSub[13]);
#else
     X_ReturnCTLMail(5, pFrom, pMSG, &mSub[13]);
#endif
#endif
#ifdef UPDATE_20080226 // 部下に対する承認のため保留中のリスト
   } else if (!strncmp(mSub, "waitmail_", 9)) {
     sprintf(mMSGFn, "%s%s\\%s.MSG", mMailSpoolDir, mMailApprovalDir, (mSub[9] ? &mSub[9] : "*"));
     sprintf(mRCPFn, "%s%s\\%s.RCP", mMailSpoolDir, mMailApprovalDir, (mSub[9] ? &mSub[9] : "*"));
     strcpy(mMSGDestFn, pMSG);
	 mMSGDestFn[strlen(mMSGDestFn)-1] = '$';
     MakeEML(pFrom, mMSGFn, pMSG, mMSGDestFn, "", 4);
     CopyFile(mMSGDestFn, pMSG, FALSE);
     //DeleteFile(mMSGDestFn);
     while(!DeleteFile(mMSGDestFn)) {
       nErr = GetLastError();
	   if (nErr == ERROR_FILE_NOT_FOUND)
	     break;
       sprintf(mLog, "DeleteFile(%s) Fail. code=(%d) retry delete.", mMSGDestFn, nErr);
       LEVEL_3_ACCEPTLOG(NULL, mLog);
       _sleep(WAIT_TIME);
	 }
#endif
   }
}

void MakeEML(char *pFrom, char *pAppMSG, char *pMSGSrc, char *pMSGDest, char *pMSGID, int nAapproval) {
  FILE *fpsrc, *fpdest, *fpd;
#ifdef UPDATE_20190110 // 承認一覧メールをHTMLメールにしたときutf-8に変換
  CHAR itm[512], mwork[512], mt[128], mMSGHead[256], mtB64[256];
#else
  CHAR mwork[256], mt[128], mMSGHead[256], mtB64[256];
#endif
  BOOL bSub = FALSE, bCont = FALSE;
  SYSTEMTIME ltime;
  DWORD nHeadNo = 0;
  char  *p;
#ifdef UPDATE_20080630 // In-Reply-To,Referencesへの対応
  CHAR mID[512], mRef[512];
#endif

#ifdef UPDATE_20080630 // In-Reply-Toへの対応
   mID[0] = mRef[0] = '\x0';
   GetInfo(_T("message-id:"), pMSGSrc, mID, sizeof(mID));
   GetInfo(_T("message-id:"), pAppMSG, mRef, sizeof(mRef));
#endif
   ///////////////////////////////////////
   strcpy(mMSGHead, pMSGSrc);
   mMSGHead[strlen(mMSGHead)-1] = '$';
   ///////////////////////////////////////
   gettime(&ltime, mt);
   translateUue2Base64(mt, strlen(mt), mtB64);
   if ((fpd = fopen(pMSGDest, "wt"))) {
#ifdef UPDATE_20080630 // In-Reply-Toへの対応
     if (mID[0])
	 {
	   fprintf(fpd, "In-Reply-To: %s\n", mRef); //mID); // 対応するメッセージIDをIn-Reply-To:ヘッダへ記載
	 }
	 if (mRef[0])
	 {
	   fprintf(fpd, "References: %s\n", mRef); //, mID); // 対応するメッセージIDをIn-Reply-To:ヘッダへ記載
	 }
#endif     	 
     if ((fpsrc = fopen(pMSGSrc, "rt"))) {
       if ((fpdest = fopen(mMSGHead, "wt"))) {
	     while(fgets( mwork, sizeof(mwork), fpsrc )) {
		   fputs(mwork, fpdest);
	       if ((mwork[0] == '.' && (mwork[1] == '\r' || mwork[1] == '\n')) ||
			   (mwork[0] == '\r' || mwork[0] == '\n')) {
		     break;
		   } else if (!strnicmp(mwork, "subject:", 8)) {
 		     bSub = TRUE;
		   } else if (!strnicmp(mwork, "content-type:", 13)) {
		     bCont = TRUE;
		   } else if (bSub || bCont) {
		     if (mwork[0] != '\t' && mwork[0] != ' ') {
               bSub = bCont = FALSE;
		       fputs(mwork, fpd);
			 }
		   } else {
		    fputs(mwork, fpd);
		   } 
		 }
		 fclose(fpdest);
	   }
	   switch(nAapproval) {
	     case  0: fprintf(fpd, "Subject: =?iso-2022-jp?B?GyRCQG4+ZSF3JSQhPCEmJV0lOSVIGyhC?=(%s)\n", pFrom); // 削除通知
		          break;
	     case  1: fprintf(fpd, "Subject: =?iso-2022-jp?B?GyRCPjVHJ0RMQ04bKEI=?=(%s)\n", pFrom); // 承認通知
			      break;
	     case  2: fprintf(fpd, "Subject: =?iso-2022-jp?B?GyRCNVEyPERMQ04bKEI=?=(%s)\n", pFrom); // 却下通知
			      break;
#ifdef UPDATE_20080226 // 部下に対する承認のため保留中のリスト
	     case  3: fprintf(fpd, "Subject: =?iso-2022-jp?B?GyRCPjVHJ0JUJEEwbE13GyhC?=(%s)\n", pFrom); // 保留一覧
			      break;
	     case  4: fprintf(fpd, "Subject: =?iso-2022-jp?B?GyRCSl1OMUNmJE4lYSE8JWsbKEI=?=(%s)\n", pFrom); // 部下取得用保留メール一覧
			      break;
#endif
	     default: fprintf(fpd, "Subject: =?iso-2022-jp?B?GyRCPjVHJ0JUJEEwbE13GyhC?=(%s)\n", pFrom); // 保留一覧
			      break;
	   }
       fprintf(fpd, "Content-Type: multipart/mixed;\n\tboundary=\"----%s\"\n", mtB64);
	   fclose(fpsrc);
	 }
     fprintf(fpd, "\n");
     fprintf(fpd, "\n------%s\n", mtB64);
#ifdef UPDATE_20190110 // 承認一覧メールをHTMLメールにしたときutf-8に変換
	 fprintf(fpd, "Content-Type: text/%s; charset=\"%s\"\n\n%s", (bMailApprovalHtml ? "html" : "plain"), (bMailApprovalHtml ? "UTF-8" : "ISO-2022-JP"), (bMailApprovalHtml ? "<html><body>\r\n" : "")); 
#else
     fprintf(fpd, "Content-Type: text/plain; charset=\"ISO-2022-JP\"\n\n"); 
#endif
	 ////////////////////////////////////////////////////////
	 // メッセージ挿入
	 fputs("\n", fpd);
     switch(nAapproval) {
#ifdef UPDATE_20190110 // 承認一覧メールをHTMLメールにしたときutf-8に変換
	   case  0: sprintf(mwork, "\x1B$BL58z$J>5G'@)8f$,\x1B(B(%s)\x1B$B$K$h$C$FMW5a$5$l$^$7$?!#\x1B(B%s\n", pFrom, (bMailApprovalHtml ? "<br>" : "")); // 削除通知
                if (bMailApprovalHtml)
				{
                  CODEPAGE2UTF8N(CP_ISO_JAPANESE, mwork, itm, sizeof(itm)); // utf-8に変換
#ifdef UPDATE_20240205 // 承認依頼メールで１行当たりの文字数を正規化
		          if (nNomalColumnMaxSize != 0)
                    LineNormalization(fpd, nNomalColumnMaxSize, itm);
		          else 
#endif
       	          fputs(itm, fpd);
				} else {
       	          fputs(mwork, fpd); // JISのまま
				} 
#else
	   case  0: fprintf(fpd, "\x1B$BL58z$J>5G'@)8f$,\x1B(B(%s)\x1B$B$K$h$C$FMW5a$5$l$^$7$?!#\x1B(B\n", pFrom); // 削除通知
#endif
	            break;
#ifdef UPDATE_20190110 // 承認一覧メールをHTMLメールにしたときutf-8に変換
	   case  1: sprintf(mwork, "\x1B$BE:IU$N%%a!<%%k$O\x1B(B(%s)\x1B$B$K$h$C$F>5G'$5$lAw?.$5$l$^$7$?!#\x1B(B%s\n", pFrom, (bMailApprovalHtml ? "<br>" : "")); // 承認通知
                if (bMailApprovalHtml)
				{
                  CODEPAGE2UTF8N(CP_ISO_JAPANESE, mwork, itm, sizeof(itm)); // utf-8に変換
#ifdef UPDATE_20240205 // 承認依頼メールで１行当たりの文字数を正規化
		          if (nNomalColumnMaxSize != 0)
                    LineNormalization(fpd, nNomalColumnMaxSize, itm);
		          else 
#endif
       	          fputs(itm, fpd);
				} else {
       	          fputs(mwork, fpd); // JISのまま
				} 
#else
	   case  1: fprintf(fpd, "\x1B$BE:IU$N%%a!<%%k$O\x1B(B(%s)\x1B$B$K$h$C$F>5G'$5$lAw?.$5$l$^$7$?!#\x1B(B\n", pFrom); // 承認通知
#endif
			    break;
#ifdef UPDATE_20190110 // 承認一覧メールをHTMLメールにしたときutf-8に変換
	   case  2: sprintf(mwork, "\x1B$BE:IU$N%%a!<%%k$O\x1B(B(%s)\x1B$B$K$h$C$F5Q2<$5$lGK4~$5$l$^$7$?!#\x1B(B%s\n", pFrom, (bMailApprovalHtml ? "<br>" : "")); // 拒否通知
                if (bMailApprovalHtml)
				{
                  CODEPAGE2UTF8N(CP_ISO_JAPANESE, mwork, itm, sizeof(itm)); // utf-8に変換
#ifdef UPDATE_20240205 // 承認依頼メールで１行当たりの文字数を正規化
		          if (nNomalColumnMaxSize != 0)
                    LineNormalization(fpd, nNomalColumnMaxSize, itm);
		          else 
#endif
       	          fputs(itm, fpd);
				} else {
       	          fputs(mwork, fpd); // JISのまま
				} 
#else
	   case  2: fprintf(fpd, "\x1B$BE:IU$N%%a!<%%k$O\x1B(B(%s)\x1B$B$K$h$C$F5Q2<$5$lGK4~$5$l$^$7$?!#\x1B(B\n", pFrom); // 拒否通知
#endif
			    break;
#ifdef UPDATE_20080226 // 部下に対する承認のため保留中のリスト
#ifdef UPDATE_20190110 // 承認一覧メールをHTMLメールにしたときutf-8に変換
	   case  3: sprintf(mwork, "(%s)\x1B$B$N>5G'BT$A0lMw\x1B(B%s\n", pFrom, (bMailApprovalHtml ? "<br>" : "")); // 保留一覧
                if (bMailApprovalHtml)
				{
                  CODEPAGE2UTF8N(CP_ISO_JAPANESE, mwork, itm, sizeof(itm)); // utf-8に変換
#ifdef UPDATE_20240205 // 承認依頼メールで１行当たりの文字数を正規化
		          if (nNomalColumnMaxSize != 0)
                    LineNormalization(fpd, nNomalColumnMaxSize, itm);
		          else 
#endif
       	          fputs(itm, fpd);
				} else {
       	          fputs(mwork, fpd); // JISのまま
				} 
#else
	   case  3: fprintf(fpd, "(%s)\x1B$B$N>5G'BT$A0lMw\x1B(B\n", pFrom); // 保留一覧
#endif
			    break;
#ifdef UPDATE_20190110 // 承認一覧メールをHTMLメールにしたときutf-8に変換
	   case  4: sprintf(mwork, "(%s)\x1B$B$,Aw?.$7$?>5G'BT$A%%a!<%%k\x1B(B%s\n", pFrom, (bMailApprovalHtml ? "<br>" : "")); // 部下取得用保留メール一覧
                if (bMailApprovalHtml)
				{
                  CODEPAGE2UTF8N(CP_ISO_JAPANESE, mwork, itm, sizeof(itm)); // utf-8に変換
#ifdef UPDATE_20240205 // 承認依頼メールで１行当たりの文字数を正規化
		          if (nNomalColumnMaxSize != 0)
                    LineNormalization(fpd, nNomalColumnMaxSize, itm);
		          else 
#endif
       	          fputs(itm, fpd);
				} else {
       	          fputs(mwork, fpd); // JISのまま
				} 
#else
	   case  4: fprintf(fpd, "(%s)\x1B$B$,Aw?.$7$?>5G'BT$A%%a!<%%k\x1B(B\n", pFrom); // 部下取得用保留メール一覧
#endif
			    break;
#endif
#ifdef UPDATE_20190110 // 承認一覧メールをHTMLメールにしたときutf-8に変換
	   default: sprintf(mwork, "(%s)\x1B$B$N>5G'BT$A0lMw\x1B(B%s\n", pFrom, (bMailApprovalHtml ? "<br>" : "")); // 保留一覧
                if (bMailApprovalHtml)
				{
                  CODEPAGE2UTF8N(CP_ISO_JAPANESE, mwork, itm, sizeof(itm)); // utf-8に変換
#ifdef UPDATE_20240205 // 承認依頼メールで１行当たりの文字数を正規化
		          if (nNomalColumnMaxSize != 0)
                    LineNormalization(fpd, nNomalColumnMaxSize, itm);
		          else 
#endif
       	          fputs(itm, fpd);
				} else {
       	          fputs(mwork, fpd); // JISのまま
				} 
#else
	   default: fprintf(fpd, "(%s)\x1B$B$N>5G'BT$A0lMw\x1B(B\n", pFrom); // 保留一覧
#endif
			    break;
	 }
	 ////////////////////////////////////////////////////////
#ifdef UPDATE_20080226 // 部下に対する承認のため保留中のリスト
	 if (nAapproval == 4 ) {  // 部下取得用保留メール一覧
       WriteWaitListsForStaff(pFrom, pAppMSG, mtB64, fpd);
#ifdef UPDATE_20190110 // 承認一覧メールをHTMLメールにしたときutf-8に変換
	   fprintf(fpd, "%s", (bMailApprovalHtml ? "</body></html>" : "")); // 保留一覧
#endif
	 } else
#endif
	 if (nAapproval == 3 ) {  // 保留一覧
       WriteWaitLists(pFrom, pAppMSG, fpd);
#ifdef UPDATE_20190110 // 承認一覧メールをHTMLメールにしたときutf-8に変換
	   fprintf(fpd, "%s", (bMailApprovalHtml ? "</body></html>" : "")); // 保留一覧
#endif
	   if (!strstr(pAppMSG, "*")) { // １つのメールなら
#ifdef UPDATE_20230602 // 上長承認、セキュアハンドラへの通知メールに添付する元メールに特殊なバイナリコードが含まれていると正しく添付が出来ない不具合。
	     if ((fpsrc = fopen(pAppMSG, "rb"))) 
#else
	     if ((fpsrc = fopen(pAppMSG, "rt"))) 
#endif
		 {
           fprintf(fpd, "\n------%s\n", mtB64);
           fprintf(fpd, "Content-Type: message/rfc822\n\n"); 
           while (fgets(mwork, sizeof(mwork), fpsrc))
		   {
#ifdef UPDATE_20230602 // 上長承認、セキュアハンドラへの通知メールに添付する元メールに特殊なバイナリコードが含まれていると正しく添付が出来ない不具合。
             if (mwork[0] != '.' && (mwork[1] != '\r' || mwork[1] != '\n'))
			 {
			   if (mwork[0] == '\r' ||  mwork[0] == '\n') {
			     fputs("\n", fpd);
			   } else if (strtok(mwork, "\r\n")) {
			     fputs(mwork, fpd); fputs("\n", fpd);
			   } else {
                 fputs(mwork, fpd);
			   }
			 }
#else
             if (mwork[0] != '.' && mwork[1] != '\n')
               fputs(mwork, fpd);
#endif
		   }
           fclose(fpsrc);
		 }
	   }
	 } else {
#ifdef UPDATE_20190110 // 承認一覧メールをHTMLメールにしたときutf-8に変換
       fprintf(fpd, "%s\n", (bMailApprovalHtml ? "</body></html>" : "")); // 保留一覧
#endif
	   if (nAapproval == 2) {
#ifdef UPDATE_20230602 // 上長承認、セキュアハンドラへの通知メールに添付する元メールに特殊なバイナリコードが含まれていると正しく添付が出来ない不具合。
		 if ((fpsrc = fopen(pMSGSrc, "rb")))  // 否認理由を添付
#else
		 if ((fpsrc = fopen(pMSGSrc, "rt")))  // 否認理由を添付
#endif
		 {
           fprintf(fpd, "\n------%s\n", mtB64);
           fprintf(fpd, "Content-Type: message/rfc822\n\n"); 
           while (fgets(mwork, sizeof(mwork), fpsrc)) 
		   {
#ifdef UPDATE_20230602 // 上長承認、セキュアハンドラへの通知メールに添付する元メールに特殊なバイナリコードが含まれていると正しく添付が出来ない不具合。
             if (mwork[0] != '.' && (mwork[1] != '\r' || mwork[1] != '\n'))
			 {
			   if (mwork[0] == '\r' ||  mwork[0] == '\n') {
			     fputs("\n", fpd);
			   } else if (strtok(mwork, "\r\n")) {
			     fputs(mwork, fpd); fputs("\n", fpd);
			   } else {
                 fputs(mwork, fpd);
			   }
			 }
#else
             if (mwork[0] != '.' && mwork[1] != '\n')
               fputs(mwork, fpd);
#endif
		   }
           fclose(fpsrc);
		 }
	   }
#ifdef UPDATE_20230602 // 上長承認、セキュアハンドラへの通知メールに添付する元メールに特殊なバイナリコードが含まれていると正しく添付が出来ない不具合。
	   if ((fpsrc = fopen(pAppMSG, "rb"))) 
#else
	   if ((fpsrc = fopen(pAppMSG, "rt"))) 
#endif
	   {
         fprintf(fpd, "\n------%s\n", mtB64);
         fprintf(fpd, "Content-Type: message/rfc822\n\n"); 
         while (fgets(mwork, sizeof(mwork), fpsrc)) 
		 {
#ifdef UPDATE_20230602 // 上長承認、セキュアハンドラへの通知メールに添付する元メールに特殊なバイナリコードが含まれていると正しく添付が出来ない不具合。
           if (mwork[0] != '.' && (mwork[1] != '\r' || mwork[1] != '\n'))
		   {
			 if (mwork[0] == '\r' ||  mwork[0] == '\n') {
			   fputs("\n", fpd);
			 } else if (strtok(mwork, "\r\n")) {
			   fputs(mwork, fpd); fputs("\n", fpd);
			 } else {
               fputs(mwork, fpd);
			 }
		   }
#else
           if (mwork[0] != '.' && mwork[1] != '\n')
             fputs(mwork, fpd);
#endif
		 }
         fclose(fpsrc);
	   }
	 }
     fprintf(fpd, "\n------%s--\n", mtB64);
     fprintf(fpd, "\n.\n");
     fflush(fpd);
     fclose(fpd);
   }
   /////////////////////////////////
   // 書き込み完了までウエイト
   while (!(fpd = fopen(pMSGDest, "rt"))) {
     if (bServiceTerminating)
       break;
      _sleep(WAIT_TIME);
   }
   fclose(fpd);
}

#ifdef UPDATE_20080226 // 部下に対する承認のため保留中のリスト
void WriteWaitListsForStaff(char *pFrom, char *pFn, char *pmtB64, FILE *fp)
{
  HANDLE             hF;
  WIN32_FIND_DATA    FD;
  BOOL bFile = TRUE;
  FILE *fprcp, *fpsrc;
  CHAR *p, *q, mFn[256], mRCP[256], mFrom[256], mMSG[1024], mwork[256];

   hF = FindFirstFile(pFn, &FD);
   if (hF != INVALID_HANDLE_VALUE) {
      bFile = TRUE;
      while (bFile) {
	    if (!(!stricmp(FD.cFileName, ".") ||
	          !stricmp(FD.cFileName, ".."))) {
	      strcpy(mRCP, pFn);
		  if (strstr(mRCP, "*")) {
	        strtok(mRCP, "*");
		  } else {
			if ((q = strstr(mRCP, FD.cFileName))) {
			  *q = '\x0';
			}
		  }
	      strcat(mRCP, FD.cFileName);
		  strcpy(mMSG, mRCP);
	      strcpy(&mRCP[strlen(mRCP)-3], "RCP");
	      strcpy(&mMSG[strlen(mMSG)-3], "MSG");
		  mFrom[0] = '\x0';
		  if ((fprcp = fopen(mRCP, "rt"))) {
		    fgets(mFrom, sizeof(mFrom), fprcp); // Message-ID
		    fgets(mFrom, sizeof(mFrom), fprcp); // Return-Path
		    fclose(fprcp);
		    strtok(mFrom, "\r\n");
		    if (!stricmp(&mFrom[13], pFrom)) {
	          fprintf(fp, "\n");
#ifdef UPDATE_20230602 // 上長承認、セキュアハンドラへの通知メールに添付する元メールに特殊なバイナリコードが含まれていると正しく添付が出来ない不具合。
	          if ((fpsrc = fopen(mMSG, "rb"))) 
#else
	          if ((fpsrc = fopen(mMSG, "rt"))) 
#endif
			  {
                fprintf(fp, "\n------%s\n", pmtB64);
                fprintf(fp, "Content-Type: message/rfc822\n\n"); 
                while (fgets(mwork, sizeof(mwork), fpsrc)) 
				{
#ifdef UPDATE_20230602 // 上長承認、セキュアハンドラへの通知メールに添付する元メールに特殊なバイナリコードが含まれていると正しく添付が出来ない不具合。
                  if (mwork[0] != '.' && (mwork[1] != '\r' || mwork[1] != '\n'))
				  {
					if (mwork[0] == '\r' ||  mwork[0] == '\n') {
					  fputs("\n", fp);
					} else if (strtok(mwork, "\r\n")) {
			          fputs(mwork, fp); fputs("\n", fp);
					} else {
                      fputs(mwork, fp);
					}
				  } 
#else
                  if (mwork[0] != '.' && mwork[1] != '\n')
                    fputs(mwork, fp);
#endif
				}
                fclose(fpsrc);
			  }
			}
		  }
		}
        bFile = FindNextFile( hF, &FD);
	  }; 
      FindClose( hF ); 
   }
}
#endif

#ifdef UPDATE_20080229 // 承認保留一覧に各メールの題名を表示する
INT GetCodePage(_TCHAR *pCharSet, _TCHAR *pWork) {
  _TCHAR mData[2048];
  INT nCP = CP_ACP; // デフォルトコード　//JAPANESE;S-JIS 

  if (!_tcsicmp(pCharSet, _T("us-ascii")) ||
	 !_tcsicmp(pCharSet, _T("ascii")) ) {
	nCP = CP_US_ASCII;
  } else if (!_tcsicmp(pCharSet, _T("Windows-1250")) ||
	         !_tcsicmp(pCharSet, _T("x-cp1250")) ) {
    nCP = CP_ANSI_CENTRAL_EUROPEAN;
  } else if (!_tcsicmp(pCharSet, _T("Windows-1251")) ||
	         !_tcsicmp(pCharSet, _T("x-cp1251")) ) {
    nCP = CP_ANSI_CYRILLIC;
  } else if (!_tcsicmp(pCharSet, _T("Windows-1253")) ) {
    nCP = CP_ANSI_GREEK;
  } else if (!_tcsicmp(pCharSet, _T("Windows-1254")) ) {
    nCP = CP_ANSI_TURKISH;
  } else if (!_tcsicmp(pCharSet, _T("Windows-1257")) ) {
    nCP = CP_ANSI_BALTIC;
  } else if (!_tcsicmp(pCharSet, _T("iso8859-2")) ||
	 !_tcsicmp(pCharSet, _T("iso-8859-2")) ||
	 !_tcsicmp(pCharSet, _T("iso_8859-2")) ||
	 !_tcsicmp(pCharSet, _T("latin2")) ||
	 !_tcsicmp(pCharSet, _T("iso_8859-2:1987")) ||
	 !_tcsicmp(pCharSet, _T("iso-ir-101")) ||
	 !_tcsicmp(pCharSet, _T("l2")) ||
	 !_tcsicmp(pCharSet, _T("csISOLatin2 ")) ) {
	nCP = CP_ISO_CENTRAL_EUROPEAN;
  } else if (!_tcsicmp(pCharSet, _T("iso8859-1")) ||
      !_tcsicmp(pCharSet, _T("iso_8859-1")) ||
      !_tcsicmp(pCharSet, _T("iso-8859-1")) ||
      !_tcsicmp(pCharSet, _T("ansi_x3.4-1968")) ||
      !_tcsicmp(pCharSet, _T("iso-ir-6")) ||
      !_tcsicmp(pCharSet, _T("ansi_x3.4-1986")) ||
      !_tcsicmp(pCharSet, _T("iso_646")) ||
      !_tcsicmp(pCharSet, _T("irv:1991")) ||
      !_tcsicmp(pCharSet, _T("iso646-us")) ||
      !_tcsicmp(pCharSet, _T("us")) ||
      !_tcsicmp(pCharSet, _T("ibm367")) ||
      !_tcsicmp(pCharSet, _T("cp367")) ||
      !_tcsicmp(pCharSet, _T("csascii")) ||
      !_tcsicmp(pCharSet, _T("latin1")) ||
      !_tcsicmp(pCharSet, _T("iso_8859-1:1987")) ||
      !_tcsicmp(pCharSet, _T("iso-ir-100")) ||
      !_tcsicmp(pCharSet, _T("ibm819")) ||
      !_tcsicmp(pCharSet, _T("cp819")) ||
      !_tcsicmp(pCharSet, _T("windows-1252 ")) ) {
	nCP = CP_ISO_EUROPEAN;
  } else if (!_tcsicmp(pCharSet, _T("iso_8859-4:1988")) ||
	         !_tcsicmp(pCharSet, _T("iso-ir-110")) ||
	         !_tcsicmp(pCharSet, _T("iso_8859-4")) ||
	         !_tcsicmp(pCharSet, _T("iso-8859-4")) ||
	         !_tcsicmp(pCharSet, _T("latin4")) ||
	         !_tcsicmp(pCharSet, _T("l4")) ||
			 !_tcsicmp(pCharSet, _T("csisolatin4 ")) ) {
	nCP = CP_ISO_BALTIC;    // バルト語 (ISO-8859-4)         // Latin 4
  } else if (!_tcsicmp(pCharSet, _T("iso_8859-5:1988")) ||
	         !_tcsicmp(pCharSet, _T("iso-ir-144")) ||
	         !_tcsicmp(pCharSet, _T("iso_8859-5")) ||
	         !_tcsicmp(pCharSet, _T("iso-8859-5")) ||
	         !_tcsicmp(pCharSet, _T("cyrillic")) ||
	         !_tcsicmp(pCharSet, _T("csisolatincyrillic")) ||
			 !_tcsicmp(pCharSet, _T("csisolatin5")) ) {
	nCP = CP_ISO_CYRILLIC;    // キリル語 (ISO-8859-5)
  } else if (!_tcsicmp(pCharSet, _T("iso_8859-7:1987")) ||
	         !_tcsicmp(pCharSet, _T("iso-ir-126")) ||
	         !_tcsicmp(pCharSet, _T("iso_8859-7")) ||
	         !_tcsicmp(pCharSet, _T("iso-8859-7")) ||
	         !_tcsicmp(pCharSet, _T("elot_928")) ||
	         !_tcsicmp(pCharSet, _T("ecma-118")) ||
	         !_tcsicmp(pCharSet, _T("greek")) ||
	         !_tcsicmp(pCharSet, _T("greek8")) ||
			 !_tcsicmp(pCharSet, _T("csisolatingreek")) ) {
	nCP = CP_ISO_GREEK;    // ギリシア語 (ISO-8859-7)
  } else if (!_tcsicmp(pCharSet, _T("iso_8859-9:1989")) ||
	         !_tcsicmp(pCharSet, _T("iso-ir-148")) ||
	         !_tcsicmp(pCharSet, _T("iso_8859-9")) ||
	         !_tcsicmp(pCharSet, _T("iso-8859-9")) ||
	         !_tcsicmp(pCharSet, _T("latin5")) ||
	         !_tcsicmp(pCharSet, _T("l5")) ||
			 !_tcsicmp(pCharSet, _T("csisolatin5")) ) {
	nCP = CP_ISO_TURKISH;    // トルコ語 (ISO-8859-9)         // Latin 5
  } else if (!_tcsicmp(pCharSet, _T("iso-8859-8i")) ||
	         !_tcsicmp(pCharSet, _T("Windows-1255"))) {
	nCP = CP_ANSI_HEBREW;     // ヘブライ語 (Windows)
  } else if (!_tcsicmp(pCharSet, _T("iso-8859-8 visual")) ||
             !_tcsicmp(pCharSet, _T("iso-8859-8")) ||
             !_tcsicmp(pCharSet, _T("iso_8859-8")) ||
             !_tcsicmp(pCharSet, _T("visual")) ) {
    nCP = CP_ISO_HEBREW_VISUAL;  // ヘブライ語 (ISO-8859-8) 視覚順
  } else if (!_tcsicmp(pCharSet, _T("dos-862")) ) {
    nCP = CP_HEBREW;      // ヘブライ語 (MS-DOS)
  } else if (!_tcsicmp(pCharSet, _T("windows-1256")) ) {
    nCP = CP_ANSI_ARABIC;    // アラビア語 (Windows)
  } else if (!_tcsicmp(pCharSet, _T("dos-720")) ) {
    nCP = CP_ARABIC_TP_ASMO;      // アラビア語 (Transparent ASMO)
  } else if (!_tcsicmp(pCharSet, _T("windows-874")) ) {
    nCP = CP_THAI;      // タイ語
  } else if (!_tcsicmp(pCharSet, _T("Windows-1258")) ) {
    nCP = CP_ANSI_VIETNAMESE;     // ベトナム語 (Windows)
  } else if (!_tcsicmp(pCharSet, _T("utf-7")) ||
      !_tcsicmp(pCharSet, _T("unicode-1-1-utf-7")) ||
      !_tcsicmp(pCharSet, _T("csunicode11utf7")) ) {
    nCP = CP_UTF_7;
  } else if (!_tcsicmp(pCharSet, _T("utf-8")) ||
      !_tcsicmp(pCharSet, _T("unicode-1-1-utf-8")) ||
      !_tcsicmp(pCharSet, _T("unicode-2-0-utf-8")) ) {
    nCP = CP_UTF_8;
  } else if (!_tcsicmp(pCharSet, _T("shift_jis")) ||
      !_tcsicmp(pCharSet, _T("x-sjis")) ||
      !_tcsicmp(pCharSet, _T("ms_kanji")) ||
      !_tcsicmp(pCharSet, _T("csshiftjis")) ||
      !_tcsicmp(pCharSet, _T("x-ms-cp932 ")) ) {
	nCP =CP_JAPANESE; 
  } else if (!_tcsicmp(pCharSet, _T("iso-2022-jp")) ||
 	  !_tcsicmp(pCharSet, _T("csiso2022jp")) ) {
	nCP =CP_ISO_JAPANESE; 
  } else if (!_tcsicmp(pCharSet, _T("x-euc")) ||
             !_tcsicmp(pCharSet, _T("x-euc-jp")) ||
#ifdef UPDATE_20151118B // 上長承認対象メールのSubjectヘッダがEUC_JPでパックされていると文字化けする。
			 !_tcsicmp(pCharSet, _T("euc-jp")) ||
#endif
	         !_tcsicmp(pCharSet, _T("extended_unix_code_packed_format_for_japanese")) ) {
#ifdef UPDATE_20151118B // 上長承認対象メールのSubjectヘッダがEUC_JPでパックされていると文字化けする。
	nCP =CP_EUC_JAPANESE_MS; 
#else
	nCP =CP_EUC_JAPANESE; 
#endif
  } if (!_tcsicmp(pCharSet, _T("iso-2022-kr")) || // UTF-8へ変換
        !_tcsicmp(pCharSet, _T("csISO2022KR")) ) {
      if (_tcsncmp((_TCHAR *)pWork, _T("\x1b$)C"), 4) != 0) {
	    _tcscpy((_TCHAR *)mData, (_TCHAR *)pWork);
		_tcscpy((_TCHAR *)pWork, _T("\x1b$)C"));
		_tcscat((_TCHAR *)pWork, (_TCHAR *)mData);
	  }
	  nCP =CP_ISO_KOREAN; 
   } else if (!_tcsicmp(pCharSet, _T("euc-kr")) || 
			  !_tcsicmp(pCharSet, _T("ks_c_5601")) ||
			  !_tcsicmp(pCharSet, _T("ks_c_5601-1987")) ||
			  !_tcsicmp(pCharSet, _T("korean")) ||
			  !_tcsicmp(pCharSet, _T("csksc56011987")) ) {
 	 nCP =CP_EUC_KOREAN_2; 
   } else if (!_tcsicmp(pCharSet, _T("big5")) || 
			  !_tcsicmp(pCharSet, _T("csbig5")) ||
			  !_tcsicmp(pCharSet, _T("x-x-big5")) ) {
	 nCP =CP_CHINESE_TRADITIONAL; 
   } else if (!_tcsicmp(pCharSet, _T("gb2312")) || 
			  !_tcsicmp(pCharSet, _T("gb_2312-80")) ||
			  !_tcsicmp(pCharSet, _T("iso-ir-58")) ||
			  !_tcsicmp(pCharSet, _T("chinese")) ||
			  !_tcsicmp(pCharSet, _T("csiso58gb231280") ) ||
			  !_tcsicmp(pCharSet, _T("csgb231280") ) ) {
 	  nCP =CP_CHINESE_SIMPLIFIED; 
   } else if (!_tcsicmp(pCharSet, _T("csKOI8R")) ||
	          !_tcsicmp(pCharSet, _T("koi8-r")) ) {
      nCP = CP_CYRILLIC_KOI8R; // キリル語 (KOI8-R)

   }

   return nCP;
}

void LineConv2(char *dest, INT ndest, char *src, char *pack)
{
  CHAR *p, *p1, pe, *pk;
  CHAR itm[1024];
  CHAR work[1024];
  CHAR mCharSet[32];
  INT  nCP;
  int  ln;
  int  nMime;
  CHAR *q1;
  BOOL bUTF7 = FALSE;
  BOOL bUTF8 = FALSE;
#ifdef UPDATE_20160707 // 上長承認をUTF-8で処理する際、全角ハイフン「－」が内部処理で文字化けする
  CHAR *pu;
#endif

   *pack = '\x0';
   p1 = src;
   *dest = '\x0';
   while (*p1) {
    p = strstr(p1, _T("=?")); // BASE64でパックされているか？
    if (p && _tcsnicmp(p, _T("=??"), 3)) {
 	   //////////////////////////
	   *p = '\x0';
	   if (ndest > (INT)(strlen(dest) + strlen(p1))) // 領域オーバーしない範囲でコピー
	     strcat(dest, p1);
	   *p = '=';
	   //////////////////////////
	   nMime = 0;
           q1 = NULL;
	   if ((q1 = _tcschr(p+2, '?'))) {
	     if (!_tcsncmp(q1, _T("?B?"), 3))
			nMime = 1;
		 else if (!_tcsncmp(q1, _T("?Q?"), 3))
			nMime = 2;
		 if (!_tcsnicmp(p+2, _T("utf-7"), 5))
			bUTF7 = TRUE;
		 if (!_tcsnicmp(p+2, _T("utf-8"), 5))
			bUTF8 = TRUE;
 	     p1 = (strstr(q1+3,_T("?=")));
	   }
	   if ((q1 = _tcschr(p+2, '?'))) {
		 if (!_tcsncmp(q1, _T("?B?"), 3))
			nMime = 1;
		 else if (!_tcsncmp(q1, _T("?Q?"), 3))
			nMime = 2;
		 if (p1 = (strstr(q1+3, _T("?=")))) {
	   	   _tcsncpy(pack, p, p1-p+2); // パックされている言語を得る
		   if (nMime != 2) {
		     if ((pk = strstr(pack,_T("B?"))))
		       *(pk+2) = '\x0';
		     else
		       *pack = '\x0';
		   } else if ((pk = strstr(pack,_T("Q?")))) {
		     *(pk+2) = '\x0';
		   }
	       pe = *(p1+2);
           *(p1+2) = '\x0';
		 }
	   }
	   //p += 16;
	   if (q1) {
	     p += strlen(pack);
	   } else {
		 pe = *(p1+strlen(p1));
	   }
	   ln = strlen(p);
	   work[0] = '\x0';
       if (nMime == 1)
         Base64DecodeLine(p, ln, (_TCHAR *)work, 1024);
	   else if (nMime == 2)
	     QuotedPrintableDecodeLine((_TCHAR *)p, (_TCHAR *)work);
	   else
	     strcpy(work, p);
	   strcpy(mCharSet, &pack[2]);
	   strtok(mCharSet, _T("?"));
	   nCP = GetCodePage(mCharSet, work);
#ifdef UPDATE_20151118 // パックされている言語の末尾のホワイトスペースを除去する
	   if (nCP != CP_ISO_JAPANESE)
#endif
	   {
	     CODEPAGE2UTF8N(nCP, work, itm, sizeof(itm));
#ifdef UPDATE_20160707 // 上長承認をUTF-8で処理する際、全角ハイフン「－」が内部処理で文字化けする
		 UTF8_Rewrite_Value(itm);
#endif
	     UTF8N2CODEPAGE(CP_ISO_JAPANESE, itm, work, sizeof(work)); // iso-2022-jpに変換
	   }
	   //UTF8N2SJIS(itm, work, sizeof(work));
       strcat(dest, work);
       if (p1) {
	     *(p1+2) = pe;
	     p1+=2;
	   } else
		break;
	 } else {
#ifdef UPDATE_20151118 // パックされている言語の末尾のホワイトスペースを除去する
       while(*p1 == '\x09' || *p1 == '\x20')
	   {
	     p1++;
	   }
#endif
       p = strstr(p1, _T("\x1b$")); // パック無しのデータか？
       if (p) {
#ifdef UPDATE_20151118 // パックされている言語の末尾のホワイトスペースを除去する
	     strcat(dest, p1);
		 p1 += strlen(p1);
#else
	     INT nCP = GetCodePage(_T("iso-2022-jp"), work);
	     CODEPAGE2UTF8N(nCP, work, itm, sizeof(itm));
	     UTF8N2SJIS(itm, work, sizeof(work));
         strcat(dest, work);
#endif
         break;
	   } else {
	     strcat(dest, p1);
	     p1 += strlen(p1);
	   }
	 }
   }
}

void GetInfo(_TCHAR *pTkn, _TCHAR *pFn, _TCHAR *pData, DWORD nMaxLen)
{
	FILE *fp;
    _TCHAR *p, *ps;
	BOOL bNext = FALSE;
	int  l = 0;
	DWORD nLen = 0;
	_TCHAR mData[1024], mDest[1024], mPack[1024];

	*pData = '\x0';
	bNext = FALSE;
	if ((fp = _tfopen(pFn, _T("rt")))) {
	  while ((p = _fgetts(mData, sizeof(mData), fp)))
	  {
		if (mData[0] == '\r' || mData[0] == '\n') // ヘッダの終わり
		  break;
        ps = mData;
		if (!_tcsnicmp(mData, pTkn, strlen(pTkn)) ||
			(bNext && (*ps == ' ' || *ps == '\t')))
		{
		  strtok(mData, _T("\r\n"));
		  if (!bNext) {
			ps = &mData[strlen(pTkn)];
		    bNext = TRUE;
		  } else {
			ps = mData;
		  }
		  while(*ps == ' ' || *ps == '\t')
		    ps++;
#ifdef UPDATE_20170112 // データ行に改行だけの場合も除去するようにする
		  if (*ps)
#endif
		  {
		    mDest[0] = '\x0';
		    LineConv2(mDest, sizeof(mDest), ps, mPack);
		    if (nMaxLen > nLen + strlen(mDest)) {  // バッファオーバーしないように対策
		      strcat(pData, mDest);
		      nLen += strlen(pData);
			}
		  }
		} else {
		  bNext = FALSE;
		}
	  }
	  fclose(fp);
	}
}
#endif

void WriteWaitLists(char *pFrom, char *pFn, FILE *fp)
{
  HANDLE             hF;
  WIN32_FIND_DATA    FD;
  BOOL bFile = TRUE;
  FILE *fprcp;
  CHAR *p, *q, mFn[256], mRCP[256], mFrom[256], mBoss[1024];
#ifdef UPDATE_20080229 // 承認保留一覧に各メールの題名を表示する
  CHAR  mMSG[256], mSubject[1024];
#endif
#ifdef UPDATE_20080514 // 上長宛承認メールのmailto:で記載した返信先アドレスのドメインを強制変更する設定
  CHAR *r1, *r2, mAFrom[512];
#endif
#ifdef UPDATE_20080515 // "waitlist_"コマンドで送信先アドレスを表示
  FILE *fpsrc;
  CHAR  itm[512];
  CHAR  mwork[512];
#endif
#ifdef UPDATE_20080517 // 上長宛承認メールの承認否認メールに対象メールの題名情報などを記録する
  CHAR mInfo[3][256];  // 承認対象メール情報
#endif
#ifdef UPDATE_20080519 // 上長宛承認メールの上長アドレスが完全一致したものを抽出する
  CHAR *pm1, *pm2;
  BOOL bMach = FALSE;
#endif
#ifdef UPDATE_20090121 // 上長承認保留ファイル名称の生成ルール変更
  FILE *fpadr;
  BOOL bWild = FALSE;
  CHAR *r, mADRFn[256], mData[1024];
#endif

#ifdef UPDATE_20090121 // 上長承認保留ファイル名称の生成ルール変更
   sprintf(mADRFn, pFn);
   if ((r = strchr(mADRFn, '*')))
   {
	 bWild = TRUE;
	 *r = '\x0';
   }
#endif
   hF = FindFirstFile(pFn, &FD);
   if (hF != INVALID_HANDLE_VALUE) {
      bFile = TRUE;
      while (bFile) {
	    if (!(!stricmp(FD.cFileName, ".") ||
	          !stricmp(FD.cFileName, ".."))) {
		  strcpy(mFn, FD.cFileName);
		  mFn[strlen(mFn)-4] = '\x0';
		  if ((p = strchr(mFn, '@'))) 
		  {
#ifdef UPDATE_20090121 // 上長承認保留ファイル名称の生成ルール変更
			if (bWild)
			{
			  *r = '\x0';
			} else {
              if ((r = strrchr(mADRFn, '\\')))
			  {
			    *(r+1) = '\x0';
			  }
			}
			if (r)
			{
			  strcat(mADRFn, mFn);
			  strcat(mADRFn, ".ADR");
	          if ((fpadr = fopen(mADRFn, "rt")))
			  {
	            fgets(mData, sizeof(mData), fpadr);
	            strtok(mData, "\r\n");
	            fclose(fpadr);
			  } else {
	            mData[0] = '\x0';
			  }
			}
			if (mData[0])
			{
			  p = mData;
			}
#endif
            Base64DecodeLine(p, strlen(p), (unsigned char *)mBoss, sizeof(mBoss));
#ifdef UPDATE_20080519 // 上長宛承認メールの上長アドレスが完全一致したものを抽出する
			bMach = FALSE;
			strtok(mBoss, " \r\n");
			pm1 = mBoss;
			while((pm2 = strchr(pm1, ',')))
			{
			  *pm2 = '\x0';
			  bMach = !stricmp(pm1, pFrom);
#ifdef UPDATE_20090120A // 承認者が３人以上の場合の処理修正
			  if (bMach)
			  {
				break;
			  }
#endif
			  pm1 = pm2 + 1;
			}
			if (!bMach) {
			  bMach = !stricmp(pm1, pFrom);
			}
			if (bMach)
#else
			if (strstr(mBoss, pFrom ))
#endif
			{
		      strcpy(mRCP, pFn);
			  if (strstr(mRCP, "*")) {
		        strtok(mRCP, "*");
			  } else {
				if ((q = strstr(mRCP, FD.cFileName))) {
				  *q = '\x0';
				}
			  }
		      strcat(mRCP, FD.cFileName);
		      strcpy(&mRCP[strlen(mRCP)-3], "RCP");
		      mFrom[0] = '\x0';
		      if ((fprcp = fopen(mRCP, "rt"))) {
		        fgets(mFrom, sizeof(mFrom), fprcp); // Message-ID
		        fgets(mFrom, sizeof(mFrom), fprcp); // Return-Path
		        fclose(fprcp);
		        strtok(mFrom, "\r\n");
#ifdef UPDATE_20080514 // 上長宛承認メールのmailto:で記載した返信先アドレスのドメインを強制変更する設定
                if (mMailApprovalDomain[0]) {
                  strcpy(mAFrom, &mFrom[13]);
                  if ((r1 = strchr(mAFrom, '@'))) {
                    r1++;
	                sprintf(r1, "%s.", mMailApprovalDomain);
				  }
	              if ((r2 = strchr(&mFrom[13], '@'))) {
	                r2++;
	                strcat(mAFrom, r2);
				  }
				}
#endif
			  }
#ifdef UPDATE_20080229 // 承認保留一覧に各メールの題名を表示する
		      strcpy(mMSG, mRCP);
		      strcpy(&mMSG[strlen(mMSG)-3], "MSG");
              GetInfo(_T("subject:"), mMSG, mSubject, sizeof(mSubject));
#ifdef UPDATE_20080517 // 上長宛承認メールの承認否認メールに対象メールの題名情報などを記録する
              mInfo[0][0] = mInfo[1][0] = mInfo[2][0] = '\x0';
              GetInfo(_T("date:"), mMSG, mwork, sizeof(mwork));
	          CODEPAGE2UTF8N(CP_ISO_JAPANESE, mwork, itm, sizeof(itm));
#ifdef UPDATE_20190110 // 承認一覧メールをHTMLメールにしたときutf-8に変換
              if (bMailApprovalHtml)
			  {
                EncodeString(itm, strlen(itm), mInfo[0]);
			  } else 
#endif
			  {
#ifdef UPDATE_20141210 // 承認保留一覧のmailtoリンクの文字コード指定
                UTF8N2CODEPAGE(nMailApprovalCodepage, itm, mwork, sizeof(mwork)); // iso-2022-jpに変換
#else
	            UTF8N2CODEPAGE(CP_JAPANESE, itm, mwork, sizeof(mwork)); // iso-2022-jpに変換
#endif
                EncodeString(mwork, strlen(mwork), mInfo[0]);
			  }
              GetInfo(_T("from:"), mMSG, mwork, sizeof(mwork));
	          CODEPAGE2UTF8N(CP_ISO_JAPANESE, mwork, itm, sizeof(itm));
#ifdef UPDATE_20190110 // 承認一覧メールをHTMLメールにしたときutf-8に変換
              if (bMailApprovalHtml)
			  {
                EncodeString(itm, strlen(itm), mInfo[1]);
			  } else 
#endif
			  {
#ifdef UPDATE_20141210 // 承認保留一覧のmailtoリンクの文字コード指定
                UTF8N2CODEPAGE(nMailApprovalCodepage, itm, mwork, sizeof(mwork)); // iso-2022-jpに変換
#else
	            UTF8N2CODEPAGE(CP_JAPANESE, itm, mwork, sizeof(mwork)); // iso-2022-jpに変換
#endif
                EncodeString(mwork, strlen(mwork), mInfo[1]);
			  }
              GetInfo(_T("subject:"), mMSG, mwork, sizeof(mwork));
	          CODEPAGE2UTF8N(CP_ISO_JAPANESE, mwork, itm, sizeof(itm));
#ifdef UPDATE_20190110 // 承認一覧メールをHTMLメールにしたときutf-8に変換
              if (bMailApprovalHtml)
			  {
                EncodeString(itm, strlen(itm), mInfo[2]);
			  } else 
#endif
			  {
#ifdef UPDATE_20141210 // 承認保留一覧のmailtoリンクの文字コード指定
                UTF8N2CODEPAGE(nMailApprovalCodepage, itm, mwork, sizeof(mwork)); // iso-2022-jpに変換
#else
	            UTF8N2CODEPAGE(CP_JAPANESE, itm, mwork, sizeof(mwork)); // iso-2022-jpに変換
#endif
                EncodeString(mwork, strlen(mwork), mInfo[2]);
			  }
#endif
#ifdef UPDATE_20190110 // 承認一覧メールをHTMLメールにしたときutf-8に変換
	          fprintf(fp, "%s\n", (bMailApprovalHtml ? "<br>" : ""));
			  sprintf(mwork, "[\x1B$BBjL>\x1B(B] %s", mSubject); //[再送]
              if (bMailApprovalHtml)
			  {
                CODEPAGE2UTF8N(CP_ISO_JAPANESE, mwork, itm, sizeof(itm)); // utf-8に変換
       	        fputs(itm, fp);
			  } else {
       	        fputs(mwork, fp); // JISのまま
			  }
#else
	          fprintf(fp, "\n");
			  fprintf(fp, "[\x1B$BBjL>\x1B(B] %s", mSubject); //[再送]
#endif
#endif
#ifdef UPDATE_20190110 // 承認一覧メールをHTMLメールにしたときutf-8に変換
	          fprintf(fp, "%s\n", (bMailApprovalHtml ? "<br>" : ""));
			  sprintf(mwork, "[\x1B$B:FAw\x1B(B] %smailto:", (bMailApprovalHtml ? "<a href=\"" : "")); //[再送]
              if (bMailApprovalHtml)
			  {
                CODEPAGE2UTF8N(CP_ISO_JAPANESE, mwork, itm, sizeof(itm)); // utf-8に変換
       	        fputs(itm, fp);
			  } else {
       	        fputs(mwork, fp); // JISのまま
			  }
#else
	          fprintf(fp, "\n");
			  fprintf(fp, "[\x1B$B:FAw\x1B(B] mailto:"); //[再送]
#endif
	          fprintf(fp, pFrom);
#ifdef UPDATE_20090213 // 上長承認メールのmailto:リンクの区切り記号を"?"以外に変更するオプションを追加
	          fprintf(fp, "%csubject=waitlist_", mMailApprovalMailTo[0]); //fprintf(fp, "?subject=approval+");
#else
	          fprintf(fp, "?subject=waitlist_"); //fprintf(fp, "?subject=approval+");
#endif
	          fprintf(fp, mFn);
#ifdef UPDATE_20080517 // 上長宛承認メールの承認否認メールに対象メールの題名情報などを記録する
#ifdef UPDATE_20190110 // 承認一覧メールをHTMLメールにしたときutf-8に変換
	          fprintf(fp, "&body=Date:%%20%s%%0AFrom:%%20%s%%0ASubject:%%20%s%s", mInfo[0], mInfo[1], mInfo[2], (bMailApprovalHtml ? "\">" : ""));
#else
	          fprintf(fp, "&body=Date:%%20%s%%0AFrom:%%20%s%%0ASubject:%%20%s", mInfo[0], mInfo[1], mInfo[2]);
#endif
#endif
#ifdef UPDATE_20190110 // 承認一覧メールをHTMLメールにしたときutf-8に変換
	          if (bMailApprovalHtml)
	          {
	            fprintf(fp, "mailto:"); //[承認]
	            fprintf(fp, pFrom);
                fprintf(fp, "%csubject=waitlist_", mMailApprovalMailTo[0]); // 上長承認メールのmailto:リンクの区切り記号を"?"以外に変更するオプションを追加 //fprintf(fp, "?subject=approval+");
	            fprintf(fp, mFn);
	            fprintf(fp, "&body=Date:%%20%s%%0AFrom:%%20%s%%0ASubject:%%20%s</a><br>", mInfo[0], mInfo[1], mInfo[2]); // 上長宛承認メールの承認否認メールに対象メールの題名情報などを記録する
	            fprintf(fp, "<br>\n");
	          } else {
	            fprintf(fp, "\n");
	          }
	          sprintf(mwork, "[\x1B$B>5G'\x1B(B] %smailto:", (bMailApprovalHtml ? "<a href=\"" : "")); //[承認]
              if (bMailApprovalHtml)
			  {
                CODEPAGE2UTF8N(CP_ISO_JAPANESE, mwork, itm, sizeof(itm)); // utf-8に変換
       	        fputs(itm, fp);
			  } else {
       	        fputs(mwork, fp); // JISのまま
			  }
#else
	          fprintf(fp, "\n");
	          fprintf(fp, "[\x1B$B>5G'\x1B(B] mailto:"); //[承認]
#endif
#ifdef UPDATE_20080514 // 上長宛承認メールのmailto:で記載した返信先アドレスのドメインを強制変更する設定
              fprintf(fp, (mMailApprovalDomain[0] ? mAFrom : &mFrom[13]));
#else
	          fprintf(fp, &mFrom[13]);
#endif
#ifdef UPDATE_20090213 // 上長承認メールのmailto:リンクの区切り記号を"?"以外に変更するオプションを追加
	          fprintf(fp, "%csubject=approval_", mMailApprovalMailTo[0]); //fprintf(fp, "?subject=approval+");
#else
	          fprintf(fp, "?subject=approval_"); //fprintf(fp, "?subject=approval+");
#endif
	          fprintf(fp, mFn);
#ifdef UPDATE_20080517 // 上長宛承認メールの承認否認メールに対象メールの題名情報などを記録する
#ifdef UPDATE_20190110 // 承認一覧メールをHTMLメールにしたときutf-8に変換
	          fprintf(fp, "&body=Date:%%20%s%%0AFrom:%%20%s%%0ASubject:%%20%s%s", mInfo[0], mInfo[1], mInfo[2], (bMailApprovalHtml ? "\">" : ""));
#else
	          fprintf(fp, "&body=Date:%%20%s%%0AFrom:%%20%s%%0ASubject:%%20%s", mInfo[0], mInfo[1], mInfo[2]);
#endif
#endif
#ifdef UPDATE_20190110 // 承認一覧メールをHTMLメールにしたときutf-8に変換
	          if (bMailApprovalHtml)
	          {
	            fprintf(fp, "mailto:"); //[承認]
                fprintf(fp, (mMailApprovalDomain[0] ? mAFrom : &mFrom[13])); // 上長宛承認メールのmailto:で記載した返信先アドレスのドメインを強制変更する設定
	            fprintf(fp, "%csubject=approval_", mMailApprovalMailTo[0]); // 上長承認メールのmailto:リンクの区切り記号を"?"以外に変更するオプションを追加//fprintf(fp, "?subject=approval+");
	            fprintf(fp, mFn);
	            fprintf(fp, "&body=Date:%%20%s%%0AFrom:%%20%s%%0ASubject:%%20%s</a><br>", mInfo[0], mInfo[1], mInfo[2]); // 上長宛承認メールの承認否認メールに対象メールの題名情報などを記録する
	            fprintf(fp, "<br>\n");
	          } else {
	            fprintf(fp, "\n");
	          }
	          sprintf(mwork, (bMailApprovalRevers && !CheckDomain(&mFrom[13]) ? "[\x1B$B5qH]\x1B(B] %smailto:" : "[\x1B$B5Q2<\x1B(B] %smailto:"), (bMailApprovalHtml ? "<a href=\"" : "")); //[拒否] [却下]
              if (bMailApprovalHtml)
			  {
                CODEPAGE2UTF8N(CP_ISO_JAPANESE, mwork, itm, sizeof(itm)); // utf-8に変換
       	        fputs(itm, fp);
			  } else {
       	        fputs(mwork, fp); // JISのまま
			  }
#else
	          fprintf(fp, "\n");
	          fprintf(fp, (bMailApprovalRevers && !CheckDomain(&mFrom[13]) ? "[\x1B$B5qH]\x1B(B] mailto:" : "[\x1B$B5Q2<\x1B(B] mailto:")); //[拒否] [却下]
#endif
#ifdef UPDATE_20080514 // 上長宛承認メールのmailto:で記載した返信先アドレスのドメインを強制変更する設定
              fprintf(fp, (mMailApprovalDomain[0] ? mAFrom : &mFrom[13]));
#else
	          fprintf(fp, &mFrom[13]);
#endif
#ifdef UPDATE_20090213 // 上長承認メールのmailto:リンクの区切り記号を"?"以外に変更するオプションを追加
	          fprintf(fp, "%csubject=reject_", mMailApprovalMailTo[0]); //fprintf(fp, "?subject=reject+");
#else
	          fprintf(fp, "?subject=reject_"); //fprintf(fp, "?subject=reject+");
#endif
	          fprintf(fp, mFn);
#ifdef UPDATE_20080517 // 上長宛承認メールの承認否認メールに対象メールの題名情報などを記録する
#ifdef UPDATE_20190110 // 承認一覧メールをHTMLメールにしたときutf-8に変換
	          fprintf(fp, "&body=Date:%%20%s%%0AFrom:%%20%s%%0ASubject:%%20%s%s", mInfo[0], mInfo[1], mInfo[2], (bMailApprovalHtml ? "\">" : ""));
#else
	          fprintf(fp, "&body=Date:%%20%s%%0AFrom:%%20%s%%0ASubject:%%20%s", mInfo[0], mInfo[1], mInfo[2]);
#endif
#endif
#ifdef UPDATE_20190110 // 承認一覧メールをHTMLメールにしたときutf-8に変換
	          if (bMailApprovalHtml)
	          {
	            fprintf(fp, "mailto:"); //[承認]
	            fprintf(fp, (mMailApprovalDomain[0] ? mAFrom : &mFrom[13])); //fprintf(fp, &mFrom[13]);
	            fprintf(fp, "%csubject=reject_", mMailApprovalMailTo[0]); // 上長承認メールのmailto:リンクの区切り記号を"?"以外に変更するオプションを追加//fprintf(fp, "?subject=reject+");
	            fprintf(fp, mFn);
	            fprintf(fp, "&body=Date:%%20%s%%0AFrom:%%20%s%%0ASubject:%%20%s</a><br>", mInfo[0], mInfo[1], mInfo[2]); // 上長宛承認メールの承認否認メールに対象メールの題名情報などを記録する
	            fprintf(fp, "<br>\n");
	          } else {
	            fprintf(fp, "\n");
	          }
#else
	          fprintf(fp, "\n");
#endif
#ifdef UPDATE_20080515 // "waitlist_"コマンドで送信先アドレスを表示
#ifdef UPDATE_20190110 // 承認一覧メールをHTMLメールにしたときutf-8に変換
              sprintf(mwork, "[\x1B$BAw?.@h%%\"%%I%%l%%9\x1B(B]%s\n", (bMailApprovalHtml ? "<br>" : "")); //[送信先アドレス]
              if (bMailApprovalHtml)
			  {
                CODEPAGE2UTF8N(CP_ISO_JAPANESE, mwork, itm, sizeof(itm)); // utf-8に変換
       	        fputs(itm, fp);
			  } else {
       	        fputs(mwork, fp); // JISのまま
			  }
#else
              fprintf(fp, "[\x1B$BAw?.@h%%\"%%I%%l%%9\x1B(B]\n"); //[送信先アドレス]
#endif
	          while(!(fpsrc = fopen(mRCP, "rt"))) {
                if (bServiceTerminating)
		          break;
                  _sleep(WAIT_TIME);
			  }
              if (fpsrc) {
		        fgets(mwork, sizeof(mwork), fpsrc);
		        fgets(mwork, sizeof(mwork), fpsrc);
                while (p = fgets(mwork, sizeof(mwork), fpsrc)) {
	              fprintf(fp, "%s", &mwork[11]); // エンベロープの送信先
				}
                fclose(fpsrc);
			  }
#ifdef UPDATE_20190110 // 承認一覧メールをHTMLメールにしたときutf-8に変換
	          fprintf(fp, "%s\n", (bMailApprovalHtml ? "<br>" : ""));
#else
	          fprintf(fp, "\n");
#endif
#endif
			}
		  }
		}
        bFile = FindNextFile( hF, &FD);
	  }; 
      FindClose( hF ); 
   }
}

BOOL X_DateChengeFile(char *pSrc, char *pDest) {
  FILE *fpSrc, *fpDest;
  CHAR *p, mt[128] , mData[1024];
  BOOL bHead = TRUE;
  SYSTEMTIME ltime;
  BOOL sts = FALSE;

  gettime(&ltime, (char *)&mt);
  if ((fpSrc = fopen(pSrc, "rt"))) {
    if ((fpDest = fopen(pDest, "wt"))) {
	 sts = TRUE;
	  while((p = fgets(mData, sizeof(mData)-1, fpSrc))) {
		if (bHead  && (mData[0] == '\r' || mData[0] == '\n')) {
          fprintf(fpDest, "Date: %s\n", mt);  // 承認日付を設定
		  bHead = FALSE;
		}
		if (bHead) {
		  if (!strnicmp(mData, "date:", 5)) {
            fprintf(fpDest, "X-Creation-%s", mData); // 作成日をX-ヘッダ付加
		  } else {
			fputs(mData, fpDest);
		  }
		} else {
		  fputs(mData, fpDest);
		}
	  }
	  fflush(fpDest);
	  fclose(fpDest);
	}
	fclose(fpSrc);
  }
  return sts;
}

#ifdef UPDATE_20080620 // TOアドレスが代理人になる機能追加
BOOL X_ReturnCTLMail(DWORD nAction, char *pFrom, char *pTo, char *pSrc, char *pSub)
#else
BOOL X_ReturnCTLMail(DWORD nAction, char *pFrom, char *pSrc, char *pSub)
#endif
{
  SYSTEMTIME lt;
  HANDLE             hF;
  WIN32_FIND_DATA    FD;
  FILE *fp, *fpSrc, *fpDest;
  CHAR mLOG[256], mFn[256], mt[128],  mtB64[256];
  CHAR *p, *q, mMSG[256], mRCP[256], mRCPSrc[256], mSub[1024], mData[1024];
  BOOL bHead = TRUE, bComment = FALSE;
  BOOL sts = FALSE, bCont = FALSE, bFile = FALSE, bHit = FALSE;
#ifdef UPDATE_20090701 // 代理承認処理メールの挿入文の書換えを可能に
  CHAR mProxyMess[256];
#endif
  ///////////////////////////////////////
  GetLocalTime(&lt);
  gettime(&lt, mt);
  translateUue2Base64(mt, strlen(mt), mtB64);
  ///////////////////////////////////////
  strcpy(mMSG, pSrc);
  if ((p = strrchr(mMSG, '.'))){
    *p = '\x0';
	sprintf(mRCPSrc, "%s.RCP", mMSG);
	sprintf(mRCP, "%s.#CP", mMSG);
	strcat(mMSG, "#SG");
  } else {
	return sts;
  }

  if ((fpSrc = fopen(mRCPSrc, "rt"))) {
    fgets(mData, sizeof(mData)-1, fpSrc);
#ifndef UPDATE_20090701 // 代理承認処理メールの挿入文の書換えを可能に
	fclose(fpSrc);
#endif
    if ((fpDest = fopen(mRCP, "wt"))) {
	  fputs(mData, fpDest);
	  fprintf(fpDest, "Return-path: %s\n", pFrom);
	  fprintf(fpDest, "Recipient: %s\n", pFrom);
#ifdef UPDATE_20080620 // TOアドレスが代理人になる機能追加
	  if (nAction == 4 && *pTo) 
	  {
#ifdef UPDATE_20090701 // 代理承認処理メールの挿入文の書換えを可能に
		q = pTo;
		while((p = strchr(q, ',')))
		{
		  *p = '\x0';
	      fprintf(fpDest, "Recipient: %s\n", q);
          q = p+1;
		}
        fprintf(fpDest, "Recipient: %s\n", q);
#else
	    fprintf(fpDest, "Recipient: %s\n", pTo);
#endif
	  }
#endif
	}
#ifdef UPDATE_20090701 // 代理承認処理メールの挿入文の書換えを可能に
	fclose(fpSrc);
#endif
	fclose(fpDest);
  }
  while(!(fp = fopen(mRCP, "rt"))) {
    if (bServiceTerminating)
	  break;
    _sleep(WAIT_TIME);
  }
  fclose(fp);
  while(!CopyFile(mRCP, mRCPSrc, FALSE)) {
    if (bServiceTerminating)
	  break;
	_sleep(WAIT_TIME);
  }
  while(!(fp = fopen(mRCPSrc, "rt"))) {
    if (bServiceTerminating)
	  break;
     _sleep(WAIT_TIME);
  }
  fclose(fp);
  /////////////////////////////////////////////////
  if ((fpSrc = fopen(pSrc, "rt"))) {
    if ((fpDest = fopen(mMSG, "wt"))) {
	 sts = TRUE;
	  while((p = fgets(mData, sizeof(mData)-1, fpSrc))) {
		if (bHead  && (mData[0] == '\r' || mData[0] == '\n')) {
#ifdef UPDATE_20070730 // HTMLメールでの処理に対応
		  if (nAction != 2) {
            fprintf(fpDest, "Content-Type: text/plain; charset=\"ISO-2022-JP\"\n\n"); 
		  } else 
#else
		  if (nAction == 2) 
#endif
		  {
            fprintf(fpDest, "Content-Type: multipart/mixed;\n\tboundary=\"----%s\"\n", mtB64);
		  } 
		  bHead = FALSE;
		}
		if (bHead) {
		  if (!strnicmp(mData, "subject:", 8)) {
			strcpy(mSub, mData);
			switch (nAction) {
			  case 0: fprintf(fpDest, "Subject: =?iso-2022-jp?B?GyRCQlA+XSROPjVHJz1oTX0kTzR7JEs0ME47JDckRiQkJF4kOSEjGyhC?=\n"); // 返信メール 承認処理
				      break;
			  case 1: fprintf(fpDest, "Subject: =?iso-2022-jp?B?Qm9zcyBDaGVjayAbJEJNek5yGyhK?=\n"); // Boss Check 履歴 拒否
				      break;
			  case 2: fprintf(fpDest, "Subject: =?iso-2022-jp?B?Qm9zcyBDaGVjayAbJEJNek5yPGhGQBsoSg==?=\n"); // Boss Check 履歴取得
				      break;
			  case 3: fprintf(fpDest, "Subject: =?iso-2022-jp?B?Qm9zcyBDaGVjayAbJEJNek5yOm89fBsoSg==?=\n"); // Boss Check 履歴削除
				      break;
#ifdef UPDATE_20070713 // 承認者の代理人を設定するオプション
	          case  4:fprintf(fpDest, "Subject: =?iso-2022-jp?B?GyRCQmVNfT41Ryc8VEBfRGobKEI=?=\n"); // 代理承認者設定
			          break;
	          case  5:fprintf(fpDest, "Subject: =?iso-2022-jp?B?GyRCQmVNfT41Ryc8VDNORycbKEI=?=\n"); // 代理承認者確認
			          break;
#endif
			}
		  }
#ifdef UPDATE_20070730 // HTMLメールでの処理に対応
		  else if (!strnicmp(mData, "content-type:", 13))
#else
		  else if (nAction == 2 && !strnicmp(mData, "content-type:", 13))
#endif
		  {
		    bCont = TRUE;
		  } else if (bCont) {
		    if (mData[0] != '\t' && mData[0] != ' ') {
              bCont = FALSE;
		      fputs(mData, fpDest);
			}
		  } else {
			fputs(mData, fpDest);
		  }
		} else {
		  fputs(mData, fpDest);
		  if (!bComment) { // コメント挿入
			switch (nAction)
			{
#ifdef UPDATE_20090701 // 代理承認処理メールの挿入文の書換えを可能に
		      case 0: GetProfileStringEx(BOSSCHECK_REG, "proxymessage00", "\x1B$BBP>]$N>5G'=hM}$O4{$K40N;$7$F$$$^$9!#\x1B(B", mProxyMess, sizeof(mProxyMess));
					  strcat(mProxyMess, "\n");
				      fprintf(fpDest, mProxyMess); // 対象の承認処理は既に完了しています。
				      break;
		      case 1: GetProfileStringEx(BOSSCHECK_REG, "proxymessage10", "\x1B$BBP>]$N=hM}$O\x1B(BBoss Check \x1B$B5!G=4IM}<T$N$_$,MxMQ$G$-$^$9!#\x1B(B", mProxyMess, sizeof(mProxyMess));
					  strcat(mProxyMess, "\n");
				      fprintf(fpDest, mProxyMess); // 対象の処理はBoss Check 機能管理者のみが利用できます。
				      break;
		      case 2: fprintf(fpDest, "\n------%s\n", mtB64);
                      fprintf(fpDest, "Content-Type: text/plain; charset=\"ISO-2022-JP\"\n\n"); 
					  GetProfileStringEx(BOSSCHECK_REG, "proxymessage20", "Boss Check \x1B$BMzNr$r<hF@$7$^$7$?!#\x1B(B", mProxyMess, sizeof(mProxyMess));
					  strcat(mProxyMess, "\n");
				      fprintf(fpDest, mProxyMess); // Boss Check 履歴を取得しました。
				      break;
		      case 3: GetProfileStringEx(BOSSCHECK_REG, "proxymessage30", "Boss Check \x1B$BMzNr$r:o=|$7$^$7$?!#\x1B(B", mProxyMess, sizeof(mProxyMess));
					  strcat(mProxyMess, "\n");
				      fprintf(fpDest, mProxyMess); // Boss Check 履歴を削除しました。
				      break;
#ifdef UPDATE_20070713 // 承認者の代理人を設定するオプション
	          case  4:if (*pSub)
					  {
					    GetProfileStringEx(BOSSCHECK_REG, "proxymessage40", "\x1B$B0J2<$N%%f!<%%6$rBeM}>5G'<T$H$7$F@_Dj$7$^$7$?!#\x1B(B\n'%s'", mProxyMess, sizeof(mProxyMess));
						strcat(mProxyMess, "\n");
						if (strstr(mProxyMess, "%s"))
						{
				          fprintf(fpDest, mProxyMess, pSub); // 代理承認者設定
						} else {
				          fprintf(fpDest, mProxyMess); // 代理承認者設定
						}
					  } else {
					    GetProfileStringEx(BOSSCHECK_REG, "proxymessage41", "\x1B$BBeM}>5G'<T@_Dj$r2r=|$7$^$7$?!#\x1B(B", mProxyMess, sizeof(mProxyMess));
						strcat(mProxyMess, "\n");
			            fprintf(fpDest, mProxyMess); // 代理承認者解除
					  }
			          break;
	          case  5:if (*pSub)
					  {
					    GetProfileStringEx(BOSSCHECK_REG, "proxymessage50", "\x1B$B0J2<$N%%f!<%%6$,BeM}>5G'<T$H$7$F@_Dj$5$l$F$$$^$9!#\x1B(B\n'%s'", mProxyMess, sizeof(mProxyMess));
					    strcat(mProxyMess, "\n");
						if (strstr(mProxyMess, "%s"))
						{
				          fprintf(fpDest, mProxyMess, pSub); // 代理承認者確認　既設定
						} else {
				          fprintf(fpDest, mProxyMess); // 代理承認者確認　既設定
						}
					  } else {
					    GetProfileStringEx(BOSSCHECK_REG, "proxymessage51", "\x1B$BBeM}>5G'<T$O@_Dj$5$l$F$$$^$;$s!#\x1B(B\n", mProxyMess, sizeof(mProxyMess));
					    strcat(mProxyMess, "\n");
				        fprintf(fpDest, mProxyMess); // 代理承認者確認 未設定
					  }
			          break;
#endif
#else
		      case 0: fprintf(fpDest, "\x1B$BBP>]$N>5G'=hM}$O4{$K40N;$7$F$$$^$9!#\x1B(B\n"); // 対象の承認処理は既に完了しています。
				      break;
		      case 1: fprintf(fpDest, "\x1B$BBP>]$N=hM}$O\x1B(BBoss Check \x1B$B5!G=4IM}<T$N$_$,MxMQ$G$-$^$9!#\x1B(B\n"); // 対象の処理はBoss Check 機能管理者のみが利用できます。
				      break;
		      case 2: fprintf(fpDest, "\n------%s\n", mtB64);
                      fprintf(fpDest, "Content-Type: text/plain; charset=\"ISO-2022-JP\"\n\n"); 
				      fprintf(fpDest, "Boss Check \x1B$BMzNr$r<hF@$7$^$7$?!#\x1B(B\n"); // Boss Check 履歴を取得しました。
				      break;
		      case 3: fprintf(fpDest, "Boss Check \x1B$BMzNr$r:o=|$7$^$7$?!#\x1B(B\n"); // Boss Check 履歴を削除しました。
				      break;
#ifdef UPDATE_20070713 // 承認者の代理人を設定するオプション
	          case  4:if (*pSub)
					    fprintf(fpDest, "\x1B$B0J2<$N%%f!<%%6$rBeM}>5G'<T$H$7$F@_Dj$7$^$7$?!#\x1B(B\n'%s'\n", pSub); // 代理承認者設定
				      else
				        fprintf(fpDest, "\x1B$BBeM}>5G'<T@_Dj$r2r=|$7$^$7$?!#\x1B(B\n");
			          break;
	          case  5:if (*pSub)
				        fprintf(fpDest, "\x1B$B0J2<$N%%f!<%%6$,BeM}>5G'<T$H$7$F@_Dj$5$l$F$$$^$9!#\x1B(B\n'%s'\n", pSub); // 代理承認者確認　設定有り
				      else
				        fprintf(fpDest, "\x1B$BBeM}>5G'<T$O@_Dj$5$l$F$$$^$;$s!#\x1B(B\n\n"); // 代理承認者確認 未設定
			          break;
#endif
#endif
			}
		    fprintf(fpDest, "[Request code] %s\n", &mSub[8]); // 対象の承認処理は既に完了しています。
#ifdef UPDATE_20070730 // HTMLメールでの処理に対応
			if (nAction != 2 && nAction != 3 ) {
              fprintf(fpDest, "\n.\n");
			  break;
			} else 
#endif
            if (nAction == 2) {
			  sprintf(mLOG, "%s%s\\log\\%s.txt", mMailSpoolDir, mMailApprovalDir, (*pSub ? pSub : "*"));
              hF = FindFirstFile(mLOG, &FD);
              if (hF != INVALID_HANDLE_VALUE) {
                bFile = TRUE;
                while (bFile) {
	              if (!(!stricmp(FD.cFileName, ".") ||
	                    !stricmp(FD.cFileName, ".."))) {
					bHit = TRUE;
		            sprintf(mFn, "%s%s\\log\\%s", mMailSpoolDir, mMailApprovalDir, FD.cFileName);
		            if ((fp = fopen(mFn, "rt"))) { // 履歴を添付
                      fprintf(fpDest, "\n------%s\n", mtB64);
                      fprintf(fpDest, "Content-Type: text/txt;\n\tname=\"%s\"\n", FD.cFileName); 
					  fprintf(fpDest, "Content-Disposition: attachment;\n\tfilename=\"%s\"\n\n", FD.cFileName); 
                      while (fgets(mData, sizeof(mData), fp)) {
                        if (mData[0] != '.' && mData[1] != '\n')
                          fputs(mData, fpDest);
					  }
                      fclose(fp);
					}
				  }
                  bFile = FindNextFile( hF, &FD);
				}
			  }
              FindClose( hF ); 
			  if (!bHit)
                fprintf(fpDest, "\x1B$B;XDj$7$?MzNr$O8+$D$+$j$^$;$s$G$7$?!#\x1B(B\n");
              fprintf(fpDest, "\n------%s--\n", mtB64);
              fprintf(fpDest, "\n.\n");
			  break;
			} else if (nAction == 3) { // 履歴を削除
			  sprintf(mLOG, "%s%s\\log\\%s.txt", mMailSpoolDir, mMailApprovalDir, (*pSub ? pSub : "*"));
              hF = FindFirstFile(mLOG, &FD);
              if (hF != INVALID_HANDLE_VALUE) {
                bFile = TRUE;
                while (bFile) {
	              if (!(!stricmp(FD.cFileName, ".") ||
	                    !stricmp(FD.cFileName, ".."))) {
					bHit = TRUE;
	                sprintf(mFn, "%s%s\\log\\%s", mMailSpoolDir, mMailApprovalDir, FD.cFileName);
					DeleteFile(mFn);
                    fprintf(fpDest, "%s \x1B$B$r:o=|$7$^$7$?!#\x1B(B\n", FD.cFileName);
				  }
                  bFile = FindNextFile( hF, &FD);
				}
			  }
              FindClose( hF ); 
			  if (!bHit)
                fprintf(fpDest, "\x1B$B;XDj$7$?MzNr$O8+$D$+$j$^$;$s$G$7$?!#\x1B(B\n");
              fprintf(fpDest, "\n.\n");
			  break;
			}
		    bComment = TRUE;
		  }
		}
	  }
	  fflush(fpDest);
	  fclose(fpDest);
	}
	fclose(fpSrc);
  }
  while(!(fp = fopen(mMSG, "rt"))) {
    if (bServiceTerminating)
	  break;
    _sleep(WAIT_TIME);
  }
  fclose(fp);
  while(!CopyFile(mMSG, pSrc, FALSE)) {
    if (bServiceTerminating)
	  break;
	_sleep(WAIT_TIME);
  }
  while(!(fp = fopen(pSrc, "rt"))) {
    if (bServiceTerminating)
	  break;
     _sleep(WAIT_TIME);
  }
  fclose(fp);
  DeleteFile(mRCP);
  DeleteFile(mMSG);
  return sts;
}

#ifdef UPDATE_20140623A // 上長承認結果ログに送信元IPの記録を追加した。
void ApprovalLog(BOOL bAction, CHAR *pIP, CHAR *pFrom, CHAR *pMail) 
#else
void ApprovalLog(BOOL bAction, CHAR *pFrom, CHAR *pMail) 
#endif
{
   char   mdata[256], mFn[256];
   SYSTEMTIME lt;
   FILE *fp;
#ifdef UPDATE_20140623 // 上長承認結果ログに送信先アドレスの記録を追加した。
   FILE *fpr;
   char mRCPFn[256];
   char mLine[256];
   char *p;
   BOOL b1st = TRUE;
#endif

   GetLocalTime(&lt);
   sprintf(mdata, "%02d%02d%02d", (lt.wYear%100), lt.wMonth, lt.wDay);
   sprintf(mFn, "%s%s\\log\\%s.txt", mMailSpoolDir, mMailApprovalDir, mdata);
/* タスクスケジューラで自動判定時にもこのログの書き込みを行うので、占有不可なので　UPDATE_20260606　は行わない
#ifdef UPDATE_20260606 // ログの書き込み信頼性向上対策
   WaitForSingleObject(g_hMutexApprovalLog, INFINITE);  // 排他開始
   fpApprovalLog = fp = OpenCloseLog(fpApprovalLog, mDTApprovalLog, "log", "", lt);
   if (fpApprovalLog) 
   {
     sprintf(mdata, "[%04d/%02d/%02d:%02d:%02d:%02d]", lt.wYear, lt.wMonth, lt.wDay, lt.wHour, lt.wMinute, lt.wSecond);
#ifdef UPDATE_20140623 // 上長承認結果ログに送信先アドレスの記録を追加した。
#ifdef UPDATE_20140623B // // 上長承認機能の履歴作成時の詳細さ設定フラグを追加した（送信元IPや送信先を）0:含まない(デフォルト) 1:含む
     if (nMailApprovalLogLevel)
#endif
	 {
#ifdef UPDATE_20140623A // 上長承認結果ログに送信元IPの記録を追加した。
	   fprintf(fpApprovalLog, "%s [%s] 承認者=%s (%s) 承認メール=%s 送信先=", mdata, (bAction ? "承認" : (bMailApprovalRevers ? "拒否" : "却下")), pFrom, pIP, pMail);
#else
	   fprintf(fpApprovalLog, "%s [%s] 承認者=%s 承認メール=%s 送信先=", mdata, (bAction ? "承認" : (bMailApprovalRevers ? "拒否" : "却下")), pFrom, pMail);
#endif
       sprintf(mRCPFn, "%s%s\\%s.RCP", mMailSpoolDir, mMailApprovalDir, pMail);
       if ((fpr = fopen(mRCPFn, "rt"))) 
	   {
	     fgets(mLine, sizeof(mLine), fpr); // Messages-ID
	     fgets(mLine, sizeof(mLine), fpr); // mail from address
	     while(fgets(mLine, sizeof(mLine), fpr))
		 {
           strtok(mLine,"\r\n");
	       if ((p = strstr(mLine," ")))
		   {
		     if (b1st)
		       p++;
		     fprintf(fpApprovalLog, "%s", p);
		     b1st = FALSE;
		   }
		 }
	     fclose(fpr);
	   }
       fprintf(fpApprovalLog, "\n");
	 }
#ifdef UPDATE_20140623B // // 上長承認機能の履歴作成時の詳細さ設定フラグを追加した（送信元IPや送信先を）0:含まない(デフォルト) 1:含む
	 else {
	   fprintf(fpApprovalLog, "%s [%s] 承認者=%s 承認メール=%s\n", mdata, (bAction ? "承認" : (bMailApprovalRevers? "拒否" : "却下")), pFrom, pMail);
	 }
#endif
#else
	 fprintf(fpApprovalLog, "%s [%s] 承認者=%s 承認メール=%s\n", mdata, (bAction ? "承認" : (bMailApprovalRevers? "拒否" : "却下")), pFrom, pMail);
#endif
     fflush(fp);
   }
#else
*/
#ifdef UPDATE_20260603 // ログ出力の排他処理化を追加
   WaitForSingleObject(g_hMutexApprovalLog, INFINITE);  // 排他開始
#endif
   if ((fp = fopen(mFn, "at"))) 
   {
     sprintf(mdata, "[%04d/%02d/%02d:%02d:%02d:%02d]", lt.wYear, lt.wMonth, lt.wDay, lt.wHour, lt.wMinute, lt.wSecond);
#ifdef UPDATE_20140623 // 上長承認結果ログに送信先アドレスの記録を追加した。
#ifdef UPDATE_20140623B // // 上長承認機能の履歴作成時の詳細さ設定フラグを追加した（送信元IPや送信先を）0:含まない(デフォルト) 1:含む
     if (nMailApprovalLogLevel)
#endif
	 {
#ifdef UPDATE_20140623A // 上長承認結果ログに送信元IPの記録を追加した。
	   fprintf(fp, "%s [%s] 承認者=%s (%s) 承認メール=%s 送信先=", mdata, (bAction ? "承認" : (bMailApprovalRevers ? "拒否" : "却下")), pFrom, pIP, pMail);
#else
	   fprintf(fp, "%s [%s] 承認者=%s 承認メール=%s 送信先=", mdata, (bAction ? "承認" : (bMailApprovalRevers ? "拒否" : "却下")), pFrom, pMail);
#endif
       sprintf(mRCPFn, "%s%s\\%s.RCP", mMailSpoolDir, mMailApprovalDir, pMail);
       if ((fpr = fopen(mRCPFn, "rt"))) 
	   {
	     fgets(mLine, sizeof(mLine), fpr); // Messages-ID
	     fgets(mLine, sizeof(mLine), fpr); // mail from address
	     while(fgets(mLine, sizeof(mLine), fpr))
		 {
           strtok(mLine,"\r\n");
	       if ((p = strstr(mLine," ")))
		   {
		     if (b1st)
		       p++;
		     fprintf(fp, "%s", p);
		     b1st = FALSE;
		   }
		 }
	     fclose(fpr);
	   }
       fprintf(fp, "\n");
	 }
#ifdef UPDATE_20140623B // // 上長承認機能の履歴作成時の詳細さ設定フラグを追加した（送信元IPや送信先を）0:含まない(デフォルト) 1:含む
	 else {
	   fprintf(fp, "%s [%s] 承認者=%s 承認メール=%s\n", mdata, (bAction ? "承認" : (bMailApprovalRevers? "拒否" : "却下")), pFrom, pMail);
	 }
#endif
#else
	 fprintf(fp, "%s [%s] 承認者=%s 承認メール=%s\n", mdata, (bAction ? "承認" : (bMailApprovalRevers? "拒否" : "却下")), pFrom, pMail);
#endif
#ifndef UPDATE_20260603 // ログ出力の排他処理化を追加
     fclose(fp);
#endif
   }
//#endif
#ifdef UPDATE_20260603 // ログ出力の排他処理化を追加
    ReleaseMutex(g_hMutexApprovalLog);  // 排他終了
#endif
}
#endif

#ifdef UPDATE_20080531 // LGWAN使用時の振分け機能の強化
//bAliases = GetAliases((LPCTSTR)SOFT_ALIASES_REG, (char *)user, (char *)pdom, pOpt);
#ifdef UPDATE_20141023A // メールアドレス単位でもGatway.datでフォワード指定可能にする対策
BOOL GetGatewayList(char *mDom, char *pTo, char *mGate, int *nPort, BOOL *bSSL)
#else
BOOL GetGatewayList(char *mDom, char *mGate, int *nPort, BOOL *bSSL)
#endif
{
  FILE    *fpSSL;
  char    *fsts, *pgate, *pport, *pssl, mSSL[1024];
  BOOL    bSts = FALSE; //ゲートウエィリストに載っているか否か
  DWORD   stsDom = 1, nLeft, nRight, nDom;
  char    *pSSL;
  CHAR    *pg[3], *pp[3], *ps[3];
  int     np[3];
  CHAR    *pSep;
  SYSTEMTIME lt;
  INT     nSel = 0, nRoot = 0, nPt = *nPort;
  int     nport = 25;
#ifdef UPDATE_20141023 // メールアドレス単位でもGatway.datでフォワード指定可能にする対策
  BOOL    bAddr = FALSE;
#endif

  GetSystemTime(&lt);
   sprintf(mSSL,"%sgateway.dat", mProgPath);
   fpSSL = fopen(mSSL, "rt");
   if (fpSSL) {
     fsts = fgets(mSSL,sizeof(mSSL), fpSSL);
     while(fsts || !feof(fpSSL)) {
	   if (mSSL[0] != '\n' && mSSL[0] != '\'' && mSSL[0] != ' ') {
	     pssl   = pport = NULL;
         *nPort = nport; // SMTP over SSL default port
         *bSSL  = FALSE;
 	     strtok(mSSL, "\n");
	     pgate = strstr(mSSL, ",");
	     if (pgate) {
	 	   *pgate = '\x0';
		   pgate++;
		   nRoot = 0;
		   do {
  		     np[nRoot] = 0;
	         ps[nRoot] = NULL;
		     pg[nRoot] = pgate;
			 if ((pSep = strchr(pgate, '\t'))) {
			   *pSep = '\x0';
			   pgate = pSep+1;
			 }
	         pp[nRoot] = strpbrk(pg[nRoot], ",:");
	         if (pp[nRoot]) {
	 	       *pp[nRoot] = '\x0';
		       pp[nRoot]++;
  		       np[nRoot] = atoi(pp[nRoot]);
	           ps[nRoot] = strstr(pp[nRoot], "*");
			 }
		     nRoot++;
		   } while (pSep && (nRoot < 3)); // 3箇所まで
		   nSel = (lt.wSecond/10)%nRoot;  // 負荷分散１０秒単位
		   strcpy(mGate, pg[nSel]);
		   *nPort = (np[nSel] ? np[nSel] : nPt);
	       pssl = ps[nSel];
		 }
#ifdef UPDATE_20141023A // メールアドレス単位でもGatway.datでフォワード指定可能にする対策
         bAddr = (strchr(mSSL,'@') ? TRUE : FALSE); // メールアドレスフォワードか否か
#endif
		 if ((pSSL = strstr(mSSL,"*"))) {
           *pSSL = '\x0';
           nLeft  = strlen(mSSL);
		   if (nLeft == 0)
			 stsDom = 0;
		   else
#ifdef UPDATE_20141023A // メールアドレス単位でもGatway.datでフォワード指定可能にする対策
			 stsDom = _strnicmp(mSSL, (bAddr ? pTo : mDom), nLeft);
#else
		     stsDom = _strnicmp(mSSL, mDom, nLeft);
#endif
           nRight = strlen(pSSL+1);
		   if (nRight) {
#ifdef UPDATE_20141023A // メールアドレス単位でもGatway.datでフォワード指定可能にする対策
		     nDom   = strlen(bAddr ? pTo : mDom);
#else
		     nDom   = strlen(mDom);
#endif
		     if (nDom < nLeft+nRight) {
 		       stsDom = -1;
			 } else {
#ifdef UPDATE_20141023A // メールアドレス単位でもGatway.datでフォワード指定可能にする対策
			   if (stsDom || _strnicmp(pSSL+1, (bAddr ? &pTo[nDom-nRight] :  &mDom[nDom-nRight] ), nRight))
#else
			   if (stsDom || _strnicmp(pSSL+1, &mDom[nDom-nRight], nRight))
#endif
			     stsDom = -1;
			 }
		   }
		 } else {
#ifdef UPDATE_20141023A // メールアドレス単位でもGatway.datでフォワード指定可能にする対策
		   stsDom = _stricmp(mSSL, (bAddr ? pTo : mDom));
#else
		   stsDom = _stricmp(mSSL, mDom);
#endif
		 }
		 if (stsDom == 0)
		 {
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

   return bSts;
}

#ifdef LGWAN_ON        // LGWAN機能拡張
void LGWANControl(PSmtpContext pContext)
{  
  FILE *fp, *fpd;
  BOOL bLocal, bIn = FALSE;
#ifdef UPDATE_20150918 // 送信元エンベロープがエイリアスと一致したとき、正しく送信元エンベロープが出力できない不具合
  BOOL bSSL = FALSE, bAliases = FALSE, bAls, bSet, bHit = FALSE, bReWrite = FALSE;  // FROMを書き換え
#else
  BOOL bSSL = FALSE, bAliases, bAls, bSet, bHit = FALSE, bReWrite = FALSE;  // FROMを書き換え
#endif
  INT  nPort = 25;
#ifdef UPDATE_20141115 // 独自アカウント運用時で最大文字数を128byteまで拡張可能にする対策をした。
  CHAR *pdom, *prcptdom, mOpt[512]="", mGate[512], mUser[512], mRCPT[512], mData[512];
#else
  CHAR *pdom, *prcptdom, mOpt[128]="", mGate[128], mUser[512], mRCPT[512], mData[512];
#endif
  CHAR mLGRCP[512];
#ifdef UPDATE_20141023A // メールアドレス単位でもGatway.datでフォワード指定可能にする対策
  CHAR mTo[512];
#endif

  if (bLGWAN) {
#ifdef UPDATE_20071204  // メッセージＩＤ採番処理を修正(Bym10id)
    sprintf(mLGRCP, "%s%s.RCL", mTempDir, pContext->mMsgId);
#else
    sprintf(mLGRCP, "%sB%010lu.RCL", mTempDir, pContext->nMsgId);
#endif
  //CopyFile(pContext->mFnData, mLGMSG, TRUE);
  //CopyFile(pContext->mFnHead, mLGRCP, TRUE);
    bReWrite = FALSE;  // MAIL FROMを書き換え
	bHit = FALSE;
	bAliases = FALSE;
#ifdef UPDATE_20140809 //転送処理時のファイル読込みでバイナリデータが含まれていてもデータが切れてしまわない対策を追加。
	if ((fp = fopen(pContext->mFnHead, "rb")))
#else
	if ((fp = fopen(pContext->mFnHead, "rt")))
#endif
	{
      fgets(mData, sizeof(mData), fp);
      fgets(mData, sizeof(mData), fp);
	  strtok(mData, "\r\n");
      strcpy(mUser, &mData[13]);
      if ((pdom = strstr(mUser, "@")))
	  {
	    *pdom = '\x0';
        bAliases = GetAliases((LPCTSTR)SOFT_ALIASES_REG, (char *)mUser, (char *)pdom, mOpt);
	  }
      if (bAliases && mOpt[1] == ',')
      {
        while(fgets(mData, sizeof(mData), fp))
		{
	      strtok(mData, "\r\n");
		  strcpy(mRCPT, &mData[11]);
#ifdef UPDATE_20141023A // メールアドレス単位でもGatway.datでフォワード指定可能にする対策
		  strcpy(mTo, &mData[11]);
#endif
          if ((prcptdom = strstr(mRCPT, "@")))
		  {
	        *prcptdom = '\x0';
		    bAls = GetAliases((LPCTSTR)SOFT_ALIASES_REG, (char *)mRCPT, (char *)prcptdom, NULL);
		  }
		  if (!bAls || (bAls && !CheckDomain(mRCPT))) //グローバルドメインであることを確認
		  {
            if ((pdom = strstr(mData, "@")))
			{
	          *pdom = '\x0';
			  pdom++;
#ifdef UPDATE_20141023A // メールアドレス単位でもGatway.datでフォワード指定可能にする対策
              if (GetGatewayList(pdom, mTo, mGate, &nPort, &bSSL))
#else
              if (GetGatewayList(pdom, mGate, &nPort, &bSSL))
#endif
			  {
			    if (IP_COMP(&mOpt[2], mGate))  // IPアドレスを確認
				{
                  bHit = TRUE;  // MAIL FROMを書き換え
                  break;
				}
              }
            }
		  }
		}
      } else {
#ifdef UPDATE_20150918 // 送信元エンベロープがエイリアスと一致したとき、正しく送信元エンベロープが出力できない不具合
		if (!bAliases)
#endif
		{
		  if (pdom) {  // NULLでないとき 20080618
		    *pdom = '@';
		  }
		}
        if (!CheckDomain(mUser))
		{
          while(fgets(mData, sizeof(mData), fp))
		  {
	        strtok(mData, "\r\n");
		    strcpy(mRCPT, &mData[11]);
            if ((prcptdom = strstr(mRCPT, "@")))
			{
	          *prcptdom = '\x0';
		      bAls = GetAliases((LPCTSTR)SOFT_ALIASES_REG, (char *)mRCPT, (char *)prcptdom, (char *)mOpt);
			}
		    if (bAls && CheckDomain(mRCPT)) //ローカルドメインであることを確認
			{
		      if (mOpt[0] == '0' && mOpt[1] == ',')
			  {
				if (IP_COMP(&mOpt[2], pContext->PeerAddr))  // IPアドレスを確認
				{
                  bIn = bReWrite = TRUE;  // RCPT TOを書き換え
                  break;
				}
			  }
			}
		  }
		}
	  }
      fclose(fp);
	}
  }
  if (bAliases)
  {
	if (mOpt[0] == '0') 
	{
      bReWrite = !bHit;  // RCPT TOを書き換え
	} else
	if (mOpt[0] == '2')
	{
      bReWrite = bHit;  // MAIL FROMを書き換え
	} 
  }
  if (bReWrite) // MAIL FROMを書換え
  {
#ifdef UPDATE_20140809 //転送処理時のファイル読込みでバイナリデータが含まれていてもデータが切れてしまわない対策を追加。
	if ((fp = fopen(pContext->mFnHead, "rb")))
#else
	if ((fp = fopen(pContext->mFnHead, "rt")))
#endif
	{
	  bSet = FALSE; // 変更有無フラグ
	  if ((fpd = fopen(mLGRCP, "wt")))
	  {
	    bSet = TRUE; // 変更有無フラグ
	    fgets(mData, sizeof(mData), fp);
		fputs(mData, fpd);
	    fgets(mData, sizeof(mData), fp);
        fprintf(fpd, "Return-path: %s\n", mUser); // MAIL FROMを書換え
		strtok(mData, "\r\n");
        ReplaceMSGHeader(pContext, &mData[13], mUser, NULL, NULL); // FROM:ヘッダを書換え
		bLocal = CheckDomain(mUser);
        while(fgets(mData, sizeof(mData), fp))
		{
		  if (bLocal || !bLocal && bIn) {
		    strcpy(mRCPT, &mData[11]);
		    strtok(mRCPT,"\r\n");
            if ((prcptdom = strstr(mRCPT, "@")))
			{
	          *prcptdom = '\x0';
		      bAls = GetAliases((LPCTSTR)SOFT_ALIASES_REG, (char *)mRCPT, (char *)prcptdom, mOpt);
		      if (bAls && (mOpt[0] == '0') && CheckDomain(mRCPT)) //グローバルドメインであることを確認
			  {
                fprintf(fpd, "Recipient: %s\n", mRCPT); // RCPT TOを書換え
				strtok(mData, "\r\n");
                ReplaceMSGHeader(pContext, NULL, NULL, &mData[11], mRCPT); // TO:,CC:ヘッダを書換え
			  } else {
		       fputs(mData, fpd);
			  }
			} else {
              fputs(mData, fpd);
			}
		  } else {
	        fputs(mData, fpd);
		  }
		}
		fflush(fpd);
	    fclose(fpd);
	  }
	  fclose(fp);
	}
	// エンベロープ情報の変更がある場合の処理
	if (bSet)
	{
	  // 変更したヘッダファイルがオープン可能になるまでウエイト
	  while(!(fp = fopen(mLGRCP, "rt"))) {
      if (bServiceTerminating)
	    break;
	   _sleep(WAIT_TIME);
	  }
	  fclose(fp);
	  // ヘッダファイルを一旦削除
	  DeleteFile(pContext->mFnHead);
	  while((fp = fopen(pContext->mFnHead, "rt")))
	  {
	    fclose(fp);
        if (bServiceTerminating)
	      break;
	     _sleep(WAIT_TIME);
 	    DeleteFile(pContext->mFnHead);
	  }
	  // 新しいエンベロープファイルと入れ替え
      MoveFileEx(mLGRCP, pContext->mFnHead, MOVEFILE_COPY_ALLOWED | MOVEFILE_WRITE_THROUGH);
	  while(!(fp = fopen(pContext->mFnHead, "rt")))
	  {
        if (bServiceTerminating)
	      break;
	     _sleep(WAIT_TIME);
	    MoveFileEx(mLGRCP, pContext->mFnHead, MOVEFILE_COPY_ALLOWED | MOVEFILE_WRITE_THROUGH);
	  }
	  fclose(fp);
	}
  }
}

// LGWAN用の"FROM:","TO:"ヘッダの書換え
void ReplaceMSGHeader(PSmtpContext pContext, CHAR *pFrom, CHAR *pFromDest, CHAR *pTo, CHAR *pToDest)
{
	FILE *fp, *fpd;
	BOOL bNext = FALSE, bHead = TRUE;
	BOOL bFrom = FALSE, bTo = FALSE, bCc = FALSE;
	CHAR *p, *ps, mData[1024], mLine[1024], mLGMSG[512];

	if (bCHGHEADER)
	{
#ifdef UPDATE_20071204  // メッセージＩＤ採番処理を修正(Bym10id)
      sprintf(mLGMSG, "%s%s.MSL", mTempDir, pContext->mMsgId);
#else
      sprintf(mLGMSG, "%sB%010lu.MSL", mTempDir, pContext->nMsgId);
#endif
	  if (pFrom) {
        _strlwr(pFrom);
	  }
	  if (pTo) {
        _strlwr(pTo);
	  }
	  if ((fp = fopen(pContext->mFnData, "rt"))) 
	  {
	    if ((fpd = fopen(mLGMSG, "wt"))) 
		{
	      while ((fgets(mData, sizeof(mData), fp)))
		  {
		    if (mData[0] == '\r' || mData[0] == '\n') { // ヘッダの終わり
		      bHead = FALSE;
			}
		    if (bHead) 
			{
              ps = mData;
		      if ((pFrom && !strnicmp(mData, "from:", 5)) ||
                  (pTo && (!strnicmp(mData, "to:", 3) || !strnicmp(mData, "cc:", 3))) ||
			      (bNext && (*ps == ' ' || *ps == '\t')))
			  {
		        if (!bNext) {
				  strcpy(mLine, mData);
				  bFrom = bTo = bCc = FALSE;
			      if (!strnicmp(mData, "from:", 5)) {
				    bFrom = TRUE;
			        ps = &mData[5];
				  } else if (!strnicmp(mData, "to:", 3)) {
				    bTo = TRUE;
			        ps = &mData[3];
				  } else if (!strnicmp(mData, "cc:", 3)) {
				    bCc = TRUE;
			        ps = &mData[3];
				  }
		          bNext = TRUE;
				} else {
#ifdef UPDATE_20140618 // LGWANオプションでの同報先ヘッダの置換えで同じアドレスだけが出力される不具合
				  strcpy(mLine, mData);
#endif
			      ps = mData;
				}
		        while(*ps == ' ' || *ps == '\t') {
		          ps++;
				}
			    // アドレス差替え
			    _strlwr(ps);
			    if (pFrom && bFrom) {
				  _strlwr(pFrom);
				  if ((p = strstr(mData, pFrom))) {
				    strcpy(mData, mLine);
				    *p = '\x0';
				    fputs(mData, fpd);
				    fputs(pFromDest, fpd);
				    p+=strlen(pFrom);
				    fputs(p, fpd);
				  } else {
				    fputs(mLine, fpd);
				  }
				} else if (pTo && (bTo || bCc)) {
				  _strlwr(pTo);
				  if ((p = strstr(mData, pTo))) {
				    strcpy(mData, mLine);
				    *p = '\x0';
				    fputs(mData, fpd);
				    fputs(pToDest, fpd);
				    p+=strlen(pTo);
				    fputs(p, fpd);
				  } else {
				    fputs(mLine, fpd);
				  }
				}
			  } else {
		        bNext = FALSE;
			    fputs(mData, fpd);
			  }
			} else {
			  fputs(mData, fpd);
			}
		  }
		  fflush(fpd);
		  fclose(fpd);
		}
	    fclose(fp);
	  }
      // 変更したヘッダファイルがオープン可能になるまでウエイト
      while(!(fp = fopen(mLGMSG, "rt"))) {
        if (bServiceTerminating)
          break;
  	     _sleep(WAIT_TIME);
	  }
	  fclose(fp);
	  // メッセージファイルを一旦削除
	  DeleteFile(pContext->mFnData);
	  while((fp = fopen(pContext->mFnData, "rt")))
	  {
	    fclose(fp);
        if (bServiceTerminating)
	      break;
	     _sleep(WAIT_TIME);
 	     DeleteFile(pContext->mFnData);
	  }
	  // 新しいメッセージファイルと入れ替え
      MoveFileEx(mLGMSG, pContext->mFnData, MOVEFILE_COPY_ALLOWED | MOVEFILE_WRITE_THROUGH);
	  while(!(fp = fopen(pContext->mFnData, "rt")))
	  {
        if (bServiceTerminating)
	      break;
	      _sleep(WAIT_TIME);
	    MoveFileEx(mLGMSG, pContext->mFnData, MOVEFILE_COPY_ALLOWED | MOVEFILE_WRITE_THROUGH);
	  }
	  fclose(fp);
   }
}
#endif
#endif

#ifdef UPDATE_20080617 // 上長承認機能で添付ファイルの拡張子による承認条件追加
//////////////////////////////////
// PDF形式が含まれるメールは上長承認する場合の書式
// .pdf
// ~*
// PDF形式が含まれるメールは上長承認しない場合の書式
// ~.pdf
// *
//////////////////////////////////
BOOL QueryMIMEType(CHAR *pFn, CHAR *pMIME, BOOL *pbHit)
{
   HKEY hKey;
   FILE *fp;
   DWORD i, retCode, nType, cbData;
   CHAR *p, mData[80], mMIME[128];
   BOOL bRoule = TRUE, bsts = FALSE;

   *pbHit = FALSE;
   if (!*pFn)  // 個別指定が無い場合は添付全てを承認対象
   {
	 *pbHit = TRUE;
	 return TRUE;
   }
   if (!*pMIME)
   {
	 return FALSE;
   }
   if ((fp = fopen(pFn, "rt")))
   {
	 while(fgets(mData, sizeof(mData), fp)) 
	 {
	   if (mData[0] == '\r' ||
		   mData[0] == '\n' ||
		   mData[0] == '\'' ||
		   mData[0] == ' ' )
	   {
		continue;
	   }
	   if (mData[0] == '~') {
		 p = &mData[1];
		 bsts = FALSE;
		 bRoule = FALSE;  // 非対称
	   } else {
		 p = mData;
		 bsts = FALSE;
		 bRoule = TRUE;   // 対象
	   }
	   strtok(mData, "\r\n");
	   if (*pMIME == '.') // 拡張子で直接比較(uuencode)
	   {
	     if (!stricmp(p, pMIME) || *p == '*')
		 {
		   if (*p != '*')
		   {
		     *pbHit = TRUE;
		   }
		   bsts = (bRoule ? TRUE : FALSE);
		   break;
		 }
	   /*
	   } else {  // MIME形式から拡張子を参照し比較(MIME Type)
         do {
	       if (i > 0)
	         _sleep(0);
           retCode = 
             RegOpenKeyEx(HKEY_CLASSES_ROOT,       // Key handle at root level.
                          (LPCTSTR)p,  // Path name of child key.
                          0,              // Reserved.
                          KEY_READ,       // Requesting read access.
                          &hKey);         // Address of key to be returned.
		 } while(retCode == ERROR_INVALID_HANDLE && i++ < 3);  // ERROR_INVALID_HANDLE = 6
         if (retCode == ERROR_SUCCESS) {
		   cbData = sizeof(mMIME);
           retCode =
	         RegQueryValueEx(hKey,            // handle of key to query 
                            (LPSTR)"Content Type", // address of name of value to query 
                            0,                // reserved 
                            &nType,  // address of buffer for value type 
                            (LPBYTE)mMIME,  // address of data buffer 
                            &cbData  // address of data buffer size 
                        ); 
            RegCloseKey(hKey);
	       if (!stricmp(mMIME, pMIME) || *p == '*')
		   {
			 if (*p != '*')
			 {
			   *pbHit = TRUE;
			 }
		     bsts = (bRoule ? TRUE : FALSE);
		     break;
		   }
		 }
	   */
	   }
	 }
	 fclose(fp);
   }
   return bsts;
}
#endif

#ifdef UPDATE_20080711 // 承認したことを、他の上長にも通知
void AddToBossAddrress(CHAR *pFrom, CHAR *pBoss, CHAR *pRCPFn)
{
  CHAR *p, *q;
  FILE *fp;

  q = pBoss;
  while((p = strchr(q, ',')))
  {
    *p = '\x0';
    if (stricmp(q, pFrom))  // 承認実施者でない承認者へCCする。
	{
	  if ((fp = fopen(pRCPFn, "at")))
	  {
#ifdef UPDATE_20090120 // 複数承認者のエンベロープ情報に改行抜け
	    fprintf(fp, "Recipient: %s\n", q); 
#else
	    fprintf(fp, "Recipient: %s", q); 
#endif
		fflush(fp);
	    fclose(fp);
	  }
	}
    *p = ',';
    q = p + 1;
  }
  if (stricmp(q, pFrom))  // 承認実施者でない承認者へCCする。
  {
    if ((fp = fopen(pRCPFn, "at")))
	{
#ifdef UPDATE_20090120 // 複数承認者のエンベロープ情報に改行抜け
      fprintf(fp, "Recipient: %s\n", q); 
#else
	  fprintf(fp, "Recipient: %s", q); 
#endif
	  fflush(fp);
	  fclose(fp);
	}
  }
}
#endif

#ifdef UPDATE_20150212 // 接続元のアドレスによって受信メールを破棄するオプション
BOOL DiscardMail(CHAR *pPeerAddr, CHAR *pFrom)
{
  BOOL bHit = FALSE;
  FILE *fp;  
  char mAddr[256];

  if (!bDISCARDMAIL) // フラグが有効でないなら無効
	return bHit;

  if ((fp = fopen(mDISCARDFile, "rt")))
  {
	 while(fgets(mAddr, sizeof(mAddr)-1, fp))
	 {
	   strtok(mAddr, "\r\n");
	   if (strchr(mAddr, '@'))
	   {
         if (!stricmp(mAddr, pFrom)) // エンベロープFROMでチェック
		 {
		   bHit = TRUE;
           break;
		 }
	   } else {
         if (!strcmp(mAddr, pPeerAddr)) // 接続元IPでチェック
		 {
		   bHit = TRUE;
           break;
		 }
	   }
	 }
	 fclose(fp);
  }
  return bHit;
}
#endif


#ifdef UPDATE_20160707 // 上長承認をUTF-8で処理する際、全角ハイフン「－」が内部処理で文字化けする
void UTF8_Rewrite_Value(CHAR *pSrc)
{ 
   FILE *fp;
   int  i, j;
   CHAR *pu, *ps, *pr;
   //CHAR cMask;
   CHAR mFn[256];
   CHAR mData[256], m1[8], m2[8], mEn[3];

    sprintf(mFn,"%srewritecode.dat", mProgPath);
	if ((fp = fopen(mFn, "rt")))
	{
	  while(fgets(mData, sizeof(mData)-1, fp))
	  {
		strtok(mData, "\r\n\' ");
		if (mData[0] != '\x0' &&
		    mData[0] != ' ' &&
		    mData[0] != '\'') 
		{  // コメントチェック
          if ((pr = strchr(mData, ',')))
		  {
			*pr = '\x0';
			pr++;
			ps = mData;
			mEn[2] = '\x0';
            /////////////////////////////////////////
			// 数値化
			j = 0;
	        for (i = 0; i < strlen(ps); i += 2) 
			{
              strncpy(mEn, (ps+i), 2);
              m1[j++] = (CHAR)tochar(mEn);
			}
			m1[j] = '\x0';
			// 数値化
			j = 0;
	        for (i = 0; i < strlen(pr); i += 2) 
			{
              strncpy(mEn, (pr+i), 2);
              m2[j++] = (CHAR)tochar(mEn);
			}
			m2[j] = '\x0';
            /////////////////////////////////////////
		    pu = pSrc;
		    while(*pu)
			{
			  if (_memicmp(pu, m1, strlen(m1)) == 0)
			  {
                memcpy(pu, m2, strlen(m2));
			    pu+=strlen(m2);
			  } else {
			    pu++;
			  }
			}
		  }
		}
	  }
	  fclose(fp);
	}
}
#endif

#ifdef UPDATE_20161112 // 送信先制限にエンベロープの送信元と日付ヘッダ中のトークンを指定可能にした。
void SetMailApprovalReversAddr(char *pAddr, char *pFn, BOOL bMode)
{
   FILE *fp;
   char mSrc[256];
   char mwork[512], itm[512];

   if (!CheckDomain(pAddr))
   {
     // 送信元エンベロープ
     sprintf(mSrc, "%s%s", mProgPath, (bMode ? mRecivePermitWhiteFile : mRecivePermitBlackFile));
     if ((fp = fopen(mSrc, "a+t")))
	 {
 	   fprintf(fp, "%s:%s\n", pAddr, (bMode ? "0" : "-1")); // 拒否設定
	   fflush(fp);
	   fclose(fp);
	 }
   }

   // Subject: ヘッダ
   sprintf(mSrc, "%s%s", mProgPath, (bMode ? mRecivePermitWhiteWordFile : mRecivePermitBlackWordFile));
   GetInfo(_T("subject:"), pFn, &mwork[1], sizeof(mwork)-1);
   if (mwork[1])
   {
	 mwork[0] ='*'; // 前方一致
#ifndef UPDATE_20161223 // セキュアハンドラ用の題名登録がリンク生成時コードの指定で影響されてしまう不具合
     CODEPAGE2UTF8N(CP_ISO_JAPANESE, mwork, itm, sizeof(itm));
     // 承認依頼メールのmailtoリンクの文字コード指定
     UTF8N2CODEPAGE(nMailApprovalCodepage, itm, mwork, sizeof(mwork)); // iso-2022-jpに変換
#endif
     if (mwork[0])
	 {
       if ((fp = fopen(mSrc, "a+t")))
	   {
 	     fprintf(fp, "%s\n", mwork); // 拒否設定
	     fflush(fp);
	     fclose(fp);
	   }
	 }
   }

}
#endif