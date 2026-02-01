#include "parser/expr/parser_expr_interpol.h"
#include "parser/util/parser_util.h"
#include "ast/ast_expr.h"
#include <stdlib.h>
#include <string.h>

Expr *parse_interpol_string(Parser *parser, Token *interpol_token)
{
    const char *content = interpol_token->literal.string_value;

    /* Handle error case where string literal content is NULL */
    if (content == NULL)
    {
        parser_error(parser, "Invalid interpolated string");
        return NULL;
    }

    Expr **parts = NULL;
    char **format_specs = NULL;
    int capacity = 0;
    int count = 0;

    /* Helper macro to add a string segment (no format spec for literals) */
    #define ADD_STRING_PART(str) do { \
        LiteralValue v; \
        v.string_value = str; \
        Expr *seg_expr = ast_create_literal_expr(parser->arena, v, \
            ast_create_primitive_type(parser->arena, TYPE_STRING), false, interpol_token); \
        if (count >= capacity) { \
            capacity = capacity == 0 ? 8 : capacity * 2; \
            Expr **new_parts = arena_alloc(parser->arena, sizeof(Expr *) * capacity); \
            char **new_formats = arena_alloc(parser->arena, sizeof(char *) * capacity); \
            if (new_parts == NULL || new_formats == NULL) exit(1); \
            if (parts != NULL && count > 0) { \
                memcpy(new_parts, parts, sizeof(Expr *) * count); \
                memcpy(new_formats, format_specs, sizeof(char *) * count); \
            } \
            parts = new_parts; \
            format_specs = new_formats; \
        } \
        parts[count] = seg_expr; \
        format_specs[count] = NULL; \
        count++; \
    } while(0)

    /* Helper macro to add an expression with optional format spec */
    #define ADD_EXPR_PART(expr, fmt) do { \
        if (count >= capacity) { \
            capacity = capacity == 0 ? 8 : capacity * 2; \
            Expr **new_parts = arena_alloc(parser->arena, sizeof(Expr *) * capacity); \
            char **new_formats = arena_alloc(parser->arena, sizeof(char *) * capacity); \
            if (new_parts == NULL || new_formats == NULL) exit(1); \
            if (parts != NULL && count > 0) { \
                memcpy(new_parts, parts, sizeof(Expr *) * count); \
                memcpy(new_formats, format_specs, sizeof(char *) * count); \
            } \
            parts = new_parts; \
            format_specs = new_formats; \
        } \
        parts[count] = expr; \
        format_specs[count] = fmt; \
        count++; \
    } while(0)

    const char *p = content;
    char *seg_buf = arena_alloc(parser->arena, strlen(content) + 1);
    int seg_len = 0;

    while (*p)
    {
        /* Handle {{ escape sequence -> literal { */
        if (*p == '{' && *(p + 1) == '{')
        {
            seg_buf[seg_len++] = '{';
            p += 2;
            continue;
        }
        /* Handle }} escape sequence -> literal } */
        if (*p == '}' && *(p + 1) == '}')
        {
            seg_buf[seg_len++] = '}';
            p += 2;
            continue;
        }
        /* Handle interpolation expression {expr} or {expr:format} */
        if (*p == '{')
        {
            /* Flush accumulated text segment */
            if (seg_len > 0)
            {
                seg_buf[seg_len] = '\0';
                char *seg = arena_strdup(parser->arena, seg_buf);
                ADD_STRING_PART(seg);
                seg_len = 0;
            }

            p++; /* skip { */
            const char *expr_start = p;
            const char *colon_pos = NULL;

            /* Use a state stack to track nesting */
            #define MAX_NESTING 64
            int state_stack[MAX_NESTING];
            int stack_top = 0;
            state_stack[stack_top] = 0;  /* Start in code mode */
            int paren_depth = 0;

            while (*p && stack_top >= 0)
            {
                int state = state_stack[stack_top];

                if (state == 1)  /* In regular string */
                {
                    if (*p == '\\' && *(p + 1))
                    {
                        p += 2;
                        continue;
                    }
                    if (*p == '"')
                    {
                        stack_top--;
                        p++;
                        continue;
                    }
                    p++;
                    continue;
                }

                if (state == 2)  /* In interpolated string text */
                {
                    if (*p == '\\' && *(p + 1))
                    {
                        p += 2;
                        continue;
                    }
                    if (*p == '"')
                    {
                        stack_top--;
                        p++;
                        continue;
                    }
                    if (*p == '{')
                    {
                        state_stack[stack_top] = 3;
                        p++;
                        continue;
                    }
                    p++;
                    continue;
                }

                /* state == 0 or 3: In code */
                if (*p == '$' && *(p + 1) == '"')
                {
                    if (stack_top < MAX_NESTING - 1)
                    {
                        stack_top++;
                        state_stack[stack_top] = 2;
                    }
                    p += 2;
                    continue;
                }
                if (*p == '"')
                {
                    if (stack_top < MAX_NESTING - 1)
                    {
                        stack_top++;
                        state_stack[stack_top] = 1;
                    }
                    p++;
                    continue;
                }
                if (*p == '(')
                {
                    if (stack_top == 0) paren_depth++;
                    p++;
                    continue;
                }
                if (*p == ')')
                {
                    if (stack_top == 0 && paren_depth > 0) paren_depth--;
                    p++;
                    continue;
                }
                if (*p == '{')
                {
                    if (stack_top < MAX_NESTING - 1)
                    {
                        stack_top++;
                        state_stack[stack_top] = state;
                    }
                    p++;
                    continue;
                }
                if (*p == '}')
                {
                    if (stack_top == 0)
                    {
                        break;
                    }
                    if (state_stack[stack_top] == 3)
                    {
                        state_stack[stack_top] = 2;
                    }
                    else
                    {
                        stack_top--;
                    }
                    p++;
                    continue;
                }
                if (*p == ':' && stack_top == 0 && paren_depth == 0 && colon_pos == NULL)
                {
                    colon_pos = p;
                }
                p++;
            }
            #undef MAX_NESTING

            if (!*p && stack_top > 0)
            {
                parser_error_at_current(parser, "Unterminated interpolated expression");
                LiteralValue zero = {0};
                return ast_create_literal_expr(parser->arena, zero,
                    ast_create_primitive_type(parser->arena, TYPE_STRING), false, NULL);
            }

            /* Extract expression and optional format specifier */
            char *expr_src;
            char *format_spec = NULL;

            if (colon_pos != NULL)
            {
                int expr_len = colon_pos - expr_start;
                expr_src = arena_strndup(parser->arena, expr_start, expr_len);

                int format_len = p - (colon_pos + 1);
                if (format_len > 0)
                {
                    format_spec = arena_strndup(parser->arena, colon_pos + 1, format_len);
                }
            }
            else
            {
                int expr_len = p - expr_start;
                expr_src = arena_strndup(parser->arena, expr_start, expr_len);
            }

            Lexer sub_lexer;
            lexer_init(parser->arena, &sub_lexer, expr_src, "interpolated");
            Parser sub_parser;
            parser_init(parser->arena, &sub_parser, &sub_lexer, parser->symbol_table);
            sub_parser.symbol_table = parser->symbol_table;

            Expr *inner = parser_expression(&sub_parser);
            if (inner == NULL || sub_parser.had_error)
            {
                parser_error_at_current(parser, "Invalid expression in interpolation");
                LiteralValue zero = {0};
                return ast_create_literal_expr(parser->arena, zero,
                    ast_create_primitive_type(parser->arena, TYPE_STRING), false, NULL);
            }

            ADD_EXPR_PART(inner, format_spec);

            if (parser->interp_count >= parser->interp_capacity)
            {
                parser->interp_capacity = parser->interp_capacity ? parser->interp_capacity * 2 : 8;
                char **new_interp_sources = arena_alloc(parser->arena,
                    sizeof(char *) * parser->interp_capacity);
                if (new_interp_sources == NULL) exit(1);
                if (parser->interp_sources != NULL && parser->interp_count > 0)
                    memcpy(new_interp_sources, parser->interp_sources,
                        sizeof(char *) * parser->interp_count);
                parser->interp_sources = new_interp_sources;
            }
            parser->interp_sources[parser->interp_count++] = expr_src;

            p++; /* skip } */
        }
        else
        {
            seg_buf[seg_len++] = *p++;
        }
    }

    /* Flush any remaining text segment */
    if (seg_len > 0)
    {
        seg_buf[seg_len] = '\0';
        char *seg = arena_strdup(parser->arena, seg_buf);
        ADD_STRING_PART(seg);
    }

    #undef ADD_STRING_PART
    #undef ADD_EXPR_PART

    return ast_create_interpolated_expr(parser->arena, parts, format_specs, count, interpol_token);
}
