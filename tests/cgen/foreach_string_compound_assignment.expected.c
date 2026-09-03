#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>
#include "sn_minimal.h"

char * __sn__suffix(long long *);
typedef struct __Closure__ {
    void *fn;
    size_t size;
    void (*__cleanup__)(void *);
    int __rc__;
} __Closure__;


char * __sn__suffix(long long *__sn__calls) {

    ({
        long long __sn_rhs__ = 1LL;
        long long *__sn_place__ = &((*__sn__calls));
        *__sn_place__ = sn_add_long(*__sn_place__, __sn_rhs__);
        *__sn_place__;
    });
    

    return ({
             sn_auto_str char *__is_p0__ = sn_strdup("!");
             sn_auto_str char *__is_p1__ = sn_str_fmt("%lld", (long long)((*__sn__calls)));
             sn_str_concat_multi(2, __is_p0__, __is_p1__);
         });}

int main() {
    sn_auto_arr SnArray * __sn__names = ({
            SnArray *__al__ = sn_array_new(sizeof(char *), 3);
            __al__->elem_tag = SN_TAG_STRING;
    
            __al__->elem_release = (void (*)(void *))sn_cleanup_str;
    
            __al__->elem_copy = sn_copy_str;
    
            sn_array_push(__al__, &(char *){ strdup("alpha") });
    
            sn_array_push(__al__, &(char *){ strdup("beta") });
    
            sn_array_push(__al__, &(char *){ strdup("gamma") });
            __al__;
        });
    long long __sn__calls = 0LL;
    long long __sn__index = 0LL;
    {
        sn_auto_arr SnArray *__arr_0__ = sn_array_copy(__sn__names);
        long long __len_0__ = __arr_0__->len;
        for (long long __idx_0__ = 0; __idx_0__ < __len_0__; __idx_0__++) {
            char * __sn____sn_string_place__source = ((char * *)__arr_0__->data)[__idx_0__];
            sn_auto_str char * __sn____sn_string_place = __sn____sn_string_place__source ? strdup(__sn____sn_string_place__source) : NULL;
            {
                sn_auto_str char * __sn__appended = strdup(({
                    char *__sct__ = __sn__suffix(&__sn__calls);
                    char *__old__ = __sn____sn_string_place;
                    __sn____sn_string_place = sn_str_concat(__sn____sn_string_place, __sct__);
                    free(__old__);
                    free(__sct__);
                    __sn____sn_string_place;
                }));
                { sn_auto_str char *__ps__ = ({
                        sn_auto_str char *__is_p0__ = sn_strdup(__sn__appended);
                        sn_auto_str char *__is_p1__ = sn_strdup("|");
                        sn_auto_str char *__is_p2__ = sn_strdup(__sn____sn_string_place);
                        sn_auto_str char *__is_p3__ = sn_strdup("|");
                        sn_auto_str char *__is_p4__ = sn_strdup((((char * *)__sn__names->data)[({ long long __ai__ = __sn__index; __ai__ < 0 ? __ai__ + __sn__names->len : __ai__; })]));
                        sn_str_concat_multi(5, __is_p0__, __is_p1__, __is_p2__, __is_p3__, __is_p4__);
                    }); sn_println(__ps__); };
                
                ({
                    long long *__sn_place__ = &(__sn__index);
                    long long __sn_rhs__ = 1LL;
                    *__sn_place__ = sn_add_long(*__sn_place__, __sn_rhs__);
                    *__sn_place__;
                });
                
            }
        }
    }
    { sn_auto_str char *__ps__ = ({
            sn_auto_str char *__is_p0__ = sn_strdup("source=");
            sn_auto_str char *__is_p1__ = sn_strdup((((char * *)__sn__names->data)[({ long long __ai__ = 0LL; __ai__ < 0 ? __ai__ + __sn__names->len : __ai__; })]));
            sn_auto_str char *__is_p2__ = sn_strdup(",");
            sn_auto_str char *__is_p3__ = sn_strdup((((char * *)__sn__names->data)[({ long long __ai__ = 1LL; __ai__ < 0 ? __ai__ + __sn__names->len : __ai__; })]));
            sn_auto_str char *__is_p4__ = sn_strdup(",");
            sn_auto_str char *__is_p5__ = sn_strdup((((char * *)__sn__names->data)[({ long long __ai__ = 2LL; __ai__ < 0 ? __ai__ + __sn__names->len : __ai__; })]));
            sn_auto_str char *__is_p6__ = sn_strdup(" calls=");
            sn_auto_str char *__is_p7__ = sn_str_fmt("%lld", (long long)(__sn__calls));
            sn_str_concat_multi(8, __is_p0__, __is_p1__, __is_p2__, __is_p3__, __is_p4__, __is_p5__, __is_p6__, __is_p7__);
        }); sn_println(__ps__); };
    
    fflush(stdout);
    return 0;
}
