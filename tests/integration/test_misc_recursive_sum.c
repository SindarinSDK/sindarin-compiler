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

long long __sn__rec_sum(RtManagedArena *, long long);

/* Interceptor thunk forward declarations */
static RtAny __thunk_0(void);
static RtAny __thunk_1(void);

int main() {
    RtManagedArena *__local_arena__ = rt_managed_arena_create();
    __main_arena__ = __local_arena__;
    int _return_value = 0;
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__local_arena__, ({
    long long __intercept_result;
    if (__rt_interceptor_count > 0) {
        RtAny __args[1];
        __args[0] = rt_box_int(10LL);
        __rt_thunk_args = __args;
        __rt_thunk_arena = __local_arena__;
        RtAny __intercepted = rt_call_intercepted("rec_sum", __args, 1, __thunk_0);
        __intercept_result = rt_unbox_int(__intercepted);
    } else {
        __intercept_result = __sn__rec_sum(__local_arena__, 10LL);
    }
    __intercept_result;
}));
        rt_str_concat(__local_arena__, _p0, "\n");
    });
        rt_print_string(_str_arg0);
    });
    goto main_return;
main_return:
    rt_managed_arena_destroy(__local_arena__);
    return _return_value;
}

long long __sn__rec_sum(RtManagedArena *__caller_arena__, long long __sn__n) {
    RtManagedArena *__local_arena__ = rt_managed_arena_create_child(__caller_arena__);
    long long _return_value = 0;
    if (rt_le_long(__sn__n, 0LL)) {
        {
            _return_value = 0LL;
            goto __sn__rec_sum_return;
        }
    }
    _return_value = rt_add_long(__sn__n, ({
    long long __intercept_result;
    if (__rt_interceptor_count > 0) {
        RtAny __args[1];
        __args[0] = rt_box_int(rt_sub_long(__sn__n, 1LL));
        __rt_thunk_args = __args;
        __rt_thunk_arena = __local_arena__;
        RtAny __intercepted = rt_call_intercepted("rec_sum", __args, 1, __thunk_1);
        __intercept_result = rt_unbox_int(__intercepted);
    } else {
        __intercept_result = __sn__rec_sum(__local_arena__, rt_sub_long(__sn__n, 1LL));
    }
    __intercept_result;
}));
    goto __sn__rec_sum_return;
__sn__rec_sum_return:
    rt_managed_arena_destroy_child(__local_arena__);
    return _return_value;
}


/* Interceptor thunk definitions */
static RtAny __thunk_0(void) {
    RtAny __result = rt_box_int(__sn__rec_sum((RtArena *)__rt_thunk_arena, rt_unbox_int(__rt_thunk_args[0])));
    return __result;
}

static RtAny __thunk_1(void) {
    RtAny __result = rt_box_int(__sn__rec_sum((RtArena *)__rt_thunk_arena, rt_unbox_int(__rt_thunk_args[0])));
    return __result;
}

