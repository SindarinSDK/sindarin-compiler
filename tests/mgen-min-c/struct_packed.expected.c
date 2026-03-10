#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>
#include "sn_minimal.h"

/* Struct: PackedData (as val) */
typedef struct {
    unsigned char __sn__flags;
    long long __sn__value;
} __sn__PackedData;

static inline __sn__PackedData __sn__PackedData_copy(const __sn__PackedData *src) {
    __sn__PackedData dst;
    dst.__sn__flags = src->__sn__flags;
    dst.__sn__value = src->__sn__value;
    return dst;
}


int main() {
    return 0LL;}
