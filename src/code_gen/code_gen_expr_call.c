/**
 * code_gen_expr_call.c - Code generation for call expressions
 *
 * This is the main dispatcher for generating C code from function calls
 * and method calls. It delegates to specialized handlers for different
 * object types (arrays, strings, files, etc.) defined in:
 * - code_gen_expr_call_array.c
 * - code_gen_expr_call_string.c
 * - code_gen_expr_call_file.c
 * - code_gen_expr_call_time.c
 */

#include "code_gen/code_gen_expr_call.h"
#include "code_gen/code_gen_expr.h"
#include "code_gen/code_gen_util.h"
#include "debug.h"
#include "symbol_table.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <ctype.h>

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

/**
 * Generate an intercepted function call.
 * This wraps a user-defined function call with interception logic:
 * - Fast path when no interceptors registered
 * - When interceptors present: box args, create thunk, call through chain
 *
 * Uses GNU C extensions (nested functions, statement expressions) which work
 * with GCC and Clang but not MSVC or TinyCC.
 *
 * @param gen           Code generator context
 * @param func_name     Name of the function being called
 * @param callee_str    Generated C code for the callee
 * @param args_list     Comma-separated list of generated argument expressions
 * @param call          The call expression AST node
 * @param arg_strs      Array of generated argument expression strings
 * @param param_types   Array of parameter types from function signature
 * @param param_count   Number of parameters
 * @param return_type   Return type of the function
 * @param callee_has_body Whether the callee function has a body (Sindarin function, needs arena arg)
 * @return Generated C code for the intercepted call
 */
static char *code_gen_intercepted_call(CodeGen *gen, const char *func_name,
                                       const char *callee_str, const char *args_list,
                                       CallExpr *call, char **arg_strs,
                                       Type **param_types, MemoryQualifier *param_quals,
                                       int param_count, Type *return_type, bool callee_has_body)
{
    Arena *arena = gen->arena;
    bool returns_void = (return_type && return_type->kind == TYPE_VOID);
    const char *ret_c = get_c_type(arena, return_type);

    /* Generate unique thunk ID */
    int thunk_id = gen->thunk_count++;
    char *thunk_name = arena_sprintf(arena, "__thunk_%d", thunk_id);

    /* Check if any parameters are 'as ref' */
    bool has_ref_params = false;
    if (param_quals != NULL)
    {
        for (int i = 0; i < param_count && i < call->arg_count; i++)
        {
            if (param_quals[i] == MEM_AS_REF)
            {
                has_ref_params = true;
                break;
            }
        }
    }

    /* Generate thunk forward declaration */
    gen->thunk_forward_decls = arena_sprintf(arena, "%sstatic RtAny %s(void);\n",
                                             gen->thunk_forward_decls, thunk_name);

    /* Generate thunk definition */
    char *thunk_def = arena_sprintf(arena, "static RtAny %s(void) {\n", thunk_name);

    /* For 'as ref' parameters, declare local variables to hold unboxed values */
    for (int i = 0; i < call->arg_count; i++)
    {
        bool is_ref = (param_quals != NULL && i < param_count && param_quals[i] == MEM_AS_REF);
        if (is_ref)
        {
            Type *arg_type = (param_types && i < param_count) ? param_types[i] : call->arguments[i]->expr_type;
            const char *c_type = get_c_type(arena, arg_type);
            const char *unbox_func = get_unboxing_function(arg_type);
            if (unbox_func != NULL)
            {
                thunk_def = arena_sprintf(arena, "%s    %s __ref_%d = %s(__rt_thunk_args[%d]);\n",
                                          thunk_def, c_type, i, unbox_func, i);
            }
        }
    }

    /* Build unboxed argument list for the thunk body */
    char *unboxed_args;
    if (callee_has_body)
    {
        unboxed_args = arena_strdup(arena, "(RtArena *)__rt_thunk_arena");
    }
    else
    {
        unboxed_args = arena_strdup(arena, "");
    }

    for (int i = 0; i < call->arg_count; i++)
    {
        Type *arg_type = (param_types && i < param_count) ? param_types[i] : call->arguments[i]->expr_type;
        const char *unbox_func = get_unboxing_function(arg_type);
        bool is_ref = (param_quals != NULL && i < param_count && param_quals[i] == MEM_AS_REF);

        bool need_comma = (i > 0) || callee_has_body;
        if (need_comma)
        {
            unboxed_args = arena_sprintf(arena, "%s, ", unboxed_args);
        }

        if (is_ref)
        {
            /* Pass address of local variable */
            unboxed_args = arena_sprintf(arena, "%s&__ref_%d", unboxed_args, i);
        }
        else if (unbox_func == NULL)
        {
            /* For 'any' type parameters, pass directly (already RtAny) */
            unboxed_args = arena_sprintf(arena, "%s__rt_thunk_args[%d]", unboxed_args, i);
        }
        else
        {
            unboxed_args = arena_sprintf(arena, "%s%s(__rt_thunk_args[%d])", unboxed_args, unbox_func, i);
        }
    }

    /* Make the actual function call */
    if (returns_void)
    {
        thunk_def = arena_sprintf(arena, "%s    %s(%s);\n", thunk_def, callee_str, unboxed_args);
    }
    else
    {
        const char *box_func = get_boxing_function(return_type);
        if (box_func == NULL)
        {
            /* Return type is 'any' - already boxed, no boxing needed */
            thunk_def = arena_sprintf(arena, "%s    RtAny __result = %s(%s);\n",
                                      thunk_def, callee_str, unboxed_args);
        }
        else if (return_type && return_type->kind == TYPE_ARRAY)
        {
            const char *elem_tag = get_element_type_tag(return_type->as.array.element_type);
            thunk_def = arena_sprintf(arena, "%s    RtAny __result = %s(%s(%s), %s);\n",
                                      thunk_def, box_func, callee_str, unboxed_args, elem_tag);
        }
        else
        {
            thunk_def = arena_sprintf(arena, "%s    RtAny __result = %s(%s(%s));\n",
                                      thunk_def, box_func, callee_str, unboxed_args);
        }
    }

    /* For 'as ref' parameters, write modified values back to args array */
    for (int i = 0; i < call->arg_count; i++)
    {
        bool is_ref = (param_quals != NULL && i < param_count && param_quals[i] == MEM_AS_REF);
        if (is_ref)
        {
            Type *arg_type = (param_types && i < param_count) ? param_types[i] : call->arguments[i]->expr_type;
            const char *box_func = get_boxing_function(arg_type);
            if (box_func != NULL)
            {
                thunk_def = arena_sprintf(arena, "%s    __rt_thunk_args[%d] = %s(__ref_%d);\n",
                                          thunk_def, i, box_func, i);
            }
        }
    }

    /* Return the result */
    if (returns_void)
    {
        thunk_def = arena_sprintf(arena, "%s    return rt_box_nil();\n", thunk_def);
    }
    else
    {
        thunk_def = arena_sprintf(arena, "%s    return __result;\n", thunk_def);
    }
    thunk_def = arena_sprintf(arena, "%s}\n", thunk_def);
    gen->thunk_definitions = arena_sprintf(arena, "%s%s\n", gen->thunk_definitions, thunk_def);

    /* Now generate the call site code */
    char *result = arena_strdup(arena, "({\n");

    /* Declare result variable */
    if (!returns_void)
    {
        result = arena_sprintf(arena, "%s    %s __intercept_result;\n", result, ret_c);
    }

    /* Fast path check */
    result = arena_sprintf(arena, "%s    if (__rt_interceptor_count > 0) {\n", result);

    /* Box arguments into RtAny array */
    result = arena_sprintf(arena, "%s        RtAny __args[%d];\n", result, call->arg_count > 0 ? call->arg_count : 1);
    for (int i = 0; i < call->arg_count; i++)
    {
        Type *arg_type = call->arguments[i]->expr_type;
        const char *box_func = get_boxing_function(arg_type);
        if (box_func == NULL)
        {
            /* Argument is already 'any' - no boxing needed */
            result = arena_sprintf(arena, "%s        __args[%d] = %s;\n",
                                   result, i, arg_strs[i]);
        }
        else if (arg_type && arg_type->kind == TYPE_ARRAY)
        {
            const char *elem_tag = get_element_type_tag(arg_type->as.array.element_type);
            result = arena_sprintf(arena, "%s        __args[%d] = %s(%s, %s);\n",
                                   result, i, box_func, arg_strs[i], elem_tag);
        }
        else
        {
            result = arena_sprintf(arena, "%s        __args[%d] = %s(%s);\n",
                                   result, i, box_func, arg_strs[i]);
        }
    }

    /* Set thread-local args and arena for thunk */
    result = arena_sprintf(arena, "%s        __rt_thunk_args = __args;\n", result);
    /* Set the thunk arena for interceptors - they return 'any' and need an arena.
     * Only set it when we have an arena; otherwise leave it unchanged so it can
     * inherit from an outer scope (e.g., thread wrapper) */
    if (gen->current_arena_var != NULL)
    {
        result = arena_sprintf(arena, "%s        __rt_thunk_arena = %s;\n", result, gen->current_arena_var);
    }

    /* Call through interceptor chain */
    result = arena_sprintf(arena, "%s        RtAny __intercepted = rt_call_intercepted(\"%s\", __args, %d, %s);\n",
                           result, func_name, call->arg_count, thunk_name);

    /* Unbox result */
    if (!returns_void)
    {
        const char *unbox_func = get_unboxing_function(return_type);
        if (unbox_func == NULL)
        {
            /* For 'any' return type, no unboxing needed */
            result = arena_sprintf(arena, "%s        __intercept_result = __intercepted;\n", result);
        }
        else
        {
            result = arena_sprintf(arena, "%s        __intercept_result = %s(__intercepted);\n", result, unbox_func);
        }
    }

    /* Write back modified values for 'as ref' parameters */
    if (has_ref_params)
    {
        for (int i = 0; i < call->arg_count; i++)
        {
            bool is_ref = (param_quals != NULL && i < param_count && param_quals[i] == MEM_AS_REF);
            if (is_ref)
            {
                Type *arg_type = (param_types && i < param_count) ? param_types[i] : call->arguments[i]->expr_type;
                const char *unbox_func = get_unboxing_function(arg_type);
                if (unbox_func != NULL)
                {
                    /* Write back to original variable: arg_strs[i] is the lvalue */
                    result = arena_sprintf(arena, "%s        %s = %s(__args[%d]);\n",
                                           result, arg_strs[i], unbox_func, i);
                }
            }
        }
    }

    /* Close interceptor branch, add fast path */
    result = arena_sprintf(arena, "%s    } else {\n", result);
    if (returns_void)
    {
        result = arena_sprintf(arena, "%s        %s(%s);\n", result, callee_str, args_list);
    }
    else
    {
        result = arena_sprintf(arena, "%s        __intercept_result = %s(%s);\n", result, callee_str, args_list);
    }
    result = arena_sprintf(arena, "%s    }\n", result);

    /* Return result */
    if (returns_void)
    {
        result = arena_sprintf(arena, "%s    (void)0;\n})", result);
    }
    else
    {
        result = arena_sprintf(arena, "%s    __intercept_result;\n})", result);
    }

    return result;
}

char *code_gen_call_expression(CodeGen *gen, Expr *expr)
{
    DEBUG_VERBOSE("Entering code_gen_call_expression");
    CallExpr *call = &expr->as.call;

    if (call->callee->type == EXPR_MEMBER) {
        MemberExpr *member = &call->callee->as.member;
        char *member_name_str = get_var_name(gen->arena, member->member_name);
        Type *object_type = member->object->expr_type;

        /* Check for namespace function call (namespace.function).
         * If the object has no type (expr_type is NULL) and is a variable,
         * this is a namespaced function call. Type checker already validated
         * this, so we can safely emit the function call directly using the
         * member name as the function name. */
        if (object_type == NULL && member->object->type == EXPR_VARIABLE)
        {
            /* Lookup the function in the namespace to check if it has a body and c_alias */
            Token ns_name = member->object->as.variable.name;
            Symbol *func_sym = symbol_table_lookup_in_namespace(gen->symbol_table, ns_name, member->member_name);
            bool callee_has_body = (func_sym != NULL &&
                                    func_sym->type != NULL &&
                                    func_sym->type->kind == TYPE_FUNCTION &&
                                    func_sym->type->as.function.has_body);

            /* Use c_alias for native functions if available */
            const char *func_name_to_use = member_name_str;
            if (func_sym != NULL && func_sym->is_native && func_sym->c_alias != NULL)
            {
                func_name_to_use = func_sym->c_alias;
            }

            /* Generate arguments */
            char **arg_strs = arena_alloc(gen->arena, call->arg_count * sizeof(char *));
            for (int i = 0; i < call->arg_count; i++)
            {
                arg_strs[i] = code_gen_expression(gen, call->arguments[i]);
            }

            /* Build args list - prepend arena if function has body (Sindarin function) */
            char *args_list;
            if (callee_has_body && gen->current_arena_var != NULL)
            {
                args_list = arena_strdup(gen->arena, gen->current_arena_var);
            }
            else if (callee_has_body)
            {
                args_list = arena_strdup(gen->arena, "NULL");
            }
            else
            {
                args_list = arena_strdup(gen->arena, "");
            }

            for (int i = 0; i < call->arg_count; i++)
            {
                if (args_list[0] == '\0')
                {
                    args_list = arg_strs[i];
                }
                else
                {
                    args_list = arena_sprintf(gen->arena, "%s, %s", args_list, arg_strs[i]);
                }
            }

            /* Emit function call using the resolved function name */
            return arena_sprintf(gen->arena, "%s(%s)", func_name_to_use, args_list);
        }

        char *result = NULL;

        /* Dispatch to type-specific handlers (modular code generation)
         * Each handler returns NULL if it doesn't handle the method,
         * allowing fallback to the original inline implementations.
         */
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
                /* Check if this is a user-defined struct method call */
                if (member->resolved_method != NULL)
                {
                    StructMethod *method = member->resolved_method;
                    Type *struct_type = member->resolved_struct_type;
                    const char *struct_name = struct_type->as.struct_type.name;

                    if (method->is_native)
                    {
                        /* Native method call - use c_alias if present, else use naming convention */
                        const char *func_name;
                        if (method->c_alias != NULL)
                        {
                            /* Use explicit c_alias from #pragma alias */
                            func_name = method->c_alias;
                        }
                        else
                        {
                            /* Create lowercase struct name for native method naming */
                            char *struct_name_lower = arena_strdup(gen->arena, struct_name);
                            for (char *p = struct_name_lower; *p; p++)
                            {
                                *p = (char)tolower((unsigned char)*p);
                            }
                            func_name = arena_sprintf(gen->arena, "rt_%s_%s", struct_name_lower, method->name);
                        }

                        /* Build args list - NO arena for native methods */
                        char *args_list = arena_strdup(gen->arena, "");

                        /* For instance native methods, pass self as first arg */
                        /* Check pass_self_by_ref to determine if we pass by pointer or by value */
                        if (!method->is_static)
                        {
                            char *self_str = code_gen_expression(gen, member->object);
                            /* For opaque handle types (native struct with c_alias), self is already a pointer */
                            if (struct_type->as.struct_type.is_native && struct_type->as.struct_type.c_alias != NULL)
                            {
                                /* Opaque handle: self is already a pointer, pass directly */
                                args_list = arena_strdup(gen->arena, self_str);
                            }
                            else if (struct_type->as.struct_type.pass_self_by_ref)
                            {
                                /* Pass by reference (pointer) */
                                args_list = arena_sprintf(gen->arena, "&%s", self_str);
                            }
                            else
                            {
                                /* Pass by value */
                                args_list = arena_strdup(gen->arena, self_str);
                            }
                        }

                        /* Add remaining arguments */
                        for (int i = 0; i < call->arg_count; i++)
                        {
                            char *arg_str = code_gen_expression(gen, call->arguments[i]);
                            if (args_list[0] != '\0')
                            {
                                args_list = arena_sprintf(gen->arena, "%s, %s", args_list, arg_str);
                            }
                            else
                            {
                                args_list = arg_str;
                            }
                        }

                        return arena_sprintf(gen->arena, "%s(%s)", func_name, args_list);
                    }
                    else
                    {
                        /* Non-native method call: StructName_methodName(arena, self, args) */
                        char *args_list = arena_strdup(gen->arena, ARENA_VAR(gen));

                        /* For instance methods, pass self */
                        if (!method->is_static)
                        {
                            char *self_str = code_gen_expression(gen, member->object);
                            /* For opaque handle types, self is already a pointer */
                            if (struct_type->as.struct_type.is_native && struct_type->as.struct_type.c_alias != NULL)
                            {
                                /* Opaque handle: self is already a pointer */
                                args_list = arena_sprintf(gen->arena, "%s, %s", args_list, self_str);
                            }
                            else
                            {
                                /* Regular struct: take address of self */
                                args_list = arena_sprintf(gen->arena, "%s, &%s", args_list, self_str);
                            }
                        }

                        /* Generate other arguments */
                        for (int i = 0; i < call->arg_count; i++)
                        {
                            char *arg_str = code_gen_expression(gen, call->arguments[i]);
                            args_list = arena_sprintf(gen->arena, "%s, %s", args_list, arg_str);
                        }

                        return arena_sprintf(gen->arena, "%s_%s(%s)",
                                             struct_name, method->name, args_list);
                    }
                }
                break;
            }
            default:
                break;
        }

        /* Fallback to original inline implementations for methods not yet
         * handled by the modular handlers (e.g., append for strings).
         */
        if (object_type->kind == TYPE_ARRAY) {
            /* Array methods - fallback for methods not in modular handler */
            char *object_str = code_gen_expression(gen, member->object);
            Type *element_type = object_type->as.array.element_type;

            // Handle push(element)
            if (strcmp(member_name_str, "push") == 0 && call->arg_count == 1) {
                char *arg_str = code_gen_expression(gen, call->arguments[0]);
                Type *arg_type = call->arguments[0]->expr_type;

                if (!ast_type_equals(element_type, arg_type)) {
                    fprintf(stderr, "Error: Argument type does not match array element type\n");
                    exit(1);
                }

                const char *push_func = NULL;
                switch (element_type->kind) {
                    case TYPE_LONG:
                    case TYPE_INT:
                        push_func = "rt_array_push_long";
                        break;
                    case TYPE_DOUBLE:
                        push_func = "rt_array_push_double";
                        break;
                    case TYPE_CHAR:
                        push_func = "rt_array_push_char";
                        break;
                    case TYPE_STRING:
                        push_func = "rt_array_push_string";
                        break;
                    case TYPE_BOOL:
                        push_func = "rt_array_push_bool";
                        break;
                    case TYPE_BYTE:
                        push_func = "rt_array_push_byte";
                        break;
                    case TYPE_FUNCTION:
                    case TYPE_ARRAY:
                        push_func = "rt_array_push_ptr";
                        break;
                    case TYPE_ANY:
                        push_func = "rt_array_push_any";
                        break;
                    case TYPE_INT32:
                        push_func = "rt_array_push_int32";
                        break;
                    case TYPE_UINT:
                        push_func = "rt_array_push_uint";
                        break;
                    case TYPE_UINT32:
                        push_func = "rt_array_push_uint32";
                        break;
                    case TYPE_FLOAT:
                        push_func = "rt_array_push_float";
                        break;
                    default:
                        fprintf(stderr, "Error: Unsupported array element type for push\n");
                        exit(1);
                }
                // push returns new array pointer, assign back to variable if object is a variable
                // For global variables, use NULL arena to trigger malloc-based allocation
                // that persists beyond any function's lifetime.
                // Global variables are detected by declaration_scope_depth <= 1 (the initial global scope).
                const char *arena_to_use = ARENA_VAR(gen);
                if (member->object->type == EXPR_VARIABLE) {
                    Symbol *sym = symbol_table_lookup_symbol(gen->symbol_table, member->object->as.variable.name);
                    if (sym != NULL && (sym->kind == SYMBOL_GLOBAL || sym->declaration_scope_depth <= 1)) {
                        arena_to_use = "NULL";
                    }
                }

                // For pointer types (function/array), we need to cast to void**
                if (element_type->kind == TYPE_FUNCTION || element_type->kind == TYPE_ARRAY) {
                    if (member->object->type == EXPR_VARIABLE) {
                        return arena_sprintf(gen->arena, "(%s = (void *)%s(%s, (void **)%s, (void *)%s))", object_str, push_func, arena_to_use, object_str, arg_str);
                    }
                    return arena_sprintf(gen->arena, "(void *)%s(%s, (void **)%s, (void *)%s)", push_func, arena_to_use, object_str, arg_str);
                }
                if (member->object->type == EXPR_VARIABLE) {
                    return arena_sprintf(gen->arena, "(%s = %s(%s, %s, %s))", object_str, push_func, arena_to_use, object_str, arg_str);
                }
                return arena_sprintf(gen->arena, "%s(%s, %s, %s)", push_func, arena_to_use, object_str, arg_str);
            }

            // Handle clear()
            if (strcmp(member_name_str, "clear") == 0 && call->arg_count == 0) {
                return arena_sprintf(gen->arena, "rt_array_clear(%s)", object_str);
            }

            // Handle pop()
            if (strcmp(member_name_str, "pop") == 0 && call->arg_count == 0) {
                const char *pop_func = NULL;
                switch (element_type->kind) {
                    case TYPE_LONG:
                    case TYPE_INT:
                        pop_func = "rt_array_pop_long";
                        break;
                    case TYPE_DOUBLE:
                        pop_func = "rt_array_pop_double";
                        break;
                    case TYPE_CHAR:
                        pop_func = "rt_array_pop_char";
                        break;
                    case TYPE_STRING:
                        pop_func = "rt_array_pop_string";
                        break;
                    case TYPE_BOOL:
                        pop_func = "rt_array_pop_bool";
                        break;
                    case TYPE_BYTE:
                        pop_func = "rt_array_pop_byte";
                        break;
                    case TYPE_FUNCTION:
                    case TYPE_ARRAY:
                        pop_func = "rt_array_pop_ptr";
                        break;
                    case TYPE_INT32:
                        pop_func = "rt_array_pop_int32";
                        break;
                    case TYPE_UINT:
                        pop_func = "rt_array_pop_uint";
                        break;
                    case TYPE_UINT32:
                        pop_func = "rt_array_pop_uint32";
                        break;
                    case TYPE_FLOAT:
                        pop_func = "rt_array_pop_float";
                        break;
                    default:
                        fprintf(stderr, "Error: Unsupported array element type for pop\n");
                        exit(1);
                }
                // For pointer types (function/array), we need to cast the result
                if (element_type->kind == TYPE_FUNCTION || element_type->kind == TYPE_ARRAY) {
                    const char *elem_type_str = get_c_type(gen->arena, element_type);
                    return arena_sprintf(gen->arena, "(%s)%s((void **)%s)", elem_type_str, pop_func, object_str);
                }
                return arena_sprintf(gen->arena, "%s(%s)", pop_func, object_str);
            }

            // Handle concat(other_array)
            if (strcmp(member_name_str, "concat") == 0 && call->arg_count == 1) {
                char *arg_str = code_gen_expression(gen, call->arguments[0]);
                const char *concat_func = NULL;
                switch (element_type->kind) {
                    case TYPE_LONG:
                    case TYPE_INT:
                        concat_func = "rt_array_concat_long";
                        break;
                    case TYPE_DOUBLE:
                        concat_func = "rt_array_concat_double";
                        break;
                    case TYPE_CHAR:
                        concat_func = "rt_array_concat_char";
                        break;
                    case TYPE_STRING:
                        concat_func = "rt_array_concat_string";
                        break;
                    case TYPE_BOOL:
                        concat_func = "rt_array_concat_bool";
                        break;
                    case TYPE_BYTE:
                        concat_func = "rt_array_concat_byte";
                        break;
                    case TYPE_FUNCTION:
                    case TYPE_ARRAY:
                        concat_func = "rt_array_concat_ptr";
                        break;
                    case TYPE_INT32:
                        concat_func = "rt_array_concat_int32";
                        break;
                    case TYPE_UINT:
                        concat_func = "rt_array_concat_uint";
                        break;
                    case TYPE_UINT32:
                        concat_func = "rt_array_concat_uint32";
                        break;
                    case TYPE_FLOAT:
                        concat_func = "rt_array_concat_float";
                        break;
                    default:
                        fprintf(stderr, "Error: Unsupported array element type for concat\n");
                        exit(1);
                }
                // concat returns a new array, doesn't modify the original
                // For pointer types (function/array), we need to cast
                if (element_type->kind == TYPE_FUNCTION || element_type->kind == TYPE_ARRAY) {
                    const char *elem_type_str = get_c_type(gen->arena, element_type);
                    return arena_sprintf(gen->arena, "(%s *)%s(%s, (void **)%s, (void **)%s)", elem_type_str, concat_func, ARENA_VAR(gen), object_str, arg_str);
                }
                return arena_sprintf(gen->arena, "%s(%s, %s, %s)", concat_func, ARENA_VAR(gen), object_str, arg_str);
            }

            // Handle indexOf(element)
            if (strcmp(member_name_str, "indexOf") == 0 && call->arg_count == 1) {
                char *arg_str = code_gen_expression(gen, call->arguments[0]);
                const char *indexof_func = NULL;
                switch (element_type->kind) {
                    case TYPE_LONG:
                    case TYPE_INT:
                        indexof_func = "rt_array_indexOf_long";
                        break;
                    case TYPE_DOUBLE:
                        indexof_func = "rt_array_indexOf_double";
                        break;
                    case TYPE_CHAR:
                        indexof_func = "rt_array_indexOf_char";
                        break;
                    case TYPE_STRING:
                        indexof_func = "rt_array_indexOf_string";
                        break;
                    case TYPE_BOOL:
                        indexof_func = "rt_array_indexOf_bool";
                        break;
                    case TYPE_BYTE:
                        indexof_func = "rt_array_indexOf_byte";
                        break;
                    case TYPE_INT32:
                        indexof_func = "rt_array_indexOf_int32";
                        break;
                    case TYPE_UINT:
                        indexof_func = "rt_array_indexOf_uint";
                        break;
                    case TYPE_UINT32:
                        indexof_func = "rt_array_indexOf_uint32";
                        break;
                    case TYPE_FLOAT:
                        indexof_func = "rt_array_indexOf_float";
                        break;
                    default:
                        fprintf(stderr, "Error: Unsupported array element type for indexOf\n");
                        exit(1);
                }
                return arena_sprintf(gen->arena, "%s(%s, %s)", indexof_func, object_str, arg_str);
            }

            // Handle contains(element)
            if (strcmp(member_name_str, "contains") == 0 && call->arg_count == 1) {
                char *arg_str = code_gen_expression(gen, call->arguments[0]);
                const char *contains_func = NULL;
                switch (element_type->kind) {
                    case TYPE_LONG:
                    case TYPE_INT:
                        contains_func = "rt_array_contains_long";
                        break;
                    case TYPE_DOUBLE:
                        contains_func = "rt_array_contains_double";
                        break;
                    case TYPE_CHAR:
                        contains_func = "rt_array_contains_char";
                        break;
                    case TYPE_STRING:
                        contains_func = "rt_array_contains_string";
                        break;
                    case TYPE_BOOL:
                        contains_func = "rt_array_contains_bool";
                        break;
                    case TYPE_BYTE:
                        contains_func = "rt_array_contains_byte";
                        break;
                    case TYPE_INT32:
                        contains_func = "rt_array_contains_int32";
                        break;
                    case TYPE_UINT:
                        contains_func = "rt_array_contains_uint";
                        break;
                    case TYPE_UINT32:
                        contains_func = "rt_array_contains_uint32";
                        break;
                    case TYPE_FLOAT:
                        contains_func = "rt_array_contains_float";
                        break;
                    default:
                        fprintf(stderr, "Error: Unsupported array element type for contains\n");
                        exit(1);
                }
                return arena_sprintf(gen->arena, "%s(%s, %s)", contains_func, object_str, arg_str);
            }

            // Handle clone()
            if (strcmp(member_name_str, "clone") == 0 && call->arg_count == 0) {
                const char *clone_func = NULL;
                switch (element_type->kind) {
                    case TYPE_LONG:
                    case TYPE_INT:
                        clone_func = "rt_array_clone_long";
                        break;
                    case TYPE_DOUBLE:
                        clone_func = "rt_array_clone_double";
                        break;
                    case TYPE_CHAR:
                        clone_func = "rt_array_clone_char";
                        break;
                    case TYPE_STRING:
                        clone_func = "rt_array_clone_string";
                        break;
                    case TYPE_BOOL:
                        clone_func = "rt_array_clone_bool";
                        break;
                    case TYPE_BYTE:
                        clone_func = "rt_array_clone_byte";
                        break;
                    case TYPE_INT32:
                        clone_func = "rt_array_clone_int32";
                        break;
                    case TYPE_UINT:
                        clone_func = "rt_array_clone_uint";
                        break;
                    case TYPE_UINT32:
                        clone_func = "rt_array_clone_uint32";
                        break;
                    case TYPE_FLOAT:
                        clone_func = "rt_array_clone_float";
                        break;
                    default:
                        fprintf(stderr, "Error: Unsupported array element type for clone\n");
                        exit(1);
                }
                return arena_sprintf(gen->arena, "%s(%s, %s)", clone_func, ARENA_VAR(gen), object_str);
            }

            // Handle join(separator)
            if (strcmp(member_name_str, "join") == 0 && call->arg_count == 1) {
                char *arg_str = code_gen_expression(gen, call->arguments[0]);
                const char *join_func = NULL;
                switch (element_type->kind) {
                    case TYPE_LONG:
                    case TYPE_INT:
                        join_func = "rt_array_join_long";
                        break;
                    case TYPE_DOUBLE:
                        join_func = "rt_array_join_double";
                        break;
                    case TYPE_CHAR:
                        join_func = "rt_array_join_char";
                        break;
                    case TYPE_STRING:
                        join_func = "rt_array_join_string";
                        break;
                    case TYPE_BOOL:
                        join_func = "rt_array_join_bool";
                        break;
                    case TYPE_BYTE:
                        join_func = "rt_array_join_byte";
                        break;
                    case TYPE_INT32:
                        join_func = "rt_array_join_int32";
                        break;
                    case TYPE_UINT:
                        join_func = "rt_array_join_uint";
                        break;
                    case TYPE_UINT32:
                        join_func = "rt_array_join_uint32";
                        break;
                    case TYPE_FLOAT:
                        join_func = "rt_array_join_float";
                        break;
                    default:
                        fprintf(stderr, "Error: Unsupported array element type for join\n");
                        exit(1);
                }
                return arena_sprintf(gen->arena, "%s(%s, %s, %s)", join_func, ARENA_VAR(gen), object_str, arg_str);
            }

            // Handle reverse() - in-place reverse
            if (strcmp(member_name_str, "reverse") == 0 && call->arg_count == 0) {
                const char *rev_func = NULL;
                switch (element_type->kind) {
                    case TYPE_LONG:
                    case TYPE_INT:
                        rev_func = "rt_array_rev_long";
                        break;
                    case TYPE_DOUBLE:
                        rev_func = "rt_array_rev_double";
                        break;
                    case TYPE_CHAR:
                        rev_func = "rt_array_rev_char";
                        break;
                    case TYPE_STRING:
                        rev_func = "rt_array_rev_string";
                        break;
                    case TYPE_BOOL:
                        rev_func = "rt_array_rev_bool";
                        break;
                    case TYPE_BYTE:
                        rev_func = "rt_array_rev_byte";
                        break;
                    case TYPE_INT32:
                        rev_func = "rt_array_rev_int32";
                        break;
                    case TYPE_UINT:
                        rev_func = "rt_array_rev_uint";
                        break;
                    case TYPE_UINT32:
                        rev_func = "rt_array_rev_uint32";
                        break;
                    case TYPE_FLOAT:
                        rev_func = "rt_array_rev_float";
                        break;
                    default:
                        fprintf(stderr, "Error: Unsupported array element type for reverse\n");
                        exit(1);
                }
                // reverse in-place: assign result back to variable
                if (member->object->type == EXPR_VARIABLE) {
                    return arena_sprintf(gen->arena, "(%s = %s(%s, %s))", object_str, rev_func, ARENA_VAR(gen), object_str);
                }
                return arena_sprintf(gen->arena, "%s(%s, %s)", rev_func, ARENA_VAR(gen), object_str);
            }

            // Handle insert(elem, index)
            if (strcmp(member_name_str, "insert") == 0 && call->arg_count == 2) {
                char *elem_str = code_gen_expression(gen, call->arguments[0]);
                char *idx_str = code_gen_expression(gen, call->arguments[1]);
                const char *ins_func = NULL;
                switch (element_type->kind) {
                    case TYPE_LONG:
                    case TYPE_INT:
                        ins_func = "rt_array_ins_long";
                        break;
                    case TYPE_DOUBLE:
                        ins_func = "rt_array_ins_double";
                        break;
                    case TYPE_CHAR:
                        ins_func = "rt_array_ins_char";
                        break;
                    case TYPE_STRING:
                        ins_func = "rt_array_ins_string";
                        break;
                    case TYPE_BOOL:
                        ins_func = "rt_array_ins_bool";
                        break;
                    case TYPE_BYTE:
                        ins_func = "rt_array_ins_byte";
                        break;
                    case TYPE_INT32:
                        ins_func = "rt_array_ins_int32";
                        break;
                    case TYPE_UINT:
                        ins_func = "rt_array_ins_uint";
                        break;
                    case TYPE_UINT32:
                        ins_func = "rt_array_ins_uint32";
                        break;
                    case TYPE_FLOAT:
                        ins_func = "rt_array_ins_float";
                        break;
                    default:
                        fprintf(stderr, "Error: Unsupported array element type for insert\n");
                        exit(1);
                }
                // insert in-place: assign result back to variable
                if (member->object->type == EXPR_VARIABLE) {
                    return arena_sprintf(gen->arena, "(%s = %s(%s, %s, %s, %s))", object_str, ins_func, ARENA_VAR(gen), object_str, elem_str, idx_str);
                }
                return arena_sprintf(gen->arena, "%s(%s, %s, %s, %s)", ins_func, ARENA_VAR(gen), object_str, elem_str, idx_str);
            }

            // Handle remove(index)
            if (strcmp(member_name_str, "remove") == 0 && call->arg_count == 1) {
                char *idx_str = code_gen_expression(gen, call->arguments[0]);
                const char *rem_func = NULL;
                switch (element_type->kind) {
                    case TYPE_LONG:
                    case TYPE_INT:
                        rem_func = "rt_array_rem_long";
                        break;
                    case TYPE_DOUBLE:
                        rem_func = "rt_array_rem_double";
                        break;
                    case TYPE_CHAR:
                        rem_func = "rt_array_rem_char";
                        break;
                    case TYPE_STRING:
                        rem_func = "rt_array_rem_string";
                        break;
                    case TYPE_BOOL:
                        rem_func = "rt_array_rem_bool";
                        break;
                    case TYPE_BYTE:
                        rem_func = "rt_array_rem_byte";
                        break;
                    case TYPE_INT32:
                        rem_func = "rt_array_rem_int32";
                        break;
                    case TYPE_UINT:
                        rem_func = "rt_array_rem_uint";
                        break;
                    case TYPE_UINT32:
                        rem_func = "rt_array_rem_uint32";
                        break;
                    case TYPE_FLOAT:
                        rem_func = "rt_array_rem_float";
                        break;
                    default:
                        fprintf(stderr, "Error: Unsupported array element type for remove\n");
                        exit(1);
                }
                // remove in-place: assign result back to variable
                if (member->object->type == EXPR_VARIABLE) {
                    return arena_sprintf(gen->arena, "(%s = %s(%s, %s, %s))", object_str, rem_func, ARENA_VAR(gen), object_str, idx_str);
                }
                return arena_sprintf(gen->arena, "%s(%s, %s, %s)", rem_func, ARENA_VAR(gen), object_str, idx_str);
            }

            /* Byte array extension methods - only for byte[] */
            if (element_type->kind == TYPE_BYTE) {
                // Handle toString() - UTF-8 decoding
                if (strcmp(member_name_str, "toString") == 0 && call->arg_count == 0) {
                    return arena_sprintf(gen->arena, "rt_byte_array_to_string(%s, %s)",
                        ARENA_VAR(gen), object_str);
                }

                // Handle toStringLatin1() - Latin-1/ISO-8859-1 decoding
                if (strcmp(member_name_str, "toStringLatin1") == 0 && call->arg_count == 0) {
                    return arena_sprintf(gen->arena, "rt_byte_array_to_string_latin1(%s, %s)",
                        ARENA_VAR(gen), object_str);
                }

                // Handle toHex() - hexadecimal encoding
                if (strcmp(member_name_str, "toHex") == 0 && call->arg_count == 0) {
                    return arena_sprintf(gen->arena, "rt_byte_array_to_hex(%s, %s)",
                        ARENA_VAR(gen), object_str);
                }

                // Handle toBase64() - Base64 encoding
                if (strcmp(member_name_str, "toBase64") == 0 && call->arg_count == 0) {
                    return arena_sprintf(gen->arena, "rt_byte_array_to_base64(%s, %s)",
                        ARENA_VAR(gen), object_str);
                }
            }
        }

        /* Handle string methods
         * NOTE: These methods are also implemented in code_gen_expr_call_string.c
         * for modular code generation. The implementations here remain for
         * backward compatibility during the transition.
         */
        if (object_type->kind == TYPE_STRING) {
            char *object_str = code_gen_expression(gen, member->object);
            bool object_is_temp = expression_produces_temp(member->object);

            // Helper macro-like pattern for string methods that return strings
            // If object is temp, we need to capture it, call method, free it, return result
            // Skip freeing in arena context - arena handles cleanup
            #define STRING_METHOD_RETURNING_STRING(method_call) \
                do { \
                    if (object_is_temp) { \
                        if (gen->current_arena_var != NULL) { \
                            return arena_sprintf(gen->arena, \
                                "({ char *_obj_tmp = %s; char *_res = %s; _res; })", \
                                object_str, method_call); \
                        } else { \
                            return arena_sprintf(gen->arena, \
                                "({ char *_obj_tmp = %s; char *_res = %s; rt_free_string(_obj_tmp); _res; })", \
                                object_str, method_call); \
                        } \
                    } else { \
                        return arena_sprintf(gen->arena, "%s", method_call); \
                    } \
                } while(0)

            // Handle substring(start, end) - returns string
            if (strcmp(member_name_str, "substring") == 0 && call->arg_count == 2) {
                char *start_str = code_gen_expression(gen, call->arguments[0]);
                char *end_str = code_gen_expression(gen, call->arguments[1]);
                char *method_call = arena_sprintf(gen->arena, "rt_str_substring(%s, %s, %s, %s)",
                    ARENA_VAR(gen), object_is_temp ? "_obj_tmp" : object_str, start_str, end_str);
                STRING_METHOD_RETURNING_STRING(method_call);
            }

            // Handle regionEquals(start, end, pattern) - non-allocating comparison, returns bool
            if (strcmp(member_name_str, "regionEquals") == 0 && call->arg_count == 3) {
                char *start_str = code_gen_expression(gen, call->arguments[0]);
                char *end_str = code_gen_expression(gen, call->arguments[1]);
                char *pattern_str = code_gen_expression(gen, call->arguments[2]);
                if (object_is_temp) {
                    return arena_sprintf(gen->arena,
                        "({ char *_obj_tmp = %s; int _res = rt_str_region_equals(_obj_tmp, %s, %s, %s); _res; })",
                        object_str, start_str, end_str, pattern_str);
                }
                return arena_sprintf(gen->arena, "rt_str_region_equals(%s, %s, %s, %s)",
                    object_str, start_str, end_str, pattern_str);
            }

            // Handle indexOf(search) - returns int, no string cleanup needed for result
            if (strcmp(member_name_str, "indexOf") == 0 && call->arg_count == 1) {
                char *arg_str = code_gen_expression(gen, call->arguments[0]);
                if (object_is_temp) {
                    if (gen->current_arena_var != NULL) {
                        return arena_sprintf(gen->arena,
                            "({ char *_obj_tmp = %s; long _res = rt_str_indexOf(_obj_tmp, %s); _res; })",
                            object_str, arg_str);
                    }
                    return arena_sprintf(gen->arena,
                        "({ char *_obj_tmp = %s; long _res = rt_str_indexOf(_obj_tmp, %s); rt_free_string(_obj_tmp); _res; })",
                        object_str, arg_str);
                }
                return arena_sprintf(gen->arena, "rt_str_indexOf(%s, %s)", object_str, arg_str);
            }

            // Handle split(delimiter) - returns array, object cleanup needed
            if (strcmp(member_name_str, "split") == 0 && call->arg_count == 1) {
                char *arg_str = code_gen_expression(gen, call->arguments[0]);
                if (object_is_temp) {
                    if (gen->current_arena_var != NULL) {
                        return arena_sprintf(gen->arena,
                            "({ char *_obj_tmp = %s; char **_res = rt_str_split(%s, _obj_tmp, %s); _res; })",
                            object_str, ARENA_VAR(gen), arg_str);
                    }
                    return arena_sprintf(gen->arena,
                        "({ char *_obj_tmp = %s; char **_res = rt_str_split(%s, _obj_tmp, %s); rt_free_string(_obj_tmp); _res; })",
                        object_str, ARENA_VAR(gen), arg_str);
                }
                return arena_sprintf(gen->arena, "rt_str_split(%s, %s, %s)", ARENA_VAR(gen), object_str, arg_str);
            }

            // Handle trim() - returns string
            if (strcmp(member_name_str, "trim") == 0 && call->arg_count == 0) {
                char *method_call = arena_sprintf(gen->arena, "rt_str_trim(%s, %s)",
                    ARENA_VAR(gen), object_is_temp ? "_obj_tmp" : object_str);
                STRING_METHOD_RETURNING_STRING(method_call);
            }

            // Handle toUpper() - returns string
            if (strcmp(member_name_str, "toUpper") == 0 && call->arg_count == 0) {
                char *method_call = arena_sprintf(gen->arena, "rt_str_toUpper(%s, %s)",
                    ARENA_VAR(gen), object_is_temp ? "_obj_tmp" : object_str);
                STRING_METHOD_RETURNING_STRING(method_call);
            }

            // Handle toLower() - returns string
            if (strcmp(member_name_str, "toLower") == 0 && call->arg_count == 0) {
                char *method_call = arena_sprintf(gen->arena, "rt_str_toLower(%s, %s)",
                    ARENA_VAR(gen), object_is_temp ? "_obj_tmp" : object_str);
                STRING_METHOD_RETURNING_STRING(method_call);
            }

            // Handle startsWith(prefix) - returns bool
            if (strcmp(member_name_str, "startsWith") == 0 && call->arg_count == 1) {
                char *arg_str = code_gen_expression(gen, call->arguments[0]);
                if (object_is_temp) {
                    if (gen->current_arena_var != NULL) {
                        return arena_sprintf(gen->arena,
                            "({ char *_obj_tmp = %s; int _res = rt_str_startsWith(_obj_tmp, %s); _res; })",
                            object_str, arg_str);
                    }
                    return arena_sprintf(gen->arena,
                        "({ char *_obj_tmp = %s; int _res = rt_str_startsWith(_obj_tmp, %s); rt_free_string(_obj_tmp); _res; })",
                        object_str, arg_str);
                }
                return arena_sprintf(gen->arena, "rt_str_startsWith(%s, %s)", object_str, arg_str);
            }

            // Handle endsWith(suffix) - returns bool
            if (strcmp(member_name_str, "endsWith") == 0 && call->arg_count == 1) {
                char *arg_str = code_gen_expression(gen, call->arguments[0]);
                if (object_is_temp) {
                    if (gen->current_arena_var != NULL) {
                        return arena_sprintf(gen->arena,
                            "({ char *_obj_tmp = %s; int _res = rt_str_endsWith(_obj_tmp, %s); _res; })",
                            object_str, arg_str);
                    }
                    return arena_sprintf(gen->arena,
                        "({ char *_obj_tmp = %s; int _res = rt_str_endsWith(_obj_tmp, %s); rt_free_string(_obj_tmp); _res; })",
                        object_str, arg_str);
                }
                return arena_sprintf(gen->arena, "rt_str_endsWith(%s, %s)", object_str, arg_str);
            }

            // Handle contains(search) - returns bool
            if (strcmp(member_name_str, "contains") == 0 && call->arg_count == 1) {
                char *arg_str = code_gen_expression(gen, call->arguments[0]);
                if (object_is_temp) {
                    if (gen->current_arena_var != NULL) {
                        return arena_sprintf(gen->arena,
                            "({ char *_obj_tmp = %s; int _res = rt_str_contains(_obj_tmp, %s); _res; })",
                            object_str, arg_str);
                    }
                    return arena_sprintf(gen->arena,
                        "({ char *_obj_tmp = %s; int _res = rt_str_contains(_obj_tmp, %s); rt_free_string(_obj_tmp); _res; })",
                        object_str, arg_str);
                }
                return arena_sprintf(gen->arena, "rt_str_contains(%s, %s)", object_str, arg_str);
            }

            // Handle replace(old, new) - returns string
            if (strcmp(member_name_str, "replace") == 0 && call->arg_count == 2) {
                char *old_str = code_gen_expression(gen, call->arguments[0]);
                char *new_str = code_gen_expression(gen, call->arguments[1]);
                char *method_call = arena_sprintf(gen->arena, "rt_str_replace(%s, %s, %s, %s)",
                    ARENA_VAR(gen), object_is_temp ? "_obj_tmp" : object_str, old_str, new_str);
                STRING_METHOD_RETURNING_STRING(method_call);
            }

            // Handle charAt(index) - returns char
            if (strcmp(member_name_str, "charAt") == 0 && call->arg_count == 1) {
                char *index_str = code_gen_expression(gen, call->arguments[0]);
                if (object_is_temp) {
                    if (gen->current_arena_var != NULL) {
                        return arena_sprintf(gen->arena,
                            "({ char *_obj_tmp = %s; char _res = (char)rt_str_charAt(_obj_tmp, %s); _res; })",
                            object_str, index_str);
                    }
                    return arena_sprintf(gen->arena,
                        "({ char *_obj_tmp = %s; char _res = (char)rt_str_charAt(_obj_tmp, %s); rt_free_string(_obj_tmp); _res; })",
                        object_str, index_str);
                }
                return arena_sprintf(gen->arena, "(char)rt_str_charAt(%s, %s)", object_str, index_str);
            }

            // Handle toBytes() - returns byte array (UTF-8 encoding)
            if (strcmp(member_name_str, "toBytes") == 0 && call->arg_count == 0) {
                if (object_is_temp) {
                    if (gen->current_arena_var != NULL) {
                        return arena_sprintf(gen->arena,
                            "({ char *_obj_tmp = %s; unsigned char *_res = rt_string_to_bytes(%s, _obj_tmp); _res; })",
                            object_str, ARENA_VAR(gen));
                    }
                    return arena_sprintf(gen->arena,
                        "({ char *_obj_tmp = %s; unsigned char *_res = rt_string_to_bytes(%s, _obj_tmp); rt_free_string(_obj_tmp); _res; })",
                        object_str, ARENA_VAR(gen));
                }
                return arena_sprintf(gen->arena, "rt_string_to_bytes(%s, %s)", ARENA_VAR(gen), object_str);
            }

            // Handle splitWhitespace() - returns string array
            if (strcmp(member_name_str, "splitWhitespace") == 0 && call->arg_count == 0) {
                if (object_is_temp) {
                    if (gen->current_arena_var != NULL) {
                        return arena_sprintf(gen->arena,
                            "({ char *_obj_tmp = %s; char **_res = rt_str_split_whitespace(%s, _obj_tmp); _res; })",
                            object_str, ARENA_VAR(gen));
                    }
                    return arena_sprintf(gen->arena,
                        "({ char *_obj_tmp = %s; char **_res = rt_str_split_whitespace(%s, _obj_tmp); rt_free_string(_obj_tmp); _res; })",
                        object_str, ARENA_VAR(gen));
                }
                return arena_sprintf(gen->arena, "rt_str_split_whitespace(%s, %s)", ARENA_VAR(gen), object_str);
            }

            // Handle splitLines() - returns string array
            if (strcmp(member_name_str, "splitLines") == 0 && call->arg_count == 0) {
                if (object_is_temp) {
                    if (gen->current_arena_var != NULL) {
                        return arena_sprintf(gen->arena,
                            "({ char *_obj_tmp = %s; char **_res = rt_str_split_lines(%s, _obj_tmp); _res; })",
                            object_str, ARENA_VAR(gen));
                    }
                    return arena_sprintf(gen->arena,
                        "({ char *_obj_tmp = %s; char **_res = rt_str_split_lines(%s, _obj_tmp); rt_free_string(_obj_tmp); _res; })",
                        object_str, ARENA_VAR(gen));
                }
                return arena_sprintf(gen->arena, "rt_str_split_lines(%s, %s)", ARENA_VAR(gen), object_str);
            }

            // Handle isBlank() - returns bool
            if (strcmp(member_name_str, "isBlank") == 0 && call->arg_count == 0) {
                if (object_is_temp) {
                    if (gen->current_arena_var != NULL) {
                        return arena_sprintf(gen->arena,
                            "({ char *_obj_tmp = %s; int _res = rt_str_is_blank(_obj_tmp); _res; })",
                            object_str);
                    }
                    return arena_sprintf(gen->arena,
                        "({ char *_obj_tmp = %s; int _res = rt_str_is_blank(_obj_tmp); rt_free_string(_obj_tmp); _res; })",
                        object_str);
                }
                return arena_sprintf(gen->arena, "rt_str_is_blank(%s)", object_str);
            }

            // Handle append(str) - appends to mutable string, returns new string pointer
            if (strcmp(member_name_str, "append") == 0 && call->arg_count == 1) {
                char *arg_str = code_gen_expression(gen, call->arguments[0]);
                Type *arg_type = call->arguments[0]->expr_type;

                if (arg_type->kind != TYPE_STRING) {
                    fprintf(stderr, "Error: append() argument must be a string\n");
                    exit(1);
                }

                // First ensure the string is mutable, then append.
                // rt_string_ensure_mutable_inline uses fast inlined check when already mutable.
                // rt_string_append returns potentially new pointer, assign back if variable.
                // IMPORTANT: Use the function's main arena (__local_arena__), not the loop arena,
                // because strings need to outlive the loop iteration.
                if (member->object->type == EXPR_VARIABLE) {
                    return arena_sprintf(gen->arena,
                        "(%s = rt_string_append(rt_string_ensure_mutable_inline(__local_arena__, %s), %s))",
                        object_str, object_str, arg_str);
                }
                return arena_sprintf(gen->arena,
                    "rt_string_append(rt_string_ensure_mutable_inline(__local_arena__, %s), %s)",
                    object_str, arg_str);
            }

            #undef STRING_METHOD_RETURNING_STRING
        }
    }

    /* Check if the callee is a closure (function type variable) */
    /* Skip builtins like print and len, and skip named functions */
    bool is_closure_call = false;
    Type *callee_type = call->callee->expr_type;

    if (callee_type && callee_type->kind == TYPE_FUNCTION && call->callee->type == EXPR_VARIABLE)
    {
        /* Native callbacks are called directly as function pointers, not closures */
        if (!callee_type->as.function.is_native)
        {
            char *name = get_var_name(gen->arena, call->callee->as.variable.name);
            /* Skip builtins */
            if (strcmp(name, "print") != 0 && strcmp(name, "len") != 0 &&
                strcmp(name, "readLine") != 0 && strcmp(name, "println") != 0 &&
                strcmp(name, "printErr") != 0 && strcmp(name, "printErrLn") != 0 &&
                strcmp(name, "exit") != 0 && strcmp(name, "assert") != 0)
            {
                /* Check if this is a named function or a closure variable.
                 * Only treat as closure if we find a symbol that ISN'T a function.
                 * If sym is NULL, assume it's a named function - this handles imported
                 * module functions whose symbols were removed after type checking. */
                Symbol *sym = symbol_table_lookup_symbol(gen->symbol_table, call->callee->as.variable.name);
                if (sym != NULL && !sym->is_function)
                {
                    /* This is a closure variable (not a named function). */
                    is_closure_call = true;
                }
            }
        }
    }
    /* Also handle array access where element is a function type (e.g., callbacks[0]()) */
    else if (callee_type && callee_type->kind == TYPE_FUNCTION && call->callee->type == EXPR_ARRAY_ACCESS)
    {
        /* Native callback arrays are not closures */
        if (!callee_type->as.function.is_native)
        {
            is_closure_call = true;
        }
    }

    if (is_closure_call)
    {
        /* Generate closure call: ((ret (*)(void*, params...))closure->fn)(closure, args...) */
        char *closure_str = code_gen_expression(gen, call->callee);

        /* Build function pointer cast */
        const char *ret_c_type = get_c_type(gen->arena, callee_type->as.function.return_type);
        char *param_types_str = arena_strdup(gen->arena, "void *");  /* First param is closure */
        for (int i = 0; i < callee_type->as.function.param_count; i++)
        {
            const char *param_c_type = get_c_type(gen->arena, callee_type->as.function.param_types[i]);
            param_types_str = arena_sprintf(gen->arena, "%s, %s", param_types_str, param_c_type);
        }

        /* Generate arguments */
        char *args_str = closure_str;  /* First arg is the closure itself */
        for (int i = 0; i < call->arg_count; i++)
        {
            char *arg_str = code_gen_expression(gen, call->arguments[i]);
            args_str = arena_sprintf(gen->arena, "%s, %s", args_str, arg_str);
        }

        /* Generate the call: ((<ret> (*)(<params>))closure->fn)(args) */
        return arena_sprintf(gen->arena, "((%s (*)(%s))%s->fn)(%s)",
                             ret_c_type, param_types_str, closure_str, args_str);
    }

    char *callee_str = code_gen_expression(gen, call->callee);

    char **arg_strs = arena_alloc(gen->arena, call->arg_count * sizeof(char *));
    bool *arg_is_temp = arena_alloc(gen->arena, call->arg_count * sizeof(bool));
    bool has_temps = false;
    for (int i = 0; i < call->arg_count; i++)
    {
        arg_strs[i] = code_gen_expression(gen, call->arguments[i]);
        arg_is_temp[i] = (call->arguments[i]->expr_type && call->arguments[i]->expr_type->kind == TYPE_STRING &&
                          expression_produces_temp(call->arguments[i]));
        if (arg_is_temp[i])
            has_temps = true;
    }

    // Special case for builtin 'print': error if wrong arg count, else map to appropriate rt_print_* based on arg type.
    if (call->callee->type == EXPR_VARIABLE)
    {
        char *callee_name = get_var_name(gen->arena, call->callee->as.variable.name);
        if (strcmp(callee_name, "print") == 0)
        {
            if (call->arg_count != 1)
            {
                fprintf(stderr, "Error: print expects exactly one argument\n");
                exit(1);
            }
            Type *arg_type = call->arguments[0]->expr_type;
            const char *print_func = NULL;
            switch (arg_type->kind)
            {
            case TYPE_INT:
            case TYPE_LONG:
                print_func = "rt_print_long";
                break;
            case TYPE_DOUBLE:
                print_func = "rt_print_double";
                break;
            case TYPE_CHAR:
                print_func = "rt_print_char";
                break;
            case TYPE_BOOL:
                print_func = "rt_print_bool";
                break;
            case TYPE_BYTE:
                print_func = "rt_print_byte";
                break;
            case TYPE_STRING:
                print_func = "rt_print_string";
                break;
            case TYPE_ARRAY:
            {
                Type *elem_type = arg_type->as.array.element_type;
                switch (elem_type->kind)
                {
                case TYPE_INT:
                case TYPE_LONG:
                    print_func = "rt_print_array_long";
                    break;
                case TYPE_DOUBLE:
                    print_func = "rt_print_array_double";
                    break;
                case TYPE_CHAR:
                    print_func = "rt_print_array_char";
                    break;
                case TYPE_BOOL:
                    print_func = "rt_print_array_bool";
                    break;
                case TYPE_BYTE:
                    print_func = "rt_print_array_byte";
                    break;
                case TYPE_STRING:
                    print_func = "rt_print_array_string";
                    break;
                default:
                    fprintf(stderr, "Error: unsupported array element type for print\n");
                    exit(1);
                }
                break;
            }
            default:
                fprintf(stderr, "Error: unsupported type for print\n");
                exit(1);
            }
            callee_str = arena_strdup(gen->arena, print_func);
        }
        // Handle len(arr) -> rt_array_length for arrays, strlen for strings
        else if (strcmp(callee_name, "len") == 0 && call->arg_count == 1)
        {
            Type *arg_type = call->arguments[0]->expr_type;
            if (arg_type && arg_type->kind == TYPE_STRING)
            {
                return arena_sprintf(gen->arena, "(long)strlen(%s)", arg_strs[0]);
            }
            return arena_sprintf(gen->arena, "rt_array_length(%s)", arg_strs[0]);
        }
        // readLine() -> rt_read_line(arena)
        else if (strcmp(callee_name, "readLine") == 0 && call->arg_count == 0)
        {
            return arena_sprintf(gen->arena, "rt_read_line(%s)", ARENA_VAR(gen));
        }
        // println(arg) -> rt_println(to_string(arg))
        else if (strcmp(callee_name, "println") == 0 && call->arg_count == 1)
        {
            Type *arg_type = call->arguments[0]->expr_type;
            if (arg_type->kind == TYPE_STRING)
            {
                return arena_sprintf(gen->arena, "rt_println(%s)", arg_strs[0]);
            }
            else
            {
                const char *to_str_func = get_rt_to_string_func_for_type(arg_type);
                return arena_sprintf(gen->arena, "rt_println(%s(%s, %s))",
                                     to_str_func, ARENA_VAR(gen), arg_strs[0]);
            }
        }
        // printErr(arg) -> rt_print_err(to_string(arg))
        else if (strcmp(callee_name, "printErr") == 0 && call->arg_count == 1)
        {
            Type *arg_type = call->arguments[0]->expr_type;
            if (arg_type->kind == TYPE_STRING)
            {
                return arena_sprintf(gen->arena, "rt_print_err(%s)", arg_strs[0]);
            }
            else
            {
                const char *to_str_func = get_rt_to_string_func_for_type(arg_type);
                return arena_sprintf(gen->arena, "rt_print_err(%s(%s, %s))",
                                     to_str_func, ARENA_VAR(gen), arg_strs[0]);
            }
        }
        // printErrLn(arg) -> rt_print_err_ln(to_string(arg))
        else if (strcmp(callee_name, "printErrLn") == 0 && call->arg_count == 1)
        {
            Type *arg_type = call->arguments[0]->expr_type;
            if (arg_type->kind == TYPE_STRING)
            {
                return arena_sprintf(gen->arena, "rt_print_err_ln(%s)", arg_strs[0]);
            }
            else
            {
                const char *to_str_func = get_rt_to_string_func_for_type(arg_type);
                return arena_sprintf(gen->arena, "rt_print_err_ln(%s(%s, %s))",
                                     to_str_func, ARENA_VAR(gen), arg_strs[0]);
            }
        }
        // exit(code: int) -> rt_exit(code)
        else if (strcmp(callee_name, "exit") == 0 && call->arg_count == 1)
        {
            return arena_sprintf(gen->arena, "rt_exit(%s)", arg_strs[0]);
        }
        // assert(condition: bool, message: str) -> rt_assert(condition, message)
        else if (strcmp(callee_name, "assert") == 0 && call->arg_count == 2)
        {
            return arena_sprintf(gen->arena, "rt_assert(%s, %s)", arg_strs[0], arg_strs[1]);
        }
        // Note: Other array operations are method-style only:
        //   arr.push(elem), arr.pop(), arr.reverse(), arr.remove(idx), arr.insert(elem, idx)
    }

    // New arena model: ALL Sindarin functions (with bodies) receive arena as first param.
    // Native functions (no body) use their declared signature - if they need arena,
    // it's explicitly in their parameter list.
    //
    // Check if callee has a body to determine if we should prepend arena.
    bool callee_has_body = false;

    if (call->callee->type == EXPR_VARIABLE)
    {
        Symbol *callee_sym = symbol_table_lookup_symbol(gen->symbol_table, call->callee->as.variable.name);

        // Check if function has a body via the symbol table's function type.
        if (callee_sym != NULL &&
            callee_sym->type != NULL &&
            callee_sym->type->kind == TYPE_FUNCTION)
        {
            callee_has_body = callee_sym->type->as.function.has_body;
        }
    }

    // Collect arg names for the call: use temp var if temp, else original str.
    // arg_base_names stores the temp variable names (for declaration and freeing)
    // arg_names stores the final call arguments (may include boxing/ref transformations)
    char **arg_base_names = arena_alloc(gen->arena, sizeof(char *) * call->arg_count);
    char **arg_names = arena_alloc(gen->arena, sizeof(char *) * call->arg_count);

    // Build args list (comma-separated).
    // If calling a Sindarin function (has body), prepend the current arena as first argument.
    // Native functions (no body) don't get arena prepended - if they need it, it's in their declaration.
    char *args_list;
    if (callee_has_body && gen->current_arena_var != NULL)
    {
        args_list = arena_strdup(gen->arena, gen->current_arena_var);
    }
    else if (callee_has_body)
    {
        // Function has body but no current arena (shouldn't happen in new model)
        args_list = arena_strdup(gen->arena, "NULL");
    }
    else
    {
        args_list = arena_strdup(gen->arena, "");
    }

    /* Get parameter memory qualifiers and types from callee's function type.
     * Only apply boxing for user-defined functions, not built-in functions.
     * Built-in functions like print() use TYPE_ANY for flexibility but don't
     * actually require boxing - the code generator handles them specially. */
    MemoryQualifier *param_quals = NULL;
    Type **param_types = NULL;
    int param_count = 0;
    bool is_user_defined_function = false;
    if (call->callee->expr_type && call->callee->expr_type->kind == TYPE_FUNCTION)
    {
        param_quals = call->callee->expr_type->as.function.param_mem_quals;
        param_types = call->callee->expr_type->as.function.param_types;
        param_count = call->callee->expr_type->as.function.param_count;

        /* Check if this is a user-defined function (has is_function flag set) */
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

        /* Handle boxing when parameter type is 'any' and argument is a concrete type.
         * Only apply for user-defined functions, not built-in functions. */
        if (is_user_defined_function && param_types != NULL && i < param_count &&
            param_types[i] != NULL && param_types[i]->kind == TYPE_ANY &&
            call->arguments[i]->expr_type != NULL &&
            call->arguments[i]->expr_type->kind != TYPE_ANY)
        {
            arg_names[i] = code_gen_box_value(gen, arg_names[i], call->arguments[i]->expr_type);
        }

        /* For 'as ref' primitive and struct parameters, pass address of the argument */
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

        /* When passing a named function to a function-type parameter, wrap it in a closure.
         * Named functions are just function pointers in C, but function parameters expect
         * __Closure__ * which has fn and arena fields. Additionally, closures are called
         * with the closure pointer as the first argument, which named functions don't expect.
         * We need to generate a thin wrapper function that adapts the calling convention. */
        if (param_types != NULL && i < param_count &&
            param_types[i] != NULL && param_types[i]->kind == TYPE_FUNCTION &&
            !param_types[i]->as.function.is_native &&
            call->arguments[i]->type == EXPR_VARIABLE)
        {
            /* Check if this argument is a named function (not a closure variable) */
            Symbol *arg_sym = symbol_table_lookup_symbol(gen->symbol_table, call->arguments[i]->as.variable.name);
            if (arg_sym != NULL && arg_sym->is_function)
            {
                /* Generate a wrapper function that adapts the closure calling convention
                 * to the named function's signature. The wrapper takes (void*, params...)
                 * and forwards to the actual function ignoring the closure pointer. */
                Type *func_type = param_types[i];
                int wrapper_id = gen->wrapper_count++;
                char *wrapper_name = arena_sprintf(gen->arena, "__wrap_%d__", wrapper_id);
                const char *ret_c_type = get_c_type(gen->arena, func_type->as.function.return_type);

                /* Build parameter list: void* first, then actual params */
                char *params_decl = arena_strdup(gen->arena, "void *__closure__");
                char *args_forward = arena_strdup(gen->arena, "");

                /* Check if wrapped function is a Sindarin function (has body) - if so, prepend arena */
                bool wrapped_has_body = (arg_sym->type != NULL &&
                                         arg_sym->type->kind == TYPE_FUNCTION &&
                                         arg_sym->type->as.function.has_body);
                if (wrapped_has_body)
                {
                    args_forward = arena_strdup(gen->arena, "((__Closure__ *)__closure__)->arena");
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
                        wrapper_name, params_decl, arg_strs[i], args_forward);
                }
                else
                {
                    wrapper_func = arena_sprintf(gen->arena,
                        "static %s %s(%s) {\n"
                        "    (void)__closure__;\n"
                        "    return %s(%s);\n"
                        "}\n\n",
                        ret_c_type, wrapper_name, params_decl, arg_strs[i], args_forward);
                }

                /* Add wrapper to lambda definitions (reusing that buffer) */
                gen->lambda_definitions = arena_sprintf(gen->arena, "%s%s",
                                                        gen->lambda_definitions, wrapper_func);

                /* Add forward declaration */
                gen->lambda_forward_decls = arena_sprintf(gen->arena, "%sstatic %s %s(%s);\n",
                                                          gen->lambda_forward_decls, ret_c_type, wrapper_name, params_decl);

                /* Wrap the wrapper function in a closure struct.
                 * If there's an arena, use it; otherwise use malloc. */
                const char *arena_var = ARENA_VAR(gen);
                if (strcmp(arena_var, "NULL") == 0)
                {
                    /* No arena - use malloc */
                    arg_names[i] = arena_sprintf(gen->arena,
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
                    /* Use arena allocation */
                    arg_names[i] = arena_sprintf(gen->arena,
                        "({\n"
                        "    __Closure__ *__cl__ = rt_arena_alloc(%s, sizeof(__Closure__));\n"
                        "    __cl__->fn = (void *)%s;\n"
                        "    __cl__->arena = %s;\n"
                        "    __cl__;\n"
                        "})",
                        arena_var, wrapper_name, arena_var);
                }
            }
        }

        bool need_comma = (i > 0) || callee_has_body;
        char *new_args = arena_sprintf(gen->arena, "%s%s%s", args_list, need_comma ? ", " : "", arg_names[i]);
        args_list = new_args;
    }

    // Determine if the call returns void (affects statement expression).
    bool returns_void = (expr->expr_type && expr->expr_type->kind == TYPE_VOID);

    // Get function name for interceptor (only needed for user-defined functions)
    // Skip interception for functions with unsupported parameter types
    char *func_name_for_intercept = NULL;
    bool skip_interception = false;
    if (is_user_defined_function && call->callee->type == EXPR_VARIABLE)
    {
        func_name_for_intercept = get_var_name(gen->arena, call->callee->as.variable.name);

        // Skip pointer and struct parameters - not yet supported for boxing
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

        // Check if it's a native function - skip interception for native functions
        if (!skip_interception && call->callee->expr_type != NULL &&
            call->callee->expr_type->kind == TYPE_FUNCTION &&
            call->callee->expr_type->as.function.is_native)
        {
            skip_interception = true;
        }

        // Skip functions that return pointer or struct types (not yet supported for boxing)
        if (!skip_interception && expr->expr_type != NULL &&
            (expr->expr_type->kind == TYPE_POINTER || expr->expr_type->kind == TYPE_STRUCT))
        {
            skip_interception = true;
        }
    }

    // If no temps, simple call (no statement expression needed).
    // Note: Expression returns without semicolon - statement handler adds it
    if (!has_temps)
    {
        // For user-defined functions, wrap with interception logic
        // Skip interception for functions with unsupported parameters
        if (is_user_defined_function && func_name_for_intercept != NULL && !skip_interception)
        {
            return code_gen_intercepted_call(gen, func_name_for_intercept,
                                             callee_str, args_list, call, arg_strs,
                                             param_types, param_quals, param_count,
                                             expr->expr_type, callee_has_body);
        }
        return arena_sprintf(gen->arena, "%s(%s)", callee_str, args_list);
    }

    // Temps present: generate multi-line statement expression for readability
    char *result = arena_strdup(gen->arena, "({\n");

    // Declare and initialize temp string arguments
    for (int i = 0; i < call->arg_count; i++)
    {
        if (arg_is_temp[i])
        {
            result = arena_sprintf(gen->arena, "%s        char *%s = %s;\n", result, arg_base_names[i], arg_strs[i]);
        }
    }

    // Make the actual call
    const char *ret_c = get_c_type(gen->arena, expr->expr_type);
    if (returns_void)
    {
        result = arena_sprintf(gen->arena, "%s        %s(%s);\n", result, callee_str, args_list);
    }
    else
    {
        result = arena_sprintf(gen->arena, "%s        %s _call_result = %s(%s);\n", result, ret_c, callee_str, args_list);
    }

    // Free temps (only strings) - skip if in arena context
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

    // End statement expression.
    if (returns_void)
    {
        result = arena_sprintf(gen->arena, "%s    })", result);
    }
    else
    {
        result = arena_sprintf(gen->arena, "%s        _call_result;\n    })", result);
    }

    return result;
}
