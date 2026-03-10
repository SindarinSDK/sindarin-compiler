#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>
#include "sn_minimal.h"
char * __sn__make_str();

char * __sn__make_str() {
    char * __sn__s = "hello";
    return __sn__s;
}

int main() {
    char * __sn__result = __sn__make_str();
    return 0LL;
}
