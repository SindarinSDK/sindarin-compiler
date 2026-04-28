#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>
#include "sn_minimal.h"

typedef struct __Closure__ {
    void *fn;
    size_t size;
    void (*__cleanup__)(void *);
    int __rc__;
} __Closure__;

static long long __lambda_0__(void *__closure__, long long __sn__a, long long __sn__b);

int main() {
    sn_auto_fn void * __sn__add = ({
        __Closure__ *__cl__ = malloc(sizeof(__Closure__));
        __cl__->fn = (void *)__lambda_0__;
        __cl__->size = sizeof(__Closure__);
        __cl__->__cleanup__ = NULL;
        __cl__->__rc__ = 1;
        __cl__;
    });
    sn_assert((((long long (*)(void *, long long, long long))((__Closure__ *)__sn__add)->fn)(__sn__add, 1LL, 2LL) == 3LL), "expected 1 + 2 to be 3");
    
    fflush(stdout);
    return 0;
}

static long long __lambda_0__(void *__closure__, long long __sn__a, long long __sn__b) {
    return sn_add_long(__sn__a, __sn__b);
}
