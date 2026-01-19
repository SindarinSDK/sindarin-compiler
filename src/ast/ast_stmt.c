#include "ast/ast_stmt.h"
#include "debug.h"
#include <stdlib.h>
#include <string.h>

Stmt *ast_create_expr_stmt(Arena *arena, Expr *expression, const Token *loc_token)
{
    if (expression == NULL)
    {
        return NULL;
    }
    Stmt *stmt = arena_alloc(arena, sizeof(Stmt));
    if (stmt == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    memset(stmt, 0, sizeof(Stmt));
    stmt->type = STMT_EXPR;
    stmt->as.expression.expression = expression;
    stmt->token = ast_dup_token(arena, loc_token);
    return stmt;
}

Stmt *ast_create_var_decl_stmt(Arena *arena, Token name, Type *type, Expr *initializer, const Token *loc_token)
{
    // type can be NULL for type inference (will be set by type checker)
    // But if both type and initializer are NULL, that's an error caught by parser
    Stmt *stmt = arena_alloc(arena, sizeof(Stmt));
    if (stmt == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    memset(stmt, 0, sizeof(Stmt));
    stmt->type = STMT_VAR_DECL;
    char *new_start = arena_strndup(arena, name.start, name.length);
    if (new_start == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    stmt->as.var_decl.name.start = new_start;
    stmt->as.var_decl.name.length = name.length;
    stmt->as.var_decl.name.line = name.line;
    stmt->as.var_decl.name.type = name.type;
    stmt->as.var_decl.name.filename = name.filename;
    stmt->as.var_decl.type = type;
    stmt->as.var_decl.initializer = initializer;
    stmt->token = ast_dup_token(arena, loc_token);
    return stmt;
}

Stmt *ast_create_function_stmt(Arena *arena, Token name, Parameter *params, int param_count,
                               Type *return_type, Stmt **body, int body_count, const Token *loc_token)
{
    Stmt *stmt = arena_alloc(arena, sizeof(Stmt));
    if (stmt == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    memset(stmt, 0, sizeof(Stmt));
    stmt->type = STMT_FUNCTION;
    char *new_name_start = arena_strndup(arena, name.start, name.length);
    if (new_name_start == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    stmt->as.function.name.start = new_name_start;
    stmt->as.function.name.length = name.length;
    stmt->as.function.name.line = name.line;
    stmt->as.function.name.type = name.type;
    stmt->as.function.name.filename = name.filename;

    if (params != NULL)
    {
        Parameter *new_params = arena_alloc(arena, sizeof(Parameter) * param_count);
        if (new_params == NULL && param_count > 0)
        {
            DEBUG_ERROR("Out of memory");
            exit(1);
        }
        for (int i = 0; i < param_count; i++)
        {
            char *new_param_start = arena_strndup(arena, params[i].name.start, params[i].name.length);
            if (new_param_start == NULL)
            {
                DEBUG_ERROR("Out of memory");
                exit(1);
            }
            new_params[i].name.start = new_param_start;
            new_params[i].name.length = params[i].name.length;
            new_params[i].name.line = params[i].name.line;
            new_params[i].name.type = params[i].name.type;
            new_params[i].name.filename = params[i].name.filename;
            new_params[i].type = params[i].type;
            new_params[i].mem_qualifier = params[i].mem_qualifier;
        }
        stmt->as.function.params = new_params;
    }
    stmt->as.function.param_count = param_count;
    stmt->as.function.return_type = return_type;
    stmt->as.function.body = body;
    stmt->as.function.body_count = body_count;
    stmt->token = ast_dup_token(arena, loc_token);
    return stmt;
}

Stmt *ast_create_return_stmt(Arena *arena, Token keyword, Expr *value, const Token *loc_token)
{
    Stmt *stmt = arena_alloc(arena, sizeof(Stmt));
    if (stmt == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    memset(stmt, 0, sizeof(Stmt));
    stmt->type = STMT_RETURN;
    stmt->as.return_stmt.keyword.start = keyword.start;
    stmt->as.return_stmt.keyword.length = keyword.length;
    stmt->as.return_stmt.keyword.line = keyword.line;
    stmt->as.return_stmt.keyword.type = keyword.type;
    stmt->as.return_stmt.keyword.filename = keyword.filename;
    stmt->as.return_stmt.value = value;
    stmt->token = ast_dup_token(arena, loc_token);
    return stmt;
}

Stmt *ast_create_block_stmt(Arena *arena, Stmt **statements, int count, const Token *loc_token)
{
    Stmt *stmt = arena_alloc(arena, sizeof(Stmt));
    if (stmt == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    memset(stmt, 0, sizeof(Stmt));
    stmt->type = STMT_BLOCK;
    stmt->as.block.statements = statements;
    stmt->as.block.count = count;
    stmt->token = ast_dup_token(arena, loc_token);
    return stmt;
}

Stmt *ast_create_if_stmt(Arena *arena, Expr *condition, Stmt *then_branch, Stmt *else_branch, const Token *loc_token)
{
    if (condition == NULL || then_branch == NULL)
    {
        return NULL;
    }
    Stmt *stmt = arena_alloc(arena, sizeof(Stmt));
    if (stmt == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    memset(stmt, 0, sizeof(Stmt));
    stmt->type = STMT_IF;
    stmt->as.if_stmt.condition = condition;
    stmt->as.if_stmt.then_branch = then_branch;
    stmt->as.if_stmt.else_branch = else_branch;
    stmt->token = ast_dup_token(arena, loc_token);
    return stmt;
}

Stmt *ast_create_while_stmt(Arena *arena, Expr *condition, Stmt *body, const Token *loc_token)
{
    if (condition == NULL || body == NULL)
    {
        return NULL;
    }
    Stmt *stmt = arena_alloc(arena, sizeof(Stmt));
    if (stmt == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    memset(stmt, 0, sizeof(Stmt));
    stmt->type = STMT_WHILE;
    stmt->as.while_stmt.condition = condition;
    stmt->as.while_stmt.body = body;
    stmt->token = ast_dup_token(arena, loc_token);
    return stmt;
}

Stmt *ast_create_for_stmt(Arena *arena, Stmt *initializer, Expr *condition, Expr *increment, Stmt *body, const Token *loc_token)
{
    if (body == NULL)
    {
        return NULL;
    }
    Stmt *stmt = arena_alloc(arena, sizeof(Stmt));
    if (stmt == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    memset(stmt, 0, sizeof(Stmt));
    stmt->type = STMT_FOR;
    stmt->as.for_stmt.initializer = initializer;
    stmt->as.for_stmt.condition = condition;
    stmt->as.for_stmt.increment = increment;
    stmt->as.for_stmt.body = body;
    stmt->token = ast_dup_token(arena, loc_token);
    return stmt;
}

Stmt *ast_create_for_each_stmt(Arena *arena, Token var_name, Expr *iterable, Stmt *body, const Token *loc_token)
{
    if (iterable == NULL || body == NULL)
    {
        return NULL;
    }
    Stmt *stmt = arena_alloc(arena, sizeof(Stmt));
    if (stmt == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    memset(stmt, 0, sizeof(Stmt));
    stmt->type = STMT_FOR_EACH;
    char *new_start = arena_strndup(arena, var_name.start, var_name.length);
    if (new_start == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    stmt->as.for_each_stmt.var_name.start = new_start;
    stmt->as.for_each_stmt.var_name.length = var_name.length;
    stmt->as.for_each_stmt.var_name.line = var_name.line;
    stmt->as.for_each_stmt.var_name.type = var_name.type;
    stmt->as.for_each_stmt.var_name.filename = var_name.filename;
    stmt->as.for_each_stmt.iterable = iterable;
    stmt->as.for_each_stmt.body = body;
    stmt->token = ast_dup_token(arena, loc_token);
    return stmt;
}

Stmt *ast_create_break_stmt(Arena *arena, const Token *loc_token)
{
    Stmt *stmt = arena_alloc(arena, sizeof(Stmt));
    if (stmt == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    memset(stmt, 0, sizeof(Stmt));
    stmt->type = STMT_BREAK;
    stmt->token = ast_dup_token(arena, loc_token);
    return stmt;
}

Stmt *ast_create_continue_stmt(Arena *arena, const Token *loc_token)
{
    Stmt *stmt = arena_alloc(arena, sizeof(Stmt));
    if (stmt == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    memset(stmt, 0, sizeof(Stmt));
    stmt->type = STMT_CONTINUE;
    stmt->token = ast_dup_token(arena, loc_token);
    return stmt;
}

Stmt *ast_create_import_stmt(Arena *arena, Token module_name, Token *namespace, const Token *loc_token)
{
    Stmt *stmt = arena_alloc(arena, sizeof(Stmt));
    if (stmt == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    memset(stmt, 0, sizeof(Stmt));
    stmt->type = STMT_IMPORT;
    char *new_start = arena_strndup(arena, module_name.start, module_name.length);
    if (new_start == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    stmt->as.import.module_name.start = new_start;
    stmt->as.import.module_name.length = module_name.length;
    stmt->as.import.module_name.line = module_name.line;
    stmt->as.import.module_name.type = module_name.type;
    stmt->as.import.module_name.filename = module_name.filename;

    /* Handle optional namespace */
    if (namespace != NULL)
    {
        Token *ns_copy = arena_alloc(arena, sizeof(Token));
        if (ns_copy == NULL)
        {
            DEBUG_ERROR("Out of memory");
            exit(1);
        }
        char *ns_start = arena_strndup(arena, namespace->start, namespace->length);
        if (ns_start == NULL)
        {
            DEBUG_ERROR("Out of memory");
            exit(1);
        }
        ns_copy->start = ns_start;
        ns_copy->length = namespace->length;
        ns_copy->line = namespace->line;
        ns_copy->type = namespace->type;
        ns_copy->filename = namespace->filename;
        stmt->as.import.namespace = ns_copy;
    }
    else
    {
        stmt->as.import.namespace = NULL;
    }

    stmt->token = ast_dup_token(arena, loc_token);
    return stmt;
}

Stmt *ast_create_pragma_stmt(Arena *arena, PragmaType pragma_type, const char *value, const Token *loc_token)
{
    Stmt *stmt = arena_alloc(arena, sizeof(Stmt));
    if (stmt == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    memset(stmt, 0, sizeof(Stmt));
    stmt->type = STMT_PRAGMA;
    stmt->as.pragma.pragma_type = pragma_type;
    char *new_value = arena_strdup(arena, value);
    if (new_value == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    stmt->as.pragma.value = new_value;
    stmt->token = ast_dup_token(arena, loc_token);
    return stmt;
}

Stmt *ast_create_type_decl_stmt(Arena *arena, Token name, Type *type, const Token *loc_token)
{
    Stmt *stmt = arena_alloc(arena, sizeof(Stmt));
    if (stmt == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    memset(stmt, 0, sizeof(Stmt));
    stmt->type = STMT_TYPE_DECL;
    char *new_start = arena_strndup(arena, name.start, name.length);
    if (new_start == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    stmt->as.type_decl.name.start = new_start;
    stmt->as.type_decl.name.length = name.length;
    stmt->as.type_decl.name.line = name.line;
    stmt->as.type_decl.name.type = name.type;
    stmt->as.type_decl.name.filename = name.filename;
    stmt->as.type_decl.type = type;
    stmt->token = ast_dup_token(arena, loc_token);
    return stmt;
}

Stmt *ast_create_struct_decl_stmt(Arena *arena, Token name, StructField *fields, int field_count,
                                   StructMethod *methods, int method_count,
                                   bool is_native, bool is_packed, bool pass_self_by_ref,
                                   const char *c_alias, const Token *loc_token)
{
    Stmt *stmt = arena_alloc(arena, sizeof(Stmt));
    if (stmt == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    memset(stmt, 0, sizeof(Stmt));
    stmt->type = STMT_STRUCT_DECL;

    char *new_start = arena_strndup(arena, name.start, name.length);
    if (new_start == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    stmt->as.struct_decl.name.start = new_start;
    stmt->as.struct_decl.name.length = name.length;
    stmt->as.struct_decl.name.line = name.line;
    stmt->as.struct_decl.name.type = name.type;
    stmt->as.struct_decl.name.filename = name.filename;

    stmt->as.struct_decl.field_count = field_count;
    stmt->as.struct_decl.method_count = method_count;
    stmt->as.struct_decl.is_native = is_native;
    stmt->as.struct_decl.is_packed = is_packed;
    stmt->as.struct_decl.pass_self_by_ref = pass_self_by_ref;
    stmt->as.struct_decl.c_alias = c_alias ? arena_strdup(arena, c_alias) : NULL;

    if (field_count > 0 && fields != NULL)
    {
        stmt->as.struct_decl.fields = arena_alloc(arena, sizeof(StructField) * field_count);
        if (stmt->as.struct_decl.fields == NULL)
        {
            DEBUG_ERROR("Out of memory");
            exit(1);
        }
        for (int i = 0; i < field_count; i++)
        {
            stmt->as.struct_decl.fields[i].name = fields[i].name
                ? arena_strdup(arena, fields[i].name) : NULL;
            stmt->as.struct_decl.fields[i].type = fields[i].type;
            stmt->as.struct_decl.fields[i].offset = fields[i].offset;
            stmt->as.struct_decl.fields[i].default_value = fields[i].default_value;
            stmt->as.struct_decl.fields[i].c_alias = fields[i].c_alias
                ? arena_strdup(arena, fields[i].c_alias) : NULL;
        }
    }
    else
    {
        stmt->as.struct_decl.fields = NULL;
    }

    /* Copy methods if any */
    if (method_count > 0 && methods != NULL)
    {
        stmt->as.struct_decl.methods = arena_alloc(arena, sizeof(StructMethod) * method_count);
        if (stmt->as.struct_decl.methods == NULL)
        {
            DEBUG_ERROR("Out of memory");
            exit(1);
        }
        for (int i = 0; i < method_count; i++)
        {
            stmt->as.struct_decl.methods[i].name = methods[i].name
                ? arena_strdup(arena, methods[i].name) : NULL;
            stmt->as.struct_decl.methods[i].params = methods[i].params;
            stmt->as.struct_decl.methods[i].param_count = methods[i].param_count;
            stmt->as.struct_decl.methods[i].return_type = methods[i].return_type;
            stmt->as.struct_decl.methods[i].body = methods[i].body;
            stmt->as.struct_decl.methods[i].body_count = methods[i].body_count;
            stmt->as.struct_decl.methods[i].modifier = methods[i].modifier;
            stmt->as.struct_decl.methods[i].is_static = methods[i].is_static;
            stmt->as.struct_decl.methods[i].is_native = methods[i].is_native;
            stmt->as.struct_decl.methods[i].name_token = methods[i].name_token;
            stmt->as.struct_decl.methods[i].c_alias = methods[i].c_alias
                ? arena_strdup(arena, methods[i].c_alias) : NULL;
        }
    }
    else
    {
        stmt->as.struct_decl.methods = NULL;
    }

    stmt->token = ast_dup_token(arena, loc_token);
    return stmt;
}


Stmt *ast_create_lock_stmt(Arena *arena, Expr *lock_expr, Stmt *body, const Token *loc_token)
{
    if (lock_expr == NULL || body == NULL)
    {
        return NULL;
    }
    Stmt *stmt = arena_alloc(arena, sizeof(Stmt));
    if (stmt == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    memset(stmt, 0, sizeof(Stmt));
    stmt->type = STMT_LOCK;
    stmt->as.lock_stmt.lock_expr = lock_expr;
    stmt->as.lock_stmt.body = body;
    stmt->token = ast_dup_token(arena, loc_token);
    return stmt;
}
