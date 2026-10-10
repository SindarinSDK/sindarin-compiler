#include "package_native_command.h"
#include "../package.h"
#include "../parser.h"
#include "../type_checker.h"
#include "../file.h"
#include "../diagnostic.h"
#include "../cgen/gen_model.h"
#include <json-c/json.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <sys/wait.h>
#endif

/* Resolve provider contracts without compiling an application or recursively
 * invoking its import-adapter pipeline. Each API file has an isolated scope. */
static bool native_provider_signatures(CompilerOptions *options, json_object *native)
{
    json_object *bindings = NULL, *declarations = NULL;
    json_object_object_get_ex(native, "bindings", &bindings);
    json_object_object_get_ex(native, "declarations", &declarations);
    json_object *signatures = json_object_new_array();
    for (size_t b = 0; b < json_object_array_length(bindings); b++) {
        json_object *binding = json_object_array_get_idx(bindings, b), *function = NULL;
        if (!json_object_object_get_ex(binding, "function", &function)) continue;
        json_object *decl = NULL;
        json_object_object_get_ex(binding, "declaration", &decl);
        char *identity = strdup(json_object_get_string(decl));
        char *separator = strstr(identity, "::");
        bool listed = false;
        if (separator) {
            *separator = 0;
            for (size_t d = 0; d < json_object_array_length(declarations); d++)
                if (!strcmp(identity, json_object_get_string(json_object_array_get_idx(declarations, d)))) listed = true;
        }
        if (!listed) {
            fprintf(stderr, "error: provider binding '%s' must name a function in native.declarations\n", json_object_get_string(decl));
            free(identity); json_object_put(signatures); return false;
        }
        char path[4096];
        const char *slash = strrchr(options->native_manifest, '/');
#ifdef _WIN32
        const char *back = strrchr(options->native_manifest, '\\');
        if (back && (!slash || back > slash)) slash = back;
#endif
        int prefix = slash ? (int)(slash - options->native_manifest + 1) : 0;
        if (snprintf(path, sizeof(path), "%.*s%s", prefix, options->native_manifest, identity) >= (int)sizeof(path)) {
            free(identity); json_object_put(signatures); return false;
        }
        Arena *arena = &options->arena;
        const char *filename = arena_strdup(arena, path);
        SymbolTable symbols;
        symbol_table_init(arena, &symbols);
        char *source = file_read(arena, filename);
        Module *module = NULL;
        if (source) {
            diagnostic_init(filename, source);
            char **imports = NULL;
            Module **modules = NULL;
            bool *direct = NULL, *emitted = NULL;
            int count = 0, capacity = 0;
            module = parse_module_with_imports(arena, &symbols, filename, &imports, &count,
                &capacity, &modules, &direct, &emitted, options->compiler_dir);
            if (module && !type_check_module(module, &symbols)) module = NULL;
        }
        FunctionStmt *fn = NULL;
        for (int i = 0; module && i < module->count; i++) {
            Stmt *stmt = module->statements[i];
            if (stmt->type == STMT_FUNCTION && stmt->as.function.name.filename &&
                !strcmp(stmt->as.function.name.filename, path) &&
                !strcmp(stmt->as.function.name.start, separator + 2)) fn = &stmt->as.function;
        }
        bool valid = fn && fn->is_native && !fn->body_count && !fn->is_variadic &&
                     !fn->type_param_count && fn->return_mem_qualifier == MEM_DEFAULT;
        if (valid) {
            json_object *signature = json_object_new_object(), *params = json_object_new_array();
            json_object *abi = NULL;
            json_object_object_get_ex(native, "abi", &abi);
            json_object_object_add(signature, "abi", json_object_get(abi));
            json_object_object_add(signature, "binding", json_object_get(binding));
            json_object_object_add(signature, "return_type", gen_model_type(arena, fn->return_type));
            json_object_object_add(signature, "params", params);
            for (int i = 0; i < fn->param_count; i++) {
                Parameter *param = &fn->params[i];
                if (param->mem_qualifier != MEM_DEFAULT) { valid = false; break; }
                json_object *entry = json_object_new_object();
                json_object_object_add(entry, "name", json_object_new_string(param->name.start));
                json_object_object_add(entry, "type", gen_model_type(arena, param->type));
                json_object_array_add(params, entry);
            }
            json_object_array_add(signatures, signature);
        }
        symbol_table_cleanup(&symbols);
        free(identity);
        if (!valid) {
            fprintf(stderr, "error: provider binding '%s' requires a native declaration without a body or reference qualifiers\n", json_object_get_string(decl));
            json_object_put(signatures); return false;
        }
    }
    if (json_object_array_length(signatures)) json_object_object_add(native, "signatures", signatures);
    else json_object_put(signatures);
    return true;
}

/* Invoke argv directly: paths and metadata are never interpreted by a shell. */
int package_native_run_driver(char *const *args)
{
#ifdef _WIN32
    size_t size = 1;
    for (int i = 0; args[i]; i++) size += strlen(args[i]) * 2 + 3;
    char *command = malloc(size);
    if (!command) return 1;
    char *out = command;
    for (int i = 0; args[i]; i++) {
        if (i) *out++ = ' ';
        *out++ = '"';
        unsigned slashes = 0;
        for (const char *p = args[i]; ; p++) {
            if (*p == '\\') { slashes++; continue; }
            unsigned emit = (*p == '"' || !*p) ? slashes * 2 : slashes;
            while (emit--) *out++ = '\\';
            slashes = 0;
            if (!*p) break;
            if (*p == '"') *out++ = '\\';
            *out++ = *p;
        }
        *out++ = '"';
    }
    *out = 0;
    STARTUPINFOA startup;
    PROCESS_INFORMATION process;
    memset(&startup, 0, sizeof(startup));
    memset(&process, 0, sizeof(process));
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESTDHANDLES;
    startup.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    startup.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    startup.hStdError = GetStdHandle(STD_ERROR_HANDLE);
    BOOL created = CreateProcessA(NULL, command, NULL, NULL, TRUE, 0, NULL, NULL, &startup, &process);
    free(command);
    if (!created) { fprintf(stderr, "error: cannot start native build driver (Windows error %lu)\n", GetLastError()); return 1; }
    WaitForSingleObject(process.hProcess, INFINITE);
    DWORD code = 1;
    GetExitCodeProcess(process.hProcess, &code);
    CloseHandle(process.hProcess); CloseHandle(process.hThread);
    return code == 0 ? 0 : 1;
#else
    pid_t child = fork();
    if (child < 0) return 1;
    if (child == 0) {
        execvp(args[0], args);
        fprintf(stderr, "error: cannot start native build driver '%s': %s\n", args[0], strerror(errno));
        _exit(127);
    }
    int status = 0;
    pid_t result;
    do { result = waitpid(child, &status, 0); } while (result < 0 && errno == EINTR);
    return result == child && WIFEXITED(status) && WEXITSTATUS(status) == 0 ? 0 : 1;
#endif
}

int package_native_command(CompilerOptions *options)
{
    if (options->native_mode == 2) {
        char driver[4096], compiler[4096], optimization[16];
        if (snprintf(driver, sizeof(driver), "%s/tools/build_native_package.py", options->compiler_dir) >= (int)sizeof(driver) ||
            snprintf(compiler, sizeof(compiler), "%s/sn%s", options->compiler_dir,
#ifdef _WIN32
                     ".exe"
#else
                     ""
#endif
                     ) >= (int)sizeof(compiler)) return 1;
        snprintf(optimization, sizeof(optimization), "%d", options->optimization_level);
        const char *python = getenv("SN_PYTHON");
        if (!python || !python[0])
#ifdef _WIN32
            python = "python";
#else
            python = "python3";
#endif
        char *args[] = {(char *)python, driver, "--compiler", compiler,
            "--manifest", options->native_manifest, "--target",
            options->target == TARGET_RUST ? "rust" : "c", "--optimization", optimization,
            "--arithmetic", options->arithmetic_mode == ARITH_CHECKED ? "checked" : "unchecked",
            "--out-dir", options->output_file ? options->output_file : ".sn/build/native", NULL};
        return package_native_run_driver(args);
    }
    PackageConfig config;
    json_object *native = NULL;
    if (!package_yaml_parse(options->native_manifest, &config) ||
        !package_yaml_native_plan(options->native_manifest, &native)) return 1;
    if (!native) { fprintf(stderr, "error: manifest has no native build metadata\n"); return 1; }
    if (!native_provider_signatures(options, native)) { json_object_put(native); return 1; }
    json_object *plan = json_object_new_object();
    json_object *package = json_object_new_object();
    json_object_object_add(package, "name", json_object_new_string(config.name));
    json_object_object_add(package, "version", json_object_new_string(config.version));
    const char *runtime = package_runtime_name(package_runtime_resolve(config.runtime,
        options->target == TARGET_RUST ? PACKAGE_RUNTIME_RS : PACKAGE_RUNTIME_C));
    json_object_object_add(package, "runtime", json_object_new_string(runtime));
    json_object_object_add(plan, "schema", json_object_new_int(1));
    json_object_object_add(plan, "package", package);
    json_object_object_add(plan, "native", native);
    json_object_object_add(plan, "compiler_dir", json_object_new_string(options->compiler_dir));
    json_object_object_add(plan, "optimization", json_object_new_int(options->optimization_level));
    json_object_object_add(plan, "arithmetic", json_object_new_string(
        options->arithmetic_mode == ARITH_CHECKED ? "checked" : "unchecked"));
    const char *json = json_object_to_json_string_ext(plan, JSON_C_TO_STRING_PRETTY);
    FILE *out = options->output_file ? fopen(options->output_file, "wb") : stdout;
    int result = !out || fprintf(out, "%s\n", json) < 0;
    if (out && out != stdout && fclose(out) != 0) result = 1;
    json_object_put(plan);
    return result;
}
