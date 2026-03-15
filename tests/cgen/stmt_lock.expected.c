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

int main() {
    long long __sn__x = 0LL;
    
    pthread_mutex_t __sn__x_mutex = PTHREAD_MUTEX_INITIALIZER;
    long long __sn__y = 0LL;
    
    pthread_mutex_t __sn__y_mutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_lock(&__sn__x_mutex);
    {
        (__sn__x = sn_add_long(__sn__x, 1LL));
        
        (__sn__y = __sn__x);
        
    }
    pthread_mutex_unlock(&__sn__x_mutex);
    sn_assert((__sn__y == 1LL), "expected y to be 1 after lock increment");
    
    fflush(stdout);
    return 0;
}
