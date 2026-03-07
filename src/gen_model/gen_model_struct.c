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

json_object *gen_model_struct(Arena *arena, StructDeclStmt *decl, SymbolTable *symbol_table,
                              ArithmeticMode arithmetic_mode)
{
    if (!decl) return json_object_new_null();

    json_object *obj = json_object_new_object();

    json_object_object_add(obj, "name", json_object_new_string(decl->name.start));
    json_object_object_add(obj, "is_native", json_object_new_boolean(decl->is_native));
    json_object_object_add(obj, "is_packed", json_object_new_boolean(decl->is_packed));
    json_object_object_add(obj, "pass_self_by_ref", json_object_new_boolean(decl->pass_self_by_ref));

    if (decl->c_alias)
    {
        json_object_object_add(obj, "c_alias", json_object_new_string(decl->c_alias));
    }

    /* Fields */
    json_object *fields = json_object_new_array();
    for (int i = 0; i < decl->field_count; i++)
    {
        StructField *f = &decl->fields[i];
        json_object *field = json_object_new_object();
        json_object_object_add(field, "name", json_object_new_string(f->name));
        json_object_object_add(field, "type", gen_model_type(arena, f->type));
        json_object_object_add(field, "offset", json_object_new_int64((int64_t)f->offset));
        if (f->c_alias)
        {
            json_object_object_add(field, "c_alias", json_object_new_string(f->c_alias));
        }
        if (f->default_value)
        {
            json_object_object_add(field, "default_value",
                gen_model_expr(arena, f->default_value, symbol_table, arithmetic_mode));
        }
        json_object_array_add(fields, field);
    }
    json_object_object_add(obj, "fields", fields);

    /* Methods */
    json_object *methods = json_object_new_array();
    for (int i = 0; i < decl->method_count; i++)
    {
        StructMethod *m = &decl->methods[i];
        json_object *method = json_object_new_object();
        json_object_object_add(method, "name", json_object_new_string(m->name));
        json_object_object_add(method, "return_type", gen_model_type(arena, m->return_type));
        json_object_object_add(method, "modifier", json_object_new_string(func_mod_str(m->modifier)));
        json_object_object_add(method, "is_static", json_object_new_boolean(m->is_static));
        json_object_object_add(method, "is_native", json_object_new_boolean(m->is_native));
        json_object_object_add(method, "has_arena_param", json_object_new_boolean(m->has_arena_param));

        if (m->c_alias)
        {
            json_object_object_add(method, "c_alias", json_object_new_string(m->c_alias));
        }

        /* Method parameters */
        json_object *params = json_object_new_array();
        for (int j = 0; j < m->param_count; j++)
        {
            json_object *p = json_object_new_object();
            json_object_object_add(p, "name", json_object_new_string(m->params[j].name.start));
            json_object_object_add(p, "type", gen_model_type(arena, m->params[j].type));
            json_object_object_add(p, "mem_qual",
                json_object_new_string(mem_qual_str(m->params[j].mem_qualifier)));
            json_object_object_add(p, "sync_mod",
                json_object_new_string(sync_mod_str(m->params[j].sync_modifier)));
            json_object_array_add(params, p);
        }
        json_object_object_add(method, "params", params);

        /* Method body */
        json_object *body = json_object_new_array();
        if (m->body)
        {
            for (int j = 0; j < m->body_count; j++)
            {
                json_object_array_add(body,
                    gen_model_stmt(arena, m->body[j], symbol_table, arithmetic_mode));
            }
        }
        json_object_object_add(method, "body", body);

        json_object_array_add(methods, method);
    }
    json_object_object_add(obj, "methods", methods);

    return obj;
}
