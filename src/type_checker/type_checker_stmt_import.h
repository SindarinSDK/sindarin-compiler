#ifndef TYPE_CHECKER_STMT_IMPORT_H
#define TYPE_CHECKER_STMT_IMPORT_H

#include "ast.h"
#include "symbol_table.h"

/* ============================================================================
 * Import Statement Type Checking
 * ============================================================================
 * Type checking for import statements, including namespace creation,
 * symbol registration, and recursive nested import processing.
 * ============================================================================ */

/* Recursively process nested import statements.
 * Ensures that namespaces defined by imports within merged modules
 * are properly created before type-checking functions. */
void process_nested_imports_recursive(Stmt **stmts, int count, SymbolTable *table);

/* Type check an import statement.
 * For non-namespaced imports: symbols are added to global scope when
 * function definitions are type-checked.
 * For namespaced imports: creates namespace and registers all symbols. */
void type_check_import_stmt(Stmt *stmt, SymbolTable *table);

#endif /* TYPE_CHECKER_STMT_IMPORT_H */
