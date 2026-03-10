#include "gen_model/gen_model.h"
#include "symbol_table/symbol_table_core.h"
#include <string.h>

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
    if (stmt->type == STMT_VAR_DECL) {
        char name[256];
        int len = stmt->as.var_decl.name.length < 255 ? stmt->as.var_decl.name.length : 255;
        strncpy(name, stmt->as.var_decl.name.start, len);
        name[len] = '\0';
        mc_add(locals, arena, name, stmt->as.var_decl.type);
    } else if (stmt->type == STMT_BLOCK) {
        for (int i = 0; i < stmt->as.block.count; i++)
            mc_collect_locals(stmt->as.block.statements[i], locals, arena);
    }
}

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
        /* Also capture the assignment target */
        {
            char name[256];
            int len = expr->as.assign.name.length < 255 ? expr->as.assign.name.length : 255;
            strncpy(name, expr->as.assign.name.start, len);
            name[len] = '\0';
            if (!mc_is_param(lam, name) && !mc_is_local(locals, name) && expr->expr_type)
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
    case EXPR_LAMBDA:
        /* Don't recurse into nested lambdas for now */
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
                            json_object_new_string(lit->value.string_value ? lit->value.string_value : ""));
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
            json_object_object_add(obj, "name", json_object_new_string(vname));
            /* Check if this variable is captured (promoted to handle-based storage) */
            for (int ci = 0; ci < g_captured_var_count; ci++) {
                if (strcmp(g_captured_vars[ci], vname) == 0) {
                    json_object_object_add(obj, "is_captured", json_object_new_boolean(true));
                    break;
                }
            }
            break;
        }

        case EXPR_ASSIGN:
        {
            json_object_object_add(obj, "kind", json_object_new_string("assign"));
            const char *aname = expr->as.assign.name.start;
            json_object_object_add(obj, "target", json_object_new_string(aname));
            json_object_object_add(obj, "value",
                gen_model_expr(arena, expr->as.assign.value, symbol_table, arithmetic_mode));
            /* Check if target is captured */
            for (int ci = 0; ci < g_captured_var_count; ci++) {
                if (strcmp(g_captured_vars[ci], aname) == 0) {
                    json_object_object_add(obj, "is_captured", json_object_new_boolean(true));
                    break;
                }
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
                            if (!is_nil && (vt == EXPR_LITERAL || vt == EXPR_VARIABLE))
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
            json_object_object_add(obj, "value",
                gen_model_expr(arena, expr->as.compound_assign.value, symbol_table, arithmetic_mode));
            break;
        }

        case EXPR_BINARY:
        {
            json_object_object_add(obj, "kind", json_object_new_string("binary"));
            json_object_object_add(obj, "op",
                json_object_new_string(binary_op_str(expr->as.binary.operator)));
            json_object_object_add(obj, "left",
                gen_model_expr(arena, expr->as.binary.left, symbol_table, arithmetic_mode));
            json_object_object_add(obj, "right",
                gen_model_expr(arena, expr->as.binary.right, symbol_table, arithmetic_mode));
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
            if (expr->as.call.callee->type == EXPR_VARIABLE)
            {
                const char *name = expr->as.call.callee->as.variable.name.start;
                if (strcmp(name, "assert") == 0 || strcmp(name, "println") == 0 ||
                    strcmp(name, "print") == 0 || strcmp(name, "exit") == 0)
                {
                    builtin_name = name;
                }
            }

            if (builtin_name)
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
                json_object_object_add(obj, "callee",
                    gen_model_expr(arena, expr->as.call.callee, symbol_table, arithmetic_mode));

                /* Get param mem quals from callee's function type */
                MemoryQualifier *pmq = NULL;
                int pmq_count = 0;
                if (expr->as.call.callee->expr_type &&
                    expr->as.call.callee->expr_type->kind == TYPE_FUNCTION)
                {
                    pmq = expr->as.call.callee->expr_type->as.function.param_mem_quals;
                    pmq_count = expr->as.call.callee->expr_type->as.function.param_count;
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
                    json_object_array_add(args, arg);
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
                    if (ct && ct->kind == TYPE_FUNCTION && !ct->as.function.is_native)
                    {
                        Symbol *sym = symbol_table_lookup_symbol(symbol_table,
                            expr->as.call.callee->as.variable.name);
                        /* If not found in symbol table, it's a local variable (closure).
                         * If found but not a function definition, also a closure. */
                        if (sym == NULL || !sym->is_function)
                            is_closure = true;
                    }
                }
                /* TODO: EXPR_ARRAY_ACCESS and EXPR_MEMBER closure calls
                 * (e.g., callbacks[0]() or handler.callback()) need special handling
                 * to distinguish from struct method calls. */
                json_object_object_add(obj, "is_closure_call",
                    json_object_new_boolean(is_closure));
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
                    json_object_array_add(args, arg);
                }
            }
            json_object_object_add(obj, "args", args);
            break;
        }

        case EXPR_STATIC_CALL:
        {
            json_object_object_add(obj, "kind", json_object_new_string("static_call"));
            json_object_object_add(obj, "type_name",
                json_object_new_string(expr->as.static_call.type_name.start));
            json_object_object_add(obj, "method_name",
                json_object_new_string(expr->as.static_call.method_name.start));
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
                    if (!is_nil && (ae->type == EXPR_LITERAL || ae->type == EXPR_VARIABLE))
                        json_object_object_add(elem, "needs_strdup", json_object_new_boolean(true));
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
                        elem_release_fn = "sn_cleanup_str";
                        elem_copy_fn = "sn_copy_str";
                        break;
                    case TYPE_ARRAY:
                        elem_release_fn = "(void (*)(void *))sn_cleanup_array";
                        elem_copy_fn = NULL;  /* TODO: nested array copy */
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
                            /* as val: check if has heap fields */
                            bool has_heap = false;
                            for (int fi = 0; fi < et->as.struct_type.field_count; fi++)
                            {
                                Type *ft = et->as.struct_type.fields[fi].type;
                                if (ft && (ft->kind == TYPE_STRING || ft->kind == TYPE_ARRAY ||
                                    ft->kind == TYPE_FUNCTION ||
                                    (ft->kind == TYPE_STRUCT && ft->as.struct_type.pass_self_by_ref)))
                                {
                                    has_heap = true;
                                    break;
                                }
                            }
                            if (has_heap)
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
                            if (!is_nil && (vt == EXPR_LITERAL || vt == EXPR_VARIABLE))
                                ia_needs_strdup = true;
                        }
                        break;
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
                            if (!is_nil && (vt == EXPR_LITERAL || vt == EXPR_VARIABLE))
                                ma_needs_strdup = true;
                        }
                        break;
                    case TYPE_ARRAY:
                        field_cleanup = "cleanup_arr";
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
            }
            break;
        }

        case EXPR_STRUCT_LITERAL:
        {
            json_object_object_add(obj, "kind", json_object_new_string("struct_literal"));
            json_object_object_add(obj, "struct_name",
                json_object_new_string(expr->as.struct_literal.struct_name.start));
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
                    bool needs_strdup = (fv->type == EXPR_LITERAL || fv->type == EXPR_VARIABLE);
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
                        json_object_new_string(p->as.literal.value.string_value ? p->as.literal.value.string_value : ""));
                }
                else
                {
                    json_object_object_add(part, "kind", json_object_new_string("expr"));
                    json_object_object_add(part, "expr",
                        gen_model_expr(arena, p, symbol_table, arithmetic_mode));
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
            break;
        }

        case EXPR_DECREMENT:
        {
            json_object_object_add(obj, "kind", json_object_new_string("decrement"));
            json_object_object_add(obj, "operand",
                gen_model_expr(arena, expr->as.operand, symbol_table, arithmetic_mode));
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

            /* Body */
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

                /* Body for the lambda function definition */
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
            json_object *call_obj = gen_model_expr(arena, call_expr, symbol_table, arithmetic_mode);
            json_object_object_add(obj, "call", call_obj);
            json_object_object_add(obj, "modifier",
                json_object_new_string(func_mod_str(expr->as.thread_spawn.modifier)));

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
                if (call_expr->type == EXPR_CALL && call_expr->as.call.callee)
                {
                    Expr *callee = call_expr->as.call.callee;
                    if (callee->type == EXPR_VARIABLE)
                    {
                        json_object_object_add(tdef, "func_name",
                            json_object_new_string(callee->as.variable.name.start));
                    }
                }

                /* Collect args with types for the ThreadArgs struct */
                json_object *args = json_object_new_array();
                if (call_expr->type == EXPR_CALL)
                {
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
                        json_object_array_add(args, arg);
                    }
                }
                json_object_object_add(tdef, "args", args);
                json_object_object_add(tdef, "has_args",
                    json_object_new_boolean(call_expr->type == EXPR_CALL &&
                                            call_expr->as.call.arg_count > 0));

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
            break;
        }

        case EXPR_SYNC_LIST:
        {
            json_object_object_add(obj, "kind", json_object_new_string("sync_list"));
            json_object *elements = json_object_new_array();
            for (int i = 0; i < expr->as.sync_list.element_count; i++)
            {
                json_object_array_add(elements,
                    gen_model_expr(arena, expr->as.sync_list.elements[i], symbol_table, arithmetic_mode));
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
            if (expr->as.typeof_expr.operand)
            {
                json_object_object_add(obj, "operand",
                    gen_model_expr(arena, expr->as.typeof_expr.operand, symbol_table, arithmetic_mode));
            }
            if (expr->as.typeof_expr.type_literal)
            {
                json_object_object_add(obj, "type_literal",
                    gen_model_type(arena, expr->as.typeof_expr.type_literal));
            }
            break;
        }

        case EXPR_IS:
        {
            json_object_object_add(obj, "kind", json_object_new_string("is"));
            json_object_object_add(obj, "operand",
                gen_model_expr(arena, expr->as.is_expr.operand, symbol_table, arithmetic_mode));
            json_object_object_add(obj, "check_type",
                gen_model_type(arena, expr->as.is_expr.check_type));
            break;
        }

        case EXPR_AS_TYPE:
        {
            json_object_object_add(obj, "kind", json_object_new_string("as_type"));
            json_object_object_add(obj, "operand",
                gen_model_expr(arena, expr->as.as_type.operand, symbol_table, arithmetic_mode));
            json_object_object_add(obj, "target_type",
                gen_model_type(arena, expr->as.as_type.target_type));
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
