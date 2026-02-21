#include "type_checker_util_layout.h"

size_t get_type_alignment(Type *type)
{
    if (type == NULL) return 1;

    switch (type->kind)
    {
        /* 1-byte alignment */
        case TYPE_BYTE:
        case TYPE_BOOL:
        case TYPE_CHAR:
            return 1;

        /* 4-byte alignment */
        case TYPE_INT32:
        case TYPE_UINT32:
        case TYPE_FLOAT:
            return 4;

        /* 8-byte alignment */
        case TYPE_INT:
        case TYPE_UINT:
        case TYPE_LONG:
        case TYPE_DOUBLE:
        case TYPE_POINTER:
        case TYPE_STRING:
        case TYPE_ARRAY:
        case TYPE_OPAQUE:
        case TYPE_FUNCTION:
            return 8;

        /* Struct types - return computed alignment */
        case TYPE_STRUCT:
            return type->as.struct_type.alignment;

        /* Any type needs 8-byte alignment */
        case TYPE_ANY:
            return 8;

        /* Void and nil have no alignment requirement */
        case TYPE_VOID:
        case TYPE_NIL:
            return 1;

        default:
            return 1;
    }
}

/* Helper: align a value to the next multiple of alignment */
static size_t align_to(size_t value, size_t alignment)
{
    if (alignment == 0) return value;
    return (value + alignment - 1) & ~(alignment - 1);
}

void calculate_struct_layout(Type *struct_type)
{
    if (struct_type == NULL || struct_type->kind != TYPE_STRUCT)
    {
        return;
    }

    bool is_packed = struct_type->as.struct_type.is_packed;
    size_t current_offset = 0;
    size_t max_alignment = 1;

    /* Account for hidden __arena__ pointer field in non-native, non-packed structs */
    if (!struct_type->as.struct_type.is_native && !is_packed)
    {
        size_t ptr_size = 8;
        size_t ptr_align = is_packed ? 1 : 8;
        current_offset = ptr_size;
        if (ptr_align > max_alignment)
        {
            max_alignment = ptr_align;
        }
    }

    /* Process each field */
    for (int i = 0; i < struct_type->as.struct_type.field_count; i++)
    {
        StructField *field = &struct_type->as.struct_type.fields[i];
        Type *field_type = field->type;

        /* Get field size and alignment */
        size_t field_size = get_type_size(field_type);
        size_t field_alignment = get_type_alignment(field_type);

        /* Ensure minimum alignment of 1 */
        if (field_alignment == 0) field_alignment = 1;

        /* For packed structs, use alignment of 1 (no padding) */
        if (is_packed)
        {
            field_alignment = 1;
        }

        /* Align current offset to field's alignment */
        current_offset = align_to(current_offset, field_alignment);

        /* Set field offset */
        field->offset = current_offset;

        /* Advance offset by field size */
        current_offset += field_size;

        /* Track maximum alignment */
        if (field_alignment > max_alignment)
        {
            max_alignment = field_alignment;
        }
    }

    /* For packed structs, no trailing padding needed - alignment is 1 */
    size_t total_size;
    if (is_packed)
    {
        total_size = current_offset;
        max_alignment = 1;
    }
    else
    {
        /* Add trailing padding to make struct size a multiple of its alignment */
        total_size = align_to(current_offset, max_alignment);
    }

    /* Store computed values */
    struct_type->as.struct_type.size = total_size;
    struct_type->as.struct_type.alignment = max_alignment;
}
