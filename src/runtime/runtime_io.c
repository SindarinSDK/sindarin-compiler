#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "runtime_io.h"

/* ============================================================================
 * Standard Stream Operations (Stdin, Stdout, Stderr)
 * ============================================================================
 * Implementation of console I/O functions for standard streams.
 * ============================================================================ */

/* ============================================================================
 * Stdin Operations
 * ============================================================================ */

/* Stdin - read line from standard input */
char *rt_stdin_read_line(RtArena *arena) {
    /* Read a line from stdin, stripping trailing newline */
    char buffer[4096];
    if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
        /* EOF or error - return empty string */
        char *result = rt_arena_alloc(arena, 1);
        result[0] = '\0';
        return result;
    }

    /* Strip trailing newline if present */
    size_t len = strlen(buffer);
    if (len > 0 && buffer[len - 1] == '\n') {
        buffer[len - 1] = '\0';
        len--;
    }

    char *result = rt_arena_alloc(arena, len + 1);
    memcpy(result, buffer, len + 1);
    return result;
}

/* Stdin - read single character from standard input */
long rt_stdin_read_char(void) {
    int ch = fgetc(stdin);
    return ch;  /* Returns -1 on EOF */
}

/* Stdin - read whitespace-delimited word from standard input */
char *rt_stdin_read_word(RtArena *arena) {
    char buffer[4096];
    if (scanf("%4095s", buffer) != 1) {
        /* EOF or error - return empty string */
        char *result = rt_arena_alloc(arena, 1);
        result[0] = '\0';
        return result;
    }

    size_t len = strlen(buffer);
    char *result = rt_arena_alloc(arena, len + 1);
    memcpy(result, buffer, len + 1);
    return result;
}

/* Stdin - check if characters available */
int rt_stdin_has_chars(void) {
    int ch = fgetc(stdin);
    if (ch == EOF) {
        return 0;
    }
    ungetc(ch, stdin);
    return 1;
}

/* Stdin - check if lines available */
int rt_stdin_has_lines(void) {
    /* Same as has_chars for stdin */
    return rt_stdin_has_chars();
}

/* Stdin - check if at EOF */
int rt_stdin_is_eof(void) {
    return feof(stdin);
}

/* ============================================================================
 * Stdout Operations
 * ============================================================================ */

/* Stdout - write text */
void rt_stdout_write(const char *text) {
    if (text != NULL) {
        fputs(text, stdout);
    }
}

/* Stdout - write text with newline */
void rt_stdout_write_line(const char *text) {
    if (text != NULL) {
        fputs(text, stdout);
    }
    fputc('\n', stdout);
}

/* Stdout - flush output */
void rt_stdout_flush(void) {
    fflush(stdout);
}

/* ============================================================================
 * Stderr Operations
 * ============================================================================ */

/* Stderr - write text */
void rt_stderr_write(const char *text) {
    if (text != NULL) {
        fputs(text, stderr);
    }
}

/* Stderr - write text with newline */
void rt_stderr_write_line(const char *text) {
    if (text != NULL) {
        fputs(text, stderr);
    }
    fputc('\n', stderr);
}

/* Stderr - flush output */
void rt_stderr_flush(void) {
    fflush(stderr);
}

/* ============================================================================
 * Convenience Functions
 * ============================================================================ */

/* Global convenience: read line */
char *rt_read_line(RtArena *arena) {
    return rt_stdin_read_line(arena);
}

/* Global convenience: print with newline */
void rt_println(const char *text) {
    rt_stdout_write_line(text);
}

/* Global convenience: print to stderr */
void rt_print_err(const char *text) {
    rt_stderr_write(text);
}

/* Global convenience: print to stderr with newline */
void rt_print_err_ln(const char *text) {
    rt_stderr_write_line(text);
}

/* ============================================================================
 * Program Control Functions
 * ============================================================================ */

/* Exit the program with the specified exit code. */
void rt_exit(int code)
{
    exit(code);
}

/* Assert that a condition is true. */
void rt_assert(int condition, const char *message)
{
    if (!condition)
    {
        fprintf(stderr, "%s\n", message);
        exit(1);
    }
}
