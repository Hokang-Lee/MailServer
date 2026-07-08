////////////////////////////////////////////////////////////
// effect.c Copyright K.kawakami
////////////////////////////////////////////////////////////
#include "smtp.h"

extern char mProgPath[];
extern char mLocalDomain[];
extern BOOL bDebug;
#ifdef UPDATE20240620 // effect.datでサブネットマスクの範囲指定の高速化と不具合修正
extern BOOL bEffectmaskengin; // ネットマスクの処理方式 デフォルト TRUE:新型 FALSE:旧型
#endif


BOOL Effective_Address(PSmtpContext pContext) {
  char *p, *pat, *pat2;
  PHOSTENT sin = NULL;
#ifdef IPv6
   struct in6_addr nip6, dip6, eip6;
   u_long nip, dip;
   //int  ReturnValue;
   BOOL    beIP;
#else
  u_long   nip, dip;
#endif
  int      i, k, l, m;
  BOOL     bIP, bOK;
  char     *peaddr, *peaddr2;
  char     eaddr[256];
  char     mIP[13] = "0123456789.*";
  int      mask, dot;
  DWORD    start, addr, num;
  char     *pmask;
#ifdef UPDATE_20060710 // ワイルドカードの処理強化
#ifdef UPDATE_20071205 //RFC2821: メールアドレス長が256Byteのときの対策
  CHAR     mFrom[512]; 
#else
  CHAR     mFrom[_MAX_PATH]; 
#endif
#ifdef UPDATE_20150319 // エンベロープの送信元によりメール受信の許可をする場合
  CHAR   mUser[1024], mMBoxDir[_MAX_PATH];
#endif

#ifdef UPDATE_20101015 // アカウント別のeffect.datが無いとき全体のonly指定が無視される不具合
  if (pContext->State < SmtpMailFrom) // 全体
  {
    sprintf(pContext->meffect,"%seffect.dat", mProgPath);
  } else { // 個別
    if (!strchr(pContext->p, '@')) // ドメインがない場合はトップのドメインを付加
      sprintf(mUser, "%s@%s", pContext->p, mLocalDomain);
    else
	  strcpy(mUser, pContext->p);
    GetMailBoxFolder(mUser, mMBoxDir);
    sprintf(pContext->meffect,"%seffect.dat", mMBoxDir);
    if ((pContext->Effefp = fopen(pContext->meffect,"rt")))
	{
      fclose(pContext->Effefp);
	} else { // ファイル無いときチェックはスキップ
      return TRUE;
	}
  }
#endif

    strncpy(mFrom, pContext->mUIDFROM, MAX_PATH);
#ifdef UPDATE_20071205 //RFC2821: メールアドレス長が255Byteのときの対策
    mFrom[511] = '\x0';
#else
    mFrom[MAX_PATH-1] = '\x0';
#endif
	strlwr(mFrom);
#endif

  pContext->sts   = FALSE;
  pContext->bmask = FALSE;
  pContext->bonly = FALSE;  // 指定したドメインのみ通過
  pContext->bebp  = TRUE;   // デフォルト対象ＩＰ通過
  pContext->bdefault = TRUE; // デフォルト条件で通過
#ifdef UPDATE_20060618 // 内部宛にしか受け付けないオプションをeffect.datに追加
  pContext->bnorelay = FALSE;  
  pContext->bnoauth = FALSE; // SMTP-AUTH優先
#endif
#ifdef IPv6
  if (strstr(pContext->mip,":"))
#ifdef VC6
    inet6_addr(pContext->mip, &nip6);
#else
    inet_pton(AF_INET6, pContext->mip, &nip6);
#endif
  else
    nip = inet_addr(pContext->mip);
#else
  nip = inet_addr(pContext->mip);
#endif
#ifdef UPDATE_20150319 // エンベロープの送信元によりメール受信の許可をする場合
#ifndef UPDATE_20101015 // アカウント別のeffect.datが無いとき全体のonly指定が無視される不具合
  if (pContext->State < SmtpMailFrom) // 全体
  {
    sprintf(pContext->meffect,"%seffect.dat", mProgPath);
  } else { // 個別
    if (!strchr(pContext->p, '@')) // ドメインがない場合はトップのドメインを付加
      sprintf(mUser, "%s@%s", pContext->p, mLocalDomain);
    else
	  strcpy(mUser, pContext->p);
    GetMailBoxFolder(mUser, mMBoxDir);
    sprintf(pContext->meffect,"%seffect.dat", mMBoxDir);
  }
#endif
#else
  sprintf(pContext->meffect,"%seffect.dat", mProgPath);
#endif
  pContext->Effefp = fopen(pContext->meffect,"rt");
  if (pContext->Effefp) {
	p = fgets(pContext->eaddr,sizeof(pContext->eaddr), pContext->Effefp);
    while(p || !feof(pContext->Effefp)) {
	  if ( ferror( pContext->Effefp ) ) {
        break;
      }
      pContext->bebp  = !pContext->bonly; //TRUE;   // デフォルト対象ＩＰ通過
	  if (!(pContext->eaddr[0] == '\r' ||
		    pContext->eaddr[0] == '\n' ||
		    pContext->eaddr[0] == ' '  ||
		    pContext->eaddr[0] == '\'' )) {
         pContext->bdefault = TRUE; // デフォルト条件で通過
#ifdef UPDATE_20060618 // 内部宛にしか受け付けないオプションをeffect.datに追加
         pContext->bnorelay = FALSE;  
         pContext->bnoauth = FALSE; // SMTP-AUTH優先
#endif
// 20240620 effect.datでサブネットマスクの範囲指定の高速化と不具合修正
//strcpy(pContext->PeerAddr,"10.8.1.9"); // TEST
//strcpy(pContext->PeerAddr,"61.121.80.166"); // TEST
//strcpy(pContext->PeerAddr,"61.32.80.166"); // TEST
//strcpy(pContext->PeerAddr,"52.193.156.99"); // TEST
         strcpy(pContext->mip, pContext->PeerAddr); //2000.12.27
 	     strtok(pContext->eaddr,"\r\n");
		 ////// IP アドレスの指定か否かの確認 // 2000.02.06
		 strcpy(eaddr, pContext->eaddr);
		 peaddr = eaddr;
		 strtok(peaddr, " ");
		 while(*peaddr) {
		   if (*peaddr == '*')
		     *peaddr = '0';
		   else if (*peaddr == '/') // 2002.07.12 '/'が使えない
		     *peaddr = '\x0';
		   peaddr++;
		 }
#ifdef IPv6
		 if (strstr(eaddr,":"))  // IPv6での指定なら
#ifdef VC6
           bIP = inet6_addr(eaddr, &dip6);
#else
#ifdef UPDATE_20211209 // IPv6判定に失敗する不具合
		   bIP = (inet_pton(AF_INET6, eaddr, &dip6) == 0 ? FALSE : TRUE);
#else
		   bIP = (inet_pton(AF_INET6, eaddr, &dip6) == 0 ? TRUE : FALSE);
#endif
#endif
		 else {
           dip = inet_addr(eaddr);
		   bIP = ((long)dip != (long)-1 ? TRUE : FALSE);
		 }
#else
         dip = inet_addr(eaddr);
		 bIP = ((long)dip != (long)-1 ? TRUE : FALSE);
#endif
		 ///////////////////////////////////////////////////
         pContext->ebp = strstr(pContext->eaddr," ");
		 if (pContext->ebp) {
		   *pContext->ebp = '\x0';
		   pContext->ebp++;
 	       strtok(pContext->ebp," ");
#ifdef UPDATE_20060618 // 内部宛にしか受け付けないオプションをeffect.datに追加
           if (!_stricmp(pContext->ebp, "false") ||
			   !_stricmp(pContext->ebp, "false,0") ||
               !_stricmp(pContext->ebp, "false,1") )
#else
           if (!_stricmp(pContext->ebp, "false"))
#endif
		   {
#ifdef UPDATE_20060618 // 内部宛にしか受け付けないオプションをeffect.datに追加
			 if (!_stricmp(pContext->ebp, "false,0")) // SMTP-AUTHを優先しない
				 pContext->bnoauth = TRUE;
			 else
				 pContext->bnoauth = FALSE;
#endif
			 pContext->bonly = FALSE;  // v1.29 にて修正
			 pContext->bebp = FALSE;
			 pContext->bdefault = FALSE;
#ifdef UPDATE_20060618 // 内部宛にしか受け付けないオプションをeffect.datに追加
             pContext->bnorelay = FALSE;  
#endif
		   }
#ifdef UPDATE_20060618 // 内部宛にしか受け付けないオプションをeffect.datに追加
           else if (!_stricmp(pContext->ebp, "only") ||
			          !_stricmp(pContext->ebp, "only,0") ||
                      !_stricmp(pContext->ebp, "only,1") )
#else
		   else if (!_stricmp(pContext->ebp, "only"))
#endif
		 {
#ifdef UPDATE_20060618 // 内部宛にしか受け付けないオプションをeffect.datに追加
			 if (!_stricmp(pContext->ebp, "only,0")) // SMTP-AUTHを優先しない
				 pContext->bnoauth = TRUE;
			 else
				 pContext->bnoauth = FALSE;
#endif
			 pContext->bonly = TRUE;
			 pContext->bebp =  !pContext->bonly;
			 pContext->bdefault = FALSE;
#ifdef UPDATE_20060618 // 内部宛にしか受け付けないオプションをeffect.datに追加
             pContext->bnorelay = FALSE;  
#endif
		   }
#ifdef UPDATE_20060618 // 内部宛にしか受け付けないオプションをeffect.datに追加
           else if (!_stricmp(pContext->ebp, "true") ||
			          !_stricmp(pContext->ebp, "true,0") ||
                      !_stricmp(pContext->ebp, "true,1") )
#else
		   else if (!_stricmp(pContext->ebp, "true")) // type 空白は true
#endif
	  {
#ifdef UPDATE_20060618 // 内部宛にしか受け付けないオプションをeffect.datに追加
			 if (!_stricmp(pContext->ebp, "true,0")) // SMTP-AUTHを優先しない
				 pContext->bnoauth = TRUE;
			 else
				 pContext->bnoauth = FALSE;
#endif
			 pContext->bonly = FALSE;  // 2000.12.28
			 pContext->bebp = TRUE;
			 pContext->bdefault = FALSE;
#ifdef UPDATE_20060618 // 内部宛にしか受け付けないオプションをeffect.datに追加
             pContext->bnorelay = FALSE;  
#endif
#ifdef EFFECT_DAT_PLUS1    // 2004.04.28 追加
		   }
#ifdef UPDATE_20060618 // 内部宛にしか受け付けないオプションをeffect.datに追加
           else if (!_stricmp(pContext->ebp, "default") ||
			          !_stricmp(pContext->ebp, "default,0") ||
                      !_stricmp(pContext->ebp, "default,1") )
#else
		   else if (!_stricmp(pContext->ebp, "default")) // 2004.04.28 type default 追加
#endif
		   {
#ifdef UPDATE_20060618 // 内部宛にしか受け付けないオプションをeffect.datに追加
			 if (!_stricmp(pContext->ebp, "default,0")) // SMTP-AUTHを優先しない
				 pContext->bnoauth = TRUE;
			 else
				 pContext->bnoauth = FALSE;
#endif
			 pContext->bdefault = TRUE; 
#ifdef UPDATE_20060618 // 内部宛にしか受け付けないオプションをeffect.datに追加
             pContext->bnorelay = FALSE;  
#endif
		   }
#ifdef UPDATE_20060618 // 内部宛にしか受け付けないオプションをeffect.datに追加
#ifdef UPDATE_20071217 // SMTP認証無効でnorelayが可能なオプション追加
           else if (!_stricmp(pContext->ebp, "norelay") ||
			          !_stricmp(pContext->ebp, "norelay,0") ||
                      !_stricmp(pContext->ebp, "norelay,1") ||
					  !_stricmp(pContext->ebp, "norelay,2"))
#else
           else if (!_stricmp(pContext->ebp, "norelay") ||
			          !_stricmp(pContext->ebp, "norelay,0") ||
                      !_stricmp(pContext->ebp, "norelay,1") )
#endif
		   {
			 if (!_stricmp(pContext->ebp, "norelay,0")) // SMTP-AUTHを優先しない
			   pContext->bnoauth = TRUE;
			 else
			   pContext->bnoauth = FALSE;
			 pContext->bdefault = TRUE; 
			 pContext->bnorelay = TRUE; 
#ifdef UPDATE_20071217 // SMTP認証無効でnorelayが可能なオプション追加
			 if (_stricmp(pContext->ebp, "norelay,0")) {
			   pContext->bnorelay = !pContext->bAUTHSUCCESS; // SMTP認証の有無で処理が変わる
			 }
			 if (!_stricmp(pContext->ebp, "norelay,2")) {
			   pContext->bonly = FALSE;
			   pContext->bebp = TRUE;
			   pContext->bdefault = FALSE;
			   pContext->bnorelay = !pContext->bAUTHSUCCESS; // SMTP認証の有無で処理が変わる
			   //pContext->bnoauth = !pContext->bAUTHSUCCESS; // SMTP認証の有無で処理が変わる
			   //pContext->bdefault = pContext->bAUTHSUCCESS; // SMTP認証の有無で処理が変わる
			 }
#endif
		   }
#endif
#endif
		 } else { // type 空白は true
		   pContext->bonly = FALSE;  // 2000.12.28
		   pContext->bebp = TRUE;
		   pContext->bdefault = FALSE;
#ifdef UPDATE_20060618 // 内部宛にしか受け付けないオプションをeffect.datに追加
           pContext->bnorelay = FALSE;  
		   pContext->bnoauth = FALSE;
#endif
		 }
         if (bDebug) printf("Effective <%s> vs <%s:%s> type %s\n", pContext->eaddr, pContext->PeerAddr, pContext->mUIDFROM, (pContext->bonly?"only":(pContext->bebp ? (pContext->bdefault ? "default" : "true") :"false")));
#ifdef IPv6
		 if (strstr(pContext->eaddr,":"))  // IPv6での指定なら
		 {
#ifdef VC6
		   beIP = inet6_addr(pContext->eaddr, &eip6);
#else
#ifdef UPDATE_20211209 // IPv6判定に失敗する不具合
		   if (pContext->mp = strstr(pContext->eaddr,"/"))
		     *pContext->mp='\x0';
		   beIP = (inet_pton(AF_INET6, pContext->eaddr, &eip6) == 0 ? FALSE : TRUE);
		   if (pContext->mp)
			 *pContext->mp='/';
#else
		   beIP = (inet_pton(AF_INET6, pContext->eaddr, &eip6) == 0 ? TRUE : FALSE);
#endif
#endif
		 } else {
		   beIP = (inet_addr(pContext->eaddr) != INADDR_NONE ? TRUE : FALSE);
		 }
		 if (strnicmp(pContext->eaddr,"#blank",6) != 0 &&  // 2001.01.11 FROM:の空白対策
			 strnicmp(pContext->eaddr,"#<>",3) != 0 && // 2001.08.30 FROM:の空白対策'<>'の場合
#ifdef EFFECT_DAT_PLUS1    // 2004.04.28 追加
			 strnicmp(pContext->eaddr,"#number", 9) &&   // ＠以降が数字でIPでドメイン指定されている場合
			 strnicmp(pContext->eaddr,"#nodomain", 7) && // ＠以降がドメイン名で指定されている場合
			 strnicmp(pContext->eaddr,"#host", 5) &&     // ＠以降がホスト名で指定されている場合
#endif
			 //!strstr(pContext->eaddr, "@") && 
			  bIP &&
			  (strstr(pContext->eaddr, "/") || 
			   //strstr(pContext->eaddr, ".") || 
			   strstr(pContext->eaddr, "*") ||
			   (!(strstr(pContext->eaddr, "*") || strstr(pContext->eaddr, "/")) && beIP ) ) )
#else
		 if (strnicmp(pContext->eaddr,"#blank",6) != 0 &&  // 2001.01.11 FROM:の空白対策
			 strnicmp(pContext->eaddr,"#<>",3) != 0 &&
#ifdef EFFECT_DAT_PLUS1    // 2004.04.28 追加
			 strnicmp(pContext->eaddr,"#number", 9) &&   // ＠以降が数字でIPでドメイン指定されている場合
			 strnicmp(pContext->eaddr,"#nodomain", 7) && // ＠以降がドメイン名で指定されている場合
			 strnicmp(pContext->eaddr,"#host", 5) &&     // ＠以降がホスト名で指定されている場合
#endif
			 //!strstr(pContext->eaddr, "@") && 
			  bIP &&
			  (strstr(pContext->eaddr, "/") || 
			   //strstr(pContext->eaddr, ".") || 
			   strstr(pContext->eaddr, "*") ||
			   (!(strstr(pContext->eaddr, "*") || strstr(pContext->eaddr, "/")) && inet_addr(pContext->eaddr) != INADDR_NONE ) ) )
#endif
		 {
           pContext->ep = strstr(pContext->eaddr,"*");
		   if (pContext->ep) {
		     pContext->mp = NULL;
		     strtok(pContext->eaddr,"*");
		   } else
             pContext->mp = strstr(pContext->eaddr,"/");
  		   if (pContext->mp) {
		     //// ネットマスク 2003.03.27 修正
		     *pContext->mp= '\x0';
#ifdef UPDATE_20211210_IPV6PREFIX // IPv6アドレスプレフィックスに対応した。
             if (strchr(pContext->eaddr,':'))
               pContext->bmask = Compear_IPv6_Addr(pContext->PeerAddr, pContext->eaddr, pContext->mp);
			 else
#endif
             {
			   /////////////////////////////////////////
			   // 2004.01.31 修正 マイナスにならないように
		       //mask = 32 - atoi(++pContext->mp);
			   if ((mask = 32 - atoi(++pContext->mp)) < 0)
 			     mask = 0;
			   /////////////////////////////////////////
		       num = 1L << mask; // 2のべき乗範囲
			   dot = ((mask-1) / 8) + 1;
#ifdef UPDATE20240621 // effect.datでサブネットマスクの旧式範囲指定の不具合修正
			   if (dot == 4)
			     addr = num / 16777216;
			   else if (dot == 3)
			     addr = num / 65536;
			   else if (dot == 2)
			     addr = num / 256;
			   else
 			     addr = num;
#else
			   if (dot > 1)
			     addr = num / (256*(dot-1));
			   else
 			     addr = num;
#endif
			   if (dot < 4) {
			     for (pContext->i = 1; pContext->i <= (unsigned int)dot; pContext->i++) {
			       if ((pContext->mp = strrchr(pContext->eaddr, '.')))
				     *pContext->mp = '\x0';
			     }
			   } else{
			     pContext->mp = pContext->eaddr;
			   }
#ifdef UPDATE_20041204
			   if (pContext->mp)
#endif
			   {
#ifdef UPDATE20240620 // effect.datでサブネットマスクの範囲指定の高速化と不具合修正
                 if (bEffectmaskengin) // 新型
                 {
                   unsigned long nip;
                   unsigned long sip;
                   unsigned long eip;

				   int  nmask = (-1L << (32-mask))-1;

                   sip = inet_addr(eaddr); //inet_pton(AF_INET, eaddr, &sip);
                   sip = htonl(sip);
				   sip = (sip & (num*-1L));
				   eip = (sip + num);
				   nip = inet_addr(pContext->mip); //inet_pton(AF_INET, pContext->mip, &nip);
                   nip = htonl(nip);
                   if (nip >= sip && nip < eip)
                   {
                     pContext->bmask = TRUE;  // 一致するものがあった
#ifndef UPDATE_20241217 // effect.datの指定でネットマスクの判定が無効になっていた不具合
                     break;
#endif
                   }
                 } else {
#endif
			       start = atoi(pContext->mp + ((dot < 4) ? 1 : 0));
	               for (pContext->i = 0; pContext->i < (unsigned int)addr; pContext->i++) {
			         *pContext->mp = '\x0';
			         pmask = &pContext->eaddr[strlen(pContext->eaddr)];
			         if (start + pContext->i > 255)  // 256は超えないようにする
				       break;
			         sprintf(pmask, "%s%d", ((dot < 4) ? "." : ""), start + pContext->i);
                     if (!strncmp(pContext->mip, pContext->eaddr, strlen(pContext->eaddr)) &&
				         (pContext->mip[strlen(pContext->eaddr)] == '.' && dot > 1 ||
				          pContext->mip[strlen(pContext->eaddr)] == '\x0' && dot == 1)) {
		               pContext->bmask = TRUE;  // 一致するものがあった
			           break;
				     }
			      }
#ifdef UPDATE20240620 // effect.datでサブネットマスクの範囲指定の高速化と不具合修正
                }
#endif
			   }
			 }
		   }
	       if (pContext->eaddr[0] == '*' ||
		 	  (pContext->mp && pContext->bmask) ||
#ifdef UPDATE_20131024 // EFFECT.DATへの固定IP指定時に末尾の数値以降がワイルドカード扱いされている不具合。
  		      (pContext->ep && !pContext->mp && strncmp(pContext->mip, pContext->eaddr, strlen(pContext->eaddr)) == 0) ||
  		      (!pContext->ep && !pContext->mp && strcmp(pContext->eaddr, pContext->mip) == 0) ) 
#else
  		      (!pContext->mp && strncmp(pContext->mip, pContext->eaddr, strlen(pContext->eaddr)) == 0) ) 
#endif
		   {
			 if (!pContext->bonly)
	           pContext->sts = pContext->bebp; //TRUE;
			 else
	           pContext->sts = pContext->bebp = TRUE;
		     break;
		   } else if (!pContext->bonly) {
             pContext->bebp = TRUE;
		   }
		 } else { // メールアドレスでのチェック
		    if (strnicmp(pContext->eaddr,"#blank",6) == 0)  // 2001.01.11 FROM:の空白対策
              pContext->eaddr[0] = '\x0';   // 2001.01.11 空白の比較
			pContext->bBlankFROMPattan = FALSE;
			if (strnicmp(pContext->eaddr,"#<>",3) == 0) { // 2001.08.30 FROM:の空白対策'<>'の場合
			  pContext->bBlankFROMPattan = TRUE;
              pContext->eaddr[0] = '\x0';    // 2001.08.30 FROM:の空白対策'<>'の場合
			}
#ifdef UPDATE_20060710 // ワイルドカードの処理強化
			if (!strstr(pContext->eaddr, "@") ) {
		      pat = strstr(mFrom, "@");
			  if (pat)
				pat++;
			  else
				pat = mFrom;
			} else {
              pat = mFrom;
			}
#else
			if (!strstr(pContext->eaddr, "@") ) {//;
		      pat = strstr(pContext->mUIDFROM, "@");
			  if (pat)
				pat++;
			  else
				pat = pContext->mUIDFROM;
			} else {
             pat = pContext->mUIDFROM;
			}
#endif
		   //if (stricmp(pContext->eaddr, pContext->mUIDFROM) == 0) {
		   // 対象アドレスの比較 *ワイルドカード有効にする 2001.02.06
		   bOK = FALSE;
		   peaddr = strstr(pContext->eaddr,"*");
		   if (peaddr) {
			 *peaddr = '\x0';
			 k = strlen(pContext->eaddr);
			 l = strlen(peaddr+1);
#ifdef UPDATE_20060710 // ２箇所目のワイルドカード
			 if ((peaddr2 = strstr(peaddr+1,"*"))) {
			   *peaddr2 = '\x0';
			   if ((pat2 = strstr(pat, peaddr+1))) {
			     l = strlen(peaddr+1);
			     m = strlen(peaddr2+1);
			     if (k+l <= (int)strlen(pat) && l+m <= (int)strlen(pat2)) {
			       if (strnicmp(pat, pContext->eaddr, k) == 0 &&
			           strnicmp(pat2, peaddr+1, l) == 0 &&
					   strnicmp((pat2+(strlen(pat2)-m)), peaddr2+1, m) == 0)
                     bOK = TRUE;
				 }
			   }
			 } else
#endif
			 {
			   if (k+l <= (int)strlen(pat)) {
			     if (strnicmp(pat, pContext->eaddr, k) == 0 &&
			         strnicmp((pat+(strlen(pat)-l)), peaddr+1, l) == 0)
                   bOK = TRUE;
			   }
			 }
		   } else if (pContext->bBlankFROMPattan) { // 2001.08.30 FROM:の空白対策'<>'の場合
             bOK = (stricmp(pat, pContext->eaddr) == 0 && pContext->bBlankFROM ? TRUE : FALSE);
		   } else
			 bOK = (stricmp(pat, pContext->eaddr) == 0 ? TRUE : FALSE);
		   ///////////////////////////////////////////////
#ifdef EFFECT_DAT_PLUS1
	       pContext->bBlankFROMNumber   = FALSE;
	       pContext->bBlankFROMNoDomain = FALSE;
	       pContext->bBlankFROMHost     = FALSE;
		   if ((pat = strstr(pContext->mUIDFROM, "@"))) { // @マーク以降の処理
			 pat++;
		     if (!strnicmp(pContext->eaddr,"#number", 9)) {       // ＠以降が数字でIPでドメイン指定されている場合
			   if (strlen(pat)) {
				 i = 0;
			     bOK = pContext->bBlankFROMNumber = TRUE;
				 for (i = 0; i < strlen(pat); i++) {
					if (!(*(pat+i) == '.' || 
						  *(pat+i) == '0' || *(pat+i) == '1' ||
						  *(pat+i) == '2' || *(pat+i) == '3' ||
						  *(pat+i) == '4' || *(pat+i) == '5' ||
						  *(pat+i) == '6' || *(pat+i) == '7' ||
						  *(pat+i) == '8' || *(pat+i) == '9' )) { // 数字だけのスせいでないもの
					  bOK = pContext->bBlankFROMNumber = FALSE;
					  break;
					}
				 }
			   }
			 } else if (!strnicmp(pContext->eaddr,"#nodomain", 7)) { // ＠以降がドメイン名で指定されている場合
			   if (strlen(pat) == 0) {
				 bOK = pContext->bBlankFROMNoDomain = TRUE;
			   }
			 } else if (!strnicmp(pContext->eaddr,"#host", 5)) {    // ＠以降がホスト名で指定されている場合
			   if (strlen(pat) && !strstr(pat, ".")) {
			     bOK = pContext->bBlankFROMHost = TRUE;
			   }
			 }
		   } else {
             if (!strnicmp(pContext->eaddr,"#nodomain", 7)) { // ＠以降がドメイン名で指定されている場合
			    bOK = pContext->bBlankFROMNoDomain = TRUE;
			 }
		   }
#endif
		   ///////////////////////////////////////////////
		   if (bOK) {
		   //if (stricmp(pat, pContext->eaddr) == 0) {
			 //pContext->bebp = TRUE;
			 if (!pContext->bonly)
	           pContext->sts = pContext->bebp; //TRUE;
			 else
	           pContext->sts = pContext->bebp = TRUE;
			 break;
		   } else if (!pContext->bonly)
             pContext->bebp = TRUE;
		 }
	  }
	  ///* // v1.29 にて修正
	  if (pContext->bonly) {
		//if (!pContext->bebp)
		  //pContext->bonly = FALSE;
	//	break;
	  }
	  //*/
	  p = fgets(pContext->eaddr,sizeof(pContext->eaddr), pContext->Effefp);
	} 
	fclose(pContext->Effefp);
  }
#ifdef UPDATE_20060618 // 内部宛にしか受け付けないオプションをeffect.datに追加
  if (bDebug) printf("Effective status=[%s]\n", (pContext->sts ? (pContext->bnorelay ? "norelay": (pContext->bdefault ? "default": "true")): "false"));
#else
  if (bDebug) printf("Effective status=[%s]\n", (pContext->sts ? (pContext->bdefault ? "default" : "true") : "false"));
#endif
  return (pContext->bdefault ? FALSE : pContext->sts);
}