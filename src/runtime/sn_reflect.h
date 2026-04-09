#ifndef SN_REFLECT_H
#define SN_REFLECT_H

/*
 * Sindarin Reflection Runtime — TypeInfo and FieldInfo structs.
 *
 * These are the C representations of the built-in TypeInfo and FieldInfo
 * struct types used by the typeOf() intrinsic.
 */

#include "sn_array.h"

/* FieldInfo: metadata about a single struct field */
typedef struct {
    char *__sn__name;        /* Field name */
    char *__sn__typeName;    /* Type name string (e.g. "int", "str", "Person") */
    long long __sn__typeId;  /* FNV-1a hash of the type name */
} __sn__FieldInfo;

/* TypeInfo: metadata about a type */
typedef struct {
    char *__sn__name;            /* Type name (e.g. "Person", "int") */
    SnArray *__sn__fields;       /* Array of FieldInfo values */
    long long __sn__fieldCount;  /* Number of fields (0 for non-struct types) */
    long long __sn__typeId;      /* FNV-1a hash of the type name */
} __sn__TypeInfo;

/* ---- FieldInfo cleanup/copy (val-type struct support) ---- */

static inline void __sn__FieldInfo_cleanup(__sn__FieldInfo *p)
{
    (void)p; /* string fields are static literals, no cleanup needed */
}

static inline __sn__FieldInfo __sn__FieldInfo_copy(__sn__FieldInfo *src)
{
    return *src; /* shallow copy — string fields are static literals */
}

static inline void __sn__FieldInfo_release_helper(__sn__FieldInfo *p)
{
    __sn__FieldInfo_cleanup(p);
}

#define sn_auto_FieldInfo __attribute__((cleanup(__sn__FieldInfo_release_helper)))

/* ---- TypeInfo cleanup/copy (val-type struct support) ---- */

static inline void __sn__TypeInfo_cleanup(__sn__TypeInfo *p)
{
    if (p->__sn__fields)
    {
        sn_cleanup_array(&p->__sn__fields);
    }
}

static inline __sn__TypeInfo __sn__TypeInfo_copy(__sn__TypeInfo *src)
{
    __sn__TypeInfo copy;
    copy.__sn__name = src->__sn__name; /* static literal */
    copy.__sn__fields = src->__sn__fields ? sn_array_copy(src->__sn__fields) : NULL;
    copy.__sn__fieldCount = src->__sn__fieldCount;
    copy.__sn__typeId = src->__sn__typeId;
    return copy;
}

static inline void __sn__TypeInfo_release_helper(__sn__TypeInfo *p)
{
    __sn__TypeInfo_cleanup(p);
}

#define sn_auto_TypeInfo __attribute__((cleanup(__sn__TypeInfo_release_helper)))

/* ---- TypeInfo creation helper ---- */

/* Create a TypeInfo for a type with optional fields.
 * fields_data: pointer to a static array of __sn__FieldInfo (NULL for non-struct types)
 * field_count: number of fields (0 for non-struct types)
 * Returns a stack-allocated TypeInfo value. */
static inline __sn__TypeInfo sn_typeinfo_create(const char *name, long long type_id,
                                                 __sn__FieldInfo *fields_data, int field_count)
{
    __sn__TypeInfo info;
    info.__sn__name = (char *)name;
    info.__sn__typeId = type_id;
    info.__sn__fieldCount = field_count;
    if (field_count > 0 && fields_data)
    {
        info.__sn__fields = sn_array_new(sizeof(__sn__FieldInfo), field_count);
        for (int i = 0; i < field_count; i++)
        {
            sn_array_push_safe(info.__sn__fields, &fields_data[i], sizeof(__sn__FieldInfo));
        }
    }
    else
    {
        info.__sn__fields = NULL;
    }
    return info;
}

#endif
