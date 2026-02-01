// parser_import.c
// Import processing during parsing

/* Forward declaration for recursive import processing */
static Module *process_import_callback(Arena *arena, SymbolTable *symbol_table, const char *import_path,
                                       ImportContext *ctx);

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

    /* Check if the import file exists before attempting to parse it */
    if (!import_file_exists(import_path)) {
        diagnostic_error_at(&parser->previous, "cannot find module '%s'", module_name);
        parser->had_error = 1;
        return NULL;
    }

    /* Check if already imported */
    for (int j = 0; j < *ctx->imported_count; j++) {
        if (strcmp((*ctx->imported)[j], import_path) == 0) {
            /* Already imported - for non-namespaced imports, return NULL to prevent
             * the caller from storing statements that would be merged again.
             * For namespaced imports, return the cached module so the namespace
             * can be set up (namespaced imports don't get merged, they stay as STMT_IMPORT). */
            if (is_namespaced) {
                return (*ctx->imported_modules)[j];
            }
            return NULL;
        }
    }

    /* Ensure capacity in imported arrays */
    if (*ctx->imported_count >= *ctx->imported_capacity) {
        int new_capacity = *ctx->imported_capacity == 0 ? 8 : *ctx->imported_capacity * 2;
        char **new_imported = arena_alloc(parser->arena, sizeof(char *) * new_capacity);
        Module **new_modules = arena_alloc(parser->arena, sizeof(Module *) * new_capacity);
        bool *new_directly = arena_alloc(parser->arena, sizeof(bool) * new_capacity);
        bool *new_ns_emitted = arena_alloc(parser->arena, sizeof(bool) * new_capacity);
        if (!new_imported || !new_modules || !new_directly || !new_ns_emitted) {
            parser_error(parser, "Failed to allocate memory for imported list");
            return NULL;
        }
        if (*ctx->imported_count > 0) {
            memmove(new_imported, *ctx->imported, sizeof(char *) * *ctx->imported_count);
            memmove(new_modules, *ctx->imported_modules, sizeof(Module *) * *ctx->imported_count);
            memmove(new_directly, *ctx->imported_directly, sizeof(bool) * *ctx->imported_count);
            memmove(new_ns_emitted, *ctx->namespace_code_emitted, sizeof(bool) * *ctx->imported_count);
        }
        /* Initialize new slots to false */
        memset(new_ns_emitted + *ctx->imported_count, 0, sizeof(bool) * (new_capacity - *ctx->imported_count));
        *ctx->imported = new_imported;
        *ctx->imported_modules = new_modules;
        *ctx->imported_directly = new_directly;
        *ctx->namespace_code_emitted = new_ns_emitted;
        *ctx->imported_capacity = new_capacity;
    }

    /* Reserve slot before recursive call to prevent infinite recursion on circular imports */
    int module_idx = *ctx->imported_count;
    (*ctx->imported)[module_idx] = import_path;
    (*ctx->imported_modules)[module_idx] = NULL;
    (*ctx->imported_directly)[module_idx] = !is_namespaced;
    /* Initialize code emission tracking to false. The actual STMT_IMPORT processing
     * in process_all_modules will claim emission for the first namespace import.
     * We don't claim here because this same STMT_IMPORT will be processed later
     * and it should be the one to emit (not skip). */
    (*ctx->namespace_code_emitted)[module_idx] = false;
    (*ctx->imported_count)++;

    /* Process the import via the callback */
    Module *imported_module = ctx->process_import(parser->arena, parser->symbol_table, import_path, ctx);
    if (!imported_module) {
        /* Import failed - mark parser as having an error */
        parser->had_error = 1;
        return NULL;
    }

    /* Store the parsed module in the cache */
    (*ctx->imported_modules)[module_idx] = imported_module;

    return imported_module;
}

/* Callback function for recursive import processing */
static Module *process_import_callback(Arena *arena, SymbolTable *symbol_table, const char *import_path,
                                       ImportContext *parent_ctx)
{
    char *source = file_read(arena, import_path);
    if (!source) {
        diagnostic_error_simple("cannot read module '%s'", import_path);
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
    import_ctx.namespace_code_emitted = parent_ctx->namespace_code_emitted;
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
