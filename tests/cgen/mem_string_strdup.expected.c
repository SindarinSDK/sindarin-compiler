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
            char __is_buf__[1024];
            int __is_off__ = 0;
            __is_buf__[0] = '\0';
            __is_off__ += snprintf(__is_buf__ + __is_off__, sizeof(__is_buf__) - __is_off__, "%s", "value: ");
            __is_off__ += snprintf(__is_buf__ + __is_off__, sizeof(__is_buf__) - __is_off__, "%s", __sn__a);
            strdup(__is_buf__);
        });
    sn_assert((sn_str_length(__sn__a) == 5LL), "a should be 5 chars");
    fflush(stdout);
    return 0;
}
