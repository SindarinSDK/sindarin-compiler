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
    RtHandle __sn__s = rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "1,2,3,4,5");
    RtHandle __sn__parts = rt_str_split_h(__local_arena__, (char *)rt_managed_pin(__local_arena__, __sn__s), ",");
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__local_arena__, rt_array_length(((RtHandle *)rt_managed_pin_array(__local_arena__, __sn__parts))));
        rt_str_concat(__local_arena__, _p0, "\n");
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_r = rt_str_concat(__local_arena__, ((char *)rt_managed_pin(__local_arena__, ((RtHandle *)rt_managed_pin_array(__local_arena__, __sn__parts))[0LL])), " ");
        _r = rt_str_concat(__local_arena__, _r, ((char *)rt_managed_pin(__local_arena__, ((RtHandle *)rt_managed_pin_array(__local_arena__, __sn__parts))[4LL])));
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    goto main_return;
main_return:
    rt_managed_arena_destroy(__local_arena__);
    return _return_value;
}

