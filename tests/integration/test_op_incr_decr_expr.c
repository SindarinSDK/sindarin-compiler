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

int main() {
    RtManagedArena *__local_arena__ = rt_managed_arena_create();
    __main_arena__ = __local_arena__;
    int _return_value = 0;
    RtThreadHandle *__x_pending__ = NULL;
    long long __sn__x = 5LL;
    rt_post_inc_long(&__sn__x);
    rt_post_inc_long(&__sn__x);
    rt_post_dec_long(&__sn__x);
    rt_print_string(({
        char *_p0 = rt_to_string_long(__local_arena__, __sn__x);
        rt_str_concat(__local_arena__, _p0, "\n");
    }));
    goto main_return;
main_return:
    rt_managed_arena_destroy(__local_arena__);
    return _return_value;
}

