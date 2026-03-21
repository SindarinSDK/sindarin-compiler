#include "type_checker/expr/type_checker_expr_ops.h"
#include "type_checker/expr/type_checker_expr.h"
#include "type_checker/util/type_checker_util.h"
#include "debug.h"
#include <string.h>

/* Look up an operator method on a struct type by operator token.
 * Returns the matching StructMethod, or NULL if none exists. */
static StructMethod *find_operator_method(Type *struct_type, SnTokenType op)
{
    if (struct_type == NULL || struct_type->kind != TYPE_STRUCT)
    {
        return NULL;
    }
    for (int i = 0; i < struct_type->as.struct_type.method_count; i++)
    {
        StructMethod *m = &struct_type->as.struct_type.methods[i];
        if (m->is_operator && m->operator_token == op)
        {
            return m;
        }
    }
    return NULL;
}

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

    /* Struct operator dispatch: if the left operand is a struct, check for an
     * operator method matching the operator. Both operands must be the same
     * struct type. */
    if (left->kind == TYPE_STRUCT)
    {
        /* Direct operator method lookup */
        StructMethod *op_method = find_operator_method(left, op);
        if (op_method != NULL)
        {
            /* Verify the right operand is the same struct type */
            if (right->kind != TYPE_STRUCT ||
                right->as.struct_type.name == NULL ||
                left->as.struct_type.name == NULL ||
                strcmp(left->as.struct_type.name, right->as.struct_type.name) != 0)
            {
                char msg[512];
                const char *sname = left->as.struct_type.name != NULL
                                        ? left->as.struct_type.name : "<unknown>";
                const char *op_sym = (op == TOKEN_EQUAL_EQUAL)   ? "=="
                                   : (op == TOKEN_BANG_EQUAL)     ? "!="
                                   : (op == TOKEN_LESS)           ? "<"
                                   : (op == TOKEN_LESS_EQUAL)     ? "<="
                                   : (op == TOKEN_GREATER)        ? ">"
                                   : (op == TOKEN_GREATER_EQUAL)  ? ">="
                                   : "?";
                snprintf(msg, sizeof(msg),
                         "operator '%s': right operand must be of type '%s'",
                         op_sym, sname);
                type_error(expr->token, msg);
                return NULL;
            }
            expr->as.binary.operator_method = op_method;
            expr->as.binary.is_derived_operator = false;
            expr->as.binary.derived_from = op;
            DEBUG_VERBOSE("Resolved struct operator method '%s' for operator %d",
                          op_method->name, op);
            return ast_create_primitive_type(table->arena, TYPE_BOOL);
        }

        /* Derived != from ==: if the struct has __op_eq__ but not __op_ne__,
         * allow != by negating the == result. */
        if (op == TOKEN_BANG_EQUAL)
        {
            StructMethod *eq_method = find_operator_method(left, TOKEN_EQUAL_EQUAL);
            if (eq_method != NULL)
            {
                if (right->kind != TYPE_STRUCT ||
                    right->as.struct_type.name == NULL ||
                    left->as.struct_type.name == NULL ||
                    strcmp(left->as.struct_type.name, right->as.struct_type.name) != 0)
                {
                    char msg[512];
                    const char *sname = left->as.struct_type.name != NULL
                                            ? left->as.struct_type.name : "<unknown>";
                    snprintf(msg, sizeof(msg),
                             "Operator '!=': right operand must be of type '%s'",
                             sname);
                    type_error(expr->token, msg);
                    return NULL;
                }
                expr->as.binary.operator_method = eq_method;
                expr->as.binary.is_derived_operator = true;
                expr->as.binary.derived_from = TOKEN_EQUAL_EQUAL;
                DEBUG_VERBOSE("Derived != from struct operator method '%s'", eq_method->name);
                return ast_create_primitive_type(table->arena, TYPE_BOOL);
            }
        }

        /* Derived > from <: a > b → b.lt(a) (swap args) */
        if (op == TOKEN_GREATER)
        {
            StructMethod *lt_method = find_operator_method(left, TOKEN_LESS);
            if (lt_method != NULL)
            {
                if (right->kind != TYPE_STRUCT ||
                    right->as.struct_type.name == NULL ||
                    left->as.struct_type.name == NULL ||
                    strcmp(left->as.struct_type.name, right->as.struct_type.name) != 0)
                {
                    char msg[512];
                    const char *sname = left->as.struct_type.name != NULL
                                            ? left->as.struct_type.name : "<unknown>";
                    snprintf(msg, sizeof(msg),
                             "Operator '>': right operand must be of type '%s'",
                             sname);
                    type_error(expr->token, msg);
                    return NULL;
                }
                expr->as.binary.operator_method = lt_method;
                expr->as.binary.is_derived_operator = false;
                expr->as.binary.is_swapped_operator = true;
                DEBUG_VERBOSE("Derived > from struct operator method '%s' (swap args)", lt_method->name);
                return ast_create_primitive_type(table->arena, TYPE_BOOL);
            }
        }

        /* Derived >= from <: a >= b → !(a.lt(b)) (negate, no swap) */
        if (op == TOKEN_GREATER_EQUAL)
        {
            StructMethod *lt_method = find_operator_method(left, TOKEN_LESS);
            if (lt_method != NULL)
            {
                if (right->kind != TYPE_STRUCT ||
                    right->as.struct_type.name == NULL ||
                    left->as.struct_type.name == NULL ||
                    strcmp(left->as.struct_type.name, right->as.struct_type.name) != 0)
                {
                    char msg[512];
                    const char *sname = left->as.struct_type.name != NULL
                                            ? left->as.struct_type.name : "<unknown>";
                    snprintf(msg, sizeof(msg),
                             "Operator '>=': right operand must be of type '%s'",
                             sname);
                    type_error(expr->token, msg);
                    return NULL;
                }
                expr->as.binary.operator_method = lt_method;
                expr->as.binary.is_derived_operator = true;
                expr->as.binary.is_swapped_operator = false;
                DEBUG_VERBOSE("Derived >= from struct operator method '%s' (negate)", lt_method->name);
                return ast_create_primitive_type(table->arena, TYPE_BOOL);
            }
        }

        /* Derived <= from <: a <= b → !(b.lt(a)) (swap AND negate) */
        if (op == TOKEN_LESS_EQUAL)
        {
            StructMethod *lt_method = find_operator_method(left, TOKEN_LESS);
            if (lt_method != NULL)
            {
                if (right->kind != TYPE_STRUCT ||
                    right->as.struct_type.name == NULL ||
                    left->as.struct_type.name == NULL ||
                    strcmp(left->as.struct_type.name, right->as.struct_type.name) != 0)
                {
                    char msg[512];
                    const char *sname = left->as.struct_type.name != NULL
                                            ? left->as.struct_type.name : "<unknown>";
                    snprintf(msg, sizeof(msg),
                             "Operator '<=': right operand must be of type '%s'",
                             sname);
                    type_error(expr->token, msg);
                    return NULL;
                }
                expr->as.binary.operator_method = lt_method;
                expr->as.binary.is_derived_operator = true;
                expr->as.binary.is_swapped_operator = true;
                DEBUG_VERBOSE("Derived <= from struct operator method '%s' (swap+negate)", lt_method->name);
                return ast_create_primitive_type(table->arena, TYPE_BOOL);
            }
        }

        /* Left is a struct but no operator method found — this is always an
         * error: structs do not support built-in operators. */
        {
            const char *op_sym = "?";
            switch (op)
            {
                case TOKEN_EQUAL_EQUAL:   op_sym = "=="; break;
                case TOKEN_BANG_EQUAL:    op_sym = "!="; break;
                case TOKEN_LESS:          op_sym = "<";  break;
                case TOKEN_LESS_EQUAL:    op_sym = "<="; break;
                case TOKEN_GREATER:       op_sym = ">";  break;
                case TOKEN_GREATER_EQUAL: op_sym = ">="; break;
                case TOKEN_PLUS:          op_sym = "+";  break;
                case TOKEN_MINUS:         op_sym = "-";  break;
                case TOKEN_STAR:          op_sym = "*";  break;
                case TOKEN_SLASH:         op_sym = "/";  break;
                case TOKEN_MODULO:        op_sym = "%";  break;
                default: break;
            }
            char msg[512];
            const char *sname = left->as.struct_type.name != NULL
                                    ? left->as.struct_type.name : "<unknown>";
            snprintf(msg, sizeof(msg),
                     "operator '%s' is not defined for type '%s'",
                     op_sym, sname);
            type_error(expr->token, msg);
            return NULL;
        }
    }

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
