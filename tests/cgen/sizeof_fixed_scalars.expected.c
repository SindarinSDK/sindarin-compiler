#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>
#include "sn_minimal.h"

char __sn__observe(long long *);
typedef struct __Closure__ {
    void *fn;
    size_t size;
    void (*__cleanup__)(void *);
    int __rc__;
} __Closure__;


char __sn__observe(long long *__sn__counter) {

    ({
        long long __sn_rhs__ = 1LL;
        long long *__sn_place__ = &((*__sn__counter));
        *__sn_place__ = sn_add_long(*__sn_place__, __sn_rhs__);
        *__sn_place__;
    });
    

    return (char)120;}

int main() {
    long long __sn__type_int = sizeof(long long);
    long long __sn__type_long = sizeof(long long);
    long long __sn__type_int32 = sizeof(int32_t);
    long long __sn__type_uint = sizeof(uint64_t);
    long long __sn__type_uint32 = sizeof(uint32_t);
    long long __sn__type_byte = sizeof(unsigned char);
    long long __sn__type_bool = sizeof(bool);
    long long __sn__type_char = sizeof(char);
    long long __sn__type_float = sizeof(float);
    long long __sn__type_double = sizeof(double);
    long long __sn__integer = 9LL;
    float __sn__single = 1.5;
    long long __sn__counter = 0LL;
    long long __sn__expression_sizes = sn_add_long(sn_add_long(sizeof(long long), sizeof(float)), sizeof(char));
    long long __sn__arithmetic = sn_add_long(sizeof(long long), sn_mul_long(sizeof(float), sizeof(unsigned char)));
    bool __sn__comparison = sn_lt_long(sizeof(char), sizeof(double));
    bool __sn__types_ok = ((((((((((__sn__type_int == 8LL) && (__sn__type_long == 8LL)) && (__sn__type_int32 == 4LL)) && (__sn__type_uint == 8LL)) && (__sn__type_uint32 == 4LL)) && (__sn__type_byte == 1LL)) && (__sn__type_bool == 1LL)) && (__sn__type_char == 1LL)) && (__sn__type_float == 4LL)) && (__sn__type_double == 8LL));
    bool __sn__expressions_ok = (__sn__expression_sizes == 13LL);
    { sn_auto_str char *__ps__ = ({
            sn_auto_str char *__is_p0__ = sn_strdup((__sn__types_ok) ? "true" : "false");
            sn_auto_str char *__is_p1__ = sn_strdup(" ");
            sn_auto_str char *__is_p2__ = sn_strdup((__sn__expressions_ok) ? "true" : "false");
            sn_auto_str char *__is_p3__ = sn_strdup(" ");
            sn_auto_str char *__is_p4__ = sn_strdup((((__sn__arithmetic == 12LL) && __sn__comparison)) ? "true" : "false");
            sn_auto_str char *__is_p5__ = sn_strdup(" ");
            sn_auto_str char *__is_p6__ = sn_strdup(((__sn__counter == 0LL)) ? "true" : "false");
            sn_str_concat_multi(7, __is_p0__, __is_p1__, __is_p2__, __is_p3__, __is_p4__, __is_p5__, __is_p6__);
        }); sn_println(__ps__); };
    
    fflush(stdout);
    return 0;
}
