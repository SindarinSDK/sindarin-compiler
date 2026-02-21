/* ============================================================================
 * code_gen_expr_string.c - String Interpolation Code Generation
 * ============================================================================
 * Generates C code for string interpolation expressions.
 * Extracted from code_gen_expr.c for modularity.
 *
 * In arena mode (V2), rt_str_concat_v2 accepts RtHandleV2* parameters.
 * All string parts are evaluated in handle mode and passed directly.
 * Non-string parts are converted to handles via rt_to_string_*_v2 functions.
 * ============================================================================ */

#include "code_gen/expr/code_gen_expr_string.h"
#include "code_gen/util/code_gen_util.h"
#include "code_gen/expr/code_gen_expr.h"
#include "ast.h"
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

/* Get the runtime format function for a type (V1 - returns char*) */
static const char *get_rt_format_func(TypeKind kind)
{
    switch (kind)
    {
    case TYPE_INT:
    case TYPE_INT32:
    case TYPE_UINT:
    case TYPE_UINT32:
    case TYPE_LONG:
    case TYPE_BYTE:
        return "rt_format_long";
    case TYPE_DOUBLE:
    case TYPE_FLOAT:
        return "rt_format_double";
    case TYPE_STRING:
        return "rt_format_string";
    default:
        return NULL;  /* No format function for this type (CHAR, BOOL, etc.) */
    }
}

/* Get the runtime format function for a type (V2 - returns RtHandleV2*) */
static const char *get_rt_format_func_v2(TypeKind kind)
{
    switch (kind)
    {
    case TYPE_INT:
    case TYPE_INT32:
    case TYPE_UINT:
    case TYPE_UINT32:
    case TYPE_LONG:
    case TYPE_BYTE:
        return "rt_format_long_v2";
    case TYPE_DOUBLE:
    case TYPE_FLOAT:
        return "rt_format_double_v2";
    case TYPE_STRING:
        return "rt_format_string_v2";
    default:
        return NULL;
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

/* Generate auto-toString code for a struct without a toString() method.
 * Produces: "StructName { field1: value1, field2: value2, ... }"
 * Returns the generated code that evaluates to a char*.
 *
 * In arena mode (V2), rt_str_concat_v2 takes RtHandleV2* arguments.
 * We use handles throughout and only extract char* at the very end.
 */
static char *generate_struct_auto_tostring(CodeGen *gen, Type *struct_type, const char *value_expr, int *temp_counter)
{
    const char *struct_name = struct_type->as.struct_type.name ? struct_type->as.struct_type.name : "struct";
    int field_count = struct_type->as.struct_type.field_count;
    StructField *fields = struct_type->as.struct_type.fields;

    /* For native structs with c_alias that are passed by ref, access fields via -> */
    bool use_arrow = struct_type->as.struct_type.pass_self_by_ref ||
                     (struct_type->as.struct_type.is_native && struct_type->as.struct_type.c_alias != NULL);
    const char *accessor = use_arrow ? "->" : ".";

    /* Build concatenation chain using RtHandleV2* handles.
     * _auto_h is the running handle accumulator. */
    char *result = arena_sprintf(gen->arena,
        "({ RtHandleV2 *_auto_h%d; "
        "_auto_h%d = rt_str_concat_v2(%s, rt_arena_v2_strdup(%s, \"%s { \"), rt_arena_v2_strdup(%s, \"\")); ",
        *temp_counter,
        *temp_counter, ARENA_VAR(gen), ARENA_VAR(gen), struct_name, ARENA_VAR(gen));

    for (int i = 0; i < field_count; i++)
    {
        const char *field_name = fields[i].name;
        Type *field_type = fields[i].type;

        /* Get the C field name (use c_alias if present, otherwise mangle) */
        const char *c_field_name = fields[i].c_alias
            ? fields[i].c_alias
            : sn_mangle_name(gen->arena, field_name);

        /* Add field name */
        result = arena_sprintf(gen->arena,
            "%s_auto_h%d = rt_str_concat_v2(%s, _auto_h%d, rt_arena_v2_strdup(%s, \"%s: \")); ",
            result, *temp_counter, ARENA_VAR(gen), *temp_counter, ARENA_VAR(gen), field_name);

        /* Add field value based on type */
        const char *field_access = arena_sprintf(gen->arena, "(%s)%s%s", value_expr, accessor, c_field_name);

        if (field_type == NULL)
        {
            /* Unknown type - skip */
            result = arena_sprintf(gen->arena,
                "%s_auto_h%d = rt_str_concat_v2(%s, _auto_h%d, rt_arena_v2_strdup(%s, \"?\")); ",
                result, *temp_counter, ARENA_VAR(gen), *temp_counter, ARENA_VAR(gen));
        }
        else if (field_type->kind == TYPE_STRING)
        {
            /* String field - wrap in quotes. Field is already an RtHandleV2*. */
            result = arena_sprintf(gen->arena,
                "%s_auto_h%d = rt_str_concat_v2(%s, _auto_h%d, rt_arena_v2_strdup(%s, \"\\\"\")); "
                "_auto_h%d = rt_str_concat_v2(%s, _auto_h%d, %s ? %s : rt_arena_v2_strdup(%s, \"null\")); "
                "_auto_h%d = rt_str_concat_v2(%s, _auto_h%d, rt_arena_v2_strdup(%s, \"\\\"\")); ",
                result, *temp_counter, ARENA_VAR(gen), *temp_counter, ARENA_VAR(gen),
                *temp_counter, ARENA_VAR(gen), *temp_counter, field_access, field_access, ARENA_VAR(gen),
                *temp_counter, ARENA_VAR(gen), *temp_counter, ARENA_VAR(gen));
        }
        else if (field_type->kind == TYPE_CHAR)
        {
            /* Char field - wrap in single quotes */
            result = arena_sprintf(gen->arena,
                "%s_auto_h%d = rt_str_concat_v2(%s, _auto_h%d, rt_arena_v2_strdup(%s, \"'\")); "
                "_auto_h%d = rt_str_concat_v2(%s, _auto_h%d, rt_to_string_char_v2(%s, %s)); "
                "_auto_h%d = rt_str_concat_v2(%s, _auto_h%d, rt_arena_v2_strdup(%s, \"'\")); ",
                result, *temp_counter, ARENA_VAR(gen), *temp_counter, ARENA_VAR(gen),
                *temp_counter, ARENA_VAR(gen), *temp_counter, ARENA_VAR(gen), field_access,
                *temp_counter, ARENA_VAR(gen), *temp_counter, ARENA_VAR(gen));
        }
        else if (field_type->kind == TYPE_ANY)
        {
            /* TYPE_ANY: rt_any_to_string returns RtHandleV2* - pass directly */
            result = arena_sprintf(gen->arena,
                "%s_auto_h%d = rt_str_concat_v2(%s, _auto_h%d, rt_any_to_string(%s, %s)); ",
                result, *temp_counter, ARENA_VAR(gen), *temp_counter, ARENA_VAR(gen), field_access);
        }
        else if (field_type->kind == TYPE_ARRAY)
        {
            /* Array toString V2 takes 1 arg (handle), returns RtHandleV2* */
            const char *to_str_func = get_rt_to_string_func_for_type_v2(field_type);
            result = arena_sprintf(gen->arena,
                "%s_auto_h%d = rt_str_concat_v2(%s, _auto_h%d, %s(%s)); ",
                result, *temp_counter, ARENA_VAR(gen), *temp_counter, to_str_func, field_access);
        }
        else
        {
            /* Other types - use appropriate V2 to_string function (returns RtHandleV2*) */
            const char *to_str_func = get_rt_to_string_func_for_type_v2(field_type);
            result = arena_sprintf(gen->arena,
                "%s_auto_h%d = rt_str_concat_v2(%s, _auto_h%d, %s(%s, %s)); ",
                result, *temp_counter, ARENA_VAR(gen), *temp_counter, to_str_func, ARENA_VAR(gen), field_access);
        }

        /* Add separator (", " or " }") */
        if (i < field_count - 1)
        {
            result = arena_sprintf(gen->arena,
                "%s_auto_h%d = rt_str_concat_v2(%s, _auto_h%d, rt_arena_v2_strdup(%s, \", \")); ",
                result, *temp_counter, ARENA_VAR(gen), *temp_counter, ARENA_VAR(gen));
        }
    }

    /* Close with " }" - pin and extract char* for the final result */
    result = arena_sprintf(gen->arena,
        "%s_auto_h%d = rt_str_concat_v2(%s, _auto_h%d, rt_arena_v2_strdup(%s, \" }\")); "
        "(char *)_auto_h%d->ptr; })",
        result, *temp_counter, ARENA_VAR(gen), *temp_counter, ARENA_VAR(gen),
        *temp_counter);

    (*temp_counter)++;
    return result;
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
            return arena_sprintf(gen->arena, "rt_arena_v2_strdup(%s, \"\")", ARENA_VAR(gen));
        }
        return "\"\"";
    }

    bool arena_mode = (gen->current_arena_var != NULL);

    /* Gather information about each part */
    char **part_strs = arena_alloc(gen->arena, count * sizeof(char *));
    Type **part_types = arena_alloc(gen->arena, count * sizeof(Type *));
    bool *is_literal = arena_alloc(gen->arena, count * sizeof(bool));
    bool *is_temp = arena_alloc(gen->arena, count * sizeof(bool));

    int needs_conversion_count = 0;
    bool uses_format_specs = has_any_format_spec(expr);

    /* In arena mode (V2): evaluate string parts in handle mode so they can be
     * passed directly to rt_str_concat_v2 which now accepts RtHandleV2*.
     * In non-arena mode: evaluate in raw-pointer mode as before. */
    bool saved_as_handle = gen->expr_as_handle;

    for (int i = 0; i < count; i++)
    {
        part_types[i] = expr->parts[i]->expr_type;
        is_literal[i] = is_string_literal_expr(expr->parts[i]);
        is_temp[i] = expression_produces_temp(expr->parts[i]);

        if (arena_mode && part_types[i]->kind == TYPE_STRING)
        {
            /* Arena mode: evaluate string parts in handle mode */
            gen->expr_as_handle = true;
            part_strs[i] = code_gen_expression(gen, expr->parts[i]);
        }
        else
        {
            /* Non-arena or non-string: evaluate in raw mode */
            gen->expr_as_handle = false;
            part_strs[i] = code_gen_expression(gen, expr->parts[i]);
        }

        if (part_types[i]->kind != TYPE_STRING) needs_conversion_count++;
    }

    gen->expr_as_handle = saved_as_handle;

    /* Optimization: Single string literal without format - use directly */
    if (count == 1 && is_literal[0] && !uses_format_specs)
    {
        if (gen->expr_as_handle && arena_mode)
        {
            /* part_strs[0] is already a handle in arena mode (from handle-mode eval) */
            return part_strs[0];
        }
        if (arena_mode)
        {
            /* Need raw char* from handle */
            return arena_sprintf(gen->arena,
                "((char *)(%s)->ptr)", part_strs[0]);
        }
        return part_strs[0];
    }

    /* Optimization: Single string variable/temp without format - return as is or copy */
    if (count == 1 && part_types[0]->kind == TYPE_STRING && !uses_format_specs)
    {
        if (gen->expr_as_handle && arena_mode)
        {
            /* In handle mode: part_strs[0] is already a handle */
            return arena_sprintf(gen->arena, "rt_to_string_string_v2(%s, %s)", ARENA_VAR(gen), part_strs[0]);
        }
        if (arena_mode)
        {
            /* Need raw char* from handle */
            return arena_sprintf(gen->arena,
                "((char *)(%s)->ptr)", part_strs[0]);
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

    /* Optimization: Two string parts without format or temps - simple concat */
    if (count == 2 && needs_conversion_count == 0 && !is_temp[0] && !is_temp[1] && !uses_format_specs)
    {
        if (gen->expr_as_handle && arena_mode)
        {
            /* Both parts are handles in arena mode */
            return arena_sprintf(gen->arena, "rt_str_concat_v2(%s, %s, %s)", ARENA_VAR(gen), part_strs[0], part_strs[1]);
        }
        if (arena_mode)
        {
            /* Concat handles, extract char* */
            return arena_sprintf(gen->arena,
                "((char *)(rt_str_concat_v2(%s, %s, %s))->ptr)",
                ARENA_VAR(gen), part_strs[0], part_strs[1]);
        }
        return arena_sprintf(gen->arena,
            "((char *)(rt_str_concat(NULL, %s, %s))->ptr)",
            part_strs[0], part_strs[1]);
    }

    /* General case: Need to build a block expression */
    char *result = arena_strdup(gen->arena, "({\n");

    /* Track which parts need temp variables and which can be used directly.
     * In arena mode, use_strs[i] will be RtHandleV2* expressions.
     * In non-arena mode, use_strs[i] will be char* expressions. */
    char **use_strs = arena_alloc(gen->arena, count * sizeof(char *));
    int temp_var_count = 0;

    /* First pass: convert non-strings and capture temps, handle format specifiers */
    for (int i = 0; i < count; i++)
    {
        char *format_spec = (expr->format_specs != NULL) ? expr->format_specs[i] : NULL;

        if (format_spec != NULL)
        {
            if (arena_mode)
            {
                /* Arena mode: use V2 format functions → RtHandleV2* */
                const char *format_func_v2 = get_rt_format_func_v2(part_types[i]->kind);
                if (format_func_v2 != NULL)
                {
                    if (part_types[i]->kind == TYPE_STRING)
                    {
                        /* String format: part_strs[i] is already a handle */
                        result = arena_sprintf(gen->arena, "%s        RtHandleV2 *_p%d = %s(%s, %s, \"%s\");\n",
                                               result, temp_var_count, format_func_v2, ARENA_VAR(gen), part_strs[i], format_spec);
                    }
                    else
                    {
                        /* Numeric format: part_strs[i] is a primitive */
                        result = arena_sprintf(gen->arena, "%s        RtHandleV2 *_p%d = %s(%s, %s, \"%s\");\n",
                                               result, temp_var_count, format_func_v2, ARENA_VAR(gen), part_strs[i], format_spec);
                    }
                }
                else
                {
                    /* Fallback: convert to string first (V2), then format.
                     * V2 toString returns RtHandleV2*. V2 format_string takes handle. */
                    const char *to_str = get_rt_to_string_func_for_type_v2(part_types[i]);
                    if (part_types[i]->kind == TYPE_ARRAY)
                    {
                        /* Array toString V2 takes 1 arg (handle) */
                        bool saved_hm = gen->expr_as_handle;
                        gen->expr_as_handle = true;
                        char *arr_handle = code_gen_expression(gen, expr->parts[i]);
                        gen->expr_as_handle = saved_hm;
                        result = arena_sprintf(gen->arena,
                            "%s        RtHandleV2 *_th%d = %s(%s);\n"
                            "        RtHandleV2 *_p%d = rt_format_string_v2(%s, _th%d, \"%s\");\n",
                            result, temp_var_count, to_str, arr_handle,
                            temp_var_count, ARENA_VAR(gen), temp_var_count, format_spec);
                    }
                    else
                    {
                        result = arena_sprintf(gen->arena,
                            "%s        RtHandleV2 *_th%d = %s(%s, %s);\n"
                            "        RtHandleV2 *_p%d = rt_format_string_v2(%s, _th%d, \"%s\");\n",
                            result, temp_var_count, to_str, ARENA_VAR(gen), part_strs[i],
                            temp_var_count, ARENA_VAR(gen), temp_var_count, format_spec);
                    }
                }
            }
            else
            {
                /* Non-arena mode: V1 format functions → char* (unchanged) */
                const char *format_func = get_rt_format_func(part_types[i]->kind);
                if (format_func != NULL)
                {
                    result = arena_sprintf(gen->arena, "%s        char *_p%d = %s(%s, %s, \"%s\");\n",
                                           result, temp_var_count, format_func, ARENA_VAR(gen), part_strs[i], format_spec);
                }
                else
                {
                    const char *to_str = get_rt_to_string_func_for_type(part_types[i]);
                    if (part_types[i]->kind == TYPE_ANY)
                    {
                        result = arena_sprintf(gen->arena,
                            "%s        RtHandleV2 *_th%d = %s(%s, %s); char *_tmp%d = (char *)_th%d->ptr;\n",
                            result, temp_var_count, to_str, ARENA_VAR(gen), part_strs[i],
                            temp_var_count, temp_var_count);
                    }
                    else
                    {
                        result = arena_sprintf(gen->arena, "%s        char *_tmp%d = %s(%s, %s);\n",
                                               result, temp_var_count, to_str, ARENA_VAR(gen), part_strs[i]);
                    }
                    result = arena_sprintf(gen->arena, "%s        char *_p%d = rt_format_string(%s, _tmp%d, \"%s\");\n",
                                           result, temp_var_count, ARENA_VAR(gen), temp_var_count, format_spec);
                }
            }
            use_strs[i] = arena_sprintf(gen->arena, "_p%d", temp_var_count);
            temp_var_count++;
        }
        else if (part_types[i]->kind == TYPE_STRUCT)
        {
            /* Struct type: check if it has a toString() method */
            StructMethod *to_string_method = ast_struct_get_method(part_types[i], "toString");
            if (to_string_method != NULL && to_string_method->return_type != NULL &&
                to_string_method->return_type->kind == TYPE_STRING)
            {
                /* Call the struct's toString() method.
                 * Method returns a string handle (RtHandleV2*). */
                const char *struct_name_str = part_types[i]->as.struct_type.name;
                char *mangled_name = sn_mangle_name(gen->arena, struct_name_str);

                if (arena_mode)
                {
                    /* Arena mode: toString returns RtHandleV2*, use directly */
                    if (part_types[i]->as.struct_type.pass_self_by_ref ||
                        (part_types[i]->as.struct_type.is_native && part_types[i]->as.struct_type.c_alias != NULL))
                    {
                        result = arena_sprintf(gen->arena, "%s        RtHandleV2 *_p%d = %s_toString(%s, %s);\n",
                                               result, temp_var_count, mangled_name, ARENA_VAR(gen), part_strs[i]);
                    }
                    else
                    {
                        result = arena_sprintf(gen->arena, "%s        RtHandleV2 *_p%d = %s_toString(%s, &%s);\n",
                                               result, temp_var_count, mangled_name, ARENA_VAR(gen), part_strs[i]);
                    }
                }
                else
                {
                    /* Non-arena: pin to get char* */
                    if (part_types[i]->as.struct_type.pass_self_by_ref ||
                        (part_types[i]->as.struct_type.is_native && part_types[i]->as.struct_type.c_alias != NULL))
                    {
                        result = arena_sprintf(gen->arena, "%s        char *_p%d = (char *)(%s_toString(%s, %s))->ptr;\n",
                                               result, temp_var_count, mangled_name, ARENA_VAR(gen), part_strs[i]);
                    }
                    else
                    {
                        result = arena_sprintf(gen->arena, "%s        char *_p%d = (char *)(%s_toString(%s, &%s))->ptr;\n",
                                               result, temp_var_count, mangled_name, ARENA_VAR(gen), part_strs[i]);
                    }
                }
            }
            else
            {
                /* No toString() method - auto-generate string showing all fields.
                 * generate_struct_auto_tostring returns code producing char* (pinned). */
                int auto_counter = temp_var_count * 100;
                char *auto_str = generate_struct_auto_tostring(gen, part_types[i], part_strs[i], &auto_counter);
                if (arena_mode)
                {
                    /* auto_str produces char* (pinned) - wrap in handle for concat */
                    result = arena_sprintf(gen->arena, "%s        RtHandleV2 *_p%d = rt_arena_v2_strdup(%s, %s);\n",
                                           result, temp_var_count, ARENA_VAR(gen), auto_str);
                }
                else
                {
                    result = arena_sprintf(gen->arena, "%s        char *_p%d = %s;\n",
                                           result, temp_var_count, auto_str);
                }
            }
            use_strs[i] = arena_sprintf(gen->arena, "_p%d", temp_var_count);
            temp_var_count++;
        }
        else if (part_types[i]->kind != TYPE_STRING)
        {
            if (arena_mode)
            {
                /* Arena mode: convert to RtHandleV2* using V2 functions */
                if (part_types[i]->kind == TYPE_ARRAY)
                {
                    /* V2 mode array: to_string_v2 takes handle directly, returns RtHandleV2* */
                    const char *to_str = get_rt_to_string_func_for_type_v2(part_types[i]);
                    bool saved_handle_mode = gen->expr_as_handle;
                    gen->expr_as_handle = true;
                    char *handle_str = code_gen_expression(gen, expr->parts[i]);
                    gen->expr_as_handle = saved_handle_mode;
                    result = arena_sprintf(gen->arena, "%s        RtHandleV2 *_p%d = %s(%s);\n",
                                           result, temp_var_count, to_str, handle_str);
                }
                else
                {
                    /* V2 scalar: returns RtHandleV2* directly */
                    const char *to_str = get_rt_to_string_func_for_type_v2(part_types[i]);
                    result = arena_sprintf(gen->arena,
                        "%s        RtHandleV2 *_p%d = %s(%s, %s);\n",
                        result, temp_var_count, to_str, ARENA_VAR(gen), part_strs[i]);
                }
            }
            else
            {
                /* Non-arena path: V1 functions return char* except rt_any_to_string */
                const char *to_str = get_rt_to_string_func_for_type(part_types[i]);
                if (part_types[i]->kind == TYPE_ANY)
                {
                    result = arena_sprintf(gen->arena,
                        "%s        char *_p%d = (char *)(%s(%s, %s))->ptr;\n",
                        result, temp_var_count, to_str, ARENA_VAR(gen), part_strs[i]);
                }
                else if (part_types[i]->kind == TYPE_ARRAY)
                {
                    /* Array toString V2 returns RtHandleV2* - pin to get char* */
                    const char *to_str_v2 = get_rt_to_string_func_for_type_v2(part_types[i]);
                    bool saved_handle_mode2 = gen->expr_as_handle;
                    gen->expr_as_handle = true;
                    char *handle_str2 = code_gen_expression(gen, expr->parts[i]);
                    gen->expr_as_handle = saved_handle_mode2;
                    result = arena_sprintf(gen->arena,
                        "%s        char *_p%d = (char *)(%s(%s))->ptr;\n",
                        result, temp_var_count, to_str_v2, handle_str2);
                }
                else
                {
                    result = arena_sprintf(gen->arena, "%s        char *_p%d = %s(%s, %s);\n",
                                           result, temp_var_count, to_str, ARENA_VAR(gen), part_strs[i]);
                }
            }
            use_strs[i] = arena_sprintf(gen->arena, "_p%d", temp_var_count);
            temp_var_count++;
        }
        else if (is_temp[i])
        {
            if (arena_mode)
            {
                /* Arena mode: temp string is already a handle - capture it */
                result = arena_sprintf(gen->arena, "%s        RtHandleV2 *_p%d = %s;\n",
                                       result, temp_var_count, part_strs[i]);
            }
            else
            {
                /* Non-arena: temp string is char* */
                result = arena_sprintf(gen->arena, "%s        char *_p%d = %s;\n",
                                       result, temp_var_count, part_strs[i]);
            }
            use_strs[i] = arena_sprintf(gen->arena, "_p%d", temp_var_count);
            temp_var_count++;
        }
        else if (is_literal[i])
        {
            /* String literal - use directly (in arena mode, already wrapped in handle) */
            use_strs[i] = part_strs[i];
        }
        else
        {
            /* String variable - use directly (in arena mode, is already a handle expression) */
            use_strs[i] = part_strs[i];
        }
    }

    /* Build concatenation chain */
    bool handle_mode = (gen->expr_as_handle && arena_mode);

    if (count == 1)
    {
        if (handle_mode)
        {
            /* Single part as handle */
            if (part_types[0]->kind == TYPE_STRING)
            {
                result = arena_sprintf(gen->arena, "%s        rt_to_string_string_v2(%s, %s);\n    })",
                                       result, ARENA_VAR(gen), use_strs[0]);
            }
            else
            {
                /* Already converted to handle in first pass */
                result = arena_sprintf(gen->arena, "%s        %s;\n    })", result, use_strs[0]);
            }
        }
        else if (arena_mode)
        {
            /* Arena mode but want char* - extract from handle */
            result = arena_sprintf(gen->arena, "%s        (char *)%s->ptr;\n    })",
                                   result, use_strs[0]);
        }
        else
        {
            result = arena_sprintf(gen->arena, "%s        %s;\n    })", result, use_strs[0]);
        }
        return result;
    }
    else if (count == 2)
    {
        if (arena_mode)
        {
            /* Arena mode: concat two handles */
            if (handle_mode)
            {
                result = arena_sprintf(gen->arena, "%s        rt_str_concat_v2(%s, %s, %s);\n    })",
                                       result, ARENA_VAR(gen), use_strs[0], use_strs[1]);
            }
            else
            {
                /* Extract char* from concat result */
                result = arena_sprintf(gen->arena,
                    "%s        (char *)(rt_str_concat_v2(%s, %s, %s))->ptr;\n    })",
                    result, ARENA_VAR(gen), use_strs[0], use_strs[1]);
            }
        }
        else
        {
            /* Non-arena: V1 concat (accepts char*, not handles) */
            result = arena_sprintf(gen->arena,
                "%s        (char *)(rt_str_concat(%s, %s, %s))->ptr;\n    })",
                result, ARENA_VAR(gen), use_strs[0], use_strs[1]);
        }
        return result;
    }
    else
    {
        if (arena_mode)
        {
            /* Arena mode: chain of handle-based concats. No pin+extract between calls. */
            result = arena_sprintf(gen->arena,
                "%s        RtHandleV2 *_rh = rt_str_concat_v2(%s, %s, %s);\n",
                result, ARENA_VAR(gen), use_strs[0], use_strs[1]);

            for (int i = 2; i < count; i++)
            {
                result = arena_sprintf(gen->arena,
                    "%s        RtHandleV2 *_rh_old_%d = _rh;\n"
                    "        _rh = rt_str_concat_v2(%s, _rh, %s);\n"
                    "        rt_arena_v2_free(_rh_old_%d);\n",
                    result, i, ARENA_VAR(gen), use_strs[i], i);
            }

            /* Free intermediate _pN temps that were used as concat operands */
            for (int i = 0; i < count; i++)
            {
                if (use_strs[i] != part_strs[i])
                {
                    /* This is a _pN temp variable — free it */
                    result = arena_sprintf(gen->arena,
                        "%s        rt_arena_v2_free(%s);\n",
                        result, use_strs[i]);
                }
                else if (part_types[i]->kind == TYPE_STRING && is_literal[i])
                {
                    /* String literal handle — free it (it was strdup'd in handle mode) */
                    result = arena_sprintf(gen->arena,
                        "%s        rt_arena_v2_free(%s);\n",
                        result, use_strs[i]);
                }
            }

            if (handle_mode)
            {
                /* Return handle directly */
                result = arena_sprintf(gen->arena, "%s        _rh;\n    })", result);
            }
            else
            {
                /* Extract char* from final handle */
                result = arena_sprintf(gen->arena, "%s        (char *)_rh->ptr;\n    })", result);
            }
        }
        else
        {
            /* Non-arena: V1 chain with extract (uses rt_str_concat which accepts char*) */
            result = arena_sprintf(gen->arena,
                "%s        RtHandleV2 *_rh = rt_str_concat(%s, %s, %s); char *_r = (char *)_rh->ptr;\n",
                result, ARENA_VAR(gen), use_strs[0], use_strs[1]);

            for (int i = 2; i < count; i++)
            {
                result = arena_sprintf(gen->arena,
                    "%s        _rh = rt_str_concat(%s, _r, %s); _r = (char *)_rh->ptr;\n",
                    result, ARENA_VAR(gen), use_strs[i]);
            }

            result = arena_sprintf(gen->arena, "%s        _r;\n    })", result);
        }
        return result;
    }
}
