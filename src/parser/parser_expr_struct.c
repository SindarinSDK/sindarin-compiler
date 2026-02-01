#include "parser_expr_struct.h"
#include "parser_util.h"
#include "ast/ast_expr.h"
#include <stdlib.h>
#include <string.h>

Expr *parse_struct_literal(Parser *parser, Token *var_token)
{
    Token struct_name = *var_token;
    struct_name.start = arena_strndup(parser->arena, var_token->start, var_token->length);
    if (struct_name.start == NULL)
    {
        parser_error(parser, "Out of memory");
        return NULL;
    }

    parser_advance(parser);  /* consume '{' */
    Token left_brace = parser->previous;

    FieldInitializer *fields = NULL;
    int field_count = 0;
    int field_capacity = 0;

    /* Skip newlines/indent/dedent after opening brace for multi-line struct literals */
    while (parser_match(parser, TOKEN_NEWLINE) ||
           parser_match(parser, TOKEN_INDENT) ||
           parser_match(parser, TOKEN_DEDENT)) {}

    if (!parser_check(parser, TOKEN_RIGHT_BRACE))
    {
        do
        {
            /* Skip newlines/indent/dedent after comma for multi-line struct literals */
            while (parser_match(parser, TOKEN_NEWLINE) ||
                   parser_match(parser, TOKEN_INDENT) ||
                   parser_match(parser, TOKEN_DEDENT)) {}

            /* Parse field name */
            if (!parser_check(parser, TOKEN_IDENTIFIER))
            {
                parser_error_at_current(parser, "Expected field name in struct literal");
                break;
            }
            Token field_name = parser->current;
            parser_advance(parser);

            /* Parse colon */
            parser_consume(parser, TOKEN_COLON, "Expected ':' after field name");

            /* Parse field value */
            Expr *field_value = parser_expression(parser);
            if (field_value == NULL)
            {
                parser_error(parser, "Expected field value");
                break;
            }

            /* Add to fields array */
            if (field_count >= field_capacity)
            {
                field_capacity = field_capacity == 0 ? 4 : field_capacity * 2;
                FieldInitializer *new_fields = arena_alloc(parser->arena,
                    sizeof(FieldInitializer) * field_capacity);
                if (new_fields == NULL)
                {
                    parser_error(parser, "Out of memory");
                    return NULL;
                }
                if (fields != NULL && field_count > 0)
                {
                    memcpy(new_fields, fields, sizeof(FieldInitializer) * field_count);
                }
                fields = new_fields;
            }
            fields[field_count].name = field_name;
            fields[field_count].value = field_value;
            field_count++;
        } while (parser_match(parser, TOKEN_COMMA));
    }

    /* Skip newlines/indent/dedent before closing brace for multi-line struct literals */
    while (parser_match(parser, TOKEN_NEWLINE) ||
           parser_match(parser, TOKEN_INDENT) ||
           parser_match(parser, TOKEN_DEDENT)) {}

    parser_consume(parser, TOKEN_RIGHT_BRACE, "Expected '}' after struct literal");

    return ast_create_struct_literal_expr(parser->arena, struct_name, fields,
                                           field_count, &left_brace);
}

Expr *parse_static_call(Parser *parser, Token *var_token)
{
    /* Save the type name token */
    Token type_name = *var_token;
    type_name.start = arena_strndup(parser->arena, var_token->start, var_token->length);

    /* Consume the dot */
    parser_advance(parser);

    /* Expect method name (identifier or type keyword like int, long, double, etc.) */
    if (!parser_check_method_name(parser))
    {
        parser_error_at_current(parser, "Expected method name after '.'");
        return NULL;
    }
    Token method_name = parser->current;
    method_name.start = arena_strndup(parser->arena, parser->current.start, parser->current.length);
    parser_advance(parser);

    /* Check for opening paren - must be a call */
    if (!parser_check(parser, TOKEN_LEFT_PAREN))
    {
        parser_error_at_current(parser, "Expected '(' after static method name");
        return NULL;
    }

    /* Parse arguments */
    parser_advance(parser);  /* consume '(' */

    Expr **arguments = NULL;
    int arg_count = 0;
    int arg_capacity = 0;

    if (!parser_check(parser, TOKEN_RIGHT_PAREN))
    {
        do
        {
            Expr *arg = parser_expression(parser);
            if (arg_count >= arg_capacity)
            {
                arg_capacity = arg_capacity == 0 ? 4 : arg_capacity * 2;
                Expr **new_args = arena_alloc(parser->arena, sizeof(Expr *) * arg_capacity);
                if (new_args == NULL)
                {
                    parser_error(parser, "Out of memory");
                    return NULL;
                }
                if (arguments != NULL && arg_count > 0)
                {
                    memcpy(new_args, arguments, sizeof(Expr *) * arg_count);
                }
                arguments = new_args;
            }
            arguments[arg_count++] = arg;
        } while (parser_match(parser, TOKEN_COMMA));
    }

    parser_consume(parser, TOKEN_RIGHT_PAREN, "Expected ')' after arguments");

    return ast_create_static_call_expr(parser->arena, type_name, method_name,
                                       arguments, arg_count, &type_name);
}
