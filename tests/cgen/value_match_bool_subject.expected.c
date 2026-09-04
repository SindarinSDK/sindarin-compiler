#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>
#include "sn_minimal.h"

bool __sn__observeSubject(long long *, long long *, bool);
bool __sn__observeResult(long long *, long long *, long long, bool);
bool __sn__accept(bool);
bool __sn__choose(bool, long long *, long long *, long long *);
typedef struct __Closure__ {
    void *fn;
    size_t size;
    void (*__cleanup__)(void *);
    int __rc__;
} __Closure__;


bool __sn__observeSubject(long long *__sn__calls, long long *__sn__order, bool __sn__value) {

    ({
        long long *__sn_place__ = &((*__sn__calls));
        long long __sn_previous__ = *__sn_place__;
        *__sn_place__ = sn_add_long(__sn_previous__, 1);
        __sn_previous__;
    });
    

    (*__sn__order = sn_add_long(sn_mul_long((*__sn__order), 10LL), 1LL));
    

    return __sn__value;}


bool __sn__observeResult(long long *__sn__calls, long long *__sn__order, long long __sn__marker, bool __sn__value) {

    ({
        long long *__sn_place__ = &((*__sn__calls));
        long long __sn_previous__ = *__sn_place__;
        *__sn_place__ = sn_add_long(__sn_previous__, 1);
        __sn_previous__;
    });
    

    (*__sn__order = sn_add_long(sn_mul_long((*__sn__order), 10LL), __sn__marker));
    

    return __sn__value;}


bool __sn__accept(bool __sn__value) {

    return __sn__value;}


bool __sn__choose(bool __sn__value, long long *__sn__subject_calls, long long *__sn__result_calls, long long *__sn__order) {

    return ({
             bool __match_result__;
             bool __match_subject__ = __sn__observeSubject(&(*__sn__subject_calls), &(*__sn__order), __sn__value);
             if (__match_subject__ == true) {
                 __match_result__ = __sn__observeResult(&(*__sn__result_calls), &(*__sn__order), 2LL, true);
             } else if (__match_subject__ == false) {
                 __match_result__ = __sn__observeResult(&(*__sn__result_calls), &(*__sn__order), 3LL, false);
             } else {
                 __match_result__ = __sn__observeResult(&(*__sn__result_calls), &(*__sn__order), 4LL, true);
             }
             __match_result__;
         });}

int main() {
    bool __sn____sn_match_result = true;
    long long __sn__subject_calls = 0LL;
    long long __sn__result_calls = 0LL;
    long long __sn__order = 0LL;
    bool __sn__first = ({
            bool __match_result__;
            bool __match_subject__ = __sn__observeSubject(&__sn__subject_calls, &__sn__order, true);
            if (__match_subject__ == true || __match_subject__ == false) {
                __match_result__ = __sn__observeResult(&__sn__result_calls, &__sn__order, 2LL, true);
            } else if (__match_subject__ == true) {
                __match_result__ = __sn__observeResult(&__sn__result_calls, &__sn__order, 3LL, false);
            } else {
                __match_result__ = __sn__observeResult(&__sn__result_calls, &__sn__order, 4LL, false);
            }
            __match_result__;
        });
    bool __sn__fallback = ({
            bool __match_result__;
            bool __match_subject__ = __sn__observeSubject(&__sn__subject_calls, &__sn__order, false);
            if (__match_subject__ == true) {
                __match_result__ = __sn__observeResult(&__sn__result_calls, &__sn__order, 5LL, false);
            } else {
                __match_result__ = __sn__observeResult(&__sn__result_calls, &__sn__order, 6LL, true);
            }
            __match_result__;
        });
    bool __sn__returned = __sn__choose(false, &__sn__subject_calls, &__sn__result_calls, &__sn__order);
    bool __sn__argument = ({
            bool __match_result__;
            bool __match_subject__ = true;
            if (__match_subject__ == true) {
                __match_result__ = __sn__accept(__sn__observeResult(&__sn__result_calls, &__sn__order, 7LL, false));
            } else {
                __match_result__ = __sn__accept(__sn__observeResult(&__sn__result_calls, &__sn__order, 8LL, true));
            }
            __match_result__;
        });
    bool __sn__nested = ({
            bool __match_result__;
            bool __match_subject__ = true;
            if (__match_subject__ == true) {
                __match_result__ = ({
            bool __match_result__;
            bool __match_subject__ = false;
            if (__match_subject__ == true) {
                __match_result__ = __sn__observeResult(&__sn__result_calls, &__sn__order, 9LL, false);
            } else {
                __match_result__ = __sn__observeResult(&__sn__result_calls, &__sn__order, 4LL, true);
            }
            __match_result__;
        });
            } else {
                __match_result__ = __sn__observeResult(&__sn__result_calls, &__sn__order, 5LL, false);
            }
            __match_result__;
        });
    { sn_auto_str char *__ps__ = ({
            sn_auto_str char *__is_p0__ = sn_strdup((__sn____sn_match_result) ? "true" : "false");
            sn_auto_str char *__is_p1__ = sn_strdup(",");
            sn_auto_str char *__is_p2__ = sn_str_fmt("%lld", (long long)(__sn__subject_calls));
            sn_auto_str char *__is_p3__ = sn_strdup(",");
            sn_auto_str char *__is_p4__ = sn_str_fmt("%lld", (long long)(__sn__result_calls));
            sn_auto_str char *__is_p5__ = sn_strdup(",");
            sn_auto_str char *__is_p6__ = sn_str_fmt("%lld", (long long)(__sn__order));
            sn_auto_str char *__is_p7__ = sn_strdup(",");
            sn_auto_str char *__is_p8__ = sn_strdup((__sn__first) ? "true" : "false");
            sn_auto_str char *__is_p9__ = sn_strdup(",");
            sn_auto_str char *__is_p10__ = sn_strdup((__sn__fallback) ? "true" : "false");
            sn_auto_str char *__is_p11__ = sn_strdup(",");
            sn_auto_str char *__is_p12__ = sn_strdup((__sn__returned) ? "true" : "false");
            sn_auto_str char *__is_p13__ = sn_strdup(",");
            sn_auto_str char *__is_p14__ = sn_strdup((__sn__argument) ? "true" : "false");
            sn_auto_str char *__is_p15__ = sn_strdup(",");
            sn_auto_str char *__is_p16__ = sn_strdup((__sn__nested) ? "true" : "false");
            sn_str_concat_multi(17, __is_p0__, __is_p1__, __is_p2__, __is_p3__, __is_p4__, __is_p5__, __is_p6__, __is_p7__, __is_p8__, __is_p9__, __is_p10__, __is_p11__, __is_p12__, __is_p13__, __is_p14__, __is_p15__, __is_p16__);
        }); sn_println(__ps__); };
    
    fflush(stdout);
    return 0;
}
