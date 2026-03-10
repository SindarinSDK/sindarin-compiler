#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>
#include "sn_minimal.h"

char * __sn__name = NULL;

int main() {
    __sn__name = strdup("hello");
    ({
        free(__sn__name);
        __sn__name = strdup("world");
        __sn__name;
    });
    sn_assert((sn_str_length(__sn__name) == 5LL), "name should be world");
    free(__sn__name);
    return 0;
}
