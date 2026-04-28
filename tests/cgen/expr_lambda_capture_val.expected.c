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

typedef struct __closure_0__ {
    void *fn;
    size_t size;
    void (*__cleanup__)(void *);
    int __rc__;
    long long x;
} __closure_0__;
static long long __lambda_0__(void *__closure__, long long __sn__n);

int main() {
    long long __sn__x = 10LL;
    sn_auto_fn void * __sn__addX = ({
        __closure_0__ *__cl__ = malloc(sizeof(__closure_0__));
        __cl__->fn = (void *)__lambda_0__;
        __cl__->size = sizeof(__closure_0__);
        __cl__->__cleanup__ = NULL;
        __cl__->__rc__ = 1;
        __cl__->x = __sn__x;
        __cl__;
    });
    sn_assert((((long long (*)(void *, long long))((__Closure__ *)__sn__addX)->fn)(__sn__addX, 5LL) == 15LL), "expected addX(5) to be 15");
    
    fflush(stdout);
    return 0;
}

static long long __lambda_0__(void *__closure__, long long __sn__n) {

    long long __sn__x = ((__closure_0__ *)__closure__)->x;
    return sn_add_long(__sn__n, __sn__x);
}
