/*
 * code_gen_stmt_var_init.c - Variable initialization helpers
 *
 * Handles array-to-any conversions and other initialization helpers
 * for variable declarations.
 */

#include "code_gen/stmt/code_gen_stmt.h"
#include "code_gen/expr/code_gen_expr.h"
#include "code_gen/util/code_gen_util.h"
#include "debug.h"
#include "symbol_table.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* Convert a typed array to any[] using the appropriate conversion function.
 * Returns the converted expression string, or NULL if no conversion needed. */
char *code_gen_array_to_any_1d(CodeGen *gen, Type *src_elem, const char *init_str)
{
    const char *conv_func = NULL;
    switch (src_elem->kind)
    {
    case TYPE_INT:
    case TYPE_INT32:
    case TYPE_UINT:
    case TYPE_UINT32:
    case TYPE_LONG:
        conv_func = "rt_array_to_any_long";
        break;
    case TYPE_DOUBLE:
    case TYPE_FLOAT:
        conv_func = "rt_array_to_any_double";
        break;
    case TYPE_CHAR:
        conv_func = "rt_array_to_any_char";
        break;
    case TYPE_BOOL:
        conv_func = "rt_array_to_any_bool";
        break;
    case TYPE_BYTE:
        conv_func = "rt_array_to_any_byte";
        break;
    case TYPE_STRING:
        conv_func = "rt_array_to_any_string";
        break;
    default:
        return NULL;
    }

    if (gen->current_arena_var != NULL)
    {
        /* V2 to_any functions take just the handle.
         * init_str should already be a handle expression. */
        return arena_sprintf(gen->arena, "%s_v2(%s)", conv_func, init_str);
    }
    else
    {
        return arena_sprintf(gen->arena, "%s(%s, %s)", conv_func, ARENA_VAR(gen), init_str);
    }
}

/* Convert a typed 2D array to any[][] using the appropriate conversion function.
 * Returns the converted expression string, or NULL if no conversion needed. */
char *code_gen_array_to_any_2d(CodeGen *gen, Type *inner_src, const char *init_str)
{
    const char *conv_func = NULL;
    switch (inner_src->kind)
    {
    case TYPE_INT:
    case TYPE_INT32:
    case TYPE_UINT:
    case TYPE_UINT32:
    case TYPE_LONG:
        conv_func = "rt_array2_to_any_long";
        break;
    case TYPE_DOUBLE:
    case TYPE_FLOAT:
        conv_func = "rt_array2_to_any_double";
        break;
    case TYPE_CHAR:
        conv_func = "rt_array2_to_any_char";
        break;
    case TYPE_BOOL:
        conv_func = "rt_array2_to_any_bool";
        break;
    case TYPE_BYTE:
        conv_func = "rt_array2_to_any_byte";
        break;
    case TYPE_STRING:
        conv_func = "rt_array2_to_any_string";
        break;
    default:
        return NULL;
    }

    if (gen->current_arena_var != NULL)
    {
        /* V2 to_any functions take just the handle */
        return arena_sprintf(gen->arena, "%s_v2(%s)", conv_func, init_str);
    }
    else
    {
        return arena_sprintf(gen->arena, "%s(%s, %s)", conv_func, ARENA_VAR(gen), init_str);
    }
}

/* Convert a typed 3D array to any[][][] using the appropriate conversion function.
 * Returns the converted expression string, or NULL if no conversion needed. */
char *code_gen_array_to_any_3d(CodeGen *gen, Type *innermost_src, const char *init_str)
{
    const char *conv_func = NULL;
    switch (innermost_src->kind)
    {
    case TYPE_INT:
    case TYPE_INT32:
    case TYPE_UINT:
    case TYPE_UINT32:
    case TYPE_LONG:
        conv_func = "rt_array3_to_any_long";
        break;
    case TYPE_DOUBLE:
    case TYPE_FLOAT:
        conv_func = "rt_array3_to_any_double";
        break;
    case TYPE_CHAR:
        conv_func = "rt_array3_to_any_char";
        break;
    case TYPE_BOOL:
        conv_func = "rt_array3_to_any_bool";
        break;
    case TYPE_BYTE:
        conv_func = "rt_array3_to_any_byte";
        break;
    case TYPE_STRING:
        conv_func = "rt_array3_to_any_string";
        break;
    default:
        return NULL;
    }

    if (gen->current_arena_var != NULL)
    {
        /* V2 to_any functions take just the handle */
        return arena_sprintf(gen->arena, "%s_v2(%s)", conv_func, init_str);
    }
    else
    {
        return arena_sprintf(gen->arena, "%s(%s, %s)", conv_func, ARENA_VAR(gen), init_str);
    }
}

/* Check if an array type has 'any' as its element type at any nesting level.
 * Used to determine if special conversion logic is needed for thread spawns. */
bool is_any_element_array_type(Type *type)
{
    if (type == NULL || type->kind != TYPE_ARRAY)
        return false;

    Type *elem = type->as.array.element_type;
    if (elem == NULL)
        return false;

    /* Check 1D: any[] */
    if (elem->kind == TYPE_ANY)
        return true;

    /* Check 2D: any[][] */
    if (elem->kind == TYPE_ARRAY && elem->as.array.element_type != NULL &&
        elem->as.array.element_type->kind == TYPE_ANY)
        return true;

    /* Check 3D: any[][][] */
    if (elem->kind == TYPE_ARRAY && elem->as.array.element_type != NULL &&
        elem->as.array.element_type->kind == TYPE_ARRAY &&
        elem->as.array.element_type->as.array.element_type != NULL &&
        elem->as.array.element_type->as.array.element_type->kind == TYPE_ANY)
        return true;

    return false;
}

/* Handle array-to-any conversion for variable declarations.
 * Returns the converted init_str, or the original if no conversion needed. */
char *code_gen_var_array_conversion(CodeGen *gen, Type *decl_type, Type *src_type, char *init_str)
{
    if (decl_type->kind != TYPE_ARRAY || decl_type->as.array.element_type == NULL)
        return init_str;
    if (src_type == NULL || src_type->kind != TYPE_ARRAY || src_type->as.array.element_type == NULL)
        return init_str;

    Type *decl_elem = decl_type->as.array.element_type;
    Type *src_elem = src_type->as.array.element_type;

    /* Check for 3D array: any[][][] = T[][][] */
    if (decl_elem->kind == TYPE_ARRAY &&
        decl_elem->as.array.element_type != NULL &&
        decl_elem->as.array.element_type->kind == TYPE_ARRAY &&
        decl_elem->as.array.element_type->as.array.element_type != NULL &&
        decl_elem->as.array.element_type->as.array.element_type->kind == TYPE_ANY &&
        src_elem->kind == TYPE_ARRAY &&
        src_elem->as.array.element_type != NULL &&
        src_elem->as.array.element_type->kind == TYPE_ARRAY &&
        src_elem->as.array.element_type->as.array.element_type != NULL &&
        src_elem->as.array.element_type->as.array.element_type->kind != TYPE_ANY)
    {
        Type *innermost_src = src_elem->as.array.element_type->as.array.element_type;
        char *converted = code_gen_array_to_any_3d(gen, innermost_src, init_str);
        if (converted != NULL)
            return converted;
    }
    /* Check for 2D array: any[][] = T[][] */
    else if (decl_elem->kind == TYPE_ARRAY &&
             decl_elem->as.array.element_type != NULL &&
             decl_elem->as.array.element_type->kind == TYPE_ANY &&
             src_elem->kind == TYPE_ARRAY &&
             src_elem->as.array.element_type != NULL &&
             src_elem->as.array.element_type->kind != TYPE_ANY)
    {
        Type *inner_src = src_elem->as.array.element_type;
        char *converted = code_gen_array_to_any_2d(gen, inner_src, init_str);
        if (converted != NULL)
            return converted;
    }
    /* Check for 1D array: any[] = T[] */
    else if (decl_elem->kind == TYPE_ANY && src_elem->kind != TYPE_ANY)
    {
        char *converted = code_gen_array_to_any_1d(gen, src_elem, init_str);
        if (converted != NULL)
            return converted;
    }

    return init_str;
}
