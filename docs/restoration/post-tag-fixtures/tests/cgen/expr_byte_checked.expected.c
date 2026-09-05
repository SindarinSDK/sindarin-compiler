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
    unsigned char __sn__max = (unsigned char)255;
    unsigned char __sn__one = (unsigned char)1;
    unsigned char __sn__add_base = (unsigned char)254;
    unsigned char __sn__sum = sn_add_byte(__sn__add_base, __sn__one);
    unsigned char __sn__difference = sn_sub_byte(__sn__one, __sn__one);
    unsigned char __sn__product = sn_mul_byte(__sn__max, __sn__one);
    unsigned char __sn__quotient = sn_div_byte(__sn__max, __sn__one);
    unsigned char __sn__remainder = sn_mod_byte(__sn__max, __sn__one);
    fflush(stdout);
    return 0;
}
