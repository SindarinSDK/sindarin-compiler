#ifndef RUNTIME_IO_H
#define RUNTIME_IO_H

#include <stdbool.h>
#include "runtime_arena.h"

/* ============================================================================
 * Standard Stream Operations (Stdin, Stdout, Stderr)
 * ============================================================================
 * Functions for reading from standard input and writing to standard output
 * and standard error streams. These provide a consistent interface for
 * console I/O operations.
 * ============================================================================ */

/* ============================================================================
 * Stdin Operations
 * ============================================================================ */

/* Read a line from standard input (strips trailing newline) */
char *rt_stdin_read_line(RtArena *arena);

/* Read a single character from standard input (returns -1 on EOF) */
long rt_stdin_read_char(void);

/* Read a whitespace-delimited word from standard input */
char *rt_stdin_read_word(RtArena *arena);

/* Check if characters are available on stdin */
int rt_stdin_has_chars(void);

/* Check if lines are available on stdin */
int rt_stdin_has_lines(void);

/* Check if stdin is at EOF */
int rt_stdin_is_eof(void);

/* ============================================================================
 * Stdout Operations
 * ============================================================================ */

/* Write text to standard output */
void rt_stdout_write(const char *text);

/* Write text with newline to standard output */
void rt_stdout_write_line(const char *text);

/* Flush standard output */
void rt_stdout_flush(void);

/* ============================================================================
 * Stderr Operations
 * ============================================================================ */

/* Write text to standard error */
void rt_stderr_write(const char *text);

/* Write text with newline to standard error */
void rt_stderr_write_line(const char *text);

/* Flush standard error */
void rt_stderr_flush(void);

/* ============================================================================
 * Convenience Functions
 * ============================================================================ */

/* Read a line from stdin (alias for rt_stdin_read_line) */
char *rt_read_line(RtArena *arena);

/* Print text with newline to stdout (alias for rt_stdout_write_line) */
void rt_println(const char *text);

/* Print text to stderr (alias for rt_stderr_write) */
void rt_print_err(const char *text);

/* Print text with newline to stderr (alias for rt_stderr_write_line) */
void rt_print_err_ln(const char *text);

/* ============================================================================
 * Program Control Functions
 * ============================================================================ */

/* Exit the program with the specified exit code.
 * This is a wrapper around the C exit() function for Sindarin programs.
 */
void rt_exit(int code);

/* Assert that a condition is true.
 * If the condition is false, writes the message to stderr and exits with code 1.
 */
void rt_assert(int condition, const char *message);

#endif /* RUNTIME_IO_H */
