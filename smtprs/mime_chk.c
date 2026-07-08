////////////////////////////////////////////////////////////
// Check of MIME Data Copyright K.kawakami
////////////////////////////////////////////////////////////
#include "smtp.h"
#include <windows.h>
#include <stdio.h>
#include <string.h>

#ifdef THIRDPARTY
BOOL ThirdpartyProcess(char *pFn, char *pResult);
#ifdef UPDATE_20050329
BOOL ThirdpartyProcess_PASSIP(char *pAddr);
#endif
#endif
#ifdef ACCEPT_LOG_LEVEL3
void LEVEL_3_ACCEPTLOG(PCLIENT_CONTEXT lpClientContext, char *mess);
#endif

#ifdef V3
extern char  mMailSpoolDir[];
extern int   nVirusDoubtful;
extern DWORD nViursMailSzie;
extern char  mVirus2File[];
int Base64DecodeLine(char * pszLine, int nLineLen, unsigned char * uchBuffer, int nBufferLen);
void  SPA_Decode(char *pSrc, char *pDest);
///////////////////////////////////////////// 
// 指定ファイルの文字列に一致するか確認
///////////////////////////////////////////// 
BOOL VirusToken(char *mData) { 
  FILE  *fp;
  CHAR  mwork[512], mKey[256];

  if ((fp = fopen(mVirus2File, "rt"))) {  // 定義ファイルがあるとき
    fgets( mwork, sizeof(mwork), fp); // ヘッダー読み飛ばし VG:
	strcpy(mwork, "\xff");
    fgets( &mwork[1], sizeof(mwork)-1, fp);
	while(!feof(fp)) {
	  if (!(mwork[0] == '\r' ||
		    mwork[0] == '\n' ||
		    mwork[0] == '\t' )) {
		strtok(mwork, "\r\n");
        SPA_Decode(mwork, mKey); // 複号化
		if (!strnicmp(mData, mKey, strlen(mKey)))
		  return TRUE;
	  }
	  strcpy(mwork, "\xff");
      fgets( &mwork[1], sizeof(mwork)-1, fp);
	}
	fclose(fp);
  } else {                               // 定義ファイルがないとき
    if (!strnicmp(mData,"iphlpapi.dll", 12) ||
        !strnicmp(mData,"wininet.dll", 11) ||
		!strnicmp(mData,"ws2_32.dll", 10)  ||
		!strnicmp(mData,"wsock32.dll", 11) ||
        !strnicmp(mData,"mswsock.dll", 11) ||
        !strnicmp(mData,"winsock.dll", 11) ||
		!strnicmp(mData,"InternetGetConnect", 18) ||
		!strnicmp(mData,"CreateObject(", 13) )
       return TRUE;
  }
  return FALSE;
}

///////////////////////////////////////////// 
// 指定ファイルの文字列に一致するか確認
///////////////////////////////////////////// 
BOOL Check_Of_MIME(PSmtpContext pContext) {
  BOOL  bsts = TRUE, bHead = TRUE, bAttach = FALSE, bZero;
  BOOL  bDos, bDoubtful, bData, bBlank = FALSE;
  FILE  *fp, *fo;
  INT   nLen, i, n; 
  CHAR  *p, *q, mTemp[256], mwork[256], mVname[256], mToken[MAX_PATH];
  char  mLog[256];
  HANDLE          hFile;
  WIN32_FIND_DATA FindData ;
  DWORD nSizeL, nSizeH;
#ifdef V4
  BOOL  bScriptType, bBase64Data = FALSE;
#endif

#ifdef THIRDPARTY
#ifdef UPDATE_20040720_LOG
  sprintf(mLog, "pContext->RCPfp = %08x", pContext->RCPfp);
  LEVEL_3_ACCEPTLOG(pContext, mLog);
#endif
  strcpy(mTemp, pContext->mFnData);
#ifdef UPDATE_20050329
  if (!ThirdpartyProcess_PASSIP(pContext->PeerAddr)) // Checking the IP
#endif
    if (!(bsts = ThirdpartyProcess(mTemp, mVname))) // mVnameにウイルス名
	  return bsts;
#ifdef UPDATE_20040720_LOG
  sprintf(mLog, "pContext->RCPfp = %08x", pContext->RCPfp);
  LEVEL_3_ACCEPTLOG(pContext, mLog);
#endif
#endif
  if (!nVirusDoubtful)
	return bsts;

#ifdef UPDATE_20040720_LOG
  strcpy(mLog, "start Check_Of_MIME()");
  LEVEL_3_ACCEPTLOG(pContext, mLog);
#endif
  hFile = FindFirstFile((LPCTSTR)pContext->mFnData, &FindData);
  if (hFile != INVALID_HANDLE_VALUE) {
    nSizeH = FindData.nFileSizeHigh; 
	nSizeL = FindData.nFileSizeLow; 
    FindClose(hFile);
  }

  if (nViursMailSzie)                       // 上限サイズ指定があるなら、０の時は無制限。
    if (nSizeH || nSizeL > nViursMailSzie)  // ウイルスメールの上限サイズより大きい時はチェックしない。
	  return bsts;

  if ((fp = fopen(pContext->mFnData, "rt"))) {
    fgets( mwork, sizeof(mwork), fp);
	while(!feof(fp)) {
	  if (bHead) { // メールのヘッダ
		if (mwork[0] == '\r' || mwork[0] == '\n') {
		  bHead = FALSE; // ヘッダの終わり
#ifdef V4
		  bScriptType = FALSE;
#endif
		} else if (!strnicmp(mwork, "content-type:", 13)) {
		  p = &mwork[13];
		  while(*p == ' ' && *p != '\x0') {
			p++;
		  }
		  if (!strnicmp(p, "multipart", 9))
			bAttach = TRUE;
		}
	  } else {     // メールの本文
		if (!bAttach)  // 添付無しなら終了
		  break;
		bDos = bDoubtful = bZero = FALSE;
#ifdef V4
        if (!bScriptType) {
		  if (!strnicmp(mwork, "--", 2)) {  // 添付のヘッダ確認
			bBase64Data = FALSE;
            fgets(mwork, sizeof(mwork), fp);
            while (!(feof(fp) || mwork[0] == '\r' || mwork[0] == '\n')) {
		      if (!strnicmp(mwork, "content-transfer-encoding:", 26)) {
				p = &mwork[26];
				while(*p == ' ') 
				  p++;
				if (!strnicmp(p, "base64", 6)) {
				  bBase64Data = TRUE;
				} else {
                  fgets(mwork, sizeof(mwork), fp);
                  while (!(feof(fp)) && (mwork[0] == '\t' || mwork[0] == ' ') ) {
				    p = mwork;
				    while(*p == '\t' || *p == ' ') 
				      p++;
				    if (!strnicmp(p, "base64", 6))
				      bBase64Data = TRUE;
                    fgets(mwork, sizeof(mwork), fp);
				  }
				  continue;
				}
			  }
		      if (!strnicmp(mwork, "content-type:", 13)) {
			    if ((q = strrchr(mwork, '.'))) {
			      if (!strnicmp(q, ".htm", 4) ||
				      !strnicmp(q, ".hta", 4) ||
				      !strnicmp(q, ".vbs", 4) )
				    bScriptType = TRUE;
				} else {
                  fgets(mwork, sizeof(mwork), fp);
                  while (!(feof(fp)) && (mwork[0] == '\t' || mwork[0] == ' ') ) {
			        if ((q = strrchr(mwork, '.'))) {
			          if (!strnicmp(q, ".htm", 4) ||
				          !strnicmp(q, ".hta", 4) ||
					      !strnicmp(q, ".vbs", 4) ) {
				        bScriptType = TRUE;
					    break;
					  }
					}
                    fgets(mwork, sizeof(mwork), fp);
				  }
				  continue;
				}
			  }
              fgets(mwork, sizeof(mwork), fp);
			}
		  }
		}
		if (!strncmp(mwork, "TV", 2) ||  // DOSプログラム
			!strncmp(mwork, "UEsDB", 5))
		  bBase64Data = TRUE;
		if (bBlank && 
			 (bScriptType ||               // html形式 又は hta形式 又は vbs形式
			  !strncmp(mwork, "TV", 2) ||  // DOSプログラム
			  !strncmp(mwork, "UEsDB", 5) ) ) // ZIP形式
#else
		if (bBlank && 
			 (!strncmp(mwork, "TV", 2) ||  // DOSプログラム
			  !strncmp(mwork, "UEsDB", 5) ) ) // ZIP形式
#endif
		{
#ifdef UPDATE_20071204  // メッセージＩＤ採番処理を修正(Bym10id)
          sprintf(mTemp, "%stemp\\%s.obj", mMailSpoolDir, pContext->mMsgId);
#else
          sprintf(mTemp, "%stemp\\B%010lu.obj", mMailSpoolDir, pContext->nMsgId);
#endif
		  if ((fo = fopen(mTemp, "wt"))) {
	        while(!feof(fp)) {
		      if (mwork[0] == '\r' && mwork[0] == '\n' ||
			 	  !strncmp(mwork, "--", 2)) { // １添付ファイルの終了
			    break;
			  }
			  strtok(mwork, "\r\n");
#ifdef V4
			  ///////////////////////////////////
			  //// BASE64の行か確認の上テキストに変換
			  if (bBase64Data)
				nLen = Base64DecodeLine(mwork, strlen(mwork), mToken, sizeof(mToken));
			  else {
				strcpy(mToken, mwork);
				nLen = strlen(mToken);
			  }
			  if (nLen)
#else
              if ((nLen = Base64DecodeLine(mwork, strlen(mwork), mToken, sizeof(mToken)))) 
#endif
			  {
				for (i = 0; i < nLen; i++) {
#ifdef V4
				  if (mToken[i] >= (bScriptType ? '\x21' : '\x20') && mToken[i] <= '\x7f') // ASCIIのみ書込み
#else
				  if (mToken[i] >= '\x20' && mToken[i] <= '\x7f') // ASCIIのみ書込み
#endif
				  {
				    fputc(mToken[i], fo);
					bZero = FALSE;
				  } else if (!bZero) { // １度目のNULLで改行
				    fputc('\n', fo);
					bZero = TRUE;
				  }
				}
			  }
              fgets( mwork, sizeof(mwork), fp);
			}
			fclose(fo);
		  }
		  n = 0;
		  bData = FALSE;
		  if ((fo = fopen(mTemp, "rt"))) {
            fgets(mwork, sizeof(mwork), fo);
	        while(!feof(fo)) {
			  strtok(mwork, "\r\n");
			  if ((q = strstr(mwork, "MZ"))) {
			    if (!strcmp(q, "MZ")) //if (strstr(mwork, "MZ"))
				  bDos = TRUE;
			  }
#ifdef V4
			  else if (bScriptType)
				bDos = TRUE;
#endif
			  if (bDos) {
				n = __max(n,strlen(mwork));
				mwork[n] = '\x0';
				if (VirusToken(mwork) ) {  // その他のトークン
                  bDoubtful = TRUE;
				  bsts = FALSE;
				  break;
				} 
#ifdef V4
				if (!bData && !strcmp(mwork, "PE")) {
			 	  if (n < 8 || n >= 44) {// "PE"出現までに、８文字より大きい文字列が存在しない＝EXEが暗号化されている可能性が大きい。
                    bDoubtful = TRUE;    // "PE"出現までに、４８文字以上の文字列が出現する場合＝ファイル名が可変されている可能性が大きい。
				    bsts = FALSE;
				    break;
				  }
				  bData = TRUE;
				}
#endif
			  }
              fgets(mwork, sizeof(mwork), fo);
			}
			fclose(fo);
#ifndef TUNING1
            DeleteFile(mTemp);
#else
            _unlink(mTemp);
#endif

			if (bDoubtful && nVirusDoubtful == 1) {  // ログに保存
#ifdef UPDATE_20071204  // メッセージＩＤ採番処理を修正(Bym10id)
		      sprintf(mTemp, "%sviruslog\\%s.MSG", mMailSpoolDir, pContext->mMsgId);
              CopyFile( pContext->mFnData, mTemp, TRUE);
		      sprintf(mTemp, "%sviruslog\\%s.RCP", mMailSpoolDir, pContext->mMsgId);
              CopyFile( pContext->mFnHead, mTemp, TRUE);
#else
		      sprintf(mTemp, "%sviruslog\\B%010lu.MSG", mMailSpoolDir, pContext->nMsgId);
              CopyFile( pContext->mFnData, mTemp, TRUE);
		      sprintf(mTemp, "%sviruslog\\B%010lu.RCP", mMailSpoolDir, pContext->nMsgId);
              CopyFile( pContext->mFnHead, mTemp, TRUE);
#endif
		      if ((fo = fopen(mTemp, "at"))) {
		        fprintf(fo, "\n");
                fprintf(fo, "  ------------ warning ------------\n");
                fprintf(fo, "  The doubt of the \"unknown virus\".\n");
                fprintf(fo, "  ---------------------------------\n");
		        fclose(fo);
			  }
			}
		  }
		}
		if (!strcmp(mwork, "\n") || !strcmp(mwork, "\r") || !strcmp(mwork, "\r\n"))
		  bBlank = TRUE;
		else
		  bBlank = FALSE;
		if (bDoubtful) // ウイルスの疑い有りと判定した場合
		  break;
	  }
      fgets( mwork, sizeof(mwork), fp);
	}
	fclose(fp);
  }
#ifdef UPDATE_20040720_LOG
  strcpy(mLog, "end Check_Of_MIME()");
  LEVEL_3_ACCEPTLOG(pContext, mLog);
#endif
  return bsts;
}
#endif