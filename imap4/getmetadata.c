////////////////////////////////////////////////////////////
// GETMETADATA.C Copyright K.kawakami
////////////////////////////////////////////////////////////
#include "imap4.h"

#ifdef ADD_METADATA
BOOL GETMETADATADispatch(PCLIENT_CONTEXT lpClientContext) {
  char *p, *q;
  
  PImap4Context pContext = &lpClientContext->Context;

  p = pContext->pToken;
  if (*p == '"') {
    p++;
    strtok(p, "\"");
    if (*p == '"')
	  *p = '\x0';
#ifdef UPDATE_20050531  // 命令にバッファフローの脆弱性
     if (strlen(p) > MAXIMAPFOLDER) // フォルダ名が長い場合の処置
  	   *(p+MAXIMAPFOLDER) = '\x0';
#endif
  }

// オプション
//MAXSIZE (METADATAの出力サイズ制限)
//DEPTH  (0:無制限)

//  sprintf(pContext->mMessages, "* METADATA \"%s\" (/shared/comment \"Shared comment\")\r\n", p);
//* METADATA "" (/shared/vendor/deltachat/irohrelay "https://mail.mydomain.jp")
  //sprintf(pContext->mMessages, "* METADATA \"%s\" (/shared/vendor/deltachat/irohrelay \"https://chat.e-postinc.jp\")", p);
  //sprintf(pContext->mMessages, "* METADATA \"\" (/shared/comment NIL /shared/admin NIL /shared/vendor/deltachat/iroh-relay \"https://chat.e-postinc.jp/\" /shared/vendor/deltachat/turn NIL)\r\n", p);
  //sprintf(pContext->mMessages, "* METADATA \"%s\" (/shared/comment NIL /shared/admin NIL /shared/vendor/deltachat/irohrelay \"https://chat.e-postinc.jp\" /shared/vendor/deltachat/turn \"turn:chat.example.org:3478?transport=udp&username=user&password=password\")\r\n", p);
  //sprintf(pContext->mMessages, "* METADATA \"\" (/shared/comment NIL /shared/admin NIL /shared/vendor/deltachat/iroh-relay \"https://nine.testrun.org\" /shared/vendor/deltachat/turn NIL)\r\n", p);
//  sprintf(pContext->mMessages, "* METADATA \"\" (/shared/comment NIL /shared/admin NIL /shared/vendor/deltachat/irohrelay \"https://chat.e-postinc.jp\" /shared/vendor/deltachat/turn NIL /shared/vendor/deltachat/push-proxy \"https://push.delta.chat\")\r\n", p);
  sprintf(pContext->mMessages, "* METADATA \"\" (/shared/comment NIL /shared/admin NIL /shared/vendor/deltachat/irohrelay \"https://chat.e-postinc.jp\" /shared/vendor/deltachat/turn NIL)\r\n", p);
  put_reply(lpClientContext, TRUE, TRUE);
 // * METADATA "" (/shared/comment "Shared comment")
  sprintf(pContext->mMessages, "%s %s %s completed\r\n", pContext->pSquence, IMAP4_GOOD_RESPONSE, pContext->pCmd );

  return TRUE;
}
#endif