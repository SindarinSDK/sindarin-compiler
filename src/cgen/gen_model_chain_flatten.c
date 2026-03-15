/*
 * gen_model_chain_flatten.c — Post-processing pass to flatten method chains.
 *
 * When a member call's object is itself a call, the template codegen cannot
 * take the address of the temporary inline (C lifetime rules).
 * This pass rewrites such chains by extracting the inner call into a preceding
 * var_decl statement with a temporary variable, then replacing the object with
 * a variable reference.
 *
 * Example: a.setValue(42).setName("x")
 *   Before: call(member(call(member(var "a"), "setValue"), "setName"), ...)
 *   After:  var_decl __chain_tmp_0 = call(member(var "a"), "setValue")
 *           call(member(var "__chain_tmp_0"), "setName", ...)
 */

#include <json-c/json.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

static int g_chain_tmp_count = 0;
static json_object *g_model_structs = NULL;

/* Look up a struct definition by name in the model's structs array to get
 * the authoritative pass_self_by_ref flag. Expression types from unresolved
 * forward references may have pass_self_by_ref=false even when the struct
 * is declared 'as ref'. */
static bool struct_is_pass_by_ref(const char *struct_name)
{
    if (!g_model_structs || !struct_name) return false;
    int len = json_object_array_length(g_model_structs);
    for (int i = 0; i < len; i++)
    {
        json_object *st = json_object_array_get_idx(g_model_structs, i);
        json_object *name_obj = NULL;
        if (json_object_object_get_ex(st, "name", &name_obj))
        {
            if (strcmp(json_object_get_string(name_obj), struct_name) == 0)
            {
                json_object *pbr = NULL;
                if (json_object_object_get_ex(st, "pass_self_by_ref", &pbr))
                    return json_object_get_boolean(pbr);
                return false;
            }
        }
    }
    return false;
}

/* Check if an expression is NOT an lvalue (needs extraction into a temp variable).
 * Lvalues are: variable, member (field access), array_access.
 * Everything else (calls, literals, binary ops, etc.) needs a temp. */
static bool needs_temp_extraction(json_object *expr)
{
    if (!expr) return false;

    json_object *kind_obj = NULL;
    if (!json_object_object_get_ex(expr, "kind", &kind_obj)) return false;
    const char *kind = json_object_get_string(kind_obj);

    /* These are lvalues — their address can be taken directly */
    if (strcmp(kind, "variable") == 0) return false;
    if (strcmp(kind, "member") == 0) return false;
    if (strcmp(kind, "array_access") == 0) return false;

    /* Everything else needs a temp */
    return true;
}

/* Add cleanup annotations to a chain temp var_decl based on its struct type.
 * Uses the model's struct definitions as the source of truth for pass_self_by_ref. */
static void annotate_chain_temp(json_object *var_decl, json_object *obj_type)
{
    if (!obj_type) return;

    json_object *kind_obj = NULL;
    json_object_object_get_ex(obj_type, "kind", &kind_obj);
    if (!kind_obj || strcmp(json_object_get_string(kind_obj), "struct") != 0) return;

    json_object *name_obj = NULL;
    json_object_object_get_ex(obj_type, "name", &name_obj);
    if (!name_obj) return;

    const char *struct_name = json_object_get_string(name_obj);
    if (struct_is_pass_by_ref(struct_name))
    {
        json_object_object_add(var_decl, "cleanup_kind",
            json_object_new_string("release"));
        json_object_object_add(var_decl, "needs_cleanup",
            json_object_new_boolean(true));

        /* Fix the type on the var_decl to have pass_self_by_ref=true,
         * since the expression type may be from an unresolved forward reference */
        json_object_object_del(obj_type, "pass_self_by_ref");
        json_object_object_add(obj_type, "pass_self_by_ref",
            json_object_new_boolean(true));
    }
    else
    {
        /* Value-type structs: chain temps need cleanup when methods return
         * copies (return self uses _copy). Without cleanup, the temp's
         * heap fields (strings, arrays) would leak. */
        json_object_object_add(var_decl, "cleanup_kind",
            json_object_new_string("val_cleanup"));
        json_object_object_add(var_decl, "needs_cleanup",
            json_object_new_boolean(true));
    }
}

/* Extract interpolated string arguments from a call's args array into
 * preceding sn_auto_str temp variables. This prevents memory leaks when
 * interpolated strings (which call strdup) are passed directly as
 * function arguments with no variable to own the allocation. */
static void extract_interp_string_args(json_object *args, json_object *inserts)
{
    if (!args) return;
    int len = json_object_array_length(args);
    for (int i = 0; i < len; i++)
    {
        json_object *arg = json_object_array_get_idx(args, i);
        if (!arg || !json_object_is_type(arg, json_type_object)) continue;

        json_object *arg_kind = NULL;
        if (!json_object_object_get_ex(arg, "kind", &arg_kind)) continue;
        if (strcmp(json_object_get_string(arg_kind), "interpolated_string") != 0) continue;

        /* Extract into a temp variable with sn_auto_str cleanup */
        char tmp_name[64];
        snprintf(tmp_name, sizeof(tmp_name), "__chain_tmp_%d", g_chain_tmp_count++);

        json_object *str_type = json_object_new_object();
        json_object_object_add(str_type, "kind", json_object_new_string("string"));

        json_object *var_decl = json_object_new_object();
        json_object_object_add(var_decl, "kind", json_object_new_string("var_decl"));
        json_object_object_add(var_decl, "name", json_object_new_string(tmp_name));
        json_object_object_add(var_decl, "type", str_type);
        json_object_object_add(var_decl, "initializer", json_object_get(arg));
        json_object_object_add(var_decl, "needs_cleanup", json_object_new_boolean(true));
        json_object_object_add(var_decl, "cleanup_kind", json_object_new_string("str"));

        json_object_array_add(inserts, var_decl);

        /* Replace the argument with a variable reference */
        json_object *var_ref = json_object_new_object();
        json_object_object_add(var_ref, "kind", json_object_new_string("variable"));
        json_object_object_add(var_ref, "name", json_object_new_string(tmp_name));
        json_object_object_add(var_ref, "type", json_object_get(str_type));

        json_object_array_put_idx(args, i, var_ref);
    }
}

/*
 * Scan an expression for member calls whose object is not an lvalue.
 * When found, extract the object into a var_decl and collect it in `inserts`.
 * Process innermost chains first (depth-first).
 */
static void flatten_expr(json_object *expr, json_object *inserts)
{
    if (!expr || !json_object_is_type(expr, json_type_object)) return;

    json_object *kind_obj = NULL;
    if (!json_object_object_get_ex(expr, "kind", &kind_obj)) return;
    const char *kind = json_object_get_string(kind_obj);

    /* Thread spawns contain a call — recurse into it but do NOT extract
     * interpolated string args, because the temp variable's sn_auto_str
     * cleanup would free the string before the thread copies the arg. */
    if (strcmp(kind, "thread_spawn") == 0)
    {
        json_object *call = NULL;
        if (json_object_object_get_ex(expr, "call", &call))
        {
            /* Recurse into the call's callee and args WITHOUT extraction */
            json_object *callee = NULL;
            if (json_object_object_get_ex(call, "callee", &callee))
                flatten_expr(callee, inserts);
            json_object *args = NULL;
            if (json_object_object_get_ex(call, "args", &args))
            {
                int len = json_object_array_length(args);
                for (int i = 0; i < len; i++)
                    flatten_expr(json_object_array_get_idx(args, i), inserts);
            }
        }
        return;
    }

    /* For call expressions with member callee, check the callee's object */
    if (strcmp(kind, "call") == 0)
    {
        json_object *callee = NULL;
        if (json_object_object_get_ex(expr, "callee", &callee))
        {
            json_object *callee_kind_obj = NULL;
            if (json_object_object_get_ex(callee, "kind", &callee_kind_obj))
            {
                const char *callee_kind = json_object_get_string(callee_kind_obj);
                if (strcmp(callee_kind, "member") == 0)
                {
                    json_object *object = NULL;
                    if (json_object_object_get_ex(callee, "object", &object))
                    {
                        /* Recurse into the object first (flatten inner chains) */
                        flatten_expr(object, inserts);

                        /* Now check if the (possibly rewritten) object needs extraction */
                        if (needs_temp_extraction(object))
                        {
                            /* Extract: create a temp variable */
                            char tmp_name[64];
                            snprintf(tmp_name, sizeof(tmp_name), "__chain_tmp_%d", g_chain_tmp_count++);

                            /* Get the type from the object expression */
                            json_object *obj_type = NULL;
                            json_object_object_get_ex(object, "type", &obj_type);

                            /* Create var_decl statement */
                            json_object *var_decl = json_object_new_object();
                            json_object_object_add(var_decl, "kind", json_object_new_string("var_decl"));
                            json_object_object_add(var_decl, "name", json_object_new_string(tmp_name));
                            if (obj_type)
                                json_object_object_add(var_decl, "type", json_object_get(obj_type));
                            json_object_object_add(var_decl, "initializer", json_object_get(object));

                            /* Add cleanup annotations for struct temps */
                            annotate_chain_temp(var_decl, obj_type);

                            json_object_array_add(inserts, var_decl);

                            /* Replace callee.object with a variable reference */
                            json_object *var_ref = json_object_new_object();
                            json_object_object_add(var_ref, "kind", json_object_new_string("variable"));
                            json_object_object_add(var_ref, "name", json_object_new_string(tmp_name));
                            if (obj_type)
                                json_object_object_add(var_ref, "type", json_object_get(obj_type));

                            json_object_object_del(callee, "object");
                            json_object_object_add(callee, "object", var_ref);
                        }
                    }
                }
            }

            /* Recurse into callee for other patterns */
            flatten_expr(callee, inserts);
        }

        /* Extract interpolated string args, then recurse into remaining args */
        json_object *args = NULL;
        if (json_object_object_get_ex(expr, "args", &args))
        {
            extract_interp_string_args(args, inserts);
            int len = json_object_array_length(args);
            for (int i = 0; i < len; i++)
                flatten_expr(json_object_array_get_idx(args, i), inserts);
        }
        return;
    }

    /* For method_call expressions, check the object directly */
    if (strcmp(kind, "method_call") == 0)
    {
        json_object *object = NULL;
        if (json_object_object_get_ex(expr, "object", &object))
        {
            flatten_expr(object, inserts);

            if (needs_temp_extraction(object))
            {
                char tmp_name[64];
                snprintf(tmp_name, sizeof(tmp_name), "__chain_tmp_%d", g_chain_tmp_count++);

                json_object *obj_type = NULL;
                json_object_object_get_ex(object, "type", &obj_type);

                json_object *var_decl = json_object_new_object();
                json_object_object_add(var_decl, "kind", json_object_new_string("var_decl"));
                json_object_object_add(var_decl, "name", json_object_new_string(tmp_name));
                if (obj_type)
                    json_object_object_add(var_decl, "type", json_object_get(obj_type));
                json_object_object_add(var_decl, "initializer", json_object_get(object));

                /* Add cleanup annotations for struct temps */
                annotate_chain_temp(var_decl, obj_type);

                json_object_array_add(inserts, var_decl);

                json_object *var_ref = json_object_new_object();
                json_object_object_add(var_ref, "kind", json_object_new_string("variable"));
                json_object_object_add(var_ref, "name", json_object_new_string(tmp_name));
                if (obj_type)
                    json_object_object_add(var_ref, "type", json_object_get(obj_type));

                json_object_object_del(expr, "object");
                json_object_object_add(expr, "object", var_ref);
            }
        }

        json_object *args = NULL;
        if (json_object_object_get_ex(expr, "args", &args))
        {
            extract_interp_string_args(args, inserts);
            int len = json_object_array_length(args);
            for (int i = 0; i < len; i++)
                flatten_expr(json_object_array_get_idx(args, i), inserts);
        }
        return;
    }

    /* Generic recursion into all object values */
    json_object_object_foreach(expr, key, val)
    {
        (void)key;
        if (json_object_is_type(val, json_type_object))
            flatten_expr(val, inserts);
        else if (json_object_is_type(val, json_type_array))
        {
            int len = json_object_array_length(val);
            for (int i = 0; i < len; i++)
            {
                json_object *elem = json_object_array_get_idx(val, i);
                if (json_object_is_type(elem, json_type_object))
                    flatten_expr(elem, inserts);
            }
        }
    }
}

/*
 * Process a statement list (json array of statements).
 * For each statement, scan its expressions and insert any extracted
 * var_decl statements before it.
 */
/* Recurse into a body that may be an array of statements or a block object */
static void flatten_body(json_object *body);

static void flatten_stmt_list(json_object *stmts)
{
    if (!stmts || !json_object_is_type(stmts, json_type_array)) return;

    int len = json_object_array_length(stmts);

    /* Build a new array with inserts spliced in */
    json_object *new_stmts = json_object_new_array();

    for (int i = 0; i < len; i++)
    {
        json_object *stmt = json_object_array_get_idx(stmts, i);

        /* First recurse into nested statement lists within this stmt */
        if (json_object_is_type(stmt, json_type_object))
        {
            json_object *kind_obj = NULL;
            if (json_object_object_get_ex(stmt, "kind", &kind_obj))
            {
                const char *kind = json_object_get_string(kind_obj);
                json_object *body = NULL;

                if (strcmp(kind, "if") == 0)
                {
                    json_object *then_body = NULL, *else_body = NULL;
                    if (json_object_object_get_ex(stmt, "then_body", &then_body))
                        flatten_body(then_body);
                    if (json_object_object_get_ex(stmt, "else_body", &else_body))
                        flatten_body(else_body);
                    json_object *else_ifs = NULL;
                    if (json_object_object_get_ex(stmt, "else_ifs", &else_ifs))
                    {
                        int eic = json_object_array_length(else_ifs);
                        for (int ei = 0; ei < eic; ei++)
                        {
                            json_object *elif = json_object_array_get_idx(else_ifs, ei);
                            json_object *elif_body = NULL;
                            if (json_object_object_get_ex(elif, "body", &elif_body))
                                flatten_body(elif_body);
                        }
                    }
                }
                else if (strcmp(kind, "while") == 0 || strcmp(kind, "for") == 0 ||
                         strcmp(kind, "for_each") == 0 || strcmp(kind, "lock") == 0)
                {
                    if (json_object_object_get_ex(stmt, "body", &body))
                        flatten_body(body);
                }
                else if (strcmp(kind, "block") == 0)
                {
                    json_object *statements = NULL;
                    if (json_object_object_get_ex(stmt, "statements", &statements))
                        flatten_stmt_list(statements);
                }
                else if (strcmp(kind, "using") == 0)
                {
                    if (json_object_object_get_ex(stmt, "body", &body))
                        flatten_body(body);
                }
            }
        }

        /* Scan this statement's expressions for chains to flatten.
         * For compound statements (while, for, if, etc.), only scan the
         * condition/header expressions — NOT the body, which was already
         * handled by the recursive flatten_stmt_list calls above. */
        json_object *inserts = json_object_new_array();
        {
            json_object *sk = NULL;
            const char *skind = NULL;
            if (json_object_is_type(stmt, json_type_object) &&
                json_object_object_get_ex(stmt, "kind", &sk))
                skind = json_object_get_string(sk);

            if (skind && (strcmp(skind, "while") == 0))
            {
                /* Only scan the condition, not the body */
                json_object *cond = NULL;
                if (json_object_object_get_ex(stmt, "condition", &cond))
                    flatten_expr(cond, inserts);
            }
            else if (skind && (strcmp(skind, "for") == 0))
            {
                /* Scan init, condition, increment — not body */
                json_object *init = NULL, *cond = NULL, *inc = NULL;
                if (json_object_object_get_ex(stmt, "init", &init))
                    flatten_expr(init, inserts);
                if (json_object_object_get_ex(stmt, "condition", &cond))
                    flatten_expr(cond, inserts);
                if (json_object_object_get_ex(stmt, "increment", &inc))
                    flatten_expr(inc, inserts);
            }
            else if (skind && (strcmp(skind, "for_each") == 0))
            {
                /* Scan iterable, not body */
                json_object *iter = NULL;
                if (json_object_object_get_ex(stmt, "iterable", &iter))
                    flatten_expr(iter, inserts);
            }
            else if (skind && (strcmp(skind, "if") == 0))
            {
                /* Scan condition and else-if conditions, not bodies */
                json_object *cond = NULL;
                if (json_object_object_get_ex(stmt, "condition", &cond))
                    flatten_expr(cond, inserts);
                json_object *else_ifs = NULL;
                if (json_object_object_get_ex(stmt, "else_ifs", &else_ifs))
                {
                    int eic = json_object_array_length(else_ifs);
                    for (int ei = 0; ei < eic; ei++)
                    {
                        json_object *elif = json_object_array_get_idx(else_ifs, ei);
                        json_object *elif_cond = NULL;
                        if (json_object_object_get_ex(elif, "condition", &elif_cond))
                            flatten_expr(elif_cond, inserts);
                    }
                }
            }
            else if (skind && (strcmp(skind, "lock") == 0 ||
                               strcmp(skind, "block") == 0 ||
                               strcmp(skind, "using") == 0))
            {
                /* These have no header expressions to scan — body already handled */
            }
            else
            {
                /* Simple statements (var_decl, expr_stmt, return, etc.) */
                flatten_expr(stmt, inserts);
            }
        }

        /* Insert any extracted var_decls before this statement */
        int insert_count = json_object_array_length(inserts);
        for (int j = 0; j < insert_count; j++)
        {
            json_object *ins = json_object_array_get_idx(inserts, j);
            json_object_array_add(new_stmts, json_object_get(ins));
        }
        json_object_put(inserts);

        /* Add the original statement */
        json_object_array_add(new_stmts, json_object_get(stmt));
    }

    /* Replace contents: clear original array and copy from new_stmts */
    /* json-c doesn't have a swap, so we delete all from stmts and re-add */
    int old_len = json_object_array_length(stmts);
    for (int i = old_len - 1; i >= 0; i--)
        json_object_array_del_idx(stmts, i, 1);

    int new_len = json_object_array_length(new_stmts);
    for (int i = 0; i < new_len; i++)
    {
        json_object *s = json_object_array_get_idx(new_stmts, i);
        json_object_array_add(stmts, json_object_get(s));
    }
    json_object_put(new_stmts);
}

/* Recurse into a body that may be a statement array or a block object.
 * The model represents bodies inconsistently — sometimes as a JSON array
 * of statements, sometimes as a single block object with a "statements" field. */
static void flatten_body(json_object *body)
{
    if (!body) return;
    if (json_object_is_type(body, json_type_array))
    {
        flatten_stmt_list(body);
        return;
    }
    if (json_object_is_type(body, json_type_object))
    {
        json_object *kind_obj = NULL;
        if (json_object_object_get_ex(body, "kind", &kind_obj) &&
            strcmp(json_object_get_string(kind_obj), "block") == 0)
        {
            json_object *stmts = NULL;
            if (json_object_object_get_ex(body, "statements", &stmts))
                flatten_stmt_list(stmts);
        }
    }
}

void gen_model_flatten_chains(json_object *model)
{
    g_chain_tmp_count = 0;

    /* Cache the structs array for lookups during flattening */
    json_object_object_get_ex(model, "structs", &g_model_structs);

    /* Process function bodies */
    json_object *functions = NULL;
    if (json_object_object_get_ex(model, "functions", &functions))
    {
        int len = json_object_array_length(functions);
        for (int i = 0; i < len; i++)
        {
            json_object *fn = json_object_array_get_idx(functions, i);
            json_object *body = NULL;
            if (json_object_object_get_ex(fn, "body", &body))
                flatten_stmt_list(body);
        }
    }

    /* Process struct method bodies */
    json_object *structs = NULL;
    if (json_object_object_get_ex(model, "structs", &structs))
    {
        int len = json_object_array_length(structs);
        for (int i = 0; i < len; i++)
        {
            json_object *st = json_object_array_get_idx(structs, i);
            json_object *methods = NULL;
            if (json_object_object_get_ex(st, "methods", &methods))
            {
                int mlen = json_object_array_length(methods);
                for (int j = 0; j < mlen; j++)
                {
                    json_object *method = json_object_array_get_idx(methods, j);
                    json_object *body = NULL;
                    if (json_object_object_get_ex(method, "body", &body))
                        flatten_stmt_list(body);
                }
            }
        }
    }

    /* Process top-level statements */
    json_object *top_level = NULL;
    if (json_object_object_get_ex(model, "top_level_statements", &top_level))
        flatten_stmt_list(top_level);

    g_model_structs = NULL;
}
