#include "cgen/gen_model.h"
#include <string.h>

/* ============================================================================
 * Enum-to-string conversions (used by stmt, func, struct model builders)
 * ============================================================================ */

const char *gen_model_mem_qual_str(MemoryQualifier mq)
{
    switch (mq)
    {
        case MEM_DEFAULT: return "default";
        case MEM_AS_VAL:  return "as_val";
        case MEM_AS_REF:  return "as_ref";
        default:          return "default";
    }
}

const char *gen_model_sync_mod_str(SyncModifier sm)
{
    switch (sm)
    {
        case SYNC_NONE:   return "none";
        case SYNC_ATOMIC: return "atomic";
        default:          return "none";
    }
}

const char *gen_model_func_mod_str(FunctionModifier fm)
{
    switch (fm)
    {
        case FUNC_DEFAULT: return "default";
        default:           return "default";
    }
}

/* ============================================================================
 * Type category classification
 * ============================================================================ */

TypeCategory gen_model_type_category(Type *type)
{
    if (!type) return TYPE_CAT_SCALAR;
    switch (type->kind)
    {
        case TYPE_INT:
        case TYPE_INT32:
        case TYPE_UINT:
        case TYPE_UINT32:
        case TYPE_LONG:
        case TYPE_DOUBLE:
        case TYPE_FLOAT:
        case TYPE_BOOL:
        case TYPE_BYTE:
        case TYPE_CHAR:
        case TYPE_VOID:
        case TYPE_NIL:
        case TYPE_POINTER:
        case TYPE_OPAQUE:
            return TYPE_CAT_SCALAR;

        case TYPE_STRING:
        case TYPE_ARRAY:
        case TYPE_FUNCTION:
            return TYPE_CAT_OWNED;

        case TYPE_STRUCT:
            if (type->as.struct_type.pass_self_by_ref)
                return TYPE_CAT_REFCOUNTED;
            if (gen_model_type_has_heap_fields(type))
                return TYPE_CAT_COMPOSITE;
            return TYPE_CAT_INERT;

        case TYPE_GENERIC_INST:
            /* Before type-checking resolution, treat as scalar placeholder */
            if (type->as.generic_inst.resolved != NULL)
                return gen_model_type_category(type->as.generic_inst.resolved);
            return TYPE_CAT_SCALAR;

        default:
            return TYPE_CAT_SCALAR;
    }
}

const char *gen_model_type_category_str(TypeCategory cat)
{
    switch (cat)
    {
        case TYPE_CAT_SCALAR:     return "scalar";
        case TYPE_CAT_OWNED:      return "owned";
        case TYPE_CAT_REFCOUNTED: return "refcounted";
        case TYPE_CAT_COMPOSITE:  return "composite";
        case TYPE_CAT_INERT:      return "inert";
        default:                  return "scalar";
    }
}

/* ============================================================================
 * Type utilities (shared helpers for cleanup/heap analysis)
 * ============================================================================ */

static bool has_heap_fields_recursive(Type *type, Type **visited, int depth)
{
    if (!type || type->kind != TYPE_STRUCT || depth > 32) return false;
    for (int v = 0; v < depth; v++)
        if (visited[v] == type) return false;  /* cycle guard */
    visited[depth] = type;
    for (int i = 0; i < type->as.struct_type.field_count; i++)
    {
        Type *ft = type->as.struct_type.fields[i].type;
        if (!ft) continue;
        if (ft->kind == TYPE_STRING || ft->kind == TYPE_ARRAY ||
            ft->kind == TYPE_FUNCTION ||
            (ft->kind == TYPE_STRUCT && ft->as.struct_type.pass_self_by_ref))
            return true;
        /* Recurse into nested val-type structs */
        if (ft->kind == TYPE_STRUCT && !ft->as.struct_type.pass_self_by_ref)
            if (has_heap_fields_recursive(ft, visited, depth + 1))
                return true;
    }
    return false;
}

bool gen_model_type_has_heap_fields(Type *type)
{
    Type *visited[33];
    return has_heap_fields_recursive(type, visited, 0);
}

const char *gen_model_var_cleanup_kind(Type *type, bool suppress_local)
{
    if (!type) return "none";
    TypeCategory cat = gen_model_type_category(type);
    switch (cat)
    {
        case TYPE_CAT_OWNED:
            if (type->kind == TYPE_FUNCTION) return "fn";
            /* string/array can be suppressed for struct-returning functions */
            return suppress_local ? "none" : (type->kind == TYPE_STRING ? "str" : "arr");
        case TYPE_CAT_REFCOUNTED:
            return "release";
        case TYPE_CAT_COMPOSITE:
            return "val_cleanup";
        case TYPE_CAT_SCALAR:
        case TYPE_CAT_INERT:
        default:
            return "none";
    }
}

void gen_model_emit_param_cleanup(json_object *param_obj, Parameter *param, bool callee_is_native)
{
    if (!param->type || param->type->kind != TYPE_STRUCT) return;

    /* 'as ref' on an already-refcounted struct is redundant — refcounted
     * structs are always passed by pointer. The call-site in
     * gen_model_expr.c skips the '&' for ref-struct args regardless of
     * MEM_AS_REF, so the param declaration must also not add an extra
     * indirection. Normalize to 'default' here so templates agree. */
    if (param->mem_qualifier == MEM_AS_REF && param->type->as.struct_type.pass_self_by_ref)
    {
        json_object_object_del(param_obj, "mem_qual");
        json_object_object_add(param_obj, "mem_qual",
            json_object_new_string("default"));
    }

    /* Explicit 'as val' param on as ref struct: owns the copy, needs release.
     * MEM_DEFAULT on as ref structs passes by pointer (no copy), so only MEM_AS_VAL triggers this. */
    if (param->mem_qualifier == MEM_AS_VAL && param->type->as.struct_type.pass_self_by_ref)
    {
        json_object_object_add(param_obj, "param_cleanup", json_object_new_string("release"));
    }

    /* Explicit 'as val' param on val-type struct with heap fields: needs struct cleanup.
     * GCC cleanup attribute can't go on function params, so we rename the param
     * and generate a local with sn_auto_StructName at function body start.
     * MEM_DEFAULT borrows the struct (no cleanup needed), only MEM_AS_VAL takes ownership. */
    if (param->mem_qualifier == MEM_AS_VAL && !param->type->as.struct_type.pass_self_by_ref)
    {
        if (gen_model_type_has_heap_fields(param->type))
        {
            json_object_object_add(param_obj, "needs_struct_cleanup", json_object_new_boolean(true));
            json_object_object_add(param_obj, "struct_cleanup_name",
                json_object_new_string(param->type->as.struct_type.name));
        }
    }

    /* Composite val-type struct with MEM_DEFAULT on non-native callee: pass by pointer.
     * Treat as 'as ref' — callee receives pointer, works through it directly.
     * No copy, no cleanup in callee. Caller retains ownership. */
    if (param->mem_qualifier == MEM_DEFAULT && !callee_is_native &&
        !param->type->as.struct_type.pass_self_by_ref)
    {
        if (gen_model_type_has_heap_fields(param->type))
        {
            json_object_object_del(param_obj, "mem_qual");
            json_object_object_add(param_obj, "mem_qual",
                json_object_new_string("as_ref"));
        }
    }
}

/* ============================================================================
 * Type kind string conversion
 * ============================================================================ */

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
        case TYPE_OPAQUE:     return "opaque";
        case TYPE_STRUCT:     return "struct";
        case TYPE_INTERFACE:     return "interface";
        case TYPE_GENERIC_INST:  return "generic_inst";
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

        case TYPE_INTERFACE:
            if (type->as.interface_type.name)
            {
                json_object_object_add(obj, "name",
                    json_object_new_string(type->as.interface_type.name));
            }
            break;

        case TYPE_FUNCTION:
        {
            json_object_object_add(obj, "return_type",
                gen_model_type(arena, type->as.function.return_type));

            json_object *params = json_object_new_array();
            for (int i = 0; i < type->as.function.param_count; i++)
            {
                Type *pt = type->as.function.param_types[i];
                json_object *pt_obj = gen_model_type(arena, pt);
                /* Flag composite val-struct params on non-native functions as
                 * passed by pointer — mirrors the ABI rewrite in
                 * gen_model_emit_param_cleanup and gen_model_func.c's
                 * g_as_ref_param_names population. Closure-call function
                 * pointer casts must emit '*' here so the cast signature
                 * agrees with the lambda/function definition. */
                if (!type->as.function.is_native && pt &&
                    pt->kind == TYPE_STRUCT &&
                    !pt->as.struct_type.pass_self_by_ref &&
                    gen_model_type_has_heap_fields(pt))
                {
                    MemoryQualifier pmq = MEM_DEFAULT;
                    if (type->as.function.param_mem_quals)
                        pmq = type->as.function.param_mem_quals[i];
                    if (pmq == MEM_DEFAULT || pmq == MEM_AS_REF)
                    {
                        json_object_object_add(pt_obj, "pass_by_ptr",
                            json_object_new_boolean(true));
                    }
                }
                json_object_array_add(params, pt_obj);
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
                    /* Normalize: 'as ref' on a refcounted struct is redundant
                     * (already passed by pointer) — must match the param
                     * emitter in gen_model_emit_param_cleanup. */
                    if (type->as.function.param_mem_quals[i] == MEM_AS_REF)
                    {
                        Type *pt = type->as.function.param_types[i];
                        if (pt && pt->kind == TYPE_STRUCT &&
                            pt->as.struct_type.pass_self_by_ref)
                        {
                            q = "default";
                        }
                    }
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

            /* Check for dispose() method @alias — used by using blocks */
            {
                StructMethod *dispose_m = ast_struct_get_method(type, "dispose");
                if (dispose_m != NULL && dispose_m->c_alias != NULL &&
                    !dispose_m->is_static && dispose_m->param_count == 0)
                {
                    json_object_object_add(obj, "dispose_alias",
                        json_object_new_string(dispose_m->c_alias));
                }
            }
            break;
        }

        case TYPE_CHAR:
            json_object_object_add(obj, "name", json_object_new_string("char"));
            break;

        case TYPE_GENERIC_INST:
        {
            /* If the type checker has resolved this instantiation, emit the resolved type. */
            if (type->as.generic_inst.resolved != NULL)
            {
                /* Replace kind and delegate to resolved type's model */
                json_object_put(obj);
                return gen_model_type(arena, type->as.generic_inst.resolved);
            }
            /* Otherwise emit a placeholder (type checker will always resolve before codegen) */
            if (type->as.generic_inst.template_name)
            {
                json_object_object_add(obj, "template_name",
                    json_object_new_string(type->as.generic_inst.template_name));
            }
            json_object_object_add(obj, "type_arg_count",
                json_object_new_int(type->as.generic_inst.type_arg_count));
            break;
        }

        default:
            /* Primitive types - kind is sufficient */
            break;
    }

    return obj;
}
