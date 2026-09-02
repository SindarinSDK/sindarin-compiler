#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>
#include "sn_minimal.h"

long long __sn__bumpAndReturn(long long *);
typedef struct __Closure__ {
    void *fn;
    size_t size;
    void (*__cleanup__)(void *);
    int __rc__;
} __Closure__;


long long __sn__bumpAndReturn(long long *__sn__counter) {

    ({
        long long __sn_rhs__ = 1LL;
        long long *__sn_place__ = &((*__sn__counter));
        *__sn_place__ = sn_add_long(*__sn_place__, __sn_rhs__);
        *__sn_place__;
    });
    

    return (-1LL);}

int main() {
    long long __sn__negative = (-1LL);
    long long __sn__as_long = __sn__int_toLong(__sn__negative);
    uint64_t __sn__as_uint = __sn__int_toUint(__sn__negative);
    unsigned char __sn__as_byte = __sn__int_toByte(__sn__negative);
    uint64_t __sn__half_uint = 9223372036854775807LL;
    uint64_t __sn__max_uint = sn_add_uint(sn_mul_uint(__sn__half_uint, 2LL), 1LL);
    long long __sn__long_value = 42LL;
    long long __sn__as_int = __sn__long_toInt(__sn__long_value);
    unsigned char __sn__byte_value = (unsigned char)200;
    long long __sn__widened = __sn__byte_toInt(__sn__byte_value);
    bool __sn____chain_tmp_0 = true;
    long long __sn__true_value = __sn__bool_toInt(__sn____chain_tmp_0);
    bool __sn____chain_tmp_1 = false;
    long long __sn__false_value = __sn__bool_toInt(__sn____chain_tmp_1);
    long long __sn__counter = 0LL;
    long long __sn____chain_tmp_2 = __sn__bumpAndReturn(&__sn__counter);
    unsigned char __sn__called_byte = __sn__int_toByte(__sn____chain_tmp_2);
    long long __sn__called_value = __sn__byte_toInt(__sn__called_byte);
    { sn_auto_str char *__ps__ = ({
            sn_auto_str char *__is_p0__ = sn_strdup(((((((((__sn__as_long == (-1LL)) && (__sn__as_uint == __sn__max_uint)) && (__sn__as_byte == (unsigned char)255)) && (__sn__as_int == 42LL)) && (__sn__widened == 200LL)) && (__sn__true_value == 1LL)) && (__sn__false_value == 0LL))) ? "true" : "false");
            sn_auto_str char *__is_p1__ = sn_strdup(" ");
            sn_auto_str char *__is_p2__ = sn_strdup((((__sn__counter == 1LL) && (__sn__called_value == 255LL))) ? "true" : "false");
            sn_str_concat_multi(3, __is_p0__, __is_p1__, __is_p2__);
        }); sn_println(__ps__); };
    
    fflush(stdout);
    return 0;
}
