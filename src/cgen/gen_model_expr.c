#include "cgen/gen_model.h"
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
        /* Member method calls (obj.method()) always return owned strings.
         * This covers string built-ins (s.toLower()), array methods
         * (arr.toString()), and struct methods returning string. */
        case EXPR_CALL:
            return (expr->as.call.callee &&
                    expr->as.call.callee->type == EXPR_MEMBER);
        default:
            return false;
    }
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
                bool assign_needs_strdup = false;
                bool assign_needs_retain = false;

                switch (atype->kind)
                {
                    case TYPE_STRING:
                        assign_cleanup = "free_str";
                        /* RHS literal/variable → needs strdup; function return/interpolation → owned */
                        {
                            Expr *val = expr->as.assign.value;
                            ExprType vt = val->type;
                            bool is_nil = (vt == EXPR_LITERAL && val->as.literal.type &&
                                           val->as.literal.type->kind == TYPE_NIL);
                            if (!is_nil && (vt == EXPR_LITERAL || vt == EXPR_VARIABLE ||
                                            vt == EXPR_ARRAY_ACCESS || vt == EXPR_MEMBER))
                                assign_needs_strdup = true;
                        }
                        break;
                    case TYPE_ARRAY:
                        assign_cleanup = "cleanup_arr";
                        break;
                    case TYPE_STRUCT:
                        if (atype->as.struct_type.pass_self_by_ref)
                        {
                            assign_cleanup = "release_ref";
                            /* Variable RHS is borrowed → need retain */
                            if (expr->as.assign.value->type == EXPR_VARIABLE)
                                assign_needs_retain = true;
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
                    default:
                        break;
                }
                json_object_object_add(obj, "assign_cleanup",
                    json_object_new_string(assign_cleanup));
                if (assign_needs_strdup)
                    json_object_object_add(obj, "needs_strdup",
                        json_object_new_boolean(true));
                if (assign_needs_retain)
                    json_object_object_add(obj, "needs_retain",
                        json_object_new_boolean(true));
                /* Val struct from variable needs deep copy to avoid shared heap fields */
                if (strcmp(assign_cleanup, "cleanup_val") == 0 &&
                    expr->as.assign.value->type == EXPR_VARIABLE)
                {
                    json_object_object_add(obj, "needs_val_copy",
                        json_object_new_boolean(true));
                }
                /* Array from variable/member needs copy to avoid double-free */
                if (strcmp(assign_cleanup, "cleanup_arr") == 0 &&
                    (expr->as.assign.value->type == EXPR_VARIABLE ||
                     expr->as.assign.value->type == EXPR_MEMBER))
                {
                    json_object_object_add(obj, "needs_arr_copy",
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
                if (is_heap_producing_string_expr(expr->as.binary.left))
                    json_object_object_add(left_obj, "is_str_temp",
                        json_object_new_boolean(true));
                if (is_heap_producing_string_expr(expr->as.binary.right))
                    json_object_object_add(right_obj, "is_str_temp",
                        json_object_new_boolean(true));
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
                json_object_object_add(obj, "args", args);
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
                            /* Exclude array built-ins and fn-field calls from borrow.
                             * Struct method calls through EXPR_CALL keep borrow. */
                            if (expr->as.call.callee->type == EXPR_MEMBER)
                            {
                                Expr *callee_obj = expr->as.call.callee->as.member.object;
                                if (callee_obj && callee_obj->expr_type)
                                {
                                    Type *obj_type = callee_obj->expr_type;
                                    if (obj_type->kind == TYPE_ARRAY)
                                        callee_is_known_fn = false;
                                    else
                                    {
                                        /* Resolve through pointer for fn-field check */
                                        Type *st = obj_type;
                                        if (st->kind == TYPE_POINTER && st->as.pointer.base_type)
                                            st = st->as.pointer.base_type;
                                        if (st->kind == TYPE_STRUCT)
                                        {
                                            const char *mname = expr->as.call.callee->as.member.member_name.start;
                                            int mlen = expr->as.call.callee->as.member.member_name.length;
                                            for (int fi = 0; fi < st->as.struct_type.field_count; fi++)
                                            {
                                                StructField *sf = &st->as.struct_type.fields[fi];
                                                if (sf->type && sf->type->kind == TYPE_FUNCTION &&
                                                    (int)strlen(sf->name) == mlen &&
                                                    strncmp(sf->name, mname, mlen) == 0)
                                                {
                                                    callee_is_known_fn = false;
                                                    break;
                                                }
                                            }
                                        }
                                    }
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
                     * These need wrapping in a __Closure__ struct at the call site,
                     * with a wrapper function that adapts the calling convention. */
                    if (callee_param_types && i < callee_param_count &&
                        callee_param_types[i] && callee_param_types[i]->kind == TYPE_FUNCTION)
                    {
                        Expr *arg_expr = expr->as.call.arguments[i];
                        if (arg_expr && arg_expr->type == EXPR_VARIABLE)
                        {
                            Symbol *arg_sym = symbol_table_lookup_symbol(symbol_table,
                                arg_expr->as.variable.name);
                            if (arg_sym && arg_sym->is_function &&
                                arg_sym->kind != SYMBOL_PARAM &&
                                !arg_expr->as.variable.is_param_ref)
                            {
                                /* Bare function references must be wrapped in closures
                                 * for uniform calling convention.  Parameter references
                                 * (is_param_ref) are already closures — re-wrapping
                                 * leaks the intermediate closure. */
                                {
                                /* Generate a wrapper function that adapts the calling convention.
                                 * The wrapper accepts (void *__closure__, params...) and forwards
                                 * to the named function with just (params...). */
                                int wrap_id = g_model_fn_wrapper_count++;
                                json_object *wrapper = json_object_new_object();
                                json_object_object_add(wrapper, "wrapper_id",
                                    json_object_new_int(wrap_id));
                                json_object_object_add(wrapper, "target_name",
                                    json_object_new_string(arg_expr->as.variable.name.start));
                                Type *fn_type = callee_param_types[i];
                                json_object_object_add(wrapper, "return_type",
                                    gen_model_type(arena, fn_type->as.function.return_type));
                                json_object *wrap_params = json_object_new_array();
                                /* Resolve the target function's actual param info to detect borrow.
                                 * Look up the target function in module stmts to get Parameter data. */
                                bool target_is_native = fn_type->as.function.is_native;
                                bool has_borrow = false;
                                for (int p = 0; p < fn_type->as.function.param_count; p++)
                                {
                                    json_object *pt = gen_model_type(arena, fn_type->as.function.param_types[p]);
                                    /* Mark borrow params: non-native target, MEM_DEFAULT or MEM_AS_REF, COMPOSITE type */
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
                                    json_object_new_boolean(fn_type->as.function.is_native));
                                if (has_borrow)
                                    json_object_object_add(wrapper, "has_borrow_cleanup",
                                        json_object_new_boolean(true));
                                json_object_array_add(g_model_fn_wrappers, wrapper);

                                json_object_object_add(arg, "is_fn_ref_arg",
                                    json_object_new_boolean(true));
                                json_object_object_add(arg, "fn_wrapper_id",
                                    json_object_new_int(wrap_id));
                                } /* end else (callee_calls_param) */
                            }
                        }
                    }
                    /* String array push/insert: args need strdup for ownership */
                    if (member_str_push && i == 0)
                    {
                        json_object_object_add(arg, "needs_strdup",
                            json_object_new_boolean(true));
                    }
                    /* Array-of-arrays push/insert: args need sn_array_copy for ownership */
                    if (member_arr_push && i == 0)
                    {
                        json_object_object_add(arg, "needs_arr_copy",
                            json_object_new_boolean(true));
                    }
                    /* Composite struct array push/insert: lvalue args need deep copy for ownership */
                    if (member_struct_push && i == 0)
                    {
                        Expr *arg_expr = expr->as.call.arguments[0];
                        bool is_lvalue = (arg_expr->type == EXPR_VARIABLE ||
                                          arg_expr->type == EXPR_ARRAY_ACCESS ||
                                          arg_expr->type == EXPR_MEMBER ||
                                          arg_expr->type == EXPR_MEMBER_ACCESS);
                        if (is_lvalue)
                        {
                            json_object_object_add(arg, "needs_struct_copy",
                                json_object_new_boolean(true));
                            json_object_object_add(arg, "copy_struct_name",
                                json_object_new_string(member_struct_push_name));
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

        case EXPR_ARRAY:
        {
            json_object_object_add(obj, "kind", json_object_new_string("array_literal"));
            json_object *elements = json_object_new_array();
            for (int i = 0; i < expr->as.array.element_count; i++)
            {
                json_object *elem = gen_model_expr(arena, expr->as.array.elements[i], symbol_table, arithmetic_mode);
                /* String array elements from literals/variables need strdup */
                Expr *ae = expr->as.array.elements[i];
                if (ae && ae->expr_type && ae->expr_type->kind == TYPE_STRING)
                {
                    bool is_nil = (ae->type == EXPR_LITERAL && ae->as.literal.type &&
                                   ae->as.literal.type->kind == TYPE_NIL);
                    if (!is_nil && (ae->type == EXPR_LITERAL || ae->type == EXPR_VARIABLE ||
                                    ae->type == EXPR_ARRAY_ACCESS || ae->type == EXPR_MEMBER))
                        json_object_object_add(elem, "needs_strdup", json_object_new_boolean(true));
                }
                /* Array-of-arrays: variable/member elements need sn_array_copy */
                if (ae && ae->expr_type && ae->expr_type->kind == TYPE_ARRAY &&
                    (ae->type == EXPR_VARIABLE || ae->type == EXPR_MEMBER))
                {
                    json_object_object_add(elem, "needs_arr_copy", json_object_new_boolean(true));
                }
                /* Composite struct arrays: lvalue elements need deep copy */
                if (ae && ae->expr_type && ae->expr_type->kind == TYPE_STRUCT &&
                    !ae->expr_type->as.struct_type.pass_self_by_ref &&
                    gen_model_type_category(ae->expr_type) == TYPE_CAT_COMPOSITE &&
                    (ae->type == EXPR_VARIABLE || ae->type == EXPR_ARRAY_ACCESS ||
                     ae->type == EXPR_MEMBER || ae->type == EXPR_MEMBER_ACCESS))
                {
                    json_object_object_add(elem, "needs_struct_copy", json_object_new_boolean(true));
                    json_object_object_add(elem, "copy_struct_name",
                        json_object_new_string(ae->expr_type->as.struct_type.name));
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
                bool ia_needs_strdup = false;

                switch (etype->kind)
                {
                    case TYPE_STRING:
                        elem_cleanup = "free_str";
                        {
                            Expr *val = expr->as.index_assign.value;
                            ExprType vt = val->type;
                            bool is_nil = (vt == EXPR_LITERAL && val->as.literal.type &&
                                           val->as.literal.type->kind == TYPE_NIL);
                            if (!is_nil && (vt == EXPR_LITERAL || vt == EXPR_VARIABLE ||
                                            vt == EXPR_ARRAY_ACCESS || vt == EXPR_MEMBER))
                                ia_needs_strdup = true;
                        }
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
                    default:
                        break;
                }
                json_object_object_add(obj, "elem_cleanup",
                    json_object_new_string(elem_cleanup));
                if (ia_needs_strdup)
                    json_object_object_add(obj, "needs_strdup",
                        json_object_new_boolean(true));
            }
            break;
        }

        case EXPR_MEMBER:
        {
            /* Detect builtin property access: str.length, array.length */
            const char *mname = expr->as.member.member_name.start;
            Type *obj_type = expr->as.member.object ? expr->as.member.object->expr_type : NULL;
            if (obj_type && strcmp(mname, "length") == 0 &&
                (obj_type->kind == TYPE_STRING || obj_type->kind == TYPE_ARRAY))
            {
                json_object_object_add(obj, "kind", json_object_new_string("builtin_length"));
                json_object_object_add(obj, "object",
                    gen_model_expr(arena, expr->as.member.object, symbol_table, arithmetic_mode));
            }
            else
            {
                json_object_object_add(obj, "kind", json_object_new_string("member"));
                json_object_object_add(obj, "object",
                    gen_model_expr(arena, expr->as.member.object, symbol_table, arithmetic_mode));
                json_object_object_add(obj, "member_name",
                    json_object_new_string(mname));
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
                bool ma_needs_strdup = false;
                bool ma_needs_retain = false;

                switch (ftype->kind)
                {
                    case TYPE_STRING:
                        field_cleanup = "free_str";
                        {
                            Expr *val = expr->as.member_assign.value;
                            ExprType vt = val->type;
                            bool is_nil = (vt == EXPR_LITERAL && val->as.literal.type &&
                                           val->as.literal.type->kind == TYPE_NIL);
                            if (!is_nil && (vt == EXPR_LITERAL || vt == EXPR_VARIABLE ||
                                            vt == EXPR_ARRAY_ACCESS || vt == EXPR_MEMBER))
                                ma_needs_strdup = true;
                        }
                        break;
                    case TYPE_ARRAY:
                        field_cleanup = "cleanup_arr";
                        /* Array from variable/member needs copy to avoid double-free */
                        {
                            Expr *val = expr->as.member_assign.value;
                            if (val->type == EXPR_VARIABLE || val->type == EXPR_MEMBER)
                            {
                                json_object_object_add(obj, "needs_arr_copy",
                                    json_object_new_boolean(true));
                            }
                        }
                        break;
                    case TYPE_STRUCT:
                        if (ftype->as.struct_type.pass_self_by_ref)
                        {
                            field_cleanup = "release_ref";
                            if (expr->as.member_assign.value->type == EXPR_VARIABLE)
                                ma_needs_retain = true;
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
                            Expr *val = expr->as.member_assign.value;
                            if (val && val->type == EXPR_VARIABLE)
                            {
                                Symbol *sym = symbol_table ? symbol_table_lookup_symbol(symbol_table, val->as.variable.name) : NULL;
                                if (sym && sym->is_function && sym->kind != SYMBOL_PARAM &&
                                    !val->as.variable.is_param_ref)
                                {
                                    /* Create a wrapper function for the bare function reference */
                                    Type *fn_type = ftype;
                                    int wrap_id = g_model_fn_wrapper_count++;
                                    json_object *wrapper = json_object_new_object();
                                    json_object_object_add(wrapper, "wrapper_id", json_object_new_int(wrap_id));
                                    json_object_object_add(wrapper, "target_name",
                                        json_object_new_string(val->as.variable.name.start));
                                    json_object_object_add(wrapper, "return_type",
                                        gen_model_type(arena, fn_type->as.function.return_type));
                                    json_object *wrap_params = json_object_new_array();
                                    bool tgt_native2 = fn_type->as.function.is_native;
                                    bool has_borrow2 = false;
                                    for (int p = 0; p < fn_type->as.function.param_count; p++)
                                    {
                                        json_object *pt = gen_model_type(arena, fn_type->as.function.param_types[p]);
                                        if (!tgt_native2)
                                        {
                                            Type *ptype = fn_type->as.function.param_types[p];
                                            MemoryQualifier pmq_val = MEM_DEFAULT;
                                            if (fn_type->as.function.param_mem_quals)
                                                pmq_val = fn_type->as.function.param_mem_quals[p];
                                            if ((pmq_val == MEM_DEFAULT || pmq_val == MEM_AS_REF) && ptype &&
                                                gen_model_type_category(ptype) == TYPE_CAT_COMPOSITE)
                                            {
                                                json_object_object_add(pt, "is_borrow", json_object_new_boolean(true));
                                                has_borrow2 = true;
                                            }
                                        }
                                        json_object_array_add(wrap_params, pt);
                                    }
                                    json_object_object_add(wrapper, "param_types", wrap_params);
                                    json_object_object_add(wrapper, "is_native",
                                        json_object_new_boolean(fn_type->as.function.is_native));
                                    if (has_borrow2)
                                        json_object_object_add(wrapper, "has_borrow_cleanup",
                                            json_object_new_boolean(true));
                                    json_object_array_add(g_model_fn_wrappers, wrapper);

                                    json_object_object_add(obj, "needs_closure_wrap", json_object_new_boolean(true));
                                    json_object_object_add(obj, "fn_wrapper_id", json_object_new_int(wrap_id));
                                }
                            }
                        }
                        break;
                    default:
                        break;
                }
                json_object_object_add(obj, "field_cleanup",
                    json_object_new_string(field_cleanup));
                if (ma_needs_strdup)
                    json_object_object_add(obj, "needs_strdup",
                        json_object_new_boolean(true));
                if (ma_needs_retain)
                    json_object_object_add(obj, "needs_retain",
                        json_object_new_boolean(true));
                /* Val struct from variable/member needs deep copy to avoid shared heap fields */
                if (strcmp(field_cleanup, "cleanup_val") == 0 &&
                    (expr->as.member_assign.value->type == EXPR_VARIABLE ||
                     expr->as.member_assign.value->type == EXPR_MEMBER))
                {
                    json_object_object_add(obj, "needs_val_copy",
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
                if (fv && fv->expr_type && fv->expr_type->kind == TYPE_STRING)
                {
                    bool needs_strdup = (fv->type == EXPR_LITERAL || fv->type == EXPR_VARIABLE ||
                                         fv->type == EXPR_MEMBER || fv->type == EXPR_ARRAY_ACCESS);
                    json_object_object_add(f, "needs_strdup",
                        json_object_new_boolean(needs_strdup));
                }
                /* Ref struct fields from variables need retain */
                if (fv && fv->expr_type && fv->expr_type->kind == TYPE_STRUCT &&
                    fv->expr_type->as.struct_type.pass_self_by_ref &&
                    fv->type == EXPR_VARIABLE)
                {
                    json_object_object_add(f, "needs_retain",
                        json_object_new_boolean(true));
                    json_object_object_add(f, "retain_type_name",
                        json_object_new_string(fv->expr_type->as.struct_type.name));
                }
                /* Val struct fields from variables/members/accesses need deep copy
                 * to transfer ownership (avoid double-free when source is cleaned up) */
                if (fv && fv->expr_type &&
                    gen_model_type_category(fv->expr_type) == TYPE_CAT_COMPOSITE &&
                    (fv->type == EXPR_VARIABLE || fv->type == EXPR_MEMBER ||
                     fv->type == EXPR_ARRAY_ACCESS))
                {
                    {
                        json_object_object_add(f, "needs_struct_copy",
                            json_object_new_boolean(true));
                        json_object_object_add(f, "copy_type_name",
                            json_object_new_string(fv->expr_type->as.struct_type.name));
                    }
                }
                /* Array fields from variables/members need deep copy to avoid
                 * double-free — the local variable and the struct field would
                 * otherwise share the same SnArray * with both having cleanup. */
                if (fv && fv->expr_type && fv->expr_type->kind == TYPE_ARRAY &&
                    (fv->type == EXPR_VARIABLE || fv->type == EXPR_MEMBER ||
                     fv->type == EXPR_ARRAY_ACCESS))
                {
                    json_object_object_add(f, "needs_arr_copy",
                        json_object_new_boolean(true));
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
                if (fv && fv->expr_type && fv->expr_type->kind == TYPE_FUNCTION && fv->type == EXPR_VARIABLE)
                {
                    Symbol *sym = symbol_table ? symbol_table_lookup_symbol(symbol_table, fv->as.variable.name) : NULL;
                    if (sym && sym->is_function && sym->kind != SYMBOL_PARAM &&
                        !fv->as.variable.is_param_ref)
                    {
                        /* Create a wrapper function (same as is_fn_ref_arg) */
                        Type *fn_type = fv->expr_type;
                        int wrap_id = g_model_fn_wrapper_count++;
                        json_object *wrapper = json_object_new_object();
                        json_object_object_add(wrapper, "wrapper_id", json_object_new_int(wrap_id));
                        json_object_object_add(wrapper, "target_name",
                            json_object_new_string(fv->as.variable.name.start));
                        json_object_object_add(wrapper, "return_type",
                            gen_model_type(arena, fn_type->as.function.return_type));
                        json_object *wrap_params = json_object_new_array();
                        bool tgt_native3 = fn_type->as.function.is_native;
                        bool has_borrow3 = false;
                        for (int p = 0; p < fn_type->as.function.param_count; p++)
                        {
                            json_object *pt = gen_model_type(arena, fn_type->as.function.param_types[p]);
                            if (!tgt_native3)
                            {
                                Type *ptype = fn_type->as.function.param_types[p];
                                MemoryQualifier pmq_val = MEM_DEFAULT;
                                if (fn_type->as.function.param_mem_quals)
                                    pmq_val = fn_type->as.function.param_mem_quals[p];
                                if ((pmq_val == MEM_DEFAULT || pmq_val == MEM_AS_REF) && ptype &&
                                    gen_model_type_category(ptype) == TYPE_CAT_COMPOSITE)
                                {
                                    json_object_object_add(pt, "is_borrow", json_object_new_boolean(true));
                                    has_borrow3 = true;
                                }
                            }
                            json_object_array_add(wrap_params, pt);
                        }
                        json_object_object_add(wrapper, "param_types", wrap_params);
                        json_object_object_add(wrapper, "is_native",
                            json_object_new_boolean(fn_type->as.function.is_native));
                        if (has_borrow3)
                            json_object_object_add(wrapper, "has_borrow_cleanup",
                                json_object_new_boolean(true));
                        json_object_array_add(g_model_fn_wrappers, wrapper);

                        json_object_object_add(f, "needs_closure_wrap", json_object_new_boolean(true));
                        json_object_object_add(f, "fn_wrapper_id", json_object_new_int(wrap_id));
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
                }
                if (expr->as.interpol.format_specs && expr->as.interpol.format_specs[i])
                {
                    json_object_object_add(part, "format_spec",
                        json_object_new_string(expr->as.interpol.format_specs[i]));
                }
                json_object_array_add(parts, part);
            }
            json_object_object_add(obj, "parts", parts);
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
                json_object_array_add(captures, cap);
            }
            json_object_object_add(obj, "captures", captures);
            json_object_object_add(obj, "has_captures",
                json_object_new_boolean(caps.count > 0));
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
                }
                g_in_lambda_body = prev_in_lambda;
            }

            /* Add to global lambdas collection for forward decls and function defs */
            if (g_model_lambdas != NULL)
            {
                json_object *ldef = json_object_new_object();
                json_object_object_add(ldef, "lambda_id", json_object_new_int(lam->lambda_id));
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
                    json_object_array_add(lcaps, lc);
                }
                json_object_object_add(ldef, "captures", lcaps);
                json_object_object_add(ldef, "has_captures",
                    json_object_new_boolean(caps.count > 0));
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
                    }
                    g_in_lambda_body = prev_in_lambda;
                }

                json_object_array_add(g_model_lambdas, ldef);
            }
            break;
        }

        case EXPR_MATCH:
        {
            json_object_object_add(obj, "kind", json_object_new_string("match"));
            json_object_object_add(obj, "subject",
                gen_model_expr(arena, expr->as.match_expr.subject, symbol_table, arithmetic_mode));
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
                        /* Check if this param has as ref qualifier */
                        if (param_mem_quals && i < param_count &&
                            param_mem_quals[i] == MEM_AS_REF)
                        {
                            json_object_object_add(arg, "is_ref",
                                json_object_new_boolean(true));
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
