#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>
#include "sn_minimal.h"

/* Struct: Point (as val) */
typedef struct {
    double __sn__x;
    double __sn__y;
} __sn__Point;

static inline __sn__Point __sn__Point_copy(const __sn__Point *src) {
    __sn__Point dst;
    dst.__sn__x = src->__sn__x;
    dst.__sn__y = src->__sn__y;
    return dst;
}


int main() {
    long long __sn__s = sizeof(__sn__Point);
    return 0LL;}
