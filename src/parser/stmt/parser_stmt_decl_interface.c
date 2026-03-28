/**
 * parser_stmt_decl_interface.c - Interface declaration parsing
 *
 * Contains the function for parsing interface declarations.
 *
 * Syntax:
 *   interface <Name> =>
 *       fn <method>(<params>): <return_type>
 *       ...
 */

#include "parser/stmt/parser_stmt_decl.h"
#include "parser/util/parser_util.h"
#include "parser/expr/parser_expr.h"
#include "parser/stmt/parser_stmt.h"
#include "ast/ast_type.h"
#include "ast/ast_stmt.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Stmt *parser_interface_declaration(Parser *parser)
{
    Token interface_token = parser->previous;
    Token name;

    /* Parse interface name */
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
        parser_error_at_current(parser, "Expected interface name");
        return NULL;
    }

    /* Parse '=>' */
    parser_consume(parser, TOKEN_ARROW, "Expected '=>' after interface name");
    skip_newlines(parser);

    StructMethod *methods = NULL;
    int method_count = 0;
    int method_capacity = 0;

    /* Expect an indented block */
    if (!parser_check(parser, TOKEN_INDENT))
    {
        parser_error_at_current(parser, "Expected indented block of method signatures after interface declaration");
        return NULL;
    }
    parser_advance(parser);  /* consume INDENT */

    /* Parse method signatures until DEDENT or EOF */
    while (!parser_is_at_end(parser) && !parser_check(parser, TOKEN_DEDENT))
    {
        /* Skip blank lines */
        while (parser_match(parser, TOKEN_NEWLINE))
        {
        }

        if (parser_check(parser, TOKEN_DEDENT) || parser_is_at_end(parser))
        {
            break;
        }

        /* Expect 'fn' */
        if (!parser_match(parser, TOKEN_FN))
        {
            parser_error_at_current(parser, "Expected 'fn' for interface method signature");
            break;
        }

        /* Parse method name */
        if (!parser_check(parser, TOKEN_IDENTIFIER))
        {
            parser_error_at_current(parser, "Expected method name after 'fn'");
            break;
        }
        Token method_name = parser->current;
        parser_advance(parser);
        method_name.start = arena_strndup(parser->arena, method_name.start, method_name.length);
        if (method_name.start == NULL)
        {
            parser_error_at_current(parser, "Out of memory");
            return NULL;
        }

        /* Parse parameter list */
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

                    /* Parameter name — allow 'val' as a parameter name */
                    if (parser_check(parser, TOKEN_VAL))
                        parser->current.type = TOKEN_IDENTIFIER;
                    if (!parser_check(parser, TOKEN_IDENTIFIER))
                    {
                        parser_error_at_current(parser, "Expected parameter name");
                        return NULL;
                    }
                    Token param_name = parser->current;
                    parser_advance(parser);
                    param_name.start = arena_strndup(parser->arena, param_name.start, param_name.length);
                    if (param_name.start == NULL)
                    {
                        parser_error_at_current(parser, "Out of memory");
                        return NULL;
                    }

                    parser_consume(parser, TOKEN_COLON, "Expected ':' after parameter name");
                    Type *param_type = parser_type(parser);
                    MemoryQualifier param_qualifier = parser_memory_qualifier(parser);

                    /* Grow params array if needed */
                    if (param_count >= param_capacity)
                    {
                        param_capacity = param_capacity == 0 ? 8 : param_capacity * 2;
                        Parameter *new_params = arena_alloc(parser->arena, sizeof(Parameter) * param_capacity);
                        if (new_params == NULL)
                        {
                            parser_error_at_current(parser, "Out of memory");
                            return NULL;
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

        /* Parse optional return type */
        Type *return_type = ast_create_primitive_type(parser->arena, TYPE_VOID);
        if (parser_match(parser, TOKEN_COLON))
        {
            return_type = parser_type(parser);
        }

        /* Reject method bodies — interface methods are signatures only */
        if (parser_check(parser, TOKEN_ARROW))
        {
            char msg[256];
            snprintf(msg, sizeof(msg),
                     "interface method '%s' cannot have a body",
                     method_name.start);
            parser_error_at_current(parser, msg);
            parser_advance(parser);  /* consume '=>' */
            /* Consume the body block if indented */
            skip_newlines(parser);
            if (parser_check(parser, TOKEN_INDENT))
            {
                int depth = 1;
                parser_advance(parser);  /* consume INDENT */
                while (!parser_is_at_end(parser) && depth > 0)
                {
                    if (parser_check(parser, TOKEN_INDENT)) depth++;
                    else if (parser_check(parser, TOKEN_DEDENT)) depth--;
                    parser_advance(parser);
                }
            }
            else
            {
                /* Inline body — consume to end of line */
                while (!parser_is_at_end(parser) &&
                       !parser_check(parser, TOKEN_NEWLINE) &&
                       !parser_check(parser, TOKEN_DEDENT))
                {
                    parser_advance(parser);
                }
            }
            parser_match(parser, TOKEN_NEWLINE);
            continue;  /* skip adding this method, move to next */
        }

        /* Grow methods array if needed */
        if (method_count >= method_capacity)
        {
            method_capacity = method_capacity == 0 ? 4 : method_capacity * 2;
            StructMethod *new_methods = arena_alloc(parser->arena, sizeof(StructMethod) * method_capacity);
            if (new_methods == NULL)
            {
                parser_error_at_current(parser, "Out of memory");
                return NULL;
            }
            if (methods != NULL && method_count > 0)
            {
                memcpy(new_methods, methods, sizeof(StructMethod) * method_count);
            }
            methods = new_methods;
        }

        StructMethod *m = &methods[method_count];
        memset(m, 0, sizeof(StructMethod));
        m->name = method_name.start;
        m->params = params;
        m->param_count = param_count;
        m->return_type = return_type;
        m->return_mem_qualifier = MEM_DEFAULT;
        m->body = NULL;
        m->body_count = 0;
        m->modifier = FUNC_DEFAULT;
        m->is_static = false;
        m->is_native = false;
        m->has_arena_param = false;
        m->name_token = method_name;
        m->c_alias = NULL;
        m->is_operator = false;
        m->operator_token = TOKEN_EOF;
        method_count++;

        /* Consume trailing newline */
        parser_match(parser, TOKEN_NEWLINE);
    }

    /* Consume DEDENT */
    if (parser_check(parser, TOKEN_DEDENT))
    {
        parser_advance(parser);
    }

    /* Register interface type in symbol table so it can be referenced later */
    Type *interface_type = ast_create_interface_type(parser->arena, name.start, methods, method_count);
    symbol_table_add_type(parser->symbol_table, name, interface_type);

    return ast_create_interface_decl_stmt(parser->arena, name, methods, method_count, &interface_token);
}
