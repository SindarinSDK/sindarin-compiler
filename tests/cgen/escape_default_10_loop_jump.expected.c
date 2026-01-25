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
    RtManagedArena *__local_arena__ = rt_managed_arena_create_child(__caller_arena__);
    RtHandle _return_value = RT_HANDLE_NULL;
    // Code Generation Test: 10 Nested Loop Arena Escape (Jump)
    //
    // Tests that string values can jump directly from innermost loop (depth 11)
    // to any outer scope, skipping intermediate arenas.
    RtHandle __sn__to_d1 = rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "");
    {
        long long __sn__a = 1LL;
        while (rt_le_long(__sn__a, 1LL)) {
            RtManagedArena *__loop_arena_0__ = rt_managed_arena_create_child(__local_arena__);
            {
                RtHandle __sn__to_d2 = rt_managed_strdup(__loop_arena_0__, RT_HANDLE_NULL, "");
                {
                    long long __sn__b = 1LL;
                    while (rt_le_long(__sn__b, 1LL)) {
                        RtManagedArena *__loop_arena_2__ = rt_managed_arena_create_child(__loop_arena_0__);
                        {
                            RtHandle __sn__to_d3 = rt_managed_strdup(__loop_arena_2__, RT_HANDLE_NULL, "");
                            {
                                long long __sn__c = 1LL;
                                while (rt_le_long(__sn__c, 1LL)) {
                                    RtManagedArena *__loop_arena_4__ = rt_managed_arena_create_child(__loop_arena_2__);
                                    {
                                        RtHandle __sn__to_d4 = rt_managed_strdup(__loop_arena_4__, RT_HANDLE_NULL, "");
                                        {
                                            long long __sn__d = 1LL;
                                            while (rt_le_long(__sn__d, 1LL)) {
                                                RtManagedArena *__loop_arena_6__ = rt_managed_arena_create_child(__loop_arena_4__);
                                                {
                                                    RtHandle __sn__to_d5 = rt_managed_strdup(__loop_arena_6__, RT_HANDLE_NULL, "");
                                                    {
                                                        long long __sn__e = 1LL;
                                                        while (rt_le_long(__sn__e, 1LL)) {
                                                            RtManagedArena *__loop_arena_8__ = rt_managed_arena_create_child(__loop_arena_6__);
                                                            {
                                                                RtHandle __sn__to_d6 = rt_managed_strdup(__loop_arena_8__, RT_HANDLE_NULL, "");
                                                                {
                                                                    long long __sn__f = 1LL;
                                                                    while (rt_le_long(__sn__f, 1LL)) {
                                                                        RtManagedArena *__loop_arena_10__ = rt_managed_arena_create_child(__loop_arena_8__);
                                                                        {
                                                                            RtHandle __sn__to_d7 = rt_managed_strdup(__loop_arena_10__, RT_HANDLE_NULL, "");
                                                                            {
                                                                                long long __sn__g = 1LL;
                                                                                while (rt_le_long(__sn__g, 1LL)) {
                                                                                    RtManagedArena *__loop_arena_12__ = rt_managed_arena_create_child(__loop_arena_10__);
                                                                                    {
                                                                                        RtHandle __sn__to_d8 = rt_managed_strdup(__loop_arena_12__, RT_HANDLE_NULL, "");
                                                                                        {
                                                                                            long long __sn__h = 1LL;
                                                                                            while (rt_le_long(__sn__h, 1LL)) {
                                                                                                RtManagedArena *__loop_arena_14__ = rt_managed_arena_create_child(__loop_arena_12__);
                                                                                                {
                                                                                                    RtHandle __sn__to_d9 = rt_managed_strdup(__loop_arena_14__, RT_HANDLE_NULL, "");
                                                                                                    {
                                                                                                        long long __sn__i = 1LL;
                                                                                                        while (rt_le_long(__sn__i, 1LL)) {
                                                                                                            RtManagedArena *__loop_arena_16__ = rt_managed_arena_create_child(__loop_arena_14__);
                                                                                                            {
                                                                                                                RtHandle __sn__to_d10 = rt_managed_strdup(__loop_arena_16__, RT_HANDLE_NULL, "");
                                                                                                                {
                                                                                                                    long long __sn__j = 1LL;
                                                                                                                    while (rt_le_long(__sn__j, 1LL)) {
                                                                                                                        RtManagedArena *__loop_arena_18__ = rt_managed_arena_create_child(__loop_arena_16__);
                                                                                                                        {
                                                                                                                            // All jumps originate from depth 11
                                                                                                                            (__sn__to_d10 = rt_managed_clone(__loop_arena_16__, __loop_arena_18__, rt_managed_strdup(__loop_arena_18__, RT_HANDLE_NULL, "d11->d10")));
                                                                                                                            (__sn__to_d9 = rt_managed_clone(__loop_arena_14__, __loop_arena_18__, rt_managed_strdup(__loop_arena_18__, RT_HANDLE_NULL, "d11->d9")));
                                                                                                                            (__sn__to_d8 = rt_managed_clone(__loop_arena_12__, __loop_arena_18__, rt_managed_strdup(__loop_arena_18__, RT_HANDLE_NULL, "d11->d8")));
                                                                                                                            (__sn__to_d7 = rt_managed_clone(__loop_arena_10__, __loop_arena_18__, rt_managed_strdup(__loop_arena_18__, RT_HANDLE_NULL, "d11->d7")));
                                                                                                                            (__sn__to_d6 = rt_managed_clone(__loop_arena_8__, __loop_arena_18__, rt_managed_strdup(__loop_arena_18__, RT_HANDLE_NULL, "d11->d6")));
                                                                                                                            (__sn__to_d5 = rt_managed_clone(__loop_arena_6__, __loop_arena_18__, rt_managed_strdup(__loop_arena_18__, RT_HANDLE_NULL, "d11->d5")));
                                                                                                                            (__sn__to_d4 = rt_managed_clone(__loop_arena_4__, __loop_arena_18__, rt_managed_strdup(__loop_arena_18__, RT_HANDLE_NULL, "d11->d4")));
                                                                                                                            (__sn__to_d3 = rt_managed_clone(__loop_arena_2__, __loop_arena_18__, rt_managed_strdup(__loop_arena_18__, RT_HANDLE_NULL, "d11->d3")));
                                                                                                                            (__sn__to_d2 = rt_managed_clone(__loop_arena_0__, __loop_arena_18__, rt_managed_strdup(__loop_arena_18__, RT_HANDLE_NULL, "d11->d2")));
                                                                                                                            (__sn__to_d1 = rt_managed_clone(__local_arena__, __loop_arena_18__, rt_managed_strdup(__loop_arena_18__, RT_HANDLE_NULL, "d11->d1")));
                                                                                                                        }
                                                                                                                    __loop_cleanup_18__:
                                                                                                                        rt_managed_arena_destroy_child(__loop_arena_18__);
                                                                                                                    __for_continue_19__:;
                                                                                                                        rt_post_inc_long(&__sn__j);
                                                                                                                    }
                                                                                                                }
                                                                                                                (__sn__to_d9 = rt_managed_clone(__loop_arena_14__, __loop_arena_16__, rt_str_concat_h(__loop_arena_16__, RT_HANDLE_NULL, (char *)rt_managed_pin(__loop_arena_16__, rt_str_concat_h(__loop_arena_16__, RT_HANDLE_NULL, (char *)rt_managed_pin(__loop_arena_14__, __sn__to_d9), " | at_d10:")), (char *)rt_managed_pin(__loop_arena_16__, __sn__to_d10))));
                                                                                                            }
                                                                                                        __loop_cleanup_16__:
                                                                                                            rt_managed_arena_destroy_child(__loop_arena_16__);
                                                                                                        __for_continue_17__:;
                                                                                                            rt_post_inc_long(&__sn__i);
                                                                                                        }
                                                                                                    }
                                                                                                    (__sn__to_d8 = rt_managed_clone(__loop_arena_12__, __loop_arena_14__, rt_str_concat_h(__loop_arena_14__, RT_HANDLE_NULL, (char *)rt_managed_pin(__loop_arena_14__, rt_str_concat_h(__loop_arena_14__, RT_HANDLE_NULL, (char *)rt_managed_pin(__loop_arena_12__, __sn__to_d8), " | at_d9:")), (char *)rt_managed_pin(__loop_arena_14__, __sn__to_d9))));
                                                                                                }
                                                                                            __loop_cleanup_14__:
                                                                                                rt_managed_arena_destroy_child(__loop_arena_14__);
                                                                                            __for_continue_15__:;
                                                                                                rt_post_inc_long(&__sn__h);
                                                                                            }
                                                                                        }
                                                                                        (__sn__to_d7 = rt_managed_clone(__loop_arena_10__, __loop_arena_12__, rt_str_concat_h(__loop_arena_12__, RT_HANDLE_NULL, (char *)rt_managed_pin(__loop_arena_12__, rt_str_concat_h(__loop_arena_12__, RT_HANDLE_NULL, (char *)rt_managed_pin(__loop_arena_10__, __sn__to_d7), " | at_d8:")), (char *)rt_managed_pin(__loop_arena_12__, __sn__to_d8))));
                                                                                    }
                                                                                __loop_cleanup_12__:
                                                                                    rt_managed_arena_destroy_child(__loop_arena_12__);
                                                                                __for_continue_13__:;
                                                                                    rt_post_inc_long(&__sn__g);
                                                                                }
                                                                            }
                                                                            (__sn__to_d6 = rt_managed_clone(__loop_arena_8__, __loop_arena_10__, rt_str_concat_h(__loop_arena_10__, RT_HANDLE_NULL, (char *)rt_managed_pin(__loop_arena_10__, rt_str_concat_h(__loop_arena_10__, RT_HANDLE_NULL, (char *)rt_managed_pin(__loop_arena_8__, __sn__to_d6), " | at_d7:")), (char *)rt_managed_pin(__loop_arena_10__, __sn__to_d7))));
                                                                        }
                                                                    __loop_cleanup_10__:
                                                                        rt_managed_arena_destroy_child(__loop_arena_10__);
                                                                    __for_continue_11__:;
                                                                        rt_post_inc_long(&__sn__f);
                                                                    }
                                                                }
                                                                (__sn__to_d5 = rt_managed_clone(__loop_arena_6__, __loop_arena_8__, rt_str_concat_h(__loop_arena_8__, RT_HANDLE_NULL, (char *)rt_managed_pin(__loop_arena_8__, rt_str_concat_h(__loop_arena_8__, RT_HANDLE_NULL, (char *)rt_managed_pin(__loop_arena_6__, __sn__to_d5), " | at_d6:")), (char *)rt_managed_pin(__loop_arena_8__, __sn__to_d6))));
                                                            }
                                                        __loop_cleanup_8__:
                                                            rt_managed_arena_destroy_child(__loop_arena_8__);
                                                        __for_continue_9__:;
                                                            rt_post_inc_long(&__sn__e);
                                                        }
                                                    }
                                                    (__sn__to_d4 = rt_managed_clone(__loop_arena_4__, __loop_arena_6__, rt_str_concat_h(__loop_arena_6__, RT_HANDLE_NULL, (char *)rt_managed_pin(__loop_arena_6__, rt_str_concat_h(__loop_arena_6__, RT_HANDLE_NULL, (char *)rt_managed_pin(__loop_arena_4__, __sn__to_d4), " | at_d5:")), (char *)rt_managed_pin(__loop_arena_6__, __sn__to_d5))));
                                                }
                                            __loop_cleanup_6__:
                                                rt_managed_arena_destroy_child(__loop_arena_6__);
                                            __for_continue_7__:;
                                                rt_post_inc_long(&__sn__d);
                                            }
                                        }
                                        (__sn__to_d3 = rt_managed_clone(__loop_arena_2__, __loop_arena_4__, rt_str_concat_h(__loop_arena_4__, RT_HANDLE_NULL, (char *)rt_managed_pin(__loop_arena_4__, rt_str_concat_h(__loop_arena_4__, RT_HANDLE_NULL, (char *)rt_managed_pin(__loop_arena_2__, __sn__to_d3), " | at_d4:")), (char *)rt_managed_pin(__loop_arena_4__, __sn__to_d4))));
                                    }
                                __loop_cleanup_4__:
                                    rt_managed_arena_destroy_child(__loop_arena_4__);
                                __for_continue_5__:;
                                    rt_post_inc_long(&__sn__c);
                                }
                            }
                            (__sn__to_d2 = rt_managed_clone(__loop_arena_0__, __loop_arena_2__, rt_str_concat_h(__loop_arena_2__, RT_HANDLE_NULL, (char *)rt_managed_pin(__loop_arena_2__, rt_str_concat_h(__loop_arena_2__, RT_HANDLE_NULL, (char *)rt_managed_pin(__loop_arena_0__, __sn__to_d2), " | at_d3:")), (char *)rt_managed_pin(__loop_arena_2__, __sn__to_d3))));
                        }
                    __loop_cleanup_2__:
                        rt_managed_arena_destroy_child(__loop_arena_2__);
                    __for_continue_3__:;
                        rt_post_inc_long(&__sn__b);
                    }
                }
                (__sn__to_d1 = rt_managed_clone(__local_arena__, __loop_arena_0__, rt_str_concat_h(__loop_arena_0__, RT_HANDLE_NULL, (char *)rt_managed_pin(__loop_arena_0__, rt_str_concat_h(__loop_arena_0__, RT_HANDLE_NULL, (char *)rt_managed_pin(__local_arena__, __sn__to_d1), " | at_d2:")), (char *)rt_managed_pin(__loop_arena_0__, __sn__to_d2))));
            }
        __loop_cleanup_0__:
            rt_managed_arena_destroy_child(__loop_arena_0__);
        __for_continue_1__:;
            rt_post_inc_long(&__sn__a);
        }
    }
    _return_value = __sn__to_d1;
    goto __sn__build_with_jumps_return;
__sn__build_with_jumps_return:
    _return_value = rt_managed_promote(__caller_arena__, __local_arena__, _return_value);
    rt_managed_arena_destroy_child(__local_arena__);
    return _return_value;
}

int main() {
    RtManagedArena *__local_arena__ = rt_managed_arena_create();
    __main_arena__ = __local_arena__;
    int _return_value = 0;
    rt_println("Jump escape test (10 loops):");
    rt_println("Values jump directly from d11 to each target depth");
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
    rt_println("PASS: All jump escapes succeeded");
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

