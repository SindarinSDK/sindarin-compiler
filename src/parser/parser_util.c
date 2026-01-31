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

int skip_whitespace_for_continuation(Parser *parser)
{
    /* Handle method chain continuation across lines.
     * Supports two patterns:
     * 1. NEWLINE + DOT (same indentation)
     * 2. NEWLINE + INDENT + DOT (indented continuation)
     */
    if (parser->current.type != TOKEN_NEWLINE)
    {
        return 0;
    }

    /* Check if the next token after the newline is a continuation operator */
    Token peeked = parser_peek_token(parser);
    if (peeked.type == TOKEN_DOT)
    {
        /* Found continuation at same indentation - consume the newline and continue */
        parser_advance(parser);
        return 1;
    }

    /* Check for indented continuation: NEWLINE + INDENT + DOT */
    if (peeked.type == TOKEN_INDENT)
    {
        /* Save parser state to peek further */
        Token saved_current = parser->current;
        Token saved_previous = parser->previous;

        /* Consume NEWLINE to get to INDENT */
        parser_advance(parser);

        /* Now current is INDENT - peek what's after it */
        Token after_indent = parser_peek_token(parser);
        if (after_indent.type == TOKEN_DOT)
        {
            /* Found indented continuation - consume the INDENT and track it */
            parser_advance(parser);
            parser->continuation_indent_depth++;
            return 1;
        }

        /* Not a continuation - restore state */
        parser->current = saved_current;
        parser->previous = saved_previous;
    }

    return 0;
}

void consume_continuation_dedents(Parser *parser)
{
    /* Consume any DEDENT tokens that were created by consuming INDENT tokens
     * for method chain continuation. Also skip any intervening NEWLINEs. */
    while (parser->continuation_indent_depth > 0)
    {
        /* Skip any newlines before the DEDENT */
        while (parser_check(parser, TOKEN_NEWLINE))
        {
            parser_advance(parser);
        }

        if (parser_check(parser, TOKEN_DEDENT))
        {
            parser_advance(parser);
            parser->continuation_indent_depth--;
        }
        else
        {
            /* No DEDENT found - this shouldn't happen if indentation is balanced,
             * but break to avoid infinite loop */
            break;
        }
    }
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

Token parser_peek_token(Parser *parser)
{
    /* Save lexer state - must include all fields that lexer_scan_token might modify */
    const char *saved_start = parser->lexer->start;
    const char *saved_current = parser->lexer->current;
    int saved_line = parser->lexer->line;
    int saved_at_line_start = parser->lexer->at_line_start;
    int saved_indent_size = parser->lexer->indent_size;
    int saved_pending_indent = parser->lexer->pending_indent;
    const char *saved_pending_current = parser->lexer->pending_current;

    /* Save a copy of the indent stack */
    int saved_indent_stack[64]; /* Should be large enough for any reasonable nesting */
    int copy_size = saved_indent_size < 64 ? saved_indent_size : 64;
    if (copy_size > 0)
    {
        memcpy(saved_indent_stack, parser->lexer->indent_stack, sizeof(int) * copy_size);
    }

    /* Scan the next token */
    Token peeked = lexer_scan_token(parser->lexer);

    /* Restore all lexer state */
    parser->lexer->start = saved_start;
    parser->lexer->current = saved_current;
    parser->lexer->line = saved_line;
    parser->lexer->at_line_start = saved_at_line_start;
    parser->lexer->indent_size = saved_indent_size;
    parser->lexer->pending_indent = saved_pending_indent;
    parser->lexer->pending_current = saved_pending_current;

    /* Restore indent stack */
    if (copy_size > 0)
    {
        memcpy(parser->lexer->indent_stack, saved_indent_stack, sizeof(int) * copy_size);
    }

    return peeked;
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
        parser_advance(parser);

        /* Check for namespace-qualified type: Namespace.TypeName */
        if (parser_check(parser, TOKEN_DOT))
        {
            parser_advance(parser); /* consume '.' */
            if (!parser_check(parser, TOKEN_IDENTIFIER))
            {
                parser_error_at_current(parser, "Expected type name after '.' in qualified type");
                return ast_create_primitive_type(parser->arena, TYPE_NIL);
            }
            Token type_id = parser->current;
            parser_advance(parser);

            /* Create qualified name "Namespace.TypeName" for later resolution */
            int qualified_len = id.length + 1 + type_id.length;
            char *qualified_name = arena_alloc(parser->arena, qualified_len + 1);
            memcpy(qualified_name, id.start, id.length);
            qualified_name[id.length] = '.';
            memcpy(qualified_name + id.length + 1, type_id.start, type_id.length);
            qualified_name[qualified_len] = '\0';

            /* Create forward reference with qualified name - type checker will resolve */
            type = ast_create_struct_type(parser->arena, qualified_name, NULL, 0, NULL, 0, false, false, false, NULL);
        }
        else
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
                    type = ast_create_struct_type(parser->arena, type_name, NULL, 0, NULL, 0, false, false, false, NULL);
                }
                else
                {
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

/* List of built-in static type names that support static method calls.
 * SDK types (Path, Directory, Bytes) are recognized via symbol table lookup. */
static const char *static_type_names[] = {
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
    /* Allow type keywords as method names (e.g., obj.int, obj.bool, obj.any, etc.) */
    SnTokenType type = parser->current.type;
    if (type == TOKEN_INT || type == TOKEN_LONG || type == TOKEN_DOUBLE ||
        type == TOKEN_BOOL || type == TOKEN_BYTE || type == TOKEN_ANY)
    {
        return 1;
    }
    return 0;
}
