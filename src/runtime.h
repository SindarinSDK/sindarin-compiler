#ifndef RUNTIME_H
#define RUNTIME_H

/* ============================================================================
 * Sindarin Runtime Library - Main Header
 * ============================================================================
 * This header provides all runtime functionality for compiled Sindarin programs.
 * Include order matters for dependencies - arena must come first.
 * ============================================================================ */

#include <stddef.h>
#include <stdbool.h>
#include <string.h>

/* Atomic compatibility layer - must come before any code using atomics */
#include "runtime/runtime_atomic_compat.h"

/* Core modules - arena must be first as other modules depend on it */
#include "runtime/runtime_arena.h"

/* Data type modules - depend on arena */
#include "runtime/runtime_string.h"
#include "runtime/runtime_array.h"

/* I/O modules - depend on arena and string */
#include "runtime/runtime_io.h"

/* Utility modules - depend on arena */
#include "runtime/runtime_byte.h"

/* Threading module - depends on arena */
#include "runtime/runtime_thread.h"

/* Any type module - depends on arena and array */
#include "runtime/runtime_any.h"

/* Interceptor module - depends on any */
#include "runtime/runtime_intercept.h"

/* ============================================================================
 * Arithmetic operations
 * ============================================================================ */

/* Long arithmetic (with overflow checking) - uses long long for 64-bit on all platforms */
long long rt_add_long(long long a, long long b);
long long rt_sub_long(long long a, long long b);
long long rt_mul_long(long long a, long long b);
long long rt_div_long(long long a, long long b);
long long rt_mod_long(long long a, long long b);
long long rt_neg_long(long long a);

/* Long comparisons - inlined for performance (use long long for 64-bit on all platforms) */
static inline int rt_eq_long(long long a, long long b) { return a == b; }
static inline int rt_ne_long(long long a, long long b) { return a != b; }
static inline int rt_lt_long(long long a, long long b) { return a < b; }
static inline int rt_le_long(long long a, long long b) { return a <= b; }
static inline int rt_gt_long(long long a, long long b) { return a > b; }
static inline int rt_ge_long(long long a, long long b) { return a >= b; }

/* Double arithmetic (with overflow checking) */
double rt_add_double(double a, double b);
double rt_sub_double(double a, double b);
double rt_mul_double(double a, double b);
double rt_div_double(double a, double b);
double rt_neg_double(double a);

/* Double comparisons - inlined for performance */
static inline int rt_eq_double(double a, double b) { return a == b; }
static inline int rt_ne_double(double a, double b) { return a != b; }
static inline int rt_lt_double(double a, double b) { return a < b; }
static inline int rt_le_double(double a, double b) { return a <= b; }
static inline int rt_gt_double(double a, double b) { return a > b; }
static inline int rt_ge_double(double a, double b) { return a >= b; }

/* Boolean operations - inlined for performance */
static inline int rt_not_bool(int a) { return !a; }

/* Increment/decrement */
long long rt_post_inc_long(long long *p);
long long rt_post_dec_long(long long *p);

/* String comparisons - inlined for performance, NULL-safe */
static inline int rt_eq_string(const char *a, const char *b) {
    if (a == NULL && b == NULL) return 1;
    if (a == NULL || b == NULL) return 0;
    return strcmp(a, b) == 0;
}
static inline int rt_ne_string(const char *a, const char *b) {
    if (a == NULL && b == NULL) return 0;
    if (a == NULL || b == NULL) return 1;
    return strcmp(a, b) != 0;
}
static inline int rt_lt_string(const char *a, const char *b) {
    if (a == NULL || b == NULL) return 0;
    return strcmp(a, b) < 0;
}
static inline int rt_le_string(const char *a, const char *b) {
    if (a == NULL && b == NULL) return 1;
    if (a == NULL || b == NULL) return 0;
    return strcmp(a, b) <= 0;
}
static inline int rt_gt_string(const char *a, const char *b) {
    if (a == NULL || b == NULL) return 0;
    return strcmp(a, b) > 0;
}
static inline int rt_ge_string(const char *a, const char *b) {
    if (a == NULL && b == NULL) return 1;
    if (a == NULL || b == NULL) return 0;
    return strcmp(a, b) >= 0;
}

/* Check if string is empty or contains only whitespace */
int rt_str_is_blank(const char *str);

/* Split string on whitespace */
char **rt_str_split_whitespace(RtArena *arena, const char *str);

/* Split string on line endings */
char **rt_str_split_lines(RtArena *arena, const char *str);

#endif /* RUNTIME_H */
