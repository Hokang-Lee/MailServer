////////////////////////////////////////////////////////////
// milter.c Copyright by K.kawakami
////////////////////////////////////////////////////////////
// 手順
// connect {ホスト名} {IPアドレス}
// helo {HELO/EHLO で報告したFQDN}
// envelope from {エンベロープのfromアドレス}
// envelope recipient {エンベロープのtoアドレス}
// data
// header {ヘッダ名1:値} 
//   :
// header {ヘッダ名N:値} 
// eoh
// body chunk単位でbodyの文章 （一括送信）
// eom
////////////////////////////////////////////////////////////
// E-POST用 MILTER通信手順
// --------------------------------------------------------------
// 名  称   ID |   内　容                        | 意味
// --------------------------------------------------------------
// M_NEGO    O | {Ver}{Act}{Setp}{0}             | ネゴシェーション
// M_CONNECT C | {HOST}{0}{Family}{Addr}{Port}{0}| 接続
// M_HELO    H | FQDN{0}                         | HELO/EHLO(FQDN)
// M_FROM    M | ENVELOPE FROM{0}                | MAIL FROM
// M_TO      R | ENVELOPE RECIPIENT{0}           | RCPT TO
// M_DATA    T | {0}                             | DATA
// M_HEADER  L | ヘッダ名{0}値{0}                | HEADER LINE
// M_EOH     N | {0}                             | HEADER END
// M_BODY    B | 本文行{0}                       | BODY LINE
// M_EOM     E | {0}                             | BODY END
// M_DISCONN Q |                                 | 切断
// M_ABORT   A |                                 | 切断
// --------------------------------------------------------------
// データ構造
// ----------------------------------------------
// |データサイズ(4byte)|ID(1byte)|データ|
// ----------------------------------------------
////////////////////////////////////////////////////////////

#include "smtp.h"

#ifdef MILTER_ON // MILTERインターフェースを追加。
extern BOOL bServiceTerminating;
extern BOOL bDebug;
extern BOOL bMILTER;         // MILTER機能拡張
extern BOOL bMMACRO;         // MILTER MACRO追加
extern char mMMCONNECT[];
extern char mMMHELO[];
extern char mMMENVFROM[];
extern char mMMENVRCPT[];
extern char mMMEOM[];
extern char mMILTERFile[];
extern char mLocalDomain[];

#ifdef ACCEPT_LOG_LEVEL3
void LEVEL_3_ACCEPTLOG(PCLIENT_CONTEXT lpClientContext, char *mess);
#endif
BOOL MLT_FillAddr(PSOCKADDR_IN psin, char *pServer, int nPort);
void MLT_PIPE_Connection(struct _MILTER *pmlt);
BOOL MLT_SOCKET_Connection(int nAF, struct _MILTER *pmlt);
void MLT_Close(PCLIENT_CONTEXT lpClientContext);

char * move_headerptr(char *p, char *pHead, int idx);
void op_header(char *pLine, char *pLine1, char mod, unsigned idx);
void close_milter_session(struct _MILTER *pmlt);
#ifdef UPDATE_20150210 // E-POST用 MAIL FILTER インターフェースへのマクロ指定を追加。
char * macro_argument(PCLIENT_CONTEXT lpClientContext, int nFlg, unsigned int *sz, char *pMacro, char *pb);
#endif

BOOL MLT_Open(PCLIENT_CONTEXT lpClientContext)
{
   FILE *fp;
   int i;
   int bSts = TRUE;
   struct _MILTER *pmlt;
   struct _NEGODATA nd;
   PSmtpContext pContext = &lpClientContext->Context;
   char *p, *q;
   CHAR mFilter[256];

   lpClientContext->nMilter = 0;
   //if ((fp = fopen("milter.ini", "rt")))
   if ((fp = fopen(mMILTERFile, "rt")))
   {
     while((fgets(mFilter, sizeof(mFilter)-1, fp)))
	 {
	   strlwr(mFilter);
	   if (!strnicmp(mFilter, "input_mail_filter(", 18))
	   {
 	     lpClientContext->nMilter++;
	   }
	 }
	 fclose(fp);
   }
   ///////////////////////////////////
   if (lpClientContext->nMilter > 0)
   {
	 i = 0;
     if ((lpClientContext->pMLT = (struct _MILTER *) malloc(sizeof(struct _MILTER) * lpClientContext->nMilter)))
	 {
       //if ((fp = fopen("milter.ini", "rt")))
       if ((fp = fopen(mMILTERFile, "rt")))
	   {
	     //*s6 = lpClientContext->pMsock6 = (char *) malloc(sizeof(SOCKADDR_IN6)*lpClientContext->nMilter);
	     // milterのアドレス指定の書式は以下のようになります。
         // TCPの場合
         // inet:port@address
         // UNIXドメインソケットの場合
         // unix:/socket_path
         // 例）INPUT_MAIL_FILTER(`filter1′, `S=inet:10000@127.0.0.1, F=T, T=R:1m’)dnl
         // INPUT_MAIL_FILTER('filter1', 'S=local:/var/run/f1.sock, F=R') 
         // INPUT_MAIL_FILTER('filter2', 'S=inet6:999@localhost, F=T, T=S:3m;R:3m;E:8m') 
         // INPUT_MAIL_FILTER('filter3', 'S=inet:3333@localhost')  
         // フラグ（F=）は次のように指定されています。 
         // フラグ
         // 意味
         // R フィルタが使用不可な場合、接続を拒否する 
         // T フィルタが使用不可な場合、一時的に接続を失敗する  
         // F=R または F=T のいずれも指定されていない場合は、フィルタが存在しない場合と同様に、メッセージは通過します。
         // また、フィルタにアクセスする際に sendmail が使用するデフォルトのタイムアウトを、「T=」を使用して上書きすることができます。「T=」内には、次の3つのフィールドがあります。
         // フラグ
         // 意味
         // C フィルタに接続する際のタイムアウト（0 の場合はシステムタイムアウトを使用） 
         // S MTA からフィルタに情報を送信する際のタイムアウト。システム負荷によっては、数分に増やすことが必要な場合もあります。 
         // R フィルタから返答を読み取る際のタイムアウト。この値は、特に、システムがサイズの大きいメッセージを受信している場合、RBL チェックが有効になっている場合、または DNS システムが遅い場合などは、通常、数分に設定してください。  
         // E メッセージ終了をフィルタに送信してから最終的な受け取り通知を受信するまでにかかる時間のタイムアウト。この値は、R の値と同じまたはそれ以上に設定してください。 
         // 各タイムアウト値はセミコロン「;」で区切ってください（コンマ「,」は既に使用されているのでタイムアウトを区切ることはできません）。デフォルト値（config で設定されていない場合）：
         // T=C:5m;E:8m;R:4m;S:2m
         // ここで「s」は秒、「m」は分です。 
         while((fgets(mFilter, sizeof(mFilter)-1, fp)))
		 {
	       strlwr(mFilter);
		   strtok(mFilter, "\r\n");
	       if (!strnicmp(mFilter, "input_mail_filter(", 18))
		   {
	         pmlt =  (struct _MILTER *)((lpClientContext->pMLT)+i);
	         p = q = NULL;
			 pmlt->s = 0;
			 pmlt->macrosz = 0;
			 pmlt->h = NULL;
		     pmlt->sts = -1;
			 pmlt->mMLTMessages[0] = pmlt->mMLTToken[0] = '\x0';
		     pmlt->pType = pmlt->pPort = pmlt->pAddr = pmlt->pAction = pmlt->pCTimer = pmlt->pSTimer = pmlt->pRTimer = pmlt->pETimer = NULL;
             // フィルタ定義解析
	         if ((p = strstr(mFilter, "s="))) // S=
			 {
			   pmlt->pType = p+2;
		       if ((q = strchr(pmlt->pType, ':')))
			   {
			     pmlt->pPort = q + 1;
		         if ((q = strchr(pmlt->pPort, '@')))
				 {
				   if (*(q+1) == '[')
				   {
					 q++;
				   }
			       pmlt->pAddr = q+1;
				 }
			   }
			 }
		     if ((p =strstr(mFilter, "f="))) // F=
			 {
			   pmlt->pAction = p+2;
			 }
		     if ((p =strstr(mFilter, "t="))) // T=
			 {
			   q = p+2;
		       if ((p = strstr(q, "C:")) ||
				   (p = strstr(q, "c:")) )
			   {
			     pmlt->pCTimer = p+2;
			   }
		       if ((p = strstr(q, "S:")) ||
				   (p = strstr(q, "s:")) )
			   {
			     pmlt->pSTimer = p+2;
			   }
		       if ((p = strstr(q, "R:")) ||
				   (p = strstr(q, "r:")) )
			   {
			     pmlt->pRTimer = p+2;
			   }
		       if ((p = strstr(q, "E:")) ||
				   (p = strstr(q, "e:")) )
			   {
			     pmlt->pETimer = p+2;
			   }
			 }
		     if (pmlt->pType)
			 {
			   strtok(pmlt->pType , ":");
			 }
		     if (pmlt->pPort)
			 {
			   strtok(pmlt->pPort, "@,");
			 }
		     if (pmlt->pAddr)
			 {
			   strtok(pmlt->pAddr, ",]'");
			 }
		     if (pmlt->pAction)
			 {
			   strtok(pmlt->pAction, ",");
			 }
		     if (pmlt->pCTimer)
			 {
			   strtok(pmlt->pCTimer, ",");
			 }
		     if (pmlt->pSTimer)
			 {
			   strtok(pmlt->pSTimer, ",");
			 }
		     if (pmlt->pRTimer)
			 {
			   strtok(pmlt->pRTimer, ",");
			 }
		     if (pmlt->pETimer)
			 {
			  strtok(pmlt->pETimer, ",");
			 }  
             ///////////////////////////////////////////		 
			 pmlt->mType[0] = pmlt->mPort[0] = pmlt->mAddr[0] = pmlt->mAction[0] ='\x0';
			 if (pmlt->pType)
			 {
			   strcpy(pmlt->mType, pmlt->pType);
			 }
			 if (pmlt->pPort)
			 {
			   strcpy(pmlt->mPort, pmlt->pPort);
			 }
			 if (pmlt->pAddr)
			 {
			   strcpy(pmlt->mAddr, pmlt->pAddr);
			 }
			 if (pmlt->pAction)
			 {
			   strcpy(pmlt->mAction, pmlt->pAction);
			 }
			 pmlt->mCTimer[0] = pmlt->mSTimer[0] = pmlt->mRTimer[0] = pmlt->mETimer[0] ='\x0';
			 if (pmlt->pCTimer)
			 {
			   strcpy(pmlt->mCTimer, pmlt->pCTimer);
			   strtok(pmlt->mCTimer, "';");
			 }
			 if (pmlt->pSTimer)
			 {
			   strcpy(pmlt->mSTimer, pmlt->pSTimer);
			   strtok(pmlt->mSTimer, "';");
			 }
			 if (pmlt->pRTimer)
			 {
			   strcpy(pmlt->mRTimer, pmlt->pRTimer);
			   strtok(pmlt->mRTimer, "';");
			 }
			 if (pmlt->pETimer)
			 {
  			   strcpy(pmlt->mETimer, pmlt->pETimer);
			   strtok(pmlt->mETimer, "';");
			 }
             ///////////////////////////////////////////		 
			 /*
	         if (strstr(pmlt->pType, "local")) // PIPE
			 {
			   MLT_PIPE_Connection(pmlt);
			 } else 
			 */
			 if (MLT_SOCKET_Connection((strstr(mFilter, "s=inet6") ? 1 : 0), pmlt) == INVALID_SOCKET)
			 {
			   if (pmlt->mAction[0])
			   {
			     bSts = FALSE;
			     break;
			   }
			 } else {
			   // ネゴシェーション
			   unsigned long ver, act, step;
               nd.sz = pmlt->nMLTLen = htonl(12+1); //ver,ac,stepネットワークバイトオーダに変換
               nd.id = (char)M_NEGO;
               ver = htonl(M_VERSION); // version
               act = htonl(0x000001FFL);//0x001FFFFFL); // action //ヘッダー追加, 本文変更,宛先追加,差出人変更 #define SMFI_CURR_PROT	0x001FFFFFL
               step = htonl(0x001FFFFFL); //0x000003FL); //0x0000L); // step #define SMFI_CURR_ACTS	0x000001FFL
               //nd.macro = NULL; // macro
               memcpy(nd.data, (char *)&ver, sizeof(unsigned long));
			   memcpy(nd.data+sizeof(unsigned long), (char *)&act, sizeof(unsigned long));
			   memcpy(nd.data+sizeof(unsigned long)*2, (char *)&step, sizeof(unsigned long));
			   memcpy(pmlt->mMLTToken, (char *)&nd, sizeof(struct _NEGODATA));
               if (!(bSts = MLT_ACCESS(lpClientContext, i, TRUE)))
			   {
		         break;
			   }
			   memcpy(&pmlt->mFnd, pmlt->mMLTMessages, sizeof(struct _NEGODATA)); // メールフィルタ側の要求を設定
               ver = ntohl(*(unsigned long *)&pmlt->mFnd.data); // version
               act = ntohl(*(unsigned long *)&pmlt->mFnd.data[sizeof(unsigned long)]); // action
               step = ntohl(*(unsigned long *)&pmlt->mFnd.data[sizeof(unsigned long)*2]); // step
			   printf("[NEGOTIATION] [%08x] [%08x] [%08x] [%08x]\r\n", ntohl(pmlt->mFnd.sz), ver, act, step);
			   if (ver == 0L && act == 0L && step == 0L) // バージョンエラーなら切断
			   {
                 close_milter_session(pmlt);
				 /*
				 if (pmlt->s) // ソケットだけクローズ
				 {
	               closesocket(pmlt->s);
		           pmlt->s = INVALID_SOCKET;
				 }
				 */
			   }
			 }
		     i++;
		   }
		 }
	     fclose(fp);
	   }
	 }
   }
   return bSts;
}

void MLT_Close(PCLIENT_CONTEXT lpClientContext)
{
   int i;
   struct _MILTER *pmlt;
   unsigned int *pl;

   for (i=0; i < lpClientContext->nMilter; i++)
   {
	 pmlt =  (struct _MILTER *)((lpClientContext->pMLT)+i);
	 if (pmlt->s != INVALID_SOCKET)
	 {
	   // MILTER側に切断要求
       pmlt->mMLTToken[4] = M_ABORT; //M_DISCONN;
	   //sprintf(&pmlt->mMLTToken[5], "\r\n");
       pmlt->nMLTLen = 0; //strlen(&pmlt->mMLTToken[5])+1;
	   pl = (unsigned int *)pmlt->mMLTToken;
	   *pl = (unsigned int)0L; //strlen(&pmlt->mMLTToken[5]);
       //MLT_ACCESS(lpClientContext, i, TRUE);
       // 自身も解放
       close_milter_session(pmlt);
	   /*
	   if (pmlt->s)
	   {
	     closesocket(pmlt->s);
		 pmlt->s = INVALID_SOCKET;
	   }
	   if (pmlt->h)
	   {
	     CloseHandle(pmlt->h);
		 pmlt->h = NULL;
	   }
	   */
	 }
   }
   if (lpClientContext->nMilter > 0 && lpClientContext->pMLT)
   {
	  free(lpClientContext->pMLT);
	  lpClientContext->pMLT = NULL;
   }
}

void MLT_PIPE_Connection(struct _MILTER *pmlt)
{
  OVERLAPPED Overlapped;

  //LPTSTR  lpszPipeName = TEXT("\\\\.\\pipe");
  CHAR szPipeName[256];

  sprintf(szPipeName, "\\\\.\\pipe%s", pmlt->mPort);
  pmlt->h = CreateFile(szPipeName,  // ファイル名
                     GENERIC_READ | GENERIC_WRITE, // アクセスモード
                     0, //FILE_SHARE_READ | FILE_SHARE_WRITE, // 共有モード
                     NULL, // セキュリティ記述子
                     CREATE_NEW,                // 作成方法
                     FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_OVERLAPPED, //FILE_ATTRIBUTE_NORMAL,                 // ファイル属性
                     NULL);                        // テンプレートファイルのハンドル
}


BOOL MLT_SOCKET_Connection(int nAF, struct _MILTER *pmlt)
{

   SOCKET msock;
   int    nSockErr;
   int    nSndsize, nl;
#ifdef IPv6
    struct addrinfo Hints, *AddrInfo, *AI, *AI2;
	char   mport[8];
	int    RetVal;
#else
   SOCKADDR_IN  dest_sin;  /* DESTination Socket INternet */
   short        h_length;
#endif
   int timeout;// = 60000 * 60; // 受信タイムアウトは60分;
   char *p;


#ifdef IPv6
    //nAF = nAddressFamily;
    memset(&Hints, 0, sizeof(Hints));
	//不要 Hints.ai_flags = AI_NUMERICHOST | AI_PASSIVE;
    Hints.ai_family =  (nAF ? AF_INET6 : AF_INET);
    Hints.ai_socktype = SOCK_STREAM;
	//itoa(nPort, mport, 10);
    RetVal = getaddrinfo(pmlt->mAddr, pmlt->mPort, &Hints, &AddrInfo);
    if (RetVal != 0) {
      if (bDebug)
        printf("Cannot resolve address [%s] and port [%s], error %d: %s\n", pmlt->mAddr, pmlt->mPort, RetVal, gai_strerror(RetVal));
        //WSACleanup();
        return FALSE; // INVALID_SOCKET
    }
	AI = AddrInfo;
    pmlt->s = socket((nAF ? AF_INET6 : AF_INET), SOCK_STREAM, 0);
    if (pmlt->s != INVALID_SOCKET) 
	{
      if (pmlt->mSTimer[0]) {
		//strtok(pmlt->mSTimer, "';");
        timeout = (int)atoi(pmlt->mSTimer)*1000; // 1秒単位
		if (timeout > 0)
		{
		  p = &pmlt->mSTimer[strlen(pmlt->mSTimer)-1];
		  if (*p='m')
		  {
		    timeout *=60; // 分に設定
		  }
          if (setsockopt( pmlt->s, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout)) != SOCKET_ERROR)
            if (bDebug)
		      printf("Set SO_SNDTIMEO success\n");
		}
	  } 
      if (pmlt->mRTimer[0]) {
		//strtok(pmlt->mRTimer, "';");
        timeout = (int)atoi(pmlt->mRTimer)*1000; // 1秒単位
		if (timeout > 0)
		{
		  p = &pmlt->mRTimer[strlen(pmlt->mRTimer)-1];
		  if (*p='m')
		  {
		    timeout *=60; // 分に設定
		  }
          if (setsockopt( pmlt->s, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) != SOCKET_ERROR)
            if (bDebug)
		      printf("Set SO_RCVTIMEO success\n");
		}
	  }
	  AI2 = AI;
      if (connect(pmlt->s, AI->ai_addr, AI->ai_addrlen) == SOCKET_ERROR) {// < 0) {
        closesocket( pmlt->s );
		pmlt->s = INVALID_SOCKET;
        if (bDebug)
          printf("winsock connect failed.\n");
	  } else {
        if (bDebug)
          printf("winsock connect success.\n");
	  }
	} else {
      closesocket( pmlt->s );
	  pmlt->s = INVALID_SOCKET;
	}
	freeaddrinfo(AddrInfo);
#else
    pmlt->s = socket( AF_INET, SOCK_STREAM, 0);
    if (pmlt->s != INVALID_SOCKET) {
      if (pmlt->mSTimer[0]) {
		//strtok(pmlt->mSTimer, "';");
        timeout = atoi(pmlt->mSTimer)*1000; // 1秒単位
		p = &pmlt->mSTimer[strlen(pmlt->mSTimer)-1];
		if (*p='m')
		{
		  timeout *=60; // 分に設定
		}
        if (setsockopt( pmlt->s, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout)) != SOCKET_ERROR)
          if (bDebug)
		    printf("Set SO_SNDTIMEO success\n");
	  } 
      if (pmlt->mRTimer[0]) {
		//strtok(pmlt->mRTimer[0], "';");
        timeout = (int)atoi(pmlt->mRTimer)*1000; // 1秒単位
		p = &pmlt->mRTimer[strlen(pmlt->mRTimer)-1];
		if (*p='m')
		{
		  timeout *=60; // 分に設定
		}
        if (setsockopt( pmlt->s, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) != SOCKET_ERROR)
          if (bDebug)
		    printf("Set SO_RCVTIMEO success\n");
	  }
      if (MLT_FillAddr( &dest_sin, pmlt->mAddr, atoi(pmlt->mPort))) 
	  {
        if (connect( pmlt->s, (PSOCKADDR) &dest_sin, sizeof( dest_sin)) == SOCKET_ERROR) {
          closesocket( pmlt->s );
		  pmlt->s = INVALID_SOCKET;
          if (bDebug)
           printf("winsock connect failed. (socket error code=%d)\n", nSockErr);
		} else {
          if (bDebug)
            printf("winsock connect success.\n", pNo);
		}
	  } else {
        closesocket( pmlt->s );
	    pmlt->s = INVALID_SOCKET;
	  }
    } else {
      pmlt->s = INVALID_SOCKET;
      if (bDebug)
        printf("socket() failed Error\n");
    }
#endif
    return pmlt->s;
  
}

BOOL MLT_FillAddr(PSOCKADDR_IN psin, char *pServer, int nPort)
{
   DWORD dwSize;
   PHOSTENT phe;
   //char szTemp[200];

   psin->sin_family = AF_INET;
   if ((phe = gethostbyname(pServer)) == NULL)
   {
	  //printf("%d is the error. Make sure '%s' is listed in the hosts file.\n", WSAGetLastError(), mserver);
      //printf("gethostbyname() failed.\n");
      return FALSE;
   }
   memcpy((char FAR *)&(psin->sin_addr), phe->h_addr, phe->h_length);
   psin->sin_port = htons((u_short)nPort);        // Convert to network ordering

   return TRUE;
}

BOOL MLT_ACCESS(PCLIENT_CONTEXT lpClientContext, int i, BOOL bRecv)
{
   BOOL bSts = TRUE;
   //int i;
   int  err;
   char mec[128];
#ifdef UPDATE_20150216 // E-POST用 MAIL FILTER 処理の詳細ログへの記録を追加した。
   CHAR mCmd[21], mLog[512];
#endif
   struct _MILTER *pmlt;
   PSmtpContext pContext = &lpClientContext->Context;
   DWORD   bytesRead;
   OVERLAPPED Overlapped;
   int n, l, remain;
   char *p;
   unsigned int *pl, sz, step;
   struct _FILTERDATA *pfd, *pfd2;

#ifdef UPDATE_20150216 // E-POST用 MAIL FILTER 処理の詳細ログへの記録を追加した。
#ifdef ACCEPT_LOG_LEVEL3
   sprintf(mLog, "[MLT_ACCESS] [%d] start", i);
   LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
#endif
   mCmd[0] ='\0';
#endif
   //for (i=0; i < lpClientContext->nMilter; i++)
   {
	 pmlt =  (struct _MILTER *)((lpClientContext->pMLT)+i);
	 if (pmlt->s != INVALID_SOCKET)
	 {

	   pmlt->mMLTMessages[0] = '\x0';
	   if (pmlt->h)
	   {
		 bytesRead = pmlt->nMLTLen;
	     TransactNamedPipe(pmlt->h,  // 名前付きパイプのハンドル
                         pmlt->mMLTToken, sizeof(pmlt->mMLTToken),
                         pmlt->mMLTMessages, sizeof(pmlt->mMLTMessages),
                         &bytesRead,
					     &Overlapped);
	   } else if (pmlt->s)
	   {
		 pfd = (struct _FILTERDATA *)pmlt->mMLTToken;
		 // SETPに対応する応答方法
	 	 step = ntohl(*(unsigned long *)&pmlt->mFnd.data[sizeof(unsigned long)*2]);
#ifdef UPDATE_20150216 // E-POST用 MAIL FILTER 処理の詳細ログへの記録を追加した。
		 switch(pfd->id)
		 {
		   case M_NEGO:    strcpy(mCmd, "M_NEGO"); break;
		   case M_CONNECT: strcpy(mCmd, "M_CONNECT"); bRecv = (step & STEP_MFT_NOCONN ? FALSE : TRUE); break;
		   case M_HELO:    strcpy(mCmd, "M_HELO"); bRecv = (step & STEP_MFT_NOHELO ? FALSE : TRUE); break;
		   case M_FROM:    strcpy(mCmd, "M_FROM"); bRecv = (step & STEP_MFT_NOFROM ? FALSE : TRUE); break;
		   case M_RCPT:    strcpy(mCmd, "M_RCPT"); bRecv = (step & STEP_MFT_NOTO ? FALSE : TRUE);   break;
		   case M_DATA:    strcpy(mCmd, "M_DATA"); bRecv = (step & STEP_MFT_NODATA ? FALSE : TRUE); break;
		   case M_HEADER:  strcpy(mCmd, "M_HEADER"); break;
           case M_EOH:     strcpy(mCmd, "M_EOH"); bRecv = (step & STEP_MFT_NOEOH ? FALSE : TRUE);  break;
           case M_BODY:    strcpy(mCmd, "M_BODY"); bRecv = (step & STEP_MFT_NOBODY ? FALSE : TRUE); break;
           case M_EOM:     strcpy(mCmd, "M_EOM"); break;
		   case M_DISCONN: strcpy(mCmd, "M_DISCONN"); break;
		   case M_MACRO:
		        switch(*(&pfd->id+1))
				{
		          case M_CONNECT: strcpy(mCmd, "M_MACRO+M_CONNECT"); bRecv = (step & STEP_MFT_NOCONN ? FALSE : TRUE); break;
		          case M_HELO:    strcpy(mCmd, "M_MACRO+M_HELO"); bRecv = (step & STEP_MFT_NOHELO ? FALSE : TRUE); break;
		          case M_FROM:    strcpy(mCmd, "M_MACRO+M_FROM"); bRecv = (step & STEP_MFT_NOFROM ? FALSE : TRUE); break;
		          case M_RCPT:    strcpy(mCmd, "M_MACRO+M_RCPT"); bRecv = (step & STEP_MFT_NOTO ? FALSE : TRUE);   break;
		          case M_DATA:    strcpy(mCmd, "M_MACRO+M_DATA"); bRecv = (step & STEP_MFT_NODATA ? FALSE : TRUE); break;
		          case M_HEADER:  strcpy(mCmd, "M_MACRO+M_HEADER"); break;
                  case M_EOH:     strcpy(mCmd, "M_MACRO+M_EOH"); bRecv = (step & STEP_MFT_NOEOH ? FALSE : TRUE);  break;
                  case M_BODY:    strcpy(mCmd, "M_MACRO+M_BODY"); bRecv = (step & STEP_MFT_NOBODY ? FALSE : TRUE); break;
                  case M_EOM:     strcpy(mCmd, "M_MACRO+M_EOM"); break;
		          case M_DISCONN: strcpy(mCmd, "M_MACRO+M_DISCONN"); break;
		          default:        strcpy(mCmd, "M_MACRO+M_UNKOWN"); bRecv = (step & STEP_MFT_NOUNKOWN ? FALSE : TRUE); break;
				}
				break;
		   default:        strcpy(mCmd, "M_UNKOWN"); bRecv = (step & STEP_MFT_NOUNKOWN ? FALSE : TRUE); break;
		 }	     
#else
	     switch(pfd->id)
		 {
		   case M_NEGO:    break;
		   case M_CONNECT: bRecv = (step & STEP_MFT_NOCONN ? FALSE : TRUE); break;
		   case M_HELO:    bRecv = (step & STEP_MFT_NOHELO ? FALSE : TRUE); break;
		   case M_FROM:    bRecv = (step & STEP_MFT_NOFROM ? FALSE : TRUE); break;
		   case M_RCPT:    bRecv = (step & STEP_MFT_NOTO ? FALSE : TRUE);   break;
		   case M_DATA:    bRecv = (step & STEP_MFT_NODATA ? FALSE : TRUE); break;
		   case M_HEADER:  break;
           case M_EOH:     bRecv = (step & STEP_MFT_NOEOH ? FALSE : TRUE);  break;
           case M_BODY:    bRecv = (step & STEP_MFT_NOBODY ? FALSE : TRUE); break;
           case M_EOM:     break;
		   case M_DISCONN: break;
		   default:        bRecv = (step & STEP_MFT_NOUNKOWN ? FALSE : TRUE); break;
		 }	     
#endif
         /////////////////////////////////////
		 if (pmlt->macrosz == 0)
		 {
		   sz = ntohl(pfd->sz)+4;
		 } else {
		   pfd2 = (struct _FILTERDATA *)&pmlt->mMLTToken[ntohl(pfd->sz)+4];
		   sz = ntohl(pfd->sz)+4 + ntohl(pfd2->sz)+4;
		 }
		 /*
#ifdef UPDATE_20150216 // E-POST用 MAIL FILTER 処理の詳細ログへの記録を追加した。
#ifdef ACCEPT_LOG_LEVEL3
	     sprintf(mLog, "[MLT_ACCESS] [%d] [%s] send() start", i, mCmd);
         LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
#endif
#endif
		 */
         err = send(pmlt->s, pmlt->mMLTToken, sz, 0 );
#ifdef UPDATE_20150216 // E-POST用 MAIL FILTER 処理の詳細ログへの記録を追加した。
#ifdef ACCEPT_LOG_LEVEL3
	     sprintf(mLog, "[MLT_ACCESS] [%d] [%s] send() end. len = %d", i, mCmd, err);
         LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
#endif
#endif
         if ( err == SOCKET_ERROR ) 
		 {
#ifdef UPDATE_20150216 // E-POST用 MAIL FILTER 処理の詳細ログへの記録を追加した。
           err = WSAGetLastError();
           switch (err) 
		   {
             case WSANOTINITIALISED: sprintf(mec, "send() SOCKET_ERROR WSANOTINITIALISED=%d", err); break;
  	         case WSAENETDOWN: sprintf(mec, "send() SOCKET_ERROR WSAENETDOWN=%d", err); break;
	         case WSAEFAULT: sprintf(mec, "send() SOCKET_ERROR WSAEFAULT=%d", err); break;
	         case WSAEINPROGRESS: sprintf(mec, "send() SOCKET_ERROR WSAEINPROGRESS=%d", err); break;
  	         case WSAEINVAL: sprintf(mec, "send() SOCKET_ERROR WSAEINVAL=%d", err); break;
	         case WSAEMFILE: sprintf(mec, "send() SOCKET_ERROR WSAEMFILE=%d", err); break;
	         case WSAENOBUFS: sprintf(mec, "send() SOCKET_ERROR WSAENOBUFS=%d", err); break;
	         case WSAENOTSOCK: sprintf(mec, "send() SOCKET_ERROR WSAENOTSOCK=%d", err); break;
	         case WSAEOPNOTSUPP: sprintf(mec, "send() SOCKET_ERROR WSAEOPNOTSUPP=%d", err); break;
	         case WSAEWOULDBLOCK: sprintf(mec, "send() SOCKET_ERROR WSAEWOULDBLOCK=%d", err); break;
	         default:  sprintf(mec, "send() SOCKET_ERROR OTHER CODE=%d", err); break;
		   }
#ifdef ACCEPT_LOG_LEVEL3
	       sprintf(mLog, "[MLT_ACCESS] [%d] [%s] send() %s", i, mCmd, mec);
           LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
#endif
#endif
		   if (pmlt->mAction[0] == 'r' || pmlt->mAction[0] == 'R')
		   {
             strcpy(pContext->mMessages, SMTP_BAD_MAILFILTER); // 応答メッセージ書換え
		     bSts = FALSE;
		   } else if (pmlt->mAction[0] == 't' || pmlt->mAction[0] == 'T')
		   {
             strcpy(pContext->mMessages, SMTP_MAILFILTER_STOP); // 応答メッセージ書換え
		     bSts = FALSE;
		   } 
           //break;
		 }
		 if (bSts && bRecv)
		 {
		   memset(pmlt->mMLTMessages, 0, 640);
		   pfd = (struct _FILTERDATA *)pmlt->mMLTMessages;
		   //pfd->pData = (char *)&pmlt->mMLTMessages[5];
		   n = 0;
		   //
#ifdef UPDATE_20171102B // MILTERで複数ヘッダの追加変更が失敗しないようにする対策
nexttoken:
#endif
		   do {
	         l = 0;
             if (n >= (sizeof(pmlt->mMLTMessages)-1)) 
			 {
 	           if (bDebug) printf("[%08x] ERROR = overflow of token. [pmlt=%08x]\n",lpClientContext, pmlt);
               return FALSE;
			 }
	         remain = sizeof(pmlt->mMLTMessages) - 1 - n;
			 /*
#ifdef UPDATE_20150216 // E-POST用 MAIL FILTER 処理の詳細ログへの記録を追加した。
#ifdef ACCEPT_LOG_LEVEL3
	         sprintf(mLog, "[MLT_ACCESS] [%d] [%s] recv() start", i, mCmd);
             LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
#endif
#endif
			 */
//printf("recv() start. remain=%d, n=%d\n", remain, n);
             l = recv(pmlt->s, &pmlt->mMLTMessages[n], remain, 0);
/*
{
  int x;
  for (x = 0; x < l; x++)
  {
  	printf("%02x ", pmlt->mMLTMessages[x]);
  }
  printf("\n");
}
*/
#ifdef UPDATE_20150216 // E-POST用 MAIL FILTER 処理の詳細ログへの記録を追加した。
#ifdef ACCEPT_LOG_LEVEL3
	         sprintf(mLog, "[MLT_ACCESS] [%d] [%s] recv() end. len = %d", i, mCmd, l);
             LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
#endif
#endif
	         if (l == 0) { // 0 = gracefully closed. // 丁寧な終了なら受信終了
	           break;  // これを有効にするとCPU負荷１００%になる場合がある。
			 } else 
	         if (l == SOCKET_ERROR) 
			 {
	           err = WSAGetLastError();
	           switch (err) 
			   {
	             case WSANOTINITIALISED: sprintf(mec, "read() SOCKET_ERROR WSANOTINITIALISED=%d", err); break;
	  	         case WSAENETDOWN: sprintf(mec, "read() SOCKET_ERROR WSAENETDOWN=%d", err); break;
		         case WSAEFAULT: sprintf(mec, "read() SOCKET_ERROR WSAEFAULT=%d", err); break;
		         case WSAEINPROGRESS: sprintf(mec, "read() SOCKET_ERROR WSAEINPROGRESS=%d", err); break;
		         case WSAEINVAL: sprintf(mec, "read() SOCKET_ERROR WSAEINVAL=%d", err); break;
		         case WSAEMFILE: sprintf(mec, "read() SOCKET_ERROR WSAEMFILE=%d", err); break;
		         case WSAENOBUFS: sprintf(mec, "read() SOCKET_ERROR WSAENOBUFS=%d", err); break;
		         case WSAENOTSOCK: sprintf(mec, "read() SOCKET_ERROR WSAENOTSOCK=%d", err); break;
		         case WSAEOPNOTSUPP: sprintf(mec, "read() SOCKET_ERROR WSAEOPNOTSUPP=%d", err); break;
		         case WSAEWOULDBLOCK: sprintf(mec, "read() SOCKET_ERROR WSAEWOULDBLOCK=%d", err); break;
		         default:  sprintf(mec, "get_reply() SOCKET_ERROR OTHER CODE=%d", err); break;
			   }
               if (bDebug) printf("[%08x] RECV[%08x] ERROR<- [%s]\n", pmlt, pmlt->s, mec);
	           if (bDebug) printf("[%08x] %s\n",pmlt, mec);
#ifdef UPDATE_20150216 // E-POST用 MAIL FILTER 処理の詳細ログへの記録を追加した。
#ifdef ACCEPT_LOG_LEVEL3
	           sprintf(mLog, "[MLT_ACCESS] [%d] [%s] recv() %s", i, mCmd, mec);
               LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
#endif
#endif
			   if (pmlt->mAction[0] == 'r' || pmlt->mAction[0] == 'R')
			   {
                 strcpy(pContext->mMessages, SMTP_BAD_MAILFILTER); // 応答メッセージ書換え
			     bSts = FALSE;
			   } else if (pmlt->mAction[0] == 't' || pmlt->mAction[0] == 'T')
			   {
                 strcpy(pContext->mMessages, SMTP_MAILFILTER_STOP); // 応答メッセージ書換え
			     bSts = FALSE;
			   } 
               break;
			 } 
             n = n+l;
	         pmlt->mMLTMessages[n] = '\x0';
			 if (ntohl(pfd->sz)+4 != n)
			 {
		       _sleep(0); // 他スレッドに切替
			 }
		   } while(ntohl(pfd->sz)+4 > n);
		   if (bSts)
		   {
anyflag:
/*
{
  int x;
  printf("l=%d\n", l);
  for (x = 0; x < l; x++)
  {
  	printf("%02x ", pmlt->mMLTMessages[x]);
  }
  printf("\n");
}
*/
			 switch(pfd->id)
			 {
			   case F_RCPTADD2: //'2' // パラメーター付きで宛先追加: <rcpt-parameters>付きで宛先を追加する。
#ifdef UPDATE_20150216 // E-POST用 MAIL FILTER 処理の詳細ログへの記録を追加した。
				   strcpy(mCmd, "F_RCPTADD2");
#endif
				   {
				     char *p;
					 if ((p = strchr(&pmlt->mMLTMessages[5], '<')))
					 {
					   p++;
					   strtok(p, ">");
					 } else 
					 {
					   p = &pmlt->mMLTMessages[5];
					 }
					 {
				       FILE *fp;
					   if ((fp= fopen(pContext->mFnHead, "at")))
					   {
					     fprintf(fp, "Recipient: %s\n", p);
					     fclose(fp);
					   }
					 }
				   }
#ifdef UPDATE_20171102B // MILTERで複数ヘッダの追加変更が失敗しないようにする対策
				   goto nexttoken;
#endif
				   n = 0;
				   break;
			   case F_RCPTADD:  //'+' // 宛先追加: 宛先を追加する応答。複数の宛先を追加するときは複数回送る。
#ifdef UPDATE_20150216 // E-POST用 MAIL FILTER 処理の詳細ログへの記録を追加した。
				   strcpy(mCmd, "F_RCPTADD");
#endif
				   {
				     FILE *fp;
					 if ((fp= fopen(pContext->mFnHead, "at")))
					 {
					   fprintf(fp, "Recipient: %s\n", &pmlt->mMLTMessages[5]);
					   fclose(fp);
					 }
				   }
#ifdef UPDATE_20171102B // MILTERで複数ヘッダの追加変更が失敗しないようにする対策
				   goto nexttoken;
#endif
				   n = 0;
				   break;
			   case F_HEADINS:  //'i' // ヘッダー挿入: 指定した位置にヘッダーを挿入する応答。
				                // index{0} | field{0} | val{0} |
#ifdef UPDATE_20150216 // E-POST用 MAIL FILTER 処理の詳細ログへの記録を追加した。
				   strcpy(mCmd, "F_HEADINS");
#endif
				   {
				     FILE *fp;
					 unsigned int *p, *q;
					 CHAR mFnData[256];
					 CHAR mLine[2048];
					 unsigned int i;
					 unsigned int sz, idx;
					 char *filed, *val;

					 strcpy(mFnData, pContext->mFnData);
					 mFnData[strlen(mFnData)-2] = 'E';
					 if ((fp= fopen(mFnData, "ab")))
					 {
#ifdef UPDATE_20171102B // MILTERで複数ヘッダの追加変更が失敗しないようにする対策
					   p = (unsigned int *)&pmlt->mMLTMessages;
					   sz = ntohl(*p);
					   l-=sz+4;
					   p = (unsigned int *)&pmlt->mMLTMessages[5];
					   idx = ntohl(*p);
					   filed = &pmlt->mMLTMessages[sizeof(unsigned int)+5];
					   val = filed + strlen(filed) + 1;
					   fprintf(fp, "I:%u:%s:%s", idx, filed, val);
					   if (*(val+strlen(val)-1) != '\r' &&
						   *(val+strlen(val)-1) != '\n')
					   {
                         fprintf(fp, "\r\n");
					   }
#else
					   p = (unsigned int *)&pmlt->mMLTMessages;
					   sz = ntohl(*p);
					   p = (unsigned int *)&pmlt->mMLTMessages[5];
					   idx = ntohl(*p);
					   filed = &pmlt->mMLTMessages[sizeof(unsigned int)+5];
					   val = filed + strlen(filed) + 1;
					   if (*(val+strlen(val)-1) != '\r' &&
						   *(val+strlen(val)-1) != '\n')
					   {
                         strcat(val, "\r\n");
					   }
					   fprintf(fp, "I:%u:%s:%s", idx, filed, val);
#endif
					   fclose(fp);
					 }
					 while(!(fp= fopen(mFnData, "rb"))) 
					 {
	                   if (bServiceTerminating)
	                     break;
                       _sleep(100);
					 }
					 fclose(fp);
#ifdef UPDATE_20171102B // MILTERで複数ヘッダの追加変更が失敗しないようにする対策
				     if (l > 0)
					 {
			           for (i = 0; i < l; i++)
					   {
					     pmlt->mMLTMessages[i] = pmlt->mMLTMessages[sz+4+i];
					   } 
					   pmlt->mMLTMessages[i] = pmlt->mMLTMessages[sz+4+i];
                       goto anyflag;
					 }
#endif
				   }
				   n = 0;
#ifdef UPDATE_20171102B // MILTERで複数ヘッダの追加変更が失敗しないようにする対策
				   goto nexttoken;
#endif
				   break;
			   case F_HEADMOD:  //'m' // ヘッダー変更: 指定した位置のヘッダーを変更する。
				                // index{0} | field{0} | val{0} |
#ifdef UPDATE_20150216 // E-POST用 MAIL FILTER 処理の詳細ログへの記録を追加した。
				   strcpy(mCmd, "F_HEADMOD");
#endif
				   {
				     FILE *fp;
					 unsigned int *p, *q;
					 CHAR mFnData[256];
					 CHAR mLine[2048];
					 unsigned int i;
					 unsigned int sz, idx;
					 char *filed, *val;

					 strcpy(mFnData, pContext->mFnData);
					 mFnData[strlen(mFnData)-2] = 'E';
					 if ((fp= fopen(mFnData, "ab")))
					 {
#ifdef UPDATE_20171102B // MILTERで複数ヘッダの追加変更が失敗しないようにする対策
					   p = (unsigned int *)&pmlt->mMLTMessages;
					   sz = ntohl(*p);
					   l-=sz+4;
					   p = (unsigned int *)&pmlt->mMLTMessages[5];
					   idx = ntohl(*p);
					   filed = &pmlt->mMLTMessages[sizeof(unsigned int)+5];
					   val = filed + strlen(filed) + 1;
					   fprintf(fp, "M:%u:%s:%s", idx, filed, val);
					   if (*(val+strlen(val)-1) != '\r' &&
						   *(val+strlen(val)-1) != '\n')
					   {
                         fprintf(fp, "\r\n");
					   }
#else
					   p = (unsigned int *)&pmlt->mMLTMessages;
					   sz = ntohl(*p);
					   p = (unsigned int *)&pmlt->mMLTMessages[5];
					   idx = ntohl(*p);
					   filed = &pmlt->mMLTMessages[sizeof(unsigned int)+5];
					   val = filed + strlen(filed) + 1;
					   if (*(val+strlen(val)-1) != '\r' &&
						   *(val+strlen(val)-1) != '\n')
					   {
                         strcat(val, "\r\n");
					   }
					   fprintf(fp, "M:%u:%s:%s", idx, filed, val);
#endif
					   fclose(fp);
					 }
					 while(!(fp= fopen(mFnData, "rb"))) 
					 {
	                   if (bServiceTerminating)
	                     break;
                       _sleep(100);
					 }
					 fclose(fp);
#ifdef UPDATE_20171102B // MILTERで複数ヘッダの追加変更が失敗しないようにする対策
				     if (l > 0)
					 {
			           for (i = 0; i < l; i++)
					   {
					     pmlt->mMLTMessages[i] = pmlt->mMLTMessages[sz+4+i];
					   }
					   pmlt->mMLTMessages[i] = pmlt->mMLTMessages[sz+4+i];
                       goto anyflag;
					 }
#endif
				   }
				   n = 0;
#ifdef UPDATE_20171102B // MILTERで複数ヘッダの追加変更が失敗しないようにする対策
				   goto nexttoken;
#endif
				   break;
			   case F_HEADADD:  //'h' // ヘッダー追加: ヘッダーを追加する応答。複数のヘッダーを追加するときは複数回送る。ヘッダーは末尾に追加される*7。
				                // field{0} | val{0} |
#ifdef UPDATE_20150216 // E-POST用 MAIL FILTER 処理の詳細ログへの記録を追加した。
				   strcpy(mCmd, "F_HEADADD");
#endif
				   {
				     FILE *fp;
					 unsigned int *p, sz;
					 CHAR mFnData[256];
					 CHAR mLine[2048];
					 unsigned int i;
					 char *filed, *val;

					 strcpy(mFnData, pContext->mFnData);
					 mFnData[strlen(mFnData)-2] = 'H';
					 if ((fp= fopen(mFnData, "ab")))
					 {
#ifdef UPDATE_20171102B // MILTERで複数ヘッダの追加変更が失敗しないようにする対策
					   p = (unsigned int *)&pmlt->mMLTMessages;
					   sz = ntohl(*p);
					   l-=sz+4;
					   filed = &pmlt->mMLTMessages[5];
					   val = filed + strlen(filed) + 1;
					   fprintf(fp, "%s:%s", filed, val);
					   if (*(val+strlen(val)-1) != '\r' &&
						   *(val+strlen(val)-1) != '\n')
					   {
                         fprintf(fp, "\r\n");
					   }
#else
					   p = (unsigned int *)&pmlt->mMLTMessages;
					   sz = ntohl(*p);
					   filed = &pmlt->mMLTMessages[5];
					   val = filed + strlen(filed) + 1;
					   if (*(val+strlen(val)-1) != '\r' &&
						   *(val+strlen(val)-1) != '\n')
					   {
                         strcat(val, "\r\n");
					   }
					   fprintf(fp, "%s:%s", filed, val);
#endif
					   fclose(fp);
					 }
					 while(!(fp= fopen(mFnData, "rb"))) 
					 {
	                   if (bServiceTerminating)
	                     break;
                       _sleep(100);
					 }
					 fclose(fp);
#ifdef UPDATE_20171102B // MILTERで複数ヘッダの追加変更が失敗しないようにする対策
				     if (l > 0)
					 {
			           for (i = 0; i < l; i++)
					   {
					     pmlt->mMLTMessages[i] = pmlt->mMLTMessages[sz+4+i];
					   } 
					   pmlt->mMLTMessages[i] = pmlt->mMLTMessages[sz+4+i];
                       goto anyflag;
					 }
#endif
				   }
				   n = 0;
#ifdef UPDATE_20171102B // MILTERで複数ヘッダの追加変更が失敗しないようにする対策
				   goto nexttoken;
#endif
				   break;
			   case F_RCPTDEL:  //'-' // 宛先削除: 宛先を削除する応答。
#ifdef UPDATE_20150216 // E-POST用 MAIL FILTER 処理の詳細ログへの記録を追加した。
				   strcpy(mCmd, "F_RCPTDEL");
#endif
				   {
				     FILE *fp;
					 unsigned int *p, sz;
					 CHAR mFnHead[256];
					 CHAR mLine[2048];
					 unsigned int i;
					 char *val;

					 strcpy(mFnHead, pContext->mFnHead);
					 mFnHead[strlen(mFnHead)-2] = 'R';
					 if ((fp= fopen(mFnHead, "ab")))
					 {
					   p = (unsigned int *)&pmlt->mMLTMessages;
					   sz = ntohl(*p);
#ifdef UPDATE_20171102B // MILTERで複数ヘッダの追加変更が失敗しないようにする対策
					   l-=sz+4;
#endif
					   val = &pmlt->mMLTMessages[5];
					   fprintf(fp, "%s\r\n", val);
					   fclose(fp);
					 }
					 while(!(fp= fopen(mFnHead, "rb"))) 
					 {
	                   if (bServiceTerminating)
	                     break;
                       _sleep(100);
					 }
					 fclose(fp);
#ifdef UPDATE_20171102B // MILTERで複数ヘッダの追加変更が失敗しないようにする対策
				     if (l > 0)
					 {
			           for (i = 0; i < l; i++)
					   {
					     pmlt->mMLTMessages[i] = pmlt->mMLTMessages[sz+4+i];
					   } 
					   pmlt->mMLTMessages[i] = pmlt->mMLTMessages[sz+4+i];
                       goto anyflag;
					 }
#endif
				   }
				   n = 0;
#ifdef UPDATE_20171102B // MILTERで複数ヘッダの追加変更が失敗しないようにする対策
				   goto nexttoken;
#endif
			       break;
			   case F_DATAMOD:  //'b' // 本文置換: 本文を変更する応答。変更後の本文が大きい場合は複数のチャンクに分けて応答する。
#ifdef UPDATE_20150216 // E-POST用 MAIL FILTER 処理の詳細ログへの記録を追加した。
				   strcpy(mCmd, "F_DATAMOD");
#endif
				   {
				     FILE *fp;
					 unsigned int *p, sz;
					 CHAR mFnData[256];
					 CHAR mLine[2048];
					 unsigned int i;
					 char *val;

					 strcpy(mFnData, pContext->mFnData);
					 mFnData[strlen(mFnData)-2] = 'M';
					 if ((fp= fopen(mFnData, "ab")))
					 {
#ifdef UPDATE_20171102B // MILTERで複数ヘッダの追加変更が失敗しないようにする対策
					   p = (unsigned int *)&pmlt->mMLTMessages;
					   sz = ntohl(*p);
					   l-=sz+4;
					   val = &pmlt->mMLTMessages[5];
                       fprintf(fp, "%s", val); // 置換本文保存
					   if (*(val+strlen(val)-1) != '\r' &&
						   *(val+strlen(val)-1) != '\n')
					   {
                         fprintf(fp, "\r\n");
					   }
#else
					   p = (unsigned int *)&pmlt->mMLTMessages;
					   sz = ntohl(*p);
					   val = &pmlt->mMLTMessages[5];
					   if (*(val+strlen(val)-1) != '\r' &&
						   *(val+strlen(val)-1) != '\n')
					   {
                         strcat(val, "\r\n");
					   }
                       fprintf(fp, "%s", val); // 置換本文保存
#endif
					   fclose(fp);
					 }
					 while(!(fp= fopen(mFnData, "rb"))) 
					 {
	                   if (bServiceTerminating)
	                     break;
                       _sleep(100);
					 }
					 fclose(fp);
#ifdef UPDATE_20171102B // MILTERで複数ヘッダの追加変更が失敗しないようにする対策
				     if (l > 0)
					 {
			           for (i = 0; i < l; i++)
					   {
					     pmlt->mMLTMessages[i] = pmlt->mMLTMessages[sz+4+i];
					   } 
					   pmlt->mMLTMessages[i] = pmlt->mMLTMessages[sz+4+i];
                       goto anyflag;
					 }
#endif
				   }
				   n = 0;
#ifdef UPDATE_20171102B // MILTERで複数ヘッダの追加変更が失敗しないようにする対策
				   goto nexttoken;
#endif
                   break;
			   case F_FROMREP:  //'e' // 差出人変更: 差出人を変更する応答。
#ifdef UPDATE_20150216 // E-POST用 MAIL FILTER 処理の詳細ログへの記録を追加した。
				   strcpy(mCmd, "F_FROMREP");
#endif
				   strcpy(pContext->mUIDFROM, &pmlt->mMLTMessages[5]);
				   //n = 0;
				   if (pContext->mFnHead[0])
				   {
				     FILE *fp;
					 unsigned int *p, sz;
					 CHAR mFnHead[256];
					 CHAR mLine[2048];
					 unsigned int i;
					 char *val;

					 strcpy(mFnHead, pContext->mFnHead);
					 mFnHead[strlen(mFnHead)-2] = 'F';
					 if ((fp= fopen(mFnHead, "wb")))
					 {
					   p = (unsigned int *)&pmlt->mMLTMessages;
					   sz = ntohl(*p);
#ifdef UPDATE_20171102B // MILTERで複数ヘッダの追加変更が失敗しないようにする対策
					   l-=sz+4;
#endif
					   val = &pmlt->mMLTMessages[5];
					   fprintf(fp, "Return-path: %s\r\n", val);
					   fclose(fp);
					 }
					 while(!(fp= fopen(mFnHead, "rb"))) 
					 {
	                   if (bServiceTerminating)
	                     break;
                       _sleep(100);
					 }
					 fclose(fp);
#ifdef UPDATE_20171102B // MILTERで複数ヘッダの追加変更が失敗しないようにする対策
				     if (l > 0)
					 {
			           for (i = 0; i < l; i++)
					   {
					     pmlt->mMLTMessages[i] = pmlt->mMLTMessages[sz+4+i];
					   } 
					   pmlt->mMLTMessages[i] = pmlt->mMLTMessages[sz+4+i];
                       goto anyflag;
					 }
#endif
				   }
				   n = 0;
#ifdef UPDATE_20171102B // MILTERで複数ヘッダの追加変更が失敗しないようにする対策
				   goto nexttoken;
#endif
                   break;
			   case F_ISOLATE:  //'q' // 隔離: このメールを隔離するという応答。
#ifdef UPDATE_20150216 // E-POST用 MAIL FILTER 処理の詳細ログへの記録を追加した。
				   strcpy(mCmd, "F_ISOLATE");
#endif
#ifdef UPDATE_20171102B // MILTERで複数ヘッダの追加変更が失敗しないようにする対策
				   goto nexttoken;
#endif
			       break;
			   case F_REJECT:   //'r' // 拒否: このメールの受信を拒否するという応答。SMTPでは5XX系のレスポンスになる。
#ifdef UPDATE_20150216 // E-POST用 MAIL FILTER 処理の詳細ログへの記録を追加した。
				   strcpy(mCmd, "F_REJECT");
#endif
                                    strcpy(pContext->mMessages, SMTP_BAD_MAILFILTER); // 応答メッセージ書換え
				                    bSts = FALSE;
								    break;
			   case F_TEMP:     //'t' // 一時拒否: このメールの受信を一時的に拒否するという応答。SMTPでは4XX系のレスポンスになる。
#ifdef UPDATE_20150216 // E-POST用 MAIL FILTER 処理の詳細ログへの記録を追加した。
				   strcpy(mCmd, "F_TEMP");
#endif
									strcpy(pContext->mMessages, SMTP_MAILFILTER_STOP); // 応答メッセージ書換え
				                    bSts = FALSE;
								    break;
			   case F_RESPONSE: //'y' // SMTPレスポンス設定: SMTPレスポンスのコードとメッセージを設定する。
#ifdef UPDATE_20150216 // E-POST用 MAIL FILTER 処理の詳細ログへの記録を追加した。
				   strcpy(mCmd, "F_RESPONSE");
#endif
									strcpy(pContext->mMessages, &pmlt->mMLTMessages[5]); // 応答メッセージ書換え
									if (!strpbrk(pContext->mMessages, "\r\n"))
									{
									  strcat(pContext->mMessages, "\r\n");
									}
									if (pContext->mMessages[0] == '4' || pContext->mMessages[0] == '5')
									{
				                      bSts = FALSE;
									}
								    break;
			   case F_EXECUTE:  //'p' // 処理中: milterの処理に時間がかかっていることをMTAに知らせる応答。MTA側のタイムアウト時間を伸ばせる。
#ifdef UPDATE_20150216 // E-POST用 MAIL FILTER 処理の詳細ログへの記録を追加した。
				   strcpy(mCmd, "F_EXECUTE");
#endif
				                    n = 0;
								    break;
			   case F_NEXT:     //'c' // 継続: 次のコマンドへいってくれという応答。基本はこれを返す。
#ifdef UPDATE_20150216 // E-POST用 MAIL FILTER 処理の詳細ログへの記録を追加した。
				   strcpy(mCmd, "F_NEXT");
#endif
								    break;
#ifdef UPDATE_20150216 // E-POST用 MAIL FILTER 処理の詳細ログへの記録を追加した。
			   case F_SKIP:     //'s' // スキップ: このメールトランザクションの処理を途中でやめるという応答。
				   strcpy(mCmd, "F_SKIP");
                                    close_milter_session(pmlt);
							        break;
			   case F_DISCON:   //'d' // 破棄: 処理中のメッセージを破棄する。SMTPでのメールトランザクションの処理はここで終了するので、milterプロトコルでのメールトランザクションも終了する。
				   strcpy(mCmd, "F_DISCON");
                                    close_milter_session(pmlt);
							        break;
			   case F_OK:       //'a' // 受理: このメールは受理するという応答。milterプロトコルでのメールトランザクションの処理はここで終了し、これ以降MTAからコマンドは送られてこない。
                                    // 自身を解放
				   strcpy(mCmd, "F_OK");
                                    close_milter_session(pmlt);
							        break;
#else
			   case F_SKIP:     //'s' // スキップ: このメールトランザクションの処理を途中でやめるという応答。
			   case F_DISCON:   //'d' // 破棄: 処理中のメッセージを破棄する。SMTPでのメールトランザクションの処理はここで終了するので、milterプロトコルでのメールトランザクションも終了する。
			   case F_OK:       //'a' // 受理: このメールは受理するという応答。milterプロトコルでのメールトランザクションの処理はここで終了し、これ以降MTAからコマンドは送られてこない。
                                    // 自身を解放
                                    close_milter_session(pmlt);
							        break;
#endif
               case M_NEGO:
#ifdef UPDATE_20150216 // E-POST用 MAIL FILTER 処理の詳細ログへの記録を追加した。
				   strcpy(mCmd, "M_NEGO");
#endif
				                   break;
			   default:
#ifdef UPDATE_20150216 // E-POST用 MAIL FILTER 処理の詳細ログへの記録を追加した。
				   strcpy(mCmd, "F_UNKOWN");
#endif
								    break;
			 }
			 /*
	         if (pfd->id == '\x0')
			 {
		       bSts = FALSE;
		       sprintf(pContext->mMessages, pmlt->mMLTMessages); // 応答メッセージ書換え
		       //break;
			 } else if (pmlt->mMLTMessages[0] != '2' && pmlt->mMLTMessages[0] != '3')
			 {
	           bSts = FALSE;
		       sprintf(pContext->mMessages, pmlt->mMLTMessages); // 応答メッセージ書換え
		       //break;
			 }
			 */
		   }
		 }
	   }
	 }
   }
#ifdef UPDATE_20150216 // E-POST用 MAIL FILTER 処理の詳細ログへの記録を追加した。
#ifdef ACCEPT_LOG_LEVEL3
   sprintf(mLog, "[MLT_ACCESS] [%d] [%s], end status = %s", i, mCmd, (bSts ? "TRUE":  "FALSE"));
   LEVEL_3_ACCEPTLOG(lpClientContext, mLog);
#endif
#endif
   return bSts;
}

BOOL MLT_CONNECT(PCLIENT_CONTEXT lpClientContext)
{
   int i;
   BOOL bSts = TRUE;
   struct _MILTER *pmlt;
   PSmtpContext pContext = &lpClientContext->Context;
   struct _FILTERDATA *pfd;
   unsigned long step;
   char family;
   unsigned short port;
   unsigned int sz;
   char *pb;


   for (i=0; i < lpClientContext->nMilter; i++)
   {
     pmlt =  (struct _MILTER *)((lpClientContext->pMLT)+i);
 	 step = ntohl(*(unsigned long *)&pmlt->mFnd.data[sizeof(unsigned long)*2]);

	 if (pmlt->s != INVALID_SOCKET &&
		 !(step & STEP_MTA_NOCONN)) // Skip
	 {
       char *sockinfo = NULL;
//
       pfd = (struct _FILTERDATA *)pmlt->mMLTToken;
   	   pmlt->macrosz = 0;
       if (bMMACRO && mMMCONNECT[0]) // マクロを追加する場合
	   {
         pfd->id = M_MACRO;
         pmlt->mMLTToken[5] = M_CONNECT;
	     pb = &pmlt->mMLTToken[6];
#ifdef UPDATE_20150210 // E-POST用 MAIL FILTER インターフェースへのマクロ指定を追加。
		 pb = macro_argument(lpClientContext, MA_CONNECT, &pmlt->macrosz, mMMCONNECT, pb);
		 pmlt->macrosz += 2;
#else
	     strcpy(pb, "{daemon_name}"); pb+=13; *pb++ = '\0';
	     strcpy(pb, SMTP_SERVICE); pb+=strlen(SMTP_SERVICE); *pb++ = '\0';
	     strcpy(pb, "{if_addr}"); pb+=9; *pb++ = '\0';
	     strcpy(pb, pContext->PeerAddr); pb+=strlen(pContext->PeerAddr); *pb++ = '\0';
	     strcpy(pb, "{if_name}"); pb+=9; *pb++ = '\0';
	     strcpy(pb, "localhost"); pb+=9; *pb++ = '\0';
	     strcpy(pb, "j"); pb+=1; *pb++ = '\0';
	     strcpy(pb, mLocalDomain); pb+=strlen(mLocalDomain); *pb++ = '\0';
	     pmlt->macrosz = 2+14+(strlen(SMTP_SERVICE)+1)+10+(strlen(pContext->PeerAddr)+1)+10+10+2+(strlen(mLocalDomain)+1);
#endif
	     pfd->sz = htonl(pmlt->macrosz);
	     pmlt->macrosz += 4;
         pfd = (struct _FILTERDATA *)&pmlt->mMLTToken[pmlt->macrosz];
	     pb+=5;
	   } else {
	     pb = &pmlt->mMLTToken[5];
	   }
       pfd->id = M_CONNECT;
	   memcpy(pb, pContext->PeerName, strlen(pContext->PeerName));
	   pb += strlen(pContext->PeerName);
	   *pb++ = '\0';
	  
	   if (strchr(pContext->PeerAddr, ':'))
	   {
	     family = (int)'6'; // IPv6
		 port = pContext->nConnectPort; //atoi(pContext->mPort);
		 sockinfo = ""; //(char *) pContext->PeerAddr;
	   } else {
	     family = (int)'4'; // IPv4
		 port = lpClientContext->sin1.sin_port;//pContext->nConnectPort; //atoi(pmlt->mPort);
		 sockinfo = (char *) inet_ntoa(lpClientContext->sin1.sin_addr);
	   }
	   //family = 'U'; // UNKNOWN;

	   memcpy(pb, &family, sizeof(family));
	   pb += sizeof(family);
	   
	   //port = atoi(pmlt->mPort);
	   memcpy(pb, &port, sizeof(port));
	   pb += sizeof(port);
       memcpy(pb, sockinfo, strlen(sockinfo) + 1);
	   
       pfd->sz = htonl(strlen(pContext->PeerName)+1+sizeof(family)+sizeof(port)+strlen(sockinfo)+1+1);
	   //pfd->sz = htonl(strlen(pContext->PeerName)+1+1);
       if (!(bSts = MLT_ACCESS(lpClientContext, i, TRUE)))
	   {
		 bSts = TRUE;
		 close_milter_session(pmlt);
		 /*
	     if (pmlt->s)
		 {
	       closesocket(pmlt->s);
		   pmlt->s = INVALID_SOCKET;
		 }
		 if (pmlt->h)
		 {
	       CloseHandle(pmlt->h);
		   pmlt->h = NULL;
		 }
		*/
		 break;
	   }
	 }
   }
   return bSts;
}

BOOL MLT_HELO(PCLIENT_CONTEXT lpClientContext)
{
   int i;
   BOOL bSts = TRUE;
   struct _MILTER *pmlt;
   PSmtpContext pContext = &lpClientContext->Context;
   struct _FILTERDATA *pfd;
   unsigned long step;
   char *pb;

   for (i=0; i < lpClientContext->nMilter; i++)
   {
     pmlt =  (struct _MILTER *)((lpClientContext->pMLT)+i);
 	 step = ntohl(*(unsigned long *)&pmlt->mFnd.data[sizeof(unsigned long)*2]);

	 if (pmlt->s != INVALID_SOCKET && 
		 !(step & STEP_MTA_NOHELO) &&
#ifdef UPDATE_20171102 // MILTERで空白データのコマンドをスキップしないようにした
		 pContext->PeerHelo)
#else
		 *pContext->PeerHelo) // Skip
#endif
	 {
       pfd = (struct _FILTERDATA *)pmlt->mMLTToken;
   	   pmlt->macrosz = 0;
       if (bMMACRO && mMMHELO[0]) // マクロを追加する場合
	   {
         pfd->id = M_MACRO;
         pmlt->mMLTToken[5] = M_HELO;
	     pb = &pmlt->mMLTToken[6];
#ifdef UPDATE_20150210 // E-POST用 MAIL FILTER インターフェースへのマクロ指定を追加。
		 pb = macro_argument(lpClientContext, MA_HELO, &pmlt->macrosz, mMMHELO, pb);
		 pmlt->macrosz += 2;
#else
	     strcpy(pb, "{cert_issuer}"); pb+=13; *pb++ = '\0';
	     strcpy(pb, "cert_issuer"); pb+=11; *pb++ = '\0';
         strcpy(pb, "{cert_subject}"); pb+=14; *pb++ = '\0';
         strcpy(pb, "cert_subject"); pb+=12; *pb++ = '\0';
         strcpy(pb, "{cipher_bits}"); pb+=13; *pb++ = '\0';
         strcpy(pb, "0"); pb+=1; *pb++ = '\0';
         strcpy(pb, "{cipher}"); pb+=8; *pb++ = '\0';
         strcpy(pb, "0"); pb+=1; *pb++ = '\0';
         strcpy(pb, "{tls_version}"); pb+=13; *pb++ = '\0';
         strcpy(pb, "0"); pb+=1; *pb++ = '\0';
	     pmlt->macrosz = 2+14+12+15+13+14+2+9+2+14+2;
#endif
	     pfd->sz = htonl(pmlt->macrosz);
	     pmlt->macrosz += 4;
         pfd = (struct _FILTERDATA *)&pmlt->mMLTToken[pmlt->macrosz];
	     pb+=5;
	   } else {
	     pb = &pmlt->mMLTToken[5];
	   }
       pfd->id = M_HELO;
	   //strcpy(pb, pContext->PeerHelo);
	   sprintf(pb, "%s", pContext->PeerHelo);
	   pb += strlen(pContext->PeerHelo);
	   //*pb++ = '\0';
#ifdef UPDATE_20171105  // MILTERのHELO処理で余分なデータが出力されてしまう不具合
       pfd->sz = htonl(strlen(pContext->PeerHelo)+2);
#else
       pfd->sz = htonl(strlen(pContext->PeerHelo)+2+1);//+1);
#endif
       if (!(bSts = MLT_ACCESS(lpClientContext, i, TRUE)))
	   {
		 bSts = TRUE;
		 close_milter_session(pmlt);
		 break;
	   }
	 }
   }
   return bSts;
}

BOOL MLT_FROM(PCLIENT_CONTEXT lpClientContext)
{
   int i;
   BOOL bSts = TRUE;
   struct _MILTER *pmlt;
   PSmtpContext pContext = &lpClientContext->Context;
   struct _FILTERDATA *pfd;
   unsigned long step;
   char *pb;

   for (i=0; i < lpClientContext->nMilter; i++)
   {
	 pmlt =  (struct _MILTER *)((lpClientContext->pMLT)+i);
 	 step = ntohl(*(unsigned long *)&pmlt->mFnd.data[sizeof(unsigned long)*2]);

	 if (pmlt->s != INVALID_SOCKET &&
		 !(step & STEP_MTA_NOFROM) &&
#ifdef UPDATE_20171102 // MILTERで空白データのコマンドをスキップしないようにした
		 pContext->mUIDFROM)
#else
		 *pContext->mUIDFROM) // Skip
#endif
	 {
       pfd = (struct _FILTERDATA *)pmlt->mMLTToken;
   	   pmlt->macrosz = 0;
       if (bMMACRO && mMMENVFROM[0]) // マクロを追加する場合
	   {
         pfd->id = M_MACRO;
         pmlt->mMLTToken[5] = M_FROM;
	     pb = &pmlt->mMLTToken[6];
#ifdef UPDATE_20150210 // E-POST用 MAIL FILTER インターフェースへのマクロ指定を追加。
		 pb = macro_argument(lpClientContext, MA_MAIL, &pmlt->macrosz, mMMENVFROM, pb);
		 pmlt->macrosz += 2;
#else
	     strcpy(pb, "i"); pb+=1; *pb++ = '\0';
	     strcpy(pb, "i"); pb+=1; *pb++ = '\0';
	     strcpy(pb, "{mail_addr}"); pb+=11; *pb++ = '\0';
	     strcpy(pb, "mail_addr"); pb+=9; *pb++ = '\0';
	     strcpy(pb, "{mail_host}"); pb+=11; *pb++ = '\0';
	     strcpy(pb, "mail_host"); pb+=9; *pb++ = '\0';
	     strcpy(pb, "{mail_mailer}"); pb+=13; *pb++ = '\0';
	     strcpy(pb, "mail_mailer"); pb+=11; *pb++ = '\0';
	     pmlt->macrosz = 2+2+2+12+10+12+10+14+12;
#endif
	     pfd->sz = htonl(pmlt->macrosz);
	     pmlt->macrosz += 4;
         pfd = (struct _FILTERDATA *)&pmlt->mMLTToken[pmlt->macrosz];
	     pb+=5;
	   } else {
	     pb = &pmlt->mMLTToken[5];
	   }
       pfd->id = M_FROM;
	   sprintf(pb, "<%s>", pContext->mUIDFROM);
	   pb += strlen(pContext->mUIDFROM)+2;
	   *pb++ = '\0';
       pfd->sz = htonl(strlen(pContext->mUIDFROM)+2+1+1);
       if (!(bSts = MLT_ACCESS(lpClientContext, i, TRUE)))
	   {
		 bSts = TRUE;
		 close_milter_session(pmlt);
		 break;
	   }
	 }
   }
   return bSts;
}

BOOL MLT_TO(PCLIENT_CONTEXT lpClientContext)
{
   int i;
   BOOL bSts = TRUE;
   struct _MILTER *pmlt;
   PSmtpContext pContext = &lpClientContext->Context;
   struct _FILTERDATA *pfd;
   unsigned long step;
   char *pb;

   for (i=0; i < lpClientContext->nMilter; i++)
   {
	 pmlt =  (struct _MILTER *)((lpClientContext->pMLT)+i);
 	 step = ntohl(*(unsigned long *)&pmlt->mFnd.data[sizeof(unsigned long)*2]);

	 if (pmlt->s != INVALID_SOCKET &&
		 !(step & STEP_MTA_NOTO) &&
		 pContext->mUIDRCPT) // Skip
	 {
       pfd = (struct _FILTERDATA *)pmlt->mMLTToken;
   	   pmlt->macrosz = 0;
       if (bMMACRO && mMMENVRCPT[0]) // マクロを追加する場合
	   {
         pfd->id = M_MACRO;
         pmlt->mMLTToken[5] = M_RCPT;
	     pb = &pmlt->mMLTToken[6];
#ifdef UPDATE_20150210 // E-POST用 MAIL FILTER インターフェースへのマクロ指定を追加。
		 pb = macro_argument(lpClientContext, MA_RCPT, &pmlt->macrosz, mMMENVRCPT, pb);
		 pmlt->macrosz += 2;
#else
	     strcpy(pb, "{rcpt_addr}"); pb+=11; *pb++ = '\0';
	     strcpy(pb, pContext->mUIDRCPT); pb+=strlen(pContext->mUIDRCPT); *pb++ = '\0';
	     strcpy(pb, "{rcpt_host}"); pb+=11; *pb++ = '\0';
	     strcpy(pb, "rcpt_host"); pb+=9; *pb++ = '\0';
	     strcpy(pb, "{rcpt_mailer}"); pb+=13; *pb++ = '\0';
	     strcpy(pb, "rcpt_mailer"); pb+=11; *pb++ = '\0';
	     pmlt->macrosz = 2+12+(strlen(pContext->mUIDRCPT)+1)+12+10+14+12;
#endif
	     pfd->sz = htonl(pmlt->macrosz);
	     pmlt->macrosz += 4;
         pfd = (struct _FILTERDATA *)&pmlt->mMLTToken[pmlt->macrosz];
	     pb+=5;
	   } else {
	     pb = &pmlt->mMLTToken[5];
	   }
       pfd->id = M_RCPT;
	   sprintf(pb, "<%s>", pContext->mUIDRCPT);
	   pb += strlen(pContext->mUIDRCPT)+2;
	   *pb++ = '\0';
       pfd->sz = htonl(strlen(pContext->mUIDRCPT)+2+1+1);
       if (!(bSts = MLT_ACCESS(lpClientContext, i, TRUE)))
	   {
		 bSts = TRUE;
		 close_milter_session(pmlt);
		 break;
	   }
	 }
   }
   return bSts;
}

BOOL MLT_DATA(PCLIENT_CONTEXT lpClientContext)
{
   int i;
   BOOL bSts = TRUE;
   BOOL bHead = TRUE;
   FILE *fp;
   struct _MILTER *pmlt;
   PSmtpContext pContext = &lpClientContext->Context;
   struct _FILTERDATA *pfd;
   unsigned long step;
   unsigned int nl, j, k;
   char *pb, *p, *pTken;
   CHAR   mMLTToken[2048];   // Command work

   for (i=0; i < lpClientContext->nMilter; i++)
   {
	 pmlt =  (struct _MILTER *)((lpClientContext->pMLT)+i);
	 step = ntohl(*(unsigned long *)&pmlt->mFnd.data[sizeof(unsigned long)*2]);

	 if (pmlt->s != INVALID_SOCKET &&
		 !(step & STEP_MTA_NODATA)) // Skip
	 {
       pfd = (struct _FILTERDATA *)pmlt->mMLTToken;
   	   pmlt->macrosz = 0;
       pfd->id = M_DATA;
	   pmlt->mMLTToken[5] = '\x0';
       pfd->sz = htonl(1);
       if (!(bSts =MLT_ACCESS(lpClientContext, i, TRUE)))
	   {
		 bSts = TRUE;
		 close_milter_session(pmlt);
	     break;
	   }
	 }
	 {
	   mMLTToken[0] = '\0';
       bHead = TRUE;
       pfd = (struct _FILTERDATA *)pmlt->mMLTToken;
   	   pmlt->macrosz = 0;
       pfd->id = M_HEADER;
       if ((fp = fopen(pContext->mFnData, "rb")))
	   {
         while(fgets(&pmlt->mMLTToken[5], 2046, fp))
		 {
next_header:
	       //if (!bHead) // BODY送信
		   {
		     if (!strcmp(&pmlt->mMLTToken[5], ".\r\n") ||
		         !strcmp(&pmlt->mMLTToken[5], ".\r") ||
		         !strcmp(&pmlt->mMLTToken[5], ".\n") ) 
			 {
               //if (pmlt->mFnd.step & STEP_MTA_NOEOM) // Skip
	             //break;
   	           pmlt->macrosz = 0;
               if (bMMACRO && mMMEOM[0]) // マクロを追加する場合
			   {
                 pfd->id = M_MACRO;
                 pmlt->mMLTToken[5] = M_EOM;
	             pb = &pmlt->mMLTToken[6];
#ifdef UPDATE_20150210 // E-POST用 MAIL FILTER インターフェースへのマクロ指定を追加。
		         pb = macro_argument(lpClientContext, MA_EOM, &pmlt->macrosz, mMMEOM, pb);
		         pmlt->macrosz += 2;
#else
				 strcpy(pb, "i"); pb+=1; *pb++ = '\0';
	             strcpy(pb, "message-id"); pb+=10; *pb++ = '\0';
	             pmlt->macrosz = 2+2+11;
#endif
	             pfd->sz = htonl(pmlt->macrosz);
	             pmlt->macrosz += 4;
                 pfd = (struct _FILTERDATA *)&pmlt->mMLTToken[pmlt->macrosz];
	             pb+=5;
			     *pb++ = '\0';
			   } else {
			     pmlt->mMLTToken[5] = '\x0';
			   }
               pfd->id = M_EOM;
               pfd->sz = htonl(1);
               bSts = MLT_ACCESS(lpClientContext, i, TRUE);
		       break;
			 }
		   }
		   if (bHead) // ヘッダ送信
		   { 
		     if (pmlt->mMLTToken[5] == '\x0' ||
		         pmlt->mMLTToken[5] == '\r' ||
		         pmlt->mMLTToken[5] == '\n')
			 {
		       bHead = FALSE;
#ifndef UPDATE_20171102A // CLAMAVMで本文の送信がスキップされないようにする対策
               if (step & STEP_MTA_NOEOH) // Skip
	             break;
#endif
               pfd->id = M_EOH;
	           pmlt->mMLTToken[5] = '\x0';
               pfd->sz = htonl(1);
               if (!(bSts = MLT_ACCESS(lpClientContext, i, TRUE)))
			   {
		         bSts = TRUE;
		         close_milter_session(pmlt);
		         break;
			   }
		       pfd->id = M_BODY;
			   fgets(&pmlt->mMLTToken[5], 2046, fp); // 1行スキップ
			 }
		   }
	       // １行データ送信
           if (!bHead && step & STEP_MTA_NOBODY) // Skip
	         continue;
           if (bHead && step & STEP_MTA_NOHEAD) // Skip
	         continue;
		   pb = &pmlt->mMLTToken[5];
		   if (bHead)
		   {
			 if (strchr(&pmlt->mMLTToken[5], ':'))
			 {
			   nl = strlen(&pmlt->mMLTToken[5]);
			   while(1) {
				 if (!fgets(mMLTToken, sizeof(mMLTToken)-1, fp))
				 {
				   break;
				 }
				 if ( mMLTToken[0] == ' ' ||
		              mMLTToken[0] == '\t')
				 {
			       nl += strlen(mMLTToken);
				   strcat(&pmlt->mMLTToken[5], mMLTToken);
				 } else {
				   break;
				 }
			   }
               //////////////////////////////////////////////
			   //strcpy(mMLTToken, &pmlt->mMLTToken[5], strlen(&pmlt->mMLTToken[5]));
			   p = &pmlt->mMLTToken[5];
			   // 末尾の改行除去
			   if (nl > 2)
			   {
				 if (*(p+nl-2) == '\r' || *(p+nl-2) == '\n') 
				 {
                   *(p+nl-2) = '\0';
				 }
			   } else {
				 if (*(p) == '\r' || *(p) == '\n') 
				 {
                   *(p) = '\0';
				 }
			   }
			   // WS除去
			   if ((pTken = strchr(p, ':'))) // ヘッダ名の区切り
			   {
				 pTken++;
                 p = pTken;
			     while(*p == ' ' || *p == '\t')
				 {
				   *p++;
				 }
			     if (p > pTken) // 先頭にWSがあった場合移動
				 {
			       nl = strlen(p);
				   for (j = 0; j <= nl; j++)
				   {
                     *(pTken+j) = *(p+j); // 移動
				   }
				 }
			   }
               //////////////////////////////////////////////
               pfd->sz = htonl(strlen(&pmlt->mMLTToken[5])+1+1);
		       pb += strlen(&pmlt->mMLTToken[5]);
		       *pb++ = '\0';
			   strtok(&pmlt->mMLTToken[5], ":");
               if (!(bSts = MLT_ACCESS(lpClientContext, i, TRUE)))
			   {
		         bSts = TRUE;
		         close_milter_session(pmlt);
	             break;
			   }
			   strcpy(&pmlt->mMLTToken[5], mMLTToken);
			   goto next_header;
			 }
		   } else {
             if (!strcmp(&pmlt->mMLTToken[5], ".\r\n") ||
		         !strcmp(&pmlt->mMLTToken[5], ".\r") ||
		         !strcmp(&pmlt->mMLTToken[5], ".\n") )
			 {
			   goto next_header;
			 } else {

			   nl = strlen(&pmlt->mMLTToken[5]);
			   while(fgets(mMLTToken, sizeof(mMLTToken)-1, fp)) 
			   {
				 nl += strlen(mMLTToken);
				 k = sizeof(pmlt->mMLTToken)-5-sizeof(mMLTToken);
				 if (nl > k ||
		             !strcmp(mMLTToken, ".\r\n") ||
		             !strcmp(mMLTToken, ".\r") ||
		             !strcmp(mMLTToken, ".\n") ) 
				 {
                   pfd->sz = htonl(strlen(&pmlt->mMLTToken[5])+1+1);
		           pb += strlen(&pmlt->mMLTToken[5]);
		           *pb++ = '\0';
                   if (!(bSts = MLT_ACCESS(lpClientContext, i, TRUE)))
				   {
		             bSts = TRUE;
		             close_milter_session(pmlt);
	                 break;
				   }
				   nl = 0;
				   pmlt->mMLTToken[5] = '\0';
				   pb = &pmlt->mMLTToken[5]; // 位置戻し
				   if (!strcmp(mMLTToken, ".\r\n") ||
		               !strcmp(mMLTToken, ".\r") ||
		               !strcmp(mMLTToken, ".\n") )
				   {
					 strcpy(&pmlt->mMLTToken[5], mMLTToken);
			         goto next_header;
				   }
				 } else {
				   strcat(&pmlt->mMLTToken[5], mMLTToken);
				 }
			   }
               if (bSts)
			   {
                 pfd->sz = htonl(strlen(&pmlt->mMLTToken[5])+1+1);
		         pb += strlen(&pmlt->mMLTToken[5]);
		         *pb++ = '\0';
			   }
			 }
		   }
           if (!(bSts = MLT_ACCESS(lpClientContext, i, TRUE)))
		   {
		     bSts = TRUE;
		     close_milter_session(pmlt);
	         break;
		   }
		 }
	     fclose(fp);
	   }
	   // 送信先削除
	   {
		 BOOL bHit;
	     FILE *fp1, *fp2, *fp3;
		 CHAR mFnHead[256];
		 CHAR mFnRHead[256];
		 CHAR mFnFHead[256];
		 CHAR mLine[256];
		 CHAR mLine1[256];
		 strcpy(mFnHead, pContext->mFnHead);
		 mFnHead[strlen(mFnHead)-2] = 'H';
		 strcpy(mFnRHead, pContext->mFnHead);
		 mFnRHead[strlen(mFnRHead)-2] = 'R';
		 strcpy(mFnFHead, pContext->mFnHead);
		 mFnFHead[strlen(mFnFHead)-2] = 'F';
         while(!MoveFileEx(pContext->mFnHead, mFnHead, MOVEFILE_COPY_ALLOWED | MOVEFILE_WRITE_THROUGH)) 
		 {
	       if (bServiceTerminating ||
               GetLastError() == ERROR_FILE_NOT_FOUND)
	         break;
           _sleep(100);
		 }
		 if ((fp1= fopen(mFnHead, "rt")))
		 {
		   if ((fp2= fopen(pContext->mFnHead, "wt")))
		   {
			 fgets(mLine, sizeof(mLine)-1, fp1);
			 fputs(mLine, fp2);
			 fgets(mLine, sizeof(mLine)-1, fp1);
             if ((fp3= fopen(mFnFHead, "rt")))
			 {
			   fgets(mLine, sizeof(mLine)-1, fp3);
			   fclose(fp3);
			 }
			 fputs(mLine, fp2);
			 while(fgets(mLine, sizeof(mLine)-1, fp1))
			 {
			   bHit = FALSE;
               if ((fp3= fopen(mFnRHead, "rt")))
			   {
			     while(fgets(mLine1, sizeof(mLine1)-1, fp3))
				 {
				   strtok(mLine1, "\r\n");
                   if (!strnicmp(&mLine[11], mLine1, strlen(mLine1)))
				   {
					 bHit = TRUE;
					 break;
				   }
				 }
				 if (!bHit)
				 {
				   fputs(mLine, fp2);
				 }
				 fclose(fp3);
			   } else {
				 fputs(mLine, fp2);
			   }
			 }
			 fclose(fp2);
		   }
		   fclose(fp1);
		 }
		 ////////
	     while(!DeleteFile(mFnHead)) 
		 {
	       if (bServiceTerminating ||
               GetLastError() == ERROR_FILE_NOT_FOUND)
		     break;
           _sleep(100);
		 }
	     while(!DeleteFile(mFnRHead)) 
		 {
	       if (bServiceTerminating ||
               GetLastError() == ERROR_FILE_NOT_FOUND)
		     break;
           _sleep(100);
		 }
	     while(!DeleteFile(mFnFHead)) 
		 {
	       if (bServiceTerminating ||
               GetLastError() == ERROR_FILE_NOT_FOUND)
		     break;
           _sleep(100);
		 }
		 while(!(fp1= fopen(pContext->mFnHead, "rt"))) 
		 {
	       if (bServiceTerminating)
	         break;
           _sleep(100);
		 }
		 fclose(fp1);
	   }
	   // 本文データ置き換え
	   {
		 unsigned int idx, j, sz, n, n2, n3;
		 BOOL bHead1 = TRUE; 
		 BOOL bNewHead;
	     FILE *fp1, *fp2, *fp3, *fp4;
		 CHAR c, *p, *q, *r, *pHead;
		 CHAR mFnData[256];
		 CHAR mFnHData[256];
		 CHAR mFnMData[256];
		 CHAR mFnEData[256];
		 CHAR mLine[2048];
		 CHAR mLine1[2048];
		 CHAR mLine2[2048];
		 strcpy(mFnData, pContext->mFnData);
		 mFnData[strlen(mFnData)-2] = 'F';
		 strcpy(mFnHData, pContext->mFnData);
		 mFnHData[strlen(mFnHData)-2] = 'H';
		 strcpy(mFnMData, pContext->mFnData);
		 mFnMData[strlen(mFnMData)-2] = 'M';
		 strcpy(mFnEData, pContext->mFnData);
		 mFnEData[strlen(mFnEData)-2] = 'E';
//void translateUue2Base64(char *line, int inlen, char *newline);
         // ヘッダ操作
		 sz = 0; // ヘッダのサイズ計算
	     if ((fp1= fopen(pContext->mFnData, "rb")))
		 {
           while(fgets(mLine, sizeof(mLine)-1, fp1))
		   {
			 sz+=strlen(mLine);
		     if (mLine[0] == '\r' || mLine[0] == '\n')
			 {
			   break;
			 }
		   }
		   fclose(fp1);
		 }
		 if ((fp1= fopen(mFnHData, "rb")))
		 {
           while(fgets(mLine, sizeof(mLine)-1, fp1))
		   {
			 sz+=strlen(mLine);
		   }
		   fclose(fp1);
		 }
		 if ((fp1= fopen(mFnEData, "rb")))
		 {
           while(fgets(mLine, sizeof(mLine)-1, fp1))
		   {
			 if (mLine[0] != ' ' && mLine[0] != '\t')
			 {
			   if ((p=strchr(&mLine[2], ':')))
			   {
			     sz+=strlen(p+1);
			   }
			 } else {
			   sz+=strlen(mLine);
			 }
		   }
		   fclose(fp1);
		 }
		 if ((pHead = LocalAlloc( LPTR , sz+1 )))
		 {
           *pHead ='\x0';
	       if ((fp1= fopen(pContext->mFnData, "rb")))
		   {
             while(fgets(mLine, sizeof(mLine)-1, fp1))
			 {
		       if (mLine[0] == '\r' || mLine[0] == '\n')
			   {
			     break;
			   }
			   strcat(pHead, mLine);
			 }
	         fclose(fp1);
		   }
#ifndef UPDATE_20171103  // MILTERでヘッダ置換えが失敗しないようにする対策
		   if ((fp1= fopen(mFnHData, "rb")))
		   {
             while(fgets(mLine, sizeof(mLine)-1, fp1))
			 {
			   strcat(pHead, mLine);
			 }
		     fclose(fp1);
		   }
#endif
           /////////
		   if ((fp1= fopen(mFnEData, "rb")))
		   {
			 bNewHead = TRUE;
             while(fgets(mLine, sizeof(mLine)-1, fp1))
			 {
#ifdef UPDATE_20171103  // MILTERでヘッダ置換えが失敗しないようにする対策
			   if (!strnicmp(mLine, "M:", 2))
			     strtok(mLine, "\r\n");
#endif			   
			   if (mLine[0] != ' ' && mLine[0] != '\t')
			   {
			     if ((p=strchr(&mLine[2], ':')))
				 {
#ifdef UPDATE_20171103  // MILTERでヘッダ置換えが失敗しないようにする対策
				   if (strnicmp(mLine, "M:", 2) && !bNewHead)
#else
				   if (!bNewHead)
#endif
				   {
                     op_header(pHead, mLine1, c, idx);
				   } 
				   p++;
				   bNewHead = FALSE;
				   c = mLine[0];
				   idx = atoi(&mLine[2]);
#ifdef UPDATE_20171103  // MILTERでヘッダ置換えが失敗しないようにする対策
				   if (!strnicmp(mLine, "M:", 2))
                     op_header(pHead, p, c, idx);
				   else
#endif
			         strcpy(mLine1, p);
				 }
			   } else {
			     strcat(mLine1, mLine);
			   }
			 }
#ifdef UPDATE_20171103  // MILTERでヘッダ置換えが失敗しないようにする対策
		     if (strnicmp(mLine, "M:", 2))
#endif
               op_header(pHead, mLine1, c, idx);

		     fclose(fp1);
		   }
           /////////
#ifdef UPDATE_20171103  // MILTERでヘッダ置換えが失敗しないようにする対策
		   if ((fp1= fopen(mFnHData, "rb")))
		   {
             while(fgets(mLine, sizeof(mLine)-1, fp1))
			 {
			   strcat(pHead, mLine);
			 }
		     fclose(fp1);
		   }
#endif
		   ///////////////////////////////////////
           while(!MoveFileEx(pContext->mFnData, mFnData, MOVEFILE_COPY_ALLOWED | MOVEFILE_WRITE_THROUGH)) 
		   {
	         if (bServiceTerminating ||
                 GetLastError() == ERROR_FILE_NOT_FOUND)
	           break;
              _sleep(100);
		   }
		   if ((fp1= fopen(mFnData, "rb")))
		   {
		     if ((fp2= fopen(pContext->mFnData, "wb")))
			 {
			   while(fgets(mLine, sizeof(mLine)-1, fp1))
			   {
			     if (bHead1)
				 {
			       if (mLine[0] == '\r' || mLine[0] == '\n')
				   {
				     bHead1 = FALSE; // ヘッダー終わり
				     fputs(pHead, fp2);
			         fputs("\r\n", fp2); // ヘッダの終了を挿入
				   }
				 } else {
		           if ((fp3= fopen(mFnMData, "rt")))
				   {
			         while(fgets(mLine1, sizeof(mLine1)-1, fp3))
					 {
  				       fputs(mLine1, fp2); // ヘッダの最後に挿入
					 }
				     fclose(fp3);
 	                 while(!DeleteFile(mFnMData)) 
					 {
  	                   if (bServiceTerminating ||
                           GetLastError() == ERROR_FILE_NOT_FOUND)
		                 break;
                        _sleep(100);
					 }
				   } else { // 本文ファイルが無い場合、置き換えしない
 				     fputs(mLine, fp2);
				   }
				 }
			   }
			   if (bHead1) // ヘッダが区切りが無くて終わった場合
			   {
				 bHead1 = FALSE; // ヘッダー終わり
				 fputs(pHead, fp2);
			   }
			   fclose(fp2);
			 }
		     fclose(fp1);
		   }
		   ////////
#ifdef UPDATE_20171103  // MILTERでヘッダ置換えが失敗しないようにする対策
	       while(!DeleteFile(mFnHData)) 
		   {
	         if (bServiceTerminating ||
                 GetLastError() == ERROR_FILE_NOT_FOUND)
		       break;
             _sleep(100);
		   }
#endif
	       while(!DeleteFile(mFnEData)) 
		   {
	         if (bServiceTerminating ||
                 GetLastError() == ERROR_FILE_NOT_FOUND)
		       break;
             _sleep(100);
		   }
	       while(!DeleteFile(mFnData)) 
		   {
	         if (bServiceTerminating ||
                 GetLastError() == ERROR_FILE_NOT_FOUND)
		       break;
             _sleep(100);
		   }
		   while(!(fp1= fopen(pContext->mFnData, "rb"))) 
		   {
	         if (bServiceTerminating)
	           break;
             _sleep(100);
		   }
		   fclose(fp1);
		 }
         if (LocalFlags(pHead) != LMEM_INVALID_HANDLE) // 解放可能かどうか確認
		 {
  	       LocalFree(pHead);
		 }
	     if (!bSts)
		 {
	       break;
		 }
	   }
	 }
   }
   return bSts;
}

// ヘッダ位置を決める
char * move_headerptr(char *p, char *pHead, int idx)
{
   int i;
   char *q;

   q = pHead;
   for (i = 1; i < idx; i++) // ヘッダ位置
   {
     while((q = strchr(p, '\n')))
	 {
	   q++;
	   p = q;
	   if (*q != ' ' && *q != '\t')
	   {
	     break;
	   }
	 }
   }
   
   return q;
}

void op_header(char *pLine, char *pLine1, char mod, unsigned idx)
{
  int i, j;
  unsigned int n, n2;
  CHAR *p, *q, *r;

//  p = pLine;
//  p = move_headerptr(p, pLine, idx+1);
  if (mod == 'M') // 入れ替え
  {
#ifdef UPDATE_20171103  // MILTERでヘッダ置換えが失敗しないようにする対策
	if ((p = strstr(pLine, pLine1)))
	{
	   r = p;
	   while ((q = strstr(r, "\r\n")))
	   {
		 if (*(q+2) != ' ' && *(q+2) != '\t')
		 {
		    //q+=2;
	        r=q+2;
		    break;
		  }
		  r=q+2;
	   };
	   j = strlen(r);
	   for (i = 0; i < j; i++)
	   {
		 *(p+i) = *(r+i);
	   }
	   *(p+i) = *(r+i);
	}
#else
    q = pLine;
    q = move_headerptr(q, pLine, idx); // 入れ替え先頭位置
    n = strlen(pLine1); // 挿入ヘッダ
    n2 = strlen(p); // 元のヘッダ群
    r = q + n; // 入れ替えた後の次のヘッダ位置
    if (r < p)
    {
      for (j = 0; j <= n2; j++) // 元ヘッダ位置移動
	  { 
        *(r+j) = *(p+j); 
	  }
	} else if (r > p) {
      for (j = n2; j >= 0; j--) // 元ヘッダ位置移動
	  { 
        *(r+j) = *(p+j); 
	  }
    }
    memcpy(q, pLine1, n); // ヘッダの挿入
#endif
  } else { // c == 'I' 挿入
#ifdef UPDATE_20171103  // MILTERでヘッダ置換えが失敗しないようにする対策
    p = pLine;
    p = move_headerptr(p, pLine, idx+1);
#endif
    n = strlen(pLine1); // 挿入ヘッダ
    n2 = strlen(p); // 元のヘッダ群
    r = p + n;
    for (j = n2; j >= 0; j--) // 元ヘッダ位置移動
    {
      *(r+j) = *(p+j); 
    }
    memcpy(p, pLine1, n); // ヘッダの挿入
  }
}

void close_milter_session(struct _MILTER *pmlt)
{	     
  if (pmlt->s)
  {
    closesocket(pmlt->s);
    pmlt->s = INVALID_SOCKET;
  }
  if (pmlt->h)
  {
    CloseHandle(pmlt->h);
    pmlt->h = NULL;
  }
}

#ifdef UPDATE_20150210 // E-POST用 MAIL FILTER インターフェースへのマクロ指定を追加。

//${mail_addr} 現在のトランザクションの MAIL FROM アドレス。
//${mail_host} 現在のトランザクションの MAIL FROM アドレスのホスト部分。
//${rcpt_addr} 現在のトランザクションの RCPT TO アドレス。
//${rcpt_host} 現在の RCPT TO アドレスのホスト部分。

char * macro_argument(PCLIENT_CONTEXT lpClientContext, int nFlg, unsigned int *sz, char *pMacro, char *pb)
{
  BOOL bOK;
  char *o, *q, *r, mMacro[256], mData[256];
  PSmtpContext pContext = &lpClientContext->Context;

  strcpy(mMacro, pMacro);
  q = mMacro;
  while(*q)
  {
	if ((r = strchr(q, ',')))
	{
	  *r = '\0';
	}
	while(*q == ' ')
	{
	  q++;
	}
	// マクロデータ設定
	bOK = TRUE;
	mData[0] = '\0';
    if (!strcmp(q, "i") && (nFlg == MA_DATA ||
		                    nFlg == MA_EOM)) // DATA, EOM  キューID 
	{
#ifdef UPDATE_20071204  // メッセージＩＤ採番処理を修正(Bym10id)
	  strcpy(mData, pContext->mMsgId); // ダミー
#else
	  sprintf(mData, "B%010lu", pContext->nMsgId); // ダミー
#endif
	} else if (!strcmp(q, "j")) // いつでも myhostname の値  
	{
	  strcpy(mData, pContext->LocalName);
	  /*
	} else if (!strcmp(q, "_")) // いつでも 検証されたクライアント名とアドレス 
	{
	  strcpy(mData, "0"); // ダミー
	} else if (!strcmp(q, "{auth_authen}") && (nFlg == MA_MAIL || 
		                                       nFlg == MA_DATA || 
											   nFlg == MA_EOM)) // MAIL, DATA, EOM  クライアントの認証資格 (認証に使用される機構) と承認 ID (AUTH=パラメータ。指定されている場合) を保持します。
	{
	  strcpy(mData, pContext->mUSER); // ログインID
	} else if (!strcmp(q, "{auth_author}") && (nFlg == MA_MAIL || 
		                                       nFlg == MA_DATA ||
											   nFlg == MA_EOM)) // MAIL, DATA, EOM  クライアントの認証資格 (認証に使用される機構) と承認 ID (AUTH=パラメータ。指定されている場合) を保持します。
	{
	  strcpy(mData, pContext->mUIDFROM); // 送信者
	} else if (!strcmp(q, "{auth_type}") && (nFlg == MA_MAIL ||
		                                     nFlg == MA_DATA ||
											 nFlg == MA_EOM)) // MAIL, DATA, EOM  クライアントの認証資格 (認証に使用される機構) と承認 ID (AUTH=パラメータ。指定されている場合) を保持します。
	{
	  strcpy(mData, (pContext->nAUTHMODE == -1 ? "" :(pContext->nAUTHMODE == 0 ? "plain" : (pContext->nAUTHMODE == 3 ? "cram-md5" : "login")))); // ダミー
	  
	} else if (!strcmp(q, "{if_addr}")) // いつでも インタフェースがループバックネット上にない場合に、受信接続用インタフェースのアドレスを提供します。このマクロは、特に仮想ホスティングに便利です。 
	{
	  strcpy(mData, pContext->MyAddr);
	} else if (!strcmp(q, "{if_name}")) // いつでも 受信接続用のインタフェースのホスト名を提供します。これは、特に仮想ホスティングに便利です。  
	{
      gethostname(mData, sizeof(mData)-1);
	  */
	} else if (!strcmp(q, "{client_addr}")) // いつでも クライアント IP アドレス 
	{
	  strcpy(mData, pContext->PeerAddr);
	  /*
	} else if (!strcmp(q, "{client_name}")) // いつでも クライアントホスト名 (検索、照合に失敗したときは "unknown") 
	{
	  strcpy(mData, pContext->PeerName);
	  */
	  /*
	} else if (!strcmp(q, "{client_connections}") && (nFlg == MA_CONNECT)) // CONNECT  このクライアントの同時接続 
	{
	  strcpy(mData, "0"); // ダミー
	  */
	  /*
	} else if (!strcmp(q, "{client_ptr}") && (nFlg == MA_HELO ||
		                                      nFlg == MA_MAIL || 
											  nFlg == MA_DATA ||
											  nFlg == MA_EOM)) // HELO, MAIL, DATA, EOM  TLS クライアント証明書の issuer (発行者) 
	{
	  strcpy(mData, ""); // ダミー
	} else if (!strcmp(q, "{cert_issuer}") && (nFlg == MA_CONNECT ||
		                                       nFlg == MA_HELO || 
											   nFlg == MA_MAIL || 
											   nFlg == MA_DATA)) // CONNECT, HELO, MAIL, DATA  クライアントの逆引き名 (検索に失敗したときは "unknown") 
	{
	  strcpy(mData, ""); // ダミー
	} else if (!strcmp(q, "{cert_subject}") && (nFlg == MA_HELO ||
		                                        nFlg == MA_MAIL ||
												nFlg == MA_DATA ||
												nFlg == MA_EOM)) // HELO, MAIL, DATA, EOM  TLS クライアント証明書の subject 
	{
	  strcpy(mData, ""); // ダミー
	} else if (!strcmp(q, "{cipher_bits}") && (nFlg == MA_HELO || 
		                                       nFlg == MA_MAIL || 
											   nFlg == MA_DATA || 
											   nFlg == MA_EOM)) // HELO, MAIL, DATA, EOM  TLS セッション鍵のサイズ 
	{
	  strcpy(mData, "");
	} else if (!strcmp(q, "{cipher}") && (nFlg == MA_HELO ||
		                                  nFlg == MA_MAIL || 
										  nFlg == MA_DATA || 
										  nFlg == MA_EOM)) // HELO, MAIL, DATA, EOM  TLS 暗号化方式 
	{
	  strcpy(mData, "");
	} else if (!strcmp(q, "{daemon_name}")) // いつでも milter_macro_daemon_name の値  
	{
	  strcpy(mData, SMTP_SERVICE); // ダミー
*/
	} else if (!strcmp(q, "{mail_addr}") && nFlg == MA_MAIL) // MAIL  送信者アドレス 
	{
	  strcpy(mData, pContext->mUIDFROM);
	} else if (!strcmp(q, "{mail_host}") && nFlg == MA_MAIL) // MAIL  送信者アドレス のホスト名
	{
	  strcpy(mData, "");
	  if ((o = strchr(pContext->mUIDFROM, '@')))
	  {
		o++;
	    strcpy(mData, o);
	  }
	} else if (!strcmp(q, "{rcpt_addr}") && nFlg == MA_RCPT) // RCPT  受信者アドレス 
	{
	  strcpy(mData, pContext->mUIDRCPT);
	} else if (!strcmp(q, "{rcpt_host}") && nFlg == MA_RCPT) // RCPT  受信者アドレス のホスト名
	{
	  strcpy(mData, "");
	  if ((o = strchr(pContext->mUIDRCPT, '@')))
	  {
		o++;
	    strcpy(mData, o);
	  }
	  /*
	} else if (!strcmp(q, "{tls_version}") && (nFlg == MA_HELO || 
		                                       nFlg == MA_MAIL || 
											   nFlg == MA_DATA || 
											   nFlg == MA_EOM)) // HELO, MAIL, DATA, EOM  TLS プロトコルバージョン 
	{
	  strcpy(mData, "");
	  */
	  /*
	} else if (!strcmp(q, "v")) // いつでも milter_macro_v の値 
	{
	  strcpy(mData, """");
	  */
	} else {
	  bOK = FALSE; // 未対応のマクロ名
	}
	if (bOK)
	{
      strcpy(pb, q); pb+=strlen(q); *pb++ = '\0';
      strcpy(pb, mData); pb+=strlen(mData); *pb++ = '\0';
	  *sz += strlen(q)+1+strlen(mData)+1;
	}
	if (r)
	{
	  q = r+1; // 次データ
	} else {
	  break;
	}
  }
  return pb;
}
#endif

#endif