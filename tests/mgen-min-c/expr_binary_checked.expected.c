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
    long long __sn__a = 100LL;
    long long __sn__b = sn_add_long(__sn__a, 50LL);
    sn_assert((__sn__b == 150LL), "expected 100 + 50 to be 150");
    return 0;
}
