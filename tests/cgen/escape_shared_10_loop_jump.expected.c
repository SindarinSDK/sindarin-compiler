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
    // Code Generation Test: 10 Nested Loop Arena Escape (Shared, Jump)
    //
    // Tests that with shared functions and loops, all allocations
    // go directly to the caller's arena - no jump escapes needed.
    RtHandle __sn__to_d1 = rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "");
    {
        long long __sn__a = 1LL;
        while (rt_le_long(__sn__a, 1LL)) {
            {
                RtHandle __sn__to_d2 = rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "");
                {
                    long long __sn__b = 1LL;
                    while (rt_le_long(__sn__b, 1LL)) {
                        {
                            RtHandle __sn__to_d3 = rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "");
                            {
                                long long __sn__c = 1LL;
                                while (rt_le_long(__sn__c, 1LL)) {
                                    {
                                        RtHandle __sn__to_d4 = rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "");
                                        {
                                            long long __sn__d = 1LL;
                                            while (rt_le_long(__sn__d, 1LL)) {
                                                {
                                                    RtHandle __sn__to_d5 = rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "");
                                                    {
                                                        long long __sn__e = 1LL;
                                                        while (rt_le_long(__sn__e, 1LL)) {
                                                            {
                                                                RtHandle __sn__to_d6 = rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "");
                                                                {
                                                                    long long __sn__f = 1LL;
                                                                    while (rt_le_long(__sn__f, 1LL)) {
                                                                        {
                                                                            RtHandle __sn__to_d7 = rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "");
                                                                            {
                                                                                long long __sn__g = 1LL;
                                                                                while (rt_le_long(__sn__g, 1LL)) {
                                                                                    {
                                                                                        RtHandle __sn__to_d8 = rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "");
                                                                                        {
                                                                                            long long __sn__h = 1LL;
                                                                                            while (rt_le_long(__sn__h, 1LL)) {
                                                                                                {
                                                                                                    RtHandle __sn__to_d9 = rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "");
                                                                                                    {
                                                                                                        long long __sn__i = 1LL;
                                                                                                        while (rt_le_long(__sn__i, 1LL)) {
                                                                                                            {
                                                                                                                RtHandle __sn__to_d10 = rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "");
                                                                                                                {
                                                                                                                    long long __sn__j = 1LL;
                                                                                                                    while (rt_le_long(__sn__j, 1LL)) {
                                                                                                                        {
                                                                                                                            // All allocations in same arena
                                                                                                                            (__sn__to_d10 = rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "d11->d10"));
                                                                                                                            (__sn__to_d9 = rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "d11->d9"));
                                                                                                                            (__sn__to_d8 = rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "d11->d8"));
                                                                                                                            (__sn__to_d7 = rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "d11->d7"));
                                                                                                                            (__sn__to_d6 = rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "d11->d6"));
                                                                                                                            (__sn__to_d5 = rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "d11->d5"));
                                                                                                                            (__sn__to_d4 = rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "d11->d4"));
                                                                                                                            (__sn__to_d3 = rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "d11->d3"));
                                                                                                                            (__sn__to_d2 = rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "d11->d2"));
                                                                                                                            (__sn__to_d1 = rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "d11->d1"));
                                                                                                                        }
                                                                                                                    __for_continue_9__:;
                                                                                                                        rt_post_inc_long(&__sn__j);
                                                                                                                    }
                                                                                                                }
                                                                                                                (__sn__to_d9 = rt_str_concat_h(__local_arena__, RT_HANDLE_NULL, (char *)rt_managed_pin(__local_arena__, rt_str_concat_h(__local_arena__, RT_HANDLE_NULL, (char *)rt_managed_pin(__local_arena__, __sn__to_d9), " | at_d10:")), (char *)rt_managed_pin(__local_arena__, __sn__to_d10)));
                                                                                                            }
                                                                                                        __for_continue_8__:;
                                                                                                            rt_post_inc_long(&__sn__i);
                                                                                                        }
                                                                                                    }
                                                                                                    (__sn__to_d8 = rt_str_concat_h(__local_arena__, RT_HANDLE_NULL, (char *)rt_managed_pin(__local_arena__, rt_str_concat_h(__local_arena__, RT_HANDLE_NULL, (char *)rt_managed_pin(__local_arena__, __sn__to_d8), " | at_d9:")), (char *)rt_managed_pin(__local_arena__, __sn__to_d9)));
                                                                                                }
                                                                                            __for_continue_7__:;
                                                                                                rt_post_inc_long(&__sn__h);
                                                                                            }
                                                                                        }
                                                                                        (__sn__to_d7 = rt_str_concat_h(__local_arena__, RT_HANDLE_NULL, (char *)rt_managed_pin(__local_arena__, rt_str_concat_h(__local_arena__, RT_HANDLE_NULL, (char *)rt_managed_pin(__local_arena__, __sn__to_d7), " | at_d8:")), (char *)rt_managed_pin(__local_arena__, __sn__to_d8)));
                                                                                    }
                                                                                __for_continue_6__:;
                                                                                    rt_post_inc_long(&__sn__g);
                                                                                }
                                                                            }
                                                                            (__sn__to_d6 = rt_str_concat_h(__local_arena__, RT_HANDLE_NULL, (char *)rt_managed_pin(__local_arena__, rt_str_concat_h(__local_arena__, RT_HANDLE_NULL, (char *)rt_managed_pin(__local_arena__, __sn__to_d6), " | at_d7:")), (char *)rt_managed_pin(__local_arena__, __sn__to_d7)));
                                                                        }
                                                                    __for_continue_5__:;
                                                                        rt_post_inc_long(&__sn__f);
                                                                    }
                                                                }
                                                                (__sn__to_d5 = rt_str_concat_h(__local_arena__, RT_HANDLE_NULL, (char *)rt_managed_pin(__local_arena__, rt_str_concat_h(__local_arena__, RT_HANDLE_NULL, (char *)rt_managed_pin(__local_arena__, __sn__to_d5), " | at_d6:")), (char *)rt_managed_pin(__local_arena__, __sn__to_d6)));
                                                            }
                                                        __for_continue_4__:;
                                                            rt_post_inc_long(&__sn__e);
                                                        }
                                                    }
                                                    (__sn__to_d4 = rt_str_concat_h(__local_arena__, RT_HANDLE_NULL, (char *)rt_managed_pin(__local_arena__, rt_str_concat_h(__local_arena__, RT_HANDLE_NULL, (char *)rt_managed_pin(__local_arena__, __sn__to_d4), " | at_d5:")), (char *)rt_managed_pin(__local_arena__, __sn__to_d5)));
                                                }
                                            __for_continue_3__:;
                                                rt_post_inc_long(&__sn__d);
                                            }
                                        }
                                        (__sn__to_d3 = rt_str_concat_h(__local_arena__, RT_HANDLE_NULL, (char *)rt_managed_pin(__local_arena__, rt_str_concat_h(__local_arena__, RT_HANDLE_NULL, (char *)rt_managed_pin(__local_arena__, __sn__to_d3), " | at_d4:")), (char *)rt_managed_pin(__local_arena__, __sn__to_d4)));
                                    }
                                __for_continue_2__:;
                                    rt_post_inc_long(&__sn__c);
                                }
                            }
                            (__sn__to_d2 = rt_str_concat_h(__local_arena__, RT_HANDLE_NULL, (char *)rt_managed_pin(__local_arena__, rt_str_concat_h(__local_arena__, RT_HANDLE_NULL, (char *)rt_managed_pin(__local_arena__, __sn__to_d2), " | at_d3:")), (char *)rt_managed_pin(__local_arena__, __sn__to_d3)));
                        }
                    __for_continue_1__:;
                        rt_post_inc_long(&__sn__b);
                    }
                }
                (__sn__to_d1 = rt_str_concat_h(__local_arena__, RT_HANDLE_NULL, (char *)rt_managed_pin(__local_arena__, rt_str_concat_h(__local_arena__, RT_HANDLE_NULL, (char *)rt_managed_pin(__local_arena__, __sn__to_d1), " | at_d2:")), (char *)rt_managed_pin(__local_arena__, __sn__to_d2)));
            }
        __for_continue_0__:;
            rt_post_inc_long(&__sn__a);
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
    rt_println("Shared jump test (10 loops):");
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

