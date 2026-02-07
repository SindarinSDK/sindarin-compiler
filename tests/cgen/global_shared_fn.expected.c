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

RtHandleV2 * __sn__append_world(RtArenaV2 *);

/* Interceptor thunk forward declarations */
static RtAny __thunk_0(void);

// Code Generation Test: Global with shared function
//
// Tests that a shared function uses the caller's arena for allocations,
// and can access global string variables from __main_arena__.
RtHandleV2 * __sn__greeting = RT_HANDLE_NULL;
RtHandleV2 * __sn__append_world(RtArenaV2 *__caller_arena__) {
    RtArenaV2 *__local_arena__ = __caller_arena__;
    RtHandleV2 * _return_value = RT_HANDLE_NULL;
    // shared function uses caller's arena for allocations
    // global 'greeting' is in __main_arena__, accessed via parent-walking
    _return_value = rt_str_concat_v2(__local_arena__, (char *)rt_handle_v2_pin(__sn__greeting), " World");
    goto __sn__append_world_return;
__sn__append_world_return:
    return _return_value;
}

int main() {
    RtArenaV2 *__local_arena__ = rt_arena_v2_create(NULL, RT_ARENA_MODE_DEFAULT, "main");
    __main_arena__ = __local_arena__;
    __sn__greeting = rt_arena_v2_strdup(__main_arena__, "Hello");
    int _return_value = 0;
    rt_println("Global with shared function test:");
    RtThreadHandle *__result_pending__ = NULL;
    RtHandleV2 * __sn__result = ({
    RtHandleV2 * __intercept_result;
    if (__rt_interceptor_count > 0) {
        RtAny __args[1];
        __rt_thunk_args = __args;
        __rt_thunk_arena = __local_arena__;
        RtAny __intercepted = rt_call_intercepted("append_world", __args, 0, __thunk_0);
        __intercept_result = rt_arena_v2_strdup(__local_arena__, rt_unbox_string(__intercepted));
    } else {
        __intercept_result = __sn__append_world(__local_arena__);
    }
    __intercept_result;
});
    rt_println((char *)rt_handle_v2_pin(__sn__result));
    rt_println("PASS");
    _return_value = 0LL;
    goto main_return;
main_return:
    rt_arena_v2_destroy(__local_arena__);
    return _return_value;
}


/* Interceptor thunk definitions */
static RtAny __thunk_0(void) {
    RtAny __result = rt_box_string((char *)rt_handle_v2_pin(__sn__append_world((RtArenaV2 *)__rt_thunk_arena)));
    return __result;
}

