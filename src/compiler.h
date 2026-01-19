#ifndef COMPILER_H
#define COMPILER_H

#include "arena.h"
#include "file.h"
#include "lexer.h"
#include "parser.h"
#include "code_gen.h"
#include <stdio.h>
#include <stdbool.h>

/* Optimization levels:
 * -O0: No optimization (for debugging, generates simpler code)
 * -O1: Basic optimizations (dead code elimination, string literal merging)
 * -O2: Full optimizations (+ tail call optimization, constant folding)
 */
#define OPT_LEVEL_NONE  0  /* -O0: No optimization */
#define OPT_LEVEL_BASIC 1  /* -O1: Basic optimizations */
#define OPT_LEVEL_FULL  2  /* -O2: Full optimizations (default) */

typedef struct
{
    Arena arena;
    SymbolTable symbol_table;
    char *source_file;
    char *output_file;
    char *executable_file;           /* Output executable path (derived or explicit) */
    char *source;
    char *compiler_dir;              /* Directory containing compiler and runtime objects */
    int verbose;
    int log_level;
    ArithmeticMode arithmetic_mode;  /* Checked or unchecked arithmetic */
    int optimization_level;          /* Optimization level (0, 1, or 2) */
    int emit_c_only;                 /* --emit-c: Only output C code, don't invoke GCC */
    int keep_c;                      /* --keep-c: Keep intermediate C file after compilation */
    int debug_build;                 /* -g: Include debug symbols and sanitizers in GCC output */
    char **link_libs;                /* Libraries to link from #pragma link directives */
    int link_lib_count;              /* Number of link libraries */
    PragmaSourceInfo *source_files;  /* C source files with location info from #pragma source */
    int source_file_count;           /* Number of source files */
} CompilerOptions;

void compiler_init(CompilerOptions *options, int argc, char **argv);
void compiler_cleanup(CompilerOptions *options);
int compiler_parse_args(int argc, char **argv, CompilerOptions *options);
Module* compiler_compile(CompilerOptions *options);

#endif