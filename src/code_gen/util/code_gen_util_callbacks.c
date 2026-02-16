/*
 * code_gen_util_callbacks.c - GC Callback Generation for Struct Types
 *
 * Generates copy/free callbacks for struct types with handle fields.
 * These callbacks make handles self-describing so rt_arena_v2_promote()
 * and GC sweep can deep-copy/free struct contents automatically.
 *
 * For a struct like:
 *   struct Person { name: str, tags: str[], nested: Inner }
 *
 * We generate:
 *   __copy_Person_inline__(dest, s)  - walks fields, promotes handles
 *   __free_Person_inline__(s)        - walks fields, frees handles
 *   __copy_Person__(dest, ptr)       - callback wrapper, casts ptr
 *   __free_Person__(handle)          - callback wrapper, casts handle->ptr
 *   __copy_array_Person__(dest, ptr) - iterates array elements
 *   __free_array_Person__(handle)    - iterates array elements
 */

#include "code_gen/util/code_gen_util.h"
#include <string.h>
#include <stdio.h>

/* Check if we already emitted callbacks for this struct type name */
static bool callbacks_already_emitted(CodeGen *gen, const char *struct_name) {
    for (int i = 0; i < gen->emitted_callback_count; i++) {
        if (strcmp(gen->emitted_callbacks[i], struct_name) == 0)
            return true;
    }
    return false;
}

/* Track that we've emitted callbacks for this struct type */
static void mark_callbacks_emitted(CodeGen *gen, const char *struct_name) {
    if (gen->emitted_callback_count >= gen->emitted_callback_capacity) {
        int new_cap = gen->emitted_callback_capacity == 0 ? 16 : gen->emitted_callback_capacity * 2;
        const char **new_arr = arena_alloc(gen->arena, new_cap * sizeof(const char *));
        if (gen->emitted_callbacks) {
            memcpy(new_arr, gen->emitted_callbacks, gen->emitted_callback_count * sizeof(const char *));
        }
        gen->emitted_callbacks = new_arr;
        gen->emitted_callback_capacity = new_cap;
    }
    gen->emitted_callbacks[gen->emitted_callback_count++] = arena_strdup(gen->arena, struct_name);
}

/* Get the C name for a struct type (c_alias or mangled name) */
static const char *get_struct_c_name(CodeGen *gen, Type *struct_type) {
    if (struct_type->as.struct_type.c_alias != NULL)
        return struct_type->as.struct_type.c_alias;
    if (struct_type->as.struct_type.name != NULL)
        return sn_mangle_name(gen->arena, struct_type->as.struct_type.name);
    return "__sn__unknown";
}

/* Get the Sindarin name for a struct (for callback function naming) */
static const char *get_struct_sn_name(CodeGen *gen, Type *struct_type) {
    if (struct_type->as.struct_type.name != NULL)
        return struct_type->as.struct_type.name;
    if (struct_type->as.struct_type.c_alias != NULL)
        return struct_type->as.struct_type.c_alias;
    return "unknown";
}

void code_gen_ensure_struct_callbacks(CodeGen *gen, Type *struct_type) {
    if (struct_type == NULL || struct_type->kind != TYPE_STRUCT) return;
    if (!struct_has_handle_fields(struct_type)) return;

    /* Resolve the type to get full field info */
    struct_type = resolve_struct_type(gen, struct_type);
    if (struct_type == NULL || struct_type->kind != TYPE_STRUCT) return;

    const char *sn_name = get_struct_sn_name(gen, struct_type);
    if (callbacks_already_emitted(gen, sn_name)) return;
    mark_callbacks_emitted(gen, sn_name);

    const char *c_name = get_struct_c_name(gen, struct_type);
    int field_count = struct_type->as.struct_type.field_count;

    /* First, recursively ensure callbacks for nested struct types */
    for (int i = 0; i < field_count; i++) {
        StructField *field = &struct_type->as.struct_type.fields[i];
        if (field->type == NULL) continue;
        if (field->type->kind == TYPE_STRUCT) {
            code_gen_ensure_struct_callbacks(gen, field->type);
        }
        /* Also check array element types for struct arrays */
        if (field->type->kind == TYPE_ARRAY) {
            Type *elem = field->type->as.array.element_type;
            if (elem && elem->kind == TYPE_STRUCT) {
                code_gen_ensure_struct_callbacks(gen, elem);
            }
        }
    }

    /* Generate inline copy function body */
    char *copy_body = arena_strdup(gen->arena, "");
    char *free_body = arena_strdup(gen->arena, "");

    for (int i = 0; i < field_count; i++) {
        StructField *field = &struct_type->as.struct_type.fields[i];
        if (field->type == NULL) continue;

        const char *f_c_name = field->c_alias != NULL
            ? field->c_alias
            : sn_mangle_name(gen->arena, field->name);
        TypeKind kind = field->type->kind;

        if (kind == TYPE_STRING || kind == TYPE_ARRAY) {
            copy_body = arena_sprintf(gen->arena,
                "%s    s->%s = rt_arena_v2_promote(dest, s->%s);\n",
                copy_body, f_c_name, f_c_name);
            free_body = arena_sprintf(gen->arena,
                "%s    if (s->%s) rt_arena_v2_free(s->%s);\n",
                free_body, f_c_name, f_c_name);
        }
        else if (kind == TYPE_ANY) {
            copy_body = arena_sprintf(gen->arena,
                "%s    rt_any_deep_copy(dest, &s->%s);\n",
                copy_body, f_c_name);
            free_body = arena_sprintf(gen->arena,
                "%s    rt_any_deep_free(&s->%s);\n",
                free_body, f_c_name);
        }
        else if (kind == TYPE_STRUCT && struct_has_handle_fields(field->type)) {
            Type *nested = resolve_struct_type(gen, field->type);
            const char *nested_sn = get_struct_sn_name(gen, nested);
            copy_body = arena_sprintf(gen->arena,
                "%s    __copy_%s_inline__(dest, &s->%s);\n",
                copy_body, nested_sn, f_c_name);
            free_body = arena_sprintf(gen->arena,
                "%s    __free_%s_inline__(&s->%s);\n",
                free_body, nested_sn, f_c_name);
        }
        else if (kind == TYPE_FUNCTION) {
            copy_body = arena_sprintf(gen->arena,
                "%s    s->%s = (__Closure__ *)rt_arena_v2_promote(dest, (RtHandleV2 *)s->%s);\n",
                copy_body, f_c_name, f_c_name);
            free_body = arena_sprintf(gen->arena,
                "%s    if (s->%s) rt_arena_v2_free((RtHandleV2 *)s->%s);\n",
                free_body, f_c_name, f_c_name);
        }
    }

    /* Forward declarations */
    gen->callback_forward_decls = arena_sprintf(gen->arena,
        "%s"
        "static void __copy_%s_inline__(RtArenaV2 *dest, %s *s);\n"
        "static void __free_%s_inline__(%s *s);\n"
        "static void __copy_%s__(RtArenaV2 *dest, void *ptr);\n"
        "static void __free_%s__(RtHandleV2 *handle);\n"
        "static void __copy_array_%s__(RtArenaV2 *dest, void *ptr);\n"
        "static void __free_array_%s__(RtHandleV2 *handle);\n",
        gen->callback_forward_decls,
        sn_name, c_name,
        sn_name, c_name,
        sn_name,
        sn_name,
        sn_name,
        sn_name);

    /* Function definitions */
    gen->callback_definitions = arena_sprintf(gen->arena,
        "%s"
        /* __copy_StructName_inline__ */
        "static void __copy_%s_inline__(RtArenaV2 *dest, %s *s) {\n"
        "%s"
        "}\n"
        /* __free_StructName_inline__ */
        "static void __free_%s_inline__(%s *s) {\n"
        "%s"
        "}\n"
        /* __copy_StructName__ - callback wrapper */
        "static void __copy_%s__(RtArenaV2 *dest, void *ptr) {\n"
        "    __copy_%s_inline__(dest, (%s *)ptr);\n"
        "}\n"
        /* __free_StructName__ - callback wrapper */
        "static void __free_%s__(RtHandleV2 *handle) {\n"
        "    __free_%s_inline__((%s *)handle->ptr);\n"
        "}\n"
        /* __copy_array_StructName__ - iterates array elements */
        "static void __copy_array_%s__(RtArenaV2 *dest, void *ptr) {\n"
        "    RtArrayMetadataV2 *meta = (RtArrayMetadataV2 *)ptr;\n"
        "    meta->arena = dest;\n"
        "    %s *arr = (%s *)((char *)ptr + sizeof(RtArrayMetadataV2));\n"
        "    for (size_t i = 0; i < meta->size; i++) {\n"
        "        __copy_%s_inline__(dest, &arr[i]);\n"
        "    }\n"
        "}\n"
        /* __free_array_StructName__ - iterates array elements */
        "static void __free_array_%s__(RtHandleV2 *handle) {\n"
        "    RtArrayMetadataV2 *meta = (RtArrayMetadataV2 *)handle->ptr;\n"
        "    %s *arr = (%s *)((char *)handle->ptr + sizeof(RtArrayMetadataV2));\n"
        "    for (size_t i = 0; i < meta->size; i++) {\n"
        "        __free_%s_inline__(&arr[i]);\n"
        "    }\n"
        "}\n",
        gen->callback_definitions,
        /* copy inline */
        sn_name, c_name, copy_body,
        /* free inline */
        sn_name, c_name, free_body,
        /* copy wrapper */
        sn_name, sn_name, c_name,
        /* free wrapper */
        sn_name, sn_name, c_name,
        /* copy array */
        sn_name, c_name, c_name, sn_name,
        /* free array */
        sn_name, c_name, c_name, sn_name);
}
