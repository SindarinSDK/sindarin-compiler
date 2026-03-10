#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>
#include "sn_minimal.h"
void __sn__print_name(char *);

void __sn__print_name(char * __sn__name) {
    sn_assert(sn_gt_long(sn_str_length(__sn__name), 0LL), "name should not be empty");
}

int main() {
    sn_auto_str char * __sn__s = strdup("Alice");
    __sn__print_name(__sn__s);
    return 0;
}
