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

long long __sn__Z_OK(RtManagedArena *);
long long __sn__Z_BEST_SPEED(RtManagedArena *);
long long __sn__Z_DATA_ERROR(RtManagedArena *);
double __sn__PI(RtManagedArena *);
uint64_t __sn__DEFAULT_PORT(RtManagedArena *);
unsigned char __sn__MAX_BYTE(RtManagedArena *);
char __sn__NEWLINE(RtManagedArena *);
bool __sn__IS_DEBUG(RtManagedArena *);
RtHandle __sn__GREETING(RtManagedArena *);
long long __sn__square(RtManagedArena *, long long);
long long __sn__negate(RtManagedArena *, long long);
bool __sn__is_positive(RtManagedArena *, long long);
bool __sn__is_zero(RtManagedArena *, long long);
float __sn__to_float(RtManagedArena *, long long);
long long __sn__byte_to_int(RtManagedArena *, unsigned char);
long long __sn__increment(RtManagedArena *, long long);
long long __sn__decrement(RtManagedArena *, long long);
long long __sn__add(RtManagedArena *, long long, long long);
long long __sn__multiply(RtManagedArena *, long long, long long);
long long __sn__max(RtManagedArena *, long long, long long);
long long __sn__min(RtManagedArena *, long long, long long);
double __sn__average(RtManagedArena *, double, double);
double __sn__area_circle(RtManagedArena *, double);
long long __sn__area_rect(RtManagedArena *, long long, long long);
double __sn__distance(RtManagedArena *, long long, long long);
unsigned char __sn__clamp_byte(RtManagedArena *, long long);
long long __sn__cube(RtManagedArena *, long long);
long long __sn__double_add(RtManagedArena *, long long, long long);
long long native_const(RtManagedArena *);
long long native_compute(RtManagedArena *, long long);

/* Interceptor thunk forward declarations */
static RtAny __thunk_0(void);
static RtAny __thunk_1(void);
static RtAny __thunk_2(void);
static RtAny __thunk_3(void);
static RtAny __thunk_4(void);
static RtAny __thunk_5(void);
static RtAny __thunk_6(void);
static RtAny __thunk_7(void);
static RtAny __thunk_8(void);
static RtAny __thunk_9(void);
static RtAny __thunk_10(void);
static RtAny __thunk_11(void);
static RtAny __thunk_12(void);
static RtAny __thunk_13(void);
static RtAny __thunk_14(void);
static RtAny __thunk_15(void);
static RtAny __thunk_16(void);
static RtAny __thunk_17(void);
static RtAny __thunk_18(void);
static RtAny __thunk_19(void);
static RtAny __thunk_20(void);
static RtAny __thunk_21(void);
static RtAny __thunk_22(void);
static RtAny __thunk_23(void);
static RtAny __thunk_24(void);
static RtAny __thunk_25(void);
static RtAny __thunk_26(void);
static RtAny __thunk_27(void);

// Comprehensive tests for expression-bodied functions
// --- Simple constant-like functions ---
long long __sn__Z_OK(RtManagedArena *__caller_arena__) {
    RtManagedArena *__local_arena__ = rt_managed_arena_create_child(__caller_arena__);
    long long _return_value = 0;
    _return_value = 0LL;
    goto __sn__Z_OK_return;
__sn__Z_OK_return:
    rt_managed_arena_destroy_child(__local_arena__);
    return _return_value;
}

long long __sn__Z_BEST_SPEED(RtManagedArena *__caller_arena__) {
    RtManagedArena *__local_arena__ = rt_managed_arena_create_child(__caller_arena__);
    long long _return_value = 0;
    _return_value = 1LL;
    goto __sn__Z_BEST_SPEED_return;
__sn__Z_BEST_SPEED_return:
    rt_managed_arena_destroy_child(__local_arena__);
    return _return_value;
}

long long __sn__Z_DATA_ERROR(RtManagedArena *__caller_arena__) {
    RtManagedArena *__local_arena__ = rt_managed_arena_create_child(__caller_arena__);
    long long _return_value = 0;
    _return_value = -3LL;
    goto __sn__Z_DATA_ERROR_return;
__sn__Z_DATA_ERROR_return:
    rt_managed_arena_destroy_child(__local_arena__);
    return _return_value;
}

double __sn__PI(RtManagedArena *__caller_arena__) {
    RtManagedArena *__local_arena__ = rt_managed_arena_create_child(__caller_arena__);
    double _return_value = 0;
    _return_value = 3.1415899999999999;
    goto __sn__PI_return;
__sn__PI_return:
    rt_managed_arena_destroy_child(__local_arena__);
    return _return_value;
}

uint64_t __sn__DEFAULT_PORT(RtManagedArena *__caller_arena__) {
    RtManagedArena *__local_arena__ = rt_managed_arena_create_child(__caller_arena__);
    uint64_t _return_value = 0;
    _return_value = 8080LL;
    goto __sn__DEFAULT_PORT_return;
__sn__DEFAULT_PORT_return:
    rt_managed_arena_destroy_child(__local_arena__);
    return _return_value;
}

unsigned char __sn__MAX_BYTE(RtManagedArena *__caller_arena__) {
    RtManagedArena *__local_arena__ = rt_managed_arena_create_child(__caller_arena__);
    unsigned char _return_value = 0;
    _return_value = 255LL;
    goto __sn__MAX_BYTE_return;
__sn__MAX_BYTE_return:
    rt_managed_arena_destroy_child(__local_arena__);
    return _return_value;
}

char __sn__NEWLINE(RtManagedArena *__caller_arena__) {
    RtManagedArena *__local_arena__ = rt_managed_arena_create_child(__caller_arena__);
    char _return_value = 0;
    _return_value = '\n';
    goto __sn__NEWLINE_return;
__sn__NEWLINE_return:
    rt_managed_arena_destroy_child(__local_arena__);
    return _return_value;
}

bool __sn__IS_DEBUG(RtManagedArena *__caller_arena__) {
    RtManagedArena *__local_arena__ = rt_managed_arena_create_child(__caller_arena__);
    bool _return_value = 0;
    _return_value = 0L;
    goto __sn__IS_DEBUG_return;
__sn__IS_DEBUG_return:
    rt_managed_arena_destroy_child(__local_arena__);
    return _return_value;
}

RtHandle __sn__GREETING(RtManagedArena *__caller_arena__) {
    RtManagedArena *__local_arena__ = rt_managed_arena_create_child(__caller_arena__);
    RtHandle _return_value = RT_HANDLE_NULL;
    _return_value = rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "Hello");
    goto __sn__GREETING_return;
__sn__GREETING_return:
    _return_value = rt_managed_promote(__caller_arena__, __local_arena__, _return_value);
    rt_managed_arena_destroy_child(__local_arena__);
    return _return_value;
}

// --- Single-parameter functions ---
long long __sn__square(RtManagedArena *__caller_arena__, long long __sn__x) {
    RtManagedArena *__local_arena__ = rt_managed_arena_create_child(__caller_arena__);
    long long _return_value = 0;
    _return_value = rt_mul_long(__sn__x, __sn__x);
    goto __sn__square_return;
__sn__square_return:
    rt_managed_arena_destroy_child(__local_arena__);
    return _return_value;
}

long long __sn__negate(RtManagedArena *__caller_arena__, long long __sn__x) {
    RtManagedArena *__local_arena__ = rt_managed_arena_create_child(__caller_arena__);
    long long _return_value = 0;
    _return_value = rt_neg_long(__sn__x);
    goto __sn__negate_return;
__sn__negate_return:
    rt_managed_arena_destroy_child(__local_arena__);
    return _return_value;
}

bool __sn__is_positive(RtManagedArena *__caller_arena__, long long __sn__x) {
    RtManagedArena *__local_arena__ = rt_managed_arena_create_child(__caller_arena__);
    bool _return_value = 0;
    _return_value = rt_gt_long(__sn__x, 0LL);
    goto __sn__is_positive_return;
__sn__is_positive_return:
    rt_managed_arena_destroy_child(__local_arena__);
    return _return_value;
}

bool __sn__is_zero(RtManagedArena *__caller_arena__, long long __sn__x) {
    RtManagedArena *__local_arena__ = rt_managed_arena_create_child(__caller_arena__);
    bool _return_value = 0;
    _return_value = rt_eq_long(__sn__x, 0LL);
    goto __sn__is_zero_return;
__sn__is_zero_return:
    rt_managed_arena_destroy_child(__local_arena__);
    return _return_value;
}

float __sn__to_float(RtManagedArena *__caller_arena__, long long __sn__x) {
    RtManagedArena *__local_arena__ = rt_managed_arena_create_child(__caller_arena__);
    float _return_value = 0;
    _return_value = ((float)(__sn__x));
    goto __sn__to_float_return;
__sn__to_float_return:
    rt_managed_arena_destroy_child(__local_arena__);
    return _return_value;
}

long long __sn__byte_to_int(RtManagedArena *__caller_arena__, unsigned char __sn__b) {
    RtManagedArena *__local_arena__ = rt_managed_arena_create_child(__caller_arena__);
    long long _return_value = 0;
    _return_value = ((long long)(__sn__b));
    goto __sn__byte_to_int_return;
__sn__byte_to_int_return:
    rt_managed_arena_destroy_child(__local_arena__);
    return _return_value;
}

long long __sn__increment(RtManagedArena *__caller_arena__, long long __sn__x) {
    RtManagedArena *__local_arena__ = rt_managed_arena_create_child(__caller_arena__);
    long long _return_value = 0;
    _return_value = rt_add_long(__sn__x, 1LL);
    goto __sn__increment_return;
__sn__increment_return:
    rt_managed_arena_destroy_child(__local_arena__);
    return _return_value;
}

long long __sn__decrement(RtManagedArena *__caller_arena__, long long __sn__x) {
    RtManagedArena *__local_arena__ = rt_managed_arena_create_child(__caller_arena__);
    long long _return_value = 0;
    _return_value = rt_sub_long(__sn__x, 1LL);
    goto __sn__decrement_return;
__sn__decrement_return:
    rt_managed_arena_destroy_child(__local_arena__);
    return _return_value;
}

// --- Multi-parameter functions ---
long long __sn__add(RtManagedArena *__caller_arena__, long long __sn__a, long long __sn__b) {
    RtManagedArena *__local_arena__ = rt_managed_arena_create_child(__caller_arena__);
    long long _return_value = 0;
    _return_value = rt_add_long(__sn__a, __sn__b);
    goto __sn__add_return;
__sn__add_return:
    rt_managed_arena_destroy_child(__local_arena__);
    return _return_value;
}

long long __sn__multiply(RtManagedArena *__caller_arena__, long long __sn__a, long long __sn__b) {
    RtManagedArena *__local_arena__ = rt_managed_arena_create_child(__caller_arena__);
    long long _return_value = 0;
    _return_value = rt_mul_long(__sn__a, __sn__b);
    goto __sn__multiply_return;
__sn__multiply_return:
    rt_managed_arena_destroy_child(__local_arena__);
    return _return_value;
}

long long __sn__max(RtManagedArena *__caller_arena__, long long __sn__a, long long __sn__b) {
    RtManagedArena *__local_arena__ = rt_managed_arena_create_child(__caller_arena__);
    long long _return_value = 0;
    _return_value = __sn__a;
    goto __sn__max_return;
__sn__max_return:
    rt_managed_arena_destroy_child(__local_arena__);
    return _return_value;
}

long long __sn__min(RtManagedArena *__caller_arena__, long long __sn__a, long long __sn__b) {
    RtManagedArena *__local_arena__ = rt_managed_arena_create_child(__caller_arena__);
    long long _return_value = 0;
    _return_value = __sn__b;
    goto __sn__min_return;
__sn__min_return:
    rt_managed_arena_destroy_child(__local_arena__);
    return _return_value;
}

double __sn__average(RtManagedArena *__caller_arena__, double __sn__a, double __sn__b) {
    RtManagedArena *__local_arena__ = rt_managed_arena_create_child(__caller_arena__);
    double _return_value = 0;
    _return_value = rt_div_double(rt_add_double(__sn__a, __sn__b), 2.0);
    goto __sn__average_return;
__sn__average_return:
    rt_managed_arena_destroy_child(__local_arena__);
    return _return_value;
}

// --- Functions with expressions ---
double __sn__area_circle(RtManagedArena *__caller_arena__, double __sn__radius) {
    RtManagedArena *__local_arena__ = rt_managed_arena_create_child(__caller_arena__);
    double _return_value = 0;
    _return_value = rt_mul_double(rt_mul_double(3.1415899999999999, __sn__radius), __sn__radius);
    goto __sn__area_circle_return;
__sn__area_circle_return:
    rt_managed_arena_destroy_child(__local_arena__);
    return _return_value;
}

long long __sn__area_rect(RtManagedArena *__caller_arena__, long long __sn__w, long long __sn__h) {
    RtManagedArena *__local_arena__ = rt_managed_arena_create_child(__caller_arena__);
    long long _return_value = 0;
    _return_value = rt_mul_long(__sn__w, __sn__h);
    goto __sn__area_rect_return;
__sn__area_rect_return:
    rt_managed_arena_destroy_child(__local_arena__);
    return _return_value;
}

double __sn__distance(RtManagedArena *__caller_arena__, long long __sn__x, long long __sn__y) {
    RtManagedArena *__local_arena__ = rt_managed_arena_create_child(__caller_arena__);
    double _return_value = 0;
    _return_value = ((double)(rt_add_long(rt_mul_long(__sn__x, __sn__x), rt_mul_long(__sn__y, __sn__y))));
    goto __sn__distance_return;
__sn__distance_return:
    rt_managed_arena_destroy_child(__local_arena__);
    return _return_value;
}

unsigned char __sn__clamp_byte(RtManagedArena *__caller_arena__, long long __sn__x) {
    RtManagedArena *__local_arena__ = rt_managed_arena_create_child(__caller_arena__);
    unsigned char _return_value = 0;
    _return_value = ((unsigned char)(rt_mod_long(__sn__x, 256LL)));
    goto __sn__clamp_byte_return;
__sn__clamp_byte_return:
    rt_managed_arena_destroy_child(__local_arena__);
    return _return_value;
}

// --- Functions calling other functions ---
long long __sn__cube(RtManagedArena *__caller_arena__, long long __sn__x) {
    RtManagedArena *__local_arena__ = rt_managed_arena_create_child(__caller_arena__);
    long long _return_value = 0;
    _return_value = rt_mul_long(__sn__x, ({
    long long __intercept_result;
    if (__rt_interceptor_count > 0) {
        RtAny __args[1];
        __args[0] = rt_box_int(__sn__x);
        __rt_thunk_args = __args;
        __rt_thunk_arena = __local_arena__;
        RtAny __intercepted = rt_call_intercepted("square", __args, 1, __thunk_0);
        __intercept_result = rt_unbox_int(__intercepted);
    } else {
        __intercept_result = __sn__square(__local_arena__, __sn__x);
    }
    __intercept_result;
}));
    goto __sn__cube_return;
__sn__cube_return:
    rt_managed_arena_destroy_child(__local_arena__);
    return _return_value;
}

long long __sn__double_add(RtManagedArena *__caller_arena__, long long __sn__a, long long __sn__b) {
    RtManagedArena *__local_arena__ = rt_managed_arena_create_child(__caller_arena__);
    long long _return_value = 0;
    _return_value = rt_mul_long(({
    long long __intercept_result;
    if (__rt_interceptor_count > 0) {
        RtAny __args[2];
        __args[0] = rt_box_int(__sn__a);
        __args[1] = rt_box_int(__sn__b);
        __rt_thunk_args = __args;
        __rt_thunk_arena = __local_arena__;
        RtAny __intercepted = rt_call_intercepted("add", __args, 2, __thunk_1);
        __intercept_result = rt_unbox_int(__intercepted);
    } else {
        __intercept_result = __sn__add(__local_arena__, __sn__a, __sn__b);
    }
    __intercept_result;
}), 2LL);
    goto __sn__double_add_return;
__sn__double_add_return:
    rt_managed_arena_destroy_child(__local_arena__);
    return _return_value;
}

// --- Native expression-bodied functions ---
long long native_const(RtManagedArena *__caller_arena__) {
    RtManagedArena *__local_arena__ = rt_managed_arena_create_child(__caller_arena__);
    long long _return_value = 0;
    _return_value = 42LL;
    goto native_const_return;
native_const_return:
    rt_managed_arena_destroy_child(__local_arena__);
    return _return_value;
}

long long native_compute(RtManagedArena *__caller_arena__, long long __sn__x) {
    RtManagedArena *__local_arena__ = rt_managed_arena_create_child(__caller_arena__);
    long long _return_value = 0;
    _return_value = rt_mul_long(__sn__x, 10LL);
    goto native_compute_return;
native_compute_return:
    rt_managed_arena_destroy_child(__local_arena__);
    return _return_value;
}

int main() {
    RtManagedArena *__local_arena__ = rt_managed_arena_create();
    __main_arena__ = __local_arena__;
    int _return_value = 0;
    rt_print_string("=== Expression-Bodied Function Tests ===\n\n");
    // Test constant functions
    rt_assert(rt_eq_long(({
    long long __intercept_result;
    if (__rt_interceptor_count > 0) {
        RtAny __args[1];
        __rt_thunk_args = __args;
        __rt_thunk_arena = __local_arena__;
        RtAny __intercepted = rt_call_intercepted("Z_OK", __args, 0, __thunk_2);
        __intercept_result = rt_unbox_int(__intercepted);
    } else {
        __intercept_result = __sn__Z_OK(__local_arena__);
    }
    __intercept_result;
}), 0LL), "Z_OK");
    rt_assert(rt_eq_long(({
    long long __intercept_result;
    if (__rt_interceptor_count > 0) {
        RtAny __args[1];
        __rt_thunk_args = __args;
        __rt_thunk_arena = __local_arena__;
        RtAny __intercepted = rt_call_intercepted("Z_BEST_SPEED", __args, 0, __thunk_3);
        __intercept_result = rt_unbox_int(__intercepted);
    } else {
        __intercept_result = __sn__Z_BEST_SPEED(__local_arena__);
    }
    __intercept_result;
}), 1LL), "Z_BEST_SPEED");
    rt_assert(rt_eq_long(({
    long long __intercept_result;
    if (__rt_interceptor_count > 0) {
        RtAny __args[1];
        __rt_thunk_args = __args;
        __rt_thunk_arena = __local_arena__;
        RtAny __intercepted = rt_call_intercepted("Z_DATA_ERROR", __args, 0, __thunk_4);
        __intercept_result = rt_unbox_int(__intercepted);
    } else {
        __intercept_result = __sn__Z_DATA_ERROR(__local_arena__);
    }
    __intercept_result;
}), -3LL), "Z_DATA_ERROR");
    rt_assert(((rt_gt_double(({
    double __intercept_result;
    if (__rt_interceptor_count > 0) {
        RtAny __args[1];
        __rt_thunk_args = __args;
        __rt_thunk_arena = __local_arena__;
        RtAny __intercepted = rt_call_intercepted("PI", __args, 0, __thunk_5);
        __intercept_result = rt_unbox_double(__intercepted);
    } else {
        __intercept_result = __sn__PI(__local_arena__);
    }
    __intercept_result;
}), 3.1400000000000001) != 0 && rt_lt_double(({
    double __intercept_result;
    if (__rt_interceptor_count > 0) {
        RtAny __args[1];
        __rt_thunk_args = __args;
        __rt_thunk_arena = __local_arena__;
        RtAny __intercepted = rt_call_intercepted("PI", __args, 0, __thunk_6);
        __intercept_result = rt_unbox_double(__intercepted);
    } else {
        __intercept_result = __sn__PI(__local_arena__);
    }
    __intercept_result;
}), 3.1499999999999999) != 0) ? 1L : 0L), "PI");
    rt_assert(rt_eq_long(({
    uint64_t __intercept_result;
    if (__rt_interceptor_count > 0) {
        RtAny __args[1];
        __rt_thunk_args = __args;
        __rt_thunk_arena = __local_arena__;
        RtAny __intercepted = rt_call_intercepted("DEFAULT_PORT", __args, 0, __thunk_7);
        __intercept_result = rt_unbox_uint(__intercepted);
    } else {
        __intercept_result = __sn__DEFAULT_PORT(__local_arena__);
    }
    __intercept_result;
}), 8080LL), "DEFAULT_PORT");
    rt_assert(rt_eq_long(({
    unsigned char __intercept_result;
    if (__rt_interceptor_count > 0) {
        RtAny __args[1];
        __rt_thunk_args = __args;
        __rt_thunk_arena = __local_arena__;
        RtAny __intercepted = rt_call_intercepted("MAX_BYTE", __args, 0, __thunk_8);
        __intercept_result = rt_unbox_byte(__intercepted);
    } else {
        __intercept_result = __sn__MAX_BYTE(__local_arena__);
    }
    __intercept_result;
}), 255LL), "MAX_BYTE");
    rt_assert(rt_eq_long(({
    char __intercept_result;
    if (__rt_interceptor_count > 0) {
        RtAny __args[1];
        __rt_thunk_args = __args;
        __rt_thunk_arena = __local_arena__;
        RtAny __intercepted = rt_call_intercepted("NEWLINE", __args, 0, __thunk_9);
        __intercept_result = rt_unbox_char(__intercepted);
    } else {
        __intercept_result = __sn__NEWLINE(__local_arena__);
    }
    __intercept_result;
}), '\n'), "NEWLINE");
    rt_assert(rt_not_bool(({
    bool __intercept_result;
    if (__rt_interceptor_count > 0) {
        RtAny __args[1];
        __rt_thunk_args = __args;
        __rt_thunk_arena = __local_arena__;
        RtAny __intercepted = rt_call_intercepted("IS_DEBUG", __args, 0, __thunk_10);
        __intercept_result = rt_unbox_bool(__intercepted);
    } else {
        __intercept_result = __sn__IS_DEBUG(__local_arena__);
    }
    __intercept_result;
})), "IS_DEBUG");
    rt_assert(rt_eq_string((char *)rt_managed_pin(__local_arena__, ({
    RtHandle __intercept_result;
    if (__rt_interceptor_count > 0) {
        RtAny __args[1];
        __rt_thunk_args = __args;
        __rt_thunk_arena = __local_arena__;
        RtAny __intercepted = rt_call_intercepted("GREETING", __args, 0, __thunk_11);
        __intercept_result = rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, rt_unbox_string(__intercepted));
    } else {
        __intercept_result = __sn__GREETING(__local_arena__);
    }
    __intercept_result;
})), "Hello"), "GREETING");
    rt_print_string("test_constant_functions: PASS\n");
    // Test single-parameter functions
    rt_assert(rt_eq_long(({
    long long __intercept_result;
    if (__rt_interceptor_count > 0) {
        RtAny __args[1];
        __args[0] = rt_box_int(5LL);
        __rt_thunk_args = __args;
        __rt_thunk_arena = __local_arena__;
        RtAny __intercepted = rt_call_intercepted("square", __args, 1, __thunk_12);
        __intercept_result = rt_unbox_int(__intercepted);
    } else {
        __intercept_result = __sn__square(__local_arena__, 5LL);
    }
    __intercept_result;
}), 25LL), "square");
    rt_assert(rt_eq_long(({
    long long __intercept_result;
    if (__rt_interceptor_count > 0) {
        RtAny __args[1];
        __args[0] = rt_box_int(10LL);
        __rt_thunk_args = __args;
        __rt_thunk_arena = __local_arena__;
        RtAny __intercepted = rt_call_intercepted("negate", __args, 1, __thunk_13);
        __intercept_result = rt_unbox_int(__intercepted);
    } else {
        __intercept_result = __sn__negate(__local_arena__, 10LL);
    }
    __intercept_result;
}), -10LL), "negate");
    rt_assert(({
    bool __intercept_result;
    if (__rt_interceptor_count > 0) {
        RtAny __args[1];
        __args[0] = rt_box_int(5LL);
        __rt_thunk_args = __args;
        __rt_thunk_arena = __local_arena__;
        RtAny __intercepted = rt_call_intercepted("is_positive", __args, 1, __thunk_14);
        __intercept_result = rt_unbox_bool(__intercepted);
    } else {
        __intercept_result = __sn__is_positive(__local_arena__, 5LL);
    }
    __intercept_result;
}), "is_positive true");
    rt_assert(rt_not_bool(({
    bool __intercept_result;
    if (__rt_interceptor_count > 0) {
        RtAny __args[1];
        __args[0] = rt_box_int(-5LL);
        __rt_thunk_args = __args;
        __rt_thunk_arena = __local_arena__;
        RtAny __intercepted = rt_call_intercepted("is_positive", __args, 1, __thunk_15);
        __intercept_result = rt_unbox_bool(__intercepted);
    } else {
        __intercept_result = __sn__is_positive(__local_arena__, -5LL);
    }
    __intercept_result;
})), "is_positive false");
    rt_assert(({
    bool __intercept_result;
    if (__rt_interceptor_count > 0) {
        RtAny __args[1];
        __args[0] = rt_box_int(0LL);
        __rt_thunk_args = __args;
        __rt_thunk_arena = __local_arena__;
        RtAny __intercepted = rt_call_intercepted("is_zero", __args, 1, __thunk_16);
        __intercept_result = rt_unbox_bool(__intercepted);
    } else {
        __intercept_result = __sn__is_zero(__local_arena__, 0LL);
    }
    __intercept_result;
}), "is_zero true");
    rt_assert(rt_not_bool(({
    bool __intercept_result;
    if (__rt_interceptor_count > 0) {
        RtAny __args[1];
        __args[0] = rt_box_int(1LL);
        __rt_thunk_args = __args;
        __rt_thunk_arena = __local_arena__;
        RtAny __intercepted = rt_call_intercepted("is_zero", __args, 1, __thunk_17);
        __intercept_result = rt_unbox_bool(__intercepted);
    } else {
        __intercept_result = __sn__is_zero(__local_arena__, 1LL);
    }
    __intercept_result;
})), "is_zero false");
    rt_assert(rt_eq_long(({
    long long __intercept_result;
    if (__rt_interceptor_count > 0) {
        RtAny __args[1];
        __args[0] = rt_box_int(200LL);
        __rt_thunk_args = __args;
        __rt_thunk_arena = __local_arena__;
        RtAny __intercepted = rt_call_intercepted("byte_to_int", __args, 1, __thunk_18);
        __intercept_result = rt_unbox_int(__intercepted);
    } else {
        __intercept_result = __sn__byte_to_int(__local_arena__, 200LL);
    }
    __intercept_result;
}), 200LL), "byte_to_int");
    rt_assert(rt_eq_long(({
    long long __intercept_result;
    if (__rt_interceptor_count > 0) {
        RtAny __args[1];
        __args[0] = rt_box_int(41LL);
        __rt_thunk_args = __args;
        __rt_thunk_arena = __local_arena__;
        RtAny __intercepted = rt_call_intercepted("increment", __args, 1, __thunk_19);
        __intercept_result = rt_unbox_int(__intercepted);
    } else {
        __intercept_result = __sn__increment(__local_arena__, 41LL);
    }
    __intercept_result;
}), 42LL), "increment");
    rt_assert(rt_eq_long(({
    long long __intercept_result;
    if (__rt_interceptor_count > 0) {
        RtAny __args[1];
        __args[0] = rt_box_int(43LL);
        __rt_thunk_args = __args;
        __rt_thunk_arena = __local_arena__;
        RtAny __intercepted = rt_call_intercepted("decrement", __args, 1, __thunk_20);
        __intercept_result = rt_unbox_int(__intercepted);
    } else {
        __intercept_result = __sn__decrement(__local_arena__, 43LL);
    }
    __intercept_result;
}), 42LL), "decrement");
    rt_print_string("test_single_param_functions: PASS\n");
    // Test multi-parameter functions
    rt_assert(rt_eq_long(({
    long long __intercept_result;
    if (__rt_interceptor_count > 0) {
        RtAny __args[2];
        __args[0] = rt_box_int(10LL);
        __args[1] = rt_box_int(32LL);
        __rt_thunk_args = __args;
        __rt_thunk_arena = __local_arena__;
        RtAny __intercepted = rt_call_intercepted("add", __args, 2, __thunk_21);
        __intercept_result = rt_unbox_int(__intercepted);
    } else {
        __intercept_result = __sn__add(__local_arena__, 10LL, 32LL);
    }
    __intercept_result;
}), 42LL), "add");
    rt_assert(rt_eq_long(({
    long long __intercept_result;
    if (__rt_interceptor_count > 0) {
        RtAny __args[2];
        __args[0] = rt_box_int(6LL);
        __args[1] = rt_box_int(7LL);
        __rt_thunk_args = __args;
        __rt_thunk_arena = __local_arena__;
        RtAny __intercepted = rt_call_intercepted("multiply", __args, 2, __thunk_22);
        __intercept_result = rt_unbox_int(__intercepted);
    } else {
        __intercept_result = __sn__multiply(__local_arena__, 6LL, 7LL);
    }
    __intercept_result;
}), 42LL), "multiply");
    rt_assert(rt_eq_double(({
    double __intercept_result;
    if (__rt_interceptor_count > 0) {
        RtAny __args[2];
        __args[0] = rt_box_double(40.0);
        __args[1] = rt_box_double(44.0);
        __rt_thunk_args = __args;
        __rt_thunk_arena = __local_arena__;
        RtAny __intercepted = rt_call_intercepted("average", __args, 2, __thunk_23);
        __intercept_result = rt_unbox_double(__intercepted);
    } else {
        __intercept_result = __sn__average(__local_arena__, 40.0, 44.0);
    }
    __intercept_result;
}), 42.0), "average");
    rt_print_string("test_multi_param_functions: PASS\n");
    // Test expression functions
    rt_assert(rt_eq_long(({
    long long __intercept_result;
    if (__rt_interceptor_count > 0) {
        RtAny __args[2];
        __args[0] = rt_box_int(6LL);
        __args[1] = rt_box_int(7LL);
        __rt_thunk_args = __args;
        __rt_thunk_arena = __local_arena__;
        RtAny __intercepted = rt_call_intercepted("area_rect", __args, 2, __thunk_24);
        __intercept_result = rt_unbox_int(__intercepted);
    } else {
        __intercept_result = __sn__area_rect(__local_arena__, 6LL, 7LL);
    }
    __intercept_result;
}), 42LL), "area_rect");
    rt_assert(rt_eq_long(({
    unsigned char __intercept_result;
    if (__rt_interceptor_count > 0) {
        RtAny __args[1];
        __args[0] = rt_box_int(300LL);
        __rt_thunk_args = __args;
        __rt_thunk_arena = __local_arena__;
        RtAny __intercepted = rt_call_intercepted("clamp_byte", __args, 1, __thunk_25);
        __intercept_result = rt_unbox_byte(__intercepted);
    } else {
        __intercept_result = __sn__clamp_byte(__local_arena__, 300LL);
    }
    __intercept_result;
}), 44LL), "clamp_byte");
    rt_print_string("test_expression_functions: PASS\n");
    // Test nested calls
    rt_assert(rt_eq_long(({
    long long __intercept_result;
    if (__rt_interceptor_count > 0) {
        RtAny __args[1];
        __args[0] = rt_box_int(3LL);
        __rt_thunk_args = __args;
        __rt_thunk_arena = __local_arena__;
        RtAny __intercepted = rt_call_intercepted("cube", __args, 1, __thunk_26);
        __intercept_result = rt_unbox_int(__intercepted);
    } else {
        __intercept_result = __sn__cube(__local_arena__, 3LL);
    }
    __intercept_result;
}), 27LL), "cube");
    rt_assert(rt_eq_long(({
    long long __intercept_result;
    if (__rt_interceptor_count > 0) {
        RtAny __args[2];
        __args[0] = rt_box_int(10LL);
        __args[1] = rt_box_int(11LL);
        __rt_thunk_args = __args;
        __rt_thunk_arena = __local_arena__;
        RtAny __intercepted = rt_call_intercepted("double_add", __args, 2, __thunk_27);
        __intercept_result = rt_unbox_int(__intercepted);
    } else {
        __intercept_result = __sn__double_add(__local_arena__, 10LL, 11LL);
    }
    __intercept_result;
}), 42LL), "double_add");
    rt_print_string("test_nested_calls: PASS\n");
    // Test native expression-bodied
    rt_assert(rt_eq_long(native_const(__local_arena__), 42LL), "native_const");
    rt_assert(rt_eq_long(native_compute(__local_arena__, 5LL), 50LL), "native_compute");
    rt_print_string("test_native_expression_bodied: PASS\n");
    rt_print_string("\nAll expression-bodied function tests passed!\n");
    goto main_return;
main_return:
    rt_managed_arena_destroy(__local_arena__);
    return _return_value;
}


/* Interceptor thunk definitions */
static RtAny __thunk_0(void) {
    RtAny __result = rt_box_int(__sn__square((RtArena *)__rt_thunk_arena, rt_unbox_int(__rt_thunk_args[0])));
    return __result;
}

static RtAny __thunk_1(void) {
    RtAny __result = rt_box_int(__sn__add((RtArena *)__rt_thunk_arena, rt_unbox_int(__rt_thunk_args[0]), rt_unbox_int(__rt_thunk_args[1])));
    return __result;
}

static RtAny __thunk_2(void) {
    RtAny __result = rt_box_int(__sn__Z_OK((RtArena *)__rt_thunk_arena));
    return __result;
}

static RtAny __thunk_3(void) {
    RtAny __result = rt_box_int(__sn__Z_BEST_SPEED((RtArena *)__rt_thunk_arena));
    return __result;
}

static RtAny __thunk_4(void) {
    RtAny __result = rt_box_int(__sn__Z_DATA_ERROR((RtArena *)__rt_thunk_arena));
    return __result;
}

static RtAny __thunk_5(void) {
    RtAny __result = rt_box_double(__sn__PI((RtArena *)__rt_thunk_arena));
    return __result;
}

static RtAny __thunk_6(void) {
    RtAny __result = rt_box_double(__sn__PI((RtArena *)__rt_thunk_arena));
    return __result;
}

static RtAny __thunk_7(void) {
    RtAny __result = rt_box_uint(__sn__DEFAULT_PORT((RtArena *)__rt_thunk_arena));
    return __result;
}

static RtAny __thunk_8(void) {
    RtAny __result = rt_box_byte(__sn__MAX_BYTE((RtArena *)__rt_thunk_arena));
    return __result;
}

static RtAny __thunk_9(void) {
    RtAny __result = rt_box_char(__sn__NEWLINE((RtArena *)__rt_thunk_arena));
    return __result;
}

static RtAny __thunk_10(void) {
    RtAny __result = rt_box_bool(__sn__IS_DEBUG((RtArena *)__rt_thunk_arena));
    return __result;
}

static RtAny __thunk_11(void) {
    RtAny __result = rt_box_string((char *)rt_managed_pin((RtArena *)__rt_thunk_arena, __sn__GREETING((RtArena *)__rt_thunk_arena)));
    return __result;
}

static RtAny __thunk_12(void) {
    RtAny __result = rt_box_int(__sn__square((RtArena *)__rt_thunk_arena, rt_unbox_int(__rt_thunk_args[0])));
    return __result;
}

static RtAny __thunk_13(void) {
    RtAny __result = rt_box_int(__sn__negate((RtArena *)__rt_thunk_arena, rt_unbox_int(__rt_thunk_args[0])));
    return __result;
}

static RtAny __thunk_14(void) {
    RtAny __result = rt_box_bool(__sn__is_positive((RtArena *)__rt_thunk_arena, rt_unbox_int(__rt_thunk_args[0])));
    return __result;
}

static RtAny __thunk_15(void) {
    RtAny __result = rt_box_bool(__sn__is_positive((RtArena *)__rt_thunk_arena, rt_unbox_int(__rt_thunk_args[0])));
    return __result;
}

static RtAny __thunk_16(void) {
    RtAny __result = rt_box_bool(__sn__is_zero((RtArena *)__rt_thunk_arena, rt_unbox_int(__rt_thunk_args[0])));
    return __result;
}

static RtAny __thunk_17(void) {
    RtAny __result = rt_box_bool(__sn__is_zero((RtArena *)__rt_thunk_arena, rt_unbox_int(__rt_thunk_args[0])));
    return __result;
}

static RtAny __thunk_18(void) {
    RtAny __result = rt_box_int(__sn__byte_to_int((RtArena *)__rt_thunk_arena, rt_unbox_byte(__rt_thunk_args[0])));
    return __result;
}

static RtAny __thunk_19(void) {
    RtAny __result = rt_box_int(__sn__increment((RtArena *)__rt_thunk_arena, rt_unbox_int(__rt_thunk_args[0])));
    return __result;
}

static RtAny __thunk_20(void) {
    RtAny __result = rt_box_int(__sn__decrement((RtArena *)__rt_thunk_arena, rt_unbox_int(__rt_thunk_args[0])));
    return __result;
}

static RtAny __thunk_21(void) {
    RtAny __result = rt_box_int(__sn__add((RtArena *)__rt_thunk_arena, rt_unbox_int(__rt_thunk_args[0]), rt_unbox_int(__rt_thunk_args[1])));
    return __result;
}

static RtAny __thunk_22(void) {
    RtAny __result = rt_box_int(__sn__multiply((RtArena *)__rt_thunk_arena, rt_unbox_int(__rt_thunk_args[0]), rt_unbox_int(__rt_thunk_args[1])));
    return __result;
}

static RtAny __thunk_23(void) {
    RtAny __result = rt_box_double(__sn__average((RtArena *)__rt_thunk_arena, rt_unbox_double(__rt_thunk_args[0]), rt_unbox_double(__rt_thunk_args[1])));
    return __result;
}

static RtAny __thunk_24(void) {
    RtAny __result = rt_box_int(__sn__area_rect((RtArena *)__rt_thunk_arena, rt_unbox_int(__rt_thunk_args[0]), rt_unbox_int(__rt_thunk_args[1])));
    return __result;
}

static RtAny __thunk_25(void) {
    RtAny __result = rt_box_byte(__sn__clamp_byte((RtArena *)__rt_thunk_arena, rt_unbox_int(__rt_thunk_args[0])));
    return __result;
}

static RtAny __thunk_26(void) {
    RtAny __result = rt_box_int(__sn__cube((RtArena *)__rt_thunk_arena, rt_unbox_int(__rt_thunk_args[0])));
    return __result;
}

static RtAny __thunk_27(void) {
    RtAny __result = rt_box_int(__sn__double_add((RtArena *)__rt_thunk_arena, rt_unbox_int(__rt_thunk_args[0]), rt_unbox_int(__rt_thunk_args[1])));
    return __result;
}

