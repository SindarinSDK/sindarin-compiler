#include "cgen/gen_model.h"
#include <string.h>

/* ============================================================================
 * Pre-scan: identify outer-scope variables captured by lambdas that need
 * handle-based (promoted) storage for capture-by-reference semantics.
 * ============================================================================ */

/* Scope depth of the symbol table at the point prescan_function_body starts,
 * before it pushes its own scope.  Variables declared at this depth or below
 * are module-level globals emitted as C globals — they are always reachable by
 * name from any function and must never be heap-promoted for ref-capture. */
int g_prescan_function_entry_depth = 0;

static bool prescan_needs_ref(Type *type)
{
    if (!type) return false;
    switch (type->kind) {
    case TYPE_INT: case TYPE_LONG: case TYPE_DOUBLE: case TYPE_BOOL:
    case TYPE_BYTE: case TYPE_CHAR: case TYPE_ARRAY:
        return true;
    default:
        return false;
    }
}

static void prescan_add_captured(Arena *arena, const char *name)
{
    for (int i = 0; i < g_captured_var_count; i++)
        if (strcmp(g_captured_vars[i], name) == 0) return;
    /* Simple grow - allocates from arena */
    if (g_captured_var_count % 8 == 0) {
        char **nv = arena_alloc(arena, (g_captured_var_count + 8) * sizeof(char *));
        for (int i = 0; i < g_captured_var_count; i++) nv[i] = g_captured_vars[i];
        g_captured_vars = nv;
    }
    g_captured_vars[g_captured_var_count++] = arena_strdup(arena, name);
}

/* Forward declarations.
 * lambda_scope_depth: -1 when not inside any lambda; otherwise the scope_depth
 * recorded just before the innermost enclosing lambda pushed its scope.  This
 * lets prescan_is_outer_var compare a symbol's declaration depth against the
 * lambda boundary to decide if it's a true capture. */
static void prescan_expr(Arena *arena, Expr *expr, SymbolTable *table, int lambda_scope_depth);
static void prescan_stmt(Arena *arena, Stmt *stmt, SymbolTable *table, int lambda_scope_depth);

/* Check if a mutated variable is from an outer scope (declared before the
 * innermost lambda boundary).  lambda_scope_depth is the scope_depth recorded
 * just before the innermost lambda pushed its own scope.  If the symbol's
 * declaration_scope_depth is less than that value, it was declared outside the
 * lambda and is a true capture. */
static bool prescan_is_outer_var(SymbolTable *table, Token name, int lambda_scope_depth)
{
    if (lambda_scope_depth < 0) return false;
    Symbol *sym = symbol_table_lookup_symbol(table, name);
    if (!sym) return true; /* not found in any scope — assume outer */
    /* Variables declared at or before the function entry depth are module-level
     * globals emitted as C globals — they are reachable by name from any function
     * and must never be heap-promoted for ref-capture. */
    if (sym->declaration_scope_depth <= g_prescan_function_entry_depth) return false;
    return sym->declaration_scope_depth <= lambda_scope_depth;
}

static void prescan_expr(Arena *arena, Expr *expr, SymbolTable *table, int lambda_scope_depth)
{
    if (!expr) return;
    switch (expr->type) {
    case EXPR_LAMBDA: {
        LambdaExpr *lam = &expr->as.lambda;
        /* Record the scope depth before pushing the lambda's scope.  Variables
         * declared at this depth or earlier are "outer" to the lambda.  For
         * nested lambdas, we keep the outermost lambda_scope_depth so that
         * only true outer-function variables get promoted. */
        int lsd = lambda_scope_depth >= 0 ? lambda_scope_depth
                                          : symbol_table_get_scope_depth(table);
        symbol_table_push_scope(table);
        for (int i = 0; i < lam->param_count; i++)
            symbol_table_add_symbol(table, lam->params[i].name, lam->params[i].type);
        if (lam->has_stmt_body) {
            for (int i = 0; i < lam->body_stmt_count; i++)
                prescan_stmt(arena, lam->body_stmts[i], table, lsd);
        }
        if (lam->body)
            prescan_expr(arena, lam->body, table, lsd);
        symbol_table_pop_scope(table);
        break;
    }
    case EXPR_VARIABLE:
        /* Only reading — no promotion needed */
        break;
    case EXPR_ASSIGN:
        prescan_expr(arena, expr->as.assign.value, table, lambda_scope_depth);
        /* Assignment target is mutated — needs promotion if it's an outer variable */
        if (lambda_scope_depth >= 0 && expr->expr_type && prescan_needs_ref(expr->expr_type)) {
            if (prescan_is_outer_var(table, expr->as.assign.name, lambda_scope_depth)) {
                char name[256];
                int len = expr->as.assign.name.length < 255 ? expr->as.assign.name.length : 255;
                strncpy(name, expr->as.assign.name.start, len);
                name[len] = '\0';
                prescan_add_captured(arena, name);
            }
        }
        break;
    case EXPR_COMPOUND_ASSIGN:
        /* Compound assignment target is mutated — needs promotion if it's an outer variable */
        if (lambda_scope_depth >= 0 && expr->as.compound_assign.target->type == EXPR_VARIABLE) {
            Expr *tgt = expr->as.compound_assign.target;
            if (tgt->expr_type && prescan_needs_ref(tgt->expr_type)) {
                if (prescan_is_outer_var(table, tgt->as.variable.name, lambda_scope_depth)) {
                    char name[256];
                    int len = tgt->as.variable.name.length < 255 ? tgt->as.variable.name.length : 255;
                    strncpy(name, tgt->as.variable.name.start, len);
                    name[len] = '\0';
                    prescan_add_captured(arena, name);
                }
            }
        }
        prescan_expr(arena, expr->as.compound_assign.target, table, lambda_scope_depth);
        prescan_expr(arena, expr->as.compound_assign.value, table, lambda_scope_depth);
        break;
    case EXPR_BINARY:
        prescan_expr(arena, expr->as.binary.left, table, lambda_scope_depth);
        prescan_expr(arena, expr->as.binary.right, table, lambda_scope_depth);
        break;
    case EXPR_UNARY:
        prescan_expr(arena, expr->as.unary.operand, table, lambda_scope_depth);
        break;
    case EXPR_CALL:
        prescan_expr(arena, expr->as.call.callee, table, lambda_scope_depth);
        for (int i = 0; i < expr->as.call.arg_count; i++)
            prescan_expr(arena, expr->as.call.arguments[i], table, lambda_scope_depth);
        break;
    case EXPR_MEMBER:
        prescan_expr(arena, expr->as.member.object, table, lambda_scope_depth);
        break;
    case EXPR_ARRAY_ACCESS:
        prescan_expr(arena, expr->as.array_access.array, table, lambda_scope_depth);
        prescan_expr(arena, expr->as.array_access.index, table, lambda_scope_depth);
        break;
    case EXPR_INCREMENT:
    case EXPR_DECREMENT:
        /* Increment/decrement mutates the target */
        if (lambda_scope_depth >= 0 && expr->as.operand && expr->as.operand->type == EXPR_VARIABLE) {
            Expr *op = expr->as.operand;
            if (op->expr_type && prescan_needs_ref(op->expr_type)) {
                if (prescan_is_outer_var(table, op->as.variable.name, lambda_scope_depth)) {
                    char name[256];
                    int len = op->as.variable.name.length < 255 ? op->as.variable.name.length : 255;
                    strncpy(name, op->as.variable.name.start, len);
                    name[len] = '\0';
                    prescan_add_captured(arena, name);
                }
            }
        }
        prescan_expr(arena, expr->as.operand, table, lambda_scope_depth);
        break;
    case EXPR_INTERPOLATED:
        for (int i = 0; i < expr->as.interpol.part_count; i++)
            prescan_expr(arena, expr->as.interpol.parts[i], table, lambda_scope_depth);
        break;
    case EXPR_METHOD_CALL:
        if (expr->as.method_call.object)
            prescan_expr(arena, expr->as.method_call.object, table, lambda_scope_depth);
        for (int i = 0; i < expr->as.method_call.arg_count; i++)
            prescan_expr(arena, expr->as.method_call.args[i], table, lambda_scope_depth);
        break;
    case EXPR_MEMBER_ACCESS:
        prescan_expr(arena, expr->as.member_access.object, table, lambda_scope_depth);
        break;
    case EXPR_ARRAY:
        for (int i = 0; i < expr->as.array.element_count; i++)
            prescan_expr(arena, expr->as.array.elements[i], table, lambda_scope_depth);
        break;
    case EXPR_INDEX_ASSIGN:
        prescan_expr(arena, expr->as.index_assign.array, table, lambda_scope_depth);
        prescan_expr(arena, expr->as.index_assign.index, table, lambda_scope_depth);
        prescan_expr(arena, expr->as.index_assign.value, table, lambda_scope_depth);
        break;
    case EXPR_ARRAY_SLICE:
        prescan_expr(arena, expr->as.array_slice.array, table, lambda_scope_depth);
        if (expr->as.array_slice.start) prescan_expr(arena, expr->as.array_slice.start, table, lambda_scope_depth);
        if (expr->as.array_slice.end) prescan_expr(arena, expr->as.array_slice.end, table, lambda_scope_depth);
        if (expr->as.array_slice.step) prescan_expr(arena, expr->as.array_slice.step, table, lambda_scope_depth);
        break;
    case EXPR_RANGE:
        prescan_expr(arena, expr->as.range.start, table, lambda_scope_depth);
        prescan_expr(arena, expr->as.range.end, table, lambda_scope_depth);
        break;
    case EXPR_SPREAD:
        prescan_expr(arena, expr->as.spread.array, table, lambda_scope_depth);
        break;
    case EXPR_MATCH:
        prescan_expr(arena, expr->as.match_expr.subject, table, lambda_scope_depth);
        for (int i = 0; i < expr->as.match_expr.arm_count; i++) {
            MatchArm *arm = &expr->as.match_expr.arms[i];
            if (!arm->is_else)
                for (int j = 0; j < arm->pattern_count; j++)
                    prescan_expr(arena, arm->patterns[j], table, lambda_scope_depth);
            if (arm->body)
                prescan_stmt(arena, arm->body, table, lambda_scope_depth);
        }
        break;
    case EXPR_STATIC_CALL:
        for (int i = 0; i < expr->as.static_call.arg_count; i++)
            prescan_expr(arena, expr->as.static_call.arguments[i], table, lambda_scope_depth);
        break;
    default:
        break;
    }
}

static void prescan_stmt(Arena *arena, Stmt *stmt, SymbolTable *table, int lambda_scope_depth)
{
    if (!stmt) return;
    switch (stmt->type) {
    case STMT_VAR_DECL:
        if (stmt->as.var_decl.initializer)
            prescan_expr(arena, stmt->as.var_decl.initializer, table, lambda_scope_depth);
        /* Register this variable in the current scope so inner lambdas can
         * distinguish it from outer-scope variables with the same name. */
        symbol_table_add_symbol_full(table, stmt->as.var_decl.name, stmt->as.var_decl.type,
                                     SYMBOL_LOCAL, stmt->as.var_decl.mem_qualifier);
        break;
    case STMT_EXPR:
        prescan_expr(arena, stmt->as.expression.expression, table, lambda_scope_depth);
        break;
    case STMT_RETURN:
        if (stmt->as.return_stmt.value)
            prescan_expr(arena, stmt->as.return_stmt.value, table, lambda_scope_depth);
        break;
    case STMT_BLOCK:
        symbol_table_push_scope(table);
        for (int i = 0; i < stmt->as.block.count; i++)
            prescan_stmt(arena, stmt->as.block.statements[i], table, lambda_scope_depth);
        symbol_table_pop_scope(table);
        break;
    case STMT_IF:
        prescan_expr(arena, stmt->as.if_stmt.condition, table, lambda_scope_depth);
        prescan_stmt(arena, stmt->as.if_stmt.then_branch, table, lambda_scope_depth);
        if (stmt->as.if_stmt.else_branch)
            prescan_stmt(arena, stmt->as.if_stmt.else_branch, table, lambda_scope_depth);
        break;
    case STMT_WHILE:
        prescan_expr(arena, stmt->as.while_stmt.condition, table, lambda_scope_depth);
        prescan_stmt(arena, stmt->as.while_stmt.body, table, lambda_scope_depth);
        break;
    case STMT_FOR:
        symbol_table_push_scope(table);
        if (stmt->as.for_stmt.initializer)
            prescan_stmt(arena, stmt->as.for_stmt.initializer, table, lambda_scope_depth);
        if (stmt->as.for_stmt.condition)
            prescan_expr(arena, stmt->as.for_stmt.condition, table, lambda_scope_depth);
        if (stmt->as.for_stmt.increment)
            prescan_expr(arena, stmt->as.for_stmt.increment, table, lambda_scope_depth);
        prescan_stmt(arena, stmt->as.for_stmt.body, table, lambda_scope_depth);
        symbol_table_pop_scope(table);
        break;
    case STMT_FOR_EACH:
        symbol_table_push_scope(table);
        prescan_expr(arena, stmt->as.for_each_stmt.iterable, table, lambda_scope_depth);
        /* Add loop variable to scope */
        if (stmt->as.for_each_stmt.iterable->expr_type &&
            stmt->as.for_each_stmt.iterable->expr_type->kind == TYPE_ARRAY) {
            Type *elem_type = stmt->as.for_each_stmt.iterable->expr_type->as.array.element_type;
            symbol_table_add_symbol(table, stmt->as.for_each_stmt.var_name, elem_type);
        } else {
            symbol_table_add_symbol(table, stmt->as.for_each_stmt.var_name, NULL);
        }
        prescan_stmt(arena, stmt->as.for_each_stmt.body, table, lambda_scope_depth);
        symbol_table_pop_scope(table);
        break;
    case STMT_LOCK:
        prescan_expr(arena, stmt->as.lock_stmt.lock_expr, table, lambda_scope_depth);
        prescan_stmt(arena, stmt->as.lock_stmt.body, table, lambda_scope_depth);
        break;
    case STMT_USING:
        prescan_expr(arena, stmt->as.using_stmt.initializer, table, lambda_scope_depth);
        prescan_stmt(arena, stmt->as.using_stmt.body, table, lambda_scope_depth);
        break;
    default:
        break;
    }
}

static void prescan_function_body(Arena *arena, Stmt **stmts, int stmt_count,
                                  SymbolTable *table)
{
    g_captured_vars = NULL;
    g_captured_var_count = 0;
    g_closure_var_count = 0;
    /* Record the scope depth before pushing so prescan_is_outer_var can
     * distinguish module-level globals (declared at this depth or below)
     * from true locals that need heap promotion when captured. */
    g_prescan_function_entry_depth = symbol_table_get_scope_depth(table);
    /* Push a scope so the prescan's symbol additions don't pollute the
     * caller's symbol table state.  Pass -1 for lambda_scope_depth since
     * we're at the function body level, not inside any lambda. */
    symbol_table_push_scope(table);
    for (int i = 0; i < stmt_count; i++)
        prescan_stmt(arena, stmts[i], table, -1);
    symbol_table_pop_scope(table);
}

/* ============================================================================
 * Pre-scan: identify variables that are assigned thread_spawn results outside
 * their var_decl (conditional spawns). These need a companion SnThread* var.
 * ============================================================================ */

static void prescan_add_thread_handle_var(Arena *arena, const char *name)
{
    for (int i = 0; i < g_thread_handle_var_count; i++)
        if (strcmp(g_thread_handle_vars[i], name) == 0) return;
    if (g_thread_handle_var_count % 8 == 0) {
        char **nv = arena_alloc(arena, (g_thread_handle_var_count + 8) * sizeof(char *));
        for (int i = 0; i < g_thread_handle_var_count; i++) nv[i] = g_thread_handle_vars[i];
        g_thread_handle_vars = nv;
    }
    g_thread_handle_vars[g_thread_handle_var_count++] = arena_strdup(arena, name);
}

static void prescan_thread_expr(Arena *arena, Expr *expr);
static void prescan_thread_stmt(Arena *arena, Stmt *stmt);

static void prescan_thread_expr(Arena *arena, Expr *expr)
{
    if (!expr) return;
    switch (expr->type) {
    case EXPR_ASSIGN:
        /* Assignment of thread_spawn to a variable */
        if (expr->as.assign.value && expr->as.assign.value->type == EXPR_THREAD_SPAWN) {
            char name[256];
            int len = expr->as.assign.name.length < 255 ? expr->as.assign.name.length : 255;
            strncpy(name, expr->as.assign.name.start, len);
            name[len] = '\0';
            prescan_add_thread_handle_var(arena, name);
        }
        prescan_thread_expr(arena, expr->as.assign.value);
        break;
    case EXPR_BINARY:
        prescan_thread_expr(arena, expr->as.binary.left);
        prescan_thread_expr(arena, expr->as.binary.right);
        break;
    case EXPR_UNARY:
        prescan_thread_expr(arena, expr->as.unary.operand);
        break;
    case EXPR_CALL:
        prescan_thread_expr(arena, expr->as.call.callee);
        for (int i = 0; i < expr->as.call.arg_count; i++)
            prescan_thread_expr(arena, expr->as.call.arguments[i]);
        break;
    case EXPR_LAMBDA: {
        LambdaExpr *lam = &expr->as.lambda;
        if (lam->has_stmt_body) {
            for (int i = 0; i < lam->body_stmt_count; i++)
                prescan_thread_stmt(arena, lam->body_stmts[i]);
        }
        if (lam->body)
            prescan_thread_expr(arena, lam->body);
        break;
    }
    default:
        break;
    }
}

static void prescan_thread_stmt(Arena *arena, Stmt *stmt)
{
    if (!stmt) return;
    switch (stmt->type) {
    case STMT_VAR_DECL:
        if (stmt->as.var_decl.initializer)
        {
            /* Track var decls with thread_spawn initializers (they get __th__ companions) */
            if (stmt->as.var_decl.initializer->type == EXPR_THREAD_SPAWN)
            {
                char name[256];
                int len = stmt->as.var_decl.name.length < 255 ? stmt->as.var_decl.name.length : 255;
                strncpy(name, stmt->as.var_decl.name.start, len);
                name[len] = '\0';
                prescan_add_thread_handle_var(arena, name);
            }
            prescan_thread_expr(arena, stmt->as.var_decl.initializer);
        }
        break;
    case STMT_EXPR:
        prescan_thread_expr(arena, stmt->as.expression.expression);
        break;
    case STMT_RETURN:
        if (stmt->as.return_stmt.value)
            prescan_thread_expr(arena, stmt->as.return_stmt.value);
        break;
    case STMT_BLOCK:
        for (int i = 0; i < stmt->as.block.count; i++)
            prescan_thread_stmt(arena, stmt->as.block.statements[i]);
        break;
    case STMT_IF:
        prescan_thread_expr(arena, stmt->as.if_stmt.condition);
        prescan_thread_stmt(arena, stmt->as.if_stmt.then_branch);
        if (stmt->as.if_stmt.else_branch)
            prescan_thread_stmt(arena, stmt->as.if_stmt.else_branch);
        break;
    case STMT_WHILE:
        prescan_thread_expr(arena, stmt->as.while_stmt.condition);
        prescan_thread_stmt(arena, stmt->as.while_stmt.body);
        break;
    case STMT_FOR:
        if (stmt->as.for_stmt.initializer)
            prescan_thread_stmt(arena, stmt->as.for_stmt.initializer);
        if (stmt->as.for_stmt.condition)
            prescan_thread_expr(arena, stmt->as.for_stmt.condition);
        if (stmt->as.for_stmt.increment)
            prescan_thread_expr(arena, stmt->as.for_stmt.increment);
        prescan_thread_stmt(arena, stmt->as.for_stmt.body);
        break;
    case STMT_FOR_EACH:
        prescan_thread_expr(arena, stmt->as.for_each_stmt.iterable);
        prescan_thread_stmt(arena, stmt->as.for_each_stmt.body);
        break;
    case STMT_LOCK:
        prescan_thread_stmt(arena, stmt->as.lock_stmt.body);
        break;
    default:
        break;
    }
}

static void prescan_thread_handle_vars(Arena *arena, Stmt **stmts, int stmt_count)
{
    g_thread_handle_vars = NULL;
    g_thread_handle_var_count = 0;
    for (int i = 0; i < stmt_count; i++)
        prescan_thread_stmt(arena, stmts[i]);
}


json_object *gen_model_function(Arena *arena, FunctionStmt *func, SymbolTable *symbol_table,
                                ArithmeticMode arithmetic_mode)
{
    if (!func) return json_object_new_null();

    json_object *obj = json_object_new_object();

    /* Prefix function name with namespace if inside a namespaced import.
     * Exception: native functions with @alias use the alias as-is (they link
     * to external C symbols like sin, compress, etc.) */
    if (g_model_namespace_prefix != NULL)
    {
        if (func->is_native && func->c_alias && func->body_count == 0)
        {
            /* Native extern with @alias: use the C symbol name directly */
            json_object_object_add(obj, "name", json_object_new_string(func->c_alias));
        }
        else
        {
            char prefixed[512];
            snprintf(prefixed, sizeof(prefixed), "%s__%.*s",
                     g_model_namespace_prefix, func->name.length, func->name.start);
            json_object_object_add(obj, "name", json_object_new_string(prefixed));
        }
    }
    else
    {
        json_object_object_add(obj, "name", json_object_new_string(func->name.start));
    }
    json_object_object_add(obj, "return_type", gen_model_type(arena, func->return_type));
    json_object_object_add(obj, "modifier", json_object_new_string(gen_model_func_mod_str(func->modifier)));
    json_object_object_add(obj, "is_native", json_object_new_boolean(func->is_native));
    json_object_object_add(obj, "has_body", json_object_new_boolean(func->body_count > 0));
    json_object_object_add(obj, "is_variadic", json_object_new_boolean(func->is_variadic));
    json_object_object_add(obj, "has_arena_param", json_object_new_boolean(func->has_arena_param));

    /* Source file tracking for modular compilation */
    if (func->name.filename)
        gen_model_add_source_file(obj, func->name.filename);

    if (func->c_alias)
    {
        json_object_object_add(obj, "has_c_alias", json_object_new_boolean(true));
        json_object_object_add(obj, "c_alias", json_object_new_string(func->c_alias));
    }

    /* Parameters */
    json_object *params = json_object_new_array();
    for (int i = 0; i < func->param_count; i++)
    {
        json_object *p = json_object_new_object();
        json_object_object_add(p, "name", json_object_new_string(func->params[i].name.start));
        json_object_object_add(p, "type", gen_model_type(arena, func->params[i].type));
        json_object_object_add(p, "mem_qual",
            json_object_new_string(gen_model_mem_qual_str(func->params[i].mem_qualifier)));
        json_object_object_add(p, "sync_mod",
            json_object_new_string(gen_model_sync_mod_str(func->params[i].sync_modifier)));
        gen_model_emit_param_cleanup(p, &func->params[i], func->is_native);
        json_object_array_add(params, p);
    }
    json_object_object_add(obj, "params", params);

    /* Pre-scan body for variables captured by lambdas */
    if (func->body)
    {
        prescan_function_body(arena, func->body, func->body_count, symbol_table);
        prescan_thread_handle_vars(arena, func->body, func->body_count);
    }

    /* Collect 'as ref' parameter names — member access needs -> in generated C */
    g_as_ref_param_names = NULL;
    g_as_ref_param_count = 0;
    /* Collect ALL parameter names — used by return-stmt emitter to detect borrowed vars. */
    g_all_param_names = NULL;
    g_all_param_count = 0;
    for (int i = 0; i < func->param_count; i++)
    {
        int nlen = func->params[i].name.length;
        char *ncopy = arena_alloc(arena, nlen + 1);
        memcpy(ncopy, func->params[i].name.start, nlen);
        ncopy[nlen] = '\0';

        /* All params */
        if (g_all_param_count % 8 == 0) {
            char **nv = arena_alloc(arena, (g_all_param_count + 8) * sizeof(char *));
            for (int j = 0; j < g_all_param_count; j++) nv[j] = g_all_param_names[j];
            g_all_param_names = nv;
        }
        g_all_param_names[g_all_param_count++] = ncopy;

        if (func->params[i].mem_qualifier == MEM_AS_REF)
        {
            if (g_as_ref_param_count % 8 == 0) {
                char **nv = arena_alloc(arena, (g_as_ref_param_count + 8) * sizeof(char *));
                for (int j = 0; j < g_as_ref_param_count; j++) nv[j] = g_as_ref_param_names[j];
                g_as_ref_param_names = nv;
            }
            g_as_ref_param_names[g_as_ref_param_count++] = ncopy;
        }
    }

    /* Local cleanup is always enabled. Struct literal returns that reference
     * local variables must use copyOf() to avoid double-free — the compiler
     * emits warnings to guide the user. */
    g_suppress_local_cleanup = false;

    /* Body */
    json_object *body = json_object_new_array();

    /* Prepend param guard locals for as-val-on-as-ref-struct override params */
    for (int i = 0; i < func->param_count; i++)
    {
        if (func->params[i].mem_qualifier == MEM_AS_VAL &&
            func->params[i].type && func->params[i].type->kind == TYPE_STRUCT &&
            func->params[i].type->as.struct_type.pass_self_by_ref)
        {
            char buf[512];
            snprintf(buf, sizeof(buf),
                "sn_auto_%s __sn__%s *__sn__%s__pc = __sn__%s;",
                func->params[i].type->as.struct_type.name,
                func->params[i].type->as.struct_type.name,
                func->params[i].name.start,
                func->params[i].name.start);
            json_object *guard = json_object_new_object();
            json_object_object_add(guard, "kind", json_object_new_string("raw_c"));
            json_object_object_add(guard, "code", json_object_new_string(buf));
            json_object_array_add(body, guard);
        }
    }

    /* Track when we're inside main() with void return type — bare returns
     * must become return 0; since C requires main to return int. */
    bool was_in_main_void = g_in_main_void;
    if (func->name.length == 4 && strncmp(func->name.start, "main", 4) == 0 &&
        func->return_type && func->return_type->kind == TYPE_VOID)
    {
        g_in_main_void = true;
    }

    if (func->body)
    {
        for (int i = 0; i < func->body_count; i++)
        {
            if (func->name.length >= 5 && strncmp(func->name.start, "compu", 5) == 0)
            { fprintf(stderr, "[DBG] body stmt %d/%d type=%d\n", i, func->body_count,
                    func->body[i] ? func->body[i]->type : -1); fflush(stderr); }
            json_object_array_add(body,
                gen_model_stmt(arena, func->body[i], symbol_table, arithmetic_mode));
            if (func->name.length >= 5 && strncmp(func->name.start, "compu", 5) == 0)
            { fprintf(stderr, "[DBG] body stmt %d done\n", i); fflush(stderr); }
        }
    }
    json_object_object_add(obj, "body", body);

    g_in_main_void = was_in_main_void;

    /* Clear per-function state after function is done */
    g_captured_vars = NULL;
    g_captured_var_count = 0;
    g_as_ref_param_names = NULL;
    g_as_ref_param_count = 0;
    g_all_param_names = NULL;
    g_all_param_count = 0;
    g_closure_var_count = 0;
    g_suppress_local_cleanup = false;

    return obj;
}
