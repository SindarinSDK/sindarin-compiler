#ifndef TYPE_CHECKER_STMT_IMPORT_NS_UTIL_H
#define TYPE_CHECKER_STMT_IMPORT_NS_UTIL_H

#include "ast.h"
#include "symbol_table.h"

/* ============================================================================
 * Namespaced Import Type Checking Utilities
 * ============================================================================
 * Helper functions for namespace symbol registration.
 * ============================================================================ */

/* Extract canonical module name from a module path.
 * E.g., "./foo/bar.sn" -> "bar" */
char *extract_canonical_module_name(const char *mod_path, int mod_len, Arena *arena);

/* Process nested namespace imports (PASS 0).
 * Creates nested namespaces for imports within the imported module. */
void process_nested_namespaces(ImportStmt *import, Token ns_token, SymbolTable *table);

/* Register functions in namespace and temporarily in global scope (PASS 1).
 * Tracks which symbols were added so they can be cleaned up later. */
void register_functions_in_namespace(ImportStmt *import, Token ns_token,
                                     Token **symbols, Type **types, int symbol_count,
                                     bool *added_to_global, SymbolTable *table);

/* Register variables and structs in namespace (PASS 1b). */
void register_vars_and_structs_in_namespace(ImportStmt *import, Token ns_token,
                                            SymbolTable *table);

#endif /* TYPE_CHECKER_STMT_IMPORT_NS_UTIL_H */
