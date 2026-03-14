#ifndef SN_CORE_H
#define SN_CORE_H

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>
#include <ctype.h>

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
static inline void sn_cleanup_fn(void **p) {
    if (*p) {
        void (**cleanup)(void *) = (void (**)(void *))((char *)*p + sizeof(void *) + sizeof(size_t));
        if (*cleanup)
            (*cleanup)(*p);
        else
            free(*p);
    }
    *p = NULL;
}

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
