// symbol_table.h
#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include "token.h"
#include "ast.h"
#include "arena.h"

#define OFFSET_ALIGNMENT 8
#define CALLEE_SAVED_SPACE 40 
#define LOCAL_BASE_OFFSET (8 + CALLEE_SAVED_SPACE)
#define PARAM_BASE_OFFSET LOCAL_BASE_OFFSET

#define MAX(a, b) ((a) > (b) ? (a) : (b))

/* Thread state tracking for variables that hold thread handles.
 * Used by the type checker to ensure thread results are properly synchronized. */
typedef enum
{
    THREAD_STATE_NORMAL,       /* Not a thread handle, or already synchronized */
    THREAD_STATE_PENDING,      /* Thread spawned but not yet synchronized */
    THREAD_STATE_SYNCHRONIZED  /* Thread has been synchronized (joined) */
} ThreadState;

/* Frozen state tracking for variables in thread contexts.
 * When a thread is spawned, captured variables are "frozen" to prevent
 * modification while the thread is running. */
typedef struct
{
    int freeze_count;  /* Number of pending threads that have captured this variable */
    bool frozen;       /* True if freeze_count > 0 */
} FrozenState;

typedef enum
{
    SYMBOL_GLOBAL,
    SYMBOL_LOCAL,
    SYMBOL_PARAM,
    SYMBOL_NAMESPACE,
    SYMBOL_TYPE       /* Type alias (opaque types) */
} SymbolKind;

typedef struct Symbol
{
    Token name;
    Type *type;
    SymbolKind kind;
    int offset;
    struct Symbol *next;
    int arena_depth;            /* Which arena depth owns this symbol */
    int declaration_scope_depth; /* Scope depth at time of declaration */
    MemoryQualifier mem_qual;   /* as val, as ref, or default */
    SyncModifier sync_mod;      /* sync for atomic operations */
    FunctionModifier func_mod;  /* For function symbols: effective modifier (shared for heap-returning) */
    FunctionModifier declared_func_mod;  /* For function symbols: original declared modifier */
    bool is_function;           /* True if this is a named function definition */
    bool is_native;             /* True if this is a native function (external C or Sindarin-implemented native) */
    const char *c_alias;        /* C function name alias (from #pragma alias), NULL if none */
    ThreadState thread_state;   /* Thread handle state for synchronization tracking */
    FrozenState frozen_state;   /* Frozen state for thread capture tracking */
    struct Symbol **frozen_args; /* Symbols frozen by this pending thread handle */
    int frozen_args_count;      /* Number of frozen symbols */
    /* Namespace support */
    bool is_namespace;          /* True if this symbol represents a namespace */
    char *namespace_name;       /* Namespace identifier (for namespaced imports) */
    struct Symbol *namespace_symbols;  /* Linked list of symbols within this namespace */
} Symbol;

typedef struct Scope
{
    Symbol *symbols;
    struct Scope *enclosing;
    int next_local_offset;
    int next_param_offset;
    int arena_depth;            /* Arena depth level for this scope */
} Scope;

typedef struct {
    Scope *current;
    Scope *global_scope;
    Arena *arena;
    Scope **scopes;
    int scopes_count;
    int scopes_capacity;
    int current_arena_depth;    /* Current arena nesting depth */
    int scope_depth;            /* Current scope nesting depth (blocks, functions) */
    int loop_depth;             /* Current loop nesting depth (for break/continue validation) */
} SymbolTable;

/* Type declaration support (opaque types) - implemented in symbol_table.c */
void symbol_table_add_type(SymbolTable *table, Token name, Type *type);
Symbol *symbol_table_lookup_type(SymbolTable *table, Token name);

/* Include sub-headers for modular function declarations */
#include "symbol_table/symbol_table_core.h"
#include "symbol_table/symbol_table_namespace.h"
#include "symbol_table/symbol_table_thread.h"

#endif