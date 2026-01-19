#include "code_gen/code_gen_expr_lambda.h"
#include "code_gen.h"
#include "code_gen/code_gen_expr.h"
#include "code_gen/code_gen_util.h"
#include "type_checker/type_checker_util.h"
#include "debug.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "../platform/compat_io.h"

/* is_primitive_type is defined in type_checker_util.c */

/* Check if a type needs to be captured by reference (pointer indirection).
 * This includes:
 * - Primitive types (int, long, etc.) - because they can be reassigned
 * - Array types - because push/pop operations return new pointers
 * This ensures modifications inside closures persist to the original variable. */
static bool needs_capture_by_ref(Type *type)
{
    if (type == NULL) return false;
    if (is_primitive_type(type)) return true;
    if (type->kind == TYPE_ARRAY) return true;
    return false;
}

/* Initialize LocalVars structure */
void local_vars_init(LocalVars *lv)
{
    lv->names = NULL;
    lv->count = 0;
    lv->capacity = 0;
}

/* Add a local variable name (de-duplicated) */
void local_vars_add(LocalVars *lv, Arena *arena, const char *name)
{
    /* Check if already tracked */
    for (int i = 0; i < lv->count; i++)
    {
        if (strcmp(lv->names[i], name) == 0)
            return;
    }
    /* Grow array if needed */
    if (lv->count >= lv->capacity)
    {
        int new_cap = lv->capacity == 0 ? 8 : lv->capacity * 2;
        char **new_names = arena_alloc(arena, new_cap * sizeof(char *));
        for (int i = 0; i < lv->count; i++)
        {
            new_names[i] = lv->names[i];
        }
        lv->names = new_names;
        lv->capacity = new_cap;
    }
    lv->names[lv->count] = arena_strdup(arena, name);
    lv->count++;
}

/* Collect local variable declarations from a statement */
void collect_local_vars_from_stmt(Stmt *stmt, LocalVars *lv, Arena *arena)
{
    if (stmt == NULL) return;

    switch (stmt->type)
    {
    case STMT_VAR_DECL:
    {
        /* Add this variable to locals */
        char name[256];
        int len = stmt->as.var_decl.name.length < 255 ? stmt->as.var_decl.name.length : 255;
        strncpy(name, stmt->as.var_decl.name.start, len);
        name[len] = '\0';
        local_vars_add(lv, arena, name);
        break;
    }
    case STMT_BLOCK:
        for (int i = 0; i < stmt->as.block.count; i++)
            collect_local_vars_from_stmt(stmt->as.block.statements[i], lv, arena);
        break;
    case STMT_IF:
        collect_local_vars_from_stmt(stmt->as.if_stmt.then_branch, lv, arena);
        if (stmt->as.if_stmt.else_branch)
            collect_local_vars_from_stmt(stmt->as.if_stmt.else_branch, lv, arena);
        break;
    case STMT_WHILE:
        collect_local_vars_from_stmt(stmt->as.while_stmt.body, lv, arena);
        break;
    case STMT_FOR:
        if (stmt->as.for_stmt.initializer)
            collect_local_vars_from_stmt(stmt->as.for_stmt.initializer, lv, arena);
        collect_local_vars_from_stmt(stmt->as.for_stmt.body, lv, arena);
        break;
    case STMT_FOR_EACH:
    {
        /* The loop variable is a local */
        char name[256];
        int len = stmt->as.for_each_stmt.var_name.length < 255 ? stmt->as.for_each_stmt.var_name.length : 255;
        strncpy(name, stmt->as.for_each_stmt.var_name.start, len);
        name[len] = '\0';
        local_vars_add(lv, arena, name);
        collect_local_vars_from_stmt(stmt->as.for_each_stmt.body, lv, arena);
        break;
    }
    case STMT_LOCK:
        collect_local_vars_from_stmt(stmt->as.lock_stmt.body, lv, arena);
        break;
    default:
        break;
    }
}

/* Check if a name is a parameter of any enclosing lambda, and get its type */
Type *find_enclosing_lambda_param(EnclosingLambdaContext *ctx, const char *name)
{
    if (ctx == NULL) return NULL;
    for (int i = 0; i < ctx->count; i++)
    {
        LambdaExpr *lambda = ctx->lambdas[i];
        for (int j = 0; j < lambda->param_count; j++)
        {
            char param_name[256];
            int len = lambda->params[j].name.length < 255 ? lambda->params[j].name.length : 255;
            strncpy(param_name, lambda->params[j].name.start, len);
            param_name[len] = '\0';
            if (strcmp(param_name, name) == 0)
            {
                return lambda->params[j].type;
            }
        }
    }
    return NULL;
}

/* Helper to check if a name is a lambda parameter */
bool is_lambda_param(LambdaExpr *lambda, const char *name)
{
    for (int i = 0; i < lambda->param_count; i++)
    {
        char param_name[256];
        int len = lambda->params[i].name.length < 255 ? lambda->params[i].name.length : 255;
        strncpy(param_name, lambda->params[i].name.start, len);
        param_name[len] = '\0';
        if (strcmp(param_name, name) == 0)
            return true;
    }
    return false;
}

/* Check if a name is a local variable in the current lambda scope */
bool is_local_var(LocalVars *lv, const char *name)
{
    for (int i = 0; i < lv->count; i++)
    {
        if (strcmp(lv->names[i], name) == 0)
            return true;
    }
    return false;
}

/* Initialize captured vars structure */
void captured_vars_init(CapturedVars *cv)
{
    cv->names = NULL;
    cv->types = NULL;
    cv->count = 0;
    cv->capacity = 0;
}

/* Add a captured variable (de-duplicated) */
void captured_vars_add(CapturedVars *cv, Arena *arena, const char *name, Type *type)
{
    /* Check if already captured */
    for (int i = 0; i < cv->count; i++)
    {
        if (strcmp(cv->names[i], name) == 0)
            return;
    }
    /* Grow arrays if needed */
    if (cv->count >= cv->capacity)
    {
        int new_cap = cv->capacity == 0 ? 4 : cv->capacity * 2;
        char **new_names = arena_alloc(arena, new_cap * sizeof(char *));
        Type **new_types = arena_alloc(arena, new_cap * sizeof(Type *));
        for (int i = 0; i < cv->count; i++)
        {
            new_names[i] = cv->names[i];
            new_types[i] = cv->types[i];
        }
        cv->names = new_names;
        cv->types = new_types;
        cv->capacity = new_cap;
    }
    cv->names[cv->count] = arena_strdup(arena, name);
    cv->types[cv->count] = type;
    cv->count++;
}

/* Forward declaration for mutual recursion */
static void collect_captured_vars_internal(Expr *expr, LambdaExpr *lambda, SymbolTable *table,
                                           CapturedVars *cv, LocalVars *lv, EnclosingLambdaContext *enclosing, Arena *arena);

/* Recursively collect captured variables from a statement */
void collect_captured_vars_from_stmt(Stmt *stmt, LambdaExpr *lambda, SymbolTable *table,
                                     CapturedVars *cv, LocalVars *lv, EnclosingLambdaContext *enclosing, Arena *arena)
{
    if (stmt == NULL) return;

    switch (stmt->type)
    {
    case STMT_EXPR:
        collect_captured_vars_internal(stmt->as.expression.expression, lambda, table, cv, lv, enclosing, arena);
        break;
    case STMT_VAR_DECL:
        if (stmt->as.var_decl.initializer)
            collect_captured_vars_internal(stmt->as.var_decl.initializer, lambda, table, cv, lv, enclosing, arena);
        break;
    case STMT_RETURN:
        if (stmt->as.return_stmt.value)
            collect_captured_vars_internal(stmt->as.return_stmt.value, lambda, table, cv, lv, enclosing, arena);
        break;
    case STMT_BLOCK:
        for (int i = 0; i < stmt->as.block.count; i++)
            collect_captured_vars_from_stmt(stmt->as.block.statements[i], lambda, table, cv, lv, enclosing, arena);
        break;
    case STMT_IF:
        collect_captured_vars_internal(stmt->as.if_stmt.condition, lambda, table, cv, lv, enclosing, arena);
        collect_captured_vars_from_stmt(stmt->as.if_stmt.then_branch, lambda, table, cv, lv, enclosing, arena);
        if (stmt->as.if_stmt.else_branch)
            collect_captured_vars_from_stmt(stmt->as.if_stmt.else_branch, lambda, table, cv, lv, enclosing, arena);
        break;
    case STMT_WHILE:
        collect_captured_vars_internal(stmt->as.while_stmt.condition, lambda, table, cv, lv, enclosing, arena);
        collect_captured_vars_from_stmt(stmt->as.while_stmt.body, lambda, table, cv, lv, enclosing, arena);
        break;
    case STMT_FOR:
        if (stmt->as.for_stmt.initializer)
            collect_captured_vars_from_stmt(stmt->as.for_stmt.initializer, lambda, table, cv, lv, enclosing, arena);
        if (stmt->as.for_stmt.condition)
            collect_captured_vars_internal(stmt->as.for_stmt.condition, lambda, table, cv, lv, enclosing, arena);
        if (stmt->as.for_stmt.increment)
            collect_captured_vars_internal(stmt->as.for_stmt.increment, lambda, table, cv, lv, enclosing, arena);
        collect_captured_vars_from_stmt(stmt->as.for_stmt.body, lambda, table, cv, lv, enclosing, arena);
        break;
    case STMT_FOR_EACH:
        collect_captured_vars_internal(stmt->as.for_each_stmt.iterable, lambda, table, cv, lv, enclosing, arena);
        collect_captured_vars_from_stmt(stmt->as.for_each_stmt.body, lambda, table, cv, lv, enclosing, arena);
        break;
    case STMT_LOCK:
        collect_captured_vars_internal(stmt->as.lock_stmt.lock_expr, lambda, table, cv, lv, enclosing, arena);
        collect_captured_vars_from_stmt(stmt->as.lock_stmt.body, lambda, table, cv, lv, enclosing, arena);
        break;
    case STMT_FUNCTION:
        /* Don't recurse into nested functions - they have their own scope */
        break;
    case STMT_BREAK:
    case STMT_CONTINUE:
    case STMT_IMPORT:
    default:
        break;
    }
}

/* Internal implementation for mutual recursion */
static void collect_captured_vars_internal(Expr *expr, LambdaExpr *lambda, SymbolTable *table,
                                           CapturedVars *cv, LocalVars *lv, EnclosingLambdaContext *enclosing, Arena *arena)
{
    if (expr == NULL) return;

    switch (expr->type)
    {
    case EXPR_VARIABLE:
    {
        char name[256];
        int len = expr->as.variable.name.length < 255 ? expr->as.variable.name.length : 255;
        strncpy(name, expr->as.variable.name.start, len);
        name[len] = '\0';

        /* Skip if it's a lambda parameter */
        if (is_lambda_param(lambda, name))
            return;

        /* Skip if it's a local variable declared in the lambda body */
        if (lv != NULL && is_local_var(lv, name))
            return;

        /* Skip builtins */
        if (strcmp(name, "print") == 0 || strcmp(name, "len") == 0)
            return;

        /* Look up in symbol table to see if it's an outer variable */
        Symbol *sym = symbol_table_lookup_symbol(table, expr->as.variable.name);
        if (sym != NULL)
        {
            /* It's a captured variable from outer scope */
            captured_vars_add(cv, arena, name, sym->type);
        }
        else
        {
            /* Check if it's a parameter from an enclosing lambda */
            Type *enclosing_type = find_enclosing_lambda_param(enclosing, name);
            if (enclosing_type != NULL)
            {
                captured_vars_add(cv, arena, name, enclosing_type);
            }
        }
        break;
    }
    case EXPR_BINARY:
        collect_captured_vars_internal(expr->as.binary.left, lambda, table, cv, lv, enclosing, arena);
        collect_captured_vars_internal(expr->as.binary.right, lambda, table, cv, lv, enclosing, arena);
        break;
    case EXPR_UNARY:
        collect_captured_vars_internal(expr->as.unary.operand, lambda, table, cv, lv, enclosing, arena);
        break;
    case EXPR_ASSIGN:
        collect_captured_vars_internal(expr->as.assign.value, lambda, table, cv, lv, enclosing, arena);
        break;
    case EXPR_INDEX_ASSIGN:
        collect_captured_vars_internal(expr->as.index_assign.array, lambda, table, cv, lv, enclosing, arena);
        collect_captured_vars_internal(expr->as.index_assign.index, lambda, table, cv, lv, enclosing, arena);
        collect_captured_vars_internal(expr->as.index_assign.value, lambda, table, cv, lv, enclosing, arena);
        break;
    case EXPR_CALL:
        collect_captured_vars_internal(expr->as.call.callee, lambda, table, cv, lv, enclosing, arena);
        for (int i = 0; i < expr->as.call.arg_count; i++)
            collect_captured_vars_internal(expr->as.call.arguments[i], lambda, table, cv, lv, enclosing, arena);
        break;
    case EXPR_ARRAY:
        for (int i = 0; i < expr->as.array.element_count; i++)
            collect_captured_vars_internal(expr->as.array.elements[i], lambda, table, cv, lv, enclosing, arena);
        break;
    case EXPR_ARRAY_ACCESS:
        collect_captured_vars_internal(expr->as.array_access.array, lambda, table, cv, lv, enclosing, arena);
        collect_captured_vars_internal(expr->as.array_access.index, lambda, table, cv, lv, enclosing, arena);
        break;
    case EXPR_INCREMENT:
    case EXPR_DECREMENT:
        collect_captured_vars_internal(expr->as.operand, lambda, table, cv, lv, enclosing, arena);
        break;
    case EXPR_INTERPOLATED:
        for (int i = 0; i < expr->as.interpol.part_count; i++)
            collect_captured_vars_internal(expr->as.interpol.parts[i], lambda, table, cv, lv, enclosing, arena);
        break;
    case EXPR_MEMBER:
        collect_captured_vars_internal(expr->as.member.object, lambda, table, cv, lv, enclosing, arena);
        break;
    case EXPR_ARRAY_SLICE:
        collect_captured_vars_internal(expr->as.array_slice.array, lambda, table, cv, lv, enclosing, arena);
        collect_captured_vars_internal(expr->as.array_slice.start, lambda, table, cv, lv, enclosing, arena);
        collect_captured_vars_internal(expr->as.array_slice.end, lambda, table, cv, lv, enclosing, arena);
        collect_captured_vars_internal(expr->as.array_slice.step, lambda, table, cv, lv, enclosing, arena);
        break;
    case EXPR_RANGE:
        collect_captured_vars_internal(expr->as.range.start, lambda, table, cv, lv, enclosing, arena);
        collect_captured_vars_internal(expr->as.range.end, lambda, table, cv, lv, enclosing, arena);
        break;
    case EXPR_SPREAD:
        collect_captured_vars_internal(expr->as.spread.array, lambda, table, cv, lv, enclosing, arena);
        break;
    case EXPR_LAMBDA:
        /* Recurse into nested lambdas to collect transitive captures */
        /* Variables captured by nested lambdas that are from outer scopes
           need to be captured by this lambda too */
        {
            LambdaExpr *nested_lambda = &expr->as.lambda;
            if (nested_lambda->has_stmt_body)
            {
                for (int i = 0; i < nested_lambda->body_stmt_count; i++)
                {
                    collect_captured_vars_from_stmt(nested_lambda->body_stmts[i], lambda, table, cv, lv, enclosing, arena);
                }
            }
            else if (nested_lambda->body)
            {
                collect_captured_vars_internal(nested_lambda->body, lambda, table, cv, lv, enclosing, arena);
            }
        }
        break;
    case EXPR_STATIC_CALL:
        for (int i = 0; i < expr->as.static_call.arg_count; i++)
        {
            collect_captured_vars_internal(expr->as.static_call.arguments[i], lambda, table, cv, lv, enclosing, arena);
        }
        break;
    case EXPR_LITERAL:
    default:
        break;
    }
}

/* Public wrapper for collect_captured_vars */
void collect_captured_vars(Expr *expr, LambdaExpr *lambda, SymbolTable *table,
                           CapturedVars *cv, LocalVars *lv, EnclosingLambdaContext *enclosing, Arena *arena)
{
    collect_captured_vars_internal(expr, lambda, table, cv, lv, enclosing, arena);
}

/* Helper to generate statement body code for lambda
 * lambda_func_name: the generated function name like "__lambda_5__"
 * This sets up context so return statements work correctly */
char *code_gen_lambda_stmt_body(CodeGen *gen, LambdaExpr *lambda, int indent,
                                 const char *lambda_func_name, Type *return_type)
{
    /* Save current context */
    char *old_function = gen->current_function;
    Type *old_return_type = gen->current_return_type;

    /* Set up lambda context - use the lambda function name for return labels */
    gen->current_function = (char *)lambda_func_name;
    gen->current_return_type = return_type;

    /* Add lambda parameters to symbol table so they can be found during code gen.
     * This ensures function-type parameters are recognized as closure variables,
     * not as named functions. */
    symbol_table_push_scope(gen->symbol_table);
    for (int i = 0; i < lambda->param_count; i++)
    {
        symbol_table_add_symbol(gen->symbol_table, lambda->params[i].name, lambda->params[i].type);
    }

    /* Generate code for each statement in the lambda body */
    /* We need to capture the output since code_gen_statement writes to gen->output */
    FILE *old_output = gen->output;
    char *body_buffer = NULL;
    size_t body_size = 0;
    gen->output = open_memstream(&body_buffer, &body_size);

    for (int i = 0; i < lambda->body_stmt_count; i++)
    {
        code_gen_statement(gen, lambda->body_stmts[i], indent);
    }

    sn_fclose(gen->output);
    gen->output = old_output;

    /* Pop the lambda parameter scope */
    symbol_table_pop_scope(gen->symbol_table);

    /* Restore context */
    gen->current_function = old_function;
    gen->current_return_type = old_return_type;

    /* Copy to arena and free temp buffer */
    char *result = arena_strdup(gen->arena, body_buffer ? body_buffer : "");
    free(body_buffer);

    return result;
}

/* Generate code for a native lambda expression (C-compatible function pointer).
 * Native lambdas don't use closures - they're direct function pointers. */
static char *code_gen_native_lambda_expression(CodeGen *gen, Expr *expr)
{
    DEBUG_VERBOSE("Entering code_gen_native_lambda_expression");
    LambdaExpr *lambda = &expr->as.lambda;
    int lambda_id = gen->lambda_count++;

    /* Store the lambda_id in the expression for later reference */
    expr->as.lambda.lambda_id = lambda_id;

    /* Get C types for return type and parameters */
    const char *ret_c_type = get_c_type(gen->arena, lambda->return_type);

    /* Build parameter list string for the static function (no closure param) */
    char *params_decl = arena_strdup(gen->arena, "");
    for (int i = 0; i < lambda->param_count; i++)
    {
        const char *param_c_type = get_c_type(gen->arena, lambda->params[i].type);
        char *param_name = arena_strndup(gen->arena, lambda->params[i].name.start,
                                         lambda->params[i].name.length);
        if (i > 0)
        {
            params_decl = arena_sprintf(gen->arena, "%s, %s %s", params_decl, param_c_type, param_name);
        }
        else
        {
            params_decl = arena_sprintf(gen->arena, "%s %s", param_c_type, param_name);
        }
    }

    /* Handle empty parameter list */
    if (lambda->param_count == 0)
    {
        params_decl = arena_strdup(gen->arena, "void");
    }

    /* Generate the lambda function name */
    char *lambda_func_name = arena_sprintf(gen->arena, "__lambda_%d__", lambda_id);

    /* Generate forward declaration */
    gen->lambda_forward_decls = arena_sprintf(gen->arena, "%sstatic %s %s(%s);\n",
                                              gen->lambda_forward_decls,
                                              ret_c_type, lambda_func_name, params_decl);

    /* Generate the lambda function body */
    char *lambda_func;
    if (lambda->has_stmt_body)
    {
        /* Statement body lambda - use helper to generate body */
        char *body_code = code_gen_lambda_stmt_body(gen, lambda, 1, lambda_func_name, lambda->return_type);

        /* Generate the function with statement body */
        lambda_func = arena_sprintf(gen->arena,
            "static %s %s(%s) {\n"
            "    %s _return_value = %s;\n"
            "%s"
            "%s_return:\n"
            "    return _return_value;\n"
            "}\n\n",
            ret_c_type, lambda_func_name, params_decl,
            ret_c_type, get_default_value(lambda->return_type),
            body_code,
            lambda_func_name);
    }
    else
    {
        /* Expression body lambda */
        char *body_code = code_gen_expression(gen, lambda->body);

        /* Generate simple function body */
        lambda_func = arena_sprintf(gen->arena,
            "static %s %s(%s) {\n"
            "    return %s;\n"
            "}\n\n",
            ret_c_type, lambda_func_name, params_decl, body_code);
    }

    /* Append to definitions buffer */
    gen->lambda_definitions = arena_sprintf(gen->arena, "%s%s",
                                            gen->lambda_definitions, lambda_func);

    /* Return the function pointer directly (no closure) */
    return arena_sprintf(gen->arena, "__lambda_%d__", lambda_id);
}

/* Generate code for a lambda expression */
char *code_gen_lambda_expression(CodeGen *gen, Expr *expr)
{
    DEBUG_VERBOSE("Entering code_gen_lambda_expression");
    LambdaExpr *lambda = &expr->as.lambda;

    /* Native lambdas are generated differently - no closures, direct function pointers */
    if (lambda->is_native)
    {
        return code_gen_native_lambda_expression(gen, expr);
    }

    /* Add lambda parameters to symbol table so they can be found during code gen.
     * This ensures function-type parameters are recognized as closure variables,
     * not as named functions. We push a new scope and add params here,
     * then pop it at the end of this function. */
    symbol_table_push_scope(gen->symbol_table);
    for (int i = 0; i < lambda->param_count; i++)
    {
        symbol_table_add_symbol(gen->symbol_table, lambda->params[i].name, lambda->params[i].type);
    }

    int lambda_id = gen->lambda_count++;
    FunctionModifier modifier = lambda->modifier;

    /* Store the lambda_id in the expression for later reference */
    expr->as.lambda.lambda_id = lambda_id;

    /* Collect captured variables - from expression body or statement body */
    CapturedVars cv;
    captured_vars_init(&cv);

    /* First collect local variables declared in the lambda body */
    LocalVars lv;
    local_vars_init(&lv);
    if (lambda->has_stmt_body)
    {
        for (int i = 0; i < lambda->body_stmt_count; i++)
        {
            collect_local_vars_from_stmt(lambda->body_stmts[i], &lv, gen->arena);
        }
    }

    /* Build enclosing lambda context from CodeGen state */
    EnclosingLambdaContext enclosing;
    enclosing.lambdas = gen->enclosing_lambdas;
    enclosing.count = gen->enclosing_lambda_count;
    enclosing.capacity = gen->enclosing_lambda_capacity;

    /* Now collect captured variables, skipping locals */
    if (lambda->has_stmt_body)
    {
        for (int i = 0; i < lambda->body_stmt_count; i++)
        {
            collect_captured_vars_from_stmt(lambda->body_stmts[i], lambda, gen->symbol_table, &cv, &lv, &enclosing, gen->arena);
        }
    }
    else
    {
        collect_captured_vars(lambda->body, lambda, gen->symbol_table, &cv, NULL, &enclosing, gen->arena);
    }

    /* Get C types for return type and parameters */
    const char *ret_c_type = get_c_type(gen->arena, lambda->return_type);

    /* Build parameter list string for the static function */
    /* First param is always the closure pointer (void *) */
    char *params_decl = arena_strdup(gen->arena, "void *__closure__");

    for (int i = 0; i < lambda->param_count; i++)
    {
        const char *param_c_type = get_c_type(gen->arena, lambda->params[i].type);
        char *param_name = arena_strndup(gen->arena, lambda->params[i].name.start,
                                         lambda->params[i].name.length);
        params_decl = arena_sprintf(gen->arena, "%s, %s %s", params_decl, param_c_type, param_name);
    }

    /* Generate arena handling code based on modifier */
    char *arena_setup = arena_strdup(gen->arena, "");
    char *arena_cleanup = arena_strdup(gen->arena, "");

    if (modifier == FUNC_PRIVATE)
    {
        /* Private lambda: create isolated arena, destroy before return */
        arena_setup = arena_sprintf(gen->arena,
            "    RtArena *__lambda_arena__ = rt_arena_create(NULL);\n"
            "    (void)__closure__;\n");
        arena_cleanup = arena_sprintf(gen->arena,
            "    rt_arena_destroy(__lambda_arena__);\n");
    }
    else
    {
        /* Default/Shared lambda: use arena from closure */
        arena_setup = arena_sprintf(gen->arena,
            "    RtArena *__lambda_arena__ = ((__Closure__ *)__closure__)->arena;\n");
    }

    if (cv.count > 0)
    {
        /* Generate custom closure struct for this lambda (with arena and size fields) */
        char *struct_def = arena_sprintf(gen->arena,
            "typedef struct __closure_%d__ {\n"
            "    void *fn;\n"
            "    RtArena *arena;\n"
            "    size_t size;\n",
            lambda_id);
        for (int i = 0; i < cv.count; i++)
        {
            const char *c_type = get_c_type(gen->arena, cv.types[i]);
            /* For types that need capture by reference (primitives and arrays),
             * store pointers to enable mutation persistence.
             * When a lambda modifies a captured variable, the change must
             * persist to the original variable and across multiple calls.
             * Arrays need this because push/pop return new pointers. */
            if (needs_capture_by_ref(cv.types[i]))
            {
                struct_def = arena_sprintf(gen->arena, "%s    %s *%s;\n",
                                           struct_def, c_type, cv.names[i]);
            }
            else
            {
                struct_def = arena_sprintf(gen->arena, "%s    %s %s;\n",
                                           struct_def, c_type, cv.names[i]);
            }
        }
        struct_def = arena_sprintf(gen->arena, "%s} __closure_%d__;\n",
                                   struct_def, lambda_id);

        /* Add struct def to forward declarations (before lambda functions) */
        gen->lambda_forward_decls = arena_sprintf(gen->arena, "%s%s",
                                                  gen->lambda_forward_decls, struct_def);

        /* Generate local variable declarations for captured vars in lambda body.
         * For types needing capture by ref (primitives and arrays), we create a
         * pointer alias that points to the closure's stored pointer. This way,
         * reads/writes use the pointer and mutations persist both to the original
         * variable and across lambda calls.
         * For other types, we just copy the value. */
        char *capture_decls = arena_strdup(gen->arena, "");
        for (int i = 0; i < cv.count; i++)
        {
            const char *c_type = get_c_type(gen->arena, cv.types[i]);
            if (needs_capture_by_ref(cv.types[i]))
            {
                /* For types captured by ref, the closure stores a pointer. We declare
                 * a local pointer variable that references the closure's stored pointer,
                 * so access like (*count) or (*data) works naturally. We use a local
                 * variable instead of #define to avoid macro replacement issues when
                 * this lambda creates nested closures. */
                capture_decls = arena_sprintf(gen->arena,
                    "%s    %s *%s = ((__closure_%d__ *)__closure__)->%s;\n",
                    capture_decls, c_type, cv.names[i], lambda_id, cv.names[i]);
            }
            else
            {
                /* Other types: copy the value */
                capture_decls = arena_sprintf(gen->arena,
                    "%s    %s %s = ((__closure_%d__ *)__closure__)->%s;\n",
                    capture_decls, c_type, cv.names[i], lambda_id, cv.names[i]);
            }
        }

        /* Generate the static lambda function body - use lambda's arena */
        char *saved_arena_var = gen->current_arena_var;
        gen->current_arena_var = "__lambda_arena__";

        /* Push this lambda to enclosing context for nested lambdas */
        if (gen->enclosing_lambda_count >= gen->enclosing_lambda_capacity)
        {
            int new_cap = gen->enclosing_lambda_capacity == 0 ? 4 : gen->enclosing_lambda_capacity * 2;
            LambdaExpr **new_lambdas = arena_alloc(gen->arena, new_cap * sizeof(LambdaExpr *));
            for (int i = 0; i < gen->enclosing_lambda_count; i++)
            {
                new_lambdas[i] = gen->enclosing_lambdas[i];
            }
            gen->enclosing_lambdas = new_lambdas;
            gen->enclosing_lambda_capacity = new_cap;
        }
        gen->enclosing_lambdas[gen->enclosing_lambda_count++] = lambda;

        /* Generate forward declaration */
        char *forward_decl = arena_sprintf(gen->arena,
            "static %s __lambda_%d__(%s);\n",
            ret_c_type, lambda_id, params_decl);

        gen->lambda_forward_decls = arena_sprintf(gen->arena, "%s%s",
                                                  gen->lambda_forward_decls, forward_decl);

        /* Generate the actual lambda function definition with capture access */
        char *lambda_func;
        char *lambda_func_name = arena_sprintf(gen->arena, "__lambda_%d__", lambda_id);

        /* Add captured variables that need ref semantics to symbol table so they get
         * dereferenced when accessed. This includes primitives and arrays.
         * Push a new scope, add the captured variables with MEM_AS_REF, generate body, pop scope. */
        symbol_table_push_scope(gen->symbol_table);
        for (int i = 0; i < cv.count; i++)
        {
            if (needs_capture_by_ref(cv.types[i]))
            {
                /* Create a token from the string name */
                Token name_token;
                name_token.start = cv.names[i];
                name_token.length = strlen(cv.names[i]);
                name_token.line = 0;
                symbol_table_add_symbol_full(gen->symbol_table, name_token, cv.types[i], SYMBOL_LOCAL, MEM_AS_REF);
            }
        }

        if (lambda->has_stmt_body)
        {
            /* Multi-line lambda with statement body - needs return value and label */
            char *body_code = code_gen_lambda_stmt_body(gen, lambda, 1, lambda_func_name, lambda->return_type);

            /* Check if void return type - special handling needed */
            int is_void_return = (lambda->return_type && lambda->return_type->kind == TYPE_VOID);

            if (is_void_return)
            {
                /* Void return - no return value declaration needed */
                if (modifier == FUNC_PRIVATE)
                {
                    lambda_func = arena_sprintf(gen->arena,
                        "static void %s(%s) {\n"
                        "%s"
                        "%s"
                        "%s"
                        "%s_return:\n"
                        "%s"
                        "    return;\n"
                        "}\n\n",
                        lambda_func_name, params_decl, arena_setup, capture_decls,
                        body_code, lambda_func_name, arena_cleanup);
                }
                else
                {
                    lambda_func = arena_sprintf(gen->arena,
                        "static void %s(%s) {\n"
                        "%s"
                        "%s"
                        "%s"
                        "%s_return:\n"
                        "    return;\n"
                        "}\n\n",
                        lambda_func_name, params_decl, arena_setup, capture_decls,
                        body_code, lambda_func_name);
                }
            }
            else
            {
                const char *default_val = get_default_value(lambda->return_type);
                if (modifier == FUNC_PRIVATE)
                {
                    lambda_func = arena_sprintf(gen->arena,
                        "static %s %s(%s) {\n"
                        "%s"
                        "%s"
                        "    %s _return_value = %s;\n"
                        "%s"
                        "%s_return:\n"
                        "%s"
                        "    return _return_value;\n"
                        "}\n\n",
                        ret_c_type, lambda_func_name, params_decl, arena_setup, capture_decls,
                        ret_c_type, default_val, body_code, lambda_func_name, arena_cleanup);
                }
                else
                {
                    lambda_func = arena_sprintf(gen->arena,
                        "static %s %s(%s) {\n"
                        "%s"
                        "%s"
                        "    %s _return_value = %s;\n"
                        "%s"
                        "%s_return:\n"
                        "    return _return_value;\n"
                        "}\n\n",
                        ret_c_type, lambda_func_name, params_decl, arena_setup, capture_decls,
                        ret_c_type, default_val, body_code, lambda_func_name);
                }
            }
        }
        else
        {
            /* Single-line lambda with expression body */
            char *body_code = code_gen_expression(gen, lambda->body);
            if (modifier == FUNC_PRIVATE)
            {
                /* Private: create arena, compute result, destroy arena, return */
                lambda_func = arena_sprintf(gen->arena,
                    "static %s %s(%s) {\n"
                    "%s"
                    "%s"
                    "    %s __result__ = %s;\n"
                    "%s"
                    "    return __result__;\n"
                    "}\n\n",
                    ret_c_type, lambda_func_name, params_decl, arena_setup, capture_decls,
                    ret_c_type, body_code, arena_cleanup);
            }
            else
            {
                lambda_func = arena_sprintf(gen->arena,
                    "static %s %s(%s) {\n"
                    "%s"
                    "%s"
                    "    return %s;\n"
                    "}\n\n",
                    ret_c_type, lambda_func_name, params_decl, arena_setup, capture_decls, body_code);
            }
        }
        gen->current_arena_var = saved_arena_var;

        /* Pop the scope we pushed for captured primitives */
        symbol_table_pop_scope(gen->symbol_table);

        /* Append to definitions buffer */
        gen->lambda_definitions = arena_sprintf(gen->arena, "%s%s",
                                                gen->lambda_definitions, lambda_func);

        /* Determine which arena to use for closure allocation.
         * If this closure is being returned from a function, allocate in the
         * caller's arena so captured variables survive the function's local
         * arena destruction.
         * Note: In a lambda context (where current_arena_var is __lambda_arena__),
         * __caller_arena__ doesn't exist. Use the lambda's arena instead, which
         * is already the correct parent arena for returned closures. */
        const char *closure_arena;
        if (gen->allocate_closure_in_caller_arena)
        {
            /* Check if we're in a lambda context (no __caller_arena__) or main context
             * (main() doesn't have a caller, so no __caller_arena__ exists) */
            bool in_lambda_context = (strcmp(gen->current_arena_var, "__lambda_arena__") == 0);
            bool in_main_context = (gen->current_function != NULL && strcmp(gen->current_function, "main") == 0);
            closure_arena = (in_lambda_context || in_main_context) ? ARENA_VAR(gen) : "__caller_arena__";
        }
        else
        {
            closure_arena = ARENA_VAR(gen);
        }

        /* Return code that creates and populates the closure */
        char *closure_init = arena_sprintf(gen->arena,
            "({\n"
            "    __closure_%d__ *__cl__ = rt_arena_alloc(%s, sizeof(__closure_%d__));\n"
            "    __cl__->fn = (void *)__lambda_%d__;\n"
            "    __cl__->arena = %s;\n"
            "    __cl__->size = sizeof(__closure_%d__);\n",
            lambda_id, closure_arena, lambda_id, lambda_id, closure_arena, lambda_id);

        for (int i = 0; i < cv.count; i++)
        {
            /* Check if this is a recursive self-capture: the lambda is capturing
             * the variable it's being assigned to. In this case, skip the capture
             * during initialization (the variable isn't assigned yet) and the
             * caller (code_gen_var_declaration) will fix it up afterwards. */
            if (gen->current_decl_var_name != NULL &&
                strcmp(cv.names[i], gen->current_decl_var_name) == 0)
            {
                /* Mark this as a recursive lambda - caller will add self-fix */
                gen->recursive_lambda_id = lambda_id;
                /* Skip initializing this capture - will be done after declaration */
                continue;
            }

            /* For types needing capture by ref (primitives and arrays): check if the
             * variable is already a pointer or a value.
             * The symbol table tells us: MEM_AS_REF means already a pointer.
             * Variables that are NOT pointers (need heap allocation):
             * - Lambda parameters (values)
             * - Loop iteration variables (values)
             * Variables that ARE already pointers (just copy):
             * - Outer function scope with pre-pass pointer declaration
             * - Variables captured from outer lambda body
             * For other types, just copy the value. */
            if (needs_capture_by_ref(cv.types[i]))
            {
                /* Lookup the symbol to check if it's already a pointer (MEM_AS_REF) */
                Token name_token;
                name_token.start = cv.names[i];
                name_token.length = strlen(cv.names[i]);
                name_token.line = 0;
                Symbol *sym = symbol_table_lookup_symbol(gen->symbol_table, name_token);

                bool already_pointer = (sym != NULL && sym->mem_qual == MEM_AS_REF);
                if (already_pointer)
                {
                    /* The variable is already a pointer - just copy it to the closure */
                    closure_init = arena_sprintf(gen->arena, "%s    __cl__->%s = %s;\n",
                                                 closure_init, cv.names[i], cv.names[i]);
                }
                else
                {
                    /* It's a value (lambda param, loop var, etc.) - need to heap-allocate */
                    const char *c_type = get_c_type(gen->arena, cv.types[i]);
                    closure_init = arena_sprintf(gen->arena,
                        "%s    __cl__->%s = ({ %s *__tmp__ = rt_arena_alloc(%s, sizeof(%s)); *__tmp__ = %s; __tmp__; });\n",
                        closure_init, cv.names[i], c_type, closure_arena, c_type, cv.names[i]);
                }
            }
            else
            {
                closure_init = arena_sprintf(gen->arena, "%s    __cl__->%s = %s;\n",
                                             closure_init, cv.names[i], cv.names[i]);
            }
        }
        closure_init = arena_sprintf(gen->arena,
            "%s    (__Closure__ *)__cl__;\n"
            "})",
            closure_init);

        /* Pop this lambda from enclosing context */
        gen->enclosing_lambda_count--;

        /* Pop the lambda parameter scope we pushed at the start */
        symbol_table_pop_scope(gen->symbol_table);

        return closure_init;
    }
    else
    {
        /* No captures - use simple generic closure */
        /* Generate the static lambda function body - use lambda's arena */
        char *saved_arena_var = gen->current_arena_var;
        gen->current_arena_var = "__lambda_arena__";

        /* Push this lambda to enclosing context for nested lambdas */
        if (gen->enclosing_lambda_count >= gen->enclosing_lambda_capacity)
        {
            int new_cap = gen->enclosing_lambda_capacity == 0 ? 4 : gen->enclosing_lambda_capacity * 2;
            LambdaExpr **new_lambdas = arena_alloc(gen->arena, new_cap * sizeof(LambdaExpr *));
            for (int i = 0; i < gen->enclosing_lambda_count; i++)
            {
                new_lambdas[i] = gen->enclosing_lambdas[i];
            }
            gen->enclosing_lambdas = new_lambdas;
            gen->enclosing_lambda_capacity = new_cap;
        }
        gen->enclosing_lambdas[gen->enclosing_lambda_count++] = lambda;

        char *lambda_func_name = arena_sprintf(gen->arena, "__lambda_%d__", lambda_id);

        /* Generate forward declaration */
        char *forward_decl = arena_sprintf(gen->arena,
            "static %s %s(%s);\n",
            ret_c_type, lambda_func_name, params_decl);

        gen->lambda_forward_decls = arena_sprintf(gen->arena, "%s%s",
                                                  gen->lambda_forward_decls, forward_decl);

        /* Generate the actual lambda function definition */
        char *lambda_func;
        if (lambda->has_stmt_body)
        {
            /* Multi-line lambda with statement body - needs return value and label */
            char *body_code = code_gen_lambda_stmt_body(gen, lambda, 1, lambda_func_name, lambda->return_type);

            /* Check if void return type - special handling needed */
            int is_void_return = (lambda->return_type && lambda->return_type->kind == TYPE_VOID);

            if (is_void_return)
            {
                /* Void return - no return value declaration needed */
                if (modifier == FUNC_PRIVATE)
                {
                    lambda_func = arena_sprintf(gen->arena,
                        "static void %s(%s) {\n"
                        "%s"
                        "%s"
                        "%s_return:\n"
                        "%s"
                        "    return;\n"
                        "}\n\n",
                        lambda_func_name, params_decl, arena_setup,
                        body_code, lambda_func_name, arena_cleanup);
                }
                else
                {
                    lambda_func = arena_sprintf(gen->arena,
                        "static void %s(%s) {\n"
                        "%s"
                        "%s"
                        "%s_return:\n"
                        "    return;\n"
                        "}\n\n",
                        lambda_func_name, params_decl, arena_setup,
                        body_code, lambda_func_name);
                }
            }
            else
            {
                const char *default_val = get_default_value(lambda->return_type);
                if (modifier == FUNC_PRIVATE)
                {
                    lambda_func = arena_sprintf(gen->arena,
                        "static %s %s(%s) {\n"
                        "%s"
                        "    %s _return_value = %s;\n"
                        "%s"
                        "%s_return:\n"
                        "%s"
                        "    return _return_value;\n"
                        "}\n\n",
                        ret_c_type, lambda_func_name, params_decl, arena_setup,
                        ret_c_type, default_val, body_code, lambda_func_name, arena_cleanup);
                }
                else
                {
                    lambda_func = arena_sprintf(gen->arena,
                        "static %s %s(%s) {\n"
                        "%s"
                        "    %s _return_value = %s;\n"
                        "%s"
                        "%s_return:\n"
                        "    return _return_value;\n"
                        "}\n\n",
                        ret_c_type, lambda_func_name, params_decl, arena_setup,
                        ret_c_type, default_val, body_code, lambda_func_name);
                }
            }
        }
        else
        {
            /* Single-line lambda with expression body */
            char *body_code = code_gen_expression(gen, lambda->body);
            if (modifier == FUNC_PRIVATE)
            {
                /* Private: create arena, compute result, destroy arena, return */
                lambda_func = arena_sprintf(gen->arena,
                    "static %s %s(%s) {\n"
                    "%s"
                    "    %s __result__ = %s;\n"
                    "%s"
                    "    return __result__;\n"
                    "}\n\n",
                    ret_c_type, lambda_func_name, params_decl, arena_setup,
                    ret_c_type, body_code, arena_cleanup);
            }
            else
            {
                lambda_func = arena_sprintf(gen->arena,
                    "static %s %s(%s) {\n"
                    "%s"
                    "    return %s;\n"
                    "}\n\n",
                    ret_c_type, lambda_func_name, params_decl, arena_setup, body_code);
            }
        }
        gen->current_arena_var = saved_arena_var;

        /* Append to definitions buffer */
        gen->lambda_definitions = arena_sprintf(gen->arena, "%s%s",
                                                gen->lambda_definitions, lambda_func);

        /* Pop this lambda from enclosing context */
        gen->enclosing_lambda_count--;

        /* Pop the lambda parameter scope we pushed at the start */
        symbol_table_pop_scope(gen->symbol_table);

        /* Determine which arena to use for closure allocation.
         * If this closure is being returned from a function, allocate in the
         * caller's arena. In a lambda or main context, use the current arena
         * since there's no __caller_arena__ available. */
        const char *closure_arena;
        if (gen->allocate_closure_in_caller_arena)
        {
            bool in_lambda_context = (strcmp(gen->current_arena_var, "__lambda_arena__") == 0);
            bool in_main_context = (gen->current_function != NULL && strcmp(gen->current_function, "main") == 0);
            closure_arena = (in_lambda_context || in_main_context) ? ARENA_VAR(gen) : "__caller_arena__";
        }
        else
        {
            closure_arena = ARENA_VAR(gen);
        }

        /* Return code that creates the closure using generic __Closure__ type */
        return arena_sprintf(gen->arena,
            "({\n"
            "    __Closure__ *__cl__ = rt_arena_alloc(%s, sizeof(__Closure__));\n"
            "    __cl__->fn = (void *)__lambda_%d__;\n"
            "    __cl__->arena = %s;\n"
            "    __cl__->size = sizeof(__Closure__);\n"
            "    __cl__;\n"
            "})",
            closure_arena, lambda_id, closure_arena);
    }
}
