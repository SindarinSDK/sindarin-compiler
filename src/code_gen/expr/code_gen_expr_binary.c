#include "code_gen/expr/code_gen_expr_binary.h"
#include "code_gen/expr/code_gen_expr.h"
#include "code_gen/util/code_gen_util.h"
#include "debug.h"
#include <stdlib.h>
#include <stdio.h>

/* Helper to determine if a type is numeric */
static bool is_numeric(Type *type)
{
    return type && (type->kind == TYPE_INT || type->kind == TYPE_LONG || type->kind == TYPE_DOUBLE);
}

/* Helper to get the promoted type for binary operations with mixed numeric types */
static Type *get_binary_promoted_type(Type *left, Type *right)
{
    if (left == NULL || right == NULL) return left;

    /* If both are numeric, promote to the wider type */
    if (is_numeric(left) && is_numeric(right))
    {
        /* double is the widest */
        if (left->kind == TYPE_DOUBLE || right->kind == TYPE_DOUBLE)
        {
            /* Return whichever is double */
            return left->kind == TYPE_DOUBLE ? left : right;
        }
        /* long is wider than int */
        if (left->kind == TYPE_LONG || right->kind == TYPE_LONG)
        {
            return left->kind == TYPE_LONG ? left : right;
        }
    }
    /* Otherwise use left type */
    return left;
}

char *code_gen_binary_expression(CodeGen *gen, BinaryExpr *expr)
{
    DEBUG_VERBOSE("Entering code_gen_binary_expression");

    /* Try constant folding first - if both operands are constants,
       evaluate at compile time and emit a direct literal */
    char *folded = try_constant_fold_binary(gen, expr);
    if (folded != NULL)
    {
        return folded;
    }

    /* For string/array operations, operands must be raw pointers (char *, type *).
     * Force expr_as_handle=false so handle variables get pinned and produce raw pointers. */
    Type *left_type = expr->left->expr_type;
    Type *right_type = expr->right->expr_type;
    bool saved_as_handle = gen->expr_as_handle;
    if ((left_type && is_handle_type(left_type)) || (right_type && is_handle_type(right_type)))
    {
        gen->expr_as_handle = false;
    }
    char *left_str = code_gen_expression(gen, expr->left);
    char *right_str = code_gen_expression(gen, expr->right);
    gen->expr_as_handle = saved_as_handle;
    /* Use promoted type for mixed numeric operations */
    Type *type = get_binary_promoted_type(left_type, right_type);
    SnTokenType op = expr->operator;
    if (op == TOKEN_AND)
    {
        return arena_sprintf(gen->arena, "((%s != 0 && %s != 0) ? 1L : 0L)", left_str, right_str);
    }
    else if (op == TOKEN_OR)
    {
        return arena_sprintf(gen->arena, "((%s != 0 || %s != 0) ? 1L : 0L)", left_str, right_str);
    }

    // Handle array comparison (== and !=)
    if (type->kind == TYPE_ARRAY && (op == TOKEN_EQUAL_EQUAL || op == TOKEN_BANG_EQUAL))
    {
        Type *elem_type = type->as.array.element_type;

        /* String arrays in arena mode: use handle-based comparison.
         * Re-evaluate operands in handle mode since they were pinned above. */
        if (elem_type->kind == TYPE_STRING && gen->current_arena_var != NULL)
        {
            gen->expr_as_handle = true;
            char *left_h = code_gen_expression(gen, expr->left);
            char *right_h = code_gen_expression(gen, expr->right);
            gen->expr_as_handle = saved_as_handle;
            if (op == TOKEN_EQUAL_EQUAL)
            {
                return arena_sprintf(gen->arena, "rt_array_eq_string_h(%s, %s, %s)",
                                     ARENA_VAR(gen), left_h, right_h);
            }
            else
            {
                return arena_sprintf(gen->arena, "(!rt_array_eq_string_h(%s, %s, %s))",
                                     ARENA_VAR(gen), left_h, right_h);
            }
        }

        const char *arr_suffix;
        switch (elem_type->kind)
        {
            case TYPE_INT:
            case TYPE_LONG:
                arr_suffix = "long";
                break;
            case TYPE_INT32:
                arr_suffix = "int32";
                break;
            case TYPE_UINT:
                arr_suffix = "uint";
                break;
            case TYPE_UINT32:
                arr_suffix = "uint32";
                break;
            case TYPE_FLOAT:
                arr_suffix = "float";
                break;
            case TYPE_DOUBLE:
                arr_suffix = "double";
                break;
            case TYPE_CHAR:
                arr_suffix = "char";
                break;
            case TYPE_BOOL:
                arr_suffix = "bool";
                break;
            case TYPE_BYTE:
                arr_suffix = "byte";
                break;
            case TYPE_STRING:
                arr_suffix = "string";
                break;
            default:
                fprintf(stderr, "Error: Unsupported array element type for comparison\n");
                exit(1);
        }
        if (op == TOKEN_EQUAL_EQUAL)
        {
            return arena_sprintf(gen->arena, "rt_array_eq_%s(%s, %s)", arr_suffix, left_str, right_str);
        }
        else
        {
            return arena_sprintf(gen->arena, "(!rt_array_eq_%s(%s, %s))", arr_suffix, left_str, right_str);
        }
    }

    // Handle pointer comparison (== and !=) with native C operators
    if ((type->kind == TYPE_POINTER || type->kind == TYPE_NIL ||
         left_type->kind == TYPE_POINTER || right_type->kind == TYPE_POINTER ||
         left_type->kind == TYPE_NIL || right_type->kind == TYPE_NIL) &&
        (op == TOKEN_EQUAL_EQUAL || op == TOKEN_BANG_EQUAL))
    {
        const char *c_op = (op == TOKEN_EQUAL_EQUAL) ? "==" : "!=";
        return arena_sprintf(gen->arena, "((%s) %s (%s))", left_str, c_op, right_str);
    }

    // Handle struct comparison (== and !=) using memcmp
    if (type->kind == TYPE_STRUCT && (op == TOKEN_EQUAL_EQUAL || op == TOKEN_BANG_EQUAL))
    {
        const char *struct_name = sn_mangle_name(gen->arena, type->as.struct_type.name);
        if (op == TOKEN_EQUAL_EQUAL)
        {
            return arena_sprintf(gen->arena, "(memcmp(&(%s), &(%s), sizeof(%s)) == 0)",
                                 left_str, right_str, struct_name);
        }
        else
        {
            return arena_sprintf(gen->arena, "(memcmp(&(%s), &(%s), sizeof(%s)) != 0)",
                                 left_str, right_str, struct_name);
        }
    }

    /* Bitwise operators always use native C operators (no overflow concerns) */
    if (op == TOKEN_AMPERSAND || op == TOKEN_PIPE || op == TOKEN_CARET ||
        op == TOKEN_LSHIFT || op == TOKEN_RSHIFT)
    {
        const char *c_op;
        switch (op)
        {
        case TOKEN_AMPERSAND: c_op = "&"; break;
        case TOKEN_PIPE:      c_op = "|"; break;
        case TOKEN_CARET:     c_op = "^"; break;
        case TOKEN_LSHIFT:    c_op = "<<"; break;
        case TOKEN_RSHIFT:    c_op = ">>"; break;
        default:              c_op = "&"; break;
        }
        return arena_sprintf(gen->arena, "((long long)((%s) %s (%s)))", left_str, c_op, right_str);
    }

    char *op_str = code_gen_binary_op_str(op);
    /* Arithmetic/comparison runtime functions: "long", "double", or "string" */
    char *suffix = "long";
    if (type != NULL) {
        if (type->kind == TYPE_DOUBLE || type->kind == TYPE_FLOAT)
            suffix = "double";
        else if (type->kind == TYPE_STRING)
            suffix = "string";
        else if (type->kind == TYPE_BOOL)
            suffix = "bool";
    }
    if (op == TOKEN_PLUS && type->kind == TYPE_STRING)
    {
        /* In arena context: use handle-based concat.
         * The operands are already raw pointers (pinned by variable expression).
         * If expr_as_handle: return RtHandle directly.
         * Otherwise: wrap result with pin to get raw pointer. */
        if (gen->current_arena_var != NULL)
        {
            if (gen->expr_as_handle)
            {
                return arena_sprintf(gen->arena, "rt_str_concat_h(%s, RT_HANDLE_NULL, %s, %s)",
                                     ARENA_VAR(gen), left_str, right_str);
            }
            else
            {
                return arena_sprintf(gen->arena, "(char *)rt_managed_pin(%s, rt_str_concat_h(%s, RT_HANDLE_NULL, %s, %s))",
                                     ARENA_VAR(gen), ARENA_VAR(gen), left_str, right_str);
            }
        }
        /* Non-arena context (legacy): use old approach */
        bool free_left = expression_produces_temp(expr->left);
        bool free_right = expression_produces_temp(expr->right);
        if (!free_left && !free_right)
        {
            return arena_sprintf(gen->arena, "rt_str_concat(NULL, %s, %s)", left_str, right_str);
        }
        char *free_l_str = free_left ? "rt_free_string(_left); " : "";
        char *free_r_str = free_right ? "rt_free_string(_right); " : "";
        return arena_sprintf(gen->arena, "({ char *_left = %s; char *_right = %s; char *_res = rt_str_concat(NULL, _left, _right); %s%s _res; })",
                             left_str, right_str, free_l_str, free_r_str);
    }
    else
    {
        /* Try to use native C operators in unchecked mode */
        char *native = gen_native_arithmetic(gen, left_str, right_str, op, type);
        if (native != NULL)
        {
            return native;
        }
        /* Fall back to runtime functions (checked mode or div/mod) */
        return arena_sprintf(gen->arena, "rt_%s_%s(%s, %s)", op_str, suffix, left_str, right_str);
    }
}

char *code_gen_unary_expression(CodeGen *gen, UnaryExpr *expr)
{
    DEBUG_VERBOSE("Entering code_gen_unary_expression");

    /* Try constant folding first - if operand is a constant,
       evaluate at compile time and emit a direct literal */
    char *folded = try_constant_fold_unary(gen, expr);
    if (folded != NULL)
    {
        return folded;
    }

    char *operand_str = code_gen_expression(gen, expr->operand);
    Type *type = expr->operand->expr_type;

    /* Try to use native C operators in unchecked mode */
    char *native = gen_native_unary(gen, operand_str, expr->operator, type);
    if (native != NULL)
    {
        return native;
    }

    /* Fall back to runtime functions (checked mode) */
    switch (expr->operator)
    {
    case TOKEN_MINUS:
        if (type->kind == TYPE_DOUBLE || type->kind == TYPE_FLOAT)
        {
            return arena_sprintf(gen->arena, "rt_neg_double(%s)", operand_str);
        }
        else
        {
            return arena_sprintf(gen->arena, "rt_neg_long(%s)", operand_str);
        }
    case TOKEN_BANG:
        return arena_sprintf(gen->arena, "rt_not_bool(%s)", operand_str);
    case TOKEN_TILDE:
        return arena_sprintf(gen->arena, "((long long)(~(%s)))", operand_str);
    default:
        exit(1);
    }
    return NULL;
}
