/*
 * symbol_table_thread.c - Thread state and arena tracking for symbol table
 *
 * This module contains operations for:
 * - Thread state management (pending, synchronized)
 * - Arena depth tracking for escape analysis
 */

#include "../symbol_table.h"
#include "symbol_table_thread.h"
#include "symbol_table_core.h"
#include "../debug.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* ============================================================================
 * Arena Depth Management
 * ============================================================================ */

void symbol_table_enter_arena(SymbolTable *table)
{
    table->current_arena_depth++;
    DEBUG_VERBOSE("Entered arena, new depth: %d", table->current_arena_depth);
}

void symbol_table_exit_arena(SymbolTable *table)
{
    if (table->current_arena_depth > 0)
    {
        table->current_arena_depth--;
    }
    DEBUG_VERBOSE("Exited arena, new depth: %d", table->current_arena_depth);
}

int symbol_table_get_arena_depth(SymbolTable *table)
{
    return table->current_arena_depth;
}

int symbol_table_get_scope_depth(SymbolTable *table)
{
    if (table == NULL)
    {
        return 0;
    }
    return table->scope_depth;
}

/* ============================================================================
 * Private Block Depth Management
 * ============================================================================ */

void symbol_table_enter_private(SymbolTable *table)
{
    table->current_private_depth++;
    DEBUG_VERBOSE("Entered private block, new depth: %d", table->current_private_depth);
}

void symbol_table_exit_private(SymbolTable *table)
{
    if (table->current_private_depth > 0)
    {
        table->current_private_depth--;
    }
    DEBUG_VERBOSE("Exited private block, new depth: %d", table->current_private_depth);
}

int symbol_table_get_private_depth(SymbolTable *table)
{
    return table->current_private_depth;
}

/* ============================================================================
 * Thread State Management
 * ============================================================================ */

bool symbol_table_mark_pending(Symbol *symbol)
{
    if (symbol == NULL)
    {
        DEBUG_ERROR("Cannot mark NULL symbol as pending");
        return false;
    }
    /* Only transition from NORMAL to PENDING */
    if (symbol->thread_state != THREAD_STATE_NORMAL)
    {
        DEBUG_VERBOSE("Symbol already in state %d, cannot mark pending", symbol->thread_state);
        return false;
    }
    symbol->thread_state = THREAD_STATE_PENDING;
    DEBUG_VERBOSE("Marked symbol as THREAD_STATE_PENDING");
    return true;
}

bool symbol_table_mark_synchronized(Symbol *symbol)
{
    if (symbol == NULL)
    {
        DEBUG_ERROR("Cannot mark NULL symbol as synchronized");
        return false;
    }
    /* Only transition from PENDING to SYNCHRONIZED */
    if (symbol->thread_state != THREAD_STATE_PENDING)
    {
        DEBUG_VERBOSE("Symbol in state %d, cannot mark synchronized", symbol->thread_state);
        return false;
    }
    symbol->thread_state = THREAD_STATE_SYNCHRONIZED;
    DEBUG_VERBOSE("Marked symbol as THREAD_STATE_SYNCHRONIZED");
    return true;
}

bool symbol_table_is_pending(Symbol *symbol)
{
    if (symbol == NULL)
    {
        return false;
    }
    return symbol->thread_state == THREAD_STATE_PENDING;
}

bool symbol_table_is_synchronized(Symbol *symbol)
{
    if (symbol == NULL)
    {
        return false;
    }
    return symbol->thread_state == THREAD_STATE_SYNCHRONIZED;
}

/* ============================================================================
 * Token-based Thread State Queries
 * ============================================================================ */

ThreadState symbol_table_get_thread_state(SymbolTable *table, Token name)
{
    if (table == NULL)
    {
        DEBUG_ERROR("Cannot get thread state from NULL table");
        return THREAD_STATE_NORMAL;
    }
    Symbol *symbol = symbol_table_lookup_symbol(table, name);
    if (symbol == NULL)
    {
        /* Symbol not found, return default state */
        return THREAD_STATE_NORMAL;
    }
    return symbol->thread_state;
}

bool symbol_table_is_variable_pending(SymbolTable *table, Token name)
{
    if (table == NULL)
    {
        return false;
    }
    Symbol *symbol = symbol_table_lookup_symbol(table, name);
    if (symbol == NULL)
    {
        return false;
    }
    return symbol->thread_state == THREAD_STATE_PENDING;
}

bool symbol_table_sync_variable(SymbolTable *table, Token name)
{
    if (table == NULL)
    {
        DEBUG_ERROR("Cannot sync variable in NULL table");
        return false;
    }

    Symbol *symbol = symbol_table_lookup_symbol(table, name);
    if (symbol == NULL)
    {
        DEBUG_ERROR("Cannot sync variable: symbol not found");
        return false;
    }

    /* Check if already synchronized - this is not an error but returns false */
    if (symbol->thread_state == THREAD_STATE_SYNCHRONIZED)
    {
        DEBUG_VERBOSE("Variable already synchronized, no action taken");
        return false;
    }

    /* Transition from pending to synchronized using mark_synchronized */
    if (!symbol_table_mark_synchronized(symbol))
    {
        DEBUG_ERROR("Cannot sync variable: mark_synchronized failed (state=%d)",
                    symbol->thread_state);
        return false;
    }

    return true;
}
