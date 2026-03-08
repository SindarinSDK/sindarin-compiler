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
static long long __lambda_0__(void *__closure__, RtArenaV2 *__caller_arena__, long long __sn__a, long long __sn__b);

int main() {
    rt_safepoint_init();
    rt_safepoint_thread_register();
    RtArenaV2 *__local_arena__ = rt_arena_v2_create(NULL, RT_ARENA_MODE_DEFAULT, "main");
    __main_arena__ = __local_arena__;
    int _return_value = 0;

    RtHandleV2 * __sn__add = ({
        RtHandleV2 *__cl_h__ = rt_arena_v2_alloc(__local_arena__, sizeof(__Closure__));
        __Closure__ *__cl__ = (__Closure__ *)__cl_h__->ptr;
        rt_handle_begin_transaction(__cl_h__);
        __cl__->fn = (void *)__lambda_0__;
        __cl__->arena = __local_arena__;
        __cl__->size = sizeof(__Closure__);
        rt_handle_end_transaction(__cl_h__);
        __cl_h__;
    });
    rt_assert_v2((((long long (*)(void *, RtArenaV2 *, long long, long long))((__Closure__ *)__sn__add->ptr)->fn)(__sn__add, __local_arena__, 1LL, 2LL) == 3LL), rt_arena_v2_strdup(__local_arena__, "expected 1 + 2 to be 3"));
main_return:
    rt_arena_v2_condemn(__local_arena__);
    return _return_value;
}


/* Lambda function definitions */
static long long __lambda_0__(void *__closure__, RtArenaV2 *__caller_arena__, long long __sn__a, long long __sn__b) {
    rt_safepoint_poll();
    RtArenaV2 *__lambda_arena__ = __caller_arena__ ? __caller_arena__ : ((__Closure__ *)((RtHandleV2 *)__closure__)->ptr)->arena;
    RtArenaV2 *__local_arena__ = __lambda_arena__;
    return rt_add_long(__sn__a, __sn__b);
}
