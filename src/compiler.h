#ifndef COMPILER_H
#define COMPILER_H

#include "arena.h"
#include "file.h"
#include "lexer.h"
#include "parser.h"
#include <stdio.h>
#include <stdbool.h>

/* Arithmetic mode for code generation */
typedef enum {
    ARITH_CHECKED,     /* Use runtime functions with overflow checking (default) */
    ARITH_UNCHECKED    /* Use native C operators without overflow checking */
} ArithmeticMode;

/* Optimization levels */
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
    int emit_c;                      /* --emit-c: Output generated C code, don't compile */
    int emit_model;                  /* --emit-model: Output JSON model, don't generate C */
    int keep_c;                      /* --keep-c: Keep generated C files after compilation */
    int debug_build;                 /* -g: Include debug symbols and sanitizers in GCC output */
    int profile_build;               /* -p: Profile build (optimized with frame pointers, no ASAN/LTO) */
    int do_init;                     /* --init: Initialize new package */
    int do_install;                  /* --install: Install packages */
    char *install_target;            /* Package URL@ref for --install */
    int clear_cache;                 /* --clear-cache: Clear package cache */
    int do_clean;                    /* --clean: Remove build cache (.sn/build/) and exit */
    int no_install;                  /* --no-install: Skip auto-install of dependencies */
    int do_format;                   /* --format: Format all .sn files recursively */
    int format_check;                /* --check: Check formatting only (don't modify files) */
} CompilerOptions;

void compiler_init(CompilerOptions *options, int argc, char **argv);
void compiler_cleanup(CompilerOptions *options);
int compiler_parse_args(int argc, char **argv, CompilerOptions *options);
Module* compiler_compile(CompilerOptions *options);

#endif
