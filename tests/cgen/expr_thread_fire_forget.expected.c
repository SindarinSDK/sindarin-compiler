#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>
#include "sn_minimal.h"

void __sn__compute();

typedef struct {
    int _padding;
} __ThreadArgs_0__;

static void *__thread_wrapper_0__(void *arg) {
    SnThread *__th__ = (SnThread *)arg;

    __sn__compute();
    sn_thread_release(__th__);
    return NULL;
}
typedef struct __Closure__ {
    void *fn;
    size_t size;
    void (*__cleanup__)(void *);
    int __rc__;
} __Closure__;


void __sn__compute() {

    return;}

int main() {
    { SnThread *__ff__ = ({
        SnThread *__th__ = sn_thread_create();
    
        __th__->result_size = sizeof(void);
        pthread_create(&__th__->thread, NULL, __thread_wrapper_0__, __th__);
        __th__;
    }); __ff__->detached = 1; pthread_detach(__ff__->thread); sn_thread_release(__ff__); }
    
    return 0LL;    fflush(stdout);
}
