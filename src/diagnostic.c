#include "diagnostic.h"
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

/* ANSI color codes for terminal output */
#define COLOR_RESET   "\033[0m"
#define COLOR_RED     "\033[1;31m"
#define COLOR_YELLOW  "\033[1;33m"
#define COLOR_BLUE    "\033[1;34m"
#define COLOR_CYAN    "\033[1;36m"
#define COLOR_BOLD    "\033[1m"

/* Global diagnostic state */
static const char *g_filename = NULL;
static const char *g_source = NULL;
static int g_error_count = 0;
static int g_warning_count = 0;
static int g_verbose = 0;

/* ============================================================
 * Initialization and state management
 * ============================================================ */

void diagnostic_init(const char *filename, const char *source)
{
    g_filename = filename;
    g_source = source;
    g_error_count = 0;
    g_warning_count = 0;
}

void diagnostic_reset(void)
{
    g_error_count = 0;
    g_warning_count = 0;
}

int diagnostic_had_error(void)
{
    return g_error_count > 0;
}

int diagnostic_error_count(void)
{
    return g_error_count;
}

int diagnostic_warning_count(void)
{
    return g_warning_count;
}

void diagnostic_set_verbose(int verbose)
{
    g_verbose = verbose;
}

/* ============================================================
 * Source line extraction
 * ============================================================ */

/*
 * Extract a specific line from the source.
 * Returns a pointer to a static buffer (not thread-safe).
 */
static const char *get_source_line(int line_num)
{
    static char line_buffer[1024];

    if (g_source == NULL || line_num < 1)
    {
        return NULL;
    }

    const char *p = g_source;
    int current_line = 1;

    /* Find the start of the requested line */
    while (*p && current_line < line_num)
    {
        if (*p == '\n')
        {
            current_line++;
        }
        p++;
    }

    if (current_line != line_num)
    {
        return NULL;
    }

    /* Copy the line to our buffer */
    int i = 0;
    while (*p && *p != '\n' && i < (int)sizeof(line_buffer) - 1)
    {
        line_buffer[i++] = *p++;
    }
    line_buffer[i] = '\0';

    return line_buffer;
}

/* ============================================================
 * Core diagnostic reporting
 * ============================================================ */

void diagnostic_report(DiagnosticLevel level, DiagnosticLoc loc,
                       const char *fmt, ...)
{
    const char *level_str;
    const char *color;

    switch (level)
    {
    case DIAG_ERROR:
        level_str = "error";
        color = COLOR_RED;
        g_error_count++;
        break;
    case DIAG_WARNING:
        level_str = "warning";
        color = COLOR_YELLOW;
        g_warning_count++;
        break;
    case DIAG_NOTE:
        level_str = "note";
        color = COLOR_CYAN;
        break;
    default:
        level_str = "error";
        color = COLOR_RED;
        break;
    }

    /* Print the error header: "error: message" */
    fprintf(stderr, "%s%s%s: ", color, level_str, COLOR_RESET);

    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fprintf(stderr, "\n");

    /* Print location: "  --> file:line:column" */
    if (loc.filename)
    {
        fprintf(stderr, "  %s-->%s %s", COLOR_BLUE, COLOR_RESET, loc.filename);
        if (loc.line > 0)
        {
            fprintf(stderr, ":%d", loc.line);
            if (loc.column > 0)
            {
                fprintf(stderr, ":%d", loc.column);
            }
        }
        fprintf(stderr, "\n");

        /* Print source context if we have it */
        if (loc.line > 0 && g_source)
        {
            const char *source_line = get_source_line(loc.line);
            if (source_line)
            {
                /* Print source line with consistent indentation */
                fprintf(stderr, "      %s\n", source_line);

                /* Print the underline/caret line */
                fprintf(stderr, "      ");

                /* Spaces to reach the column */
                int col = loc.column > 0 ? loc.column : 1;
                for (int i = 1; i < col; i++)
                {
                    /* Preserve tabs in the source line */
                    if (i <= (int)strlen(source_line) && source_line[i - 1] == '\t')
                    {
                        fprintf(stderr, "\t");
                    }
                    else
                    {
                        fprintf(stderr, " ");
                    }
                }

                /* Print the underline (carets) */
                int len = loc.length > 0 ? loc.length : 1;
                fprintf(stderr, "%s", color);
                for (int i = 0; i < len; i++)
                {
                    fprintf(stderr, "^");
                }
                fprintf(stderr, "%s\n", COLOR_RESET);
            }
        }
    }

    fprintf(stderr, "\n");
}

/* ============================================================
 * Convenience functions
 * ============================================================ */

DiagnosticLoc diagnostic_loc_from_token(Token *token)
{
    DiagnosticLoc loc = {0};
    if (token)
    {
        loc.filename = token->filename;
        loc.line = token->line;
        loc.column = 1;  /* Token doesn't have column info directly */
        loc.length = token->length;

        /* Try to compute column from source if available */
        /* Only do this if token is within the bounds of g_source */
        if (g_source && token->start)
        {
            size_t source_len = strlen(g_source);
            const char *source_end = g_source + source_len;

            /* Check that token->start is within g_source bounds */
            if (token->start >= g_source && token->start < source_end)
            {
                const char *line_start = token->start;
                /* Walk back to find the start of the line */
                while (line_start > g_source && *(line_start - 1) != '\n')
                {
                    line_start--;
                }
                loc.column = (int)(token->start - line_start) + 1;
            }
        }
    }
    return loc;
}

void diagnostic_error(const char *filename, int line, int column, int length,
                      const char *fmt, ...)
{
    DiagnosticLoc loc = {filename, line, column, length};

    /* Format the message */
    char message[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(message, sizeof(message), fmt, args);
    va_end(args);

    diagnostic_report(DIAG_ERROR, loc, "%s", message);
}

void diagnostic_error_simple(const char *fmt, ...)
{
    g_error_count++;

    fprintf(stderr, "%serror%s: ", COLOR_RED, COLOR_RESET);

    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);

    fprintf(stderr, "\n\n");
}

void diagnostic_error_at(Token *token, const char *fmt, ...)
{
    DiagnosticLoc loc = diagnostic_loc_from_token(token);

    char message[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(message, sizeof(message), fmt, args);
    va_end(args);

    diagnostic_report(DIAG_ERROR, loc, "%s", message);
}

void diagnostic_error_with_suggestion(Token *token, const char *suggestion,
                                      const char *fmt, ...)
{
    DiagnosticLoc loc = diagnostic_loc_from_token(token);

    char message[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(message, sizeof(message), fmt, args);
    va_end(args);

    /* Append suggestion to message */
    char full_message[1200];
    if (suggestion)
    {
        snprintf(full_message, sizeof(full_message), "%s (did you mean '%s'?)",
                 message, suggestion);
    }
    else
    {
        snprintf(full_message, sizeof(full_message), "%s", message);
    }

    diagnostic_report(DIAG_ERROR, loc, "%s", full_message);
}

void diagnostic_warning_at(Token *token, const char *fmt, ...)
{
    DiagnosticLoc loc = diagnostic_loc_from_token(token);

    char message[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(message, sizeof(message), fmt, args);
    va_end(args);

    diagnostic_report(DIAG_WARNING, loc, "%s", message);
}

void diagnostic_note_at(Token *token, const char *fmt, ...)
{
    DiagnosticLoc loc = diagnostic_loc_from_token(token);

    char message[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(message, sizeof(message), fmt, args);
    va_end(args);

    diagnostic_report(DIAG_NOTE, loc, "%s", message);
}

/* ============================================================
 * Compilation progress reporting
 * ============================================================ */

static const char *phase_names[] = {
    "Parsing",
    "Type checking",
    "Code generation",
    "Linking"
};

void diagnostic_compile_start(const char *filename)
{
    fprintf(stderr, "%sCompiling%s %s...\n", COLOR_BOLD, COLOR_RESET, filename);
}

void diagnostic_phase_start(CompilationPhase phase)
{
    fprintf(stderr, "  %s...", phase_names[phase]);
    fflush(stderr);
}

void diagnostic_phase_done(CompilationPhase phase, double elapsed_secs)
{
    (void)phase;  /* Unused when not verbose */

    if (g_verbose && elapsed_secs > 0)
    {
        fprintf(stderr, " %sdone%s (%.2fs)\n", COLOR_CYAN, COLOR_RESET, elapsed_secs);
    }
    else
    {
        fprintf(stderr, " %sdone%s\n", COLOR_CYAN, COLOR_RESET);
    }
}

void diagnostic_phase_failed(CompilationPhase phase)
{
    (void)phase;
    fprintf(stderr, " %sfailed%s\n\n", COLOR_RED, COLOR_RESET);
}

void diagnostic_compile_success(const char *output_file, long file_size,
                                double total_secs)
{
    /* Format file size nicely */
    char size_str[32];
    if (file_size >= 1024 * 1024)
    {
        snprintf(size_str, sizeof(size_str), "%.1f MB",
                 (double)file_size / (1024 * 1024));
    }
    else if (file_size >= 1024)
    {
        snprintf(size_str, sizeof(size_str), "%.1f KB",
                 (double)file_size / 1024);
    }
    else
    {
        snprintf(size_str, sizeof(size_str), "%ld bytes", file_size);
    }

    if (g_verbose)
    {
        fprintf(stderr, "\n%sDone%s: %s (%s) in %.2fs\n",
                COLOR_CYAN, COLOR_RESET, output_file, size_str, total_secs);
    }
    else
    {
        fprintf(stderr, "%sDone%s: %s (%s)\n",
                COLOR_CYAN, COLOR_RESET, output_file, size_str);
    }
}

void diagnostic_compile_failed(void)
{
    if (g_error_count > 0)
    {
        fprintf(stderr, "%sCompilation failed%s: %d error%s\n",
                COLOR_RED, COLOR_RESET,
                g_error_count, g_error_count == 1 ? "" : "s");
    }
    else
    {
        fprintf(stderr, "%sCompilation failed%s\n", COLOR_RED, COLOR_RESET);
    }
}
