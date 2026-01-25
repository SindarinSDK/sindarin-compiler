#include "code_gen/code_gen_expr_core.h"
#include "code_gen/code_gen_expr.h"
#include "code_gen/code_gen_expr_lambda.h"
#include "code_gen/code_gen_expr_array.h"
#include "code_gen/code_gen_util.h"
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
            return arena_sprintf(gen->arena, "rt_managed_strdup(%s, RT_HANDLE_NULL, %s)",
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
        /* In handle mode (arena context), nil for strings/arrays is RT_HANDLE_NULL */
        if (gen->expr_as_handle && gen->current_arena_var != NULL)
        {
            return arena_strdup(gen->arena, "RT_HANDLE_NULL");
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
                            const char *pin_arena = gen->function_arena_var ? gen->function_arena_var : "__local_arena__";
                            return arena_sprintf(gen->arena, "(char *)rt_managed_pin(%s, %s)",
                                                 pin_arena, mangled_param);
                        }
                        else if (ptype->kind == TYPE_ARRAY)
                        {
                            const char *pin_arena = gen->function_arena_var ? gen->function_arena_var : "__local_arena__";
                            const char *elem_c = get_c_array_elem_type(gen->arena, ptype->as.array.element_type);
                            return arena_sprintf(gen->arena, "((%s *)rt_managed_pin_array(%s, %s))",
                                                 elem_c, pin_arena, mangled_param);
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
            const char *pin_arena = gen->function_arena_var ? gen->function_arena_var : "__local_arena__";
            if (symbol->type->kind == TYPE_STRING)
                return arena_sprintf(gen->arena, "(char *)rt_managed_pin(%s, %s)", pin_arena, deref);
            else if (symbol->type->kind == TYPE_ARRAY)
            {
                const char *elem_c = get_c_array_elem_type(gen->arena, symbol->type->as.array.element_type);
                return arena_sprintf(gen->arena, "((%s *)rt_managed_pin_array(%s, %s))", elem_c, pin_arena, deref);
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

    char *mangled = sn_mangle_name(gen->arena, var_name);

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
        /* Clone the global handle to the current local arena */
        return arena_sprintf(gen->arena, "rt_managed_clone(%s, __main_arena__, %s)",
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
        /* Determine the correct arena and pin function for this symbol's handle.
         * Globals are in __main_arena__, params may be from any parent arena,
         * locals in the function's arena. */
        const char *pin_arena;
        const char *pin_func = "rt_managed_pin";
        const char *pin_array_func = "rt_managed_pin_array";
        if (symbol->kind == SYMBOL_GLOBAL)
        {
            pin_arena = "__main_arena__";
        }
        else if (symbol->kind == SYMBOL_PARAM)
        {
            /* Parameters may receive handles from any parent arena (e.g., globals).
             * Use rt_managed_pin_any to search the arena tree. */
            pin_arena = "__caller_arena__";
            pin_func = "rt_managed_pin_any";
            pin_array_func = "rt_managed_pin_array_any";
        }
        else
        {
            /* Use symbol's pin_arena if available and we're in the same function
             * context (not inside a lambda where the outer arena var doesn't exist). */
            bool in_lambda = gen->function_arena_var &&
                             strcmp(gen->function_arena_var, "__lambda_arena__") == 0;
            if (symbol->pin_arena && !in_lambda)
                pin_arena = symbol->pin_arena;
            else
                pin_arena = (gen->function_arena_var ? gen->function_arena_var : "__local_arena__");
        }
        if (symbol->type->kind == TYPE_STRING)
        {
            return arena_sprintf(gen->arena, "(char *)%s(%s, %s)",
                                 pin_func, pin_arena, mangled);
        }
        else if (symbol->type->kind == TYPE_ARRAY)
        {
            const char *elem_c = get_c_array_elem_type(gen->arena, symbol->type->as.array.element_type);
            return arena_sprintf(gen->arena, "((%s *)%s(%s, %s))",
                                 elem_c, pin_array_func, pin_arena, mangled);
        }
    }

    return mangled;
}

char *code_gen_assign_expression(CodeGen *gen, AssignExpr *expr)
{
    DEBUG_VERBOSE("Entering code_gen_assign_expression");
    char *var_name = sn_mangle_name(gen->arena, get_var_name(gen->arena, expr->name));
    Symbol *symbol = symbol_table_lookup_symbol(gen->symbol_table, expr->name);
    if (symbol == NULL)
    {
        exit(1);
    }
    Type *type = symbol->type;

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
            /* Enable handle mode only when a 2D/3D any conversion will be applied,
             * or when assigning to any[] (1D conversion needs pin). */
            if ((decl_elem->kind == TYPE_ANY && src_elem->kind != TYPE_ANY) ||
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
    // Track if the conversion already produced a handle (2D/3D cases).
    bool value_is_new_handle = false;
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
                if (gen->current_arena_var != NULL) {
                    value_str = arena_sprintf(gen->arena, "%s_h(%s, %s)", conv_func, ARENA_VAR(gen), value_str);
                    value_is_new_handle = true;
                } else {
                    value_str = arena_sprintf(gen->arena, "%s(%s, %s)", conv_func, ARENA_VAR(gen), value_str);
                }
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
                if (gen->current_arena_var != NULL) {
                    value_str = arena_sprintf(gen->arena, "%s_h(%s, %s)", conv_func, ARENA_VAR(gen), value_str);
                    value_is_new_handle = true;
                } else {
                    value_str = arena_sprintf(gen->arena, "%s(%s, %s)", conv_func, ARENA_VAR(gen), value_str);
                }
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
                if (gen->current_arena_var != NULL)
                {
                    if (src_elem->kind == TYPE_STRING)
                    {
                        /* String arrays store RtHandle elements — use dedicated _h function. */
                        value_str = arena_sprintf(gen->arena,
                            "rt_array_to_any_string_h(%s, %s)",
                            ARENA_VAR(gen), value_str);
                    }
                    else
                    {
                        /* Non-string: pin source handle, then convert to RtAny*. */
                        const char *elem_c = get_c_type(gen->arena, src_elem);
                        value_str = arena_sprintf(gen->arena,
                            "%s(%s, (%s *)rt_managed_pin_array(%s, %s))",
                            conv_func, ARENA_VAR(gen), elem_c, ARENA_VAR(gen), value_str);
                    }
                }
                else
                {
                    value_str = arena_sprintf(gen->arena, "%s(%s, %s)", conv_func, ARENA_VAR(gen), value_str);
                }
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
            // Generate: ({ StructType *_tmp = (StructType *)rt_arena_alloc(arena, sizeof(StructType));
            //             StructType __src_tmp__ = value;
            //             memcpy(_tmp, &__src_tmp__, sizeof(StructType));
            //             var = *_tmp; var; })
            // This allocates in the outer arena first, then copies using memcpy
            return arena_sprintf(gen->arena,
                "({ %s *__esc_tmp__ = (%s *)rt_arena_alloc(%s, sizeof(%s)); "
                "%s __esc_src__ = %s; "
                "memcpy(__esc_tmp__, &__esc_src__, sizeof(%s)); "
                "%s = *__esc_tmp__; %s; })",
                struct_name, struct_name, ARENA_VAR(gen), struct_name,
                struct_name, value_str,
                struct_name,
                var_name, var_name);
        }
    }

    // Check if target is a global variable (needs promotion to main arena)
    bool is_global = (symbol->kind == SYMBOL_GLOBAL || symbol->declaration_scope_depth <= 1);
    bool in_arena_context = (gen->current_arena_var != NULL);

    // Check if value escapes from a loop arena to outer scope
    // The type checker marks expressions as escaping via ast_expr_mark_escapes()
    bool escapes_loop = (gen->loop_arena_depth > 0 &&
                         gen->function_arena_var != NULL &&
                         expr->value != NULL &&
                         ast_expr_escapes_scope(expr->value));

    // Determine the target arena for escaping values based on where the variable was declared.
    // symbol->arena_depth: 0 = function scope, 1 = first loop, 2 = second loop, etc.
    // gen->loop_arena_stack[i] corresponds to depth i+1 (stack[0] is depth 1)
    const char *escape_target_arena = NULL;
    if (escapes_loop)
    {
        // symbol->arena_depth accounts for function scope (depth=1) + loops
        // So depth=1 means function scope, depth=2 means first loop, etc.
        // loop_arena_stack[0] = first loop, stack[1] = second loop, etc.
        // Formula: for depth > 1, use stack[depth - 2]
        int target_depth = symbol->arena_depth;
        if (target_depth <= 1)
        {
            // Variable declared at function scope (depth 0 or 1)
            escape_target_arena = gen->function_arena_var;
        }
        else
        {
            // Variable declared in a loop arena - find the right one
            // depth=2 -> stack[0], depth=3 -> stack[1], etc.
            int stack_index = target_depth - 2;
            if (stack_index >= 0 && stack_index < gen->loop_arena_depth && gen->loop_arena_stack[stack_index] != NULL)
            {
                escape_target_arena = gen->loop_arena_stack[stack_index];
            }
            else
            {
                // Fallback to function arena if something is wrong
                escape_target_arena = gen->function_arena_var;
            }
        }
    }

    if (type->kind == TYPE_STRING)
    {
        if (in_arena_context)
        {
            if (string_as_handle)
            {
                // Value expression was evaluated in handle mode - already returns RtHandle.
                // For globals, promote the handle to main arena so it survives function return.
                if (is_global)
                {
                    return arena_sprintf(gen->arena, "(%s = rt_managed_promote(__main_arena__, %s, %s))",
                                         var_name, ARENA_VAR(gen), value_str);
                }
                // For values escaping a loop, clone to the target variable's arena
                if (escapes_loop && escape_target_arena)
                {
                    return arena_sprintf(gen->arena, "(%s = rt_managed_clone(%s, %s, %s))",
                                         var_name, escape_target_arena, ARENA_VAR(gen), value_str);
                }
                // For locals, just do a direct assignment.
                return arena_sprintf(gen->arena, "(%s = %s)", var_name, value_str);
            }
            // For handle-based strings: use rt_managed_strdup with old handle.
            // The value_str is a raw pointer (pinned by expression generator).
            // For globals, promote to main arena. For escaping, use target arena. Otherwise local.
            const char *target_arena = is_global ? "__main_arena__" :
                                       (escapes_loop && escape_target_arena ? escape_target_arena : ARENA_VAR(gen));
            return arena_sprintf(gen->arena, "(%s = rt_managed_strdup(%s, %s, %s))",
                                 var_name, target_arena, var_name, value_str);
        }
        return arena_sprintf(gen->arena, "({ char *_val = %s; if (%s) rt_free_string(%s); %s = _val; _val; })",
                             value_str, var_name, var_name, var_name);
    }
    else if (type->kind == TYPE_ARRAY && in_arena_context)
    {
        if (value_is_new_handle)
        {
            // 2D/3D conversion already produced a new handle — just assign.
            // But if escaping, need to clone to target variable's arena
            if (escapes_loop && escape_target_arena)
            {
                Type *elem_type = type->as.array.element_type;
                const char *suffix = code_gen_type_suffix(elem_type);
                return arena_sprintf(gen->arena, "(%s = rt_array_clone_%s_h(%s, 0, %s))",
                                     var_name, suffix, escape_target_arena, value_str);
            }
            return arena_sprintf(gen->arena, "(%s = %s)", var_name, value_str);
        }
        // For handle-based arrays: clone to target arena with old handle.
        Type *elem_type = type->as.array.element_type;
        const char *suffix = code_gen_type_suffix(elem_type);
        const char *target_arena = is_global ? "__main_arena__" :
                                   (escapes_loop && escape_target_arena ? escape_target_arena : ARENA_VAR(gen));
        return arena_sprintf(gen->arena, "(%s = rt_array_clone_%s_h(%s, %s, %s))",
                             var_name, suffix, target_arena, var_name, value_str);
    }
    else if (type->kind == TYPE_STRUCT && in_arena_context && is_global)
    {
        // Struct is value-copied, but string/array fields are handles.
        // Deep-promote those fields to main's arena using rt_managed_promote.
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
            // Build: ({ var = value; promote fields...; var; })
            char *result = arena_sprintf(gen->arena, "({ %s = %s; ", var_name, value_str);
            for (int i = 0; i < field_count; i++)
            {
                StructField *field = &type->as.struct_type.fields[i];
                if (field->type == NULL) continue;
                const char *c_field_name = field->c_alias != NULL ? field->c_alias : sn_mangle_name(gen->arena, field->name);
                if (field->type->kind == TYPE_STRING || field->type->kind == TYPE_ARRAY)
                {
                    result = arena_sprintf(gen->arena, "%sif (%s.%s) %s.%s = rt_managed_promote(__main_arena__, %s, %s.%s); ",
                                           result, var_name, c_field_name, var_name, c_field_name, ARENA_VAR(gen), var_name, c_field_name);
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
    char *array_str = code_gen_expression(gen, expr->array);
    char *index_str = code_gen_expression(gen, expr->index);
    char *value_str = code_gen_expression(gen, expr->value);

    // Check if index is provably non-negative (literal >= 0 or tracked loop counter)
    if (is_provably_non_negative(gen, expr->index))
    {
        // Non-negative index - direct array access
        return arena_sprintf(gen->arena, "(%s[%s] = %s)",
                             array_str, index_str, value_str);
    }

    // Check if index is a negative literal - can simplify to: arr[len + idx]
    if (expr->index->type == EXPR_LITERAL &&
        expr->index->as.literal.type != NULL &&
        (expr->index->as.literal.type->kind == TYPE_INT ||
         expr->index->as.literal.type->kind == TYPE_LONG))
    {
        // Negative literal - adjust by array length
        return arena_sprintf(gen->arena, "(%s[rt_array_length(%s) + %s] = %s)",
                             array_str, array_str, index_str, value_str);
    }

    // For potentially negative variable indices, generate runtime check
    return arena_sprintf(gen->arena, "(%s[(%s) < 0 ? rt_array_length(%s) + (%s) : (%s)] = %s)",
                         array_str, index_str, array_str, index_str, index_str, value_str);
}
