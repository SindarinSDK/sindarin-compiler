#include "code_gen/expr/code_gen_expr_access.h"
#include "code_gen/expr/code_gen_expr.h"
#include "code_gen/util/code_gen_util.h"
#include "symbol_table.h"
#include "debug.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/**
 * Generate code for struct member access expression.
 * point.x -> point.x
 * ptr_to_struct.x -> ptr_to_struct->x (auto-dereference)
 */
char *code_gen_member_access_expression(CodeGen *gen, Expr *expr)
{
    DEBUG_VERBOSE("Generating member access expression");

    MemberAccessExpr *access = &expr->as.member_access;

    /* Object evaluation should not be in handle mode (structs aren't handles) */
    bool saved_handle = gen->expr_as_handle;
    gen->expr_as_handle = false;
    char *object_code = code_gen_expression(gen, access->object);
    gen->expr_as_handle = saved_handle;

    Type *object_type = access->object->expr_type;

    /* Get field name - start with Sindarin name */
    char *field_name = arena_strndup(gen->arena, access->field_name.start, access->field_name.length);

    /* Look up field in struct type to get c_alias if available */
    Type *struct_type = object_type;
    if (struct_type != NULL && struct_type->kind == TYPE_POINTER)
    {
        struct_type = struct_type->as.pointer.base_type;
    }
    Type *field_type = NULL;
    if (struct_type != NULL && struct_type->kind == TYPE_STRUCT)
    {
        StructField *field = ast_struct_get_field(struct_type, field_name);
        if (field != NULL)
        {
            field_type = field->type;
            if (field->c_alias != NULL)
            {
                field_name = arena_strdup(gen->arena, field->c_alias);
            }
            else
            {
                /* Mangle field name to avoid C reserved word conflicts */
                field_name = sn_mangle_name(gen->arena, field_name);
            }
        }
    }

    /* Build the access expression */
    char *result;
    if (object_type != NULL && object_type->kind == TYPE_POINTER)
    {
        result = arena_sprintf(gen->arena, "%s->%s", object_code, field_name);
    }
    else
    {
        result = arena_sprintf(gen->arena, "%s.%s", object_code, field_name);
    }

    /* If the field is a string/array stored as RtHandle, and the caller expects
       a raw pointer (not a handle), pin the handle to get the raw pointer.
       rt_managed_pin automatically walks the parent chain to find the handle. */
    if (gen->current_arena_var != NULL && !gen->expr_as_handle && field_type != NULL)
    {
        if (field_type->kind == TYPE_STRING)
        {
            result = arena_sprintf(gen->arena, "((char *)rt_managed_pin(%s, %s))",
                                   ARENA_VAR(gen), result);
        }
        else if (field_type->kind == TYPE_ARRAY)
        {
            const char *elem_c = get_c_array_elem_type(gen->arena, field_type->as.array.element_type);
            result = arena_sprintf(gen->arena, "((%s *)rt_managed_pin_array(%s, %s))",
                                   elem_c, ARENA_VAR(gen), result);
        }
    }

    return result;
}

/**
 * Generate code for compound assignment expression.
 * x += 5 -> x = x + 5
 * arr[i] *= 2 -> arr.data[i] = arr.data[i] * 2
 * point.x -= 1 -> point.x = point.x - 1
 *
 * For sync variables, uses atomic operations where available:
 * sync_var += 5 -> __atomic_fetch_add(&sync_var, 5, __ATOMIC_SEQ_CST)
 * sync_var -= 5 -> __atomic_fetch_sub(&sync_var, 5, __ATOMIC_SEQ_CST)
 */
char *code_gen_compound_assign_expression(CodeGen *gen, Expr *expr)
{
    DEBUG_VERBOSE("Generating compound assign expression");

    CompoundAssignExpr *compound = &expr->as.compound_assign;
    Expr *target = compound->target;
    SnTokenType op = compound->operator;
    Type *target_type = target->expr_type;

    /* Check if target is a sync variable */
    Symbol *symbol = NULL;
    if (target->type == EXPR_VARIABLE)
    {
        symbol = symbol_table_lookup_symbol(gen->symbol_table, target->as.variable.name);
    }

    /* Generate code for the value */
    char *value_code = code_gen_expression(gen, compound->value);

    /* For sync variables, use atomic operations */
    if (symbol != NULL && symbol->sync_mod == SYNC_ATOMIC)
    {
        char *var_name = sn_mangle_name(gen->arena, get_var_name(gen->arena, target->as.variable.name));
        const char *c_type = get_c_type(gen->arena, target_type);

        switch (op)
        {
            case TOKEN_PLUS:
                return arena_sprintf(gen->arena, "__atomic_fetch_add(&%s, %s, __ATOMIC_SEQ_CST)",
                                     var_name, value_code);
            case TOKEN_MINUS:
                return arena_sprintf(gen->arena, "__atomic_fetch_sub(&%s, %s, __ATOMIC_SEQ_CST)",
                                     var_name, value_code);
            case TOKEN_STAR:
            case TOKEN_SLASH:
            case TOKEN_MODULO:
            {
                /* Use CAS loop for *, /, % since no atomic builtin exists */
                const char *op_char = (op == TOKEN_STAR) ? "*" : (op == TOKEN_SLASH) ? "/" : "%";
                int cas_id = gen->temp_count++;
                return arena_sprintf(gen->arena,
                    "({ %s __old_%d__, __new_%d__; "
                    "do { __old_%d__ = __atomic_load_n(&%s, __ATOMIC_SEQ_CST); "
                    "__new_%d__ = __old_%d__ %s %s; } "
                    "while (!__atomic_compare_exchange_n(&%s, &__old_%d__, __new_%d__, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST)); "
                    "__old_%d__; })",
                    c_type, cas_id, cas_id,
                    cas_id, var_name,
                    cas_id, cas_id, op_char, value_code,
                    var_name, cas_id, cas_id,
                    cas_id);
            }
            default:
                break;
        }
    }

    /* Get the operator symbol */
    const char *op_str;
    switch (op)
    {
        case TOKEN_PLUS: op_str = "+"; break;
        case TOKEN_MINUS: op_str = "-"; break;
        case TOKEN_STAR: op_str = "*"; break;
        case TOKEN_SLASH: op_str = "/"; break;
        case TOKEN_MODULO: op_str = "%"; break;
        case TOKEN_AMPERSAND: op_str = "&"; break;
        case TOKEN_PIPE: op_str = "|"; break;
        case TOKEN_CARET: op_str = "^"; break;
        case TOKEN_LSHIFT: op_str = "<<"; break;
        case TOKEN_RSHIFT: op_str = ">>"; break;
        default:
            fprintf(stderr, "Error: Unknown compound assignment operator\n");
            exit(1);
    }

    /* Generate code for the target */
    char *target_code = code_gen_expression(gen, target);

    /* For string concatenation (str += ...), use handle-based runtime function */
    if (target_type != NULL && target_type->kind == TYPE_STRING && op == TOKEN_PLUS)
    {
        if (gen->current_arena_var != NULL && target->type == EXPR_VARIABLE)
        {
            /* Handle-based: target is an RtHandle variable.
             * Generate: var = rt_str_concat_h(arena, var, pinned_var, value)
             * target_code is already the pinned form from variable expression. */
            char *var_name = sn_mangle_name(gen->arena, get_var_name(gen->arena, target->as.variable.name));
            return arena_sprintf(gen->arena, "%s = rt_str_concat_h(%s, %s, %s, %s)",
                                 var_name, ARENA_VAR(gen), var_name, target_code, value_code);
        }
        /* Legacy non-arena context */
        return arena_sprintf(gen->arena, "%s = rt_str_concat(NULL, %s, %s)",
                             target_code, target_code, value_code);
    }

    /* For numeric types, generate: target = target op value */
    return arena_sprintf(gen->arena, "%s = %s %s %s",
                         target_code, target_code, op_str, value_code);
}

/**
 * Generate code for struct member assignment expression.
 * point.x = 5.0 -> point.x = 5.0
 * ptr_to_struct.x = 5.0 -> ptr_to_struct->x = 5.0 (auto-dereference)
 */
char *code_gen_member_assign_expression(CodeGen *gen, Expr *expr)
{
    DEBUG_VERBOSE("Generating member assign expression");

    MemberAssignExpr *assign = &expr->as.member_assign;
    Type *object_type = assign->object->expr_type;

    /* Get field name - start with Sindarin name */
    char *field_name = arena_strndup(gen->arena, assign->field_name.start, assign->field_name.length);

    /* Look up field in struct type to get c_alias and type info */
    Type *struct_type = object_type;
    if (struct_type != NULL && struct_type->kind == TYPE_POINTER)
    {
        struct_type = struct_type->as.pointer.base_type;
    }
    StructField *field = NULL;
    if (struct_type != NULL && struct_type->kind == TYPE_STRUCT)
    {
        field = ast_struct_get_field(struct_type, field_name);
        if (field != NULL && field->c_alias != NULL)
        {
            field_name = arena_strdup(gen->arena, field->c_alias);
        }
        else if (field != NULL)
        {
            /* Mangle field name to avoid C reserved word conflicts */
            field_name = sn_mangle_name(gen->arena, field_name);
        }
    }

    /* Generate object in non-handle mode (structs aren't handles) */
    bool saved_handle = gen->expr_as_handle;
    gen->expr_as_handle = false;
    char *object_code = code_gen_expression(gen, assign->object);

    /* For string/array fields stored as RtHandle, value must be in handle mode */
    if (field != NULL && gen->current_arena_var != NULL &&
        (field->type->kind == TYPE_STRING || field->type->kind == TYPE_ARRAY))
    {
        gen->expr_as_handle = true;
    }
    else
    {
        gen->expr_as_handle = false;
    }

    /* If assigning to a ref parameter's field or a global struct's field,
     * allocate the value in the appropriate arena:
     * - ref parameters: use __caller_arena__ (struct lives in caller's scope)
     * - global variables: use __main_arena__ (struct outlives all local arenas)
     * The callee's __local_arena__ is destroyed on return, which would leave
     * dangling handles in the struct. */
    char *prev_arena_var = gen->current_arena_var;
    if (assign->object->type == EXPR_VARIABLE && gen->current_arena_var != NULL &&
        field != NULL && (field->type->kind == TYPE_STRING || field->type->kind == TYPE_ARRAY))
    {
        Symbol *obj_sym = symbol_table_lookup_symbol(gen->symbol_table, assign->object->as.variable.name);
        if (obj_sym != NULL)
        {
            if (obj_sym->mem_qual == MEM_AS_REF)
            {
                gen->current_arena_var = "__caller_arena__";
            }
            else if (obj_sym->kind == SYMBOL_GLOBAL || obj_sym->declaration_scope_depth <= 1)
            {
                /* Global variable - use main arena so handles survive function return */
                gen->current_arena_var = "__main_arena__";
            }
        }
    }

    char *value_code;
    /* Handle function-typed fields: when assigning a named function,
       wrap it in a closure. Named functions are just function pointers in C,
       but function-typed fields expect __Closure__ *. */
    if (field != NULL && field->type->kind == TYPE_FUNCTION &&
        !field->type->as.function.is_native &&
        assign->value->type == EXPR_VARIABLE)
    {
        Symbol *func_sym = symbol_table_lookup_symbol(gen->symbol_table, assign->value->as.variable.name);
        if (func_sym != NULL && func_sym->is_function)
        {
            /* Generate a wrapper function that adapts the closure calling convention
             * to the named function's signature. */
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
                args_forward = arena_strdup(gen->arena, "(RtManagedArena *)rt_get_thread_arena_or(((__Closure__ *)__closure__)->arena)");
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
                arena_strndup(gen->arena, assign->value->as.variable.name.start, assign->value->as.variable.name.length));
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
                    "    __cl__;\n"
                    "})",
                    wrapper_name);
            }
            else
            {
                /* Use arena allocation */
                value_code = arena_sprintf(gen->arena,
                    "({\n"
                    "    __Closure__ *__cl__ = rt_arena_alloc(%s, sizeof(__Closure__));\n"
                    "    __cl__->fn = (void *)%s;\n"
                    "    __cl__->arena = %s;\n"
                    "    __cl__;\n"
                    "})",
                    arena_var, wrapper_name, arena_var);
            }
        }
        else
        {
            /* Not a named function - generate normally (probably a closure variable) */
            value_code = code_gen_expression(gen, assign->value);
        }
    }
    else
    {
        value_code = code_gen_expression(gen, assign->value);
    }
    gen->current_arena_var = prev_arena_var;
    gen->expr_as_handle = saved_handle;

    /* For string fields: ensure the value is copied to the current arena.
     * This is critical when the value comes from a parameter (which lives in
     * the caller's arena) but the struct will be returned. Without this copy,
     * rt_managed_promote at return time fails because it can't find the handle
     * in the local arena. */
    if (field != NULL && field->type->kind == TYPE_STRING && gen->current_arena_var != NULL)
    {
        /* Skip the copy for literal strings and function calls that already
         * allocate in the local arena. Only wrap array accesses and variable
         * references which might be from a different arena. */
        if (assign->value->type == EXPR_ARRAY_ACCESS ||
            assign->value->type == EXPR_VARIABLE ||
            assign->value->type == EXPR_MEMBER_ACCESS)
        {
            value_code = arena_sprintf(gen->arena,
                "rt_managed_strdup(%s, RT_HANDLE_NULL, (char *)rt_managed_pin(%s, %s))",
                gen->current_arena_var, gen->current_arena_var, value_code);
        }
    }

    /* Check if this is pointer-to-struct (needs -> instead of .) */
    if (object_type != NULL && object_type->kind == TYPE_POINTER)
    {
        return arena_sprintf(gen->arena, "%s->%s = %s", object_code, field_name, value_code);
    }

    return arena_sprintf(gen->arena, "%s.%s = %s", object_code, field_name, value_code);
}
