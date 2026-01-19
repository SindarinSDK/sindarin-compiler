#include "optimizer/optimizer_string.h"
#include "arena.h"
#include <string.h>

/* ============================================================================
 * String Interpolation Optimization
 * ============================================================================
 * Merge adjacent string literals in interpolated expressions to reduce
 * runtime concatenations and temporary variables.
 */

/* Check if an expression is a string literal */
static bool is_string_literal(Expr *expr)
{
    if (expr == NULL) return false;
    if (expr->type != EXPR_LITERAL) return false;
    return expr->as.literal.type && expr->as.literal.type->kind == TYPE_STRING;
}

/* Get the string value from a string literal expression */
static const char *get_string_literal_value(Expr *expr)
{
    if (!is_string_literal(expr)) return NULL;
    return expr->as.literal.value.string_value;
}

/* Create a new string literal expression with the given value */
static Expr *create_string_literal(Arena *arena, const char *value)
{
    Expr *expr = arena_alloc(arena, sizeof(Expr));
    expr->type = EXPR_LITERAL;
    expr->as.literal.type = ast_create_primitive_type(arena, TYPE_STRING);
    expr->as.literal.value.string_value = arena_strdup(arena, value);
    expr->expr_type = expr->as.literal.type;
    return expr;
}

/* Merge adjacent string literals in an interpolated expression.
   Returns true if any merging was done. */
static bool merge_interpolated_parts(Optimizer *opt, InterpolExpr *interpol)
{
    if (interpol == NULL || interpol->part_count < 2) return false;

    int new_count = 0;
    Expr **new_parts = arena_alloc(opt->arena, interpol->part_count * sizeof(Expr *));
    bool any_merged = false;

    int i = 0;
    while (i < interpol->part_count)
    {
        if (is_string_literal(interpol->parts[i]))
        {
            /* Start merging string literals */
            const char *merged = get_string_literal_value(interpol->parts[i]);
            int merge_start = i;
            i++;

            /* Keep merging adjacent string literals */
            while (i < interpol->part_count && is_string_literal(interpol->parts[i]))
            {
                const char *next = get_string_literal_value(interpol->parts[i]);
                size_t merged_len = strlen(merged);
                size_t next_len = strlen(next);
                char *new_merged = arena_alloc(opt->arena, merged_len + next_len + 1);
                memcpy(new_merged, merged, merged_len);
                memcpy(new_merged + merged_len, next, next_len);
                new_merged[merged_len + next_len] = '\0';
                merged = new_merged;
                i++;
            }

            if (i - merge_start > 1)
            {
                /* We merged multiple literals */
                opt->string_literals_merged += (i - merge_start - 1);
                any_merged = true;
            }

            /* Create a new literal with the merged string */
            new_parts[new_count++] = create_string_literal(opt->arena, merged);
        }
        else
        {
            /* Non-string-literal, just copy it */
            new_parts[new_count++] = interpol->parts[i];
            i++;
        }
    }

    if (any_merged)
    {
        interpol->parts = new_parts;
        interpol->part_count = new_count;
    }

    return any_merged;
}

/* Recursively optimize string expressions */
Expr *optimize_string_expr(Optimizer *opt, Expr *expr)
{
    if (expr == NULL) return NULL;

    switch (expr->type)
    {
    case EXPR_INTERPOLATED:
        /* Merge adjacent string literals */
        merge_interpolated_parts(opt, &expr->as.interpol);

        /* Recursively optimize parts (they may contain nested interpolations) */
        for (int i = 0; i < expr->as.interpol.part_count; i++)
        {
            expr->as.interpol.parts[i] = optimize_string_expr(opt, expr->as.interpol.parts[i]);
        }
        break;

    case EXPR_BINARY:
        expr->as.binary.left = optimize_string_expr(opt, expr->as.binary.left);
        expr->as.binary.right = optimize_string_expr(opt, expr->as.binary.right);

        /* Merge string literal concatenations: "a" + "b" -> "ab" */
        if (expr->as.binary.operator == TOKEN_PLUS &&
            is_string_literal(expr->as.binary.left) &&
            is_string_literal(expr->as.binary.right))
        {
            const char *left = get_string_literal_value(expr->as.binary.left);
            const char *right = get_string_literal_value(expr->as.binary.right);
            size_t left_len = strlen(left);
            size_t right_len = strlen(right);
            char *merged = arena_alloc(opt->arena, left_len + right_len + 1);
            memcpy(merged, left, left_len);
            memcpy(merged + left_len, right, right_len);
            merged[left_len + right_len] = '\0';

            opt->string_literals_merged++;
            return create_string_literal(opt->arena, merged);
        }
        break;

    case EXPR_CALL:
        for (int i = 0; i < expr->as.call.arg_count; i++)
        {
            expr->as.call.arguments[i] = optimize_string_expr(opt, expr->as.call.arguments[i]);
        }
        break;

    case EXPR_UNARY:
        expr->as.unary.operand = optimize_string_expr(opt, expr->as.unary.operand);
        break;

    case EXPR_ASSIGN:
        expr->as.assign.value = optimize_string_expr(opt, expr->as.assign.value);
        break;

    case EXPR_ARRAY:
        for (int i = 0; i < expr->as.array.element_count; i++)
        {
            expr->as.array.elements[i] = optimize_string_expr(opt, expr->as.array.elements[i]);
        }
        break;

    case EXPR_ARRAY_ACCESS:
        expr->as.array_access.array = optimize_string_expr(opt, expr->as.array_access.array);
        expr->as.array_access.index = optimize_string_expr(opt, expr->as.array_access.index);
        break;

    case EXPR_SIZED_ARRAY_ALLOC:
        expr->as.sized_array_alloc.size_expr = optimize_string_expr(opt, expr->as.sized_array_alloc.size_expr);
        if (expr->as.sized_array_alloc.default_value)
        {
            expr->as.sized_array_alloc.default_value = optimize_string_expr(opt, expr->as.sized_array_alloc.default_value);
        }
        break;

    default:
        break;
    }

    return expr;
}

/* Optimize string expressions in a statement */
static void optimize_string_stmt(Optimizer *opt, Stmt *stmt)
{
    if (stmt == NULL) return;

    switch (stmt->type)
    {
    case STMT_EXPR:
        stmt->as.expression.expression = optimize_string_expr(opt, stmt->as.expression.expression);
        break;

    case STMT_VAR_DECL:
        if (stmt->as.var_decl.initializer)
        {
            stmt->as.var_decl.initializer = optimize_string_expr(opt, stmt->as.var_decl.initializer);
        }
        break;

    case STMT_RETURN:
        if (stmt->as.return_stmt.value)
        {
            stmt->as.return_stmt.value = optimize_string_expr(opt, stmt->as.return_stmt.value);
        }
        break;

    case STMT_BLOCK:
        for (int i = 0; i < stmt->as.block.count; i++)
        {
            optimize_string_stmt(opt, stmt->as.block.statements[i]);
        }
        break;

    case STMT_IF:
        stmt->as.if_stmt.condition = optimize_string_expr(opt, stmt->as.if_stmt.condition);
        optimize_string_stmt(opt, stmt->as.if_stmt.then_branch);
        if (stmt->as.if_stmt.else_branch)
        {
            optimize_string_stmt(opt, stmt->as.if_stmt.else_branch);
        }
        break;

    case STMT_WHILE:
        stmt->as.while_stmt.condition = optimize_string_expr(opt, stmt->as.while_stmt.condition);
        optimize_string_stmt(opt, stmt->as.while_stmt.body);
        break;

    case STMT_FOR:
        if (stmt->as.for_stmt.initializer)
        {
            optimize_string_stmt(opt, stmt->as.for_stmt.initializer);
        }
        if (stmt->as.for_stmt.condition)
        {
            stmt->as.for_stmt.condition = optimize_string_expr(opt, stmt->as.for_stmt.condition);
        }
        if (stmt->as.for_stmt.increment)
        {
            stmt->as.for_stmt.increment = optimize_string_expr(opt, stmt->as.for_stmt.increment);
        }
        optimize_string_stmt(opt, stmt->as.for_stmt.body);
        break;

    case STMT_FOR_EACH:
        stmt->as.for_each_stmt.iterable = optimize_string_expr(opt, stmt->as.for_each_stmt.iterable);
        optimize_string_stmt(opt, stmt->as.for_each_stmt.body);
        break;

    default:
        break;
    }
}

/* Optimize string expressions in a function */
static void optimize_string_function(Optimizer *opt, FunctionStmt *fn)
{
    if (fn == NULL || fn->body == NULL) return;

    for (int i = 0; i < fn->body_count; i++)
    {
        optimize_string_stmt(opt, fn->body[i]);
    }
}

int optimizer_merge_string_literals(Optimizer *opt, Module *module)
{
    if (module == NULL || module->statements == NULL) return 0;

    int initial = opt->string_literals_merged;

    for (int i = 0; i < module->count; i++)
    {
        Stmt *stmt = module->statements[i];
        if (stmt->type == STMT_FUNCTION)
        {
            optimize_string_function(opt, &stmt->as.function);
        }
    }

    return opt->string_literals_merged - initial;
}
