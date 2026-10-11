#undef NDEBUG
#include "sn_abi.h"
#include "sn_array.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

typedef struct { int refs; long long value; } Record;
static int destroyed, copied_slots;
static SnAbiValue *consume_source;
static void release_record(void *slot)
{
    Record *record = *(Record **)slot;
    if (record && --record->refs == 0) { destroyed++; free(record); }
}
static void copy_record(const void *source, void *destination)
{
    Record *record = *(Record *const *)source;
    if (record) record->refs++;
    *(Record **)destination = record;
    copied_slots++;
    if (consume_source) {
        SnAbiValue *wire = consume_source;
        consume_source = NULL;
        sn_abi_v1_release(wire);
    }
}
static void release_child(void *slot) { sn_array_free(*(SnArray **)slot); }
static SnArray *leaf(void)
{
    SnArray *array = sn_array_new(sizeof(Record *), 3);
    array->elem_tag = SN_TAG_STRUCT;
    array->elem_copy = copy_record;
    array->elem_release = release_record;
    Record *record = calloc(1, sizeof(*record));
    record->refs = 2;
    record->value = 7;
    sn_array_push(array, &record);
    sn_array_push(array, &record);
    record = NULL;
    sn_array_push(array, &record);
    return array;
}
int main(void)
{
    const char *identity = "pkg.Record@1";
    SnAbiInfo info;
    assert(sn_abi_v1_query(SN_ABI_V1_8_VERSION, SN_ABI_CAP_NATIVE_RECORD_ARRAYS, &info, sizeof(info)) == 0);
    assert(sn_abi_v1_query(SN_ABI_V1_7_VERSION, SN_ABI_CAP_NATIVE_RECORD_ARRAYS, &info, sizeof(info)) == SN_ABI_UNSUPPORTED);
    SnAbiValue *wire = (void *)(uintptr_t)1;
    assert(sn_abi_v1_native_record_array_borrow(NULL, 32, identity, &wire) == 0 && wire == NULL);
    assert(sn_abi_v1_native_record_array_borrow(NULL, 0, identity, &wire) == SN_ABI_INVALID_ARGUMENT);
    assert(sn_abi_v1_native_record_array_borrow(NULL, 1, "", &wire) == SN_ABI_INVALID_ARGUMENT);
    SnArray *original = leaf(), *header = (void *)(uintptr_t)1;
    char identity_buffer[] = "pkg.Record@1";
    assert(sn_abi_v1_native_record_array_borrow(original, 1, identity_buffer, &wire) == 0);
    identity_buffer[0] = 'X';
    assert(sn_abi_v1_native_record_array_data(wire, 1, identity, &header) == 0 && header == original);
    header = (void *)(uintptr_t)1;
    assert(sn_abi_v1_native_record_array_data(wire, 1, "wrong", &header) == SN_ABI_WRONG_KIND && header == (void *)(uintptr_t)1);
    assert(sn_abi_v1_native_record_array_data(wire, 2, identity, &header) == SN_ABI_WRONG_KIND);
    assert(sn_abi_v1_native_array_data(wire, (SnAbiNativeArrayType){SN_ABI_ARRAY_INT64,1}, &header) == SN_ABI_WRONG_KIND);
    original->elem_size = 1;
    assert(sn_abi_v1_native_record_array_data(wire, 1, identity, &header) == SN_ABI_WRONG_KIND);
    original->elem_size = sizeof(Record *);
    original->elem_tag = SN_TAG_INT;
    assert(sn_abi_v1_native_record_array_data(wire, 1, identity, &header) == SN_ABI_WRONG_KIND);
    original->elem_tag = SN_TAG_STRUCT;
    original->len = -1;
    assert(sn_abi_v1_native_record_array_data(wire, 1, identity, &header) == SN_ABI_INVALID_ARGUMENT);
    original->len = 3;
    original->cap = INT64_MAX;
    assert(sn_abi_v1_native_record_array_data(wire, 1, identity, &header) == SN_ABI_OUT_OF_RANGE);
    original->cap = 3;
    assert(header == (void *)(uintptr_t)1);
    SnAbiValue *copy = NULL;
    assert(sn_abi_v1_native_record_array_copy(wire, &copy) == 0 && copied_slots == 3);
    assert(((Record **)original->data)[0]->refs == 4);
    SnAbiValue *alias = sn_abi_v1_retain(copy);
    assert(sn_abi_v1_native_record_array_take(&copy, 1, identity, &header) == SN_ABI_INVALID_ARGUMENT && copy == alias);
    sn_abi_v1_release(alias);
    assert(sn_abi_v1_native_record_array_take(&wire, 1, identity, &header) == SN_ABI_INVALID_ARGUMENT);
    sn_abi_v1_release(wire);
    sn_array_free(original);
    assert(destroyed == 0);
    assert(sn_abi_v1_native_record_array_take(&copy, 1, identity, &header) == 0 && copy == NULL);
    assert(header->elem_copy == copy_record && header->elem_release == release_record);
    assert(((Record **)header->data)[0] == ((Record **)header->data)[1]);
    assert(((Record **)header->data)[0]->value == 7);
    sn_array_free(header);
    assert(destroyed == 1);
    original = leaf();
    assert(sn_abi_v1_native_record_array_adopt(original, 1, identity, &wire) == 0);
    consume_source = wire;
    assert(sn_abi_v1_native_record_array_copy(wire, &copy) == 0 && consume_source == NULL);
    assert(destroyed == 1);
    sn_abi_v1_release(copy);
    assert(destroyed == 2);
    original = leaf();
    original->elem_copy = NULL;
    assert(sn_abi_v1_native_record_array_borrow(original, 1, identity, &wire) == 0);
    copy = (void *)(uintptr_t)1;
    assert(sn_abi_v1_native_record_array_copy(wire, &copy) == SN_ABI_UNSUPPORTED && copy == (void *)(uintptr_t)1);
    sn_abi_v1_release(wire);
    original->elem_release = NULL;
    wire = (void *)(uintptr_t)1;
    assert(sn_abi_v1_native_record_array_adopt(original, 1, identity, &wire) == SN_ABI_INVALID_ARGUMENT && wire == (void *)(uintptr_t)1);
    original->elem_release = release_record;
    sn_array_free(original);
    assert(destroyed == 3);
    original = sn_array_new(sizeof(SnArray *), 1);
    original->elem_tag = SN_TAG_ARRAY;
    SnArray *child = leaf();
    sn_array_push(original, &child);
    assert(sn_abi_v1_native_record_array_borrow(original, 2, identity, &wire) == 0);
    assert(sn_abi_v1_native_record_array_copy(wire, &copy) == 0);
    sn_abi_v1_release(wire);
    sn_array_free(original);
    sn_array_free(child);
    sn_abi_v1_release(copy);
    assert(destroyed == 4);
    wire = NULL;
    assert(sn_abi_v1_native_record_array_take(&wire, 1, identity, &header) == 0 && !wire && !header);
    for (uint32_t rank = 1; rank <= 32; rank++) {
        original = leaf();
        for (uint32_t layer = 1; layer < rank; layer++) {
            SnArray *parent = sn_array_new(sizeof(SnArray *), 2);
            parent->elem_tag = SN_TAG_ARRAY;
            parent->elem_release = release_child;
            sn_array_push(parent, &original);
            SnArray *nil_child = NULL;
            sn_array_push(parent, &nil_child);
            original = parent;
        }
        assert(sn_abi_v1_native_record_array_adopt(original, rank, identity, &wire) == 0);
        assert(sn_abi_v1_native_record_array_copy(wire, &copy) == 0);
        sn_abi_v1_release(wire);
        assert(sn_abi_v1_native_record_array_data(copy, rank, identity, &header) == 0 && header);
        sn_abi_v1_release(copy);
        assert(destroyed == (int)rank + 4);
    }
    puts("native record array views: pass");
}
