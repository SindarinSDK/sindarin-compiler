#include "code_gen/util/code_gen_util.h"
#include <string.h>

/* ============================================================================
 * Constant Folding Optimization
 * ============================================================================
 * These functions detect compile-time constant expressions and evaluate them
 * at compile time to generate direct literals instead of runtime function calls.
 * For example: rt_add_long(5L, 3L) becomes 8L
 */

bool is_constant_expr(Expr *expr)
{
    if (expr == NULL)
    {
        return false;
    }

    switch (expr->type)
    {
    case EXPR_LITERAL:
    {
        /* Literals are constant if they're numeric or boolean */
        Type *type = expr->as.literal.type;
        if (type == NULL) return false;
        return type->kind == TYPE_INT ||
               type->kind == TYPE_INT32 ||
               type->kind == TYPE_UINT ||
               type->kind == TYPE_UINT32 ||
               type->kind == TYPE_LONG ||
               type->kind == TYPE_DOUBLE ||
               type->kind == TYPE_FLOAT ||
               type->kind == TYPE_BOOL;
    }
    case EXPR_BINARY:
    {
        /* Binary expressions are constant if both operands are constant
           and the operator is a foldable arithmetic/comparison op */
        SnTokenType op = expr->as.binary.operator;
        /* Only fold arithmetic and comparison operations */
        if (op == TOKEN_PLUS || op == TOKEN_MINUS ||
            op == TOKEN_STAR || op == TOKEN_SLASH ||
            op == TOKEN_MODULO ||
            op == TOKEN_EQUAL_EQUAL || op == TOKEN_BANG_EQUAL ||
            op == TOKEN_LESS || op == TOKEN_LESS_EQUAL ||
            op == TOKEN_GREATER || op == TOKEN_GREATER_EQUAL ||
            op == TOKEN_AND || op == TOKEN_OR)
        {
            return is_constant_expr(expr->as.binary.left) &&
                   is_constant_expr(expr->as.binary.right);
        }
        return false;
    }
    case EXPR_UNARY:
    {
        /* Unary expressions are constant if the operand is constant */
        SnTokenType op = expr->as.unary.operator;
        if (op == TOKEN_MINUS || op == TOKEN_BANG)
        {
            return is_constant_expr(expr->as.unary.operand);
        }
        return false;
    }
    default:
        return false;
    }
}

bool try_fold_constant(Expr *expr, int64_t *out_int_value, double *out_double_value, bool *out_is_double)
{
    if (expr == NULL)
    {
        return false;
    }

    switch (expr->type)
    {
    case EXPR_LITERAL:
    {
        Type *type = expr->as.literal.type;
        if (type == NULL) return false;

        switch (type->kind)
        {
        case TYPE_INT:
        case TYPE_INT32:
        case TYPE_UINT:
        case TYPE_UINT32:
        case TYPE_LONG:
            *out_int_value = expr->as.literal.value.int_value;
            *out_is_double = false;
            return true;
        case TYPE_DOUBLE:
        case TYPE_FLOAT:
            *out_double_value = expr->as.literal.value.double_value;
            *out_is_double = true;
            return true;
        case TYPE_BOOL:
            *out_int_value = expr->as.literal.value.bool_value ? 1 : 0;
            *out_is_double = false;
            return true;
        default:
            return false;
        }
    }

    case EXPR_UNARY:
    {
        int64_t operand_int;
        double operand_double;
        bool operand_is_double;

        if (!try_fold_constant(expr->as.unary.operand, &operand_int, &operand_double, &operand_is_double))
        {
            return false;
        }

        SnTokenType op = expr->as.unary.operator;
        switch (op)
        {
        case TOKEN_MINUS:
            if (operand_is_double)
            {
                *out_double_value = -operand_double;
                *out_is_double = true;
            }
            else
            {
                *out_int_value = -operand_int;
                *out_is_double = false;
            }
            return true;
        case TOKEN_BANG:
            /* Logical not - result is always an integer (boolean) */
            if (operand_is_double)
            {
                *out_int_value = (operand_double == 0.0) ? 1 : 0;
            }
            else
            {
                *out_int_value = (operand_int == 0) ? 1 : 0;
            }
            *out_is_double = false;
            return true;
        case TOKEN_TILDE:
            if (operand_is_double) return false;
            *out_int_value = ~operand_int;
            *out_is_double = false;
            return true;
        default:
            return false;
        }
    }

    case EXPR_BINARY:
    {
        int64_t left_int, right_int;
        double left_double, right_double;
        bool left_is_double, right_is_double;

        if (!try_fold_constant(expr->as.binary.left, &left_int, &left_double, &left_is_double))
        {
            return false;
        }
        if (!try_fold_constant(expr->as.binary.right, &right_int, &right_double, &right_is_double))
        {
            return false;
        }

        /* Promote to double if either operand is double */
        bool result_is_double = left_is_double || right_is_double;

        /* Convert to common type */
        double left_val = left_is_double ? left_double : (double)left_int;
        double right_val = right_is_double ? right_double : (double)right_int;

        SnTokenType op = expr->as.binary.operator;

        /* For comparison and logical operators, result is always integer (bool) */
        if (op == TOKEN_EQUAL_EQUAL || op == TOKEN_BANG_EQUAL ||
            op == TOKEN_LESS || op == TOKEN_LESS_EQUAL ||
            op == TOKEN_GREATER || op == TOKEN_GREATER_EQUAL ||
            op == TOKEN_AND || op == TOKEN_OR)
        {
            *out_is_double = false;
            switch (op)
            {
            case TOKEN_EQUAL_EQUAL:
                *out_int_value = (left_val == right_val) ? 1 : 0;
                return true;
            case TOKEN_BANG_EQUAL:
                *out_int_value = (left_val != right_val) ? 1 : 0;
                return true;
            case TOKEN_LESS:
                *out_int_value = (left_val < right_val) ? 1 : 0;
                return true;
            case TOKEN_LESS_EQUAL:
                *out_int_value = (left_val <= right_val) ? 1 : 0;
                return true;
            case TOKEN_GREATER:
                *out_int_value = (left_val > right_val) ? 1 : 0;
                return true;
            case TOKEN_GREATER_EQUAL:
                *out_int_value = (left_val >= right_val) ? 1 : 0;
                return true;
            case TOKEN_AND:
                *out_int_value = ((left_val != 0) && (right_val != 0)) ? 1 : 0;
                return true;
            case TOKEN_OR:
                *out_int_value = ((left_val != 0) || (right_val != 0)) ? 1 : 0;
                return true;
            default:
                return false;
            }
        }

        /* Arithmetic operations */
        *out_is_double = result_is_double;

        if (result_is_double)
        {
            switch (op)
            {
            case TOKEN_PLUS:
                *out_double_value = left_val + right_val;
                return true;
            case TOKEN_MINUS:
                *out_double_value = left_val - right_val;
                return true;
            case TOKEN_STAR:
                *out_double_value = left_val * right_val;
                return true;
            case TOKEN_SLASH:
                if (right_val == 0.0)
                {
                    /* Division by zero - don't fold, let runtime handle */
                    return false;
                }
                *out_double_value = left_val / right_val;
                return true;
            case TOKEN_MODULO:
                /* Modulo on doubles is not standard - don't fold */
                return false;
            default:
                return false;
            }
        }
        else
        {
            /* Integer arithmetic */
            switch (op)
            {
            case TOKEN_PLUS:
                *out_int_value = left_int + right_int;
                return true;
            case TOKEN_MINUS:
                *out_int_value = left_int - right_int;
                return true;
            case TOKEN_STAR:
                *out_int_value = left_int * right_int;
                return true;
            case TOKEN_SLASH:
                if (right_int == 0)
                {
                    /* Division by zero - don't fold, let runtime handle */
                    return false;
                }
                *out_int_value = left_int / right_int;
                return true;
            case TOKEN_MODULO:
                if (right_int == 0)
                {
                    /* Division by zero - don't fold, let runtime handle */
                    return false;
                }
                *out_int_value = left_int % right_int;
                return true;
            case TOKEN_AMPERSAND:
                *out_int_value = left_int & right_int;
                return true;
            case TOKEN_PIPE:
                *out_int_value = left_int | right_int;
                return true;
            case TOKEN_CARET:
                *out_int_value = left_int ^ right_int;
                return true;
            case TOKEN_LSHIFT:
                *out_int_value = left_int << right_int;
                return true;
            case TOKEN_RSHIFT:
                *out_int_value = left_int >> right_int;
                return true;
            default:
                return false;
            }
        }
    }

    default:
        return false;
    }
}

char *try_constant_fold_binary(CodeGen *gen, BinaryExpr *expr)
{
    /* Create a temporary Expr wrapper to use try_fold_constant */
    Expr temp_expr;
    temp_expr.type = EXPR_BINARY;
    temp_expr.as.binary = *expr;

    int64_t int_result;
    double double_result;
    bool is_double;

    if (!try_fold_constant(&temp_expr, &int_result, &double_result, &is_double))
    {
        return NULL;
    }

    if (is_double)
    {
        char *str = arena_sprintf(gen->arena, "%.17g", double_result);
        /* Ensure the literal has a decimal point for doubles */
        if (strchr(str, '.') == NULL && strchr(str, 'e') == NULL && strchr(str, 'E') == NULL)
        {
            str = arena_sprintf(gen->arena, "%s.0", str);
        }
        return str;
    }
    else
    {
        return arena_sprintf(gen->arena, "%lldLL", (long long)int_result);
    }
}

char *try_constant_fold_unary(CodeGen *gen, UnaryExpr *expr)
{
    /* Create a temporary Expr wrapper to use try_fold_constant */
    Expr temp_expr;
    temp_expr.type = EXPR_UNARY;
    temp_expr.as.unary = *expr;

    int64_t int_result;
    double double_result;
    bool is_double;

    if (!try_fold_constant(&temp_expr, &int_result, &double_result, &is_double))
    {
        return NULL;
    }

    if (is_double)
    {
        char *str = arena_sprintf(gen->arena, "%.17g", double_result);
        /* Ensure the literal has a decimal point for doubles */
        if (strchr(str, '.') == NULL && strchr(str, 'e') == NULL && strchr(str, 'E') == NULL)
        {
            str = arena_sprintf(gen->arena, "%s.0", str);
        }
        return str;
    }
    else
    {
        return arena_sprintf(gen->arena, "%lldLL", (long long)int_result);
    }
}
