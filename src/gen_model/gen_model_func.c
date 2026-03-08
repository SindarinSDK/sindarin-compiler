#include "gen_model/gen_model.h"
#include <string.h>

/* ============================================================================
 * Pre-scan: identify outer-scope variables captured by lambdas that need
 * handle-based (promoted) storage for capture-by-reference semantics.
 * ============================================================================ */

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

/* Forward declarations */
static void prescan_expr(Arena *arena, Expr *expr, int lambda_depth);
static void prescan_stmt(Arena *arena, Stmt *stmt, int lambda_depth);

static void prescan_expr(Arena *arena, Expr *expr, int lambda_depth)
{
    if (!expr) return;
    switch (expr->type) {
    case EXPR_LAMBDA: {
        LambdaExpr *lam = &expr->as.lambda;
        if (lam->has_stmt_body) {
            for (int i = 0; i < lam->body_stmt_count; i++)
                prescan_stmt(arena, lam->body_stmts[i], lambda_depth + 1);
        }
        if (lam->body)
            prescan_expr(arena, lam->body, lambda_depth + 1);
        break;
    }
    case EXPR_VARIABLE:
        /* Only reading — no promotion needed */
        break;
    case EXPR_ASSIGN:
        prescan_expr(arena, expr->as.assign.value, lambda_depth);
        /* Assignment target is mutated — needs promotion if inside lambda */
        if (lambda_depth > 0 && expr->expr_type && prescan_needs_ref(expr->expr_type)) {
            char name[256];
            int len = expr->as.assign.name.length < 255 ? expr->as.assign.name.length : 255;
            strncpy(name, expr->as.assign.name.start, len);
            name[len] = '\0';
            prescan_add_captured(arena, name);
        }
        break;
    case EXPR_COMPOUND_ASSIGN:
        /* Compound assignment target is mutated — needs promotion if inside lambda */
        if (lambda_depth > 0 && expr->as.compound_assign.target->type == EXPR_VARIABLE) {
            Expr *tgt = expr->as.compound_assign.target;
            if (tgt->expr_type && prescan_needs_ref(tgt->expr_type)) {
                char name[256];
                int len = tgt->as.variable.name.length < 255 ? tgt->as.variable.name.length : 255;
                strncpy(name, tgt->as.variable.name.start, len);
                name[len] = '\0';
                prescan_add_captured(arena, name);
            }
        }
        prescan_expr(arena, expr->as.compound_assign.target, lambda_depth);
        prescan_expr(arena, expr->as.compound_assign.value, lambda_depth);
        break;
    case EXPR_BINARY:
        prescan_expr(arena, expr->as.binary.left, lambda_depth);
        prescan_expr(arena, expr->as.binary.right, lambda_depth);
        break;
    case EXPR_UNARY:
        prescan_expr(arena, expr->as.unary.operand, lambda_depth);
        break;
    case EXPR_CALL:
        prescan_expr(arena, expr->as.call.callee, lambda_depth);
        for (int i = 0; i < expr->as.call.arg_count; i++)
            prescan_expr(arena, expr->as.call.arguments[i], lambda_depth);
        break;
    case EXPR_MEMBER:
        prescan_expr(arena, expr->as.member.object, lambda_depth);
        break;
    case EXPR_ARRAY_ACCESS:
        prescan_expr(arena, expr->as.array_access.array, lambda_depth);
        prescan_expr(arena, expr->as.array_access.index, lambda_depth);
        break;
    case EXPR_INCREMENT:
    case EXPR_DECREMENT:
        /* Increment/decrement mutates the target */
        if (lambda_depth > 0 && expr->as.operand && expr->as.operand->type == EXPR_VARIABLE) {
            Expr *op = expr->as.operand;
            if (op->expr_type && prescan_needs_ref(op->expr_type)) {
                char name[256];
                int len = op->as.variable.name.length < 255 ? op->as.variable.name.length : 255;
                strncpy(name, op->as.variable.name.start, len);
                name[len] = '\0';
                prescan_add_captured(arena, name);
            }
        }
        prescan_expr(arena, expr->as.operand, lambda_depth);
        break;
    case EXPR_INTERPOLATED:
        for (int i = 0; i < expr->as.interpol.part_count; i++)
            prescan_expr(arena, expr->as.interpol.parts[i], lambda_depth);
        break;
    default:
        break;
    }
}

static void prescan_stmt(Arena *arena, Stmt *stmt, int lambda_depth)
{
    if (!stmt) return;
    switch (stmt->type) {
    case STMT_VAR_DECL:
        if (stmt->as.var_decl.initializer)
            prescan_expr(arena, stmt->as.var_decl.initializer, lambda_depth);
        break;
    case STMT_EXPR:
        prescan_expr(arena, stmt->as.expression.expression, lambda_depth);
        break;
    case STMT_RETURN:
        if (stmt->as.return_stmt.value)
            prescan_expr(arena, stmt->as.return_stmt.value, lambda_depth);
        break;
    case STMT_BLOCK:
        for (int i = 0; i < stmt->as.block.count; i++)
            prescan_stmt(arena, stmt->as.block.statements[i], lambda_depth);
        break;
    case STMT_IF:
        prescan_expr(arena, stmt->as.if_stmt.condition, lambda_depth);
        prescan_stmt(arena, stmt->as.if_stmt.then_branch, lambda_depth);
        if (stmt->as.if_stmt.else_branch)
            prescan_stmt(arena, stmt->as.if_stmt.else_branch, lambda_depth);
        break;
    case STMT_WHILE:
        prescan_expr(arena, stmt->as.while_stmt.condition, lambda_depth);
        prescan_stmt(arena, stmt->as.while_stmt.body, lambda_depth);
        break;
    case STMT_FOR:
        if (stmt->as.for_stmt.initializer)
            prescan_stmt(arena, stmt->as.for_stmt.initializer, lambda_depth);
        if (stmt->as.for_stmt.condition)
            prescan_expr(arena, stmt->as.for_stmt.condition, lambda_depth);
        if (stmt->as.for_stmt.increment)
            prescan_expr(arena, stmt->as.for_stmt.increment, lambda_depth);
        prescan_stmt(arena, stmt->as.for_stmt.body, lambda_depth);
        break;
    default:
        break;
    }
}

static void prescan_function_body(Arena *arena, Stmt **stmts, int stmt_count)
{
    g_captured_vars = NULL;
    g_captured_var_count = 0;
    for (int i = 0; i < stmt_count; i++)
        prescan_stmt(arena, stmts[i], 0);
}

static const char *mem_qual_str(MemoryQualifier mq)
{
    switch (mq)
    {
        case MEM_DEFAULT: return "default";
        case MEM_AS_VAL:  return "as_val";
        case MEM_AS_REF:  return "as_ref";
        default:          return "default";
    }
}

static const char *sync_mod_str(SyncModifier sm)
{
    switch (sm)
    {
        case SYNC_NONE:   return "none";
        case SYNC_ATOMIC: return "atomic";
        default:          return "none";
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

json_object *gen_model_function(Arena *arena, FunctionStmt *func, SymbolTable *symbol_table,
                                ArithmeticMode arithmetic_mode)
{
    if (!func) return json_object_new_null();

    json_object *obj = json_object_new_object();

    json_object_object_add(obj, "name", json_object_new_string(func->name.start));
    json_object_object_add(obj, "return_type", gen_model_type(arena, func->return_type));
    json_object_object_add(obj, "modifier", json_object_new_string(func_mod_str(func->modifier)));
    json_object_object_add(obj, "is_native", json_object_new_boolean(func->is_native));
    json_object_object_add(obj, "is_variadic", json_object_new_boolean(func->is_variadic));
    json_object_object_add(obj, "has_arena_param", json_object_new_boolean(func->has_arena_param));

    if (func->c_alias)
    {
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
            json_object_new_string(mem_qual_str(func->params[i].mem_qualifier)));
        json_object_object_add(p, "sync_mod",
            json_object_new_string(sync_mod_str(func->params[i].sync_modifier)));
        json_object_array_add(params, p);
    }
    json_object_object_add(obj, "params", params);

    /* Pre-scan body for variables captured by lambdas */
    if (func->body)
    {
        prescan_function_body(arena, func->body, func->body_count);
    }

    /* Add 'as ref' parameters to captured vars — they also need dereference */
    for (int i = 0; i < func->param_count; i++)
    {
        if (func->params[i].mem_qualifier == MEM_AS_REF)
        {
            prescan_add_captured(arena, func->params[i].name.start);
        }
    }

    /* Body */
    json_object *body = json_object_new_array();
    if (func->body)
    {
        for (int i = 0; i < func->body_count; i++)
        {
            json_object_array_add(body,
                gen_model_stmt(arena, func->body[i], symbol_table, arithmetic_mode));
        }
    }
    json_object_object_add(obj, "body", body);

    /* Clear captured vars after function is done */
    g_captured_vars = NULL;
    g_captured_var_count = 0;

    return obj;
}
