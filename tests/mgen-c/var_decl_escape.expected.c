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

RtHandleV2 * __sn__make_str(RtArenaV2 *);

/* Lambda forward declarations */
RtHandleV2 * __sn__make_str(RtArenaV2 *__caller_arena__) {
    rt_safepoint_poll();
    RtArenaV2 *__local_arena__ = __caller_arena__;
    RtHandleV2 * _return_value = NULL;

    RtHandleV2 * __sn__s = rt_arena_v2_strdup(__local_arena__, "hello");

    _return_value = __sn__s; goto __sn__make_str_return;
__sn__make_str_return:
    return _return_value;
}


int main() {
    rt_safepoint_init();
    rt_safepoint_thread_register();
    RtArenaV2 *__local_arena__ = rt_arena_v2_create(NULL, RT_ARENA_MODE_DEFAULT, "main");
    __main_arena__ = __local_arena__;
    int _return_value = 0;


    RtHandleV2 * __sn__result = __sn__make_str(__local_arena__);
    _return_value = 0LL; goto main_return;
main_return:
    rt_arena_v2_condemn(__local_arena__);
    return _return_value;
}


/* Lambda function definitions */
