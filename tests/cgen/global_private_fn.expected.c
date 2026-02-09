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

long long __sn__process_greeting(RtArenaV2 *);

/* Interceptor thunk forward declarations */
static RtAny __thunk_0(void);

// Code Generation Test: Global with private function
//
// Tests that a private function creates its own private arena for allocations,
// can read global string variables from __main_arena__, and can modify
// global handles using __main_arena__ for the mutation.
RtHandleV2 * __sn__greeting = NULL;
RtHandleV2 * __sn__message = NULL;
long long __sn__process_greeting(RtArenaV2 *__caller_arena__) {
    RtArenaV2 *__local_arena__ = rt_arena_v2_create(__caller_arena__, RT_ARENA_MODE_PRIVATE, "func");
    long long _return_value = 0;
    // private function uses its own private arena (__local_arena__)
    // global 'greeting' is in __main_arena__, accessed via parent-walking
    // Mutating 'message' should use __main_arena__ since it's a global
    (__sn__message = rt_arena_v2_promote(__main_arena__, rt_str_concat_v2(__local_arena__, (char *)rt_handle_v2_pin(__sn__greeting), " World")));
    _return_value = (long)strlen((char *)rt_handle_v2_pin(__sn__greeting));
    goto __sn__process_greeting_return;
__sn__process_greeting_return:
    rt_arena_v2_destroy(__local_arena__);
    return _return_value;
}

int main() {
    RtArenaV2 *__local_arena__ = rt_arena_v2_create(NULL, RT_ARENA_MODE_DEFAULT, "main");
    __main_arena__ = __local_arena__;
    __sn__greeting = rt_arena_v2_strdup(__main_arena__, "Hello");
    __sn__message = rt_arena_v2_strdup(__main_arena__, "");
    int _return_value = 0;
    rt_println("Global with private function test:");
    RtThread *__result_pending__ = NULL;
    long long __sn__result = ({
    long long __intercept_result;
    if (__rt_interceptor_count > 0) {
        RtAny __args[1];
        __rt_thunk_args = __args;
        __rt_thunk_arena = __local_arena__;
        RtAny __intercepted = rt_call_intercepted("process_greeting", __args, 0, __thunk_0);
        __intercept_result = rt_unbox_int(__intercepted);
    } else {
        __intercept_result = __sn__process_greeting(__local_arena__);
    }
    __intercept_result;
});
    rt_println((char *)rt_handle_v2_pin(__sn__message));
    rt_println(({
        char *_p0 = rt_to_string_long_raw_v2(__local_arena__, __sn__result);
        rt_str_concat_raw_v2(__local_arena__, "Greeting length: ", _p0);
    }));
    rt_println("PASS");
    _return_value = 0LL;
    goto main_return;
main_return:
    rt_arena_v2_destroy(__local_arena__);
    return _return_value;
}


/* Interceptor thunk definitions */
static RtAny __thunk_0(void) {
    RtAny __result = rt_box_int(__sn__process_greeting((RtArenaV2 *)__rt_thunk_arena));
    return __result;
}

