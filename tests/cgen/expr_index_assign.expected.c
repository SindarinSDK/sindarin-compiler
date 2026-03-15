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
    
    
            sn_array_push(__al__, &(long long){ 10LL });
    
            sn_array_push(__al__, &(long long){ 20LL });
    
            sn_array_push(__al__, &(long long){ 30LL });
            __al__;
        });
    ((long long *)__sn__arr->data)[({ long long __ai__ = 0LL; __ai__ < 0 ? __ai__ + __sn__arr->len : __ai__; })] = 99LL;
    
    return 0LL;    fflush(stdout);
}
