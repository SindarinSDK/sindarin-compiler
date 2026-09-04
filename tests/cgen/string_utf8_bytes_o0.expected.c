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
    sn_auto_str char * __sn__boundaries = strdup("߿ࠀ퟿￿𐀀􏿿");
    sn_auto_arr SnArray * __sn__boundaryBytes = __sn___toBytes(&__sn__boundaries);
    sn_auto_str char * __sn__controls = strdup("\n\t\r\"\\");
    sn_auto_arr SnArray * __sn__controlBytes = __sn___toBytes(&__sn__controls);
    sn_auto_str char * __sn__unicode = strdup("é世界🙂");
    sn_auto_arr SnArray * __sn__unicodeBytes = __sn___toBytes(&__sn__unicode);
    sn_auto_str char * __sn__ascii = strdup("ASCII");
    sn_auto_str char * __sn__accent = strdup("é");
    sn_auto_str char * __sn__world = strdup("世界");
    sn_auto_str char * __sn__emoji = strdup("🙂");
    sn_auto_str char * __sn__decomposed = strdup("é");
    sn_auto_str char * __sn__source = strdup("X\x1f\x41\x62\x30\x39Y");
    char * __sn____chain_tmp_0 = "X\x1f\x41\x62\x30\x39Y";
    sn_auto_arr SnArray * __sn__rows = __sn___split(&__sn____chain_tmp_0, "never-present");
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
            long long __match_subject__ = 2LL;
            if (__match_subject__ == 2LL) {
                __match_result__ = strdup((((char * *)__sn__rows->data)[({ long long __ai__ = 0LL; __ai__ < 0 ? __ai__ + __sn__rows->len : __ai__; })]));
            } else {
                __match_result__ = strdup("wrong");
            }
            __match_result__;
        });
    sn_auto_str char * __sn__nested = ({
            char * __match_result__;
            bool __match_subject__ = true;
            if (__match_subject__ == true) {
                __match_result__ = ({
            char * __match_result__;
            char * __match_subject__ = "X\x1f\x41Y";
            if (strcmp(__match_subject__, "X\x1f\x41Y") == 0) {
                __match_result__ = strdup("X\x1f\x41\x62\x30\x39Y");
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
    sn_auto_str char * __sn__concatenated = sn_str_concat("X\x1f", "Ab09Y");
    sn_auto_str char * __sn__interpolated = ({
            sn_auto_str char *__is_p0__ = sn_strdup("X\x1f\x41\x62\x30\x39Y");
            sn_str_concat_multi(1, __is_p0__);
        });
    bool __sn__ok = ((((((((((((((((((((((((((((((((((((((((((sn_array_length(__sn__directBytes) == 4LL) && ((((unsigned char *)__sn__directBytes->data)[({ long long __ai__ = 0LL; __ai__ < 0 ? __ai__ + __sn__directBytes->len : __ai__; })]) == (unsigned char)88)) && ((((unsigned char *)__sn__directBytes->data)[({ long long __ai__ = 1LL; __ai__ < 0 ? __ai__ + __sn__directBytes->len : __ai__; })]) == (unsigned char)31)) && ((((unsigned char *)__sn__directBytes->data)[({ long long __ai__ = 2LL; __ai__ < 0 ? __ai__ + __sn__directBytes->len : __ai__; })]) == (unsigned char)65)) && ((((unsigned char *)__sn__directBytes->data)[({ long long __ai__ = 3LL; __ai__ < 0 ? __ai__ + __sn__directBytes->len : __ai__; })]) == (unsigned char)89)) && (sn_array_length(__sn__longBytes) == 7LL)) && ((((unsigned char *)__sn__longBytes->data)[({ long long __ai__ = 0LL; __ai__ < 0 ? __ai__ + __sn__longBytes->len : __ai__; })]) == (unsigned char)88)) && ((((unsigned char *)__sn__longBytes->data)[({ long long __ai__ = 1LL; __ai__ < 0 ? __ai__ + __sn__longBytes->len : __ai__; })]) == (unsigned char)31)) && ((((unsigned char *)__sn__longBytes->data)[({ long long __ai__ = 2LL; __ai__ < 0 ? __ai__ + __sn__longBytes->len : __ai__; })]) == (unsigned char)65)) && ((((unsigned char *)__sn__longBytes->data)[({ long long __ai__ = 3LL; __ai__ < 0 ? __ai__ + __sn__longBytes->len : __ai__; })]) == (unsigned char)98)) && ((((unsigned char *)__sn__longBytes->data)[({ long long __ai__ = 4LL; __ai__ < 0 ? __ai__ + __sn__longBytes->len : __ai__; })]) == (unsigned char)48)) && ((((unsigned char *)__sn__longBytes->data)[({ long long __ai__ = 5LL; __ai__ < 0 ? __ai__ + __sn__longBytes->len : __ai__; })]) == (unsigned char)57)) && ((((unsigned char *)__sn__longBytes->data)[({ long long __ai__ = 6LL; __ai__ < 0 ? __ai__ + __sn__longBytes->len : __ai__; })]) == (unsigned char)89)) && (sn_array_length(__sn__lowerBytes) == 4LL)) && ((((unsigned char *)__sn__lowerBytes->data)[({ long long __ai__ = 1LL; __ai__ < 0 ? __ai__ + __sn__lowerBytes->len : __ai__; })]) == (unsigned char)31)) && ((((unsigned char *)__sn__lowerBytes->data)[({ long long __ai__ = 2LL; __ai__ < 0 ? __ai__ + __sn__lowerBytes->len : __ai__; })]) == (unsigned char)97)) && (sn_array_length(__sn__boundaryBytes) == 24LL)) && ((((unsigned char *)__sn__boundaryBytes->data)[({ long long __ai__ = 0LL; __ai__ < 0 ? __ai__ + __sn__boundaryBytes->len : __ai__; })]) == (unsigned char)194)) && ((((unsigned char *)__sn__boundaryBytes->data)[({ long long __ai__ = 1LL; __ai__ < 0 ? __ai__ + __sn__boundaryBytes->len : __ai__; })]) == (unsigned char)128)) && ((((unsigned char *)__sn__boundaryBytes->data)[({ long long __ai__ = 20LL; __ai__ < 0 ? __ai__ + __sn__boundaryBytes->len : __ai__; })]) == (unsigned char)244)) && ((((unsigned char *)__sn__boundaryBytes->data)[({ long long __ai__ = 21LL; __ai__ < 0 ? __ai__ + __sn__boundaryBytes->len : __ai__; })]) == (unsigned char)143)) && ((((unsigned char *)__sn__boundaryBytes->data)[({ long long __ai__ = 22LL; __ai__ < 0 ? __ai__ + __sn__boundaryBytes->len : __ai__; })]) == (unsigned char)191)) && ((((unsigned char *)__sn__boundaryBytes->data)[({ long long __ai__ = 23LL; __ai__ < 0 ? __ai__ + __sn__boundaryBytes->len : __ai__; })]) == (unsigned char)191)) && (sn_array_length(__sn__controlBytes) == 5LL)) && ((((unsigned char *)__sn__controlBytes->data)[({ long long __ai__ = 0LL; __ai__ < 0 ? __ai__ + __sn__controlBytes->len : __ai__; })]) == (unsigned char)10)) && ((((unsigned char *)__sn__controlBytes->data)[({ long long __ai__ = 1LL; __ai__ < 0 ? __ai__ + __sn__controlBytes->len : __ai__; })]) == (unsigned char)9)) && ((((unsigned char *)__sn__controlBytes->data)[({ long long __ai__ = 2LL; __ai__ < 0 ? __ai__ + __sn__controlBytes->len : __ai__; })]) == (unsigned char)13)) && ((((unsigned char *)__sn__controlBytes->data)[({ long long __ai__ = 3LL; __ai__ < 0 ? __ai__ + __sn__controlBytes->len : __ai__; })]) == (unsigned char)34)) && ((((unsigned char *)__sn__controlBytes->data)[({ long long __ai__ = 4LL; __ai__ < 0 ? __ai__ + __sn__controlBytes->len : __ai__; })]) == (unsigned char)92)) && (sn_array_length(__sn__unicodeBytes) == 12LL)) && ((((unsigned char *)__sn__unicodeBytes->data)[({ long long __ai__ = 0LL; __ai__ < 0 ? __ai__ + __sn__unicodeBytes->len : __ai__; })]) == (unsigned char)195)) && ((((unsigned char *)__sn__unicodeBytes->data)[({ long long __ai__ = 11LL; __ai__ < 0 ? __ai__ + __sn__unicodeBytes->len : __ai__; })]) == (unsigned char)130)) && (sn_str_length(__sn__ascii) == 5LL)) && (sn_str_length(__sn__accent) == 2LL)) && (sn_str_length(__sn__world) == 6LL)) && (sn_str_length(__sn__emoji) == 4LL)) && (sn_str_length(__sn__decomposed) == 3LL)) && (strcmp(__sn__borrowed, __sn__longGreedy) == 0)) && (strcmp(__sn__indexed, __sn__longGreedy) == 0)) && (strcmp(__sn__nested, __sn__longGreedy) == 0)) && (strcmp(__sn__concatenated, __sn__longGreedy) == 0)) && (strcmp(__sn__interpolated, __sn__longGreedy) == 0));
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
