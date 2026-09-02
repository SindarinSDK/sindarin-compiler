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
    long long __sn__sum = 0LL;
    {
        for (long long __sn__i = 0LL; sn_lt_long(__sn__i, 10LL); ({
        long long *__sn_place__ = &(__sn__i);
        long long __sn_previous__ = *__sn_place__;
        *__sn_place__ = sn_add_long(__sn_previous__, 1);
        __sn_previous__;
    })) {
            ({
                long long *__sn_place__ = &(__sn__sum);
                long long __sn_rhs__ = __sn__i;
                *__sn_place__ = sn_add_long(*__sn_place__, __sn_rhs__);
                *__sn_place__;
            });
            
        }
    }
    sn_assert((__sn__sum == 45LL), "expected sum of 0..9 to be 45");
    
    fflush(stdout);
    return 0;
}
