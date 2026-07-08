#include <windows.h>
#include <winnls.h>
#include <string.h>

#ifndef __UTF7CV_H__
#define __UTF7CV_H__

#ifdef __cplusplus
extern "C"
{
#endif

//---------------------------------------------------------------------------
// ŠO•”’ñ‹ŸŠÖ”
//---------------------------------------------------------------------------
extern int SJIS2UTF7N(const char * ssrc,char * sdst,int idstsz);
extern int UTF7N2SJIS(const char * ssrc,char * sdst,int idstsz);

#ifdef __cplusplus
}
#endif

#endif

