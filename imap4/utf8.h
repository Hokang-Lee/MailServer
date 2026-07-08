#include <windows.h>
#include <winnls.h>
#include <string.h>

#ifndef __UTF8CV_H__
#define __UTF8CV_H__

#ifdef __cplusplus
extern "C"
{
#endif

//---------------------------------------------------------------------------
// ŠO•”’ñ‹ŸŠÖ”
//---------------------------------------------------------------------------
extern int SJIS2UTF8N(const char * ssrc,char * sdst,int idstsz);
extern int UTF8N2SJIS(const char * ssrc,char * sdst,int idstsz);
extern int CODEPAGE2UTF8N(INT copdepage, const char * ssrc,char * sdst,int idstsz);
extern int UTF8N2CODEPAGE(INT copdepage, const char * ssrc,char * sdst,int idstsz);

#ifdef __cplusplus
}
#endif

#endif

