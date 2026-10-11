#include "package_callable.h"
#include "package_native_types.h"
#include "../cgen/gen_model.h"
#include <stdlib.h>
#include <string.h>

static bool callable_file(const char *left, const char *right)
{
    if (!left || !right) return false;
#ifdef _WIN32
    char *a = _fullpath(NULL, left, 0), *b = _fullpath(NULL, right, 0);
    bool same = a && b && _stricmp(a, b) == 0;
#else
    char *a = realpath(left, NULL), *b = realpath(right, NULL);
    bool same = a && b && strcmp(a, b) == 0;
#endif
    free(a); free(b);
    return same;
}

static bool callable_statements(Arena *arena, Stmt **statements, int count, const char *file,
                                const char *name, PackageCallable *out, unsigned depth)
{
    if (depth > 64) return false;
    const char *dot = strchr(name, '.');
    for (int i = 0; i < count; i++) {
        Stmt *stmt = statements[i];
        if (stmt->type == STMT_IMPORT && callable_statements(arena, stmt->as.import.imported_stmts,
            stmt->as.import.imported_count, file, name, out, depth + 1)) return true;
        if (!dot && stmt->type == STMT_FUNCTION && callable_file(stmt->as.function.name.filename, file) &&
            !strcmp(stmt->as.function.name.start, name)) {
            out->function = &stmt->as.function;
            return true;
        }
        if (!dot || stmt->type != STMT_STRUCT_DECL) continue;
        StructDeclStmt *owner = &stmt->as.struct_decl;
        if (!callable_file(owner->name.filename, file) || strlen(owner->name.start) != (size_t)(dot - name) ||
            strncmp(owner->name.start, name, (size_t)(dot - name))) continue;
        for (int m = 0; m < owner->method_count; m++) {
            StructMethod *method = &owner->methods[m];
            if (strcmp(method->name, dot + 1)) continue;
            out->method = method; out->owner = owner;
            if (!method->is_static)
                out->self_type = ast_create_struct_type(arena, owner->name.start, owner->fields, owner->field_count,
                    owner->methods, owner->method_count, owner->is_native, owner->is_packed,
                    owner->pass_self_by_ref, owner->c_alias);
            return true;
        }
    }
    return false;
}

bool package_find_callable(Arena *arena, Module *module, const char *file,
                            const char *name, PackageCallable *out)
{
    memset(out, 0, sizeof(*out));
    return module && callable_statements(arena, module->statements, module->count, file, name, out, 0);
}

bool package_binding_is_sindarin(json_object *plan, json_object *binding)
{
    json_object *builds = NULL, *build = NULL;
    json_object_object_get_ex(plan, "builds", &builds);
    json_object_object_get_ex(binding, "build", &build);
    for (size_t i = 0; i < json_object_array_length(builds); i++) {
        json_object *unit = json_object_array_get_idx(builds, i), *name = NULL, *language = NULL;
        json_object_object_get_ex(unit, "name", &name);
        json_object_object_get_ex(unit, "language", &language);
        if (!strcmp(json_object_get_string(name), json_object_get_string(build)))
            return !strcmp(json_object_get_string(language), "SN");
    }
    return false;
}

bool package_callable_valid(const PackageCallable *callable, bool sindarin)
{
    Parameter *params = NULL;
    int count = 0;
    if (callable->function) {
        FunctionStmt *fn = callable->function;
        if ((!sindarin && (!fn->is_native || fn->body_count)) || fn->is_variadic || fn->type_param_count ||
            fn->return_mem_qualifier != MEM_DEFAULT) return false;
        params = fn->params; count = fn->param_count;
    } else if (callable->method) {
        StructMethod *method = callable->method;
        if (!sindarin || callable->owner->type_param_count || method->return_mem_qualifier != MEM_DEFAULT) return false;
        if (!method->is_static && (!callable->owner->is_native || !callable->owner->pass_self_by_ref)) return false;
        params = method->params; count = method->param_count;
    } else return false;
    for (int i = 0; i < count; i++)
        if (params[i].mem_qualifier != MEM_DEFAULT || params[i].sync_modifier != SYNC_NONE) return false;
    return true;
}

json_object *package_callable_signature(CompilerOptions *options, const PackageCallable *callable,
                                       json_object *plan, json_object *binding, Module *module,
                                       Module *const *modules, int count)
{
    json_object *signature = json_object_new_object(), *params = json_object_new_array(), *abi = NULL;
    json_object_object_get_ex(plan, "abi", &abi);
    json_object_object_add(signature, "abi", json_object_get(abi));
    json_object_object_add(signature, "binding", json_object_get(binding));
    Type *result = callable->function ? callable->function->return_type : callable->method->return_type;
    json_object_object_add(signature, "return_type", package_native_model_type(&options->arena, result, module, modules, count));
    json_object_object_add(signature, "params", params);
    Parameter *parameters = callable->function ? callable->function->params : callable->method->params;
    int parameter_count = callable->function ? callable->function->param_count : callable->method->param_count;
    if (callable->self_type) {
        json_object *self = json_object_new_object();
        json_object_object_add(self, "name", json_object_new_string("self"));
        json_object_object_add(self, "type", package_native_model_type(&options->arena, callable->self_type, module, modules, count));
        json_object_array_add(params, self);
    }
    for (int i = 0; i < parameter_count; i++) {
        json_object *entry = json_object_new_object();
        json_object_object_add(entry, "name", json_object_new_string(parameters[i].name.start));
        json_object_object_add(entry, "type", package_native_model_type(&options->arena, parameters[i].type, module, modules, count));
        json_object_array_add(params, entry);
    }
    return signature;
}

static bool callable_token(const Token *left, const Token *right)
{
    return left->line == right->line && callable_file(left->filename, right->filename);
}

typedef struct { Type **items; size_t count, capacity; } CallableTypes;

static void callable_adapt_type(Type *type, const StructMethod *target, const char *alias, CallableTypes *seen)
{
    if (!type) return;
    for (size_t i = 0; i < seen->count; i++) if (seen->items[i] == type) return;
    if (seen->count == seen->capacity) {
        size_t capacity = seen->capacity ? seen->capacity * 2 : 64;
        Type **next = realloc(seen->items, capacity * sizeof(*next));
        if (!next) return;
        seen->items = next;
        seen->capacity = capacity;
    }
    seen->items[seen->count++] = type;
    if (type->kind == TYPE_ARRAY) callable_adapt_type(type->as.array.element_type, target, alias, seen);
    else if (type->kind == TYPE_POINTER) callable_adapt_type(type->as.pointer.base_type, target, alias, seen);
    else if (type->kind == TYPE_FUNCTION) {
        callable_adapt_type(type->as.function.return_type, target, alias, seen);
        for (int i = 0; i < type->as.function.param_count; i++)
            callable_adapt_type(type->as.function.param_types[i], target, alias, seen);
    } else if (type->kind == TYPE_STRUCT) {
        for (int i = 0; i < type->as.struct_type.method_count; i++) {
            StructMethod *method = &type->as.struct_type.methods[i];
            if (callable_token(&method->name_token, &target->name_token)) {
                method->is_native = true; method->body = NULL; method->body_count = 0; method->c_alias = alias;
            }
            callable_adapt_type(method->return_type, target, alias, seen);
            for (int p = 0; p < method->param_count; p++) callable_adapt_type(method->params[p].type, target, alias, seen);
        }
        for (int i = 0; i < type->as.struct_type.field_count; i++)
            callable_adapt_type(type->as.struct_type.fields[i].type, target, alias, seen);
    }
}

static void callable_adapt_symbols(Symbol *symbols, const PackageCallable *callable,
                                    const char *alias, CallableTypes *seen)
{
    for (Symbol *symbol = symbols; symbol; symbol = symbol->next) {
        if (callable->function && symbol->is_function && callable_token(&symbol->name, &callable->function->name))
        {
            symbol->c_alias = alias;
            if (symbol->type && symbol->type->kind == TYPE_FUNCTION) {
                symbol->type->as.function.is_native = true;
                symbol->type->as.function.has_body = false;
            }
        }
        if (callable->method) callable_adapt_type(symbol->type, callable->method, alias, seen);
        if (symbol->namespace_symbols) callable_adapt_symbols(symbol->namespace_symbols, callable, alias, seen);
    }
}

void package_callable_adapt(CompilerOptions *options, const PackageCallable *callable,
                            const char *alias)
{
    CallableTypes seen = {0};
    for (int i = 0; i < options->symbol_table.scopes_count; i++)
        callable_adapt_symbols(options->symbol_table.scopes[i]->symbols, callable, alias, &seen);
    free(seen.items);
    if (callable->function) {
        callable->function->is_native = true; callable->function->body = NULL;
        callable->function->body_count = 0; callable->function->c_alias = alias;
    } else {
        callable->method->is_native = true; callable->method->body = NULL;
        callable->method->body_count = 0; callable->method->c_alias = alias;
    }
}
