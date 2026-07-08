////////////////////////////////////////////////////////////
// Cluster.c Copyright K.kawakami
////////////////////////////////////////////////////////////
#include "smtp.h"
#include <windows.h>
#include <stdio.h>
#include "profile.h"

#ifdef CLUSTERING
extern char   mMsgIDFN[];
extern char   mMsgIDFNLock[];
extern BOOL   bServiceTerminating;
extern char   mMailSpoolDir[];

////////////////////////////////////////////////////////////////
//  メッセージＩＤカウンタ値を任意のファイルから読込み(クラスタ対応)
//  他のマシンとファイル共有にする。
////////////////////////////////////////////////////////////////
DWORD ReadLastMsgIdForAscii(HANDLE *hFile, DWORD nMsgId ) {
  FILE *fp;
  DWORD n;
  char  mWork[256];
  WIN32_FIND_DATA    FD;
   
  if (mMsgIDFN[0]) {
     sprintf(mWork, "%s*", mMailSpoolDir);
     *hFile = FindFirstFile(mWork, &FD);
     if (*hFile == INVALID_HANDLE_VALUE) // フォルダが無い＝存在しない
       return -1;
     FindClose( *hFile ); 

     while ((*hFile = CreateFile((LPCTSTR)mMsgIDFNLock,
                        GENERIC_WRITE,
                        0,   // 排他アクセス = 0
                        NULL,
                        CREATE_ALWAYS,
                        FILE_ATTRIBUTE_NORMAL, 
                        NULL)) == INVALID_HANDLE_VALUE) {
      if (bServiceTerminating)
	    break;
	   _sleep(WAIT_TIME);
	} 
#ifdef UPDATE_20051122 // WINDOWS以外のOS メッセージＩＤが更新されないタイミングの対策
	while (!( fp = fopen(mMsgIDFN, "r+t"))) {
      if (bServiceTerminating)
	    break;
	   _sleep(WAIT_TIME);
	}
    fscanf(fp, "%ul" , &n);
    fclose(fp);
    return n;
#else
    if (( fp = fopen(mMsgIDFN, "rt"))) {
      fscanf(fp, "%lu" , &n);
	  fclose(fp);
	  if (n > nMsgId)
	    return n;
	  else
	    return nMsgId;
	} else {
	  return nMsgId;
	}
#endif
  } else {
	return nMsgId;
  }
}

////////////////////////////////////////////////////////////////
//  メッセージＩＤカウンタ値を任意のファイルに保存(クラスタ対応)
//  他のマシンとファイル共有にする。
////////////////////////////////////////////////////////////////
void WriteLastMsgIdForAscii(HANDLE *hFile, DWORD nMsgId){
  FILE *fp;

  if (mMsgIDFN[0]) {
    if (( fp = fopen(mMsgIDFN, "wt"))) {
      fprintf(fp, "%lu" , nMsgId);
	  fclose(fp);
	}
    if (*hFile)
      CloseHandle(*hFile);
  }
}

#endif
