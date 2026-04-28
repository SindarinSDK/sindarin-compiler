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
    int __rc__;
} __Closure__;

int main() {
    sn_auto_str char * __sn__name = strdup("world");
    sn_auto_str char * __sn__msg = ({
            sn_auto_str char *__is_p0__ = sn_strdup("Hello ");
            sn_auto_str char *__is_p1__ = sn_strdup(__sn__name);
            sn_auto_str char *__is_p2__ = sn_strdup("!");
            sn_str_concat_multi(3, __is_p0__, __is_p1__, __is_p2__);
        });
    sn_assert((sn_str_length(__sn__msg) == 12LL), "expected interpolated string length to be 12");
    
    fflush(stdout);
    return 0;
}
