#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>
#include "sn_minimal.h"

bool __sn__observeSubject(long long *, long long *, bool);
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

int main() {
    long long __sn__subject_calls = 0LL;
    long long __sn__order = 0LL;
    long long __sn__first = 0LL;
    ({
            long long __match_result__;
            bool __match_subject__ = __sn__observeSubject(&__sn__subject_calls, &__sn__order, true);
            if (__match_subject__ == true) {
                (__sn__order = sn_add_long(sn_mul_long(__sn__order, 10LL), 2LL));
    
    __match_result__ = (__sn__first = 10LL);
            } else if (__match_subject__ == true) {
                (__sn__order = sn_add_long(sn_mul_long(__sn__order, 10LL), 3LL));
    
    __match_result__ = (__sn__first = 20LL);
            } else {
                __match_result__ = (__sn__first = 30LL);
            }
            __match_result__;
        });
    
    bool __sn__true_hit = false;
    ({
            bool __match_subject__ = true;
            if (__match_subject__ == true) {
                (__sn__true_hit = true);
    
    
            } else if (__match_subject__ == false) {
                (__sn__true_hit = false);
    
    
            }
        });
    
    bool __sn__false_hit = false;
    ({
            bool __match_subject__ = false;
            if (__match_subject__ == true) {
                (__sn__false_hit = false);
    
    
            } else if (__match_subject__ == false) {
                (__sn__false_hit = true);
    
    
            }
        });
    
    long long __sn__fallback = 0LL;
    ({
            long long __match_result__;
            bool __match_subject__ = false;
            if (__match_subject__ == true) {
                __match_result__ = (__sn__fallback = 1LL);
            } else {
                __match_result__ = (__sn__fallback = 7LL);
            }
            __match_result__;
        });
    
    long long __sn__unchanged = 11LL;
    ({
            bool __match_subject__ = false;
            if (__match_subject__ == true) {
                (__sn__unchanged = 99LL);
    
    
            }
        });
    
    long long __sn__alternatives = 0LL;
    ({
            long long __match_result__;
            bool __match_subject__ = false;
            if (__match_subject__ == true || __match_subject__ == false) {
                __match_result__ = (__sn__alternatives = 1LL);
            } else {
                __match_result__ = (__sn__alternatives = 2LL);
            }
            __match_result__;
        });
    
    long long __sn__nested = 0LL;
    ({
            bool __match_subject__ = true;
            if (__match_subject__ == true) {
                ({
            bool __match_subject__ = false;
            if (__match_subject__ == false) {
                (__sn__nested = 5LL);
    
    
            }
        });
    
    
            }
        });
    
    { sn_auto_str char *__ps__ = ({
            sn_auto_str char *__is_p0__ = sn_str_fmt("%lld", (long long)(__sn__subject_calls));
            sn_auto_str char *__is_p1__ = sn_strdup(",");
            sn_auto_str char *__is_p2__ = sn_str_fmt("%lld", (long long)(__sn__order));
            sn_auto_str char *__is_p3__ = sn_strdup(",");
            sn_auto_str char *__is_p4__ = sn_str_fmt("%lld", (long long)(__sn__first));
            sn_auto_str char *__is_p5__ = sn_strdup(",");
            sn_auto_str char *__is_p6__ = sn_strdup((__sn__true_hit) ? "true" : "false");
            sn_auto_str char *__is_p7__ = sn_strdup(",");
            sn_auto_str char *__is_p8__ = sn_strdup((__sn__false_hit) ? "true" : "false");
            sn_auto_str char *__is_p9__ = sn_strdup(",");
            sn_auto_str char *__is_p10__ = sn_str_fmt("%lld", (long long)(__sn__fallback));
            sn_auto_str char *__is_p11__ = sn_strdup(",");
            sn_auto_str char *__is_p12__ = sn_str_fmt("%lld", (long long)(__sn__unchanged));
            sn_auto_str char *__is_p13__ = sn_strdup(",");
            sn_auto_str char *__is_p14__ = sn_str_fmt("%lld", (long long)(__sn__alternatives));
            sn_auto_str char *__is_p15__ = sn_strdup(",");
            sn_auto_str char *__is_p16__ = sn_str_fmt("%lld", (long long)(__sn__nested));
            sn_str_concat_multi(17, __is_p0__, __is_p1__, __is_p2__, __is_p3__, __is_p4__, __is_p5__, __is_p6__, __is_p7__, __is_p8__, __is_p9__, __is_p10__, __is_p11__, __is_p12__, __is_p13__, __is_p14__, __is_p15__, __is_p16__);
        }); sn_println(__ps__); };
    
    fflush(stdout);
    return 0;
}
