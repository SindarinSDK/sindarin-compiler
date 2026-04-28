#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>
#include "sn_minimal.h"

char * __sn__make_str();
typedef struct __Closure__ {
    void *fn;
    size_t size;
    void (*__cleanup__)(void *);
    int __rc__;
} __Closure__;


char * __sn__make_str() {

    sn_auto_str char * __sn__s = strdup("hello");

    {
        char * __ret__ = __sn__s;
        __sn__s = NULL;
        return __ret__;
    }}

int main() {
    sn_auto_str char * __sn__result = __sn__make_str();
    return 0LL;    fflush(stdout);
}
