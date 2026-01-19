/* ============================================================================
 * type_checker_expr_call_core.c - Core Call Expression Type Checking
 * ============================================================================
 * Type checking for function calls, including built-in functions (len),
 * regular user-defined function calls, static method calls, and lambda
 * argument type inference.
 *
 * This is the core module that contains the main dispatchers and helpers.
 * Type-specific method checking is delegated to specialized modules:
 * - type_checker_expr_call_array.c for array methods
 * - type_checker_expr_call_string.c for string methods
 * ============================================================================ */

#include "type_checker/type_checker_expr_call_core.h"
#include "type_checker/type_checker_expr_call.h"
#include "type_checker/type_checker_expr_call_array.h"
#include "type_checker/type_checker_expr_call_string.h"
#include "type_checker/type_checker_expr.h"
#include "type_checker/type_checker_util.h"
#include "debug.h"
#include <string.h>
#include <stdio.h>

/* ============================================================================
 * Helper Functions
 * ============================================================================ */

/* Check if callee matches a built-in function name */
bool is_builtin_name(Expr *callee, const char *name)
{
    if (callee->type != EXPR_VARIABLE) return false;
    Token tok = callee->as.variable.name;
    size_t len = strlen(name);
    return tok.length == (int)len && strncmp(tok.start, name, len) == 0;
}

/* Compare a token's text against a string */
bool token_equals(Token tok, const char *str)
{
    size_t len = strlen(str);
    return tok.length == (int)len && strncmp(tok.start, str, len) == 0;
}

/* ============================================================================
 * Call Expression Type Checking
 * ============================================================================ */

Type *type_check_call_expression(Expr *expr, SymbolTable *table)
{
    DEBUG_VERBOSE("Type checking function call with %d arguments", expr->as.call.arg_count);

    // Handle array built-in functions specially
    Expr *callee = expr->as.call.callee;

    // len(arr) -> int (works on arrays and strings)
    if (is_builtin_name(callee, "len") && expr->as.call.arg_count == 1)
    {
        Type *arg_type = type_check_expr(expr->as.call.arguments[0], table);
        if (arg_type == NULL) return NULL;
        if (arg_type->kind != TYPE_ARRAY && arg_type->kind != TYPE_STRING)
        {
            type_error(expr->token, "len() requires array or string argument");
            return NULL;
        }
        return ast_create_primitive_type(table->arena, TYPE_INT);
    }

    // exit(code: int) -> void
    if (is_builtin_name(callee, "exit") && expr->as.call.arg_count == 1)
    {
        Type *arg_type = type_check_expr(expr->as.call.arguments[0], table);
        if (arg_type == NULL) return NULL;
        if (arg_type->kind != TYPE_INT)
        {
            type_error(expr->token, "exit() requires int argument");
            return NULL;
        }
        return ast_create_primitive_type(table->arena, TYPE_VOID);
    }

    // assert(condition: bool, message: str) -> void
    if (is_builtin_name(callee, "assert") && expr->as.call.arg_count == 2)
    {
        Type *cond_type = type_check_expr(expr->as.call.arguments[0], table);
        if (cond_type == NULL) return NULL;
        if (cond_type->kind != TYPE_BOOL)
        {
            type_error(expr->token, "assert() first argument must be bool");
            return NULL;
        }
        Type *msg_type = type_check_expr(expr->as.call.arguments[1], table);
        if (msg_type == NULL) return NULL;
        if (msg_type->kind != TYPE_STRING)
        {
            type_error(expr->token, "assert() second argument must be str");
            return NULL;
        }
        return ast_create_primitive_type(table->arena, TYPE_VOID);
    }

    // Note: Other array operations are method-style only:
    //   arr.push(elem), arr.pop(), arr.reverse(), arr.remove(idx), arr.insert(elem, idx)


    // Standard function call handling
    Type *callee_type = type_check_expr(expr->as.call.callee, table);

    /* Get function name for error messages */
    char func_name[128] = "<anonymous>";
    if (expr->as.call.callee->type == EXPR_VARIABLE)
    {
        int name_len = expr->as.call.callee->as.variable.name.length;
        int copy_len = name_len < 127 ? name_len : 127;
        memcpy(func_name, expr->as.call.callee->as.variable.name.start, copy_len);
        func_name[copy_len] = '\0';
    }

    if (callee_type == NULL)
    {
        char msg[256];
        snprintf(msg, sizeof(msg), "Invalid callee '%s' in function call", func_name);
        type_error(expr->token, msg);
        return NULL;
    }
    if (callee_type->kind != TYPE_FUNCTION)
    {
        char msg[256];
        snprintf(msg, sizeof(msg), "'%s' is of type '%s', cannot call non-function",
                 func_name, type_name(callee_type));
        type_error(expr->token, msg);
        return NULL;
    }
    int expected_params = callee_type->as.function.param_count;
    bool is_variadic = callee_type->as.function.is_variadic;

    /* For variadic functions, we need at least the fixed parameters.
     * For non-variadic functions, exact count is required. */
    if (is_variadic)
    {
        if (expr->as.call.arg_count < expected_params)
        {
            char msg[256];
            snprintf(msg, sizeof(msg), "Function '%s' requires at least %d argument(s), got %d",
                     func_name, expected_params, expr->as.call.arg_count);
            type_error(expr->token, msg);
            return NULL;
        }
    }
    else
    {
        if (expected_params != expr->as.call.arg_count)
        {
            argument_count_error(expr->token, func_name,
                                expected_params,
                                expr->as.call.arg_count);
            return NULL;
        }
    }

    /* Type check the fixed parameters */
    for (int i = 0; i < expected_params; i++)
    {
        Expr *arg_expr = expr->as.call.arguments[i];
        Type *param_type = callee_type->as.function.param_types[i];

        /* If argument is a lambda with missing types, infer from parameter type */
        if (arg_expr != NULL && arg_expr->type == EXPR_LAMBDA &&
            param_type != NULL && param_type->kind == TYPE_FUNCTION)
        {
            LambdaExpr *lambda = &arg_expr->as.lambda;
            Type *func_type = param_type;

            /* Check parameter count matches */
            if (lambda->param_count == func_type->as.function.param_count)
            {
                /* Infer missing parameter types */
                for (int j = 0; j < lambda->param_count; j++)
                {
                    if (lambda->params[j].type == NULL)
                    {
                        lambda->params[j].type = func_type->as.function.param_types[j];
                        DEBUG_VERBOSE("Inferred call argument lambda param %d type", j);
                    }
                }

                /* Infer missing return type */
                if (lambda->return_type == NULL)
                {
                    lambda->return_type = func_type->as.function.return_type;
                    DEBUG_VERBOSE("Inferred call argument lambda return type");
                }
            }
        }

        Type *arg_type = type_check_expr(arg_expr, table);
        if (arg_type == NULL)
        {
            type_error(expr->token, "Invalid argument in function call");
            return NULL;
        }
        if (param_type->kind == TYPE_ANY)
        {
            if (!is_printable_type(arg_type))
            {
                type_error(expr->token, "Unsupported type for built-in function");
                return NULL;
            }
        }
        else
        {
            if (!ast_type_equals(arg_type, param_type))
            {
                argument_type_error(expr->token, func_name, i, param_type, arg_type);
                return NULL;
            }
        }
    }

    /* Type check variadic arguments - must be primitives, str, or pointers (not arrays) */
    if (is_variadic)
    {
        for (int i = expected_params; i < expr->as.call.arg_count; i++)
        {
            Expr *arg_expr = expr->as.call.arguments[i];
            Type *arg_type = type_check_expr(arg_expr, table);
            if (arg_type == NULL)
            {
                type_error(expr->token, "Invalid argument in function call");
                return NULL;
            }
            if (!is_variadic_compatible_type(arg_type))
            {
                char msg[256];
                snprintf(msg, sizeof(msg),
                         "Variadic argument %d has type '%s', but only primitives, str, and pointers are allowed",
                         i + 1, type_name(arg_type));
                type_error(expr->token, msg);
                return NULL;
            }
        }
    }

    DEBUG_VERBOSE("Returning function return type: %d", callee_type->as.function.return_type->kind);
    return callee_type->as.function.return_type;
}

/* ============================================================================
 * Static Method Call Type Checking
 * ============================================================================
 * Handles type checking for static method calls like Stdin.readLine(),
 * Stdout.write(), Interceptor.register(), and user-defined struct static methods.
 * ============================================================================ */

Type *type_check_static_method_call(Expr *expr, SymbolTable *table)
{
    StaticCallExpr *call = &expr->as.static_call;
    Token type_name = call->type_name;
    Token method_name = call->method_name;

    /* Type check all arguments first */
    for (int i = 0; i < call->arg_count; i++)
    {
        Type *arg_type = type_check_expr(call->arguments[i], table);
        if (arg_type == NULL)
        {
            return NULL;
        }
    }

    /* Stdin static methods - console input */
    if (token_equals(type_name, "Stdin"))
    {
        if (token_equals(method_name, "readLine"))
        {
            /* Stdin.readLine(): str */
            if (call->arg_count != 0)
            {
                type_error(&method_name, "Stdin.readLine takes no arguments");
                return NULL;
            }
            return ast_create_primitive_type(table->arena, TYPE_STRING);
        }
        else if (token_equals(method_name, "readChar"))
        {
            /* Stdin.readChar(): int */
            if (call->arg_count != 0)
            {
                type_error(&method_name, "Stdin.readChar takes no arguments");
                return NULL;
            }
            return ast_create_primitive_type(table->arena, TYPE_INT);
        }
        else if (token_equals(method_name, "readWord"))
        {
            /* Stdin.readWord(): str */
            if (call->arg_count != 0)
            {
                type_error(&method_name, "Stdin.readWord takes no arguments");
                return NULL;
            }
            return ast_create_primitive_type(table->arena, TYPE_STRING);
        }
        else if (token_equals(method_name, "hasChars"))
        {
            /* Stdin.hasChars(): bool */
            if (call->arg_count != 0)
            {
                type_error(&method_name, "Stdin.hasChars takes no arguments");
                return NULL;
            }
            return ast_create_primitive_type(table->arena, TYPE_BOOL);
        }
        else if (token_equals(method_name, "hasLines"))
        {
            /* Stdin.hasLines(): bool */
            if (call->arg_count != 0)
            {
                type_error(&method_name, "Stdin.hasLines takes no arguments");
                return NULL;
            }
            return ast_create_primitive_type(table->arena, TYPE_BOOL);
        }
        else if (token_equals(method_name, "isEof"))
        {
            /* Stdin.isEof(): bool */
            if (call->arg_count != 0)
            {
                type_error(&method_name, "Stdin.isEof takes no arguments");
                return NULL;
            }
            return ast_create_primitive_type(table->arena, TYPE_BOOL);
        }
        else
        {
            char msg[128];
            snprintf(msg, sizeof(msg), "Unknown Stdin method '%.*s'",
                     method_name.length, method_name.start);
            type_error(&method_name, msg);
            return NULL;
        }
    }

    /* Stdout static methods - console output */
    if (token_equals(type_name, "Stdout"))
    {
        if (token_equals(method_name, "write"))
        {
            /* Stdout.write(text: str): void */
            if (call->arg_count != 1)
            {
                type_error(&method_name, "Stdout.write requires exactly 1 argument");
                return NULL;
            }
            Type *arg_type = call->arguments[0]->expr_type;
            if (arg_type == NULL || arg_type->kind != TYPE_STRING)
            {
                type_error(&method_name, "Stdout.write requires a string argument");
                return NULL;
            }
            return ast_create_primitive_type(table->arena, TYPE_VOID);
        }
        else if (token_equals(method_name, "writeLine"))
        {
            /* Stdout.writeLine(text: str): void */
            if (call->arg_count != 1)
            {
                type_error(&method_name, "Stdout.writeLine requires exactly 1 argument");
                return NULL;
            }
            Type *arg_type = call->arguments[0]->expr_type;
            if (arg_type == NULL || arg_type->kind != TYPE_STRING)
            {
                type_error(&method_name, "Stdout.writeLine requires a string argument");
                return NULL;
            }
            return ast_create_primitive_type(table->arena, TYPE_VOID);
        }
        else if (token_equals(method_name, "flush"))
        {
            /* Stdout.flush(): void */
            if (call->arg_count != 0)
            {
                type_error(&method_name, "Stdout.flush takes no arguments");
                return NULL;
            }
            return ast_create_primitive_type(table->arena, TYPE_VOID);
        }
        else
        {
            char msg[128];
            snprintf(msg, sizeof(msg), "Unknown Stdout method '%.*s'",
                     method_name.length, method_name.start);
            type_error(&method_name, msg);
            return NULL;
        }
    }

    /* Stderr static methods - error output */
    if (token_equals(type_name, "Stderr"))
    {
        if (token_equals(method_name, "write"))
        {
            /* Stderr.write(text: str): void */
            if (call->arg_count != 1)
            {
                type_error(&method_name, "Stderr.write requires exactly 1 argument");
                return NULL;
            }
            Type *arg_type = call->arguments[0]->expr_type;
            if (arg_type == NULL || arg_type->kind != TYPE_STRING)
            {
                type_error(&method_name, "Stderr.write requires a string argument");
                return NULL;
            }
            return ast_create_primitive_type(table->arena, TYPE_VOID);
        }
        else if (token_equals(method_name, "writeLine"))
        {
            /* Stderr.writeLine(text: str): void */
            if (call->arg_count != 1)
            {
                type_error(&method_name, "Stderr.writeLine requires exactly 1 argument");
                return NULL;
            }
            Type *arg_type = call->arguments[0]->expr_type;
            if (arg_type == NULL || arg_type->kind != TYPE_STRING)
            {
                type_error(&method_name, "Stderr.writeLine requires a string argument");
                return NULL;
            }
            return ast_create_primitive_type(table->arena, TYPE_VOID);
        }
        else if (token_equals(method_name, "flush"))
        {
            /* Stderr.flush(): void */
            if (call->arg_count != 0)
            {
                type_error(&method_name, "Stderr.flush takes no arguments");
                return NULL;
            }
            return ast_create_primitive_type(table->arena, TYPE_VOID);
        }
        else
        {
            char msg[128];
            snprintf(msg, sizeof(msg), "Unknown Stderr method '%.*s'",
                     method_name.length, method_name.start);
            type_error(&method_name, msg);
            return NULL;
        }
    }

    /* Interceptor static methods - function interception for debugging/mocking */
    if (token_equals(type_name, "Interceptor"))
    {
        if (token_equals(method_name, "register"))
        {
            /* Interceptor.register(handler: fn(str, any[], fn(): any): any): void */
            if (call->arg_count != 1)
            {
                type_error(&method_name, "Interceptor.register requires exactly 1 argument (handler function)");
                return NULL;
            }
            Type *handler_type = call->arguments[0]->expr_type;
            if (handler_type == NULL || handler_type->kind != TYPE_FUNCTION)
            {
                type_error(&method_name, "Interceptor.register requires a function argument");
                return NULL;
            }
            /* Validate handler signature is fn(str, any[], fn(): any): any */

            /* Check parameter count */
            if (handler_type->as.function.param_count != 3)
            {
                type_error(&method_name, "Interceptor handler must have 3 parameters: (name: str, args: any[], continue_fn: fn(): any)");
                return NULL;
            }

            /* Check param 0 is str */
            if (handler_type->as.function.param_types[0] == NULL ||
                handler_type->as.function.param_types[0]->kind != TYPE_STRING)
            {
                type_error(&method_name, "Interceptor handler first parameter must be 'str' (function name)");
                return NULL;
            }

            /* Check param 1 is any[] */
            if (handler_type->as.function.param_types[1] == NULL ||
                handler_type->as.function.param_types[1]->kind != TYPE_ARRAY ||
                handler_type->as.function.param_types[1]->as.array.element_type == NULL ||
                handler_type->as.function.param_types[1]->as.array.element_type->kind != TYPE_ANY)
            {
                type_error(&method_name, "Interceptor handler second parameter must be 'any[]' (arguments)");
                return NULL;
            }

            /* Check param 2 is fn(): any */
            Type *continue_param = handler_type->as.function.param_types[2];
            if (continue_param == NULL || continue_param->kind != TYPE_FUNCTION)
            {
                type_error(&method_name, "Interceptor handler third parameter must be 'fn(): any' (continue function)");
                return NULL;
            }
            if (continue_param->as.function.param_count != 0 ||
                continue_param->as.function.return_type == NULL ||
                continue_param->as.function.return_type->kind != TYPE_ANY)
            {
                type_error(&method_name, "Interceptor handler third parameter must be 'fn(): any' (continue function)");
                return NULL;
            }

            /* Check return type is any */
            if (handler_type->as.function.return_type == NULL ||
                handler_type->as.function.return_type->kind != TYPE_ANY)
            {
                type_error(&method_name, "Interceptor handler must return 'any'");
                return NULL;
            }

            return ast_create_primitive_type(table->arena, TYPE_VOID);
        }
        else if (token_equals(method_name, "registerWhere"))
        {
            /* Interceptor.registerWhere(handler: fn(str, any[], fn(): any): any, pattern: str): void */
            if (call->arg_count != 2)
            {
                type_error(&method_name, "Interceptor.registerWhere requires exactly 2 arguments (handler, pattern)");
                return NULL;
            }
            Type *handler_type = call->arguments[0]->expr_type;
            Type *pattern_type = call->arguments[1]->expr_type;
            if (handler_type == NULL || handler_type->kind != TYPE_FUNCTION)
            {
                type_error(&method_name, "Interceptor.registerWhere first argument must be a function");
                return NULL;
            }
            if (pattern_type == NULL || pattern_type->kind != TYPE_STRING)
            {
                type_error(&method_name, "Interceptor.registerWhere second argument must be a pattern string");
                return NULL;
            }

            /* Validate handler signature is fn(str, any[], fn(): any): any */

            /* Check parameter count */
            if (handler_type->as.function.param_count != 3)
            {
                type_error(&method_name, "Interceptor handler must have 3 parameters: (name: str, args: any[], continue_fn: fn(): any)");
                return NULL;
            }

            /* Check param 0 is str */
            if (handler_type->as.function.param_types[0] == NULL ||
                handler_type->as.function.param_types[0]->kind != TYPE_STRING)
            {
                type_error(&method_name, "Interceptor handler first parameter must be 'str' (function name)");
                return NULL;
            }

            /* Check param 1 is any[] */
            if (handler_type->as.function.param_types[1] == NULL ||
                handler_type->as.function.param_types[1]->kind != TYPE_ARRAY ||
                handler_type->as.function.param_types[1]->as.array.element_type == NULL ||
                handler_type->as.function.param_types[1]->as.array.element_type->kind != TYPE_ANY)
            {
                type_error(&method_name, "Interceptor handler second parameter must be 'any[]' (arguments)");
                return NULL;
            }

            /* Check param 2 is fn(): any */
            Type *continue_param = handler_type->as.function.param_types[2];
            if (continue_param == NULL || continue_param->kind != TYPE_FUNCTION)
            {
                type_error(&method_name, "Interceptor handler third parameter must be 'fn(): any' (continue function)");
                return NULL;
            }
            if (continue_param->as.function.param_count != 0 ||
                continue_param->as.function.return_type == NULL ||
                continue_param->as.function.return_type->kind != TYPE_ANY)
            {
                type_error(&method_name, "Interceptor handler third parameter must be 'fn(): any' (continue function)");
                return NULL;
            }

            /* Check return type is any */
            if (handler_type->as.function.return_type == NULL ||
                handler_type->as.function.return_type->kind != TYPE_ANY)
            {
                type_error(&method_name, "Interceptor handler must return 'any'");
                return NULL;
            }

            return ast_create_primitive_type(table->arena, TYPE_VOID);
        }
        else if (token_equals(method_name, "clearAll"))
        {
            /* Interceptor.clearAll(): void */
            if (call->arg_count != 0)
            {
                type_error(&method_name, "Interceptor.clearAll takes no arguments");
                return NULL;
            }
            return ast_create_primitive_type(table->arena, TYPE_VOID);
        }
        else if (token_equals(method_name, "isActive"))
        {
            /* Interceptor.isActive(): bool */
            if (call->arg_count != 0)
            {
                type_error(&method_name, "Interceptor.isActive takes no arguments");
                return NULL;
            }
            return ast_create_primitive_type(table->arena, TYPE_BOOL);
        }
        else if (token_equals(method_name, "count"))
        {
            /* Interceptor.count(): int */
            if (call->arg_count != 0)
            {
                type_error(&method_name, "Interceptor.count takes no arguments");
                return NULL;
            }
            return ast_create_primitive_type(table->arena, TYPE_INT);
        }
        else
        {
            char msg[128];
            snprintf(msg, sizeof(msg), "Unknown Interceptor static method '%.*s'",
                     method_name.length, method_name.start);
            type_error(&method_name, msg);
            return NULL;
        }
    }

    /* Check for user-defined struct static methods */
    {
        /* Create a token for looking up the struct type */
        Token lookup_tok;
        lookup_tok.start = type_name.start;
        lookup_tok.length = type_name.length;
        lookup_tok.line = type_name.line;
        lookup_tok.filename = type_name.filename;
        lookup_tok.type = TOKEN_IDENTIFIER;

        Symbol *struct_sym = symbol_table_lookup_type(table, lookup_tok);
        if (struct_sym != NULL && struct_sym->type != NULL &&
            struct_sym->type->kind == TYPE_STRUCT)
        {
            Type *struct_type = struct_sym->type;

            /* Look for a static method with matching name */
            for (int i = 0; i < struct_type->as.struct_type.method_count; i++)
            {
                StructMethod *method = &struct_type->as.struct_type.methods[i];
                if (method->is_static &&
                    method_name.length == (int)strlen(method->name) &&
                    strncmp(method_name.start, method->name, method_name.length) == 0)
                {
                    /* Found static method - validate argument count */
                    if (call->arg_count != method->param_count)
                    {
                        char msg[256];
                        snprintf(msg, sizeof(msg),
                                 "%s.%s expects %d argument(s), got %d",
                                 struct_type->as.struct_type.name,
                                 method->name, method->param_count, call->arg_count);
                        type_error(&method_name, msg);
                        return NULL;
                    }

                    /* Validate argument types */
                    for (int j = 0; j < call->arg_count; j++)
                    {
                        Type *arg_type = call->arguments[j]->expr_type;
                        Type *param_type = method->params[j].type;
                        if (!ast_type_equals(arg_type, param_type))
                        {
                            char msg[256];
                            snprintf(msg, sizeof(msg),
                                     "%s.%s argument %d: type mismatch",
                                     struct_type->as.struct_type.name,
                                     method->name, j + 1);
                            type_error(&method_name, msg);
                            return NULL;
                        }
                    }

                    /* Store resolved method for code generation */
                    call->resolved_method = method;
                    call->resolved_struct_type = struct_type;

                    expr->expr_type = method->return_type;
                    return method->return_type;
                }
            }

            /* Check if they tried to call an instance method statically */
            for (int i = 0; i < struct_type->as.struct_type.method_count; i++)
            {
                StructMethod *method = &struct_type->as.struct_type.methods[i];
                if (!method->is_static &&
                    method_name.length == (int)strlen(method->name) &&
                    strncmp(method_name.start, method->name, method_name.length) == 0)
                {
                    char msg[256];
                    snprintf(msg, sizeof(msg),
                             "Cannot call instance method '%s' on type '%s'. "
                             "Use an instance: var obj: %s = ...; obj.%s(...)",
                             method->name, struct_type->as.struct_type.name,
                             struct_type->as.struct_type.name, method->name);
                    type_error(&method_name, msg);
                    return NULL;
                }
            }

            /* No matching method found */
            char msg[256];
            snprintf(msg, sizeof(msg),
                     "No static method '%.*s' found in struct '%s'",
                     method_name.length, method_name.start,
                     struct_type->as.struct_type.name);
            type_error(&method_name, msg);
            return NULL;
        }
    }

    type_error(&type_name, "Unknown static type");
    return NULL;
}
