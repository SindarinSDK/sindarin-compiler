#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>
#include "sn_minimal.h"

/* Struct type definitions */
typedef struct {
    long long __sn__value;
} __sn__Counter;

void __sn__Counter_increment(__sn__Counter *);
long long __sn__Counter_getValue(__sn__Counter *);

void __sn__Counter_increment(__sn__Counter *self) {
    self->__sn__value = self->__sn__value + 1LL;
}
long long __sn__Counter_getValue(__sn__Counter *self) {
    return self->__sn__value;
}
int main() {
    __sn__Counter __sn__c = (__sn__Counter){ .__sn__value = 0LL };
    __sn__Counter_increment(&__sn__c);
    sn_assert((__sn__Counter_getValue(&__sn__c) == 1LL), "expected counter to be 1 after increment");
    return 0;
}
