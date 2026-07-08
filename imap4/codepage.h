// codepage.h

#ifndef CODEPAGE_H
#define CODEPAGE_H


// Codepage definitions

// See <winnls.h>
// http://msdn.microsoft.com/library/en-us/intl/unicode_81rn.asp


// DOS
#define CP_US                             437      // 米国 (MS-DOS) as known as OEM
#define CP_ARABIC_ASMO_708                708      // アラビア語 (ASMO 708)
#define CP_ARABIC_ASMO_449P               709      // アラビア語 (ASMO 449+, BCON V4)
#define CP_ARABIC_TP                      710      // アラビア語 (Transparent Arabic)
#define CP_ARABIC_NAFITHA                 711      // アラビア語 (Nafitha Enhanced)
#define CP_ARABIC_SAKHR                   714      // アラビア語 (Sakhr)
#define CP_ARABIC_TP_ASMO                 720      // アラビア語 (Transparent ASMO)
#define CP_ARABIC_NAFITHA_INT             721      // アラビア語 (Nafitha International)
#define CP_GREEK                          737      // ギリシャ語 (IBM)
#define CP_BALTIC                         775      // バルト語 (MS-DOS)             // Latin 7
#define CP_ARABIC_MUSSAED                 786      // アラビア語 (Mussaed Al Arabi)
#define CP_WIN31_EUROPEAN                 819      // 西ヨーロッパ言語 (Windows 3.1) = CP_ISO_EUROPEAN
#define CP_EUROPEAN                       850      // ヨーロッパ多言語 (MS-DOS)     // Latin 1
#define CP_GREEK_OLD                      851      // ギリシャ語 (IBM) - obsolete
#define CP_CENTRAL_EUROPEAN               852      // 中央ヨーロッパ多言語 (MS-DOS) // Latin 2
#define CP_TURKISH_OLD                    853      // トルコ語 (MS-DOS)             // Latin 3
#define CP_CYRILLIC                       855      // キリル語 (IBM) - obsolete
#define CP_HEBREW_OLD                     856      // ヘブライ語 (IBM)
#define CP_TURKISH                        857      // トルコ語 (IBM)                // Latin 5
#define CP_EUROPEAN_EURO                  858      // ヨーロッパ多言語＋ユーロ記号(IBM)
#define CP_PORTUGUESE                     860      // ポルトガル語 (MS-DOS)
#define CP_ICELANDIC                      861      // アイスランド語 (MS-DOS)
#define CP_HEBREW                         862      // ヘブライ語 (MS-DOS)
#define CP_CANADIAN_FRENCH                863      // カナダ系フランス語 (MS-DOS)
#define CP_ARABIC                         864      // アラビア語 (MS-DOS)
#define CP_NORDIC                         865      // 北欧語 (MS-DOS)
#define CP_RUSSIAN                        866      // ロシア語 (MS-DOS)
#define CP_MODERN_GREEK                   869      // 現代ギリシャ語 (IBM)
#define CP_THAI                           874      // タイ語
#define CP_CZECH                          895      // チェコ語 (Kamenicky CS)
#define CP_JAPANESE                       932      // 日本語 (Shift-JIS)
#define CP_CHINESE_SIMPLIFIED             936      // 中国語簡体字 (GB2312)
#define CP_KOREAN                         949      // 韓国語 (UHC)
#define CP_CHINESE_TRADITIONAL            950      // 中国語繁体字 (BIG5)

// UTF-16
#define CP_UNICODE                        1200     // Unicode UTF-16
#define CP_UNICODE_BE                     1201     // Unicode UTF-16 (Big Endian)

// Windows
#define CP_ANSI_CENTRAL_EUROPEAN          1250     // 中央ヨーロッパ語 (Windows)
#define CP_ANSI_CYRILLIC                  1251     // キリル語 (Windows)
#define CP_ANSI_EUROPEAN                  1252     // ヨーロッパ語 (Windows)
#define CP_ANSI_GREEK                     1253     // ギリシア語 (Windows)
#define CP_ANSI_TURKISH                   1254     // トルコ語 (Windows)
#define CP_ANSI_HEBREW                    1255     // ヘブライ語 (Windows)
#define CP_ANSI_ARABIC                    1256     // アラビア語 (Windows)
#define CP_ANSI_BALTIC                    1257     // バルト語 (Windows)
#define CP_ANSI_VIETNAMESE                1258     // ベトナム語 (Windows)

#define CP_KOREAN_JOHAB                   1361     // 韓国語 (Johab)

// Macintosh
#define CP_MAC_EUROPEAN                   10000
#define CP_MAC_JAPANESE                   10001
#define CP_MAC_CHINESE_TRADITIONAL        10002
#define CP_MAC_KOREAN                     10003
#define CP_MAC_ARABIC                     10004
#define CP_MAC_HEBREW                     10005
#define CP_MAC_GREEK                      10006
#define CP_MAC_CYRILLIC                   10007
#define CP_MAC_CHINESE_SIMPLIFIED         10008
#define CP_MAC_ROMANIAN                   10010
#define CP_MAC_UKRAINIAN                  10017
#define CP_MAC_THAI                       10021
#define CP_MAC_CENTRAL_EUROPEAN           10029
#define CP_MAC_ICELANDIC                  10079
#define CP_MAC_TURKISH                    10081
#define CP_MAC_CROATIAN                   10082

// Taiwan
#define CP_CHINESE_TRADITIONAL_CNS        20000    // 通用碼
#define CP_CHINESE_TRADITIONAL_TCA        20001    // 公会碼
#define CP_CHINESE_TRADITIONAL_ETEN       20002    // 倚天碼
#define CP_CHINESE_TRADITIONAL_IBM5550    20003    // IBM5550
#define CP_CHINESE_TRADITIONAL_TELETEXT   20004    // 電信碼
#define CP_CHINESE_TRADITIONAL_WANG       20005    // 王安碼

// What is IA5 ?
#define CP_IA5_WESTERN_EUROPEAN           20105    // 7-bit
#define CP_IA5_GERMAN                     20106    // 7-bit
#define CP_IA5_SWEDISH                    20107    // 7-bit
#define CP_IA5_NORWEGIAN                  20108    // 7-bit

#define CP_US_ASCII                       20127    // 米国 ASCII 7-bit
#define CP_T61                            20261    // TeleText
#define CP_ISO_6937                       20269    // Non-Spacing Accent

// EUC
#define CP_EUC_JAPANESE_MS                20932    // 日本語 (MS独自のEUC風)
#define CP_EUC_CHINESE_SIMPLIFIED         20936    // 中国語簡体字
#define CP_EUC_KOREAN                     20949    // 韓国語 (＝CP51949)

#define CP_EXT_ALPHA_LOWERCASE            21027    // Ext Alpha Lowercase (deprecated)
#define CP_EUROPA                         29001    // ?

// KOI8
#define CP_CYRILLIC_KOI8R                 20866    // キリル語 (KOI8-R)
#define CP_CYRILLIC_KOI8U                 21866    // キリル語 (KOI8-U)

// ISO
#define CP_ISO_EUROPEAN                   28591    // 西ヨーロッパ語 (ISO-8859-1)   // Latin 1
#define CP_ISO_CENTRAL_EUROPEAN           28592    // 中央ヨーロッパ語 (ISO-8859-2) // Latin 2
#define CP_ISO_TURKISH_OLD                28593    //          (ISO-8859-3)         // Latin 3
#define CP_ISO_BALTIC                     28594    // バルト語 (ISO-8859-4)         // Latin 4
#define CP_ISO_CYRILLIC                   28595    // キリル語 (ISO-8859-5)
#define CP_ISO_ARABIC                     28596    // アラビア語 (ISO-8859-6)
#define CP_ISO_GREEK                      28597    // ギリシア語 (ISO-8859-7)
#define CP_ISO_HEBREW_VISUAL              28598    // ヘブライ語 (ISO-8859-8) 視覚順
#define CP_ISO_TURKISH                    28599    // トルコ語 (ISO-8859-9)         // Latin 5
#define CP_ISO_NORDIC                     28600    // 北欧語 (ISO-8859-10)          // Latin 6
#define CP_ISO_THAI                       28601    // タイ語 (ISO-8859-11)
#define CP_ISO_BALTIC_EX                  28603    // バルト語 (ISO-8859-13)        // Latin 7
#define CP_ISO_CELTIC                     28604    // ケルト語 (ISO-8859-14)        // Latin 8
#define CP_ISO_EUROPEAN_EX                28605    // 西ヨーロッパ語 (ISO-8859-15)  // Latin 9
#define CP_ISO_ROMANIAN                   28606    // ルーマニア語 (ISO-8859-16)    // Latin 10

#define CP_ISO_HEBREW_LOGICAL             38598    // ヘブライ語 (ISO-8859-8) 論理順

// ISO-2022
#define CP_ISO_JAPANESE                   50220    // 日本語 (ISO-2022-JP)
#define CP_ISO_JAPANESE_AND_KANA          50221    // 日本語 (JIS-Allow 1 byte Kana)
#define CP_ISO_JAPANESE_AND_KANA_SIO      50222    // 日本語 (JIS-Allow 1 byte Kana - SO/SI)
#define CP_ISO_KOREAN                     50225    // 韓国語 (ISO-2022-KR)
#define CP_ISO_CHINESE_SIMPLIFIED         50227    // 中国語簡体字 (ISO-2022)
#define CP_ISO_CHINESE_TRADITIONAL        50229    // 中国語繁体字 (ISO-2022)

// EUC
#define CP_EUC_JAPANESE                   51932    // 日本語 (EUC)
#define CP_CHINESE_SIMPLIFIED_2           51936    // 中国語簡体字 (＝CP936)
#define CP_EUC_KOREAN_2                   51949    // 韓国語 (＝CP20949)
#define CP_CHINESE_TRADITIONAL_2          51950    // 中国語繁体字 (≒CP950)

#define CP_HZ_CHINESE_SIMPLIFIED          52936    // 中国語簡体字 (HZ)
#define CP_CHINESE_SIMPLIFIED_EX          54936    // 中国語簡体字 (GB18030)

// ISCII
#define CP_ISCII_DEVANAGARI               57002    // デーバナーガリー
#define CP_ISCII_BENGALI                  57003    // ベンガル語
#define CP_ISCII_TAMIL                    57004    // タミル語
#define CP_ISCII_TELUGU                   57005    // テルグ語
#define CP_ISCII_ASSAMESE                 57006    // アッサム語
#define CP_ISCII_ORIYA                    57007    // オリヤ語
#define CP_ISCII_KANNADA                  57008    // カンナダ語
#define CP_ISCII_MALAYALAM                57009    // マラヤーラム語
#define CP_ISCII_GUJARATHI                57010    // グジャラート語
#define CP_ISCII_PANJABI                  57011    // パンジャブ語   (GURMUKHI)

// UTF-7/8/32
#define CP_UTF_7                          65000    // Unicode UTF-7
#define CP_UTF_8                          65001    // Unicode UTF-8
#define CP_UTF_32                         65005    // Unicode UTF-32
#define CP_UTF_32_BE                      65006    // Unicode UTF-32 (Big Endian)



// IBM EBCDIC
#define CP_EBCDIC_US_CANADA               37
#define CP_EBCDIC_INTERNATIONAL           500
#define CP_EBCDIC_MULTILINGUAL            870
#define CP_EBCDIC_GREEK_MODERN            875
#define CP_EBCDIC_AIX_PAKISTAN            1006
#define CP_EBCDIC_CYRILLIC                1025
#define CP_EBCDIC_TURKISH                 1026
#define CP_EBCDIC_OPEN_EDITION_US         1046
#define CP_EBCDIC_SYSTEM_390              1047
#define CP_EBCDIC_PERSIAN                 1097
#define CP_EBCDIC_PERSIAN_PC              1098
#define CP_EBCDIC_LATVIA_LITHUANIA        1112
#define CP_EBCDIC_ESTONIA                 1122
#define CP_EBCDIC_UKRAINE                 1123
#define CP_EBCDIC_AIX_UKRAINE             1124
#define CP_EBCDIC_VIETNAMESE              1130
#define CP_EBCDIC_LAO                     1132
#define CP_EBCDIC_DEVANAGARI              1137
#define CP_EBCDIC_US_CANADA_EURO          1140
#define CP_EBCDIC_GERMANY_EURO            1141
#define CP_EBCDIC_DENMARK_NORWAY_EURO     1142
#define CP_EBCDIC_FINLAND_SWEDEN_EURO     1143
#define CP_EBCDIC_ITALY_EURO              1144
#define CP_EBCDIC_SPAIN_EURO              1145
#define CP_EBCDIC_UK_EURO                 1146
#define CP_EBCDIC_FRANCE_EURO             1147
#define CP_EBCDIC_INTERNATIONAL_EURO      1148
#define CP_EBCDIC_ICELANDIC_EURO          1149
#define CP_EBCDIC_MULTILINGUAL_EUR        1153
#define CP_EBCDIC_CYRILLIC_EURO           1154
#define CP_EBCDIC_TURKEY_EURO             1155
#define CP_EBCDIC_BALTIC_EURO             1156
#define CP_EBCDIC_ESTONIA_EURO            1157
#define CP_EBCDIC_UKRAINE_EURO            1158
#define CP_EBCDIC_VIETNAMESE_EURO         1164

#define CP_EBCDIC_GERMANY                 20273
#define CP_EBCDIC_DENMARK_NORWAY          20277
#define CP_EBCDIC_FINLAND_SWEDEN          20278
#define CP_EBCDIC_ITALY                   20280
#define CP_EBCDIC_SPAIN                   20284
#define CP_EBCDIC_UK                      20285
#define CP_EBCDIC_JAPANESE_KATAKANA       20290
#define CP_EBCDIC_FRANCE                  20297
#define CP_EBCDIC_ARABIC                  20420
#define CP_EBCDIC_GREEK                   20423
#define CP_EBCDIC_HEBREW                  20424
#define CP_EBCDIC_KOREAN_EXTENDED         20833
#define CP_EBCDIC_THAI                    20838
#define CP_EBCDIC_ICELANDIC               20871
#define CP_EBCDIC_RUSSIAN                 20880
#define CP_EBCDIC_TURKISH2                20905
#define CP_EBCDIC_LATIN1_OPEN_SYSTEM      20924
#define CP_EBCDIC_SERBIAN_BULGARIAN       21025

#define CP_EBCDIC_JAPANESE_AND_KATAKANA   50930
#define CP_EBCDIC_JAPANESE_AND_LATIN      50939
#define CP_EBCDIC_JAPANESE_AND_US_CANADA  50931
#define CP_EBCDIC_KOREAN_AND_KOREAN_EX    50933
#define CP_EBCDIC_SIMPLIFIED_CHINESE      50935
#define CP_EBCDIC_TRADITIONAL_CHINESE     50937



// Not for General Use
#define CP_USER_DEFINED                   50000
#define CP_AUTO_SELECT                    50001
#define CP_JAPANESE_AUTO_SELECT           50932
#define CP_KOREAN_AUTO_SELECT             50949


// Some compilers do not support
#ifndef CP_SYMBOL
#define CP_SYMBOL 42
#endif



// Only Valid for RTF Converter
#define CP_CHINESE_HKSCS                  951      // BIG5-HKSCS

// Only Valid for RTF Converter
#define CP_RTFCONV_SPECIFIC_MIN           58000
#define CP_GEORGIAN                       58000    // グルジア語 (GEOSTD8)
#define CP_ARMENIAN                       58001    // アルメニア語 (ARMSCII-8)
#define CP_ARMENIAN_8A                    58002    // アルメニア語 (ARMSCII-8A)
#define CP_GEORGIAN_AC                    58003    // グルジア語 (Georgian Academy)
#define CP_GEORGIAN_PS                    58004    // グルジア語 (Georgian Parliament-Soros)
#define CP_CYRILLIC_KOI8RU                58010    // キリル語 (KOI8-RU)
#define CP_CYRILLIC_KOI8T                 58011    // キリル語 (KOI8-T)
#define CP_CYRILLIC_KOI8C                 58012    // キリル語 (KOI8-C)
#define CP_CYRILLIC_KOI8O                 58013    // キリル語 (KOI8-O)
#define CP_CYRILLIC_KOI8UNI               58014    // キリル語 (KOI8-UNI)
#define CP_CYRILLIC_ECMA                  58015    // キリル語 (ISO-IR-111, ECMA-Cyrillic)
#define CP_CYRILLIC_CYRWIN                58016    // キリル語 (CyrWin)
#define CP_CYRILLIC_ASIAN                 58017    // キリル語 (PT154)
#define CP_CYRILLIC_KAZ                   58018    // キリル語 (KZ-1048)
#define CP_PERSIAN_ISIRI                  58060    // ペルシア語 (ISIRI-3342)
#define CP_TAMIL_TSCII                    58100    // タミル語 (TSCII)
#define CP_TAMIL_TAM                      58101    // タミル語 (Tamil Monolingual)
#define CP_TAMIL_TAB                      58102    // タミル語 (Tamil Bilingual)
#define CP_VIETNAMESE_TCVN                58200    // ベトナム語 (TCVN-1)
#define CP_VIETNAMESE_VISCII              58201    // ベトナム語 (VISCII)
#define CP_VIETNAMESE_VPS                 58202    // ベトナム語 (VPS)
#define CP_VIETNAMESE_VNI                 58203    // ベトナム語 (VNI)
#define CP_VIETNAMESE_VIQR                58204    // ベトナム語 (Vietnamese Quoted-Readable)
#define CP_VIETNAMESE_TCVN2               58205    // ベトナム語 (TCVN-2)
#define CP_VIETNAMESE_TCVN3               58206    // ベトナム語 (TCVN-3)
#define CP_EUC_CHINESE_TRADITIONAL        58950    // 中国語繁体字 (EUC)
#define CP_RTFCONV_SPECIFIC_MAX           58950



#endif


