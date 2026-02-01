/**
 * code_gen_expr_thread_util.c - Thread code generation utility functions
 *
 * Contains helper functions for thread spawn/sync code generation.
 */

#include "code_gen/expr/thread/code_gen_expr_thread_util.h"

const char *get_rt_result_type(Type *type)
{
    if (type == NULL || type->kind == TYPE_VOID)
    {
        return "RT_TYPE_VOID";
    }

    switch (type->kind)
    {
        case TYPE_INT:
        case TYPE_INT32:
        case TYPE_UINT32:
            return "RT_TYPE_INT";
        case TYPE_LONG:
        case TYPE_UINT:
            return "RT_TYPE_LONG";
        case TYPE_DOUBLE:
        case TYPE_FLOAT:
            return "RT_TYPE_DOUBLE";
        case TYPE_BOOL:
            return "RT_TYPE_BOOL";
        case TYPE_BYTE:
            return "RT_TYPE_BYTE";
        case TYPE_CHAR:
            return "RT_TYPE_CHAR";
        case TYPE_STRING:
            return "RT_TYPE_STRING";
        case TYPE_STRUCT:
            return "RT_TYPE_STRUCT";
        case TYPE_ARRAY:
        {
            Type *elem = type->as.array.element_type;
            switch (elem->kind)
            {
                case TYPE_INT:
                case TYPE_INT32:
                case TYPE_UINT32:
                    return "RT_TYPE_ARRAY_INT";
                case TYPE_LONG:
                case TYPE_UINT:
                    return "RT_TYPE_ARRAY_LONG";
                case TYPE_DOUBLE:
                case TYPE_FLOAT:
                    return "RT_TYPE_ARRAY_DOUBLE";
                case TYPE_BOOL:
                    return "RT_TYPE_ARRAY_BOOL";
                case TYPE_BYTE:
                    return "RT_TYPE_ARRAY_BYTE";
                case TYPE_CHAR:
                    return "RT_TYPE_ARRAY_CHAR";
                case TYPE_STRING:
                    return "RT_TYPE_ARRAY_STRING";
                case TYPE_STRUCT:
                    return "RT_TYPE_ARRAY_HANDLE";
                case TYPE_ARRAY: {
                    /* 2D/3D+ arrays: outer array contains RtHandle elements
                     * Check if inner element type is string or another array */
                    Type *inner_elem = elem->as.array.element_type;
                    if (inner_elem != NULL && inner_elem->kind == TYPE_STRING) {
                        /* str[][] needs deepest promotion for nested string handles */
                        return "RT_TYPE_ARRAY2_STRING";
                    }
                    if (inner_elem != NULL && inner_elem->kind == TYPE_ARRAY) {
                        /* 3D arrays - check if innermost is string */
                        Type *innermost = inner_elem->as.array.element_type;
                        if (innermost != NULL && innermost->kind == TYPE_STRING) {
                            /* str[][][] needs three levels of string promotion */
                            return "RT_TYPE_ARRAY3_STRING";
                        }
                        /* Other 3D arrays need extra depth of handle promotion */
                        return "RT_TYPE_ARRAY_HANDLE_3D";
                    }
                    /* 2D arrays use RT_TYPE_ARRAY_HANDLE for deep promotion */
                    return "RT_TYPE_ARRAY_HANDLE";
                }
                case TYPE_ANY:
                    /* any[] arrays contain RtAny elements */
                    return "RT_TYPE_ARRAY_ANY";
                default:
                    return "RT_TYPE_VOID";
            }
        }
        default:
            return "RT_TYPE_VOID";
    }
}
