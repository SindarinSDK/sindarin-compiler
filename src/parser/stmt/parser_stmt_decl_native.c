/**
 * parser_stmt_decl_native.c - Native function declaration parsing
 *
 * Contains functions for parsing native function declarations and types.
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

Stmt *parser_native_function_declaration(Parser *parser, FunctionModifier modifier)
{
    Token native_token = parser->previous;  /* Points to 'fn' after 'native' */
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
    bool is_variadic = false;
    bool has_arena_param = false;

    if (parser_match(parser, TOKEN_LEFT_PAREN))
    {
        /* Check for 'arena' identifier as first parameter (implicit arena passing) */
        if (parser_check(parser, TOKEN_IDENTIFIER) &&
            parser->current.length == 5 &&
            memcmp(parser->current.start, "arena", 5) == 0)
        {
            /* Peek ahead to see if this is the contextual 'arena' keyword (followed by , or )) */
            /* If followed by ':', it's a regular parameter named 'arena', not the keyword */
            Token next = parser_peek_token(parser);
            if (next.type == TOKEN_COMMA || next.type == TOKEN_RIGHT_PAREN)
            {
                parser_advance(parser);  /* consume 'arena' */
                has_arena_param = true;
                /* Consume comma if there are more parameters */
                if (!parser_check(parser, TOKEN_RIGHT_PAREN))
                {
                    parser_consume(parser, TOKEN_COMMA, "Expected ',' after 'arena'");
                }
            }
        }

        if (!parser_check(parser, TOKEN_RIGHT_PAREN))
        {
            do
            {
                /* Check for variadic '...' - must be last in parameter list */
                if (parser_match(parser, TOKEN_SPREAD))
                {
                    is_variadic = true;
                    /* '...' must be followed by ')' - no more parameters allowed */
                    break;
                }

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
                /* Parse optional "as val" for parameter */
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
            "Example: %s native fn %.*s(...): type",
            keyword, keyword, name.length, name.start);
        parser_error_at_current(parser, error_msg);
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
    /* Mark function type as variadic and native */
    function_type->as.function.is_variadic = is_variadic;
    function_type->as.function.is_native = true;  /* Native functions need is_native on the type too */
    function_type->as.function.has_arena_param = has_arena_param;

    DEBUG_VERBOSE("Parsed native function '%.*s' with has_arena_param=%d",
                  name.length, name.start, has_arena_param);

    symbol_table_add_symbol(parser->symbol_table, name, function_type);

    Stmt **stmts = NULL;
    int stmt_count = 0;

    /* Native functions can have either:
     * 1. No body (just declaration) - ends with newline or semicolon
     * 2. A Sindarin body using '=>' (native fn with Sindarin implementation)
     *    This can be expression-bodied (same line) or block-bodied (indented)
     */
    if (parser_match(parser, TOKEN_ARROW))
    {
        Token arrow_token = parser->previous;

        /* Set native context so lambdas parsed in body are marked as native */
        int saved_in_native = parser->in_native_function;
        parser->in_native_function = 1;

        /* Check for expression-bodied native function (expression on same line as arrow) */
        if (parser->current.line == arrow_token.line &&
            parser_can_start_expression(parser->current.type))
        {
            /* Expression-bodied native function: native fn foo(): int => 42 */
            Expr *body_expr = parser_expression(parser);

            /* Create an implicit return statement wrapping the expression */
            Stmt *return_stmt = ast_create_return_stmt(parser->arena, arrow_token, body_expr, &arrow_token);

            stmts = arena_alloc(parser->arena, sizeof(Stmt *));
            if (stmts == NULL)
            {
                parser_error(parser, "Out of memory");
                parser->in_native_function = saved_in_native;
                return NULL;
            }
            stmts[0] = return_stmt;
            stmt_count = 1;
        }
        else
        {
            /* Block-bodied native function */
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

        /* Restore native context */
        parser->in_native_function = saved_in_native;
    }
    else
    {
        /* Native function without body (external C function declaration) */
        if (!parser_match(parser, TOKEN_SEMICOLON) && !parser_match(parser, TOKEN_NEWLINE))
        {
            if (!parser_check(parser, TOKEN_NEWLINE) && !parser_check(parser, TOKEN_EOF) &&
                !parser_check(parser, TOKEN_FN) && !parser_check(parser, TOKEN_NATIVE) &&
                !parser_check(parser, TOKEN_VAR) && !parser_check(parser, TOKEN_DEDENT))
            {
                parser_consume(parser, TOKEN_NEWLINE, "Expected newline or '=>' after native function signature");
            }
        }
    }

    Stmt *func_stmt = ast_create_function_stmt(parser->arena, name, params, param_count, return_type, stmts, stmt_count, &native_token);
    if (func_stmt != NULL)
    {
        func_stmt->as.function.modifier = func_modifier;
        func_stmt->as.function.is_native = true;
        func_stmt->as.function.is_variadic = is_variadic;
        func_stmt->as.function.has_arena_param = has_arena_param;

        /* Handle #pragma alias for native functions */
        if (parser->pending_alias != NULL)
        {
            func_stmt->as.function.c_alias = parser->pending_alias;
            parser->pending_alias = NULL;
        }
    }
    return func_stmt;
}

/* Parse a native function type: native fn(params): return_type */
Type *parser_native_function_type(Parser *parser)
{
    /* Consume 'fn' after 'native' */
    parser_consume(parser, TOKEN_FN, "Expected 'fn' after 'native' in type declaration");
    parser_consume(parser, TOKEN_LEFT_PAREN, "Expected '(' after 'fn' in native function type");

    Type **param_types = NULL;
    int param_count = 0;
    int param_capacity = 0;

    if (!parser_check(parser, TOKEN_RIGHT_PAREN))
    {
        do
        {
            /* For native function types, we parse: param_name: type */
            /* Skip the parameter name if present */
            if (parser_check(parser, TOKEN_IDENTIFIER))
            {
                Token param_name = parser->current;
                parser_advance(parser);

                /* Check if followed by colon (named parameter) or it's a type name */
                if (parser_check(parser, TOKEN_COLON))
                {
                    parser_advance(parser); /* consume ':' */
                    /* Now parse the actual type */
                }
                else
                {
                    /* It was a type name, need to back up - but we can't.
                     * Instead, create the type from the identifier.
                     * Check if it's a known type */
                    Symbol *type_symbol = symbol_table_lookup_type(parser->symbol_table, param_name);
                    if (type_symbol != NULL && type_symbol->type != NULL)
                    {
                        Type *param_type = ast_clone_type(parser->arena, type_symbol->type);
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
                        continue;
                    }
                    else
                    {
                        parser_error_at_current(parser, "Expected ':' after parameter name in native function type");
                        return NULL;
                    }
                }
            }

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
    parser_consume(parser, TOKEN_COLON, "Expected ':' before return type in native function type");
    Type *return_type = parser_type(parser);

    Type *func_type = ast_create_function_type(parser->arena, return_type, param_types, param_count);
    func_type->as.function.is_native = true;

    return func_type;
}
