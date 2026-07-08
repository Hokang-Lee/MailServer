////////////////////////////////////////////////////////////
// CAPABILITY.C Copyright K.kawakami
////////////////////////////////////////////////////////////
#include "imap4.h"

#ifdef UPDATE_20091116 // CAPABILITY 命令で応答する認証方法を設定変更可能にした。
extern CHAR mImapAuthType[];
#endif
#ifdef UPDATE_20140528 // IDLEコマンド実装
extern BOOL bIDLECmd; // IDLEコマンド有効有無 TRUE:有効 FALSE:無効
#endif
#ifdef UPDATE_20151125 // SSL証明書と秘密鍵が指定されているときCAPABILITY命令にSTARTTLSを含めるようにした。
extern BOOL    bUsedSTLS;
#endif

BOOL CAPABILITYDispatch(PCLIENT_CONTEXT lpClientContext) {
  PImap4Context pContext = &lpClientContext->Context;
#ifdef UPDATE_20091116 // CAPABILITY 命令で応答する認証方法を設定変更可能にした。
  CHAR *p, mAuth[256];
#endif
#ifdef UPDATE_20180820A // SASLにPLAIN方式を追加
  CHAR *q;
#endif
#ifdef ADD_XOAUTH2_A // OAUTH2を加えるとCAPABILITYの表記が認証方式３つまでしか表記できない
  CHAR *r = NULL;
#endif

#ifdef UPDATE_20091116 // CAPABILITY 命令で応答する認証方法を設定変更可能にした。
  strcpy(mAuth, mImapAuthType);
  if (p = strpbrk(mAuth, " ,\t"))
  {
	*p = '\x0';
	p++;
#ifdef UPDATE_20180820A // SASLにPLAIN方式を追加
    if (q = strpbrk(p, " ,\t"))
	{
      *q = '\x0';
	  q++;
#ifdef ADD_XOAUTH2_A // OAUTH2を加えるとCAPABILITYの表記が認証方式３つまでしか表記できない
	  if (r = strpbrk(q, " ,\t\r\n"))
	  {
        *r = '\x0';
		r++;
	  } 
#else
	  strtok(q, " ,\t\r\n");
#endif
	} else
 	  strtok(p, " ,\t\r\n");
#else
	strtok(p, " ,\t\r\n");
#endif
  }
  strtok(mAuth, " ,\t\r\n");
#ifdef ADD_XOAUTH2_A // OAUTH2を加えるとCAPABILITYの表記が認証方式３つまでしか表記できない
  if (p && q && r)
  {
#ifdef ADD_METADATA //ADD_ID_XDELTAPUSH
    //sprintf(pContext->mMessages, "* CAPABILITY IMAP4 IMAP4rev1 ID XDELTAPUSH %s%sNAMESPACE AUTH=%s AUTH=%s AUTH=%s AUTH=%s\r\n", ((bUsedSTLS || pContext->bUsedSTLS) && !pContext->bUsedSSL ? "STARTTLS ": ""), (bIDLECmd ? "IDLE " : ""), mAuth, p, q, r);
    sprintf(pContext->mMessages, "* CAPABILITY IMAP4 IMAP4rev1 METADATA XCHATMAIL X-CHATMAIL %s%sNAMESPACE AUTH=%s AUTH=%s AUTH=%s AUTH=%s\r\n", ((bUsedSTLS || pContext->bUsedSTLS) && !pContext->bUsedSSL ? "STARTTLS ": ""), (bIDLECmd ? "IDLE " : ""), mAuth, p, q, r);
#else
    sprintf(pContext->mMessages, "* CAPABILITY IMAP4 IMAP4rev1 %s%sNAMESPACE AUTH=%s AUTH=%s AUTH=%s AUTH=%s\r\n", ((bUsedSTLS || pContext->bUsedSTLS) && !pContext->bUsedSSL ? "STARTTLS ": ""), (bIDLECmd ? "IDLE " : ""), mAuth, p, q, r);
#endif
//	  sprintf(pContext->mMessages, "* CAPABILITY IMAP4rev1 SASL-IR LOGIN-REFERRALS ID ENABLE IDLE SORT SORT=DISPLAY THREAD=REFERENCES THREAD=REFS THREAD=ORDEREDSUBJECT MULTIAPPEND URL-PARTIAL CATENATE UNSELECT CHILDREN NAMESPACE UIDPLUS LIST-EXTENDED I18NLEVEL=1 CONDSTORE QRESYNC ESEARCH ESORT SEARCHRES WITHIN CONTEXT=SEARCH LIST-STATUS BINARY MOVE SNIPPET=FUZZY PREVIEW=FUZZY PREVIEW STATUS=SIZE SAVEDATE LITERAL+ NOTIFY SPECIAL-USE\r\n");
  } else 
#endif
#ifdef UPDATE_20180820A // SASLにPLAIN方式を追加
  if (p && q)
  {
#ifdef ADD_METADATA //ADD_ID_XDELTAPUSH
//    sprintf(pContext->mMessages, "* CAPABILITY IMAP4 IMAP4rev1 ID XDELTAPUSH %s%sNAMESPACE AUTH=%s AUTH=%s AUTH=%s\r\n", ((bUsedSTLS || pContext->bUsedSTLS) && !pContext->bUsedSSL ? "STARTTLS ": ""), (bIDLECmd ? "IDLE " : ""), mAuth, p, q);
    sprintf(pContext->mMessages, "* CAPABILITY IMAP4 IMAP4rev1 METADATA XCHATMAIL X-CHATMAIL %s%sNAMESPACE AUTH=%s AUTH=%s AUTH=%s\r\n", ((bUsedSTLS || pContext->bUsedSTLS) && !pContext->bUsedSSL ? "STARTTLS ": ""), (bIDLECmd ? "IDLE " : ""), mAuth, p, q);
#else
    sprintf(pContext->mMessages, "* CAPABILITY IMAP4 IMAP4rev1 %s%sNAMESPACE AUTH=%s AUTH=%s AUTH=%s\r\n", ((bUsedSTLS || pContext->bUsedSTLS) && !pContext->bUsedSSL ? "STARTTLS ": ""), (bIDLECmd ? "IDLE " : ""), mAuth, p, q);
#endif
//	  sprintf(pContext->mMessages, "* CAPABILITY IMAP4rev1 SASL-IR LOGIN-REFERRALS ID ENABLE IDLE SORT SORT=DISPLAY THREAD=REFERENCES THREAD=REFS THREAD=ORDEREDSUBJECT MULTIAPPEND URL-PARTIAL CATENATE UNSELECT CHILDREN NAMESPACE UIDPLUS LIST-EXTENDED I18NLEVEL=1 CONDSTORE QRESYNC ESEARCH ESORT SEARCHRES WITHIN CONTEXT=SEARCH LIST-STATUS BINARY MOVE SNIPPET=FUZZY PREVIEW=FUZZY PREVIEW STATUS=SIZE SAVEDATE LITERAL+ NOTIFY SPECIAL-USE\r\n");
  } else 
#endif
  if (p)
  {
#ifdef ADD_METADATA //ADD_ID_XDELTAPUSH
//    sprintf(pContext->mMessages, "* CAPABILITY IMAP4 IMAP4rev1 ID XDELTAPUSH %s%sNAMESPACE AUTH=%s AUTH=%s\r\n", ((bUsedSTLS || pContext->bUsedSTLS) && !pContext->bUsedSSL ? "STARTTLS ": ""), (bIDLECmd ? "IDLE " : ""), mAuth, p);
    sprintf(pContext->mMessages, "* CAPABILITY IMAP4 IMAP4rev1 METADATA XCHATMAIL X-CHATMAIL %s%sNAMESPACE AUTH=%s AUTH=%s\r\n", ((bUsedSTLS || pContext->bUsedSTLS) && !pContext->bUsedSSL ? "STARTTLS ": ""), (bIDLECmd ? "IDLE " : ""), mAuth, p);
#else
#ifdef UPDATE_20151125 // SSL証明書と秘密鍵が指定されているときCAPABILITY命令にSTARTTLSを含めるようにした。
    sprintf(pContext->mMessages, "* CAPABILITY IMAP4 IMAP4rev1 %s%sNAMESPACE AUTH=%s AUTH=%s\r\n", ((bUsedSTLS || pContext->bUsedSTLS) && !pContext->bUsedSSL ? "STARTTLS ": ""), (bIDLECmd ? "IDLE " : ""), mAuth, p);
#else
#ifdef UPDATE_20140528 // IDLEコマンド実装
	sprintf(pContext->mMessages, "* CAPABILITY IMAP4 IMAP4rev1 %sNAMESPACE AUTH=%s AUTH=%s\r\n", (bIDLECmd ? "IDLE " : ""), mAuth, p);
#else
    sprintf(pContext->mMessages, "* CAPABILITY IMAP4 IMAP4rev1 NAMESPACE AUTH=%s AUTH=%s\r\n", mAuth, p);
#endif
#endif
#endif
  } else {
	if (mAuth[0])
	{
#ifdef ADD_METADATA //ADD_ID_XDELTAPUSH
//      sprintf(pContext->mMessages, "* CAPABILITY IMAP4 IMAP4rev1 ID XDELTAPUSH X-CHATMAIL %s%sNAMESPACE AUTH=%s\r\n", ((bUsedSTLS || pContext->bUsedSTLS)  && !pContext->bUsedSSL ? "STARTTLS ": ""), (bIDLECmd ? "IDLE " : ""), mAuth);
      sprintf(pContext->mMessages, "* CAPABILITY IMAP4 IMAP4rev1 METADATA XCHATMAIL X-CHATMAIL %s%sNAMESPACE AUTH=%s\r\n", ((bUsedSTLS || pContext->bUsedSTLS)  && !pContext->bUsedSSL ? "STARTTLS ": ""), (bIDLECmd ? "IDLE " : ""), mAuth);
#else
#ifdef UPDATE_20151125 // SSL証明書と秘密鍵が指定されているときCAPABILITY命令にSTARTTLSを含めるようにした。
      sprintf(pContext->mMessages, "* CAPABILITY IMAP4 IMAP4rev1 %s%sNAMESPACE AUTH=%s\r\n", ((bUsedSTLS || pContext->bUsedSTLS)  && !pContext->bUsedSSL ? "STARTTLS ": ""), (bIDLECmd ? "IDLE " : ""), mAuth);
#else
#ifdef UPDATE_20140528 // IDLEコマンド実装
      sprintf(pContext->mMessages, "* CAPABILITY IMAP4 IMAP4rev1 %sNAMESPACE AUTH=%s\r\n", (bIDLECmd ? "IDLE " : ""), mAuth);
#else
      sprintf(pContext->mMessages, "* CAPABILITY IMAP4 IMAP4rev1 NAMESPACE AUTH=%s\r\n", mAuth);
#endif
#endif
#endif
	} else {
#ifdef ADD_METADATA //ADD_ID_XDELTAPUSH
//      sprintf(pContext->mMessages, "* CAPABILITY IMAP4 IMAP4rev1 ID XDELTAPUSH %s%sNAMESPACE\r\n", ((bUsedSTLS || pContext->bUsedSTLS)  && !!pContext->bUsedSSL ? "STARTTLS ": ""), (bIDLECmd ? "IDLE " : ""));
      sprintf(pContext->mMessages, "* CAPABILITY IMAP4 IMAP4rev1 METADATA XCHATMAIL X-CHATMAIL %s%sNAMESPACE\r\n", ((bUsedSTLS || pContext->bUsedSTLS)  && !!pContext->bUsedSSL ? "STARTTLS ": ""), (bIDLECmd ? "IDLE " : ""));
#else
#ifdef UPDATE_20151125 // SSL証明書と秘密鍵が指定されているときCAPABILITY命令にSTARTTLSを含めるようにした。
      sprintf(pContext->mMessages, "* CAPABILITY IMAP4 IMAP4rev1 %s%sNAMESPACE\r\n", ((bUsedSTLS || pContext->bUsedSTLS)  && !!pContext->bUsedSSL ? "STARTTLS ": ""), (bIDLECmd ? "IDLE " : ""));
#else
#ifdef UPDATE_20140528 // IDLEコマンド実装
      sprintf(pContext->mMessages, "* CAPABILITY IMAP4 IMAP4rev1 %sNAMESPACE\r\n", (bIDLECmd ? "IDLE " : ""));
#else
      strcpy(pContext->mMessages, "* CAPABILITY IMAP4 IMAP4rev1 NAMESPACE\r\n");
#endif
#endif
#endif
	}
  }
#else
#ifdef ADD_METADATA //ADD_ID_XDELTAPUSH
//  sprintf(pContext->mMessages, "* CAPABILITY IMAP4 IMAP4rev1 ID XDELTAPUSH X-CHATMAIL %sNAMESPACE AUTH=CRAM-MD5 AUTH=LOGIN\r\n", (bIDLECmd ? "IDLE " : ""));
  sprintf(pContext->mMessages, "* CAPABILITY IMAP4 IMAP4rev1 METADATA XCHATMAIL X-CHATMAIL %sNAMESPACE AUTH=CRAM-MD5 AUTH=LOGIN\r\n", (bIDLECmd ? "IDLE " : ""));
#else
#ifdef UPDATE_20140528 // IDLEコマンド実装
  sprintf(pContext->mMessages, "* CAPABILITY IMAP4 IMAP4rev1 %sNAMESPACE AUTH=CRAM-MD5 AUTH=LOGIN\r\n", (bIDLECmd ? "IDLE " : ""));
#else
  sprintf(pContext->mMessages, "* CAPABILITY IMAP4 IMAP4rev1 NAMESPACE AUTH=CRAM-MD5 AUTH=LOGIN\r\n");
#endif
#endif
#endif
  //sprintf(pContext->mMessages, "* CAPABILITY IMAP4 IMAP4rev1 AUTH=LOGIN\r\n");
  //sprintf(pContext->mMessages, "* CAPABILITY IMAP4 IMAP4rev1 NAMESPACE AUTH=LOGIN\r\n");
  put_reply(lpClientContext, TRUE, TRUE);
  sprintf(pContext->mMessages, "%s %s %s completed\r\n", pContext->pSquence, IMAP4_GOOD_RESPONSE, pContext->pCmd );
  return TRUE;
}