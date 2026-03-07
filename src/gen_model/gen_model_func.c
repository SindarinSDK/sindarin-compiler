#include "gen_model/gen_model.h"

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

static const char *func_mod_str(FunctionModifier fm)
{
    switch (fm)
    {
        case FUNC_DEFAULT: return "default";
        default:           return "default";
    }
}

json_object *gen_model_function(Arena *arena, FunctionStmt *func, SymbolTable *symbol_table,
                                ArithmeticMode arithmetic_mode)
{
    if (!func) return json_object_new_null();

    json_object *obj = json_object_new_object();

    json_object_object_add(obj, "name", json_object_new_string(func->name.start));
    json_object_object_add(obj, "return_type", gen_model_type(arena, func->return_type));
    json_object_object_add(obj, "modifier", json_object_new_string(func_mod_str(func->modifier)));
    json_object_object_add(obj, "is_native", json_object_new_boolean(func->is_native));
    json_object_object_add(obj, "is_variadic", json_object_new_boolean(func->is_variadic));
    json_object_object_add(obj, "has_arena_param", json_object_new_boolean(func->has_arena_param));

    if (func->c_alias)
    {
        json_object_object_add(obj, "c_alias", json_object_new_string(func->c_alias));
    }

    /* Parameters */
    json_object *params = json_object_new_array();
    for (int i = 0; i < func->param_count; i++)
    {
        json_object *p = json_object_new_object();
        json_object_object_add(p, "name", json_object_new_string(func->params[i].name.start));
        json_object_object_add(p, "type", gen_model_type(arena, func->params[i].type));
        json_object_object_add(p, "mem_qual",
            json_object_new_string(mem_qual_str(func->params[i].mem_qualifier)));
        json_object_object_add(p, "sync_mod",
            json_object_new_string(sync_mod_str(func->params[i].sync_modifier)));
        json_object_array_add(params, p);
    }
    json_object_object_add(obj, "params", params);

    /* Body */
    json_object *body = json_object_new_array();
    if (func->body)
    {
        for (int i = 0; i < func->body_count; i++)
        {
            json_object_array_add(body,
                gen_model_stmt(arena, func->body[i], symbol_table, arithmetic_mode));
        }
    }
    json_object_object_add(obj, "body", body);

    return obj;
}
