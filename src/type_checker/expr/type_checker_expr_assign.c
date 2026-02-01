#include "type_checker/expr/type_checker_expr_assign.h"
#include "type_checker/expr/type_checker_expr.h"
#include "type_checker/util/type_checker_util.h"
#include "debug.h"
#include <string.h>

/* Helper to get the base scope depth from an expression.
 * For EXPR_VARIABLE: returns the symbol's declaration_scope_depth
 * For EXPR_MEMBER_ACCESS: returns the already-computed scope_depth (propagated from base)
 * Returns -1 if unable to determine scope depth. */
int get_expr_scope_depth(Expr *expr, SymbolTable *table)
{
    if (expr == NULL) return -1;

    if (expr->type == EXPR_VARIABLE)
    {
        Symbol *sym = symbol_table_lookup_symbol(table, expr->as.variable.name);
        if (sym != NULL)
        {
            return sym->declaration_scope_depth;
        }
    }
    else if (expr->type == EXPR_MEMBER_ACCESS)
    {
        /* After type checking, member access has scope_depth set */
        return expr->as.member_access.scope_depth;
    }

    return -1;
}

/* Helper to mark ALL member access nodes in a chain as escaped.
 * For an expression like outer.a.b, this marks both outer.a and outer.a.b.
 * This walks up the chain to mark all intermediate member accesses. */
void mark_member_access_chain_escaped(Expr *expr)
{
    while (expr != NULL && expr->type == EXPR_MEMBER_ACCESS)
    {
        expr->as.member_access.escaped = true;
        ast_expr_mark_escapes(expr);
        DEBUG_VERBOSE("Marked member access in chain as escaped");
        /* Walk up to the object (which may be another member access) */
        expr = expr->as.member_access.object;
    }
}

/* Helper to get the BASE (root variable) scope depth from a member access chain.
 * For outer.a.b, this returns the scope depth of 'outer' (the root variable).
 * This ensures we compare RHS scope against the actual base object scope. */
int get_base_scope_depth(Expr *expr, SymbolTable *table)
{
    if (expr == NULL) return -1;

    /* Walk down to the base of the chain */
    while (expr != NULL && expr->type == EXPR_MEMBER_ACCESS)
    {
        expr = expr->as.member_access.object;
    }

    /* Now expr should be the base variable */
    if (expr != NULL && expr->type == EXPR_VARIABLE)
    {
        Symbol *sym = symbol_table_lookup_symbol(table, expr->as.variable.name);
        if (sym != NULL)
        {
            return sym->declaration_scope_depth;
        }
    }

    return -1;
}

Type *type_check_assign(Expr *expr, SymbolTable *table)
{
    DEBUG_VERBOSE("Type checking assignment to variable: %.*s", expr->as.assign.name.length, expr->as.assign.name.start);

    /* Look up symbol first to get target type for inference */
    Symbol *sym = symbol_table_lookup_symbol(table, expr->as.assign.name);
    if (sym == NULL)
    {
        undefined_variable_error_for_assign(&expr->as.assign.name, table);
        return NULL;
    }

    /* Check if trying to assign to a namespace */
    if (sym->is_namespace)
    {
        char name_str[128];
        int name_len = expr->as.assign.name.length < 127 ? expr->as.assign.name.length : 127;
        memcpy(name_str, expr->as.assign.name.start, name_len);
        name_str[name_len] = '\0';

        char msg[256];
        snprintf(msg, sizeof(msg), "'%s' is a namespace, not a variable", name_str);
        type_error_with_suggestion(&expr->as.assign.name, msg,
            "Use namespace.symbol to access symbols in a namespace");
        return NULL;
    }

    /* Check if variable is a pending thread (cannot reassign before sync) */
    if (symbol_table_is_pending(sym))
    {
        char msg[256];
        snprintf(msg, sizeof(msg),
                 "Cannot reassign pending thread variable '%.*s' (use %.*s! to sync first)",
                 expr->as.assign.name.length, expr->as.assign.name.start,
                 expr->as.assign.name.length, expr->as.assign.name.start);
        type_error(&expr->as.assign.name, msg);
        return NULL;
    }

    /* If value is a lambda with missing types, infer from target variable's type */
    Expr *value_expr = expr->as.assign.value;
    if (value_expr != NULL && value_expr->type == EXPR_LAMBDA &&
        sym->type != NULL && sym->type->kind == TYPE_FUNCTION)
    {
        LambdaExpr *lambda = &value_expr->as.lambda;
        Type *func_type = sym->type;

        /* Check parameter count matches */
        if (lambda->param_count == func_type->as.function.param_count)
        {
            /* Infer missing parameter types */
            for (int i = 0; i < lambda->param_count; i++)
            {
                if (lambda->params[i].type == NULL)
                {
                    lambda->params[i].type = func_type->as.function.param_types[i];
                    DEBUG_VERBOSE("Inferred assignment lambda param %d type from target", i);
                }
            }

            /* Infer missing return type */
            if (lambda->return_type == NULL)
            {
                lambda->return_type = func_type->as.function.return_type;
                DEBUG_VERBOSE("Inferred assignment lambda return type from target");
            }
        }
    }

    Type *value_type = type_check_expr(value_expr, table);
    if (value_type == NULL)
    {
        type_error(expr->token, "Invalid value in assignment");
        return NULL;
    }
    // Void thread spawns cannot be assigned to variables (fire-and-forget only)
    if (value_expr->type == EXPR_THREAD_SPAWN && value_type->kind == TYPE_VOID)
    {
        type_error(&expr->as.assign.name, "Cannot assign void thread spawn to variable");
        return NULL;
    }
    // Allow assigning any concrete type to an 'any' variable (boxing)
    // Also allow assigning T[] to any[], T[][] to any[][], etc. (element-wise boxing)
    bool types_compatible = ast_type_equals(sym->type, value_type) ||
                           (sym->type->kind == TYPE_ANY && value_type != NULL);

    // Check for any[] assignment compatibility at any nesting level
    if (!types_compatible && sym->type->kind == TYPE_ARRAY && value_type != NULL && value_type->kind == TYPE_ARRAY)
    {
        // Walk down both types to find the innermost element types
        Type *decl_elem = sym->type->as.array.element_type;
        Type *init_elem = value_type->as.array.element_type;

        // Count nesting levels and check structure matches
        while (decl_elem != NULL && init_elem != NULL &&
               decl_elem->kind == TYPE_ARRAY && init_elem->kind == TYPE_ARRAY)
        {
            decl_elem = decl_elem->as.array.element_type;
            init_elem = init_elem->as.array.element_type;
        }

        // If decl's innermost element is any and init's innermost element is a concrete type, allow it
        if (decl_elem != NULL && decl_elem->kind == TYPE_ANY && init_elem != NULL)
        {
            types_compatible = true;
        }
    }

    if (!types_compatible)
    {
        type_error(&expr->as.assign.name, "Type mismatch in assignment");
        return NULL;
    }

    // Escape analysis for private functions.
    // Private functions have strict escape rules: only primitives can escape.
    // Regular functions allow strings/arrays to escape via promotion.
    int current_private_depth = symbol_table_get_private_depth(table);
    int current_arena_depth = symbol_table_get_arena_depth(table);

    if (current_private_depth > sym->private_depth && !can_escape_private(value_type))
    {
        // Escaping from a private function - strict rules apply, no cloning allowed
        const char *reason = get_private_escape_block_reason(value_type);
        char msg[512];
        snprintf(msg, sizeof(msg), "Cannot assign to variable declared outside private block: %s",
                 reason ? reason : "type contains heap data");
        type_error(&expr->as.assign.name, msg);
        return NULL;
    }
    else if (current_arena_depth > sym->arena_depth && !can_escape_private(value_type))
    {
        // Escaping from inner scope - cloning/promotion allowed for strings/arrays
        if (value_type->kind == TYPE_STRING || value_type->kind == TYPE_ARRAY)
        {
            ast_expr_mark_escapes(value_expr);
            DEBUG_VERBOSE("Arena escape (will promote): '%.*s' (arena_depth %d) assigned from depth %d",
                          expr->as.assign.name.length, expr->as.assign.name.start,
                          sym->arena_depth, current_arena_depth);
        }
        else
        {
            // Other non-escapable types (structs with heap fields, etc.): error
            const char *reason = get_private_escape_block_reason(value_type);
            char msg[512];
            snprintf(msg, sizeof(msg), "Cannot assign to variable declared outside private block: %s",
                     reason ? reason : "type contains heap data");
            type_error(&expr->as.assign.name, msg);
            return NULL;
        }
    }

    // Mark variable as pending if assigned a thread spawn (non-void)
    if (value_expr->type == EXPR_THREAD_SPAWN && value_type->kind != TYPE_VOID)
    {
        symbol_table_mark_pending(sym);
    }

    /* Escape analysis: detect when RHS variable escapes to outer scope.
     * If RHS is a variable declared in a deeper scope than LHS, the value escapes. */
    if (value_expr->type == EXPR_VARIABLE)
    {
        Symbol *rhs_sym = symbol_table_lookup_symbol(table, value_expr->as.variable.name);
        if (rhs_sym != NULL && rhs_sym->declaration_scope_depth > sym->declaration_scope_depth)
        {
            /* RHS variable is from a deeper scope - it escapes to outer scope */
            ast_expr_mark_escapes(value_expr);
            DEBUG_VERBOSE("Escape detected: variable '%.*s' (scope %d) escaping to '%.*s' (scope %d)",
                          value_expr->as.variable.name.length, value_expr->as.variable.name.start,
                          rhs_sym->declaration_scope_depth,
                          expr->as.assign.name.length, expr->as.assign.name.start,
                          sym->declaration_scope_depth);
        }
    }

    DEBUG_VERBOSE("Assignment type matches: %d", sym->type->kind);
    return sym->type;
}

Type *type_check_index_assign(Expr *expr, SymbolTable *table)
{
    DEBUG_VERBOSE("Type checking index assignment");

    Expr *array_expr = expr->as.index_assign.array;

    /* Type check the array expression */
    Type *array_type = type_check_expr(array_expr, table);
    if (array_type == NULL)
    {
        type_error(expr->token, "Invalid array in index assignment");
        return NULL;
    }

    if (array_type->kind != TYPE_ARRAY)
    {
        type_error(expr->token, "Cannot index into non-array type");
        return NULL;
    }

    /* Type check the index expression */
    Type *index_type = type_check_expr(expr->as.index_assign.index, table);
    if (index_type == NULL)
    {
        type_error(expr->token, "Invalid index expression");
        return NULL;
    }

    if (index_type->kind != TYPE_INT)
    {
        type_error(expr->token, "Array index must be an integer");
        return NULL;
    }

    /* Get element type from array */
    Type *element_type = array_type->as.array.element_type;

    /* Type check the value expression */
    Type *value_type = type_check_expr(expr->as.index_assign.value, table);
    if (value_type == NULL)
    {
        type_error(expr->token, "Invalid value in index assignment");
        return NULL;
    }

    /* Check that value type matches element type */
    if (!ast_type_equals(element_type, value_type))
    {
        type_error(expr->token, "Type mismatch in index assignment");
        return NULL;
    }

    DEBUG_VERBOSE("Index assignment type check passed");
    return element_type;
}

Type *type_check_increment_decrement(Expr *expr, SymbolTable *table)
{
    DEBUG_VERBOSE("Type checking %s expression", expr->type == EXPR_INCREMENT ? "increment" : "decrement");

    Type *operand_type = type_check_expr(expr->as.operand, table);
    if (operand_type == NULL || !is_numeric_type(operand_type))
    {
        type_error(expr->token, "Increment/decrement on non-numeric type");
        return NULL;
    }
    return operand_type;
}
