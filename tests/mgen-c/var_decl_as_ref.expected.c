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

void __sn__increment(RtArenaV2 *, long long *);

/* Lambda forward declarations */
void __sn__increment(RtArenaV2 *__caller_arena__, long long *__sn__x) {
    rt_safepoint_poll();
    RtArenaV2 *__local_arena__ = __caller_arena__;

    (*__sn__x = rt_add_long((*__sn__x), 1LL));
__sn__increment_return:
    return;
}


int main() {
    rt_safepoint_init();
    rt_safepoint_thread_register();
    RtArenaV2 *__local_arena__ = rt_arena_v2_create(NULL, RT_ARENA_MODE_DEFAULT, "main");
    __main_arena__ = __local_arena__;
    int _return_value = 0;


    long long __sn__a = 10LL;
    __sn__increment(__local_arena__, &__sn__a);
    rt_assert_v2((__sn__a == 11LL), rt_arena_v2_strdup(__local_arena__, "expected a to be 11 after increment by ref"));
main_return:
    rt_arena_v2_condemn(__local_arena__);
    return _return_value;
}


/* Lambda function definitions */
