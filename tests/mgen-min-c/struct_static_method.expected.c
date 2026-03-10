#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>
#include "sn_minimal.h"

/* Struct: Counter (as val) */
typedef struct {
    long long __sn__value;
} __sn__Counter;

static inline __sn__Counter __sn__Counter_copy(const __sn__Counter *src) {
    __sn__Counter dst;
    dst.__sn__value = src->__sn__value;
    return dst;
}

long long __sn__Counter_zero();

long long __sn__Counter_zero() {
    return 0LL;}
int main() {
    long long __sn__z = __sn__Counter_zero();
    return __sn__z;}
