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

long long __sn__add(RtManagedArena *, long long, long long);
long long __sn__mul(RtManagedArena *, long long, long long);

/* Interceptor thunk forward declarations */
static RtAny __thunk_0(void);
static RtAny __thunk_1(void);
static RtAny __thunk_2(void);

int main() {
    RtManagedArena *__local_arena__ = rt_managed_arena_create();
    __main_arena__ = __local_arena__;
    int _return_value = 0;
    RtThreadHandle *__result_pending__ = NULL;
    long long __sn__result = ({
    long long __iarg_2_0 = ({
    long long __intercept_result;
    if (__rt_interceptor_count > 0) {
        RtAny __args[2];
        __args[0] = rt_box_int(2LL);
        __args[1] = rt_box_int(3LL);
        __rt_thunk_args = __args;
        __rt_thunk_arena = __local_arena__;
        RtAny __intercepted = rt_call_intercepted("mul", __args, 2, __thunk_0);
        __intercept_result = rt_unbox_int(__intercepted);
    } else {
        __intercept_result = __sn__mul(__local_arena__, 2LL, 3LL);
    }
    __intercept_result;
});
    long long __iarg_2_1 = ({
    long long __intercept_result;
    if (__rt_interceptor_count > 0) {
        RtAny __args[2];
        __args[0] = rt_box_int(4LL);
        __args[1] = rt_box_int(5LL);
        __rt_thunk_args = __args;
        __rt_thunk_arena = __local_arena__;
        RtAny __intercepted = rt_call_intercepted("mul", __args, 2, __thunk_1);
        __intercept_result = rt_unbox_int(__intercepted);
    } else {
        __intercept_result = __sn__mul(__local_arena__, 4LL, 5LL);
    }
    __intercept_result;
});
    long long __intercept_result;
    if (__rt_interceptor_count > 0) {
        RtAny __args[2];
        __args[0] = rt_box_int(__iarg_2_0);
        __args[1] = rt_box_int(__iarg_2_1);
        __rt_thunk_args = __args;
        __rt_thunk_arena = __local_arena__;
        RtAny __intercepted = rt_call_intercepted("add", __args, 2, __thunk_2);
        __intercept_result = rt_unbox_int(__intercepted);
    } else {
        __intercept_result = __sn__add(__local_arena__, __iarg_2_0, __iarg_2_1);
    }
    __intercept_result;
});
    rt_print_string(({
        char *_p0 = rt_to_string_long(__local_arena__, __sn__result);
        rt_str_concat(__local_arena__, _p0, "\n");
    }));
    goto main_return;
main_return:
    rt_managed_arena_destroy(__local_arena__);
    return _return_value;
}

long long __sn__add(RtManagedArena *__caller_arena__, long long __sn__a, long long __sn__b) {
    RtManagedArena *__local_arena__ = rt_managed_arena_create_child(__caller_arena__);
    long long _return_value = 0;
    _return_value = rt_add_long(__sn__a, __sn__b);
    goto __sn__add_return;
__sn__add_return:
    rt_managed_arena_destroy_child(__local_arena__);
    return _return_value;
}

long long __sn__mul(RtManagedArena *__caller_arena__, long long __sn__a, long long __sn__b) {
    RtManagedArena *__local_arena__ = rt_managed_arena_create_child(__caller_arena__);
    long long _return_value = 0;
    _return_value = rt_mul_long(__sn__a, __sn__b);
    goto __sn__mul_return;
__sn__mul_return:
    rt_managed_arena_destroy_child(__local_arena__);
    return _return_value;
}


/* Interceptor thunk definitions */
static RtAny __thunk_0(void) {
    RtAny __result = rt_box_int(__sn__mul((RtArena *)__rt_thunk_arena, rt_unbox_int(__rt_thunk_args[0]), rt_unbox_int(__rt_thunk_args[1])));
    return __result;
}

static RtAny __thunk_1(void) {
    RtAny __result = rt_box_int(__sn__mul((RtArena *)__rt_thunk_arena, rt_unbox_int(__rt_thunk_args[0]), rt_unbox_int(__rt_thunk_args[1])));
    return __result;
}

static RtAny __thunk_2(void) {
    RtAny __result = rt_box_int(__sn__add((RtArena *)__rt_thunk_arena, rt_unbox_int(__rt_thunk_args[0]), rt_unbox_int(__rt_thunk_args[1])));
    return __result;
}

