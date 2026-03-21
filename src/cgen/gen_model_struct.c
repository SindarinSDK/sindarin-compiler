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
        case TYPE_FUNCTION: return "free";  /* closures are heap-allocated */
        case TYPE_STRUCT:
        {
            TypeCategory cat = gen_model_type_category(type);
            if (cat == TYPE_CAT_REFCOUNTED) return "release";
            if (cat == TYPE_CAT_COMPOSITE) return "cleanup_val";
            return "none";
        }
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
        case TYPE_FUNCTION: return "closure_copy";  /* deep-copy heap closures */
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
    json_object_object_add(obj, "is_serializable", json_object_new_boolean(decl->is_serializable));

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

        /* Serialization metadata for @serializable structs */
        if (decl->is_serializable)
        {
            const char *serial_action = NULL;
            const char *serial_struct_name = NULL;
            const char *serial_elem_struct_name = NULL;

            if (f->type)
            {
                switch (f->type->kind)
                {
                    case TYPE_STRING: serial_action = "str"; break;
                    case TYPE_INT:    serial_action = "int"; break;
                    case TYPE_DOUBLE: serial_action = "double"; break;
                    case TYPE_BOOL:   serial_action = "bool"; break;
                    case TYPE_STRUCT:
                        serial_action = "object";
                        serial_struct_name = f->type->as.struct_type.name;
                        break;
                    case TYPE_ARRAY:
                        if (f->type->as.array.element_type)
                        {
                            Type *elem = f->type->as.array.element_type;
                            switch (elem->kind)
                            {
                                case TYPE_STRING: serial_action = "array_str"; break;
                                case TYPE_INT:    serial_action = "array_int"; break;
                                case TYPE_DOUBLE: serial_action = "array_double"; break;
                                case TYPE_BOOL:   serial_action = "array_bool"; break;
                                case TYPE_STRUCT:
                                    serial_action = "array_object";
                                    serial_elem_struct_name = elem->as.struct_type.name;
                                    break;
                                default: break;
                            }
                        }
                        break;
                    default: break;
                }
            }
            if (serial_action)
                json_object_object_add(field, "serial_action", json_object_new_string(serial_action));
            if (serial_struct_name)
                json_object_object_add(field, "serial_struct_name", json_object_new_string(serial_struct_name));
            if (serial_elem_struct_name)
                json_object_object_add(field, "serial_elem_struct_name", json_object_new_string(serial_elem_struct_name));
        }

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
        /* Strip leading/trailing double-underscores from operator method names,
         * e.g. "__op_eq__" → "op_eq", so the template emits the correct C suffix. */
        const char *emit_name = m->name;
        if (m->is_operator && emit_name)
        {
            size_t rlen = strlen(emit_name);
            if (rlen > 4 &&
                emit_name[0] == '_' && emit_name[1] == '_' &&
                emit_name[rlen - 1] == '_' && emit_name[rlen - 2] == '_')
            {
                emit_name = arena_strndup(arena, emit_name + 2, rlen - 4);
            }
        }
        json_object_object_add(method, "name", json_object_new_string(emit_name));
        json_object_object_add(method, "return_type", gen_model_type(arena, m->return_type));
        json_object_object_add(method, "modifier", json_object_new_string(gen_model_func_mod_str(m->modifier)));
        json_object_object_add(method, "is_static", json_object_new_boolean(m->is_static));
        json_object_object_add(method, "is_native", json_object_new_boolean(m->is_native));
        json_object_object_add(method, "has_arena_param", json_object_new_boolean(m->has_arena_param));

        /* Flag compiler-generated serializable methods (encode/decode) so templates can skip forward decls */
        bool is_serial_method = (decl->is_serializable &&
            (strcmp(m->name, "encode") == 0 || strcmp(m->name, "decode") == 0 ||
             strcmp(m->name, "encodeArray") == 0 || strcmp(m->name, "decodeArray") == 0) &&
            m->is_native && m->body == NULL);
        json_object_object_add(method, "is_serializable_method", json_object_new_boolean(is_serial_method));

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
            gen_model_emit_param_cleanup(p, &m->params[j], m->is_native);
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

    /* Check for dispose() method with @alias — used by release function */
    bool has_dispose = false;
    const char *dispose_alias = NULL;
    for (int i = 0; i < decl->method_count; i++)
    {
        StructMethod *m = &decl->methods[i];
        if (strcmp(m->name, "dispose") == 0 && m->c_alias != NULL &&
            !m->is_static && m->param_count == 0)
        {
            has_dispose = true;
            dispose_alias = m->c_alias;
            break;
        }
    }
    json_object_object_add(obj, "has_dispose", json_object_new_boolean(has_dispose));
    if (dispose_alias)
        json_object_object_add(obj, "dispose_alias", json_object_new_string(dispose_alias));

    /* For val-type structs: detect user-defined copy() method to suppress auto-generated _copy */
    if (!decl->pass_self_by_ref)
    {
        bool has_user_copy = false;
        for (int i = 0; i < decl->method_count; i++)
        {
            StructMethod *m = &decl->methods[i];
            if (strcmp(m->name, "copy") == 0 && !m->is_static && !m->is_native)
            {
                has_user_copy = true;
                break;
            }
        }
        if (has_user_copy)
            json_object_object_add(obj, "has_user_copy_method", json_object_new_boolean(true));
    }

    return obj;
}
