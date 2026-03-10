#include "gen_model/gen_model.h"
#include <stdio.h>
#include <string.h>

/* Global lambda collection */
json_object *g_model_lambdas = NULL;

/* Global thread collection */
json_object *g_model_threads = NULL;
int g_model_thread_count = 0;

/* Global captured-variable set */
char **g_captured_vars = NULL;
int g_captured_var_count = 0;

json_object *gen_model_build(Arena *arena, Module *module, SymbolTable *symbol_table,
                             ArithmeticMode arithmetic_mode)
{
    json_object *root = json_object_new_object();

    /* Module metadata */
    json_object *mod = json_object_new_object();
    json_object_object_add(mod, "filename",
        json_object_new_string(module->filename ? module->filename : ""));

    /* Check if module has a main function and forward-declarable functions */
    bool has_main = false;
    bool main_returns = false;
    bool has_forward_decls = false;
    for (int i = 0; i < module->count; i++)
    {
        Stmt *stmt = module->statements[i];
        if (stmt->type == STMT_FUNCTION)
        {
            if (strcmp(stmt->as.function.name.start, "main") == 0)
            {
                has_main = true;
                int bc = stmt->as.function.body_count;
                if (bc > 0 && stmt->as.function.body[bc - 1]->type == STMT_RETURN)
                    main_returns = true;
            }
            else if (!stmt->as.function.is_native)
            {
                has_forward_decls = true;
            }
        }
    }
    json_object_object_add(mod, "has_main", json_object_new_boolean(has_main));
    json_object_object_add(mod, "main_returns", json_object_new_boolean(main_returns));
    json_object_object_add(mod, "has_forward_decls", json_object_new_boolean(has_forward_decls));
    json_object_object_add(root, "module", mod);

    /* Collect top-level declarations into categorized arrays */
    json_object *pragmas = json_object_new_array();
    json_object *imports = json_object_new_array();
    json_object *type_decls = json_object_new_array();
    json_object *structs = json_object_new_array();
    json_object *globals = json_object_new_array();
    json_object *functions = json_object_new_array();
    json_object *top_level = json_object_new_array();

    /* Initialize global lambda collection */
    g_model_lambdas = json_object_new_array();

    /* Initialize global thread collection */
    g_model_threads = json_object_new_array();
    g_model_thread_count = 0;

    for (int i = 0; i < module->count; i++)
    {
        Stmt *stmt = module->statements[i];

        switch (stmt->type)
        {
            case STMT_PRAGMA:
            {
                json_object_array_add(pragmas,
                    gen_model_stmt(arena, stmt, symbol_table, arithmetic_mode));
                break;
            }

            case STMT_IMPORT:
            {
                json_object_array_add(imports,
                    gen_model_stmt(arena, stmt, symbol_table, arithmetic_mode));
                break;
            }

            case STMT_TYPE_DECL:
            {
                json_object_array_add(type_decls,
                    gen_model_stmt(arena, stmt, symbol_table, arithmetic_mode));
                break;
            }

            case STMT_STRUCT_DECL:
            {
                json_object_array_add(structs,
                    gen_model_struct(arena, &stmt->as.struct_decl, symbol_table, arithmetic_mode));
                break;
            }

            case STMT_VAR_DECL:
            {
                json_object *gvar = gen_model_stmt(arena, stmt, symbol_table, arithmetic_mode);
                /* Check if initializer is non-constant (needs deferred init in main) */
                if (stmt->as.var_decl.initializer &&
                    stmt->as.var_decl.initializer->type != EXPR_LITERAL)
                {
                    json_object_object_add(gvar, "is_deferred", json_object_new_boolean(true));
                }
                json_object_array_add(globals, gvar);
                break;
            }

            case STMT_FUNCTION:
            {
                json_object_array_add(functions,
                    gen_model_function(arena, &stmt->as.function, symbol_table, arithmetic_mode));
                break;
            }

            default:
            {
                /* Other top-level statements (expression statements, etc.) */
                json_object_array_add(top_level,
                    gen_model_stmt(arena, stmt, symbol_table, arithmetic_mode));
                break;
            }
        }
    }

    json_object_object_add(root, "pragmas", pragmas);
    json_object_object_add(root, "imports", imports);
    json_object_object_add(root, "type_decls", type_decls);
    json_object_object_add(root, "structs", structs);
    json_object_object_add(root, "globals", globals);
    json_object_object_add(root, "functions", functions);
    json_object_object_add(root, "top_level_statements", top_level);
    json_object_object_add(root, "lambdas", g_model_lambdas);
    g_model_lambdas = NULL;
    json_object_object_add(root, "threads", g_model_threads);
    g_model_threads = NULL;

    return root;
}

int gen_model_write(json_object *model, const char *output_path)
{
    const char *json_str = json_object_to_json_string_ext(model,
        JSON_C_TO_STRING_PRETTY | JSON_C_TO_STRING_SPACED);

    if (!json_str)
    {
        fprintf(stderr, "Error: Failed to serialize model to JSON\n");
        return 1;
    }

    FILE *f = fopen(output_path, "wb");
    if (!f)
    {
        fprintf(stderr, "Error: Cannot open output file: %s\n", output_path);
        return 1;
    }

    fprintf(f, "%s\n", json_str);
    fclose(f);

    return 0;
}
