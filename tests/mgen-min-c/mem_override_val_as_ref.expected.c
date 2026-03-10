#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>
#include "sn_minimal.h"

/* Struct: Point (as val) */
typedef struct {
    long long __sn__x;
    long long __sn__y;
} __sn__Point;

static inline __sn__Point __sn__Point_copy(const __sn__Point *src) {
    __sn__Point dst;
    dst.__sn__x = src->__sn__x;
    dst.__sn__y = src->__sn__y;
    return dst;
}

void __sn__mutate(__sn__Point *);

void __sn__mutate(__sn__Point *__sn__p) {
    ((*__sn__p).__sn__x = 99LL);
}

int main() {
    __sn__Point __sn__p = (__sn__Point){ .__sn__x = 1LL, .__sn__y = 2LL };
    __sn__mutate(&__sn__p);
    sn_assert((__sn__p.__sn__x == 99LL), "should be mutated via ref");
    return 0;
}
