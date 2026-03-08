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

/* Struct type definitions */
typedef struct {
    RtArenaV2 *__arena__;
    long long __sn__value;
} __sn__Counter;


/* Forward declarations */
static RtArenaV2 *__main_arena__ = NULL;


long long __sn__Counter_zero(RtArenaV2 *);


long long __sn__Counter_zero(RtArenaV2 *__caller_arena__) {
    rt_safepoint_poll();
    RtArenaV2 *__local_arena__ = __caller_arena__;
    long long _return_value = 0;

    _return_value = 0LL; goto __sn__zero_return;
__sn__zero_return:
    return _return_value;
}

int main() {
    rt_safepoint_init();
    rt_safepoint_thread_register();
    RtArenaV2 *__local_arena__ = rt_arena_v2_create(NULL, RT_ARENA_MODE_DEFAULT, "main");
    __main_arena__ = __local_arena__;
    int _return_value = 0;
    long long __sn__z = __sn__Counter_zero(__local_arena__);
    _return_value = __sn__z; goto main_return;
main_return:
    rt_arena_v2_condemn(__local_arena__);
    return _return_value;
}
