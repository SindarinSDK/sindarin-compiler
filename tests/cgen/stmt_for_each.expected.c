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
    sn_auto_arr SnArray * __sn__arr = ({
            SnArray *__al__ = sn_array_new(sizeof(long long), 3);
            __al__->elem_tag = SN_TAG_INT;
    
    
            sn_array_push(__al__, &(long long){ 1LL });
    
            sn_array_push(__al__, &(long long){ 2LL });
    
            sn_array_push(__al__, &(long long){ 3LL });
            __al__;
        });
    long long __sn__sum = 0LL;
    {
        SnArray *__arr_0__ = __sn__arr;
        long long __len_0__ = __arr_0__->len;
        for (long long __idx_0__ = 0; __idx_0__ < __len_0__; __idx_0__++) {
            long long __sn__item = ((long long *)__arr_0__->data)[__idx_0__];
            {
                __sn__sum = __sn__sum + __sn__item;
            }
        }
    }
    sn_assert((__sn__sum == 6LL), "expected sum of {1,2,3} to be 6");
    return 0;
}
