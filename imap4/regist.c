////////////////////////////////////////////////////////////
// Regist.c Copyright K.kawakami
////////////////////////////////////////////////////////////
#include "imap4.h"
#include "version.h"
#include "service.h"
#include <time.h>

extern BOOL  bDebug;

#define  SERIAL_BASE 18

#ifdef V4
extern INT  nMAXUser;
#endif

#ifdef E_POST
#ifdef FOR_BRIDGEGATE 
#ifdef _ENGLISH_ // 英語版 //8609-6679-0004#%10%#&2853-6678-0004#%10%#
  char mKeyReg[14][9] = {"8pin2CAN","6At 8eak","0NY 5gE ","9*-+3XQ;","-uN -iCA","6oDe6uch","6ITi6.l.","7AnD7.25","9&&@8$%Q","-aNg-iBE","0nUN0DEM","0+**0OK-","0iT 0egg","4T  4RI "};
  char mLev[14][9]    = {"wX0004@@", "~8000A. ", "b 0014e@", " 3004AbC","^!00A0;+", "(@0140'(", "#!04A0&%", "Oo0A00P ", "341400Yx", " 54A00.-", " 9A00012", "%$A140aN", "yHA4A0IU", "mSAA00Wd"};
#else                     //8609-7780-0004#%10%#&2853-7781-0004#%10%#
  char mKeyReg[14][9] = {"8pin2CAN","6At 8eak","0NY 5gE ","9*-+3XQ;","-uN -iCA","7oDe7uch","7ITi7.l.","8AnD8.25","0&&@1$%Q","-aNg-iBE","0nUN0DEM","0+**0OK-","0iT 0egg","4T  4RI "};
  char mLev[14][9]    = {"wX0004@@", "~8000A. ", "b 0014e@", " 3004AbC","^!00A0;+", "(@0140'(", "#!04A0&%", "Oo0A00P ", "341400Yx", " 54A00.-", " 9A00012", "%$A140aN", "yHA4A0IU", "mSAA00Wd"};
#endif
#else
#ifdef V4            //5963-0423-0004#%10%#&6064-0423-0004#%10%#
#ifdef WIN64       //5910-0420-0004#%10%#&6010-0420-0004#%10%#
  char mKeyReg[14][9] = {"Spin6CAN","PAt 0eak","ANY 1gE ",";*-+0XQ;","PuN -iCA","moDe0uch","pITi4.l.","zAnD2.25","!&&@0$%Q","xaNg-iBE","QnUN0DEM","M+**0OK-","=iT 0egg","MT  4RI "};
  char mKeyPopReg[14][9] = {"Spin6CAN","PAt 0eak","ANY 1gE ",";*-+0XQ;","SuN -iCA","moDe0uch","tITi4.l.","pAnD2.25","@&&@0$%Q","YaNg-iBE","unUN0DEM","-+**0OK-","KiT 0egg","iT  4RI "};
  char mLev[14][9]    = {"wX0004@@", "~8000A. ", "b 0014e@", " 3004AbC","^!00A0;+", "(@0140'(", "#!04A0&%", "Oo0A00P ", "341400Yx", " 54A00.-", " 9A00012", "%$A140aN", "yHA4A0IU", "mSAA00Wd"};
#else              //5964-0423-0004#%10%#&6065-0423-0004#%10%#
  char mKeyReg[14][9] = {"Spin6CAN","PAt 0eak","ANY 6gE ",";*-+5XQ;","PuN -iCA","moDe0uch","pITi4.l.","zAnD2.25","!&&@3$%Q","xaNg-iBE","QnUN0DEM","M+**0OK-","=iT 0egg","MT  4RI "};
  char mKeyPopReg[14][9] = {"Spin6CAN","PAt 0eak","ANY 6gE ",";*-+5XQ;","SuN -iCA","moDe0uch","tITi4.l.","pAnD2.25","@&&@3$%Q","YaNg-iBE","unUN0DEM","-+**0OK-","KiT 0egg","iT  4RI "};
  char mLev[14][9]    = {"wX0004@@", "~8000A. ", "b 0014e@", " 3004AbC","^!00A0;+", "(@0140'(", "#!04A0&%", "Oo0A00P ", "341400Yx", " 54A00.-", " 9A00012", "%$A140aN", "yHA4A0IU", "mSAA00Wd"};
#endif
#else
#ifdef V3
  char mKeyReg[14][9] = {"EestEven","Pot PeN ","StopSloT","Top TOP","StarPOOL","mastoPeN","tronpari","ping3YEN","@#$!@21@","Yet KALE","uo  ibis","-/*;-@-@","Kit yak ","it  Mach"};
  char mKeyPopReg[14][9] = {"EestEven","Pot PeN ","StopSloT","Top TOP","StarPOOL","mastoPeN","tronpari","ping3YEN","@#$!@21@","Yet KALE","uo  ibis","-/*;-@-@","Kit yak ","it  Mach"};
#else
  char mKeyReg[14][9] = {"Pen Pace","ret ruBY","eat ears","Man MBA","in  iris","x123xray","20922ton","Ken Kibe","@%  @21@","Let PUB ","it  owe ","mon plan","in  3PO ","tai shad"};
  char mKeyPopReg[14][9] = {"Pen Pace","ret ruBY","eat ears","Man MBA","in  iris","x123xray","20922ton","Ken Kibe","@%  @21@","Let PUB ","it  owe ","mon plan","in  3PO ","tai shad"};
#endif
#endif
#endif
#else
#ifdef V4            //5963-0423-0004#%10%#&6064-0423-0004#%10%#
  char mKeyReg[14][9] = {"Spin6CAN","PAt 0eak","ANY 6gE ",";*-+4XQ;","PuN -iCA","moDe0uch","pITi4.l.","zAnD2.25","!&&@3$%Q","xaNg-iBE","QnUN0DEM","M+**0OK-","=iT 0egg","MT  4RI "};
  char mKeyPopReg[14][9] = {"Spin6CAN","PAt 0eak","ANY 6gE ",";*-+4XQ;","SuN -iCA","moDe0uch","tITi4.l.","pAnD2.25","@&&@3$%Q","YaNg-iBE","unUN0DEM","-+**0OK-","KiT 0egg","iT  4RI "};
  char mLev[14][9]    = {"wX0004@@", "~8000A. ", "b 0014e@", " 3004AbC","^!00A0;+", "(@0140'(", "#!04A0&%", "Oo0A00P ", "341400Yx", " 54A00.-", " 9A00012", "%$A140aN", "yHA4A0IU", "mSAA00Wd"};
#else
#ifdef V3
#ifdef HOME_VERSION   //4510-3400-0005#%010%&4420-3400-0005#%010%#
  char mKeyReg[14][9] = {"4pin4CAN","5At 4eak","1NY 2gE ","0*-+0XQ;","-uN -iCA","3oDe3uch","4ITi4.l.","0AnD0.25","0&&@0$%Q","-aNg-iBE","0nUN0DEM","0+**0OK-","0iT 0egg","5T  5RI "};
  char mKeyPopReg[14][9] = {"4pin4CAN","5At 4eak","1NY 2gE ","0*-+0XQ;","-uN -iCA","3oDe3uch","4ITi4.l.","0AnD0.25","0&&@0$%Q","-aNg-iBE","0nUN0DEM","0+**0OK-","0iT 0egg","5T  5RI "};
#else
  char mKeyReg[14][9] = {"SpinSCAN","PAt Peak","ANY AgE ",";*-+;XQ;","SuN PiCA","moDeouch","tITip.l.","pAnD3.25","@&&@@$%Q","YaNgKiBE","unUNiDEM","-+**-OK-","KiT yegg","iT  MRI "};
  char mKeyPopReg[14][9] = {"SpinSCAN","PAt Peak","ANY AgE ",";*-+;XQ;","PuN iiCA","moDemuch","pITiA.l.","zAnDP.25","!&&@#$%Q","xaNgNiBE","QnUNoDEM","M+**vOK-","=iT 2egg","MT  1RI "};
//  char mKeyPopReg[14][9] = {"SpinSCAN","PAt Peak","ANY AgE ",";*-+;XQ;","SuN PiCA","moDeouch","tITip.l.","pAnD3.25","@&&@@$%Q","YaNgKiBE","unUNiDEM","-+**-OK-","KiT yegg","iT  MRI "};
#endif
#else
  char mKeyReg[14][9] = {"StopSea ","PorTPICO","And AiD ",";:;:;p;O","2YenIton","ePonMKM ","5KM 0kg ","9kg 2KH ","-mAt#^@^","LoNG2ace","cn  1PeN","yAn mea ","hS  AyEn","GoN piam"};
  char mKeyPopReg[14][9] = {"StopSea ","PorTPICO","And AiD ",";:;:;p;O","2Yen2ton","0Pon0KM ","0KM 0kg ","0kg 0KH ","@mAt@^@^","LoNGPace","in  oPeN","mAn pea ","iS  3yEn","toN siam"};
#endif
#endif
#endif

BOOL RegistUser(char *pmode, BOOL *bPassport);
#ifdef V3
BOOL Person_No(char *pKey);
#endif

void Regist(char *regdata)
{
  WriteProfileStringEx(SYSTEM_REG, LIMITKEY, regdata);
  if (RegistUser(NULL, NULL))
	printf("It succeeded.\n");
  else
	printf("It failed. (Illegal key = \"%s\")\n",regdata);
}

BOOL RegistUser(char *pmode,BOOL *bPassport)
{
  int  i;
  BOOL bkey = TRUE, bPOP = TRUE;
  char mInpReg[SERIAL_BASE+15]; //解除キー "SPA;2000@Pop3s-0001-0002K-1605JP"
  int  nkey;

#ifdef V4
  nMAXUser = 0;
#endif
  // POP3の解除キーがあれば優先
  GetProfileStringEx(SYSTEM_REG_POP3, LIMITKEY, "", mInpReg, sizeof(mInpReg));
  if (!mInpReg[0]) {// なければIMAP4のキーにて確認
	bPOP = FALSE;
    GetProfileStringEx(SYSTEM_REG, LIMITKEY, "", mInpReg, sizeof(mInpReg));
  }
  //if (strlen(mInpReg) > 14 && pmode) {
  if (pmode) {
	strcpy(pmode, "Bld.");
    if (strlen(&mInpReg[14]) < 10) {
	  strncat(pmode, "4cAAAJJAB4", strlen(&mInpReg[14])-10);
	} else {
	  for (i = 0; i < 10; i++) {
        *(pmode+i+3) = (int)mInpReg[i+14]+('A'-'0');
	  }
      *(pmode+i+3) = '\x0';
	}
  }
  for (nkey = 0; nkey < (int)14; nkey++) {
    if (bPassport) 
      *bPassport = TRUE;
	if (bPOP) { // POP3の解除キーでチェック
#ifdef V4
	// 5963-0423-0004
    if (nkey == 10) { // コード別制限
	  if (!strncmp(&mInpReg[nkey], &mLev[0][2], 4)) {  // 無制限
	    nMAXUser = 0;
	    nkey+=4;
/*
	  } else if (!strncmp(&mInpReg[nkey], &mLev[1][2], 4)) {  // 5ユーザー
	    nMAXUser = 5;
	    nkey+=4;
	  } else if (!strncmp(&mInpReg[nkey], &mLev[2][2], 4)) {  // 12ユーザー
#ifdef FOR_BRIDGEGATE 
	    nMAXUser = 1000;
#else
	    nMAXUser = 12;
#endif
	    nkey+=4;
	  } else if (!strncmp(&mInpReg[nkey], &mLev[3][2], 4)) {  // 25ユーザー
#ifdef FOR_BRIDGEGATE 
	    nMAXUser = 2500;
#else
	    nMAXUser = 25;
#endif
	    nkey+=4;
*/
	  } else if (!strncmp(&mInpReg[nkey], &mLev[4][2], 4)) { // 50ユーザー
#ifdef FOR_BRIDGEGATE 
	    nMAXUser = 5000;
#else
	    nMAXUser = 50;
#endif
	    nkey+=4;
	  } else if (!strncmp(&mInpReg[nkey], &mLev[5][2], 4)) { // 100ユーザー
	    nMAXUser = 100;
	    nkey+=4;
	  } else if (!strncmp(&mInpReg[nkey], &mLev[6][2], 4)) { // 250ユーザー
	    nMAXUser = 250;
	    nkey+=4;
	  } else if (!strncmp(&mInpReg[nkey], &mLev[7][2], 4)) { // 500ユーザー
	    nMAXUser = 500;
	    nkey+=4;
	  } else if (!strncmp(&mInpReg[nkey], &mLev[8][2], 4)) { // 1000ユーザー
	    nMAXUser = 1000;
	    nkey+=4;
	  } else if (!strncmp(&mInpReg[nkey], &mLev[9][2], 4)) { // 2000ユーザー
	    nMAXUser = 2000;
	    nkey+=4;
	  } else if (!strncmp(&mInpReg[nkey], &mLev[10][2], 4)) { // 3000ユーザー
	    nMAXUser = 3000;
	    nkey+=4;
	  } else if (!strncmp(&mInpReg[nkey], &mLev[11][2], 4)) { // 5000ユーザー
	    nMAXUser = 5000;
	    nkey+=4;
	  } else if (!strncmp(&mInpReg[nkey], &mLev[12][2], 4)) { // 10000ユーザー
	    nMAXUser = 10000;
	    nkey+=4;
/*
	  } else if (!strncmp(&mInpReg[nkey], &mLev[13][2], 4)) { // 50000ユーザー
	    nMAXUser = 50000;
	    nkey+=4;
*/
	  } else {
	    bkey = FALSE;
	    break;
	  }
	} 
#endif
	if (nkey < 14 && mInpReg[nkey] != mKeyReg[nkey][4]) {
	    bkey = FALSE;
	    break;
	  }
	} else {    // IMAP4の専用キーでチェック
#ifdef V4
	// 5963-0423-0004
    if (nkey == 10) { // コード別制限
	  if (!strncmp(&mInpReg[nkey], &mLev[0][2], 4)) {  // 無制限
	    nMAXUser = 0;
	    nkey+=4;
/*
	  } else if (!strncmp(&mInpReg[nkey], &mLev[1][2], 4)) {  // 5ユーザー
	    nMAXUser = 5;
	    nkey+=4;
*/
	  } else if (!strncmp(&mInpReg[nkey], &mLev[2][2], 4)) {  // 12ユーザー
	    nMAXUser = 12;
	    nkey+=4;
	  } else if (!strncmp(&mInpReg[nkey], &mLev[3][2], 4)) {  // 25ユーザー
	    nMAXUser = 25;
	    nkey+=4;
	  } else if (!strncmp(&mInpReg[nkey], &mLev[4][2], 4)) { // 50ユーザー
	    nMAXUser = 50;
	    nkey+=4;
/*
	  } else if (!strncmp(&mInpReg[nkey], &mLev[5][2], 4)) { // 100ユーザー
	    nMAXUser = 100;
	    nkey+=4;
	  } else if (!strncmp(&mInpReg[nkey], &mLev[6][2], 4)) { // 250ユーザー
	    nMAXUser = 250;
	    nkey+=4;
	  } else if (!strncmp(&mInpReg[nkey], &mLev[7][2], 4)) { // 500ユーザー
	    nMAXUser = 500;
	    nkey+=4;
	  } else if (!strncmp(&mInpReg[nkey], &mLev[8][2], 4)) { // 1000ユーザー
	    nMAXUser = 1000;
	    nkey+=4;
	  } else if (!strncmp(&mInpReg[nkey], &mLev[9][2], 4)) { // 2500ユーザー
	    nMAXUser = 2500;
	    nkey+=4;
	  } else if (!strncmp(&mInpReg[nkey], &mLev[10][2], 4)) { // 5000ユーザー
	    nMAXUser = 5000;
	    nkey+=4;
	  } else if (!strncmp(&mInpReg[nkey], &mLev[11][2], 4)) { // 10000ユーザー
	    nMAXUser = 10000;
	    nkey+=4;
	  } else if (!strncmp(&mInpReg[nkey], &mLev[12][2], 4)) { // 25000ユーザー
	    nMAXUser = 25000;
	    nkey+=4;
	  } else if (!strncmp(&mInpReg[nkey], &mLev[13][2], 4)) { // 50000ユーザー
	    nMAXUser = 50000;
	    nkey+=4;
*/
	  } else {
	    bkey = FALSE;
	    break;
	  }
	} 
#endif
	if (nkey < 14 && mInpReg[nkey] != mKeyReg[nkey][4]) {
	    bkey = FALSE;
	    break;
	  }
	}
  }
#ifdef V3 // 別チェック
  if (strlen(mInpReg) == 26 && bkey) {
    bkey = Person_No(&mInpReg[14]);
  } else {
    bkey = FALSE;
  }
#endif

#ifdef BETA
  bkey = TRUE;
#endif

  return bkey;
}

#ifdef LDAP_OPTION_LICENSE // LDAP Optionのライセンスチェックを行なう
BOOL LDAP_Option_Check(void)
{
  BOOL bSts = TRUE;

  __int64 ltn, lt30day = 36000000000 * 24 * 30;
  __int64 ltbegin, ltnow, ltb;
  SYSTEMTIME lt, ft;
  FILETIME   ltime;
  ULARGE_INTEGER *u1;
  CHAR mInpReg[SERIAL_BASE+15]; //解除キー "2007-0919-1127#%10%#"

  GetProfileStringEx(LDAPOPTION_REG, LDAPOPTION_LIMITKEY, "", mInpReg, sizeof(mInpReg));
  if (strncmp(mInpReg,"2007", 4) ||
      strncmp(&mInpReg[5],"0919", 4) ||
      strncmp(&mInpReg[10],"1127", 4) )
  { // 解除キー不一致なら
    ltime.dwHighDateTime = (DWORD)GetProfileIntEx(LDAPOPTION_REG, LDAPOPTION_BEGINHIGH, 0);
    ltime.dwLowDateTime = (DWORD)GetProfileIntEx(LDAPOPTION_REG, LDAPOPTION_BEGINLOW, 0);
    if (ltime.dwHighDateTime == 0 && ltime.dwLowDateTime == 0) 
	{
      GetSystemTime(&lt);
	  SystemTimeToFileTime(&lt, &ltime);
      WriteProfileIntEx(LDAPOPTION_REG, LDAPOPTION_BEGINHIGH, (DWORD)ltime.dwHighDateTime);
      WriteProfileIntEx(LDAPOPTION_REG, LDAPOPTION_BEGINLOW, (DWORD)ltime.dwLowDateTime);
	}
    u1 = (ULARGE_INTEGER *)&ltime;
	ltbegin = u1->QuadPart; //FileTime.dwHighDateTime;
    /// 現在時刻
    GetSystemTime(&ft);
    SystemTimeToFileTime(&ft, &ltime);
    //u1 = (ULARGE_INTEGER *)&ltime;
    ltn  = u1->QuadPart;
    ltb = (__int64)(ltbegin + lt30day);
    if (ltb - ltn > lt30day) 
	{
	  ltb = ltn - lt30day;
	}
    if(ltn > ltb) 
	{
      if (bDebug) printf("'LDAP Option' Exceeded an expiration date.\n"); // 使用期限メッセージ
	  AddToMessageLog(TEXT("Exceeded an expiration date. (LDAP Option)"), 101,  TEXT("EPSTIMAP4S"), EVENTLOG_WARNING_TYPE );
	  //bSts = FALSE;
	} else {
      if (bDebug) printf("'LDAP Option' Licence will be expired in %ld days. \n", ((ltn > ltb) ? 0 : ((ltb - ltn)/(36000000000 * 24)+1)));
	}
  } else {
    if (bDebug) printf("'LDAP Option' Licenced.\n");
  }
  return bSts;
}
#endif

#ifdef Y2038_BUG
BOOL ReSetTimeLimit(__int64 *ltbegin)
#else
BOOL ReSetTimeLimit(time_t *ltbegin)
#endif
{

#ifdef Y2038_BUG
  SYSTEMTIME lt;
  FILETIME   ltime;
  ULARGE_INTEGER *u1;
  if (*ltbegin == 0) {
    GetSystemTime(&lt);
	SystemTimeToFileTime(&lt, &ltime);
	u1 = (ULARGE_INTEGER *)&ltime;
	*ltbegin = u1->QuadPart; //FileTime.dwHighDateTime;
    WriteProfileIntEx(SYSTEM_REG, BEGINHIGH, (DWORD)ltime.dwHighDateTime);
    WriteProfileIntEx(SYSTEM_REG, BEGINLOW, (DWORD)ltime.dwLowDateTime);
  }
#else
  if (*ltbegin == 0) {
    time(ltbegin);
    //WriteProfileIntEx("SYSTEM\\CurrentControlSet\\Services\\SPARS", "Begin", (DWORD)*ltbegin);
    WriteProfileIntEx(SYSTEM_REG, "Begin", (DWORD)*ltbegin);
  }
#endif
  return FALSE;
}

#ifdef Y2038_BUG
BOOL GetTimeLimit(__int64 *ltbegin, char *mode, BOOL *bPassport)
#else
BOOL GetTimeLimit(time_t *ltbegin, char *mode, BOOL *bPassport)
#endif
{

#ifdef Y2038_BUG
  SYSTEMTIME ltime;
#ifdef BETA
  __int64 lt, lt30day = 36000000000 * 24 * 180;
#else
  __int64 lt, lt30day = 36000000000 * 24 * 30;
#endif
  __int64 ltb;
  FILETIME ft;
  ULARGE_INTEGER *u1;
#else
  time_t lt, lt30day = 3600*24*30;
  time_t ltb;
#endif
  BOOL   bLimit = FALSE;
  strcpy(mode, "");
  if (!RegistUser(mode, bPassport)) {
	//strcpy(mode, "trial ");
    *bPassport ^= ReSetTimeLimit(ltbegin);
#ifdef Y2038_BUG
    GetSystemTime(&ltime);
	SystemTimeToFileTime(&ltime, &ft);
	u1 = (ULARGE_INTEGER *)&ft;
	lt  = u1->QuadPart;
	ltb = (__int64)(*ltbegin + lt30day);
	if (ltb - lt > lt30day) 
	  ltb = lt - lt30day;
	sprintf(mode, "Licence will be expired in %ld days. ", ((lt > ltb) ? 0 : ((ltb - lt)/(36000000000 * 24)+1)));
	//sprintf(mode, "trial period limiting on the %ld days. ", ((lt > ltb) ? 0 : ((ltb - lt)/(36000000000 * 24)+1)));
#else
    time( &lt );
	ltb = (time_t)(*ltbegin + lt30day);
	if (ltb - lt > lt30day) 
	  ltb = lt - lt30day;
	sprintf(mode, "Licence will be expired in %ld days. ", ((lt > ltb) ? 0 : : ((ltb - lt)/(3600*24)+1)));
	//sprintf(mode, "trial period limiting on the %ld days. ", ((lt > ltb) ? 0 : ((ltb - lt)/(3600*24)+1)));
#endif
    if(lt > ltb) {
       //if (bDebug) printf("Because the term of the trying-out passed, service was stopped.\n"); // 使用期限メッセージ
#ifdef E_POST
	   if (bDebug) printf("Exceeded an expiration date.\n"); // 使用期限メッセージ
	   AddToMessageLog(TEXT("Exceeded an expiration date."), 101,  TEXT("EPSTIMAP4S"), EVENTLOG_WARNING_TYPE );
#else
	   if (bDebug) printf("Exceeded an expiration date, service was stopped.\n"); // 使用期限メッセージ
	   AddToMessageLog(TEXT("Exceeded an expiration date, service was stopped."), 101, TEXT("SPAIMAP4S stop."), EVENTLOG_WARNING_TYPE );
       //AddToMessageLog(TEXT("Because the term of the trying-out passed, service was stopped."), TEXT("SPARS stop."), EVENTLOG_WARNING_TYPE );
#endif
	   bLimit = FALSE;
	} else {
      bLimit = TRUE;
	}
  } else {
    bLimit = TRUE;
  }
  return bLimit && bPassport;
}

#ifdef V3
BOOL Person_No(char *pKey)
{
  SYSTEMTIME ltime;
  int  nkey, nYear, nMonth, nNo, nSum = 0;
  char c, c1, c2, c3;
  BOOL bResult = FALSE;

  GetSystemTime(&ltime); // 現在日付
  {
	pKey+=1;
    c = *(pKey+4);
    *(pKey+4) = '\x0';
	nYear  = atoi(pKey);
    *(pKey+4) = c;
    c1 = *(pKey+8);
    *(pKey+8) = '\x0';
	nNo    = atoi(pKey+5);
    *(pKey+8) = c1;
	nMonth = atoi(pKey+8);
	if (nMonth >= 1 && nMonth <= 12) {
	  if (nYear == 2002 && nMonth >= 6 ||
	 	  nYear >  2002 && nYear < ltime.wYear ||  
		  nYear == ltime.wYear && nMonth <= ltime.wMonth) {
		  // チェックサム
        for (nkey = 0; nkey < (int)10; nkey++) {
		  if (nkey == 4)
		    continue;
          nSum += (int) *(pKey + nkey) - '0';
		}
		c1 = '#' + (nSum % 10);
		c2 = '0' + (nSum % 10);
		c3 = 'z' - (nSum % 10);
		if (*(pKey-1) == c1 && c == c2 &&  *(pKey+10) == c3)
          bResult = TRUE;
		else if (!(nYear <= 2001 ||
			     nYear == 2002 && nMonth < 6 ||
			     nYear >= 2003 && nMonth > 1)) { // チェックサム付き無し
		  if (*(pKey-1) == '#' && *(pKey+10) == '#' &&
		  	  (c == '0' || c == '1' || c == '2'))
            bResult = TRUE;
		}
	  }
	}
  }
  return bResult;
}
#endif
