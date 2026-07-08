///////////////////////////////////////////////////////////
// レジストリ情報の管理をフォルダで処理
// Copyright K-TEC Corp. K.Kawakami
///////////////////////////////////////////////////////////
#include "imap4.h"
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <direct.h>

extern BOOL    bServiceTerminating;
extern int     nReadyDriveTime;
#ifdef UPDATE_20070521 // OSの予約語対策
extern CHAR   mReservedWords[];
#endif

#ifdef REGTOFILE
///////////////////////////////////////////////////////////
// DISKが使用可能になるまでウエイトとさせる
///////////////////////////////////////////////////////////
void ReadyDrive(char *pStr) {
  char *p, mDrive[256];
  HANDLE             hF;
  WIN32_FIND_DATA    FD;
  int i = 0;
	strcpy(mDrive, pStr);
	if (!strnicmp(mDrive, "\\\\", 2)) {
	  if ((p = strstr(&mDrive[2], "\\")))
	    strtok(p+1, "\\");
	} else {
	  strtok(mDrive, "\\");
	}
	strcat(mDrive, "\\*");
	while ((hF = FindFirstFile(mDrive, &FD)) == INVALID_HANDLE_VALUE) {
	  Sleep(1000*60);
	  i++;
	  if (i >= nReadyDriveTime)
        break;
	}
	if (hF)
      FindClose( hF ); 
}
#ifdef UPDATE_20070425 // MSCSのスタンバイ側に対応
///////////////////////////////////////////////////////////
// DISKが使用可能か確認
///////////////////////////////////////////////////////////
BOOL QueryDrive(char *pStr) {
  BOOL bSts = FALSE;
  char *p, mDrive[256], mTest[256], mSys[256];
  HANDLE             hF;
  WIN32_FIND_DATA    FD;
  FILE *fp;
  int i = 0;
	strcpy(mDrive, pStr);
	if (!strnicmp(mDrive, "\\\\", 2)) {
	  if ((p = strstr(&mDrive[2], "\\")))
	    strtok(p+1, "\\");
#ifdef UPDATE_20090903 // メール作業領域で環境変数%systemroot%があるとドライブチェックが正しく出来ない
#ifdef UPDATE_20091224 // 前バージョン環境変数%systemroot%の対策ロジックのミス
	} else if (mDrive[0] == '%') {
#else
	} if (mDrive[0] == '%') {
#endif
	  if ((p = strchr(&mDrive[1], '%')))
	  {
		*p = '\x0';
        strcpy(mSys, getenv(&mDrive[1]));
		strcpy(mDrive, mSys);
        strtok(mDrive, "\\");
	  } else {
	    mDrive[0] = '\x0';
	  }
#endif
	} else {
	  strtok(mDrive, "\\");
	}
	sprintf(mTest, "%s\\%s", mDrive, IMAP4_SERVICE);
	strcat(mDrive, "\\*");
	if ((hF = FindFirstFile(mDrive, &FD)) != INVALID_HANDLE_VALUE) {
	  if ((fp = fopen(mTest, "wt"))) {
		fclose(fp);
		_unlink(mTest);
        bSts = TRUE;
	  }
      FindClose( hF ); 
	}
	return bSts;
}
#endif UPDATE_20070425 // MSCSのスタンバイ側に対応
///////////////////////////////////////////////////////////
// レジストリ情報保管フォルダの作成
///////////////////////////////////////////////////////////
DWORD FileCreateKey(char *pKeyRoot, char *pKey) {
  char mKeyName[512];
  char *p;
#ifdef UPDATE_20070521 // 予約語対策
  char *q, mKey[512];
  BOOL bReservedWords = FALSE;

  strcpy(mKey, pKey);
  strtok(mKey, "@");
  if ((p = strrchr(mKey, '\\'))) {
	p++;
    if (ReservedWords(p)) {
	  if ((q = strstr(pKey, p))) {
	    strcpy(p, mReservedWords);
        strcat(mKey, q);
		bReservedWords = TRUE;
	  }
	}

  }
  if (bReservedWords)
    sprintf(mKeyName, "%s\\reg\\%s", pKeyRoot, mKey);
  else
#endif
  sprintf(mKeyName, "%s\\reg\\%s", pKeyRoot, pKey);
  p = strstr(mKeyName,":\\");
  if (p)
    p = strstr(p+2,"\\");
  else {
    if ((p = strstr(mKeyName,"\\\\")))
      if ((p = strstr(p+2,"\\")))
       p = strstr(p+1,"\\");
  }
  while(p) {
    *p = '\x0';
    _mkdir(mKeyName);         // 処理用フォルダ作成
    *p = '\\';
     p = strstr(p+1,"\\");
  }
  _mkdir(mKeyName);         // 処理用フォルダ作成

  return ERROR_SUCCESS;
}

///////////////////////////////////////////////////////////
// 定義ファイルの排他アクセスを可能にする
///////////////////////////////////////////////////////////
DWORD OpenKeyFile(char *pKeyRoot, char *pKey, char *pValue, HANDLE *hFile) {
  char mKeyLock[512];
  HANDLE             hF;
  WIN32_FIND_DATA    FD;
#ifdef UPDATE_20070521 // 予約語対策
  char *p, *q, mKey[512];
  BOOL bReservedWords = FALSE;

  strcpy(mKey, pKey);
  strtok(mKey, "@");
  if ((p = strrchr(mKey, '\\'))) {
	p++;
    if (ReservedWords(p)) {
	  if ((q = strstr(pKey, p))) {
	    strcpy(p, mReservedWords);
        strcat(mKey, q);
		bReservedWords = TRUE;
	  }
	}

  }
#endif

  // フォルダの存在を確認
#ifdef UPDATE_20070521 // 予約語対策
  if (bReservedWords)
    sprintf(mKeyLock, "%s\\reg\\%s*", pKeyRoot, mKey);
  else
#endif
  sprintf(mKeyLock, "%s\\reg\\%s*", pKeyRoot, pKey);
  hF = FindFirstFile(mKeyLock, &FD);
  if (hF == INVALID_HANDLE_VALUE) // フォルダが無い＝存在しない
    return -1;
  FindClose( hF ); 

#ifdef UPDATE_20070521 // 予約語対策
  if (bReservedWords)
    sprintf(mKeyLock, "%s\\reg\\%s%s.lck", pKeyRoot, mKey, pValue);
  else
#endif
  sprintf(mKeyLock, "%s\\reg\\%s%s.lck", pKeyRoot, pKey, pValue);
  while ((*hFile = CreateFile((LPCTSTR)mKeyLock,
                        GENERIC_WRITE,
                        0,   // 排他アクセス = 0
                        NULL,
                        CREATE_ALWAYS,
                        FILE_ATTRIBUTE_NORMAL, 
                        NULL)) == INVALID_HANDLE_VALUE) {
    if (bServiceTerminating) 
      return -1;
    _sleep(WAIT_TIME);
  } 
  return ERROR_SUCCESS;
}

DWORD KeyFileQueryValueEx(char *pKeyRoot, char *pKey, char *pValue, HANDLE hFile, DWORD dwType, LPBYTE pRet, DWORD *nSize) {
  DWORD nSts = -1;
  char mKeyName[512];
  FILE *fp;

    switch(dwType) {
       case REG_BINARY: memset(pRet, 0, *nSize); // クリア
                        sprintf(mKeyName, "%s\\reg\\%s%s.0", pKeyRoot, pKey, pValue);
                        if ((fp = fopen(mKeyName, "rb"))) {
                          *nSize = fread(pRet, sizeof(char), *nSize, fp);
                          fclose(fp);
                          nSts = ERROR_SUCCESS;
                        } else {
					      *nSize = 0;
                        }
		    break;
       case REG_DWORD: sprintf(mKeyName, "%s\\reg\\%s%s.1", pKeyRoot, pKey, pValue);
                       if ((fp = fopen(mKeyName, "rt"))) {
                          fscanf(fp, "%lu", pRet);
                          *nSize = sizeof(DWORD);
                          fclose(fp);
                          nSts = ERROR_SUCCESS;
                       } else {
					     *nSize = 0;
                       }
 	            break;
       case REG_NONE:  // 無視
		    break;
       default:     sprintf(mKeyName, "%s\\reg\\%s%s.2", pKeyRoot, pKey, pValue);
                    if ((fp = fopen(mKeyName, "rt"))) { // REG_SZ, REG_EXPAND_SZ, REG_MULTI_SZ, 
                      fgets((char *)pRet, *nSize, fp);
                      *nSize = strlen(pRet);
                      fclose(fp);
                      nSts = ERROR_SUCCESS;
                    } else {
				      *nSize = 0;
                    }
		    break;
      }
  
  return nSts;
}

DWORD KeyFileSetValueEx(char *pKeyRoot, char *pKey, char *pValue, HANDLE hFile, DWORD  dwType, LPBYTE pData, DWORD nSize)
{
  DWORD nSts = -1, *pn;
  char mKeyName[512];
  FILE *fp;

    switch(dwType) {
       case REG_BINARY: sprintf(mKeyName, "%s\\reg\\%s%s.0", pKeyRoot, pKey, pValue);
                        if ((fp = fopen(mKeyName, "wb"))) {
                          fwrite(pData, sizeof(char), nSize, fp);
                          fclose(fp);
                          nSts = ERROR_SUCCESS;
                        } 
		    break;
       case REG_DWORD: sprintf(mKeyName, "%s\\reg\\%s%s.1", pKeyRoot, pKey, pValue);
		               pn = pData;
                       if ((fp = fopen(mKeyName, "wt"))) {
                          fprintf(fp, "%lu", *pn);
                          fclose(fp);
                          nSts = ERROR_SUCCESS;
                       }
 	            break;
       case REG_NONE:  // 無視
		    break;
       default:     sprintf(mKeyName, "%s\\reg\\%s%s.2", pKeyRoot, pKey, pValue);
                    if ((fp = fopen(mKeyName, "wt"))) { // REG_SZ, REG_EXPAND_SZ, REG_MULTI_SZ, 
                      fputs(pData, fp);
                      fclose(fp);
                      nSts = ERROR_SUCCESS;
                    }
		    break;
      }
  
  return nSts;
}

DWORD KeyFileEnumValue(char *pKeyRoot, char *pKey, DWORD nIndex, char *pValue, DWORD  dwType, LPBYTE pRet, DWORD *nSize)
{
  DWORD nSts = -1;
  char mKeyName[512];
  DWORD n = 0;
  HANDLE             hF, hFile;
  WIN32_FIND_DATA    FD;
  BOOL bFile = TRUE;


  switch(dwType) {
       case REG_BINARY: sprintf(mKeyName, "%s\\reg\\%s*.0", pKeyRoot, pKey);
		    break;
       case REG_DWORD: sprintf(mKeyName, "%s\\reg\\%s*.1", pKeyRoot, pKey);
 	            break;
       case REG_NONE:  // 無視
		    break;
       default:     sprintf(mKeyName, "%s\\reg\\%s*.2", pKeyRoot, pKey);
		    break;
 }

 hF = FindFirstFile(mKeyName, &FD);
 if (hF != INVALID_HANDLE_VALUE) {
    bFile = TRUE;
    while (bFile) {
	  if (!(!stricmp(FD.cFileName, ".") ||
	        !stricmp(FD.cFileName, "..") ||
	        !stricmp(FD.cFileName, ".lck"))) {
        if (n == nIndex) {
		  strcpy(pValue, FD.cFileName);
		  *(pValue+strlen(pValue)-2) = '\x0';
          OpenKeyFile(pKeyRoot, pKey, pValue, &hFile);
          if (hFile) {
            nSts =  KeyFileQueryValueEx(pKeyRoot, pKey, pValue, hFile, dwType, pRet, nSize);
            CloseHandle(hFile);
		  }
          break; 
		}
        n++;
	  }
      bFile = FindNextFile( hF, &FD);
    }; 
    FindClose( hF ); 
  }

  return (bFile ? nSts : ERROR_NO_MORE_ITEMS);
}

DWORD KeyFileEnumKey(char *pKeyRoot, char *pKey, DWORD nIndex, char *pValue, DWORD *nSize)
{
  DWORD nSts = -1;
  char mKeyName[512];
  DWORD n = 0;
  HANDLE             hF;
  WIN32_FIND_DATA    FD;
  BOOL bFile = TRUE;


  sprintf(mKeyName, "%s\\reg\\%s*", pKeyRoot, pKey);

  if (pValue)
    *pValue = '\x0';
   hF = FindFirstFile(mKeyName, &FD);
   if (hF != INVALID_HANDLE_VALUE) {
      bFile = TRUE;
      while (bFile) {
		/// ディレクトリのみ表示
		if (FD.dwFileAttributes == FILE_ATTRIBUTE_DIRECTORY) {
	      if (!(!stricmp(FD.cFileName, ".") ||
	            !stricmp(FD.cFileName, ".."))) {
            if (n == nIndex) {
              if (pValue) {
                strcpy(pValue, FD.cFileName);
                *nSize = strlen(pValue);
                nSts = ERROR_SUCCESS;
			  }
              break; 
			}
            n++;
		  }
		}
        bFile = FindNextFile( hF, &FD);
	  }; 
      FindClose( hF ); 
   }
   return nSts;
}


DWORD KeyFileDeleteValue(char *pKeyRoot, char *pKey, char *pValue)
{
  DWORD nSts = -1;
  char mKeyName[512];

  sprintf(mKeyName, "%s\\reg\\%s%s.0", pKeyRoot, pKey, pValue);
  DeleteFile(mKeyName);
  sprintf(mKeyName, "%s\\reg\\%s%s.1", pKeyRoot, pKey, pValue);
  DeleteFile(mKeyName);
  sprintf(mKeyName, "%s\\reg\\%s%s.2", pKeyRoot, pKey, pValue);
  DeleteFile(mKeyName);

  return ERROR_SUCCESS;
}

#endif
