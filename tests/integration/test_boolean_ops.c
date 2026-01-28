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
    // Test boolean operators && and ||
    // Test && with comparisons
    long long __sn__a = 5LL;
    long long __sn__b = 10LL;
    if (((rt_gt_long(__sn__a, 0LL) != 0 && rt_gt_long(__sn__b, 0LL) != 0) ? 1L : 0L)) {
        {
            rt_print_string("Both positive\n");
        }
    }
    if (((rt_gt_long(__sn__a, 10LL) != 0 && rt_gt_long(__sn__b, 0LL) != 0) ? 1L : 0L)) {
        {
            rt_print_string("Should not print\n");
        }
    }
    else {
        {
            rt_print_string("First condition failed\n");
        }
    }
    if (((rt_gt_long(__sn__a, 10LL) != 0 || rt_gt_long(__sn__b, 5LL) != 0) ? 1L : 0L)) {
        {
            rt_print_string("At least one true\n");
        }
    }
    if (((rt_gt_long(__sn__a, 10LL) != 0 || rt_gt_long(__sn__b, 20LL) != 0) ? 1L : 0L)) {
        {
            rt_print_string("Should not print\n");
        }
    }
    else {
        {
            rt_print_string("Both conditions failed\n");
        }
    }
    if (((((rt_gt_long(__sn__a, 0LL) != 0 && rt_gt_long(__sn__b, 0LL) != 0) ? 1L : 0L) != 0 || rt_gt_long(__sn__a, 100LL) != 0) ? 1L : 0L)) {
        {
            rt_print_string("Complex condition passed\n");
        }
    }
    bool __sn__x = rt_gt_long(__sn__a, 3LL);
    bool __sn__y = rt_lt_long(__sn__b, 20LL);
    if (((__sn__x != 0 && __sn__y != 0) ? 1L : 0L)) {
        {
            rt_print_string("x and y both true\n");
        }
    }
    if (((__sn__x != 0 || __sn__y != 0) ? 1L : 0L)) {
        {
            rt_print_string("x or y true\n");
        }
    }
    if (((rt_not_bool(__sn__x) != 0 || __sn__y != 0) ? 1L : 0L)) {
        {
            rt_print_string("Not x or y\n");
        }
    }
    goto main_return;
main_return:
    rt_managed_arena_destroy(__local_arena__);
    return _return_value;
}

