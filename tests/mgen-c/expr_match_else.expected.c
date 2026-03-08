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


int main() {
    rt_safepoint_init();
    rt_safepoint_thread_register();
    RtArenaV2 *__local_arena__ = rt_arena_v2_create(NULL, RT_ARENA_MODE_DEFAULT, "main");
    __main_arena__ = __local_arena__;
    int _return_value = 0;
    long long __sn__x = 99LL;
    long long __sn__result = ({
            long long __match_result__;
            long long __match_subject__ = __sn__x;
            if (__match_subject__ == 1LL) {
                __match_result__ = 10LL;
            }
            __match_result__ = 0LL;
            __match_result__;
        });
    _return_value = __sn__result; goto main_return;
main_return:
    rt_arena_v2_condemn(__local_arena__);
    return _return_value;
}
