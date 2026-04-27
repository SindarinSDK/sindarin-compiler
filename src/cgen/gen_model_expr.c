#include "cgen/gen_model.h"
#include "cgen/ownership.h"
#include "symbol_table/symbol_table_core.h"
#include <string.h>

/* Re-escape a string value for C output.
 * The lexer already interprets escape sequences (\n → 0x0A), but the C
 * templates need the two-character escape form so that the generated C
 * string literals are valid.  Returns an arena-allocated string. */
static const char *c_escape_string(Arena *arena, const char *src)
{
    if (!src) return "";
    /* First pass: compute output length */
    size_t len = 0;
    for (const char *p = src; *p; p++) {
        switch (*p) {
            case '\n': case '\t': case '\r': case '\\': case '"':
            case '\a': case '\b': case '\f': case '\v':
                len += 2; break;
            default:
                if ((unsigned char)*p < 0x20) len += 4; /* \xHH */
                else len += 1;
                break;
        }
    }
    char *buf = arena_alloc(arena, len + 1);
    char *dst = buf;
    for (const char *p = src; *p; p++) {
        switch (*p) {
            case '\n': *dst++ = '\\'; *dst++ = 'n';  break;
            case '\t': *dst++ = '\\'; *dst++ = 't';  break;
            case '\r': *dst++ = '\\'; *dst++ = 'r';  break;
            case '\\': *dst++ = '\\'; *dst++ = '\\'; break;
            case '"':  *dst++ = '\\'; *dst++ = '"';  break;
            case '\a': *dst++ = '\\'; *dst++ = 'a';  break;
            case '\b': *dst++ = '\\'; *dst++ = 'b';  break;
            case '\f': *dst++ = '\\'; *dst++ = 'f';  break;
            case '\v': *dst++ = '\\'; *dst++ = 'v';  break;
            default:
                if ((unsigned char)*p < 0x20) {
                    *dst++ = '\\'; *dst++ = 'x';
                    *dst++ = "0123456789abcdef"[((unsigned char)*p >> 4) & 0xf];
                    *dst++ = "0123456789abcdef"[(unsigned char)*p & 0xf];
                } else {
                    *dst++ = *p;
                }
                break;
        }
    }
    *dst = '\0';
    return buf;
}

/* ============================================================================
 * Lightweight Capture Analysis for Lambda Expressions
 * ============================================================================ */

typedef struct {
    char **names;
    Type **types;
    int count;
    int capacity;
} ModelCaptures;

static void mc_init(ModelCaptures *mc) { mc->names = NULL; mc->types = NULL; mc->count = 0; mc->capacity = 0; }

static void mc_add(ModelCaptures *mc, Arena *arena, const char *name, Type *type)
{
    for (int i = 0; i < mc->count; i++)
        if (strcmp(mc->names[i], name) == 0) return;
    if (mc->count >= mc->capacity) {
        int new_cap = mc->capacity == 0 ? 4 : mc->capacity * 2;
        char **nn = arena_alloc(arena, new_cap * sizeof(char *));
        Type **nt = arena_alloc(arena, new_cap * sizeof(Type *));
        for (int i = 0; i < mc->count; i++) { nn[i] = mc->names[i]; nt[i] = mc->types[i]; }
        mc->names = nn; mc->types = nt; mc->capacity = new_cap;
    }
    mc->names[mc->count] = arena_strdup(arena, name);
    mc->types[mc->count] = type;
    mc->count++;
}

/* Hoist borrow-tmp args out of inline statement expressions.
 * When a composite struct rvalue (e.g. function call result) is passed
 * as a borrowed arg, the codegen creates a temp inside a ({...}) whose
 * lifetime ends before the call executes. This hoists those temps into
 * the call-level scope so the pointer remains valid during the call.
 *
 * Also hoists array-literal arg temps so the SnArray * (and any
 * refcounted elements) get released after the call returns. Without
 * lifting, an array literal at a call site is built inside a bare
 * statement-expression with no cleanup attribute, leaking the SnArray
 * and (for ref-bearing element types) every retained element. */
static void hoist_borrow_temps(json_object *call_obj, json_object *args, Expr *call_expr)
{
    bool has_any = false;
    bool has_fn_ref = false;
    for (int i = 0; i < (int)json_object_array_length(args); i++)
    {
        json_object *arg = json_object_array_get_idx(args, i);
        json_object *bt_flag, *al_flag, *fn_flag;
        bool is_bt = json_object_object_get_ex(arg, "is_borrow_tmp", &bt_flag) &&
                     json_object_get_boolean(bt_flag);
        bool is_al = json_object_object_get_ex(arg, "is_arr_lit_borrow", &al_flag) &&
                     json_object_get_boolean(al_flag);
        bool is_fn = json_object_object_get_ex(arg, "is_fn_ref_arg", &fn_flag) &&
                     json_object_get_boolean(fn_flag);
        if (is_bt || is_al)
        {
            char var_name[64];
            if (is_al)
                snprintf(var_name, sizeof(var_name), "__al_tmp_%d__", i);
            else
                snprintf(var_name, sizeof(var_name), "__bt_%d__", i);
            json_object_object_add(arg, "borrow_tmp_var",
                json_object_new_string(var_name));
            has_any = true;
        }
        /* Also detect lambda expressions used as call args — these
         * malloc a __Closure__ that needs to be freed after the call.
         *
         * CAVEAT: if the callee returns a struct, the closure may be
         * stored in a struct field (ownership transfer).  In that case,
         * sn_auto_fn would free the closure while the struct still
         * references it.  Only hoist when the callee returns a
         * non-struct type (it only borrows the closure). */
        json_object *kind_obj = NULL;
        bool is_lambda = false;
        if (json_object_object_get_ex(arg, "kind", &kind_obj))
            is_lambda = strcmp(json_object_get_string(kind_obj), "lambda") == 0;

        bool callee_may_store = false;
        if (call_expr && call_expr->type == EXPR_CALL &&
            call_expr->as.call.callee && call_expr->as.call.callee->expr_type &&
            call_expr->as.call.callee->expr_type->kind == TYPE_FUNCTION)
        {
            Type *ret = call_expr->as.call.callee->expr_type->as.function.return_type;
            if (ret && ret->kind == TYPE_STRUCT)
                callee_may_store = true;
        }
        if ((is_fn || is_lambda) && !callee_may_store)
        {
            char var_name[64];
            snprintf(var_name, sizeof(var_name), "__fn_tmp_%d__", i);
            json_object_object_add(arg, "fn_ref_tmp_var",
                json_object_new_string(var_name));
            has_fn_ref = true;
        }
    }
    if (has_any || has_fn_ref)
        json_object_object_add(call_obj, "has_borrow_temps",
            json_object_new_boolean(true));
}

static bool mc_is_param(LambdaExpr *lam, const char *name)
{
    for (int i = 0; i < lam->param_count; i++) {
        if (strncmp(lam->params[i].name.start, name, lam->params[i].name.length) == 0
            && name[lam->params[i].name.length] == '\0')
            return true;
    }
    return false;
}

static bool mc_is_local(ModelCaptures *locals, const char *name)
{
    for (int i = 0; i < locals->count; i++)
        if (strcmp(locals->names[i], name) == 0) return true;
    return false;
}

/* Forward declarations for mutual recursion */
static void mc_collect_expr(Expr *expr, LambdaExpr *lam, ModelCaptures *locals, ModelCaptures *caps, Arena *arena);
static void mc_collect_stmt(Stmt *stmt, LambdaExpr *lam, ModelCaptures *locals, ModelCaptures *caps, Arena *arena);

static void mc_collect_locals(Stmt *stmt, ModelCaptures *locals, Arena *arena)
{
    if (!stmt) return;
    switch (stmt->type) {
    case STMT_VAR_DECL: {
        char name[256];
        int len = stmt->as.var_decl.name.length < 255 ? stmt->as.var_decl.name.length : 255;
        strncpy(name, stmt->as.var_decl.name.start, len);
        name[len] = '\0';
        mc_add(locals, arena, name, stmt->as.var_decl.type);
        break;
    }
    case STMT_BLOCK:
        for (int i = 0; i < stmt->as.block.count; i++)
            mc_collect_locals(stmt->as.block.statements[i], locals, arena);
        break;
    case STMT_FOR:
        if (stmt->as.for_stmt.initializer)
            mc_collect_locals(stmt->as.for_stmt.initializer, locals, arena);
        mc_collect_locals(stmt->as.for_stmt.body, locals, arena);
        break;
    case STMT_FOR_EACH: {
        char name[256];
        int len = stmt->as.for_each_stmt.var_name.length < 255 ? stmt->as.for_each_stmt.var_name.length : 255;
        strncpy(name, stmt->as.for_each_stmt.var_name.start, len);
        name[len] = '\0';
        /* For-each iterator type comes from the iterable's element type;
         * use NULL here since mc_add only needs the name for lookup */
        mc_add(locals, arena, name, NULL);
        mc_collect_locals(stmt->as.for_each_stmt.body, locals, arena);
        break;
    }
    case STMT_IF:
        mc_collect_locals(stmt->as.if_stmt.then_branch, locals, arena);
        if (stmt->as.if_stmt.else_branch)
            mc_collect_locals(stmt->as.if_stmt.else_branch, locals, arena);
        break;
    case STMT_WHILE:
        mc_collect_locals(stmt->as.while_stmt.body, locals, arena);
        break;
    default:
        break;
    }
}

static void mc_analyze(Arena *arena, LambdaExpr *lam, ModelCaptures *caps);

static void mc_collect_expr(Expr *expr, LambdaExpr *lam, ModelCaptures *locals, ModelCaptures *caps, Arena *arena)
{
    if (!expr) return;
    switch (expr->type) {
    case EXPR_VARIABLE: {
        char name[256];
        int len = expr->as.variable.name.length < 255 ? expr->as.variable.name.length : 255;
        strncpy(name, expr->as.variable.name.start, len);
        name[len] = '\0';
        if (!mc_is_param(lam, name) && !mc_is_local(locals, name)) {
            /* Skip builtins */
            if (strcmp(name, "print") == 0 || strcmp(name, "println") == 0 ||
                strcmp(name, "assert") == 0 || strcmp(name, "exit") == 0 ||
                strcmp(name, "len") == 0 || strcmp(name, "readLine") == 0) break;
            /* Skip module-level globals — they are C globals accessible by name
             * from any function and must not be copied into closure structs. */
            if (expr->as.variable.declaration_scope_depth > 0 &&
                expr->as.variable.declaration_scope_depth <= g_prescan_function_entry_depth)
                break;
            if (expr->expr_type)
                mc_add(caps, arena, name, expr->expr_type);
        }
        break;
    }
    case EXPR_BINARY:
        mc_collect_expr(expr->as.binary.left, lam, locals, caps, arena);
        mc_collect_expr(expr->as.binary.right, lam, locals, caps, arena);
        break;
    case EXPR_UNARY:
        mc_collect_expr(expr->as.unary.operand, lam, locals, caps, arena);
        break;
    case EXPR_ASSIGN:
        mc_collect_expr(expr->as.assign.value, lam, locals, caps, arena);
        /* Also capture the assignment target, unless it is a module-level global */
        {
            char name[256];
            int len = expr->as.assign.name.length < 255 ? expr->as.assign.name.length : 255;
            strncpy(name, expr->as.assign.name.start, len);
            name[len] = '\0';
            if (!mc_is_param(lam, name) && !mc_is_local(locals, name) && expr->expr_type &&
                !(expr->as.assign.lhs_scope_depth > 0 &&
                  expr->as.assign.lhs_scope_depth <= g_prescan_function_entry_depth))
                mc_add(caps, arena, name, expr->expr_type);
        }
        break;
    case EXPR_COMPOUND_ASSIGN:
        mc_collect_expr(expr->as.compound_assign.target, lam, locals, caps, arena);
        mc_collect_expr(expr->as.compound_assign.value, lam, locals, caps, arena);
        break;
    case EXPR_CALL:
        mc_collect_expr(expr->as.call.callee, lam, locals, caps, arena);
        for (int i = 0; i < expr->as.call.arg_count; i++)
            mc_collect_expr(expr->as.call.arguments[i], lam, locals, caps, arena);
        break;
    case EXPR_MEMBER:
        mc_collect_expr(expr->as.member.object, lam, locals, caps, arena);
        break;
    case EXPR_ARRAY_ACCESS:
        mc_collect_expr(expr->as.array_access.array, lam, locals, caps, arena);
        mc_collect_expr(expr->as.array_access.index, lam, locals, caps, arena);
        break;
    case EXPR_INCREMENT:
    case EXPR_DECREMENT:
        mc_collect_expr(expr->as.operand, lam, locals, caps, arena);
        break;
    case EXPR_INTERPOLATED:
        for (int i = 0; i < expr->as.interpol.part_count; i++)
            mc_collect_expr(expr->as.interpol.parts[i], lam, locals, caps, arena);
        break;
    case EXPR_LAMBDA:
    {
        /* Collect nested lambda's captures — any variable it captures that isn't
         * a param or local of the current lambda must be transitively captured. */
        ModelCaptures nested_caps;
        mc_init(&nested_caps);
        mc_analyze(arena, &expr->as.lambda, &nested_caps);
        for (int i = 0; i < nested_caps.count; i++) {
            if (!mc_is_param(lam, nested_caps.names[i]) && !mc_is_local(locals, nested_caps.names[i])) {
                mc_add(caps, arena, nested_caps.names[i], nested_caps.types[i]);
            }
        }
        break;
    }
    case EXPR_TYPEOF:
        mc_collect_expr(expr->as.typeof_expr.operand, lam, locals, caps, arena);
        break;
    default:
        break;
    }
}

static void mc_collect_stmt(Stmt *stmt, LambdaExpr *lam, ModelCaptures *locals, ModelCaptures *caps, Arena *arena)
{
    if (!stmt) return;
    switch (stmt->type) {
    case STMT_EXPR:
        mc_collect_expr(stmt->as.expression.expression, lam, locals, caps, arena);
        break;
    case STMT_VAR_DECL:
        if (stmt->as.var_decl.initializer)
            mc_collect_expr(stmt->as.var_decl.initializer, lam, locals, caps, arena);
        break;
    case STMT_RETURN:
        if (stmt->as.return_stmt.value)
            mc_collect_expr(stmt->as.return_stmt.value, lam, locals, caps, arena);
        break;
    case STMT_BLOCK:
        for (int i = 0; i < stmt->as.block.count; i++)
            mc_collect_stmt(stmt->as.block.statements[i], lam, locals, caps, arena);
        break;
    case STMT_IF:
        mc_collect_expr(stmt->as.if_stmt.condition, lam, locals, caps, arena);
        mc_collect_stmt(stmt->as.if_stmt.then_branch, lam, locals, caps, arena);
        if (stmt->as.if_stmt.else_branch)
            mc_collect_stmt(stmt->as.if_stmt.else_branch, lam, locals, caps, arena);
        break;
    case STMT_WHILE:
        mc_collect_expr(stmt->as.while_stmt.condition, lam, locals, caps, arena);
        mc_collect_stmt(stmt->as.while_stmt.body, lam, locals, caps, arena);
        break;
    case STMT_FOR:
        if (stmt->as.for_stmt.initializer)
            mc_collect_stmt(stmt->as.for_stmt.initializer, lam, locals, caps, arena);
        if (stmt->as.for_stmt.condition)
            mc_collect_expr(stmt->as.for_stmt.condition, lam, locals, caps, arena);
        if (stmt->as.for_stmt.increment)
            mc_collect_expr(stmt->as.for_stmt.increment, lam, locals, caps, arena);
        mc_collect_stmt(stmt->as.for_stmt.body, lam, locals, caps, arena);
        break;
    default:
        break;
    }
}

static void mc_analyze(Arena *arena, LambdaExpr *lam, ModelCaptures *caps)
{
    mc_init(caps);
    ModelCaptures locals;
    mc_init(&locals);

    /* Collect local variable declarations */
    if (lam->has_stmt_body) {
        for (int i = 0; i < lam->body_stmt_count; i++)
            mc_collect_locals(lam->body_stmts[i], &locals, arena);
    }

    /* Collect free variables */
    if (lam->has_stmt_body) {
        for (int i = 0; i < lam->body_stmt_count; i++)
            mc_collect_stmt(lam->body_stmts[i], lam, &locals, caps, arena);
    } else if (lam->body) {
        mc_collect_expr(lam->body, lam, &locals, caps, arena);
    }
}

static bool mc_is_primitive(Type *type)
{
    if (!type) return false;
    switch (type->kind) {
    case TYPE_INT: case TYPE_LONG: case TYPE_DOUBLE: case TYPE_BOOL:
    case TYPE_BYTE: case TYPE_CHAR:
        return true;
    default:
        return false;
    }
}

/* Decide how a captured outer variable must be copied into the closure
 * environment and how that copy must be released when the closure is
 * destroyed. Without owning copies, a closure that outlives the enclosing
 * function reads memory that the caller's sn_auto_* cleanup has already
 * freed.
 *
 * `is_ref` captures are heap-promoted boxes shared with the outer scope —
 * ownership transfers to the closure only on escape (handled separately
 * by __closure_<id>_free__). Their cap_cleanup stays "none" so the regular
 * destruction path does not double-free a box still owned by sn_auto_capture.
 *
 * Returns true when the capture introduces an owned heap copy that needs
 * release on every closure cleanup path. */
static bool mc_emit_capture_actions(json_object *cap, Type *type, bool is_ref)
{
    const char *cap_action = "assign";
    const char *cap_cleanup = "none";
    const char *struct_type_name = NULL;

    if (is_ref) {
        /* Borrowed pointer to outer's heap-promoted box; outer owns it
         * unless ownership transfers via escape. */
    } else if (type) {
        switch (type->kind) {
        case TYPE_STRING:
            cap_action = "strdup";
            cap_cleanup = "free";
            break;
        case TYPE_ARRAY:
            cap_action = "array_copy";
            cap_cleanup = "cleanup_array";
            break;
        case TYPE_STRUCT:
            if (type->as.struct_type.pass_self_by_ref) {
                cap_action = "retain";
                cap_cleanup = "release";
                struct_type_name = type->as.struct_type.name;
            } else if (gen_model_type_has_heap_fields(type)) {
                cap_action = "struct_copy";
                cap_cleanup = "struct_cleanup";
                struct_type_name = type->as.struct_type.name;
            }
            break;
        default:
            break;
        }
    }

    json_object_object_add(cap, "cap_action", json_object_new_string(cap_action));
    json_object_object_add(cap, "cap_cleanup", json_object_new_string(cap_cleanup));
    if (struct_type_name)
        json_object_object_add(cap, "cap_struct_type_name",
            json_object_new_string(struct_type_name));
    return strcmp(cap_cleanup, "none") != 0;
}

/* ============================================================================ */

static const char *binary_op_str(SnTokenType op)
{
    switch (op)
    {
        case TOKEN_PLUS:           return "add";
        case TOKEN_MINUS:          return "subtract";
        case TOKEN_STAR:           return "multiply";
        case TOKEN_SLASH:          return "divide";
        case TOKEN_MODULO:         return "modulo";
        case TOKEN_EQUAL_EQUAL:    return "eq";
        case TOKEN_BANG_EQUAL:     return "neq";
        case TOKEN_LESS:           return "lt";
        case TOKEN_LESS_EQUAL:     return "lte";
        case TOKEN_GREATER:        return "gt";
        case TOKEN_GREATER_EQUAL:  return "gte";
        case TOKEN_AND:            return "and";
        case TOKEN_OR:             return "or";
        case TOKEN_AMPERSAND:      return "bitand";
        case TOKEN_PIPE:           return "bitor";
        case TOKEN_CARET:          return "bitxor";
        case TOKEN_LSHIFT:         return "shl";
        case TOKEN_RSHIFT:         return "shr";
        default:                   return "unknown";
    }
}

static const char *unary_op_str(SnTokenType op)
{
    switch (op)
    {
        case TOKEN_MINUS: return "negate";
        case TOKEN_BANG:  return "not";
        case TOKEN_TILDE: return "bitnot";
        default:          return "unknown";
    }
}

static const char *func_mod_str(FunctionModifier fm)
{
    switch (fm)
    {
        case FUNC_DEFAULT: return "default";
        default:           return "default";
    }
}

/* Set up g_as_ref_param_names for a lambda body emission.  Lambdas share the
 * same by-pointer ABI rewrite as regular functions for composite val-struct
 * parameters (heap fields, non-native): the definition signature declares the
 * param as a pointer, and member/variable references in the body must render
 * as (*__sn__name).field via the is_captured path.  Without this setup the
 * body would emit __sn__name.field on a pointer — a C compile error.
 *
 * Saves the caller's state into the supplied slots and populates the globals
 * with the lambda's own composite val-struct parameters. */
static void push_lambda_as_ref_params(Arena *arena, LambdaExpr *lam,
                                      char ***saved_names, int *saved_count)
{
    *saved_names = g_as_ref_param_names;
    *saved_count = g_as_ref_param_count;
    g_as_ref_param_names = NULL;
    g_as_ref_param_count = 0;
    for (int i = 0; i < lam->param_count; i++)
    {
        Parameter *p = &lam->params[i];
        /* 'as ref' on an already-refcounted struct is redundant — param is a
         * single pointer, field access uses the ref-struct path. Skip. */
        bool is_as_ref = (p->mem_qualifier == MEM_AS_REF &&
                          !(p->type && p->type->kind == TYPE_STRUCT &&
                            p->type->as.struct_type.pass_self_by_ref));
        bool is_composite_val = (p->mem_qualifier == MEM_DEFAULT && !lam->is_native &&
                                 p->type && p->type->kind == TYPE_STRUCT &&
                                 !p->type->as.struct_type.pass_self_by_ref &&
                                 gen_model_type_has_heap_fields(p->type));
        if (!is_as_ref && !is_composite_val) continue;

        int nlen = p->name.length;
        char *ncopy = arena_alloc(arena, nlen + 1);
        memcpy(ncopy, p->name.start, nlen);
        ncopy[nlen] = '\0';

        if (g_as_ref_param_count % 8 == 0) {
            char **nv = arena_alloc(arena, (g_as_ref_param_count + 8) * sizeof(char *));
            for (int j = 0; j < g_as_ref_param_count; j++) nv[j] = g_as_ref_param_names[j];
            g_as_ref_param_names = nv;
        }
        g_as_ref_param_names[g_as_ref_param_count++] = ncopy;
    }
}

static void pop_lambda_as_ref_params(char **saved_names, int saved_count)
{
    g_as_ref_param_names = saved_names;
    g_as_ref_param_count = saved_count;
}

/* Predicate: does this string-typed expression produce a NEW heap allocation?
 * Uses a whitelist of known heap-producing expression types.
 * Generic EXPR_CALL with variable callee is excluded because closures/native
 * functions may return borrowed strings. Member method calls (obj.method())
 * always return owned strings per the ownership model. */
static bool is_heap_producing_string_expr(Expr *expr)
{
    if (!expr || !expr->expr_type || expr->expr_type->kind != TYPE_STRING)
        return false;
    switch (expr->type)
    {
        /* String concat (a + b) always allocates */
        case EXPR_BINARY:
            return (expr->as.binary.operator == TOKEN_PLUS);
        /* Method calls on strings (toLower, trim, replace, etc.) always return owned */
        case EXPR_METHOD_CALL:
        /* Static calls (e.g., String.fromChar) always return owned */
        case EXPR_STATIC_CALL:
        /* Interpolated strings allocate */
        case EXPR_INTERPOLATED:
            return true;
        /* All calls returning string produce owned heap strings:
         * member method calls (s.toLower(), arr.toString()),
         * named function calls, and closure/lambda calls.
         * Closures always return owned strings because body_needs_strdup
         * ensures a strdup wrapper when the body is not already heap-producing. */
        case EXPR_CALL:
            return true;
        default:
            return false;
    }
}

/* Check if an EXPR_CALL to a named function returns an owned string.
 * Closures/lambdas are excluded — they may return borrowed strings. */
static bool is_named_fn_str_call(Expr *expr, SymbolTable *symbol_table)
{
    if (!expr || !symbol_table) return false;
    if (expr->type != EXPR_CALL) return false;
    if (!expr->expr_type || expr->expr_type->kind != TYPE_STRING) return false;
    if (!expr->as.call.callee || expr->as.call.callee->type != EXPR_VARIABLE)
        return false;
    Symbol *sym = symbol_table_lookup_symbol(symbol_table,
        expr->as.call.callee->as.variable.name);
    return (sym && sym->is_function);
}

/* Wrap a bare top-level function reference into a __Closure__ at the call/
 * assignment site. fn_type must be the destination's TYPE_FUNCTION (the
 * parameter type, the field type, or the assignment target's type).
 *
 * Returns the wrapper_id appended to g_model_fn_wrappers if a wrap was
 * emitted, or -1 if the source expression is not a bare top-level function
 * reference (lambda, parameter reference, member access, call result, etc).
 *
 * Callers attach the returned id and a context-appropriate boolean flag to
 * their own JSON output:
 *   - call/static-call argument:  "is_fn_ref_arg" + "fn_wrapper_id"
 *   - struct-literal field:       "needs_closure_wrap" + "fn_wrapper_id"
 *   - member-assign value:        "needs_closure_wrap" + "fn_wrapper_id"
 *
 * The four sites used to inline this logic; consolidating prevents a fourth
 * site (EXPR_STATIC_CALL — see crash on Box.make(produce) when Box stores
 * a fn-typed field that later goes through __sn__Box_copy) from drifting
 * out of sync with the rest. */
static int maybe_emit_fn_ref_wrapper(Arena *arena, Expr *arg_expr,
                                     Type *fn_type, SymbolTable *symbol_table)
{
    if (!fn_type || fn_type->kind != TYPE_FUNCTION) return -1;
    if (!arg_expr || arg_expr->type != EXPR_VARIABLE) return -1;

    Symbol *arg_sym = symbol_table
        ? symbol_table_lookup_symbol(symbol_table, arg_expr->as.variable.name)
        : NULL;
    if (!arg_sym || !arg_sym->is_function ||
        arg_sym->kind == SYMBOL_PARAM ||
        arg_expr->as.variable.is_param_ref)
    {
        return -1;
    }

    int wrap_id = g_model_fn_wrapper_count++;
    json_object *wrapper = json_object_new_object();
    json_object_object_add(wrapper, "wrapper_id", json_object_new_int(wrap_id));
    json_object_object_add(wrapper, "target_name",
        json_object_new_string(arg_expr->as.variable.name.start));
    json_object_object_add(wrapper, "return_type",
        gen_model_type(arena, fn_type->as.function.return_type));

    json_object *wrap_params = json_object_new_array();
    bool target_is_native = fn_type->as.function.is_native;
    bool has_borrow = false;
    for (int p = 0; p < fn_type->as.function.param_count; p++)
    {
        json_object *pt = gen_model_type(arena, fn_type->as.function.param_types[p]);
        if (!target_is_native)
        {
            Type *ptype = fn_type->as.function.param_types[p];
            MemoryQualifier pmq_val = MEM_DEFAULT;
            if (fn_type->as.function.param_mem_quals)
                pmq_val = fn_type->as.function.param_mem_quals[p];
            if ((pmq_val == MEM_DEFAULT || pmq_val == MEM_AS_REF) && ptype &&
                gen_model_type_category(ptype) == TYPE_CAT_COMPOSITE)
            {
                json_object_object_add(pt, "is_borrow",
                    json_object_new_boolean(true));
                has_borrow = true;
            }
        }
        json_object_array_add(wrap_params, pt);
    }
    json_object_object_add(wrapper, "param_types", wrap_params);
    json_object_object_add(wrapper, "is_native",
        json_object_new_boolean(target_is_native));
    if (has_borrow)
        json_object_object_add(wrapper, "has_borrow_cleanup",
            json_object_new_boolean(true));

    json_object_array_add(g_model_fn_wrappers, wrapper);
    return wrap_id;
}

json_object *gen_model_expr(Arena *arena, Expr *expr, SymbolTable *symbol_table,
                            ArithmeticMode arithmetic_mode)
{
    (void)symbol_table;

    if (!expr) return json_object_new_null();

    json_object *obj = json_object_new_object();

    /* Add type info if available */
    if (expr->expr_type)
    {
        json_object_object_add(obj, "type", gen_model_type(arena, expr->expr_type));
    }

    switch (expr->type)
    {
        case EXPR_LITERAL:
        {
            json_object_object_add(obj, "kind", json_object_new_string("literal"));
            LiteralExpr *lit = &expr->as.literal;
            if (lit->type)
            {
                switch (lit->type->kind)
                {
                    case TYPE_INT:
                    case TYPE_INT32:
                    case TYPE_UINT:
                    case TYPE_UINT32:
                    case TYPE_LONG:
                        json_object_object_add(obj, "value_kind", json_object_new_string("int"));
                        json_object_object_add(obj, "value",
                            json_object_new_int64(lit->value.int_value));
                        break;
                    case TYPE_DOUBLE:
                    case TYPE_FLOAT:
                        json_object_object_add(obj, "value_kind", json_object_new_string("double"));
                        json_object_object_add(obj, "value",
                            json_object_new_double(lit->value.double_value));
                        break;
                    case TYPE_STRING:
                        json_object_object_add(obj, "value_kind", json_object_new_string("string"));
                        json_object_object_add(obj, "value",
                            json_object_new_string(c_escape_string(arena, lit->value.string_value)));
                        break;
                    case TYPE_BOOL:
                        json_object_object_add(obj, "value_kind", json_object_new_string("bool"));
                        json_object_object_add(obj, "value",
                            json_object_new_boolean(lit->value.int_value != 0));
                        break;
                    case TYPE_CHAR:
                        json_object_object_add(obj, "value_kind", json_object_new_string("char"));
                        json_object_object_add(obj, "value",
                            json_object_new_int64(lit->value.int_value));
                        break;
                    case TYPE_BYTE:
                        json_object_object_add(obj, "value_kind", json_object_new_string("byte"));
                        json_object_object_add(obj, "value",
                            json_object_new_int64(lit->value.int_value));
                        break;
                    case TYPE_NIL:
                        json_object_object_add(obj, "value_kind", json_object_new_string("nil"));
                        break;
                    default:
                        json_object_object_add(obj, "value_kind", json_object_new_string("unknown"));
                        break;
                }
            }
            break;
        }

        case EXPR_VARIABLE:
        {
            json_object_object_add(obj, "kind", json_object_new_string("variable"));
            const char *vname = expr->as.variable.name.start;

            /* When inside a namespaced import, prefix module-level variable
             * and function references so they resolve to namespace-qualified names.
             * Static vars use canonical prefix (shared); instance vars/fns use namespace prefix. */
            if (g_model_namespace_prefix)
            {
                bool found = false;
                const char *prefix_to_use = g_model_namespace_prefix;
                /* Check static vars first (use canonical prefix) */
                if (g_model_ns_static_var_names)
                {
                    for (int vi = 0; vi < g_model_ns_static_var_count; vi++)
                    {
                        if (strcmp(g_model_ns_static_var_names[vi], vname) == 0)
                        { found = true; prefix_to_use = g_model_canonical_prefix; break; }
                    }
                }
                /* Check instance vars (use namespace prefix) */
                if (!found && g_model_ns_instance_var_names)
                {
                    for (int vi = 0; vi < g_model_ns_instance_var_count; vi++)
                    {
                        if (strcmp(g_model_ns_instance_var_names[vi], vname) == 0)
                        { found = true; break; }
                    }
                }
                /* Check function names (use namespace prefix, or c_alias if available) */
                if (!found && g_model_ns_fn_names)
                {
                    for (int fi = 0; fi < g_model_ns_fn_count; fi++)
                    {
                        if (strcmp(g_model_ns_fn_names[fi], vname) == 0)
                        {
                            found = true;
                            /* If this function has a c_alias (e.g., native fn with @alias),
                             * use the alias directly instead of namespace-prefixing */
                            if (g_model_ns_fn_aliases && g_model_ns_fn_aliases[fi])
                            {
                                json_object_object_add(obj, "name",
                                    json_object_new_string(g_model_ns_fn_aliases[fi]));
                                vname = NULL; /* mark as handled */
                            }
                            break;
                        }
                    }
                }
                if (found && prefix_to_use && vname)
                {
                    char prefixed[512];
                    snprintf(prefixed, sizeof(prefixed), "%s__%s",
                             prefix_to_use, vname);
                    json_object_object_add(obj, "name", json_object_new_string(prefixed));
                    vname = NULL; /* mark as handled */
                }
            }
            if (vname)
                json_object_object_add(obj, "name", json_object_new_string(vname));

            /* Retrieve the name we stored (may be prefixed) */
            json_object *name_obj = NULL;
            json_object_object_get_ex(obj, "name", &name_obj);
            const char *stored_name = name_obj ? json_object_get_string(name_obj) : "";

            /* self inside a method is always a pointer — force arrow access */
            if (strcmp(stored_name, "self") == 0)
            {
                json_object *type_obj = NULL;
                json_object_object_get_ex(obj, "type", &type_obj);
                if (type_obj)
                    json_object_object_add(type_obj, "pass_self_by_ref",
                        json_object_new_boolean(true));
            }
            /* Check if this variable is captured (promoted to handle-based storage)
             * or is an 'as ref' parameter (a C pointer that must be dereferenced). */
            {
                bool mark_captured = false;
                for (int ci = 0; ci < g_captured_var_count; ci++) {
                    if (strcmp(g_captured_vars[ci], stored_name) == 0) {
                        mark_captured = true;
                        break;
                    }
                }
                if (!mark_captured) {
                    for (int ri = 0; ri < g_as_ref_param_count; ri++) {
                        if (strcmp(g_as_ref_param_names[ri], stored_name) == 0) {
                            mark_captured = true;
                            break;
                        }
                    }
                }
                if (mark_captured)
                    json_object_object_add(obj, "is_captured", json_object_new_boolean(true));
            }
            break;
        }

        case EXPR_ASSIGN:
        {
            json_object_object_add(obj, "kind", json_object_new_string("assign"));
            const char *aname = expr->as.assign.name.start;
            /* Prefix module-level variable assignments in namespaced imports.
             * Static vars use canonical prefix; instance vars use namespace prefix. */
            char assign_prefixed[512];
            if (g_model_namespace_prefix)
            {
                bool found = false;
                const char *prefix_to_use = g_model_namespace_prefix;
                if (g_model_ns_static_var_names)
                {
                    for (int vi = 0; vi < g_model_ns_static_var_count; vi++)
                    {
                        if (strcmp(g_model_ns_static_var_names[vi], aname) == 0)
                        { found = true; prefix_to_use = g_model_canonical_prefix; break; }
                    }
                }
                if (!found && g_model_ns_instance_var_names)
                {
                    for (int vi = 0; vi < g_model_ns_instance_var_count; vi++)
                    {
                        if (strcmp(g_model_ns_instance_var_names[vi], aname) == 0)
                        { found = true; break; }
                    }
                }
                if (found && prefix_to_use)
                {
                    snprintf(assign_prefixed, sizeof(assign_prefixed), "%s__%s",
                             prefix_to_use, aname);
                    aname = assign_prefixed;
                }
            }
            json_object_object_add(obj, "target", json_object_new_string(aname));
            json_object_object_add(obj, "value",
                gen_model_expr(arena, expr->as.assign.value, symbol_table, arithmetic_mode));
            /* Check if target is captured or is an 'as ref' param (C pointer needing *) */
            {
                bool mark_captured = false;
                for (int ci = 0; ci < g_captured_var_count; ci++) {
                    if (strcmp(g_captured_vars[ci], aname) == 0) {
                        mark_captured = true;
                        break;
                    }
                }
                if (!mark_captured) {
                    for (int ri = 0; ri < g_as_ref_param_count; ri++) {
                        if (strcmp(g_as_ref_param_names[ri], aname) == 0) {
                            mark_captured = true;
                            break;
                        }
                    }
                }
                if (mark_captured)
                    json_object_object_add(obj, "is_captured", json_object_new_boolean(true));
            }
            /* Assignment cleanup annotations for c-min codegen */
            if (expr->expr_type)
            {
                Type *atype = expr->expr_type;
                const char *assign_cleanup = "none";

                /* Issue #49: lifted member RHS already runs the keeper op
                 * inside the lift block — skip the assign-side ownership op
                 * or we'd double-strdup / double-copy. */
                bool assign_rhs_is_lifted_member = false;
                {
                    Expr *val = expr->as.assign.value;
                    if (val && val->type == EXPR_MEMBER && val->as.member.object)
                    {
                        Expr *mo = val->as.member.object;
                        Type *mot = mo->expr_type;
                        if ((mo->type == EXPR_CALL || mo->type == EXPR_STATIC_CALL) &&
                            mot && mot->kind == TYPE_STRUCT &&
                            !mot->as.struct_type.pass_self_by_ref &&
                            gen_model_type_has_heap_fields(mot))
                        {
                            assign_rhs_is_lifted_member = true;
                        }
                    }
                }

                switch (atype->kind)
                {
                    case TYPE_STRING:
                        assign_cleanup = "free_str";
                        break;
                    case TYPE_ARRAY:
                        assign_cleanup = "cleanup_arr";
                        break;
                    case TYPE_STRUCT:
                        if (atype->as.struct_type.pass_self_by_ref)
                        {
                            assign_cleanup = "release_ref";
                        }
                        else
                        {
                            /* as val: only needs cleanup if it has heap fields */
                            for (int fi = 0; fi < atype->as.struct_type.field_count; fi++)
                            {
                                Type *ft = atype->as.struct_type.fields[fi].type;
                                if (ft && (ft->kind == TYPE_STRING || ft->kind == TYPE_ARRAY ||
                                    ft->kind == TYPE_FUNCTION ||
                                    (ft->kind == TYPE_STRUCT && ft->as.struct_type.pass_self_by_ref)))
                                {
                                    assign_cleanup = "cleanup_val";
                                    break;
                                }
                            }
                        }
                        break;
                    case TYPE_FUNCTION:
                        assign_cleanup = "free_closure";
                        break;
                    default:
                        break;
                }
                json_object_object_add(obj, "assign_cleanup",
                    json_object_new_string(assign_cleanup));
                /* Unified classifier: source_is_borrow when the RHS reads
                 * through a live owner and the destination's cleanup_kind
                 * requires a fresh +1 credit. The template selects the
                 * concrete acquire (strdup / retain / val-copy / arr-copy)
                 * off assign_cleanup. */
                if (!assign_rhs_is_lifted_member &&
                    ownership_kind(expr->as.assign.value) == OWNERSHIP_BORROW &&
                    (strcmp(assign_cleanup, "free_str") == 0 ||
                     strcmp(assign_cleanup, "release_ref") == 0 ||
                     strcmp(assign_cleanup, "cleanup_val") == 0 ||
                     strcmp(assign_cleanup, "cleanup_arr") == 0))
                {
                    json_object_object_add(obj, "source_is_borrow",
                        json_object_new_boolean(true));
                }
                if (atype->kind == TYPE_STRUCT)
                    json_object_object_add(obj, "target_type",
                        gen_model_type(arena, atype));
            }
            break;
        }

        case EXPR_COMPOUND_ASSIGN:
        {
            json_object_object_add(obj, "kind", json_object_new_string("compound_assign"));
            json_object_object_add(obj, "op",
                json_object_new_string(binary_op_str(expr->as.compound_assign.operator)));
            json_object_object_add(obj, "target",
                gen_model_expr(arena, expr->as.compound_assign.target, symbol_table, arithmetic_mode));
            json_object *ca_val = gen_model_expr(arena, expr->as.compound_assign.value, symbol_table, arithmetic_mode);
            /* For string +=, mark heap-producing value for temp cleanup */
            if (is_heap_producing_string_expr(expr->as.compound_assign.value))
                json_object_object_add(ca_val, "is_str_temp",
                    json_object_new_boolean(true));
            json_object_object_add(obj, "value", ca_val);
            break;
        }

        case EXPR_BINARY:
        {
            /* Struct operator overload: emit as method_call instead of binary op */
            if (expr->as.binary.operator_method != NULL)
            {
                StructMethod *op_m = expr->as.binary.operator_method;

                /* Strip leading/trailing double-underscores from the synthetic name,
                 * e.g. "__op_eq__" → "op_eq", to match the C function suffix. */
                const char *raw_name = op_m->name;
                const char *method_name = raw_name;
                {
                    size_t rlen = strlen(raw_name);
                    if (rlen > 4 &&
                        raw_name[0] == '_' && raw_name[1] == '_' &&
                        raw_name[rlen - 1] == '_' && raw_name[rlen - 2] == '_')
                    {
                        method_name = arena_strndup(arena, raw_name + 2, rlen - 4);
                    }
                }

                /* mc_obj is the json object that receives the method_call fields.
                 * For a derived != operator we emit:
                 *   obj  = { kind:"unary", op:"!", operand: mc_obj }
                 * For a direct operator:
                 *   obj = mc_obj  (same pointer) */
                json_object *mc_obj;
                if (expr->as.binary.is_derived_operator)
                {
                    json_object_object_add(obj, "kind", json_object_new_string("unary"));
                    json_object_object_add(obj, "op", json_object_new_string("not"));
                    mc_obj = json_object_new_object();
                    if (expr->expr_type)
                        json_object_object_add(mc_obj, "type",
                            gen_model_type(arena, expr->expr_type));
                    json_object_object_add(obj, "operand", mc_obj);
                }
                else
                {
                    mc_obj = obj;
                }

                json_object_object_add(mc_obj, "kind", json_object_new_string("method_call"));
                /* For swapped operators (e.g. > derived from <), the method is called on
                 * the right operand with the left operand as argument: b.lt(a) */
                Expr *mc_object_expr = expr->as.binary.is_swapped_operator
                    ? expr->as.binary.right : expr->as.binary.left;
                Expr *mc_arg_expr = expr->as.binary.is_swapped_operator
                    ? expr->as.binary.left : expr->as.binary.right;
                json_object_object_add(mc_obj, "object",
                    gen_model_expr(arena, mc_object_expr, symbol_table, arithmetic_mode));
                json_object_object_add(mc_obj, "method_name",
                    json_object_new_string(method_name));
                json_object_object_add(mc_obj, "is_static", json_object_new_boolean(false));
                if (mc_object_expr && mc_object_expr->expr_type)
                    json_object_object_add(mc_obj, "struct_type",
                        gen_model_type(arena, mc_object_expr->expr_type));
                json_object *op_args = json_object_new_array();
                json_object_array_add(op_args,
                    gen_model_expr(arena, mc_arg_expr, symbol_table, arithmetic_mode));
                json_object_object_add(mc_obj, "args", op_args);
                break;
            }

            /* Check for chained string concat: a + b + c → str_concat_multi(3, a, b, c) */
            if (expr->as.binary.operator == TOKEN_PLUS &&
                expr->expr_type && expr->expr_type->kind == TYPE_STRING)
            {
                /* Collect all parts of the chain */
                #define MAX_CONCAT_PARTS 32
                Expr *parts[MAX_CONCAT_PARTS];
                int part_count = 0;

                /* Flatten left-associative chain: ((a + b) + c) → [a, b, c] */
                Expr *stack[MAX_CONCAT_PARTS];
                int stack_top = 0;
                stack[stack_top++] = expr;
                while (stack_top > 0)
                {
                    Expr *e = stack[--stack_top];
                    if (e->type == EXPR_BINARY && e->as.binary.operator == TOKEN_PLUS &&
                        e->expr_type && e->expr_type->kind == TYPE_STRING)
                    {
                        /* Push right first so left is processed first */
                        if (stack_top < MAX_CONCAT_PARTS - 1)
                            stack[stack_top++] = e->as.binary.right;
                        if (stack_top < MAX_CONCAT_PARTS - 1)
                            stack[stack_top++] = e->as.binary.left;
                    }
                    else
                    {
                        if (part_count < MAX_CONCAT_PARTS)
                            parts[part_count++] = e;
                    }
                }
                #undef MAX_CONCAT_PARTS

                if (part_count > 2)
                {
                    /* Emit str_concat_multi */
                    json_object_object_add(obj, "kind",
                        json_object_new_string("str_concat_multi"));
                    json_object *parts_arr = json_object_new_array();
                    for (int pi = 0; pi < part_count; pi++)
                    {
                        json_object *part = gen_model_expr(arena, parts[pi],
                            symbol_table, arithmetic_mode);
                        /* Check if this part is a temp string allocation that needs cleanup */
                        if (is_heap_producing_string_expr(parts[pi]))
                            json_object_object_add(part, "is_str_temp",
                                json_object_new_boolean(true));
                        json_object_array_add(parts_arr, part);
                    }
                    json_object_object_add(obj, "parts", parts_arr);
                    json_object_object_add(obj, "part_count",
                        json_object_new_int(part_count));
                    break;
                }
                /* Fall through for 2-part concat — check for temp operands */
            }

            json_object_object_add(obj, "kind", json_object_new_string("binary"));
            json_object_object_add(obj, "op",
                json_object_new_string(binary_op_str(expr->as.binary.operator)));
            json_object *left_obj = gen_model_expr(arena, expr->as.binary.left, symbol_table, arithmetic_mode);
            json_object *right_obj = gen_model_expr(arena, expr->as.binary.right, symbol_table, arithmetic_mode);

            /* For string binary ops, mark heap-producing operands for temp cleanup */
            {
                if (is_heap_producing_string_expr(expr->as.binary.left) ||
                    is_named_fn_str_call(expr->as.binary.left, symbol_table))
                    json_object_object_add(left_obj, "is_str_temp",
                        json_object_new_boolean(true));
                if (is_heap_producing_string_expr(expr->as.binary.right) ||
                    is_named_fn_str_call(expr->as.binary.right, symbol_table))
                    json_object_object_add(right_obj, "is_str_temp",
                        json_object_new_boolean(true));
            }
            /* For array binary ops (== / !=), mark array literal operands
             * for temp cleanup so the inline array is freed after comparison */
            {
                Expr *lhs = expr->as.binary.left;
                Expr *rhs = expr->as.binary.right;
                if (lhs && lhs->expr_type && lhs->expr_type->kind == TYPE_ARRAY &&
                    (expr->as.binary.operator == TOKEN_EQUAL_EQUAL ||
                     expr->as.binary.operator == TOKEN_BANG_EQUAL))
                {
                    if (lhs->type == EXPR_ARRAY || lhs->type == EXPR_RANGE)
                        json_object_object_add(left_obj, "is_arr_temp",
                            json_object_new_boolean(true));
                    if (rhs->type == EXPR_ARRAY || rhs->type == EXPR_RANGE)
                        json_object_object_add(right_obj, "is_arr_temp",
                            json_object_new_boolean(true));
                }
            }

            json_object_object_add(obj, "left", left_obj);
            json_object_object_add(obj, "right", right_obj);
            json_object_object_add(obj, "arithmetic_mode",
                json_object_new_string(arithmetic_mode == ARITH_CHECKED ? "checked" : "unchecked"));
            break;
        }

        case EXPR_UNARY:
        {
            json_object_object_add(obj, "kind", json_object_new_string("unary"));
            json_object_object_add(obj, "op",
                json_object_new_string(unary_op_str(expr->as.unary.operator)));
            json_object_object_add(obj, "operand",
                gen_model_expr(arena, expr->as.unary.operand, symbol_table, arithmetic_mode));
            break;
        }

        case EXPR_CALL:
        {
            /* Check for builtin functions */
            const char *builtin_name = NULL;
            bool is_len_builtin = false;
            if (expr->as.call.callee->type == EXPR_VARIABLE)
            {
                const char *name = expr->as.call.callee->as.variable.name.start;
                if (strcmp(name, "assert") == 0 || strcmp(name, "println") == 0 ||
                    strcmp(name, "print") == 0 || strcmp(name, "exit") == 0)
                {
                    builtin_name = name;
                }
                else if (strcmp(name, "len") == 0 && expr->as.call.arg_count == 1)
                {
                    is_len_builtin = true;
                }
            }

            if (is_len_builtin)
            {
                /* len(x) maps to builtin_length with object field, same as x.length */
                json_object_object_add(obj, "kind", json_object_new_string("builtin_length"));
                json_object_object_add(obj, "object",
                    gen_model_expr(arena, expr->as.call.arguments[0], symbol_table, arithmetic_mode));
                Expr *len_arg = expr->as.call.arguments[0];
                Type *lat = len_arg ? len_arg->expr_type : NULL;
                if (lat && lat->kind == TYPE_STRING &&
                    (len_arg->type == EXPR_CALL || len_arg->type == EXPR_STATIC_CALL ||
                     len_arg->type == EXPR_METHOD_CALL))
                    json_object_object_add(obj, "length_owns_str",
                        json_object_new_boolean(true));
            }
            else if (builtin_name)
            {
                char kind_buf[64];
                snprintf(kind_buf, sizeof(kind_buf), "builtin_%s", builtin_name);
                json_object_object_add(obj, "kind", json_object_new_string(kind_buf));
                json_object *args = json_object_new_array();
                for (int i = 0; i < expr->as.call.arg_count; i++)
                {
                    json_object_array_add(args,
                        gen_model_expr(arena, expr->as.call.arguments[i], symbol_table, arithmetic_mode));
                }
                hoist_borrow_temps(obj, args, expr);
                json_object_object_add(obj, "args", args);

                /* assert(cond, message): if the message arg is a heap-producing
                 * string (e.g. interpolated string), flag it so the template
                 * can wrap the call to free the message after. */
                if (strcmp(builtin_name, "assert") == 0 && expr->as.call.arg_count >= 2)
                {
                    Expr *msg = expr->as.call.arguments[1];
                    if (msg && msg->expr_type && msg->expr_type->kind == TYPE_STRING &&
                        (is_heap_producing_string_expr(msg) || msg->type == EXPR_INTERPOLATED))
                    {
                        json_object_object_add(obj, "has_heap_message",
                            json_object_new_boolean(true));
                        json_object_object_add(obj, "cond_arg",
                            json_object_get(json_object_array_get_idx(args, 0)));
                        json_object_object_add(obj, "message_arg",
                            json_object_get(json_object_array_get_idx(args, 1)));
                    }
                }
            }
            else
            {
                json_object_object_add(obj, "kind", json_object_new_string("call"));

                /* Detect namespace function calls: callee is EXPR_MEMBER where
                 * the root object is an EXPR_VARIABLE with no expr_type (namespace, not a value).
                 * Handles single-level (NS.func) and chained (NS.sub.func) module access.
                 * Rewrite as a direct call to __sn__<ns>__<sub>__<fn>() */
                bool is_namespace_call = false;
                if (expr->as.call.callee->type == EXPR_MEMBER)
                {
                    /* Walk the member chain to find the root */
                    Expr *root = expr->as.call.callee->as.member.object;
                    while (root && root->type == EXPR_MEMBER && root->expr_type == NULL)
                        root = root->as.member.object;

                    if (root && root->type == EXPR_VARIABLE && root->expr_type == NULL)
                    {
                        /* Build the combined namespace__fn name by walking the chain */
                        char ns_fn_name[512];
                        int pos = 0;

                        /* Collect member chain segments into a stack */
                        const char *segments[32];
                        int seg_lens[32];
                        int seg_count = 0;

                        /* Start with root variable name */
                        segments[seg_count] = root->as.variable.name.start;
                        seg_lens[seg_count] = root->as.variable.name.length;
                        seg_count++;

                        /* Walk from root up through intermediate members */
                        Expr *walk = expr->as.call.callee->as.member.object;
                        while (walk && walk->type == EXPR_MEMBER && walk->expr_type == NULL && seg_count < 31)
                        {
                            segments[seg_count] = walk->as.member.member_name.start;
                            seg_lens[seg_count] = walk->as.member.member_name.length;
                            seg_count++;
                            /* Check if this member's object is the root — if so, stop */
                            if (walk->as.member.object == root) break;
                            walk = walk->as.member.object;
                        }

                        /* Add the final function name */
                        Token fn_tok = expr->as.call.callee->as.member.member_name;
                        segments[seg_count] = fn_tok.start;
                        seg_lens[seg_count] = fn_tok.length;
                        seg_count++;

                        /* Join with __ separator */
                        for (int si = 0; si < seg_count && pos < 510; si++)
                        {
                            if (si > 0) { ns_fn_name[pos++] = '_'; ns_fn_name[pos++] = '_'; }
                            int copy_len = seg_lens[si];
                            if (pos + copy_len > 510) copy_len = 510 - pos;
                            memcpy(ns_fn_name + pos, segments[si], copy_len);
                            pos += copy_len;
                        }
                        ns_fn_name[pos] = '\0';

                        /* When inside a namespace context, prepend the outer
                         * namespace prefix so sub-module calls resolve correctly
                         * (e.g., level_02__createAggregate → L1__level_02__createAggregate) */
                        char final_ns_fn_name[1024];
                        const char *emit_name = ns_fn_name;
                        if (g_model_namespace_prefix)
                        {
                            snprintf(final_ns_fn_name, sizeof(final_ns_fn_name), "%s__%s",
                                     g_model_namespace_prefix, ns_fn_name);
                            emit_name = final_ns_fn_name;
                        }

                        /* Build a synthetic variable callee with the combined name */
                        json_object *callee_obj = json_object_new_object();
                        json_object_object_add(callee_obj, "kind", json_object_new_string("variable"));
                        json_object_object_add(callee_obj, "name", json_object_new_string(emit_name));
                        /* Copy the function type from the callee if available */
                        if (expr->as.call.callee->expr_type)
                            json_object_object_add(callee_obj, "type",
                                gen_model_type(arena, expr->as.call.callee->expr_type));

                        /* Propagate c_alias for namespace function calls.
                         * If the target function has @alias (e.g., native fn with @alias "sin"),
                         * look it up in the namespace's symbol list and use the alias. */
                        if (symbol_table)
                        {
                            Symbol *ns_sym = symbol_table_lookup_symbol(symbol_table, root->as.variable.name);
                            if (ns_sym && ns_sym->namespace_symbols)
                            {
                                Token fn_tok = expr->as.call.callee->as.member.member_name;
                                Symbol *fn_sym = ns_sym->namespace_symbols;
                                while (fn_sym)
                                {
                                    if (fn_sym->name.length == fn_tok.length &&
                                        memcmp(fn_sym->name.start, fn_tok.start, fn_tok.length) == 0)
                                    {
                                        if (fn_sym->c_alias)
                                        {
                                            json_object_object_add(callee_obj, "has_c_alias", json_object_new_boolean(true));
                                            json_object_object_add(callee_obj, "c_alias", json_object_new_string(fn_sym->c_alias));
                                        }
                                        break;
                                    }
                                    fn_sym = fn_sym->next;
                                }
                            }
                        }

                        json_object_object_add(obj, "callee", callee_obj);
                        is_namespace_call = true;
                    }
                }

                if (!is_namespace_call)
                {
                    json_object *callee_model = gen_model_expr(arena, expr->as.call.callee, symbol_table, arithmetic_mode);
                    json_object_object_add(obj, "callee", callee_model);

                    /* Propagate c_alias for direct calls to native functions with @alias.
                     * e.g., test_printf("hello") where test_printf has @alias "printf"
                     * should generate printf("hello") instead of test_printf("hello"). */
                    if (expr->as.call.callee->type == EXPR_VARIABLE && symbol_table)
                    {
                        Symbol *fn_sym = symbol_table_lookup_symbol(symbol_table,
                            expr->as.call.callee->as.variable.name);
                        if (fn_sym && fn_sym->c_alias)
                        {
                            json_object_object_add(callee_model, "has_c_alias", json_object_new_boolean(true));
                            json_object_object_add(callee_model, "c_alias",
                                json_object_new_string(fn_sym->c_alias));
                        }
                    }

                    /* Propagate c_alias from resolved method for member-style method calls.
                     * When callee is a member access (obj.method()), the template normally
                     * generates __sn__StructName_methodName.  If the method has @alias, use
                     * that instead.  If the method has no alias (common case) but IS a struct
                     * method (resolved_method != NULL), synthesize the canonical C name so the
                     * template uses the has_c_alias path, which correctly passes &obj as self.
                     * This also bypasses the array built-in shortcut (e.g. __sn___pop) which
                     * fires incorrectly when the object is a struct with a method named "pop". */
                    if (expr->as.call.callee->type == EXPR_MEMBER &&
                        expr->as.call.callee->as.member.resolved_method)
                    {
                        StructMethod *rsm = expr->as.call.callee->as.member.resolved_method;
                        if (rsm->c_alias)
                        {
                            json_object_object_add(callee_model, "has_c_alias", json_object_new_boolean(true));
                            json_object_object_add(callee_model, "c_alias",
                                json_object_new_string(rsm->c_alias));
                        }
                        else
                        {
                            /* Synthesize __sn__StructName_methodName as the c_alias so the
                             * template's has_c_alias path is taken (not the array shortcut). */
                            Type *rstype = expr->as.call.callee->as.member.resolved_struct_type;
                            if (rstype && rstype->kind == TYPE_STRUCT &&
                                rstype->as.struct_type.name)
                            {
                                Token mn = expr->as.call.callee->as.member.member_name;
                                char alias_buf[512];
                                snprintf(alias_buf, sizeof(alias_buf), "__sn__%s_%.*s",
                                         rstype->as.struct_type.name, mn.length, mn.start);
                                json_object_object_add(callee_model, "has_c_alias",
                                                       json_object_new_boolean(true));
                                json_object_object_add(callee_model, "c_alias",
                                                       json_object_new_string(arena_strdup(arena, alias_buf)));
                            }
                        }
                    }

                    /* For primitive type conversion methods (e.g., 27.toChar(), str.toInt()),
                     * the runtime defines type-prefixed macros: __sn__int_toChar, __sn__str_toInt.
                     * Generic methods (contains, length, push, etc.) use __sn___methodName (no prefix).
                     * We detect conversion methods by the "to" prefix and add the type alias. */
                    if (expr->as.call.callee->type == EXPR_MEMBER &&
                        !expr->as.call.callee->as.member.resolved_method)
                    {
                        Token mn = expr->as.call.callee->as.member.member_name;
                        /* Only type conversion methods need the type prefix.
                         * String methods like toUpper/toLower/toBytes use generic __sn___name form. */
                        if ((mn.length == 6 && strncmp(mn.start, "toChar", 6) == 0) ||
                            (mn.length == 5 && strncmp(mn.start, "toInt", 5) == 0) ||
                            (mn.length == 6 && strncmp(mn.start, "toLong", 6) == 0) ||
                            (mn.length == 8 && strncmp(mn.start, "toDouble", 8) == 0) ||
                            (mn.length == 7 && strncmp(mn.start, "toFloat", 7) == 0) ||
                            (mn.length == 6 && strncmp(mn.start, "toByte", 6) == 0) ||
                            (mn.length == 6 && strncmp(mn.start, "toUint", 6) == 0) ||
                            (mn.length == 6 && strncmp(mn.start, "toBool", 6) == 0))
                        {
                            Type *obj_type = expr->as.call.callee->as.member.object->expr_type;
                            if (obj_type && obj_type->kind != TYPE_STRUCT && obj_type->kind != TYPE_ARRAY)
                            {
                                const char *tp = NULL;
                                switch (obj_type->kind)
                                {
                                    case TYPE_INT:    tp = "int"; break;
                                    case TYPE_LONG:   tp = "long"; break;
                                    case TYPE_DOUBLE:  tp = "double"; break;
                                    case TYPE_FLOAT:   tp = "float"; break;
                                    case TYPE_CHAR:    tp = "char"; break;
                                    case TYPE_BOOL:    tp = "bool"; break;
                                    case TYPE_BYTE:    tp = "byte"; break;
                                    case TYPE_STRING:  tp = "str"; break;
                                    case TYPE_UINT:    tp = "uint"; break;
                                    case TYPE_UINT32:  tp = "uint32"; break;
                                    case TYPE_INT32:   tp = "int32"; break;
                                    default: break;
                                }
                                if (tp)
                                {
                                    char buf[256];
                                    snprintf(buf, sizeof(buf), "__sn__%s_%.*s", tp, mn.length, mn.start);
                                    json_object_object_add(callee_model, "has_c_alias", json_object_new_boolean(true));
                                    json_object_object_add(callee_model, "c_alias", json_object_new_string(buf));
                                    /* Primitive conversion macros accept values, not pointers */
                                    json_object_object_add(callee_model, "alias_pass_by_value", json_object_new_boolean(true));
                                }
                            }
                        }
                    }
                }

                /* Get param mem quals from callee's function type */
                MemoryQualifier *pmq = NULL;
                int pmq_count = 0;
                if (expr->as.call.callee->expr_type &&
                    expr->as.call.callee->expr_type->kind == TYPE_FUNCTION)
                {
                    pmq = expr->as.call.callee->expr_type->as.function.param_mem_quals;
                    pmq_count = expr->as.call.callee->expr_type->as.function.param_count;
                }

                /* Detect push/insert on string arrays — args need strdup for ownership */
                bool member_str_push = false;
                /* Detect push/insert on array-of-arrays — args need sn_array_copy for ownership */
                bool member_arr_push = false;
                /* Detect push/insert on composite struct arrays — lvalue args need deep copy */
                bool member_struct_push = false;
                const char *member_struct_push_name = NULL;
                /* Detect push/insert on as-ref struct arrays — lvalue args need retain */
                bool member_ref_push = false;
                const char *member_ref_push_name = NULL;
                /* Detect insert specifically — args need reordering (idx before val) for variadic macro */
                bool member_is_insert = false;
                if (expr->as.call.callee->type == EXPR_MEMBER)
                {
                    Token mn = expr->as.call.callee->as.member.member_name;
                    bool is_push = (mn.length == 4 && strncmp(mn.start, "push", 4) == 0);
                    bool is_insert = (mn.length == 6 && strncmp(mn.start, "insert", 6) == 0);
                    bool is_push_or_insert = is_push || is_insert;
                    if (is_insert && expr->as.call.arg_count == 2)
                        member_is_insert = true;
                    if (is_push_or_insert)
                    {
                        Type *obj_type = expr->as.call.callee->as.member.object->expr_type;
                        if (obj_type && obj_type->kind == TYPE_ARRAY &&
                            obj_type->as.array.element_type)
                        {
                            Type *et = obj_type->as.array.element_type;
                            if (et->kind == TYPE_STRING)
                                member_str_push = true;
                            else if (et->kind == TYPE_ARRAY)
                                member_arr_push = true;
                            else if (et->kind == TYPE_STRUCT &&
                                     !et->as.struct_type.pass_self_by_ref &&
                                     gen_model_type_category(et) == TYPE_CAT_COMPOSITE)
                            {
                                member_struct_push = true;
                                member_struct_push_name = et->as.struct_type.name;
                            }
                            else if (et->kind == TYPE_STRUCT &&
                                     et->as.struct_type.pass_self_by_ref)
                            {
                                member_ref_push = true;
                                member_ref_push_name = et->as.struct_type.name;
                            }
                        }
                    }
                }

                /* Get param types from callee's function type for fn-ref wrapping */
                Type **callee_param_types = NULL;
                int callee_param_count = 0;
                if (expr->as.call.callee->expr_type &&
                    expr->as.call.callee->expr_type->kind == TYPE_FUNCTION)
                {
                    callee_param_types = expr->as.call.callee->expr_type->as.function.param_types;
                    callee_param_count = expr->as.call.callee->expr_type->as.function.param_count;
                }

                json_object *args = json_object_new_array();
                for (int i = 0; i < expr->as.call.arg_count; i++)
                {
                    json_object *arg = gen_model_expr(arena, expr->as.call.arguments[i], symbol_table, arithmetic_mode);
                    /* Override matrix: param annotation vs arg type */
                    if (pmq && i < pmq_count)
                    {
                        Expr *arg_expr = expr->as.call.arguments[i];
                        Type *arg_type = arg_expr ? arg_expr->expr_type : NULL;
                        bool is_ref_struct = (arg_type && arg_type->kind == TYPE_STRUCT &&
                                              arg_type->as.struct_type.pass_self_by_ref);
                        if (pmq[i] == MEM_AS_REF && !is_ref_struct)
                            json_object_object_add(arg, "is_ref_arg", json_object_new_boolean(true));
                        else if (pmq[i] == MEM_AS_VAL && is_ref_struct)
                        {
                            json_object_object_add(arg, "is_copy_arg", json_object_new_boolean(true));
                            json_object_object_add(arg, "copy_type_name",
                                json_object_new_string(arg_type->as.struct_type.name));
                        }
                    }
                    /* Interface-typed params accept void *: pass &arg for val-type struct args */
                    if (callee_param_types && i < callee_param_count &&
                        callee_param_types[i] && callee_param_types[i]->kind == TYPE_INTERFACE)
                    {
                        Expr *arg_expr = expr->as.call.arguments[i];
                        Type *arg_type = arg_expr ? arg_expr->expr_type : NULL;
                        if (arg_type && arg_type->kind == TYPE_STRUCT &&
                            !arg_type->as.struct_type.pass_self_by_ref)
                        {
                            json_object *existing_ref;
                            if (!json_object_object_get_ex(arg, "is_ref_arg", &existing_ref))
                                json_object_object_add(arg, "is_ref_arg", json_object_new_boolean(true));
                        }
                    }
                    /* For composite val-type struct args (heap fields, MEM_DEFAULT):
                     * borrow by pointer for non-native callees.
                     * Lvalue args pass &arg; non-lvalue args use a statement-expression temp. */
                    {
                        json_object *existing_copy, *existing_ref;
                        if (!json_object_object_get_ex(arg, "is_copy_arg", &existing_copy) &&
                            !json_object_object_get_ex(arg, "is_ref_arg", &existing_ref))
                        {
                            Expr *arg_expr = expr->as.call.arguments[i];
                            Type *arg_type = arg_expr ? arg_expr->expr_type : NULL;
                            /* Only borrow when: callee is a known non-native function,
                             * param is MEM_DEFAULT, not a built-in array method,
                             * and we're not in a thread spawn. */
                            bool callee_is_known_fn = false;
                            bool callee_native = false;
                            Type *ctype = expr->as.call.callee->expr_type;
                            if (ctype && ctype->kind == TYPE_FUNCTION)
                            {
                                callee_is_known_fn = true;
                                callee_native = ctype->as.function.is_native;
                            }
                            /* Exclude array built-ins from borrow.  Struct method calls
                             * and fn-field calls keep borrow — the callee ABI expects
                             * composite val-struct params by pointer (see
                             * gen_model_type.c TYPE_FUNCTION pass_by_ptr annotation and
                             * gen_model_func.c g_as_ref_param_names population). */
                            if (expr->as.call.callee->type == EXPR_MEMBER)
                            {
                                Expr *callee_obj = expr->as.call.callee->as.member.object;
                                if (callee_obj && callee_obj->expr_type)
                                {
                                    Type *obj_type = callee_obj->expr_type;
                                    if (obj_type->kind == TYPE_ARRAY)
                                        callee_is_known_fn = false;
                                }
                            }
                            bool param_is_default = true;
                            if (pmq && i < pmq_count && pmq[i] != MEM_DEFAULT)
                                param_is_default = false;
                            if (arg_type && gen_model_type_category(arg_type) == TYPE_CAT_COMPOSITE)
                            {
                                bool is_lvalue = (arg_expr->type == EXPR_VARIABLE ||
                                                  arg_expr->type == EXPR_MEMBER ||
                                                  arg_expr->type == EXPR_ARRAY_ACCESS);
                                if (callee_is_known_fn && !callee_native &&
                                    param_is_default && !g_in_thread_spawn_call)
                                {
                                    /* MEM_DEFAULT + non-native: borrow by pointer */
                                    if (is_lvalue)
                                    {
                                        json_object_object_add(arg, "is_ref_arg",
                                            json_object_new_boolean(true));
                                    }
                                    else
                                    {
                                        json_object_object_add(arg, "is_borrow_tmp",
                                            json_object_new_boolean(true));
                                        json_object_object_add(arg, "borrow_type_name",
                                            json_object_new_string(arg_type->as.struct_type.name));
                                        if (gen_model_type_has_heap_fields(arg_type))
                                            json_object_object_add(arg, "borrow_needs_cleanup",
                                                json_object_new_boolean(true));
                                    }
                                }
                                else if (is_lvalue)
                                {
                                    /* Fallback: deep-copy for non-borrow cases (as val, native, etc.)
                                     * to avoid shared heap pointers leading to double-free. */
                                    json_object_object_add(arg, "is_copy_arg",
                                        json_object_new_boolean(true));
                                    json_object_object_add(arg, "copy_type_name",
                                        json_object_new_string(arg_type->as.struct_type.name));
                                    json_object_object_add(arg, "copy_needs_addr",
                                        json_object_new_boolean(true));
                                }
                            }
                        }
                    }
                    /* Detect named function references passed to function-type params.
                     * Wrap into __Closure__ for uniform calling convention. */
                    if (callee_param_types && i < callee_param_count)
                    {
                        int wrap_id = maybe_emit_fn_ref_wrapper(arena,
                            expr->as.call.arguments[i], callee_param_types[i],
                            symbol_table);
                        if (wrap_id >= 0)
                        {
                            json_object_object_add(arg, "is_fn_ref_arg",
                                json_object_new_boolean(true));
                            json_object_object_add(arg, "fn_wrapper_id",
                                json_object_new_int(wrap_id));
                        }
                    }
                    /* String array push/insert: BORROW args need strdup so slot
                     * contributes +1 while source stays live. OWNED args (call
                     * results, interpolated strings, binary ops, literal array
                     * allocations that flatten through chain_tmps) transfer the
                     * existing +1 into the slot — consumes_source suppresses
                     * the chain_tmp's sn_auto_str release. */
                    if (member_str_push && i == 0)
                    {
                        Expr *arg_expr = expr->as.call.arguments[0];
                        if (ownership_kind(arg_expr) == OWNERSHIP_BORROW)
                        {
                            json_object_object_add(arg, "source_is_borrow",
                                json_object_new_boolean(true));
                        }
                        else
                        {
                            json_object_object_add(arg, "consumes_source",
                                json_object_new_boolean(true));
                        }
                    }
                    /* Array-of-arrays push/insert: mirror the str rule — BORROW
                     * copies, OWNED transfers via consumes_source. */
                    if (member_arr_push && i == 0)
                    {
                        Expr *arg_expr = expr->as.call.arguments[0];
                        if (ownership_kind(arg_expr) == OWNERSHIP_BORROW)
                        {
                            json_object_object_add(arg, "source_is_borrow",
                                json_object_new_boolean(true));
                        }
                        else
                        {
                            json_object_object_add(arg, "consumes_source",
                                json_object_new_boolean(true));
                        }
                    }
                    /* Composite struct array push/insert: BORROW args need deep copy
                     * for ownership (the source still owns its heap fields after
                     * the push; without a copy, both would try to free them). */
                    if (member_struct_push && i == 0)
                    {
                        Expr *arg_expr = expr->as.call.arguments[0];
                        if (ownership_kind(arg_expr) == OWNERSHIP_BORROW)
                        {
                            json_object_object_add(arg, "copy_struct_name",
                                json_object_new_string(member_struct_push_name));
                            json_object_object_add(arg, "source_is_borrow",
                                json_object_new_boolean(true));
                        }
                    }
                    /* Ref struct array push/insert:
                     *   BORROW source → retain so slot contributes +1 while source stays live.
                     *   OWNED source  → no retain; slot consumes the existing +1. If the
                     *     OWNED source is a call result the chain flattener would lift it
                     *     into a __chain_tmp_N with sn_auto_T release — that release would
                     *     drop the rc out from under the slot. Mark consumes_source so the
                     *     flattener skips the release on that chain_tmp. */
                    if (member_ref_push && i == 0)
                    {
                        Expr *arg_expr = expr->as.call.arguments[0];
                        OwnershipKind k = ownership_kind(arg_expr);
                        if (k == OWNERSHIP_BORROW)
                        {
                            json_object_object_add(arg, "retain_type_name",
                                json_object_new_string(member_ref_push_name));
                            json_object_object_add(arg, "source_is_borrow",
                                json_object_new_boolean(true));
                        }
                        else
                        {
                            json_object_object_add(arg, "consumes_source",
                                json_object_new_boolean(true));
                        }
                    }
                    /* Issue #47: Array literal / range call args have no auto
                     * cleanup — the SnArray * temp leaks.  Lift into a
                     * sn_auto_arr local in the call's wrapping
                     * statement-expression so cleanup runs after the call. */
                    {
                        Expr *arg_expr = expr->as.call.arguments[i];
                        Type *arg_type = arg_expr ? arg_expr->expr_type : NULL;
                        if (arg_expr && arg_type && arg_type->kind == TYPE_ARRAY &&
                            (arg_expr->type == EXPR_ARRAY ||
                             arg_expr->type == EXPR_RANGE))
                        {
                            json_object_object_add(arg, "is_arr_lit_borrow",
                                json_object_new_boolean(true));
                        }
                    }
                    json_object_array_add(args, arg);
                }
                /* For array insert, swap args so idx comes before val.
                 * The __sn__arr_insert macro needs idx first so val (which
                 * may be a compound literal with commas) can be variadic. */
                if (member_is_insert && json_object_array_length(args) == 2)
                {
                    json_object *val = json_object_get(json_object_array_get_idx(args, 0));
                    json_object *idx = json_object_get(json_object_array_get_idx(args, 1));
                    json_object *swapped = json_object_new_array();
                    json_object_array_add(swapped, idx);
                    json_object_array_add(swapped, val);
                    args = swapped;
                }
                hoist_borrow_temps(obj, args, expr);
                json_object_object_add(obj, "args", args);
                json_object_object_add(obj, "is_tail_call",
                    json_object_new_boolean(expr->as.call.is_tail_call));

                /* Detect closure calls vs regular function calls.
                 * By model-build time, only top-level symbols remain in the symbol table.
                 * If a function-typed variable is NOT a named function, it's a closure. */
                bool is_closure = false;
                if (expr->as.call.callee->type == EXPR_VARIABLE)
                {
                    Type *ct = expr->as.call.callee->expr_type;
                    if (ct && ct->kind == TYPE_FUNCTION)
                    {
                        Symbol *sym = symbol_table_lookup_symbol(symbol_table,
                            expr->as.call.callee->as.variable.name);
                        /* If not found in symbol table, it's a local variable (closure).
                         * If found but not a function definition, also a closure. */
                        if (sym == NULL || !sym->is_function)
                            is_closure = true;
                    }
                }
                /* Array access with function element type is also a closure call
                 * (e.g., callbacks[0](5) where callbacks is fn(int):int[]) */
                if (!is_closure && expr->as.call.callee->type == EXPR_ARRAY_ACCESS)
                {
                    Type *ct = expr->as.call.callee->expr_type;
                    if (ct && ct->kind == TYPE_FUNCTION)
                        is_closure = true;
                }
                /* Detect function-typed field calls (e.g., handler.callback()).
                 * When the callee is a member access on a struct and the member name
                 * matches a function-typed FIELD (not a method), this is an indirect
                 * call through a function pointer, not a method call. */
                bool is_fn_field_call = false;
                if (!is_closure && expr->as.call.callee->type == EXPR_MEMBER)
                {
                    Expr *obj_expr = expr->as.call.callee->as.member.object;
                    Type *ct = expr->as.call.callee->expr_type;
                    if (ct && ct->kind == TYPE_FUNCTION &&
                        obj_expr && obj_expr->expr_type)
                    {
                        /* Resolve through pointer to get the struct type */
                        Type *st = obj_expr->expr_type;
                        if (st->kind == TYPE_POINTER && st->as.pointer.base_type)
                            st = st->as.pointer.base_type;
                        if (st->kind == TYPE_STRUCT)
                        {
                            /* Check if the member name matches a struct field */
                            const char *mname = expr->as.call.callee->as.member.member_name.start;
                            int mlen = expr->as.call.callee->as.member.member_name.length;
                            for (int fi = 0; fi < st->as.struct_type.field_count; fi++)
                            {
                                StructField *sf = &st->as.struct_type.fields[fi];
                                if (sf->type && sf->type->kind == TYPE_FUNCTION &&
                                    (int)strlen(sf->name) == mlen &&
                                    strncmp(sf->name, mname, mlen) == 0)
                                {
                                    is_fn_field_call = true;
                                    break;
                                }
                            }
                        }
                    }
                }
                json_object_object_add(obj, "is_closure_call",
                    json_object_new_boolean(is_closure));
                json_object_object_add(obj, "is_fn_field_call",
                    json_object_new_boolean(is_fn_field_call));

                /* Borrow-inference wrapping for native fn calls returning a
                 * ref-struct that aliases at least one ref-struct argument.
                 *
                 * Native fns may return one of their inputs unchanged (borrowed)
                 * or a freshly allocated tensor (owned). The compiler can't tell
                 * which from the signature alone. We wrap the call in a runtime
                 * snapshot+check that retains the result iff it matches one of
                 * the snapshotted ref-struct args AND that arg's __rc__ is
                 * unchanged across the call. After wrapping, the result is
                 * guaranteed +1 owned regardless of what the native code did. */
                if (!is_closure && !is_fn_field_call)
                {
                    Type *ctype2 = expr->as.call.callee->expr_type;
                    bool callee_is_native = (ctype2 && ctype2->kind == TYPE_FUNCTION &&
                                             ctype2->as.function.is_native);
                    Type *ret_type = (ctype2 && ctype2->kind == TYPE_FUNCTION)
                                     ? ctype2->as.function.return_type : NULL;
                    bool ret_is_ref_struct = (ret_type && ret_type->kind == TYPE_STRUCT &&
                                              ret_type->as.struct_type.pass_self_by_ref);
                    if (callee_is_native && ret_is_ref_struct)
                    {
                        const char *ret_name = ret_type->as.struct_type.name;
                        Type **ptypes = ctype2->as.function.param_types;
                        int pcount = ctype2->as.function.param_count;
                        json_object *check_args = json_object_new_array();
                        int n_checks = 0;
                        for (int pi = 0; pi < pcount && pi < expr->as.call.arg_count; pi++)
                        {
                            Type *pt = ptypes[pi];
                            if (pt && pt->kind == TYPE_STRUCT &&
                                pt->as.struct_type.pass_self_by_ref &&
                                pt->as.struct_type.name &&
                                ret_name &&
                                strcmp(pt->as.struct_type.name, ret_name) == 0)
                            {
                                json_object *check = json_object_new_object();
                                json_object_object_add(check, "type_name",
                                    json_object_new_string(ret_name));
                                json_object_object_add(check, "ptr_expr",
                                    gen_model_expr(arena, expr->as.call.arguments[pi],
                                                   symbol_table, arithmetic_mode));
                                json_object_array_add(check_args, check);
                                n_checks++;
                            }
                        }
                        if (n_checks > 0)
                        {
                            json_object *wrapper = json_object_new_object();
                            json_object_object_add(wrapper, "kind",
                                json_object_new_string("borrow_inferred_call"));
                            json_object_object_add(wrapper, "result_type_name",
                                json_object_new_string(ret_name));
                            json_object_object_add(wrapper, "borrow_check_args", check_args);
                            json_object_object_add(wrapper, "inner_call", obj);
                            /* Propagate type so downstream consumers see the wrapper
                             * as the same type as the original call */
                            json_object *type_copy;
                            if (json_object_object_get_ex(obj, "type", &type_copy))
                                json_object_object_add(wrapper, "type",
                                    json_object_get(type_copy));
                            obj = wrapper;
                        }
                        else
                        {
                            json_object_put(check_args);
                        }
                    }
                }
            }
            break;
        }

        case EXPR_METHOD_CALL:
        {
            json_object_object_add(obj, "kind", json_object_new_string("method_call"));
            if (expr->as.method_call.object)
            {
                json_object_object_add(obj, "object",
                    gen_model_expr(arena, expr->as.method_call.object, symbol_table, arithmetic_mode));
            }
            json_object_object_add(obj, "method_name",
                json_object_new_string(expr->as.method_call.method_name.start));
            json_object_object_add(obj, "is_static",
                json_object_new_boolean(expr->as.method_call.is_static));
            if (expr->as.method_call.struct_type)
            {
                json_object_object_add(obj, "struct_type",
                    gen_model_type(arena, expr->as.method_call.struct_type));
            }
            /* Propagate c_alias from resolved method for native method calls */
            {
                StructMethod *resolved = expr->as.method_call.method;
                if (resolved && resolved->c_alias)
                {
                    json_object_object_add(obj, "has_c_alias", json_object_new_boolean(true));
                    json_object_object_add(obj, "c_alias",
                        json_object_new_string(resolved->c_alias));
                }
            }
            json_object *args = json_object_new_array();
            {
                StructMethod *m = expr->as.method_call.method;
                for (int i = 0; i < expr->as.method_call.arg_count; i++)
                {
                    json_object *arg = gen_model_expr(arena, expr->as.method_call.args[i], symbol_table, arithmetic_mode);
                    /* Override matrix for method params */
                    if (m && i < m->param_count)
                    {
                        MemoryQualifier mq = m->params[i].mem_qualifier;
                        Expr *arg_expr = expr->as.method_call.args[i];
                        Type *arg_type = arg_expr ? arg_expr->expr_type : NULL;
                        bool is_ref_struct = (arg_type && arg_type->kind == TYPE_STRUCT &&
                                              arg_type->as.struct_type.pass_self_by_ref);
                        if (mq == MEM_AS_REF && !is_ref_struct)
                            json_object_object_add(arg, "is_ref_arg", json_object_new_boolean(true));
                        else if (mq == MEM_AS_VAL && is_ref_struct)
                        {
                            json_object_object_add(arg, "is_copy_arg", json_object_new_boolean(true));
                            json_object_object_add(arg, "copy_type_name",
                                json_object_new_string(arg_type->as.struct_type.name));
                        }
                    }
                    /* Composite val-type struct args: borrow for non-native methods */
                    {
                        json_object *existing_copy, *existing_ref;
                        if (!json_object_object_get_ex(arg, "is_copy_arg", &existing_copy) &&
                            !json_object_object_get_ex(arg, "is_ref_arg", &existing_ref))
                        {
                            Expr *arg_expr = expr->as.method_call.args[i];
                            Type *arg_type = arg_expr ? arg_expr->expr_type : NULL;
                            bool method_param_default = true;
                            if (m && i < m->param_count && m->params[i].mem_qualifier != MEM_DEFAULT)
                                method_param_default = false;
                            if (arg_type && gen_model_type_category(arg_type) == TYPE_CAT_COMPOSITE &&
                                m && !m->is_native && method_param_default)
                            {
                                bool is_lvalue = (arg_expr->type == EXPR_VARIABLE ||
                                                  arg_expr->type == EXPR_MEMBER ||
                                                  arg_expr->type == EXPR_ARRAY_ACCESS);
                                if (is_lvalue)
                                    json_object_object_add(arg, "is_ref_arg", json_object_new_boolean(true));
                                else
                                {
                                    json_object_object_add(arg, "is_borrow_tmp", json_object_new_boolean(true));
                                    json_object_object_add(arg, "borrow_type_name",
                                        json_object_new_string(arg_type->as.struct_type.name));
                                    if (gen_model_type_has_heap_fields(arg_type))
                                        json_object_object_add(arg, "borrow_needs_cleanup",
                                            json_object_new_boolean(true));
                                }
                            }
                        }
                    }
                    json_object_array_add(args, arg);
                }
            }
            json_object_object_add(obj, "args", args);
            break;
        }

        case EXPR_STATIC_CALL:
        {
            json_object_object_add(obj, "kind", json_object_new_string("static_call"));
            /* Use resolved struct type name for generics (monomorphized name) */
            {
                const char *effective_type_name = expr->as.static_call.type_name.start;
                if (expr->as.static_call.resolved_struct_type != NULL &&
                    expr->as.static_call.resolved_struct_type->kind == TYPE_STRUCT &&
                    expr->as.static_call.resolved_struct_type->as.struct_type.name != NULL)
                {
                    effective_type_name = expr->as.static_call.resolved_struct_type->as.struct_type.name;
                }
                json_object_object_add(obj, "type_name",
                    json_object_new_string(effective_type_name));
            }
            json_object_object_add(obj, "method_name",
                json_object_new_string(expr->as.static_call.method_name.start));
            /* Propagate c_alias from resolved method for native static calls */
            {
                StructMethod *m2 = expr->as.static_call.resolved_method;
                if (m2 && m2->c_alias)
                {
                    json_object_object_add(obj, "has_c_alias", json_object_new_boolean(true));
                    json_object_object_add(obj, "c_alias",
                        json_object_new_string(m2->c_alias));
                }
            }
            json_object *args = json_object_new_array();
            {
                StructMethod *m = expr->as.static_call.resolved_method;
                for (int i = 0; i < expr->as.static_call.arg_count; i++)
                {
                    json_object *arg = gen_model_expr(arena, expr->as.static_call.arguments[i], symbol_table, arithmetic_mode);
                    /* Override matrix for static method params */
                    if (m && i < m->param_count)
                    {
                        MemoryQualifier mq = m->params[i].mem_qualifier;
                        Expr *arg_expr = expr->as.static_call.arguments[i];
                        Type *arg_type = arg_expr ? arg_expr->expr_type : NULL;
                        bool is_ref_struct = (arg_type && arg_type->kind == TYPE_STRUCT &&
                                              arg_type->as.struct_type.pass_self_by_ref);
                        if (mq == MEM_AS_REF && !is_ref_struct)
                            json_object_object_add(arg, "is_ref_arg", json_object_new_boolean(true));
                        else if (mq == MEM_AS_VAL && is_ref_struct)
                        {
                            json_object_object_add(arg, "is_copy_arg", json_object_new_boolean(true));
                            json_object_object_add(arg, "copy_type_name",
                                json_object_new_string(arg_type->as.struct_type.name));
                        }
                    }
                    /* Composite val-type struct args: borrow for non-native static methods */
                    {
                        json_object *existing_copy, *existing_ref;
                        if (!json_object_object_get_ex(arg, "is_copy_arg", &existing_copy) &&
                            !json_object_object_get_ex(arg, "is_ref_arg", &existing_ref))
                        {
                            Expr *arg_expr = expr->as.static_call.arguments[i];
                            Type *arg_type = arg_expr ? arg_expr->expr_type : NULL;
                            bool static_param_default = true;
                            if (m && i < m->param_count && m->params[i].mem_qualifier != MEM_DEFAULT)
                                static_param_default = false;
                            if (arg_type && gen_model_type_category(arg_type) == TYPE_CAT_COMPOSITE &&
                                m && !m->is_native && static_param_default)
                            {
                                bool is_lvalue = (arg_expr->type == EXPR_VARIABLE ||
                                                  arg_expr->type == EXPR_MEMBER ||
                                                  arg_expr->type == EXPR_ARRAY_ACCESS);
                                if (is_lvalue)
                                    json_object_object_add(arg, "is_ref_arg", json_object_new_boolean(true));
                                else
                                {
                                    json_object_object_add(arg, "is_borrow_tmp", json_object_new_boolean(true));
                                    json_object_object_add(arg, "borrow_type_name",
                                        json_object_new_string(arg_type->as.struct_type.name));
                                    if (gen_model_type_has_heap_fields(arg_type))
                                        json_object_object_add(arg, "borrow_needs_cleanup",
                                            json_object_new_boolean(true));
                                }
                            }
                        }
                    }
                    /* Issue #47: lift array literal / range args (see EXPR_CALL above) */
                    {
                        Expr *arg_expr = expr->as.static_call.arguments[i];
                        Type *arg_type = arg_expr ? arg_expr->expr_type : NULL;
                        if (arg_expr && arg_type && arg_type->kind == TYPE_ARRAY &&
                            (arg_expr->type == EXPR_ARRAY ||
                             arg_expr->type == EXPR_RANGE))
                        {
                            json_object_object_add(arg, "is_arr_lit_borrow",
                                json_object_new_boolean(true));
                        }
                    }
                    /* Wrap bare top-level fn refs into __Closure__ when passed
                     * to a fn-typed param. Without this, the receiving static
                     * method stores a raw fn pointer in a fn-typed struct
                     * field, which crashes on the next struct copy when copy
                     * code dereferences the field as __Closure__. */
                    if (m && i < m->param_count && m->params[i].type)
                    {
                        int wrap_id = maybe_emit_fn_ref_wrapper(arena,
                            expr->as.static_call.arguments[i],
                            m->params[i].type, symbol_table);
                        if (wrap_id >= 0)
                        {
                            json_object_object_add(arg, "is_fn_ref_arg",
                                json_object_new_boolean(true));
                            json_object_object_add(arg, "fn_wrapper_id",
                                json_object_new_int(wrap_id));
                        }
                    }
                    json_object_array_add(args, arg);
                }
            }
            hoist_borrow_temps(obj, args, expr);
            json_object_object_add(obj, "args", args);
            break;
        }

        case EXPR_ARRAY:
        {
            json_object_object_add(obj, "kind", json_object_new_string("array_literal"));
            json_object *elements = json_object_new_array();
            for (int i = 0; i < expr->as.array.element_count; i++)
            {
                json_object *elem = gen_model_expr(arena, expr->as.array.elements[i], symbol_table, arithmetic_mode);
                /* Unified classifier: BORROW element source → source_is_borrow +
                 * type-name context keys the template needs for the concrete
                 * acquire call. */
                Expr *ae = expr->as.array.elements[i];
                if (ae && ae->expr_type &&
                    ownership_kind(ae) == OWNERSHIP_BORROW)
                {
                    Type *et = ae->expr_type;
                    if (et->kind == TYPE_STRING || et->kind == TYPE_ARRAY)
                    {
                        json_object_object_add(elem, "source_is_borrow",
                            json_object_new_boolean(true));
                    }
                    else if (et->kind == TYPE_STRUCT &&
                             !et->as.struct_type.pass_self_by_ref &&
                             gen_model_type_category(et) == TYPE_CAT_COMPOSITE)
                    {
                        json_object_object_add(elem, "source_is_borrow",
                            json_object_new_boolean(true));
                        json_object_object_add(elem, "copy_struct_name",
                            json_object_new_string(et->as.struct_type.name));
                    }
                    else if (et->kind == TYPE_STRUCT &&
                             et->as.struct_type.pass_self_by_ref)
                    {
                        /* Handled below via the refcounted-struct branch. */
                    }
                }
                /* Composite struct arrays: function call elements need a temp variable
                 * to avoid invalid compound literal wrapping of the return value */
                if (ae && ae->expr_type && ae->expr_type->kind == TYPE_STRUCT &&
                    !ae->expr_type->as.struct_type.pass_self_by_ref &&
                    gen_model_type_category(ae->expr_type) == TYPE_CAT_COMPOSITE &&
                    (ae->type == EXPR_CALL || ae->type == EXPR_STATIC_CALL))
                {
                    json_object_object_add(elem, "needs_struct_tmp", json_object_new_boolean(true));
                    json_object_object_add(elem, "tmp_struct_type",
                        json_object_new_string(ae->expr_type->as.struct_type.name));
                }
                /* Refcounted-struct array elements: BORROW → retain, OWNED →
                 * consumes_source so the flattener suppresses the chain_tmp's
                 * sn_auto_T release (mirrors push/insert and struct-lit). */
                if (ae && ae->expr_type && ae->expr_type->kind == TYPE_STRUCT &&
                    ae->expr_type->as.struct_type.pass_self_by_ref)
                {
                    OwnershipKind k = ownership_kind(ae);
                    if (k == OWNERSHIP_BORROW)
                    {
                        json_object_object_add(elem, "source_is_borrow",
                            json_object_new_boolean(true));
                        json_object_object_add(elem, "retain_type_name",
                            json_object_new_string(ae->expr_type->as.struct_type.name));
                    }
                    else
                    {
                        json_object_object_add(elem, "consumes_source",
                            json_object_new_boolean(true));
                    }
                }
                json_object_array_add(elements, elem);
            }
            json_object_object_add(obj, "elements", elements);
            /* Element callback names for c-min codegen */
            if (expr->expr_type && expr->expr_type->kind == TYPE_ARRAY &&
                expr->expr_type->as.array.element_type)
            {
                Type *et = expr->expr_type->as.array.element_type;
                const char *elem_release_fn = NULL;
                const char *elem_copy_fn = NULL;
                switch (et->kind)
                {
                    case TYPE_STRING:
                        elem_release_fn = "(void (*)(void *))sn_cleanup_str";
                        elem_copy_fn = "sn_copy_str";
                        break;
                    case TYPE_ARRAY:
                        elem_release_fn = "(void (*)(void *))sn_cleanup_array";
                        elem_copy_fn = "sn_copy_array";
                        break;
                    case TYPE_STRUCT:
                        if (et->as.struct_type.pass_self_by_ref)
                        {
                            char buf_r[256], buf_c[256];
                            snprintf(buf_r, sizeof(buf_r),
                                "__sn__%s_release_elem", et->as.struct_type.name);
                            snprintf(buf_c, sizeof(buf_c),
                                "__sn__%s_retain_into", et->as.struct_type.name);
                            elem_release_fn = arena_strdup(arena, buf_r);
                            elem_copy_fn = arena_strdup(arena, buf_c);
                        }
                        else
                        {
                            /* as val: check if composite (has heap fields) */
                            if (gen_model_type_category(et) == TYPE_CAT_COMPOSITE)
                            {
                                char buf_r[256], buf_c[256];
                                snprintf(buf_r, sizeof(buf_r),
                                    "__sn__%s_cleanup_elem", et->as.struct_type.name);
                                snprintf(buf_c, sizeof(buf_c),
                                    "__sn__%s_copy_into", et->as.struct_type.name);
                                elem_release_fn = arena_strdup(arena, buf_r);
                                elem_copy_fn = arena_strdup(arena, buf_c);
                            }
                        }
                        break;
                    default:
                        break;
                }
                if (elem_release_fn)
                    json_object_object_add(obj, "elem_release_fn",
                        json_object_new_string(elem_release_fn));
                if (elem_copy_fn)
                    json_object_object_add(obj, "elem_copy_fn",
                        json_object_new_string(elem_copy_fn));
            }
            break;
        }

        case EXPR_ARRAY_ACCESS:
        {
            json_object_object_add(obj, "kind", json_object_new_string("array_access"));
            json_object_object_add(obj, "array",
                gen_model_expr(arena, expr->as.array_access.array, symbol_table, arithmetic_mode));
            json_object_object_add(obj, "index",
                gen_model_expr(arena, expr->as.array_access.index, symbol_table, arithmetic_mode));
            break;
        }

        case EXPR_ARRAY_SLICE:
        {
            json_object_object_add(obj, "kind", json_object_new_string("array_slice"));
            json_object_object_add(obj, "array",
                gen_model_expr(arena, expr->as.array_slice.array, symbol_table, arithmetic_mode));
            if (expr->as.array_slice.start)
                json_object_object_add(obj, "start",
                    gen_model_expr(arena, expr->as.array_slice.start, symbol_table, arithmetic_mode));
            if (expr->as.array_slice.end)
                json_object_object_add(obj, "end",
                    gen_model_expr(arena, expr->as.array_slice.end, symbol_table, arithmetic_mode));
            if (expr->as.array_slice.step)
                json_object_object_add(obj, "step",
                    gen_model_expr(arena, expr->as.array_slice.step, symbol_table, arithmetic_mode));
            /* Detect pointer slicing: ptr[start..end] where ptr is a pointer type */
            if (expr->as.array_slice.array->expr_type &&
                expr->as.array_slice.array->expr_type->kind == TYPE_POINTER)
            {
                json_object_object_add(obj, "is_pointer_slice",
                    json_object_new_boolean(true));
            }
            break;
        }

        case EXPR_INDEX_ASSIGN:
        {
            json_object_object_add(obj, "kind", json_object_new_string("index_assign"));
            json_object_object_add(obj, "array",
                gen_model_expr(arena, expr->as.index_assign.array, symbol_table, arithmetic_mode));
            json_object_object_add(obj, "index",
                gen_model_expr(arena, expr->as.index_assign.index, symbol_table, arithmetic_mode));
            json_object_object_add(obj, "value",
                gen_model_expr(arena, expr->as.index_assign.value, symbol_table, arithmetic_mode));
            /* Element assignment cleanup annotations for c-min codegen */
            if (expr->expr_type)
            {
                Type *etype = expr->expr_type;
                const char *elem_cleanup = "none";

                switch (etype->kind)
                {
                    case TYPE_STRING:
                        elem_cleanup = "free_str";
                        if (ownership_kind(expr->as.index_assign.value) == OWNERSHIP_BORROW)
                            json_object_object_add(obj, "source_is_borrow",
                                json_object_new_boolean(true));
                        break;
                    case TYPE_STRUCT:
                    {
                        /* Composite val-type structs need old element cleanup + deep copy or ownership transfer */
                        if (!etype->as.struct_type.pass_self_by_ref &&
                            gen_model_type_category(etype) == TYPE_CAT_COMPOSITE)
                        {
                            elem_cleanup = "struct_composite";
                            json_object_object_add(obj, "elem_struct_name",
                                json_object_new_string(etype->as.struct_type.name));
                            /* Deep copy needed when source borrows data (variable, array access, member) */
                            Expr *val = expr->as.index_assign.value;
                            ExprType vt = val->type;
                            bool needs_deep_copy = (vt == EXPR_VARIABLE || vt == EXPR_ARRAY_ACCESS ||
                                                     vt == EXPR_MEMBER || vt == EXPR_MEMBER_ACCESS);
                            json_object_object_add(obj, "needs_deep_copy",
                                json_object_new_boolean(needs_deep_copy));
                        }
                        else if (etype->as.struct_type.pass_self_by_ref)
                        {
                            elem_cleanup = "struct_ref";
                            json_object_object_add(obj, "elem_struct_name",
                                json_object_new_string(etype->as.struct_type.name));
                        }
                        break;
                    }
                    case TYPE_ARRAY:
                    {
                        /* Array-of-array element slot owns its SnArray *. Subscript-assign must
                         * release the old slot and either deep-copy or transfer ownership of the
                         * new value, mirroring sn_array_push semantics. Without this, both the
                         * container slot and any local var holding the source pointer claim
                         * ownership at sn_auto_arr cleanup time → double-free. */
                        elem_cleanup = "array_ref";
                        Expr *val = expr->as.index_assign.value;
                        ExprType vt = val->type;
                        bool needs_deep_copy = (vt == EXPR_VARIABLE || vt == EXPR_ARRAY_ACCESS ||
                                                 vt == EXPR_MEMBER || vt == EXPR_MEMBER_ACCESS);
                        json_object_object_add(obj, "needs_deep_copy",
                            json_object_new_boolean(needs_deep_copy));
                        break;
                    }
                    default:
                        break;
                }
                json_object_object_add(obj, "elem_cleanup",
                    json_object_new_string(elem_cleanup));
            }
            break;
        }

        case EXPR_MEMBER:
        {
            /* Detect builtin property access: str.length, array.length */
            const char *mname = expr->as.member.member_name.start;
            Type *obj_type = expr->as.member.object ? expr->as.member.object->expr_type : NULL;

            /* Optimise typeOf(x).typeId / .fieldCount / .name — emit the
             * value as a compile-time literal instead of allocating a full
             * TypeInfo at runtime (which would leak when used inline). */
            if (expr->as.member.object &&
                expr->as.member.object->type == EXPR_TYPEOF)
            {
                Expr *typeof_expr = expr->as.member.object;
                Type *op_type = typeof_expr->as.typeof_expr.operand->expr_type;

                /* Compute canonical type name (same logic as EXPR_TYPEOF below) */
                const char *type_name = "unknown";
                if (op_type) {
                    switch (op_type->kind) {
                        case TYPE_INT:    type_name = "int"; break;
                        case TYPE_INT32:  type_name = "int32"; break;
                        case TYPE_UINT:   type_name = "uint"; break;
                        case TYPE_UINT32: type_name = "uint32"; break;
                        case TYPE_LONG:   type_name = "long"; break;
                        case TYPE_DOUBLE: type_name = "double"; break;
                        case TYPE_FLOAT:  type_name = "float"; break;
                        case TYPE_CHAR:   type_name = "char"; break;
                        case TYPE_STRING: type_name = "str"; break;
                        case TYPE_BOOL:   type_name = "bool"; break;
                        case TYPE_BYTE:   type_name = "byte"; break;
                        case TYPE_VOID:   type_name = "void"; break;
                        case TYPE_STRUCT: type_name = op_type->as.struct_type.name
                                                      ? op_type->as.struct_type.name : "struct"; break;
                        case TYPE_ARRAY:  type_name = "array"; break;
                        default:          type_name = "unknown"; break;
                    }
                }

                /* FNV-1a hash */
                unsigned long hash = 2166136261u;
                for (const char *p = type_name; *p; p++) {
                    hash ^= (unsigned char)*p;
                    hash *= 16777619u;
                }
                int type_id = (int)(hash & 0x7FFFFFFF);
                int field_count = (op_type && op_type->kind == TYPE_STRUCT)
                                  ? op_type->as.struct_type.field_count : 0;

                if (strcmp(mname, "typeId") == 0) {
                    char buf[64];
                    snprintf(buf, sizeof(buf), "%dLL", type_id);
                    json_object_object_add(obj, "kind", json_object_new_string("literal"));
                    json_object_object_add(obj, "c_literal", json_object_new_string(buf));
                    break;
                }
                if (strcmp(mname, "fieldCount") == 0) {
                    char buf[64];
                    snprintf(buf, sizeof(buf), "%dLL", field_count);
                    json_object_object_add(obj, "kind", json_object_new_string("literal"));
                    json_object_object_add(obj, "c_literal", json_object_new_string(buf));
                    break;
                }
                if (strcmp(mname, "name") == 0) {
                    char buf[256];
                    snprintf(buf, sizeof(buf), "(char *)\"%s\"", type_name);
                    json_object_object_add(obj, "kind", json_object_new_string("literal"));
                    json_object_object_add(obj, "c_literal", json_object_new_string(buf));
                    break;
                }
                /* Fall through to normal member access for .fields etc. */
            }

            if (obj_type && strcmp(mname, "length") == 0 &&
                (obj_type->kind == TYPE_STRING || obj_type->kind == TYPE_ARRAY))
            {
                json_object_object_add(obj, "kind", json_object_new_string("builtin_length"));
                json_object_object_add(obj, "object",
                    gen_model_expr(arena, expr->as.member.object, symbol_table, arithmetic_mode));
                /* Detect when .length is called on an owned string rvalue
                 * (method call / function call returning str) — the temp
                 * string must be freed after measuring its length. */
                if (obj_type->kind == TYPE_STRING)
                {
                    Expr *o = expr->as.member.object;
                    if (o && (o->type == EXPR_CALL || o->type == EXPR_STATIC_CALL ||
                              o->type == EXPR_METHOD_CALL))
                        json_object_object_add(obj, "length_owns_str",
                            json_object_new_boolean(true));
                }
            }
            else
            {
                json_object_object_add(obj, "kind", json_object_new_string("member"));
                json_object_object_add(obj, "object",
                    gen_model_expr(arena, expr->as.member.object, symbol_table, arithmetic_mode));
                json_object_object_add(obj, "member_name",
                    json_object_new_string(mname));

                /* Issue #49: Member access on a value-struct rvalue with
                 * non-trivial cleanup (e.g. `build().probs`).  Without
                 * intervention, the rvalue's other refcounted/heap fields
                 * leak — there is no variable to attach `sn_auto_<S>` to.
                 *
                 * Lift the call result into a local with `sn_auto_<S>` so
                 * its cleanup runs after the field is read.  An inline
                 * "keeper" op (retain / strdup / sn_array_copy / val copy)
                 * preserves the read field past the cleanup, returning an
                 * owned value the caller can hold.  Consumers (var_decl,
                 * etc.) detect this pattern and skip their own ownership
                 * op so we don't double-retain. */
                Expr *o = expr->as.member.object;
                if (o && (o->type == EXPR_CALL || o->type == EXPR_STATIC_CALL) &&
                    obj_type && obj_type->kind == TYPE_STRUCT &&
                    !obj_type->as.struct_type.pass_self_by_ref &&
                    gen_model_type_has_heap_fields(obj_type))
                {
                    /* Determine the keeper op for the accessed field. */
                    Type *ft = expr->expr_type;
                    const char *keeper = "none";  /* plain value: no keeper */
                    const char *keeper_type_name = NULL;
                    if (ft)
                    {
                        if (ft->kind == TYPE_STRING)
                            keeper = "strdup";
                        else if (ft->kind == TYPE_ARRAY)
                            keeper = "arr_copy";
                        else if (ft->kind == TYPE_STRUCT &&
                                 ft->as.struct_type.pass_self_by_ref)
                        {
                            keeper = "retain";
                            keeper_type_name = ft->as.struct_type.name;
                        }
                        else if (ft->kind == TYPE_STRUCT &&
                                 !ft->as.struct_type.pass_self_by_ref &&
                                 gen_model_type_has_heap_fields(ft))
                        {
                            keeper = "val_copy";
                            keeper_type_name = ft->as.struct_type.name;
                        }
                    }
                    int lift_id = g_model_member_lift_count++;
                    char tmp_var[64];
                    snprintf(tmp_var, sizeof(tmp_var), "__mtmp_%d__", lift_id);
                    json_object_object_add(obj, "needs_struct_tmp_lift",
                        json_object_new_boolean(true));
                    json_object_object_add(obj, "lift_struct_name",
                        json_object_new_string(obj_type->as.struct_type.name));
                    json_object_object_add(obj, "lift_tmp_var",
                        json_object_new_string(arena_strdup(arena, tmp_var)));
                    json_object_object_add(obj, "lift_keeper",
                        json_object_new_string(keeper));
                    if (keeper_type_name)
                        json_object_object_add(obj, "lift_keeper_type",
                            json_object_new_string(keeper_type_name));
                }
            }
            break;
        }

        case EXPR_MEMBER_ACCESS:
        {
            json_object_object_add(obj, "kind", json_object_new_string("member_access"));
            json_object_object_add(obj, "object",
                gen_model_expr(arena, expr->as.member_access.object, symbol_table, arithmetic_mode));
            json_object_object_add(obj, "field_name",
                json_object_new_string(expr->as.member_access.field_name.start));
            json_object_object_add(obj, "field_index",
                json_object_new_int(expr->as.member_access.field_index));
            break;
        }

        case EXPR_MEMBER_ASSIGN:
        {
            json_object_object_add(obj, "kind", json_object_new_string("member_assign"));
            json_object_object_add(obj, "object",
                gen_model_expr(arena, expr->as.member_assign.object, symbol_table, arithmetic_mode));
            json_object_object_add(obj, "field_name",
                json_object_new_string(expr->as.member_assign.field_name.start));
            json_object_object_add(obj, "value",
                gen_model_expr(arena, expr->as.member_assign.value, symbol_table, arithmetic_mode));
            /* Field assignment cleanup annotations for c-min codegen */
            if (expr->expr_type)
            {
                Type *ftype = expr->expr_type;
                const char *field_cleanup = "none";

                switch (ftype->kind)
                {
                    case TYPE_STRING:
                        field_cleanup = "free_str";
                        break;
                    case TYPE_ARRAY:
                        field_cleanup = "cleanup_arr";
                        break;
                    case TYPE_STRUCT:
                        if (ftype->as.struct_type.pass_self_by_ref)
                        {
                            field_cleanup = "release_ref";
                            json_object_object_add(obj, "field_type_name",
                                json_object_new_string(ftype->as.struct_type.name));
                        }
                        else
                        {
                            for (int fi = 0; fi < ftype->as.struct_type.field_count; fi++)
                            {
                                Type *ft = ftype->as.struct_type.fields[fi].type;
                                if (ft && (ft->kind == TYPE_STRING || ft->kind == TYPE_ARRAY ||
                                    ft->kind == TYPE_FUNCTION ||
                                    (ft->kind == TYPE_STRUCT && ft->as.struct_type.pass_self_by_ref)))
                                {
                                    field_cleanup = "cleanup_val";
                                    json_object_object_add(obj, "field_type_name",
                                        json_object_new_string(ftype->as.struct_type.name));
                                    break;
                                }
                            }
                        }
                        break;
                    case TYPE_FUNCTION:
                        field_cleanup = "free_closure";
                        {
                            int wrap_id = maybe_emit_fn_ref_wrapper(arena,
                                expr->as.member_assign.value, ftype, symbol_table);
                            if (wrap_id >= 0)
                            {
                                json_object_object_add(obj, "needs_closure_wrap",
                                    json_object_new_boolean(true));
                                json_object_object_add(obj, "fn_wrapper_id",
                                    json_object_new_int(wrap_id));
                            }
                        }
                        break;
                    default:
                        break;
                }
                json_object_object_add(obj, "field_cleanup",
                    json_object_new_string(field_cleanup));
                /* Unified classifier: source_is_borrow when RHS reads through a
                 * live owner and the field's cleanup_kind requires a fresh +1
                 * credit. Template selects strdup / retain / val-copy / arr-copy
                 * off field_cleanup. */
                if ((strcmp(field_cleanup, "free_str") == 0 ||
                     strcmp(field_cleanup, "release_ref") == 0 ||
                     strcmp(field_cleanup, "cleanup_val") == 0 ||
                     strcmp(field_cleanup, "cleanup_arr") == 0) &&
                    ownership_kind(expr->as.member_assign.value) == OWNERSHIP_BORROW)
                {
                    json_object_object_add(obj, "source_is_borrow",
                        json_object_new_boolean(true));
                }
            }
            break;
        }

        case EXPR_STRUCT_LITERAL:
        {
            json_object_object_add(obj, "kind", json_object_new_string("struct_literal"));
            /* Use the resolved type name (from expr_type) when available — this gives the
             * monomorphized name (e.g. "Stack_int") rather than the template name ("Stack")
             * that the parser stored in struct_literal.struct_name. */
            const char *sl_name = expr->as.struct_literal.struct_name.start;
            if (expr->expr_type && expr->expr_type->kind == TYPE_STRUCT &&
                expr->expr_type->as.struct_type.name != NULL)
            {
                sl_name = expr->expr_type->as.struct_type.name;
            }
            json_object_object_add(obj, "struct_name", json_object_new_string(sl_name));
            /* Add struct type info for c-min memory management */
            if (expr->expr_type)
            {
                json_object_object_add(obj, "type",
                    gen_model_type(arena, expr->expr_type));
            }
            json_object *fields = json_object_new_array();
            for (int i = 0; i < expr->as.struct_literal.field_count; i++)
            {
                json_object *f = json_object_new_object();
                json_object_object_add(f, "name",
                    json_object_new_string(expr->as.struct_literal.fields[i].name.start));
                json_object_object_add(f, "value",
                    gen_model_expr(arena, expr->as.struct_literal.fields[i].value, symbol_table, arithmetic_mode));
                /* Check if this field's value needs ownership wrapping */
                Expr *fv = expr->as.struct_literal.fields[i].value;
                /* Issue #49: lifted member access already runs the keeper op
                 * (strdup/sn_array_copy/retain/val_copy) inside the lift block,
                 * so the field must skip its own ownership op. */
                bool fv_is_lifted_member = false;
                if (fv && fv->type == EXPR_MEMBER && fv->as.member.object)
                {
                    Expr *mo = fv->as.member.object;
                    Type *mot = mo->expr_type;
                    if ((mo->type == EXPR_CALL || mo->type == EXPR_STATIC_CALL) &&
                        mot && mot->kind == TYPE_STRUCT &&
                        !mot->as.struct_type.pass_self_by_ref &&
                        gen_model_type_has_heap_fields(mot))
                    {
                        fv_is_lifted_member = true;
                    }
                }
                /* Unified classifier: BORROW source → emit source_is_borrow
                 * plus the type-name context keys the template needs to build
                 * the concrete acquire call (retain_type_name for ref struct,
                 * copy_type_name for val-struct with heap fields). OWNED
                 * ref-struct source emits consumes_source so the chain
                 * flattener suppresses the lifted chain_tmp's sn_auto_T
                 * release, transferring the existing +1 into the field. */
                if (fv && fv->expr_type && !fv_is_lifted_member)
                {
                    OwnershipKind k = ownership_kind(fv);
                    Type *ft = fv->expr_type;
                    if (ft->kind == TYPE_STRUCT && ft->as.struct_type.pass_self_by_ref)
                    {
                        if (k == OWNERSHIP_BORROW)
                        {
                            json_object_object_add(f, "retain_type_name",
                                json_object_new_string(ft->as.struct_type.name));
                            json_object_object_add(f, "source_is_borrow",
                                json_object_new_boolean(true));
                        }
                        else
                        {
                            json_object_object_add(f, "consumes_source",
                                json_object_new_boolean(true));
                        }
                    }
                    else if (ft->kind == TYPE_STRUCT &&
                             gen_model_type_category(ft) == TYPE_CAT_COMPOSITE)
                    {
                        if (k == OWNERSHIP_BORROW)
                        {
                            json_object_object_add(f, "copy_type_name",
                                json_object_new_string(ft->as.struct_type.name));
                            json_object_object_add(f, "source_is_borrow",
                                json_object_new_boolean(true));
                        }
                    }
                    else if (ft->kind == TYPE_STRING || ft->kind == TYPE_ARRAY)
                    {
                        if (k == OWNERSHIP_BORROW)
                        {
                            json_object_object_add(f, "source_is_borrow",
                                json_object_new_boolean(true));
                        }
                    }
                }
                /* Empty array literal with nil element type: fix type from struct field def */
                if (fv && fv->type == EXPR_ARRAY && fv->as.array.element_count == 0 &&
                    expr->expr_type && expr->expr_type->kind == TYPE_STRUCT)
                {
                    /* Find this field's declared type in the struct definition */
                    const char *fname = expr->as.struct_literal.fields[i].name.start;
                    for (int fi = 0; fi < expr->expr_type->as.struct_type.field_count; fi++)
                    {
                        StructField *sf = &expr->expr_type->as.struct_type.fields[fi];
                        if (strcmp(sf->name, fname) == 0 && sf->type &&
                            sf->type->kind == TYPE_ARRAY && sf->type->as.array.element_type)
                        {
                            Type *expected_et = sf->type->as.array.element_type;
                            /* Override the type on the generated field value model */
                            json_object *fval_obj;
                            json_object_object_get_ex(f, "value", &fval_obj);
                            if (fval_obj)
                            {
                                json_object_object_add(fval_obj, "type",
                                    gen_model_type(arena, sf->type));
                                /* Also add elem_release/elem_copy callbacks */
                                if (expected_et->kind == TYPE_STRING)
                                {
                                    json_object_object_add(fval_obj, "elem_release_fn",
                                        json_object_new_string("(void (*)(void *))sn_cleanup_str"));
                                    json_object_object_add(fval_obj, "elem_copy_fn",
                                        json_object_new_string("sn_copy_str"));
                                }
                                else if (expected_et->kind == TYPE_ARRAY)
                                {
                                    json_object_object_add(fval_obj, "elem_release_fn",
                                        json_object_new_string("(void (*)(void *))sn_cleanup_array"));
                                    json_object_object_add(fval_obj, "elem_copy_fn",
                                        json_object_new_string("sn_copy_array"));
                                }
                                else if (expected_et->kind == TYPE_STRUCT)
                                {
                                    TypeCategory et_cat = gen_model_type_category(expected_et);
                                    if (et_cat == TYPE_CAT_REFCOUNTED || et_cat == TYPE_CAT_COMPOSITE)
                                    {
                                        char buf_r[256], buf_c[256];
                                        if (expected_et->as.struct_type.pass_self_by_ref)
                                        {
                                            snprintf(buf_r, sizeof(buf_r),
                                                "__sn__%s_release_elem", expected_et->as.struct_type.name);
                                            snprintf(buf_c, sizeof(buf_c),
                                                "__sn__%s_retain_into", expected_et->as.struct_type.name);
                                        }
                                        else
                                        {
                                            snprintf(buf_r, sizeof(buf_r),
                                                "__sn__%s_cleanup_elem", expected_et->as.struct_type.name);
                                            snprintf(buf_c, sizeof(buf_c),
                                                "__sn__%s_copy_into", expected_et->as.struct_type.name);
                                        }
                                        json_object_object_add(fval_obj, "elem_release_fn",
                                            json_object_new_string(buf_r));
                                        json_object_object_add(fval_obj, "elem_copy_fn",
                                            json_object_new_string(buf_c));
                                    }
                                }
                            }
                            break;
                        }
                    }
                }
                /* Function-typed fields need heap closure wrapping for bare function references.
                 * Parameters of function type are already closure pointers. */
                if (fv && fv->expr_type && fv->expr_type->kind == TYPE_FUNCTION)
                {
                    int wrap_id = maybe_emit_fn_ref_wrapper(arena, fv,
                        fv->expr_type, symbol_table);
                    if (wrap_id >= 0)
                    {
                        json_object_object_add(f, "needs_closure_wrap",
                            json_object_new_boolean(true));
                        json_object_object_add(f, "fn_wrapper_id",
                            json_object_new_int(wrap_id));
                    }
                }
                json_object_array_add(fields, f);
            }
            json_object_object_add(obj, "fields", fields);
            break;
        }

        case EXPR_INTERPOLATED:
        {
            json_object_object_add(obj, "kind", json_object_new_string("interpolated_string"));
            json_object *parts = json_object_new_array();
            for (int i = 0; i < expr->as.interpol.part_count; i++)
            {
                json_object *part = json_object_new_object();
                Expr *p = expr->as.interpol.parts[i];
                if (p->type == EXPR_LITERAL && p->as.literal.type &&
                    p->as.literal.type->kind == TYPE_STRING)
                {
                    json_object_object_add(part, "kind", json_object_new_string("text"));
                    json_object_object_add(part, "value",
                        json_object_new_string(c_escape_string(arena, p->as.literal.value.string_value)));
                }
                else
                {
                    json_object_object_add(part, "kind", json_object_new_string("expr"));
                    json_object_object_add(part, "expr",
                        gen_model_expr(arena, p, symbol_table, arithmetic_mode));

                    /* Flag struct expressions with toString() for interpolation */
                    if (p->expr_type && p->expr_type->kind == TYPE_STRUCT)
                    {
                        StructMethod *ts = ast_struct_get_method(p->expr_type, "toString");
                        json_object_object_add(part, "has_toString",
                            json_object_new_boolean(ts != NULL));
                    }

                    /* Flag heap-producing string expressions so the template can free them */
                    if (is_heap_producing_string_expr(p))
                    {
                        json_object_object_add(part, "is_str_temp",
                            json_object_new_boolean(true));
                    }
                    /* Named function calls returning string always produce owned values. */
                    else if (is_named_fn_str_call(p, symbol_table))
                    {
                        json_object_object_add(part, "is_str_temp",
                            json_object_new_boolean(true));
                    }
                    /* Closure/lambda calls returning string: lambdas now always return
                     * owned strings (body_needs_strdup ensures this), so mark for cleanup. */
                    else if (p->type == EXPR_CALL &&
                             p->expr_type && p->expr_type->kind == TYPE_STRING &&
                             p->as.call.callee &&
                             p->as.call.callee->type == EXPR_VARIABLE)
                    {
                        json_object_object_add(part, "is_str_temp",
                            json_object_new_boolean(true));
                    }
                }
                if (expr->as.interpol.format_specs && expr->as.interpol.format_specs[i])
                {
                    json_object_object_add(part, "format_spec",
                        json_object_new_string(expr->as.interpol.format_specs[i]));
                }
                json_object_array_add(parts, part);
            }
            json_object_object_add(obj, "parts", parts);
            json_object_object_add(obj, "part_count",
                json_object_new_int(expr->as.interpol.part_count));
            break;
        }

        case EXPR_INCREMENT:
        {
            json_object_object_add(obj, "kind", json_object_new_string("increment"));
            json_object_object_add(obj, "operand",
                gen_model_expr(arena, expr->as.operand, symbol_table, arithmetic_mode));
            /* Sync variable: emit mutex name so template can wrap in lock/unlock */
            if (expr->as.operand->type == EXPR_VARIABLE && symbol_table) {
                Symbol *sym = symbol_table_lookup_symbol(symbol_table, expr->as.operand->as.variable.name);
                if (sym && sym->sync_mod == SYNC_ATOMIC) {
                    int nlen = expr->as.operand->as.variable.name.length;
                    char vname[256];
                    if (nlen >= (int)sizeof(vname)) nlen = (int)sizeof(vname) - 1;
                    strncpy(vname, expr->as.operand->as.variable.name.start, nlen);
                    vname[nlen] = '\0';
                    json_object_object_add(obj, "sync_var_name", json_object_new_string(vname));
                }
            }
            break;
        }

        case EXPR_DECREMENT:
        {
            json_object_object_add(obj, "kind", json_object_new_string("decrement"));
            json_object_object_add(obj, "operand",
                gen_model_expr(arena, expr->as.operand, symbol_table, arithmetic_mode));
            /* Sync variable: emit mutex name so template can wrap in lock/unlock */
            if (expr->as.operand->type == EXPR_VARIABLE && symbol_table) {
                Symbol *sym = symbol_table_lookup_symbol(symbol_table, expr->as.operand->as.variable.name);
                if (sym && sym->sync_mod == SYNC_ATOMIC) {
                    int nlen = expr->as.operand->as.variable.name.length;
                    char vname[256];
                    if (nlen >= (int)sizeof(vname)) nlen = (int)sizeof(vname) - 1;
                    strncpy(vname, expr->as.operand->as.variable.name.start, nlen);
                    vname[nlen] = '\0';
                    json_object_object_add(obj, "sync_var_name", json_object_new_string(vname));
                }
            }
            break;
        }

        case EXPR_RANGE:
        {
            json_object_object_add(obj, "kind", json_object_new_string("range"));
            json_object_object_add(obj, "start",
                gen_model_expr(arena, expr->as.range.start, symbol_table, arithmetic_mode));
            json_object_object_add(obj, "end",
                gen_model_expr(arena, expr->as.range.end, symbol_table, arithmetic_mode));
            break;
        }

        case EXPR_SPREAD:
        {
            json_object_object_add(obj, "kind", json_object_new_string("spread"));
            json_object_object_add(obj, "operand",
                gen_model_expr(arena, expr->as.spread.array, symbol_table, arithmetic_mode));
            break;
        }

        case EXPR_SIZED_ARRAY_ALLOC:
        {
            json_object_object_add(obj, "kind", json_object_new_string("sized_array"));
            json_object_object_add(obj, "element_type",
                gen_model_type(arena, expr->as.sized_array_alloc.element_type));
            json_object_object_add(obj, "size",
                gen_model_expr(arena, expr->as.sized_array_alloc.size_expr, symbol_table, arithmetic_mode));
            if (expr->as.sized_array_alloc.default_value)
            {
                json_object_object_add(obj, "default_value",
                    gen_model_expr(arena, expr->as.sized_array_alloc.default_value, symbol_table, arithmetic_mode));
            }
            break;
        }

        case EXPR_LAMBDA:
        {
            json_object_object_add(obj, "kind", json_object_new_string("lambda"));
            LambdaExpr *lam = &expr->as.lambda;
            lam->lambda_id = g_model_lambda_count++;
            json_object_object_add(obj, "lambda_id", json_object_new_int(lam->lambda_id));
            json_object_object_add(obj, "return_type", gen_model_type(arena, lam->return_type));
            json_object_object_add(obj, "modifier", json_object_new_string(func_mod_str(lam->modifier)));
            json_object_object_add(obj, "is_native", json_object_new_boolean(lam->is_native));

            json_object *params = json_object_new_array();
            for (int i = 0; i < lam->param_count; i++)
            {
                json_object *p = json_object_new_object();
                json_object_object_add(p, "name",
                    json_object_new_string(lam->params[i].name.start));
                json_object_object_add(p, "type",
                    gen_model_type(arena, lam->params[i].type));
                json_object_object_add(p, "mem_qual",
                    json_object_new_string(gen_model_mem_qual_str(lam->params[i].mem_qualifier)));
                gen_model_emit_param_cleanup(p, &lam->params[i], lam->is_native);
                json_object_array_add(params, p);
            }
            json_object_object_add(obj, "params", params);

            /* Capture analysis — walk body to find free variables */
            ModelCaptures caps;
            mc_analyze(arena, lam, &caps);

            json_object *captures = json_object_new_array();
            bool has_struct_copy_captures = false;
            bool has_capture_cleanup = false;
            for (int i = 0; i < caps.count; i++)
            {
                json_object *cap = json_object_new_object();
                json_object_object_add(cap, "name",
                    json_object_new_string(caps.names[i]));
                json_object_object_add(cap, "type",
                    gen_model_type(arena, caps.types[i]));
                json_object_object_add(cap, "is_primitive",
                    json_object_new_boolean(mc_is_primitive(caps.types[i])));
                /* is_ref: true if outer var is promoted (in g_captured_vars) */
                bool is_ref = false;
                for (int ci = 0; ci < g_captured_var_count; ci++) {
                    if (strcmp(g_captured_vars[ci], caps.names[i]) == 0) { is_ref = true; break; }
                }
                json_object_object_add(cap, "is_ref", json_object_new_boolean(is_ref));

                /* Emit per-capture cap_action / cap_cleanup so the closure owns
                 * its captured heap data. Without this, str / refcounted-struct /
                 * array captures share pointers with the caller and trigger a
                 * heap-use-after-free once the caller's sn_auto_* cleanup runs. */
                if (mc_emit_capture_actions(cap, caps.types[i], is_ref))
                    has_capture_cleanup = true;

                /* Legacy needs_struct_copy flag — used by the closure type
                 * definition to know which fields are deep-copied value structs. */
                if (!is_ref && caps.types[i] &&
                    caps.types[i]->kind == TYPE_STRUCT &&
                    !caps.types[i]->as.struct_type.pass_self_by_ref &&
                    gen_model_type_has_heap_fields(caps.types[i]))
                {
                    json_object_object_add(cap, "needs_struct_copy",
                        json_object_new_boolean(true));
                    json_object_object_add(cap, "struct_type_name",
                        json_object_new_string(caps.types[i]->as.struct_type.name));
                    has_struct_copy_captures = true;
                }

                json_object_array_add(captures, cap);
            }
            json_object_object_add(obj, "captures", captures);
            json_object_object_add(obj, "has_captures",
                json_object_new_boolean(caps.count > 0));
            json_object_object_add(obj, "has_struct_copy_captures",
                json_object_new_boolean(has_struct_copy_captures));
            json_object_object_add(obj, "has_capture_cleanup",
                json_object_new_boolean(has_capture_cleanup));
            /* Check if any capture is by reference (needs heap promotion) */
            {
                bool has_ref_caps = false;
                for (int ci = 0; ci < caps.count; ci++)
                {
                    for (int gi = 0; gi < g_captured_var_count; gi++)
                    {
                        if (strcmp(g_captured_vars[gi], caps.names[ci]) == 0)
                        { has_ref_caps = true; break; }
                    }
                    if (has_ref_caps) break;
                }
                json_object_object_add(obj, "has_ref_captures",
                    json_object_new_boolean(has_ref_caps));
            }

            /* Body */
            {
                bool prev_in_lambda = g_in_lambda_body;
                g_in_lambda_body = true;
                char **saved_ref_names;
                int saved_ref_count;
                push_lambda_as_ref_params(arena, lam, &saved_ref_names, &saved_ref_count);
                if (lam->has_stmt_body)
                {
                    json_object *body = json_object_new_array();
                    for (int i = 0; i < lam->body_stmt_count; i++)
                    {
                        json_object_array_add(body,
                            gen_model_stmt(arena, lam->body_stmts[i], symbol_table, arithmetic_mode));
                    }
                    json_object_object_add(obj, "body_stmts", body);
                }
                else if (lam->body)
                {
                    json_object_object_add(obj, "body",
                        gen_model_expr(arena, lam->body, symbol_table, arithmetic_mode));

                    /* If the lambda returns a string and the body expression is NOT
                     * already heap-producing, wrap the return in strdup() so that ALL
                     * string-returning lambdas have a uniform ownership model (caller
                     * always owns the returned string and must free it). */
                    if (lam->return_type && lam->return_type->kind == TYPE_STRING &&
                        !is_heap_producing_string_expr(lam->body) &&
                        !is_named_fn_str_call(lam->body, symbol_table))
                    {
                        json_object_object_add(obj, "body_needs_strdup",
                            json_object_new_boolean(true));
                    }
                }
                pop_lambda_as_ref_params(saved_ref_names, saved_ref_count);
                g_in_lambda_body = prev_in_lambda;
            }

            /* Add to global lambdas collection for forward decls and function defs */
            if (g_model_lambdas != NULL)
            {
                json_object *ldef = json_object_new_object();
                json_object_object_add(ldef, "lambda_id", json_object_new_int(lam->lambda_id));
                /* Record the source file so gen_model_split can route the
                 * lambda body to the same TU as the function that references
                 * it. Without this, lambdas inside imported modules end up
                 * emitted as static in main.c and fail to link. */
                if (expr->token && expr->token->filename)
                    gen_model_add_source_file(ldef, expr->token->filename);
                json_object_object_add(ldef, "return_type", gen_model_type(arena, lam->return_type));

                json_object *lparams = json_object_new_array();
                for (int i = 0; i < lam->param_count; i++)
                {
                    json_object *lp = json_object_new_object();
                    json_object_object_add(lp, "name",
                        json_object_new_string(lam->params[i].name.start));
                    json_object_object_add(lp, "type",
                        gen_model_type(arena, lam->params[i].type));
                    json_object_object_add(lp, "mem_qual",
                        json_object_new_string(gen_model_mem_qual_str(lam->params[i].mem_qualifier)));
                    gen_model_emit_param_cleanup(lp, &lam->params[i], lam->is_native);
                    json_object_array_add(lparams, lp);
                }
                json_object_object_add(ldef, "params", lparams);

                /* Add captures to the lambda definition too */
                json_object *lcaps = json_object_new_array();
                bool ldef_has_struct_copy_captures = false;
                bool ldef_has_capture_cleanup = false;
                for (int i = 0; i < caps.count; i++)
                {
                    json_object *lc = json_object_new_object();
                    json_object_object_add(lc, "name",
                        json_object_new_string(caps.names[i]));
                    json_object_object_add(lc, "type",
                        gen_model_type(arena, caps.types[i]));
                    json_object_object_add(lc, "is_primitive",
                        json_object_new_boolean(mc_is_primitive(caps.types[i])));
                    bool lc_is_ref = false;
                    for (int ci = 0; ci < g_captured_var_count; ci++) {
                        if (strcmp(g_captured_vars[ci], caps.names[i]) == 0) { lc_is_ref = true; break; }
                    }
                    json_object_object_add(lc, "is_ref", json_object_new_boolean(lc_is_ref));

                    if (mc_emit_capture_actions(lc, caps.types[i], lc_is_ref))
                        ldef_has_capture_cleanup = true;

                    /* Mirror the needs_struct_copy flag onto the lambda definition's
                     * captures so the closure type definition / cleanup function
                     * template can emit the deep cleanup. */
                    if (!lc_is_ref && caps.types[i] &&
                        caps.types[i]->kind == TYPE_STRUCT &&
                        !caps.types[i]->as.struct_type.pass_self_by_ref &&
                        gen_model_type_has_heap_fields(caps.types[i]))
                    {
                        json_object_object_add(lc, "needs_struct_copy",
                            json_object_new_boolean(true));
                        json_object_object_add(lc, "struct_type_name",
                            json_object_new_string(caps.types[i]->as.struct_type.name));
                        ldef_has_struct_copy_captures = true;
                    }

                    json_object_array_add(lcaps, lc);
                }
                json_object_object_add(ldef, "captures", lcaps);
                json_object_object_add(ldef, "has_captures",
                    json_object_new_boolean(caps.count > 0));
                json_object_object_add(ldef, "has_struct_copy_captures",
                    json_object_new_boolean(ldef_has_struct_copy_captures));
                json_object_object_add(ldef, "has_capture_cleanup",
                    json_object_new_boolean(ldef_has_capture_cleanup));
                /* Check if any capture is by reference */
                {
                    bool has_ref_caps = false;
                    for (int ci = 0; ci < caps.count; ci++)
                    {
                        for (int gi = 0; gi < g_captured_var_count; gi++)
                        {
                            if (strcmp(g_captured_vars[gi], caps.names[ci]) == 0)
                            { has_ref_caps = true; break; }
                        }
                        if (has_ref_caps) break;
                    }
                    json_object_object_add(ldef, "has_ref_captures",
                        json_object_new_boolean(has_ref_caps));
                }

                /* Body for the lambda function definition */
                {
                    bool prev_in_lambda = g_in_lambda_body;
                    g_in_lambda_body = true;
                    char **saved_ref_names;
                    int saved_ref_count;
                    push_lambda_as_ref_params(arena, lam, &saved_ref_names, &saved_ref_count);
                    if (lam->has_stmt_body)
                    {
                        json_object *lbody = json_object_new_array();
                        for (int i = 0; i < lam->body_stmt_count; i++)
                        {
                            json_object_array_add(lbody,
                                gen_model_stmt(arena, lam->body_stmts[i], symbol_table, arithmetic_mode));
                        }
                        json_object_object_add(ldef, "body_stmts", lbody);
                    }
                    else if (lam->body)
                    {
                        json_object_object_add(ldef, "body",
                            gen_model_expr(arena, lam->body, symbol_table, arithmetic_mode));

                        /* If the lambda returns a string and the body expression is NOT
                         * already heap-producing, wrap the return in strdup() so that ALL
                         * string-returning lambdas have a uniform ownership model (caller
                         * always owns the returned string and must free it). */
                        if (lam->return_type && lam->return_type->kind == TYPE_STRING &&
                            !is_heap_producing_string_expr(lam->body) &&
                            !is_named_fn_str_call(lam->body, symbol_table))
                        {
                            json_object_object_add(ldef, "body_needs_strdup",
                                json_object_new_boolean(true));
                        }
                    }
                    pop_lambda_as_ref_params(saved_ref_names, saved_ref_count);
                    g_in_lambda_body = prev_in_lambda;
                }

                json_object_array_add(g_model_lambdas, ldef);
            }
            break;
        }

        case EXPR_MATCH:
        {
            json_object_object_add(obj, "kind", json_object_new_string("match"));
            Expr *match_subj = expr->as.match_expr.subject;
            json_object_object_add(obj, "subject",
                gen_model_expr(arena, match_subj, symbol_table, arithmetic_mode));

            /* If the match subject is a string-returning function/closure call,
             * flag it so the template can free __match_subject__ after the match. */
            if (match_subj && match_subj->expr_type &&
                match_subj->expr_type->kind == TYPE_STRING &&
                (is_heap_producing_string_expr(match_subj) ||
                 is_named_fn_str_call(match_subj, symbol_table) ||
                 (match_subj->type == EXPR_CALL &&
                  match_subj->as.call.callee &&
                  match_subj->as.call.callee->type == EXPR_VARIABLE)))
            {
                json_object_object_add(obj, "subject_is_str_temp",
                    json_object_new_boolean(true));
            }

            json_object *arms = json_object_new_array();
            for (int i = 0; i < expr->as.match_expr.arm_count; i++)
            {
                MatchArm *arm = &expr->as.match_expr.arms[i];
                json_object *a = json_object_new_object();
                json_object_object_add(a, "is_else", json_object_new_boolean(arm->is_else));
                json_object *patterns = json_object_new_array();
                for (int j = 0; j < arm->pattern_count; j++)
                {
                    json_object_array_add(patterns,
                        gen_model_expr(arena, arm->patterns[j], symbol_table, arithmetic_mode));
                }
                json_object_object_add(a, "patterns", patterns);
                if (arm->body)
                {
                    json_object_object_add(a, "body",
                        gen_model_stmt(arena, arm->body, symbol_table, arithmetic_mode));

                    /* For string-returning match expressions: check if the arm's
                     * last expression already produces an owned string (e.g. a
                     * nested match, interpolation, concat, or function call).
                     * If so, the template should NOT wrap it in an extra strdup. */
                    if (expr->expr_type && expr->expr_type->kind == TYPE_STRING &&
                        arm->body->type == STMT_BLOCK && arm->body->as.block.count > 0)
                    {
                        Stmt *last = arm->body->as.block.statements[arm->body->as.block.count - 1];
                        if (last->type == STMT_EXPR && last->as.expression.expression)
                        {
                            Expr *arm_e = last->as.expression.expression;
                            if (arm_e->type == EXPR_MATCH ||
                                arm_e->type == EXPR_INTERPOLATED ||
                                is_heap_producing_string_expr(arm_e) ||
                                is_named_fn_str_call(arm_e, symbol_table) ||
                                (arm_e->type == EXPR_CALL &&
                                 arm_e->expr_type && arm_e->expr_type->kind == TYPE_STRING))
                            {
                                json_object_object_add(a, "arm_expr_is_owned",
                                    json_object_new_boolean(true));
                            }
                        }
                    }
                }
                json_object_array_add(arms, a);
            }
            json_object_object_add(obj, "arms", arms);
            break;
        }

        case EXPR_THREAD_SPAWN:
        {
            json_object_object_add(obj, "kind", json_object_new_string("thread_spawn"));
            int thread_id = g_model_thread_count++;
            json_object_object_add(obj, "thread_id", json_object_new_int(thread_id));

            Expr *call_expr = expr->as.thread_spawn.call;
            bool was_in_ts = g_in_thread_spawn_call;
            g_in_thread_spawn_call = true;
            json_object *call_obj = gen_model_expr(arena, call_expr, symbol_table, arithmetic_mode);
            g_in_thread_spawn_call = was_in_ts;
            json_object_object_add(obj, "call", call_obj);
            json_object_object_add(obj, "modifier",
                json_object_new_string(func_mod_str(expr->as.thread_spawn.modifier)));

            /* As-ref struct args are retained by the thread spawn template
             * (thread_spawn.hbs) and released by the thread wrapper after
             * the call completes. No nullification needed here — refcounting
             * handles both joinable and fire-and-forget ownership. */

            /* Determine if return type is void */
            Type *ret_type = call_expr->expr_type;
            bool is_void = (ret_type && ret_type->kind == TYPE_VOID);
            json_object_object_add(obj, "is_void", json_object_new_boolean(is_void));

            /* Collect thread definition for top-level wrapper generation */
            if (g_model_threads != NULL)
            {
                json_object *tdef = json_object_new_object();
                json_object_object_add(tdef, "thread_id", json_object_new_int(thread_id));
                json_object_object_add(tdef, "is_void", json_object_new_boolean(is_void));
                json_object_object_add(tdef, "return_type",
                    gen_model_type(arena, ret_type));

                /* Extract function name from callee */
                bool is_method_spawn = false;
                bool is_closure_spawn = false;
                MemoryQualifier *param_mem_quals = NULL;
                int param_count = 0;
                if (call_expr->type == EXPR_CALL && call_expr->as.call.callee)
                {
                    Expr *callee = call_expr->as.call.callee;
                    if (callee->type == EXPR_VARIABLE)
                    {
                        /* Check if this is a closure variable (same logic as
                         * is_closure_call detection for regular calls). */
                        Type *ct = callee->expr_type;
                        if (ct && ct->kind == TYPE_FUNCTION)
                        {
                            Symbol *sym = symbol_table_lookup_symbol(symbol_table,
                                callee->as.variable.name);
                            if (sym == NULL || !sym->is_function)
                                is_closure_spawn = true;
                        }
                        if (is_closure_spawn)
                        {
                            json_object_object_add(tdef, "is_closure_spawn",
                                json_object_new_boolean(true));
                            json_object_object_add(tdef, "closure_var_name",
                                json_object_new_string(callee->as.variable.name.start));
                        }
                        else
                        {
                            json_object_object_add(tdef, "func_name",
                                json_object_new_string(callee->as.variable.name.start));
                            /* Native functions use unmangled names in C — pass
                             * is_native + target_name so the thread wrapper
                             * template calls the correct symbol. */
                            Type *fn_type = callee->expr_type;
                            if (fn_type && fn_type->kind == TYPE_FUNCTION &&
                                fn_type->as.function.is_native)
                            {
                                json_object_object_add(tdef, "is_native",
                                    json_object_new_boolean(true));
                                Symbol *func_sym = symbol_table_lookup_symbol(
                                    symbol_table, callee->as.variable.name);
                                const char *tname = (func_sym && func_sym->c_alias)
                                    ? func_sym->c_alias
                                    : callee->as.variable.name.start;
                                json_object_object_add(tdef, "target_name",
                                    json_object_new_string(tname));
                            }
                        }
                    }
                    else if (callee->type == EXPR_MEMBER && callee->as.member.resolved_struct_type)
                    {
                        /* Method call: func_name = StructName_methodName */
                        const char *struct_name = callee->as.member.resolved_struct_type->as.struct_type.name;
                        const char *method_name = callee->as.member.member_name.start;
                        char func_name_buf[256];
                        snprintf(func_name_buf, sizeof(func_name_buf), "%s_%s", struct_name, method_name);
                        json_object_object_add(tdef, "func_name",
                            json_object_new_string(func_name_buf));
                        json_object_object_add(tdef, "is_method", json_object_new_boolean(true));
                        /* Add struct type for the self pointer */
                        json_object_object_add(tdef, "struct_name",
                            json_object_new_string(struct_name));
                        is_method_spawn = true;
                    }
                    /* Extract param memory qualifiers for as ref detection */
                    if (callee->expr_type && callee->expr_type->kind == TYPE_FUNCTION)
                    {
                        param_mem_quals = callee->expr_type->as.function.param_mem_quals;
                        param_count = callee->expr_type->as.function.param_count;
                    }
                }

                /* Collect args with types for the ThreadArgs struct */
                json_object *args = json_object_new_array();
                if (call_expr->type == EXPR_CALL)
                {
                    /* For method calls, add self as the first arg */
                    if (is_method_spawn)
                    {
                        json_object *self_arg = json_object_new_object();
                        Expr *callee = call_expr->as.call.callee;
                        json_object_object_add(self_arg, "type",
                            gen_model_type(arena, callee->as.member.object->expr_type));
                        json_object_object_add(self_arg, "name",
                            json_object_new_string("self_arg"));
                        json_object_array_add(args, self_arg);
                    }
                    for (int i = 0; i < call_expr->as.call.arg_count; i++)
                    {
                        json_object *arg = json_object_new_object();
                        Expr *arg_expr = call_expr->as.call.arguments[i];
                        json_object_object_add(arg, "type",
                            gen_model_type(arena, arg_expr->expr_type));
                        /* Create a unique arg name */
                        char arg_name[32];
                        snprintf(arg_name, sizeof(arg_name), "arg%d", i);
                        json_object_object_add(arg, "name",
                            json_object_new_string(arg_name));
                        /* Check if this param has as ref qualifier. 'as ref' on
                         * an already-refcounted struct is redundant — the arg
                         * is already a pointer, so don't add another level of
                         * indirection (matches the normalization in
                         * gen_model_emit_param_cleanup). */
                        if (param_mem_quals && i < param_count &&
                            param_mem_quals[i] == MEM_AS_REF)
                        {
                            Type *arg_type_for_ref = arg_expr->expr_type;
                            bool redundant_ref = (arg_type_for_ref &&
                                arg_type_for_ref->kind == TYPE_STRUCT &&
                                arg_type_for_ref->as.struct_type.pass_self_by_ref);
                            if (!redundant_ref)
                            {
                                json_object_object_add(arg, "is_ref",
                                    json_object_new_boolean(true));
                            }
                        }
                        /* Check if this param would be a borrow in the function signature.
                         * Thread wrapper must pass &args->argN for borrow params. */
                        {
                            Type *arg_type = arg_expr->expr_type;
                            bool callee_native = false;
                            Type *ctype = call_expr->as.call.callee->expr_type;
                            if (ctype && ctype->kind == TYPE_FUNCTION)
                                callee_native = ctype->as.function.is_native;
                            MemoryQualifier mq = (param_mem_quals && i < param_count)
                                ? param_mem_quals[i] : MEM_DEFAULT;
                            if (mq == MEM_DEFAULT && !callee_native &&
                                arg_type && gen_model_type_category(arg_type) == TYPE_CAT_COMPOSITE)
                            {
                                json_object_object_add(arg, "is_borrow",
                                    json_object_new_boolean(true));
                                /* Thread args need deep copy + cleanup for val-type
                                 * structs with heap fields (arrays, strings) to avoid
                                 * sharing pointers with the parent thread. */
                                json_object_object_add(arg, "needs_val_copy",
                                    json_object_new_boolean(true));
                                json_object_object_add(arg, "val_copy_name",
                                    json_object_new_string(arg_type->as.struct_type.name));
                            }
                        }
                        /* Thread string args from heap-producing expressions (interpolations,
                         * concatenations, method calls) must be freed in the wrapper after
                         * the function call, since the shallow args_copy keeps the pointer. */
                        if (is_heap_producing_string_expr(arg_expr) ||
                            is_named_fn_str_call(arg_expr, symbol_table))
                        {
                            json_object_object_add(arg, "needs_str_cleanup",
                                json_object_new_boolean(true));
                        }
                        /* Check if corresponding call arg is a fn_ref_arg (bare function
                         * reference wrapped in a heap-allocated __Closure__).  The thread
                         * wrapper needs this flag to free the closure after the call. */
                        json_object *call_args_arr = NULL;
                        json_object_object_get_ex(call_obj, "args", &call_args_arr);
                        if (call_args_arr)
                        {
                            json_object *call_arg = json_object_array_get_idx(call_args_arr, i);
                            json_object *fn_ref = NULL;
                            if (call_arg &&
                                json_object_object_get_ex(call_arg, "is_fn_ref_arg", &fn_ref) &&
                                json_object_get_boolean(fn_ref))
                            {
                                json_object_object_add(arg, "is_fn_ref_arg",
                                    json_object_new_boolean(true));
                            }
                        }
                        json_object_array_add(args, arg);
                    }
                }
                json_object_object_add(tdef, "args", args);
                json_object_object_add(tdef, "has_args",
                    json_object_new_boolean(
                        (call_expr->type == EXPR_CALL && call_expr->as.call.arg_count > 0) ||
                        is_method_spawn || is_closure_spawn));

                json_object_array_add(g_model_threads, tdef);
            }

            break;
        }

        case EXPR_THREAD_SYNC:
        {
            json_object_object_add(obj, "kind", json_object_new_string("thread_sync"));
            json_object_object_add(obj, "handle",
                gen_model_expr(arena, expr->as.thread_sync.handle, symbol_table, arithmetic_mode));
            json_object_object_add(obj, "is_array",
                json_object_new_boolean(expr->as.thread_sync.is_array));
            /* Add the result type for sync extraction */
            json_object_object_add(obj, "result_type",
                gen_model_type(arena, expr->expr_type));
            json_object_object_add(obj, "is_void",
                json_object_new_boolean(expr->expr_type->kind == TYPE_VOID));
            /* Struct results with heap fields need cleanup before overwrite */
            if (expr->expr_type->kind == TYPE_STRUCT &&
                gen_model_type_category(expr->expr_type) == TYPE_CAT_COMPOSITE)
            {
                json_object_object_add(obj, "needs_val_cleanup",
                    json_object_new_boolean(true));
            }

            /* Detect array sync patterns */
            Expr *sync_handle = expr->as.thread_sync.handle;
            if (sync_handle->type == EXPR_VARIABLE &&
                expr->expr_type && expr->expr_type->kind == TYPE_ARRAY)
            {
                /* Check if this variable has a companion __th__ (was directly assigned a spawn).
                 * If so, use normal variable sync, not array sync. */
                const char *vname = sync_handle->as.variable.name.start;
                bool has_th_companion = false;
                for (int ti = 0; ti < g_thread_handle_var_count; ti++)
                {
                    if (strcmp(g_thread_handle_vars[ti], vname) == 0)
                    {
                        has_th_companion = true;
                        break;
                    }
                }
                if (!has_th_companion)
                {
                    /* handles! — sync entire array of thread handles */
                    json_object_object_add(obj, "is_array_sync", json_object_new_boolean(true));
                    json_object_object_add(obj, "element_type",
                        gen_model_type(arena, expr->expr_type->as.array.element_type));
                }
            }
            else if (sync_handle->type == EXPR_ARRAY_ACCESS)
            {
                /* handles[i]! — sync single element from thread handle array */
                json_object_object_add(obj, "is_element_sync", json_object_new_boolean(true));
                json_object_object_add(obj, "element_type",
                    gen_model_type(arena, expr->expr_type));
            }
            break;
        }

        case EXPR_THREAD_DETACH:
        {
            json_object_object_add(obj, "kind", json_object_new_string("thread_detach"));
            json_object_object_add(obj, "handle",
                gen_model_expr(arena, expr->as.thread_detach.handle, symbol_table, arithmetic_mode));
            break;
        }

        case EXPR_SYNC_LIST:
        {
            json_object_object_add(obj, "kind", json_object_new_string("sync_list"));
            json_object *elements = json_object_new_array();
            for (int i = 0; i < expr->as.sync_list.element_count; i++)
            {
                Expr *elem = expr->as.sync_list.elements[i];
                json_object *elem_obj = gen_model_expr(arena, elem, symbol_table, arithmetic_mode);
                json_object_array_add(elements, elem_obj);
            }
            json_object_object_add(obj, "elements", elements);
            break;
        }

        case EXPR_ADDRESS_OF:
        {
            json_object_object_add(obj, "kind", json_object_new_string("address_of"));
            json_object_object_add(obj, "operand",
                gen_model_expr(arena, expr->as.address_of.operand, symbol_table, arithmetic_mode));
            /* For as ref structs, addressOf is a no-op (already pointer) */
            if (expr->as.address_of.operand->expr_type &&
                expr->as.address_of.operand->expr_type->kind == TYPE_STRUCT &&
                expr->as.address_of.operand->expr_type->as.struct_type.pass_self_by_ref)
            {
                json_object_object_add(obj, "is_noop", json_object_new_boolean(true));
            }
            /* For arrays, addressOf returns a pointer to the data buffer */
            else if (expr->as.address_of.operand->expr_type &&
                     expr->as.address_of.operand->expr_type->kind == TYPE_ARRAY)
            {
                json_object_object_add(obj, "is_array_data", json_object_new_boolean(true));
                Type *elem = expr->as.address_of.operand->expr_type->as.array.element_type;
                if (elem)
                    json_object_object_add(obj, "element_type", gen_model_type(arena, elem));
            }
            break;
        }

        case EXPR_VALUE_OF:
        {
            json_object_object_add(obj, "kind", json_object_new_string("value_of"));
            json_object_object_add(obj, "operand",
                gen_model_expr(arena, expr->as.value_of.operand, symbol_table, arithmetic_mode));
            /* For as ref structs, valueOf does a deep copy to produce a stack value */
            if (expr->as.value_of.operand->expr_type &&
                expr->as.value_of.operand->expr_type->kind == TYPE_STRUCT)
            {
                if (expr->as.value_of.operand->expr_type->as.struct_type.pass_self_by_ref)
                {
                    json_object_object_add(obj, "is_deep_copy", json_object_new_boolean(true));
                    json_object_object_add(obj, "type_name",
                        json_object_new_string(expr->as.value_of.operand->expr_type->as.struct_type.name));
                }
                else
                {
                    /* as val struct: valueOf is a no-op */
                    json_object_object_add(obj, "is_noop", json_object_new_boolean(true));
                }
            }
            /* valueOf on *char produces an owned string copy (strdup) */
            else if (expr->as.value_of.operand->expr_type &&
                     expr->as.value_of.operand->expr_type->kind == TYPE_POINTER &&
                     expr->as.value_of.operand->expr_type->as.pointer.base_type &&
                     expr->as.value_of.operand->expr_type->as.pointer.base_type->kind == TYPE_CHAR)
            {
                json_object_object_add(obj, "is_strdup", json_object_new_boolean(true));
            }
            /* valueOf on array (e.g. from pointer slice) is a no-op —
             * the slice already produces an owned SnArray* */
            else if (expr->as.value_of.operand->expr_type &&
                     expr->as.value_of.operand->expr_type->kind == TYPE_ARRAY)
            {
                json_object_object_add(obj, "is_noop", json_object_new_boolean(true));
            }
            break;
        }

        case EXPR_COPY_OF:
        {
            json_object_object_add(obj, "kind", json_object_new_string("copy_of"));
            json_object_object_add(obj, "operand",
                gen_model_expr(arena, expr->as.copy_of.operand, symbol_table, arithmetic_mode));
            break;
        }

        case EXPR_TYPEOF:
        {
            json_object_object_add(obj, "kind", json_object_new_string("typeof"));

            /* Resolve the operand's type at compile time */
            Type *op_type = expr->as.typeof_expr.operand->expr_type;

            /* Compute canonical type name */
            const char *type_name = "unknown";
            if (op_type)
            {
                switch (op_type->kind)
                {
                    case TYPE_INT:    type_name = "int"; break;
                    case TYPE_INT32:  type_name = "int32"; break;
                    case TYPE_UINT:   type_name = "uint"; break;
                    case TYPE_UINT32: type_name = "uint32"; break;
                    case TYPE_LONG:   type_name = "long"; break;
                    case TYPE_DOUBLE: type_name = "double"; break;
                    case TYPE_FLOAT:  type_name = "float"; break;
                    case TYPE_CHAR:   type_name = "char"; break;
                    case TYPE_STRING: type_name = "str"; break;
                    case TYPE_BOOL:   type_name = "bool"; break;
                    case TYPE_BYTE:   type_name = "byte"; break;
                    case TYPE_VOID:   type_name = "void"; break;
                    case TYPE_STRUCT: type_name = op_type->as.struct_type.name ? op_type->as.struct_type.name : "struct"; break;
                    case TYPE_ARRAY:  type_name = "array"; break;
                    default:          type_name = "unknown"; break;
                }
            }

            /* FNV-1a hash for type ID */
            unsigned long hash = 2166136261u;
            for (const char *p = type_name; *p; p++)
            {
                hash ^= (unsigned char)*p;
                hash *= 16777619u;
            }
            int type_id = (int)(hash & 0x7FFFFFFF);

            json_object_object_add(obj, "type_name", json_object_new_string(type_name));
            json_object_object_add(obj, "type_id", json_object_new_int(type_id));

            /* Build fields array (only for structs) */
            json_object *fields_arr = json_object_new_array();
            int field_count = 0;

            if (op_type && op_type->kind == TYPE_STRUCT)
            {
                field_count = op_type->as.struct_type.field_count;
                for (int i = 0; i < field_count; i++)
                {
                    StructField *f = &op_type->as.struct_type.fields[i];
                    json_object *field_obj = json_object_new_object();
                    json_object_object_add(field_obj, "name", json_object_new_string(f->name));

                    /* Compute field type name */
                    const char *field_type_name = "unknown";
                    if (f->type)
                    {
                        switch (f->type->kind)
                        {
                            case TYPE_INT:    field_type_name = "int"; break;
                            case TYPE_INT32:  field_type_name = "int32"; break;
                            case TYPE_UINT:   field_type_name = "uint"; break;
                            case TYPE_UINT32: field_type_name = "uint32"; break;
                            case TYPE_LONG:   field_type_name = "long"; break;
                            case TYPE_DOUBLE: field_type_name = "double"; break;
                            case TYPE_FLOAT:  field_type_name = "float"; break;
                            case TYPE_CHAR:   field_type_name = "char"; break;
                            case TYPE_STRING: field_type_name = "str"; break;
                            case TYPE_BOOL:   field_type_name = "bool"; break;
                            case TYPE_BYTE:   field_type_name = "byte"; break;
                            case TYPE_VOID:   field_type_name = "void"; break;
                            case TYPE_STRUCT: field_type_name = f->type->as.struct_type.name ? f->type->as.struct_type.name : "struct"; break;
                            case TYPE_ARRAY:  field_type_name = "array"; break;
                            default:          field_type_name = "unknown"; break;
                        }
                    }
                    json_object_object_add(field_obj, "type_name", json_object_new_string(field_type_name));

                    /* FNV-1a hash for field type ID */
                    unsigned long fhash = 2166136261u;
                    for (const char *p = field_type_name; *p; p++)
                    {
                        fhash ^= (unsigned char)*p;
                        fhash *= 16777619u;
                    }
                    json_object_object_add(field_obj, "type_id", json_object_new_int((int)(fhash & 0x7FFFFFFF)));

                    json_object_array_add(fields_arr, field_obj);
                }
            }

            json_object_object_add(obj, "fields", fields_arr);
            json_object_object_add(obj, "field_count", json_object_new_int(field_count));
            break;
        }

        case EXPR_SIZEOF:
        {
            json_object_object_add(obj, "kind", json_object_new_string("sizeof"));
            if (expr->as.sizeof_expr.type_operand)
            {
                json_object_object_add(obj, "target_type",
                    gen_model_type(arena, expr->as.sizeof_expr.type_operand));
            }
            if (expr->as.sizeof_expr.expr_operand)
            {
                json_object_object_add(obj, "operand",
                    gen_model_expr(arena, expr->as.sizeof_expr.expr_operand, symbol_table, arithmetic_mode));
                /* Also set target_type from the expression's resolved type */
                if (!expr->as.sizeof_expr.type_operand && expr->as.sizeof_expr.expr_operand->expr_type)
                {
                    json_object_object_add(obj, "target_type",
                        gen_model_type(arena, expr->as.sizeof_expr.expr_operand->expr_type));
                }
            }
            break;
        }
    }

    /* Escape info */
    if (expr->escape_info.escapes_scope || expr->escape_info.needs_heap_allocation)
    {
        json_object *esc = json_object_new_object();
        json_object_object_add(esc, "escapes_scope",
            json_object_new_boolean(expr->escape_info.escapes_scope));
        json_object_object_add(esc, "needs_heap",
            json_object_new_boolean(expr->escape_info.needs_heap_allocation));
        json_object_object_add(obj, "escape_info", esc);
    }

    return obj;
}
