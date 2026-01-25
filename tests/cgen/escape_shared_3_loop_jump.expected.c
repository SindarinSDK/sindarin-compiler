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

RtHandle __sn__build_with_jumps(RtManagedArena *);

/* Interceptor thunk forward declarations */
static RtAny __thunk_0(void);

RtHandle __sn__build_with_jumps(RtManagedArena *__caller_arena__) {
    RtManagedArena *__local_arena__ = __caller_arena__;
    RtHandle _return_value = RT_HANDLE_NULL;
    // Code Generation Test: 3 Nested Loop Arena Escape (Shared, Jump)
    //
    // Tests that with shared functions and loops, all allocations
    // go directly to the caller's arena - no jump escapes needed.
    RtHandle __sn__to_d1 = rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "");
    {
        long long __sn__i = 1LL;
        while (rt_le_long(__sn__i, 2LL)) {
            {
                RtHandle __sn__to_d2 = rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "");
                {
                    long long __sn__j = 1LL;
                    while (rt_le_long(__sn__j, 2LL)) {
                        {
                            RtHandle __sn__to_d3 = rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "");
                            {
                                long long __sn__k = 1LL;
                                while (rt_le_long(__sn__k, 2LL)) {
                                    {
                                        // All allocations in same arena - no jumps
                                        RtHandle __sn__tag = ({
        char *_p0 = rt_to_string_long(__local_arena__, __sn__i);
        char *_p1 = rt_to_string_long(__local_arena__, __sn__j);
        char *_p2 = rt_to_string_long(__local_arena__, __sn__k);
        char *_r = rt_str_concat(__local_arena__, "[i=", _p0);
        _r = rt_str_concat(__local_arena__, _r, ",j=");
        _r = rt_str_concat(__local_arena__, _r, _p1);
        _r = rt_str_concat(__local_arena__, _r, ",k=");
        _r = rt_str_concat(__local_arena__, _r, _p2);
        _r = rt_str_concat(__local_arena__, _r, "]");
        rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, _r);
    });
                                        (__sn__to_d3 = rt_str_concat_h(__local_arena__, RT_HANDLE_NULL, (char *)rt_managed_pin(__local_arena__, rt_str_concat_h(__local_arena__, RT_HANDLE_NULL, (char *)rt_managed_pin(__local_arena__, rt_str_concat_h(__local_arena__, RT_HANDLE_NULL, (char *)rt_managed_pin(__local_arena__, __sn__to_d3), "d4->d3")), (char *)rt_managed_pin(__local_arena__, __sn__tag))), " "));
                                        (__sn__to_d2 = rt_str_concat_h(__local_arena__, RT_HANDLE_NULL, (char *)rt_managed_pin(__local_arena__, rt_str_concat_h(__local_arena__, RT_HANDLE_NULL, (char *)rt_managed_pin(__local_arena__, rt_str_concat_h(__local_arena__, RT_HANDLE_NULL, (char *)rt_managed_pin(__local_arena__, __sn__to_d2), "d4->d2")), (char *)rt_managed_pin(__local_arena__, __sn__tag))), " "));
                                        (__sn__to_d1 = rt_str_concat_h(__local_arena__, RT_HANDLE_NULL, (char *)rt_managed_pin(__local_arena__, rt_str_concat_h(__local_arena__, RT_HANDLE_NULL, (char *)rt_managed_pin(__local_arena__, rt_str_concat_h(__local_arena__, RT_HANDLE_NULL, (char *)rt_managed_pin(__local_arena__, __sn__to_d1), "d4->d1")), (char *)rt_managed_pin(__local_arena__, __sn__tag))), " "));
                                    }
                                __for_continue_2__:;
                                    rt_post_inc_long(&__sn__k);
                                }
                            }
                            (__sn__to_d2 = rt_str_concat_h(__local_arena__, RT_HANDLE_NULL, (char *)rt_managed_pin(__local_arena__, rt_str_concat_h(__local_arena__, RT_HANDLE_NULL, (char *)rt_managed_pin(__local_arena__, rt_str_concat_h(__local_arena__, RT_HANDLE_NULL, (char *)rt_managed_pin(__local_arena__, __sn__to_d2), "| collected_d3:(")), (char *)rt_managed_pin(__local_arena__, __sn__to_d3))), ") "));
                        }
                    __for_continue_1__:;
                        rt_post_inc_long(&__sn__j);
                    }
                }
                (__sn__to_d1 = rt_str_concat_h(__local_arena__, RT_HANDLE_NULL, (char *)rt_managed_pin(__local_arena__, rt_str_concat_h(__local_arena__, RT_HANDLE_NULL, (char *)rt_managed_pin(__local_arena__, rt_str_concat_h(__local_arena__, RT_HANDLE_NULL, (char *)rt_managed_pin(__local_arena__, __sn__to_d1), "| collected_d2:(")), (char *)rt_managed_pin(__local_arena__, __sn__to_d2))), ")\n"));
            }
        __for_continue_0__:;
            rt_post_inc_long(&__sn__i);
        }
    }
    _return_value = __sn__to_d1;
    goto __sn__build_with_jumps_return;
__sn__build_with_jumps_return:
    return _return_value;
}

int main() {
    RtManagedArena *__local_arena__ = rt_managed_arena_create();
    __main_arena__ = __local_arena__;
    int _return_value = 0;
    rt_println("Shared jump test (3 loops):");
    rt_println("All values in same arena - no jump escapes needed");
    rt_println((char *)rt_managed_pin(__local_arena__, ({
    RtHandle __intercept_result;
    if (__rt_interceptor_count > 0) {
        RtAny __args[1];
        __rt_thunk_args = __args;
        __rt_thunk_arena = __local_arena__;
        RtAny __intercepted = rt_call_intercepted("build_with_jumps", __args, 0, __thunk_0);
        __intercept_result = rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, rt_unbox_string(__intercepted));
    } else {
        __intercept_result = __sn__build_with_jumps(__local_arena__);
    }
    __intercept_result;
})));
    rt_println("PASS: All shared allocations succeeded");
    _return_value = 0LL;
    goto main_return;
main_return:
    rt_managed_arena_destroy(__local_arena__);
    return _return_value;
}


/* Interceptor thunk definitions */
static RtAny __thunk_0(void) {
    RtAny __result = rt_box_string((char *)rt_managed_pin((RtArena *)__rt_thunk_arena, __sn__build_with_jumps((RtArena *)__rt_thunk_arena)));
    return __result;
}

