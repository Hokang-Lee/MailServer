/****************************************************************************
Copyright (C) 2017, K-TEC Inc. Katsuyuki Kawakami, http://www.ktinc.jp/

This program is free software; 
you can redistribute it and/or modify it under the terms of the GNU General 
Public License as published by the Free Software Foundation; 
either version 3 of the License, or (at your option) any later version. 

This program is distributed in the hope that it will be useful, but WITHOUT ANY 
WARRANTY; 
without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
PURPOSE. 
See the GNU General Public License for more details. 

You should have received a copy of the GNU General Public License along with this 
program. If not, see <http://www.gnu.org/licenses/>.
-------------------------------------------------------------------------------
このプログラムはフリーソフトウェアです。
あなたはこれを、フリーソフトウェア財団によって発行されたGNU 一般公衆利用許諾書
(バージョン３か、それ以降のバージョンのうちどれか)が定める条件の下で再頒布または
改変 することができます。 

このプログラムは有用であることを願って頒布されますが、*全くの無保証*です。
商業可能性の保証や特定目的への適合性は、言外に示されたものも含め、全く存在しません。
詳しくはGNU 一般公衆利用許諾書をご覧ください。 

あなたはこのプログラムと共に、GNU 一般公衆利用許諾書のコピーを一部受け取っている
はずです。
もし受け取っていなければ、<http://www.gnu.org/licenses/> をご覧ください。 
****************************************************************************/
#include "imap4.h"

#ifndef __CODECV_H__
#define __CODEV_H__

#ifdef WINDOWS
#ifdef __cplusplus
extern "C"
{
#endif

//---------------------------------------------------------------------------
// 外部提供関数
//---------------------------------------------------------------------------
extern int doconv(INT srccp, INT destcp, const char * ssrc,char * sdst,int idstsz);

#ifdef __cplusplus
}
#endif
#else
#define _T (const char *)
extern int doconv(char *psrccd, char *pnestcd, const char *inbuf, char *outbuf, size_t bufsize);
#endif

#endif
