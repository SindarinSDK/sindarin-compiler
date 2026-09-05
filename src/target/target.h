#ifndef SN_TARGET_H
#define SN_TARGET_H

#include "compiler.h"
#include <stdbool.h>

typedef enum {
    GENERATED_SOURCE,
    GENERATED_HEADER,
    GENERATED_SUPPORT
} GeneratedFileKind;

typedef struct {
    char *relative_path;
    char *contents;
    GeneratedFileKind kind;
} GeneratedFile;

typedef struct {
    GeneratedFile *files;
    int file_count;
    int file_capacity;
    int primary_file;
    void *target_data;
    void (*free_target_data)(void *data);
} GeneratedFileSet;

typedef enum {
    TARGET_EMIT_SINGLE,
    TARGET_EMIT_BUILD
} TargetEmitMode;

typedef struct TargetCompiler {
    TargetKind kind;
    const char *name;
    const char *source_extension;
    const char *template_subdir;

    bool (*check_toolchain)(const CompilerOptions *options);
    bool (*emit)(CompilerOptions *options, Module *module,
                 TargetEmitMode mode, GeneratedFileSet *result);
    bool (*build)(const CompilerOptions *options, const char *build_dir,
                  const GeneratedFileSet *files);
} TargetCompiler;

void generated_file_set_init(GeneratedFileSet *set);
bool generated_file_set_add(GeneratedFileSet *set, const char *relative_path,
                            char *contents, GeneratedFileKind kind, bool primary);
void generated_file_set_free(GeneratedFileSet *set);

const char *target_kind_name(TargetKind kind);
bool target_kind_parse(const char *name, TargetKind *kind);

/* Rust-only driver. C retains its original lifecycle in main.c. */
int rust_target_compile(CompilerOptions *options, Module *module);

#endif
