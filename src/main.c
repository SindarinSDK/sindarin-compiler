#include "compiler.h"
#include "diagnostic.h"
#include "debug.h"
#include "package.h"
#include "formatter.h"
#include "cgen/gen_model.h"
#include "target/target.h"
#include <stdio.h>
#include <stdlib.h>

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

static void report_success(const char *path)
{
    struct stat st;
    long file_size = 0;
    if (stat(path, &st) == 0) file_size = st.st_size;
    diagnostic_compile_success(path, file_size, 0);
}

/* ============================================================================
 * Main entry point
 * ============================================================================ */

int main(int argc, char **argv)
{
    CompilerOptions options;

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
    if (options.do_format)
    {
        int result = formatter_format_directory(".", options.format_check);
        compiler_cleanup(&options);
        if (result < 0) return 1;
        if (options.format_check && result > 0)
        {
            printf("\n%d file(s) would be reformatted.\n", result);
            return 1;
        }
        if (result == 0)
            printf("All files already formatted.\n");
        else
            printf("\n%d file(s) reformatted.\n", result);
        return 0;
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

    if (options.output_kind == OUTPUT_MODEL)
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

    int result = target_compile(&options, module);
    compiler_cleanup(&options);
    return result;
}
