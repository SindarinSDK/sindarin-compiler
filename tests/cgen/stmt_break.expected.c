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
    long long __sn__x = 0LL;
    while (true) {
        ({
            long long *__sn_place__ = &(__sn__x);
            long long __sn_previous__ = *__sn_place__;
            *__sn_place__ = sn_add_long(__sn_previous__, 1);
            __sn_previous__;
        });
        
        if ((__sn__x == 5LL)) {
            break;
        }
    }
    sn_assert((__sn__x == 5LL), "expected x to be 5 after break");
    
    fflush(stdout);
    return 0;
}
