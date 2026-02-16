#include "code_gen/stmt/code_gen_stmt_loop.h"
#include "code_gen/stmt/code_gen_stmt.h"
#include "code_gen/expr/code_gen_expr.h"
#include "code_gen/util/code_gen_util.h"
#include "debug.h"
#include "symbol_table.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* ============================================================================
 * Simplified Loop Code Generation
 * ============================================================================
 * All loops share the function's arena. No per-iteration arenas are created.
 * This simplifies handle management - handles are always in the function arena.
 * ============================================================================ */

void code_gen_while_statement(CodeGen *gen, WhileStmt *stmt, int indent)
{
    DEBUG_VERBOSE("Entering code_gen_while_statement");

    char *cond_str = code_gen_expression(gen, stmt->condition);
    indented_fprintf(gen, indent, "while (%s) {\n", cond_str);

    code_gen_statement(gen, stmt->body, indent + 1);

    indented_fprintf(gen, indent, "}\n");
}

void code_gen_for_statement(CodeGen *gen, ForStmt *stmt, int indent)
{
    DEBUG_VERBOSE("Entering code_gen_for_statement");

    symbol_table_push_scope(gen->symbol_table);
    indented_fprintf(gen, indent, "{\n");

    /* Track loop counter variable for optimization if initializer is a var declaration */
    bool tracking_loop_counter = false;
    if (stmt->initializer && stmt->initializer->type == STMT_VAR_DECL)
    {
        const char *var_name = stmt->initializer->as.var_decl.name.start;
        push_loop_counter(gen, var_name);
        tracking_loop_counter = true;
    }

    if (stmt->initializer)
    {
        code_gen_statement(gen, stmt->initializer, indent + 1);
    }
    char *cond_str = NULL;
    if (stmt->condition)
    {
        cond_str = code_gen_expression(gen, stmt->condition);
    }

    /* Save old continue label and create new one for this for loop */
    char *old_continue_label = gen->for_continue_label;
    int label_num = code_gen_new_label(gen);
    char *continue_label = arena_sprintf(gen->arena, "__for_continue_%d__", label_num);
    gen->for_continue_label = continue_label;

    indented_fprintf(gen, indent + 1, "while (%s) {\n", cond_str ? cond_str : "1");

    code_gen_statement(gen, stmt->body, indent + 2);

    /* Generate continue label before increment */
    indented_fprintf(gen, indent + 1, "%s:;\n", continue_label);

    if (stmt->increment)
    {
        char *inc_str = code_gen_expression(gen, stmt->increment);
        indented_fprintf(gen, indent + 2, "%s;\n", inc_str);
    }
    indented_fprintf(gen, indent + 1, "}\n");

    /* Restore old continue label */
    gen->for_continue_label = old_continue_label;

    code_gen_free_locals(gen, gen->symbol_table->current, false, indent + 1);
    indented_fprintf(gen, indent, "}\n");

    /* Pop loop counter if we were tracking one */
    if (tracking_loop_counter)
    {
        pop_loop_counter(gen);
    }

    symbol_table_pop_scope(gen->symbol_table);
}

void code_gen_for_each_statement(CodeGen *gen, ForEachStmt *stmt, int indent)
{
    DEBUG_VERBOSE("Entering code_gen_for_each_statement");

    /* Generate a unique index variable name */
    int temp_idx = gen->temp_count++;
    char *idx_var = arena_sprintf(gen->arena, "__idx_%d__", temp_idx);
    char *len_var = arena_sprintf(gen->arena, "__len_%d__", temp_idx);
    char *arr_var = arena_sprintf(gen->arena, "__arr_%d__", temp_idx);

    /* Get the element type from the iterable's type */
    Type *iterable_type = stmt->iterable->expr_type;
    Type *elem_type = iterable_type->as.array.element_type;
    const char *elem_c_type = get_c_array_elem_type(gen->arena, elem_type);
    /* Build pinned pointer type for the loop array variable (element_type *) */
    const char *arr_c_type = arena_sprintf(gen->arena, "%s *", elem_c_type);

    /* Get the loop variable name */
    char *var_name = sn_mangle_name(gen->arena, get_var_name(gen->arena, stmt->var_name));

    /* Create a new scope */
    symbol_table_push_scope(gen->symbol_table);

    /* Add the loop variable to the symbol table */
    symbol_table_add_symbol_with_kind(gen->symbol_table, stmt->var_name, elem_type, SYMBOL_LOCAL);

    indented_fprintf(gen, indent, "{\n");

    /* In V2 mode, get handle first for length, then use rt_array_data_v2 for data pointer */
    if (gen->current_arena_var != NULL)
    {
        /* Get the iterable as handle */
        bool prev_as_handle = gen->expr_as_handle;
        gen->expr_as_handle = true;
        char *handle_str = code_gen_expression(gen, stmt->iterable);
        gen->expr_as_handle = prev_as_handle;

        /* Generate V2 for-each loop with per-element transaction:
         * {
         *     RtHandleV2 *__handle__ = iterable;
         *     long __len__ = rt_array_length_v2(__handle__);
         *     arr_type __arr__;
         *     for (long __idx__ = 0; __idx__ < __len__; __idx__++) {
         *         rt_handle_begin_transaction(__handle__);
         *         __arr__ = (arr_type)rt_array_data_v2(__handle__);
         *         elem_type var = __arr__[__idx__];
         *         rt_handle_end_transaction(__handle__);
         *         body
         *     }
         * }
         *
         * Transaction is acquired only to extract the element, then released
         * BEFORE the loop body executes. This prevents deadlocks when the
         * body contains blocking operations (e.g., thread sync). */
        char *handle_var = arena_sprintf(gen->arena, "__handle_%d__", temp_idx);
        indented_fprintf(gen, indent + 1, "RtHandleV2 *%s = %s;\n", handle_var, handle_str);
        indented_fprintf(gen, indent + 1, "long %s = rt_array_length_v2(%s);\n", len_var, handle_var);
        indented_fprintf(gen, indent + 1, "%s %s;\n", arr_c_type, arr_var);
    }
    else
    {
        /* Get the iterable expression - evaluate as raw pointer (pinned form) */
        bool prev_as_handle = gen->expr_as_handle;
        gen->expr_as_handle = false;
        char *iterable_str = code_gen_expression(gen, stmt->iterable);
        gen->expr_as_handle = prev_as_handle;

        /* Generate V1 for-each loop desugared to:
         * {
         *     arr_type __arr__ = iterable;
         *     long __len__ = rt_v2_data_array_length(__arr__);
         *     for (long __idx__ = 0; __idx__ < __len__; __idx__++) {
         *         elem_type var = __arr__[__idx__];
         *         body
         *     }
         * } */
        indented_fprintf(gen, indent + 1, "%s %s = %s;\n", arr_c_type, arr_var, iterable_str);
        indented_fprintf(gen, indent + 1, "long %s = rt_v2_data_array_length(%s);\n", len_var, arr_var);
    }
    indented_fprintf(gen, indent + 1, "for (long %s = 0; %s < %s; %s++) {\n", idx_var, idx_var, len_var, idx_var);

    /* In V2 mode, acquire transaction to extract element, then release before body */
    if (gen->current_arena_var != NULL)
    {
        indented_fprintf(gen, indent + 2, "rt_handle_begin_transaction(__handle_%d__);\n", temp_idx);
        indented_fprintf(gen, indent + 2, "%s = (%s)rt_array_data_v2(__handle_%d__);\n", arr_var, arr_c_type, temp_idx);
    }

    indented_fprintf(gen, indent + 2, "%s %s = %s[%s];\n", elem_c_type, var_name, arr_var, idx_var);

    if (gen->current_arena_var != NULL)
    {
        indented_fprintf(gen, indent + 2, "rt_handle_end_transaction(__handle_%d__);\n", temp_idx);
    }

    /* Generate the body (no transaction held - safe for blocking operations) */
    code_gen_statement(gen, stmt->body, indent + 2);

    indented_fprintf(gen, indent + 1, "}\n");
    code_gen_free_locals(gen, gen->symbol_table->current, false, indent + 1);
    indented_fprintf(gen, indent, "}\n");

    symbol_table_pop_scope(gen->symbol_table);
}

/* ============================================================================
 * Loop Counter Tracking (for optimization)
 * ============================================================================
 * Tracks variables known to be non-negative for index bounds check optimization.
 * ============================================================================ */

void push_loop_counter(CodeGen *gen, const char *var_name)
{
    /* Grow stack if needed */
    if (gen->loop_counter_count >= gen->loop_counter_capacity)
    {
        int new_cap = gen->loop_counter_capacity == 0 ? 8 : gen->loop_counter_capacity * 2;
        char **new_stack = arena_alloc(gen->arena, new_cap * sizeof(char *));
        for (int i = 0; i < gen->loop_counter_count; i++)
        {
            new_stack[i] = gen->loop_counter_names[i];
        }
        gen->loop_counter_names = new_stack;
        gen->loop_counter_capacity = new_cap;
    }
    gen->loop_counter_names[gen->loop_counter_count] = arena_strdup(gen->arena, var_name);
    gen->loop_counter_count++;
}

void pop_loop_counter(CodeGen *gen)
{
    if (gen->loop_counter_count > 0)
    {
        gen->loop_counter_count--;
    }
}

bool is_tracked_loop_counter(CodeGen *gen, const char *var_name)
{
    if (var_name == NULL)
    {
        return false;
    }
    for (int i = 0; i < gen->loop_counter_count; i++)
    {
        if (gen->loop_counter_names[i] != NULL &&
            strcmp(gen->loop_counter_names[i], var_name) == 0)
        {
            return true;
        }
    }
    return false;
}
