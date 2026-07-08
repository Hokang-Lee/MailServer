//---------------------------------------------------------------------------
// UTF8(N)変換
//  char(WideChar)<-(unicode)->UTF8Nの変換関数
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
#ifdef WINDOWS
#include "utf8.h"
#else
#include "imap4.h"
#endif
//---------------------------------------------------------------------------
// ローカル関数
//---------------------------------------------------------------------------
#ifdef WINDOWS
static int doconv(const char * ssrc,char * sdst,int idstsz,int istoutf);
static int doconvcodepage(INT codepage, const char * ssrc,char * sdst,int idstsz,int istoutf);
#else
int doconv(char *psrccd, char *pdestcd, const char *inbuf, char *outbuf, size_t bufsize);
char *getCodepageToEncode(int cp);
#endif

//! NULL終端のコードページの文字列を、NULL終端のUTF8Nに変換する
int CODEPAGE2UTF8N(INT codepage, const char * ssrc,char * sdst,int idstsz)
{
#ifdef WINDOWS
	return doconvcodepage(codepage, ssrc,sdst,idstsz,1);
#else
	return doconv(getCodepageToEncode(codepage), "UTF-8", ssrc, sdst, idstsz);
#endif
}

//! NULL終端のコードページの文字列を、NULL終端のUTF8Nに変換する
int UTF8N2CODEPAGE(INT codepage, const char * ssrc,char * sdst,int idstsz)
{
#ifdef WINDOWS
	return doconvcodepage(codepage, ssrc,sdst,idstsz,0);
#else
	return doconv("UTF-8", getCodepageToEncode(codepage), ssrc, sdst, idstsz);
#endif
}

//---------------------------------------------------------------------------
// SJIS2UTF8N()
//! NULL終端のSJISの文字列を、NULL終端のUTF8Nに変換する
/*!
\param	ssrc	[i]		変換元のSJIS文字列ポインタ
\param	sdst	[i/o]	変換後のUTF8Nを返す先のポインタ
\param	idstsz	[i]		wdstの上限サイズ。この値-1byteまで記録できる。
						0を指定した場合、sdstは無視され、NULL終端込みの必要な容量が返る
\retval	1以上	変換後のバイト数
\retval	0		エラー
\retval	-1		作業メモリの確保失敗
*/
//---------------------------------------------------------------------------
int SJIS2UTF8N(const char * ssrc,char * sdst,int idstsz)
{
#ifdef WINDOWS
	return doconv(ssrc,sdst,idstsz,1);
#else
	return doconv("SHIFT_JIS", "UTF-8", ssrc, sdst, idstsz);
#endif

}

//---------------------------------------------------------------------------
// UTF8N2SJIS()
//! NULL終端のUTF8N文字列をSJISへ変換する
/*!
\param	ssrc	[i]		変換元のUTF8N文字列ポインタ
\param	sdst	[i/o]	変換後のSJISを返す先のポインタ
\param	idstsz	[i]		wdstの上限サイズ。この値-1byteまで記録できる
						0を指定した場合、sdstは無視され、NULL終端込みの必要な容量が返る
\retval	0		エラー
\retval	1以上	変換後のバイト数
\retval	-1		作業メモリの確保失敗
*/
//---------------------------------------------------------------------------
int UTF8N2SJIS(const char * ssrc,char * sdst,int idstsz)
{
#ifdef WINDOWS
	return doconv(ssrc,sdst,idstsz,0);
#else
	return doconv("UTF-8", "SHIFT_JIS", ssrc, sdst, idstsz);
#endif

}
#ifdef WINDOWS
//---------------------------------------------------------------------------
// doconv()
//! 実際の変換処理。
/*!
\param	ssrc	[i]		変換元の文字列ポインタ
\param	sdst	[i/o]	変換後の文字列を返す先のポインタ
\param	idstsz	[i]		wdstの上限サイズ。この値-1byteまで記録できる
\retval	0		エラー
\retval	1以上	変換後のバイト数
\retval	-1		作業メモリの確保失敗
*/
//---------------------------------------------------------------------------
int doconv(const char * ssrc,char * sdst,int idstsz,int istoutf)
{
	int i;
    int ires;
    LPWSTR wbuf;
    LPSTR  putf;

    // unicodeへ変換
	//// 事前チェック
	ires = MultiByteToWideChar(
		(istoutf!=0) ? CP_ACP : CP_UTF8,
        				// 文字コード
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
	ires = MultiByteToWideChar(
		(istoutf!=0) ? CP_ACP : CP_UTF8,
        				// 文字コード
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
	ires = WideCharToMultiByte(
		(istoutf==0) ? CP_ACP : CP_UTF8,
						// UTF8
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
	ires = WideCharToMultiByte(
		(istoutf==0) ? CP_ACP : CP_UTF8,
						// UTF8
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

//---------------------------------------------------------------------------
// doconvcodepage()
//! 実際の変換処理。
/*!
\param	codepage[i]		変換元の文字のページコード
\param	ssrc	[i]		変換元の文字列ポインタ
\param	sdst	[i/o]	変換後の文字列を返す先のポインタ
\param	idstsz	[i]		wdstの上限サイズ。この値-1byteまで記録できる
\retval	0		エラー
\retval	1以上	変換後のバイト数
\retval	-1		作業メモリの確保失敗
*/
//---------------------------------------------------------------------------
int doconvcodepage(INT codepage, const char * ssrc,char * sdst,int idstsz,int istoutf)
{
	int i;
    int ires;
    LPWSTR wbuf;
    LPSTR  putf;

    // unicodeへ変換
	//// 事前チェック
	ires = MultiByteToWideChar(
		(istoutf!=0) ? codepage : CP_UTF8,
        				// 文字コード
		0, //MB_COMPOSITE, //0,				// フラグなし
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
	ires = MultiByteToWideChar(
		(istoutf!=0) ? codepage : CP_UTF8,
        				// 文字コード
		0, //MB_COMPOSITE, //0,				// フラグなし
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
	ires = WideCharToMultiByte(
		(istoutf==0) ? codepage : CP_UTF8,
						// UTF8
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
	ires = WideCharToMultiByte(
		(istoutf==0) ? codepage : CP_UTF8,
						// UTF8
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
#endif
