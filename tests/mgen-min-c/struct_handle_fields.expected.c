#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>
#include "sn_minimal.h"

/* Struct type definitions */
typedef struct {
    char * __sn__name;
    long long __sn__age;
} __sn__Person;


int main() {
    __sn__Person __sn__p = (__sn__Person){ .__sn__name = "Alice", .__sn__age = 30LL };
    return 0LL;
}
