#include "lexer/lexer_util.h"
#include "debug.h"
#include <stdio.h>
#include <string.h>

static char error_buffer[128];

void lexer_init(Arena *arena, Lexer *lexer, const char *source, const char *filename)
{
    lexer->start = source;
    lexer->current = source;
    lexer->line = 1;
    lexer->filename = filename;
    lexer->indent_capacity = 8;
    lexer->indent_stack = arena_alloc(arena, sizeof(int) * lexer->indent_capacity);
    lexer->indent_size = 1;
    lexer->indent_stack[0] = 0;
    lexer->at_line_start = 1;
    lexer->pending_indent = -1;
    lexer->pending_current = NULL;
    lexer->arena = arena;
}

void lexer_cleanup(Lexer *lexer)
{
    lexer->indent_stack = NULL;
}

void lexer_report_indentation_error(Lexer *lexer, int expected, int actual)
{
    snprintf(error_buffer, sizeof(error_buffer), "Indentation error: expected %d spaces, got %d spaces",
             expected, actual);
    lexer_error_token(lexer, error_buffer);
}

int lexer_is_at_end(Lexer *lexer)
{
    return *lexer->current == '\0';
}

char lexer_advance(Lexer *lexer)
{
    return *lexer->current++;
}

char lexer_peek(Lexer *lexer)
{
    return *lexer->current;
}

char lexer_peek_next(Lexer *lexer)
{
    if (lexer_is_at_end(lexer))
        return '\0';
    return lexer->current[1];
}

int lexer_match(Lexer *lexer, char expected)
{
    if (lexer_is_at_end(lexer))
        return 0;
    if (*lexer->current != expected)
        return 0;
    lexer->current++;
    return 1;
}

Token lexer_make_token(Lexer *lexer, SnTokenType type)
{
    int length = (int)(lexer->current - lexer->start);
    char *dup_start = arena_strndup(lexer->arena, lexer->start, length);
    if (dup_start == NULL)
    {
        DEBUG_ERROR("Out of memory duplicating lexeme");
        exit(1);
    }
    Token token;
    token_init(&token, type, dup_start, length, lexer->line, lexer->filename);
    return token;
}

Token lexer_error_token(Lexer *lexer, const char *message)
{
    char *dup_message = arena_strdup(lexer->arena, message);
    if (dup_message == NULL)
    {
        DEBUG_ERROR("Out of memory");
        exit(1);
    }
    Token token;
    token_init(&token, TOKEN_ERROR, dup_message, (int)strlen(dup_message), lexer->line, lexer->filename);
    return token;
}

void lexer_skip_whitespace(Lexer *lexer)
{
    for (;;)
    {
        char c = lexer_peek(lexer);
        switch (c)
        {
        case ' ':
        case '\t':
        case '\r':
            lexer_advance(lexer);
            break;
        case '\n':
            return;
        case '/':
            if (lexer_peek_next(lexer) == '/')
            {
                while (lexer_peek(lexer) != '\n' && !lexer_is_at_end(lexer))
                {
                    lexer_advance(lexer);
                }
            }
            else
            {
                return;
            }
            break;
        case '#':
            // Check if this is a pragma directive (not a comment)
            // lexer->current points to '#', check if next chars are "pragma"
            if (strncmp(lexer->current + 1, "pragma", 6) == 0)
            {
                // This is a pragma directive, don't skip it
                return;
            }
            // Single-line comment starting with #
            while (lexer_peek(lexer) != '\n' && !lexer_is_at_end(lexer))
            {
                lexer_advance(lexer);
            }
            break;
        default:
            return;
        }
    }
}
