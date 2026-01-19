#include "type_checker/type_checker_expr.h"
#include "type_checker/type_checker_expr_call.h"
#include "type_checker/type_checker_expr_call_array.h"
#include "type_checker/type_checker_expr_call_string.h"
#include "type_checker/type_checker_expr_array.h"
#include "type_checker/type_checker_expr_lambda.h"
#include "type_checker/type_checker_util.h"
#include "type_checker/type_checker_stmt.h"
#include "symbol_table/symbol_table_thread.h"
#include "debug.h"
#include <string.h>

/* Helper to get the base scope depth from an expression.
 * For EXPR_VARIABLE: returns the symbol's declaration_scope_depth
 * For EXPR_MEMBER_ACCESS: returns the already-computed scope_depth (propagated from base)
 * Returns -1 if unable to determine scope depth. */
static int get_expr_scope_depth(Expr *expr, SymbolTable *table)
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
static void mark_member_access_chain_escaped(Expr *expr)
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
static int get_base_scope_depth(Expr *expr, SymbolTable *table)
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

static Type *type_check_binary(Expr *expr, SymbolTable *table)
{
    DEBUG_VERBOSE("Type checking binary expression with operator: %d", expr->as.binary.operator);
    Type *left = type_check_expr(expr->as.binary.left, table);
    Type *right = type_check_expr(expr->as.binary.right, table);
    if (left == NULL || right == NULL)
    {
        type_error(expr->token, "Invalid operand in binary expression");
        return NULL;
    }
    SnTokenType op = expr->as.binary.operator;

    /* Reject pointer arithmetic - pointers cannot be used with arithmetic operators.
     * This includes +, -, *, /, %. Pointer comparison (==, !=) with nil is still allowed. */
    if (is_arithmetic_operator(op) || op == TOKEN_PLUS)
    {
        if (left->kind == TYPE_POINTER || right->kind == TYPE_POINTER)
        {
            type_error(expr->token, "Pointer arithmetic is not allowed");
            return NULL;
        }
    }

    if (is_comparison_operator(op))
    {
        /* Allow numeric type promotion for comparisons (int vs double) */
        if (!ast_type_equals(left, right))
        {
            /* Check if both are numeric types - promotion is allowed */
            if (is_numeric_type(left) && is_numeric_type(right))
            {
                /* This is valid - int and double can be compared */
                DEBUG_VERBOSE("Numeric type promotion in comparison allowed");
            }
            else
            {
                type_error(expr->token, "Type mismatch in comparison");
                return NULL;
            }
        }
        DEBUG_VERBOSE("Returning BOOL type for comparison operator");
        return ast_create_primitive_type(table->arena, TYPE_BOOL);
    }
    else if (is_arithmetic_operator(op))
    {
        Type *promoted = get_promoted_type(table->arena, left, right);
        if (promoted == NULL)
        {
            type_error(expr->token, "Invalid types for arithmetic operator");
            return NULL;
        }
        DEBUG_VERBOSE("Returning promoted type for arithmetic operator");
        return promoted;
    }
    else if (op == TOKEN_PLUS)
    {
        /* Check for numeric type promotion */
        Type *promoted = get_promoted_type(table->arena, left, right);
        if (promoted != NULL)
        {
            DEBUG_VERBOSE("Returning promoted type for numeric + operator");
            return promoted;
        }
        else if (left->kind == TYPE_STRING && is_printable_type(right))
        {
            DEBUG_VERBOSE("Returning STRING type for string + printable");
            return left;
        }
        else if (is_printable_type(left) && right->kind == TYPE_STRING)
        {
            DEBUG_VERBOSE("Returning STRING type for printable + string");
            return right;
        }
        else
        {
            type_error(expr->token, "Invalid types for + operator");
            return NULL;
        }
    }
    else if (op == TOKEN_AND || op == TOKEN_OR)
    {
        // Logical operators require boolean operands
        if (left->kind != TYPE_BOOL || right->kind != TYPE_BOOL)
        {
            type_error(expr->token, "Logical operators require boolean operands");
            return NULL;
        }
        DEBUG_VERBOSE("Returning BOOL type for logical operator");
        return ast_create_primitive_type(table->arena, TYPE_BOOL);
    }
    else
    {
        type_error(expr->token, "Invalid binary operator");
        return NULL;
    }
}

static Type *type_check_unary(Expr *expr, SymbolTable *table)
{
    DEBUG_VERBOSE("Type checking unary expression with operator: %d", expr->as.unary.operator);
    Type *operand = type_check_expr(expr->as.unary.operand, table);
    if (operand == NULL)
    {
        type_error(expr->token, "Invalid operand in unary expression");
        return NULL;
    }
    if (expr->as.unary.operator == TOKEN_MINUS)
    {
        if (!is_numeric_type(operand))
        {
            type_error(expr->token, "Unary minus on non-numeric");
            return NULL;
        }
        DEBUG_VERBOSE("Returning operand type for unary minus");
        return operand;
    }
    else if (expr->as.unary.operator == TOKEN_BANG)
    {
        if (operand->kind != TYPE_BOOL)
        {
            type_error(expr->token, "Unary ! on non-bool");
            return NULL;
        }
        DEBUG_VERBOSE("Returning operand type for unary !");
        return operand;
    }
    type_error(expr->token, "Invalid unary operator");
    return NULL;
}

static Type *type_check_interpolated(Expr *expr, SymbolTable *table)
{
    DEBUG_VERBOSE("Type checking interpolated string with %d parts", expr->as.interpol.part_count);
    for (int i = 0; i < expr->as.interpol.part_count; i++)
    {
        Type *part_type = type_check_expr(expr->as.interpol.parts[i], table);
        if (part_type == NULL)
        {
            type_error(expr->token, "Invalid expression in interpolated string part");
            return NULL;
        }
        if (!is_printable_type(part_type))
        {
            type_error(expr->token, "Non-printable type in interpolated string");
            return NULL;
        }
    }
    DEBUG_VERBOSE("Returning STRING type for interpolated string");
    return ast_create_primitive_type(table->arena, TYPE_STRING);
}

static Type *type_check_literal(Expr *expr, SymbolTable *table)
{
    (void)table;
    DEBUG_VERBOSE("Type checking literal expression");
    return expr->as.literal.type;
}

static Type *type_check_variable(Expr *expr, SymbolTable *table)
{
    DEBUG_VERBOSE("Type checking variable: %.*s", expr->as.variable.name.length, expr->as.variable.name.start);
    Symbol *sym = symbol_table_lookup_symbol(table, expr->as.variable.name);
    if (sym == NULL)
    {
        undefined_variable_error(&expr->as.variable.name, table);
        return NULL;
    }
    if (sym->type == NULL)
    {
        /* Check if this is a namespace being used incorrectly as a variable */
        if (sym->is_namespace)
        {
            char name_str[128];
            int name_len = expr->as.variable.name.length < 127 ? expr->as.variable.name.length : 127;
            memcpy(name_str, expr->as.variable.name.start, name_len);
            name_str[name_len] = '\0';

            char msg[256];
            snprintf(msg, sizeof(msg), "'%s' is a namespace, not a variable", name_str);
            type_error_with_suggestion(&expr->as.variable.name, msg,
                "Use namespace.symbol to access symbols in a namespace");
        }
        else
        {
            type_error(&expr->as.variable.name, "Symbol has no type");
        }
        return NULL;
    }

    /* Check if variable is a pending thread (not yet synchronized) */
    if (symbol_table_is_pending(sym))
    {
        char msg[256];
        snprintf(msg, sizeof(msg),
                 "Cannot access pending thread variable '%.*s' before synchronization (use %.*s! to sync)",
                 expr->as.variable.name.length, expr->as.variable.name.start,
                 expr->as.variable.name.length, expr->as.variable.name.start);
        type_error(&expr->as.variable.name, msg);
        return NULL;
    }

    DEBUG_VERBOSE("Variable type found: %d", sym->type->kind);
    return sym->type;
}

static Type *type_check_assign(Expr *expr, SymbolTable *table)
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

    /* Check if variable is frozen (captured by pending thread) */
    if (symbol_table_is_frozen(sym))
    {
        char msg[256];
        snprintf(msg, sizeof(msg), "Cannot modify frozen variable '%.*s' (captured by pending thread)",
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

    // Escape analysis: check if non-primitive is escaping a private block
    // Symbol's arena_depth tells us when it was declared
    // Current arena_depth tells us if we're in a private block
    int current_depth = symbol_table_get_arena_depth(table);
    if (current_depth > sym->arena_depth && !can_escape_private(value_type))
    {
        const char *reason = get_private_escape_block_reason(value_type);
        char error_msg[512];
        if (reason != NULL)
        {
            snprintf(error_msg, sizeof(error_msg),
                     "Cannot assign to variable declared outside private block: %s", reason);
        }
        else
        {
            snprintf(error_msg, sizeof(error_msg),
                     "Cannot assign non-primitive type to variable declared outside private block");
        }
        type_error(&expr->as.assign.name, error_msg);
        return NULL;
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

static Type *type_check_index_assign(Expr *expr, SymbolTable *table)
{
    DEBUG_VERBOSE("Type checking index assignment");

    /* Check if array is a frozen variable (captured by pending thread) */
    Expr *array_expr = expr->as.index_assign.array;
    if (array_expr->type == EXPR_VARIABLE)
    {
        Symbol *sym = symbol_table_lookup_symbol(table, array_expr->as.variable.name);
        if (sym != NULL && symbol_table_is_frozen(sym))
        {
            char msg[256];
            snprintf(msg, sizeof(msg), "Cannot modify frozen variable '%.*s' (captured by pending thread)",
                     array_expr->as.variable.name.length, array_expr->as.variable.name.start);
            type_error(expr->token, msg);
            return NULL;
        }
    }

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

static Type *type_check_increment_decrement(Expr *expr, SymbolTable *table)
{
    DEBUG_VERBOSE("Type checking %s expression", expr->type == EXPR_INCREMENT ? "increment" : "decrement");

    /* Check if operand is a frozen variable (captured by pending thread) */
    if (expr->as.operand->type == EXPR_VARIABLE)
    {
        Symbol *sym = symbol_table_lookup_symbol(table, expr->as.operand->as.variable.name);
        if (sym != NULL && symbol_table_is_frozen(sym))
        {
            char msg[256];
            snprintf(msg, sizeof(msg), "Cannot modify frozen variable '%.*s' (captured by pending thread)",
                     expr->as.operand->as.variable.name.length, expr->as.operand->as.variable.name.start);
            type_error(expr->token, msg);
            return NULL;
        }
    }

    Type *operand_type = type_check_expr(expr->as.operand, table);
    if (operand_type == NULL || !is_numeric_type(operand_type))
    {
        type_error(expr->token, "Increment/decrement on non-numeric type");
        return NULL;
    }
    return operand_type;
}

static Type *type_check_member(Expr *expr, SymbolTable *table)
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
            if (sym->type == NULL)
            {
                type_error(&member_name, "Namespaced symbol has no type");
                return NULL;
            }
            DEBUG_VERBOSE("Found namespaced symbol with type kind: %d", sym->type->kind);
            return sym->type;
        }
    }

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
        /* Check for mutating methods on frozen arrays */
        const char *method_name = expr->as.member.member_name.start;
        bool is_mutating = (strcmp(method_name, "push") == 0 ||
                           strcmp(method_name, "pop") == 0 ||
                           strcmp(method_name, "clear") == 0 ||
                           strcmp(method_name, "reverse") == 0 ||
                           strcmp(method_name, "insert") == 0 ||
                           strcmp(method_name, "remove") == 0);

        if (is_mutating && expr->as.member.object->type == EXPR_VARIABLE)
        {
            Symbol *sym = symbol_table_lookup_symbol(table, expr->as.member.object->as.variable.name);
            if (sym != NULL && symbol_table_is_frozen(sym))
            {
                char msg[256];
                snprintf(msg, sizeof(msg),
                         "Cannot call mutating method '%s' on frozen array '%.*s' (captured by pending thread)",
                         method_name,
                         expr->as.member.object->as.variable.name.length,
                         expr->as.member.object->as.variable.name.start);
                type_error(&expr->as.member.member_name, msg);
                return NULL;
            }
        }

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
        for (int i = 0; i < struct_type->as.struct_type.field_count; i++)
        {
            StructField *field = &struct_type->as.struct_type.fields[i];
            if (field_name.length == (int)strlen(field->name) &&
                strncmp(field_name.start, field->name, field_name.length) == 0)
            {
                return field->type;
            }
        }
        /* Field not found - fall through to error handling */
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

/* Thread spawn expression type checking - &fn() or &Type.method() */
static Type *type_check_thread_spawn(Expr *expr, SymbolTable *table)
{
    Expr *call = expr->as.thread_spawn.call;

    /* Validate that the spawn expression is a function call or static call */
    if (call == NULL || (call->type != EXPR_CALL && call->type != EXPR_STATIC_CALL))
    {
        type_error(expr->token, "Thread spawn requires function call");
        return NULL;
    }

    /* Handle static method calls like &Process.run(...) */
    if (call->type == EXPR_STATIC_CALL)
    {
        /* Type check the static call expression */
        Type *result_type = type_check_expr(call, table);
        if (result_type == NULL)
        {
            return NULL;
        }

        /* Static methods use default modifier (no shared/private support) */
        expr->as.thread_spawn.modifier = FUNC_DEFAULT;

        DEBUG_VERBOSE("Thread spawn static call type checked, return type: %d", result_type->kind);
        return result_type;
    }

    /* Regular function call handling below */

    /* Type check the call expression to get the function type */
    Expr *callee = call->as.call.callee;
    Type *func_type = type_check_expr(callee, table);
    if (func_type == NULL)
    {
        type_error(expr->token, "Cannot resolve function in thread spawn");
        return NULL;
    }

    /* Validate that the callee is a function type */
    if (func_type->kind != TYPE_FUNCTION)
    {
        type_error(expr->token, "Thread spawn requires function call");
        return NULL;
    }

    /* Look up the function symbol to get its DECLARED modifier (shared/private/default).
     * The symbol stores both func_mod (effective modifier for code gen) and
     * declared_func_mod (what the user wrote). For thread spawning, we need the
     * declared modifier because:
     * - Default mode: thread gets own arena, results are promoted to caller's arena
     * - Shared mode: thread uses caller's frozen arena, no promotion needed
     * Functions that are "implicitly shared" (return heap types) use declared=default
     * but effective=shared, so they spawn in default mode with result promotion. */
    FunctionModifier func_modifier = FUNC_DEFAULT;
    if (callee->type == EXPR_VARIABLE)
    {
        Symbol *func_sym = symbol_table_lookup_symbol(table, callee->as.variable.name);
        if (func_sym != NULL && func_sym->is_function)
        {
            /* Use declared modifier for thread spawn mode, not effective modifier */
            func_modifier = func_sym->declared_func_mod;
            DEBUG_VERBOSE("Thread spawn function '%.*s' has declared modifier: %d (effective: %d)",
                          callee->as.variable.name.length,
                          callee->as.variable.name.start,
                          func_modifier, func_sym->func_mod);
        }
    }
    /* Store the function modifier in the spawn expression for code generation */
    expr->as.thread_spawn.modifier = func_modifier;

    /* Also type check the full call expression to validate arguments */
    Type *result_type = type_check_expr(call, table);
    if (result_type == NULL)
    {
        return NULL;
    }

    /* Extract return type from function type */
    Type *return_type = func_type->as.function.return_type;

    /* Private functions can only return primitive types.
     * This is enforced because private functions have isolated arenas that
     * are freed immediately after execution - only primitives can escape. */
    if (func_modifier == FUNC_PRIVATE && return_type != NULL)
    {
        bool is_primitive = (return_type->kind == TYPE_INT ||
                             return_type->kind == TYPE_LONG ||
                             return_type->kind == TYPE_DOUBLE ||
                             return_type->kind == TYPE_BOOL ||
                             return_type->kind == TYPE_BYTE ||
                             return_type->kind == TYPE_CHAR ||
                             return_type->kind == TYPE_VOID);
        if (!is_primitive)
        {
            type_error(expr->token, "Private function can only return primitive types");
            return NULL;
        }
    }

    /* Freeze variables passed as arguments that are captured by reference.
     * Arrays and strings are always passed by reference.
     * Primitives with 'as ref' are also passed by reference.
     * Frozen variables cannot be modified while thread is running. */
    int arg_count = call->as.call.arg_count;
    Expr **arguments = call->as.call.arguments;
    MemoryQualifier *param_quals = func_type->as.function.param_mem_quals;
    int param_count = func_type->as.function.param_count;

    for (int i = 0; i < arg_count; i++)
    {
        Expr *arg = arguments[i];
        if (arg == NULL) continue;

        /* Only freeze variable references that are passed by reference */
        if (arg->type == EXPR_VARIABLE)
        {
            Symbol *sym = symbol_table_lookup_symbol(table, arg->as.variable.name);
            if (sym != NULL && sym->type != NULL)
            {
                bool should_freeze = false;

                /* Arrays and strings are always passed by reference - freeze them */
                if (sym->type->kind == TYPE_ARRAY || sym->type->kind == TYPE_STRING)
                {
                    should_freeze = true;
                }
                /* Check if parameter has 'as ref' qualifier (primitives by reference) */
                else if (param_quals != NULL && i < param_count)
                {
                    if (param_quals[i] == MEM_AS_REF)
                    {
                        should_freeze = true;
                        DEBUG_VERBOSE("Detected 'as ref' parameter at position %d", i);
                    }
                }

                if (should_freeze)
                {
                    symbol_table_freeze_symbol(sym);
                    DEBUG_VERBOSE("Froze variable '%.*s' for thread spawn",
                                  arg->as.variable.name.length,
                                  arg->as.variable.name.start);
                }
            }
        }
    }

    DEBUG_VERBOSE("Thread spawn type checked, return type: %d", return_type->kind);

    return return_type;
}

/* Thread sync expression type checking - var! or &fn()! or [r1,r2,r3]! */
static Type *type_check_thread_sync(Expr *expr, SymbolTable *table)
{
    Expr *handle = expr->as.thread_sync.handle;
    bool is_array = expr->as.thread_sync.is_array;

    if (handle == NULL)
    {
        type_error(expr->token, "Thread sync requires handle expression");
        return NULL;
    }

    /* Check for sync list pattern: [r1, r2, r3]! */
    if (is_array)
    {
        /* Validate handle is a sync list expression */
        if (handle->type != EXPR_SYNC_LIST)
        {
            type_error(expr->token, "Multi-sync requires [var1, var2, ...]! syntax");
            return NULL;
        }

        SyncListExpr *sync_list = &handle->as.sync_list;

        /* First pass: validate all elements are valid thread variables.
         * We validate all before syncing any to ensure atomic-like behavior. */
        for (int i = 0; i < sync_list->element_count; i++)
        {
            Expr *elem = sync_list->elements[i];
            if (elem == NULL)
            {
                type_error(expr->token, "Sync list element cannot be null");
                return NULL;
            }

            if (elem->type != EXPR_VARIABLE)
            {
                type_error(expr->token, "Sync list elements must be thread handle variables");
                return NULL;
            }

            Symbol *sym = symbol_table_lookup_symbol(table, elem->as.variable.name);
            if (sym == NULL)
            {
                type_error(expr->token, "Cannot sync unknown variable in sync list");
                return NULL;
            }

            /* Check thread state - must be either pending or already synchronized.
             * Normal state (never spawned) is an error. */
            if (sym->thread_state == THREAD_STATE_NORMAL)
            {
                type_error(expr->token, "Sync list element is not a thread variable");
                return NULL;
            }
        }

        /* Second pass: sync all pending variables.
         * Skip already synchronized ones (handles mixed states gracefully). */
        int synced_count = 0;
        for (int i = 0; i < sync_list->element_count; i++)
        {
            Expr *elem = sync_list->elements[i];
            Symbol *sym = symbol_table_lookup_symbol(table, elem->as.variable.name);

            /* Only sync if still pending - already synchronized is OK */
            if (symbol_table_is_pending(sym))
            {
                /* Sync the variable - transitions from pending to synchronized
                 * and unfreezes captured arguments */
                symbol_table_sync_variable(table, elem->as.variable.name,
                                           sym->frozen_args, sym->frozen_args_count);
                synced_count++;
            }
            /* Already synchronized variables are silently accepted */
        }

        DEBUG_VERBOSE("Sync list type checked with %d elements, %d newly synced, returning void",
                      sync_list->element_count, synced_count);

        /* Sync list returns void - no single return value */
        return ast_create_primitive_type(table->arena, TYPE_VOID);
    }

    /* Check for inline spawn-sync pattern: &fn()! */
    if (handle->type == EXPR_THREAD_SPAWN)
    {
        /* Type check the spawn expression - this validates the call */
        Type *spawn_type = type_check_thread_spawn(handle, table);
        if (spawn_type == NULL)
        {
            return NULL;
        }

        /* For inline spawn-sync, we don't mark anything as pending.
         * The thread is spawned and immediately joined, so no variable
         * is left in pending state. Just return the synchronized type. */
        DEBUG_VERBOSE("Inline spawn-sync type checked, return type: %d", spawn_type->kind);
        return spawn_type;
    }

    /* Regular sync on a variable: var! */
    if (handle->type == EXPR_VARIABLE)
    {
        Symbol *sym = symbol_table_lookup_symbol(table, handle->as.variable.name);
        if (sym == NULL)
        {
            type_error(expr->token, "Cannot sync unknown variable");
            return NULL;
        }

        /* Validate the variable is pending (was assigned a thread spawn) */
        if (!symbol_table_is_pending(sym))
        {
            type_error(expr->token, "Cannot sync variable that is not a pending thread");
            return NULL;
        }

        /* Sync the variable - transitions from pending to synchronized.
         * Pass frozen_args from the symbol to unfreeze captured arguments. */
        symbol_table_sync_variable(table, handle->as.variable.name,
                                   sym->frozen_args, sym->frozen_args_count);

        DEBUG_VERBOSE("Variable sync type checked, return type: %d", sym->type->kind);
        return sym->type;
    }

    type_error(expr->token, "Sync requires thread handle variable");
    return NULL;
}

Type *type_check_expr(Expr *expr, SymbolTable *table)
{
    if (expr == NULL)
    {
        DEBUG_VERBOSE("Expression is NULL");
        return NULL;
    }
    if (expr->expr_type)
    {
        DEBUG_VERBOSE("Using cached expression type: %d", expr->expr_type->kind);
        return expr->expr_type;
    }
    Type *t = NULL;
    DEBUG_VERBOSE("Type checking expression type: %d", expr->type);
    switch (expr->type)
    {
    case EXPR_BINARY:
        t = type_check_binary(expr, table);
        break;
    case EXPR_UNARY:
        t = type_check_unary(expr, table);
        break;
    case EXPR_LITERAL:
        t = type_check_literal(expr, table);
        break;
    case EXPR_VARIABLE:
        t = type_check_variable(expr, table);
        break;
    case EXPR_ASSIGN:
        t = type_check_assign(expr, table);
        break;
    case EXPR_INDEX_ASSIGN:
        t = type_check_index_assign(expr, table);
        break;
    case EXPR_CALL:
        t = type_check_call_expression(expr, table);
        break;
    case EXPR_ARRAY:
        t = type_check_array(expr, table);
        break;
    case EXPR_ARRAY_ACCESS:
        t = type_check_array_access(expr, table);
        break;
    case EXPR_INCREMENT:
    case EXPR_DECREMENT:
        t = type_check_increment_decrement(expr, table);
        break;
    case EXPR_INTERPOLATED:
        t = type_check_interpolated(expr, table);
        break;
    case EXPR_MEMBER:
        t = type_check_member(expr, table);
        break;
    case EXPR_ARRAY_SLICE:
        t = type_check_array_slice(expr, table);
        break;
    case EXPR_RANGE:
        t = type_check_range(expr, table);
        break;
    case EXPR_SPREAD:
        t = type_check_spread(expr, table);
        break;
    case EXPR_LAMBDA:
        t = type_check_lambda(expr, table);
        break;
    case EXPR_STATIC_CALL:
        t = type_check_static_method_call(expr, table);
        break;
    case EXPR_METHOD_CALL:
        /* Method calls are handled via EXPR_MEMBER + EXPR_CALL pattern.
         * If we reach here directly, it's a type error. */
        type_error(expr->token, "Internal error: EXPR_METHOD_CALL reached directly");
        t = NULL;
        break;
    case EXPR_SIZED_ARRAY_ALLOC:
        t = type_check_sized_array_alloc(expr, table);
        break;
    case EXPR_THREAD_SPAWN:
        t = type_check_thread_spawn(expr, table);
        break;
    case EXPR_THREAD_SYNC:
        t = type_check_thread_sync(expr, table);
        break;
    case EXPR_SYNC_LIST:
        /* Sync lists are only valid as part of thread sync: [r1, r2]!
         * A standalone sync list is a type error */
        type_error(expr->token, "Sync list [r1, r2, ...] must be followed by '!' for synchronization");
        t = NULL;
        break;
    case EXPR_AS_VAL:
        /* 'as val' has multiple uses:
         * 1. Pointer dereference: *int -> int, *double -> double, etc.
         * 2. C string conversion: *char -> str (null-terminated string)
         * 3. Struct deep copy: Struct as val -> Struct (with array fields copied)
         * 4. Array pass-through: used with pointer slices (ptr[0..len] as val) */
        {
            /* Enter as_val context so pointer slices know they're wrapped */
            as_val_context_enter();
            Type *operand_type = type_check_expr(expr->as.as_val.operand, table);
            as_val_context_exit();
            if (operand_type == NULL)
            {
                type_error(expr->token, "Invalid operand in 'as val' expression");
                t = NULL;
            }
            else if (operand_type->kind == TYPE_ARRAY)
            {
                /* Array type: 'as val' is a no-op, returns same type.
                 * This supports ptr[0..len] as val where the slice already
                 * produces an array type. */
                t = operand_type;
                expr->as.as_val.is_cstr_to_str = false;
                expr->as.as_val.is_noop = true;
                expr->as.as_val.is_struct_deep_copy = false;
                DEBUG_VERBOSE("'as val' on array type (no-op): returns same array type");
            }
            else if (operand_type->kind == TYPE_STRUCT)
            {
                /* Struct type: 'as val' creates a deep copy of the struct.
                 * Array fields inside the struct are independently copied.
                 * This ensures modifications to the copy don't affect the original. */
                t = operand_type;
                expr->as.as_val.is_cstr_to_str = false;
                expr->as.as_val.is_noop = false;
                expr->as.as_val.is_struct_deep_copy = true;
                DEBUG_VERBOSE("'as val' on struct type: returns deep copy of struct");
            }
            else if (operand_type->kind != TYPE_POINTER)
            {
                type_error(expr->token, "'as val' requires a pointer, array, or struct type operand");
                t = NULL;
            }
            else if (operand_type->as.pointer.base_type != NULL &&
                     operand_type->as.pointer.base_type->kind == TYPE_OPAQUE)
            {
                /* Opaque types cannot be dereferenced */
                type_error(expr->token, "Cannot dereference pointer to opaque type");
                t = NULL;
            }
            else
            {
                Type *base_type = operand_type->as.pointer.base_type;
                /* Special case: *char => str (null-terminated string conversion) */
                if (base_type->kind == TYPE_CHAR)
                {
                    t = ast_create_primitive_type(table->arena, TYPE_STRING);
                    expr->as.as_val.is_cstr_to_str = true;
                    expr->as.as_val.is_noop = false;
                    expr->as.as_val.is_struct_deep_copy = false;
                    DEBUG_VERBOSE("'as val' converts *char to str (null-terminated string)");
                }
                else
                {
                    /* Result type is the base type of the pointer */
                    t = base_type;
                    expr->as.as_val.is_cstr_to_str = false;
                    expr->as.as_val.is_noop = false;
                    expr->as.as_val.is_struct_deep_copy = false;
                    DEBUG_VERBOSE("'as val' unwraps pointer to type: %d", t->kind);
                }
            }
        }
        break;
    case EXPR_AS_REF:
        /* 'as ref' gets a pointer to a value - counterpart to 'as val':
         * int as ref -> *int
         * byte[] as ref -> *byte (pointer to array data)
         * Only allowed in native function context */
        {
            Type *operand_type = type_check_expr(expr->as.as_ref.operand, table);
            if (operand_type == NULL)
            {
                type_error(expr->token, "Invalid operand in 'as ref' expression");
                t = NULL;
            }
            else if (!native_context_is_active())
            {
                type_error(expr->token, "'as ref' is only allowed in native function bodies");
                t = NULL;
            }
            else if (operand_type->kind == TYPE_ARRAY)
            {
                /* Array: return pointer to element type (e.g., byte[] -> *byte) */
                Type *elem_type = operand_type->as.array.element_type;
                t = ast_create_pointer_type(table->arena, elem_type);
                DEBUG_VERBOSE("'as ref' on array: returns *element_type");
            }
            else if (operand_type->kind == TYPE_POINTER)
            {
                type_error(expr->token, "'as ref' cannot be applied to pointer type (already a pointer)");
                t = NULL;
            }
            else
            {
                /* For primitives and other types: return pointer to that type */
                t = ast_create_pointer_type(table->arena, operand_type);
                DEBUG_VERBOSE("'as ref' on value: returns pointer type");
            }
        }
        break;
    case EXPR_TYPEOF:
        /* typeof operator returns a type tag value for runtime comparison */
        /* typeof(value) - returns runtime type of any value */
        /* typeof(Type) - returns the type tag for that type */
        {
            if (expr->as.typeof_expr.type_literal != NULL)
            {
                /* typeof(int), typeof(str), etc. - always valid */
                t = ast_create_primitive_type(table->arena, TYPE_INT);  /* Type tag is int */
                DEBUG_VERBOSE("typeof type literal: returns type tag");
            }
            else if (expr->as.typeof_expr.operand != NULL)
            {
                Type *operand_type = type_check_expr(expr->as.typeof_expr.operand, table);
                if (operand_type == NULL)
                {
                    type_error(expr->token, "Invalid operand in typeof expression");
                    t = NULL;
                }
                else if (operand_type->kind != TYPE_ANY)
                {
                    /* For non-any types, typeof is a compile-time constant */
                    t = ast_create_primitive_type(table->arena, TYPE_INT);  /* Type tag is int */
                    DEBUG_VERBOSE("typeof non-any value: compile-time type tag");
                }
                else
                {
                    /* For any type, typeof is a runtime operation */
                    t = ast_create_primitive_type(table->arena, TYPE_INT);  /* Type tag is int */
                    DEBUG_VERBOSE("typeof any value: runtime type tag");
                }
            }
            else
            {
                type_error(expr->token, "typeof requires an operand or type");
                t = NULL;
            }
        }
        break;
    case EXPR_IS:
        /* 'is' operator: checks if an any value is of a specific type */
        /* Returns bool */
        {
            Type *operand_type = type_check_expr(expr->as.is_expr.operand, table);
            if (operand_type == NULL)
            {
                type_error(expr->token, "Invalid operand in 'is' expression");
                t = NULL;
            }
            else if (operand_type->kind != TYPE_ANY)
            {
                type_error(expr->token, "'is' operator requires an 'any' type operand");
                t = NULL;
            }
            else
            {
                t = ast_create_primitive_type(table->arena, TYPE_BOOL);
                DEBUG_VERBOSE("'is' type check: returns bool");
            }
        }
        break;
    case EXPR_AS_TYPE:
        /* 'as Type' operator: casts an any value to a concrete type,
         * or performs numeric type conversions (int -> byte, double -> int, etc.) */
        /* Returns the target type */
        {
            Type *operand_type = type_check_expr(expr->as.as_type.operand, table);
            Type *target_type = expr->as.as_type.target_type;
            if (operand_type == NULL)
            {
                type_error(expr->token, "Invalid operand in 'as' cast expression");
                t = NULL;
            }
            else if (operand_type->kind == TYPE_ANY)
            {
                /* Single any value cast to concrete type */
                t = target_type;
                DEBUG_VERBOSE("'as' type cast: returns target type %d", t->kind);
            }
            else if (operand_type->kind == TYPE_ARRAY &&
                     operand_type->as.array.element_type != NULL &&
                     operand_type->as.array.element_type->kind == TYPE_ANY)
            {
                /* any[] cast to T[] */
                if (target_type->kind != TYPE_ARRAY)
                {
                    type_error(expr->token, "'as <type>' cast from any[] requires array target type");
                    t = NULL;
                }
                else
                {
                    t = target_type;
                    DEBUG_VERBOSE("'as' array type cast: returns target type %d", t->kind);
                }
            }
            else if (is_numeric_type(operand_type) && is_numeric_type(target_type))
            {
                /* Numeric type conversion: int -> byte, double -> int, etc. */
                t = target_type;
                DEBUG_VERBOSE("'as' numeric type cast: %d -> %d", operand_type->kind, target_type->kind);
            }
            else if (operand_type->kind == TYPE_BOOL && is_numeric_type(target_type))
            {
                /* Bool to numeric conversion: true -> 1, false -> 0 */
                t = target_type;
                DEBUG_VERBOSE("'as' bool to numeric cast: -> %d", target_type->kind);
            }
            else
            {
                type_error(expr->token, "'as <type>' cast requires an 'any', 'any[]', or numeric type operand");
                t = NULL;
            }
        }
        break;
    case EXPR_STRUCT_LITERAL:
        /* Struct literal: StructName { field1: value1, field2: value2, ... } */
        {
            Token struct_name = expr->as.struct_literal.struct_name;
            Symbol *struct_sym = symbol_table_lookup_type(table, struct_name);
            if (struct_sym == NULL || struct_sym->type == NULL)
            {
                type_error(&struct_name, "Unknown struct type");
                t = NULL;
            }
            else if (struct_sym->type->kind != TYPE_STRUCT)
            {
                type_error(&struct_name, "Expected struct type");
                t = NULL;
            }
            else
            {
                Type *struct_type = struct_sym->type;

                /* Native struct usage context validation:
                 * Native structs can only be instantiated inside native fn context */
                if (struct_type->as.struct_type.is_native && !native_context_is_active())
                {
                    char msg[512];
                    snprintf(msg, sizeof(msg),
                             "Native struct '%s' can only be used in native function context. "
                             "Declare the function with 'native fn':\n"
                             "    native fn example(): void =>\n"
                             "        var x: %s = %s { ... }",
                             struct_type->as.struct_type.name,
                             struct_type->as.struct_type.name,
                             struct_type->as.struct_type.name);
                    type_error(&struct_name, msg);
                    t = NULL;
                }
                else
                {
                    /* Store resolved struct type for code generation */
                    expr->as.struct_literal.struct_type = struct_type;

                    /* Allocate and initialize the fields_initialized tracking array */
                    int total_fields = struct_type->as.struct_type.field_count;
                    expr->as.struct_literal.total_field_count = total_fields;
                    if (total_fields > 0)
                    {
                        expr->as.struct_literal.fields_initialized = arena_alloc(table->arena, sizeof(bool) * total_fields);
                        if (expr->as.struct_literal.fields_initialized == NULL)
                        {
                            type_error(&struct_name, "Out of memory allocating field initialization tracking");
                            t = NULL;
                            break;
                        }
                        /* Initialize all fields as not initialized */
                        for (int i = 0; i < total_fields; i++)
                        {
                            expr->as.struct_literal.fields_initialized[i] = false;
                        }
                    }

                    /* Type check each field initializer and mark fields as initialized */
                    for (int i = 0; i < expr->as.struct_literal.field_count; i++)
                    {
                        FieldInitializer *init = &expr->as.struct_literal.fields[i];
                        Type *init_type = type_check_expr(init->value, table);

                        /* Find the field in the struct definition */
                        bool found = false;
                        for (int j = 0; j < struct_type->as.struct_type.field_count; j++)
                        {
                            StructField *field = &struct_type->as.struct_type.fields[j];
                            if (init->name.length == strlen(field->name) &&
                                strncmp(init->name.start, field->name, init->name.length) == 0)
                            {
                                found = true;
                                /* Mark this field as explicitly initialized */
                                if (expr->as.struct_literal.fields_initialized != NULL)
                                {
                                    expr->as.struct_literal.fields_initialized[j] = true;
                                }
                                /* Type check: initializer type must match field type */
                                if (init_type != NULL && field->type != NULL &&
                                    !ast_type_equals(init_type, field->type))
                                {
                                    char msg[256];
                                    snprintf(msg, sizeof(msg),
                                             "Type mismatch for field '%s' in struct literal",
                                             field->name);
                                    type_error(&init->name, msg);
                                }
                                break;
                            }
                        }
                        if (!found)
                        {
                            char msg[256];
                            snprintf(msg, sizeof(msg),
                                     "Unknown field '%.*s' in struct literal",
                                     init->name.length, init->name.start);
                            type_error(&init->name, msg);
                        }
                    }

                    /* Apply default values for uninitialized fields that have defaults.
                     * Count how many defaults we need to add, then reallocate the fields array. */
                    int defaults_to_add = 0;
                    for (int i = 0; i < total_fields; i++)
                    {
                        if (!expr->as.struct_literal.fields_initialized[i] &&
                            struct_type->as.struct_type.fields[i].default_value != NULL)
                        {
                            defaults_to_add++;
                        }
                    }

                    if (defaults_to_add > 0)
                    {
                        /* Reallocate the fields array to hold additional synthetic initializers */
                        int new_field_count = expr->as.struct_literal.field_count + defaults_to_add;
                        FieldInitializer *new_fields = arena_alloc(table->arena, sizeof(FieldInitializer) * new_field_count);
                        if (new_fields == NULL)
                        {
                            type_error(&struct_name, "Out of memory allocating field initializers for defaults");
                            t = NULL;
                            break;
                        }

                        /* Copy existing field initializers */
                        for (int i = 0; i < expr->as.struct_literal.field_count; i++)
                        {
                            new_fields[i] = expr->as.struct_literal.fields[i];
                        }

                        /* Add synthetic initializers for fields with default values */
                        int added = 0;
                        for (int i = 0; i < total_fields; i++)
                        {
                            if (!expr->as.struct_literal.fields_initialized[i] &&
                                struct_type->as.struct_type.fields[i].default_value != NULL)
                            {
                                StructField *field = &struct_type->as.struct_type.fields[i];

                                /* Type-check the default value */
                                Type *default_type = type_check_expr(field->default_value, table);
                                if (default_type != NULL && field->type != NULL &&
                                    !ast_type_equals(default_type, field->type))
                                {
                                    char msg[256];
                                    snprintf(msg, sizeof(msg),
                                             "Type mismatch for default value of field '%s' in struct '%s'",
                                             field->name, struct_type->as.struct_type.name);
                                    type_error(expr->token, msg);
                                }

                                /* Create synthetic field initializer with the default value */
                                int idx = expr->as.struct_literal.field_count + added;

                                /* Create a Token for the field name */
                                new_fields[idx].name.start = field->name;
                                new_fields[idx].name.length = strlen(field->name);
                                new_fields[idx].name.line = expr->token ? expr->token->line : 0;
                                new_fields[idx].name.type = TOKEN_IDENTIFIER;
                                new_fields[idx].name.filename = expr->token ? expr->token->filename : NULL;
                                new_fields[idx].value = field->default_value;

                                /* Mark this field as initialized (via default) */
                                expr->as.struct_literal.fields_initialized[i] = true;

                                added++;
                            }
                        }

                        /* Update the struct literal with the new fields array */
                        expr->as.struct_literal.fields = new_fields;
                        expr->as.struct_literal.field_count = new_field_count;
                    }

                    /* Check for uninitialized fields that have no default values.
                     * These are required fields that must be explicitly provided. */
                    int missing_count = 0;
                    char missing_fields[512] = "";
                    int missing_pos = 0;

                    for (int i = 0; i < total_fields; i++)
                    {
                        if (!expr->as.struct_literal.fields_initialized[i])
                        {
                            StructField *field = &struct_type->as.struct_type.fields[i];
                            /* This field is not initialized and has no default - it's required */
                            if (missing_count > 0 && missing_pos < (int)sizeof(missing_fields) - 3)
                            {
                                missing_pos += snprintf(missing_fields + missing_pos,
                                                        sizeof(missing_fields) - missing_pos, ", ");
                            }
                            if (missing_pos < (int)sizeof(missing_fields) - 1)
                            {
                                missing_pos += snprintf(missing_fields + missing_pos,
                                                        sizeof(missing_fields) - missing_pos, "'%s'",
                                                        field->name);
                            }
                            missing_count++;
                        }
                    }

                    if (missing_count > 0)
                    {
                        char msg[768];
                        if (missing_count == 1)
                        {
                            snprintf(msg, sizeof(msg),
                                     "Missing required field %s in struct literal '%s'",
                                     missing_fields, struct_type->as.struct_type.name);
                        }
                        else
                        {
                            snprintf(msg, sizeof(msg),
                                     "Missing %d required fields in struct literal '%s': %s",
                                     missing_count, struct_type->as.struct_type.name, missing_fields);
                        }
                        type_error(&struct_name, msg);
                        t = NULL;
                    }
                    else
                    {
                        t = struct_type;
                    }
                    DEBUG_VERBOSE("Struct literal type check: returns struct type '%s'",
                                  struct_type->as.struct_type.name);
                }
            }
        }
        break;
    case EXPR_MEMBER_ACCESS:
        /* Member access: expr.field_name */
        {
            Type *object_type = type_check_expr(expr->as.member_access.object, table);
            if (object_type == NULL)
            {
                type_error(expr->token, "Invalid object in member access");
                t = NULL;
            }
            else if (object_type->kind == TYPE_STRUCT)
            {
                /* Direct struct access */
                Token field_name = expr->as.member_access.field_name;
                bool found = false;
                for (int i = 0; i < object_type->as.struct_type.field_count; i++)
                {
                    StructField *field = &object_type->as.struct_type.fields[i];
                    if (field_name.length == strlen(field->name) &&
                        strncmp(field_name.start, field->name, field_name.length) == 0)
                    {
                        found = true;
                        expr->as.member_access.field_index = i;
                        t = field->type;

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
                        break;
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
                    t = NULL;
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
                    t = NULL;
                }
                else
                {
                    Type *struct_type = object_type->as.pointer.base_type;
                    Token field_name = expr->as.member_access.field_name;
                    bool found = false;
                    for (int i = 0; i < struct_type->as.struct_type.field_count; i++)
                    {
                        StructField *field = &struct_type->as.struct_type.fields[i];
                        if (field_name.length == strlen(field->name) &&
                            strncmp(field_name.start, field->name, field_name.length) == 0)
                        {
                            found = true;
                            expr->as.member_access.field_index = i;
                            t = field->type;

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
                            break;
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
                        t = NULL;
                    }
                }
            }
            else
            {
                type_error(expr->token, "Member access requires struct or pointer to struct type");
                t = NULL;
            }
        }
        break;
    case EXPR_MEMBER_ASSIGN:
        /* Member assignment: expr.field_name = value */
        {
            Type *object_type = type_check_expr(expr->as.member_assign.object, table);
            Type *value_type = type_check_expr(expr->as.member_assign.value, table);
            if (object_type == NULL)
            {
                type_error(expr->token, "Invalid object in member assignment");
                t = NULL;
            }
            else if (object_type->kind == TYPE_STRUCT)
            {
                /* Direct struct field assignment */
                Token field_name = expr->as.member_assign.field_name;
                bool found = false;
                for (int i = 0; i < object_type->as.struct_type.field_count; i++)
                {
                    StructField *field = &object_type->as.struct_type.fields[i];
                    if (field_name.length == strlen(field->name) &&
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
                        t = field->type;
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

                        break;
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
                    t = NULL;
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
                    if (field_name.length == strlen(field->name) &&
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
                        t = field->type;
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

                        break;
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
                    t = NULL;
                }
            }
            else
            {
                type_error(expr->token, "Member assignment requires struct or pointer to struct type");
                t = NULL;
            }
        }
        break;
    case EXPR_SIZEOF:
        /* sizeof operator: returns the size in bytes of a type or expression */
        /* sizeof(Type) or sizeof(expr) - returns int */
        {
            if (expr->as.sizeof_expr.type_operand != NULL)
            {
                /* sizeof(Type) - resolve the type and get its size */
                Type *type_operand = expr->as.sizeof_expr.type_operand;

                /* If it's a struct type reference, resolve it */
                if (type_operand->kind == TYPE_STRUCT && type_operand->as.struct_type.fields == NULL)
                {
                    /* Forward reference - look up the actual struct */
                    Token name_token;
                    name_token.start = type_operand->as.struct_type.name;
                    name_token.length = strlen(type_operand->as.struct_type.name);
                    Symbol *struct_sym = symbol_table_lookup_type(table, name_token);
                    if (struct_sym != NULL && struct_sym->type != NULL &&
                        struct_sym->type->kind == TYPE_STRUCT)
                    {
                        expr->as.sizeof_expr.type_operand = struct_sym->type;
                    }
                    else
                    {
                        char msg[256];
                        snprintf(msg, sizeof(msg), "Unknown type '%s' in sizeof",
                                 type_operand->as.struct_type.name);
                        type_error(expr->token, msg);
                        t = NULL;
                        break;
                    }
                }
                t = ast_create_primitive_type(table->arena, TYPE_INT);
                DEBUG_VERBOSE("sizeof type: returns int");
            }
            else if (expr->as.sizeof_expr.expr_operand != NULL)
            {
                /* sizeof(expr) - type check the expression to get its type */
                Type *operand_type = type_check_expr(expr->as.sizeof_expr.expr_operand, table);
                if (operand_type == NULL)
                {
                    type_error(expr->token, "Invalid operand in sizeof expression");
                    t = NULL;
                }
                else
                {
                    t = ast_create_primitive_type(table->arena, TYPE_INT);
                    DEBUG_VERBOSE("sizeof expression: returns int");
                }
            }
            else
            {
                type_error(expr->token, "sizeof requires a type or expression operand");
                t = NULL;
            }
        }
        break;
    case EXPR_COMPOUND_ASSIGN:
        /* Compound assignment: x += value, x -= value, x *= value, x /= value, x %= value */
        {
            Expr *target = expr->as.compound_assign.target;
            Expr *value_expr = expr->as.compound_assign.value;
            SnTokenType op = expr->as.compound_assign.operator;

            /* Check if target is a frozen variable (captured by pending thread) */
            if (target->type == EXPR_VARIABLE)
            {
                Symbol *sym = symbol_table_lookup_symbol(table, target->as.variable.name);
                if (sym != NULL && symbol_table_is_frozen(sym))
                {
                    char msg[256];
                    snprintf(msg, sizeof(msg), "Cannot modify frozen variable '%.*s' (captured by pending thread)",
                             target->as.variable.name.length, target->as.variable.name.start);
                    type_error(expr->token, msg);
                    t = NULL;
                    break;
                }
            }

            /* Type check the target */
            Type *target_type = type_check_expr(target, table);
            if (target_type == NULL)
            {
                type_error(expr->token, "Invalid target in compound assignment");
                t = NULL;
                break;
            }

            /* Type check the value */
            Type *value_type = type_check_expr(value_expr, table);
            if (value_type == NULL)
            {
                type_error(expr->token, "Invalid value in compound assignment");
                t = NULL;
                break;
            }

            /* For compound assignment, the target must be a valid lvalue
             * (variable, array index, or member access) */
            if (target->type != EXPR_VARIABLE &&
                target->type != EXPR_ARRAY_ACCESS &&
                target->type != EXPR_MEMBER &&
                target->type != EXPR_MEMBER_ACCESS)
            {
                type_error(expr->token, "Compound assignment target must be a variable, array element, or struct field");
                t = NULL;
                break;
            }

            /* For +=, -= on strings, only += is valid (string concatenation) */
            if (target_type->kind == TYPE_STRING)
            {
                if (op == TOKEN_PLUS)
                {
                    /* String concatenation: str += value is valid if value is printable */
                    if (!is_printable_type(value_type))
                    {
                        type_error(expr->token, "Cannot concatenate non-printable type to string");
                        t = NULL;
                        break;
                    }
                    t = target_type;
                }
                else
                {
                    type_error(expr->token, "Only += is valid for string compound assignment");
                    t = NULL;
                }
                break;
            }

            /* For all other operators, target must be numeric */
            if (!is_numeric_type(target_type))
            {
                type_error(expr->token, "Compound assignment requires numeric target type");
                t = NULL;
                break;
            }

            /* Value must also be numeric */
            if (!is_numeric_type(value_type))
            {
                type_error(expr->token, "Compound assignment value must be numeric");
                t = NULL;
                break;
            }

            /* Result type is the target type */
            t = target_type;
            DEBUG_VERBOSE("Compound assignment type check passed: target type %d, op %d", target_type->kind, op);
        }
        break;
    }
    expr->expr_type = t;
    if (t != NULL)
    {
        DEBUG_VERBOSE("Expression type check result: %d", t->kind);
    }
    else
    {
        DEBUG_VERBOSE("Expression type check failed: NULL type");
    }
    return t;
}
