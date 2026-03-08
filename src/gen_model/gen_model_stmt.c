#include "gen_model/gen_model.h"
#include <string.h>

static const char *mem_qual_str(MemoryQualifier mq)
{
    switch (mq)
    {
        case MEM_DEFAULT: return "default";
        case MEM_AS_VAL:  return "as_val";
        case MEM_AS_REF:  return "as_ref";
        default:          return "default";
    }
}

static const char *sync_mod_str(SyncModifier sm)
{
    switch (sm)
    {
        case SYNC_NONE:   return "none";
        case SYNC_ATOMIC: return "atomic";
        default:          return "none";
    }
}

json_object *gen_model_stmt(Arena *arena, Stmt *stmt, SymbolTable *symbol_table,
                            ArithmeticMode arithmetic_mode)
{
    if (!stmt) return json_object_new_null();

    json_object *obj = json_object_new_object();

    /* Attached comments */
    if (stmt->comment_count > 0)
    {
        json_object *comments = json_object_new_array();
        for (int i = 0; i < stmt->comment_count; i++)
        {
            json_object_array_add(comments, json_object_new_string(stmt->comments[i]));
        }
        json_object_object_add(obj, "comments", comments);
    }

    switch (stmt->type)
    {
        case STMT_EXPR:
        {
            json_object_object_add(obj, "kind", json_object_new_string("expr"));
            json_object_object_add(obj, "expr",
                gen_model_expr(arena, stmt->as.expression.expression, symbol_table, arithmetic_mode));
            break;
        }

        case STMT_VAR_DECL:
        {
            json_object_object_add(obj, "kind", json_object_new_string("var_decl"));
            json_object_object_add(obj, "name",
                json_object_new_string(stmt->as.var_decl.name.start));
            json_object_object_add(obj, "type",
                gen_model_type(arena, stmt->as.var_decl.type));
            json_object_object_add(obj, "mem_qual",
                json_object_new_string(mem_qual_str(stmt->as.var_decl.mem_qualifier)));
            json_object_object_add(obj, "sync_mod",
                json_object_new_string(sync_mod_str(stmt->as.var_decl.sync_modifier)));
            json_object_object_add(obj, "is_static",
                json_object_new_boolean(stmt->as.var_decl.is_static));
            /* Check if this variable is captured by a lambda (needs promoted storage) */
            {
                const char *vname = stmt->as.var_decl.name.start;
                bool captured = false;
                for (int ci = 0; ci < g_captured_var_count; ci++) {
                    if (strcmp(g_captured_vars[ci], vname) == 0) { captured = true; break; }
                }
                if (captured)
                    json_object_object_add(obj, "is_captured", json_object_new_boolean(true));
            }
            if (stmt->as.var_decl.initializer)
            {
                json_object_object_add(obj, "initializer",
                    gen_model_expr(arena, stmt->as.var_decl.initializer, symbol_table, arithmetic_mode));
            }
            break;
        }

        case STMT_RETURN:
        {
            json_object_object_add(obj, "kind", json_object_new_string("return"));
            if (stmt->as.return_stmt.value)
            {
                json_object_object_add(obj, "value",
                    gen_model_expr(arena, stmt->as.return_stmt.value, symbol_table, arithmetic_mode));
            }
            break;
        }

        case STMT_BLOCK:
        {
            json_object_object_add(obj, "kind", json_object_new_string("block"));
            json_object *stmts = json_object_new_array();
            for (int i = 0; i < stmt->as.block.count; i++)
            {
                json_object_array_add(stmts,
                    gen_model_stmt(arena, stmt->as.block.statements[i], symbol_table, arithmetic_mode));
            }
            json_object_object_add(obj, "statements", stmts);
            break;
        }

        case STMT_IF:
        {
            json_object_object_add(obj, "kind", json_object_new_string("if"));
            json_object_object_add(obj, "condition",
                gen_model_expr(arena, stmt->as.if_stmt.condition, symbol_table, arithmetic_mode));
            /* Wrap non-block then_body in a synthetic block so templates can always iterate .statements */
            if (stmt->as.if_stmt.then_branch->type == STMT_BLOCK)
            {
                json_object_object_add(obj, "then_body",
                    gen_model_stmt(arena, stmt->as.if_stmt.then_branch, symbol_table, arithmetic_mode));
            }
            else
            {
                json_object *block = json_object_new_object();
                json_object_object_add(block, "kind", json_object_new_string("block"));
                json_object *stmts = json_object_new_array();
                json_object_array_add(stmts,
                    gen_model_stmt(arena, stmt->as.if_stmt.then_branch, symbol_table, arithmetic_mode));
                json_object_object_add(block, "statements", stmts);
                json_object_object_add(obj, "then_body", block);
            }
            if (stmt->as.if_stmt.else_branch)
            {
                if (stmt->as.if_stmt.else_branch->type == STMT_BLOCK)
                {
                    json_object_object_add(obj, "else_body",
                        gen_model_stmt(arena, stmt->as.if_stmt.else_branch, symbol_table, arithmetic_mode));
                }
                else
                {
                    json_object *block = json_object_new_object();
                    json_object_object_add(block, "kind", json_object_new_string("block"));
                    json_object *stmts = json_object_new_array();
                    json_object_array_add(stmts,
                        gen_model_stmt(arena, stmt->as.if_stmt.else_branch, symbol_table, arithmetic_mode));
                    json_object_object_add(block, "statements", stmts);
                    json_object_object_add(obj, "else_body", block);
                }
            }
            break;
        }

        case STMT_WHILE:
        {
            json_object_object_add(obj, "kind", json_object_new_string("while"));
            json_object_object_add(obj, "condition",
                gen_model_expr(arena, stmt->as.while_stmt.condition, symbol_table, arithmetic_mode));
            json_object_object_add(obj, "body",
                gen_model_stmt(arena, stmt->as.while_stmt.body, symbol_table, arithmetic_mode));
            break;
        }

        case STMT_FOR:
        {
            json_object_object_add(obj, "kind", json_object_new_string("for"));
            if (stmt->as.for_stmt.initializer)
            {
                json_object_object_add(obj, "init",
                    gen_model_stmt(arena, stmt->as.for_stmt.initializer, symbol_table, arithmetic_mode));
            }
            if (stmt->as.for_stmt.condition)
            {
                json_object_object_add(obj, "condition",
                    gen_model_expr(arena, stmt->as.for_stmt.condition, symbol_table, arithmetic_mode));
            }
            if (stmt->as.for_stmt.increment)
            {
                json_object_object_add(obj, "increment",
                    gen_model_expr(arena, stmt->as.for_stmt.increment, symbol_table, arithmetic_mode));
            }
            json_object_object_add(obj, "body",
                gen_model_stmt(arena, stmt->as.for_stmt.body, symbol_table, arithmetic_mode));
            break;
        }

        case STMT_FOR_EACH:
        {
            json_object_object_add(obj, "kind", json_object_new_string("for_each"));
            json_object_object_add(obj, "iterator_name",
                json_object_new_string(stmt->as.for_each_stmt.var_name.start));
            json_object_object_add(obj, "iterable",
                gen_model_expr(arena, stmt->as.for_each_stmt.iterable, symbol_table, arithmetic_mode));
            json_object_object_add(obj, "body",
                gen_model_stmt(arena, stmt->as.for_each_stmt.body, symbol_table, arithmetic_mode));
            break;
        }

        case STMT_BREAK:
        {
            json_object_object_add(obj, "kind", json_object_new_string("break"));
            break;
        }

        case STMT_CONTINUE:
        {
            json_object_object_add(obj, "kind", json_object_new_string("continue"));
            break;
        }

        case STMT_IMPORT:
        {
            json_object_object_add(obj, "kind", json_object_new_string("import"));
            json_object_object_add(obj, "module_name",
                json_object_new_string(stmt->as.import.module_name.start));
            if (stmt->as.import.namespace)
            {
                json_object_object_add(obj, "namespace",
                    json_object_new_string(stmt->as.import.namespace->start));
            }
            json_object_object_add(obj, "also_imported_directly",
                json_object_new_boolean(stmt->as.import.also_imported_directly));
            break;
        }

        case STMT_PRAGMA:
        {
            json_object_object_add(obj, "kind", json_object_new_string("pragma"));
            const char *ptype = "unknown";
            switch (stmt->as.pragma.pragma_type)
            {
                case PRAGMA_INCLUDE: ptype = "include"; break;
                case PRAGMA_LINK:    ptype = "link"; break;
                case PRAGMA_SOURCE:  ptype = "source"; break;
                case PRAGMA_PACK:    ptype = "pack"; break;
                case PRAGMA_ALIAS:   ptype = "alias"; break;
            }
            json_object_object_add(obj, "pragma_type", json_object_new_string(ptype));
            if (stmt->as.pragma.value)
            {
                json_object_object_add(obj, "value",
                    json_object_new_string(stmt->as.pragma.value));
            }
            break;
        }

        case STMT_TYPE_DECL:
        {
            json_object_object_add(obj, "kind", json_object_new_string("type_decl"));
            json_object_object_add(obj, "name",
                json_object_new_string(stmt->as.type_decl.name.start));
            json_object_object_add(obj, "type",
                gen_model_type(arena, stmt->as.type_decl.type));
            break;
        }

        case STMT_STRUCT_DECL:
        {
            json_object_object_add(obj, "kind", json_object_new_string("struct_decl"));
            json_object_object_add(obj, "struct",
                gen_model_struct(arena, &stmt->as.struct_decl, symbol_table, arithmetic_mode));
            break;
        }

        case STMT_FUNCTION:
        {
            json_object_object_add(obj, "kind", json_object_new_string("function"));
            json_object_object_add(obj, "function",
                gen_model_function(arena, &stmt->as.function, symbol_table, arithmetic_mode));
            break;
        }

        case STMT_LOCK:
        {
            json_object_object_add(obj, "kind", json_object_new_string("lock"));
            json_object_object_add(obj, "lock_expr",
                gen_model_expr(arena, stmt->as.lock_stmt.lock_expr, symbol_table, arithmetic_mode));
            json_object_object_add(obj, "body",
                gen_model_stmt(arena, stmt->as.lock_stmt.body, symbol_table, arithmetic_mode));
            break;
        }

        case STMT_USING:
        {
            json_object_object_add(obj, "kind", json_object_new_string("using"));
            json_object_object_add(obj, "name",
                json_object_new_string(stmt->as.using_stmt.name.start));
            if (stmt->as.using_stmt.type)
            {
                json_object_object_add(obj, "type",
                    gen_model_type(arena, stmt->as.using_stmt.type));
            }
            json_object_object_add(obj, "initializer",
                gen_model_expr(arena, stmt->as.using_stmt.initializer, symbol_table, arithmetic_mode));
            json_object_object_add(obj, "body",
                gen_model_stmt(arena, stmt->as.using_stmt.body, symbol_table, arithmetic_mode));
            break;
        }
    }

    return obj;
}
