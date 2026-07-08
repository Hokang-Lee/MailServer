#include "smtp.h"
#include <windows.h>
#include <stdio.h>

#define MAX_ORDB 10

extern BOOL bDebug;
extern int  nAddressFamily;
//extern BOOL bORDB;
extern char mORDBFile[];
extern char mPASSDBFile[];

struct ORDB_TBL {
  char   mSite[128];            // Site list of "Open relay data base"
#ifdef IPv6
  char   mIP[INET6_ADDRSTRLEN]; // Black list IP xxxx:xxxx:xxxx::::xxxx
#else
  char   mIP[21];               // Black list IP xxx.xxx.xxx.xxx
#endif
} ORDB[MAX_ORDB];

int GetSiteListOfORDB(void) {
  FILE *fp;
  char *p, mwork[128];
  int  i;
    for (i = 0; i < MAX_ORDB; i++) {
	  ORDB[i].mSite[0] = '\x0'; // サイトリスト初期化
	  ORDB[i].mIP[0] = '\x0'; // サイトリスト初期化
	}
	i = 0;
	if ((fp = fopen(mORDBFile, "rt"))) {
	  p = fgets(mwork, sizeof(mwork), fp); // dummy
	  while((p || !feof(fp)) && (i < MAX_ORDB)) {
		p = strpbrk(mwork, "\r\n");
		if (p)
		  *p = '\x0';
		if (mwork[0] != '\x0' &&
		    mwork[0] != ' ' &&
		    mwork[0] != '\'') {  // コメントチェック
#ifdef DNS_QUERY
	      strcpy(ORDB[i].mSite, mwork);
#else
		  p = strstr(mwork,",");
		  if (p) {
			*p = '\x0';
		    strcpy(ORDB[i].mSite, mwork);
		    strcpy(ORDB[i].mIP, (p+1));
		  } else {
		    strcpy(ORDB[i].mSite, mwork);
		    strcpy(ORDB[i].mIP, "127.0.0.2");
		  }
#endif
	      i++;
		}
	    mwork[0] ='\x0';
	    p = fgets(mwork, sizeof(mwork), fp); // dummy
	  }
	  fclose(fp);
	}
	return i;
}

int GetQueryHostName(char *pAddr, char *pHostName) {
   char *p, *p6;
   int nIPType = 0; // 0:Error, 1:IPv4, 2:IPv6

   *pHostName = '\x0';
   if ((p = strrchr(pAddr, ':'))) {
	 p6 = p;
	 nIPType = 2; // IPv6
	 if ((p = strrchr(pAddr, '.'))) { // IPv4アドレスが含まれるなら
       strcpy(pHostName, p+1);
       strcat(pHostName, ".");
       *p = '\x0';
	   while ((p = strrchr(pAddr, '.'))) {
	     strcat(pHostName, p+1);
	     strcat(pHostName, ".");
	     *p = '\x0';
	   };
	   p = strrchr(pAddr, ':');
       strcat(pHostName, p+1);
       //strcat(pHostName, ".");
	 } else {
	   p = p6;
       strcpy(pHostName, p+1); // ':'のみ
       strcat(pHostName, ".");
	 }
     *p = '\x0';
	 while ((p = strrchr(pAddr, ':'))) {
	   strcat(pHostName, p+1);
	   strcat(pHostName, ".");
	   *p = '\x0';
	 }
     strcat(pHostName, pAddr);
     strcat(pHostName, ".");
   } else if ((p = strrchr(pAddr, '.'))) {
	 nIPType = 1; // IPv4
     strcpy(pHostName, p+1);
     strcat(pHostName, ".");
     *p = '\x0';
	 while ((p = strrchr(pAddr, '.'))) {
	   strcat(pHostName, p+1);
	   strcat(pHostName, ".");
	   *p = '\x0';
	 };
     strcat(pHostName, pAddr);
     strcat(pHostName, ".");
   }
   return nIPType;
}

BOOL Query_PASSIP(char *pAddr) {
  FILE *fp;
  int  i, n, mask, num, addr, start, dot;
  char *p, *pwild, *pmask, mwork[128];
  BOOL bsts = FALSE; // 一致するものが無い

  if ((fp = fopen(mPASSDBFile, "rt"))) {
	p = fgets(mwork, sizeof(mwork), fp); // dummy
	while(p || !feof(fp)) {
	  p = strpbrk(mwork, "\r\n");
	  if (p)
		*p = '\x0';
	  if (mwork[0] != '\x0' &&
	      mwork[0] != ' ' &&
	      mwork[0] != '\'') {  // コメントチェック
#ifdef UPDATE_20070511 // IPアドレス確認ロジックの共有化
	   if ((bsts = IP_COMP(mwork, pAddr)))
	 	 break;
#else
		num = 1; // 比較アドレス数
		if ((pwild = strchr(mwork, '*'))) { // ワイルドカードでのマスク
		  *pwild = '\x0';
		} else if (strchr(mwork, '.')) { // IPv4のみ
		  if ((pwild = strchr(mwork, '/'))) {// ネットマスク
		    *pwild = '\x0';
		    mask = 32 - atoi(++pwild);
		    num = 1 << mask; // 2のべき乗範囲 num <<= mask; // 2のべき乗範囲
			dot = ((mask-1) / 8) + 1;
			if (dot > 1)
			  addr = num / (256*(dot-1));
			 else
			   addr = num;
			for (i = 1; i <= dot; i++) {
			  if ((pwild = strrchr(mwork, '.')))
				*pwild = '\x0';
			}
			start = atoi(pwild + 1);
		  }
		}
		if (num == 1) {
          if (!strncmp(pAddr, mwork, strlen(mwork))) {
		    bsts = TRUE;  // 一致するものがあった
		    break;
		  }
		} else { // ネットマスクの場合
	      for ( i = 0; i < addr; i++) {
			*pwild = '\x0';
			pmask = &mwork[strlen(mwork)];
		    sprintf(pmask, ".%d", start + i);
            if (!strncmp(pAddr, mwork, strlen(mwork))) {
		      bsts = TRUE;  // 一致するものがあった
			  break;
			}
		  }
		  if (bsts)
			break;
		}
#endif
	  }
	  mwork[0] ='\x0';
	  p = fgets(mwork, sizeof(mwork), fp); // dummy
	}
	fclose(fp);
  }
  return bsts;
}

BOOL Query_ORDB(char *pAddr) { // pAddr = "xxx.xxx.xxx.xxx"
	BOOL sts = FALSE; // ブラックリストに載っていない
	char mHostName[128];
	HOSTENT *phost;
	char    *pSite;
	int     nIP, i;
#ifdef IPv6
	int    nErr = 0;
	CHAR   mPaddr[INET6_ADDRSTRLEN]; // Peer Host Address IPv6
#else
	CHAR   mPaddr[21];   // Peer Host Address xxx.xxx.xxx.xxx
#endif
#ifndef VC6
    HOSTENT hst;
    struct addrinfo Hints, *AddrInfo;
#endif
#ifdef DNS_QUERY
	CHAR *pDNS = NULL;
#ifdef UPDATE_20080703 // メールフィルタのRBL参照DNS指定で、キャッシュ有効無効のオプションを追加
	BOOL bQuery = TRUE;
	CHAR *pQuery;
#endif
#endif
	
    strcpy(mPaddr, pAddr);
	if (Query_PASSIP(mPaddr)) // ORDBチェックをパスしてよいIP
	  return sts;

	//strcpy(mPaddr, "::203.65.95.66"); 
	//strcpy(mPaddr, "203.65.95.66"); // test black list IP
	nIP = GetQueryHostName(mPaddr, mHostName);
	pSite = mHostName + strlen(mHostName); // サイト名の加算位置
	for (i = 0; i < MAX_ORDB; i++) {
	  if (ORDB[i].mSite[0] == '\x0') // サイトリストが無ければ終了
		break;
	  strcpy(pSite, ORDB[i].mSite);
#ifdef IPv6
      if (nIP == 2) { // IPv4 address
#ifdef VC6
	    phost = getipnodebyname(mHostName, AF_INET6, AI_ALL, &nErr); //ORDBにレコードがあるか？
		if (phost) {
		  freehostent(phost);
          sts = FALSE; // ブラックリストに不掲載
		  break;
		}
#else
		phost = NULL;
        memset(&Hints, 0, sizeof(Hints));
		//Hints.ai_flags = AI_NUMERICHOST | AI_PASSIVE;
        Hints.ai_family =  AF_INET6;
        Hints.ai_socktype = SOCK_STREAM;
		if (getaddrinfo(mHostName, NULL, &Hints, &AddrInfo) == 0) {
          freeaddrinfo(AddrInfo);
          sts = FALSE; // ブラックリストに不掲載
		  break;
        }
#endif
	  } else if (nIP == 1) { // IPv4 address
#ifdef DNS_QUERY
	   if ((pDNS = strrchr(pSite, '\t'))) 
	   {
		 *pDNS = '\x0';
		 pDNS++;
#ifdef UPDATE_20080703 // メールフィルタのRBL参照DNS指定で、キャッシュ有効無効のオプションを追加
		 bQuery = TRUE;  // DNSキャシュ有効
		 if ((pQuery =strrchr(pDNS, ',')))
		 {
           *pQuery = '\x0';
		   pQuery++;
		   if (*pQuery == 'N' || *pQuery == 'n')
		   {
			 bQuery = FALSE;  // DNSキャシュ無効
		   }
		 }
	     if (IPQuery(AF_INET, pDNS, bQuery, mHostName))
#else
	     if (IPQuery(AF_INET, pDNS, mHostName))
#endif
		 {
	       sts = TRUE; // ブラックリストに掲載
	       break; 
		 }
	   } else 
#endif
#ifdef VC6
	   {
	     phost = getipnodebyname(mHostName, AF_INET, 0, &nErr); //ORDBにレコードがあるか？
		 if (phost) {
		   freehostent(phost);
	       sts = TRUE; // ブラックリストに掲載
	       break; 
		 }
	   }
#else
		phost = NULL;
        memset(&Hints, 0, sizeof(Hints));
		//Hints.ai_flags = AI_NUMERICHOST | AI_PASSIVE;
        Hints.ai_family =  AF_INET;
        Hints.ai_socktype = SOCK_STREAM;
		if (getaddrinfo(mHostName, NULL, &Hints, &AddrInfo) == 0) {
          freeaddrinfo(AddrInfo);
	      sts = TRUE; // ブラックリストに掲載
	      break; 
        }
#endif
	  }
#else
	  phost = gethostbyname(mHostName); //ORDBにレコードがあるか？
	  if (phost) {
        sts = TRUE; // ブラックリストに掲載
	    break; 
	  }
#endif
	  if (bDebug) printf("Query_ORDB(%s) = %s\n", mHostName, (sts ? "found list": "not found list"));

	}
    return sts;
}