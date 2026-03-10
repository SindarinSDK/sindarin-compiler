#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>
#include "sn_minimal.h"

int main() {
    long long __sn__x = 10LL;
    if (sn_gt_long(__sn__x, 5LL)) {
        (__sn__x = 1LL);
    } else {
        (__sn__x = 2LL);
    }
    sn_assert((__sn__x == 1LL), "expected x to be 1 after if branch");
    return 0;
}
