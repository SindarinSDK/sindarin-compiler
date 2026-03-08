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

long long __sn__apply(RtArenaV2 *, RtHandleV2 *, long long);
/* Lambda forward declarations */
static long long __lambda_0__(void *__closure__, RtArenaV2 *__caller_arena__, long long __sn__n);
long long __sn__apply(RtArenaV2 *__caller_arena__, RtHandleV2 * __sn__f, long long __sn__x) {
    rt_safepoint_poll();
    RtArenaV2 *__local_arena__ = __caller_arena__;
    long long _return_value = 0;

    _return_value = ((long long (*)(void *, RtArenaV2 *, long long))((__Closure__ *)__sn__f->ptr)->fn)(__sn__f, __local_arena__, __sn__x); goto __sn__apply_return;
__sn__apply_return:
    return _return_value;
}


int main() {
    rt_safepoint_init();
    rt_safepoint_thread_register();
    RtArenaV2 *__local_arena__ = rt_arena_v2_create(NULL, RT_ARENA_MODE_DEFAULT, "main");
    __main_arena__ = __local_arena__;
    int _return_value = 0;


    RtHandleV2 * __sn__double_it = ({
        RtHandleV2 *__cl_h__ = rt_arena_v2_alloc(__local_arena__, sizeof(__Closure__));
        __Closure__ *__cl__ = (__Closure__ *)__cl_h__->ptr;
        rt_handle_begin_transaction(__cl_h__);
        __cl__->fn = (void *)__lambda_0__;
        __cl__->arena = __local_arena__;
        __cl__->size = sizeof(__Closure__);
        rt_handle_end_transaction(__cl_h__);
        __cl_h__;
    });
    rt_assert_v2((__sn__apply(__local_arena__, __sn__double_it, 5LL) == 10LL), rt_arena_v2_strdup(__local_arena__, "expected apply(double_it, 5) to be 10"));
main_return:
    rt_arena_v2_condemn(__local_arena__);
    return _return_value;
}


/* Lambda function definitions */
static long long __lambda_0__(void *__closure__, RtArenaV2 *__caller_arena__, long long __sn__n) {
    rt_safepoint_poll();
    RtArenaV2 *__lambda_arena__ = __caller_arena__ ? __caller_arena__ : ((__Closure__ *)((RtHandleV2 *)__closure__)->ptr)->arena;
    RtArenaV2 *__local_arena__ = __lambda_arena__;
    return rt_mul_long(__sn__n, 2LL);
}
