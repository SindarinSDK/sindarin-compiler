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
    sn_auto_str char * __sn__s = strdup("hello");
    ({
        char *__sn_tmp__ = strdup("world");
        free(__sn__s);
        __sn__s = __sn_tmp__;
        __sn__s;
    });
    
    sn_assert((sn_str_length(__sn__s) == 5LL), "s should be 5 chars after reassign");
    
    fflush(stdout);
    return 0;
}
