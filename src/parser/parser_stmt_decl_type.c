/**
 * parser_stmt_decl_type.c - Type declaration parsing
 *
 * Contains functions for parsing type alias declarations.
 */

#include "parser/parser_stmt_decl.h"
#include "parser/parser_util.h"
#include "parser/parser_expr.h"
#include "ast/ast_expr.h"
#include "ast/ast_stmt.h"
#include <stdlib.h>
#include <string.h>

Stmt *parser_type_declaration(Parser *parser)
{
    Token type_token = parser->previous;
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
        parser_error_at_current(parser, "Expected type alias name");
        return NULL;
    }

    parser_consume(parser, TOKEN_EQUAL, "Expected '=' after type alias name");

    Type *declared_type = NULL;

    /* Check for 'native fn(...)' (native callback type) or 'opaque' */
    if (parser_match(parser, TOKEN_NATIVE))
    {
        /* Parse native function type: native fn(params): return_type */
        declared_type = parser_native_function_type(parser);
        /* Store the typedef name for code generation */
        if (declared_type != NULL)
        {
            /* Create null-terminated string from token */
            char *typedef_name = arena_alloc(parser->arena, name.length + 1);
            strncpy(typedef_name, name.start, name.length);
            typedef_name[name.length] = '\0';
            declared_type->as.function.typedef_name = typedef_name;
        }
    }
    else if (parser_match(parser, TOKEN_OPAQUE))
    {
        /* Create the opaque type with the name */
        declared_type = ast_create_opaque_type(parser->arena, name.start);
    }
    else if (parser_check(parser, TOKEN_FN))
    {
        /* Parse regular function type alias: fn(params): return_type */
        declared_type = parser_type(parser);
        /* Store the typedef name for code generation */
        if (declared_type != NULL && declared_type->kind == TYPE_FUNCTION)
        {
            char *typedef_name = arena_alloc(parser->arena, name.length + 1);
            strncpy(typedef_name, name.start, name.length);
            typedef_name[name.length] = '\0';
            declared_type->as.function.typedef_name = typedef_name;
        }
    }
    else
    {
        parser_error_at_current(parser, "Expected 'opaque', 'native fn', or 'fn' after '=' in type declaration");
        return NULL;
    }

    if (declared_type == NULL)
    {
        return NULL;
    }

    /* Register the type in the symbol table immediately so it can be used by later declarations */
    symbol_table_add_type(parser->symbol_table, name, declared_type);

    /* Consume optional newline/semicolon */
    if (!parser_match(parser, TOKEN_SEMICOLON) && !parser_check(parser, TOKEN_NEWLINE) && !parser_is_at_end(parser))
    {
        if (!parser_check(parser, TOKEN_DEDENT) && !parser_check(parser, TOKEN_FN) &&
            !parser_check(parser, TOKEN_NATIVE) && !parser_check(parser, TOKEN_VAR) &&
            !parser_check(parser, TOKEN_KEYWORD_TYPE))
        {
            parser_consume(parser, TOKEN_SEMICOLON, "Expected ';' or newline after type declaration");
        }
    }
    else if (parser_match(parser, TOKEN_SEMICOLON))
    {
    }

    return ast_create_type_decl_stmt(parser->arena, name, declared_type, &type_token);
}
