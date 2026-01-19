/*
 * symbol_table_thread.c - Thread state and arena tracking for symbol table
 *
 * This module contains operations for:
 * - Thread state management (pending, synchronized)
 * - Frozen state management (freeze, unfreeze)
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
 * Frozen State Management
 * ============================================================================ */

bool symbol_table_freeze_symbol(Symbol *symbol)
{
    if (symbol == NULL)
    {
        DEBUG_ERROR("Cannot freeze NULL symbol");
        return false;
    }
    symbol->frozen_state.freeze_count++;
    symbol->frozen_state.frozen = true;
    DEBUG_VERBOSE("Froze symbol, freeze_count now: %d", symbol->frozen_state.freeze_count);
    return true;
}

bool symbol_table_unfreeze_symbol(Symbol *symbol)
{
    if (symbol == NULL)
    {
        DEBUG_ERROR("Cannot unfreeze NULL symbol");
        return false;
    }
    if (symbol->frozen_state.freeze_count <= 0)
    {
        DEBUG_ERROR("Cannot unfreeze symbol with freeze_count <= 0");
        return false;
    }
    symbol->frozen_state.freeze_count--;
    if (symbol->frozen_state.freeze_count == 0)
    {
        symbol->frozen_state.frozen = false;
    }
    DEBUG_VERBOSE("Unfroze symbol, freeze_count now: %d, frozen: %d",
                  symbol->frozen_state.freeze_count, symbol->frozen_state.frozen);
    return true;
}

bool symbol_table_is_frozen(Symbol *symbol)
{
    if (symbol == NULL)
    {
        return false;
    }
    return symbol->frozen_state.frozen;
}

int symbol_table_get_freeze_count(Symbol *symbol)
{
    if (symbol == NULL)
    {
        return 0;
    }
    return symbol->frozen_state.freeze_count;
}

void symbol_table_set_frozen_args(Symbol *symbol, Symbol **frozen_args, int count)
{
    if (symbol == NULL)
    {
        DEBUG_ERROR("Cannot set frozen args on NULL symbol");
        return;
    }
    symbol->frozen_args = frozen_args;
    symbol->frozen_args_count = count;
    DEBUG_VERBOSE("Set %d frozen args on symbol", count);
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

bool symbol_table_is_variable_frozen(SymbolTable *table, Token name)
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
    return symbol->frozen_state.frozen;
}

bool symbol_table_sync_variable(SymbolTable *table, Token name,
                                Symbol **frozen_args, int frozen_count)
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

    /* Unfreeze all frozen argument symbols associated with this thread */
    if (frozen_args != NULL && frozen_count > 0)
    {
        for (int i = 0; i < frozen_count; i++)
        {
            if (frozen_args[i] != NULL)
            {
                symbol_table_unfreeze_symbol(frozen_args[i]);
            }
        }
        DEBUG_VERBOSE("Unfroze %d argument symbols", frozen_count);
    }

    return true;
}
