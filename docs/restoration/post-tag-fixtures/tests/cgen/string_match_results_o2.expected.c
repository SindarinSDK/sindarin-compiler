#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>
#include "sn_minimal.h"

char * __sn__ownedResult(char *);
typedef struct __Closure__ {
    void *fn;
    size_t size;
    void (*__cleanup__)(void *);
    int __rc__;
} __Closure__;


char * __sn__ownedResult(char * __sn__value) {

    return ({
             sn_auto_str char *__is_p0__ = sn_strdup("<");
             sn_auto_str char *__is_p1__ = sn_strdup(__sn__value);
             sn_auto_str char *__is_p2__ = sn_strdup(">");
             sn_str_concat_multi(3, __is_p0__, __is_p1__, __is_p2__);
         });}

int main() {
    sn_auto_str char * __sn__source = strdup("borrowed\n\tquote:\" slash:\\");
    char * __sn____chain_tmp_0 = "indexed\n\tquote:\" slash:\\";
    sn_auto_arr SnArray * __sn__rows = __sn___split(&__sn____chain_tmp_0, "never-present");
    sn_auto_str char * __sn__direct = ({
            char * __match_result__;
            bool __match_subject__ = true;
            if (__match_subject__ == true) {
                __match_result__ = strdup("direct\n\tquote:\" slash:\\");
            } else {
                __match_result__ = strdup("wrong");
            }
            __match_result__;
        });
    sn_auto_str char * __sn__borrowed = ({
            char * __match_result__;
            long long __match_subject__ = 1LL;
            if (__match_subject__ == 1LL) {
                __match_result__ = strdup(__sn__source);
            } else {
                __match_result__ = strdup("wrong");
            }
            __match_result__;
        });
    sn_auto_str char * __sn__indexed = ({
            char * __match_result__;
            bool __match_subject__ = false;
            if (__match_subject__ == true) {
                __match_result__ = strdup("wrong");
            } else {
                __match_result__ = strdup((((char * *)__sn__rows->data)[({ long long __ai__ = 0LL; __ai__ < 0 ? __ai__ + __sn__rows->len : __ai__; })]));
            }
            __match_result__;
        });
    sn_auto_str char * __sn__concatenated = ({
            char * __match_result__;
            long long __match_subject__ = 2LL;
            if (__match_subject__ == 2LL) {
                __match_result__ = sn_str_concat("owned-", __sn__source);
            } else {
                __match_result__ = strdup("wrong");
            }
            __match_result__;
        });
    sn_auto_str char * __sn__called = ({
            char * __match_result__;
            long long __match_subject__ = 3LL;
            if (__match_subject__ == 3LL) {
                __match_result__ = __sn__ownedResult("called");
            } else {
                __match_result__ = strdup("wrong");
            }
            __match_result__;
        });
    sn_auto_str char * __sn__nested = ({
            char * __match_result__;
            long long __match_subject__ = 4LL;
            if (__match_subject__ == 4LL) {
                __match_result__ = ({
            char * __match_result__;
            char * __match_subject__ = "inner";
            if (strcmp(__match_subject__, "inner") == 0) {
                __match_result__ = strdup("nested\n\tquote:\" slash:\\");
            } else {
                __match_result__ = strdup("wrong-inner");
            }
            __match_result__;
        });
            } else {
                __match_result__ = strdup("wrong-outer");
            }
            __match_result__;
        });
    bool __sn__ok = ((((((strcmp(__sn__direct, "direct\n\tquote:\" slash:\\") == 0) && (strcmp(__sn__borrowed, "borrowed\n\tquote:\" slash:\\") == 0)) && (strcmp(__sn__indexed, "indexed\n\tquote:\" slash:\\") == 0)) && (strcmp(__sn__concatenated, "owned-borrowed\n\tquote:\" slash:\\") == 0)) && (strcmp(__sn__called, "<called>") == 0)) && (strcmp(__sn__nested, "nested\n\tquote:\" slash:\\") == 0));
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
