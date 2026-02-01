#include "parser/expr/parser_expr_lambda.h"
#include "parser/util/parser_util.h"
#include "parser/stmt/parser_stmt.h"
#include "ast/ast_expr.h"
#include <stdlib.h>
#include <string.h>

Expr *parse_lambda_expr(Parser *parser, Token *fn_token)
{
    parser_consume(parser, TOKEN_LEFT_PAREN, "Expected '(' after 'fn' in lambda");

    Parameter *params = NULL;
    int param_count = 0;
    int param_capacity = 0;

    if (!parser_check(parser, TOKEN_RIGHT_PAREN))
    {
        do
        {
            /* Parse: name [: type] [as val|ref] */
            Token param_name = parser->current;
            parser_consume(parser, TOKEN_IDENTIFIER, "Expected parameter name");
            param_name.start = arena_strndup(parser->arena, param_name.start, param_name.length);

            /* Type is optional - if no colon, set type to NULL for inference */
            Type *param_type = NULL;
            if (parser_match(parser, TOKEN_COLON))
            {
                param_type = parser_type(parser);
            }

            /* Parse optional "as val" or "as ref" for parameter */
            MemoryQualifier param_qualifier = parser_memory_qualifier(parser);

            /* Add to params array */
            if (param_count >= param_capacity)
            {
                param_capacity = param_capacity == 0 ? 4 : param_capacity * 2;
                Parameter *new_params = arena_alloc(parser->arena, sizeof(Parameter) * param_capacity);
                if (new_params == NULL)
                {
                    parser_error(parser, "Out of memory");
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
            param_count++;
        } while (parser_match(parser, TOKEN_COMMA));
    }

    parser_consume(parser, TOKEN_RIGHT_PAREN, "Expected ')' after lambda parameters");

    /* Parse optional function modifier (shared/private) before return type */
    FunctionModifier modifier = parser_function_modifier(parser);

    /* Return type is optional - if => comes next, set return_type to NULL for inference */
    Type *return_type = NULL;
    if (parser_match(parser, TOKEN_COLON))
    {
        return_type = parser_type(parser);
    }

    /* Parse body */
    parser_consume(parser, TOKEN_ARROW, "Expected '=>' before lambda body");

    /* Check for multi-line lambda body (newline followed by indent) */
    if (parser_check(parser, TOKEN_NEWLINE))
    {
        skip_newlines(parser);
        if (parser_check(parser, TOKEN_INDENT))
        {
            /* Multi-line lambda with statement body */
            Stmt *block = parser_indented_block(parser);
            if (block == NULL)
            {
                parser_error(parser, "Expected indented block for lambda body");
                return NULL;
            }
            Stmt **stmts = block->as.block.statements;
            int stmt_count = block->as.block.count;
            bool is_native_lambda = parser->in_native_function != 0;
            return ast_create_lambda_stmt_expr(parser->arena, params, param_count,
                                               return_type, stmts, stmt_count, modifier,
                                               is_native_lambda, fn_token);
        }
        /* Newline but no indent - single expression on next line is not valid */
        parser_error(parser, "Expected expression or indented block after '=>'");
        return NULL;
    }

    /* Single-line lambda with expression body */
    Expr *body = parser_expression(parser);
    bool is_native_lambda = parser->in_native_function != 0;
    return ast_create_lambda_expr(parser->arena, params, param_count, return_type, body,
                                  modifier, is_native_lambda, fn_token);
}
