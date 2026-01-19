/*
 * symbol_table.c - Symbol table implementation
 *
 * This file contains type declaration support.
 * Core scope/symbol management is in symbol_table/symbol_table_core.c
 * Namespace operations are in symbol_table/symbol_table_namespace.c
 * Thread state and arena tracking are in symbol_table/symbol_table_thread.c
 */

#include "symbol_table.h"
#include "debug.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* ============================================================================
 * Type Declaration Support
 * ============================================================================ */

void symbol_table_add_type(SymbolTable *table, Token name, Type *type)
{
    char name_str[256];
    int name_len = name.length < 255 ? name.length : 255;
    strncpy(name_str, name.start, name_len);
    name_str[name_len] = '\0';
    DEBUG_VERBOSE("Adding type alias: '%s'", name_str);

    if (table == NULL || table->global_scope == NULL)
    {
        DEBUG_ERROR("Cannot add type: NULL table or global scope");
        return;
    }

    /* Check if type already exists in global scope */
    Symbol *existing = table->global_scope->symbols;
    while (existing != NULL)
    {
        if (existing->kind == SYMBOL_TYPE &&
            existing->name.length == name.length &&
            memcmp(existing->name.start, name.start, name.length) == 0)
        {
            DEBUG_VERBOSE("Type alias '%s' already exists, updating type", name_str);
            existing->type = ast_clone_type(table->arena, type);
            return;
        }
        existing = existing->next;
    }

    /* Allocate and initialize the type symbol */
    Symbol *symbol = arena_alloc(table->arena, sizeof(Symbol));
    if (symbol == NULL)
    {
        DEBUG_ERROR("Out of memory when creating type symbol");
        return;
    }

    /* Duplicate the type name */
    char *dup_name = arena_strndup(table->arena, name.start, name.length);
    if (dup_name == NULL)
    {
        DEBUG_ERROR("Out of memory duplicating type name");
        return;
    }

    symbol->name.start = dup_name;
    symbol->name.length = name.length;
    symbol->name.line = name.line;
    symbol->name.type = name.type;
    symbol->name.filename = name.filename;
    symbol->type = ast_clone_type(table->arena, type);
    symbol->kind = SYMBOL_TYPE;
    symbol->offset = 0;
    symbol->arena_depth = 0;  /* Global scope */
    symbol->mem_qual = MEM_DEFAULT;
    symbol->func_mod = FUNC_DEFAULT;
    symbol->declared_func_mod = FUNC_DEFAULT;
    symbol->is_function = false;
    symbol->is_native = false;
    symbol->thread_state = THREAD_STATE_NORMAL;
    symbol->frozen_state.freeze_count = 0;
    symbol->frozen_state.frozen = false;
    symbol->frozen_args = NULL;
    symbol->frozen_args_count = 0;
    symbol->is_namespace = false;
    symbol->namespace_name = NULL;
    symbol->namespace_symbols = NULL;

    /* Add to global scope */
    symbol->next = table->global_scope->symbols;
    table->global_scope->symbols = symbol;

    DEBUG_VERBOSE("Type alias '%s' added to global scope", name_str);
}

Symbol *symbol_table_lookup_type(SymbolTable *table, Token name)
{
    char name_str[256];
    int name_len = name.length < 255 ? name.length : 255;
    strncpy(name_str, name.start, name_len);
    name_str[name_len] = '\0';
    DEBUG_VERBOSE("Looking up type alias: '%s'", name_str);

    if (table == NULL || table->global_scope == NULL)
    {
        DEBUG_VERBOSE("NULL table or global scope in lookup_type");
        return NULL;
    }

    /* Search for the type symbol in global scope */
    Symbol *symbol = table->global_scope->symbols;
    while (symbol != NULL)
    {
        if (symbol->kind == SYMBOL_TYPE &&
            symbol->name.length == name.length &&
            memcmp(symbol->name.start, name.start, name.length) == 0)
        {
            DEBUG_VERBOSE("Found type alias '%s'", name_str);
            return symbol;
        }
        symbol = symbol->next;
    }

    DEBUG_VERBOSE("Type alias '%s' not found", name_str);
    return NULL;
}
