// symbol_table_thread.h - Thread state and arena tracking for symbol table
#ifndef SYMBOL_TABLE_THREAD_H
#define SYMBOL_TABLE_THREAD_H

/* Note: This header is included from symbol_table.h after all type definitions.
 * Do not include symbol_table.h here to avoid circular dependencies. */

/*
 * Thread State Management
 * =======================
 * These functions manage the thread state of symbols for async/thread operations.
 *
 * State transitions:
 *   THREAD_STATE_NORMAL -> THREAD_STATE_PENDING (via mark_pending)
 *   THREAD_STATE_PENDING -> THREAD_STATE_SYNCHRONIZED (via mark_synchronized)
 *
 * The frozen state is separate from thread state and tracks whether a symbol's
 * value is "frozen" (captured) for use in a thread context.
 */

/* Thread state transitions (operate on Symbol directly) */
bool symbol_table_mark_pending(Symbol *symbol);
bool symbol_table_mark_synchronized(Symbol *symbol);
bool symbol_table_is_pending(Symbol *symbol);
bool symbol_table_is_synchronized(Symbol *symbol);

/* Frozen state management (operate on Symbol directly) */
bool symbol_table_freeze_symbol(Symbol *symbol);
bool symbol_table_unfreeze_symbol(Symbol *symbol);
bool symbol_table_is_frozen(Symbol *symbol);
int symbol_table_get_freeze_count(Symbol *symbol);
void symbol_table_set_frozen_args(Symbol *symbol, Symbol **frozen_args, int count);

/* Token-based thread state queries (lookup symbol first, then query state) */
ThreadState symbol_table_get_thread_state(SymbolTable *table, Token name);
bool symbol_table_is_variable_pending(SymbolTable *table, Token name);
bool symbol_table_is_variable_frozen(SymbolTable *table, Token name);
bool symbol_table_sync_variable(SymbolTable *table, Token name, Symbol **frozen_args, int frozen_count);

/* Arena depth tracking for escape analysis */
void symbol_table_enter_arena(SymbolTable *table);
void symbol_table_exit_arena(SymbolTable *table);
int symbol_table_get_arena_depth(SymbolTable *table);

/* Scope depth tracking for type checking context.
 * Unlike arena_depth (which tracks private/arena blocks for memory management),
 * scope_depth tracks general block/function nesting depth.
 * This is automatically managed by push_scope/pop_scope but can also be
 * queried directly for type checking decisions (e.g., struct allocation). */
int symbol_table_get_scope_depth(SymbolTable *table);

#endif
