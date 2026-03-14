#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>
#include "sn_minimal.h"

void __sn__increment(long long *);
typedef struct __Closure__ {
    void *fn;
    size_t size;
    void (*__cleanup__)(void *);
} __Closure__;


void __sn__increment(long long *__sn__x) {

    (*__sn__x = sn_add_long((*__sn__x), 1LL));
}

int main() {
    long long __sn__a = 10LL;
    __sn__increment(&__sn__a);
    sn_assert((__sn__a == 11LL), "expected a to be 11 after increment by ref");
    return 0;
}
