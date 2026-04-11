/* ============================================================================
 * formatter.c — Sindarin source code formatter
 *
 * Formats .sn source files with consistent style:
 *   - 2-space indentation
 *   - Consistent operator/punctuation spacing
 *   - Multi-line struct initializers for 2+ fields
 *   - Trailing whitespace removal
 *   - Blank line normalization (max 1 consecutive)
 *   - Comment normalization (# and //)
 *
 * Works line-by-line with a per-line tokenizer. Does NOT use the compiler's
 * lexer/parser — operates purely on source text so comments are preserved.
 * ============================================================================ */

#include "formatter.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>
#include <limits.h>

#ifndef _WIN32
#include <dirent.h>
#else
#include <windows.h>
#endif

/* ============================================================================
 * Section 1: Dynamic buffer
 * ============================================================================ */

typedef struct
{
    char *data;
    int len;
    int cap;
} Buf;

static void buf_init(Buf *b)
{
    b->cap = 4096;
    b->len = 0;
    b->data = malloc(b->cap);
}

static void buf_grow(Buf *b, int needed)
{
    if (b->len + needed <= b->cap) return;
    while (b->cap < b->len + needed) b->cap *= 2;
    b->data = realloc(b->data, b->cap);
}

static void buf_char(Buf *b, char c)
{
    buf_grow(b, 1);
    b->data[b->len++] = c;
}

static void buf_str(Buf *b, const char *s, int n)
{
    if (n <= 0) return;
    buf_grow(b, n);
    memcpy(b->data + b->len, s, n);
    b->len += n;
}

static void buf_cstr(Buf *b, const char *s)
{
    buf_str(b, s, (int)strlen(s));
}

static void buf_indent(Buf *b, int level)
{
    for (int i = 0; i < level * 2; i++)
        buf_char(b, ' ');
}

static void buf_null(Buf *b)
{
    buf_grow(b, 1);
    b->data[b->len] = '\0';
}

/* Emit text from start up to end, stripping trailing whitespace. */
static void buf_trimmed(Buf *b, const char *start, const char *end)
{
    while (end > start && (*(end - 1) == ' ' || *(end - 1) == '\t'))
        end--;
    if (end > start)
        buf_str(b, start, (int)(end - start));
}

/* ============================================================================
 * Section 2: Line tokenizer
 *
 * A lightweight tokenizer that works on a single line of source code.
 * It recognizes just enough to apply spacing rules and detect struct inits.
 * Unlike the compiler's lexer, it preserves # comments and doesn't produce
 * INDENT/DEDENT tokens.
 * ============================================================================ */

#define MAX_LINE_TOKENS 512

typedef enum
{
    FT_WORD,             /* identifier or keyword */
    FT_NUMBER,           /* numeric literal */
    FT_STRING,           /* "..." or $"..." including delimiters */
    FT_CHAR_LIT,         /* '.' including delimiters */
    FT_PRAGMA,           /* @alias, @source, etc. */
    FT_COMMENT,          /* // ... rest of line */
    FT_BINOP,            /* binary operator: + - * / % == != < <= > >= && || & | ^ << >> */
    FT_UNARY,            /* unary operator: ! ~ (and unary - +) */
    FT_INC_DEC,          /* ++ or -- */
    FT_EQUAL,            /* = */
    FT_COMPOUND_ASSIGN,  /* += -= *= /= %= &= |= ^= <<= >>= */
    FT_ARROW,            /* => */
    FT_COLON,            /* : */
    FT_COMMA,            /* , */
    FT_SEMICOLON,        /* ; */
    FT_DOT,              /* . */
    FT_RANGE,            /* .. */
    FT_SPREAD,           /* ... */
    FT_OPEN_PAREN,       /* ( */
    FT_CLOSE_PAREN,      /* ) */
    FT_OPEN_BRACE,       /* { */
    FT_CLOSE_BRACE,      /* } */
    FT_OPEN_BRACKET,     /* [ */
    FT_CLOSE_BRACKET,    /* ] */
    FT_ANGLE_OPEN,       /* < when part of a generic parameter list (reclassified) */
    FT_ANGLE_CLOSE,      /* > when part of a generic parameter list (reclassified) */
    FT_END,
} FTType;

typedef struct
{
    FTType type;
    const char *text;
    int len;
} FT;

/* Is this the end of the line content? */
static int is_eol(char c) { return c == '\0' || c == '\n' || c == '\r'; }

/* Should minus/plus be classified as unary based on the previous token? */
static int is_unary_context(FT *prev)
{
    if (!prev) return 1;
    FTType p = prev->type;
    return p == FT_EQUAL || p == FT_ARROW || p == FT_OPEN_PAREN ||
           p == FT_OPEN_BRACKET || p == FT_OPEN_BRACE || p == FT_COMMA ||
           p == FT_COLON || p == FT_BINOP || p == FT_COMPOUND_ASSIGN ||
           p == FT_UNARY || p == FT_SEMICOLON;
}

/* Tokenize a single line of code (content after leading whitespace, up to EOL).
 * Returns the number of tokens. Tokens are written to the provided array. */
static int tokenize_line(const char *p, const char *line_end, FT *tokens)
{
    int count = 0;
    FT *prev = NULL;

    while (p < line_end && !is_eol(*p) && count < MAX_LINE_TOKENS - 1)
    {
        /* Skip whitespace within line */
        while (p < line_end && (*p == ' ' || *p == '\t')) p++;
        if (p >= line_end || is_eol(*p)) break;

        FT *tok = &tokens[count];
        tok->text = p;

        /* ---- String literal ---- */
        if (*p == '"')
        {
            p++;
            while (p < line_end && *p != '"' && !is_eol(*p))
            {
                if (*p == '\\' && p + 1 < line_end) p++;
                p++;
            }
            if (p < line_end && *p == '"') p++;
            tok->type = FT_STRING;
        }
        /* ---- Interpolated string or interpolated pipe ---- */
        else if (*p == '$' && p + 1 < line_end && (*(p + 1) == '"' || *(p + 1) == '|'))
        {
            if (*(p + 1) == '|')
            {
                /* $| pipe block opener — consume $| as a single token */
                p += 2;
                tok->type = FT_STRING;
            }
            else
            {
                p += 2;
                int depth = 0;
                while (p < line_end && !is_eol(*p) && !(*p == '"' && depth == 0))
                {
                    if (*p == '{') depth++;
                    else if (*p == '}') { if (depth > 0) depth--; }
                    else if (*p == '\\' && p + 1 < line_end) { p++; p++; continue; }
                    p++;
                }
                if (p < line_end && *p == '"') p++;
                tok->type = FT_STRING;
            }
        }
        /* ---- Char literal ---- */
        else if (*p == '\'')
        {
            p++;
            if (p < line_end && *p == '\\' && p + 1 < line_end) p++;
            if (p < line_end && !is_eol(*p)) p++;
            if (p < line_end && *p == '\'') p++;
            tok->type = FT_CHAR_LIT;
        }
        /* ---- Identifier / keyword ---- */
        else if (isalpha((unsigned char)*p) || *p == '_')
        {
            while (p < line_end && (isalnum((unsigned char)*p) || *p == '_')) p++;
            tok->type = FT_WORD;
        }
        /* ---- Number ---- */
        else if (isdigit((unsigned char)*p))
        {
            if (*p == '0' && p + 1 < line_end && (*(p + 1) == 'x' || *(p + 1) == 'X'))
            {
                p += 2;
                while (p < line_end && (isxdigit((unsigned char)*p) || *p == '_')) p++;
            }
            else if (*p == '0' && p + 1 < line_end && (*(p + 1) == 'b' || *(p + 1) == 'B'))
            {
                p += 2;
                while (p < line_end && (*p == '0' || *p == '1' || *p == '_')) p++;
            }
            else if (*p == '0' && p + 1 < line_end && (*(p + 1) == 'o' || *(p + 1) == 'O'))
            {
                p += 2;
                while (p < line_end && ((*p >= '0' && *p <= '7') || *p == '_')) p++;
            }
            else
            {
                while (p < line_end && (isdigit((unsigned char)*p) || *p == '_')) p++;
                if (p < line_end && *p == '.' && p + 1 < line_end && isdigit((unsigned char)*(p + 1)))
                {
                    p++;
                    while (p < line_end && (isdigit((unsigned char)*p) || *p == '_')) p++;
                }
            }
            /* Optional suffix — consume alpha+digit sequences like u32, i32, UL, f */
            while (p < line_end && (isalpha((unsigned char)*p) || isdigit((unsigned char)*p))) p++;
            tok->type = FT_NUMBER;
        }
        /* ---- @ pragma ---- */
        else if (*p == '@')
        {
            p++;
            while (p < line_end && (isalpha((unsigned char)*p) || *p == '_')) p++;
            tok->type = FT_PRAGMA;
        }
        /* ---- // comment ---- */
        else if (*p == '/' && p + 1 < line_end && *(p + 1) == '/')
        {
            while (p < line_end && !is_eol(*p)) p++;
            tok->type = FT_COMMENT;
        }
        /* ---- Operators and punctuation ---- */
        else if (*p == '=' && p + 1 < line_end && *(p + 1) == '=')
            { p += 2; tok->type = FT_BINOP; }
        else if (*p == '=' && p + 1 < line_end && *(p + 1) == '>')
            { p += 2; tok->type = FT_ARROW; }
        else if (*p == '=')
            { p++; tok->type = FT_EQUAL; }
        else if (*p == '!' && p + 1 < line_end && *(p + 1) == '=')
            { p += 2; tok->type = FT_BINOP; }
        else if (*p == '!')
            { p++; tok->type = FT_UNARY; }
        else if (*p == '<' && p + 2 < line_end && *(p + 1) == '<' && *(p + 2) == '=')
            { p += 3; tok->type = FT_COMPOUND_ASSIGN; }
        else if (*p == '<' && p + 1 < line_end && *(p + 1) == '<')
            { p += 2; tok->type = FT_BINOP; }
        else if (*p == '<' && p + 1 < line_end && *(p + 1) == '=')
            { p += 2; tok->type = FT_BINOP; }
        else if (*p == '<')
            { p++; tok->type = FT_BINOP; }
        else if (*p == '>' && p + 2 < line_end && *(p + 1) == '>' && *(p + 2) == '=')
            { p += 3; tok->type = FT_COMPOUND_ASSIGN; }
        else if (*p == '>' && p + 1 < line_end && *(p + 1) == '>')
            { p += 2; tok->type = FT_BINOP; }
        else if (*p == '>' && p + 1 < line_end && *(p + 1) == '=')
            { p += 2; tok->type = FT_BINOP; }
        else if (*p == '>')
            { p++; tok->type = FT_BINOP; }
        else if (*p == '+' && p + 1 < line_end && *(p + 1) == '+')
            { p += 2; tok->type = FT_INC_DEC; }
        else if (*p == '+' && p + 1 < line_end && *(p + 1) == '=')
            { p += 2; tok->type = FT_COMPOUND_ASSIGN; }
        else if (*p == '+')
            { p++; tok->type = is_unary_context(prev) ? FT_UNARY : FT_BINOP; }
        else if (*p == '-' && p + 1 < line_end && *(p + 1) == '-')
            { p += 2; tok->type = FT_INC_DEC; }
        else if (*p == '-' && p + 1 < line_end && *(p + 1) == '=')
            { p += 2; tok->type = FT_COMPOUND_ASSIGN; }
        else if (*p == '-')
            { p++; tok->type = is_unary_context(prev) ? FT_UNARY : FT_BINOP; }
        else if (*p == '*' && p + 1 < line_end && *(p + 1) == '=')
            { p += 2; tok->type = FT_COMPOUND_ASSIGN; }
        else if (*p == '*')
        {
            p++;
            /* * after : or ( or , is pointer type, not multiplication */
            tok->type = (prev && (prev->type == FT_COLON || prev->type == FT_OPEN_PAREN ||
                         prev->type == FT_COMMA)) ? FT_UNARY : FT_BINOP;
        }
        else if (*p == '/' && p + 1 < line_end && *(p + 1) == '=')
            { p += 2; tok->type = FT_COMPOUND_ASSIGN; }
        else if (*p == '/')
            { p++; tok->type = FT_BINOP; }
        else if (*p == '%' && p + 1 < line_end && *(p + 1) == '=')
            { p += 2; tok->type = FT_COMPOUND_ASSIGN; }
        else if (*p == '%')
            { p++; tok->type = FT_BINOP; }
        else if (*p == '&' && p + 1 < line_end && *(p + 1) == '&')
            { p += 2; tok->type = FT_BINOP; }
        else if (*p == '&' && p + 1 < line_end && *(p + 1) == '=')
            { p += 2; tok->type = FT_COMPOUND_ASSIGN; }
        else if (*p == '&')
            { p++; tok->type = FT_BINOP; }
        else if (*p == '|' && p + 1 < line_end && *(p + 1) == '|')
            { p += 2; tok->type = FT_BINOP; }
        else if (*p == '|' && p + 1 < line_end && *(p + 1) == '=')
            { p += 2; tok->type = FT_COMPOUND_ASSIGN; }
        else if (*p == '|')
            { p++; tok->type = FT_BINOP; }
        else if (*p == '^' && p + 1 < line_end && *(p + 1) == '=')
            { p += 2; tok->type = FT_COMPOUND_ASSIGN; }
        else if (*p == '^')
            { p++; tok->type = FT_BINOP; }
        else if (*p == '~')
            { p++; tok->type = FT_UNARY; }
        else if (*p == '.' && p + 2 < line_end && *(p + 1) == '.' && *(p + 2) == '.')
            { p += 3; tok->type = FT_SPREAD; }
        else if (*p == '.' && p + 1 < line_end && *(p + 1) == '.')
            { p += 2; tok->type = FT_RANGE; }
        else if (*p == '.')
            { p++; tok->type = FT_DOT; }
        else if (*p == '(') { p++; tok->type = FT_OPEN_PAREN; }
        else if (*p == ')') { p++; tok->type = FT_CLOSE_PAREN; }
        else if (*p == '{') { p++; tok->type = FT_OPEN_BRACE; }
        else if (*p == '}') { p++; tok->type = FT_CLOSE_BRACE; }
        else if (*p == '[') { p++; tok->type = FT_OPEN_BRACKET; }
        else if (*p == ']') { p++; tok->type = FT_CLOSE_BRACKET; }
        else if (*p == ':') { p++; tok->type = FT_COLON; }
        else if (*p == ',') { p++; tok->type = FT_COMMA; }
        else if (*p == ';') { p++; tok->type = FT_SEMICOLON; }
        else
        {
            /* Unknown character — preserve as word-like token */
            p++;
            tok->type = FT_WORD;
        }

        tok->len = (int)(p - tok->text);
        prev = tok;
        count++;
    }

    tokens[count].type = FT_END;
    tokens[count].text = p;
    tokens[count].len = 0;
    return count;
}

/* Is this FT_BINOP token the single character `<`? */
static int is_single_lt(FT *t)
{
    return t->type == FT_BINOP && t->len == 1 && t->text[0] == '<';
}

/* Is this FT_BINOP token the single character `>`? */
static int is_single_gt(FT *t)
{
    return t->type == FT_BINOP && t->len == 1 && t->text[0] == '>';
}

/* Is this FT_BINOP token the fused `>>`? (right-shift or nested generic close) */
static int is_fused_gt_gt(FT *t)
{
    return t->type == FT_BINOP && t->len == 2 &&
           t->text[0] == '>' && t->text[1] == '>';
}

/* Inside a generic argument list, only these token types are permitted. */
static int allowed_inside_generic(FTType t)
{
    return t == FT_WORD || t == FT_COMMA ||
           t == FT_OPEN_BRACKET || t == FT_CLOSE_BRACKET;
}

/* After a closing `>` of a generic, the next token must not be an operand or
 * anything that would naturally follow a comparison result. This is how we
 * distinguish `Stack<int> = ...` (generic) from `i < length || ...` (compare). */
static int allowed_after_generic_close(FTType t)
{
    switch (t)
    {
    case FT_OPEN_PAREN:
    case FT_OPEN_BRACE:
    case FT_OPEN_BRACKET:
    case FT_CLOSE_PAREN:
    case FT_CLOSE_BRACE:
    case FT_CLOSE_BRACKET:
    case FT_EQUAL:
    case FT_ARROW:
    case FT_COLON:
    case FT_COMMA:
    case FT_SEMICOLON:
    case FT_COMPOUND_ASSIGN:
    case FT_COMMENT:
    case FT_END:
        return 1;
    default:
        return 0;
    }
}

/* Reclassify single-char `<` `>` tokens that form a generic parameter list
 * from FT_BINOP to FT_ANGLE_OPEN / FT_ANGLE_CLOSE. Handles nested generics
 * via a stack, and splits fused `>>` tokens (lexer output for right-shift)
 * into two single-`>` tokens when they appear as nested generic closes. */
static void classify_generics(FT *tokens, int *count)
{
    for (int i = 0; i < *count; i++)
    {
        if (!is_single_lt(&tokens[i])) continue;
        if (i == 0 || tokens[i - 1].type != FT_WORD) continue;

        /* Walk forward, tracking nested angle depth. Record every pair so we
         * can reclassify them all atomically once the outer match closes.
         * `pair_close_half` is 1 for the second `>` of a fused `>>`, 0 otherwise.
         * Fused-close token positions get split in place after classification. */
        int open_stack[64];
        int sp = 0;
        int pair_open[64];
        int pair_close[64];
        int pair_close_half[64];
        int num_pairs = 0;
        int outer_close = -1;
        int outer_fused_half = 0;
        int ok = 1;

        open_stack[sp++] = i;
        int j = i + 1;
        while (j < *count && sp > 0)
        {
            FT *t = &tokens[j];
            if (is_single_lt(t))
            {
                if (sp >= 64) { ok = 0; break; }
                open_stack[sp++] = j;
            }
            else if (is_single_gt(t))
            {
                if (num_pairs >= 64) { ok = 0; break; }
                sp--;
                pair_open[num_pairs] = open_stack[sp];
                pair_close[num_pairs] = j;
                pair_close_half[num_pairs] = 0;
                num_pairs++;
                if (sp == 0) { outer_close = j; outer_fused_half = 0; break; }
            }
            else if (is_fused_gt_gt(t))
            {
                /* `>>` closes two generic levels at once. */
                if (sp < 2 || num_pairs + 2 > 64) { ok = 0; break; }
                sp--;
                pair_open[num_pairs] = open_stack[sp];
                pair_close[num_pairs] = j;
                pair_close_half[num_pairs] = 0;
                num_pairs++;

                sp--;
                pair_open[num_pairs] = open_stack[sp];
                pair_close[num_pairs] = j;
                pair_close_half[num_pairs] = 1;
                num_pairs++;

                if (sp == 0) { outer_close = j; outer_fused_half = 1; break; }
            }
            else if (!allowed_inside_generic(t->type))
            {
                ok = 0;
                break;
            }
            j++;
        }

        if (!ok || outer_close < 0) continue;

        /* Check the token that follows the entire classified region. If the
         * outer close sits in a fused `>>`, the following token is still the
         * one originally at outer_close + 1 — splitting doesn't change what's
         * beyond the fused position. */
        FTType next = tokens[outer_close + 1].type;  /* FT_END sentinel is safe */
        if (!allowed_after_generic_close(next)) continue;

        /* Collect the unique fused-close positions, process them right-to-left,
         * and split each `>>` into two single-`>` BINOP tokens. Processing
         * right-to-left keeps earlier indices valid; we still patch the
         * recorded pair_open / pair_close entries that live to the right of
         * each split so they remain accurate. */
        int fused_positions[64];
        int num_fused = 0;
        for (int k = 0; k < num_pairs; k++)
        {
            if (pair_close_half[k] != 1) continue;
            int dup = 0;
            for (int m = 0; m < num_fused; m++)
                if (fused_positions[m] == pair_close[k]) { dup = 1; break; }
            if (!dup) fused_positions[num_fused++] = pair_close[k];
        }
        /* Insertion sort descending. */
        for (int a = 1; a < num_fused; a++)
        {
            int key = fused_positions[a];
            int b = a - 1;
            while (b >= 0 && fused_positions[b] < key)
            {
                fused_positions[b + 1] = fused_positions[b];
                b--;
            }
            fused_positions[b + 1] = key;
        }

        int split_outer = 0;
        for (int a = 0; a < num_fused; a++)
        {
            int fp = fused_positions[a];
            if (*count + 1 > MAX_LINE_TOKENS) { ok = 0; break; }
            /* Shift tokens[fp+1 .. *count-1] right by one slot (and carry the
             * FT_END sentinel if it sits just past *count). */
            int last = *count;
            for (int k = last; k > fp; k--)
                tokens[k + 1] = tokens[k];
            /* Split the `>>` at fp. tokens[fp+1] is a fresh copy after the
             * shift — retarget it at the second `>` character. */
            tokens[fp].len = 1;
            tokens[fp + 1] = tokens[fp];
            tokens[fp + 1].text = tokens[fp].text + 1;
            (*count)++;

            /* Fix up recorded indices for everything strictly to the right
             * of fp (opens and closes of other pairs). */
            for (int k = 0; k < num_pairs; k++)
            {
                if (pair_open[k] > fp) pair_open[k]++;
                if (pair_close[k] > fp) pair_close[k]++;
            }
            /* Any second-half close that sat at fp now lives at fp+1. */
            for (int k = 0; k < num_pairs; k++)
            {
                if (pair_close[k] == fp && pair_close_half[k] == 1)
                    pair_close[k] = fp + 1;
            }
            if (outer_close == fp && outer_fused_half == 1)
            {
                outer_close = fp + 1;
                split_outer = 1;
            }
            else if (outer_close > fp)
            {
                outer_close++;
            }
        }
        (void)split_outer;

        if (!ok) continue;

        /* All checks passed — reclassify every recorded pair. */
        for (int k = 0; k < num_pairs; k++)
        {
            tokens[pair_open[k]].type = FT_ANGLE_OPEN;
            tokens[pair_close[k]].type = FT_ANGLE_CLOSE;
        }

        /* Skip past the outer close to avoid re-scanning already-classified pairs. */
        i = outer_close;
    }
}

/* ============================================================================
 * Section 3: Spacing rules
 * ============================================================================ */

static int is_word_like(FTType t)
{
    return t == FT_WORD || t == FT_NUMBER || t == FT_STRING || t == FT_CHAR_LIT;
}

static int is_control_keyword(FT *tok)
{
    if (tok->type != FT_WORD) return 0;
    int n = tok->len;
    const char *w = tok->text;
    if (n == 2 && w[0] == 'i' && w[1] == 'f') return 1;
    if (n == 4 && strncmp(w, "else", 4) == 0) return 1;
    if (n == 5 && strncmp(w, "while", 5) == 0) return 1;
    if (n == 3 && strncmp(w, "for", 3) == 0) return 1;
    if (n == 5 && strncmp(w, "match", 5) == 0) return 1;
    if (n == 6 && strncmp(w, "return", 6) == 0) return 1;
    if (n == 4 && strncmp(w, "lock", 4) == 0) return 1;
    if (n == 5 && strncmp(w, "using", 5) == 0) return 1;
    return 0;
}

/* Determine whether a space is needed between two consecutive tokens. */
static int needs_space(FT *prev, FT *cur)
{
    FTType p = prev->type;
    FTType c = cur->type;

    /* No space after opening delimiters */
    if (p == FT_OPEN_PAREN || p == FT_OPEN_BRACKET || p == FT_ANGLE_OPEN) return 0;

    /* No space before closing delimiters */
    if (c == FT_CLOSE_PAREN || c == FT_CLOSE_BRACKET || c == FT_ANGLE_CLOSE) return 0;

    /* No space between a type/function name and the `<` that opens its generic
     * argument list, or between a generic close and a following `(` / `[`. */
    if (p == FT_WORD && c == FT_ANGLE_OPEN) return 0;
    if (p == FT_ANGLE_CLOSE && (c == FT_OPEN_PAREN || c == FT_OPEN_BRACKET)) return 0;

    /* No space around . */
    if (p == FT_DOT || c == FT_DOT) return 0;

    /* No space around .. */
    if (p == FT_RANGE || c == FT_RANGE) return 0;

    /* No space after ... */
    if (p == FT_SPREAD) return 0;

    /* No space before , : ; */
    if (c == FT_COMMA || c == FT_COLON || c == FT_SEMICOLON) return 0;

    /* Space after , and : */
    if (p == FT_COMMA || p == FT_COLON) return 1;

    /* Space around = */
    if (p == FT_EQUAL || c == FT_EQUAL) return 1;

    /* Space around => */
    if (p == FT_ARROW || c == FT_ARROW) return 1;

    /* Space around binary operators */
    if (p == FT_BINOP || c == FT_BINOP) return 1;

    /* Space around compound assignment */
    if (p == FT_COMPOUND_ASSIGN || c == FT_COMPOUND_ASSIGN) return 1;

    /* No space after unary operators */
    if (p == FT_UNARY) return 0;

    /* No space before ++ -- */
    if (c == FT_INC_DEC) return 0;

    /* Space before { */
    if (c == FT_OPEN_BRACE) return 1;

    /* Space after } */
    if (p == FT_CLOSE_BRACE) return 1;

    /* No space between word and ( for function calls, but space after control keywords */
    if (p == FT_WORD && c == FT_OPEN_PAREN)
        return is_control_keyword(prev);

    /* No space between word/]/)/} and [ (array access, type annotation) */
    if ((p == FT_WORD || p == FT_CLOSE_BRACKET || p == FT_CLOSE_PAREN ||
         p == FT_CLOSE_BRACE) && c == FT_OPEN_BRACKET)
        return 0;

    /* Space between word-like tokens */
    if (is_word_like(p) && is_word_like(c)) return 1;

    /* Space after ) before word-like tokens */
    if (p == FT_CLOSE_PAREN && is_word_like(c)) return 1;

    /* Space after pragma before its arguments */
    if (p == FT_PRAGMA) return 1;

    /* Default: space */
    return 1;
}

/* ============================================================================
 * Section 4: Struct init analysis helpers
 * ============================================================================ */

/* Find the index of the matching } for the { at open_idx.
 * Returns -1 if not found within [open_idx+1, count). */
static int find_close_brace(FT *tokens, int count, int open_idx)
{
    int depth = 1;
    for (int i = open_idx + 1; i < count; i++)
    {
        if (tokens[i].type == FT_OPEN_BRACE) depth++;
        else if (tokens[i].type == FT_CLOSE_BRACE)
        {
            depth--;
            if (depth == 0) return i;
        }
    }
    return -1;
}

/* Is the { at brace_idx the start of a struct initializer?
 * Keyed on content: non-empty braces whose first tokens are WORD COLON are
 * always a struct init, whether the brace is preceded by a type name (named
 * form `Name { ... }`) or by `=` / `,` / `:` (inferred form `var x: T = { ... }`,
 * nested field, etc.). Empty braces `{}` only count when preceded by a type
 * name so we don't misclassify empty array literals. */
static int is_struct_init_at(FT *tokens, int count, int brace_idx)
{
    int j = brace_idx + 1;
    if (j >= count) return 0;
    if (tokens[j].type == FT_CLOSE_BRACE)
        return brace_idx > 0 && tokens[brace_idx - 1].type == FT_WORD;
    if (j + 1 < count && tokens[j].type == FT_WORD && tokens[j + 1].type == FT_COLON)
        return 1;
    return 0;
}

/* Count the number of fields in a struct init / array literal.
 * Counts top-level commas between open_idx and the matching close brace, +1. */
static int count_fields_at(FT *tokens, int count, int brace_idx)
{
    int close = find_close_brace(tokens, count, brace_idx);
    if (close < 0) return 0;
    if (close == brace_idx + 1) return 0; /* empty */
    int fields = 1;
    int depth = 0;
    for (int i = brace_idx + 1; i < close; i++)
    {
        if (tokens[i].type == FT_OPEN_BRACE || tokens[i].type == FT_OPEN_PAREN ||
            tokens[i].type == FT_OPEN_BRACKET)
            depth++;
        else if (tokens[i].type == FT_CLOSE_BRACE || tokens[i].type == FT_CLOSE_PAREN ||
                 tokens[i].type == FT_CLOSE_BRACKET)
            depth--;
        else if (tokens[i].type == FT_COMMA && depth == 0)
            fields++;
    }
    return fields;
}

/* Length in tokens of a type reference starting at `start`, or 0 if the
 * tokens there don't form one. Recognizes a bare `WORD` and a generic
 * `WORD ANGLE_OPEN ... ANGLE_CLOSE` (with arbitrary nesting). Array suffixes
 * like `T[]` are intentionally not supported — array literals don't use a
 * leading type name, so they never appear in the pattern we rewrite. */
static int type_seq_len(FT *tokens, int count, int start)
{
    if (start >= count || tokens[start].type != FT_WORD) return 0;
    int i = start + 1;
    if (i < count && tokens[i].type == FT_ANGLE_OPEN)
    {
        int depth = 1;
        i++;
        while (i < count && depth > 0)
        {
            if (tokens[i].type == FT_ANGLE_OPEN) depth++;
            else if (tokens[i].type == FT_ANGLE_CLOSE) depth--;
            i++;
        }
        if (depth != 0) return 0;
    }
    return i - start;
}

static int type_seqs_equal(FT *a, FT *b, int len)
{
    for (int k = 0; k < len; k++)
    {
        if (a[k].type != b[k].type) return 0;
        if (a[k].len != b[k].len) return 0;
        if (memcmp(a[k].text, b[k].text, a[k].len) != 0) return 0;
    }
    return 1;
}

/* Strip a redundant type name on the RHS of a declaration:
 * `var x: T = T { ... }` becomes `var x: T = { ... }`, and similarly for
 * generic types like `Stack<int> = Stack<int> { ... }`.
 *
 * Matches `COLON <type-seq> EQUAL <type-seq> OPEN_BRACE ...` where the two
 * type sequences are byte-identical. The brace must be empty or start with
 * a `WORD COLON` field so we don't rewrite an array literal like `{1, 2, 3}`. */
static void strip_redundant_struct_type(FT *tokens, int *count)
{
    int n = *count;
    int i = 0;
    while (i < n)
    {
        if (tokens[i].type != FT_COLON) { i++; continue; }
        int lhs_len = type_seq_len(tokens, n, i + 1);
        if (lhs_len == 0) { i++; continue; }
        int eq = i + 1 + lhs_len;
        if (eq >= n || tokens[eq].type != FT_EQUAL) { i++; continue; }
        int rhs_start = eq + 1;
        int rhs_len = type_seq_len(tokens, n, rhs_start);
        if (rhs_len == 0) { i++; continue; }
        int brace = rhs_start + rhs_len;
        if (brace >= n || tokens[brace].type != FT_OPEN_BRACE) { i++; continue; }
        /* Brace body must be empty `{}`, start with `WORD COLON`, or continue
         * on subsequent lines (brace is the last token on this line). The
         * third case covers multi-line struct literals where the body sits
         * on the following lines — the LHS/RHS type-sequence match already
         * makes the intent unambiguous. */
        int body = brace + 1;
        int ok = 0;
        if (body >= n)
            ok = 1;
        else if (tokens[body].type == FT_CLOSE_BRACE)
            ok = 1;
        else if (body + 1 < n && tokens[body].type == FT_WORD &&
                 tokens[body + 1].type == FT_COLON)
            ok = 1;
        if (!ok) { i++; continue; }
        if (lhs_len != rhs_len) { i++; continue; }
        if (!type_seqs_equal(&tokens[i + 1], &tokens[rhs_start], lhs_len))
        {
            i++;
            continue;
        }
        /* Drop the RHS type sequence. */
        for (int j = rhs_start; j + rhs_len < n; j++)
            tokens[j] = tokens[j + rhs_len];
        n -= rhs_len;
        i++;
    }
    *count = n;
}

/* Find the end of a struct init field starting at 'start'.
 * Returns the index of the top-level COMMA or close_idx (whichever comes first). */
static int find_field_end(FT *tokens, int start, int close_idx)
{
    int depth = 0;
    for (int i = start; i < close_idx; i++)
    {
        if (tokens[i].type == FT_OPEN_BRACE || tokens[i].type == FT_OPEN_PAREN ||
            tokens[i].type == FT_OPEN_BRACKET)
            depth++;
        else if (tokens[i].type == FT_CLOSE_BRACE || tokens[i].type == FT_CLOSE_PAREN ||
                 tokens[i].type == FT_CLOSE_BRACKET)
            depth--;
        else if (tokens[i].type == FT_COMMA && depth == 0)
            return i;
    }
    return close_idx;
}

/* ============================================================================
 * Section 5: Token emission with struct init splitting
 *
 * emit_tokens() walks a range of tokens and writes formatted output.
 * When it encounters a struct init with 2+ fields on one line, it splits
 * into multi-line format. This is recursive for nested struct inits.
 * ============================================================================ */

static void emit_tokens(Buf *out, FT *tokens, int start, int end, int indent);

/* Emit a multi-line struct initializer.
 * open_idx is the { token, close_idx is the matching }. */
static void emit_struct_init_multi(Buf *out, FT *tokens, int token_count,
                                   int open_idx, int close_idx, int indent)
{
    int total_fields = count_fields_at(tokens, token_count, open_idx);

    buf_char(out, '{');

    int i = open_idx + 1;
    int field_num = 0;

    while (i < close_idx)
    {
        field_num++;
        int field_end = find_field_end(tokens, i, close_idx);

        /* Newline + indent for each field */
        buf_char(out, '\n');
        buf_indent(out, indent + 1);

        /* Emit field tokens (may recursively split nested struct inits) */
        emit_tokens(out, tokens, i, field_end, indent + 1);

        /* Comma after every field except the last */
        if (field_num < total_fields)
            buf_char(out, ',');

        /* Advance past the comma */
        i = field_end;
        if (i < close_idx && tokens[i].type == FT_COMMA) i++;
    }

    /* Closing brace on its own line at the original indent */
    buf_char(out, '\n');
    buf_indent(out, indent);
    buf_char(out, '}');
}

/* Emit tokens[start..end) with proper spacing.
 * Handles struct init detection and splitting. */
static void emit_tokens(Buf *out, FT *tokens, int start, int end, int indent)
{
    /* Virtual close-brace token for prev tracking after brace processing */
    FT virt_close = { FT_CLOSE_BRACE, "}", 1 };
    FT *prev = NULL;

    for (int i = start; i < end; i++)
    {
        FT *tok = &tokens[i];

        /* ---- Handle { specially ---- */
        if (tok->type == FT_OPEN_BRACE)
        {
            int close = find_close_brace(tokens, end, i);
            if (close >= 0)
            {
                int is_struct = is_struct_init_at(tokens, end, i);
                int fields = count_fields_at(tokens, end, i);

                /* Multi-line struct init (2+ fields) */
                if (is_struct && fields >= 2)
                {
                    if (prev && needs_space(prev, tok)) buf_char(out, ' ');
                    emit_struct_init_multi(out, tokens, end, i, close, indent);
                    prev = &virt_close;
                    i = close;
                    continue;
                }

                /* Single-line brace content */
                if (prev && needs_space(prev, tok)) buf_char(out, ' ');
                buf_char(out, '{');

                if (close > i + 1)
                {
                    /* Non-empty content — space inside struct inits and arrays
                     * containing struct inits (e.g., {Point { x: 1 }}) */
                    int inner_has_struct = (i + 2 < close &&
                        tokens[i + 1].type == FT_WORD &&
                        tokens[i + 2].type == FT_OPEN_BRACE);
                    if (is_struct || inner_has_struct) buf_char(out, ' ');
                    emit_tokens(out, tokens, i + 1, close, indent);
                    if (is_struct || inner_has_struct) buf_char(out, ' ');
                }

                buf_char(out, '}');
                prev = &virt_close;
                i = close;
                continue;
            }
            /* No matching } on this line — emit { as regular token (multi-line in source) */
        }

        /* ---- Trailing // comment ---- */
        if (tok->type == FT_COMMENT)
        {
            buf_cstr(out, "  //");
            const char *c = tok->text + 2;
            const char *c_end = tok->text + tok->len;
            while (c < c_end && *c == ' ') c++;
            /* Trim trailing whitespace from comment */
            while (c_end > c && (*(c_end - 1) == ' ' || *(c_end - 1) == '\t'))
                c_end--;
            if (c < c_end)
            {
                buf_char(out, ' ');
                buf_str(out, c, (int)(c_end - c));
            }
            break; /* Comment ends the line */
        }

        /* ---- Regular token ---- */
        if (prev && needs_space(prev, tok))
            buf_char(out, ' ');

        buf_str(out, tok->text, tok->len);
        prev = tok;
    }
}

/* ============================================================================
 * Section 6: Source formatting (line by line)
 *
 * Processes source line-by-line:
 *   1. Measures original indentation
 *   2. Computes logical indent level via an indent stack
 *   3. Classifies line (blank, # comment, // comment, @pragma, code)
 *   4. Emits formatted output with 2-space indentation
 * ============================================================================ */

#define MAX_INDENT_DEPTH 128

/* Compute the logical indent level given the original character indent.
 * Updates the indent stack accordingly. */
static int compute_indent_level(int *stack, int *depth, int orig_indent)
{
    if (orig_indent > stack[*depth - 1])
    {
        /* Indent increased — push */
        if (*depth < MAX_INDENT_DEPTH)
            stack[(*depth)++] = orig_indent;
        return *depth - 1;
    }

    if (orig_indent < stack[*depth - 1])
    {
        /* Indent decreased — pop until we find a match or lower */
        while (*depth > 1 && stack[*depth - 1] > orig_indent)
            (*depth)--;
        /* If we overshot (indent doesn't exactly match a stack level), push it */
        if (stack[*depth - 1] < orig_indent)
        {
            if (*depth < MAX_INDENT_DEPTH)
                stack[(*depth)++] = orig_indent;
        }
        return *depth - 1;
    }

    /* Unchanged */
    return *depth - 1;
}

/* Format a # comment line. Normalizes to "# content". */
static void format_hash_comment(Buf *out, const char *content, const char *end)
{
    buf_char(out, '#');
    content++; /* skip the # */
    /* Skip whitespace after # */
    while (content < end && (*content == ' ' || *content == '\t')) content++;
    if (content < end && !is_eol(*content))
    {
        buf_char(out, ' ');
        buf_trimmed(out, content, end);
    }
}

/* Format a standalone // comment line. Normalizes to "// content". */
static void format_slash_comment(Buf *out, const char *content, const char *end)
{
    buf_cstr(out, "//");
    content += 2; /* skip the // */
    while (content < end && *content == ' ') content++;
    if (content < end && !is_eol(*content))
    {
        buf_char(out, ' ');
        buf_trimmed(out, content, end);
    }
}

/* Format an @pragma line. Normalizes spacing between @keyword and arguments. */
static void format_pragma(Buf *out, const char *content, const char *end)
{
    /* Emit @keyword */
    const char *p = content + 1; /* skip @ */
    while (p < end && (isalpha((unsigned char)*p) || *p == '_')) p++;
    buf_str(out, content, (int)(p - content));

    /* Skip whitespace */
    while (p < end && (*p == ' ' || *p == '\t')) p++;

    /* Emit rest of line (arguments) with one space separator */
    if (p < end && !is_eol(*p))
    {
        buf_char(out, ' ');
        buf_trimmed(out, p, end);
    }
}

/* Check if a line of code opens a multi-line string or pipe block.
 * Scans from 'content' to 'end' looking for unmatched quotes or trailing |.
 * Returns: 0 = normal line, '"' = inside multi-line string, '$' = inside multi-line
 * interpolated string, '|' = pipe block. */
static int detect_multiline_open(const char *content, const char *end)
{
    const char *p = content;
    while (p < end)
    {
        if (*p == '\'')
        {
            /* Char literal — skip */
            p++;
            if (p < end && *p == '\\') p++;
            if (p < end) p++;
            if (p < end && *p == '\'') p++;
            continue;
        }
        if (*p == '$' && p + 1 < end && *(p + 1) == '"')
        {
            /* Interpolated string */
            p += 2;
            int depth = 0;
            while (p < end && !(*p == '"' && depth == 0))
            {
                if (*p == '{') depth++;
                else if (*p == '}') { if (depth > 0) depth--; }
                else if (*p == '\\' && p + 1 < end) { p += 2; continue; }
                p++;
            }
            if (p < end && *p == '"') { p++; continue; } /* Closed on this line */
            return '$'; /* Unclosed interpolated string */
        }
        if (*p == '"')
        {
            /* Regular string */
            p++;
            while (p < end && *p != '"')
            {
                if (*p == '\\' && p + 1 < end) p++;
                p++;
            }
            if (p < end && *p == '"') { p++; continue; } /* Closed on this line */
            return '"'; /* Unclosed string */
        }
        if (*p == '/' && p + 1 < end && *(p + 1) == '/')
            break; /* Rest is comment, stop scanning */
        if (*p == '#')
            break; /* Rest is comment */
        p++;
    }

    /* Check for pipe block: line ends with = | or = $| (with optional whitespace) */
    const char *q = end;
    while (q > content && (*(q - 1) == ' ' || *(q - 1) == '\t')) q--;
    if (q > content && *(q - 1) == '|')
    {
        q--;
        /* Skip optional $ prefix (interpolated pipe: $|) */
        if (q > content && *(q - 1) == '$') q--;
        /* Check there's a = before (with optional whitespace) */
        while (q > content && (*(q - 1) == ' ' || *(q - 1) == '\t')) q--;
        if (q > content && *(q - 1) == '=')
            return '|';
    }

    return 0;
}

/* Format an entire source file. Returns a newly allocated string. */
static char *format_source(const char *source)
{
    Buf out;
    buf_init(&out);

    int indent_stack[MAX_INDENT_DEPTH];
    int indent_depth = 1;
    indent_stack[0] = 0;

    int consecutive_blanks = 0;
    int in_multiline = 0; /* 0 = normal, '"' = multi-line string, '$' = multi-line
                             interpolated string, '|' = pipe block */
    const char *p = source;

    while (*p)
    {
        /* Find line boundaries */
        const char *line_start = p;
        while (*p && *p != '\n') p++;
        const char *line_end = p;
        if (*p == '\n') p++;

        /* Strip trailing \r (Windows line endings) */
        if (line_end > line_start && *(line_end - 1) == '\r')
            line_end--;

        /* ---- Multi-line string / pipe block continuation ---- */
        if (in_multiline)
        {
            /* Emit line verbatim (preserve content exactly) */
            buf_str(&out, line_start, (int)(line_end - line_start));
            buf_char(&out, '\n');

            if (in_multiline == '|')
            {
                /* Pipe block ends at a blank line (content is empty after stripping indent) */
                const char *t = line_start;
                while (t < line_end && (*t == ' ' || *t == '\t')) t++;
                if (t >= line_end)
                    in_multiline = 0; /* Blank line terminates pipe block */
            }
            else
            {
                /* String continues until we find the closing quote on this line */
                const char *q = line_start;
                while (q < line_end)
                {
                    if (*q == '\\' && q + 1 < line_end) { q += 2; continue; }
                    if (*q == '"') { in_multiline = 0; break; }
                    q++;
                }
            }
            consecutive_blanks = 0;
            continue;
        }

        /* Measure original indentation */
        const char *content = line_start;
        int orig_indent = 0;
        while (content < line_end && (*content == ' ' || *content == '\t'))
        {
            if (*content == '\t') orig_indent += 4;
            else orig_indent++;
            content++;
        }

        /* ---- Blank line ---- */
        if (content >= line_end)
        {
            consecutive_blanks++;
            if (consecutive_blanks <= 1)
                buf_char(&out, '\n');
            continue;
        }
        consecutive_blanks = 0;

        /* Compute indent level */
        int level = compute_indent_level(indent_stack, &indent_depth, orig_indent);

        /* ---- # comment or #pragma ---- */
        if (*content == '#')
        {
            buf_indent(&out, level);
            if (line_end - content >= 7 && strncmp(content, "#pragma", 7) == 0)
                buf_trimmed(&out, content, line_end); /* #pragma — preserve as-is */
            else
                format_hash_comment(&out, content, line_end);
            buf_char(&out, '\n');
            continue;
        }

        /* ---- @pragma ---- */
        if (*content == '@')
        {
            buf_indent(&out, level);
            format_pragma(&out, content, line_end);
            buf_char(&out, '\n');
            continue;
        }

        /* ---- Standalone // comment ---- */
        if (content + 1 < line_end && content[0] == '/' && content[1] == '/')
        {
            buf_indent(&out, level);
            format_slash_comment(&out, content, line_end);
            buf_char(&out, '\n');
            continue;
        }

        /* ---- Code line ---- */
        FT tokens[MAX_LINE_TOKENS];
        int count = tokenize_line(content, line_end, tokens);
        classify_generics(tokens, &count);
        strip_redundant_struct_type(tokens, &count);

        if (count > 0)
        {
            buf_indent(&out, level);
            emit_tokens(&out, tokens, 0, count, level);
        }
        buf_char(&out, '\n');

        /* Check if this line opens a multi-line string or pipe block */
        in_multiline = detect_multiline_open(content, line_end);
    }

    /* Ensure file ends with exactly one trailing newline */
    while (out.len > 1 && out.data[out.len - 1] == '\n' && out.data[out.len - 2] == '\n')
        out.len--;

    buf_null(&out);
    return out.data;
}

/* ============================================================================
 * Section 7: File I/O
 * ============================================================================ */

static char *read_file_contents(const char *path, long *out_size)
{
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    char *data = malloc(size + 1);
    if (!data) { fclose(f); return NULL; }

    size_t nread = fread(data, 1, size, f);
    data[nread] = '\0';
    fclose(f);

    if (out_size) *out_size = (long)nread;
    return data;
}

int formatter_format_file(const char *path, int check_only)
{
    long size;
    char *source = read_file_contents(path, &size);
    if (!source)
    {
        fprintf(stderr, "Error: cannot read '%s'\n", path);
        return -1;
    }

    char *formatted = format_source(source);
    if (!formatted)
    {
        free(source);
        return -1;
    }

    /* Compare ignoring \r so that --check tolerates CRLF line endings.
     * The formatter always outputs LF, but on Windows the source may have CRLF. */
    int changed = 0;
    {
        const char *s = source;
        const char *f = formatted;
        while (*s && *f)
        {
            if (*s == '\r') { s++; continue; }
            if (*s != *f) { changed = 1; break; }
            s++;
            f++;
        }
        /* Skip any trailing \r in source */
        while (*s == '\r') s++;
        if (!changed && (*s || *f)) changed = 1;
    }

    if (changed)
    {
        if (check_only)
        {
            printf("Would reformat: %s\n", path);
        }
        else
        {
            FILE *f = fopen(path, "wb");
            if (!f)
            {
                fprintf(stderr, "Error: cannot write '%s'\n", path);
                free(source);
                free(formatted);
                return -1;
            }
            fwrite(formatted, 1, strlen(formatted), f);
            fclose(f);
            printf("Formatted: %s\n", path);
        }
    }

    free(source);
    free(formatted);
    return changed ? 1 : 0;
}

/* ============================================================================
 * Section 8: Directory walking
 * ============================================================================ */

#ifndef _WIN32

int formatter_format_directory(const char *dir, int check_only)
{
    DIR *d = opendir(dir);
    if (!d)
    {
        fprintf(stderr, "Error: cannot open directory '%s'\n", dir);
        return -1;
    }

    int total = 0;
    struct dirent *entry;

    while ((entry = readdir(d)) != NULL)
    {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        /* Skip .sn directory (package cache / build artifacts) */
        if (strcmp(entry->d_name, ".sn") == 0)
            continue;

        /* Skip hidden directories */
        if (entry->d_name[0] == '.')
            continue;

        char path[PATH_MAX];
        snprintf(path, sizeof(path), "%s/%s", dir, entry->d_name);

        struct stat st;
        if (stat(path, &st) != 0) continue;

        if (S_ISDIR(st.st_mode))
        {
            int sub = formatter_format_directory(path, check_only);
            if (sub > 0) total += sub;
        }
        else if (S_ISREG(st.st_mode))
        {
            int nlen = (int)strlen(entry->d_name);
            if (nlen > 3 && strcmp(entry->d_name + nlen - 3, ".sn") == 0)
            {
                int result = formatter_format_file(path, check_only);
                if (result > 0) total++;
            }
        }
    }

    closedir(d);
    return total;
}

#else /* _WIN32 */

int formatter_format_directory(const char *dir, int check_only)
{
    char search_path[MAX_PATH];
    snprintf(search_path, sizeof(search_path), "%s\\*", dir);

    WIN32_FIND_DATAA fdata;
    HANDLE h = FindFirstFileA(search_path, &fdata);
    if (h == INVALID_HANDLE_VALUE) return -1;

    int total = 0;
    do
    {
        if (strcmp(fdata.cFileName, ".") == 0 || strcmp(fdata.cFileName, "..") == 0)
            continue;
        if (strcmp(fdata.cFileName, ".sn") == 0)
            continue;
        if (fdata.cFileName[0] == '.')
            continue;

        char path[MAX_PATH];
        snprintf(path, sizeof(path), "%s\\%s", dir, fdata.cFileName);

        if (fdata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            int sub = formatter_format_directory(path, check_only);
            if (sub > 0) total += sub;
        }
        else
        {
            int nlen = (int)strlen(fdata.cFileName);
            if (nlen > 3 && strcmp(fdata.cFileName + nlen - 3, ".sn") == 0)
            {
                int result = formatter_format_file(path, check_only);
                if (result > 0) total++;
            }
        }
    } while (FindNextFileA(h, &fdata));

    FindClose(h);
    return total;
}

#endif
