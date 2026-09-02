#ifndef SN_ARITH_H
#define SN_ARITH_H

#include <stdint.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

/* ---- Checked arithmetic (long long) ---- */

static inline long long sn_add_long(long long a, long long b)
{
    if ((b > 0 && a > LLONG_MAX - b) ||
        (b < 0 && a < LLONG_MIN - b)) {
        fprintf(stderr, "Runtime error: integer overflow in addition\n");
        exit(1);
    }
    return a + b;
}

static inline long long sn_sub_long(long long a, long long b)
{
    if ((b > 0 && a < LLONG_MIN + b) ||
        (b < 0 && a > LLONG_MAX + b)) {
        fprintf(stderr, "Runtime error: integer overflow in subtraction\n");
        exit(1);
    }
    return a - b;
}

static inline long long sn_mul_long(long long a, long long b)
{
    if ((a > 0 && ((b > 0 && a > LLONG_MAX / b) ||
                   (b < 0 && b < LLONG_MIN / a))) ||
        (a < 0 && ((b > 0 && a < LLONG_MIN / b) ||
                   (b < 0 && a < LLONG_MAX / b)))) {
        fprintf(stderr, "Runtime error: integer overflow in multiplication\n");
        exit(1);
    }
    return a * b;
}

static inline long long sn_div_long(long long a, long long b)
{
    if (b == 0) {
        fprintf(stderr, "panic: Division by zero\n");
        exit(1);
    }
    if (a == LLONG_MIN && b == -1) {
        fprintf(stderr, "Runtime error: integer overflow in division\n");
        exit(1);
    }
    return a / b;
}

static inline long long sn_mod_long(long long a, long long b)
{
    if (b == 0) {
        fprintf(stderr, "panic: Modulo by zero\n");
        exit(1);
    }
    if (a == LLONG_MIN && b == -1) {
        fprintf(stderr, "Runtime error: integer overflow in modulo\n");
        exit(1);
    }
    return a % b;
}

static inline long long sn_neg_long(long long a) { return -a; }

/* ---- Double arithmetic ---- */

static inline double sn_add_double(double a, double b) { return a + b; }
static inline double sn_sub_double(double a, double b) { return a - b; }
static inline double sn_mul_double(double a, double b) { return a * b; }
static inline double sn_div_double(double a, double b) { return a / b; }
static inline double sn_neg_double(double a) { return -a; }

/* ---- Float arithmetic ---- */

static inline float sn_add_float(float a, float b) { return a + b; }
static inline float sn_sub_float(float a, float b) { return a - b; }
static inline float sn_mul_float(float a, float b) { return a * b; }
static inline float sn_div_float(float a, float b) { return a / b; }

/* ---- Checked byte arithmetic ---- */

/* Source-language bytes are exact u8 values.  Check the operands before
 * evaluating an operation whose result would leave that range, so the
 * subsequent promoted operation and implicit return conversion are exact. */
static inline unsigned char sn_add_byte(unsigned char a, unsigned char b)
{
    if (a > UINT8_MAX - b) {
        fprintf(stderr, "Runtime error: integer overflow in addition\n");
        exit(1);
    }
    return a + b;
}

static inline unsigned char sn_sub_byte(unsigned char a, unsigned char b)
{
    if (a < b) {
        fprintf(stderr, "Runtime error: integer overflow in subtraction\n");
        exit(1);
    }
    return a - b;
}

static inline unsigned char sn_mul_byte(unsigned char a, unsigned char b)
{
    if (b != 0 && a > UINT8_MAX / b) {
        fprintf(stderr, "Runtime error: integer overflow in multiplication\n");
        exit(1);
    }
    return a * b;
}

static inline unsigned char sn_div_byte(unsigned char a, unsigned char b)
{
    if (b == 0) {
        fprintf(stderr, "panic: Division by zero\n");
        exit(1);
    }
    return a / b;
}

static inline unsigned char sn_mod_byte(unsigned char a, unsigned char b)
{
    if (b == 0) {
        fprintf(stderr, "panic: Modulo by zero\n");
        exit(1);
    }
    return a % b;
}

/* ---- Checked int32 arithmetic ---- */

static inline int32_t sn_add_int32(int32_t a, int32_t b)
{
    int64_t r = (int64_t)a + (int64_t)b;
    if (r < INT32_MIN || r > INT32_MAX) {
        fprintf(stderr, "Runtime error: integer overflow in addition\n");
        exit(1);
    }
    return (int32_t)r;
}

static inline int32_t sn_sub_int32(int32_t a, int32_t b)
{
    int64_t r = (int64_t)a - (int64_t)b;
    if (r < INT32_MIN || r > INT32_MAX) {
        fprintf(stderr, "Runtime error: integer overflow in subtraction\n");
        exit(1);
    }
    return (int32_t)r;
}

static inline int32_t sn_mul_int32(int32_t a, int32_t b)
{
    int64_t r = (int64_t)a * (int64_t)b;
    if (r < INT32_MIN || r > INT32_MAX) {
        fprintf(stderr, "Runtime error: integer overflow in multiplication\n");
        exit(1);
    }
    return (int32_t)r;
}

static inline int32_t sn_div_int32(int32_t a, int32_t b)
{
    if (b == 0) {
        fprintf(stderr, "panic: Division by zero\n");
        exit(1);
    }
    if (a == INT32_MIN && b == -1) {
        fprintf(stderr, "Runtime error: integer overflow in division\n");
        exit(1);
    }
    return a / b;
}

static inline int32_t sn_mod_int32(int32_t a, int32_t b)
{
    if (b == 0) {
        fprintf(stderr, "panic: Modulo by zero\n");
        exit(1);
    }
    if (a == INT32_MIN && b == -1) {
        fprintf(stderr, "Runtime error: integer overflow in modulo\n");
        exit(1);
    }
    return a % b;
}

/* ---- Checked uint arithmetic ---- */

static inline uint64_t sn_add_uint(uint64_t a, uint64_t b)
{
    if (a > UINT64_MAX - b) {
        fprintf(stderr, "Runtime error: integer overflow in addition\n");
        exit(1);
    }
    return a + b;
}

static inline uint64_t sn_sub_uint(uint64_t a, uint64_t b)
{
    if (a < b) {
        fprintf(stderr, "Runtime error: integer overflow in subtraction\n");
        exit(1);
    }
    return a - b;
}

static inline uint64_t sn_mul_uint(uint64_t a, uint64_t b)
{
    if (b != 0 && a > UINT64_MAX / b) {
        fprintf(stderr, "Runtime error: integer overflow in multiplication\n");
        exit(1);
    }
    return a * b;
}

static inline uint64_t sn_div_uint(uint64_t a, uint64_t b)
{
    if (b == 0) {
        fprintf(stderr, "panic: Division by zero\n");
        exit(1);
    }
    return a / b;
}

static inline uint64_t sn_mod_uint(uint64_t a, uint64_t b)
{
    if (b == 0) {
        fprintf(stderr, "panic: Modulo by zero\n");
        exit(1);
    }
    return a % b;
}

/* ---- Checked uint32 arithmetic ---- */

static inline uint32_t sn_add_uint32(uint32_t a, uint32_t b)
{
    uint64_t r = (uint64_t)a + (uint64_t)b;
    if (r > UINT32_MAX) {
        fprintf(stderr, "Runtime error: integer overflow in addition\n");
        exit(1);
    }
    return (uint32_t)r;
}

static inline uint32_t sn_sub_uint32(uint32_t a, uint32_t b)
{
    if (a < b) {
        fprintf(stderr, "Runtime error: integer overflow in subtraction\n");
        exit(1);
    }
    return a - b;
}

static inline uint32_t sn_mul_uint32(uint32_t a, uint32_t b)
{
    uint64_t r = (uint64_t)a * (uint64_t)b;
    if (r > UINT32_MAX) {
        fprintf(stderr, "Runtime error: integer overflow in multiplication\n");
        exit(1);
    }
    return (uint32_t)r;
}

static inline uint32_t sn_div_uint32(uint32_t a, uint32_t b)
{
    if (b == 0) {
        fprintf(stderr, "panic: Division by zero\n");
        exit(1);
    }
    return a / b;
}

static inline uint32_t sn_mod_uint32(uint32_t a, uint32_t b)
{
    if (b == 0) {
        fprintf(stderr, "panic: Modulo by zero\n");
        exit(1);
    }
    return a % b;
}

#endif
