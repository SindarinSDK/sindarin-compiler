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

int main() {
    _Atomic long long __sn__counter = 0LL;
    
    pthread_mutex_t __sn__counter_mutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_lock(&__sn__counter_mutex);
    {
        (__sn__counter = sn_add_long(__sn__counter, 1LL));
        
    }
    pthread_mutex_unlock(&__sn__counter_mutex);
    sn_assert((__sn__counter == 1LL), "expected counter to be 1 after lock increment");
    
    long long __sn__after_increment = ({pthread_mutex_lock(&__sn__counter_mutex); __sn__counter++; long long __sn_result__ = __sn__counter; pthread_mutex_unlock(&__sn__counter_mutex); __sn_result__; });
    long long __sn__after_decrement = ({pthread_mutex_lock(&__sn__counter_mutex); __sn__counter--; long long __sn_result__ = __sn__counter; pthread_mutex_unlock(&__sn__counter_mutex); __sn_result__; });
    fflush(stdout);
    return 0;
}
