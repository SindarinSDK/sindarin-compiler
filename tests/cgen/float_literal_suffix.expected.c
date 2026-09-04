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
    sn_auto_arr SnArray * __sn__values = ({
            SnArray *__al__ = sn_array_new(sizeof(float), 2);
            __al__->elem_tag = SN_TAG_DOUBLE;
    
    
            sn_array_push(__al__, &(float){ 16777217.0f });
    
            sn_array_push(__al__, &(float){ 1.5f });
            __al__;
        });
    printf("%s\n", ((16777217.0f == 16777216.0f)) ? "true" : "false");
    
    printf("%s\n", (__sn__arr_contains(&__sn__values, 16777217.0f)) ? "true" : "false");
    
    printf("%lld\n", (long long)(__sn__arr_indexOf(&__sn__values, 16777217.0f)));
    
    printf("%s\n", ((((((float *)__sn__values->data)[({ long long __ai__ = 0LL; __ai__ < 0 ? __ai__ + __sn__values->len : __ai__; })]) == 16777216.0f) && ((((float *)__sn__values->data)[({ long long __ai__ = 1LL; __ai__ < 0 ? __ai__ + __sn__values->len : __ai__; })]) == 1.5f))) ? "true" : "false");
    
    double __sn__precise = 16777217.0;
    printf("%s\n", ((__sn__precise == 16777217.0)) ? "true" : "false");
    
    fflush(stdout);
    return 0;
}
