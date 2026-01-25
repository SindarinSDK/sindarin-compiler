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

RtHandle __sn__build_nested(RtManagedArena *);

/* Interceptor thunk forward declarations */
static RtAny __thunk_0(void);

RtHandle __sn__g_result = RT_HANDLE_NULL;
RtHandle __sn__build_nested(RtManagedArena *__caller_arena__) {
    RtManagedArena *__local_arena__ = rt_managed_arena_create_child(__caller_arena__);
    RtHandle _return_value = RT_HANDLE_NULL;
    RtHandle __sn__result = rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "");
    {
        long long __sn__i = 1LL;
        while (rt_le_long(__sn__i, 2LL)) {
            RtManagedArena *__loop_arena_0__ = rt_managed_arena_create_child(__local_arena__);
            {
                RtHandle __sn__outer_str = rt_managed_strdup(__loop_arena_0__, RT_HANDLE_NULL, "");
                {
                    long long __sn__j = 1LL;
                    while (rt_le_long(__sn__j, 2LL)) {
                        RtManagedArena *__loop_arena_2__ = rt_managed_arena_create_child(__loop_arena_0__);
                        {
                            RtHandle __sn__middle_str = rt_managed_strdup(__loop_arena_2__, RT_HANDLE_NULL, "");
                            {
                                long long __sn__k = 1LL;
                                while (rt_le_long(__sn__k, 2LL)) {
                                    RtManagedArena *__loop_arena_4__ = rt_managed_arena_create_child(__loop_arena_2__);
                                    {
                                        RtHandle __sn__inner_str = ({
        char *_p0 = rt_to_string_long(__loop_arena_4__, __sn__i);
        char *_p1 = rt_to_string_long(__loop_arena_4__, __sn__j);
        char *_p2 = rt_to_string_long(__loop_arena_4__, __sn__k);
        char *_r = rt_str_concat(__loop_arena_4__, "(", _p0);
        _r = rt_str_concat(__loop_arena_4__, _r, ",");
        _r = rt_str_concat(__loop_arena_4__, _r, _p1);
        _r = rt_str_concat(__loop_arena_4__, _r, ",");
        _r = rt_str_concat(__loop_arena_4__, _r, _p2);
        _r = rt_str_concat(__loop_arena_4__, _r, ")");
        rt_managed_strdup(__loop_arena_4__, RT_HANDLE_NULL, _r);
    });
                                        (__sn__middle_str = rt_managed_clone(__loop_arena_2__, __loop_arena_4__, rt_str_concat_h(__loop_arena_4__, RT_HANDLE_NULL, (char *)rt_managed_pin(__loop_arena_4__, rt_str_concat_h(__loop_arena_4__, RT_HANDLE_NULL, (char *)rt_managed_pin(__loop_arena_2__, __sn__middle_str), (char *)rt_managed_pin(__loop_arena_4__, __sn__inner_str))), " ")));
                                    }
                                __loop_cleanup_4__:
                                    rt_managed_arena_destroy_child(__loop_arena_4__);
                                __for_continue_5__:;
                                    rt_post_inc_long(&__sn__k);
                                }
                            }
                            (__sn__outer_str = rt_managed_clone(__loop_arena_0__, __loop_arena_2__, rt_str_concat_h(__loop_arena_2__, RT_HANDLE_NULL, (char *)rt_managed_pin(__loop_arena_2__, rt_str_concat_h(__loop_arena_2__, RT_HANDLE_NULL, (char *)rt_managed_pin(__loop_arena_2__, rt_str_concat_h(__loop_arena_2__, RT_HANDLE_NULL, (char *)rt_managed_pin(__loop_arena_0__, __sn__outer_str), "[")), (char *)rt_managed_pin(__loop_arena_2__, __sn__middle_str))), "] ")));
                        }
                    __loop_cleanup_2__:
                        rt_managed_arena_destroy_child(__loop_arena_2__);
                    __for_continue_3__:;
                        rt_post_inc_long(&__sn__j);
                    }
                }
                (__sn__result = rt_managed_clone(__local_arena__, __loop_arena_0__, rt_str_concat_h(__loop_arena_0__, RT_HANDLE_NULL, (char *)rt_managed_pin(__loop_arena_0__, rt_str_concat_h(__loop_arena_0__, RT_HANDLE_NULL, (char *)rt_managed_pin(__loop_arena_0__, rt_str_concat_h(__loop_arena_0__, RT_HANDLE_NULL, (char *)rt_managed_pin(__local_arena__, __sn__result), "<")), (char *)rt_managed_pin(__loop_arena_0__, __sn__outer_str))), ">\n")));
            }
        __loop_cleanup_0__:
            rt_managed_arena_destroy_child(__loop_arena_0__);
        __for_continue_1__:;
            rt_post_inc_long(&__sn__i);
        }
    }
    _return_value = __sn__result;
    goto __sn__build_nested_return;
__sn__build_nested_return:
    _return_value = rt_managed_promote(__caller_arena__, __local_arena__, _return_value);
    rt_managed_arena_destroy_child(__local_arena__);
    return _return_value;
}

int main() {
    RtManagedArena *__local_arena__ = rt_managed_arena_create();
    __main_arena__ = __local_arena__;
    __sn__g_result = rt_managed_strdup(__main_arena__, RT_HANDLE_NULL, "");
    int _return_value = 0;
    RtHandle __sn__local_result = ({
    RtHandle __intercept_result;
    if (__rt_interceptor_count > 0) {
        RtAny __args[1];
        __rt_thunk_args = __args;
        __rt_thunk_arena = __local_arena__;
        RtAny __intercepted = rt_call_intercepted("build_nested", __args, 0, __thunk_0);
        __intercept_result = rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, rt_unbox_string(__intercepted));
    } else {
        __intercept_result = __sn__build_nested(__local_arena__);
    }
    __intercept_result;
});
    (__sn__g_result = rt_managed_promote(__main_arena__, __local_arena__, __sn__local_result));
    rt_print_string((char *)rt_managed_pin(__main_arena__, __sn__g_result));
    _return_value = 0LL;
    goto main_return;
main_return:
    rt_managed_arena_destroy(__local_arena__);
    return _return_value;
}


/* Interceptor thunk definitions */
static RtAny __thunk_0(void) {
    RtAny __result = rt_box_string((char *)rt_managed_pin((RtArena *)__rt_thunk_arena, __sn__build_nested((RtArena *)__rt_thunk_arena)));
    return __result;
}

