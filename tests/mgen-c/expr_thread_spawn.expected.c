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

long long __sn__compute(RtArenaV2 *);
long long __sn__compute(RtArenaV2 *__caller_arena__) {
    rt_safepoint_poll();
    RtArenaV2 *__local_arena__ = __caller_arena__;
    long long _return_value = 0;

    _return_value = 42LL; goto __sn__compute_return;
__sn__compute_return:
    return _return_value;
}


int main() {
    rt_safepoint_init();
    rt_safepoint_thread_register();
    RtArenaV2 *__local_arena__ = rt_arena_v2_create(NULL, RT_ARENA_MODE_DEFAULT, "main");
    __main_arena__ = __local_arena__;
    int _return_value = 0;
    long long __sn__handle = rt_thread_spawn(__local_arena__, __sn__compute(__local_arena__));
    long long __sn__result = rt_thread_sync(__sn__handle);
    _return_value = __sn__result; goto main_return;
main_return:
    rt_arena_v2_condemn(__local_arena__);
    return _return_value;
}
