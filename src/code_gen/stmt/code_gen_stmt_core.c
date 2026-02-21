/*
 * code_gen_stmt_core.c - Core statement code generation
 *
 * Contains the main statement dispatcher and small helper functions.
 * Large functions have been split into separate files:
 *   - code_gen_stmt_import.c   - Import helpers
 *   - code_gen_stmt_thread.c   - Thread sync statement
 *   - code_gen_stmt_struct.c   - Struct method generation
 *   - code_gen_stmt_var.c      - Variable declaration
 *   - code_gen_stmt_func.c     - Function generation
 *   - code_gen_stmt_return.c   - Return statement
 */

#include "code_gen/stmt/code_gen_stmt.h"
#include "code_gen/stmt/code_gen_stmt_loop.h"
#include "code_gen/stmt/code_gen_stmt_capture.h"
#include "code_gen/stmt/code_gen_stmt_import.h"
#include "code_gen/stmt/code_gen_stmt_thread.h"
#include "code_gen/expr/code_gen_expr.h"
#include "code_gen/util/code_gen_util.h"
#include "debug.h"
#include "symbol_table.h"
#include "symbol_table/symbol_table_core.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

void code_gen_expression_statement(CodeGen *gen, ExprStmt *stmt, int indent)
{
    DEBUG_VERBOSE("Entering code_gen_expression_statement");

    /* Special handling for thread sync statements */
    if (stmt->expression->type == EXPR_THREAD_SYNC)
    {
        code_gen_thread_sync_statement(gen, stmt->expression, indent);
        return;
    }

    /* Fire-and-forget thread spawn: result is discarded, wrapper should do cleanup */
    bool is_fire_and_forget_spawn = (stmt->expression->type == EXPR_THREAD_SPAWN);
    if (is_fire_and_forget_spawn)
    {
        gen->spawn_is_fire_and_forget = true;
    }

    char *expr_str = code_gen_expression(gen, stmt->expression);

    /* Reset the flag after generating the expression */
    if (is_fire_and_forget_spawn)
    {
        gen->spawn_is_fire_and_forget = false;
    }
    DEBUG_VERBOSE("Expression statement type: %p", (void*)stmt->expression->expr_type);

    if (stmt->expression->expr_type != NULL &&
        stmt->expression->expr_type->kind == TYPE_STRING && expression_produces_temp(stmt->expression))
    {
        /* Skip freeing in arena context - arena handles cleanup */
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
        indented_fprintf(gen, indent, "%s;\n", expr_str);
    }
    else
    {
        indented_fprintf(gen, indent, "%s;\n", expr_str);
    }

    /* Flush any arena temps accumulated during this expression statement */
    if (gen->current_arena_var != NULL && gen->arena_temp_count > 0)
    {
        bool in_method = (gen->function_arena_var != NULL &&
                          strcmp(gen->function_arena_var, "__caller_arena__") == 0);
        if (gen->loop_scope_depth > 0 || in_method)
        {
            /* In loops: flush to prevent accumulation across iterations.
             * In struct methods: flush always — no arena condemn to clean up. */
            code_gen_flush_arena_temps(gen, indent);
        }
        else
        {
            /* Outside loops in regular functions: don't free (arena condemn
             * handles cleanup), but reset counter so stale temps don't
             * leak into loops. */
            gen->arena_temp_count = 0;
        }
    }
}

void code_gen_free_locals(CodeGen *gen, Scope *scope, bool is_function, int indent)
{
    DEBUG_VERBOSE("Entering code_gen_free_locals");

    bool in_arena_context = (gen->current_arena_var != NULL);

    Symbol *sym = scope->symbols;
    while (sym)
    {
        if (sym->type && sym->kind == SYMBOL_LOCAL)
        {
            char *var_name = sn_mangle_name(gen->arena, get_var_name(gen->arena, sym->name));

            /* In arena context, handle struct types with handle fields, arrays,
             * and string handles. All need explicit cleanup at scope exit to
             * prevent handle accumulation in loops. */
            if (in_arena_context)
            {
                if (sym->type->kind == TYPE_STRUCT && struct_has_handle_fields(sym->type))
                {
                    /* Skip native structs - they manage their own memory */
                    if (sym->type->as.struct_type.is_native)
                    {
                        /* Built-in type - skip cleanup, runtime handles it */
                    }
                    else
                    {
                        /* Call the generated free callback to mark handle fields as dead.
                         * Pass the local arena as owner so only handles owned by this arena
                         * are freed - handles borrowed from other arenas are left alone. */
                        const char *struct_name = sym->type->as.struct_type.name;
                        if (struct_name != NULL)
                        {
                            indented_fprintf(gen, indent, "__free_%s_inline__(&%s, %s);\n",
                                struct_name, var_name, gen->current_arena_var);
                        }
                        /* Condemn the struct's child arena so the GC can reclaim
                         * temporaries allocated during method calls on this struct.
                         * Guard: if the function returns a struct, skip condemn when
                         * this variable's arena IS the return value's arena (ownership
                         * transfers to the caller). */
                        if (!sym->type->as.struct_type.is_packed)
                        {
                            bool returns_struct = (gen->current_return_type != NULL &&
                                                   gen->current_return_type->kind == TYPE_STRUCT);
                            if (returns_struct)
                            {
                                indented_fprintf(gen, indent, "if (%s.__arena__ && %s.__arena__ != _return_value.__arena__) rt_arena_v2_condemn(%s.__arena__);\n",
                                    var_name, var_name, var_name);
                            }
                            else
                            {
                                indented_fprintf(gen, indent, "if (%s.__arena__) rt_arena_v2_condemn(%s.__arena__);\n",
                                    var_name, var_name);
                            }
                        }
                    }
                }
                else if (sym->type->kind == TYPE_STRUCT
                         && !sym->type->as.struct_type.is_native
                         && !sym->type->as.struct_type.is_packed)
                {
                    /* Struct without handle fields but still has __arena__.
                     * Condemn the arena so GC can reclaim method temporaries. */
                    bool returns_struct = (gen->current_return_type != NULL &&
                                           gen->current_return_type->kind == TYPE_STRUCT);
                    if (returns_struct)
                    {
                        indented_fprintf(gen, indent, "if (%s.__arena__ && %s.__arena__ != _return_value.__arena__) rt_arena_v2_condemn(%s.__arena__);\n",
                            var_name, var_name, var_name);
                    }
                    else
                    {
                        indented_fprintf(gen, indent, "if (%s.__arena__) rt_arena_v2_condemn(%s.__arena__);\n",
                            var_name, var_name);
                    }
                }
                else if (sym->type->kind == TYPE_ARRAY)
                {
                    /* Free array handle so its free callback runs.
                     * This is needed for arrays of structs (to free struct handle fields)
                     * and arrays of strings (to free string handles). */
                    Type *elem_type = sym->type->as.array.element_type;
                    bool needs_cleanup = false;
                    if (elem_type != NULL)
                    {
                        if (elem_type->kind == TYPE_STRING)
                            needs_cleanup = true;
                        else if (elem_type->kind == TYPE_STRUCT && struct_has_handle_fields(elem_type))
                            needs_cleanup = true;
                        else if (elem_type->kind == TYPE_ARRAY)
                            needs_cleanup = true; /* nested arrays */
                    }
                    if (needs_cleanup)
                    {
                        /* In struct methods, also free individual string elements
                         * since there's no arena condemn to clean them up. */
                        bool in_method = (gen->function_arena_var != NULL &&
                                          strcmp(gen->function_arena_var, "__caller_arena__") == 0);
                        if (elem_type->kind == TYPE_STRING && in_method && is_function)
                        {
                            indented_fprintf(gen, indent, "if (%s) { for (long long __fi = 0; __fi < rt_array_length_v2(%s); __fi++) {\n", var_name, var_name);
                            indented_fprintf(gen, indent + 1, "RtHandleV2 *__fe = rt_array_get_handle_v2(%s, __fi);\n", var_name);
                            if (gen->current_return_type && gen->current_return_type->kind == TYPE_STRING)
                            {
                                indented_fprintf(gen, indent + 1, "if (__fe != _return_value) rt_arena_v2_free(__fe);\n");
                            }
                            else
                            {
                                indented_fprintf(gen, indent + 1, "rt_arena_v2_free(__fe);\n");
                            }
                            indented_fprintf(gen, indent, "} }\n");
                        }
                        indented_fprintf(gen, indent, "rt_arena_v2_free(%s);\n", var_name);
                    }
                }
                else if (sym->type->kind == TYPE_STRING && !is_function)
                {
                    /* Free string handle at block scope exit to prevent handle
                     * accumulation in loops. Each iteration creates a new handle
                     * for the variable — without this, old handles leak.
                     * Skip at function scope: rt_arena_v2_condemn handles it.
                     * Note: GC's rescue mechanism (ref count check) protects handles
                     * that are still referenced by live data structures. */
                    indented_fprintf(gen, indent, "rt_arena_v2_free(%s);\n", var_name);
                }
                else if (sym->type->kind == TYPE_STRING && is_function
                         && gen->function_arena_var != NULL
                         && strcmp(gen->function_arena_var, "__caller_arena__") == 0)
                {
                    /* Struct method: no local arena condemn, must free explicitly.
                     * Guard against freeing the return value handle. */
                    if (gen->current_return_type && gen->current_return_type->kind == TYPE_STRING)
                    {
                        indented_fprintf(gen, indent, "if (%s != _return_value) rt_arena_v2_free(%s);\n",
                                         var_name, var_name);
                    }
                    else if (gen->current_return_type && gen->current_return_type->kind == TYPE_STRUCT
                             && struct_has_handle_fields(gen->current_return_type))
                    {
                        /* Return type is a struct with handle fields — the string local
                         * might be stored in a return value field. Guard against each
                         * string field of the return struct. */
                        int field_count = gen->current_return_type->as.struct_type.field_count;
                        char *guard = arena_strdup(gen->arena, "");
                        for (int fi = 0; fi < field_count; fi++)
                        {
                            StructField *field = &gen->current_return_type->as.struct_type.fields[fi];
                            if (field->type && field->type->kind == TYPE_STRING)
                            {
                                const char *c_field = field->c_alias != NULL
                                    ? field->c_alias
                                    : sn_mangle_name(gen->arena, field->name);
                                if (strlen(guard) > 0)
                                    guard = arena_sprintf(gen->arena, "%s && ", guard);
                                guard = arena_sprintf(gen->arena, "%s%s != _return_value.%s",
                                                       guard, var_name, c_field);
                            }
                        }
                        if (strlen(guard) > 0)
                            indented_fprintf(gen, indent, "if (%s) rt_arena_v2_free(%s);\n", guard, var_name);
                        else
                            indented_fprintf(gen, indent, "rt_arena_v2_free(%s);\n", var_name);
                    }
                    else
                    {
                        indented_fprintf(gen, indent, "rt_arena_v2_free(%s);\n", var_name);
                    }
                }
            }
            else
            {
                /* Non-arena context: manual memory management */
                if (sym->type->kind == TYPE_STRING)
                {
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
                else if (sym->type->kind == TYPE_ARRAY)
                {
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
            }
        }
        sym = sym->next;
    }
}

void code_gen_block(CodeGen *gen, BlockStmt *stmt, int indent)
{
    DEBUG_VERBOSE("Entering code_gen_block");

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

    /* Flush temps created during if-condition evaluation.
     * In struct methods, there's no arena condemn, so condition temps
     * (e.g., string literals for comparisons) must be explicitly freed.
     * In loops, flush to prevent accumulation across iterations. */
    if (gen->current_arena_var != NULL && gen->arena_temp_count > 0)
    {
        bool in_method = (gen->function_arena_var != NULL &&
                          strcmp(gen->function_arena_var, "__caller_arena__") == 0);
        if (in_method || gen->loop_scope_depth > 0)
        {
            code_gen_flush_arena_temps(gen, indent);
        }
    }
}

void code_gen_statement(CodeGen *gen, Stmt *stmt, int indent)
{
    DEBUG_VERBOSE("Entering code_gen_statement");

    /* Emit any attached comments */
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
    case STMT_CONTINUE:
    {
        /* In struct methods, free tracked arena temps before break/continue.
         * Temps (e.g. from trim/toLower) would normally be freed at statement
         * boundary, but break/continue skips past that cleanup.
         * IMPORTANT: Do NOT clear arena_temp_count. The per-statement/if-condition
         * flush also emits frees for the non-break path. Both paths need frees;
         * only one executes at runtime. */
        if (gen->current_arena_var != NULL && gen->arena_temp_count > 0 &&
            gen->function_arena_var != NULL &&
            strcmp(gen->function_arena_var, "__caller_arena__") == 0)
        {
            for (int i = 0; i < gen->arena_temp_count; i++)
            {
                indented_fprintf(gen, indent, "rt_arena_v2_free(%s);\n", gen->arena_temps[i]);
            }
        }

        /* Clean up struct locals in all scopes from current up to the loop scope.
         * This handles the case where break/continue is nested inside inner blocks. */
        Scope *loop_scope = NULL;
        if (gen->loop_scope_depth > 0)
        {
            loop_scope = gen->loop_scope_stack[gen->loop_scope_depth - 1];
        }
        Scope *scope = gen->symbol_table->current;
        while (scope != NULL && scope != loop_scope)
        {
            code_gen_free_locals(gen, scope, false, indent);
            scope = scope->enclosing;
        }

        if (stmt->type == STMT_BREAK)
        {
            indented_fprintf(gen, indent, "break;\n");
        }
        else  /* STMT_CONTINUE */
        {
            if (gen->for_continue_label)
            {
                indented_fprintf(gen, indent, "goto %s;\n", gen->for_continue_label);
            }
            else
            {
                indented_fprintf(gen, indent, "continue;\n");
            }
        }
        break;
    }
    case STMT_IMPORT:
        /* Handle namespaced imports */
        if (stmt->as.import.namespace != NULL && stmt->as.import.imported_stmts != NULL)
        {
            bool emit_functions = true;
            Token ns_name = *stmt->as.import.namespace;
            Symbol *ns_symbol = NULL;

            if (gen->current_namespace_prefix != NULL)
            {
                Token parent_ns_token;
                parent_ns_token.start = gen->current_namespace_prefix;
                parent_ns_token.length = strlen(gen->current_namespace_prefix);
                parent_ns_token.type = TOKEN_IDENTIFIER;
                parent_ns_token.line = 0;
                parent_ns_token.filename = NULL;
                ns_symbol = symbol_table_lookup_nested_namespace(gen->symbol_table, parent_ns_token, ns_name);
            }

            if (ns_symbol == NULL)
            {
                ns_symbol = symbol_table_lookup_symbol(gen->symbol_table, ns_name);
            }

            if (ns_symbol != NULL && ns_symbol->is_namespace)
            {
                symbol_table_push_scope(gen->symbol_table);
                add_namespace_symbols_to_scope(gen, ns_symbol);
            }

            const char *old_namespace_prefix = gen->current_namespace_prefix;
            const char *old_canonical_module = gen->current_canonical_module;
            char ns_prefix[256];
            int ns_len = ns_name.length < 255 ? ns_name.length : 255;
            memcpy(ns_prefix, ns_name.start, ns_len);
            ns_prefix[ns_len] = '\0';
            gen->current_namespace_prefix = arena_strdup(gen->arena, ns_prefix);

            if (ns_symbol != NULL && ns_symbol->canonical_module_name != NULL)
            {
                gen->current_canonical_module = ns_symbol->canonical_module_name;
            }
            else
            {
                char mod_path[512];
                int mod_len = stmt->as.import.module_name.length < 511 ? stmt->as.import.module_name.length : 511;
                memcpy(mod_path, stmt->as.import.module_name.start, mod_len);
                mod_path[mod_len] = '\0';

                const char *base_name = mod_path;
                for (int k = mod_len - 1; k >= 0; k--)
                {
                    if (mod_path[k] == '/' || mod_path[k] == '\\')
                    {
                        base_name = &mod_path[k + 1];
                        break;
                    }
                }
                char *canonical = arena_strdup(gen->arena, base_name);
                int can_len = strlen(canonical);
                if (can_len > 3 && strcmp(canonical + can_len - 3, ".sn") == 0)
                {
                    canonical[can_len - 3] = '\0';
                }
                gen->current_canonical_module = canonical;
            }

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
                if (!emit_functions && imported_stmt->type == STMT_FUNCTION)
                    continue;
                code_gen_statement(gen, imported_stmt, indent);
            }

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

            gen->current_namespace_prefix = old_namespace_prefix;
            gen->current_canonical_module = old_canonical_module;

            if (ns_symbol != NULL && ns_symbol->is_namespace)
            {
                symbol_table_pop_scope(gen->symbol_table);
            }
        }
        break;
    case STMT_PRAGMA:
        /* Pragmas handled at module level */
        break;
    case STMT_TYPE_DECL:
        /* Type declarations handled at module level */
        break;
    case STMT_STRUCT_DECL:
        /* Struct declarations handled at module level */
        break;
    case STMT_LOCK:
    {
        LockStmt *lock_stmt = &stmt->as.lock_stmt;
        char *lock_var = code_gen_expression(gen, lock_stmt->lock_expr);
        indented_fprintf(gen, indent, "rt_sync_lock(&%s);\n", lock_var);

        /* Push lock variable onto the lock stack so return statements
         * inside this block can emit the corresponding unlock. */
        if (gen->lock_stack_depth >= gen->lock_stack_capacity)
        {
            int new_cap = gen->lock_stack_capacity == 0 ? 4 : gen->lock_stack_capacity * 2;
            char **new_stack = arena_alloc(gen->arena, new_cap * sizeof(char *));
            for (int i = 0; i < gen->lock_stack_depth; i++)
            {
                new_stack[i] = gen->lock_stack[i];
            }
            gen->lock_stack = new_stack;
            gen->lock_stack_capacity = new_cap;
        }
        gen->lock_stack[gen->lock_stack_depth++] = lock_var;

        indented_fprintf(gen, indent, "{\n");
        code_gen_statement(gen, lock_stmt->body, indent + 1);
        indented_fprintf(gen, indent, "}\n");

        /* Pop the lock stack */
        gen->lock_stack_depth--;

        indented_fprintf(gen, indent, "rt_sync_unlock(&%s);\n", lock_var);
        break;
    }
    }
}
