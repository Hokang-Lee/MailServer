#include "smtp.h"
#ifdef THIRDPARTY

#include <stdio.h> 
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef ACCEPT_LOG_LEVEL3
void LEVEL_3_ACCEPTLOG(PCLIENT_CONTEXT lpClientContext, char *mess);
#endif
#ifdef UPDATE_20181025 // 外部DLLの呼出しポインタをグローバル化
extern HMODULE hDLL;
#endif

extern  char   mThirdparty[];
extern char    mPASS3rdFile[];
extern BOOL bServiceTerminating;

#ifdef UPDATE_20050329
BOOL ThirdpartyProcess_PASSIP(char *pAddr) {
  FILE *fp;
  int  i, n, mask, num, addr, start, dot;
  char *p, *pwild, *pmask, mwork[128];
  BOOL bsts = FALSE; // 一致するものが無い

  if ((fp = fopen(mPASS3rdFile, "rt"))) {
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

BOOL ThirdpartyProcess(char *pFn, char *pResult) {
  BOOL bsts = TRUE;
#ifndef UPDATE_20181025 // 外部DLLの呼出しポインタをグローバル化
  HMODULE hDLL;
#endif
  FARPROC lpfnDll;    // Function pointer
  char *p, *pm, mMBox[256], mSrc[256], mModule[256], mOption[256]; //オプション
  char mLog[256];
  FILE *fp;

#ifdef UPDATE_20090908 // ウイルスチェック用メモリの初期化 スキャンDLLロード失敗でウイルス情報として初期化されないデータが表示される可能性
  *pResult = '\x0';
#endif

  if (mThirdparty[0] == '\x0') // サードパーティモジュールがない場合は終了。
	return bsts;

#ifdef UPDATE_20071024 // メールフィルタ処理中にCPU100%にならない対策
   _sleep(0);
#endif
#ifdef UPDATE_20070204 // TEMPフォルダ内".MSG"のファイルが読み込み可能か確認
  while(!(fp = fopen(pFn, "rt"))) {
    if (bServiceTerminating)
      break;
    _sleep(WAIT_TIME);
  }
  fclose(fp);
#endif

#ifdef UPDATE_20040720_LOG
  strcpy(mLog, "start ThirdpartyProcess");
  LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
  ///////////////////////////////////
  strcpy(mMBox, pFn);
  if ((p = strrchr(mMBox, '\\')))
	*p = '\x0';
  strcpy(mSrc, pFn);
  ///////////////////////////////////
  if (mThirdparty[0] == '"') {
    strcpy(mModule, &mThirdparty[1]);
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
    strcpy(mModule, mThirdparty);
    pm = strstr(mModule, " ");
  }
  mOption[0] = '\x0';
  if (pm) {
	*pm= '\x0';
    strcpy(mOption, pm+1);
  }
//printf("LoadLibrary(%s)\n", mModule);
#ifdef UPDATE_20181025 // 外部DLLの呼出しポインタをグローバル化
  if (!hDLL)
#endif
  {
    hDLL = LoadLibrary(mModule); // モジュールのファイル名
  }
  if (hDLL) {
    lpfnDll = GetProcAddress(hDLL, "MessageProc");
	if (lpfnDll) {
 //printf("lpfnDll(%s, %s, %s);\n", mMBox, mSrc, mOption);
      bsts = lpfnDll(mMBox, mSrc, mOption); // プロセス実行
	}
#ifndef UPDATE_20181025 // 外部DLLの呼出しポインタをグローバル化
    FreeLibrary(hDLL); // DLL モジュールの開放
#endif
	// mOptionに結果の内容を返す。
	strcpy(pResult, mOption);
  }

#ifdef UPDATE_20040720_LOG
  strcpy(mLog, "end ThirdpartyProcess");
  LEVEL_3_ACCEPTLOG(NULL, mLog);
#endif
  return bsts;
}
#endif
