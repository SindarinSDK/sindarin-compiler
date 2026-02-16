#include "code_gen.h"
#include "code_gen/util/code_gen_util.h"
#include "code_gen/expr/code_gen_expr.h"
#include "code_gen/stmt/code_gen_stmt.h"
#include "code_gen/emit/code_gen_pragma.h"
#include "code_gen/emit/code_gen_struct_emit.h"
#include "code_gen/emit/code_gen_native_extern.h"
#include "code_gen/emit/code_gen_method_emit.h"
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

    /* Initialize lock stack for tracking active locks */
    gen->lock_stack = NULL;
    gen->lock_stack_depth = 0;
    gen->lock_stack_capacity = 0;

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

    /* Initialize GC callback generation buffers */
    gen->callback_forward_decls = arena_strdup(arena, "");
    gen->callback_definitions = arena_strdup(arena, "");
    gen->emitted_callbacks = NULL;
    gen->emitted_callback_count = 0;
    gen->emitted_callback_capacity = 0;

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
    /* pthread.h is included via runtime.h -> runtime_thread_v3.h (handles Windows compatibility) */
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
     * RtHandleV2, RtArenaV2, RtArrayMetadataV2,
     * and all arena / runtime function declarations.
     * Only need __Closure__ typedef and non-header function declarations below. */
    indented_fprintf(gen, 0, "\n");

    /* Generic closure type for lambdas */
    indented_fprintf(gen, 0, "/* Closure type for lambdas */\n");
    indented_fprintf(gen, 0, "typedef struct __Closure__ { void *fn; RtArenaV2 *arena; size_t size; } __Closure__;\n\n");
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
    fprintf(gen->output, "RtArenaV2 *");
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
    int method_count = code_gen_emit_struct_method_forwards(gen, module->statements, module->count);
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
    indented_fprintf(gen, 0, "static RtArenaV2 *__main_arena__ = NULL;\n\n");
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
    code_gen_emit_struct_method_implementations(gen, module->statements, module->count);

    if (!has_main)
    {
        // Generate main with arena V2 lifecycle
        indented_fprintf(gen, 0, "int main() {\n");
        indented_fprintf(gen, 1, "RtArenaV2 *__local_arena__ = rt_arena_v2_create(NULL, RT_ARENA_MODE_DEFAULT, \"main\");\n");
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
        indented_fprintf(gen, 1, "rt_arena_v2_condemn(__local_arena__);\n");
        indented_fprintf(gen, 1, "return _return_value;\n");
        indented_fprintf(gen, 0, "}\n");
    }

    // Restore original output
    gen->output = original_output;

    /* Output GC callback forward declarations BEFORE lambda forward declarations */
    if (gen->callback_forward_decls && strlen(gen->callback_forward_decls) > 0)
    {
        indented_fprintf(gen, 0, "/* GC callback forward declarations */\n");
        fprintf(gen->output, "%s", gen->callback_forward_decls);
        indented_fprintf(gen, 0, "\n");
    }

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

    /* Output GC callback definitions BEFORE lambda definitions */
    if (gen->callback_definitions && strlen(gen->callback_definitions) > 0)
    {
        indented_fprintf(gen, 0, "\n/* GC callback definitions */\n");
        fprintf(gen->output, "%s", gen->callback_definitions);
    }

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
