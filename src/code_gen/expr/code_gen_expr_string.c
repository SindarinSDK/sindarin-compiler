/* ============================================================================
 * code_gen_expr_string.c - String Interpolation Code Generation
 * ============================================================================
 * Generates C code for string interpolation expressions.
 * Extracted from code_gen_expr.c for modularity.
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

/* Get the runtime format function for a type */
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

    /* Start building: "StructName { " */
    char *result = arena_sprintf(gen->arena,
        "({ char *_auto_str%d = rt_str_concat(%s, \"%s { \", \"\"); ",
        *temp_counter, ARENA_VAR(gen), struct_name);

    for (int i = 0; i < field_count; i++)
    {
        const char *field_name = fields[i].name;
        Type *field_type = fields[i].type;

        /* Get the C field name (use c_alias if present, otherwise mangle) */
        const char *c_field_name = fields[i].c_alias
            ? fields[i].c_alias
            : sn_mangle_name(gen->arena, field_name);

        /* Add field name (use original name for display) */
        result = arena_sprintf(gen->arena, "%s_auto_str%d = rt_str_concat(%s, _auto_str%d, \"%s: \"); ",
                               result, *temp_counter, ARENA_VAR(gen), *temp_counter, field_name);

        /* Add field value based on type (use C name for access) */
        const char *field_access = arena_sprintf(gen->arena, "(%s)%s%s", value_expr, accessor, c_field_name);

        if (field_type == NULL)
        {
            /* Unknown type - skip */
            result = arena_sprintf(gen->arena, "%s_auto_str%d = rt_str_concat(%s, _auto_str%d, \"?\"); ",
                                   result, *temp_counter, ARENA_VAR(gen), *temp_counter);
        }
        else if (field_type->kind == TYPE_STRING)
        {
            /* String field - need to pin handle and wrap in quotes */
            result = arena_sprintf(gen->arena,
                "%s_auto_str%d = rt_str_concat(%s, _auto_str%d, \"\\\"\"); "
                "{ char *_fstr = (char *)rt_managed_pin(%s, %s); "
                "_auto_str%d = rt_str_concat(%s, _auto_str%d, _fstr ? _fstr : \"null\"); } "
                "_auto_str%d = rt_str_concat(%s, _auto_str%d, \"\\\"\"); ",
                result, *temp_counter, ARENA_VAR(gen), *temp_counter,
                ARENA_VAR(gen), field_access,
                *temp_counter, ARENA_VAR(gen), *temp_counter,
                *temp_counter, ARENA_VAR(gen), *temp_counter);
        }
        else if (field_type->kind == TYPE_CHAR)
        {
            /* Char field - wrap in single quotes */
            result = arena_sprintf(gen->arena,
                "%s_auto_str%d = rt_str_concat(%s, _auto_str%d, \"'\"); "
                "_auto_str%d = rt_str_concat(%s, _auto_str%d, rt_to_string_char(%s, %s)); "
                "_auto_str%d = rt_str_concat(%s, _auto_str%d, \"'\"); ",
                result, *temp_counter, ARENA_VAR(gen), *temp_counter,
                *temp_counter, ARENA_VAR(gen), *temp_counter, ARENA_VAR(gen), field_access,
                *temp_counter, ARENA_VAR(gen), *temp_counter);
        }
        else
        {
            /* Other types - use appropriate to_string function */
            const char *to_str_func = get_rt_to_string_func_for_type(field_type);
            result = arena_sprintf(gen->arena,
                "%s_auto_str%d = rt_str_concat(%s, _auto_str%d, %s(%s, %s)); ",
                result, *temp_counter, ARENA_VAR(gen), *temp_counter, to_str_func, ARENA_VAR(gen), field_access);
        }

        /* Add separator (", " or " }") */
        if (i < field_count - 1)
        {
            result = arena_sprintf(gen->arena, "%s_auto_str%d = rt_str_concat(%s, _auto_str%d, \", \"); ",
                                   result, *temp_counter, ARENA_VAR(gen), *temp_counter);
        }
    }

    /* Close with " }" */
    result = arena_sprintf(gen->arena, "%s_auto_str%d = rt_str_concat(%s, _auto_str%d, \" }\"); _auto_str%d; })",
                           result, *temp_counter, ARENA_VAR(gen), *temp_counter, *temp_counter);

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
        else if (part_types[i]->kind == TYPE_STRUCT)
        {
            /* Struct type: check if it has a toString() method */
            StructMethod *to_string_method = ast_struct_get_method(part_types[i], "toString");
            if (to_string_method != NULL && to_string_method->return_type != NULL &&
                to_string_method->return_type->kind == TYPE_STRING)
            {
                /* Call the struct's toString() method.
                 * Method naming convention: StructName_toString(arena, self)
                 * The method returns a string handle (RtHandle), so we need to pin it to get char* */
                const char *struct_name = part_types[i]->as.struct_type.name;
                char *mangled_name = sn_mangle_name(gen->arena, struct_name);

                /* For 'as ref' structs (native handle types), self is passed as-is (already a pointer).
                 * For regular structs, self needs to be passed by address. */
                if (part_types[i]->as.struct_type.pass_self_by_ref ||
                    (part_types[i]->as.struct_type.is_native && part_types[i]->as.struct_type.c_alias != NULL))
                {
                    result = arena_sprintf(gen->arena, "%s        char *_p%d = (char *)rt_managed_pin(%s, %s_toString(%s, %s));\n",
                                           result, temp_var_count, ARENA_VAR(gen), mangled_name, ARENA_VAR(gen), part_strs[i]);
                }
                else
                {
                    result = arena_sprintf(gen->arena, "%s        char *_p%d = (char *)rt_managed_pin(%s, %s_toString(%s, &%s));\n",
                                           result, temp_var_count, ARENA_VAR(gen), mangled_name, ARENA_VAR(gen), part_strs[i]);
                }
            }
            else
            {
                /* No toString() method - auto-generate string showing all fields */
                int auto_counter = temp_var_count * 100;  /* Avoid collision with other temp vars */
                char *auto_str = generate_struct_auto_tostring(gen, part_types[i], part_strs[i], &auto_counter);
                result = arena_sprintf(gen->arena, "%s        char *_p%d = %s;\n",
                                       result, temp_var_count, auto_str);
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
