#include "type_checker/stmt/type_checker_stmt_control.h"
#include "type_checker/stmt/type_checker_stmt.h"
#include "type_checker/util/type_checker_util.h"
#include "type_checker/expr/type_checker_expr.h"
#include "symbol_table/symbol_table_core.h"
#include "debug.h"
#include <stdio.h>
#include <string.h>

void type_check_return(Stmt *stmt, SymbolTable *table, Type *return_type)
{
    DEBUG_VERBOSE("Type checking return statement");
    Type *value_type;
    if (stmt->as.return_stmt.value)
    {
        value_type = type_check_expr(stmt->as.return_stmt.value, table);
        if (value_type == NULL)
            return;

        /* Escape analysis: detect when returning a local variable.
         * Local variables (declared inside function, scope_depth >= 2) escape when returned.
         * Parameters (SYMBOL_PARAM) and globals (scope_depth == 1) don't escape. */
        Expr *return_expr = stmt->as.return_stmt.value;
        if (return_expr->type == EXPR_VARIABLE)
        {
            Symbol *sym = symbol_table_lookup_symbol(table, return_expr->as.variable.name);
            if (sym != NULL && sym->kind != SYMBOL_PARAM && sym->declaration_scope_depth >= 2)
            {
                /* Local variable is escaping via return - mark it */
                ast_expr_mark_escapes(return_expr);
                DEBUG_VERBOSE("Escape detected: local variable '%.*s' (scope_depth %d) returned from function",
                              return_expr->as.variable.name.length,
                              return_expr->as.variable.name.start,
                              sym->declaration_scope_depth);
            }
        }
    }
    else
    {
        value_type = ast_create_primitive_type(table->arena, TYPE_VOID);
    }

    if (!ast_type_equals(value_type, return_type))
    {
        /* Special case: In method context, allow returning 'self' (pointer to struct)
         * when the declared return type is the struct itself. */
        bool is_self_return_as_struct = false;
        if (method_context_is_active() &&
            value_type != NULL && value_type->kind == TYPE_POINTER &&
            value_type->as.pointer.base_type != NULL &&
            value_type->as.pointer.base_type->kind == TYPE_STRUCT &&
            return_type != NULL && return_type->kind == TYPE_STRUCT)
        {
            if (ast_type_equals(value_type->as.pointer.base_type, return_type))
            {
                is_self_return_as_struct = true;
                if (stmt->as.return_stmt.value != NULL)
                {
                    stmt->as.return_stmt.value->expr_type = return_type;
                }
                DEBUG_VERBOSE("Allowing implicit dereference of self pointer for struct return");
            }
        }

        if (!is_self_return_as_struct)
        {
            type_error(stmt->token, "Return type does not match function return type");
        }
    }
}

void type_check_block(Stmt *stmt, SymbolTable *table, Type *return_type)
{
    DEBUG_VERBOSE("Type checking block with %d statements", stmt->as.block.count);

    symbol_table_push_scope(table);
    for (int i = 0; i < stmt->as.block.count; i++)
    {
        type_check_stmt(stmt->as.block.statements[i], table, return_type);
    }
    symbol_table_pop_scope(table);
}

void type_check_if(Stmt *stmt, SymbolTable *table, Type *return_type)
{
    DEBUG_VERBOSE("Type checking if statement");
    Type *cond_type = type_check_expr(stmt->as.if_stmt.condition, table);
    if (cond_type && cond_type->kind != TYPE_BOOL)
    {
        type_error(stmt->as.if_stmt.condition->token, "If condition must be boolean");
    }
    type_check_stmt(stmt->as.if_stmt.then_branch, table, return_type);
    if (stmt->as.if_stmt.else_branch)
    {
        DEBUG_VERBOSE("Type checking else branch");
        type_check_stmt(stmt->as.if_stmt.else_branch, table, return_type);
    }
}

void type_check_while(Stmt *stmt, SymbolTable *table, Type *return_type)
{
    DEBUG_VERBOSE("Type checking while statement");
    Type *cond_type = type_check_expr(stmt->as.while_stmt.condition, table);
    if (cond_type && cond_type->kind != TYPE_BOOL)
    {
        type_error(stmt->as.while_stmt.condition->token, "While condition must be boolean");
    }

    /* Track loop context for break/continue validation */
    symbol_table_enter_loop(table);
    type_check_stmt(stmt->as.while_stmt.body, table, return_type);
    symbol_table_exit_loop(table);
}

void type_check_for(Stmt *stmt, SymbolTable *table, Type *return_type)
{
    DEBUG_VERBOSE("Type checking for statement");
    symbol_table_push_scope(table);
    if (stmt->as.for_stmt.initializer)
    {
        type_check_stmt(stmt->as.for_stmt.initializer, table, return_type);
    }
    if (stmt->as.for_stmt.condition)
    {
        Type *cond_type = type_check_expr(stmt->as.for_stmt.condition, table);
        if (cond_type && cond_type->kind != TYPE_BOOL)
        {
            type_error(stmt->as.for_stmt.condition->token, "For condition must be boolean");
        }
    }
    if (stmt->as.for_stmt.increment)
    {
        type_check_expr(stmt->as.for_stmt.increment, table);
    }

    /* Track loop context for break/continue validation */
    symbol_table_enter_loop(table);
    type_check_stmt(stmt->as.for_stmt.body, table, return_type);
    symbol_table_exit_loop(table);

    symbol_table_pop_scope(table);
}

void type_check_for_each(Stmt *stmt, SymbolTable *table, Type *return_type)
{
    DEBUG_VERBOSE("Type checking for-each statement");

    /* Type check the iterable expression */
    Type *iterable_type = type_check_expr(stmt->as.for_each_stmt.iterable, table);
    if (iterable_type == NULL)
    {
        return;
    }

    Type *element_type = NULL;

    if (iterable_type->kind == TYPE_ARRAY)
    {
        /* Array iteration — existing path */
        element_type = iterable_type->as.array.element_type;
    }
    else if (iterable_type->kind == TYPE_STRUCT)
    {
        /* Iterator protocol: check for iter() -> IterType with hasNext(): bool, next(): T */
        char msg[512];

        /* 1. Look for iter() method on the iterable struct */
        StructMethod *iter_method = ast_struct_get_method(iterable_type, "iter");
        if (iter_method == NULL)
        {
            snprintf(msg, sizeof(msg),
                     "Type '%s' is not iterable: missing method 'iter()'",
                     iterable_type->as.struct_type.name);
            type_error(stmt->as.for_each_stmt.iterable->token, msg);
            return;
        }
        if (iter_method->param_count != 0)
        {
            snprintf(msg, sizeof(msg),
                     "Method 'iter()' on '%s' must take no parameters",
                     iterable_type->as.struct_type.name);
            type_error(stmt->as.for_each_stmt.iterable->token, msg);
            return;
        }

        Type *iter_type = iter_method->return_type;
        if (iter_type == NULL || iter_type->kind != TYPE_STRUCT)
        {
            snprintf(msg, sizeof(msg),
                     "Method 'iter()' on '%s' must return a struct type",
                     iterable_type->as.struct_type.name);
            type_error(stmt->as.for_each_stmt.iterable->token, msg);
            return;
        }

        /* 2. Check for hasNext(): bool on the iterator struct */
        StructMethod *has_next = ast_struct_get_method(iter_type, "hasNext");
        if (has_next == NULL)
        {
            snprintf(msg, sizeof(msg),
                     "Iterator type '%s' is missing method 'hasNext()'",
                     iter_type->as.struct_type.name);
            type_error(stmt->as.for_each_stmt.iterable->token, msg);
            return;
        }
        if (has_next->param_count != 0)
        {
            snprintf(msg, sizeof(msg),
                     "Method 'hasNext()' on '%s' must take no parameters",
                     iter_type->as.struct_type.name);
            type_error(stmt->as.for_each_stmt.iterable->token, msg);
            return;
        }
        if (has_next->return_type == NULL || has_next->return_type->kind != TYPE_BOOL)
        {
            snprintf(msg, sizeof(msg),
                     "Method 'hasNext()' on '%s' must return bool",
                     iter_type->as.struct_type.name);
            type_error(stmt->as.for_each_stmt.iterable->token, msg);
            return;
        }

        /* 3. Check for next(): T on the iterator struct */
        StructMethod *next_method = ast_struct_get_method(iter_type, "next");
        if (next_method == NULL)
        {
            snprintf(msg, sizeof(msg),
                     "Iterator type '%s' is missing method 'next()'",
                     iter_type->as.struct_type.name);
            type_error(stmt->as.for_each_stmt.iterable->token, msg);
            return;
        }
        if (next_method->param_count != 0)
        {
            snprintf(msg, sizeof(msg),
                     "Method 'next()' on '%s' must take no parameters",
                     iter_type->as.struct_type.name);
            type_error(stmt->as.for_each_stmt.iterable->token, msg);
            return;
        }
        if (next_method->return_type == NULL || next_method->return_type->kind == TYPE_VOID)
        {
            snprintf(msg, sizeof(msg),
                     "Method 'next()' on '%s' must return a value",
                     iter_type->as.struct_type.name);
            type_error(stmt->as.for_each_stmt.iterable->token, msg);
            return;
        }

        element_type = next_method->return_type;

        /* Store iterator info on the AST for code generation */
        stmt->as.for_each_stmt.iterator_type = iter_type;
        stmt->as.for_each_stmt.element_type = element_type;
    }
    else
    {
        type_error(stmt->as.for_each_stmt.iterable->token,
                   "For-each requires an array or iterable struct type");
        return;
    }

    /* Create a new scope and add the loop variable.
     * Use SYMBOL_PARAM so it's not freed - loop var is a reference to element. */
    symbol_table_push_scope(table);
    symbol_table_add_symbol_with_kind(table, stmt->as.for_each_stmt.var_name, element_type, SYMBOL_PARAM);

    /* Track loop context for break/continue validation */
    symbol_table_enter_loop(table);
    type_check_stmt(stmt->as.for_each_stmt.body, table, return_type);
    symbol_table_exit_loop(table);

    symbol_table_pop_scope(table);
}
