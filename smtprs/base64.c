////////////////////////////////////////////////////////////
// Base64 Convertor Copyright K.kawakami
////////////////////////////////////////////////////////////
#include "smtp.h"
#include <windows.h>
#include <stdio.h>
#include <string.h>

#define PROFILE_ROOT_TREE "%s\\"
#define MAX_VALUE_NAME    128
#define MAXMAILDATA       1024 

extern BOOL bDebug;
extern char mMailSpoolDir[];
#ifdef UPDATE_20240219 // 付加表題付きのＭＬでの文字化け対策
extern char mProgPath[];
extern char mUTF8ToSJISTBL[]; // レジストリ：バイナリの限界値 [4096];
extern DWORD   nUTF8ToSJISTBL;
#endif
#ifdef UPDATE_20240228 // 付加表題もまとめてパックするオプション
extern BOOL    bMLtknInc;  // TRUE:まとめる FALSE:分離したまま(従来)
#endif

#ifdef CLUSTERING
extern BOOL nClustering;
#endif

int QuickConvertBuff(int argc, char *argv[], char *buff);
void QuotedPrintableDecodeLine(unsigned char * pSrc, unsigned char * pDest);
int UTF7N2SJIS(const char * ssrc,char * sdst,int idstsz);
int UTF8N2SJIS(const char * ssrc,char * sdst,int idstsz);

#ifdef UPDATE_20240219 // 付加表題付きのＭＬでの文字化け対策
#ifdef UPDATE_20240221 // 文字コード区切りでないデータへの対応
void UTF8Normalization(char *pwork, char *pstack) // SJISへの変換の為の正規化
#else
void UTF8Normalization(char *pwork) // SJISへの変換の為の正規化
#endif
{
  FILE *fp;
  char *pch = pwork;
  char *ptbl;

#ifndef UPDATE_20240221 // 文字コード区切りでないデータへの対応
  if (nUTF8ToSJISTBL == 0)
	return;
#endif
  /////////////////////////////////////////////
  while (*pch != NULL) 
  {
    if (((char)*pch & (char)0xe0) == (char)0xc0 &&
		((char)*(pch+1) & (char)0xc0) == (char)0x80 ) // ２バイト文字
    {
      pch+=2;
    }
    else if (((char)*pch & (char)0xe0) == (char)0xe0 &&
		     ((char)*(pch+1) & (char)0xc0) == (char)0x80 &&
			 ((char)*(pch+2) & (char)0xc0) == (char)0x80 ) // ３バイト文字
	{
#ifdef UPDATE_20240221 // 文字コード区切りでないデータへの対応
      if (nUTF8ToSJISTBL != 0)
	  {
#endif
	    //E28892,E28094 '‐'	-> EFBC8D に置きかえ
	    ptbl = mUTF8ToSJISTBL;
        while(*ptbl)
		{
          if ((char)*pch == (char)*ptbl &&
	          (char)*(pch+1) == (char)*(ptbl+1) &&
	          (char)*(pch+2) == (char)*(ptbl+2) ) 
		  {
	        *pch = (char)*(ptbl+3);
	        *(pch+1) = (char)*(ptbl+4);
	        *(pch+2) = (char)*(ptbl+5);
		  }
		  if (ptbl = strpbrk(ptbl, "\t "))
		    ptbl++;
		  else
		    break;
		} 
#ifdef UPDATE_20240221 // 文字コード区切りでないデータへの対応
      }
#endif
	  pch+=3;
    }
#ifdef UPDATE_20240221 // 文字コード区切りでないデータへの対応
	else if (((char)*pch & (char)0x80) == (char)0x00) // １バイト文字
	{
	  pch++;
	}
    else // １バイト文字＆他
    { 
      strcpy(pstack, pch);
	  *pch='\x0';
      pch+=strlen(pstack);
    }
#else
    else // １バイト文字＆他
    { 
      pch++;
    }
#endif
  }
}
#endif

#ifdef UPDATE_20080517 // 上長宛承認メールの承認否認メールに対象メールの題名情報などを記録する
void EncodeString(char *line, int inlen, char *dest)
{
  char *p;
  int  i, n = 0;

   p = line;
   *dest = '\x0';
   for (i = 0; i < inlen; i++) {
	 *(dest+n) = '%';
	 switch (*p & 0xf) {
	   case 0x0: *(dest+n+2) = '0'; break;
	   case 0x1: *(dest+n+2) = '1'; break;
	   case 0x2: *(dest+n+2) = '2'; break;
	   case 0x3: *(dest+n+2) = '3'; break;
	   case 0x4: *(dest+n+2) = '4'; break;
	   case 0x5: *(dest+n+2) = '5'; break;
	   case 0x6: *(dest+n+2) = '6'; break;
	   case 0x7: *(dest+n+2) = '7'; break;
	   case 0x8: *(dest+n+2) = '8'; break;
	   case 0x9: *(dest+n+2) = '9'; break;
	   case 0xa: *(dest+n+2) = 'A'; break;
	   case 0xb: *(dest+n+2) = 'B'; break;
	   case 0xc: *(dest+n+2) = 'C'; break;
	   case 0xd: *(dest+n+2) = 'D'; break;
	   case 0xe: *(dest+n+2) = 'E'; break;
	   case 0xf: *(dest+n+2) = 'F'; break;
	 }
	 switch ((*p >> 4) & 0xf) {
	   case 0x0: *(dest+n+1) = '0'; break;
	   case 0x1: *(dest+n+1) = '1'; break;
	   case 0x2: *(dest+n+1) = '2'; break;
	   case 0x3: *(dest+n+1) = '3'; break;
	   case 0x4: *(dest+n+1) = '4'; break;
	   case 0x5: *(dest+n+1) = '5'; break;
	   case 0x6: *(dest+n+1) = '6'; break;
	   case 0x7: *(dest+n+1) = '7'; break;
	   case 0x8: *(dest+n+1) = '8'; break;
	   case 0x9: *(dest+n+1) = '9'; break;
	   case 0xa: *(dest+n+1) = 'A'; break;
	   case 0xb: *(dest+n+1) = 'B'; break;
	   case 0xc: *(dest+n+1) = 'C'; break;
	   case 0xd: *(dest+n+1) = 'D'; break;
	   case 0xe: *(dest+n+1) = 'E'; break;
	   case 0xf: *(dest+n+1) = 'F'; break;
	 }
	 n += 3;
     *(dest+n) = '\x0';
	 *p++;
   }
}
#endif

BOOL CheckB64Line(char *p)
{
	// Note that this works only on ASCII machines.
	int  n;
	BOOL sts = TRUE;
	if (*p == '\x0')  // データがないならチェックしない
	  return FALSE;
	///////////////////////////////
	// 4の倍数で４０バイト以上
	n = strlen(p);
	if (n % 4 != 0 && n < 40)
	  return FALSE;
	///////////////////////////////
	while (*p) {
	  if ( !('A' <= *p && *p <= 'Z') &&
	       !('a' <= *p && *p <= 'z') &&
	       !('0' <= *p && *p <= '9') &&
	        (*p != '+') &&
	        (*p != '/') &&
		    (*p != '=') ) {
		 sts = FALSE;
		 break;
	  }
	  p++;
	}
	return sts;
}

int DecodeValue(const int c)
{
	// Note that this works only on ASCII machines.
	if ('A' <= c && c <= 'Z')
		return c - 'A';
	if ('a' <= c && c <= 'z')
		return c - 'a' + 26;
	if ('0' <= c && c <= '9')
		return c - '0' + 52;
	if (c == '+')
		return 62;
	if (c == '/')
		return 63;
	if (c == '=')
		return -1;
	return -2;
}

int Base64DecodeLine(char * pszLine, int nLineLen, unsigned char * uchBuffer, int nBufferLen) {
	int iLineIndex = 0;
	int iBufferIndex = 0;
	unsigned char m_uchStoredChars[4];
	int		m_iChars = 0;

#ifdef UPDATE_20060116 // SMTP認証でのバッファオーバーフロー対策
	nLineLen = min(nLineLen, nBufferLen-1);
#endif
	while (iLineIndex < nLineLen) {
		// Group together four characters for decode.
		while (iLineIndex < nLineLen && m_iChars < 4) {
			int c = pszLine[iLineIndex++];
			// Ignore characters that aren't BASE64 characters
            // (e.g., spaces, CRLF, etc.).
			if (DecodeValue(c) != -2)
				m_uchStoredChars[m_iChars++] = (unsigned char) c;
		}

		if (m_iChars == 4) {
			// We've got four characters, so decode them.
			m_iChars = 0;

			// Decode first byte.
			if (iBufferIndex == nBufferLen) return -1; //RESULT_ERROR;
			uchBuffer[iBufferIndex++] = (unsigned char)
				(((unsigned char)DecodeValue(m_uchStoredChars[0]) << 2) |
				((unsigned char)DecodeValue(m_uchStoredChars[1]) >> 4));
			//if (uchBuffer[iBufferIndex-1] == '\x0')
			  //uchBuffer[iBufferIndex-1] = ' ';
			uchBuffer[iBufferIndex] = '\x0';

			// Decode second byte.
			if (iBufferIndex == nBufferLen) return -1; //RESULT_ERROR;
			if (m_uchStoredChars[2] == '=') return iBufferIndex;
			uchBuffer[iBufferIndex++] = (unsigned char)
				(((unsigned char)DecodeValue(m_uchStoredChars[1]) << 4) |
				((unsigned char)DecodeValue(m_uchStoredChars[2]) >> 2));
			//if (uchBuffer[iBufferIndex-1] == '\x0')
			  //uchBuffer[iBufferIndex-1] = ' ';
			uchBuffer[iBufferIndex] = '\x0';

			// Decode third byte.
			if (iBufferIndex == nBufferLen) return -1; //RESULT_ERROR;
			if (m_uchStoredChars[3] == '=') return iBufferIndex;
			uchBuffer[iBufferIndex++] = (unsigned char)
				(((unsigned char)DecodeValue(m_uchStoredChars[2]) << 6) |
				((unsigned char)DecodeValue(m_uchStoredChars[3])));
			//if (uchBuffer[iBufferIndex-1] == '\x0')
			  //uchBuffer[iBufferIndex-1] = ' ';
			uchBuffer[iBufferIndex] = '\x0';
		}
	}
    // Return the count of decoded bytes.
    return iBufferIndex;
}

#ifdef UPDATE_20070928 // メーリングリスト MIME-Qへの対応
#ifdef UPDATE_20050107
void LineConv(char *dest, INT ndest, char *src, char *pack)
#else
void LineConv(char *dest, char *src, char *pack)
#endif
{
  char *p, *p1, pe, *pk;
  char *itmv[4], itm[4][MAXMAILDATA];
  char work[MAXMAILDATA];
  int  ln;
  int  nMime;
  char *q1;
  BOOL bUTF7 = FALSE;
  BOOL bUTF8 = FALSE;
#ifdef UPDATE_20240219 // 付加表題付きのＭＬでの文字化け対策
  char mStack[MAXMAILDATA]; // UTF8文字変換時コードの切れ目と違う場合の余データの保管用
#endif

   *pack = '\x0';
   p1 = src;
   *dest = '\x0';
#ifdef UPDATE_20240221 // 文字コード区切りでないデータへの対応
   mStack[0] = '\x0';
#endif
   while (*p1) {
    p = strstr(p1, "=?"); // BASE64でパックされているか？
	///////////////////
    if (p && strnicmp(p, "=??", 3)) {
 	   //////////////////////////
	   *p = '\x0';
#ifdef UPDATE_20050107
	   if (ndest > (INT)(strlen(dest) + strlen(p1))) // 領域オーバーしない範囲でコピー
#endif
	     strcat(dest, p1);
	   *p = '=';
	   //////////////////////////
	   nMime = 0;
           q1 = NULL;
	   if ((q1 = strchr(p+2, '?'))) {
#ifdef UPDATE_20241003 //MIME-B/Qエンコードの指定文字が小文字だとエンコードに失敗していた不具合
	     if (!strnicmp(q1, "?B?", 3))
			nMime = 1;
		 else if (!strnicmp(q1, "?Q?", 3))
			nMime = 2;
#else
	     if (!strncmp(q1, "?B?", 3))
			nMime = 1;
		 else if (!strncmp(q1, "?Q?", 3))
			nMime = 2;
#endif
#ifdef UPDATE_20241111_ML // メーリングリストの題名挿入時にMIME指定毎に異なる文字コードが指定されていると文字化けする
		 bUTF7 = bUTF8 = FALSE;
#endif
		 if (!strnicmp(p+2, "utf-7", 5))
			bUTF7 = TRUE;
		 if (!strnicmp(p+2, "utf-8", 5))
			bUTF8 = TRUE;
 	     p1 = (strstr(q1+3,"?="));
	   }
	   if ((q1 = strchr(p+2, '?'))) {
#ifdef UPDATE_20241003 //MIME-B/Qエンコードの指定文字が小文字だとエンコードに失敗していた不具合
		 if (!strnicmp(q1, "?B?", 3))
			nMime = 1;
		 else if (!strnicmp(q1, "?Q?", 3))
			nMime = 2;
#else
		 if (!strncmp(q1, "?B?", 3))
			nMime = 1;
		 else if (!strncmp(q1, "?Q?", 3))
			nMime = 2;
#endif
		 if (p1 = (strstr(q1+3, "?="))) {
#ifdef UPDATE_20241111_ML // メーリングリストの題名挿入時にMIME指定毎に異なる文字コードが指定されていると文字化けする
		   memset(pack, 0, p1-p+2+1);
#endif
	   	   strncpy(pack, p, p1-p+2); // パックされている言語を得る
		   if (nMime != 2) {
#ifdef UPDATE_20241003 //MIME-B/Qエンコードの指定文字が小文字だとエンコードに失敗していた不具合
		     if ((pk = strstr(pack,"B?")) || (pk = strstr(pack,"b?")))
#else
		     if ((pk = strstr(pack,"B?")))
#endif
		       *(pk+2) = '\x0';
		     else
		       *pack = '\x0';
#ifdef UPDATE_20241003 //MIME-B/Qエンコードの指定文字が小文字だとエンコードに失敗していた不具合
		   } else if ((pk = strstr(pack,"Q?")) || (pk = strstr(pack,"q?"))) {
#else
		   } else if ((pk = strstr(pack,"Q?"))) {
#endif
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
	   //memset(work, 0, sizeof(work));
	   work[0] = '\x0';
       if (nMime == 1)
         Base64DecodeLine(p, ln, (unsigned char *)work, MAXMAILDATA);
	   else if (nMime == 2) {
	     QuotedPrintableDecodeLine((unsigned char *)p, (unsigned char *)work);
	   } else {
	     strcpy(work, p);
	   }
	   if (bUTF7) { 
		 UTF7N2SJIS(work, itm[2], sizeof(itm[2]));
	   } else if (bUTF8) {
#ifdef UPDATE_20240221 // 文字コード区切りでないデータへの対応
		 strcat(mStack, work); // 切れ端データの結合
		 strcpy(work, mStack);
		 mStack[0] = '\x0';
         UTF8Normalization(work, mStack); // SJISへの変換の為の正規化
#else
#ifdef UPDATE_20240219 // 付加表題付きのＭＬでの文字化け対策
         UTF8Normalization(work); // SJISへの変換の為の正規化
#endif
#endif
		 UTF8N2SJIS(work, itm[2], sizeof(itm[2]));
	   } else {
#ifdef UPDATE_20241111_ML // メーリングリストの題名挿入時にMIME指定毎に異なる文字コードが指定されていると文字化けする
         //////コード変換 (EUC, JIS) -> S-JIS /////////////
         strcpy(itm[0], "");         itmv[0] = itm[0];
         strcpy(itm[1], "-s");       itmv[1] = itm[1];
         strcpy(itm[2], work);       itmv[2] = itm[2];
         QuickConvertBuff(3, (char **)&itmv, (char *)itm[2]);
         //////////////////////////////////////////////////
#else
		 /*
         //////コード変換 (EUC, JIS) -> S-JIS /////////////
         strcpy(itm[0], "");         itmv[0] = itm[0];
         strcpy(itm[1], "-s");       itmv[1] = itm[1];
         strcpy(itm[2], work);       itmv[2] = itm[2];
         QuickConvertBuff(3, (char **)&itmv, (char *)itm[2]);
         //////////////////////////////////////////////////
		 */
		 strcpy(itm[2], work);
#endif
	   }
#ifdef UPDATE_20050107
	   if (ndest > (INT)(strlen(dest) + strlen(itm[2]))) // 領域オーバーしない範囲でコピー
#endif
         strcat(dest, itm[2]);
	   //strcat(dest, work);
       if (p1) {
#ifdef UPDATE_20170420 // MIME-Qエンコードの開始しか含まれていないヘッダがあるとハングする不具合
         if (!q1)
		 {
		    p1+=strlen(p1);
		 } else 
#endif
		 {
	       *(p1+2) = pe;
	       p1+=2;
		 }
	   } else
		break;
	 } else {
#ifdef UPDATE_20151118 // パックされている言語の末尾のホワイトスペースを除去する
       while(*p1 == '\x09' || *p1 == '\x20')
	   {
	     p1++;
	   }
#endif

		/*
       p = strstr(p1, "\x1b$"); // パック無しのデータか？
       if (p) {
         //////コード変換 (EUC, JIS) -> S-JIS /////////////
         strcpy(itm[0], "");         itmv[0] = itm[0];
         strcpy(itm[1], "-s");       itmv[1] = itm[1];
         strcpy(itm[2], p1);       itmv[2] = itm[2];
         QuickConvertBuff(3, (char **)&itmv, (char *)itm[2]);
         //////////////////////////////////////////////////
         strcat(dest, itm[2]);
         break;
	   } else {
	     strcat(dest, p1);
	     p1 += strlen(p1);
	   }
	   */
#ifdef UPDATE_20050107
	   if (ndest > strlen(dest) + strlen(p1)) // 領域オーバーしない範囲でコピー
#endif
	   {
	     strcat(dest, p1);
	     p1 += strlen(p1);
	   }
#ifdef UPDATE_20080916 // メーリングリスト宛の題名が長いメールでループする
	   else if (strlen(dest) == 0)
	   {
	     strncpy(dest, p1, ndest-32);
		 *(dest+ndest-32) = '\x0';
	     p1 += strlen(p1);
	   } else {
	     p1 += strlen(p1);
	   }
#endif

	 }
   }
}
#else
#ifdef UPDATE_20050107
void LineConv(char *dest, INT ndest, char *src, char *pack)
#else
void LineConv(char *dest, char *src, char *pack)
#endif
{
  char *p, *p1, pe, *pk;
  char *itmv[4], itm[4][MAXMAILDATA];
  char work[MAXMAILDATA];
  int  ln, nEnd = 0;

   *pack = '\x0';
   p1 = src;
   *dest = '\x0';
   while (*p1) {
     //p = strstr(p1, "=?iso-2022-jp?B?");
     p = strstr(p1, "=?"); // BASE64でパックされているか？
     //if (p == NULL)
       //p = strstr(p1, "=?ISO-2022-JP?B?");
     if (p) {
 	   //////////////////////////
	   *p = '\x0';
#ifdef UPDATE_20050107
	   if (ndest > (INT)(strlen(dest) + strlen(p1))) // 領域オーバーしない範囲でコピー
#endif
         strcat(dest, p1);
	   *p = '=';
	   //////////////////////////
 	   p1 = (strstr(p,"?="));
	   if (p1) {
		 strncpy(pack, p, p1-p+2); // パックされている言語を得る
		 pk = strstr(pack,"B?");
		 if (pk)
		   *(pk+2) = '\x0';
		 else 
		   *pack = '\x0';
	     pe = *(p1+2);
	     *(p1+2) = '\x0';
	   }
	   //p += 16;
	   p += strlen(pack);
	   ln = strlen(p);
       Base64DecodeLine(p, ln, (unsigned char *)work, MAXMAILDATA);
//printf("%s\n", work);
       //////コード変換 (EUC, JIS) -> S-JIS /////////////
       //strcpy(itm[0], "");         itmv[0] = itm[0];
       //strcpy(itm[1], "-s");       itmv[1] = itm[1];
       //strcpy(itm[2], work);       itmv[2] = itm[2];
       //QuickConvertBuff(3, (char **)&itmv, (char *)itm[2]);
       //////////////////////////////////////////////////
       //strcat(dest, itm[2]);
#ifdef UPDATE_20050107
	   if (ndest > (INT)(strlen(dest) + strlen(work))) // 領域オーバーしない範囲でコピー
#endif
	     strcat(dest, work);
       if (p1) {
	     *(p1+2) = pe;
	     p1+=2;
	   } else
		break;
	 } else {
	   /*
       p = strstr(p1, "\x1b$"); // パック無しのデータか？
       if (p) {
         //////コード変換 (EUC, JIS) -> S-JIS /////////////
         strcpy(itm[0], "");         itmv[0] = itm[0];
         strcpy(itm[1], "-s");       itmv[1] = itm[1];
         strcpy(itm[2], p1);       itmv[2] = itm[2];
         QuickConvertBuff(3, (char **)&itmv, (char *)itm[2]);
         //////////////////////////////////////////////////
         strcat(dest, itm[2]);
         break;
	   } else {
	     strcat(dest, p1);
	     p1 += strlen(p1);
	   }
	   */
	   ///////////////////////
#ifdef UPDATE_20050107
	   if (ndest > strlen(dest) + strlen(p1)) { // 領域オーバーしない範囲でコピー
#endif
	     strcat(dest, p1);
	     p1 += strlen(p1);
#ifdef UPDATE_20050107
	   }
#endif
	   ///////////////////////
	 }
   }
}
#endif

DWORD GetMimeType(LPCTSTR lpAppName, LPCTSTR lpKeyName, LPCTSTR lpDefault, LPTSTR lpReturnedString, DWORD nSize) {
  HKEY hKeyRoot = HKEY_CLASSES_ROOT;
  HKEY hKey;
  CHAR mkey[256];
  DWORD  cbValueName = MAX_VALUE_NAME;
  DWORD  retCode;
  DWORD  dwIndex = 0;	         // index of subkey to enumerate 
  DWORD  dwType;
  int    i = 0;
#ifdef REGTOFILE
  HANDLE hFile;
#endif

  // READ THE KEY DATA.
  memset(lpReturnedString, 0, nSize);
  strcpy(lpReturnedString, lpDefault);
  sprintf(mkey,PROFILE_ROOT_TREE,lpAppName);

   do {
	 if (i > 0)
	  _sleep(0);
#ifdef REGTOFILE
	 if (nClustering && !strnicmp(mkey, "software\\emwac", 14)) {
       retCode = 
         OpenKeyFile(mMailSpoolDir,
		             mkey,
					 (char *)lpKeyName,
					 &hFile);

	 } else
#endif
	 {
     retCode = 
      RegOpenKeyEx(hKeyRoot,       // Key handle at root level.
                   (LPCTSTR)mkey,  // Path name of child key.
                   0,              // Reserved.
                   KEY_READ,       // Requesting read access.
                   &hKey);         // Address of key to be returned.
	 }
  } while(retCode == ERROR_INVALID_HANDLE && i++ < 3);  // ERROR_INVALID_HANDLE = 6
 
  if (retCode == ERROR_SUCCESS) {
	dwType = REG_SZ;
#ifdef REGTOFILE
	 if (nClustering && !strnicmp(mkey, "software\\emwac", 14)) {
       retCode =
		 KeyFileQueryValueEx(mMailSpoolDir,
		                     mkey,
					         (char *)lpKeyName,
							 &hFile,
							 dwType,
							 (LPBYTE)lpReturnedString,
							 &nSize);
	     CloseHandle(hFile);
	 } else
#endif
	 {
    retCode =
	   RegQueryValueEx(hKey,            // handle of key to query 
                      (LPSTR)lpKeyName, // address of name of value to query 
                      0,                // reserved 
                      &dwType,  // address of buffer for value type 
                      (LPBYTE)lpReturnedString,  // address of data buffer 
                      &nSize  // address of data buffer size 
                      ); 
    RegCloseKey(hKey);
	 }
	if (retCode) {
      strcpy(lpReturnedString, lpDefault);
	}
  } else
    strcpy(lpReturnedString, lpDefault);

  return nSize;
}

void translateUue2Base64(char *line, int inlen, char *newline)
{
	int  outlen, i, c;
	register char *p, *q;

	/*
	 * Figure out how many characters should be in the line.
	 */
    if (line == NULL)
      return;
	//inlen = strlen(line);
	if (inlen == 0)
      return;
	outlen = (int) (inlen * 8 / 6);
    if ((int) (inlen * 8 % 6) > 0)
      outlen++;

	/*
	 * Pad the line to the right number of spaces, in case it got
	 * truncated along the line somewhere.
	 */
	line[outlen+1] = '\0';

	/*
	 * Now translate from uuencode's character set to base64's.
	 */
    q = newline; p = line;
    for (i = 0; i < outlen; i++) {
       switch((int)(i % 4)) {
          case 0: c = ((int)*p >> 2) & 0x3f;
                  break;
          case 1: c = (((int)*p << 4) & 0x30);
                  p++;
                  c |=(((int)*p >> 4) & 0x0f);
                  break;
          case 2: c = (((int)*p << 2) & 0x3c);
                  p++;
                  c |=(((int)*p >> 6) & 0x03);
                  break;
          case 3: c = (int)*p & 0x3f;
                  p++;
                  break;
       }

       switch ((int)(c & 0x3f)) {
           case  0: *q++ = 'A'; break;
           case  1: *q++ = 'B'; break;
           case  2: *q++ = 'C'; break;
           case  3: *q++ = 'D'; break;
           case  4: *q++ = 'E'; break;
           case  5: *q++ = 'F'; break;
           case  6: *q++ = 'G'; break;
           case  7: *q++ = 'H'; break;
           case  8: *q++ = 'I'; break;
           case  9: *q++ = 'J'; break;
           case 10: *q++ = 'K'; break;
           case 11: *q++ = 'L'; break;
           case 12: *q++ = 'M'; break;
           case 13: *q++ = 'N'; break;
           case 14: *q++ = 'O'; break;
           case 15: *q++ = 'P'; break;
           case 16: *q++ = 'Q'; break;
           case 17: *q++ = 'R'; break;
           case 18: *q++ = 'S'; break;
           case 19: *q++ = 'T'; break;
           case 20: *q++ = 'U'; break;
           case 21: *q++ = 'V'; break;
           case 22: *q++ = 'W'; break;
           case 23: *q++ = 'X'; break;
           case 24: *q++ = 'Y'; break;
           case 25: *q++ = 'Z'; break;
           case 26: *q++ = 'a'; break;
           case 27: *q++ = 'b'; break;
           case 28: *q++ = 'c'; break;
           case 29: *q++ = 'd'; break;
           case 30: *q++ = 'e'; break;
           case 31: *q++ = 'f'; break;
           case 32: *q++ = 'g'; break;
           case 33: *q++ = 'h'; break;
           case 34: *q++ = 'i'; break;
           case 35: *q++ = 'j'; break;
           case 36: *q++ = 'k'; break;
           case 37: *q++ = 'l'; break;
           case 38: *q++ = 'm'; break;
           case 39: *q++ = 'n'; break;
           case 40: *q++ = 'o'; break;
           case 41: *q++ = 'p'; break;
           case 42: *q++ = 'q'; break;
           case 43: *q++ = 'r'; break;
           case 44: *q++ = 's'; break;
           case 45: *q++ = 't'; break;
           case 46: *q++ = 'u'; break;
           case 47: *q++ = 'v'; break;
           case 48: *q++ = 'w'; break;
           case 49: *q++ = 'x'; break;
           case 50: *q++ = 'y'; break;
           case 51: *q++ = 'z'; break;
           case 52: *q++ = '0'; break;
           case 53: *q++ = '1'; break;
           case 54: *q++ = '2'; break;
           case 55: *q++ = '3'; break;
           case 56: *q++ = '4'; break;
           case 57: *q++ = '5'; break;
           case 58: *q++ = '6'; break;
           case 59: *q++ = '7'; break;
           case 60: *q++ = '8'; break;
           case 61: *q++ = '9'; break;
           case 62: *q++ = '+'; break;
           case 63: *q++ = '/'; break;
           default: *q++ = *p;  break;
		}
    }

	/*
	 * I'm not sure this loop is needed.
	 */
	for (p = newline; *p != (int)NULL; p++) {
		if (*p == ' ')
			*p = 'A';
	}
	
	/*
	 * Add in base64's special notification of padding.
	 */
	if ((inlen % 3) == 2) {
		*q++ = '=';
	}
	else if ((inlen % 3) == 1) {
		*q++ = '=';
		*q++ = '=';
	}
	*q = '\0';
}

void SubjectConv(FILE *Tempfp, char *mMLWord, char *mMLtkn, char *mSubject, char *mDBSubject, char *mPack) {
  char *pMLCnt, *pMLEnd, *pSubject, *pRe, *pMLNo;
  char mMime[1024], mtop[5];
  int  nML;
  BOOL bRe;
#ifdef UPDATE_20151118A // メーリングリスト投稿時にSubjectヘッダがEUC_JPでパックされていると文字化けする。
  char *pPack;
#endif
#ifdef UPDATE_20241115_ML // MLへの付加表題がMIMEエンコードで記述されていると、付加表題の付け替えに失敗する
  char mMLWordEnc[512], mPackEnc[512];
#endif
#ifdef UPDATE_20241116_ML // MLへの付加表題がMIMEエンコードで記述されていると、付加表題の付け替えに失敗する
  char cMime, *pMimest, *pMimeed;
#endif
#ifdef UPDATE_20070927 // メーリングリストの付加ヘッダに連番指定がないとハングする
   pMLEnd = &mMLWord[strlen(mMLWord)-1];
#endif

#ifdef UPDATE_20170923 // メーリングリストに付加表題が無いとき題名を再構築しない
   if (!mMLWord[0])
   {
     fputs(mDBSubject, Tempfp);
	 return;
   }
#endif
#ifdef UPDATE_20241115_ML // MLへの付加表題がMIMEエンコードで記述されていると、付加表題の付け替えに失敗する
#ifdef UPDATE_20241116_ML // MLへの付加表題がMIMEエンコードで記述されていると、付加表題の付け替えに失敗する
   if (pMimest = strstr(mMLWord, "=?"))
   {
     if (pMimeed = strstr(pMimest, "?="))
	 {
	   *pMimest = '\x0';
	   strcpy(mMLWordEnc, mMLWord);
	   *pMimest = '=';
	   cMime = *(pMimeed+2);
       *(pMimeed+2) = '\x0';
       LineConv(&mMLWordEnc[strlen(mMLWordEnc)], sizeof(mMLWordEnc)-strlen(mMLWordEnc), pMimest, mPackEnc);
       *(pMimeed+2) = cMime;
	   strcat(mMLWordEnc, (pMimeed+2));
	   strcpy(mMLWord, mMLWordEnc);
	 }
   } 
#else
   LineConv(mMLWordEnc, sizeof(mMLWordEnc), mMLWord , mPackEnc);
   strcpy(mMLWord, mMLWordEnc);
#endif
#endif
   pMLEnd = &mMLWord[strlen(mMLWord)-1];
   strncpy(mtop, mDBSubject, 4);
   mtop[4] = '\x0';
   pMLCnt = strpbrk(mMLWord, "%");
   if (pMLCnt) {
     *pMLCnt = '\x0';
     nML = strlen(mMLWord) + atoi(pMLCnt+1);
     pMLEnd = strpbrk(pMLCnt+1, "d");
	 if (pMLEnd) {
	   pMLEnd++;
	   nML += strlen(pMLEnd);
	 }
	 pSubject = strstr(mDBSubject, mMLWord);
#ifdef UPDATE_20040318                // 連番指定で全角コードの場合置換え先を間違うことがある。
#ifdef UPDATE_20070927 // メーリングリストの付加ヘッダに連番指定がないとハングする
	 if (pSubject && pMLEnd)
#else
	 if (pSubject)
#endif
	 {
	   pMLNo = strstr(pSubject, pMLEnd);
	   if (!pMLNo ||                            // 末尾データが見つからない時又は
#ifdef UPDATE_20241115_ML // MLへの付加表題がMIMEエンコードで記述されていると、付加表題の付け替えに失敗する
		   pMLNo && pSubject+nML != pMLNo+strlen(pMLEnd)) // 末尾データ位置が一致しない場合
#else
		   pMLNo && pSubject+nML-strlen(pMLEnd) != pMLNo) // 末尾データ位置が一致しない場合
#endif
	   {
         nML = 0;
	     pSubject = NULL;
	   }
	 }
#endif
   } else {
     nML = strlen(mMLWord);
     pSubject = NULL;
   }
   bRe = FALSE;
   if (nML > 0) {
	 pSubject = strstr(mDBSubject, mMLWord);
   } 
#ifdef UPDATE_20071022B // メーリングリストの付加ヘッダ指定がないとループする
   else {
     pSubject = NULL;
   }
#endif
   if (pSubject)
   {
     *pSubject = '\x0';
     while(pSubject)
	 {
       pMLCnt = pSubject + nML;
       pSubject = strstr(pMLCnt, mMLWord);
#ifdef UPDATE_20040318                // 連番指定で全角コードの場合置換え先を間違うことがある。
#ifdef UPDATE_20070927 // メーリングリストの付加ヘッダに連番指定がないとハングする
	   if (pSubject && pMLEnd)
#else
	   if (pSubject)
#endif
	   {
	     pMLNo = strstr(pSubject, pMLEnd);
	     if (!pMLNo ||                            // 末尾データが見つからない時又は
	 	     pMLNo && pSubject+nML-strlen(pMLEnd) != pMLNo) { // 末尾データ位置が一致しない場合
	       pSubject = NULL;
		 }
	   }
#endif
       if (pSubject)
         *pSubject = '\x0';
       do {
	     pRe = strstr(pMLCnt, "RE:");
	     if (!pRe)
	       pRe = strstr(pMLCnt, "Re:");
	     if (!pRe)
	       pRe = strstr(pMLCnt, "re:");
	     if (!pRe)
	       pRe = strstr(pMLCnt, "rE:");
	     if (pRe) {
           pMLCnt = pRe + 3;
		 }
		 if (*pMLCnt == ' ')
		   pMLCnt++;
	   } while (pRe);
	   if (strlen(pMLCnt) && strcmp(pMLCnt," ") != 0) {
	     strncat(mDBSubject, pMLCnt, 128-strlen(mDBSubject));
	   }
	 }
   }
#ifdef UPDATE_20240228 // 付加表題もまとめてパックするオプション
   if (bMLtknInc && mMLtkn[0]) 
   {
	 CHAR mSBDSubject2[512];
	 strcpy(mSBDSubject2, mMLtkn);
	 strcat(mSBDSubject2, mDBSubject);
	 strcpy(mDBSubject, mSBDSubject2);
	 mSubject[0] = '\x0';
   } else
#endif
     strcpy(mSubject, mMLtkn);
   if (strlen(mPack)) {
#ifdef UPDATE_20080204 // 題名がUNICODE utf-7/utf-8でのメーリングリスト投稿で文字化けする
     if (!strnicmp(mPack, "=?utf-7", 7)) { 
	   SJIS2UTF7N(mDBSubject, mMime, sizeof(mMime));
	   strcpy(mDBSubject, mMime);
	 } else if (!strnicmp(mPack, "=?utf-8", 7)) { 
	   SJIS2UTF8N(mDBSubject, mMime, sizeof(mMime));
	   strcpy(mDBSubject, mMime);
	 }
#endif
     translateUue2Base64(mDBSubject, strlen(mDBSubject), mMime);
/*
printf("translateUue2Base64()\n");
printf("mDBSubject Len = %d \n", strlen(mDBSubject));
printf("mMime Len      = %d \n", strlen(mMime));
*/
#ifdef UPDATE_20070928 // メーリングリスト MIME-Qへの対応と付加へ空なら処理しない
#ifdef UPDATE_20080204 // 題名がUNICODE utf-7/utf-8でのメーリングリスト投稿で文字化けする
     if (!strnicmp(mPack, "=?utf-7", 7)) {
       strcat(mSubject, "=?utf-7?B?");
	 } else if (!strnicmp(mPack, "=?utf-8", 7)) { 
       strcat(mSubject, "=?utf-8?B?");
	 } else 
#endif
	 {
#ifdef UPDATE_20241111_ML // メーリングリストの題名挿入時にMIME指定毎に異なる文字コードが指定されていると文字化けする
       if (!strnicmp(mPack, "=?us-ascii?", 11)  || // utf-7以外は全部 utf8に置換え
           !strnicmp(mPack, "=?us?", 5)  || 
           !strnicmp(mPack, "=?cp367?", 8)  || 
           !strnicmp(mPack, "=?csascii?", 10) ||
		   !strnicmp(mPack, "=?euc-jp?", 8) ||
		   !strnicmp(mPack, "=?csshiftjis?", 13) ||
		   !strnicmp(mPack, "=?shift-jis?", 12) ||
		   !strnicmp(mPack, "=?shift_jis?", 12) )
	   {
         strcpy(mPack, "=?iso-2022-jp?B?");
	   }
#endif
#ifdef UPDATE_20151118A // メーリングリスト投稿時にSubjectヘッダがEUC_JPでパックされていると文字化けする。
	   if (pPack = strstr(mPack, "?Q?"))
	   {
		 *(pPack+1) = 'B';
	   }
       strcat(mSubject, mPack);
	   if (pPack)
	   {
		 *(pPack+1) = 'Q';
	   }
#else
       strcat(mSubject, "=?iso-2022-jp?B?");
#endif
	 }
#else
     strcat(mSubject, mPack); //"=?iso-2022-jp?B?");
#endif
     strcat(mSubject, mMime); 
     strcat(mSubject, "?=\r\n");
   } else {
     strcat(mSubject, mDBSubject);
     strcat(mSubject, "\r\n");
   }
   fputs(mSubject, Tempfp);
   strcpy(mSubject, mDBSubject);
   strcpy(mDBSubject, mMLtkn);
   strcat(mDBSubject, mSubject);
}

BOOL AttachFile(FILE *fp, char *mBound, char *mEnd, char *nToken, int nFile, char *mBound2, BOOL bHtml) {
  char *p;
  FILE *fbin;
  int  nbinLen;
  char line[80], newline[80];
  char binFile[128], *pFn;
  char mType[128], nb[8];
  char *pmime;
  BOOL sts = FALSE;
  int  i;

  int nf, nc;


  p = strstr(nToken,"File:");
  if (bDebug) printf("Stat Attach File(%d): %s\n", *p, nToken);
  if (p){
    strcpy(binFile, (const char *)(p+5));
	strtok(binFile,"\r\n");
	for (i = strlen(binFile); i > 0; i--) {
	  pFn = binFile + i;
	  if (*pFn == '\\' || *pFn == '/') {
		pFn++;
		break;
	  }
	}
    pmime = strstr(binFile,".");
    if (pmime)
      GetMimeType((LPCTSTR) pmime, (LPCTSTR) "Content Type", "application/octet-stream", (LPTSTR) mType, sizeof(mType));
    
    //if (!bHtml && mBound[0] != '\x0')
      //fprintf(fp, "--%s\n", mBound);

    nf = nFile % MAX_FILE;
    nc = nFile / MAX_FILE;
	if (bHtml && 
		(stricmp(pmime, ".htm") == 0 || stricmp(pmime, ".html") == 0 ) ) {
    //if (nf == 0) {
	  if (bHtml)
		if(mBound[0] != '\x0')
          fprintf(fp, "--%s\n", mBound);
		else
          fprintf(fp, "--%s\n", mBound2);
      fprintf(fp,"Content-Type: multipart/related;\n");
	  fprintf(fp,"\tboundary=\"_%03d%s\"\n\n", nc+1 ,mBound2);
      ///// 添付形式ヘッダ挿入 /////////////////////////////
	}
	if (bHtml) {
	  nb[0] = '\x0';
      sprintf(nb, "_%03d", nc+1);
      if (mBound[0] != '\x0')
        fprintf(fp, "--%s%s\n", nb, mBound);
      else
        fprintf(fp, "--%s%s\n", nb, mBound2);
	} else if (mBound[0] != '\x0')
        fprintf(fp, "--%s\n", mBound);

	if (stricmp(pmime, ".htm") == 0 || stricmp(pmime, ".html") == 0 )
      fprintf(fp, "Content-Type: %s;\n", mType);
	else
      fprintf(fp, "Content-Type: %s; name=\"%s\"\n", mType, pFn);
    fprintf(fp, "Content-Transfer-Encoding: base64\n");
    //fprintf(fp, "Content-Disposition: inline; filename=\"%s\"\n", pFn); //binFile);
	if (!strstr(mType, "text"))
      fprintf(fp, "Content-ID: <%s>\n", pFn);
    fprintf(fp, "\n");
    ///////////////////////////////////////////////////////
    fbin = fopen( binFile, "rb" );
	if (fbin) {
  	  nbinLen = fread(line, 1, 48, fbin);
      while(!feof(fbin)) {
		translateUue2Base64(line, nbinLen, newline);
		//strcat(newline,"\r\n");
		//send(sock, newline, strlen(newline), 0 );
		strcat(newline,"\n");
        fputs( newline, fp );
   	    nbinLen = fread(line, 1, 48, fbin);
	  }
      if (nbinLen) {
 	    translateUue2Base64(line, nbinLen, newline);
	    //strcat(newline,"\r\n");
 	    //send(sock, newline, strlen(newline), 0 );
		strcat(newline,"\n");
        fputs( newline, fp );
	  }
      fclose(fbin);
	  /*
	  if (strstr(mType, "html")) {
	    if (bDebug) printf("--%s_0001--\n",mBound);
	    if (bDebug) printf("\n");
	  }
	  */
	}
	sts = TRUE;
  }
  return sts;
}
