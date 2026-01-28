#include "code_gen.h"
#include "code_gen/code_gen_util.h"
#include "code_gen/code_gen_expr.h"
#include "code_gen/code_gen_stmt.h"
#include "debug.h"
#include "parser.h"
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include "arena.h"

void code_gen_init(Arena *arena, CodeGen *gen, SymbolTable *symbol_table, const char *output_file)
{
    DEBUG_VERBOSE("Entering code_gen_init");
    gen->arena = arena;
    gen->label_count = 0;
    gen->symbol_table = symbol_table;
    gen->output = fopen(output_file, "w");
    gen->current_function = NULL;
    gen->current_return_type = NULL;
    gen->temp_count = 0;
    gen->for_continue_label = NULL;

    /* Initialize arena context fields */
    gen->arena_depth = 0;
    gen->in_shared_context = false;
    gen->in_private_context = false;
    gen->current_arena_var = NULL;
    gen->current_func_modifier = FUNC_DEFAULT;

    /* Initialize loop counter tracking for optimization */
    gen->loop_counter_names = NULL;
    gen->loop_counter_count = 0;
    gen->loop_counter_capacity = 0;

    /* Initialize private block arena stack */
    gen->arena_stack = NULL;
    gen->arena_stack_depth = 0;
    gen->arena_stack_capacity = 0;

    /* Initialize lambda fields */
    gen->lambda_count = 0;
    gen->lambda_forward_decls = arena_strdup(arena, "");
    gen->lambda_definitions = arena_strdup(arena, "");
    gen->enclosing_lambdas = NULL;
    gen->enclosing_lambda_count = 0;
    gen->enclosing_lambda_capacity = 0;

    /* Initialize thread wrapper support */
    gen->thread_wrapper_count = 0;

    /* Initialize named function wrapper support */
    gen->wrapper_count = 0;

    /* Initialize buffering fields */
    gen->function_definitions = arena_strdup(arena, "");
    gen->buffering_functions = false;

    /* Initialize optimization settings */
    gen->arithmetic_mode = ARITH_CHECKED;  /* Default to checked arithmetic */

    /* Initialize tail call optimization state */
    gen->in_tail_call_function = false;
    gen->tail_call_fn = NULL;

    /* Initialize captured primitive tracking */
    gen->captured_primitives = NULL;
    gen->captured_prim_ptrs = NULL;
    gen->captured_prim_count = 0;
    gen->captured_prim_capacity = 0;

    /* Initialize pragma tracking */
    gen->pragma_includes = NULL;
    gen->pragma_include_count = 0;
    gen->pragma_include_capacity = 0;
    gen->pragma_links = NULL;
    gen->pragma_link_count = 0;
    gen->pragma_link_capacity = 0;
    gen->pragma_sources = NULL;
    gen->pragma_source_count = 0;
    gen->pragma_source_capacity = 0;

    /* Initialize match expression support */
    gen->match_count = 0;

    /* Initialize interceptor thunk support */
    gen->thunk_count = 0;
    gen->thunk_forward_decls = arena_strdup(arena, "");
    gen->thunk_definitions = arena_strdup(arena, "");

    /* Initialize array compound literal context flag */
    gen->in_array_compound_literal = false;

    /* Initialize handle-mode flag for expressions */
    gen->expr_as_handle = false;

    /* Initialize namespace prefix for imported module code generation */
    gen->current_namespace_prefix = NULL;
    gen->current_canonical_module = NULL;

    /* Initialize pin counter */
    gen->pin_counter = 0;

    /* Initialize return closure allocation flag */
    gen->allocate_closure_in_caller_arena = false;

    /* Initialize recursive lambda support */
    gen->current_decl_var_name = NULL;
    gen->recursive_lambda_id = -1;

    /* Initialize deferred global initialization tracking */
    gen->deferred_global_names = NULL;
    gen->deferred_global_values = NULL;
    gen->deferred_global_count = 0;
    gen->deferred_global_capacity = 0;

    /* Initialize struct method emission tracking */
    gen->emitted_struct_methods = NULL;
    gen->emitted_struct_methods_count = 0;
    gen->emitted_struct_methods_capacity = 0;

    /* Initialize static global variable emission tracking */
    gen->emitted_static_globals = NULL;
    gen->emitted_static_globals_count = 0;
    gen->emitted_static_globals_capacity = 0;

    /* Initialize function emission tracking (for diamond imports) */
    gen->emitted_functions = NULL;
    gen->emitted_functions_count = 0;
    gen->emitted_functions_capacity = 0;

    /* Initialize global variable emission tracking (for diamond imports) */
    gen->emitted_globals = NULL;
    gen->emitted_globals_count = 0;
    gen->emitted_globals_capacity = 0;

    if (gen->output == NULL)
    {
        exit(1);
    }
}

void code_gen_cleanup(CodeGen *gen)
{
    DEBUG_VERBOSE("Entering code_gen_cleanup");
    if (gen->output != NULL)
    {
        fclose(gen->output);
    }
    gen->current_function = NULL;
}

int code_gen_new_label(CodeGen *gen)
{
    DEBUG_VERBOSE("Entering code_gen_new_label");
    return gen->label_count++;
}

static void code_gen_headers(CodeGen *gen)
{
    DEBUG_VERBOSE("Entering code_gen_headers");
    indented_fprintf(gen, 0, "#include <stdlib.h>\n");
    indented_fprintf(gen, 0, "#include <string.h>\n");
    indented_fprintf(gen, 0, "#include <stdio.h>\n");
    indented_fprintf(gen, 0, "#include <stdbool.h>\n");
    indented_fprintf(gen, 0, "#include <stdint.h>\n");  /* For int32_t, uint32_t, uint64_t */
    indented_fprintf(gen, 0, "#include <limits.h>\n");
    indented_fprintf(gen, 0, "#include <setjmp.h>\n");  /* For thread panic handling */
    /* pthread.h is included via runtime.h -> runtime_thread.h (handles Windows compatibility) */
    /* Include runtime.h for inline function definitions (comparisons, array_length, etc.) */
    indented_fprintf(gen, 0, "#include \"runtime.h\"\n");
    /* Undefine Windows min/max macros to avoid name collisions with user functions */
    indented_fprintf(gen, 0, "#ifdef _WIN32\n");
    indented_fprintf(gen, 0, "#undef min\n");
    indented_fprintf(gen, 0, "#undef max\n");
    indented_fprintf(gen, 0, "#endif\n\n");
}

static void code_gen_externs(CodeGen *gen)
{
    DEBUG_VERBOSE("Entering code_gen_externs");

    /* Types already provided by runtime.h includes:
     * RtHandle, RT_HANDLE_NULL, RtManagedArena, RtArena, RtArrayMetadata,
     * and all managed arena / runtime function declarations.
     * Only need __Closure__ typedef and non-header function declarations below. */
    indented_fprintf(gen, 0, "\n");

    /* Generic closure type for lambdas */
    indented_fprintf(gen, 0, "/* Closure type for lambdas */\n");
    indented_fprintf(gen, 0, "typedef struct __Closure__ { void *fn; RtArena *arena; size_t size; } __Closure__;\n\n");
}

/* Generate C typedef for a native callback type declaration.
 * For type X = native fn(a: int, b: double): void
 * Generates: typedef void (*X)(long, double);
 */
static void code_gen_native_callback_typedef(CodeGen *gen, TypeDeclStmt *type_decl)
{
    Type *type = type_decl->type;

    /* Only handle native function types */
    if (type->kind != TYPE_FUNCTION || !type->as.function.is_native)
    {
        return;
    }

    /* Get the type alias name */
    char *type_name = get_var_name(gen->arena, type_decl->name);

    /* Get the C return type */
    const char *ret_c = get_c_type(gen->arena, type->as.function.return_type);

    /* Generate: typedef ret_type (*TypeName)(param_types); */
    indented_fprintf(gen, 0, "typedef %s (*%s)(", ret_c, type_name);

    /* Generate parameter types */
    for (int i = 0; i < type->as.function.param_count; i++)
    {
        if (i > 0)
        {
            fprintf(gen->output, ", ");
        }
        const char *param_c = get_c_type(gen->arena, type->as.function.param_types[i]);
        fprintf(gen->output, "%s", param_c);
    }

    /* Handle empty parameter list */
    if (type->as.function.param_count == 0)
    {
        fprintf(gen->output, "void");
    }

    fprintf(gen->output, ");\n");
}

static void code_gen_forward_declaration(CodeGen *gen, FunctionStmt *fn)
{
    char *raw_fn_name = get_var_name(gen->arena, fn->name);

    // Skip main - it doesn't need a forward declaration
    if (strcmp(raw_fn_name, "main") == 0)
    {
        return;
    }

    /* Native functions without a body are external C declarations.
     * We don't generate any forward declaration - they must be provided via
     * #pragma include or linked via #pragma link. */
    if (fn->is_native && fn->body_count == 0)
    {
        return;
    }

    /* Native functions keep their raw name (no mangling) */
    char *fn_name = fn->is_native ? raw_fn_name : sn_mangle_name(gen->arena, raw_fn_name);

    /* New arena model: ALL non-main Sindarin functions receive __caller_arena__ as first param.
     * This is true regardless of whether they are shared, default, or private.
     * The modifier only affects how the local arena is set up inside the function. */
    const char *ret_c = get_c_type(gen->arena, fn->return_type);
    indented_fprintf(gen, 0, "%s %s(", ret_c, fn_name);

    /* All non-main functions receive caller's arena as first parameter */
    fprintf(gen->output, "RtManagedArena *");
    if (fn->param_count > 0)
    {
        fprintf(gen->output, ", ");
    }

    for (int i = 0; i < fn->param_count; i++)
    {
        const char *param_type = get_c_param_type(gen->arena, fn->params[i].type);
        if (i > 0)
        {
            fprintf(gen->output, ", ");
        }
        /* 'as ref' primitive and struct parameters become pointer types */
        bool is_ref_param = false;
        if (fn->params[i].mem_qualifier == MEM_AS_REF && fn->params[i].type != NULL)
        {
            TypeKind kind = fn->params[i].type->kind;
            bool is_prim = (kind == TYPE_INT || kind == TYPE_INT32 || kind == TYPE_UINT ||
                           kind == TYPE_UINT32 || kind == TYPE_LONG || kind == TYPE_DOUBLE ||
                           kind == TYPE_FLOAT || kind == TYPE_CHAR || kind == TYPE_BOOL ||
                           kind == TYPE_BYTE);
            bool is_struct = (kind == TYPE_STRUCT);
            is_ref_param = is_prim || is_struct;
        }
        if (is_ref_param)
        {
            fprintf(gen->output, "%s *", param_type);
        }
        else
        {
            fprintf(gen->output, "%s", param_type);
        }
    }

    /* With new arena model, ALL non-main functions have at least the arena param,
     * so we never need to output "void" for empty parameter list */

    fprintf(gen->output, ");\n");
}

/**
 * Check if a function name is a C standard library function declared in
 * headers that the generated code always includes (stdlib.h, string.h,
 * stdio.h, stdint.h, limits.h, setjmp.h).
 * We must not emit conflicting extern declarations for these.
 */
static bool is_c_stdlib_function(const char *name)
{
    static const char *stdlib_names[] = {
        /* stdlib.h */
        "atoi", "atol", "atoll", "atof", "strtol", "strtoll", "strtoul",
        "strtoull", "strtod", "strtof", "strtold", "malloc", "calloc",
        "realloc", "free", "abort", "exit", "_Exit", "atexit", "at_quick_exit",
        "quick_exit", "system", "getenv", "abs", "labs", "llabs", "div",
        "ldiv", "lldiv", "rand", "srand", "qsort", "bsearch", "mblen",
        "mbtowc", "wctomb", "mbstowcs", "wcstombs",
        /* string.h */
        "strlen", "strcmp", "strncmp", "strcpy", "strncpy", "strcat",
        "strncat", "memcpy", "memmove", "memcmp", "memset", "memchr",
        "strchr", "strrchr", "strstr", "strtok", "strerror", "strpbrk",
        "strspn", "strcspn", "strcoll", "strxfrm",
        /* stdio.h */
        "printf", "fprintf", "sprintf", "snprintf", "vprintf", "vfprintf",
        "vsprintf", "vsnprintf", "scanf", "fscanf", "sscanf", "fopen",
        "fclose", "fread", "fwrite", "fgets", "fputs", "gets", "puts",
        "getchar", "putchar", "getc", "putc", "fgetc", "fputc", "ungetc",
        "fseek", "ftell", "rewind", "feof", "ferror", "clearerr", "perror",
        "remove", "rename", "tmpfile", "tmpnam", "fflush", "freopen",
        "setbuf", "setvbuf", "fgetpos", "fsetpos",
        /* setjmp.h */
        "setjmp", "longjmp",
        NULL
    };
    for (int i = 0; stdlib_names[i] != NULL; i++)
    {
        if (strcmp(name, stdlib_names[i]) == 0)
        {
            return true;
        }
    }
    return false;
}

/* Generate extern declaration for native function without body */
static void code_gen_native_extern_declaration(CodeGen *gen, FunctionStmt *fn)
{
    char *fn_name = get_var_name(gen->arena, fn->name);
    /* Native functions with arena param use handle-based types:
     * TYPE_STRING → RtHandle, TYPE_ARRAY → element_type *
     * Native functions without arena use raw C types:
     * TYPE_STRING → char *, TYPE_ARRAY → element_type * */
    const char *ret_c;
    if (fn->return_type && fn->return_type->kind == TYPE_STRING && fn->has_arena_param) {
        /* String returns with arena use RtHandle (handle-based strings) */
        ret_c = "RtHandle";
    } else if (fn->return_type && fn->return_type->kind == TYPE_STRING) {
        ret_c = "char *";
    } else if (fn->return_type && fn->return_type->kind == TYPE_ARRAY && fn->has_arena_param) {
        /* Array returns with arena use RtHandle (handle-based arrays) */
        ret_c = "RtHandle";
    } else if (fn->return_type && fn->return_type->kind == TYPE_ARRAY) {
        const char *elem_c = get_c_array_elem_type(gen->arena, fn->return_type->as.array.element_type);
        ret_c = arena_sprintf(gen->arena, "%s *", elem_c);
    } else {
        ret_c = get_c_type(gen->arena, fn->return_type);
    }

    indented_fprintf(gen, 0, "extern %s %s(", ret_c, fn_name);

    /* If function has implicit arena param, prepend RtManagedArena* for managed arena system */
    bool has_other_params = fn->param_count > 0 || fn->is_variadic;
    if (fn->has_arena_param)
    {
        fprintf(gen->output, "RtManagedArena *");
        if (has_other_params)
        {
            fprintf(gen->output, ", ");
        }
    }

    for (int i = 0; i < fn->param_count; i++)
    {
        const char *param_type = get_c_native_param_type(gen->arena, fn->params[i].type);
        if (i > 0)
        {
            fprintf(gen->output, ", ");
        }
        /* 'as ref' primitive and struct parameters become pointer types */
        bool is_ref_param = false;
        if (fn->params[i].mem_qualifier == MEM_AS_REF && fn->params[i].type != NULL)
        {
            TypeKind kind = fn->params[i].type->kind;
            bool is_prim = (kind == TYPE_INT || kind == TYPE_INT32 || kind == TYPE_UINT ||
                           kind == TYPE_UINT32 || kind == TYPE_LONG || kind == TYPE_DOUBLE ||
                           kind == TYPE_FLOAT || kind == TYPE_CHAR || kind == TYPE_BOOL ||
                           kind == TYPE_BYTE);
            bool is_struct = (kind == TYPE_STRUCT);
            is_ref_param = is_prim || is_struct;
        }
        if (is_ref_param)
        {
            fprintf(gen->output, "%s *", param_type);
        }
        else
        {
            fprintf(gen->output, "%s", param_type);
        }
    }

    /* Add variadic marker if function is variadic */
    if (fn->is_variadic)
    {
        if (fn->param_count > 0)
        {
            fprintf(gen->output, ", ...");
        }
        else
        {
            fprintf(gen->output, "...");
        }
    }
    else if (fn->param_count == 0 && !fn->has_arena_param)
    {
        fprintf(gen->output, "void");
    }

    fprintf(gen->output, ");\n");
}

/* Helper to add a pragma include (with deduplication) */
static void code_gen_add_pragma_include(CodeGen *gen, const char *include)
{
    /* Check for duplicates */
    for (int i = 0; i < gen->pragma_include_count; i++)
    {
        if (strcmp(gen->pragma_includes[i], include) == 0)
        {
            return; /* Already added */
        }
    }

    if (gen->pragma_include_count >= gen->pragma_include_capacity)
    {
        int new_capacity = gen->pragma_include_capacity == 0 ? 8 : gen->pragma_include_capacity * 2;
        char **new_includes = arena_alloc(gen->arena, new_capacity * sizeof(char *));
        for (int i = 0; i < gen->pragma_include_count; i++)
        {
            new_includes[i] = gen->pragma_includes[i];
        }
        gen->pragma_includes = new_includes;
        gen->pragma_include_capacity = new_capacity;
    }
    gen->pragma_includes[gen->pragma_include_count++] = arena_strdup(gen->arena, include);
}

/* Helper to add a pragma link (with deduplication) */
static void code_gen_add_pragma_link(CodeGen *gen, const char *link)
{
    /* Check for duplicates */
    for (int i = 0; i < gen->pragma_link_count; i++)
    {
        if (strcmp(gen->pragma_links[i], link) == 0)
        {
            return; /* Already added */
        }
    }

    if (gen->pragma_link_count >= gen->pragma_link_capacity)
    {
        int new_capacity = gen->pragma_link_capacity == 0 ? 8 : gen->pragma_link_capacity * 2;
        char **new_links = arena_alloc(gen->arena, new_capacity * sizeof(char *));
        for (int i = 0; i < gen->pragma_link_count; i++)
        {
            new_links[i] = gen->pragma_links[i];
        }
        gen->pragma_links = new_links;
        gen->pragma_link_capacity = new_capacity;
    }
    gen->pragma_links[gen->pragma_link_count++] = arena_strdup(gen->arena, link);
}

/* Helper to extract directory from a file path */
static const char *get_directory_from_path(Arena *arena, const char *filepath)
{
    if (filepath == NULL) return ".";

    char *path_copy = arena_strdup(arena, filepath);
    char *last_sep = strrchr(path_copy, '/');
#ifdef _WIN32
    char *last_win_sep = strrchr(path_copy, '\\');
    if (last_win_sep > last_sep) last_sep = last_win_sep;
#endif

    if (last_sep != NULL)
    {
        *last_sep = '\0';
        return path_copy;
    }
    return ".";
}

/* Helper to add a pragma source file with location info (with deduplication) */
static void code_gen_add_pragma_source(CodeGen *gen, const char *source, const char *source_dir)
{
    /* Check for duplicates (same value and source_dir) */
    for (int i = 0; i < gen->pragma_source_count; i++)
    {
        if (strcmp(gen->pragma_sources[i].value, source) == 0 &&
            strcmp(gen->pragma_sources[i].source_dir, source_dir) == 0)
        {
            return; /* Already added */
        }
    }

    if (gen->pragma_source_count >= gen->pragma_source_capacity)
    {
        int new_capacity = gen->pragma_source_capacity == 0 ? 8 : gen->pragma_source_capacity * 2;
        PragmaSourceInfo *new_sources = arena_alloc(gen->arena, new_capacity * sizeof(PragmaSourceInfo));
        for (int i = 0; i < gen->pragma_source_count; i++)
        {
            new_sources[i] = gen->pragma_sources[i];
        }
        gen->pragma_sources = new_sources;
        gen->pragma_source_capacity = new_capacity;
    }
    gen->pragma_sources[gen->pragma_source_count].value = arena_strdup(gen->arena, source);
    gen->pragma_sources[gen->pragma_source_count].source_dir = arena_strdup(gen->arena, source_dir);
    gen->pragma_source_count++;
}

/* Helper to emit a single struct typedef */
static void code_gen_emit_struct_typedef(CodeGen *gen, StructDeclStmt *struct_decl, int *count_ptr)
{
    /* Skip native structs that have a c_alias - they are aliases to external types */
    if (struct_decl->is_native && struct_decl->c_alias != NULL)
    {
        return;
    }

    char *raw_struct_name = arena_strndup(gen->arena, struct_decl->name.start, struct_decl->name.length);
    char *struct_name = sn_mangle_name(gen->arena, raw_struct_name);

    /* Check if this struct has already been emitted (avoid duplicates from aliased imports) */
    for (int i = 0; i < gen->emitted_struct_methods_count; i++)
    {
        if (strcmp(gen->emitted_struct_methods[i], struct_name) == 0)
        {
            return; /* Already emitted */
        }
    }

    /* Track this struct as emitted */
    if (gen->emitted_struct_methods_count >= gen->emitted_struct_methods_capacity)
    {
        int new_capacity = gen->emitted_struct_methods_capacity == 0 ? 8 : gen->emitted_struct_methods_capacity * 2;
        const char **new_array = arena_alloc(gen->arena, sizeof(const char *) * new_capacity);
        for (int i = 0; i < gen->emitted_struct_methods_count; i++)
        {
            new_array[i] = gen->emitted_struct_methods[i];
        }
        gen->emitted_struct_methods = new_array;
        gen->emitted_struct_methods_capacity = new_capacity;
    }
    gen->emitted_struct_methods[gen->emitted_struct_methods_count++] = struct_name;

    if (*count_ptr == 0)
    {
        indented_fprintf(gen, 0, "/* Struct type definitions */\n");
    }

    /* Emit #pragma pack for packed structs */
    if (struct_decl->is_packed)
    {
        indented_fprintf(gen, 0, "#pragma pack(push, 1)\n");
    }

    /* Generate: typedef struct { fields... } StructName; */
    indented_fprintf(gen, 0, "typedef struct {\n");
    for (int j = 0; j < struct_decl->field_count; j++)
    {
        StructField *field = &struct_decl->fields[j];
        const char *c_type = get_c_type(gen->arena, field->type);
        const char *c_field_name = field->c_alias != NULL
            ? field->c_alias
            : sn_mangle_name(gen->arena, field->name);
        indented_fprintf(gen, 1, "%s %s;\n", c_type, c_field_name);
    }
    indented_fprintf(gen, 0, "} %s;\n", struct_name);

    /* Close #pragma pack for packed structs */
    if (struct_decl->is_packed)
    {
        indented_fprintf(gen, 0, "#pragma pack(pop)\n");
    }

    (*count_ptr)++;
}

/* Recursively emit struct typedefs from imported modules */
static void code_gen_emit_imported_struct_typedefs(CodeGen *gen, Stmt **statements, int count, int *struct_count)
{
    for (int i = 0; i < count; i++)
    {
        Stmt *stmt = statements[i];
        if (stmt->type == STMT_STRUCT_DECL)
        {
            code_gen_emit_struct_typedef(gen, &stmt->as.struct_decl, struct_count);
        }
        else if (stmt->type == STMT_IMPORT && stmt->as.import.imported_stmts != NULL)
        {
            /* Recursively process imported modules */
            code_gen_emit_imported_struct_typedefs(gen, stmt->as.import.imported_stmts,
                                                   stmt->as.import.imported_count, struct_count);
        }
    }
}

/* Tracking structure for emitted native alias forward declarations */
typedef struct {
    const char **names;
    int count;
    int capacity;
} EmittedNativeAliases;

/* Helper to emit native struct forward declaration (with deduplication) */
static void code_gen_emit_native_alias_fwd(CodeGen *gen, StructDeclStmt *struct_decl,
                                           int *count_ptr, EmittedNativeAliases *emitted)
{
    if (!struct_decl->is_native || struct_decl->c_alias == NULL)
    {
        return;
    }

    /* Check if already emitted */
    for (int i = 0; i < emitted->count; i++)
    {
        if (strcmp(emitted->names[i], struct_decl->c_alias) == 0)
        {
            return;
        }
    }

    /* Track this alias */
    if (emitted->count >= emitted->capacity)
    {
        int new_cap = emitted->capacity == 0 ? 8 : emitted->capacity * 2;
        const char **new_arr = arena_alloc(gen->arena, sizeof(const char *) * new_cap);
        for (int i = 0; i < emitted->count; i++)
        {
            new_arr[i] = emitted->names[i];
        }
        emitted->names = new_arr;
        emitted->capacity = new_cap;
    }
    emitted->names[emitted->count++] = struct_decl->c_alias;

    if (*count_ptr == 0)
    {
        indented_fprintf(gen, 0, "/* Native struct forward declarations */\n");
    }
    indented_fprintf(gen, 0, "typedef struct %s %s;\n",
        struct_decl->c_alias, struct_decl->c_alias);
    (*count_ptr)++;
}

/* Recursively emit native struct forward declarations from imports */
static void code_gen_emit_imported_native_aliases(CodeGen *gen, Stmt **statements, int count,
                                                   int *alias_count, EmittedNativeAliases *emitted)
{
    for (int i = 0; i < count; i++)
    {
        Stmt *stmt = statements[i];
        if (stmt->type == STMT_STRUCT_DECL)
        {
            code_gen_emit_native_alias_fwd(gen, &stmt->as.struct_decl, alias_count, emitted);
        }
        else if (stmt->type == STMT_IMPORT && stmt->as.import.imported_stmts != NULL)
        {
            code_gen_emit_imported_native_aliases(gen, stmt->as.import.imported_stmts,
                                                   stmt->as.import.imported_count, alias_count, emitted);
        }
    }
}

/* Tracking structure for emitted native extern declarations */
typedef struct {
    const char **names;
    int count;
    int capacity;
} EmittedNativeExterns;

/* Helper to check if an extern has already been emitted */
static bool is_extern_already_emitted(EmittedNativeExterns *emitted, const char *fn_name)
{
    for (int i = 0; i < emitted->count; i++)
    {
        if (strcmp(emitted->names[i], fn_name) == 0)
        {
            return true;
        }
    }
    return false;
}

/* Helper to track an emitted extern */
static void track_emitted_extern(Arena *arena, EmittedNativeExterns *emitted, const char *fn_name)
{
    if (emitted->count >= emitted->capacity)
    {
        int new_cap = emitted->capacity == 0 ? 16 : emitted->capacity * 2;
        const char **new_arr = arena_alloc(arena, sizeof(const char *) * new_cap);
        for (int i = 0; i < emitted->count; i++)
        {
            new_arr[i] = emitted->names[i];
        }
        emitted->names = new_arr;
        emitted->capacity = new_cap;
    }
    emitted->names[emitted->count++] = fn_name;
}

/* Check if a list of statements contains an @include pragma */
static bool has_include_pragma(Stmt **statements, int count)
{
    for (int i = 0; i < count; i++)
    {
        if (statements[i]->type == STMT_PRAGMA &&
            statements[i]->as.pragma.pragma_type == PRAGMA_INCLUDE)
        {
            return true;
        }
    }
    return false;
}

/* Recursively emit native extern declarations from imported modules (with deduplication) */
static void code_gen_emit_imported_native_externs_recursive(
    CodeGen *gen, Stmt **statements, int count, int *extern_count,
    EmittedNativeExterns *emitted, bool skip_due_to_include)
{
    /* If this module has @include pragma, its native functions are declared in the header */
    if (!skip_due_to_include && has_include_pragma(statements, count))
    {
        skip_due_to_include = true;
    }

    for (int i = 0; i < count; i++)
    {
        Stmt *stmt = statements[i];
        if (stmt->type == STMT_FUNCTION)
        {
            FunctionStmt *fn = &stmt->as.function;
            if (fn->is_native && fn->body_count == 0)
            {
                /* Get function name for checking */
                char *fn_name = arena_strndup(gen->arena, fn->name.start, fn->name.length);

                /* Skip if module has @include - header provides declarations.
                 * BUT: still emit declarations for sn_* functions, as these are
                 * typically defined in @source files, not in the included header. */
                if (skip_due_to_include && strncmp(fn_name, "sn_", 3) != 0)
                {
                    continue;
                }
                /* Skip extern declaration for runtime functions (starting with rt_)
                 * since they are already declared in runtime headers. */
                if (strncmp(fn_name, "rt_", 3) == 0)
                {
                    continue;
                }
                /* Skip extern declaration for C standard library functions that are
                 * already declared in the always-included headers (stdlib.h, string.h,
                 * stdio.h, etc). Emitting our own extern with different types would
                 * cause GCC conflicting type errors. */
                if (is_c_stdlib_function(fn_name))
                {
                    continue;
                }
                /* Skip if already emitted (deduplication) */
                if (is_extern_already_emitted(emitted, fn_name))
                {
                    continue;
                }
                track_emitted_extern(gen->arena, emitted, fn_name);
                if (*extern_count == 0)
                {
                    indented_fprintf(gen, 0, "/* Native function extern declarations */\n");
                }
                code_gen_native_extern_declaration(gen, fn);
                (*extern_count)++;
            }
        }
        else if (stmt->type == STMT_IMPORT && stmt->as.import.imported_stmts != NULL)
        {
            /* Recursively process imported modules - each import starts fresh for include check */
            code_gen_emit_imported_native_externs_recursive(gen, stmt->as.import.imported_stmts,
                                                            stmt->as.import.imported_count, extern_count, emitted, false);
        }
    }
}

/* Entry point for emitting native extern declarations */
static void code_gen_emit_imported_native_externs(CodeGen *gen, Stmt **statements, int count, int *extern_count)
{
    EmittedNativeExterns emitted = { NULL, 0, 0 };
    code_gen_emit_imported_native_externs_recursive(gen, statements, count, extern_count, &emitted, false);
}

/* Collect pragma directives from a list of statements (recursively handles imports) */
static void code_gen_collect_pragmas(CodeGen *gen, Stmt **statements, int count)
{
    for (int i = 0; i < count; i++)
    {
        Stmt *stmt = statements[i];
        if (stmt->type == STMT_PRAGMA)
        {
            if (stmt->as.pragma.pragma_type == PRAGMA_INCLUDE)
            {
                code_gen_add_pragma_include(gen, stmt->as.pragma.value);
            }
            else if (stmt->as.pragma.pragma_type == PRAGMA_LINK)
            {
                code_gen_add_pragma_link(gen, stmt->as.pragma.value);
            }
            else if (stmt->as.pragma.pragma_type == PRAGMA_SOURCE)
            {
                const char *source_dir = get_directory_from_path(gen->arena,
                    stmt->token ? stmt->token->filename : NULL);
                code_gen_add_pragma_source(gen, stmt->as.pragma.value, source_dir);
            }
        }
        else if (stmt->type == STMT_IMPORT &&
                 stmt->as.import.imported_stmts != NULL)
        {
            /* Recursively collect pragmas from all imported modules */
            code_gen_collect_pragmas(gen, stmt->as.import.imported_stmts,
                                     stmt->as.import.imported_count);
        }
    }
}

void code_gen_module(CodeGen *gen, Module *module)
{
    DEBUG_VERBOSE("Entering code_gen_module");

    // First collect pragma directives
    code_gen_collect_pragmas(gen, module->statements, module->count);

    code_gen_headers(gen);

    // Emit pragma includes after standard headers
    if (gen->pragma_include_count > 0)
    {
        indented_fprintf(gen, 0, "/* User-specified includes */\n");
        for (int i = 0; i < gen->pragma_include_count; i++)
        {
            indented_fprintf(gen, 0, "#include %s\n", gen->pragma_includes[i]);
        }
        indented_fprintf(gen, 0, "\n");
    }

    code_gen_externs(gen);

    // Emit opaque type forward struct declarations
    // Skip standard C library types that are already defined (e.g., FILE from stdio.h)
    int opaque_count = 0;
    for (int i = 0; i < module->count; i++)
    {
        Stmt *stmt = module->statements[i];
        if (stmt->type == STMT_TYPE_DECL)
        {
            TypeDeclStmt *type_decl = &stmt->as.type_decl;
            if (type_decl->type->kind == TYPE_OPAQUE && type_decl->type->as.opaque.name != NULL)
            {
                const char *name = type_decl->type->as.opaque.name;
                /* Skip standard C library types that are typically provided by headers */
                if (strcmp(name, "FILE") == 0 ||
                    strcmp(name, "DIR") == 0 ||
                    strcmp(name, "dirent") == 0)
                {
                    continue;
                }
                if (opaque_count == 0)
                {
                    indented_fprintf(gen, 0, "/* Opaque type forward declarations */\n");
                }
                indented_fprintf(gen, 0, "typedef struct %s %s;\n", name, name);
                opaque_count++;
            }
        }
    }
    if (opaque_count > 0)
    {
        indented_fprintf(gen, 0, "\n");
    }

    // Emit forward declarations for native structs with c_alias (including imports)
    // These are opaque handle types that alias external C types (e.g., SnDate -> RtDate)
    int native_alias_count = 0;
    EmittedNativeAliases emitted_aliases = { NULL, 0, 0 };
    code_gen_emit_imported_native_aliases(gen, module->statements, module->count,
                                          &native_alias_count, &emitted_aliases);
    if (native_alias_count > 0)
    {
        indented_fprintf(gen, 0, "\n");
    }

    // Emit struct type definitions (including imported structs)
    int struct_count = 0;
    code_gen_emit_imported_struct_typedefs(gen, module->statements, module->count, &struct_count);
    if (struct_count > 0)
    {
        indented_fprintf(gen, 0, "\n");
    }

    /* Emit struct method forward declarations */
    int method_count = 0;
    for (int i = 0; i < module->count; i++)
    {
        Stmt *stmt = module->statements[i];
        if (stmt->type == STMT_STRUCT_DECL)
        {
            StructDeclStmt *struct_decl = &stmt->as.struct_decl;
            char *raw_struct_name = arena_strndup(gen->arena, struct_decl->name.start, struct_decl->name.length);
            char *struct_name = sn_mangle_name(gen->arena, raw_struct_name);

            /* Create lowercase struct name for native method naming (uses raw name) */
            char *struct_name_lower = arena_strdup(gen->arena, raw_struct_name);
            for (char *p = struct_name_lower; *p; p++)
            {
                *p = (char)tolower((unsigned char)*p);
            }

            for (int j = 0; j < struct_decl->method_count; j++)
            {
                StructMethod *method = &struct_decl->methods[j];

                if (method_count == 0)
                {
                    indented_fprintf(gen, 0, "/* Struct method forward declarations */\n");
                }

                const char *ret_type;
                if (method->is_native && method->body == NULL &&
                    method->return_type != NULL && method->return_type->kind == TYPE_STRING) {
                    /* Native methods returning str use RtHandle (handle-based strings) */
                    ret_type = "RtHandle";
                } else if (method->is_native && method->body == NULL &&
                           method->return_type != NULL && method->return_type->kind == TYPE_ARRAY) {
                    /* Native methods returning arrays now return RtHandle directly */
                    ret_type = "RtHandle";
                } else {
                    ret_type = get_c_type(gen->arena, method->return_type);
                }

                if (method->is_native && method->body == NULL)
                {
                    /* Native method - generate extern declaration.
                     * If c_alias is set, use that as the function name.
                     * Otherwise, use the rt_{struct_lowercase}_{method_name} convention. */
                    const char *func_name;
                    if (method->c_alias != NULL)
                    {
                        func_name = method->c_alias;
                    }
                    else
                    {
                        func_name = arena_sprintf(gen->arena, "rt_%s_%s", struct_name_lower, method->name);
                    }

                    /* For opaque handle types (native struct with c_alias), use the C type directly */
                    const char *self_c_type;
                    if (struct_decl->is_native && struct_decl->c_alias != NULL)
                    {
                        /* Opaque handle: self type is already a pointer (e.g., RtDate *) */
                        self_c_type = arena_sprintf(gen->arena, "%s *", struct_decl->c_alias);
                    }
                    else if (struct_decl->pass_self_by_ref)
                    {
                        /* as ref: self is passed by pointer */
                        self_c_type = arena_sprintf(gen->arena, "%s *", struct_name);
                    }
                    else
                    {
                        self_c_type = struct_name;
                    }

                    if (method->is_static)
                    {
                        /* Static native: extern RetType func_name(params); */
                        indented_fprintf(gen, 0, "extern %s %s(",
                                         ret_type, func_name);
                        /* If method has implicit arena param, prepend RtArena* */
                        if (method->has_arena_param)
                        {
                            indented_fprintf(gen, 0, "RtArena *");
                            if (method->param_count > 0)
                            {
                                indented_fprintf(gen, 0, ", ");
                            }
                        }
                        if (method->param_count == 0 && !method->has_arena_param)
                        {
                            indented_fprintf(gen, 0, "void");
                        }
                        else
                        {
                            for (int k = 0; k < method->param_count; k++)
                            {
                                if (k > 0)
                                    indented_fprintf(gen, 0, ", ");
                                Parameter *param = &method->params[k];
                                const char *param_type = get_c_native_param_type(gen->arena, param->type);
                                indented_fprintf(gen, 0, "%s", param_type);
                            }
                        }
                        indented_fprintf(gen, 0, ");\n");
                    }
                    else
                    {
                        /* Instance native: extern RetType func_name(self_type self, params); */
                        /* If method has implicit arena param, prepend RtArena* */
                        if (method->has_arena_param)
                        {
                            indented_fprintf(gen, 0, "extern %s %s(RtArena *, %s",
                                             ret_type, func_name, self_c_type);
                        }
                        else
                        {
                            indented_fprintf(gen, 0, "extern %s %s(%s",
                                             ret_type, func_name, self_c_type);
                        }
                        for (int k = 0; k < method->param_count; k++)
                        {
                            Parameter *param = &method->params[k];
                            const char *param_type = get_c_native_param_type(gen->arena, param->type);
                            indented_fprintf(gen, 0, ", %s", param_type);
                        }
                        indented_fprintf(gen, 0, ");\n");
                    }
                }
                else
                {
                    /* Non-native method - regular forward declaration with arena */
                    if (method->is_static)
                    {
                        /* Static method: no self parameter */
                        if (method->param_count == 0)
                        {
                            indented_fprintf(gen, 0, "%s %s_%s(RtManagedArena *__caller_arena__);\n",
                                             ret_type, struct_name, method->name);
                        }
                        else
                        {
                            indented_fprintf(gen, 0, "%s %s_%s(RtManagedArena *__caller_arena__",
                                             ret_type, struct_name, method->name);
                            for (int k = 0; k < method->param_count; k++)
                            {
                                Parameter *param = &method->params[k];
                                const char *param_type = get_c_param_type(gen->arena, param->type);
                                char *param_name = sn_mangle_name(gen->arena, arena_strndup(gen->arena, param->name.start, param->name.length));
                                indented_fprintf(gen, 0, ", %s %s", param_type, param_name);
                            }
                            indented_fprintf(gen, 0, ");\n");
                        }
                    }
                    else
                    {
                        /* Instance method: first parameter is self (pointer to struct) */
                        /* For opaque handle types (native struct with c_alias), self is already a pointer */
                        if (struct_decl->is_native && struct_decl->c_alias != NULL)
                        {
                            /* Opaque handle: self type is the C alias pointer */
                            indented_fprintf(gen, 0, "%s %s_%s(RtManagedArena *__caller_arena__, %s *__sn__self",
                                             ret_type, struct_name, method->name, struct_decl->c_alias);
                        }
                        else
                        {
                            /* Regular struct: self is pointer to struct */
                            indented_fprintf(gen, 0, "%s %s_%s(RtManagedArena *__caller_arena__, %s *__sn__self",
                                             ret_type, struct_name, method->name, struct_name);
                        }
                        for (int k = 0; k < method->param_count; k++)
                        {
                            Parameter *param = &method->params[k];
                            const char *param_type = get_c_param_type(gen->arena, param->type);
                            char *param_name = sn_mangle_name(gen->arena, arena_strndup(gen->arena, param->name.start, param->name.length));
                            indented_fprintf(gen, 0, ", %s %s", param_type, param_name);
                        }
                        indented_fprintf(gen, 0, ");\n");
                    }
                }
                method_count++;
            }
        }
    }
    if (method_count > 0)
    {
        indented_fprintf(gen, 0, "\n");
    }

    // Emit type declarations (native callback typedefs) before forward declarations
    int typedef_count = 0;
    for (int i = 0; i < module->count; i++)
    {
        Stmt *stmt = module->statements[i];
        if (stmt->type == STMT_TYPE_DECL)
        {
            TypeDeclStmt *type_decl = &stmt->as.type_decl;
            if (type_decl->type->kind == TYPE_FUNCTION && type_decl->type->as.function.is_native)
            {
                if (typedef_count == 0)
                {
                    indented_fprintf(gen, 0, "/* Native callback type definitions */\n");
                }
                code_gen_native_callback_typedef(gen, type_decl);
                typedef_count++;
            }
        }
    }
    if (typedef_count > 0)
    {
        indented_fprintf(gen, 0, "\n");
    }

    // Emit extern declarations for native functions without body (including imported modules)
    int native_extern_count = 0;
    code_gen_emit_imported_native_externs(gen, module->statements, module->count, &native_extern_count);
    if (native_extern_count > 0)
    {
        indented_fprintf(gen, 0, "\n");
    }

    // Second pass: emit forward declarations for all user-defined functions
    indented_fprintf(gen, 0, "/* Forward declarations */\n");
    // Emit global arena reference (set by main at startup)
    indented_fprintf(gen, 0, "static RtManagedArena *__main_arena__ = NULL;\n\n");
    int forward_decl_count = 0;
    for (int i = 0; i < module->count; i++)
    {
        Stmt *stmt = module->statements[i];
        if (stmt->type == STMT_FUNCTION)
        {
            char *fn_name = get_var_name(gen->arena, stmt->as.function.name);
            if (strcmp(fn_name, "main") != 0)
            {
                code_gen_forward_declaration(gen, &stmt->as.function);
                forward_decl_count++;
            }
        }
    }
    if (forward_decl_count > 0)
    {
        indented_fprintf(gen, 0, "\n");
    }

    // Second pass: emit full function definitions to a temp file
    // This allows us to collect lambda forward declarations first
    FILE *original_output = gen->output;
    FILE *func_temp = tmpfile();
    if (func_temp == NULL)
    {
        fprintf(stderr, "Error: Failed to create temp file for function buffering\n");
        exit(1);
    }
    gen->output = func_temp;

    /* First, detect if there's a user-defined main function */
    bool has_main = false;
    for (int i = 0; i < module->count; i++)
    {
        Stmt *stmt = module->statements[i];
        if (stmt->type == STMT_FUNCTION &&
            stmt->as.function.name.length == 4 &&
            strncmp(stmt->as.function.name.start, "main", 4) == 0)
        {
            has_main = true;
            break;
        }
    }

    /* Emit declarations at global scope; defer executable statements to generated main */
    for (int i = 0; i < module->count; i++)
    {
        Stmt *stmt = module->statements[i];
        if (stmt->type == STMT_FUNCTION || stmt->type == STMT_STRUCT_DECL ||
            stmt->type == STMT_TYPE_DECL || stmt->type == STMT_IMPORT ||
            stmt->type == STMT_PRAGMA)
        {
            code_gen_statement(gen, stmt, 0);
        }
        else if (has_main && stmt->type == STMT_VAR_DECL)
        {
            /* When there's a user-defined main, global var decls go at file scope
             * so they're accessible by all functions */
            code_gen_statement(gen, stmt, 0);
        }
    }

    /* Emit struct method implementations */
    for (int i = 0; i < module->count; i++)
    {
        Stmt *stmt = module->statements[i];
        if (stmt->type == STMT_STRUCT_DECL)
        {
            StructDeclStmt *struct_decl = &stmt->as.struct_decl;
            char *struct_name = sn_mangle_name(gen->arena, arena_strndup(gen->arena, struct_decl->name.start, struct_decl->name.length));

            for (int j = 0; j < struct_decl->method_count; j++)
            {
                StructMethod *method = &struct_decl->methods[j];

                /* Skip native methods with no body - they are extern declared elsewhere */
                if (method->is_native && method->body == NULL)
                {
                    continue;
                }

                const char *ret_type = get_c_type(gen->arena, method->return_type);

                /* Generate function signature */
                if (method->is_static)
                {
                    if (method->param_count == 0)
                    {
                        indented_fprintf(gen, 0, "%s %s_%s(RtManagedArena *__caller_arena__) {\n",
                                         ret_type, struct_name, method->name);
                    }
                    else
                    {
                        indented_fprintf(gen, 0, "%s %s_%s(RtManagedArena *__caller_arena__",
                                         ret_type, struct_name, method->name);
                        for (int k = 0; k < method->param_count; k++)
                        {
                            Parameter *param = &method->params[k];
                            const char *param_type = get_c_param_type(gen->arena, param->type);
                            char *param_name = sn_mangle_name(gen->arena, arena_strndup(gen->arena, param->name.start, param->name.length));
                            indented_fprintf(gen, 0, ", %s %s", param_type, param_name);
                        }
                        indented_fprintf(gen, 0, ") {\n");
                    }
                }
                else
                {
                    /* Instance method: first parameter is self (pointer to struct) */
                    /* For opaque handle types (native struct with c_alias), self is already a pointer */
                    if (struct_decl->is_native && struct_decl->c_alias != NULL)
                    {
                        /* Opaque handle: self type is the C alias pointer */
                        indented_fprintf(gen, 0, "%s %s_%s(RtManagedArena *__caller_arena__, %s *__sn__self",
                                         ret_type, struct_name, method->name, struct_decl->c_alias);
                    }
                    else
                    {
                        /* Regular struct: self is pointer to struct */
                        indented_fprintf(gen, 0, "%s %s_%s(RtManagedArena *__caller_arena__, %s *__sn__self",
                                         ret_type, struct_name, method->name, struct_name);
                    }
                    for (int k = 0; k < method->param_count; k++)
                    {
                        Parameter *param = &method->params[k];
                        const char *param_type = get_c_param_type(gen->arena, param->type);
                        char *param_name = sn_mangle_name(gen->arena, arena_strndup(gen->arena, param->name.start, param->name.length));
                        indented_fprintf(gen, 0, ", %s %s", param_type, param_name);
                    }
                    indented_fprintf(gen, 0, ") {\n");
                }

                /* Set up code generator state for method */
                char *method_full_name = arena_sprintf(gen->arena, "%s_%s", struct_name, method->name);
                char *saved_function = gen->current_function;
                Type *saved_return_type = gen->current_return_type;
                char *saved_arena_var = gen->current_arena_var;
                char *saved_function_arena = gen->function_arena_var;

                gen->current_function = method_full_name;
                gen->current_return_type = method->return_type;
                gen->current_arena_var = "__caller_arena__";
                gen->function_arena_var = "__caller_arena__";

                /* Push scope and add method params to symbol table for proper pinning */
                symbol_table_push_scope(gen->symbol_table);
                symbol_table_enter_arena(gen->symbol_table);
                for (int k = 0; k < method->param_count; k++)
                {
                    symbol_table_add_symbol_full(gen->symbol_table, method->params[k].name,
                                                 method->params[k].type, SYMBOL_PARAM,
                                                 method->params[k].mem_qualifier);
                }

                /* Determine if we need a _return_value variable */
                bool has_return_value = (method->return_type && method->return_type->kind != TYPE_VOID);

                /* Add _return_value if needed */
                if (has_return_value)
                {
                    const char *default_val = get_default_value(method->return_type);
                    indented_fprintf(gen, 1, "%s _return_value = %s;\n", ret_type, default_val);
                }

                /* Generate method body */
                for (int k = 0; k < method->body_count; k++)
                {
                    if (method->body[k] != NULL)
                    {
                        code_gen_statement(gen, method->body[k], 1);
                    }
                }

                /* Add return label and return statement */
                indented_fprintf(gen, 0, "%s_return:\n", method_full_name);
                if (has_return_value)
                {
                    indented_fprintf(gen, 1, "return _return_value;\n");
                }
                else
                {
                    indented_fprintf(gen, 1, "return;\n");
                }

                /* Pop method scope */
                symbol_table_pop_scope(gen->symbol_table);

                /* Restore code generator state */
                gen->current_function = saved_function;
                gen->current_return_type = saved_return_type;
                gen->current_arena_var = saved_arena_var;
                gen->function_arena_var = saved_function_arena;

                /* Close function */
                indented_fprintf(gen, 0, "}\n\n");
            }
        }
    }

    if (!has_main)
    {
        // Generate main with managed arena lifecycle
        indented_fprintf(gen, 0, "int main() {\n");
        indented_fprintf(gen, 1, "RtManagedArena *__local_arena__ = rt_managed_arena_create();\n");
        indented_fprintf(gen, 1, "__main_arena__ = __local_arena__;\n");
        // Emit deferred global initializations (handle-type globals that couldn't
        // be initialized at file scope because C doesn't allow non-constant initializers)
        for (int i = 0; i < gen->deferred_global_count; i++)
        {
            indented_fprintf(gen, 1, "%s = %s;\n",
                             gen->deferred_global_names[i],
                             gen->deferred_global_values[i]);
        }
        indented_fprintf(gen, 1, "int _return_value = 0;\n");

        // Set up arena context for top-level statements
        gen->current_arena_var = "__local_arena__";
        gen->current_function = "main";
        gen->expr_as_handle = true;

        // Emit all non-declaration top-level statements inside main
        for (int i = 0; i < module->count; i++)
        {
            Stmt *stmt = module->statements[i];
            if (stmt->type != STMT_FUNCTION && stmt->type != STMT_STRUCT_DECL &&
                stmt->type != STMT_TYPE_DECL && stmt->type != STMT_IMPORT &&
                stmt->type != STMT_PRAGMA)
            {
                code_gen_statement(gen, stmt, 1);
            }
        }

        gen->current_arena_var = NULL;
        gen->current_function = NULL;
        gen->expr_as_handle = false;

        indented_fprintf(gen, 1, "goto main_return;\n");
        indented_fprintf(gen, 0, "main_return:\n");
        indented_fprintf(gen, 1, "rt_managed_arena_destroy(__local_arena__);\n");
        indented_fprintf(gen, 1, "return _return_value;\n");
        indented_fprintf(gen, 0, "}\n");
    }

    // Restore original output
    gen->output = original_output;

    /* Output accumulated lambda forward declarations BEFORE function definitions */
    if (gen->lambda_forward_decls && strlen(gen->lambda_forward_decls) > 0)
    {
        indented_fprintf(gen, 0, "/* Lambda forward declarations */\n");
        fprintf(gen->output, "%s", gen->lambda_forward_decls);
        indented_fprintf(gen, 0, "\n");
    }

    /* Output accumulated interceptor thunk forward declarations */
    if (gen->thunk_forward_decls && strlen(gen->thunk_forward_decls) > 0)
    {
        indented_fprintf(gen, 0, "/* Interceptor thunk forward declarations */\n");
        fprintf(gen->output, "%s", gen->thunk_forward_decls);
        indented_fprintf(gen, 0, "\n");
    }

    /* Now copy the buffered function definitions */
    fseek(func_temp, 0, SEEK_END);
    long func_size = ftell(func_temp);
    if (func_size > 0)
    {
        fseek(func_temp, 0, SEEK_SET);
        char *func_buf = arena_alloc(gen->arena, func_size + 1);
        size_t read_size = fread(func_buf, 1, func_size, func_temp);
        func_buf[read_size] = '\0';
        fprintf(gen->output, "%s", func_buf);
    }
    fclose(func_temp);

    /* Output accumulated lambda function definitions at the end */
    if (gen->lambda_definitions && strlen(gen->lambda_definitions) > 0)
    {
        indented_fprintf(gen, 0, "\n/* Lambda function definitions */\n");
        fprintf(gen->output, "%s", gen->lambda_definitions);
    }

    /* Output accumulated interceptor thunk definitions */
    if (gen->thunk_definitions && strlen(gen->thunk_definitions) > 0)
    {
        indented_fprintf(gen, 0, "\n/* Interceptor thunk definitions */\n");
        fprintf(gen->output, "%s", gen->thunk_definitions);
    }
}
