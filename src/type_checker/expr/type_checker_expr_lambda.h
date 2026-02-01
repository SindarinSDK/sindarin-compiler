#ifndef TYPE_CHECKER_EXPR_LAMBDA_H
#define TYPE_CHECKER_EXPR_LAMBDA_H

#include "ast.h"
#include "symbol_table.h"

/* ============================================================================
 * Lambda Expression Type Checking
 * ============================================================================
 * Type checking for lambda expressions including parameter type inference,
 * return type validation, and body type checking.
 * Extracted from type_checker_expr.c for modularity.
 * ============================================================================ */

/* Lambda expression type checking
 * Validates lambda parameters, return type, body type, and memory qualifiers.
 * Returns function type representing the lambda's signature.
 */
Type *type_check_lambda(Expr *expr, SymbolTable *table);

#endif /* TYPE_CHECKER_EXPR_LAMBDA_H */
