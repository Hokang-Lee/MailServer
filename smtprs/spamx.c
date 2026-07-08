////////////////////////////////////////////////////////////
// SPAM Filter spamx.c Copyright K.kawakami       2005.10.19
//
// BOOL FirstTimeMail(char *pMSG)
//      繰り返す同一内容のメール除去
////////////////////////////////////////////////////////////
#include "smtp.h"

#ifdef FIRST_TIME_MAIL // 繰り返す同一内容のメール除去
#include <share.h>

extern char mPASSFTMFile[];
extern char mMailSpoolDir[];
extern DWORD nFTMExpire; // 同一メールの保管期間　デフォルト９０日
extern DWORD nFTMLength; // 比較トークンの長さ　　デフォルト１０バイト
extern DWORD nFTMMargin; // 比較結果の許容範囲　　デフォルト８０％一致で同一と判断
#ifdef CLUSTERING
extern BOOL   nClustering;
extern char   mComName[];
#endif

#ifdef Y2038_BUG
void gettime(SYSTEMTIME *ltime, char *mt);
extern char mMonth[12][4];
extern char mWeek[7][4];
#else
void gettime(time_t *ltime, char *mt);
#endif
void LEVEL_3_ACCEPTLOG(PCLIENT_CONTEXT lpClientContext, char *mess);
void FTMLOG(char *pMsgID, char *pDB);

DWORD MachSize(char *pTkn1, char *pTkn2, DWORD n) {
  DWORD i, nResult = 0;

  for (i = 0; i < n; i++) {
	if (*(pTkn1+i) == *(pTkn2+i))
	  nResult++;
  }
  return nResult;
}

BOOL RCPCheck(char *pSouce, char *pDB) {
  BOOL bMach = FALSE;
  FILE *fpSrc, *fpDB;
  CHAR *p1, *p2, mSRC[1024], mDB[1024];
  char mLog[1024];
  //////////////////////////////////
  // データ比較
  if ((fpSrc = fopen(pSouce, "rt"))) {
    p1 =fgets(mSRC, sizeof(mSRC), fpSrc); // MSG-ID
    p1 =fgets(mSRC, sizeof(mSRC), fpSrc); // Return path
    p1 =fgets(mSRC, sizeof(mSRC), fpSrc); // Rcpient 
    if ((fpDB = fopen(pDB, "rt"))) {
      p2 =fgets(mDB, sizeof(mDB), fpDB); // MSG-ID
      p2 =fgets(mDB, sizeof(mDB), fpDB); // Return path
      p2 =fgets(mDB, sizeof(mDB), fpDB); // Rcpient 
	  bMach = TRUE;
	  while((p1 || !feof(fpSrc)) && (p2 || !feof(fpDB))) {
		if (stricmp(mSRC, mDB)) { // 送信先を比較
		  bMach = FALSE;
		  break;
		}
        p1 =fgets(mSRC, sizeof(mSRC), fpSrc); // Rcpient 
        p2 =fgets(mDB, sizeof(mDB), fpDB);    // Rcpient 
	  }
      fclose(fpDB);
	}
    fclose(fpSrc);
  }

  if (bMach) {
    sprintf(mLog, "RCPCheck(%s. %s) Status = %s", pSouce, pDB, (bMach ? "TRUE" : "FALSE"));
    LEVEL_3_ACCEPTLOG(NULL, mLog);
  }

  return bMach;
}

///////////////////////////////////////////
// 本文が一致するファイルが見つかるか検査
///////////////////////////////////////////
BOOL BodyCheck(char *pSouce, char *pDB) {
  BOOL bMach = FALSE;
  DWORD nMachCount, nMach100;
  FILE *fpSrc, *fpDB;
  CHAR *p1, *p2, mSRC[1024], mDB[1024];
  char mLog[1024];

  nMachCount = nMach100 = 0;
  //////////////////////////////////
  // ヘッダ読飛ばし１
  if ((fpSrc = fopen(pSouce, "rt"))) {
	p1 = fgets(mSRC, sizeof(mSRC), fpSrc);
	while(p1 || !feof(fpSrc)) {
	  if (mSRC[0] == '\r' || mSRC[0] == '\n') 
		break;
	  p1 =fgets(mSRC, sizeof(mSRC), fpSrc);
	}
  }
  //////////////////////////////////
  // ヘッダ読飛ばし２
  if ((fpDB = fopen(pDB, "rt"))) {
	p2 =fgets(mDB, sizeof(mDB), fpDB);
	while(p2 || !feof(fpDB)) {
	  if (mDB[0] == '\r' || mDB[0] == '\n') 
		break;
	  p2 =fgets(mDB, sizeof(mDB), fpDB);
	}
  }
  //////////////////////////////////
  // データ比較
  if (p1 || !feof(fpSrc)) {
    p1 =fgets(mSRC, sizeof(mSRC), fpSrc);
    if (p2 || !feof(fpDB)) {
      p2 =fgets(mDB, sizeof(mDB), fpDB);
	  bMach = TRUE;
	  while((p1 || !feof(fpSrc)) && (p2 || !feof(fpDB))) {
		///////////////////////////////
		// 小文字に統一
		_strlwr(mSRC);
		_strlwr(mDB);
		///////////////////////////////
		nMach100   += __min(nFTMLength, sizeof(mSRC)); // 比較トータル文字数
		nMachCount += MachSize(mSRC, mDB, __min(nFTMLength, sizeof(mSRC))); // 一致した文字数
		//if (strncmp(mSRC, mDB, __min(nFTMLength, sizeof(mSRC)-1))) { // デフォルト１０バイト比較で不一致なら終了
		//  bMach = FALSE;
		//  break;
		//}
        p1 =fgets(mSRC, sizeof(mSRC), fpSrc);
        p2 =fgets(mDB, sizeof(mDB), fpDB);
	  }
	}
  }
  fclose(fpSrc);
  fclose(fpDB);

  if (bMach) {
    if ((nMachCount*100)/nMach100 < nFTMMargin) // 比較結果の80%以下なら別のもの
	  bMach = FALSE;
    sprintf(mLog, "BodyCheck((%s. %s) Total = %lu / Mach = %lu", pSouce, pDB, nMach100, nMachCount);
    LEVEL_3_ACCEPTLOG(NULL, mLog);
    sprintf(mLog, "BodyCheck((%s. %s) Status = %s", pSouce, pDB, (bMach ? "TRUE" : "FALSE"));
    LEVEL_3_ACCEPTLOG(NULL, mLog);
  }

  return bMach;
}

BOOL FirstTimeMail(char *pMSG)
{
  char            mDir[256];
  char            *p, *ppMSG, *ppRCP, mMSG[256], margvRCP[256], mRCP[256];
  char            mOldMSG[256], mOldRCP[256];
  HANDLE          hF;
  WIN32_FIND_DATA FD;
  BOOL bFile = TRUE, bMach = FALSE;
  SYSTEMTIME st;
  FILETIME   ft;
  ULARGE_INTEGER *u1, *u2;
  unsigned __int64 lt;
  unsigned __int64 ltexpire = 36000000000 * 24 * nFTMExpire; // デフォルト90日
  char mLog[1024];
  
  if (!(ppMSG = strrchr(pMSG, '\\'))) 
	return bMach;

  sprintf(mLog, "Start FirstTimeMail(%s)", pMSG);
  LEVEL_3_ACCEPTLOG(NULL, mLog);

  ppMSG++;
  strcpy(mMSG, ppMSG); // チェック対象ファイル
  strcpy(margvRCP, pMSG); // チェック対象ファイル
  if ((p = strrchr(margvRCP, '.'))) {
	*p = '\x0';
	strcat(margvRCP, ".RCP");
    if ((ppRCP = strrchr(margvRCP, '\\'))) {
	  ppRCP++;
	}
  }

  sprintf(mDir, "%smaildb\\*.MSG", mMailSpoolDir);
  hF = FindFirstFile(mDir, &FD); // １回受信したメールＤＢフォルダ
  if (hF != INVALID_HANDLE_VALUE) {
    ///////////////////////////
    // 現在時刻を取得
    GetSystemTime(&st);
    SystemTimeToFileTime(&st, &ft);
    u1 = (ULARGE_INTEGER *)&ft;
    lt = u1->QuadPart;
    ///////////////////////////
    while (bFile) {
     u2 = (ULARGE_INTEGER *)&FD.ftCreationTime;
	 if (lt - u2->QuadPart > ltexpire) { // 有効期限切れのファイル
	   sprintf(mMSG, "%smaildb\\%s", mMailSpoolDir, FD.cFileName);
       DeleteFile(mMSG);
	   strcpy(mRCP, mMSG);
       if ((p = strrchr(mRCP, '.'))) {
	     *p = '\x0';
	     strcat(mRCP, ".RCP");
	   }
       DeleteFile(mRCP);
	 } else {                           // 有効期間内のファイル
	   sprintf(mMSG, "%smaildb\\%s", mMailSpoolDir, FD.cFileName);
	   strcpy(mRCP, mMSG);
       if ((p = strrchr(mRCP, '.'))) {
	     *p = '\x0';
	     strcat(mRCP, ".RCP");
	   }
	   if (RCPCheck(margvRCP, mRCP) &&
		   BodyCheck(pMSG, mMSG)) { // 一致するファイルが見つかった
		 FTMLOG(pMSG, mMSG); // ログの記録
		 //////////////////////////////////
		 // 一致したDBファイル名を保管
         strcpy(mOldMSG, mMSG); //DeleteFile(mMSG);
         strcpy(mOldRCP, mRCP); //DeleteFile(mRCP);
		 //////////////////////////////////
	     bMach = TRUE;
         break;
	   }
	 }
      bFile = FindNextFile( hF, &FD);
    }; 
    FindClose( hF ); 
  }
  //////////////////////////////////////////////
  /// 処理が完了したら届いたメールはＤＢに格納
  /// 一致の場合は一致した元のファイル名でコピー
  /// 新規の場合は、そのままのファイル名でコピー
  sprintf(mMSG, "%smaildb\\%s", mMailSpoolDir, ppMSG);
  CopyFile(pMSG, (bMach ? mOldMSG:mMSG), FALSE);
  sprintf(mRCP, "%smaildb\\%s", mMailSpoolDir, ppRCP);
  CopyFile(margvRCP, (bMach ? mOldRCP:mRCP), FALSE);
  
  sprintf(mLog, "End FirstTimeMail(%s)", pMSG);
  LEVEL_3_ACCEPTLOG(NULL, mLog);

  return bMach;
}

BOOL FirstTimeMail_PASSIP(char *pAddr) {
  FILE *fp;
  int  i, mask, num, addr, start, dot;
  char *p, *pwild, *pmask, mwork[128];
  BOOL bsts = FALSE; // 一致するものが無い

  if ((fp = fopen(mPASSFTMFile, "rt"))) {
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

void FTMLOG(char *pMsgID, char *pDB) {
  FILE *fp;
  {
	  CHAR   mtime[256];
      char   mdata[256], mFTMLogFn[256];
#ifdef Y2038_BUG
      SYSTEMTIME ltime, lt;
#else
      time_t ltime;
   	  struct tm lt;
#endif
      gettime(&ltime, mtime);
	  //time(&ltime); 
#ifdef Y2038_BUG
      _tzset();
	  SystemTimeToTzSpecificLocalTime(NULL, &ltime, &lt);
      sprintf(mdata, "%02d%02d%02d", (lt.wYear%100), lt.wMonth, lt.wDay );
#else
      lt = *localtime(&ltime);
      strftime(mdata, 128, "%y%m%d", &lt );
#endif
#ifdef UPDATE_20260606 // ログの書き込み信頼性向上対策
      WaitForSingleObject(g_hMutexMaildbLog, INFINITE);  // 排他開始
      fpMaildbLog = fp = OpenCloseLog(fpMaildbLog, mDTMaildbLog, "maildblog", mComName, lt);
#endif
#ifdef REGTOFILE
      if (nClustering) {
	    sprintf(mFTMLogFn, "%smaildblog\\%s\\%s.log", mMailSpoolDir, mComName, mdata);
	  } else {
#endif
      sprintf(mFTMLogFn, "%smaildblog\\%s.log", mMailSpoolDir, mdata);
#ifdef REGTOFILE
	  }
#endif
 	  if (*mFTMLogFn != '\x0') {
#ifdef UPDATE_20260606 // ログの書き込み信頼性向上対策
	    if (fpMaildbLog) {
#ifdef Y2038_BUG
          sprintf(mdata, "%02d/%s/%04d:%02d:%02d:%02d",lt.wDay, mMonth[lt.wMonth-1], lt.wYear, lt.wHour, lt.wMinute, lt.wSecond);
#else
          strftime(mdata, 128, "%d/%b/%Y:%H:%M:%S", &lt );
#endif
          fprintf(fpMaildbLog, "[%s] [IN=%s] / [maildb=%s] mach!!\n", mdata, pMsgID, pDB);
      	  fflush(fpMaildbLog);
		}
#else
#ifdef UPDATE_20260603 // ログ出力の排他処理化を追加
        WaitForSingleObject(g_hMutexMaildbLog, INFINITE);  // 排他開始
#endif
        fp = _fsopen(mFTMLogFn, "at", _SH_DENYNO);
	    if (fp) {
#ifdef Y2038_BUG
          sprintf(mdata, "%02d/%s/%04d:%02d:%02d:%02d",lt.wDay, mMonth[lt.wMonth-1], lt.wYear, lt.wHour, lt.wMinute, lt.wSecond);
#else
          strftime(mdata, 128, "%d/%b/%Y:%H:%M:%S", &lt );
#endif
          fprintf(fp, "[%s] [IN=%s] / [maildb=%s] mach!!\n", mdata, pMsgID, pDB);
    	  fclose(fp);
		}
#endif
#ifdef UPDATE_20260603 // ログ出力の排他処理化を追加
        ReleaseMutex(g_hMutexMaildbLog);  // 排他終了
#endif
	  }
	}
}

#endif