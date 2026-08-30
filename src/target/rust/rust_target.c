#include "target/target.h"
#include "target/rust/rust_render.h"
#include "cgen/gen_model.h"
#include "debug.h"
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *rustc_command(void)
{
    const char *configured = getenv("SN_RUSTC");
    return configured && configured[0] ? configured : "rustc";
}

static bool rust_check_toolchain(const CompilerOptions *options)
{
    char command[PATH_MAX + 64];
#ifdef _WIN32
    snprintf(command, sizeof(command), "%s --version >NUL 2>&1", rustc_command());
#else
    snprintf(command, sizeof(command), "%s --version >/dev/null 2>&1", rustc_command());
#endif
    if (system(command) == 0)
    {
        if (options->verbose) DEBUG_INFO("Rust compiler '%s' found", rustc_command());
        return true;
    }
    fprintf(stderr, "Error: Rust compiler '%s' is not installed or not in PATH.\n", rustc_command());
    fprintf(stderr, "Set SN_RUSTC to a different compiler, or use --emit-rust.\n");
    return false;
}

static bool array_is_empty(json_object *object, const char *key)
{
    json_object *array = NULL;
    return !json_object_object_get_ex(object, key, &array) ||
           json_object_array_length(array) == 0;
}

static bool rust_type_supported(json_object *type)
{
    json_object *kind_obj = NULL;
    if (!type || !json_object_object_get_ex(type, "kind", &kind_obj)) return false;
    const char *kind = json_object_get_string(kind_obj);
    if (!kind) return false;
    if (strcmp(kind, "array") == 0)
    {
        json_object *element_type = NULL;
        return json_object_object_get_ex(type, "element_type", &element_type) &&
               rust_type_supported(element_type);
    }
    return strcmp(kind, "void") == 0 || strcmp(kind, "int") == 0 ||
        strcmp(kind, "long") == 0 || strcmp(kind, "int32") == 0 ||
        strcmp(kind, "uint") == 0 || strcmp(kind, "uint32") == 0 ||
        strcmp(kind, "double") == 0 || strcmp(kind, "float") == 0 ||
        strcmp(kind, "bool") == 0 || strcmp(kind, "char") == 0 ||
        strcmp(kind, "byte") == 0 || strcmp(kind, "string") == 0 ||
        strcmp(kind, "struct") == 0;
}

static const char *json_string_property(json_object *object, const char *key)
{
    json_object *value = NULL;
    return object && json_object_object_get_ex(object, key, &value)
        ? json_object_get_string(value) : NULL;
}

static bool json_boolean_property(json_object *object, const char *key)
{
    json_object *value = NULL;
    return object && json_object_object_get_ex(object, key, &value) &&
           json_object_get_boolean(value);
}

static bool rust_validate_structs(json_object *model)
{
    json_object *structs = NULL;
    if (!json_object_object_get_ex(model, "structs", &structs)) return true;

    size_t count = json_object_array_length(structs);
    for (size_t i = 0; i < count; i++)
    {
        json_object *structure = json_object_array_get_idx(structs, i);
        const char *name = json_string_property(structure, "name");
        const char *mem_mode = json_string_property(structure, "mem_mode");
        json_object *methods = NULL, *fields = NULL;
        json_object_object_get_ex(structure, "methods", &methods);

        if (json_boolean_property(structure, "is_native") ||
            json_boolean_property(structure, "is_packed") ||
            json_boolean_property(structure, "is_serializable") ||
            (mem_mode && strcmp(mem_mode, "val") != 0) ||
            (methods && json_object_array_length(methods) > 0))
        {
            fprintf(stderr,
                    "Error: Rust target currently supports only plain value struct '%s'\n",
                    name ? name : "<anonymous>");
            return false;
        }

        if (!json_object_object_get_ex(structure, "fields", &fields)) continue;
        size_t field_count = json_object_array_length(fields);
        for (size_t f = 0; f < field_count; f++)
        {
            json_object *field = json_object_array_get_idx(fields, f);
            json_object *type = NULL;
            const char *field_name = json_string_property(field, "name");
            if (!json_object_object_get_ex(field, "type", &type) ||
                !rust_type_supported(type))
            {
                fprintf(stderr,
                        "Error: Rust target does not support field '%s.%s' yet\n",
                        name ? name : "<anonymous>",
                        field_name ? field_name : "<anonymous>");
                return false;
            }
        }
    }
    return true;
}

static bool rust_validate_expr(json_object *expr);

static bool rust_array_method_supported(const char *name)
{
    if (!name) return false;
    return strcmp(name, "push") == 0 || strcmp(name, "pop") == 0 ||
           strcmp(name, "insert") == 0 || strcmp(name, "remove") == 0 ||
           strcmp(name, "reverse") == 0 || strcmp(name, "clear") == 0 ||
           strcmp(name, "clone") == 0;
}

static bool rust_validate_expr_array(json_object *array)
{
    if (!array) return true;
    size_t count = json_object_array_length(array);
    for (size_t i = 0; i < count; i++)
        if (!rust_validate_expr(json_object_array_get_idx(array, i))) return false;
    return true;
}

static bool rust_validate_expr(json_object *expr)
{
    json_object *kind_obj = NULL;
    if (!expr || !json_object_object_get_ex(expr, "kind", &kind_obj)) return false;
    const char *kind = json_object_get_string(kind_obj);
    json_object *child = NULL;
    if (!kind) return false;

    if (strcmp(kind, "literal") == 0 || strcmp(kind, "variable") == 0)
        return true;
    if (strcmp(kind, "struct_literal") == 0)
    {
        json_object *fields = NULL;
        if (!json_object_object_get_ex(expr, "fields", &fields)) return true;
        size_t count = json_object_array_length(fields);
        for (size_t i = 0; i < count; i++)
        {
            json_object *field = json_object_array_get_idx(fields, i);
            if (!json_object_object_get_ex(field, "value", &child) ||
                !rust_validate_expr(child)) return false;
        }
        return true;
    }
    if (strcmp(kind, "array_literal") == 0)
    {
        json_object *elements = NULL;
        if (!json_object_object_get_ex(expr, "elements", &elements)) return true;
        size_t count = json_object_array_length(elements);
        for (size_t i = 0; i < count; i++)
        {
            json_object *element = json_object_array_get_idx(elements, i);
            const char *element_kind = json_string_property(element, "kind");
            if (!element_kind || strcmp(element_kind, "range") == 0 ||
                strcmp(element_kind, "spread") == 0 ||
                !rust_validate_expr(element)) return false;
        }
        return true;
    }
    if (strcmp(kind, "range") == 0)
    {
        json_object *start = NULL, *end = NULL;
        return json_object_object_get_ex(expr, "start", &start) &&
               json_object_object_get_ex(expr, "end", &end) &&
               rust_validate_expr(start) && rust_validate_expr(end);
    }
    if (strcmp(kind, "interpolated_string") == 0)
    {
        json_object *parts = NULL;
        if (!json_object_object_get_ex(expr, "parts", &parts)) return true;
        size_t count = json_object_array_length(parts);
        for (size_t i = 0; i < count; i++)
        {
            json_object *part = json_object_array_get_idx(parts, i);
            const char *part_kind = json_string_property(part, "kind");
            json_object *value = NULL, *format_spec = NULL;
            if (!part_kind) return false;
            if (strcmp(part_kind, "text") == 0) continue;
            if (strcmp(part_kind, "expr") != 0 ||
                json_object_object_get_ex(part, "format_spec", &format_spec) ||
                !json_object_object_get_ex(part, "expr", &value) ||
                !rust_validate_expr(value)) return false;
        }
        return true;
    }
    if (strcmp(kind, "sized_array") == 0)
    {
        json_object *element_type = NULL, *size = NULL;
        if (!json_object_object_get_ex(expr, "element_type", &element_type) ||
            !json_object_object_get_ex(expr, "size", &size) ||
            !rust_type_supported(element_type)) return false;
        const char *element_kind = json_string_property(element_type, "kind");
        return element_kind && strcmp(element_kind, "struct") != 0 &&
               rust_validate_expr(size);
    }
    if (strcmp(kind, "array_access") == 0)
    {
        json_object *array = NULL, *index = NULL;
        return json_object_object_get_ex(expr, "array", &array) &&
               json_object_object_get_ex(expr, "index", &index) &&
               rust_validate_expr(array) && rust_validate_expr(index);
    }
    if (strcmp(kind, "index_assign") == 0)
    {
        json_object *array = NULL, *index = NULL, *value = NULL;
        return json_object_object_get_ex(expr, "array", &array) &&
               json_object_object_get_ex(expr, "index", &index) &&
               json_object_object_get_ex(expr, "value", &value) &&
               rust_validate_expr(array) && rust_validate_expr(index) &&
               rust_validate_expr(value);
    }
    if (strcmp(kind, "builtin_length") == 0)
        return json_object_object_get_ex(expr, "object", &child) &&
               rust_validate_expr(child);
    if (strcmp(kind, "member") == 0)
        return json_object_object_get_ex(expr, "object", &child) &&
               rust_validate_expr(child);
    if (strcmp(kind, "member_assign") == 0)
    {
        json_object *object = NULL, *value = NULL;
        return json_object_object_get_ex(expr, "object", &object) &&
               json_object_object_get_ex(expr, "value", &value) &&
               rust_validate_expr(object) && rust_validate_expr(value);
    }
    if (strcmp(kind, "binary") == 0)
    {
        json_object *left = NULL, *right = NULL;
        return json_object_object_get_ex(expr, "left", &left) &&
               json_object_object_get_ex(expr, "right", &right) &&
               rust_validate_expr(left) && rust_validate_expr(right);
    }
    if (strcmp(kind, "unary") == 0 || strcmp(kind, "increment") == 0 ||
        strcmp(kind, "decrement") == 0)
        return json_object_object_get_ex(expr, "operand", &child) && rust_validate_expr(child);
    if (strcmp(kind, "assign") == 0)
        return json_object_object_get_ex(expr, "value", &child) && rust_validate_expr(child);
    if (strcmp(kind, "call") == 0)
    {
        json_object *callee = NULL, *args = NULL, *callee_kind = NULL;
        if (!json_object_object_get_ex(expr, "callee", &callee) ||
            !json_object_object_get_ex(callee, "kind", &callee_kind))
            return false;
        const char *callee_kind_name = json_object_get_string(callee_kind);
        if (!callee_kind_name) return false;
        if (strcmp(callee_kind_name, "member") == 0)
        {
            json_object *object = NULL, *object_type = NULL;
            const char *object_type_kind = NULL;
            if (!json_object_object_get_ex(callee, "object", &object) ||
                !json_object_object_get_ex(object, "type", &object_type) ||
                !(object_type_kind = json_string_property(object_type, "kind")) ||
                strcmp(object_type_kind, "array") != 0 ||
                !rust_array_method_supported(json_string_property(callee, "member_name")) ||
                !rust_validate_expr(object)) return false;
        }
        else if (strcmp(callee_kind_name, "variable") != 0) return false;
        json_object_object_get_ex(expr, "args", &args);
        return rust_validate_expr_array(args);
    }
    if (strcmp(kind, "builtin_print") == 0 || strcmp(kind, "builtin_println") == 0)
    {
        json_object *args = NULL;
        json_object_object_get_ex(expr, "args", &args);
        return rust_validate_expr_array(args);
    }
    return false;
}

static bool rust_model_uses_arrays(json_object *node)
{
    if (!node) return false;
    if (json_object_is_type(node, json_type_array))
    {
        size_t count = json_object_array_length(node);
        for (size_t i = 0; i < count; i++)
            if (rust_model_uses_arrays(json_object_array_get_idx(node, i))) return true;
        return false;
    }
    if (!json_object_is_type(node, json_type_object)) return false;

    const char *kind = json_string_property(node, "kind");
    if (kind && (strcmp(kind, "array") == 0 || strcmp(kind, "array_literal") == 0 ||
                 strcmp(kind, "array_access") == 0 || strcmp(kind, "index_assign") == 0 ||
                 strcmp(kind, "sized_array") == 0)) return true;

    json_object_object_foreach(node, key, value)
    {
        (void)key;
        if (rust_model_uses_arrays(value)) return true;
    }
    return false;
}

static bool rust_validate_stmt(json_object *stmt);

static bool json_tree_contains_kind(json_object *node, const char *wanted)
{
    if (!node) return false;
    if (json_object_is_type(node, json_type_array))
    {
        size_t count = json_object_array_length(node);
        for (size_t i = 0; i < count; i++)
            if (json_tree_contains_kind(json_object_array_get_idx(node, i), wanted))
                return true;
        return false;
    }
    if (!json_object_is_type(node, json_type_object)) return false;
    const char *kind = json_string_property(node, "kind");
    if (kind && strcmp(kind, wanted) == 0) return true;
    json_object_object_foreach(node, key, value)
    {
        (void)key;
        if (json_tree_contains_kind(value, wanted)) return true;
    }
    return false;
}

static bool rust_validate_statements(json_object *statements)
{
    if (!statements) return true;
    size_t count = json_object_array_length(statements);
    for (size_t i = 0; i < count; i++)
        if (!rust_validate_stmt(json_object_array_get_idx(statements, i))) return false;
    return true;
}

static bool rust_validate_block(json_object *block)
{
    json_object *statements = NULL;
    return block && json_object_object_get_ex(block, "statements", &statements) &&
           rust_validate_statements(statements);
}

static bool rust_validate_stmt(json_object *stmt)
{
    json_object *kind_obj = NULL;
    if (!stmt || !json_object_object_get_ex(stmt, "kind", &kind_obj)) return false;
    const char *kind = json_object_get_string(kind_obj);
    json_object *child = NULL;
    if (!kind) return false;
    if (strcmp(kind, "break") == 0 || strcmp(kind, "continue") == 0) return true;
    if (strcmp(kind, "return") == 0)
        return !json_object_object_get_ex(stmt, "value", &child) || rust_validate_expr(child);
    if (strcmp(kind, "expr") == 0)
        return json_object_object_get_ex(stmt, "expr", &child) && rust_validate_expr(child);
    if (strcmp(kind, "var_decl") == 0)
    {
        json_object *type = NULL;
        if (!json_object_object_get_ex(stmt, "type", &type) || !rust_type_supported(type)) return false;
        if (json_object_object_get_ex(stmt, "initializer", &child))
            return rust_validate_expr(child);
        const char *type_kind = json_string_property(type, "kind");
        return !type_kind || strcmp(type_kind, "struct") != 0;
    }
    if (strcmp(kind, "block") == 0) return rust_validate_block(stmt);
    if (strcmp(kind, "while") == 0)
    {
        json_object *condition = NULL, *body = NULL;
        return json_object_object_get_ex(stmt, "condition", &condition) &&
               json_object_object_get_ex(stmt, "body", &body) &&
               rust_validate_expr(condition) && rust_validate_block(body);
    }
    if (strcmp(kind, "for") == 0)
    {
        json_object *init = NULL, *condition = NULL, *increment = NULL, *body = NULL;
        return json_object_object_get_ex(stmt, "init", &init) &&
               json_object_object_get_ex(stmt, "condition", &condition) &&
               json_object_object_get_ex(stmt, "increment", &increment) &&
               json_object_object_get_ex(stmt, "body", &body) &&
               !json_tree_contains_kind(body, "continue") &&
               rust_validate_stmt(init) && rust_validate_expr(condition) &&
               rust_validate_expr(increment) && rust_validate_block(body);
    }
    if (strcmp(kind, "for_each") == 0)
    {
        json_object *iterable = NULL, *body = NULL;
        return json_object_object_get_ex(stmt, "iterable", &iterable) &&
               json_object_object_get_ex(stmt, "body", &body) &&
               rust_validate_expr(iterable) && rust_validate_block(body);
    }
    if (strcmp(kind, "if") == 0)
    {
        json_object *condition = NULL, *then_body = NULL, *else_body = NULL;
        if (!json_object_object_get_ex(stmt, "condition", &condition) ||
            !json_object_object_get_ex(stmt, "then_body", &then_body) ||
            !rust_validate_expr(condition) || !rust_validate_block(then_body)) return false;
        return !json_object_object_get_ex(stmt, "else_body", &else_body) ||
               rust_validate_block(else_body);
    }
    return false;
}

static bool rust_validate_model(json_object *model)
{
    const char *unsupported = NULL;
    if (!array_is_empty(model, "globals")) unsupported = "global variables";
    else if (!array_is_empty(model, "lambdas")) unsupported = "closures";
    else if (!array_is_empty(model, "threads")) unsupported = "threads";
    else if (!array_is_empty(model, "type_decls")) unsupported = "type declarations";

    json_object *pragmas = NULL;
    if (!unsupported && json_object_object_get_ex(model, "pragmas", &pragmas))
    {
        size_t count = json_object_array_length(pragmas);
        for (size_t i = 0; i < count; i++)
        {
            json_object *pragma = json_object_array_get_idx(pragmas, i);
            json_object *kind = NULL;
            if (json_object_object_get_ex(pragma, "pragma_type", &kind))
            {
                const char *value = json_object_get_string(kind);
                if (value && (strcmp(value, "source") == 0 || strcmp(value, "include") == 0))
                {
                    unsupported = "native C source/include pragmas";
                    break;
                }
            }
        }
    }

    if (unsupported)
    {
        fprintf(stderr, "Error: Rust target does not support %s yet\n", unsupported);
        return false;
    }

    if (!rust_validate_structs(model)) return false;

    json_object *functions = NULL;
    if (json_object_object_get_ex(model, "functions", &functions))
    {
        size_t count = json_object_array_length(functions);
        for (size_t i = 0; i < count; i++)
        {
            json_object *function = json_object_array_get_idx(functions, i);
            json_object *name_obj = NULL, *return_type = NULL, *params = NULL, *body = NULL;
            json_object *is_native = NULL;
            const char *name = json_object_object_get_ex(function, "name", &name_obj)
                ? json_object_get_string(name_obj) : "<anonymous>";
            if ((json_object_object_get_ex(function, "is_native", &is_native) &&
                 json_object_get_boolean(is_native)) ||
                !json_object_object_get_ex(function, "return_type", &return_type) ||
                !rust_type_supported(return_type))
            {
                fprintf(stderr, "Error: Rust target does not support function '%s' yet\n", name);
                return false;
            }
            json_object *return_kind = NULL;
            if (strcmp(name, "main") == 0 &&
                json_object_object_get_ex(return_type, "kind", &return_kind) &&
                strcmp(json_object_get_string(return_kind), "void") != 0)
            {
                fprintf(stderr, "Error: Rust target currently requires main to return void\n");
                return false;
            }
            if (json_object_object_get_ex(function, "params", &params))
            {
                size_t param_count = json_object_array_length(params);
                for (size_t p = 0; p < param_count; p++)
                {
                    json_object *param = json_object_array_get_idx(params, p);
                    json_object *param_type = NULL;
                    if (!json_object_object_get_ex(param, "type", &param_type) ||
                        !rust_type_supported(param_type))
                    {
                        fprintf(stderr, "Error: Rust target does not support a parameter of function '%s'\n", name);
                        return false;
                    }
                }
            }
            json_object_object_get_ex(function, "body", &body);
            if (!rust_validate_statements(body))
            {
                fprintf(stderr, "Error: Rust target encountered an unsupported construct in function '%s'\n", name);
                return false;
            }
        }
    }
    return true;
}

/* Annotate target-neutral binary nodes with the Rust checked-arithmetic method
 * selected by this backend. Templates remain declarative and other targets do
 * not need to understand Rust's checked_* APIs. */
static void rust_lower_checked_arithmetic(json_object *node)
{
    if (!node) return;
    if (json_object_is_type(node, json_type_array))
    {
        size_t count = json_object_array_length(node);
        for (size_t i = 0; i < count; i++)
            rust_lower_checked_arithmetic(json_object_array_get_idx(node, i));
        return;
    }
    if (!json_object_is_type(node, json_type_object)) return;

    json_object_object_foreach(node, key, value)
    {
        (void)key;
        rust_lower_checked_arithmetic(value);
    }

    json_object *kind_obj = NULL, *mode_obj = NULL, *op_obj = NULL, *type_obj = NULL;
    if (!json_object_object_get_ex(node, "kind", &kind_obj) ||
        strcmp(json_object_get_string(kind_obj), "binary") != 0 ||
        !json_object_object_get_ex(node, "arithmetic_mode", &mode_obj) ||
        strcmp(json_object_get_string(mode_obj), "checked") != 0 ||
        !json_object_object_get_ex(node, "op", &op_obj) ||
        !json_object_object_get_ex(node, "type", &type_obj))
        return;

    json_object *type_kind_obj = NULL;
    if (!json_object_object_get_ex(type_obj, "kind", &type_kind_obj)) return;
    const char *type_kind = json_object_get_string(type_kind_obj);
    if (!type_kind || (strcmp(type_kind, "int") != 0 && strcmp(type_kind, "long") != 0 &&
        strcmp(type_kind, "int32") != 0 && strcmp(type_kind, "uint") != 0 &&
        strcmp(type_kind, "uint32") != 0 && strcmp(type_kind, "byte") != 0))
        return;

    const char *op = json_object_get_string(op_obj);
    const char *method = NULL;
    if (strcmp(op, "add") == 0) method = "checked_add";
    else if (strcmp(op, "subtract") == 0) method = "checked_sub";
    else if (strcmp(op, "multiply") == 0) method = "checked_mul";
    else if (strcmp(op, "divide") == 0) method = "checked_div";
    else if (strcmp(op, "modulo") == 0) method = "checked_rem";
    if (method)
        json_object_object_add(node, "rust_checked_method", json_object_new_string(method));
}

static bool rust_emit(CompilerOptions *options, Module *module,
                      TargetEmitMode mode, GeneratedFileSet *result)
{
    (void)mode;
    json_object *model = gen_model_build(&options->arena, module,
                                          &options->symbol_table,
                                          options->arithmetic_mode);
    if (!model) return false;
    if (!rust_validate_model(model))
    {
        json_object_put(model);
        return false;
    }
    rust_lower_checked_arithmetic(model);
    if (rust_model_uses_arrays(model))
        json_object_object_add(model, "rust_uses_arrays", json_object_new_boolean(true));

    char template_dir[1024];
    snprintf(template_dir, sizeof(template_dir), "%s/templates/rust", options->compiler_dir);
    char *code = rust_render_model(model, template_dir);
    json_object_put(model);
    if (!code) return false;
    if (!generated_file_set_add(result, "main.rs", code, GENERATED_SOURCE, true))
    {
        free(code);
        return false;
    }
    return true;
}

static bool rust_build(const CompilerOptions *options, const char *build_dir,
                       const GeneratedFileSet *files)
{
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
    snprintf(command, sizeof(command), "%s --edition=2021 %s %s \"%s\" -o \"%s\"",
             rustc_command(), profile_flags, rustflags, source_path,
             options->executable_file);
    if (options->verbose) DEBUG_INFO("Executing: %s", command);
    if (system(command) != 0)
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
