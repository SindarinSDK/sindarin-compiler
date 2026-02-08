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

/* User-specified includes */
#include <zlib.h>


/* Closure type for lambdas */
typedef struct __Closure__ { void *fn; RtArena *arena; size_t size; } __Closure__;

/* Native struct forward declarations */
typedef struct RtUuid RtUuid;

/* Native function extern declarations */
extern RtUuid * sn_uuid_create(RtManagedArena *);
extern RtUuid * sn_uuid_v4(RtManagedArena *);
extern RtUuid * sn_uuid_v5(RtManagedArena *, RtUuid *, const char *);
extern RtUuid * sn_uuid_v7(RtManagedArena *);
extern RtUuid * sn_uuid_from_string(RtManagedArena *, const char *);
extern RtUuid * sn_uuid_from_hex(RtManagedArena *, const char *);
extern RtUuid * sn_uuid_from_base64(RtManagedArena *, const char *);
extern RtUuid * sn_uuid_nil(RtManagedArena *);
extern RtUuid * sn_uuid_max(RtManagedArena *);
extern RtUuid * sn_uuid_namespace_dns(RtManagedArena *);
extern RtUuid * sn_uuid_namespace_url(RtManagedArena *);
extern RtUuid * sn_uuid_namespace_oid(RtManagedArena *);
extern RtUuid * sn_uuid_namespace_x500(RtManagedArena *);
extern RtHandle sn_uuid_to_string(RtManagedArena *, RtUuid *);
extern RtHandle sn_uuid_to_hex(RtManagedArena *, RtUuid *);
extern RtHandle sn_uuid_to_base64(RtManagedArena *, RtUuid *);

/* Forward declarations */
static RtManagedArena *__main_arena__ = NULL;

void __sn__test_basic_compress_decompress_A(RtManagedArena *);

/* Interceptor thunk forward declarations */
static RtAny __thunk_0(void);
static RtAny __thunk_1(void);
static RtAny __thunk_2(void);
static RtAny __thunk_3(void);
static RtAny __thunk_4(void);
static RtAny __thunk_5(void);
static RtAny __thunk_6(void);
static RtAny __thunk_7(void);
static RtAny __thunk_8(void);
static RtAny __thunk_9(void);
static RtAny __thunk_10(void);
static RtAny __thunk_11(void);
static RtAny __thunk_12(void);
static RtAny __thunk_13(void);
static RtAny __thunk_14(void);
static RtAny __thunk_15(void);
static RtAny __thunk_16(void);
static RtAny __thunk_17(void);
static RtAny __thunk_18(void);
static RtAny __thunk_19(void);
static RtAny __thunk_20(void);
static RtAny __thunk_21(void);
static RtAny __thunk_22(void);
static RtAny __thunk_23(void);
static RtAny __thunk_24(void);
static RtAny __thunk_25(void);
static RtAny __thunk_26(void);
static RtAny __thunk_27(void);
static RtAny __thunk_28(void);
static RtAny __thunk_29(void);
static RtAny __thunk_30(void);
static RtAny __thunk_31(void);
static RtAny __thunk_32(void);

// Test SDK compression module
long long __sn__zlibA__zlibOk(RtManagedArena *);
long long __sn__zlibA__zlibStreamEnd(RtManagedArena *);
long long __sn__zlibA__zlibNeedDict(RtManagedArena *);
long long __sn__zlibA__zlibErrno(RtManagedArena *);
long long __sn__zlibA__zlibStreamError(RtManagedArena *);
long long __sn__zlibA__zlibDataError(RtManagedArena *);
long long __sn__zlibA__zlibMemError(RtManagedArena *);
long long __sn__zlibA__zlibBufError(RtManagedArena *);
long long __sn__zlibA__zlibVersionError(RtManagedArena *);
long long __sn__zlibA__noCompression(RtManagedArena *);
long long __sn__zlibA__bestSpeed(RtManagedArena *);
long long __sn__zlibA__bestCompression(RtManagedArena *);
long long __sn__zlibA__defaultCompression(RtManagedArena *);
RtHandle __sn__zlibA__errorMessage(RtManagedArena *, long long);
long long __sn__zlibA__compressTo(RtManagedArena *, RtHandle, RtHandle);
long long __sn__zlibA__compressToLevel(RtManagedArena *, RtHandle, RtHandle, long long);
long long __sn__zlibA__decompressTo(RtManagedArena *, RtHandle, RtHandle);
long long __sn__zlibA__maxCompressedSize(RtManagedArena *, long long);
RtHandle __sn__zlibA__compressData(RtManagedArena *, RtHandle);
RtHandle __sn__zlibA__compressDataLevel(RtManagedArena *, RtHandle, long long);
RtHandle __sn__zlibA__decompressData(RtManagedArena *, RtHandle, long long);
bool __sn__zlibA__isCompressed(RtManagedArena *, RtHandle);
double __sn__zlibA__compressionRatio(RtManagedArena *, long long, long long);
double __sn__zlibA__spaceSaved(RtManagedArena *, long long, long long);
long long __sn__zlibA__zlibOk(RtManagedArena *__caller_arena__) {
    RtManagedArena *__local_arena__ = rt_managed_arena_create_child(__caller_arena__);
    long long _return_value = 0;
    _return_value = 0LL;
    goto __sn__zlibA__zlibOk_return;
__sn__zlibA__zlibOk_return:
    rt_managed_arena_destroy_child(__local_arena__);
    return _return_value;
}

long long __sn__zlibA__zlibStreamEnd(RtManagedArena *__caller_arena__) {
    RtManagedArena *__local_arena__ = rt_managed_arena_create_child(__caller_arena__);
    long long _return_value = 0;
    _return_value = 1LL;
    goto __sn__zlibA__zlibStreamEnd_return;
__sn__zlibA__zlibStreamEnd_return:
    rt_managed_arena_destroy_child(__local_arena__);
    return _return_value;
}

long long __sn__zlibA__zlibNeedDict(RtManagedArena *__caller_arena__) {
    RtManagedArena *__local_arena__ = rt_managed_arena_create_child(__caller_arena__);
    long long _return_value = 0;
    _return_value = 2LL;
    goto __sn__zlibA__zlibNeedDict_return;
__sn__zlibA__zlibNeedDict_return:
    rt_managed_arena_destroy_child(__local_arena__);
    return _return_value;
}

long long __sn__zlibA__zlibErrno(RtManagedArena *__caller_arena__) {
    RtManagedArena *__local_arena__ = rt_managed_arena_create_child(__caller_arena__);
    long long _return_value = 0;
    _return_value = -1LL;
    goto __sn__zlibA__zlibErrno_return;
__sn__zlibA__zlibErrno_return:
    rt_managed_arena_destroy_child(__local_arena__);
    return _return_value;
}

long long __sn__zlibA__zlibStreamError(RtManagedArena *__caller_arena__) {
    RtManagedArena *__local_arena__ = rt_managed_arena_create_child(__caller_arena__);
    long long _return_value = 0;
    _return_value = -2LL;
    goto __sn__zlibA__zlibStreamError_return;
__sn__zlibA__zlibStreamError_return:
    rt_managed_arena_destroy_child(__local_arena__);
    return _return_value;
}

long long __sn__zlibA__zlibDataError(RtManagedArena *__caller_arena__) {
    RtManagedArena *__local_arena__ = rt_managed_arena_create_child(__caller_arena__);
    long long _return_value = 0;
    _return_value = -3LL;
    goto __sn__zlibA__zlibDataError_return;
__sn__zlibA__zlibDataError_return:
    rt_managed_arena_destroy_child(__local_arena__);
    return _return_value;
}

long long __sn__zlibA__zlibMemError(RtManagedArena *__caller_arena__) {
    RtManagedArena *__local_arena__ = rt_managed_arena_create_child(__caller_arena__);
    long long _return_value = 0;
    _return_value = -4LL;
    goto __sn__zlibA__zlibMemError_return;
__sn__zlibA__zlibMemError_return:
    rt_managed_arena_destroy_child(__local_arena__);
    return _return_value;
}

long long __sn__zlibA__zlibBufError(RtManagedArena *__caller_arena__) {
    RtManagedArena *__local_arena__ = rt_managed_arena_create_child(__caller_arena__);
    long long _return_value = 0;
    _return_value = -5LL;
    goto __sn__zlibA__zlibBufError_return;
__sn__zlibA__zlibBufError_return:
    rt_managed_arena_destroy_child(__local_arena__);
    return _return_value;
}

long long __sn__zlibA__zlibVersionError(RtManagedArena *__caller_arena__) {
    RtManagedArena *__local_arena__ = rt_managed_arena_create_child(__caller_arena__);
    long long _return_value = 0;
    _return_value = -6LL;
    goto __sn__zlibA__zlibVersionError_return;
__sn__zlibA__zlibVersionError_return:
    rt_managed_arena_destroy_child(__local_arena__);
    return _return_value;
}

long long __sn__zlibA__noCompression(RtManagedArena *__caller_arena__) {
    RtManagedArena *__local_arena__ = rt_managed_arena_create_child(__caller_arena__);
    long long _return_value = 0;
    _return_value = 0LL;
    goto __sn__zlibA__noCompression_return;
__sn__zlibA__noCompression_return:
    rt_managed_arena_destroy_child(__local_arena__);
    return _return_value;
}

long long __sn__zlibA__bestSpeed(RtManagedArena *__caller_arena__) {
    RtManagedArena *__local_arena__ = rt_managed_arena_create_child(__caller_arena__);
    long long _return_value = 0;
    _return_value = 1LL;
    goto __sn__zlibA__bestSpeed_return;
__sn__zlibA__bestSpeed_return:
    rt_managed_arena_destroy_child(__local_arena__);
    return _return_value;
}

long long __sn__zlibA__bestCompression(RtManagedArena *__caller_arena__) {
    RtManagedArena *__local_arena__ = rt_managed_arena_create_child(__caller_arena__);
    long long _return_value = 0;
    _return_value = 9LL;
    goto __sn__zlibA__bestCompression_return;
__sn__zlibA__bestCompression_return:
    rt_managed_arena_destroy_child(__local_arena__);
    return _return_value;
}

long long __sn__zlibA__defaultCompression(RtManagedArena *__caller_arena__) {
    RtManagedArena *__local_arena__ = rt_managed_arena_create_child(__caller_arena__);
    long long _return_value = 0;
    _return_value = -1LL;
    goto __sn__zlibA__defaultCompression_return;
__sn__zlibA__defaultCompression_return:
    rt_managed_arena_destroy_child(__local_arena__);
    return _return_value;
}

RtHandle __sn__zlibA__errorMessage(RtManagedArena *__caller_arena__, long long __sn__code) {
    RtManagedArena *__local_arena__ = rt_managed_arena_create_child(__caller_arena__);
    RtHandle _return_value = RT_HANDLE_NULL;
    if (rt_eq_long(__sn__code, ({
    long long __intercept_result;
    if (__rt_interceptor_count > 0) {
        RtAny __args[1];
        __rt_thunk_args = __args;
        __rt_thunk_arena = __local_arena__;
        RtAny __intercepted = rt_call_intercepted("zlibOk", __args, 0, __thunk_0);
        __intercept_result = rt_unbox_int(__intercepted);
    } else {
        __intercept_result = __sn__zlibA__zlibOk(__local_arena__);
    }
    __intercept_result;
}))) {
        {
            _return_value = rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "OK");
            goto __sn__zlibA__errorMessage_return;
        }
    }
    if (rt_eq_long(__sn__code, ({
    long long __intercept_result;
    if (__rt_interceptor_count > 0) {
        RtAny __args[1];
        __rt_thunk_args = __args;
        __rt_thunk_arena = __local_arena__;
        RtAny __intercepted = rt_call_intercepted("zlibStreamEnd", __args, 0, __thunk_1);
        __intercept_result = rt_unbox_int(__intercepted);
    } else {
        __intercept_result = __sn__zlibA__zlibStreamEnd(__local_arena__);
    }
    __intercept_result;
}))) {
        {
            _return_value = rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "Stream end");
            goto __sn__zlibA__errorMessage_return;
        }
    }
    if (rt_eq_long(__sn__code, ({
    long long __intercept_result;
    if (__rt_interceptor_count > 0) {
        RtAny __args[1];
        __rt_thunk_args = __args;
        __rt_thunk_arena = __local_arena__;
        RtAny __intercepted = rt_call_intercepted("zlibNeedDict", __args, 0, __thunk_2);
        __intercept_result = rt_unbox_int(__intercepted);
    } else {
        __intercept_result = __sn__zlibA__zlibNeedDict(__local_arena__);
    }
    __intercept_result;
}))) {
        {
            _return_value = rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "Need dictionary");
            goto __sn__zlibA__errorMessage_return;
        }
    }
    if (rt_eq_long(__sn__code, ({
    long long __intercept_result;
    if (__rt_interceptor_count > 0) {
        RtAny __args[1];
        __rt_thunk_args = __args;
        __rt_thunk_arena = __local_arena__;
        RtAny __intercepted = rt_call_intercepted("zlibErrno", __args, 0, __thunk_3);
        __intercept_result = rt_unbox_int(__intercepted);
    } else {
        __intercept_result = __sn__zlibA__zlibErrno(__local_arena__);
    }
    __intercept_result;
}))) {
        {
            _return_value = rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "File error");
            goto __sn__zlibA__errorMessage_return;
        }
    }
    if (rt_eq_long(__sn__code, ({
    long long __intercept_result;
    if (__rt_interceptor_count > 0) {
        RtAny __args[1];
        __rt_thunk_args = __args;
        __rt_thunk_arena = __local_arena__;
        RtAny __intercepted = rt_call_intercepted("zlibStreamError", __args, 0, __thunk_4);
        __intercept_result = rt_unbox_int(__intercepted);
    } else {
        __intercept_result = __sn__zlibA__zlibStreamError(__local_arena__);
    }
    __intercept_result;
}))) {
        {
            _return_value = rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "Stream error");
            goto __sn__zlibA__errorMessage_return;
        }
    }
    if (rt_eq_long(__sn__code, ({
    long long __intercept_result;
    if (__rt_interceptor_count > 0) {
        RtAny __args[1];
        __rt_thunk_args = __args;
        __rt_thunk_arena = __local_arena__;
        RtAny __intercepted = rt_call_intercepted("zlibDataError", __args, 0, __thunk_5);
        __intercept_result = rt_unbox_int(__intercepted);
    } else {
        __intercept_result = __sn__zlibA__zlibDataError(__local_arena__);
    }
    __intercept_result;
}))) {
        {
            _return_value = rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "Data error (corrupted or incomplete)");
            goto __sn__zlibA__errorMessage_return;
        }
    }
    if (rt_eq_long(__sn__code, ({
    long long __intercept_result;
    if (__rt_interceptor_count > 0) {
        RtAny __args[1];
        __rt_thunk_args = __args;
        __rt_thunk_arena = __local_arena__;
        RtAny __intercepted = rt_call_intercepted("zlibMemError", __args, 0, __thunk_6);
        __intercept_result = rt_unbox_int(__intercepted);
    } else {
        __intercept_result = __sn__zlibA__zlibMemError(__local_arena__);
    }
    __intercept_result;
}))) {
        {
            _return_value = rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "Memory error");
            goto __sn__zlibA__errorMessage_return;
        }
    }
    if (rt_eq_long(__sn__code, ({
    long long __intercept_result;
    if (__rt_interceptor_count > 0) {
        RtAny __args[1];
        __rt_thunk_args = __args;
        __rt_thunk_arena = __local_arena__;
        RtAny __intercepted = rt_call_intercepted("zlibBufError", __args, 0, __thunk_7);
        __intercept_result = rt_unbox_int(__intercepted);
    } else {
        __intercept_result = __sn__zlibA__zlibBufError(__local_arena__);
    }
    __intercept_result;
}))) {
        {
            _return_value = rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "Buffer error (output too small)");
            goto __sn__zlibA__errorMessage_return;
        }
    }
    if (rt_eq_long(__sn__code, ({
    long long __intercept_result;
    if (__rt_interceptor_count > 0) {
        RtAny __args[1];
        __rt_thunk_args = __args;
        __rt_thunk_arena = __local_arena__;
        RtAny __intercepted = rt_call_intercepted("zlibVersionError", __args, 0, __thunk_8);
        __intercept_result = rt_unbox_int(__intercepted);
    } else {
        __intercept_result = __sn__zlibA__zlibVersionError(__local_arena__);
    }
    __intercept_result;
}))) {
        {
            _return_value = rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "Version error");
            goto __sn__zlibA__errorMessage_return;
        }
    }
    _return_value = rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "Unknown error");
    goto __sn__zlibA__errorMessage_return;
__sn__zlibA__errorMessage_return:
    _return_value = rt_managed_promote(__caller_arena__, __local_arena__, _return_value);
    rt_managed_arena_destroy_child(__local_arena__);
    return _return_value;
}

long long compressTo(RtManagedArena *__caller_arena__, RtHandle __sn__source, RtHandle __sn__dest) {
    RtManagedArena *__local_arena__ = rt_managed_arena_create_child(__caller_arena__);
    long long _return_value = 0;
    uint64_t __sn__destLen = ((uint64_t)(rt_array_length(((unsigned char *)rt_managed_pin_array(__local_arena__, __sn__dest)))));
    RtThread *__result_pending__ = NULL;
    long long __sn__result = compress(((unsigned char *)rt_managed_pin_array(__local_arena__, __sn__dest)), &__sn__destLen, ((unsigned char *)rt_managed_pin_array(__local_arena__, __sn__source)), ((uint64_t)(rt_array_length(((unsigned char *)rt_managed_pin_array(__local_arena__, __sn__source))))));
    if (rt_ne_long(__sn__result, 0LL)) {
        {
            _return_value = -1LL;
            goto compressTo_return;
        }
    }
    _return_value = ((long long)(__sn__destLen));
    goto compressTo_return;
compressTo_return:
    rt_managed_arena_destroy_child(__local_arena__);
    return _return_value;
}

long long compressToLevel(RtManagedArena *__caller_arena__, RtHandle __sn__source, RtHandle __sn__dest, long long __sn__level) {
    RtManagedArena *__local_arena__ = rt_managed_arena_create_child(__caller_arena__);
    long long _return_value = 0;
    uint64_t __sn__destLen = ((uint64_t)(rt_array_length(((unsigned char *)rt_managed_pin_array(__local_arena__, __sn__dest)))));
    RtThread *__result_pending__ = NULL;
    long long __sn__result = compress2(((unsigned char *)rt_managed_pin_array(__local_arena__, __sn__dest)), &__sn__destLen, ((unsigned char *)rt_managed_pin_array(__local_arena__, __sn__source)), ((uint64_t)(rt_array_length(((unsigned char *)rt_managed_pin_array(__local_arena__, __sn__source))))), __sn__level);
    if (rt_ne_long(__sn__result, 0LL)) {
        {
            _return_value = -1LL;
            goto compressToLevel_return;
        }
    }
    _return_value = ((long long)(__sn__destLen));
    goto compressToLevel_return;
compressToLevel_return:
    rt_managed_arena_destroy_child(__local_arena__);
    return _return_value;
}

long long decompressTo(RtManagedArena *__caller_arena__, RtHandle __sn__source, RtHandle __sn__dest) {
    RtManagedArena *__local_arena__ = rt_managed_arena_create_child(__caller_arena__);
    long long _return_value = 0;
    uint64_t __sn__destLen = ((uint64_t)(rt_array_length(((unsigned char *)rt_managed_pin_array(__local_arena__, __sn__dest)))));
    RtThread *__result_pending__ = NULL;
    long long __sn__result = uncompress(((unsigned char *)rt_managed_pin_array(__local_arena__, __sn__dest)), &__sn__destLen, ((unsigned char *)rt_managed_pin_array(__local_arena__, __sn__source)), ((uint64_t)(rt_array_length(((unsigned char *)rt_managed_pin_array(__local_arena__, __sn__source))))));
    if (rt_ne_long(__sn__result, 0LL)) {
        {
            _return_value = -1LL;
            goto decompressTo_return;
        }
    }
    _return_value = ((long long)(__sn__destLen));
    goto decompressTo_return;
decompressTo_return:
    rt_managed_arena_destroy_child(__local_arena__);
    return _return_value;
}

long long __sn__zlibA__maxCompressedSize(RtManagedArena *__caller_arena__, long long __sn__sourceLen) {
    RtManagedArena *__local_arena__ = rt_managed_arena_create_child(__caller_arena__);
    long long _return_value = 0;
    _return_value = ((long long)(compressBound(((uint64_t)(__sn__sourceLen)))));
    goto __sn__zlibA__maxCompressedSize_return;
__sn__zlibA__maxCompressedSize_return:
    rt_managed_arena_destroy_child(__local_arena__);
    return _return_value;
}

RtHandle compressData(RtManagedArena *__caller_arena__, RtHandle __sn__source) {
    RtManagedArena *__local_arena__ = rt_managed_arena_create_child(__caller_arena__);
    RtHandle _return_value = RT_HANDLE_NULL;
    if (rt_eq_long(rt_array_length(((unsigned char *)rt_managed_pin_array(__local_arena__, __sn__source))), 0LL)) {
        {
            _return_value = __sn__source;
            goto compressData_return;
        }
    }
    RtThread *__bound_pending__ = NULL;
    long long __sn__bound = ({
    long long __intercept_result;
    if (__rt_interceptor_count > 0) {
        RtAny __args[1];
        __args[0] = rt_box_int(rt_array_length(((unsigned char *)rt_managed_pin_array(__local_arena__, __sn__source))));
        __rt_thunk_args = __args;
        __rt_thunk_arena = __local_arena__;
        RtAny __intercepted = rt_call_intercepted("maxCompressedSize", __args, 1, __thunk_9);
        __intercept_result = rt_unbox_int(__intercepted);
    } else {
        __intercept_result = __sn__zlibA__maxCompressedSize(__local_arena__, rt_array_length(((unsigned char *)rt_managed_pin_array(__local_arena__, __sn__source))));
    }
    __intercept_result;
});
    RtThread *__dest_pending__ = NULL;
    RtHandle __sn__dest = rt_array_alloc_byte_h(__local_arena__, __sn__bound, 0);
    uint64_t __sn__destLen = ((uint64_t)(__sn__bound));
    RtThread *__result_pending__ = NULL;
    long long __sn__result = compress(((unsigned char *)rt_managed_pin_array(__local_arena__, __sn__dest)), &__sn__destLen, ((unsigned char *)rt_managed_pin_array(__local_arena__, __sn__source)), ((uint64_t)(rt_array_length(((unsigned char *)rt_managed_pin_array(__local_arena__, __sn__source))))));
    if (rt_ne_long(__sn__result, 0LL)) {
        {
            RtThread *__empty_pending__ = NULL;
            RtHandle __sn__empty = rt_array_alloc_byte_h(__local_arena__, 0LL, 0);
            _return_value = __sn__empty;
            goto compressData_return;
        }
    }
    _return_value = rt_array_slice_byte_h(__local_arena__, ((unsigned char *)rt_managed_pin_array(__local_arena__, __sn__dest)), 0LL, ((long long)(__sn__destLen)), LONG_MIN);
    goto compressData_return;
compressData_return:
    _return_value = rt_managed_promote(__caller_arena__, __local_arena__, _return_value);
    rt_managed_arena_destroy_child(__local_arena__);
    return _return_value;
}

RtHandle compressDataLevel(RtManagedArena *__caller_arena__, RtHandle __sn__source, long long __sn__level) {
    RtManagedArena *__local_arena__ = rt_managed_arena_create_child(__caller_arena__);
    RtHandle _return_value = RT_HANDLE_NULL;
    if (rt_eq_long(rt_array_length(((unsigned char *)rt_managed_pin_array(__local_arena__, __sn__source))), 0LL)) {
        {
            _return_value = __sn__source;
            goto compressDataLevel_return;
        }
    }
    RtThread *__bound_pending__ = NULL;
    long long __sn__bound = ({
    long long __intercept_result;
    if (__rt_interceptor_count > 0) {
        RtAny __args[1];
        __args[0] = rt_box_int(rt_array_length(((unsigned char *)rt_managed_pin_array(__local_arena__, __sn__source))));
        __rt_thunk_args = __args;
        __rt_thunk_arena = __local_arena__;
        RtAny __intercepted = rt_call_intercepted("maxCompressedSize", __args, 1, __thunk_10);
        __intercept_result = rt_unbox_int(__intercepted);
    } else {
        __intercept_result = __sn__zlibA__maxCompressedSize(__local_arena__, rt_array_length(((unsigned char *)rt_managed_pin_array(__local_arena__, __sn__source))));
    }
    __intercept_result;
});
    RtThread *__dest_pending__ = NULL;
    RtHandle __sn__dest = rt_array_alloc_byte_h(__local_arena__, __sn__bound, 0);
    uint64_t __sn__destLen = ((uint64_t)(__sn__bound));
    RtThread *__result_pending__ = NULL;
    long long __sn__result = compress2(((unsigned char *)rt_managed_pin_array(__local_arena__, __sn__dest)), &__sn__destLen, ((unsigned char *)rt_managed_pin_array(__local_arena__, __sn__source)), ((uint64_t)(rt_array_length(((unsigned char *)rt_managed_pin_array(__local_arena__, __sn__source))))), __sn__level);
    if (rt_ne_long(__sn__result, 0LL)) {
        {
            RtThread *__empty_pending__ = NULL;
            RtHandle __sn__empty = rt_array_alloc_byte_h(__local_arena__, 0LL, 0);
            _return_value = __sn__empty;
            goto compressDataLevel_return;
        }
    }
    _return_value = rt_array_slice_byte_h(__local_arena__, ((unsigned char *)rt_managed_pin_array(__local_arena__, __sn__dest)), 0LL, ((long long)(__sn__destLen)), LONG_MIN);
    goto compressDataLevel_return;
compressDataLevel_return:
    _return_value = rt_managed_promote(__caller_arena__, __local_arena__, _return_value);
    rt_managed_arena_destroy_child(__local_arena__);
    return _return_value;
}

RtHandle decompressData(RtManagedArena *__caller_arena__, RtHandle __sn__source, long long __sn__expectedSize) {
    RtManagedArena *__local_arena__ = rt_managed_arena_create_child(__caller_arena__);
    RtHandle _return_value = RT_HANDLE_NULL;
    if (rt_eq_long(rt_array_length(((unsigned char *)rt_managed_pin_array(__local_arena__, __sn__source))), 0LL)) {
        {
            _return_value = __sn__source;
            goto decompressData_return;
        }
    }
    RtThread *__dest_pending__ = NULL;
    RtHandle __sn__dest = rt_array_alloc_byte_h(__local_arena__, __sn__expectedSize, 0);
    uint64_t __sn__destLen = ((uint64_t)(__sn__expectedSize));
    RtThread *__result_pending__ = NULL;
    long long __sn__result = uncompress(((unsigned char *)rt_managed_pin_array(__local_arena__, __sn__dest)), &__sn__destLen, ((unsigned char *)rt_managed_pin_array(__local_arena__, __sn__source)), ((uint64_t)(rt_array_length(((unsigned char *)rt_managed_pin_array(__local_arena__, __sn__source))))));
    if (rt_ne_long(__sn__result, 0LL)) {
        {
            RtThread *__empty_pending__ = NULL;
            RtHandle __sn__empty = rt_array_alloc_byte_h(__local_arena__, 0LL, 0);
            _return_value = __sn__empty;
            goto decompressData_return;
        }
    }
    _return_value = rt_array_slice_byte_h(__local_arena__, ((unsigned char *)rt_managed_pin_array(__local_arena__, __sn__dest)), 0LL, ((long long)(__sn__destLen)), LONG_MIN);
    goto decompressData_return;
decompressData_return:
    _return_value = rt_managed_promote(__caller_arena__, __local_arena__, _return_value);
    rt_managed_arena_destroy_child(__local_arena__);
    return _return_value;
}

bool __sn__zlibA__isCompressed(RtManagedArena *__caller_arena__, RtHandle __sn__data) {
    RtManagedArena *__local_arena__ = rt_managed_arena_create_child(__caller_arena__);
    bool _return_value = 0;
    if (rt_lt_long(rt_array_length(((unsigned char *)rt_managed_pin_array(__local_arena__, __sn__data))), 2LL)) {
        {
            _return_value = 0L;
            goto __sn__zlibA__isCompressed_return;
        }
    }
    RtThread *__cmf_pending__ = NULL;
    long long __sn__cmf = ((unsigned char *)rt_managed_pin_array(__local_arena__, __sn__data))[0LL];
    RtThread *__flg_pending__ = NULL;
    long long __sn__flg = ((unsigned char *)rt_managed_pin_array(__local_arena__, __sn__data))[1LL];
    if (rt_ne_long(__sn__cmf, 120LL)) {
        {
            _return_value = 0L;
            goto __sn__zlibA__isCompressed_return;
        }
    }
    RtThread *__check_pending__ = NULL;
    long long __sn__check = rt_add_long(rt_mul_long(__sn__cmf, 256LL), __sn__flg);
    _return_value = rt_eq_long(rt_mod_long(__sn__check, 31LL), 0LL);
    goto __sn__zlibA__isCompressed_return;
__sn__zlibA__isCompressed_return:
    rt_managed_arena_destroy_child(__local_arena__);
    return _return_value;
}

double __sn__zlibA__compressionRatio(RtManagedArena *__caller_arena__, long long __sn__originalSize, long long __sn__compressedSize) {
    RtManagedArena *__local_arena__ = rt_managed_arena_create_child(__caller_arena__);
    double _return_value = 0;
    if (rt_eq_long(__sn__originalSize, 0LL)) {
        {
            _return_value = 0.0;
            goto __sn__zlibA__compressionRatio_return;
        }
    }
    _return_value = rt_div_double(rt_mul_double(((double)(__sn__compressedSize)), 100.0), ((double)(__sn__originalSize)));
    goto __sn__zlibA__compressionRatio_return;
__sn__zlibA__compressionRatio_return:
    rt_managed_arena_destroy_child(__local_arena__);
    return _return_value;
}

double __sn__zlibA__spaceSaved(RtManagedArena *__caller_arena__, long long __sn__originalSize, long long __sn__compressedSize) {
    RtManagedArena *__local_arena__ = rt_managed_arena_create_child(__caller_arena__);
    double _return_value = 0;
    _return_value = rt_sub_double(100.0, ({
    double __intercept_result;
    if (__rt_interceptor_count > 0) {
        RtAny __args[2];
        __args[0] = rt_box_int(__sn__originalSize);
        __args[1] = rt_box_int(__sn__compressedSize);
        __rt_thunk_args = __args;
        __rt_thunk_arena = __local_arena__;
        RtAny __intercepted = rt_call_intercepted("compressionRatio", __args, 2, __thunk_11);
        __intercept_result = rt_unbox_double(__intercepted);
    } else {
        __intercept_result = __sn__zlibA__compressionRatio(__local_arena__, __sn__originalSize, __sn__compressedSize);
    }
    __intercept_result;
}));
    goto __sn__zlibA__spaceSaved_return;
__sn__zlibA__spaceSaved_return:
    rt_managed_arena_destroy_child(__local_arena__);
    return _return_value;
}

long long __sn__moduleB__zlibB__zlibOk(RtManagedArena *);
long long __sn__moduleB__zlibB__zlibStreamEnd(RtManagedArena *);
long long __sn__moduleB__zlibB__zlibNeedDict(RtManagedArena *);
long long __sn__moduleB__zlibB__zlibErrno(RtManagedArena *);
long long __sn__moduleB__zlibB__zlibStreamError(RtManagedArena *);
long long __sn__moduleB__zlibB__zlibDataError(RtManagedArena *);
long long __sn__moduleB__zlibB__zlibMemError(RtManagedArena *);
long long __sn__moduleB__zlibB__zlibBufError(RtManagedArena *);
long long __sn__moduleB__zlibB__zlibVersionError(RtManagedArena *);
long long __sn__moduleB__zlibB__noCompression(RtManagedArena *);
long long __sn__moduleB__zlibB__bestSpeed(RtManagedArena *);
long long __sn__moduleB__zlibB__bestCompression(RtManagedArena *);
long long __sn__moduleB__zlibB__defaultCompression(RtManagedArena *);
RtHandle __sn__moduleB__zlibB__errorMessage(RtManagedArena *, long long);
long long __sn__moduleB__zlibB__compressTo(RtManagedArena *, RtHandle, RtHandle);
long long __sn__moduleB__zlibB__compressToLevel(RtManagedArena *, RtHandle, RtHandle, long long);
long long __sn__moduleB__zlibB__decompressTo(RtManagedArena *, RtHandle, RtHandle);
long long __sn__moduleB__zlibB__maxCompressedSize(RtManagedArena *, long long);
RtHandle __sn__moduleB__zlibB__compressData(RtManagedArena *, RtHandle);
RtHandle __sn__moduleB__zlibB__compressDataLevel(RtManagedArena *, RtHandle, long long);
RtHandle __sn__moduleB__zlibB__decompressData(RtManagedArena *, RtHandle, long long);
bool __sn__moduleB__zlibB__isCompressed(RtManagedArena *, RtHandle);
double __sn__moduleB__zlibB__compressionRatio(RtManagedArena *, long long, long long);
double __sn__moduleB__zlibB__spaceSaved(RtManagedArena *, long long, long long);
void __sn__moduleB__test_basic_compress_decompress_B(RtManagedArena *);
// Test SDK compression module
long long __sn__zlibB__zlibOk(RtManagedArena *);
long long __sn__zlibB__zlibStreamEnd(RtManagedArena *);
long long __sn__zlibB__zlibNeedDict(RtManagedArena *);
long long __sn__zlibB__zlibErrno(RtManagedArena *);
long long __sn__zlibB__zlibStreamError(RtManagedArena *);
long long __sn__zlibB__zlibDataError(RtManagedArena *);
long long __sn__zlibB__zlibMemError(RtManagedArena *);
long long __sn__zlibB__zlibBufError(RtManagedArena *);
long long __sn__zlibB__zlibVersionError(RtManagedArena *);
long long __sn__zlibB__noCompression(RtManagedArena *);
long long __sn__zlibB__bestSpeed(RtManagedArena *);
long long __sn__zlibB__bestCompression(RtManagedArena *);
long long __sn__zlibB__defaultCompression(RtManagedArena *);
RtHandle __sn__zlibB__errorMessage(RtManagedArena *, long long);
long long __sn__zlibB__compressTo(RtManagedArena *, RtHandle, RtHandle);
long long __sn__zlibB__compressToLevel(RtManagedArena *, RtHandle, RtHandle, long long);
long long __sn__zlibB__decompressTo(RtManagedArena *, RtHandle, RtHandle);
long long __sn__zlibB__maxCompressedSize(RtManagedArena *, long long);
RtHandle __sn__zlibB__compressData(RtManagedArena *, RtHandle);
RtHandle __sn__zlibB__compressDataLevel(RtManagedArena *, RtHandle, long long);
RtHandle __sn__zlibB__decompressData(RtManagedArena *, RtHandle, long long);
bool __sn__zlibB__isCompressed(RtManagedArena *, RtHandle);
double __sn__zlibB__compressionRatio(RtManagedArena *, long long, long long);
double __sn__zlibB__spaceSaved(RtManagedArena *, long long, long long);
long long __sn__zlibB__zlibOk(RtManagedArena *__caller_arena__) {
    RtManagedArena *__local_arena__ = rt_managed_arena_create_child(__caller_arena__);
    long long _return_value = 0;
    _return_value = 0LL;
    goto __sn__zlibB__zlibOk_return;
__sn__zlibB__zlibOk_return:
    rt_managed_arena_destroy_child(__local_arena__);
    return _return_value;
}

long long __sn__zlibB__zlibStreamEnd(RtManagedArena *__caller_arena__) {
    RtManagedArena *__local_arena__ = rt_managed_arena_create_child(__caller_arena__);
    long long _return_value = 0;
    _return_value = 1LL;
    goto __sn__zlibB__zlibStreamEnd_return;
__sn__zlibB__zlibStreamEnd_return:
    rt_managed_arena_destroy_child(__local_arena__);
    return _return_value;
}

long long __sn__zlibB__zlibNeedDict(RtManagedArena *__caller_arena__) {
    RtManagedArena *__local_arena__ = rt_managed_arena_create_child(__caller_arena__);
    long long _return_value = 0;
    _return_value = 2LL;
    goto __sn__zlibB__zlibNeedDict_return;
__sn__zlibB__zlibNeedDict_return:
    rt_managed_arena_destroy_child(__local_arena__);
    return _return_value;
}

long long __sn__zlibB__zlibErrno(RtManagedArena *__caller_arena__) {
    RtManagedArena *__local_arena__ = rt_managed_arena_create_child(__caller_arena__);
    long long _return_value = 0;
    _return_value = -1LL;
    goto __sn__zlibB__zlibErrno_return;
__sn__zlibB__zlibErrno_return:
    rt_managed_arena_destroy_child(__local_arena__);
    return _return_value;
}

long long __sn__zlibB__zlibStreamError(RtManagedArena *__caller_arena__) {
    RtManagedArena *__local_arena__ = rt_managed_arena_create_child(__caller_arena__);
    long long _return_value = 0;
    _return_value = -2LL;
    goto __sn__zlibB__zlibStreamError_return;
__sn__zlibB__zlibStreamError_return:
    rt_managed_arena_destroy_child(__local_arena__);
    return _return_value;
}

long long __sn__zlibB__zlibDataError(RtManagedArena *__caller_arena__) {
    RtManagedArena *__local_arena__ = rt_managed_arena_create_child(__caller_arena__);
    long long _return_value = 0;
    _return_value = -3LL;
    goto __sn__zlibB__zlibDataError_return;
__sn__zlibB__zlibDataError_return:
    rt_managed_arena_destroy_child(__local_arena__);
    return _return_value;
}

long long __sn__zlibB__zlibMemError(RtManagedArena *__caller_arena__) {
    RtManagedArena *__local_arena__ = rt_managed_arena_create_child(__caller_arena__);
    long long _return_value = 0;
    _return_value = -4LL;
    goto __sn__zlibB__zlibMemError_return;
__sn__zlibB__zlibMemError_return:
    rt_managed_arena_destroy_child(__local_arena__);
    return _return_value;
}

long long __sn__zlibB__zlibBufError(RtManagedArena *__caller_arena__) {
    RtManagedArena *__local_arena__ = rt_managed_arena_create_child(__caller_arena__);
    long long _return_value = 0;
    _return_value = -5LL;
    goto __sn__zlibB__zlibBufError_return;
__sn__zlibB__zlibBufError_return:
    rt_managed_arena_destroy_child(__local_arena__);
    return _return_value;
}

long long __sn__zlibB__zlibVersionError(RtManagedArena *__caller_arena__) {
    RtManagedArena *__local_arena__ = rt_managed_arena_create_child(__caller_arena__);
    long long _return_value = 0;
    _return_value = -6LL;
    goto __sn__zlibB__zlibVersionError_return;
__sn__zlibB__zlibVersionError_return:
    rt_managed_arena_destroy_child(__local_arena__);
    return _return_value;
}

long long __sn__zlibB__noCompression(RtManagedArena *__caller_arena__) {
    RtManagedArena *__local_arena__ = rt_managed_arena_create_child(__caller_arena__);
    long long _return_value = 0;
    _return_value = 0LL;
    goto __sn__zlibB__noCompression_return;
__sn__zlibB__noCompression_return:
    rt_managed_arena_destroy_child(__local_arena__);
    return _return_value;
}

long long __sn__zlibB__bestSpeed(RtManagedArena *__caller_arena__) {
    RtManagedArena *__local_arena__ = rt_managed_arena_create_child(__caller_arena__);
    long long _return_value = 0;
    _return_value = 1LL;
    goto __sn__zlibB__bestSpeed_return;
__sn__zlibB__bestSpeed_return:
    rt_managed_arena_destroy_child(__local_arena__);
    return _return_value;
}

long long __sn__zlibB__bestCompression(RtManagedArena *__caller_arena__) {
    RtManagedArena *__local_arena__ = rt_managed_arena_create_child(__caller_arena__);
    long long _return_value = 0;
    _return_value = 9LL;
    goto __sn__zlibB__bestCompression_return;
__sn__zlibB__bestCompression_return:
    rt_managed_arena_destroy_child(__local_arena__);
    return _return_value;
}

long long __sn__zlibB__defaultCompression(RtManagedArena *__caller_arena__) {
    RtManagedArena *__local_arena__ = rt_managed_arena_create_child(__caller_arena__);
    long long _return_value = 0;
    _return_value = -1LL;
    goto __sn__zlibB__defaultCompression_return;
__sn__zlibB__defaultCompression_return:
    rt_managed_arena_destroy_child(__local_arena__);
    return _return_value;
}

RtHandle __sn__zlibB__errorMessage(RtManagedArena *__caller_arena__, long long __sn__code) {
    RtManagedArena *__local_arena__ = rt_managed_arena_create_child(__caller_arena__);
    RtHandle _return_value = RT_HANDLE_NULL;
    if (rt_eq_long(__sn__code, ({
    long long __intercept_result;
    if (__rt_interceptor_count > 0) {
        RtAny __args[1];
        __rt_thunk_args = __args;
        __rt_thunk_arena = __local_arena__;
        RtAny __intercepted = rt_call_intercepted("zlibOk", __args, 0, __thunk_12);
        __intercept_result = rt_unbox_int(__intercepted);
    } else {
        __intercept_result = __sn__zlibB__zlibOk(__local_arena__);
    }
    __intercept_result;
}))) {
        {
            _return_value = rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "OK");
            goto __sn__zlibB__errorMessage_return;
        }
    }
    if (rt_eq_long(__sn__code, ({
    long long __intercept_result;
    if (__rt_interceptor_count > 0) {
        RtAny __args[1];
        __rt_thunk_args = __args;
        __rt_thunk_arena = __local_arena__;
        RtAny __intercepted = rt_call_intercepted("zlibStreamEnd", __args, 0, __thunk_13);
        __intercept_result = rt_unbox_int(__intercepted);
    } else {
        __intercept_result = __sn__zlibB__zlibStreamEnd(__local_arena__);
    }
    __intercept_result;
}))) {
        {
            _return_value = rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "Stream end");
            goto __sn__zlibB__errorMessage_return;
        }
    }
    if (rt_eq_long(__sn__code, ({
    long long __intercept_result;
    if (__rt_interceptor_count > 0) {
        RtAny __args[1];
        __rt_thunk_args = __args;
        __rt_thunk_arena = __local_arena__;
        RtAny __intercepted = rt_call_intercepted("zlibNeedDict", __args, 0, __thunk_14);
        __intercept_result = rt_unbox_int(__intercepted);
    } else {
        __intercept_result = __sn__zlibB__zlibNeedDict(__local_arena__);
    }
    __intercept_result;
}))) {
        {
            _return_value = rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "Need dictionary");
            goto __sn__zlibB__errorMessage_return;
        }
    }
    if (rt_eq_long(__sn__code, ({
    long long __intercept_result;
    if (__rt_interceptor_count > 0) {
        RtAny __args[1];
        __rt_thunk_args = __args;
        __rt_thunk_arena = __local_arena__;
        RtAny __intercepted = rt_call_intercepted("zlibErrno", __args, 0, __thunk_15);
        __intercept_result = rt_unbox_int(__intercepted);
    } else {
        __intercept_result = __sn__zlibB__zlibErrno(__local_arena__);
    }
    __intercept_result;
}))) {
        {
            _return_value = rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "File error");
            goto __sn__zlibB__errorMessage_return;
        }
    }
    if (rt_eq_long(__sn__code, ({
    long long __intercept_result;
    if (__rt_interceptor_count > 0) {
        RtAny __args[1];
        __rt_thunk_args = __args;
        __rt_thunk_arena = __local_arena__;
        RtAny __intercepted = rt_call_intercepted("zlibStreamError", __args, 0, __thunk_16);
        __intercept_result = rt_unbox_int(__intercepted);
    } else {
        __intercept_result = __sn__zlibB__zlibStreamError(__local_arena__);
    }
    __intercept_result;
}))) {
        {
            _return_value = rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "Stream error");
            goto __sn__zlibB__errorMessage_return;
        }
    }
    if (rt_eq_long(__sn__code, ({
    long long __intercept_result;
    if (__rt_interceptor_count > 0) {
        RtAny __args[1];
        __rt_thunk_args = __args;
        __rt_thunk_arena = __local_arena__;
        RtAny __intercepted = rt_call_intercepted("zlibDataError", __args, 0, __thunk_17);
        __intercept_result = rt_unbox_int(__intercepted);
    } else {
        __intercept_result = __sn__zlibB__zlibDataError(__local_arena__);
    }
    __intercept_result;
}))) {
        {
            _return_value = rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "Data error (corrupted or incomplete)");
            goto __sn__zlibB__errorMessage_return;
        }
    }
    if (rt_eq_long(__sn__code, ({
    long long __intercept_result;
    if (__rt_interceptor_count > 0) {
        RtAny __args[1];
        __rt_thunk_args = __args;
        __rt_thunk_arena = __local_arena__;
        RtAny __intercepted = rt_call_intercepted("zlibMemError", __args, 0, __thunk_18);
        __intercept_result = rt_unbox_int(__intercepted);
    } else {
        __intercept_result = __sn__zlibB__zlibMemError(__local_arena__);
    }
    __intercept_result;
}))) {
        {
            _return_value = rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "Memory error");
            goto __sn__zlibB__errorMessage_return;
        }
    }
    if (rt_eq_long(__sn__code, ({
    long long __intercept_result;
    if (__rt_interceptor_count > 0) {
        RtAny __args[1];
        __rt_thunk_args = __args;
        __rt_thunk_arena = __local_arena__;
        RtAny __intercepted = rt_call_intercepted("zlibBufError", __args, 0, __thunk_19);
        __intercept_result = rt_unbox_int(__intercepted);
    } else {
        __intercept_result = __sn__zlibB__zlibBufError(__local_arena__);
    }
    __intercept_result;
}))) {
        {
            _return_value = rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "Buffer error (output too small)");
            goto __sn__zlibB__errorMessage_return;
        }
    }
    if (rt_eq_long(__sn__code, ({
    long long __intercept_result;
    if (__rt_interceptor_count > 0) {
        RtAny __args[1];
        __rt_thunk_args = __args;
        __rt_thunk_arena = __local_arena__;
        RtAny __intercepted = rt_call_intercepted("zlibVersionError", __args, 0, __thunk_20);
        __intercept_result = rt_unbox_int(__intercepted);
    } else {
        __intercept_result = __sn__zlibB__zlibVersionError(__local_arena__);
    }
    __intercept_result;
}))) {
        {
            _return_value = rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "Version error");
            goto __sn__zlibB__errorMessage_return;
        }
    }
    _return_value = rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "Unknown error");
    goto __sn__zlibB__errorMessage_return;
__sn__zlibB__errorMessage_return:
    _return_value = rt_managed_promote(__caller_arena__, __local_arena__, _return_value);
    rt_managed_arena_destroy_child(__local_arena__);
    return _return_value;
}

long long __sn__zlibB__maxCompressedSize(RtManagedArena *__caller_arena__, long long __sn__sourceLen) {
    RtManagedArena *__local_arena__ = rt_managed_arena_create_child(__caller_arena__);
    long long _return_value = 0;
    _return_value = ((long long)(compressBound(((uint64_t)(__sn__sourceLen)))));
    goto __sn__zlibB__maxCompressedSize_return;
__sn__zlibB__maxCompressedSize_return:
    rt_managed_arena_destroy_child(__local_arena__);
    return _return_value;
}

bool __sn__zlibB__isCompressed(RtManagedArena *__caller_arena__, RtHandle __sn__data) {
    RtManagedArena *__local_arena__ = rt_managed_arena_create_child(__caller_arena__);
    bool _return_value = 0;
    if (rt_lt_long(rt_array_length(((unsigned char *)rt_managed_pin_array(__local_arena__, __sn__data))), 2LL)) {
        {
            _return_value = 0L;
            goto __sn__zlibB__isCompressed_return;
        }
    }
    RtThread *__cmf_pending__ = NULL;
    long long __sn__cmf = ((unsigned char *)rt_managed_pin_array(__local_arena__, __sn__data))[0LL];
    RtThread *__flg_pending__ = NULL;
    long long __sn__flg = ((unsigned char *)rt_managed_pin_array(__local_arena__, __sn__data))[1LL];
    if (rt_ne_long(__sn__cmf, 120LL)) {
        {
            _return_value = 0L;
            goto __sn__zlibB__isCompressed_return;
        }
    }
    RtThread *__check_pending__ = NULL;
    long long __sn__check = rt_add_long(rt_mul_long(__sn__cmf, 256LL), __sn__flg);
    _return_value = rt_eq_long(rt_mod_long(__sn__check, 31LL), 0LL);
    goto __sn__zlibB__isCompressed_return;
__sn__zlibB__isCompressed_return:
    rt_managed_arena_destroy_child(__local_arena__);
    return _return_value;
}

double __sn__zlibB__compressionRatio(RtManagedArena *__caller_arena__, long long __sn__originalSize, long long __sn__compressedSize) {
    RtManagedArena *__local_arena__ = rt_managed_arena_create_child(__caller_arena__);
    double _return_value = 0;
    if (rt_eq_long(__sn__originalSize, 0LL)) {
        {
            _return_value = 0.0;
            goto __sn__zlibB__compressionRatio_return;
        }
    }
    _return_value = rt_div_double(rt_mul_double(((double)(__sn__compressedSize)), 100.0), ((double)(__sn__originalSize)));
    goto __sn__zlibB__compressionRatio_return;
__sn__zlibB__compressionRatio_return:
    rt_managed_arena_destroy_child(__local_arena__);
    return _return_value;
}

double __sn__zlibB__spaceSaved(RtManagedArena *__caller_arena__, long long __sn__originalSize, long long __sn__compressedSize) {
    RtManagedArena *__local_arena__ = rt_managed_arena_create_child(__caller_arena__);
    double _return_value = 0;
    _return_value = rt_sub_double(100.0, ({
    double __intercept_result;
    if (__rt_interceptor_count > 0) {
        RtAny __args[2];
        __args[0] = rt_box_int(__sn__originalSize);
        __args[1] = rt_box_int(__sn__compressedSize);
        __rt_thunk_args = __args;
        __rt_thunk_arena = __local_arena__;
        RtAny __intercepted = rt_call_intercepted("compressionRatio", __args, 2, __thunk_21);
        __intercept_result = rt_unbox_double(__intercepted);
    } else {
        __intercept_result = __sn__zlibB__compressionRatio(__local_arena__, __sn__originalSize, __sn__compressedSize);
    }
    __intercept_result;
}));
    goto __sn__zlibB__spaceSaved_return;
__sn__zlibB__spaceSaved_return:
    rt_managed_arena_destroy_child(__local_arena__);
    return _return_value;
}

RtUuid * __sn__UUID_create(RtManagedArena *__caller_arena__) {
    RtUuid * _return_value = {0};
    _return_value = sn_uuid_create(__caller_arena__);
    goto __sn__UUID_create_return;
__sn__UUID_create_return:
    return _return_value;
}

RtUuid * __sn__UUID_new(RtManagedArena *__caller_arena__) {
    RtUuid * _return_value = {0};
    _return_value = sn_uuid_create(__caller_arena__);
    goto __sn__UUID_new_return;
__sn__UUID_new_return:
    return _return_value;
}

RtUuid * __sn__UUID_v4(RtManagedArena *__caller_arena__) {
    RtUuid * _return_value = {0};
    _return_value = sn_uuid_v4(__caller_arena__);
    goto __sn__UUID_v4_return;
__sn__UUID_v4_return:
    return _return_value;
}

RtUuid * __sn__UUID_v5(RtManagedArena *__caller_arena__, RtUuid * __sn__namespace, RtHandle __sn__name) {
    RtUuid * _return_value = {0};
    _return_value = sn_uuid_v5(__caller_arena__, __sn__namespace, (char *)rt_managed_pin(__caller_arena__, __sn__name));
    goto __sn__UUID_v5_return;
__sn__UUID_v5_return:
    return _return_value;
}

RtUuid * __sn__UUID_v7(RtManagedArena *__caller_arena__) {
    RtUuid * _return_value = {0};
    _return_value = sn_uuid_v7(__caller_arena__);
    goto __sn__UUID_v7_return;
__sn__UUID_v7_return:
    return _return_value;
}

RtUuid * __sn__UUID_fromString(RtManagedArena *__caller_arena__, RtHandle __sn__s) {
    RtUuid * _return_value = {0};
    _return_value = sn_uuid_from_string(__caller_arena__, (char *)rt_managed_pin(__caller_arena__, __sn__s));
    goto __sn__UUID_fromString_return;
__sn__UUID_fromString_return:
    return _return_value;
}

RtUuid * __sn__UUID_fromHex(RtManagedArena *__caller_arena__, RtHandle __sn__s) {
    RtUuid * _return_value = {0};
    _return_value = sn_uuid_from_hex(__caller_arena__, (char *)rt_managed_pin(__caller_arena__, __sn__s));
    goto __sn__UUID_fromHex_return;
__sn__UUID_fromHex_return:
    return _return_value;
}

RtUuid * __sn__UUID_fromBase64(RtManagedArena *__caller_arena__, RtHandle __sn__s) {
    RtUuid * _return_value = {0};
    _return_value = sn_uuid_from_base64(__caller_arena__, (char *)rt_managed_pin(__caller_arena__, __sn__s));
    goto __sn__UUID_fromBase64_return;
__sn__UUID_fromBase64_return:
    return _return_value;
}

RtUuid * __sn__UUID_zero(RtManagedArena *__caller_arena__) {
    RtUuid * _return_value = {0};
    _return_value = sn_uuid_nil(__caller_arena__);
    goto __sn__UUID_zero_return;
__sn__UUID_zero_return:
    return _return_value;
}

RtUuid * __sn__UUID_max(RtManagedArena *__caller_arena__) {
    RtUuid * _return_value = {0};
    _return_value = sn_uuid_max(__caller_arena__);
    goto __sn__UUID_max_return;
__sn__UUID_max_return:
    return _return_value;
}

RtUuid * __sn__UUID_namespaceDns(RtManagedArena *__caller_arena__) {
    RtUuid * _return_value = {0};
    _return_value = sn_uuid_namespace_dns(__caller_arena__);
    goto __sn__UUID_namespaceDns_return;
__sn__UUID_namespaceDns_return:
    return _return_value;
}

RtUuid * __sn__UUID_namespaceUrl(RtManagedArena *__caller_arena__) {
    RtUuid * _return_value = {0};
    _return_value = sn_uuid_namespace_url(__caller_arena__);
    goto __sn__UUID_namespaceUrl_return;
__sn__UUID_namespaceUrl_return:
    return _return_value;
}

RtUuid * __sn__UUID_namespaceOid(RtManagedArena *__caller_arena__) {
    RtUuid * _return_value = {0};
    _return_value = sn_uuid_namespace_oid(__caller_arena__);
    goto __sn__UUID_namespaceOid_return;
__sn__UUID_namespaceOid_return:
    return _return_value;
}

RtUuid * __sn__UUID_namespaceX500(RtManagedArena *__caller_arena__) {
    RtUuid * _return_value = {0};
    _return_value = sn_uuid_namespace_x500(__caller_arena__);
    goto __sn__UUID_namespaceX500_return;
__sn__UUID_namespaceX500_return:
    return _return_value;
}

RtHandle __sn__UUID_toString(RtManagedArena *__caller_arena__, RtUuid *__sn__self) {
    RtHandle _return_value = RT_HANDLE_NULL;
    _return_value = sn_uuid_to_string(__caller_arena__, __sn__self);
    goto __sn__UUID_toString_return;
__sn__UUID_toString_return:
    return _return_value;
}

RtHandle __sn__UUID_toHex(RtManagedArena *__caller_arena__, RtUuid *__sn__self) {
    RtHandle _return_value = RT_HANDLE_NULL;
    _return_value = sn_uuid_to_hex(__caller_arena__, __sn__self);
    goto __sn__UUID_toHex_return;
__sn__UUID_toHex_return:
    return _return_value;
}

RtHandle __sn__UUID_toBase64(RtManagedArena *__caller_arena__, RtUuid *__sn__self) {
    RtHandle _return_value = RT_HANDLE_NULL;
    _return_value = sn_uuid_to_base64(__caller_arena__, __sn__self);
    goto __sn__UUID_toBase64_return;
__sn__UUID_toBase64_return:
    return _return_value;
}

RtHandle __sn__moduleB__randomValue = RT_HANDLE_NULL;
void __sn__moduleB__test_basic_compress_decompress_B(RtManagedArena *__caller_arena__) {
    RtManagedArena *__local_arena__ = rt_managed_arena_create_child(__caller_arena__);
    rt_print_string("test_basic_compress_decompress: ");
    // Original data - "Hello, World!"
    RtThread *__original_pending__ = NULL;
    RtHandle __sn__original = rt_array_create_byte_h(__local_arena__, 13, (unsigned char[]){72LL, 101LL, 108LL, 108LL, 111LL, 44LL, 32LL, 87LL, 111LL, 114LL, 108LL, 100LL, 33LL});
    // Compress
    RtThread *__compressed_pending__ = NULL;
    RtHandle __sn__compressed = compressData(__local_arena__, __sn__original);
    rt_assert(rt_gt_long(rt_array_length(((unsigned char *)rt_managed_pin_array(__local_arena__, __sn__compressed))), 0LL), "Compression should produce output");
    // Decompress
    RtThread *__decompressed_pending__ = NULL;
    RtHandle __sn__decompressed = decompressData(__local_arena__, __sn__compressed, rt_array_length(((unsigned char *)rt_managed_pin_array(__local_arena__, __sn__original))));
    rt_assert(rt_eq_long(rt_array_length(((unsigned char *)rt_managed_pin_array(__local_arena__, __sn__decompressed))), rt_array_length(((unsigned char *)rt_managed_pin_array(__local_arena__, __sn__original)))), "Decompressed size should match original");
    // Verify content matches
    RtThread *__i_pending__ = NULL;
    long long __sn__i = 0LL;
    while (rt_lt_long(__sn__i, rt_array_length(((unsigned char *)rt_managed_pin_array(__local_arena__, __sn__original))))) {
        {
            rt_assert(rt_eq_long(((unsigned char *)rt_managed_pin_array(__local_arena__, __sn__decompressed))[(__sn__i) < 0 ? rt_array_length(((unsigned char *)rt_managed_pin_array(__local_arena__, __sn__decompressed))) + (__sn__i) : (__sn__i)], ((unsigned char *)rt_managed_pin_array(__local_arena__, __sn__original))[(__sn__i) < 0 ? rt_array_length(((unsigned char *)rt_managed_pin_array(__local_arena__, __sn__original))) + (__sn__i) : (__sn__i)]), "Decompressed content should match original");
            (__sn__i = rt_add_long(__sn__i, 1LL));
        }
    }
    rt_print_string("PASS\n");
    goto __sn__moduleB__test_basic_compress_decompress_B_return;
__sn__moduleB__test_basic_compress_decompress_B_return:
    rt_managed_arena_destroy_child(__local_arena__);
    return;
}

long long __sn__moduleC__zlibC__zlibOk(RtManagedArena *);
long long __sn__moduleC__zlibC__zlibStreamEnd(RtManagedArena *);
long long __sn__moduleC__zlibC__zlibNeedDict(RtManagedArena *);
long long __sn__moduleC__zlibC__zlibErrno(RtManagedArena *);
long long __sn__moduleC__zlibC__zlibStreamError(RtManagedArena *);
long long __sn__moduleC__zlibC__zlibDataError(RtManagedArena *);
long long __sn__moduleC__zlibC__zlibMemError(RtManagedArena *);
long long __sn__moduleC__zlibC__zlibBufError(RtManagedArena *);
long long __sn__moduleC__zlibC__zlibVersionError(RtManagedArena *);
long long __sn__moduleC__zlibC__noCompression(RtManagedArena *);
long long __sn__moduleC__zlibC__bestSpeed(RtManagedArena *);
long long __sn__moduleC__zlibC__bestCompression(RtManagedArena *);
long long __sn__moduleC__zlibC__defaultCompression(RtManagedArena *);
RtHandle __sn__moduleC__zlibC__errorMessage(RtManagedArena *, long long);
long long __sn__moduleC__zlibC__compressTo(RtManagedArena *, RtHandle, RtHandle);
long long __sn__moduleC__zlibC__compressToLevel(RtManagedArena *, RtHandle, RtHandle, long long);
long long __sn__moduleC__zlibC__decompressTo(RtManagedArena *, RtHandle, RtHandle);
long long __sn__moduleC__zlibC__maxCompressedSize(RtManagedArena *, long long);
RtHandle __sn__moduleC__zlibC__compressData(RtManagedArena *, RtHandle);
RtHandle __sn__moduleC__zlibC__compressDataLevel(RtManagedArena *, RtHandle, long long);
RtHandle __sn__moduleC__zlibC__decompressData(RtManagedArena *, RtHandle, long long);
bool __sn__moduleC__zlibC__isCompressed(RtManagedArena *, RtHandle);
double __sn__moduleC__zlibC__compressionRatio(RtManagedArena *, long long, long long);
double __sn__moduleC__zlibC__spaceSaved(RtManagedArena *, long long, long long);
void __sn__moduleC__test_basic_compress_decompress_C(RtManagedArena *);
// Test SDK compression module
long long __sn__zlibC__zlibOk(RtManagedArena *);
long long __sn__zlibC__zlibStreamEnd(RtManagedArena *);
long long __sn__zlibC__zlibNeedDict(RtManagedArena *);
long long __sn__zlibC__zlibErrno(RtManagedArena *);
long long __sn__zlibC__zlibStreamError(RtManagedArena *);
long long __sn__zlibC__zlibDataError(RtManagedArena *);
long long __sn__zlibC__zlibMemError(RtManagedArena *);
long long __sn__zlibC__zlibBufError(RtManagedArena *);
long long __sn__zlibC__zlibVersionError(RtManagedArena *);
long long __sn__zlibC__noCompression(RtManagedArena *);
long long __sn__zlibC__bestSpeed(RtManagedArena *);
long long __sn__zlibC__bestCompression(RtManagedArena *);
long long __sn__zlibC__defaultCompression(RtManagedArena *);
RtHandle __sn__zlibC__errorMessage(RtManagedArena *, long long);
long long __sn__zlibC__compressTo(RtManagedArena *, RtHandle, RtHandle);
long long __sn__zlibC__compressToLevel(RtManagedArena *, RtHandle, RtHandle, long long);
long long __sn__zlibC__decompressTo(RtManagedArena *, RtHandle, RtHandle);
long long __sn__zlibC__maxCompressedSize(RtManagedArena *, long long);
RtHandle __sn__zlibC__compressData(RtManagedArena *, RtHandle);
RtHandle __sn__zlibC__compressDataLevel(RtManagedArena *, RtHandle, long long);
RtHandle __sn__zlibC__decompressData(RtManagedArena *, RtHandle, long long);
bool __sn__zlibC__isCompressed(RtManagedArena *, RtHandle);
double __sn__zlibC__compressionRatio(RtManagedArena *, long long, long long);
double __sn__zlibC__spaceSaved(RtManagedArena *, long long, long long);
long long __sn__zlibC__zlibOk(RtManagedArena *__caller_arena__) {
    RtManagedArena *__local_arena__ = rt_managed_arena_create_child(__caller_arena__);
    long long _return_value = 0;
    _return_value = 0LL;
    goto __sn__zlibC__zlibOk_return;
__sn__zlibC__zlibOk_return:
    rt_managed_arena_destroy_child(__local_arena__);
    return _return_value;
}

long long __sn__zlibC__zlibStreamEnd(RtManagedArena *__caller_arena__) {
    RtManagedArena *__local_arena__ = rt_managed_arena_create_child(__caller_arena__);
    long long _return_value = 0;
    _return_value = 1LL;
    goto __sn__zlibC__zlibStreamEnd_return;
__sn__zlibC__zlibStreamEnd_return:
    rt_managed_arena_destroy_child(__local_arena__);
    return _return_value;
}

long long __sn__zlibC__zlibNeedDict(RtManagedArena *__caller_arena__) {
    RtManagedArena *__local_arena__ = rt_managed_arena_create_child(__caller_arena__);
    long long _return_value = 0;
    _return_value = 2LL;
    goto __sn__zlibC__zlibNeedDict_return;
__sn__zlibC__zlibNeedDict_return:
    rt_managed_arena_destroy_child(__local_arena__);
    return _return_value;
}

long long __sn__zlibC__zlibErrno(RtManagedArena *__caller_arena__) {
    RtManagedArena *__local_arena__ = rt_managed_arena_create_child(__caller_arena__);
    long long _return_value = 0;
    _return_value = -1LL;
    goto __sn__zlibC__zlibErrno_return;
__sn__zlibC__zlibErrno_return:
    rt_managed_arena_destroy_child(__local_arena__);
    return _return_value;
}

long long __sn__zlibC__zlibStreamError(RtManagedArena *__caller_arena__) {
    RtManagedArena *__local_arena__ = rt_managed_arena_create_child(__caller_arena__);
    long long _return_value = 0;
    _return_value = -2LL;
    goto __sn__zlibC__zlibStreamError_return;
__sn__zlibC__zlibStreamError_return:
    rt_managed_arena_destroy_child(__local_arena__);
    return _return_value;
}

long long __sn__zlibC__zlibDataError(RtManagedArena *__caller_arena__) {
    RtManagedArena *__local_arena__ = rt_managed_arena_create_child(__caller_arena__);
    long long _return_value = 0;
    _return_value = -3LL;
    goto __sn__zlibC__zlibDataError_return;
__sn__zlibC__zlibDataError_return:
    rt_managed_arena_destroy_child(__local_arena__);
    return _return_value;
}

long long __sn__zlibC__zlibMemError(RtManagedArena *__caller_arena__) {
    RtManagedArena *__local_arena__ = rt_managed_arena_create_child(__caller_arena__);
    long long _return_value = 0;
    _return_value = -4LL;
    goto __sn__zlibC__zlibMemError_return;
__sn__zlibC__zlibMemError_return:
    rt_managed_arena_destroy_child(__local_arena__);
    return _return_value;
}

long long __sn__zlibC__zlibBufError(RtManagedArena *__caller_arena__) {
    RtManagedArena *__local_arena__ = rt_managed_arena_create_child(__caller_arena__);
    long long _return_value = 0;
    _return_value = -5LL;
    goto __sn__zlibC__zlibBufError_return;
__sn__zlibC__zlibBufError_return:
    rt_managed_arena_destroy_child(__local_arena__);
    return _return_value;
}

long long __sn__zlibC__zlibVersionError(RtManagedArena *__caller_arena__) {
    RtManagedArena *__local_arena__ = rt_managed_arena_create_child(__caller_arena__);
    long long _return_value = 0;
    _return_value = -6LL;
    goto __sn__zlibC__zlibVersionError_return;
__sn__zlibC__zlibVersionError_return:
    rt_managed_arena_destroy_child(__local_arena__);
    return _return_value;
}

long long __sn__zlibC__noCompression(RtManagedArena *__caller_arena__) {
    RtManagedArena *__local_arena__ = rt_managed_arena_create_child(__caller_arena__);
    long long _return_value = 0;
    _return_value = 0LL;
    goto __sn__zlibC__noCompression_return;
__sn__zlibC__noCompression_return:
    rt_managed_arena_destroy_child(__local_arena__);
    return _return_value;
}

long long __sn__zlibC__bestSpeed(RtManagedArena *__caller_arena__) {
    RtManagedArena *__local_arena__ = rt_managed_arena_create_child(__caller_arena__);
    long long _return_value = 0;
    _return_value = 1LL;
    goto __sn__zlibC__bestSpeed_return;
__sn__zlibC__bestSpeed_return:
    rt_managed_arena_destroy_child(__local_arena__);
    return _return_value;
}

long long __sn__zlibC__bestCompression(RtManagedArena *__caller_arena__) {
    RtManagedArena *__local_arena__ = rt_managed_arena_create_child(__caller_arena__);
    long long _return_value = 0;
    _return_value = 9LL;
    goto __sn__zlibC__bestCompression_return;
__sn__zlibC__bestCompression_return:
    rt_managed_arena_destroy_child(__local_arena__);
    return _return_value;
}

long long __sn__zlibC__defaultCompression(RtManagedArena *__caller_arena__) {
    RtManagedArena *__local_arena__ = rt_managed_arena_create_child(__caller_arena__);
    long long _return_value = 0;
    _return_value = -1LL;
    goto __sn__zlibC__defaultCompression_return;
__sn__zlibC__defaultCompression_return:
    rt_managed_arena_destroy_child(__local_arena__);
    return _return_value;
}

RtHandle __sn__zlibC__errorMessage(RtManagedArena *__caller_arena__, long long __sn__code) {
    RtManagedArena *__local_arena__ = rt_managed_arena_create_child(__caller_arena__);
    RtHandle _return_value = RT_HANDLE_NULL;
    if (rt_eq_long(__sn__code, ({
    long long __intercept_result;
    if (__rt_interceptor_count > 0) {
        RtAny __args[1];
        __rt_thunk_args = __args;
        __rt_thunk_arena = __local_arena__;
        RtAny __intercepted = rt_call_intercepted("zlibOk", __args, 0, __thunk_22);
        __intercept_result = rt_unbox_int(__intercepted);
    } else {
        __intercept_result = __sn__zlibC__zlibOk(__local_arena__);
    }
    __intercept_result;
}))) {
        {
            _return_value = rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "OK");
            goto __sn__zlibC__errorMessage_return;
        }
    }
    if (rt_eq_long(__sn__code, ({
    long long __intercept_result;
    if (__rt_interceptor_count > 0) {
        RtAny __args[1];
        __rt_thunk_args = __args;
        __rt_thunk_arena = __local_arena__;
        RtAny __intercepted = rt_call_intercepted("zlibStreamEnd", __args, 0, __thunk_23);
        __intercept_result = rt_unbox_int(__intercepted);
    } else {
        __intercept_result = __sn__zlibC__zlibStreamEnd(__local_arena__);
    }
    __intercept_result;
}))) {
        {
            _return_value = rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "Stream end");
            goto __sn__zlibC__errorMessage_return;
        }
    }
    if (rt_eq_long(__sn__code, ({
    long long __intercept_result;
    if (__rt_interceptor_count > 0) {
        RtAny __args[1];
        __rt_thunk_args = __args;
        __rt_thunk_arena = __local_arena__;
        RtAny __intercepted = rt_call_intercepted("zlibNeedDict", __args, 0, __thunk_24);
        __intercept_result = rt_unbox_int(__intercepted);
    } else {
        __intercept_result = __sn__zlibC__zlibNeedDict(__local_arena__);
    }
    __intercept_result;
}))) {
        {
            _return_value = rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "Need dictionary");
            goto __sn__zlibC__errorMessage_return;
        }
    }
    if (rt_eq_long(__sn__code, ({
    long long __intercept_result;
    if (__rt_interceptor_count > 0) {
        RtAny __args[1];
        __rt_thunk_args = __args;
        __rt_thunk_arena = __local_arena__;
        RtAny __intercepted = rt_call_intercepted("zlibErrno", __args, 0, __thunk_25);
        __intercept_result = rt_unbox_int(__intercepted);
    } else {
        __intercept_result = __sn__zlibC__zlibErrno(__local_arena__);
    }
    __intercept_result;
}))) {
        {
            _return_value = rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "File error");
            goto __sn__zlibC__errorMessage_return;
        }
    }
    if (rt_eq_long(__sn__code, ({
    long long __intercept_result;
    if (__rt_interceptor_count > 0) {
        RtAny __args[1];
        __rt_thunk_args = __args;
        __rt_thunk_arena = __local_arena__;
        RtAny __intercepted = rt_call_intercepted("zlibStreamError", __args, 0, __thunk_26);
        __intercept_result = rt_unbox_int(__intercepted);
    } else {
        __intercept_result = __sn__zlibC__zlibStreamError(__local_arena__);
    }
    __intercept_result;
}))) {
        {
            _return_value = rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "Stream error");
            goto __sn__zlibC__errorMessage_return;
        }
    }
    if (rt_eq_long(__sn__code, ({
    long long __intercept_result;
    if (__rt_interceptor_count > 0) {
        RtAny __args[1];
        __rt_thunk_args = __args;
        __rt_thunk_arena = __local_arena__;
        RtAny __intercepted = rt_call_intercepted("zlibDataError", __args, 0, __thunk_27);
        __intercept_result = rt_unbox_int(__intercepted);
    } else {
        __intercept_result = __sn__zlibC__zlibDataError(__local_arena__);
    }
    __intercept_result;
}))) {
        {
            _return_value = rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "Data error (corrupted or incomplete)");
            goto __sn__zlibC__errorMessage_return;
        }
    }
    if (rt_eq_long(__sn__code, ({
    long long __intercept_result;
    if (__rt_interceptor_count > 0) {
        RtAny __args[1];
        __rt_thunk_args = __args;
        __rt_thunk_arena = __local_arena__;
        RtAny __intercepted = rt_call_intercepted("zlibMemError", __args, 0, __thunk_28);
        __intercept_result = rt_unbox_int(__intercepted);
    } else {
        __intercept_result = __sn__zlibC__zlibMemError(__local_arena__);
    }
    __intercept_result;
}))) {
        {
            _return_value = rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "Memory error");
            goto __sn__zlibC__errorMessage_return;
        }
    }
    if (rt_eq_long(__sn__code, ({
    long long __intercept_result;
    if (__rt_interceptor_count > 0) {
        RtAny __args[1];
        __rt_thunk_args = __args;
        __rt_thunk_arena = __local_arena__;
        RtAny __intercepted = rt_call_intercepted("zlibBufError", __args, 0, __thunk_29);
        __intercept_result = rt_unbox_int(__intercepted);
    } else {
        __intercept_result = __sn__zlibC__zlibBufError(__local_arena__);
    }
    __intercept_result;
}))) {
        {
            _return_value = rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "Buffer error (output too small)");
            goto __sn__zlibC__errorMessage_return;
        }
    }
    if (rt_eq_long(__sn__code, ({
    long long __intercept_result;
    if (__rt_interceptor_count > 0) {
        RtAny __args[1];
        __rt_thunk_args = __args;
        __rt_thunk_arena = __local_arena__;
        RtAny __intercepted = rt_call_intercepted("zlibVersionError", __args, 0, __thunk_30);
        __intercept_result = rt_unbox_int(__intercepted);
    } else {
        __intercept_result = __sn__zlibC__zlibVersionError(__local_arena__);
    }
    __intercept_result;
}))) {
        {
            _return_value = rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "Version error");
            goto __sn__zlibC__errorMessage_return;
        }
    }
    _return_value = rt_managed_strdup(__local_arena__, RT_HANDLE_NULL, "Unknown error");
    goto __sn__zlibC__errorMessage_return;
__sn__zlibC__errorMessage_return:
    _return_value = rt_managed_promote(__caller_arena__, __local_arena__, _return_value);
    rt_managed_arena_destroy_child(__local_arena__);
    return _return_value;
}

long long __sn__zlibC__maxCompressedSize(RtManagedArena *__caller_arena__, long long __sn__sourceLen) {
    RtManagedArena *__local_arena__ = rt_managed_arena_create_child(__caller_arena__);
    long long _return_value = 0;
    _return_value = ((long long)(compressBound(((uint64_t)(__sn__sourceLen)))));
    goto __sn__zlibC__maxCompressedSize_return;
__sn__zlibC__maxCompressedSize_return:
    rt_managed_arena_destroy_child(__local_arena__);
    return _return_value;
}

bool __sn__zlibC__isCompressed(RtManagedArena *__caller_arena__, RtHandle __sn__data) {
    RtManagedArena *__local_arena__ = rt_managed_arena_create_child(__caller_arena__);
    bool _return_value = 0;
    if (rt_lt_long(rt_array_length(((unsigned char *)rt_managed_pin_array(__local_arena__, __sn__data))), 2LL)) {
        {
            _return_value = 0L;
            goto __sn__zlibC__isCompressed_return;
        }
    }
    RtThread *__cmf_pending__ = NULL;
    long long __sn__cmf = ((unsigned char *)rt_managed_pin_array(__local_arena__, __sn__data))[0LL];
    RtThread *__flg_pending__ = NULL;
    long long __sn__flg = ((unsigned char *)rt_managed_pin_array(__local_arena__, __sn__data))[1LL];
    if (rt_ne_long(__sn__cmf, 120LL)) {
        {
            _return_value = 0L;
            goto __sn__zlibC__isCompressed_return;
        }
    }
    RtThread *__check_pending__ = NULL;
    long long __sn__check = rt_add_long(rt_mul_long(__sn__cmf, 256LL), __sn__flg);
    _return_value = rt_eq_long(rt_mod_long(__sn__check, 31LL), 0LL);
    goto __sn__zlibC__isCompressed_return;
__sn__zlibC__isCompressed_return:
    rt_managed_arena_destroy_child(__local_arena__);
    return _return_value;
}

double __sn__zlibC__compressionRatio(RtManagedArena *__caller_arena__, long long __sn__originalSize, long long __sn__compressedSize) {
    RtManagedArena *__local_arena__ = rt_managed_arena_create_child(__caller_arena__);
    double _return_value = 0;
    if (rt_eq_long(__sn__originalSize, 0LL)) {
        {
            _return_value = 0.0;
            goto __sn__zlibC__compressionRatio_return;
        }
    }
    _return_value = rt_div_double(rt_mul_double(((double)(__sn__compressedSize)), 100.0), ((double)(__sn__originalSize)));
    goto __sn__zlibC__compressionRatio_return;
__sn__zlibC__compressionRatio_return:
    rt_managed_arena_destroy_child(__local_arena__);
    return _return_value;
}

double __sn__zlibC__spaceSaved(RtManagedArena *__caller_arena__, long long __sn__originalSize, long long __sn__compressedSize) {
    RtManagedArena *__local_arena__ = rt_managed_arena_create_child(__caller_arena__);
    double _return_value = 0;
    _return_value = rt_sub_double(100.0, ({
    double __intercept_result;
    if (__rt_interceptor_count > 0) {
        RtAny __args[2];
        __args[0] = rt_box_int(__sn__originalSize);
        __args[1] = rt_box_int(__sn__compressedSize);
        __rt_thunk_args = __args;
        __rt_thunk_arena = __local_arena__;
        RtAny __intercepted = rt_call_intercepted("compressionRatio", __args, 2, __thunk_31);
        __intercept_result = rt_unbox_double(__intercepted);
    } else {
        __intercept_result = __sn__zlibC__compressionRatio(__local_arena__, __sn__originalSize, __sn__compressedSize);
    }
    __intercept_result;
}));
    goto __sn__zlibC__spaceSaved_return;
__sn__zlibC__spaceSaved_return:
    rt_managed_arena_destroy_child(__local_arena__);
    return _return_value;
}

static RtUuid * __sn__import_aliasing_codegen_C__randomUuid = NULL;
RtHandle __sn__moduleC__randomValue = RT_HANDLE_NULL;
void __sn__moduleC__test_basic_compress_decompress_C(RtManagedArena *__caller_arena__) {
    RtManagedArena *__local_arena__ = rt_managed_arena_create_child(__caller_arena__);
    rt_print_string("test_basic_compress_decompress: ");
    // Original data - "Hello, World!"
    RtThread *__original_pending__ = NULL;
    RtHandle __sn__original = rt_array_create_byte_h(__local_arena__, 13, (unsigned char[]){72LL, 101LL, 108LL, 108LL, 111LL, 44LL, 32LL, 87LL, 111LL, 114LL, 108LL, 100LL, 33LL});
    // Compress
    RtThread *__compressed_pending__ = NULL;
    RtHandle __sn__compressed = compressData(__local_arena__, __sn__original);
    rt_assert(rt_gt_long(rt_array_length(((unsigned char *)rt_managed_pin_array(__local_arena__, __sn__compressed))), 0LL), "Compression should produce output");
    // Decompress
    RtThread *__decompressed_pending__ = NULL;
    RtHandle __sn__decompressed = decompressData(__local_arena__, __sn__compressed, rt_array_length(((unsigned char *)rt_managed_pin_array(__local_arena__, __sn__original))));
    rt_assert(rt_eq_long(rt_array_length(((unsigned char *)rt_managed_pin_array(__local_arena__, __sn__decompressed))), rt_array_length(((unsigned char *)rt_managed_pin_array(__local_arena__, __sn__original)))), "Decompressed size should match original");
    // Verify content matches
    RtThread *__i_pending__ = NULL;
    long long __sn__i = 0LL;
    while (rt_lt_long(__sn__i, rt_array_length(((unsigned char *)rt_managed_pin_array(__local_arena__, __sn__original))))) {
        {
            rt_assert(rt_eq_long(((unsigned char *)rt_managed_pin_array(__local_arena__, __sn__decompressed))[(__sn__i) < 0 ? rt_array_length(((unsigned char *)rt_managed_pin_array(__local_arena__, __sn__decompressed))) + (__sn__i) : (__sn__i)], ((unsigned char *)rt_managed_pin_array(__local_arena__, __sn__original))[(__sn__i) < 0 ? rt_array_length(((unsigned char *)rt_managed_pin_array(__local_arena__, __sn__original))) + (__sn__i) : (__sn__i)]), "Decompressed content should match original");
            (__sn__i = rt_add_long(__sn__i, 1LL));
        }
    }
    rt_print_string("PASS\n");
    goto __sn__moduleC__test_basic_compress_decompress_C_return;
__sn__moduleC__test_basic_compress_decompress_C_return:
    rt_managed_arena_destroy_child(__local_arena__);
    return;
}

long long __sn__moduleC_1__zlibC__zlibOk(RtManagedArena *);
long long __sn__moduleC_1__zlibC__zlibStreamEnd(RtManagedArena *);
long long __sn__moduleC_1__zlibC__zlibNeedDict(RtManagedArena *);
long long __sn__moduleC_1__zlibC__zlibErrno(RtManagedArena *);
long long __sn__moduleC_1__zlibC__zlibStreamError(RtManagedArena *);
long long __sn__moduleC_1__zlibC__zlibDataError(RtManagedArena *);
long long __sn__moduleC_1__zlibC__zlibMemError(RtManagedArena *);
long long __sn__moduleC_1__zlibC__zlibBufError(RtManagedArena *);
long long __sn__moduleC_1__zlibC__zlibVersionError(RtManagedArena *);
long long __sn__moduleC_1__zlibC__noCompression(RtManagedArena *);
long long __sn__moduleC_1__zlibC__bestSpeed(RtManagedArena *);
long long __sn__moduleC_1__zlibC__bestCompression(RtManagedArena *);
long long __sn__moduleC_1__zlibC__defaultCompression(RtManagedArena *);
RtHandle __sn__moduleC_1__zlibC__errorMessage(RtManagedArena *, long long);
long long __sn__moduleC_1__zlibC__compressTo(RtManagedArena *, RtHandle, RtHandle);
long long __sn__moduleC_1__zlibC__compressToLevel(RtManagedArena *, RtHandle, RtHandle, long long);
long long __sn__moduleC_1__zlibC__decompressTo(RtManagedArena *, RtHandle, RtHandle);
long long __sn__moduleC_1__zlibC__maxCompressedSize(RtManagedArena *, long long);
RtHandle __sn__moduleC_1__zlibC__compressData(RtManagedArena *, RtHandle);
RtHandle __sn__moduleC_1__zlibC__compressDataLevel(RtManagedArena *, RtHandle, long long);
RtHandle __sn__moduleC_1__zlibC__decompressData(RtManagedArena *, RtHandle, long long);
bool __sn__moduleC_1__zlibC__isCompressed(RtManagedArena *, RtHandle);
double __sn__moduleC_1__zlibC__compressionRatio(RtManagedArena *, long long, long long);
double __sn__moduleC_1__zlibC__spaceSaved(RtManagedArena *, long long, long long);
void __sn__moduleC_1__test_basic_compress_decompress_C(RtManagedArena *);
// Test SDK compression module
long long __sn__zlibC__zlibOk(RtManagedArena *);
long long __sn__zlibC__zlibStreamEnd(RtManagedArena *);
long long __sn__zlibC__zlibNeedDict(RtManagedArena *);
long long __sn__zlibC__zlibErrno(RtManagedArena *);
long long __sn__zlibC__zlibStreamError(RtManagedArena *);
long long __sn__zlibC__zlibDataError(RtManagedArena *);
long long __sn__zlibC__zlibMemError(RtManagedArena *);
long long __sn__zlibC__zlibBufError(RtManagedArena *);
long long __sn__zlibC__zlibVersionError(RtManagedArena *);
long long __sn__zlibC__noCompression(RtManagedArena *);
long long __sn__zlibC__bestSpeed(RtManagedArena *);
long long __sn__zlibC__bestCompression(RtManagedArena *);
long long __sn__zlibC__defaultCompression(RtManagedArena *);
RtHandle __sn__zlibC__errorMessage(RtManagedArena *, long long);
long long __sn__zlibC__compressTo(RtManagedArena *, RtHandle, RtHandle);
long long __sn__zlibC__compressToLevel(RtManagedArena *, RtHandle, RtHandle, long long);
long long __sn__zlibC__decompressTo(RtManagedArena *, RtHandle, RtHandle);
long long __sn__zlibC__maxCompressedSize(RtManagedArena *, long long);
RtHandle __sn__zlibC__compressData(RtManagedArena *, RtHandle);
RtHandle __sn__zlibC__compressDataLevel(RtManagedArena *, RtHandle, long long);
RtHandle __sn__zlibC__decompressData(RtManagedArena *, RtHandle, long long);
bool __sn__zlibC__isCompressed(RtManagedArena *, RtHandle);
double __sn__zlibC__compressionRatio(RtManagedArena *, long long, long long);
double __sn__zlibC__spaceSaved(RtManagedArena *, long long, long long);
RtHandle __sn__moduleC_1__randomValue = RT_HANDLE_NULL;
void __sn__moduleC_1__test_basic_compress_decompress_C(RtManagedArena *__caller_arena__) {
    RtManagedArena *__local_arena__ = rt_managed_arena_create_child(__caller_arena__);
    rt_print_string("test_basic_compress_decompress: ");
    // Original data - "Hello, World!"
    RtThread *__original_pending__ = NULL;
    RtHandle __sn__original = rt_array_create_byte_h(__local_arena__, 13, (unsigned char[]){72LL, 101LL, 108LL, 108LL, 111LL, 44LL, 32LL, 87LL, 111LL, 114LL, 108LL, 100LL, 33LL});
    // Compress
    RtThread *__compressed_pending__ = NULL;
    RtHandle __sn__compressed = compressData(__local_arena__, __sn__original);
    rt_assert(rt_gt_long(rt_array_length(((unsigned char *)rt_managed_pin_array(__local_arena__, __sn__compressed))), 0LL), "Compression should produce output");
    // Decompress
    RtThread *__decompressed_pending__ = NULL;
    RtHandle __sn__decompressed = decompressData(__local_arena__, __sn__compressed, rt_array_length(((unsigned char *)rt_managed_pin_array(__local_arena__, __sn__original))));
    rt_assert(rt_eq_long(rt_array_length(((unsigned char *)rt_managed_pin_array(__local_arena__, __sn__decompressed))), rt_array_length(((unsigned char *)rt_managed_pin_array(__local_arena__, __sn__original)))), "Decompressed size should match original");
    // Verify content matches
    RtThread *__i_pending__ = NULL;
    long long __sn__i = 0LL;
    while (rt_lt_long(__sn__i, rt_array_length(((unsigned char *)rt_managed_pin_array(__local_arena__, __sn__original))))) {
        {
            rt_assert(rt_eq_long(((unsigned char *)rt_managed_pin_array(__local_arena__, __sn__decompressed))[(__sn__i) < 0 ? rt_array_length(((unsigned char *)rt_managed_pin_array(__local_arena__, __sn__decompressed))) + (__sn__i) : (__sn__i)], ((unsigned char *)rt_managed_pin_array(__local_arena__, __sn__original))[(__sn__i) < 0 ? rt_array_length(((unsigned char *)rt_managed_pin_array(__local_arena__, __sn__original))) + (__sn__i) : (__sn__i)]), "Decompressed content should match original");
            (__sn__i = rt_add_long(__sn__i, 1LL));
        }
    }
    rt_print_string("PASS\n");
    goto __sn__moduleC_1__test_basic_compress_decompress_C_return;
__sn__moduleC_1__test_basic_compress_decompress_C_return:
    rt_managed_arena_destroy_child(__local_arena__);
    return;
}

static long long __sn__lockVal = 1LL;
void __sn__test_basic_compress_decompress_A(RtManagedArena *__caller_arena__) {
    RtManagedArena *__local_arena__ = rt_managed_arena_create_child(__caller_arena__);
    rt_print_string("test_basic_compress_decompress: ");
    // Original data - "Hello, World!"
    RtThread *__original_pending__ = NULL;
    RtHandle __sn__original = rt_array_create_byte_h(__local_arena__, 13, (unsigned char[]){72LL, 101LL, 108LL, 108LL, 111LL, 44LL, 32LL, 87LL, 111LL, 114LL, 108LL, 100LL, 33LL});
    // Compress
    RtThread *__compressed_pending__ = NULL;
    RtHandle __sn__compressed = compressData(__local_arena__, __sn__original);
    rt_assert(rt_gt_long(rt_array_length(((unsigned char *)rt_managed_pin_array(__local_arena__, __sn__compressed))), 0LL), "Compression should produce output");
    // Decompress
    RtThread *__decompressed_pending__ = NULL;
    RtHandle __sn__decompressed = decompressData(__local_arena__, __sn__compressed, rt_array_length(((unsigned char *)rt_managed_pin_array(__local_arena__, __sn__original))));
    rt_assert(rt_eq_long(rt_array_length(((unsigned char *)rt_managed_pin_array(__local_arena__, __sn__decompressed))), rt_array_length(((unsigned char *)rt_managed_pin_array(__local_arena__, __sn__original)))), "Decompressed size should match original");
    // Verify content matches
    RtThread *__i_pending__ = NULL;
    long long __sn__i = 0LL;
    while (rt_lt_long(__sn__i, rt_array_length(((unsigned char *)rt_managed_pin_array(__local_arena__, __sn__original))))) {
        {
            rt_assert(rt_eq_long(((unsigned char *)rt_managed_pin_array(__local_arena__, __sn__decompressed))[(__sn__i) < 0 ? rt_array_length(((unsigned char *)rt_managed_pin_array(__local_arena__, __sn__decompressed))) + (__sn__i) : (__sn__i)], ((unsigned char *)rt_managed_pin_array(__local_arena__, __sn__original))[(__sn__i) < 0 ? rt_array_length(((unsigned char *)rt_managed_pin_array(__local_arena__, __sn__original))) + (__sn__i) : (__sn__i)]), "Decompressed content should match original");
            (__sn__i = rt_add_long(__sn__i, 1LL));
        }
    }
    rt_print_string("PASS\n");
    goto __sn__test_basic_compress_decompress_A_return;
__sn__test_basic_compress_decompress_A_return:
    rt_managed_arena_destroy_child(__local_arena__);
    return;
}

int main() {
    RtManagedArena *__local_arena__ = rt_managed_arena_create();
    __main_arena__ = __local_arena__;
    __sn__moduleB__randomValue = __sn__UUID_toString(__main_arena__, __sn__UUID_create(__main_arena__));
    __sn__import_aliasing_codegen_C__randomUuid = __sn__UUID_create(__main_arena__);
    __sn__moduleC__randomValue = __sn__UUID_toString(__main_arena__, __sn__UUID_create(__main_arena__));
    __sn__moduleC_1__randomValue = __sn__UUID_toString(__main_arena__, __sn__UUID_create(__main_arena__));
    int _return_value = 0;
    rt_print_string("=== SDK Compression Tests ===\n\n");
    ({
    if (__rt_interceptor_count > 0) {
        RtAny __args[1];
        __rt_thunk_args = __args;
        __rt_thunk_arena = __local_arena__;
        RtAny __intercepted = rt_call_intercepted("test_basic_compress_decompress_A", __args, 0, __thunk_32);
    } else {
        __sn__test_basic_compress_decompress_A(__local_arena__);
    }
    (void)0;
});
    __sn__moduleB__test_basic_compress_decompress_B(__local_arena__);
    __sn__moduleC__test_basic_compress_decompress_C(__local_arena__);
    rt_assert(rt_ne_string((char *)rt_managed_pin(__local_arena__, __sn__moduleB__randomValue), (char *)rt_managed_pin(__local_arena__, __sn__moduleC__randomValue)), "Should be different because different modules");
    rt_assert(rt_ne_string((char *)rt_managed_pin(__local_arena__, __sn__moduleC__randomValue), (char *)rt_managed_pin(__local_arena__, __sn__moduleC_1__randomValue)), "Should be different because different aliased modules should have different state");
    // Static variables are SHARED across all aliases of the same module (canonical module identity)
    // Non-static variables are SEPARATE per alias (each alias has its own instance)
    rt_assert(rt_eq_string((char *)rt_managed_pin(__local_arena__, __sn__UUID_toString(__local_arena__, __sn__import_aliasing_codegen_C__randomUuid)), (char *)rt_managed_pin(__local_arena__, __sn__UUID_toString(__local_arena__, __sn__import_aliasing_codegen_C__randomUuid))), "Should be same because static vars are shared across aliases");
    rt_print_string("\nAll compression tests passed!\n");
    rt_sync_lock(&__sn__lockVal);
    {
        {
            (__sn__lockVal = 3LL);
        }
    }
    rt_sync_unlock(&__sn__lockVal);
    goto main_return;
main_return:
    rt_managed_arena_destroy(__local_arena__);
    return _return_value;
}


/* Interceptor thunk definitions */
static RtAny __thunk_0(void) {
    RtAny __result = rt_box_int(__sn__zlibA__zlibOk((RtArena *)__rt_thunk_arena));
    return __result;
}

static RtAny __thunk_1(void) {
    RtAny __result = rt_box_int(__sn__zlibA__zlibStreamEnd((RtArena *)__rt_thunk_arena));
    return __result;
}

static RtAny __thunk_2(void) {
    RtAny __result = rt_box_int(__sn__zlibA__zlibNeedDict((RtArena *)__rt_thunk_arena));
    return __result;
}

static RtAny __thunk_3(void) {
    RtAny __result = rt_box_int(__sn__zlibA__zlibErrno((RtArena *)__rt_thunk_arena));
    return __result;
}

static RtAny __thunk_4(void) {
    RtAny __result = rt_box_int(__sn__zlibA__zlibStreamError((RtArena *)__rt_thunk_arena));
    return __result;
}

static RtAny __thunk_5(void) {
    RtAny __result = rt_box_int(__sn__zlibA__zlibDataError((RtArena *)__rt_thunk_arena));
    return __result;
}

static RtAny __thunk_6(void) {
    RtAny __result = rt_box_int(__sn__zlibA__zlibMemError((RtArena *)__rt_thunk_arena));
    return __result;
}

static RtAny __thunk_7(void) {
    RtAny __result = rt_box_int(__sn__zlibA__zlibBufError((RtArena *)__rt_thunk_arena));
    return __result;
}

static RtAny __thunk_8(void) {
    RtAny __result = rt_box_int(__sn__zlibA__zlibVersionError((RtArena *)__rt_thunk_arena));
    return __result;
}

static RtAny __thunk_9(void) {
    RtAny __result = rt_box_int(__sn__zlibA__maxCompressedSize((RtArena *)__rt_thunk_arena, rt_unbox_int(__rt_thunk_args[0])));
    return __result;
}

static RtAny __thunk_10(void) {
    RtAny __result = rt_box_int(__sn__zlibA__maxCompressedSize((RtArena *)__rt_thunk_arena, rt_unbox_int(__rt_thunk_args[0])));
    return __result;
}

static RtAny __thunk_11(void) {
    RtAny __result = rt_box_double(__sn__zlibA__compressionRatio((RtArena *)__rt_thunk_arena, rt_unbox_int(__rt_thunk_args[0]), rt_unbox_int(__rt_thunk_args[1])));
    return __result;
}

static RtAny __thunk_12(void) {
    RtAny __result = rt_box_int(__sn__zlibB__zlibOk((RtArena *)__rt_thunk_arena));
    return __result;
}

static RtAny __thunk_13(void) {
    RtAny __result = rt_box_int(__sn__zlibB__zlibStreamEnd((RtArena *)__rt_thunk_arena));
    return __result;
}

static RtAny __thunk_14(void) {
    RtAny __result = rt_box_int(__sn__zlibB__zlibNeedDict((RtArena *)__rt_thunk_arena));
    return __result;
}

static RtAny __thunk_15(void) {
    RtAny __result = rt_box_int(__sn__zlibB__zlibErrno((RtArena *)__rt_thunk_arena));
    return __result;
}

static RtAny __thunk_16(void) {
    RtAny __result = rt_box_int(__sn__zlibB__zlibStreamError((RtArena *)__rt_thunk_arena));
    return __result;
}

static RtAny __thunk_17(void) {
    RtAny __result = rt_box_int(__sn__zlibB__zlibDataError((RtArena *)__rt_thunk_arena));
    return __result;
}

static RtAny __thunk_18(void) {
    RtAny __result = rt_box_int(__sn__zlibB__zlibMemError((RtArena *)__rt_thunk_arena));
    return __result;
}

static RtAny __thunk_19(void) {
    RtAny __result = rt_box_int(__sn__zlibB__zlibBufError((RtArena *)__rt_thunk_arena));
    return __result;
}

static RtAny __thunk_20(void) {
    RtAny __result = rt_box_int(__sn__zlibB__zlibVersionError((RtArena *)__rt_thunk_arena));
    return __result;
}

static RtAny __thunk_21(void) {
    RtAny __result = rt_box_double(__sn__zlibB__compressionRatio((RtArena *)__rt_thunk_arena, rt_unbox_int(__rt_thunk_args[0]), rt_unbox_int(__rt_thunk_args[1])));
    return __result;
}

static RtAny __thunk_22(void) {
    RtAny __result = rt_box_int(__sn__zlibC__zlibOk((RtArena *)__rt_thunk_arena));
    return __result;
}

static RtAny __thunk_23(void) {
    RtAny __result = rt_box_int(__sn__zlibC__zlibStreamEnd((RtArena *)__rt_thunk_arena));
    return __result;
}

static RtAny __thunk_24(void) {
    RtAny __result = rt_box_int(__sn__zlibC__zlibNeedDict((RtArena *)__rt_thunk_arena));
    return __result;
}

static RtAny __thunk_25(void) {
    RtAny __result = rt_box_int(__sn__zlibC__zlibErrno((RtArena *)__rt_thunk_arena));
    return __result;
}

static RtAny __thunk_26(void) {
    RtAny __result = rt_box_int(__sn__zlibC__zlibStreamError((RtArena *)__rt_thunk_arena));
    return __result;
}

static RtAny __thunk_27(void) {
    RtAny __result = rt_box_int(__sn__zlibC__zlibDataError((RtArena *)__rt_thunk_arena));
    return __result;
}

static RtAny __thunk_28(void) {
    RtAny __result = rt_box_int(__sn__zlibC__zlibMemError((RtArena *)__rt_thunk_arena));
    return __result;
}

static RtAny __thunk_29(void) {
    RtAny __result = rt_box_int(__sn__zlibC__zlibBufError((RtArena *)__rt_thunk_arena));
    return __result;
}

static RtAny __thunk_30(void) {
    RtAny __result = rt_box_int(__sn__zlibC__zlibVersionError((RtArena *)__rt_thunk_arena));
    return __result;
}

static RtAny __thunk_31(void) {
    RtAny __result = rt_box_double(__sn__zlibC__compressionRatio((RtArena *)__rt_thunk_arena, rt_unbox_int(__rt_thunk_args[0]), rt_unbox_int(__rt_thunk_args[1])));
    return __result;
}

static RtAny __thunk_32(void) {
    __sn__test_basic_compress_decompress_A((RtArena *)__rt_thunk_arena);
    return rt_box_nil();
}

