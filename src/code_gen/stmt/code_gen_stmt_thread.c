/*
 * code_gen_stmt_thread.c - Thread synchronization statement code generation
 *
 * Handles the thread sync statement (r!) for waiting on async results.
 */

#include "code_gen/stmt/code_gen_stmt.h"
#include "code_gen/expr/code_gen_expr.h"
#include "code_gen/util/code_gen_util.h"
#include "debug.h"
#include "symbol_table.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* Generate thread sync as a statement - assigns results back to variables
 * For single sync (r!): r = sync_result
 * For sync list ([r1, r2, r3]!): r1 = sync_result1; r2 = sync_result2; ... */
void code_gen_thread_sync_statement(CodeGen *gen, Expr *expr, int indent)
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
