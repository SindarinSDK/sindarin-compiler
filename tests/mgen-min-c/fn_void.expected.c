#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>
#include "sn_minimal.h"
void __sn__doNothing();

void __sn__doNothing() {
    return;
}

int main() {
    __sn__doNothing();
    return 0LL;
}
