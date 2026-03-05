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
typedef struct __Closure__ { void *fn; RtArenaV2 *arena; size_t size; } __Closure__;

/* Forward declarations */
static RtArenaV2 *__main_arena__ = NULL;

long long __sn__add(RtArenaV2 *, long long, long long);

/* Interceptor thunk forward declarations */
static RtAny __thunk_0(void);

// Code Generation Test: Arena inference for pure functions
//
// Tests that a pure function (no arena allocations) uses __caller_arena__
// alias instead of creating a child arena.
long long __sn__add(RtArenaV2 *__caller_arena__, long long __sn__a, long long __sn__b) {
    rt_safepoint_poll();
    RtArenaV2 *__local_arena__ = __caller_arena__;
    long long _return_value = 0;
    _return_value = rt_add_long(__sn__a, __sn__b);
    goto __sn__add_return;
__sn__add_return:
    return _return_value;
}

int main() {
    rt_safepoint_init();
    rt_safepoint_thread_register();
    RtArenaV2 *__local_arena__ = rt_arena_v2_create(NULL, RT_ARENA_MODE_DEFAULT, "main");
    __main_arena__ = __local_arena__;
    int _return_value = 0;
    RtHandleV2 *__result_pending__ = NULL;
    long long __sn__result = ({
    long long __intercept_result;
    if (__rt_interceptor_count > 0) {
        RtAny __args[2];
        __args[0] = rt_box_int(1LL);
        __args[1] = rt_box_int(2LL);
        __rt_thunk_args = __args;
        __rt_thunk_arena = __local_arena__;
        RtAny __intercepted = rt_call_intercepted(rt_arena_v2_strdup(__local_arena__, "add"), __args, 2, __thunk_0);
        __intercept_result = rt_unbox_int(__intercepted);
    } else {
        __intercept_result = __sn__add(__local_arena__, 1LL, 2LL);
    }
    __intercept_result;
});
    rt_println_v2(rt_to_string_long_v2(__local_arena__, __sn__result));
    _return_value = 0LL;
    goto main_return;
main_return:
    rt_arena_v2_condemn(__local_arena__);
    return _return_value;
}


/* Interceptor thunk definitions */
static RtAny __thunk_0(void) {
    RtAny __result = rt_box_int(__sn__add((RtArenaV2 *)__rt_thunk_arena, rt_unbox_int(__rt_thunk_args[0]), rt_unbox_int(__rt_thunk_args[1])));
    return __result;
}

