#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>
#include "sn_minimal.h"

long long __sn__compute();

typedef struct {
    int _padding;
} __ThreadArgs_0__;

static void *__thread_wrapper_0__(void *arg) {
    SnThread *__th__ = (SnThread *)arg;

    long long __result__ = __sn__compute();
    if (!__th__->result) __th__->result = calloc(1, sizeof(long long));
    *(long long *)__th__->result = __result__;
    sn_thread_release(__th__);
    return NULL;
}
typedef struct __Closure__ {
    void *fn;
    size_t size;
    void (*__cleanup__)(void *);
    int __rc__;
} __Closure__;


long long __sn__compute() {

    return 42LL;}

int main() {
    long long __sn__handle = 0; sn_auto_thread SnThread * __sn__handle__th__ = ({
        SnThread *__th__ = sn_thread_create();
    
        __th__->result_size = sizeof(long long);
        pthread_create(&__th__->thread, NULL, __thread_wrapper_0__, __th__);
        __th__;
    });
    ({
        SnThread *__dt__ = __sn__handle__th__;
        if (__dt__) {
            __dt__->detached = 1;
            pthread_detach(__dt__->thread);
        }
        (void)0;
    });
    
    fflush(stdout);
    return 0;
}
