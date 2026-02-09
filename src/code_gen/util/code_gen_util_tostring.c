#include "code_gen/util/code_gen_util.h"
#include "debug.h"
#include <stdlib.h>

const char *get_rt_to_string_func(TypeKind kind)
{
    DEBUG_VERBOSE("Entering get_rt_to_string_func");
    switch (kind)
    {
    case TYPE_INT:
    case TYPE_INT32:
    case TYPE_UINT:
    case TYPE_UINT32:
    case TYPE_LONG:
        return "rt_to_string_long";
    case TYPE_DOUBLE:
    case TYPE_FLOAT:
        return "rt_to_string_double";
    case TYPE_CHAR:
        return "rt_to_string_char";
    case TYPE_STRING:
        return "rt_to_string_string";
    case TYPE_BOOL:
        return "rt_to_string_bool";
    case TYPE_BYTE:
        return "rt_to_string_byte";
    case TYPE_VOID:
        return "rt_to_string_void";
    case TYPE_ANY:
        return "rt_any_to_string";
    case TYPE_NIL:
    case TYPE_ARRAY:
    case TYPE_FUNCTION:
    case TYPE_POINTER:
    case TYPE_STRUCT:
        /* For structs, fallback to pointer representation.
         * Proper struct-to-string conversion (via toString() method)
         * is handled at a higher level in code_gen_expr_string.c */
        return "rt_to_string_pointer";
    default:
        exit(1);
    }
    return NULL;
}

const char *get_rt_to_string_func_v2(TypeKind kind)
{
    /* V2 raw pointer versions - for use with RtArenaV2* */
    switch (kind)
    {
    case TYPE_INT:
    case TYPE_INT32:
    case TYPE_UINT:
    case TYPE_UINT32:
    case TYPE_LONG:
        return "rt_to_string_long_raw_v2";
    case TYPE_DOUBLE:
    case TYPE_FLOAT:
        return "rt_to_string_double_raw_v2";
    case TYPE_CHAR:
        return "rt_to_string_char_raw_v2";
    case TYPE_STRING:
        return "rt_to_string_string";  /* Strings don't need conversion */
    case TYPE_BOOL:
        return "rt_to_string_bool_raw_v2";
    case TYPE_BYTE:
        return "rt_to_string_byte_raw_v2";
    default:
        return get_rt_to_string_func(kind);  /* Fallback to V1 */
    }
}

const char *get_rt_to_string_func_for_type_v2(Type *type)
{
    /* V2 versions for all types */
    if (type == NULL) return "rt_to_string_pointer";

    /* For simple types, use V2 raw functions */
    if (type->kind == TYPE_INT || type->kind == TYPE_INT32 ||
        type->kind == TYPE_UINT || type->kind == TYPE_UINT32 ||
        type->kind == TYPE_LONG || type->kind == TYPE_DOUBLE ||
        type->kind == TYPE_FLOAT || type->kind == TYPE_CHAR ||
        type->kind == TYPE_BOOL || type->kind == TYPE_BYTE)
    {
        return get_rt_to_string_func_v2(type->kind);
    }

    /* Handle arrays - V2 functions take handles directly */
    if (type->kind == TYPE_ARRAY && type->as.array.element_type != NULL)
    {
        Type *elem_type = type->as.array.element_type;
        TypeKind elem_kind = elem_type->kind;

        /* 2D arrays */
        if (elem_kind == TYPE_ARRAY && elem_type->as.array.element_type != NULL)
        {
            TypeKind inner_kind = elem_type->as.array.element_type->kind;
            switch (inner_kind)
            {
            case TYPE_INT:
            case TYPE_INT32:
            case TYPE_UINT:
            case TYPE_UINT32:
            case TYPE_LONG:
                return "rt_to_string_array2_long_v2";
            case TYPE_DOUBLE:
            case TYPE_FLOAT:
                return "rt_to_string_array2_double_v2";
            case TYPE_CHAR:
                return "rt_to_string_array2_char_v2";
            case TYPE_BOOL:
                return "rt_to_string_array2_bool_v2";
            case TYPE_BYTE:
                return "rt_to_string_array2_byte_v2";
            case TYPE_STRING:
                return "rt_to_string_array2_string_v2";
            case TYPE_ANY:
                return "rt_to_string_array2_any_v2";
            case TYPE_ARRAY: {
                /* 3D array: select formatter based on innermost element type */
                Type *innermost = elem_type->as.array.element_type->as.array.element_type;
                if (innermost != NULL) {
                    switch (innermost->kind) {
                    case TYPE_INT:
                    case TYPE_INT32:
                    case TYPE_UINT:
                    case TYPE_UINT32:
                    case TYPE_LONG:
                        return "rt_to_string_array3_long_v2";
                    case TYPE_DOUBLE:
                    case TYPE_FLOAT:
                        return "rt_to_string_array3_double_v2";
                    case TYPE_CHAR:
                        return "rt_to_string_array3_char_v2";
                    case TYPE_BOOL:
                        return "rt_to_string_array3_bool_v2";
                    case TYPE_BYTE:
                        return "rt_to_string_array3_byte_v2";
                    case TYPE_STRING:
                        return "rt_to_string_array3_string_v2";
                    case TYPE_ANY:
                        return "rt_to_string_array3_any_v2";
                    default:
                        return "rt_to_string_pointer";
                    }
                }
                return "rt_to_string_pointer";
            }
            default:
                return "rt_to_string_pointer";
            }
        }

        /* 1D arrays */
        switch (elem_kind)
        {
        case TYPE_INT:
        case TYPE_LONG:
            return "rt_to_string_array_long_v2";
        case TYPE_INT32:
            return "rt_to_string_array_int32_v2";
        case TYPE_UINT:
            return "rt_to_string_array_uint_v2";
        case TYPE_UINT32:
            return "rt_to_string_array_uint32_v2";
        case TYPE_DOUBLE:
            return "rt_to_string_array_double_v2";
        case TYPE_FLOAT:
            return "rt_to_string_array_float_v2";
        case TYPE_CHAR:
            return "rt_to_string_array_char_v2";
        case TYPE_BOOL:
            return "rt_to_string_array_bool_v2";
        case TYPE_BYTE:
            return "rt_to_string_array_byte_v2";
        case TYPE_STRING:
            return "rt_to_string_array_string_v2";
        case TYPE_ANY:
            return "rt_to_string_array_any_v2";
        default:
            return "rt_to_string_pointer";
        }
    }

    /* Non-arrays: use base functions */
    return get_rt_to_string_func(type->kind);
}

const char *get_rt_to_string_func_for_type(Type *type)
{
    DEBUG_VERBOSE("Entering get_rt_to_string_func_for_type");
    if (type == NULL) return "rt_to_string_pointer";

    /* Handle arrays specially - need to look at element type */
    if (type->kind == TYPE_ARRAY && type->as.array.element_type != NULL)
    {
        Type *elem_type = type->as.array.element_type;
        TypeKind elem_kind = elem_type->kind;

        /* Check for nested arrays (2D arrays) */
        if (elem_kind == TYPE_ARRAY && elem_type->as.array.element_type != NULL)
        {
            Type *inner_type = elem_type->as.array.element_type;
            TypeKind inner_kind = inner_type->kind;

            /* Check for 3D arrays */
            if (inner_kind == TYPE_ARRAY && inner_type->as.array.element_type != NULL)
            {
                TypeKind innermost_kind = inner_type->as.array.element_type->kind;
                /* Currently only support 3D any arrays */
                if (innermost_kind == TYPE_ANY)
                {
                    return "rt_to_string_array3_any";
                }
                /* 3D+ arrays of other types - fallback to pointer */
                return "rt_to_string_pointer";
            }

            switch (inner_kind)
            {
            case TYPE_INT:
            case TYPE_INT32:
            case TYPE_UINT:
            case TYPE_UINT32:
            case TYPE_LONG:
                return "rt_to_string_array2_long";
            case TYPE_DOUBLE:
            case TYPE_FLOAT:
                return "rt_to_string_array2_double";
            case TYPE_CHAR:
                return "rt_to_string_array2_char";
            case TYPE_BOOL:
                return "rt_to_string_array2_bool";
            case TYPE_BYTE:
                return "rt_to_string_array2_byte";
            case TYPE_STRING:
                return "rt_to_string_array2_string";
            case TYPE_ANY:
                return "rt_to_string_array2_any";
            default:
                /* Other nested types - fallback to pointer */
                return "rt_to_string_pointer";
            }
        }

        /* 1D arrays */
        switch (elem_kind)
        {
        case TYPE_INT:
        case TYPE_INT32:
        case TYPE_UINT:
        case TYPE_UINT32:
        case TYPE_LONG:
            return "rt_to_string_array_long";
        case TYPE_DOUBLE:
        case TYPE_FLOAT:
            return "rt_to_string_array_double";
        case TYPE_CHAR:
            return "rt_to_string_array_char";
        case TYPE_BOOL:
            return "rt_to_string_array_bool";
        case TYPE_BYTE:
            return "rt_to_string_array_byte";
        case TYPE_STRING:
            return "rt_to_string_array_string";
        case TYPE_ANY:
            return "rt_to_string_array_any";
        default:
            /* Other complex element types - fallback to pointer */
            return "rt_to_string_pointer";
        }
    }

    /* For non-arrays, use the existing function */
    return get_rt_to_string_func(type->kind);
}

const char *get_default_value(Type *type)
{
    DEBUG_VERBOSE("Entering get_default_value");
    if (type->kind == TYPE_STRING || type->kind == TYPE_ARRAY)
    {
        return "NULL";
    }
    else if (type->kind == TYPE_ANY)
    {
        return "rt_box_nil()";
    }
    else if (type->kind == TYPE_STRUCT)
    {
        /* Native structs with c_alias are treated as opaque handle pointer types.
         * Return NULL for pointer types instead of {0}. */
        if (type->as.struct_type.is_native && type->as.struct_type.c_alias != NULL)
        {
            return "NULL";
        }
        /* Regular struct default: use C99 compound literal with zeroed fields.
         * This creates a value-initialized struct at runtime. */
        return "{0}";
    }
    else
    {
        return "0";
    }
}
