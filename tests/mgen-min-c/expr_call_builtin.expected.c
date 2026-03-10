#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>
#include "sn_minimal.h"

int main() {
    char * __sn__s = "hello";
    long long __sn__n = sn_str_length(__sn__s);
    sn_assert((__sn__n == 5LL), "expected string length to be 5");
    return 0;
}
