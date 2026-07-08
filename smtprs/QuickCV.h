/*
 *    QuickCV.h
 *
 *    Copyright (C) 1992 by K.Kawakami
 *    All rights reserved.
 */

#if defined __TURBOC__ || defined LSI_C
#  define MSDOS
#endif
#if defined MSDOS
#  if !defined __TURBOC__ && !defined LSI_C
#    define MS_C
#  endif
#endif
#if defined __human68k__
#  define HUMAN
#endif
#if defined MSDOS || defined HUMAN
#  define MSDOS_HUMAN
#else
#  define UNIX
#endif
#if defined __TURBOC__
#  define CALLTYPE cdecl
#else
#  define CALLTYPE
#endif

#define MODE_SJIS 0
#define MODE_EUC 1
#define MODE_JIS 2
#define MODE_COPY 3
#define MODE_HELP 4
#define MODE_DHELP 5
#define INPUT_AUTO 0
#define INPUT_SJIS 1
#define INPUT_EUC 2
#define INPUT_JIS 3
#define INPUT_ASCII 4
#define INPUT_UNKNOWN 5
#define INPUT_BINARY 6
#define CHECK_OK 0
#define CHECK_BINARY 1
#define CHECK_SJIS 2
#define CHECK_EUC 3
#define CHECK_JIS 4
#define CHECK_ASCII 5
#define CHECK_UNKNOWN 6
#define CHECK_NOTHING 7
#define NEWLINE_NO 0
#define NEWLINE_CRLF 1
#define NEWLINE_LF 2
#define NEWLINE_CR 3
#define ESC 0x1b
#define CTRLZ 0x1a
#define SO 0x0e
#define SI 0x0f
#define NEWJIS_K "$B"
#define NEWJIS_A "(J"
#define NEWJIS2_K "$B"
#define NEWJIS2_A "(B"
#define OLDJIS_K "$@"
#define OLDJIS_A "(J"
#define NECJIS_K "K"
#define NECJIS_A "H"
#define MKTEMP_MAX 16
#define CHK_BIN_SIZE 1024
#define CHK_KANA_SIZE 1024

#ifdef MSDOS_HUMAN
#  define MODE_DEFAULT MODE_SJIS
#else
#  define MODE_DEFAULT MODE_EUC
#endif
#ifdef MSDOS
#  ifdef LSI_C
#    define IO_BUF_SIZE 8192
#  else
#    define IO_BUF_SIZE 32752
#  endif
#  define TMP_EXT ".$$$"
#else
#  define IO_BUF_SIZE 65536
#  define TMP_EXT ".tmp"
#  ifndef O_BINARY
#    define O_BINARY 0
#  endif
#endif

#define iskanji1st(c) ((c) >= 0x81 && (c) <= 0x9f ||\
                       (c) >= 0xe0 && (c) <= 0xfc)
#define iskanji2nd(c) ((c) >= 0x40 && (c) <= 0xfc && (c) != 0x7f)
#define iseuc(c) ((c) >= 0xa1 && (c) <= 0xfe)
#define isjis(c) ((c) >= 0x21 && (c) <= 0x7f)
#define iskana(c) ((c) >= 0xa0 && (c) <= 0xdf)

FILE *stin, *stout;
