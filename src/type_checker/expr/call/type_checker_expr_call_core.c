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

#include "type_checker/expr/call/type_checker_expr_call_core.h"
#include "type_checker/expr/call/type_checker_expr_call.h"
#include "type_checker/expr/call/type_checker_expr_call_array.h"
#include "type_checker/expr/call/type_checker_expr_call_string.h"
#include "type_checker/expr/type_checker_expr.h"
#include "type_checker/util/type_checker_util.h"
#include "type_checker/stmt/type_checker_stmt_func.h"
#include "type_checker/stmt/type_checker_stmt_interface.h"
#include "type_checker/type_checker_generics.h"
#include "symbol_table/symbol_table_core.h"
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

    /* Generic function call: if the callee is a plain variable name that matches a
     * registered generic function template, perform type inference and monomorphize. */
    if (callee->type == EXPR_VARIABLE)
    {
        Token call_name_tok = callee->as.variable.name;
        char call_name[256];
        int call_name_len = call_name_tok.length < 255 ? call_name_tok.length : 255;
        memcpy(call_name, call_name_tok.start, call_name_len);
        call_name[call_name_len] = '\0';

        GenericFunctionTemplate *fn_tmpl = generic_registry_find_function_template(call_name);
        if (fn_tmpl != NULL)
        {
            /* Type-check all arguments first to get their concrete types */
            int arg_count = expr->as.call.arg_count;
            Type **arg_types = arena_alloc(table->arena, sizeof(Type *) * (arg_count + 1));
            for (int i = 0; i < arg_count; i++)
            {
                arg_types[i] = type_check_expr(expr->as.call.arguments[i], table);
                if (arg_types[i] == NULL)
                {
                    type_error(expr->token, "Invalid argument in generic function call");
                    return NULL;
                }
            }

            /* Infer type parameters */
            int tp_count = fn_tmpl->decl->type_param_count;
            Type **inferred = arena_alloc(table->arena, sizeof(Type *) * tp_count);
            if (!infer_type_params_from_args(fn_tmpl->decl, arg_types, arg_count, inferred))
            {
                char msg[256];
                snprintf(msg, sizeof(msg),
                         "Cannot infer type parameters for generic function '%s'", call_name);
                type_error(expr->token, msg);
                return NULL;
            }

            /* Check type parameter constraints */
            if (!check_type_param_constraints(call_name,
                                               (const char **)fn_tmpl->decl->type_params,
                                               fn_tmpl->decl->type_param_constraints,
                                               fn_tmpl->decl->type_param_constraint_counts,
                                               inferred, tp_count))
            {
                return NULL; /* error already reported */
            }

            /* Check instantiation cache */
            GenericFunctionInstantiation *fn_cached =
                generic_registry_find_function_instantiation(call_name, inferred, tp_count);

            FunctionStmt *mono_fn;
            if (fn_cached != NULL)
            {
                mono_fn = fn_cached->instantiated_decl;
            }
            else
            {
                /* Monomorphize */
                mono_fn = monomorphize_function(table->arena, fn_tmpl, inferred, tp_count);
                if (mono_fn == NULL)
                {
                    type_error(expr->token, "Failed to monomorphize generic function");
                    return NULL;
                }

                /* Cache — use an arena-duped name so the pointer remains valid.
                 * call_name is a stack buffer; storing a raw pointer to it would
                 * leave a dangling pointer once this stack frame is reused. */
                const char *call_name_dup = arena_strdup(table->arena, call_name);
                Type **inferred_copy = arena_alloc(table->arena, sizeof(Type *) * tp_count);
                for (int i = 0; i < tp_count; i++)
                    inferred_copy[i] = inferred[i];
                generic_registry_add_function_instantiation(call_name_dup, inferred_copy,
                                                             tp_count, mono_fn);

                /* Clear cached expr_type on all shared body expression nodes.
                 * The body Stmt/Expr nodes are shared across all instantiations of this
                 * template.  type_check_expr() caches results in expr->expr_type, so
                 * without clearing, the second instantiation (e.g., identity<str>) would
                 * see the cached type from the first (identity<int>) and emit false errors. */
                clear_expr_types_in_stmts(mono_fn->body, mono_fn->body_count);

                /* Type-check the monomorphized function body.
                 * Register type params as type aliases so that body statements that
                 * explicitly reference T (e.g. var x: T = ...) resolve correctly. */
                for (int t = 0; t < tp_count; t++)
                {
                    Token tp_tok;
                    tp_tok.start    = fn_tmpl->decl->type_params[t];
                    tp_tok.length   = (int)strlen(fn_tmpl->decl->type_params[t]);
                    tp_tok.type     = TOKEN_IDENTIFIER;
                    tp_tok.line     = 0;
                    tp_tok.filename = NULL;
                    symbol_table_add_type(table, tp_tok, inferred[t]);
                }

                Stmt wrapper;
                memset(&wrapper, 0, sizeof(Stmt));
                wrapper.type         = STMT_FUNCTION;
                wrapper.token        = &mono_fn->name;
                wrapper.as.function  = *mono_fn;
                type_check_function(&wrapper, table);
            }

            /* Return the concrete return type (with substitution applied) */
            Type *ret = mono_fn->return_type;

            /* Validate argument count vs monomorphized param count */
            if (arg_count != mono_fn->param_count)
            {
                char msg[256];
                snprintf(msg, sizeof(msg),
                         "Generic function '%s' expects %d arguments, got %d",
                         call_name, mono_fn->param_count, arg_count);
                type_error(expr->token, msg);
                return NULL;
            }

            /* Validate argument types against substituted param types */
            for (int i = 0; i < arg_count; i++)
            {
                Type *param_type = mono_fn->params[i].type;
                if (param_type != NULL && !ast_type_equals(arg_types[i], param_type))
                {
                    char msg[256];
                    snprintf(msg, sizeof(msg),
                             "Argument %d type mismatch in generic function call '%s'",
                             i + 1, call_name);
                    type_error(expr->token, msg);
                    return NULL;
                }
            }

            /* Rewrite the callee variable name to the monomorphized name so codegen
             * emits the correct C function name (e.g., "identity_int" instead of "identity"). */
            {
                const char *mono_fn_name = mono_fn->name.start;
                char *name_copy = arena_strdup(table->arena, mono_fn_name);
                callee->as.variable.name.start  = name_copy;
                callee->as.variable.name.length = (int)strlen(name_copy);
            }

            /* Set the callee expression type to the monomorphized function type so codegen
             * can emit correct type information. */
            {
                Type **param_types_copy = arena_alloc(table->arena,
                    sizeof(Type *) * mono_fn->param_count);
                for (int i = 0; i < mono_fn->param_count; i++)
                    param_types_copy[i] = mono_fn->params[i].type;
                Type *fn_type = ast_create_function_type(table->arena, ret,
                    param_types_copy, mono_fn->param_count);
                callee->expr_type = fn_type;
            }

            return ret;
        }
    }

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

    /* Special case: string.split() accepts 1 or 2 arguments */
    bool is_string_split = false;
    if (expr->as.call.callee->type == EXPR_MEMBER)
    {
        Expr *member = expr->as.call.callee;
        Type *object_type = member->as.member.object->expr_type;
        if (object_type && object_type->kind == TYPE_STRING)
        {
            Token member_name = member->as.member.member_name;
            if (member_name.length == 5 && strncmp(member_name.start, "split", 5) == 0)
            {
                is_string_split = true;
            }
        }
    }

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
    else if (is_string_split)
    {
        /* string.split accepts 1 or 2 arguments */
        if (expr->as.call.arg_count < 1 || expr->as.call.arg_count > 2)
        {
            char msg[256];
            snprintf(msg, sizeof(msg), "string.split() expects 1 or 2 arguments, got %d",
                     expr->as.call.arg_count);
            type_error(expr->token, msg);
            return NULL;
        }
        /* Type check first argument (delimiter: str) */
        Type *arg1_type = type_check_expr(expr->as.call.arguments[0], table);
        if (arg1_type == NULL || arg1_type->kind != TYPE_STRING)
        {
            type_error(expr->token, "split() first argument must be a string");
            return NULL;
        }
        /* Type check optional second argument (limit: int) */
        if (expr->as.call.arg_count == 2)
        {
            Type *arg2_type = type_check_expr(expr->as.call.arguments[1], table);
            if (arg2_type == NULL || arg2_type->kind != TYPE_INT)
            {
                type_error(expr->token, "split() second argument (limit) must be an int");
                return NULL;
            }
        }
        /* Return the string array type */
        return callee_type->as.function.return_type;
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
        if (param_type != NULL && param_type->kind == TYPE_INTERFACE)
        {
            /* Interface parameter: check structural satisfaction */
            const char *missing = NULL;
            const char *reason = NULL;
            if (arg_type == NULL || arg_type->kind != TYPE_STRUCT ||
                !struct_satisfies_interface(arg_type, param_type, &missing, &reason))
            {
                char msg[512];
                const char *iface_name = param_type->as.interface_type.name != NULL
                    ? param_type->as.interface_type.name : "interface";
                const char *actual_name = "unknown";
                if (arg_type != NULL)
                {
                    if (arg_type->kind == TYPE_STRUCT && arg_type->as.struct_type.name != NULL)
                        actual_name = arg_type->as.struct_type.name;
                    else
                        actual_name = type_name(arg_type);
                }
                if (missing != NULL)
                {
                    snprintf(msg, sizeof(msg),
                             "'%s' does not implement interface '%s': missing method '%s'",
                             actual_name, iface_name, missing);
                }
                else if (reason != NULL)
                {
                    snprintf(msg, sizeof(msg),
                             "'%s' does not implement interface '%s': %s",
                             actual_name, iface_name, reason);
                }
                else
                {
                    snprintf(msg, sizeof(msg),
                             "'%s' does not implement interface '%s'",
                             actual_name, iface_name);
                }
                type_error(expr->token, msg);
                return NULL;
            }
        }
        else if (!ast_type_equals(arg_type, param_type))
        {
            argument_type_error(expr->token, func_name, i, param_type, arg_type);
            return NULL;
        }

        /* Check that 'as ref' parameters receive an lvalue (variable), not a literal or expression result */
        MemoryQualifier *param_quals = callee_type->as.function.param_mem_quals;
        if (param_quals != NULL && param_quals[i] == MEM_AS_REF)
        {
            if (arg_expr->type != EXPR_VARIABLE && arg_expr->type != EXPR_MEMBER_ACCESS)
            {
                type_error(arg_expr->token,
                    "'as ref' parameter requires a variable or field, not a literal or expression");
                return NULL;
            }
        }

        /* Warn when a composite val-struct is passed by default (borrowed).
         * The callee gets a shallow view — mutating heap fields (str, arr)
         * would free the caller's data. Use 'as val' for ownership transfer. */
        {
            MemoryQualifier mq = MEM_DEFAULT;
            if (param_quals != NULL) mq = param_quals[i];
            if (mq == MEM_DEFAULT && !callee_type->as.function.is_native &&
                param_type && param_type->kind == TYPE_STRUCT &&
                !param_type->as.struct_type.pass_self_by_ref &&
                type_has_heap_fields(param_type))
            {
                char msg[256];
                snprintf(msg, sizeof(msg),
                    "composite struct '%s' is borrowed by default — "
                    "use 'as val' if the callee mutates heap fields (str/arr)",
                    param_type->as.struct_type.name);
                type_warning(arg_expr->token, msg);
            }
        }
    }

    /* Type check variadic arguments - must be primitives, str, or pointers (not arrays).
     * Exempt builtin print/println which handle arrays natively. */
    bool is_builtin_print = (strcmp(func_name, "print") == 0 || strcmp(func_name, "println") == 0 ||
                             strcmp(func_name, "printErr") == 0 || strcmp(func_name, "printErrLn") == 0);
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
            if (!is_builtin_print && !is_variadic_compatible_type(arg_type))
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

    /* Detect thread spawn pushed into array: arr.push(&fn())
     * If a spawn expression is pushed into an array variable, mark the array
     * symbol as having pending elements for thread sync support. */
    if (callee->type == EXPR_MEMBER &&
        expr->as.call.arg_count == 1 &&
        expr->as.call.arguments[0] != NULL &&
        expr->as.call.arguments[0]->type == EXPR_THREAD_SPAWN)
    {
        Expr *member_expr = callee;
        Token member_name = member_expr->as.member.member_name;
        if (member_name.length == 4 && strncmp(member_name.start, "push", 4) == 0)
        {
            Expr *object = member_expr->as.member.object;
            if (object->type == EXPR_VARIABLE)
            {
                Symbol *arr_sym = symbol_table_lookup_symbol(table, object->as.variable.name);
                if (arr_sym != NULL && arr_sym->type != NULL && arr_sym->type->kind == TYPE_ARRAY)
                {
                    arr_sym->has_pending_elements = true;
                    /* Propagate flag to VarDeclStmt so codegen can emit companion variable */
                    if (arr_sym->var_decl_origin != NULL)
                    {
                        arr_sym->var_decl_origin->as.var_decl.has_pending_elements = true;
                    }
                }
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
 * Stdout.write(), and user-defined struct static methods.
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

    /* Check for user-defined struct static methods */
    {
        Type *struct_type = NULL;

        /* If this is a generic static call (e.g., Stack<int>.new()),
         * resolve the generic instantiation to get the monomorphized type. */
        if (call->generic_type != NULL &&
            call->generic_type->kind == TYPE_GENERIC_INST)
        {
            Type *concrete = resolve_generic_instantiation(table->arena,
                                                            call->generic_type, table);
            if (concrete != NULL && concrete->kind == TYPE_STRUCT)
            {
                struct_type = concrete;
            }
        }

        /* Non-generic path: look up struct type by name */
        if (struct_type == NULL)
        {
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
                struct_type = struct_sym->type;
            }
        }

        if (struct_type != NULL)
        {

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
