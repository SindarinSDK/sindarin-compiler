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
    while (true) {
        __sn__x++;
        
        if ((__sn__x == 5LL)) {
            break;
        }
    }
    sn_assert((__sn__x == 5LL), "expected x to be 5 after break");
    
    fflush(stdout);
    return 0;
}
