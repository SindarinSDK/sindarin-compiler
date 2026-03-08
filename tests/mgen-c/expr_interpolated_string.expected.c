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


int main() {
    rt_safepoint_init();
    rt_safepoint_thread_register();
    RtArenaV2 *__local_arena__ = rt_arena_v2_create(NULL, RT_ARENA_MODE_DEFAULT, "main");
    __main_arena__ = __local_arena__;
    int _return_value = 0;
    RtHandleV2 *__name_pending__ = NULL;
        RtHandleV2 * __sn__name = rt_arena_v2_strdup(__local_arena__, "world");
    RtHandleV2 *__msg_pending__ = NULL;
        RtHandleV2 * __sn__msg = ({
            RtHandleV2 *__is_0__ = rt_arena_v2_strdup(__local_arena__, "Hello ");
            RtHandleV2 *__is_1__ = __sn__name;
            RtHandleV2 *__is_2__ = rt_arena_v2_strdup(__local_arena__, "!");
            rt_string_concat_multi_v2(__local_arena__, , __is_0__, __is_1__, __is_2__);
        });
    _return_value = 0LL; goto main_return;
main_return:
    rt_arena_v2_condemn(__local_arena__);
    return _return_value;
}
