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

RtHandle __sn__append_world(RtManagedArena *);

/* Interceptor thunk forward declarations */
static RtAny __thunk_0(void);

// Code Generation Test: Global with default function
//
// Tests that a default function uses its own arena for allocations,
// and can access global string variables from __main_arena__.
RtHandle __sn__greeting = RT_HANDLE_NULL;
RtHandle __sn__append_world(RtManagedArena *__caller_arena__) {
    RtManagedArena *__local_arena__ = rt_managed_arena_create_child(__caller_arena__);
    RtHandle _return_value = RT_HANDLE_NULL;
    // default function uses its own arena (__local_arena__)
    // global 'greeting' is in __main_arena__, accessed via parent-walking
    _return_value = rt_str_concat_h(__local_arena__, RT_HANDLE_NULL, (char *)rt_managed_pin(__local_arena__, __sn__greeting), " World");
    goto __sn__append_world_return;
__sn__append_world_return:
    _return_value = rt_managed_promote(__caller_arena__, __local_arena__, _return_value);
    rt_managed_arena_destroy_child(__local_arena__);
    return _return_value;
}

int main() {
    RtManagedArena *__local_arena__ = rt_managed_arena_create();
    __main_arena__ = __local_arena__;
    __sn__greeting = rt_managed_strdup(__main_arena__, RT_HANDLE_NULL, "Hello");
    int _return_value = 0;
    rt_println("Global with default function test:");
    RtThreadHandle *__result_pending__ = NULL;
    RtHandle __sn__result = ({
    RtHandle __intercept_result;
    if (__rt_interceptor_count > 0) {
        RtAny __args[1];
        __rt_thunk_args = __args;
        __rt_thunk_arena = __local_arena__;
        RtAny __intercepted = rt_call_intercepted("append_world", __args, 0, __thunk_0);
        __intercept_result = rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, rt_unbox_string(__intercepted));
    } else {
        __intercept_result = __sn__append_world(__local_arena__);
    }
    __intercept_result;
});
    rt_println((char *)rt_managed_pin(__local_arena__, __sn__result));
    rt_println("PASS");
    _return_value = 0LL;
    goto main_return;
main_return:
    rt_managed_arena_destroy(__local_arena__);
    return _return_value;
}


/* Interceptor thunk definitions */
static RtAny __thunk_0(void) {
    RtAny __result = rt_box_string((char *)rt_managed_pin((RtArena *)__rt_thunk_arena, __sn__append_world((RtArena *)__rt_thunk_arena)));
    return __result;
}

