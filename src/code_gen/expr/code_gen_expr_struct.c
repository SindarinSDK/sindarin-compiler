#include "code_gen/expr/code_gen_expr_struct.h"
#include "code_gen/expr/code_gen_expr.h"
#include "code_gen/util/code_gen_util.h"
#include "symbol_table.h"
#include "debug.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/**
 * Generate code for struct literal expression.
 * Point { x: 1.0, y: 2.0 } -> (Point){ .x = 1.0, .y = 2.0 }
 */
char *code_gen_struct_literal_expression(CodeGen *gen, Expr *expr)
{
    DEBUG_VERBOSE("Generating struct literal expression");

    StructLiteralExpr *lit = &expr->as.struct_literal;
    Type *struct_type = lit->struct_type;

    if (struct_type == NULL || struct_type->kind != TYPE_STRUCT)
    {
        fprintf(stderr, "Error: Struct literal has no resolved type\n");
        fprintf(stderr, "  struct_name: %.*s\n",
                (int)lit->struct_name.length, lit->struct_name.start);
        fprintf(stderr, "  namespace_prefix: %s\n",
                gen->current_namespace_prefix ? gen->current_namespace_prefix : "(none)");
        exit(1);
    }

    /* Use c_alias for C type name if available, otherwise use mangled Sindarin name */
    const char *c_type_name = struct_type->as.struct_type.c_alias != NULL
        ? struct_type->as.struct_type.c_alias
        : sn_mangle_name(gen->arena, struct_type->as.struct_type.name);
    int total_fields = struct_type->as.struct_type.field_count;

    /* Build the C compound literal: (StructName){ .field1 = val1, .field2 = val2, ... }
     * When inside an array compound literal, omit the outer (StructName) cast since
     * the array type already establishes the element type. This is required for TCC
     * compatibility which doesn't handle nested compound literal casts. */
    char *result;
    if (gen->in_array_compound_literal) {
        result = arena_strdup(gen->arena, "{ ");
    } else {
        result = arena_sprintf(gen->arena, "(%s){ ", c_type_name);
    }

    bool first = true;

    /* Initialize hidden arena reference for non-native, non-packed structs */
    if (!struct_type->as.struct_type.is_native && !struct_type->as.struct_type.is_packed)
    {
        const char *arena_var = ARENA_VAR(gen);
        if (gen->current_arena_var == NULL)
        {
            /* File scope - no arena available, use NULL (C requires constant initializers) */
            result = arena_sprintf(gen->arena, "%s.__arena__ = NULL", result);
        }
        else
        {
            result = arena_sprintf(gen->arena, "%s.__arena__ = rt_arena_v2_create(%s, RT_ARENA_MODE_DEFAULT, \"struct\")", result, arena_var);
        }
        first = false;
    }

    for (int i = 0; i < total_fields; i++)
    {
        StructField *field = &struct_type->as.struct_type.fields[i];

        /* Find if this field was explicitly initialized */
        Expr *init_value = NULL;
        for (int j = 0; j < lit->field_count; j++)
        {
            const char *init_name = lit->fields[j].name.start;
            int init_len = lit->fields[j].name.length;
            if ((int)strlen(field->name) == init_len &&
                strncmp(field->name, init_name, init_len) == 0)
            {
                init_value = lit->fields[j].value;
                break;
            }
        }

        /* Use default value if not explicitly initialized */
        if (init_value == NULL)
        {
            init_value = field->default_value;
        }

        /* Generate field initializer */
        if (init_value != NULL)
        {
            /* For string/array fields stored as RtHandle, force handle mode so the
               value is properly wrapped with rt_managed_strdup / handle creation.
               For other field types, use raw mode.
               At file scope (no arena), string/array fields must be NULL
               since we can't call runtime functions in global initializers. */
            char *value_code;
            if (gen->current_arena_var == NULL &&
                (field->type->kind == TYPE_STRING || field->type->kind == TYPE_ARRAY))
            {
                value_code = arena_strdup(gen->arena, "NULL");
            }
            /* Handle function-typed fields: when the value is a named function,
               wrap it in a closure. Named functions are just function pointers in C,
               but function-typed fields expect __Closure__ * which has fn and arena fields. */
            else if (field->type->kind == TYPE_FUNCTION &&
                     !field->type->as.function.is_native &&
                     init_value->type == EXPR_VARIABLE)
            {
                Symbol *func_sym = symbol_table_lookup_symbol(gen->symbol_table, init_value->as.variable.name);
                if (func_sym != NULL && func_sym->is_function)
                {
                    /* Generate a wrapper function that adapts the closure calling convention
                     * to the named function's signature. The wrapper takes (void*, params...)
                     * and forwards to the actual function ignoring the closure pointer. */
                    Type *func_type = field->type;
                    int wrapper_id = gen->wrapper_count++;
                    char *wrapper_name = arena_sprintf(gen->arena, "__wrap_%d__", wrapper_id);
                    const char *ret_c_type = get_c_type(gen->arena, func_type->as.function.return_type);

                    /* Build parameter list: void* first, then actual params */
                    char *params_decl = arena_strdup(gen->arena, "void *__closure__");
                    char *args_forward = arena_strdup(gen->arena, "");

                    /* Check if wrapped function is a Sindarin function (has body) - if so, prepend arena.
                     * Use rt_get_thread_arena_or() to prefer thread arena when called from thread context. */
                    bool wrapped_has_body = (func_sym->type != NULL &&
                                             func_sym->type->kind == TYPE_FUNCTION &&
                                             func_sym->type->as.function.has_body);
                    if (wrapped_has_body)
                    {
                        args_forward = arena_strdup(gen->arena, "({ RtArenaV2 *__tls_a = rt_tls_arena_get(); __tls_a ? __tls_a : ((__Closure__ *)__closure__)->arena; })");
                    }

                    for (int p = 0; p < func_type->as.function.param_count; p++)
                    {
                        const char *param_c_type = get_c_type(gen->arena, func_type->as.function.param_types[p]);
                        params_decl = arena_sprintf(gen->arena, "%s, %s __p%d__", params_decl, param_c_type, p);
                        if (p > 0 || wrapped_has_body)
                            args_forward = arena_sprintf(gen->arena, "%s, ", args_forward);
                        args_forward = arena_sprintf(gen->arena, "%s__p%d__", args_forward, p);
                    }

                    /* Generate wrapper function */
                    char *func_name = sn_mangle_name(gen->arena,
                        arena_strndup(gen->arena, init_value->as.variable.name.start, init_value->as.variable.name.length));
                    bool is_void_return = (func_type->as.function.return_type &&
                                           func_type->as.function.return_type->kind == TYPE_VOID);
                    char *wrapper_func;
                    if (is_void_return)
                    {
                        wrapper_func = arena_sprintf(gen->arena,
                            "static void %s(%s) {\n"
                            "    (void)__closure__;\n"
                            "    %s(%s);\n"
                            "}\n\n",
                            wrapper_name, params_decl, func_name, args_forward);
                    }
                    else
                    {
                        wrapper_func = arena_sprintf(gen->arena,
                            "static %s %s(%s) {\n"
                            "    (void)__closure__;\n"
                            "    return %s(%s);\n"
                            "}\n\n",
                            ret_c_type, wrapper_name, params_decl, func_name, args_forward);
                    }

                    /* Add wrapper to lambda definitions */
                    gen->lambda_definitions = arena_sprintf(gen->arena, "%s%s",
                                                            gen->lambda_definitions, wrapper_func);

                    /* Add forward declaration */
                    gen->lambda_forward_decls = arena_sprintf(gen->arena, "%sstatic %s %s(%s);\n",
                                                              gen->lambda_forward_decls, ret_c_type, wrapper_name, params_decl);

                    /* Wrap the wrapper function in a closure struct */
                    const char *arena_var = ARENA_VAR(gen);
                    if (strcmp(arena_var, "NULL") == 0)
                    {
                        /* No arena - use malloc */
                        value_code = arena_sprintf(gen->arena,
                            "({\n"
                            "    __Closure__ *__cl__ = malloc(sizeof(__Closure__));\n"
                            "    __cl__->fn = (void *)%s;\n"
                            "    __cl__->arena = NULL;\n"
                            "    __cl__->size = sizeof(__Closure__);\n"
                            "    __cl__;\n"
                            "})",
                            wrapper_name);
                    }
                    else
                    {
                        /* Use V2 arena allocation */
                        value_code = arena_sprintf(gen->arena,
                            "({\n"
                            "    RtHandleV2 *__cl_h__ = rt_arena_v2_alloc(%s, sizeof(__Closure__));\n"
                            "    rt_handle_begin_transaction(__cl_h__);\n"
                            "    __Closure__ *__cl__ = (__Closure__ *)__cl_h__->ptr;\n"
                            "    __cl__->fn = (void *)%s;\n"
                            "    __cl__->arena = %s;\n"
                            "    __cl__->size = sizeof(__Closure__);\n"
                            "    rt_handle_end_transaction(__cl_h__);\n"
                            "    __cl__;\n"
                            "})",
                            arena_var, wrapper_name, arena_var);
                    }
                }
                else
                {
                    /* Not a named function - generate normally (probably a closure variable) */
                    bool saved_handle = gen->expr_as_handle;
                    gen->expr_as_handle = false;
                    value_code = code_gen_expression(gen, init_value);
                    gen->expr_as_handle = saved_handle;
                }
            }
            /* Special handling for empty array literals: use field's element type
             * instead of the expression's TYPE_NIL element type which generates NULL */
            else if (field->type->kind == TYPE_ARRAY &&
                     init_value->type == EXPR_ARRAY &&
                     init_value->as.array.element_count == 0 &&
                     gen->current_arena_var != NULL)
            {
                Type *elem_type = field->type->as.array.element_type;
                const char *elem_c = get_c_type(gen->arena, elem_type);

                if (elem_type->kind == TYPE_STRING) {
                    value_code = arena_sprintf(gen->arena, "rt_array_create_string_v2(%s, 0, NULL)",
                                               ARENA_VAR(gen));
                } else if (elem_type->kind == TYPE_STRUCT && struct_has_handle_fields(elem_type)) {
                    code_gen_ensure_struct_callbacks(gen, elem_type);
                    const char *sn_name = elem_type->as.struct_type.name
                        ? elem_type->as.struct_type.name : elem_c;
                    value_code = arena_sprintf(gen->arena,
                        "({ RtHandleV2 *__arr_h__ = rt_array_create_generic_v2(%s, 0, sizeof(%s), NULL);"
                        " rt_handle_set_copy_callback(__arr_h__, __copy_array_%s__); __arr_h__; })",
                        ARENA_VAR(gen), elem_c, sn_name);
                } else {
                    value_code = arena_sprintf(gen->arena, "rt_array_create_generic_v2(%s, 0, sizeof(%s), NULL)",
                                               ARENA_VAR(gen), elem_c);
                }
            }
            else
            {
                bool saved_handle = gen->expr_as_handle;
                if (gen->current_arena_var != NULL &&
                    (field->type->kind == TYPE_STRING || field->type->kind == TYPE_ARRAY))
                {
                    gen->expr_as_handle = true;
                }
                else
                {
                    gen->expr_as_handle = false;
                }
                value_code = code_gen_expression(gen, init_value);
                gen->expr_as_handle = saved_handle;
            }
            if (!first)
            {
                result = arena_sprintf(gen->arena, "%s, ", result);
            }
            /* Use c_alias for field name if available, otherwise mangle Sindarin name */
            const char *c_field_name = field->c_alias != NULL
                ? field->c_alias
                : sn_mangle_name(gen->arena, field->name);
            result = arena_sprintf(gen->arena, "%s.%s = %s", result, c_field_name, value_code);
            first = false;
        }
        else
        {
            /* No value provided and no default - use C default (zero) */
            /* For C compound literals, uninitialized fields are zeroed */
        }
    }

    result = arena_sprintf(gen->arena, "%s }", result);
    return result;
}
