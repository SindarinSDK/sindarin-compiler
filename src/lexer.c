#include "lexer.h"
#include "lexer/lexer_util.h"
#include "lexer/lexer_scan.h"
#include "debug.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>

static char error_buffer[128];

Token lexer_scan_token(Lexer *lexer)
{
    DEBUG_VERBOSE("Line %d: Starting lexer_scan_token, at_line_start = %d, pending_indent = %d",
                  lexer->line, lexer->at_line_start, lexer->pending_indent);
    if (lexer->at_line_start)
    {
        int current_indent;
        const char *temp;

        // Check if we have pending indent from previous multi-dedent
        if (lexer->pending_indent >= 0)
        {
            // Use saved values from previous DEDENT
            current_indent = lexer->pending_indent;
            temp = lexer->pending_current;
            DEBUG_VERBOSE("Line %d: Using pending indent = %d", lexer->line, current_indent);
        }
        else
        {
            // Calculate indent normally
            current_indent = 0;
            const char *indent_start = lexer->current;
            while (lexer_peek(lexer) == ' ' || lexer_peek(lexer) == '\t')
            {
                current_indent++;
                lexer_advance(lexer);
            }
            DEBUG_VERBOSE("Line %d: Calculated indent = %d", lexer->line, current_indent);
            temp = lexer->current;
            while (lexer_peek(lexer) == ' ' || lexer_peek(lexer) == '\t')
            {
                lexer_advance(lexer);
            }
            /* Check if this is a comment-only line (skip indent processing)
             * Note: #pragma is NOT a comment, so we must check for it specifically */
            int is_comment_line = 0;
            if (lexer_is_at_end(lexer) || lexer_peek(lexer) == '\n' || lexer_peek(lexer) == '\r' ||
                (lexer_peek(lexer) == '/' && lexer_peek_next(lexer) == '/'))
            {
                is_comment_line = 1;
            }
            else if (lexer_peek(lexer) == '#')
            {
                /* Only treat as comment if NOT followed by 'pragma' */
                /* lexer->current points at '#', so check current+1 for 'pragma' */
                if (strncmp(lexer->current + 1, "pragma", 6) != 0)
                {
                    is_comment_line = 1;
                }
            }
            if (is_comment_line)
            {
                DEBUG_VERBOSE("Line %d: Ignoring line (whitespace or comment only)", lexer->line);
                lexer->current = indent_start;
                lexer->start = indent_start;
                goto skip_indent_processing;
            }
        }

        lexer->current = temp;
        lexer->start = lexer->current;
        int top = lexer->indent_stack[lexer->indent_size - 1];
        DEBUG_VERBOSE("Line %d: Top of indent_stack = %d, indent_size = %d",
                      lexer->line, top, lexer->indent_size);
        if (current_indent > top)
        {
            if (lexer->indent_size >= lexer->indent_capacity)
            {
                lexer->indent_capacity *= 2;
                lexer->indent_stack = arena_alloc(lexer->arena,
                                                  lexer->indent_capacity * sizeof(int));
                if (lexer->indent_stack == NULL)
                {
                    DEBUG_ERROR("Out of memory");
                    exit(1);
                }
                DEBUG_VERBOSE("Line %d: Resized indent_stack, new capacity = %d",
                              lexer->line, lexer->indent_capacity);
            }
            lexer->indent_stack[lexer->indent_size++] = current_indent;
            lexer->at_line_start = 0;
            lexer->pending_indent = -1;
            lexer->pending_current = NULL;
            DEBUG_VERBOSE("Line %d: Pushing indent level %d, emitting INDENT",
                          lexer->line, current_indent);
            return lexer_make_token(lexer, TOKEN_INDENT);
        }
        else if (current_indent < top)
        {
            lexer->indent_size--;
            int new_top = lexer->indent_stack[lexer->indent_size - 1];
            DEBUG_VERBOSE("Line %d: Popped indent level, new top = %d, indent_size = %d",
                          lexer->line, new_top, lexer->indent_size);
            if (current_indent == new_top)
            {
                lexer->at_line_start = 0;
                lexer->pending_indent = -1;
                lexer->pending_current = NULL;
                DEBUG_VERBOSE("Line %d: Emitting DEDENT, indentation matches stack",
                              lexer->line);
            }
            else if (current_indent > new_top)
            {
                lexer->pending_indent = -1;
                lexer->pending_current = NULL;
                DEBUG_VERBOSE("Line %d: Error - Inconsistent indentation (current %d > new_top %d)",
                              lexer->line, current_indent, new_top);
                snprintf(error_buffer, sizeof(error_buffer), "Inconsistent indentation");
                return lexer_error_token(lexer, error_buffer);
            }
            else
            {
                // More DEDENTs pending - save current state
                lexer->pending_indent = current_indent;
                lexer->pending_current = temp;
                DEBUG_VERBOSE("Line %d: Emitting DEDENT, more dedents pending (saved indent=%d)",
                              lexer->line, current_indent);
            }
            return lexer_make_token(lexer, TOKEN_DEDENT);
        }
        else
        {
            lexer->at_line_start = 0;
            lexer->pending_indent = -1;
            lexer->pending_current = NULL;
            DEBUG_VERBOSE("Line %d: Indentation unchanged, proceeding to scan token",
                          lexer->line);
        }
    }
skip_indent_processing:
    DEBUG_VERBOSE("Line %d: Skipping whitespace within the line", lexer->line);
    lexer_skip_whitespace(lexer);
    lexer->start = lexer->current;

    if (!lexer_is_at_end(lexer) && lexer_peek(lexer) == '\n') {
        lexer_advance(lexer);
        lexer->line++;
        lexer->at_line_start = 1;
        DEBUG_VERBOSE("Line %d: Emitting NEWLINE after skip", lexer->line - 1);
        return lexer_make_token(lexer, TOKEN_NEWLINE);
    }

    if (lexer_is_at_end(lexer))
    {
        DEBUG_VERBOSE("Line %d: End of file reached", lexer->line);
        return lexer_make_token(lexer, TOKEN_EOF);
    }
    char c = lexer_advance(lexer);
    DEBUG_VERBOSE("Line %d: Scanning character '%c'", lexer->line, c);
    if (c == '\n')
    {
        lexer->line++;
        lexer->at_line_start = 1;
        DEBUG_VERBOSE("Line %d: Emitting NEWLINE", lexer->line - 1);
        return lexer_make_token(lexer, TOKEN_NEWLINE);
    }
    if (isalpha(c) || c == '_')
    {
        Token token = lexer_scan_identifier(lexer);
        DEBUG_VERBOSE("Line %d: Emitting identifier token type %d", lexer->line, token.type);
        return token;
    }
    if (isdigit(c))
    {
        Token token = lexer_scan_number(lexer);
        DEBUG_VERBOSE("Line %d: Emitting number token type %d", lexer->line, token.type);
        return token;
    }
    switch (c)
    {
    case '&':
        if (lexer_match(lexer, '&'))
        {
            DEBUG_VERBOSE("Line %d: Emitting AND", lexer->line);
            return lexer_make_token(lexer, TOKEN_AND);
        }
        DEBUG_VERBOSE("Line %d: Emitting AMPERSAND", lexer->line);
        return lexer_make_token(lexer, TOKEN_AMPERSAND);
    case '%':
        if (lexer_match(lexer, '='))
        {
            DEBUG_VERBOSE("Line %d: Emitting MODULO_EQUAL", lexer->line);
            return lexer_make_token(lexer, TOKEN_MODULO_EQUAL);
        }
        DEBUG_VERBOSE("Line %d: Emitting MODULO", lexer->line);
        return lexer_make_token(lexer, TOKEN_MODULO);
    case '/':
        if (lexer_match(lexer, '='))
        {
            DEBUG_VERBOSE("Line %d: Emitting SLASH_EQUAL", lexer->line);
            return lexer_make_token(lexer, TOKEN_SLASH_EQUAL);
        }
        DEBUG_VERBOSE("Line %d: Emitting SLASH", lexer->line);
        return lexer_make_token(lexer, TOKEN_SLASH);
    case '*':
        if (lexer_match(lexer, '='))
        {
            DEBUG_VERBOSE("Line %d: Emitting STAR_EQUAL", lexer->line);
            return lexer_make_token(lexer, TOKEN_STAR_EQUAL);
        }
        DEBUG_VERBOSE("Line %d: Emitting STAR", lexer->line);
        return lexer_make_token(lexer, TOKEN_STAR);
    case '+':
        if (lexer_match(lexer, '+'))
        {
            DEBUG_VERBOSE("Line %d: Emitting PLUS_PLUS", lexer->line);
            return lexer_make_token(lexer, TOKEN_PLUS_PLUS);
        }
        else if (lexer_match(lexer, '='))
        {
            DEBUG_VERBOSE("Line %d: Emitting PLUS_EQUAL", lexer->line);
            return lexer_make_token(lexer, TOKEN_PLUS_EQUAL);
        }
        DEBUG_VERBOSE("Line %d: Emitting PLUS", lexer->line);
        return lexer_make_token(lexer, TOKEN_PLUS);
    case '(':
        DEBUG_VERBOSE("Line %d: Emitting LEFT_PAREN", lexer->line);
        return lexer_make_token(lexer, TOKEN_LEFT_PAREN);
    case ')':
        DEBUG_VERBOSE("Line %d: Emitting RIGHT_PAREN", lexer->line);
        return lexer_make_token(lexer, TOKEN_RIGHT_PAREN);
    case ':':
        DEBUG_VERBOSE("Line %d: Emitting COLON", lexer->line);
        return lexer_make_token(lexer, TOKEN_COLON);
    case '-':
        if (lexer_match(lexer, '-'))
        {
            DEBUG_VERBOSE("Line %d: Emitting MINUS_MINUS", lexer->line);
            return lexer_make_token(lexer, TOKEN_MINUS_MINUS);
        }
        else if (lexer_match(lexer, '='))
        {
            DEBUG_VERBOSE("Line %d: Emitting MINUS_EQUAL", lexer->line);
            return lexer_make_token(lexer, TOKEN_MINUS_EQUAL);
        }
        else if (lexer_match(lexer, '>'))
        {
            DEBUG_VERBOSE("Line %d: Emitting ARROW", lexer->line);
            return lexer_make_token(lexer, TOKEN_ARROW);
        }
        DEBUG_VERBOSE("Line %d: Emitting MINUS", lexer->line);
        return lexer_make_token(lexer, TOKEN_MINUS);
    case '=':
        if (lexer_match(lexer, '='))
        {
            DEBUG_VERBOSE("Line %d: Emitting EQUAL_EQUAL", lexer->line);
            return lexer_make_token(lexer, TOKEN_EQUAL_EQUAL);
        }
        if (lexer_match(lexer, '>'))
        {
            DEBUG_VERBOSE("Line %d: Emitting ARROW (for '=>')", lexer->line);
            return lexer_make_token(lexer, TOKEN_ARROW);
        }
        DEBUG_VERBOSE("Line %d: Emitting EQUAL", lexer->line);
        return lexer_make_token(lexer, TOKEN_EQUAL);
    case '<':
        if (lexer_match(lexer, '='))
        {
            DEBUG_VERBOSE("Line %d: Emitting LESS_EQUAL", lexer->line);
            return lexer_make_token(lexer, TOKEN_LESS_EQUAL);
        }
        DEBUG_VERBOSE("Line %d: Emitting LESS", lexer->line);
        return lexer_make_token(lexer, TOKEN_LESS);
    case '>':
        if (lexer_match(lexer, '='))
        {
            DEBUG_VERBOSE("Line %d: Emitting GREATER_EQUAL", lexer->line);
            return lexer_make_token(lexer, TOKEN_GREATER_EQUAL);
        }
        DEBUG_VERBOSE("Line %d: Emitting GREATER", lexer->line);
        return lexer_make_token(lexer, TOKEN_GREATER);
    case ',':
        DEBUG_VERBOSE("Line %d: Emitting COMMA", lexer->line);
        return lexer_make_token(lexer, TOKEN_COMMA);
    case ';':
        DEBUG_VERBOSE("Line %d: Emitting SEMICOLON", lexer->line);
        return lexer_make_token(lexer, TOKEN_SEMICOLON);
    case '.':
        if (lexer_match(lexer, '.'))
        {
            // Check for third dot (spread operator)
            if (lexer_match(lexer, '.'))
            {
                DEBUG_VERBOSE("Line %d: Emitting SPREAD", lexer->line);
                return lexer_make_token(lexer, TOKEN_SPREAD);
            }
            DEBUG_VERBOSE("Line %d: Emitting RANGE", lexer->line);
            return lexer_make_token(lexer, TOKEN_RANGE);
        }
        DEBUG_VERBOSE("Line %d: Emitting DOT", lexer->line);
        return lexer_make_token(lexer, TOKEN_DOT);
    case '[':
        DEBUG_VERBOSE("Line %d: Emitting LEFT_BRACKET", lexer->line);
        return lexer_make_token(lexer, TOKEN_LEFT_BRACKET);
    case ']':
        DEBUG_VERBOSE("Line %d: Emitting RIGHT_BRACKET", lexer->line);
        return lexer_make_token(lexer, TOKEN_RIGHT_BRACKET);
    case '{':
        DEBUG_VERBOSE("Line %d: Emitting LEFT_BRACE", lexer->line);
        return lexer_make_token(lexer, TOKEN_LEFT_BRACE);
    case '}':
        DEBUG_VERBOSE("Line %d: Emitting RIGHT_BRACE", lexer->line);
        return lexer_make_token(lexer, TOKEN_RIGHT_BRACE);
    case '"':
    {
        Token string_token = lexer_scan_string(lexer);
        DEBUG_VERBOSE("Line %d: Emitting STRING_LITERAL", lexer->line);
        return string_token;
    }
    case '\'':
    {
        Token char_token = lexer_scan_char(lexer);
        DEBUG_VERBOSE("Line %d: Emitting CHAR_LITERAL", lexer->line);
        return char_token;
    }
    case '|':
        if (lexer_match(lexer, '|'))
        {
            DEBUG_VERBOSE("Line %d: Emitting OR", lexer->line);
            return lexer_make_token(lexer, TOKEN_OR);
        }
        /* Check for pipe block string: | followed by optional whitespace then newline */
        {
            const char *check = lexer->current;
            while (*check == ' ' || *check == '\t') check++;
            if (*check == '\n' || *check == '\r' || *check == '\0')
            {
                /* This is a pipe block string */
                return lexer_scan_pipe_string(lexer, 0);
            }
        }
        DEBUG_VERBOSE("Line %d: Emitting OR (single)", lexer->line);
        return lexer_make_token(lexer, TOKEN_OR);
    case '!':
        if (lexer_match(lexer, '='))
        {
            DEBUG_VERBOSE("Line %d: Emitting BANG_EQUAL", lexer->line);
            return lexer_make_token(lexer, TOKEN_BANG_EQUAL);
        }
        DEBUG_VERBOSE("Line %d: Emitting BANG", lexer->line);
        return lexer_make_token(lexer, TOKEN_BANG);
    case '$':
        if (lexer_peek(lexer) == '"')
        {
            lexer_advance(lexer);
            Token token = lexer_scan_string(lexer);
            token.type = TOKEN_INTERPOL_STRING;
            DEBUG_VERBOSE("Line %d: Emitting INTERPOL_STRING", lexer->line);
            return token;
        }
        /* Check for interpolated pipe block string: $| followed by optional whitespace then newline */
        if (lexer_peek(lexer) == '|')
        {
            lexer_advance(lexer); /* consume '|' */
            const char *check = lexer->current;
            while (*check == ' ' || *check == '\t') check++;
            if (*check == '\n' || *check == '\r' || *check == '\0')
            {
                /* This is an interpolated pipe block string */
                return lexer_scan_pipe_string(lexer, 1);
            }
            /* Not a pipe block string, back up and treat '$' as error */
            lexer->current--;
        }
        /* intentional fallthrough */
    case '#':
        // Check for #pragma directive
        if (strncmp(lexer->current, "pragma", 6) == 0)
        {
            lexer->current += 6; // Skip "pragma"
            // Skip whitespace
            while (*lexer->current == ' ' || *lexer->current == '\t')
            {
                lexer->current++;
            }
            // Mark that we're not at line start anymore
            lexer->at_line_start = 0;
            // Check for "include" or "link"
            if (strncmp(lexer->current, "include", 7) == 0)
            {
                lexer->current += 7;
                DEBUG_VERBOSE("Line %d: Emitting PRAGMA_INCLUDE", lexer->line);
                return lexer_make_token(lexer, TOKEN_PRAGMA_INCLUDE);
            }
            else if (strncmp(lexer->current, "link", 4) == 0)
            {
                lexer->current += 4;
                DEBUG_VERBOSE("Line %d: Emitting PRAGMA_LINK", lexer->line);
                return lexer_make_token(lexer, TOKEN_PRAGMA_LINK);
            }
            else if (strncmp(lexer->current, "source", 6) == 0)
            {
                lexer->current += 6;
                DEBUG_VERBOSE("Line %d: Emitting PRAGMA_SOURCE", lexer->line);
                return lexer_make_token(lexer, TOKEN_PRAGMA_SOURCE);
            }
            else if (strncmp(lexer->current, "pack", 4) == 0)
            {
                lexer->current += 4;
                DEBUG_VERBOSE("Line %d: Emitting PRAGMA_PACK", lexer->line);
                return lexer_make_token(lexer, TOKEN_PRAGMA_PACK);
            }
            else if (strncmp(lexer->current, "alias", 5) == 0)
            {
                lexer->current += 5;
                DEBUG_VERBOSE("Line %d: Emitting PRAGMA_ALIAS", lexer->line);
                return lexer_make_token(lexer, TOKEN_PRAGMA_ALIAS);
            }
            else
            {
                snprintf(error_buffer, sizeof(error_buffer), "Unknown pragma directive");
                return lexer_error_token(lexer, error_buffer);
            }
        }
        /* '#' without 'pragma' following is an error */
        snprintf(error_buffer, sizeof(error_buffer), "Unexpected character '%c'", c);
        DEBUG_VERBOSE("Line %d: Error - %s", lexer->line, error_buffer);
        return lexer_error_token(lexer, error_buffer);
    default:
        snprintf(error_buffer, sizeof(error_buffer), "Unexpected character '%c'", c);
        DEBUG_VERBOSE("Line %d: Error - %s", lexer->line, error_buffer);
        return lexer_error_token(lexer, error_buffer);
    }
    if (lexer_is_at_end(lexer))
    {
        while (lexer->indent_size > 1)
        {
            lexer->indent_size--;
            return lexer_make_token(lexer, TOKEN_DEDENT);
        }
        return lexer_make_token(lexer, TOKEN_EOF);
    }
}
