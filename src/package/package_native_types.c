#include "package_native_types.h"
#include "../cgen/gen_model.h"
#include "../parser.h"
#include "../type_checker.h"
#include "../file.h"
#include "../diagnostic.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *record_absolute(const char *path)
{
#ifdef _WIN32
    return _fullpath(NULL, path, 0);
#else
    return realpath(path, NULL);
#endif
}

static bool record_same_file(const char *left, const char *right)
{
    char *a = record_absolute(left), *b = record_absolute(right);
#ifdef _WIN32
    bool same = a && b && _stricmp(a, b) == 0;
#else
    bool same = a && b && strcmp(a, b) == 0;
#endif
    free(a); free(b);
    return same;
}

static bool record_bind_statements(Arena *arena, json_object *records, const char *root,
                                   Stmt **statements, int count, unsigned depth)
{
    if (depth > 64) return false;
    for (int i = 0; i < count; i++) {
        Stmt *stmt = statements[i];
        if (stmt->type == STMT_IMPORT) {
            if (!record_bind_statements(arena, records, root, stmt->as.import.imported_stmts,
                                         stmt->as.import.imported_count, depth + 1)) return false;
            continue;
        }
        if (stmt->type != STMT_STRUCT_DECL || !stmt->as.struct_decl.name.filename) continue;
        StructDeclStmt *decl = &stmt->as.struct_decl;
        for (size_t r = 0; r < json_object_array_length(records); r++) {
            json_object *record = json_object_array_get_idx(records, r), *value = NULL;
            json_object_object_get_ex(record, "declaration", &value);
            const char *identity = json_object_get_string(value), *separator = strstr(identity, "::");
            char file[4096];
            if (!separator || snprintf(file, sizeof(file), "%s/%.*s", root,
                (int)(separator - identity), identity) >= (int)sizeof(file)) return false;
            const char *name = decl->name.start, *wanted = separator + 2;
            size_t length = strlen(name), wanted_length = strlen(wanted);
            bool matches = !strcmp(name, wanted) || (length > wanted_length + 2 &&
                name[length-wanted_length-1] == '_' && name[length-wanted_length-2] == '_' &&
                !strcmp(name + length-wanted_length, wanted));
            if (!matches || !record_same_file(file, decl->name.filename)) continue;
            if (!decl->is_native || !decl->pass_self_by_ref || decl->is_packed ||
                decl->is_serializable || decl->type_param_count) {
                fprintf(stderr, "error: native record '%s' requires an unpacked native as-ref declaration\n", identity);
                return false;
            }
            json_object *storage = NULL;
            json_object_deep_copy(record, &storage, NULL);
            json_object_object_get_ex(record, "header", &value);
            char header[4096];
            if (snprintf(header, sizeof(header), "%s/%s", root, json_object_get_string(value)) >= (int)sizeof(header)) {
                json_object_put(storage); return false;
            }
            char *absolute = record_absolute(header);
            if (!absolute) {
                fprintf(stderr, "error: native record '%s' public C header is missing\n", identity);
                json_object_put(storage); return false;
            }
            /* Keep signatures relocatable; only the private emission model
             * needs the header's resolved path. */
            for (char *p = absolute; *p; p++) if (*p == '\\') *p = '/';
            json_object_object_add(storage, "resolved_header", json_object_new_string(absolute));
            decl->native_record_contract = arena_strdup(arena, json_object_to_json_string(storage));
            free(absolute); json_object_put(storage);
        }
    }
    return true;
}

bool package_native_bind_records(Arena *arena, json_object *plan, const char *manifest,
                                 Module *module)
{
    json_object *records = NULL;
    if (!module || !json_object_object_get_ex(plan, "types", &records)) return true;
    char *root = strdup(manifest), *end = strrchr(root, '/');
#ifdef _WIN32
    char *back = strrchr(root, '\\');
    if (back && (!end || back > end)) end = back;
#endif
    if (end) *end = 0; else strcpy(root, ".");
    bool success = record_bind_statements(arena, records, root, module->statements, module->count, 0);
    free(root);
    return success;
}

static const char *record_find_contract(Stmt **statements, int count, const char *name, unsigned depth)
{
    if (depth > 64) return NULL;
    for (int i = 0; i < count; i++) {
        Stmt *stmt = statements[i];
        if (stmt->type == STMT_STRUCT_DECL && !strcmp(stmt->as.struct_decl.name.start, name) &&
            stmt->as.struct_decl.native_record_contract) return stmt->as.struct_decl.native_record_contract;
        if (stmt->type == STMT_IMPORT) {
            const char *found = record_find_contract(stmt->as.import.imported_stmts,
                stmt->as.import.imported_count, name, depth + 1);
            if (found) return found;
        }
    }
    return NULL;
}

json_object *package_native_model_type(Arena *arena, Type *type, Module *module,
                                       Module *const *modules, int count)
{
    json_object *result = gen_model_type(arena, type);
    if (type && type->kind == TYPE_ARRAY) {
        json_object_object_add(result, "element_type", package_native_model_type(arena,
            type->as.array.element_type, module, modules, count));
        return result;
    }
    if (!type || type->kind != TYPE_STRUCT) return result;
    const char *contract = module ? record_find_contract(module->statements, module->count,
        type->as.struct_type.name, 0) : NULL;
    for (int i = 0; !contract && i < count; i++)
        if (modules[i]) contract = record_find_contract(modules[i]->statements, modules[i]->count,
            type->as.struct_type.name, 0);
    if (contract) {
        json_object *storage = json_tokener_parse(contract);
        json_object_object_del(storage, "resolved_header");
        json_object_object_add(result, "native_record", storage);
    }
    return result;
}

bool package_native_validate_records(CompilerOptions *options, json_object *plan)
{
    json_object *records = NULL;
    if (!json_object_object_get_ex(plan, "types", &records)) return true;
    const char *manifest = options->native_manifest, *slash = strrchr(manifest, '/');
#ifdef _WIN32
    const char *back = strrchr(manifest, '\\');
    if (back && (!slash || back > slash)) slash = back;
#endif
    int prefix = slash ? (int)(slash - manifest + 1) : 0;
    for (size_t r = 0; r < json_object_array_length(records); r++) {
        json_object *record = json_object_array_get_idx(records, r), *value = NULL;
        json_object_object_get_ex(record, "declaration", &value);
        const char *identity = json_object_get_string(value), *separator = strstr(identity, "::");
        char path[4096];
        if (!separator || snprintf(path, sizeof(path), "%.*s%.*s", prefix, manifest,
            (int)(separator - identity), identity) >= (int)sizeof(path)) return false;
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
            if (module && !package_native_bind_records(arena, plan, manifest, module)) module = NULL;
        }
        const char *contract = module ? record_find_contract(module->statements, module->count, separator + 2, 0) : NULL;
        bool valid = false;
        if (contract) {
            json_object *resolved = json_tokener_parse(contract), *declaration = NULL;
            json_object_object_get_ex(resolved, "declaration", &declaration);
            valid = !strcmp(json_object_get_string(declaration), identity);
            json_object_put(resolved);
        }
        symbol_table_cleanup(&symbols);
        if (!valid) {
            fprintf(stderr, "error: native record '%s' does not resolve to its declared public native as-ref type\n", identity);
            return false;
        }
    }
    return true;
}

static StructDeclStmt *resource_find_declaration(Stmt **statements, int count,
                                                const char *file, const char *name, unsigned depth)
{
    if (depth > 64) return NULL;
    for (int i = 0; i < count; i++) {
        Stmt *stmt = statements[i];
        if (stmt->type == STMT_STRUCT_DECL && !strcmp(stmt->as.struct_decl.name.start, name) &&
            stmt->as.struct_decl.name.filename && record_same_file(stmt->as.struct_decl.name.filename, file))
            return &stmt->as.struct_decl;
        if (stmt->type == STMT_IMPORT) {
            StructDeclStmt *found = resource_find_declaration(stmt->as.import.imported_stmts,
                stmt->as.import.imported_count, file, name, depth + 1);
            if (found) return found;
        }
    }
    return NULL;
}

bool package_native_resolve_resource_result(CompilerOptions *options, json_object *plan,
    json_object *binding, Type *type, json_object *model, const char *manifest)
{
    const char *resource = json_object_get_string(json_object_object_get(binding, "result_type"));
    if (!resource && (!type || type->kind != TYPE_POINTER)) return true;
    if (!resource || !type || type->kind != TYPE_POINTER || !type->as.pointer.base_type ||
        type->as.pointer.base_type->kind != TYPE_VOID) {
        fprintf(stderr, "error: native result_type is required for an owned opaque *void result and cannot annotate other types\n");
        return false;
    }
    const char *separator = strstr(resource, "::"), *slash = strrchr(manifest, '/');
#ifdef _WIN32
    const char *back = strrchr(manifest, '\\');
    if (back && (!slash || back > slash)) slash = back;
#endif
    int prefix = slash ? (int)(slash - manifest + 1) : 0;
    char path[4096];
    if (!separator || snprintf(path, sizeof(path), "%.*s%.*s", prefix, manifest,
        (int)(separator - resource), resource) >= (int)sizeof(path)) return false;
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
        if (module && !package_native_bind_records(arena, plan, manifest, module)) module = NULL;
    }
    StructDeclStmt *decl = module ? resource_find_declaration(module->statements, module->count,
        filename, separator + 2, 0) : NULL;
    bool valid = decl && decl->native_record_contract;
    if (valid) {
        json_object *record = json_tokener_parse(decl->native_record_contract);
        valid = record && !strcmp(json_object_get_string(json_object_object_get(record, "declaration")), resource);
        if (valid) {
            Type *record_type = ast_create_struct_type(arena, decl->name.start, decl->fields, decl->field_count,
                decl->methods, decl->method_count, decl->is_native, decl->is_packed,
                decl->pass_self_by_ref, decl->c_alias);
            json_object_object_del(record, "resolved_header");
            json_object_object_add(model, "native_record", json_object_get(record));
            json_object_object_add(model, "resource_layout", gen_model_type(arena, record_type));
        }
        json_object_put(record);
    }
    symbol_table_cleanup(&symbols);
    if (!valid) fprintf(stderr, "error: opaque result_type '%s' must resolve to its canonical native as-ref record\n", resource);
    return valid;
}
