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
} __Closure__;

typedef struct __closure_0__ {
    void *fn;
    size_t size;
    long long *x;
} __closure_0__;
static void __lambda_0__(void *__closure__);

int main() {
    long long __sn__x_storage = 10LL; long long *__sn__x = &__sn__x_storage;
    sn_auto_ptr void * __sn__inc = ({
        __closure_0__ *__cl__ = malloc(sizeof(__closure_0__));
        __cl__->fn = (void *)__lambda_0__;
        __cl__->size = sizeof(__closure_0__);
        __cl__->x = __sn__x;
        __cl__;
    });
    ((void (*)(void *))((__Closure__ *)__sn__inc)->fn)(__sn__inc);
    sn_assert(((*__sn__x) == 11LL), "expected x to be 11 after inc()");
    return 0;
}

static void __lambda_0__(void *__closure__) {
    long long *__sn__x = ((__closure_0__ *)__closure__)->x;

    (*__sn__x) = (*__sn__x) + 1LL;
}
