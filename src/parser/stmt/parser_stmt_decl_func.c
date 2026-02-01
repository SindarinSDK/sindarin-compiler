/**
 * parser_stmt_decl_func.c - Function declaration parsing
 *
 * Contains functions for parsing regular and native function declarations.
 */

#include "parser/stmt/parser_stmt_decl.h"
#include "parser/util/parser_util.h"
#include "parser/expr/parser_expr.h"
#include "parser/stmt/parser_stmt.h"
#include "ast/ast_expr.h"
#include "ast/ast_stmt.h"
#include "debug.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Helper function to check if a token type can start an expression */
int parser_can_start_expression(SnTokenType type)
{
    switch (type)
    {
        /* Literals */
        case TOKEN_INT_LITERAL:
        case TOKEN_LONG_LITERAL:
        case TOKEN_BYTE_LITERAL:
        case TOKEN_DOUBLE_LITERAL:
        case TOKEN_FLOAT_LITERAL:
        case TOKEN_UINT_LITERAL:
        case TOKEN_UINT32_LITERAL:
        case TOKEN_INT32_LITERAL:
        case TOKEN_CHAR_LITERAL:
        case TOKEN_STRING_LITERAL:
        case TOKEN_INTERPOL_STRING:
        case TOKEN_BOOL_LITERAL:
        case TOKEN_NIL:
        /* Identifiers and lambdas */
        case TOKEN_IDENTIFIER:
        case TOKEN_FN:
        /* Grouping and array literals */
        case TOKEN_LEFT_PAREN:
        case TOKEN_LEFT_BRACE:
        /* Unary operators */
        case TOKEN_BANG:
        case TOKEN_MINUS:
        /* Type operators */
        case TOKEN_TYPEOF:
        case TOKEN_SIZEOF:
        /* Function reference */
        case TOKEN_AMPERSAND:
            return 1;
        default:
            return 0;
    }
}

Stmt *parser_function_declaration(Parser *parser, FunctionModifier modifier)
{
    Token fn_token = parser->previous;
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
        parser_error_at_current(parser, "Expected function name");
        name = parser->current;
        parser_advance(parser);
        name.start = arena_strndup(parser->arena, name.start, name.length);
        if (name.start == NULL)
        {
            parser_error_at_current(parser, "Out of memory");
            return NULL;
        }
    }

    Parameter *params = NULL;
    int param_count = 0;
    int param_capacity = 0;

    if (parser_match(parser, TOKEN_LEFT_PAREN))
    {
        if (!parser_check(parser, TOKEN_RIGHT_PAREN))
        {
            do
            {
                if (param_count >= 255)
                {
                    parser_error_at_current(parser, "Cannot have more than 255 parameters");
                }
                Token param_name;
                if (parser_check(parser, TOKEN_IDENTIFIER))
                {
                    param_name = parser->current;
                    parser_advance(parser);
                    param_name.start = arena_strndup(parser->arena, param_name.start, param_name.length);
                    if (param_name.start == NULL)
                    {
                        parser_error_at_current(parser, "Out of memory");
                        return NULL;
                    }
                }
                else
                {
                    parser_error_at_current(parser, "Expected parameter name");
                    param_name = parser->current;
                    parser_advance(parser);
                    param_name.start = arena_strndup(parser->arena, param_name.start, param_name.length);
                    if (param_name.start == NULL)
                    {
                        parser_error_at_current(parser, "Out of memory");
                        return NULL;
                    }
                }
                parser_consume(parser, TOKEN_COLON, "Expected ':' after parameter name");
                Type *param_type = parser_type(parser);
                // Parse optional "as val" for parameter
                MemoryQualifier param_qualifier = parser_memory_qualifier(parser);

                if (param_count >= param_capacity)
                {
                    param_capacity = param_capacity == 0 ? 8 : param_capacity * 2;
                    Parameter *new_params = arena_alloc(parser->arena, sizeof(Parameter) * param_capacity);
                    if (new_params == NULL)
                    {
                        exit(1);
                    }
                    if (params != NULL && param_count > 0)
                    {
                        memcpy(new_params, params, sizeof(Parameter) * param_count);
                    }
                    params = new_params;
                }
                params[param_count].name = param_name;
                params[param_count].type = param_type;
                params[param_count].mem_qualifier = param_qualifier;
                params[param_count].sync_modifier = SYNC_NONE;
                param_count++;
            } while (parser_match(parser, TOKEN_COMMA));
        }
        parser_consume(parser, TOKEN_RIGHT_PAREN, "Expected ')' after parameters");
    }

    /* Check for misplaced modifiers after parameters */
    if (parser_check(parser, TOKEN_SHARED) || parser_check(parser, TOKEN_PRIVATE) || parser_check(parser, TOKEN_STATIC))
    {
        const char *keyword = parser_check(parser, TOKEN_SHARED) ? "shared" :
                              parser_check(parser, TOKEN_PRIVATE) ? "private" : "static";
        char error_msg[256];
        snprintf(error_msg, sizeof(error_msg),
            "'%s' must be declared before 'fn', not after the parameter list. "
            "Example: %s fn %.*s(...): type => ...",
            keyword, keyword, name.length, name.start);
        parser_error_at_current(parser, error_msg);
        /* Consume the misplaced modifier to allow continued parsing */
        parser_advance(parser);
    }

    FunctionModifier func_modifier = modifier;

    Type *return_type = ast_create_primitive_type(parser->arena, TYPE_VOID);
    if (parser_match(parser, TOKEN_COLON))
    {
        return_type = parser_type(parser);
    }

    Type **param_types = arena_alloc(parser->arena, sizeof(Type *) * param_count);
    if (param_types == NULL && param_count > 0)
    {
        exit(1);
    }
    for (int i = 0; i < param_count; i++)
    {
        param_types[i] = params[i].type;
    }
    Type *function_type = ast_create_function_type(parser->arena, return_type, param_types, param_count);

    symbol_table_add_symbol(parser->symbol_table, name, function_type);

    parser_consume(parser, TOKEN_ARROW, "Expected '=>' before function body");

    /* Check for expression-bodied function (expression on same line as arrow) */
    Token arrow_token = parser->previous;
    Stmt **stmts = NULL;
    int stmt_count = 0;

    if (parser->current.line == arrow_token.line &&
        parser_can_start_expression(parser->current.type))
    {
        /* Expression-bodied function: fn foo(): int => 42 */
        Expr *body_expr = parser_expression(parser);

        /* Create an implicit return statement wrapping the expression */
        Stmt *return_stmt = ast_create_return_stmt(parser->arena, arrow_token, body_expr, &arrow_token);

        stmts = arena_alloc(parser->arena, sizeof(Stmt *));
        if (stmts == NULL)
        {
            parser_error(parser, "Out of memory");
            return NULL;
        }
        stmts[0] = return_stmt;
        stmt_count = 1;
    }
    else
    {
        /* Block-bodied function */
        skip_newlines(parser);

        Stmt *body = parser_indented_block(parser);
        if (body == NULL)
        {
            body = ast_create_block_stmt(parser->arena, NULL, 0, NULL);
        }

        stmts = body->as.block.statements;
        stmt_count = body->as.block.count;
        body->as.block.statements = NULL;
    }

    Stmt *func_stmt = ast_create_function_stmt(parser->arena, name, params, param_count, return_type, stmts, stmt_count, &fn_token);
    if (func_stmt != NULL)
    {
        func_stmt->as.function.modifier = func_modifier;
    }
    return func_stmt;
}
