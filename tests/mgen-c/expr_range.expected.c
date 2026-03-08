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

/* Lambda forward declarations */

int main() {
    rt_safepoint_init();
    rt_safepoint_thread_register();
    RtArenaV2 *__local_arena__ = rt_arena_v2_create(NULL, RT_ARENA_MODE_DEFAULT, "main");
    __main_arena__ = __local_arena__;
    int _return_value = 0;

    long long __sn__sum = 0LL;
    {
        RtHandleV2 *__handle_0__ = rt_array_range_v2(__local_arena__, 1LL, 11LL);
        long __len_0__ = rt_array_length_v2(__handle_0__);
        long long * __arr_0__;
        for (long __idx_0__ = 0; __idx_0__ < __len_0__; __idx_0__++) {
            rt_handle_begin_transaction(__handle_0__);
            __arr_0__ = (long long *)rt_array_data_v2(__handle_0__);
            long long __sn__x = __arr_0__[__idx_0__];
            rt_handle_end_transaction(__handle_0__);
            {
                __sn__sum = __sn__sum + __sn__x;
            }
            rt_safepoint_poll();
        }
    }    rt_assert_v2((__sn__sum == 55LL), rt_arena_v2_strdup(__local_arena__, "expected sum of 1..10 to be 55"));
main_return:
    rt_arena_v2_condemn(__local_arena__);
    return _return_value;
}


/* Lambda function definitions */
