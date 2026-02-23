#include "code_gen/expr/lambda/code_gen_expr_lambda.h"
#include "code_gen.h"
#include "code_gen/expr/code_gen_expr.h"
#include "code_gen/util/code_gen_util.h"
#include "type_checker/util/type_checker_util.h"
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

/* Include split modules (unity build style) */
#include "code_gen_expr_lambda_local.c"
#include "code_gen_expr_lambda_capture.c"

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

    /* Push a scope for body-level local variables. Lambda parameters are NOT
     * added here — they're already in the enclosing scope pushed by
     * code_gen_lambda_expression. Adding them again would cause duplicate
     * cleanup (e.g. double rt_arena_v2_free) when the return statement
     * walks scopes to generate cleanup code. */
    symbol_table_push_scope(gen->symbol_table);

    /* Generate code for each statement in the lambda body */
    /* We need to capture the output since code_gen_statement writes to gen->output */
    int saved_tc = gen->arena_temp_count;
    int saved_ts = gen->arena_temp_serial;
    gen->arena_temp_count = 0;
    gen->arena_temp_serial = 0;

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

    gen->arena_temp_count = saved_tc;
    gen->arena_temp_serial = saved_ts;

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

/* Include native lambda generation */
#include "code_gen_expr_lambda_native.c"

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
        char *param_name = sn_mangle_name(gen->arena, arena_strndup(gen->arena, lambda->params[i].name.start,
                                         lambda->params[i].name.length));
        params_decl = arena_sprintf(gen->arena, "%s, %s %s", params_decl, param_c_type, param_name);
    }

    /* Generate arena handling code based on modifier.
     * Use rt_get_thread_arena_or() to prefer thread arena when closure is called
     * from a thread context. This ensures closures created in main() use the
     * calling thread's arena rather than main's arena. */
    char *arena_setup = arena_strdup(gen->arena, "");
    char *arena_cleanup = arena_strdup(gen->arena, "");

    if (modifier == FUNC_PRIVATE)
    {
        /* Private lambda: create child arena, destroy before return.
         * Parent is thread arena if in thread context, otherwise closure's stored arena. */
        arena_setup = arena_sprintf(gen->arena,
            "    RtArenaV2 *__lambda_arena__ = rt_arena_v2_create("
            "({ RtArenaV2 *__tls_a = rt_tls_arena_get(); __tls_a ? __tls_a : ((__Closure__ *)__closure__)->arena; }), RT_ARENA_MODE_PRIVATE, \"lambda\");\n"
            "    (void)__closure__;\n");
        arena_cleanup = arena_sprintf(gen->arena,
            "    rt_arena_v2_condemn(__lambda_arena__);\n");
    }
    else if (modifier == FUNC_SHARED)
    {
        /* Shared lambda: ALWAYS use the closure's stored arena.
         * This ensures the lambda operates on the arena where it was created,
         * which is critical for closures capturing arrays or other state
         * that needs to remain in the original arena. When a shared closure
         * is called from a thread, it should access the main thread's data. */
        arena_setup = arena_sprintf(gen->arena,
            "    RtArenaV2 *__lambda_arena__ = ((__Closure__ *)__closure__)->arena;\n");
    }
    else
    {
        /* Default lambda: use thread arena if in thread context,
         * otherwise use arena from closure */
        arena_setup = arena_sprintf(gen->arena,
            "    RtArenaV2 *__lambda_arena__ = "
            "({ RtArenaV2 *__tls_a = rt_tls_arena_get(); __tls_a ? __tls_a : ((__Closure__ *)__closure__)->arena; });\n");
    }

    if (cv.count > 0)
    {
        /* Generate custom closure struct for this lambda (with arena and size fields) */
        char *struct_def = arena_sprintf(gen->arena,
            "typedef struct __closure_%d__ {\n"
            "    void *fn;\n"
            "    RtArenaV2 *arena;\n"
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
            char *mangled_cv_name = sn_mangle_name(gen->arena, cv.names[i]);
            if (needs_capture_by_ref(cv.types[i]))
            {
                /* For types captured by ref, the closure stores a pointer. We declare
                 * a local pointer variable that references the closure's stored pointer,
                 * so access like (*count) or (*data) works naturally. We use a local
                 * variable instead of #define to avoid macro replacement issues when
                 * this lambda creates nested closures. */
                capture_decls = arena_sprintf(gen->arena,
                    "%s    %s *%s = ((__closure_%d__ *)__closure__)->%s;\n",
                    capture_decls, c_type, mangled_cv_name, lambda_id, cv.names[i]);
            }
            else
            {
                /* Other types: copy the value */
                capture_decls = arena_sprintf(gen->arena,
                    "%s    %s %s = ((__closure_%d__ *)__closure__)->%s;\n",
                    capture_decls, c_type, mangled_cv_name, lambda_id, cv.names[i]);
            }
        }

        /* Generate the static lambda function body - use lambda's arena */
        char *saved_arena_var = gen->current_arena_var;
        char *saved_function_arena = gen->function_arena_var;
        gen->current_arena_var = "__lambda_arena__";
        gen->function_arena_var = "__lambda_arena__";

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
            bool is_handle_return = gen->current_arena_var != NULL &&
                                    (lambda->return_type->kind == TYPE_ARRAY || lambda->return_type->kind == TYPE_STRING);
            bool saved_expr_handle = gen->expr_as_handle;
            if (is_handle_return) gen->expr_as_handle = true;

            /* Redirect output so hoisted arena temps go into the lambda
             * function body, not the enclosing function. */
            int saved_tc_e = gen->arena_temp_count;
            int saved_ts_e = gen->arena_temp_serial;
            gen->arena_temp_count = 0;
            gen->arena_temp_serial = 0;
            FILE *old_out_e = gen->output;
            char *expr_buf = NULL;
            size_t expr_sz = 0;
            gen->output = open_memstream(&expr_buf, &expr_sz);

            char *body_code = code_gen_expression(gen, lambda->body);

            sn_fclose(gen->output);
            gen->output = old_out_e;
            char *hoisted_decls = arena_strdup(gen->arena, expr_buf ? expr_buf : "");
            free(expr_buf);
            gen->arena_temp_count = saved_tc_e;
            gen->arena_temp_serial = saved_ts_e;

            gen->expr_as_handle = saved_expr_handle;
            if (modifier == FUNC_PRIVATE)
            {
                /* Private: create arena, compute result, destroy arena, return */
                lambda_func = arena_sprintf(gen->arena,
                    "static %s %s(%s) {\n"
                    "%s"
                    "%s"
                    "%s"
                    "    %s __result__ = %s;\n"
                    "%s"
                    "    return __result__;\n"
                    "}\n\n",
                    ret_c_type, lambda_func_name, params_decl, arena_setup, capture_decls,
                    hoisted_decls, ret_c_type, body_code, arena_cleanup);
            }
            else
            {
                lambda_func = arena_sprintf(gen->arena,
                    "static %s %s(%s) {\n"
                    "%s"
                    "%s"
                    "%s"
                    "    return %s;\n"
                    "}\n\n",
                    ret_c_type, lambda_func_name, params_decl, arena_setup, capture_decls, hoisted_decls, body_code);
            }
        }
        gen->current_arena_var = saved_arena_var;
        gen->function_arena_var = saved_function_arena;

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

        /* Return code that creates and populates the closure (V2 arena allocation) */
        char *closure_init = arena_sprintf(gen->arena,
            "({\n"
            "    RtHandleV2 *__cl_h__ = rt_arena_v2_alloc(%s, sizeof(__closure_%d__));\n"
            "    __closure_%d__ *__cl__ = (__closure_%d__ *)__cl_h__->ptr;\n"
            "    rt_handle_begin_transaction(__cl_h__);\n"
            "    __cl__->fn = (void *)__lambda_%d__;\n"
            "    __cl__->arena = %s;\n"
            "    __cl__->size = sizeof(__closure_%d__);\n",
            closure_arena, lambda_id, lambda_id, lambda_id, lambda_id, closure_arena, lambda_id);

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

                char *mangled_cv_name = sn_mangle_name(gen->arena, cv.names[i]);
                bool already_pointer = (sym != NULL && sym->mem_qual == MEM_AS_REF);
                if (already_pointer)
                {
                    /* The variable is already a pointer - just copy it to the closure */
                    closure_init = arena_sprintf(gen->arena, "%s    __cl__->%s = %s;\n",
                                                 closure_init, cv.names[i], mangled_cv_name);
                }
                else
                {
                    /* It's a value (lambda param, loop var, etc.) - need to heap-allocate (V2) */
                    const char *c_type = get_c_type(gen->arena, cv.types[i]);
                    closure_init = arena_sprintf(gen->arena,
                        "%s    __cl__->%s = ({ RtHandleV2 *__ah = rt_arena_v2_alloc(%s, sizeof(%s)); rt_handle_begin_transaction(__ah); %s *__tmp__ = (%s *)__ah->ptr; *__tmp__ = %s; rt_handle_end_transaction(__ah); __tmp__; });\n",
                        closure_init, cv.names[i], closure_arena, c_type, c_type, c_type, mangled_cv_name);
                }
            }
            else
            {
                closure_init = arena_sprintf(gen->arena, "%s    __cl__->%s = %s;\n",
                                             closure_init, cv.names[i], sn_mangle_name(gen->arena, cv.names[i]));
            }
        }
        closure_init = arena_sprintf(gen->arena,
            "%s    rt_handle_end_transaction(__cl_h__);\n"
            "    (__Closure__ *)__cl__;\n"
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
        char *saved_function_arena = gen->function_arena_var;
        gen->current_arena_var = "__lambda_arena__";
        gen->function_arena_var = "__lambda_arena__";

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
            bool is_handle_return = gen->current_arena_var != NULL &&
                                    (lambda->return_type->kind == TYPE_ARRAY || lambda->return_type->kind == TYPE_STRING);
            bool saved_expr_handle = gen->expr_as_handle;
            if (is_handle_return) gen->expr_as_handle = true;

            /* Redirect output so hoisted arena temps go into the lambda
             * function body, not the enclosing function. */
            int saved_tc_e = gen->arena_temp_count;
            int saved_ts_e = gen->arena_temp_serial;
            gen->arena_temp_count = 0;
            gen->arena_temp_serial = 0;
            FILE *old_out_e = gen->output;
            char *expr_buf = NULL;
            size_t expr_sz = 0;
            gen->output = open_memstream(&expr_buf, &expr_sz);

            char *body_code = code_gen_expression(gen, lambda->body);

            sn_fclose(gen->output);
            gen->output = old_out_e;
            char *hoisted_decls = arena_strdup(gen->arena, expr_buf ? expr_buf : "");
            free(expr_buf);
            gen->arena_temp_count = saved_tc_e;
            gen->arena_temp_serial = saved_ts_e;

            gen->expr_as_handle = saved_expr_handle;
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
                    ret_c_type, lambda_func_name, params_decl, arena_setup,
                    hoisted_decls, ret_c_type, body_code, arena_cleanup);
            }
            else
            {
                lambda_func = arena_sprintf(gen->arena,
                    "static %s %s(%s) {\n"
                    "%s"
                    "%s"
                    "    return %s;\n"
                    "}\n\n",
                    ret_c_type, lambda_func_name, params_decl, arena_setup, hoisted_decls, body_code);
            }
        }
        gen->current_arena_var = saved_arena_var;
        gen->function_arena_var = saved_function_arena;

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
            "    RtHandleV2 *__cl_h__ = rt_arena_v2_alloc(%s, sizeof(__Closure__));\n"
            "    __Closure__ *__cl__ = (__Closure__ *)__cl_h__->ptr;\n"
            "    rt_handle_begin_transaction(__cl_h__);\n"
            "    __cl__->fn = (void *)__lambda_%d__;\n"
            "    __cl__->arena = %s;\n"
            "    __cl__->size = sizeof(__Closure__);\n"
            "    rt_handle_end_transaction(__cl_h__);\n"
            "    __cl__;\n"
            "})",
            closure_arena, lambda_id, closure_arena);
    }
}
