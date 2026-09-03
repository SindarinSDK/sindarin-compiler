#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>
#include "sn_minimal.h"

long long __sn__nextSubject(long long *);
typedef struct __Closure__ {
    void *fn;
    size_t size;
    void (*__cleanup__)(void *);
    int __rc__;
} __Closure__;


long long __sn__nextSubject(long long *__sn__calls) {

    ({
        long long *__sn_place__ = &((*__sn__calls));
        long long __sn_previous__ = *__sn_place__;
        *__sn_place__ = sn_add_long(__sn_previous__, 1);
        __sn_previous__;
    });
    

    return 2LL;}

int main() {
    long long __sn____sn_match_subject = 2LL;
    long long __sn__subject_calls = 0LL;
    long long __sn__selected = 0LL;
    long long __sn__effects = 0LL;
    ({
            long long __match_result__;
            long long __match_subject__ = __sn__nextSubject(&__sn__subject_calls);
            if (__match_subject__ == 1LL || __match_subject__ == 2LL) {
                (__sn__selected = 10LL);
    
    __match_result__ = ({
        long long *__sn_place__ = &(__sn__effects);
        long long __sn_previous__ = *__sn_place__;
        *__sn_place__ = sn_add_long(__sn_previous__, 1);
        __sn_previous__;
    });
            } else if (__match_subject__ == 2LL || __match_subject__ == 3LL) {
                (__sn__selected = 20LL);
    
    __match_result__ = ({
        long long *__sn_place__ = &(__sn__effects);
        long long __sn_rhs__ = 100LL;
        *__sn_place__ = sn_add_long(*__sn_place__, __sn_rhs__);
        *__sn_place__;
    });
            } else {
                (__sn__selected = 30LL);
    
    __match_result__ = ({
        long long *__sn_place__ = &(__sn__effects);
        long long __sn_rhs__ = 1000LL;
        *__sn_place__ = sn_add_long(*__sn_place__, __sn_rhs__);
        *__sn_place__;
    });
            }
            __match_result__;
        });
    
    long long __sn__fallback = 0LL;
    ({
            long long __match_result__;
            long long __match_subject__ = 99LL;
            if (__match_subject__ == 1LL || __match_subject__ == 2LL) {
                __match_result__ = (__sn__fallback = 1LL);
            } else {
                __match_result__ = (__sn__fallback = 7LL);
            }
            __match_result__;
        });
    
    long long __sn__unchanged = 11LL;
    ({
            long long __match_subject__ = 42LL;
            if (__match_subject__ == 1LL || __match_subject__ == 2LL) {
                (__sn__unchanged = 99LL);
    
    
            }
        });
    
    long long __sn__negative = 0LL;
    ({
            long long __match_result__;
            long long __match_subject__ = (-7LL);
            if (__match_subject__ == (-8LL) || __match_subject__ == (-7LL)) {
                __match_result__ = (__sn__negative = 1LL);
            } else {
                __match_result__ = (__sn__negative = 2LL);
            }
            __match_result__;
        });
    
    long long __sn__parser_extremes = 0LL;
    ({
            long long __match_result__;
            long long __match_subject__ = (-9223372036854775807LL);
            if (__match_subject__ == (-9223372036854775807LL) || __match_subject__ == 9223372036854775807LL) {
                __match_result__ = ({
        long long *__sn_place__ = &(__sn__parser_extremes);
        long long __sn_rhs__ = 1LL;
        *__sn_place__ = sn_add_long(*__sn_place__, __sn_rhs__);
        *__sn_place__;
    });
            } else {
                __match_result__ = (__sn__parser_extremes = 100LL);
            }
            __match_result__;
        });
    
    ({
            long long __match_result__;
            long long __match_subject__ = 9223372036854775807LL;
            if (__match_subject__ == (-9223372036854775807LL) || __match_subject__ == 9223372036854775807LL) {
                __match_result__ = ({
        long long *__sn_place__ = &(__sn__parser_extremes);
        long long __sn_rhs__ = 10LL;
        *__sn_place__ = sn_add_long(*__sn_place__, __sn_rhs__);
        *__sn_place__;
    });
            } else {
                __match_result__ = (__sn__parser_extremes = 200LL);
            }
            __match_result__;
        });
    
    long long __sn__nested = 0LL;
    ({
            long long __match_subject__ = 1LL;
            if (__match_subject__ == 1LL) {
                ({
            long long __match_subject__ = 2LL;
            if (__match_subject__ == 2LL) {
                (__sn__nested = 5LL);
    
    
            }
        });
    
    
            }
        });
    
    { sn_auto_str char *__ps__ = ({
            sn_auto_str char *__is_p0__ = sn_str_fmt("%lld", (long long)(__sn____sn_match_subject));
            sn_auto_str char *__is_p1__ = sn_strdup(",");
            sn_auto_str char *__is_p2__ = sn_str_fmt("%lld", (long long)(__sn__subject_calls));
            sn_auto_str char *__is_p3__ = sn_strdup(",");
            sn_auto_str char *__is_p4__ = sn_str_fmt("%lld", (long long)(__sn__selected));
            sn_auto_str char *__is_p5__ = sn_strdup(",");
            sn_auto_str char *__is_p6__ = sn_str_fmt("%lld", (long long)(__sn__effects));
            sn_auto_str char *__is_p7__ = sn_strdup(",");
            sn_auto_str char *__is_p8__ = sn_str_fmt("%lld", (long long)(__sn__fallback));
            sn_auto_str char *__is_p9__ = sn_strdup(",");
            sn_auto_str char *__is_p10__ = sn_str_fmt("%lld", (long long)(__sn__unchanged));
            sn_auto_str char *__is_p11__ = sn_strdup(",");
            sn_auto_str char *__is_p12__ = sn_str_fmt("%lld", (long long)(__sn__negative));
            sn_auto_str char *__is_p13__ = sn_strdup(",");
            sn_auto_str char *__is_p14__ = sn_str_fmt("%lld", (long long)(__sn__parser_extremes));
            sn_auto_str char *__is_p15__ = sn_strdup(",");
            sn_auto_str char *__is_p16__ = sn_str_fmt("%lld", (long long)(__sn__nested));
            sn_str_concat_multi(17, __is_p0__, __is_p1__, __is_p2__, __is_p3__, __is_p4__, __is_p5__, __is_p6__, __is_p7__, __is_p8__, __is_p9__, __is_p10__, __is_p11__, __is_p12__, __is_p13__, __is_p14__, __is_p15__, __is_p16__);
        }); sn_println(__ps__); };
    
    fflush(stdout);
    return 0;
}
