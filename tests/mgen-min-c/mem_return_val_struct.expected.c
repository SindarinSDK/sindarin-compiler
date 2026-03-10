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

__sn__Point __sn__make_point(long long, long long);

__sn__Point __sn__make_point(long long __sn__x, long long __sn__y) {
    __sn__Point __sn__p = (__sn__Point){ .__sn__x = __sn__x, .__sn__y = __sn__y };
    return __sn__p;}

int main() {
    __sn__Point __sn__p = __sn__make_point(3LL, 4LL);
    sn_assert((__sn__p.__sn__x == 3LL), "x should be 3");
    return 0;
}
