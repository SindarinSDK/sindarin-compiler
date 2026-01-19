#include "runtime_any.h"
#include "runtime_string.h"
#include "runtime_array.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ============================================================================
 * Boxing Functions
 * ============================================================================ */

RtAny rt_box_nil(void) {
    RtAny result;
    result.tag = RT_ANY_NIL;
    result.value.obj = NULL;
    result.element_tag = RT_ANY_NIL;
    return result;
}

RtAny rt_box_int(int64_t value) {
    RtAny result;
    result.tag = RT_ANY_INT;
    result.value.i64 = value;
    result.element_tag = RT_ANY_NIL;
    return result;
}

RtAny rt_box_long(int64_t value) {
    RtAny result;
    result.tag = RT_ANY_LONG;
    result.value.i64 = value;
    result.element_tag = RT_ANY_NIL;
    return result;
}

RtAny rt_box_int32(int32_t value) {
    RtAny result;
    result.tag = RT_ANY_INT32;
    result.value.i32 = value;
    result.element_tag = RT_ANY_NIL;
    return result;
}

RtAny rt_box_uint(uint64_t value) {
    RtAny result;
    result.tag = RT_ANY_UINT;
    result.value.u64 = value;
    result.element_tag = RT_ANY_NIL;
    return result;
}

RtAny rt_box_uint32(uint32_t value) {
    RtAny result;
    result.tag = RT_ANY_UINT32;
    result.value.u32 = value;
    result.element_tag = RT_ANY_NIL;
    return result;
}

RtAny rt_box_double(double value) {
    RtAny result;
    result.tag = RT_ANY_DOUBLE;
    result.value.d = value;
    result.element_tag = RT_ANY_NIL;
    return result;
}

RtAny rt_box_float(float value) {
    RtAny result;
    result.tag = RT_ANY_FLOAT;
    result.value.f = value;
    result.element_tag = RT_ANY_NIL;
    return result;
}

RtAny rt_box_string(const char *value) {
    RtAny result;
    result.tag = RT_ANY_STRING;
    result.value.s = (char *)value;
    result.element_tag = RT_ANY_NIL;
    return result;
}

RtAny rt_box_char(char value) {
    RtAny result;
    result.tag = RT_ANY_CHAR;
    result.value.c = value;
    result.element_tag = RT_ANY_NIL;
    return result;
}

RtAny rt_box_bool(bool value) {
    RtAny result;
    result.tag = RT_ANY_BOOL;
    result.value.b = value;
    result.element_tag = RT_ANY_NIL;
    return result;
}

RtAny rt_box_byte(uint8_t value) {
    RtAny result;
    result.tag = RT_ANY_BYTE;
    result.value.byte = value;
    result.element_tag = RT_ANY_NIL;
    return result;
}

RtAny rt_box_array(void *arr, RtAnyTag element_tag) {
    RtAny result;
    result.tag = RT_ANY_ARRAY;
    result.value.arr = arr;
    result.element_tag = element_tag;
    return result;
}

RtAny rt_box_function(void *fn) {
    RtAny result;
    result.tag = RT_ANY_FUNCTION;
    result.value.fn = fn;
    result.element_tag = RT_ANY_NIL;
    return result;
}

RtAny rt_box_struct(RtArena *arena, void *struct_data, size_t struct_size, int struct_type_id) {
    RtAny result;
    result.tag = RT_ANY_STRUCT;
    /* Allocate space in the arena and copy struct data */
    void *copy = rt_arena_alloc(arena, struct_size);
    memcpy(copy, struct_data, struct_size);
    result.value.obj = copy;
    /* Store struct type ID in element_tag (repurposed for structs) */
    result.element_tag = (RtAnyTag)struct_type_id;
    return result;
}

/* ============================================================================
 * Unboxing Functions
 * ============================================================================ */

static void rt_any_type_error(const char *expected, RtAny value) {
    fprintf(stderr, "Type error: expected %s, got %s\n",
            expected, rt_any_type_name(value));
    exit(1);
}

int64_t rt_unbox_int(RtAny value) {
    if (value.tag != RT_ANY_INT) {
        rt_any_type_error("int", value);
    }
    return value.value.i64;
}

int64_t rt_unbox_long(RtAny value) {
    if (value.tag != RT_ANY_LONG) {
        rt_any_type_error("long", value);
    }
    return value.value.i64;
}

int32_t rt_unbox_int32(RtAny value) {
    if (value.tag != RT_ANY_INT32) {
        rt_any_type_error("int32", value);
    }
    return value.value.i32;
}

uint64_t rt_unbox_uint(RtAny value) {
    if (value.tag != RT_ANY_UINT) {
        rt_any_type_error("uint", value);
    }
    return value.value.u64;
}

uint32_t rt_unbox_uint32(RtAny value) {
    if (value.tag != RT_ANY_UINT32) {
        rt_any_type_error("uint32", value);
    }
    return value.value.u32;
}

double rt_unbox_double(RtAny value) {
    if (value.tag != RT_ANY_DOUBLE) {
        rt_any_type_error("double", value);
    }
    return value.value.d;
}

float rt_unbox_float(RtAny value) {
    if (value.tag != RT_ANY_FLOAT) {
        rt_any_type_error("float", value);
    }
    return value.value.f;
}

const char *rt_unbox_string(RtAny value) {
    if (value.tag != RT_ANY_STRING) {
        rt_any_type_error("str", value);
    }
    return value.value.s;
}

char rt_unbox_char(RtAny value) {
    if (value.tag != RT_ANY_CHAR) {
        rt_any_type_error("char", value);
    }
    return value.value.c;
}

bool rt_unbox_bool(RtAny value) {
    if (value.tag != RT_ANY_BOOL) {
        rt_any_type_error("bool", value);
    }
    return value.value.b;
}

uint8_t rt_unbox_byte(RtAny value) {
    if (value.tag != RT_ANY_BYTE) {
        rt_any_type_error("byte", value);
    }
    return value.value.byte;
}

void *rt_unbox_array(RtAny value) {
    if (value.tag != RT_ANY_ARRAY) {
        rt_any_type_error("array", value);
    }
    return value.value.arr;
}

void *rt_unbox_function(RtAny value) {
    if (value.tag != RT_ANY_FUNCTION) {
        rt_any_type_error("function", value);
    }
    return value.value.fn;
}

void *rt_unbox_struct(RtAny value, int expected_type_id) {
    if (value.tag != RT_ANY_STRUCT) {
        rt_any_type_error("struct", value);
    }
    /* Check struct type ID matches */
    int actual_type_id = (int)value.element_tag;
    if (actual_type_id != expected_type_id) {
        fprintf(stderr, "Type error: struct type mismatch (expected type id %d, got %d)\n",
                expected_type_id, actual_type_id);
        exit(1);
    }
    return value.value.obj;
}

/* ============================================================================
 * Type Checking Functions
 * ============================================================================ */

bool rt_any_is_nil(RtAny value) { return value.tag == RT_ANY_NIL; }
bool rt_any_is_int(RtAny value) { return value.tag == RT_ANY_INT; }
bool rt_any_is_long(RtAny value) { return value.tag == RT_ANY_LONG; }
bool rt_any_is_int32(RtAny value) { return value.tag == RT_ANY_INT32; }
bool rt_any_is_uint(RtAny value) { return value.tag == RT_ANY_UINT; }
bool rt_any_is_uint32(RtAny value) { return value.tag == RT_ANY_UINT32; }
bool rt_any_is_double(RtAny value) { return value.tag == RT_ANY_DOUBLE; }
bool rt_any_is_float(RtAny value) { return value.tag == RT_ANY_FLOAT; }
bool rt_any_is_string(RtAny value) { return value.tag == RT_ANY_STRING; }
bool rt_any_is_char(RtAny value) { return value.tag == RT_ANY_CHAR; }
bool rt_any_is_bool(RtAny value) { return value.tag == RT_ANY_BOOL; }
bool rt_any_is_byte(RtAny value) { return value.tag == RT_ANY_BYTE; }
bool rt_any_is_array(RtAny value) { return value.tag == RT_ANY_ARRAY; }
bool rt_any_is_function(RtAny value) { return value.tag == RT_ANY_FUNCTION; }

bool rt_any_is_struct_type(RtAny value, int expected_type_id) {
    if (value.tag != RT_ANY_STRUCT) {
        return false;
    }
    /* Check if the struct type ID matches */
    int actual_type_id = (int)value.element_tag;
    return actual_type_id == expected_type_id;
}

RtAnyTag rt_any_get_tag(RtAny value) {
    return value.tag;
}

const char *rt_any_tag_name(RtAnyTag tag) {
    switch (tag) {
        case RT_ANY_NIL: return "nil";
        case RT_ANY_INT: return "int";
        case RT_ANY_LONG: return "long";
        case RT_ANY_INT32: return "int32";
        case RT_ANY_UINT: return "uint";
        case RT_ANY_UINT32: return "uint32";
        case RT_ANY_DOUBLE: return "double";
        case RT_ANY_FLOAT: return "float";
        case RT_ANY_STRING: return "str";
        case RT_ANY_CHAR: return "char";
        case RT_ANY_BOOL: return "bool";
        case RT_ANY_BYTE: return "byte";
        case RT_ANY_ARRAY: return "array";
        case RT_ANY_FUNCTION: return "function";
        case RT_ANY_STRUCT: return "struct";
        default: return "unknown";
    }
}

const char *rt_any_type_name(RtAny value) {
    return rt_any_tag_name(value.tag);
}

/* ============================================================================
 * Comparison Functions
 * ============================================================================ */

bool rt_any_same_type(RtAny a, RtAny b) {
    return a.tag == b.tag;
}

bool rt_any_equals(RtAny a, RtAny b) {
    /* Different types are never equal */
    if (a.tag != b.tag) {
        return false;
    }

    switch (a.tag) {
        case RT_ANY_NIL:
            return true;
        case RT_ANY_INT:
        case RT_ANY_LONG:
            return a.value.i64 == b.value.i64;
        case RT_ANY_INT32:
            return a.value.i32 == b.value.i32;
        case RT_ANY_UINT:
            return a.value.u64 == b.value.u64;
        case RT_ANY_UINT32:
            return a.value.u32 == b.value.u32;
        case RT_ANY_DOUBLE:
            return a.value.d == b.value.d;
        case RT_ANY_FLOAT:
            return a.value.f == b.value.f;
        case RT_ANY_STRING:
            if (a.value.s == NULL && b.value.s == NULL) return true;
            if (a.value.s == NULL || b.value.s == NULL) return false;
            return strcmp(a.value.s, b.value.s) == 0;
        case RT_ANY_CHAR:
            return a.value.c == b.value.c;
        case RT_ANY_BOOL:
            return a.value.b == b.value.b;
        case RT_ANY_BYTE:
            return a.value.byte == b.value.byte;
        case RT_ANY_ARRAY: {
            /* Compare arrays element by element */
            if (a.value.arr == NULL && b.value.arr == NULL) return true;
            if (a.value.arr == NULL || b.value.arr == NULL) return false;
            size_t len_a = rt_array_length(a.value.arr);
            size_t len_b = rt_array_length(b.value.arr);
            if (len_a != len_b) return false;
            /* For any[] arrays, compare element by element */
            if (a.element_tag == RT_ANY_NIL) {
                /* This is an any[] array */
                RtAny *arr_a = (RtAny *)a.value.arr;
                RtAny *arr_b = (RtAny *)b.value.arr;
                for (size_t i = 0; i < len_a; i++) {
                    if (!rt_any_equals(arr_a[i], arr_b[i])) {
                        return false;
                    }
                }
                return true;
            }
            /* For typed arrays, just compare pointers for now */
            return a.value.arr == b.value.arr;
        }
        case RT_ANY_FUNCTION:
            return a.value.fn == b.value.fn;
        case RT_ANY_STRUCT:
            /* For structs, compare type ID and pointer (identity comparison) */
            if (a.element_tag != b.element_tag) {
                return false;  /* Different struct types */
            }
            return a.value.obj == b.value.obj;
        default:
            return false;
    }
}

/* ============================================================================
 * Utility Functions
 * ============================================================================ */

char *rt_any_to_string(RtArena *arena, RtAny value) {
    char buffer[256];

    switch (value.tag) {
        case RT_ANY_NIL:
            return rt_arena_strdup(arena, "nil");
        case RT_ANY_INT:
        case RT_ANY_LONG:
            snprintf(buffer, sizeof(buffer), "%lld", (long long)value.value.i64);
            return rt_arena_strdup(arena, buffer);
        case RT_ANY_INT32:
            snprintf(buffer, sizeof(buffer), "%d", value.value.i32);
            return rt_arena_strdup(arena, buffer);
        case RT_ANY_UINT:
            snprintf(buffer, sizeof(buffer), "%llu", (unsigned long long)value.value.u64);
            return rt_arena_strdup(arena, buffer);
        case RT_ANY_UINT32:
            snprintf(buffer, sizeof(buffer), "%u", value.value.u32);
            return rt_arena_strdup(arena, buffer);
        case RT_ANY_DOUBLE:
            snprintf(buffer, sizeof(buffer), "%g", value.value.d);
            return rt_arena_strdup(arena, buffer);
        case RT_ANY_FLOAT:
            snprintf(buffer, sizeof(buffer), "%g", (double)value.value.f);
            return rt_arena_strdup(arena, buffer);
        case RT_ANY_STRING:
            if (value.value.s) {
                size_t len = strlen(value.value.s);
                char *result = rt_arena_alloc(arena, len + 3);  /* "str" + null */
                result[0] = '"';
                memcpy(result + 1, value.value.s, len);
                result[len + 1] = '"';
                result[len + 2] = '\0';
                return result;
            }
            return rt_arena_strdup(arena, "null");
        case RT_ANY_CHAR:
            snprintf(buffer, sizeof(buffer), "%c", value.value.c);
            return rt_arena_strdup(arena, buffer);
        case RT_ANY_BOOL:
            return rt_arena_strdup(arena, value.value.b ? "true" : "false");
        case RT_ANY_BYTE:
            snprintf(buffer, sizeof(buffer), "%u", value.value.byte);
            return rt_arena_strdup(arena, buffer);
        case RT_ANY_ARRAY:
            snprintf(buffer, sizeof(buffer), "[array of %zu elements]",
                    value.value.arr ? rt_array_length(value.value.arr) : 0);
            return rt_arena_strdup(arena, buffer);
        case RT_ANY_FUNCTION:
            return rt_arena_strdup(arena, "[function]");
        case RT_ANY_STRUCT:
            return rt_arena_strdup(arena, "[struct]");
        default:
            return rt_arena_strdup(arena, "[unknown]");
    }
}

/* ============================================================================
 * Arena Promotion for Any Values
 * ============================================================================
 * Promotes an any value's heap-allocated data to a target arena.
 * This is needed when returning any values from functions, as the function's
 * local arena will be destroyed after return.
 */

RtAny rt_any_promote(RtArena *target_arena, RtAny value) {
    RtAny result = value;
    
    switch (value.tag) {
        case RT_ANY_STRING:
            /* Strings need to be copied to the target arena */
            if (value.value.s != NULL) {
                result.value.s = rt_arena_strdup(target_arena, value.value.s);
            }
            break;
            
        case RT_ANY_ARRAY:
            /* Arrays need deep cloning - for now just copy pointer
             * TODO: implement proper array cloning for any[] */
            break;
            
        /* Primitive types don't need promotion */
        case RT_ANY_NIL:
        case RT_ANY_INT:
        case RT_ANY_LONG:
        case RT_ANY_INT32:
        case RT_ANY_UINT:
        case RT_ANY_UINT32:
        case RT_ANY_DOUBLE:
        case RT_ANY_FLOAT:
        case RT_ANY_CHAR:
        case RT_ANY_BOOL:
        case RT_ANY_BYTE:
            break;
            
        /* Object types - shallow copy for now */
        case RT_ANY_FUNCTION:
            break;

        case RT_ANY_STRUCT:
            /* Structs are arena-allocated; for now shallow copy pointer.
             * Full deep copy would require storing struct size metadata. */
            break;

        default:
            break;
    }
    
    return result;
}
