#include "cgen/gen_model.h"
#include <string.h>
#include <stdlib.h>
#include <limits.h>


json_object *gen_model_stmt(Arena *arena, Stmt *stmt, SymbolTable *symbol_table,
                            ArithmeticMode arithmetic_mode)
{
    if (!stmt) return json_object_new_null();

    json_object *obj = json_object_new_object();

    /* Attached comments */
    if (stmt->comment_count > 0)
    {
        json_object *comments = json_object_new_array();
        for (int i = 0; i < stmt->comment_count; i++)
        {
            json_object_array_add(comments, json_object_new_string(stmt->comments[i]));
        }
        json_object_object_add(obj, "comments", comments);
    }

    switch (stmt->type)
    {
        case STMT_EXPR:
        {
            json_object_object_add(obj, "kind", json_object_new_string("expr"));
            json_object_object_add(obj, "expr",
                gen_model_expr(arena, stmt->as.expression.expression, symbol_table, arithmetic_mode));
            /* Fire-and-forget thread spawn: mark the thread def so the wrapper
             * frees __th__, and mark the statement for pthread_detach. */
            if (stmt->as.expression.expression->type == EXPR_THREAD_SPAWN)
            {
                json_object *expr_obj = NULL;
                json_object_object_get_ex(obj, "expr", &expr_obj);
                json_object *tid_obj = NULL;
                if (expr_obj && json_object_object_get_ex(expr_obj, "thread_id", &tid_obj))
                {
                    int tid = json_object_get_int(tid_obj);
                    int tlen = json_object_array_length(g_model_threads);
                    for (int ti = 0; ti < tlen; ti++)
                    {
                        json_object *tdef = json_object_array_get_idx(g_model_threads, ti);
                        json_object *tdef_id = NULL;
                        if (json_object_object_get_ex(tdef, "thread_id", &tdef_id) &&
                            json_object_get_int(tdef_id) == tid)
                        {
                            json_object_object_add(tdef, "is_fire_and_forget",
                                json_object_new_boolean(true));
                            break;
                        }
                    }
                    json_object_object_add(obj, "is_fire_and_forget_thread",
                        json_object_new_boolean(true));
                }
            }
            /* Discarded expression cleanup: when a STMT_EXPR produces a value of
             * Owned/Refcounted/Composite type that nobody captures, wrap it in a
             * cleanup block to prevent leaks (e.g. res.setHeader("X", "Y")). */
            {
                Expr *ex = stmt->as.expression.expression;
                /* Skip expressions that don't produce heap values or are side-effect-only */
                bool skip_discard = (ex->type == EXPR_ASSIGN ||
                                     ex->type == EXPR_INDEX_ASSIGN ||
                                     ex->type == EXPR_MEMBER_ASSIGN ||
                                     ex->type == EXPR_COMPOUND_ASSIGN ||
                                     ex->type == EXPR_INCREMENT ||
                                     ex->type == EXPR_DECREMENT ||
                                     ex->type == EXPR_THREAD_SPAWN ||
                                     ex->type == EXPR_THREAD_SYNC ||
                                     ex->type == EXPR_THREAD_DETACH);
                /* Skip array built-in method calls (push, pop, remove, insert, etc.)
                 * — their C macros may return void even if type checker infers a type */
                if (!skip_discard && ex->type == EXPR_CALL && ex->as.call.callee &&
                    ex->as.call.callee->type == EXPR_MEMBER)
                {
                    Expr *obj = ex->as.call.callee->as.member.object;
                    if (obj && obj->expr_type && obj->expr_type->kind == TYPE_ARRAY)
                        skip_discard = true;
                }
                if (!skip_discard && ex->expr_type)
                {
                    TypeCategory cat = gen_model_type_category(ex->expr_type);
                    switch (cat)
                    {
                        case TYPE_CAT_OWNED:
                            json_object_object_add(obj, "needs_discard_cleanup",
                                json_object_new_boolean(true));
                            if (ex->expr_type->kind == TYPE_STRING)
                                json_object_object_add(obj, "discard_kind",
                                    json_object_new_string("str"));
                            else if (ex->expr_type->kind == TYPE_ARRAY)
                                json_object_object_add(obj, "discard_kind",
                                    json_object_new_string("arr"));
                            else
                                json_object_object_add(obj, "discard_kind",
                                    json_object_new_string("fn"));
                            break;
                        case TYPE_CAT_REFCOUNTED:
                            /* Do NOT discard-cleanup refcounted types. Builder methods
                             * (setX, addY) return self — the discarded return value is
                             * an alias, not a new reference. Releasing it would free
                             * the object the caller still holds. */
                            break;
                        case TYPE_CAT_COMPOSITE:
                            json_object_object_add(obj, "needs_discard_cleanup",
                                json_object_new_boolean(true));
                            json_object_object_add(obj, "discard_kind",
                                json_object_new_string("val_cleanup"));
                            json_object_object_add(obj, "discard_type_name",
                                json_object_new_string(ex->expr_type->as.struct_type.name));
                            break;
                        default:
                            break;
                    }
                }
            }
            break;
        }

        case STMT_VAR_DECL:
        {
            json_object_object_add(obj, "kind", json_object_new_string("var_decl"));
            json_object_object_add(obj, "name",
                json_object_new_string(stmt->as.var_decl.name.start));
            json_object_object_add(obj, "type",
                gen_model_type(arena, stmt->as.var_decl.resolved_type
                                      ? stmt->as.var_decl.resolved_type
                                      : stmt->as.var_decl.type));
            json_object_object_add(obj, "mem_qual",
                json_object_new_string(gen_model_mem_qual_str(stmt->as.var_decl.mem_qualifier)));
            json_object_object_add(obj, "sync_mod",
                json_object_new_string(gen_model_sync_mod_str(stmt->as.var_decl.sync_modifier)));
            json_object_object_add(obj, "is_static",
                json_object_new_boolean(stmt->as.var_decl.is_static));
            /* Check if this variable is captured by a lambda (needs promoted storage) */
            {
                const char *vname = stmt->as.var_decl.name.start;
                bool captured = false;
                for (int ci = 0; ci < g_captured_var_count; ci++) {
                    if (strcmp(g_captured_vars[ci], vname) == 0) { captured = true; break; }
                }
                if (captured)
                    json_object_object_add(obj, "is_captured", json_object_new_boolean(true));
            }
            /* Compute cleanup annotations for c-min codegen */
            Type *vtype = stmt->as.var_decl.resolved_type
                          ? stmt->as.var_decl.resolved_type
                          : stmt->as.var_decl.type;
            {
                if (vtype)
                {
                    const char *cleanup_kind = gen_model_var_cleanup_kind(vtype, g_suppress_local_cleanup);
                    bool needs_cleanup = (strcmp(cleanup_kind, "none") != 0);
                    json_object_object_add(obj, "needs_cleanup",
                        json_object_new_boolean(needs_cleanup));
                    json_object_object_add(obj, "cleanup_kind",
                        json_object_new_string(cleanup_kind));
                }
            }
            /* Check if this variable is assigned a thread_spawn elsewhere (conditional spawn).
             * Skip if the initializer is already a thread_spawn (handled by is_thread_handle). */
            {
                bool init_is_spawn = (stmt->as.var_decl.initializer &&
                    stmt->as.var_decl.initializer->type == EXPR_THREAD_SPAWN);
                if (!init_is_spawn)
                {
                    const char *vname = stmt->as.var_decl.name.start;
                    for (int ti = 0; ti < g_thread_handle_var_count; ti++)
                    {
                        if (strcmp(g_thread_handle_vars[ti], vname) == 0)
                        {
                            json_object_object_add(obj, "needs_thread_handle", json_object_new_boolean(true));
                            break;
                        }
                    }
                }
            }
            if (stmt->as.var_decl.initializer)
            {
                json_object_object_add(obj, "initializer",
                    gen_model_expr(arena, stmt->as.var_decl.initializer, symbol_table, arithmetic_mode));
                /* Thread spawn results are RtHandleV2* at the C level */
                if (stmt->as.var_decl.initializer->type == EXPR_THREAD_SPAWN)
                {
                    json_object_object_add(obj, "is_thread_handle", json_object_new_boolean(true));
                }
                /* For string vars: determine if initializer needs strdup wrapping.
                 * Literals and variable refs are borrowed — need strdup.
                 * Function calls, interpolation, concat return owned strings — no strdup.
                 * nil literals must NOT be strdup'd (strdup(NULL) crashes). */
                if (vtype && vtype->kind == TYPE_STRING)
                {
                    Expr *init = stmt->as.var_decl.initializer;
                    ExprType itype = init->type;
                    bool is_nil_literal = (itype == EXPR_LITERAL && init->as.literal.type &&
                                           init->as.literal.type->kind == TYPE_NIL);
                    bool needs_strdup = !is_nil_literal &&
                                        (itype == EXPR_LITERAL || itype == EXPR_VARIABLE ||
                                         itype == EXPR_ARRAY_ACCESS || itype == EXPR_MEMBER);
                    json_object_object_add(obj, "needs_strdup",
                        json_object_new_boolean(needs_strdup));
                }
                /* For array vars: variable/member initializer needs sn_array_copy to avoid double-free.
                 * Array literals and function calls return owned arrays — no copy needed. */
                if (vtype && vtype->kind == TYPE_ARRAY)
                {
                    Expr *init = stmt->as.var_decl.initializer;
                    ExprType itype = init->type;
                    bool needs_copy = (itype == EXPR_VARIABLE || itype == EXPR_MEMBER ||
                                       itype == EXPR_ARRAY_ACCESS);
                    if (needs_copy)
                    {
                        json_object_object_add(obj, "needs_arr_copy",
                            json_object_new_boolean(true));
                    }
                }
                /* For composite val struct vars: variable/member/array_access initializer needs deep copy */
                if (gen_model_type_category(vtype) == TYPE_CAT_COMPOSITE &&
                    (stmt->as.var_decl.initializer->type == EXPR_VARIABLE ||
                     stmt->as.var_decl.initializer->type == EXPR_MEMBER ||
                     stmt->as.var_decl.initializer->type == EXPR_ARRAY_ACCESS ||
                     stmt->as.var_decl.initializer->type == EXPR_MEMBER_ACCESS))
                {
                    json_object_object_add(obj, "needs_val_copy",
                        json_object_new_boolean(true));
                }
                /* For refcounted struct (as ref) vars: variable/member/array_access initializer
                 * needs retain to balance the sn_auto release on scope exit.
                 * Function calls and constructors return new references (rc=1) — no retain needed. */
                if (gen_model_type_category(vtype) == TYPE_CAT_REFCOUNTED &&
                    (stmt->as.var_decl.initializer->type == EXPR_VARIABLE ||
                     stmt->as.var_decl.initializer->type == EXPR_MEMBER ||
                     stmt->as.var_decl.initializer->type == EXPR_ARRAY_ACCESS ||
                     stmt->as.var_decl.initializer->type == EXPR_MEMBER_ACCESS))
                {
                    json_object_object_add(obj, "needs_retain",
                        json_object_new_boolean(true));
                }
                /* Recursive lambda self-capture: when a var_decl initializer is a lambda
                 * that captures the var being declared, mark it for post-init patching. */
                if (stmt->as.var_decl.initializer->type == EXPR_LAMBDA)
                {
                    const char *vname = stmt->as.var_decl.name.start;
                    int lid = stmt->as.var_decl.initializer->as.lambda.lambda_id;

                    /* Find this lambda in g_model_lambdas and mark self-captures */
                    int lam_count = json_object_array_length(g_model_lambdas);
                    for (int li = 0; li < lam_count; li++)
                    {
                        json_object *lam_obj = json_object_array_get_idx(g_model_lambdas, li);
                        json_object *lid_obj;
                        json_object_object_get_ex(lam_obj, "lambda_id", &lid_obj);
                        if (json_object_get_int(lid_obj) == lid)
                        {
                            json_object *caps_arr;
                            json_object_object_get_ex(lam_obj, "captures", &caps_arr);
                            int cap_count = json_object_array_length(caps_arr);
                            for (int ci = 0; ci < cap_count; ci++)
                            {
                                json_object *cap = json_object_array_get_idx(caps_arr, ci);
                                json_object *name_obj;
                                json_object_object_get_ex(cap, "name", &name_obj);
                                if (strcmp(json_object_get_string(name_obj), vname) == 0)
                                {
                                    json_object_object_add(cap, "is_self",
                                        json_object_new_boolean(true));
                                    json_object_object_add(obj, "has_self_capture",
                                        json_object_new_boolean(true));
                                    json_object_object_add(obj, "self_capture_lambda_id",
                                        json_object_new_int(lid));
                                    json_object_object_add(obj, "self_capture_name",
                                        json_object_new_string(vname));
                                    break;
                                }
                            }
                            break;
                        }
                    }
                    /* Also mark in the inline initializer's captures */
                    json_object *init_obj;
                    json_object_object_get_ex(obj, "initializer", &init_obj);
                    if (init_obj)
                    {
                        json_object *init_caps;
                        json_object_object_get_ex(init_obj, "captures", &init_caps);
                        if (init_caps)
                        {
                            int ic_count = json_object_array_length(init_caps);
                            for (int ci = 0; ci < ic_count; ci++)
                            {
                                json_object *cap = json_object_array_get_idx(init_caps, ci);
                                json_object *name_obj;
                                json_object_object_get_ex(cap, "name", &name_obj);
                                if (strcmp(json_object_get_string(name_obj), vname) == 0)
                                {
                                    json_object_object_add(cap, "is_self",
                                        json_object_new_boolean(true));
                                    break;
                                }
                            }
                        }
                    }
                    /* If the lambda has ref captures, use closure-specific cleanup
                     * instead of generic sn_auto_ptr so captured heap vars are freed. */
                    {
                        json_object *init_obj;
                        json_object_object_get_ex(obj, "initializer", &init_obj);
                        if (init_obj)
                        {
                            json_object *ref_caps_obj;
                            if (json_object_object_get_ex(init_obj, "has_ref_captures", &ref_caps_obj) &&
                                json_object_get_boolean(ref_caps_obj))
                            {
                                json_object_object_add(obj, "cleanup_kind",
                                    json_object_new_string("closure"));
                                json_object_object_add(obj, "closure_lambda_id",
                                    json_object_new_int(lid));
                                /* Register for return-site cleanup activation */
                                if (g_closure_var_count < MAX_CLOSURE_VAR_MAP)
                                {
                                    int nlen = stmt->as.var_decl.name.length;
                                    char *ncopy = arena_alloc(arena, nlen + 1);
                                    strncpy(ncopy, stmt->as.var_decl.name.start, nlen);
                                    ncopy[nlen] = '\0';
                                    g_closure_var_names[g_closure_var_count] = ncopy;
                                    g_closure_var_lambda_ids[g_closure_var_count] = lid;
                                    g_closure_var_count++;
                                }
                            }
                        }
                    }
                }
            }
            break;
        }

        case STMT_RETURN:
        {
            json_object_object_add(obj, "kind", json_object_new_string("return"));
            if (stmt->as.return_stmt.value)
            {
                /* If the return value is a void expression, flag it so the template
                 * can emit the expression as a statement instead of return expr; */
                if (stmt->as.return_stmt.value->expr_type &&
                    stmt->as.return_stmt.value->expr_type->kind == TYPE_VOID)
                {
                    json_object_object_add(obj, "is_void_return",
                        json_object_new_boolean(true));
                }
                json_object_object_add(obj, "value",
                    gen_model_expr(arena, stmt->as.return_stmt.value, symbol_table, arithmetic_mode));
                /* Ownership transfer: if returning a local variable of a type that needs cleanup,
                 * the local must be nulled out to prevent the cleanup attribute from freeing it.
                 * Exception: 'return self' in methods — self is a pointer parameter, not a local.
                 * memset would zero the caller's struct through the pointer, destroying data
                 * when the return value is discarded (e.g., chaining: obj.method(x)) */
                Expr *rv = stmt->as.return_stmt.value;
                bool is_return_self = (rv->type == EXPR_VARIABLE &&
                    rv->as.variable.name.length == 4 &&
                    strncmp(rv->as.variable.name.start, "self", 4) == 0);
                if (is_return_self)
                {
                    json_object_object_add(obj, "is_return_self",
                        json_object_new_boolean(true));
                    if (rv->expr_type)
                        json_object_object_add(obj, "return_self_type",
                            gen_model_type(arena, rv->expr_type));
                }
                if (!is_return_self && rv->type == EXPR_VARIABLE && rv->expr_type)
                {
                    Type *rt = rv->expr_type;
                    bool is_owned = false;
                    const char *transfer_kind = "none";
                    if (rt->kind == TYPE_STRING)
                    {
                        is_owned = true;
                        transfer_kind = "null_ptr";
                    }
                    else if (rt->kind == TYPE_ARRAY)
                    {
                        is_owned = true;
                        transfer_kind = "null_ptr";
                    }
                    else if (rt->kind == TYPE_FUNCTION)
                    {
                        is_owned = true;
                        transfer_kind = "null_ptr";
                        /* Null out captured variables to prevent sn_auto_capture
                         * from freeing them — the closure takes ownership.
                         * Also activate the closure's __cleanup__ function so
                         * the caller's sn_auto_fn can free captures.
                         * Only do this in regular functions, not inside lambdas. */
                        if (!g_in_lambda_body && g_captured_var_count > 0)
                        {
                            json_object *cap_vars = json_object_new_array();
                            for (int ci = 0; ci < g_captured_var_count; ci++)
                            {
                                json_object_array_add(cap_vars,
                                    json_object_new_string(g_captured_vars[ci]));
                            }
                            json_object_object_add(obj, "captured_vars_to_null", cap_vars);
                            /* Find the lambda_id for this closure variable */
                            const char *vname = rv->as.variable.name.start;
                            int vlen = rv->as.variable.name.length;
                            for (int ci = 0; ci < g_closure_var_count; ci++)
                            {
                                if ((int)strlen(g_closure_var_names[ci]) == vlen &&
                                    strncmp(g_closure_var_names[ci], vname, vlen) == 0)
                                {
                                    json_object_object_add(obj, "has_closure_escape",
                                        json_object_new_boolean(true));
                                    json_object_object_add(obj, "closure_escape_lambda_id",
                                        json_object_new_int(g_closure_var_lambda_ids[ci]));
                                    break;
                                }
                            }
                        }
                    }
                    else if (rt->kind == TYPE_STRUCT)
                    {
                        if (rt->as.struct_type.pass_self_by_ref)
                        {
                            is_owned = true;
                            transfer_kind = "null_ptr";
                        }
                        /* as val with heap fields (recursively) needs memset */
                        else
                        {
                            if (gen_model_type_has_heap_fields(rt))
                            {
                                is_owned = true;
                                transfer_kind = "memset";
                            }
                        }
                    }
                    /* When returning a function parameter (not a local variable) of an
                     * owned type (str, array), the parameter is BORROWED — the callee
                     * must not transfer ownership.  Return an independent copy instead.
                     * This arises in generic passthrough functions like identity<str>
                     * where the body is literally "return x" with x being the str param.
                     *
                     * Exception: struct as-val parameters are compiled as owned locals in C
                     * (e.g., sn_auto_Counter __sn__s = __p__s), so they DO need ownership
                     * transfer (memset) when returned.  Only truly borrowed params (str, array,
                     * as-ref struct) skip the transfer. */
                    bool is_as_val_struct =
                        (rt->kind == TYPE_STRUCT && !rt->as.struct_type.pass_self_by_ref &&
                         transfer_kind && strcmp(transfer_kind, "memset") == 0);
                    if (is_owned && !is_as_val_struct && rv->type == EXPR_VARIABLE)
                    {
                        const char *vname = rv->as.variable.name.start;
                        int vlen = rv->as.variable.name.length;
                        for (int pi = 0; pi < g_all_param_count; pi++)
                        {
                            if ((int)strlen(g_all_param_names[pi]) == vlen &&
                                strncmp(g_all_param_names[pi], vname, vlen) == 0)
                            {
                                is_owned = false;
                                /* For str params: return strdup so the caller gets an owned copy. */
                                if (rt->kind == TYPE_STRING)
                                    json_object_object_add(obj, "needs_strdup",
                                                           json_object_new_boolean(true));
                                /* For arrays/functions: just return directly — the callee doesn't
                                 * own the param's array; the caller retains ownership. */
                                break;
                            }
                        }
                    }

                    if (is_owned)
                    {
                        json_object_object_add(obj, "is_ownership_transfer",
                            json_object_new_boolean(true));
                        json_object_object_add(obj, "transfer_kind",
                            json_object_new_string(transfer_kind));
                        /* Prefix transfer_var with namespace if it's a module-level variable.
                         * Static vars use canonical prefix; instance vars use namespace prefix. */
                        const char *tvar = rv->as.variable.name.start;
                        if (g_model_namespace_prefix)
                        {
                            bool found = false;
                            const char *prefix_to_use = g_model_namespace_prefix;
                            if (g_model_ns_static_var_names)
                            {
                                for (int vi = 0; vi < g_model_ns_static_var_count; vi++)
                                {
                                    if (strcmp(g_model_ns_static_var_names[vi], tvar) == 0)
                                    { found = true; prefix_to_use = g_model_canonical_prefix; break; }
                                }
                            }
                            if (!found && g_model_ns_instance_var_names)
                            {
                                for (int vi = 0; vi < g_model_ns_instance_var_count; vi++)
                                {
                                    if (strcmp(g_model_ns_instance_var_names[vi], tvar) == 0)
                                    { found = true; break; }
                                }
                            }
                            if (found && prefix_to_use)
                            {
                                char prefixed_tvar[512];
                                snprintf(prefixed_tvar, sizeof(prefixed_tvar), "%s__%s",
                                         prefix_to_use, tvar);
                                tvar = arena_alloc(arena, strlen(prefixed_tvar) + 1);
                                strcpy((char *)tvar, prefixed_tvar);
                            }
                        }
                        json_object_object_add(obj, "transfer_var",
                            json_object_new_string(tvar));
                        json_object_object_add(obj, "transfer_type",
                            gen_model_type(arena, rt));
                    }
                }
                /* Member access return: if returning a field of a LOCAL struct with
                 * sn_auto cleanup, the field will be freed by the cleanup before the
                 * caller can use it.  Copy the value to avoid use-after-free.
                 * Exclude method self and function parameters — they're borrowed. */
                else if (rv->type == EXPR_MEMBER && rv->expr_type)
                {
                    Type *rt = rv->expr_type;
                    Expr *obj_expr = rv->as.member.object;
                    bool source_has_cleanup = false;
                    if (obj_expr && obj_expr->expr_type &&
                        obj_expr->expr_type->kind == TYPE_STRUCT &&
                        !obj_expr->expr_type->as.struct_type.pass_self_by_ref &&
                        gen_model_type_has_heap_fields(obj_expr->expr_type))
                    {
                        source_has_cleanup = true;
                        /* Exclude parameters (including self) — they're borrowed */
                        if (obj_expr->type == EXPR_VARIABLE)
                        {
                            const char *vname = obj_expr->as.variable.name.start;
                            int vlen = obj_expr->as.variable.name.length;
                            /* Check against function parameters */
                            for (int pi = 0; pi < g_all_param_count; pi++)
                            {
                                if ((int)strlen(g_all_param_names[pi]) == vlen &&
                                    strncmp(g_all_param_names[pi], vname, vlen) == 0)
                                {
                                    source_has_cleanup = false;
                                    break;
                                }
                            }
                            /* Check for 'self' in methods */
                            if (vlen == 4 && strncmp(vname, "self", 4) == 0)
                                source_has_cleanup = false;
                        }
                    }
                    if (source_has_cleanup)
                    {
                        if (rt->kind == TYPE_ARRAY)
                        {
                            json_object_object_add(obj, "needs_arr_copy",
                                json_object_new_boolean(true));
                        }
                        else if (rt->kind == TYPE_STRING)
                        {
                            json_object_object_add(obj, "needs_strdup",
                                json_object_new_boolean(true));
                        }
                    }
                }
                /* Struct literal return: fields may reference local variables with cleanup.
                 * Collect those variables so the template can NULL them before returning. */
                else if (rv->type == EXPR_STRUCT_LITERAL && rv->expr_type &&
                         rv->expr_type->kind == TYPE_STRUCT)
                {
                    StructLiteralExpr *sl = &rv->as.struct_literal;
                    json_object *transfer_vars = NULL;
                    for (int fi = 0; fi < sl->field_count; fi++)
                    {
                        Expr *fv = sl->fields[fi].value;
                        if (fv && fv->type == EXPR_VARIABLE && fv->expr_type)
                        {
                            Type *ft = fv->expr_type;
                            /* Only null-out arrays (no copy in struct literal).
                             * Strings are NOT nulled: struct literal strdup's string fields,
                             * creating an independent copy — let sn_auto_str free the original. */
                            if (ft->kind == TYPE_ARRAY)
                            {
                                if (!transfer_vars) transfer_vars = json_object_new_array();
                                json_object_array_add(transfer_vars,
                                    json_object_new_string(fv->as.variable.name.start));
                            }
                        }
                    }
                    if (transfer_vars)
                    {
                        json_object_object_add(obj, "has_struct_transfer",
                            json_object_new_boolean(true));
                        json_object_object_add(obj, "struct_transfer_vars", transfer_vars);
                        json_object_object_add(obj, "struct_transfer_type",
                            gen_model_type(arena, rv->expr_type));
                    }
                }
                /* String return from non-owned source: needs strdup so caller can free.
                 * Literals, member accesses, and array accesses all borrow from their
                 * source — the returned string must be an independent copy. */
                else if (rv->expr_type && rv->expr_type->kind == TYPE_STRING &&
                         (rv->type == EXPR_LITERAL || rv->type == EXPR_MEMBER ||
                          rv->type == EXPR_ARRAY_ACCESS))
                {
                    json_object_object_add(obj, "needs_strdup",
                        json_object_new_boolean(true));
                }
            }
            /* Bare return in void main: C requires main to return int,
             * so emit return 0; instead of bare return; */
            if (!stmt->as.return_stmt.value && g_in_main_void)
            {
                json_object_object_add(obj, "is_main_void_return",
                    json_object_new_boolean(true));
            }
            break;
        }

        case STMT_BLOCK:
        {
            json_object_object_add(obj, "kind", json_object_new_string("block"));
            json_object *stmts = json_object_new_array();
            for (int i = 0; i < stmt->as.block.count; i++)
            {
                json_object_array_add(stmts,
                    gen_model_stmt(arena, stmt->as.block.statements[i], symbol_table, arithmetic_mode));
            }
            json_object_object_add(obj, "statements", stmts);
            break;
        }

        case STMT_IF:
        {
            json_object_object_add(obj, "kind", json_object_new_string("if"));
            json_object_object_add(obj, "condition",
                gen_model_expr(arena, stmt->as.if_stmt.condition, symbol_table, arithmetic_mode));
            /* Wrap non-block then_body in a synthetic block so templates can always iterate .statements */
            if (stmt->as.if_stmt.then_branch->type == STMT_BLOCK)
            {
                json_object_object_add(obj, "then_body",
                    gen_model_stmt(arena, stmt->as.if_stmt.then_branch, symbol_table, arithmetic_mode));
            }
            else
            {
                json_object *block = json_object_new_object();
                json_object_object_add(block, "kind", json_object_new_string("block"));
                json_object *stmts = json_object_new_array();
                json_object_array_add(stmts,
                    gen_model_stmt(arena, stmt->as.if_stmt.then_branch, symbol_table, arithmetic_mode));
                json_object_object_add(block, "statements", stmts);
                json_object_object_add(obj, "then_body", block);
            }
            if (stmt->as.if_stmt.else_branch)
            {
                if (stmt->as.if_stmt.else_branch->type == STMT_BLOCK)
                {
                    json_object_object_add(obj, "else_body",
                        gen_model_stmt(arena, stmt->as.if_stmt.else_branch, symbol_table, arithmetic_mode));
                }
                else
                {
                    json_object *block = json_object_new_object();
                    json_object_object_add(block, "kind", json_object_new_string("block"));
                    json_object *stmts = json_object_new_array();
                    json_object_array_add(stmts,
                        gen_model_stmt(arena, stmt->as.if_stmt.else_branch, symbol_table, arithmetic_mode));
                    json_object_object_add(block, "statements", stmts);
                    json_object_object_add(obj, "else_body", block);
                }
            }
            break;
        }

        case STMT_WHILE:
        {
            json_object_object_add(obj, "kind", json_object_new_string("while"));
            json_object_object_add(obj, "condition",
                gen_model_expr(arena, stmt->as.while_stmt.condition, symbol_table, arithmetic_mode));
            json_object_object_add(obj, "body",
                gen_model_stmt(arena, stmt->as.while_stmt.body, symbol_table, arithmetic_mode));
            break;
        }

        case STMT_FOR:
        {
            json_object_object_add(obj, "kind", json_object_new_string("for"));
            if (stmt->as.for_stmt.initializer)
            {
                json_object_object_add(obj, "init",
                    gen_model_stmt(arena, stmt->as.for_stmt.initializer, symbol_table, arithmetic_mode));
            }
            if (stmt->as.for_stmt.condition)
            {
                json_object_object_add(obj, "condition",
                    gen_model_expr(arena, stmt->as.for_stmt.condition, symbol_table, arithmetic_mode));
            }
            if (stmt->as.for_stmt.increment)
            {
                json_object_object_add(obj, "increment",
                    gen_model_expr(arena, stmt->as.for_stmt.increment, symbol_table, arithmetic_mode));
            }
            json_object_object_add(obj, "body",
                gen_model_stmt(arena, stmt->as.for_stmt.body, symbol_table, arithmetic_mode));
            break;
        }

        case STMT_FOR_EACH:
        {
            Type *iter_type = stmt->as.for_each_stmt.iterator_type;
            if (iter_type != NULL)
            {
                /* Iterator protocol: emit for_each_iter model */
                json_object_object_add(obj, "kind", json_object_new_string("for_each_iter"));
                json_object_object_add(obj, "iterator_name",
                    json_object_new_string(stmt->as.for_each_stmt.var_name.start));
                json_object_object_add(obj, "iterable",
                    gen_model_expr(arena, stmt->as.for_each_stmt.iterable, symbol_table, arithmetic_mode));
                json_object_object_add(obj, "body",
                    gen_model_stmt(arena, stmt->as.for_each_stmt.body, symbol_table, arithmetic_mode));

                /* Iterable struct type name (for __sn__<Name>_iter call) */
                Type *iterable_type = stmt->as.for_each_stmt.iterable->expr_type;
                json_object_object_add(obj, "iterable_type_name",
                    json_object_new_string(iterable_type->as.struct_type.name));
                json_object_object_add(obj, "iterable_pass_by_ref",
                    json_object_new_boolean(iterable_type->as.struct_type.pass_self_by_ref));

                /* Iterator struct type name (for __sn__<Name>_hasNext / _next calls) */
                json_object_object_add(obj, "iter_type_name",
                    json_object_new_string(iter_type->as.struct_type.name));
                json_object_object_add(obj, "iter_type",
                    gen_model_type(arena, iter_type));
                json_object_object_add(obj, "iter_pass_by_ref",
                    json_object_new_boolean(iter_type->as.struct_type.pass_self_by_ref));

                /* Element type (return type of next()) */
                Type *elem_type = stmt->as.for_each_stmt.element_type;
                json_object_object_add(obj, "element_type",
                    gen_model_type(arena, elem_type));

                /* Element cleanup kind for loop variable (next() returns owned values) */
                if (elem_type)
                {
                    const char *elem_ck = gen_model_var_cleanup_kind(elem_type, false);
                    json_object_object_add(obj, "element_cleanup_kind",
                        json_object_new_string(elem_ck));
                }
            }
            else
            {
                /* Array iteration: existing path */
                json_object_object_add(obj, "kind", json_object_new_string("for_each"));
                json_object_object_add(obj, "iterator_name",
                    json_object_new_string(stmt->as.for_each_stmt.var_name.start));
                json_object_object_add(obj, "iterable",
                    gen_model_expr(arena, stmt->as.for_each_stmt.iterable, symbol_table, arithmetic_mode));
                json_object_object_add(obj, "body",
                    gen_model_stmt(arena, stmt->as.for_each_stmt.body, symbol_table, arithmetic_mode));
                /* iterable needs cleanup if it's a temporary (not a variable/member reference) */
                Expr *iter_expr = stmt->as.for_each_stmt.iterable;
                bool iter_is_temp = (iter_expr->type != EXPR_VARIABLE &&
                                     iter_expr->type != EXPR_MEMBER &&
                                     iter_expr->type != EXPR_ARRAY_ACCESS);
                json_object_object_add(obj, "needs_iterable_cleanup",
                    json_object_new_boolean(iter_is_temp));
            }
            break;
        }

        case STMT_BREAK:
        {
            json_object_object_add(obj, "kind", json_object_new_string("break"));
            break;
        }

        case STMT_CONTINUE:
        {
            json_object_object_add(obj, "kind", json_object_new_string("continue"));
            break;
        }

        case STMT_IMPORT:
        {
            json_object_object_add(obj, "kind", json_object_new_string("import"));
            json_object_object_add(obj, "module_name",
                json_object_new_string(stmt->as.import.module_name.start));
            if (stmt->as.import.namespace)
            {
                json_object_object_add(obj, "namespace",
                    json_object_new_string(stmt->as.import.namespace->start));
            }
            json_object_object_add(obj, "also_imported_directly",
                json_object_new_boolean(stmt->as.import.also_imported_directly));
            break;
        }

        case STMT_PRAGMA:
        {
            json_object_object_add(obj, "kind", json_object_new_string("pragma"));
            const char *ptype = "unknown";
            switch (stmt->as.pragma.pragma_type)
            {
                case PRAGMA_INCLUDE: ptype = "include"; break;
                case PRAGMA_LINK:    ptype = "link"; break;
                case PRAGMA_SOURCE:  ptype = "source"; break;
                case PRAGMA_PACK:    ptype = "pack"; break;
                case PRAGMA_ALIAS:   ptype = "alias"; break;
            }
            json_object_object_add(obj, "pragma_type", json_object_new_string(ptype));
            if (stmt->as.pragma.value)
            {
                /* For source pragmas, resolve the absolute path so #include directives
                 * in codegen 2/3 can find the .sn.c file regardless of where the
                 * generated C file is placed */
                if (stmt->as.pragma.pragma_type == PRAGMA_SOURCE && stmt->token &&
                    stmt->token->filename)
                {
                    const char *prag_file = stmt->token->filename;
                    const char *val = stmt->as.pragma.value;
                    size_t vlen = strlen(val);

                    /* Strip quotes from value if present */
                    char unquoted[1024];
                    if (vlen >= 2 && val[0] == '"' && val[vlen - 1] == '"')
                    {
                        memcpy(unquoted, val + 1, vlen - 2);
                        unquoted[vlen - 2] = '\0';
                    }
                    else
                    {
                        strncpy(unquoted, val, sizeof(unquoted) - 1);
                        unquoted[sizeof(unquoted) - 1] = '\0';
                    }

                    /* Build relative path from source directory + filename */
                    const char *last_slash = strrchr(prag_file, '/');
#ifdef _WIN32
                    const char *last_bslash = strrchr(prag_file, '\\');
                    if (last_bslash && (!last_slash || last_bslash > last_slash))
                        last_slash = last_bslash;
#endif
                    char rel_path[2048];
                    if (last_slash)
                    {
                        int dir_len = (int)(last_slash - prag_file);
                        char dir_buf[1024];
                        memcpy(dir_buf, prag_file, dir_len);
                        dir_buf[dir_len] = '\0';
                        snprintf(rel_path, sizeof(rel_path), "%s/%s", dir_buf, unquoted);
                        json_object_object_add(obj, "source_dir",
                            json_object_new_string(dir_buf));
                    }
                    else
                    {
                        strncpy(rel_path, unquoted, sizeof(rel_path) - 1);
                        rel_path[sizeof(rel_path) - 1] = '\0';
                        json_object_object_add(obj, "source_dir",
                            json_object_new_string("."));
                    }

                    /* Store the bare filename — gcc_backend prepends source_dir
                     * when compiling pragma sources. Using realpath() here would
                     * make the generated C machine-dependent. */
                    char full_path[PATH_MAX + 4];
                    snprintf(full_path, sizeof(full_path), "\"%s\"", unquoted);

                    json_object_object_add(obj, "value",
                        json_object_new_string(full_path));
                }
                else
                {
                    json_object_object_add(obj, "value",
                        json_object_new_string(stmt->as.pragma.value));
                }
            }
            break;
        }

        case STMT_TYPE_DECL:
        {
            json_object_object_add(obj, "kind", json_object_new_string("type_decl"));
            json_object_object_add(obj, "name",
                json_object_new_string(stmt->as.type_decl.name.start));
            json_object_object_add(obj, "type",
                gen_model_type(arena, stmt->as.type_decl.type));
            break;
        }

        case STMT_STRUCT_DECL:
        {
            json_object_object_add(obj, "kind", json_object_new_string("struct_decl"));
            json_object_object_add(obj, "struct",
                gen_model_struct(arena, &stmt->as.struct_decl, symbol_table, arithmetic_mode));
            break;
        }

        case STMT_INTERFACE_DECL:
            /* Interfaces are compile-time only — no C output */
            break;

        case STMT_FUNCTION:
        {
            json_object_object_add(obj, "kind", json_object_new_string("function"));
            json_object_object_add(obj, "function",
                gen_model_function(arena, &stmt->as.function, symbol_table, arithmetic_mode));
            break;
        }

        case STMT_LOCK:
        {
            json_object_object_add(obj, "kind", json_object_new_string("lock"));
            json_object_object_add(obj, "lock_expr",
                gen_model_expr(arena, stmt->as.lock_stmt.lock_expr, symbol_table, arithmetic_mode));
            json_object_object_add(obj, "body",
                gen_model_stmt(arena, stmt->as.lock_stmt.body, symbol_table, arithmetic_mode));
            break;
        }

        case STMT_USING:
        {
            json_object_object_add(obj, "kind", json_object_new_string("using"));
            json_object_object_add(obj, "name",
                json_object_new_string(stmt->as.using_stmt.name.start));
            if (stmt->as.using_stmt.type)
            {
                json_object_object_add(obj, "type",
                    gen_model_type(arena, stmt->as.using_stmt.type));
            }
            json_object_object_add(obj, "initializer",
                gen_model_expr(arena, stmt->as.using_stmt.initializer, symbol_table, arithmetic_mode));
            json_object_object_add(obj, "body",
                gen_model_stmt(arena, stmt->as.using_stmt.body, symbol_table, arithmetic_mode));
            break;
        }
    }

    return obj;
}
