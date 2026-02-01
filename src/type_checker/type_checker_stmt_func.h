#ifndef TYPE_CHECKER_STMT_FUNC_H
#define TYPE_CHECKER_STMT_FUNC_H

#include "ast.h"
#include "symbol_table.h"

/* ============================================================================
 * Function Declaration Type Checking
 * ============================================================================
 * Type checking for function declarations, including parameter validation,
 * return type checking, and function body type checking.
 * ============================================================================ */

/* Add 'arena' built-in identifier to current scope.
 * Makes 'arena' available in non-native functions and methods. */
void add_arena_builtin(SymbolTable *table, Token *ref_token);

/* Type-check only the function body, without adding to global scope.
 * Used for namespaced imports where the function is registered under a namespace. */
void type_check_function_body_only(Stmt *stmt, SymbolTable *table);

/* Type check a function declaration.
 * Validates parameters, return type, adds to symbol table, and type-checks body. */
void type_check_function(Stmt *stmt, SymbolTable *table);

#endif /* TYPE_CHECKER_STMT_FUNC_H */
