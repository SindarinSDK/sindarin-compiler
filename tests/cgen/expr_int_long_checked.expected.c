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
    long long __sn__int_max_minus_one = 9223372036854775806LL;
    long long __sn__int_one = 1LL;
    long long __sn__int_max = sn_add_long(__sn__int_max_minus_one, __sn__int_one);
    long long __sn__int_min_base = (-9223372036854775807LL);
    long long __sn__int_min = sn_sub_long(__sn__int_min_base, __sn__int_one);
    long long __sn__int_mul_base = (-4611686018427387904LL);
    long long __sn__int_two = 2LL;
    long long __sn__int_product = sn_mul_long(__sn__int_mul_base, __sn__int_two);
    long long __sn__int_quotient = sn_div_long(__sn__int_min, __sn__int_one);
    long long __sn__int_remainder = sn_mod_long(__sn__int_min, __sn__int_one);
    long long __sn__long_max_minus_one = 9223372036854775806LL;
    long long __sn__long_one = 1LL;
    long long __sn__long_max = sn_add_long(__sn__long_max_minus_one, __sn__long_one);
    long long __sn__long_min_base = (-9223372036854775807LL);
    long long __sn__long_min = sn_sub_long(__sn__long_min_base, __sn__long_one);
    long long __sn__long_mul_base = (-4611686018427387904LL);
    long long __sn__long_two = 2LL;
    long long __sn__long_product = sn_mul_long(__sn__long_mul_base, __sn__long_two);
    long long __sn__long_quotient = sn_div_long(__sn__long_min, __sn__long_one);
    long long __sn__long_remainder = sn_mod_long(__sn__long_min, __sn__long_one);
    return 0LL;    fflush(stdout);
}
