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
    sn_auto_str char * __sn__a = strdup("hello");
    sn_auto_str char * __sn__b = strdup(__sn__a);
    sn_auto_str char * __sn__c = ({
            sn_auto_str char *__is_p0__ = sn_strdup("value: ");
            sn_auto_str char *__is_p1__ = sn_strdup(__sn__a);
            sn_str_concat_multi(2, __is_p0__, __is_p1__);
        });
    sn_assert((sn_str_length(__sn__a) == 5LL), "a should be 5 chars");
    
    fflush(stdout);
    return 0;
}
