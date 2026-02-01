#include "type_checker/expr/type_checker_expr_member.h"
#include "type_checker/expr/type_checker_expr.h"
#include "type_checker/expr/call/type_checker_expr_call.h"
#include "type_checker/expr/call/type_checker_expr_call_char.h"
#include "type_checker/util/type_checker_util.h"
#include "debug.h"
#include <string.h>

Type *type_check_member(Expr *expr, SymbolTable *table)
{
    DEBUG_VERBOSE("Type checking member access: %s", expr->as.member.member_name.start);

    /* Check if this is a namespace member access (namespace.symbol) */
    if (expr->as.member.object->type == EXPR_VARIABLE)
    {
        Token ns_name = expr->as.member.object->as.variable.name;
        if (symbol_table_is_namespace(table, ns_name))
        {
            /* This is a namespace member access */
            Token member_name = expr->as.member.member_name;
            Symbol *sym = symbol_table_lookup_in_namespace(table, ns_name, member_name);
            if (sym == NULL)
            {
                /* Symbol not found in namespace - provide clear error message with suggestions */
                char ns_str[128], member_str[128];
                int ns_len = ns_name.length < 127 ? ns_name.length : 127;
                int member_len = member_name.length < 127 ? member_name.length : 127;
                memcpy(ns_str, ns_name.start, ns_len);
                ns_str[ns_len] = '\0';
                memcpy(member_str, member_name.start, member_len);
                member_str[member_len] = '\0';

                char msg[512];
                snprintf(msg, sizeof(msg), "Symbol '%s' not found in namespace '%s'", member_str, ns_str);

                /* Check if the symbol exists in global scope - suggest using it directly */
                Symbol *global_sym = symbol_table_lookup_symbol(table, member_name);
                if (global_sym != NULL && global_sym->is_function)
                {
                    char suggestion[256];
                    snprintf(suggestion, sizeof(suggestion),
                             "Did you mean to access '%s' directly instead of '%s.%s'?",
                             member_str, ns_str, member_str);
                    type_error_with_suggestion(&member_name, msg, suggestion);
                }
                else
                {
                    type_error(&member_name, msg);
                }
                return NULL;
            }
            /* Check if the symbol is a nested namespace - mark it for further access */
            if (sym->is_namespace)
            {
                /* Store info for nested namespace access - return a special marker type */
                expr->as.member.resolved_namespace = sym;
                return NULL; /* Signal that this resolved to a namespace, not a value */
            }
            /* Check if the symbol is a struct type - mark for static method access */
            if (sym->is_struct_type)
            {
                /* Store struct type for static method resolution */
                expr->as.member.resolved_struct_type = sym->type;
                /* Return NULL to signal this is a type reference, not a value.
                 * The subsequent member access will use resolved_struct_type. */
                return NULL;
            }
            if (sym->type == NULL)
            {
                type_error(&member_name, "Namespaced symbol has no type");
                return NULL;
            }
            DEBUG_VERBOSE("Found namespaced symbol with type kind: %d", sym->type->kind);
            return sym->type;
        }
    }

    /* Check if this is a nested namespace member access (parentNS.nestedNS.symbol) */
    if (expr->as.member.object->type == EXPR_MEMBER)
    {
        /* First, type-check the object to see if it resolves to a nested namespace */
        Type *obj_type = type_check_expr(expr->as.member.object, table);
        if (obj_type == NULL && expr->as.member.object->as.member.resolved_namespace != NULL)
        {
            /* The object is a nested namespace - look up the member in it */
            Symbol *nested_ns = expr->as.member.object->as.member.resolved_namespace;
            Token member_name = expr->as.member.member_name;

            /* Search for the symbol within the nested namespace */
            Symbol *sym = nested_ns->namespace_symbols;
            while (sym != NULL)
            {
                if (sym->name.length == member_name.length &&
                    memcmp(sym->name.start, member_name.start, member_name.length) == 0)
                {
                    if (sym->is_namespace)
                    {
                        /* Even deeper nesting - store for further access */
                        expr->as.member.resolved_namespace = sym;
                        return NULL;
                    }
                    if (sym->type == NULL)
                    {
                        type_error(&member_name, "Nested namespaced symbol has no type");
                        return NULL;
                    }
                    DEBUG_VERBOSE("Found symbol in nested namespace with type kind: %d", sym->type->kind);
                    return sym->type;
                }
                sym = sym->next;
            }

            /* Symbol not found in nested namespace */
            char nested_str[128], member_str[128];
            int nested_len = nested_ns->name.length < 127 ? nested_ns->name.length : 127;
            int member_len = member_name.length < 127 ? member_name.length : 127;
            memcpy(nested_str, nested_ns->name.start, nested_len);
            nested_str[nested_len] = '\0';
            memcpy(member_str, member_name.start, member_len);
            member_str[member_len] = '\0';

            char msg[512];
            snprintf(msg, sizeof(msg), "Symbol '%s' not found in nested namespace '%s'", member_str, nested_str);
            type_error(&member_name, msg);
            return NULL;
        }
        /* Check if the object resolved to a struct type (for static method access) */
        if (obj_type == NULL && expr->as.member.object->as.member.resolved_struct_type != NULL)
        {
            /* The object is a struct type reference - look up static methods */
            Type *struct_type = expr->as.member.object->as.member.resolved_struct_type;
            Token member_name = expr->as.member.member_name;

            /* Search for a static method in the struct type */
            for (int i = 0; i < struct_type->as.struct_type.method_count; i++)
            {
                StructMethod *method = &struct_type->as.struct_type.methods[i];
                if (method->is_static &&
                    member_name.length == (int)strlen(method->name) &&
                    strncmp(member_name.start, method->name, member_name.length) == 0)
                {
                    /* Found a static method - create function type for it */
                    Type **param_types = NULL;
                    if (method->param_count > 0)
                    {
                        param_types = arena_alloc(table->arena, sizeof(Type *) * method->param_count);
                        for (int j = 0; j < method->param_count; j++)
                        {
                            param_types[j] = method->params[j].type;
                        }
                    }
                    Type *func_type = ast_create_function_type(table->arena, method->return_type,
                        param_types, method->param_count);
                    func_type->as.function.is_native = method->is_native;

                    /* Store method reference for code generation */
                    expr->as.member.resolved_method = method;
                    expr->as.member.resolved_struct_type = struct_type;

                    DEBUG_VERBOSE("Found static method '%s' in namespace struct type '%s'",
                        method->name, struct_type->as.struct_type.name);
                    return func_type;
                }
            }

            /* Static method not found */
            char type_str[128], member_str[128];
            int type_len = strlen(struct_type->as.struct_type.name);
            type_len = type_len < 127 ? type_len : 127;
            int member_len = member_name.length < 127 ? member_name.length : 127;
            memcpy(type_str, struct_type->as.struct_type.name, type_len);
            type_str[type_len] = '\0';
            memcpy(member_str, member_name.start, member_len);
            member_str[member_len] = '\0';

            char msg[512];
            snprintf(msg, sizeof(msg), "Static method '%s' not found in struct type '%s'", member_str, type_str);
            type_error(&member_name, msg);
            return NULL;
        }

        /* If obj_type is not NULL, fall through to normal member access handling */
        if (obj_type != NULL)
        {
            goto normal_member_access;
        }
        return NULL;
    }

normal_member_access:;  /* Empty statement needed - labels cannot be followed directly by declarations in C11/C17 */

    Type *object_type = type_check_expr(expr->as.member.object, table);
    if (object_type == NULL)
    {
        return NULL;
    }

    /* Resolve forward references for struct types.
     * This handles cases where a struct method takes the same struct type as a parameter,
     * and the type was registered early (incomplete) for struct literal support. */
    object_type = resolve_struct_forward_reference(object_type, table);

    /* Array methods - DELEGATED to type_checker_expr_call.c */
    if (object_type->kind == TYPE_ARRAY)
    {
        Type *result = type_check_array_method(expr, object_type, expr->as.member.member_name, table);
        if (result != NULL)
        {
            return result;
        }
        /* Fall through to error handling if not a valid array method */
    }

    /* String methods - DELEGATED to type_checker_expr_call.c */
    if (object_type->kind == TYPE_STRING)
    {
        Type *result = type_check_string_method(expr, object_type, expr->as.member.member_name, table);
        if (result != NULL)
        {
            return result;
        }
        /* Fall through to error handling if not a valid string method */
    }

    /* Char methods - DELEGATED to type_checker_expr_call_char.c */
    if (object_type->kind == TYPE_CHAR)
    {
        Type *result = type_check_char_method(expr, object_type, expr->as.member.member_name, table);
        if (result != NULL)
        {
            return result;
        }
        /* Fall through to error handling if not a valid char method */
    }

    /* Handle struct field access and methods via EXPR_MEMBER */
    if (object_type->kind == TYPE_STRUCT)
    {
        Token member_name = expr->as.member.member_name;

        /* First check if it's an instance method (non-static methods) */
        for (int i = 0; i < object_type->as.struct_type.method_count; i++)
        {
            StructMethod *method = &object_type->as.struct_type.methods[i];
            if (!method->is_static &&
                member_name.length == (int)strlen(method->name) &&
                strncmp(member_name.start, method->name, member_name.length) == 0)
            {
                /* Found an instance method - create a function type for it */
                /* Add 1 to param_count to account for implicit 'self' parameter */
                int total_params = method->param_count;
                Type **param_types = NULL;
                if (total_params > 0)
                {
                    param_types = arena_alloc(table->arena, sizeof(Type *) * total_params);
                    for (int j = 0; j < method->param_count; j++)
                    {
                        param_types[j] = method->params[j].type;
                    }
                }
                Type *func_type = ast_create_function_type(table->arena, method->return_type, param_types, total_params);
                func_type->as.function.is_native = method->is_native;

                /* Store the method reference in the expression for code generation */
                expr->as.member.resolved_method = method;
                expr->as.member.resolved_struct_type = object_type;

                return func_type;
            }
        }

        /* Check if it's a field */
        for (int i = 0; i < object_type->as.struct_type.field_count; i++)
        {
            StructField *field = &object_type->as.struct_type.fields[i];
            if (member_name.length == (int)strlen(field->name) &&
                strncmp(member_name.start, field->name, member_name.length) == 0)
            {
                return field->type;
            }
        }
        /* Field/method not found - fall through to error handling */
    }

    /* Handle pointer-to-struct field access via EXPR_MEMBER */
    if (object_type->kind == TYPE_POINTER &&
        object_type->as.pointer.base_type != NULL &&
        object_type->as.pointer.base_type->kind == TYPE_STRUCT)
    {
        /* Check native or method context requirement */
        if (!native_context_is_active() && !method_context_is_active())
        {
            Type *struct_type = object_type->as.pointer.base_type;
            char msg[512];
            snprintf(msg, sizeof(msg),
                     "Pointer to struct member access requires native function context. "
                     "Declare the function with 'native fn' to access '*%s' fields",
                     struct_type->as.struct_type.name);
            type_error(expr->token, msg);
            return NULL;
        }

        Type *struct_type = object_type->as.pointer.base_type;
        Token field_name = expr->as.member.member_name;

        /* Check for instance methods first (e.g., self.method() inside a method body) */
        for (int i = 0; i < struct_type->as.struct_type.method_count; i++)
        {
            StructMethod *method = &struct_type->as.struct_type.methods[i];
            if (!method->is_static &&
                field_name.length == (int)strlen(method->name) &&
                strncmp(field_name.start, method->name, field_name.length) == 0)
            {
                int total_params = method->param_count;
                Type **param_types = NULL;
                if (total_params > 0)
                {
                    param_types = arena_alloc(table->arena, sizeof(Type *) * total_params);
                    for (int j = 0; j < method->param_count; j++)
                    {
                        param_types[j] = method->params[j].type;
                    }
                }
                Type *func_type = ast_create_function_type(table->arena, method->return_type, param_types, total_params);
                func_type->as.function.is_native = method->is_native;
                expr->as.member.resolved_method = method;
                expr->as.member.resolved_struct_type = struct_type;
                return func_type;
            }
        }

        /* Check for fields */
        for (int i = 0; i < struct_type->as.struct_type.field_count; i++)
        {
            StructField *field = &struct_type->as.struct_type.fields[i];
            if (field_name.length == (int)strlen(field->name) &&
                strncmp(field_name.start, field->name, field_name.length) == 0)
            {
                return field->type;
            }
        }
        /* Field/method not found - fall through to error handling */
    }

    /* No valid method found */
    {
        /* Create null-terminated member name for error message */
        char member_name[128];
        int name_len = expr->as.member.member_name.length;
        int copy_len = name_len < 127 ? name_len : 127;
        memcpy(member_name, expr->as.member.member_name.start, copy_len);
        member_name[copy_len] = '\0';

        invalid_member_error(expr->token, object_type, member_name);
        return NULL;
    }
}
