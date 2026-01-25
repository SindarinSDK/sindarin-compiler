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

RtHandle __sn__build_chained(RtManagedArena *);

/* Interceptor thunk forward declarations */
static RtAny __thunk_0(void);

RtHandle __sn__build_chained(RtManagedArena *__caller_arena__) {
    RtManagedArena *__local_arena__ = rt_managed_arena_create_child(__caller_arena__);
    RtHandle _return_value = RT_HANDLE_NULL;
    // Code Generation Test: 3 Nested Loop Arena Escape (Chained)
    //
    // Tests that string values escape correctly through 3 nested loops,
    // one level at a time: depth 4 -> 3 -> 2 -> 1
    RtHandle __sn__result = rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "");
    {
        long long __sn__i = 1LL;
        while (rt_le_long(__sn__i, 2LL)) {
            RtManagedArena *__loop_arena_0__ = rt_managed_arena_create_child(__local_arena__);
            {
                RtHandle __sn__outer = rt_managed_strdup(__loop_arena_0__, RT_HANDLE_NULL, "");
                {
                    long long __sn__j = 1LL;
                    while (rt_le_long(__sn__j, 2LL)) {
                        RtManagedArena *__loop_arena_2__ = rt_managed_arena_create_child(__loop_arena_0__);
                        {
                            RtHandle __sn__middle = rt_managed_strdup(__loop_arena_2__, RT_HANDLE_NULL, "");
                            {
                                long long __sn__k = 1LL;
                                while (rt_le_long(__sn__k, 2LL)) {
                                    RtManagedArena *__loop_arena_4__ = rt_managed_arena_create_child(__loop_arena_2__);
                                    {
                                        RtHandle __sn__inner = ({
        char *_p0 = rt_to_string_long(__loop_arena_4__, __sn__i);
        char *_p1 = rt_to_string_long(__loop_arena_4__, __sn__j);
        char *_p2 = rt_to_string_long(__loop_arena_4__, __sn__k);
        char *_r = rt_str_concat(__loop_arena_4__, "[d4:i=", _p0);
        _r = rt_str_concat(__loop_arena_4__, _r, ",j=");
        _r = rt_str_concat(__loop_arena_4__, _r, _p1);
        _r = rt_str_concat(__loop_arena_4__, _r, ",k=");
        _r = rt_str_concat(__loop_arena_4__, _r, _p2);
        _r = rt_str_concat(__loop_arena_4__, _r, "]");
        rt_managed_strdup(__loop_arena_4__, RT_HANDLE_NULL, _r);
    });
                                        // Escape from d4 to d3
                                        (__sn__middle = rt_managed_clone(__loop_arena_2__, __loop_arena_4__, rt_str_concat_h(__loop_arena_4__, RT_HANDLE_NULL, (char *)rt_managed_pin(__loop_arena_4__, rt_str_concat_h(__loop_arena_4__, RT_HANDLE_NULL, (char *)rt_managed_pin(__loop_arena_2__, __sn__middle), (char *)rt_managed_pin(__loop_arena_4__, __sn__inner))), "->d3 ")));
                                    }
                                __loop_cleanup_4__:
                                    rt_managed_arena_destroy_child(__loop_arena_4__);
                                __for_continue_5__:;
                                    rt_post_inc_long(&__sn__k);
                                }
                            }
                            (__sn__outer = rt_managed_clone(__loop_arena_0__, __loop_arena_2__, rt_str_concat_h(__loop_arena_2__, RT_HANDLE_NULL, (char *)rt_managed_pin(__loop_arena_2__, rt_str_concat_h(__loop_arena_2__, RT_HANDLE_NULL, (char *)rt_managed_pin(__loop_arena_2__, rt_str_concat_h(__loop_arena_2__, RT_HANDLE_NULL, (char *)rt_managed_pin(__loop_arena_0__, __sn__outer), "(")), (char *)rt_managed_pin(__loop_arena_2__, __sn__middle))), ")->d2 ")));
                        }
                    __loop_cleanup_2__:
                        rt_managed_arena_destroy_child(__loop_arena_2__);
                    __for_continue_3__:;
                        rt_post_inc_long(&__sn__j);
                    }
                }
                (__sn__result = rt_managed_clone(__local_arena__, __loop_arena_0__, rt_str_concat_h(__loop_arena_0__, RT_HANDLE_NULL, (char *)rt_managed_pin(__loop_arena_0__, rt_str_concat_h(__loop_arena_0__, RT_HANDLE_NULL, (char *)rt_managed_pin(__loop_arena_0__, rt_str_concat_h(__loop_arena_0__, RT_HANDLE_NULL, (char *)rt_managed_pin(__local_arena__, __sn__result), "<")), (char *)rt_managed_pin(__loop_arena_0__, __sn__outer))), ">->d1\n")));
            }
        __loop_cleanup_0__:
            rt_managed_arena_destroy_child(__loop_arena_0__);
        __for_continue_1__:;
            rt_post_inc_long(&__sn__i);
        }
    }
    _return_value = __sn__result;
    goto __sn__build_chained_return;
__sn__build_chained_return:
    _return_value = rt_managed_promote(__caller_arena__, __local_arena__, _return_value);
    rt_managed_arena_destroy_child(__local_arena__);
    return _return_value;
}

int main() {
    RtManagedArena *__local_arena__ = rt_managed_arena_create();
    __main_arena__ = __local_arena__;
    int _return_value = 0;
    rt_println("Chained escape test (3 loops):");
    rt_println("Each value escapes one level at a time");
    rt_println((char *)rt_managed_pin(__local_arena__, ({
    RtHandle __intercept_result;
    if (__rt_interceptor_count > 0) {
        RtAny __args[1];
        __rt_thunk_args = __args;
        __rt_thunk_arena = __local_arena__;
        RtAny __intercepted = rt_call_intercepted("build_chained", __args, 0, __thunk_0);
        __intercept_result = rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, rt_unbox_string(__intercepted));
    } else {
        __intercept_result = __sn__build_chained(__local_arena__);
    }
    __intercept_result;
})));
    rt_println("PASS: All values survived chained escape");
    _return_value = 0LL;
    goto main_return;
main_return:
    rt_managed_arena_destroy(__local_arena__);
    return _return_value;
}


/* Interceptor thunk definitions */
static RtAny __thunk_0(void) {
    RtAny __result = rt_box_string((char *)rt_managed_pin((RtArena *)__rt_thunk_arena, __sn__build_chained((RtArena *)__rt_thunk_arena)));
    return __result;
}

