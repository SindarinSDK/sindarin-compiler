#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>
#include "sn_minimal.h"
long long __sn__sideEffect();

long long __sn__sideEffect() {
    return 42LL;
}

int main() {
    __sn__sideEffect();
    return 0LL;
}
