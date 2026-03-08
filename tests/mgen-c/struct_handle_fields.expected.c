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

/* Struct type definitions */
typedef struct {
    RtArenaV2 *__arena__;
    RtHandleV2 * __sn__name;
    long long __sn__age;
} __sn__Person;


/* Forward declarations */
static RtArenaV2 *__main_arena__ = NULL;


int main() {
    rt_safepoint_init();
    rt_safepoint_thread_register();
    RtArenaV2 *__local_arena__ = rt_arena_v2_create(NULL, RT_ARENA_MODE_DEFAULT, "main");
    __main_arena__ = __local_arena__;
    int _return_value = 0;
    __sn__Person __sn__p = (__sn__Person){ .__arena__ = rt_arena_v2_create(__local_arena__, RT_ARENA_MODE_DEFAULT, "struct"), .__sn__name = rt_arena_v2_strdup(__local_arena__, "Alice"), .__sn__age = 30LL };
    _return_value = 0LL; goto main_return;
main_return:
    rt_arena_v2_condemn(__local_arena__);
    return _return_value;
}
