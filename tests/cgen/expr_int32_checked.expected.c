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
    return sn_mod_int32(sn_mul_int32(sn_sub_int32(sn_add_int32(2147483646LL, 1LL), 1LL), sn_div_int32(2LL, 2LL)), 2147483646LL);    fflush(stdout);
}
