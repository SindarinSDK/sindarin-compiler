#include "type_checker/type_checker_expr_ops.h"
#include "type_checker/type_checker_expr.h"
#include "type_checker/type_checker_util.h"
#include "debug.h"

Type *type_check_binary(Expr *expr, SymbolTable *table)
{
    DEBUG_VERBOSE("Type checking binary expression with operator: %d", expr->as.binary.operator);
    Type *left = type_check_expr(expr->as.binary.left, table);
    Type *right = type_check_expr(expr->as.binary.right, table);
    if (left == NULL || right == NULL)
    {
        type_error(expr->token, "Invalid operand in binary expression");
        return NULL;
    }
    SnTokenType op = expr->as.binary.operator;

    /* Reject pointer arithmetic - pointers cannot be used with arithmetic operators.
     * This includes +, -, *, /, %. Pointer comparison (==, !=) with nil is still allowed. */
    if (is_arithmetic_operator(op) || op == TOKEN_PLUS)
    {
        if (left->kind == TYPE_POINTER || right->kind == TYPE_POINTER)
        {
            type_error(expr->token, "Pointer arithmetic is not allowed");
            return NULL;
        }
    }

    if (is_comparison_operator(op))
    {
        /* Allow numeric type promotion for comparisons (int vs double) */
        if (!ast_type_equals(left, right))
        {
            /* Check if both are numeric types - promotion is allowed */
            if (is_numeric_type(left) && is_numeric_type(right))
            {
                /* This is valid - int and double can be compared */
                DEBUG_VERBOSE("Numeric type promotion in comparison allowed");
            }
            else
            {
                type_error(expr->token, "Type mismatch in comparison");
                return NULL;
            }
        }
        DEBUG_VERBOSE("Returning BOOL type for comparison operator");
        return ast_create_primitive_type(table->arena, TYPE_BOOL);
    }
    else if (is_arithmetic_operator(op))
    {
        Type *promoted = get_promoted_type(table->arena, left, right);
        if (promoted == NULL)
        {
            type_error(expr->token, "Invalid types for arithmetic operator");
            return NULL;
        }
        DEBUG_VERBOSE("Returning promoted type for arithmetic operator");
        return promoted;
    }
    else if (op == TOKEN_PLUS)
    {
        /* Check for numeric type promotion */
        Type *promoted = get_promoted_type(table->arena, left, right);
        if (promoted != NULL)
        {
            DEBUG_VERBOSE("Returning promoted type for numeric + operator");
            return promoted;
        }
        else if (left->kind == TYPE_STRING && is_printable_type(right))
        {
            DEBUG_VERBOSE("Returning STRING type for string + printable");
            return left;
        }
        else if (is_printable_type(left) && right->kind == TYPE_STRING)
        {
            DEBUG_VERBOSE("Returning STRING type for printable + string");
            return right;
        }
        else
        {
            type_error(expr->token, "Invalid types for + operator");
            return NULL;
        }
    }
    else if (op == TOKEN_AND || op == TOKEN_OR)
    {
        // Logical operators require boolean operands
        if (left->kind != TYPE_BOOL || right->kind != TYPE_BOOL)
        {
            type_error(expr->token, "Logical operators require boolean operands");
            return NULL;
        }
        DEBUG_VERBOSE("Returning BOOL type for logical operator");
        return ast_create_primitive_type(table->arena, TYPE_BOOL);
    }
    else if (op == TOKEN_AMPERSAND || op == TOKEN_PIPE || op == TOKEN_CARET ||
             op == TOKEN_LSHIFT || op == TOKEN_RSHIFT)
    {
        /* Bitwise operators require integer operands (not float/double) */
        bool left_int = left->kind == TYPE_INT || left->kind == TYPE_INT32 ||
                        left->kind == TYPE_UINT || left->kind == TYPE_UINT32 ||
                        left->kind == TYPE_LONG || left->kind == TYPE_BYTE ||
                        left->kind == TYPE_CHAR;
        bool right_int = right->kind == TYPE_INT || right->kind == TYPE_INT32 ||
                         right->kind == TYPE_UINT || right->kind == TYPE_UINT32 ||
                         right->kind == TYPE_LONG || right->kind == TYPE_BYTE ||
                         right->kind == TYPE_CHAR;
        if (!left_int || !right_int)
        {
            type_error(expr->token, "Bitwise operators require integer operands");
            return NULL;
        }
        Type *promoted = get_promoted_type(table->arena, left, right);
        if (promoted == NULL)
        {
            type_error(expr->token, "Invalid types for bitwise operator");
            return NULL;
        }
        DEBUG_VERBOSE("Returning promoted type for bitwise operator");
        return promoted;
    }
    else
    {
        type_error(expr->token, "Invalid binary operator");
        return NULL;
    }
}

Type *type_check_unary(Expr *expr, SymbolTable *table)
{
    DEBUG_VERBOSE("Type checking unary expression with operator: %d", expr->as.unary.operator);
    Type *operand = type_check_expr(expr->as.unary.operand, table);
    if (operand == NULL)
    {
        type_error(expr->token, "Invalid operand in unary expression");
        return NULL;
    }
    if (expr->as.unary.operator == TOKEN_MINUS)
    {
        if (!is_numeric_type(operand))
        {
            type_error(expr->token, "Unary minus on non-numeric");
            return NULL;
        }
        DEBUG_VERBOSE("Returning operand type for unary minus");
        return operand;
    }
    else if (expr->as.unary.operator == TOKEN_BANG)
    {
        if (operand->kind != TYPE_BOOL)
        {
            type_error(expr->token, "Unary ! on non-bool");
            return NULL;
        }
        DEBUG_VERBOSE("Returning operand type for unary !");
        return operand;
    }
    else if (expr->as.unary.operator == TOKEN_TILDE)
    {
        bool is_int = operand->kind == TYPE_INT || operand->kind == TYPE_INT32 ||
                      operand->kind == TYPE_UINT || operand->kind == TYPE_UINT32 ||
                      operand->kind == TYPE_LONG || operand->kind == TYPE_BYTE ||
                      operand->kind == TYPE_CHAR;
        if (!is_int)
        {
            type_error(expr->token, "Bitwise NOT (~) requires integer operand");
            return NULL;
        }
        DEBUG_VERBOSE("Returning operand type for bitwise NOT");
        return operand;
    }
    type_error(expr->token, "Invalid unary operator");
    return NULL;
}

Type *type_check_interpolated(Expr *expr, SymbolTable *table)
{
    DEBUG_VERBOSE("Type checking interpolated string with %d parts", expr->as.interpol.part_count);
    for (int i = 0; i < expr->as.interpol.part_count; i++)
    {
        Type *part_type = type_check_expr(expr->as.interpol.parts[i], table);
        if (part_type == NULL)
        {
            type_error(expr->token, "Invalid expression in interpolated string part");
            return NULL;
        }
        if (!is_printable_type(part_type))
        {
            type_error(expr->token, "Non-printable type in interpolated string");
            return NULL;
        }
    }
    DEBUG_VERBOSE("Returning STRING type for interpolated string");
    return ast_create_primitive_type(table->arena, TYPE_STRING);
}
