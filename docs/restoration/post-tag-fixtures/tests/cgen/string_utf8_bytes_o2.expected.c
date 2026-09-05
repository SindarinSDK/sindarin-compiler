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
    sn_auto_str char * __sn__direct = strdup("X\x1f\x41Y");
    sn_auto_arr SnArray * __sn__directBytes = __sn___toBytes(&__sn__direct);
    sn_auto_str char * __sn__longGreedy = strdup("X\x1f\x41\x62\x30\x39Y");
    sn_auto_arr SnArray * __sn__longBytes = __sn___toBytes(&__sn__longGreedy);
    sn_auto_str char * __sn__lower = strdup("x\x1f\x61y");
    sn_auto_arr SnArray * __sn__lowerBytes = __sn___toBytes(&__sn__lower);
    bool __sn__ok = ((((((((((((((((sn_array_length(__sn__directBytes) == 4LL) && ((((unsigned char *)__sn__directBytes->data)[({ long long __ai__ = 0LL; __ai__ < 0 ? __ai__ + __sn__directBytes->len : __ai__; })]) == (unsigned char)88)) && ((((unsigned char *)__sn__directBytes->data)[({ long long __ai__ = 1LL; __ai__ < 0 ? __ai__ + __sn__directBytes->len : __ai__; })]) == (unsigned char)31)) && ((((unsigned char *)__sn__directBytes->data)[({ long long __ai__ = 2LL; __ai__ < 0 ? __ai__ + __sn__directBytes->len : __ai__; })]) == (unsigned char)65)) && ((((unsigned char *)__sn__directBytes->data)[({ long long __ai__ = 3LL; __ai__ < 0 ? __ai__ + __sn__directBytes->len : __ai__; })]) == (unsigned char)89)) && (sn_array_length(__sn__longBytes) == 7LL)) && ((((unsigned char *)__sn__longBytes->data)[({ long long __ai__ = 0LL; __ai__ < 0 ? __ai__ + __sn__longBytes->len : __ai__; })]) == (unsigned char)88)) && ((((unsigned char *)__sn__longBytes->data)[({ long long __ai__ = 1LL; __ai__ < 0 ? __ai__ + __sn__longBytes->len : __ai__; })]) == (unsigned char)31)) && ((((unsigned char *)__sn__longBytes->data)[({ long long __ai__ = 2LL; __ai__ < 0 ? __ai__ + __sn__longBytes->len : __ai__; })]) == (unsigned char)65)) && ((((unsigned char *)__sn__longBytes->data)[({ long long __ai__ = 3LL; __ai__ < 0 ? __ai__ + __sn__longBytes->len : __ai__; })]) == (unsigned char)98)) && ((((unsigned char *)__sn__longBytes->data)[({ long long __ai__ = 4LL; __ai__ < 0 ? __ai__ + __sn__longBytes->len : __ai__; })]) == (unsigned char)48)) && ((((unsigned char *)__sn__longBytes->data)[({ long long __ai__ = 5LL; __ai__ < 0 ? __ai__ + __sn__longBytes->len : __ai__; })]) == (unsigned char)57)) && ((((unsigned char *)__sn__longBytes->data)[({ long long __ai__ = 6LL; __ai__ < 0 ? __ai__ + __sn__longBytes->len : __ai__; })]) == (unsigned char)89)) && (sn_array_length(__sn__lowerBytes) == 4LL)) && ((((unsigned char *)__sn__lowerBytes->data)[({ long long __ai__ = 1LL; __ai__ < 0 ? __ai__ + __sn__lowerBytes->len : __ai__; })]) == (unsigned char)31)) && ((((unsigned char *)__sn__lowerBytes->data)[({ long long __ai__ = 2LL; __ai__ < 0 ? __ai__ + __sn__lowerBytes->len : __ai__; })]) == (unsigned char)97));
    return ({
             long long __match_result__;
             bool __match_subject__ = __sn__ok;
             if (__match_subject__ == true) {
                 __match_result__ = 0LL;
             } else {
                 __match_result__ = 1LL;
             }
             __match_result__;
         });    fflush(stdout);
}
