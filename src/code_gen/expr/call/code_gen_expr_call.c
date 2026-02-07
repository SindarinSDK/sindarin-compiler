/**
 * code_gen_expr_call.c - Code generation for call expressions
 *
 * This is the main dispatcher for generating C code from function calls
 * and method calls. It delegates to specialized handlers for different
 * object types (arrays, strings, files, etc.) defined in:
 * - code_gen_expr_call_namespace.c
 * - code_gen_expr_call_struct.c
 * - code_gen_expr_call_closure.c
 * - code_gen_expr_call_builtin.c
 * - code_gen_expr_call_array.c
 * - code_gen_expr_call_string.c
 * - code_gen_expr_call_char.c
 * - code_gen_expr_call_intercept.c
 */

#include "code_gen/expr/call/code_gen_expr_call.h"
#include "code_gen/expr/code_gen_expr.h"
#include "code_gen/util/code_gen_util.h"
#include "debug.h"
#include "symbol_table.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <ctype.h>

/* ============================================================================
 * Helper Functions
 * ============================================================================ */

bool expression_produces_temp(Expr *expr)
{
    DEBUG_VERBOSE("Entering expression_produces_temp");
    if (expr->expr_type->kind != TYPE_STRING)
        return false;
    switch (expr->type)
    {
    case EXPR_VARIABLE:
    case EXPR_ASSIGN:
    case EXPR_INDEX_ASSIGN:
    case EXPR_LITERAL:
        return false;
    case EXPR_BINARY:
    case EXPR_CALL:
    case EXPR_INTERPOLATED:
        return true;
    default:
        return false;
    }
}

char *code_gen_self_ref(CodeGen *gen, Expr *object, const char *struct_c_type, char *self_str)
{
    /* Check if object is an rvalue that requires a temporary */
    if (object->type == EXPR_CALL || object->type == EXPR_STATIC_CALL)
    {
        /* Object is an rvalue (method chaining) - emit temp variable */
        int tmp_id = gen->temp_count++;
        char *tmp_name = arena_sprintf(gen->arena, "_chain_tmp_%d", tmp_id);
        indented_fprintf(gen, gen->current_indent, "%s %s = %s;\n",
                         struct_c_type, tmp_name, self_str);
        return arena_sprintf(gen->arena, "&%s", tmp_name);
    }
    else
    {
        /* Object is an lvalue - take address directly */
        return arena_sprintf(gen->arena, "&%s", self_str);
    }
}

char *code_gen_wrap_fn_arg_as_closure(CodeGen *gen, Type *param_type, Expr *arg_expr, const char *arg_str)
{
    if (param_type == NULL || param_type->kind != TYPE_FUNCTION ||
        param_type->as.function.is_native ||
        arg_expr->type != EXPR_VARIABLE)
    {
        return NULL;
    }

    Symbol *arg_sym = symbol_table_lookup_symbol(gen->symbol_table, arg_expr->as.variable.name);
    if (arg_sym == NULL || !arg_sym->is_function)
    {
        return NULL;
    }

    Type *func_type = param_type;
    int wrapper_id = gen->wrapper_count++;
    char *wrapper_name = arena_sprintf(gen->arena, "__wrap_%d__", wrapper_id);
    const char *ret_c_type = get_c_type(gen->arena, func_type->as.function.return_type);

    char *params_decl = arena_strdup(gen->arena, "void *__closure__");
    char *args_forward = arena_strdup(gen->arena, "");

    bool wrapped_has_body = (arg_sym->type != NULL &&
                             arg_sym->type->kind == TYPE_FUNCTION &&
                             arg_sym->type->as.function.has_body);
    if (wrapped_has_body)
    {
        args_forward = arena_strdup(gen->arena, "rt_arena_v2_thread_or(((__Closure__ *)__closure__)->arena)");
    }

    for (int p = 0; p < func_type->as.function.param_count; p++)
    {
        const char *param_c_type = get_c_type(gen->arena, func_type->as.function.param_types[p]);
        params_decl = arena_sprintf(gen->arena, "%s, %s __p%d__", params_decl, param_c_type, p);
        if (p > 0 || wrapped_has_body)
            args_forward = arena_sprintf(gen->arena, "%s, ", args_forward);
        args_forward = arena_sprintf(gen->arena, "%s__p%d__", args_forward, p);
    }

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
            wrapper_name, params_decl, arg_str, args_forward);
    }
    else
    {
        wrapper_func = arena_sprintf(gen->arena,
            "static %s %s(%s) {\n"
            "    (void)__closure__;\n"
            "    return %s(%s);\n"
            "}\n\n",
            ret_c_type, wrapper_name, params_decl, arg_str, args_forward);
    }

    gen->lambda_definitions = arena_sprintf(gen->arena, "%s%s",
                                            gen->lambda_definitions, wrapper_func);
    gen->lambda_forward_decls = arena_sprintf(gen->arena, "%sstatic %s %s(%s);\n",
                                              gen->lambda_forward_decls, ret_c_type, wrapper_name, params_decl);

    const char *arena_var = ARENA_VAR(gen);
    if (strcmp(arena_var, "NULL") == 0)
    {
        return arena_sprintf(gen->arena,
            "({\n"
            "    __Closure__ *__cl__ = malloc(sizeof(__Closure__));\n"
            "    __cl__->fn = (void *)%s;\n"
            "    __cl__->arena = NULL;\n"
            "    __cl__;\n"
            "})",
            wrapper_name);
    }
    else
    {
        return arena_sprintf(gen->arena,
            "({\n"
            "    __Closure__ *__cl__ = (__Closure__ *)rt_handle_v2_pin(rt_arena_v2_alloc(%s, sizeof(__Closure__)));\n"
            "    __cl__->fn = (void *)%s;\n"
            "    __cl__->arena = %s;\n"
            "    __cl__;\n"
            "})",
            arena_var, wrapper_name, arena_var);
    }
}

/* ============================================================================
 * Member Expression Dispatch (method calls on objects)
 * ============================================================================ */

static char *code_gen_member_call(CodeGen *gen, Expr *expr, CallExpr *call)
{
    MemberExpr *member = &call->callee->as.member;
    char *member_name_str = get_var_name(gen->arena, member->member_name);
    Type *object_type = member->object->expr_type;

    /* Namespace function call (namespace.function) */
    if (object_type == NULL && member->object->type == EXPR_VARIABLE)
    {
        return code_gen_namespace_function_call(gen, expr, member, member_name_str, call);
    }

    /* Nested namespace function call (parent.nested.function) */
    if (object_type == NULL && member->object->type == EXPR_MEMBER &&
        member->object->as.member.resolved_namespace != NULL)
    {
        return code_gen_nested_namespace_call(gen, expr, member, member_name_str, call);
    }

    /* Namespace struct type static method call (namespace.StructType.staticMethod) */
    if (object_type == NULL && member->object->type == EXPR_MEMBER &&
        member->object->as.member.resolved_struct_type != NULL)
    {
        char *result = code_gen_namespace_static_method_call(gen, expr, member, call);
        if (result) return result;
    }

    char *result = NULL;

    if (object_type == NULL)
    {
        fprintf(stderr, "Internal error: NULL object_type in member call expression for '%s'\n",
                member_name_str);
        return arena_strdup(gen->arena, "0 /* ERROR: unresolved type */");
    }

    /* Dispatch to type-specific handlers */
    switch (object_type->kind) {
        case TYPE_ARRAY: {
            Type *element_type = object_type->as.array.element_type;
            result = code_gen_array_method_call(gen, expr, member_name_str,
                member->object, element_type, call->arg_count, call->arguments);
            if (result) return result;
            break;
        }
        case TYPE_STRING: {
            bool object_is_temp = expression_produces_temp(member->object);
            result = code_gen_string_method_call(gen, member_name_str,
                member->object, object_is_temp, call->arg_count, call->arguments);
            if (result) return result;
            break;
        }
        case TYPE_STRUCT: {
            if (member->resolved_method != NULL)
            {
                StructMethod *method = member->resolved_method;
                Type *struct_type = member->resolved_struct_type;

                if (method->is_native)
                {
                    return code_gen_native_struct_method_call(gen, expr, member,
                                                               method, struct_type, call);
                }
                else
                {
                    return code_gen_sindarin_struct_method_call(gen, expr, member,
                                                                 method, struct_type, call);
                }
            }
            break;
        }
        case TYPE_POINTER: {
            result = code_gen_pointer_struct_method_call(gen, expr, member, object_type, call);
            if (result) return result;
            break;
        }
        case TYPE_CHAR: {
            result = code_gen_char_method_call(gen, member_name_str,
                member->object, call->arg_count);
            if (result) return result;
            break;
        }
        default:
            break;
    }

    return NULL;  /* Not handled - fall through to regular call processing */
}

/* ============================================================================
 * Regular Function Call Generation
 * ============================================================================ */

static char *code_gen_regular_call(CodeGen *gen, Expr *expr, CallExpr *call)
{
    char *callee_str = code_gen_expression(gen, call->callee);

    /* Determine if callee is a Sindarin function (has body) and if it's a native function */
    bool callee_is_sindarin = false;
    bool callee_is_native = false;
    if (call->callee->type == EXPR_VARIABLE)
    {
        Symbol *callee_sym = symbol_table_lookup_symbol(gen->symbol_table, call->callee->as.variable.name);
        if (callee_sym != NULL && callee_sym->type != NULL &&
            callee_sym->type->kind == TYPE_FUNCTION)
        {
            callee_is_sindarin = callee_sym->type->as.function.has_body;
            callee_is_native = callee_sym->type->as.function.is_native;
        }
    }

    /* Generate args in appropriate mode */
    bool outer_as_handle = gen->expr_as_handle;
    gen->expr_as_handle = (callee_is_sindarin && gen->current_arena_var != NULL);

    char **arg_strs = arena_alloc(gen->arena, call->arg_count * sizeof(char *));
    bool *arg_is_temp = arena_alloc(gen->arena, call->arg_count * sizeof(bool));
    bool has_temps = false;
    for (int i = 0; i < call->arg_count; i++)
    {
        /* For native functions receiving str[] args */
        if (!callee_is_sindarin && callee_is_native && gen->current_arena_var != NULL &&
            call->arguments[i]->expr_type != NULL &&
            call->arguments[i]->expr_type->kind == TYPE_ARRAY &&
            call->arguments[i]->expr_type->as.array.element_type != NULL &&
            call->arguments[i]->expr_type->as.array.element_type->kind == TYPE_STRING)
        {
            bool prev = gen->expr_as_handle;
            gen->expr_as_handle = true;
            char *handle_expr = code_gen_expression(gen, call->arguments[i]);
            gen->expr_as_handle = prev;
            arg_strs[i] = arena_sprintf(gen->arena, "rt_pin_string_array_v2(%s)",
                                         handle_expr);
        }
        /* For native functions receiving individual str args - convert RtHandle to const char* */
        else if (!callee_is_sindarin && callee_is_native && gen->current_arena_var != NULL &&
                 call->arguments[i]->expr_type != NULL &&
                 call->arguments[i]->expr_type->kind == TYPE_STRING)
        {
            bool prev = gen->expr_as_handle;
            gen->expr_as_handle = true;
            char *handle_expr = code_gen_expression(gen, call->arguments[i]);
            gen->expr_as_handle = prev;
            arg_strs[i] = arena_sprintf(gen->arena, "(char *)rt_handle_v2_pin(%s)",
                                         handle_expr);
        }
        else
        {
            arg_strs[i] = code_gen_expression(gen, call->arguments[i]);
        }
        arg_is_temp[i] = (!callee_is_sindarin &&
                          call->arguments[i]->expr_type && call->arguments[i]->expr_type->kind == TYPE_STRING &&
                          expression_produces_temp(call->arguments[i]));
        if (arg_is_temp[i])
            has_temps = true;
    }

    gen->expr_as_handle = outer_as_handle;

    /* Try builtin call first */
    char *builtin_result = code_gen_try_builtin_call(gen, expr, call, arg_strs, &callee_str);
    if (builtin_result)
        return builtin_result;

    /* Determine arena handling */
    bool callee_has_body = callee_is_sindarin;
    bool callee_needs_arena = false;

    if (call->callee->type == EXPR_VARIABLE && !callee_has_body)
    {
        Symbol *callee_sym = symbol_table_lookup_symbol(gen->symbol_table, call->callee->as.variable.name);
        if (callee_sym != NULL && callee_sym->type != NULL &&
            callee_sym->type->kind == TYPE_FUNCTION)
        {
            callee_needs_arena = callee_sym->type->as.function.has_arena_param;
        }
    }

    bool prepend_arena = callee_has_body || callee_needs_arena;

    /* Build args list */
    char **arg_base_names = arena_alloc(gen->arena, sizeof(char *) * call->arg_count);
    char **arg_names = arena_alloc(gen->arena, sizeof(char *) * call->arg_count);

    char *args_list;
    if (prepend_arena && gen->current_arena_var != NULL)
    {
        args_list = arena_strdup(gen->arena, gen->current_arena_var);
    }
    else if (prepend_arena)
    {
        args_list = arena_strdup(gen->arena, "NULL");
    }
    else
    {
        args_list = arena_strdup(gen->arena, "");
    }

    /* Get parameter types for boxing/ref handling */
    MemoryQualifier *param_quals = NULL;
    Type **param_types = NULL;
    int param_count = 0;
    bool is_user_defined_function = false;
    if (call->callee->expr_type && call->callee->expr_type->kind == TYPE_FUNCTION)
    {
        param_quals = call->callee->expr_type->as.function.param_mem_quals;
        param_types = call->callee->expr_type->as.function.param_types;
        param_count = call->callee->expr_type->as.function.param_count;

        if (call->callee->type == EXPR_VARIABLE)
        {
            Symbol *sym = symbol_table_lookup_symbol(gen->symbol_table, call->callee->as.variable.name);
            is_user_defined_function = (sym != NULL && sym->is_function);
        }
    }

    for (int i = 0; i < call->arg_count; i++)
    {
        if (arg_is_temp[i])
        {
            arg_base_names[i] = arena_sprintf(gen->arena, "_str_arg%d", i);
            arg_names[i] = arg_base_names[i];
        }
        else
        {
            arg_base_names[i] = arg_strs[i];
            arg_names[i] = arg_strs[i];
        }

        /* Handle boxing when parameter type is 'any' */
        if (is_user_defined_function && param_types != NULL && i < param_count &&
            param_types[i] != NULL && param_types[i]->kind == TYPE_ANY &&
            call->arguments[i]->expr_type != NULL &&
            call->arguments[i]->expr_type->kind != TYPE_ANY)
        {
            arg_names[i] = code_gen_box_value(gen, arg_names[i], call->arguments[i]->expr_type);
        }

        /* Handle 'as ref' parameters */
        bool is_ref_param = false;
        if (param_quals != NULL && i < param_count && param_quals[i] == MEM_AS_REF &&
            call->arguments[i]->expr_type != NULL)
        {
            TypeKind kind = call->arguments[i]->expr_type->kind;
            bool is_prim = (kind == TYPE_INT || kind == TYPE_INT32 || kind == TYPE_UINT ||
                           kind == TYPE_UINT32 || kind == TYPE_LONG || kind == TYPE_DOUBLE ||
                           kind == TYPE_FLOAT || kind == TYPE_CHAR || kind == TYPE_BOOL ||
                           kind == TYPE_BYTE);
            bool is_struct = (kind == TYPE_STRUCT);
            is_ref_param = is_prim || is_struct;
        }
        if (is_ref_param)
        {
            arg_names[i] = arena_sprintf(gen->arena, "&%s", arg_names[i]);
        }

        /* Handle closure wrapping for function parameters */
        if (param_types != NULL && i < param_count &&
            param_types[i] != NULL && param_types[i]->kind == TYPE_FUNCTION &&
            !param_types[i]->as.function.is_native &&
            call->arguments[i]->type == EXPR_VARIABLE)
        {
            Symbol *arg_sym = symbol_table_lookup_symbol(gen->symbol_table, call->arguments[i]->as.variable.name);
            if (arg_sym != NULL && arg_sym->is_function)
            {
                char *wrapped = code_gen_wrap_fn_arg_as_closure(gen, param_types[i],
                    call->arguments[i], arg_strs[i]);
                if (wrapped != NULL)
                {
                    arg_names[i] = wrapped;
                }
            }
        }

        bool need_comma = (i > 0) || prepend_arena;
        char *new_args = arena_sprintf(gen->arena, "%s%s%s", args_list, need_comma ? ", " : "", arg_names[i]);
        args_list = new_args;
    }

    bool returns_void = (expr->expr_type && expr->expr_type->kind == TYPE_VOID);

    /* Check for interception */
    char *func_name_for_intercept = NULL;
    bool skip_interception = false;
    if (is_user_defined_function && call->callee->type == EXPR_VARIABLE)
    {
        func_name_for_intercept = get_var_name(gen->arena, call->callee->as.variable.name);

        if (param_types != NULL && !skip_interception)
        {
            for (int i = 0; i < param_count; i++)
            {
                if (param_types[i] != NULL &&
                    (param_types[i]->kind == TYPE_POINTER || param_types[i]->kind == TYPE_STRUCT))
                {
                    skip_interception = true;
                    break;
                }
            }
        }

        if (!skip_interception && call->callee->expr_type != NULL &&
            call->callee->expr_type->kind == TYPE_FUNCTION &&
            call->callee->expr_type->as.function.is_native)
        {
            skip_interception = true;
        }

        if (!skip_interception && expr->expr_type != NULL &&
            (expr->expr_type->kind == TYPE_POINTER || expr->expr_type->kind == TYPE_STRUCT))
        {
            skip_interception = true;
        }
    }

    /* Generate call (simple path - no temps) */
    if (!has_temps)
    {
        if (is_user_defined_function && func_name_for_intercept != NULL && !skip_interception)
        {
            char *intercept_expr = code_gen_intercepted_call(gen, func_name_for_intercept,
                                             callee_str, call, arg_strs, arg_names,
                                             param_types, param_quals, param_count,
                                             expr->expr_type, callee_has_body);
            if (!gen->expr_as_handle && gen->current_arena_var != NULL &&
                expr->expr_type != NULL && is_handle_type(expr->expr_type))
            {
                if (expr->expr_type->kind == TYPE_STRING)
                {
                    return arena_sprintf(gen->arena, "(char *)rt_handle_v2_pin(%s)",
                                         intercept_expr);
                }
                else if (expr->expr_type->kind == TYPE_ARRAY)
                {
                    const char *elem_c = get_c_array_elem_type(gen->arena, expr->expr_type->as.array.element_type);
                    return arena_sprintf(gen->arena, "(((%s *)rt_array_data_v2(%s)))",
                                         elem_c, intercept_expr);
                }
            }
            return intercept_expr;
        }

        char *call_expr = arena_sprintf(gen->arena, "%s(%s)", callee_str, args_list);

        /* Handle return type conversions */
        if (!gen->expr_as_handle && callee_has_body &&
            gen->current_arena_var != NULL &&
            expr->expr_type != NULL && is_handle_type(expr->expr_type))
        {
            if (expr->expr_type->kind == TYPE_STRING)
            {
                return arena_sprintf(gen->arena, "(char *)rt_handle_v2_pin(%s)",
                                     call_expr);
            }
            else if (expr->expr_type->kind == TYPE_ARRAY)
            {
                const char *elem_c = get_c_array_elem_type(gen->arena, expr->expr_type->as.array.element_type);
                return arena_sprintf(gen->arena, "((%s *)rt_array_data_v2(%s))",
                                     elem_c, call_expr);
            }
        }

        if (gen->expr_as_handle && !callee_has_body &&
            gen->current_arena_var != NULL &&
            expr->expr_type != NULL && is_handle_type(expr->expr_type))
        {
            if (expr->expr_type->kind == TYPE_STRING)
            {
                if (!callee_needs_arena)
                {
                    return arena_sprintf(gen->arena, "rt_arena_v2_strdup(%s, %s)",
                                         ARENA_VAR(gen), call_expr);
                }
            }
            else if (expr->expr_type->kind == TYPE_ARRAY)
            {
                Type *elem = expr->expr_type->as.array.element_type;
                if (elem != NULL && elem->kind == TYPE_STRING)
                {
                    if (!callee_needs_arena)
                    {
                        return arena_sprintf(gen->arena, "rt_array_from_legacy_string_v2(%s, %s)",
                                             ARENA_VAR(gen), call_expr);
                    }
                }
                else if (!callee_needs_arena)
                {
                    /* Native function returned raw pointer - wrap in handle */
                    const char *elem_c = get_c_array_elem_type(gen->arena, elem);
                    if (elem->kind == TYPE_STRING) {
                        return arena_sprintf(gen->arena,
                            "({ %s *__native_arr = %s; "
                            "rt_array_create_string_v2(%s, rt_v2_data_array_length((void *)__native_arr), __native_arr); })",
                            elem_c, call_expr, ARENA_VAR(gen));
                    }
                    return arena_sprintf(gen->arena,
                        "({ %s *__native_arr = %s; "
                        "rt_array_create_generic_v2(%s, rt_v2_data_array_length((void *)__native_arr), sizeof(%s), __native_arr); })",
                        elem_c, call_expr, ARENA_VAR(gen), elem_c);
                }
            }
        }
        return call_expr;
    }

    /* Complex path with temps - generate statement expression */
    char *result = arena_strdup(gen->arena, "({\n");

    for (int i = 0; i < call->arg_count; i++)
    {
        if (arg_is_temp[i])
        {
            result = arena_sprintf(gen->arena, "%s        char *%s = %s;\n", result, arg_base_names[i], arg_strs[i]);
        }
    }

    const char *ret_c = get_c_type(gen->arena, expr->expr_type);
    if (returns_void)
    {
        result = arena_sprintf(gen->arena, "%s        %s(%s);\n", result, callee_str, args_list);
    }
    else
    {
        result = arena_sprintf(gen->arena, "%s        %s _call_result = %s(%s);\n", result, ret_c, callee_str, args_list);
    }

    if (gen->current_arena_var == NULL)
    {
        for (int i = 0; i < call->arg_count; i++)
        {
            if (arg_is_temp[i])
            {
                result = arena_sprintf(gen->arena, "%s        rt_free_string(%s);\n", result, arg_base_names[i]);
            }
        }
    }

    if (returns_void)
    {
        result = arena_sprintf(gen->arena, "%s    })", result);
    }
    else if (!gen->expr_as_handle && callee_has_body &&
             gen->current_arena_var != NULL &&
             expr->expr_type != NULL && is_handle_type(expr->expr_type))
    {
        if (expr->expr_type->kind == TYPE_STRING)
        {
            result = arena_sprintf(gen->arena, "%s        (char *)rt_handle_v2_pin(_call_result);\n    })",
                                   result);
        }
        else
        {
            const char *elem_c = get_c_array_elem_type(gen->arena, expr->expr_type->as.array.element_type);
            result = arena_sprintf(gen->arena, "%s        (%s *)rt_array_data_v2(_call_result);\n    })",
                                   result, elem_c);
        }
    }
    else if (gen->expr_as_handle && !callee_has_body &&
             gen->current_arena_var != NULL &&
             expr->expr_type != NULL && is_handle_type(expr->expr_type))
    {
        if (expr->expr_type->kind == TYPE_STRING)
        {
            if (!callee_needs_arena)
            {
                result = arena_sprintf(gen->arena, "%s        rt_arena_v2_strdup(%s, _call_result);\n    })",
                                       result, ARENA_VAR(gen));
            }
            else
            {
                result = arena_sprintf(gen->arena, "%s        _call_result;\n    })", result);
            }
        }
        else
        {
            Type *elem = expr->expr_type->as.array.element_type;
            if (elem != NULL && elem->kind == TYPE_STRING)
            {
                result = arena_sprintf(gen->arena, "%s        rt_array_from_legacy_string_v2(%s, _call_result);\n    })",
                                       result, ARENA_VAR(gen));
            }
            else
            {
                /* Native function returned raw pointer - wrap in handle */
                const char *elem_c = get_c_array_elem_type(gen->arena, elem);
                result = arena_sprintf(gen->arena,
                    "%s        rt_array_create_generic_v2(%s, rt_v2_data_array_length((void *)_call_result), sizeof(%s), _call_result);\n    })",
                    result, ARENA_VAR(gen), elem_c);
            }
        }
    }
    else
    {
        result = arena_sprintf(gen->arena, "%s        _call_result;\n    })", result);
    }

    return result;
}

/* ============================================================================
 * Main Entry Point
 * ============================================================================ */

char *code_gen_call_expression(CodeGen *gen, Expr *expr)
{
    DEBUG_VERBOSE("Entering code_gen_call_expression");
    CallExpr *call = &expr->as.call;

    /* Handle member expression calls (method calls) */
    if (call->callee->type == EXPR_MEMBER)
    {
        char *result = code_gen_member_call(gen, expr, call);
        if (result) return result;
        /* Fall through to regular call processing if not handled */
    }

    /* Handle closure calls */
    if (is_closure_call_expr(gen, call))
    {
        return code_gen_closure_call(gen, expr, call);
    }

    /* Handle regular function calls */
    return code_gen_regular_call(gen, expr, call);
}
