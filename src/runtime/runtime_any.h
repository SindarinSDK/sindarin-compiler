#ifndef RUNTIME_ANY_H
#define RUNTIME_ANY_H

#include <stdint.h>
#include <stdbool.h>
#include "arena/arena_v2.h"

/* ============================================================================
 * Any Type - Runtime Type System
 * ============================================================================
 * The 'any' type is a tagged union that can hold any Sindarin value.
 * It provides runtime type checking and casting capabilities.
 * ============================================================================ */

/* Type tags for runtime type identification */
typedef enum {
    RT_ANY_NIL = 0,
    RT_ANY_INT,
    RT_ANY_LONG,
    RT_ANY_INT32,
    RT_ANY_UINT,
    RT_ANY_UINT32,
    RT_ANY_DOUBLE,
    RT_ANY_FLOAT,
    RT_ANY_STRING,
    RT_ANY_CHAR,
    RT_ANY_BOOL,
    RT_ANY_BYTE,
    RT_ANY_ARRAY,
    RT_ANY_FUNCTION,
    RT_ANY_STRUCT       /* Boxed struct value */
} RtAnyTag;

/* The any type - a tagged union */
typedef struct {
    RtAnyTag tag;
    union {
        int64_t i64;        /* int, long */
        int32_t i32;        /* int32 */
        uint64_t u64;       /* uint */
        uint32_t u32;       /* uint32 */
        double d;           /* double */
        float f;            /* float */
        char *s;            /* str */
        char c;             /* char */
        bool b;             /* bool */
        uint8_t byte;       /* byte */
        RtHandleV2 *arr;    /* arrays (RtArray*) */
        RtHandleV2 *fn;     /* function pointers */
        RtHandleV2 *obj;    /* object types (files, etc.) */
    } value;
    /* For arrays: track element type tag for nested any[] support */
    RtAnyTag element_tag;
} RtAny;

/* ============================================================================
 * Boxing Functions - Convert concrete types to any
 * ============================================================================ */

RtAny rt_box_nil(void);
RtAny rt_box_int(int64_t value);
RtAny rt_box_long(int64_t value);
RtAny rt_box_int32(int32_t value);
RtAny rt_box_uint(uint64_t value);
RtAny rt_box_uint32(uint32_t value);
RtAny rt_box_double(double value);
RtAny rt_box_float(float value);
RtAny rt_box_string(const char *value);
RtAny rt_box_char(char value);
RtAny rt_box_bool(bool value);
RtAny rt_box_byte(uint8_t value);
RtAny rt_box_array(RtHandleV2 *arr, RtAnyTag element_tag);
RtAny rt_box_function(RtHandleV2 *fn);
RtAny rt_box_struct(RtArenaV2 *arena, RtHandleV2 *struct_data, size_t struct_size, int struct_type_id);

/* ============================================================================
 * Unboxing Functions - Convert any to concrete types (panic on type mismatch)
 * ============================================================================ */

int64_t rt_unbox_int(RtAny value);
int64_t rt_unbox_long(RtAny value);
int32_t rt_unbox_int32(RtAny value);
uint64_t rt_unbox_uint(RtAny value);
uint32_t rt_unbox_uint32(RtAny value);
double rt_unbox_double(RtAny value);
float rt_unbox_float(RtAny value);
const char *rt_unbox_string(RtAny value);
char rt_unbox_char(RtAny value);
bool rt_unbox_bool(RtAny value);
uint8_t rt_unbox_byte(RtAny value);
RtHandleV2 *rt_unbox_array(RtAny value);
RtHandleV2 *rt_unbox_function(RtAny value);
RtHandleV2 *rt_unbox_struct(RtAny value, int expected_type_id);

/* ============================================================================
 * Type Checking Functions
 * ============================================================================ */

/* Check if any value has a specific type */
bool rt_any_is_nil(RtAny value);
bool rt_any_is_int(RtAny value);
bool rt_any_is_long(RtAny value);
bool rt_any_is_int32(RtAny value);
bool rt_any_is_uint(RtAny value);
bool rt_any_is_uint32(RtAny value);
bool rt_any_is_double(RtAny value);
bool rt_any_is_float(RtAny value);
bool rt_any_is_string(RtAny value);
bool rt_any_is_char(RtAny value);
bool rt_any_is_bool(RtAny value);
bool rt_any_is_byte(RtAny value);
bool rt_any_is_array(RtAny value);
bool rt_any_is_function(RtAny value);
bool rt_any_is_struct_type(RtAny value, int expected_type_id);

/* Get type tag */
RtAnyTag rt_any_get_tag(RtAny value);

/* Get type name as string (for error messages and debugging) */
const char *rt_any_type_name(RtAny value);
const char *rt_any_tag_name(RtAnyTag tag);

/* ============================================================================
 * Comparison Functions
 * ============================================================================ */

/* Compare two any values for equality */
bool rt_any_equals(RtAny a, RtAny b);

/* Compare type tags */
bool rt_any_same_type(RtAny a, RtAny b);

/* ============================================================================
 * Utility Functions
 * ============================================================================ */

/* Convert any value to string representation (for debugging) */
char *rt_any_to_string(RtArenaV2 *arena, RtAny value);

/* Promote an any value's heap-allocated data to a target arena.
 * Used when returning any values from functions to ensure data survives
 * the destruction of the function's local arena. */
RtAny rt_any_promote(RtArenaV2 *target_arena, RtAny value);

/* V2 version: Promote an any value's heap-allocated data to a target arena.
 * For V2 arena mode - strings are RtHandleV2* not raw char*. */
struct RtArenaV2;
RtAny rt_any_promote_v2(struct RtArenaV2 *target_arena, RtAny value);

#endif /* RUNTIME_ANY_H */
