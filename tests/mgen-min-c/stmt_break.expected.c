#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>
#include "sn_minimal.h"

int main() {
    long long __sn__x = 0LL;
    while (true) {
        __sn__x++;
        if ((__sn__x == 5LL)) {
            break;
        }
    }
    sn_assert((__sn__x == 5LL), "expected x to be 5 after break");
    return 0;
}
