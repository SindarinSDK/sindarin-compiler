/**
 * code_gen_pragma.c - Pragma handling for code generation
 *
 * Contains functions for collecting and managing pragma directives
 * (include, link, source) during code generation.
 */

#include "code_gen/code_gen_pragma.h"
#include "arena.h"
#include <string.h>

void code_gen_add_pragma_include(CodeGen *gen, const char *include)
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

void code_gen_add_pragma_link(CodeGen *gen, const char *link)
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

const char *get_directory_from_path(Arena *arena, const char *filepath)
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

void code_gen_add_pragma_source(CodeGen *gen, const char *source, const char *source_dir)
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

void code_gen_collect_pragmas(CodeGen *gen, Stmt **statements, int count)
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
