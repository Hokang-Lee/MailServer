#ifndef SMTPDS_COMPAT_RESOLV_H
#define SMTPDS_COMPAT_RESOLV_H

#include <winsock2.h>
#include "arpa/nameser.h"

#ifndef MAXNS
#define MAXNS 3
#endif

#ifndef NAMESERVER_PORT
#define NAMESERVER_PORT 53
#endif

#ifndef RES_INIT
#define RES_INIT 0x00000001
#endif

#ifndef RES_IGNTC
#define RES_IGNTC 0x00000020
#endif

#ifndef RES_RECURSE
#define RES_RECURSE 0x00000040
#endif

#ifndef RES_DEFNAMES
#define RES_DEFNAMES 0x00000080
#endif

#ifndef RES_DNSRCH
#define RES_DNSRCH 0x00000200
#endif

typedef struct __res_state {
    int retrans;
    int retry;
    unsigned long options;
    int nscount;
    struct sockaddr_in nsaddr_list[MAXNS];
    char defdname[MAXDNAME];
    struct sockaddr_in6 nsaddr6_list[MAXNS];
} RES_STATE;

extern RES_STATE _res;

#ifdef __cplusplus
extern "C" {
#endif

int res_query(const char *dname, int class, int type, unsigned char *answer, int anslen);
int res_search(const char *dname, int class, int type, unsigned char *answer, int anslen);
int res_mkquery(int op, const char *dname, int class, int type, const void *data, int datalen, const void *newrr, void *buf, int buflen);
int res_send(const void *msg, int msglen, void *answer, int anslen, int af);
int dn_expand(const unsigned char *msg, const unsigned char *eomorig, const unsigned char *comp_dn, char *exp_dn, int length);
int dn_skipname(const unsigned char *ptr, const unsigned char *eom);
unsigned short _getshort(const unsigned char *msgp);
unsigned long _getlong(const unsigned char *msgp);

#ifdef __cplusplus
}
#endif

#endif
