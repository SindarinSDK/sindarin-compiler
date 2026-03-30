#ifndef SN_ARITH_H
#define SN_ARITH_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

/* ---- Checked arithmetic (long long) ---- */

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
        fprintf(stderr, "panic: Division by zero\n");
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

/* ---- Byte arithmetic (unsigned, wrapping) ---- */

static inline unsigned char sn_add_byte(unsigned char a, unsigned char b) { return (unsigned char)(a + b); }
static inline unsigned char sn_sub_byte(unsigned char a, unsigned char b) { return (unsigned char)(a - b); }
static inline unsigned char sn_mul_byte(unsigned char a, unsigned char b) { return (unsigned char)(a * b); }

/* ---- int32 arithmetic (wrapping) ---- */

static inline int32_t sn_add_int32(int32_t a, int32_t b) { return (int32_t)((uint32_t)a + (uint32_t)b); }
static inline int32_t sn_sub_int32(int32_t a, int32_t b) { return (int32_t)((uint32_t)a - (uint32_t)b); }
static inline int32_t sn_mul_int32(int32_t a, int32_t b) { return (int32_t)((uint32_t)a * (uint32_t)b); }

/* ---- uint arithmetic (wrapping) ---- */

static inline uint64_t sn_add_uint(uint64_t a, uint64_t b) { return a + b; }
static inline uint64_t sn_sub_uint(uint64_t a, uint64_t b) { return a - b; }
static inline uint64_t sn_mul_uint(uint64_t a, uint64_t b) { return a * b; }

/* ---- uint32 arithmetic (wrapping) ---- */

static inline uint32_t sn_add_uint32(uint32_t a, uint32_t b) { return a + b; }
static inline uint32_t sn_sub_uint32(uint32_t a, uint32_t b) { return a - b; }
static inline uint32_t sn_mul_uint32(uint32_t a, uint32_t b) { return a * b; }

#endif
