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
    RtHandle __sn__b = rt_array_create_byte_h(__local_arena__, 5, (unsigned char[]){72LL, 101LL, 108LL, 108LL, 111LL});
    RtHandle __sn__s = rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, rt_byte_array_to_string(__local_arena__, ((unsigned char *)rt_managed_pin_array(__local_arena__, __sn__b))));
    ({
        char *_str_arg0 = rt_str_concat(__local_arena__, (char *)rt_managed_pin(__local_arena__, __sn__s), "\n");
        rt_print_string(_str_arg0);
    });
    goto main_return;
main_return:
    rt_managed_arena_destroy(__local_arena__);
    return _return_value;
}

