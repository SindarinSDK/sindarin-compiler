// symbol_table_core.h - Core scope and symbol management functions
#ifndef SYMBOL_TABLE_CORE_H
#define SYMBOL_TABLE_CORE_H

/* Note: This header is included from symbol_table.h after all type definitions.
 * Do not include symbol_table.h here to avoid circular dependencies. */

/* Type size calculation */
int get_type_size(Type *type);

/* Token comparison utility (internal to symbol_table module) */
int symbol_table_tokens_equal(Token a, Token b);

/* Debug printing */
void symbol_table_print(SymbolTable *table, const char *where);

/* Initialization and cleanup */
void symbol_table_init(Arena *arena, SymbolTable *table);
void symbol_table_cleanup(SymbolTable *table);

/* Scope management */
void symbol_table_push_scope(SymbolTable *table);
void symbol_table_pop_scope(SymbolTable *table);
void symbol_table_begin_function_scope(SymbolTable *table);

/* Symbol addition */
void symbol_table_add_symbol(SymbolTable *table, Token name, Type *type);
void symbol_table_add_symbol_with_kind(SymbolTable *table, Token name, Type *type, SymbolKind kind);
void symbol_table_add_symbol_full(SymbolTable *table, Token name, Type *type, SymbolKind kind, MemoryQualifier mem_qual);
void symbol_table_add_function(SymbolTable *table, Token name, Type *type, FunctionModifier func_mod, FunctionModifier declared_func_mod);
void symbol_table_add_native_function(SymbolTable *table, Token name, Type *type, FunctionModifier func_mod, FunctionModifier declared_func_mod);

/* Symbol lookup */
Symbol *symbol_table_lookup_symbol(SymbolTable *table, Token name);
Symbol *symbol_table_lookup_symbol_current(SymbolTable *table, Token name);
int symbol_table_get_symbol_offset(SymbolTable *table, Token name);
bool symbol_table_remove_symbol_from_global(SymbolTable *table, Token name);

/* Loop context tracking (for break/continue validation) */
void symbol_table_enter_loop(SymbolTable *table);
void symbol_table_exit_loop(SymbolTable *table);
bool symbol_table_in_loop(SymbolTable *table);

#endif
