#include "gen_model/gen_model.h"

const char *gen_model_type_kind_str(TypeKind kind)
{
    switch (kind)
    {
        case TYPE_INT:    return "int";
        case TYPE_INT32:  return "int32";
        case TYPE_UINT:   return "uint";
        case TYPE_UINT32: return "uint32";
        case TYPE_LONG:   return "long";
        case TYPE_DOUBLE: return "double";
        case TYPE_FLOAT:  return "float";
        case TYPE_CHAR:   return "char";
        case TYPE_STRING: return "string";
        case TYPE_BOOL:   return "bool";
        case TYPE_BYTE:   return "byte";
        case TYPE_VOID:   return "void";
        case TYPE_NIL:    return "nil";
        case TYPE_ARRAY:  return "array";
        case TYPE_FUNCTION: return "function";
        case TYPE_POINTER:  return "pointer";
        case TYPE_OPAQUE:   return "opaque";
        case TYPE_STRUCT:   return "struct";
        default:          return "unknown";
    }
}

json_object *gen_model_type(Arena *arena, Type *type)
{
    (void)arena;

    if (!type) return json_object_new_null();

    json_object *obj = json_object_new_object();
    json_object_object_add(obj, "kind", json_object_new_string(gen_model_type_kind_str(type->kind)));

    switch (type->kind)
    {
        case TYPE_ARRAY:
            json_object_object_add(obj, "name", json_object_new_string("arr"));
            if (type->as.array.element_type)
            {
                json_object_object_add(obj, "element_type",
                    gen_model_type(arena, type->as.array.element_type));
            }
            break;

        case TYPE_POINTER:
            if (type->as.pointer.base_type)
            {
                json_object_object_add(obj, "base_type",
                    gen_model_type(arena, type->as.pointer.base_type));
            }
            break;

        case TYPE_OPAQUE:
            if (type->as.opaque.name)
            {
                json_object_object_add(obj, "name",
                    json_object_new_string(type->as.opaque.name));
            }
            break;

        case TYPE_FUNCTION:
        {
            json_object_object_add(obj, "return_type",
                gen_model_type(arena, type->as.function.return_type));

            json_object *params = json_object_new_array();
            for (int i = 0; i < type->as.function.param_count; i++)
            {
                json_object_array_add(params,
                    gen_model_type(arena, type->as.function.param_types[i]));
            }
            json_object_object_add(obj, "param_types", params);

            /* Parameter memory qualifiers (as_ref / as_val) */
            if (type->as.function.param_mem_quals)
            {
                json_object *quals = json_object_new_array();
                for (int i = 0; i < type->as.function.param_count; i++)
                {
                    const char *q = "default";
                    if (type->as.function.param_mem_quals[i] == MEM_AS_REF) q = "as_ref";
                    else if (type->as.function.param_mem_quals[i] == MEM_AS_VAL) q = "as_val";
                    json_object_array_add(quals, json_object_new_string(q));
                }
                json_object_object_add(obj, "param_mem_quals", quals);
            }

            json_object_object_add(obj, "is_variadic",
                json_object_new_boolean(type->as.function.is_variadic));
            json_object_object_add(obj, "is_native",
                json_object_new_boolean(type->as.function.is_native));
            json_object_object_add(obj, "has_arena_param",
                json_object_new_boolean(type->as.function.has_arena_param));
            break;
        }

        case TYPE_STRUCT:
        {
            json_object_object_add(obj, "name",
                json_object_new_string(type->as.struct_type.name));
            json_object_object_add(obj, "is_native",
                json_object_new_boolean(type->as.struct_type.is_native));
            json_object_object_add(obj, "is_packed",
                json_object_new_boolean(type->as.struct_type.is_packed));
            json_object_object_add(obj, "pass_self_by_ref",
                json_object_new_boolean(type->as.struct_type.pass_self_by_ref));

            if (type->as.struct_type.c_alias)
            {
                json_object_object_add(obj, "c_alias",
                    json_object_new_string(type->as.struct_type.c_alias));
            }

            json_object_object_add(obj, "size",
                json_object_new_int64((int64_t)type->as.struct_type.size));
            json_object_object_add(obj, "alignment",
                json_object_new_int64((int64_t)type->as.struct_type.alignment));

            /* Fields */
            json_object *fields = json_object_new_array();
            for (int i = 0; i < type->as.struct_type.field_count; i++)
            {
                StructField *f = &type->as.struct_type.fields[i];
                json_object *field = json_object_new_object();
                json_object_object_add(field, "name", json_object_new_string(f->name));
                json_object_object_add(field, "type", gen_model_type(arena, f->type));
                json_object_object_add(field, "offset", json_object_new_int64((int64_t)f->offset));
                if (f->c_alias)
                {
                    json_object_object_add(field, "c_alias", json_object_new_string(f->c_alias));
                }
                json_object_array_add(fields, field);
            }
            json_object_object_add(obj, "fields", fields);
            break;
        }

        case TYPE_CHAR:
            json_object_object_add(obj, "name", json_object_new_string("char"));
            break;

        default:
            /* Primitive types - kind is sufficient */
            break;
    }

    return obj;
}
