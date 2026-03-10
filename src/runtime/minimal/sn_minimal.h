#ifndef SN_MINIMAL_H
#define SN_MINIMAL_H

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>

/* ---- Threading ---- */

/* Platform-specific pthread include */
#ifdef _WIN32
    #if (defined(__MINGW32__) || defined(__MINGW64__)) && !defined(SN_USE_WIN32_THREADS)
        #include <pthread.h>
    #else
        #include "platform/compat_pthread.h"
    #endif
#else
    #include <pthread.h>
#endif

typedef struct {
    pthread_t thread;
    void *result;
    size_t result_size;
    int joined;
} SnThread;

static inline void sn_cleanup_thread(SnThread **p)
{
    if (*p) {
        if (!(*p)->joined) {
            pthread_join((*p)->thread, NULL);
        }
        free((*p)->result);
        free(*p);
    }
}

#define sn_auto_thread __attribute__((cleanup(sn_cleanup_thread)))

static inline SnThread *sn_thread_create(void)
{
    SnThread *t = calloc(1, sizeof(SnThread));
    return t;
}

static inline void sn_thread_join(SnThread *t)
{
    if (t && !t->joined) {
        pthread_join(t->thread, NULL);
        t->joined = 1;
    }
}

/* ---- Scope-based cleanup ---- */

static inline void sn_cleanup_str(char **p) { free(*p); }
static inline void sn_cleanup_ptr(void **p) { free(*p); }

#define sn_auto_str __attribute__((cleanup(sn_cleanup_str)))
#define sn_auto_ptr __attribute__((cleanup(sn_cleanup_ptr)))

/* ---- String operations ---- */

static inline char *sn_strdup(const char *s)
{
    return s ? strdup(s) : NULL;
}

static inline long long sn_str_length(const char *s)
{
    return s ? (long long)strlen(s) : 0;
}

char *sn_str_concat(const char *a, const char *b);
char *sn_str_concat_multi(int count, ...);

/* ---- Dynamic array ---- */

typedef struct {
    void *data;
    long long len;
    long long cap;
    size_t elem_size;
    void (*elem_release)(void *);                    /* per-element cleanup (NULL = no-op) */
    void (*elem_copy)(const void *src, void *dst);   /* per-element copy (NULL = memcpy) */
} SnArray;

static inline void sn_cleanup_array(SnArray **p)
{
    if (*p) {
        if ((*p)->elem_release) {
            for (long long i = 0; i < (*p)->len; i++) {
                void *elem = (char *)(*p)->data + (i * (*p)->elem_size);
                (*p)->elem_release(elem);
            }
        }
        free((*p)->data);
        free(*p);
    }
}

#define sn_auto_arr __attribute__((cleanup(sn_cleanup_array)))

SnArray *sn_array_new(size_t elem_size, long long initial_cap);
void sn_array_push(SnArray *arr, const void *elem);
void *sn_array_get(SnArray *arr, long long index);
long long sn_array_length(SnArray *arr);
SnArray *sn_array_range(long long start, long long end);
SnArray *sn_array_copy(const SnArray *src);

/* ---- String copy helper (for array elem_copy) ---- */

static inline void sn_copy_str(const void *src, void *dst)
{
    const char *s = *(const char **)src;
    *(char **)dst = s ? strdup(s) : NULL;
}

/* Type-specific array accessors */
static inline long long sn_array_get_long(SnArray *arr, long long index)
{
    return ((long long *)arr->data)[index];
}

static inline double sn_array_get_double(SnArray *arr, long long index)
{
    return ((double *)arr->data)[index];
}

/* ---- I/O ---- */

static inline void sn_print(const char *s)
{
    if (s) fputs(s, stdout);
}

static inline void sn_println(const char *s)
{
    if (s) puts(s);
    else puts("");
}

/* ---- Assertions ---- */

static inline void sn_assert(bool condition, const char *message)
{
    if (!condition) {
        fprintf(stderr, "Assertion failed: %s\n", message ? message : "(no message)");
        exit(1);
    }
}

/* ---- Checked arithmetic ---- */

static inline long long sn_add_long(long long a, long long b)
{
    long long r;
    if (__builtin_add_overflow(a, b, &r)) {
        fprintf(stderr, "Runtime error: integer overflow in addition\n");
        exit(1);
    }
    return r;
}

static inline long long sn_sub_long(long long a, long long b)
{
    long long r;
    if (__builtin_sub_overflow(a, b, &r)) {
        fprintf(stderr, "Runtime error: integer overflow in subtraction\n");
        exit(1);
    }
    return r;
}

static inline long long sn_mul_long(long long a, long long b)
{
    long long r;
    if (__builtin_mul_overflow(a, b, &r)) {
        fprintf(stderr, "Runtime error: integer overflow in multiplication\n");
        exit(1);
    }
    return r;
}

static inline long long sn_div_long(long long a, long long b)
{
    if (b == 0) {
        fprintf(stderr, "Runtime error: division by zero\n");
        exit(1);
    }
    return a / b;
}

static inline long long sn_mod_long(long long a, long long b)
{
    if (b == 0) {
        fprintf(stderr, "Runtime error: modulo by zero\n");
        exit(1);
    }
    return a % b;
}

static inline long long sn_neg_long(long long a)
{
    return -a;
}

static inline double sn_add_double(double a, double b) { return a + b; }
static inline double sn_sub_double(double a, double b) { return a - b; }
static inline double sn_mul_double(double a, double b) { return a * b; }
static inline double sn_div_double(double a, double b) { return a / b; }
static inline double sn_neg_double(double a) { return -a; }

/* ---- Comparison helpers ---- */

static inline bool sn_eq_long(long long a, long long b) { return a == b; }
static inline bool sn_lt_long(long long a, long long b) { return a < b; }
static inline bool sn_gt_long(long long a, long long b) { return a > b; }
static inline bool sn_le_long(long long a, long long b) { return a <= b; }
static inline bool sn_ge_long(long long a, long long b) { return a >= b; }

/* ---- Exit ---- */

static inline void sn_exit(int code)
{
    exit(code);
}

#endif
