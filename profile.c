////////////////////////////////////////////////////////////
// Profile.c Copyright K.kawakami
// Get profile key and data.
////////////////////////////////////////////////////////////
#include "smtp.h"
#include <windows.h>
#include <stdio.h>
#include "profile.h"
#ifdef ADD_IDCACHE
#include <share.h>
#endif
#ifdef UPDATE_20240127 // ＭＬ制御応答＆送信エラー通知メールにもDKIMサインを追加可能にするオプション
#include <process.h>
#endif
#ifdef UPDATE_20260603 // ログ出力の排他処理化を追加
#include <stdarg.h>
#endif 
//#define WAIT_TIME      10
#define PROFILE_ROOT_TREE "%s\\"
#define MAX_VALUE_NAME    128

#define MAX_REG_RCPT      384

BOOL  wildcard_stricmp(const char *string1, const char *string2);
void SetRCPFile(struct _RCP *rcp, char *mRCP, BOOL bmod, DWORD *no);
void printfTrace(char *str);
void translateUue2Base64(char *line, int inlen, char *newline);
int Base64DecodeLine(char * pszLine, int nLineLen, unsigned char * uchBuffer, int nBufferLen);
NET_API_STATUS GetWinUserList(char *pName, char *pDom, struct _RCP *rcp, char *mRCP, DWORD nMLDomDiv, DWORD nNoOfDiv, DWORD *nListNo);
NET_API_STATUS GetSpaUserList(char *pName, char *pDom, struct _RCP *rcp, char *mRCP, DWORD nMLDomDiv, DWORD nNoOfDiv, DWORD *nListNo);

#ifdef CLUSTERING
extern BOOL   nClustering;
#endif

#ifdef V3
extern BOOL    bUserMan;
extern BOOL    bInboxEnc;
#endif
extern BOOL    bDebug;
extern BOOL    bCountLock;
extern DWORD   nDomainDivide;
extern DWORD   nMailInBoxSize;
extern CHAR    mMailBoxDir[];
extern char    mLocalDomain[];
extern char    mMailSpoolDir[];
extern char    mPasswordFile[];
extern int     nSenderLog;
#ifdef LDAP_ON         // LDAPへのログイン処理でSMTP認証の確認を行うオプション // //CRAM-MD5は複合できないので、これを利用する場合は、PLAINかLOGINのみとします。
extern BOOL   bLDAP;
extern DWORD  nLDAPPort;
extern char   mLDAPAdminID[];
extern char   mLDAPAdminPW[];
extern char   mLDAPSchemaHome[];
extern char   mLDAPSchemaName[];
#endif
#ifdef UPDATE_20070521 // OSの予約語対策
extern CHAR   mReservedWords[];
#endif
#ifdef UPDATE_20150108 // 送信許可時間の設定機能を追加
extern int  nUseTime;
extern char mUseTimeFile[];
extern char mProgPath[];
#endif
#ifdef ADD_IDCACHE
extern BOOL bServiceTerminating;
extern int  nIDCashLiveTime; // ADキャッシュ利用有効時間(秒単位)
extern char mComName[];
#endif
#ifdef UPDATE_20230105 // X-UIDLヘッダを追加するオプション
extern BOOL bADDXUIDL; // X-UIDLヘッダを追加するか否か
#endif
#ifdef UPDATE_20231007 // STARTTLSで暗号化ネゴシェーションに失敗するドメインリストの作成と失敗先へのプレーン送信への切替対策
extern BOOL bBadSTLSDom; // STARTTLSで暗号ネゴシェーションに失敗するサイトの記録 0:しない。（デフォルト）1:する。
#endif
#ifdef UPDATE_20240127 // ＭＬ制御応答＆送信エラー通知メールにもDKIMサインを追加可能にするオプション
// DKIM設定箇所の選択 bit0:(受信時付加)デフォルト bit1:上長承認メールへ付加 bit2:添付分離めーるへ付加
extern int    bDKIM;
extern CHAR   mDomainAUTHDKIM[];
#endif
#ifdef UPDATE_20240127C // DKIMサイン実施無効フラグを追加 1:無効(bit0:送信エラー bit1:ＭＬ制御応答 bit2:転送メール bit:3自動応答)
extern int    nOnDKIM;
extern CHAR   mDomainAUTHARC[];
#endif

#ifdef UPDATE_20231103 // エラーメール生成時にメールヘッダの情報を利用するオプション機能を追加。
//CHAR  mwork[512]; GetInfo(_T("subject:"), mMSG, mwork, sizeof(mwork));
void GetInfo(_TCHAR *pTkn, _TCHAR *pFn, _TCHAR *pData, DWORD nMaxLen)
{
	FILE *fp;
    _TCHAR *p, *ps;
	BOOL bNext = FALSE;
	int  l = 0;
	DWORD nLen = 0;
	_TCHAR mData[1024], mDest[1024], mPack[1024];

	*pData = '\x0';
	bNext = FALSE;
	if ((fp = _tfopen(pFn, _T("rt")))) {
	  while ((p = _fgetts(mData, sizeof(mData), fp)))
	  {
		if (mData[0] == '\r' || mData[0] == '\n') // ヘッダの終わり
		  break;
        ps = mData;
		if (!_tcsnicmp(mData, pTkn, strlen(pTkn)) ||
			(bNext && (*ps == ' ' || *ps == '\t')))
		{
		  strtok(mData, _T("\r\n"));
		  if (!bNext) {
			ps = &mData[strlen(pTkn)];
		    bNext = TRUE;
		  } else {
			ps = mData;
		  }
		  while(*ps == ' ' || *ps == '\t')
		    ps++;
          // データ行に改行だけの場合も除去するようにする
		  if (*ps)
		  {
		    mDest[0] = '\x0';
			/*
		    LineConv2(mDest, sizeof(mDest), ps, mPack);
		    if (nMaxLen > nLen + strlen(mDest)) {  // バッファオーバーしないように対策
		      strcat(pData, mDest);
		      nLen += strlen(pData);
			}
			*/
		    if (nMaxLen > nLen + strlen(ps)) {  // バッファオーバーしないように対策
		      strcat(pData, ps);
		      nLen += strlen(pData);
			}

		  }
		} else {
		  bNext = FALSE;
		}
	  }
	  fclose(fp);
	}
}

void GetEnvelope(char *pEnveopes, char *pMSG, char *pEnv)
{
  BOOL bHit = FALSE;
  char *p, *q, mHeader[512], mMailEnv[512], mwork[512];

  if (*pEnveopes)
  {
    mHeader[0] = mMailEnv[0] = mwork[0] = '\x0';
    p = mMailEnv;
    strcpy(mMailEnv, pEnveopes); // エラーメールのエンベローブのヘッダ一覧の設定
	strtok(mMailEnv, "\r\n");
    while((q = strpbrk(p, ",;: \t")))
	{
	  *q = '\x0';
	  sprintf(mHeader, "%s:", p);
	  GetInfo(mHeader, pMSG, mwork, sizeof(mwork));
//if (bDebug) printf("GetEnvelope():Header=%s/mwork=%s\n", mHeader, mwork);
	  if (strchr(mwork, '@'))
        break;
	  p = q+1;
	}
    if (!mwork[0]) {
	  sprintf(mHeader, "%s:", p);
	  GetInfo(mHeader, pMSG, mwork, sizeof(mwork));
//if (bDebug) printf("GetEnvelope():Header=%s/mwork=%s\n", mHeader, mwork);
	}

    if (p = strchr(mwork, '<'))
	{
	  p++;
	  strtok(p, ">");
	  bHit = TRUE;
	} else {
      p = mwork;
	}
    while(*p == ' ' || *p == '\t')
	  p++;

    if (!bHit && strchr(p, '@'))
      bHit = TRUE;

    if (bHit)
      strcpy(pEnv, p);
  }
}
#endif

#ifdef UPDATE_20231007 // STARTTLSで暗号化ネゴシェーションに失敗するドメインリストの作成と失敗先へのプレーン送信への切替対策
// STARTTLSに失敗するドメインリストの設定
void SetSTLSCheck(char *pDom)
{
#ifdef TRACE_MODE
	char str[LOG_BUFFER];
#endif
  FILE    *fp;
  char    mSTLSFail[512];

  if (bBadSTLSDom)
  {
#ifdef TRACE_MODE
    if (nSenderLog) {
      sprintf(str, "[          ] SetSTLSFailList=[%s]\n", pDom);
      printfTrace(str);
	}
#endif

#ifdef REGTOFILE
    sprintf(mSTLSFail,"%s\\badstls\\", mMailSpoolDir);
    _mkdir(mSTLSFail);         // cashフォルダ作成
    if (nClustering) {
      sprintf(mSTLSFail,"%s\\badstls\\%s\\", mMailSpoolDir, mComName);
    } else {
#endif
      sprintf(mSTLSFail,"%s\\badstls\\", mMailSpoolDir);
#ifdef REGTOFILE
    }
#endif
    _mkdir(mSTLSFail);         // cashフォルダ作成
    strcat(mSTLSFail, pDom);
    if (fp = fopen(mSTLSFail, "wt"))
    {
      fclose(fp);
    }
  }
}

// STARTTLSに失敗するドメインリストの確認
BOOL GetSTLSCheck(char *pDom)
{
#ifdef TRACE_MODE
	char str[LOG_BUFFER];
#endif
  FILE    *fp;

  char    mSTLSFail[512];
  BOOL    bSts = TRUE; // STLSFailフォルダに記録されている否か

  if (bBadSTLSDom)
  {
#ifdef REGTOFILE
    if (nClustering) {
      sprintf(mSTLSFail,"%s\\badstls\\%s\\%s", mMailSpoolDir, mComName, pDom);  // STLSFailフォルダ
	} else {
#endif
      sprintf(mSTLSFail,"%s\\badstls\\%s", mMailSpoolDir, pDom);  // STLSFailフォルダ
#ifdef REGTOFILE
	}
#endif
    if ((fp = fopen(mSTLSFail, "rt")))
    {
       bSts = FALSE;
       fclose(fp);
    }
#ifdef TRACE_MODE
    if (nSenderLog) {
      sprintf(str, "[          ] GetSTLSFailList=[%s] status=%s\n", pDom, (bSts ? "Hit" : "Fail"));
      printfTrace(str);
	}
#endif
  }
  return bSts;
}

// STARTTLSに失敗するドメインリストの初期化
void CleanSTLSCheck(void) 
{
#ifdef TRACE_MODE
	char str[LOG_BUFFER];
#endif
  HANDLE             hF, hFile;
  WIN32_FIND_DATA    FD;
  BOOL               bFile = TRUE;
  char    mSTLSFail[512], mCash[512];
  FILE    *fp;

#ifdef REGTOFILE
      if (nClustering) {
        sprintf(mSTLSFail,"%s\\badstls\\%s\\*", mMailSpoolDir, mComName);  // badstlsフォルダ
	  } else {
#endif
   sprintf(mSTLSFail,"%s\\badstls\\*", mMailSpoolDir);  // badstlsフォルダ
#ifdef REGTOFILE
	  }
#endif
     /////////////////////////////////////////////////////////////////
     hF = FindFirstFile(mSTLSFail, &FD);
     if (hF != INVALID_HANDLE_VALUE) {
       bFile = TRUE;
       while (bFile) {
	     if (!(!stricmp(FD.cFileName, ".") ||
	           !stricmp(FD.cFileName, ".."))) {
#ifdef REGTOFILE
           if (nClustering) {
             sprintf(mCash,"%s\\badstls\\%s\\%s", mMailSpoolDir, mComName, FD.cFileName);  // badstlsフォルダ
		   } else
#endif
		   {
             sprintf(mCash,"%s\\badstls\\%s", mMailSpoolDir, FD.cFileName);  // badstlsフォルダ
		   }
           _unlink(mCash);
#ifdef TRACE_MODE
           if (nSenderLog) {
             sprintf(str, "[          ] [%s] CleanSTLSCheck()\n", mCash);
             printfTrace(str);
		   }
#endif
		 }
         bFile = FindNextFile( hF, &FD);
	   }; 
       FindClose( hF ); 
	 }
}
#endif

#ifdef UPDATE_20230105 // X-UIDLヘッダを追加するオプション
void AddUIDLHeader(char *pMSG, char *pToken, BOOL bAct)
{
  long hFindFileMSG;
  struct _finddata_t  FindMSG;
  FILE *fpsrc, *fpdst;
  char *p, mMSG[256], mMSG2[256];
  char mLine[1024];

  if (!bADDXUIDL) // 追加オプション無効なら何もしない。
	return;
 
  strcpy(mMSG, pMSG);
  strcpy(mMSG2, pMSG);
  mMSG[strlen(mMSG)-3]='~';
  mMSG2[strlen(mMSG)-3]='-';
  if (p = strrchr(pToken, '\\'))
  {
    if (bAct)
	{
      if (fpdst=fopen(mMSG, "wb"))
	  {
        fprintf(fpdst, "X-UIDL: %s\r\n", p+1);
        if (fpsrc=fopen(pMSG, "rb"))
		{
		  while(fgets(mLine, sizeof(mLine)-1, fpsrc))
		  {
		    fputs(mLine, fpdst);
		  }
	      fclose(fpsrc);
		}
        fflush(fpdst);
	    fclose(fpdst);
	  }
      while((hFindFileMSG = _findfirst(mMSG, &FindMSG)) == -1L) { // ファイルが削除されていること
        if (bServiceTerminating) {
          return;
		}
	    _sleep(WAIT_TIME);
	  }
	  _findclose( hFindFileMSG );
 	  //DeleteFile(pMSG);
      if((hFindFileMSG = _findfirst(mMSG2, &FindMSG)) == -1L) {
	    X_MoveFile(pMSG, mMSG2); 
        while((hFindFileMSG = _findfirst(pMSG, &FindMSG)) != -1L) { // ファイルが削除されていること
	      _findclose( hFindFileMSG );
	      //DeleteFile(pMSG);
          X_MoveFile(pMSG, mMSG2);
          if (bServiceTerminating) {
            return;
		  }
	      _sleep(WAIT_TIME);
		}
	  } else {
        _findclose( hFindFileMSG );
	  }
	  X_MoveFile(mMSG, pMSG); // 書換え
      while((hFindFileMSG = _findfirst(pMSG, &FindMSG)) == -1L) { // ファイルが削除されていること
        if (bServiceTerminating) {
          return;
		}
	    _sleep(WAIT_TIME);
	    X_MoveFile(mMSG, pMSG);
	  }
	  _findclose( hFindFileMSG );
	} else {
	  DeleteFile(pMSG);
      while((hFindFileMSG = _findfirst(pMSG, &FindMSG)) != -1L) { // ファイルが削除されていること
	    _findclose( hFindFileMSG );
	    DeleteFile(pMSG);
        if (bServiceTerminating) {
          return;
		}
	    _sleep(WAIT_TIME);
	  }
	  X_MoveFile(mMSG2, pMSG); // 復帰
      while((hFindFileMSG = _findfirst(pMSG, &FindMSG)) == -1L) { // ファイルが削除されていること
        if (bServiceTerminating) {
          return;
		}
	    _sleep(WAIT_TIME);
	    X_MoveFile(mMSG2, pMSG); // 復帰
	  }
	  _findclose( hFindFileMSG );
	  DeleteFile(mMSG2);
      while((hFindFileMSG = _findfirst(mMSG2, &FindMSG)) != -1L) { // ファイルが削除されていること
	    _findclose( hFindFileMSG );
	    DeleteFile(mMSG2);
        if (bServiceTerminating) {
          return;
		}
	    _sleep(WAIT_TIME);
	  }
	}
  }
}
#endif

#ifdef UPDATE_20190426 // 利用する暗号アルゴリズムの組み合わせオプション 
void Print_SSL_CIPHER_names(const SSL_CTX *ctx)
{
#ifdef VC6
  const _STACK *ciphers;
#else
  STACK_OF(SSL_CIPHER) *ciphers;
#endif
  const SSL_CIPHER *cipher;
  int i, num;

  if (ciphers = SSL_CTX_get_ciphers(ctx))
  {
#ifdef VC6
	 num = (int)((ciphers) ? (ciphers)->num:-1); 
#else
	 num = sk_SSL_CIPHER_num(ciphers);
#endif
     for(i = 0; i < num; i++)
	 {
#ifdef VC6
	   cipher = ((ciphers) ? (ciphers)->data[i] : NULL); 
#else
	   cipher = sk_SSL_CIPHER_value(ciphers, i);
#endif
       printf("%s\n", SSL_CIPHER_get_name(cipher));
     }
#ifdef VC6
	 /*
     if (ciphers == NULL)
       return;
     for (i = 0; i < num; i++)
       if (ciphers->data[i] != NULL)
         OPENSSL_free((void *)ciphers->data[i]); // ハングする
     OPENSSL_free(ciphers);
	 */
	 //sk_SSL_CIPHER_free(ciphers);
#else
     //sk_SSL_CIPHER_free(ciphers);
#endif
  }
}
#endif
#ifdef UPDATE_20070621 // ユーザ毎のSMTP認証ID/PWを設定可能にする
BOOL GetPasswordFile(char  *pDir, char *pUser, char *mpass)
{
  FILE   *fp;
  CHAR   Path[MAX_PATH];
  BOOL   bsts = FALSE;
  char   mPwd[256];
    //sprintf(Path, "%sapop.dat", pDir);
    sprintf(Path, "%s%s", pDir, mPasswordFile);
    if (bDebug) printf("Authentication file name = %s\n", Path);
	if ((fp = fopen(Path, "rt"))) {
      fgets(mpass, _MAX_PATH, fp);
	  strtok(mpass, "\n");
	  fclose(fp);
      SPA_Decode(mpass, mPwd); // 2002.08.30
	  strcpy(mpass, mPwd);     // 2002.08.30
	  bsts = TRUE;
      if (bDebug) printf(" -> found Authentication file name (mailbox)\n");
	}
    return bsts;
}
#endif

#ifdef UPDATE_20241124 // OAUTH2での認証方式用にユーザ別認証コード保管ファイル(xoauth2_code.dat)を各アカウントフォルダに設定可能にした
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
       sprintf(Path, "%soauth2/%s/%s/xoauth2_code.dat", pDir, p+1, pUser);
#endif
	   *p = '@';
	} else {
#ifdef WINDOWS
       sprintf(Path, "%soauth2\\%s\\%s", pDir, pUser, pFn);
#else
       sprintf(Path, "%soauth2/%s/xoauth2_code.dat", pDir, pUser);
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

BOOL GetXOAUTH2CODEFile(char  *pDir, char *pUser, char *mpass)
{
  FILE   *fp;
  CHAR   Path[MAX_PATH];
  BOOL   bsts = FALSE;
  char   mPwd[3072];
#ifdef UPDATE_20241127_BOX // OAUTH2でのアクセストークンとリフレッシュトークンの保存先を[メール作業フォルダ/oauth2/メールアドレス]に変更するようにした。
  char   c;
  char   *p, *q;
#endif

#ifdef UPDATE_20241127_BOX // OAUTH2でのアクセストークンとリフレッシュトークンの保存先を[メール作業フォルダ/oauth2/メールアドレス]に変更するようにした。
    sprintf(Path, "%soauth2", pDir);
	_mkdir(Path);

	strcpy(Path, GetXOAUTH2Path(pDir, pUser, "xoauth2_code.dat"));

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
#else
    //sprintf(Path, "%sapop.dat", pDir);
    sprintf(Path, "%sxoauth2_code.dat", pDir);
#endif
    if (bDebug) printf("Authentication file name = %s\n", Path);
	if ((fp = fopen(Path, "rt"))) {
      fgets(mpass, sizeof(mPwd)-1, fp);
	  strtok(mpass, "\r\n");
#ifdef UPDATE_20241129 // // XOAUTH2の認可コードの読出し完了時のファイルのクローズ抜けていた。
	  fclose(fp);
#endif
	  bsts = TRUE;
      if (bDebug) printf(" -> found Authentication file name (mailbox)\n");
	}
    return bsts;
}
#endif

#ifdef UPDATE_20070301 // 対象アドレスの比較 *ワイルドカード有効にする 2007.03.01
char * _stristr( const char *string1, const char *string2 ) {
   int  n;
   char *p, *q, *r;
   char c[2];

   r = p = string1;
   q = string2;
   n = strlen(string2);
   if (n == 0)
	 return NULL;

   while(*p) {
	 if (*p >= 0x41 && *p <= 0x5a) // アルファベットが大文字なら小文字に変換
	   c[0] = *p + 0x20;
      else	 
	   c[0] = *p;
	 if (*q >= 0x41 && *q <= 0x5a) // アルファベットが大文字なら小文字に変換
	   c[1] = *q + 0x20;
      else	 
	   c[1] = *q;

      p++;
	  if (c[0] == c[1]) {
		q++;
		n--;
		if (n == 0)
		  break;
	  } else {  // はじめからチェック
		r = p;
	    q = string2;
        n = strlen(string2);
	  }
   }

   if (n != 0)
	 return NULL;
   else
	 return r;

}

BOOL  wildcard_stricmp(const char *string1, const char *string2) {
  char *p, *q, *r;
  int  k, l, m, n = 0;

  if (p = strchr(string1,'*')) {
    *p = '\x0';
    k = strlen(string1);
    l = strlen(p+1);
    *p = '*';
	m = 0;
    // UPDATE_20070922 // 先頭カラムと最後尾カラムのワイルドカード指定があると処理できない
	if (k == 0) {
	  m = strlen((char *)(string1+1));
	}
    ////////////////////
    //if (k+l <= (int)strlen(string2))
	{
      if (strnicmp(string2, string1, k) == 0) {
		n += k;
	    if (q = strchr(p+1, '*')) {
	      *q = '\x0';
          l = strlen(p+1);
	      m = strlen(q+1);
		}
		//if ((r = strstr(string2+n, p+1))) {
		r = _stristr(string2+n, p+1);
		if (q)
		  *q = '*';
		if (r) { // 大小文字
          if (m > 0) {
            return wildcard_stricmp(p+1, r);
		  } else {
			return (strlen(p+1) == strlen(r) || q ? TRUE : FALSE);
		    //return TRUE;
		  }
        } else {
		  return (q && r ? TRUE : FALSE);
		}
      } else {
		return FALSE;
	  }
    //} else {
  	//  return FALSE;
	}
  } else {
	return (stricmp(string1, string2)== 0 ? TRUE : FALSE);
  }
}
#endif

#ifdef UPDATE_20060529 // 設定ドメイン毎のＩＰに送信元ＩＰを切替える設定
void GetMTAIP(char *pDom, char *pMTAIP) {
  time_t  lt;
  DWORD   n = 1, m;
  char    *p, mMTAIP[0x4000];       // レジストリ：バイナリの限界値 [4096];
#ifdef UPDATE_20070521 // 予約語対策
  CHAR    mKey[256];
#endif

  time(&lt);
#ifdef UPDATE_20070521 // 予約語対策
  strcpy(mKey, pDom);
  strtok(mKey, "@");
  if (ReservedWords(mKey)) {
    strcpy(mKey, mReservedWords);
    strcat(mKey, pDom);
  } else
  {
    strcpy(mKey, pDom);
  }
  GetProfileBinaryEx((LPCTSTR)DOMAIN_MTAIP, (LPCTSTR)mKey, "", (LPTSTR)mMTAIP, (DWORD)sizeof(mMTAIP));
#else
  GetProfileBinaryEx((LPCTSTR)DOMAIN_MTAIP, (LPCTSTR)pDom, "", (LPTSTR)mMTAIP, (DWORD)sizeof(mMTAIP));
#endif
  if (mMTAIP[0]) {
    p = mMTAIP;
    while(*p) { // NULLなら終了
	  p += strlen(p) + 1;
#ifdef UPDATE_20070122 // 設定ドメイン毎のＩＰに送信元ＩＰを切替える設定の修正
	  if (*p) // NULLでないならカウント
#endif
        n++;
	}
    m = lt % n;
    p = mMTAIP;
#ifdef UPDATE_20070122 // 設定ドメイン毎のＩＰに送信元ＩＰを切替える設定の修正
    for (n = 1; n <= m; n++)
#else
    for (n = 0; n < m; n++)
#endif
	{
	  p += strlen(p) + 1;
	}
#ifdef UPDATE_20070122 // 設定ドメイン毎のＩＰに送信元ＩＰを切替える設定の修正
	strcpy(pMTAIP, p);
#else
    pMTAIP = p;
#endif
  } else {
#ifdef UPDATE_20070122 // 設定ドメイン毎のＩＰに送信元ＩＰを切替える設定の修正
	strcpy(pMTAIP, mMTAIP);
#else
    *pMTAIP = mMTAIP[0];
#endif
  }
}
#endif

#ifdef UPDATE_20060513 // ＭＬ管理メールにキーコードを付加
char tochar(char *p) {
  int  i;
  char c = 0x00;
 
  for (i = 0; i < 2; i++) {
	switch (*(p+i)) {
	case '0': c |= 0x00 >> 4*i; break;
	case '1': c |= 0x10 >> 4*i; break;
	case '2': c |= 0x20 >> 4*i; break;
	case '3': c |= 0x30 >> 4*i; break;
	case '4': c |= 0x40 >> 4*i; break;
	case '5': c |= 0x50 >> 4*i; break;
	case '6': c |= 0x60 >> 4*i; break;
	case '7': c |= 0x70 >> 4*i; break;
	case '8': c |= 0x80 >> 4*i; break;
	case '9': c |= 0x90 >> 4*i; break;
	case 'a': 
	case 'A': c |= 0xA0 >> 4*i; break;
	case 'b': 
	case 'B': c |= 0xB0 >> 4*i; break;
	case 'c': 
	case 'C': c |= 0xC0 >> 4*i; break;
	case 'd': 
	case 'D': c |= 0xD0 >> 4*i; break;
	case 'e': 
	case 'E': c |= 0xE0 >> 4*i; break;
	case 'f': 
	case 'F': c |= 0xF0 >> 4*i; break;
	}
  }
  return c;
}

void SPA_Encode(char *pSrc, char *pDest) { // 記号化
   int i, n;
   char mEn[3];
   SYSTEMTIME lt;
   unsigned char cMask;
   
   if (*pSrc != '\xff' && strlen(pSrc)) {// 記号化フラグ無し
     GetSystemTime(&lt);
     //cMask = (char)(lt.wMilliseconds % 0xfe + 1);
	 cMask = (char)(lt.wMilliseconds % 0xef + 0x10);
     sprintf(pDest, "\xff%c", cMask); // 記号化フラグセット
     n = strlen(pSrc);
     for (i = 0; i < n; i++) {
	   //sprintf(mEn, "%02x", __toascii(*pSrc));
       sprintf(mEn, "%02x", (unsigned char)((unsigned char)((*pSrc)^cMask)));
       strcat(pDest, mEn);
	   pSrc++;
	 }
   } else {
     strcpy(pDest, pSrc); // そのまま
   }
}

void SPA_Decode(char *pSrc, char *pDest) { // 復号化
   int i, n;
   char mEn[3];
   unsigned char cMask;

   mEn[2] = '\x0';
   if (*pSrc == '\xff') {// 記号化フラグ有り
	 cMask = *(pSrc+1);
	 pSrc+=2;
     n = strlen(pSrc);
	 for (i = 0; i < n; i += 2) {
       strncpy(mEn, (pSrc+i), 2);
       *pDest = (char)(tochar(mEn)^cMask);
	   pDest++;
	 }
	 *pDest = '\x0';
   } else { // 記号化フラグ無し
	 strcpy(pDest, pSrc);
   }
}
#endif

void SPA_FileEncode(char *pSrc, char *pDest, char cMask) { // 記号化
   int i, n;
   char mEn[3];
   
   *pDest = '\x0';
   n = strlen(pSrc);
   for (i = 0; i < n; i++) {
     //sprintf(mEn, "%02x", __toascii(*pSrc));
     sprintf(mEn, "%02x",  (unsigned char)((unsigned char)((*pSrc)^cMask)));
     strcat(pDest, mEn);
     pSrc++;
   }
}

void CopyFile_Encode(char *pSrc, char *pDest) {
   FILE *fpSrc, *fpDest;
   char mLine[512], mEnc[1024], *p;
   SYSTEMTIME lt;
   unsigned char cMask;

   fpSrc = fopen(pSrc, "rt");
   if (fpSrc) {
     if ((fpDest = fopen(pDest, "wt"))) {
       GetSystemTime(&lt);
       //cMask = (char)(lt.wMilliseconds % 0xfe + 1);
	   cMask = (char)(lt.wMilliseconds % 0xef + 0x10);
       fprintf(fpDest, "\xff%c", cMask); // 記号化フラグセット
	   //fputc('\xff', fpDest); // 記号化フラグセット
	   p = fgets(mLine, sizeof(mLine)-2, fpSrc);
	   while(p || !feof(fpSrc)) {
		 mLine[sizeof(mLine)-2] = '\x0';
		 if (mLine[0] == '\r' || mLine[0] == '\n')
		   mLine[0] = '\x0';
		 strtok(mLine, "\r\n");
		 strcat(mLine, "\r\n");
         //if (strlen(mLine) == 511)
         //  printf("");
		 SPA_FileEncode(mLine, mEnc, cMask);
         //printf("%d->%d\n", strlen(mLine), strlen(mEnc));
		 fputs(mEnc, fpDest);
         //fputs(mEnc, fpDest);
		 mLine[0] = '\x0';
		 p = NULL;
	     p = fgets(mLine, sizeof(mLine)-2, fpSrc);
	   }
	   fflush(fpDest);
	   fclose(fpDest);
	 }
	 fclose(fpSrc);
   }
}

BOOL CopyB64EncodeFile(char *pSrc, char *pDest) { //BASE64で記号化して保存します。
   FILE *fpSrc, *fpDest;
   char mLine[129], mMime[256], *p;
   BOOL bSts = FALSE;

   fpSrc = fopen(pSrc, "rt");
   if (fpSrc) {
     if ((fpDest = fopen(pDest, "wt"))) {
	   p = fgets(mLine, sizeof(mLine)-1, fpSrc);
	   while(p || !feof(fpSrc)) {
		 mLine[sizeof(mLine)-1] = '\x0';
	     translateUue2Base64(mLine, strlen(mLine), mMime);
		 fprintf(fpDest, "%s\n", mMime);
		 p = NULL;
	     p = fgets(mLine, sizeof(mLine)-1, fpSrc);
	   }
	   bSts = TRUE;
	   fflush(fpDest);
	   fclose(fpDest);
	 }
	 fclose(fpSrc);
   }
   return bSts;
}

BOOL CopyB64DecodeFile(char *pSrc, char *pDest) { //BASE64から復号して保存します。
   FILE *fpSrc, *fpDest;
   char mLine[256], mDec[256], *p;
   BOOL bSts = FALSE;

   fpSrc = fopen(pSrc, "rt");
   if (fpSrc) {
     if ((fpDest = fopen(pDest, "wt"))) {
	   p = fgets(mLine, sizeof(mLine)-1, fpSrc);
	   while(p || !feof(fpSrc)) {
		 mLine[sizeof(mLine)-1] = '\x0';
		 strtok(mLine, "\n");
		 Base64DecodeLine(mLine, strlen(mLine), mDec, sizeof(mDec));
		 fputs(mDec, fpDest);
		 p = NULL;
	     p = fgets(mLine, sizeof(mLine)-1, fpSrc);
	   }
	   bSts = TRUE;
	   fflush(fpDest);
	   fclose(fpDest);
	 }
	 fclose(fpSrc);
   }
   return bSts;
}

BOOL CheckDomain(char *mID) {
  char *pldom, *pid;
  BOOL sts = FALSE; // Global
  
  pldom = mLocalDomain;
  pid  = strstr(mID, "@");
  if (pid) {
	pid++;
    while (strlen(pldom)){
      //printf(" \"%s\"", pldom);
	  if (stricmp(pldom, pid) == 0) {
		 sts = TRUE; //Local
		 break;
	  }
      pldom += strlen(pldom);
      pldom++;
	};
  } else
	sts = TRUE;  //Local
  return sts;
}

void GetMailBoxFolder(char *user, char *pMailBox) {
 CHAR mHome[256];
 CHAR mMIBD[256], *p1, *p2;
 BOOL bHome = FALSE;
 INT  nl;
#ifdef UPDATE_20070521 // 予約語対策
 CHAR mKey[256];
#endif

#ifdef V3
    BOOL bLink = FALSE;
    char mLink[256], *pdomain;

	mLink[0] = '\x0';
	pdomain = strstr(user, "@");
    if (pdomain) {
	  *pdomain = '\x0';
	  pdomain++;
#ifdef UPDATE_20070521 // 予約語対策
      strcpy(mKey, pdomain);
      strtok(mKey, "@");
      if (ReservedWords(mKey)) {
        strcpy(mKey, mReservedWords);
        strcat(mKey, pdomain);
	  } else
	  {
        strcpy(mKey, pdomain);
	  }
      GetProfileStringEx(DOMAIN_FOLDER, mKey, "", mLink, sizeof(mLink)); // 対象ドメインの応答ＩＰアドレス取出し
#else
      GetProfileStringEx(DOMAIN_FOLDER, pdomain, "", mLink, sizeof(mLink)); // 対象ドメインの応答ＩＰアドレス取出し
#endif
#ifdef UPDATE_20071022 // ドメイン無しのアカウントはデフォルトドメインを指定する
	} else {
#ifdef UPDATE_20070521 // 予約語対策
      strcpy(mKey, mLocalDomain);
      strtok(mKey, "@");
      if (ReservedWords(mKey)) {
        strcpy(mKey, mReservedWords);
        strcat(mKey, mLocalDomain);
	  } else
	  {
        strcpy(mKey, mLocalDomain);
	  }
      GetProfileStringEx(DOMAIN_FOLDER, mKey, "", mLink, sizeof(mLink)); // 対象ドメインの応答ＩＰアドレス取出し
#else
      GetProfileStringEx(DOMAIN_FOLDER, mLocalDomain, "", mLink, sizeof(mLink)); // 対象ドメインの応答ＩＰアドレス取出し
#endif
#endif
	}
	if (mLink[0] != '\x0') {
	  bLink = TRUE;
#ifdef UPDATE_20070521 // 予約語対策
      strcpy(mKey, mLink);
      strtok(mKey, "@");
      if (ReservedWords(mKey)) {
        strcpy(mKey, mReservedWords);
	    strcat(mKey, mLink);
        strcpy(mLink, mKey);
	  }
#endif
	}   
#ifdef UPDATE_20091101 // HOMEディレクトリタイプで無いならホームフォルダの検索をしないようにした
   mHome[0] = '\x0';
   if (!strnicmp(mMailBoxDir, "%home%", 6))
#endif
   {
#ifdef UPDATE_20091014A // ドメイン無しアカウントでホームディレクトリ参照時ハングする可能性がある
	 UserHomeDir( NULL, NULL, user, (pdomain ? pdomain : mLocalDomain), mHome);
#else
     UserHomeDir( NULL, NULL, user, pdomain, mHome);
#endif
#else
     UserHomeDir( NULL, NULL, user, mHome);
#endif
   }
   *pMailBox = '\x0';
   /*
   GetProfileStringEx((LPCTSTR)SOFT_REG,(LPCTSTR)"MailInBoxDir", (LPCTSTR)MAIL_BOX, (LPTSTR)mMIBD, 256);
	if (mMIBD[0] == '\x0')
	  strcpy(mMIBD, MAIL_BOX);
   */
   strcpy(mMIBD, mMailBoxDir);
   if (strnicmp(mMIBD, "%home%", 6) == 0) {
	 strcat(pMailBox, mHome);
	 bHome = TRUE;
   }
   p1 = strstr(mMIBD, "%USERNAME%");
   if (!p1)
     p1 = strstr(mMIBD, "%username%");
   if (p1) {
	 *p1 = '\x0';
     p2 = p1 + 10;
     if (bHome)
	   strcat(pMailBox, &mMIBD[6]);
	 else
	   strcat(pMailBox, mMIBD);
#ifdef V3
	 if (bLink) {
       strcat(pMailBox, mLink);
       strcat(pMailBox, "\\");
	 }
#endif
#ifdef UPDATE_20070521 // 予約語対策
	 if (ReservedWords(user))
	   strcat(pMailBox, mReservedWords);
#endif
#ifdef UPDATE_20060116 // バッファオーバーフロー対策
	 strncat(pMailBox, user, 250-(strlen(pMailBox)+strlen(p2)));
	 *(pMailBox+250) = '\x0';
#else
	 strcat(pMailBox, user);
#endif
	 strcat(pMailBox, p2);
   } else {
	 if (bHome) // %HOME% があるとき
	   strcat(pMailBox, &mMIBD[6]); 
	 else
	   strcat(pMailBox, mMIBD);     // %HOME% , %USERNAME%が共にない場合、メールボックスフォルダに固定 2003/08/17
   }
   // 末尾に "\"マークをなければ追加
   nl = strlen(pMailBox);
   if (nl != 0) {
     nl--;
     if (pMailBox[nl] != '\\')
      strcat(pMailBox,"\\");
   }
}

#ifdef UPDATE_20070521 // 予約語対策
BOOL ReservedWords(char *pUser) {
    BOOL bsts = FALSE;
    int i, n[2];
#ifdef UPDATE_20071205 // メールアドレス長が255Byteのときの対策
	CHAR mData[512];
#else
	CHAR mData[256];
#endif
	CHAR mWords[25][7] = {"CON", "PRN", "AUX", "CLOCK$", "NUL",
                          "COM0", "COM1", "COM2", "COM3", "COM4", "COM5", "COM6", "COM7", "COM8", "COM9",
                          "LPT0", "LPT1", "LPT2", "LPT3", "LPT4", "LPT5", "LPT6", "LPT7", "LPT8", "LPT9"};

	strcpy(mData, pUser);
	strtok(mData, "@");
    for(i = 0; i < 25; i++) {
	   n[0] = strlen(mData);
	   n[1] = strlen(mWords[i]);
	   if ( !stricmp(mData, mWords[i]) ||
           (!strnicmp(mData, mWords[i], n[1]) && *(pUser+n[1]) == '.') ||
		   (!strnicmp(mData, mWords[i], n[1]) && n[0] == n[1]+1 && *(pUser+n[1]) == ' ') ) {
		 bsts = TRUE;
	   }
	}
	return bsts;
}
#endif

void GetPersonInfo(char *user, CHAR *mFileName, CHAR *mPersonFile) { 
 CHAR mTempDir[_MAX_PATH];
 //CHAR *tmp, *p, 
 CHAR *pdom;
 //INT  nl;

  strcpy(mTempDir, mMailBoxDir);
  pdom = strstr(user, "@");
  if (pdom)
    *pdom = '\x0';
  mPersonFile[0] = '\x0';
  /*
  p = mTempDir;          // %HOME%,%USENAME% をチェックする。
  tmp = strstr(p, "%");
  while (tmp) {
    *(tmp) = '\x0';
    strcat(mPersonFile, p);
    p = tmp+1;
    tmp = strstr(p, "%");
    *(tmp) = '\x0';
    if (_stricmp(p, "home") == 0) {
      strcat(mPersonFile, getenv("HOMEDRIVE"));
      strcat(mPersonFile, getenv("HOMEPATH"));
	} else if (_stricmp(p, "username") == 0)
      strcat(mPersonFile, user);
    tmp++;
    p = tmp;
    tmp = strstr(p, "%");
  } 
  strcat(mPersonFile, p);
  */
  GetMailBoxFolder(user, mPersonFile); // 2000.06.17 %HOME%が使えるように
  /*
  nl = strlen(mPersonFile);
  if (nl != 0) {
    nl--;
    if (mPersonFile[nl] != '\\')
      strcat(mPersonFile,"\\");
  }
  */
  strcat(mPersonFile, mFileName);
  if (pdom)
    *pdom = '@';
}

BOOL GetMailBoxSize(char *user, BOOL bLocal, BOOL bSubLocal) {
  CHAR mMBoxDir[_MAX_PATH], mFileGroup[_MAX_PATH];
  CHAR mTempDir[_MAX_PATH];
  //CHAR *tmp, *p, 
  CHAR *pdom;
  //INT  nl;
  BOOL bFile;
  long               hFindFile;
  struct _finddata_t FindFileData;
  BOOL bBox = TRUE;
  DWORD nSize = 0, nBoxSize;

  //return (DWORD)0;
  if (bLocal && !bSubLocal) {
    nBoxSize = nMailInBoxSize;
    if (nBoxSize != 0) {
	  strcpy(mTempDir, mMailBoxDir);
      pdom = strstr(user, "@");
      if (pdom)
        *pdom = '\x0';
      mMBoxDir[0] = '\x0';
	  /*
      p = mTempDir;          // %HOME%,%USENAME% をチェックする。
      tmp = strstr(p, "%");
      while (tmp) {
        *(tmp) = '\x0';
        strcat(mMBoxDir, p);
        p = tmp+1;
        tmp = strstr(p, "%");
        *(tmp) = '\x0';
	    if (_stricmp(p, "home") == 0) {
          strcat(mMBoxDir, getenv("HOMEDRIVE"));
          strcat(mMBoxDir, getenv("HOMEPATH"));
		} else if (_stricmp(p, "username") == 0)
          strcat(mMBoxDir, user);
        tmp++;
        p = tmp;
        tmp = strstr(p, "%");
	  } 
      strcat(mMBoxDir, p);
	  */
	  GetMailBoxFolder(user, mMBoxDir);  // 2000.06.17 %HOME%が使えるように
	  /*
      nl = strlen(mMBoxDir);
      if (nl != 0) {
        nl--;
        if (mMBoxDir[nl] != '\\')
          strcat(mMBoxDir,"\\");
	  }
	  */
      sprintf(mFileGroup, "%s*.msg", mMBoxDir);
     hFindFile = _findfirst(mFileGroup, &FindFileData);// hFindFile = FindFirstFile( mFileGroup, &FindFileData);
	 if (hFindFile != -1L) { //INVALID_HANDLE_VALUE) {
		bFile = TRUE;
        while (bFile) {
	      nSize += FindFileData.size; //nFileSizeLow;
		  bFile = (_findnext(hFindFile, &FindFileData) == 0 ? TRUE : FALSE); //bFile = FindNextFile( hFindFile, &FindFileData);
		}; 
        _findclose(hFindFile); //FindClose( hFindFile ); 
	  }
      if (pdom)
        *pdom = '@';
      if (nBoxSize < nSize)
        bBox = FALSE;
	}
  }
  return bBox;
}

BOOL MLUserCheck(char *mFrom, char *mRcpt) {
  HKEY hKeyRoot = HKEY_LOCAL_MACHINE;
  HKEY   hKey;
#ifdef UPDATE_20071205 // メールアドレス長が255Byteのときの対策
  CHAR  mkey[MAX_REG_RCPT], *pname;
#else
  CHAR  mkey[256], *pname;
#endif
  BOOL  sts = FALSE;
  DWORD  cbValueName = MAX_VALUE_NAME;
  DWORD  retCode;
  DWORD  dwIndex = 0;	         // index of subkey to enumerate 
  CHAR   mName[_MAX_PATH];	     // address of buffer for subkey name 
  DWORD  lpcbName = 0;
  int    i = 0;
#ifdef REGTOFILE
  HANDLE hFile;
#endif
#ifdef UPDATE_20070521 // 予約語対策
   char *q;
   char mUID[512];
#endif

#ifdef UPDATE_20060616 // エイリアスの場合実アカウントに変換されてしまう。
  char  *pdom, mAliase[256];

  strcpy(mAliase, mFrom);
  pdom = strstr(mAliase, "@");
  if (pdom)
    *pdom = '\x0';
#ifdef UPDATE_20070509 // エイリアス設定の実アドレスオプションを別データとして処理する
  if (GetAliases((LPCTSTR)SOFT_ALIASES_REG, (char *)mAliase, (char *)pdom, NULL))
#else
  if (GetAliases((LPCTSTR)SOFT_ALIASES_REG, (char *)mAliase, (char *)pdom))
#endif
  {
    printf("\t-> aliases = %s\n", mAliase);
  } else {
    if (pdom)
      *pdom = '@';
  }
#endif

   sprintf(mkey,"%s\\%s", SOFT_LISTS_REG, mRcpt);
#ifdef UPDATE_20070521 // 予約語対策
     strcpy(mUID, mRcpt);
     strtok(mUID, "@");
     if (ReservedWords(mUID)) {
       if ((q = strstr(mRcpt, mUID))) {
         strcpy(mUID, mReservedWords);
         strcat(mUID, q);
         sprintf(mkey,"%s\\%s", SOFT_LISTS_REG, mUID);
	   }
	 }
#endif
#ifdef MULTI_ML
   if (!QueryMLists((LPCTSTR)SOFT_LISTS_REG, mRcpt)) // ドメイン付MLが無い場合
     strtok(mkey,"@");
#else
   strtok(mkey,"@");
#endif
   strcat(mkey,"\\Members");
  // OPEN THE KEY.
   do {
	 if (i > 0)
	  _sleep(0);
#ifdef REGTOFILE
	 if (nClustering && !strnicmp(mkey, "software\\emwac", 14)) {
	   strcat(mkey, "\\");
       retCode = 
         OpenKeyFile(mMailSpoolDir,
		             mkey,
					 "",
					 &hFile);

	 } else {
#endif
     retCode = RegOpenKeyEx(hKeyRoot,    // Key handle at root level.
                          (LPCTSTR)mkey,     // Path name of child key.
                          0,           // Reserved.
                          KEY_READ, // Requesting read access.
                          &hKey);      // Address of key to be returned.
#ifdef REGTOFILE
	 }
#endif
  } while(retCode == ERROR_INVALID_HANDLE && i++ < 3);  // ERROR_INVALID_HANDLE = 6

   if (retCode == ERROR_SUCCESS) {
     // READ THE KEY DATA.
     do {
#ifdef REGTOFILE
	 if (nClustering && !strnicmp(mkey, "software\\emwac", 14)) {
       retCode =
		 KeyFileEnumKey(mMailSpoolDir,
		                mkey,
						dwIndex, 
						(LPTSTR)mName, 
						(unsigned long *)&lpcbName);
	 } else
#endif
	 {
       retCode =
        RegEnumKey(hKey,	        // handle of key to query 
                   dwIndex,	    // index of subkey to query 
                   (LPTSTR)mName, // address of buffer for subkey name  
                   (unsigned long)&lpcbName 	    // size of subkey buffer 
        );
	 }
	   if (retCode == ERROR_SUCCESS) {
		  pname = strstr(mName, "<");
		  if (pname)
			pname++;
		  else
			pname = mName;
#ifdef UPDATE_20070521 // 予約語対策
		if (nClustering && !strnicmp(pname, mReservedWords, strlen(mReservedWords))) {
          pname = pname+strlen(mReservedWords);
		}
#endif

#ifdef UPDATE_20060616 // エイリアスの場合実アカウントに変換されてしまう。
		  if ( (mFrom[0] && _strnicmp(pname, mFrom, strlen(mFrom)) == 0) ||
		       (mAliase[0] && _strnicmp(pname, mAliase, strlen(mAliase)) == 0) )
#else
#ifdef UPDATE_20040522
		  if (mFrom[0] && _strnicmp(pname, mFrom, strlen(mFrom)) == 0)
#else
		  if (_strnicmp(pname, mFrom, strlen(mFrom)) == 0)
#endif
#endif
		  {
            sts = TRUE;
			break;
		  }
          dwIndex++;
	   }
	 } while (retCode == ERROR_SUCCESS);
   }
   //RegFlushKey(hKey);
#ifdef REGTOFILE
   if (nClustering && !strnicmp(mkey, "software\\emwac", 14))
 	 CloseHandle(hFile);
   else
#endif
    RegCloseKey (hKey);
   return sts;
}

BOOL ListsCheck(char *mFrom, char *mRcpt, char *mMlTkn, BOOL bCount) {
#ifdef UPDATE_20071205 // メールアドレス長が255Byteのときの対策
   CHAR  mkey[MAX_REG_RCPT], mtkn[64], mModerator[_MAX_PATH], *p;
#else
   CHAR  mkey[256], mtkn[64], mModerator[_MAX_PATH], *p;
#endif
   int   i, nketa;
   char  *pdom;
   DWORD nWhoCanSend, nMlId, nMax;
   BOOL  bsts = FALSE, bAliases = FALSE;
#ifdef UPDATE_20070521 // 予約語対策
   char *q;
   char mUID[512];
#endif

   pdom = strstr(mRcpt, "@");
   if (pdom)
	 *pdom = '\x0';
   //bAliases = GetAliases((LPCTSTR)"SOFTWARE\\EMWAC\\IMS\\Aliases", (char *)mRcpt, (char *)pdom);
#ifdef UPDATE_20070509 // エイリアス設定の実アドレスオプションを別データとして処理する
   bAliases = GetAliases((LPCTSTR) SOFT_ALIASES_REG, (char *)mRcpt, (char *)pdom, NULL);
#else
   bAliases = GetAliases((LPCTSTR) SOFT_ALIASES_REG, (char *)mRcpt, (char *)pdom);
#endif
   if (pdom)
	 *pdom = '@';
   if (!bAliases) {
     sprintf(mkey,"%s\\%s", SOFT_LISTS_REG, mRcpt);
#ifdef UPDATE_20070521 // 予約語対策
     strcpy(mUID, mRcpt);
     strtok(mUID, "@");
     if (ReservedWords(mUID)) {
       if ((q = strstr(mRcpt, mUID))) {
         strcpy(mUID, mReservedWords);
         strcat(mUID, q);
         sprintf(mkey,"%s\\%s", SOFT_LISTS_REG, mUID);
	   }
	 }
#endif
#ifdef MULTI_ML
     if (!QueryMLists((LPCTSTR)SOFT_LISTS_REG, mRcpt)) // ドメイン付MLが無い場合
       strtok(mkey,"@");
#else
     strtok(mkey,"@");
#endif
     ////////////////////////////////////////////////////////////
     // WhoCanSend 0x1 = Member, 0x2 = Anyone, 0x3 = Moderator //
     ////////////////////////////////////////////////////////////
     nWhoCanSend = GetProfileIntEx(mkey, "WhoCanSend", (INT)1); // Default Member
     switch(nWhoCanSend) {
	    case 1: if (MLUserCheck(mFrom, mRcpt))
	 			  bsts = TRUE;
		        break;
	    case 2: bsts = TRUE;
		        break;
	    case 3: GetProfileStringEx(mkey,"Moderator", "", mModerator, sizeof(mModerator));
		        if (_stricmp(mFrom, mModerator) == 0)
				  bsts = TRUE;
		       break;
	 }
     if (bsts && bCount) {
       nMlId = GetProfileIntEx(mkey, "MlId", (INT)0) + 1;
	   GetProfileStringEx(mkey,"MlToken", "", mtkn, sizeof(mtkn));
	   nketa = 4; // デフォルト４桁
       p = strstr(mtkn, "%");
	   if (p) {
         nketa = atoi(p+1);
	   }
	   nMax = 1;
	   for (i = 1; i <= nketa; i++)
	     nMax *= 10;
       if (nMlId >= nMax)
	     nMlId = 0;
	   sprintf(mMlTkn, mtkn, nMlId);
	   WriteProfileIntEx(mkey, "MlId", (DWORD)nMlId);
	 }
   }
   return bsts;
}

#ifdef UPDATE_20070509 // エイリアス設定の実アドレスオプションを別データとして処理する
BOOL GetAliases(LPCTSTR lpAppName, char *uid, char *dom, char *pOpt)
#else
BOOL GetAliases(LPCTSTR lpAppName, char *uid, char *dom)
#endif
{
  HKEY hKeyRoot = HKEY_LOCAL_MACHINE;
  HKEY hKey;
#ifdef UPDATE_20071205 // メールアドレス長が255Byteのときの対策
  CHAR   mkey[MAX_REG_RCPT];
#else
  CHAR   mkey[256];
#endif
  DWORD  cbValueName = MAX_VALUE_NAME;
  DWORD  retCode;
  DWORD  dwIndex = 0;	         // index of subkey to enumerate 
  CHAR   lpName[_MAX_PATH];	 // address of buffer for subkey name 
  DWORD  lpcbName = 0;
  CHAR   mAliasKey[_MAX_PATH];
  CHAR   mAlias[_MAX_PATH];
  CHAR   *pAlias, *pAdom, *pMapto;
  CHAR   mMappedTo[_MAX_PATH], mwork[_MAX_PATH];
  BOOL   sts = FALSE;
  DWORD  stsDom = 0, stsUid = 1, nLeft, nRight, nUid;
  int    i = 0;
#ifdef REGTOFILE
  HANDLE hFile, hF = NULL;
#endif

  // OPEN THE KEY.
  sprintf(mkey,PROFILE_ROOT_TREE,lpAppName);
   do {
	 if (i > 0)
	  _sleep(0);
#ifdef REGTOFILE
	 if (nClustering && !strnicmp(mkey, "software\\emwac", 14)) {
       retCode = 
         OpenKeyFile(mMailSpoolDir,
		             mkey,
					 "",
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
  // READ THE KEY DATA.
    do {
#ifdef REGTOFILE
	 if (nClustering && !strnicmp(mkey, "software\\emwac", 14)) {
       retCode =
		 KeyFileEnumKeyEx(&hF,
		                mMailSpoolDir,
		                mkey,
						//dwIndex, 
						(LPTSTR)lpName, 
						(unsigned long *)&lpcbName);
	 } else
#endif
	 {
      retCode =
          RegEnumKey(hKey,	        // handle of key to query 
                     dwIndex,	    // index of subkey to query 
                     (LPTSTR)lpName, // address of buffer for subkey name  
                     (unsigned long)&lpcbName 	    // size of subkey buffer 
         );
	 }
      if (retCode == ERROR_SUCCESS) {
		//sprintf(mAliasKey, "SOFTWARE\\EMWAC\\IMS\\Aliases\\%s", lpName);
		sprintf(mAliasKey, "%s\\%s", SOFT_ALIASES_REG, lpName);
        GetProfileStringEx(mAliasKey,"Alias", "", mAlias, sizeof(mAlias));
		_strlwr(mAlias);
		//////////////////////////////////////
		/////  Doamin Check
		pAdom = strstr(mAlias, "@");
	    stsDom = 1;
		if (pAdom)
          *pAdom = '\x0';
		if (dom && pAdom) {                // Domain check
		  stsDom = _stricmp(dom+1, pAdom+1);
		} else if (!dom && !pAdom) {
		  stsDom = 0;
		}
		//////////////////////////////////////
		/////  User Check
		pAlias = strstr(mAlias,"*");
		if (pAlias) {
          *pAlias = '\x0';
          /* if (*(pAlias+1) == '\x0')
			stsUid = 0;
		  else */ {
             nLeft  = strlen(mAlias);
			 stsUid = _strnicmp(mAlias, uid, nLeft);
             nRight = strlen(pAlias+1);
			 if (nRight) {
			   nUid   = strlen(uid);
			   if (nUid < nLeft+nRight) {
 			     stsUid = -1;
			   } else {
			     if (stsUid || _strnicmp(pAlias+1, &uid[nUid-nRight], nRight))
				   stsUid = -1;
			   }
			 }
			 
		  }
		} else {
		  stsUid = _stricmp(mAlias, uid);
		}
		//////////////////////////////////////
        if (stsDom == 0 && stsUid == 0) {
		  sts = TRUE;
          GetProfileStringEx(mAliasKey,"MappedTo", "", mMappedTo, sizeof(mMappedTo));
#ifdef UPDATE_20070509 // エイリアス設定の実アドレスオプションを別データとして処理する
		  if (pOpt) {  // NULLでないなら
			if ((pMapto = strchr(mMappedTo, ','))) {
			  *pMapto = '\x0';  // オプション削除
			  pMapto++;         // オプション情報
			  strcpy(pOpt, pMapto);
			}
		  } else {
			strtok(mMappedTo, ","); // オプション削除
		  }
#endif
          pMapto = strstr(mMappedTo,"*");
		  if (pMapto) {
	        strcpy(mwork, &uid[strlen(mAlias)]);
			*pMapto = '\x0';
			//sprintf(uid, "%s%s%s", mMappedTo, uid, pMapto+1);
			sprintf(uid, "%s%s%s", mMappedTo, mwork, pMapto+1);
		  } else
	        strcpy(uid, mMappedTo);
		  break;
		}
	  }
      dwIndex++;
	} while (retCode == ERROR_SUCCESS);
#ifdef REGTOFILE
    if (nClustering && !strnicmp(mkey, "software\\emwac", 14)) {
	  if (hF)
	    FindClose(hF); 
	  CloseHandle(hFile);
    } else
#endif
    RegCloseKey(hKey);
  }
  return sts;
}

BOOL GetPostMaster(char *uid) {
  CHAR   mMappedTo[_MAX_PATH];
  BOOL   sts = FALSE;

  // OPEN THE KEY.
  if (_stricmp(uid,"postmaster") == 0) {
    //GetProfileStringEx("SOFTWARE\\EMWAC\\IMS","PostMaster", "", mMappedTo, sizeof(mMappedTo));
    GetProfileStringEx(SOFT_REG ,"PostMaster", "administrator", mMappedTo, sizeof(mMappedTo));
    if (mMappedTo[0] == '\x0')
 	  strcpy(mMappedTo, "administrator");
    if (strlen(mMappedTo) > 0) {
	  //strcpy(uid, mMappedTo);
      sts = TRUE;
	}
  }
  return sts;
}

BOOL QueryMLists(LPCTSTR lpAppName, char *uid) {
  HKEY hKeyRoot = HKEY_LOCAL_MACHINE;
  HKEY hKey;
#ifdef UPDATE_20071205 // メールアドレス長が255Byteのときの対策
  CHAR   mkey[MAX_REG_RCPT];
#else
  CHAR   mkey[256];
#endif
  DWORD  retCode;
  BOOL   sts = FALSE;
  CHAR   mName[_MAX_PATH];	 // address of buffer for subkey name 
  char   *p, *q;
  int    i = 0;
#ifdef REGTOFILE
  HANDLE hFile;
#endif
#ifdef UPDATE_20091222A // 異常に長いメールアドレスの処理でハングする可能性
  int   l, n;
#endif

  // OPEN THE KEY.
  strcpy(mName, uid);
  sprintf(mkey,PROFILE_ROOT_TREE,lpAppName);
  if ((p =strstr(mName, "-request"))) {
	*p = '\x0';
	strcat(mkey, mName);
	p++;
    if ((q = strstr(p, "@")) ) {
 	  strcat(mkey, q);
	}
  } else {
#ifdef UPDATE_20091222A // 異常に長いメールアドレスの処理でハングする可能性
	l = strlen(mkey);
	n = l + strlen(uid);
	if (n >= MAX_REG_RCPT)
	{
      strncpy(&mkey[l], uid, MAX_REG_RCPT-l);
	  mkey[MAX_REG_RCPT-1] = '\x0';
	} else 
#endif
	{
      strcat(mkey, uid);
	}
  }
  if (bDebug)
    printf("Get Mailing List = %s\n", mkey);
   do {
	 if (i > 0)
	  _sleep(0);
#ifdef REGTOFILE
	 if (nClustering && !strnicmp(mkey, "software\\emwac", 14)) {
       retCode = 
         OpenKeyFile(mMailSpoolDir,
		             mkey,
					 "",
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
#ifdef REGTOFILE
    if (nClustering && !strnicmp(mkey, "software\\emwac", 14))
	  CloseHandle(hFile);
    else
#endif
    RegCloseKey(hKey);
	sts = TRUE;
  }

  return sts;
}

BOOL GetMLists(LPCTSTR lpAppName, char *uid, BOOL *bRequest) {
  HKEY hKeyRoot = HKEY_LOCAL_MACHINE;
  HKEY hKey;
#ifdef UPDATE_20071205 // メールアドレス長が255Byteのときの対策
  CHAR   mkey[MAX_REG_RCPT];
#else
  CHAR   mkey[256];
#endif
  DWORD  cbValueName = MAX_VALUE_NAME;
  DWORD  retCode;
  DWORD  dwIndex = 0;	         // index of subkey to enumerate 
  CHAR   mName[_MAX_PATH];	 // address of buffer for subkey name 
  CHAR   mRequestName[_MAX_PATH];	// address of buffer for subkey name 
  DWORD  lpcbName = 0;
  BOOL   sts = FALSE;
  char   *p;
  int    i = 0;
#ifdef REGTOFILE
  HANDLE hFile, hF = NULL;
#endif
#ifdef UPDATE_20070521 // 予約語対策
  char *q, mUID[512];
  BOOL bReservedWords = FALSE;

  if (nClustering){
	strcpy(mUID, uid);
    strtok(mUID, "@");
    if (ReservedWords(mUID)) {
      if ((q = strstr(uid, mUID))) {
        strcpy(mUID, mReservedWords);
        strcat(mUID, q);
  	    bReservedWords = TRUE;
	  }
	}
  }
#endif
  
  // OPEN THE KEY.
  sprintf(mkey,PROFILE_ROOT_TREE,lpAppName);
  do {
	if (i > 0)
	  _sleep(0);
#ifdef REGTOFILE
	 if (nClustering && !strnicmp(mkey, "software\\emwac", 14)) {
       retCode = 
         OpenKeyFile(mMailSpoolDir,
		             mkey,
					 "",
					 &hFile);

	 } else
#endif
	 {
    retCode = RegOpenKeyEx(hKeyRoot, (LPCTSTR)mkey,0, KEY_READ, &hKey);
	 }
  } while(retCode == ERROR_INVALID_HANDLE && i++ < 3);  // ERROR_INVALID_HANDLE = 6
  if (retCode != ERROR_SUCCESS) { // リトライ1
    do {
	  if (i > 0)
	    _sleep(0);
#ifdef REGTOFILE
	 if (nClustering && !strnicmp(mkey, "software\\emwac", 14)) {
       retCode = 
         OpenKeyFile(mMailSpoolDir,
		             mkey,
					 "",
					 &hFile);

	 } else
#endif
	 {
      retCode = RegOpenKeyEx(hKeyRoot, (LPCTSTR)mkey,0, KEY_READ, &hKey);
	 }
	} while(retCode == ERROR_INVALID_HANDLE && i++ < 3);  // ERROR_INVALID_HANDLE = 6
    if (retCode != ERROR_SUCCESS) { // リトライ2
      do {
	    if (i > 0)
	      _sleep(0);
#ifdef REGTOFILE
	 if (nClustering && !strnicmp(mkey, "software\\emwac", 14)) {
       retCode = 
         OpenKeyFile(mMailSpoolDir,
		             mkey,
					 "",
					 &hFile);

	 } else
#endif
	 {
        retCode = RegOpenKeyEx(hKeyRoot, (LPCTSTR)mkey,0, KEY_READ, &hKey);
	 }
	  } while(retCode == ERROR_INVALID_HANDLE && i++ < 3);  // ERROR_INVALID_HANDLE = 6
	}
  }
  if (retCode == ERROR_SUCCESS) {
  // READ THE KEY DATA.
    do {
#ifdef REGTOFILE
	 if (nClustering && !strnicmp(mkey, "software\\emwac", 14)) {
       retCode =
		 KeyFileEnumKeyEx(&hF,
		                mMailSpoolDir,
		                mkey,
						//dwIndex, 
						(LPTSTR)mName, 
						(unsigned long *)&lpcbName);
	 } else
#endif
	 {
      retCode =
          RegEnumKey(hKey,	        // handle of key to query 
                     dwIndex,	    // index of subkey to query 
                     (LPTSTR)mName, // address of buffer for subkey name  
                     (unsigned long)&lpcbName 	    // size of subkey buffer 
         );
	 }
      if (retCode == ERROR_SUCCESS) {
#ifdef MULTI_ML
		// 2002.09.04 ドメイン付なら@マーク以降を削除
		strcpy(mRequestName, mName);
#ifdef UPDATE_20070521 // 予約語対策
		if (!strnicmp(mName, mReservedWords, strlen(mReservedWords))) {
          strtok(&mRequestName[strlen(mReservedWords)], "@");
		  strcat(mRequestName, "-request");
		  p = strstr(&mName[strlen(mReservedWords)], "@");
		  if (p) {
		    strcat(mRequestName, p);
		  }
		} else 
#endif
		{
		  strtok(mRequestName, "@");
		  strcat(mRequestName, "-request");
		  p = strstr(mName, "@");
		  if (p) {
		    strcat(mRequestName, p);
		  }
		}
#else
	    sprintf(mRequestName,"%s-request", mName);
#endif
#ifdef UPDATE_20070521 // 予約語対策
		if (bReservedWords) {
          if (_stricmp(mName, mUID) == 0 ||
		      _stricmp(mRequestName, mUID) == 0) {
		    sts = TRUE;
		    if (_stricmp(mRequestName, mUID) == 0)
			  *bRequest = TRUE;
		    break;
		  }
        } else
#endif
		{
          if (_stricmp(mName, uid) == 0 ||
		      _stricmp(mRequestName, uid) == 0) {
		    sts = TRUE;
		    if (bRequest && _stricmp(mRequestName, uid) == 0)
			  *bRequest = TRUE;
		    break;
		  }
		}
	  }
      dwIndex++;
	} while (retCode == ERROR_SUCCESS);
#ifdef REGTOFILE
    if (nClustering && !strnicmp(mkey, "software\\emwac", 14)) {
	  if (hF)
	    FindClose(hF); 
	  CloseHandle(hFile);
	} else
#endif
    RegCloseKey(hKey);
  }
  return sts;
}

DWORD SetListsUser(LPCTSTR lpAppName, struct _RCP *rcp, char *mRCP, DWORD nMLDomDiv, DWORD nNoOfDiv) {
#ifdef TRACE_MODE
   char str[LOG_BUFFER];
#endif
  HKEY hKeyRoot = HKEY_LOCAL_MACHINE;
  HKEY hKey;
#ifdef UPDATE_20071205 // メールアドレス長が255Byteのときの対策
  CHAR   mkey[MAX_REG_RCPT];
#else
  CHAR   mkey[256];
#endif
  DWORD  cbValueName = MAX_VALUE_NAME;
  DWORD  retCode;
  DWORD  dwIndex = 0;	         // index of subkey to enumerate 
  CHAR   mName[_MAX_PATH];	 // address of buffer for subkey name 
  DWORD  lpcbName = 0;
  BOOL   bsts = FALSE; //sts = FALSE;
  DWORD  sts = 0;
  char   *pdom;
  char   mRCPDest[_MAX_PATH];
  DWORD  nDiv, nListNo = 0;
  long               hF;
  struct _finddata_t FD;
  //ULARGE_INTEGER  u0, u1, u2;
  time_t  u0 = 0, u1 = 0, u2 = 0;
  int     i = 0;
#ifdef REGTOFILE
  HANDLE hFile;
#endif
#ifdef UPDATE_20070521 // 予約語対策
  char *p, *q, mKey[512];
#endif
#ifdef UPDATE_20071210A // ML参加アドレスを含む256Byte超のフォルダ名になったと場合の対策
  FILE *fp;
  char mMLFn[256];
#endif

  // OPEN THE KEY.
#ifndef V4
#ifdef TRACE_MODE
if (nSenderLog) {
  sprintf(str, "%s:Start SetListsUser:\n", rcp->mMNo);
  printfTrace(str);
}
#endif
#endif
  sprintf(mkey,PROFILE_ROOT_TREE,lpAppName);
  do {
	if (i > 0)
	  _sleep(0);
#ifdef REGTOFILE
	 if (nClustering && !strnicmp(mkey, "software\\emwac", 14)) {
       retCode = 
         OpenKeyFile(mMailSpoolDir,
		             mkey,
					 "",
					 &hFile);

	 } else
#endif
	 {
    retCode = RegOpenKeyEx(hKeyRoot, (LPCTSTR)mkey,0, KEY_READ, &hKey);
	 }
  } while(retCode == ERROR_INVALID_HANDLE && i++ < 3);  // ERROR_INVALID_HANDLE = 6
  if (retCode != ERROR_SUCCESS) { // リトライ1
    do {
	  if (i > 0)
	    _sleep(0);
#ifdef REGTOFILE
	 if (nClustering && !strnicmp(mkey, "software\\emwac", 14)) {
       retCode = 
         OpenKeyFile(mMailSpoolDir,
		             mkey,
					 "",
					 &hFile);

	 } else
#endif
	 {
      retCode = RegOpenKeyEx(hKeyRoot, (LPCTSTR)mkey,0, KEY_READ, &hKey);
	 }
	} while(retCode == ERROR_INVALID_HANDLE && i++ < 3);  // ERROR_INVALID_HANDLE = 6
    if (retCode != ERROR_SUCCESS) { // リトライ2
      do {
	    if (i > 0)
	      _sleep(0);
#ifdef REGTOFILE
	 if (nClustering && !strnicmp(mkey, "software\\emwac", 14)) {
       retCode = 
         OpenKeyFile(mMailSpoolDir,
		             mkey,
					 "",
					 &hFile);

	 } else
#endif
	 {
        retCode = RegOpenKeyEx(hKeyRoot, (LPCTSTR)mkey,0, KEY_READ, &hKey);
	 }
	  } while(retCode == ERROR_INVALID_HANDLE && i++ < 3);  // ERROR_INVALID_HANDLE = 6
	}
  }
#ifndef V4
#ifdef TRACE_MODE
if (nSenderLog) {
  sprintf(str, "%s:SetListsUser->RegOpenKeyEx:retCode=%ld\n", rcp->mMNo, retCode);
  printfTrace(str);
}
#endif
#endif
  if (retCode == ERROR_SUCCESS) {
#ifdef UPDATE_20071210A // ML参加アドレスを含む256Byte超のフォルダ名になったと場合の対策
    sprintf(mMLFn, "%sreg\\%sext.dat", mMailSpoolDir, mkey);
	fp = fopen(mMLFn, "rt");
#endif
  // READ THE KEY DATA.
    do {
	  mName[0] = '\x0';
#ifdef REGTOFILE
	 if (nClustering && !strnicmp(mkey, "software\\emwac", 14)) {
       retCode =
		 KeyFileEnumKey(mMailSpoolDir,
		                mkey,
						dwIndex, 
						(LPTSTR)mName, 
						(unsigned long *)&lpcbName);
#ifdef UPDATE_20071210A // ML参加アドレスを含む256Byte超のフォルダ名になったと場合の対策
	    if (fp && retCode != ERROR_SUCCESS) {
		  mName[0] = '\x0';
		  fgets(mName, sizeof(mName), fp);
          if (mName[0] && mName[0] != '\r' && mName[0] != '\n') {
			strtok(mName, "\r\n");
		    retCode = ERROR_SUCCESS;
		  }
		}
#endif
#ifdef UPDATE_20070521 // 予約語対策
        if (retCode == ERROR_SUCCESS) {
          strcpy(mKey, mName);
          strtok(mKey, "@");
          if (!strnicmp(mKey, mReservedWords, strlen(mReservedWords))
		      && ReservedWords(&mKey[strlen(mReservedWords)])) {
            if ((q = strchr(mName, '@'))) {
	          strcpy(mKey, mName+strlen(mReservedWords));
		      strcpy(mName, mKey);
			}
		  }
		}
#endif
	 } else
#endif
	 {
      retCode =
          RegEnumKey(hKey,	        // handle of key to query 
                     dwIndex,	    // index of subkey to query 
                     (LPTSTR)mName, // address of buffer for subkey name  
                     (unsigned long)&lpcbName 	    // size of subkey buffer 
         );
	 }
#ifndef V4
#ifdef TRACE_MODE
if (nSenderLog) {
  sprintf(str, "%s:[%8d] SetListsUser->RegEnumKey:retCode=%ld, mName=%s\n", rcp->mMNo, dwIndex, retCode, mName);
  printfTrace(str);
}
#endif
#endif
      if (retCode == ERROR_SUCCESS) {
		if (bDebug)
	      printf("Check To: address = %s\n", mName);
	    pdom = strstr(mName, "@");
	    if (pdom)
	      *pdom = '\x0';
#ifdef UPDATE_20070509 // エイリアス設定の実アドレスオプションを別データとして処理する
        bsts = GetAliases((LPCTSTR)SOFT_ALIASES_REG, (char *)mName, (char *)pdom, NULL);
#else
        bsts = GetAliases((LPCTSTR)SOFT_ALIASES_REG, (char *)mName, (char *)pdom);
#endif
	    if (bsts) {
		  if (bDebug)
            printf("\t-> aliases = %s\n", mName);
		} else {
	     if (pdom)
	       *pdom = '@';
		}
#ifdef UPDATE_20070520 // ＭＬメンバでワイルドカード指定が出来ないため、"@"をワイルドカードに流用
		if (mName[0] == ' ' && mName[1] == '@') {  // 全ユーザー指定の場合
		  mName[0] = '*';
		}
#endif

#ifdef ML_ASTER_OPTION
		if (strchr(mName,'*')) {  // 全ユーザー指定の場合
		  if (pdom)  // ドメインが指定されていること
		    sts = SetListsUserAster(mName, rcp, mRCP, nMLDomDiv, nNoOfDiv, &nListNo);
		} else {
#endif
		strcpy(rcp->mTo, mName);
		if (nMLDomDiv == 0) { // 強制分割で無い場合はドメイン分割
	      sprintf(mRCPDest, mRCP, (pdom ? pdom : "@localhost"), nNoOfDiv); // 配送先ドメインで分離
		  for (nDiv = 0; nDiv < nNoOfDiv; nDiv++) { // 最大100分割まで
		    sprintf(mRCPDest, mRCP, (pdom ? pdom : "@localhost"), nDiv); 
	        hF = _findfirst(mRCPDest, &FD); //hF = FindFirstFile( mRCPDest, &FD);
            if (hF == -1L) { //INVALID_HANDLE_VALUE) {
			  break; // ファイルが無いなら保存先決定
			}
            u2  = FD.time_write;  //ftLastWriteTime.dwLowDateTime;
            //u2.HighPart = 0; //FD.time_write.dwHighDateTime; //ftLastWriteTime.dwHighDateTime;
            _findclose(hF); //FindClose( hF ); 
			if (nDiv == nNoOfDiv-1) {
			  if (u0 > u2) {
			    break;
			  } else {
		        sprintf(mRCPDest, mRCP, (pdom ? pdom : "@localhost"), 0); // トップに戻る
			  }
			} else {
			  if (nDiv > 0 && u1 > u2)
			    break;
			}
			if (nDiv == 0) // トップの時間バックアップ
			  u0 = u1 = u2;
			else
			  u1 = u2;
		  }
///////////////// 入替え 2003.3.3
#ifndef V4
#ifdef TRACE_MODE
          if (nSenderLog) {
            sprintf(str, "%s:SetListsUser->SetRCPFile:%s\n", rcp->mMNo, mRCPDest);
            printfTrace(str);
		  }
#endif
#endif
          SetRCPFile(rcp, mRCPDest, FALSE, &nListNo);
		  sts = nListNo;
#ifndef V4
#ifdef TRACE_MODE
          if (nSenderLog) {
            sprintf(str, "%s:SetListsUser->SetRCPFile Complete.%d:%s\n", rcp->mMNo, sts, mRCPDest);
            printfTrace(str);
		  }
#endif
#endif
		} else {
		  sprintf(mRCPDest, mRCP, (nListNo%nMLDomDiv+1));
		  nListNo++;
#ifndef V4
#ifdef TRACE_MODE
          if (nSenderLog) {
            sprintf(str, "%s:SetListsUser->SetRCPFile:%s\n", rcp->mMNo, mRCPDest);
            printfTrace(str);
		  }
#endif
#endif
          SetRCPFile(rcp, mRCPDest, FALSE, NULL);
		  sts = nMLDomDiv;
		  if (nListNo < (DWORD) nMLDomDiv)
		    sts = nListNo;
		}
#ifdef ML_ASTER_OPTION
		}
#endif
	  }
      dwIndex++;
	} while (retCode == ERROR_SUCCESS);
#ifdef UPDATE_20071210A // ML参加アドレスを含む256Byte超のフォルダ名になったと場合の対策
	if (fp) {
      fclose(fp);
	}
#endif
#ifdef REGTOFILE
    if (nClustering && !strnicmp(mkey, "software\\emwac", 14))
	  CloseHandle(hFile);
    else
#endif
    RegCloseKey(hKey);
  }
#ifndef V4
#ifdef TRACE_MODE
if (nSenderLog) {
  sprintf(str, "%s:End SetListsUser:sts=%d\n", rcp->mMNo, sts);
  printfTrace(str);
}
#endif
#endif
  return sts;
}

#ifdef ML_ASTER_OPTION
DWORD SetListsUserAster(char *pName, struct _RCP *rcp, char *mRCP, DWORD nMLDomDiv, DWORD nNoOfDiv, DWORD *nListNo) {
   char *pDom, mName[256];
   DWORD sts = 0;

   if ((pDom = strchr(pName, '@'))) {
	 *pDom = '\x0';
	 strcpy(mName, pName);
	 *pDom = '@';
	 pDom++;
#ifdef V3
     if (bUserMan)// Windows管理
#endif
	 {
#ifdef LDAP_ON         // LDAPへのログイン処理でSMTP認証の確認を行うオプション // //CRAM-MD5は複合できないので、これを利用する場合は、PLAINかLOGINのみとします。
	    if (bLDAP) {
          sts = GetLDAPUserList(mName, pDom, rcp, mRCP, nMLDomDiv, nNoOfDiv, nListNo);
		} else
#endif
		{
          sts = GetWinUserList(mName, pDom, rcp, mRCP, nMLDomDiv, nNoOfDiv, nListNo);
		}
	 }
#ifdef V3
	 else {
	   sts = GetSpaUserList(mName, pDom, rcp, mRCP, nMLDomDiv, nNoOfDiv, nListNo);
	 }
#endif
   }
   return sts;
}
#endif

BOOL CheckLists(LPCTSTR lpAppName, char *uid) {
  HKEY hKeyRoot = HKEY_LOCAL_MACHINE;
  HKEY hKey;
#ifdef UPDATE_20071205 // メールアドレス長が255Byteのときの対策
  CHAR   mkey[MAX_REG_RCPT];
#else
  CHAR   mkey[256];
#endif
  DWORD  cbValueName = MAX_VALUE_NAME;
  DWORD  retCode;
  DWORD  dwIndex = 0;	         // index of subkey to enumerate 
  CHAR   mName[_MAX_PATH];	 // address of buffer for subkey name 
  CHAR   mRequestName[_MAX_PATH];	// address of buffer for subkey name 
  DWORD  lpcbName = 0;
  BOOL   sts = FALSE;
  char   muid[256], *p;
  int    i = 0;
#ifdef REGTOFILE
  HANDLE hFile;
#endif
#ifdef UPDATE_20070521 // 予約語対策
  char *pdom, *q, mKEY[512];
  BOOL bReservedWords = FALSE;

  if (nClustering){
	strcpy(mKEY, uid);
    strtok(mKEY, "@");
    if (ReservedWords(mKEY)) {
      if ((q = strstr(uid, mKEY))) {
        strcpy(mKEY, mReservedWords);
        strcat(mKEY, q);
  	    bReservedWords = TRUE;
	  }
	}
  }
#endif

  // OPEN THE KEY.
#ifdef UPDATE_20070521 // 予約語対策
  if (bReservedWords)
    strcpy(muid, mKEY);
  else
#endif
  strcpy(muid, uid);
#ifdef UPDATE_20070521 // 予約語対策
  if (bReservedWords)
    strtok(&muid[strlen(mReservedWords)], "@"); // ドメイン無し
  else
#endif
  strtok(muid, "@"); // ドメイン無し
  sprintf(mkey,PROFILE_ROOT_TREE,lpAppName);
  do {
	if (i > 0)
	  _sleep(0);
#ifdef REGTOFILE
	 if (nClustering && !strnicmp(mkey, "software\\emwac", 14)) {
       retCode = 
         OpenKeyFile(mMailSpoolDir,
		             mkey,
					 "",
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
  // READ THE KEY DATA.
    do {
#ifdef REGTOFILE
	 if (nClustering && !strnicmp(mkey, "software\\emwac", 14)) {
       retCode =
		 KeyFileEnumKey(mMailSpoolDir,
		                mkey,
						dwIndex, 
						(LPTSTR)mName, 
						(unsigned long *)&lpcbName);
	 } else
#endif
	 {
      retCode =
          RegEnumKey(hKey,	        // handle of key to query 
                     dwIndex,	    // index of subkey to query 
                     (LPTSTR)mName, // address of buffer for subkey name  
                     (unsigned long)&lpcbName 	    // size of subkey buffer 
         );
	 }
      if (retCode == ERROR_SUCCESS) {
#ifdef MULTI_ML
		// 2002.09.04 ドメイン付なら@マーク以降を削除
	    strcpy(mRequestName, mName);
#ifdef UPDATE_20070521 // 予約語対策
		if (!strnicmp(mName, mReservedWords, strlen(mReservedWords))) {
          strtok(&mRequestName[strlen(mReservedWords)], "@");
		  strcat(mRequestName, "-request");
		  p = strstr(&mName[strlen(mReservedWords)], "@");
		  if (p) {
		    strcat(mRequestName, p);
		  }
		} else 
#endif
		{
		  strtok(mRequestName, "@");
		  strcat(mRequestName, "-request");
		  p = strstr(mName, "@");
		  if (p) {
		    strcat(mRequestName, p);
		  }
		}
#else
	    sprintf(mRequestName,"%s-request", mName);
#endif
        if (_stricmp(mName, (bReservedWords ? mKEY : uid)) == 0 ||
			_stricmp(mName, muid) == 0) {
		  sts = TRUE;
		  break;
		} else if (_stricmp(mRequestName, (bReservedWords ? mKEY : uid)) == 0 ||
			       _stricmp(mRequestName, muid) == 0 ) {
		//} else if (_strnicmp(mName, uid, strlen(mName)) == 0 &&
		//	       _stricmp(&uid[strlen(mName)], "-request") == 0) {
/*
#ifdef UPDATE_20070521 // 予約語対策
		  if (bReservedWords) {
		    strcpy(uid, &mName[strlen(mReservedWords)]);
		  } else
#endif
*/
		  {
		    strcpy(uid, mName);
		  }
		  sts = TRUE;
		  break;
		}
	  }
      dwIndex++;
	} while (retCode == ERROR_SUCCESS);
#ifdef REGTOFILE
    if (nClustering && !strnicmp(mkey, "software\\emwac", 14))
	  CloseHandle(hFile);
    else
#endif
    RegCloseKey(hKey);
  }
  return sts;
}

DWORD GetProfileIntEx(LPCTSTR lpAppName, LPCTSTR lpKeyName, INT nDefault) {
  HKEY hKeyRoot = HKEY_LOCAL_MACHINE;
  HKEY hKey;
#ifdef UPDATE_20071205 // メールアドレス長が255Byteのときの対策
  CHAR   mkey[MAX_REG_RCPT];
#else
  CHAR   mkey[256];
#endif
  DWORD  cbValueName = MAX_VALUE_NAME;
  DWORD  retCode;
  DWORD  dwIndex = 0;	         // index of subkey to enumerate 
  DWORD  dwType;
  DWORD  nReturned;
  DWORD  nSize = sizeof(DWORD);
  int    i = 0;
#ifdef REGTOFILE
  HANDLE hFile;
#endif
  // READ THE KEY DATA.
  nReturned = (DWORD)nDefault;
  sprintf(mkey,PROFILE_ROOT_TREE,lpAppName);
   do {
	 if (i > 0)
	  _sleep(0);
#ifdef REGTOFILE
    if (nClustering && !strnicmp(lpAppName, "software\\emwac", 14)) {
#ifdef READING_ASYNCHRONOUS
       retCode = 
         OpenKeyFile(mMailSpoolDir,
		             mkey,
					 (char *)lpKeyName,
					 &hFile);
#else
	   retCode = ERROR_SUCCESS;
#endif
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
	dwType = REG_DWORD;
#ifdef REGTOFILE
    if (nClustering && !strnicmp(lpAppName, "software\\emwac", 14)) {
       retCode =
		 KeyFileQueryValueEx(mMailSpoolDir,
		                     mkey,
					         (char *)lpKeyName,
							 &hFile,
							 dwType,
							 (LPBYTE)&nReturned,
							 &nSize);
#ifdef READING_ASYNCHRONOUS
	     CloseHandle(hFile);
#endif
	 } else
#endif
	{
    retCode =
	   RegQueryValueEx(hKey,            // handle of key to query 
                      (LPSTR)lpKeyName, // address of name of value to query 
                      0,                // reserved 
                      &dwType,  // address of buffer for value type 
                      (LPBYTE)&nReturned,  // address of data buffer 
                      &nSize  // address of data buffer size 
                      ); 
    RegCloseKey(hKey);
	 }
	if (retCode) {
      nReturned = (DWORD)nDefault;
	}
  } else 
    nReturned = (DWORD)nDefault;
  return nReturned;
} 

DWORD GetProfileStringEx(LPCTSTR lpAppName, LPCTSTR lpKeyName, LPCTSTR lpDefault, LPTSTR lpReturnedString, DWORD nSize) {
  HKEY hKeyRoot = HKEY_LOCAL_MACHINE;
  HKEY hKey;
#ifdef UPDATE_20071205 // メールアドレス長が255Byteのときの対策
  CHAR   mkey[MAX_REG_RCPT];
#else
  CHAR   mkey[256];
#endif
  DWORD  cbValueName = MAX_VALUE_NAME;
  DWORD  retCode;
  DWORD  dwIndex = 0;	         // index of subkey to enumerate 
  DWORD  dwType;
  int    i = 0;
#ifdef REGTOFILE
  HANDLE hFile;
#endif
  // READ THE KEY DATA.
#ifndef TUNING
  memset(lpReturnedString, 0, nSize);
  strcpy(lpReturnedString, lpDefault);
#else
  *lpReturnedString = '\x0';
#endif
  sprintf(mkey,PROFILE_ROOT_TREE,lpAppName);
   do {
	 if (i > 0)
	  _sleep(0);
#ifdef REGTOFILE
    if (nClustering && !strnicmp(lpAppName, "software\\emwac", 14)) {
#ifdef READING_ASYNCHRONOUS
       retCode = 
         OpenKeyFile(mMailSpoolDir,
		             mkey,
					 (char *)lpKeyName,
					 &hFile);
#else
	   retCode = ERROR_SUCCESS;
#endif
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
    if (nClustering && !strnicmp(lpAppName, "software\\emwac", 14)) {
       retCode =
		 KeyFileQueryValueEx(mMailSpoolDir,
		                     mkey,
					         (char *)lpKeyName,
							 &hFile,
							 dwType,
							 (LPBYTE)lpReturnedString,
							 &nSize);
#ifdef READING_ASYNCHRONOUS
	     CloseHandle(hFile);
#endif
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

DWORD GetProfileBinaryEx(LPCTSTR lpAppName, LPCTSTR lpKeyName, LPCTSTR lpDefault, LPTSTR lpReturnedString, DWORD nSize) {
  HKEY hKeyRoot = HKEY_LOCAL_MACHINE;
  HKEY hKey;
#ifdef UPDATE_20071205 // メールアドレス長が255Byteのときの対策
  CHAR   mkey[MAX_REG_RCPT];
#else
  CHAR   mkey[256];
#endif
  DWORD  cbValueName = MAX_VALUE_NAME;
  DWORD  retCode;
  DWORD  dwIndex = 0;	         // index of subkey to enumerate 
  DWORD  dwType;
  int    i = 0;
#ifdef REGTOFILE
  HANDLE hFile;
#endif
  // READ THE KEY DATA.
#ifndef TUNING
  memset(lpReturnedString, 0, nSize);
  strcpy(lpReturnedString, lpDefault);
#else
  *lpReturnedString = '\x0';
#endif
  sprintf(mkey,PROFILE_ROOT_TREE,lpAppName);
   do {
	 if (i > 0)
	  _sleep(0);
#ifdef REGTOFILE
    if (nClustering && !strnicmp(lpAppName, "software\\emwac", 14)) {
#ifdef READING_ASYNCHRONOUS
       retCode = 
         OpenKeyFile(mMailSpoolDir,
		             mkey,
					 (char *)lpKeyName,
					 &hFile);
#else
	   retCode = ERROR_SUCCESS;
#endif
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
	dwType = REG_BINARY;
#ifdef REGTOFILE
    if (nClustering && !strnicmp(lpAppName, "software\\emwac", 14)) {
       retCode =
		 KeyFileQueryValueEx(mMailSpoolDir,
		                     mkey,
					         (char *)lpKeyName,
							 &hFile,
							 dwType,
							 (LPBYTE)lpReturnedString,
							 &nSize);
#ifdef READING_ASYNCHRONOUS
	     CloseHandle(hFile);
#endif
	 } else
#endif
	{
    retCode =
	     RegQueryValueEx(hKey,            // handle of key to query 
                      (LPSTR)lpKeyName, // address of name of value to query 
                      0,                // reserved 
                      &dwType,  // address of buffer for value type 
                      lpReturnedString,  // address of data buffer 
                      &nSize  // address of data buffer size 
                      ); 
    RegCloseKey(hKey);
	 }
	if (retCode != ERROR_SUCCESS)
#ifndef TUNING
      strcpy(lpReturnedString, "");
#else
	  *lpReturnedString = '\x0';
#endif
  } else
    strcpy(lpReturnedString, lpDefault);
  return nSize;
}

void WriteProfileIntEx(CHAR * KeyPath, CHAR * ValuePath, DWORD ValueInt) {
  HKEY   hKeyRoot = HKEY_LOCAL_MACHINE;
  HKEY   hKey;
  DWORD  retCode;
  int    mach = 1;
  DWORD  cbValueName = 80;
  DWORD  dwIndex = 0;	         // index of subkey to enumerate 
  DWORD  lpData = 0;
  DWORD  cbData = 80;
#ifdef UPDATE_20110909 // ML名+ドメイン名が53byteを超えるとハングする
  CHAR   RegPath[512];
#else
  CHAR   RegPath[80];
#endif
  int    i = 0;
#ifdef REGTOFILE
  HANDLE hFile;
#endif

  sprintf(RegPath, PROFILE_ROOT_TREE, KeyPath);
  lpData = (unsigned long)ValueInt;
  cbData = sizeof(DWORD);

  do {
	if (i > 0)
	  _sleep(0);
#ifdef REGTOFILE
     if (nClustering && !strnicmp(KeyPath, "software\\emwac", 14)) {
       retCode = 
         OpenKeyFile(mMailSpoolDir,
		             RegPath,
					 (char *)ValuePath,
					 &hFile);

	 } else
#endif
	 {
    retCode = 
      RegOpenKeyEx(hKeyRoot,    // Key handle at root level.
                   RegPath,     // Path name of child key.
                   0,           // Reserved.
                   KEY_WRITE,  // KEY_READ | KEY_WRITE, // Requesting read access.
                   &hKey);      // Address of key to be returned.
	 }
  } while(retCode == ERROR_INVALID_HANDLE && i++ < 3);  // ERROR_INVALID_HANDLE = 6
	if (retCode == ERROR_SUCCESS) {
#ifdef REGTOFILE
     if (nClustering && !strnicmp(KeyPath, "software\\emwac", 14)) {
       retCode =
		 KeyFileSetValueEx(mMailSpoolDir,
		                   RegPath,
					       ValuePath,
						   &hFile,
						   REG_DWORD,
						   (LPBYTE)&lpData,
						   (DWORD)cbData);
	     CloseHandle(hFile);
	 } else
#endif
	 {
      retCode =
         RegSetValueEx(hKey,	// handle of key to set value for  
                       ValuePath,	// address of value to set 
                       0,
                       REG_DWORD,	// type of value 
                       (CONST BYTE *)&lpData,	// address of value data 
                       (DWORD)cbData 	// size of value data 
                      );	
      RegCloseKey((HKEY)hKey);
	 }
	}
}

void WriteProfileStringEx(CHAR * KeyPath, CHAR * ValuePath, LPCTSTR ValueString) {
  HKEY   hKeyRoot = HKEY_LOCAL_MACHINE;
  HKEY   hKey;
  DWORD  retCode;
  int    mach = 1;
  DWORD  cbValueName = 80;
  DWORD  dwIndex = 0;	         // index of subkey to enumerate 
  DWORD  lpData = 0;
  DWORD  cbData = 80;
#ifdef UPDATE_20110909 // ML名+ドメイン名が53byteを超えるとハングする
  CHAR   RegPath[512];
#else
  CHAR   RegPath[80];
#endif
  int    i = 0;
#ifdef REGTOFILE
  HANDLE hFile;
#endif

  sprintf(RegPath, PROFILE_ROOT_TREE, KeyPath);
  lpData = (unsigned long)ValueString;
  cbData = strlen(ValueString);

  do {
	if (i > 0)
	  _sleep(0);
#ifdef REGTOFILE
     if (nClustering && !strnicmp(KeyPath, "software\\emwac", 14)) {
       retCode = 
         OpenKeyFile(mMailSpoolDir,
		             RegPath,
					 ValuePath,
					 &hFile);

	 } else
#endif
	 {
    retCode = 
      RegOpenKeyEx(hKeyRoot,    // Key handle at root level.
                   RegPath,     // Path name of child key.
                   0,           // Reserved.
                   KEY_WRITE,   // KEY_READ | KEY_WRITE, // Requesting read access.
                   &hKey);      // Address of key to be returned.
	 }
  } while(retCode == ERROR_INVALID_HANDLE && i++ < 3);  // ERROR_INVALID_HANDLE = 6
	if (retCode == ERROR_SUCCESS) {
#ifdef REGTOFILE
     if (nClustering && !strnicmp(KeyPath, "software\\emwac", 14)) {
       retCode =
		 KeyFileSetValueEx(mMailSpoolDir,
		                     RegPath,
					         ValuePath,
							 &hFile,
							 REG_SZ,
							 (LPBYTE)lpData,
							 (DWORD)cbData);
	     CloseHandle(hFile);
	 } else
#endif
	 {
      retCode =
         RegSetValueEx(hKey,	// handle of key to set value for  
                       ValuePath,	// address of value to set 
                       0,
                       REG_SZ,	// type of value 
                       (CONST BYTE *)lpData,	// address of value data 
                       (DWORD)cbData 	// size of value data 
                      );	
      RegCloseKey((HKEY)hKey);
	 }
	}
}

#ifdef UPDATE_20070405 // イベントログデータベースを追加。
///////////////////////////////////////////////////////////////////////////
// [形    式]    InitEventLog
// [引    数]    なし
// [説    明]    イベントログを出力可能にするための初期設定
// [戻 り 値]    正常終了  ERROR_SUCCESS
//               異常終了  エラーコード
///////////////////////////////////////////////////////////////////////////
LRESULT InitEventLog(VOID)
{
	HKEY	hKey;
	DWORD	dwType = EVENTLOG_ERROR_TYPE
					| EVENTLOG_WARNING_TYPE
					| EVENTLOG_INFORMATION_TYPE;
	CHAR	szRegPath[MAX_PATH];
	CHAR	szFilePath[2][MAX_PATH];
	LRESULT lResult = ERROR_CALL_NOT_IMPLEMENTED;
	CHAR    *p;

	sprintf(szRegPath, "%s%s", EVENT_REG, SMTP_SERVICE);

	// レジストリキーが既に存在するなら処理しない
	if ((lResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE, (LPCTSTR)szRegPath, 0, KEY_READ, &hKey)) == ERROR_SUCCESS) {
	  goto LabelExit;
	}

	// レジストリキー作成
	if((lResult = RegCreateKey(
			HKEY_LOCAL_MACHINE, szRegPath, &hKey)) != ERROR_SUCCESS){
		return lResult;
	}

	// サポートタイプ設定
	if((lResult = RegSetValueEx(hKey, "TypesSupported",
			0, REG_DWORD, (LPBYTE)&dwType, sizeof(DWORD))) != ERROR_SUCCESS){
		goto LabelExit;
	}

	// メッセージファイルパス設定
	if(GetModuleFileName(NULL, szFilePath[0], sizeof(szFilePath[0])) == 0){
		lResult = GetLastError();
		goto LabelExit;
	}
    GetLongPathName(szFilePath[0], szFilePath[1], sizeof(szFilePath[1]));

	if ((p = strrchr(szFilePath[0], '\\'))) {
	  *p = '\x0';
	  if ((p = strrchr(szFilePath[1], '\\'))) {
	    strcat(szFilePath[0], p);
	    if ((p = strrchr(szFilePath[0], '.'))) {
	      *p = '\x0';
	      strcat(szFilePath[0], ".dll");
 	      lResult = RegSetValueEx(hKey, "EventMessageFile",
			   0, REG_EXPAND_SZ, (LPBYTE)szFilePath[0], lstrlen(szFilePath[0]) + 1);
		}
	  }
	}

LabelExit:
	RegCloseKey(hKey);
	return lResult;
}
#endif

#ifdef UPDATE_20150108 // 送信許可時間の設定機能を追加
BOOL GetUseTimeFile(char *pUserFrom, char *pUserTo, char *pDom) 
{
  FILE   *fp;
  CHAR   *p, *pWeek, Path[MAX_PATH];
  BOOL   bsts = FALSE;
  BOOL   bHit = FALSE;
  BOOL   bFind = FALSE;
  SYSTEMTIME lt;
  CHAR   mUser[1024], mMBoxDir[_MAX_PATH], mt[256];
  CHAR   *pl, mWeek[4], mSchedule[256];
  int    nFrom, nTo, nNow;
  int    i;
  
  for (i = (nUseTime == 2 ? 0 : 1); i < 2; i++) // 個別と全体
  {

	switch (i)
	{
	  case 0:
	    strcpy(mUser, pUserFrom);
        GetMailBoxFolder(mUser, mMBoxDir);
        sprintf(Path, "%s%s", mMBoxDir, mUseTimeFile); // アカウント単位
	    break;
	  default:
        sprintf(Path, "%s%s", mProgPath, mUseTimeFile); // ゲートウェイ先単位
	    break;
	}
    if (bDebug) printf("Valid time file name = %s\n", Path);
	if ((fp = fopen(Path, "rt"))) 
	{
	  if (i == 1)
	  {
	    fgets(mSchedule, sizeof(mSchedule), fp);
	    if (strchr(mSchedule, ',')) // 先頭にキーがない場合はOTHER扱い
		{
	      bFind = TRUE;
	      fseek(fp, SEEK_SET, 0L);
		} else {
	      fseek(fp, SEEK_SET, 0L);
          while(fgets(mSchedule, sizeof(mSchedule), fp))
		  {
		    if (mSchedule[0])
			{
			  strtok(mSchedule, " ]\t\r\n");
			  if (strchr(mSchedule,'@')) // メールアドレス指定
			  {
				if (wildcard_stricmp(&mSchedule[1], pUserTo))
				{
			      bFind = TRUE;
			      break;
				}
			  } else {
			    if (wildcard_stricmp(&mSchedule[1], pDom)) // 転送先SMTP指定
				{
			      bFind = TRUE;
			      break;
				}
			  }
			}
		  }
		}
	  }
      if (i == 0 || bFind)
	  {
  	    bHit = TRUE;
	    gettime(&lt, mt); // 現在時刻取り出し
	    pWeek = mt;      mt[3] = '\x0';
	    nNow = atoi(&mt[17]) * 60 + atoi(&mt[20]);
        //sprintf(mt, "%3s, %02d %3s %04d %02d:%02d:%02d %s", mWeek[lt.wDayOfWeek], lt.wDay, mMonth[lt.wMonth-1], lt.wYear, lt.wHour, lt.wMinute, lt.wSecond, mzone);
	    while((p = fgets(mSchedule, sizeof(mSchedule), fp))) 
		{
		  if (!strchr(mSchedule, ','))
		  {
			break;
		  }
		  mSchedule[3] = '\x0'; strcpy(mWeek, mSchedule);
          if ((pl = strpbrk(&mSchedule[4], ":"))) {
		    *pl++;
		    nTo = nFrom = atoi(&mSchedule[4]) * 60 + atoi(pl);
		    nTo++;
            if ((pl = strpbrk(pl, "-"))) {
			  pl++;
		      nTo = atoi(pl) * 60;
              if ((pl = strpbrk(pl, ":"))) {
			    pl++;
		        nTo += atoi(pl);
			  }
			}
		    if (!stricmp(mWeek, pWeek) && nFrom <= nNow && nTo >= nNow) {  // 曜日＆時間範囲一致で許可
	          bsts = TRUE;
			  break;
			}
		  } else {
		    if (!stricmp(mWeek, pWeek)) {  // 曜日一致で許可
	          bsts = TRUE;
		  	  break;
			}
		  }
		}
	  }
	  fclose(fp);
      if (bDebug) printf(" -> found Valid time file (%s)\n", Path);
	} else {
	  if (i != 0)
	  {
	    bsts = TRUE;
	  }
      if (bDebug) printf(" -> not found Valid time file (%s)\n", Path);
	}
	if (bHit)
	{
	  break;
	}
  }
  if (!bHit) // どちらもヒットしない場合
  {
    bsts = TRUE;
  }
  return bsts;
}
#endif

#ifdef ADD_IDCACHE

BOOL IDLiveTime(char *pFn) 
{
  SYSTEMTIME ltime;
  FILETIME   st, ft;
  ULARGE_INTEGER *u1, *u2;
  __int64    n1, n2;
  HANDLE hFile;

   /// 現在時間
   GetSystemTime(&ltime);
   SystemTimeToFileTime(&ltime, &st);
   u1 = (ULARGE_INTEGER *)&st;
   n2 = n1  = u1->QuadPart;
   /// キャッシュ作成時間
   hFile = CreateFile( (LPCTSTR)pFn,
                        GENERIC_READ,
                        0,
                        NULL,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL, 
                        NULL);
   if (hFile != INVALID_HANDLE_VALUE) {
	 //GetFileTime(hFile, &ft, &st, NULL);
     GetFileTime(hFile, NULL, &st, &ft);
     CloseHandle(hFile);
     u2 = (ULARGE_INTEGER *)&ft;
     n2  = u2->QuadPart;
     if (n1 < (n2 + (__int64)nIDCashLiveTime*10000000) )
	   return TRUE;    // キャッシュ有効
   }
   return FALSE;   // キャッシュ無効
}

BOOL ReserveIDChche(char *pID)
{
  FILE    *fp;
  char    mIDCash[512];
  BOOL    bSts = TRUE;

#ifdef REGTOFILE
   sprintf(mIDCash,"%s\\" ADCACHE, mMailSpoolDir);
   _mkdir(mIDCash);         // cashフォルダ作成
   if (nClustering) {
     sprintf(mIDCash,"%s\\" ADCACHE "\\%s\\", mMailSpoolDir, mComName);
   } else {
#endif
     sprintf(mIDCash,"%s\\" ADCACHE "\\", mMailSpoolDir);
#ifdef REGTOFILE
   }
#endif

   _mkdir(mIDCash);         // cashフォルダ作成
   strcat(mIDCash, pID);
   strcat(mIDCash, ".lck");

     while (!(fp = _fsopen(mIDCash, "wb", _SH_DENYWR))) 
	 {
       if (bServiceTerminating)
         break;
	   _sleep(WAIT_TIME);
	 }
     if (fp)
     { 
       fclose(fp);
       while (!(fp = _fsopen(mIDCash, "rb", _SH_DENYWR))) 
	   {
         if (bServiceTerminating)
           break;
	     _sleep(WAIT_TIME);
         // IDロックファイルが作成されない場合の対策
         if ((fp = _fsopen(mIDCash, "wb", _SH_DENYWR))) 
		 { 
           fclose(fp);
		 }
	   }
       fclose(fp);
     } else {
       bSts = FALSE;
     }
     return bSts;
}

void ReleaseIDChche(char *pID)
{
  FILE    *fp;
  char    mIDCash[512];
  BOOL    bSts = TRUE;

#ifdef REGTOFILE
      if (nClustering) {
        sprintf(mIDCash,"%s\\" ADCACHE "\\%s\\%s.lck", mMailSpoolDir, mComName, pID);  // cashフォルダ
	  } else {
#endif
   sprintf(mIDCash,"%s\\" ADCACHE "\\%s.lck", mMailSpoolDir, pID);  // cashフォルダ
#ifdef REGTOFILE
	  }
#endif
     _unlink(mIDCash);
     while ((fp = _fsopen(mIDCash, "rb", _SH_DENYWR))) {
	  fclose(fp);
      if (bServiceTerminating)
        break;
	   _sleep(WAIT_TIME);
       // IDロックファイルが削除されない場合の対策
       _unlink(mIDCash);
	}
}

void SetIDCashList(char *pID, char *pData, char *pName)
{
#ifdef TRACE_MODE
	char str[LOG_BUFFER];
#endif
  FILE    *fpr, *fpw;
  char    mIDCash[512];
  char    mIDB[256];
  char    mID[512];

   if (nIDCashLiveTime == 0) // 更新時間が設定されていない場合は設定しない
	 return;
   if (strlen(pID)==0) 
     return;
   strcpy(mIDB, pID);
   translateUue2Base64(mIDB, strlen(mIDB), mID);
   if (!ReserveIDChche(mID)) // ロックファイルがないなら処理可能
   {
	 return;
   }
   strcat(mID, ".");
   strcat(mID, pName);

#ifdef TRACE_MODE
if (nSenderLog) {
   sprintf(str, "[          ] [%s] SetIDCashList=[%s]\n", pID, pData);
   printfTrace(str);
}
#endif

#ifdef REGTOFILE
   sprintf(mIDCash,"%s\\" ADCACHE, mMailSpoolDir);
   _mkdir(mIDCash);         // cashフォルダ作成
   if (nClustering) {
     sprintf(mIDCash,"%s\\" ADCACHE "\\%s\\", mMailSpoolDir, mComName);
   } else {
#endif
     sprintf(mIDCash,"%s\\" ADCACHE "\\", mMailSpoolDir);
#ifdef REGTOFILE
   }
#endif

   _mkdir(mIDCash);         // cashフォルダ作成
   strcat(mIDCash, mID);
   //if (!(fpr = _fsopen(mIDCash, "rb", _SH_DENYWR))) 
   {
     while (!(fpw = _fsopen(mIDCash, "wb", _SH_DENYWR))) 
	 {
       if (bServiceTerminating)
         break;
	   _sleep(WAIT_TIME);
	 }
	 if (fpw)
	 {
	   SPA_Encode(pData, mIDB);
       fprintf(fpw, "%s", mIDB);
	   fclose(fpw);
	 }
     // IDキャッシュデータの削除でハングする
     while (!(fpw = _fsopen(mIDCash, "rb", _SH_DENYWR))) {
       if (bServiceTerminating)
         break;
	    _sleep(WAIT_TIME);
	 }
     fclose(fpw);
	 /*
   } else {
     fclose(fpr);
	 */
   }
   strtok(mID, ".");
   ReleaseIDChche(mID);
}

BOOL CheckIDCashList(char *pID)
{
  char    mIDB[256];
  char    mID[512];
  BOOL    bSts = FALSE;

   if (strlen(pID)==0) 
     return bSts;
   strcpy(mIDB, pID);
   translateUue2Base64(mIDB, strlen(mIDB), mID);
   if (!ReserveIDChche(mID)) // ロックファイルがないなら処理可能
   {
	 return bSts;
   } 
   return TRUE;
}

void AddIDCashList(char *pID, char *pData, char *pName)
{
#ifdef TRACE_MODE
	char str[LOG_BUFFER];
#endif
  FILE    *fpr, *fpw;
  char    mIDCash[512];
  char    mIDB[256];
  char    mID[512];

   if (nIDCashLiveTime == 0) // 更新時間が設定されていない場合は設定しない
	 return;
   if (strlen(pID)==0) 
     return;
   strcpy(mIDB, pID);
   translateUue2Base64(mIDB, strlen(mIDB), mID);
   strcat(mID, ".");
   strcat(mID, pName);

#ifdef TRACE_MODE
if (nSenderLog) {
   sprintf(str, "[          ] [%s] AddIDCashList=[%s]\n", pID, pName);
   printfTrace(str);
}
#endif

#ifdef REGTOFILE
   sprintf(mIDCash,"%s\\" ADCACHE "\\", mMailSpoolDir);
   _mkdir(mIDCash);         // cashフォルダ作成
   if (nClustering) {
     sprintf(mIDCash,"%s\\" ADCACHE "\\%s\\", mMailSpoolDir, mComName);
   } else {
#endif
     sprintf(mIDCash,"%s\\" ADCACHE "\\", mMailSpoolDir);
#ifdef REGTOFILE
   }
#endif

   _mkdir(mIDCash);         // cashフォルダ作成
   strcat(mIDCash, mID);
   {
     while (!(fpw = _fsopen(mIDCash, "ab", _SH_DENYWR))) 
	 {
       if (bServiceTerminating)
         break;
	   _sleep(WAIT_TIME);
	 }
	 if (fpw)
	 {
	   SPA_Encode(pData, mIDB);
       fprintf(fpw, "%s\n", mIDB);
	   fclose(fpw);
       // IDキャッシュデータの削除でハングする
       while (!(fpw = _fsopen(mIDCash, "rb", _SH_DENYWR))) {
         if (bServiceTerminating)
           break;
	     _sleep(WAIT_TIME);
	   }
	   fclose(fpw);
	 }
   }
}

int CountIDCashList(char *pID, char *pData, char *pName, char *pMGroup, BOOL *pbGroup, BOOL *pbResult)
{
#ifdef TRACE_MODE
	char str[LOG_BUFFER];
#endif
  FILE    *fpr, *fpw;
  char    mIDCash[512];
  char    mIDB[256];
  char    mID[512];
  int     nCnt = 0;

   *pbResult = FALSE;
   *pbGroup = FALSE;
   if (nIDCashLiveTime == 0) // 更新時間が設定されていない場合は設定しない
	 return;
   if (strlen(pID)==0) 
     return;
   strcpy(mIDB, pID);
   translateUue2Base64(mIDB, strlen(mIDB), mID);
   strcat(mID, ".");
   strcat(mID, pName);

#ifdef TRACE_MODE
if (nSenderLog) {
   sprintf(str, "[          ] [%s] CountIDCashList=[%s]\n", pID, pName);
   printfTrace(str);
}
#endif

#ifdef REGTOFILE
   sprintf(mIDCash,"%s\\" ADCACHE "\\", mMailSpoolDir);
   _mkdir(mIDCash);         // cashフォルダ作成
   if (nClustering) {
     sprintf(mIDCash,"%s\\" ADCACHE "\\%s\\", mMailSpoolDir, mComName);
   } else {
#endif
     sprintf(mIDCash,"%s\\" ADCACHE "\\", mMailSpoolDir);
#ifdef REGTOFILE
   }
#endif

   _mkdir(mIDCash);         // cashフォルダ作成
   strcat(mIDCash, mID);

   if ((fpw = _fsopen(mIDCash, "rb", _SH_DENYWR))) {
     while((fgets(mIDB, sizeof(mIDB), fpw)))
	 {
	   strtok(mIDB, "\n");
	   SPA_Decode(mIDB, pData);
#ifdef TRACE_MODE
if (nSenderLog) {
   sprintf(str, "%s's Local group check. (Cache)=(\"%s\" vs. \"%s\") ", pID, pMGroup, pData);
   printfTrace(str);
}
#endif
       if (stricmp(pData, pMGroup) == 0) { // 2004.03.15 大小文字の違いなし
          *pbGroup = TRUE;
	   }
	   if (strstr(pData,".")) {
		 *pbResult = TRUE;
	   }
       nCnt++;
	 }
	 fclose(fpw);
   }
   strtok(mID, ".");
   ReleaseIDChche(mID);
   return nCnt;
}

void GetNumIDCashList(char *pID, char *pData, char *pName, int nEnt)
{
#ifdef TRACE_MODE
	char str[LOG_BUFFER];
#endif
  FILE    *fpr, *fpw;
  char    *p;
  char    mIDCash[512];
  char    mIDB[256];
  char    mID[512];
  char    mGrp[256];
  int     nCnt = 0;

   if (nIDCashLiveTime == 0) // 更新時間が設定されていない場合は設定しない
	 return;
   if (strlen(pID)==0) 
     return;
   strcpy(mIDB, pID);
   translateUue2Base64(mIDB, strlen(mIDB), mID);
   strcat(mID, ".");
   strcat(mID, pName);

#ifdef TRACE_MODE
if (nSenderLog) {
   sprintf(str, "[          ] [%s] CountIDCashList=[%s]\n", pID, pName);
   printfTrace(str);
}
#endif

#ifdef REGTOFILE
   sprintf(mIDCash,"%s\\" ADCACHE "\\", mMailSpoolDir);
   _mkdir(mIDCash);         // cashフォルダ作成
   if (nClustering) {
     sprintf(mIDCash,"%s\\" ADCACHE "\\%s\\", mMailSpoolDir, mComName);
   } else {
#endif
     sprintf(mIDCash,"%s\\" ADCACHE "\\", mMailSpoolDir);
#ifdef REGTOFILE
   }
#endif

   _mkdir(mIDCash);         // cashフォルダ作成
   strcat(mIDCash, mID);

   mGrp[0] = '\x0';
   if ((fpw = _fsopen(mIDCash, "rb", _SH_DENYWR))) {
	 for (nCnt = 0; nCnt <= nEnt; nCnt++)
	 {
       if(fgets(mID, sizeof(mID), fpw))
	   {
	     strtok(mID, "\n");
	     SPA_Decode(mID, mGrp);
	   }
	 }
	 strcpy(pData, mGrp);
	 fclose(fpw);
   }

   if ((p = strrchr(mIDCash, '\\')))
   {
     strtok(p++, ".");
     ReleaseIDChche(p);
   }
}

BOOL GetIDCashList(char *pID, char *pData, char *pName, BOOL *pResult)
{
#ifdef TRACE_MODE
	char str[LOG_BUFFER];
#endif
  FILE    *fp;
  char    *fsts;
  char    *p;
  char    mIDCash[512];
  char    mIDB[256];
  char    mID[512];
  char    mData[512] = "";
  BOOL    bSts = FALSE; //ゲートウェイリストに載っているか否か

   if (strlen(pID)==0) 
     return;
   strcpy(mIDB, pID);
   translateUue2Base64(mIDB, strlen(mIDB), mID);
   if (!ReserveIDChche(mID)) // ロックファイルがないなら処理可能
   {
	 return bSts;
   }
   strcat(mID, ".");
   strcat(mID, pName);

#ifdef REGTOFILE
      if (nClustering) {
        sprintf(mIDCash,"%s\\" ADCACHE "\\%s\\%s", mMailSpoolDir, mComName, mID);  // cashフォルダ
	  } else {
#endif
   sprintf(mIDCash,"%s\\" ADCACHE "\\%s", mMailSpoolDir, mID);  // cashフォルダ
#ifdef REGTOFILE
	  }
#endif
   if ((bSts = IDLiveTime(mIDCash))) {
     if ((fp = _fsopen(mIDCash, "rb", _SH_DENYWR))) {
	   mID[0] = '\x0';
       if ((fsts = fgets(mID, sizeof(mID)-1, fp)))
         strtok(mID, "\n");
	   fclose(fp);
   	   SPA_Decode(mID, mData);
	   fclose(fp);
	   if (!strcmp(pName, "hm") )//||
		   //!strcmp(pName, "gp") )
	   {
		 *pResult = (mData[0]  ? TRUE : FALSE);
		 strcpy(pData, (mData[0]  ? mData : ""));
	   } else {
	     *pResult = !strcmp(mData, pData);
	   }
	   bSts = TRUE;
	 } else {
	   bSts = FALSE;
	 }
	 /*
   } else {
	 _unlink(mIDCash);
     // IDキャッシュデータの削除でハングする
     while ((fp = _fsopen(mIDCash, "rb", _SH_DENYWR))) {
	   fclose(fp);
       if (bServiceTerminating)
         break;
	   _sleep(WAIT_TIME);
	 }
	 */
   }
   if ((p = strrchr(mIDCash, '\\')))
   {
     strtok(p++, ".");
     ReleaseIDChche(p);
   }
#ifdef TRACE_MODE
if (nSenderLog) {
   sprintf(str, "[          ] [%s] GetIDCashList=[%s] status=%s\n", pID, pData, (bSts ? "Hit" : "Fail"));
   printfTrace(str);
}
#endif
   return bSts;
}

void DelIDCashList(char *pID, char *pName)
{
#ifdef TRACE_MODE
	char str[LOG_BUFFER];
#endif
  char    mIDCash[512];
  char    mIDB[256];
  char    mID[512];
  FILE    *fp;

   if (strlen(pID)==0) 
     return;
   strcpy(mIDB, pID);
   translateUue2Base64(mIDB, strlen(mIDB), mID);
   strcat(mID, ".");
   strcat(mID, pName);

#ifdef REGTOFILE
      if (nClustering) {
        sprintf(mIDCash,"%s\\" ADCACHE "\\%s\\%s", mMailSpoolDir, mComName, mID);  // cashフォルダ
	  } else {
#endif
   sprintf(mIDCash,"%s\\" ADCACHE "\\%s", mMailSpoolDir, mID);  // cashフォルダ
#ifdef REGTOFILE
	  }
#endif
/////////////////////////////////////////////////////////////////
   _unlink(mIDCash);
    // ADキャッシュデータの削除でハングする
    while ((fp = _fsopen(mIDCash, "rb", _SH_DENYWR))) {
	  fclose(fp);
      if (bServiceTerminating)
        break;
	   _sleep(WAIT_TIME);
#ifdef UPDATE_20071129A // MXキャッシュファイルが削除されない場合の対策
       _unlink(mIDCash);
#endif
	}
/////////////////////////////////////////////////////////////////
#ifdef TRACE_MODE
if (nSenderLog) {
   sprintf(str, "[          ] [%s] DelIDCashList()\n", pID);
   printfTrace(str);
}
#endif
}

// ADキャッシュを全て削除する
void CleanIDCashList(void) {
#ifdef TRACE_MODE
	char str[LOG_BUFFER];
#endif
  HANDLE             hF, hFile;
  WIN32_FIND_DATA    FD;
  BOOL               bFile = TRUE;
  char    mIDCash[512], mCash[512];
  FILE    *fp;

#ifdef REGTOFILE
      if (nClustering) {
        sprintf(mIDCash,"%s\\" ADCACHE "\\%s\\*", mMailSpoolDir, mComName);  // cashフォルダ
	  } else {
#endif
   sprintf(mIDCash,"%s\\" ADCACHE "\\*", mMailSpoolDir);  // cashフォルダ
#ifdef REGTOFILE
	  }
#endif
     /////////////////////////////////////////////////////////////////
     hF = FindFirstFile(mIDCash, &FD);
     if (hF != INVALID_HANDLE_VALUE) {
       bFile = TRUE;
       while (bFile) {
	     if (!(!stricmp(FD.cFileName, ".") ||
	           !stricmp(FD.cFileName, ".."))) {
#ifdef REGTOFILE
           if (nClustering) {
             sprintf(mCash,"%s\\" ADCACHE "\\%s\\%s", mMailSpoolDir, mComName, FD.cFileName);  // cashフォルダ
		   } else
#endif
		   {
             sprintf(mCash,"%s\\" ADCACHE "\\%s", mMailSpoolDir, FD.cFileName);  // cashフォルダ
		   }
           _unlink(mCash);
#ifdef TRACE_MODE
           if (nSenderLog) {
             sprintf(str, "[          ] [%s] CleanIDCashList()\n", mCash);
             printfTrace(str);
		   }
#endif
		 }
         bFile = FindNextFile( hF, &FD);
	   }; 
       FindClose( hF ); 
	 }
}
#endif

#ifdef UPDATE_20211210_IPV6PREFIX // IPv6アドレスプレフィックスに対応した。
BOOL Compear_IPv6_Addr(char *pSrcAddr, char *pCompAddr, char *pPrefix)
{
  int      mask;
  BOOL     bobf;
  char     *pmp;
  BOOL     bResult = FALSE;
  DWORD    slit=0, slitbit=0;
  struct in6_addr pip6, dip6, rip6;
  unsigned __int64 deip6[2], dsip6[2], ddip6[2];
			   
#ifdef VC6
  inet6_addr(pSrcAddr, &pip6);
  inet6_addr(pCompAddr, &dip6);
#else
  inet_pton(AF_INET6, pSrcAddr, &pip6);
  inet_pton(AF_INET6, pCompAddr, &dip6);
#endif
  memcpy((void *)&rip6, (void *)&dip6, sizeof(struct in6_addr));
  //接続元アドレス
  deip6[0] = (unsigned __int64)((unsigned __int64)pip6.s6_addr[0]*0x100000000000000+
                                (unsigned __int64)pip6.s6_addr[1]*0x1000000000000+
                                (unsigned __int64)pip6.s6_addr[2]*0x10000000000+
                                (unsigned __int64)pip6.s6_addr[3]*0x100000000+
                                (unsigned __int64)pip6.s6_addr[4]*0x1000000+
                                (unsigned __int64)pip6.s6_addr[5]*0x10000+
                                (unsigned __int64)pip6.s6_addr[6]*0x100+
                                (unsigned __int64)pip6.s6_addr[7]);
  deip6[1] = (unsigned __int64)((unsigned __int64)pip6.s6_addr[8]*0x100000000000000+
                                (unsigned __int64)pip6.s6_addr[9]*0x1000000000000+
                                (unsigned __int64)pip6.s6_addr[10]*0x10000000000+
                                (unsigned __int64)pip6.s6_addr[11]*0x100000000+
                                (unsigned __int64)pip6.s6_addr[12]*0x1000000+
                                (unsigned __int64)pip6.s6_addr[13]*0x10000+
                                (unsigned __int64)pip6.s6_addr[14]*0x100+
                                (unsigned __int64)pip6.s6_addr[15]);
  //下限アドレス
  dsip6[0] = (unsigned __int64)((unsigned __int64)dip6.s6_addr[0]*0x100000000000000+
                                (unsigned __int64)dip6.s6_addr[1]*0x1000000000000+
                                (unsigned __int64)dip6.s6_addr[2]*0x10000000000+
                                (unsigned __int64)dip6.s6_addr[3]*0x100000000+
                                (unsigned __int64)dip6.s6_addr[4]*0x1000000+
                                (unsigned __int64)dip6.s6_addr[5]*0x10000+
                                (unsigned __int64)dip6.s6_addr[6]*0x100+
                                (unsigned __int64)dip6.s6_addr[7]);
  dsip6[1] = (unsigned __int64)((unsigned __int64)dip6.s6_addr[8]*0x100000000000000+
                                (unsigned __int64)dip6.s6_addr[9]*0x1000000000000+
                                (unsigned __int64)dip6.s6_addr[10]*0x10000000000+
                                (unsigned __int64)dip6.s6_addr[11]*0x100000000+
                                (unsigned __int64)dip6.s6_addr[12]*0x1000000+
                                (unsigned __int64)dip6.s6_addr[13]*0x10000+
                                (unsigned __int64)dip6.s6_addr[14]*0x100+
                                (unsigned __int64)dip6.s6_addr[15]);
  ///上限アドレス
  ddip6[0] = (unsigned __int64)((unsigned __int64)rip6.s6_addr[0]*0x100000000000000+
                                (unsigned __int64)rip6.s6_addr[1]*0x1000000000000+
                                (unsigned __int64)rip6.s6_addr[2]*0x10000000000+
                                (unsigned __int64)rip6.s6_addr[3]*0x100000000+
                                (unsigned __int64)rip6.s6_addr[4]*0x1000000+
                                (unsigned __int64)rip6.s6_addr[5]*0x10000+
                                (unsigned __int64)rip6.s6_addr[6]*0x100+
                                (unsigned __int64)rip6.s6_addr[7]);
  ddip6[1] = (unsigned __int64)((unsigned __int64)rip6.s6_addr[8]*0x100000000000000+
                                (unsigned __int64)rip6.s6_addr[9]*0x1000000000000+
                                (unsigned __int64)rip6.s6_addr[10]*0x10000000000+
                                (unsigned __int64)rip6.s6_addr[11]*0x100000000+
                                (unsigned __int64)rip6.s6_addr[12]*0x1000000+
                                (unsigned __int64)rip6.s6_addr[13]*0x10000+
                                (unsigned __int64)rip6.s6_addr[14]*0x100+
                                (unsigned __int64)rip6.s6_addr[15]);
  // 上限アドレス算出
  bobf = FALSE;
  if ((mask = 128 - atoi((char *)(pPrefix+1))) < 0)
    mask = 0;
  if (mask < 64)
  {
    if ((unsigned __int64)(ddip6[1] +(unsigned __int64)((unsigned __int64)1<<mask)) < (unsigned __int64)ddip6[1])
	{
	  bobf = TRUE;
      ddip6[0]++; // オーバフロー
	}
	if (mask < 64)
      ddip6[1] +=(unsigned __int64)((unsigned __int64)1<<mask);
  } else{ // 64以上
	bobf = TRUE;
    ddip6[0] +=(unsigned __int64)((unsigned __int64)1<<(64-mask));
  }
  // 範囲内か比較
  if (dsip6[0] == deip6[0]) // 上位桁への繰上りが無いとき。
  {
    if (dsip6[1] <= deip6[1] && ((!bobf && ddip6[1] > deip6[1]) || bobf))
      bResult = TRUE;  // 範囲内と判定した。
  } else if (ddip6[0] == deip6[0]) // 上位桁への繰上りが有るとき。
  {
    if (ddip6[1] > deip6[1])
      bResult = TRUE;  // 範囲内と判定した。
  }

  return bResult;
}
#endif

#ifdef UPDATE_20240127 // ＭＬ制御応答＆送信エラー通知メールにもDKIMサインを追加可能にするオプション
void AddDKIMHeader(char *pMsg, char *pDomain)
{
   FILE *fp;
   CHAR mMyMTAIP[256]="";
   CHAR mCmdl[256];
   CHAR mArgv[_MAX_PATH*2];
   CHAR mMess[512];

   sprintf(mCmdl, "\"%s\"", mDomainAUTHDKIM);
   sprintf(mArgv, "\"postmaster@%s\"", pDomain);
   GetMTAIP(pDomain, mMyMTAIP);
//   if (!mMyMTAIP[0])
//	 strcpy(mMyMTAIP, "127.0.0.1");
   if (!(nOnDKIM & 0x80))
   {
#ifdef UPDATE_20240129 // DKIMサインが追加されない
     _spawnl(_P_WAIT, mDomainAUTHDKIM, mCmdl, pDomain, mArgv, pMsg, pDomain, NULL);
#else
     _spawnl(_P_WAIT, mDomainAUTHDKIM, mCmdl, mMyMTAIP, mArgv, pMsg, pDomain, NULL);
#endif
   } else {
#ifdef UPDATE_20240129 // DKIMサインが追加されない
     sprintf(mCmdl, "\"\"%s\" %s %s %s %s\"", mDomainAUTHDKIM, pDomain, mArgv, pMsg, pDomain);
#else
     sprintf(mCmdl, "\"\"%s\" %s %s %s %s\"", mDomainAUTHDKIM, mMyMTAIP, mArgv, pMsg, pDomain);
#endif
     if ((fp = (FILE *)_popen((const char *)mCmdl, (const char *)"rt")))
	 {
       while(fgets( mMess, 128, fp ));
        _pclose((FILE *)fp);
	 }
   }
}
#endif

#ifdef UPDATE_20240127A // 転送メールにARCヘッダを追加可能にした。
void AddARCHeader(struct _RCP *rcp, int no)
{
   FILE *fp;
   CHAR *pARC;
   CHAR mARCPath[256];
   CHAR mCmdl[512];
   CHAR mMess[512];
   CHAR mARCCmd[256];
   CHAR mARC[256];
#ifdef TRACE_MODE
   char str[LOG_BUFFER];
#endif
   CHAR mArgv[_MAX_PATH*2];
   CHAR mINI[_MAX_PATH*2];

   if (mDomainAUTHARC[0])
     strcpy(mARC, mDomainAUTHARC);
   else
     strcpy(mARC, mDomainAUTHDKIM);
   if (pARC = strrchr(mARC, '\\'))
   {
#ifdef TRACE_MODE
     if (nSenderLog) {
       sprintf(str, "[%s-%03d]:Call ARC:%s\n", rcp->mMNo, no, mCmdl);
       printf("%s", str);
       printfTrace(str);
	 }
#endif
     *pARC = '\x0';
     strcpy(mARCPath, mARC);
	 if (mDomainAUTHARC[0]) {
       *pARC = '\\';
       sprintf(mARCCmd, "\"%s\"", mDomainAUTHARC);
	 } else {
       sprintf(mARCCmd, "\"%s\\epstarc.exe\"", mARC);
       *pARC = '\\';
       strcpy(mARC, &mARCCmd[1]);
	   mARC[strlen(mARC)-1]= '\x0';
	 }
     sprintf(mArgv, "\"postmaster@%s\"", rcp->mMID);
	 sprintf(mINI, "\"%s\\epstarc.%s.ini\"", mARCPath, rcp->mMID);
     if (!(nOnDKIM & 0x80))
	 {
       _spawnl(_P_WAIT, mARC, mARCCmd, "-i", rcp->mMID, "-e", mArgv, "-t", rcp->mMsg, "-c", mINI, NULL);
	 } else {
       sprintf(mCmdl, "\"%s -i %s -e %s -t \"%s\" -c %s\"", mARCCmd, rcp->mMID, mArgv, rcp->mMsg, mINI);
       if ((fp = (FILE *)_popen((const char *)mCmdl, (const char *)"rt")))
	   {
         while(fgets( mMess, 128, fp));
          _pclose((FILE *)fp);
	   }
	 }
#ifdef TRACE_MODE
     if (nSenderLog) {
       sprintf(str, "[%s-%03d]:Add ARC :%s\n", rcp->mMsg, no, mCmdl);
       printf("%s", str);
       printfTrace(str);
	 }
#endif
   }
}
#endif

#ifdef UPDATE_20260603 // ログ出力の排他処理化を追加
HANDLE g_hMutexOutLog = NULL;
HANDLE g_hMutexOutLocalLog = NULL;
HANDLE g_hMutexOutFailLog = NULL;
HANDLE g_hMutexOutSenderLog = NULL;

#ifdef UPDATE_20260606 // ログの書き込み信頼性向上対策
/*
  ログの書き込み用ファイルディスクプリタを新規で開いた後保持して
　一定時間間隔（１時間間隔）内の書込みは実行し時間を超えた時の書き
　込みはクローズ再オープンし、書き込み時は、MUTEXで排他する。
*/
FILE *fpOutLog = NULL;
FILE *fpOutLocalLog = NULL;
FILE *fpOutFailLog = NULL;
FILE *fpOutSenderLog = NULL;

char mDTOutLog[16] = "";
char mDTOutLocalLog[16] = "";
char mDTOutFailLog[16] = "";
char mDTOutSenderLog[16] = "";
#endif

/* 初期化：アプリ起動時に1回だけ呼ぶ */
void InitLogger()
{
    g_hMutexOutLog = CreateMutex(NULL, FALSE, TEXT("Local\\MyLogMutexOutlog"));
    if (bDebug) printf("g_hMutexOutLog = %08x\n", g_hMutexOutLog);
    g_hMutexOutLocalLog = CreateMutex(NULL, FALSE, TEXT("Local\\MyLogMutexOutLocallog"));
    if (bDebug) printf("g_hMutexOutLocalLog = %08x\n", g_hMutexOutLocalLog);
    g_hMutexOutFailLog = CreateMutex(NULL, FALSE, TEXT("Local\\MyLogMutexOutFaillog"));
    if (bDebug) printf("g_hMutexOutFailLog = %08x\n", g_hMutexOutFailLog);
    g_hMutexOutSenderLog = CreateMutex(NULL, FALSE, TEXT("Local\\MyLogMutexOutSenderlog"));
    if (bDebug) printf("g_hMutexOutSenderLog = %08x\n", g_hMutexOutSenderLog);
}

/* 終了処理：アプリ終了時に1回だけ呼ぶ */
void CloseLogger()
{
    if (g_hMutexOutLog) {
        CloseHandle(g_hMutexOutLog);
        g_hMutexOutLog = NULL;
    }
    if (g_hMutexOutLocalLog) {
        CloseHandle(g_hMutexOutLocalLog);
        g_hMutexOutLocalLog = NULL;
    }
    if (g_hMutexOutFailLog) {
        CloseHandle(g_hMutexOutFailLog);
        g_hMutexOutFailLog = NULL;
    }
    if (g_hMutexOutSenderLog) {
        CloseHandle(g_hMutexOutSenderLog);
        g_hMutexOutSenderLog = NULL;
    }

}

/* 排他付きログ出力 サンプル*/
/*
void WriteLog(const char* fmt, ...)
{
    WaitForSingleObject(g_hMutexLog, INFINITE);  // 排他開始

    FILE* fp = fopen("app.log", "a");
    if (fp) {
        va_list args;
        va_start(args, fmt);
        vfprintf(fp, fmt, args);
        fprintf(fp, "\n");
        va_end(args);
        fclose(fp);
    }

    ReleaseMutex(g_hMutexLog);  // 排他終了
}
*/

#ifdef UPDATE_20260606 // ログの書き込み信頼性向上対策

FILE *OpenCloseLog(FILE *pfp, char *pDT, char *plog, char *pCom, SYSTEMTIME lt)
{
   char mdata[16];
   char mFn[256];

   sprintf(mdata, "%02d%02d%02d%02d%02d", (lt.wYear%100), lt.wMonth, lt.wDay, lt.wHour, lt.wMinute);
   mdata[8] = '\x0'; // 10桁で分単位 9桁で10分 8桁で時間

   if (*pDT == 0 || strcmp(mdata, pDT)) // 不一致なら
   {
	 int retry = 0;
	 if (pfp) 
	 {
	   fflush(pfp);
	   fclose(pfp);
 	   pfp = NULL;
	 }
	 /////
#ifdef REGTOFILE
      if (nClustering) {
         sprintf(mFn, "%s\\%s\\%s\\%02d%02d%02d.log", mMailSpoolDir, plog, pCom, (lt.wYear%100), lt.wMonth, lt.wDay);
	  } else {
#endif
         sprintf(mFn, "%s\\%s\\%02d%02d%02d.log", mMailSpoolDir, plog, (lt.wYear%100), lt.wMonth, lt.wDay);
#ifdef REGTOFILE
	  }
#endif
       while (pfp == NULL && retry < 3) {
         pfp = fopen(mFn, "at");
		 //pfp = _fsopen(mFn, "at", _SH_DENYNO);
         if (pfp)
			break;
		 Sleep(10); // AVスキャン完了を少し待つ
         retry++;
	   }
	   if (pfp) // 開けたらな日付をコピー
	   {
		 strcpy(pDT, mdata);
	   }
	 ////
   }
   return pfp;
}
#endif
#endif