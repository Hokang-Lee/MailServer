#include "smtp.h"
#include <windows.h>       /* required for all Windows applications */
#include <winsock.h>
#include <stdio.h>         /* for sprintf                           */
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef  CS_RETURNMAIL // ReturnMail()中の割り込み処理禁止
extern CRITICAL_SECTION g_csReturnMail;
#endif
extern BOOL    bDebug;
extern BOOL    bServiceTerminating;
extern char    mPostmaster[];
extern char    mDefaultCode[];
extern char    mSendGateway[];
extern int     nSenderLog;
extern int     nSender2Log;
#ifdef UPDATE_20231104 // 自動応答生成時にメールヘッダの情報を利用するオプション機能を追加。
extern char    mMailReplyFrom[]; //[512];// 自動応答のエンベローブの送信先として利用するヘッダ
extern char    mMailReplyTo[]; //[512];// 自動応答のエンベローブの送信元として利用するヘッダ
#endif
#ifdef UPDATE_20240127 // ＭＬ制御応答＆送信エラー通知メールにもDKIMサインを追加可能にするオプション
// DKIM設定箇所の選択 bit0:(受信時付加)デフォルト bit1:上長承認メールへ付加 bit2:添付分離めーるへ付加
extern int    bDKIM;
extern CHAR   mDomainAUTHDKIM[];
#endif
#ifdef UPDATE_20240127C // DKIMサイン実施無効フラグを追加 1:無効(bit0:送信エラー bit1:ＭＬ制御応答 bit2:転送メール bit:3自動応答)
extern int    nOnDKIM;
extern CHAR   mDomainAUTHARC[];
#endif

#ifdef UPDATE_20070301 // 対象アドレスの比較 *ワイルドカード有効にする 2007.03.01
BOOL  wildcard_stricmp(const char *string1, const char *string2);
#endif
#ifdef Y2038_BUG
void gettime(SYSTEMTIME *ltime, char *mt);
#else
void gettime(time_t *ltime, char *mt);
#endif
void strSkipWSP(char *pstr);
void strword(char *pstr, char *c1, char *c2);
BOOL SendGlobalMailStatus(char *mDomain, char *mMsg, BOOL sts, BOOL btyp);
BOOL CheckLocalServer(char *mDomain);
int  SendGlobalMail(struct _RCP *rcp);
void outlog(struct _RCP *rcp);
void faillog(struct _RCP *rcp);
void SetRetry(struct _RCP *rcp, BOOL bCount, DWORD nErrCode, int nCC); //BOOL bINIT);
void ReturnMail(char *mNS, struct _RCP *rcp, DWORD no);
BOOL CheckDomain(char *mID);
#ifdef UPDATE_20241217_RCP // gateway.datにSMTP認証の指定が別のセッションで上書きされない対策
DWORD GetSMTPServer(char *mNS, char *mDomain, char *pFrom, char *pTo, char *mMsg, char *mSMTPServer, int *nPort, BOOL *bSSL, BOOL *bNotNS, BOOL *bNotMX, int *nGateAuthType);
#else
#ifdef UPDATE_20241128 // GATEWAY.DATにエンベロープの送信元の条件も追加可能に拡張した。
DWORD GetSMTPServer(char *mNS, char *mDomain, char *pFrom, char *pTo, char *mMsg, char *mSMTPServer, int *nPort, BOOL *bSSL, BOOL *bNotNS, BOOL *bNotMX);
#else
#ifdef UPDATE_20141023 // メールアドレス単位でもGatway.datでフォワード指定可能にする対策
DWORD GetSMTPServer(char *mNS, char *mDomain, char *pTo, char *mMsg, char *mSMTPServer, int *nPort, BOOL *bSSL, BOOL *bNotNS, BOOL *bNotMX);
#else
DWORD GetSMTPServer(char *mNS, char *mDomain, char *mMsg, char *mSMTPServer, int *nPort, BOOL *bSSL, BOOL *bNotNS, BOOL *bNotMX);
#endif
#endif
#endif
void printfTrace(char *str);

char * GetCtlWord(char *tkn) {
  BOOL bToken = FALSE;

   while(*tkn) {
     if (*tkn = '=')
 	   bToken = TRUE;
 	 tkn++;
	 if ( ( bToken && *tkn != ' ') ||
		  (!bToken && *tkn != ' ' && *tkn != '=') )
	   break;
   }
   return tkn;
}
void InitMailControl(struct _MAIL_CTL *mctl) {
   mctl->nMode = 0;
   mctl->Subject[0] = '\x0';
   mctl->ForwardSubject[0] = '\x0';
   mctl->ReplyFrom[0] = '\x0';
   mctl->ReplyTo[0] = '\x0';
   mctl->ForwardFrom[0] = '\x0';
   mctl->ForwardTo[0] = '\x0';
   mctl->RecordForward[0] = '\x0';
   mctl->bDiscard = TRUE;
   mctl->bRecordReplies = FALSE;
   mctl->bRecordDate = FALSE;
   mctl->bEchoSubject = FALSE;
   mctl->bEchoMessage = FALSE;
   mctl->bDeleteHeader = FALSE;
   mctl->bCopyReplies = FALSE;    // 自動応答メールアドレスをNOREPLY.TXTにもコピーする
   mctl->bDataDiscard = FALSE;    // 転送時に添付ファイルを破棄するか否か、0(no):破棄しない、1(yes):破棄する
   mctl->bDataSize    = FALSE;
   mctl->nDataSize = 0;           // 転送時の本文データサイズ制限 byte単位、bBodyDiscard=1の場合のみ有効
#ifdef AUTOPROCESS
   mctl->bAutoProcess = FALSE;    // 自動実行フラグ FALSE:実行しない TRUE:実行する
   mctl->Module[0] = '\x0';       // 自動実行モジュール
   mctl->CharSet[0]   = '\x0';    // 自動実行のキャラクタセット us-ascii,iso-2022-jp.... etc
#endif
}

BOOL GetMailControl(struct _MAIL_CTL *mctl, char *mCtlFn) {
   char *ptkn, tkn[256];
   char *pkey;
   FILE *fp;
   BOOL bsts = FALSE;

   InitMailControl(mctl);
   fp = fopen(mCtlFn, "rt");
   if (fp) {
	 ptkn = fgets(tkn, sizeof(tkn), fp);
	 while(ptkn || !feof(fp)) {
	   strtok(tkn, "\n");
  	   while(*ptkn == ' ' && *ptkn != '\x0')
	     ptkn++;
       if (strnicmp(ptkn, "[autoforward]", 13) == 0)
		 mctl->nMode = 1;
	   else if (strnicmp(ptkn, "[autoreply]", 11) == 0)
		 mctl->nMode = 2;
	   if (strnicmp(ptkn, "[autoprocess]", 13) == 0) { // 自動処理有効なら
		 mctl->bAutoProcess = TRUE;
	   }
	   pkey = strpbrk(ptkn, " =");
	   if (pkey) {
         if (strnicmp(ptkn, "subject", 7) == 0) {
           strcpy(mctl->Subject, GetCtlWord(pkey));
		   bsts = TRUE;
		 } else if (strnicmp(ptkn, "forwardsubject", 14) == 0) {
           strcpy(mctl->ForwardSubject, GetCtlWord(pkey));
		   bsts = TRUE;
		 } else if (strnicmp(ptkn, "replyfrom", 9) == 0) {
           strcpy(mctl->ReplyFrom, GetCtlWord(pkey));
		   bsts = TRUE;
		 } else if (strnicmp(ptkn, "replyto", 7) == 0) {
           strcpy(mctl->ReplyTo, GetCtlWord(pkey));
		   bsts = TRUE;
		 } else if (strnicmp(ptkn, "forwardfrom", 11) == 0) {
#ifdef UPDATE_20041105
		   if (!strcmp(pkey,"= "))   // 空白１文字＝エラーをメールを返信しない
              strcpy(mctl->ForwardFrom, pkey+1);
		   else
#endif
           strcpy(mctl->ForwardFrom, GetCtlWord(pkey));
		   bsts = TRUE;
		 } else if (strnicmp(ptkn, "forwardto", 9) == 0) {
           strcpy(mctl->ForwardTo, GetCtlWord(pkey));
		   bsts = TRUE;
		 } else if (strnicmp(ptkn, "recordforward", 13) == 0) {
           strcpy(mctl->RecordForward, GetCtlWord(pkey));
		   bsts = TRUE;
		 } else if (strnicmp(ptkn, "discard", 7) == 0) {
           if (stricmp(GetCtlWord(pkey), "yes") == 0)
             mctl->bDiscard = FALSE;
		   else
             mctl->bDiscard = TRUE;
		   bsts = TRUE;
		 } else if (strnicmp(ptkn, "recordreplies", 13) == 0) {
           if (stricmp(GetCtlWord(pkey), "yes") == 0)
             mctl->bRecordReplies = TRUE;
		   else
             mctl->bRecordReplies = FALSE;
		   bsts = TRUE;
		 } else if (strnicmp(ptkn, "recorddate", 10) == 0) {
           if (stricmp(GetCtlWord(pkey), "yes") == 0)
             mctl->bRecordDate = TRUE;
		   else
             mctl->bRecordDate = FALSE;
		   bsts = TRUE;
		 } else if (strnicmp(ptkn, "echosubject", 11) == 0) {
           if (stricmp(GetCtlWord(pkey), "yes") == 0)
             mctl->bEchoSubject = TRUE;
		   else
             mctl->bEchoSubject = FALSE;
		   bsts = TRUE;
		 } else if (strnicmp(ptkn, "echomessage", 11) == 0) {
           if (stricmp(GetCtlWord(pkey), "yes") == 0)
             mctl->bEchoMessage = TRUE;
		   else
             mctl->bEchoMessage = FALSE;
		   bsts = TRUE;
		 } else if (strnicmp(ptkn, "deleteheader", 12) == 0) {
           if (stricmp(GetCtlWord(pkey), "yes") == 0)
             mctl->bDeleteHeader = TRUE;
		   else
             mctl->bDeleteHeader = FALSE;
		   bsts = TRUE;
		 } else if (strnicmp(ptkn, "copyreplies", 11) == 0) {
           if (stricmp(GetCtlWord(pkey), "yes") == 0)
             mctl->bCopyReplies = TRUE;
		   else
             mctl->bCopyReplies = FALSE;
		   bsts = TRUE;
		 } else if (strnicmp(ptkn, "datadiscard", 11) == 0) {
           if (stricmp(GetCtlWord(pkey), "yes") == 0)
             mctl->bDataDiscard = TRUE;
		   else
             mctl->bDataDiscard = FALSE;
		   bsts = TRUE;
		 } else if (strnicmp(ptkn, "datasize", 8) == 0) {
		   mctl->bDataSize = TRUE;
           mctl->nDataSize = (DWORD)atol(GetCtlWord(pkey));
		   bsts = TRUE;
#ifdef AUTOPROCESS
		 } else if (strnicmp(ptkn, "module", 6) == 0) {
           strcpy(mctl->Module, GetCtlWord(pkey));
		   bsts = TRUE;
		 } else if (strnicmp(ptkn, "charset", 7) == 0) {
           strcpy(mctl->CharSet, GetCtlWord(pkey));
		   bsts = TRUE;
#endif
		 }
	   }
	   ptkn = fgets(tkn, sizeof(tkn), fp);
	 }
	 fclose(fp);
   }
   return (mctl->nMode == 0 ? (mctl->bAutoProcess ? TRUE : FALSE) : TRUE);
}

void RecordReplies(char *mRep, char *mRepFn, BOOL bRecordDate) {
  FILE *fp;
  char mt[128];
#ifdef Y2038_BUG
   SYSTEMTIME ltime;
#else
   time_t ltime;
#endif

  gettime(&ltime, mt);
  fp = fopen(mRepFn, "at");
  if (fp) {
	if (bRecordDate) // 年月日の記録
	  fprintf(fp, "%s %s\n", mRep, mt);
	else
	  fprintf(fp, "%s\n", mRep);
	fclose(fp);
  }
}

#ifdef AUTOPROCESS
BOOL AutoProcess(struct _MAIL_CTL *mc, char *mMBox, char *mSrc) {
  BOOL bsts = FALSE;
  HMODULE hDLL;
  FARPROC lpfnDll;    // Function pointer
  char *p, *pm, mModule[256], mOption[256]; //オプション

  if (mc->Module[0] == '\x0') // 翻訳モジュールがない場合は終了。
	return bsts;

  if (mc->Module[0] == '"') {
    strcpy(mModule, &mc->Module[1]);
	p = strstr(mModule, "\"");
	if (p) {
	  *p = '\x0';
	  p++;
	  if (*p == ' ')
		pm = p;
	  else
        pm = strstr(p, " ");
	} else
      pm = strstr(mModule, " ");
  } else {
    strcpy(mModule, mc->Module);
    pm = strstr(mModule, " ");
  }
  mOption[0] = '\x0';
  //pm = strstr(mModule, " ");
  if (pm) {
	*pm= '\x0';
    strcpy(mOption, pm+1);
  }
  hDLL = LoadLibrary(mModule); // モジュールのファイル名
  if (hDLL) {
    lpfnDll = GetProcAddress(hDLL, "MessageProc");
	if (lpfnDll)
      bsts = lpfnDll(mMBox, mSrc, mOption); // プロセス実行
    FreeLibrary(hDLL); // DLL モジュールの開放
	if (strnicmp(mOption, "CharSet:", 8) == 0)
	  strcpy(mc->CharSet, &mOption[8]); // キャラクタコードを受取る。
  }

  return bsts;
}
#endif

BOOL MailDataDiscard(struct _MAIL_CTL *mc, char *mSrc, char *mDest) {
  BOOL bsts = TRUE, battch = FALSE, bHead = TRUE, bBody = FALSE, bDisBody = FALSE;
  char *ptkn, *pbound, tkn[1024], mwk[1024], mbound[256];
  FILE *fpsrc, *fpdest;
  DWORD nwk, ntkn, nd = 0;
#ifdef UPDATE_20180124 // HTMLメールで添付削除時、メール本体とプレーンデータの"Content-Transfer-Encoding:"の形式がメーラでの表示が正常に行えない不具合
  BOOL bCte;
  char mCte[1024];
#endif
#ifdef UPDATE_20181122 // Content-Transfer-EncodingヘッダとContent-Typeヘッダの出現順位により正しく添付の削除が出来るようにする対策
  BOOL bCtype = FALSE;
  BOOL bbound = FALSE;
#endif

#ifdef UPDATE_20180124 // HTMLメールで添付削除時、メール本体とプレーンデータの"Content-Transfer-Encoding:"の形式がメーラでの表示が正常に行えない不具合
  bCte = FALSE;
  mCte[0] = '\x0';
#endif
  mbound[0] ='\x0';
#ifdef UPDATE_20181122 // Content-Transfer-EncodingヘッダとContent-Typeヘッダの出現順位により正しく添付の削除が出来るようにする対策
  mwk[0] = '\x0';
  if ((fpsrc = fopen(mSrc, "rb")))
  {
    while(fgets(tkn, sizeof(tkn), fpsrc)) {
 	  if (tkn[0] == '\r' || tkn[0] == '\n') {
	    break;
	  }
      strtok(tkn, "\r\n");
	  if (!strnicmp(tkn, "content-type:", 13) || bCtype)
	  {
		if (!bCtype) // ホワイトスペースなら
		{
	      strcpy(mwk, tkn);
		} else if (bCtype && (tkn[0] == '\t' || tkn[0] == ' ')) // ホワイトスペースなら
		{
	      strcat(mwk, tkn);
		} else {
		  _strlwr(mwk);
		  if (strstr(mwk, "boundary="))
		  {
			bbound = TRUE;
		  }
		  break;
		}
        bCtype = TRUE;
	  }
	}
	fclose(fpsrc);
  }
  mwk[0] = '\x0';
#endif
#ifdef UPDATE_20140809 //転送処理時のファイル読込みでバイナリデータが含まれていてもデータが切れてしまわない対策を追加。
  fpsrc = fopen(mSrc, "rb");
#else
  fpsrc = fopen(mSrc, "rt");
#endif
  if (fpsrc) {
    mbound[0]='\x0';
#ifdef UPDATE_20140809 //転送処理時のファイル読込みでバイナリデータが含まれていてもデータが切れてしまわない対策を追加。
    if ((fpdest = fopen(mDest, "wb"))) 
#else
    if ((fpdest = fopen(mDest, "wt"))) 
#endif
	{
	  ptkn = fgets(tkn, sizeof(tkn), fpsrc);
	  while(ptkn || !feof(fpsrc)) {
#ifdef UPDATE_20240128 // 転送時に添付除去指定した場合のARCヘッダを整合するようにする
        if (bDKIM && (nOnDKIM & 0x4) && // 転送メール
#ifdef UPDATE_20240130 // 添付削除した転送メールにDKIMサインを付けなおす
           (!strnicmp(tkn, "dkim-signature:", 15) ||
		    !strnicmp(tkn, "arc-authentication-results:", 27) ||
#else
		    (!strnicmp(tkn, "arc-authentication-results:", 27) ||
#endif
            !strnicmp(tkn, "arc-message-signature:", 22) ||
		    !strnicmp(tkn, "arc-seal:", 9) ||
	        !strnicmp(tkn, "authentication-results:", 23)))
	    {
          // ４ヘッダは除去
          // printf("%s\n", tkn);
	      while(fgets(tkn, sizeof(tkn), fpsrc)) // 読み飛ばし
		  {
		    if (!(tkn[0] == ' ' || tkn[0] == '\t'))
		    {
		      //fseek(fpsrc, (long)(0-strlen(tkn)), SEEK_CUR);
		      break;
		    }
		  }
	      if (!(tkn[0] == ' ' || tkn[0] == '\t'))
	        continue;
	   }
#endif
		strcpy(mwk, tkn);
		if (mwk[0] == '\r' || mwk[0] == '\n') {
          bHead = FALSE;
		  if (bBody) {
			bDisBody = TRUE;
		  }
		  if (mbound[0] == '\x0') {
            bBody = bDisBody = TRUE;
		  } else if (!bBody) {
			ptkn = fgets(tkn, sizeof(tkn), fpsrc);
			continue;
		  }
#ifdef UPDATE_20180124 // HTMLメールで添付削除時、メール本体とプレーンデータの"Content-Transfer-Encoding:"の形式がメーラでの表示が正常に行えない不具合
          else if (!bCte && mCte[0])
		  {
			fputs(mCte, fpdest);
		  }
#endif
		}
		if (mwk[0] != '\t' && mwk[0] != ' ') {
          battch = FALSE;
		}
		strtok(mwk, "\r\n");
		if (mbound[0]) {
		  if (!bBody && strcmp(mbound, &mwk[2]) == 0) {
	        ptkn = fgets(tkn, sizeof(tkn), fpsrc);
            bBody = TRUE;
			continue;
		  } else if (bBody && strcmp(mbound, &mwk[2]) == 0) {
			fprintf(fpdest, ".\r\n");
			break; 
		  }
		}
		_strlwr(mwk);
#ifdef UPDATE_20181126 // boundary内に':'が含まれると新しいヘッダとして誤認識してしまう不具合
		if (mwk[0] == ' ' || mwk[0] == '\t')
		{
          ptkn = mwk;
		} else 
#endif
		{
		  ptkn = strstr(mwk, ":");
		  if (ptkn) {
		    *ptkn = '\x0';
		    ptkn++;
		  } else if (battch) {
            ptkn = mwk;
		  }
		}
		if (stricmp(mwk, "content-type") == 0 || battch) {
		  if (strstr(ptkn, "multipart") || battch) {
#ifdef UPDATE_20110202 // 自動転送の添付削除でHTMLメールに添付があると正しく削除されない
		    if (bHead)
			{
#endif
            battch = TRUE;
            pbound = strstr(ptkn, "boundary=");
			if (pbound) {
			  strcpy(mwk, tkn);
			  strtok(mwk, "\r\n");
			  pbound += 9;
			  if (*pbound == '"')
			    pbound++;
		      strcpy(mbound, pbound);
			  strtok(mbound,"\"\r\n");
			  ptkn = fgets(tkn, sizeof(tkn), fpsrc);
			  continue;
			}
#ifdef UPDATE_20110202 // 自動転送の添付削除でHTMLメールに添付があると正しく削除されない
			}
#endif
		  }
		}
		if (!battch && bHead || bBody) {
		  if (bDisBody) {  // 本文バイト数カウント
			ntkn = strlen(tkn);
		    nd += ntkn;
		    if (mc->bDataDiscard && mc->bDataSize && nd > mc->nDataSize) {
			  nwk = nd - mc->nDataSize;
 			  strcpy(&tkn[ntkn-nwk], "\r\n.\r\n");
		      fputs(tkn, fpdest);
			  break;
			}
		  }
#ifdef UPDATE_20180124 // HTMLメールで添付削除時、メール本体とプレーンデータの"Content-Transfer-Encoding:"の形式がメーラでの表示が正常に行えない不具合
#ifdef UPDATE_20181120 // シングルパートのメールではcontent-transfer-encodingヘッダの除去を行わない対策
#ifdef UPDATE_20181122 // Content-Transfer-EncodingヘッダとContent-Typeヘッダの出現順位により正しく添付の削除が出来るようにする対策
		  if (bHead && bbound && !strnicmp(tkn, "content-transfer-encoding:", 26))
#else
		  if (bHead && mbound[0] && !strnicmp(tkn, "content-transfer-encoding:", 26))
#endif
#else
		  if (bHead && !strnicmp(tkn, "content-transfer-encoding:", 26))
#endif
		  {
			bCte = FALSE;
		    strcpy(mCte, tkn);
		  } else 
#endif
		  {
#ifdef UPDATE_20180124 // HTMLメールで添付削除時、メール本体とプレーンデータの"Content-Transfer-Encoding:"の形式がメーラでの表示が正常に行えない不具合
            if (!strnicmp(tkn, "content-transfer-encoding:", 26))
			{
			  bCte = TRUE;
			}
#endif
		    fputs(tkn, fpdest);
		  }
		}
	    ptkn = fgets(tkn, sizeof(tkn), fpsrc);
	  }
	  fflush(fpdest);
      fclose(fpdest);
	} else {
      bsts = FALSE;
	}
 	fclose(fpsrc);
  }
  return bsts;
}

void AutoTransfar(char *mNS, struct _RCP *rcp, struct _MAIL_CTL *mc, char *mfn, DWORD n) {
#ifdef TRACE_MODE
   char str[LOG_BUFFER];
#endif
  char mwork[256]; //, mNS[256];
  char mFrom[256], mTo[256];
#ifdef NEW_SENDER
  BOOL bOK;//, bAlias;
  FILE *fp[2];
  char *psts, mNo[12], mUserRcp[2][256], mUserMsg[256], mRCP[256];//, mrcpTo[256];
#endif
#ifdef UPDATE_20060508 // 転送エラーメールの送信MSGとRCPファイルが孤児化してしまう。
  DWORD no = 0;
  char  mBackRCP[256], mBackMSG[256];
#endif
  int  sts = 0;
  BOOL bNotNS = FALSE, bMySmtp = FALSE;
  BOOL bNotMX = FALSE;
  char *pdom, mMaster[256];
#ifndef UPDATE_20240202  // 添付削除した転送メールの転送先が複数あるとその度にサインしないようにする。UPDATE_20240130Aの訂正
#ifdef UPDATE_20240130A // 添付削除した転送メールの転送先が複数あるとその度にサインしないようにする。
  BOOL bSign;
#endif
#endif

   //////////////////////////
#ifdef NEW_SENDER
#ifdef TRACE_MODE
   if (nSenderLog) {
     sprintf(str, "[           ] start AutoTransfar(%s, %s)\n", rcp->mMsg, rcp->mRcp);
     printf("%s", str);
     printfTrace(str);
     sprintf(str, "[           ] mc->ForwardTo=%s\n", mc->ForwardTo);
     printf("%s", str);
     printfTrace(str);
   }
#endif
   strcpy(mUserMsg, rcp->mMsg);
   strcpy(mUserRcp[0], rcp->mMsg);
   strcpy(mUserRcp[1], rcp->mMsg);
   mUserMsg[strlen(mUserMsg)-4] = '\x0';
   mUserRcp[0][strlen(mUserRcp[0])-4] = '\x0';
   mUserRcp[1][strlen(mUserRcp[1])-4] = '\x0';
   sprintf(mNo, "-T%03lu.", n);
   strcat(mUserMsg, mNo);
   strcat(mUserRcp[0], mNo);
   strcat(mUserRcp[1], mNo);
   strcat(mUserMsg, "MSG");
   strcat(mUserRcp[0], "$CP");
   strcat(mUserRcp[1], "RCP");
#endif
#ifdef UPDATE_20061129 // 同報処理内での転送エラーメール発生時にエラーメールのファイル名が重複する不具合。
   strcpy(rcp->mSetRetryMode, mNo);
   strtok(rcp->mSetRetryMode, ".");
#endif

   //////////////////////////
   strcpy(mTo, rcp->mTo);
   strcpy(mFrom, rcp->mFrom);
   //////////////////////////
   // 回帰的送信が繰り返さないように送信元がPostmaster定義された
   // メールアドレス以外を自動転送対象とする
   //if (strnicmp(mFrom, mPostmaster, strlen(mPostmaster)) == 0) 
   // return;
   //////////////////////////
#ifdef UPDATE_20060616 // 管理アカウントがエイリアスの場合、実アドレス参照
   strcpy(mMaster, mPostmaster);
   pdom = strstr(mMaster, "@");
   if (pdom)
     *pdom = '\x0';
#ifdef UPDATE_20070509 // エイリアス設定の実アドレスオプションを別データとして処理する
   if (GetAliases((LPCTSTR)SOFT_ALIASES_REG, (char *)mMaster, (char *)pdom, NULL))
#else 
   if (GetAliases((LPCTSTR)SOFT_ALIASES_REG, (char *)mMaster, (char *)pdom))
#endif
   {
     printf("\t-> aliases = %s\n", mMaster);
   } else {
     if (pdom)
       *pdom = '@';
   }
#endif
   if (stricmp(mc->ForwardFrom, "*") ) { // 未置換
     strcpy(rcp->mFrom, mPostmaster);  // デフォルトの転送主はPostmaster@domainとする。
     if (mc->ForwardFrom[0] != '\x0') {// 送信元の変更する場合
	   if (stricmp(mc->ForwardFrom, " ") == 0) { // 送信元をブランクにする場合
         rcp->mFrom[0] = '\x0';
	   } else {
         strcpy(rcp->mFrom, mc->ForwardFrom);
	   }
	 }
   }
   //////////////////////////
   // 回帰的送信が繰り返さないように送信元がPostmaster定義された
   // メールアドレス以外を自動転送対象とする
#ifdef UPDATE_20060616 // 管理アカウントがエイリアスの場合、実アドレス参照
   if (stricmp(mFrom, mMaster) == 0 &&
	   stricmp(rcp->mFrom, mMaster) == 0) 
	 return;
#else
   if (stricmp(mFrom, mPostmaster) == 0 &&
	   stricmp(rcp->mFrom, mPostmaster) == 0) 
	 return;
#endif

#ifdef NEW_SENDER
   bOK = FALSE;
   if ((fp[0] = fopen(mUserRcp[0], "wt"))) {
#ifdef DATA_CASH
     setvbuf(fp[0], rcp->mFwbuf, _IOFBF, sizeof(rcp->mFwbuf) );
#endif
     //fprintf(fp[0],"Message-ID: <%s>\n",rcp->mMIDNo);
     //fprintf(fp[0], "Return-path: %s\n",rcp->mFrom);
	 if (strnicmp(mc->ForwardTo, "file:", 5)) { // メールアドレスなら
       //strcpy(mrcpTo, mc->ForwardTo);
       //pdom = strstr(mrcpTo, "@");
       //if (pdom)
       //  *pdom = '\x0';
       //bAlias = GetAliases((LPCTSTR)SOFT_ALIASES_REG, (char *)mrcpTo, (char *)pdom);
       //fprintf(fp[0], "Recipient: %s\n", (bAlias ? mrcpTo : mc->ForwardTo));
	   //fprintf(fp[0], "%s\n", (bAlias ? mrcpTo : mc->ForwardTo));
	   fprintf(fp[0], "%s\n", mc->ForwardTo);
	   bOK = TRUE;
	 } else { // 複数アドレスの指定（ファイルを指定）
	   if ((fp[1] = fopen(&mc->ForwardTo[5], "rt"))) {
#ifdef DATA_CASH
          setvbuf(fp[1], rcp->mFrbuf, _IOFBF, sizeof(rcp->mFrbuf) );
#endif
		  psts = fgets(mRCP, sizeof(mRCP), fp[1]);
		  while (psts) {
			strtok(mRCP, "\n");
            //strcpy(mrcpTo, mRCP);
            //pdom = strstr(mrcpTo, "@");
            //if (pdom)
            //  *pdom = '\x0';
            //bAlias = GetAliases((LPCTSTR)SOFT_ALIASES_REG, (char *)mrcpTo, (char *)pdom);
            //fprintf(fp[0], "Recipient: %s\n", (bAlias ? mrcpTo : mRCP));
#ifdef UPDATE_20080626 // 転送先アドレスの先頭に全角空白や半角空白が含まれている場合は、転送処理をスキップするようにした。
		    if (mRCP[0] != '\x0' &&
				mRCP[0] != '\n' &&
				mRCP[0] != '\r' &&
			    mRCP[0] != ' ' &&
			    strnicmp(mRCP, "　", 2))

#endif
			{
              fprintf(fp[0], "%s\n", mRCP);
			}
		    psts = fgets(mRCP, sizeof(mRCP), fp[1]);
		  }
		  fclose(fp[1]);
		  bOK = TRUE;
	   }
	 }
     fflush(fp[0]);
     fclose(fp[0]);
	 if (bOK) {
	   /*
       X_CopyFile(rcp->mMsg, mUserMsg, TRUE);
	   while(rename(mUserRcp[0], mUserRcp[1])) { // 転送先 RCPファイル
	   //while(!MoveFile(mUserRcp[0], mUserRcp[1])) { // 転送先 RCPファイル
         if (bServiceTerminating)
		   break;
	     printf("Faild - MoveFile(mUserRcp[0], mUserRcp[1])\n");
	     _sleep(WAIT_TIME);
	   }
	   */
	   /////////////////////  2003.06.17 ここから追加
	   // 直接転送
       strcpy(mwork, mNS);
       while(!(fp[0] = fopen(mUserRcp[0], "rt"))) {
         if (bServiceTerminating)
	 	   break;
	     _sleep(WAIT_TIME);
	   }
#ifndef UPDATE_20240202  // 添付削除した転送メールの転送先が複数あるとその度にサインしないようにする。UPDATE_20240130Aの訂正
#ifdef UPDATE_20240130A // 添付削除した転送メールの転送先が複数あるとその度にサインしないようにする。
       bSign = TRUE;
#endif
#endif
       if (fp[0]) {
#ifdef UPDATE_20240202  // 添付削除した転送メールの転送先が複数あるとその度にサインしないようにする。UPDATE_20240130Aの訂正
         if (bDKIM && (nOnDKIM & 0x4)) // 転送メール
           AddDKIMHeader(rcp->mMsg, rcp->mMID);
#endif
		 psts = fgets(mRCP, sizeof(mRCP), fp[0]);
	     while (psts || !feof(fp[0])) {
		   strtok(mRCP, "\n");
#ifdef TRACE_MODE
           if (nSenderLog) {
             sprintf(str, "[           ] Forward address =%s\n", mRCP);
             printf("%s", str);
             printfTrace(str);
		   }
#endif
           strcpy(rcp->mDomain, mRCP);  // 転送先
           strcpy(rcp->mTo, mRCP);
           strword(rcp->mDomain, "@", "\r\n");
#ifdef E_POST
           gethostname(rcp->mSmtp, 256);       // 一旦、自分のSMTPを通過させないといけない。
	       bMySmtp = TRUE;
#else
           if (mSendGateway[0] != '\x0') {  // 中継用ゲートウェイを指定
             strcpy(rcp->mSmtp, mSendGateway);
		   } else {
            if (!CheckDomain(rcp->mTo)) // 外部ドメインへの転送かチェック
              strcpy(rcp->mSmtp, rcp->mDomain); // 外部だったら対象SMTPへ接続
            else {                              // 内部だったら自SMTPへ接続
              gethostname(rcp->mSmtp, 256);     // 一旦、自分のSMTPを通過させないといけない。
	          bMySmtp = TRUE;
			}
		   }
#endif
           ////////////////////////////////////////////////////////////////
		   if (bDebug)
			 printf("AutoTransfar():GetSMTPServer(%s, %s)\n", mNS, rcp->mDomain);
#ifdef UPDATE_20241217_RCP // gateway.datにSMTP認証の指定が別のセッションで上書きされない対策
           if (!GetSMTPServer(mNS, rcp->mDomain, rcp->mFrom, rcp->mTo, rcp->mMsg, rcp->mSmtp, &rcp->nPort, &rcp->bUsedSSL, &bNotNS, &bNotMX, &rcp->nGateAuthType)) 
#else
#ifdef UPDATE_20241128 // GATEWAY.DATにエンベロープの送信元の条件も追加可能に拡張した。
           if (!GetSMTPServer(mNS, rcp->mDomain, rcp->mFrom, rcp->mTo, rcp->mMsg, rcp->mSmtp, &rcp->nPort, &rcp->bUsedSSL, &bNotNS, &bNotMX)) 
#else
#ifdef UPDATE_20141023 // メールアドレス単位でもGatway.datでフォワード指定可能にする対策
           if (!GetSMTPServer(mNS, rcp->mDomain, rcp->mTo, rcp->mMsg, rcp->mSmtp, &rcp->nPort, &rcp->bUsedSSL, &bNotNS, &bNotMX)) 
#else
           if (!GetSMTPServer(mNS, rcp->mDomain, rcp->mMsg, rcp->mSmtp, &rcp->nPort, &rcp->bUsedSSL, &bNotNS, &bNotMX)) 
#endif
#endif
#endif
		   {
             strcpy(rcp->mSmtp, rcp->mDomain);
#ifdef UPDATE_20191024 // 送信先がhosts(Aレコード参照)で解決されるとoutlogに記録されない不具合
             if (!CheckLocalServer(rcp->mDomain))
			 {
	           bMySmtp = FALSE;
			 }
#endif
		   } 
#ifdef UPDATE_20080704 // 転送指定があると、SMTP情報が書き換わってoutlocallogに記載されてしまう
		   else {
#ifdef UPDATE_20080708 // 外部ドメインの転送だけ記録
             if (!CheckLocalServer(rcp->mDomain))
#endif
			 {
	           bMySmtp = FALSE;
			 }
		   }
#endif
           ////////////////////////////////////////////////////////////////
           strcpy(mNS, mwork);
           printf("domain:[%s] smtp server:[%s]\n", rcp->mDomain, rcp->mSmtp);
           if (rcp->mSmtp[0] != '\x0') {
#ifndef UPDATE_20240202  // 添付削除した転送メールの転送先が複数あるとその度にサインしないようにする。UPDATE_20240130Aの訂正
#ifdef UPDATE_20240127A // 転送メールにARCヘッダを追加可能にした。
#ifdef UPDATE_20240127C // DKIMサイン実施無効フラグを追加 1:無効(bit0:送信エラー bit1:ＭＬ制御応答 bit2:転送メール bit:3自動応答)
#ifdef UPDATE_20240130A // 添付削除した転送メールの転送先が複数あるとその度にサインしないようにする。
             if (bSign && bDKIM && (nOnDKIM & 0x4)) // 転送メール
#else
             if (bDKIM && (nOnDKIM & 0x4)) // 転送メール
#endif
#else
             if (bDKIM) 
#endif
#ifdef UPDATE_20240130 // 添付削除した転送メールにDKIMサインを付けなおす
               AddDKIMHeader(rcp->mMsg, rcp->mMID);
#else
			   AddARCHeader(rcp, no);
#endif
#endif
#ifdef UPDATE_20240130A // 添付削除した転送メールの転送先が複数あるとその度にサインしないようにする。
             bSign = FALSE; // ２回目以降のサインは不要
#endif
#endif
             sts = SendGlobalMail(rcp);
	         if (sts == (int)TRUE) {
		       if (!bMySmtp)    // 自分のSMTP以外へ接続し送信完了した時ログを残す
	             outlog(rcp);
	           printf(" Messages %s send.\n", rcp->mDomain);
			 } else{
			   if (SendGlobalMailStatus(rcp->mDomain, rcp->mMsg,  sts, FALSE)) {
                 SetRetry(rcp, TRUE, (DWORD) sts, 1); //FALSE); // ドメインが応答なしならDomainフォルダとHoldフォルダに振分け保存
			   } else {
	             _unlink(rcp->mToOK); //DeleteFile(rcp->mToOK);           // 同一ドメイン送信失敗ならOKファイル削除
#ifndef UPDATE_20040525
                 strcpy(rcp->mTo, mTo);
                 strcpy(rcp->mFrom, mFrom);
#endif
#ifdef UPDATE_20060508 // 転送エラーメールの送信MSGとRCPファイルが孤児化してしまう。
                strcpy(mBackRCP, rcp->mRcp);
                strcpy(rcp->mRcp, rcp->mMsg);
                rcp->mRcp[strlen(rcp->mRcp)-4] = '\x0';
				strcat(rcp->mRcp, ".RCP");
#endif
#ifdef UPDATE_0x15639_HUNG_DEBUG
printf("[%s-%03d]:AutoTransfar(1) mNS=%x\n", rcp->mMNo, no, mNS);
if (mNS > 0x0a000000)
{
  if (nSenderLog || nSender2Log) 
  {
    sprintf(str, "[%s-%03d]:AutoTransfar(1) mNS=%x\n", rcp->mMNo, no, mNS);
    printfTrace(str);
  }
  exit(0);
}
if (no > 10000)
{
  exit(0);
}
#endif
#ifdef  CS_RETURNMAIL // ReturnMail()中の割り込み処理禁止
   EnterCriticalSection(&g_csReturnMail);
#endif
#ifdef UPDATE_20060508 // 転送エラーメールの送信MSGとRCPファイルが孤児化してしまう。
                ReturnMail(mNS, rcp, no++);               // 送信失敗ならメールを戻す。
                strcpy(rcp->mRcp, mBackRCP);
#else
                ReturnMail(mNS, rcp, 0);               // 送信失敗ならメールを戻す。
#endif
#ifdef  CS_RETURNMAIL // ReturnMail()中の割り込み処理禁止
  LeaveCriticalSection(&g_csReturnMail);
#endif
                 faillog(rcp);                          // 配送エラーログ
			   }
		       printf(" Messages %s not send.\n", rcp->mDomain);
			 }
		   } else {
             SetRetry(rcp, TRUE, (DWORD) sts, 1); //FALSE); // ドメインが応答なしならDomainフォルダとHoldフォルダに振分け保存
		   }
           ////////////////////
		   psts = fgets(mRCP, sizeof(mRCP), fp[0]);
		 }
		 fclose(fp[0]);
	   }
	   /////////////////////  2003.06.17 ここまで追加
	 }
	 _unlink(mUserRcp[0]); //DeleteFile(mUserRcp[0]);
#ifdef UPDATE_20240510 // メール転送時に転送先を一時的には保存したファイルが削除されずに残ってしまうことがある。
     while ((fp[0] = fopen(mUserRcp[0], "rt")))
	 {
       fclose(fp[0]);
	   _unlink(mUserRcp[0]); //DeleteFile(mUserRcp[0]);
       if (bServiceTerminating) 
         break;
       _sleep(WAIT_TIME);
	 }
#endif
   }
#else
   strcpy(rcp->mDomain, mc->ForwardTo);
   strcpy(rcp->mTo, mc->ForwardTo);
   strword(rcp->mDomain, "@", "\r\n");
   strcpy(mwork, mNS);
   ////////////////////////////////////////////////////////////////
#ifdef E_POST
     gethostname(rcp->mSmtp, 256);       // 一旦、自分のSMTPを通過させないといけない。
	 bMySmtp = TRUE;
#else
   if (mSendGateway[0] != '\x0') {  // 中継用ゲートウェイを指定
       strcpy(rcp->mSmtp, mSendGateway);
   } else {
     if (!CheckDomain(rcp->mTo)) // 外部ドメインへの転送かチェック
       strcpy(rcp->mSmtp, rcp->mDomain); // 外部だったら対象SMTPへ接続
     else {                              // 内部だったら自SMTPへ接続
       gethostname(rcp->mSmtp, 256);     // 一旦、自分のSMTPを通過させないといけない。
	   bMySmtp = TRUE;
	 }
   }
#endif
   ////////////////////////////////////////////////////////////////
   printf("GetSMTPServer(%s, %s)\n", mNS, rcp->mDomain);
#ifdef UPDATE_20241217_RCP // gateway.datにSMTP認証の指定が別のセッションで上書きされない対策
   if (!GetSMTPServer(mNS, rcp->mDomain, rcp->mFrom, rcp->mTo, rcp->mMsg, rcp->mSmtp, &rcp->nPort, &rcp->bUsedSSL, &bNotNS, &bNotMX, &rcp->nGateAuthType)) 
#else
#ifdef UPDATE_20241128 // GATEWAY.DATにエンベロープの送信元の条件も追加可能に拡張した。
   if (!GetSMTPServer(mNS, rcp->mDomain, rcp->mFrom, rcp->mTo, rcp->mMsg, rcp->mSmtp, &rcp->nPort, &rcp->bUsedSSL, &bNotNS, &bNotMX)) 
#else
#ifdef UPDATE_20141023 // メールアドレス単位でもGatway.datでフォワード指定可能にする対策
   if (!GetSMTPServer(mNS, rcp->mDomain, rcp->mTo, rcp->mMsg, rcp->mSmtp, &rcp->nPort, &rcp->bUsedSSL, &bNotNS, &bNotMX)) 
#else
   if (!GetSMTPServer(mNS, rcp->mDomain, rcp->mMsg, rcp->mSmtp, &rcp->nPort, &rcp->bUsedSSL, &bNotNS, &bNotMX)) 
#endif
#endif
#endif
   {
   //if (!GetSMTPServer(mNS, rcp->mDomain, rcp->mMsg, rcp->mSmtp, &bNotNS, &bNotMX)) {
     strcpy(rcp->mSmtp, rcp->mDomain);
   }
#ifdef UPDATE_20080704 // 転送指定があると、SMTP情報が書き換わってoutlocallogに記載されてしまう
   else {
#ifdef UPDATE_20080708 // 外部ドメインの転送だけ記録
     if (!CheckLocalServer(rcp->mDomain))
#endif
	 {
       bMySmtp = FALSE;
	 }
   }
#endif
   ////////////////////////////////////////////////////////////////
   strcpy(mNS, mwork);
   printf("domain:[%s] smtp server:[%s]\n", rcp->mDomain, rcp->mSmtp);
   if (rcp->mSmtp[0] != '\x0') {
	 {
       sts = SendGlobalMail(rcp);
	   if (sts == (int)TRUE) {
		 if (!bMySmtp)    // 自分のSMTP以外へ接続し送信完了した時ログを残す
	       outlog(rcp);
	     printf(" Messages %s send.\n", rcp->mDomain);
	   } else{
		/*
         //if (sts == (int)INVALID_SOCKET ||
			//sts == (int) FALSE)
		*/
		 if (SendGlobalMailStatus(rcp->mDomain, rcp->mMsg,  sts, FALSE)) {
           SetRetry(rcp, TRUE, (DWORD) sts, 1); //FALSE); // ドメインが応答なしならDomainフォルダとHoldフォルダに振分け保存
		 } else {
	       _unlink(rcp->mToOK); //DeleteFile(rcp->mToOK);           // 同一ドメイン送信失敗ならOKファイル削除
           strcpy(rcp->mTo, mTo);
           strcpy(rcp->mFrom, mFrom);
#ifdef UPDATE_0x15639_HUNG_DEBUG
printf("[%s-%03d]:AutoTransfar(2) mNS=%x\n", rcp.mMNo, no, mNS);
if (mNS > 0x0a000000)
{
  if (nSenderLog || nSender2Log) 
  {
    sprintf(str, "[%s-%03d]:AutoTransfar(2) mNS=%x\n", rcp.mMNo, no, mNS);
    printfTrace(str);
  }
  exit(0);
}
#endif
#ifdef  CS_RETURNMAIL // ReturnMail()中の割り込み処理禁止
   EnterCriticalSection(&g_csReturnMail);
#endif
           ReturnMail(mNS, rcp, 0);               // 送信失敗ならメールを戻す。
#ifdef  CS_RETURNMAIL // ReturnMail()中の割り込み処理禁止
  LeaveCriticalSection(&g_csReturnMail);
#endif
           faillog(rcp);                          // 配送エラーログ
		 }
		 printf(" Messages %s not send.\n", rcp->mDomain);
	   }
	 }
   } else {
     SetRetry(rcp, TRUE, (DWORD) sts, 1); //FALSE); // ドメインが応答なしならDomainフォルダとHoldフォルダに振分け保存
   }
#endif
}

BOOL NoReply(struct _RCP *rcp, char *mfn) {
   char mwork[256], *p;
   FILE *fp;
   BOOL sts = FALSE;
   fp = fopen(mfn, "rt");
   if (fp) {
	 mwork[0] = '\x0';
	 p = fgets(mwork, sizeof(mwork), fp);
	 while (p || !feof(fp)) {
	   strtok(mwork,"\r\n");
#ifdef UPDATE_20121213 // 対象アドレスの比較 *ワイルドカード有効にする 
       if (wildcard_stricmp(mwork, rcp->mFrom))  // 一致
#else
       if (stricmp(rcp->mFrom, mwork) == 0)
#endif
	   {
		 sts = TRUE;
	     break;
	   }
  	   mwork[0] = '\x0';
	   p = fgets(mwork, sizeof(mwork), fp);
	 }
     if (p && !sts) {
	   strtok(mwork,"\r\n");
       if (stricmp(rcp->mFrom, mwork) == 0)
		 sts = TRUE;
	 }
	 fclose(fp);
   }
   return sts;
}

BOOL YesReply(struct _RCP *rcp, char *mfn, char *mkey) {
   char mwork[256], minclude[256], mwkey[256], *p, *q, *pk;
   char mMSG[256],  mmkey[256], mbasekey[256];
   FILE *fp;
   BOOL sts = FALSE;  // ファイルが存在していなければ全てReplay可
   
   strcpy(mbasekey, "#key#");
   fp = fopen(mkey, "rt");
   if (fp) {          // 買物キーに変更があれば"KEY.txt"から取出し
	 fgets(mbasekey, sizeof(mbasekey), fp);
	 strtok(mbasekey,"\r\n");
	 fclose(fp);
   }
   /////////////////////////
   strcpy(mMSG, rcp->mMsg);
   mmkey[0] = '\x0';  // Premix用アカウント別買物キー付きかチェック
   fp = fopen(mMSG, "rt");
   if (fp) {
	 mwork[0] = '\x0';
	 p = fgets(mwork, sizeof(mwork), fp);
	 while (p || !feof(fp)) {
	   strtok(mwork,"\r\n");
	   //if (strnicmp(mwork, mbasekey, strlen(mbasekey)) == 0) {
	   pk = strstr(mwork, mbasekey);
	   if (pk) {
		  pk += strlen(mbasekey);
		  strncpy(mmkey, pk, 256);
		  break;
	   } 
  	   mwork[0] = '\x0';
	   p = fgets(mwork, sizeof(mwork), fp);
	 }
	 fclose(fp);
   }

   fp = fopen(mfn, "rt");
   if (fp) {
	 sts = TRUE;      // ファイルが存在していなければとりあえずReplay不可
	 mwork[0] = '\x0';
	 p = fgets(mwork, sizeof(mwork), fp);
	 while (p || !feof(fp)) {
	   mwkey[0] = '\x0';
	   q = strpbrk(mwork," \r\n");
	   if (q) {
		*q = '\x0';
		strcpy(mwkey, ++q);
	   }
       //q = strtok(mwork," \r\n");
       if ( (stricmp(rcp->mFrom, mwork) == 0 || stricmp("*", mwork) == 0 ) &&  // アカウント一致で
		    (strcmp(mmkey, mwkey) == 0 ||
		     mwkey[0] == '\x0' ||
		     mwkey[0] == '*' && mmkey[0] != '\x0') 
		   ) {                // 買物キーの一致のとき
		 sts = FALSE;
	     break;
	   } else if (strnicmp(mwork, "file:", 5) == 0) {
		 strcpy(minclude, &mwork[5]);
         fclose(fp);
         fp = fopen(minclude, "rt");
         if (!fp) {
		   sts = FALSE;
		   return sts;
		 }
	   }
  	   mwork[0] = '\x0';
	   p = fgets(mwork, sizeof(mwork), fp);
	 }
     if (p && !sts) {
	   strtok(mwork,"\r\n");
       if ( (stricmp(rcp->mFrom, mwork) == 0 || stricmp("*", mwork) == 0 ) &&  // アカウント一致で
		    (strcmp(mmkey, mwkey) == 0 ||
		     mwkey[0] == '\x0' ||
		     mwkey[0] == '*' && mmkey[0] != '\x0') 
		  )   // 買物キーの一致のとき
		 sts = FALSE;
	 }
	 fclose(fp);
   }
   return sts;
}

void AutoReply(char *mNS, struct _RCP *rcp, struct _MAIL_CTL *mc, char *mrep, BOOL bmode, DWORD n) {
#ifdef TRACE_MODE
   char str[LOG_BUFFER];
#endif
   char mwork[256], mMSG[256]; // mNS[256],
   char mFrom[256], mTo[256], mSb[10];
   char mt[128], *p; //mfn[256], 
#ifdef NEW_SENDER
  BOOL bOK, bAlias;
  FILE *fp[2];
  char *pdom, mNo[12], mUserRcp[2][256], mUserMsg[256], mrcpTo[256];
#endif
#ifdef Y2038_BUG
   SYSTEMTIME ltime;
#else
   time_t ltime;
#endif
   FILE *fpsrc, *fpdest;
   int  sts = 0;
   BOOL bMsg = FALSE;
   BOOL bSubj  = FALSE;
   BOOL bNotNS = FALSE, bMySmtp = FALSE;
   BOOL bNotMX = FALSE;
#ifdef ANNOUNCE
   char mcode[32];
#endif
   
   //////////////////////////
#ifdef NEW_SENDER
#ifdef TRACE_MODE
  if (nSenderLog) {
    sprintf(str, "[           ] start AutoReply(%s, %s)\n", rcp->mMsg, rcp->mRcp);
    printf("%s", str);
    printfTrace(str);
  }
#endif
   strcpy(mUserMsg, rcp->mMsg);
   strcpy(mUserRcp[0], rcp->mMsg);
   strcpy(mUserRcp[1], rcp->mMsg);
   mUserMsg[strlen(mUserMsg)-4] = '\x0';
   mUserRcp[0][strlen(mUserRcp[0])-4] = '\x0';
   mUserRcp[1][strlen(mUserRcp[1])-4] = '\x0';
   sprintf(mNo, "-Y%03lu.", n);
   strcat(mUserMsg, mNo);
   strcat(mUserRcp[0], mNo);
   strcat(mUserRcp[1], mNo);
   strcat(mUserMsg, "MSG");
   strcat(mUserRcp[0], "$CP");
   strcat(mUserRcp[1], "RCP");
#endif
#ifdef UPDATE_20061129 // 同報処理内での転送エラーメール発生時にエラーメールのファイル名が重複する不具合。
   strcpy(rcp->mSetRetryMode, mNo);
   strtok(rcp->mSetRetryMode, ".");
#endif
   //////////////////////////
   strcpy(mwork, rcp->mTo);
   strcpy(rcp->mTo, rcp->mFrom);
   strcpy(mTo, rcp->mTo);
   strcpy(mFrom, rcp->mFrom);
   if (!bmode)
     strcpy(rcp->mTo, mc->ReplyTo[0] == '\x0' ? rcp->mFrom : mc->ReplyTo);
   else {
	 if (mc->RecordForward[0] != '\x0')
       strcpy(rcp->mTo, mc->RecordForward);
	 else
	   return;
   }

#ifdef UPDATE_20050428
   if (strnicmp(rcp->mTo, "recipient:", 10))
	 strtok(rcp->mTo, "\r\n");
   else
#endif
     strword(rcp->mTo, " ", "\r\n");
   strword(rcp->mTo, "<", ">");
   if (!bmode)
     strcpy(rcp->mFrom, mwork);
   else {
     //  送信者アドレス通知であれば管理者メールとして送信。
	 if (strstr(mPostmaster,"@"))
       sprintf(rcp->mFrom, "%s", mPostmaster);
	 else
  	   sprintf(rcp->mFrom, "%s@%s", mPostmaster, rcp->mDomain); //rcp->mSmtp); // エラーメールとして返信
   }

   strcpy(mMSG, rcp->mMsg);
   ///////////////////////////// 2001.12.30
   //strtok(rcp->mMsg,".");
   p = strrchr(rcp->mMsg, '.');
   if (p)
     *p = '\x0';
   /////////////////////////////////////////
//   rcp->mMsg[strlen(rcp->mMsg)-4] = '\x0'; // 5.18
   strcat(rcp->mMsg,".tmp");
   gettime(&ltime, mt);
#ifndef NEW_SENDER
   if ((fpdest = fopen(rcp->mMsg, "wt")))
#else
   if ((fpdest = fopen(mUserMsg, "wt")))
#endif
   {
#ifdef DATA_CASH
     setvbuf(fpdest, rcp->mFwbuf, _IOFBF, sizeof(rcp->mFwbuf) );
#endif
	 if (!bmode) {
   	   if (!mc->bEchoSubject) // 受信タイトルエコーでないなら
         fprintf(fpdest, "Subject: %s\n", mc->Subject[0] == '\x0' ? "Automatic reply" : mc->Subject); 
       fprintf(fpdest, "From: %s\n",  mc->ReplyFrom[0] == '\x0' ? rcp->mFrom : mc->ReplyFrom);
       fprintf(fpdest, "To: %s\n", mc->ReplyTo[0] == '\x0' ? rcp->mTo : mc->ReplyTo); //rcp->mTo);
	   if (mc->ReplyFrom[0] != '\x0') {
		 p = strpbrk(mc->ReplyFrom, "<");
		 if (!p)
		   strcpy(rcp->mFrom, mc->ReplyFrom);
		 else {
		   strcpy(rcp->mFrom, p+1);
		   strtok(rcp->mFrom, ">");
		 }
	   }
	 } else {
       fprintf(fpdest, "Subject: %s %s\n", mc->ForwardSubject[0] == '\x0' ? "Automatic reply address" : mc->ForwardSubject, strstr(mrep, "FAIL.TXT") ? "(fail)":"(success)"); 
       fprintf(fpdest, "From: %s\n",  mFrom);
       fprintf(fpdest, "To: %s\n", rcp->mTo);
	 }

	 if (mc->CharSet[0] != '\x0')
	   fprintf(fpdest, "Content-Type: text/plain; charset=\"%s\"\n", mc->CharSet);
     else {
#ifdef ANNOUNCE
     strcpy(mcode, mDefaultCode); // 文字コードを設定
     strtok(mcode, " ");
	 fprintf(fpdest, "Content-Type: text/plain; charset=\"%s\"\n", mcode);
	 fprintf(fpdest, "Content-Transfer-Encoding: 7bit\n");
#endif
	 }
     fprintf(fpdest, "Date: %s\n", mt);
   	 if (!mc->bEchoSubject)  // 受信タイトルエコーでないなら
	   fprintf(fpdest, "\n");
	 //if (bmode)
	  //fprintf(fpdest, "\n> From: %s\n\n", mFrom);
	 //// AUTORPLAY.TXT 書込み
     fpsrc = fopen(mrep, "rt");
     if (fpsrc) {
	   mwork[0] = '\x0';
	   p = fgets(mwork, sizeof(mwork), fpsrc);
	   while (p || !feof(fpsrc)) {
	     if (p && (strcmp(mwork, ".\n") == 0 || strcmp(mwork, ".\r\n") == 0) )
			break;
           fputs(mwork, fpdest);
		 mwork[0] = '\x0';
	     p = fgets(mwork, sizeof(mwork), fpsrc);
	   }
       if (p)
		 if (!(strcmp(mwork, ".\n") == 0 || strcmp(mwork, ".\r\n") == 0))
           fputs(mwork, fpdest);
	   fclose(fpsrc);
	 }
	 if (mc->bEchoMessage) { // 受信メールも添付なら
	   if (!mc->bDeleteHeader)
	     fprintf(fpdest, "\n\nYour message reads:\n\n");
       while (!(fpsrc = fopen(mMSG, "rt"))) {
         if (bServiceTerminating)
		   break;
		 _sleep(WAIT_TIME);
	   }
       if (fpsrc) {
		 mwork[0] = '\x0';
	     p = fgets(mwork, sizeof(mwork), fpsrc);
	     while (p || !feof(fpsrc)) {
		   if (p && (strcmp(mwork, ".\n") == 0 || strcmp(mwork, ".\r\n") == 0) )
		 	 break;
   	       if (!bMsg && mc->bEchoSubject) { // 受信タイトルエコーなら
			 if (!bSubj) {
			   strncpy(mSb, mwork, 10);
			   mSb[8] = '\x0';
			   if (strnicmp(mSb, "subject:", 8) == 0)
			     bSubj = TRUE;
			 } else {
			   if (!(mSb[0] == '\x20' || mSb[0] == '\x08') ) {
			     bSubj = FALSE;
	             fprintf(fpdest, "\n");
			   }
			 }
			 if (bSubj)
               fputs(mwork, fpdest);  // 受信タイトル出力
		   }
		   if ( bMsg ||
			   !bMsg && !mc->bDeleteHeader)
             fputs(mwork, fpdest);
		   if (mwork[0] == '\n') // ヘッダの区切り
			 bMsg = TRUE;
		   mwork[0] = '\x0';
	       p = fgets(mwork, sizeof(mwork), fpsrc);
		 }
         if (p)
		   if (!(strcmp(mwork, ".\n") == 0 || strcmp(mwork, ".\r\n") == 0)) {
		     if ( bMsg ||
			     !bMsg && !mc->bDeleteHeader)
             fputs(mwork, fpdest);
		   }
	     fclose(fpsrc);
	   }
	 }
	 fputs("\n\n.\n", fpdest); // ターミネータ付加
     fflush(fpdest);
     fclose(fpdest);
   }
#ifdef UPDATE_20240127B // 自動応答メールにもDKIMサインを追加可能にするオプション
#ifdef UPDATE_20240127C // DKIMサイン実施無効フラグを追加 1:無効(bit0:送信エラー bit1:ＭＬ制御応答 bit2:転送メール bit3:自動応答)
   if (bDKIM && (nOnDKIM & 0x8)) // 自動応答
#else
   if (bDKIM)
#endif
     AddDKIMHeader(mUserMsg, rcp->mMID);
#endif

#ifdef NEW_SENDER
   bOK = FALSE;
   if ((fp[0] =  fopen(mUserRcp[0], "wt"))) {
     fprintf(fp[0],"Message-ID: <%s>\n",rcp->mMIDNo);
#ifdef UPDATE_20231104 // 自動応答生成時にメールヘッダの情報を利用するオプション機能を追加。
     GetEnvelope(mMailReplyFrom, mMSG, rcp->mFrom);
#endif
     fprintf(fp[0], "Return-path: %s\n",rcp->mFrom);
	 { // メールアドレスなら
#ifdef UPDATE_20231104 // 自動応答生成時にメールヘッダの情報を利用するオプション機能を追加。
       GetEnvelope(mMailReplyTo, mMSG, rcp->mTo);
#endif
       strcpy(mrcpTo, rcp->mTo);
       pdom = strstr(mrcpTo, "@");
       if (pdom)
         *pdom = '\x0';
#ifdef UPDATE_20070509 // エイリアス設定の実アドレスオプションを別データとして処理する
       bAlias = GetAliases((LPCTSTR)SOFT_ALIASES_REG, (char *)mrcpTo, (char *)pdom, NULL);
#else
       bAlias = GetAliases((LPCTSTR)SOFT_ALIASES_REG, (char *)mrcpTo, (char *)pdom);
#endif
       fprintf(fp[0], "Recipient: %s\n", (bAlias ? mrcpTo : rcp->mTo));
	   bOK = TRUE;
	 }
     fflush(fp[0]);
     fclose(fp[0]);
	 if (bOK) {
	   while(rename(mUserRcp[0], mUserRcp[1])) { // 転送先 RCPファイル
	   //while(!MoveFile(mUserRcp[0], mUserRcp[1])) { // 転送先 RCPファイル
         if (bServiceTerminating)
		   break;
	     printf("Faild - MoveFile(mUserRcp[0], mUserRcp[1])\n");
	     _sleep(WAIT_TIME);
	   }
	 } else {
	   _unlink(mUserRcp[0]); //DeleteFile(mUserRcp[0]);
	   _unlink(mUserMsg); //DeleteFile(mUserMsg);
	 }
   }

#else   
   // データ送信
   sprintf(mfn, "%s.MSG", rcp->mMNo);
   strcpy(rcp->mDomain, rcp->mTo);
   strword(rcp->mDomain, "@", "\r\n");
   strcpy(mwork, mNS);
   ////////////////////////////////////////////////////////////////
#ifdef E_POST
     gethostname(rcp->mSmtp, 256);       // 一旦、自分のSMTPを通過させないといけない。
     //bMySmtp = TRUE;
#else
   if (mSendGateway[0] != '\x0') {  // 中継用ゲートウェイを指定
       strcpy(rcp->mSmtp, mSendGateway);
   } else {
     if (!CheckDomain(rcp->mTo)) // 外部ドメインへの転送かチェック
       strcpy(rcp->mSmtp, rcp->mDomain); // 外部だったら対象SMTPへ接続
     else {                                // 内部だったら自SMTPへ接続
       gethostname(rcp->mSmtp, 256);       // 一旦、自分のSMTPを通過させないといけない。
       bMySmtp = TRUE;
	 }
   }
#endif
   ////////////////////////////////////////////////////////////////
   //strcpy(rcp->mSmtp, rcp->mDomain);
   //gethostname(rcp->mSmtp, 256); // 一旦、自分のSMTPを通過させないといけない。
   ////////////////////////////////////////////////////////////////
   printf("GetSMTPServer(%s, %s)\n", mNS, rcp->mDomain);
#ifdef UPDATE_20241217_RCP // gateway.datにSMTP認証の指定が別のセッションで上書きされない対策
   if (!GetSMTPServer(mNS, rcp->mDomain, rcp->mFrom, rcp->mTo, rcp->mMsg, rcp->mSmtp, &rcp->nPort, &rcp->bUsedSSL, &bNotNS, &bNotMX, &rcp->nGateAuthType))
#else
#ifdef UPDATE_20241128 // GATEWAY.DATにエンベロープの送信元の条件も追加可能に拡張した。
   if (!GetSMTPServer(mNS, rcp->mDomain, rcp->mFrom, rcp->mTo, rcp->mMsg, rcp->mSmtp, &rcp->nPort, &rcp->bUsedSSL, &bNotNS, &bNotMX)) 
#else
#ifdef UPDATE_20141023 // メールアドレス単位でもGatway.datでフォワード指定可能にする対策
   if (!GetSMTPServer(mNS, rcp->mDomain, rcp->mTo, rcp->mMsg, rcp->mSmtp, &rcp->nPort, &rcp->bUsedSSL, &bNotNS, &bNotMX)) 
#else
   if (!GetSMTPServer(mNS, rcp->mDomain, rcp->mMsg, rcp->mSmtp, &rcp->nPort, &rcp->bUsedSSL, &bNotNS, &bNotMX))
#endif
#endif
#endif
   {
   //if (!GetSMTPServer(mNS, rcp->mDomain, rcp->mMsg, rcp->mSmtp, &bNotNS, &bNotMX)) {
     strcpy(rcp->mSmtp, rcp->mDomain);
   }
   ////////////////////////////////////////////////////////////////
   strcpy(mNS, mwork);
   printf("domain:[%s] smtp server:[%s]\n", rcp->mDomain, rcp->mSmtp);

   if (rcp->mSmtp[0] != '\x0') {
	 {
       sts = SendGlobalMail(rcp);
	   if (sts == (int)TRUE) {
		 if (!bMySmtp)    // 自分のSMTP以外へ接続し送信完了した時ログを残す
	       outlog(rcp); 
	     printf(" Messages %s send.\n", rcp->mDomain);
	   } else{
		 /*
		 if (sts == (int)INVALID_SOCKET ||
			 sts == (int) FALSE)
		 */
		 if (SendGlobalMailStatus(rcp->mDomain, rcp->mMsg, sts, FALSE)) {
           SetRetry(rcp, TRUE, (DWORD) sts, 1); //FALSE); // ドメインが応答なしならDomainフォルダとHoldフォルダに振分け保存
		 } else {
	       _unlink(rcp->mToOK); //DeleteFile(rcp->mToOK);           // 同一ドメイン送信失敗ならOKファイル削除
           strcpy(rcp->mTo, mTo);
           strcpy(rcp->mFrom, mFrom);
#ifdef UPDATE_0x15639_HUNG_DEBUG
printf("[%s-%03d]:AutoReply(1) mNS=%x\n", rcp.mMNo, no, mNS);
if (mNS > 0x0a000000)
{
  if (nSenderLog || nSender2Log) 
  {
    sprintf(str, "[%s-%03d]:AutoReply(1) mNS=%x\n", rcp.mMNo, no, mNS);
    printfTrace(str);
  }
  exit(0);
}
#endif
#ifdef  CS_RETURNMAIL // ReturnMail()中の割り込み処理禁止
   EnterCriticalSection(&g_csReturnMail);
#endif
           ReturnMail(mNS, rcp, 0); // 送信失敗ならメールを戻す。
#ifdef  CS_RETURNMAIL // ReturnMail()中の割り込み処理禁止
  LeaveCriticalSection(&g_csReturnMail);
#endif
           faillog(rcp);               // 配送エラーログ
		 }
		 printf(" Messages %s not send.\n", rcp->mDomain);
	   }
	 }
   } else {
     SetRetry(rcp, TRUE, (DWORD) sts, 1); //FALSE); // ドメインが応答なしならDomainフォルダとHoldフォルダに振分け保存
   }

   _unlink(rcp->mMsg); //DeleteFile(rcp->mMsg);
#endif

   strcpy(rcp->mMsg, mMSG);
   strcpy(rcp->mFrom, mFrom); // 元に戻す。
}

#ifdef V4 
BOOL QueryPermitFrom(char *pFrom, char *pFn) {
#ifdef TRACE_MODE
   char str[LOG_BUFFER];
#endif
  BOOL sts = FALSE;
  FILE *fp;
  char *p, *q, mFrom[256];
  int  k, l;

#ifdef TRACE_MODE
   if (nSenderLog) {
     sprintf(str, "[           ] start QueryPermitFrom(%s, %s)\n", pFrom, pFn);
     printf("%s", str);
     printfTrace(str);
   }
#endif
   fp = fopen(pFn, "rt");
   if (fp) {
	 p = fgets(mFrom, sizeof(mFrom), fp);
	 while(p || !feof(fp)) {
	   strtok(mFrom, "\r\n");
#ifdef UPDATE_20070301 // 対象アドレスの比較 *ワイルドカード有効にする 2007.03.01
       if (wildcard_stricmp(mFrom, pFrom)) {  // 一致していたら転送ＯＫ
		 sts = TRUE;
		 break;
	   } 
#else
	   if (!stricmp(mFrom, pFrom)) {  // 一致していたら転送ＯＫ
		 sts = TRUE;
		 break;
	   }
#endif
	   p = fgets(mFrom, sizeof(mFrom), fp);
	 }
	 fclose(fp);
   } else {  
	 sts = TRUE;  // ファイルが無い時全てＯＫ
   }
#ifdef TRACE_MODE
   if (nSenderLog) {
	 sprintf(str, "[           ] start QueryPermitFrom() = %s\n", (sts ? "mach" : "unmach"));
     printf("%s", str);
     printfTrace(str);
   }
#endif
   return sts;
}
#endif