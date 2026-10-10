#include "sn_abi.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static unsigned destroyed;
static void count_destroy(void *resource, uintptr_t context)
{
    assert(resource == NULL && context == 7);
    destroyed++;
}

static void length_is(SnAbiValue *array, uint64_t expected)
{
    uint64_t length = UINT64_MAX;
    assert(sn_abi_v1_value_array_length(array, &length) == 0 && length == expected);
}

static void slot_is(SnAbiValue *array, uint64_t index, SnAbiValue *expected)
{
    SnAbiValue *actual = NULL;
    assert(sn_abi_v1_value_array_get(array, index, &actual) == 0 && actual == expected);
    sn_abi_v1_release(actual);
}

typedef struct {
    SnAbiValue *array;
    SnAbiValue *marker;
    uint64_t published_length;
    unsigned calls;
    unsigned consume_caller_credit;
} Reentry;

static void reenter(void *resource, uintptr_t context)
{
    assert(resource == NULL);
    Reentry *state = (Reentry *)context;
    length_is(state->array, state->published_length);
    if (state->published_length) slot_is(state->array, 0, state->marker);
    if (state->consume_caller_credit) sn_abi_v1_release(state->array);
    /* Force relocation, then mutate again after the caller's credit is gone. */
    for (unsigned i = 0; i < 64; i++)
        assert(sn_abi_v1_value_array_insert(state->array, 0, NULL) == 0);
    assert(sn_abi_v1_value_array_reverse(state->array) == 0);
    SnAbiValue *popped = state->marker;
    assert(sn_abi_v1_value_array_pop(state->array, &popped) == 0 && popped == NULL);
    assert(sn_abi_v1_value_array_clear(state->array) == 0);
    assert(sn_abi_v1_value_array_insert(state->array, 0, state->marker) == 0);
    length_is(state->array, 1);
    state->calls++;
}

int main(void)
{
    SnAbiInfo info;
    const uint32_t versions[] = {SN_ABI_V1_VERSION, SN_ABI_V1_1_VERSION,
        SN_ABI_V1_2_VERSION, SN_ABI_V1_3_VERSION};
    const uint64_t masks[] = {7, 31, 63, 127};
    for (unsigned i = 0; i < 4; i++) {
        assert(sn_abi_v1_query(versions[i], 0, &info, sizeof(info)) == 0);
        assert(info.abi_version == versions[i] && info.capabilities == masks[i]);
        SnAbiInfo previous = info;
        assert(sn_abi_v1_query(versions[i], SN_ABI_CAP_ARRAY_MUTATION, &info, sizeof(info)) == SN_ABI_UNSUPPORTED);
        assert(memcmp(&previous, &info, sizeof(info)) == 0);
    }
    assert(sn_abi_v1_query(SN_ABI_V1_4_VERSION, SN_ABI_CAP_ARRAY_MUTATION, &info, sizeof(info)) == 0);
    assert(info.abi_version == SN_ABI_V1_4_VERSION && info.capabilities == 255 && sizeof(info) == 32);

    SnAbiValue *array = NULL, *marker = NULL, *raw = NULL;
    assert(sn_abi_v1_value_array_new(&array) == 0);
    assert(sn_abi_v1_string_copy("", &marker) == 0);
    const char octets[] = {(char)0x80, (char)0xff, 0};
    assert(sn_abi_v1_string_copy(octets, &raw) == 0);
    SnAbiValue *out = marker;
    assert(sn_abi_v1_value_array_take(array, 0, &out) == SN_ABI_OUT_OF_RANGE && out == marker);
    assert(sn_abi_v1_value_array_pop(array, &out) == SN_ABI_OUT_OF_RANGE && out == marker);
    assert(sn_abi_v1_value_array_pop(NULL, &out) == SN_ABI_OUT_OF_RANGE && out == marker);
    assert(sn_abi_v1_value_array_take(array, 0, NULL) == SN_ABI_INVALID_ARGUMENT);
    assert(sn_abi_v1_value_array_pop(array, NULL) == SN_ABI_INVALID_ARGUMENT);
    assert(sn_abi_v1_value_array_insert(NULL, 0, marker) == SN_ABI_INVALID_ARGUMENT);
    assert(sn_abi_v1_value_array_reverse(NULL) == SN_ABI_INVALID_ARGUMENT);
    assert(sn_abi_v1_value_array_clear(NULL) == SN_ABI_INVALID_ARGUMENT);
    assert(sn_abi_v1_value_array_remove(NULL, 0) == SN_ABI_OUT_OF_RANGE);
    assert(sn_abi_v1_value_array_insert(raw, 0, marker) == SN_ABI_WRONG_KIND);
    assert(sn_abi_v1_value_array_take(raw, 0, &out) == SN_ABI_WRONG_KIND && out == marker);
    assert(sn_abi_v1_value_array_pop(raw, &out) == SN_ABI_WRONG_KIND && out == marker);
    assert(sn_abi_v1_value_array_remove(raw, 0) == SN_ABI_WRONG_KIND);
    assert(sn_abi_v1_value_array_clear(raw) == SN_ABI_WRONG_KIND);
    assert(sn_abi_v1_value_array_reverse(raw) == SN_ABI_WRONG_KIND);
    assert(sn_abi_v1_value_array_insert(array, UINT64_MAX, marker) == SN_ABI_OUT_OF_RANGE);
    length_is(array, 0);
    assert(sn_abi_v1_value_array_reverse(array) == 0);
    assert(sn_abi_v1_value_array_insert(array, 0, raw) == 0);
    assert(sn_abi_v1_value_array_insert(array, 0, marker) == 0);
    assert(sn_abi_v1_value_array_insert(array, 1, NULL) == 0);
    SnAbiValue *alias = sn_abi_v1_retain(array);
    assert(sn_abi_v1_value_array_insert(array, 4, marker) == SN_ABI_OUT_OF_RANGE);
    assert(sn_abi_v1_value_array_take(array, UINT64_MAX, &out) == SN_ABI_OUT_OF_RANGE && out == marker);
    assert(sn_abi_v1_value_array_remove(array, 3) == SN_ABI_OUT_OF_RANGE);
    length_is(alias, 3);
    assert(sn_abi_v1_value_array_reverse(array) == 0);
    slot_is(alias, 0, raw); slot_is(alias, 1, NULL); slot_is(alias, 2, marker);
    assert(sn_abi_v1_value_array_take(array, 1, &out) == 0 && out == NULL);
    assert(sn_abi_v1_value_array_pop(array, &out) == 0 && out == marker);
    sn_abi_v1_release(out);
    assert(sn_abi_v1_value_array_take(alias, 0, &out) == 0 && out == raw);
    length_is(array, 0);
    sn_abi_v1_release(array); sn_abi_v1_release(alias);
    SnAbiBytes bytes;
    assert(sn_abi_v1_string_bytes(out, &bytes) == 0 && bytes.length == 2 && bytes.data[0] == 128 && bytes.data[1] == 255);
    sn_abi_v1_release(out);

    SnAbiValue *resource = NULL, *copy = NULL;
    assert(sn_abi_v1_value_array_new(&array) == 0);
    assert(sn_abi_v1_resource_new(NULL, count_destroy, 7, &resource) == 0);
    assert(sn_abi_v1_value_array_insert(array, 0, resource) == 0);
    sn_abi_v1_release(resource);
    assert(sn_abi_v1_value_array_copy(array, &copy) == 0);
    assert(sn_abi_v1_value_array_remove(array, 0) == 0 && destroyed == 0);
    assert(sn_abi_v1_value_array_pop(copy, &out) == 0 && out == resource && destroyed == 0);
    sn_abi_v1_release(copy); sn_abi_v1_release(array);
    assert(destroyed == 0);
    sn_abi_v1_release(out); assert(destroyed == 1);

    /* Removal and clearing publish before reentrant destruction. */
    for (unsigned clear = 0; clear < 2; clear++) {
        assert(sn_abi_v1_value_array_new(&array) == 0);
        Reentry state = {array, marker, clear ? 0 : 1, 0, 0};
        assert(sn_abi_v1_resource_new(NULL, reenter, (uintptr_t)&state, &resource) == 0);
        assert(sn_abi_v1_value_array_insert(array, 0, resource) == 0);
        assert(sn_abi_v1_value_array_insert(array, 1, marker) == 0);
        sn_abi_v1_release(resource);
        alias = sn_abi_v1_retain(array);
        assert((clear ? sn_abi_v1_value_array_clear(array) : sn_abi_v1_value_array_remove(array, 0)) == 0);
        assert(state.calls == 1);
        length_is(alias, 1); slot_is(alias, 0, marker);
        sn_abi_v1_release(array); sn_abi_v1_release(alias);
    }
    assert(sn_abi_v1_value_array_new(&array) == 0);
    Reentry state = {array, marker, 0, 0, 1};
    assert(sn_abi_v1_resource_new(NULL, reenter, (uintptr_t)&state, &resource) == 0);
    assert(sn_abi_v1_value_array_push(array, resource) == 0);
    sn_abi_v1_release(resource);
    assert(sn_abi_v1_value_array_remove(array, 0) == 0 && state.calls == 1);
    /* Its operation credit is now gone; marker's independent credit survives. */
    assert(sn_abi_v1_string_bytes(marker, &bytes) == 0 && bytes.data && bytes.length == 0);
    sn_abi_v1_release(marker); sn_abi_v1_release(raw);
    puts("managed array mutation: pass");
    return 0;
}
