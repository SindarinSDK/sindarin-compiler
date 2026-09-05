#include "target/target.h"
#include "diagnostic.h"
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#ifdef _WIN32
#include <direct.h>
#include <io.h>
#include <process.h>
#define unlink _unlink
#else
#include <unistd.h>
#endif

extern const TargetCompiler sn_rust_target;

void generated_file_set_init(GeneratedFileSet *set)
{
    memset(set, 0, sizeof(*set));
    set->primary_file = -1;
}

bool generated_file_set_add(GeneratedFileSet *set, const char *relative_path,
                            char *contents, GeneratedFileKind kind, bool primary)
{
    if (!set || !relative_path || !contents) return false;
    if (set->file_count == set->file_capacity)
    {
        int capacity = set->file_capacity == 0 ? 4 : set->file_capacity * 2;
        GeneratedFile *files = realloc(set->files, sizeof(GeneratedFile) * (size_t)capacity);
        if (!files) return false;
        set->files = files;
        set->file_capacity = capacity;
    }

    GeneratedFile *file = &set->files[set->file_count];
    file->relative_path = strdup(relative_path);
    if (!file->relative_path) return false;
    file->contents = contents;
    file->kind = kind;
    if (primary) set->primary_file = set->file_count;
    set->file_count++;
    return true;
}

void generated_file_set_free(GeneratedFileSet *set)
{
    if (!set) return;
    for (int i = 0; i < set->file_count; i++)
    {
        free(set->files[i].relative_path);
        free(set->files[i].contents);
    }
    free(set->files);
    if (set->free_target_data)
        set->free_target_data(set->target_data);
    generated_file_set_init(set);
}

const char *target_kind_name(TargetKind kind)
{
    switch (kind)
    {
        case TARGET_C: return "c";
        case TARGET_RUST: return "rust";
    }
    return "unknown";
}

bool target_kind_parse(const char *name, TargetKind *kind)
{
    if (!name || !kind) return false;
    if (strcmp(name, "c") == 0)
        *kind = TARGET_C;
    else if (strcmp(name, "rust") == 0 || strcmp(name, "rs") == 0)
        *kind = TARGET_RUST;
    else
        return false;
    return true;
}

static bool write_file(const char *path, const char *content)
{
    FILE *file = fopen(path, "wb");
    if (!file)
    {
        fprintf(stderr, "Error: cannot open output file '%s': %s\n", path, strerror(errno));
        return false;
    }
    bool ok = fputs(content, file) >= 0;
    if (fclose(file) != 0) ok = false;
    if (!ok)
        fprintf(stderr, "Error: cannot write output file '%s'\n", path);
    return ok;
}

static bool ensure_directory(const char *path)
{
#ifdef _WIN32
    if (_mkdir(path) == 0 || errno == EEXIST) return true;
#else
    if (mkdir(path, 0755) == 0 || errno == EEXIST) return true;
#endif
    fprintf(stderr, "Error: cannot create build directory '%s': %s\n", path, strerror(errno));
    return false;
}

static bool get_build_dir(const CompilerOptions *options, const TargetCompiler *target,
                          char *build_dir, size_t build_dir_size)
{
    const char *slash = strrchr(options->source_file, '/');
#ifdef _WIN32
    const char *backslash = strrchr(options->source_file, '\\');
    if (backslash && (!slash || backslash > slash)) slash = backslash;
#endif
    const char *base = slash ? slash + 1 : options->source_file;
    const char *dot = strrchr(base, '.');
    size_t base_len = dot ? (size_t)(dot - base) : strlen(base);
    if (base_len > 200) base_len = 200;

    char base_name[256];
    memcpy(base_name, base, base_len);
    base_name[base_len] = '\0';
#ifdef _WIN32
    int pid = _getpid();
#else
    int pid = (int)getpid();
#endif
    int written = snprintf(build_dir, build_dir_size, ".sn/build/%s/%s_%d",
                           target->name, base_name, pid);
    return written >= 0 && (size_t)written < build_dir_size;
}

static bool ensure_build_dir(const TargetCompiler *target, const char *build_dir)
{
    char target_dir[PATH_MAX];
    if (snprintf(target_dir, sizeof(target_dir), ".sn/build/%s", target->name) >= (int)sizeof(target_dir))
        return false;
    return ensure_directory(".sn") && ensure_directory(".sn/build") &&
           ensure_directory(target_dir) && ensure_directory(build_dir);
}

static bool write_generated_files(const char *build_dir, const GeneratedFileSet *files)
{
    for (int i = 0; i < files->file_count; i++)
    {
        char path[PATH_MAX];
        int written = snprintf(path, sizeof(path), "%s/%s", build_dir,
                               files->files[i].relative_path);
        if (written < 0 || written >= (int)sizeof(path) ||
            !write_file(path, files->files[i].contents))
            return false;
    }
    return true;
}

static void remove_generated_files(const char *build_dir, const GeneratedFileSet *files)
{
    for (int i = 0; i < files->file_count; i++)
    {
        char path[PATH_MAX];
        int written = snprintf(path, sizeof(path), "%s/%s", build_dir,
                               files->files[i].relative_path);
        if (written >= 0 && written < (int)sizeof(path))
            unlink(path);
    }
}

static void report_success(const char *path)
{
    struct stat st;
    long file_size = 0;
    if (stat(path, &st) == 0) file_size = st.st_size;
    diagnostic_compile_success(path, file_size, 0);
}

int rust_target_compile(CompilerOptions *options, Module *module)
{
    const TargetCompiler *target = &sn_rust_target;

    if (options->output_kind == OUTPUT_EXECUTABLE &&
        (!target->check_toolchain || !target->check_toolchain(options)))
        return 1;

    GeneratedFileSet files;
    generated_file_set_init(&files);

    diagnostic_phase_start(PHASE_CODE_GEN);
    TargetEmitMode emit_mode = options->output_kind == OUTPUT_SOURCE
        ? TARGET_EMIT_SINGLE : TARGET_EMIT_BUILD;
    if (!target->emit(options, module, emit_mode, &files))
    {
        diagnostic_phase_failed(PHASE_CODE_GEN);
        generated_file_set_free(&files);
        return 1;
    }

    if (options->output_kind == OUTPUT_SOURCE)
    {
        int primary = files.primary_file >= 0 ? files.primary_file : 0;
        if (files.file_count == 0 || !write_file(options->output_file, files.files[primary].contents))
        {
            diagnostic_phase_failed(PHASE_CODE_GEN);
            generated_file_set_free(&files);
            return 1;
        }
        diagnostic_phase_done(PHASE_CODE_GEN, 0);
        report_success(options->output_file);
        generated_file_set_free(&files);
        return 0;
    }

    char build_dir[PATH_MAX];
    if (!get_build_dir(options, target, build_dir, sizeof(build_dir)) ||
        !ensure_build_dir(target, build_dir) || !write_generated_files(build_dir, &files))
    {
        diagnostic_phase_failed(PHASE_CODE_GEN);
        generated_file_set_free(&files);
        return 1;
    }
    diagnostic_phase_done(PHASE_CODE_GEN, 0);

    diagnostic_phase_start(PHASE_LINKING);
    bool build_ok = target->build && target->build(options, build_dir, &files);
    if (!build_ok)
    {
        diagnostic_phase_failed(PHASE_LINKING);
        diagnostic_compile_failed();
        generated_file_set_free(&files);
        return 1;
    }
    diagnostic_phase_done(PHASE_LINKING, 0);
    report_success(options->executable_file);

    if (!options->keep_generated)
        remove_generated_files(build_dir, &files);
    generated_file_set_free(&files);
    return 0;
}
