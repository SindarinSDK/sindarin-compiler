#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>
#include "sn_minimal.h"

/* Struct: IntSequence (as val) */
typedef struct {
    long long __sn__value;
    long long __sn__remaining;
    long long __sn__has_next_calls;
    long long __sn__next_calls;
} __sn__IntSequence;
/* Value operations */
static inline __sn__IntSequence __sn__IntSequence_copy(const __sn__IntSequence *src) {
    __sn__IntSequence dst;
    dst.__sn__value = src->__sn__value;
    dst.__sn__remaining = src->__sn__remaining;
    dst.__sn__has_next_calls = src->__sn__has_next_calls;
    dst.__sn__next_calls = src->__sn__next_calls;
    return dst;
}

static inline void __sn__IntSequence_cleanup(__sn__IntSequence *p) {

}

#define sn_auto_IntSequence __attribute__((cleanup(__sn__IntSequence_cleanup)))

static inline void __sn__IntSequence_cleanup_elem(void *p) { __sn__IntSequence_cleanup((__sn__IntSequence *)p); }
static inline void __sn__IntSequence_copy_into(const void *src, void *dst) { *(__sn__IntSequence *)dst = __sn__IntSequence_copy((const __sn__IntSequence *)src); }

/* Ref/pointer operations */
static inline __sn__IntSequence *__sn__IntSequence_alloc(void) {
    return calloc(1, sizeof(__sn__IntSequence));
}

static inline void __sn__IntSequence_release(__sn__IntSequence **p) {
    if (*p) {
        free(*p);
    }
    *p = NULL;
}

#define sn_auto_ref_IntSequence __attribute__((cleanup(__sn__IntSequence_release)))

static inline void __sn__IntSequence_release_elem(void *p) { __sn__IntSequence_release((__sn__IntSequence **)p); }

/* Auto-toString for string interpolation */
static inline char *__sn__IntSequence_to_string(const __sn__IntSequence *p) {
    char buf[1024];
    int off = 0;
    off += snprintf(buf + off, sizeof(buf) - off, "IntSequence { ");
    off += snprintf(buf + off, sizeof(buf) - off, "value: ");
    off += snprintf(buf + off, sizeof(buf) - off, "%lld", (long long)p->__sn__value);
    off += snprintf(buf + off, sizeof(buf) - off, ", ");
    off += snprintf(buf + off, sizeof(buf) - off, "remaining: ");
    off += snprintf(buf + off, sizeof(buf) - off, "%lld", (long long)p->__sn__remaining);
    off += snprintf(buf + off, sizeof(buf) - off, ", ");
    off += snprintf(buf + off, sizeof(buf) - off, "has_next_calls: ");
    off += snprintf(buf + off, sizeof(buf) - off, "%lld", (long long)p->__sn__has_next_calls);
    off += snprintf(buf + off, sizeof(buf) - off, ", ");
    off += snprintf(buf + off, sizeof(buf) - off, "next_calls: ");
    off += snprintf(buf + off, sizeof(buf) - off, "%lld", (long long)p->__sn__next_calls);
    off += snprintf(buf + off, sizeof(buf) - off, " }");
    return strdup(buf);
}



/* Struct: LongSequence (as val) */
typedef struct {
    long long __sn__value;
    long long __sn__remaining;
} __sn__LongSequence;
/* Value operations */
static inline __sn__LongSequence __sn__LongSequence_copy(const __sn__LongSequence *src) {
    __sn__LongSequence dst;
    dst.__sn__value = src->__sn__value;
    dst.__sn__remaining = src->__sn__remaining;
    return dst;
}

static inline void __sn__LongSequence_cleanup(__sn__LongSequence *p) {

}

#define sn_auto_LongSequence __attribute__((cleanup(__sn__LongSequence_cleanup)))

static inline void __sn__LongSequence_cleanup_elem(void *p) { __sn__LongSequence_cleanup((__sn__LongSequence *)p); }
static inline void __sn__LongSequence_copy_into(const void *src, void *dst) { *(__sn__LongSequence *)dst = __sn__LongSequence_copy((const __sn__LongSequence *)src); }

/* Ref/pointer operations */
static inline __sn__LongSequence *__sn__LongSequence_alloc(void) {
    return calloc(1, sizeof(__sn__LongSequence));
}

static inline void __sn__LongSequence_release(__sn__LongSequence **p) {
    if (*p) {
        free(*p);
    }
    *p = NULL;
}

#define sn_auto_ref_LongSequence __attribute__((cleanup(__sn__LongSequence_release)))

static inline void __sn__LongSequence_release_elem(void *p) { __sn__LongSequence_release((__sn__LongSequence **)p); }

/* Auto-toString for string interpolation */
static inline char *__sn__LongSequence_to_string(const __sn__LongSequence *p) {
    char buf[1024];
    int off = 0;
    off += snprintf(buf + off, sizeof(buf) - off, "LongSequence { ");
    off += snprintf(buf + off, sizeof(buf) - off, "value: ");
    off += snprintf(buf + off, sizeof(buf) - off, "%lld", (long long)p->__sn__value);
    off += snprintf(buf + off, sizeof(buf) - off, ", ");
    off += snprintf(buf + off, sizeof(buf) - off, "remaining: ");
    off += snprintf(buf + off, sizeof(buf) - off, "%lld", (long long)p->__sn__remaining);
    off += snprintf(buf + off, sizeof(buf) - off, " }");
    return strdup(buf);
}



/* Struct: Int32Sequence (as val) */
typedef struct {
    int32_t __sn__value;
    long long __sn__remaining;
} __sn__Int32Sequence;
/* Value operations */
static inline __sn__Int32Sequence __sn__Int32Sequence_copy(const __sn__Int32Sequence *src) {
    __sn__Int32Sequence dst;
    dst.__sn__value = src->__sn__value;
    dst.__sn__remaining = src->__sn__remaining;
    return dst;
}

static inline void __sn__Int32Sequence_cleanup(__sn__Int32Sequence *p) {

}

#define sn_auto_Int32Sequence __attribute__((cleanup(__sn__Int32Sequence_cleanup)))

static inline void __sn__Int32Sequence_cleanup_elem(void *p) { __sn__Int32Sequence_cleanup((__sn__Int32Sequence *)p); }
static inline void __sn__Int32Sequence_copy_into(const void *src, void *dst) { *(__sn__Int32Sequence *)dst = __sn__Int32Sequence_copy((const __sn__Int32Sequence *)src); }

/* Ref/pointer operations */
static inline __sn__Int32Sequence *__sn__Int32Sequence_alloc(void) {
    return calloc(1, sizeof(__sn__Int32Sequence));
}

static inline void __sn__Int32Sequence_release(__sn__Int32Sequence **p) {
    if (*p) {
        free(*p);
    }
    *p = NULL;
}

#define sn_auto_ref_Int32Sequence __attribute__((cleanup(__sn__Int32Sequence_release)))

static inline void __sn__Int32Sequence_release_elem(void *p) { __sn__Int32Sequence_release((__sn__Int32Sequence **)p); }

/* Auto-toString for string interpolation */
static inline char *__sn__Int32Sequence_to_string(const __sn__Int32Sequence *p) {
    char buf[1024];
    int off = 0;
    off += snprintf(buf + off, sizeof(buf) - off, "Int32Sequence { ");
    off += snprintf(buf + off, sizeof(buf) - off, "value: ");
    off += snprintf(buf + off, sizeof(buf) - off, "%lld", (long long)p->__sn__value);
    off += snprintf(buf + off, sizeof(buf) - off, ", ");
    off += snprintf(buf + off, sizeof(buf) - off, "remaining: ");
    off += snprintf(buf + off, sizeof(buf) - off, "%lld", (long long)p->__sn__remaining);
    off += snprintf(buf + off, sizeof(buf) - off, " }");
    return strdup(buf);
}



/* Struct: ByteSequence (as val) */
typedef struct {
    unsigned char __sn__value;
    long long __sn__remaining;
} __sn__ByteSequence;
/* Value operations */
static inline __sn__ByteSequence __sn__ByteSequence_copy(const __sn__ByteSequence *src) {
    __sn__ByteSequence dst;
    dst.__sn__value = src->__sn__value;
    dst.__sn__remaining = src->__sn__remaining;
    return dst;
}

static inline void __sn__ByteSequence_cleanup(__sn__ByteSequence *p) {

}

#define sn_auto_ByteSequence __attribute__((cleanup(__sn__ByteSequence_cleanup)))

static inline void __sn__ByteSequence_cleanup_elem(void *p) { __sn__ByteSequence_cleanup((__sn__ByteSequence *)p); }
static inline void __sn__ByteSequence_copy_into(const void *src, void *dst) { *(__sn__ByteSequence *)dst = __sn__ByteSequence_copy((const __sn__ByteSequence *)src); }

/* Ref/pointer operations */
static inline __sn__ByteSequence *__sn__ByteSequence_alloc(void) {
    return calloc(1, sizeof(__sn__ByteSequence));
}

static inline void __sn__ByteSequence_release(__sn__ByteSequence **p) {
    if (*p) {
        free(*p);
    }
    *p = NULL;
}

#define sn_auto_ref_ByteSequence __attribute__((cleanup(__sn__ByteSequence_release)))

static inline void __sn__ByteSequence_release_elem(void *p) { __sn__ByteSequence_release((__sn__ByteSequence **)p); }

/* Auto-toString for string interpolation */
static inline char *__sn__ByteSequence_to_string(const __sn__ByteSequence *p) {
    char buf[1024];
    int off = 0;
    off += snprintf(buf + off, sizeof(buf) - off, "ByteSequence { ");
    off += snprintf(buf + off, sizeof(buf) - off, "value: ");
    off += snprintf(buf + off, sizeof(buf) - off, "%u", (unsigned)p->__sn__value);
    off += snprintf(buf + off, sizeof(buf) - off, ", ");
    off += snprintf(buf + off, sizeof(buf) - off, "remaining: ");
    off += snprintf(buf + off, sizeof(buf) - off, "%lld", (long long)p->__sn__remaining);
    off += snprintf(buf + off, sizeof(buf) - off, " }");
    return strdup(buf);
}



/* Struct: Uint32Sequence (as val) */
typedef struct {
    uint32_t __sn__value;
    long long __sn__remaining;
} __sn__Uint32Sequence;
/* Value operations */
static inline __sn__Uint32Sequence __sn__Uint32Sequence_copy(const __sn__Uint32Sequence *src) {
    __sn__Uint32Sequence dst;
    dst.__sn__value = src->__sn__value;
    dst.__sn__remaining = src->__sn__remaining;
    return dst;
}

static inline void __sn__Uint32Sequence_cleanup(__sn__Uint32Sequence *p) {

}

#define sn_auto_Uint32Sequence __attribute__((cleanup(__sn__Uint32Sequence_cleanup)))

static inline void __sn__Uint32Sequence_cleanup_elem(void *p) { __sn__Uint32Sequence_cleanup((__sn__Uint32Sequence *)p); }
static inline void __sn__Uint32Sequence_copy_into(const void *src, void *dst) { *(__sn__Uint32Sequence *)dst = __sn__Uint32Sequence_copy((const __sn__Uint32Sequence *)src); }

/* Ref/pointer operations */
static inline __sn__Uint32Sequence *__sn__Uint32Sequence_alloc(void) {
    return calloc(1, sizeof(__sn__Uint32Sequence));
}

static inline void __sn__Uint32Sequence_release(__sn__Uint32Sequence **p) {
    if (*p) {
        free(*p);
    }
    *p = NULL;
}

#define sn_auto_ref_Uint32Sequence __attribute__((cleanup(__sn__Uint32Sequence_release)))

static inline void __sn__Uint32Sequence_release_elem(void *p) { __sn__Uint32Sequence_release((__sn__Uint32Sequence **)p); }

/* Auto-toString for string interpolation */
static inline char *__sn__Uint32Sequence_to_string(const __sn__Uint32Sequence *p) {
    char buf[1024];
    int off = 0;
    off += snprintf(buf + off, sizeof(buf) - off, "Uint32Sequence { ");
    off += snprintf(buf + off, sizeof(buf) - off, "value: ");
    off += snprintf(buf + off, sizeof(buf) - off, "%lld", (long long)p->__sn__value);
    off += snprintf(buf + off, sizeof(buf) - off, ", ");
    off += snprintf(buf + off, sizeof(buf) - off, "remaining: ");
    off += snprintf(buf + off, sizeof(buf) - off, "%lld", (long long)p->__sn__remaining);
    off += snprintf(buf + off, sizeof(buf) - off, " }");
    return strdup(buf);
}



/* Struct: UintSequence (as val) */
typedef struct {
    uint64_t __sn__value;
    long long __sn__remaining;
} __sn__UintSequence;
/* Value operations */
static inline __sn__UintSequence __sn__UintSequence_copy(const __sn__UintSequence *src) {
    __sn__UintSequence dst;
    dst.__sn__value = src->__sn__value;
    dst.__sn__remaining = src->__sn__remaining;
    return dst;
}

static inline void __sn__UintSequence_cleanup(__sn__UintSequence *p) {

}

#define sn_auto_UintSequence __attribute__((cleanup(__sn__UintSequence_cleanup)))

static inline void __sn__UintSequence_cleanup_elem(void *p) { __sn__UintSequence_cleanup((__sn__UintSequence *)p); }
static inline void __sn__UintSequence_copy_into(const void *src, void *dst) { *(__sn__UintSequence *)dst = __sn__UintSequence_copy((const __sn__UintSequence *)src); }

/* Ref/pointer operations */
static inline __sn__UintSequence *__sn__UintSequence_alloc(void) {
    return calloc(1, sizeof(__sn__UintSequence));
}

static inline void __sn__UintSequence_release(__sn__UintSequence **p) {
    if (*p) {
        free(*p);
    }
    *p = NULL;
}

#define sn_auto_ref_UintSequence __attribute__((cleanup(__sn__UintSequence_release)))

static inline void __sn__UintSequence_release_elem(void *p) { __sn__UintSequence_release((__sn__UintSequence **)p); }

/* Auto-toString for string interpolation */
static inline char *__sn__UintSequence_to_string(const __sn__UintSequence *p) {
    char buf[1024];
    int off = 0;
    off += snprintf(buf + off, sizeof(buf) - off, "UintSequence { ");
    off += snprintf(buf + off, sizeof(buf) - off, "value: ");
    off += snprintf(buf + off, sizeof(buf) - off, "%lld", (long long)p->__sn__value);
    off += snprintf(buf + off, sizeof(buf) - off, ", ");
    off += snprintf(buf + off, sizeof(buf) - off, "remaining: ");
    off += snprintf(buf + off, sizeof(buf) - off, "%lld", (long long)p->__sn__remaining);
    off += snprintf(buf + off, sizeof(buf) - off, " }");
    return strdup(buf);
}



/* Struct: FloatSequence (as val) */
typedef struct {
    float __sn__value;
    long long __sn__remaining;
} __sn__FloatSequence;
/* Value operations */
static inline __sn__FloatSequence __sn__FloatSequence_copy(const __sn__FloatSequence *src) {
    __sn__FloatSequence dst;
    dst.__sn__value = src->__sn__value;
    dst.__sn__remaining = src->__sn__remaining;
    return dst;
}

static inline void __sn__FloatSequence_cleanup(__sn__FloatSequence *p) {

}

#define sn_auto_FloatSequence __attribute__((cleanup(__sn__FloatSequence_cleanup)))

static inline void __sn__FloatSequence_cleanup_elem(void *p) { __sn__FloatSequence_cleanup((__sn__FloatSequence *)p); }
static inline void __sn__FloatSequence_copy_into(const void *src, void *dst) { *(__sn__FloatSequence *)dst = __sn__FloatSequence_copy((const __sn__FloatSequence *)src); }

/* Ref/pointer operations */
static inline __sn__FloatSequence *__sn__FloatSequence_alloc(void) {
    return calloc(1, sizeof(__sn__FloatSequence));
}

static inline void __sn__FloatSequence_release(__sn__FloatSequence **p) {
    if (*p) {
        free(*p);
    }
    *p = NULL;
}

#define sn_auto_ref_FloatSequence __attribute__((cleanup(__sn__FloatSequence_release)))

static inline void __sn__FloatSequence_release_elem(void *p) { __sn__FloatSequence_release((__sn__FloatSequence **)p); }

/* Auto-toString for string interpolation */
static inline char *__sn__FloatSequence_to_string(const __sn__FloatSequence *p) {
    char buf[1024];
    int off = 0;
    off += snprintf(buf + off, sizeof(buf) - off, "FloatSequence { ");
    off += snprintf(buf + off, sizeof(buf) - off, "value: ");
    off += snprintf(buf + off, sizeof(buf) - off, "%.5f", (double)p->__sn__value);
    off += snprintf(buf + off, sizeof(buf) - off, ", ");
    off += snprintf(buf + off, sizeof(buf) - off, "remaining: ");
    off += snprintf(buf + off, sizeof(buf) - off, "%lld", (long long)p->__sn__remaining);
    off += snprintf(buf + off, sizeof(buf) - off, " }");
    return strdup(buf);
}



/* Struct: DoubleSequence (as val) */
typedef struct {
    double __sn__value;
    long long __sn__remaining;
} __sn__DoubleSequence;
/* Value operations */
static inline __sn__DoubleSequence __sn__DoubleSequence_copy(const __sn__DoubleSequence *src) {
    __sn__DoubleSequence dst;
    dst.__sn__value = src->__sn__value;
    dst.__sn__remaining = src->__sn__remaining;
    return dst;
}

static inline void __sn__DoubleSequence_cleanup(__sn__DoubleSequence *p) {

}

#define sn_auto_DoubleSequence __attribute__((cleanup(__sn__DoubleSequence_cleanup)))

static inline void __sn__DoubleSequence_cleanup_elem(void *p) { __sn__DoubleSequence_cleanup((__sn__DoubleSequence *)p); }
static inline void __sn__DoubleSequence_copy_into(const void *src, void *dst) { *(__sn__DoubleSequence *)dst = __sn__DoubleSequence_copy((const __sn__DoubleSequence *)src); }

/* Ref/pointer operations */
static inline __sn__DoubleSequence *__sn__DoubleSequence_alloc(void) {
    return calloc(1, sizeof(__sn__DoubleSequence));
}

static inline void __sn__DoubleSequence_release(__sn__DoubleSequence **p) {
    if (*p) {
        free(*p);
    }
    *p = NULL;
}

#define sn_auto_ref_DoubleSequence __attribute__((cleanup(__sn__DoubleSequence_release)))

static inline void __sn__DoubleSequence_release_elem(void *p) { __sn__DoubleSequence_release((__sn__DoubleSequence **)p); }

/* Auto-toString for string interpolation */
static inline char *__sn__DoubleSequence_to_string(const __sn__DoubleSequence *p) {
    char buf[1024];
    int off = 0;
    off += snprintf(buf + off, sizeof(buf) - off, "DoubleSequence { ");
    off += snprintf(buf + off, sizeof(buf) - off, "value: ");
    off += snprintf(buf + off, sizeof(buf) - off, "%.5f", (double)p->__sn__value);
    off += snprintf(buf + off, sizeof(buf) - off, ", ");
    off += snprintf(buf + off, sizeof(buf) - off, "remaining: ");
    off += snprintf(buf + off, sizeof(buf) - off, "%lld", (long long)p->__sn__remaining);
    off += snprintf(buf + off, sizeof(buf) - off, " }");
    return strdup(buf);
}



long long __sn__rhsInt(long long *);
float __sn__rhsFloat(long long *);
long long __sn__selectInt(long long *);
__sn__IntSequence __sn__IntSequence_iter(__sn__IntSequence *);
bool __sn__IntSequence_hasNext(__sn__IntSequence *);
long long __sn__IntSequence_next(__sn__IntSequence *);
__sn__LongSequence __sn__LongSequence_iter(__sn__LongSequence *);
bool __sn__LongSequence_hasNext(__sn__LongSequence *);
long long __sn__LongSequence_next(__sn__LongSequence *);
__sn__Int32Sequence __sn__Int32Sequence_iter(__sn__Int32Sequence *);
bool __sn__Int32Sequence_hasNext(__sn__Int32Sequence *);
int32_t __sn__Int32Sequence_next(__sn__Int32Sequence *);
__sn__ByteSequence __sn__ByteSequence_iter(__sn__ByteSequence *);
bool __sn__ByteSequence_hasNext(__sn__ByteSequence *);
unsigned char __sn__ByteSequence_next(__sn__ByteSequence *);
__sn__Uint32Sequence __sn__Uint32Sequence_iter(__sn__Uint32Sequence *);
bool __sn__Uint32Sequence_hasNext(__sn__Uint32Sequence *);
uint32_t __sn__Uint32Sequence_next(__sn__Uint32Sequence *);
__sn__UintSequence __sn__UintSequence_iter(__sn__UintSequence *);
bool __sn__UintSequence_hasNext(__sn__UintSequence *);
uint64_t __sn__UintSequence_next(__sn__UintSequence *);
__sn__FloatSequence __sn__FloatSequence_iter(__sn__FloatSequence *);
bool __sn__FloatSequence_hasNext(__sn__FloatSequence *);
float __sn__FloatSequence_next(__sn__FloatSequence *);
__sn__DoubleSequence __sn__DoubleSequence_iter(__sn__DoubleSequence *);
bool __sn__DoubleSequence_hasNext(__sn__DoubleSequence *);
double __sn__DoubleSequence_next(__sn__DoubleSequence *);
typedef struct __Closure__ {
    void *fn;
    size_t size;
    void (*__cleanup__)(void *);
    int __rc__;
} __Closure__;


long long __sn__rhsInt(long long *__sn__calls) {

    ({
        long long __sn_rhs__ = 1LL;
        long long *__sn_place__ = &((*__sn__calls));
        *__sn_place__ = sn_add_long(*__sn_place__, __sn_rhs__);
        *__sn_place__;
    });
    

    return 2LL;}


float __sn__rhsFloat(long long *__sn__calls) {

    ({
        long long __sn_rhs__ = 1LL;
        long long *__sn_place__ = &((*__sn__calls));
        *__sn_place__ = sn_add_long(*__sn_place__, __sn_rhs__);
        *__sn_place__;
    });
    

    return 2.0f;}


long long __sn__selectInt(long long *__sn__calls) {

    ({
        long long __sn_rhs__ = 1LL;
        long long *__sn_place__ = &((*__sn__calls));
        *__sn_place__ = sn_add_long(*__sn_place__, __sn_rhs__);
        *__sn_place__;
    });
    

    return 0LL;}


__sn__IntSequence __sn__IntSequence_iter(__sn__IntSequence *__sn__self) {

    return __sn__IntSequence_copy(__sn__self);}

bool __sn__IntSequence_hasNext(__sn__IntSequence *__sn__self) {

    ({
        long long *__sn_place__ = &(__sn__self->__sn__has_next_calls);
        long long __sn_rhs__ = 1LL;
        *__sn_place__ = sn_add_long(*__sn_place__, __sn_rhs__);
        *__sn_place__;
    });
    

    return sn_lt_long(__sn__self->__sn__next_calls, __sn__self->__sn__remaining);}

long long __sn__IntSequence_next(__sn__IntSequence *__sn__self) {

    long long __sn__result = sn_add_long(sn_add_long(sn_mul_long(__sn__self->__sn__has_next_calls, 100LL), sn_mul_long(__sn__self->__sn__next_calls, 10LL)), __sn__self->__sn__value);

    ({
        long long *__sn_place__ = &(__sn__self->__sn__next_calls);
        long long __sn_rhs__ = 1LL;
        *__sn_place__ = sn_add_long(*__sn_place__, __sn_rhs__);
        *__sn_place__;
    });
    

    return __sn__result;}


__sn__LongSequence __sn__LongSequence_iter(__sn__LongSequence *__sn__self) {

    return __sn__LongSequence_copy(__sn__self);}

bool __sn__LongSequence_hasNext(__sn__LongSequence *__sn__self) {

    return sn_gt_long(__sn__self->__sn__remaining, 0LL);}

long long __sn__LongSequence_next(__sn__LongSequence *__sn__self) {

    ({
        long long *__sn_place__ = &(__sn__self->__sn__remaining);
        long long __sn_rhs__ = 1LL;
        *__sn_place__ = sn_sub_long(*__sn_place__, __sn_rhs__);
        *__sn_place__;
    });
    

    return __sn__self->__sn__value;}


__sn__Int32Sequence __sn__Int32Sequence_iter(__sn__Int32Sequence *__sn__self) {

    return __sn__Int32Sequence_copy(__sn__self);}

bool __sn__Int32Sequence_hasNext(__sn__Int32Sequence *__sn__self) {

    return sn_gt_long(__sn__self->__sn__remaining, 0LL);}

int32_t __sn__Int32Sequence_next(__sn__Int32Sequence *__sn__self) {

    ({
        long long *__sn_place__ = &(__sn__self->__sn__remaining);
        long long __sn_rhs__ = 1LL;
        *__sn_place__ = sn_sub_long(*__sn_place__, __sn_rhs__);
        *__sn_place__;
    });
    

    return __sn__self->__sn__value;}


__sn__ByteSequence __sn__ByteSequence_iter(__sn__ByteSequence *__sn__self) {

    return __sn__ByteSequence_copy(__sn__self);}

bool __sn__ByteSequence_hasNext(__sn__ByteSequence *__sn__self) {

    return sn_gt_long(__sn__self->__sn__remaining, 0LL);}

unsigned char __sn__ByteSequence_next(__sn__ByteSequence *__sn__self) {

    ({
        long long *__sn_place__ = &(__sn__self->__sn__remaining);
        long long __sn_rhs__ = 1LL;
        *__sn_place__ = sn_sub_long(*__sn_place__, __sn_rhs__);
        *__sn_place__;
    });
    

    return __sn__self->__sn__value;}


__sn__Uint32Sequence __sn__Uint32Sequence_iter(__sn__Uint32Sequence *__sn__self) {

    return __sn__Uint32Sequence_copy(__sn__self);}

bool __sn__Uint32Sequence_hasNext(__sn__Uint32Sequence *__sn__self) {

    return sn_gt_long(__sn__self->__sn__remaining, 0LL);}

uint32_t __sn__Uint32Sequence_next(__sn__Uint32Sequence *__sn__self) {

    ({
        long long *__sn_place__ = &(__sn__self->__sn__remaining);
        long long __sn_rhs__ = 1LL;
        *__sn_place__ = sn_sub_long(*__sn_place__, __sn_rhs__);
        *__sn_place__;
    });
    

    return __sn__self->__sn__value;}


__sn__UintSequence __sn__UintSequence_iter(__sn__UintSequence *__sn__self) {

    return __sn__UintSequence_copy(__sn__self);}

bool __sn__UintSequence_hasNext(__sn__UintSequence *__sn__self) {

    return sn_gt_long(__sn__self->__sn__remaining, 0LL);}

uint64_t __sn__UintSequence_next(__sn__UintSequence *__sn__self) {

    ({
        long long *__sn_place__ = &(__sn__self->__sn__remaining);
        long long __sn_rhs__ = 1LL;
        *__sn_place__ = sn_sub_long(*__sn_place__, __sn_rhs__);
        *__sn_place__;
    });
    

    return __sn__self->__sn__value;}


__sn__FloatSequence __sn__FloatSequence_iter(__sn__FloatSequence *__sn__self) {

    return __sn__FloatSequence_copy(__sn__self);}

bool __sn__FloatSequence_hasNext(__sn__FloatSequence *__sn__self) {

    return sn_gt_long(__sn__self->__sn__remaining, 0LL);}

float __sn__FloatSequence_next(__sn__FloatSequence *__sn__self) {

    ({
        long long *__sn_place__ = &(__sn__self->__sn__remaining);
        long long __sn_rhs__ = 1LL;
        *__sn_place__ = sn_sub_long(*__sn_place__, __sn_rhs__);
        *__sn_place__;
    });
    

    return __sn__self->__sn__value;}


__sn__DoubleSequence __sn__DoubleSequence_iter(__sn__DoubleSequence *__sn__self) {

    return __sn__DoubleSequence_copy(__sn__self);}

bool __sn__DoubleSequence_hasNext(__sn__DoubleSequence *__sn__self) {

    return sn_gt_long(__sn__self->__sn__remaining, 0LL);}

double __sn__DoubleSequence_next(__sn__DoubleSequence *__sn__self) {

    ({
        long long *__sn_place__ = &(__sn__self->__sn__remaining);
        long long __sn_rhs__ = 1LL;
        *__sn_place__ = sn_sub_long(*__sn_place__, __sn_rhs__);
        *__sn_place__;
    });
    

    return __sn__self->__sn__value;}

int main() {
    long long __sn____sn_rhs = 41LL;
    long long __sn____sn_place = 42LL;
    long long __sn____sn_next = 43LL;
    long long __sn____sn_previous = 44LL;
    long long __sn____sn_iter_0 = 45LL;
    long long __sn__iterable_calls = 0LL;
    long long __sn__int_rhs_calls = 0LL;
    long long __sn__float_rhs_calls = 0LL;
    sn_auto_arr SnArray * __sn__int_sources = ({
            SnArray *__al__ = sn_array_new(sizeof(__sn__IntSequence), 1);
            __al__->elem_tag = SN_TAG_STRUCT;
    
    
            sn_array_push(__al__, &((__sn__IntSequence){ .__sn__value = 8LL, .__sn__remaining = 2LL, .__sn__has_next_calls = 0LL, .__sn__next_calls = 0LL }));
            __al__;
        });
    {
        __sn__IntSequence __sn_iter__ = __sn__IntSequence_iter(&((((__sn__IntSequence *)__sn__int_sources->data)[({ long long __ai__ = __sn__selectInt(&__sn__iterable_calls); __ai__ < 0 ? __ai__ + __sn__int_sources->len : __ai__; })])));
        while (__sn__IntSequence_hasNext(&__sn_iter__)) {
            long long __sn__value = __sn__IntSequence_next(&__sn_iter__);
            {
                long long __sn__original = __sn__value;
                long long __sn__compound = __sn__value = __sn__value + __sn__rhsInt(&__sn__int_rhs_calls);
                long long __sn__postfix = __sn__value++;
                { sn_auto_str char *__ps__ = ({
                        sn_auto_str char *__is_p0__ = sn_strdup("int ");
                        sn_auto_str char *__is_p1__ = sn_str_fmt("%lld", (long long)(__sn__original));
                        sn_auto_str char *__is_p2__ = sn_strdup(" ");
                        sn_auto_str char *__is_p3__ = sn_str_fmt("%lld", (long long)(__sn__compound));
                        sn_auto_str char *__is_p4__ = sn_strdup(" ");
                        sn_auto_str char *__is_p5__ = sn_str_fmt("%lld", (long long)(__sn__postfix));
                        sn_auto_str char *__is_p6__ = sn_strdup(" ");
                        sn_auto_str char *__is_p7__ = sn_str_fmt("%lld", (long long)(__sn__value));
                        sn_str_concat_multi(8, __is_p0__, __is_p1__, __is_p2__, __is_p3__, __is_p4__, __is_p5__, __is_p6__, __is_p7__);
                    }); sn_println(__ps__); };
                
                if ((__sn__original == 108LL)) {
                    continue;
                }
            }
        }
    }
    __sn__LongSequence __sn__longs = (__sn__LongSequence){ .__sn__value = 20LL, .__sn__remaining = 2LL };
    {
        __sn__LongSequence __sn_iter__ = __sn__LongSequence_iter(&(__sn__longs));
        while (__sn__LongSequence_hasNext(&__sn_iter__)) {
            long long __sn__value = __sn__LongSequence_next(&__sn_iter__);
            {
                long long __sn__compound = __sn__value = __sn__value - 3LL;
                long long __sn__postfix = __sn__value--;
                { sn_auto_str char *__ps__ = ({
                        sn_auto_str char *__is_p0__ = sn_strdup("long ");
                        sn_auto_str char *__is_p1__ = sn_str_fmt("%lld", (long long)(__sn__compound));
                        sn_auto_str char *__is_p2__ = sn_strdup(" ");
                        sn_auto_str char *__is_p3__ = sn_str_fmt("%lld", (long long)(__sn__postfix));
                        sn_auto_str char *__is_p4__ = sn_strdup(" ");
                        sn_auto_str char *__is_p5__ = sn_str_fmt("%lld", (long long)(__sn__value));
                        sn_str_concat_multi(6, __is_p0__, __is_p1__, __is_p2__, __is_p3__, __is_p4__, __is_p5__);
                    }); sn_println(__ps__); };
                
            }
        }
    }
    __sn__Int32Sequence __sn__int32s = (__sn__Int32Sequence){ .__sn__value = 6LL, .__sn__remaining = 2LL };
    {
        __sn__Int32Sequence __sn_iter__ = __sn__Int32Sequence_iter(&(__sn__int32s));
        while (__sn__Int32Sequence_hasNext(&__sn_iter__)) {
            int32_t __sn__value = __sn__Int32Sequence_next(&__sn_iter__);
            {
                int32_t __sn__compound = __sn__value = __sn__value * 2LL;
                int32_t __sn__postfix = __sn__value++;
                { sn_auto_str char *__ps__ = ({
                        sn_auto_str char *__is_p0__ = sn_strdup("int32 ");
                        sn_auto_str char *__is_p1__ = sn_str_fmt("%lld", (long long)(__sn__compound));
                        sn_auto_str char *__is_p2__ = sn_strdup(" ");
                        sn_auto_str char *__is_p3__ = sn_str_fmt("%lld", (long long)(__sn__postfix));
                        sn_auto_str char *__is_p4__ = sn_strdup(" ");
                        sn_auto_str char *__is_p5__ = sn_str_fmt("%lld", (long long)(__sn__value));
                        sn_str_concat_multi(6, __is_p0__, __is_p1__, __is_p2__, __is_p3__, __is_p4__, __is_p5__);
                    }); sn_println(__ps__); };
                
            }
        }
    }
    __sn__ByteSequence __sn__bytes = (__sn__ByteSequence){ .__sn__value = (unsigned char)20, .__sn__remaining = 2LL };
    {
        __sn__ByteSequence __sn_iter__ = __sn__ByteSequence_iter(&(__sn__bytes));
        while (__sn__ByteSequence_hasNext(&__sn_iter__)) {
            unsigned char __sn__value = __sn__ByteSequence_next(&__sn_iter__);
            {
                unsigned char __sn__compound = __sn__value = __sn__value / (unsigned char)2;
                unsigned char __sn__postfix = __sn__value--;
                { sn_auto_str char *__ps__ = ({
                        sn_auto_str char *__is_p0__ = sn_strdup("byte ");
                        sn_auto_str char *__is_p1__ = sn_str_fmt("%u", (unsigned)(__sn__compound));
                        sn_auto_str char *__is_p2__ = sn_strdup(" ");
                        sn_auto_str char *__is_p3__ = sn_str_fmt("%u", (unsigned)(__sn__postfix));
                        sn_auto_str char *__is_p4__ = sn_strdup(" ");
                        sn_auto_str char *__is_p5__ = sn_str_fmt("%u", (unsigned)(__sn__value));
                        sn_str_concat_multi(6, __is_p0__, __is_p1__, __is_p2__, __is_p3__, __is_p4__, __is_p5__);
                    }); sn_println(__ps__); };
                
            }
        }
    }
    __sn__Uint32Sequence __sn__uint32s = (__sn__Uint32Sequence){ .__sn__value = 10LL, .__sn__remaining = 2LL };
    {
        __sn__Uint32Sequence __sn_iter__ = __sn__Uint32Sequence_iter(&(__sn__uint32s));
        while (__sn__Uint32Sequence_hasNext(&__sn_iter__)) {
            uint32_t __sn__value = __sn__Uint32Sequence_next(&__sn_iter__);
            {
                uint32_t __sn__compound = __sn__value = __sn__value % 6LL;
                uint32_t __sn__postfix = __sn__value++;
                { sn_auto_str char *__ps__ = ({
                        sn_auto_str char *__is_p0__ = sn_strdup("uint32 ");
                        sn_auto_str char *__is_p1__ = sn_str_fmt("%lld", (long long)(__sn__compound));
                        sn_auto_str char *__is_p2__ = sn_strdup(" ");
                        sn_auto_str char *__is_p3__ = sn_str_fmt("%lld", (long long)(__sn__postfix));
                        sn_auto_str char *__is_p4__ = sn_strdup(" ");
                        sn_auto_str char *__is_p5__ = sn_str_fmt("%lld", (long long)(__sn__value));
                        sn_str_concat_multi(6, __is_p0__, __is_p1__, __is_p2__, __is_p3__, __is_p4__, __is_p5__);
                    }); sn_println(__ps__); };
                
            }
        }
    }
    __sn__UintSequence __sn__uints = (__sn__UintSequence){ .__sn__value = 10LL, .__sn__remaining = 2LL };
    {
        __sn__UintSequence __sn_iter__ = __sn__UintSequence_iter(&(__sn__uints));
        while (__sn__UintSequence_hasNext(&__sn_iter__)) {
            uint64_t __sn__value = __sn__UintSequence_next(&__sn_iter__);
            {
                uint64_t __sn__compound = __sn__value = __sn__value + 3LL;
                uint64_t __sn__postfix = __sn__value--;
                { sn_auto_str char *__ps__ = ({
                        sn_auto_str char *__is_p0__ = sn_strdup("uint ");
                        sn_auto_str char *__is_p1__ = sn_str_fmt("%lld", (long long)(__sn__compound));
                        sn_auto_str char *__is_p2__ = sn_strdup(" ");
                        sn_auto_str char *__is_p3__ = sn_str_fmt("%lld", (long long)(__sn__postfix));
                        sn_auto_str char *__is_p4__ = sn_strdup(" ");
                        sn_auto_str char *__is_p5__ = sn_str_fmt("%lld", (long long)(__sn__value));
                        sn_str_concat_multi(6, __is_p0__, __is_p1__, __is_p2__, __is_p3__, __is_p4__, __is_p5__);
                    }); sn_println(__ps__); };
                
            }
        }
    }
    __sn__FloatSequence __sn__floats = (__sn__FloatSequence){ .__sn__value = 4.0f, .__sn__remaining = 2LL };
    {
        __sn__FloatSequence __sn_iter__ = __sn__FloatSequence_iter(&(__sn__floats));
        while (__sn__FloatSequence_hasNext(&__sn_iter__)) {
            float __sn__value = __sn__FloatSequence_next(&__sn_iter__);
            {
                float __sn__added = __sn__value = __sn__value + __sn__rhsFloat(&__sn__float_rhs_calls);
                float __sn__subtracted = __sn__value = __sn__value - 1.0f;
                float __sn__postfix = __sn__value++;
                { sn_auto_str char *__ps__ = ({
                        sn_auto_str char *__is_p0__ = sn_strdup("float ");
                        sn_auto_str char *__is_p1__ = sn_strdup(((__sn__added == 6.0f)) ? "true" : "false");
                        sn_auto_str char *__is_p2__ = sn_strdup(" ");
                        sn_auto_str char *__is_p3__ = sn_strdup(((__sn__subtracted == 5.0f)) ? "true" : "false");
                        sn_auto_str char *__is_p4__ = sn_strdup(" ");
                        sn_auto_str char *__is_p5__ = sn_strdup(((__sn__postfix == 5.0f)) ? "true" : "false");
                        sn_auto_str char *__is_p6__ = sn_strdup(" ");
                        sn_auto_str char *__is_p7__ = sn_strdup(((__sn__value == 6.0f)) ? "true" : "false");
                        sn_str_concat_multi(8, __is_p0__, __is_p1__, __is_p2__, __is_p3__, __is_p4__, __is_p5__, __is_p6__, __is_p7__);
                    }); sn_println(__ps__); };
                
            }
        }
    }
    __sn__DoubleSequence __sn__doubles = (__sn__DoubleSequence){ .__sn__value = 8.0, .__sn__remaining = 2LL };
    {
        __sn__DoubleSequence __sn_iter__ = __sn__DoubleSequence_iter(&(__sn__doubles));
        while (__sn__DoubleSequence_hasNext(&__sn_iter__)) {
            double __sn__value = __sn__DoubleSequence_next(&__sn_iter__);
            {
                double __sn__multiplied = __sn__value = __sn__value * 0.5;
                double __sn__divided = __sn__value = __sn__value / 2.0;
                double __sn__postfix = __sn__value--;
                { sn_auto_str char *__ps__ = ({
                        sn_auto_str char *__is_p0__ = sn_strdup("double ");
                        sn_auto_str char *__is_p1__ = sn_strdup(((__sn__multiplied == 4.0)) ? "true" : "false");
                        sn_auto_str char *__is_p2__ = sn_strdup(" ");
                        sn_auto_str char *__is_p3__ = sn_strdup(((__sn__divided == 2.0)) ? "true" : "false");
                        sn_auto_str char *__is_p4__ = sn_strdup(" ");
                        sn_auto_str char *__is_p5__ = sn_strdup(((__sn__postfix == 2.0)) ? "true" : "false");
                        sn_auto_str char *__is_p6__ = sn_strdup(" ");
                        sn_auto_str char *__is_p7__ = sn_strdup(((__sn__value == 1.0)) ? "true" : "false");
                        sn_str_concat_multi(8, __is_p0__, __is_p1__, __is_p2__, __is_p3__, __is_p4__, __is_p5__, __is_p6__, __is_p7__);
                    }); sn_println(__ps__); };
                
            }
        }
    }
    __sn__IntSequence __sn__nested_outer = (__sn__IntSequence){ .__sn__value = 1LL, .__sn__remaining = 2LL, .__sn__has_next_calls = 0LL, .__sn__next_calls = 0LL };
    __sn__ByteSequence __sn__nested_inner = (__sn__ByteSequence){ .__sn__value = (unsigned char)5, .__sn__remaining = 2LL };
    {
        __sn__IntSequence __sn_iter__ = __sn__IntSequence_iter(&(__sn__nested_outer));
        while (__sn__IntSequence_hasNext(&__sn_iter__)) {
            long long __sn__value = __sn__IntSequence_next(&__sn_iter__);
            {
                long long __sn__outer_compound = __sn__value = __sn__value + 1LL;
                {
                    __sn__ByteSequence __sn_iter__ = __sn__ByteSequence_iter(&(__sn__nested_inner));
                    while (__sn__ByteSequence_hasNext(&__sn_iter__)) {
                        unsigned char __sn__value = __sn__ByteSequence_next(&__sn_iter__);
                        {
                            unsigned char __sn__inner_postfix = __sn__value++;
                            { sn_auto_str char *__ps__ = ({
                                    sn_auto_str char *__is_p0__ = sn_strdup("nested inner ");
                                    sn_auto_str char *__is_p1__ = sn_str_fmt("%u", (unsigned)(__sn__inner_postfix));
                                    sn_auto_str char *__is_p2__ = sn_strdup(" ");
                                    sn_auto_str char *__is_p3__ = sn_str_fmt("%u", (unsigned)(__sn__value));
                                    sn_str_concat_multi(4, __is_p0__, __is_p1__, __is_p2__, __is_p3__);
                                }); sn_println(__ps__); };
                            
                            break;
                        }
                    }
                }
                long long __sn__outer_postfix = __sn__value--;
                { sn_auto_str char *__ps__ = ({
                        sn_auto_str char *__is_p0__ = sn_strdup("nested outer ");
                        sn_auto_str char *__is_p1__ = sn_str_fmt("%lld", (long long)(__sn__outer_compound));
                        sn_auto_str char *__is_p2__ = sn_strdup(" ");
                        sn_auto_str char *__is_p3__ = sn_str_fmt("%lld", (long long)(__sn__outer_postfix));
                        sn_auto_str char *__is_p4__ = sn_strdup(" ");
                        sn_auto_str char *__is_p5__ = sn_str_fmt("%lld", (long long)(__sn__value));
                        sn_str_concat_multi(6, __is_p0__, __is_p1__, __is_p2__, __is_p3__, __is_p4__, __is_p5__);
                    }); sn_println(__ps__); };
                
                continue;
            }
        }
    }
    { sn_auto_str char *__ps__ = ({
            sn_auto_str char *__is_p0__ = sn_strdup("state ");
            sn_auto_str char *__is_p1__ = sn_str_fmt("%lld", (long long)(__sn__iterable_calls));
            sn_auto_str char *__is_p2__ = sn_strdup(" ");
            sn_auto_str char *__is_p3__ = sn_str_fmt("%lld", (long long)(__sn__int_rhs_calls));
            sn_auto_str char *__is_p4__ = sn_strdup(" ");
            sn_auto_str char *__is_p5__ = sn_str_fmt("%lld", (long long)(__sn__float_rhs_calls));
            sn_auto_str char *__is_p6__ = sn_strdup(" ");
            sn_auto_str char *__is_p7__ = sn_str_fmt("%lld", (long long)((((__sn__IntSequence *)__sn__int_sources->data)[({ long long __ai__ = 0LL; __ai__ < 0 ? __ai__ + __sn__int_sources->len : __ai__; })]).__sn__value));
            sn_auto_str char *__is_p8__ = sn_strdup(" ");
            sn_auto_str char *__is_p9__ = sn_str_fmt("%lld", (long long)((((__sn__IntSequence *)__sn__int_sources->data)[({ long long __ai__ = 0LL; __ai__ < 0 ? __ai__ + __sn__int_sources->len : __ai__; })]).__sn__has_next_calls));
            sn_auto_str char *__is_p10__ = sn_strdup(" ");
            sn_auto_str char *__is_p11__ = sn_str_fmt("%lld", (long long)((((__sn__IntSequence *)__sn__int_sources->data)[({ long long __ai__ = 0LL; __ai__ < 0 ? __ai__ + __sn__int_sources->len : __ai__; })]).__sn__next_calls));
            sn_auto_str char *__is_p12__ = sn_strdup(" ");
            sn_auto_str char *__is_p13__ = sn_str_fmt("%lld", (long long)(__sn__longs.__sn__value));
            sn_auto_str char *__is_p14__ = sn_strdup(" ");
            sn_auto_str char *__is_p15__ = sn_str_fmt("%u", (unsigned)(__sn__bytes.__sn__value));
            sn_str_concat_multi(16, __is_p0__, __is_p1__, __is_p2__, __is_p3__, __is_p4__, __is_p5__, __is_p6__, __is_p7__, __is_p8__, __is_p9__, __is_p10__, __is_p11__, __is_p12__, __is_p13__, __is_p14__, __is_p15__);
        }); sn_println(__ps__); };
    
    { sn_auto_str char *__ps__ = ({
            sn_auto_str char *__is_p0__ = sn_strdup("helpers ");
            sn_auto_str char *__is_p1__ = sn_str_fmt("%lld", (long long)(__sn____sn_rhs));
            sn_auto_str char *__is_p2__ = sn_strdup(" ");
            sn_auto_str char *__is_p3__ = sn_str_fmt("%lld", (long long)(__sn____sn_place));
            sn_auto_str char *__is_p4__ = sn_strdup(" ");
            sn_auto_str char *__is_p5__ = sn_str_fmt("%lld", (long long)(__sn____sn_next));
            sn_auto_str char *__is_p6__ = sn_strdup(" ");
            sn_auto_str char *__is_p7__ = sn_str_fmt("%lld", (long long)(__sn____sn_previous));
            sn_auto_str char *__is_p8__ = sn_strdup(" ");
            sn_auto_str char *__is_p9__ = sn_str_fmt("%lld", (long long)(__sn____sn_iter_0));
            sn_str_concat_multi(10, __is_p0__, __is_p1__, __is_p2__, __is_p3__, __is_p4__, __is_p5__, __is_p6__, __is_p7__, __is_p8__, __is_p9__);
        }); sn_println(__ps__); };
    
    fflush(stdout);
    return 0;
}
