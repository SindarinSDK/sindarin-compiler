#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>
#include "sn_minimal.h"

long long __sn__x = 10LL;

long long __sn__y = 0;

typedef struct __Closure__ {
    void *fn;
    size_t size;
    void (*__cleanup__)(void *);
} __Closure__;

int main() {
    __sn__y = sn_add_long(__sn__x, 5LL);
    sn_assert((__sn__y == 15LL), "expected y to be 15");
    fflush(stdout);
    return 0;
}
