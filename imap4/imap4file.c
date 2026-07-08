//+---------------------------------------------------------------------------
//  File:       imap4file.c
//----------------------------------------------------------------------------

#include "imap4.h"
#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#pragma hdrstop

extern BOOL  bServiceTerminating;
extern BOOL  bDebug;
extern char  mMailExtension[];
extern CHAR  mMailBoxDir[];
extern DWORD nTMOut;
#ifdef UPDATE_20140522 // IMAPでフォルダを選択した時、排他処理フラグが同期されずに以降の処理をロックさせてしまう場合がある
extern DWORD   nLockTMOut;
#endif
extern char  mPasswordFile[];
//extern BOOL    bInboxEnc;
#ifdef UPDATE_20090322 // ログイン時のロックファイルのクリティカルセクション化
extern CRITICAL_SECTION g_csLogin;
#endif
#ifdef UPDATE_20110301B // サービス停止で残ったユーザ毎のロックファイルをサービス再起動後に一旦削除する対策
extern 	CHAR mImapStartFile[];
#endif
#ifdef UPDATE_20210224 // ADアカウントまたは、WINDOWSアカウント運用時のAUTHENTICATE LOGIN/PLAINでのパスワードファイルの参照を無視する
extern BOOL  bSimpleAuth;
#endif


void GetMailBoxFolder(char *user, char *pMailBox);
void GetDomainFromIP(char *myaddr, char *mydomain, DWORD nSize);

#ifdef UPDATE_20241209 // 外部アカウントでのXOAUTH2での認証方式用にユーザ別認証コード保管ファイル(oauth2.dat)を各アカウントフォルダに設定可能にした
char * GetXOAUTH2Path(char *pDir, char *pUser, char *pFn)
{
  char c, *p, *q;
  char Path[MAX_PATH];

    if (p = strchr(pUser, '@')) // ドメインの区切り
	{
	   *p = '\x0';
#ifdef WINDOWS
       sprintf(Path, "%soauth2\\%s\\%s\\%s", pDir, p+1, pUser, pFn);
#else
       sprintf(Path, "%soauth2/%s/%s/%s", pDir, p+1, pUser);
#endif
	   *p = '@';
	} else {
#ifdef WINDOWS
       sprintf(Path, "%soauth2\\%s\\%s", pDir, pUser, pFn);
#else
       sprintf(Path, "%soauth2/%s/%s", pDir, pUser);
#endif
	}
	if (p = strstr(Path, "oauth2"))
	{
	  while ((q = strchr(p, '\\')) || (q = strchr(p, '/')) )
	  {
		 c = *q;
		 *q = '\x0';
		 _mkdir(Path);
		 *q = c;
		 p = q+1;
	  }
	}
	return Path;
}
#endif

void GetBaseDirectory(CHAR *BaseDir, char *mMBdir, char *muser, char *myaddr) {
  char mTempDir[MAX_PATH];
  char mMBoxDir[MAX_PATH];

#ifdef V3
  char mduser[256];
  char mydomain[256];
    // ドメイン区切りに@と%で可能に
    if (!strstr(muser, "@") && !strstr(muser, "%"))  // ドメイン指定をしていない場合。
	  GetDomainFromIP(myaddr, mydomain, sizeof(mydomain));
#endif
	mTempDir[0] = '\x0';
	strcpy(mMBoxDir, mMBdir);
#ifdef V3
    // ドメイン区切りに@と%で可能に
	memset(mduser, 0, sizeof(mduser)); // ０クリア必須：削除するとユーザーチェックでハングする
    if (!strstr(muser, "@") && !strstr(muser, "%"))
	  sprintf(mduser,"%s@%s", muser, mydomain);
	else
	  strcpy(mduser, muser);
	GetMailBoxFolder(mduser, BaseDir);
#else
	GetMailBoxFolder(muser, BaseDir);
#endif
}

BOOL GetPasswordFile(char  *pDir, char *pUser, char *mpass) {
  FILE   *fp;
  CHAR   Path[MAX_PATH];
  BOOL   bsts = FALSE;
  char   mPwd[256];

    //sprintf(Path, "%sapop.dat", pDir);
    sprintf(Path, "%s%s", pDir, mPasswordFile);
    if (bDebug) printf("Authentication file name = %s\n", Path);
    fp = fopen(Path, "rt");
	if (fp) {
      fgets(mpass, _MAX_PATH, fp);
	  strtok(mpass, "\n");
	  fclose(fp);
      SPA_Decode(mpass, mPwd); // 2002.08.30
	  strcpy(mpass, mPwd);     // 2002.08.30
	  bsts = TRUE;
      if (bDebug) printf(" -> found Authentication file name (mailbox)\n");
	}
#ifdef UPDATE_20210224 // ADアカウントまたは、WINDOWSアカウント運用時のAUTHENTICATE LOGIN/PLAINでのパスワードファイルの参照を無視する
    return bsts || bSimpleAuth;
#else
    return bsts;
#endif

}

HANDLE LockMailDirectory(PCHAR  pszPath, PCHAR pszMyaddr)
{
    CHAR   BaseDirectory[MAX_PATH];
    CHAR   PathToOpen[MAX_PATH];
    HANDLE  hLockFile;
    HANDLE hTMOFile;
    SYSTEMTIME ltime;
    FILETIME   lt, last;
    ULARGE_INTEGER *u1, *u2;
#ifdef UPDATE_20110404A // 存在しないフォルダを選択すると排他処理で無限ループする不具合
    HANDLE     hd;
    WIN32_FIND_DATA  FD;
#endif
    // Build lock file name
	GetBaseDirectory(BaseDirectory, mMailBoxDir, pszPath, pszMyaddr);
#ifdef UPDATE_20110404A // 存在しないフォルダを選択すると排他処理で無限ループする不具合
    if ((hd = FindFirstFile((LPCTSTR)BaseDirectory, &FD)) == INVALID_HANDLE_VALUE)
	{
  	  return TRUE; // フォルダが無いならフラグは無視
	}
    FindClose(hd);
#endif
	sprintf(PathToOpen, "%s%s", BaseDirectory, IMAP4_LOCKFILE);
    if (bDebug) printf("LockMailDirectory = [%s]\n", PathToOpen);
#ifdef UPDATE_20090322 // ログイン時のロックファイルのクリティカルセクション化
    EnterCriticalSection(&g_csLogin);
#endif
#ifdef  UPDATE_20110404B // サービス再起動前のロックファイルを削除する
    Start_for_Fast_LOCKFILE(PathToOpen);
#endif
	// Mail Box Access Timeout check
   hTMOFile = CreateFile( (LPCTSTR)PathToOpen,
                        GENERIC_READ,
                        0,
                        NULL,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL, 
                        NULL);
   if (hTMOFile != INVALID_HANDLE_VALUE) {
     u1 = (ULARGE_INTEGER *)&lt;
     u2 = (ULARGE_INTEGER *)&last;
     GetSystemTime(&ltime);
     SystemTimeToFileTime(&ltime, &lt);
     GetFileTime(hTMOFile, NULL, &last, NULL); // 最終アクセス日時
     CloseHandle(hTMOFile);
     u1->QuadPart /= (__int64)10000000; // １秒単位に
     u2->QuadPart /= (__int64)10000000; // １秒単位に
     if ((__int64)u1->QuadPart > (__int64)(u2->QuadPart + nTMOut)) {
       if (bDebug) printf("Locked TimeOut. UnLockMailDirectory = [%s]\n", PathToOpen);
 	   if (bDebug) printf("Delete locked file\n");
#ifdef UPDATE_20090323 // ログイン時のロックファイルの削除で完全に削除されるまでウェイトする
	   while(!DeleteFile(PathToOpen))
	   {
	     if (GetLastError() == ERROR_FILE_NOT_FOUND)
		 {
	       break;
		 }
	     _sleep(100);
	   }
#else
#ifdef UPDATE_20110301C // ロックファイルの開放が完全に確認できるまで待つ対策
	   while(DeleteFile(PathToOpen)) 
	   {
	     if (GetLastError() == ERROR_FILE_NOT_FOUND)
		 {
	       break;
		 }
	     _sleep(100);
	   }
#else
       DeleteFile(PathToOpen);
#endif
#endif

	 } else {
#ifdef UPDATE_20110301B // サービス停止で残ったユーザ毎のロックファイルをサービス再起動後に一旦削除する対策
       hTMOFile = CreateFile( (LPCTSTR)mImapStartFile,
                        GENERIC_READ,
                        0,
                        NULL,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL, 
                        NULL);
       if (hTMOFile != INVALID_HANDLE_VALUE) {
         GetFileTime(hTMOFile, NULL, &lt, NULL); // 最終アクセス日時
         CloseHandle(hTMOFile);
         u1->QuadPart /= (__int64)10000000; // １秒単位に
         if ((__int64)u1->QuadPart >= (__int64)(u2->QuadPart)) 
		 {
           if (bDebug) printf("Locked TimeOut. UnLockMailDirectory = [%s]\n", PathToOpen);
 	       if (bDebug) printf("Delete locked file\n");
#ifdef UPDATE_20090323 // ログイン時のロックファイルの削除で完全に削除されるまでウェイトする
	       while(!DeleteFile(PathToOpen))
		   {
	         if (GetLastError() == ERROR_FILE_NOT_FOUND)
			 {
	           break;
			 }
	         _sleep(100);
		   }
#else
#ifdef UPDATE_20110301C // ロックファイルの開放が完全に確認できるまで待つ対策
	       while(DeleteFile(PathToOpen)) 
		   {
	         if (GetLastError() == ERROR_FILE_NOT_FOUND)
			 {
	           break;
			 }
	         _sleep(100);
		   }
#else
           DeleteFile(PathToOpen);
#endif 
#endif
		 } else 
#endif
		 {
           if (bDebug) printf("Locked Now. UnLockMailDirectory = [%s]\n", PathToOpen);
		 }
	   }
	 }

   }
    // Attempt to create the file.  This will fail if it already exists,
    // which would indicate that another thread/client is accessing this
    // mailbox.
    hLockFile = CreateFile( (LPCTSTR)PathToOpen,
                            GENERIC_READ | GENERIC_WRITE,
                            0,
                            NULL,
                            CREATE_NEW,
                            FILE_ATTRIBUTE_NORMAL, // | FILE_FLAG_DELETE_ON_CLOSE,
                            NULL);
    if (hLockFile != INVALID_HANDLE_VALUE) {
	  CloseHandle(hLockFile);
	}
    // Return a handle, or NULL if failed
    if (bDebug) printf("LockMailDirectory() Stats = [%ld]\n", hLockFile != INVALID_HANDLE_VALUE ? hLockFile : NULL);
#ifdef UPDATE_20090322 // ログイン時のロックファイルのクリティカルセクション化
    LeaveCriticalSection(&g_csLogin);
#endif
    return(hLockFile != INVALID_HANDLE_VALUE ? hLockFile : NULL);

}

BOOL UnLockMailDirectory(PCHAR  pszPath, PCHAR pszMyaddr)
{
    CHAR   BaseDirectory[MAX_PATH];
    CHAR   PathToOpen[MAX_PATH];
#ifdef UPDATE_20110404A // 存在しないフォルダを選択すると排他処理で無限ループする不具合
    HANDLE     hd;
    WIN32_FIND_DATA  FD;
#endif
    // Build lock file name
	GetBaseDirectory(BaseDirectory, mMailBoxDir, pszPath, pszMyaddr);
#ifdef UPDATE_20110404A // 存在しないフォルダを選択すると排他処理で無限ループする不具合
    if ((hd = FindFirstFile((LPCTSTR)BaseDirectory, &FD)) == INVALID_HANDLE_VALUE)
	{
  	  return TRUE; // フォルダが無いならフラグは無視
	}
    FindClose(hd);
#endif
	sprintf(PathToOpen, "%s%s", BaseDirectory, IMAP4_LOCKFILE);
    if (bDebug) printf("UnLockMailDirectory = [%s]\n", PathToOpen);
#ifdef UPDATE_20090322 // ログイン時のロックファイルのクリティカルセクション化
    EnterCriticalSection(&g_csLogin);
#endif
#ifdef UPDATE_20090323 // ログイン時のロックファイルの削除で完全に削除されるまでウェイトする
	while(!DeleteFile(PathToOpen)) 
	{
	  if (GetLastError() == ERROR_FILE_NOT_FOUND)
	  {
	    break;
	  }
	   _sleep(100);
	}
#else
#ifdef UPDATE_20110301C // ロックファイルの開放が完全に確認できるまで待つ対策
	while(DeleteFile(PathToOpen)) 
	{
	  if (GetLastError() == ERROR_FILE_NOT_FOUND)
	  {
	    break;
	  }
	  _sleep(100);
	}
#else
    DeleteFile(PathToOpen);
#endif
#endif
#ifdef UPDATE_20090322 // ログイン時のロックファイルのクリティカルセクション化
    LeaveCriticalSection(&g_csLogin);
#endif
   return TRUE;
}

#ifdef UPDATE_20110404 // 選択フォルダ単位で排他する対策
HANDLE LockMailSelectDirectory(PCHAR  pszPath)
{
    CHAR   BaseDirectory[MAX_PATH];
    CHAR   PathToOpen[MAX_PATH];
    HANDLE  hLockFile;
    HANDLE hTMOFile;
    SYSTEMTIME ltime;
    FILETIME   lt, last;
    ULARGE_INTEGER *u1, *u2;
#ifdef UPDATE_20110404A // 存在しないフォルダを選択すると排他処理で無限ループする不具合
    HANDLE     hd;
    WIN32_FIND_DATA  FD;
#endif
#ifdef UPDATE_20140522 // IMAPでフォルダを選択した時、排他処理フラグが同期されずに以降の処理をロックさせてしまう場合がある
	FILE *fp;
#endif

#ifdef UPDATE_20110404A // 存在しないフォルダを選択すると排他処理で無限ループする不具合
    if ((hd = FindFirstFile((LPCTSTR)pszPath, &FD)) == INVALID_HANDLE_VALUE)
	{
  	  return TRUE; // フォルダが無いならフラグは無視
	}
    FindClose(hd);
#endif
    // Build lock file name
	//GetBaseDirectory(BaseDirectory, mMailBoxDir, pszPath, pszMyaddr);
	sprintf(PathToOpen, "%s\\%s", pszPath, IMAP4_LOCKFILE);
    if (bDebug) printf("LockMailSelectDirectory = [%s]\n", PathToOpen);
#ifdef UPDATE_20090322 // ログイン時のロックファイルのクリティカルセクション化
    EnterCriticalSection(&g_csLogin);
#endif
//Sleep(6000);
#ifdef  UPDATE_20110404B // サービス再起動前のロックファイルを削除する
    Start_for_Fast_LOCKFILE(PathToOpen);
#endif
	// Mail Box Access Timeout check
   hTMOFile = CreateFile( (LPCTSTR)PathToOpen,
                        GENERIC_READ,
                        0,
                        NULL,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL, 
                        NULL);
   if (hTMOFile != INVALID_HANDLE_VALUE) {
     u1 = (ULARGE_INTEGER *)&lt;
     u2 = (ULARGE_INTEGER *)&last;
     GetSystemTime(&ltime);
     SystemTimeToFileTime(&ltime, &lt);
     GetFileTime(hTMOFile, NULL, &last, NULL); // 最終アクセス日時
     CloseHandle(hTMOFile);
     u1->QuadPart /= (__int64)10000000; // １秒単位に
     u2->QuadPart /= (__int64)10000000; // １秒単位に
#ifdef UPDATE_20140522 // IMAPでフォルダを選択した時、排他処理フラグが同期されずに以降の処理をロックさせてしまう場合がある
     if ((__int64)u1->QuadPart > (__int64)(u2->QuadPart + nLockTMOut)) 
#else
     if ((__int64)u1->QuadPart > (__int64)(u2->QuadPart + nTMOut)) 
#endif
	 {
       if (bDebug) printf("Locked TimeOut. UnLockMailSelectDirectory = [%s]\n", PathToOpen);
 	   if (bDebug) printf("Delete locked file\n");
#ifdef UPDATE_20090323 // ログイン時のロックファイルの削除で完全に削除されるまでウェイトする
	   while(!DeleteFile(PathToOpen))
	   {
	     if (GetLastError() == ERROR_FILE_NOT_FOUND)
		 {
	       break;
		 }
	     _sleep(100);
	   }
#else
#ifdef UPDATE_20110301C // ロックファイルの開放が完全に確認できるまで待つ対策
	   while(DeleteFile(PathToOpen)) 
	   {
	     if (GetLastError() == ERROR_FILE_NOT_FOUND)
		 {
	       break;
		 }
	     _sleep(100);
	   }
#else
       DeleteFile(PathToOpen);
#endif
#endif

	 } else {
#ifdef UPDATE_20110301B // サービス停止で残ったユーザ毎のロックファイルをサービス再起動後に一旦削除する対策
       hTMOFile = CreateFile( (LPCTSTR)mImapStartFile,
                        GENERIC_READ,
                        0,
                        NULL,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL, 
                        NULL);
       if (hTMOFile != INVALID_HANDLE_VALUE) {
         GetFileTime(hTMOFile, NULL, &lt, NULL); // 最終アクセス日時
         CloseHandle(hTMOFile);
         u1->QuadPart /= (__int64)10000000; // １秒単位に
         if ((__int64)u1->QuadPart >= (__int64)(u2->QuadPart)) 
		 {
           if (bDebug) printf("Locked TimeOut. UnLockMailSelectDirectory = [%s]\n", PathToOpen);
 	       if (bDebug) printf("Delete locked file\n");
#ifdef UPDATE_20090323 // ログイン時のロックファイルの削除で完全に削除されるまでウェイトする
	       while(!DeleteFile(PathToOpen))
		   {
	         if (GetLastError() == ERROR_FILE_NOT_FOUND)
			 {
	           break;
			 }
	         _sleep(100);
		   }
#else
#ifdef UPDATE_20110301C // ロックファイルの開放が完全に確認できるまで待つ対策
	       while(DeleteFile(PathToOpen)) 
		   {
	         if (GetLastError() == ERROR_FILE_NOT_FOUND)
			 {
	           break;
			 }
	         _sleep(100);
		   }
#else
           DeleteFile(PathToOpen);
#endif 
#endif
		 } else 
#endif
		 {
           if (bDebug) printf("Locked Now. UnLockMailDirectory = [%s]\n", PathToOpen);
		 }
	   }
	 }

   }
    // Attempt to create the file.  This will fail if it already exists,
    // which would indicate that another thread/client is accessing this
    // mailbox.
    hLockFile = CreateFile( (LPCTSTR)PathToOpen,
                            GENERIC_READ | GENERIC_WRITE,
                            0,
                            NULL,
                            CREATE_NEW,
                            FILE_ATTRIBUTE_NORMAL, // | FILE_FLAG_DELETE_ON_CLOSE,
                            NULL);
    if (hLockFile != INVALID_HANDLE_VALUE) {
	  CloseHandle(hLockFile);
#ifdef UPDATE_20140522 // IMAPでフォルダを選択した時、排他処理フラグが同期されずに以降の処理をロックさせてしまう場合がある
	  while(!(fp = fopen(PathToOpen, "rt"))) // ファイルがこの時点で
	  {
        if (bServiceTerminating)
  	      break;
	    _sleep(WAIT_TIME);
	  }
#ifdef UPDATE_20180827 ファイルオープン中でサービス停止時が行われたときにハングしない対策
	  if (fp)
#endif
	  {
	    fclose(fp);
	  }
#endif
	}
    // Return a handle, or NULL if failed
    if (bDebug) printf("LockMailSelectDirectory() Stats = [%ld]\n", hLockFile != INVALID_HANDLE_VALUE ? hLockFile : NULL);
#ifdef UPDATE_20090322 // ログイン時のロックファイルのクリティカルセクション化
    LeaveCriticalSection(&g_csLogin);
#endif
    return(hLockFile != INVALID_HANDLE_VALUE ? hLockFile : NULL);

}

BOOL UnLockMailSelectDirectory(PCHAR  pszPath)
{
    CHAR   BaseDirectory[MAX_PATH];
    CHAR   PathToOpen[MAX_PATH];
    // Build lock file name
	//GetBaseDirectory(BaseDirectory, mMailBoxDir, pszPath, pszMyaddr);
#ifdef UPDATE_20110404A // 存在しないフォルダを選択すると排他処理で無限ループする不具合
    HANDLE     hd;
    WIN32_FIND_DATA  FD;
#endif

#ifdef UPDATE_20110404A // 存在しないフォルダを選択すると排他処理で無限ループする不具合
    if ((hd = FindFirstFile((LPCTSTR)pszPath, &FD)) == INVALID_HANDLE_VALUE)
	{
  	  return TRUE; // フォルダが無いならフラグは無視
	}
    FindClose(hd);
#endif
	sprintf(PathToOpen, "%s\\%s", pszPath, IMAP4_LOCKFILE);
    if (bDebug) printf("UnLockMailSelectDirectory = [%s]\n", PathToOpen);
#ifdef UPDATE_20090322 // ログイン時のロックファイルのクリティカルセクション化
    EnterCriticalSection(&g_csLogin);
#endif
#ifdef UPDATE_20090323 // ログイン時のロックファイルの削除で完全に削除されるまでウェイトする
	while(!DeleteFile(PathToOpen)) 
	{
	  if (GetLastError() == ERROR_FILE_NOT_FOUND)
	  {
	    break;
	  }
	   _sleep(100);
	}
#else
#ifdef UPDATE_20110301C // ロックファイルの開放が完全に確認できるまで待つ対策
	while(DeleteFile(PathToOpen)) 
	{
	  if (GetLastError() == ERROR_FILE_NOT_FOUND)
	  {
	    break;
	  }
	  _sleep(100);
	}
#else
    DeleteFile(PathToOpen);
#endif
#endif
#ifdef UPDATE_20090322 // ログイン時のロックファイルのクリティカルセクション化
    LeaveCriticalSection(&g_csLogin);
#endif
   return TRUE;
}
#endif