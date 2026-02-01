#include "parser/expr/parser_expr_match.h"
#include "parser/util/parser_util.h"
#include "parser/stmt/parser_stmt.h"
#include "ast/ast_expr.h"
#include <stdlib.h>
#include <string.h>

Expr *parse_match_expr(Parser *parser, Token *match_token)
{
    /* Parse subject expression */
    Expr *subject = parser_expression(parser);

    /* Consume => after subject */
    parser_consume(parser, TOKEN_ARROW, "Expected '=>' after match subject");

    /* Skip newlines before indented block */
    while (parser_match(parser, TOKEN_NEWLINE)) {}

    /* Expect INDENT for arm block */
    if (!parser_check(parser, TOKEN_INDENT))
    {
        parser_error(parser, "Expected indented block after match");
        return subject;
    }
    parser_advance(parser); /* consume INDENT */

    /* Parse arms */
    MatchArm *arms = NULL;
    int arm_count = 0;
    int arm_capacity = 0;

    while (!parser_check(parser, TOKEN_DEDENT) && !parser_is_at_end(parser))
    {
        while (parser_match(parser, TOKEN_NEWLINE)) {}

        if (parser_check(parser, TOKEN_DEDENT) || parser_is_at_end(parser))
        {
            break;
        }

        /* Grow arms array */
        if (arm_count >= arm_capacity)
        {
            arm_capacity = arm_capacity == 0 ? 4 : arm_capacity * 2;
            MatchArm *new_arms = arena_alloc(parser->arena, sizeof(MatchArm) * arm_capacity);
            if (new_arms == NULL)
            {
                parser_error(parser, "Out of memory");
                return NULL;
            }
            if (arms != NULL && arm_count > 0)
            {
                memcpy(new_arms, arms, sizeof(MatchArm) * arm_count);
            }
            arms = new_arms;
        }

        MatchArm *arm = &arms[arm_count];
        memset(arm, 0, sizeof(MatchArm));

        if (parser_match(parser, TOKEN_ELSE))
        {
            /* else arm */
            arm->is_else = true;
            arm->patterns = NULL;
            arm->pattern_count = 0;
        }
        else
        {
            /* Parse comma-separated pattern expressions */
            Expr **patterns = NULL;
            int pattern_count = 0;
            int pattern_capacity = 0;

            do
            {
                if (pattern_count >= pattern_capacity)
                {
                    pattern_capacity = pattern_capacity == 0 ? 4 : pattern_capacity * 2;
                    Expr **new_patterns = arena_alloc(parser->arena, sizeof(Expr *) * pattern_capacity);
                    if (new_patterns == NULL)
                    {
                        parser_error(parser, "Out of memory");
                        return NULL;
                    }
                    if (patterns != NULL && pattern_count > 0)
                    {
                        memcpy(new_patterns, patterns, sizeof(Expr *) * pattern_count);
                    }
                    patterns = new_patterns;
                }
                patterns[pattern_count++] = parser_expression(parser);
            } while (parser_match(parser, TOKEN_COMMA));

            arm->patterns = patterns;
            arm->pattern_count = pattern_count;
            arm->is_else = false;
        }

        /* Consume => before arm body */
        parser_consume(parser, TOKEN_ARROW, "Expected '=>' after match arm pattern");

        /* Parse arm body: indented block or single statement */
        if (parser_check(parser, TOKEN_NEWLINE))
        {
            while (parser_match(parser, TOKEN_NEWLINE)) {}
            if (parser_check(parser, TOKEN_INDENT))
            {
                arm->body = parser_indented_block(parser);
            }
            else
            {
                /* Single statement on next line at same indent level */
                Stmt *stmt = parser_statement(parser);
                Stmt **stmts = arena_alloc(parser->arena, sizeof(Stmt *));
                stmts[0] = stmt;
                arm->body = ast_create_block_stmt(parser->arena, stmts, 1, match_token);
            }
        }
        else
        {
            /* Single statement on same line */
            Stmt *stmt = parser_statement(parser);
            Stmt **stmts = arena_alloc(parser->arena, sizeof(Stmt *));
            stmts[0] = stmt;
            arm->body = ast_create_block_stmt(parser->arena, stmts, 1, match_token);
        }

        arm_count++;
    }

    /* Consume DEDENT */
    if (parser_check(parser, TOKEN_DEDENT))
    {
        parser_advance(parser);
    }

    return ast_create_match_expr(parser->arena, subject, arms, arm_count, match_token);
}
