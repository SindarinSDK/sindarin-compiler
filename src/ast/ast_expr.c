#include "ast/ast_expr.h"
#include "ast/ast_type.h"
#include "debug.h"
#include <stdlib.h>
#include <string.h>

Expr *ast_create_comparison_expr(Arena *arena, Expr *left, Expr *right, SnTokenType comparison_type, const Token *loc_token)
{
    if (left == NULL || right == NULL)
    {
        DEBUG_ERROR("Cannot create comparison with NULL expressions");
        return NULL;
    }

    return ast_create_binary_expr(arena, left, comparison_type, right, loc_token);
}

Expr *ast_create_binary_expr(Arena *arena, Expr *left, SnTokenType operator, Expr *right, const Token *loc_token)
{
    if (left == NULL || right == NULL)
    {
        return NULL;
    }

    Expr *expr = arena_alloc(arena, sizeof(Expr));
    if (expr == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    memset(expr, 0, sizeof(Expr));
    expr->type = EXPR_BINARY;
    expr->as.binary.left = left;
    expr->as.binary.right = right;
    expr->as.binary.operator = operator;
    expr->expr_type = NULL;
    expr->token = ast_dup_token(arena, loc_token);
    return expr;
}

Expr *ast_create_unary_expr(Arena *arena, SnTokenType operator, Expr *operand, const Token *loc_token)
{
    if (operand == NULL)
    {
        return NULL;
    }

    Expr *expr = arena_alloc(arena, sizeof(Expr));
    if (expr == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    memset(expr, 0, sizeof(Expr));
    expr->type = EXPR_UNARY;
    expr->as.unary.operator = operator;
    expr->as.unary.operand = operand;
    expr->expr_type = NULL;
    expr->token = ast_dup_token(arena, loc_token);
    return expr;
}

Expr *ast_create_literal_expr(Arena *arena, LiteralValue value, Type *type, bool is_interpolated, const Token *loc_token)
{
    if (type == NULL)
    {
        return NULL;
    }

    Expr *expr = arena_alloc(arena, sizeof(Expr));
    if (expr == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    memset(expr, 0, sizeof(Expr));
    expr->type = EXPR_LITERAL;
    expr->as.literal.value = value;
    expr->as.literal.type = type;
    expr->as.literal.is_interpolated = is_interpolated;
    expr->expr_type = NULL;
    expr->token = ast_dup_token(arena, loc_token);
    return expr;
}

Expr *ast_create_variable_expr(Arena *arena, Token name, const Token *loc_token)
{
    Expr *expr = arena_alloc(arena, sizeof(Expr));
    if (expr == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    memset(expr, 0, sizeof(Expr));
    expr->type = EXPR_VARIABLE;
    char *new_start = arena_strndup(arena, name.start, name.length);
    if (new_start == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    expr->as.variable.name.start = new_start;
    expr->as.variable.name.length = name.length;
    expr->as.variable.name.line = name.line;
    expr->as.variable.name.type = name.type;
    expr->as.variable.name.filename = name.filename;
    expr->expr_type = NULL;
    expr->token = ast_dup_token(arena, loc_token);
    return expr;
}

Expr *ast_create_assign_expr(Arena *arena, Token name, Expr *value, const Token *loc_token)
{
    if (value == NULL)
    {
        return NULL;
    }
    Expr *expr = arena_alloc(arena, sizeof(Expr));
    if (expr == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    memset(expr, 0, sizeof(Expr));
    expr->type = EXPR_ASSIGN;
    char *new_start = arena_strndup(arena, name.start, name.length);
    if (new_start == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    expr->as.assign.name.start = new_start;
    expr->as.assign.name.length = name.length;
    expr->as.assign.name.line = name.line;
    expr->as.assign.name.type = name.type;
    expr->as.assign.name.filename = name.filename;
    expr->as.assign.value = value;
    expr->expr_type = NULL;
    expr->token = ast_dup_token(arena, loc_token);
    return expr;
}

Expr *ast_create_index_assign_expr(Arena *arena, Expr *array, Expr *index, Expr *value, const Token *loc_token)
{
    if (array == NULL || index == NULL || value == NULL)
    {
        return NULL;
    }
    Expr *expr = arena_alloc(arena, sizeof(Expr));
    if (expr == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    memset(expr, 0, sizeof(Expr));
    expr->type = EXPR_INDEX_ASSIGN;
    expr->as.index_assign.array = array;
    expr->as.index_assign.index = index;
    expr->as.index_assign.value = value;
    expr->expr_type = NULL;
    expr->token = ast_dup_token(arena, loc_token);
    return expr;
}

Expr *ast_create_call_expr(Arena *arena, Expr *callee, Expr **arguments, int arg_count, const Token *loc_token)
{
    if (callee == NULL)
    {
        return NULL;
    }
    Expr *expr = arena_alloc(arena, sizeof(Expr));
    if (expr == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    memset(expr, 0, sizeof(Expr));
    expr->type = EXPR_CALL;
    expr->as.call.callee = callee;
    expr->as.call.arguments = arguments;
    expr->as.call.arg_count = arg_count;
    expr->expr_type = NULL;
    expr->token = ast_dup_token(arena, loc_token);
    return expr;
}

Expr *ast_create_array_expr(Arena *arena, Expr **elements, int element_count, const Token *loc_token)
{
    Expr *expr = arena_alloc(arena, sizeof(Expr));
    if (expr == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    memset(expr, 0, sizeof(Expr));
    expr->type = EXPR_ARRAY;
    expr->as.array.elements = elements;
    expr->as.array.element_count = element_count;
    expr->expr_type = NULL;
    expr->token = ast_dup_token(arena, loc_token);
    return expr;
}

Expr *ast_create_array_access_expr(Arena *arena, Expr *array, Expr *index, const Token *loc_token)
{
    if (array == NULL || index == NULL)
    {
        return NULL;
    }
    Expr *expr = arena_alloc(arena, sizeof(Expr));
    if (expr == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    memset(expr, 0, sizeof(Expr));
    expr->type = EXPR_ARRAY_ACCESS;
    expr->as.array_access.array = array;
    expr->as.array_access.index = index;
    expr->expr_type = NULL;
    expr->token = ast_dup_token(arena, loc_token);
    return expr;
}

Expr *ast_create_sized_array_alloc_expr(Arena *arena, Type *element_type, Expr *size_expr, Expr *default_value, const Token *loc_token)
{
    if (element_type == NULL || size_expr == NULL)
    {
        DEBUG_ERROR("Cannot create sized array alloc with NULL element_type or size_expr");
        return NULL;
    }
    Expr *expr = arena_alloc(arena, sizeof(Expr));
    if (expr == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    memset(expr, 0, sizeof(Expr));
    expr->type = EXPR_SIZED_ARRAY_ALLOC;
    expr->as.sized_array_alloc.element_type = element_type;
    expr->as.sized_array_alloc.size_expr = size_expr;
    expr->as.sized_array_alloc.default_value = default_value;  /* Can be NULL */
    expr->expr_type = NULL;
    expr->token = ast_dup_token(arena, loc_token);
    return expr;
}

Expr *ast_create_increment_expr(Arena *arena, Expr *operand, const Token *loc_token)
{
    if (operand == NULL)
    {
        return NULL;
    }
    Expr *expr = arena_alloc(arena, sizeof(Expr));
    if (expr == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    memset(expr, 0, sizeof(Expr));
    expr->type = EXPR_INCREMENT;
    expr->as.operand = operand;
    expr->expr_type = NULL;
    expr->token = ast_dup_token(arena, loc_token);
    return expr;
}

Expr *ast_create_decrement_expr(Arena *arena, Expr *operand, const Token *loc_token)
{
    if (operand == NULL)
    {
        return NULL;
    }
    Expr *expr = arena_alloc(arena, sizeof(Expr));
    if (expr == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    memset(expr, 0, sizeof(Expr));
    expr->type = EXPR_DECREMENT;
    expr->as.operand = operand;
    expr->expr_type = NULL;
    expr->token = ast_dup_token(arena, loc_token);
    return expr;
}

Expr *ast_create_interpolated_expr(Arena *arena, Expr **parts, char **format_specs, int part_count, const Token *loc_token)
{
    Expr *expr = arena_alloc(arena, sizeof(Expr));
    if (expr == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    memset(expr, 0, sizeof(Expr));
    expr->type = EXPR_INTERPOLATED;
    expr->as.interpol.parts = parts;
    expr->as.interpol.format_specs = format_specs;
    expr->as.interpol.part_count = part_count;
    expr->expr_type = NULL;
    expr->token = ast_dup_token(arena, loc_token);
    return expr;
}

Expr *ast_create_member_expr(Arena *arena, Expr *object, Token member_name, const Token *loc_token)
{
    if (object == NULL)
    {
        return NULL;
    }
    Expr *expr = arena_alloc(arena, sizeof(Expr));
    if (expr == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    memset(expr, 0, sizeof(Expr));
    expr->type = EXPR_MEMBER;
    expr->as.member.object = object;
    char *new_start = arena_strndup(arena, member_name.start, member_name.length);
    if (new_start == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    expr->as.member.member_name.start = new_start;
    expr->as.member.member_name.length = member_name.length;
    expr->as.member.member_name.line = member_name.line;
    expr->as.member.member_name.type = member_name.type;
    expr->as.member.member_name.filename = member_name.filename;
    expr->expr_type = NULL;
    expr->token = ast_clone_token(arena, loc_token);
    return expr;
}

Expr *ast_create_array_slice_expr(Arena *arena, Expr *array, Expr *start, Expr *end, Expr *step, const Token *loc_token)
{
    if (array == NULL)
    {
        return NULL;
    }
    Expr *expr = arena_alloc(arena, sizeof(Expr));
    if (expr == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    memset(expr, 0, sizeof(Expr));
    expr->type = EXPR_ARRAY_SLICE;
    expr->as.array_slice.array = array;
    expr->as.array_slice.start = start;  // Can be NULL
    expr->as.array_slice.end = end;      // Can be NULL
    expr->as.array_slice.step = step;    // Can be NULL (defaults to 1)
    expr->expr_type = NULL;
    expr->token = ast_clone_token(arena, loc_token);
    return expr;
}

Expr *ast_create_range_expr(Arena *arena, Expr *start, Expr *end, const Token *loc_token)
{
    if (start == NULL || end == NULL)
    {
        return NULL;
    }
    Expr *expr = arena_alloc(arena, sizeof(Expr));
    if (expr == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    memset(expr, 0, sizeof(Expr));
    expr->type = EXPR_RANGE;
    expr->as.range.start = start;
    expr->as.range.end = end;
    expr->expr_type = NULL;
    expr->token = ast_clone_token(arena, loc_token);
    return expr;
}

Expr *ast_create_spread_expr(Arena *arena, Expr *array, const Token *loc_token)
{
    if (array == NULL)
    {
        return NULL;
    }
    Expr *expr = arena_alloc(arena, sizeof(Expr));
    if (expr == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    memset(expr, 0, sizeof(Expr));
    expr->type = EXPR_SPREAD;
    expr->as.spread.array = array;
    expr->expr_type = NULL;
    expr->token = ast_clone_token(arena, loc_token);
    return expr;
}

Expr *ast_create_lambda_expr(Arena *arena, Parameter *params, int param_count,
                             Type *return_type, Expr *body, FunctionModifier modifier,
                             bool is_native, const Token *loc_token)
{
    /* body is required, but return_type can be NULL for type inference */
    if (body == NULL)
    {
        return NULL;
    }
    Expr *expr = arena_alloc(arena, sizeof(Expr));
    if (expr == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    memset(expr, 0, sizeof(Expr));
    expr->type = EXPR_LAMBDA;
    expr->as.lambda.params = params;
    expr->as.lambda.param_count = param_count;
    expr->as.lambda.return_type = return_type;
    expr->as.lambda.body = body;
    expr->as.lambda.body_stmts = NULL;
    expr->as.lambda.body_stmt_count = 0;
    expr->as.lambda.has_stmt_body = 0;  /* Expression body */
    expr->as.lambda.modifier = modifier;
    expr->as.lambda.is_native = is_native;
    expr->as.lambda.captured_vars = NULL;
    expr->as.lambda.captured_types = NULL;
    expr->as.lambda.capture_count = 0;
    expr->as.lambda.lambda_id = 0;  /* Assigned during code gen */
    expr->expr_type = NULL;
    expr->token = ast_clone_token(arena, loc_token);
    return expr;
}

Expr *ast_create_lambda_stmt_expr(Arena *arena, Parameter *params, int param_count,
                                  Type *return_type, Stmt **body_stmts, int body_stmt_count,
                                  FunctionModifier modifier, bool is_native, const Token *loc_token)
{
    /* body_stmts is required for statement-body lambdas */
    if (body_stmts == NULL && body_stmt_count > 0)
    {
        return NULL;
    }
    Expr *expr = arena_alloc(arena, sizeof(Expr));
    if (expr == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    memset(expr, 0, sizeof(Expr));
    expr->type = EXPR_LAMBDA;
    expr->as.lambda.params = params;
    expr->as.lambda.param_count = param_count;
    expr->as.lambda.return_type = return_type;
    expr->as.lambda.body = NULL;
    expr->as.lambda.body_stmts = body_stmts;
    expr->as.lambda.body_stmt_count = body_stmt_count;
    expr->as.lambda.has_stmt_body = 1;  /* Statement body */
    expr->as.lambda.modifier = modifier;
    expr->as.lambda.is_native = is_native;
    expr->as.lambda.captured_vars = NULL;
    expr->as.lambda.captured_types = NULL;
    expr->as.lambda.capture_count = 0;
    expr->as.lambda.lambda_id = 0;  /* Assigned during code gen */
    expr->expr_type = NULL;
    expr->token = ast_clone_token(arena, loc_token);
    return expr;
}

Expr *ast_create_static_call_expr(Arena *arena, Token type_name, Token method_name,
                                   Expr **arguments, int arg_count, const Token *loc_token)
{
    Expr *expr = arena_alloc(arena, sizeof(Expr));
    if (expr == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    memset(expr, 0, sizeof(Expr));
    expr->type = EXPR_STATIC_CALL;

    // Clone type_name token
    char *type_start = arena_strndup(arena, type_name.start, type_name.length);
    if (type_start == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    expr->as.static_call.type_name.start = type_start;
    expr->as.static_call.type_name.length = type_name.length;
    expr->as.static_call.type_name.line = type_name.line;
    expr->as.static_call.type_name.type = type_name.type;
    expr->as.static_call.type_name.filename = type_name.filename;

    // Clone method_name token
    char *method_start = arena_strndup(arena, method_name.start, method_name.length);
    if (method_start == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    expr->as.static_call.method_name.start = method_start;
    expr->as.static_call.method_name.length = method_name.length;
    expr->as.static_call.method_name.line = method_name.line;
    expr->as.static_call.method_name.type = method_name.type;
    expr->as.static_call.method_name.filename = method_name.filename;

    expr->as.static_call.arguments = arguments;
    expr->as.static_call.arg_count = arg_count;
    expr->expr_type = NULL;
    expr->token = ast_clone_token(arena, loc_token);
    return expr;
}

Expr *ast_create_thread_spawn_expr(Arena *arena, Expr *call, FunctionModifier modifier, const Token *loc_token)
{
    if (call == NULL)
    {
        DEBUG_ERROR("Cannot create thread spawn with NULL call expression");
        return NULL;
    }
    Expr *expr = arena_alloc(arena, sizeof(Expr));
    if (expr == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    memset(expr, 0, sizeof(Expr));
    expr->type = EXPR_THREAD_SPAWN;
    expr->as.thread_spawn.call = call;
    expr->as.thread_spawn.modifier = modifier;
    expr->expr_type = NULL;
    expr->token = ast_clone_token(arena, loc_token);
    return expr;
}

Expr *ast_create_thread_sync_expr(Arena *arena, Expr *handle, bool is_array, const Token *loc_token)
{
    if (handle == NULL)
    {
        DEBUG_ERROR("Cannot create thread sync with NULL handle expression");
        return NULL;
    }
    Expr *expr = arena_alloc(arena, sizeof(Expr));
    if (expr == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    memset(expr, 0, sizeof(Expr));
    expr->type = EXPR_THREAD_SYNC;
    expr->as.thread_sync.handle = handle;
    expr->as.thread_sync.is_array = is_array;
    expr->expr_type = NULL;
    expr->token = ast_clone_token(arena, loc_token);
    return expr;
}

Expr *ast_create_sync_list_expr(Arena *arena, Expr **elements, int element_count, const Token *loc_token)
{
    Expr *expr = arena_alloc(arena, sizeof(Expr));
    if (expr == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    memset(expr, 0, sizeof(Expr));
    expr->type = EXPR_SYNC_LIST;
    expr->as.sync_list.elements = elements;
    expr->as.sync_list.element_count = element_count;
    expr->expr_type = NULL;
    expr->token = ast_clone_token(arena, loc_token);
    return expr;
}

Expr *ast_create_as_val_expr(Arena *arena, Expr *operand, const Token *loc_token)
{
    if (operand == NULL)
    {
        DEBUG_ERROR("Cannot create as_val expression with NULL operand");
        return NULL;
    }
    Expr *expr = arena_alloc(arena, sizeof(Expr));
    if (expr == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    memset(expr, 0, sizeof(Expr));
    expr->type = EXPR_AS_VAL;
    expr->as.as_val.operand = operand;
    expr->expr_type = NULL;
    expr->token = ast_clone_token(arena, loc_token);
    return expr;
}

Expr *ast_create_as_ref_expr(Arena *arena, Expr *operand, const Token *loc_token)
{
    if (operand == NULL)
    {
        DEBUG_ERROR("Cannot create as_ref expression with NULL operand");
        return NULL;
    }
    Expr *expr = arena_alloc(arena, sizeof(Expr));
    if (expr == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    memset(expr, 0, sizeof(Expr));
    expr->type = EXPR_AS_REF;
    expr->as.as_ref.operand = operand;
    expr->expr_type = NULL;
    expr->token = ast_clone_token(arena, loc_token);
    return expr;
}

Expr *ast_create_typeof_expr(Arena *arena, Expr *operand, Type *type_literal, const Token *loc_token)
{
    /* Either operand or type_literal must be non-NULL, but not both */
    Expr *expr = arena_alloc(arena, sizeof(Expr));
    if (expr == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    memset(expr, 0, sizeof(Expr));
    expr->type = EXPR_TYPEOF;
    expr->as.typeof_expr.operand = operand;
    expr->as.typeof_expr.type_literal = type_literal;
    expr->expr_type = NULL;
    expr->token = ast_clone_token(arena, loc_token);
    return expr;
}

Expr *ast_create_is_expr(Arena *arena, Expr *operand, Type *check_type, const Token *loc_token)
{
    if (operand == NULL || check_type == NULL)
    {
        DEBUG_ERROR("Cannot create is expression with NULL operand or type");
        return NULL;
    }
    Expr *expr = arena_alloc(arena, sizeof(Expr));
    if (expr == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    memset(expr, 0, sizeof(Expr));
    expr->type = EXPR_IS;
    expr->as.is_expr.operand = operand;
    expr->as.is_expr.check_type = check_type;
    expr->expr_type = NULL;
    expr->token = ast_clone_token(arena, loc_token);
    return expr;
}

Expr *ast_create_as_type_expr(Arena *arena, Expr *operand, Type *target_type, const Token *loc_token)
{
    if (operand == NULL || target_type == NULL)
    {
        DEBUG_ERROR("Cannot create as type expression with NULL operand or type");
        return NULL;
    }
    Expr *expr = arena_alloc(arena, sizeof(Expr));
    if (expr == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    memset(expr, 0, sizeof(Expr));
    expr->type = EXPR_AS_TYPE;
    expr->as.as_type.operand = operand;
    expr->as.as_type.target_type = target_type;
    expr->expr_type = NULL;
    expr->token = ast_clone_token(arena, loc_token);
    return expr;
}

Expr *ast_create_struct_literal_expr(Arena *arena, Token struct_name, FieldInitializer *fields,
                                      int field_count, const Token *loc_token)
{
    Expr *expr = arena_alloc(arena, sizeof(Expr));
    if (expr == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    memset(expr, 0, sizeof(Expr));
    expr->type = EXPR_STRUCT_LITERAL;

    /* Clone struct_name token */
    char *name_start = arena_strndup(arena, struct_name.start, struct_name.length);
    if (name_start == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    expr->as.struct_literal.struct_name.start = name_start;
    expr->as.struct_literal.struct_name.length = struct_name.length;
    expr->as.struct_literal.struct_name.line = struct_name.line;
    expr->as.struct_literal.struct_name.type = struct_name.type;
    expr->as.struct_literal.struct_name.filename = struct_name.filename;

    expr->as.struct_literal.field_count = field_count;
    expr->as.struct_literal.struct_type = NULL;  /* Set during type checking */
    expr->as.struct_literal.fields_initialized = NULL;  /* Allocated during type checking */
    expr->as.struct_literal.total_field_count = 0;  /* Set during type checking */

    if (field_count > 0 && fields != NULL)
    {
        expr->as.struct_literal.fields = arena_alloc(arena, sizeof(FieldInitializer) * field_count);
        if (expr->as.struct_literal.fields == NULL)
        {
            DEBUG_ERROR("Out of memory");
            exit(1);
        }
        for (int i = 0; i < field_count; i++)
        {
            /* Clone field name token */
            char *field_start = arena_strndup(arena, fields[i].name.start, fields[i].name.length);
            if (field_start == NULL)
            {
                DEBUG_ERROR("Out of memory");
                exit(1);
            }
            expr->as.struct_literal.fields[i].name.start = field_start;
            expr->as.struct_literal.fields[i].name.length = fields[i].name.length;
            expr->as.struct_literal.fields[i].name.line = fields[i].name.line;
            expr->as.struct_literal.fields[i].name.type = fields[i].name.type;
            expr->as.struct_literal.fields[i].name.filename = fields[i].name.filename;
            expr->as.struct_literal.fields[i].value = fields[i].value;
        }
    }
    else
    {
        expr->as.struct_literal.fields = NULL;
    }

    expr->expr_type = NULL;
    expr->token = ast_clone_token(arena, loc_token);
    return expr;
}

Expr *ast_create_member_access_expr(Arena *arena, Expr *object, Token field_name, const Token *loc_token)
{
    if (object == NULL)
    {
        DEBUG_ERROR("Cannot create member access with NULL object");
        return NULL;
    }
    Expr *expr = arena_alloc(arena, sizeof(Expr));
    if (expr == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    memset(expr, 0, sizeof(Expr));
    expr->type = EXPR_MEMBER_ACCESS;
    expr->as.member_access.object = object;

    /* Clone field_name token */
    char *name_start = arena_strndup(arena, field_name.start, field_name.length);
    if (name_start == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    expr->as.member_access.field_name.start = name_start;
    expr->as.member_access.field_name.length = field_name.length;
    expr->as.member_access.field_name.line = field_name.line;
    expr->as.member_access.field_name.type = field_name.type;
    expr->as.member_access.field_name.filename = field_name.filename;
    expr->as.member_access.field_index = -1;  /* Set during type checking */
    expr->as.member_access.escaped = false;   /* Set during escape analysis */
    expr->as.member_access.scope_depth = 0;   /* Set during type checking */

    expr->expr_type = NULL;
    expr->token = ast_clone_token(arena, loc_token);
    return expr;
}

Expr *ast_create_member_assign_expr(Arena *arena, Expr *object, Token field_name, Expr *value, const Token *loc_token)
{
    if (object == NULL || value == NULL)
    {
        DEBUG_ERROR("Cannot create member assign with NULL object or value");
        return NULL;
    }
    Expr *expr = arena_alloc(arena, sizeof(Expr));
    if (expr == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    memset(expr, 0, sizeof(Expr));
    expr->type = EXPR_MEMBER_ASSIGN;
    expr->as.member_assign.object = object;
    expr->as.member_assign.value = value;

    /* Clone field_name token */
    char *name_start = arena_strndup(arena, field_name.start, field_name.length);
    if (name_start == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    expr->as.member_assign.field_name.start = name_start;
    expr->as.member_assign.field_name.length = field_name.length;
    expr->as.member_assign.field_name.line = field_name.line;
    expr->as.member_assign.field_name.type = field_name.type;
    expr->as.member_assign.field_name.filename = field_name.filename;

    expr->expr_type = NULL;
    expr->token = ast_clone_token(arena, loc_token);
    return expr;
}

Expr *ast_create_sizeof_type_expr(Arena *arena, Type *type_operand, const Token *loc_token)
{
    if (type_operand == NULL)
    {
        DEBUG_ERROR("Cannot create sizeof expression with NULL type");
        return NULL;
    }
    Expr *expr = arena_alloc(arena, sizeof(Expr));
    if (expr == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    memset(expr, 0, sizeof(Expr));
    expr->type = EXPR_SIZEOF;
    expr->as.sizeof_expr.type_operand = type_operand;
    expr->as.sizeof_expr.expr_operand = NULL;
    expr->expr_type = NULL;
    expr->token = ast_clone_token(arena, loc_token);
    return expr;
}

Expr *ast_create_sizeof_expr_expr(Arena *arena, Expr *expr_operand, const Token *loc_token)
{
    if (expr_operand == NULL)
    {
        DEBUG_ERROR("Cannot create sizeof expression with NULL expression");
        return NULL;
    }
    Expr *expr = arena_alloc(arena, sizeof(Expr));
    if (expr == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    memset(expr, 0, sizeof(Expr));
    expr->type = EXPR_SIZEOF;
    expr->as.sizeof_expr.type_operand = NULL;
    expr->as.sizeof_expr.expr_operand = expr_operand;
    expr->expr_type = NULL;
    expr->token = ast_clone_token(arena, loc_token);
    return expr;
}

/* Escape analysis helper functions */

void ast_expr_mark_escapes(Expr *expr)
{
    if (expr != NULL)
    {
        expr->escape_info.escapes_scope = true;
    }
}

void ast_expr_mark_needs_heap(Expr *expr)
{
    if (expr != NULL)
    {
        expr->escape_info.needs_heap_allocation = true;
    }
}

void ast_expr_clear_escape_info(Expr *expr)
{
    if (expr != NULL)
    {
        expr->escape_info.escapes_scope = false;
        expr->escape_info.needs_heap_allocation = false;
    }
}

bool ast_expr_escapes_scope(Expr *expr)
{
    if (expr == NULL)
    {
        return false;
    }
    return expr->escape_info.escapes_scope;
}

bool ast_expr_needs_heap_allocation(Expr *expr)
{
    if (expr == NULL)
    {
        return false;
    }
    return expr->escape_info.needs_heap_allocation;
}

Expr *ast_create_compound_assign_expr(Arena *arena, Expr *target, SnTokenType operator, Expr *value, const Token *loc_token)
{
    if (target == NULL || value == NULL)
    {
        return NULL;
    }
    Expr *expr = arena_alloc(arena, sizeof(Expr));
    if (expr == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    memset(expr, 0, sizeof(Expr));
    expr->type = EXPR_COMPOUND_ASSIGN;
    expr->as.compound_assign.target = target;
    expr->as.compound_assign.operator = operator;
    expr->as.compound_assign.value = value;
    expr->expr_type = NULL;
    expr->token = ast_clone_token(arena, loc_token);
    return expr;
}
