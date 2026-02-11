/**
 * code_gen_expr_thread_sync.c - Thread sync expression code generation
 *
 * Contains code generation for var! sync expressions.
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
        /* Sync list: [r1, r2, r3]!
         * This generates code to:
         * 1. Build an array of RtThreadHandle* pointers
         * 2. Call rt_thread_sync_all to sync all handles
         * 3. Return void
         */
        DEBUG_VERBOSE("Thread sync: sync list");

        /* The handle expression should be a sync list expression */
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
            /* Empty sync list - no-op */
            return arena_strdup(gen->arena, "((void)0)");
        }

        /* Generate a unique temp ID for the handles array */
        int temp_id = gen->temp_count++;

        /* Build the array initializer with handle expressions */
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

        /* Generate the sync_all call with inline array (V2 API)
         * Result:
         * ({
         *     RtThread *__sync_handles_N__[] = {r1, r2, r3};
         *     rt_thread_v2_sync_all(__sync_handles_N__, count);
         *     (void)0;
         * })
         */
        return arena_sprintf(gen->arena,
            "({\n"
            "    RtThread *__sync_handles_%d__[] = {%s};\n"
            "    rt_thread_v2_sync_all(__sync_handles_%d__, %d);\n"
            "    (void)0;\n"
            "})",
            temp_id, handles_init,
            temp_id, count);
    }
    else
    {
        /* Single variable sync: r!
         * This generates code to:
         * 1. Call rt_thread_join to wait for thread and get result pointer
         * 2. Cast the result to the correct type
         * 3. Dereference for primitives, return pointer for reference types
         */
        DEBUG_VERBOSE("Thread sync: single variable sync");

        /* Generate the handle expression */
        char *handle_code = code_gen_expression(gen, sync->handle);

        /* Get the result type from the sync expression */
        Type *result_type = expr->expr_type;

        /* Check if the result is void - no value to retrieve */
        if (result_type == NULL || result_type->kind == TYPE_VOID)
        {
            /* Void sync - V2: call rt_thread_v2_sync for synchronization with panic propagation */
            return arena_sprintf(gen->arena,
                "({\n"
                "    rt_thread_v2_sync(%s);\n"
                "    (void)0;\n"
                "})",
                handle_code);
        }

        /* Get the C type for casting */
        const char *c_type = get_c_type(gen->arena, result_type);

        /* V2: RtResultType no longer needed - result is always RtHandleV2* */

        /* Determine if this is a primitive type (needs dereferencing) or reference type */
        bool is_primitive = (result_type->kind == TYPE_INT ||
                             result_type->kind == TYPE_LONG ||
                             result_type->kind == TYPE_DOUBLE ||
                             result_type->kind == TYPE_BOOL ||
                             result_type->kind == TYPE_BYTE ||
                             result_type->kind == TYPE_CHAR);
        /* Handle types (array/string in arena mode) also use the pending var pattern
         * because RtHandle (uint32_t) can't hold a RtThread pointer */
        bool is_handle_type = gen->current_arena_var != NULL &&
                              (result_type->kind == TYPE_STRING || result_type->kind == TYPE_ARRAY);
        /* Struct types also need dereferencing and use the pending var pattern */
        bool is_struct_type = (result_type->kind == TYPE_STRUCT);
        /* Check if struct has handle fields that need promotion */
        bool struct_needs_field_promotion = is_struct_type && struct_has_handle_fields(result_type);
        /* Check if array needs deep promotion (2D/3D arrays or string arrays) */
        bool array_needs_deep_promotion = is_handle_type && result_type->kind == TYPE_ARRAY &&
            result_type->as.array.element_type != NULL &&
            (result_type->as.array.element_type->kind == TYPE_ARRAY ||
             result_type->as.array.element_type->kind == TYPE_STRING);

        /* Check if the handle is a variable - if so, we need to update it after sync
         * This ensures that after x! is used, subsequent uses of x return the synced value */
        bool is_variable_handle = (sync->handle->type == EXPR_VARIABLE);

        if (array_needs_deep_promotion)
        {
            /* Arrays that need deep promotion (2D/3D arrays, string arrays):
             * Use sync_keep_arena to get result without destroying thread arena,
             * then deep promote the array and destroy the arena. */
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

            if (is_variable_handle)
            {
                char *raw_var_name = get_var_name(gen->arena, sync->handle->as.variable.name);
                char *var_name = sn_mangle_name(gen->arena, raw_var_name);
                char *pending_var = arena_sprintf(gen->arena, "__%s_pending__", raw_var_name);
                return arena_sprintf(gen->arena,
                    "({\n"
                    "    if (%s != NULL) {\n"
                    "        /* Save thread arena reference before sync */\n"
                    "        RtArenaV2 *__thread_arena__ = %s->arena;\n"
                    "        /* V2: Get array from thread result - arena NOT destroyed yet */\n"
                    "        RtHandleV2 *__sync_tmp__ = rt_thread_v2_sync_keep_arena(%s);\n"
                    "        /* Deep promote array from thread arena to caller arena */\n"
                    "        %s = (%s)%s(%s, __sync_tmp__);\n"
                    "        /* Now destroy the thread arena */\n"
                    "        if (__thread_arena__ != NULL) rt_arena_v2_destroy(__thread_arena__);\n"
                    "        %s = NULL;\n"
                    "    }\n"
                    "    %s;\n"
                    "})",
                    pending_var,
                    pending_var,
                    pending_var,
                    var_name, c_type, promo_func, ARENA_VAR(gen),
                    pending_var,
                    var_name);
            }
            else
            {
                /* Non-variable inline spawn with array */
                return arena_sprintf(gen->arena,
                    "({\n"
                    "    RtThread *__sync_handle__ = %s;\n"
                    "    /* Save thread arena reference before sync */\n"
                    "    RtArenaV2 *__thread_arena__ = __sync_handle__->arena;\n"
                    "    /* V2: Get array from thread result - arena NOT destroyed yet */\n"
                    "    RtHandleV2 *__sync_tmp__ = rt_thread_v2_sync_keep_arena(__sync_handle__);\n"
                    "    /* Deep promote array from thread arena to caller arena */\n"
                    "    %s __result__ = (%s)%s(%s, __sync_tmp__);\n"
                    "    /* Now destroy the thread arena */\n"
                    "    if (__thread_arena__ != NULL) rt_arena_v2_destroy(__thread_arena__);\n"
                    "    __result__;\n"
                    "})",
                    handle_code,
                    c_type, c_type, promo_func, ARENA_VAR(gen));
            }
        }
        else if (is_handle_type)
        {
            /* Handle types (string/simple array in arena mode): thread stored result directly,
             * sync returns the promoted handle which we assign directly (no dereferencing) */
            if (is_variable_handle)
            {
                char *raw_var_name = get_var_name(gen->arena, sync->handle->as.variable.name);
                char *var_name = sn_mangle_name(gen->arena, raw_var_name);
                char *pending_var = arena_sprintf(gen->arena, "__%s_pending__", raw_var_name);
                return arena_sprintf(gen->arena,
                    "({\n"
                    "    if (%s != NULL) {\n"
                    "        %s = (%s)rt_thread_v2_sync(%s);\n"
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
                /* Non-variable (e.g., inline spawn): just return the value */
                return arena_sprintf(gen->arena,
                    "((%s)rt_thread_v2_sync(%s))",
                    c_type, handle_code);
            }
        }
        else if (is_primitive || (is_struct_type && !struct_needs_field_promotion))
        {
            /* Primitive/simple struct type: V2 sync returns RtHandleV2*, pin and dereference.
             * For variable handles, we have TWO variables: __var_pending__ (RtThread*)
             * and var (actual type). Sync uses the pending handle, assigns to the typed var. */
            if (is_variable_handle)
            {
                /* For primitive thread spawn variables, we declared:
                 *   RtThread *__var_pending__ = NULL;  // or = &fn() if initialized with spawn
                 *   type var = initial_value;  // or uninitialized if spawn
                 * Now sync using __var_pending__ if it's not NULL, and assign result to var.
                 * If __var_pending__ is NULL, the variable was never assigned a thread spawn,
                 * so just return the current value.
                 * Pattern:
                 * ({
                 *     if (__var_pending__ != NULL) {
                 *         RtHandleV2 *__sync_h__ = rt_thread_v2_sync(__var_pending__);
                 *         rt_handle_v2_pin(__sync_h__);
                 *         var = *(type*)__sync_h__->ptr;
                 *         __var_pending__ = NULL;
                 *     }
                 *     var;
                 * }) */
                char *raw_var_name = get_var_name(gen->arena, sync->handle->as.variable.name);
                char *var_name = sn_mangle_name(gen->arena, raw_var_name);
                char *pending_var = arena_sprintf(gen->arena, "__%s_pending__", raw_var_name);
                return arena_sprintf(gen->arena,
                    "({\n"
                    "    if (%s != NULL) {\n"
                    "        RtHandleV2 *__sync_h__ = rt_thread_v2_sync(%s);\n"
                    "        rt_handle_v2_pin(__sync_h__);\n"
                    "        %s = *(%s *)__sync_h__->ptr;\n"
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
                /* Non-variable (e.g., inline spawn): just return the value */
                return arena_sprintf(gen->arena,
                    "({ RtHandleV2 *__sync_h__ = rt_thread_v2_sync(%s); rt_handle_v2_pin(__sync_h__); *(%s *)__sync_h__->ptr; })",
                    handle_code, c_type);
            }
        }
        else if (struct_needs_field_promotion)
        {
            /* Struct with handle fields: V2 - sync without destroying arena,
             * copy struct, promote each field, then destroy the arena.
             * We use rt_thread_v2_sync_keep_arena which:
             * 1. Syncs the thread and gets the result (promoted to caller arena)
             * 2. Does NOT destroy the thread arena (so we can promote handle fields)
             * Then we promote fields and call rt_arena_v2_destroy.
             */
            char *promo_code = gen_struct_field_promotion(gen, result_type, "__sync_tmp__",
                                                           ARENA_VAR(gen), "__thread_arena__");

            if (is_variable_handle)
            {
                char *raw_var_name = get_var_name(gen->arena, sync->handle->as.variable.name);
                char *var_name = sn_mangle_name(gen->arena, raw_var_name);
                char *pending_var = arena_sprintf(gen->arena, "__%s_pending__", raw_var_name);

                return arena_sprintf(gen->arena,
                    "({\n"
                    "    if (%s != NULL) {\n"
                    "        /* Save thread arena reference before sync */\n"
                    "        RtArenaV2 *__thread_arena__ = %s->arena;\n"
                    "        /* V2: Get struct from thread result - arena NOT destroyed yet */\n"
                    "        RtHandleV2 *__sync_h__ = rt_thread_v2_sync_keep_arena(%s);\n"
                    "        rt_handle_v2_pin(__sync_h__);\n"
                    "        %s __sync_tmp__ = *(%s *)__sync_h__->ptr;\n"
                    "        /* Promote handle fields from thread arena to caller arena */\n"
                    "%s"
                    "        /* Now destroy the thread arena */\n"
                    "        if (__thread_arena__ != NULL) rt_arena_v2_destroy(__thread_arena__);\n"
                    "        %s = __sync_tmp__;\n"
                    "        %s = NULL;\n"
                    "    }\n"
                    "    %s;\n"
                    "})",
                    pending_var,
                    pending_var,
                    pending_var,
                    c_type, c_type,
                    promo_code,
                    var_name,
                    pending_var,
                    var_name);
            }
            else
            {
                /* Non-variable inline spawn with struct */
                return arena_sprintf(gen->arena,
                    "({\n"
                    "    RtThread *__sync_handle__ = %s;\n"
                    "    /* Save thread arena reference before sync */\n"
                    "    RtArenaV2 *__thread_arena__ = __sync_handle__->arena;\n"
                    "    /* V2: Get struct from thread result - arena NOT destroyed yet */\n"
                    "    RtHandleV2 *__sync_h__ = rt_thread_v2_sync_keep_arena(__sync_handle__);\n"
                    "    rt_handle_v2_pin(__sync_h__);\n"
                    "    %s __sync_tmp__ = *(%s *)__sync_h__->ptr;\n"
                    "    /* Promote handle fields from thread arena to caller arena */\n"
                    "%s"
                    "    /* Now destroy the thread arena */\n"
                    "    if (__thread_arena__ != NULL) rt_arena_v2_destroy(__thread_arena__);\n"
                    "    __sync_tmp__;\n"
                    "})",
                    handle_code,
                    c_type, c_type,
                    promo_code);
            }
        }
        else
        {
            /* Reference type (string, array, etc.): V2 sync returns RtHandleV2* directly */
            if (is_variable_handle)
            {
                /* Update variable and return value */
                char *raw_var_name = get_var_name(gen->arena, sync->handle->as.variable.name);
                char *var_name = sn_mangle_name(gen->arena, raw_var_name);
                char *pending_var = arena_sprintf(gen->arena, "__%s_pending__", raw_var_name);
                return arena_sprintf(gen->arena,
                    "({\n"
                    "    if (%s != NULL) {\n"
                    "        %s = (%s)rt_thread_v2_sync(%s);\n"
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
                /* Non-variable: just return the value */
                return arena_sprintf(gen->arena,
                    "((%s)rt_thread_v2_sync(%s))",
                    c_type, handle_code);
            }
        }
    }
}
