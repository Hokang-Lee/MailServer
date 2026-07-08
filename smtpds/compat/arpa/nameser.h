#ifndef SMTPDS_COMPAT_ARPA_NAMESER_H
#define SMTPDS_COMPAT_ARPA_NAMESER_H

#include <winsock2.h>

#ifndef PACKETSZ
#define PACKETSZ 512
#endif

#ifndef MAXDNAME
#define MAXDNAME 1025
#endif

#ifndef C_IN
#define C_IN 1
#endif

#ifndef T_A
#define T_A 1
#endif

#ifndef T_NS
#define T_NS 2
#endif

#ifndef T_MX
#define T_MX 15
#endif

#ifndef T_AAAA
#define T_AAAA 28
#endif

#ifndef QFIXEDSZ
#define QFIXEDSZ 4
#endif

#ifndef QUERY
#define QUERY 0
#endif

#ifndef FORMERR
#define FORMERR 1
#endif

#ifndef SERVFAIL
#define SERVFAIL 2
#endif

#ifndef NXDOMAIN
#define NXDOMAIN 3
#endif

#ifndef NOTIMP
#define NOTIMP 4
#endif

#ifndef REFUSED
#define REFUSED 5
#endif

typedef struct {
    unsigned id : 16;
    unsigned rd : 1;
    unsigned tc : 1;
    unsigned aa : 1;
    unsigned opcode : 4;
    unsigned qr : 1;
    unsigned rcode : 4;
    unsigned cd : 1;
    unsigned ad : 1;
    unsigned unused : 1;
    unsigned ra : 1;
    unsigned qdcount : 16;
    unsigned ancount : 16;
    unsigned nscount : 16;
    unsigned arcount : 16;
} HEADER;

#endif
