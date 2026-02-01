#include "type_checker/type_checker_expr_misc.h"
#include "type_checker/type_checker_expr.h"
#include "type_checker/type_checker_stmt.h"
#include "type_checker/type_checker_util.h"
#include "debug.h"

/* Compound assignment: x += value, x -= value, x *= value, x /= value, x %= value */
Type *type_check_compound_assign(Expr *expr, SymbolTable *table)
{
    Expr *target = expr->as.compound_assign.target;
    Expr *value_expr = expr->as.compound_assign.value;
    SnTokenType op = expr->as.compound_assign.operator;

    /* Type check the target */
    Type *target_type = type_check_expr(target, table);
    if (target_type == NULL)
    {
        type_error(expr->token, "Invalid target in compound assignment");
        return NULL;
    }

    /* Type check the value */
    Type *value_type = type_check_expr(value_expr, table);
    if (value_type == NULL)
    {
        type_error(expr->token, "Invalid value in compound assignment");
        return NULL;
    }

    /* For compound assignment, the target must be a valid lvalue
     * (variable, array index, or member access) */
    if (target->type != EXPR_VARIABLE &&
        target->type != EXPR_ARRAY_ACCESS &&
        target->type != EXPR_MEMBER &&
        target->type != EXPR_MEMBER_ACCESS)
    {
        type_error(expr->token, "Compound assignment target must be a variable, array element, or struct field");
        return NULL;
    }

    /* For +=, -= on strings, only += is valid (string concatenation) */
    if (target_type->kind == TYPE_STRING)
    {
        if (op == TOKEN_PLUS)
        {
            /* String concatenation: str += value is valid if value is printable */
            if (!is_printable_type(value_type))
            {
                type_error(expr->token, "Cannot concatenate non-printable type to string");
                return NULL;
            }
            return target_type;
        }
        else
        {
            type_error(expr->token, "Only += is valid for string compound assignment");
            return NULL;
        }
    }

    /* For bitwise compound operators, require integer types */
    if (op == TOKEN_AMPERSAND || op == TOKEN_PIPE || op == TOKEN_CARET ||
        op == TOKEN_LSHIFT || op == TOKEN_RSHIFT)
    {
        bool target_int = target_type->kind == TYPE_INT || target_type->kind == TYPE_INT32 ||
                          target_type->kind == TYPE_UINT || target_type->kind == TYPE_UINT32 ||
                          target_type->kind == TYPE_LONG || target_type->kind == TYPE_BYTE ||
                          target_type->kind == TYPE_CHAR;
        bool value_int = value_type->kind == TYPE_INT || value_type->kind == TYPE_INT32 ||
                         value_type->kind == TYPE_UINT || value_type->kind == TYPE_UINT32 ||
                         value_type->kind == TYPE_LONG || value_type->kind == TYPE_BYTE ||
                         value_type->kind == TYPE_CHAR;
        if (!target_int || !value_int)
        {
            type_error(expr->token, "Bitwise compound assignment requires integer operands");
            return NULL;
        }
        return target_type;
    }

    /* For all other operators, target must be numeric */
    if (!is_numeric_type(target_type))
    {
        type_error(expr->token, "Compound assignment requires numeric target type");
        return NULL;
    }

    /* Value must also be numeric */
    if (!is_numeric_type(value_type))
    {
        type_error(expr->token, "Compound assignment value must be numeric");
        return NULL;
    }

    /* Result type is the target type */
    DEBUG_VERBOSE("Compound assignment type check passed: target type %d, op %d", target_type->kind, op);
    return target_type;
}

/* Match expression type checking */
Type *type_check_match(Expr *expr, SymbolTable *table)
{
    MatchExpr *match = &expr->as.match_expr;

    /* Type check the subject expression */
    Type *subject_type = type_check_expr(match->subject, table);
    if (subject_type == NULL)
    {
        type_error(expr->token, "Invalid match subject expression");
        return NULL;
    }

    bool has_else = false;
    Type *arm_result_type = NULL;
    bool all_arms_same_type = true;

    for (int i = 0; i < match->arm_count; i++)
    {
        MatchArm *arm = &match->arms[i];

        if (arm->is_else)
        {
            has_else = true;
        }
        else
        {
            /* Type check each pattern expression */
            for (int j = 0; j < arm->pattern_count; j++)
            {
                Type *pattern_type = type_check_expr(arm->patterns[j], table);
                if (pattern_type == NULL)
                {
                    type_error(expr->token, "Invalid match arm pattern");
                    return NULL;
                }
                if (!ast_type_equals(pattern_type, subject_type))
                {
                    /* Allow numeric type widening (int patterns matching int subject) */
                    bool compatible = false;
                    if ((subject_type->kind == TYPE_INT || subject_type->kind == TYPE_INT32 ||
                         subject_type->kind == TYPE_UINT || subject_type->kind == TYPE_UINT32 ||
                         subject_type->kind == TYPE_LONG) &&
                        (pattern_type->kind == TYPE_INT || pattern_type->kind == TYPE_INT32 ||
                         pattern_type->kind == TYPE_UINT || pattern_type->kind == TYPE_UINT32 ||
                         pattern_type->kind == TYPE_LONG))
                    {
                        compatible = true;
                    }
                    if ((subject_type->kind == TYPE_DOUBLE || subject_type->kind == TYPE_FLOAT) &&
                        (pattern_type->kind == TYPE_DOUBLE || pattern_type->kind == TYPE_FLOAT))
                    {
                        compatible = true;
                    }
                    if (!compatible)
                    {
                        type_error(arm->patterns[j]->token, "Match arm pattern type does not match subject type");
                    }
                }
            }
        }

        /* Type check the arm body */
        if (arm->body != NULL)
        {
            type_check_stmt(arm->body, table, NULL);

            /* Determine arm result type from last expression in body */
            if (arm->body->type == STMT_BLOCK && arm->body->as.block.count > 0)
            {
                Stmt *last_stmt = arm->body->as.block.statements[arm->body->as.block.count - 1];
                if (last_stmt->type == STMT_EXPR && last_stmt->as.expression.expression != NULL)
                {
                    Type *this_arm_type = last_stmt->as.expression.expression->expr_type;
                    if (this_arm_type != NULL && this_arm_type->kind != TYPE_VOID)
                    {
                        if (arm_result_type == NULL)
                        {
                            arm_result_type = this_arm_type;
                        }
                        else if (!ast_type_equals(arm_result_type, this_arm_type))
                        {
                            all_arms_same_type = false;
                        }
                    }
                    else
                    {
                        all_arms_same_type = false;
                    }
                }
                else
                {
                    all_arms_same_type = false;
                }
            }
            else
            {
                all_arms_same_type = false;
            }
        }
    }

    /* If all arms have the same type and else exists, match is an expression */
    if (has_else && all_arms_same_type && arm_result_type != NULL)
    {
        return arm_result_type;
    }
    else
    {
        return ast_create_primitive_type(table->arena, TYPE_VOID);
    }
}
