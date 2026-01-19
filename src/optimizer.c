#include "optimizer.h"
#include "optimizer/optimizer_util.h"
#include "optimizer/optimizer_tail_call.h"
#include "optimizer/optimizer_string.h"
#include "debug.h"
#include <stdlib.h>

void optimizer_init(Optimizer *opt, Arena *arena)
{
    opt->arena = arena;
    opt->statements_removed = 0;
    opt->variables_removed = 0;
    opt->noops_removed = 0;
    opt->tail_calls_optimized = 0;
    opt->string_literals_merged = 0;
}

void optimizer_get_stats(Optimizer *opt, int *stmts_removed, int *vars_removed, int *noops_removed)
{
    if (stmts_removed) *stmts_removed = opt->statements_removed;
    if (vars_removed) *vars_removed = opt->variables_removed;
    if (noops_removed) *noops_removed = opt->noops_removed;
}

/* ============================================================================
 * Terminator Detection
 * ============================================================================
 * Detect statements that always terminate control flow: return, break, continue
 */

bool stmt_is_terminator(Stmt *stmt)
{
    if (stmt == NULL) return false;

    switch (stmt->type)
    {
    case STMT_RETURN:
    case STMT_BREAK:
    case STMT_CONTINUE:
        return true;

    case STMT_BLOCK:
        /* A block is a terminator if any of its statements is a terminator
           that is not inside a conditional */
        for (int i = 0; i < stmt->as.block.count; i++)
        {
            if (stmt_is_terminator(stmt->as.block.statements[i]))
            {
                return true;
            }
        }
        return false;

    case STMT_IF:
        /* An if is a terminator only if BOTH branches exist and BOTH terminate */
        if (stmt->as.if_stmt.else_branch == NULL)
        {
            return false;
        }
        return stmt_is_terminator(stmt->as.if_stmt.then_branch) &&
               stmt_is_terminator(stmt->as.if_stmt.else_branch);

    default:
        return false;
    }
}

/* ============================================================================
 * Dead Code Removal
 * ============================================================================
 */

/* Remove unreachable statements after a terminator in a block.
   Returns the number of statements removed. */
int remove_unreachable_statements(Optimizer *opt, Stmt ***stmts, int *count)
{
    if (stmts == NULL || *stmts == NULL || *count <= 0) return 0;

    int removed = 0;
    int new_count = 0;

    for (int i = 0; i < *count; i++)
    {
        Stmt *stmt = (*stmts)[i];

        /* If we found a terminator, all remaining statements are unreachable */
        if (new_count > 0 && stmt_is_terminator((*stmts)[new_count - 1]))
        {
            /* This statement is unreachable */
            removed++;
            continue;
        }

        /* Keep this statement */
        (*stmts)[new_count] = stmt;
        new_count++;

        /* Recursively process nested blocks */
        switch (stmt->type)
        {
        case STMT_BLOCK:
            removed += remove_unreachable_statements(opt, &stmt->as.block.statements, &stmt->as.block.count);
            break;

        case STMT_IF:
            if (stmt->as.if_stmt.then_branch && stmt->as.if_stmt.then_branch->type == STMT_BLOCK)
            {
                removed += remove_unreachable_statements(opt,
                    &stmt->as.if_stmt.then_branch->as.block.statements,
                    &stmt->as.if_stmt.then_branch->as.block.count);
            }
            if (stmt->as.if_stmt.else_branch && stmt->as.if_stmt.else_branch->type == STMT_BLOCK)
            {
                removed += remove_unreachable_statements(opt,
                    &stmt->as.if_stmt.else_branch->as.block.statements,
                    &stmt->as.if_stmt.else_branch->as.block.count);
            }
            break;

        case STMT_WHILE:
            if (stmt->as.while_stmt.body && stmt->as.while_stmt.body->type == STMT_BLOCK)
            {
                removed += remove_unreachable_statements(opt,
                    &stmt->as.while_stmt.body->as.block.statements,
                    &stmt->as.while_stmt.body->as.block.count);
            }
            break;

        case STMT_FOR:
            if (stmt->as.for_stmt.body && stmt->as.for_stmt.body->type == STMT_BLOCK)
            {
                removed += remove_unreachable_statements(opt,
                    &stmt->as.for_stmt.body->as.block.statements,
                    &stmt->as.for_stmt.body->as.block.count);
            }
            break;

        case STMT_FOR_EACH:
            if (stmt->as.for_each_stmt.body && stmt->as.for_each_stmt.body->type == STMT_BLOCK)
            {
                removed += remove_unreachable_statements(opt,
                    &stmt->as.for_each_stmt.body->as.block.statements,
                    &stmt->as.for_each_stmt.body->as.block.count);
            }
            break;

        default:
            break;
        }
    }

    *count = new_count;
    opt->statements_removed += removed;
    return removed;
}

/* Run dead code elimination on a function */
void optimizer_eliminate_dead_code_function(Optimizer *opt, FunctionStmt *fn)
{
    if (fn == NULL || fn->body == NULL || fn->body_count == 0) return;

    /* 1. Remove unreachable statements after return/break/continue */
    remove_unreachable_statements(opt, &fn->body, &fn->body_count);

    /* 2. Simplify no-op expressions */
    for (int i = 0; i < fn->body_count; i++)
    {
        simplify_noop_stmt(opt, fn->body[i]);
    }

    /* 3. Remove unused variable declarations (do this last since
       simplification might affect variable usage) */
    remove_unused_variables(opt, fn->body, &fn->body_count);
}

/* Run dead code elimination on an entire module */
void optimizer_dead_code_elimination(Optimizer *opt, Module *module)
{
    if (module == NULL || module->statements == NULL) return;

    for (int i = 0; i < module->count; i++)
    {
        Stmt *stmt = module->statements[i];
        if (stmt->type == STMT_FUNCTION)
        {
            optimizer_eliminate_dead_code_function(opt, &stmt->as.function);
        }
    }
}
