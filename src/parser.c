#include "parser.h"
#include "parser/parser_util.h"
#include "parser/parser_expr.h"
#include "parser/parser_stmt.h"
#include "debug.h"
#include "file.h"
#include "gcc_backend.h"
#include <stdio.h>
#include <string.h>

void parser_init(Arena *arena, Parser *parser, Lexer *lexer, SymbolTable *symbol_table)
{
    parser->arena = arena;
    parser->lexer = lexer;
    parser->had_error = 0;
    parser->panic_mode = 0;
    parser->symbol_table = symbol_table;
    parser->sized_array_pending = 0;
    parser->sized_array_size = NULL;
    parser->in_native_function = 0;
    parser->pack_alignment = 0;  /* 0 = default alignment, 1 = packed */
    parser->pending_alias = NULL;  /* No pending alias initially */

    Token print_token;
    print_token.start = arena_strdup(arena, "print");
    print_token.length = 5;
    print_token.type = TOKEN_IDENTIFIER;
    print_token.line = 0;
    print_token.filename = arena_strdup(arena, "<built-in>");

    Type *any_type = ast_create_primitive_type(arena, TYPE_ANY);
    Type **builtin_params = arena_alloc(arena, sizeof(Type *));
    builtin_params[0] = any_type;

    Type *print_type = ast_create_function_type(arena, ast_create_primitive_type(arena, TYPE_VOID), builtin_params, 1);
    symbol_table_add_symbol_with_kind(parser->symbol_table, print_token, print_type, SYMBOL_GLOBAL);

    Token to_string_token;
    to_string_token.start = arena_strdup(arena, "to_string");
    to_string_token.length = 9;
    to_string_token.type = TOKEN_IDENTIFIER;
    to_string_token.line = 0;
    to_string_token.filename = arena_strdup(arena, "<built-in>");

    Type *to_string_type = ast_create_function_type(arena, ast_create_primitive_type(arena, TYPE_STRING), builtin_params, 1);
    symbol_table_add_symbol_with_kind(parser->symbol_table, to_string_token, to_string_type, SYMBOL_GLOBAL);

    // Array built-in: len(arr) -> int (works on arrays and strings)
    Token len_token;
    len_token.start = arena_strdup(arena, "len");
    len_token.length = 3;
    len_token.type = TOKEN_IDENTIFIER;
    len_token.line = 0;
    len_token.filename = arena_strdup(arena, "<built-in>");
    Type *len_type = ast_create_function_type(arena, ast_create_primitive_type(arena, TYPE_INT), builtin_params, 1);
    symbol_table_add_symbol_with_kind(parser->symbol_table, len_token, len_type, SYMBOL_GLOBAL);

    // Console I/O convenience functions
    // readLine() -> str
    Token readLine_token;
    readLine_token.start = arena_strdup(arena, "readLine");
    readLine_token.length = 8;
    readLine_token.type = TOKEN_IDENTIFIER;
    readLine_token.line = 0;
    readLine_token.filename = arena_strdup(arena, "<built-in>");
    Type *readLine_type = ast_create_function_type(arena, ast_create_primitive_type(arena, TYPE_STRING), NULL, 0);
    symbol_table_add_symbol_with_kind(parser->symbol_table, readLine_token, readLine_type, SYMBOL_GLOBAL);

    // println(any) -> void
    Token println_token;
    println_token.start = arena_strdup(arena, "println");
    println_token.length = 7;
    println_token.type = TOKEN_IDENTIFIER;
    println_token.line = 0;
    println_token.filename = arena_strdup(arena, "<built-in>");
    Type *println_type = ast_create_function_type(arena, ast_create_primitive_type(arena, TYPE_VOID), builtin_params, 1);
    symbol_table_add_symbol_with_kind(parser->symbol_table, println_token, println_type, SYMBOL_GLOBAL);

    // printErr(any) -> void
    Token printErr_token;
    printErr_token.start = arena_strdup(arena, "printErr");
    printErr_token.length = 8;
    printErr_token.type = TOKEN_IDENTIFIER;
    printErr_token.line = 0;
    printErr_token.filename = arena_strdup(arena, "<built-in>");
    Type *printErr_type = ast_create_function_type(arena, ast_create_primitive_type(arena, TYPE_VOID), builtin_params, 1);
    symbol_table_add_symbol_with_kind(parser->symbol_table, printErr_token, printErr_type, SYMBOL_GLOBAL);

    // printErrLn(any) -> void
    Token printErrLn_token;
    printErrLn_token.start = arena_strdup(arena, "printErrLn");
    printErrLn_token.length = 10;
    printErrLn_token.type = TOKEN_IDENTIFIER;
    printErrLn_token.line = 0;
    printErrLn_token.filename = arena_strdup(arena, "<built-in>");
    Type *printErrLn_type = ast_create_function_type(arena, ast_create_primitive_type(arena, TYPE_VOID), builtin_params, 1);
    symbol_table_add_symbol_with_kind(parser->symbol_table, printErrLn_token, printErrLn_type, SYMBOL_GLOBAL);

    // exit(code: int) -> void
    Token exit_token;
    exit_token.start = arena_strdup(arena, "exit");
    exit_token.length = 4;
    exit_token.type = TOKEN_IDENTIFIER;
    exit_token.line = 0;
    exit_token.filename = arena_strdup(arena, "<built-in>");
    Type **exit_params = arena_alloc(arena, sizeof(Type *));
    exit_params[0] = ast_create_primitive_type(arena, TYPE_INT);
    Type *exit_type = ast_create_function_type(arena, ast_create_primitive_type(arena, TYPE_VOID), exit_params, 1);
    symbol_table_add_symbol_with_kind(parser->symbol_table, exit_token, exit_type, SYMBOL_GLOBAL);

    // assert(condition: bool, message: str) -> void
    Token assert_token;
    assert_token.start = arena_strdup(arena, "assert");
    assert_token.length = 6;
    assert_token.type = TOKEN_IDENTIFIER;
    assert_token.line = 0;
    assert_token.filename = arena_strdup(arena, "<built-in>");
    Type **assert_params = arena_alloc(arena, sizeof(Type *) * 2);
    assert_params[0] = ast_create_primitive_type(arena, TYPE_BOOL);
    assert_params[1] = ast_create_primitive_type(arena, TYPE_STRING);
    Type *assert_type = ast_create_function_type(arena, ast_create_primitive_type(arena, TYPE_VOID), assert_params, 2);
    symbol_table_add_symbol_with_kind(parser->symbol_table, assert_token, assert_type, SYMBOL_GLOBAL);

    // Note: Other array operations (push, pop, rev, rem, ins) are now method-style only:
    //   arr.push(elem), arr.pop(), arr.reverse(), arr.remove(idx), arr.insert(elem, idx)

    parser->previous.type = TOKEN_ERROR;
    parser->previous.start = NULL;
    parser->previous.length = 0;
    parser->previous.line = 0;
    parser->current = parser->previous;

    parser_advance(parser);
    parser->interp_sources = NULL;
    parser->interp_count = 0;
    parser->interp_capacity = 0;
    parser->import_ctx = NULL;
}

void parser_cleanup(Parser *parser)
{
    parser->interp_sources = NULL;
    parser->interp_count = 0;
    parser->interp_capacity = 0;
    parser->previous.start = NULL;
    parser->current.start = NULL;
}

Module *parser_execute(Parser *parser, const char *filename)
{
    Module *module = arena_alloc(parser->arena, sizeof(Module));
    ast_init_module(parser->arena, module, filename);

    while (!parser_is_at_end(parser))
    {
        while (parser_match(parser, TOKEN_NEWLINE))
        {
        }
        if (parser_is_at_end(parser))
            break;

        Stmt *stmt = parser_declaration(parser);
        if (stmt != NULL)
        {
            ast_module_add_statement(parser->arena, module, stmt);
            ast_print_stmt(parser->arena, stmt, 0);
        }

        if (parser->panic_mode)
        {
            synchronize(parser);
        }
    }

    if (parser->had_error)
    {
        return NULL;
    }

    return module;
}

/* Forward declaration for recursive import processing */
static Module *process_import_callback(Arena *arena, SymbolTable *symbol_table, const char *import_path,
                                       ImportContext *ctx);

/* Helper function to construct the import path from current file and module name */
static char *construct_import_path(Arena *arena, const char *current_file, const char *module_name)
{
    /* Find the last path separator (handle both Unix '/' and Windows '\') */
    const char *dir_end_fwd = strrchr(current_file, '/');
    const char *dir_end_back = strrchr(current_file, '\\');
    const char *dir_end = dir_end_fwd;
    if (dir_end_back && (!dir_end || dir_end_back > dir_end)) {
        dir_end = dir_end_back;
    }
    size_t dir_len = dir_end ? (size_t)(dir_end - current_file + 1) : 0;

    size_t mod_name_len = strlen(module_name);
    size_t path_len = dir_len + mod_name_len + 4; /* +4 for ".sn\0" */
    char *import_path = arena_alloc(arena, path_len);
    if (!import_path) {
        return NULL;
    }

    if (dir_len > 0) {
        strncpy(import_path, current_file, dir_len);
        import_path[dir_len] = '\0';
    } else {
        import_path[0] = '\0';
    }
    strcat(import_path, module_name);
    strcat(import_path, ".sn");

    return import_path;
}

/* Helper: check if file exists */
static bool import_file_exists(const char *path)
{
    FILE *f = fopen(path, "r");
    if (f) {
        fclose(f);
        return true;
    }
    return false;
}

/* Process an import immediately during parsing - registers types and functions */
Module *parser_process_import(Parser *parser, const char *module_name, bool is_namespaced)
{
    if (!parser->import_ctx) {
        /* No import context - imports will be processed later by parse_module_with_imports */
        return NULL;
    }

    ImportContext *ctx = parser->import_ctx;

    /* Construct the full import path (relative to current file) */
    char *import_path = construct_import_path(parser->arena, ctx->current_file, module_name);
    if (!import_path) {
        parser_error(parser, "Failed to allocate memory for import path");
        return NULL;
    }

    /* If relative path doesn't exist, try SDK path */
    if (!import_file_exists(import_path) && ctx->compiler_dir) {
        const char *sdk_path = gcc_resolve_sdk_import(ctx->compiler_dir, module_name);
        if (sdk_path) {
            /* Use SDK path instead - need to copy to arena since sdk_path is static */
            import_path = arena_strdup(parser->arena, sdk_path);
            if (!import_path) {
                parser_error(parser, "Failed to allocate memory for SDK import path");
                return NULL;
            }
        }
    }

    /* Check if already imported */
    for (int j = 0; j < *ctx->imported_count; j++) {
        if (strcmp(ctx->imported[j], import_path) == 0) {
            /* Already imported - return the cached module */
            return ctx->imported_modules[j];
        }
    }

    /* Ensure capacity in imported arrays */
    if (*ctx->imported_count >= *ctx->imported_capacity) {
        int new_capacity = *ctx->imported_capacity == 0 ? 8 : *ctx->imported_capacity * 2;
        char **new_imported = arena_alloc(parser->arena, sizeof(char *) * new_capacity);
        Module **new_modules = arena_alloc(parser->arena, sizeof(Module *) * new_capacity);
        bool *new_directly = arena_alloc(parser->arena, sizeof(bool) * new_capacity);
        if (!new_imported || !new_modules || !new_directly) {
            parser_error(parser, "Failed to allocate memory for imported list");
            return NULL;
        }
        if (*ctx->imported_count > 0) {
            memmove(new_imported, ctx->imported, sizeof(char *) * *ctx->imported_count);
            memmove(new_modules, ctx->imported_modules, sizeof(Module *) * *ctx->imported_count);
            memmove(new_directly, ctx->imported_directly, sizeof(bool) * *ctx->imported_count);
        }
        ctx->imported = new_imported;
        ctx->imported_modules = new_modules;
        ctx->imported_directly = new_directly;
        *ctx->imported_capacity = new_capacity;
    }

    /* Reserve slot before recursive call to prevent infinite recursion on circular imports */
    int module_idx = *ctx->imported_count;
    ctx->imported[module_idx] = import_path;
    ctx->imported_modules[module_idx] = NULL;
    ctx->imported_directly[module_idx] = !is_namespaced;
    (*ctx->imported_count)++;

    /* Process the import via the callback */
    Module *imported_module = ctx->process_import(parser->arena, parser->symbol_table, import_path, ctx);
    if (!imported_module) {
        /* Import failed - error already reported */
        return NULL;
    }

    /* Store the parsed module in the cache */
    ctx->imported_modules[module_idx] = imported_module;

    return imported_module;
}

/* Callback function for recursive import processing */
static Module *process_import_callback(Arena *arena, SymbolTable *symbol_table, const char *import_path,
                                       ImportContext *parent_ctx)
{
    char *source = file_read(arena, import_path);
    if (!source) {
        DEBUG_ERROR("Failed to read file: %s", import_path);
        return NULL;
    }

    Lexer lexer;
    lexer_init(arena, &lexer, source, import_path);

    Parser parser;
    parser_init(arena, &parser, &lexer, symbol_table);

    /* Set up import context for the imported module using parent's arrays */
    ImportContext import_ctx;
    import_ctx.imported = parent_ctx->imported;
    import_ctx.imported_count = parent_ctx->imported_count;
    import_ctx.imported_capacity = parent_ctx->imported_capacity;
    import_ctx.imported_modules = parent_ctx->imported_modules;
    import_ctx.imported_directly = parent_ctx->imported_directly;
    import_ctx.current_file = import_path;
    import_ctx.compiler_dir = parent_ctx->compiler_dir;
    import_ctx.process_import = process_import_callback;

    parser.import_ctx = &import_ctx;

    Module *module = parser_execute(&parser, import_path);
    if (!module || parser.had_error) {
        parser_cleanup(&parser);
        lexer_cleanup(&lexer);
        return NULL;
    }

    parser_cleanup(&parser);
    lexer_cleanup(&lexer);

    /* Merge transitive imports into this module's statements.
     * This is necessary so that when this module is imported by another file,
     * all struct definitions (including those from transitive imports) are included.
     * Without this, a chain like A -> B -> C would not include C's structs in A. */
    Stmt **all_statements = NULL;
    int all_count = 0;
    int all_capacity = 0;

    for (int i = 0; i < module->count; i++) {
        Stmt *stmt = module->statements[i];
        if (stmt != NULL && stmt->type == STMT_IMPORT &&
            stmt->as.import.imported_stmts != NULL &&
            stmt->as.import.namespace == NULL) {
            /* This import was processed during parsing - merge its statements */
            int import_count = stmt->as.import.imported_count;
            int new_all_count = all_count + import_count;
            int old_capacity = all_capacity;
            while (new_all_count > all_capacity) {
                all_capacity = all_capacity == 0 ? 8 : all_capacity * 2;
            }
            if (all_capacity > old_capacity || all_statements == NULL) {
                Stmt **new_statements = arena_alloc(arena, sizeof(Stmt *) * all_capacity);
                if (!new_statements) {
                    DEBUG_ERROR("Failed to allocate memory for statements");
                    return NULL;
                }
                if (all_count > 0 && all_statements != NULL) {
                    memmove(new_statements, all_statements, sizeof(Stmt *) * all_count);
                }
                all_statements = new_statements;
            }
            memmove(all_statements + all_count, stmt->as.import.imported_stmts, sizeof(Stmt *) * import_count);
            all_count = new_all_count;

            /* Remove the STMT_IMPORT since we merged its contents */
            memmove(&module->statements[i], &module->statements[i + 1], sizeof(Stmt *) * (module->count - i - 1));
            module->count--;
            i--; /* Re-check this index since we shifted statements */
        }
    }

    /* If we merged any imports, combine with the module's own statements */
    if (all_count > 0) {
        int new_all_count = all_count + module->count;
        int old_capacity = all_capacity;
        while (new_all_count > all_capacity) {
            all_capacity = all_capacity == 0 ? 8 : all_capacity * 2;
        }
        if (all_capacity > old_capacity) {
            Stmt **new_statements = arena_alloc(arena, sizeof(Stmt *) * all_capacity);
            if (!new_statements) {
                DEBUG_ERROR("Failed to allocate memory for statements");
                return NULL;
            }
            if (all_count > 0 && all_statements != NULL) {
                memmove(new_statements, all_statements, sizeof(Stmt *) * all_count);
            }
            all_statements = new_statements;
        }
        memmove(all_statements + all_count, module->statements, sizeof(Stmt *) * module->count);
        all_count = new_all_count;

        module->statements = all_statements;
        module->count = all_count;
        module->capacity = all_count;
    }

    return module;
}

Module *parse_module_with_imports(Arena *arena, SymbolTable *symbol_table, const char *filename,
                                  char ***imported, int *imported_count, int *imported_capacity,
                                  Module ***imported_modules, bool **imported_directly,
                                  const char *compiler_dir)
{
    char *source = file_read(arena, filename);
    if (!source)
    {
        DEBUG_ERROR("Failed to read file: %s", filename);
        return NULL;
    }

    Lexer lexer;
    lexer_init(arena, &lexer, source, filename);

    Parser parser;
    parser_init(arena, &parser, &lexer, symbol_table);

    /* Set up import context for import-first processing.
     * This allows types from imported modules to be registered
     * DURING parsing, before they are referenced. */
    ImportContext import_ctx;
    import_ctx.imported = *imported;
    import_ctx.imported_count = imported_count;
    import_ctx.imported_capacity = imported_capacity;
    import_ctx.imported_modules = *imported_modules;
    import_ctx.imported_directly = *imported_directly;
    import_ctx.current_file = filename;
    import_ctx.compiler_dir = compiler_dir;
    import_ctx.process_import = process_import_callback;

    parser.import_ctx = &import_ctx;

    Module *module = parser_execute(&parser, filename);

    /* Update the caller's pointers since arrays may have been reallocated */
    *imported = import_ctx.imported;
    *imported_modules = import_ctx.imported_modules;
    *imported_directly = import_ctx.imported_directly;

    if (!module || parser.had_error)
    {
        parser_cleanup(&parser);
        lexer_cleanup(&lexer);
        return NULL;
    }

    Stmt **all_statements = NULL;
    int all_count = 0;
    int all_capacity = 0;

    /* Find the last path separator (handle both Unix '/' and Windows '\') */
    const char *dir_end_fwd = strrchr(filename, '/');
    const char *dir_end_back = strrchr(filename, '\\');
    const char *dir_end = dir_end_fwd;
    if (dir_end_back && (!dir_end || dir_end_back > dir_end)) {
        dir_end = dir_end_back;
    }
    size_t dir_len = dir_end ? (size_t)(dir_end - filename + 1) : 0;
    char *dir = NULL;
    if (dir_len > 0) {
        dir = arena_alloc(arena, dir_len + 1);
        if (!dir) {
            DEBUG_ERROR("Failed to allocate memory for directory path");
            parser_cleanup(&parser);
            lexer_cleanup(&lexer);
            return NULL;
        }
        strncpy(dir, filename, dir_len);
        dir[dir_len] = '\0';
    }

    for (int i = 0; i < module->count;)
    {
        Stmt *stmt = module->statements[i];
        if (stmt->type == STMT_IMPORT)
        {
            Token mod_name = stmt->as.import.module_name;
            size_t mod_name_len = strlen(mod_name.start);
            size_t path_len = dir_len + mod_name_len + 4;
            char *import_path = arena_alloc(arena, path_len);
            if (!import_path)
            {
                DEBUG_ERROR("Failed to allocate memory for import path");
                parser_cleanup(&parser);
                lexer_cleanup(&lexer);
                return NULL;
            }
            if (dir_len > 0) {
                strcpy(import_path, dir);
            } else {
                import_path[0] = '\0';
            }
            strcat(import_path, mod_name.start);
            strcat(import_path, ".sn");

            /* If relative path doesn't exist, try SDK path */
            if (!import_file_exists(import_path) && compiler_dir) {
                const char *sdk_path = gcc_resolve_sdk_import(compiler_dir, mod_name.start);
                if (sdk_path) {
                    /* Use SDK path instead */
                    import_path = arena_strdup(arena, sdk_path);
                    if (!import_path) {
                        DEBUG_ERROR("Failed to allocate memory for SDK import path");
                        parser_cleanup(&parser);
                        lexer_cleanup(&lexer);
                        return NULL;
                    }
                }
            }

            int already_imported_idx = -1;
            for (int j = 0; j < *imported_count; j++)
            {
                if (strcmp((*imported)[j], import_path) == 0)
                {
                    already_imported_idx = j;
                    break;
                }
            }

            if (already_imported_idx >= 0)
            {
                /* Module already imported. Handle different cases:
                 * 1. Import-first processing: statements already in stmt->as.import.imported_stmts
                 * 2. Namespaced import after any import: create namespace, mark if also direct
                 * 3. Direct import after namespaced: merge functions into main module
                 * 4. Direct import after direct: skip entirely (true duplicate) */

                bool was_imported_directly = (*imported_directly != NULL) ?
                    (*imported_directly)[already_imported_idx] : false;

                /* Check if this import was processed during parsing (import-first processing).
                 * If so, imported_stmts will be populated and we need to merge them. */
                if (stmt->as.import.imported_stmts != NULL && stmt->as.import.namespace == NULL)
                {
                    /* Import was processed during parsing - merge the statements */
                    int import_count = stmt->as.import.imported_count;
                    int new_all_count = all_count + import_count;
                    int old_capacity = all_capacity;
                    while (new_all_count > all_capacity)
                    {
                        all_capacity = all_capacity == 0 ? 8 : all_capacity * 2;
                    }
                    if (all_capacity > old_capacity || all_statements == NULL)
                    {
                        Stmt **new_statements = arena_alloc(arena, sizeof(Stmt *) * all_capacity);
                        if (!new_statements)
                        {
                            DEBUG_ERROR("Failed to allocate memory for statements");
                            parser_cleanup(&parser);
                            lexer_cleanup(&lexer);
                            return NULL;
                        }
                        if (all_count > 0 && all_statements != NULL)
                        {
                            memmove(new_statements, all_statements, sizeof(Stmt *) * all_count);
                        }
                        all_statements = new_statements;
                    }
                    memmove(all_statements + all_count, stmt->as.import.imported_stmts, sizeof(Stmt *) * import_count);
                    all_count = new_all_count;

                    /* Remove this import statement since we merged */
                    memmove(&module->statements[i], &module->statements[i + 1], sizeof(Stmt *) * (module->count - i - 1));
                    module->count--;
                    continue;
                }

                if (stmt->as.import.namespace != NULL && *imported_modules != NULL)
                {
                    /* Namespaced import: keep STMT_IMPORT for type checker to create namespace.
                     * Mark if module was also imported directly so code gen skips emitting. */
                    Module *cached_module = (*imported_modules)[already_imported_idx];
                    if (cached_module != NULL)
                    {
                        stmt->as.import.imported_stmts = cached_module->statements;
                        stmt->as.import.imported_count = cached_module->count;
                        stmt->as.import.also_imported_directly = was_imported_directly;
                        i++; /* Keep the STMT_IMPORT and move to next statement */
                        continue;
                    }
                }
                else if (!was_imported_directly && *imported_modules != NULL)
                {
                    /* Direct import after only namespaced imports: need to merge functions
                     * into main module and update the imported_directly flag. */
                    Module *cached_module = (*imported_modules)[already_imported_idx];
                    if (cached_module != NULL)
                    {
                        /* Mark that this module is now also imported directly */
                        if (*imported_directly != NULL)
                        {
                            (*imported_directly)[already_imported_idx] = true;
                        }

                        /* Find and update any existing namespace import for this module.
                         * This is necessary because the namespace STMT_IMPORT was already processed
                         * and code gen would otherwise emit the functions twice. */
                        for (int k = 0; k < i; k++)
                        {
                            Stmt *prev_stmt = module->statements[k];
                            if (prev_stmt != NULL && prev_stmt->type == STMT_IMPORT &&
                                prev_stmt->as.import.namespace != NULL &&
                                prev_stmt->as.import.module_name.length == stmt->as.import.module_name.length &&
                                memcmp(prev_stmt->as.import.module_name.start,
                                       stmt->as.import.module_name.start,
                                       stmt->as.import.module_name.length) == 0)
                            {
                                prev_stmt->as.import.also_imported_directly = true;
                            }
                        }

                        /* Merge the cached module's statements into all_statements */
                        int new_all_count = all_count + cached_module->count;
                        int old_capacity = all_capacity;
                        while (new_all_count > all_capacity)
                        {
                            all_capacity = all_capacity == 0 ? 8 : all_capacity * 2;
                        }
                        if (all_capacity > old_capacity || all_statements == NULL)
                        {
                            Stmt **new_statements = arena_alloc(arena, sizeof(Stmt *) * all_capacity);
                            if (!new_statements)
                            {
                                DEBUG_ERROR("Failed to allocate memory for statements");
                                parser_cleanup(&parser);
                                lexer_cleanup(&lexer);
                                return NULL;
                            }
                            if (all_count > 0 && all_statements != NULL)
                            {
                                memmove(new_statements, all_statements, sizeof(Stmt *) * all_count);
                            }
                            all_statements = new_statements;
                        }
                        memmove(all_statements + all_count, cached_module->statements, sizeof(Stmt *) * cached_module->count);
                        all_count = new_all_count;

                        /* Remove this import statement since we merged */
                        memmove(&module->statements[i], &module->statements[i + 1], sizeof(Stmt *) * (module->count - i - 1));
                        module->count--;
                        continue;
                    }
                }
                /* True duplicate non-namespaced import: remove the import statement */
                memmove(&module->statements[i], &module->statements[i + 1], sizeof(Stmt *) * (module->count - i - 1));
                module->count--;
                continue;
            }

            if (*imported_count >= *imported_capacity)
            {
                int new_capacity = *imported_capacity == 0 ? 8 : *imported_capacity * 2;
                char **new_imported = arena_alloc(arena, sizeof(char *) * new_capacity);
                Module **new_modules = arena_alloc(arena, sizeof(Module *) * new_capacity);
                bool *new_directly = arena_alloc(arena, sizeof(bool) * new_capacity);
                if (!new_imported || !new_modules || !new_directly)
                {
                    DEBUG_ERROR("Failed to allocate memory for imported list");
                    parser_cleanup(&parser);
                    lexer_cleanup(&lexer);
                    return NULL;
                }
                if (*imported_count > 0)
                {
                    memmove(new_imported, *imported, sizeof(char *) * *imported_count);
                    if (*imported_modules != NULL)
                    {
                        memmove(new_modules, *imported_modules, sizeof(Module *) * *imported_count);
                    }
                    if (*imported_directly != NULL)
                    {
                        memmove(new_directly, *imported_directly, sizeof(bool) * *imported_count);
                    }
                }
                *imported = new_imported;
                *imported_modules = new_modules;
                *imported_directly = new_directly;
                *imported_capacity = new_capacity;
            }
            /* Store the path and reserve the module slot BEFORE recursive call */
            int module_idx = *imported_count;
            (*imported)[module_idx] = import_path;
            if (*imported_modules != NULL)
            {
                (*imported_modules)[module_idx] = NULL; /* Will be filled after parse */
            }
            if (*imported_directly != NULL)
            {
                /* Mark whether this import is direct (non-namespaced) */
                (*imported_directly)[module_idx] = (stmt->as.import.namespace == NULL);
            }
            (*imported_count)++;

            Module *imported_module = parse_module_with_imports(arena, symbol_table, import_path, imported, imported_count, imported_capacity, imported_modules, imported_directly, compiler_dir);
            if (!imported_module)
            {
                parser_cleanup(&parser);
                lexer_cleanup(&lexer);
                return NULL;
            }

            /* Store the parsed module in the cache for potential re-use by namespaced imports */
            if (*imported_modules != NULL)
            {
                (*imported_modules)[module_idx] = imported_module;
            }

            /* For namespaced imports: store statements in ImportStmt for type checker
             * to register symbols under the namespace. DON'T merge into main module
             * to prevent adding to global scope. Keep the STMT_IMPORT so type
             * checker and code generator can process it.
             *
             * IMPORTANT: The parser adds all function symbols to the symbol table
             * during parsing (for forward references). For namespaced imports, we
             * must REMOVE these symbols from global scope, since they should only
             * be accessible via the namespace. */
            if (stmt->as.import.namespace != NULL)
            {
                /* Remove imported function symbols from global scope */
                for (int j = 0; j < imported_module->count; j++)
                {
                    Stmt *imported_stmt = imported_module->statements[j];
                    if (imported_stmt != NULL && imported_stmt->type == STMT_FUNCTION)
                    {
                        symbol_table_remove_symbol_from_global(symbol_table, imported_stmt->as.function.name);
                    }
                }
                stmt->as.import.imported_stmts = imported_module->statements;
                stmt->as.import.imported_count = imported_module->count;
                i++; /* Keep the STMT_IMPORT and move to next statement */
            }
            else
            {
                /* For non-namespaced imports: merge statements and remove STMT_IMPORT */
                int new_all_count = all_count + imported_module->count;
                int old_capacity = all_capacity;
                while (new_all_count > all_capacity)
                {
                    all_capacity = all_capacity == 0 ? 8 : all_capacity * 2;
                }
                if (all_capacity > old_capacity || all_statements == NULL)
                {
                    Stmt **new_statements = arena_alloc(arena, sizeof(Stmt *) * all_capacity);
                    if (!new_statements)
                    {
                        DEBUG_ERROR("Failed to allocate memory for statements");
                        parser_cleanup(&parser);
                        lexer_cleanup(&lexer);
                        return NULL;
                    }
                    if (all_count > 0 && all_statements != NULL)
                    {
                        memmove(new_statements, all_statements, sizeof(Stmt *) * all_count);
                    }
                    all_statements = new_statements;
                }
                memmove(all_statements + all_count, imported_module->statements, sizeof(Stmt *) * imported_module->count);
                all_count = new_all_count;

                memmove(&module->statements[i], &module->statements[i + 1], sizeof(Stmt *) * (module->count - i - 1));
                module->count--;
            }
        }
        else
        {
            i++;
        }
    }

    int new_all_count = all_count + module->count;
    int old_capacity = all_capacity;
    while (new_all_count > all_capacity)
    {
        all_capacity = all_capacity == 0 ? 8 : all_capacity * 2;
    }
    if (all_capacity > old_capacity || all_statements == NULL)
    {
        Stmt **new_statements = arena_alloc(arena, sizeof(Stmt *) * all_capacity);
        if (!new_statements)
        {
            DEBUG_ERROR("Failed to allocate memory for statements");
            parser_cleanup(&parser);
            lexer_cleanup(&lexer);
            return NULL;
        }
        if (all_count > 0 && all_statements != NULL)
        {
            memmove(new_statements, all_statements, sizeof(Stmt *) * all_count);
        }
        all_statements = new_statements;
    }
    memmove(all_statements + all_count, module->statements, sizeof(Stmt *) * module->count);
    all_count = new_all_count;

    module->statements = all_statements;
    module->count = all_count;
    module->capacity = all_count;

    parser_cleanup(&parser);
    lexer_cleanup(&lexer);
    return module;
}
