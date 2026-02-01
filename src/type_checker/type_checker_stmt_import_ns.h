#ifndef TYPE_CHECKER_STMT_IMPORT_NS_H
#define TYPE_CHECKER_STMT_IMPORT_NS_H

#include "ast.h"
#include "symbol_table.h"

/* ============================================================================
 * Namespaced Import Type Checking
 * ============================================================================
 * Type checking for namespaced import statements (import X as Y).
 * Creates namespace, registers symbols, and type-checks imported bodies.
 * ============================================================================ */

/* Type check a namespaced import statement.
 * Creates namespace entry, registers all symbols under that namespace,
 * and type-checks the imported function bodies. */
void type_check_import_namespaced(Stmt *stmt, SymbolTable *table);

#endif /* TYPE_CHECKER_STMT_IMPORT_NS_H */
