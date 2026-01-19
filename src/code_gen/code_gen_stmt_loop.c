#include "code_gen/code_gen_stmt_loop.h"
#include "code_gen/code_gen_stmt.h"
#include "code_gen/code_gen_expr.h"
#include "code_gen/code_gen_util.h"
#include "debug.h"
#include "symbol_table.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* Push a loop arena onto the stack when entering a loop with per-iteration arena */
void push_loop_arena(CodeGen *gen, char *arena_var, char *cleanup_label)
{
    if (gen->loop_arena_depth >= gen->loop_arena_capacity)
    {
        int new_capacity = gen->loop_arena_capacity == 0 ? 8 : gen->loop_arena_capacity * 2;
        char **new_arena_stack = arena_alloc(gen->arena, new_capacity * sizeof(char *));
        char **new_cleanup_stack = arena_alloc(gen->arena, new_capacity * sizeof(char *));
        for (int i = 0; i < gen->loop_arena_depth; i++)
        {
            new_arena_stack[i] = gen->loop_arena_stack[i];
            new_cleanup_stack[i] = gen->loop_cleanup_stack[i];
        }
        gen->loop_arena_stack = new_arena_stack;
        gen->loop_cleanup_stack = new_cleanup_stack;
        gen->loop_arena_capacity = new_capacity;
    }
    gen->loop_arena_stack[gen->loop_arena_depth] = arena_var;
    gen->loop_cleanup_stack[gen->loop_arena_depth] = cleanup_label;
    gen->loop_arena_depth++;

    /* Update current loop arena vars */
    gen->loop_arena_var = arena_var;
    gen->loop_cleanup_label = cleanup_label;
}

/* Pop a loop arena from the stack when exiting a loop */
void pop_loop_arena(CodeGen *gen)
{
    if (gen->loop_arena_depth > 0)
    {
        gen->loop_arena_depth--;
        if (gen->loop_arena_depth > 0)
        {
            /* Restore to the enclosing loop's arena */
            gen->loop_arena_var = gen->loop_arena_stack[gen->loop_arena_depth - 1];
            gen->loop_cleanup_label = gen->loop_cleanup_stack[gen->loop_arena_depth - 1];
        }
        else
        {
            /* No more enclosing loops */
            gen->loop_arena_var = NULL;
            gen->loop_cleanup_label = NULL;
        }
    }
}

void code_gen_while_statement(CodeGen *gen, WhileStmt *stmt, int indent)
{
    DEBUG_VERBOSE("Entering code_gen_while_statement");

    bool old_in_shared_context = gen->in_shared_context;
    char *old_current_arena_var = gen->current_arena_var;

    bool is_shared = stmt->is_shared;
    // Don't create loop arena if: loop is shared, OR we're inside a shared context
    bool needs_loop_arena = !is_shared && !gen->in_shared_context && gen->current_arena_var != NULL;

    // Shared loops don't create per-iteration arenas
    if (is_shared)
    {
        gen->in_shared_context = true;
    }

    // For non-shared loops with arena context (and not inside shared block), create per-iteration arena
    char *loop_arena = NULL;
    char *loop_cleanup = NULL;
    if (needs_loop_arena)
    {
        int label_num = code_gen_new_label(gen);
        loop_arena = arena_sprintf(gen->arena, "__loop_arena_%d__", label_num);
        loop_cleanup = arena_sprintf(gen->arena, "__loop_cleanup_%d__", label_num);
        push_loop_arena(gen, loop_arena, loop_cleanup);
    }

    char *cond_str = code_gen_expression(gen, stmt->condition);
    indented_fprintf(gen, indent, "while (%s) {\n", cond_str);

    // Create per-iteration arena at start of loop body
    if (needs_loop_arena)
    {
        indented_fprintf(gen, indent + 1, "RtArena *%s = rt_arena_create(%s);\n",
                         loop_arena, ARENA_VAR(gen));
        // Switch to using the loop arena for allocations inside the loop body
        gen->current_arena_var = loop_arena;
    }

    code_gen_statement(gen, stmt->body, indent + 1);

    // Restore parent arena before cleanup (for cleanup label context)
    if (needs_loop_arena)
    {
        gen->current_arena_var = old_current_arena_var;
    }

    // Cleanup label and arena destruction
    if (needs_loop_arena)
    {
        indented_fprintf(gen, indent, "%s:\n", loop_cleanup);
        indented_fprintf(gen, indent + 1, "rt_arena_destroy(%s);\n", loop_arena);
        pop_loop_arena(gen);
    }

    indented_fprintf(gen, indent, "}\n");

    gen->in_shared_context = old_in_shared_context;
}

void code_gen_for_statement(CodeGen *gen, ForStmt *stmt, int indent)
{
    DEBUG_VERBOSE("Entering code_gen_for_statement");

    bool old_in_shared_context = gen->in_shared_context;
    char *old_current_arena_var = gen->current_arena_var;

    bool is_shared = stmt->is_shared;
    // Don't create loop arena if: loop is shared, OR we're inside a shared context
    bool needs_loop_arena = !is_shared && !gen->in_shared_context && gen->current_arena_var != NULL;

    // Shared loops don't create per-iteration arenas
    if (is_shared)
    {
        gen->in_shared_context = true;
    }

    // For non-shared loops with arena context (and not inside shared block), create per-iteration arena
    char *loop_arena = NULL;
    char *loop_cleanup = NULL;
    if (needs_loop_arena)
    {
        int arena_label_num = code_gen_new_label(gen);
        loop_arena = arena_sprintf(gen->arena, "__loop_arena_%d__", arena_label_num);
        loop_cleanup = arena_sprintf(gen->arena, "__loop_cleanup_%d__", arena_label_num);
        push_loop_arena(gen, loop_arena, loop_cleanup);
    }

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

    // Save old continue label and create new one for this for loop
    char *old_continue_label = gen->for_continue_label;
    int label_num = code_gen_new_label(gen);
    char *continue_label = arena_sprintf(gen->arena, "__for_continue_%d__", label_num);
    gen->for_continue_label = continue_label;

    indented_fprintf(gen, indent + 1, "while (%s) {\n", cond_str ? cond_str : "1");

    // Create per-iteration arena at start of loop body
    if (needs_loop_arena)
    {
        indented_fprintf(gen, indent + 2, "RtArena *%s = rt_arena_create(%s);\n",
                         loop_arena, ARENA_VAR(gen));
        // Switch to using the loop arena for allocations inside the loop body
        gen->current_arena_var = loop_arena;
    }

    code_gen_statement(gen, stmt->body, indent + 2);

    // Restore parent arena before cleanup
    if (needs_loop_arena)
    {
        gen->current_arena_var = old_current_arena_var;
    }

    // Cleanup label and arena destruction (before increment)
    if (needs_loop_arena)
    {
        indented_fprintf(gen, indent + 1, "%s:\n", loop_cleanup);
        indented_fprintf(gen, indent + 2, "rt_arena_destroy(%s);\n", loop_arena);
        pop_loop_arena(gen);
    }

    // Generate continue label before increment
    indented_fprintf(gen, indent + 1, "%s:;\n", continue_label);

    if (stmt->increment)
    {
        char *inc_str = code_gen_expression(gen, stmt->increment);
        indented_fprintf(gen, indent + 2, "%s;\n", inc_str);
    }
    indented_fprintf(gen, indent + 1, "}\n");

    // Restore old continue label
    gen->for_continue_label = old_continue_label;

    code_gen_free_locals(gen, gen->symbol_table->current, false, indent + 1);
    indented_fprintf(gen, indent, "}\n");

    /* Pop loop counter if we were tracking one */
    if (tracking_loop_counter)
    {
        pop_loop_counter(gen);
    }

    symbol_table_pop_scope(gen->symbol_table);

    gen->in_shared_context = old_in_shared_context;
}

void code_gen_for_each_statement(CodeGen *gen, ForEachStmt *stmt, int indent)
{
    DEBUG_VERBOSE("Entering code_gen_for_each_statement");

    bool old_in_shared_context = gen->in_shared_context;

    bool is_shared = stmt->is_shared;
    // Don't create loop arena if: loop is shared, OR we're inside a shared context
    bool needs_loop_arena = !is_shared && !gen->in_shared_context && gen->current_arena_var != NULL;

    // Shared loops don't create per-iteration arenas
    if (is_shared)
    {
        gen->in_shared_context = true;
    }

    // For non-shared loops with arena context (and not inside shared block), create per-iteration arena
    char *loop_arena = NULL;
    char *loop_cleanup = NULL;
    if (needs_loop_arena)
    {
        int arena_label_num = code_gen_new_label(gen);
        loop_arena = arena_sprintf(gen->arena, "__loop_arena_%d__", arena_label_num);
        loop_cleanup = arena_sprintf(gen->arena, "__loop_cleanup_%d__", arena_label_num);
        push_loop_arena(gen, loop_arena, loop_cleanup);
    }

    // Generate a unique index variable name
    int temp_idx = gen->temp_count++;
    char *idx_var = arena_sprintf(gen->arena, "__idx_%d__", temp_idx);
    char *len_var = arena_sprintf(gen->arena, "__len_%d__", temp_idx);
    char *arr_var = arena_sprintf(gen->arena, "__arr_%d__", temp_idx);

    // Get the iterable expression
    char *iterable_str = code_gen_expression(gen, stmt->iterable);

    // Get the element type from the iterable's type
    Type *iterable_type = stmt->iterable->expr_type;
    Type *elem_type = iterable_type->as.array.element_type;
    const char *elem_c_type = get_c_type(gen->arena, elem_type);
    const char *arr_c_type = get_c_type(gen->arena, iterable_type);

    // Get the loop variable name
    char *var_name = get_var_name(gen->arena, stmt->var_name);

    // Create a new scope
    symbol_table_push_scope(gen->symbol_table);

    // Add the loop variable to the symbol table
    // Use SYMBOL_PARAM so it's not freed - loop var is a reference to array element, not owned
    symbol_table_add_symbol_with_kind(gen->symbol_table, stmt->var_name, elem_type, SYMBOL_PARAM);

    // Generate the for-each loop desugared to:
    // {
    //     arr_type __arr__ = iterable;
    //     long __len__ = rt_array_length(__arr__);
    //     for (long __idx__ = 0; __idx__ < __len__; __idx__++) {
    //         [RtArena *__loop_arena__ = rt_arena_create(parent);]  // if needs_loop_arena
    //         elem_type var = __arr__[__idx__];
    //         body
    //         [__loop_cleanup__: rt_arena_destroy(__loop_arena__);] // if needs_loop_arena
    //     }
    // }
    indented_fprintf(gen, indent, "{\n");
    indented_fprintf(gen, indent + 1, "%s %s = %s;\n", arr_c_type, arr_var, iterable_str);
    indented_fprintf(gen, indent + 1, "long %s = rt_array_length(%s);\n", len_var, arr_var);
    indented_fprintf(gen, indent + 1, "for (long %s = 0; %s < %s; %s++) {\n", idx_var, idx_var, len_var, idx_var);

    // Create per-iteration arena at start of loop body
    char *old_current_arena_var = gen->current_arena_var;
    if (needs_loop_arena)
    {
        indented_fprintf(gen, indent + 2, "RtArena *%s = rt_arena_create(%s);\n",
                         loop_arena, ARENA_VAR(gen));
        // Switch to using the loop arena for allocations inside the loop body
        gen->current_arena_var = loop_arena;
    }

    indented_fprintf(gen, indent + 2, "%s %s = %s[%s];\n", elem_c_type, var_name, arr_var, idx_var);

    // Generate the body
    code_gen_statement(gen, stmt->body, indent + 2);

    // Restore parent arena before cleanup
    if (needs_loop_arena)
    {
        gen->current_arena_var = old_current_arena_var;
    }

    // Cleanup label and arena destruction
    if (needs_loop_arena)
    {
        indented_fprintf(gen, indent + 1, "%s:\n", loop_cleanup);
        indented_fprintf(gen, indent + 2, "rt_arena_destroy(%s);\n", loop_arena);
        pop_loop_arena(gen);
    }

    indented_fprintf(gen, indent + 1, "}\n");
    code_gen_free_locals(gen, gen->symbol_table->current, false, indent + 1);
    indented_fprintf(gen, indent, "}\n");

    gen->in_shared_context = old_in_shared_context;

    symbol_table_pop_scope(gen->symbol_table);
}

/* Push a loop counter variable name onto the tracking stack.
 * Loop counters (like for-each __idx__ vars or C-style for loop vars starting at 0)
 * are provably non-negative and can skip negative index checks. */
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

/* Pop a loop counter variable name from the tracking stack. */
void pop_loop_counter(CodeGen *gen)
{
    if (gen->loop_counter_count > 0)
    {
        gen->loop_counter_count--;
    }
}

/* Check if a variable name is a tracked loop counter (provably non-negative). */
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
