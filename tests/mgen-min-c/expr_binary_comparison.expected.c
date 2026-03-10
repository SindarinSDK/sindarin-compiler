#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>
#include "sn_minimal.h"

int main() {
    bool __sn__a = (1LL == 2LL);
    bool __sn__b = (1LL != 2LL);
    bool __sn__c = sn_lt_long(1LL, 2LL);
    bool __sn__d = sn_gt_long(1LL, 2LL);
    return 0LL;
}
