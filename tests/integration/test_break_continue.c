#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>
#include <setjmp.h>
#include "runtime.h"
#ifdef _WIN32
#undef min
#undef max
#endif


/* Closure type for lambdas */
typedef struct __Closure__ { void *fn; RtArena *arena; size_t size; } __Closure__;

/* Forward declarations */
static RtManagedArena *__main_arena__ = NULL;

int main() {
    RtManagedArena *__local_arena__ = rt_managed_arena_create();
    __main_arena__ = __local_arena__;
    int _return_value = 0;
    // Test break
    rt_print_string("Testing break:\n");
    long long __sn__i = 0LL;
    while (rt_lt_long(__sn__i, 10LL)) {
        {
            if (rt_eq_long(__sn__i, 5LL)) {
                {
                    break;
                }
            }
            ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__local_arena__, __sn__i);
        rt_str_concat(__local_arena__, _p0, " ");
    });
        rt_print_string(_str_arg0);
    });
            (__sn__i = rt_add_long(__sn__i, 1LL));
        }
    }
    rt_print_string("\n");
    // Test continue
    rt_print_string("Testing continue:\n");
    (__sn__i = 0LL);
    while (rt_lt_long(__sn__i, 10LL)) {
        {
            (__sn__i = rt_add_long(__sn__i, 1LL));
            if (rt_eq_long(rt_mod_long(__sn__i, 2LL), 0LL)) {
                {
                    continue;
                }
            }
            ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__local_arena__, __sn__i);
        rt_str_concat(__local_arena__, _p0, " ");
    });
        rt_print_string(_str_arg0);
    });
        }
    }
    rt_print_string("\n");
    // Test break in for loop
    rt_print_string("Testing break in for:\n");
    {
        long long __sn__j = 0LL;
        while (rt_lt_long(__sn__j, 10LL)) {
            {
                if (rt_eq_long(__sn__j, 3LL)) {
                    {
                        break;
                    }
                }
                ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__local_arena__, __sn__j);
        rt_str_concat(__local_arena__, _p0, " ");
    });
        rt_print_string(_str_arg0);
    });
            }
        __for_continue_0__:;
            rt_post_inc_long(&__sn__j);
        }
    }
    rt_print_string("\n");
    // Test continue in for-each
    rt_print_string("Testing continue in for-each:\n");
    RtHandle __sn__nums = rt_array_create_long_h(__local_arena__, 5, (long long[]){1LL, 2LL, 3LL, 4LL, 5LL});
    {
        long long * __arr_0__ = ((long long *)rt_managed_pin_array(__local_arena__, __sn__nums));
        long __len_0__ = rt_array_length(__arr_0__);
        for (long __idx_0__ = 0; __idx_0__ < __len_0__; __idx_0__++) {
            long long __sn__n = __arr_0__[__idx_0__];
            {
                if (rt_eq_long(__sn__n, 3LL)) {
                    {
                        continue;
                    }
                }
                ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__local_arena__, __sn__n);
        rt_str_concat(__local_arena__, _p0, " ");
    });
        rt_print_string(_str_arg0);
    });
            }
        }
    }
    rt_print_string("\n");
    _return_value = 0LL;
    goto main_return;
main_return:
    rt_managed_arena_destroy(__local_arena__);
    return _return_value;
}

