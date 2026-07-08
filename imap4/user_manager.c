////////////////////////////////////////////////////////////
// User_manager.c Copyright K.kawakami 2001.11.15
////////////////////////////////////////////////////////////
#include "imap4.h"

#ifdef V4
extern INT  nMAXUser;
#endif
extern char mProgPath[];
extern char mAuthDomain[];
extern char mLocalDomain[];
#ifdef UPDATE_20070521 // OSの予約語対策
extern CHAR   mReservedWords[];
#endif
int Base64DecodeLine(char * pszLine, int nLineLen, unsigned char * uchBuffer, int nBufferLen);

typedef struct _User_DB {
    char        account[256];  // ユーザー名
    char        password[256]; // パスワード
	char        fullname[256]; // フルネーム
	char        domain[256];   // ドメイン名
	char        home[256];     // ホームフォルダ
};

NET_API_STATUS confirms_user(char *Domain, char *user, char *myaddr, struct _User_DB *userdb, DWORD *total) {
  char mDB[256], mIP[256];
  char mDec[1024], mline[1024];
  int  n;
  NET_API_STATUS sts = NERR_UserNotFound;
  FILE *fdb;
  char *pldom;
#ifdef HOME_VERSION
  int  nUN = USER_LIMIT; // 最大ユーザー数
#endif
#ifdef V4
  int  nUN = nMAXUser;  // 最大ユーザー数
#endif
  BOOL bOneDomain = FALSE;
#ifdef UPDATE_20070521 // 予約語対策
  CHAR mKey[256];
#endif
#ifdef UPDATE_20190207 // 独自アカウントデータベース内のパスワードの暗号化
  char mpass[256]; // パスワード
#endif

  userdb->account[0] = userdb->fullname[0] = userdb->domain[0] = userdb->home[0] = '\x0';
  *total = 0;

   if (!Domain)
     pldom = mLocalDomain;
   else if (*Domain == '\x0')
     pldom = mLocalDomain;
   else {
     pldom = Domain;
	 bOneDomain = TRUE;
   }
   
//printf("confirms_user():mLocalDomain=[%s]:myaddr=[%s]\n",mLocalDomain, myaddr);

   while (strlen(pldom)){
//printf("confirms_user():pldom=[%s]\n", pldom);
	  // IP定義に不一致な場合はデータベース検索をとばす。
	  if (myaddr[0] != '\x0') {
        GetProfileStringEx(DOMAIN_IMAP4IP, pldom, "", mIP, sizeof(mIP)); // 対象ドメインの応答ＩＰアドレス取出し
#ifdef UPDATE_20070521 // 予約語対策
        strcpy(mKey, pldom);
        strtok(mKey, "@");
        if (ReservedWords(mKey)) {
          strcpy(mKey, mReservedWords);
          strcat(mKey, pldom);
		  GetProfileStringEx(DOMAIN_IMAP4IP, mKey, "", mIP, sizeof(mIP)); // 対象ドメインの応答ＩＰアドレス取出し
		}
#endif
	  } else {
		mIP[0] = '\x0';
	  }

#ifdef CLUSTERING
	  if (mIP[0] == '\x0' || strstr(mIP, myaddr))
#else
	  if (mIP[0] == '\x0' || strcmp(myaddr, mIP) == 0)
#endif
	  {
	    /////////////////////////
		if (mAuthDomain[0] == '\x0') // アカウントフォルダが空白なら実行パスをアカウントフォルダにする。
          sprintf(mDB,"%s%s.idx", mProgPath, pldom);
		else
          sprintf(mDB,"%s%s.idx", mAuthDomain, pldom);
#ifdef UPDATE_20070521 // 予約語対策
        strcpy(mKey, pldom);
        strtok(mKey, "@");
        if (ReservedWords(mKey)) {
          strcpy(mKey, mReservedWords);
          strcat(mKey, pldom);
		  if (mAuthDomain[0] == '\x0') // アカウントフォルダが空白なら実行パスをアカウントフォルダにする。
            sprintf(mDB,"%s%s.idx", mProgPath, mKey);
		  else
            sprintf(mDB,"%s%s.idx", mAuthDomain, mKey);
		}
#endif
	    ///////////////////////////////////////////
        fdb = fopen( mDB, "rt" );
printf("confirms_user()fopen()=%08x:mDB=[%s]\n", fdb, mDB);
        if (fdb) {
	      do {
#ifndef TUNING
			memset(mDec, 0, sizeof(mDec));
#else
			mDec[0] = '\x0';
#endif
            fgets(mDec, sizeof(mDec), fdb);
 	        if (feof(fdb))
              break;
	        strtok(mDec, " \n");
            Base64DecodeLine(mDec, strlen(mDec), (unsigned char *)mline, sizeof(mline));
            strtok(mline, "\t");
//printf("confirms_user():user=[%s] mline=[%s]\n", user, mline);
            if (stricmp(mline, user) == 0) {
 		      n = strlen(mline)+1;
		      strcpy(userdb->account, mline);
#ifdef UPDATE_20190207 // 独自アカウントデータベース内のパスワードの暗号化
			  if (mline[n] != '\t')
                strtok(&mline[n], "\t");
			  else
 		        mline[n] = '\x0';
			  SPA_Decode(&mline[n], mpass); 
              strcpy(userdb->password, mpass);
#else
              strcpy(userdb->password, &mline[n]);
			  if (userdb->password[0] != '\t')
                strtok(userdb->password, "\t");
			  else
 		        userdb->password[0] = '\x0';
#endif
#ifdef UPDATE_20190207 // 独自アカウントデータベース内のパスワードの暗号化
			  n += strlen(&mline[n])+1;
#else
		      n += strlen(userdb->password)+1;
#endif
              strcpy(userdb->fullname, &mline[n]);
			  if (userdb->fullname[0] != '\t')
                strtok(userdb->fullname, "\t");
			  else
 		        userdb->fullname[0] = '\x0';
		      n += strlen(userdb->fullname)+1;
              strcpy(userdb->domain, &mline[n]);
			  if (userdb->domain[0] != '\t')
                strtok(userdb->domain, "\t");
			  else
 		        userdb->domain[0] = '\x0';
		      n += strlen(userdb->domain)+1;
		      strcpy(userdb->home, &mline[n]);
			  if (userdb->home[0] != '\t')
                strtok(userdb->home, "\t");
			  else
 		        userdb->home[0] = '\x0';
		      *total = 1;
	          sts = NERR_Success;
	          break;
			} else {
              userdb->account[0] = userdb->fullname[0] = userdb->domain[0] = userdb->home[0] = '\x0';
			}
#ifdef HOME_VERSION
            nUN--; // 最大ユーザー数
#else
#ifdef V4
            if (nMAXUser) // 無制限でないなら
		      nUN--; // 最大ユーザー数
#endif
#endif
		  }
#ifdef HOME_VERSION
		  while(!feof(fdb) && nUN); // 人数制限ないなら繰り返し
#else
#ifdef V4
		  while(!feof(fdb) && (nMAXUser ? nUN : 1)); // 人数制限ないなら繰り返し
#else
		  while(!feof(fdb));
#endif
#endif
	      fclose(fdb);
		}
	    if (sts == NERR_Success)
		  break;
	  }
  	  ///////////////////////////////////////////
	  if (bOneDomain)
		break;
      pldom += strlen(pldom);
 	  pldom++;
//printf("confirms_user()=%s, %lu [%s]\n",(sts == NERR_Success ? "Success" : "Fail"), fdb, mDB);
   };
///////////////////////////////////
//printf("confirms_user()=%d:strstr(%s, %s)\n",sts,  mIP, myaddr);
///////////////////////////////////

   return sts;
}
