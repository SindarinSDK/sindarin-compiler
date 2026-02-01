// parser.c
// Main parser module that orchestrates import processing

#include "parser.h"
#include "parser/parser_util.h"
#include "parser/parser_expr.h"
#include "parser/parser_stmt.h"
#include "debug.h"
#include "diagnostic.h"
#include "file.h"
#include "gcc_backend.h"
#include <stdio.h>
#include <string.h>

/* Include split modules */
#include "parser_import_util.c"
#include "parser_init.c"
#include "parser_import.c"

/* ============================================================================
 * Main Import Processing Function
 * ============================================================================ */

Module *parse_module_with_imports(Arena *arena, SymbolTable *symbol_table, const char *filename,
                                  char ***imported, int *imported_count, int *imported_capacity,
                                  Module ***imported_modules, bool **imported_directly,
                                  bool **namespace_code_emitted, const char *compiler_dir)
{
    char *source = file_read(arena, filename);
    if (!source)
    {
        diagnostic_error_simple("cannot read file '%s'", filename);
        return NULL;
    }

    Lexer lexer;
    lexer_init(arena, &lexer, source, filename);

    Parser parser;
    parser_init(arena, &parser, &lexer, symbol_table);

    /* Set up import context for import-first processing.
     * This allows types from imported modules to be registered
     * DURING parsing, before they are referenced.
     * Note: We pass the pointer-to-pointer for arrays so all recursive calls
     * share the same underlying pointer, allowing reallocation to be visible
     * to all callers in the recursive chain. */
    ImportContext import_ctx;
    import_ctx.imported = imported;
    import_ctx.imported_count = imported_count;
    import_ctx.imported_capacity = imported_capacity;
    import_ctx.imported_modules = imported_modules;
    import_ctx.imported_directly = imported_directly;
    import_ctx.namespace_code_emitted = namespace_code_emitted;
    import_ctx.current_file = filename;
    import_ctx.compiler_dir = compiler_dir;
    import_ctx.process_import = process_import_callback;

    parser.import_ctx = &import_ctx;

    Module *module = parser_execute(&parser, filename);

    /* No need to update caller's pointers - the ImportContext uses pointer-to-pointer
     * for arrays, so all updates are automatically visible to the caller. */

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

            /* Normalize path to remove redundant ./ components for consistent cache lookup */
            import_path = normalize_path(arena, import_path);
            if (!import_path) {
                DEBUG_ERROR("Failed to normalize import path");
                parser_cleanup(&parser);
                lexer_cleanup(&lexer);
                return NULL;
            }

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

            /* Check if the import file exists before attempting to parse it */
            if (!import_file_exists(import_path)) {
                diagnostic_error_at(&mod_name, "cannot find module '%s'", mod_name.start);
                parser_cleanup(&parser);
                lexer_cleanup(&lexer);
                return NULL;
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
                     * Determine if code gen should skip emitting this module's functions:
                     * 1. If module was imported directly (without namespace), functions are in main scope
                     * 2. If another namespace import already claimed code emission for this module */
                    Module *cached_module = (*imported_modules)[already_imported_idx];
                    if (cached_module != NULL)
                    {
                        stmt->as.import.imported_stmts = cached_module->statements;
                        stmt->as.import.imported_count = cached_module->count;

                        /* Check if we should emit code for this namespace import */
                        bool should_skip_emit = was_imported_directly;
                        if (!should_skip_emit && *namespace_code_emitted != NULL) {
                            DEBUG_VERBOSE("Checking namespace_code_emitted[%d] for '%.*s': %s",
                                         already_imported_idx,
                                         (int)stmt->as.import.module_name.length,
                                         stmt->as.import.module_name.start,
                                         (*namespace_code_emitted)[already_imported_idx] ? "true" : "false");
                            if ((*namespace_code_emitted)[already_imported_idx]) {
                                /* Another namespace import already claimed emission */
                                should_skip_emit = true;
                            } else {
                                /* This is the first namespace import - claim emission */
                                (*namespace_code_emitted)[already_imported_idx] = true;
                                DEBUG_VERBOSE("Set namespace_code_emitted[%d] = true for '%.*s'",
                                             already_imported_idx,
                                             (int)stmt->as.import.module_name.length,
                                             stmt->as.import.module_name.start);
                            }
                        }
                        DEBUG_VERBOSE("Namespace import '%.*s' should_skip_emit=%s",
                                     (int)stmt->as.import.module_name.length,
                                     stmt->as.import.module_name.start,
                                     should_skip_emit ? "true" : "false");
                        stmt->as.import.also_imported_directly = should_skip_emit;

                        /* If this import will emit (not skip), mark all nested namespace imports
                         * within the imported module to skip emission. This prevents the same
                         * module from being emitted multiple times through different import paths. */
                        if (!should_skip_emit && cached_module != NULL)
                        {
                            const char *cached_file = (*imported)[already_imported_idx];
                            for (int j = 0; j < cached_module->count; j++)
                            {
                                Stmt *nested_stmt = cached_module->statements[j];
                                if (nested_stmt != NULL && nested_stmt->type == STMT_IMPORT &&
                                    nested_stmt->as.import.namespace != NULL)
                                {
                                    /* Find this nested import's module index */
                                    Token nested_mod_name = nested_stmt->as.import.module_name;

                                    /* Construct the full import path using the imported module's directory */
                                    char *nested_path = construct_import_path(arena, cached_file, nested_mod_name.start);
                                    if (!nested_path) continue;

                                    /* Try SDK path if relative doesn't exist */
                                    if (!import_file_exists(nested_path) && compiler_dir) {
                                        const char *sdk_path = gcc_resolve_sdk_import(compiler_dir, nested_mod_name.start);
                                        if (sdk_path) {
                                            nested_path = arena_strdup(arena, sdk_path);
                                        }
                                    }

                                    /* Find in imported array */
                                    for (int k = 0; k < *imported_count; k++)
                                    {
                                        if (nested_path && strcmp((*imported)[k], nested_path) == 0)
                                        {
                                            /* Check if this nested module's emission is already claimed */
                                            if ((*namespace_code_emitted)[k])
                                            {
                                                /* Already claimed - mark this nested import to skip */
                                                nested_stmt->as.import.also_imported_directly = true;
                                                DEBUG_VERBOSE("Nested import '%.*s' in '%.*s' set to skip (already claimed)",
                                                             (int)nested_mod_name.length, nested_mod_name.start,
                                                             (int)stmt->as.import.module_name.length, stmt->as.import.module_name.start);
                                            }
                                            break;
                                        }
                                    }
                                }
                            }
                        }

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
                bool *new_ns_emitted = arena_alloc(arena, sizeof(bool) * new_capacity);
                if (!new_imported || !new_modules || !new_directly || !new_ns_emitted)
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
                    if (*namespace_code_emitted != NULL)
                    {
                        memmove(new_ns_emitted, *namespace_code_emitted, sizeof(bool) * *imported_count);
                    }
                }
                *imported = new_imported;
                *imported_modules = new_modules;
                *imported_directly = new_directly;
                *namespace_code_emitted = new_ns_emitted;
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
            if (*namespace_code_emitted != NULL)
            {
                /* For namespace imports, claim code emission BEFORE recursive parsing.
                 * This ensures nested imports of the same module see that it's already claimed.
                 * For non-namespace (direct) imports, leave it false since code merges into main. */
                (*namespace_code_emitted)[module_idx] = (stmt->as.import.namespace != NULL);
                if (stmt->as.import.namespace != NULL)
                {
                    DEBUG_VERBOSE("First namespace import of '%.*s' claims emission (idx=%d)",
                                 (int)stmt->as.import.module_name.length,
                                 stmt->as.import.module_name.start,
                                 module_idx);
                }
            }
            (*imported_count)++;

            Module *imported_module = parse_module_with_imports(arena, symbol_table, import_path, imported, imported_count, imported_capacity, imported_modules, imported_directly, namespace_code_emitted, compiler_dir);
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

                /* Note: namespace_code_emitted was already set before parsing */

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
