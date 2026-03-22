#include "compiler.h"
#include "diagnostic.h"
#include "debug.h"
#include "type_checker.h"
#include "gcc_backend.h"
#include "version.h"
#include "package.h"
#include "cgen/gen_model.h"
#include "cgen/gen_model_render.h"
#include "cgen/gen_model_split.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#ifdef _WIN32
#include <process.h>
#define getpid _getpid
#else
#include <unistd.h>
#endif

#ifdef _WIN32
#include "platform/platform.h"
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #include <windows.h>
    #if defined(__MINGW32__) || defined(__MINGW64__)
    #include <unistd.h>
    #include <sys/stat.h>
    #endif
#else
#include <unistd.h>
#include <sys/stat.h>
#endif

/* ---- Helpers ---- */

static void report_success(const char *path)
{
    struct stat st;
    long file_size = 0;
    if (stat(path, &st) == 0)
        file_size = st.st_size;
    diagnostic_compile_success(path, file_size, 0);
}

static bool write_file(const char *path, const char *content)
{
    FILE *f = fopen(path, "wb");
    if (!f)
    {
        fprintf(stderr, "Error: cannot open output file: %s\n", path);
        return false;
    }
    fputs(content, f);
    fclose(f);
    return true;
}

static void get_build_dir(const char *source_file, char *build_dir, size_t build_dir_size)
{
    const char *slash = strrchr(source_file, '/');
#ifdef _WIN32
    const char *bslash = strrchr(source_file, '\\');
    if (bslash && (!slash || bslash > slash)) slash = bslash;
#endif
    const char *base = slash ? slash + 1 : source_file;
    const char *dot = strrchr(base, '.');
    int blen = dot ? (int)(dot - base) : (int)strlen(base);
    if (blen > 200) blen = 200;
    char base_name[256];
    memcpy(base_name, base, blen);
    base_name[blen] = '\0';
    /* Include PID so parallel compilations of the same source don't collide */
    snprintf(build_dir, build_dir_size, ".sn/build/%s_%d", base_name, (int)getpid());
}

static void ensure_build_dir(const char *build_dir)
{
#ifdef _WIN32
    mkdir(".sn");
    mkdir(".sn/build");
    mkdir(build_dir);
#else
    mkdir(".sn", 0755);
    mkdir(".sn/build", 0755);
    mkdir(build_dir, 0755);
#endif
}

/* ============================================================================
 * Compilation pipeline
 *
 * AST → JSON model → flatten chains → split by source file
 *     → render sn_types.h + per-module .c files → compile each to .o → link
 *
 * Build artifacts go to .sn/build/<source_basename>/
 * ============================================================================ */

static int compile_to_executable(CompilerOptions *options, CCBackendConfig *cc_config, Module *module)
{
    /* Build JSON model from typed AST */
    json_object *model = gen_model_build(&options->arena, module,
                                          &options->symbol_table,
                                          options->arithmetic_mode);
    gen_model_flatten_chains(model);

    /* Split model into per-source-file modules */
    ModularModel *split = gen_model_split(model, options->source_file);
    json_object_put(model);
    if (!split)
    {
        fprintf(stderr, "Error: model splitting failed\n");
        return 1;
    }

    /* Render common header + per-module .c files */
    char template_dir[1024];
    snprintf(template_dir, sizeof(template_dir), "%s/templates/c", options->compiler_dir);

    register_helpers_fn reg_fn = gen_model_get_min_c_register_fn();
    ModularRenderResult *rendered = gen_model_render_modular_min_c(split, template_dir, reg_fn);
    if (!rendered)
    {
        fprintf(stderr, "Error: modular rendering failed\n");
        modular_model_free(split);
        return 1;
    }

    /* Write generated files to .sn/build/<source_basename>_<pid>/
     * The PID suffix ensures parallel compilations don't collide. */
    char build_dir[PATH_MAX];
    get_build_dir(options->source_file, build_dir, sizeof(build_dir));
    ensure_build_dir(build_dir);

    char header_path[PATH_MAX];
    snprintf(header_path, sizeof(header_path), "%s/sn_types.h", build_dir);
    if (!write_file(header_path, rendered->header_code))
    {
        modular_render_result_free(rendered);
        modular_model_free(split);
        return 1;
    }

    for (int i = 0; i < rendered->impl_count; i++)
    {
        char c_path[PATH_MAX];
        snprintf(c_path, sizeof(c_path), "%s/%s.c", build_dir, rendered->impl_names[i]);
        if (!write_file(c_path, rendered->impl_codes[i]))
        {
            modular_render_result_free(rendered);
            modular_model_free(split);
            return 1;
        }
    }

    fprintf(stderr, "[DBG] files written, phase done\n"); fflush(stderr);
    diagnostic_phase_done(PHASE_CODE_GEN, 0);

    /* Compile each .c → .o, then link all .o → executable */
    diagnostic_phase_start(PHASE_LINKING);

    char *c_filenames[256];
    for (int i = 0; i < rendered->impl_count && i < 256; i++)
    {
        size_t len = strlen(rendered->impl_names[i]) + 3;
        c_filenames[i] = malloc(len);
        snprintf(c_filenames[i], len, "%s.c", rendered->impl_names[i]);
    }

    bool link_ok = gcc_compile_modular(cc_config, build_dir,
                                        (const char **)c_filenames, rendered->impl_count,
                                        options->executable_file, options->compiler_dir,
                                        options->verbose, options->debug_build, options->profile_build,
                                        split->link_libs, split->link_lib_count,
                                        split->source_files, split->source_dirs,
                                        split->source_file_count);

    for (int i = 0; i < rendered->impl_count && i < 256; i++)
        free(c_filenames[i]);

    if (!link_ok)
    {
        diagnostic_phase_failed(PHASE_LINKING);
        diagnostic_compile_failed();
        modular_render_result_free(rendered);
        modular_model_free(split);
        return 1;
    }

    diagnostic_phase_done(PHASE_LINKING, 0);
    report_success(options->executable_file);

    /* Clean up .c files (keep .o for future incremental builds) */
    if (!options->keep_c)
    {
        unlink(header_path);
        for (int i = 0; i < rendered->impl_count; i++)
        {
            char c_path[PATH_MAX];
            snprintf(c_path, sizeof(c_path), "%s/%s.c", build_dir, rendered->impl_names[i]);
            unlink(c_path);
        }
    }

    modular_render_result_free(rendered);
    modular_model_free(split);
    return 0;
}

/* ============================================================================
 * Main entry point
 * ============================================================================ */

int main(int argc, char **argv)
{
    CompilerOptions options;
    CCBackendConfig cc_config;

    compiler_init(&options, argc, argv);
    init_debug(options.log_level);

    /* ---- Standalone commands (no compilation) ---- */

    if (options.do_init)
    {
        bool ok = package_init();
        compiler_cleanup(&options);
        return ok ? 0 : 1;
    }
    if (options.do_install)
    {
        bool ok = package_install(options.install_target);
        compiler_cleanup(&options);
        return ok ? 0 : 1;
    }
    if (options.clear_cache)
    {
        bool ok = package_clear_cache();
        compiler_cleanup(&options);
        return ok ? 0 : 1;
    }
    if (options.do_clean)
    {
        const char *build_dir = ".sn/build";
        struct stat st;
        if (stat(build_dir, &st) == 0)
        {
            printf("Removing build cache: %s\n", build_dir);
            char cmd[256];
            snprintf(cmd, sizeof(cmd), "rm -rf \"%s\"", build_dir);
            if (system(cmd) != 0)
                fprintf(stderr, "Warning: failed to remove build cache\n");
            else
                printf("Build cache cleared.\n");
        }
        else
        {
            printf("Build cache is empty.\n");
        }
        compiler_cleanup(&options);
        return 0;
    }

    /* ---- Compilation setup ---- */

    cc_backend_load_config(options.compiler_dir);
    cc_backend_init_config(&cc_config);

    if (!options.emit_model && !options.emit_c)
    {
        if (!gcc_check_available(&cc_config, options.verbose))
        {
            compiler_cleanup(&options);
            return 1;
        }
    }

    /* Package management */
    if (!options.no_install)
    {
        if (!package_sync())
            fprintf(stderr, "Warning: Package synchronization had issues\n");

        if (package_yaml_exists() && !package_deps_installed())
        {
            printf("Installing missing dependencies...\n");
            if (!package_install_all())
                fprintf(stderr, "Warning: Some dependencies failed to install\n");
        }
    }

    /* ---- Parse, type-check, optimize ---- */

    Module *module = compiler_compile(&options);
    if (!module)
    {
        compiler_cleanup(&options);
        return 1;
    }

    /* ---- Diagnostic emit modes (no executable produced) ---- */

    if (options.emit_model)
    {
        diagnostic_phase_start(PHASE_CODE_GEN);
        json_object *model = gen_model_build(&options.arena, module,
                                              &options.symbol_table, options.arithmetic_mode);
        int wr = gen_model_write(model, options.output_file);
        json_object_put(model);
        diagnostic_phase_done(PHASE_CODE_GEN, 0);
        if (wr != 0) { compiler_cleanup(&options); return 1; }
        report_success(options.output_file);
        compiler_cleanup(&options);
        return 0;
    }

    if (options.emit_c)
    {
        diagnostic_phase_start(PHASE_CODE_GEN);
        json_object *model = gen_model_build(&options.arena, module,
                                              &options.symbol_table, options.arithmetic_mode);
        gen_model_flatten_chains(model);
        char td[1024];
        snprintf(td, sizeof(td), "%s/templates/c", options.compiler_dir);
        char *code = gen_model_render_min_c(model, td);
        json_object_put(model);
        if (!code || !write_file(options.output_file, code))
        { free(code); diagnostic_phase_failed(PHASE_CODE_GEN); compiler_cleanup(&options); return 1; }
        free(code);
        diagnostic_phase_done(PHASE_CODE_GEN, 0);
        report_success(options.output_file);
        compiler_cleanup(&options);
        return 0;
    }

    /* ---- Full compilation to executable ---- */

    diagnostic_phase_start(PHASE_CODE_GEN);
    int result = compile_to_executable(&options, &cc_config, module);
    compiler_cleanup(&options);
    return result;
}
