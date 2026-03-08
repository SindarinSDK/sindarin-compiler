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

long long __sn__apply(RtArenaV2 *, void, long long);

long long __sn__apply(RtArenaV2 *__caller_arena__, void __sn__f, long long __sn__x) {
    rt_safepoint_poll();
    RtArenaV2 *__local_arena__ = __caller_arena__;
    long long _return_value = 0;

    _return_value = __sn__f(__local_arena__, __sn__x); goto __sn__apply_return;
__sn__apply_return:
    return _return_value;
}

int main() {
    rt_safepoint_init();
    rt_safepoint_thread_register();
    RtArenaV2 *__local_arena__ = rt_arena_v2_create(NULL, RT_ARENA_MODE_DEFAULT, "main");
    __main_arena__ = __local_arena__;
    int _return_value = 0;
    void __sn__double_it = /* lambda 0 */
        NULL /* TODO: lambda codegen */;
    _return_value = __sn__apply(__local_arena__, __sn__double_it, 5LL); goto main_return;
main_return:
    rt_arena_v2_condemn(__local_arena__);
    return _return_value;
}
