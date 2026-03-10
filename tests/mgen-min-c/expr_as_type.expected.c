#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>
#include "sn_minimal.h"

int main() {
    long long __sn__a = 42LL;
    long long __sn__b = ((long long)(__sn__a));
    sn_assert((__sn__b == 42LL), "expected unboxed value to be 42");
    return 0;
}
