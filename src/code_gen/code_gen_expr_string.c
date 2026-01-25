/* ============================================================================
 * code_gen_expr_string.c - String Interpolation Code Generation
 * ============================================================================
 * Generates C code for string interpolation expressions.
 * Extracted from code_gen_expr.c for modularity.
 * ============================================================================ */

#include "code_gen/code_gen_expr_string.h"
#include "code_gen/code_gen_util.h"
#include "code_gen/code_gen_expr.h"
#include "debug.h"
#include <stdbool.h>

/* ============================================================================
 * Helper Functions
 * ============================================================================ */

/* Check if an expression is a string literal - can be used directly without copying */
static bool is_string_literal_expr(Expr *expr)
{
    if (expr == NULL) return false;
    if (expr->type != EXPR_LITERAL) return false;
    if (expr->expr_type == NULL) return false;
    return expr->expr_type->kind == TYPE_STRING;
}

/* Get the runtime format function for a type */
static const char *get_rt_format_func(TypeKind kind)
{
    switch (kind)
    {
    case TYPE_INT:
    case TYPE_LONG:
        return "rt_format_long";
    case TYPE_DOUBLE:
        return "rt_format_double";
    case TYPE_STRING:
        return "rt_format_string";
    default:
        return NULL;  /* No format function for this type */
    }
}

/* Check if any part has a format specifier */
static bool has_any_format_spec(InterpolExpr *expr)
{
    if (expr->format_specs == NULL) return false;
    for (int i = 0; i < expr->part_count; i++)
    {
        if (expr->format_specs[i] != NULL) return true;
    }
    return false;
}

/* ============================================================================
 * Interpolated Expression Code Generation
 * ============================================================================ */

char *code_gen_interpolated_expression(CodeGen *gen, InterpolExpr *expr)
{
    DEBUG_VERBOSE("Entering code_gen_interpolated_expression");
    int count = expr->part_count;
    if (count == 0)
    {
        /* Empty interpolation - return empty string literal, wrapped in handle if needed */
        if (gen->expr_as_handle && gen->current_arena_var != NULL)
        {
            return arena_sprintf(gen->arena, "rt_managed_strdup(%s, RT_HANDLE_NULL, \"\")", ARENA_VAR(gen));
        }
        return "\"\"";
    }

    /* Gather information about each part */
    char **part_strs = arena_alloc(gen->arena, count * sizeof(char *));
    Type **part_types = arena_alloc(gen->arena, count * sizeof(Type *));
    bool *is_literal = arena_alloc(gen->arena, count * sizeof(bool));
    bool *is_temp = arena_alloc(gen->arena, count * sizeof(bool));

    int needs_conversion_count = 0;
    bool uses_format_specs = has_any_format_spec(expr);

    /* Parts are always evaluated in raw-pointer mode (expr_as_handle=false)
     * because the intermediate rt_str_concat calls need char* arguments.
     * The final result is wrapped in a handle if handle_mode is active. */
    bool saved_as_handle = gen->expr_as_handle;
    gen->expr_as_handle = false;

    for (int i = 0; i < count; i++)
    {
        part_strs[i] = code_gen_expression(gen, expr->parts[i]);
        part_types[i] = expr->parts[i]->expr_type;
        is_literal[i] = is_string_literal_expr(expr->parts[i]);
        is_temp[i] = expression_produces_temp(expr->parts[i]);

        if (part_types[i]->kind != TYPE_STRING) needs_conversion_count++;
    }

    gen->expr_as_handle = saved_as_handle;

    /* Optimization: Single string literal without format - use directly */
    if (count == 1 && is_literal[0] && !uses_format_specs)
    {
        if (gen->expr_as_handle && gen->current_arena_var != NULL)
        {
            return arena_sprintf(gen->arena, "rt_managed_strdup(%s, RT_HANDLE_NULL, %s)", ARENA_VAR(gen), part_strs[0]);
        }
        return part_strs[0];
    }

    /* Optimization: Single string variable/temp without format - return as is or copy */
    if (count == 1 && part_types[0]->kind == TYPE_STRING && !uses_format_specs)
    {
        if (gen->expr_as_handle && gen->current_arena_var != NULL)
        {
            /* In handle mode: create a new handle from the string value */
            return arena_sprintf(gen->arena, "rt_managed_strdup(%s, RT_HANDLE_NULL, %s)", ARENA_VAR(gen), part_strs[0]);
        }
        if (is_temp[0])
        {
            return part_strs[0];
        }
        else if (is_literal[0])
        {
            return part_strs[0];
        }
        else
        {
            /* Variable - already pinned by variable expression, return as is */
            return part_strs[0];
        }
    }

    /* Optimization: Two string literals or all literals without format - simple concat */
    if (count == 2 && needs_conversion_count == 0 && !is_temp[0] && !is_temp[1] && !uses_format_specs)
    {
        if (gen->expr_as_handle && gen->current_arena_var != NULL)
        {
            return arena_sprintf(gen->arena, "rt_str_concat_h(%s, RT_HANDLE_NULL, %s, %s)", ARENA_VAR(gen), part_strs[0], part_strs[1]);
        }
        return arena_sprintf(gen->arena, "rt_str_concat(%s, %s, %s)", ARENA_VAR(gen), part_strs[0], part_strs[1]);
    }

    /* General case: Need to build a block expression */
    char *result = arena_strdup(gen->arena, "({\n");

    /* Track which parts need temp variables and which can be used directly */
    char **use_strs = arena_alloc(gen->arena, count * sizeof(char *));
    int temp_var_count = 0;

    /* First pass: convert non-strings and capture temps, handle format specifiers */
    for (int i = 0; i < count; i++)
    {
        char *format_spec = (expr->format_specs != NULL) ? expr->format_specs[i] : NULL;

        if (format_spec != NULL)
        {
            /* Has format specifier - use rt_format_* functions */
            const char *format_func = get_rt_format_func(part_types[i]->kind);
            if (format_func != NULL)
            {
                result = arena_sprintf(gen->arena, "%s        char *_p%d = %s(%s, %s, \"%s\");\n",
                                       result, temp_var_count, format_func, ARENA_VAR(gen), part_strs[i], format_spec);
            }
            else
            {
                /* Fallback: convert to string first, then format */
                const char *to_str = gen->current_arena_var
                    ? get_rt_to_string_func_for_type_h(part_types[i])
                    : get_rt_to_string_func_for_type(part_types[i]);
                result = arena_sprintf(gen->arena, "%s        char *_tmp%d = %s(%s, %s);\n",
                                       result, temp_var_count, to_str, ARENA_VAR(gen), part_strs[i]);
                result = arena_sprintf(gen->arena, "%s        char *_p%d = rt_format_string(%s, _tmp%d, \"%s\");\n",
                                       result, temp_var_count, ARENA_VAR(gen), temp_var_count, format_spec);
            }
            use_strs[i] = arena_sprintf(gen->arena, "_p%d", temp_var_count);
            temp_var_count++;
        }
        else if (part_types[i]->kind != TYPE_STRING)
        {
            /* Non-string needs conversion (no format specifier) */
            const char *to_str = gen->current_arena_var
                ? get_rt_to_string_func_for_type_h(part_types[i])
                : get_rt_to_string_func_for_type(part_types[i]);
            result = arena_sprintf(gen->arena, "%s        char *_p%d = %s(%s, %s);\n",
                                   result, temp_var_count, to_str, ARENA_VAR(gen), part_strs[i]);
            use_strs[i] = arena_sprintf(gen->arena, "_p%d", temp_var_count);
            temp_var_count++;
        }
        else if (is_temp[i])
        {
            /* Temp string - capture it */
            result = arena_sprintf(gen->arena, "%s        char *_p%d = %s;\n",
                                   result, temp_var_count, part_strs[i]);
            use_strs[i] = arena_sprintf(gen->arena, "_p%d", temp_var_count);
            temp_var_count++;
        }
        else if (is_literal[i])
        {
            /* String literal - use directly (no copy needed) */
            use_strs[i] = part_strs[i];
        }
        else
        {
            /* String variable - can use directly in concat (rt_str_concat handles it) */
            use_strs[i] = part_strs[i];
        }
    }

    /* Build concatenation chain */
    bool handle_mode = (gen->expr_as_handle && gen->current_arena_var != NULL);

    if (count == 1)
    {
        if (handle_mode)
        {
            result = arena_sprintf(gen->arena, "%s        rt_managed_strdup(%s, RT_HANDLE_NULL, %s);\n    })",
                                   result, ARENA_VAR(gen), use_strs[0]);
        }
        else
        {
            result = arena_sprintf(gen->arena, "%s        %s;\n    })", result, use_strs[0]);
        }
        return result;
    }
    else if (count == 2)
    {
        if (handle_mode)
        {
            result = arena_sprintf(gen->arena, "%s        rt_str_concat_h(%s, RT_HANDLE_NULL, %s, %s);\n    })",
                                   result, ARENA_VAR(gen), use_strs[0], use_strs[1]);
        }
        else
        {
            result = arena_sprintf(gen->arena, "%s        rt_str_concat(%s, %s, %s);\n    })",
                                   result, ARENA_VAR(gen), use_strs[0], use_strs[1]);
        }
        return result;
    }
    else
    {
        /* Chain of concats - use intermediate char* temps, convert final to handle if needed */
        result = arena_sprintf(gen->arena, "%s        char *_r = rt_str_concat(%s, %s, %s);\n",
                               result, ARENA_VAR(gen), use_strs[0], use_strs[1]);

        for (int i = 2; i < count; i++)
        {
            result = arena_sprintf(gen->arena, "%s        _r = rt_str_concat(%s, _r, %s);\n",
                                   result, ARENA_VAR(gen), use_strs[i]);
        }

        if (handle_mode)
        {
            result = arena_sprintf(gen->arena, "%s        rt_managed_strdup(%s, RT_HANDLE_NULL, _r);\n    })", result, ARENA_VAR(gen));
        }
        else
        {
            result = arena_sprintf(gen->arena, "%s        _r;\n    })", result);
        }
        return result;
    }
}
