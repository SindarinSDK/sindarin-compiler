#include "type_checker/stmt/type_checker_stmt.h"
#include "type_checker/stmt/type_checker_stmt_var.h"
#include "type_checker/stmt/type_checker_stmt_func.h"
#include "type_checker/stmt/type_checker_stmt_control.h"
#include "type_checker/stmt/type_checker_stmt_struct.h"
#include "type_checker/stmt/type_checker_stmt_import.h"
#include "type_checker/util/type_checker_util.h"
#include "type_checker/expr/type_checker_expr.h"
#include "symbol_table/symbol_table_core.h"
#include "debug.h"
#include <stdio.h>

/* Type check a lock statement */
static void type_check_lock(Stmt *stmt, SymbolTable *table, Type *return_type)
{
    DEBUG_VERBOSE("Type checking lock statement");

    /* Type check the lock expression */
    type_check_expr(stmt->as.lock_stmt.lock_expr, table);

    /* Validate that the lock expression is a sync variable */
    if (stmt->as.lock_stmt.lock_expr->type == EXPR_VARIABLE)
    {
        Symbol *lock_sym = symbol_table_lookup_symbol(table,
                               stmt->as.lock_stmt.lock_expr->as.variable.name);
        if (lock_sym == NULL)
        {
            type_error(stmt->as.lock_stmt.lock_expr->token,
                       "Undefined variable in lock expression");
        }
        else if (lock_sym->sync_mod != SYNC_ATOMIC)
        {
            type_error(stmt->as.lock_stmt.lock_expr->token,
                       "Lock expression must be a sync variable");
        }
    }
    else
    {
        type_error(stmt->as.lock_stmt.lock_expr->token,
                   "Lock expression must be a sync variable");
    }

    /* Type check the body */
    type_check_stmt(stmt->as.lock_stmt.body, table, return_type);
}

/* Type check a type declaration statement */
static void type_check_type_decl(Stmt *stmt, SymbolTable *table)
{
    DEBUG_VERBOSE("Type checking type declaration: %.*s",
                  stmt->as.type_decl.name.length, stmt->as.type_decl.name.start);

    if (stmt->as.type_decl.type == NULL)
    {
        type_error(&stmt->as.type_decl.name, "Type declaration must have a type");
    }
    else if (stmt->as.type_decl.type->kind == TYPE_OPAQUE)
    {
        /* Valid opaque type declaration */
    }
    else if (stmt->as.type_decl.type->kind == TYPE_FUNCTION &&
             stmt->as.type_decl.type->as.function.is_native)
    {
        /* Valid native callback type declaration.
         * Validate that all parameter types and return type are C-compatible. */
        Type *func_type = stmt->as.type_decl.type;
        for (int i = 0; i < func_type->as.function.param_count; i++)
        {
            Type *param_type = func_type->as.function.param_types[i];
            if (!is_c_compatible_type(param_type))
            {
                type_error(&stmt->as.type_decl.name,
                           "Native callback parameter type must be C-compatible "
                           "(primitives, pointers, or opaque types)");
                break;
            }
        }
        if (!is_c_compatible_type(func_type->as.function.return_type))
        {
            type_error(&stmt->as.type_decl.name,
                       "Native callback return type must be C-compatible "
                       "(primitives, pointers, or opaque types)");
        }
    }
    else if (stmt->as.type_decl.type->kind == TYPE_FUNCTION)
    {
        /* Valid regular function type alias */
    }
    else
    {
        type_error(&stmt->as.type_decl.name,
                   "Type declaration must be 'opaque', 'native fn(...)', or 'fn(...)'");
    }
}

void type_check_stmt(Stmt *stmt, SymbolTable *table, Type *return_type)
{
    if (stmt == NULL)
    {
        DEBUG_VERBOSE("Statement is NULL");
        return;
    }

    DEBUG_VERBOSE("Type checking statement type: %d", stmt->type);

    switch (stmt->type)
    {
    case STMT_EXPR:
        type_check_expr(stmt->as.expression.expression, table);
        break;

    case STMT_VAR_DECL:
        type_check_var_decl(stmt, table, return_type);
        break;

    case STMT_FUNCTION:
        type_check_function(stmt, table);
        break;

    case STMT_RETURN:
        type_check_return(stmt, table, return_type);
        break;

    case STMT_BLOCK:
        type_check_block(stmt, table, return_type);
        break;

    case STMT_IF:
        type_check_if(stmt, table, return_type);
        break;

    case STMT_WHILE:
        type_check_while(stmt, table, return_type);
        break;

    case STMT_FOR:
        type_check_for(stmt, table, return_type);
        break;

    case STMT_FOR_EACH:
        type_check_for_each(stmt, table, return_type);
        break;

    case STMT_BREAK:
        DEBUG_VERBOSE("Type checking break statement");
        if (!symbol_table_in_loop(table))
        {
            type_error(stmt->token, "'break' statement must be inside a loop");
        }
        break;

    case STMT_CONTINUE:
        DEBUG_VERBOSE("Type checking continue statement");
        if (!symbol_table_in_loop(table))
        {
            type_error(stmt->token, "'continue' statement must be inside a loop");
        }
        break;

    case STMT_IMPORT:
        type_check_import_stmt(stmt, table);
        break;

    case STMT_PRAGMA:
        DEBUG_VERBOSE("Type checking pragma statement (no-op)");
        break;

    case STMT_TYPE_DECL:
        type_check_type_decl(stmt, table);
        break;

    case STMT_STRUCT_DECL:
        type_check_struct_decl(stmt, table);
        break;

    case STMT_LOCK:
        type_check_lock(stmt, table, return_type);
        break;
    }
}
