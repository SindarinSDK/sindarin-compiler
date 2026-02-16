/**
 * code_gen_expr_thread_sync.c - Thread sync expression code generation
 *
 * Contains code generation for var! sync expressions.
 * Uses V3 threading API where rt_thread_v3_sync handles promotion
 * including deep copy via copy_callback.
 */

#include "code_gen/expr/thread/code_gen_expr_thread_sync.h"
#include "code_gen/expr/thread/code_gen_expr_thread_util.h"
#include "code_gen/expr/code_gen_expr.h"
#include "code_gen/util/code_gen_util.h"
#include "debug.h"
#include <stdio.h>
#include <stdlib.h>

char *code_gen_thread_sync_expression(CodeGen *gen, Expr *expr)
{
    DEBUG_VERBOSE("Entering code_gen_thread_sync_expression");

    ThreadSyncExpr *sync = &expr->as.thread_sync;

    if (sync->is_array)
    {
        /* Sync list: [r1, r2, r3]! */
        DEBUG_VERBOSE("Thread sync: sync list");

        Expr *list_expr = sync->handle;
        if (list_expr->type != EXPR_SYNC_LIST)
        {
            fprintf(stderr, "Error: Multi-sync requires sync list expression\n");
            exit(1);
        }

        SyncListExpr *sync_list = &list_expr->as.sync_list;
        int count = sync_list->element_count;

        if (count == 0)
        {
            return arena_strdup(gen->arena, "((void)0)");
        }

        int temp_id = gen->temp_count++;

        char *handles_init = arena_strdup(gen->arena, "");
        for (int i = 0; i < count; i++)
        {
            char *handle_code = code_gen_expression(gen, sync_list->elements[i]);
            if (i > 0)
            {
                handles_init = arena_sprintf(gen->arena, "%s, ", handles_init);
            }
            handles_init = arena_sprintf(gen->arena, "%s%s", handles_init, handle_code);
        }

        /* V3: sync_all with RtHandleV2* array */
        return arena_sprintf(gen->arena,
            "({\n"
            "    RtHandleV2 *__sync_handles_%d__[] = {%s};\n"
            "    rt_thread_v3_sync_all(__sync_handles_%d__, %d);\n"
            "    (void)0;\n"
            "})",
            temp_id, handles_init,
            temp_id, count);
    }
    else
    {
        /* Array element sync: arr[i]! */
        if (sync->handle->type == EXPR_ARRAY_ACCESS)
        {
            Expr *arr_expr = sync->handle->as.array_access.array;
            Expr *idx_expr = sync->handle->as.array_access.index;

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

                if (is_primitive || is_struct)
                {
                    /* Primitive or struct: V3 sync returns promoted handle, dereference it */
                    return arena_sprintf(gen->arena,
                        "({\n"
                        "    int __sync_idx__ = (int)(%s);\n"
                        "    if (__sync_idx__ < 0) __sync_idx__ = (int)rt_array_length_v2(%s) + __sync_idx__;\n"
                        "    if (%s != NULL) {\n"
                        "        void **__pe_data__ = (void **)rt_array_data_v2(%s);\n"
                        "        if (__pe_data__[__sync_idx__] != NULL) {\n"
                        "            RtHandleV2 *__sync_h__ = rt_thread_v3_sync((RtHandleV2 *)__pe_data__[__sync_idx__]);\n"
                        "            rt_handle_begin_transaction(__sync_h__);\n"
                        "            ((%s *)rt_array_data_v2(%s))[__sync_idx__] = *(%s *)__sync_h__->ptr;\n"
                        "            rt_handle_end_transaction(__sync_h__);\n"
                        "            __pe_data__[__sync_idx__] = NULL;\n"
                        "        }\n"
                        "    }\n"
                        "    ((%s *)rt_array_data_v2(%s))[__sync_idx__];\n"
                        "})",
                        idx_str,
                        arr_name,
                        pending_elems_var,
                        pending_elems_var,
                        c_type, arr_name, c_type,
                        c_type, arr_name);
                }
                else
                {
                    /* Handle types (string/array): V3 sync returns promoted handle directly */
                    return arena_sprintf(gen->arena,
                        "({\n"
                        "    int __sync_idx__ = (int)(%s);\n"
                        "    if (__sync_idx__ < 0) __sync_idx__ = (int)rt_array_length_v2(%s) + __sync_idx__;\n"
                        "    if (%s != NULL) {\n"
                        "        void **__pe_data__ = (void **)rt_array_data_v2(%s);\n"
                        "        if (__pe_data__[__sync_idx__] != NULL) {\n"
                        "            ((%s *)rt_array_data_v2(%s))[__sync_idx__] = (%s)rt_thread_v3_sync((RtHandleV2 *)__pe_data__[__sync_idx__]);\n"
                        "            __pe_data__[__sync_idx__] = NULL;\n"
                        "        }\n"
                        "    }\n"
                        "    ((%s *)rt_array_data_v2(%s))[__sync_idx__];\n"
                        "})",
                        idx_str,
                        arr_name,
                        pending_elems_var,
                        pending_elems_var,
                        c_type, arr_name, c_type,
                        c_type, arr_name);
                }
            }
        }

        /* Whole-array sync for variables with pending elements: arr! */
        if (sync->handle->type == EXPR_VARIABLE)
        {
            Symbol *sym = symbol_table_lookup_symbol(gen->symbol_table, sync->handle->as.variable.name);
            if (sym != NULL && sym->has_pending_elements)
            {
                char *raw_var_name = get_var_name(gen->arena, sync->handle->as.variable.name);
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

                if (is_primitive_elem || is_struct_elem)
                {
                    /* Primitive or struct element: V3 sync returns promoted handle, dereference */
                    return arena_sprintf(gen->arena,
                        "({\n"
                        "    if (%s != NULL) {\n"
                        "        int __sync_len__ = (int)rt_array_length_v2(%s);\n"
                        "        void **__pe_data__ = (void **)rt_array_data_v2(%s);\n"
                        "        for (int __i__ = 0; __i__ < __sync_len__; __i__++) {\n"
                        "            if (__pe_data__[__i__] != NULL) {\n"
                        "                RtHandleV2 *__sync_h__ = rt_thread_v3_sync((RtHandleV2 *)__pe_data__[__i__]);\n"
                        "                rt_handle_begin_transaction(__sync_h__);\n"
                        "                ((%s *)rt_array_data_v2(%s))[__i__] = *(%s *)__sync_h__->ptr;\n"
                        "                rt_handle_end_transaction(__sync_h__);\n"
                        "                __pe_data__[__i__] = NULL;\n"
                        "            }\n"
                        "        }\n"
                        "    }\n"
                        "    %s;\n"
                        "})",
                        pending_elems_var,
                        pending_elems_var,
                        pending_elems_var,
                        elem_c_type, var_name, elem_c_type,
                        var_name);
                }
                else
                {
                    /* Handle types (string/array): V3 sync returns promoted handle directly */
                    return arena_sprintf(gen->arena,
                        "({\n"
                        "    if (%s != NULL) {\n"
                        "        int __sync_len__ = (int)rt_array_length_v2(%s);\n"
                        "        void **__pe_data__ = (void **)rt_array_data_v2(%s);\n"
                        "        for (int __i__ = 0; __i__ < __sync_len__; __i__++) {\n"
                        "            if (__pe_data__[__i__] != NULL) {\n"
                        "                ((%s *)rt_array_data_v2(%s))[__i__] = (%s)rt_thread_v3_sync((RtHandleV2 *)__pe_data__[__i__]);\n"
                        "                __pe_data__[__i__] = NULL;\n"
                        "            }\n"
                        "        }\n"
                        "    }\n"
                        "    %s;\n"
                        "})",
                        pending_elems_var,
                        pending_elems_var,
                        pending_elems_var,
                        elem_c_type, var_name, elem_c_type,
                        var_name);
                }
            }
        }

        /* Single variable sync: r! */
        DEBUG_VERBOSE("Thread sync: single variable sync");

        char *handle_code = code_gen_expression(gen, sync->handle);
        Type *result_type = expr->expr_type;

        /* Void sync */
        if (result_type == NULL || result_type->kind == TYPE_VOID)
        {
            return arena_sprintf(gen->arena,
                "({\n"
                "    rt_thread_v3_sync(%s);\n"
                "    (void)0;\n"
                "})",
                handle_code);
        }

        const char *c_type = get_c_type(gen->arena, result_type);

        bool is_primitive = (result_type->kind == TYPE_INT ||
                             result_type->kind == TYPE_LONG ||
                             result_type->kind == TYPE_DOUBLE ||
                             result_type->kind == TYPE_BOOL ||
                             result_type->kind == TYPE_BYTE ||
                             result_type->kind == TYPE_CHAR);
        bool is_handle_type = gen->current_arena_var != NULL &&
                              (result_type->kind == TYPE_STRING || result_type->kind == TYPE_ARRAY);
        bool is_struct_type = (result_type->kind == TYPE_STRUCT);

        bool is_variable_handle = (sync->handle->type == EXPR_VARIABLE);

        if (is_handle_type)
        {
            /* Handle types (string/array): V3 sync returns promoted handle directly */
            if (is_variable_handle)
            {
                char *raw_var_name = get_var_name(gen->arena, sync->handle->as.variable.name);
                char *var_name = sn_mangle_name(gen->arena, raw_var_name);
                char *pending_var = arena_sprintf(gen->arena, "__%s_pending__", raw_var_name);
                return arena_sprintf(gen->arena,
                    "({\n"
                    "    if (%s != NULL) {\n"
                    "        %s = (%s)rt_thread_v3_sync(%s);\n"
                    "        %s = NULL;\n"
                    "    }\n"
                    "    %s;\n"
                    "})",
                    pending_var,
                    var_name, c_type, pending_var,
                    pending_var,
                    var_name);
            }
            else
            {
                return arena_sprintf(gen->arena,
                    "((%s)rt_thread_v3_sync(%s))",
                    c_type, handle_code);
            }
        }
        else if (is_primitive || is_struct_type)
        {
            /* Primitive or struct: V3 sync returns promoted handle, dereference.
             * For structs with handle fields, copy_callback handles deep copy during sync. */
            if (is_variable_handle)
            {
                char *raw_var_name = get_var_name(gen->arena, sync->handle->as.variable.name);
                char *var_name = sn_mangle_name(gen->arena, raw_var_name);
                char *pending_var = arena_sprintf(gen->arena, "__%s_pending__", raw_var_name);
                return arena_sprintf(gen->arena,
                    "({\n"
                    "    if (%s != NULL) {\n"
                    "        RtHandleV2 *__sync_h__ = rt_thread_v3_sync(%s);\n"
                    "        rt_handle_begin_transaction(__sync_h__);\n"
                    "        %s = *(%s *)__sync_h__->ptr;\n"
                    "        rt_handle_end_transaction(__sync_h__);\n"
                    "        %s = NULL;\n"
                    "    }\n"
                    "    %s;\n"
                    "})",
                    pending_var,
                    pending_var,
                    var_name, c_type,
                    pending_var,
                    var_name);
            }
            else
            {
                return arena_sprintf(gen->arena,
                    "({ RtHandleV2 *__sync_h__ = rt_thread_v3_sync(%s); rt_handle_begin_transaction(__sync_h__); %s __sync_val__ = *(%s *)__sync_h__->ptr; rt_handle_end_transaction(__sync_h__); __sync_val__; })",
                    handle_code, c_type, c_type);
            }
        }
        else
        {
            /* Reference type without arena mode: V3 sync returns result directly */
            if (is_variable_handle)
            {
                char *raw_var_name = get_var_name(gen->arena, sync->handle->as.variable.name);
                char *var_name = sn_mangle_name(gen->arena, raw_var_name);
                char *pending_var = arena_sprintf(gen->arena, "__%s_pending__", raw_var_name);
                return arena_sprintf(gen->arena,
                    "({\n"
                    "    if (%s != NULL) {\n"
                    "        %s = (%s)rt_thread_v3_sync(%s);\n"
                    "        %s = NULL;\n"
                    "    }\n"
                    "    %s;\n"
                    "})",
                    pending_var,
                    var_name, c_type, pending_var,
                    pending_var,
                    var_name);
            }
            else
            {
                return arena_sprintf(gen->arena,
                    "((%s)rt_thread_v3_sync(%s))",
                    c_type, handle_code);
            }
        }
    }
}
