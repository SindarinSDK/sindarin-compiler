#include "ast.h"
#include "ast/ast_type.h"
#include "ast/ast_expr.h"
#include "ast/ast_stmt.h"
#include "ast/ast_print.h"
#include "debug.h"
#include <string.h>
#include <stdlib.h>

Token *ast_clone_token(Arena *arena, const Token *src)
{
    if (src == NULL)
    {
        return NULL;
    }

    Token *token = arena_alloc(arena, sizeof(Token));
    if (token == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }

    token->type = src->type;
    token->line = src->line;
    token->length = src->length;
    token->literal = src->literal;

    if (src->start != NULL)
    {
        token->start = arena_strndup(arena, src->start, src->length);
        if (token->start == NULL)
        {
            DEBUG_ERROR("Out of memory");
            exit(1);
        }
    }
    else
    {
        token->start = NULL;
    }

    if (src->filename != NULL)
    {
        token->filename = arena_strdup(arena, src->filename);
        if (token->filename == NULL)
        {
            DEBUG_ERROR("Out of memory");
            exit(1);
        }
    }
    else
    {
        token->filename = NULL;
    }

    return token;
}

void ast_init_module(Arena *arena, Module *module, const char *filename)
{
    if (module == NULL)
        return;

    module->count = 0;
    module->capacity = 8;

    module->statements = arena_alloc(arena, sizeof(Stmt *) * module->capacity);
    if (module->statements == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    memset(module->statements, 0, sizeof(Stmt *) * module->capacity);

    module->filename = filename;
}

void ast_module_add_statement(Arena *arena, Module *module, Stmt *stmt)
{
    if (module == NULL || stmt == NULL)
        return;

    if (module->count == module->capacity)
    {
        size_t new_capacity = module->capacity * 2;
        Stmt **new_statements = arena_alloc(arena, sizeof(Stmt *) * new_capacity);
        if (new_statements == NULL)
        {
            DEBUG_ERROR("Out of memory");
            exit(1);
        }
        memcpy(new_statements, module->statements, sizeof(Stmt *) * module->capacity);
        memset(new_statements + module->capacity, 0, sizeof(Stmt *) * (new_capacity - module->capacity));
        module->statements = new_statements;
        module->capacity = new_capacity;
    }

    module->statements[module->count++] = stmt;
}
