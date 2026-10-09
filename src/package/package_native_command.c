#include "package_native_command.h"
#include "../package.h"
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

int package_native_command(const CompilerOptions *options)
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
