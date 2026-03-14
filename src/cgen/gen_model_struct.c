#include "cgen/gen_model.h"
#include <string.h>


/* Determine the cleanup action for a field type */
static const char *field_cleanup_action(Type *type)
{
    if (!type) return "none";
    switch (type->kind)
    {
        case TYPE_STRING:   return "free";
        case TYPE_POINTER:  return "free";  /* raw pointer fields (e.g. *byte) are heap-allocated */
        case TYPE_ARRAY:    return "cleanup_array";
        case TYPE_FUNCTION: return "none";  /* fn pointers aren't heap-allocated */
        case TYPE_STRUCT:
            if (type->as.struct_type.pass_self_by_ref)
                return "release";
            /* For as val structs, check if they have heap fields */
            for (int i = 0; i < type->as.struct_type.field_count; i++)
            {
                const char *fa = field_cleanup_action(type->as.struct_type.fields[i].type);
                if (strcmp(fa, "none") != 0) return "cleanup_val";
            }
            return "none";
        default:
            return "none";
    }
}

/* Determine the copy action for a field type */
static const char *field_copy_action(Type *type)
{
    if (!type) return "value_copy";
    switch (type->kind)
    {
        case TYPE_STRING:   return "strdup";
        case TYPE_ARRAY:    return "array_copy";
        case TYPE_FUNCTION: return "none";  /* closures are shared */
        case TYPE_STRUCT:
            if (type->as.struct_type.pass_self_by_ref)
                return "retain";
            return "copy_val";
        default:
            return "value_copy";
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

    /* Source file tracking for modular compilation */
    if (decl->name.filename)
        gen_model_add_source_file(obj, decl->name.filename);

    /* Memory mode for c-min codegen */
    json_object_object_add(obj, "mem_mode",
        json_object_new_string(decl->pass_self_by_ref ? "ref" : "val"));

    if (decl->c_alias)
    {
        json_object_object_add(obj, "c_alias", json_object_new_string(decl->c_alias));
    }

    /* Check if any field needs heap cleanup */
    bool has_heap = false;

    /* Fields */
    json_object *fields = json_object_new_array();
    for (int i = 0; i < decl->field_count; i++)
    {
        StructField *f = &decl->fields[i];
        json_object *field = json_object_new_object();
        json_object_object_add(field, "name", json_object_new_string(f->name));
        json_object_object_add(field, "type", gen_model_type(arena, f->type));
        json_object_object_add(field, "offset", json_object_new_int64((int64_t)f->offset));

        /* Per-field cleanup and copy actions for c-min codegen */
        const char *ca = field_cleanup_action(f->type);
        const char *cpa = field_copy_action(f->type);
        json_object_object_add(field, "cleanup_action", json_object_new_string(ca));
        json_object_object_add(field, "copy_action", json_object_new_string(cpa));

        if (strcmp(ca, "none") != 0) has_heap = true;

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
    json_object_object_add(obj, "has_heap_fields", json_object_new_boolean(has_heap));

    /* Methods */
    json_object *methods = json_object_new_array();
    for (int i = 0; i < decl->method_count; i++)
    {
        StructMethod *m = &decl->methods[i];
        json_object *method = json_object_new_object();
        json_object_object_add(method, "name", json_object_new_string(m->name));
        json_object_object_add(method, "return_type", gen_model_type(arena, m->return_type));
        json_object_object_add(method, "modifier", json_object_new_string(gen_model_func_mod_str(m->modifier)));
        json_object_object_add(method, "is_static", json_object_new_boolean(m->is_static));
        json_object_object_add(method, "is_native", json_object_new_boolean(m->is_native));
        json_object_object_add(method, "has_arena_param", json_object_new_boolean(m->has_arena_param));

        json_object_object_add(method, "has_c_alias", json_object_new_boolean(m->c_alias != NULL));
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
                json_object_new_string(gen_model_mem_qual_str(m->params[j].mem_qualifier)));
            json_object_object_add(p, "sync_mod",
                json_object_new_string(gen_model_sync_mod_str(m->params[j].sync_modifier)));
            gen_model_emit_param_cleanup(p, &m->params[j]);
            json_object_array_add(params, p);
        }
        json_object_object_add(method, "params", params);

        /* Method body */
        json_object *body = json_object_new_array();

        /* Prepend param guard locals for as-val-on-as-ref-struct override params */
        for (int j = 0; j < m->param_count; j++)
        {
            if (m->params[j].mem_qualifier == MEM_AS_VAL &&
                m->params[j].type && m->params[j].type->kind == TYPE_STRUCT &&
                m->params[j].type->as.struct_type.pass_self_by_ref)
            {
                char buf[512];
                snprintf(buf, sizeof(buf),
                    "sn_auto_%s __sn__%s *__sn__%s__pc = __sn__%s;",
                    m->params[j].type->as.struct_type.name,
                    m->params[j].type->as.struct_type.name,
                    m->params[j].name.start,
                    m->params[j].name.start);
                json_object *guard = json_object_new_object();
                json_object_object_add(guard, "kind", json_object_new_string("raw_c"));
                json_object_object_add(guard, "code", json_object_new_string(buf));
                json_object_array_add(body, guard);
            }
        }

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
