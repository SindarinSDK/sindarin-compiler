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
/* Lambda forward declarations */
long long __sn__add(RtArenaV2 *__caller_arena__, long long __sn__a, long long __sn__b) {
    rt_safepoint_poll();
    RtArenaV2 *__local_arena__ = __caller_arena__;
    long long _return_value = 0;

    _return_value = rt_add_long(__sn__a, __sn__b); goto __sn__add_return;
__sn__add_return:
    return _return_value;
}


int main() {
    rt_safepoint_init();
    rt_safepoint_thread_register();
    RtArenaV2 *__local_arena__ = rt_arena_v2_create(NULL, RT_ARENA_MODE_DEFAULT, "main");
    __main_arena__ = __local_arena__;
    int _return_value = 0;


    long long __sn__x = 42LL;
    long long __sn__y = __sn__add(__local_arena__, __sn__x, 10LL);
    rt_assert_v2((__sn__y == 52LL), rt_arena_v2_strdup(__local_arena__, "expected add(42, 10) to be 52"));
main_return:
    rt_arena_v2_condemn(__local_arena__);
    return _return_value;
}


/* Lambda function definitions */
