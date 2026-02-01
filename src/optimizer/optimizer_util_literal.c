// optimizer_util_literal.c
// Literal detection and no-op detection utilities

/* ============================================================================
 * Literal Detection
 * ============================================================================
 */

bool is_literal_zero(Expr *expr)
{
    if (expr == NULL) return false;
    if (expr->type != EXPR_LITERAL) return false;
    if (expr->as.literal.type == NULL) return false;

    if (expr->as.literal.type->kind == TYPE_INT ||
        expr->as.literal.type->kind == TYPE_LONG)
    {
        return expr->as.literal.value.int_value == 0;
    }
    if (expr->as.literal.type->kind == TYPE_DOUBLE)
    {
        return expr->as.literal.value.double_value == 0.0;
    }
    return false;
}

bool is_literal_one(Expr *expr)
{
    if (expr == NULL) return false;
    if (expr->type != EXPR_LITERAL) return false;
    if (expr->as.literal.type == NULL) return false;

    if (expr->as.literal.type->kind == TYPE_INT ||
        expr->as.literal.type->kind == TYPE_LONG)
    {
        return expr->as.literal.value.int_value == 1;
    }
    if (expr->as.literal.type->kind == TYPE_DOUBLE)
    {
        return expr->as.literal.value.double_value == 1.0;
    }
    return false;
}

/* ============================================================================
 * No-op Detection
 * ============================================================================
 */

bool expr_is_noop(Expr *expr, Expr **simplified)
{
    if (expr == NULL)
    {
        *simplified = NULL;
        return false;
    }

    switch (expr->type)
    {
    case EXPR_BINARY:
    {
        SnTokenType op = expr->as.binary.operator;
        Expr *left = expr->as.binary.left;
        Expr *right = expr->as.binary.right;

        /* x + 0 or 0 + x => x */
        if (op == TOKEN_PLUS)
        {
            if (is_literal_zero(right))
            {
                *simplified = left;
                return true;
            }
            if (is_literal_zero(left))
            {
                *simplified = right;
                return true;
            }
        }

        /* x - 0 => x */
        if (op == TOKEN_MINUS)
        {
            if (is_literal_zero(right))
            {
                *simplified = left;
                return true;
            }
        }

        /* x * 1 or 1 * x => x */
        if (op == TOKEN_STAR)
        {
            if (is_literal_one(right))
            {
                *simplified = left;
                return true;
            }
            if (is_literal_one(left))
            {
                *simplified = right;
                return true;
            }
            /* Note: x * 0 optimization is tricky because of side effects
               in x, so we don't do it here */
        }

        /* x / 1 => x */
        if (op == TOKEN_SLASH)
        {
            if (is_literal_one(right))
            {
                *simplified = left;
                return true;
            }
        }

        /* && with false, || with true could be simplified but
           we need to be careful about side effects */
        break;
    }

    case EXPR_UNARY:
    {
        /* !(!x) => x (double negation for boolean) */
        if (expr->as.unary.operator == TOKEN_BANG)
        {
            Expr *operand = expr->as.unary.operand;
            if (operand != NULL &&
                operand->type == EXPR_UNARY &&
                operand->as.unary.operator == TOKEN_BANG)
            {
                *simplified = operand->as.unary.operand;
                return true;
            }
        }

        /* -(-x) => x (double negation for numbers) */
        if (expr->as.unary.operator == TOKEN_MINUS)
        {
            Expr *operand = expr->as.unary.operand;
            if (operand != NULL &&
                operand->type == EXPR_UNARY &&
                operand->as.unary.operator == TOKEN_MINUS)
            {
                *simplified = operand->as.unary.operand;
                return true;
            }
        }
        break;
    }

    default:
        break;
    }

    *simplified = expr;
    return false;
}
