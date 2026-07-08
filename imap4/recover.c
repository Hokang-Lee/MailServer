////////////////////////////////////////////////////////////////////
// recover.c 
// Copyright(C) 2011 e-POST Inc.
// IMAPフォルダ内の連番構造で重複ファイル名が発生した場合の回復機能
////////////////////////////////////////////////////////////////////
#include "imap4.h"

//#ifdef UPDATE_20110202 //IMAPフォルダ内の連番構造で重複ファイル名が発生した場合の回復機能の追加
extern BOOL bServiceTerminating;
extern BOOL   bUIDRecover;
extern CHAR   mImapStartFile[];
#ifdef UPDATE_20110301B // サービス停止で残ったユーザ毎のロックファイルをサービス再起動後に一旦削除する対策
extern CHAR mImapStartFile[];
#endif
extern BOOL bDebug;
#ifdef UPDATE_20190510 // NTFS以外のファイルシステムからのファイル一覧取得可能にする対策（一覧ソート）を追加
extern BOOL   bOtherFS; // TRUE:対応する FALSE:対応しない（旧仕様）
#endif

unsigned int getLastNo(char *pFolder);
void recoveid(char *pFolder, unsigned int nLastNo);
void ignorecheck(char *pFolder, char *pFn, unsigned int *pnLastNo);

/*
int main(int argc, char* argv[])
{
  unsigned int nLastNo;

  printf("recoverid v1.00 Copyright(C) 2011 e-POST Inc.\n");
	if (argc == 2)
	{
	  nLastNo = getLastNo(argv[1]) + 1;
	  recoveid(argv[1], nLastNo);
	} else {
	  printf("recoveid <folder (full path)>\n");
	}
	return 0;
}
*/

unsigned int getLastNo(char *pFolder)
{
  unsigned int nNo = 0;
#ifdef UPDATE_20190510 // NTFS以外のファイルシステムからのファイル一覧取得可能にする対策（一覧ソート）を追加
  DWORD            nSearch, no;
  WIN32_FIND_DATA  *pfd;
#endif
  HANDLE           hf;
  WIN32_FIND_DATA  fd;
  CHAR mFn[MAX_PATH+1];
#ifdef UPDATE_20110302A // フラグを変更対象のファイル名の一覧表を作成後にリネーム処理し、"~SG"が残されないようにする対策。
    sprintf(mFn, "%s\\*-??????.MSG", pFolder);  // メッセージフォルダ取出し
#else
#ifdef UPDATE_20110203A //フラグ変更中に新着の検出が発生したとき最後のUIDを正しく取得できないタイミングがある不具合
    sprintf(mFn, "%s\\*-??????.?SG", pFolder);  // メッセージフォルダ取出し
#else
    sprintf(mFn, "%s\\*-??????.MSG", pFolder);  // メッセージフォルダ取出し
#endif
#endif
#ifdef UPDATE_20190510 // NTFS以外のファイルシステムからのファイル一覧取得可能にする対策（一覧ソート）を追加
    if (bOtherFS) // TRUE:対応する FALSE:対応しない（旧仕様）
    {
      pfd = FindFirstFileSort((LPCTSTR)mFn, &fd, &nSearch);
      no = 0;
    } else {
	  hf = FindFirstFile((LPCTSTR)mFn, &fd);
    }
    if (bOtherFS && pfd || !bOtherFS && hf != INVALID_HANDLE_VALUE)
    {
#else
    if ((hf = FindFirstFile((LPCTSTR)mFn, &fd)) != INVALID_HANDLE_VALUE)
	{
#endif
  	  do 
	  {
        nNo = atol(fd.cFileName);
#ifdef UPDATE_20190510 // NTFS以外のファイルシステムからのファイル一覧取得可能にする対策（一覧ソート）を追加
        if (bOtherFS) // TRUE:対応する FALSE:対応しない（旧仕様）
  	    {
	      no++;
	      if (no < nSearch) {
 	        FindNextFileSort(&fd, pfd+no);
	      }
	    }
      } while(bOtherFS ? (no < nSearch) : FindNextFile(hf, &fd) );
      if (bOtherFS)
        FindCloseSort(pfd);
      else
  	    FindClose(hf);
#else
	  } while (FindNextFile(hf, &fd));
	  FindClose(hf);
#endif
	}
	return nNo;
}

void recoveid(char *pFolder, unsigned int nLastNo)
{
  FILE *fp;
#ifdef UPDATE_20110203 // UID回復処理で該当フォルダにメールが無いときUID値が初期化されてしまう不具合
  unsigned int     nUID = 0;
#endif
  HANDLE           hf;
  WIN32_FIND_DATA  fd;
  CHAR mFn[MAX_PATH+1];
  CHAR mFn1[MAX_PATH+1];
  CHAR mFn2[MAX_PATH+1];
  CHAR mFnUID[MAX_PATH+1];

    sprintf(mFnUID, "%s\\uid", pFolder);  // メッセージフォルダ取出し
#ifdef UPDATE_20110203 // UID回復処理で該当フォルダにメールが無いときUID値が初期化されてしまう不具合
    if ((fp = fopen(mFnUID, "rt")))
	{
      fscanf(fp, "%lu", &nUID);
	  fclose(fp);
	}
#endif
#ifdef UPDATE_20110222 //UPDATE_20110202でパフォーマンス改善の対策。
    if (bUIDRecover)
#endif
	{
#ifdef UPDATE_20110302A // フラグを変更対象のファイル名の一覧表を作成後にリネーム処理し、"~SG"が残されないようにする対策。
      sprintf(mFn, "%s\\*-??????.MSG", pFolder);  // メッセージフォルダ取出し
#else
#ifdef UPDATE_20110203A //フラグ変更中に新着の検出が発生したとき最後のUIDを正しく取得できないタイミングがある不具合
      sprintf(mFn, "%s\\*-??????.?SG", pFolder);  // メッセージフォルダ取出し
#else
      sprintf(mFn, "%s\\*-??????.MSG", pFolder);  // メッセージフォルダ取出し
#endif
#endif
      if ((hf = FindFirstFile((LPCTSTR)mFn, &fd)) != INVALID_HANDLE_VALUE)
	  {
  	    do 
		{
          ignorecheck(pFolder, fd.cFileName, &nLastNo);
		} while (FindNextFile(hf, &fd));
	    FindClose(hf);
	  }
	}
#ifndef UPDATE_20110302A // フラグを変更対象のファイル名の一覧表を作成後にリネーム処理し、"~SG"が残されないようにする対策。
#ifdef UPDATE_20110203A //フラグ変更中に新着の検出が発生したとき最後のUIDを正しく取得できないタイミングがある不具合
      sprintf(mFn, "%s\\*-??????.~SG", pFolder);  // メッセージフォルダ取出し
      if ((hf = FindFirstFile((LPCTSTR)mFn, &fd)) != INVALID_HANDLE_VALUE)
	  {
  	    do 
		{
          sprintf(mFn1, "%s\\%s", pFolder, fd.cFileName);  // ".~SG"ファイル名
          sprintf(mFn2, mFn1);  
	      mFn2[strlen(mFn2)-3] = 'M'; // ".MSG"に回復
	      printf("recover %s -> %s\n", mFn1, mFn2);
#ifdef UPDATE_20110228 // サービス停止のタイミングでフラグ変更中のファイルの復旧対策
	      if (rename(mFn1, mFn2) != 0) // ファイルをリネーム
		  {
		    if (errno == EACCES ||
				errno == EEXIST)
			{
              unlink(mFn1); // 重複なら削除
			}
		  }
#else
	      rename(mFn1, mFn2); // ファイルをリネーム
#endif
		} while (FindNextFile(hf, &fd));
	    FindClose(hf);
	  }
#endif
#endif
	/// UIDを更新
#ifdef UPDATE_20110203 // UID回復処理で該当フォルダにメールが無いときUID値が初期化されてしまう不具合
	if (nLastNo > nUID)
#endif
	{
      sprintf(mFnUID, "%s\\uid", pFolder);  // メッセージフォルダ取出し
      if ((fp = fopen(mFnUID, "wt")))
	  {
        fprintf(fp, "%lu", nLastNo);
	    fclose(fp);
	  }
	}
}

void ignorecheck(char *pFolder, char *pFn, unsigned int *pnLastNo)
{
  int n = 0;
  FILE *fp;
  HANDLE           hf;
  WIN32_FIND_DATA  fd;
  CHAR mFn[MAX_PATH+1];
  CHAR mFn1[MAX_PATH+1];
  CHAR mFn2[MAX_PATH+1];
  CHAR mFn3[MAX_PATH+1];
  CHAR mFnUID[MAX_PATH+1];

    sprintf(mFnUID, "%s\\uid", pFolder);  // メッセージフォルダ取出し
    strcpy(mFn2, pFn);  // メッセージフォルダ取出し
    mFn2[10] = '\x0';
    sprintf(mFn, "%s\\%s-??????.MSG", pFolder, mFn2);  // メッセージフォルダ取出し
    if ((hf = FindFirstFile((LPCTSTR)mFn, &fd)) != INVALID_HANDLE_VALUE)
	{
  	  do 
	  {
		if (n > 0)
		{
          sprintf(mFn1, "%s\\%s", pFolder, fd.cFileName);  // 重複ファイル名
		  strcpy(mFn3, &fd.cFileName[11]);
          sprintf(mFn2, "%s\\%010lu-%s", pFolder, *pnLastNo, mFn3);  // 新しいファイル名生成
		  *pnLastNo += 1;
		  /*
		  if ((fp = fopen(mFnUID, "wt")))
		  {
            fprintf(fp, "%lu", *pnLastNo);
		    fclose(fp);
		  }
		  */
		  printf("recover %s -> %s\n", mFn1, mFn2);
		  rename(mFn1, mFn2); // ファイルをリネーム
#ifdef UPDATE_20110302 // リカバリ処理でファイル名が正しく変更されるまで待つ対策
		  while(!(fp = fopen(mFn2, "rt")))
		  {
#ifdef UPDATE_20110301 // STROEコマンドでフラグ変更時に拡張子"~SG"のファイルが残ってしまうタイミングが無いようにする対策
		    if (rename(mFn1, mFn2) != 0) // フラグ変更
			{
			  break;
			}
#endif
            if (bServiceTerminating)
  	          break;
	         _sleep(WAIT_TIME);
		  }
		  if (fp)
		  {
		    fclose(fp);
		  }
#endif
		}
        n++;
	  } while (FindNextFile(hf, &fd));
	  FindClose(hf);
    }
}
//#endif

#ifdef UPDATE_20110304 // v4.34(フラグ変更の為の拡張子変更)使用している場合のバージョン差替え時の拡張子修復対策。
void recover_Fast_SELECT(char *pFolder)
{
  FILE *fp;

  HANDLE             hF;
  WIN32_FIND_DATA    FD;
  CHAR   mFn[MAX_PATH+1];
  CHAR   mFn1[MAX_PATH+1];
  CHAR   mFn2[MAX_PATH+1];
  FILETIME ltu, lts;
  ULARGE_INTEGER *u1, *u2;
  int ns;

  sprintf(mFn, "%s\\rid", pFolder);  // メッセージフォルダ取出し
  if (!(fp = fopen(mFn, "rt")))
  {
#ifdef UPDATE_20110325A // 先頭カラムにチルダのあるメールファイルの回復処理
     sprintf(mFn, "%s\\~*-??????.MSG", pFolder);  // メッセージフォルダ取出し
     if ((hF = FindFirstFile((LPCTSTR)mFn, &FD)) != INVALID_HANDLE_VALUE)
	 {
  	   do 
	   {
         sprintf(mFn1, "%s\\%s", pFolder, FD.cFileName);  // ".~SG"ファイル名
         sprintf(mFn2, "%s\\%s", pFolder, &FD.cFileName[1]);  
	     //mFn2[strlen(mFn2)-3] = 'M'; // ".MSG"に回復
	     printf("recover %s -> %s\n", mFn1, mFn2);
	     if (rename(mFn1, mFn2) != 0)
		 {
		   if (errno == EACCES ||
		 	   errno == EEXIST)
		   {
              unlink(mFn2); // 重複なら削除
		   }
		 }
         while(!(fp = fopen(mFn2, "rt"))) 
		 { 
           if (bServiceTerminating)
  	         break;
	       _sleep(WAIT_TIME);
		   if (rename(mFn1, mFn2) != 0) // リトライ
		   {
		     break;
		   }
		 }
		 if (fp)
		 {
		   fclose(fp);
		 }
	   } while (FindNextFile(hF, &FD));
	   FindClose(hF);
	 }
#endif
     sprintf(mFn, "%s\\*-??????.~SG", pFolder);  // メッセージフォルダ取出し
     if ((hF = FindFirstFile((LPCTSTR)mFn, &FD)) != INVALID_HANDLE_VALUE)
	 {
  	   do 
	   {
         sprintf(mFn1, "%s\\%s", pFolder, FD.cFileName);  // ".~SG"ファイル名
         sprintf(mFn2, mFn1);  
	     mFn2[strlen(mFn2)-3] = 'M'; // ".MSG"に回復
	     printf("recover %s -> %s\n", mFn1, mFn2);
	     if (rename(mFn1, mFn2) != 0)
		 {
		   if (errno == EACCES ||
		 	   errno == EEXIST)
		   {
              unlink(mFn2); // 重複なら削除
		   }
		 }
         while(!(fp = fopen(mFn2, "rt"))) 
		 { 
           if (bServiceTerminating)
  	         break;
	       _sleep(WAIT_TIME);
		   if (rename(mFn1, mFn2) != 0) // リトライ
		   {
		     break;
		   }
		 }
		 if (fp)
		 {
		   fclose(fp);
		 }
	   } while (FindNextFile(hF, &FD));
	   FindClose(hF);
	 }
     sprintf(mFn, "%s\\rid", pFolder);  // メッセージフォルダ取出し
     fp = fopen(mFn, "wt"); // ファイルの作成（空でよい）
  }
  if (fp)
  {
    fclose(fp);
  }
}
#endif

#ifdef UPDATE_20110326 // サービス起動時最初のフォルダ選択について判定し最初ならリカバリー処理を実施する
BOOL Start_for_Fast_SELECT(char *pFolder)
{
	BOOL    bResult = FALSE;
    HANDLE  hTMOFile;
    HANDLE  hRIDFile;
    SYSTEMTIME ltime;
    FILETIME   lt, last;
    ULARGE_INTEGER *u1, *u2;
	CHAR   mFn[MAX_PATH+1];

	///////////////////////////
    u1 = (ULARGE_INTEGER *)&lt;
    u2 = (ULARGE_INTEGER *)&last;
	lt.dwHighDateTime = lt.dwLowDateTime = 0;
	last.dwHighDateTime = last.dwLowDateTime = 0;
    hTMOFile = CreateFile( (LPCTSTR)mImapStartFile,
                        GENERIC_READ,
                        0,
                        NULL,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL, 
                        NULL);
     if (hTMOFile != INVALID_HANDLE_VALUE) 
	 {
       GetFileTime(hTMOFile, NULL, &lt, NULL); // 最終アクセス日時
       CloseHandle(hTMOFile);
       //u1->QuadPart /= (__int64)10000000; // １秒単位に
       ///////////////////////////
       sprintf(mFn, "%s\\rid", pFolder);  // メッセージフォルダ取出し
       hRIDFile = CreateFile( (LPCTSTR)mFn,
                        GENERIC_READ,
                        0,
                        NULL,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL, 
                        NULL);
       if (hRIDFile != INVALID_HANDLE_VALUE) 
	   {
         GetFileTime(hRIDFile, NULL, &last, NULL); // 最終アクセス日時
	     CloseHandle(hRIDFile);
         //u2->QuadPart /= (__int64)10000000; // １秒単位に
	   }
	 }

	 if (u1->QuadPart >= u2->QuadPart)
	 {
	   bResult = TRUE;
	   _unlink(mFn); // ridファイルの削除
	 }

    if (bDebug) printf("Start_for_Fast_SELECT() = %s\n", bResult ? "Start" : "Skip");
	return bResult;
}
#endif

#ifdef  UPDATE_20110404B // サービス再起動前のロックファイルを削除する
BOOL Start_for_Fast_LOCKFILE(char *pFn)
{
	BOOL    bResult = FALSE;
    HANDLE  hTMOFile;
    HANDLE  hRIDFile;
    SYSTEMTIME ltime;
    FILETIME   lt, last;
    ULARGE_INTEGER *u1, *u2;
	CHAR   mFn[MAX_PATH+1];

	///////////////////////////
    u1 = (ULARGE_INTEGER *)&lt;
    u2 = (ULARGE_INTEGER *)&last;
	lt.dwHighDateTime = lt.dwLowDateTime = 0;
	last.dwHighDateTime = last.dwLowDateTime = 0;
    hTMOFile = CreateFile( (LPCTSTR)mImapStartFile,
                        GENERIC_READ,
                        0,
                        NULL,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL, 
                        NULL);
     if (hTMOFile != INVALID_HANDLE_VALUE) 
	 {
       GetFileTime(hTMOFile, NULL, &lt, NULL); // 最終アクセス日時
       CloseHandle(hTMOFile);
       //u1->QuadPart /= (__int64)10000000; // １秒単位に
       ///////////////////////////
       //sprintf(mFn, "%s\\%s", pFolder, IMAP4_LOCKFILE );  // メッセージフォルダ取出し
       hRIDFile = CreateFile( (LPCTSTR)pFn,
                        GENERIC_READ,
                        0,
                        NULL,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL, 
                        NULL);
       if (hRIDFile != INVALID_HANDLE_VALUE) 
	   {
         GetFileTime(hRIDFile, NULL, &last, NULL); // 最終アクセス日時
	     CloseHandle(hRIDFile);
         //u2->QuadPart /= (__int64)10000000; // １秒単位に
	   }
	 }

	 if (u1->QuadPart >= u2->QuadPart)
	 {
	   bResult = TRUE;
	   _unlink(pFn); // ridファイルの削除
	 }

    if (bDebug) printf("Start_for_Fast_SELECT() = %s\n", bResult ? "Start" : "Skip");
	return bResult;
}
#endif