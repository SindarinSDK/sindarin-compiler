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
    RtHandleV2 *__x_pending__ = NULL;
    long long __sn__x = 0LL;
    RtHandleV2 *__y_pending__ = NULL;
    long long __sn__y = 0LL;
    rt_sync_lock(&__sn__x);
    {
        {
            (__sn__x = rt_add_long(__sn__x, 1LL));
            (__sn__y = __sn__x);
        }
    }
    rt_sync_unlock(&__sn__x);
    RtHandleV2 *__htmp_0__ = rt_arena_v2_strdup(__local_arena__, "expected y to be 1 after lock increment");
    RtHandleV2 *__htmp_1__ = rt_arena_v2_strdup(__local_arena__, "expected y to be 1 after lock increment");
    rt_assert_v2(rt_eq_long(__sn__y, 1LL), __htmp_1__);
    goto main_return;
main_return:
    rt_arena_v2_condemn(__local_arena__);
    return _return_value;
}

