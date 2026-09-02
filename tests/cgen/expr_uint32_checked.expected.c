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
    uint32_t __sn__max = 4294967295LL;
    uint32_t __sn__one = 1LL;
    uint32_t __sn__add_base = 4294967294LL;
    uint32_t __sn__sum = sn_add_uint32(__sn__add_base, __sn__one);
    uint32_t __sn__difference = sn_sub_uint32(__sn__one, __sn__one);
    uint32_t __sn__mul_left = 65535LL;
    uint32_t __sn__mul_right = 65537LL;
    uint32_t __sn__product = sn_mul_uint32(__sn__mul_left, __sn__mul_right);
    uint32_t __sn__quotient = sn_div_uint32(__sn__max, __sn__one);
    uint32_t __sn__remainder = sn_mod_uint32(__sn__max, 2LL);
    fflush(stdout);
    return 0;
}
