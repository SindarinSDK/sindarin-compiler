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

RtHandle __sn__build_chained(RtManagedArena *);

/* Interceptor thunk forward declarations */
static RtAny __thunk_0(void);

RtHandle __sn__build_chained(RtManagedArena *__caller_arena__) {
    RtManagedArena *__local_arena__ = __caller_arena__;
    RtHandle _return_value = RT_HANDLE_NULL;
    RtHandle __sn__result = rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "");
    {
        long long __sn__a = 1LL;
        while (rt_le_long(__sn__a, 1LL)) {
            {
                RtHandle __sn__s1 = rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "");
                {
                    long long __sn__b = 1LL;
                    while (rt_le_long(__sn__b, 1LL)) {
                        {
                            RtHandle __sn__s2 = rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "");
                            {
                                long long __sn__c = 1LL;
                                while (rt_le_long(__sn__c, 1LL)) {
                                    {
                                        RtHandle __sn__s3 = rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "");
                                        {
                                            long long __sn__d = 1LL;
                                            while (rt_le_long(__sn__d, 1LL)) {
                                                {
                                                    RtHandle __sn__s4 = rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "");
                                                    {
                                                        long long __sn__e = 1LL;
                                                        while (rt_le_long(__sn__e, 1LL)) {
                                                            {
                                                                RtHandle __sn__s5 = rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "");
                                                                {
                                                                    long long __sn__f = 1LL;
                                                                    while (rt_le_long(__sn__f, 1LL)) {
                                                                        {
                                                                            RtHandle __sn__s6 = rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "");
                                                                            {
                                                                                long long __sn__g = 1LL;
                                                                                while (rt_le_long(__sn__g, 1LL)) {
                                                                                    {
                                                                                        RtHandle __sn__s7 = rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "");
                                                                                        {
                                                                                            long long __sn__h = 1LL;
                                                                                            while (rt_le_long(__sn__h, 1LL)) {
                                                                                                {
                                                                                                    RtHandle __sn__s8 = rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "");
                                                                                                    {
                                                                                                        long long __sn__i = 1LL;
                                                                                                        while (rt_le_long(__sn__i, 1LL)) {
                                                                                                            {
                                                                                                                RtHandle __sn__s9 = rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "");
                                                                                                                {
                                                                                                                    long long __sn__j = 1LL;
                                                                                                                    while (rt_le_long(__sn__j, 1LL)) {
                                                                                                                        {
                                                                                                                            RtHandle __sn__inner = rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "d11");
                                                                                                                            (__sn__s9 = rt_str_concat_h(__local_arena__, RT_HANDLE_NULL, (char *)rt_managed_pin(__local_arena__, rt_str_concat_h(__local_arena__, RT_HANDLE_NULL, (char *)rt_managed_pin(__local_arena__, __sn__s9), (char *)rt_managed_pin(__local_arena__, __sn__inner))), "->d10 "));
                                                                                                                        }
                                                                                                                    __for_continue_9__:;
                                                                                                                        rt_post_inc_long(&__sn__j);
                                                                                                                    }
                                                                                                                }
                                                                                                                (__sn__s8 = rt_str_concat_h(__local_arena__, RT_HANDLE_NULL, (char *)rt_managed_pin(__local_arena__, rt_str_concat_h(__local_arena__, RT_HANDLE_NULL, (char *)rt_managed_pin(__local_arena__, __sn__s8), (char *)rt_managed_pin(__local_arena__, __sn__s9))), "->d9 "));
                                                                                                            }
                                                                                                        __for_continue_8__:;
                                                                                                            rt_post_inc_long(&__sn__i);
                                                                                                        }
                                                                                                    }
                                                                                                    (__sn__s7 = rt_str_concat_h(__local_arena__, RT_HANDLE_NULL, (char *)rt_managed_pin(__local_arena__, rt_str_concat_h(__local_arena__, RT_HANDLE_NULL, (char *)rt_managed_pin(__local_arena__, __sn__s7), (char *)rt_managed_pin(__local_arena__, __sn__s8))), "->d8 "));
                                                                                                }
                                                                                            __for_continue_7__:;
                                                                                                rt_post_inc_long(&__sn__h);
                                                                                            }
                                                                                        }
                                                                                        (__sn__s6 = rt_str_concat_h(__local_arena__, RT_HANDLE_NULL, (char *)rt_managed_pin(__local_arena__, rt_str_concat_h(__local_arena__, RT_HANDLE_NULL, (char *)rt_managed_pin(__local_arena__, __sn__s6), (char *)rt_managed_pin(__local_arena__, __sn__s7))), "->d7 "));
                                                                                    }
                                                                                __for_continue_6__:;
                                                                                    rt_post_inc_long(&__sn__g);
                                                                                }
                                                                            }
                                                                            (__sn__s5 = rt_str_concat_h(__local_arena__, RT_HANDLE_NULL, (char *)rt_managed_pin(__local_arena__, rt_str_concat_h(__local_arena__, RT_HANDLE_NULL, (char *)rt_managed_pin(__local_arena__, __sn__s5), (char *)rt_managed_pin(__local_arena__, __sn__s6))), "->d6 "));
                                                                        }
                                                                    __for_continue_5__:;
                                                                        rt_post_inc_long(&__sn__f);
                                                                    }
                                                                }
                                                                (__sn__s4 = rt_str_concat_h(__local_arena__, RT_HANDLE_NULL, (char *)rt_managed_pin(__local_arena__, rt_str_concat_h(__local_arena__, RT_HANDLE_NULL, (char *)rt_managed_pin(__local_arena__, __sn__s4), (char *)rt_managed_pin(__local_arena__, __sn__s5))), "->d5 "));
                                                            }
                                                        __for_continue_4__:;
                                                            rt_post_inc_long(&__sn__e);
                                                        }
                                                    }
                                                    (__sn__s3 = rt_str_concat_h(__local_arena__, RT_HANDLE_NULL, (char *)rt_managed_pin(__local_arena__, rt_str_concat_h(__local_arena__, RT_HANDLE_NULL, (char *)rt_managed_pin(__local_arena__, __sn__s3), (char *)rt_managed_pin(__local_arena__, __sn__s4))), "->d4 "));
                                                }
                                            __for_continue_3__:;
                                                rt_post_inc_long(&__sn__d);
                                            }
                                        }
                                        (__sn__s2 = rt_str_concat_h(__local_arena__, RT_HANDLE_NULL, (char *)rt_managed_pin(__local_arena__, rt_str_concat_h(__local_arena__, RT_HANDLE_NULL, (char *)rt_managed_pin(__local_arena__, __sn__s2), (char *)rt_managed_pin(__local_arena__, __sn__s3))), "->d3 "));
                                    }
                                __for_continue_2__:;
                                    rt_post_inc_long(&__sn__c);
                                }
                            }
                            (__sn__s1 = rt_str_concat_h(__local_arena__, RT_HANDLE_NULL, (char *)rt_managed_pin(__local_arena__, rt_str_concat_h(__local_arena__, RT_HANDLE_NULL, (char *)rt_managed_pin(__local_arena__, __sn__s1), (char *)rt_managed_pin(__local_arena__, __sn__s2))), "->d2 "));
                        }
                    __for_continue_1__:;
                        rt_post_inc_long(&__sn__b);
                    }
                }
                (__sn__result = rt_str_concat_h(__local_arena__, RT_HANDLE_NULL, (char *)rt_managed_pin(__local_arena__, rt_str_concat_h(__local_arena__, RT_HANDLE_NULL, (char *)rt_managed_pin(__local_arena__, __sn__result), (char *)rt_managed_pin(__local_arena__, __sn__s1))), "->d1"));
            }
        __for_continue_0__:;
            rt_post_inc_long(&__sn__a);
        }
    }
    _return_value = __sn__result;
    goto __sn__build_chained_return;
__sn__build_chained_return:
    return _return_value;
}

int main() {
    RtManagedArena *__local_arena__ = rt_managed_arena_create();
    __main_arena__ = __local_arena__;
    int _return_value = 0;
    rt_println("Shared chained test (10 loops):");
    rt_println("All values allocated directly in caller arena");
    rt_println((char *)rt_managed_pin(__local_arena__, rt_str_concat_h(__local_arena__, RT_HANDLE_NULL, "Path: ", (char *)rt_managed_pin(__local_arena__, ({
    RtHandle __intercept_result;
    if (__rt_interceptor_count > 0) {
        RtAny __args[1];
        __rt_thunk_args = __args;
        __rt_thunk_arena = __local_arena__;
        RtAny __intercepted = rt_call_intercepted("build_chained", __args, 0, __thunk_0);
        __intercept_result = rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, rt_unbox_string(__intercepted));
    } else {
        __intercept_result = __sn__build_chained(__local_arena__);
    }
    __intercept_result;
})))));
    rt_println("PASS: All shared allocations succeeded");
    _return_value = 0LL;
    goto main_return;
main_return:
    rt_managed_arena_destroy(__local_arena__);
    return _return_value;
}


/* Interceptor thunk definitions */
static RtAny __thunk_0(void) {
    RtAny __result = rt_box_string((char *)rt_managed_pin((RtArena *)__rt_thunk_arena, __sn__build_chained((RtArena *)__rt_thunk_arena)));
    return __result;
}

