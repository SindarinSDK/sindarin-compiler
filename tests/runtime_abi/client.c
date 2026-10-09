#include "sn_abi.h"
#include <assert.h>
#include <limits.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned destroyed;
static void destroy_resource(void *resource, uintptr_t context)
{
    assert(context == 91);
    assert(*(int *)resource == 42);
    free(resource);
    destroyed++;
    /* Cleanup callbacks may call the runtime again. */
    SnAbiValue *nested = NULL;
    assert(sn_abi_v1_string_copy("reentrant", &nested) == SN_ABI_OK);
    sn_abi_v1_release(nested);
}

static void *acquire_credits(void *value)
{
    for (unsigned i = 0; i < 10000; i++)
        sn_abi_v1_release(sn_abi_v1_retain(value));
    return NULL;
}

static void *drop_credit(void *value)
{
    sn_abi_v1_release(value);
    return NULL;
}

int main(void)
{
    SnAbiInfo info = {0};
    assert(sizeof(info) == 32);
    assert(sn_abi_v1_query(SN_ABI_V1_VERSION, 7, &info, sizeof(info)) == SN_ABI_OK);
    assert(info.abi_version == SN_ABI_V1_VERSION && info.capabilities == 7);
    assert(info.pointer_bits == sizeof(void *) * CHAR_BIT);
    assert(info.int_bits == 64 && info.char_bits == 8);
    assert(info.float_bits == 32 && info.double_bits == 64);
    SnAbiInfo preserved = info;
    assert(sn_abi_v1_query(0, 0, &info, sizeof(info)) == SN_ABI_VERSION_MISMATCH);
    assert(memcmp(&info, &preserved, sizeof(info)) == 0);
    assert(sn_abi_v1_query(SN_ABI_V1_VERSION, UINT64_MAX, &info, sizeof(info)) == SN_ABI_UNSUPPORTED);
    assert(sn_abi_v1_query(SN_ABI_V1_VERSION, 0, &info, sizeof(info) - 1) == SN_ABI_INVALID_ARGUMENT);
    assert(sn_abi_v1_query(SN_ABI_V1_VERSION, 0, NULL, sizeof(info)) == SN_ABI_INVALID_ARGUMENT);
    assert(strcmp(sn_abi_v1_status_message(SN_ABI_WRONG_KIND), "incorrect ABI value kind") == 0);
    assert(strcmp(sn_abi_v1_status_message(99), "unknown runtime ABI status") == 0);
    assert(sn_abi_v1_retain(NULL) == NULL);
    sn_abi_v1_release(NULL);

    SnAbiValue *nil = NULL, *empty = NULL, *text = NULL, *joined = NULL;
    assert(sn_abi_v1_string_copy(NULL, &nil) == SN_ABI_OK && nil == NULL);
    assert(sn_abi_v1_string_copy("", &empty) == SN_ABI_OK && empty != NULL);
    const char c_text[] = {'h', (char)0xff, 0, 'z', 0};
    assert(sn_abi_v1_string_copy(c_text, &text) == SN_ABI_OK);
    SnAbiBytes bytes;
    assert(sn_abi_v1_bytes(nil, &bytes) == SN_ABI_OK && bytes.data == NULL && bytes.length == 0);
    assert(sn_abi_v1_bytes(empty, &bytes) == SN_ABI_OK && bytes.data != NULL && bytes.length == 0);
    SnAbiValue *alias = sn_abi_v1_retain(text);
    assert(alias == text);
    sn_abi_v1_release(text);
    assert(sn_abi_v1_bytes(alias, &bytes) == SN_ABI_OK);
    assert(bytes.length == 2 && bytes.data[0] == 'h' && bytes.data[1] == 255 && bytes.data[2] == 0);
    assert(sn_abi_v1_string_concat(alias, empty, &joined) == SN_ABI_OK);
    assert(joined != alias);
    assert(sn_abi_v1_bytes(joined, &bytes) == SN_ABI_OK && bytes.length == 2);
    sn_abi_v1_release(joined);
    assert(sn_abi_v1_string_concat(NULL, NULL, &joined) == SN_ABI_OK && joined != NULL);
    sn_abi_v1_release(joined);
    sn_abi_v1_release(empty);
    sn_abi_v1_release(alias);

    const uint8_t binary[] = {0, 128, 255, 0};
    SnAbiValue *buffer = NULL;
    assert(sn_abi_v1_buffer_copy(binary, sizeof(binary), &buffer) == SN_ABI_OK);
    assert(sn_abi_v1_bytes(buffer, &bytes) == SN_ABI_OK && bytes.length == sizeof(binary));
    assert(memcmp(bytes.data, binary, sizeof(binary)) == 0 && bytes.data != binary);
    SnAbiValue *output = buffer;
    assert(sn_abi_v1_buffer_copy(NULL, 1, &output) == SN_ABI_INVALID_ARGUMENT && output == buffer);
    assert(sn_abi_v1_string_concat(buffer, NULL, &output) == SN_ABI_WRONG_KIND && output == buffer);
    assert(sn_abi_v1_buffer_copy(NULL, 0, &nil) == SN_ABI_OK && nil == NULL);
    assert(sn_abi_v1_buffer_copy(binary, 0, &empty) == SN_ABI_OK && empty != NULL);
    assert(sn_abi_v1_bytes(empty, &bytes) == SN_ABI_OK && bytes.data != NULL && bytes.length == 0);
    sn_abi_v1_release(empty);

    SnAbiValue *array = NULL, *copy = NULL;
    assert(sn_abi_v1_array_new(sizeof(int64_t), &array) == SN_ABI_OK);
    uint64_t size = 0, length = 99;
    assert(sn_abi_v1_array_element_size(array, &size) == SN_ABI_OK && size == sizeof(int64_t));
    assert(sn_abi_v1_array_length(NULL, &length) == SN_ABI_OK && length == 0);
    for (int64_t i = 0; i < 128; i++)
        assert(sn_abi_v1_array_push(array, &i, sizeof(i)) == SN_ABI_OK);
    assert(sn_abi_v1_array_length(array, &length) == SN_ABI_OK && length == 128);
    alias = sn_abi_v1_retain(array);
    assert(sn_abi_v1_array_copy(array, &copy) == SN_ABI_OK && copy != array);
    sn_abi_v1_release(array);
    int64_t value = 42, original = -1;
    assert(sn_abi_v1_array_set(alias, 0, &value, sizeof(value)) == SN_ABI_OK);
    assert(sn_abi_v1_array_get(alias, 0, &original, sizeof(original)) == SN_ABI_OK && original == 42);
    assert(sn_abi_v1_array_get(copy, 0, &original, sizeof(original)) == SN_ABI_OK && original == 0);
    original = 99;
    assert(sn_abi_v1_array_get(alias, UINT64_MAX, &original, sizeof(original)) == SN_ABI_OUT_OF_RANGE);
    assert(original == 99);
    assert(sn_abi_v1_array_set(alias, 128, &value, sizeof(value)) == SN_ABI_OUT_OF_RANGE);
    assert(sn_abi_v1_array_push(alias, &value, 1) == SN_ABI_INVALID_ARGUMENT);
    assert(sn_abi_v1_array_get(buffer, 0, &original, sizeof(original)) == SN_ABI_WRONG_KIND);
    SnAbiBytes retained_bytes = {binary, sizeof(binary)};
    assert(sn_abi_v1_bytes(alias, &retained_bytes) == SN_ABI_WRONG_KIND);
    assert(retained_bytes.data == binary && retained_bytes.length == sizeof(binary));
    output = buffer;
    assert(sn_abi_v1_array_new(0, &output) == SN_ABI_INVALID_ARGUMENT && output == buffer);
    assert(sn_abi_v1_array_new(UINT64_MAX, &output) == SN_ABI_OUT_OF_RANGE && output == buffer);
    assert(sn_abi_v1_array_copy(NULL, &nil) == SN_ABI_OK && nil == NULL);
    sn_abi_v1_release(alias);
    sn_abi_v1_release(copy);
    sn_abi_v1_release(buffer);

    int *payload = malloc(sizeof(*payload));
    assert(payload);
    *payload = 42;
    SnAbiValue *resource = NULL;
    assert(sn_abi_v1_resource_new(payload, destroy_resource, 91, NULL) == SN_ABI_INVALID_ARGUMENT);
    assert(destroyed == 0 && *payload == 42);
    assert(sn_abi_v1_resource_new(payload, destroy_resource, 91, &resource) == SN_ABI_OK);
    void *borrowed = NULL;
    assert(sn_abi_v1_resource_data(resource, &borrowed) == SN_ABI_OK && borrowed == payload);
    pthread_t threads[8];
    for (unsigned i = 0; i < 8; i++)
        assert(pthread_create(&threads[i], NULL, acquire_credits, resource) == 0);
    for (unsigned i = 0; i < 8; i++) assert(pthread_join(threads[i], NULL) == 0);
    assert(destroyed == 0);
    alias = sn_abi_v1_retain(resource);
    sn_abi_v1_release(resource);
    assert(destroyed == 0);
    sn_abi_v1_release(alias);
    assert(destroyed == 1);
    payload = malloc(sizeof(*payload));
    assert(payload);
    *payload = 42;
    assert(sn_abi_v1_resource_new(payload, destroy_resource, 91, &resource) == SN_ABI_OK);
    assert(pthread_create(&threads[0], NULL, drop_credit, resource) == 0);
    assert(pthread_join(threads[0], NULL) == 0);
    assert(destroyed == 2);
    assert(sn_abi_v1_resource_data(NULL, &borrowed) == SN_ABI_OK && borrowed == NULL);
    puts("shared runtime ABI: pass");
    return 0;
}
