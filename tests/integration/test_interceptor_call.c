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
RtAny __sn__myInterceptor(RtArenaV2 *, RtHandleV2 *, RtHandleV2 *, __Closure__ *);

/* Interceptor thunk forward declarations */
static RtAny __thunk_0(void);
static RtAny __thunk_1(void);
static RtAny __thunk_2(void);

// Test: Interceptor call site transformation
// Tests automatic interception of user-defined function calls
long long __sn__call_count = 0LL;
long long __sn__add(RtArenaV2 *__caller_arena__, long long __sn__a, long long __sn__b) {
    RtArenaV2 *__local_arena__ = rt_arena_v2_create(__caller_arena__, RT_ARENA_MODE_DEFAULT, "func");
    long long _return_value = 0;
    _return_value = rt_add_long(__sn__a, __sn__b);
    goto __sn__add_return;
__sn__add_return:
    rt_arena_v2_condemn(__local_arena__);
    return _return_value;
}

RtAny __sn__myInterceptor(RtArenaV2 *__caller_arena__, RtHandleV2 * __sn__name, RtHandleV2 * __sn__args, __Closure__ * __sn__continue_fn) {
    RtArenaV2 *__local_arena__ = rt_arena_v2_create(__caller_arena__, RT_ARENA_MODE_DEFAULT, "func");
    __sn__name = rt_arena_v2_clone(__local_arena__, __sn__name);
    RtAny _return_value = rt_box_nil();
    (__sn__call_count = rt_add_long(__sn__call_count, 1LL));
    rt_print_string(({
        RtHandleV2 *_rh = rt_str_concat_v2(__local_arena__, "Intercepting: ", ({ rt_handle_v2_pin(__sn__name); (char *)__sn__name->ptr; })); rt_handle_v2_pin(_rh); char *_r = (char *)_rh->ptr;
        _rh = rt_str_concat_v2(__local_arena__, _r, "\n"); rt_handle_v2_pin(_rh); _r = (char *)_rh->ptr;
        _r;
    }));
    // Return a modified result instead of calling continue_fn
    // This tests the case where an interceptor overrides the result entirely
    RtAny __sn__result = rt_box_int(42LL);
    _return_value = __sn__result;
    goto __sn__myInterceptor_return;
__sn__myInterceptor_return:
    _return_value = rt_any_promote_v2(__caller_arena__, _return_value);
    rt_arena_v2_condemn(__local_arena__);
    return _return_value;
}

int main() {
    RtArenaV2 *__local_arena__ = rt_arena_v2_create(NULL, RT_ARENA_MODE_DEFAULT, "main");
    __main_arena__ = __local_arena__;
    int _return_value = 0;
    // Without interceptor
    RtThread *__result_pending__ = NULL;
    long long __sn__result = ({
    long long __intercept_result;
    if (__rt_interceptor_count > 0) {
        RtAny __args[2];
        __args[0] = rt_box_int(10LL);
        __args[1] = rt_box_int(20LL);
        __rt_thunk_args = __args;
        __rt_thunk_arena = __local_arena__;
        RtAny __intercepted = rt_call_intercepted("add", __args, 2, __thunk_0);
        __intercept_result = rt_unbox_int(__intercepted);
    } else {
        __intercept_result = __sn__add(__local_arena__, 10LL, 20LL);
    }
    __intercept_result;
});
    rt_print_string(({
        RtHandleV2 *_ph0 = rt_to_string_long_v2(__local_arena__, __sn__result); rt_handle_v2_pin(_ph0); char *_p0 = (char *)_ph0->ptr;
        RtHandleV2 *_rh = rt_str_concat_v2(__local_arena__, "Result without interceptor: ", _p0); rt_handle_v2_pin(_rh); char *_r = (char *)_rh->ptr;
        _rh = rt_str_concat_v2(__local_arena__, _r, "\n"); rt_handle_v2_pin(_rh); _r = (char *)_rh->ptr;
        _r;
    }));
    if (rt_eq_long(__sn__result, 30LL)) {
        {
            rt_print_string("Direct call works - PASS\n");
        }
    }
    else {
        {
            rt_print_string("Direct call failed - FAIL\n");
        }
    }
    (rt_interceptor_register((RtInterceptHandler)__sn__myInterceptor), (void)0);
    rt_print_string(({
        RtHandleV2 *_ph0 = rt_to_string_long_v2(__local_arena__, rt_interceptor_count()); rt_handle_v2_pin(_ph0); char *_p0 = (char *)_ph0->ptr;
        RtHandleV2 *_rh = rt_str_concat_v2(__local_arena__, "Interceptor count: ", _p0); rt_handle_v2_pin(_rh); char *_r = (char *)_rh->ptr;
        _rh = rt_str_concat_v2(__local_arena__, _r, "\n"); rt_handle_v2_pin(_rh); _r = (char *)_rh->ptr;
        _r;
    }));
    // Call with interceptor - should return 42 from interceptor
    (__sn__result = ({
    long long __intercept_result;
    if (__rt_interceptor_count > 0) {
        RtAny __args[2];
        __args[0] = rt_box_int(5LL);
        __args[1] = rt_box_int(7LL);
        __rt_thunk_args = __args;
        __rt_thunk_arena = __local_arena__;
        RtAny __intercepted = rt_call_intercepted("add", __args, 2, __thunk_1);
        __intercept_result = rt_unbox_int(__intercepted);
    } else {
        __intercept_result = __sn__add(__local_arena__, 5LL, 7LL);
    }
    __intercept_result;
}));
    rt_print_string(({
        RtHandleV2 *_ph0 = rt_to_string_long_v2(__local_arena__, __sn__result); rt_handle_v2_pin(_ph0); char *_p0 = (char *)_ph0->ptr;
        RtHandleV2 *_rh = rt_str_concat_v2(__local_arena__, "Result with interceptor: ", _p0); rt_handle_v2_pin(_rh); char *_r = (char *)_rh->ptr;
        _rh = rt_str_concat_v2(__local_arena__, _r, "\n"); rt_handle_v2_pin(_rh); _r = (char *)_rh->ptr;
        _r;
    }));
    if (rt_eq_long(__sn__result, 42LL)) {
        {
            rt_print_string("Intercepted call returns modified result - PASS\n");
        }
    }
    else {
        {
            rt_print_string(({
        RtHandleV2 *_ph0 = rt_to_string_long_v2(__local_arena__, __sn__result); rt_handle_v2_pin(_ph0); char *_p0 = (char *)_ph0->ptr;
        RtHandleV2 *_rh = rt_str_concat_v2(__local_arena__, "Intercepted call failed, got ", _p0); rt_handle_v2_pin(_rh); char *_r = (char *)_rh->ptr;
        _rh = rt_str_concat_v2(__local_arena__, _r, " expected 42 - FAIL\n"); rt_handle_v2_pin(_rh); _r = (char *)_rh->ptr;
        _r;
    }));
        }
    }
    if (rt_eq_long(__sn__call_count, 1LL)) {
        {
            rt_print_string("Interceptor was called once - PASS\n");
        }
    }
    else {
        {
            rt_print_string(({
        RtHandleV2 *_ph0 = rt_to_string_long_v2(__local_arena__, __sn__call_count); rt_handle_v2_pin(_ph0); char *_p0 = (char *)_ph0->ptr;
        RtHandleV2 *_rh = rt_str_concat_v2(__local_arena__, "Interceptor call count wrong: ", _p0); rt_handle_v2_pin(_rh); char *_r = (char *)_rh->ptr;
        _rh = rt_str_concat_v2(__local_arena__, _r, " - FAIL\n"); rt_handle_v2_pin(_rh); _r = (char *)_rh->ptr;
        _r;
    }));
        }
    }
    (rt_interceptor_clear_all(), (void)0);
    (__sn__call_count = 0LL);
    (__sn__result = ({
    long long __intercept_result;
    if (__rt_interceptor_count > 0) {
        RtAny __args[2];
        __args[0] = rt_box_int(3LL);
        __args[1] = rt_box_int(4LL);
        __rt_thunk_args = __args;
        __rt_thunk_arena = __local_arena__;
        RtAny __intercepted = rt_call_intercepted("add", __args, 2, __thunk_2);
        __intercept_result = rt_unbox_int(__intercepted);
    } else {
        __intercept_result = __sn__add(__local_arena__, 3LL, 4LL);
    }
    __intercept_result;
}));
    if (rt_eq_long(__sn__result, 7LL)) {
        {
            rt_print_string("Direct call after clearAll works - PASS\n");
        }
    }
    else {
        {
            rt_print_string(({
        RtHandleV2 *_ph0 = rt_to_string_long_v2(__local_arena__, __sn__result); rt_handle_v2_pin(_ph0); char *_p0 = (char *)_ph0->ptr;
        RtHandleV2 *_rh = rt_str_concat_v2(__local_arena__, "Direct call after clearAll failed, got ", _p0); rt_handle_v2_pin(_rh); char *_r = (char *)_rh->ptr;
        _rh = rt_str_concat_v2(__local_arena__, _r, " - FAIL\n"); rt_handle_v2_pin(_rh); _r = (char *)_rh->ptr;
        _r;
    }));
        }
    }
    if (rt_eq_long(__sn__call_count, 0LL)) {
        {
            rt_print_string("No interception after clearAll - PASS\n");
        }
    }
    else {
        {
            rt_print_string("Interceptor still active after clearAll - FAIL\n");
        }
    }
    rt_print_string("All interceptor call tests passed!\n");
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

static RtAny __thunk_1(void) {
    RtAny __result = rt_box_int(__sn__add((RtArenaV2 *)__rt_thunk_arena, rt_unbox_int(__rt_thunk_args[0]), rt_unbox_int(__rt_thunk_args[1])));
    return __result;
}

static RtAny __thunk_2(void) {
    RtAny __result = rt_box_int(__sn__add((RtArenaV2 *)__rt_thunk_arena, rt_unbox_int(__rt_thunk_args[0]), rt_unbox_int(__rt_thunk_args[1])));
    return __result;
}

