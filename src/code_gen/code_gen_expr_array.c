#include "code_gen/code_gen_expr.h"
#include "code_gen/code_gen_util.h"
#include "code_gen/code_gen_stmt.h"
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
        // Generate: ({ RtAny *_arr = NULL; _arr = rt_array_push_any(...); ... _arr; })
        char *pushes = arena_strdup(gen->arena, "");
        for (int i = 0; i < arr->element_count; i++) {
            Expr *elem = arr->elements[i];
            char *el = code_gen_expression(gen, elem);
            // Box the element based on its actual type
            if (elem->expr_type != NULL && elem->expr_type->kind != TYPE_ANY) {
                el = code_gen_box_value(gen, el, elem->expr_type);
            }
            pushes = arena_sprintf(gen->arena, "%s _arr = rt_array_push_any(%s, _arr, %s);",
                                   pushes, ARENA_VAR(gen), el);
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
        // Start with empty array or first element
        char *result = NULL;

        for (int i = 0; i < arr->element_count; i++) {
            Expr *elem = arr->elements[i];
            char *elem_str;

            if (elem->type == EXPR_SPREAD) {
                // Spread: clone the array to avoid aliasing issues
                char *arr_str = code_gen_expression(gen, elem->as.spread.array);
                elem_str = arena_sprintf(gen->arena, "rt_array_clone_%s(%s, %s)", suffix, ARENA_VAR(gen), arr_str);
            } else if (elem->type == EXPR_RANGE) {
                // Range: concat the range result
                elem_str = code_gen_range_expression(gen, elem);
            } else {
                // Regular element: create single-element array
                char *val = code_gen_expression(gen, elem);
                const char *literal_type = (elem_type->kind == TYPE_BOOL) ? "int" : elem_c;
                elem_str = arena_sprintf(gen->arena, "rt_array_create_%s(%s, 1, (%s[]){%s})",
                                        suffix, ARENA_VAR(gen), literal_type, val);
            }

            if (result == NULL) {
                result = elem_str;
            } else {
                // Concat with previous result
                result = arena_sprintf(gen->arena, "rt_array_concat_%s(%s, %s, %s)",
                                      suffix, ARENA_VAR(gen), result, elem_str);
            }
        }

        return result ? result : arena_sprintf(gen->arena, "rt_array_create_%s(%s, 0, NULL)", suffix, ARENA_VAR(gen));
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
    for (int i = 0; i < arr->element_count; i++) {
        char *el = code_gen_expression(gen, arr->elements[i]);
        char *sep = i > 0 ? ", " : "";
        inits = arena_sprintf(gen->arena, "%s%s%s", inits, sep, el);
    }

    if (is_struct_array) {
        gen->in_array_compound_literal = false;
    }

    if (suffix == NULL) {
        // For empty arrays with unknown element type (TYPE_NIL), return NULL.
        // The runtime handles NULL as an empty array.
        if (arr->element_count == 0 && elem_type->kind == TYPE_NIL) {
            return arena_strdup(gen->arena, "NULL");
        }
        // For empty arrays of function or nested array types, return NULL.
        // The runtime push functions handle NULL as an empty array.
        if (arr->element_count == 0 && (elem_type->kind == TYPE_FUNCTION || elem_type->kind == TYPE_ARRAY)) {
            return arena_strdup(gen->arena, "NULL");
        }
        // For unsupported element types (like nested arrays), fall back to
        // compound literal without runtime wrapper
        return arena_sprintf(gen->arena, "(%s[]){%s}", elem_c, inits);
    }

    // Generate: rt_array_create_<suffix>(arena, count, (elem_type[]){...})
    // For bool arrays, use "int" for compound literal since runtime uses int internally
    // For nested arrays, use "void *" since rt_array_create_ptr expects void**
    const char *literal_type;
    if (elem_type->kind == TYPE_BOOL) {
        literal_type = "int";
    } else if (elem_type->kind == TYPE_ARRAY) {
        literal_type = "void *";
    } else {
        literal_type = elem_c;
    }
    return arena_sprintf(gen->arena, "rt_array_create_%s(%s, %d, (%s[]){%s})",
                         suffix, ARENA_VAR(gen), arr->element_count, literal_type, inits);
}

char *code_gen_array_access_expression(CodeGen *gen, ArrayAccessExpr *expr)
{
    DEBUG_VERBOSE("Entering code_gen_array_access_expression");
    char *array_str = code_gen_expression(gen, expr->array);
    char *index_str = code_gen_expression(gen, expr->index);

    // Check if index is provably non-negative (literal >= 0 or tracked loop counter)
    if (is_provably_non_negative(gen, expr->index))
    {
        // Non-negative index - direct array access, no adjustment needed
        return arena_sprintf(gen->arena, "%s[%s]", array_str, index_str);
    }

    // Check if index is a negative literal - can simplify to: arr[len + idx]
    if (expr->index->type == EXPR_LITERAL &&
        expr->index->as.literal.type != NULL &&
        (expr->index->as.literal.type->kind == TYPE_INT ||
         expr->index->as.literal.type->kind == TYPE_LONG))
    {
        // Negative literal - adjust by array length
        return arena_sprintf(gen->arena, "%s[rt_array_length(%s) + %s]",
                             array_str, array_str, index_str);
    }

    // For potentially negative variable indices, generate runtime check
    // arr[idx < 0 ? rt_array_length(arr) + idx : idx]
    return arena_sprintf(gen->arena, "%s[(%s) < 0 ? rt_array_length(%s) + (%s) : (%s)]",
                         array_str, index_str, array_str, index_str, index_str);
}

char *code_gen_array_slice_expression(CodeGen *gen, Expr *expr)
{
    DEBUG_VERBOSE("Entering code_gen_array_slice_expression");
    ArraySliceExpr *slice = &expr->as.array_slice;

    char *array_str = code_gen_expression(gen, slice->array);

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
     * Use rt_array_create_<type>(arena, length, ptr + start) instead of
     * the array slice functions which require runtime array metadata. */
    if (operand_type->kind == TYPE_POINTER)
    {
        const char *create_func = NULL;
        switch (elem_type->kind) {
            case TYPE_LONG:
            case TYPE_INT:
                create_func = "rt_array_create_long";
                break;
            case TYPE_INT32:
                create_func = "rt_array_create_int32";
                break;
            case TYPE_UINT:
                create_func = "rt_array_create_uint";
                break;
            case TYPE_UINT32:
                create_func = "rt_array_create_uint32";
                break;
            case TYPE_FLOAT:
                create_func = "rt_array_create_float";
                break;
            case TYPE_DOUBLE:
                create_func = "rt_array_create_double";
                break;
            case TYPE_CHAR:
                create_func = "rt_array_create_char";
                break;
            case TYPE_BYTE:
                create_func = "rt_array_create_byte";
                break;
            default:
                fprintf(stderr, "Error: Unsupported pointer element type for slice\n");
                exit(1);
        }
        /* Generate: rt_array_create_<type>(arena, (size_t)(end - start), ptr + start) */
        return arena_sprintf(gen->arena, "%s(%s, (size_t)((%s) - (%s)), (%s) + (%s))",
                             create_func, ARENA_VAR(gen), end_str, start_str, array_str, start_str);
    }

    /* For array slicing, use the regular slice functions */
    const char *slice_func = NULL;
    switch (elem_type->kind) {
        case TYPE_LONG:
        case TYPE_INT:
            slice_func = "rt_array_slice_long";
            break;
        case TYPE_INT32:
            slice_func = "rt_array_slice_int32";
            break;
        case TYPE_UINT:
            slice_func = "rt_array_slice_uint";
            break;
        case TYPE_UINT32:
            slice_func = "rt_array_slice_uint32";
            break;
        case TYPE_FLOAT:
            slice_func = "rt_array_slice_float";
            break;
        case TYPE_DOUBLE:
            slice_func = "rt_array_slice_double";
            break;
        case TYPE_CHAR:
            slice_func = "rt_array_slice_char";
            break;
        case TYPE_STRING:
            slice_func = "rt_array_slice_string";
            break;
        case TYPE_BOOL:
            slice_func = "rt_array_slice_bool";
            break;
        case TYPE_BYTE:
            slice_func = "rt_array_slice_byte";
            break;
        default:
            fprintf(stderr, "Error: Unsupported array element type for slice\n");
            exit(1);
    }

    return arena_sprintf(gen->arena, "%s(%s, %s, %s, %s, %s)", slice_func, ARENA_VAR(gen), array_str, start_str, end_str, step_str);
}
