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
            SnArray *__al__ = sn_array_new(sizeof(long long), 5);
            __al__->elem_tag = SN_TAG_INT;
    
    
            sn_array_push(__al__, &(long long){ 1LL });
    
            sn_array_push(__al__, &(long long){ 2LL });
    
            sn_array_push(__al__, &(long long){ 3LL });
    
            sn_array_push(__al__, &(long long){ 4LL });
    
            sn_array_push(__al__, &(long long){ 5LL });
            __al__;
        });
    sn_auto_arr SnArray * __sn__s = sn_array_slice(__sn__arr, 1LL, 3LL);
    return 0LL;    fflush(stdout);
}
