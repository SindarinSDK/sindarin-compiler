#include "code_gen/expr/code_gen_expr.h"
#include "code_gen/util/code_gen_util.h"
#include "code_gen/stmt/code_gen_stmt.h"
#include "symbol_table.h"
#include "debug.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <limits.h>

/* Forward declaration for range expression (defined in code_gen_expr.c) */
char *code_gen_range_expression(CodeGen *gen, Expr *expr);

/* Check if an expression is provably non-negative (for array index optimization).
 * Returns true for:
 *   - Integer literals >= 0
 *   - Long literals >= 0
 *   - Variables that are tracked as loop counters (provably non-negative)
 * Returns false for negative literals, untracked variables, and all other expressions.
 */
bool is_provably_non_negative(CodeGen *gen, Expr *expr)
{
    if (expr == NULL) return false;

    /* Check for non-negative integer/long literals */
    if (expr->type == EXPR_LITERAL)
    {
        if (expr->as.literal.type == NULL) return false;

        if (expr->as.literal.type->kind == TYPE_INT ||
            expr->as.literal.type->kind == TYPE_LONG)
        {
            return expr->as.literal.value.int_value >= 0;
        }
        /* Other literal types (double, bool, etc.) are not valid array indices */
        return false;
    }

    /* Check for loop counter variables (provably non-negative) */
    if (expr->type == EXPR_VARIABLE)
    {
        char *var_name = get_var_name(gen->arena, expr->as.variable.name);
        return is_tracked_loop_counter(gen, var_name);
    }

    /* All other expression types are not provably non-negative */
    return false;
}

char *code_gen_array_expression(CodeGen *gen, Expr *e)
{
    ArrayExpr *arr = &e->as.array;
    DEBUG_VERBOSE("Entering code_gen_array_expression");
    Type *arr_type = e->expr_type;
    if (arr_type->kind != TYPE_ARRAY) {
        fprintf(stderr, "Error: Expected array type\n");
        exit(1);
    }
    Type *elem_type = arr_type->as.array.element_type;
    const char *elem_c = get_c_type(gen->arena, elem_type);

    // Check if we have any spread or range elements
    bool has_complex = false;
    for (int i = 0; i < arr->element_count; i++) {
        if (arr->elements[i]->type == EXPR_SPREAD || arr->elements[i]->type == EXPR_RANGE) {
            has_complex = true;
            break;
        }
    }

    // Handle any[] arrays specially - need to box each element
    // Use push-based approach to avoid TCC limitations with compound literals of structs
    if (elem_type->kind == TYPE_ANY)
    {
        // For any[] arrays, box each element according to its actual type
        char *pushes = arena_strdup(gen->arena, "");
        for (int i = 0; i < arr->element_count; i++) {
            Expr *elem = arr->elements[i];
            char *el = code_gen_expression(gen, elem);
            // Box the element based on its actual type
            if (elem->expr_type != NULL && elem->expr_type->kind != TYPE_ANY) {
                el = code_gen_box_value(gen, el, elem->expr_type);
            }
            pushes = arena_sprintf(gen->arena, "%s _arr = rt_array_push_any_v2(%s, _arr, %s);",
                                   pushes, ARENA_VAR(gen), el);
        }
        return arena_sprintf(gen->arena, "({ RtHandleV2 *_arr = NULL;%s _arr; })", pushes);
    }

    // Determine the runtime function suffix based on element type
    const char *suffix = NULL;
    switch (elem_type->kind) {
        case TYPE_INT:
        case TYPE_LONG: suffix = "long"; break;
        case TYPE_INT32: suffix = "int32"; break;
        case TYPE_UINT: suffix = "uint"; break;
        case TYPE_UINT32: suffix = "uint32"; break;
        case TYPE_FLOAT: suffix = "float"; break;
        case TYPE_DOUBLE: suffix = "double"; break;
        case TYPE_CHAR: suffix = "char"; break;
        case TYPE_BOOL: suffix = "bool"; break;
        case TYPE_BYTE: suffix = "byte"; break;
        case TYPE_STRING: suffix = "string"; break;
        case TYPE_ARRAY: suffix = "ptr"; break;  /* Nested arrays use pointer arrays */
        default: suffix = NULL; break;
    }

    // If we have spread or range elements, generate concatenation code
    if (has_complex && suffix != NULL) {
        char *result = NULL;

        /* String arrays: elements are RtHandle values in memory,
         * so we must use _h variants directly. */
        bool string_handle_mode = (elem_type->kind == TYPE_STRING);

        for (int i = 0; i < arr->element_count; i++) {
            Expr *elem = arr->elements[i];
            char *elem_str;

            if (elem->type == EXPR_SPREAD) {
                if (string_handle_mode) {
                    /* String arrays: get handle and clone directly */
                    char *arr_h = code_gen_expression(gen, elem->as.spread.array);
                    elem_str = arena_sprintf(gen->arena, "rt_array_clone_string_v2(%s)", arr_h);
                } else {
                    /* Non-string types: get handle and clone using generic */
                    char *arr_h = code_gen_expression(gen, elem->as.spread.array);
                    const char *sizeof_expr = get_c_sizeof_elem(gen->arena, elem_type);
                    elem_str = arena_sprintf(gen->arena, "rt_array_clone_v2(%s, %s)", arr_h, sizeof_expr);
                }
            } else if (elem->type == EXPR_RANGE) {
                // Range: concat the range result
                elem_str = code_gen_range_expression(gen, elem);
            } else {
                if (string_handle_mode) {
                    // Get RtHandleV2* value; create_ptr_v2 stores handles directly
                    char *val = code_gen_expression(gen, elem);
                    elem_str = arena_sprintf(gen->arena,
                        "rt_array_create_ptr_v2(%s, 1, (RtHandleV2 *[]){%s})",
                        ARENA_VAR(gen), val);
                } else {
                    // Non-string: create single-element array using generic
                    char *val = code_gen_expression(gen, elem);
                    const char *literal_type;
                    if (elem_type->kind == TYPE_BOOL) {
                        literal_type = "int";
                    } else if (elem_type->kind == TYPE_ARRAY) {
                        literal_type = "RtHandleV2 *";
                    } else {
                        literal_type = elem_c;
                    }
                    if (elem_type->kind == TYPE_ARRAY) {
                        elem_str = arena_sprintf(gen->arena, "rt_array_create_ptr_v2(%s, 1, (RtHandleV2 *[]){%s})",
                                                ARENA_VAR(gen), val);
                    } else if (elem_type->kind == TYPE_STRUCT && struct_has_handle_fields(elem_type)) {
                        code_gen_ensure_struct_callbacks(gen, elem_type);
                        const char *sn_name = elem_type->as.struct_type.name
                            ? elem_type->as.struct_type.name : literal_type;
                        elem_str = arena_sprintf(gen->arena,
                            "({ RtHandleV2 *__arr_h__ = rt_array_create_generic_v2(%s, 1, sizeof(%s), (%s[]){%s});"
                            " rt_handle_set_copy_callback(__arr_h__, __copy_array_%s__); __arr_h__; })",
                            ARENA_VAR(gen), literal_type, literal_type, val, sn_name);
                    } else {
                        elem_str = arena_sprintf(gen->arena, "rt_array_create_generic_v2(%s, 1, sizeof(%s), (%s[]){%s})",
                                                ARENA_VAR(gen), literal_type, literal_type, val);
                    }
                }
            }

            if (result == NULL) {
                result = elem_str;
            } else {
                if (string_handle_mode) {
                    /* Concat takes two handles directly */
                    result = arena_sprintf(gen->arena, "rt_array_concat_string_v2(%s, %s)",
                                          result, elem_str);
                } else {
                    /* Non-string types: concat using generic */
                    const char *sizeof_expr = get_c_sizeof_elem(gen->arena, elem_type);
                    result = arena_sprintf(gen->arena, "rt_array_concat_v2(%s, %s, %s)",
                                          result, elem_str, sizeof_expr);
                }
            }
        }

        // Result is already an RtHandle
        if (result) return result;
        // Empty array fallback - strings and nested arrays use ptr create, others use generic
        if (string_handle_mode) {
            return arena_sprintf(gen->arena, "rt_array_create_ptr_v2(%s, 0, NULL)", ARENA_VAR(gen));
        }
        if (elem_type->kind == TYPE_ARRAY) {
            return arena_sprintf(gen->arena, "rt_array_create_ptr_v2(%s, 0, NULL)", ARENA_VAR(gen));
        }
        if (elem_type->kind == TYPE_STRUCT && struct_has_handle_fields(elem_type)) {
            code_gen_ensure_struct_callbacks(gen, elem_type);
            const char *elem_c = get_c_type(gen->arena, elem_type);
            const char *sn_name = elem_type->as.struct_type.name
                ? elem_type->as.struct_type.name : elem_c;
            return arena_sprintf(gen->arena,
                "({ RtHandleV2 *__arr_h__ = rt_array_create_generic_v2(%s, 0, sizeof(%s), NULL);"
                " rt_handle_set_copy_callback(__arr_h__, __copy_array_%s__); __arr_h__; })",
                ARENA_VAR(gen), elem_c, sn_name);
        }
        const char *sizeof_expr = get_c_sizeof_elem(gen->arena, elem_type);
        return arena_sprintf(gen->arena, "rt_array_create_generic_v2(%s, 0, %s, NULL)", ARENA_VAR(gen), sizeof_expr);
    }

    // Simple case: no spread or range elements
    // Build the element list
    // For struct element types, set the flag so struct literals omit their outer cast
    // (TCC doesn't support nested compound literal casts like (Point[]){(Point){...}})
    bool is_struct_array = (elem_type->kind == TYPE_STRUCT);
    if (is_struct_array) {
        gen->in_array_compound_literal = true;
    }

    char *inits = arena_strdup(gen->arena, "");
    /* String and nested array elements in handle mode:
     * - Strings: elements produce RtHandleV2*, used with rt_array_create_ptr_v2
     * - Nested arrays: elements are RtHandle values (inner arrays produce handles)
     * Both keep handle mode so elements are RtHandleV2*. */
    for (int i = 0; i < arr->element_count; i++) {
        char *el = code_gen_expression(gen, arr->elements[i]);
        char *sep = i > 0 ? ", " : "";
        inits = arena_sprintf(gen->arena, "%s%s%s", inits, sep, el);
    }

    if (is_struct_array) {
        gen->in_array_compound_literal = false;
    }

    if (suffix == NULL) {
        // For empty arrays with unknown element type (TYPE_NIL), return NULL/NULL.
        if (arr->element_count == 0 && elem_type->kind == TYPE_NIL) {
            return arena_strdup(gen->arena, "NULL");
        }
        // For empty arrays of function or nested array types, return NULL/NULL.
        if (arr->element_count == 0 && (elem_type->kind == TYPE_FUNCTION || elem_type->kind == TYPE_ARRAY)) {
            return arena_strdup(gen->arena, "NULL");
        }
        // For struct arrays, use rt_array_create_generic so the array has proper metadata
        if (is_struct_array) {
            /* Check if struct has handle fields - if so, ensure callbacks are generated
             * and wrap creation to set them on the new array handle */
            bool needs_callbacks = struct_has_handle_fields(elem_type);
            if (needs_callbacks) {
                code_gen_ensure_struct_callbacks(gen, elem_type);
                const char *sn_name = elem_type->as.struct_type.name
                    ? elem_type->as.struct_type.name : elem_c;
                if (arr->element_count == 0) {
                    return arena_sprintf(gen->arena,
                        "({ RtHandleV2 *__arr_h__ = rt_array_create_generic_v2(%s, 0, sizeof(%s), NULL);"
                        " rt_handle_set_copy_callback(__arr_h__, __copy_array_%s__); __arr_h__; })",
                        ARENA_VAR(gen), elem_c, sn_name);
                }
                return arena_sprintf(gen->arena,
                    "({ RtHandleV2 *__arr_h__ = rt_array_create_generic_v2(%s, %d, sizeof(%s), (%s[]){%s});"
                    " rt_handle_set_copy_callback(__arr_h__, __copy_array_%s__); __arr_h__; })",
                    ARENA_VAR(gen), arr->element_count, elem_c, elem_c, inits, sn_name);
            }
            if (arr->element_count == 0) {
                return arena_sprintf(gen->arena, "rt_array_create_generic_v2(%s, 0, sizeof(%s), NULL)",
                                     ARENA_VAR(gen), elem_c);
            }
            return arena_sprintf(gen->arena, "rt_array_create_generic_v2(%s, %d, sizeof(%s), (%s[]){%s})",
                                 ARENA_VAR(gen), arr->element_count, elem_c, elem_c, inits);
        }
        // For unsupported element types (like nested arrays), fall back to
        // compound literal without runtime wrapper
        return arena_sprintf(gen->arena, "(%s[]){%s}", elem_c, inits);
    }

    // Generate array creation using appropriate function variant
    // For bool arrays, use "int" for compound literal since runtime uses int internally
    // For nested arrays and string arrays, use "RtHandleV2 *" since elements are handles
    const char *literal_type;
    if (elem_type->kind == TYPE_BOOL) {
        literal_type = "int";
    } else if (elem_type->kind == TYPE_ARRAY) {
        literal_type = "RtHandleV2 *";
    } else if (elem_type->kind == TYPE_STRING) {
        literal_type = "RtHandleV2 *";
    } else {
        literal_type = elem_c;
    }

    /* Strings and nested arrays use ptr create, others use generic */
    if (elem_type->kind == TYPE_STRING) {
        return arena_sprintf(gen->arena, "rt_array_create_ptr_v2(%s, %d, (RtHandleV2 *[]){%s})",
                             ARENA_VAR(gen), arr->element_count, inits);
    }
    if (elem_type->kind == TYPE_ARRAY) {
        return arena_sprintf(gen->arena, "rt_array_create_ptr_v2(%s, %d, (RtHandleV2 *[]){%s})",
                             ARENA_VAR(gen), arr->element_count, inits);
    }
    if (elem_type->kind == TYPE_STRUCT && struct_has_handle_fields(elem_type)) {
        code_gen_ensure_struct_callbacks(gen, elem_type);
        const char *sn_name = elem_type->as.struct_type.name
            ? elem_type->as.struct_type.name : literal_type;
        return arena_sprintf(gen->arena,
            "({ RtHandleV2 *__arr_h__ = rt_array_create_generic_v2(%s, %d, sizeof(%s), (%s[]){%s});"
            " rt_handle_set_copy_callback(__arr_h__, __copy_array_%s__); __arr_h__; })",
            ARENA_VAR(gen), arr->element_count, literal_type, literal_type, inits, sn_name);
    }
    return arena_sprintf(gen->arena, "rt_array_create_generic_v2(%s, %d, sizeof(%s), (%s[]){%s})",
                         ARENA_VAR(gen), arr->element_count, literal_type, literal_type, inits);
}

char *code_gen_array_access_expression(CodeGen *gen, ArrayAccessExpr *expr)
{
    DEBUG_VERBOSE("Entering code_gen_array_access_expression");
    /* For V2 arrays, use typed element accessors with built-in transactions.
     * This ensures the data pointer is stable during element access. */
    char *handle_str = code_gen_expression(gen, expr->array);

    char *index_str = code_gen_expression(gen, expr->index);

    /* Get the element type for proper casting */
    Type *elem_type = NULL;
    if (expr->array->expr_type && expr->array->expr_type->kind == TYPE_ARRAY)
    {
        elem_type = expr->array->expr_type->as.array.element_type;
    }
    /* Resolve forward-declared struct types so we get the correct c_alias */
    if (elem_type) elem_type = resolve_struct_type(gen, elem_type);

    /* Compute the adjusted index for negative index support */
    char *adj_index;
    if (is_provably_non_negative(gen, expr->index))
    {
        adj_index = index_str;
    }
    else if (expr->index->type == EXPR_LITERAL &&
        expr->index->as.literal.type != NULL &&
        (expr->index->as.literal.type->kind == TYPE_INT ||
         expr->index->as.literal.type->kind == TYPE_LONG))
    {
        adj_index = arena_sprintf(gen->arena, "(rt_array_length_v2(%s) + %s)",
                                  handle_str, index_str);
    }
    else
    {
        adj_index = arena_sprintf(gen->arena, "((%s) < 0 ? rt_array_length_v2(%s) + (%s) : (%s))",
                                  index_str, handle_str, index_str, index_str);
    }

    /* Use typed accessor if available (primitives, handles) */
    const char *suffix = get_array_accessor_suffix(elem_type);
    char *result;

    if (suffix != NULL)
    {
        /* Typed accessor: rt_array_get_<suffix>_v2(handle, index) */
        result = arena_sprintf(gen->arena, "rt_array_get_%s_v2(%s, %s)",
                               suffix, handle_str, adj_index);
    }
    else
    {
        /* Struct/complex type: use rt_array_data_v2 for lvalue access.
         * This must return an lvalue so code like &(arr[i]) works. */
        const char *elem_c = elem_type ? get_c_array_elem_type(gen->arena, elem_type) : "long long";
        result = arena_sprintf(gen->arena, "((%s *)rt_array_data_v2(%s))[%s]",
                               elem_c, handle_str, adj_index);
    }

    return result;
}

char *code_gen_array_slice_expression(CodeGen *gen, Expr *expr)
{
    DEBUG_VERBOSE("Entering code_gen_array_slice_expression");
    ArraySliceExpr *slice = &expr->as.array_slice;

    /* V2 mode: Get handle for array slicing. For pointer types, evaluate as raw pointer. */
    char *array_str;
    char *handle_str = NULL;
    if (slice->array->expr_type != NULL && slice->array->expr_type->kind == TYPE_ARRAY)
    {
        handle_str = code_gen_expression(gen, slice->array);
        Type *elem_type = resolve_struct_type(gen, slice->array->expr_type->as.array.element_type);
        const char *elem_c = get_c_array_elem_type(gen->arena, elem_type);
        array_str = arena_sprintf(gen->arena, "((%s *)rt_array_data_v2(%s))", elem_c, handle_str);
    }
    else
    {
        /* Pointer type: evaluate as raw pointer */
        array_str = code_gen_expression(gen, slice->array);
    }

    // Get start, end, and step values - use LONG_MIN to signal defaults
    char *start_str = slice->start ? code_gen_expression(gen, slice->start) : "LONG_MIN";
    char *end_str = slice->end ? code_gen_expression(gen, slice->end) : "LONG_MIN";
    char *step_str = slice->step ? code_gen_expression(gen, slice->step) : "LONG_MIN";

    // Determine element type for the correct slice function
    // Can be either array type or pointer type (for pointer slicing)
    Type *operand_type = slice->array->expr_type;
    Type *elem_type = NULL;
    if (operand_type->kind == TYPE_ARRAY)
    {
        elem_type = operand_type->as.array.element_type;
    }
    else if (operand_type->kind == TYPE_POINTER)
    {
        elem_type = operand_type->as.pointer.base_type;
    }
    else
    {
        fprintf(stderr, "Error: Cannot slice non-array, non-pointer type\n");
        exit(1);
    }

    /* For pointer slicing, we need to create an array from the pointer.
     * Use rt_array_create_generic[_v2](arena, length, sizeof, ptr + start) instead of
     * the array slice functions which require runtime array metadata. */
    if (operand_type->kind == TYPE_POINTER)
    {
        const char *elem_c = get_c_type(gen->arena, elem_type);
        if (elem_type->kind == TYPE_STRUCT && struct_has_handle_fields(elem_type)) {
            code_gen_ensure_struct_callbacks(gen, elem_type);
            const char *sn_name = elem_type->as.struct_type.name
                ? elem_type->as.struct_type.name : elem_c;
            return arena_sprintf(gen->arena,
                "({ RtHandleV2 *__arr_h__ = rt_array_create_generic_v2(%s, (size_t)((%s) - (%s)), sizeof(%s), (%s) + (%s));"
                " rt_handle_set_copy_callback(__arr_h__, __copy_array_%s__); __arr_h__; })",
                ARENA_VAR(gen), end_str, start_str, elem_c, array_str, start_str, sn_name);
        }
        return arena_sprintf(gen->arena,
            "rt_array_create_generic_v2(%s, (size_t)((%s) - (%s)), sizeof(%s), (%s) + (%s))",
            ARENA_VAR(gen), end_str, start_str, elem_c, array_str, start_str);
    }

    /* V2 slice uses generic function with sizeof */
    /* String arrays use specialized function */
    if (elem_type->kind == TYPE_STRING) {
        return arena_sprintf(gen->arena, "rt_array_slice_string_v2(%s, %s, %s, %s)",
                             handle_str, start_str, end_str, step_str);
    }
    /* All other types use generic slice with sizeof */
    const char *sizeof_expr = get_c_sizeof_elem(gen->arena, elem_type);
    return arena_sprintf(gen->arena, "rt_array_slice_v2(%s, %s, %s, %s, %s)",
                         handle_str, start_str, end_str, step_str, sizeof_expr);
}
