#include "optimizer/optimizer_util.h"
#include "optimizer.h"
#include <stdlib.h>
#include <string.h>

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

/* ============================================================================
 * Variable Usage Tracking
 * ============================================================================
 */

void add_used_variable(Token **used_vars, int *used_count, int *used_capacity,
                       Arena *arena, Token name)
{
    /* Check if already in list */
    for (int i = 0; i < *used_count; i++)
    {
        if ((*used_vars)[i].length == name.length &&
            strncmp((*used_vars)[i].start, name.start, name.length) == 0)
        {
            return;  /* Already tracked */
        }
    }

    /* Grow array if needed */
    if (*used_count >= *used_capacity)
    {
        int new_cap = (*used_capacity == 0) ? 16 : (*used_capacity * 2);
        Token *new_vars = arena_alloc(arena, new_cap * sizeof(Token));
        for (int i = 0; i < *used_count; i++)
        {
            new_vars[i] = (*used_vars)[i];
        }
        *used_vars = new_vars;
        *used_capacity = new_cap;
    }

    (*used_vars)[*used_count] = name;
    (*used_count)++;
}

void collect_used_variables(Expr *expr, Token **used_vars, int *used_count,
                            int *used_capacity, Arena *arena)
{
    if (expr == NULL) return;

    switch (expr->type)
    {
    case EXPR_VARIABLE:
        add_used_variable(used_vars, used_count, used_capacity, arena, expr->as.variable.name);
        break;

    case EXPR_BINARY:
        collect_used_variables(expr->as.binary.left, used_vars, used_count, used_capacity, arena);
        collect_used_variables(expr->as.binary.right, used_vars, used_count, used_capacity, arena);
        break;

    case EXPR_UNARY:
        collect_used_variables(expr->as.unary.operand, used_vars, used_count, used_capacity, arena);
        break;

    case EXPR_ASSIGN:
        /* The variable being assigned TO is not a "use" (it's a def),
           but the value being assigned IS a use */
        collect_used_variables(expr->as.assign.value, used_vars, used_count, used_capacity, arena);
        break;

    case EXPR_INDEX_ASSIGN:
        collect_used_variables(expr->as.index_assign.array, used_vars, used_count, used_capacity, arena);
        collect_used_variables(expr->as.index_assign.index, used_vars, used_count, used_capacity, arena);
        collect_used_variables(expr->as.index_assign.value, used_vars, used_count, used_capacity, arena);
        break;

    case EXPR_CALL:
        collect_used_variables(expr->as.call.callee, used_vars, used_count, used_capacity, arena);
        for (int i = 0; i < expr->as.call.arg_count; i++)
        {
            collect_used_variables(expr->as.call.arguments[i], used_vars, used_count, used_capacity, arena);
        }
        break;

    case EXPR_ARRAY:
        for (int i = 0; i < expr->as.array.element_count; i++)
        {
            collect_used_variables(expr->as.array.elements[i], used_vars, used_count, used_capacity, arena);
        }
        break;

    case EXPR_ARRAY_ACCESS:
        collect_used_variables(expr->as.array_access.array, used_vars, used_count, used_capacity, arena);
        collect_used_variables(expr->as.array_access.index, used_vars, used_count, used_capacity, arena);
        break;

    case EXPR_ARRAY_SLICE:
        collect_used_variables(expr->as.array_slice.array, used_vars, used_count, used_capacity, arena);
        if (expr->as.array_slice.start)
            collect_used_variables(expr->as.array_slice.start, used_vars, used_count, used_capacity, arena);
        if (expr->as.array_slice.end)
            collect_used_variables(expr->as.array_slice.end, used_vars, used_count, used_capacity, arena);
        if (expr->as.array_slice.step)
            collect_used_variables(expr->as.array_slice.step, used_vars, used_count, used_capacity, arena);
        break;

    case EXPR_RANGE:
        collect_used_variables(expr->as.range.start, used_vars, used_count, used_capacity, arena);
        collect_used_variables(expr->as.range.end, used_vars, used_count, used_capacity, arena);
        break;

    case EXPR_SPREAD:
        collect_used_variables(expr->as.spread.array, used_vars, used_count, used_capacity, arena);
        break;

    case EXPR_INCREMENT:
    case EXPR_DECREMENT:
        collect_used_variables(expr->as.operand, used_vars, used_count, used_capacity, arena);
        break;

    case EXPR_INTERPOLATED:
        for (int i = 0; i < expr->as.interpol.part_count; i++)
        {
            collect_used_variables(expr->as.interpol.parts[i], used_vars, used_count, used_capacity, arena);
        }
        break;

    case EXPR_MEMBER:
        collect_used_variables(expr->as.member.object, used_vars, used_count, used_capacity, arena);
        break;

    case EXPR_LAMBDA:
        /* Lambda bodies should track their own variables, but captured
           variables from outer scope count as uses */
        if (expr->as.lambda.body)
        {
            collect_used_variables(expr->as.lambda.body, used_vars, used_count, used_capacity, arena);
        }
        for (int i = 0; i < expr->as.lambda.body_stmt_count; i++)
        {
            collect_used_variables_stmt(expr->as.lambda.body_stmts[i], used_vars, used_count, used_capacity, arena);
        }
        break;

    case EXPR_STATIC_CALL:
        for (int i = 0; i < expr->as.static_call.arg_count; i++)
        {
            collect_used_variables(expr->as.static_call.arguments[i], used_vars, used_count, used_capacity, arena);
        }
        break;

    case EXPR_SIZED_ARRAY_ALLOC:
        /* Sized array allocations use both the size expression and default value */
        collect_used_variables(expr->as.sized_array_alloc.size_expr, used_vars, used_count, used_capacity, arena);
        if (expr->as.sized_array_alloc.default_value)
        {
            collect_used_variables(expr->as.sized_array_alloc.default_value, used_vars, used_count, used_capacity, arena);
        }
        break;

    case EXPR_THREAD_SPAWN:
        /* Thread spawn wraps a function call - collect variables from the call */
        collect_used_variables(expr->as.thread_spawn.call, used_vars, used_count, used_capacity, arena);
        break;

    case EXPR_THREAD_SYNC:
        /* Thread sync uses the handle expression */
        collect_used_variables(expr->as.thread_sync.handle, used_vars, used_count, used_capacity, arena);
        break;

    case EXPR_TYPEOF:
        /* typeof uses its operand expression */
        if (expr->as.typeof_expr.operand)
        {
            collect_used_variables(expr->as.typeof_expr.operand, used_vars, used_count, used_capacity, arena);
        }
        break;

    case EXPR_IS:
        /* is uses its operand expression */
        collect_used_variables(expr->as.is_expr.operand, used_vars, used_count, used_capacity, arena);
        break;

    case EXPR_AS_TYPE:
        /* as uses its operand expression */
        collect_used_variables(expr->as.as_type.operand, used_vars, used_count, used_capacity, arena);
        break;

    case EXPR_AS_VAL:
        /* as val uses its operand expression */
        collect_used_variables(expr->as.as_val.operand, used_vars, used_count, used_capacity, arena);
        break;

    case EXPR_AS_REF:
        /* as ref uses its operand expression */
        collect_used_variables(expr->as.as_ref.operand, used_vars, used_count, used_capacity, arena);
        break;

    case EXPR_STRUCT_LITERAL:
        /* Struct literal field value expressions use variables */
        for (int i = 0; i < expr->as.struct_literal.field_count; i++)
        {
            collect_used_variables(expr->as.struct_literal.fields[i].value, used_vars, used_count, used_capacity, arena);
        }
        break;

    case EXPR_MEMBER_ACCESS:
        /* Member access uses the object expression */
        collect_used_variables(expr->as.member_access.object, used_vars, used_count, used_capacity, arena);
        break;

    case EXPR_MEMBER_ASSIGN:
        /* Member assignment uses both the object and value expressions */
        collect_used_variables(expr->as.member_assign.object, used_vars, used_count, used_capacity, arena);
        collect_used_variables(expr->as.member_assign.value, used_vars, used_count, used_capacity, arena);
        break;

    case EXPR_LITERAL:
    default:
        break;
    }
}

void collect_used_variables_stmt(Stmt *stmt, Token **used_vars, int *used_count,
                                 int *used_capacity, Arena *arena)
{
    if (stmt == NULL) return;

    switch (stmt->type)
    {
    case STMT_EXPR:
        collect_used_variables(stmt->as.expression.expression, used_vars, used_count, used_capacity, arena);
        break;

    case STMT_VAR_DECL:
        /* The variable being declared is not a use, but the initializer is */
        if (stmt->as.var_decl.initializer)
        {
            collect_used_variables(stmt->as.var_decl.initializer, used_vars, used_count, used_capacity, arena);
        }
        break;

    case STMT_RETURN:
        if (stmt->as.return_stmt.value)
        {
            collect_used_variables(stmt->as.return_stmt.value, used_vars, used_count, used_capacity, arena);
        }
        break;

    case STMT_BLOCK:
        for (int i = 0; i < stmt->as.block.count; i++)
        {
            collect_used_variables_stmt(stmt->as.block.statements[i], used_vars, used_count, used_capacity, arena);
        }
        break;

    case STMT_IF:
        collect_used_variables(stmt->as.if_stmt.condition, used_vars, used_count, used_capacity, arena);
        collect_used_variables_stmt(stmt->as.if_stmt.then_branch, used_vars, used_count, used_capacity, arena);
        if (stmt->as.if_stmt.else_branch)
        {
            collect_used_variables_stmt(stmt->as.if_stmt.else_branch, used_vars, used_count, used_capacity, arena);
        }
        break;

    case STMT_WHILE:
        collect_used_variables(stmt->as.while_stmt.condition, used_vars, used_count, used_capacity, arena);
        collect_used_variables_stmt(stmt->as.while_stmt.body, used_vars, used_count, used_capacity, arena);
        break;

    case STMT_FOR:
        if (stmt->as.for_stmt.initializer)
        {
            collect_used_variables_stmt(stmt->as.for_stmt.initializer, used_vars, used_count, used_capacity, arena);
        }
        if (stmt->as.for_stmt.condition)
        {
            collect_used_variables(stmt->as.for_stmt.condition, used_vars, used_count, used_capacity, arena);
        }
        if (stmt->as.for_stmt.increment)
        {
            collect_used_variables(stmt->as.for_stmt.increment, used_vars, used_count, used_capacity, arena);
        }
        collect_used_variables_stmt(stmt->as.for_stmt.body, used_vars, used_count, used_capacity, arena);
        break;

    case STMT_FOR_EACH:
        collect_used_variables(stmt->as.for_each_stmt.iterable, used_vars, used_count, used_capacity, arena);
        collect_used_variables_stmt(stmt->as.for_each_stmt.body, used_vars, used_count, used_capacity, arena);
        break;

    case STMT_FUNCTION:
        /* Don't descend into nested function definitions for variable tracking */
        break;

    case STMT_BREAK:
    case STMT_CONTINUE:
    case STMT_IMPORT:
    default:
        break;
    }
}

bool is_variable_used(Token *used_vars, int used_count, Token name)
{
    for (int i = 0; i < used_count; i++)
    {
        if (used_vars[i].length == name.length &&
            strncmp(used_vars[i].start, name.start, name.length) == 0)
        {
            return true;
        }
    }
    return false;
}

/* ============================================================================
 * Dead Code Removal Helpers
 * ============================================================================
 */

int remove_unused_variables(Optimizer *opt, Stmt **stmts, int *count)
{
    if (stmts == NULL || *count <= 0) return 0;

    /* First, collect all variable uses in the entire block */
    Token *used_vars = NULL;
    int used_count = 0;
    int used_capacity = 0;

    for (int i = 0; i < *count; i++)
    {
        collect_used_variables_stmt(stmts[i], &used_vars, &used_count, &used_capacity, opt->arena);
    }

    /* Now filter out unused variable declarations */
    int removed = 0;
    int new_count = 0;

    for (int i = 0; i < *count; i++)
    {
        Stmt *stmt = stmts[i];

        if (stmt->type == STMT_VAR_DECL)
        {
            /* Check if this variable is used */
            if (!is_variable_used(used_vars, used_count, stmt->as.var_decl.name))
            {
                /* Variable is not used - but we can only remove it if:
                   1. It has no initializer, OR
                   2. The initializer has no side effects */
                Expr *init = stmt->as.var_decl.initializer;
                bool has_side_effects = false;

                if (init != NULL)
                {
                    /* Conservative: assume function calls and thread operations have side effects */
                    switch (init->type)
                    {
                    case EXPR_CALL:
                    case EXPR_INCREMENT:
                    case EXPR_DECREMENT:
                    case EXPR_ASSIGN:
                    case EXPR_INDEX_ASSIGN:
                    case EXPR_THREAD_SPAWN:
                    case EXPR_THREAD_SYNC:
                        has_side_effects = true;
                        break;
                    default:
                        has_side_effects = false;
                    }
                }

                if (!has_side_effects)
                {
                    removed++;
                    continue;  /* Skip this declaration */
                }
            }
        }

        /* Keep this statement */
        stmts[new_count] = stmt;
        new_count++;
    }

    *count = new_count;
    opt->variables_removed += removed;
    return removed;
}

Expr *simplify_noop_expr(Optimizer *opt, Expr *expr)
{
    if (expr == NULL) return NULL;

    /* First, recursively simplify sub-expressions */
    switch (expr->type)
    {
    case EXPR_BINARY:
        expr->as.binary.left = simplify_noop_expr(opt, expr->as.binary.left);
        expr->as.binary.right = simplify_noop_expr(opt, expr->as.binary.right);
        break;

    case EXPR_UNARY:
        expr->as.unary.operand = simplify_noop_expr(opt, expr->as.unary.operand);
        break;

    case EXPR_ASSIGN:
        expr->as.assign.value = simplify_noop_expr(opt, expr->as.assign.value);
        break;

    case EXPR_INDEX_ASSIGN:
        expr->as.index_assign.array = simplify_noop_expr(opt, expr->as.index_assign.array);
        expr->as.index_assign.index = simplify_noop_expr(opt, expr->as.index_assign.index);
        expr->as.index_assign.value = simplify_noop_expr(opt, expr->as.index_assign.value);
        break;

    case EXPR_CALL:
        expr->as.call.callee = simplify_noop_expr(opt, expr->as.call.callee);
        for (int i = 0; i < expr->as.call.arg_count; i++)
        {
            expr->as.call.arguments[i] = simplify_noop_expr(opt, expr->as.call.arguments[i]);
        }
        break;

    case EXPR_ARRAY:
        for (int i = 0; i < expr->as.array.element_count; i++)
        {
            expr->as.array.elements[i] = simplify_noop_expr(opt, expr->as.array.elements[i]);
        }
        break;

    case EXPR_ARRAY_ACCESS:
        expr->as.array_access.array = simplify_noop_expr(opt, expr->as.array_access.array);
        expr->as.array_access.index = simplify_noop_expr(opt, expr->as.array_access.index);
        break;

    case EXPR_ARRAY_SLICE:
        expr->as.array_slice.array = simplify_noop_expr(opt, expr->as.array_slice.array);
        if (expr->as.array_slice.start)
            expr->as.array_slice.start = simplify_noop_expr(opt, expr->as.array_slice.start);
        if (expr->as.array_slice.end)
            expr->as.array_slice.end = simplify_noop_expr(opt, expr->as.array_slice.end);
        if (expr->as.array_slice.step)
            expr->as.array_slice.step = simplify_noop_expr(opt, expr->as.array_slice.step);
        break;

    case EXPR_RANGE:
        expr->as.range.start = simplify_noop_expr(opt, expr->as.range.start);
        expr->as.range.end = simplify_noop_expr(opt, expr->as.range.end);
        break;

    case EXPR_SPREAD:
        expr->as.spread.array = simplify_noop_expr(opt, expr->as.spread.array);
        break;

    case EXPR_INCREMENT:
    case EXPR_DECREMENT:
        expr->as.operand = simplify_noop_expr(opt, expr->as.operand);
        break;

    case EXPR_INTERPOLATED:
        for (int i = 0; i < expr->as.interpol.part_count; i++)
        {
            expr->as.interpol.parts[i] = simplify_noop_expr(opt, expr->as.interpol.parts[i]);
        }
        break;

    case EXPR_MEMBER:
        expr->as.member.object = simplify_noop_expr(opt, expr->as.member.object);
        break;

    case EXPR_SIZED_ARRAY_ALLOC:
        expr->as.sized_array_alloc.size_expr = simplify_noop_expr(opt, expr->as.sized_array_alloc.size_expr);
        if (expr->as.sized_array_alloc.default_value)
        {
            expr->as.sized_array_alloc.default_value = simplify_noop_expr(opt, expr->as.sized_array_alloc.default_value);
        }
        break;

    case EXPR_STRUCT_LITERAL:
        for (int i = 0; i < expr->as.struct_literal.field_count; i++)
        {
            expr->as.struct_literal.fields[i].value = simplify_noop_expr(opt, expr->as.struct_literal.fields[i].value);
        }
        break;

    case EXPR_MEMBER_ACCESS:
        expr->as.member_access.object = simplify_noop_expr(opt, expr->as.member_access.object);
        break;

    case EXPR_MEMBER_ASSIGN:
        expr->as.member_assign.object = simplify_noop_expr(opt, expr->as.member_assign.object);
        expr->as.member_assign.value = simplify_noop_expr(opt, expr->as.member_assign.value);
        break;

    default:
        break;
    }

    /* Now check if this expression itself is a no-op */
    Expr *simplified;
    if (expr_is_noop(expr, &simplified))
    {
        opt->noops_removed++;
        return simplified;
    }

    return expr;
}

void simplify_noop_stmt(Optimizer *opt, Stmt *stmt)
{
    if (stmt == NULL) return;

    switch (stmt->type)
    {
    case STMT_EXPR:
        stmt->as.expression.expression = simplify_noop_expr(opt, stmt->as.expression.expression);
        break;

    case STMT_VAR_DECL:
        if (stmt->as.var_decl.initializer)
        {
            stmt->as.var_decl.initializer = simplify_noop_expr(opt, stmt->as.var_decl.initializer);
        }
        break;

    case STMT_RETURN:
        if (stmt->as.return_stmt.value)
        {
            stmt->as.return_stmt.value = simplify_noop_expr(opt, stmt->as.return_stmt.value);
        }
        break;

    case STMT_BLOCK:
        for (int i = 0; i < stmt->as.block.count; i++)
        {
            simplify_noop_stmt(opt, stmt->as.block.statements[i]);
        }
        break;

    case STMT_IF:
        stmt->as.if_stmt.condition = simplify_noop_expr(opt, stmt->as.if_stmt.condition);
        simplify_noop_stmt(opt, stmt->as.if_stmt.then_branch);
        if (stmt->as.if_stmt.else_branch)
        {
            simplify_noop_stmt(opt, stmt->as.if_stmt.else_branch);
        }
        break;

    case STMT_WHILE:
        stmt->as.while_stmt.condition = simplify_noop_expr(opt, stmt->as.while_stmt.condition);
        simplify_noop_stmt(opt, stmt->as.while_stmt.body);
        break;

    case STMT_FOR:
        if (stmt->as.for_stmt.initializer)
        {
            simplify_noop_stmt(opt, stmt->as.for_stmt.initializer);
        }
        if (stmt->as.for_stmt.condition)
        {
            stmt->as.for_stmt.condition = simplify_noop_expr(opt, stmt->as.for_stmt.condition);
        }
        if (stmt->as.for_stmt.increment)
        {
            stmt->as.for_stmt.increment = simplify_noop_expr(opt, stmt->as.for_stmt.increment);
        }
        simplify_noop_stmt(opt, stmt->as.for_stmt.body);
        break;

    case STMT_FOR_EACH:
        stmt->as.for_each_stmt.iterable = simplify_noop_expr(opt, stmt->as.for_each_stmt.iterable);
        simplify_noop_stmt(opt, stmt->as.for_each_stmt.body);
        break;

    default:
        break;
    }
}
