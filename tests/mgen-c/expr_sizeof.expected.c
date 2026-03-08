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
    double __sn__x;
    double __sn__y;
} __sn__Point;


/* Forward declarations */
static RtArenaV2 *__main_arena__ = NULL;




int main() {
    rt_safepoint_init();
    rt_safepoint_thread_register();
    RtArenaV2 *__local_arena__ = rt_arena_v2_create(NULL, RT_ARENA_MODE_DEFAULT, "main");
    __main_arena__ = __local_arena__;
    int _return_value = 0;
    long long __sn__s = sizeof(__sn__{ "kind": "struct", "name": "Point", "is_native": false, "is_packed": false, "pass_self_by_ref": false, "size": 0, "alignment": 0, "fields": [ { "name": "x", "type": { "kind": "double" }, "offset": 0 }, { "name": "y", "type": { "kind": "double" }, "offset": 0 } ] });
    _return_value = 0LL; goto main_return;
main_return:
    rt_arena_v2_condemn(__local_arena__);
    return _return_value;
}
