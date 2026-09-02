#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>
#include "sn_minimal.h"

void __sn__incrementCaptured(long long *);
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
    long long *value;
} __closure_0__;
static void __closure_0_free__(void *p) {
    __closure_0__ *cl = (__closure_0__ *)p;
    free(cl->value);
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


void __sn__incrementCaptured(long long *__sn__value) {

    sn_auto_closure_0 void * __sn__increment = ({
        __closure_0__ *__cl__ = malloc(sizeof(__closure_0__));
        __cl__->fn = (void *)__lambda_0__;
        __cl__->size = sizeof(__closure_0__);
        __cl__->__cleanup__ = NULL;
        __cl__->__rc__ = 1;
        __cl__->value = __sn__value;
        __cl__;
    });

    ((void (*)(void *))((__Closure__ *)__sn__increment)->fn)(__sn__increment);
    
}

int main() {
    long long __sn__value = 9223372036854775807LL;
    __sn__incrementCaptured(&__sn__value);
    
    printf("%lld\n", (long long)(__sn__value));
    
    fflush(stdout);
    return 0;
}

static void __lambda_0__(void *__closure__) {

    long long *__sn__value = ((__closure_0__ *)__closure__)->value;

    (*__sn__value) = (*__sn__value) + 1LL;
    
}
