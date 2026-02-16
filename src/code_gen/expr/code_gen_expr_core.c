#include "code_gen/expr/code_gen_expr_core.h"
#include "code_gen/expr/code_gen_expr.h"
#include "code_gen/expr/lambda/code_gen_expr_lambda.h"
#include "code_gen/expr/code_gen_expr_array.h"
#include "code_gen/util/code_gen_util.h"
#include "debug.h"
#include "symbol_table.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

char *code_gen_literal_expression(CodeGen *gen, LiteralExpr *expr)
{
    DEBUG_VERBOSE("Entering code_gen_literal_expression");
    Type *type = expr->type;
    switch (type->kind)
    {
    case TYPE_INT:
        return arena_sprintf(gen->arena, "%lldLL", (long long)expr->value.int_value);
    case TYPE_LONG:
        return arena_sprintf(gen->arena, "%lldLL", (long long)expr->value.int_value);
    case TYPE_DOUBLE:
    {
        char *str = arena_sprintf(gen->arena, "%.17g", expr->value.double_value);
        if (strchr(str, '.') == NULL && strchr(str, 'e') == NULL && strchr(str, 'E') == NULL)
        {
            str = arena_sprintf(gen->arena, "%s.0", str);
        }
        return str;
    }
    case TYPE_CHAR:
        return escape_char_literal(gen->arena, expr->value.char_value);
    case TYPE_STRING:
    {
        char *raw = escape_c_string(gen->arena, expr->value.string_value);
        /* In handle mode, wrap string literals to produce an RtHandle */
        if (gen->expr_as_handle && gen->current_arena_var != NULL)
        {
            return arena_sprintf(gen->arena, "rt_arena_v2_strdup(%s, %s)",
                                 ARENA_VAR(gen), raw);
        }
        return raw;
    }
    case TYPE_BOOL:
        return arena_sprintf(gen->arena, "%ldL", expr->value.bool_value ? 1L : 0L);
    case TYPE_BYTE:
        return arena_sprintf(gen->arena, "(uint8_t)%lldLL", (long long)expr->value.int_value);
    case TYPE_FLOAT:
    {
        char *str = arena_sprintf(gen->arena, "%.9gf", expr->value.double_value);
        if (strchr(str, '.') == NULL && strchr(str, 'e') == NULL && strchr(str, 'E') == NULL)
        {
            /* Remove the 'f' suffix and add ".0f" */
            str[strlen(str) - 1] = '\0';
            str = arena_sprintf(gen->arena, "%s.0f", str);
        }
        return str;
    }
    case TYPE_UINT:
        return arena_sprintf(gen->arena, "%lluULL", (unsigned long long)expr->value.int_value);
    case TYPE_UINT32:
        return arena_sprintf(gen->arena, "%uU", (unsigned int)expr->value.int_value);
    case TYPE_INT32:
        return arena_sprintf(gen->arena, "%d", (int)expr->value.int_value);
    case TYPE_NIL:
        /* In handle mode (arena context), nil for strings/arrays is NULL */
        if (gen->expr_as_handle && gen->current_arena_var != NULL)
        {
            return arena_strdup(gen->arena, "NULL");
        }
        return arena_strdup(gen->arena, "NULL");
    default:
        exit(1);
    }
    return NULL;
}

char *code_gen_variable_expression(CodeGen *gen, VariableExpr *expr)
{
    DEBUG_VERBOSE("Entering code_gen_variable_expression");
    char *var_name = get_var_name(gen->arena, expr->name);

    /* Handle 'arena' built-in identifier - resolve to current arena variable */
    if (expr->name.length == 5 && strncmp(expr->name.start, "arena", 5) == 0)
    {
        if (gen->current_arena_var != NULL)
        {
            return arena_strdup(gen->arena, gen->current_arena_var);
        }
        /* Fallback to rt_current_arena() if no arena variable is available */
        return arena_strdup(gen->arena, "rt_current_arena()");
    }

    // Check if we're inside a lambda and this is a lambda parameter.
    // Lambda parameters shadow outer variables, so don't look up in symbol table.
    if (gen->enclosing_lambda_count > 0)
    {
        LambdaExpr *innermost = gen->enclosing_lambdas[gen->enclosing_lambda_count - 1];
        if (is_lambda_param(innermost, var_name))
        {
            char *mangled_param = sn_mangle_name(gen->arena, var_name);

            /* Lambda params of handle type need pinning when caller expects raw pointer */
            if (!gen->expr_as_handle && gen->current_arena_var != NULL)
            {
                /* Find the param type from the lambda definition */
                for (int pi = 0; pi < innermost->param_count; pi++)
                {
                    char pname[256];
                    int plen = innermost->params[pi].name.length < 255 ? innermost->params[pi].name.length : 255;
                    strncpy(pname, innermost->params[pi].name.start, plen);
                    pname[plen] = '\0';
                    if (strcmp(pname, var_name) == 0 && innermost->params[pi].type != NULL)
                    {
                        Type *ptype = innermost->params[pi].type;
                        if (ptype->kind == TYPE_STRING)
                        {
                            return arena_sprintf(gen->arena, "((char *)(%s)->ptr)", mangled_param);
                        }
                        else if (ptype->kind == TYPE_ARRAY)
                        {
                            const char *elem_c = get_c_array_elem_type(gen->arena, ptype->as.array.element_type);
                            return arena_sprintf(gen->arena, "((%s *)rt_array_data_v2(%s))",
                                                 elem_c, mangled_param);
                        }
                        break;
                    }
                }
            }
            return mangled_param;
        }
    }

    // Check if variable is 'as ref' - if so, dereference it
    Symbol *symbol = symbol_table_lookup_symbol(gen->symbol_table, expr->name);
    if (symbol && symbol->mem_qual == MEM_AS_REF)
    {
        char *deref = arena_sprintf(gen->arena, "(*%s)", sn_mangle_name(gen->arena, var_name));
        /* as ref handle types need pinning when caller expects raw pointer */
        if (!gen->expr_as_handle && gen->current_arena_var != NULL &&
            symbol->type != NULL && is_handle_type(symbol->type))
        {
            if (symbol->type->kind == TYPE_STRING)
                return arena_sprintf(gen->arena, "((char *)(%s)->ptr)", deref);
            else if (symbol->type->kind == TYPE_ARRAY)
            {
                const char *elem_c = get_c_array_elem_type(gen->arena, symbol->type->as.array.element_type);
                return arena_sprintf(gen->arena, "((%s *)rt_array_data_v2(%s))", elem_c, deref);
            }
        }
        return deref;
    }

    /* For native functions/variables, use the C name (c_alias or raw name) */
    if (symbol && symbol->is_native)
    {
        if (symbol->c_alias != NULL)
        {
            return arena_strdup(gen->arena, symbol->c_alias);
        }
        return var_name;  /* Native without alias: Sindarin name IS the C name */
    }

    /* If we're generating code for an imported namespace, prefix global symbols
     * with the appropriate namespace to avoid collisions between different modules.
     * Parameters and local variables are NOT prefixed.
     *
     * For STATIC global variables: use canonical_module_name so all aliases of the same
     * module access the same static variable storage.
     *
     * For NON-STATIC global variables and functions: use namespace_prefix so each alias
     * has its own instance and functions can access the correct per-alias state. */
    char *mangled;
    const char *prefix_to_use = NULL;

    if (symbol != NULL && (symbol->kind == SYMBOL_GLOBAL || symbol->is_function))
    {
        /* Determine which prefix to use based on whether this is a static variable */
        if (symbol->kind == SYMBOL_GLOBAL && symbol->is_static &&
            gen->current_canonical_module != NULL)
        {
            /* Static variable: use canonical module name for sharing across aliases */
            prefix_to_use = gen->current_canonical_module;
        }
        else if (gen->current_namespace_prefix != NULL)
        {
            /* Non-static variable or function: use namespace prefix */
            prefix_to_use = gen->current_namespace_prefix;
        }
    }

    if (prefix_to_use != NULL)
    {
        char *prefixed_name = arena_sprintf(gen->arena, "%s__%s", prefix_to_use, var_name);
        mangled = sn_mangle_name(gen->arena, prefixed_name);
    }
    else
    {
        mangled = sn_mangle_name(gen->arena, var_name);
    }

    /* Global handle-type variables passed as function arguments (expr_as_handle=true)
     * must be cloned to the local arena. Without cloning, the function would try to
     * pin the handle from its caller arena, but the handle exists in __main_arena__.
     * Handle indices are arena-local, so the same index could refer to different data. */
    if (symbol && gen->expr_as_handle &&
        gen->current_arena_var != NULL &&
        symbol->kind == SYMBOL_GLOBAL &&
        symbol->type != NULL &&
        is_handle_type(symbol->type))
    {
        /* Clone the global handle to the current local arena.
         * V2 handles carry their own arena reference, so no source arena needed. */
        return arena_sprintf(gen->arena, "rt_arena_v2_clone(%s, %s)",
                             ARENA_VAR(gen), mangled);
    }

    /* Handle-type variables (string/array/params) need pinning when used in
     * contexts expecting raw pointers (expr_as_handle = false).
     * IMPORTANT: pins must use the arena that owns the handle, not necessarily
     * the current arena (which may be a loop child arena). */
    if (symbol && !gen->expr_as_handle &&
        gen->current_arena_var != NULL &&
        symbol->type != NULL &&
        is_handle_type(symbol->type))
    {
        /* Pin handles. V2 handles don't need the arena parameter.
         * For strings, pin the handle then access ->ptr for the string data.
         * For arrays, rt_array_data_v2 returns the element data (not metadata). */
        if (symbol->type->kind == TYPE_STRING)
        {
            /* Transaction-based raw pointer extraction for V1 callers */
            return arena_sprintf(gen->arena,
                "((char *)(%s)->ptr)",
                mangled);
        }
        else if (symbol->type->kind == TYPE_ARRAY)
        {
            const char *elem_c = get_c_array_elem_type(gen->arena, symbol->type->as.array.element_type);
            return arena_sprintf(gen->arena, "((%s *)rt_array_data_v2(%s))",
                                 elem_c, mangled);
        }
    }

    return mangled;
}

char *code_gen_assign_expression(CodeGen *gen, AssignExpr *expr)
{
    DEBUG_VERBOSE("Entering code_gen_assign_expression");
    char *base_var_name = get_var_name(gen->arena, expr->name);
    Symbol *symbol = symbol_table_lookup_symbol(gen->symbol_table, expr->name);
    if (symbol == NULL)
    {
        exit(1);
    }

    /* Apply namespace prefix for global variables and functions in imported modules,
     * similar to code_gen_variable_expression.
     * Static variables use canonical module name for sharing across aliases.
     * Non-static variables use namespace prefix for per-alias instance. */
    char *var_name;
    const char *prefix_to_use = NULL;
    if (symbol->kind == SYMBOL_GLOBAL && symbol->is_static &&
        gen->current_canonical_module != NULL)
    {
        /* Static variable: use canonical module name for sharing across aliases */
        prefix_to_use = gen->current_canonical_module;
    }
    else if (gen->current_namespace_prefix != NULL)
    {
        if (symbol->kind == SYMBOL_GLOBAL || symbol->is_function)
        {
            /* Non-static variable or function: use namespace prefix */
            prefix_to_use = gen->current_namespace_prefix;
        }
    }
    if (prefix_to_use != NULL)
    {
        char *prefixed_name = arena_sprintf(gen->arena, "%s__%s",
                                            prefix_to_use, base_var_name);
        var_name = sn_mangle_name(gen->arena, prefixed_name);
    }
    else
    {
        var_name = sn_mangle_name(gen->arena, base_var_name);
    }

    Type *type = symbol->type;

    /* Special handling for thread spawn assignments.
     * When assigning a thread spawn to a variable, we assign to the __var_pending__
     * variable instead of the actual variable. This enables conditional thread spawn:
     *   var h: Result = default_value
     *   if condition =>
     *       h = &compute()  // Assigns to __h_pending__
     *   h!  // Syncs if __h_pending__ is not NULL
     */
    if (expr->value != NULL && expr->value->type == EXPR_THREAD_SPAWN &&
        expr->value->expr_type != NULL && expr->value->expr_type->kind != TYPE_VOID)
    {
        Type *result_type = expr->value->expr_type;
        bool is_primitive_type = (result_type->kind == TYPE_INT ||
                                  result_type->kind == TYPE_LONG ||
                                  result_type->kind == TYPE_DOUBLE ||
                                  result_type->kind == TYPE_BOOL ||
                                  result_type->kind == TYPE_BYTE ||
                                  result_type->kind == TYPE_CHAR);
        bool is_spawn_handle_result = gen->current_arena_var != NULL &&
                                      (result_type->kind == TYPE_STRING ||
                                       result_type->kind == TYPE_ARRAY);
        bool is_struct_result = (result_type->kind == TYPE_STRUCT);

        if (is_primitive_type || is_spawn_handle_result || is_struct_result)
        {
            /* Generate the thread spawn expression */
            char *spawn_str = code_gen_expression(gen, expr->value);

            /* Assign to the pending variable instead of the actual variable */
            char *pending_var = arena_sprintf(gen->arena, "__%s_pending__", base_var_name);

            return arena_sprintf(gen->arena, "(%s = %s)", pending_var, spawn_str);
        }
    }

    /* When assigning to a handle type (array/string) or boxing an array
     * into 'any', evaluate in handle mode so we produce RtHandle expressions. */
    bool saved_as_handle = gen->expr_as_handle;
    bool string_as_handle = false;
    if (gen->current_arena_var != NULL)
    {
        /* For string types, enable handle mode so string operations return RtHandle. */
        if (type->kind == TYPE_STRING)
        {
            gen->expr_as_handle = true;
            string_as_handle = true;
        }
        else if (type->kind == TYPE_ARRAY &&
            type->as.array.element_type != NULL &&
            expr->value->expr_type != NULL &&
            expr->value->expr_type->kind == TYPE_ARRAY &&
            expr->value->expr_type->as.array.element_type != NULL)
        {
            Type *decl_elem = type->as.array.element_type;
            Type *src_elem = expr->value->expr_type->as.array.element_type;
            /* Enable handle mode for array expressions so they produce
             * RtHandle values via rt_array_create_*_v2 or method _v2 functions.
             * EXCEPTION: Don't enable handle mode when assigning to any[] because
             * the conversion functions (rt_array_to_any_*) need raw pointers. */
            if (decl_elem->kind != TYPE_ANY)
            {
                gen->expr_as_handle = true;
            }
            /* Enable handle mode when a 2D/3D any conversion will be applied,
             * or when assigning to any[] (1D conversion needs pin). */
            else if ((decl_elem->kind == TYPE_ANY && src_elem->kind != TYPE_ANY) ||
                (decl_elem->kind == TYPE_ARRAY &&
                 decl_elem->as.array.element_type != NULL &&
                 decl_elem->as.array.element_type->kind == TYPE_ANY &&
                 src_elem->kind == TYPE_ARRAY &&
                 src_elem->as.array.element_type != NULL &&
                 src_elem->as.array.element_type->kind != TYPE_ANY) ||
                (decl_elem->kind == TYPE_ARRAY &&
                 decl_elem->as.array.element_type != NULL &&
                 decl_elem->as.array.element_type->kind == TYPE_ARRAY &&
                 decl_elem->as.array.element_type->as.array.element_type != NULL &&
                 decl_elem->as.array.element_type->as.array.element_type->kind == TYPE_ANY))
            {
                gen->expr_as_handle = true;
            }
        }
        else if (type->kind == TYPE_ANY &&
                 expr->value->expr_type != NULL && expr->value->expr_type->kind == TYPE_ARRAY)
        {
            gen->expr_as_handle = true;
        }
    }
    char *value_str = code_gen_expression(gen, expr->value);
    gen->expr_as_handle = saved_as_handle;

    // Handle boxing when assigning to 'any' type
    if (type->kind == TYPE_ANY && expr->value->expr_type != NULL &&
        expr->value->expr_type->kind != TYPE_ANY)
    {
        value_str = code_gen_box_value(gen, value_str, expr->value->expr_type);
    }

    // Handle conversion when assigning typed array to any[], any[][], or any[][][]
    // Track if the conversion already produced a handle (2D/3D cases, or array literals).
    // Array literals (EXPR_ARRAY) already produce a fresh handle via rt_array_create_*_h,
    // so they don't need cloning - just direct assignment.
    // Thread sync expressions (EXPR_THREAD_SYNC) also produce fresh handles - the sync
    // returns an RtHandle directly from rt_thread_sync_with_result, no cloning needed.
    bool value_is_new_handle = (expr->value->type == EXPR_ARRAY) ||
                               (expr->value->type == EXPR_THREAD_SYNC &&
                                expr->value->expr_type != NULL &&
                                expr->value->expr_type->kind == TYPE_ARRAY);
    if (type->kind == TYPE_ARRAY &&
        type->as.array.element_type != NULL &&
        expr->value->expr_type != NULL &&
        expr->value->expr_type->kind == TYPE_ARRAY &&
        expr->value->expr_type->as.array.element_type != NULL)
    {
        Type *decl_elem = type->as.array.element_type;
        Type *src_elem = expr->value->expr_type->as.array.element_type;

        // Check for 3D array: any[][][] = T[][][]
        if (decl_elem->kind == TYPE_ARRAY &&
            decl_elem->as.array.element_type != NULL &&
            decl_elem->as.array.element_type->kind == TYPE_ARRAY &&
            decl_elem->as.array.element_type->as.array.element_type != NULL &&
            decl_elem->as.array.element_type->as.array.element_type->kind == TYPE_ANY &&
            src_elem->kind == TYPE_ARRAY &&
            src_elem->as.array.element_type != NULL &&
            src_elem->as.array.element_type->kind == TYPE_ARRAY &&
            src_elem->as.array.element_type->as.array.element_type != NULL &&
            src_elem->as.array.element_type->as.array.element_type->kind != TYPE_ANY)
        {
            Type *innermost_src = src_elem->as.array.element_type->as.array.element_type;
            const char *conv_func = NULL;
            switch (innermost_src->kind)
            {
            case TYPE_INT:
            case TYPE_INT32:
            case TYPE_UINT:
            case TYPE_UINT32:
            case TYPE_LONG:
                conv_func = "rt_array3_to_any_long";
                break;
            case TYPE_DOUBLE:
            case TYPE_FLOAT:
                conv_func = "rt_array3_to_any_double";
                break;
            case TYPE_CHAR:
                conv_func = "rt_array3_to_any_char";
                break;
            case TYPE_BOOL:
                conv_func = "rt_array3_to_any_bool";
                break;
            case TYPE_BYTE:
                conv_func = "rt_array3_to_any_byte";
                break;
            case TYPE_STRING:
                conv_func = "rt_array3_to_any_string";
                break;
            default:
                break;
            }
            if (conv_func != NULL)
            {
                /* V2 to_any functions take just the handle */
                value_str = arena_sprintf(gen->arena, "%s_v2(%s)", conv_func, value_str);
                value_is_new_handle = true;
            }
        }
        // Check for 2D array: any[][] = T[][]
        else if (decl_elem->kind == TYPE_ARRAY &&
            decl_elem->as.array.element_type != NULL &&
            decl_elem->as.array.element_type->kind == TYPE_ANY &&
            src_elem->kind == TYPE_ARRAY &&
            src_elem->as.array.element_type != NULL &&
            src_elem->as.array.element_type->kind != TYPE_ANY)
        {
            Type *inner_src = src_elem->as.array.element_type;
            const char *conv_func = NULL;
            switch (inner_src->kind)
            {
            case TYPE_INT:
            case TYPE_INT32:
            case TYPE_UINT:
            case TYPE_UINT32:
            case TYPE_LONG:
                conv_func = "rt_array2_to_any_long";
                break;
            case TYPE_DOUBLE:
            case TYPE_FLOAT:
                conv_func = "rt_array2_to_any_double";
                break;
            case TYPE_CHAR:
                conv_func = "rt_array2_to_any_char";
                break;
            case TYPE_BOOL:
                conv_func = "rt_array2_to_any_bool";
                break;
            case TYPE_BYTE:
                conv_func = "rt_array2_to_any_byte";
                break;
            case TYPE_STRING:
                conv_func = "rt_array2_to_any_string";
                break;
            default:
                break;
            }
            if (conv_func != NULL)
            {
                /* V2 to_any functions take just the handle */
                value_str = arena_sprintf(gen->arena, "%s_v2(%s)", conv_func, value_str);
                value_is_new_handle = true;
            }
        }
        // Check for 1D array: any[] = T[]
        else if (decl_elem->kind == TYPE_ANY && src_elem->kind != TYPE_ANY)
        {
            const char *conv_func = NULL;
            switch (src_elem->kind)
            {
            case TYPE_INT:
            case TYPE_INT32:
            case TYPE_UINT:
            case TYPE_UINT32:
            case TYPE_LONG:
                conv_func = "rt_array_to_any_long";
                break;
            case TYPE_DOUBLE:
            case TYPE_FLOAT:
                conv_func = "rt_array_to_any_double";
                break;
            case TYPE_CHAR:
                conv_func = "rt_array_to_any_char";
                break;
            case TYPE_BOOL:
                conv_func = "rt_array_to_any_bool";
                break;
            case TYPE_BYTE:
                conv_func = "rt_array_to_any_byte";
                break;
            case TYPE_STRING:
                conv_func = "rt_array_to_any_string";
                break;
            default:
                break;
            }
            if (conv_func != NULL)
            {
                /* V2 to_any functions take just the handle */
                value_str = arena_sprintf(gen->arena, "%s_v2(%s)", conv_func, value_str);
                value_is_new_handle = true;
            }
        }
    }

    // Handle 'as ref' - dereference pointer for assignment
    if (symbol->mem_qual == MEM_AS_REF)
    {
        return arena_sprintf(gen->arena, "(*%s = %s)", var_name, value_str);
    }

    // Handle escaping struct assignments - copy to outer arena
    // When a struct value from an inner scope is assigned to an outer scope variable,
    // we need to ensure the struct data is allocated in the appropriate arena.
    if (type->kind == TYPE_STRUCT &&
        expr->value != NULL &&
        ast_expr_escapes_scope(expr->value) &&
        gen->current_arena_var != NULL)
    {
        // Get the struct type name for sizeof
        const char *struct_name = sn_mangle_name(gen->arena, type->as.struct_type.name);
        if (struct_name != NULL)
        {
            // Generate: ({ RtHandleV2 *__esc_h__ = rt_arena_v2_alloc(arena, sizeof(StructType));
            //             StructType *__esc_tmp__ = (StructType *)__esc_h__->ptr;
            //             StructType __src_tmp__ = value;
            //             memcpy(__esc_tmp__, &__src_tmp__, sizeof(StructType));
            //             var = *__esc_tmp__; var; })
            // This allocates in the outer arena first, then copies using memcpy
            return arena_sprintf(gen->arena,
                "({ RtHandleV2 *__esc_h__ = rt_arena_v2_alloc(%s, sizeof(%s)); "
                "rt_handle_begin_transaction(__esc_h__); "
                "%s *__esc_tmp__ = (%s *)__esc_h__->ptr; "
                "%s __esc_src__ = %s; "
                "memcpy(__esc_tmp__, &__esc_src__, sizeof(%s)); "
                "%s = *__esc_tmp__; "
                "rt_handle_end_transaction(__esc_h__); "
                "%s; })",
                ARENA_VAR(gen), struct_name,
                struct_name, struct_name,
                struct_name, value_str,
                struct_name,
                var_name, var_name);
        }
    }

    /* Check if target is a global variable (needs promotion to main arena) */
    bool is_global = (symbol->kind == SYMBOL_GLOBAL || symbol->declaration_scope_depth <= 1);
    bool in_arena_context = (gen->current_arena_var != NULL);

    /* With function-level arenas, handles are always local to the function or come
     * from a parent arena. No loop escape handling needed. */

    if (type->kind == TYPE_STRING)
    {
        if (in_arena_context)
        {
            if (string_as_handle)
            {
                /* Value expression was evaluated in handle mode - already returns RtHandleV2*.
                 * For globals, promote the handle to main arena so it survives function return.
                 * V2 handles carry their own arena reference, so no source arena needed. */
                if (is_global)
                {
                    return arena_sprintf(gen->arena, "({ rt_arena_v2_free(%s); %s = rt_arena_v2_promote(__main_arena__, %s); })",
                                         var_name, var_name, value_str);
                }
                /* For locals, just do a direct assignment. */
                return arena_sprintf(gen->arena, "(%s = %s)", var_name, value_str);
            }
            /* For handle-based strings: create new handle and assign (old will be GC'd).
             * The value_str is a raw pointer (pinned by expression generator).
             * For globals, use main arena and free old handle. Otherwise use function arena. */
            const char *target_arena = is_global ? "__main_arena__" : ARENA_VAR(gen);
            if (is_global)
            {
                return arena_sprintf(gen->arena, "({ rt_arena_v2_free(%s); %s = rt_arena_v2_strdup(%s, %s); })",
                                     var_name, var_name, target_arena, value_str);
            }
            return arena_sprintf(gen->arena, "(%s = rt_arena_v2_strdup(%s, %s))",
                                 var_name, target_arena, value_str);
        }
        return arena_sprintf(gen->arena, "({ char *_val = %s; if (%s) rt_free_string(%s); %s = _val; _val; })",
                             value_str, var_name, var_name, var_name);
    }
    else if (type->kind == TYPE_ARRAY && in_arena_context)
    {
        /* Reset pending elements companion when reassigning an array with pending elements */
        char *pending_reset = arena_strdup(gen->arena, "");
        if (symbol->has_pending_elements)
        {
            char *pending_elems_var = arena_sprintf(gen->arena, "__%s_pending_elems__", base_var_name);
            pending_reset = arena_sprintf(gen->arena, " %s = NULL;", pending_elems_var);
        }

        if (value_is_new_handle)
        {
            /* Array literal or 2D/3D conversion already produced a new handle.
             * For globals, promote to main arena so it survives function return.
             * For locals, just assign directly.
             * V2 handles carry their own arena reference, so no source arena needed. */
            if (is_global)
            {
                return arena_sprintf(gen->arena, "({%s rt_arena_v2_free(%s); %s = rt_arena_v2_promote(__main_arena__, %s); %s; })",
                                     pending_reset, var_name, var_name, value_str, var_name);
            }
            if (symbol->has_pending_elements)
            {
                return arena_sprintf(gen->arena, "({%s %s = %s; %s; })",
                                     pending_reset, var_name, value_str, var_name);
            }
            return arena_sprintf(gen->arena, "(%s = %s)", var_name, value_str);
        }
        /* For handle-based arrays: clone to target arena with old handle.
         * Use rt_arena_v2_clone for cross-arena cloning (e.g., to __main_arena__ for globals). */
        const char *target_arena = is_global ? "__main_arena__" : ARENA_VAR(gen);
        if (is_global)
        {
            if (symbol->has_pending_elements)
            {
                return arena_sprintf(gen->arena, "({%s rt_arena_v2_free(%s); %s = rt_arena_v2_clone(%s, %s); %s; })",
                                     pending_reset, var_name, var_name, target_arena, value_str, var_name);
            }
            return arena_sprintf(gen->arena, "({ rt_arena_v2_free(%s); %s = rt_arena_v2_clone(%s, %s); })",
                                 var_name, var_name, target_arena, value_str);
        }
        if (symbol->has_pending_elements)
        {
            return arena_sprintf(gen->arena, "({%s %s = rt_arena_v2_clone(%s, %s); %s; })",
                                 pending_reset, var_name, target_arena, value_str, var_name);
        }
        return arena_sprintf(gen->arena, "(%s = rt_arena_v2_clone(%s, %s))",
                             var_name, target_arena, value_str);
    }
    else if (type->kind == TYPE_STRUCT && in_arena_context && is_global)
    {
        // Struct is value-copied, but string/array fields are handles.
        // Deep-promote those fields to main's arena using rt_arena_v2_promote.
        // V2 handles carry their own arena reference, so no source arena needed.
        int field_count = type->as.struct_type.field_count;
        bool needs_deep_promote = false;
        for (int i = 0; i < field_count; i++)
        {
            StructField *field = &type->as.struct_type.fields[i];
            if (field->type && (field->type->kind == TYPE_STRING || field->type->kind == TYPE_ARRAY))
            {
                needs_deep_promote = true;
                break;
            }
        }
        if (needs_deep_promote)
        {
            // Build: ({ free old fields; var = value; promote new fields...; var; })
            char *result = arena_sprintf(gen->arena, "({ ");
            // Free old handle fields first
            for (int i = 0; i < field_count; i++)
            {
                StructField *field = &type->as.struct_type.fields[i];
                if (field->type == NULL) continue;
                const char *c_field_name = field->c_alias != NULL ? field->c_alias : sn_mangle_name(gen->arena, field->name);
                if (field->type->kind == TYPE_STRING || field->type->kind == TYPE_ARRAY)
                {
                    result = arena_sprintf(gen->arena, "%srt_arena_v2_free(%s.%s); ",
                                           result, var_name, c_field_name);
                }
            }
            // Assign new value
            result = arena_sprintf(gen->arena, "%s%s = %s; ", result, var_name, value_str);
            // Promote new handle fields to main arena
            for (int i = 0; i < field_count; i++)
            {
                StructField *field = &type->as.struct_type.fields[i];
                if (field->type == NULL) continue;
                const char *c_field_name = field->c_alias != NULL ? field->c_alias : sn_mangle_name(gen->arena, field->name);
                if (field->type->kind == TYPE_STRING || field->type->kind == TYPE_ARRAY)
                {
                    result = arena_sprintf(gen->arena, "%sif (%s.%s) %s.%s = rt_arena_v2_promote(__main_arena__, %s.%s); ",
                                           result, var_name, c_field_name, var_name, c_field_name, var_name, c_field_name);
                }
            }
            result = arena_sprintf(gen->arena, "%s%s; })", result, var_name);
            return result;
        }
        return arena_sprintf(gen->arena, "(%s = %s)", var_name, value_str);
    }
    else
    {
        return arena_sprintf(gen->arena, "(%s = %s)", var_name, value_str);
    }
}

char *code_gen_index_assign_expression(CodeGen *gen, IndexAssignExpr *expr)
{
    DEBUG_VERBOSE("Entering code_gen_index_assign_expression");

    /* Get the element type for typed accessor selection */
    Type *elem_type = NULL;
    if (expr->array->expr_type && expr->array->expr_type->kind == TYPE_ARRAY)
    {
        elem_type = expr->array->expr_type->as.array.element_type;
    }
    if (elem_type) elem_type = resolve_struct_type(gen, elem_type);

    const char *suffix = get_array_accessor_suffix(elem_type);

    /* In V2 arena mode with typed accessors, evaluate array as handle */
    if (gen->current_arena_var != NULL && suffix != NULL)
    {
        bool saved_as_handle = gen->expr_as_handle;
        gen->expr_as_handle = true;
        char *handle_str = code_gen_expression(gen, expr->array);
        gen->expr_as_handle = saved_as_handle;

        char *index_str = code_gen_expression(gen, expr->index);

        /* For handle-type values (string/array), evaluate value in handle mode */
        if (elem_type != NULL && is_handle_type(elem_type))
        {
            gen->expr_as_handle = true;
        }
        char *value_str = code_gen_expression(gen, expr->value);
        gen->expr_as_handle = saved_as_handle;

        /* Compute adjusted index for negative index support */
        char *adj_index;
        if (is_provably_non_negative(gen, expr->index))
        {
            adj_index = index_str;
        }
        else if (expr->index->type == EXPR_LITERAL &&
            expr->index->as.literal.type != NULL &&
            (expr->index->as.literal.type->kind == TYPE_INT ||
             expr->index->as.literal.type->kind == TYPE_LONG))
        {
            adj_index = arena_sprintf(gen->arena, "(rt_array_length_v2(%s) + %s)",
                                      handle_str, index_str);
        }
        else
        {
            adj_index = arena_sprintf(gen->arena, "((%s) < 0 ? rt_array_length_v2(%s) + (%s) : (%s))",
                                      index_str, handle_str, index_str, index_str);
        }

        return arena_sprintf(gen->arena, "rt_array_set_%s_v2(%s, %s, %s)",
                             suffix, handle_str, adj_index, value_str);
    }

    /* Fallback: non-arena mode or struct elements - use data pointer approach */
    char *array_str = code_gen_expression(gen, expr->array);
    char *index_str = code_gen_expression(gen, expr->index);
    char *value_str = code_gen_expression(gen, expr->value);

    if (is_provably_non_negative(gen, expr->index))
    {
        return arena_sprintf(gen->arena, "(%s[%s] = %s)",
                             array_str, index_str, value_str);
    }

    if (expr->index->type == EXPR_LITERAL &&
        expr->index->as.literal.type != NULL &&
        (expr->index->as.literal.type->kind == TYPE_INT ||
         expr->index->as.literal.type->kind == TYPE_LONG))
    {
        return arena_sprintf(gen->arena, "(%s[rt_v2_data_array_length(%s) + %s] = %s)",
                             array_str, array_str, index_str, value_str);
    }

    return arena_sprintf(gen->arena, "(%s[(%s) < 0 ? rt_v2_data_array_length(%s) + (%s) : (%s)] = %s)",
                         array_str, index_str, array_str, index_str, index_str, value_str);
}
