#ifndef TYPE_CHECKER_STMT_INTERFACE_H
#define TYPE_CHECKER_STMT_INTERFACE_H

#include "ast.h"
#include "symbol_table.h"
#include <stdbool.h>

/* ============================================================================
 * Interface Declaration Type Checking
 * ============================================================================
 * Type checking for interface declarations and structural satisfaction checks.
 * Sindarin interfaces are Go-style structural contracts: no 'implements'
 * keyword, compile-time only, no vtable.
 * ============================================================================ */

/* Type check an interface declaration.
 * Validates:
 * 1. No duplicate method names within the interface */
void type_check_interface_decl(Stmt *stmt, SymbolTable *table);

/* Check whether a struct type structurally satisfies an interface type.
 * Takes:
 *   struct_type  - must be TYPE_STRUCT
 *   iface_type   - must be TYPE_INTERFACE
 * Returns true if all required interface methods are present on the struct
 * with matching signatures.
 * On failure, sets one of:
 *   *missing_method  - name of a method not found on the struct
 *   *mismatch_reason - description of a signature mismatch
 * (Only one of the two output pointers is set per call.) */
bool struct_satisfies_interface(Type *struct_type, Type *iface_type,
                                const char **missing_method,
                                const char **mismatch_reason);

#endif /* TYPE_CHECKER_STMT_INTERFACE_H */
