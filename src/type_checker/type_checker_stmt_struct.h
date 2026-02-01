#ifndef TYPE_CHECKER_STMT_STRUCT_H
#define TYPE_CHECKER_STMT_STRUCT_H

#include "ast.h"
#include "symbol_table.h"

/* ============================================================================
 * Struct Declaration Type Checking
 * ============================================================================
 * Type checking for struct declarations, including field validation,
 * method type checking, and circular dependency detection.
 * ============================================================================ */

/* Type check a struct declaration.
 * Validates:
 * 1. All field types are valid (primitives, arrays, strings, or defined struct/opaque types)
 * 2. Pointer fields are only allowed in native structs
 * 3. Default value types match field types
 * 4. Method bodies are type-checked with proper 'self' binding
 * 5. No circular dependencies exist */
void type_check_struct_decl(Stmt *stmt, SymbolTable *table);

#endif /* TYPE_CHECKER_STMT_STRUCT_H */
