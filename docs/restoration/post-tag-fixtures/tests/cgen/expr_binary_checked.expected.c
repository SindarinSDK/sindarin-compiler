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
    long long __sn__add_left = 9223372036854775806LL;
    long long __sn__add_right = 1LL;
    long long __sn__sum = sn_add_long(__sn__add_left, __sn__add_right);
    long long __sn__min_base = (-9223372036854775807LL);
    long long __sn__min_step = 1LL;
    long long __sn__minimum = sn_sub_long(__sn__min_base, __sn__min_step);
    { sn_auto_str char *__ps__ = ({
            sn_auto_str char *__is_p0__ = sn_str_fmt("%lld", (long long)(__sn__sum));
            sn_auto_str char *__is_p1__ = sn_strdup(" ");
            sn_auto_str char *__is_p2__ = sn_str_fmt("%lld", (long long)(__sn__minimum));
            sn_str_concat_multi(3, __is_p0__, __is_p1__, __is_p2__);
        }); sn_println(__ps__); };
    
    fflush(stdout);
    return 0;
}
