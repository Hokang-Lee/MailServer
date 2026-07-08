////////////////////////////////////////////////////////////
// Quoted Printable Convertor Copyright K.kawakami
////////////////////////////////////////////////////////////
#include "smtp.h"
#include <windows.h>
#include <stdio.h>
#include <string.h>

void QuotedPrintableDecodeLine(unsigned char * pSrc, unsigned char * pDest) {
	unsigned char *o;
	unsigned char *p = pSrc;
	unsigned char *q = pDest;
	short    n[3];
	BOOL     bMach = FALSE;
	
	*pDest = '\x0';
	if (!p)
	  return;
	while (*p) {
	  //if (!strcmp((const char *)p, "?="))  // =? 終了
	    //break;
	  if (*p == '&' && *(p+1) == '#' && *(p+4) == ';' && strlen((const char *)p) > 4) { // &#??; のパターン
		n[0] = *(p+2) - 0x30;
		n[1] = *(p+3) - 0x30;
        p+=5;
        *q = (char)(n[0] * 10 + n[1]);
	  } else if (*p == '&' && *(p+1) == '#' && *(p+5) == ';' && strlen((const char *)p) > 5) { // &#???; のパターン
		n[0] = *(p+2) - 0x30;
		n[1] = *(p+3) - 0x30;
		n[2] = *(p+4) - 0x30;
        p+=6;
        *q = (char)(n[0] * 100 + n[1] * 10 + n[2]);
	  } else if (*p == '=') { // 8Bit
		p++;
		n[0] = n[1] = 0;
		if (*p >= 0x30 && *p <= 0x39) {
		  n[0] = *p - 0x30;
		  bMach = TRUE;
		} else if (*p >= 0x41 && *p <= 0x46) {
		  n[0] = *p - 0x37;
		  bMach = TRUE;
		} else if (*p >= 0x61 && *p <= 0x66) {
		  n[0] = *p - 0x57;
		  bMach = TRUE;
		} else {
		  p--;
		  bMach = FALSE;
		}
		if (bMach) {
          p++;
		  if (*p >= 0x30 && *p <= 0x39) {
		    n[1] = *p - 0x30;
		  } else if (*p >= 0x41 && *p <= 0x46) {
		    n[1] = *p - 0x37;
		  } else if (*p >= 0x61 && *p <= 0x66) {
		    n[1] = *p - 0x57;
		  } else {
            p -= 2;
		    bMach = FALSE;
            *q = *p;
	        p++;
		  }
		  if (bMach) {
            p++;
		    *q = (char)(n[0] * 16 + n[1]);
		  }
		} else {
          *q = *p;
	      p++;
		}
	  } else {         // ASCII
		if (*p == '_')
          *q = ' ';
		else
          *q = *p;
	    p++;
	  }
	  q++;
	}
	*q = '\x0';
#ifdef UPDATE_20151119 // Quoted-Printableのデコードで文字が欠ける場合がある
	if (strlen(pDest) >= 2)
	{
      if ((o = strstr((char *)(pDest+strlen(pDest)-2), "?="))) { // パックの終わりを除去
        *o = '\x0';
	  }
	} 
#else
    if ((o = strstr(pDest, "?="))) { // パックの終わりを除去
      *o = '\x0';
	}
#endif
}
