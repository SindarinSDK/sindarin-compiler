#ifndef DIAGNOSTIC_H
#define DIAGNOSTIC_H

#include <stdio.h>
#include <stdarg.h>
#include "token.h"

/*
 * Diagnostic system for Sn compiler
 * Provides unified error reporting with source context
 */

/* Diagnostic severity levels */
typedef enum {
    DIAG_NOTE,
    DIAG_WARNING,
    DIAG_ERROR
} DiagnosticLevel;

/* Source location for diagnostics */
typedef struct {
    const char *filename;
    int line;
    int column;
    int length;  /* Length of the error span for underlining */
} DiagnosticLoc;

/*
 * Initialize the diagnostic system with source info.
 * Must be called before reporting any diagnostics.
 *
 * @param filename  Name of the source file
 * @param source    Full source code (for extracting context lines)
 */
void diagnostic_init(const char *filename, const char *source);

/*
 * Reset diagnostic state (clear error count, etc.)
 */
void diagnostic_reset(void);

/*
 * Check if any errors have been reported
 */
int diagnostic_had_error(void);

/*
 * Get the number of errors reported
 */
int diagnostic_error_count(void);

/*
 * Get the number of warnings reported
 */
int diagnostic_warning_count(void);

/*
 * Report a diagnostic with full location info.
 *
 * @param level     Severity (DIAG_ERROR, DIAG_WARNING, DIAG_NOTE)
 * @param loc       Location in source
 * @param fmt       Printf-style format string
 * @param ...       Format arguments
 */
void diagnostic_report(DiagnosticLevel level, DiagnosticLoc loc,
                       const char *fmt, ...);

/*
 * Report an error at a specific location
 */
void diagnostic_error(const char *filename, int line, int column, int length,
                      const char *fmt, ...);

/*
 * Report a simple error without source context (e.g., file not found)
 */
void diagnostic_error_simple(const char *fmt, ...);

/*
 * Report an error at a token's location
 */
void diagnostic_error_at(Token *token, const char *fmt, ...);

/*
 * Report an error with a suggestion (for "did you mean X?")
 */
void diagnostic_error_with_suggestion(Token *token, const char *suggestion,
                                      const char *fmt, ...);

/*
 * Report a warning at a token's location
 */
void diagnostic_warning_at(Token *token, const char *fmt, ...);

/*
 * Report a note (additional context for a previous error)
 */
void diagnostic_note_at(Token *token, const char *fmt, ...);

/*
 * Create a DiagnosticLoc from a Token
 */
DiagnosticLoc diagnostic_loc_from_token(Token *token);

/* ============================================================
 * Compilation progress reporting
 * ============================================================ */

/* Compilation phases */
typedef enum {
    PHASE_PARSING,
    PHASE_TYPE_CHECK,
    PHASE_CODE_GEN,
    PHASE_LINKING
} CompilationPhase;

/*
 * Set verbose mode for progress output
 */
void diagnostic_set_verbose(int verbose);

/*
 * Report compilation start
 */
void diagnostic_compile_start(const char *filename);

/*
 * Report phase start
 */
void diagnostic_phase_start(CompilationPhase phase);

/*
 * Report phase completion (with optional timing in verbose mode)
 */
void diagnostic_phase_done(CompilationPhase phase, double elapsed_secs);

/*
 * Report phase failure
 */
void diagnostic_phase_failed(CompilationPhase phase);

/*
 * Report successful compilation
 *
 * @param output_file   Path to the output file
 * @param file_size     Size of output in bytes
 * @param total_secs    Total compilation time
 */
void diagnostic_compile_success(const char *output_file, long file_size,
                                double total_secs);

/*
 * Report compilation failure
 */
void diagnostic_compile_failed(void);

#endif /* DIAGNOSTIC_H */
