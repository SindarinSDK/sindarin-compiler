/**
 * code_gen_native_extern.c - Native extern declaration emission
 *
 * Contains functions for emitting extern declarations for native
 * functions without bodies during code generation.
 */

#include "code_gen/code_gen_native_extern.h"
#include "code_gen/code_gen_util.h"
#include "arena.h"
#include <string.h>

bool is_c_stdlib_function(const char *name)
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

void code_gen_native_extern_declaration(CodeGen *gen, FunctionStmt *fn)
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

void code_gen_emit_imported_native_externs(CodeGen *gen, Stmt **statements, int count, int *extern_count)
{
    EmittedNativeExterns emitted = { NULL, 0, 0 };
    code_gen_emit_imported_native_externs_recursive(gen, statements, count, extern_count, &emitted, false);
}
