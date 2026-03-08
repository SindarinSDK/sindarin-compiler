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
    long long __sn__value;
} __sn__Counter;


/* Forward declarations */
static RtArenaV2 *__main_arena__ = NULL;

void __sn__Counter_increment(RtArenaV2 *, __sn__Counter *);
long long __sn__Counter_getValue(RtArenaV2 *, __sn__Counter *);
/* Lambda forward declarations */

void __sn__Counter_increment(RtArenaV2 *__caller_arena__, __sn__Counter *self) {
    rt_safepoint_poll();
    RtArenaV2 *__local_arena__ = __caller_arena__;

    self->__sn__value = self->__sn__value + 1LL;
__sn__increment_return:
    return;
}

long long __sn__Counter_getValue(RtArenaV2 *__caller_arena__, __sn__Counter *self) {
    rt_safepoint_poll();
    RtArenaV2 *__local_arena__ = __caller_arena__;
    long long _return_value = 0;

    _return_value = self->__sn__value; goto __sn__getValue_return;
__sn__getValue_return:
    return _return_value;
}

int main() {
    rt_safepoint_init();
    rt_safepoint_thread_register();
    RtArenaV2 *__local_arena__ = rt_arena_v2_create(NULL, RT_ARENA_MODE_DEFAULT, "main");
    __main_arena__ = __local_arena__;
    int _return_value = 0;

    __sn__Counter __sn__c = (__sn__Counter){ .__arena__ = rt_arena_v2_create(__local_arena__, RT_ARENA_MODE_DEFAULT, "struct"), .__sn__value = 0LL };
    __sn__Counter_increment(__local_arena__, &__sn__c);
    rt_assert_v2((__sn__Counter_getValue(__local_arena__, &__sn__c) == 1LL), rt_arena_v2_strdup(__local_arena__, "expected counter to be 1 after increment"));
main_return:
    rt_arena_v2_condemn(__local_arena__);
    return _return_value;
}


/* Lambda function definitions */
