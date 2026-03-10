#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>
#include "sn_minimal.h"

/* Struct type definitions */
typedef struct {
    long long __sn__value;
} __sn__Counter;

long long __sn__Counter_zero();

long long __sn__Counter_zero() {
    return 0LL;
}
int main() {
    long long __sn__z = __sn__Counter_zero();
    return __sn__z;
}
