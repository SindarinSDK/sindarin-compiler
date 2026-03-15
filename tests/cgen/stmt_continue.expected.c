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
    long long __sn__sum = 0LL;
    {
        for (long long __sn__i = 0LL; sn_lt_long(__sn__i, 10LL); __sn__i++) {
            if ((__sn__i == 5LL)) {
                continue;
            }
            __sn__sum = __sn__sum + __sn__i;
            
        }
    }
    sn_assert((__sn__sum == 40LL), "expected sum to be 40 (skipping 5)");
    
    fflush(stdout);
    return 0;
}
