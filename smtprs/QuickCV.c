/*
 *    CodeConvertor
 *
 *    Copyright (C) 1996 by K.Kawakami
 *    All rights reserved.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <io.h>
#include "QuickCV.h"

char *strdup2(char *s);
void allocerror();
int readbuf();
void writebuf();

char binarytbl[32] = {
    1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1
};

unsigned char rot13tbl[256] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
    0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
    0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27,
    0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
    0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
    0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,
    0x40, 0x4e, 0x4f, 0x50, 0x51, 0x52, 0x53, 0x54,
    0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0x41, 0x42,
    0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a,
    0x4b, 0x4c, 0x4d, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f,
    0x60, 0x6e, 0x6f, 0x70, 0x71, 0x72, 0x73, 0x74,
    0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0x61, 0x62,
    0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a,
    0x6b, 0x6c, 0x6d, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f,
    0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
    0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f,
    0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97,
    0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f,
    0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7,
    0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf,
    0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7,
    0xb8, 0xb9, 0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf,
    0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7,
    0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf,
    0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7,
    0xd8, 0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf,
    0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7,
    0xe8, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef,
    0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7,
    0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff
};

unsigned char rot47tbl[256] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
    0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
    0x20, 0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56,
    0x57, 0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e,
    0x5f, 0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66,
    0x67, 0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e,
    0x6f, 0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76,
    0x77, 0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7d, 0x7e,
    0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28,
    0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 0x30,
    0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38,
    0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f, 0x40,
    0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48,
    0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f, 0x7f,
    0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
    0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f,
    0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97,
    0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f,
    0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7,
    0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf,
    0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7,
    0xb8, 0xb9, 0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf,
    0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7,
    0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf,
    0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7,
    0xd8, 0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf,
    0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7,
    0xe8, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef,
    0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7,
    0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff
};

unsigned char kanji2nd;
int execpascalver = 1;
int convmode = MODE_DEFAULT;
int inputcode = INPUT_AUTO;
int newlineconvmode = NEWLINE_NO;
int forcenewlineconv = 0;
int outtostdout = 0;
int infromstdin = 0;
int _7bitmode = 0;
int removectrlz = 0;
int usetempfile = 0;
int quietmode = 0;
int rotencoding = 0;
int emulationmode = 0;
unsigned char esc_k[2] = {'$', 'B'};
unsigned char esc_a[2] = {'(', 'J'};

int infd, outfd;
int ineof;
unsigned char *inbuf, *outbuf, *newlinebuf;
unsigned char *inbufptr, *outbufptr;
unsigned char *inbufend, *outbufend;
int detectedcode, newlineconv, crflag;
static char mess [256];

#define get() (inbufptr >= inbufend && readbuf() ? EOF : *inbufptr++)
#define put(c) (outbufptr >= outbufend ?\
               (writebuf(), *outbufptr++ = (c)) : (*outbufptr++ = (c)))

/* check if s is option or not */
int isoption(char *s)
{
    if (emulationmode)
        return *s == '-';
    else
        return *s++ == '-' && *s != '-';
}

/* invalid option */
void invalidoption(int c)
{
    printf("CodeConvertor: Invalid command line option: \'%c\'", c);
    exit(1);
}

/* set escape sequence */
void setescseq(unsigned char *d,unsigned char *s)
{
    *d++ = *s++;
    *d = *s;
}

/* if argv[1] = '-nkf', qkc emulates nkf */
void checkemulationmode(char *s)
{
    if (s != NULL && !strcmp(s, "-nkf")) {
        emulationmode = outtostdout = quietmode = _7bitmode = 1;
        convmode = MODE_JIS;
        setescseq((unsigned char *)esc_k, (unsigned char *)NEWJIS2_K);
        setescseq((unsigned char *)esc_a, (unsigned char *)NEWJIS2_A);
    }
}

/* check option (nkf emulation mode) */
void checkoption_nkf(char *opt)
{
    static int skip = 1;

    if (skip) {
        skip = 0;
        return;
    }
    opt++;
    while (*opt)
        switch (*opt++) {
        case 'j':
        case 'n':
            convmode = MODE_JIS;
            break;
        case 'e':
        case 'a':
            convmode = MODE_EUC;
            break;
        case 's':
        case 'x':
            convmode = MODE_SJIS;
            break;
        case 't':
            convmode = MODE_COPY;
            break;
        case 'i':
            if (*opt)
                esc_k[1] = *opt++;
            break;
        case 'o':
            if (*opt)
                esc_a[1] = *opt++;
            break;
        case 'B':
            if (*opt == 'B') {
                opt++;
                esc_k[1] = esc_a[1] = 'B';
            }
            else if (*opt == 'J') {
                opt++;
                esc_k[1] = 'B';
                esc_a[1] = 'J';
            }
            break;
        case 'J':
            esc_k[1] = 'B';
            esc_a[1] = 'J';
            break;
        case 'r':
            rotencoding = 1;
            break;
/*
        case 'v':
            onlinehelp();
            exit(0);
*/
        }
}

/* check option */
void checkoption(char *opt)
{
    if (emulationmode) {
        checkoption_nkf(opt);
        return;
    }
    if (*++opt == '\0') {
        infromstdin = 1;
        return;
    }
    while (*opt)
        switch (*opt++) {
        case 's':
            convmode = MODE_SJIS;
            break;
        case 'e':
            convmode = MODE_EUC;
            break;
        case 'j':
            convmode = MODE_JIS;
            setescseq((unsigned char *)esc_k, (unsigned char *)NEWJIS_K);
            setescseq((unsigned char *)esc_a, (unsigned char *)NEWJIS_A);
            break;
        case 'b':
            if (*opt++ == 'j') {
                convmode = MODE_JIS;
                setescseq((unsigned char *)esc_k, (unsigned char *)NEWJIS2_K);
                setescseq((unsigned char *)esc_a, (unsigned char *)NEWJIS2_A);
            }
            else
                invalidoption('b');
            break;
        case 'o':
            if (*opt++ == 'j') {
                convmode = MODE_JIS;
                setescseq((unsigned char *)esc_k, (unsigned char *)OLDJIS_K);
                setescseq((unsigned char *)esc_a, (unsigned char *)OLDJIS_A);
            }
            else
                invalidoption('o');
            break;
        case 'n':
            if (*opt == 'j') {
                opt++;
                convmode = MODE_JIS;
                setescseq((unsigned char *)esc_k, (unsigned char *)NECJIS_K);
                setescseq((unsigned char *)esc_a, (unsigned char *)NECJIS_A);
            }
            else
                newlineconvmode = NEWLINE_NO;
            break;
        case 'O':
            if (*opt == '-') {
                opt++;
                outtostdout = 0;
            }
            else
                outtostdout = 1;
            break;
        case 'I':
            if (*opt == '-') {
                opt++;
                infromstdin = 0;
            }
            else
                infromstdin = 1;
            break;
        case 'm':
            if (*opt == 'a') {
                opt++;
                newlineconvmode = NEWLINE_CR;
            }
            else
                newlineconvmode = NEWLINE_CRLF;
            break;
        case 'u':
            newlineconvmode = NEWLINE_LF;
            break;
        case 'l':
            if (*opt == '-') {
                opt++;
                forcenewlineconv = 0;
            }
            else
                forcenewlineconv = 1;
            break;
        case '8':
            _7bitmode = 0;
            break;
        case '7':
            _7bitmode = 1;
            break;
        case 'z':
            if (*opt == '-') {
                opt++;
                removectrlz = 0;
            }
            else
                removectrlz = 1;
            break;
        case 'q':
            if (*opt == '-') {
                opt++;
                quietmode = 0;
            }
            else
                quietmode = 1;
            break;
        case 'i':
            switch (*opt++) {
            case 's':
                inputcode = INPUT_SJIS;
                break;
            case 'e':
                inputcode = INPUT_EUC;
                break;
            case 'j':
                inputcode = INPUT_JIS;
                break;
            case 'a':
                inputcode = INPUT_AUTO;
                break;
            default:
                invalidoption('i');
            }
            break;
        case 'h':
            convmode = MODE_HELP;
            break;
        case 'H':
            convmode = MODE_DHELP;
            break;
        case 't':
            if (*opt == '-') {
                opt++;
                usetempfile = 0;
            }
            else
                usetempfile = 1;
            break;
        case 'c':
            break;
#ifdef MSDOS
        case 'P':
            execpascalver = !execpascalver;
            break;
#endif
        default:
            invalidoption(*--opt);
        }
}

/* check environment option */
void checkenvoption()
{
    char *org, *s, *t, bak;

    if (emulationmode)
        return;
    if ((org = getenv("QKC")) == NULL)
        return;
    if ((s = strdup2(org)) == NULL)
        allocerror();
    while (1) {
        while (*s == ' ' || *s == '\t')
            s++;
        if (*s == '\0')
            break;
        t = s;
        while (*t != ' ' && *t != '\t' && *t != '\0')
            t++;
        bak = *t;
        *t = '\0';
        if (isoption(s))
            checkoption(s);
        if (bak == '\0')
            break;
        s = ++t;
    }
    free(s);
}

/* function version of get() */
int getf()
{
    return get();
}

/* function version of put() */
void putf(int c)
{
    put(c);
}

/* convert JIS code to Shift-JIS code */
#ifdef VC6
unsigned char jistojms(register unsigned char c1, register unsigned char c2)
#else
unsigned char jistojms(unsigned char c1, unsigned char c2)
#endif
{
    if (c1 & 1) {
        c1 = (c1 >> 1) + 0x71;
        c2 += 0x1f;
        if (c2 >= 0x7f)
            c2++;
    }
    else {
        c1 = (c1 >> 1) + 0x70;
        c2 += 0x7e;
    }
    if (c1 > 0x9f)
        c1 += 0x40;
    kanji2nd = c2;
    return c1;
}

/* convert Shift-JIS code to JIS code */
#ifdef VC6
unsigned char jmstojis(register unsigned char c1, register unsigned char c2)
#else
unsigned char jmstojis(unsigned char c1, unsigned char c2)
#endif
{
    c1 -= (c1 <= 0x9f) ? 0x70 : 0xb0;
    c1 <<= 1;
    if (c2 < 0x9f) {
        c2 -= (c2 < 0x7f) ? 0x1f : 0x20;
        c1--;
    }
    else
        c2 -= 0x7e;
    kanji2nd = c2;
    return c1;
}

/* output escape sequence for 1 byte character */
void set1byte()
{
    putf(ESC);
    putf(esc_a[0]);
    if (esc_a[1])
        putf(esc_a[1]);
}

/* output escape sequence for 2 byte character */
void set2byte()
{
    putf(ESC);
    putf(esc_k[0]);
    if (esc_k[1])
        putf(esc_k[1]);
}

/* at the end of JIS file, output escape sequence for 1 byte character */
void jisend(char s, char k)
{
    if (k)
        putf(SI);
    else if (s)
        set1byte();
}

/* copy */
void copy()
{
#ifdef VC6
    register int c;
#else
    int c;
#endif

    while ((c = get()) != EOF)
        put(c);
}

/* Shift-JIS to EUC */
void sjistoeuc()
{
#ifdef VC6
    register int c, c2;
    register char rot;
#else
    int c, c2;
    char rot;
#endif
    rot = rotencoding;
    if ((c = getf()) == EOF)
        return;
    while (1) {
        if (c < 0x80) {
            if (rot)
                c = rot13tbl[c];
            putf(c);
        }
        else if (iskanji1st(c)) {
            if ((c2 = get()) == EOF) {
                putf(c);
                return;
            }
            if (iskanji2nd(c2)) {
#ifdef VC6
                (unsigned char)c = jmstojis((register unsigned char)c, (register unsigned char)c2);
#else
                (unsigned char)c = jmstojis((unsigned char)c, (unsigned char)c2);
#endif
                if (rot) {
                    c = rot47tbl[c];
                    kanji2nd = rot47tbl[kanji2nd];
                }
                put(c | 0x80);
                put(kanji2nd | 0x80);
            }
            else {
                putf(c);
                putf(c2);
            }
        }
        else if (iskana(c)) {
            putf(0x8e);
            putf(c);
        }
        else
            putf(c);
        if ((c = get()) == EOF)
            return;
    }
}

/* EUC to Shift-JIS */
void euctosjis()
{
#ifdef VC6
    register int c, c2;
    register char rot;
#else
    int c, c2;
    char rot;
#endif

    rot = rotencoding;
    if ((c = getf()) == EOF)
        return;
    while (1) {
        if (c < 0x80) {
            if (rot)
                c = rot13tbl[c];
            put(c);
        }
        else if (iseuc(c)) {
            if ((c2 = get()) == EOF) {
                putf(c);
                return;
            }
            if (iseuc(c2)) {
                if (rot) {
                    c = rot47tbl[c & 0x7f];
                    c2 = rot47tbl[c2 & 0x7f];
                }
#ifdef VC6
                (unsigned char)c = jistojms((register unsigned char)(c & 0x7f), (register unsigned char)(c2 & 0x7f));
#else
                (unsigned char)c = jistojms((unsigned char)(c & 0x7f), (unsigned char)(c2 & 0x7f));
#endif
                put(c);
                put(kanji2nd);
            }
            else {
                putf(c);
                putf(c2);
            }
        }
        else if (c == 0x8e) {
            if ((c = getf()) == EOF)
                return;
            putf(c);
        }
        else if (c == 0x8f) {
            if ((c = getf()) == EOF)
                return;
            putf(c);
            if ((c = getf()) == EOF)
                return;
            putf(c);
        }
        else
            putf(c);
        if ((c = get()) == EOF)
            return;
    }
}

/* Shift-JIS to JIS */
void sjistojis()
{
#ifdef VC6
    register int c, c2;
    register char rot, state, kana;
#else
    int c, c2;
    char rot, state, kana;
#endif
    char _7bit;

    rot = rotencoding;
    _7bit = _7bitmode;
    state = kana = 0;
    if ((c = getf()) == EOF)
        return;
    while (1) {
        if (c < 0x80) {
            if (state) {
                set1byte();
                state = 0;
            }
            else if (kana) {
                putf(SI);
                kana = 0;
            }
            if (rot)
                c = rot13tbl[c];
            put(c);
        }
        else if (iskanji1st(c)) {
            if ((c2 = get()) == EOF) {
                jisend(state, kana);
                putf(c);
                return;
            }
            if (!state) {
                if (kana) {
                    putf(SI);
                    kana = 0;
                }
                set2byte();
                state = 1;
            }
            if (iskanji2nd(c2)) {
#ifdef VC6
                (unsigned char)c = jmstojis((register unsigned char)c, (register unsigned char)c2);
#else
                (unsigned char)c = jmstojis((unsigned char)c, (unsigned char)c2);
#endif
                if (rot) {
                    c = rot47tbl[c];
                    kanji2nd = rot47tbl[kanji2nd];
                }
                put(c);
                put(kanji2nd);
            }
            else {
                putf(c);
                putf(c2);
            }
        }
        else if (iskana(c)) {
            if (state) {
                set1byte();
                state = 0;
            }
            if (_7bit) {
                if (!kana) {
                    putf(SO);
                    kana = 1;
                }
                c &= 0x7f;
            }
            putf(c);
        }
        else {
            if (state) {
                set1byte();
                state = 0;
            }
            else if (kana) {
                putf(SI);
                kana = 0;
            }
            putf(c);
        }
        if ((c = get()) == EOF) {
            jisend(state, kana);
            return;
        }
    }
}

/* JIS to Shift-JIS */
void jistosjis()
{
#ifdef VC6
    register int c, c2;
    register char rot, state, kana;
#else
    int c, c2;
    char rot, state, kana;
#endif
    char _7bit;

    rot = rotencoding;
    _7bit = _7bitmode;
    state = kana = 0;
    if ((c = getf()) == EOF)
        return;
    while (1) {
        if (c == ESC) {
            if ((c = getf()) == EOF) {
                putf(ESC);
                return;
            }
            switch (c) {
            case '$':
                if ((c2 = getf()) == EOF) {
                    putf(ESC);
                    putf(c);
                    return;
                }
                if (c2 == 'B' || c2 == '@')
                    state = 1;
                else {
                    putf(ESC);
                    putf(c);
                    c = c2;
                    continue;
                }
                break;
            case '(':
                if ((c2 = getf()) == EOF) {
                    putf(ESC);
                    putf(c);
                    return;
                }
                if (c2 == 'J' || c2 == 'B' || c2 == 'H')
                    state = 0;
                else {
                    putf(ESC);
                    putf(c);
                    c = c2;
                    continue;
                }
                break;
            case 'K':
                state = 1;
                break;
            case 'H':
                state = 0;
                break;
            default:
                putf(ESC);
                continue;
            }
        }
        else if (c <= 0x20 || c == 0x7f) {
            if (_7bit && (c == SO || c == SI))
                kana = c == SO;
            else
                putf(c);
        }
        else if (state) {
            if ((c2 = get()) == EOF) {
                putf(c);
                return;
            }
            if (c2 <= 0x20) {
                putf(c);
                c = c2;
                continue;
            }
            if (c < 0x80 && isjis(c2)) {
                if (rot) {
                    c = rot47tbl[c];
                    c2 = rot47tbl[c2];
                }
#ifdef VC6
                (unsigned char)c = jistojms((register unsigned char)c, (register unsigned char)c2);
#else
                (unsigned char)c = jistojms((unsigned char)c, (unsigned char)c2);
#endif
                put(c);
                put(kanji2nd);
            }
            else {
                putf(c);
                putf(c2);
            }
        }
        else {
            if (_7bit && kana)
                c |= 0x80;
            else if (rot)
                c = rot13tbl[c];
            put(c);
        }
        if ((c = get()) == EOF)
            return;
    }
}

/* EUC to JIS */
void euctojis()
{
#ifdef VC6
    register int c, c2;
    register char rot, state, kana;
#else
    int c, c2;
    char rot, state, kana;
#endif
    char _7bit;

    rot = rotencoding;
    _7bit = _7bitmode;
    state = kana = 0;
    if ((c = getf()) == EOF)
        return;
    while (1) {
        if (c < 0x80) {
            if (state) {
                set1byte();
                state = 0;
            }
            else if (kana) {
                putf(SI);
                kana = 0;
            }
            if (rot)
                c = rot13tbl[c];
            put(c);
        }
        else if (iseuc(c)) {
            if ((c2 = get()) == EOF) {
                jisend(state, kana);
                putf(c);
                return;
            }
            if (!state) {
                if (kana) {
                    putf(SI);
                    kana = 0;
                }
                set2byte();
                state = 1;
            }
            if (iseuc(c2)) {
                if (rot) {
                    c = rot47tbl[c & 0x7f];
                    c2 = rot47tbl[c2 & 0x7f];
                }
                put(c & 0x7f);
                put(c2 & 0x7f);
            }
            else {
                putf(c);
                putf(c2);
            }
        }
        else if (c == 0x8e) {
            if ((c = getf()) == EOF) {
                jisend(state, kana);
                return;
            }
            if (state) {
                set1byte();
                state = 0;
            }
            if (_7bit) {
                if (!kana) {
                    putf(SO);
                    kana = 1;
                }
                c &= 0x7f;
            }
            putf(c);
        }
        else if (c == 0x8f) {
            if ((c = getf()) == EOF) {
                jisend(state, kana);
                return;
            }
            if ((c2 = getf()) == EOF) {
                jisend(state, kana);
                putf(c);
                return;
            }
            if (!state) {
                if (kana) {
                    putf(SI);
                    kana = 0;
                }
                set2byte();
                state = 1;
            }
            putf(c);
            putf(c2);
        }
        else {
            if (state) {
                set1byte();
                state = 0;
            }
            else if (kana) {
                putf(SI);
                kana = 0;
            }
            putf(c);
        }
        if ((c = get()) == EOF) {
            jisend(state, kana);
            return;
        }
    }
}

/* JIS to EUC */
void jistoeuc()
{
#ifdef VC6
    register int c, c2;
    register char rot, state, kana;
#else
    int c, c2;
    char rot, state, kana;
#endif
    char _7bit;

    rot = rotencoding;
    _7bit = _7bitmode;
    state = kana = 0;
    if ((c = getf()) == EOF)
        return;
    while (1) {
        if (c == ESC) {
            if ((c = getf()) == EOF) {
                putf(ESC);
                return;
            }
            switch (c) {
            case '$':
                if ((c2 = getf()) == EOF) {
                    putf(ESC);
                    putf(c);
                    return;
                }
                if (c2 == 'B' || c2 == '@')
                    state = 1;
                else {
                    putf(ESC);
                    putf(c);
                    c = c2;
                    continue;
                }
                break;
            case '(':
                if ((c2 = getf()) == EOF) {
                    putf(ESC);
                    putf(c);
                    return;
                }
                if (c2 == 'J' || c2 == 'B' || c2 == 'H')
                    state = 0;
                else {
                    putf(ESC);
                    putf(c);
                    c = c2;
                    continue;
                }
                break;
            case 'K':
                state = 1;
                break;
            case 'H':
                state = 0;
                break;
            default:
                putf(ESC);
                continue;
            }
        }
        else if (c <= 0x20 || c == 0x7f) {
            if (_7bit && (c == SO || c == SI))
                kana = c == SO;
            else
                putf(c);
        }
        else if (state) {
            if ((c2 = get()) == EOF) {
                putf(c);
                return;
            }
            if (c2 <= 0x20 || c2 == 0x7f) {
                putf(c);
                c = c2;
                continue;
            }
            if (c < 0x80 && isjis(c2) && rot) {
                c = rot47tbl[c];
                c2 = rot47tbl[c2];
            }
            put(c | 0x80);
            put(c2 | 0x80);
        }
        else {
            if (_7bit && kana)
                c |= 0x80;
            else if (rot)
                c = rot13tbl[c];
            if (iskana(c))
                putf(0x8e);
            put(c);
        }
        if ((c = get()) == EOF)
            return;
    }
}

/* Shift-JIS to Shift-JIS */
void sjistosjis()
{
#ifdef VC6
    register int c, c2;
#else
    int c, c2;
#endif

    if (!rotencoding) {
        copy();
        return;
    }
    if ((c = getf()) == EOF)
        return;
    while (1) {
        if (iskanji1st(c)) {
            if ((c2 = get()) == EOF) {
                putf(c);
                return;
            }
            if (iskanji2nd(c2)) {
#ifdef VC6
                (unsigned char)c = jmstojis((register unsigned char)c, (register unsigned char)c2);
                (unsigned char)c = jistojms((register unsigned char)rot47tbl[c], (register unsigned char)rot47tbl[kanji2nd]);
#else
                (unsigned char)c = jmstojis((unsigned char)c, (unsigned char)c2);
                (unsigned char)c = jistojms((unsigned char)rot47tbl[c], (unsigned char)rot47tbl[kanji2nd]);
#endif
                put(c);
                put(kanji2nd);
            }
            else {
                putf(c);
                putf(c2);
            }
        }
        else {
            c = rot13tbl[c];
            put(c);
        }
        if ((c = get()) == EOF)
            return;
    }
}

/* EUC to EUC */
void euctoeuc()
{
#ifdef VC6
    register int c, c2;
#else
    int c, c2;
#endif

    if (!rotencoding) {
        copy();
        return;
    }
    if ((c = getf()) == EOF)
        return;
    while (1) {
        if (c < 0x80) {
            c = rot13tbl[c];
            put(c);
        }
        else if (iseuc(c)) {
            if ((c2 = get()) == EOF) {
                putf(c);
                return;
            }
            if (iseuc(c2)) {
                c = rot47tbl[c & 0x7f] | 0x80;
                c2 = rot47tbl[c2 & 0x7f] | 0x80;
            }
            put(c);
            put(c2);
        }
        else if (c == 0x8e || c == 0x8f) {
            putf(c);
            c2 = c;
            if ((c = getf()) == EOF)
                return;
            putf(c);
            if ((c = getf()) == EOF)
                return;
            if (c2 == 0x8e)
                continue;
            putf(c);
        }
        else
            putf(c);
        if ((c = get()) == EOF)
            return;
    }
}

/* JIS to JIS */
void jistojis()
{
#ifdef VC6
    register int c, c2;
    register char i_state, o_state;
#else
    int c, c2;
    char i_state, o_state;
#endif
    char rot, _7bit, i_kana, o_kana;

    rot = rotencoding;
    _7bit = _7bitmode;
    i_state = o_state = i_kana = o_kana = 0;
    if ((c = getf()) == EOF)
        return;
    while (1) {
        if (c == ESC) {
            if ((c = getf()) == EOF) {
                jisend(o_state, o_kana);
                putf(ESC);
                return;
            }
            switch (c) {
            case '$':
                if ((c2 = getf()) == EOF) {
                    jisend(o_state, o_kana);
                    putf(ESC);
                    putf(c);
                    return;
                }
                if (c2 == 'B' || c2 == '@')
                    i_state = 1;
                else {
                    if (o_state) {
                        set1byte();
                        o_state = 0;
                    }
                    putf(ESC);
                    putf(c);
                    c = c2;
                    continue;
                }
                break;
            case '(':
                if ((c2 = getf()) == EOF) {
                    jisend(o_state, o_kana);
                    putf(ESC);
                    putf(c);
                    return;
                }
                if (c2 == 'J' || c2 == 'B' || c2 == 'H')
                    i_state = 0;
                else {
                    if (o_state) {
                        set1byte();
                        o_state = 0;
                    }
                    putf(ESC);
                    putf(c);
                    c = c2;
                    continue;
                }
                break;
            case 'K':
                i_state = 1;
                break;
            case 'H':
                i_state = 0;
                break;
            default:
                if (o_state) {
                    set1byte();
                    o_state = 0;
                }
                putf(ESC);
                continue;
            }
        }
        else if (c <= 0x20 || c == 0x7f)
            switch (c) {
            case SO:
                i_kana = 1;
                break;
            case SI:
                i_kana = 0;
                break;
            default:
                if (o_state) {
                    set1byte();
                    o_state = 0;
                }
                else if (o_kana) {
                    putf(SI);
                    o_kana = 0;
                }
                putf(c);
            }
        else if (i_state) {
            if ((c2 = get()) == EOF) {
                jisend(o_state, o_kana);
                putf(c);
                return;
            }
            if (c2 <= 0x20 || c2 == 0x7f) {
                if (o_state) {
                    set1byte();
                    o_state = 0;
                }
                putf(c);
                c = c2;
                continue;
            }
            if (!o_state) {
                if (o_kana) {
                    putf(SI);
                    o_kana = 0;
                }
                set2byte();
                o_state = 1;
            }
            if (c < 0x80 && isjis(c2) && rot) {
                c = rot47tbl[c];
                c2 = rot47tbl[c2];
            }
            put(c);
            put(c2);
        }
        else {
            if (i_kana)
                c |= 0x80;
            if (o_state) {
                set1byte();
                o_state = 0;
            }
            if (iskana(c) && _7bit) {
                if (!o_kana) {
                    putf(SO);
                    o_kana = 1;
                }
                c &= 0x7f;
            }
            else {
                if (o_kana) {
                    putf(SI);
                    o_kana = 0;
                }
                if (rot)
                    c = rot13tbl[c];
            }
            put(c);
        }
        if ((c = get()) == EOF)
            return;
    }
}

/* call perror then exit(1) */
void runerror()
{
    perror("qkc");
    exit(1);
}

/* memory allocation error */
void allocerror()
{
    printf("CodeConvertor: Memory allocation error");
    exit(1);
}

/* no files found */
void filenotfound()
{
    printf("CodeConvertor: File not found or permission denied");
    exit(1);
}

/* out of memory */
void outofmemory()
{
    printf("CodeConvertor: Out of memory");
    exit(1);
}

/* write error */
void writeerror()
{
    printf("CodeConvertor: Write error");
    exit(1);
}

/* if no files specified, input from stdin */
void fromstdin(int c)
{
    if (c == 0)
        infromstdin = 1;
    if (infromstdin)
        outtostdout = quietmode = 1;
}

/* if stdout is redirected to a file, output to stdout */
void tostdout()
{
//    if (!emulationmode)
    if (!emulationmode && fileno(stdout) > 0)
        outtostdout = !isatty(fileno(stdout));
}

/* get memory for input and output buffer */
void getmemory()
{
    if ((inbuf = (unsigned char *) malloc(IO_BUF_SIZE)) == NULL)
        outofmemory();
    if ((outbuf = (unsigned char *) malloc(IO_BUF_SIZE)) == NULL)
        outofmemory();
    outbufptr = outbuf;
    outbufend = outbuf + IO_BUF_SIZE;
    if (newlineconvmode != NEWLINE_NO)
        if ((newlinebuf = (unsigned char *) malloc(IO_BUF_SIZE)) == NULL)
            outofmemory();
}

/* duplicate string */
char *strdup2(char *s)
{
    char *p;

    p = (char *) malloc(strlen(s) + 1);
    if (p)
        strcpy(p, s);
    return p;
}

/* split path into directory and filename */
void pathsplit(char *path, char **dir, char **name)
{
    char *s, *p;

    if ((p = strdup2(path)) == NULL)
        allocerror();
    s = p;
    while (*p)
        p++;
    while (--p >= s)
#ifdef MSDOS_HUMAN
        if (*p == '\\' || *p == ':' || *p == '/')
#else
        if (*p == '/')
#endif
            break;
    if ((*name = strdup2(++p)) == NULL)
        allocerror();
    *p = '\0';
    *dir = s;
}

/* make temporary filename (length(base) <= 3, length(ext) <= 4) */
char *maketemp(char *base, char *ext, char *dir)
{
    int i;
    char *tempname;
    static int done = 0;

    if (!done) {
        srand((unsigned int) time(NULL));
        done = 1;
    }
    if ((tempname = (char *) malloc(strlen(dir) + 13)) == NULL)
        allocerror();
    for (i = 0; i < MKTEMP_MAX; i++) {
        sprintf(tempname, "%s%s%05ld%s",
                dir, base, (long) rand() % 100000, ext);
        if (access(tempname, 00))
            return tempname;
    }
    free(tempname);
    return NULL;
}

/* close input file */
void closeinfile()
{
    if (!infromstdin && close(infd))
        runerror();
}

/* open temporary file */
char *opentempfile(char *dir)
{
    char *tempname;

    if ((tempname = maketemp("qkc", TMP_EXT, dir)) == NULL) {
        closeinfile();
        printf("CodeConvertor: Cannot open temporary file");
        exit(1);
    }
    if ((outfd = open(tempname, O_RDWR | O_CREAT | O_BINARY, 0600)) == -1) {
        free(tempname);
        return NULL;
    }
    return tempname;
}

/* convert newline code to CR LF */
unsigned int convertnewline1(unsigned char *buf, unsigned int size)
{
#ifdef VC6
    register unsigned char *s, *t, c;
    register unsigned int i;
#else
    unsigned char *s, *t, c;
    unsigned int i;
#endif

    if (size == 0)
        return 0;
    s = buf;
    t = newlinebuf;
    if (crflag && *s == '\n') {
        crflag = 0;
        s++;
        size--;
    }
    for (i = 0; i < size; i++)
        switch (c = *s++) {
        case '\n':
            *t++ = '\r';
            *t++ = '\n';
            break;
        case '\r':
            *t++ = '\r';
            *t++ = '\n';
            if (i < size - 1 && *s == '\n') {
                s++;
                i++;
            }
            else
                crflag = 1;
            break;
        default:
            *t++ = c;
        }
    return t - newlinebuf;
}

/* convert newline code to LF or CR */
unsigned int convertnewline2(unsigned char *buf, unsigned int size, unsigned char code)
{
#ifdef VC6
    register unsigned char *s, *t, c;
    register unsigned int i;
#else
    unsigned char *s, *t, c;
    unsigned int i;
#endif

    if (size == 0)
        return 0;
    s = buf;
    t = newlinebuf;
    if (crflag && *s == '\n') {
        crflag = 0;
        s++;
        size--;
    }
    for (i = 0; i < size; i++) {
        c = *s++;
        switch (c) {
        case '\n':
            *t++ = code;
            break;
        case '\r':
            *t++ = code;
            if (i < size - 1 && *s == '\n') {
                s++;
                i++;
            }
            else
                crflag = 1;
            break;
        default:
            *t++ = c;
        }
    }
    return t - newlinebuf;
}

/* read into input buffer from input file */
int readbuf()
{
    unsigned int size;
    int result;
    unsigned char *end;

    if (ineof)
        return 1;
    inbufptr = inbufend = inbuf;
    end = inbuf + IO_BUF_SIZE;
    while (inbufend < end) {
        size = end - inbufend;
        if (infd > 0)
          result = read(infd, inbufend, size);
        else
          result = 0;
        if (result == -1) {
            printf("CodeConvertor: Read error\n");
            exit(1);
        }
        else if (result == 0) {
            ineof = 1;
            return inbufend == inbuf;
        }
        inbufend += result;
    }
    return 0;
}

/* write from output buffer to output file */
void writebuf()
{
    unsigned int size, sizetemp;
    unsigned char *buf;

    size = outbufptr - outbuf;
    if (newlineconv == NEWLINE_CRLF) {
        if (size > IO_BUF_SIZE / 2) {
            sizetemp = convertnewline1(outbuf, IO_BUF_SIZE / 2);
            if (write(outfd, newlinebuf, sizetemp) < (int)sizetemp)
                if (!outtostdout || !isatty(outfd))
                    writeerror();
            buf = outbuf + IO_BUF_SIZE / 2;
            size -= IO_BUF_SIZE / 2;
        }
        else
            buf = outbuf;
        size = convertnewline1(buf, size);
        buf = newlinebuf;
    }
    else if (newlineconv != NEWLINE_NO) {
        (unsigned int )size = convertnewline2((unsigned char *)outbuf, (unsigned int)size,
                               (unsigned char)(newlineconv == NEWLINE_LF ? '\n' : '\r'));
        buf = newlinebuf;
    }
    else
        buf = outbuf;
    if (write(outfd, buf, size) < (int)size)
        if (!outtostdout || !isatty(outfd))
            writeerror();
    outbufptr = outbuf;
}

/* truncate ctrl-Z */
void truncatectrlz()
{
    long pos;
    unsigned char buf;

#ifdef UNIX
    if (outtostdout)
        return;
#endif
    if ((pos = lseek(outfd, 0L, 2)) == -1 || pos == 0)
        return;
    if ((pos = lseek(outfd, -1L, 1)) == -1
        || read(outfd, &buf, sizeof buf) == -1)
        runerror();
    if (buf == CTRLZ)
    if (chsize(outfd, pos))
        runerror();
    if (lseek(outfd, 0L, 2) == -1)
        runerror();
}

/* close output file */
void closeoutfile()
{
    writebuf();
    if (removectrlz)
        truncatectrlz();
    if (!outtostdout && close(outfd))
        runerror();
}

/* check if buf's code is binary */
#ifdef VC6
int checkbinary(register unsigned char *buf, register unsigned int size)
#else
int checkbinary(unsigned char *buf, unsigned int size)
#endif
{
#ifdef VC6
    register int i, count;
#else
    int i, count;
#endif

    if (size > CHK_BIN_SIZE)
        size = CHK_BIN_SIZE;
    count = 0;
    for (i = 0; i < (int)size; i++) {
        if (*buf < 0x20 && binarytbl[*buf])
            count++;
        buf++;
    }
    return count << 5 > (int)size;
}

/* detect newline code */
#ifdef VC6
int detectnewlinecode(register unsigned char *buf, register unsigned int size)
#else
int detectnewlinecode(unsigned char *buf, unsigned int size)
#endif
{
#ifdef VC6
    register unsigned char *end;
#else
    unsigned char *end;
#endif

    end = buf + size;
    while (buf < end)
        switch (*buf) {
        case '\n':
            return NEWLINE_LF;
        case '\r':
            if (++buf == end)
                return NEWLINE_NO;
            else if (*buf == '\n')
                return NEWLINE_CRLF;
            else
                return NEWLINE_CR;
        default:
            buf++;
        }
    return NEWLINE_NO;
}

/* scan forward if Shift-JIS code or JIS code exists */
int checkforward(unsigned char *p, unsigned int size)
{
    unsigned char c;

    if (size > CHK_KANA_SIZE)
        size = CHK_KANA_SIZE;
    if (size == 0)
        return INPUT_EUC;
    c = *p++;
    while (1) {
        if (c == ESC) {
            if (--size == 0)
                break;
            if ((c = *p++) == '$') {
                if (--size == 0)
                    break;
                if ((c = *p++) == 'B' || c == '@')
                    return INPUT_JIS;
                else
                    continue;
            }
            else if (c == 'K')
                return INPUT_JIS;
            else
                continue;
        }
        else if (c >= 0x81) {
            if (c == 0x8e) {
                if (--size == 0)
                    break;
                p++;
            }
            else if (c <= 0x9f) {
                if (--size == 0)
                    break;
                c = *p++;
                if (iskanji2nd(c))
                    return INPUT_SJIS;
                else
                    continue;
            }
            else if (c >= 0xa1 && c <= 0xdf) {
                if (--size == 0)
                    break;
                c = *p++;
                if (iskana(c))
                    continue;
                else if (iseuc(c))
                    return INPUT_EUC;
                else
                    continue;
            }
            else if (c != 0xa0)
                return INPUT_EUC;
        }
        if (--size == 0)
            break;
        c = *p++;
    }
    return INPUT_EUC;
}

/* detect kanji code */
int detectcode(unsigned char *buf,unsigned int size)
{
#ifdef VC6
    register unsigned char *p, c;
    register unsigned int count;
#else
    unsigned char *p, c;
    unsigned int count;
#endif

    int unknownstat;

    count = size;
    if (count == 0)
        return INPUT_ASCII;
    p = buf;
    unknownstat = 0;
    c = *p++;
    while (1) {
        if (c == ESC) {
            if (--count == 0)
                break;
            if ((c = *p++) == '$') {
                if (--count == 0)
                    break;
                if ((c = *p++) == 'B' || c == '@')
                    return INPUT_JIS;
                else
                    continue;
            }
            else if (c == 'K')
                return INPUT_JIS;
            else
                continue;
        }
        else if (c >= 0x81) {
            if (c == 0x8e) {
                if (--count == 0)
                    break;
                c = *p++;
                if (iskana(c))
                    unknownstat |= 2;
                else if (iskanji2nd(c))
                    return INPUT_SJIS;
                else
                    continue;
            }
            else if (c <= 0x9f) {
                if (--count == 0)
                    break;
                c = *p++;
                if (iskanji2nd(c))
                    return INPUT_SJIS;
                else
                    continue;
            }
            else if (c >= 0xa1 && c <= 0xdf || c == 0xfd || c == 0xfe) {
                if (--count == 0)
                    break;
                c = *p++;
                if (iseuc(c))
                    if (iskana(c))
                        return checkforward(p, count - 1);
                    else
                        return INPUT_EUC;
                else
                    continue;
            }
            else if (c >= 0xe0 && c <= 0xfc) {
                if (--count == 0)
                    break;
                c = *p++;
                if (iskanji2nd(c))
                    if (iseuc(c))
                        unknownstat |= 1;
                    else
                        return INPUT_SJIS;
                else
                    if (iseuc(c))
                        return INPUT_EUC;
                    else
                        continue;
            }
        }
        if (--count == 0)
            break;
        c = *p++;
    }
    switch (unknownstat) {
    case 1:
    case 3:
        if (!outtostdout)
            return INPUT_UNKNOWN;
    case 2:
        return INPUT_SJIS;
    default:
        return INPUT_ASCII;
    }
}

/* check if input file needs conversion */
int checkfile()
{
    int binary, result, same;
    unsigned int bufsize;

    ineof = 0;
    readbuf();
    bufsize = inbufend - inbuf;
    binary = 0;
    if (inputcode == INPUT_AUTO) {
        binary = checkbinary(inbuf, bufsize);
        if (binary && !outtostdout)
            return CHECK_BINARY;
    }
    if (!forcenewlineconv)
        if (!binary && newlineconvmode != NEWLINE_NO) {
            result = detectnewlinecode(inbuf, bufsize);
            if (result == NEWLINE_NO || result == newlineconvmode)
                newlineconv = NEWLINE_NO;
            else
                newlineconv = newlineconvmode;
        }
        else
            newlineconv = NEWLINE_NO;
    else
        newlineconv = newlineconvmode;
    if (binary)
        detectedcode = INPUT_BINARY;
    else if (inputcode == INPUT_AUTO)
        detectedcode = detectcode(inbuf, bufsize);
    else
        detectedcode = inputcode;
    same = detectedcode == INPUT_SJIS && convmode == MODE_SJIS
           || detectedcode == INPUT_EUC && convmode == MODE_EUC
           || detectedcode == INPUT_JIS && convmode == MODE_JIS
           || convmode == MODE_COPY;
    if ((detectedcode == INPUT_ASCII || detectedcode == INPUT_UNKNOWN || same)
        && !outtostdout && newlineconv == NEWLINE_NO)
        if (inputcode == INPUT_AUTO)
            switch (detectedcode) {
            case INPUT_SJIS:
                return CHECK_SJIS;
            case INPUT_EUC:
                return CHECK_EUC;
            case INPUT_JIS:
                return CHECK_JIS;
            case INPUT_ASCII:
                return CHECK_ASCII;
            case INPUT_UNKNOWN:
                return CHECK_UNKNOWN;
            default:
                return CHECK_NOTHING;
            }
        else if (same && convmode != MODE_JIS)
            return CHECK_NOTHING;
    return CHECK_OK;
}

int checkfilebuff(char *token)
{
    int binary, result, same;
    unsigned int bufsize;

    ineof = 0;

    ///////////////////////////
    //readbuf();
    strcpy((char *)inbuf, token);
    inbufptr = inbufend = inbuf;
    inbufend += strlen(token);  
    ///////////////////////////
    bufsize = strlen(token); //inbufend - inbuf;
    binary = 0;
    if (inputcode == INPUT_AUTO) {
        binary = checkbinary(inbuf, bufsize);
        if (binary && !outtostdout)
            return CHECK_BINARY;
    }
// printf("checkfile():inbuf=[%s]:bufsize=[%d]<br>\n",inbuf,bufsize);
    if (!forcenewlineconv)
        if (!binary && newlineconvmode != NEWLINE_NO) {
            result = detectnewlinecode(inbuf, bufsize);
            if (result == NEWLINE_NO || result == newlineconvmode)
                newlineconv = NEWLINE_NO;
            else
                newlineconv = newlineconvmode;
        }
        else
            newlineconv = NEWLINE_NO;
    else
        newlineconv = newlineconvmode;
    if (binary)
        detectedcode = INPUT_BINARY;
    else if (inputcode == INPUT_AUTO)
        detectedcode = detectcode(inbuf, bufsize);
    else
        detectedcode = inputcode;
    same = detectedcode == INPUT_SJIS && convmode == MODE_SJIS
           || detectedcode == INPUT_EUC && convmode == MODE_EUC
           || detectedcode == INPUT_JIS && convmode == MODE_JIS
           || convmode == MODE_COPY;
    if ((detectedcode == INPUT_ASCII || detectedcode == INPUT_UNKNOWN || same)
        && !outtostdout && newlineconv == NEWLINE_NO)
        if (inputcode == INPUT_AUTO)
            switch (detectedcode) {
            case INPUT_SJIS:
                return CHECK_SJIS;
            case INPUT_EUC:
                return CHECK_EUC;
            case INPUT_JIS:
                return CHECK_JIS;
            case INPUT_ASCII:
                return CHECK_ASCII;
            case INPUT_UNKNOWN:
                return CHECK_UNKNOWN;
            default:
                return CHECK_NOTHING;
            }
        else if (same && convmode != MODE_JIS)
            return CHECK_NOTHING;
    return CHECK_OK;
}

/* convert input file's kanji code, then output */
void conv()
{
    crflag = 0;
    if (convmode == MODE_COPY || detectedcode == INPUT_BINARY
        || detectedcode == INPUT_ASCII || detectedcode == INPUT_UNKNOWN)
        copy();
    else if (convmode == MODE_SJIS)
        switch (detectedcode) {
        case INPUT_SJIS:
            sjistosjis();
            break;
        case INPUT_EUC:
            euctosjis();
            break;
        case INPUT_JIS:
            jistosjis();
            break;
        default:
            copy();
        }
    else if (convmode == MODE_EUC)
        switch (detectedcode) {
        case INPUT_SJIS:
            sjistoeuc();
            break;
        case INPUT_EUC:
            euctoeuc();
            break;
        case INPUT_JIS:
            jistoeuc();
            break;
        default:
            copy();
        }
    else if (convmode == MODE_JIS)
        switch (detectedcode) {
        case INPUT_SJIS:
            sjistojis();
            break;
        case INPUT_EUC:
            euctojis();
            break;
        case INPUT_JIS:
            jistojis();
            break;
        default:
            copy();
        }
    else
        copy();
}

/* conversion information string */
char *convinfostr(char *buf)
{
    switch (detectedcode) {
    case INPUT_BINARY:
        strcpy(buf, "(Binary)");
        break;
    case INPUT_ASCII:
        strcpy(buf, "(Ascii)");
        break;
    case INPUT_UNKNOWN:
        strcpy(buf, "(Unknown)");
        break;
    case INPUT_SJIS:
        strcpy(buf, "(Shift-JIS");
        if (convmode == MODE_EUC)
            strcat(buf, " to EUC)");
        else if (convmode == MODE_JIS)
            strcat(buf, " to JIS)");
        else
            strcat(buf, ")");
        break;
    case INPUT_EUC:
        strcpy(buf, "(EUC");
        if (convmode == MODE_SJIS)
            strcat(buf, " to Shift-JIS)");
        else if (convmode == MODE_JIS)
            strcat(buf, " to JIS)");
        else
            strcat(buf, ")");
        break;
    case INPUT_JIS:
        strcpy(buf, "(JIS");
        if (convmode == MODE_SJIS)
            strcat(buf, " to Shift-JIS)");
        else if (convmode == MODE_EUC)
            strcat(buf, " to EUC)");
        else
            strcat(buf, ")");
        break;
    default:
        *buf = '\0';
    }
    return buf;
}

/* file opening, check, code detection, conversion, etc. */
int convert(char *filename)
{
    char *dir, *name, *tempname, *msg; //, info[19];
    int width, result;
    struct stat statbuf;
#ifdef MSDOS_HUMAN
    static int done_i = 0, done_o = 0;
#endif

    if (filename && filename[0] == '-' && filename[1] == '-')
        filename++;
    if (!infromstdin) {
        msg = NULL;
        if ((infd = open(filename, O_RDONLY | O_BINARY)) == -1) {
            return 0;
        }
        width = (((int) strlen(filename) - 1) / 12 + 1) * 12;
        if (stat(filename, &statbuf))
            runerror();
#if defined LSI_C || defined MS_C
        if (isatty(infd))
#else
        if ((statbuf.st_mode & S_IFMT) != S_IFREG)
#endif
            msg = "Is not a file";
        else if (!outtostdout && access(filename, 02))
            msg = "File is read only";
        else if (statbuf.st_size == 0)
            msg = "File is empty";
        if (msg) {
            closeinfile();
            return 1;
        }
    }
    else {

        infd = fileno(stdin);
        //infd = fileno(stin);
#ifdef MSDOS_HUMAN
        if (!done_i) {
#  ifdef LSI_C
            fsetbin(stdin);
#  elif defined HUMAN
            fmode(stdin, 1);
           // fmode(stin, 1);
#  else
            if (setmode(infd, O_BINARY) == -1) {
                printf("CodeConvertor: Binary mode not available\n");
                exit(1);
            }
#  endif
            done_i = 1;
        }
#endif

    }
    result = checkfile();
    if (result != CHECK_OK) {
        switch (result) {
        case CHECK_BINARY:
            msg = "May be binary file";
            break;
        case CHECK_SJIS:
            msg = "Shift-JIS code already exists";
            break;
        case CHECK_EUC:
            msg = "EUC already exists";
            break;
        case CHECK_JIS:
            msg = "JIS code already exists";
            break;
        case CHECK_ASCII:
            msg = "No KANJI code exists";
            break;
        case CHECK_UNKNOWN:
            msg = "Cannot identify KANJI code";
            break;
        case CHECK_NOTHING:
            msg = "Nothing to do";
        }
        closeinfile();
        return 1;
    }
    if (!outtostdout) {
        pathsplit(filename, &dir, &name);
        tempname = opentempfile(dir);
        free(dir);
        free(name);
        if (tempname == NULL) {
            closeinfile();
            return 1;
        }
    }
    else {

        outfd = fileno(stdout);
        //outfd = fileno(stout);
#ifdef MSDOS_HUMAN
       if (!done_o) {
#  ifdef LSI_C
            fsetbin(stdout);
#  elif defined HUMAN
            fmode(stdout, 1);
            //fmode(stout, 1);
#  else
            if (setmode(outfd, O_BINARY) == -1) {
                printf("CodeConvertor: Binary mode not available");
                exit(1);
            }
#  endif
            done_o = 1;
        }
#endif

    }
    conv();
    closeinfile();
    closeoutfile();
    if (!outtostdout) {
        if (chmod(tempname, statbuf.st_mode & 0777))
            runerror();
        if (unlink(filename))
            runerror();
        if (rename(tempname, filename))
            runerror();
        free(tempname);
    }
    return 1;
}


/* file opening, check, code detection, conversion, etc. */
int convertbuff(char *token)
{
    char *msg;
    int  result;
    static int done_i = 0, done_o = 0;

    if (token && token[0] == '-' && token[1] == '-')
      token++;
    result = checkfilebuff(token);
//printf("convert():token[%s]:result[%d]<br>\n",token,result);

    if (result != CHECK_OK) {
        switch (result) {
        case CHECK_BINARY:
            msg = "May be binary file";
            break;
        case CHECK_SJIS:
            msg = "Shift-JIS code already exists";
            break;
        case CHECK_EUC:
            msg = "EUC already exists";
            break;
        case CHECK_JIS:
            msg = "JIS code already exists";
            break;
        case CHECK_ASCII:
            msg = "No KANJI code exists";
            break;
        case CHECK_UNKNOWN:
            msg = "Cannot identify KANJI code";
            break;
        case CHECK_NOTHING:
            msg = "Nothing to do";
        }
        strcpy((char *)outbuf,(char *)inbuf);
        return 1;
    }
    done_o = 1;
    conv();
    outbufptr[0] = '\x0';
    return 1;
}

/* function main */
//int CALLTYPE main(argc, argv)
int QuickConvert(int argc, char *argv[])
{
    int filefound;

    checkemulationmode(*(argv + 1));
    tostdout();
    checkenvoption();
    while (--argc > 0)
        if (isoption(*++argv))
            checkoption(*argv);
        else
            break;
    fromstdin(argc);
    getmemory();
    if (!infromstdin) {
      filefound = 0;
      while (argc-- > 0)
        filefound |= convert(*argv++);
        if (!filefound)
          filenotfound();
    }
    else
      convert(NULL);

    free(inbuf);
    free(outbuf);
    free(newlinebuf);
    return 0;
}

int QuickConvertBuff(int argc, char *argv[], char *buff)
{
    int filefound;

//printf("argv[2]=%s<br>\n",argv[2]);

    checkemulationmode(*(argv + 1));
    //tostdout();
    checkenvoption();
    //while (--argc > 0)
	    --argc;
        if (isoption(*++argv))
            checkoption(*argv);
        //else
        //    break;
    fromstdin(argc);
    //if (convmode == MODE_HELP || convmode == MODE_DHELP)
    //    onlinehelp();
    //else {
#ifdef MSDOS
       // execqkcexe(argc, argv);
#endif
        getmemory();
        if (!infromstdin) {
            filefound = 0;
            while (argc-- > 0)
                filefound |= convertbuff(*argv++);
            if (!filefound)
                filenotfound();
        }
        else
            convertbuff(NULL);
    //}
    strcpy(buff, (char *)outbuf);
    //printf("%-4d:%s<br>\n", strlen(buff), buff);
    //printf("%-4d:%s\n", strlen(outbuf), outbuf);
    free(inbuf);
    free(outbuf);
    free(newlinebuf);
    return 0;
}
