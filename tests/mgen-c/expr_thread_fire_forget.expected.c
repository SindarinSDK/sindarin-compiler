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

void __sn__compute(RtArenaV2 *);
typedef struct {
    /* Function-specific arguments */
} __ThreadArgs_0__;

static void *__thread_wrapper_0__(void *arg) {
    RtHandleV2 *__th__ = (RtHandleV2 *)arg;
    rt_tls_thread_set_v3(__th__);
    RtArenaV2 *__arena__ = rt_thread_v3_get_arena(__th__);
    rt_safepoint_adopt_registration();

    /* Unpack args from thread handle */
    RtHandleV2 *__args_h__ = rt_thread_v3_get_args(__th__);
    rt_handle_begin_transaction(__args_h__);
    __ThreadArgs_0__ __args_copy__ = *(__ThreadArgs_0__ *)__args_h__->ptr;
    rt_handle_end_transaction(__args_h__);
    __ThreadArgs_0__ *args = &__args_copy__;

    /* Call the function */
    __sn__compute(__arena__);

    rt_thread_v3_dispose(__th__);
    rt_safepoint_poll();
    rt_safepoint_thread_deregister();
    rt_tls_thread_set_v3(NULL);
    return NULL;
}

/* Lambda forward declarations */
void __sn__compute(RtArenaV2 *__caller_arena__) {
    rt_safepoint_poll();
    RtArenaV2 *__local_arena__ = __caller_arena__;

    goto __sn__compute_return;
__sn__compute_return:
    return;
}


int main() {
    rt_safepoint_init();
    rt_safepoint_thread_register();
    RtArenaV2 *__local_arena__ = rt_arena_v2_create(NULL, RT_ARENA_MODE_DEFAULT, "main");
    __main_arena__ = __local_arena__;
    int _return_value = 0;


    ({
        /* V3: Create thread with its own arena */
        RtHandleV2 *__spawn_handle_0__ = rt_thread_v3_create(__local_arena__, RT_THREAD_MODE_DEFAULT);
        RtArenaV2 *__thread_arena__ = rt_thread_v3_get_arena(__spawn_handle_0__);
    
        /* Allocate args in thread arena (min 1 byte for empty structs) */
        RtHandleV2 *__args_h__ = rt_arena_v2_alloc(__thread_arena__, sizeof(__ThreadArgs_0__) > 0 ? sizeof(__ThreadArgs_0__) : 1);
        rt_handle_begin_transaction(__args_h__);
        __ThreadArgs_0__ *__spawn_args_0__ = (__ThreadArgs_0__ *)__args_h__->ptr;
            rt_handle_end_transaction(__args_h__);
        rt_thread_v3_set_args(__spawn_handle_0__, __args_h__);
    
        /* Start the thread */
        rt_thread_v3_start(__spawn_handle_0__, __thread_wrapper_0__);
        __spawn_handle_0__;
    });
    _return_value = 0LL; goto main_return;
main_return:
    rt_arena_v2_condemn(__local_arena__);
    return _return_value;
}


/* Lambda function definitions */
