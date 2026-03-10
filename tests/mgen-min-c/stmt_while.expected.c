#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>
#include "sn_minimal.h"

int main() {
    long long __sn__x = 0LL;
    while (sn_lt_long(__sn__x, 10LL)) {
        __sn__x++;
    }
    sn_assert((__sn__x == 10LL), "expected x to be 10 after while loop");
    return 0;
}
