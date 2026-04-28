#ifndef SN_CORE_H
#define SN_CORE_H

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>
#include <ctype.h>

/* ---- Sanitizer defaults (active only when compiled with -g) ---- */

#if defined(__SANITIZE_ADDRESS__)
#define SN_ASAN_ACTIVE 1
#elif defined(__has_feature)
#if __has_feature(address_sanitizer)
#define SN_ASAN_ACTIVE 1
#endif
#endif

#ifdef SN_ASAN_ACTIVE
__attribute__((weak)) const char *__asan_default_options(void) {
    return "detect_leaks=1:halt_on_error=1:abort_on_error=0";
}
#endif

#if defined(__SANITIZE_UNDEFINED__)
#define SN_UBSAN_ACTIVE 1
#elif defined(__has_feature)
#if __has_feature(undefined_behavior_sanitizer)
#define SN_UBSAN_ACTIVE 1
#endif
#endif

#ifdef SN_UBSAN_ACTIVE
__attribute__((weak)) const char *__ubsan_default_options(void) {
    return "halt_on_error=1:print_stacktrace=1";
}
#endif

/* ---- OOM-safe allocation ---- */

static inline void *sn_malloc(size_t size)
{
    void *p = malloc(size);
    if (!p) { fprintf(stderr, "fatal: out of memory (malloc %zu bytes)\n", size); exit(1); }
    return p;
}

static inline void *sn_calloc(size_t n, size_t size)
{
    void *p = calloc(n, size);
    if (!p) { fprintf(stderr, "fatal: out of memory (calloc %zu x %zu bytes)\n", n, size); exit(1); }
    return p;
}

static inline void *sn_realloc(void *ptr, size_t size)
{
    void *p = realloc(ptr, size);
    if (!p && size > 0) { fprintf(stderr, "fatal: out of memory (realloc %zu bytes)\n", size); exit(1); }
    return p;
}

/* ---- Scope-based cleanup ---- */

static inline void sn_cleanup_str(char **p) { free(*p); }
static inline void sn_cleanup_ptr(void **p) { free(*p); }

/* Closures (and their per-lambda specialisations) all share this prefix.
 * The codegen emits matching __Closure__ / __closure_<id>__ typedefs whose
 * first four fields are byte-compatible with this layout, so generic
 * runtime helpers can retain/release without knowing the closure's type.
 *
 * Refcount semantics: every closure box is born with __rc__ == 1; sharing
 * (capturing into another closure, copying into a struct field, etc.) must
 * call sn_closure_retain; every destruction path must call sn_closure_release,
 * which runs __cleanup__ exactly once when the count hits zero. __cleanup__
 * is responsible for releasing owned captures and freeing the box itself. */
typedef struct {
    void *fn;
    size_t size;
    void (*__cleanup__)(void *);
    int __rc__;
} __SnClosureHeader__;

static inline void *sn_closure_retain(void *p) {
    if (p) ((__SnClosureHeader__ *)p)->__rc__++;
    return p;
}

static inline void sn_closure_release(void **p) {
    if (*p) {
        __SnClosureHeader__ *h = (__SnClosureHeader__ *)*p;
        if (--h->__rc__ == 0) {
            if (h->__cleanup__)
                h->__cleanup__(*p);
            else
                free(*p);
        }
    }
    *p = NULL;
}

static inline void sn_cleanup_fn(void **p) { sn_closure_release(p); }

#define sn_auto_str __attribute__((cleanup(sn_cleanup_str)))
#define sn_auto_ptr __attribute__((cleanup(sn_cleanup_ptr)))
#define sn_auto_fn __attribute__((cleanup(sn_cleanup_fn)))
/* Generic cleanup for captured variables (any pointer type) */
static inline void sn_cleanup_capture(void *p) { free(*(void **)p); *(void **)p = NULL; }
#define sn_auto_capture __attribute__((cleanup(sn_cleanup_capture)))

/* ---- Basic string helpers ---- */

static inline char *sn_strdup(const char *s)
{
    return s ? strdup(s) : NULL;
}

static inline long long sn_str_length(const char *s)
{
    return s ? (long long)strlen(s) : 0;
}

#endif
