#include "code_gen/code_gen_stmt.h"
#include "code_gen/code_gen_stmt_loop.h"
#include "code_gen/code_gen_stmt_capture.h"
#include "code_gen/code_gen_expr.h"
#include "code_gen/code_gen_util.h"
#include "debug.h"
#include "symbol_table.h"
#include "symbol_table/symbol_table_core.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* Forward declaration for recursive helper */
static void emit_import_forward_declarations_recursive(CodeGen *gen, Stmt **stmts, int count, const char *ns_prefix);

/* Recursive helper to emit forward declarations for all functions in imported modules.
 * This includes nested namespace imports with their combined namespace prefixes. */
static void emit_import_forward_declarations_recursive(CodeGen *gen, Stmt **stmts, int count, const char *ns_prefix)
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

            indented_fprintf(gen, 0, "%s %s(RtManagedArena *", ret_c, mangled_name);
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

/* Threshold for stack vs heap allocation for structs.
 * Structs smaller than this are stack-allocated.
 * Structs >= this size are heap-allocated via rt_arena_alloc.
 * This matches the same threshold used for fixed arrays. */
#define STRUCT_STACK_THRESHOLD 8192  /* 8KB */

/* Helper function to recursively add namespace symbols to the current scope.
 * This handles nested namespaces (e.g., moduleB imports uuid as randomB). */
static void add_namespace_symbols_to_scope(CodeGen *gen, Symbol *ns_sym)
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

/* Generate thread sync as a statement - assigns results back to variables
 * For single sync (r!): r = sync_result
 * For sync list ([r1, r2, r3]!): r1 = sync_result1; r2 = sync_result2; ... */
static void code_gen_thread_sync_statement(CodeGen *gen, Expr *expr, int indent)
{
    ThreadSyncExpr *sync = &expr->as.thread_sync;

    if (sync->is_array)
    {
        /* Sync list: [r1, r2, r3]!
         * Generate sync and assignment for each variable */
        Expr *list_expr = sync->handle;
        if (list_expr->type != EXPR_SYNC_LIST)
        {
            fprintf(stderr, "Error: Multi-sync requires sync list expression\n");
            exit(1);
        }

        SyncListExpr *sync_list = &list_expr->as.sync_list;

        if (sync_list->element_count == 0)
        {
            /* Empty sync list - no-op */
            return;
        }

        /* Sync each variable and assign result back */
        for (int i = 0; i < sync_list->element_count; i++)
        {
            Expr *elem = sync_list->elements[i];
            if (elem->type != EXPR_VARIABLE)
            {
                fprintf(stderr, "Error: Sync list elements must be variables\n");
                exit(1);
            }

            char *raw_var_name = get_var_name(gen->arena, elem->as.variable.name);
            char *var_name = sn_mangle_name(gen->arena, raw_var_name);

            /* Look up the variable's type from the symbol table
             * The elem->expr_type may not be set for array elements */
            Symbol *sym = symbol_table_lookup_symbol(gen->symbol_table, elem->as.variable.name);
            Type *result_type = (sym != NULL) ? sym->type : elem->expr_type;

            /* Check if void - just sync, no assignment */
            if (result_type == NULL || result_type->kind == TYPE_VOID)
            {
                indented_fprintf(gen, indent, "rt_thread_sync(%s);\n", var_name);
                continue;
            }

            const char *c_type = get_c_type(gen->arena, result_type);
            const char *rt_type = get_rt_result_type(result_type);

            bool is_primitive = (result_type->kind == TYPE_INT ||
                                result_type->kind == TYPE_LONG ||
                                result_type->kind == TYPE_DOUBLE ||
                                result_type->kind == TYPE_BOOL ||
                                result_type->kind == TYPE_BYTE ||
                                result_type->kind == TYPE_CHAR);
            bool is_handle = gen->current_arena_var != NULL &&
                            (result_type->kind == TYPE_STRING || result_type->kind == TYPE_ARRAY);

            bool is_struct = (result_type->kind == TYPE_STRUCT);
            bool struct_needs_field_promotion = is_struct && struct_has_handle_fields(result_type);

            if (is_primitive || is_handle || is_struct)
            {
                /* For primitives/handle/struct types, we declared two variables:
                 * __var_pending__ (RtThreadHandle*) and var (actual type).
                 * Sync the pending handle if not NULL and assign to the typed var.
                 * Pattern:
                 * if (__var_pending__ != NULL) {
                 *     var = *(type *)sync(__var_pending__, ...);
                 *     __var_pending__ = NULL;
                 * } */
                char *pending_var = arena_sprintf(gen->arena, "__%s_pending__", raw_var_name);
                indented_fprintf(gen, indent, "if (%s != NULL) {\n", pending_var);

                if (struct_needs_field_promotion) {
                    /* Struct with handle fields - keep arena alive for field promotion */
                    indented_fprintf(gen, indent + 1, "%s = *(%s *)rt_thread_sync_with_result_keep_arena(%s, %s, %s);\n",
                        var_name, c_type, pending_var, ARENA_VAR(gen), rt_type);
                    /* Generate field promotion code */
                    char *field_promotion = gen_struct_field_promotion(gen, result_type, var_name,
                        ARENA_VAR(gen), arena_sprintf(gen->arena, "%s->thread_arena", pending_var));
                    if (field_promotion && field_promotion[0] != '\0') {
                        indented_fprintf(gen, indent + 1, "%s", field_promotion);
                    }
                    /* Clean up the thread arena after field promotion */
                    indented_fprintf(gen, indent + 1, "rt_thread_cleanup_arena(%s);\n", pending_var);
                } else {
                    indented_fprintf(gen, indent + 1, "%s = *(%s *)rt_thread_sync_with_result(%s, %s, %s);\n",
                        var_name, c_type, pending_var, ARENA_VAR(gen), rt_type);
                }

                indented_fprintf(gen, indent + 1, "%s = NULL;\n", pending_var);
                indented_fprintf(gen, indent, "}\n");
            }
            else
            {
                /* For reference types (arrays, strings) without arena mode,
                 * direct assignment works because both are pointer types */
                indented_fprintf(gen, indent, "%s = (%s)rt_thread_sync_with_result(%s, %s, %s);\n",
                    var_name, c_type, var_name, ARENA_VAR(gen), rt_type);
            }
        }
    }
    else
    {
        /* Single sync: r!
         * Only assign back if the handle is a variable */
        Expr *handle = sync->handle;

        if (handle->type == EXPR_VARIABLE)
        {
            char *raw_var_name = get_var_name(gen->arena, handle->as.variable.name);
            char *var_name = sn_mangle_name(gen->arena, raw_var_name);
            Type *result_type = expr->expr_type;

            /* Check if void - just sync, no assignment */
            if (result_type == NULL || result_type->kind == TYPE_VOID)
            {
                indented_fprintf(gen, indent, "rt_thread_sync(%s);\n", var_name);
                return;
            }

            const char *c_type = get_c_type(gen->arena, result_type);
            const char *rt_type = get_rt_result_type(result_type);

            bool is_primitive = (result_type->kind == TYPE_INT ||
                                result_type->kind == TYPE_LONG ||
                                result_type->kind == TYPE_DOUBLE ||
                                result_type->kind == TYPE_BOOL ||
                                result_type->kind == TYPE_BYTE ||
                                result_type->kind == TYPE_CHAR);
            bool is_handle = gen->current_arena_var != NULL &&
                            (result_type->kind == TYPE_STRING || result_type->kind == TYPE_ARRAY);
            bool is_struct = (result_type->kind == TYPE_STRUCT);
            bool struct_needs_field_promotion = is_struct && struct_has_handle_fields(result_type);

            if (is_primitive || is_handle || is_struct)
            {
                /* For primitives/handle/struct types, we declared two variables:
                 * __var_pending__ (RtThreadHandle*) and var (actual type).
                 * Sync the pending handle if not NULL and assign to the typed var.
                 * Pattern:
                 * if (__var_pending__ != NULL) {
                 *     var = *(type *)sync(__var_pending__, ...);
                 *     __var_pending__ = NULL;
                 * } */
                char *pending_var = arena_sprintf(gen->arena, "__%s_pending__", raw_var_name);
                indented_fprintf(gen, indent, "if (%s != NULL) {\n", pending_var);

                if (struct_needs_field_promotion) {
                    /* Struct with handle fields - keep arena alive for field promotion */
                    indented_fprintf(gen, indent + 1, "%s = *(%s *)rt_thread_sync_with_result_keep_arena(%s, %s, %s);\n",
                        var_name, c_type, pending_var, ARENA_VAR(gen), rt_type);
                    /* Generate field promotion code */
                    char *field_promotion = gen_struct_field_promotion(gen, result_type, var_name,
                        ARENA_VAR(gen), arena_sprintf(gen->arena, "%s->thread_arena", pending_var));
                    if (field_promotion && field_promotion[0] != '\0') {
                        indented_fprintf(gen, indent + 1, "%s", field_promotion);
                    }
                    /* Clean up the thread arena after field promotion */
                    indented_fprintf(gen, indent + 1, "rt_thread_cleanup_arena(%s);\n", pending_var);
                } else {
                    indented_fprintf(gen, indent + 1, "%s = *(%s *)rt_thread_sync_with_result(%s, %s, %s);\n",
                        var_name, c_type, pending_var, ARENA_VAR(gen), rt_type);
                }

                indented_fprintf(gen, indent + 1, "%s = NULL;\n", pending_var);
                indented_fprintf(gen, indent, "}\n");
            }
            else
            {
                /* For reference types (arrays, strings) without arena mode,
                 * direct assignment works because both are pointer types */
                indented_fprintf(gen, indent, "%s = (%s)rt_thread_sync_with_result(%s, %s, %s);\n",
                    var_name, c_type, var_name, ARENA_VAR(gen), rt_type);
            }
        }
        else
        {
            /* Non-variable sync (e.g., &fn()!) - just execute the sync expression */
            char *expr_str = code_gen_expression(gen, expr);
            indented_fprintf(gen, indent, "%s;\n", expr_str);
        }
    }
}

void code_gen_expression_statement(CodeGen *gen, ExprStmt *stmt, int indent)
{
    DEBUG_VERBOSE("Entering code_gen_expression_statement");

    /* Special handling for thread sync statements - need to assign results back to variables */
    if (stmt->expression->type == EXPR_THREAD_SYNC)
    {
        code_gen_thread_sync_statement(gen, stmt->expression, indent);
        return;
    }

    char *expr_str = code_gen_expression(gen, stmt->expression);
    DEBUG_VERBOSE("Expression statement type: %p", (void*)stmt->expression->expr_type);
    if (stmt->expression->expr_type != NULL &&
        stmt->expression->expr_type->kind == TYPE_STRING && expression_produces_temp(stmt->expression))
    {
        // Skip freeing in arena context - arena handles cleanup
        if (gen->current_arena_var == NULL)
        {
            indented_fprintf(gen, indent, "{\n");
            indented_fprintf(gen, indent + 1, "char *_tmp = %s;\n", expr_str);
            indented_fprintf(gen, indent + 1, "(void)_tmp;\n");
            indented_fprintf(gen, indent + 1, "rt_free_string(_tmp);\n");
            indented_fprintf(gen, indent, "}\n");
        }
        else
        {
            indented_fprintf(gen, indent, "%s;\n", expr_str);
        }
    }
    else if (stmt->expression->type == EXPR_CALL && stmt->expression->expr_type->kind == TYPE_VOID)
    {
        // Statement expressions need a semicolon after them
        indented_fprintf(gen, indent, "%s;\n", expr_str);
    }
    else
    {
        indented_fprintf(gen, indent, "%s;\n", expr_str);
    }
}

void code_gen_struct_methods(CodeGen *gen, StructDeclStmt *struct_decl, int indent)
{
    DEBUG_VERBOSE("Entering code_gen_struct_methods");

    /* Track already-emitted struct methods to avoid duplicates.
     * This can happen when the same module is imported via different namespaces. */
    char *raw_struct_name = arena_strndup(gen->arena, struct_decl->name.start, struct_decl->name.length);
    char *struct_name = sn_mangle_name(gen->arena, raw_struct_name);

    /* Check if this struct's methods have already been emitted */
    for (int i = 0; i < gen->emitted_struct_methods_count; i++)
    {
        if (strcmp(gen->emitted_struct_methods[i], struct_name) == 0)
        {
            /* Already emitted, skip */
            return;
        }
    }

    /* Mark this struct as emitted */
    if (gen->emitted_struct_methods_count >= gen->emitted_struct_methods_capacity)
    {
        int new_capacity = gen->emitted_struct_methods_capacity == 0 ? 8 : gen->emitted_struct_methods_capacity * 2;
        const char **new_array = arena_alloc(gen->arena, sizeof(const char *) * new_capacity);
        for (int i = 0; i < gen->emitted_struct_methods_count; i++)
        {
            new_array[i] = gen->emitted_struct_methods[i];
        }
        gen->emitted_struct_methods = new_array;
        gen->emitted_struct_methods_capacity = new_capacity;
    }
    gen->emitted_struct_methods[gen->emitted_struct_methods_count++] = struct_name;

    for (int j = 0; j < struct_decl->method_count; j++)
    {
        StructMethod *method = &struct_decl->methods[j];

        /* Skip native methods with no body - they are extern declared elsewhere */
        if (method->is_native && method->body == NULL)
        {
            continue;
        }

        /* Resolve return type (may be forward-declared struct without c_alias) */
        Type *resolved_return_type = resolve_struct_type(gen, method->return_type);
        const char *ret_type = get_c_type(gen->arena, resolved_return_type);

        /* Generate function signature */
        if (method->is_static)
        {
            if (method->param_count == 0)
            {
                indented_fprintf(gen, indent, "%s %s_%s(RtManagedArena *__caller_arena__) {\n",
                                 ret_type, struct_name, method->name);
            }
            else
            {
                indented_fprintf(gen, indent, "%s %s_%s(RtManagedArena *__caller_arena__",
                                 ret_type, struct_name, method->name);
                for (int k = 0; k < method->param_count; k++)
                {
                    Parameter *param = &method->params[k];
                    Type *resolved_param_type = resolve_struct_type(gen, param->type);
                    const char *param_type = get_c_param_type(gen->arena, resolved_param_type);
                    char *param_name = sn_mangle_name(gen->arena, arena_strndup(gen->arena, param->name.start, param->name.length));
                    indented_fprintf(gen, 0, ", %s %s", param_type, param_name);
                }
                indented_fprintf(gen, 0, ") {\n");
            }
        }
        else
        {
            /* Instance method: first parameter is self (pointer to struct) */
            if (struct_decl->is_native && struct_decl->c_alias != NULL)
            {
                /* Opaque handle: self type is the C alias pointer */
                indented_fprintf(gen, indent, "%s %s_%s(RtManagedArena *__caller_arena__, %s *__sn__self",
                                 ret_type, struct_name, method->name, struct_decl->c_alias);
            }
            else
            {
                /* Regular struct: self is pointer to struct */
                indented_fprintf(gen, indent, "%s %s_%s(RtManagedArena *__caller_arena__, %s *__sn__self",
                                 ret_type, struct_name, method->name, struct_name);
            }
            for (int k = 0; k < method->param_count; k++)
            {
                Parameter *param = &method->params[k];
                Type *resolved_param_type = resolve_struct_type(gen, param->type);
                const char *param_type = get_c_param_type(gen->arena, resolved_param_type);
                char *param_name = sn_mangle_name(gen->arena, arena_strndup(gen->arena, param->name.start, param->name.length));
                indented_fprintf(gen, 0, ", %s %s", param_type, param_name);
            }
            indented_fprintf(gen, 0, ") {\n");
        }

        /* Set up code generator state for method */
        char *method_full_name = arena_sprintf(gen->arena, "%s_%s", struct_name, method->name);
        char *saved_function = gen->current_function;
        Type *saved_return_type = gen->current_return_type;
        char *saved_arena_var = gen->current_arena_var;
        char *saved_function_arena = gen->function_arena_var;

        gen->current_function = method_full_name;
        gen->current_return_type = method->return_type;
        gen->current_arena_var = "__caller_arena__";
        gen->function_arena_var = "__caller_arena__";

        /* Push scope and add method params to symbol table for proper pinning */
        symbol_table_push_scope(gen->symbol_table);
        symbol_table_enter_arena(gen->symbol_table);
        for (int k = 0; k < method->param_count; k++)
        {
            symbol_table_add_symbol_full(gen->symbol_table, method->params[k].name,
                                         method->params[k].type, SYMBOL_PARAM,
                                         method->params[k].mem_qualifier);
        }

        /* Determine if we need a _return_value variable */
        bool has_return_value = (method->return_type && method->return_type->kind != TYPE_VOID);

        /* Add _return_value if needed */
        if (has_return_value)
        {
            const char *default_val = get_default_value(method->return_type);
            indented_fprintf(gen, indent + 1, "%s _return_value = %s;\n", ret_type, default_val);
        }

        /* Generate method body */
        for (int k = 0; k < method->body_count; k++)
        {
            if (method->body[k] != NULL)
            {
                code_gen_statement(gen, method->body[k], indent + 1);
            }
        }

        /* Add return label and return statement */
        indented_fprintf(gen, indent, "%s_return:\n", method_full_name);
        if (has_return_value)
        {
            indented_fprintf(gen, indent + 1, "return _return_value;\n");
        }
        else
        {
            indented_fprintf(gen, indent + 1, "return;\n");
        }

        /* Pop method scope */
        symbol_table_pop_scope(gen->symbol_table);

        /* Restore code generator state */
        gen->current_function = saved_function;
        gen->current_return_type = saved_return_type;
        gen->current_arena_var = saved_arena_var;
        gen->function_arena_var = saved_function_arena;

        /* Close function */
        indented_fprintf(gen, indent, "}\n\n");
    }
}

void code_gen_var_declaration(CodeGen *gen, VarDeclStmt *stmt, int indent)
{
    DEBUG_VERBOSE("Entering code_gen_var_declaration");

    char *raw_var_name = get_var_name(gen->arena, stmt->name);

    // Detect global scope: no current arena means we're at file scope
    // Global arrays with empty initializers must be initialized to NULL since C
    // doesn't allow function calls or compound literals in global initializers.
    // Arrays with actual values need runtime initialization (handled separately).
    bool is_global_scope = (gen->current_arena_var == NULL);

    // Static prefix for module-level static variables
    const char *static_prefix = (stmt->is_static && is_global_scope) ? "static " : "";

    /* If we're generating code for an imported namespace AND this is a global variable,
     * prefix the variable name with the appropriate namespace to avoid collisions.
     * Local variables inside functions should NOT be prefixed.
     *
     * For STATIC variables: use canonical_module_name so all aliases of the same module
     * share the same static variable storage.
     *
     * For NON-STATIC variables: use namespace_prefix so each alias has its own instance. */
    char *var_name;
    if (is_global_scope)
    {
        const char *prefix_to_use = NULL;
        if (stmt->is_static && gen->current_canonical_module != NULL)
        {
            /* Static variable: use canonical module name for sharing across aliases */
            prefix_to_use = gen->current_canonical_module;
        }
        else if (gen->current_namespace_prefix != NULL)
        {
            /* Non-static variable: use namespace prefix for per-alias instance */
            prefix_to_use = gen->current_namespace_prefix;
        }

        if (prefix_to_use != NULL)
        {
            char *prefixed_name = arena_sprintf(gen->arena, "%s__%s", prefix_to_use, raw_var_name);
            var_name = sn_mangle_name(gen->arena, prefixed_name);
        }
        else
        {
            var_name = sn_mangle_name(gen->arena, raw_var_name);
        }
    }
    else
    {
        var_name = sn_mangle_name(gen->arena, raw_var_name);
    }

    /* For global variables, check if we've already emitted this exact variable name.
     * This prevents double emission in diamond import scenarios where
     * the same module is reachable via multiple import paths with the same prefix. */
    if (is_global_scope)
    {
        /* Check if already emitted (both static and non-static globals) */
        for (int i = 0; i < gen->emitted_globals_count; i++)
        {
            if (strcmp(gen->emitted_globals[i], var_name) == 0)
            {
                DEBUG_VERBOSE("Skipping duplicate global: %s", var_name);
                return;
            }
        }
        /* Track this global as emitted */
        if (gen->emitted_globals_count >= gen->emitted_globals_capacity)
        {
            int new_capacity = gen->emitted_globals_capacity == 0 ? 16 : gen->emitted_globals_capacity * 2;
            const char **new_array = arena_alloc(gen->arena, sizeof(const char *) * new_capacity);
            for (int i = 0; i < gen->emitted_globals_count; i++)
            {
                new_array[i] = gen->emitted_globals[i];
            }
            gen->emitted_globals = new_array;
            gen->emitted_globals_capacity = new_capacity;
        }
        gen->emitted_globals[gen->emitted_globals_count++] = arena_strdup(gen->arena, var_name);
    }

    /* For static global variables, also track in the static globals list
     * for backwards compatibility with existing code that checks that list. */
    if (stmt->is_static && is_global_scope)
    {
        /* Check if already emitted in static globals list */
        bool found_in_static = false;
        for (int i = 0; i < gen->emitted_static_globals_count; i++)
        {
            if (strcmp(gen->emitted_static_globals[i], var_name) == 0)
            {
                found_in_static = true;
                break;
            }
        }
        if (!found_in_static)
        {
            /* Not emitted yet - track it */
            if (gen->emitted_static_globals_count >= gen->emitted_static_globals_capacity)
            {
                int new_capacity = gen->emitted_static_globals_capacity == 0 ? 8 : gen->emitted_static_globals_capacity * 2;
                const char **new_array = arena_alloc(gen->arena, sizeof(const char *) * new_capacity);
                for (int i = 0; i < gen->emitted_static_globals_count; i++)
                {
                    new_array[i] = gen->emitted_static_globals[i];
                }
                gen->emitted_static_globals = new_array;
                gen->emitted_static_globals_capacity = new_capacity;
            }
            gen->emitted_static_globals[gen->emitted_static_globals_count++] = arena_strdup(gen->arena, var_name);
            DEBUG_VERBOSE("Tracking static global: %s", var_name);
        }
    }

    if (is_global_scope && stmt->type->kind == TYPE_ARRAY)
    {
        // Check if this is an empty initializer or no initializer
        bool is_empty = (stmt->initializer == NULL);
        if (!is_empty && stmt->initializer->type == EXPR_ARRAY)
        {
            is_empty = (stmt->initializer->as.array.element_count == 0);
        }

        if (is_empty)
        {
            const char *type_c = get_c_type(gen->arena, stmt->type);
            /* Global variables use SYMBOL_GLOBAL for correct pinning with __main_arena__ */
            symbol_table_add_symbol_full(gen->symbol_table, stmt->name, stmt->type, SYMBOL_GLOBAL, stmt->mem_qualifier);
            /* Set sync modifier if present */
            if (stmt->sync_modifier == SYNC_ATOMIC)
            {
                Symbol *sym = symbol_table_lookup_symbol_current(gen->symbol_table, stmt->name);
                if (sym != NULL) sym->sync_mod = SYNC_ATOMIC;
            }
            indented_fprintf(gen, indent, "%s%s %s = RT_HANDLE_NULL;\n", static_prefix, type_c, var_name);
            return;
        }
        // Non-empty global arrays will fall through and get the function call initializer,
        // which may cause C compile errors. This is a known limitation - global arrays
        // with values should be avoided or initialized in main().
    }

    // Check if this is a thread spawn assignment.
    // For thread spawns with primitive types, we declare TWO variables:
    //   1. __varname_pending__ of type RtThreadHandle* to hold the handle
    //   2. varname of the actual type to hold the result after sync
    // For reference types (arrays, strings), we use the actual type directly
    // since both handles and results are pointer types.
    bool is_thread_spawn = (stmt->initializer != NULL &&
                            stmt->initializer->type == EXPR_THREAD_SPAWN);
    bool is_primitive_type = (stmt->type->kind == TYPE_INT ||
                              stmt->type->kind == TYPE_LONG ||
                              stmt->type->kind == TYPE_DOUBLE ||
                              stmt->type->kind == TYPE_BOOL ||
                              stmt->type->kind == TYPE_BYTE ||
                              stmt->type->kind == TYPE_CHAR);
    /* Handle types (array/string in arena mode) also need a pending variable
     * because RtHandle (uint32_t) can't hold a RtThreadHandle pointer.
     * EXCEPTION: Arrays with 'any' elements need special conversion logic
     * (rt_array_to_any_*), so they must go through the original code path. */
    bool is_any_element_array = false;
    if (stmt->type->kind == TYPE_ARRAY && stmt->type->as.array.element_type != NULL)
    {
        Type *elem = stmt->type->as.array.element_type;
        /* Check 1D: any[] */
        if (elem->kind == TYPE_ANY) is_any_element_array = true;
        /* Check 2D: any[][] */
        else if (elem->kind == TYPE_ARRAY && elem->as.array.element_type != NULL &&
                 elem->as.array.element_type->kind == TYPE_ANY) is_any_element_array = true;
        /* Check 3D: any[][][] */
        else if (elem->kind == TYPE_ARRAY && elem->as.array.element_type != NULL &&
                 elem->as.array.element_type->kind == TYPE_ARRAY &&
                 elem->as.array.element_type->as.array.element_type != NULL &&
                 elem->as.array.element_type->as.array.element_type->kind == TYPE_ANY) is_any_element_array = true;
    }
    bool is_spawn_handle_result = gen->current_arena_var != NULL &&
                                  (stmt->type->kind == TYPE_STRING ||
                                   (stmt->type->kind == TYPE_ARRAY && !is_any_element_array));
    /* Struct types also need a pending variable - the result type is the struct,
     * not RtThreadHandle*, so we need separate variables for the handle and result */
    bool is_struct_result = (stmt->type->kind == TYPE_STRUCT);

    /* Check if this type could potentially be used with thread spawn later
     * (via conditional assignment). If so, we always declare a pending variable. */
    bool needs_pending_var = is_primitive_type || is_spawn_handle_result || is_struct_result;

    const char *type_c = get_c_type(gen->arena, stmt->type);

    // For types that could be thread spawn results, always declare a pending variable
    // This enables conditional thread spawn assignment: h = &compute() inside if blocks
    // EXCEPTIONS:
    // - 'as ref' and 'as val' variables have special memory handling
    // - Primitives captured by closures need special reference treatment
    bool has_special_mem_qual = (stmt->mem_qualifier == MEM_AS_REF || stmt->mem_qualifier == MEM_AS_VAL);
    bool is_captured_primitive = is_primitive_type && code_gen_is_captured_primitive(gen, raw_var_name);
    if (needs_pending_var && !is_global_scope && !has_special_mem_qual && !is_captured_primitive)
    {
        char *pending_var = arena_sprintf(gen->arena, "__%s_pending__", raw_var_name);

        if (is_thread_spawn)
        {
            // Thread spawn initializer: assign spawn to pending, leave var uninitialized
            char *init_str = code_gen_expression(gen, stmt->initializer);
            indented_fprintf(gen, indent, "RtThreadHandle *%s = %s;\n", pending_var, init_str);
            indented_fprintf(gen, indent, "%s %s;\n", type_c, var_name);
        }
        else
        {
            // Non-thread-spawn initializer: pending is NULL, var gets the value
            indented_fprintf(gen, indent, "RtThreadHandle *%s = NULL;\n", pending_var);
            if (stmt->initializer)
            {
                /* For handle types (array/string in arena mode), evaluate in handle mode
                 * so the expression returns RtHandle values. */
                bool prev_as_handle = gen->expr_as_handle;
                if (is_spawn_handle_result)
                {
                    gen->expr_as_handle = true;
                }
                char *init_str = code_gen_expression(gen, stmt->initializer);
                gen->expr_as_handle = prev_as_handle;
                indented_fprintf(gen, indent, "%s %s = %s;\n", type_c, var_name, init_str);
            }
            else
            {
                indented_fprintf(gen, indent, "%s %s;\n", type_c, var_name);
            }
        }

        // Add to symbol table
        symbol_table_add_symbol_full(gen->symbol_table, stmt->name, stmt->type, SYMBOL_LOCAL, stmt->mem_qualifier);
        /* Set sync modifier if present */
        if (stmt->sync_modifier == SYNC_ATOMIC)
        {
            Symbol *sym = symbol_table_lookup_symbol_current(gen->symbol_table, stmt->name);
            if (sym != NULL) sym->sync_mod = SYNC_ATOMIC;
        }
        return;
    }

    // Check if this primitive is captured by a closure - if so, treat it like 'as ref'
    // This ensures mutations inside closures are visible to the outer scope
    MemoryQualifier effective_qual = stmt->mem_qualifier;
    if (effective_qual == MEM_DEFAULT && code_gen_is_captured_primitive(gen, raw_var_name))
    {
        effective_qual = MEM_AS_REF;
    }

    // Add to symbol table with effective qualifier so accesses are dereferenced correctly
    // Global variables use SYMBOL_GLOBAL for correct pinning with __main_arena__
    SymbolKind sym_kind = is_global_scope ? SYMBOL_GLOBAL : SYMBOL_LOCAL;
    symbol_table_add_symbol_full(gen->symbol_table, stmt->name, stmt->type, sym_kind, effective_qual);
    {
        Symbol *sym = symbol_table_lookup_symbol_current(gen->symbol_table, stmt->name);
        if (sym != NULL)
        {
            /* Set sync modifier if present */
            if (stmt->sync_modifier == SYNC_ATOMIC)
            {
                sym->sync_mod = SYNC_ATOMIC;
            }
        }
    }

    char *init_str;
    if (stmt->initializer)
    {
        /* For lambda initializers, track the variable name so we can detect recursive lambdas */
        if (stmt->initializer->type == EXPR_LAMBDA)
        {
            gen->current_decl_var_name = raw_var_name;
            gen->recursive_lambda_id = -1;  /* Will be set by lambda codegen if recursive */
        }

        /* For handle-type variables, evaluate initializer in handle mode
         * so that expression generators return RtHandle expressions.
         * Exception: 'as val' needs raw pointer for clone functions.
         * Also: when boxing an array into 'any', use handle mode
         * so the RtHandle value gets stored (not a pinned pointer).
         * Note: strings stay as raw char* in 'any' boxes because runtime
         * functions (like rt_any_promote) need real pointers. */
        bool prev_as_handle = gen->expr_as_handle;
        if (!is_global_scope && gen->current_arena_var != NULL
            && stmt->mem_qualifier != MEM_AS_VAL)
        {
            if (is_handle_type(stmt->type))
            {
                gen->expr_as_handle = true;
            }
            else if (stmt->type->kind == TYPE_ANY && stmt->initializer != NULL &&
                     stmt->initializer->expr_type != NULL &&
                     stmt->initializer->expr_type->kind == TYPE_ARRAY)
            {
                gen->expr_as_handle = true;
            }
        }

        /* For global scope handle types and struct types with function call initializers,
         * we need deferred initialization (in main). Set a temporary arena context
         * so the expression is generated with __main_arena__ instead of NULL. */
        char *saved_arena_var = gen->current_arena_var;
        if (is_global_scope && gen->current_arena_var == NULL)
        {
            bool will_need_deferred = is_handle_type(stmt->type) ||
                (stmt->type->kind == TYPE_STRUCT && stmt->initializer != NULL &&
                 (stmt->initializer->type == EXPR_CALL ||
                  stmt->initializer->type == EXPR_METHOD_CALL));
            if (will_need_deferred)
            {
                gen->current_arena_var = "__main_arena__";
                gen->expr_as_handle = is_handle_type(stmt->type);
            }
        }

        init_str = code_gen_expression(gen, stmt->initializer);

        gen->current_arena_var = saved_arena_var;

        gen->expr_as_handle = prev_as_handle;

        // When a local string variable is initialized from a parameter handle,
        // copy it to the local arena. Handles are arena-scoped and the parameter's
        // handle belongs to the caller's arena, so it can't be pinned locally.
        if (!is_global_scope && gen->current_arena_var != NULL &&
            stmt->type->kind == TYPE_STRING && stmt->mem_qualifier != MEM_AS_VAL &&
            stmt->initializer != NULL && stmt->initializer->type == EXPR_VARIABLE)
        {
            Symbol *init_sym = symbol_table_lookup_symbol(gen->symbol_table, stmt->initializer->as.variable.name);
            if (init_sym != NULL && init_sym->kind == SYMBOL_PARAM)
            {
                init_str = arena_sprintf(gen->arena,
                    "rt_managed_strdup(%s, RT_HANDLE_NULL, (char *)rt_managed_pin(__caller_arena__, %s))",
                    ARENA_VAR(gen), init_str);
            }
        }

        // Global-scope handle variables (string/array) can't use function calls or
        // non-constant initializers in C. Use RT_HANDLE_NULL and record deferred
        // initialization to be emitted at the start of main().
        // Also defer struct types (like UUID) that have function call initializers.
        // Also defer any primitive type with function call initializers.
        bool needs_deferred_init = false;
        if (is_global_scope)
        {
            if (is_handle_type(stmt->type))
            {
                needs_deferred_init = true;
            }
            else if (stmt->initializer != NULL &&
                     (stmt->initializer->type == EXPR_CALL ||
                      stmt->initializer->type == EXPR_METHOD_CALL))
            {
                // Any function call initializer at global scope needs deferred init
                needs_deferred_init = true;
            }
        }

        if (needs_deferred_init)
        {
            // Record deferred initialization: the original init_str contains the
            // expression that should be assigned in main().
            // Since we already set expr_as_handle = true for handle types, init_str
            // is already the correct expression (e.g., rt_managed_strdup for strings).
            char *deferred_value = arena_strdup(gen->arena, init_str);

            // Grow the deferred list if needed
            if (gen->deferred_global_count >= gen->deferred_global_capacity)
            {
                int new_cap = gen->deferred_global_capacity == 0 ? 8 : gen->deferred_global_capacity * 2;
                char **new_names = arena_alloc(gen->arena, new_cap * sizeof(char *));
                char **new_values = arena_alloc(gen->arena, new_cap * sizeof(char *));
                for (int i = 0; i < gen->deferred_global_count; i++)
                {
                    new_names[i] = gen->deferred_global_names[i];
                    new_values[i] = gen->deferred_global_values[i];
                }
                gen->deferred_global_names = new_names;
                gen->deferred_global_values = new_values;
                gen->deferred_global_capacity = new_cap;
            }
            gen->deferred_global_names[gen->deferred_global_count] = var_name;
            gen->deferred_global_values[gen->deferred_global_count] = deferred_value;
            gen->deferred_global_count++;

            // Use appropriate null/zero initializer for the declaration
            if (is_handle_type(stmt->type))
            {
                init_str = arena_strdup(gen->arena, "RT_HANDLE_NULL");
            }
            else if (stmt->type->kind == TYPE_STRUCT)
            {
                // For struct types, use NULL (for pointer types like RtUuid *)
                // or {0} for value types
                if (stmt->type->as.struct_type.is_native && stmt->type->as.struct_type.c_alias != NULL)
                {
                    init_str = arena_strdup(gen->arena, "NULL");
                }
                else
                {
                    init_str = arena_strdup(gen->arena, "{0}");
                }
            }
            else
            {
                // For primitive types (int, bool, double, etc.), use 0
                init_str = arena_strdup(gen->arena, "0");
            }
        }

        // Handle boxing when assigning to 'any' type
        // If the variable is 'any' and initializer is a concrete type, wrap with boxing function
        if (stmt->type->kind == TYPE_ANY && stmt->initializer->expr_type != NULL &&
            stmt->initializer->expr_type->kind != TYPE_ANY)
        {
            init_str = code_gen_box_value(gen, init_str, stmt->initializer->expr_type);
        }

        // Handle conversion when assigning typed array to any[], any[][], or any[][][]
        if (stmt->type->kind == TYPE_ARRAY &&
            stmt->type->as.array.element_type != NULL &&
            stmt->initializer->expr_type != NULL &&
            stmt->initializer->expr_type->kind == TYPE_ARRAY &&
            stmt->initializer->expr_type->as.array.element_type != NULL)
        {
            Type *decl_elem = stmt->type->as.array.element_type;
            Type *src_elem = stmt->initializer->expr_type->as.array.element_type;

            // Check for 3D array: any[][][] = T[][][]
            if (decl_elem->kind == TYPE_ARRAY &&
                decl_elem->as.array.element_type != NULL &&
                decl_elem->as.array.element_type->kind == TYPE_ARRAY &&
                decl_elem->as.array.element_type->as.array.element_type != NULL &&
                decl_elem->as.array.element_type->as.array.element_type->kind == TYPE_ANY &&
                src_elem->kind == TYPE_ARRAY &&
                src_elem->as.array.element_type != NULL &&
                src_elem->as.array.element_type->kind == TYPE_ARRAY &&
                src_elem->as.array.element_type->as.array.element_type != NULL &&
                src_elem->as.array.element_type->as.array.element_type->kind != TYPE_ANY)
            {
                Type *innermost_src = src_elem->as.array.element_type->as.array.element_type;
                const char *conv_func = NULL;
                switch (innermost_src->kind)
                {
                case TYPE_INT:
                case TYPE_INT32:
                case TYPE_UINT:
                case TYPE_UINT32:
                case TYPE_LONG:
                    conv_func = "rt_array3_to_any_long";
                    break;
                case TYPE_DOUBLE:
                case TYPE_FLOAT:
                    conv_func = "rt_array3_to_any_double";
                    break;
                case TYPE_CHAR:
                    conv_func = "rt_array3_to_any_char";
                    break;
                case TYPE_BOOL:
                    conv_func = "rt_array3_to_any_bool";
                    break;
                case TYPE_BYTE:
                    conv_func = "rt_array3_to_any_byte";
                    break;
                case TYPE_STRING:
                    conv_func = "rt_array3_to_any_string";
                    break;
                default:
                    break;
                }
                if (conv_func != NULL)
                {
                    if (gen->current_arena_var != NULL) {
                        init_str = arena_sprintf(gen->arena, "%s_h(%s, %s)", conv_func, ARENA_VAR(gen), init_str);
                    } else {
                        init_str = arena_sprintf(gen->arena, "%s(%s, %s)", conv_func, ARENA_VAR(gen), init_str);
                    }
                }
            }
            // Check for 2D array: any[][] = T[][]
            else if (decl_elem->kind == TYPE_ARRAY &&
                decl_elem->as.array.element_type != NULL &&
                decl_elem->as.array.element_type->kind == TYPE_ANY &&
                src_elem->kind == TYPE_ARRAY &&
                src_elem->as.array.element_type != NULL &&
                src_elem->as.array.element_type->kind != TYPE_ANY)
            {
                Type *inner_src = src_elem->as.array.element_type;
                const char *conv_func = NULL;
                switch (inner_src->kind)
                {
                case TYPE_INT:
                case TYPE_INT32:
                case TYPE_UINT:
                case TYPE_UINT32:
                case TYPE_LONG:
                    conv_func = "rt_array2_to_any_long";
                    break;
                case TYPE_DOUBLE:
                case TYPE_FLOAT:
                    conv_func = "rt_array2_to_any_double";
                    break;
                case TYPE_CHAR:
                    conv_func = "rt_array2_to_any_char";
                    break;
                case TYPE_BOOL:
                    conv_func = "rt_array2_to_any_bool";
                    break;
                case TYPE_BYTE:
                    conv_func = "rt_array2_to_any_byte";
                    break;
                case TYPE_STRING:
                    conv_func = "rt_array2_to_any_string";
                    break;
                default:
                    break;
                }
                if (conv_func != NULL)
                {
                    if (gen->current_arena_var != NULL) {
                        init_str = arena_sprintf(gen->arena, "%s_h(%s, %s)", conv_func, ARENA_VAR(gen), init_str);
                    } else {
                        init_str = arena_sprintf(gen->arena, "%s(%s, %s)", conv_func, ARENA_VAR(gen), init_str);
                    }
                }
            }
            // Check for 1D array: any[] = T[]
            else if (decl_elem->kind == TYPE_ANY && src_elem->kind != TYPE_ANY)
            {
                const char *conv_func = NULL;
                switch (src_elem->kind)
                {
                case TYPE_INT:
                case TYPE_INT32:
                case TYPE_UINT:
                case TYPE_UINT32:
                case TYPE_LONG:
                    conv_func = "rt_array_to_any_long";
                    break;
                case TYPE_DOUBLE:
                case TYPE_FLOAT:
                    conv_func = "rt_array_to_any_double";
                    break;
                case TYPE_CHAR:
                    conv_func = "rt_array_to_any_char";
                    break;
                case TYPE_BOOL:
                    conv_func = "rt_array_to_any_bool";
                    break;
                case TYPE_BYTE:
                    conv_func = "rt_array_to_any_byte";
                    break;
                case TYPE_STRING:
                    conv_func = "rt_array_to_any_string";
                    break;
                default:
                    break;
                }
                if (conv_func != NULL)
                {
                    if (gen->current_arena_var != NULL)
                    {
                        if (src_elem->kind == TYPE_STRING)
                        {
                            /* String arrays store RtHandle elements — use dedicated _h function.
                             * Clone result to handle for the declaration. */
                            init_str = arena_sprintf(gen->arena,
                                "rt_array_clone_void_h(%s, RT_HANDLE_NULL, rt_array_to_any_string_h(%s, %s))",
                                ARENA_VAR(gen), ARENA_VAR(gen), init_str);
                        }
                        else
                        {
                            /* Non-string types: pin source, legacy convert, clone to handle. */
                            const char *elem_c = get_c_type(gen->arena, src_elem);
                            init_str = arena_sprintf(gen->arena,
                                "rt_array_clone_void_h(%s, RT_HANDLE_NULL, %s(%s, (%s *)rt_managed_pin_array(%s, %s)))",
                                ARENA_VAR(gen), conv_func, ARENA_VAR(gen), elem_c, ARENA_VAR(gen), init_str);
                        }
                    }
                    else
                    {
                        init_str = arena_sprintf(gen->arena, "%s(%s, %s)", conv_func, ARENA_VAR(gen), init_str);
                    }
                }
            }
        }

        // Handle 'as val' - create a copy for arrays and strings (handle-based)
        if (stmt->mem_qualifier == MEM_AS_VAL)
        {
            if (stmt->type->kind == TYPE_ARRAY)
            {
                // Get element type suffix for the clone function
                Type *elem_type = stmt->type->as.array.element_type;
                const char *suffix = code_gen_type_suffix(elem_type);
                init_str = arena_sprintf(gen->arena, "rt_array_clone_%s_h(%s, RT_HANDLE_NULL, %s)", suffix, ARENA_VAR(gen), init_str);
            }
            else if (stmt->type->kind == TYPE_STRING)
            {
                init_str = arena_sprintf(gen->arena, "rt_managed_strdup(%s, RT_HANDLE_NULL, %s)", ARENA_VAR(gen), init_str);
            }
        }
    }
    else
    {
        init_str = arena_strdup(gen->arena, get_default_value(stmt->type));
    }

    // Handle 'as ref' or captured primitives - heap-allocate via arena
    if (effective_qual == MEM_AS_REF)
    {
        // Allocate on arena and store pointer
        // e.g., long *x = (long *)rt_arena_alloc(__arena_1__, sizeof(long)); *x = 42L;
        // When the function returns a closure type, allocate in caller's arena so
        // the captured data survives the function's local arena destruction.
        // Exception: main() has no caller, so always use local arena.
        bool in_main = (gen->current_function != NULL && strcmp(gen->current_function, "main") == 0);
        const char *alloc_arena = (gen->allocate_closure_in_caller_arena &&
                                   strcmp(gen->current_arena_var, "__local_arena__") == 0 &&
                                   !in_main)
            ? "__caller_arena__"
            : ARENA_VAR(gen);
        indented_fprintf(gen, indent, "%s *%s = (%s *)rt_arena_alloc(%s, sizeof(%s));\n",
                         type_c, var_name, type_c, alloc_arena, type_c);
        indented_fprintf(gen, indent, "*%s = %s;\n", var_name, init_str);
    }
    // Handle large struct allocation (>= 8KB threshold) - heap-allocate via arena
    else if (stmt->type->kind == TYPE_STRUCT && gen->current_arena_var != NULL)
    {
        // Get the struct size - try from the type itself first, otherwise look up from symbol table
        int struct_size = (int)stmt->type->as.struct_type.size;
        if (struct_size == 0 && stmt->type->as.struct_type.name != NULL)
        {
            // Look up the struct type from the symbol table which has the computed layout
            Token struct_name_token = {
                .start = stmt->type->as.struct_type.name,
                .length = (int)strlen(stmt->type->as.struct_type.name)
            };
            Symbol *struct_sym = symbol_table_lookup_type(gen->symbol_table, struct_name_token);
            if (struct_sym != NULL && struct_sym->type != NULL && struct_sym->type->kind == TYPE_STRUCT)
            {
                struct_size = (int)struct_sym->type->as.struct_type.size;
            }
        }
        if (struct_size >= STRUCT_STACK_THRESHOLD)
        {
            // Large struct: allocate on arena and store as pointer
            // e.g., LargeStruct *s = (LargeStruct *)rt_arena_alloc(__arena_1__, sizeof(LargeStruct));
            //       *s = (LargeStruct){ .field = value, ... };
            indented_fprintf(gen, indent, "%s *%s = (%s *)rt_arena_alloc(%s, sizeof(%s));\n",
                             type_c, var_name, type_c, ARENA_VAR(gen), type_c);
            indented_fprintf(gen, indent, "*%s = %s;\n", var_name, init_str);

            // Update symbol table to mark as pointer for proper access
            Symbol *sym = symbol_table_lookup_symbol_current(gen->symbol_table, stmt->name);
            if (sym != NULL)
            {
                sym->mem_qual = MEM_AS_REF;  // Mark as pointer for proper dereferencing
            }
        }
        else
        {
            // Small struct: stack allocation with value semantics
            indented_fprintf(gen, indent, "%s%s %s = %s;\n", static_prefix, type_c, var_name, init_str);
        }
    }
    else
    {
        indented_fprintf(gen, indent, "%s%s %s = %s;\n", static_prefix, type_c, var_name, init_str);
    }

    /* For recursive lambdas, we need to fix up the self-reference after declaration.
     * The lambda's closure was created without the self-capture to avoid using
     * an uninitialized variable. Now that the variable is initialized, we can
     * set the self-reference in the closure. */
    if (gen->recursive_lambda_id >= 0 && stmt->initializer != NULL &&
        stmt->initializer->type == EXPR_LAMBDA)
    {
        int lambda_id = gen->recursive_lambda_id;
        /* Generate: ((__closure_N__ *)var)->field = var; */
        indented_fprintf(gen, indent, "((__closure_%d__ *)%s)->%s = %s;\n",
                         lambda_id, var_name, raw_var_name, var_name);
        gen->recursive_lambda_id = -1;
    }

    /* Clear the current decl var name */
    gen->current_decl_var_name = NULL;
}

void code_gen_free_locals(CodeGen *gen, Scope *scope, bool is_function, int indent)
{
    DEBUG_VERBOSE("Entering code_gen_free_locals");

    // Skip manual freeing when in arena context - arena handles all deallocation
    if (gen->current_arena_var != NULL)
    {
        return;
    }

    Symbol *sym = scope->symbols;
    while (sym)
    {
        if (sym->type && sym->type->kind == TYPE_STRING && sym->kind == SYMBOL_LOCAL)
        {
            char *var_name = sn_mangle_name(gen->arena, get_var_name(gen->arena, sym->name));
            indented_fprintf(gen, indent, "if (%s) {\n", var_name);
            if (is_function && gen->current_return_type && gen->current_return_type->kind == TYPE_STRING)
            {
                indented_fprintf(gen, indent + 1, "if (%s != _return_value) {\n", var_name);
                indented_fprintf(gen, indent + 2, "rt_free_string(%s);\n", var_name);
                indented_fprintf(gen, indent + 1, "}\n");
            }
            else
            {
                indented_fprintf(gen, indent + 1, "rt_free_string(%s);\n", var_name);
            }
            indented_fprintf(gen, indent, "}\n");
        }
        else if (sym->type && sym->type->kind == TYPE_ARRAY && sym->kind == SYMBOL_LOCAL)
        {
            char *var_name = sn_mangle_name(gen->arena, get_var_name(gen->arena, sym->name));
            Type *elem_type = sym->type->as.array.element_type;
            indented_fprintf(gen, indent, "if (%s) {\n", var_name);
            if (is_function && gen->current_return_type && gen->current_return_type->kind == TYPE_ARRAY)
            {
                indented_fprintf(gen, indent + 1, "if (%s != _return_value) {\n", var_name);
                if (elem_type && elem_type->kind == TYPE_STRING)
                {
                    indented_fprintf(gen, indent + 2, "rt_array_free_string(%s);\n", var_name);
                }
                else
                {
                    indented_fprintf(gen, indent + 2, "rt_array_free(%s);\n", var_name);
                }
                indented_fprintf(gen, indent + 1, "}\n");
            }
            else
            {
                if (elem_type && elem_type->kind == TYPE_STRING)
                {
                    indented_fprintf(gen, indent + 1, "rt_array_free_string(%s);\n", var_name);
                }
                else
                {
                    indented_fprintf(gen, indent + 1, "rt_array_free(%s);\n", var_name);
                }
            }
            indented_fprintf(gen, indent, "}\n");
        }
        sym = sym->next;
    }
}

void code_gen_block(CodeGen *gen, BlockStmt *stmt, int indent)
{
    DEBUG_VERBOSE("Entering code_gen_block");

    /* Note: BLOCK_SHARED and BLOCK_PRIVATE are no longer supported.
     * All blocks now use the function's arena (BLOCK_DEFAULT). */

    symbol_table_push_scope(gen->symbol_table);

    indented_fprintf(gen, indent, "{\n");

    for (int i = 0; i < stmt->count; i++)
    {
        code_gen_statement(gen, stmt->statements[i], indent + 1);
    }
    code_gen_free_locals(gen, gen->symbol_table->current, false, indent + 1);

    indented_fprintf(gen, indent, "}\n");
    symbol_table_pop_scope(gen->symbol_table);
}

void code_gen_function(CodeGen *gen, FunctionStmt *stmt)
{
    DEBUG_VERBOSE("Entering code_gen_function");

    /* Native functions without a body are external C declarations.
     * We don't generate any code - they must be provided via #pragma include
     * or linked via #pragma link. */
    if (stmt->is_native && stmt->body_count == 0)
    {
        DEBUG_VERBOSE("Skipping native function without body: %.*s", stmt->name.length, stmt->name.start);
        return;
    }

    char *old_function = gen->current_function;
    Type *old_return_type = gen->current_return_type;
    FunctionModifier old_func_modifier = gen->current_func_modifier;
    bool old_in_private_context = gen->in_private_context;
    bool old_in_shared_context = gen->in_shared_context;
    char *old_arena_var = gen->current_arena_var;
    int old_arena_depth = gen->arena_depth;

    char *raw_fn_name = get_var_name(gen->arena, stmt->name);
    DEBUG_VERBOSE("Code generating function: %s", raw_fn_name);
    bool is_main = strcmp(raw_fn_name, "main") == 0;

    /* Functions from imported modules need namespace prefixes to avoid name collisions.
     * When current_namespace_prefix is set (during import processing), prepend it to the
     * function name to generate unique C function names like __sn__ModuleA__functionName. */
    if (is_main || stmt->is_native)
    {
        gen->current_function = raw_fn_name;
    }
    else if (gen->current_namespace_prefix != NULL)
    {
        /* Namespace-prefixed function: __sn__Namespace__functionName */
        char prefixed_name[512];
        snprintf(prefixed_name, sizeof(prefixed_name), "%s__%s", gen->current_namespace_prefix, raw_fn_name);
        gen->current_function = sn_mangle_name(gen->arena, prefixed_name);
    }
    else
    {
        gen->current_function = sn_mangle_name(gen->arena, raw_fn_name);
    }

    /* Check if this exact function (with namespace prefix) has already been emitted.
     * This prevents double emission in diamond import scenarios where
     * the same module is reachable via multiple import paths with the same prefix. */
    for (int i = 0; i < gen->emitted_functions_count; i++)
    {
        if (strcmp(gen->emitted_functions[i], gen->current_function) == 0)
        {
            DEBUG_VERBOSE("Skipping already-emitted function: %s", gen->current_function);
            gen->current_function = old_function;
            return;
        }
    }

    /* Track this function as emitted */
    if (gen->emitted_functions_count >= gen->emitted_functions_capacity)
    {
        int new_capacity = gen->emitted_functions_capacity == 0 ? 16 : gen->emitted_functions_capacity * 2;
        const char **new_array = arena_alloc(gen->arena, sizeof(const char *) * new_capacity);
        for (int i = 0; i < gen->emitted_functions_count; i++)
        {
            new_array[i] = gen->emitted_functions[i];
        }
        gen->emitted_functions = new_array;
        gen->emitted_functions_capacity = new_capacity;
    }
    gen->emitted_functions[gen->emitted_functions_count++] = arena_strdup(gen->arena, gen->current_function);
    gen->current_return_type = stmt->return_type;
    gen->current_func_modifier = stmt->modifier;
    bool main_has_args = is_main && stmt->param_count == 1;  // Type checker validated it's str[]
    bool is_private = stmt->modifier == FUNC_PRIVATE;
    bool is_shared = stmt->modifier == FUNC_SHARED;

    // New arena model: ALL non-main functions receive __caller_arena__ as first parameter.
    // The modifier determines how the function uses it:
    //   shared:  __local_arena__ = __caller_arena__ (alias, no new arena)
    //   default: __local_arena__ = rt_arena_create(__caller_arena__) (new arena with parent)
    //   private: __local_arena__ = rt_arena_create(__caller_arena__) (new arena, strict escape)
    //
    // main() is special - it creates the root arena with no caller.
    //
    // For default functions returning heap types, the return value is promoted to
    // __caller_arena__ before __local_arena__ is destroyed.

    // Set up arena context - all functions use __local_arena__
    if (is_private) gen->in_private_context = true;
    gen->in_shared_context = is_shared;
    gen->current_arena_var = "__local_arena__";
    gen->function_arena_var = "__local_arena__";

    // Special case for main: always use "int" return type in C for standard entry point.
    const char *ret_c = is_main ? "int" : get_c_type(gen->arena, gen->current_return_type);
    // Determine if we need a _return_value variable: only for non-void or main.
    bool has_return_value = (gen->current_return_type && gen->current_return_type->kind != TYPE_VOID) || is_main;
    symbol_table_push_scope(gen->symbol_table);

    // All functions have arena context
    symbol_table_enter_arena(gen->symbol_table);

    for (int i = 0; i < stmt->param_count; i++)
    {
        /* Pass memory qualifier so code gen knows about 'as ref' parameters */
        symbol_table_add_symbol_full(gen->symbol_table, stmt->params[i].name, stmt->params[i].type,
                                     SYMBOL_PARAM, stmt->params[i].mem_qualifier);
    }

    // Pre-pass: scan function body for primitives captured by closures
    // These need to be declared as pointers for mutation persistence
    code_gen_scan_captured_primitives(gen, stmt->body, stmt->body_count);

    indented_fprintf(gen, 0, "%s %s(", ret_c, gen->current_function);

    // Main with args gets special C signature: int main(int argc, char **argv)
    if (main_has_args)
    {
        fprintf(gen->output, "int argc, char **argv");
    }
    else
    {
        // All non-main functions receive caller's arena as first parameter
        if (!is_main)
        {
            fprintf(gen->output, "RtManagedArena *__caller_arena__");
            if (stmt->param_count > 0)
            {
                fprintf(gen->output, ", ");
            }
        }

        for (int i = 0; i < stmt->param_count; i++)
        {
            const char *param_type_c = get_c_param_type(gen->arena, stmt->params[i].type);
            char *param_name = sn_mangle_name(gen->arena, get_var_name(gen->arena, stmt->params[i].name));

            /* 'as ref' primitive and struct parameters become pointer types */
            bool is_ref_param = false;
            if (stmt->params[i].mem_qualifier == MEM_AS_REF && stmt->params[i].type != NULL)
            {
                TypeKind kind = stmt->params[i].type->kind;
                bool is_prim = (kind == TYPE_INT || kind == TYPE_INT32 || kind == TYPE_UINT ||
                               kind == TYPE_UINT32 || kind == TYPE_LONG || kind == TYPE_DOUBLE ||
                               kind == TYPE_FLOAT || kind == TYPE_CHAR || kind == TYPE_BOOL ||
                               kind == TYPE_BYTE);
                bool is_struct = (kind == TYPE_STRUCT);
                is_ref_param = is_prim || is_struct;
            }
            if (is_ref_param)
            {
                fprintf(gen->output, "%s *%s", param_type_c, param_name);
            }
            else
            {
                fprintf(gen->output, "%s %s", param_type_c, param_name);
            }

            if (i < stmt->param_count - 1)
            {
                fprintf(gen->output, ", ");
            }
        }
    }
    indented_fprintf(gen, 0, ") {\n");

    // Set up __local_arena__ based on modifier:
    //   main:    create root arena (no parent)
    //   shared:  alias to caller's arena
    //   default: new arena with caller as parent
    //   private: new arena with caller as parent (strict escape rules enforced at compile time)
    if (is_main)
    {
        indented_fprintf(gen, 1, "RtManagedArena *__local_arena__ = rt_managed_arena_create();\n");
        indented_fprintf(gen, 1, "__main_arena__ = __local_arena__;\n");
        // Emit deferred global initializations (handle-type globals that couldn't
        // be initialized at file scope because C doesn't allow non-constant initializers)
        for (int i = 0; i < gen->deferred_global_count; i++)
        {
            indented_fprintf(gen, 1, "%s = %s;\n",
                             gen->deferred_global_names[i],
                             gen->deferred_global_values[i]);
        }
    }
    else if (is_shared)
    {
        indented_fprintf(gen, 1, "RtManagedArena *__local_arena__ = __caller_arena__;\n");
    }
    else
    {
        // default or private - create new child arena
        indented_fprintf(gen, 1, "RtManagedArena *__local_arena__ = rt_managed_arena_create_child(__caller_arena__);\n");
    }

    // Clone handle-type parameters from __caller_arena__ to __local_arena__.
    // This ensures handles passed to sub-functions (which receive __local_arena__
    // as their __caller_arena__) can be correctly resolved.
    if (!is_main && !is_shared && !main_has_args)
    {
        for (int i = 0; i < stmt->param_count; i++)
        {
            Type *param_type = stmt->params[i].type;
            if (param_type == NULL) continue;

            if (param_type->kind == TYPE_STRING)
            {
                /* Strings are immutable — cloning is safe and ensures the handle
                 * is resolvable when passed to sub-functions via __local_arena__.
                 * Use clone_any to search current arena first, then parent arenas.
                 * Index collisions are avoided by child arenas starting their indices
                 * at an offset from their parent's current count. */
                char *param_name = sn_mangle_name(gen->arena, get_var_name(gen->arena, stmt->params[i].name));
                indented_fprintf(gen, 1, "%s = rt_managed_clone_any(__local_arena__, __caller_arena__, %s);\n",
                                 param_name, param_name);
                /* Update symbol kind so pin calls use __local_arena__ instead of __caller_arena__ */
                Symbol *sym = symbol_table_lookup_symbol(gen->symbol_table, stmt->params[i].name);
                if (sym) sym->kind = SYMBOL_LOCAL;
            }
            else if (param_type->kind == TYPE_STRUCT && stmt->params[i].mem_qualifier != MEM_AS_REF)
            {
                /* Clone string handle fields of value struct parameters (not 'as ref' pointers).
                 * Array fields are NOT cloned to preserve pass-by-reference mutation semantics.
                 * Use clone_any to search current arena first, then parent arenas. */
                int field_count = param_type->as.struct_type.field_count;
                char *param_name = sn_mangle_name(gen->arena, get_var_name(gen->arena, stmt->params[i].name));
                for (int f = 0; f < field_count; f++)
                {
                    StructField *field = &param_type->as.struct_type.fields[f];
                    if (field->type && field->type->kind == TYPE_STRING)
                    {
                        const char *c_field_name = field->c_alias != NULL ? field->c_alias : sn_mangle_name(gen->arena, field->name);
                        indented_fprintf(gen, 1, "%s.%s = rt_managed_clone_any(__local_arena__, __caller_arena__, %s.%s);\n",
                                         param_name, c_field_name, param_name, c_field_name);
                    }
                }
            }
        }
    }

    // Add _return_value only if needed (non-void or main).
    if (has_return_value)
    {
        const char *default_val = is_main ? "0" : get_default_value(gen->current_return_type);
        indented_fprintf(gen, 1, "%s _return_value = %s;\n", ret_c, default_val);
    }

    // Initialize args array for main if it has parameters
    if (main_has_args)
    {
        char *param_name = sn_mangle_name(gen->arena, get_var_name(gen->arena, stmt->params[0].name));
        indented_fprintf(gen, 1, "RtHandle %s = rt_args_create_h(%s, argc, argv);\n",
                         param_name, gen->current_arena_var);
        /* Update the symbol kind from SYMBOL_PARAM to SYMBOL_LOCAL so that the
         * pin logic in code_gen_variable_expression recognizes it as a handle. */
        Symbol *args_sym = symbol_table_lookup_symbol(gen->symbol_table, stmt->params[0].name);
        if (args_sym) args_sym->kind = SYMBOL_LOCAL;
    }

    // Clone 'as val' array parameters to ensure copy semantics
    for (int i = 0; i < stmt->param_count; i++)
    {
        if (stmt->params[i].mem_qualifier == MEM_AS_VAL)
        {
            Type *param_type = stmt->params[i].type;
            if (param_type->kind == TYPE_ARRAY)
            {
                char *param_name = sn_mangle_name(gen->arena, get_var_name(gen->arena, stmt->params[i].name));
                Type *elem_type = param_type->as.array.element_type;
                const char *suffix = code_gen_type_suffix(elem_type);
                if (gen->current_arena_var != NULL)
                {
                    /* Handle mode: pin the source handle from caller arena, clone into local arena.
                     * rt_managed_pin_array walks the parent chain to find the handle. */
                    const char *elem_c = get_c_type(gen->arena, elem_type);
                    indented_fprintf(gen, 1, "%s = rt_array_clone_%s_h(%s, RT_HANDLE_NULL, ((%s *)rt_managed_pin_array(%s, %s)));\n",
                                     param_name, suffix, ARENA_VAR(gen), elem_c, ARENA_VAR(gen), param_name);
                    /* Update symbol to local so it's treated as a local variable */
                    Symbol *sym = symbol_table_lookup_symbol(gen->symbol_table, stmt->params[i].name);
                    if (sym)
                    {
                        sym->kind = SYMBOL_LOCAL;
                    }
                }
                else
                {
                    indented_fprintf(gen, 1, "%s = rt_array_clone_%s(%s, %s);\n",
                                     param_name, suffix, ARENA_VAR(gen), param_name);
                }
            }
            else if (param_type->kind == TYPE_STRING)
            {
                char *param_name = sn_mangle_name(gen->arena, get_var_name(gen->arena, stmt->params[i].name));
                indented_fprintf(gen, 1, "%s = rt_to_string_string(%s, %s);\n",
                                 param_name, ARENA_VAR(gen), param_name);
                /* Update symbol to local so it's treated as a local variable */
                Symbol *sym = symbol_table_lookup_symbol(gen->symbol_table, stmt->params[i].name);
                if (sym)
                {
                    sym->kind = SYMBOL_LOCAL;
                }
            }
        }
    }

    // Check if function has marked tail calls for optimization
    bool has_tail_calls = function_has_marked_tail_calls(stmt);

    // Set up tail call optimization state
    bool old_in_tail_call_function = gen->in_tail_call_function;
    FunctionStmt *old_tail_call_fn = gen->tail_call_fn;

    if (has_tail_calls)
    {
        gen->in_tail_call_function = true;
        gen->tail_call_fn = stmt;
        // Wrap function body in a loop for tail call optimization
        indented_fprintf(gen, 1, "while (1) { /* tail call loop */\n");
    }

    bool has_return = false;
    if (stmt->body_count > 0 && stmt->body[stmt->body_count - 1]->type == STMT_RETURN)
    {
        has_return = true;
    }

    // If the function returns a closure type, set the flag so all closures
    // created in this function are allocated in the caller's arena.
    // This handles the pattern where a closure is stored in a variable
    // before being returned.
    bool old_allocate_closure_in_caller_arena = gen->allocate_closure_in_caller_arena;
    if (!is_main && stmt->return_type && stmt->return_type->kind == TYPE_FUNCTION)
    {
        gen->allocate_closure_in_caller_arena = true;
    }

    int body_indent = has_tail_calls ? 2 : 1;
    for (int i = 0; i < stmt->body_count; i++)
    {
        code_gen_statement(gen, stmt->body[i], body_indent);
    }

    // Restore the flag
    gen->allocate_closure_in_caller_arena = old_allocate_closure_in_caller_arena;
    if (!has_return)
    {
        indented_fprintf(gen, body_indent, "goto %s_return;\n", gen->current_function);
    }

    if (has_tail_calls)
    {
        indented_fprintf(gen, 1, "} /* end tail call loop */\n");
    }

    // Restore tail call state
    gen->in_tail_call_function = old_in_tail_call_function;
    gen->tail_call_fn = old_tail_call_fn;

    indented_fprintf(gen, 0, "%s_return:\n", gen->current_function);
    code_gen_free_locals(gen, gen->symbol_table->current, true, 1);

    // For non-main, non-shared functions with heap return types, promote the return
    // value to the caller's arena before destroying the local arena.
    bool needs_promotion = !is_main && !is_shared && has_return_value && stmt->return_type;
    if (needs_promotion)
    {
        TypeKind kind = stmt->return_type->kind;
        if (kind == TYPE_STRING)
        {
            // Promote string handle from local arena to caller's arena
            indented_fprintf(gen, 1, "_return_value = rt_managed_promote(__caller_arena__, __local_arena__, _return_value);\n");
        }
        else if (kind == TYPE_ARRAY)
        {
            // Check if this is a string array or 2D+ array - needs deep promotion
            Type *elem_type = stmt->return_type->as.array.element_type;
            if (elem_type && elem_type->kind == TYPE_STRING)
            {
                // String arrays need deep promotion: promote array AND each string element
                indented_fprintf(gen, 1, "_return_value = rt_managed_promote_array_string(__caller_arena__, __local_arena__, _return_value);\n");
            }
            else if (elem_type && elem_type->kind == TYPE_ARRAY)
            {
                // 2D/3D arrays need deep promotion
                Type *inner_elem = elem_type->as.array.element_type;
                if (inner_elem && inner_elem->kind == TYPE_STRING)
                {
                    // str[][] needs extra deep promotion for string elements
                    indented_fprintf(gen, 1, "_return_value = rt_managed_promote_array2_string(__caller_arena__, __local_arena__, _return_value);\n");
                }
                else if (inner_elem && inner_elem->kind == TYPE_ARRAY)
                {
                    // 3D arrays: check innermost for string type
                    Type *innermost = inner_elem->as.array.element_type;
                    if (innermost && innermost->kind == TYPE_STRING)
                    {
                        // str[][][] needs three levels of string promotion
                        indented_fprintf(gen, 1, "_return_value = rt_managed_promote_array3_string(__caller_arena__, __local_arena__, _return_value);\n");
                    }
                    else
                    {
                        // Other 3D arrays: promote all three levels of handles
                        indented_fprintf(gen, 1, "_return_value = rt_managed_promote_array_handle_3d(__caller_arena__, __local_arena__, _return_value);\n");
                    }
                }
                else
                {
                    // 2D arrays: promote outer array AND each inner array handle
                    indented_fprintf(gen, 1, "_return_value = rt_managed_promote_array_handle(__caller_arena__, __local_arena__, _return_value);\n");
                }
            }
            else
            {
                // Non-string, non-nested arrays: shallow promote is sufficient
                indented_fprintf(gen, 1, "_return_value = rt_managed_promote(__caller_arena__, __local_arena__, _return_value);\n");
            }
        }
        else if (kind == TYPE_STRUCT)
        {
            // Struct is returned by value — only handle fields need promotion
            // to the caller's arena so they survive local arena destruction.
            int field_count = stmt->return_type->as.struct_type.field_count;
            for (int i = 0; i < field_count; i++)
            {
                StructField *field = &stmt->return_type->as.struct_type.fields[i];
                if (field->type == NULL) continue;
                const char *c_field_name = field->c_alias != NULL ? field->c_alias : sn_mangle_name(gen->arena, field->name);
                if (field->type->kind == TYPE_STRING)
                {
                    indented_fprintf(gen, 1, "_return_value.%s = rt_managed_promote(__caller_arena__, __local_arena__, _return_value.%s);\n",
                                     c_field_name, c_field_name);
                }
                else if (field->type->kind == TYPE_ARRAY)
                {
                    // Check if this is a string array or 2D+ array field - needs deep promotion
                    Type *elem_type = field->type->as.array.element_type;
                    if (elem_type && elem_type->kind == TYPE_STRING)
                    {
                        indented_fprintf(gen, 1, "_return_value.%s = rt_managed_promote_array_string(__caller_arena__, __local_arena__, _return_value.%s);\n",
                                         c_field_name, c_field_name);
                    }
                    else if (elem_type && elem_type->kind == TYPE_ARRAY)
                    {
                        indented_fprintf(gen, 1, "_return_value.%s = rt_managed_promote_array_handle(__caller_arena__, __local_arena__, _return_value.%s);\n",
                                         c_field_name, c_field_name);
                    }
                    else if (elem_type && elem_type->kind == TYPE_STRUCT && struct_has_handle_fields(elem_type))
                    {
                        // Struct[] where struct has handle fields - need to promote handles in each element
                        const char *struct_c_name = elem_type->as.struct_type.c_alias != NULL
                            ? elem_type->as.struct_type.c_alias
                            : sn_mangle_name(gen->arena, elem_type->as.struct_type.name);
                        indented_fprintf(gen, 1, "{ /* Promote handles in struct array elements */\n");
                        indented_fprintf(gen, 1, "    %s *__parr__ = ((%s *)rt_managed_pin_array(__local_arena__, _return_value.%s));\n",
                                         struct_c_name, struct_c_name, c_field_name);
                        indented_fprintf(gen, 1, "    long __plen__ = rt_array_length(__parr__);\n");
                        indented_fprintf(gen, 1, "    for (long __pi__ = 0; __pi__ < __plen__; __pi__++) {\n");
                        // Generate promotion for each handle field in the struct element
                        int struct_field_count = elem_type->as.struct_type.field_count;
                        for (int fi = 0; fi < struct_field_count; fi++)
                        {
                            StructField *sf = &elem_type->as.struct_type.fields[fi];
                            if (sf->type == NULL) continue;
                            const char *sf_c_name = sf->c_alias != NULL ? sf->c_alias
                                : sn_mangle_name(gen->arena, sf->name);
                            if (sf->type->kind == TYPE_STRING)
                            {
                                indented_fprintf(gen, 1, "        __parr__[__pi__].%s = rt_managed_promote(__caller_arena__, __local_arena__, __parr__[__pi__].%s);\n",
                                                 sf_c_name, sf_c_name);
                            }
                            else if (sf->type->kind == TYPE_ARRAY)
                            {
                                Type *sf_elem = sf->type->as.array.element_type;
                                if (sf_elem && sf_elem->kind == TYPE_STRING)
                                {
                                    indented_fprintf(gen, 1, "        __parr__[__pi__].%s = rt_managed_promote_array_string(__caller_arena__, __local_arena__, __parr__[__pi__].%s);\n",
                                                     sf_c_name, sf_c_name);
                                }
                                else
                                {
                                    indented_fprintf(gen, 1, "        __parr__[__pi__].%s = rt_managed_promote(__caller_arena__, __local_arena__, __parr__[__pi__].%s);\n",
                                                     sf_c_name, sf_c_name);
                                }
                            }
                        }
                        indented_fprintf(gen, 1, "    }\n");
                        indented_fprintf(gen, 1, "    _return_value.%s = rt_managed_promote(__caller_arena__, __local_arena__, _return_value.%s);\n",
                                         c_field_name, c_field_name);
                        indented_fprintf(gen, 1, "}\n");
                    }
                    else
                    {
                        indented_fprintf(gen, 1, "_return_value.%s = rt_managed_promote(__caller_arena__, __local_arena__, _return_value.%s);\n",
                                         c_field_name, c_field_name);
                    }
                }
            }
        }
        else if (kind == TYPE_FUNCTION)
        {
            // Closures - copy closure struct to caller's arena so it survives
            // local arena destruction. Uses actual size to include captures.
            indented_fprintf(gen, 1, "{ __Closure__ *__src_cl__ = _return_value;\n");
            indented_fprintf(gen, 1, "  _return_value = (__Closure__ *)rt_arena_alloc(__caller_arena__, __src_cl__->size);\n");
            indented_fprintf(gen, 1, "  memcpy(_return_value, __src_cl__, __src_cl__->size);\n");
            indented_fprintf(gen, 1, "  _return_value->arena = __caller_arena__; }\n");
        }
        else if (kind == TYPE_ANY)
        {
            // Any values may contain heap-allocated data (strings, arrays).
            // Use runtime promotion to copy to caller's arena.
            indented_fprintf(gen, 1, "_return_value = rt_any_promote(__caller_arena__, _return_value);\n");
        }
    }

    // Destroy local arena for main and non-shared functions
    // (shared functions just alias the caller's arena, so don't destroy)
    if (is_main)
    {
        indented_fprintf(gen, 1, "rt_managed_arena_destroy(__local_arena__);\n");
    }
    else if (!is_shared)
    {
        indented_fprintf(gen, 1, "rt_managed_arena_destroy_child(__local_arena__);\n");
    }

    // Return _return_value only if needed; otherwise, plain return.
    if (has_return_value)
    {
        indented_fprintf(gen, 1, "return _return_value;\n");
    }
    else
    {
        indented_fprintf(gen, 1, "return;\n");
    }
    indented_fprintf(gen, 0, "}\n\n");

    // Exit arena scope in symbol table (all functions have arena context now)
    symbol_table_exit_arena(gen->symbol_table);

    symbol_table_pop_scope(gen->symbol_table);

    // Clear captured primitives list
    code_gen_clear_captured_primitives(gen);

    gen->current_function = old_function;
    gen->current_return_type = old_return_type;
    gen->current_func_modifier = old_func_modifier;
    gen->in_private_context = old_in_private_context;
    gen->in_shared_context = old_in_shared_context;
    gen->current_arena_var = old_arena_var;
    gen->arena_depth = old_arena_depth;
}

void code_gen_return_statement(CodeGen *gen, ReturnStmt *stmt, int indent)
{
    DEBUG_VERBOSE("Entering code_gen_return_statement");
    /* Check if returning from a void function/lambda */
    int is_void_return = (gen->current_return_type && gen->current_return_type->kind == TYPE_VOID);

    /* Check if this return contains a tail call that should be optimized */
    if (gen->in_tail_call_function && stmt->value &&
        stmt->value->type == EXPR_CALL && stmt->value->as.call.is_tail_call)
    {
        CallExpr *call = &stmt->value->as.call;
        FunctionStmt *fn = gen->tail_call_fn;

        /* Generate parameter assignments */
        /* For multiple parameters, we need temp variables to handle cases like
           return f(b, a) when the current params are (a, b) */
        if (fn->param_count > 1)
        {
            /* First, generate temp variables for all new argument values */
            for (int i = 0; i < call->arg_count; i++)
            {
                const char *param_type_c = get_c_type(gen->arena, fn->params[i].type);
                char *arg_str = code_gen_expression(gen, call->arguments[i]);
                indented_fprintf(gen, indent, "%s __tail_arg_%d__ = %s;\n",
                                 param_type_c, i, arg_str);
            }
            /* Then, assign temps to actual parameters */
            for (int i = 0; i < call->arg_count; i++)
            {
                char *param_name = sn_mangle_name(gen->arena, get_var_name(gen->arena, fn->params[i].name));
                indented_fprintf(gen, indent, "%s = __tail_arg_%d__;\n",
                                 param_name, i);
            }
        }
        else if (fn->param_count == 1)
        {
            /* Single parameter - direct assignment is safe */
            char *param_name = sn_mangle_name(gen->arena, get_var_name(gen->arena, fn->params[0].name));
            char *arg_str = code_gen_expression(gen, call->arguments[0]);
            indented_fprintf(gen, indent, "%s = %s;\n", param_name, arg_str);
        }
        /* Continue the tail call loop */
        indented_fprintf(gen, indent, "continue;\n");
        return;
    }

    /* Normal return */
    if (stmt->value && !is_void_return)
    {
        /* If returning a lambda expression directly, allocate it in the caller's
         * arena so captured variables survive the function's arena destruction. */
        bool is_lambda_return = (stmt->value->type == EXPR_LAMBDA);
        if (is_lambda_return)
        {
            gen->allocate_closure_in_caller_arena = true;
        }

        /* If the function returns a handle type (string/array), the return expression
         * must produce an RtHandle value (expr_as_handle = true). */
        bool prev_as_handle = gen->expr_as_handle;
        if (gen->current_return_type != NULL && is_handle_type(gen->current_return_type) &&
            gen->current_arena_var != NULL)
        {
            gen->expr_as_handle = true;
        }

        char *value_str = code_gen_expression(gen, stmt->value);

        gen->expr_as_handle = prev_as_handle;

        if (is_lambda_return)
        {
            gen->allocate_closure_in_caller_arena = false;
        }

        /* Handle returning 'self' pointer as struct value (builder/fluent pattern).
         * In struct instance methods, 'self' is a pointer to the struct, but the
         * declared return type may be the struct itself (for fluent APIs).
         * When returning 'self' with a struct return type, dereference the pointer. */
        if (stmt->value->type == EXPR_VARIABLE &&
            gen->current_return_type != NULL &&
            gen->current_return_type->kind == TYPE_STRUCT)
        {
            /* Check if the variable is 'self' */
            Token var_name = stmt->value->as.variable.name;
            if (var_name.length == 4 && strncmp(var_name.start, "self", 4) == 0)
            {
                /* Dereference the pointer to get the struct value */
                value_str = arena_sprintf(gen->arena, "(*%s)", value_str);
            }
        }

        /* Handle boxing when function returns 'any' but expression is a concrete type */
        if (gen->current_return_type != NULL && gen->current_return_type->kind == TYPE_ANY &&
            stmt->value->expr_type != NULL && stmt->value->expr_type->kind != TYPE_ANY)
        {
            value_str = code_gen_box_value(gen, value_str, stmt->value->expr_type);
        }

        indented_fprintf(gen, indent, "_return_value = %s;\n", value_str);
    }

    /* Clean up all active private block arenas before returning (innermost first).
     * The function-level arena is NOT on this stack - it's destroyed at the return label.
     * This stack only contains private block arenas that need explicit cleanup. */
    for (int i = gen->arena_stack_depth - 1; i >= 0; i--)
    {
        if (gen->arena_stack[i] != NULL)
        {
            indented_fprintf(gen, indent, "rt_managed_arena_destroy_child(%s);\n", gen->arena_stack[i]);
        }
    }

    indented_fprintf(gen, indent, "goto %s_return;\n", gen->current_function);
}

void code_gen_if_statement(CodeGen *gen, IfStmt *stmt, int indent)
{
    DEBUG_VERBOSE("Entering code_gen_if_statement");
    char *cond_str = code_gen_expression(gen, stmt->condition);
    indented_fprintf(gen, indent, "if (%s) {\n", cond_str);
    code_gen_statement(gen, stmt->then_branch, indent + 1);
    indented_fprintf(gen, indent, "}\n");
    if (stmt->else_branch)
    {
        indented_fprintf(gen, indent, "else {\n");
        code_gen_statement(gen, stmt->else_branch, indent + 1);
        indented_fprintf(gen, indent, "}\n");
    }
}

void code_gen_statement(CodeGen *gen, Stmt *stmt, int indent)
{
    DEBUG_VERBOSE("Entering code_gen_statement");

    /* Emit any attached comments (// comments are preserved) */
    if (stmt->comment_count > 0 && stmt->comments != NULL)
    {
        for (int i = 0; i < stmt->comment_count; i++)
        {
            if (stmt->comments[i] != NULL)
            {
                indented_fprintf(gen, indent, "//%s\n", stmt->comments[i]);
            }
        }
    }

    gen->current_indent = indent;
    switch (stmt->type)
    {
    case STMT_EXPR:
        code_gen_expression_statement(gen, &stmt->as.expression, indent);
        break;
    case STMT_VAR_DECL:
        code_gen_var_declaration(gen, &stmt->as.var_decl, indent);
        break;
    case STMT_FUNCTION:
        code_gen_function(gen, &stmt->as.function);
        break;
    case STMT_RETURN:
        code_gen_return_statement(gen, &stmt->as.return_stmt, indent);
        break;
    case STMT_BLOCK:
        code_gen_block(gen, &stmt->as.block, indent);
        break;
    case STMT_IF:
        code_gen_if_statement(gen, &stmt->as.if_stmt, indent);
        break;
    case STMT_WHILE:
        code_gen_while_statement(gen, &stmt->as.while_stmt, indent);
        break;
    case STMT_FOR:
        code_gen_for_statement(gen, &stmt->as.for_stmt, indent);
        break;
    case STMT_FOR_EACH:
        code_gen_for_each_statement(gen, &stmt->as.for_each_stmt, indent);
        break;
    case STMT_BREAK:
        indented_fprintf(gen, indent, "break;\n");
        break;
    case STMT_CONTINUE:
        /* In for loops, continue needs to jump to the continue label (before increment) */
        if (gen->for_continue_label)
        {
            indented_fprintf(gen, indent, "goto %s;\n", gen->for_continue_label);
        }
        /* In while/for-each loops, regular continue works fine */
        else
        {
            indented_fprintf(gen, indent, "continue;\n");
        }
        break;
    case STMT_IMPORT:
        /* For namespaced imports, emit the imported module's function definitions.
         * Non-namespaced imports have their statements merged by the parser,
         * so they don't need special handling here.
         *
         * If the module was ALSO imported directly (without namespace) or already
         * imported by another namespace, we skip function definitions to avoid
         * duplicates. However, we still emit variable declarations with unique
         * namespace prefixes so that each import alias has its own variable instance. */
        if (stmt->as.import.namespace != NULL && stmt->as.import.imported_stmts != NULL)
        {
            /* Always emit functions for each namespace alias.
             * Even when the same module is imported with multiple aliases, each alias
             * needs its own function copies because they access alias-specific instance
             * variables (non-static module-level variables). */
            bool emit_functions = true;
            /* Before generating code for the imported module's functions, add the
             * namespace's function symbols to a temporary scope. This allows intra-module
             * function calls (e.g., errorMessage() calling zlibOk()) to find their
             * callees and correctly determine that they need arena arguments. */
            Token ns_name = *stmt->as.import.namespace;
            Symbol *ns_symbol = NULL;

            /* If we're inside a parent namespace, look up the nested namespace.
             * Otherwise, look in global scope. */
            if (gen->current_namespace_prefix != NULL)
            {
                /* Build parent namespace token from the current prefix */
                Token parent_ns_token;
                parent_ns_token.start = gen->current_namespace_prefix;
                parent_ns_token.length = strlen(gen->current_namespace_prefix);
                parent_ns_token.type = TOKEN_IDENTIFIER;
                parent_ns_token.line = 0;
                parent_ns_token.filename = NULL;

                /* Look up as nested namespace under the current parent */
                ns_symbol = symbol_table_lookup_nested_namespace(gen->symbol_table, parent_ns_token, ns_name);
            }

            /* Fall back to global lookup if nested lookup failed or no parent */
            if (ns_symbol == NULL)
            {
                ns_symbol = symbol_table_lookup_symbol(gen->symbol_table, ns_name);
            }

            if (ns_symbol != NULL && ns_symbol->is_namespace)
            {
                /* Push a new scope and add all namespace symbols (including from nested namespaces) */
                symbol_table_push_scope(gen->symbol_table);
                add_namespace_symbols_to_scope(gen, ns_symbol);
            }

            /* Set namespace prefix for variable and function name generation.
             * This ensures symbols from different modules get unique C names. */
            const char *old_namespace_prefix = gen->current_namespace_prefix;
            const char *old_canonical_module = gen->current_canonical_module;
            char ns_prefix[256];
            int ns_len = ns_name.length < 255 ? ns_name.length : 255;
            memcpy(ns_prefix, ns_name.start, ns_len);
            ns_prefix[ns_len] = '\0';
            gen->current_namespace_prefix = arena_strdup(gen->arena, ns_prefix);

            /* Set canonical module name for static variable sharing.
             * All aliases of the same module share static variables under this name.
             * If the symbol doesn't have canonical_module_name set (e.g., for deeply
             * nested imports), extract it from the import statement's module path. */
            if (ns_symbol != NULL && ns_symbol->canonical_module_name != NULL)
            {
                gen->current_canonical_module = ns_symbol->canonical_module_name;
            }
            else
            {
                /* Extract canonical module name from the import path */
                char mod_path[512];
                int mod_len = stmt->as.import.module_name.length < 511 ? stmt->as.import.module_name.length : 511;
                memcpy(mod_path, stmt->as.import.module_name.start, mod_len);
                mod_path[mod_len] = '\0';

                /* Find the last path separator and extract the base name */
                const char *base_name = mod_path;
                for (int k = mod_len - 1; k >= 0; k--)
                {
                    if (mod_path[k] == '/' || mod_path[k] == '\\')
                    {
                        base_name = &mod_path[k + 1];
                        break;
                    }
                }
                /* Remove .sn extension if present */
                char *canonical = arena_strdup(gen->arena, base_name);
                int can_len = strlen(canonical);
                if (can_len > 3 && strcmp(canonical + can_len - 3, ".sn") == 0)
                {
                    canonical[can_len - 3] = '\0';
                }
                gen->current_canonical_module = canonical;
            }

            /* Emit forward declarations for all functions in the imported module.
             * This ensures functions can call each other regardless of definition order.
             * Uses recursive helper to handle nested namespace imports. */
            if (emit_functions)
            {
                emit_import_forward_declarations_recursive(gen, stmt->as.import.imported_stmts,
                                                           stmt->as.import.imported_count,
                                                           gen->current_namespace_prefix);
            }

            for (int i = 0; i < stmt->as.import.imported_count; i++)
            {
                Stmt *imported_stmt = stmt->as.import.imported_stmts[i];
                if (imported_stmt == NULL)
                    continue;

                /* If we're skipping function emission, only emit variable declarations.
                 * Variables need separate instances per namespace alias for unique state. */
                if (!emit_functions && imported_stmt->type == STMT_FUNCTION)
                {
                    continue;  /* Skip function - already emitted by first import */
                }

                code_gen_statement(gen, imported_stmt, indent);
            }

            /* Emit struct method implementations for imported structs.
             * Unlike functions, struct methods are not emitted by code_gen_statement. */
            if (emit_functions)
            {
                for (int i = 0; i < stmt->as.import.imported_count; i++)
                {
                    Stmt *imported_stmt = stmt->as.import.imported_stmts[i];
                    if (imported_stmt != NULL && imported_stmt->type == STMT_STRUCT_DECL)
                    {
                        code_gen_struct_methods(gen, &imported_stmt->as.struct_decl, indent);
                    }
                }
            }

            /* Restore previous namespace prefix and canonical module */
            gen->current_namespace_prefix = old_namespace_prefix;
            gen->current_canonical_module = old_canonical_module;

            /* Pop the temporary scope if we pushed one */
            if (ns_symbol != NULL && ns_symbol->is_namespace)
            {
                symbol_table_pop_scope(gen->symbol_table);
            }
        }
        break;
    case STMT_PRAGMA:
        /* Pragmas are collected in code_gen_module and emitted at the top of the file.
         * No action needed here during statement code generation. */
        break;
    case STMT_TYPE_DECL:
        /* Type declarations are handled at the module level where forward declarations
         * are emitted. No code generation is needed for the statement itself. */
        break;
    case STMT_STRUCT_DECL:
        /* Struct declarations are handled at the module level where typedef
         * declarations are emitted. No code generation is needed for the statement itself. */
        break;

    case STMT_LOCK:
    {
        /* Lock block: lock(sync_var) => { ... }
         * Generates:
         *   rt_sync_lock(&sync_var);
         *   { body }
         *   rt_sync_unlock(&sync_var);
         */
        LockStmt *lock_stmt = &stmt->as.lock_stmt;

        /* Generate the lock expression (should be a variable name) */
        char *lock_var = code_gen_expression(gen, lock_stmt->lock_expr);

        /* Generate lock acquisition */
        indented_fprintf(gen, indent, "rt_sync_lock(&%s);\n", lock_var);

        /* Generate lock body in a block */
        indented_fprintf(gen, indent, "{\n");
        code_gen_statement(gen, lock_stmt->body, indent + 1);
        indented_fprintf(gen, indent, "}\n");

        /* Generate lock release */
        indented_fprintf(gen, indent, "rt_sync_unlock(&%s);\n", lock_var);
        break;
    }
    }
}
