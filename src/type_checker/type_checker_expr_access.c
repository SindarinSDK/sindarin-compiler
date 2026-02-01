#include "type_checker/type_checker_expr_access.h"
#include "type_checker/type_checker_expr.h"
#include "type_checker/type_checker_expr_assign.h"
#include "type_checker/type_checker_util.h"
#include "debug.h"
#include <string.h>

/* Member access: expr.field_name */
Type *type_check_member_access(Expr *expr, SymbolTable *table)
{
    Type *object_type = type_check_expr(expr->as.member_access.object, table);
    if (object_type == NULL)
    {
        type_error(expr->token, "Invalid object in member access");
        return NULL;
    }

    /* Resolve forward references for struct types */
    if (object_type->kind == TYPE_STRUCT)
    {
        object_type = resolve_struct_forward_reference(object_type, table);
    }

    if (object_type->kind == TYPE_STRUCT)
    {
        /* Direct struct access */
        Token field_name = expr->as.member_access.field_name;
        bool found = false;
        for (int i = 0; i < object_type->as.struct_type.field_count; i++)
        {
            StructField *field = &object_type->as.struct_type.fields[i];
            if ((size_t)field_name.length == strlen(field->name) &&
                strncmp(field_name.start, field->name, field_name.length) == 0)
            {
                found = true;
                expr->as.member_access.field_index = i;
                Type *t = field->type;

                /* Set scope depth for escape analysis.
                 * For nested chains (a.b.c), propagate scope depth from base:
                 * - If object is EXPR_VARIABLE, use the variable's declaration scope depth
                 * - If object is EXPR_MEMBER_ACCESS, propagate its scope_depth
                 * - Otherwise, use current scope depth as fallback */
                Expr *object = expr->as.member_access.object;
                if (object->type == EXPR_VARIABLE)
                {
                    Symbol *base_sym = symbol_table_lookup_symbol(table, object->as.variable.name);
                    if (base_sym != NULL)
                    {
                        expr->as.member_access.scope_depth = base_sym->declaration_scope_depth;
                    }
                    else
                    {
                        expr->as.member_access.scope_depth = symbol_table_get_scope_depth(table);
                    }
                }
                else if (object->type == EXPR_MEMBER_ACCESS)
                {
                    /* Propagate scope depth from the nested member access */
                    expr->as.member_access.scope_depth = object->as.member_access.scope_depth;
                }
                else
                {
                    expr->as.member_access.scope_depth = symbol_table_get_scope_depth(table);
                }
                DEBUG_VERBOSE("Member access: field '%s' has type %d, scope_depth=%d",
                              field->name, t->kind, expr->as.member_access.scope_depth);
                return t;
            }
        }
        if (!found)
        {
            char msg[256];
            snprintf(msg, sizeof(msg),
                     "Unknown field '%.*s' in struct '%s'",
                     field_name.length, field_name.start,
                     object_type->as.struct_type.name);
            type_error(&field_name, msg);
            return NULL;
        }
    }
    else if (object_type->kind == TYPE_POINTER &&
             object_type->as.pointer.base_type != NULL &&
             object_type->as.pointer.base_type->kind == TYPE_STRUCT)
    {
        /* Pointer to struct access (auto-deref like C's ->) - only in native or method context */
        if (!native_context_is_active() && !method_context_is_active())
        {
            Type *struct_type = object_type->as.pointer.base_type;
            char msg[512];
            snprintf(msg, sizeof(msg),
                     "Pointer to struct member access requires native function or method context. "
                     "Declare the function with 'native fn' to access '*%s' fields",
                     struct_type->as.struct_type.name);
            type_error(expr->token, msg);
            return NULL;
        }
        else
        {
            Type *struct_type = object_type->as.pointer.base_type;
            Token field_name = expr->as.member_access.field_name;
            bool found = false;
            for (int i = 0; i < struct_type->as.struct_type.field_count; i++)
            {
                StructField *field = &struct_type->as.struct_type.fields[i];
                if ((size_t)field_name.length == strlen(field->name) &&
                    strncmp(field_name.start, field->name, field_name.length) == 0)
                {
                    found = true;
                    expr->as.member_access.field_index = i;
                    Type *t = field->type;

                    /* Set scope depth for escape analysis.
                     * For nested chains, propagate scope depth from base. */
                    Expr *object = expr->as.member_access.object;
                    if (object->type == EXPR_VARIABLE)
                    {
                        Symbol *base_sym = symbol_table_lookup_symbol(table, object->as.variable.name);
                        if (base_sym != NULL)
                        {
                            expr->as.member_access.scope_depth = base_sym->declaration_scope_depth;
                        }
                        else
                        {
                            expr->as.member_access.scope_depth = symbol_table_get_scope_depth(table);
                        }
                    }
                    else if (object->type == EXPR_MEMBER_ACCESS)
                    {
                        expr->as.member_access.scope_depth = object->as.member_access.scope_depth;
                    }
                    else
                    {
                        expr->as.member_access.scope_depth = symbol_table_get_scope_depth(table);
                    }
                    DEBUG_VERBOSE("Pointer member access: field '%s' has type %d, scope_depth=%d",
                                  field->name, t->kind, expr->as.member_access.scope_depth);
                    return t;
                }
            }
            if (!found)
            {
                char msg[256];
                snprintf(msg, sizeof(msg),
                         "Unknown field '%.*s' in struct '%s'",
                         field_name.length, field_name.start,
                         struct_type->as.struct_type.name);
                type_error(&field_name, msg);
                return NULL;
            }
        }
    }

    type_error(expr->token, "Member access requires struct or pointer to struct type");
    return NULL;
}

/* Member assignment: expr.field_name = value */
Type *type_check_member_assign(Expr *expr, SymbolTable *table)
{
    Type *object_type = type_check_expr(expr->as.member_assign.object, table);
    Type *value_type = type_check_expr(expr->as.member_assign.value, table);
    if (object_type == NULL)
    {
        type_error(expr->token, "Invalid object in member assignment");
        return NULL;
    }

    /* Resolve forward references for struct types */
    if (object_type->kind == TYPE_STRUCT)
    {
        object_type = resolve_struct_forward_reference(object_type, table);
    }

    if (object_type->kind == TYPE_STRUCT)
    {
        /* Direct struct field assignment */
        Token field_name = expr->as.member_assign.field_name;
        bool found = false;
        for (int i = 0; i < object_type->as.struct_type.field_count; i++)
        {
            StructField *field = &object_type->as.struct_type.fields[i];
            if ((size_t)field_name.length == strlen(field->name) &&
                strncmp(field_name.start, field->name, field_name.length) == 0)
            {
                found = true;
                /* Type check: value type must match field type */
                if (value_type != NULL && field->type != NULL &&
                    !ast_type_equals(value_type, field->type))
                {
                    char msg[256];
                    snprintf(msg, sizeof(msg),
                             "Type mismatch for field '%s' assignment",
                             field->name);
                    type_error(&field_name, msg);
                }
                Type *t = field->type;
                DEBUG_VERBOSE("Member assign: field '%s' has type %d", field->name, t->kind);

                /* Escape analysis: detect when RHS value escapes to outer scope via field assignment.
                 * If RHS is a variable declared in a deeper scope than the LHS object's BASE scope,
                 * the value escapes and ALL nodes in the LHS field access chain should be marked as escaped.
                 * For nested chains like outer.a.b = local, we compare against 'outer's scope, not 'outer.a'. */
                Expr *value_expr = expr->as.member_assign.value;
                Expr *object_expr = expr->as.member_assign.object;
                /* Get the BASE scope depth (the root variable of the chain) */
                int lhs_scope_depth = get_base_scope_depth(object_expr, table);
                int rhs_scope_depth = -1;

                if (value_expr->type == EXPR_VARIABLE)
                {
                    rhs_scope_depth = get_expr_scope_depth(value_expr, table);
                }
                else if (value_expr->type == EXPR_MEMBER_ACCESS)
                {
                    rhs_scope_depth = value_expr->as.member_access.scope_depth;
                }

                if (lhs_scope_depth >= 0 && rhs_scope_depth >= 0 &&
                    rhs_scope_depth > lhs_scope_depth)
                {
                    /* RHS is from a deeper scope - value escapes to outer scope.
                     * Mark ALL nodes in the LHS member access chain as escaped. */
                    mark_member_access_chain_escaped(object_expr);
                    ast_expr_mark_escapes(value_expr);
                    DEBUG_VERBOSE("Escape detected in field assign: RHS (scope %d) escaping to LHS field (base scope %d)",
                                  rhs_scope_depth, lhs_scope_depth);
                }

                return t;
            }
        }
        if (!found)
        {
            char msg[256];
            snprintf(msg, sizeof(msg),
                     "Unknown field '%.*s' in struct '%s'",
                     field_name.length, field_name.start,
                     object_type->as.struct_type.name);
            type_error(&field_name, msg);
            return NULL;
        }
    }
    else if (object_type->kind == TYPE_POINTER &&
             object_type->as.pointer.base_type != NULL &&
             object_type->as.pointer.base_type->kind == TYPE_STRUCT)
    {
        /* Pointer to struct field assignment (auto-deref) */
        Type *struct_type = object_type->as.pointer.base_type;
        Token field_name = expr->as.member_assign.field_name;
        bool found = false;
        for (int i = 0; i < struct_type->as.struct_type.field_count; i++)
        {
            StructField *field = &struct_type->as.struct_type.fields[i];
            if ((size_t)field_name.length == strlen(field->name) &&
                strncmp(field_name.start, field->name, field_name.length) == 0)
            {
                found = true;
                /* Type check: value type must match field type */
                if (value_type != NULL && field->type != NULL &&
                    !ast_type_equals(value_type, field->type))
                {
                    char msg[256];
                    snprintf(msg, sizeof(msg),
                             "Type mismatch for field '%s' assignment",
                             field->name);
                    type_error(&field_name, msg);
                }
                Type *t = field->type;
                DEBUG_VERBOSE("Pointer member assign: field '%s' has type %d", field->name, t->kind);

                /* Escape analysis for pointer-to-struct field assignment.
                 * Get BASE scope depth and mark ALL nodes in the chain. */
                Expr *value_expr = expr->as.member_assign.value;
                Expr *object_expr = expr->as.member_assign.object;
                int lhs_scope_depth = get_base_scope_depth(object_expr, table);
                int rhs_scope_depth = -1;

                if (value_expr->type == EXPR_VARIABLE)
                {
                    rhs_scope_depth = get_expr_scope_depth(value_expr, table);
                }
                else if (value_expr->type == EXPR_MEMBER_ACCESS)
                {
                    rhs_scope_depth = value_expr->as.member_access.scope_depth;
                }

                if (lhs_scope_depth >= 0 && rhs_scope_depth >= 0 &&
                    rhs_scope_depth > lhs_scope_depth)
                {
                    /* RHS is from a deeper scope - value escapes to outer scope.
                     * Mark ALL nodes in the LHS member access chain as escaped. */
                    mark_member_access_chain_escaped(object_expr);
                    ast_expr_mark_escapes(value_expr);
                    DEBUG_VERBOSE("Escape detected in ptr field assign: RHS (scope %d) escaping to LHS field (base scope %d)",
                                  rhs_scope_depth, lhs_scope_depth);
                }

                return t;
            }
        }
        if (!found)
        {
            char msg[256];
            snprintf(msg, sizeof(msg),
                     "Unknown field '%.*s' in struct '%s'",
                     field_name.length, field_name.start,
                     struct_type->as.struct_type.name);
            type_error(&field_name, msg);
            return NULL;
        }
    }

    type_error(expr->token, "Member assignment requires struct or pointer to struct type");
    return NULL;
}
