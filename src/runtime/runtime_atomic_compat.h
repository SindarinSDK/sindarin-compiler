#ifndef RUNTIME_ATOMIC_COMPAT_H
#define RUNTIME_ATOMIC_COMPAT_H

/* ============================================================================
 * Atomic Operations Compatibility Layer
 * ============================================================================
 * Provides mutex-based fallback implementations of GCC atomic builtins for
 * compilers that don't support them (e.g., TinyCC).
 *
 * When __TINYC__ is defined, these macros replace the GCC atomic builtins
 * with pthread mutex-based equivalents. This is slower but functionally
 * correct for all atomic operations.
 * ============================================================================ */

#ifdef __TINYC__

#include <pthread.h>

/* Global mutex for all atomic operations.
 * Using a single mutex is simpler and correct, though coarse-grained.
 * For better performance with many sync variables, a hash-based mutex
 * pool could be used, but this is sufficient for correctness. */
static pthread_mutex_t __sn_atomic_mutex = PTHREAD_MUTEX_INITIALIZER;

/* Memory order constants (ignored in mutex implementation, but needed for API compat) */
#ifndef __ATOMIC_SEQ_CST
#define __ATOMIC_SEQ_CST 5
#endif

/* __atomic_fetch_add: atomically add and return old value */
#define __atomic_fetch_add(ptr, val, order) ({ \
    pthread_mutex_lock(&__sn_atomic_mutex); \
    typeof(*(ptr)) __sn_old = *(ptr); \
    *(ptr) += (val); \
    pthread_mutex_unlock(&__sn_atomic_mutex); \
    __sn_old; \
})

/* __atomic_fetch_sub: atomically subtract and return old value */
#define __atomic_fetch_sub(ptr, val, order) ({ \
    pthread_mutex_lock(&__sn_atomic_mutex); \
    typeof(*(ptr)) __sn_old = *(ptr); \
    *(ptr) -= (val); \
    pthread_mutex_unlock(&__sn_atomic_mutex); \
    __sn_old; \
})

/* __atomic_load_n: atomically load value */
#define __atomic_load_n(ptr, order) ({ \
    pthread_mutex_lock(&__sn_atomic_mutex); \
    typeof(*(ptr)) __sn_val = *(ptr); \
    pthread_mutex_unlock(&__sn_atomic_mutex); \
    __sn_val; \
})

/* __atomic_store_n: atomically store value */
#define __atomic_store_n(ptr, val, order) ({ \
    pthread_mutex_lock(&__sn_atomic_mutex); \
    *(ptr) = (val); \
    pthread_mutex_unlock(&__sn_atomic_mutex); \
})

/* __atomic_compare_exchange_n: compare-and-swap operation
 * If *ptr == *expected, store desired and return true.
 * Otherwise, store *ptr into *expected and return false. */
#define __atomic_compare_exchange_n(ptr, expected, desired, weak, succ_order, fail_order) ({ \
    pthread_mutex_lock(&__sn_atomic_mutex); \
    int __sn_success = (*(ptr) == *(expected)); \
    if (__sn_success) { \
        *(ptr) = (desired); \
    } else { \
        *(expected) = *(ptr); \
    } \
    pthread_mutex_unlock(&__sn_atomic_mutex); \
    __sn_success; \
})

#endif /* __TINYC__ */

#endif /* RUNTIME_ATOMIC_COMPAT_H */
