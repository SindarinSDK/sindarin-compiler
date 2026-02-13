/*
 * code_gen_stmt_thread.c - Thread synchronization statement code generation
 *
 * Handles the thread sync statement (r!) for waiting on async results.
 * Uses V2 threading API.
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
                indented_fprintf(gen, indent, "rt_thread_v2_sync(%s);\n", var_name);
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
            bool struct_needs_field_promotion = is_struct && struct_has_handle_fields(result_type);

            /* Check if array needs deep promotion (2D/3D arrays or string arrays) */
            bool array_needs_deep_promotion = is_handle && result_type->kind == TYPE_ARRAY &&
                result_type->as.array.element_type != NULL &&
                (result_type->as.array.element_type->kind == TYPE_ARRAY ||
                 result_type->as.array.element_type->kind == TYPE_STRING);

            char *pending_var = arena_sprintf(gen->arena, "__%s_pending__", raw_var_name);
            indented_fprintf(gen, indent, "if (%s != NULL) {\n", pending_var);

            if (array_needs_deep_promotion)
            {
                /* Arrays that need deep promotion (2D/3D arrays, string arrays):
                 * Use sync_keep_arena, then deep promote, then destroy arena */
                Type *elem_type = result_type->as.array.element_type;
                const char *promo_func;
                if (elem_type->kind == TYPE_STRING)
                {
                    /* 1D string array - promote each string handle */
                    promo_func = "rt_promote_array_string_v2";
                }
                else if (elem_type->kind == TYPE_ARRAY)
                {
                    /* Nested array - check depth and element type */
                    Type *inner_elem = elem_type->as.array.element_type;
                    if (inner_elem != NULL && inner_elem->kind == TYPE_ARRAY)
                    {
                        /* 3D array - check innermost type */
                        Type *innermost = inner_elem->as.array.element_type;
                        if (innermost != NULL && innermost->kind == TYPE_STRING)
                        {
                            /* 3D string array (string[][][]) */
                            promo_func = "rt_promote_array3_string_v2";
                        }
                        else
                        {
                            /* 3D array of primitives (T[][][]) */
                            promo_func = "rt_promote_array_handle_3d_v2";
                        }
                    }
                    else if (inner_elem != NULL && inner_elem->kind == TYPE_STRING)
                    {
                        /* 2D string array (string[][]) */
                        promo_func = "rt_promote_array2_string_v2";
                    }
                    else
                    {
                        /* 2D array of primitives (T[][]) */
                        promo_func = "rt_promote_array_handle_v2";
                    }
                }
                else
                {
                    promo_func = "rt_arena_v2_promote"; /* Fallback - shouldn't happen */
                }

                indented_fprintf(gen, indent + 1, "RtArenaV2 *__thread_arena__ = %s->arena;\n", pending_var);
                indented_fprintf(gen, indent + 1, "RtHandleV2 *__sync_tmp__ = rt_thread_v2_sync_keep_arena(%s);\n", pending_var);
                indented_fprintf(gen, indent + 1, "%s = (%s)%s(%s, __sync_tmp__);\n",
                    var_name, c_type, promo_func, ARENA_VAR(gen));
                indented_fprintf(gen, indent + 1, "if (__thread_arena__ != NULL) rt_arena_v2_destroy(__thread_arena__);\n");
            }
            else if (is_handle)
            {
                /* Handle types (string/simple array in arena mode): thread stored result directly,
                 * sync returns the promoted handle which we assign directly */
                indented_fprintf(gen, indent + 1, "%s = (%s)rt_thread_v2_sync(%s);\n",
                    var_name, c_type, pending_var);
            }
            else if (struct_needs_field_promotion)
            {
                /* Struct with handle fields - keep arena alive for field promotion */
                indented_fprintf(gen, indent + 1, "RtArenaV2 *__thread_arena__ = %s->arena;\n", pending_var);
                indented_fprintf(gen, indent + 1, "RtHandleV2 *__sync_h__ = rt_thread_v2_sync_keep_arena(%s);\n", pending_var);
                indented_fprintf(gen, indent + 1, "rt_handle_v2_pin(__sync_h__);\n");
                indented_fprintf(gen, indent + 1, "%s = *(%s *)__sync_h__->ptr;\n",
                    var_name, c_type);
                /* Generate field promotion code */
                char *field_promotion = gen_struct_field_promotion(gen, result_type, var_name,
                    ARENA_VAR(gen), "__thread_arena__");
                if (field_promotion && field_promotion[0] != '\0') {
                    indented_fprintf(gen, indent + 1, "%s", field_promotion);
                }
                /* Clean up the thread arena after field promotion */
                indented_fprintf(gen, indent + 1, "if (__thread_arena__ != NULL) rt_arena_v2_destroy(__thread_arena__);\n");
            }
            else if (is_primitive || is_struct)
            {
                /* Primitives and simple structs: result is wrapped, dereference it */
                indented_fprintf(gen, indent + 1, "RtHandleV2 *__sync_h__ = rt_thread_v2_sync(%s);\n", pending_var);
                indented_fprintf(gen, indent + 1, "rt_handle_v2_pin(__sync_h__);\n");
                indented_fprintf(gen, indent + 1, "%s = *(%s *)__sync_h__->ptr;\n",
                    var_name, c_type);
            }
            else
            {
                /* Reference types without arena mode: assign directly */
                indented_fprintf(gen, indent + 1, "%s = (%s)rt_thread_v2_sync(%s);\n",
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

                indented_fprintf(gen, indent, "{\n");
                indented_fprintf(gen, indent + 1, "int __sync_idx__ = (int)(%s);\n", idx_str);
                indented_fprintf(gen, indent + 1, "if (__sync_idx__ < 0) __sync_idx__ = (int)rt_array_length_v2(%s) + __sync_idx__;\n", arr_name);
                indented_fprintf(gen, indent + 1, "if (%s != NULL) {\n", pending_elems_var);
                indented_fprintf(gen, indent + 2, "void **__pe_data__ = (void **)rt_array_data_v2(%s);\n", pending_elems_var);
                indented_fprintf(gen, indent + 2, "if (__pe_data__[__sync_idx__] != NULL) {\n");

                if (is_primitive)
                {
                    indented_fprintf(gen, indent + 3, "RtHandleV2 *__sync_h__ = rt_thread_v2_sync((RtThread *)__pe_data__[__sync_idx__]);\n");
                    indented_fprintf(gen, indent + 3, "rt_handle_v2_pin(__sync_h__);\n");
                    indented_fprintf(gen, indent + 3, "((%s *)rt_array_data_v2(%s))[__sync_idx__] = *(%s *)__sync_h__->ptr;\n",
                        c_type, arr_name, c_type);
                }
                else
                {
                    indented_fprintf(gen, indent + 3, "((%s *)rt_array_data_v2(%s))[__sync_idx__] = (%s)rt_thread_v2_sync((RtThread *)__pe_data__[__sync_idx__]);\n",
                        c_type, arr_name, c_type);
                }

                indented_fprintf(gen, indent + 3, "__pe_data__[__sync_idx__] = NULL;\n");
                indented_fprintf(gen, indent + 2, "}\n");
                indented_fprintf(gen, indent + 1, "}\n");
                indented_fprintf(gen, indent, "}\n");
                return;
            }
        }

        /* Whole-array sync statement: arr! (for arrays with pending elements) */
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

                indented_fprintf(gen, indent, "if (%s != NULL) {\n", pending_elems_var);
                indented_fprintf(gen, indent + 1, "int __sync_len__ = (int)rt_array_length_v2(%s);\n", pending_elems_var);
                indented_fprintf(gen, indent + 1, "void **__pe_data__ = (void **)rt_array_data_v2(%s);\n", pending_elems_var);
                indented_fprintf(gen, indent + 1, "for (int __i__ = 0; __i__ < __sync_len__; __i__++) {\n");
                indented_fprintf(gen, indent + 2, "if (__pe_data__[__i__] != NULL) {\n");

                if (is_primitive_elem)
                {
                    indented_fprintf(gen, indent + 3, "RtHandleV2 *__sync_h__ = rt_thread_v2_sync((RtThread *)__pe_data__[__i__]);\n");
                    indented_fprintf(gen, indent + 3, "rt_handle_v2_pin(__sync_h__);\n");
                    indented_fprintf(gen, indent + 3, "((%s *)rt_array_data_v2(%s))[__i__] = *(%s *)__sync_h__->ptr;\n",
                        elem_c_type, var_name, elem_c_type);
                }
                else
                {
                    indented_fprintf(gen, indent + 3, "((%s *)rt_array_data_v2(%s))[__i__] = (%s)rt_thread_v2_sync((RtThread *)__pe_data__[__i__]);\n",
                        elem_c_type, var_name, elem_c_type);
                }

                indented_fprintf(gen, indent + 3, "__pe_data__[__i__] = NULL;\n");
                indented_fprintf(gen, indent + 2, "}\n");
                indented_fprintf(gen, indent + 1, "}\n");
                indented_fprintf(gen, indent, "}\n");
                return;
            }
        }

        /* Single sync: r!
         * Only assign back if the handle is a variable */

        if (handle->type == EXPR_VARIABLE)
        {
            char *raw_var_name = get_var_name(gen->arena, handle->as.variable.name);
            char *var_name = sn_mangle_name(gen->arena, raw_var_name);
            Type *result_type = expr->expr_type;

            /* Check if void - just sync, no assignment */
            if (result_type == NULL || result_type->kind == TYPE_VOID)
            {
                char *pending_var = arena_sprintf(gen->arena, "__%s_pending__", raw_var_name);
                indented_fprintf(gen, indent, "if (%s != NULL) {\n", pending_var);
                indented_fprintf(gen, indent + 1, "rt_thread_v2_sync(%s);\n", pending_var);
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
            bool struct_needs_field_promotion = is_struct && struct_has_handle_fields(result_type);

            /* Check if array needs deep promotion (2D/3D arrays or string arrays) */
            bool array_needs_deep_promotion = is_handle && result_type->kind == TYPE_ARRAY &&
                result_type->as.array.element_type != NULL &&
                (result_type->as.array.element_type->kind == TYPE_ARRAY ||
                 result_type->as.array.element_type->kind == TYPE_STRING);

            char *pending_var = arena_sprintf(gen->arena, "__%s_pending__", raw_var_name);
            indented_fprintf(gen, indent, "if (%s != NULL) {\n", pending_var);

            if (array_needs_deep_promotion)
            {
                /* Arrays that need deep promotion (2D/3D arrays, string arrays):
                 * Use sync_keep_arena, then deep promote, then destroy arena */
                Type *elem_type = result_type->as.array.element_type;
                const char *promo_func;
                if (elem_type->kind == TYPE_STRING)
                {
                    /* 1D string array - promote each string handle */
                    promo_func = "rt_promote_array_string_v2";
                }
                else if (elem_type->kind == TYPE_ARRAY)
                {
                    /* Nested array - check depth and element type */
                    Type *inner_elem = elem_type->as.array.element_type;
                    if (inner_elem != NULL && inner_elem->kind == TYPE_ARRAY)
                    {
                        /* 3D array - check innermost type */
                        Type *innermost = inner_elem->as.array.element_type;
                        if (innermost != NULL && innermost->kind == TYPE_STRING)
                        {
                            /* 3D string array (string[][][]) */
                            promo_func = "rt_promote_array3_string_v2";
                        }
                        else
                        {
                            /* 3D array of primitives (T[][][]) */
                            promo_func = "rt_promote_array_handle_3d_v2";
                        }
                    }
                    else if (inner_elem != NULL && inner_elem->kind == TYPE_STRING)
                    {
                        /* 2D string array (string[][]) */
                        promo_func = "rt_promote_array2_string_v2";
                    }
                    else
                    {
                        /* 2D array of primitives (T[][]) */
                        promo_func = "rt_promote_array_handle_v2";
                    }
                }
                else
                {
                    promo_func = "rt_arena_v2_promote"; /* Fallback - shouldn't happen */
                }

                indented_fprintf(gen, indent + 1, "RtArenaV2 *__thread_arena__ = %s->arena;\n", pending_var);
                indented_fprintf(gen, indent + 1, "RtHandleV2 *__sync_tmp__ = rt_thread_v2_sync_keep_arena(%s);\n", pending_var);
                indented_fprintf(gen, indent + 1, "%s = (%s)%s(%s, __sync_tmp__);\n",
                    var_name, c_type, promo_func, ARENA_VAR(gen));
                indented_fprintf(gen, indent + 1, "if (__thread_arena__ != NULL) rt_arena_v2_destroy(__thread_arena__);\n");
            }
            else if (is_handle)
            {
                /* Handle types (string/simple array in arena mode): thread stored result directly,
                 * sync returns the promoted handle which we assign directly */
                indented_fprintf(gen, indent + 1, "%s = (%s)rt_thread_v2_sync(%s);\n",
                    var_name, c_type, pending_var);
            }
            else if (struct_needs_field_promotion)
            {
                /* Struct with handle fields - keep arena alive for field promotion */
                indented_fprintf(gen, indent + 1, "RtArenaV2 *__thread_arena__ = %s->arena;\n", pending_var);
                indented_fprintf(gen, indent + 1, "RtHandleV2 *__sync_h__ = rt_thread_v2_sync_keep_arena(%s);\n", pending_var);
                indented_fprintf(gen, indent + 1, "rt_handle_v2_pin(__sync_h__);\n");
                indented_fprintf(gen, indent + 1, "%s = *(%s *)__sync_h__->ptr;\n",
                    var_name, c_type);
                /* Generate field promotion code */
                char *field_promotion = gen_struct_field_promotion(gen, result_type, var_name,
                    ARENA_VAR(gen), "__thread_arena__");
                if (field_promotion && field_promotion[0] != '\0') {
                    indented_fprintf(gen, indent + 1, "%s", field_promotion);
                }
                /* Clean up the thread arena after field promotion */
                indented_fprintf(gen, indent + 1, "if (__thread_arena__ != NULL) rt_arena_v2_destroy(__thread_arena__);\n");
            }
            else if (is_primitive || is_struct)
            {
                /* Primitives and simple structs: result is wrapped, dereference it */
                indented_fprintf(gen, indent + 1, "RtHandleV2 *__sync_h__ = rt_thread_v2_sync(%s);\n", pending_var);
                indented_fprintf(gen, indent + 1, "rt_handle_v2_pin(__sync_h__);\n");
                indented_fprintf(gen, indent + 1, "%s = *(%s *)__sync_h__->ptr;\n",
                    var_name, c_type);
            }
            else
            {
                /* Reference types without arena mode: assign directly */
                indented_fprintf(gen, indent + 1, "%s = (%s)rt_thread_v2_sync(%s);\n",
                    var_name, c_type, pending_var);
            }

            indented_fprintf(gen, indent + 1, "%s = NULL;\n", pending_var);
            indented_fprintf(gen, indent, "}\n");
        }
        else
        {
            /* Non-variable sync (e.g., &fn()!) - just execute the sync expression */
            char *expr_str = code_gen_expression(gen, expr);
            indented_fprintf(gen, indent, "%s;\n", expr_str);
        }
    }
}
