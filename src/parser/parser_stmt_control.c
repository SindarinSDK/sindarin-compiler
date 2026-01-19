#include "parser/parser_stmt_control.h"
#include "parser/parser_stmt.h"
#include "parser/parser_util.h"
#include "parser/parser_expr.h"
#include "ast/ast_expr.h"
#include "debug.h"
#include <stdlib.h>
#include <string.h>

Stmt *parser_return_statement(Parser *parser)
{
    Token keyword = parser->previous;
    keyword.start = arena_strndup(parser->arena, keyword.start, keyword.length);
    if (keyword.start == NULL)
    {
        parser_error_at_current(parser, "Out of memory");
        return NULL;
    }

    Expr *value = NULL;
    if (!parser_check(parser, TOKEN_SEMICOLON) && !parser_check(parser, TOKEN_NEWLINE) && !parser_is_at_end(parser))
    {
        value = parser_expression(parser);
    }

    if (!parser_match(parser, TOKEN_SEMICOLON) && !parser_check(parser, TOKEN_NEWLINE) && !parser_is_at_end(parser))
    {
        parser_consume(parser, TOKEN_SEMICOLON, "Expected ';' or newline after return value");
    }
    else if (parser_match(parser, TOKEN_SEMICOLON))
    {
    }

    return ast_create_return_stmt(parser->arena, keyword, value, &keyword);
}

Stmt *parser_if_statement(Parser *parser)
{
    Token if_token = parser->previous;
    Expr *condition = parser_expression(parser);
    parser_consume(parser, TOKEN_ARROW, "Expected '=>' after if condition");
    skip_newlines(parser);

    Stmt *then_branch;
    if (parser_check(parser, TOKEN_INDENT))
    {
        then_branch = parser_indented_block(parser);
    }
    else
    {
        then_branch = parser_statement(parser);
        skip_newlines(parser);
        if (parser_check(parser, TOKEN_INDENT))
        {
            Stmt **block_stmts = arena_alloc(parser->arena, sizeof(Stmt *) * 2);
            if (block_stmts == NULL)
            {
                exit(1);
            }
            block_stmts[0] = then_branch;
            Stmt *indented = parser_indented_block(parser);
            block_stmts[1] = indented ? indented : ast_create_block_stmt(parser->arena, NULL, 0, NULL);
            then_branch = ast_create_block_stmt(parser->arena, block_stmts, 2, NULL);
        }
    }

    Stmt *else_branch = NULL;
    skip_newlines(parser);
    if (parser_match(parser, TOKEN_ELSE))
    {
        /* Support 'else if' syntax sugar - if followed by 'if', parse if statement directly */
        if (parser_match(parser, TOKEN_IF))
        {
            /* Parse the 'if' statement as the else branch (no arrow needed between else and if) */
            else_branch = parser_if_statement(parser);
        }
        else
        {
            /* Original 'else =>' syntax */
            parser_consume(parser, TOKEN_ARROW, "Expected '=>' after else");
            skip_newlines(parser);
            if (parser_check(parser, TOKEN_INDENT))
            {
                else_branch = parser_indented_block(parser);
            }
            else
            {
                else_branch = parser_statement(parser);
                skip_newlines(parser);
                if (parser_check(parser, TOKEN_INDENT))
                {
                    Stmt **block_stmts = arena_alloc(parser->arena, sizeof(Stmt *) * 2);
                    if (block_stmts == NULL)
                    {
                        exit(1);
                    }
                    block_stmts[0] = else_branch;
                    Stmt *indented = parser_indented_block(parser);
                    block_stmts[1] = indented ? indented : ast_create_block_stmt(parser->arena, NULL, 0, NULL);
                    else_branch = ast_create_block_stmt(parser->arena, block_stmts, 2, NULL);
                }
            }
        }
    }

    return ast_create_if_stmt(parser->arena, condition, then_branch, else_branch, &if_token);
}

Stmt *parser_while_statement(Parser *parser, bool is_shared)
{
    Token while_token = parser->previous;
    Expr *condition = parser_expression(parser);
    parser_consume(parser, TOKEN_ARROW, "Expected '=>' after while condition");
    skip_newlines(parser);

    Stmt *body;
    if (parser_check(parser, TOKEN_INDENT))
    {
        body = parser_indented_block(parser);
    }
    else
    {
        body = parser_statement(parser);
        skip_newlines(parser);
        if (parser_check(parser, TOKEN_INDENT))
        {
            Stmt **block_stmts = arena_alloc(parser->arena, sizeof(Stmt *) * 2);
            if (block_stmts == NULL)
            {
                exit(1);
            }
            block_stmts[0] = body;
            Stmt *indented = parser_indented_block(parser);
            block_stmts[1] = indented ? indented : ast_create_block_stmt(parser->arena, NULL, 0, NULL);
            body = ast_create_block_stmt(parser->arena, block_stmts, 2, NULL);
        }
    }

    Stmt *stmt = ast_create_while_stmt(parser->arena, condition, body, &while_token);
    if (stmt != NULL)
    {
        stmt->as.while_stmt.is_shared = is_shared;
    }
    return stmt;
}

Stmt *parser_for_statement(Parser *parser, bool is_shared)
{
    Token for_token = parser->previous;

    // Check for for-each syntax: for x in arr =>
    // We need to look ahead: if we see IDENTIFIER followed by IN, it's for-each
    if (parser_check(parser, TOKEN_IDENTIFIER))
    {
        Token var_name = parser->current;
        parser_advance(parser);

        if (parser_check(parser, TOKEN_IN))
        {
            // This is a for-each loop
            parser_advance(parser); // consume 'in'
            var_name.start = arena_strndup(parser->arena, var_name.start, var_name.length);
            if (var_name.start == NULL)
            {
                parser_error_at_current(parser, "Out of memory");
                return NULL;
            }

            Expr *iterable = parser_expression(parser);
            parser_consume(parser, TOKEN_ARROW, "Expected '=>' after for-each iterable");
            skip_newlines(parser);

            Stmt *body;
            if (parser_check(parser, TOKEN_INDENT))
            {
                body = parser_indented_block(parser);
            }
            else
            {
                body = parser_statement(parser);
                skip_newlines(parser);
                if (parser_check(parser, TOKEN_INDENT))
                {
                    Stmt **block_stmts = arena_alloc(parser->arena, sizeof(Stmt *) * 2);
                    if (block_stmts == NULL)
                    {
                        exit(1);
                    }
                    block_stmts[0] = body;
                    Stmt *indented = parser_indented_block(parser);
                    block_stmts[1] = indented ? indented : ast_create_block_stmt(parser->arena, NULL, 0, NULL);
                    body = ast_create_block_stmt(parser->arena, block_stmts, 2, NULL);
                }
            }

            Stmt *stmt = ast_create_for_each_stmt(parser->arena, var_name, iterable, body, &for_token);
            if (stmt != NULL)
            {
                stmt->as.for_each_stmt.is_shared = is_shared;
            }
            return stmt;
        }
        else
        {
            // Not for-each, backtrack - need to re-parse as expression
            // Since we already consumed the identifier, we need to create a variable expr
            // and parse the rest as expression statement initializer
            var_name.start = arena_strndup(parser->arena, var_name.start, var_name.length);
            if (var_name.start == NULL)
            {
                parser_error_at_current(parser, "Out of memory");
                return NULL;
            }
            Expr *var_expr = ast_create_variable_expr(parser->arena, var_name, &var_name);

            // Parse the rest of the expression (e.g., = 0)
            Expr *init_expr = var_expr;
            if (parser_match(parser, TOKEN_EQUAL))
            {
                Expr *value = parser_expression(parser);
                init_expr = ast_create_assign_expr(parser->arena, var_name, value, &var_name);
            }
            else
            {
                // May have more expression parts like function call, etc.
                // For now, just use the variable as the initializer
                // This handles cases like: for i; i < 10; i++ =>
            }
            Stmt *initializer = ast_create_expr_stmt(parser->arena, init_expr, NULL);

            parser_consume(parser, TOKEN_SEMICOLON, "Expected ';' after initializer");

            Expr *condition = NULL;
            if (!parser_check(parser, TOKEN_SEMICOLON))
            {
                condition = parser_expression(parser);
            }
            parser_consume(parser, TOKEN_SEMICOLON, "Expected ';' after condition");

            Expr *increment = NULL;
            if (!parser_check(parser, TOKEN_ARROW))
            {
                increment = parser_expression(parser);
            }
            parser_consume(parser, TOKEN_ARROW, "Expected '=>' after for clauses");
            skip_newlines(parser);

            Stmt *body;
            if (parser_check(parser, TOKEN_INDENT))
            {
                body = parser_indented_block(parser);
            }
            else
            {
                body = parser_statement(parser);
                skip_newlines(parser);
                if (parser_check(parser, TOKEN_INDENT))
                {
                    Stmt **block_stmts = arena_alloc(parser->arena, sizeof(Stmt *) * 2);
                    if (block_stmts == NULL)
                    {
                        exit(1);
                    }
                    block_stmts[0] = body;
                    Stmt *indented = parser_indented_block(parser);
                    block_stmts[1] = indented ? indented : ast_create_block_stmt(parser->arena, NULL, 0, NULL);
                    body = ast_create_block_stmt(parser->arena, block_stmts, 2, NULL);
                }
            }

            Stmt *stmt = ast_create_for_stmt(parser->arena, initializer, condition, increment, body, &for_token);
            if (stmt != NULL)
            {
                stmt->as.for_stmt.is_shared = is_shared;
            }
            return stmt;
        }
    }

    // Traditional for loop parsing
    Stmt *initializer = NULL;
    if (parser_match(parser, TOKEN_VAR))
    {
        Token var_token = parser->previous;
        Token name;
        if (parser_check(parser, TOKEN_IDENTIFIER))
        {
            name = parser->current;
            parser_advance(parser);
            name.start = arena_strndup(parser->arena, name.start, name.length);
            if (name.start == NULL)
            {
                parser_error_at_current(parser, "Out of memory");
                return NULL;
            }
        }
        else
        {
            parser_error_at_current(parser, "Expected variable name");
            name = parser->current;
            parser_advance(parser);
            name.start = arena_strndup(parser->arena, name.start, name.length);
            if (name.start == NULL)
            {
                parser_error_at_current(parser, "Out of memory");
                return NULL;
            }
        }
        parser_consume(parser, TOKEN_COLON, "Expected ':' after variable name");
        Type *type = parser_type(parser);
        Expr *init_expr = NULL;
        if (parser_match(parser, TOKEN_EQUAL))
        {
            init_expr = parser_expression(parser);
        }
        initializer = ast_create_var_decl_stmt(parser->arena, name, type, init_expr, &var_token);
    }
    else if (!parser_check(parser, TOKEN_SEMICOLON))
    {
        Expr *init_expr = parser_expression(parser);
        initializer = ast_create_expr_stmt(parser->arena, init_expr, NULL);
    }
    parser_consume(parser, TOKEN_SEMICOLON, "Expected ';' after initializer");

    Expr *condition = NULL;
    if (!parser_check(parser, TOKEN_SEMICOLON))
    {
        condition = parser_expression(parser);
    }
    parser_consume(parser, TOKEN_SEMICOLON, "Expected ';' after condition");

    Expr *increment = NULL;
    if (!parser_check(parser, TOKEN_ARROW))
    {
        increment = parser_expression(parser);
    }
    parser_consume(parser, TOKEN_ARROW, "Expected '=>' after for clauses");
    skip_newlines(parser);

    Stmt *body;
    if (parser_check(parser, TOKEN_INDENT))
    {
        body = parser_indented_block(parser);
    }
    else
    {
        body = parser_statement(parser);
        skip_newlines(parser);
        if (parser_check(parser, TOKEN_INDENT))
        {
            Stmt **block_stmts = arena_alloc(parser->arena, sizeof(Stmt *) * 2);
            if (block_stmts == NULL)
            {
                exit(1);
            }
            block_stmts[0] = body;
            Stmt *indented = parser_indented_block(parser);
            block_stmts[1] = indented ? indented : ast_create_block_stmt(parser->arena, NULL, 0, NULL);
            body = ast_create_block_stmt(parser->arena, block_stmts, 2, NULL);
        }
    }

    Stmt *stmt = ast_create_for_stmt(parser->arena, initializer, condition, increment, body, &for_token);
    if (stmt != NULL)
    {
        stmt->as.for_stmt.is_shared = is_shared;
    }
    return stmt;
}
