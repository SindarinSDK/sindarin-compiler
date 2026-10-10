#include "sn_abi.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    SnAbiValue *destination;
    SnAbiValue *source;
    unsigned calls;
    unsigned release_credit;
} Reentry;

static void cleanup(void *resource, uintptr_t token)
{
    assert(resource == NULL);
    Reentry *state = (Reentry *)token;
    uint64_t length = 99;
    assert(sn_abi_v1_value_array_length(state->destination, &length) == 0 && length == 4);
    SnAbiValue *element = NULL;
    SnAbiBytes bytes;
    assert(sn_abi_v1_value_array_get(state->destination, 0, &element) == 0);
    assert(sn_abi_v1_string_bytes(element, &bytes) == 0 && bytes.length == 3);
    assert(memcmp(bytes.data, "new", 3) == 0);
    sn_abi_v1_release(element);
    /* Nested replacement and growth must not invalidate outer cleanup slots. */
    assert(sn_abi_v1_value_array_assign(state->destination, state->source) == 0);
    for (unsigned i = 0; i < 32; i++)
        assert(sn_abi_v1_value_array_push(state->destination, NULL) == 0);
    state->calls++;
    if (state->release_credit) sn_abi_v1_release(state->destination);
}

int main(void)
{
    SnAbiInfo info;
    const uint32_t versions[] = {SN_ABI_V1_VERSION, SN_ABI_V1_1_VERSION, SN_ABI_V1_2_VERSION};
    const uint64_t masks[] = {7, 31, 63};
    for (unsigned i = 0; i < 3; i++) {
        assert(sn_abi_v1_query(versions[i], 0, &info, sizeof(info)) == 0);
        assert(info.abi_version == versions[i] && info.capabilities == masks[i]);
        SnAbiInfo previous = info;
        assert(sn_abi_v1_query(versions[i], SN_ABI_CAP_ARRAY_REPLACEMENT, &info, sizeof(info)) == SN_ABI_UNSUPPORTED);
        assert(memcmp(&previous, &info, sizeof(info)) == 0);
    }
    assert(sn_abi_v1_query(SN_ABI_V1_3_VERSION, SN_ABI_CAP_ARRAY_REPLACEMENT, &info, sizeof(info)) == 0);
    assert(info.abi_version == SN_ABI_V1_3_VERSION && info.capabilities == 127 && sizeof(info) == 32);
    SnAbiValue *source = NULL, *destination = NULL, *text = NULL, *empty = NULL, *raw = NULL;
    assert(sn_abi_v1_value_array_new(&source) == 0);
    assert(sn_abi_v1_string_copy("new", &text) == 0);
    assert(sn_abi_v1_string_copy("", &empty) == 0);
    const char octets[] = {(char)0x80, (char)0xff, 0};
    assert(sn_abi_v1_string_copy(octets, &raw) == 0);
    assert(sn_abi_v1_value_array_push(source, text) == 0);
    assert(sn_abi_v1_value_array_push(source, NULL) == 0);
    assert(sn_abi_v1_value_array_push(source, empty) == 0);
    assert(sn_abi_v1_value_array_push(source, raw) == 0);
    assert(sn_abi_v1_value_array_new(&destination) == 0);
    Reentry state = {destination, source, 0, 0};
    SnAbiValue *resource = NULL;
    assert(sn_abi_v1_resource_new(NULL, cleanup, (uintptr_t)&state, &resource) == 0);
    assert(sn_abi_v1_value_array_push(destination, resource) == 0);
    sn_abi_v1_release(resource);
    SnAbiValue *alias = sn_abi_v1_retain(destination);
    assert(sn_abi_v1_value_array_assign(NULL, source) == SN_ABI_INVALID_ARGUMENT);
    assert(sn_abi_v1_value_array_assign(text, source) == SN_ABI_WRONG_KIND);
    assert(sn_abi_v1_value_array_assign(destination, text) == SN_ABI_WRONG_KIND);
    assert(state.calls == 0);
    uint64_t length = 99;
    assert(sn_abi_v1_value_array_length(destination, &length) == 0 && length == 1);
    assert(sn_abi_v1_value_array_assign(destination, destination) == 0 && state.calls == 0);
    assert(sn_abi_v1_value_array_assign(destination, source) == 0 && state.calls == 1);
    assert(alias == destination && sn_abi_v1_value_array_length(alias, &length) == 0 && length == 36);
    SnAbiValue *element = NULL;
    assert(sn_abi_v1_value_array_get(alias, 0, &element) == 0 && element == text);
    sn_abi_v1_release(element);
    assert(sn_abi_v1_value_array_set(source, 0, NULL) == 0);
    assert(sn_abi_v1_value_array_get(alias, 0, &element) == 0 && element == text);
    sn_abi_v1_release(element);
    assert(sn_abi_v1_value_array_get(alias, 1, &element) == 0 && element == NULL);
    SnAbiBytes bytes;
    assert(sn_abi_v1_value_array_get(alias, 2, &element) == 0 && element == empty);
    assert(sn_abi_v1_string_bytes(element, &bytes) == 0 && bytes.data && bytes.length == 0);
    sn_abi_v1_release(element);
    assert(sn_abi_v1_value_array_get(alias, 3, &element) == 0 && element == raw);
    assert(sn_abi_v1_string_bytes(element, &bytes) == 0 && bytes.length == 2 && bytes.data[0] == 128 && bytes.data[1] == 255);
    sn_abi_v1_release(destination);
    assert(sn_abi_v1_value_array_assign(alias, NULL) == 0);
    assert(sn_abi_v1_value_array_length(alias, &length) == 0 && length == 0);
    sn_abi_v1_release(alias);
    /* A destructor may consume the external destination credit during cleanup.
     * The operation credit protects its subsequent nested mutation and teardown. */
    assert(sn_abi_v1_value_array_set(source, 0, text) == 0);
    assert(sn_abi_v1_value_array_new(&destination) == 0);
    state.destination = destination; state.release_credit = 1;
    assert(sn_abi_v1_resource_new(NULL, cleanup, (uintptr_t)&state, &resource) == 0);
    assert(sn_abi_v1_value_array_push(destination, resource) == 0);
    sn_abi_v1_release(resource);
    assert(sn_abi_v1_value_array_assign(destination, source) == 0 && state.calls == 2);
    /* Destination is now gone; an independently retained element stays alive. */
    assert(sn_abi_v1_string_bytes(element, &bytes) == 0 && bytes.length == 2);
    sn_abi_v1_release(element);
    sn_abi_v1_release(source); sn_abi_v1_release(text);
    sn_abi_v1_release(empty); sn_abi_v1_release(raw);
    puts("managed array replacement: pass");
    return 0;
}
