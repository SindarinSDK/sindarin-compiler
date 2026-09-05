#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>
#include "sn_minimal.h"

long long __sn__observeSubject(long long *, long long);
long long __sn__observeResult(long long *, long long);
long long __sn__choose(long long, long long *, long long *);
typedef struct __Closure__ {
    void *fn;
    size_t size;
    void (*__cleanup__)(void *);
    int __rc__;
} __Closure__;


long long __sn__observeSubject(long long *__sn__calls, long long __sn__value) {

    ({
        long long *__sn_place__ = &((*__sn__calls));
        long long __sn_previous__ = *__sn_place__;
        *__sn_place__ = sn_add_long(__sn_previous__, 1);
        __sn_previous__;
    });
    

    return __sn__value;}


long long __sn__observeResult(long long *__sn__calls, long long __sn__value) {

    ({
        long long *__sn_place__ = &((*__sn__calls));
        long long __sn_previous__ = *__sn_place__;
        *__sn_place__ = sn_add_long(__sn_previous__, 1);
        __sn_previous__;
    });
    

    return __sn__value;}


long long __sn__choose(long long __sn__value, long long *__sn__subject_calls, long long *__sn__result_calls) {

    return ({
             long long __match_result__;
             long long __match_subject__ = __sn__observeSubject(&(*__sn__subject_calls), __sn__value);
             if (__match_subject__ == 1LL || __match_subject__ == 2LL) {
                 __match_result__ = __sn__observeResult(&(*__sn__result_calls), 10LL);
             } else if (__match_subject__ == 2LL || __match_subject__ == 3LL) {
                 __match_result__ = __sn__observeResult(&(*__sn__result_calls), 20LL);
             } else {
                 __match_result__ = __sn__observeResult(&(*__sn__result_calls), 30LL);
             }
             __match_result__;
         });}

int main() {
    long long __sn____sn_match_result = 41LL;
    long long __sn__subject_calls = 0LL;
    long long __sn__result_calls = 0LL;
    long long __sn__first = ({
            long long __match_result__;
            long long __match_subject__ = __sn__observeSubject(&__sn__subject_calls, 2LL);
            if (__match_subject__ == 1LL || __match_subject__ == 2LL) {
                __match_result__ = __sn__observeResult(&__sn__result_calls, 10LL);
            } else if (__match_subject__ == 2LL || __match_subject__ == 3LL) {
                __match_result__ = __sn__observeResult(&__sn__result_calls, 20LL);
            } else {
                __match_result__ = __sn__observeResult(&__sn__result_calls, 30LL);
            }
            __match_result__;
        });
    long long __sn__fallback = ({
            long long __match_result__;
            long long __match_subject__ = __sn__observeSubject(&__sn__subject_calls, 99LL);
            if (__match_subject__ == 1LL || __match_subject__ == 2LL) {
                __match_result__ = __sn__observeResult(&__sn__result_calls, 10LL);
            } else if (__match_subject__ == 2LL || __match_subject__ == 3LL) {
                __match_result__ = __sn__observeResult(&__sn__result_calls, 20LL);
            } else {
                __match_result__ = __sn__observeResult(&__sn__result_calls, 30LL);
            }
            __match_result__;
        });
    long long __sn__returned = __sn__choose(3LL, &__sn__subject_calls, &__sn__result_calls);
    long long __sn__nested = ({
            long long __match_result__;
            long long __match_subject__ = (-9223372036854775807LL);
            if (__match_subject__ == (-9223372036854775807LL) || __match_subject__ == 9223372036854775807LL) {
                __match_result__ = ({
            long long __match_result__;
            long long __match_subject__ = 9223372036854775807LL;
            if (__match_subject__ == (-9223372036854775807LL)) {
                __match_result__ = __sn__observeResult(&__sn__result_calls, 40LL);
            } else if (__match_subject__ == 9223372036854775807LL) {
                __match_result__ = __sn__observeResult(&__sn__result_calls, 50LL);
            } else {
                __match_result__ = __sn__observeResult(&__sn__result_calls, 60LL);
            }
            __match_result__;
        });
            } else {
                __match_result__ = __sn__observeResult(&__sn__result_calls, 70LL);
            }
            __match_result__;
        });
    { sn_auto_str char *__ps__ = ({
            sn_auto_str char *__is_p0__ = sn_str_fmt("%lld", (long long)(__sn____sn_match_result));
            sn_auto_str char *__is_p1__ = sn_strdup(",");
            sn_auto_str char *__is_p2__ = sn_str_fmt("%lld", (long long)(__sn__subject_calls));
            sn_auto_str char *__is_p3__ = sn_strdup(",");
            sn_auto_str char *__is_p4__ = sn_str_fmt("%lld", (long long)(__sn__result_calls));
            sn_auto_str char *__is_p5__ = sn_strdup(",");
            sn_auto_str char *__is_p6__ = sn_str_fmt("%lld", (long long)(__sn__first));
            sn_auto_str char *__is_p7__ = sn_strdup(",");
            sn_auto_str char *__is_p8__ = sn_str_fmt("%lld", (long long)(__sn__fallback));
            sn_auto_str char *__is_p9__ = sn_strdup(",");
            sn_auto_str char *__is_p10__ = sn_str_fmt("%lld", (long long)(__sn__returned));
            sn_auto_str char *__is_p11__ = sn_strdup(",");
            sn_auto_str char *__is_p12__ = sn_str_fmt("%lld", (long long)(__sn__nested));
            sn_str_concat_multi(13, __is_p0__, __is_p1__, __is_p2__, __is_p3__, __is_p4__, __is_p5__, __is_p6__, __is_p7__, __is_p8__, __is_p9__, __is_p10__, __is_p11__, __is_p12__);
        }); sn_println(__ps__); };
    
    fflush(stdout);
    return 0;
}
