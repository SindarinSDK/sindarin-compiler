#ifndef TYPE_CHECKER_UTIL_STRUCT_H
#define TYPE_CHECKER_UTIL_STRUCT_H

#include "type_checker_util.h"
#include "ast.h"

/* Check if a type is valid for a struct field */
bool is_valid_field_type(Type *type, SymbolTable *table);

/* Check if a type is C-compatible for FFI */
bool is_c_compatible_type(Type *type);

/* Detect circular dependencies in struct definitions.
 * Returns true if a cycle is found, with the chain written to chain_out.
 * The table parameter is used to resolve forward references. */
bool detect_struct_circular_dependency(Type *struct_type, SymbolTable *table,
                                        char *chain_out, int chain_size);

/* Resolve forward-referenced struct types using the symbol table.
 * Returns the resolved type (may be the same as input if no resolution needed). */
Type *resolve_struct_forward_reference(Type *type, SymbolTable *table);

/* Extract exported symbols from an imported module.
 * Allocates arrays using the table's arena. */
void get_module_symbols(Module *imported_module, SymbolTable *table,
                        Token ***symbols_out, Type ***types_out, int *count_out);

#endif /* TYPE_CHECKER_UTIL_STRUCT_H */
