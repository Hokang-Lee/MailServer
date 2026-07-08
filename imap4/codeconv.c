//---------------------------------------------------------------------------
// 文字コード変換
//---------------------------------------------------------------------------
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
#include "codeconv.h"

//---------------------------------------------------------------------------
// doconvcodepage()
//! 実際の変換処理。
/*!
\param	srccp   [i]		変換元の文字のページコード
\param	destcp  [i]		変換後の文字のページコード
\param	ssrc	[i]		変換元の文字列ポインタ
\param	sdst	[i/o]	変換後の文字列を返す先のポインタ
\param	idstsz	[i]		wdstの上限サイズ。この値-1byteまで記録できる
\retval	0		エラー
\retval	1以上	変換後のバイト数
\retval	-1		作業メモリの確保失敗
*/
//---------------------------------------------------------------------------
#ifdef WINDOWS
int doconv(INT srccp, INT destcp, const char * ssrc,char * sdst,int idstsz)
{
	int i;
    int ires;
    LPWSTR wbuf;
    LPSTR  putf;

    // unicodeへ変換
	//// 事前チェック
	ires = MultiByteToWideChar(srccp, // 現文字コード
		0,				// フラグなし
		ssrc,			// 変換元文字列
        -1,				// 変換文字列バイト数
        NULL,			// 変換後格納先
        0				// 格納領域取得
		);
	if (ires == 0)
    {
    	return 0;
    }
    //// ワーク取得
    wbuf = new WCHAR[ires+1];
    if (wbuf == NULL)
    {
    	return -1;
    }
	//// 実変換
	ires = MultiByteToWideChar(srccp, // 現文字コード
		0,				// フラグなし
		ssrc,			// 変換元文字列
        -1,				// 変換文字列バイト数
        wbuf,			// 変換後格納先
        ires			// 格納領域取得
		);
	wbuf[ires]  = 0;
    if (ires == 0)
    {
    	return 0;
    }

    // UTF8への変換
	//// 事前チェック
	ires = WideCharToMultiByte(destcp, // 変換後文字コード
		0,				// フラグなし
		wbuf,			// 変換元文字列
        -1,				// 変換文字列バイト数
        NULL,			// 変換後格納先
        0,				// 格納領域
		NULL,NULL
		);
	if (ires == 0)
    {
		delete [] wbuf;
    	return 0;
    }
	// 容量チェック
    if (idstsz == 0)
    {
    	delete [] wbuf;
        return ires+1;
    }

    //// 領域確保
    putf = new char[ires+1];
    if (putf == NULL)
    {
		delete [] wbuf;
		return -1;
    }
    //// 実変換
	ires = WideCharToMultiByte(destcp, // 変換後文字コード
		0,				// フラグなし
		wbuf,			// 変換元文字列
        -1,				// 変換文字列バイト数
        putf,			// 変換後格納先
        ires,			// 格納領域
		NULL,NULL
		);
	if (ires == 0)
    {
	    delete [] wbuf;
    	delete [] putf;
        return 0;
    }

    // コピー
	ires = ((idstsz-1) > ires) ? ires : idstsz-1;
    for (i=0 ; i<ires ; i++)
    {
		sdst[i] = putf[i];
    }
    sdst[i] = 0;

    delete [] wbuf;
   	delete [] putf;
	return i;
}
#else
#include <iconv.h>
int doconv(char *psrccd, char *pdestcd, const char *inbuf, char *outbuf, size_t bufsize)
{
   iconv_t cd;
   char *p_src = inbuf;
   char *p_dst = outbuf;
   size_t n_src = strlen(inbuf);
   size_t n_dst = bufsize;

   if( (cd = iconv_open(pdestcd, psrccd)) != (iconv_t)-1 )
   {
//printf("iconv_open success outbufsize=%d\n", n_dst);
     iconv(cd, &p_src, &n_src, &p_dst, &n_dst);
     *p_dst='\x0';
     iconv_close(cd);
   }
   return strlen(outbuf);
}
#endif

char *getCodepageToEncode(INT cp)
{
  switch(cp)
  {
    case 932: return("SHIFT_JIS"); break;
    case 50220: return("ISO-2022-JP"); break;
    case 51932: return("EUC-JP"); break;
    case 65000: return("UTF-7"); break;
    case 65001: return("UTF-8"); break;
    default: return(""); break;
  }
}
