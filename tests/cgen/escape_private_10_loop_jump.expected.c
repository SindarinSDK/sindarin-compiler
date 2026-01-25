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

long long __sn__compute_with_jumps(RtManagedArena *);

/* Interceptor thunk forward declarations */
static RtAny __thunk_0(void);

long long __sn__compute_with_jumps(RtManagedArena *__caller_arena__) {
    RtManagedArena *__local_arena__ = rt_managed_arena_create_child(__caller_arena__);
    long long _return_value = 0;
    // Code Generation Test: 10 Nested Loop Arena Escape (Private, Jump)
    //
    // Tests that primitives can jump directly from innermost loop to any
    // outer scope in private functions through 10 nested loops.
    long long __sn__to_d1 = 0LL;
    {
        long long __sn__a = 1LL;
        while (rt_le_long(__sn__a, 1LL)) {
            RtManagedArena *__loop_arena_0__ = rt_managed_arena_create_child(__local_arena__);
            {
                long long __sn__to_d2 = 0LL;
                {
                    long long __sn__b = 1LL;
                    while (rt_le_long(__sn__b, 1LL)) {
                        RtManagedArena *__loop_arena_2__ = rt_managed_arena_create_child(__loop_arena_0__);
                        {
                            long long __sn__to_d3 = 0LL;
                            {
                                long long __sn__c = 1LL;
                                while (rt_le_long(__sn__c, 1LL)) {
                                    RtManagedArena *__loop_arena_4__ = rt_managed_arena_create_child(__loop_arena_2__);
                                    {
                                        long long __sn__to_d4 = 0LL;
                                        {
                                            long long __sn__d = 1LL;
                                            while (rt_le_long(__sn__d, 1LL)) {
                                                RtManagedArena *__loop_arena_6__ = rt_managed_arena_create_child(__loop_arena_4__);
                                                {
                                                    long long __sn__to_d5 = 0LL;
                                                    {
                                                        long long __sn__e = 1LL;
                                                        while (rt_le_long(__sn__e, 1LL)) {
                                                            RtManagedArena *__loop_arena_8__ = rt_managed_arena_create_child(__loop_arena_6__);
                                                            {
                                                                long long __sn__to_d6 = 0LL;
                                                                {
                                                                    long long __sn__f = 1LL;
                                                                    while (rt_le_long(__sn__f, 1LL)) {
                                                                        RtManagedArena *__loop_arena_10__ = rt_managed_arena_create_child(__loop_arena_8__);
                                                                        {
                                                                            long long __sn__to_d7 = 0LL;
                                                                            {
                                                                                long long __sn__g = 1LL;
                                                                                while (rt_le_long(__sn__g, 1LL)) {
                                                                                    RtManagedArena *__loop_arena_12__ = rt_managed_arena_create_child(__loop_arena_10__);
                                                                                    {
                                                                                        long long __sn__to_d8 = 0LL;
                                                                                        {
                                                                                            long long __sn__h = 1LL;
                                                                                            while (rt_le_long(__sn__h, 1LL)) {
                                                                                                RtManagedArena *__loop_arena_14__ = rt_managed_arena_create_child(__loop_arena_12__);
                                                                                                {
                                                                                                    long long __sn__to_d9 = 0LL;
                                                                                                    {
                                                                                                        long long __sn__i = 1LL;
                                                                                                        while (rt_le_long(__sn__i, 1LL)) {
                                                                                                            RtManagedArena *__loop_arena_16__ = rt_managed_arena_create_child(__loop_arena_14__);
                                                                                                            {
                                                                                                                long long __sn__to_d10 = 0LL;
                                                                                                                {
                                                                                                                    long long __sn__j = 1LL;
                                                                                                                    while (rt_le_long(__sn__j, 1LL)) {
                                                                                                                        RtManagedArena *__loop_arena_18__ = rt_managed_arena_create_child(__loop_arena_16__);
                                                                                                                        {
                                                                                                                            // All jumps from depth 11
                                                                                                                            (__sn__to_d10 = 10LL);
                                                                                                                            (__sn__to_d9 = 9LL);
                                                                                                                            (__sn__to_d8 = 8LL);
                                                                                                                            (__sn__to_d7 = 7LL);
                                                                                                                            (__sn__to_d6 = 6LL);
                                                                                                                            (__sn__to_d5 = 5LL);
                                                                                                                            (__sn__to_d4 = 4LL);
                                                                                                                            (__sn__to_d3 = 3LL);
                                                                                                                            (__sn__to_d2 = 2LL);
                                                                                                                            (__sn__to_d1 = 1LL);
                                                                                                                        }
                                                                                                                    __loop_cleanup_18__:
                                                                                                                        rt_managed_arena_destroy_child(__loop_arena_18__);
                                                                                                                    __for_continue_19__:;
                                                                                                                        rt_post_inc_long(&__sn__j);
                                                                                                                    }
                                                                                                                }
                                                                                                                (__sn__to_d9 = rt_add_long(__sn__to_d9, __sn__to_d10));
                                                                                                            }
                                                                                                        __loop_cleanup_16__:
                                                                                                            rt_managed_arena_destroy_child(__loop_arena_16__);
                                                                                                        __for_continue_17__:;
                                                                                                            rt_post_inc_long(&__sn__i);
                                                                                                        }
                                                                                                    }
                                                                                                    (__sn__to_d8 = rt_add_long(__sn__to_d8, __sn__to_d9));
                                                                                                }
                                                                                            __loop_cleanup_14__:
                                                                                                rt_managed_arena_destroy_child(__loop_arena_14__);
                                                                                            __for_continue_15__:;
                                                                                                rt_post_inc_long(&__sn__h);
                                                                                            }
                                                                                        }
                                                                                        (__sn__to_d7 = rt_add_long(__sn__to_d7, __sn__to_d8));
                                                                                    }
                                                                                __loop_cleanup_12__:
                                                                                    rt_managed_arena_destroy_child(__loop_arena_12__);
                                                                                __for_continue_13__:;
                                                                                    rt_post_inc_long(&__sn__g);
                                                                                }
                                                                            }
                                                                            (__sn__to_d6 = rt_add_long(__sn__to_d6, __sn__to_d7));
                                                                        }
                                                                    __loop_cleanup_10__:
                                                                        rt_managed_arena_destroy_child(__loop_arena_10__);
                                                                    __for_continue_11__:;
                                                                        rt_post_inc_long(&__sn__f);
                                                                    }
                                                                }
                                                                (__sn__to_d5 = rt_add_long(__sn__to_d5, __sn__to_d6));
                                                            }
                                                        __loop_cleanup_8__:
                                                            rt_managed_arena_destroy_child(__loop_arena_8__);
                                                        __for_continue_9__:;
                                                            rt_post_inc_long(&__sn__e);
                                                        }
                                                    }
                                                    (__sn__to_d4 = rt_add_long(__sn__to_d4, __sn__to_d5));
                                                }
                                            __loop_cleanup_6__:
                                                rt_managed_arena_destroy_child(__loop_arena_6__);
                                            __for_continue_7__:;
                                                rt_post_inc_long(&__sn__d);
                                            }
                                        }
                                        (__sn__to_d3 = rt_add_long(__sn__to_d3, __sn__to_d4));
                                    }
                                __loop_cleanup_4__:
                                    rt_managed_arena_destroy_child(__loop_arena_4__);
                                __for_continue_5__:;
                                    rt_post_inc_long(&__sn__c);
                                }
                            }
                            (__sn__to_d2 = rt_add_long(__sn__to_d2, __sn__to_d3));
                        }
                    __loop_cleanup_2__:
                        rt_managed_arena_destroy_child(__loop_arena_2__);
                    __for_continue_3__:;
                        rt_post_inc_long(&__sn__b);
                    }
                }
                (__sn__to_d1 = rt_add_long(__sn__to_d1, __sn__to_d2));
            }
        __loop_cleanup_0__:
            rt_managed_arena_destroy_child(__loop_arena_0__);
        __for_continue_1__:;
            rt_post_inc_long(&__sn__a);
        }
    }
    _return_value = __sn__to_d1;
    goto __sn__compute_with_jumps_return;
__sn__compute_with_jumps_return:
    rt_managed_arena_destroy_child(__local_arena__);
    return _return_value;
}

int main() {
    RtManagedArena *__local_arena__ = rt_managed_arena_create();
    __main_arena__ = __local_arena__;
    int _return_value = 0;
    rt_println("Private jump test (10 loops):");
    rt_println("Primitives jump directly to each target depth");
    long long __sn__computed = ({
    long long __intercept_result;
    if (__rt_interceptor_count > 0) {
        RtAny __args[1];
        __rt_thunk_args = __args;
        __rt_thunk_arena = __local_arena__;
        RtAny __intercepted = rt_call_intercepted("compute_with_jumps", __args, 0, __thunk_0);
        __intercept_result = rt_unbox_int(__intercepted);
    } else {
        __intercept_result = __sn__compute_with_jumps(__local_arena__);
    }
    __intercept_result;
});
    // After jumps: d10=10, d9=9+10=19, d8=8+19=27, d7=7+27=34, d6=6+34=40
    // d5=5+40=45, d4=4+45=49, d3=3+49=52, d2=2+52=54, d1=1+54=55
    rt_println(({
        char *_p0 = rt_to_string_long(__local_arena__, __sn__computed);
        rt_str_concat(__local_arena__, "Computed checksum: ", _p0);
    }));
    if (rt_eq_long(__sn__computed, 55LL)) {
        {
            rt_println("PASS: Checksum correct - primitives escaped properly");
        }
    }
    else {
        {
            rt_println("FAIL: Checksum incorrect");
        }
    }
    _return_value = 0LL;
    goto main_return;
main_return:
    rt_managed_arena_destroy(__local_arena__);
    return _return_value;
}


/* Interceptor thunk definitions */
static RtAny __thunk_0(void) {
    RtAny __result = rt_box_int(__sn__compute_with_jumps((RtArena *)__rt_thunk_arena));
    return __result;
}

