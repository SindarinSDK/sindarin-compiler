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
typedef struct __Closure__ { void *fn; RtArena *arena; size_t size; } __Closure__;

/* Forward declarations */
static RtManagedArena *__main_arena__ = NULL;

/* Lambda forward declarations */
static long long __lambda_0__(void *__closure__, long long __sn__x);

int main() {
    RtManagedArena *__local_arena__ = rt_managed_arena_create();
    __main_arena__ = __local_arena__;
    int _return_value = 0;
    __Closure__ * __sn__op = ({
    __Closure__ *__cl__ = rt_arena_alloc(__local_arena__, sizeof(__Closure__));
    __cl__->fn = (void *)__lambda_0__;
    __cl__->arena = __local_arena__;
    __cl__->size = sizeof(__Closure__);
    __cl__;
});
    RtThreadHandle *__result_pending__ = NULL;
    long long __sn__result = ((long long (*)(void *, long long))__sn__op->fn)(__sn__op, 5LL);
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__local_arena__, __sn__result);
        rt_str_concat(__local_arena__, _p0, "\n");
    });
        rt_print_string(_str_arg0);
    });
    goto main_return;
main_return:
    rt_managed_arena_destroy(__local_arena__);
    return _return_value;
}


/* Lambda function definitions */
static long long __lambda_0__(void *__closure__, long long __sn__x) {
    RtManagedArena *__lambda_arena__ = (RtManagedArena *)rt_get_thread_arena_or(((__Closure__ *)__closure__)->arena);
    return rt_mul_long(__sn__x, 2LL);
}

