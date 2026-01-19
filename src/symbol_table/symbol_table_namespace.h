// symbol_table_namespace.h - Namespace operations for symbol table
#ifndef SYMBOL_TABLE_NAMESPACE_H
#define SYMBOL_TABLE_NAMESPACE_H

/* Note: This header is included from symbol_table.h after all type definitions.
 * Do not include symbol_table.h here to avoid circular dependencies. */

/*
 * Namespace Symbol Storage Design
 * ================================
 * Namespaces provide scoped access to imported module symbols via the syntax:
 *   import "module.sn" as myns
 *
 * Storage Approach:
 * - Namespaces are stored as Symbol entries in the global scope with is_namespace=true
 * - The Symbol's kind is set to SYMBOL_NAMESPACE to distinguish from regular symbols
 * - Each namespace Symbol contains a namespace_symbols field: a linked list of symbols
 *   that belong to that namespace (functions, variables, types from the imported module)
 *
 * Lookup Strategy (Two-Phase):
 * 1. Find the namespace Symbol by name in global scope (checking is_namespace=true)
 * 2. Search the namespace's namespace_symbols linked list for the target symbol
 */

/* Add a new namespace to the symbol table's global scope */
void symbol_table_add_namespace(SymbolTable *table, Token name);

/* Add a symbol (variable) to an existing namespace */
void symbol_table_add_symbol_to_namespace(SymbolTable *table, Token namespace_name, Token symbol_name, Type *type);

/* Add a function to an existing namespace */
void symbol_table_add_function_to_namespace(SymbolTable *table, Token namespace_name, Token symbol_name, Type *type, FunctionModifier func_mod, FunctionModifier declared_func_mod);

/* Look up a symbol within a namespace */
Symbol *symbol_table_lookup_in_namespace(SymbolTable *table, Token namespace_name, Token symbol_name);

/* Check if a name refers to a namespace */
bool symbol_table_is_namespace(SymbolTable *table, Token name);

#endif
