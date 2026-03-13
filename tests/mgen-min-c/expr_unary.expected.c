#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>
#include "sn_minimal.h"

typedef struct __Closure__ {
    void *fn;
    size_t size;
    void (*__cleanup__)(void *);
} __Closure__;

int main() {
    long long __sn__x = 10LL;
    long long __sn__y = (-__sn__x);
    bool __sn__b = true;
    bool __sn__c = (!__sn__b);
    return 0LL;}
