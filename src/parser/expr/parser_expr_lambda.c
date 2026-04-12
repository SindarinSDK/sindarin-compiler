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
            /* Allow 'val' as a parameter name */
            if (parser_check(parser, TOKEN_VAL))
                parser->current.type = TOKEN_IDENTIFIER;
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

    FunctionModifier modifier = FUNC_DEFAULT;

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

    /* Inside balanced delimiters (e.g. function call arguments), the lexer
     * suppresses NEWLINE and INDENT tokens. If the token after '=>' is on a
     * different line, a newline was suppressed and this may be a multi-line
     * lambda.  Re-lex from the position after '=>' with bracket_depth
     * temporarily set to 0 so the lexer emits NEWLINE/INDENT/DEDENT normally
     * for the lambda body. */
    if (parser->lexer->bracket_depth > 0 &&
        parser->current.line > parser->previous.line)
    {
        int saved_depth = parser->lexer->bracket_depth;

        /* lexer_make_token arena-copies token text, so parser->previous.start
         * does NOT point into the source buffer.  Use the lexer's own source
         * pointer instead: lexer->start currently points to the first character
         * of parser->current's token in the real source buffer.  Scan backwards
         * from there past the leading whitespace and the newline to find the
         * position right after '=>' in the source. */
        const char *src_cur_token = parser->lexer->start;
        const char *p = src_cur_token;
        /* Back up past whitespace that precedes the current token on its line */
        while (p > parser->lexer->start - 256 && p[-1] != '\n' && p[-1] != '\0')
            p--;
        /* Now p points to the first character of the current line (past \n).
         * Go back one more to the \n itself. */
        if (p > parser->lexer->start - 256 && p[-1] == '\n')
            p--;
        /* p now points to the \n between '=>' and the lambda body. */

        /* Reset lexer to that \n and re-lex with bracket_depth=0 */
        parser->lexer->bracket_depth = 0;
        parser->lexer->current = p;
        parser->lexer->start = p;
        parser->lexer->line = parser->previous.line;
        parser->lexer->at_line_start = 0;
        parser->lexer->pending_indent = -1;
        parser->lexer->pending_current = NULL;

        parser_advance(parser); /* should yield TOKEN_NEWLINE */

        if (parser_check(parser, TOKEN_NEWLINE))
        {
            skip_newlines(parser);
            if (parser_check(parser, TOKEN_INDENT))
            {
                /* Multi-line lambda inside balanced delimiters */
                Stmt *block = parser_indented_block(parser);

                /* Restore bracket_depth.  The token now sitting in
                 * parser->current was scanned while bracket_depth was 0.
                 * If it is a closing delimiter the decrement that the lexer
                 * normally performs was skipped, so apply it here. */
                parser->lexer->bracket_depth = saved_depth;
                if (parser->current.type == TOKEN_RIGHT_PAREN ||
                    parser->current.type == TOKEN_RIGHT_BRACKET ||
                    parser->current.type == TOKEN_RIGHT_BRACE)
                {
                    parser->lexer->bracket_depth--;
                }

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
        }

        /* Not actually multi-line — restore bracket_depth and fall through
         * to single-line parsing.  Re-lex from the \n with original
         * bracket_depth so parser->current is correct. */
        parser->lexer->bracket_depth = saved_depth;
        parser->lexer->current = p;
        parser->lexer->start = p;
        parser->lexer->line = parser->previous.line;
        parser->lexer->at_line_start = 0;
        parser->lexer->pending_indent = -1;
        parser->lexer->pending_current = NULL;
        parser_advance(parser);
    }

    /* Single-line lambda with expression body */
    Expr *body = parser_expression(parser);
    bool is_native_lambda = parser->in_native_function != 0;
    return ast_create_lambda_expr(parser->arena, params, param_count, return_type, body,
                                  modifier, is_native_lambda, fn_token);
}
