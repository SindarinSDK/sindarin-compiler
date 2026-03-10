#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>
#include "sn_minimal.h"

long long __sn__counter = 0LL;

int main() {
    (__sn__counter = 42LL);
    sn_assert((__sn__counter == 42LL), "expected counter to be 42");
    return 0;
}
