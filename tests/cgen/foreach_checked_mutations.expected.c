#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>
#include "sn_minimal.h"

long long __sn__rhs(long long *);
typedef struct __Closure__ {
    void *fn;
    size_t size;
    void (*__cleanup__)(void *);
    int __rc__;
} __Closure__;


long long __sn__rhs(long long *__sn__calls) {

    ({
        long long __sn_rhs__ = 1LL;
        long long *__sn_place__ = &((*__sn__calls));
        *__sn_place__ = sn_add_long(*__sn_place__, __sn_rhs__);
        *__sn_place__;
    });
    

    return 2LL;}

int main() {
    sn_auto_arr SnArray * __sn__values = ({
            SnArray *__al__ = sn_array_new(sizeof(long long), 1);
            __al__->elem_tag = SN_TAG_INT;
    
    
            sn_array_push(__al__, &(long long){ 8LL });
            __al__;
        });
    long long __sn__calls = 0LL;
    {
        sn_auto_arr SnArray *__arr_0__ = sn_array_copy(__sn__values);
        long long __len_0__ = __arr_0__->len;
        for (long long __idx_0__ = 0; __idx_0__ < __len_0__; __idx_0__++) {
            long long __sn__value__source = ((long long *)__arr_0__->data)[__idx_0__];
            long long __sn__value = __sn__value__source;
            {
                long long __sn__added = ({
                    long long *__sn_place__ = &(__sn__value);
                    long long __sn_rhs__ = __sn__rhs(&__sn__calls);
                    *__sn_place__ = sn_add_long(*__sn_place__, __sn_rhs__);
                    *__sn_place__;
                });
                long long __sn__divided = ({
                    long long *__sn_place__ = &(__sn__value);
                    long long __sn_rhs__ = __sn__rhs(&__sn__calls);
                    *__sn_place__ = sn_div_long(*__sn_place__, __sn_rhs__);
                    *__sn_place__;
                });
                long long __sn__before_inc = ({
                    long long *__sn_place__ = &(__sn__value);
                    long long __sn_previous__ = *__sn_place__;
                    *__sn_place__ = sn_add_long(__sn_previous__, 1);
                    __sn_previous__;
                });
                long long __sn__before_dec = ({
                    long long *__sn_place__ = &(__sn__value);
                    long long __sn_previous__ = *__sn_place__;
                    *__sn_place__ = sn_sub_long(__sn_previous__, 1);
                    __sn_previous__;
                });
                { sn_auto_str char *__ps__ = ({
                        sn_auto_str char *__is_p0__ = sn_str_fmt("%lld", (long long)(__sn__added));
                        sn_auto_str char *__is_p1__ = sn_strdup(" ");
                        sn_auto_str char *__is_p2__ = sn_str_fmt("%lld", (long long)(__sn__divided));
                        sn_auto_str char *__is_p3__ = sn_strdup(" ");
                        sn_auto_str char *__is_p4__ = sn_str_fmt("%lld", (long long)(__sn__before_inc));
                        sn_auto_str char *__is_p5__ = sn_strdup(" ");
                        sn_auto_str char *__is_p6__ = sn_str_fmt("%lld", (long long)(__sn__before_dec));
                        sn_auto_str char *__is_p7__ = sn_strdup(" ");
                        sn_auto_str char *__is_p8__ = sn_str_fmt("%lld", (long long)(__sn__value));
                        sn_auto_str char *__is_p9__ = sn_strdup(" ");
                        sn_auto_str char *__is_p10__ = sn_str_fmt("%lld", (long long)(__sn__calls));
                        sn_auto_str char *__is_p11__ = sn_strdup(" ");
                        sn_auto_str char *__is_p12__ = sn_str_fmt("%lld", (long long)((((long long *)__sn__values->data)[({ long long __ai__ = 0LL; __ai__ < 0 ? __ai__ + __sn__values->len : __ai__; })])));
                        sn_str_concat_multi(13, __is_p0__, __is_p1__, __is_p2__, __is_p3__, __is_p4__, __is_p5__, __is_p6__, __is_p7__, __is_p8__, __is_p9__, __is_p10__, __is_p11__, __is_p12__);
                    }); sn_println(__ps__); };
                
            }
        }
    }
    fflush(stdout);
    return 0;
}
