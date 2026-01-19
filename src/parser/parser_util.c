#include "parser/parser_util.h"
#include "ast/ast_type.h"
#include "diagnostic.h"
#include "debug.h"
#include <stdio.h>
#include <string.h>

int parser_is_at_end(Parser *parser)
{
    return parser->current.type == TOKEN_EOF;
}

void skip_newlines(Parser *parser)
{
    while (parser_match(parser, TOKEN_NEWLINE))
    {
        if (parser_check(parser, TOKEN_INDENT) || parser_check(parser, TOKEN_DEDENT))
        {
            break;
        }
    }
}

int skip_newlines_and_check_end(Parser *parser)
{
    while (parser_match(parser, TOKEN_NEWLINE))
    {
    }
    return parser_is_at_end(parser);
}

void parser_error(Parser *parser, const char *message)
{
    parser_error_at(parser, &parser->previous, message);
}

void parser_error_at_current(Parser *parser, const char *message)
{
    parser_error_at(parser, &parser->current, message);
}

void parser_error_at(Parser *parser, Token *token, const char *message)
{
    if (parser->panic_mode)
        return;

    parser->panic_mode = 1;
    parser->had_error = 1;

    /* Use the diagnostic system for Rust-style error output */
    if (token->type == TOKEN_EOF)
    {
        diagnostic_error(token->filename, token->line, 1, 1,
                        "%s at end of file", message);
    }
    else if (token->type == TOKEN_ERROR)
    {
        /* Error token - message is in the token itself */
        diagnostic_error_at(token, "%s", message);
    }
    else
    {
        /* Regular token - show what we got vs what we expected */
        diagnostic_error_at(token, "%s (got '%.*s')", message, token->length, token->start);
    }

    parser->lexer->indent_size = 1;

    if (token == &parser->current)
    {
        parser_advance(parser);
    }
}

void parser_advance(Parser *parser)
{
    parser->previous = parser->current;

    for (;;)
    {
        parser->current = lexer_scan_token(parser->lexer);
        if (parser->current.type != TOKEN_ERROR)
            break;
        parser_error_at_current(parser, parser->current.start);
    }
}

void parser_consume(Parser *parser, SnTokenType type, const char *message)
{
    if (parser->current.type == type)
    {
        parser_advance(parser);
        return;
    }
    parser_error_at_current(parser, message);
}

int parser_check(Parser *parser, SnTokenType type)
{
    return parser->current.type == type;
}

int parser_match(Parser *parser, SnTokenType type)
{
    if (!parser_check(parser, type))
        return 0;
    parser_advance(parser);
    return 1;
}

void synchronize(Parser *parser)
{
    parser->panic_mode = 0;

    while (!parser_is_at_end(parser))
    {
        if (parser->previous.type == TOKEN_SEMICOLON || parser->previous.type == TOKEN_NEWLINE)
            return;

        switch (parser->current.type)
        {
        case TOKEN_FN:
        case TOKEN_VAR:
        case TOKEN_FOR:
        case TOKEN_IF:
        case TOKEN_WHILE:
        case TOKEN_RETURN:
        case TOKEN_IMPORT:
        case TOKEN_ELSE:
            return;
        case TOKEN_NEWLINE:
            parser_advance(parser);
            break;
        default:
            parser_advance(parser);
            break;
        }
    }
}

static Type *parser_function_type(Parser *parser)
{
    /* Parse function type: fn(param_types...): return_type */
    parser_consume(parser, TOKEN_LEFT_PAREN, "Expected '(' after 'fn' in function type");

    Type **param_types = NULL;
    int param_count = 0;
    int param_capacity = 0;

    if (!parser_check(parser, TOKEN_RIGHT_PAREN))
    {
        do
        {
            Type *param_type = parser_type(parser);
            if (param_count >= param_capacity)
            {
                param_capacity = param_capacity == 0 ? 4 : param_capacity * 2;
                Type **new_params = arena_alloc(parser->arena, sizeof(Type *) * param_capacity);
                if (new_params == NULL)
                {
                    parser_error(parser, "Out of memory");
                    return NULL;
                }
                if (param_types != NULL && param_count > 0)
                {
                    memcpy(new_params, param_types, sizeof(Type *) * param_count);
                }
                param_types = new_params;
            }
            param_types[param_count++] = param_type;
        } while (parser_match(parser, TOKEN_COMMA));
    }

    parser_consume(parser, TOKEN_RIGHT_PAREN, "Expected ')' after parameter types");
    parser_consume(parser, TOKEN_COLON, "Expected ':' before return type in function type");
    Type *return_type = parser_type(parser);

    return ast_create_function_type(parser->arena, return_type, param_types, param_count);
}

Type *parser_type(Parser *parser)
{
    Type *type = NULL;

    /* Handle pointer types: *T, **T, *void */
    if (parser_match(parser, TOKEN_STAR))
    {
        /* Parse the base type recursively (handles **T) */
        Type *base_type = parser_type(parser);
        return ast_create_pointer_type(parser->arena, base_type);
    }

    /* Handle parenthesized types for grouping, e.g., (fn(int): int)[] */
    if (parser_match(parser, TOKEN_LEFT_PAREN))
    {
        type = parser_type(parser);
        parser_consume(parser, TOKEN_RIGHT_PAREN, "Expected ')' after type");
        /* Fall through to array suffix handling below */
    }
    else if (parser_match(parser, TOKEN_FN))
    {
        type = parser_function_type(parser);
        /* Fall through to array suffix handling below */
    }
    else if (parser_match(parser, TOKEN_INT))
    {
        type = ast_create_primitive_type(parser->arena, TYPE_INT);
    }
    else if (parser_match(parser, TOKEN_INT32))
    {
        type = ast_create_primitive_type(parser->arena, TYPE_INT32);
    }
    else if (parser_match(parser, TOKEN_UINT))
    {
        type = ast_create_primitive_type(parser->arena, TYPE_UINT);
    }
    else if (parser_match(parser, TOKEN_UINT32))
    {
        type = ast_create_primitive_type(parser->arena, TYPE_UINT32);
    }
    else if (parser_match(parser, TOKEN_LONG))
    {
        type = ast_create_primitive_type(parser->arena, TYPE_LONG);
    }
    else if (parser_match(parser, TOKEN_DOUBLE))
    {
        type = ast_create_primitive_type(parser->arena, TYPE_DOUBLE);
    }
    else if (parser_match(parser, TOKEN_FLOAT))
    {
        type = ast_create_primitive_type(parser->arena, TYPE_FLOAT);
    }
    else if (parser_match(parser, TOKEN_CHAR))
    {
        type = ast_create_primitive_type(parser->arena, TYPE_CHAR);
    }
    else if (parser_match(parser, TOKEN_STR))
    {
        type = ast_create_primitive_type(parser->arena, TYPE_STRING);
    }
    else if (parser_match(parser, TOKEN_BOOL))
    {
        type = ast_create_primitive_type(parser->arena, TYPE_BOOL);
    }
    else if (parser_match(parser, TOKEN_BYTE))
    {
        type = ast_create_primitive_type(parser->arena, TYPE_BYTE);
    }
    else if (parser_match(parser, TOKEN_VOID))
    {
        type = ast_create_primitive_type(parser->arena, TYPE_VOID);
    }
    else if (parser_match(parser, TOKEN_ANY))
    {
        type = ast_create_primitive_type(parser->arena, TYPE_ANY);
    }
    else if (parser_check(parser, TOKEN_IDENTIFIER))
    {
        Token id = parser->current;
        {
            /* Check if this is a type alias (opaque type) */
            Symbol *type_symbol = symbol_table_lookup_type(parser->symbol_table, id);
            if (type_symbol != NULL && type_symbol->type != NULL)
            {
                Type *found_type = type_symbol->type;
                /* Check if this is an incomplete struct type (registered early for self-reference).
                 * An incomplete struct has kind TYPE_STRUCT but 0 fields.
                 * We treat it as a forward reference so type checker resolves it properly. */
                bool is_incomplete = (found_type->kind == TYPE_STRUCT &&
                                      found_type->as.struct_type.field_count == 0 &&
                                      found_type->as.struct_type.method_count == 0);
                if (is_incomplete)
                {
                    /* Create a forward reference that type checker will resolve */
                    char *type_name = arena_strndup(parser->arena, id.start, id.length);
                    parser_advance(parser);
                    type = ast_create_struct_type(parser->arena, type_name, NULL, 0, NULL, 0, false, false, false, NULL);
                }
                else
                {
                    parser_advance(parser);
                    /* Return a clone of the type to avoid aliasing issues */
                    type = ast_clone_type(parser->arena, found_type);
                }
            }
            else
            {
                /* Treat unknown identifier as potential struct type reference.
                 * Create a forward reference TYPE_STRUCT with just the name.
                 * The type checker will resolve this to the actual struct definition. */
                char *type_name = arena_strndup(parser->arena, id.start, id.length);
                parser_advance(parser);
                type = ast_create_struct_type(parser->arena, type_name, NULL, 0, NULL, 0, false, false, false, NULL);
            }
        }
    }
    else
    {
        parser_error_at_current(parser, "Expected type");
        return ast_create_primitive_type(parser->arena, TYPE_NIL);
    }

    while (parser_check(parser, TOKEN_LEFT_BRACKET))
    {
        parser_advance(parser); /* consume '[' */

        if (!parser_check(parser, TOKEN_RIGHT_BRACKET))
        {
            /* Sized array detected: TYPE[expr] - parse size expression */
            Expr *size_expr = parser_expression(parser);
            if (size_expr == NULL)
            {
                parser_error_at_current(parser, "Expected size expression in sized array type");
                return type;
            }

            /* Consume the closing bracket */
            if (!parser_match(parser, TOKEN_RIGHT_BRACKET))
            {
                parser_error_at_current(parser, "Expected ']' after size expression");
                return type;
            }

            /* Store for caller to use */
            parser->sized_array_pending = 1;
            parser->sized_array_size = size_expr;
            return type;
        }
        /* Normal array type: TYPE[] - consume ']' and create array type */
        parser_advance(parser); /* consume ']' */
        type = ast_create_array_type(parser->arena, type);
    }

    return type;
}

ParsedType parser_type_with_size(Parser *parser)
{
    ParsedType result;
    result.size_expr = NULL;
    result.is_sized_array = 0;

    /* Reset any previous sized array state */
    parser->sized_array_pending = 0;
    parser->sized_array_size = NULL;

    /* Parse the type */
    result.type = parser_type(parser);

    /* Check if parser_type() detected a sized array */
    if (parser->sized_array_pending)
    {
        result.size_expr = parser->sized_array_size;
        result.is_sized_array = 1;
        /* Reset the parser state */
        parser->sized_array_pending = 0;
        parser->sized_array_size = NULL;
    }

    return result;
}

/* List of known static type names that support static method calls */
static const char *static_type_names[] = {
    "Path",
    "Directory",
    "Bytes",
    "Stdin",
    "Stdout",
    "Stderr",
    "Interceptor",
    NULL
};

int parser_is_static_type_name(const char *name, int length)
{
    for (int i = 0; static_type_names[i] != NULL; i++)
    {
        if (strlen(static_type_names[i]) == (size_t)length &&
            strncmp(name, static_type_names[i], length) == 0)
        {
            return 1;
        }
    }
    return 0;
}

int parser_check_method_name(Parser *parser)
{
    /* Allow identifiers as method names */
    if (parser_check(parser, TOKEN_IDENTIFIER))
    {
        return 1;
    }
    /* Allow type keywords as method names (e.g., obj.int, obj.bool, etc.) */
    SnTokenType type = parser->current.type;
    if (type == TOKEN_INT || type == TOKEN_LONG || type == TOKEN_DOUBLE ||
        type == TOKEN_BOOL || type == TOKEN_BYTE)
    {
        return 1;
    }
    return 0;
}
