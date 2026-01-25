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

long long __sn__compute_chained(RtManagedArena *);

/* Interceptor thunk forward declarations */
static RtAny __thunk_0(void);

long long __sn__compute_chained(RtManagedArena *__caller_arena__) {
    RtManagedArena *__local_arena__ = rt_managed_arena_create_child(__caller_arena__);
    long long _return_value = 0;
    // Code Generation Test: 10 Nested Loop Arena Escape (Private, Chained)
    //
    // Tests that primitive values escape correctly from private functions
    // through 10 nested loops. Only primitives can escape private blocks.
    long long __sn__result = 0LL;
    {
        long long __sn__a = 1LL;
        while (rt_le_long(__sn__a, 1LL)) {
            RtManagedArena *__loop_arena_0__ = rt_managed_arena_create_child(__local_arena__);
            {
                long long __sn__s1 = 0LL;
                {
                    long long __sn__b = 1LL;
                    while (rt_le_long(__sn__b, 1LL)) {
                        RtManagedArena *__loop_arena_2__ = rt_managed_arena_create_child(__loop_arena_0__);
                        {
                            long long __sn__s2 = 0LL;
                            {
                                long long __sn__c = 1LL;
                                while (rt_le_long(__sn__c, 1LL)) {
                                    RtManagedArena *__loop_arena_4__ = rt_managed_arena_create_child(__loop_arena_2__);
                                    {
                                        long long __sn__s3 = 0LL;
                                        {
                                            long long __sn__d = 1LL;
                                            while (rt_le_long(__sn__d, 1LL)) {
                                                RtManagedArena *__loop_arena_6__ = rt_managed_arena_create_child(__loop_arena_4__);
                                                {
                                                    long long __sn__s4 = 0LL;
                                                    {
                                                        long long __sn__e = 1LL;
                                                        while (rt_le_long(__sn__e, 1LL)) {
                                                            RtManagedArena *__loop_arena_8__ = rt_managed_arena_create_child(__loop_arena_6__);
                                                            {
                                                                long long __sn__s5 = 0LL;
                                                                {
                                                                    long long __sn__f = 1LL;
                                                                    while (rt_le_long(__sn__f, 1LL)) {
                                                                        RtManagedArena *__loop_arena_10__ = rt_managed_arena_create_child(__loop_arena_8__);
                                                                        {
                                                                            long long __sn__s6 = 0LL;
                                                                            {
                                                                                long long __sn__g = 1LL;
                                                                                while (rt_le_long(__sn__g, 1LL)) {
                                                                                    RtManagedArena *__loop_arena_12__ = rt_managed_arena_create_child(__loop_arena_10__);
                                                                                    {
                                                                                        long long __sn__s7 = 0LL;
                                                                                        {
                                                                                            long long __sn__h = 1LL;
                                                                                            while (rt_le_long(__sn__h, 1LL)) {
                                                                                                RtManagedArena *__loop_arena_14__ = rt_managed_arena_create_child(__loop_arena_12__);
                                                                                                {
                                                                                                    long long __sn__s8 = 0LL;
                                                                                                    {
                                                                                                        long long __sn__i = 1LL;
                                                                                                        while (rt_le_long(__sn__i, 1LL)) {
                                                                                                            RtManagedArena *__loop_arena_16__ = rt_managed_arena_create_child(__loop_arena_14__);
                                                                                                            {
                                                                                                                long long __sn__s9 = 0LL;
                                                                                                                {
                                                                                                                    long long __sn__j = 1LL;
                                                                                                                    while (rt_le_long(__sn__j, 1LL)) {
                                                                                                                        RtManagedArena *__loop_arena_18__ = rt_managed_arena_create_child(__loop_arena_16__);
                                                                                                                        {
                                                                                                                            long long __sn__inner = 11LL;
                                                                                                                            (__sn__s9 = rt_add_long(rt_add_long(__sn__s9, __sn__inner), 10LL));
                                                                                                                        }
                                                                                                                    __loop_cleanup_18__:
                                                                                                                        rt_managed_arena_destroy_child(__loop_arena_18__);
                                                                                                                    __for_continue_19__:;
                                                                                                                        rt_post_inc_long(&__sn__j);
                                                                                                                    }
                                                                                                                }
                                                                                                                (__sn__s8 = rt_add_long(rt_add_long(__sn__s8, __sn__s9), 9LL));
                                                                                                            }
                                                                                                        __loop_cleanup_16__:
                                                                                                            rt_managed_arena_destroy_child(__loop_arena_16__);
                                                                                                        __for_continue_17__:;
                                                                                                            rt_post_inc_long(&__sn__i);
                                                                                                        }
                                                                                                    }
                                                                                                    (__sn__s7 = rt_add_long(rt_add_long(__sn__s7, __sn__s8), 8LL));
                                                                                                }
                                                                                            __loop_cleanup_14__:
                                                                                                rt_managed_arena_destroy_child(__loop_arena_14__);
                                                                                            __for_continue_15__:;
                                                                                                rt_post_inc_long(&__sn__h);
                                                                                            }
                                                                                        }
                                                                                        (__sn__s6 = rt_add_long(rt_add_long(__sn__s6, __sn__s7), 7LL));
                                                                                    }
                                                                                __loop_cleanup_12__:
                                                                                    rt_managed_arena_destroy_child(__loop_arena_12__);
                                                                                __for_continue_13__:;
                                                                                    rt_post_inc_long(&__sn__g);
                                                                                }
                                                                            }
                                                                            (__sn__s5 = rt_add_long(rt_add_long(__sn__s5, __sn__s6), 6LL));
                                                                        }
                                                                    __loop_cleanup_10__:
                                                                        rt_managed_arena_destroy_child(__loop_arena_10__);
                                                                    __for_continue_11__:;
                                                                        rt_post_inc_long(&__sn__f);
                                                                    }
                                                                }
                                                                (__sn__s4 = rt_add_long(rt_add_long(__sn__s4, __sn__s5), 5LL));
                                                            }
                                                        __loop_cleanup_8__:
                                                            rt_managed_arena_destroy_child(__loop_arena_8__);
                                                        __for_continue_9__:;
                                                            rt_post_inc_long(&__sn__e);
                                                        }
                                                    }
                                                    (__sn__s3 = rt_add_long(rt_add_long(__sn__s3, __sn__s4), 4LL));
                                                }
                                            __loop_cleanup_6__:
                                                rt_managed_arena_destroy_child(__loop_arena_6__);
                                            __for_continue_7__:;
                                                rt_post_inc_long(&__sn__d);
                                            }
                                        }
                                        (__sn__s2 = rt_add_long(rt_add_long(__sn__s2, __sn__s3), 3LL));
                                    }
                                __loop_cleanup_4__:
                                    rt_managed_arena_destroy_child(__loop_arena_4__);
                                __for_continue_5__:;
                                    rt_post_inc_long(&__sn__c);
                                }
                            }
                            (__sn__s1 = rt_add_long(rt_add_long(__sn__s1, __sn__s2), 2LL));
                        }
                    __loop_cleanup_2__:
                        rt_managed_arena_destroy_child(__loop_arena_2__);
                    __for_continue_3__:;
                        rt_post_inc_long(&__sn__b);
                    }
                }
                (__sn__result = rt_add_long(rt_add_long(__sn__result, __sn__s1), 1LL));
            }
        __loop_cleanup_0__:
            rt_managed_arena_destroy_child(__loop_arena_0__);
        __for_continue_1__:;
            rt_post_inc_long(&__sn__a);
        }
    }
    _return_value = __sn__result;
    goto __sn__compute_chained_return;
__sn__compute_chained_return:
    rt_managed_arena_destroy_child(__local_arena__);
    return _return_value;
}

int main() {
    RtManagedArena *__local_arena__ = rt_managed_arena_create();
    __main_arena__ = __local_arena__;
    int _return_value = 0;
    rt_println("Private chained test (10 loops):");
    rt_println("Primitives escape one level at a time");
    long long __sn__computed = ({
    long long __intercept_result;
    if (__rt_interceptor_count > 0) {
        RtAny __args[1];
        __rt_thunk_args = __args;
        __rt_thunk_arena = __local_arena__;
        RtAny __intercepted = rt_call_intercepted("compute_chained", __args, 0, __thunk_0);
        __intercept_result = rt_unbox_int(__intercepted);
    } else {
        __intercept_result = __sn__compute_chained(__local_arena__);
    }
    __intercept_result;
});
    // 11 + 10 = 21, +9 = 30, +8 = 38, +7 = 45, +6 = 51, +5 = 56, +4 = 60, +3 = 63, +2 = 65, +1 = 66
    rt_println(({
        char *_p0 = rt_to_string_long(__local_arena__, __sn__computed);
        rt_str_concat(__local_arena__, "Computed checksum: ", _p0);
    }));
    if (rt_eq_long(__sn__computed, 66LL)) {
        {
            rt_println("PASS: Checksum correct - primitives escaped through all 10 levels");
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
    RtAny __result = rt_box_int(__sn__compute_chained((RtArena *)__rt_thunk_arena));
    return __result;
}

