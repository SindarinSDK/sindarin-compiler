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

/* Struct type definitions */
typedef struct {
    long long __sn__x;
    long long __sn__y;
} __sn__Point;
typedef struct {
    RtHandle __sn__name;
    long long __sn__age;
} __sn__Person;
typedef struct {
    RtHandle __sn__values;
    RtHandle __sn__label;
} __sn__WithArray;
typedef struct {
    RtHandle __sn__matrix;
    RtHandle __sn__name;
} __sn__WithArray2D;
typedef struct {
    __sn__Point __sn__inner;
    RtHandle __sn__name;
} __sn__Nested;

/* Forward declarations */
static RtManagedArena *__main_arena__ = NULL;

__sn__Point __sn__makePoint(RtManagedArena *, long long, long long);
__sn__Person __sn__makePerson(RtManagedArena *, RtHandle, long long);
__sn__WithArray __sn__makeWithArray(RtManagedArena *, RtHandle);
__sn__WithArray __sn__makeEmptyArray(RtManagedArena *);
__sn__WithArray2D __sn__makeWithArray2D(RtManagedArena *, RtHandle);
__sn__WithArray2D __sn__makeEmptyArray2D(RtManagedArena *);
__sn__Nested __sn__makeNested(RtManagedArena *, RtHandle);
__sn__Nested __sn__makeEmptyNested(RtManagedArena *);

/* Lambda forward declarations */
static RtAny __thread_thunk_0(void);
typedef struct {
    /* These fields match RtThreadArgs layout */
    void *func_ptr;
    void *args_data;
    size_t args_size;
    RtThreadResult *result;
    RtArena *caller_arena;
    RtArena *thread_arena;
    bool is_shared;
    bool is_private;
    /* Function-specific arguments follow */
    long long arg0;
    long long arg1;
} __ThreadArgs_0__;

static void *__thread_wrapper_0__(void *args_ptr) {
    __ThreadArgs_0__ *args = (__ThreadArgs_0__ *)args_ptr;

    /* Use arena created by rt_thread_spawn(). For shared mode, this is
     * the caller's arena. For default/private modes, it's a new arena. */
    RtArena *__arena__ = args->thread_arena;

    /* Set up panic context to catch panics in this thread */
    RtThreadPanicContext __panic_ctx__;
    rt_thread_panic_context_init(&__panic_ctx__, args->result, __arena__);
    if (setjmp(__panic_ctx__.jump_buffer) != 0) {
        /* Panic occurred - cleanup and return */
        rt_thread_panic_context_clear();
        return NULL;
    }

    /* Call the function with interceptor support */
    __sn__Point __result__;
    if (__rt_interceptor_count > 0) {
        RtAny __args[2];
        __args[0] = rt_box_int(args->arg0);
        __args[1] = rt_box_int(args->arg1);
        __rt_thunk_args = __args;
        __rt_thunk_arena = __arena__;
        RtAny __intercepted = rt_call_intercepted("makePoint", __args, 2, __thread_thunk_0);
        __result__ = *((__sn__Point *)rt_unbox_struct(__intercepted, 233133007));
    } else {
        __result__ = __sn__makePoint(__arena__, args->arg0, args->arg1);
    }

    /* Store result in thread result structure using runtime function */
    RtArena *__result_arena__ = args->thread_arena ? args->thread_arena : args->caller_arena;
    rt_thread_result_set_value(args->result, &__result__, sizeof(__sn__Point), __result_arena__);

    /* Clear panic context on successful completion */
    rt_thread_panic_context_clear();
    return NULL;
}

static RtAny __thread_thunk_1(void);
typedef struct {
    /* These fields match RtThreadArgs layout */
    void *func_ptr;
    void *args_data;
    size_t args_size;
    RtThreadResult *result;
    RtArena *caller_arena;
    RtArena *thread_arena;
    bool is_shared;
    bool is_private;
    /* Function-specific arguments follow */
    RtHandle arg0;
    long long arg1;
} __ThreadArgs_1__;

static void *__thread_wrapper_1__(void *args_ptr) {
    __ThreadArgs_1__ *args = (__ThreadArgs_1__ *)args_ptr;

    /* Use arena created by rt_thread_spawn(). For shared mode, this is
     * the caller's arena. For default/private modes, it's a new arena. */
    RtArena *__arena__ = args->thread_arena;

    /* Set up panic context to catch panics in this thread */
    RtThreadPanicContext __panic_ctx__;
    rt_thread_panic_context_init(&__panic_ctx__, args->result, __arena__);
    if (setjmp(__panic_ctx__.jump_buffer) != 0) {
        /* Panic occurred - cleanup and return */
        rt_thread_panic_context_clear();
        return NULL;
    }

    /* Call the function with interceptor support */
    __sn__Person __result__;
    if (__rt_interceptor_count > 0) {
        RtAny __args[2];
        __args[0] = rt_box_string((char *)rt_managed_pin(args->caller_arena, args->arg0));
        __args[1] = rt_box_int(args->arg1);
        __rt_thunk_args = __args;
        __rt_thunk_arena = __arena__;
        RtAny __intercepted = rt_call_intercepted("makePerson", __args, 2, __thread_thunk_1);
        __result__ = *((__sn__Person *)rt_unbox_struct(__intercepted, 1239407900));
    } else {
        __result__ = __sn__makePerson(__arena__, rt_managed_clone(__arena__, args->caller_arena, args->arg0), args->arg1);
    }

    /* Store result in thread result structure using runtime function */
    RtArena *__result_arena__ = args->thread_arena ? args->thread_arena : args->caller_arena;
    rt_thread_result_set_value(args->result, &__result__, sizeof(__sn__Person), __result_arena__);

    /* Clear panic context on successful completion */
    rt_thread_panic_context_clear();
    return NULL;
}

static RtAny __thread_thunk_2(void);
typedef struct {
    /* These fields match RtThreadArgs layout */
    void *func_ptr;
    void *args_data;
    size_t args_size;
    RtThreadResult *result;
    RtArena *caller_arena;
    RtArena *thread_arena;
    bool is_shared;
    bool is_private;
    /* Function-specific arguments follow */
    RtHandle arg0;
} __ThreadArgs_2__;

static void *__thread_wrapper_2__(void *args_ptr) {
    __ThreadArgs_2__ *args = (__ThreadArgs_2__ *)args_ptr;

    /* Use arena created by rt_thread_spawn(). For shared mode, this is
     * the caller's arena. For default/private modes, it's a new arena. */
    RtArena *__arena__ = args->thread_arena;

    /* Set up panic context to catch panics in this thread */
    RtThreadPanicContext __panic_ctx__;
    rt_thread_panic_context_init(&__panic_ctx__, args->result, __arena__);
    if (setjmp(__panic_ctx__.jump_buffer) != 0) {
        /* Panic occurred - cleanup and return */
        rt_thread_panic_context_clear();
        return NULL;
    }

    /* Call the function with interceptor support */
    __sn__WithArray __result__;
    if (__rt_interceptor_count > 0) {
        RtAny __args[1];
        __args[0] = rt_box_string((char *)rt_managed_pin(args->caller_arena, args->arg0));
        __rt_thunk_args = __args;
        __rt_thunk_arena = __arena__;
        RtAny __intercepted = rt_call_intercepted("makeWithArray", __args, 1, __thread_thunk_2);
        __result__ = *((__sn__WithArray *)rt_unbox_struct(__intercepted, 1211889920));
    } else {
        __result__ = __sn__makeWithArray(__arena__, rt_managed_clone(__arena__, args->caller_arena, args->arg0));
    }

    /* Store result in thread result structure using runtime function */
    RtArena *__result_arena__ = args->thread_arena ? args->thread_arena : args->caller_arena;
    rt_thread_result_set_value(args->result, &__result__, sizeof(__sn__WithArray), __result_arena__);

    /* Clear panic context on successful completion */
    rt_thread_panic_context_clear();
    return NULL;
}

static RtAny __thread_thunk_3(void);
typedef struct {
    /* These fields match RtThreadArgs layout */
    void *func_ptr;
    void *args_data;
    size_t args_size;
    RtThreadResult *result;
    RtArena *caller_arena;
    RtArena *thread_arena;
    bool is_shared;
    bool is_private;
    /* Function-specific arguments follow */
    RtHandle arg0;
} __ThreadArgs_3__;

static void *__thread_wrapper_3__(void *args_ptr) {
    __ThreadArgs_3__ *args = (__ThreadArgs_3__ *)args_ptr;

    /* Use arena created by rt_thread_spawn(). For shared mode, this is
     * the caller's arena. For default/private modes, it's a new arena. */
    RtArena *__arena__ = args->thread_arena;

    /* Set up panic context to catch panics in this thread */
    RtThreadPanicContext __panic_ctx__;
    rt_thread_panic_context_init(&__panic_ctx__, args->result, __arena__);
    if (setjmp(__panic_ctx__.jump_buffer) != 0) {
        /* Panic occurred - cleanup and return */
        rt_thread_panic_context_clear();
        return NULL;
    }

    /* Call the function with interceptor support */
    __sn__WithArray2D __result__;
    if (__rt_interceptor_count > 0) {
        RtAny __args[1];
        __args[0] = rt_box_string((char *)rt_managed_pin(args->caller_arena, args->arg0));
        __rt_thunk_args = __args;
        __rt_thunk_arena = __arena__;
        RtAny __intercepted = rt_call_intercepted("makeWithArray2D", __args, 1, __thread_thunk_3);
        __result__ = *((__sn__WithArray2D *)rt_unbox_struct(__intercepted, 1193164726));
    } else {
        __result__ = __sn__makeWithArray2D(__arena__, rt_managed_clone(__arena__, args->caller_arena, args->arg0));
    }

    /* Store result in thread result structure using runtime function */
    RtArena *__result_arena__ = args->thread_arena ? args->thread_arena : args->caller_arena;
    rt_thread_result_set_value(args->result, &__result__, sizeof(__sn__WithArray2D), __result_arena__);

    /* Clear panic context on successful completion */
    rt_thread_panic_context_clear();
    return NULL;
}

static RtAny __thread_thunk_4(void);
typedef struct {
    /* These fields match RtThreadArgs layout */
    void *func_ptr;
    void *args_data;
    size_t args_size;
    RtThreadResult *result;
    RtArena *caller_arena;
    RtArena *thread_arena;
    bool is_shared;
    bool is_private;
    /* Function-specific arguments follow */
    RtHandle arg0;
} __ThreadArgs_4__;

static void *__thread_wrapper_4__(void *args_ptr) {
    __ThreadArgs_4__ *args = (__ThreadArgs_4__ *)args_ptr;

    /* Use arena created by rt_thread_spawn(). For shared mode, this is
     * the caller's arena. For default/private modes, it's a new arena. */
    RtArena *__arena__ = args->thread_arena;

    /* Set up panic context to catch panics in this thread */
    RtThreadPanicContext __panic_ctx__;
    rt_thread_panic_context_init(&__panic_ctx__, args->result, __arena__);
    if (setjmp(__panic_ctx__.jump_buffer) != 0) {
        /* Panic occurred - cleanup and return */
        rt_thread_panic_context_clear();
        return NULL;
    }

    /* Call the function with interceptor support */
    __sn__Nested __result__;
    if (__rt_interceptor_count > 0) {
        RtAny __args[1];
        __args[0] = rt_box_string((char *)rt_managed_pin(args->caller_arena, args->arg0));
        __rt_thunk_args = __args;
        __rt_thunk_arena = __arena__;
        RtAny __intercepted = rt_call_intercepted("makeNested", __args, 1, __thread_thunk_4);
        __result__ = *((__sn__Nested *)rt_unbox_struct(__intercepted, 1161173800));
    } else {
        __result__ = __sn__makeNested(__arena__, rt_managed_clone(__arena__, args->caller_arena, args->arg0));
    }

    /* Store result in thread result structure using runtime function */
    RtArena *__result_arena__ = args->thread_arena ? args->thread_arena : args->caller_arena;
    rt_thread_result_set_value(args->result, &__result__, sizeof(__sn__Nested), __result_arena__);

    /* Clear panic context on successful completion */
    rt_thread_panic_context_clear();
    return NULL;
}

static RtAny __thread_thunk_5(void);
typedef struct {
    /* These fields match RtThreadArgs layout */
    void *func_ptr;
    void *args_data;
    size_t args_size;
    RtThreadResult *result;
    RtArena *caller_arena;
    RtArena *thread_arena;
    bool is_shared;
    bool is_private;
    /* Function-specific arguments follow */
    long long arg0;
    long long arg1;
} __ThreadArgs_5__;

static void *__thread_wrapper_5__(void *args_ptr) {
    __ThreadArgs_5__ *args = (__ThreadArgs_5__ *)args_ptr;

    /* Use arena created by rt_thread_spawn(). For shared mode, this is
     * the caller's arena. For default/private modes, it's a new arena. */
    RtArena *__arena__ = args->thread_arena;

    /* Set up panic context to catch panics in this thread */
    RtThreadPanicContext __panic_ctx__;
    rt_thread_panic_context_init(&__panic_ctx__, args->result, __arena__);
    if (setjmp(__panic_ctx__.jump_buffer) != 0) {
        /* Panic occurred - cleanup and return */
        rt_thread_panic_context_clear();
        return NULL;
    }

    /* Call the function with interceptor support */
    __sn__Point __result__;
    if (__rt_interceptor_count > 0) {
        RtAny __args[2];
        __args[0] = rt_box_int(args->arg0);
        __args[1] = rt_box_int(args->arg1);
        __rt_thunk_args = __args;
        __rt_thunk_arena = __arena__;
        RtAny __intercepted = rt_call_intercepted("makePoint", __args, 2, __thread_thunk_5);
        __result__ = *((__sn__Point *)rt_unbox_struct(__intercepted, 233133007));
    } else {
        __result__ = __sn__makePoint(__arena__, args->arg0, args->arg1);
    }

    /* Store result in thread result structure using runtime function */
    RtArena *__result_arena__ = args->thread_arena ? args->thread_arena : args->caller_arena;
    rt_thread_result_set_value(args->result, &__result__, sizeof(__sn__Point), __result_arena__);

    /* Clear panic context on successful completion */
    rt_thread_panic_context_clear();
    return NULL;
}


// Test conditional thread spawn for struct types
// Covers: primitive fields, string fields, 1D/2D arrays, nested structs
__sn__Point __sn__makePoint(RtManagedArena *__caller_arena__, long long __sn__x, long long __sn__y) {
    RtManagedArena *__local_arena__ = rt_managed_arena_create_child(__caller_arena__);
    __sn__Point _return_value = {0};
    _return_value = (__sn__Point){ .__sn__x = __sn__x, .__sn__y = __sn__y };
    goto __sn__makePoint_return;
__sn__makePoint_return:
    rt_managed_arena_destroy_child(__local_arena__);
    return _return_value;
}

__sn__Person __sn__makePerson(RtManagedArena *__caller_arena__, RtHandle __sn__name, long long __sn__age) {
    RtManagedArena *__local_arena__ = rt_managed_arena_create_child(__caller_arena__);
    __sn__name = rt_managed_clone_any(__local_arena__, __caller_arena__, __sn__name);
    __sn__Person _return_value = {0};
    _return_value = (__sn__Person){ .__sn__name = __sn__name, .__sn__age = __sn__age };
    goto __sn__makePerson_return;
__sn__makePerson_return:
    _return_value.__sn__name = rt_managed_promote(__caller_arena__, __local_arena__, _return_value.__sn__name);
    rt_managed_arena_destroy_child(__local_arena__);
    return _return_value;
}

__sn__WithArray __sn__makeWithArray(RtManagedArena *__caller_arena__, RtHandle __sn__label) {
    RtManagedArena *__local_arena__ = rt_managed_arena_create_child(__caller_arena__);
    __sn__label = rt_managed_clone_any(__local_arena__, __caller_arena__, __sn__label);
    __sn__WithArray _return_value = {0};
    RtThreadHandle *__values_pending__ = NULL;
    RtHandle __sn__values = rt_array_create_long_h(__local_arena__, 3, (long long[]){1LL, 2LL, 3LL});
    _return_value = (__sn__WithArray){ .__sn__values = __sn__values, .__sn__label = __sn__label };
    goto __sn__makeWithArray_return;
__sn__makeWithArray_return:
    _return_value.__sn__values = rt_managed_promote(__caller_arena__, __local_arena__, _return_value.__sn__values);
    _return_value.__sn__label = rt_managed_promote(__caller_arena__, __local_arena__, _return_value.__sn__label);
    rt_managed_arena_destroy_child(__local_arena__);
    return _return_value;
}

__sn__WithArray __sn__makeEmptyArray(RtManagedArena *__caller_arena__) {
    RtManagedArena *__local_arena__ = rt_managed_arena_create_child(__caller_arena__);
    __sn__WithArray _return_value = {0};
    RtThreadHandle *__values_pending__ = NULL;
    RtHandle __sn__values = rt_array_create_long_h(__local_arena__, 1, (long long[]){0LL});
    _return_value = (__sn__WithArray){ .__sn__values = __sn__values, .__sn__label = rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "") };
    goto __sn__makeEmptyArray_return;
__sn__makeEmptyArray_return:
    _return_value.__sn__values = rt_managed_promote(__caller_arena__, __local_arena__, _return_value.__sn__values);
    _return_value.__sn__label = rt_managed_promote(__caller_arena__, __local_arena__, _return_value.__sn__label);
    rt_managed_arena_destroy_child(__local_arena__);
    return _return_value;
}

__sn__WithArray2D __sn__makeWithArray2D(RtManagedArena *__caller_arena__, RtHandle __sn__name) {
    RtManagedArena *__local_arena__ = rt_managed_arena_create_child(__caller_arena__);
    __sn__name = rt_managed_clone_any(__local_arena__, __caller_arena__, __sn__name);
    __sn__WithArray2D _return_value = {0};
    RtThreadHandle *__row1_pending__ = NULL;
    RtHandle __sn__row1 = rt_array_create_long_h(__local_arena__, 2, (long long[]){1LL, 2LL});
    RtThreadHandle *__row2_pending__ = NULL;
    RtHandle __sn__row2 = rt_array_create_long_h(__local_arena__, 2, (long long[]){3LL, 4LL});
    RtThreadHandle *__matrix_pending__ = NULL;
    RtHandle __sn__matrix = rt_array_create_ptr_h(__local_arena__, 2, (RtHandle[]){__sn__row1, __sn__row2});
    _return_value = (__sn__WithArray2D){ .__sn__matrix = __sn__matrix, .__sn__name = __sn__name };
    goto __sn__makeWithArray2D_return;
__sn__makeWithArray2D_return:
    _return_value.__sn__matrix = rt_managed_promote_array_handle(__caller_arena__, __local_arena__, _return_value.__sn__matrix);
    _return_value.__sn__name = rt_managed_promote(__caller_arena__, __local_arena__, _return_value.__sn__name);
    rt_managed_arena_destroy_child(__local_arena__);
    return _return_value;
}

__sn__WithArray2D __sn__makeEmptyArray2D(RtManagedArena *__caller_arena__) {
    RtManagedArena *__local_arena__ = rt_managed_arena_create_child(__caller_arena__);
    __sn__WithArray2D _return_value = {0};
    RtThreadHandle *__row_pending__ = NULL;
    RtHandle __sn__row = rt_array_create_long_h(__local_arena__, 1, (long long[]){0LL});
    RtThreadHandle *__matrix_pending__ = NULL;
    RtHandle __sn__matrix = rt_array_create_ptr_h(__local_arena__, 1, (RtHandle[]){__sn__row});
    _return_value = (__sn__WithArray2D){ .__sn__matrix = __sn__matrix, .__sn__name = rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "") };
    goto __sn__makeEmptyArray2D_return;
__sn__makeEmptyArray2D_return:
    _return_value.__sn__matrix = rt_managed_promote_array_handle(__caller_arena__, __local_arena__, _return_value.__sn__matrix);
    _return_value.__sn__name = rt_managed_promote(__caller_arena__, __local_arena__, _return_value.__sn__name);
    rt_managed_arena_destroy_child(__local_arena__);
    return _return_value;
}

__sn__Nested __sn__makeNested(RtManagedArena *__caller_arena__, RtHandle __sn__name) {
    RtManagedArena *__local_arena__ = rt_managed_arena_create_child(__caller_arena__);
    __sn__name = rt_managed_clone_any(__local_arena__, __caller_arena__, __sn__name);
    __sn__Nested _return_value = {0};
    _return_value = (__sn__Nested){ .__sn__inner = (__sn__Point){ .__sn__x = 5LL, .__sn__y = 6LL }, .__sn__name = __sn__name };
    goto __sn__makeNested_return;
__sn__makeNested_return:
    _return_value.__sn__name = rt_managed_promote(__caller_arena__, __local_arena__, _return_value.__sn__name);
    rt_managed_arena_destroy_child(__local_arena__);
    return _return_value;
}

__sn__Nested __sn__makeEmptyNested(RtManagedArena *__caller_arena__) {
    RtManagedArena *__local_arena__ = rt_managed_arena_create_child(__caller_arena__);
    __sn__Nested _return_value = {0};
    _return_value = (__sn__Nested){ .__sn__inner = (__sn__Point){ .__sn__x = 0LL, .__sn__y = 0LL }, .__sn__name = rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "") };
    goto __sn__makeEmptyNested_return;
__sn__makeEmptyNested_return:
    _return_value.__sn__name = rt_managed_promote(__caller_arena__, __local_arena__, _return_value.__sn__name);
    rt_managed_arena_destroy_child(__local_arena__);
    return _return_value;
}

int main() {
    RtManagedArena *__local_arena__ = rt_managed_arena_create();
    __main_arena__ = __local_arena__;
    int _return_value = 0;
    RtThreadHandle *__doSpawn_pending__ = NULL;
    bool __sn__doSpawn = 1L;
    // Test 1: Point struct (primitives only)
    RtThreadHandle *__pt_pending__ = NULL;
    __sn__Point __sn__pt = (__sn__Point){ .__sn__x = 0LL, .__sn__y = 0LL };
    if (__sn__doSpawn) {
        {
            (__pt_pending__ = ({
    /* Allocate thread arguments structure */
    __ThreadArgs_0__ *__spawn_args_0__ = (__ThreadArgs_0__ *)rt_arena_alloc(__local_arena__, sizeof(__ThreadArgs_0__));
    __spawn_args_0__->caller_arena = __local_arena__;
    __spawn_args_0__->thread_arena = NULL;
    __spawn_args_0__->result = rt_thread_result_create(__local_arena__);
    __spawn_args_0__->is_shared = false;
    __spawn_args_0__->is_private = false;
    __spawn_args_0__->arg0 = 10LL; __spawn_args_0__->arg1 = 20LL; 
    /* Spawn the thread */
    RtThreadHandle *__spawn_handle_0__ = rt_thread_spawn(__local_arena__, __thread_wrapper_0__, __spawn_args_0__);
    __spawn_handle_0__->result_type = RT_TYPE_STRUCT;
    __spawn_handle_0__;
}));
        }
    }
    if (__pt_pending__ != NULL) {
        __sn__pt = *(__sn__Point *)rt_thread_sync_with_result(__pt_pending__, __local_arena__, RT_TYPE_STRUCT);
        __pt_pending__ = NULL;
    }
    rt_println(({
        char *_p0 = rt_to_string_long(__local_arena__, __sn__pt.__sn__x);
        char *_p1 = rt_to_string_long(__local_arena__, __sn__pt.__sn__y);
        char *_r = rt_str_concat(__local_arena__, "Point: (", _p0);
        _r = rt_str_concat(__local_arena__, _r, ", ");
        _r = rt_str_concat(__local_arena__, _r, _p1);
        _r = rt_str_concat(__local_arena__, _r, ")");
        _r;
    }));
    // Test 2: Person struct with string field
    RtThreadHandle *__p_pending__ = NULL;
    __sn__Person __sn__p = (__sn__Person){ .__sn__name = rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, ""), .__sn__age = 0LL };
    if (__sn__doSpawn) {
        {
            (__p_pending__ = ({
    /* Allocate thread arguments structure */
    __ThreadArgs_1__ *__spawn_args_1__ = (__ThreadArgs_1__ *)rt_arena_alloc(__local_arena__, sizeof(__ThreadArgs_1__));
    __spawn_args_1__->caller_arena = __local_arena__;
    __spawn_args_1__->thread_arena = NULL;
    __spawn_args_1__->result = rt_thread_result_create(__local_arena__);
    __spawn_args_1__->is_shared = false;
    __spawn_args_1__->is_private = false;
    __spawn_args_1__->arg0 = rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "Alice"); __spawn_args_1__->arg1 = 30LL; 
    /* Spawn the thread */
    RtThreadHandle *__spawn_handle_1__ = rt_thread_spawn(__local_arena__, __thread_wrapper_1__, __spawn_args_1__);
    __spawn_handle_1__->result_type = RT_TYPE_STRUCT;
    __spawn_handle_1__;
}));
        }
    }
    if (__p_pending__ != NULL) {
        __sn__p = *(__sn__Person *)rt_thread_sync_with_result_keep_arena(__p_pending__, __local_arena__, RT_TYPE_STRUCT);
                __sn__p.__sn__name = rt_managed_promote(__local_arena__, __p_pending__->thread_arena, __sn__p.__sn__name);
        rt_thread_cleanup_arena(__p_pending__);
        __p_pending__ = NULL;
    }
    rt_println(({
        char *_p0 = rt_to_string_long(__local_arena__, __sn__p.__sn__age);
        char *_r = rt_str_concat(__local_arena__, "Person: ", ((char *)rt_managed_pin(__local_arena__, __sn__p.__sn__name)));
        _r = rt_str_concat(__local_arena__, _r, ", age ");
        _r = rt_str_concat(__local_arena__, _r, _p0);
        _r;
    }));
    // Test 3: Struct with 1D array field
    RtThreadHandle *__wa_pending__ = NULL;
    __sn__WithArray __sn__wa = __sn__makeEmptyArray(__local_arena__);
    if (__sn__doSpawn) {
        {
            (__wa_pending__ = ({
    /* Allocate thread arguments structure */
    __ThreadArgs_2__ *__spawn_args_2__ = (__ThreadArgs_2__ *)rt_arena_alloc(__local_arena__, sizeof(__ThreadArgs_2__));
    __spawn_args_2__->caller_arena = __local_arena__;
    __spawn_args_2__->thread_arena = NULL;
    __spawn_args_2__->result = rt_thread_result_create(__local_arena__);
    __spawn_args_2__->is_shared = false;
    __spawn_args_2__->is_private = false;
    __spawn_args_2__->arg0 = rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "numbers"); 
    /* Spawn the thread */
    RtThreadHandle *__spawn_handle_2__ = rt_thread_spawn(__local_arena__, __thread_wrapper_2__, __spawn_args_2__);
    __spawn_handle_2__->result_type = RT_TYPE_STRUCT;
    __spawn_handle_2__;
}));
        }
    }
    if (__wa_pending__ != NULL) {
        __sn__wa = *(__sn__WithArray *)rt_thread_sync_with_result_keep_arena(__wa_pending__, __local_arena__, RT_TYPE_STRUCT);
                __sn__wa.__sn__values = rt_managed_promote(__local_arena__, __wa_pending__->thread_arena, __sn__wa.__sn__values);
        __sn__wa.__sn__label = rt_managed_promote(__local_arena__, __wa_pending__->thread_arena, __sn__wa.__sn__label);
        rt_thread_cleanup_arena(__wa_pending__);
        __wa_pending__ = NULL;
    }
    rt_println(({
        char *_p0 = rt_to_string_long(__local_arena__, ((long long *)rt_managed_pin_array(__local_arena__, __sn__wa.__sn__values))[0LL]);
        char *_r = rt_str_concat(__local_arena__, "WithArray: ", ((char *)rt_managed_pin(__local_arena__, __sn__wa.__sn__label)));
        _r = rt_str_concat(__local_arena__, _r, ", values[0]=");
        _r = rt_str_concat(__local_arena__, _r, _p0);
        _r;
    }));
    // Test 4: Struct with 2D array field
    RtThreadHandle *__wa2d_pending__ = NULL;
    __sn__WithArray2D __sn__wa2d = __sn__makeEmptyArray2D(__local_arena__);
    if (__sn__doSpawn) {
        {
            (__wa2d_pending__ = ({
    /* Allocate thread arguments structure */
    __ThreadArgs_3__ *__spawn_args_3__ = (__ThreadArgs_3__ *)rt_arena_alloc(__local_arena__, sizeof(__ThreadArgs_3__));
    __spawn_args_3__->caller_arena = __local_arena__;
    __spawn_args_3__->thread_arena = NULL;
    __spawn_args_3__->result = rt_thread_result_create(__local_arena__);
    __spawn_args_3__->is_shared = false;
    __spawn_args_3__->is_private = false;
    __spawn_args_3__->arg0 = rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "grid"); 
    /* Spawn the thread */
    RtThreadHandle *__spawn_handle_3__ = rt_thread_spawn(__local_arena__, __thread_wrapper_3__, __spawn_args_3__);
    __spawn_handle_3__->result_type = RT_TYPE_STRUCT;
    __spawn_handle_3__;
}));
        }
    }
    if (__wa2d_pending__ != NULL) {
        __sn__wa2d = *(__sn__WithArray2D *)rt_thread_sync_with_result_keep_arena(__wa2d_pending__, __local_arena__, RT_TYPE_STRUCT);
                __sn__wa2d.__sn__matrix = rt_managed_promote_array_handle(__local_arena__, __wa2d_pending__->thread_arena, __sn__wa2d.__sn__matrix);
        __sn__wa2d.__sn__name = rt_managed_promote(__local_arena__, __wa2d_pending__->thread_arena, __sn__wa2d.__sn__name);
        rt_thread_cleanup_arena(__wa2d_pending__);
        __wa2d_pending__ = NULL;
    }
    rt_println(({
        char *_p0 = rt_to_string_long(__local_arena__, ((long long *)rt_managed_pin_array(__local_arena__, ((RtHandle *)rt_managed_pin_array(__local_arena__, __sn__wa2d.__sn__matrix))[1LL]))[0LL]);
        char *_r = rt_str_concat(__local_arena__, "WithArray2D: ", ((char *)rt_managed_pin(__local_arena__, __sn__wa2d.__sn__name)));
        _r = rt_str_concat(__local_arena__, _r, ", matrix[1][0]=");
        _r = rt_str_concat(__local_arena__, _r, _p0);
        _r;
    }));
    // Test 5: Nested struct
    RtThreadHandle *__n_pending__ = NULL;
    __sn__Nested __sn__n = __sn__makeEmptyNested(__local_arena__);
    if (__sn__doSpawn) {
        {
            (__n_pending__ = ({
    /* Allocate thread arguments structure */
    __ThreadArgs_4__ *__spawn_args_4__ = (__ThreadArgs_4__ *)rt_arena_alloc(__local_arena__, sizeof(__ThreadArgs_4__));
    __spawn_args_4__->caller_arena = __local_arena__;
    __spawn_args_4__->thread_arena = NULL;
    __spawn_args_4__->result = rt_thread_result_create(__local_arena__);
    __spawn_args_4__->is_shared = false;
    __spawn_args_4__->is_private = false;
    __spawn_args_4__->arg0 = rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "nested"); 
    /* Spawn the thread */
    RtThreadHandle *__spawn_handle_4__ = rt_thread_spawn(__local_arena__, __thread_wrapper_4__, __spawn_args_4__);
    __spawn_handle_4__->result_type = RT_TYPE_STRUCT;
    __spawn_handle_4__;
}));
        }
    }
    if (__n_pending__ != NULL) {
        __sn__n = *(__sn__Nested *)rt_thread_sync_with_result_keep_arena(__n_pending__, __local_arena__, RT_TYPE_STRUCT);
                __sn__n.__sn__name = rt_managed_promote(__local_arena__, __n_pending__->thread_arena, __sn__n.__sn__name);
        rt_thread_cleanup_arena(__n_pending__);
        __n_pending__ = NULL;
    }
    rt_println(({
        char *_p0 = rt_to_string_long(__local_arena__, __sn__n.__sn__inner.__sn__x);
        char *_p1 = rt_to_string_long(__local_arena__, __sn__n.__sn__inner.__sn__y);
        char *_r = rt_str_concat(__local_arena__, "Nested: ", ((char *)rt_managed_pin(__local_arena__, __sn__n.__sn__name)));
        _r = rt_str_concat(__local_arena__, _r, ", inner=(");
        _r = rt_str_concat(__local_arena__, _r, _p0);
        _r = rt_str_concat(__local_arena__, _r, ", ");
        _r = rt_str_concat(__local_arena__, _r, _p1);
        _r = rt_str_concat(__local_arena__, _r, ")");
        _r;
    }));
    // Test 6: non-spawn path
    RtThreadHandle *__noSpawn_pending__ = NULL;
    __sn__Point __sn__noSpawn = (__sn__Point){ .__sn__x = 99LL, .__sn__y = 98LL };
    if (0L) {
        {
            (__noSpawn_pending__ = ({
    /* Allocate thread arguments structure */
    __ThreadArgs_5__ *__spawn_args_5__ = (__ThreadArgs_5__ *)rt_arena_alloc(__local_arena__, sizeof(__ThreadArgs_5__));
    __spawn_args_5__->caller_arena = __local_arena__;
    __spawn_args_5__->thread_arena = NULL;
    __spawn_args_5__->result = rt_thread_result_create(__local_arena__);
    __spawn_args_5__->is_shared = false;
    __spawn_args_5__->is_private = false;
    __spawn_args_5__->arg0 = 1LL; __spawn_args_5__->arg1 = 2LL; 
    /* Spawn the thread */
    RtThreadHandle *__spawn_handle_5__ = rt_thread_spawn(__local_arena__, __thread_wrapper_5__, __spawn_args_5__);
    __spawn_handle_5__->result_type = RT_TYPE_STRUCT;
    __spawn_handle_5__;
}));
        }
    }
    if (__noSpawn_pending__ != NULL) {
        __sn__noSpawn = *(__sn__Point *)rt_thread_sync_with_result(__noSpawn_pending__, __local_arena__, RT_TYPE_STRUCT);
        __noSpawn_pending__ = NULL;
    }
    rt_println(({
        char *_p0 = rt_to_string_long(__local_arena__, __sn__noSpawn.__sn__x);
        char *_p1 = rt_to_string_long(__local_arena__, __sn__noSpawn.__sn__y);
        char *_r = rt_str_concat(__local_arena__, "no-spawn: (", _p0);
        _r = rt_str_concat(__local_arena__, _r, ", ");
        _r = rt_str_concat(__local_arena__, _r, _p1);
        _r = rt_str_concat(__local_arena__, _r, ")");
        _r;
    }));
    goto main_return;
main_return:
    rt_managed_arena_destroy(__local_arena__);
    return _return_value;
}


/* Interceptor thunk definitions */
static RtAny __thread_thunk_0(void) {
    __sn__Point __tmp_result = __sn__makePoint((RtArena *)__rt_thunk_arena, rt_unbox_int(__rt_thunk_args[0]), rt_unbox_int(__rt_thunk_args[1]));
    RtAny __result = rt_box_struct((RtArena *)__rt_thunk_arena, &__tmp_result, sizeof(__sn__Point), 233133007);
    return __result;
}

static RtAny __thread_thunk_1(void) {
    __sn__Person __tmp_result = __sn__makePerson((RtArena *)__rt_thunk_arena, rt_managed_strdup((RtManagedArena *)__rt_thunk_arena, RT_HANDLE_NULL, rt_unbox_string(__rt_thunk_args[0])), rt_unbox_int(__rt_thunk_args[1]));
    RtAny __result = rt_box_struct((RtArena *)__rt_thunk_arena, &__tmp_result, sizeof(__sn__Person), 1239407900);
    return __result;
}

static RtAny __thread_thunk_2(void) {
    __sn__WithArray __tmp_result = __sn__makeWithArray((RtArena *)__rt_thunk_arena, rt_managed_strdup((RtManagedArena *)__rt_thunk_arena, RT_HANDLE_NULL, rt_unbox_string(__rt_thunk_args[0])));
    RtAny __result = rt_box_struct((RtArena *)__rt_thunk_arena, &__tmp_result, sizeof(__sn__WithArray), 1211889920);
    return __result;
}

static RtAny __thread_thunk_3(void) {
    __sn__WithArray2D __tmp_result = __sn__makeWithArray2D((RtArena *)__rt_thunk_arena, rt_managed_strdup((RtManagedArena *)__rt_thunk_arena, RT_HANDLE_NULL, rt_unbox_string(__rt_thunk_args[0])));
    RtAny __result = rt_box_struct((RtArena *)__rt_thunk_arena, &__tmp_result, sizeof(__sn__WithArray2D), 1193164726);
    return __result;
}

static RtAny __thread_thunk_4(void) {
    __sn__Nested __tmp_result = __sn__makeNested((RtArena *)__rt_thunk_arena, rt_managed_strdup((RtManagedArena *)__rt_thunk_arena, RT_HANDLE_NULL, rt_unbox_string(__rt_thunk_args[0])));
    RtAny __result = rt_box_struct((RtArena *)__rt_thunk_arena, &__tmp_result, sizeof(__sn__Nested), 1161173800);
    return __result;
}

static RtAny __thread_thunk_5(void) {
    __sn__Point __tmp_result = __sn__makePoint((RtArena *)__rt_thunk_arena, rt_unbox_int(__rt_thunk_args[0]), rt_unbox_int(__rt_thunk_args[1]));
    RtAny __result = rt_box_struct((RtArena *)__rt_thunk_arena, &__tmp_result, sizeof(__sn__Point), 233133007);
    return __result;
}

