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
    RtThreadHandle *__arr_pending__ = NULL;
    RtHandle __sn__arr = rt_array_create_long_h(__local_arena__, 3, (long long[]){10LL, 20LL, 30LL});
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__local_arena__, rt_array_indexOf_long(((long long *)rt_managed_pin_array(__local_arena__, __sn__arr)), 10LL));
        rt_str_concat(__local_arena__, _p0, "\n");
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__local_arena__, rt_array_indexOf_long(((long long *)rt_managed_pin_array(__local_arena__, __sn__arr)), 30LL));
        rt_str_concat(__local_arena__, _p0, "\n");
    });
        rt_print_string(_str_arg0);
    });
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__local_arena__, rt_array_indexOf_long(((long long *)rt_managed_pin_array(__local_arena__, __sn__arr)), 99LL));
        rt_str_concat(__local_arena__, _p0, "\n");
    });
        rt_print_string(_str_arg0);
    });
    goto main_return;
main_return:
    rt_managed_arena_destroy(__local_arena__);
    return _return_value;
}

