#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>
#include "sn_minimal.h"
long long __sn__apply(void *, long long);

typedef struct __Closure__ {
    void *fn;
    size_t size;
} __Closure__;

static long long __lambda_0__(void *__closure__, long long __sn__n);

long long __sn__apply(void * __sn__f, long long __sn__x) {
    return ((long long (*)(void *, long long))((__Closure__ *)__sn__f)->fn)(__sn__f, __sn__x);}

int main() {
    sn_auto_ptr void * __sn__double_it = ({
        __Closure__ *__cl__ = malloc(sizeof(__Closure__));
        __cl__->fn = (void *)__lambda_0__;
        __cl__->size = sizeof(__Closure__);
        __cl__;
    });
    sn_assert((__sn__apply(__sn__double_it, 5LL) == 10LL), "expected apply(double_it, 5) to be 10");
    return 0;
}

static long long __lambda_0__(void *__closure__, long long __sn__n) {
    return sn_mul_long(__sn__n, 2LL);
}
