#include "code_gen/expr/code_gen_expr_type.h"
#include "code_gen/expr/code_gen_expr.h"
#include "code_gen/util/code_gen_util.h"
#include "debug.h"
#include <stdlib.h>
#include <stdbool.h>

/**
 * Get the runtime type tag constant for a type.
 */
static const char *get_type_tag_constant(TypeKind kind)
{
    switch (kind)
    {
        case TYPE_NIL: return "RT_ANY_NIL";
        case TYPE_INT: return "RT_ANY_INT";
        case TYPE_LONG: return "RT_ANY_LONG";
        case TYPE_INT32: return "RT_ANY_INT32";
        case TYPE_UINT: return "RT_ANY_UINT";
        case TYPE_UINT32: return "RT_ANY_UINT32";
        case TYPE_DOUBLE: return "RT_ANY_DOUBLE";
        case TYPE_FLOAT: return "RT_ANY_FLOAT";
        case TYPE_STRING: return "RT_ANY_STRING";
        case TYPE_CHAR: return "RT_ANY_CHAR";
        case TYPE_BOOL: return "RT_ANY_BOOL";
        case TYPE_BYTE: return "RT_ANY_BYTE";
        case TYPE_ARRAY: return "RT_ANY_ARRAY";
        case TYPE_FUNCTION: return "RT_ANY_FUNCTION";
        case TYPE_STRUCT: return "RT_ANY_STRUCT";
        case TYPE_ANY: return "RT_ANY_NIL";  /* any has no fixed tag */
        case TYPE_VOID: return "RT_ANY_NIL";  /* void is not a value type */
        case TYPE_POINTER: return "RT_ANY_NIL";  /* raw pointers cannot be boxed */
        case TYPE_OPAQUE: return "RT_ANY_NIL";  /* opaque C types cannot be boxed */
        default: return "RT_ANY_NIL";
    }
}

char *code_gen_sizeof_expression(CodeGen *gen, Expr *expr)
{
    DEBUG_VERBOSE("Generating sizeof expression");

    SizeofExpr *sizeof_expr = &expr->as.sizeof_expr;

    if (sizeof_expr->type_operand != NULL)
    {
        /* sizeof(Type) - compile-time size of the type.
         * String and array types are RtHandle (uint32_t = 4 bytes). */
        const char *c_type = get_c_param_type(gen->arena, sizeof_expr->type_operand);
        return arena_sprintf(gen->arena, "(long long)sizeof(%s)", c_type);
    }
    else
    {
        /* sizeof(expr) - size of the expression's type */
        Type *expr_type = sizeof_expr->expr_operand->expr_type;
        const char *c_type = get_c_param_type(gen->arena, expr_type);
        return arena_sprintf(gen->arena, "(long long)sizeof(%s)", c_type);
    }
}

char *code_gen_typeof_expression(CodeGen *gen, Expr *expr)
{
    DEBUG_VERBOSE("Generating typeof expression");

    TypeofExpr *typeof_expr = &expr->as.typeof_expr;

    if (typeof_expr->type_literal != NULL)
    {
        /* typeof(int), typeof(str), etc. - compile-time constant */
        return arena_sprintf(gen->arena, "%s", get_type_tag_constant(typeof_expr->type_literal->kind));
    }
    else
    {
        /* typeof(value) - get runtime type tag */
        char *operand_code = code_gen_expression(gen, typeof_expr->operand);
        Type *operand_type = typeof_expr->operand->expr_type;

        if (operand_type->kind == TYPE_ANY)
        {
            /* For any type, get the runtime tag */
            return arena_sprintf(gen->arena, "rt_any_get_tag(%s)", operand_code);
        }
        else
        {
            /* For concrete types, return compile-time constant */
            return arena_sprintf(gen->arena, "%s", get_type_tag_constant(operand_type->kind));
        }
    }
}

char *code_gen_is_expression(CodeGen *gen, Expr *expr)
{
    DEBUG_VERBOSE("Generating is expression");

    IsExpr *is_expr = &expr->as.is_expr;
    char *operand_code = code_gen_expression(gen, is_expr->operand);
    const char *type_tag = get_type_tag_constant(is_expr->check_type->kind);

    /* For array types, also check the element type tag */
    if (is_expr->check_type->kind == TYPE_ARRAY &&
        is_expr->check_type->as.array.element_type != NULL)
    {
        Type *elem_type = is_expr->check_type->as.array.element_type;
        const char *elem_tag = get_type_tag_constant(elem_type->kind);
        return arena_sprintf(gen->arena, "((%s).tag == %s && (%s).element_tag == %s)",
                             operand_code, type_tag, operand_code, elem_tag);
    }

    /* For struct types, use the struct type ID for runtime type checking */
    if (is_expr->check_type->kind == TYPE_STRUCT)
    {
        int type_id = get_struct_type_id(is_expr->check_type);
        return arena_sprintf(gen->arena, "rt_any_is_struct_type(%s, %d)",
                             operand_code, type_id);
    }

    return arena_sprintf(gen->arena, "((%s).tag == %s)", operand_code, type_tag);
}

/* Helper to check if a type kind is numeric (for numeric type casts) */
static bool is_numeric_kind(TypeKind kind)
{
    return kind == TYPE_INT || kind == TYPE_INT32 ||
           kind == TYPE_UINT || kind == TYPE_UINT32 ||
           kind == TYPE_LONG || kind == TYPE_DOUBLE ||
           kind == TYPE_FLOAT || kind == TYPE_BYTE ||
           kind == TYPE_CHAR;
}

char *code_gen_as_type_expression(CodeGen *gen, Expr *expr)
{
    DEBUG_VERBOSE("Generating as type expression");

    AsTypeExpr *as_type = &expr->as.as_type;
    char *operand_code = code_gen_expression(gen, as_type->operand);
    Type *target_type = as_type->target_type;
    Type *operand_type = as_type->operand->expr_type;

    /* Check if this is an any[] to T[] cast */
    if (operand_type != NULL &&
        operand_type->kind == TYPE_ARRAY &&
        operand_type->as.array.element_type != NULL &&
        operand_type->as.array.element_type->kind == TYPE_ANY &&
        target_type->kind == TYPE_ARRAY &&
        target_type->as.array.element_type != NULL)
    {
        Type *target_elem = target_type->as.array.element_type;
        const char *conv_func = NULL;
        switch (target_elem->kind)
        {
        case TYPE_INT:
        case TYPE_LONG:
            conv_func = "rt_array_from_any_long";
            break;
        case TYPE_INT32:
            conv_func = "rt_array_from_any_int32";
            break;
        case TYPE_UINT:
            conv_func = "rt_array_from_any_uint";
            break;
        case TYPE_UINT32:
            conv_func = "rt_array_from_any_uint32";
            break;
        case TYPE_DOUBLE:
            conv_func = "rt_array_from_any_double";
            break;
        case TYPE_FLOAT:
            conv_func = "rt_array_from_any_float";
            break;
        case TYPE_CHAR:
            conv_func = "rt_array_from_any_char";
            break;
        case TYPE_BOOL:
            conv_func = "rt_array_from_any_bool";
            break;
        case TYPE_BYTE:
            conv_func = "rt_array_from_any_byte";
            break;
        case TYPE_STRING:
            conv_func = "rt_array_from_any_string";
            break;
        default:
            break;
        }
        if (conv_func != NULL)
        {
            if (gen->current_arena_var != NULL)
            {
                /* In handle mode: pin the source any[] handle, call legacy from_any,
                 * convert the raw result into a new handle. */
                if (target_elem->kind == TYPE_STRING)
                {
                    /* Strings need special handling: legacy from_any returns char**
                     * (8-byte pointers) but handle arrays store RtHandleV2* (8-byte).
                     * Use dedicated conversion that rt_arena_v2_strdup's each element.
                     * Must use rt_array_data_v2 to get past the V2 metadata header. */
                    return arena_sprintf(gen->arena,
                        "rt_array_from_legacy_string_v2(%s, %s(%s, (RtAny *)rt_array_data_v2(%s)))",
                        ARENA_VAR(gen), conv_func, ARENA_VAR(gen), operand_code);
                }
                /* Convert any[] to typed array: from_any returns raw pointer, wrap in handle */
                const char *elem_c = get_c_array_elem_type(gen->arena, target_elem);
                if (target_elem->kind == TYPE_STRING) {
                    return arena_sprintf(gen->arena,
                        "({ %s *__conv_data = %s(%s, (RtAny *)rt_array_data_v2(%s)); "
                        "rt_array_create_string_v2(%s, rt_v2_data_array_length((void *)__conv_data), __conv_data); })",
                        elem_c, conv_func, ARENA_VAR(gen), operand_code, ARENA_VAR(gen));
                }
                return arena_sprintf(gen->arena,
                    "({ %s *__conv_data = %s(%s, (RtAny *)rt_array_data_v2(%s)); "
                    "rt_array_create_generic_v2(%s, rt_v2_data_array_length((void *)__conv_data), sizeof(%s), __conv_data); })",
                    elem_c, conv_func, ARENA_VAR(gen), operand_code, ARENA_VAR(gen), elem_c);
            }
            return arena_sprintf(gen->arena, "%s(%s, %s)", conv_func, ARENA_VAR(gen), operand_code);
        }
    }

    /* Check if this is a numeric type cast */
    if (operand_type != NULL && target_type != NULL &&
        (is_numeric_kind(operand_type->kind) || operand_type->kind == TYPE_BOOL) &&
        is_numeric_kind(target_type->kind))
    {
        /* Generate a C-style cast for numeric conversions */
        const char *c_type = get_c_type(gen->arena, target_type);
        return arena_sprintf(gen->arena, "((%s)(%s))", c_type, operand_code);
    }

    /* Use the unbox helper function for single any values */
    return code_gen_unbox_value(gen, operand_code, target_type);
}
