#include "type_checker/stmt/type_checker_stmt_import.h"
#include "type_checker/stmt/type_checker_stmt_func.h"
#include "type_checker/stmt/type_checker_stmt_var.h"
#include "type_checker/util/type_checker_util.h"
#include "type_checker/expr/type_checker_expr.h"
#include "symbol_table/symbol_table_core.h"
#include "debug.h"
#include <string.h>
#include <stdio.h>

/* Forward declaration for namespace handling (in type_checker_stmt_import_ns.c) */
void type_check_import_namespaced(Stmt *stmt, SymbolTable *table);

void process_nested_imports_recursive(Stmt **stmts, int count, SymbolTable *table)
{
    for (int i = 0; i < count; i++)
    {
        Stmt *stmt = stmts[i];
        if (stmt == NULL || stmt->type != STMT_IMPORT)
            continue;

        ImportStmt *nested_import = &stmt->as.import;
        if (nested_import->namespace == NULL)
            continue;  /* Non-aliased imports don't create namespaces */

        Token nested_ns_token = *nested_import->namespace;

        /* Skip if namespace already exists */
        if (symbol_table_is_namespace(table, nested_ns_token))
            continue;

        /* Create the namespace */
        symbol_table_add_namespace(table, nested_ns_token);

        if (nested_import->imported_stmts == NULL || nested_import->imported_count == 0)
            continue;

        /* Get module symbols */
        Module nested_temp;
        nested_temp.statements = nested_import->imported_stmts;
        nested_temp.count = nested_import->imported_count;
        nested_temp.capacity = nested_import->imported_count;
        nested_temp.filename = NULL;

        Token **nested_symbols = NULL;
        Type **nested_types = NULL;
        int nested_symbol_count = 0;

        get_module_symbols(&nested_temp, table, &nested_symbols, &nested_types, &nested_symbol_count);

        /* Add functions, structs, and variables to the namespace */
        int nested_sym_idx = 0;
        for (int j = 0; j < nested_import->imported_count && nested_sym_idx < nested_symbol_count; j++)
        {
            Stmt *nested_stmt = nested_import->imported_stmts[j];
            if (nested_stmt == NULL)
                continue;

            if (nested_stmt->type == STMT_FUNCTION)
            {
                FunctionStmt *func = &nested_stmt->as.function;
                Type *func_type = nested_types[nested_sym_idx];
                Token *func_name = nested_symbols[nested_sym_idx];
                nested_sym_idx++;

                FunctionModifier modifier = func->modifier;
                FunctionModifier effective_modifier = modifier;
                symbol_table_add_function_to_namespace(table, nested_ns_token, *func_name,
                    func_type, effective_modifier, modifier);
            }
            else if (nested_stmt->type == STMT_STRUCT_DECL)
            {
                StructDeclStmt *struct_decl = &nested_stmt->as.struct_decl;
                Token struct_name = struct_decl->name;

                Type *struct_type = ast_create_struct_type(table->arena,
                    arena_strndup(table->arena, struct_name.start, struct_name.length),
                    struct_decl->fields, struct_decl->field_count,
                    struct_decl->methods, struct_decl->method_count,
                    struct_decl->is_native, struct_decl->is_packed,
                    struct_decl->pass_self_by_ref, struct_decl->c_alias);

                symbol_table_add_struct_to_namespace(table, nested_ns_token, struct_name,
                    struct_type, nested_stmt);
            }
            else if (nested_stmt->type == STMT_VAR_DECL)
            {
                VarDeclStmt *var = &nested_stmt->as.var_decl;
                symbol_table_add_symbol_to_namespace(table, nested_ns_token, var->name, var->type);
                if (var->is_static)
                {
                    Symbol *ns_sym = symbol_table_lookup_in_namespace(table, nested_ns_token, var->name);
                    if (ns_sym != NULL)
                    {
                        ns_sym->is_static = true;
                    }
                }
            }
        }

        /* Recursively process any nested imports within this import's statements */
        process_nested_imports_recursive(nested_import->imported_stmts, nested_import->imported_count, table);

        /* Track which functions we add so we only remove those */
        bool *added_functions = arena_alloc(table->arena, sizeof(bool) * nested_symbol_count);
        for (int j = 0; j < nested_symbol_count; j++)
            added_functions[j] = false;

        /* Add functions and variables to global scope temporarily for intra-module access */
        nested_sym_idx = 0;
        int added_func_idx = 0;
        for (int j = 0; j < nested_import->imported_count && nested_sym_idx < nested_symbol_count; j++)
        {
            Stmt *nested_stmt = nested_import->imported_stmts[j];
            if (nested_stmt == NULL)
                continue;

            if (nested_stmt->type == STMT_FUNCTION)
            {
                FunctionStmt *func = &nested_stmt->as.function;
                Type *func_type = nested_types[nested_sym_idx];
                Token *func_name = nested_symbols[nested_sym_idx];
                nested_sym_idx++;

                Symbol *existing = symbol_table_lookup_symbol(table, *func_name);
                if (existing == NULL)
                {
                    FunctionModifier modifier = func->modifier;
                    FunctionModifier effective_modifier = modifier;
                    if (func->is_native)
                        symbol_table_add_native_function(table, *func_name, func_type, effective_modifier, modifier);
                    else
                        symbol_table_add_function(table, *func_name, func_type, effective_modifier, modifier);
                    added_functions[added_func_idx] = true;
                }
                added_func_idx++;
            }
            else if (nested_stmt->type == STMT_VAR_DECL)
            {
                VarDeclStmt *var = &nested_stmt->as.var_decl;
                Symbol *existing_var = symbol_table_lookup_symbol(table, var->name);
                if (existing_var == NULL)
                {
                    symbol_table_add_symbol_with_kind(table, var->name, var->type, SYMBOL_GLOBAL);
                    if (var->is_static)
                    {
                        Symbol *global_sym = symbol_table_lookup_symbol_current(table, var->name);
                        if (global_sym != NULL)
                        {
                            global_sym->is_static = true;
                        }
                    }
                }
            }
        }

        /* Type-check function bodies and variable initializers */
        nested_sym_idx = 0;
        for (int j = 0; j < nested_import->imported_count && nested_sym_idx < nested_symbol_count; j++)
        {
            Stmt *nested_stmt = nested_import->imported_stmts[j];
            if (nested_stmt == NULL)
                continue;

            if (nested_stmt->type == STMT_FUNCTION)
            {
                nested_sym_idx++;
                type_check_function_body_only(nested_stmt, table);
            }
            else if (nested_stmt->type == STMT_VAR_DECL)
            {
                VarDeclStmt *var_decl = &nested_stmt->as.var_decl;
                if (var_decl->initializer != NULL)
                {
                    type_check_expr(var_decl->initializer, table);
                }
            }
        }

        /* Remove only the functions we added from global scope */
        nested_sym_idx = 0;
        added_func_idx = 0;
        for (int j = 0; j < nested_import->imported_count && nested_sym_idx < nested_symbol_count; j++)
        {
            Stmt *nested_stmt = nested_import->imported_stmts[j];
            if (nested_stmt == NULL)
                continue;

            if (nested_stmt->type == STMT_FUNCTION)
            {
                Token *func_name = nested_symbols[nested_sym_idx];
                nested_sym_idx++;
                if (added_functions[added_func_idx])
                {
                    symbol_table_remove_symbol_from_global(table, *func_name);
                }
                added_func_idx++;
            }
        }
    }
}

void type_check_import_stmt(Stmt *stmt, SymbolTable *table)
{
    ImportStmt *import = &stmt->as.import;

    char mod_str[128];
    int mod_len = import->module_name.length < 127 ? import->module_name.length : 127;
    memcpy(mod_str, import->module_name.start, mod_len);
    mod_str[mod_len] = '\0';

    if (import->namespace == NULL)
    {
        /* Non-namespaced import: symbols are added to global scope when
         * the imported function definitions are type-checked. The parser
         * merges imported statements into the main module, and collision
         * detection is handled by type_check_function when those merged
         * function statements are processed. */
        DEBUG_VERBOSE("Type checking non-namespaced import of '%s'", mod_str);
    }
    else
    {
        /* Namespaced import: delegate to namespace-specific handler */
        type_check_import_namespaced(stmt, table);
    }
}
