#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>
#include "sn_minimal.h"

long long __sn__add(long long, long long);
typedef struct __Closure__ {
    void *fn;
    size_t size;
    void (*__cleanup__)(void *);
} __Closure__;


long long __sn__add(long long __sn__a, long long __sn__b) {

    return sn_add_long(__sn__a, __sn__b);}

int main() {
    long long __sn__x = 42LL;
    long long __sn__y = __sn__add(__sn__x, 10LL);
    sn_assert((__sn__y == 52LL), "expected add(42, 10) to be 52");
    fflush(stdout);
    return 0;
}
