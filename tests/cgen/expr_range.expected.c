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
    long long __sn__sum = 0LL;
    {
        sn_auto_arr SnArray *__arr_0__ = sn_array_range(1LL, 11LL);
        long long __len_0__ = __arr_0__->len;
        for (long long __idx_0__ = 0; __idx_0__ < __len_0__; __idx_0__++) {
            long long __sn__x = ((long long *)__arr_0__->data)[__idx_0__];
            {
                __sn__sum = __sn__sum + __sn__x;
            }
        }
    }
    sn_assert((__sn__sum == 55LL), "expected sum of 1..10 to be 55");
    fflush(stdout);
    return 0;
}
