#include "package_import.h"
#include "package_native_command.h"
#include "package_native_types.h"
#include "package_callable.h"
#include "../package.h"
#include "../cgen/gen_model.h"
#include <json-c/json.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#ifdef _WIN32
#include <direct.h>
#include <process.h>
#define import_mkdir(p) _mkdir(p)
#define import_pid() _getpid()
#else
#include <unistd.h>
#define import_mkdir(p) mkdir(p, 0755)
#define import_pid() getpid()
#endif

static char *import_absolute(const char *path)
{
#ifdef _WIN32
    return _fullpath(NULL, path, 0);
#else
    return realpath(path, NULL);
#endif
}

static bool import_same(const char *left, const char *right)
{
#ifdef _WIN32
    return _stricmp(left, right) == 0;
#else
    return strcmp(left, right) == 0;
#endif
}

static bool import_directory(const char *path)
{
    struct stat st;
    return (stat(path, &st) == 0) || import_mkdir(path) == 0;
}

/* Normalize declared source paths even when prebuilt consumption has removed
 * the implementation file. The containing public module/manifest still exists. */
static char *import_source_absolute(const char *path)
{
#ifdef _WIN32
    return _fullpath(NULL, path, 0);
#else
    char absolute[8192];
    if (path[0] == '/') snprintf(absolute, sizeof(absolute), "%s", path);
    else {
        char cwd[4096];
        if (!getcwd(cwd, sizeof(cwd))) return NULL;
        if (snprintf(absolute, sizeof(absolute), "%s/%s", cwd, path) >= (int)sizeof(absolute)) return NULL;
    }
    char *parts[4096], *save = NULL;
    int count = 0;
    for (char *part = strtok_r(absolute, "/", &save); part; part = strtok_r(NULL, "/", &save)) {
        if (!strcmp(part, ".")) continue;
        if (!strcmp(part, "..")) { if (count) count--; continue; }
        if (count == 4096) return NULL;
        parts[count++] = part;
    }
    size_t size = strlen(path) + 4098;
    char *result = calloc(size, 1);
    if (!result) return NULL;
    for (int i = 0; i < count; i++) { strcat(result, "/"); strcat(result, parts[i]); }
    return result;
#endif
}

static bool import_provided_source(Stmt *stmt, const char *manifest, json_object *plan, const char *origin)
{
    char *root = strdup(manifest), *directory = strdup(origin);
    for (char *p = root; *p; p++) if (*p == '\\') *p = '/';
    for (char *p = directory; *p; p++) if (*p == '\\') *p = '/';
    char *end = strrchr(root, '/');
    if (end) *end = 0; else strcpy(root, ".");
    end = strrchr(directory, '/');
    if (end) *end = 0; else strcpy(directory, ".");
    const char *value = stmt->as.pragma.value;
    size_t length = strlen(value);
    if (length >= 2 && value[0] == '"' && value[length - 1] == '"') { value++; length -= 2; }
    char path[8192];
    bool absolute = value[0] == '/' || (length > 1 && value[1] == ':');
    int written = absolute ? snprintf(path, sizeof(path), "%.*s", (int)length, value)
                           : snprintf(path, sizeof(path), "%s/%.*s", directory, (int)length, value);
    char *source = written < (int)sizeof(path) ? import_source_absolute(path) : NULL;
    bool provided = false;
    json_object *builds = NULL;
    json_object_object_get_ex(plan, "builds", &builds);
    for (size_t i = 0; source && i < json_object_array_length(builds); i++) {
        json_object *unit = json_object_array_get_idx(builds, i), *paths = NULL;
        if (!json_object_object_get_ex(unit, "provides_sources", &paths)) continue;
        for (size_t p = 0; p < json_object_array_length(paths); p++) {
            snprintf(path, sizeof(path), "%s/%s", root, json_object_get_string(json_object_array_get_idx(paths, p)));
            char *candidate = import_source_absolute(path);
            if (candidate && import_same(source, candidate)) provided = true;
            free(candidate);
        }
    }
    free(source); free(root); free(directory);
    return provided;
}

static void import_mark_provided_sources(Stmt **statements, int count, const char *manifest, json_object *plan, unsigned depth)
{
    if (depth > 64) return;
    for (int i = 0; i < count; i++) {
        Stmt *stmt = statements[i];
        if (stmt->type == STMT_IMPORT)
            import_mark_provided_sources(stmt->as.import.imported_stmts, stmt->as.import.imported_count, manifest, plan, depth + 1);
        if (stmt->type != STMT_PRAGMA || stmt->as.pragma.pragma_type != PRAGMA_SOURCE || !stmt->token || !stmt->token->filename) continue;
        char *owner = package_source_manifest(stmt->token->filename);
        if (owner && import_same(owner, manifest) && import_provided_source(stmt, manifest, plan, stmt->token->filename))
            stmt->as.pragma.package_owned_source = true;
        free(owner);
    }
}

static void import_add_pragma(CompilerOptions *options, Module *module, PragmaType kind, const char *value)
{
    if (module->count == module->capacity) {
        int capacity = module->capacity ? module->capacity * 2 : 8;
        Stmt **array = arena_alloc(&options->arena, sizeof(*array) * (size_t)capacity);
        memcpy(array, module->statements, sizeof(*array) * (size_t)module->count);
        module->statements = array; module->capacity = capacity;
    }
    module->statements[module->count++] = ast_create_pragma_stmt(&options->arena, kind, value, NULL);
}

static bool import_type(Type *type)
{
    if (!type) return false;
    if (type->kind == TYPE_STRUCT) return type->as.struct_type.is_native && type->as.struct_type.pass_self_by_ref;
    if (type->kind == TYPE_ARRAY && type->as.array.element_type &&
        (type->as.array.element_type->kind == TYPE_STRING || type->as.array.element_type->kind == TYPE_BYTE)) return true;
    return type->kind == TYPE_VOID || type->kind == TYPE_INT || type->kind == TYPE_LONG ||
           type->kind == TYPE_UINT || type->kind == TYPE_INT32 || type->kind == TYPE_UINT32 ||
           type->kind == TYPE_BYTE || type->kind == TYPE_CHAR || type->kind == TYPE_BOOL ||
           type->kind == TYPE_FLOAT || type->kind == TYPE_DOUBLE || type->kind == TYPE_STRING;
}

static bool import_file_loaded(CompilerOptions *options, const char *file,
                               const char *const *paths, int count)
{
    for (int i = -1; i < count; i++) {
        const char *path = i < 0 ? options->source_file : paths[i];
        char *absolute = path ? import_absolute(path) : NULL;
        bool loaded = absolute && import_same(file, absolute);
        free(absolute);
        if (loaded) return true;
    }
    return false;
}

bool package_prepare_native_imports(CompilerOptions *options, Module *module,
                                   const char *const *paths, Module *const *modules, int count)
{
    json_object *contracts = json_object_new_array();
    char **seen = calloc((size_t)count + 1, sizeof(*seen));
    if (!contracts || !seen) {
        if (contracts) json_object_put(contracts);
        free(seen);
        return false;
    }
    int seen_count = 0;
    bool success = true;
    char *body_manifest = options->package_body ? package_source_manifest(options->source_file) : NULL;
    for (int p = -1; success && p < count; p++) {
        const char *source = p < 0 ? options->source_file : paths[p];
        char *manifest = package_source_manifest(source);
        if (!manifest) continue;
        if (body_manifest && import_same(manifest, body_manifest)) {
            PackageConfig config;
            json_object *plan = NULL;
            success = package_yaml_parse(manifest, &config);
            if (success && config.has_native) {
                success = package_yaml_native_plan(manifest, &plan);
                if (success) {
                    success = package_native_bind_records(&options->arena, plan, manifest, module);
                    import_mark_provided_sources(module->statements, module->count, manifest, plan, 0);
                    for (int i = 0; success && i < count; i++) {
                        success = package_native_bind_records(&options->arena, plan, manifest, modules[i]);
                        if (modules[i]) import_mark_provided_sources(modules[i]->statements, modules[i]->count, manifest, plan, 0);
                    }
                    json_object_put(plan);
                }
            }
            free(manifest); continue;
        }
        bool duplicate = false;
        for (int i = 0; i < seen_count; i++) if (import_same(manifest, seen[i])) duplicate = true;
        if (duplicate) { free(manifest); continue; }
        seen[seen_count++] = manifest;
        PackageConfig config;
        if (!package_yaml_parse(manifest, &config)) { success = false; break; }
        if (!config.has_native) continue;
        json_object *plan = NULL;
        if (!package_yaml_native_plan(manifest, &plan)) { success = false; break; }
        import_mark_provided_sources(module->statements, module->count, manifest, plan, 0);
        for (int i = 0; i < count; i++)
            if (modules[i]) import_mark_provided_sources(modules[i]->statements, modules[i]->count, manifest, plan, 0);
        if (!package_native_bind_records(&options->arena, plan, manifest, module)) success = false;
        for (int i = 0; success && i < count; i++)
            success = package_native_bind_records(&options->arena, plan, manifest, modules[i]);
        /* A source directive would compile a second per-application backing
         * and could silently shadow the independent archive. Keep that legacy
         * route unchanged for manifests without native metadata. */
        for (int i = -1; success && i < count; i++) {
            Module *owner = i < 0 ? module : modules[i];
            for (int j = 0; owner && j < owner->count; j++) {
                Stmt *stmt = owner->statements[j];
                if (stmt->type != STMT_PRAGMA || stmt->as.pragma.pragma_type != PRAGMA_SOURCE || stmt->as.pragma.package_owned_source) continue;
                const char *origin = stmt->token && stmt->token->filename ? stmt->token->filename : owner->filename;
                char *source_owner = origin ? package_source_manifest(origin) : NULL;
                bool belongs = source_owner && import_same(source_owner, manifest);
                free(source_owner);
                if (belongs) {
                    fprintf(stderr, "error: %s: independent package artifacts and generated adapters cannot also compile per-application @source backing\n", manifest);
                    success = false; break;
                }
            }
        }
        if (!success) { json_object_put(plan); break; }
        char *root = strdup(manifest);
        char *end = strrchr(root, '/');
#ifdef _WIN32
        char *back = strrchr(root, '\\');
        if (back && (!end || back > end)) end = back;
#endif
        if (end) *end = 0;
        json_object *bindings = NULL;
        json_object_object_get_ex(plan, "bindings", &bindings);
        json_object *contract = json_object_new_object();
        json_object_object_add(contract, "manifest", json_object_new_string(manifest));
        json_object *signatures = json_object_new_array();
        json_object_object_add(contract, "signatures", signatures);
        for (size_t b = 0; success && b < json_object_array_length(bindings); b++) {
            json_object *binding = json_object_array_get_idx(bindings, b), *decl = NULL;
            json_object_object_get_ex(binding, "declaration", &decl);
            char *identity = strdup(json_object_get_string(decl));
            char *separator = strstr(identity, "::");
            if (!separator) { free(identity); success = false; break; }
            *separator = 0;
            char candidate[4096];
            snprintf(candidate, sizeof(candidate), "%s/%s", root, identity);
            char *file = import_absolute(candidate);
            /* One artifact may expose several API modules. Only declarations
             * actually loaded into this application need consumer adapters;
             * the independent provider build validates the complete artifact. */
            if (file && !import_file_loaded(options, file, paths, count)) {
                free(identity); free(file); continue;
            }
            PackageCallable callable;
            bool found = file && package_find_callable(&options->arena, module, file, separator + 2, &callable);
            for (int i = 0; !found && file && i < count; i++)
                found = package_find_callable(&options->arena, modules[i], file, separator + 2, &callable);
            bool sindarin = package_binding_is_sindarin(plan, binding);
            Type *return_type = found ? (callable.function ? callable.function->return_type : callable.method->return_type) : NULL;
            if (!found || !package_callable_valid(&callable, sindarin) || !import_type(return_type)) {
                fprintf(stderr, "error: %s: native binding '%s' requires a supported native declaration or independently compiled Sindarin callable\n",
                        manifest, json_object_get_string(decl));
                free(identity); free(file); success = false; break;
            }
            json_object *signature = package_callable_signature(options, &callable, plan, binding, module, modules, count);
            Parameter *parameters = callable.function ? callable.function->params : callable.method->params;
            int parameter_count = callable.function ? callable.function->param_count : callable.method->param_count;
            for (int i = 0; i < parameter_count; i++) {
                Parameter *param = &parameters[i];
                if (!import_type(param->type) || param->type->kind == TYPE_VOID || param->mem_qualifier != MEM_DEFAULT) {
                    fprintf(stderr, "error: %s: native parameter '%s' requires an implemented ABI representation\n", manifest, param->name.start);
                    success = false; break;
                }
            }
            char alias[96];
            /* Per-compilation package identity plus binding index prevents
             * consumer names from colliding with one another. */
            snprintf(alias, sizeof(alias), "__sn_package_%d_call_%zu", seen_count, b);
            const char *owned_alias = arena_strdup(&options->arena, alias);
            package_callable_adapt(options, &callable, owned_alias);
            json_object_object_add(signature, "adapter", json_object_new_string(alias));
            json_object_array_add(signatures, signature);
            free(identity); free(file);
        }
        /* Until independent Sindarin body compilation is available, fixed
         * cross-runtime packages may contain only native declarations. */
        PackageRuntime application = options->target == TARGET_RUST ? PACKAGE_RUNTIME_RS : PACKAGE_RUNTIME_C;
        PackageRuntime runtime = p < 0 ? application : package_runtime_resolve(config.runtime, application);
        for (int i = -1; success && i < count; i++) {
            Module *owner = i < 0 ? module : modules[i];
            for (int s = 0; owner && s < owner->count; s++) {
                Stmt *stmt = owner->statements[s];
                const char *origin = NULL;
                bool implementation = false;
                if (stmt->type == STMT_FUNCTION) {
                    origin = stmt->as.function.name.filename;
                    implementation = stmt->as.function.body_count > 0;
                } else if (stmt->type == STMT_STRUCT_DECL) {
                    origin = stmt->as.struct_decl.name.filename;
                    for (int m = 0; m < stmt->as.struct_decl.method_count; m++)
                        if (stmt->as.struct_decl.methods[m].body_count > 0) implementation = true;
                } else if (stmt->type == STMT_VAR_DECL) {
                    origin = stmt->as.var_decl.name.filename;
                    implementation = true; /* Package-owned storage needs package initialization. */
                }
                if (!implementation || !origin) continue;
                char *owned_manifest = package_source_manifest(origin);
                bool belongs = owned_manifest && import_same(owned_manifest, manifest);
                free(owned_manifest);
                if (belongs && runtime !=
                        (options->target == TARGET_RUST ? PACKAGE_RUNTIME_RS : PACKAGE_RUNTIME_C)) {
                    fprintf(stderr, "error: %s: Sindarin package bodies require independent %s compilation; that package pipeline is not implemented\n",
                            manifest, package_runtime_name(runtime));
                    success = false; break;
                }
            }
        }
        free(root); json_object_put(plan);
        if (success) json_object_array_add(contracts, contract);
        else json_object_put(contract);
    }
    for (int i = 0; i < seen_count; i++) free(seen[i]);
    free(body_manifest);
    free(seen);
    if (!success || json_object_array_length(contracts) == 0) { json_object_put(contracts); return success; }
    if (!import_directory(".sn") || !import_directory(".sn/build") || !import_directory(".sn/build/native-imports"))
    { json_object_put(contracts); return false; }
    char request[256], response[256], driver[4096], compiler[4096];
    snprintf(request, sizeof(request), ".sn/build/native-imports/request-%ld.json", (long)import_pid());
    snprintf(response, sizeof(response), ".sn/build/native-imports/response-%ld.json", (long)import_pid());
    json_object *data = json_object_new_object();
    json_object_object_add(data, "packages", contracts);
    json_object_object_add(data, "output", json_object_new_string(response));
    json_object_object_add(data, "target", json_object_new_string(options->target == TARGET_RUST ? "rust" : "c"));
    json_object_object_add(data, "optimization", json_object_new_int(options->optimization_level));
    json_object_object_add(data, "arithmetic", json_object_new_string(options->arithmetic_mode == ARITH_CHECKED ? "checked" : "unchecked"));
    if (json_object_to_file_ext(request, data, JSON_C_TO_STRING_PRETTY) != 0) { json_object_put(data); return false; }
    json_object_put(data);
    snprintf(driver, sizeof(driver), "%s/tools/prepare_native_imports.py", options->compiler_dir);
    snprintf(compiler, sizeof(compiler), "%s/sn%s", options->compiler_dir,
#ifdef _WIN32
             ".exe"
#else
             ""
#endif
             );
    const char *python = getenv("SN_PYTHON");
    if (!python || !python[0]) python =
#ifdef _WIN32
        "python";
#else
        "python3";
#endif
    char *args[] = {(char *)python, driver, "--contract", request, "--compiler", compiler, NULL};
    if (package_native_run_driver(args) != 0) return false;
    json_object *result = json_object_from_file(response);
    if (!result) return false;
    json_object *sources = NULL, *links = NULL, *includes = NULL;
    json_object_object_get_ex(result, "sources", &sources);
    json_object_object_get_ex(result, "links", &links);
    json_object_object_get_ex(result, "includes", &includes);
    for (size_t i = 0; includes && i < json_object_array_length(includes); i++) {
        const char *path = json_object_get_string(json_object_array_get_idx(includes, i));
        char quoted[8192]; snprintf(quoted, sizeof(quoted), "\"%s\"", path);
        import_add_pragma(options, module, PRAGMA_INCLUDE, quoted);
    }
    for (size_t i = 0; sources && i < json_object_array_length(sources); i++) {
        const char *path = json_object_get_string(json_object_array_get_idx(sources, i));
        char quoted[8192]; snprintf(quoted, sizeof(quoted), "\"%s\"", path);
        import_add_pragma(options, module, PRAGMA_SOURCE, quoted);
    }
    for (size_t i = 0; links && i < json_object_array_length(links); i++)
        import_add_pragma(options, module, PRAGMA_LINK, json_object_get_string(json_object_array_get_idx(links, i)));
    json_object_put(result);
    return true;
}
