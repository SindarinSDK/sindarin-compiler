#include "compiler.h"
#include "diagnostic.h"
#include "debug.h"
#include "type_checker.h"
#include "gcc_backend.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include "platform/platform.h"
    #if defined(__MINGW32__) || defined(__MINGW64__)
    /* MinGW is POSIX-compatible */
    #include <unistd.h>
    #include <sys/stat.h>
    #endif
#else
#include <unistd.h>
#include <sys/stat.h>
#endif

int main(int argc, char **argv)
{
    CompilerOptions options;
    CCBackendConfig cc_config;
    Module *module = NULL;
    int result = 0;

    compiler_init(&options, argc, argv);
    init_debug(options.log_level);

    /* Load backend config file if it exists (e.g., sn.gcc.cfg, sn.tcc.cfg) */
    cc_backend_load_config(options.compiler_dir);

    /* Initialize C compiler backend configuration from environment variables */
    cc_backend_init_config(&cc_config);

    /* Check for C compiler availability early (unless --emit-c mode) */
    if (!options.emit_c_only)
    {
        if (!gcc_check_available(&cc_config, options.verbose))
        {
            compiler_cleanup(&options);
            return 1;
        }
    }

    /* Compile Sn source to AST */
    module = compiler_compile(&options);

    if (module == NULL) {
        compiler_cleanup(&options);
        return 1;
    }

    /* Phase 3: Code generation */
    diagnostic_phase_start(PHASE_CODE_GEN);

    CodeGen gen;
    code_gen_init(&options.arena, &gen, &options.symbol_table, options.output_file);
    gen.arithmetic_mode = options.arithmetic_mode;
    code_gen_module(&gen, module);

    /* Copy link libraries and source files from CodeGen to options for C backend */
    options.link_libs = gen.pragma_links;
    options.link_lib_count = gen.pragma_link_count;
    options.source_files = gen.pragma_sources;
    options.source_file_count = gen.pragma_source_count;

    code_gen_cleanup(&gen);

    diagnostic_phase_done(PHASE_CODE_GEN, 0);

    /* If --emit-c mode, we're done */
    if (options.emit_c_only)
    {
        /* Get file size for the success message */
        struct stat st;
        long file_size = 0;
        if (stat(options.output_file, &st) == 0)
        {
            file_size = st.st_size;
        }
        diagnostic_compile_success(options.output_file, file_size, 0);
        compiler_cleanup(&options);
        return 0;
    }

    /* Phase 4: Linking (C compilation) */
    diagnostic_phase_start(PHASE_LINKING);

    /* Validate pragma source files before passing to C compiler */
    if (!gcc_validate_pragma_sources(options.source_files, options.source_file_count,
                                      options.verbose))
    {
        diagnostic_phase_failed(PHASE_LINKING);
        diagnostic_compile_failed();
        compiler_cleanup(&options);
        return 1;
    }

    if (!gcc_compile(&cc_config, options.output_file, options.executable_file,
                     options.compiler_dir, options.verbose, options.debug_build,
                     options.link_libs, options.link_lib_count,
                     options.source_files, options.source_file_count))
    {
        diagnostic_phase_failed(PHASE_LINKING);
        diagnostic_compile_failed();
        result = 1;
    }
    else
    {
        diagnostic_phase_done(PHASE_LINKING, 0);

        /* Get file size for the success message */
        struct stat st;
        long file_size = 0;
        if (stat(options.executable_file, &st) == 0)
        {
            file_size = st.st_size;
        }
        diagnostic_compile_success(options.executable_file, file_size, 0);
    }

    /* Delete intermediate C file unless --keep-c was specified */
    if (!options.keep_c && result == 0)
    {
        if (unlink(options.output_file) != 0)
        {
            /* Non-fatal: just warn if we couldn't delete */
            DEBUG_WARNING("Could not remove intermediate C file: %s", options.output_file);
        }
    }

    compiler_cleanup(&options);

    return result;
}
