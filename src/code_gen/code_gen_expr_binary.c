#include "code_gen/code_gen_expr_binary.h"
#include "code_gen/code_gen_expr.h"
#include "code_gen/code_gen_util.h"
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

    char *left_str = code_gen_expression(gen, expr->left);
    char *right_str = code_gen_expression(gen, expr->right);
    Type *left_type = expr->left->expr_type;
    Type *right_type = expr->right->expr_type;
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
        const char *struct_name = type->as.struct_type.name;
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

    char *op_str = code_gen_binary_op_str(op);
    char *suffix = code_gen_type_suffix(type);
    if (op == TOKEN_PLUS && type->kind == TYPE_STRING)
    {
        bool free_left = expression_produces_temp(expr->left);
        bool free_right = expression_produces_temp(expr->right);
        // Optimization: Direct call if no temps (common for literals/variables).
        if (!free_left && !free_right)
        {
            return arena_sprintf(gen->arena, "rt_str_concat(%s, %s, %s)", ARENA_VAR(gen), left_str, right_str);
        }
        // Otherwise, use temps/block for safe freeing (skip freeing in arena context).
        char *free_l_str = (free_left && gen->current_arena_var == NULL) ? "rt_free_string(_left); " : "";
        char *free_r_str = (free_right && gen->current_arena_var == NULL) ? "rt_free_string(_right); " : "";
        return arena_sprintf(gen->arena, "({ char *_left = %s; char *_right = %s; char *_res = rt_str_concat(%s, _left, _right); %s%s _res; })",
                             left_str, right_str, ARENA_VAR(gen), free_l_str, free_r_str);
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
    default:
        exit(1);
    }
    return NULL;
}
