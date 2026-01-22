/* ==============================================================================
 * sdk/stdio.sn.c - Standard I/O Implementation for Sindarin SDK
 * ==============================================================================
 * This file provides the C implementation for Stdin, Stdout, and Stderr types.
 * It is compiled via @source and linked with Sindarin code.
 *
 * These are thin wrappers around the runtime I/O functions in runtime_io.c.
 * ============================================================================== */

#include "runtime/runtime_io.h"

/* ============================================================================
 * Type Definitions (Static-only, never instantiated)
 * ============================================================================ */

typedef struct RtStdin {
    int _unused;  /* Placeholder - this struct is never instantiated */
} RtStdin;

typedef struct RtStdout {
    int _unused;  /* Placeholder - this struct is never instantiated */
} RtStdout;

typedef struct RtStderr {
    int _unused;  /* Placeholder - this struct is never instantiated */
} RtStderr;

/* ============================================================================
 * Stdin Functions
 * ============================================================================ */

/* Read a line from standard input (strips trailing newline) */
char *sn_stdin_read_line(RtArena *arena)
{
    return rt_stdin_read_line(arena);
}

/* Read a single character from standard input (returns -1 on EOF) */
long sn_stdin_read_char(void)
{
    return rt_stdin_read_char();
}

/* Read a whitespace-delimited word from standard input */
char *sn_stdin_read_word(RtArena *arena)
{
    return rt_stdin_read_word(arena);
}

/* Check if characters are available on stdin */
int sn_stdin_has_chars(void)
{
    return rt_stdin_has_chars();
}

/* Check if lines are available on stdin */
int sn_stdin_has_lines(void)
{
    return rt_stdin_has_lines();
}

/* Check if stdin is at EOF */
int sn_stdin_is_eof(void)
{
    return rt_stdin_is_eof();
}

/* ============================================================================
 * Stdout Functions
 * ============================================================================ */

/* Write text to standard output */
void sn_stdout_write(const char *text)
{
    rt_stdout_write(text);
}

/* Write text with newline to standard output */
void sn_stdout_write_line(const char *text)
{
    rt_stdout_write_line(text);
}

/* Flush standard output */
void sn_stdout_flush(void)
{
    rt_stdout_flush();
}

/* ============================================================================
 * Stderr Functions
 * ============================================================================ */

/* Write text to standard error */
void sn_stderr_write(const char *text)
{
    rt_stderr_write(text);
}

/* Write text with newline to standard error */
void sn_stderr_write_line(const char *text)
{
    rt_stderr_write_line(text);
}

/* Flush standard error */
void sn_stderr_flush(void)
{
    rt_stderr_flush();
}
