/**
 * code_gen_struct_emit.c - Struct typedef emission for code generation
 *
 * Contains functions for emitting struct typedefs and native struct
 * forward declarations during code generation.
 */

#include "code_gen/emit/code_gen_struct_emit.h"
#include "code_gen/util/code_gen_util.h"
#include "arena.h"
#include <string.h>

void code_gen_emit_struct_typedef(CodeGen *gen, StructDeclStmt *struct_decl, int *count_ptr)
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

void code_gen_emit_imported_struct_typedefs(CodeGen *gen, Stmt **statements, int count, int *struct_count)
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

void code_gen_emit_native_alias_fwd(CodeGen *gen, StructDeclStmt *struct_decl,
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

void code_gen_emit_imported_native_aliases(CodeGen *gen, Stmt **statements, int count,
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
