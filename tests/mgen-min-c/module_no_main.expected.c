#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>
#include "sn_minimal.h"
long long __sn__add(long long, long long);

long long __sn__add(long long __sn__a, long long __sn__b) {
    return sn_add_long(__sn__a, __sn__b);
}


