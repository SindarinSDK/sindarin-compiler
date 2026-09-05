#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>
#include "sn_minimal.h"

float __sn__observeFloat(long long *, long long *, long long, float);
double __sn__observeDouble(long long *, long long *, long long, double);
long long __sn__observeInt(long long *, long long *, long long, long long);
double __sn__chooseDouble(long long *, long long *, long long *);
typedef struct __Closure__ {
    void *fn;
    size_t size;
    void (*__cleanup__)(void *);
    int __rc__;
} __Closure__;


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


long long __sn__observeInt(long long *__sn__calls, long long *__sn__order, long long __sn__marker, long long __sn__value) {

    ({
        long long *__sn_place__ = &((*__sn__calls));
        long long __sn_previous__ = *__sn_place__;
        *__sn_place__ = sn_add_long(__sn_previous__, 1);
        __sn_previous__;
    });
    

    (*__sn__order = sn_add_long(sn_mul_long((*__sn__order), 10LL), __sn__marker));
    

    return __sn__value;}


double __sn__chooseDouble(long long *__sn__subject_calls, long long *__sn__body_calls, long long *__sn__order) {

    return ({
             double __match_result__;
             double __match_subject__ = __sn__observeDouble(&(*__sn__subject_calls), &(*__sn__order), 7LL, 9.5);
             if (__match_subject__ == 9.5) {
                 __match_result__ = __sn__observeDouble(&(*__sn__body_calls), &(*__sn__order), 8LL, 42.25);
             } else {
                 __match_result__ = __sn__observeDouble(&(*__sn__body_calls), &(*__sn__order), 9LL, 0.0);
             }
             __match_result__;
         });}

int main() {
    long long __sn____sn_match_subject = 41LL;
    long long __sn____sn_match_result = 42LL;
    long long __sn__subject_calls = 0LL;
    long long __sn__body_calls = 0LL;
    long long __sn__order = 0LL;
    long long __sn__selected = 0LL;
    ({
            long long __match_result__;
            float __match_subject__ = __sn__observeFloat(&__sn__subject_calls, &__sn__order, 1LL, 2.5f);
            if (__match_subject__ == 1.0f || __match_subject__ == 2.5f || __match_subject__ == (-2.5f)) {
                (__sn__selected = 10LL);
    
    __match_result__ = __sn__observeInt(&__sn__body_calls, &__sn__order, 2LL, 0LL);
            } else if (__match_subject__ == 2.5f) {
                (__sn__selected = 20LL);
    
    __match_result__ = __sn__observeInt(&__sn__body_calls, &__sn__order, 8LL, 0LL);
            } else {
                (__sn__selected = 30LL);
    
    __match_result__ = __sn__observeInt(&__sn__body_calls, &__sn__order, 9LL, 0LL);
            }
            __match_result__;
        });
    
    ({
            long long __match_result__;
            double __match_subject__ = __sn__observeDouble(&__sn__subject_calls, &__sn__order, 3LL, (-4.5));
            if (__match_subject__ == 4.5 || __match_subject__ == (-4.5) || __match_subject__ == (-1.0)) {
                ({
        long long *__sn_place__ = &(__sn__selected);
        long long __sn_rhs__ = 20LL;
        *__sn_place__ = sn_add_long(*__sn_place__, __sn_rhs__);
        *__sn_place__;
    });
    
    __match_result__ = __sn__observeInt(&__sn__body_calls, &__sn__order, 4LL, 0LL);
            } else if (__match_subject__ == (-4.5)) {
                (__sn__selected = 200LL);
    
    __match_result__ = __sn__observeInt(&__sn__body_calls, &__sn__order, 8LL, 0LL);
            } else {
                (__sn__selected = 300LL);
    
    __match_result__ = __sn__observeInt(&__sn__body_calls, &__sn__order, 9LL, 0LL);
            }
            __match_result__;
        });
    
    long long __sn__nested_statement = 0LL;
    ({
            float __match_subject__ = 1.0f;
            if (__match_subject__ == 1.0f) {
                ({
            double __match_subject__ = 2.0;
            if (__match_subject__ == 2.0) {
                (__sn__nested_statement = 12LL);
    
    
            }
        });
    
    
            }
        });
    
    long long __sn__nan_statement_hits = 0LL;
    ({
            float __match_subject__ = sn_div_float(0.0f, 0.0f);
            if (__match_subject__ == 0.0f) {
                ({
        long long *__sn_place__ = &(__sn__nan_statement_hits);
        long long __sn_previous__ = *__sn_place__;
        *__sn_place__ = sn_add_long(__sn_previous__, 1);
        __sn_previous__;
    });
    
    
            }
        });
    
    long long __sn__zero_hits = 0LL;
    ({
            float __match_subject__ = (-0.0f);
            if (__match_subject__ == 0.0f) {
                ({
        long long *__sn_place__ = &(__sn__zero_hits);
        long long __sn_previous__ = *__sn_place__;
        *__sn_place__ = sn_add_long(__sn_previous__, 1);
        __sn_previous__;
    });
    
    
            }
        });
    
    ({
            double __match_subject__ = 0.0;
            if (__match_subject__ == (-0.0)) {
                ({
        long long *__sn_place__ = &(__sn__zero_hits);
        long long __sn_previous__ = *__sn_place__;
        *__sn_place__ = sn_add_long(__sn_previous__, 1);
        __sn_previous__;
    });
    
    
            }
        });
    
    long long __sn__first_value = ({
            long long __match_result__;
            float __match_subject__ = __sn__observeFloat(&__sn__subject_calls, &__sn__order, 5LL, (-2.5f));
            if (__match_subject__ == (-2.5f) || __match_subject__ == (-1.0f)) {
                __match_result__ = __sn__observeInt(&__sn__body_calls, &__sn__order, 6LL, 100LL);
            } else if (__match_subject__ == (-2.5f)) {
                __match_result__ = __sn__observeInt(&__sn__body_calls, &__sn__order, 8LL, 200LL);
            } else {
                __match_result__ = __sn__observeInt(&__sn__body_calls, &__sn__order, 9LL, 300LL);
            }
            __match_result__;
        });
    bool __sn__nan_value = ({
            bool __match_result__;
            double __match_subject__ = sn_div_double(0.0, 0.0);
            if (__match_subject__ == 0.0) {
                __match_result__ = false;
            } else {
                __match_result__ = true;
            }
            __match_result__;
        });
    float __sn__float_value = ({
            float __match_result__;
            float __match_subject__ = 7.25f;
            if (__match_subject__ == 7.25f) {
                __match_result__ = (-3.5f);
            } else {
                __match_result__ = 0.0f;
            }
            __match_result__;
        });
    double __sn__double_value = ({
            double __match_result__;
            double __match_subject__ = (-6.5);
            if (__match_subject__ == (-6.5) || __match_subject__ == 1.0) {
                __match_result__ = 6.75;
            } else {
                __match_result__ = 0.0;
            }
            __match_result__;
        });
    double __sn__returned = __sn__chooseDouble(&__sn__subject_calls, &__sn__body_calls, &__sn__order);
    long long __sn__nested_value = ({
            long long __match_result__;
            float __match_subject__ = (-1.0f);
            if (__match_subject__ == (-1.0f)) {
                __match_result__ = ({
            long long __match_result__;
            double __match_subject__ = 2.0;
            if (__match_subject__ == 2.0) {
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
            sn_auto_str char *__is_p6__ = sn_str_fmt("%lld", (long long)(__sn__body_calls));
            sn_auto_str char *__is_p7__ = sn_strdup(",");
            sn_auto_str char *__is_p8__ = sn_str_fmt("%lld", (long long)(__sn__order));
            sn_auto_str char *__is_p9__ = sn_strdup(",");
            sn_auto_str char *__is_p10__ = sn_str_fmt("%lld", (long long)(__sn__selected));
            sn_auto_str char *__is_p11__ = sn_strdup(",");
            sn_auto_str char *__is_p12__ = sn_str_fmt("%lld", (long long)(__sn__nested_statement));
            sn_auto_str char *__is_p13__ = sn_strdup(",");
            sn_auto_str char *__is_p14__ = sn_str_fmt("%lld", (long long)(__sn__nan_statement_hits));
            sn_auto_str char *__is_p15__ = sn_strdup(",");
            sn_auto_str char *__is_p16__ = sn_str_fmt("%lld", (long long)(__sn__zero_hits));
            sn_auto_str char *__is_p17__ = sn_strdup(",");
            sn_auto_str char *__is_p18__ = sn_str_fmt("%lld", (long long)(__sn__first_value));
            sn_auto_str char *__is_p19__ = sn_strdup(",");
            sn_auto_str char *__is_p20__ = sn_strdup((__sn__nan_value) ? "true" : "false");
            sn_auto_str char *__is_p21__ = sn_strdup(",");
            sn_auto_str char *__is_p22__ = sn_strdup(((__sn__float_value == (-3.5f))) ? "true" : "false");
            sn_auto_str char *__is_p23__ = sn_strdup(",");
            sn_auto_str char *__is_p24__ = sn_strdup(((__sn__double_value == 6.75)) ? "true" : "false");
            sn_auto_str char *__is_p25__ = sn_strdup(",");
            sn_auto_str char *__is_p26__ = sn_strdup(((__sn__returned == 42.25)) ? "true" : "false");
            sn_auto_str char *__is_p27__ = sn_strdup(",");
            sn_auto_str char *__is_p28__ = sn_str_fmt("%lld", (long long)(__sn__nested_value));
            sn_str_concat_multi(29, __is_p0__, __is_p1__, __is_p2__, __is_p3__, __is_p4__, __is_p5__, __is_p6__, __is_p7__, __is_p8__, __is_p9__, __is_p10__, __is_p11__, __is_p12__, __is_p13__, __is_p14__, __is_p15__, __is_p16__, __is_p17__, __is_p18__, __is_p19__, __is_p20__, __is_p21__, __is_p22__, __is_p23__, __is_p24__, __is_p25__, __is_p26__, __is_p27__, __is_p28__);
        }); sn_println(__ps__); };
    
    fflush(stdout);
    return 0;
}
