#include "type_checker/stmt/type_checker_stmt_import_ns.h"
#include "type_checker/stmt/type_checker_stmt_import_ns_util.h"
#include "type_checker/stmt/type_checker_stmt_import.h"
#include "type_checker/stmt/type_checker_stmt_func.h"
#include "type_checker/stmt/type_checker_stmt_var.h"
#include "type_checker/util/type_checker_util.h"
#include "type_checker/expr/type_checker_expr.h"
#include "symbol_table/symbol_table_core.h"
#include "debug.h"
#include <string.h>
#include <stdio.h>

/* Helper: Type-check imported function bodies and variable initializers (PASS 2) */
static void typecheck_imported_bodies(ImportStmt *import, SymbolTable *table)
{
    for (int i = 0; i < import->imported_count; i++)
    {
        Stmt *imported_stmt = import->imported_stmts[i];
        if (imported_stmt == NULL)
            continue;

        if (imported_stmt->type == STMT_FUNCTION)
        {
            type_check_function_body_only(imported_stmt, table);
        }
        else if (imported_stmt->type == STMT_VAR_DECL)
        {
            VarDeclStmt *var_decl = &imported_stmt->as.var_decl;
            if (var_decl->initializer != NULL)
            {
                type_check_expr(var_decl->initializer, table);
            }
        }
    }
}

/* Helper: Remove temporarily added symbols from global scope (PASS 3) */
static void cleanup_temp_global_symbols(ImportStmt *import, Token **symbols,
                                        int symbol_count, bool *added_to_global,
                                        SymbolTable *table)
{
    int sym_idx = 0;
    int added_idx = 0;

    for (int i = 0; i < import->imported_count && sym_idx < symbol_count; i++)
    {
        Stmt *imported_stmt = import->imported_stmts[i];
        if (imported_stmt == NULL)
            continue;

        if (imported_stmt->type == STMT_FUNCTION)
        {
            Token *func_name = symbols[sym_idx];
            sym_idx++;
            if (added_to_global[added_idx])
            {
                symbol_table_remove_symbol_from_global(table, *func_name);
            }
            added_idx++;
        }
    }
}

void type_check_import_namespaced(Stmt *stmt, SymbolTable *table)
{
    ImportStmt *import = &stmt->as.import;
    Token ns_token = *import->namespace;

    char ns_str[128];
    int ns_len = ns_token.length < 127 ? ns_token.length : 127;
    memcpy(ns_str, ns_token.start, ns_len);
    ns_str[ns_len] = '\0';

    char mod_str[128];
    int mod_len = import->module_name.length < 127 ? import->module_name.length : 127;
    memcpy(mod_str, import->module_name.start, mod_len);
    mod_str[mod_len] = '\0';

    DEBUG_VERBOSE("Type checking namespaced import of '%s' as '%s'", mod_str, ns_str);

    /* Validate namespace name is not a reserved keyword */
    const char *reserved = is_reserved_keyword(ns_token);
    if (reserved != NULL)
    {
        char msg[256];
        snprintf(msg, sizeof(msg), "Cannot use reserved keyword '%s' as namespace name", reserved);
        type_error(&ns_token, msg);
        return;
    }

    /* Check if namespace already exists */
    if (symbol_table_is_namespace(table, ns_token))
    {
        char msg[256];
        snprintf(msg, sizeof(msg), "Namespace '%s' is already defined", ns_str);
        type_error(&ns_token, msg);
        return;
    }

    /* Check if a non-namespace symbol with this name exists */
    Symbol *existing = symbol_table_lookup_symbol(table, ns_token);
    if (existing != NULL)
    {
        char msg[256];
        snprintf(msg, sizeof(msg), "Cannot use '%s' as namespace: name already in use", ns_str);
        type_error(&ns_token, msg);
        return;
    }

    /* Create the namespace */
    symbol_table_add_namespace(table, ns_token);

    Symbol *ns_symbol = symbol_table_lookup_symbol(table, ns_token);
    if (ns_symbol != NULL && ns_symbol->is_namespace)
    {
        ns_symbol->imported_stmts = import->imported_stmts;
        ns_symbol->canonical_module_name = extract_canonical_module_name(mod_str, mod_len, table->arena);

        if (import->also_imported_directly)
        {
            ns_symbol->also_imported_directly = true;

            /* Find canonical namespace for duplicate import */
            for (Symbol *sym = table->global_scope->symbols; sym != NULL; sym = sym->next)
            {
                if (sym != ns_symbol && sym->is_namespace &&
                    !sym->also_imported_directly && sym->canonical_namespace_prefix == NULL &&
                    sym->imported_stmts == import->imported_stmts)
                {
                    ns_symbol->canonical_namespace_prefix = arena_strdup(table->arena, sym->namespace_name);
                    break;
                }
            }
        }
    }

    /* PASS 0: Process nested namespaces */
    process_nested_namespaces(import, ns_token, table);

    /* Get module symbols */
    Module temp_module;
    temp_module.statements = import->imported_stmts;
    temp_module.count = import->imported_count;
    temp_module.capacity = import->imported_count;
    temp_module.filename = NULL;

    Token **symbols = NULL;
    Type **types = NULL;
    int symbol_count = 0;

    get_module_symbols(&temp_module, table, &symbols, &types, &symbol_count);

    /* Allocate tracking array */
    bool *added_to_global = arena_alloc(table->arena, sizeof(bool) * symbol_count);
    for (int j = 0; j < symbol_count; j++)
        added_to_global[j] = false;

    /* PASS 1: Register functions */
    register_functions_in_namespace(import, ns_token, symbols, types, symbol_count,
                                    added_to_global, table);

    /* PASS 1b: Register variables and structs */
    register_vars_and_structs_in_namespace(import, ns_token, table);

    /* PASS 1.5: Process nested imports */
    process_nested_imports_recursive(import->imported_stmts, import->imported_count, table);

    /* PASS 2: Type-check bodies */
    typecheck_imported_bodies(import, table);

    /* PASS 3: Cleanup */
    cleanup_temp_global_symbols(import, symbols, symbol_count, added_to_global, table);
}
