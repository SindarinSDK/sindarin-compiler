#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>
#include "sn_minimal.h"

int main() {
    long long __sn__a = 42LL;
    sn_assert(/* TODO: is check for minimal */ true, "expected a is int to be true");
    return 0;
}
