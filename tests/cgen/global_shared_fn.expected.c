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

// Code Generation Test: Global with function
//
// Tests that a function can access global string variables.
RtHandleV2 * __sn__greeting = NULL;
RtHandleV2 * __sn__append_world(RtArenaV2 *__caller_arena__) {
    rt_safepoint_poll();
    RtArenaV2 *__local_arena__ = rt_arena_v2_create(__caller_arena__, RT_ARENA_MODE_DEFAULT, "func");
    RtHandleV2 * _return_value = NULL;
    // function accesses global 'greeting'
    RtHandleV2 *__htmp_0__ = rt_arena_v2_strdup(__local_arena__, " World");
    RtHandleV2 *__htmp_1__ = rt_str_concat_v2(__local_arena__, rt_arena_v2_clone(__local_arena__, __sn__greeting), __htmp_0__);
    _return_value = __htmp_1__;
    goto __sn__append_world_return;
__sn__append_world_return:
    if (_return_value && _return_value->arena == __local_arena__)
        _return_value = rt_arena_v2_promote(__caller_arena__, _return_value);
    else if (_return_value)
        _return_value = rt_arena_v2_clone(__caller_arena__, _return_value);
    rt_arena_v2_condemn(__local_arena__);
    return _return_value;
}

int main() {
    rt_safepoint_init();
    rt_safepoint_thread_register();
    RtArenaV2 *__local_arena__ = rt_arena_v2_create(NULL, RT_ARENA_MODE_DEFAULT, "main");
    __main_arena__ = __local_arena__;
    __sn__greeting = rt_arena_v2_strdup(__main_arena__, "Hello");
    int _return_value = 0;
    RtHandleV2 *__htmp_0__ = rt_arena_v2_strdup(__local_arena__, "Global with function test:");
    RtHandleV2 *__htmp_1__ = rt_arena_v2_strdup(__local_arena__, "Global with function test:");
    rt_println_v2(__htmp_1__);
    RtHandleV2 *__result_pending__ = NULL;
    RtHandleV2 * __sn__result = ({
    RtHandleV2 * __intercept_result;
    if (__rt_interceptor_count > 0) {
        RtAny __args[1];
        __rt_thunk_args = __args;
        __rt_thunk_arena = __local_arena__;
        RtAny __intercepted = rt_call_intercepted(rt_arena_v2_strdup(__local_arena__, "append_world"), __args, 0, __thunk_0);
        __intercept_result = rt_unbox_string_v2(__intercepted);
    } else {
        __intercept_result = __sn__append_world(__local_arena__);
    }
    __intercept_result;
});
    rt_println_v2(__sn__result);
    RtHandleV2 *__htmp_2__ = rt_arena_v2_strdup(__local_arena__, "PASS");
    RtHandleV2 *__htmp_3__ = rt_arena_v2_strdup(__local_arena__, "PASS");
    rt_println_v2(__htmp_3__);
    _return_value = 0LL;
    goto main_return;
main_return:
    rt_arena_v2_condemn(__local_arena__);
    return _return_value;
}


/* Interceptor thunk definitions */
static RtAny __thunk_0(void) {
    RtAny __result = rt_box_string_v2(__sn__append_world((RtArenaV2 *)__rt_thunk_arena));
    return __result;
}

