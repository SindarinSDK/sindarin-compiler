#ifndef TYPE_CHECKER_STMT_CONTROL_H
#define TYPE_CHECKER_STMT_CONTROL_H

#include "ast.h"
#include "symbol_table.h"

/* ============================================================================
 * Control Flow Statement Type Checking
 * ============================================================================
 * Type checking for control flow statements: return, if, while, for, for-each.
 * ============================================================================ */

/* Type check a return statement.
 * Validates that return value type matches function return type. */
void type_check_return(Stmt *stmt, SymbolTable *table, Type *return_type);

/* Type check a block statement.
 * Creates new scope and type-checks all statements within. */
void type_check_block(Stmt *stmt, SymbolTable *table, Type *return_type);

/* Type check an if statement.
 * Validates condition is boolean and type-checks both branches. */
void type_check_if(Stmt *stmt, SymbolTable *table, Type *return_type);

/* Type check a while loop.
 * Validates condition is boolean and type-checks body with loop context. */
void type_check_while(Stmt *stmt, SymbolTable *table, Type *return_type);

/* Type check a for loop.
 * Type-checks initializer, condition (must be bool), increment, and body. */
void type_check_for(Stmt *stmt, SymbolTable *table, Type *return_type);

/* Type check a for-each loop.
 * Validates iterable is an array and creates loop variable with element type. */
void type_check_for_each(Stmt *stmt, SymbolTable *table, Type *return_type);

#endif /* TYPE_CHECKER_STMT_CONTROL_H */
