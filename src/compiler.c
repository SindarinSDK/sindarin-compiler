#include "compiler.h"
#include "diagnostic.h"
#include "debug.h"
#include "type_checker.h"
#include "optimizer.h"
#include "gcc_backend.h"
#include "version.h"
#include <stdlib.h>
#include <string.h>

void compiler_init(CompilerOptions *options, int argc, char **argv)
{
    if (!options)
    {
        DEBUG_ERROR("CompilerOptions is NULL");
        exit(1);
    }

    arena_init(&options->arena, 4096);
    options->source_file = NULL;
    options->output_file = NULL;
    options->executable_file = NULL;
    options->source = NULL;
    options->compiler_dir = NULL;
    options->verbose = 0;
    options->log_level = DEBUG_LEVEL_ERROR;
    options->arithmetic_mode = ARITH_CHECKED;
    options->optimization_level = OPT_LEVEL_FULL;
    options->emit_c = 0;
    options->emit_model = 0;
    options->keep_c = 0;
    options->debug_build = 0;
    options->do_init = 0;
    options->do_install = 0;
    options->install_target = NULL;
    options->clear_cache = 0;
    options->do_clean = 0;
    options->no_install = 0;

    options->compiler_dir = (char *)gcc_get_compiler_dir(argv[0]);

    if (!compiler_parse_args(argc, argv, options))
    {
        compiler_cleanup(options);
        exit(1);
    }

    symbol_table_init(&options->arena, &options->symbol_table);
}

void compiler_cleanup(CompilerOptions *options)
{
    if (!options) return;

    symbol_table_cleanup(&options->symbol_table);
    arena_free(&options->arena);

    options->source_file = NULL;
    options->output_file = NULL;
    options->executable_file = NULL;
    options->source = NULL;
    options->compiler_dir = NULL;
}

int compiler_parse_args(int argc, char **argv, CompilerOptions *options)
{
    /* Check for standalone commands first */
    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "--version") == 0)
        {
            printf("sn %s\n", SN_VERSION_STRING);
            exit(0);
        }
        if (strcmp(argv[i], "--init") == 0)
        {
            options->do_init = 1;
            return 1;
        }
        if (strcmp(argv[i], "--install") == 0)
        {
            options->do_install = 1;
            if (i + 1 < argc && argv[i + 1][0] != '-')
                options->install_target = arena_strdup(&options->arena, argv[i + 1]);
            return 1;
        }
        if (strcmp(argv[i], "--clear-cache") == 0)
        {
            options->clear_cache = 1;
            return 1;
        }
        if (strcmp(argv[i], "--clean") == 0)
        {
            options->do_clean = 1;
            return 1;
        }
        if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0)
        {
            printf(
                "Sindarin Compiler\n"
                "\n"
                "Usage: %s <source_file> [-o <executable>] [options]\n"
                "\n"
                "Output options:\n"
                "  -o <file>          Specify output executable (default: source_file without extension)\n"
                "  --emit-c           Output generated C code, don't compile to executable\n"
                "  --emit-model       Output JSON model, don't generate C\n"
                "  --keep-c           Keep generated C files after compilation\n"
                "\n"
                "Debug options:\n"
                "  -v                 Verbose mode (show compilation steps)\n"
                "  -g                 Debug build (includes symbols and address sanitizer)\n"
                "  -p                 Profile build (optimized with frame pointers, no ASAN or LTO)\n"
                "  -l <level>         Set log level (0=none, 1=error, 2=warning, 3=info, 4=verbose)\n"
                "\n"
                "Code generation options:\n"
                "  --checked          Force checked arithmetic (overflow detection, slower)\n"
                "  --unchecked        Force unchecked arithmetic (no overflow checking, faster)\n"
                "  -O0                No Sn optimization (for debugging)\n"
                "  -O1                Basic Sn optimizations (dead code elimination, string merging)\n"
                "  -O2                Full Sn optimizations (default: + tail call, unchecked arithmetic)\n"
                "\n"
                "Help:\n"
                "  -h, --help         Show this help message\n"
                "  --version          Show version information\n"
                "\n"
                "Package management:\n"
                "  --init             Initialize a new project (creates sn.yaml)\n"
                "  --install          Install dependencies from sn.yaml\n"
                "  --install <url>    Install a package (e.g., https://github.com/user/lib.git@v1.0)\n"
                "  --clear-cache      Clear the package cache (~/.sn-cache)\n"
                "  --clean            Remove build cache (.sn/build/)\n"
                "  --no-install       Skip automatic dependency installation\n"
                "\n"
                "By default, compiles to an executable and removes the intermediate C files.\n",
                argv[0]);
            exit(0);
        }
    }

    if (argc < 2)
    {
        fprintf(stderr,
            "Usage: %s <source_file> [-o <executable>] [options]\n"
            "Try '%s --help' for more information.\n",
            argv[0], argv[0]);
        return 0;
    }

    /* First pass: parse log level early */
    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "-l") == 0 && i + 1 < argc)
        {
            i++;
            int log_level = atoi(argv[i]);
            if (log_level < DEBUG_LEVEL_NONE || log_level > DEBUG_LEVEL_VERBOSE)
            {
                fprintf(stderr, "Invalid log level: %s (must be 0-4)\n", argv[i]);
                return 0;
            }
            options->log_level = log_level;
            init_debug(log_level);
        }
    }

    int arithmetic_mode_explicit = 0;
    int o2_explicit = 0;

    /* Second pass: parse all arguments */
    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "-o") == 0 && i + 1 < argc)
        {
            i++;
            options->output_file = arena_strdup(&options->arena, argv[i]);
        }
        else if (strcmp(argv[i], "-v") == 0)
        {
            options->verbose = 1;
        }
        else if (strcmp(argv[i], "-l") == 0 && i + 1 < argc)
        {
            i++; /* Already processed */
        }
        else if (strcmp(argv[i], "--unchecked") == 0)
        {
            options->arithmetic_mode = ARITH_UNCHECKED;
            arithmetic_mode_explicit = 1;
        }
        else if (strcmp(argv[i], "--checked") == 0)
        {
            options->arithmetic_mode = ARITH_CHECKED;
            arithmetic_mode_explicit = 1;
        }
        else if (strcmp(argv[i], "-O0") == 0)
        {
            options->optimization_level = OPT_LEVEL_NONE;
        }
        else if (strcmp(argv[i], "-O1") == 0)
        {
            options->optimization_level = OPT_LEVEL_BASIC;
        }
        else if (strcmp(argv[i], "-O2") == 0)
        {
            options->optimization_level = OPT_LEVEL_FULL;
            o2_explicit = 1;
        }
        else if (strcmp(argv[i], "--emit-c") == 0)
        {
            options->emit_c = 1;
        }
        else if (strcmp(argv[i], "--emit-model") == 0)
        {
            options->emit_model = 1;
        }
        else if (strcmp(argv[i], "--keep-c") == 0)
        {
            options->keep_c = 1;
        }
        else if (strcmp(argv[i], "-g") == 0)
        {
            options->debug_build = 1;
        }
        else if (strcmp(argv[i], "-p") == 0)
        {
            options->profile_build = 1;
        }
        else if (strcmp(argv[i], "--no-install") == 0)
        {
            options->no_install = 1;
        }
        else if (argv[i][0] == '-')
        {
            DEBUG_ERROR("Unknown option: %s", argv[i]);
            return 0;
        }
        else
        {
            if (options->source_file != NULL)
            {
                DEBUG_ERROR("Multiple source files specified: %s and %s", options->source_file, argv[i]);
                return 0;
            }
            options->source_file = arena_strdup(&options->arena, argv[i]);
        }
    }

    /* Explicit -O2 defaults to unchecked arithmetic */
    if (o2_explicit && !arithmetic_mode_explicit)
        options->arithmetic_mode = ARITH_UNCHECKED;

    /* Validate conflicting flags */
    if (options->profile_build && options->debug_build)
    {
        fprintf(stderr, "Error: -p (profile) and -g (debug) cannot be used together\n");
        return 0;
    }

    if (options->source_file == NULL)
    {
        fprintf(stderr, "Error: No source file specified\n");
        return 0;
    }

    /* Determine output paths */
    const char *dot = strrchr(options->source_file, '.');
    size_t base_len = dot ? (size_t)(dot - options->source_file) : strlen(options->source_file);

    if (options->emit_model)
    {
        /* --emit-model: output JSON */
        if (options->output_file == NULL)
        {
            char *out = arena_alloc(&options->arena, base_len + 6);
            strncpy(out, options->source_file, base_len);
            strcpy(out + base_len, ".json");
            options->output_file = out;
        }
        options->executable_file = NULL;
    }
    else if (options->emit_c)
    {
        /* --emit-c: output C file */
        if (options->output_file == NULL)
        {
            char *out = arena_alloc(&options->arena, base_len + 3);
            strncpy(out, options->source_file, base_len);
            strcpy(out + base_len, ".c");
            options->output_file = out;
        }
        options->executable_file = NULL;
    }
    else
    {
        /* Normal mode: compile to executable */
        if (options->output_file != NULL)
        {
            options->executable_file = options->output_file;
        }
        else
        {
            char *exe = arena_alloc(&options->arena, base_len + 1);
            strncpy(exe, options->source_file, base_len);
            exe[base_len] = '\0';
            options->executable_file = exe;
        }
        options->output_file = NULL;
    }

    return 1;
}

Module* compiler_compile(CompilerOptions *options)
{
    char **imported = NULL;
    Module **imported_modules = NULL;
    bool *imported_directly = NULL;
    bool *namespace_code_emitted = NULL;
    int imported_count = 0;
    int imported_capacity = 0;

    char *source = file_read(&options->arena, options->source_file);
    if (!source)
    {
        diagnostic_error_simple("cannot read file '%s'", options->source_file);
        return NULL;
    }
    options->source = source;

    diagnostic_init(options->source_file, source);
    diagnostic_set_verbose(options->verbose);
    diagnostic_compile_start(options->source_file);

    /* Phase 1: Parsing */
    diagnostic_phase_start(PHASE_PARSING);
    Module *module = parse_module_with_imports(&options->arena, &options->symbol_table,
        options->source_file, &imported, &imported_count, &imported_capacity,
        &imported_modules, &imported_directly, &namespace_code_emitted, options->compiler_dir);
    if (!module)
    {
        diagnostic_phase_failed(PHASE_PARSING);
        diagnostic_compile_failed();
        return NULL;
    }
    diagnostic_phase_done(PHASE_PARSING, 0);

    /* Phase 2: Type checking */
    diagnostic_phase_start(PHASE_TYPE_CHECK);
    if (!type_check_module(module, &options->symbol_table))
    {
        diagnostic_phase_failed(PHASE_TYPE_CHECK);
        diagnostic_compile_failed();
        return NULL;
    }
    diagnostic_phase_done(PHASE_TYPE_CHECK, 0);

    /* Optimization */
    if (options->optimization_level >= OPT_LEVEL_BASIC)
    {
        Optimizer opt;
        optimizer_init(&opt, &options->arena);
        optimizer_dead_code_elimination(&opt, module);
        optimizer_merge_string_literals(&opt, module);

        if (options->optimization_level >= OPT_LEVEL_FULL)
            optimizer_tail_call_optimization(&opt, module);

        if (options->verbose)
        {
            int stmts_removed, vars_removed, noops_removed;
            optimizer_get_stats(&opt, &stmts_removed, &vars_removed, &noops_removed);
            DEBUG_INFO("Optimization level: -O%d", options->optimization_level);
            if (stmts_removed > 0 || vars_removed > 0 || noops_removed > 0)
                DEBUG_INFO("Optimizer: removed %d unreachable statements, %d unused variables, %d no-ops",
                           stmts_removed, vars_removed, noops_removed);
            if (options->optimization_level >= OPT_LEVEL_FULL && opt.tail_calls_optimized > 0)
                DEBUG_INFO("Optimizer: marked %d tail calls for optimization", opt.tail_calls_optimized);
            if (opt.string_literals_merged > 0)
                DEBUG_INFO("Optimizer: merged %d adjacent string literals", opt.string_literals_merged);
        }
    }
    else if (options->verbose)
    {
        DEBUG_INFO("Optimization disabled (-O0)");
    }

    return module;
}
