#include <stdio.h>
#include <string.h>

FILE *__cdecl __acrt_iob_func(unsigned);

FILE *__cdecl __iob_func(void)
{
    return __acrt_iob_func(0);
}

int strcasecmp(const char *s1, const char *s2)
{
    return _stricmp(s1, s2);
}

int dn_skipname(const unsigned char *ptr, const unsigned char *eom)
{
    const unsigned char *start = ptr;

    while (ptr < eom) {
        unsigned char len = *ptr++;

        if (len == 0) {
            return (int)(ptr - start);
        }

        if ((len & 0xC0) == 0xC0) {
            if (ptr >= eom) {
                return -1;
            }
            return (int)(ptr + 1 - start);
        }

        if ((len & 0xC0) != 0 || ptr + len > eom) {
            return -1;
        }

        ptr += len;
    }

    return -1;
}
