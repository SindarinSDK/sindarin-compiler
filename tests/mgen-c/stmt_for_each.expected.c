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
    RtHandleV2 *__arr_pending__ = NULL;
        RtHandleV2 * __sn__arr = ({
            RtHandleV2 *__al__ = rt_array_create_v2(__local_arena__, , sizeof(long long));
            rt_array_push_int_v2(__al__, 1LL);
            rt_array_push_int_v2(__al__, 2LL);
            rt_array_push_int_v2(__al__, 3LL);
            __al__;
        });
    long long __sn__sum = 0LL;
    {
        RtHandleV2 *__fe_arr__ = __sn__arr;
        long long __fe_len__ = rt_array_length_v2(__fe_arr__);
        for (long long __fe_i__ = 0; __fe_i__ < __fe_len__; __fe_i__++) {
            long long __sn__item = rt_array_get_int(__fe_arr__, __fe_i__);
            __sn__sum = __sn__sum + __sn__item;
            rt_safepoint_poll();
        }
    }
    _return_value = __sn__sum; goto main_return;
main_return:
    rt_arena_v2_condemn(__local_arena__);
    return _return_value;
}
