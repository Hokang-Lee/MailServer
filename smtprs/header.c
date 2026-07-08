////////////////////////////////////////////////////////////
// header.c Copyright K.kawakami
////////////////////////////////////////////////////////////
#include "smtp.h"
#include "version.h"
#ifdef V3

extern char mProgPath[];
extern BOOL bDebug;
extern char  mdomain[];

#ifdef Y2038_BUG
void gettime(SYSTEMTIME *ltime, char *mt);
#else
void gettime(time_t *ltime, char *mt);
#endif
#ifdef UPDATE_20070509 // エイリアス設定の実アドレスオプションを別データとして処理する
BOOL CheckUser(char *user, char *pOpt, char *myaddr, BOOL *bLocal, BOOL *bSubLocal, BOOL *bMList, BOOL *bOutSideAliases, LPBYTE fullname);
#else
BOOL CheckUser(char *user, char *myaddr, BOOL *bLocal, BOOL *bSubLocal, BOOL *bMList, BOOL *bOutSideAliases, LPBYTE fullname);
#endif

BOOL Make_Received_Header(PSmtpContext pContext) {
  char     *p, *q, *r;
  char     mData[512];
  DWORD    nRecipient = 0, nRcv = 0, nSnd = 0, nSide = 0;
  BOOL     bSts = FALSE;
#ifdef UPDATE_20050216
  DWORD    nSide2 = 0;
  char     cWS;
#endif
#ifdef UPDATE_20050908 // １行内に入れ子を含んだ複数の条件指定があると無視されることがある
  BOOL     bOther;
#endif
#ifdef UPDATE_20080512 // 同報送信時に受領した最後のアドレスを"&RECIPIENT"で表示する
  char     mRcpt[512];
#endif
#ifdef UPDATE_20230712 // header.datにユニーク値生成変数を追加
  char     mUniq[256];
#endif
  //// 同報かシングルか確認
  if ((pContext->Rcptfp = fopen(pContext->mFnHead, "rt"))) {
	p = fgets(mData,sizeof(mData), pContext->Rcptfp);
    while(p || !feof(pContext->Rcptfp)) {
	  strtok(mData, "\r\n");
	  if (!strnicmp(mData, "Recipient: ", 11)) {
#ifdef UPDATE_20080512 // 同報送信時に受領した最後のアドレスを"&RECIPIENT"で表示する
        strcpy(mRcpt, &mData[11]);
#endif
		 nRecipient++;
#ifdef UPDATE_20070509 // エイリアス設定の実アドレスオプションを別データとして処理する
		 if (CheckUser( &mData[11],
			            NULL,
			            pContext->MyAddr,
						&pContext->bDomainVRFY,
						NULL,
						NULL,
					    NULL,
						pContext->fullname))
#else
		 if (CheckUser( &mData[11],
			            pContext->MyAddr,
						&pContext->bDomainVRFY,
						NULL,
						NULL,
					    NULL,
						pContext->fullname))
#endif
		   nRcv++;                    // 内部宛のアドレス数
	  }
	  p = fgets(mData,sizeof(mData), pContext->Rcptfp);
	}
	fclose(pContext->Rcptfp);
	nSnd = nRecipient - nRcv; // 外部宛のアドレス数
  }

#ifdef UPDATE_20230620 // 受信メールに任意のヘッダを追加するオプション
  pContext->mReplyTo[0] = pContext->mSender[0] = pContext->mReturnPath[0] = '\x0';
#endif
  sprintf(pContext->meffect,"%sheader.dat", mProgPath);
  pContext->Effefp = fopen(pContext->meffect,"rt");
  if (pContext->Effefp) {
	p = fgets(pContext->eaddr,sizeof(pContext->eaddr), pContext->Effefp);
    while(p || !feof(pContext->Effefp)) {
	  if ( ferror( pContext->Effefp ) ) {
        break;
      }
	  if (!(pContext->eaddr[0] == '\r' ||
		    pContext->eaddr[0] == '\n' ||
			pContext->eaddr[0] == '\'' )) {
			//(nRecipient == 1 &&  pContext->bRCPT && strnicmp(pContext->eaddr, "single_in=", 10) ||
			//(nRecipient == 1 && !pContext->bRCPT && strnicmp(pContext->eaddr, "single_out=", 11)) ||
			//(nRecipient == 1 && strnicmp(pContext->eaddr, "single=", 7)) ||
			//(nRecipient != 1 && strnicmp(pContext->eaddr, "more=", 5)))) { // コメント
         bSts = TRUE; // コメント以外の指定があれば設定優先
		 pContext->mdata[0] = '\x0';
 	     strtok(pContext->eaddr,"\r\n");
#ifdef UPDATE_20050216
		 if (pContext->eaddr[0] == '\t' || pContext->eaddr[0] == ' ') {
		   cWS = pContext->eaddr[0];
		 } else {
		   cWS = '\x0';
		 }
#endif
		 //if (nRecipient == 1)
		 //  q = pContext->eaddr + 7;
		 //else
		 //  q = pContext->eaddr + 5;
		 q = pContext->eaddr;
		 while (*q) {
/*
		   if (((nSide == 1 || nSide == 3) && nRecipient == 1 && !pContext->bRCPT) ||
			    (nSide == 2 && nRecipient == 1 && pContext->bRCPT)) {
			 if ((r = strchr(q, '}'))) { // 読み飛ばし
		       nSide = 0;
			   r++;
   			   q = r; // 次のトークン
			 }
			 continue;
		   } else if (((nSide == 1 || nSide == 3) && nRecipient == 1 && pContext->bRCPT) ||
			           (nSide == 2 && nRecipient == 1 && !pContext->bRCPT) ) {
			 if ((r = strpbrk(q, "}&"))) {
			   if (*r == '}') {
		         nSide = 0;
				 *r = '\x0';
				 r++;
                 strcat(pContext->mdata, q);
   			     q = r; // 次のトークン
			     continue;
			   }
			 }
		   }
*/
	       if ((nSide == 1 && nRcv) ||
               (nSide == 2 && nSnd) ||
   		       (nSide == 3 && nRecipient == 1)) {
			 if ((r = strpbrk(q, "}&"))) {
			   if (*r == '}') {
#ifdef UPDATE_20050216
				 nSide = nSide2; // 入れ子対策
				 nSide2 = 0; // 入れ子対策
#else
		         nSide = 0;
#endif
				 *r = '\x0';
				 r++;
                 strcat(pContext->mdata, q);
   			     q = r; // 次のトークン
			     continue;
			   }
			 }
		   } else if ((nSide == 1 && !nRcv) ||
                      (nSide == 2 && !nSnd) ||
   		              (nSide == 3 && !nRecipient == 1)) {
#ifdef UPDATE_20050908 // １行内に入れ子を含んだ複数の条件指定があると無視されることがある
			 bOther = FALSE;
#endif
#ifdef UPDATE_20050216
			 while((r = strchr(q, '{'))) { // 入れ子があるか？
			   if ((r = strchr(q, '}'))) { // 入れ子の終わりがあるか
				 q = r + 1; // 読み飛ばし
#ifdef UPDATE_20050908 // １行内に入れ子を含んだ複数の条件指定があると無視されることがある
				 if ((r = strpbrk(q, "&{")))
				   if (*r == '&') { // 入れ子始まりの前に条件指定があるか？
					 bOther = TRUE;
				     break;
				   }
#endif
			   } else {
			     if ((r = strchr(q, ';'))) { // 入れ子の終わりがあるか
				   q = r; // 読み飛ばし
				 } else {
			       break; 
				 }
			   }
			 }
#endif
#ifdef UPDATE_20050908 // １行内に入れ子を含んだ複数の条件指定があると無視されることがある
			 if (bOther) {// 入れ子始まりの前に条件指定があるか？
		       nSide = 0;
   			   q = r; // 次のトークン
			 } else 
#endif
			 if ((r = strchr(q, '}'))) { // 読み飛ばし
		       nSide = 0;
			   r++;
   			   q = r; // 次のトークン
#ifdef UPDATE_20050216
			 } else { // 次の行読み込み
	           q += strlen(q);
#endif
			 }
			 continue;
		   }

		   if ((r = strchr(q, '&'))) {
			 *r = '\x0';
             strcat(pContext->mdata, q);
			 r++;
			 if (!strnicmp(r, "RCV{", 4) ||
			     !strnicmp(r, "SND{", 4)) {
			   if (!strnicmp(r, "RCV{", 4))
				 nSide = 1;
               else if (!strnicmp(r, "SND{", 4)) 
				 nSide = 2;
   			   r += 4;
		       q = r; // 次のトークン
			   continue;
			 } else if (!strnicmp(r, "PER{", 4)) {
			   if (nRecipient == 1) {  // 単数
#ifdef UPDATE_20050216
				 nSide2 = nSide; // 入れ子対策
#endif
			     nSide = 3;
 		         r += 4;
			   } else {  // 複数
			     if ((r = strchr(r, '}')))  // 読み飛ばし
			       r++;
			   }
		       q = r; // 次のトークン
			   continue;
			 }
/*
			 if (!strnicmp(r, "RCV{", 4)) {
			   r += 4;
			   if (nRecipient == 1) {
			     nSide = 1;
			   } else { // 複数の場合は無視
				 if ((r = strchr(r, '}')))
				   r++;
			   }
		       q = r; // 次のトークン
			   continue;
			 } if (!strnicmp(r, "SND{", 4)) {
			   r += 4;
			   if (nRecipient == 1) {
			     nSide = 2;
			   } else { // 複数の場合は無視
				 if ((r = strchr(r, '}')))
				   r++;
			   }
			   q = r; // 次のトークン
			   continue;
			 }
*/
 			 if ( nSide == 0 ||
	             (nSide == 1 && nRcv) ||
                 (nSide == 2 && nSnd) ||
   		         (nSide == 3 && nRecipient == 1)) {
/*
				 ((nSide == 1 && pContext->bRCPT) ||
				  (nSide == 3 && nRecipient == 1 && pContext->bRCPT) ||
				  (nSide == 2 && nRecipient == 1 && !pContext->bRCPT) ) {
*/
			   if (!strnicmp(r, "SENDER", 6)) {
			     r += 6;
			     strcat(pContext->mdata, pContext->mUIDFROM);
			   } else if (!strnicmp(r, "RECIPIENT", 9)) { // ここでやっても意味がない。
			     r += 9;
			     if (nRecipient == 1)  // Singleのみ
				 {
#ifdef UPDATE_20080512 // 同報送信時に受領した最後のアドレスを"&RECIPIENT"で表示する
			       strcat(pContext->mdata, mRcpt);
#else
			       strcat(pContext->mdata, pContext->mUIDRCPT);
#endif
				 }
#ifdef UPDATE_20220603 // 接続時のTLSバージョンとChiperの表記を追加した。
			   } else if (!strnicmp(r, "SSLINFO", 7) || !strnicmp(r, "TLSINFO", 7)) {
			     r += 7;
				 if (pContext->mTLSInfo[0])
				 {
			       strcat(pContext->mdata, pContext->mTLSInfo);
				 } else {
				   char *t;
				   t = &pContext->mdata[strlen(pContext->mdata)]-1;
				   while(*t == ' ' || *t == '\t')
				   {
				      *t = '\x0';
					  t--;
				   }
				 }
#endif
#ifdef UPDATE_20220728 // RFC5831(821/2821) でエンベロープFROMの書式違反の判定フラグの追加
			   } else if (!strnicmp(r, "ENVEJUGE", 8)) {
			     r += 8;
				 if (pContext->bRFC3Dot3Sec)
			       strcat(pContext->mdata, "no");
				 else
			       strcat(pContext->mdata, "yes");
#endif
#ifdef UPDATE_20230712 // header.datにユニーク値生成変数を追加
			   } else if (!strnicmp(r, "UNIQID", 6)) {
			     r += 6;
				 gettime(&pContext->ltime, pContext->mtime);
				 sprintf(mData, "B%010lu%s%s", pContext->nMsgId, pContext->PeerAddr, pContext->mUIDFROM);
				 hmac_md5(mData, pContext->mtime, mUniq);
                 strcat(pContext->mdata, mUniq);
#endif
			   } else if (!strnicmp(r, "SOFT-NAME", 9)) {
			     r += 9;
			     strcat(pContext->mdata, ESMTP_NAME);
			   } else if (!strnicmp(r, "SOFT-VER", 8)) {
			     r += 8;
			     strcat(pContext->mdata, VERSION);
			   } else if (!strnicmp(r, "ORIGIN-IP", 9)) {
			     r += 9;
			     strcat(pContext->mdata, pContext->PeerAddr);
			   } else if (!strnicmp(r, "ORIGIN", 6)) {
			     r += 6;
			     strcat(pContext->mdata, pContext->PeerName);
			   } else if (!strnicmp(r, "DATETIME", 8)) {
			     r += 8;
                 gettime(&pContext->ltime, pContext->mtime); // 日付時間を確保
			     strcat(pContext->mdata, pContext->mtime); // 日付
			   } else if (!strnicmp(r, "MTA-NAME", 8)) {
			     r += 8;
			     strcat(pContext->mdata, pContext->LocalName);
			   } else if (!strnicmp(r, "MSG-ID", 6)) {
			     r += 6;
#ifdef UPDATE_20071204  // メッセージＩＤ採番処理を修正(Bym10id)
			     strcpy(mData, pContext->mMsgId);
#else
			     sprintf(mData, "B%010lu", pContext->nMsgId);
#endif
			     strcat(pContext->mdata, mData);
			   } else if (!strnicmp(r, "HELO", 4)) {
			     r += 4;
			     strcat(pContext->mdata, pContext->PeerHelo);
			   } else {
			     strcat(pContext->mdata, "&");
			   }
			 }
			 q = r; // 次のトークン
		   } else {
			 if ( nSide == 0 ||
	             (nSide == 1 && nRcv) ||
                 (nSide == 2 && nSnd) ||
   		         (nSide == 3 && nRecipient == 1))
/*
				 ((nSide == 1 || nSide == 3) && nRecipient == 1 && pContext->bRCPT) ||
				  (nSide == 2 && nRecipient == 1 && !pContext->bRCPT) )
*/
               strcat(pContext->mdata, q);
			 q += strlen(q);
		   }
		 }
#ifdef UPDATE_20050216
		 if (strlen(pContext->mdata) > 1 ||
			 (strlen(pContext->mdata) == 1 &&
			  pContext->mdata[0] != '\t' &&
			  pContext->mdata[0] != ' ') ) { // 空白のデータでないなら出力
		   if (cWS != '\x0' && cWS != pContext->mdata[0])
			 fputc(cWS, pContext->Datafp);
#endif
		   strcat(pContext->mdata, "\r\n");
#ifdef UPDATE_20230620 // 受信メールに任意のヘッダを追加するオプション
		   if (!strnicmp(pContext->mdata, "reply-to:", 9)) 
		   {
             strcpy(pContext->mReplyTo, pContext->mdata);
			 strtok(pContext->mReplyTo, "\r\n");
		   }
		   else if (!strnicmp(pContext->mdata, "sender:", 7))
		   {
             strcpy(pContext->mSender,pContext->mdata);
			 strtok(pContext->mSender, "\r\n");
		   }
		   else if (!strnicmp(pContext->mdata, "return-path:", 12))
		   {
             strcpy(pContext->mReturnPath,pContext->mdata);
			 strtok(pContext->mReturnPath, "\r\n");
		   } else 
#endif
		   {
	         fputs(pContext->mdata, pContext->Datafp);
		   }
#ifdef UPDATE_20050216
		 }
#endif
	  }
	  p = fgets(pContext->eaddr,sizeof(pContext->eaddr), pContext->Effefp);
	} 
	fclose(pContext->Effefp);
  }
  return bSts;
}
#endif