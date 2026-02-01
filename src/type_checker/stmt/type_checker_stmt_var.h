#ifndef TYPE_CHECKER_STMT_VAR_H
#define TYPE_CHECKER_STMT_VAR_H

#include "ast.h"
#include "symbol_table.h"

/* ============================================================================
 * Variable Declaration Type Checking
 * ============================================================================
 * Type checking for variable declarations, including type inference,
 * memory qualifiers, and initializer compatibility.
 * ============================================================================ */

/* Check if a token matches a reserved keyword.
 * Returns the keyword string if it matches, NULL otherwise.
 * Used for validating namespace names. */
const char *is_reserved_keyword(Token token);

/* Infer missing lambda types from a function type annotation.
 * When a lambda is assigned to a typed variable, infer parameter
 * and return types from the declared function type. */
void infer_lambda_types(Expr *lambda_expr, Type *func_type);

/* Type check a variable declaration statement.
 * Validates types, handles type inference, checks memory qualifiers,
 * and adds the symbol to the symbol table. */
void type_check_var_decl(Stmt *stmt, SymbolTable *table, Type *return_type);

#endif /* TYPE_CHECKER_STMT_VAR_H */
