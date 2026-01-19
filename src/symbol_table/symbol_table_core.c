/*
 * symbol_table_core.c - Core scope and symbol management functions
 *
 * This module contains the fundamental symbol table operations:
 * - Symbol table initialization and cleanup
 * - Scope management (push, pop, function scope)
 * - Symbol addition (basic, with kind, full, functions)
 * - Symbol lookup (current scope, all scopes)
 * - Utility functions (type size, token comparison, printing)
 */

#include "../symbol_table.h"
#include "symbol_table_core.h"
#include "../debug.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* ============================================================================
 * Utility Functions
 * ============================================================================ */

int get_type_size(Type *type)
{
    if (type == NULL) return 0;

    DEBUG_VERBOSE("Entering get_type_size with type kind: %d", type->kind);
    switch (type->kind)
    {
    /* 1-byte types */
    case TYPE_BYTE:
    case TYPE_BOOL:
    case TYPE_CHAR:
        DEBUG_VERBOSE("Returning size 1 for byte/bool/char");
        return 1;

    /* 4-byte types */
    case TYPE_INT32:
    case TYPE_UINT32:
    case TYPE_FLOAT:
        DEBUG_VERBOSE("Returning size 4 for int32/uint32/float");
        return 4;

    /* 8-byte types */
    case TYPE_INT:
    case TYPE_UINT:
    case TYPE_LONG:
    case TYPE_DOUBLE:
        DEBUG_VERBOSE("Returning size 8 for int/uint/long/double");
        return 8;

    /* Pointer types - always 8 bytes on 64-bit systems */
    case TYPE_POINTER:
        DEBUG_VERBOSE("Returning size 8 for pointer");
        return 8;

    /* String is a pointer in Sindarin runtime */
    case TYPE_STRING:
        DEBUG_VERBOSE("Returning size 8 for TYPE_STRING");
        return 8;

    /* Arrays are pointers in Sindarin runtime */
    case TYPE_ARRAY:
        DEBUG_VERBOSE("Returning size 8 for array");
        return 8;

    /* Opaque types are pointers */
    case TYPE_OPAQUE:
        DEBUG_VERBOSE("Returning size 8 for opaque");
        return 8;

    /* Struct types - return computed size */
    case TYPE_STRUCT:
        DEBUG_VERBOSE("Returning computed struct size: %zu", type->as.struct_type.size);
        return (int)type->as.struct_type.size;

    /* Function types are pointers */
    case TYPE_FUNCTION:
        DEBUG_VERBOSE("Returning size 8 for function");
        return 8;

    /* Any type is a tagged union (type tag + value), typically 16 bytes */
    case TYPE_ANY:
        DEBUG_VERBOSE("Returning size 16 for any");
        return 16;

    /* Void and nil have no size */
    case TYPE_VOID:
    case TYPE_NIL:
        DEBUG_VERBOSE("Returning size 0 for void/nil");
        return 0;

    default:
        DEBUG_VERBOSE("Returning default size 8 for unknown type kind: %d", type->kind);
        return 8;
    }
}

static char *token_to_string(Token token)
{
    DEBUG_VERBOSE("Converting token to string, token start: %p, length: %d", (void *)token.start, token.length);
    static char buf[256];
    int len = token.length < 255 ? token.length : 255;
    strncpy(buf, token.start, len);
    buf[len] = '\0';
    DEBUG_VERBOSE("Token converted to string: '%s'", buf);
    return buf;
}

int symbol_table_tokens_equal(Token a, Token b)
{
    DEBUG_VERBOSE("Comparing tokens, a: %p (length: %d), b: %p (length: %d)",
                  (void *)a.start, a.length, (void *)b.start, b.length);
    if (a.length != b.length)
    {
        char a_str[256], b_str[256];
        int a_len = a.length < 255 ? a.length : 255;
        int b_len = b.length < 255 ? b.length : 255;

        strncpy(a_str, a.start, a_len);
        a_str[a_len] = '\0';
        strncpy(b_str, b.start, b_len);
        b_str[b_len] = '\0';

        DEBUG_VERBOSE("Token length mismatch: '%s'(%d) vs '%s'(%d)",
                      a_str, a.length, b_str, b.length);
        return 0;
    }

    if (a.start == b.start)
    {
        DEBUG_VERBOSE("Token address match at %p", (void *)a.start);
        return 1;
    }

    int result = memcmp(a.start, b.start, a.length);

    char a_str[256], b_str[256];
    int a_len = a.length < 255 ? a.length : 255;
    int b_len = b.length < 255 ? b.length : 255;

    strncpy(a_str, a.start, a_len);
    a_str[a_len] = '\0';
    strncpy(b_str, b.start, b_len);
    b_str[b_len] = '\0';

    if (result == 0)
    {
        DEBUG_VERBOSE("Token content match: '%s' == '%s'", a_str, b_str);
        return 1;
    }
    else
    {
        DEBUG_VERBOSE("Token content mismatch: '%s' != '%s'", a_str, b_str);
        return 0;
    }
}

/* ============================================================================
 * Debug Printing
 * ============================================================================ */

void symbol_table_print(SymbolTable *table, const char *where)
{
    DEBUG_VERBOSE("==== SYMBOL TABLE DUMP (%s) ====", where);

    if (!table || !table->current)
    {
        DEBUG_VERBOSE("  [Empty symbol table or no current scope]");
        return;
    }

    int scope_level = 0;
    Scope *scope = table->current;

    while (scope)
    {
        DEBUG_VERBOSE("  Scope Level %d:", scope_level);
        DEBUG_VERBOSE("    next_local_offset: %d, next_param_offset: %d",
                      scope->next_local_offset, scope->next_param_offset);

        Symbol *symbol = scope->symbols;
        if (!symbol)
        {
            DEBUG_VERBOSE("    [No symbols in this scope]");
        }

        while (symbol)
        {
            /* Get thread state string */
            const char *thread_state_str;
            switch (symbol->thread_state)
            {
            case THREAD_STATE_PENDING:
                thread_state_str = "pending";
                break;
            case THREAD_STATE_SYNCHRONIZED:
                thread_state_str = "synchronized";
                break;
            case THREAD_STATE_NORMAL:
            default:
                thread_state_str = "normal";
                break;
            }

            DEBUG_VERBOSE("    Symbol: '%s', Type: %s, Kind: %d, Offset: %d",
                          token_to_string(symbol->name),
                          ast_type_to_string(table->arena, symbol->type),
                          symbol->kind,
                          symbol->offset);
            DEBUG_VERBOSE("           thread_state: %s", thread_state_str);

            /* Show frozen state only when relevant (non-zero freeze_count or frozen) */
            if (symbol->frozen_state.frozen || symbol->frozen_state.freeze_count > 0)
            {
                DEBUG_VERBOSE("           frozen: %s, freeze_count: %d",
                              symbol->frozen_state.frozen ? "yes" : "no",
                              symbol->frozen_state.freeze_count);
            }

            /* Show namespace contents if this is a namespace symbol */
            if (symbol->is_namespace)
            {
                DEBUG_VERBOSE("           [NAMESPACE] contains:");
                Symbol *ns_sym = symbol->namespace_symbols;
                if (!ns_sym)
                {
                    DEBUG_VERBOSE("             (empty)");
                }
                while (ns_sym)
                {
                    DEBUG_VERBOSE("             - '%s': %s",
                                  token_to_string(ns_sym->name),
                                  ast_type_to_string(table->arena, ns_sym->type));
                    ns_sym = ns_sym->next;
                }
            }

            symbol = symbol->next;
        }

        scope = scope->enclosing;
        scope_level++;
    }

    DEBUG_VERBOSE("====================================");
}

/* ============================================================================
 * Initialization and Cleanup
 * ============================================================================ */

void symbol_table_init(Arena *arena, SymbolTable *table)
{
    DEBUG_VERBOSE("Initializing symbol table with arena: %p, table: %p", (void *)arena, (void *)table);
    if (arena == NULL)
    {
        DEBUG_ERROR("NULL arena in symbol_table_init");
        return;
    }
    if (table == NULL)
    {
        DEBUG_ERROR("NULL table in symbol_table_init");
        return;
    }
    table->arena = arena;
    table->scopes = arena_alloc(arena, sizeof(Scope *) * 8);
    if (table->scopes == NULL)
    {
        DEBUG_ERROR("Out of memory creating scopes array");
    }
    table->scopes_count = 0;
    table->scopes_capacity = 8;
    table->current = NULL;
    table->current_arena_depth = 0;
    table->scope_depth = 0;
    table->loop_depth = 0;

    DEBUG_VERBOSE("Calling symbol_table_push_scope for initial scope");
    symbol_table_push_scope(table);
    table->global_scope = table->current;
    DEBUG_VERBOSE("Symbol table initialized, global_scope: %p", (void *)table->global_scope);
}

static void free_scope(Scope *scope)
{
    DEBUG_VERBOSE("Freeing scope: %p", (void *)scope);
    if (scope == NULL)
    {
        DEBUG_VERBOSE("Scope is NULL, returning");
        return;
    }

    Symbol *symbol = scope->symbols;
    while (symbol != NULL)
    {
        DEBUG_VERBOSE("Processing symbol: %p", (void *)symbol);
        Symbol *next = symbol->next;
        symbol = next;
    }
    DEBUG_VERBOSE("Finished freeing scope: %p", (void *)scope);
}

void symbol_table_cleanup(SymbolTable *table)
{
    DEBUG_VERBOSE("Cleaning up symbol table: %p", (void *)table);
    if (table == NULL)
    {
        DEBUG_VERBOSE("Table is NULL, returning");
        return;
    }
    for (int i = 0; i < table->scopes_count; i++)
    {
        DEBUG_VERBOSE("Freeing scope %d: %p", i, (void *)table->scopes[i]);
        free_scope(table->scopes[i]);
    }
    DEBUG_VERBOSE("Finished cleaning up symbol table");
}

/* ============================================================================
 * Scope Management
 * ============================================================================ */

void symbol_table_push_scope(SymbolTable *table)
{
    DEBUG_VERBOSE("Pushing new scope for table: %p", (void *)table);
    Scope *scope = arena_alloc(table->arena, sizeof(Scope));
    if (scope == NULL)
    {
        DEBUG_ERROR("Out of memory creating scope");
        return;
    }

    scope->symbols = NULL;
    Scope *enclosing = table->current;
    scope->enclosing = enclosing;
    scope->next_local_offset = enclosing ? enclosing->next_local_offset : LOCAL_BASE_OFFSET;
    scope->next_param_offset = enclosing ? enclosing->next_param_offset : PARAM_BASE_OFFSET;
    scope->arena_depth = table->current_arena_depth;
    table->current = scope;
    table->scope_depth++;
    DEBUG_VERBOSE("New scope created: %p, enclosing: %p, local_offset: %d, param_offset: %d, arena_depth: %d, scope_depth: %d",
                  (void *)scope, (void *)enclosing, scope->next_local_offset, scope->next_param_offset, scope->arena_depth, table->scope_depth);

    if (table->scopes_count >= table->scopes_capacity)
    {
        DEBUG_VERBOSE("Expanding scopes array, current capacity: %d", table->scopes_capacity);
        int new_capacity = table->scopes_capacity * 2;
        Scope **new_scopes = arena_alloc(table->arena, sizeof(Scope *) * new_capacity);
        if (new_scopes == NULL)
        {
            DEBUG_ERROR("Out of memory expanding scopes array");
            return;
        }
        memcpy(new_scopes, table->scopes, sizeof(Scope *) * table->scopes_count);
        table->scopes = new_scopes;
        table->scopes_capacity = new_capacity;
        DEBUG_VERBOSE("Scopes array expanded to new capacity: %d", new_capacity);
    }
    table->scopes[table->scopes_count++] = scope;
    DEBUG_VERBOSE("Scope added to table, scopes_count: %d", table->scopes_count);
}

void symbol_table_begin_function_scope(SymbolTable *table)
{
    DEBUG_VERBOSE("Beginning function scope for table: %p", (void *)table);
    symbol_table_push_scope(table);
    table->current->next_local_offset = LOCAL_BASE_OFFSET;
    table->current->next_param_offset = PARAM_BASE_OFFSET;
    DEBUG_VERBOSE("Function scope set, local_offset: %d, param_offset: %d",
                  table->current->next_local_offset, table->current->next_param_offset);
}

void symbol_table_pop_scope(SymbolTable *table)
{
    DEBUG_VERBOSE("Popping scope from table: %p", (void *)table);
    if (table->current == NULL || table->current == table->global_scope)
    {
        DEBUG_VERBOSE("Current scope is NULL or global scope, returning");
        return;
    }
    Scope *to_free = table->current;
    table->current = to_free->enclosing;
    if (table->scope_depth > 0)
    {
        table->scope_depth--;
    }
    if (table->current != NULL)
    {
        table->current->next_local_offset = MAX(table->current->next_local_offset, to_free->next_local_offset);
        table->current->next_param_offset = MAX(table->current->next_param_offset, to_free->next_param_offset);
        DEBUG_VERBOSE("Updated enclosing scope offsets, local_offset: %d, param_offset: %d",
                      table->current->next_local_offset, table->current->next_param_offset);
    }
    DEBUG_VERBOSE("Scope popped, new current scope: %p, scope_depth: %d", (void *)table->current, table->scope_depth);
}

/* ============================================================================
 * Symbol Addition
 * ============================================================================ */

void symbol_table_add_symbol_with_kind(SymbolTable *table, Token name, Type *type, SymbolKind kind)
{
    char name_str[256];
    int name_len = name.length < 255 ? name.length : 255;
    strncpy(name_str, name.start, name_len);
    name_str[name_len] = '\0';
    DEBUG_VERBOSE("Adding symbol with kind: %d, name: '%s', type: %p", kind, name_str, (void *)type);

    if (table->current == NULL)
    {
        DEBUG_ERROR("No active scope when adding symbol");
        return;
    }

    Symbol *existing = symbol_table_lookup_symbol_current(table, name);
    if (existing != NULL)
    {
        DEBUG_VERBOSE("Existing symbol found: '%s', updating type", name_str);
        existing->type = ast_clone_type(table->arena, type);
        return;
    }

    Symbol *symbol = arena_alloc(table->arena, sizeof(Symbol));
    if (symbol == NULL)
    {
        DEBUG_ERROR("Out of memory when creating symbol");
        return;
    }

    symbol->name = name;
    symbol->type = ast_clone_type(table->arena, type);
    symbol->kind = kind;

    if (kind == SYMBOL_PARAM)
    {
        symbol->offset = -table->current->next_param_offset;
        int type_size = get_type_size(type);
        int aligned_size = ((type_size + OFFSET_ALIGNMENT - 1) / OFFSET_ALIGNMENT) * OFFSET_ALIGNMENT;
        table->current->next_param_offset += aligned_size;
        DEBUG_VERBOSE("Added parameter symbol: '%s', offset: %d, aligned_size: %d, new next_param_offset: %d",
                      name_str, symbol->offset, aligned_size, table->current->next_param_offset);
    }
    else if (kind == SYMBOL_LOCAL)
    {
        symbol->offset = -table->current->next_local_offset;
        int type_size = get_type_size(type);
        int aligned_size = ((type_size + OFFSET_ALIGNMENT - 1) / OFFSET_ALIGNMENT) * OFFSET_ALIGNMENT;
        table->current->next_local_offset += aligned_size;
        DEBUG_VERBOSE("Added local symbol: '%s', offset: %d, aligned_size: %d, new next_local_offset: %d",
                      name_str, symbol->offset, aligned_size, table->current->next_local_offset);
    }
    else
    {
        symbol->offset = 0;
        DEBUG_VERBOSE("Added global symbol: '%s', offset: 0", name_str);
    }

    symbol->name.start = arena_strndup(table->arena, name.start, name.length);
    if (symbol->name.start == NULL)
    {
        DEBUG_ERROR("Out of memory duplicating token string");
        return;
    }
    symbol->name.length = name.length;
    symbol->name.line = name.line;
    symbol->name.type = name.type;
    symbol->arena_depth = table->current_arena_depth;
    symbol->declaration_scope_depth = table->scope_depth;
    symbol->mem_qual = MEM_DEFAULT;
    symbol->func_mod = FUNC_DEFAULT;
    symbol->declared_func_mod = FUNC_DEFAULT;
    symbol->is_function = false;
    symbol->is_native = false;
    symbol->c_alias = NULL;
    symbol->thread_state = THREAD_STATE_NORMAL;
    symbol->frozen_state.freeze_count = 0;
    symbol->frozen_state.frozen = false;
    symbol->frozen_args = NULL;
    symbol->frozen_args_count = 0;
    symbol->is_namespace = false;
    symbol->namespace_name = NULL;
    symbol->namespace_symbols = NULL;
    DEBUG_VERBOSE("Symbol name duplicated: '%s', length: %d, line: %d, arena_depth: %d",
                  name_str, symbol->name.length, symbol->name.line, symbol->arena_depth);

    symbol->next = table->current->symbols;
    table->current->symbols = symbol;
    DEBUG_VERBOSE("Symbol added to current scope, new symbol: %p", (void *)symbol);
}

void symbol_table_add_symbol(SymbolTable *table, Token name, Type *type)
{
    char name_str[256];
    int name_len = name.length < 255 ? name.length : 255;
    strncpy(name_str, name.start, name_len);
    name_str[name_len] = '\0';
    DEBUG_VERBOSE("Adding symbol (default kind SYMBOL_LOCAL): '%s', type: %p", name_str, (void *)type);
    symbol_table_add_symbol_with_kind(table, name, type, SYMBOL_LOCAL);
}

void symbol_table_add_symbol_full(SymbolTable *table, Token name, Type *type, SymbolKind kind, MemoryQualifier mem_qual)
{
    char name_str[256];
    int name_len = name.length < 255 ? name.length : 255;
    strncpy(name_str, name.start, name_len);
    name_str[name_len] = '\0';
    DEBUG_VERBOSE("Adding symbol with full info: '%s', kind: %d, mem_qual: %d", name_str, kind, mem_qual);

    /* First add the symbol using the standard function */
    symbol_table_add_symbol_with_kind(table, name, type, kind);

    /* Then update the memory qualifier on the newly added symbol */
    Symbol *symbol = symbol_table_lookup_symbol_current(table, name);
    if (symbol != NULL)
    {
        symbol->mem_qual = mem_qual;
        DEBUG_VERBOSE("Updated symbol '%s' mem_qual to: %d", name_str, mem_qual);
    }
}

void symbol_table_add_function(SymbolTable *table, Token name, Type *type, FunctionModifier func_mod, FunctionModifier declared_func_mod)
{
    char name_str[256];
    int name_len = name.length < 255 ? name.length : 255;
    strncpy(name_str, name.start, name_len);
    name_str[name_len] = '\0';
    DEBUG_VERBOSE("Adding function symbol: '%s', func_mod: %d, declared: %d", name_str, func_mod, declared_func_mod);

    /* First add the symbol using the standard function */
    symbol_table_add_symbol_with_kind(table, name, type, SYMBOL_LOCAL);

    /* Then update the function modifier on the newly added symbol */
    Symbol *symbol = symbol_table_lookup_symbol_current(table, name);
    if (symbol != NULL)
    {
        symbol->func_mod = func_mod;
        symbol->declared_func_mod = declared_func_mod;
        symbol->is_function = true;
        DEBUG_VERBOSE("Updated function symbol '%s' func_mod to: %d, declared: %d, is_function: true", name_str, func_mod, declared_func_mod);
    }
}

void symbol_table_add_native_function(SymbolTable *table, Token name, Type *type, FunctionModifier func_mod, FunctionModifier declared_func_mod)
{
    char name_str[256];
    int name_len = name.length < 255 ? name.length : 255;
    strncpy(name_str, name.start, name_len);
    name_str[name_len] = '\0';
    DEBUG_VERBOSE("Adding native function symbol: '%s', func_mod: %d, declared: %d", name_str, func_mod, declared_func_mod);

    /* First add the symbol using the standard function */
    symbol_table_add_symbol_with_kind(table, name, type, SYMBOL_LOCAL);

    /* Then update the function modifier and native flag on the newly added symbol */
    Symbol *symbol = symbol_table_lookup_symbol_current(table, name);
    if (symbol != NULL)
    {
        symbol->func_mod = func_mod;
        symbol->declared_func_mod = declared_func_mod;
        symbol->is_function = true;
        symbol->is_native = true;
        DEBUG_VERBOSE("Updated native function symbol '%s' func_mod to: %d, declared: %d, is_function: true, is_native: true", name_str, func_mod, declared_func_mod);
    }
}

/* ============================================================================
 * Symbol Lookup
 * ============================================================================ */

Symbol *symbol_table_lookup_symbol_current(SymbolTable *table, Token name)
{
    char name_str[256];
    int name_len = name.length < 255 ? name.length : 255;
    strncpy(name_str, name.start, name_len);
    name_str[name_len] = '\0';
    DEBUG_VERBOSE("Looking up symbol in current scope: '%s'", name_str);

    if (table->current == NULL)
    {
        DEBUG_VERBOSE("Current scope is NULL, returning NULL");
        return NULL;
    }

    Symbol *symbol = table->current->symbols;
    while (symbol != NULL)
    {
        if (symbol_table_tokens_equal(symbol->name, name))
        {
            char sym_name[256];
            int sym_len = symbol->name.length < 255 ? symbol->name.length : 255;
            strncpy(sym_name, symbol->name.start, sym_len);
            sym_name[sym_len] = '\0';
            DEBUG_VERBOSE("Found symbol in current scope: '%s'", sym_name);
            return symbol;
        }
        symbol = symbol->next;
    }

    DEBUG_VERBOSE("Symbol '%s' not found in current scope", name_str);
    return NULL;
}

Symbol *symbol_table_lookup_symbol(SymbolTable *table, Token name)
{
    char name_str[256];
    int name_len = name.length < 255 ? name.length : 255;
    strncpy(name_str, name.start, name_len);
    name_str[name_len] = '\0';
    DEBUG_VERBOSE("Looking up symbol '%s' at address %p, length %d",
                  name_str, (void *)name.start, name.length);

    if (!table || !table->current)
    {
        DEBUG_VERBOSE("Null table or current scope in lookup_symbol");
        return NULL;
    }

    Scope *scope = table->current;
    int scope_level = 0;

    while (scope != NULL)
    {
        DEBUG_VERBOSE("  Checking scope level %d", scope_level);

        Symbol *symbol = scope->symbols;
        while (symbol != NULL)
        {
            char sym_name[256];
            int sym_len = symbol->name.length < 255 ? symbol->name.length : 255;
            strncpy(sym_name, symbol->name.start, sym_len);
            sym_name[sym_len] = '\0';

            DEBUG_VERBOSE("    Symbol '%s' at address %p, length %d",
                          sym_name, (void *)symbol->name.start, symbol->name.length);

            if (symbol->name.start == name.start && symbol->name.length == name.length)
            {
                DEBUG_VERBOSE("Found symbol '%s' in scope level %d (direct pointer match)",
                              sym_name, scope_level);
                return symbol;
            }

            if (symbol->name.length == name.length &&
                memcmp(symbol->name.start, name.start, name.length) == 0)
            {
                DEBUG_VERBOSE("Found symbol '%s' in scope level %d (content match)",
                              sym_name, scope_level);
                return symbol;
            }

            if (strcmp(sym_name, name_str) == 0)
            {
                DEBUG_VERBOSE("Found symbol '%s' by string comparison in scope level %d",
                              sym_name, scope_level);
                return symbol;
            }

            symbol = symbol->next;
        }

        scope = scope->enclosing;
        scope_level++;
    }

    DEBUG_VERBOSE("Symbol '%s' not found in any scope", name_str);
    return NULL;
}

int symbol_table_get_symbol_offset(SymbolTable *table, Token name)
{
    char name_str[256];
    int name_len = name.length < 255 ? name.length : 255;
    strncpy(name_str, name.start, name_len);
    name_str[name_len] = '\0';
    DEBUG_VERBOSE("Getting offset for symbol: '%s'", name_str);

    Symbol *symbol = symbol_table_lookup_symbol(table, name);
    if (symbol == NULL)
    {
        DEBUG_ERROR("Symbol not found in get_symbol_offset: '%s'", name_str);
        return -1;
    }

    DEBUG_VERBOSE("Found symbol '%s', returning offset: %d", name_str, symbol->offset);
    return symbol->offset;
}

bool symbol_table_remove_symbol_from_global(SymbolTable *table, Token name)
{
    char name_str[256];
    int name_len = name.length < 255 ? name.length : 255;
    strncpy(name_str, name.start, name_len);
    name_str[name_len] = '\0';
    DEBUG_VERBOSE("Removing symbol from global scope: '%s'", name_str);

    if (table == NULL || table->global_scope == NULL)
    {
        DEBUG_ERROR("Cannot remove symbol: NULL table or global scope");
        return false;
    }

    Symbol **prev_ptr = &table->global_scope->symbols;
    Symbol *current = table->global_scope->symbols;

    while (current != NULL)
    {
        if (current->name.length == name.length &&
            memcmp(current->name.start, name.start, name.length) == 0 &&
            !current->is_namespace)
        {
            /* Found the symbol - unlink it from the list */
            *prev_ptr = current->next;
            DEBUG_VERBOSE("Removed symbol '%s' from global scope", name_str);
            return true;
        }
        prev_ptr = &current->next;
        current = current->next;
    }

    DEBUG_VERBOSE("Symbol '%s' not found in global scope", name_str);
    return false;
}

/* ============================================================================
 * Loop Context Tracking (for break/continue validation)
 * ============================================================================ */

void symbol_table_enter_loop(SymbolTable *table)
{
    DEBUG_VERBOSE("Entering loop context, loop_depth: %d -> %d", table->loop_depth, table->loop_depth + 1);
    table->loop_depth++;
}

void symbol_table_exit_loop(SymbolTable *table)
{
    DEBUG_VERBOSE("Exiting loop context, loop_depth: %d -> %d", table->loop_depth, table->loop_depth - 1);
    if (table->loop_depth > 0)
    {
        table->loop_depth--;
    }
}

bool symbol_table_in_loop(SymbolTable *table)
{
    DEBUG_VERBOSE("Checking if in loop, loop_depth: %d", table->loop_depth);
    return table->loop_depth > 0;
}
