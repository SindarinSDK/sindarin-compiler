/**
 * parser_stmt_decl_var.c - Variable declaration parsing
 *
 * Contains functions for parsing variable declarations.
 */

#include "parser/stmt/parser_stmt_decl.h"
#include "parser/util/parser_util.h"
#include "parser/expr/parser_expr.h"
#include "ast/ast_expr.h"
#include "ast/ast_stmt.h"
#include <stdlib.h>
#include <string.h>

Stmt *parser_var_declaration(Parser *parser, SyncModifier sync_modifier)
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

    // Type annotation is optional if there's an initializer (type inference)
    Type *type = NULL;
    MemoryQualifier mem_qualifier = MEM_DEFAULT;
    Expr *sized_array_size_expr = NULL;
    if (parser_match(parser, TOKEN_COLON))
    {
        ParsedType parsed = parser_type_with_size(parser);
        type = parsed.type;

        /* Check if this is a sized array type (e.g., int[10]) */
        if (parsed.is_sized_array)
        {
            sized_array_size_expr = parsed.size_expr;
        }

        // Parse optional "as val" or "as ref" after type
        mem_qualifier = parser_memory_qualifier(parser);
    }

    Expr *initializer = NULL;
    if (parser_match(parser, TOKEN_EQUAL))
    {
        initializer = parser_expression(parser);
    }

    /* If this is a sized array declaration, create the sized array alloc expression */
    if (sized_array_size_expr != NULL)
    {
        Expr *default_value = initializer; /* The initializer becomes the default value */
        initializer = ast_create_sized_array_alloc_expr(
            parser->arena, type, sized_array_size_expr, default_value, &var_token);
        /* The variable type becomes an array of the element type */
        type = ast_create_array_type(parser->arena, type);
    }

    // Must have either type annotation or initializer (or both)
    if (type == NULL && initializer == NULL)
    {
        parser_error_at_current(parser, "Variable declaration requires type annotation or initializer");
    }

    /* After multi-line lambda with statement body, we may be at the next statement already
     * (no NEWLINE token between DEDENT and the next statement). Also handle DEDENT case
     * when the var declaration is the last statement in a block. */
    if (!parser_match(parser, TOKEN_SEMICOLON) && !parser_match(parser, TOKEN_NEWLINE))
    {
        /* Allow if we're at the next statement (IDENTIFIER covers print, etc.),
         * a keyword that starts a statement, end of block (DEDENT), or EOF */
        if (!parser_check(parser, TOKEN_IDENTIFIER) && !parser_check(parser, TOKEN_VAR) &&
            !parser_check(parser, TOKEN_FN) && !parser_check(parser, TOKEN_IF) &&
            !parser_check(parser, TOKEN_WHILE) && !parser_check(parser, TOKEN_FOR) &&
            !parser_check(parser, TOKEN_RETURN) && !parser_check(parser, TOKEN_BREAK) &&
            !parser_check(parser, TOKEN_CONTINUE) &&
            !parser_check(parser, TOKEN_DEDENT) && !parser_check(parser, TOKEN_EOF))
        {
            parser_consume(parser, TOKEN_SEMICOLON, "Expected ';' or newline after variable declaration");
        }
    }

    Stmt *stmt = ast_create_var_decl_stmt(parser->arena, name, type, initializer, &var_token);
    if (stmt != NULL)
    {
        stmt->as.var_decl.mem_qualifier = mem_qualifier;
        stmt->as.var_decl.sync_modifier = sync_modifier;
    }
    return stmt;
}
