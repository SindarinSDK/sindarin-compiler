/* ============================================================================
 * code_gen_util_temps.c - Arena Temp Handle Tracking
 * ============================================================================
 * Manages temporary RtHandleV2* variables created during expression evaluation.
 * Temps are hoisted to named variables, tracked, and freed after the containing
 * statement completes — unless a consumer (var decl, return, assignment) adopts them.
 * ============================================================================ */

#include "code_gen/util/code_gen_util.h"
#include <string.h>
#include <stdlib.h>

/* Emit a pre-declaration for a temporary handle variable and track it.
 * Returns the temp variable name (e.g. "__htmp_0__").
 * The caller's expression string is assigned to the temp. */
char *code_gen_emit_arena_temp(CodeGen *gen, const char *expr_str)
{
    /* At global scope (no current function), can't emit pre-declarations.
     * Return the expression inline — it'll be used in deferred init or file-scope. */
    if (gen->current_function == NULL)
    {
        return arena_sprintf(gen->arena, "%s", expr_str);
    }

    /* Allocate unique name */
    char *name = arena_sprintf(gen->arena, "__htmp_%d__", gen->arena_temp_serial++);

    /* Emit pre-declaration at current indent level */
    indented_fprintf(gen, gen->current_indent, "RtHandleV2 *%s = %s;\n", name, expr_str);

    /* Grow tracking array if needed */
    if (gen->arena_temp_count >= gen->arena_temp_capacity)
    {
        int new_cap = gen->arena_temp_capacity == 0 ? 8 : gen->arena_temp_capacity * 2;
        const char **new_arr = arena_alloc(gen->arena, new_cap * sizeof(const char *));
        for (int i = 0; i < gen->arena_temp_count; i++)
        {
            new_arr[i] = gen->arena_temps[i];
        }
        gen->arena_temps = new_arr;
        gen->arena_temp_capacity = new_cap;
    }
    gen->arena_temps[gen->arena_temp_count++] = name;

    return name;
}

/* Free all tracked arena temps. Called after a statement completes. */
void code_gen_flush_arena_temps(CodeGen *gen, int indent)
{
    if (gen->current_arena_var == NULL) return;

    for (int i = 0; i < gen->arena_temp_count; i++)
    {
        indented_fprintf(gen, indent, "rt_arena_v2_free(%s);\n", gen->arena_temps[i]);
    }
    gen->arena_temp_count = 0;
}

/* Remove temps from saved_count onwards — they are adopted by a consumer
 * (var decl, return, assignment) and must NOT be freed. */
void code_gen_adopt_arena_temps_from(CodeGen *gen, int saved_count)
{
    gen->arena_temp_count = saved_count;
}
