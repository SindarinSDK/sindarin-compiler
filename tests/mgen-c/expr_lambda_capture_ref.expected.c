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


/* Lambda forward declarations */
typedef struct __closure_0__ {
    void *fn;
    RtArenaV2 *arena;
    size_t size;
    RtHandleV2 *x;
} __closure_0__;
static void __lambda_0__(void *__closure__, RtArenaV2 *__caller_arena__);

int main() {
    rt_safepoint_init();
    rt_safepoint_thread_register();
    RtArenaV2 *__local_arena__ = rt_arena_v2_create(NULL, RT_ARENA_MODE_DEFAULT, "main");
    __main_arena__ = __local_arena__;
    int _return_value = 0;

    RtHandleV2 *____sn__x_h__ = rt_arena_v2_alloc(__local_arena__, sizeof(long long)); long long *__sn__x = (long long *)____sn__x_h__->ptr; *__sn__x = 10LL;
    RtHandleV2 * __sn__inc = ({
        RtHandleV2 *__cl_h__ = rt_arena_v2_alloc(__local_arena__, sizeof(__closure_0__));
        __closure_0__ *__cl__ = (__closure_0__ *)__cl_h__->ptr;
        rt_handle_begin_transaction(__cl_h__);
        __cl__->fn = (void *)__lambda_0__;
        __cl__->arena = __local_arena__;
        __cl__->size = sizeof(__closure_0__);
        __cl__->x = ____sn__x_h__;
        rt_handle_end_transaction(__cl_h__);
        __cl_h__;
    });
    ((void (*)(void *, RtArenaV2 *))((__Closure__ *)__sn__inc->ptr)->fn)(__sn__inc, __local_arena__);
    rt_assert_v2(((*__sn__x) == 11LL), rt_arena_v2_strdup(__local_arena__, "expected x to be 11 after inc()"));
main_return:
    rt_arena_v2_condemn(__local_arena__);
    return _return_value;
}


/* Lambda function definitions */
static void __lambda_0__(void *__closure__, RtArenaV2 *__caller_arena__) {
    rt_safepoint_poll();
    RtArenaV2 *__lambda_arena__ = __caller_arena__ ? __caller_arena__ : ((__Closure__ *)((RtHandleV2 *)__closure__)->ptr)->arena;
    RtArenaV2 *__local_arena__ = __lambda_arena__;
    RtHandleV2 *____sn__x_h__ = ((__closure_0__ *)((RtHandleV2 *)__closure__)->ptr)->x;
    long long *__sn__x = (long long *)____sn__x_h__->ptr;

    (*__sn__x) = (*__sn__x) + 1LL;
__lambda_0___return:
    return;
}
