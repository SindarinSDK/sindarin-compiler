#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>
#include "sn_minimal.h"

/* Struct: Config (as val) */
typedef struct {
    long long __sn__width;
    long long __sn__height;
    double __sn__scale;
} __sn__Config;

static inline __sn__Config __sn__Config_copy(const __sn__Config *src) {
    __sn__Config dst;
    dst.__sn__width = src->__sn__width;
    dst.__sn__height = src->__sn__height;
    dst.__sn__scale = src->__sn__scale;
    return dst;
}


int main() {
    __sn__Config __sn__c = (__sn__Config){ .__sn__width = 800LL, .__sn__height = 600LL, .__sn__scale = 1.0 };
    sn_assert((__sn__c.__sn__width == 800LL), "expected default width to be 800");
    sn_assert((__sn__c.__sn__height == 600LL), "expected default height to be 600");
    return 0;
}
