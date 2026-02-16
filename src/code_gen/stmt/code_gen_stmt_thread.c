/*
 * code_gen_stmt_thread.c - Thread synchronization statement code generation
 *
 * Handles the thread sync statement (r!) for waiting on async results.
 * Uses V3 threading API where rt_thread_v3_sync handles promotion
 * including deep copy via copy_callback.
 */

#include "code_gen/stmt/code_gen_stmt.h"
#include "code_gen/expr/code_gen_expr.h"
#include "code_gen/util/code_gen_util.h"
#include "debug.h"
#include "symbol_table.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* Generate thread sync as a statement - assigns results back to variables */
void code_gen_thread_sync_statement(CodeGen *gen, Expr *expr, int indent)
{
    ThreadSyncExpr *sync = &expr->as.thread_sync;

    if (sync->is_array)
    {
        /* Sync list: [r1, r2, r3]! */
        Expr *list_expr = sync->handle;
        if (list_expr->type != EXPR_SYNC_LIST)
        {
            fprintf(stderr, "Error: Multi-sync requires sync list expression\n");
            exit(1);
        }

        SyncListExpr *sync_list = &list_expr->as.sync_list;

        if (sync_list->element_count == 0)
        {
            return;
        }

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

            Symbol *sym = symbol_table_lookup_symbol(gen->symbol_table, elem->as.variable.name);
            Type *result_type = (sym != NULL) ? sym->type : elem->expr_type;

            if (result_type == NULL || result_type->kind == TYPE_VOID)
            {
                char *pending_var = arena_sprintf(gen->arena, "__%s_pending__", raw_var_name);
                indented_fprintf(gen, indent, "if (%s != NULL) {\n", pending_var);
                indented_fprintf(gen, indent + 1, "rt_thread_v3_sync(%s);\n", pending_var);
                indented_fprintf(gen, indent + 1, "%s = NULL;\n", pending_var);
                indented_fprintf(gen, indent, "}\n");
                continue;
            }

            const char *c_type = get_c_type(gen->arena, result_type);

            bool is_primitive = (result_type->kind == TYPE_INT ||
                                result_type->kind == TYPE_LONG ||
                                result_type->kind == TYPE_DOUBLE ||
                                result_type->kind == TYPE_BOOL ||
                                result_type->kind == TYPE_BYTE ||
                                result_type->kind == TYPE_CHAR);
            bool is_handle = gen->current_arena_var != NULL &&
                            (result_type->kind == TYPE_STRING || result_type->kind == TYPE_ARRAY);
            bool is_struct = (result_type->kind == TYPE_STRUCT);

            char *pending_var = arena_sprintf(gen->arena, "__%s_pending__", raw_var_name);
            indented_fprintf(gen, indent, "if (%s != NULL) {\n", pending_var);

            if (is_handle)
            {
                /* Handle types: V3 sync returns promoted handle directly */
                indented_fprintf(gen, indent + 1, "%s = (%s)rt_thread_v3_sync(%s);\n",
                    var_name, c_type, pending_var);
            }
            else if (is_primitive || is_struct)
            {
                /* Primitives and structs: V3 sync returns promoted handle, dereference.
                 * For structs with handle fields, copy_callback handles deep copy. */
                indented_fprintf(gen, indent + 1, "RtHandleV2 *__sync_h__ = rt_thread_v3_sync(%s);\n", pending_var);
                indented_fprintf(gen, indent + 1, "rt_handle_begin_transaction(__sync_h__);\n");
                indented_fprintf(gen, indent + 1, "%s = *(%s *)__sync_h__->ptr;\n",
                    var_name, c_type);
                indented_fprintf(gen, indent + 1, "rt_handle_end_transaction(__sync_h__);\n");
            }
            else
            {
                /* Reference types without arena mode */
                indented_fprintf(gen, indent + 1, "%s = (%s)rt_thread_v3_sync(%s);\n",
                    var_name, c_type, pending_var);
            }

            indented_fprintf(gen, indent + 1, "%s = NULL;\n", pending_var);
            indented_fprintf(gen, indent, "}\n");
        }
    }
    else
    {
        /* Array element sync statement: arr[i]! */
        Expr *handle = sync->handle;

        if (handle->type == EXPR_ARRAY_ACCESS)
        {
            Expr *arr_expr = handle->as.array_access.array;
            Expr *idx_expr = handle->as.array_access.index;

            if (arr_expr->type == EXPR_VARIABLE)
            {
                char *raw_arr_name = get_var_name(gen->arena, arr_expr->as.variable.name);
                char *arr_name = sn_mangle_name(gen->arena, raw_arr_name);
                char *pending_elems_var = arena_sprintf(gen->arena, "__%s_pending_elems__", raw_arr_name);
                char *idx_str = code_gen_expression(gen, idx_expr);

                Type *result_type = expr->expr_type;
                const char *c_type = get_c_type(gen->arena, result_type);

                bool is_primitive = (result_type->kind == TYPE_INT ||
                                    result_type->kind == TYPE_LONG ||
                                    result_type->kind == TYPE_DOUBLE ||
                                    result_type->kind == TYPE_BOOL ||
                                    result_type->kind == TYPE_BYTE ||
                                    result_type->kind == TYPE_CHAR);
                bool is_struct = (result_type->kind == TYPE_STRUCT);

                indented_fprintf(gen, indent, "{\n");
                indented_fprintf(gen, indent + 1, "int __sync_idx__ = (int)(%s);\n", idx_str);
                indented_fprintf(gen, indent + 1, "if (__sync_idx__ < 0) __sync_idx__ = (int)rt_array_length_v2(%s) + __sync_idx__;\n", arr_name);
                indented_fprintf(gen, indent + 1, "if (%s != NULL) {\n", pending_elems_var);
                indented_fprintf(gen, indent + 2, "void **__pe_data__ = (void **)rt_array_data_v2(%s);\n", pending_elems_var);
                indented_fprintf(gen, indent + 2, "if (__pe_data__[__sync_idx__] != NULL) {\n");

                if (is_primitive || is_struct)
                {
                    /* V3 sync returns promoted handle, dereference */
                    indented_fprintf(gen, indent + 3, "RtHandleV2 *__sync_h__ = rt_thread_v3_sync((RtHandleV2 *)__pe_data__[__sync_idx__]);\n");
                    indented_fprintf(gen, indent + 3, "rt_handle_begin_transaction(__sync_h__);\n");
                    indented_fprintf(gen, indent + 3, "((%s *)rt_array_data_v2(%s))[__sync_idx__] = *(%s *)__sync_h__->ptr;\n",
                        c_type, arr_name, c_type);
                    indented_fprintf(gen, indent + 3, "rt_handle_end_transaction(__sync_h__);\n");
                }
                else
                {
                    /* Handle types: V3 sync returns promoted handle directly */
                    indented_fprintf(gen, indent + 3, "((%s *)rt_array_data_v2(%s))[__sync_idx__] = (%s)rt_thread_v3_sync((RtHandleV2 *)__pe_data__[__sync_idx__]);\n",
                        c_type, arr_name, c_type);
                }

                indented_fprintf(gen, indent + 3, "__pe_data__[__sync_idx__] = NULL;\n");
                indented_fprintf(gen, indent + 2, "}\n");
                indented_fprintf(gen, indent + 1, "}\n");
                indented_fprintf(gen, indent, "}\n");
                return;
            }
        }

        /* Whole-array sync statement: arr! */
        if (handle->type == EXPR_VARIABLE)
        {
            Symbol *sym = symbol_table_lookup_symbol(gen->symbol_table, handle->as.variable.name);
            if (sym != NULL && sym->has_pending_elements)
            {
                char *raw_var_name = get_var_name(gen->arena, handle->as.variable.name);
                char *var_name = sn_mangle_name(gen->arena, raw_var_name);
                char *pending_elems_var = arena_sprintf(gen->arena, "__%s_pending_elems__", raw_var_name);

                Type *result_type = expr->expr_type;
                Type *elem_type = result_type->as.array.element_type;
                const char *elem_c_type = get_c_type(gen->arena, elem_type);

                bool is_primitive_elem = (elem_type->kind == TYPE_INT ||
                                          elem_type->kind == TYPE_LONG ||
                                          elem_type->kind == TYPE_DOUBLE ||
                                          elem_type->kind == TYPE_BOOL ||
                                          elem_type->kind == TYPE_BYTE ||
                                          elem_type->kind == TYPE_CHAR);
                bool is_struct_elem = (elem_type->kind == TYPE_STRUCT);

                indented_fprintf(gen, indent, "if (%s != NULL) {\n", pending_elems_var);
                indented_fprintf(gen, indent + 1, "int __sync_len__ = (int)rt_array_length_v2(%s);\n", pending_elems_var);
                indented_fprintf(gen, indent + 1, "void **__pe_data__ = (void **)rt_array_data_v2(%s);\n", pending_elems_var);
                indented_fprintf(gen, indent + 1, "for (int __i__ = 0; __i__ < __sync_len__; __i__++) {\n");
                indented_fprintf(gen, indent + 2, "if (__pe_data__[__i__] != NULL) {\n");

                if (is_primitive_elem || is_struct_elem)
                {
                    /* V3 sync returns promoted handle, dereference */
                    indented_fprintf(gen, indent + 3, "RtHandleV2 *__sync_h__ = rt_thread_v3_sync((RtHandleV2 *)__pe_data__[__i__]);\n");
                    indented_fprintf(gen, indent + 3, "rt_handle_begin_transaction(__sync_h__);\n");
                    indented_fprintf(gen, indent + 3, "((%s *)rt_array_data_v2(%s))[__i__] = *(%s *)__sync_h__->ptr;\n",
                        elem_c_type, var_name, elem_c_type);
                    indented_fprintf(gen, indent + 3, "rt_handle_end_transaction(__sync_h__);\n");
                }
                else
                {
                    /* Handle types: V3 sync returns promoted handle directly */
                    indented_fprintf(gen, indent + 3, "((%s *)rt_array_data_v2(%s))[__i__] = (%s)rt_thread_v3_sync((RtHandleV2 *)__pe_data__[__i__]);\n",
                        elem_c_type, var_name, elem_c_type);
                }

                indented_fprintf(gen, indent + 3, "__pe_data__[__i__] = NULL;\n");
                indented_fprintf(gen, indent + 2, "}\n");
                indented_fprintf(gen, indent + 1, "}\n");
                indented_fprintf(gen, indent, "}\n");
                return;
            }
        }

        /* Single sync: r! */
        if (handle->type == EXPR_VARIABLE)
        {
            char *raw_var_name = get_var_name(gen->arena, handle->as.variable.name);
            char *var_name = sn_mangle_name(gen->arena, raw_var_name);
            Type *result_type = expr->expr_type;

            if (result_type == NULL || result_type->kind == TYPE_VOID)
            {
                char *pending_var = arena_sprintf(gen->arena, "__%s_pending__", raw_var_name);
                indented_fprintf(gen, indent, "if (%s != NULL) {\n", pending_var);
                indented_fprintf(gen, indent + 1, "rt_thread_v3_sync(%s);\n", pending_var);
                indented_fprintf(gen, indent + 1, "%s = NULL;\n", pending_var);
                indented_fprintf(gen, indent, "}\n");
                return;
            }

            const char *c_type = get_c_type(gen->arena, result_type);

            bool is_primitive = (result_type->kind == TYPE_INT ||
                                result_type->kind == TYPE_LONG ||
                                result_type->kind == TYPE_DOUBLE ||
                                result_type->kind == TYPE_BOOL ||
                                result_type->kind == TYPE_BYTE ||
                                result_type->kind == TYPE_CHAR);
            bool is_handle = gen->current_arena_var != NULL &&
                            (result_type->kind == TYPE_STRING || result_type->kind == TYPE_ARRAY);
            bool is_struct = (result_type->kind == TYPE_STRUCT);

            char *pending_var = arena_sprintf(gen->arena, "__%s_pending__", raw_var_name);
            indented_fprintf(gen, indent, "if (%s != NULL) {\n", pending_var);

            if (is_handle)
            {
                /* Handle types: V3 sync returns promoted handle directly */
                indented_fprintf(gen, indent + 1, "%s = (%s)rt_thread_v3_sync(%s);\n",
                    var_name, c_type, pending_var);
            }
            else if (is_primitive || is_struct)
            {
                /* Primitives and structs: V3 sync returns promoted handle, dereference.
                 * For structs with handle fields, copy_callback handles deep copy. */
                indented_fprintf(gen, indent + 1, "RtHandleV2 *__sync_h__ = rt_thread_v3_sync(%s);\n", pending_var);
                indented_fprintf(gen, indent + 1, "rt_handle_begin_transaction(__sync_h__);\n");
                indented_fprintf(gen, indent + 1, "%s = *(%s *)__sync_h__->ptr;\n",
                    var_name, c_type);
                indented_fprintf(gen, indent + 1, "rt_handle_end_transaction(__sync_h__);\n");
            }
            else
            {
                /* Reference types without arena mode */
                indented_fprintf(gen, indent + 1, "%s = (%s)rt_thread_v3_sync(%s);\n",
                    var_name, c_type, pending_var);
            }

            indented_fprintf(gen, indent + 1, "%s = NULL;\n", pending_var);
            indented_fprintf(gen, indent, "}\n");
        }
        else
        {
            /* Non-variable sync - just execute the sync expression */
            char *expr_str = code_gen_expression(gen, expr);
            indented_fprintf(gen, indent, "%s;\n", expr_str);
        }
    }
}
