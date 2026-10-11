#include "native_string_array_bridge.h"
#include "sn_array.h"

static uint64_t destroyed;
static SnAbiValue *copy_consumes;
static void release_slot(void *slot) { free(*(char **)slot); destroyed++; }
static void copy_slot(const void *source, void *destination)
{
    if (copy_consumes) {
        SnAbiValue *value = copy_consumes;
        copy_consumes = NULL;
        sn_abi_v1_release(value);
    }
    const char *text = *(char *const *)source;
    *(char **)destination = text ? strdup(text) : NULL;
    if (text && !*(char **)destination) abort();
}
void sn_test_native_strings_copy_consume(SnAbiValue *value) { copy_consumes = value; }
SnArray *sn_test_native_strings_new(void)
{
    SnArray *array = sn_array_new(sizeof(char *), 0);
    array->elem_tag = SN_TAG_STRING;
    array->elem_release = release_slot;
    array->elem_copy = copy_slot;
    const char *texts[] = {"one", NULL, "", "\200\377"};
    for (size_t i = 0; i < 4; i++) {
        char *text = texts[i] ? strdup(texts[i]) : NULL;
        sn_array_push(array, &text);
    }
    return array;
}
void sn_test_native_strings_free(SnArray *array) { sn_array_free(array); }
uint64_t sn_test_native_strings_destroyed(void) { return destroyed; }
SnAbiStatus sn_test_native_strings_length(SnAbiValue *view, uint64_t *out)
{
    if (!out) return SN_ABI_INVALID_ARGUMENT;
    SnArray *array = NULL;
    SnAbiStatus status = sn_abi_v1_native_string_array_data(view, &array);
    if (status) return status;
    *out = array ? (uint64_t)array->len : 0;
    return SN_ABI_OK;
}
SnAbiStatus sn_test_native_strings_read(SnAbiValue *view, uint64_t index, SnAbiValue **out)
{
    if (!out) return SN_ABI_INVALID_ARGUMENT;
    SnArray *array = NULL;
    SnAbiStatus status = sn_abi_v1_native_string_array_data(view, &array);
    if (status) return status;
    if (!array || index >= (uint64_t)array->len) return SN_ABI_OUT_OF_RANGE;
    return sn_abi_v1_string_copy(((char **)array->data)[index], out);
}
SnAbiStatus sn_test_native_strings_push(SnAbiValue *view, const char *text)
{
    SnArray *array = NULL;
    SnAbiStatus status = sn_abi_v1_native_string_array_data(view, &array);
    if (status) return status;
    if (!array) return SN_ABI_INVALID_ARGUMENT;
    char *copy = text ? strdup(text) : NULL;
    sn_array_push(array, &copy);
    return SN_ABI_OK;
}
SnAbiStatus sn_test_native_strings_reenter(SnAbiValue *view, SnTestArrayObserve observe, uintptr_t context)
{
    if (!observe) return SN_ABI_INVALID_ARGUMENT;
    SnAbiValue *guard = sn_abi_v1_retain(view);
    SnAbiStatus status = SN_ABI_OK;
    for (int i = 0; !status && i < 64; i++) {
        status = sn_test_native_strings_push(guard, "outer");
        if (!status) status = observe(guard, context);
    }
    sn_abi_v1_release(guard);
    return status;
}
