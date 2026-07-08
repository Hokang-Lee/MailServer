////////////////////////////////////////////////////////////
// Version.h Copyright K.kawakami
////////////////////////////////////////////////////////////
#ifdef E_POST
#ifdef V5
 #define VERSION "(5.00)"
#else
#ifdef V4
#ifdef WIN64
 #define VERSION "(4.74) x64"
#else
 #define VERSION "(4.74)"
#endif
#else
#ifdef V3
 #define VERSION "(3.00)"
#else
#ifdef IPv6
 #define VERSION "(2.00)"
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
#ifdef BETA
 #define VERSION "(0.12)"
#else
#ifdef V4
 #define VERSION "(4.16)"
#else
#ifdef V3
 #define VERSION "(3.28)"
#else
#ifdef IPv6
 #define VERSION "(2.24)"
#else
 #define VERSION "(1.23)"
#endif
#endif
#endif
#endif
#endif
#endif
