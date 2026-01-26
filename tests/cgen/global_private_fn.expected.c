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

long long __sn__process_greeting(RtManagedArena *);

/* Interceptor thunk forward declarations */
static RtAny __thunk_0(void);

// Code Generation Test: Global with private function
//
// Tests that a private function creates its own private arena for allocations,
// can read global string variables from __main_arena__, and can modify
// global handles using __main_arena__ for the mutation.
RtHandle __sn__greeting = RT_HANDLE_NULL;
RtHandle __sn__message = RT_HANDLE_NULL;
long long __sn__process_greeting(RtManagedArena *__caller_arena__) {
    RtManagedArena *__local_arena__ = rt_managed_arena_create_child(__caller_arena__);
    long long _return_value = 0;
    // private function uses its own private arena (__local_arena__)
    // global 'greeting' is in __main_arena__, accessed via parent-walking
    // Mutating 'message' should use __main_arena__ since it's a global
    (__sn__message = rt_managed_promote(__main_arena__, __local_arena__, rt_str_concat_h(__local_arena__, RT_HANDLE_NULL, (char *)rt_managed_pin(__local_arena__, __sn__greeting), " World")));
    _return_value = (long)strlen((char *)rt_managed_pin(__local_arena__, __sn__greeting));
    goto __sn__process_greeting_return;
__sn__process_greeting_return:
    rt_managed_arena_destroy_child(__local_arena__);
    return _return_value;
}

int main() {
    RtManagedArena *__local_arena__ = rt_managed_arena_create();
    __main_arena__ = __local_arena__;
    __sn__greeting = rt_managed_strdup(__main_arena__, RT_HANDLE_NULL, "Hello");
    __sn__message = rt_managed_strdup(__main_arena__, RT_HANDLE_NULL, "");
    int _return_value = 0;
    rt_println("Global with private function test:");
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
    rt_println((char *)rt_managed_pin(__local_arena__, __sn__message));
    rt_println(({
        char *_p0 = rt_to_string_long(__local_arena__, __sn__result);
        rt_str_concat(__local_arena__, "Greeting length: ", _p0);
    }));
    rt_println("PASS");
    _return_value = 0LL;
    goto main_return;
main_return:
    rt_managed_arena_destroy(__local_arena__);
    return _return_value;
}


/* Interceptor thunk definitions */
static RtAny __thunk_0(void) {
    RtAny __result = rt_box_int(__sn__process_greeting((RtArena *)__rt_thunk_arena));
    return __result;
}

