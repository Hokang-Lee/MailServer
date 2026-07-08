////////////////////////////////////////////////////////////
// Base64 Convertor Copyright K.kawakami
////////////////////////////////////////////////////////////
#include "smtp.h"
#include <windows.h>
#include <stdio.h>
#include <string.h>

#ifdef ESMTP_ON

#define PROFILE_ROOT_TREE "%s\\"
#define MAX_VALUE_NAME    128
#define MAXMAILDATA       1024 

extern BOOL bDebug;
extern char mMailSpoolDir[];

#ifdef CLUSTERING
extern BOOL nClustering;
#endif

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

#ifdef UPDATE_20060116 // バッファオーバーフロー対策
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

void LineConv(char *dest, char *src, char *pack) {
  char *p, *p1, pe, *pk;
  //char *itmv[4], itm[4][MAXMAILDATA];
  char work[MAXMAILDATA];
  int  ln;

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
       //////コード変換 (EUC, JIS) -> S-JIS /////////////
       //strcpy(itm[0], "");         itmv[0] = itm[0];
       //strcpy(itm[1], "-s");       itmv[1] = itm[1];
       //strcpy(itm[2], work);       itmv[2] = itm[2];
       //QuickConvertBuff(3, (char **)&itmv, (char *)itm[2]);
       //////////////////////////////////////////////////
       //strcat(dest, itm[2]);
	   strcat(dest, work);
       if (p1) {
	     *(p1+2) = pe;
	     p1+=2;
	   }
	 } else {
	   strcat(dest, p1);
	   p1 += strlen(p1);
	 }
   }
}


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

	 } else {
#endif
     retCode = 
      RegOpenKeyEx(hKeyRoot,       // Key handle at root level.
                   (LPCTSTR)mkey,  // Path name of child key.
                   0,              // Reserved.
                   KEY_READ,       // Requesting read access.
                   &hKey);         // Address of key to be returned.
#ifdef REGTOFILE
	 }
#endif
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
	 } else {
#endif
    retCode =
	   RegQueryValueEx(hKey,            // handle of key to query 
                      (LPSTR)lpKeyName, // address of name of value to query 
                      0,                // reserved 
                      &dwType,  // address of buffer for value type 
                      (LPBYTE)lpReturnedString,  // address of data buffer 
                      &nSize  // address of data buffer size 
                      ); 
    RegCloseKey(hKey);
#ifdef REGTOFILE
	 }
#endif
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
  char *pMLCnt, *pMLEnd, *pSubject, *pRe;
  char mMime[256], mtop[5];
  int  nML;
  BOOL bRe;

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
   } else {
     nML = strlen(mMLWord);
     pSubject = NULL;
   }
   bRe = FALSE;
   if (nML > 0) {
	 pSubject = strstr(mDBSubject, mMLWord);
   }
   if (pSubject) {
     *pSubject = '\x0';
     while(pSubject) {
       pMLCnt = pSubject + nML;
       pSubject = strstr(pMLCnt, mMLWord);
       if (pSubject)
         *pSubject = '\x0';
       do {
	     pRe = strstr(pMLCnt, "RE: ");
	     if (!pRe)
	       pRe = strstr(pMLCnt, "Re: ");
	     if (!pRe)
	       pRe = strstr(pMLCnt, "re: ");
	     if (!pRe)
	       pRe = strstr(pMLCnt, "rE: ");
	     if (pRe) {
           pMLCnt = pRe + 4;
		 }
	   } while (pRe);
	   if (strlen(pMLCnt) && strcmp(pMLCnt," ") != 0) {
	     strncat(mDBSubject, pMLCnt, 128-strlen(mDBSubject));
	   }
	 }
   }
   strcpy(mSubject, mMLtkn);
   if (strlen(mPack)) {
     translateUue2Base64(mDBSubject, strlen(mDBSubject), mMime);
     strcat(mSubject, mPack); //"=?iso-2022-jp?B?");
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

/*
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
  printf("Stat Attach File(%d): %s\n", *p, nToken);
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
		strcat(newline,"\n");
        fputs( newline, fp );
   	    nbinLen = fread(line, 1, 48, fbin);
	  }
      if (nbinLen) {
 	    translateUue2Base64(line, nbinLen, newline);
		strcat(newline,"\n");
        fputs( newline, fp );
	  }
      fclose(fbin);
	}
	sts = TRUE;
  }
  return sts;
}
*/

#endif