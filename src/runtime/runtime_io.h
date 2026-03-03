#ifndef RUNTIME_IO_H
#define RUNTIME_IO_H

#include <stdbool.h>
#include "arena/arena_v2.h"

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
RtHandleV2 *rt_stdin_read_line(RtArenaV2 *arena);

/* Read a single character from standard input (returns -1 on EOF) */
long rt_stdin_read_char(void);

/* Read a whitespace-delimited word from standard input */
RtHandleV2 *rt_stdin_read_word(RtArenaV2 *arena);

/* Check if characters are available on stdin */
int rt_stdin_has_chars(void);

/* Check if lines are available on stdin */
int rt_stdin_has_lines(void);

/* Check if stdin is at EOF */
int rt_stdin_is_eof(void);

/* ============================================================================
 * Stdout Operations
 * ============================================================================ */

/* Flush standard output */
void rt_stdout_flush(void);

/* ============================================================================
 * Stderr Operations
 * ============================================================================ */

/* Flush standard error */
void rt_stderr_flush(void);

/* ============================================================================
 * Convenience Functions
 * ============================================================================ */

/* Read a line from stdin (alias for rt_stdin_read_line) */
RtHandleV2 *rt_read_line(RtArenaV2 *arena);

/* ============================================================================
 * Handle-Based I/O Functions
 * ============================================================================ */

/* Print string handle to stdout (no newline) */
void rt_print_string_v2(RtHandleV2 *text);

/* Print string handle with newline to stdout */
void rt_println_v2(RtHandleV2 *text);

/* Print string handle to stderr (no newline) */
void rt_print_err_v2(RtHandleV2 *text);

/* Print string handle with newline to stderr */
void rt_print_err_ln_v2(RtHandleV2 *text);

/* ============================================================================
 * Program Control Functions
 * ============================================================================ */

/* Exit the program with the specified exit code. */
void rt_exit(int code);

/* Assert that a condition is true.
 * If the condition is false, writes the message to stderr and exits with code 1.
 */
void rt_assert_v2(int condition, RtHandleV2 *message);

#endif /* RUNTIME_IO_H */
