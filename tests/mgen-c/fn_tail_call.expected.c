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

long long __sn__factorial(RtArenaV2 *, long long, long long);
/* Lambda forward declarations */
long long __sn__factorial(RtArenaV2 *__caller_arena__, long long __sn__n, long long __sn__acc) {
    rt_safepoint_poll();
    RtArenaV2 *__local_arena__ = __caller_arena__;
    long long _return_value = 0;

    if ((__sn__n <= 1LL)) {
        _return_value = __sn__acc; goto __sn__factorial_return;
    }

    _return_value = __sn__factorial(__local_arena__, rt_sub_long(__sn__n, 1LL), rt_mul_long(__sn__acc, __sn__n)); goto __sn__factorial_return;
__sn__factorial_return:
    return _return_value;
}


int main() {
    rt_safepoint_init();
    rt_safepoint_thread_register();
    RtArenaV2 *__local_arena__ = rt_arena_v2_create(NULL, RT_ARENA_MODE_DEFAULT, "main");
    __main_arena__ = __local_arena__;
    int _return_value = 0;


    rt_assert_v2((__sn__factorial(__local_arena__, 5LL, 1LL) == 120LL), rt_arena_v2_strdup(__local_arena__, "expected 5! to be 120"));
    rt_assert_v2((__sn__factorial(__local_arena__, 1LL, 1LL) == 1LL), rt_arena_v2_strdup(__local_arena__, "expected 1! to be 1"));
    rt_assert_v2((__sn__factorial(__local_arena__, 0LL, 1LL) == 1LL), rt_arena_v2_strdup(__local_arena__, "expected 0! to be 1"));
main_return:
    rt_arena_v2_condemn(__local_arena__);
    return _return_value;
}


/* Lambda function definitions */
