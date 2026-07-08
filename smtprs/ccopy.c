////////////////////////////////////////////////////////////
// CCopy.c Copyright K.kawakami
////////////////////////////////////////////////////////////
#include "smtp.h"

extern char    mMailInCopy[];
extern DWORD   nMailInCopy;
#ifdef UPDATE_20211020 // 特定接続元IPからの複写転送先指定無効アドレス設定テーブル追加(ccdisableip.dat)
extern char    mCCDisableFile[]; // 複写転送先指定無効アドレス設定テーブル

BOOL CC_DisableIP(char *pAddr);
#endif

void CarbonCopy(PSmtpContext pContext) {
  char *prcpt;
#ifdef MAILFROM_MATCH_CC   // エンベロープのFROMが一致したメールをCCする。 2011.07.19追加
  BOOL bMach;
  char *p, *pFrom;
  char mFROM[_MAX_PATH*2];
#endif
#ifdef UPDATE_20210314A // 複写転送指定に、指定以外オプションを追加した。
  char *pOther = NULL; 
#endif
#ifdef UPDATE_20210318 // 複写転送アドレスに送信者のアドレスにも送るオプションを追加。
   BOOL bAddFrom = FALSE;
#endif

  if (strlen(mMailInCopy)) {
#ifdef DATA_CASH
    pContext->Rcptfp = fopen(pContext->mFnHead, "at");
#endif
    prcpt = mMailInCopy;
    do {
#ifndef DATA_CASH
      pContext->Rcptfp = fopen(pContext->mFnHead, "at");
#endif
      if (pContext->Rcptfp) {
#ifdef MAILFROM_MATCH_CC   // エンベロープのFROMが一致したメールをCCする。 2011.07.19追加
		bMach = TRUE;
		if ((p = strchr(prcpt, '|')))
		{
		  *p = '\x0';
		  pFrom = prcpt; 
		  prcpt = p+1;
		  _strlwr(pFrom);
		  strcpy(mFROM, pContext->mUIDFROM);
		  _strlwr(mFROM);
#ifdef UPDATE_20210314A // 複写転送指定に、指定以外オプションを追加した。
		  if (pOther = strrchr(pFrom, ','))
		  {
			*pOther = '\x0';
			pOther++;
#ifdef UPDATE_20210318 // 複写転送アドレスに送信者のアドレスにも送るオプションを追加。
			if (*pOther == '2' || *pOther == '3')
              bAddFrom = TRUE;
			if (*pOther == '0' || *pOther == '2')
#else
			if (*pOther == '0')
#endif
			{
			  bMach = FALSE;
		      if (mFROM[0] != '\x0' && !wildcard_stricmp(pFrom, mFROM) ||
			      mFROM[0] == '\x0' && strcmp(pFrom, "*") != 0)
			  {
                bMach = TRUE;
			  }
			} else {
			  bMach = TRUE;
		      if (mFROM[0] != '\x0' && !wildcard_stricmp(pFrom, mFROM) ||
			      mFROM[0] == '\x0' && strcmp(pFrom, "*") != 0)
			  {
                bMach = FALSE;
			  }
			}
			*(pOther - 1) = ',';
		  } else 
#endif
#ifdef UPDATE_20210314A // 複写転送指定に、指定以外オプションを追加した。
		  if (mFROM[0] != '\x0' && !wildcard_stricmp(pFrom, mFROM) ||
			  mFROM[0] == '\x0' && strcmp(pFrom, "*") != 0)
#else
		  if (!strstr(mFROM, pFrom))
#endif
		  {
            bMach = FALSE;
		  }
		  *p = '|';
		}
#endif
#ifndef DATA_CASH
#ifdef MAILFROM_MATCH_CC   // エンベロープのFROMが一致したメールをCCする。 2011.07.19追加
#ifdef UPDATE_20211020 // 特定接続元IPからの複写転送先指定無効アドレス設定テーブル追加(ccdisableip.dat)
		if (bMach && !CC_DisableIP(pContext->PeerAddr))
#else
		if (bMach)
#endif
		{
#endif
#ifdef UPDATE_20210318 // 複写転送アドレスに送信者のアドレスにも送るオプションを追加。
		  if (bAddFrom)
            fprintf(pContext->Rcptfp, "Recipient: %s\n", pContext->mUIDFROM);
#endif
          fprintf(pContext->Rcptfp, "Recipient: %s\n", prcpt);
          fflush(pContext->Rcptfp);
#ifdef MAILFROM_MATCH_CC   // エンベロープのFROMが一致したメールをCCする。 2011.07.19追加
		}
#endif
        fclose(pContext->Rcptfp);
#else
        fputs("Recipient: ", pContext->Rcptfp);
        fputs(prcpt, pContext->Rcptfp);
        fputs("\n", pContext->Rcptfp);
#endif
	  }
      prcpt += strlen(prcpt);
	  prcpt++;
	} while (strlen(prcpt));
#ifdef DATA_CASH
	if (pContext->Rcptfp)
      fclose(pContext->Rcptfp);
#endif
  }
}

#ifdef UPDATE_20211020 // 特定接続元IPからの複写転送先指定無効アドレス設定テーブル追加(ccdisableip.dat)
BOOL CC_DisableIP(char *pAddr) {
  FILE *fp;
  int  i, n, mask, num, addr, start, dot;
  char *p, *pwild, *pmask, mwork[128];
  BOOL bsts = FALSE; // 一致するものが無い

  if ((fp = fopen(mCCDisableFile, "rt"))) {
	p = fgets(mwork, sizeof(mwork), fp); // dummy
	while(p || !feof(fp)) {
	  p = strpbrk(mwork, "\r\n");
	  if (p)
		*p = '\x0';
	  if (mwork[0] != '\x0' &&
	      mwork[0] != ' ' &&
	      mwork[0] != '\'') {  // コメントチェック
#ifdef UPDATE_20070511 // IPアドレス確認ロジックの共有化
	   if ((bsts = IP_COMP(mwork, pAddr)))
	 	 break;
#else
		num = 1; // 比較アドレス数
		if ((pwild = strchr(mwork, '*'))) { // ワイルドカードでのマスク
		  *pwild = '\x0';
		} else if (strchr(mwork, '.')) { // IPv4のみ
		  if ((pwild = strchr(mwork, '/'))) {// ネットマスク
		    *pwild = '\x0';
		    mask = 32 - atoi(++pwild);
		    num = 1 << mask; // 2のべき乗範囲 num <<= mask; // 2のべき乗範囲
			dot = ((mask-1) / 8) + 1;
			if (dot > 1)
			  addr = num / (256*(dot-1));
			 else
			   addr = num;
			for (i = 1; i <= dot; i++) {
			  if ((pwild = strrchr(mwork, '.')))
				*pwild = '\x0';
			}
			start = atoi(pwild + 1);
		  }
		}
		if (num == 1) {
          if (!strncmp(pAddr, mwork, strlen(mwork))) {
		    bsts = TRUE;  // 一致するものがあった
		    break;
		  }
		} else { // ネットマスクの場合
	      for ( i = 0; i < addr; i++) {
			*pwild = '\x0';
			pmask = &mwork[strlen(mwork)];
		    sprintf(pmask, ".%d", start + i);
            if (!strncmp(pAddr, mwork, strlen(mwork))) {
		      bsts = TRUE;  // 一致するものがあった
			  break;
			}
		  }
		  if (bsts)
			break;
		}
#endif
	  }
	  mwork[0] ='\x0';
	  p = fgets(mwork, sizeof(mwork), fp); // dummy
	}
	fclose(fp);
  }
  return bsts;
}
#endif
