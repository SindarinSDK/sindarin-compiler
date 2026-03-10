#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>
#include "sn_minimal.h"

int main() {
    sn_auto_str char * __sn__s = strdup("hello");
    ({
        free(__sn__s);
        __sn__s = strdup("world");
        __sn__s;
    });
    sn_assert((sn_str_length(__sn__s) == 5LL), "s should be 5 chars after reassign");
    return 0;
}
