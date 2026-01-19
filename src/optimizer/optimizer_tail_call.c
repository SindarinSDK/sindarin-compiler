#include "optimizer/optimizer_tail_call.h"
#include <string.h>

/* ============================================================================
 * Tail Call Optimization
 * ============================================================================
 * Detect and mark tail-recursive calls for optimization.
 */

/* Compare two tokens for name equality */
static bool tokens_equal(Token a, Token b)
{
    if (a.length != b.length) return false;
    return strncmp(a.start, b.start, a.length) == 0;
}

/* Check if an expression is a direct tail call to the given function name.
   Returns the call expression if it's a tail call, NULL otherwise. */
static Expr *get_tail_call_expr(Expr *expr, Token func_name)
{
    if (expr == NULL) return NULL;

    /* A direct call is a tail call if it calls the function directly */
    if (expr->type == EXPR_CALL)
    {
        CallExpr *call = &expr->as.call;
        if (call->callee->type == EXPR_VARIABLE)
        {
            if (tokens_equal(call->callee->as.variable.name, func_name))
            {
                return expr;
            }
        }
    }

    return NULL;
}

/* Check if a return statement contains a tail-recursive call to the given function */
bool is_tail_recursive_return(Stmt *stmt, Token func_name)
{
    if (stmt == NULL) return false;
    if (stmt->type != STMT_RETURN) return false;

    ReturnStmt *ret = &stmt->as.return_stmt;
    if (ret->value == NULL) return false;

    return get_tail_call_expr(ret->value, func_name) != NULL;
}

/* Check if a function has any tail-recursive patterns.
   Searches all return statements for tail calls. */
static bool check_stmt_for_tail_recursion(Stmt *stmt, Token func_name)
{
    if (stmt == NULL) return false;

    switch (stmt->type)
    {
    case STMT_RETURN:
        return is_tail_recursive_return(stmt, func_name);

    case STMT_BLOCK:
        for (int i = 0; i < stmt->as.block.count; i++)
        {
            if (check_stmt_for_tail_recursion(stmt->as.block.statements[i], func_name))
            {
                return true;
            }
        }
        return false;

    case STMT_IF:
        if (check_stmt_for_tail_recursion(stmt->as.if_stmt.then_branch, func_name))
        {
            return true;
        }
        if (stmt->as.if_stmt.else_branch &&
            check_stmt_for_tail_recursion(stmt->as.if_stmt.else_branch, func_name))
        {
            return true;
        }
        return false;

    default:
        return false;
    }
}

bool function_has_tail_recursion(FunctionStmt *fn)
{
    if (fn == NULL || fn->body == NULL || fn->body_count == 0) return false;

    for (int i = 0; i < fn->body_count; i++)
    {
        if (check_stmt_for_tail_recursion(fn->body[i], fn->name))
        {
            return true;
        }
    }
    return false;
}

/* Mark tail calls in a statement, returns count of calls marked */
int mark_tail_calls_in_stmt(Stmt *stmt, Token func_name)
{
    if (stmt == NULL) return 0;

    int marked = 0;

    switch (stmt->type)
    {
    case STMT_RETURN:
    {
        Expr *call_expr = get_tail_call_expr(stmt->as.return_stmt.value, func_name);
        if (call_expr != NULL)
        {
            call_expr->as.call.is_tail_call = true;
            marked++;
        }
        break;
    }

    case STMT_BLOCK:
        for (int i = 0; i < stmt->as.block.count; i++)
        {
            marked += mark_tail_calls_in_stmt(stmt->as.block.statements[i], func_name);
        }
        break;

    case STMT_IF:
        marked += mark_tail_calls_in_stmt(stmt->as.if_stmt.then_branch, func_name);
        if (stmt->as.if_stmt.else_branch)
        {
            marked += mark_tail_calls_in_stmt(stmt->as.if_stmt.else_branch, func_name);
        }
        break;

    default:
        break;
    }

    return marked;
}

int optimizer_mark_tail_calls(Optimizer *opt, FunctionStmt *fn)
{
    if (fn == NULL || fn->body == NULL || fn->body_count == 0) return 0;

    int marked = 0;
    for (int i = 0; i < fn->body_count; i++)
    {
        marked += mark_tail_calls_in_stmt(fn->body[i], fn->name);
    }

    opt->tail_calls_optimized += marked;
    return marked;
}

void optimizer_tail_call_optimization(Optimizer *opt, Module *module)
{
    if (module == NULL || module->statements == NULL) return;

    for (int i = 0; i < module->count; i++)
    {
        Stmt *stmt = module->statements[i];
        if (stmt->type == STMT_FUNCTION)
        {
            optimizer_mark_tail_calls(opt, &stmt->as.function);
        }
    }
}
