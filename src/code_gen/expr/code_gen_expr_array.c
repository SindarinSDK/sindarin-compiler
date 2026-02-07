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
        bool use_handle = gen->expr_as_handle && gen->current_arena_var != NULL;
        // For any[] arrays, box each element according to its actual type
        char *pushes = arena_strdup(gen->arena, "");
        for (int i = 0; i < arr->element_count; i++) {
            Expr *elem = arr->elements[i];
            /* String elements must be generated in raw mode (expr_as_handle=false)
             * because rt_box_string expects a char* pointer, not an RtHandle. */
            bool saved_elem_handle = gen->expr_as_handle;
            if (elem->expr_type != NULL && elem->expr_type->kind == TYPE_STRING) {
                gen->expr_as_handle = false;
            }
            char *el = code_gen_expression(gen, elem);
            gen->expr_as_handle = saved_elem_handle;
            // Box the element based on its actual type
            if (elem->expr_type != NULL && elem->expr_type->kind != TYPE_ANY) {
                el = code_gen_box_value(gen, el, elem->expr_type);
            }
            if (use_handle) {
                pushes = arena_sprintf(gen->arena, "%s _arr = rt_array_push_any_v2(%s, _arr, %s);",
                                       pushes, ARENA_VAR(gen), el);
            } else {
                pushes = arena_sprintf(gen->arena, "%s _arr = rt_array_push_any(%s, _arr, %s);",
                                       pushes, ARENA_VAR(gen), el);
            }
        }
        if (use_handle) {
            return arena_sprintf(gen->arena, "({ RtHandleV2 *_arr = NULL;%s _arr; })", pushes);
        }
        return arena_sprintf(gen->arena, "({ RtAny *_arr = NULL;%s _arr; })", pushes);
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
        bool saved_handle = gen->expr_as_handle;
        char *result = NULL;

        /* String arrays in handle mode: elements are RtHandle values in memory,
         * so we must use _h variants directly (legacy clone/concat would
         * interpret handle values as char* pointers, causing crashes). */
        bool string_handle_mode = (elem_type->kind == TYPE_STRING &&
                                   gen->current_arena_var != NULL);

        if (!string_handle_mode) {
            gen->expr_as_handle = false;  // Force legacy mode for sub-expressions
        }

        for (int i = 0; i < arr->element_count; i++) {
            Expr *elem = arr->elements[i];
            char *elem_str;

            if (elem->type == EXPR_SPREAD) {
                if (string_handle_mode) {
                    /* V2 string arrays: get handle and clone directly */
                    gen->expr_as_handle = true;
                    char *arr_h = code_gen_expression(gen, elem->as.spread.array);
                    gen->expr_as_handle = saved_handle;
                    elem_str = arena_sprintf(gen->arena, "rt_array_clone_string_v2(%s)", arr_h);
                } else if (gen->current_arena_var != NULL) {
                    /* V2 mode for non-string types: get handle and clone using generic */
                    gen->expr_as_handle = true;
                    char *arr_h = code_gen_expression(gen, elem->as.spread.array);
                    gen->expr_as_handle = saved_handle;
                    const char *sizeof_expr = get_c_sizeof_elem(gen->arena, elem_type);
                    elem_str = arena_sprintf(gen->arena, "rt_array_clone_v2(%s, %s)", arr_h, sizeof_expr);
                } else {
                    // V1 mode: clone the array to avoid aliasing issues
                    char *arr_str = code_gen_expression(gen, elem->as.spread.array);
                    elem_str = arena_sprintf(gen->arena, "rt_array_clone_%s(%s, %s)", suffix, ARENA_VAR(gen), arr_str);
                }
            } else if (elem->type == EXPR_RANGE) {
                // Range: concat the range result
                if (gen->current_arena_var != NULL) {
                    /* V2 mode: enable handle mode for range expression */
                    gen->expr_as_handle = true;
                    elem_str = code_gen_range_expression(gen, elem);
                    gen->expr_as_handle = saved_handle;
                } else {
                    elem_str = code_gen_range_expression(gen, elem);
                }
            } else {
                if (string_handle_mode) {
                    // Get raw char* value; create_string_v2 converts to handle internally
                    gen->expr_as_handle = false;
                    char *val = code_gen_expression(gen, elem);
                    gen->expr_as_handle = saved_handle;
                    elem_str = arena_sprintf(gen->arena,
                        "rt_array_create_string_v2(%s, 1, (char *[]){%s})",
                        ARENA_VAR(gen), val);
                } else if (gen->current_arena_var != NULL) {
                    // V2 mode for non-string: create single-element array using generic
                    char *val = code_gen_expression(gen, elem);
                    const char *literal_type;
                    if (elem_type->kind == TYPE_BOOL) {
                        literal_type = "int";
                    } else if (elem_type->kind == TYPE_ARRAY) {
                        literal_type = "RtHandleV2 *";
                    } else {
                        literal_type = elem_c;
                    }
                    elem_str = arena_sprintf(gen->arena, "rt_array_create_generic_v2(%s, 1, sizeof(%s), (%s[]){%s})",
                                            ARENA_VAR(gen), literal_type, literal_type, val);
                } else {
                    // V1 mode: create single-element array as raw pointer
                    char *val = code_gen_expression(gen, elem);
                    const char *literal_type;
                    if (elem_type->kind == TYPE_BOOL) {
                        literal_type = "int";
                    } else if (elem_type->kind == TYPE_ARRAY) {
                        literal_type = "void *";
                    } else {
                        literal_type = elem_c;
                    }
                    elem_str = arena_sprintf(gen->arena, "rt_array_create_%s(%s, 1, (%s[]){%s})",
                                            suffix, ARENA_VAR(gen), literal_type, val);
                }
            }

            if (result == NULL) {
                result = elem_str;
            } else {
                if (string_handle_mode) {
                    /* V2 concat takes two handles directly */
                    result = arena_sprintf(gen->arena, "rt_array_concat_string_v2(%s, %s)",
                                          result, elem_str);
                } else if (gen->current_arena_var != NULL) {
                    /* V2 mode for non-string types: concat using generic */
                    const char *sizeof_expr = get_c_sizeof_elem(gen->arena, elem_type);
                    result = arena_sprintf(gen->arena, "rt_array_concat_v2(%s, %s, %s)",
                                          result, elem_str, sizeof_expr);
                } else {
                    // V1 mode: concat with raw pointers
                    result = arena_sprintf(gen->arena, "rt_array_concat_%s(%s, %s, %s)",
                                          suffix, ARENA_VAR(gen), result, elem_str);
                }
            }
        }

        gen->expr_as_handle = saved_handle;

        if (string_handle_mode || (saved_handle && gen->current_arena_var != NULL)) {
            // V2 mode: result is already an RtHandle
            if (result) return result;
            // Empty array fallback - strings need special create, others use generic
            if (string_handle_mode) {
                return arena_sprintf(gen->arena, "rt_array_create_string_v2(%s, 0, NULL)", ARENA_VAR(gen));
            }
            const char *sizeof_expr = get_c_sizeof_elem(gen->arena, elem_type);
            return arena_sprintf(gen->arena, "rt_array_create_generic_v2(%s, 0, %s, NULL)", ARENA_VAR(gen), sizeof_expr);
        }

        if (result == NULL) {
            result = arena_sprintf(gen->arena, "rt_array_create_%s(%s, 0, NULL)", suffix, ARENA_VAR(gen));
        }

        return result;
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
     * - Strings: rt_array_create_string_h takes raw char* and converts to handles
     * - Nested arrays: elements are RtHandle values (inner arrays produce handles)
     * So for strings we force raw mode; for nested arrays we keep handle mode. */
    bool saved_handle_for_elems = gen->expr_as_handle;
    if (elem_type->kind == TYPE_STRING) {
        gen->expr_as_handle = false;
    }
    for (int i = 0; i < arr->element_count; i++) {
        char *el = code_gen_expression(gen, arr->elements[i]);
        char *sep = i > 0 ? ", " : "";
        inits = arena_sprintf(gen->arena, "%s%s%s", inits, sep, el);
    }
    if (elem_type->kind == TYPE_STRING || elem_type->kind == TYPE_ARRAY) {
        gen->expr_as_handle = saved_handle_for_elems;
    }

    if (is_struct_array) {
        gen->in_array_compound_literal = false;
    }

    if (suffix == NULL) {
        // For empty arrays with unknown element type (TYPE_NIL), return NULL/NULL.
        if (arr->element_count == 0 && elem_type->kind == TYPE_NIL) {
            return arena_strdup(gen->arena, gen->expr_as_handle ? "NULL" : "NULL");
        }
        // For empty arrays of function or nested array types, return NULL/NULL.
        if (arr->element_count == 0 && (elem_type->kind == TYPE_FUNCTION || elem_type->kind == TYPE_ARRAY)) {
            return arena_strdup(gen->arena, gen->expr_as_handle ? "NULL" : "NULL");
        }
        // For struct arrays, use rt_array_create_generic so the array has proper metadata
        if (is_struct_array) {
            if (gen->expr_as_handle && gen->current_arena_var != NULL) {
                /* Handle mode: use _h variant returning RtHandle */
                if (arr->element_count == 0) {
                    return arena_sprintf(gen->arena, "rt_array_create_generic_v2(%s, 0, sizeof(%s), NULL)",
                                         ARENA_VAR(gen), elem_c);
                }
                return arena_sprintf(gen->arena, "rt_array_create_generic_v2(%s, %d, sizeof(%s), (%s[]){%s})",
                                     ARENA_VAR(gen), arr->element_count, elem_c, elem_c, inits);
            }
            if (arr->element_count == 0) {
                return arena_sprintf(gen->arena, "rt_array_create_generic(%s, 0, sizeof(%s), NULL)",
                                     ARENA_VAR(gen), elem_c);
            }
            return arena_sprintf(gen->arena, "(%s *)rt_array_create_generic(%s, %d, sizeof(%s), (%s[]){%s})",
                                 elem_c, ARENA_VAR(gen), arr->element_count, elem_c, elem_c, inits);
        }
        // For unsupported element types (like nested arrays), fall back to
        // compound literal without runtime wrapper
        return arena_sprintf(gen->arena, "(%s[]){%s}", elem_c, inits);
    }

    // Generate array creation using appropriate function variant
    // For bool arrays, use "int" for compound literal since runtime uses int internally
    // For nested arrays in handle mode, use "RtHandleV2 *" since elements are handles
    // For string arrays, use "char *" since rt_array_create_string expects const char **
    const char *literal_type;
    if (elem_type->kind == TYPE_BOOL) {
        literal_type = "int";
    } else if (elem_type->kind == TYPE_ARRAY) {
        literal_type = (gen->expr_as_handle && gen->current_arena_var != NULL) ? "RtHandleV2 *" : "void *";
    } else if (elem_type->kind == TYPE_STRING) {
        literal_type = "char *";
    } else {
        literal_type = elem_c;
    }

    if (gen->expr_as_handle && gen->current_arena_var != NULL)
    {
        /* Handle mode: strings need special create, others use generic */
        if (elem_type->kind == TYPE_STRING) {
            return arena_sprintf(gen->arena, "rt_array_create_string_v2(%s, %d, (%s[]){%s})",
                                 ARENA_VAR(gen), arr->element_count, literal_type, inits);
        }
        return arena_sprintf(gen->arena, "rt_array_create_generic_v2(%s, %d, sizeof(%s), (%s[]){%s})",
                             ARENA_VAR(gen), arr->element_count, literal_type, literal_type, inits);
    }
    return arena_sprintf(gen->arena, "rt_array_create_%s(%s, %d, (%s[]){%s})",
                         suffix, ARENA_VAR(gen), arr->element_count, literal_type, inits);
}

char *code_gen_array_access_expression(CodeGen *gen, ArrayAccessExpr *expr)
{
    DEBUG_VERBOSE("Entering code_gen_array_access_expression");
    /* For V2 arrays:
     * - Keep handle form for rt_array_length_v2(handle) and rt_array_data_v2(handle)
     * - Use rt_array_data_v2() to get the data pointer for element access
     */
    bool saved_as_handle = gen->expr_as_handle;
    gen->expr_as_handle = true;  /* Keep handle form for V2 functions */
    char *handle_str = code_gen_expression(gen, expr->array);
    gen->expr_as_handle = saved_as_handle;

    char *index_str = code_gen_expression(gen, expr->index);

    /* Get the element type for proper casting */
    Type *elem_type = NULL;
    if (expr->array->expr_type && expr->array->expr_type->kind == TYPE_ARRAY)
    {
        elem_type = expr->array->expr_type->as.array.element_type;
    }
    const char *elem_c = elem_type ? get_c_array_elem_type(gen->arena, elem_type) : "long long";

    /* Generate the data pointer access: (T *)rt_array_data_v2(handle) */
    char *data_str = arena_sprintf(gen->arena, "((%s *)rt_array_data_v2(%s))", elem_c, handle_str);

    /* Build the subscript expression */
    char *result;
    // Check if index is provably non-negative (literal >= 0 or tracked loop counter)
    if (is_provably_non_negative(gen, expr->index))
    {
        // Non-negative index - direct array access, no adjustment needed
        result = arena_sprintf(gen->arena, "%s[%s]", data_str, index_str);
    }
    // Check if index is a negative literal - can simplify to: arr[len + idx]
    else if (expr->index->type == EXPR_LITERAL &&
        expr->index->as.literal.type != NULL &&
        (expr->index->as.literal.type->kind == TYPE_INT ||
         expr->index->as.literal.type->kind == TYPE_LONG))
    {
        // Negative literal - adjust by array length
        result = arena_sprintf(gen->arena, "%s[rt_array_length_v2(%s) + %s]",
                             data_str, handle_str, index_str);
    }
    else
    {
        // For potentially negative variable indices, generate runtime check
        // data[idx < 0 ? rt_array_length_v2(handle) + idx : idx]
        result = arena_sprintf(gen->arena, "%s[(%s) < 0 ? rt_array_length_v2(%s) + (%s) : (%s)]",
                             data_str, index_str, handle_str, index_str, index_str);
    }

    /* If the element type is a handle type (string/array) and we're NOT in handle mode,
     * pin the element to get a raw pointer. */
    if (!saved_as_handle && elem_type != NULL && is_handle_type(elem_type) &&
        gen->current_arena_var != NULL)
    {
        /* Pin element handles. V2 handles don't need the arena parameter. */
        if (elem_type->kind == TYPE_STRING)
        {
            return arena_sprintf(gen->arena, "((char *)rt_handle_v2_pin(%s))",
                                 result);
        }
        else if (elem_type->kind == TYPE_ARRAY)
        {
            const char *inner_elem_c = get_c_array_elem_type(gen->arena, elem_type->as.array.element_type);
            return arena_sprintf(gen->arena, "((%s *)rt_array_data_v2(%s))",
                                 inner_elem_c, result);
        }
    }

    return result;
}

char *code_gen_array_slice_expression(CodeGen *gen, Expr *expr)
{
    DEBUG_VERBOSE("Entering code_gen_array_slice_expression");
    ArraySliceExpr *slice = &expr->as.array_slice;

    /* V2 mode: Get handle for array slicing. For pointer types, evaluate as raw pointer. */
    bool saved_as_handle = gen->expr_as_handle;
    char *array_str;
    char *handle_str = NULL;
    if (slice->array->expr_type != NULL && slice->array->expr_type->kind == TYPE_ARRAY)
    {
        gen->expr_as_handle = true;
        handle_str = code_gen_expression(gen, slice->array);
        gen->expr_as_handle = saved_as_handle;
        Type *elem_type = slice->array->expr_type->as.array.element_type;
        const char *elem_c = get_c_array_elem_type(gen->arena, elem_type);
        array_str = arena_sprintf(gen->arena, "((%s *)rt_array_data_v2(%s))", elem_c, handle_str);
    }
    else
    {
        /* Pointer type: evaluate as raw pointer */
        gen->expr_as_handle = false;
        array_str = code_gen_expression(gen, slice->array);
        gen->expr_as_handle = saved_as_handle;
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
        if (gen->expr_as_handle) {
            return arena_sprintf(gen->arena,
                "rt_array_create_generic_v2(%s, (size_t)((%s) - (%s)), sizeof(%s), (%s) + (%s))",
                ARENA_VAR(gen), end_str, start_str, elem_c, array_str, start_str);
        }
        return arena_sprintf(gen->arena,
            "(%s *)rt_array_create_generic(%s, (size_t)((%s) - (%s)), sizeof(%s), (%s) + (%s))",
            elem_c, ARENA_VAR(gen), end_str, start_str, elem_c, array_str, start_str);
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
