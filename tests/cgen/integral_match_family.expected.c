#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>
#include "sn_minimal.h"

long long __sn__observeLong(long long *, long long *, long long);
uint64_t __sn__observeUint(long long *, uint64_t);
float __sn__observeFloat(long long *, long long *, long long, float);
double __sn__observeDouble(long long *, long long *, long long, double);
typedef struct __Closure__ {
    void *fn;
    size_t size;
    void (*__cleanup__)(void *);
    int __rc__;
} __Closure__;


long long __sn__observeLong(long long *__sn__calls, long long *__sn__order, long long __sn__value) {

    ({
        long long *__sn_place__ = &((*__sn__calls));
        long long __sn_previous__ = *__sn_place__;
        *__sn_place__ = sn_add_long(__sn_previous__, 1);
        __sn_previous__;
    });
    

    (*__sn__order = sn_add_long(sn_mul_long((*__sn__order), 10LL), 1LL));
    

    return __sn__value;}


uint64_t __sn__observeUint(long long *__sn__calls, uint64_t __sn__value) {

    ({
        long long *__sn_place__ = &((*__sn__calls));
        long long __sn_previous__ = *__sn_place__;
        *__sn_place__ = sn_add_long(__sn_previous__, 1);
        __sn_previous__;
    });
    

    return __sn__value;}


float __sn__observeFloat(long long *__sn__calls, long long *__sn__order, long long __sn__marker, float __sn__value) {

    ({
        long long *__sn_place__ = &((*__sn__calls));
        long long __sn_previous__ = *__sn_place__;
        *__sn_place__ = sn_add_long(__sn_previous__, 1);
        __sn_previous__;
    });
    

    (*__sn__order = sn_add_long(sn_mul_long((*__sn__order), 10LL), __sn__marker));
    

    return __sn__value;}


double __sn__observeDouble(long long *__sn__calls, long long *__sn__order, long long __sn__marker, double __sn__value) {

    ({
        long long *__sn_place__ = &((*__sn__calls));
        long long __sn_previous__ = *__sn_place__;
        *__sn_place__ = sn_add_long(__sn_previous__, 1);
        __sn_previous__;
    });
    

    (*__sn__order = sn_add_long(sn_mul_long((*__sn__order), 10LL), __sn__marker));
    

    return __sn__value;}

int main() {
    long long __sn____sn_match_subject = 41LL;
    long long __sn____sn_match_result = 42LL;
    long long __sn__subject_calls = 0LL;
    long long __sn__result_calls = 0LL;
    long long __sn__order = 0LL;
    long long __sn__selected = 0LL;
    ({
            long long __match_result__;
            long long __match_subject__ = __sn__observeLong(&__sn__subject_calls, &__sn__order, 2LL);
            if (__match_subject__ == 1LL || __match_subject__ == 2LL || __match_subject__ == 2LL || __match_subject__ == 2LL || __match_subject__ == 2LL) {
                (__sn__selected = 10LL);
    
    __match_result__ = (__sn__order = sn_add_long(sn_mul_long(__sn__order, 10LL), 2LL));
            } else if (__match_subject__ == 2LL) {
                (__sn__selected = 20LL);
    
    __match_result__ = (__sn__order = sn_add_long(sn_mul_long(__sn__order, 10LL), 8LL));
            } else {
                (__sn__selected = 30LL);
    
    __match_result__ = (__sn__order = sn_add_long(sn_mul_long(__sn__order, 10LL), 9LL));
            }
            __match_result__;
        });
    
    long long __sn__int32_selected = 0LL;
    int32_t __sn__int32_value = (-7LL);
    ({
            long long __match_result__;
            int32_t __match_subject__ = __sn__int32_value;
            if (__match_subject__ == (-6LL) || __match_subject__ == (-7LL) || __match_subject__ == (-7LL)) {
                __match_result__ = (__sn__int32_selected = 1LL);
            } else {
                __match_result__ = (__sn__int32_selected = 2LL);
            }
            __match_result__;
        });
    
    long long __sn__uint32_selected = 0LL;
    uint32_t __sn__uint32_value = 4LL;
    ({
            long long __match_result__;
            uint32_t __match_subject__ = __sn__uint32_value;
            if (__match_subject__ == 1LL || __match_subject__ == 2LL || __match_subject__ == 3LL || __match_subject__ == 4LL || __match_subject__ == 5LL) {
                __match_result__ = (__sn__uint32_selected = 1LL);
            } else {
                __match_result__ = (__sn__uint32_selected = 2LL);
            }
            __match_result__;
        });
    
    long long __sn__uint_selected = 0LL;
    uint64_t __sn__uint_value = 5LL;
    ({
            long long __match_result__;
            uint64_t __match_subject__ = __sn__uint_value;
            if (__match_subject__ == 1LL || __match_subject__ == 2LL || __match_subject__ == 3LL || __match_subject__ == 4LL || __match_subject__ == 5LL) {
                __match_result__ = (__sn__uint_selected = 1LL);
            } else {
                __match_result__ = (__sn__uint_selected = 2LL);
            }
            __match_result__;
        });
    
    long long __sn__byte_selected = 0LL;
    unsigned char __sn__byte_value = (unsigned char)255;
    ({
            long long __match_result__;
            unsigned char __match_subject__ = __sn__byte_value;
            if (__match_subject__ == (unsigned char)1 || __match_subject__ == 255LL) {
                __match_result__ = (__sn__byte_selected = 1LL);
            } else {
                __match_result__ = (__sn__byte_selected = 2LL);
            }
            __match_result__;
        });
    
    long long __sn__no_match = 7LL;
    ({
            unsigned char __match_subject__ = (unsigned char)9;
            if (__match_subject__ == (unsigned char)8) {
                (__sn__no_match = 99LL);
    
    
            }
        });
    
    long long __sn__boundary_hits = 0LL;
    ({
            long long __match_subject__ = (-9223372036854775807LL);
            if (__match_subject__ == (-9223372036854775807LL)) {
                ({
        long long *__sn_place__ = &(__sn__boundary_hits);
        long long __sn_previous__ = *__sn_place__;
        *__sn_place__ = sn_add_long(__sn_previous__, 1);
        __sn_previous__;
    });
    
    
            }
        });
    
    ({
            long long __match_subject__ = 9223372036854775807LL;
            if (__match_subject__ == 9223372036854775807LL) {
                ({
        long long *__sn_place__ = &(__sn__boundary_hits);
        long long __sn_previous__ = *__sn_place__;
        *__sn_place__ = sn_add_long(__sn_previous__, 1);
        __sn_previous__;
    });
    
    
            }
        });
    
    int32_t __sn__int32_min = (-2147483647LL);
    ({
        int32_t *__sn_place__ = &(__sn__int32_min);
        int32_t __sn_rhs__ = 1LL;
        *__sn_place__ = sn_sub_int32(*__sn_place__, __sn_rhs__);
        *__sn_place__;
    });
    
    ({
            int32_t __match_subject__ = __sn__int32_min;
            if (__match_subject__ == (-2147483648LL)) {
                ({
        long long *__sn_place__ = &(__sn__boundary_hits);
        long long __sn_previous__ = *__sn_place__;
        *__sn_place__ = sn_add_long(__sn_previous__, 1);
        __sn_previous__;
    });
    
    
            }
        });
    
    ({
            int32_t __match_subject__ = 2147483647LL;
            if (__match_subject__ == 2147483647LL) {
                ({
        long long *__sn_place__ = &(__sn__boundary_hits);
        long long __sn_previous__ = *__sn_place__;
        *__sn_place__ = sn_add_long(__sn_previous__, 1);
        __sn_previous__;
    });
    
    
            }
        });
    
    ({
            uint32_t __match_subject__ = 0LL;
            if (__match_subject__ == 0LL) {
                ({
        long long *__sn_place__ = &(__sn__boundary_hits);
        long long __sn_previous__ = *__sn_place__;
        *__sn_place__ = sn_add_long(__sn_previous__, 1);
        __sn_previous__;
    });
    
    
            }
        });
    
    ({
            uint32_t __match_subject__ = 4294967295LL;
            if (__match_subject__ == 4294967295LL) {
                ({
        long long *__sn_place__ = &(__sn__boundary_hits);
        long long __sn_previous__ = *__sn_place__;
        *__sn_place__ = sn_add_long(__sn_previous__, 1);
        __sn_previous__;
    });
    
    
            }
        });
    
    ({
            uint64_t __match_subject__ = 0LL;
            if (__match_subject__ == 0LL) {
                ({
        long long *__sn_place__ = &(__sn__boundary_hits);
        long long __sn_previous__ = *__sn_place__;
        *__sn_place__ = sn_add_long(__sn_previous__, 1);
        __sn_previous__;
    });
    
    
            }
        });
    
    ({
            uint64_t __match_subject__ = 9223372036854775807LL;
            if (__match_subject__ == 9223372036854775807LL) {
                ({
        long long *__sn_place__ = &(__sn__boundary_hits);
        long long __sn_previous__ = *__sn_place__;
        *__sn_place__ = sn_add_long(__sn_previous__, 1);
        __sn_previous__;
    });
    
    
            }
        });
    
    ({
            unsigned char __match_subject__ = (unsigned char)0;
            if (__match_subject__ == 0LL) {
                ({
        long long *__sn_place__ = &(__sn__boundary_hits);
        long long __sn_previous__ = *__sn_place__;
        *__sn_place__ = sn_add_long(__sn_previous__, 1);
        __sn_previous__;
    });
    
    
            }
        });
    
    ({
            unsigned char __match_subject__ = (unsigned char)255;
            if (__match_subject__ == (unsigned char)255) {
                ({
        long long *__sn_place__ = &(__sn__boundary_hits);
        long long __sn_previous__ = *__sn_place__;
        *__sn_place__ = sn_add_long(__sn_previous__, 1);
        __sn_previous__;
    });
    
    
            }
        });
    
    long long __sn__nested_statement = 0LL;
    ({
            uint32_t __match_subject__ = 1LL;
            if (__match_subject__ == 1LL) {
                ({
            unsigned char __match_subject__ = (unsigned char)2;
            if (__match_subject__ == 2LL) {
                (__sn__nested_statement = 12LL);
    
    
            }
        });
    
    
            }
        });
    
    bool __sn__bool_result = ({
            bool __match_result__;
            long long __match_subject__ = 1LL;
            if (__match_subject__ == 1LL) {
                __match_result__ = true;
            } else {
                __match_result__ = false;
            }
            __match_result__;
        });
    long long __sn__int_result = ({
            long long __match_result__;
            int32_t __match_subject__ = (-2LL);
            if (__match_subject__ == (-2LL)) {
                __match_result__ = 20LL;
            } else {
                __match_result__ = 0LL;
            }
            __match_result__;
        });
    long long __sn__long_result = ({
            long long __match_result__;
            uint32_t __match_subject__ = 4294967295LL;
            if (__match_subject__ == 4294967295LL) {
                __match_result__ = (-30LL);
            } else {
                __match_result__ = 0LL;
            }
            __match_result__;
        });
    int32_t __sn__int32_result = ({
            int32_t __match_result__;
            uint64_t __match_subject__ = 3LL;
            if (__match_subject__ == 3LL) {
                __match_result__ = (-40LL);
            } else {
                __match_result__ = 0LL;
            }
            __match_result__;
        });
    uint32_t __sn__uint32_result = ({
            uint32_t __match_result__;
            unsigned char __match_subject__ = (unsigned char)4;
            if (__match_subject__ == 4LL) {
                __match_result__ = 50LL;
            } else {
                __match_result__ = 0LL;
            }
            __match_result__;
        });
    uint64_t __sn__uint_result = ({
            uint64_t __match_result__;
            long long __match_subject__ = 5LL;
            if (__match_subject__ == 5LL) {
                __match_result__ = 60LL;
            } else {
                __match_result__ = 0LL;
            }
            __match_result__;
        });
    unsigned char __sn__byte_result = ({
            unsigned char __match_result__;
            int32_t __match_subject__ = 6LL;
            if (__match_subject__ == 6LL) {
                __match_result__ = (unsigned char)70;
            } else {
                __match_result__ = (unsigned char)0;
            }
            __match_result__;
        });
    long long __sn__bool_subject_int_result = ({
            long long __match_result__;
            bool __match_subject__ = true;
            if (__match_subject__ == true) {
                __match_result__ = 80LL;
            } else {
                __match_result__ = 0LL;
            }
            __match_result__;
        });
    bool __sn__int_subject_bool_result = ({
            bool __match_result__;
            long long __match_subject__ = 1LL;
            if (__match_subject__ == 1LL) {
                __match_result__ = true;
            } else {
                __match_result__ = false;
            }
            __match_result__;
        });
    float __sn__float_result = ({
            float __match_result__;
            uint32_t __match_subject__ = 7LL;
            if (__match_subject__ == 7LL) {
                __match_result__ = __sn__observeFloat(&__sn__result_calls, &__sn__order, 3LL, 6.25f);
            } else {
                __match_result__ = __sn__observeFloat(&__sn__result_calls, &__sn__order, 8LL, 0.0f);
            }
            __match_result__;
        });
    double __sn__double_result = ({
            double __match_result__;
            uint64_t __match_subject__ = __sn__observeUint(&__sn__subject_calls, 8LL);
            if (__match_subject__ == 8LL) {
                __match_result__ = __sn__observeDouble(&__sn__result_calls, &__sn__order, 4LL, 7.5);
            } else {
                __match_result__ = __sn__observeDouble(&__sn__result_calls, &__sn__order, 9LL, 0.0);
            }
            __match_result__;
        });
    long long __sn__nested_value = ({
            long long __match_result__;
            unsigned char __match_subject__ = (unsigned char)1;
            if (__match_subject__ == 1LL) {
                __match_result__ = ({
            long long __match_result__;
            uint32_t __match_subject__ = 2LL;
            if (__match_subject__ == 2LL) {
                __match_result__ = 77LL;
            } else {
                __match_result__ = 0LL;
            }
            __match_result__;
        });
            } else {
                __match_result__ = (-1LL);
            }
            __match_result__;
        });
    { sn_auto_str char *__ps__ = ({
            sn_auto_str char *__is_p0__ = sn_str_fmt("%lld", (long long)(__sn____sn_match_subject));
            sn_auto_str char *__is_p1__ = sn_strdup(",");
            sn_auto_str char *__is_p2__ = sn_str_fmt("%lld", (long long)(__sn____sn_match_result));
            sn_auto_str char *__is_p3__ = sn_strdup(",");
            sn_auto_str char *__is_p4__ = sn_str_fmt("%lld", (long long)(__sn__subject_calls));
            sn_auto_str char *__is_p5__ = sn_strdup(",");
            sn_auto_str char *__is_p6__ = sn_str_fmt("%lld", (long long)(__sn__result_calls));
            sn_auto_str char *__is_p7__ = sn_strdup(",");
            sn_auto_str char *__is_p8__ = sn_str_fmt("%lld", (long long)(__sn__order));
            sn_auto_str char *__is_p9__ = sn_strdup(",");
            sn_auto_str char *__is_p10__ = sn_str_fmt("%lld", (long long)(__sn__selected));
            sn_auto_str char *__is_p11__ = sn_strdup(",");
            sn_auto_str char *__is_p12__ = sn_str_fmt("%lld", (long long)(__sn__int32_selected));
            sn_auto_str char *__is_p13__ = sn_strdup(",");
            sn_auto_str char *__is_p14__ = sn_str_fmt("%lld", (long long)(__sn__uint32_selected));
            sn_auto_str char *__is_p15__ = sn_strdup(",");
            sn_auto_str char *__is_p16__ = sn_str_fmt("%lld", (long long)(__sn__uint_selected));
            sn_auto_str char *__is_p17__ = sn_strdup(",");
            sn_auto_str char *__is_p18__ = sn_str_fmt("%lld", (long long)(__sn__byte_selected));
            sn_auto_str char *__is_p19__ = sn_strdup(",");
            sn_auto_str char *__is_p20__ = sn_str_fmt("%lld", (long long)(__sn__no_match));
            sn_auto_str char *__is_p21__ = sn_strdup(",");
            sn_auto_str char *__is_p22__ = sn_str_fmt("%lld", (long long)(__sn__boundary_hits));
            sn_auto_str char *__is_p23__ = sn_strdup(",");
            sn_auto_str char *__is_p24__ = sn_str_fmt("%lld", (long long)(__sn__nested_statement));
            sn_auto_str char *__is_p25__ = sn_strdup(",");
            sn_auto_str char *__is_p26__ = sn_strdup((__sn__bool_result) ? "true" : "false");
            sn_auto_str char *__is_p27__ = sn_strdup(",");
            sn_auto_str char *__is_p28__ = sn_str_fmt("%lld", (long long)(__sn__int_result));
            sn_auto_str char *__is_p29__ = sn_strdup(",");
            sn_auto_str char *__is_p30__ = sn_str_fmt("%lld", (long long)(__sn__long_result));
            sn_auto_str char *__is_p31__ = sn_strdup(",");
            sn_auto_str char *__is_p32__ = sn_str_fmt("%lld", (long long)(__sn__int32_result));
            sn_auto_str char *__is_p33__ = sn_strdup(",");
            sn_auto_str char *__is_p34__ = sn_str_fmt("%lld", (long long)(__sn__uint32_result));
            sn_auto_str char *__is_p35__ = sn_strdup(",");
            sn_auto_str char *__is_p36__ = sn_str_fmt("%lld", (long long)(__sn__uint_result));
            sn_auto_str char *__is_p37__ = sn_strdup(",");
            sn_auto_str char *__is_p38__ = sn_str_fmt("%u", (unsigned)(__sn__byte_result));
            sn_auto_str char *__is_p39__ = sn_strdup(",");
            sn_auto_str char *__is_p40__ = sn_str_fmt("%lld", (long long)(__sn__bool_subject_int_result));
            sn_auto_str char *__is_p41__ = sn_strdup(",");
            sn_auto_str char *__is_p42__ = sn_strdup((__sn__int_subject_bool_result) ? "true" : "false");
            sn_auto_str char *__is_p43__ = sn_strdup(",");
            sn_auto_str char *__is_p44__ = sn_strdup(((__sn__float_result == 6.25f)) ? "true" : "false");
            sn_auto_str char *__is_p45__ = sn_strdup(",");
            sn_auto_str char *__is_p46__ = sn_strdup(((__sn__double_result == 7.5)) ? "true" : "false");
            sn_auto_str char *__is_p47__ = sn_strdup(",");
            sn_auto_str char *__is_p48__ = sn_str_fmt("%lld", (long long)(__sn__nested_value));
            sn_str_concat_multi(49, __is_p0__, __is_p1__, __is_p2__, __is_p3__, __is_p4__, __is_p5__, __is_p6__, __is_p7__, __is_p8__, __is_p9__, __is_p10__, __is_p11__, __is_p12__, __is_p13__, __is_p14__, __is_p15__, __is_p16__, __is_p17__, __is_p18__, __is_p19__, __is_p20__, __is_p21__, __is_p22__, __is_p23__, __is_p24__, __is_p25__, __is_p26__, __is_p27__, __is_p28__, __is_p29__, __is_p30__, __is_p31__, __is_p32__, __is_p33__, __is_p34__, __is_p35__, __is_p36__, __is_p37__, __is_p38__, __is_p39__, __is_p40__, __is_p41__, __is_p42__, __is_p43__, __is_p44__, __is_p45__, __is_p46__, __is_p47__, __is_p48__);
        }); sn_println(__ps__); };
    
    fflush(stdout);
    return 0;
}
