#include "native_record_array_bridge.h"
#include "sn_array.h"
typedef struct { int refs; int64_t value; } Record;
static uint64_t destroyed;
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
}
SnArray *sn_test_record_array_new(void)
{
    SnArray *array = sn_array_new(sizeof(Record *), 2);
    array->elem_tag = SN_TAG_STRUCT;
    array->elem_release = release_record;
    array->elem_copy = copy_record;
    Record *record = calloc(1, sizeof(*record));
    record->refs = 1;
    record->value = 7;
    sn_array_push(array, &record);
    record = NULL;
    sn_array_push(array, &record);
    return array;
}
void sn_test_record_array_free(SnArray *array) { sn_array_free(array); }
uint64_t sn_test_record_array_destroyed(void) { return destroyed; }
int64_t sn_test_record_array_value(SnAbiValue *value)
{
    SnArray *array = NULL;
    if (sn_abi_v1_native_record_array_data(value, 1, "pkg.Record@1", &array) || !array) return -1;
    return ((Record **)array->data)[0]->value;
}
SnAbiStatus sn_test_record_array_reenter(SnAbiValue *value,
    SnAbiStatus (*callback)(SnAbiValue *, uintptr_t), uintptr_t context)
{
    if (!value || !callback) return SN_ABI_INVALID_ARGUMENT;
    SnAbiValue *guard = sn_abi_v1_retain(value);
    SnAbiStatus status = SN_ABI_OK;
    for (int i = 0; i < 64 && !status; i++) {
        SnArray *array = NULL;
        status = sn_abi_v1_native_record_array_data(value, 1, "pkg.Record@1", &array);
        if (status) break;
        ((Record **)array->data)[0]->value++;
        status = callback(value, context);
    }
    sn_abi_v1_release(guard);
    return status;
}
