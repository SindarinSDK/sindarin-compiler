#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>
#include "sn_minimal.h"

/* Struct type definitions */
typedef struct {
    double __sn__x;
    double __sn__y;
} __sn__Point;


int main() {
    __sn__Point __sn__p = (__sn__Point){ .__sn__x = 1.0, .__sn__y = 2.0 };
    return 0LL;
}
