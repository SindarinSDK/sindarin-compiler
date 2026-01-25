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
    long long __sn__to_d1 = 0LL;
    {
        long long __sn__i = 1LL;
        while (rt_le_long(__sn__i, 2LL)) {
            RtManagedArena *__loop_arena_0__ = rt_managed_arena_create_child(__local_arena__);
            {
                long long __sn__to_d2 = 0LL;
                {
                    long long __sn__j = 1LL;
                    while (rt_le_long(__sn__j, 2LL)) {
                        RtManagedArena *__loop_arena_2__ = rt_managed_arena_create_child(__loop_arena_0__);
                        {
                            long long __sn__to_d3 = 0LL;
                            {
                                long long __sn__k = 1LL;
                                while (rt_le_long(__sn__k, 2LL)) {
                                    RtManagedArena *__loop_arena_4__ = rt_managed_arena_create_child(__loop_arena_2__);
                                    {
                                        long long __sn__value = rt_add_long(rt_add_long(rt_mul_long(__sn__i, 100LL), rt_mul_long(__sn__j, 10LL)), __sn__k);
                                        (__sn__to_d3 = rt_add_long(__sn__to_d3, __sn__value));
                                        (__sn__to_d2 = rt_add_long(__sn__to_d2, rt_mul_long(__sn__value, 2LL)));
                                        (__sn__to_d1 = rt_add_long(__sn__to_d1, rt_mul_long(__sn__value, 3LL)));
                                    }
                                __loop_cleanup_4__:
                                    rt_managed_arena_destroy_child(__loop_arena_4__);
                                __for_continue_5__:;
                                    rt_post_inc_long(&__sn__k);
                                }
                            }
                            (__sn__to_d2 = rt_add_long(__sn__to_d2, __sn__to_d3));
                        }
                    __loop_cleanup_2__:
                        rt_managed_arena_destroy_child(__loop_arena_2__);
                    __for_continue_3__:;
                        rt_post_inc_long(&__sn__j);
                    }
                }
                (__sn__to_d1 = rt_add_long(__sn__to_d1, __sn__to_d2));
            }
        __loop_cleanup_0__:
            rt_managed_arena_destroy_child(__loop_arena_0__);
        __for_continue_1__:;
            rt_post_inc_long(&__sn__i);
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
    rt_println("Private jump test (3 loops):");
    rt_println("Primitives jump directly to outer scopes");
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
    rt_println(({
        char *_p0 = rt_to_string_long(__local_arena__, __sn__computed);
        rt_str_concat(__local_arena__, "Computed checksum: ", _p0);
    }));
    if (rt_eq_long(__sn__computed, 7992LL)) {
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

