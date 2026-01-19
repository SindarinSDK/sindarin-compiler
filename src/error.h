#ifndef ERROR_H
#define ERROR_H

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>

/*
 * Error severity levels
 */
typedef enum {
    ERROR_WARNING,
    ERROR_ERROR,
    ERROR_FATAL
} ErrorSeverity;

/*
 * Error context for better debugging
 */
typedef struct {
    const char *file;
    int line;
    int column;
} ErrorLocation;

/*
 * Report an error with location context.
 * For FATAL errors, exits the program.
 */
static inline void report_error(ErrorSeverity severity, ErrorLocation loc, 
                                const char *fmt, ...) {
    const char *prefix;
    switch (severity) {
        case ERROR_WARNING: prefix = "Warning"; break;
        case ERROR_ERROR:   prefix = "Error"; break;
        case ERROR_FATAL:   prefix = "Fatal"; break;
        default:            prefix = "Error"; break;
    }
    
    if (loc.file && loc.line > 0) {
        fprintf(stderr, "%s:%d:%d: %s: ", loc.file, loc.line, loc.column, prefix);
    } else if (loc.file) {
        fprintf(stderr, "%s: %s: ", loc.file, prefix);
    } else {
        fprintf(stderr, "%s: ", prefix);
    }
    
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    
    fprintf(stderr, "\n");
    
    if (severity == ERROR_FATAL) {
        exit(1);
    }
}

/*
 * Report a compiler internal error (bug)
 */
static inline void internal_error(const char *file, int line, const char *fmt, ...) {
    fprintf(stderr, "Internal compiler error at %s:%d: ", file, line);
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fprintf(stderr, "\nPlease report this bug.\n");
    exit(1);
}

/*
 * Convenience macros
 */
#define FATAL_ERROR(loc, ...) report_error(ERROR_FATAL, loc, __VA_ARGS__)
#define ERROR(loc, ...) report_error(ERROR_ERROR, loc, __VA_ARGS__)
#define WARNING(loc, ...) report_error(ERROR_WARNING, loc, __VA_ARGS__)

#define NO_LOCATION ((ErrorLocation){NULL, 0, 0})
#define FILE_LOCATION(f) ((ErrorLocation){f, 0, 0})
#define LOCATION(f, l, c) ((ErrorLocation){f, l, c})

#define INTERNAL_ERROR(...) internal_error(__FILE__, __LINE__, __VA_ARGS__)

/*
 * Assert with error message (for internal checks)
 */
#define COMPILER_ASSERT(cond, ...) do { \
    if (!(cond)) { \
        internal_error(__FILE__, __LINE__, __VA_ARGS__); \
    } \
} while(0)

#endif /* ERROR_H */
