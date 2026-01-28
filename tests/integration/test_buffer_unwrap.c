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

/* Native function extern declarations */
extern unsigned char* get_buffer_ptr(void);

/* Forward declarations */
static RtManagedArena *__main_arena__ = NULL;

unsigned char* mock_get_buffer(RtManagedArena *);
long long get_buffer_length(RtManagedArena *);
void __sn__test_pointer_slice_codegen(RtManagedArena *);
void __sn__test_slice_patterns(RtManagedArena *);

/* Interceptor thunk forward declarations */
static RtAny __thunk_0(void);
static RtAny __thunk_1(void);

unsigned char* mock_get_buffer(RtManagedArena *__caller_arena__) {
    RtManagedArena *__local_arena__ = rt_managed_arena_create_child(__caller_arena__);
    unsigned char* _return_value = 0;
    _return_value = NULL;
    goto mock_get_buffer_return;
mock_get_buffer_return:
    rt_managed_arena_destroy_child(__local_arena__);
    return _return_value;
}

long long get_buffer_length(RtManagedArena *__caller_arena__) {
    RtManagedArena *__local_arena__ = rt_managed_arena_create_child(__caller_arena__);
    long long _return_value = 0;
    _return_value = 5LL;
    goto get_buffer_length_return;
get_buffer_length_return:
    rt_managed_arena_destroy_child(__local_arena__);
    return _return_value;
}

void __sn__test_pointer_slice_codegen(RtManagedArena *__caller_arena__) {
    RtManagedArena *__local_arena__ = rt_managed_arena_create_child(__caller_arena__);
    long long __sn__len = get_buffer_length(__local_arena__);
    RtHandle __sn__data = rt_array_create_byte_h(__local_arena__, (size_t)((__sn__len) - (0LL)), (mock_get_buffer(__local_arena__)) + (0LL));
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__local_arena__, __sn__len);
        char *_r = rt_str_concat(__local_arena__, "Array created successfully with length parameter: ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    long long __sn__arr_len = rt_array_length(((unsigned char *)rt_managed_pin_array(__local_arena__, __sn__data)));
    ({
        char *_str_arg0 = ({
        char *_p0 = rt_to_string_long(__local_arena__, __sn__arr_len);
        char *_r = rt_str_concat(__local_arena__, "Array length property: ", _p0);
        _r = rt_str_concat(__local_arena__, _r, "\n");
        _r;
    });
        rt_print_string(_str_arg0);
    });
    goto __sn__test_pointer_slice_codegen_return;
__sn__test_pointer_slice_codegen_return:
    rt_managed_arena_destroy_child(__local_arena__);
    return;
}

void __sn__test_slice_patterns(RtManagedArena *__caller_arena__) {
    RtManagedArena *__local_arena__ = rt_managed_arena_create_child(__caller_arena__);
    long long __sn__len = 10LL;
    RtHandle __sn__data1 = rt_array_create_byte_h(__local_arena__, (size_t)((__sn__len) - (0LL)), (mock_get_buffer(__local_arena__)) + (0LL));
    rt_print_string("Pattern 1 (ptr[0..len] as val): OK\n");
    RtHandle __sn__data3 = rt_array_create_byte_h(__local_arena__, (size_t)((get_buffer_length(__local_arena__)) - (0LL)), (mock_get_buffer(__local_arena__)) + (0LL));
    rt_print_string("Pattern 3 (fn()[0..fn2()] as val): OK\n");
    goto __sn__test_slice_patterns_return;
__sn__test_slice_patterns_return:
    rt_managed_arena_destroy_child(__local_arena__);
    return;
}

int main() {
    RtManagedArena *__local_arena__ = rt_managed_arena_create();
    __main_arena__ = __local_arena__;
    int _return_value = 0;
    rt_print_string("Testing buffer unwrap code generation...\n");
    ({
    if (__rt_interceptor_count > 0) {
        RtAny __args[1];
        __rt_thunk_args = __args;
        __rt_thunk_arena = __local_arena__;
        RtAny __intercepted = rt_call_intercepted("test_pointer_slice_codegen", __args, 0, __thunk_0);
    } else {
        __sn__test_pointer_slice_codegen(__local_arena__);
    }
    (void)0;
});
    ({
    if (__rt_interceptor_count > 0) {
        RtAny __args[1];
        __rt_thunk_args = __args;
        __rt_thunk_arena = __local_arena__;
        RtAny __intercepted = rt_call_intercepted("test_slice_patterns", __args, 0, __thunk_1);
    } else {
        __sn__test_slice_patterns(__local_arena__);
    }
    (void)0;
});
    rt_print_string("Buffer unwrap code generation test complete!\n");
    goto main_return;
main_return:
    rt_managed_arena_destroy(__local_arena__);
    return _return_value;
}


/* Interceptor thunk definitions */
static RtAny __thunk_0(void) {
    __sn__test_pointer_slice_codegen((RtArena *)__rt_thunk_arena);
    return rt_box_nil();
}

static RtAny __thunk_1(void) {
    __sn__test_slice_patterns((RtArena *)__rt_thunk_arena);
    return rt_box_nil();
}

