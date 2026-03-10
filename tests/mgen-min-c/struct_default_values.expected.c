#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>
#include "sn_minimal.h"

/* Struct type definitions */
typedef struct {
    long long __sn__width;
    long long __sn__height;
    double __sn__scale;
} __sn__Config;


int main() {
    __sn__Config __sn__c = (__sn__Config){ .__sn__width = 800LL, .__sn__height = 600LL, .__sn__scale = 1.0 };
    sn_assert((__sn__c.__sn__width == 800LL), "expected default width to be 800");
    sn_assert((__sn__c.__sn__height == 600LL), "expected default height to be 600");
    return 0;
}
