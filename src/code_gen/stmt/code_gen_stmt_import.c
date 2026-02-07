/*
 * code_gen_stmt_import.c - Import statement code generation helpers
 *
 * Handles forward declarations and symbol management for imported modules.
 */

#include "code_gen/stmt/code_gen_stmt.h"
#include "code_gen/expr/code_gen_expr.h"
#include "code_gen/util/code_gen_util.h"
#include "debug.h"
#include "symbol_table.h"
#include "symbol_table/symbol_table_core.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* Recursive helper to emit forward declarations for all functions in imported modules.
 * This includes nested namespace imports with their combined namespace prefixes. */
void emit_import_forward_declarations_recursive(CodeGen *gen, Stmt **stmts, int count, const char *ns_prefix)
{
    for (int i = 0; i < count; i++)
    {
        Stmt *stmt = stmts[i];
        if (stmt == NULL)
            continue;

        if (stmt->type == STMT_FUNCTION)
        {
            FunctionStmt *fn = &stmt->as.function;
            /* Skip if already emitted (handles diamond imports) */
            if (fn->code_emitted)
                continue;
            /* Skip native functions without body */
            if (fn->is_native && fn->body_count == 0)
                continue;
            /* Skip main */
            char *fn_name = get_var_name(gen->arena, fn->name);
            if (strcmp(fn_name, "main") == 0)
                continue;

            /* Generate forward declaration with namespace prefix */
            char *prefixed_name = arena_sprintf(gen->arena, "%s__%s", ns_prefix, fn_name);
            char *mangled_name = sn_mangle_name(gen->arena, prefixed_name);
            const char *ret_c = get_c_type(gen->arena, fn->return_type);

            indented_fprintf(gen, 0, "%s %s(RtArenaV2 *", ret_c, mangled_name);
            for (int j = 0; j < fn->param_count; j++)
            {
                const char *param_type = get_c_param_type(gen->arena, fn->params[j].type);
                fprintf(gen->output, ", %s", param_type);
            }
            fprintf(gen->output, ");\n");
        }
        else if (stmt->type == STMT_IMPORT && stmt->as.import.namespace != NULL &&
                 stmt->as.import.imported_stmts != NULL)
        {
            /* Nested namespace import - recursively emit forward declarations */
            ImportStmt *imp = &stmt->as.import;
            char nested_ns[256];
            int nested_len = imp->namespace->length < 255 ? imp->namespace->length : 255;
            memcpy(nested_ns, imp->namespace->start, nested_len);
            nested_ns[nested_len] = '\0';

            /* Combine parent namespace with nested namespace */
            char *combined_prefix = arena_sprintf(gen->arena, "%s__%s", ns_prefix, nested_ns);

            emit_import_forward_declarations_recursive(gen, imp->imported_stmts, imp->imported_count, combined_prefix);
        }
    }
}

/* Helper function to recursively add namespace symbols to the current scope.
 * This handles nested namespaces (e.g., moduleB imports uuid as randomB). */
void add_namespace_symbols_to_scope(CodeGen *gen, Symbol *ns_sym)
{
    for (Symbol *sym = ns_sym->namespace_symbols; sym != NULL; sym = sym->next)
    {
        /* Recursively process nested namespaces */
        if (sym->is_namespace)
        {
            add_namespace_symbols_to_scope(gen, sym);
            continue;
        }

        if (sym->type != NULL && sym->type->kind == TYPE_FUNCTION)
        {
            /* For native functions, we need to preserve c_alias and is_native */
            if (sym->is_native)
            {
                symbol_table_add_native_function(gen->symbol_table, sym->name, sym->type,
                                                 sym->func_mod, sym->declared_func_mod);
                /* Copy the c_alias to the newly added symbol */
                Symbol *added = symbol_table_lookup_symbol_current(gen->symbol_table, sym->name);
                if (added != NULL)
                {
                    added->c_alias = sym->c_alias;
                }
            }
            else
            {
                symbol_table_add_function(gen->symbol_table, sym->name, sym->type,
                                          sym->func_mod, sym->declared_func_mod);
            }
        }
        else
        {
            /* Add namespace-level variables as SYMBOL_GLOBAL so they can be
             * distinguished from function-local variables during code generation. */
            symbol_table_add_symbol_with_kind(gen->symbol_table, sym->name, sym->type, SYMBOL_GLOBAL);
            /* Copy the is_static flag from the original symbol - this is critical for
             * code generation to know whether to prefix the variable name with the namespace. */
            Symbol *added = symbol_table_lookup_symbol_current(gen->symbol_table, sym->name);
            if (added != NULL)
            {
                added->is_static = sym->is_static;
            }
        }
    }
}
