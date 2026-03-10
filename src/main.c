#include "compiler.h"
#include "diagnostic.h"
#include "debug.h"
#include "type_checker.h"
#include "gcc_backend.h"
#include "version.h"
#include "package.h"
#include "gen_model/gen_model.h"
#include "gen_model/gen_model_render.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include "platform/platform.h"
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #include <windows.h>
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

    /* Handle --init flag (package initialization) */
    if (options.do_init)
    {
        bool success = package_init();
        compiler_cleanup(&options);
        return success ? 0 : 1;
    }

    /* Handle --install flag (package installation) */
    if (options.do_install)
    {
        bool success = package_install(options.install_target);
        compiler_cleanup(&options);
        return success ? 0 : 1;
    }

    /* Handle --clear-cache flag (clear package cache) */
    if (options.clear_cache)
    {
        bool success = package_clear_cache();
        compiler_cleanup(&options);
        return success ? 0 : 1;
    }

    /* Load backend config file if it exists (e.g., sn.gcc.cfg, sn.tcc.cfg) */
    cc_backend_load_config(options.compiler_dir);

    /* Initialize C compiler backend configuration from environment variables */
    cc_backend_init_config(&cc_config);

    /* Check for C compiler availability early (unless --emit-c or --emit-model mode) */
    if (!options.emit_c_only && !options.emit_model && !options.emit_model_c && !options.emit_model_min_c && !options.emit_model_rust)
    {
        if (!gcc_check_available(&cc_config, options.verbose))
        {
            compiler_cleanup(&options);
            return 1;
        }
    }

    /* Package management (unless --no-install is specified) */
    if (!options.no_install)
    {
        /* Synchronize packages with sn.yaml:
         * - Remove orphaned packages not in sn.yaml
         * - Update packages with changed branches
         * - Verify tag packages are at correct SHA */
        if (!package_sync())
        {
            fprintf(stderr, "Warning: Package synchronization had issues\n");
        }

        /* Auto-install dependencies if sn.yaml exists but deps are missing */
        if (package_yaml_exists() && !package_deps_installed())
        {
            printf("Installing missing dependencies...\n");
            if (!package_install_all())
            {
                fprintf(stderr, "Warning: Some dependencies failed to install\n");
            }
        }
    }

    /* Compile Sn source to AST */
    module = compiler_compile(&options);

    if (module == NULL) {
        compiler_cleanup(&options);
        return 1;
    }

    /* Handle --emit-model mode: generate JSON model and exit */
    if (options.emit_model)
    {
        diagnostic_phase_start(PHASE_CODE_GEN);

        json_object *model = gen_model_build(&options.arena, module,
                                              &options.symbol_table,
                                              options.arithmetic_mode);
        int write_result = gen_model_write(model, options.output_file);
        json_object_put(model);

        diagnostic_phase_done(PHASE_CODE_GEN, 0);

        if (write_result != 0)
        {
            compiler_cleanup(&options);
            return 1;
        }

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

    /* Handle --emit-model-c mode: generate C via JSON model + Handlebars templates */
    if (options.emit_model_c)
    {
        diagnostic_phase_start(PHASE_CODE_GEN);

        json_object *model = gen_model_build(&options.arena, module,
                                              &options.symbol_table,
                                              options.arithmetic_mode);

        /* Find templates directory relative to compiler binary */
        char template_dir[1024];
        snprintf(template_dir, sizeof(template_dir), "%s/templates/c", options.compiler_dir);

        char *c_code = gen_model_render_c(model, template_dir);
        json_object_put(model);

        if (!c_code)
        {
            fprintf(stderr, "Error: template rendering failed\n");
            diagnostic_phase_failed(PHASE_CODE_GEN);
            compiler_cleanup(&options);
            return 1;
        }

        /* Write C code to output file (binary mode for consistent line endings) */
        FILE *out = fopen(options.output_file, "wb");
        if (!out)
        {
            fprintf(stderr, "Error: cannot open output file: %s\n", options.output_file);
            free(c_code);
            diagnostic_phase_failed(PHASE_CODE_GEN);
            compiler_cleanup(&options);
            return 1;
        }
        fputs(c_code, out);
        fclose(out);
        free(c_code);

        diagnostic_phase_done(PHASE_CODE_GEN, 0);

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

    /* Handle --emit-model-min-c mode: generate minimal C via JSON model + Handlebars templates */
    if (options.emit_model_min_c)
    {
        diagnostic_phase_start(PHASE_CODE_GEN);

        json_object *model = gen_model_build(&options.arena, module,
                                              &options.symbol_table,
                                              options.arithmetic_mode);

        /* Find templates directory relative to compiler binary */
        char template_dir[1024];
        snprintf(template_dir, sizeof(template_dir), "%s/templates/c-min", options.compiler_dir);

        char *c_code = gen_model_render_min_c(model, template_dir);
        json_object_put(model);

        if (!c_code)
        {
            fprintf(stderr, "Error: minimal C template rendering failed\n");
            diagnostic_phase_failed(PHASE_CODE_GEN);
            compiler_cleanup(&options);
            return 1;
        }

        /* Write C code to output file (binary mode for consistent line endings) */
        FILE *out = fopen(options.output_file, "wb");
        if (!out)
        {
            fprintf(stderr, "Error: cannot open output file: %s\n", options.output_file);
            free(c_code);
            diagnostic_phase_failed(PHASE_CODE_GEN);
            compiler_cleanup(&options);
            return 1;
        }
        fputs(c_code, out);
        fclose(out);
        free(c_code);

        diagnostic_phase_done(PHASE_CODE_GEN, 0);

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

    /* Handle --emit-model-rust mode: generate Rust via JSON model + Handlebars templates */
    if (options.emit_model_rust)
    {
        diagnostic_phase_start(PHASE_CODE_GEN);

        json_object *model = gen_model_build(&options.arena, module,
                                              &options.symbol_table,
                                              options.arithmetic_mode);

        /* Find templates directory relative to compiler binary */
        char template_dir[1024];
        snprintf(template_dir, sizeof(template_dir), "%s/templates/rust", options.compiler_dir);

        char *rust_code = gen_model_render_rust(model, template_dir);
        json_object_put(model);

        if (!rust_code)
        {
            fprintf(stderr, "Error: Rust template rendering failed\n");
            diagnostic_phase_failed(PHASE_CODE_GEN);
            compiler_cleanup(&options);
            return 1;
        }

        /* Write Rust code to output file (binary mode for consistent line endings) */
        FILE *out = fopen(options.output_file, "wb");
        if (!out)
        {
            fprintf(stderr, "Error: cannot open output file: %s\n", options.output_file);
            free(rust_code);
            diagnostic_phase_failed(PHASE_CODE_GEN);
            compiler_cleanup(&options);
            return 1;
        }
        fputs(rust_code, out);
        fclose(out);
        free(rust_code);

        diagnostic_phase_done(PHASE_CODE_GEN, 0);

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

    /* Phase 3: Code generation */
    diagnostic_phase_start(PHASE_CODE_GEN);

    if (options.codegen_mode == 2 || options.codegen_mode == 3)
    {
        /* Model-based codegen: JSON model + Handlebars templates */
        json_object *model = gen_model_build(&options.arena, module,
                                              &options.symbol_table,
                                              options.arithmetic_mode);

        char template_dir[1024];
        if (options.codegen_mode == 3)
            snprintf(template_dir, sizeof(template_dir), "%s/templates/c-min", options.compiler_dir);
        else
            snprintf(template_dir, sizeof(template_dir), "%s/templates/c", options.compiler_dir);

        char *c_code = (options.codegen_mode == 3)
            ? gen_model_render_min_c(model, template_dir)
            : gen_model_render_c(model, template_dir);
        json_object_put(model);

        if (!c_code)
        {
            fprintf(stderr, "Error: template rendering failed\n");
            diagnostic_phase_failed(PHASE_CODE_GEN);
            compiler_cleanup(&options);
            return 1;
        }

        FILE *out = fopen(options.output_file, "wb");
        if (!out)
        {
            fprintf(stderr, "Error: cannot open output file: %s\n", options.output_file);
            free(c_code);
            diagnostic_phase_failed(PHASE_CODE_GEN);
            compiler_cleanup(&options);
            return 1;
        }
        fputs(c_code, out);
        fclose(out);
        free(c_code);
    }
    else
    {
        /* Legacy codegen */
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
    }

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
                     options.compiler_dir, options.verbose,
                     options.debug_build, options.profile_build,
                     options.link_libs, options.link_lib_count,
                     options.source_files, options.source_file_count,
                     options.codegen_mode))
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
