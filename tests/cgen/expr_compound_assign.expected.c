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
    long long __sn__x = 10LL;
    ({
        long long *__sn_place__ = &(__sn__x);
        long long __sn_rhs__ = 5LL;
        *__sn_place__ = sn_add_long(*__sn_place__, __sn_rhs__);
        *__sn_place__;
    });
    
    ({
        long long *__sn_place__ = &(__sn__x);
        long long __sn_rhs__ = 3LL;
        *__sn_place__ = sn_sub_long(*__sn_place__, __sn_rhs__);
        *__sn_place__;
    });
    
    return 0LL;    fflush(stdout);
}
