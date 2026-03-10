#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>
#include "sn_minimal.h"

/* Struct type definitions */
typedef struct {
    char * __sn__key;
    char * __sn__value;
} __sn__Entry;


int main() {
    __sn__Entry __sn__e = (__sn__Entry){ .__sn__key = "name", .__sn__value = "Alice" };
    return 0LL;
}
