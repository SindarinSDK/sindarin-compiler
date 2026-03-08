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

    RtHandleV2 * __sn__arr = ({
            RtHandleV2 *__al__ = rt_array_create_generic_v2(__local_arena__, 0, sizeof(long long), NULL);
            __al__ = rt_array_push_v2(__local_arena__, __al__, &(long long){ 1LL }, sizeof(long long));
            __al__ = rt_array_push_v2(__local_arena__, __al__, &(long long){ 2LL }, sizeof(long long));
            __al__ = rt_array_push_v2(__local_arena__, __al__, &(long long){ 3LL }, sizeof(long long));
            __al__;
        });
    long long __sn__sum = 0LL;
    {
        RtHandleV2 *__handle_0__ = __sn__arr;
        long __len_0__ = rt_array_length_v2(__handle_0__);
        long long * __arr_0__;
        for (long __idx_0__ = 0; __idx_0__ < __len_0__; __idx_0__++) {
            rt_handle_begin_transaction(__handle_0__);
            __arr_0__ = (long long *)rt_array_data_v2(__handle_0__);
            long long __sn__item = __arr_0__[__idx_0__];
            rt_handle_end_transaction(__handle_0__);
            {
                __sn__sum = __sn__sum + __sn__item;
            }
            rt_safepoint_poll();
        }
    }    rt_assert_v2((__sn__sum == 6LL), rt_arena_v2_strdup(__local_arena__, "expected sum of {1,2,3} to be 6"));
main_return:
    rt_arena_v2_condemn(__local_arena__);
    return _return_value;
}


/* Lambda function definitions */
