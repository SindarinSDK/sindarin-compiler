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
} __Closure__;

typedef struct __closure_0__ {
    void *fn;
    size_t size;
    void (*__cleanup__)(void *);
    long long *x;
} __closure_0__;
static void __closure_0_free__(void *p) {
    __closure_0__ *cl = (__closure_0__ *)p;
    free(cl->x);
    free(cl);
}
static void __closure_0_cleanup__(void **p) {
    if (*p) {
        free(*p);
    }
    *p = NULL;
}
#define sn_auto_closure_0 __attribute__((cleanup(__closure_0_cleanup__)))
static void __lambda_0__(void *__closure__);

int main() {
    sn_auto_capture long long *__sn__x = malloc(sizeof(long long)); *__sn__x = 10LL;
    sn_auto_closure_0 void * __sn__inc = ({
        __closure_0__ *__cl__ = malloc(sizeof(__closure_0__));
        __cl__->fn = (void *)__lambda_0__;
        __cl__->size = sizeof(__closure_0__);
        __cl__->__cleanup__ = NULL;
        __cl__->x = __sn__x;
        __cl__;
    });
    ((void (*)(void *))((__Closure__ *)__sn__inc)->fn)(__sn__inc);
    sn_assert(((*__sn__x) == 11LL), "expected x to be 11 after inc()");
    fflush(stdout);
    return 0;
}

static void __lambda_0__(void *__closure__) {

    long long *__sn__x = ((__closure_0__ *)__closure__)->x;

    (*__sn__x) = (*__sn__x) + 1LL;
}
