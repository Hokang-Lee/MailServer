////////////////////////////////////////////////////////////
// Service.h Copyright K.kawakami
////////////////////////////////////////////////////////////
#ifndef _SERVICE_H
#define _SERVICE_H


#ifdef __cplusplus
extern "C" {
#endif


//////////////////////////////////////////////////////////////////////////////
//// todo: change to desired strings
////
// name of the executable
#define SZAPPNAME            ESMTP_NAME
// internal name of the service
#define SZSERVICENAME        SMTP_SERVICE //"SPARS" //SMTP_NAME
// displayed name of the service
#define SZSERVICEDISPLAYNAME ESMTP_NAME
#define SZDEPENDENCIES       ""
//////////////////////////////////////////////////////////////////////////////

VOID ServiceStart(DWORD dwArgc, LPTSTR *lpszArgv);
VOID ServiceStop();
//////////////////////////////////////////////////////////////////////////////
BOOL ReportStatusToSCMgr(DWORD dwCurrentState, DWORD dwWin32ExitCode, DWORD dwWaitHint);
VOID AddToMessageLog(LPTSTR lpszMsg, DWORD numMessage, LPTSTR lpformat, WORD wType);
//void AddToMessageLog(LPTSTR lpszMsg, LPTSTR lpformat, WORD wType);


#ifdef __cplusplus
}
#endif

#endif
