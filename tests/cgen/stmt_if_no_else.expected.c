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
    if (sn_gt_long(__sn__x, 5LL)) {
        (__sn__x = 1LL);
        
    }
    sn_assert((__sn__x == 1LL), "expected x to be 1 after if branch");
    
    fflush(stdout);
    return 0;
}
