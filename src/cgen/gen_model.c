#include "cgen/gen_model.h"
#include "type_checker/type_checker_generics.h"
#include "type_checker/stmt/type_checker_stmt_func.h"
#include <stdio.h>
#include <string.h>

/* Global lambda collection */
json_object *g_model_lambdas = NULL;

/* Global thread collection */
json_object *g_model_threads = NULL;
int g_model_thread_count = 0;
int g_model_lambda_count = 0;

/* Global captured-variable set */
char **g_captured_vars = NULL;
int g_captured_var_count = 0;

/* Global as-ref parameter names (used to emit -> for member access) */
char **g_as_ref_param_names = NULL;
int g_as_ref_param_count = 0;

/* All parameter names of the current function (used to detect borrowed returns) */
char **g_all_param_names = NULL;
int g_all_param_count = 0;

/* Global thread-handle-variable set */
char **g_thread_handle_vars = NULL;
int g_thread_handle_var_count = 0;

/* Suppress auto-cleanup on local array/string vars (set for functions returning structs with heap fields) */
bool g_suppress_local_cleanup = false;
bool g_has_pragma_source = false;
bool g_in_lambda_body = false;
bool g_in_main_void = false;
bool g_in_thread_spawn_call = false;

char *g_closure_var_names[MAX_CLOSURE_VAR_MAP];
int g_closure_var_lambda_ids[MAX_CLOSURE_VAR_MAP];
int g_closure_var_count = 0;

/* Lock scope stack */
const char *g_lock_var_names[MAX_LOCK_DEPTH];
int g_lock_depth = 0;

/* Global fn-wrapper collection for function-as-parameter wrapping */
json_object *g_model_fn_wrappers = NULL;
int g_model_fn_wrapper_count = 0;

/* Counter for synthetic struct-rvalue member-access lift temporaries (issue #49) */
int g_model_member_lift_count = 0;

/* Current namespace prefix for namespaced imports */
const char *g_model_namespace_prefix = NULL;

/* Module statements for callee body analysis */
Stmt **g_model_module_stmts = NULL;
int g_model_module_stmt_count = 0;

/* Module-level names for namespace prefixing inside function bodies */
const char **g_model_ns_static_var_names = NULL;
int g_model_ns_static_var_count = 0;
const char **g_model_ns_instance_var_names = NULL;
int g_model_ns_instance_var_count = 0;
const char **g_model_ns_fn_names = NULL;
const char **g_model_ns_fn_aliases = NULL;
int g_model_ns_fn_count = 0;

/* Canonical module prefix for static variable sharing */
const char *g_model_canonical_prefix = NULL;

/* Extract canonical module name from import path (basename without extension).
 * E.g., "./imports/deep_chain/level_01" → "level_01" */
static const char *canonical_name_from_path(Arena *arena, const char *path, int path_len)
{
    /* Find the last '/' to get basename */
    int start = 0;
    for (int i = path_len - 1; i >= 0; i--)
    {
        if (path[i] == '/')
        { start = i + 1; break; }
    }
    /* Strip .sn extension if present */
    int end = path_len;
    if (end - start > 3 && path[end - 3] == '.' && path[end - 2] == 's' && path[end - 1] == 'n')
        end -= 3;
    int len = end - start;
    char *name = arena_alloc(arena, len + 1);
    memcpy(name, path + start, len);
    name[len] = '\0';
    return name;
}

/* Check if name exists in emitted list */
static bool already_emitted(const char *name, const char **emitted, int count)
{
    for (int d = 0; d < count; d++)
        if (strcmp(emitted[d], name) == 0)
            return true;
    return false;
}

/* Add name to emitted list (arena-duplicated) */
static void track_emitted(Arena *arena, const char *name,
                           const char **emitted, int *count, int capacity)
{
    if (*count >= capacity) return;
    char *dup = arena_alloc(arena, strlen(name) + 1);
    strcpy(dup, name);
    emitted[(*count)++] = dup;
}

/* Recursive helper: emit structs, globals, and functions from a namespace import.
 * ns_prefix is the alias-based prefix (e.g., "L1__level_02__L3").
 * canonical is the module-name-based prefix (e.g., "level_01__level_02__level_03").
 * Static vars use canonical prefix (shared); instance vars use ns_prefix (per-alias).
 * emitted_names/emitted_count track all emitted names to avoid duplicates. */
static void emit_ns_import_recursive(
    Arena *arena, Stmt **stmts, int count,
    const char *ns_prefix, const char *canonical,
    json_object *structs, json_object *globals, json_object *functions,
    json_object *pragmas,
    SymbolTable *symbol_table, ArithmeticMode arithmetic_mode,
    const char **emitted_names, int *emitted_count, int emitted_capacity)
{
    const char *old_prefix = g_model_namespace_prefix;
    const char *old_canonical = g_model_canonical_prefix;
    g_model_namespace_prefix = ns_prefix;
    g_model_canonical_prefix = canonical;

    /* Collect module-level variable names (split static vs instance) and function names */
    const char *static_var_names[256];
    int static_var_count = 0;
    const char *instance_var_names[256];
    int instance_var_count = 0;
    const char *fn_names[256];
    const char *fn_aliases[256];
    int fn_count = 0;

    for (int i = 0; i < count; i++)
    {
        Stmt *s = stmts[i];
        if (!s) continue;
        if (s->type == STMT_VAR_DECL)
        {
            char *vn = arena_alloc(arena, s->as.var_decl.name.length + 1);
            memcpy(vn, s->as.var_decl.name.start, s->as.var_decl.name.length);
            vn[s->as.var_decl.name.length] = '\0';

            if (s->as.var_decl.is_static)
            {
                if (static_var_count < 256)
                    static_var_names[static_var_count++] = vn;

                /* Static vars use canonical prefix, emitted once per module */
                char prefixed[512];
                snprintf(prefixed, sizeof(prefixed), "%s__%s", canonical, vn);
                if (!already_emitted(prefixed, emitted_names, *emitted_count))
                {
                    track_emitted(arena, prefixed, emitted_names, emitted_count, emitted_capacity);
                    json_object *gvar = gen_model_stmt(arena, s, symbol_table, arithmetic_mode);
                    json_object_object_add(gvar, "name", json_object_new_string(prefixed));
                    json_object_object_add(gvar, "is_global", json_object_new_boolean(true));
                    json_object_object_add(gvar, "is_static", json_object_new_boolean(true));
                    bool is_deferred = false;
                    if (s->as.var_decl.initializer)
                    {
                        if (s->as.var_decl.initializer->type != EXPR_LITERAL)
                            is_deferred = true;
                        else if (s->as.var_decl.type && s->as.var_decl.type->kind == TYPE_STRING)
                            is_deferred = true;
                    }
                    if (is_deferred)
                        json_object_object_add(gvar, "is_deferred", json_object_new_boolean(true));
                    json_object_array_add(globals, gvar);
                }
            }
            else
            {
                if (instance_var_count < 256)
                    instance_var_names[instance_var_count++] = vn;

                /* Instance vars use namespace prefix, emitted per alias */
                char prefixed[512];
                snprintf(prefixed, sizeof(prefixed), "%s__%s", ns_prefix, vn);
                if (!already_emitted(prefixed, emitted_names, *emitted_count))
                {
                    track_emitted(arena, prefixed, emitted_names, emitted_count, emitted_capacity);
                    json_object *gvar = gen_model_stmt(arena, s, symbol_table, arithmetic_mode);
                    json_object_object_add(gvar, "name", json_object_new_string(prefixed));
                    json_object_object_add(gvar, "is_global", json_object_new_boolean(true));
                    bool is_deferred = false;
                    if (s->as.var_decl.initializer)
                    {
                        if (s->as.var_decl.initializer->type != EXPR_LITERAL)
                            is_deferred = true;
                        else if (s->as.var_decl.type && s->as.var_decl.type->kind == TYPE_STRING)
                            is_deferred = true;
                    }
                    if (is_deferred)
                        json_object_object_add(gvar, "is_deferred", json_object_new_boolean(true));
                    json_object_array_add(globals, gvar);
                }
            }
        }
        else if (s->type == STMT_FUNCTION && fn_count < 256)
        {
            char *fn = arena_alloc(arena, s->as.function.name.length + 1);
            memcpy(fn, s->as.function.name.start, s->as.function.name.length);
            fn[s->as.function.name.length] = '\0';
            fn_aliases[fn_count] = s->as.function.c_alias; /* NULL if no alias */
            fn_names[fn_count++] = fn;
        }
    }

    /* Collect pragmas from imported module (@include, @link directives) */
    if (pragmas)
    {
        for (int i = 0; i < count; i++)
        {
            Stmt *s = stmts[i];
            if (!s || s->type != STMT_PRAGMA) continue;
            json_object_array_add(pragmas,
                gen_model_stmt(arena, s, symbol_table, arithmetic_mode));
        }
    }

    /* Emit structs (once per struct name, regardless of import path) */
    for (int i = 0; i < count; i++)
    {
        Stmt *s = stmts[i];
        if (!s || s->type != STMT_STRUCT_DECL) continue;

        const char *sname = s->as.struct_decl.name.start;
        int slen = s->as.struct_decl.name.length;
        char sname_buf[256];
        if (slen > 255) slen = 255;
        memcpy(sname_buf, sname, slen);
        sname_buf[slen] = '\0';

        if (!already_emitted(sname_buf, emitted_names, *emitted_count))
        {
            track_emitted(arena, sname_buf, emitted_names, emitted_count, emitted_capacity);
            json_object_array_add(structs,
                gen_model_struct(arena, &s->as.struct_decl,
                                 symbol_table, arithmetic_mode));
        }
    }

    /* Emit functions with namespace variable/function name lists active.
     * Functions are emitted per alias (ns_prefix) because they reference
     * instance vars with alias-specific prefixes. */
    for (int i = 0; i < count; i++)
    {
        Stmt *s = stmts[i];
        if (!s || s->type != STMT_FUNCTION) continue;

        if (strncmp(s->as.function.name.start, "main", 4) == 0 &&
            s->as.function.name.length == 4)
            continue;
        if (s->as.function.is_native && s->as.function.body_count == 0)
            continue;

        /* Dedup by fully qualified name (alias path + function name) */
        char fqn[512];
        snprintf(fqn, sizeof(fqn), "%s__%.*s", ns_prefix,
                 s->as.function.name.length, s->as.function.name.start);
        if (already_emitted(fqn, emitted_names, *emitted_count))
            continue;
        track_emitted(arena, fqn, emitted_names, emitted_count, emitted_capacity);

        g_model_ns_static_var_names = static_var_names;
        g_model_ns_static_var_count = static_var_count;
        g_model_ns_instance_var_names = instance_var_names;
        g_model_ns_instance_var_count = instance_var_count;
        g_model_ns_fn_names = fn_names;
        g_model_ns_fn_aliases = fn_aliases;
        g_model_ns_fn_count = fn_count;
        json_object_array_add(functions,
            gen_model_function(arena, &s->as.function,
                               symbol_table, arithmetic_mode));
        g_model_ns_static_var_names = NULL;
        g_model_ns_static_var_count = 0;
        g_model_ns_instance_var_names = NULL;
        g_model_ns_instance_var_count = 0;
        g_model_ns_fn_names = NULL;
        g_model_ns_fn_aliases = NULL;
        g_model_ns_fn_count = 0;
    }

    /* Recurse into nested namespace imports */
    for (int i = 0; i < count; i++)
    {
        Stmt *s = stmts[i];
        if (!s) continue;
        if (s->type == STMT_IMPORT &&
            s->as.import.namespace != NULL &&
            s->as.import.imported_stmts != NULL)
        {
            char nested_buf[256];
            int nlen = s->as.import.namespace->length < 255
                       ? s->as.import.namespace->length : 255;
            memcpy(nested_buf, s->as.import.namespace->start, nlen);
            nested_buf[nlen] = '\0';

            /* Build combined alias prefix */
            char combined_ns[512];
            snprintf(combined_ns, sizeof(combined_ns), "%s__%s", ns_prefix, nested_buf);

            /* Canonical prefix is just the module's own filename (not chained).
             * This ensures all paths to the same module share static vars. */
            const char *nested_canonical = canonical_name_from_path(
                arena, s->as.import.module_name.start, s->as.import.module_name.length);

            emit_ns_import_recursive(arena, s->as.import.imported_stmts,
                                      s->as.import.imported_count,
                                      combined_ns, nested_canonical,
                                      structs, globals, functions, pragmas,
                                      symbol_table, arithmetic_mode,
                                      emitted_names, emitted_count, emitted_capacity);
        }
    }

    g_model_namespace_prefix = old_prefix;
    g_model_canonical_prefix = old_canonical;
}

json_object *gen_model_build(Arena *arena, Module *module, SymbolTable *symbol_table,
                             ArithmeticMode arithmetic_mode)
{
    g_model_module_stmts = module->statements;
    g_model_module_stmt_count = module->count;
    json_object *root = json_object_new_object();

    /* Module metadata */
    json_object *mod = json_object_new_object();
    /* Normalize path separators so JSON model is consistent across platforms */
    {
        const char *fn = module->filename ? module->filename : "";
        char norm_fn[1024];
        size_t fnlen = strlen(fn);
        if (fnlen >= sizeof(norm_fn)) fnlen = sizeof(norm_fn) - 1;
        memcpy(norm_fn, fn, fnlen);
        norm_fn[fnlen] = '\0';
        for (size_t i = 0; i < fnlen; i++)
            if (norm_fn[i] == '\\') norm_fn[i] = '/';
        json_object_object_add(mod, "filename", json_object_new_string(norm_fn));
    }

    /* Check if module has a main function and forward-declarable functions */
    bool has_main = false;
    bool main_returns = false;
    bool has_main_args = false;
    bool has_forward_decls = false;
    for (int i = 0; i < module->count; i++)
    {
        Stmt *stmt = module->statements[i];
        if (stmt->type == STMT_FUNCTION)
        {
            if (strcmp(stmt->as.function.name.start, "main") == 0)
            {
                has_main = true;
                if (stmt->as.function.param_count > 0)
                    has_main_args = true;
                int bc = stmt->as.function.body_count;
                if (bc > 0 && stmt->as.function.body[bc - 1]->type == STMT_RETURN)
                {
                    Expr *ret_val = stmt->as.function.body[bc - 1]->as.return_stmt.value;
                    /* Only count as main_returns if the return value is non-void */
                    if (!ret_val || !ret_val->expr_type || ret_val->expr_type->kind != TYPE_VOID)
                        main_returns = true;
                }
            }
            else if (!stmt->as.function.is_native)
            {
                has_forward_decls = true;
            }
        }
    }
    json_object_object_add(mod, "has_main", json_object_new_boolean(has_main));
    json_object_object_add(mod, "main_returns", json_object_new_boolean(main_returns));
    json_object_object_add(mod, "has_main_args", json_object_new_boolean(has_main_args));
    json_object_object_add(mod, "has_forward_decls", json_object_new_boolean(has_forward_decls));
    json_object_object_add(root, "module", mod);

    /* Collect top-level declarations into categorized arrays */
    json_object *pragmas = json_object_new_array();
    json_object *imports = json_object_new_array();
    json_object *type_decls = json_object_new_array();
    json_object *structs = json_object_new_array();
    json_object *globals = json_object_new_array();
    json_object *functions = json_object_new_array();
    json_object *top_level = json_object_new_array();

    /* Initialize global lambda collection */
    g_model_lambdas = json_object_new_array();

    /* Initialize global thread collection */
    g_model_threads = json_object_new_array();
    g_model_thread_count = 0;
    g_model_lambda_count = 0;

    /* Initialize global fn-wrapper collection */
    g_model_fn_wrappers = json_object_new_array();
    g_model_fn_wrapper_count = 0;

    /* Reset member-lift counter */
    g_model_member_lift_count = 0;

    /* Track emitted names (structs, globals, functions) to avoid duplicates
     * from multiple import paths reaching the same module */
    const int emitted_capacity = 4096;
    const char **emitted_names = arena_alloc(arena, sizeof(const char *) * emitted_capacity);
    int emitted_count = 0;

    /* Pre-register all monomorphized generic functions in the global symbol table.
     * During type-checking, monomorphized functions (e.g. identity_int) are added to
     * whatever function scope triggered their instantiation (e.g. inside main()).
     * That scope is popped after type-checking completes, so by codegen time the
     * symbols are gone.  The closure-call detector in gen_model_expr looks them up:
     * if not found → is_closure=true → crash.  Re-registering here, before any
     * module body is emitted, ensures every generic function instantiation is found
     * as a regular (non-closure) function. */
    {
        int fn_inst_count = generic_registry_function_instantiation_count();
        for (int i = 0; i < fn_inst_count; i++)
        {
            GenericFunctionInstantiation *inst = generic_registry_get_function_instantiation(i);
            if (inst == NULL || inst->instantiated_decl == NULL)
                continue;
            FunctionStmt *mono_fn = inst->instantiated_decl;
            Type **ptypes = arena_alloc(arena, sizeof(Type *) * (mono_fn->param_count + 1));
            for (int j = 0; j < mono_fn->param_count; j++)
                ptypes[j] = mono_fn->params[j].type;
            Type *fn_type = ast_create_function_type(arena, mono_fn->return_type,
                                                      ptypes, mono_fn->param_count);
            symbol_table_add_function(symbol_table, mono_fn->name, fn_type,
                                      FUNC_DEFAULT, FUNC_DEFAULT);
        }
    }

    for (int i = 0; i < module->count; i++)
    {
        Stmt *stmt = module->statements[i];

        switch (stmt->type)
        {
            case STMT_PRAGMA:
            {
                json_object_array_add(pragmas,
                    gen_model_stmt(arena, stmt, symbol_table, arithmetic_mode));
                break;
            }

            case STMT_IMPORT:
            {
                json_object_array_add(imports,
                    gen_model_stmt(arena, stmt, symbol_table, arithmetic_mode));

                /* For namespaced imports, recursively emit structs, globals,
                 * and functions with namespace-prefixed names.
                 * Deduplication is handled by qualified name tracking inside
                 * the recursive helper, so all aliases are processed. */
                if (stmt->as.import.namespace != NULL &&
                    stmt->as.import.imported_stmts != NULL)
                {
                    char ns_buf[256];
                    int ns_len = stmt->as.import.namespace->length < 255
                                 ? stmt->as.import.namespace->length : 255;
                    memcpy(ns_buf, stmt->as.import.namespace->start, ns_len);
                    ns_buf[ns_len] = '\0';

                    /* Canonical prefix from import path basename */
                    const char *canon = canonical_name_from_path(
                        arena, stmt->as.import.module_name.start,
                        stmt->as.import.module_name.length);

                    emit_ns_import_recursive(arena, stmt->as.import.imported_stmts,
                                              stmt->as.import.imported_count,
                                              ns_buf, canon,
                                              structs, globals, functions, pragmas,
                                              symbol_table, arithmetic_mode,
                                              emitted_names, &emitted_count,
                                              emitted_capacity);
                }
                break;
            }

            case STMT_TYPE_DECL:
            {
                json_object_array_add(type_decls,
                    gen_model_stmt(arena, stmt, symbol_table, arithmetic_mode));
                break;
            }

            case STMT_STRUCT_DECL:
            {
                /* Skip generic templates — only emit monomorphized instantiations */
                if (stmt->as.struct_decl.type_param_count > 0)
                    break;
                json_object_array_add(structs,
                    gen_model_struct(arena, &stmt->as.struct_decl, symbol_table, arithmetic_mode));
                break;
            }

            case STMT_INTERFACE_DECL:
                /* Interfaces are compile-time only — no C output */
                break;

            case STMT_VAR_DECL:
            {
                json_object *gvar = gen_model_stmt(arena, stmt, symbol_table, arithmetic_mode);
                /* Source file tracking for modular compilation */
                if (stmt->as.var_decl.name.filename)
                    gen_model_add_source_file(gvar, stmt->as.var_decl.name.filename);
                /* Check if initializer is non-constant (needs deferred init in main).
                 * For c-min: string globals with literal initializers are also deferred
                 * so they can be strdup'd in main (string literals can't be free'd). */
                bool is_deferred = false;
                if (stmt->as.var_decl.initializer)
                {
                    if (stmt->as.var_decl.initializer->type != EXPR_LITERAL)
                        is_deferred = true;
                    else if (stmt->as.var_decl.type &&
                             stmt->as.var_decl.type->kind == TYPE_STRING)
                        is_deferred = true;
                }
                if (is_deferred)
                    json_object_object_add(gvar, "is_deferred", json_object_new_boolean(true));
                /* Mark as global for cleanup purposes (globals don't get cleanup attrs,
                 * but need explicit cleanup at program exit) */
                json_object_object_add(gvar, "is_global", json_object_new_boolean(true));
                json_object_array_add(globals, gvar);
                break;
            }

            case STMT_FUNCTION:
            {
                /* Skip generic function templates — only emit monomorphized instantiations */
                if (stmt->as.function.type_param_count > 0)
                    break;
                json_object_array_add(functions,
                    gen_model_function(arena, &stmt->as.function, symbol_table, arithmetic_mode));
                break;
            }

            default:
            {
                /* Other top-level statements (expression statements, etc.) */
                json_object_array_add(top_level,
                    gen_model_stmt(arena, stmt, symbol_table, arithmetic_mode));
                break;
            }
        }
    }

    /* Emit all monomorphized generic struct instantiations.
     * These are generated on-demand during type checking and not present in the module
     * statement list, so they must be injected here.
     *
     * Before emitting each instantiation, re-type-check its method bodies with the
     * correct concrete type parameters.  Generic struct method bodies are SHARED between
     * all instantiations.  Cached expr_type values from the last type-check pass (which
     * may correspond to a different instantiation) would cause the codegen to emit
     * incorrect types (e.g. push/pop emitting strdup for a Stack<int> int field).
     * Re-running the body type-check immediately before codegen refreshes expr_type. */
    {
        int inst_count = generic_registry_instantiation_count();
        for (int i = 0; i < inst_count; i++)
        {
            GenericInstantiation *inst = generic_registry_get_instantiation(i);
            if (inst == NULL || inst->instantiated_decl == NULL)
                continue;

            /* Find the corresponding template so we can set up type-param aliases. */
            GenericTemplate *tmpl = generic_registry_find_template(inst->template_name);
            if (tmpl != NULL && tmpl->decl->type_param_count == inst->type_arg_count)
            {
                /* Register type param → concrete type mappings as aliases. */
                for (int t = 0; t < inst->type_arg_count; t++)
                {
                    if (tmpl->decl->type_params[t] == NULL || inst->type_args[t] == NULL)
                        continue;
                    Token tp_tok;
                    tp_tok.start    = tmpl->decl->type_params[t];
                    tp_tok.length   = (int)strlen(tmpl->decl->type_params[t]);
                    tp_tok.type     = TOKEN_IDENTIFIER;
                    tp_tok.line     = 0;
                    tp_tok.filename = NULL;
                    symbol_table_add_type(symbol_table, tp_tok, inst->type_args[t]);
                }

                /* Clear and re-type-check shared method bodies. */
                type_check_generic_instantiation_decl(arena, inst->instantiated_decl,
                                                       symbol_table);
            }

            json_object_array_add(structs,
                gen_model_struct(arena, inst->instantiated_decl,
                                 symbol_table, arithmetic_mode));
        }
    }

    /* Emit all monomorphized generic function instantiations.
     *
     * Before emitting each instantiation, re-type-check its body with the
     * correct concrete type parameters.  Generic function bodies are SHARED
     * between all instantiations.  type_check_expr() caches its result in
     * expr->expr_type, so after type-checking identity<int> and then
     * identity<str>, the shared body nodes carry TYPE_STRING from the second
     * pass.  When codegen emits identity_int it would see the stale str types
     * and emit e.g. "char * __ret__ = __sn__x" for a long long variable.
     * Re-running the body type-check immediately before codegen refreshes
     * expr_type with the correct concrete types for this instantiation. */
    {
        int fn_inst_count = generic_registry_function_instantiation_count();
        for (int i = 0; i < fn_inst_count; i++)
        {
            GenericFunctionInstantiation *inst = generic_registry_get_function_instantiation(i);
            if (inst == NULL || inst->instantiated_decl == NULL)
                continue;

            /* Re-type-check the shared body with correct concrete type params. */
            GenericFunctionTemplate *fn_tmpl =
                generic_registry_find_function_template(inst->template_name);
            if (fn_tmpl != NULL &&
                fn_tmpl->decl->type_param_count == inst->type_arg_count)
            {
                /* Clear stale expr_type cache from shared body AST nodes. */
                clear_expr_types_in_stmts(inst->instantiated_decl->body,
                                          inst->instantiated_decl->body_count);

                /* Register type param → concrete type mappings as aliases. */
                for (int t = 0; t < inst->type_arg_count; t++)
                {
                    if (fn_tmpl->decl->type_params[t] == NULL || inst->type_args[t] == NULL)
                        continue;
                    Token tp_tok;
                    tp_tok.start    = fn_tmpl->decl->type_params[t];
                    tp_tok.length   = (int)strlen(fn_tmpl->decl->type_params[t]);
                    tp_tok.type     = TOKEN_IDENTIFIER;
                    tp_tok.line     = 0;
                    tp_tok.filename = NULL;
                    symbol_table_add_type(symbol_table, tp_tok, inst->type_args[t]);
                }

                /* Re-type-check using a stack wrapper (avoids duplicate global registration). */
                Stmt wrapper;
                memset(&wrapper, 0, sizeof(Stmt));
                wrapper.type        = STMT_FUNCTION;
                wrapper.token       = &inst->instantiated_decl->name;
                wrapper.as.function = *inst->instantiated_decl;
                type_check_function_body_only(&wrapper, symbol_table);
            }

            json_object_array_add(functions,
                gen_model_function(arena, inst->instantiated_decl,
                                   symbol_table, arithmetic_mode));
        }
    }

    json_object_object_add(root, "pragmas", pragmas);
    json_object_object_add(root, "imports", imports);
    json_object_object_add(root, "type_decls", type_decls);
    json_object_object_add(root, "structs", structs);
    json_object_object_add(root, "globals", globals);
    json_object_object_add(root, "functions", functions);
    json_object_object_add(root, "top_level_statements", top_level);
    json_object_object_add(root, "lambdas", g_model_lambdas);
    g_model_lambdas = NULL;
    json_object_object_add(root, "threads", g_model_threads);
    g_model_threads = NULL;
    json_object_object_add(root, "fn_wrappers", g_model_fn_wrappers);
    g_model_fn_wrappers = NULL;

    return root;
}

int gen_model_write(json_object *model, const char *output_path)
{
    const char *json_str = json_object_to_json_string_ext(model,
        JSON_C_TO_STRING_PRETTY | JSON_C_TO_STRING_SPACED);

    if (!json_str)
    {
        fprintf(stderr, "Error: Failed to serialize model to JSON\n");
        return 1;
    }

    FILE *f = fopen(output_path, "wb");
    if (!f)
    {
        fprintf(stderr, "Error: Cannot open output file: %s\n", output_path);
        return 1;
    }

    fprintf(f, "%s\n", json_str);
    fclose(f);

    return 0;
}
