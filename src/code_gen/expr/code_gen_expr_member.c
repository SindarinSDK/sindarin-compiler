#include "code_gen/expr/code_gen_expr_member.h"
#include "code_gen/expr/code_gen_expr.h"
#include "code_gen/util/code_gen_util.h"
#include "debug.h"
#include "symbol_table.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

char *code_gen_member_expression(CodeGen *gen, Expr *expr)
{
    DEBUG_VERBOSE("Entering code_gen_member_expression");
    MemberExpr *member = &expr->as.member;
    char *member_name_str = get_var_name(gen->arena, member->member_name);
    Type *object_type = member->object->expr_type;

    /* Handle namespace member access (namespace.symbol).
     * If the object has no type (expr_type is NULL) and is a variable,
     * this is a namespace member reference. */
    if (object_type == NULL && member->object->type == EXPR_VARIABLE)
    {
        /* Look up the namespace symbol to check for native/c_alias and whether it's a function */
        Token ns_name = member->object->as.variable.name;
        Symbol *sym = symbol_table_lookup_in_namespace(gen->symbol_table, ns_name, member->member_name);
        if (sym != NULL && sym->is_native)
        {
            return arena_strdup(gen->arena, sym->c_alias != NULL ? sym->c_alias : member_name_str);
        }
        /* Check if this is a variable (not a function) - needs namespace prefix for uniqueness.
         * Static variables use canonical module name so all aliases share the same storage.
         * Non-static variables use namespace prefix so each alias has its own instance. */
        if (sym != NULL && !sym->is_function && sym->type != NULL && sym->type->kind != TYPE_FUNCTION)
        {
            const char *prefix_to_use = NULL;

            /* For static variables, look up the namespace to get its canonical module name */
            if (sym->is_static)
            {
                Symbol *ns_sym = symbol_table_lookup_symbol(gen->symbol_table, ns_name);
                if (ns_sym != NULL && ns_sym->canonical_module_name != NULL)
                {
                    prefix_to_use = ns_sym->canonical_module_name;
                }
            }

            /* Fall back to namespace prefix for non-static variables or if no canonical name found */
            if (prefix_to_use == NULL)
            {
                char ns_prefix[256];
                int ns_len = ns_name.length < 255 ? ns_name.length : 255;
                memcpy(ns_prefix, ns_name.start, ns_len);
                ns_prefix[ns_len] = '\0';
                prefix_to_use = arena_strdup(gen->arena, ns_prefix);
            }

            char *prefixed_name = arena_sprintf(gen->arena, "%s__%s", prefix_to_use, member_name_str);
            char *mangled = sn_mangle_name(gen->arena, prefixed_name);

            /* Handle-type namespace variables need pinning when used in contexts
             * expecting raw pointers (expr_as_handle = false). */
            if (!gen->expr_as_handle && gen->current_arena_var != NULL && is_handle_type(sym->type))
            {
                if (sym->type->kind == TYPE_STRING)
                {
                    return arena_sprintf(gen->arena, "(char *)rt_managed_pin(%s, %s)",
                                         ARENA_VAR(gen), mangled);
                }
                else if (sym->type->kind == TYPE_ARRAY)
                {
                    const char *elem_c = get_c_array_elem_type(gen->arena, sym->type->as.array.element_type);
                    return arena_sprintf(gen->arena, "((%s *)rt_managed_pin_array(%s, %s))",
                                         elem_c, ARENA_VAR(gen), mangled);
                }
            }
            return mangled;
        }
        /* Namespace function access - emit just the function name (functions are globally unique) */
        return sn_mangle_name(gen->arena, member_name_str);
    }

    /* Handle nested namespace member access (parentNS.nestedNS.symbol).
     * If the object is itself a member expression with resolved_namespace set,
     * this is a nested namespace access. Look up the symbol in the resolved namespace. */
    if (object_type == NULL && member->object->type == EXPR_MEMBER)
    {
        MemberExpr *inner_member = &member->object->as.member;
        DEBUG_VERBOSE("Checking nested namespace access: resolved_namespace=%p", (void*)inner_member->resolved_namespace);
        if (inner_member->resolved_namespace != NULL)
        {
            /* The inner member resolved to a namespace - look up the symbol in it */
            Symbol *nested_ns = inner_member->resolved_namespace;

            /* Search the nested namespace's symbols for our target */
            Symbol *func_sym = NULL;
            for (Symbol *sym = nested_ns->namespace_symbols; sym != NULL; sym = sym->next)
            {
                if (sym->name.length == member->member_name.length &&
                    memcmp(sym->name.start, member->member_name.start, sym->name.length) == 0)
                {
                    func_sym = sym;
                    break;
                }
            }

            if (func_sym != NULL && func_sym->is_native)
            {
                return arena_strdup(gen->arena, func_sym->c_alias != NULL ? func_sym->c_alias : member_name_str);
            }
            /* Nested namespace member access - emit just the function name */
            return sn_mangle_name(gen->arena, member_name_str);
        }
    }

    /* For array/string member access (.length, etc.), the object must be
     * evaluated in raw-pointer mode so handle variables get pinned. */
    bool saved_as_handle = gen->expr_as_handle;
    if (object_type != NULL && (object_type->kind == TYPE_ARRAY || object_type->kind == TYPE_STRING))
    {
        gen->expr_as_handle = false;
    }
    char *object_str = code_gen_expression(gen, member->object);
    gen->expr_as_handle = saved_as_handle;

    // Handle array.length
    if (object_type->kind == TYPE_ARRAY && strcmp(member_name_str, "length") == 0) {
        return arena_sprintf(gen->arena, "rt_array_length(%s)", object_str);
    }

    // Handle string.length
    if (object_type->kind == TYPE_STRING && strcmp(member_name_str, "length") == 0) {
        return arena_sprintf(gen->arena, "rt_str_length(%s)", object_str);
    }

    /* Handle struct field access - generates object.__sn__field */
    if (object_type->kind == TYPE_STRUCT) {
        /* Look up field to check for c_alias */
        const char *c_field_name = member_name_str;
        StructField *field = ast_struct_get_field(object_type, member_name_str);
        if (field != NULL && field->c_alias != NULL)
        {
            c_field_name = field->c_alias;
        }
        else
        {
            c_field_name = sn_mangle_name(gen->arena, member_name_str);
        }
        char *result = arena_sprintf(gen->arena, "%s.%s", object_str, c_field_name);
        /* If field is string/array stored as RtHandle and caller wants raw pointer, pin it.
         * rt_managed_pin automatically walks the parent chain to find the handle. */
        if (field != NULL && gen->current_arena_var != NULL && !gen->expr_as_handle)
        {
            if (field->type->kind == TYPE_STRING)
            {
                result = arena_sprintf(gen->arena, "((char *)rt_managed_pin(%s, %s))",
                                       ARENA_VAR(gen), result);
            }
            else if (field->type->kind == TYPE_ARRAY)
            {
                const char *elem_c = get_c_array_elem_type(gen->arena, field->type->as.array.element_type);
                result = arena_sprintf(gen->arena, "((%s *)rt_managed_pin_array(%s, %s))",
                                       elem_c, ARENA_VAR(gen), result);
            }
        }
        return result;
    }

    /* Handle pointer-to-struct field access - generates object->__sn__field */
    if (object_type->kind == TYPE_POINTER &&
        object_type->as.pointer.base_type != NULL &&
        object_type->as.pointer.base_type->kind == TYPE_STRUCT) {
        Type *base_struct = object_type->as.pointer.base_type;
        const char *c_field_name = member_name_str;
        StructField *field = ast_struct_get_field(base_struct, member_name_str);
        if (field != NULL && field->c_alias != NULL)
        {
            c_field_name = field->c_alias;
        }
        else
        {
            c_field_name = sn_mangle_name(gen->arena, member_name_str);
        }
        char *result = arena_sprintf(gen->arena, "%s->%s", object_str, c_field_name);
        /* If field is string/array stored as RtHandle and caller wants raw pointer, pin it.
         * rt_managed_pin automatically walks the parent chain to find the handle. */
        if (field != NULL && gen->current_arena_var != NULL && !gen->expr_as_handle)
        {
            if (field->type->kind == TYPE_STRING)
            {
                result = arena_sprintf(gen->arena, "((char *)rt_managed_pin(%s, %s))",
                                       ARENA_VAR(gen), result);
            }
            else if (field->type->kind == TYPE_ARRAY)
            {
                const char *elem_c = get_c_array_elem_type(gen->arena, field->type->as.array.element_type);
                result = arena_sprintf(gen->arena, "((%s *)rt_managed_pin_array(%s, %s))",
                                       elem_c, ARENA_VAR(gen), result);
            }
        }
        return result;
    }

    // Generic struct member access (not currently supported)
    fprintf(stderr, "Error: Unsupported member access on type\n");
    exit(1);
}
