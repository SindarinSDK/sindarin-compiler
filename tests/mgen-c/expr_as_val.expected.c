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

void __sn__modify(RtArenaV2 *, RtHandleV2 *);
/* Lambda forward declarations */
void __sn__modify(RtArenaV2 *__caller_arena__, RtHandleV2 * __sn__arr) {
    rt_safepoint_poll();
    RtArenaV2 *__local_arena__ = __caller_arena__;

    goto __sn__modify_return;
__sn__modify_return:
    return;
}


int main() {
    rt_safepoint_init();
    rt_safepoint_thread_register();
    RtArenaV2 *__local_arena__ = rt_arena_v2_create(NULL, RT_ARENA_MODE_DEFAULT, "main");
    __main_arena__ = __local_arena__;
    int _return_value = 0;


    RtHandleV2 * __sn__a = ({
            RtHandleV2 *__al__ = rt_array_create_generic_v2(__local_arena__, 0, sizeof(long long), NULL);
            __al__ = rt_array_push_v2(__local_arena__, __al__, &(long long){ 1LL }, sizeof(long long));
            __al__ = rt_array_push_v2(__local_arena__, __al__, &(long long){ 2LL }, sizeof(long long));
            __al__ = rt_array_push_v2(__local_arena__, __al__, &(long long){ 3LL }, sizeof(long long));
            __al__;
        });
    __sn__modify(__local_arena__, __sn__a);
    _return_value = 0LL; goto main_return;
main_return:
    rt_arena_v2_condemn(__local_arena__);
    return _return_value;
}


/* Lambda function definitions */
