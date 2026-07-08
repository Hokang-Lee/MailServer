////////////////////////////////////////////////////////////
// Version.h Copyright K.kawakami
////////////////////////////////////////////////////////////
#ifdef E_POST
#ifdef V5
 #define VERSION "(5.00)"
#else
#ifdef V4
#ifdef WIN64
 #define VERSION "(4.AN) x64"
#else
 #define VERSION "(4.AN)"
#endif
#else
#ifdef V3
 #define VERSION "(3.04)"
#else
#ifdef IPv6
 #define VERSION "(2.18)"
#else
 #define VERSION "(1.00)"
#endif
#endif
#endif
#endif
#else
#ifdef V5
 #define VERSION "(5.00)"
#else
#ifdef V4
 #define VERSION "(4.24)"
#else
#ifdef V3
#ifdef K_SEARCH // K_SEARCH OEM ”Å //KTEC SMTP Archive Receiver
 #define VERSION "(1.03)"
#else
 #define VERSION "(3.40)"
#endif
#else
#ifdef IPv6
 #define VERSION "(2.47)"
#else
 #define VERSION "(1.89)"
#endif
#endif
#endif
#endif
#endif
