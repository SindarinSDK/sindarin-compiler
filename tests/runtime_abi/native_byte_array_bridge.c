#include "native_byte_array_bridge.h"
#include "sn_array.h"

static uint64_t destroyed;
static SnAbiValue *copy_consumes;
static void release_slot(void *slot) { (void)slot; destroyed++; }
static void copy_slot(const void *source, void *destination)
{
    if (copy_consumes) {
        SnAbiValue *value = copy_consumes;
        copy_consumes = NULL;
        sn_abi_v1_release(value);
    }
    *(uint8_t *)destination = *(const uint8_t *)source;
}
void sn_test_native_bytes_copy_consume(SnAbiValue *value) { copy_consumes = value; }
SnArray *sn_test_native_bytes_new(void)
{
    SnArray *array = sn_array_new(sizeof(uint8_t), 0);
    array->elem_tag = SN_TAG_BYTE;
    array->elem_release = release_slot;
    array->elem_copy = copy_slot;
    const uint8_t bytes[] = {0, 127, 128, 255};
    for (size_t i = 0; i < sizeof(bytes); i++) sn_array_push(array, &bytes[i]);
    return array;
}
void sn_test_native_bytes_free(SnArray *array) { sn_array_free(array); }
uint64_t sn_test_native_bytes_destroyed(void) { return destroyed; }
SnAbiStatus sn_test_native_bytes_push(SnAbiValue *view, uint8_t byte)
{
    SnArray *array = NULL;
    SnAbiStatus status = sn_abi_v1_native_byte_array_data(view, &array);
    if (status) return status;
    if (!array) return SN_ABI_INVALID_ARGUMENT;
    sn_array_push(array, &byte);
    return SN_ABI_OK;
}
SnAbiStatus sn_test_native_bytes_reenter(SnAbiValue *view,
    SnAbiStatus (*observe)(SnAbiValue *, uintptr_t), uintptr_t context)
{
    if (!observe) return SN_ABI_INVALID_ARGUMENT;
    SnAbiValue *guard = sn_abi_v1_retain(view);
    SnAbiStatus status = SN_ABI_OK;
    for (int i = 0; !status && i < 64; i++) {
        status = sn_test_native_bytes_push(guard, 255);
        if (!status) status = observe(guard, context);
    }
    sn_abi_v1_release(guard);
    return status;
}
