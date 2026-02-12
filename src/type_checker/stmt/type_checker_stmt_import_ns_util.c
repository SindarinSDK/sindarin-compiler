#include "type_checker/stmt/type_checker_stmt_import_ns_util.h"
#include "type_checker/util/type_checker_util.h"
#include "symbol_table/symbol_table_core.h"
#include "debug.h"
#include <string.h>

char *extract_canonical_module_name(const char *mod_path, int mod_len, Arena *arena)
{
    /* Find the last path separator and extract the base name */
    const char *base_name = mod_path;
    for (int i = mod_len - 1; i >= 0; i--)
    {
        if (mod_path[i] == '/' || mod_path[i] == '\\')
        {
            base_name = &mod_path[i + 1];
            break;
        }
    }

    /* Remove .sn extension if present */
    char *canonical = arena_strdup(arena, base_name);
    int can_len = strlen(canonical);
    if (can_len > 3 && strcmp(canonical + can_len - 3, ".sn") == 0)
    {
        canonical[can_len - 3] = '\0';
    }
    return canonical;
}

void process_nested_namespaces(ImportStmt *import, Token ns_token, SymbolTable *table)
{
    for (int i = 0; i < import->imported_count; i++)
    {
        Stmt *imported_stmt = import->imported_stmts[i];
        if (imported_stmt == NULL)
            continue;

        if (imported_stmt->type == STMT_IMPORT && imported_stmt->as.import.namespace != NULL)
        {
            ImportStmt *nested_import = &imported_stmt->as.import;
            Token nested_ns_token = *nested_import->namespace;

            /* Create nested namespace inside the parent namespace */
            symbol_table_add_nested_namespace(table, ns_token, nested_ns_token);

            /* Set canonical_module_name for the nested namespace */
            Symbol *nested_ns_symbol = symbol_table_lookup_in_namespace(table, ns_token, nested_ns_token);
            if (nested_ns_symbol != NULL && nested_ns_symbol->is_namespace)
            {
                char nested_mod_path[512];
                int nested_mod_len = nested_import->module_name.length < 511 ?
                                     nested_import->module_name.length : 511;
                memcpy(nested_mod_path, nested_import->module_name.start, nested_mod_len);
                nested_mod_path[nested_mod_len] = '\0';

                nested_ns_symbol->canonical_module_name =
                    extract_canonical_module_name(nested_mod_path, nested_mod_len, table->arena);
            }

            /* Add symbols to the nested namespace */
            if (nested_import->imported_stmts != NULL)
            {
                Module nested_temp;
                nested_temp.statements = nested_import->imported_stmts;
                nested_temp.count = nested_import->imported_count;
                nested_temp.capacity = nested_import->imported_count;
                nested_temp.filename = NULL;

                Token **nested_symbols = NULL;
                Type **nested_types = NULL;
                int nested_symbol_count = 0;

                get_module_symbols(&nested_temp, table, &nested_symbols, &nested_types, &nested_symbol_count);

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
                        symbol_table_add_function_to_nested_namespace(table, ns_token, nested_ns_token,
                            *func_name, func_type, effective_modifier, modifier);
                    }
                    else if (nested_stmt->type == STMT_VAR_DECL)
                    {
                        VarDeclStmt *var = &nested_stmt->as.var_decl;
                        symbol_table_add_symbol_to_nested_namespace(table, ns_token, nested_ns_token,
                            var->name, var->type, var->is_static);
                    }
                }
            }
        }
    }
}

void register_functions_in_namespace(ImportStmt *import, Token ns_token,
                                     Token **symbols, Type **types, int symbol_count,
                                     bool *added_to_global, SymbolTable *table)
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
            FunctionStmt *func = &imported_stmt->as.function;
            Type *func_type = types[sym_idx];
            Token *func_name = symbols[sym_idx];
            sym_idx++;

            FunctionModifier modifier = func->modifier;
            FunctionModifier effective_modifier = modifier;

            /* Add to namespace */
            symbol_table_add_function_to_namespace(table, ns_token, *func_name,
                                                   func_type, effective_modifier, modifier);

            if (func->c_alias != NULL)
            {
                Symbol *ns_func_sym = symbol_table_lookup_in_namespace(table, ns_token, *func_name);
                if (ns_func_sym != NULL)
                    ns_func_sym->c_alias = func->c_alias;
            }

            /* Also add to global scope temporarily */
            Symbol *existing = symbol_table_lookup_symbol(table, *func_name);
            if (existing == NULL)
            {
                if (func->is_native)
                    symbol_table_add_native_function(table, *func_name, func_type, effective_modifier, modifier);
                else
                    symbol_table_add_function(table, *func_name, func_type, effective_modifier, modifier);

                if (func->c_alias != NULL)
                {
                    Symbol *global_func_sym = symbol_table_lookup_symbol_current(table, *func_name);
                    if (global_func_sym != NULL)
                        global_func_sym->c_alias = func->c_alias;
                }
                added_to_global[added_idx] = true;
            }
            else
            {
                existing->type = ast_clone_type(table->arena, func_type);
                existing->is_function = true;
                existing->is_native = func->is_native;
                existing->func_mod = effective_modifier;
                existing->declared_func_mod = modifier;
                existing->c_alias = func->c_alias;
            }
            added_idx++;
        }
    }
}

void register_vars_and_structs_in_namespace(ImportStmt *import, Token ns_token,
                                            SymbolTable *table)
{
    for (int i = 0; i < import->imported_count; i++)
    {
        Stmt *imported_stmt = import->imported_stmts[i];
        if (imported_stmt == NULL)
            continue;

        if (imported_stmt->type == STMT_VAR_DECL)
        {
            VarDeclStmt *var = &imported_stmt->as.var_decl;

            symbol_table_add_symbol_to_namespace(table, ns_token, var->name, var->type);

            if (var->is_static)
            {
                Symbol *ns_sym = symbol_table_lookup_in_namespace(table, ns_token, var->name);
                if (ns_sym != NULL)
                    ns_sym->is_static = true;
            }

            /* Also add to global scope for module functions */
            Symbol *existing_var = symbol_table_lookup_symbol(table, var->name);
            if (existing_var == NULL)
            {
                symbol_table_add_symbol_with_kind(table, var->name, var->type, SYMBOL_GLOBAL);
                if (var->is_static)
                {
                    Symbol *global_sym = symbol_table_lookup_symbol_current(table, var->name);
                    if (global_sym != NULL)
                        global_sym->is_static = true;
                }
            }
        }
        else if (imported_stmt->type == STMT_STRUCT_DECL)
        {
            StructDeclStmt *struct_decl = &imported_stmt->as.struct_decl;
            Token struct_name = struct_decl->name;

            Type *struct_type = ast_create_struct_type(table->arena,
                arena_strndup(table->arena, struct_name.start, struct_name.length),
                struct_decl->fields, struct_decl->field_count,
                struct_decl->methods, struct_decl->method_count,
                struct_decl->is_native, struct_decl->is_packed,
                struct_decl->pass_self_by_ref, struct_decl->c_alias);

            symbol_table_add_struct_to_namespace(table, ns_token, struct_name,
                struct_type, imported_stmt);
        }
    }
}
