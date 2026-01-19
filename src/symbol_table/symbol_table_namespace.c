/*
 * symbol_table_namespace.c - Namespace operations for symbol table
 *
 * This module contains operations for managing namespaced symbols:
 * - Creating namespaces
 * - Adding symbols/functions to namespaces
 * - Looking up symbols within namespaces
 * - Checking if a name is a namespace
 */

#include "../symbol_table.h"
#include "symbol_table_namespace.h"
#include "../debug.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* ============================================================================
 * Namespace Operations
 * ============================================================================ */

void symbol_table_add_namespace(SymbolTable *table, Token name)
{
    char name_str[256];
    int name_len = name.length < 255 ? name.length : 255;
    strncpy(name_str, name.start, name_len);
    name_str[name_len] = '\0';
    DEBUG_VERBOSE("Adding namespace symbol: '%s'", name_str);

    if (table == NULL || table->global_scope == NULL)
    {
        DEBUG_ERROR("Cannot add namespace: NULL table or global scope");
        return;
    }

    /* Check if namespace already exists in global scope */
    Symbol *existing = table->global_scope->symbols;
    while (existing != NULL)
    {
        if (existing->name.length == name.length &&
            memcmp(existing->name.start, name.start, name.length) == 0)
        {
            DEBUG_ERROR("Namespace '%s' already exists in global scope", name_str);
            return;
        }
        existing = existing->next;
    }

    /* Allocate and initialize the namespace symbol */
    Symbol *symbol = arena_alloc(table->arena, sizeof(Symbol));
    if (symbol == NULL)
    {
        DEBUG_ERROR("Out of memory when creating namespace symbol");
        return;
    }

    /* Duplicate the namespace name */
    char *dup_name = arena_strndup(table->arena, name.start, name.length);
    if (dup_name == NULL)
    {
        DEBUG_ERROR("Out of memory duplicating namespace name");
        return;
    }

    symbol->name.start = dup_name;
    symbol->name.length = name.length;
    symbol->name.line = name.line;
    symbol->name.type = name.type;
    symbol->name.filename = name.filename;
    symbol->type = NULL;  /* Namespaces don't have a type */
    symbol->kind = SYMBOL_NAMESPACE;
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

    /* Set namespace-specific fields */
    symbol->is_namespace = true;
    symbol->namespace_name = dup_name;
    symbol->namespace_symbols = NULL;  /* Initially empty */

    /* Add to global scope */
    symbol->next = table->global_scope->symbols;
    table->global_scope->symbols = symbol;

    DEBUG_VERBOSE("Namespace '%s' added to global scope", name_str);
}

void symbol_table_add_symbol_to_namespace(SymbolTable *table, Token namespace_name, Token symbol_name, Type *type)
{
    char ns_str[256], sym_str[256];
    int ns_len = namespace_name.length < 255 ? namespace_name.length : 255;
    int sym_len = symbol_name.length < 255 ? symbol_name.length : 255;
    strncpy(ns_str, namespace_name.start, ns_len);
    ns_str[ns_len] = '\0';
    strncpy(sym_str, symbol_name.start, sym_len);
    sym_str[sym_len] = '\0';
    DEBUG_VERBOSE("Adding symbol '%s' to namespace '%s'", sym_str, ns_str);

    if (table == NULL || table->global_scope == NULL)
    {
        DEBUG_ERROR("Cannot add symbol to namespace: NULL table or global scope");
        return;
    }

    /* Find the namespace symbol in global scope */
    Symbol *ns_symbol = table->global_scope->symbols;
    while (ns_symbol != NULL)
    {
        if (ns_symbol->is_namespace &&
            ns_symbol->name.length == namespace_name.length &&
            memcmp(ns_symbol->name.start, namespace_name.start, namespace_name.length) == 0)
        {
            break;
        }
        ns_symbol = ns_symbol->next;
    }

    if (ns_symbol == NULL)
    {
        DEBUG_ERROR("Namespace '%s' not found", ns_str);
        return;
    }

    /* Check if symbol already exists in this namespace */
    Symbol *existing = ns_symbol->namespace_symbols;
    while (existing != NULL)
    {
        if (existing->name.length == symbol_name.length &&
            memcmp(existing->name.start, symbol_name.start, symbol_name.length) == 0)
        {
            DEBUG_VERBOSE("Symbol '%s' already exists in namespace '%s', updating type", sym_str, ns_str);
            existing->type = ast_clone_type(table->arena, type);
            return;
        }
        existing = existing->next;
    }

    /* Allocate and initialize the new symbol */
    Symbol *symbol = arena_alloc(table->arena, sizeof(Symbol));
    if (symbol == NULL)
    {
        DEBUG_ERROR("Out of memory when creating symbol for namespace");
        return;
    }

    /* Duplicate the symbol name */
    char *dup_name = arena_strndup(table->arena, symbol_name.start, symbol_name.length);
    if (dup_name == NULL)
    {
        DEBUG_ERROR("Out of memory duplicating symbol name");
        return;
    }

    symbol->name.start = dup_name;
    symbol->name.length = symbol_name.length;
    symbol->name.line = symbol_name.line;
    symbol->name.type = symbol_name.type;
    symbol->name.filename = symbol_name.filename;
    symbol->type = ast_clone_type(table->arena, type);
    symbol->kind = SYMBOL_GLOBAL;
    symbol->offset = 0;
    symbol->arena_depth = 0;  /* Global scope */
    symbol->mem_qual = MEM_DEFAULT;
    symbol->func_mod = FUNC_DEFAULT;
    symbol->declared_func_mod = FUNC_DEFAULT;
    symbol->is_function = false;
    symbol->is_native = (type != NULL && type->kind == TYPE_FUNCTION && type->as.function.is_native);
    symbol->thread_state = THREAD_STATE_NORMAL;
    symbol->frozen_state.freeze_count = 0;
    symbol->frozen_state.frozen = false;
    symbol->frozen_args = NULL;
    symbol->frozen_args_count = 0;
    symbol->is_namespace = false;
    symbol->namespace_name = NULL;
    symbol->namespace_symbols = NULL;

    /* Add to namespace's symbol list */
    symbol->next = ns_symbol->namespace_symbols;
    ns_symbol->namespace_symbols = symbol;

    DEBUG_VERBOSE("Symbol '%s' added to namespace '%s'", sym_str, ns_str);
}

void symbol_table_add_function_to_namespace(SymbolTable *table, Token namespace_name, Token symbol_name, Type *type, FunctionModifier func_mod, FunctionModifier declared_func_mod)
{
    char ns_str[256], sym_str[256];
    int ns_len = namespace_name.length < 255 ? namespace_name.length : 255;
    int sym_len = symbol_name.length < 255 ? symbol_name.length : 255;
    strncpy(ns_str, namespace_name.start, ns_len);
    ns_str[ns_len] = '\0';
    strncpy(sym_str, symbol_name.start, sym_len);
    sym_str[sym_len] = '\0';
    DEBUG_VERBOSE("Adding function '%s' to namespace '%s' (mod=%d)", sym_str, ns_str, func_mod);

    if (table == NULL || table->global_scope == NULL)
    {
        DEBUG_ERROR("Cannot add function to namespace: NULL table or global scope");
        return;
    }

    /* Find the namespace symbol in global scope */
    Symbol *ns_symbol = table->global_scope->symbols;
    while (ns_symbol != NULL)
    {
        if (ns_symbol->is_namespace &&
            ns_symbol->name.length == namespace_name.length &&
            memcmp(ns_symbol->name.start, namespace_name.start, namespace_name.length) == 0)
        {
            break;
        }
        ns_symbol = ns_symbol->next;
    }

    if (ns_symbol == NULL)
    {
        DEBUG_ERROR("Namespace '%s' not found", ns_str);
        return;
    }

    /* Check if symbol already exists in this namespace */
    Symbol *existing = ns_symbol->namespace_symbols;
    while (existing != NULL)
    {
        if (existing->name.length == symbol_name.length &&
            memcmp(existing->name.start, symbol_name.start, symbol_name.length) == 0)
        {
            DEBUG_VERBOSE("Function '%s' already exists in namespace '%s', updating", sym_str, ns_str);
            existing->type = ast_clone_type(table->arena, type);
            existing->func_mod = func_mod;
            existing->declared_func_mod = declared_func_mod;
            existing->is_function = true;
            existing->is_native = (type != NULL && type->kind == TYPE_FUNCTION && type->as.function.is_native);
            return;
        }
        existing = existing->next;
    }

    /* Allocate and initialize the new symbol */
    Symbol *symbol = arena_alloc(table->arena, sizeof(Symbol));
    if (symbol == NULL)
    {
        DEBUG_ERROR("Out of memory when creating function symbol for namespace");
        return;
    }

    /* Duplicate the symbol name */
    char *dup_name = arena_strndup(table->arena, symbol_name.start, symbol_name.length);
    if (dup_name == NULL)
    {
        DEBUG_ERROR("Out of memory duplicating function name");
        return;
    }

    symbol->name.start = dup_name;
    symbol->name.length = symbol_name.length;
    symbol->name.line = symbol_name.line;
    symbol->name.type = symbol_name.type;
    symbol->name.filename = symbol_name.filename;
    symbol->type = ast_clone_type(table->arena, type);
    symbol->kind = SYMBOL_GLOBAL;
    symbol->offset = 0;
    symbol->arena_depth = 0;
    symbol->mem_qual = MEM_DEFAULT;
    symbol->func_mod = func_mod;
    symbol->declared_func_mod = declared_func_mod;
    symbol->is_function = true;
    symbol->is_native = (type != NULL && type->kind == TYPE_FUNCTION && type->as.function.is_native);
    symbol->thread_state = THREAD_STATE_NORMAL;
    symbol->frozen_state.freeze_count = 0;
    symbol->frozen_state.frozen = false;
    symbol->frozen_args = NULL;
    symbol->frozen_args_count = 0;
    symbol->is_namespace = false;
    symbol->namespace_name = NULL;
    symbol->namespace_symbols = NULL;

    /* Add to namespace's symbol list */
    symbol->next = ns_symbol->namespace_symbols;
    ns_symbol->namespace_symbols = symbol;

    DEBUG_VERBOSE("Function '%s' added to namespace '%s'", sym_str, ns_str);
}

Symbol *symbol_table_lookup_in_namespace(SymbolTable *table, Token namespace_name, Token symbol_name)
{
    char ns_str[256], sym_str[256];
    int ns_len = namespace_name.length < 255 ? namespace_name.length : 255;
    int sym_len = symbol_name.length < 255 ? symbol_name.length : 255;
    strncpy(ns_str, namespace_name.start, ns_len);
    ns_str[ns_len] = '\0';
    strncpy(sym_str, symbol_name.start, sym_len);
    sym_str[sym_len] = '\0';
    DEBUG_VERBOSE("Looking up symbol '%s' in namespace '%s'", sym_str, ns_str);

    if (table == NULL || table->global_scope == NULL)
    {
        DEBUG_VERBOSE("NULL table or global scope in lookup_in_namespace");
        return NULL;
    }

    /* Find the namespace symbol in global scope */
    Symbol *ns_symbol = table->global_scope->symbols;
    while (ns_symbol != NULL)
    {
        if (ns_symbol->is_namespace &&
            ns_symbol->name.length == namespace_name.length &&
            memcmp(ns_symbol->name.start, namespace_name.start, namespace_name.length) == 0)
        {
            break;
        }
        ns_symbol = ns_symbol->next;
    }

    if (ns_symbol == NULL)
    {
        DEBUG_VERBOSE("Namespace '%s' not found", ns_str);
        return NULL;
    }

    /* Search for the symbol within the namespace */
    Symbol *symbol = ns_symbol->namespace_symbols;
    while (symbol != NULL)
    {
        if (symbol->name.length == symbol_name.length &&
            memcmp(symbol->name.start, symbol_name.start, symbol_name.length) == 0)
        {
            DEBUG_VERBOSE("Found symbol '%s' in namespace '%s'", sym_str, ns_str);
            return symbol;
        }
        symbol = symbol->next;
    }

    DEBUG_VERBOSE("Symbol '%s' not found in namespace '%s'", sym_str, ns_str);
    return NULL;
}

bool symbol_table_is_namespace(SymbolTable *table, Token name)
{
    char name_str[256];
    int name_len = name.length < 255 ? name.length : 255;
    strncpy(name_str, name.start, name_len);
    name_str[name_len] = '\0';
    DEBUG_VERBOSE("Checking if '%s' is a namespace", name_str);

    if (table == NULL || table->global_scope == NULL)
    {
        DEBUG_VERBOSE("NULL table or global scope in is_namespace");
        return false;
    }

    /* Search for the symbol in global scope */
    Symbol *symbol = table->global_scope->symbols;
    while (symbol != NULL)
    {
        if (symbol->name.length == name.length &&
            memcmp(symbol->name.start, name.start, name.length) == 0)
        {
            if (symbol->is_namespace)
            {
                DEBUG_VERBOSE("'%s' is a namespace", name_str);
                return true;
            }
            else
            {
                DEBUG_VERBOSE("'%s' exists but is not a namespace", name_str);
                return false;
            }
        }
        symbol = symbol->next;
    }

    DEBUG_VERBOSE("'%s' not found in global scope", name_str);
    return false;
}
