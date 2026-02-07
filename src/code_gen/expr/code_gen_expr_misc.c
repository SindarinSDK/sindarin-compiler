#include "code_gen/expr/code_gen_expr_misc.h"
#include "code_gen/expr/code_gen_expr.h"
#include "code_gen/util/code_gen_util.h"
#include "debug.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

char *code_gen_range_expression(CodeGen *gen, Expr *expr)
{
    DEBUG_VERBOSE("Entering code_gen_range_expression");
    RangeExpr *range = &expr->as.range;

    char *start_str = code_gen_expression(gen, range->start);
    char *end_str = code_gen_expression(gen, range->end);

    if (gen->expr_as_handle && gen->current_arena_var != NULL)
    {
        return arena_sprintf(gen->arena, "rt_array_range_v2(%s, %s, %s)", ARENA_VAR(gen), start_str, end_str);
    }
    return arena_sprintf(gen->arena, "rt_array_range(%s, %s, %s)", ARENA_VAR(gen), start_str, end_str);
}

char *code_gen_spread_expression(CodeGen *gen, Expr *expr)
{
    DEBUG_VERBOSE("Entering code_gen_spread_expression");
    // Spread expressions are typically handled in array literal context
    // but if used standalone, just return the array
    return code_gen_expression(gen, expr->as.spread.array);
}

char *code_gen_sized_array_alloc_expression(CodeGen *gen, Expr *expr)
{
    DEBUG_VERBOSE("Entering code_gen_sized_array_alloc_expression");

    /* Extract fields from sized_array_alloc */
    SizedArrayAllocExpr *alloc = &expr->as.sized_array_alloc;
    Type *element_type = alloc->element_type;
    Expr *size_expr = alloc->size_expr;
    Expr *default_value = alloc->default_value;

    /* Determine the runtime function suffix based on element type */
    const char *suffix = NULL;
    switch (element_type->kind) {
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
        default:
            fprintf(stderr, "Error: Unsupported element type for sized array allocation\n");
            exit(1);
    }

    /* Generate code for the size expression */
    char *size_str = code_gen_expression(gen, size_expr);

    /* Generate code for the default value */
    char *default_str;
    if (default_value != NULL) {
        /* For string arrays, the alloc function takes a raw char* and converts
           to RtHandle internally, so we must evaluate in raw mode */
        bool saved_handle = gen->expr_as_handle;
        if (element_type->kind == TYPE_STRING) {
            gen->expr_as_handle = false;
        }
        default_str = code_gen_expression(gen, default_value);
        gen->expr_as_handle = saved_handle;
    } else {
        /* Use type-appropriate zero value when no default provided */
        switch (element_type->kind) {
            case TYPE_INT:
            case TYPE_LONG: default_str = "0"; break;
            case TYPE_INT32: default_str = "0"; break;
            case TYPE_UINT: default_str = "0"; break;
            case TYPE_UINT32: default_str = "0"; break;
            case TYPE_FLOAT: default_str = "0.0f"; break;
            case TYPE_DOUBLE: default_str = "0.0"; break;
            case TYPE_CHAR: default_str = "'\\0'"; break;
            case TYPE_BOOL: default_str = "0"; break;
            case TYPE_BYTE: default_str = "0"; break;
            case TYPE_STRING: default_str = "NULL"; break;
            default: default_str = "0"; break;
        }
    }

    /* Construct the runtime function call: rt_array_alloc_{suffix}[_v2](arena, size, default) */
    const char *h_suffix = (gen->expr_as_handle && gen->current_arena_var != NULL) ? "_v2" : "";
    return arena_sprintf(gen->arena, "rt_array_alloc_%s%s(%s, %s, %s)",
                         suffix, h_suffix, ARENA_VAR(gen), size_str, default_str);
}

const char *get_array_clone_suffix(Type *element_type)
{
    if (element_type == NULL) return NULL;

    switch (element_type->kind)
    {
        case TYPE_INT:
        case TYPE_LONG: return "long";
        case TYPE_INT32: return "int32";
        case TYPE_UINT: return "uint";
        case TYPE_UINT32: return "uint32";
        case TYPE_DOUBLE: return "double";
        case TYPE_FLOAT: return "float";
        case TYPE_CHAR: return "char";
        case TYPE_BOOL: return "bool";
        case TYPE_BYTE: return "byte";
        case TYPE_STRING: return "string";
        default: return NULL;
    }
}

/**
 * Generate code for struct deep copy.
 * Creates a new struct with all array fields independently copied.
 */
static char *code_gen_struct_deep_copy(CodeGen *gen, Type *struct_type, char *operand_code)
{
    /* Generate a statement expression that:
     * 1. Copies the struct itself (shallow copy)
     * 2. For each array field, creates an independent copy
     */
    const char *struct_name = sn_mangle_name(gen->arena, struct_type->as.struct_type.name);
    int field_count = struct_type->as.struct_type.field_count;

    /* Check if any fields need deep copying (arrays or strings) */
    bool has_array_fields = false;
    for (int i = 0; i < field_count; i++)
    {
        StructField *field = &struct_type->as.struct_type.fields[i];
        if (field->type != NULL &&
            (field->type->kind == TYPE_ARRAY || field->type->kind == TYPE_STRING))
        {
            has_array_fields = true;
            break;
        }
    }

    /* If no array fields, just do a simple struct copy */
    if (!has_array_fields)
    {
        return operand_code;
    }

    /* Generate statement expression for deep copy */
    char *result = arena_sprintf(gen->arena, "({\n        %s __deep_copy = %s;\n", struct_name, operand_code);

    /* For each array field, generate deep copy */
    for (int i = 0; i < field_count; i++)
    {
        StructField *field = &struct_type->as.struct_type.fields[i];
        /* Use c_alias for native struct fields, otherwise mangle */
        const char *c_field_name = field->c_alias != NULL
            ? field->c_alias
            : sn_mangle_name(gen->arena, field->name);

        if (field->type != NULL && field->type->kind == TYPE_ARRAY)
        {
            /* Get the element type and corresponding clone function */
            Type *element_type = field->type->as.array.element_type;
            const char *suffix = get_array_clone_suffix(element_type);
            if (suffix != NULL)
            {
                /* Copy array field: pin source handle → clone to new handle */
                result = arena_sprintf(gen->arena, "%s        __deep_copy.%s = rt_array_clone_%s_v2(%s, rt_array_data_v2(__deep_copy.%s));\n",
                                       result, c_field_name, suffix, ARENA_VAR(gen), c_field_name);
            }
            /* If no clone function available (e.g., nested arrays), leave as shallow copy */
        }
        else if (field->type != NULL && field->type->kind == TYPE_STRING)
        {
            /* Copy string field: pin source handle → strdup to new handle */
            result = arena_sprintf(gen->arena,
                                   "%s        __deep_copy.%s = __deep_copy.%s ? rt_arena_v2_strdup(%s, (char *)rt_handle_v2_pin(__deep_copy.%s)) : NULL;\n",
                                   result, c_field_name, c_field_name, ARENA_VAR(gen), c_field_name);
        }
    }

    /* Return the deep copied struct */
    result = arena_sprintf(gen->arena, "%s        __deep_copy;\n    })", result);

    return result;
}

char *code_gen_as_ref_expression(CodeGen *gen, Expr *expr)
{
    DEBUG_VERBOSE("Generating as_ref expression");

    AsRefExpr *as_ref = &expr->as.as_ref;
    char *operand_code = code_gen_expression(gen, as_ref->operand);
    Type *operand_type = as_ref->operand->expr_type;

    if (operand_type != NULL && operand_type->kind == TYPE_ARRAY)
    {
        /* Arrays: the variable already holds a pointer to the array data.
         * In C, Sindarin arrays are represented as T* pointing to data. */
        return operand_code;
    }
    else
    {
        /* Other types: use address-of operator */
        return arena_sprintf(gen->arena, "(&(%s))", operand_code);
    }
}

char *code_gen_as_val_expression(CodeGen *gen, Expr *expr)
{
    DEBUG_VERBOSE("Generating as_val expression");

    AsValExpr *as_val = &expr->as.as_val;
    char *operand_code = code_gen_expression(gen, as_val->operand);

    if (as_val->is_noop)
    {
        /* Operand is already an array type (e.g., from ptr[0..len] slice).
         * Just pass through without any transformation. */
        return operand_code;
    }
    else if (as_val->is_cstr_to_str)
    {
        /* *char => str: convert C string to arena-managed string.
         * Handle NULL pointer by returning empty string. */
        if (gen->expr_as_handle && gen->current_arena_var != NULL)
        {
            /* Handle mode: produce RtHandleV2* via arena strdup */
            return arena_sprintf(gen->arena, "((%s) ? rt_arena_v2_strdup(%s, %s) : rt_arena_v2_strdup(%s, \"\"))",
                                operand_code, ARENA_VAR(gen), operand_code, ARENA_VAR(gen));
        }
        /* Raw pointer mode: use bridge layer for permanent pin */
        return arena_sprintf(gen->arena, "((%s) ? rt_arena_strdup(%s, %s) : rt_arena_strdup(%s, \"\"))",
                            operand_code, ARENA_VAR(gen), operand_code, ARENA_VAR(gen));
    }
    else if (as_val->is_struct_deep_copy)
    {
        /* Struct deep copy: copy struct and independently copy array fields */
        Type *operand_type = as_val->operand->expr_type;
        if (operand_type != NULL && operand_type->kind == TYPE_STRUCT)
        {
            return code_gen_struct_deep_copy(gen, operand_type, operand_code);
        }
        /* Fallback: should not happen, but just return operand */
        return operand_code;
    }
    else
    {
        /* Primitive pointer dereference: *int, *double, *float, etc. */
        return arena_sprintf(gen->arena, "(*(%s))", operand_code);
    }
}
