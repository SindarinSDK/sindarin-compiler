#include "target/target.h"
#include "target/rust/rust_render.h"
#include "target/rust/rust_native.h"
#include "target/rust/projection/rust_model.h"
#include "debug.h"
#include <ctype.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *rustc_command(void)
{
    const char *configured = getenv("SN_RUSTC");
    return configured && configured[0] ? configured : "rustc";
}

static bool rustc_quoted(char *out, size_t out_size)
{
    const char *path = rustc_command();
    size_t len = strlen(path);
    size_t needed;
#ifdef _WIN32
    /* Windows: wrap the executable path in double quotes so it stays exactly
     * one token; a path ending in a backslash needs a doubled backslash before
     * the closing quote. */
    size_t trailing_bs = (len > 0 && path[len - 1] == '\\') ? 1 : 0;
    needed = 2 + len + trailing_bs;
    if (needed >= out_size) return false;
    char *wcursor = out;
    size_t wremaining = out_size;
    *wcursor++ = '"';
    wremaining--;
    for (size_t i = 0; i < len; i++)
    {
        *wcursor++ = path[i];
        wremaining--;
    }
    if (trailing_bs)
    {
        *wcursor++ = '\\';
        wremaining--;
    }
    *wcursor++ = '"';
    wremaining--;
    *wcursor = '\0';
    (void)wremaining;
#else
    /* POSIX: shell-quote the path as one token so spaces and ampersands stay
     * literal. An embedded single quote is encoded with the standard
     * close-quote / escaped-quote / reopen-quote sequence. */
    size_t apostrophes = 0;
    for (size_t i = 0; i < len; i++)
        if (path[i] == '\'')
            apostrophes++;
    needed = 2 + len + 3 * apostrophes;
    if (needed >= out_size) return false;

    char *cursor = out;
    size_t remaining = out_size;
    *cursor++ = '\'';
    remaining--;
    for (size_t i = 0; i < len; i++)
    {
        if (path[i] == '\'')
        {
            static const char seq[4] = { '\'', '\\', '\'', '\'' };
            memcpy(cursor, seq, sizeof(seq));
            cursor += sizeof(seq);
            remaining -= sizeof(seq);
        }
        else
        {
            *cursor++ = path[i];
            remaining--;
        }
    }
    *cursor++ = '\'';
    remaining--;
    *cursor = '\0';
    (void)remaining;
    #endif
    return true;
}

static bool rust_run_command(const char *command)
{
#ifdef _WIN32
    /* Windows cmd.exe /c strips the first opening quote from a command that
     * begins with a quoted executable path (so a spaced SN_RUSTC path splits
     * at the first space). Wrap the complete command in one additional outer
     * pair of double quotes so the executable path stays a single token. */
    size_t len = strlen(command);
    char *to_run = (char *)malloc(len + 3);
    if (!to_run)
    {
        fprintf(stderr, "Error: failed to allocate buffer for rustc invocation\n");
        return false;
    }
    to_run[0] = '"';
    memcpy(to_run + 1, command, len);
    to_run[len + 1] = '"';
    to_run[len + 2] = '\0';
    int status = system(to_run);
    free(to_run);
    return status == 0;
#else
    /* POSIX: pass the command through unchanged; no extra allocation. */
    return system(command) == 0;
#endif
}

static bool rust_check_toolchain(const CompilerOptions *options)
{
    char command[PATH_MAX + 64];
    char quoted_rustc[PATH_MAX + 8];
    if (!rustc_quoted(quoted_rustc, sizeof(quoted_rustc)))
    {
        fprintf(stderr,
                "Error: SN_RUSTC path is too long to shell-quote safely; use --emit-rust.\n");
        return false;
    }
    int written;
#ifdef _WIN32
    written = snprintf(command, sizeof(command), "%s --version >NUL 2>&1", quoted_rustc);
#else
    written = snprintf(command, sizeof(command), "%s --version >/dev/null 2>&1", quoted_rustc);
#endif
    if (written < 0 || (size_t)written >= sizeof(command))
    {
        fprintf(stderr, "Error: failed to build rustc --version command\n");
        return false;
    }
    if (rust_run_command(command))
    {
        if (options->verbose) DEBUG_INFO("Rust compiler '%s' found", rustc_command());
        return true;
    }
    fprintf(stderr, "Error: Rust compiler '%s' is not installed or not in PATH.\n", rustc_command());
    fprintf(stderr, "Set SN_RUSTC to a different compiler, or use --emit-rust.\n");
    return false;
}

/* Private fragments share this translation unit and retain static ownership. */
#include "rust_validate.c"
#include "rust_lower.c"
#include "rust_concurrency.c"
#include "rust_thread_arrays.c"
#include "rust_thread_refs.c"
#include "rust_thread_receivers.c"
#include "rust_lower_float.c"
#include "rust_nullable_strings.c"
#include "rust_nullable_arrays.c"
#include "rust_reference_records.c"
#include "rust_native_record_storage.c"
#include "rust_native_handle_nodes.c"
#include "rust_zero_records.c"

static bool rust_emit(CompilerOptions *options, Module *module,
                      TargetEmitMode mode, GeneratedFileSet *result)
{
    json_object *model = rust_gen_model_build(&options->arena, module,
                                               &options->symbol_table,
                                               options->arithmetic_mode);
    if (!model) return false;
    RustNativePlan *native_plan = NULL;
    if (!rust_native_partition_model(model, options, &native_plan))
    {
        json_object_put(model);
        return false;
    }
    if (mode == TARGET_EMIT_SINGLE && rust_native_plan_has_work(native_plan))
    {
        fprintf(stderr,
                "Error: --emit-rust cannot represent native C bodies, headers, sources, or link options; build the Rust target executable instead\n");
        rust_native_plan_free(native_plan);
        json_object_put(model);
        return false;
    }
    result->target_data = native_plan;
    result->free_target_data = rust_native_plan_free;
    rust_prepare_native_handle_nodes(model, model);
    rust_prepare_zero_record_bindings(model, model);
    rust_prepare_nullable_strings(model);
    rust_prepare_nullable_arrays(model);
    rust_prepare_sizeof_only_declarations(model);
    rust_prepare_thread_receivers(model);
    rust_prepare_reference_record_nodes(model, model, NULL, NULL);
    rust_lower_native_record_storage(model, model);
    if (!rust_prepare_by_value_scalar_parameter_mutations(model))
    {
        json_object_put(model);
        return false;
    }
    rust_prepare_scalar_reference_callables(model);
    if (!rust_prepare_thread_references(model)) { json_object_put(model); return false; }
    if (!rust_validate_model(model, options->arithmetic_mode, native_plan))
    {
        json_object_put(model);
        return false;
    }
    json_object_object_add(model, "rust_sizeof_layouts", json_object_new_array());
    if (!rust_lower_sizeof_layouts(model, model))
    {
        fprintf(stderr, "Error: Rust target could not lower C sizeof layouts\n");
        json_object_put(model);
        return false;
    }
    rust_lower_closures(model);
    rust_lower_byte_arithmetic(model);
    rust_lower_numeric_promotions(model);
    rust_lower_checked_arithmetic(model);
    rust_lower_checked_mutations(model);
    if (rust_model_uses_checked_arithmetic(model))
    {
        json_object_object_add(model, "rust_uses_checked_arithmetic",
                               json_object_new_boolean(true));
        if (!rust_assign_checked_helper_names(model)) { json_object_put(model); return false; }
    }
    rust_lower_floating_mutations(model);
    rust_lower_strings(model);
    if (!rust_specialize_field_array_calls(model))
    {
        fprintf(stderr, "Error: Rust target could not preserve array field call owners\n");
        json_object_put(model);
        return false;
    }
    if (!rust_lower_calls(model))
    {
        fprintf(stderr, "Error: Rust target could not lower default-array call aliases\n");
        json_object_put(model);
        return false;
    }
    if (!rust_lower_byte_encoding(model))
    {
        fprintf(stderr, "Error: Rust target could not assign byte encoding runtime names\n");
        json_object_put(model);
        return false;
    }
    if (!rust_lower_ctype_calls(model))
    {
        fprintf(stderr, "Error: Rust target could not assign character runtime helper names\n");
        json_object_put(model);
        return false;
    }
    if (!rust_lower_sized_default_names(model, model))
    {
        fprintf(stderr, "Error: Rust target could not assign hygienic sized-array names\n");
        json_object_put(model);
        return false;
    }
    if (!rust_lower_assert_temp_names(model, model))
    {
        fprintf(stderr, "Error: Rust target could not assign hygienic assertion temporary names\n");
        json_object_put(model);
        return false;
    }
    if (!rust_lower_string_method_helper_names(model))
    {
        fprintf(stderr, "Error: Rust target could not assign hygienic string helper names\n");
        json_object_put(model);
        return false;
    }
    if (!rust_lower_float_array_equality(model, model))
    {
        fprintf(stderr, "Error: Rust target could not lower floating-array equality\n");
        json_object_put(model);
        return false;
    }
    rust_lower_interpolation_formats(model);
    rust_lower_for_continues(model);
    rust_lower_scalar_ref_parameters(model);
    rust_lower_float_conversions(model, model, NULL);
    size_t place_temp_id = 0;
    if (!rust_lower_member_assignment_places(model, model, &place_temp_id))
    {
        fprintf(stderr, "Error: Rust target could not lower indexed assignment place\n");
        json_object_put(model);
        return false;
    }
    if (!rust_lower_numeric_places(model, model, &place_temp_id))
    {
        fprintf(stderr, "Error: Rust target could not lower computed numeric mutation\n");
        json_object_put(model);
        return false;
    }
    if (!rust_lower_nullable_array_stores(model, model, &place_temp_id))
    { json_object_put(model); return false; }
    size_t match_temp_id = 0;
    if (!rust_lower_match_temp_names(model, model, &match_temp_id))
    {
        fprintf(stderr, "Error: Rust target could not assign hygienic match temporary names\n");
        json_object_put(model);
        return false;
    }
    size_t iterator_temp_id = 0;
    if (!rust_lower_iterator_temp_names(model, model, &iterator_temp_id))
    {
        fprintf(stderr, "Error: Rust target could not assign hygienic iterator temporary names\n");
        json_object_put(model);
        return false;
    }
    if (!rust_assign_array_text_names(model))
    {
        fprintf(stderr, "Error: Rust target could not assign hygienic array text helper names\n");
        json_object_put(model);
        return false;
    }
    size_t array_join_index_id = 0;
    size_t array_join_owner_id = 0;
    if (!rust_assign_array_join_index_names(
            model, model, &array_join_index_id, &array_join_owner_id))
    {
        fprintf(stderr, "Error: Rust target could not assign hygienic array join index names\n");
        json_object_put(model);
        return false;
    }
    if (rust_model_uses_arrays(model))
        json_object_object_add(model, "rust_uses_arrays", json_object_new_boolean(true));
    if (!rust_assign_pointer_slice_helper(model))
    {
        fprintf(stderr, "Error: Rust target could not assign a pointer slice helper name\n");
        json_object_put(model);
        return false;
    }
    if (!rust_assign_windows_text_names(model))
    {
        fprintf(stderr, "Error: Rust target could not assign Windows text helper names\n");
        json_object_put(model);
        return false;
    }
    bool uses_array_text = rust_model_uses_array_text(model);
    if (uses_array_text)
        json_object_object_add(model, "rust_uses_array_text", json_object_new_boolean(true));
    if (rust_model_uses_reflection(model))
        json_object_object_add(model, "rust_uses_reflection", json_object_new_boolean(true));
    if (uses_array_text || rust_model_uses_byte_strings(model))
        json_object_object_add(model, "rust_uses_byte_strings", json_object_new_boolean(true));
    if (rust_model_uses_string_helpers(model))
        json_object_object_add(model, "rust_uses_string_helpers", json_object_new_boolean(true));
    if (rust_model_uses_split_helpers(model))
        json_object_object_add(model, "rust_uses_string_split_helpers", json_object_new_boolean(true));
    if (rust_model_uses_string_format_helpers(model))
        json_object_object_add(model, "rust_uses_string_format_helpers",
                               json_object_new_boolean(true));

    rust_lower_concurrency(model);
    rust_lower_thread_arrays(model);
    rust_lower_thread_receivers(model);
    rust_lower_native_record_storage(model, model);

    char template_dir[1024];
    snprintf(template_dir, sizeof(template_dir), "%s/templates/rust", options->compiler_dir);
    size_t native_temporary_id = 0;
    rust_lower_native_handle_temporaries(model, model, &native_temporary_id);
    rust_prepare_native_handle_nodes(model, model);
    rust_lower_native_handle_reads(model, model, false);
    /* Serialization bodies are generated after record storage lowering. */
    json_object *serial_structs = rust_nullable_child(model, "structs");
    for (size_t i = 0; i < json_object_array_length(serial_structs); i++) {
        json_object *structure = json_object_array_get_idx(serial_structs, i);
        if (!json_boolean_property(structure, "rust_serializable")) continue;
        json_object *methods = rust_nullable_child(structure, "methods");
        for (size_t m = 0; m < json_object_array_length(methods); m++) {
            json_object *method = json_object_array_get_idx(methods, m);
            if (!json_boolean_property(method, "is_serializable_method")) continue;
            json_object_object_add(method, "rust_serial_thread_fields", json_object_new_boolean(
                json_boolean_property(structure, "rust_thread_fields")));
            json_object_object_add(method, "rust_serial_reference_identity", json_object_new_boolean(
                json_boolean_property(structure, "rust_thread_reference_identity")));
        }
    }
    if (!rust_lower_physical_receivers(model)) { json_object_put(model); return false; }
    char *code = rust_render_model(model, template_dir);
    json_object_put(model);
    if (!code) return false;
    if (!generated_file_set_add(result, "main.rs", code, GENERATED_SOURCE, true))
    {
        free(code);
        return false;
    }
    return rust_native_emit_support(native_plan, result, options->compiler_dir);
}

static bool rust_build(const CompilerOptions *options, const char *build_dir,
                       const GeneratedFileSet *files)
{
    RustNativePlan *native_plan = files->target_data;
    if (rust_native_plan_has_work(native_plan))
        return rust_native_build(options, build_dir, files, native_plan);
    if (files->primary_file < 0) return false;
    char source_path[PATH_MAX];
    snprintf(source_path, sizeof(source_path), "%s/%s", build_dir,
             files->files[files->primary_file].relative_path);

    const char *rustflags = getenv("SN_RUSTFLAGS");
    if (!rustflags) rustflags = "";
    const char *profile_flags = options->debug_build
        ? "-C debuginfo=2 -C opt-level=0"
        : options->profile_build
            ? "-C debuginfo=1 -C opt-level=3 -C force-frame-pointers=yes"
            : "-C opt-level=3";

    char command[PATH_MAX * 3];
    char quoted_rustc[PATH_MAX + 8];
    if (!rustc_quoted(quoted_rustc, sizeof(quoted_rustc)))
    {
        fprintf(stderr,
                "Error: SN_RUSTC path is too long to shell-quote safely; use --emit-rust.\n");
        return false;
    }
    int written = snprintf(command, sizeof(command), "%s --edition=2021 %s %s \"%s\" -o \"%s\"",
                           quoted_rustc, profile_flags, rustflags, source_path,
                           options->executable_file);
    if (written < 0 || (size_t)written >= sizeof(command))
    {
        fprintf(stderr, "Error: failed to build rustc build command\n");
        return false;
    }
    if (options->verbose) DEBUG_INFO("Executing: %s", command);
    if (!rust_run_command(command))
    {
        fprintf(stderr, "Error: rustc failed to build generated source\n");
        return false;
    }
    return true;
}

const TargetCompiler sn_rust_target = {
    TARGET_RUST,
    "rust",
    ".rs",
    "rust",
    rust_check_toolchain,
    rust_emit,
    rust_build
};
