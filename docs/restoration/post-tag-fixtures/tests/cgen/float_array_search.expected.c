#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>
#include "sn_minimal.h"

SnArray * __sn__observeReceiver(long long *);
float __sn__observeNeedle(long long *, float);
typedef struct __Closure__ {
    void *fn;
    size_t size;
    void (*__cleanup__)(void *);
    int __rc__;
} __Closure__;


SnArray * __sn__observeReceiver(long long *__sn__calls) {

    ({
        long long *__sn_place__ = &((*__sn__calls));
        long long __sn_previous__ = *__sn_place__;
        *__sn_place__ = sn_add_long(__sn_previous__, 1);
        __sn_previous__;
    });
    

    return ({
             SnArray *__al__ = sn_array_new(sizeof(float), 3);
             __al__->elem_tag = SN_TAG_DOUBLE;
     
     
             sn_array_push(__al__, &(float){ 0.0f });
     
             sn_array_push(__al__, &(float){ 1.5f });
     
             sn_array_push(__al__, &(float){ 1.5f });
             __al__;
         });}


float __sn__observeNeedle(long long *__sn__calls, float __sn__value) {

    ({
        long long *__sn_place__ = &((*__sn__calls));
        long long __sn_previous__ = *__sn_place__;
        *__sn_place__ = sn_add_long(__sn_previous__, 1);
        __sn_previous__;
    });
    

    return __sn__value;}

int main() {
    float __sn__positive_zero = 0.0f;
    float __sn__negative_zero = (-0.0f);
    float __sn__nan = sn_div_float(__sn__positive_zero, __sn__positive_zero);
    float __sn__copied_nan = __sn__nan;
    sn_auto_arr SnArray * __sn__values = ({
            SnArray *__al__ = sn_array_new(sizeof(float), 4);
            __al__->elem_tag = SN_TAG_DOUBLE;
    
    
            sn_array_push(__al__, &(float){ __sn__positive_zero });
    
            sn_array_push(__al__, &(float){ 1.5f });
    
            sn_array_push(__al__, &(float){ 1.5f });
    
            sn_array_push(__al__, &(float){ __sn__nan });
            __al__;
        });
    sn_auto_arr SnArray * __sn__empty = ({
            SnArray *__al__ = sn_array_new(sizeof(float), 0);
            __al__->elem_tag = SN_TAG_DOUBLE;
    
            __al__;
        });
    printf("%s\n", (__sn__arr_contains(&__sn__values, 1.5f)) ? "true" : "false");
    
    printf("%s\n", (__sn__arr_contains(&__sn__values, 9.5f)) ? "true" : "false");
    
    printf("%lld\n", (long long)(__sn__arr_indexOf(&__sn__values, 1.5f)));
    
    printf("%lld\n", (long long)(__sn__arr_indexOf(&__sn__values, 9.5f)));
    
    printf("%s\n", (__sn__arr_contains(&__sn__empty, 1.5f)) ? "true" : "false");
    
    printf("%lld\n", (long long)(__sn__arr_indexOf(&__sn__empty, 1.5f)));
    
    printf("%s\n", (__sn__arr_contains(&__sn__values, __sn__negative_zero)) ? "true" : "false");
    
    printf("%lld\n", (long long)(__sn__arr_indexOf(&__sn__values, __sn__negative_zero)));
    
    printf("%s\n", (__sn__arr_contains(&__sn__values, __sn__copied_nan)) ? "true" : "false");
    
    printf("%lld\n", (long long)(__sn__arr_indexOf(&__sn__values, __sn__copied_nan)));
    
    long long __sn__receiver_calls = 0LL;
    long long __sn__needle_calls = 0LL;
    sn_auto_arr SnArray * __sn____chain_tmp_0 = __sn__observeReceiver(&__sn__receiver_calls);
    printf("%s\n", (__sn__arr_contains(&__sn____chain_tmp_0, __sn__observeNeedle(&__sn__needle_calls, 1.5f))) ? "true" : "false");
    
    { sn_auto_str char *__ps__ = ({
            sn_auto_str char *__is_p0__ = sn_str_fmt("%lld", (long long)(__sn__receiver_calls));
            sn_auto_str char *__is_p1__ = sn_strdup(",");
            sn_auto_str char *__is_p2__ = sn_str_fmt("%lld", (long long)(__sn__needle_calls));
            sn_str_concat_multi(3, __is_p0__, __is_p1__, __is_p2__);
        }); sn_println(__ps__); };
    
    sn_auto_arr SnArray * __sn____chain_tmp_1 = __sn__observeReceiver(&__sn__receiver_calls);
    printf("%lld\n", (long long)(__sn__arr_indexOf(&__sn____chain_tmp_1, __sn__observeNeedle(&__sn__needle_calls, 1.5f))));
    
    { sn_auto_str char *__ps__ = ({
            sn_auto_str char *__is_p0__ = sn_str_fmt("%lld", (long long)(__sn__receiver_calls));
            sn_auto_str char *__is_p1__ = sn_strdup(",");
            sn_auto_str char *__is_p2__ = sn_str_fmt("%lld", (long long)(__sn__needle_calls));
            sn_str_concat_multi(3, __is_p0__, __is_p1__, __is_p2__);
        }); sn_println(__ps__); };
    
    fflush(stdout);
    return 0;
}
