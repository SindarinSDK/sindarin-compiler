/* ============================================================================
 * type_checker_expr_lambda.c - Lambda Expression Type Checking
 * ============================================================================
 * Type checking for lambda expressions including parameter type inference,
 * return type validation, and body type checking.
 * Extracted from type_checker_expr.c for modularity.
 * ============================================================================ */

#include "type_checker/type_checker_expr_lambda.h"
#include "type_checker/type_checker_expr.h"
#include "type_checker/type_checker_util.h"
#include "type_checker/type_checker_stmt.h"
#include "debug.h"
#include <string.h>
#include <stdio.h>

/* ============================================================================
 * Helper: Check if a name is a parameter of the given lambda
 * ============================================================================ */
static bool is_lambda_parameter(LambdaExpr *lambda, const char *name, int name_len)
{
    for (int i = 0; i < lambda->param_count; i++)
    {
        Token *param_name = &lambda->params[i].name;
        if (param_name->length == name_len &&
            strncmp(param_name->start, name, name_len) == 0)
        {
            return true;
        }
    }
    return false;
}

/* ============================================================================
 * Helper: Check if a name is a builtin function
 * ============================================================================ */
static bool is_builtin_function(const char *name, int name_len)
{
    static const char *builtins[] = {
        "print", "len", "panic", "assert", "readln", "sleep", "toInt",
        "toDouble", "toStr", "type", "exit", NULL
    };

    for (int i = 0; builtins[i] != NULL; i++)
    {
        if ((int)strlen(builtins[i]) == name_len &&
            strncmp(builtins[i], name, name_len) == 0)
        {
            return true;
        }
    }
    return false;
}

/* Forward declaration for mutual recursion */
static bool check_native_lambda_captures_stmt(LambdaExpr *lambda, Stmt *stmt,
                                               SymbolTable *table, Token **first_capture);
static bool check_native_lambda_captures_expr(LambdaExpr *lambda, Expr *expr,
                                               SymbolTable *table, Token **first_capture);

/* ============================================================================
 * Check if an expression in a native lambda captures a variable from enclosing scope
 * Returns true if a capture was found, sets first_capture to the offending token
 * ============================================================================ */
static bool check_native_lambda_captures_expr(LambdaExpr *lambda, Expr *expr,
                                               SymbolTable *table, Token **first_capture)
{
    if (expr == NULL) return false;

    switch (expr->type)
    {
    case EXPR_VARIABLE:
    {
        const char *name = expr->as.variable.name.start;
        int name_len = expr->as.variable.name.length;

        /* Check if it's a lambda parameter - allowed */
        if (is_lambda_parameter(lambda, name, name_len))
        {
            return false;
        }

        /* Check if it's a builtin function - allowed */
        if (is_builtin_function(name, name_len))
        {
            return false;
        }

        /* Check if it's a global function or type - allowed
         * We look it up in the table. If it's a function symbol at global scope,
         * or a type/namespace, it's not a closure capture. */
        Symbol *sym = symbol_table_lookup_symbol(table, expr->as.variable.name);
        if (sym != NULL)
        {
            /* Functions, namespaces, and types at global scope are OK */
            if (sym->is_function || sym->is_namespace || sym->kind == SYMBOL_TYPE)
            {
                return false;
            }

            /* Local variables from enclosing scope = capture! */
            *first_capture = &expr->as.variable.name;
            return true;
        }

        /* Variable not found - will be reported as undefined elsewhere */
        return false;
    }
    case EXPR_BINARY:
        return check_native_lambda_captures_expr(lambda, expr->as.binary.left, table, first_capture) ||
               check_native_lambda_captures_expr(lambda, expr->as.binary.right, table, first_capture);
    case EXPR_UNARY:
        return check_native_lambda_captures_expr(lambda, expr->as.unary.operand, table, first_capture);
    case EXPR_ASSIGN:
        /* Check the assigned-to variable */
        {
            const char *name = expr->as.assign.name.start;
            int name_len = expr->as.assign.name.length;

            if (!is_lambda_parameter(lambda, name, name_len) &&
                !is_builtin_function(name, name_len))
            {
                Symbol *sym = symbol_table_lookup_symbol(table, expr->as.assign.name);
                if (sym != NULL && !sym->is_function && !sym->is_namespace && sym->kind != SYMBOL_TYPE)
                {
                    *first_capture = &expr->as.assign.name;
                    return true;
                }
            }
        }
        return check_native_lambda_captures_expr(lambda, expr->as.assign.value, table, first_capture);
    case EXPR_INDEX_ASSIGN:
        return check_native_lambda_captures_expr(lambda, expr->as.index_assign.array, table, first_capture) ||
               check_native_lambda_captures_expr(lambda, expr->as.index_assign.index, table, first_capture) ||
               check_native_lambda_captures_expr(lambda, expr->as.index_assign.value, table, first_capture);
    case EXPR_CALL:
        if (check_native_lambda_captures_expr(lambda, expr->as.call.callee, table, first_capture))
        {
            return true;
        }
        for (int i = 0; i < expr->as.call.arg_count; i++)
        {
            if (check_native_lambda_captures_expr(lambda, expr->as.call.arguments[i], table, first_capture))
            {
                return true;
            }
        }
        return false;
    case EXPR_ARRAY:
        for (int i = 0; i < expr->as.array.element_count; i++)
        {
            if (check_native_lambda_captures_expr(lambda, expr->as.array.elements[i], table, first_capture))
            {
                return true;
            }
        }
        return false;
    case EXPR_ARRAY_ACCESS:
        return check_native_lambda_captures_expr(lambda, expr->as.array_access.array, table, first_capture) ||
               check_native_lambda_captures_expr(lambda, expr->as.array_access.index, table, first_capture);
    case EXPR_INCREMENT:
    case EXPR_DECREMENT:
        return check_native_lambda_captures_expr(lambda, expr->as.operand, table, first_capture);
    case EXPR_INTERPOLATED:
        for (int i = 0; i < expr->as.interpol.part_count; i++)
        {
            if (check_native_lambda_captures_expr(lambda, expr->as.interpol.parts[i], table, first_capture))
            {
                return true;
            }
        }
        return false;
    case EXPR_MEMBER:
        return check_native_lambda_captures_expr(lambda, expr->as.member.object, table, first_capture);
    case EXPR_ARRAY_SLICE:
        return check_native_lambda_captures_expr(lambda, expr->as.array_slice.array, table, first_capture) ||
               check_native_lambda_captures_expr(lambda, expr->as.array_slice.start, table, first_capture) ||
               check_native_lambda_captures_expr(lambda, expr->as.array_slice.end, table, first_capture) ||
               check_native_lambda_captures_expr(lambda, expr->as.array_slice.step, table, first_capture);
    case EXPR_RANGE:
        return check_native_lambda_captures_expr(lambda, expr->as.range.start, table, first_capture) ||
               check_native_lambda_captures_expr(lambda, expr->as.range.end, table, first_capture);
    case EXPR_SPREAD:
        return check_native_lambda_captures_expr(lambda, expr->as.spread.array, table, first_capture);
    case EXPR_LAMBDA:
        /* Nested lambdas have their own scope - don't recurse into them */
        return false;
    case EXPR_STATIC_CALL:
        for (int i = 0; i < expr->as.static_call.arg_count; i++)
        {
            if (check_native_lambda_captures_expr(lambda, expr->as.static_call.arguments[i], table, first_capture))
            {
                return true;
            }
        }
        return false;
    case EXPR_SIZED_ARRAY_ALLOC:
        return check_native_lambda_captures_expr(lambda, expr->as.sized_array_alloc.size_expr, table, first_capture);
    case EXPR_THREAD_SPAWN:
        return check_native_lambda_captures_expr(lambda, expr->as.thread_spawn.call, table, first_capture);
    case EXPR_THREAD_SYNC:
        return check_native_lambda_captures_expr(lambda, expr->as.thread_sync.handle, table, first_capture);
    case EXPR_SYNC_LIST:
        for (int i = 0; i < expr->as.sync_list.element_count; i++)
        {
            if (check_native_lambda_captures_expr(lambda, expr->as.sync_list.elements[i], table, first_capture))
            {
                return true;
            }
        }
        return false;
    case EXPR_AS_VAL:
        return check_native_lambda_captures_expr(lambda, expr->as.as_val.operand, table, first_capture);
    case EXPR_AS_REF:
        return check_native_lambda_captures_expr(lambda, expr->as.as_ref.operand, table, first_capture);
    case EXPR_LITERAL:
    default:
        return false;
    }
}

/* ============================================================================
 * Check if a statement in a native lambda captures a variable from enclosing scope
 * Returns true if a capture was found
 * ============================================================================ */
static bool check_native_lambda_captures_stmt(LambdaExpr *lambda, Stmt *stmt,
                                               SymbolTable *table, Token **first_capture)
{
    if (stmt == NULL) return false;

    switch (stmt->type)
    {
    case STMT_EXPR:
        return check_native_lambda_captures_expr(lambda, stmt->as.expression.expression, table, first_capture);
    case STMT_VAR_DECL:
        /* Check initializer, but not the declared variable name (it's local) */
        return check_native_lambda_captures_expr(lambda, stmt->as.var_decl.initializer, table, first_capture);
    case STMT_RETURN:
        return check_native_lambda_captures_expr(lambda, stmt->as.return_stmt.value, table, first_capture);
    case STMT_BLOCK:
        for (int i = 0; i < stmt->as.block.count; i++)
        {
            if (check_native_lambda_captures_stmt(lambda, stmt->as.block.statements[i], table, first_capture))
            {
                return true;
            }
        }
        return false;
    case STMT_IF:
        return check_native_lambda_captures_expr(lambda, stmt->as.if_stmt.condition, table, first_capture) ||
               check_native_lambda_captures_stmt(lambda, stmt->as.if_stmt.then_branch, table, first_capture) ||
               check_native_lambda_captures_stmt(lambda, stmt->as.if_stmt.else_branch, table, first_capture);
    case STMT_WHILE:
        return check_native_lambda_captures_expr(lambda, stmt->as.while_stmt.condition, table, first_capture) ||
               check_native_lambda_captures_stmt(lambda, stmt->as.while_stmt.body, table, first_capture);
    case STMT_FOR:
        return check_native_lambda_captures_stmt(lambda, stmt->as.for_stmt.initializer, table, first_capture) ||
               check_native_lambda_captures_expr(lambda, stmt->as.for_stmt.condition, table, first_capture) ||
               check_native_lambda_captures_expr(lambda, stmt->as.for_stmt.increment, table, first_capture) ||
               check_native_lambda_captures_stmt(lambda, stmt->as.for_stmt.body, table, first_capture);
    case STMT_FOR_EACH:
        return check_native_lambda_captures_expr(lambda, stmt->as.for_each_stmt.iterable, table, first_capture) ||
               check_native_lambda_captures_stmt(lambda, stmt->as.for_each_stmt.body, table, first_capture);
    case STMT_LOCK:
        return check_native_lambda_captures_expr(lambda, stmt->as.lock_stmt.lock_expr, table, first_capture) ||
               check_native_lambda_captures_stmt(lambda, stmt->as.lock_stmt.body, table, first_capture);
    case STMT_FUNCTION:
        /* Don't recurse into nested functions - they have their own scope */
        return false;
    case STMT_BREAK:
    case STMT_CONTINUE:
    case STMT_IMPORT:
    case STMT_PRAGMA:
    case STMT_TYPE_DECL:
    default:
        return false;
    }
}

/* ============================================================================
 * Lambda Expression Type Checking
 * ============================================================================ */

Type *type_check_lambda(Expr *expr, SymbolTable *table)
{
    LambdaExpr *lambda = &expr->as.lambda;
    DEBUG_VERBOSE("Type checking lambda with %d params, modifier: %d", lambda->param_count, lambda->modifier);

    /* Check for missing types that couldn't be inferred */
    if (lambda->return_type == NULL)
    {
        type_error(expr->token,
                   "Cannot infer lambda return type. Provide explicit type or use typed variable declaration.");
        return NULL;
    }

    for (int i = 0; i < lambda->param_count; i++)
    {
        if (lambda->params[i].type == NULL)
        {
            type_error(expr->token,
                       "Cannot infer lambda parameter type. Provide explicit type or use typed variable declaration.");
            return NULL;
        }
    }

    /* Validate private lambda return type - only primitives allowed */
    if (lambda->modifier == FUNC_PRIVATE)
    {
        if (!can_escape_private(lambda->return_type))
        {
            type_error(expr->token,
                       "Private lambda can only return primitive types (int, double, bool, char)");
            return NULL;
        }
    }

    /* Validate parameter memory qualifiers */
    for (int i = 0; i < lambda->param_count; i++)
    {
        Parameter *param = &lambda->params[i];

        /* 'as ref' is only valid for primitive and struct types (makes them heap-allocated/passed by pointer) */
        if (param->mem_qualifier == MEM_AS_REF)
        {
            if (!is_primitive_type(param->type) && param->type->kind != TYPE_STRUCT)
            {
                type_error(expr->token, "'as ref' can only be used with primitive or struct types");
                return NULL;
            }
        }

        /* 'as val' is only meaningful for reference types (arrays, strings) */
        if (param->mem_qualifier == MEM_AS_VAL)
        {
            if (is_primitive_type(param->type))
            {
                type_error(expr->token, "'as val' is only meaningful for array types");
                return NULL;
            }
        }
    }

    /* Native lambdas cannot capture variables from enclosing scope.
     * C function pointers have no mechanism for closures.
     * Check for captures BEFORE pushing the lambda's scope. */
    if (lambda->is_native)
    {
        Token *captured_var = NULL;
        bool has_capture = false;

        if (lambda->has_stmt_body)
        {
            for (int i = 0; i < lambda->body_stmt_count; i++)
            {
                if (check_native_lambda_captures_stmt(lambda, lambda->body_stmts[i], table, &captured_var))
                {
                    has_capture = true;
                    break;
                }
            }
        }
        else if (lambda->body != NULL)
        {
            has_capture = check_native_lambda_captures_expr(lambda, lambda->body, table, &captured_var);
        }

        if (has_capture && captured_var != NULL)
        {
            char var_name[256];
            int len = captured_var->length < 255 ? captured_var->length : 255;
            strncpy(var_name, captured_var->start, len);
            var_name[len] = '\0';

            char error_msg[512];
            snprintf(error_msg, sizeof(error_msg),
                     "Native lambda cannot capture variable '%s' - use void* userdata pattern instead",
                     var_name);
            type_error(expr->token, error_msg);
            return NULL;
        }
    }

    /* Push new scope for lambda parameters */
    symbol_table_push_scope(table);

    /* Add parameters to scope */
    for (int i = 0; i < lambda->param_count; i++)
    {
        symbol_table_add_symbol_with_kind(table, lambda->params[i].name,
                                          lambda->params[i].type, SYMBOL_PARAM);
    }

    if (lambda->has_stmt_body)
    {
        /* Multi-line lambda with statement body */
        for (int i = 0; i < lambda->body_stmt_count; i++)
        {
            type_check_stmt(lambda->body_stmts[i], table, lambda->return_type);
        }
        /* Return type checking is handled by return statements within the body */
    }
    else
    {
        /* Single-line lambda with expression body */
        Type *body_type = type_check_expr(lambda->body, table);
        if (body_type == NULL)
        {
            symbol_table_pop_scope(table);
            type_error(expr->token, "Lambda body type check failed");
            return NULL;
        }

        /* Verify return type matches body */
        if (!ast_type_equals(body_type, lambda->return_type))
        {
            symbol_table_pop_scope(table);
            type_error(expr->token, "Lambda body type does not match declared return type");
            return NULL;
        }
    }

    symbol_table_pop_scope(table);

    /* Build function type */
    Type **param_types = NULL;
    if (lambda->param_count > 0)
    {
        param_types = arena_alloc(table->arena, sizeof(Type *) * lambda->param_count);
        for (int i = 0; i < lambda->param_count; i++)
        {
            param_types[i] = lambda->params[i].type;
        }
    }

    return ast_create_function_type(table->arena, lambda->return_type,
                                    param_types, lambda->param_count);
}
