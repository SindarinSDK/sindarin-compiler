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
    uint64_t __sn__half = 9223372036854775807LL;
    uint64_t __sn__two = 2LL;
    uint64_t __sn__one = 1LL;
    uint64_t __sn__max_minus_one = sn_mul_uint(__sn__half, __sn__two);
    uint64_t __sn__max = sn_add_uint(__sn__max_minus_one, __sn__one);
    uint64_t __sn__sum = sn_add_uint(__sn__max_minus_one, __sn__one);
    uint64_t __sn__difference = sn_sub_uint(__sn__one, __sn__one);
    uint64_t __sn__product = sn_mul_uint(__sn__half, __sn__two);
    uint64_t __sn__quotient = sn_div_uint(__sn__max, __sn__one);
    uint64_t __sn__remainder = sn_mod_uint(__sn__max, __sn__two);
    fflush(stdout);
    return 0;
}
