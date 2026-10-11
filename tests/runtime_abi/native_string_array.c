#include <assert.h>
#include <string.h>
#include "sn_array.h"
#include "native_string_array_bridge.h"

static SnAbiValue *alias;
static SnArray *original;
static uint64_t callbacks;
static SnAbiStatus observe(SnAbiValue *view, uintptr_t context)
{
    assert(context == 42);
    SnArray *header = NULL;
    assert(sn_abi_v1_native_string_array_data(alias, &header) == 0 && header == original);
    assert(header->len == 5 + (long long)(callbacks * 2));
    assert(sn_test_native_strings_push(view, "callback") == 0);
    callbacks++;
    return 0;
}
static SnAbiStatus consume_credit(SnAbiValue *view, uintptr_t context)
{
    (void)context;
    if (!callbacks++) sn_abi_v1_release(view);
    return sn_test_native_strings_push(view, "callback");
}
int main(void)
{
    SnAbiInfo info;
    const uint64_t masks[] = {7, 31, 63, 127, 255};
    for (uint32_t i = 0; i < 5; i++) {
        assert(sn_abi_v1_query(SN_ABI_V1_VERSION + i, 0, &info, sizeof(info)) == 0);
        assert(info.capabilities == masks[i]);
        SnAbiInfo saved = info;
        assert(sn_abi_v1_query(SN_ABI_V1_VERSION + i, SN_ABI_CAP_NATIVE_STRING_ARRAYS, &info, sizeof(info)) == SN_ABI_UNSUPPORTED);
        assert(memcmp(&info, &saved, sizeof(info)) == 0);
    }
    assert(sn_abi_v1_query(SN_ABI_V1_5_VERSION, SN_ABI_CAP_NATIVE_STRING_ARRAYS, &info, sizeof(info)) == 0 && info.capabilities == 511);
    SnAbiInfo saved = info;
    assert(sn_abi_v1_query(SN_ABI_V1_7_VERSION + 1, 0, &info, sizeof(info)) == SN_ABI_VERSION_MISMATCH);
    assert(memcmp(&info, &saved, sizeof(info)) == 0);
    SnAbiValue *view = NULL, *copy = NULL, *wrong = NULL, *out = (SnAbiValue *)(uintptr_t)1;
    assert(sn_abi_v1_native_string_array_borrow(NULL, &out) == 0 && !out);
    assert(sn_abi_v1_native_string_array_adopt(NULL, &out) == 0 && !out);
    assert(sn_abi_v1_native_string_array_copy(NULL, &out) == 0 && !out);
    SnArray *header = (SnArray *)(uintptr_t)1;
    assert(sn_abi_v1_native_string_array_data(NULL, &header) == 0 && !header);
    assert(sn_abi_v1_native_string_array_data(NULL, NULL) == SN_ABI_INVALID_ARGUMENT);
    assert(sn_abi_v1_native_string_array_copy(NULL, NULL) == SN_ABI_INVALID_ARGUMENT);
    original = sn_test_native_strings_new();
    assert(sn_abi_v1_native_string_array_adopt(original, NULL) == SN_ABI_INVALID_ARGUMENT);
    assert(sn_abi_v1_native_string_array_borrow(original, NULL) == SN_ABI_INVALID_ARGUMENT);
    size_t width = original->elem_size;
    original->elem_size = 1;
    out = (SnAbiValue *)(uintptr_t)1;
    assert(sn_abi_v1_native_string_array_adopt(original, &out) == SN_ABI_WRONG_KIND && out == (SnAbiValue *)(uintptr_t)1);
    original->elem_size = width;
    original->elem_tag = SN_TAG_INT;
    assert(sn_abi_v1_native_string_array_borrow(original, &out) == SN_ABI_WRONG_KIND && out == (SnAbiValue *)(uintptr_t)1);
    original->elem_tag = SN_TAG_STRING;
    long long capacity = original->cap;
    original->cap = 1;
    assert(sn_abi_v1_native_string_array_adopt(original, &out) == SN_ABI_INVALID_ARGUMENT && out == (SnAbiValue *)(uintptr_t)1);
    original->cap = capacity;
    original->cap = LLONG_MAX;
    assert(sn_abi_v1_native_string_array_adopt(original, &out) == SN_ABI_OUT_OF_RANGE && out == (SnAbiValue *)(uintptr_t)1);
    original->cap = capacity;
    original->len = -1;
    assert(sn_abi_v1_native_string_array_borrow(original, &out) == SN_ABI_INVALID_ARGUMENT && out == (SnAbiValue *)(uintptr_t)1);
    original->len = 4;
    void *data = original->data;
    original->data = NULL;
    assert(sn_abi_v1_native_string_array_borrow(original, &out) == SN_ABI_INVALID_ARGUMENT && out == (SnAbiValue *)(uintptr_t)1);
    original->data = data;
    assert(sn_abi_v1_native_string_array_borrow(original, &view) == 0);
    alias = sn_abi_v1_retain(view);
    SnAbiValue *second = NULL;
    assert(sn_abi_v1_native_string_array_borrow(original, &second) == 0 && second != view);
    assert(sn_abi_v1_native_string_array_data(second, &header) == 0 && header == original);
    sn_abi_v1_release(second);
    assert(sn_abi_v1_native_string_array_copy(view, &copy) == 0);
    assert(sn_abi_v1_native_string_array_data(copy, &header) == 0 && header != original);
    assert(header->elem_release == original->elem_release && header->elem_copy == original->elem_copy);
    assert(!((char **)header->data)[1] && ((char **)header->data)[2] && !*((char **)header->data)[2]);
    assert(memcmp(((char **)header->data)[3], "\200\377", 3) == 0);
    assert(((char **)header->data)[0] != ((char **)original->data)[0]);
    assert(sn_test_native_strings_reenter(view, observe, 42) == 0 && callbacks == 64);
    assert(original->len == 132 && header->len == 4);
    assert(sn_abi_v1_value_array_length(view, &callbacks) == SN_ABI_WRONG_KIND && callbacks == 64);
    assert(sn_abi_v1_string_copy("wrong kind", &wrong) == 0);
    SnArray *sentinel = header;
    assert(sn_abi_v1_native_string_array_data(wrong, &header) == SN_ABI_WRONG_KIND && header == sentinel);
    assert(sn_abi_v1_native_string_array_copy(wrong, &out) == SN_ABI_WRONG_KIND && out == (SnAbiValue *)(uintptr_t)1);
    sn_abi_v1_release(wrong);
    sn_abi_v1_release(view);
    sn_abi_v1_release(alias);
    assert(sn_test_native_strings_destroyed() == 0 && original->len == 132);
    sn_test_native_strings_free(original);
    assert(sn_test_native_strings_destroyed() == 132);
    sn_abi_v1_release(copy);
    assert(sn_test_native_strings_destroyed() == 136);
    callbacks = 0;
    original = sn_test_native_strings_new();
    assert(sn_abi_v1_native_string_array_adopt(original, &view) == 0);
    assert(sn_test_native_strings_reenter(view, consume_credit, 0) == 0 && callbacks == 64);
    assert(sn_test_native_strings_destroyed() == 268);
    assert(sn_abi_v1_native_string_array_adopt(sn_test_native_strings_new(), &view) == 0);
    sn_test_native_strings_copy_consume(view);
    assert(sn_abi_v1_native_string_array_copy(view, &copy) == 0);
    assert(sn_test_native_strings_destroyed() == 272);
    assert(sn_abi_v1_native_string_array_data(copy, &header) == 0 && header->len == 4);
    assert(!strcmp(((char **)header->data)[0], "one") && !((char **)header->data)[1]);
    sn_abi_v1_release(copy);
    assert(sn_test_native_strings_destroyed() == 276);
    /* Empty untyped native storage can acquire a typed view without rewriting
     * the legacy header. Its element width is still explicitly validated. */
    original = sn_array_new(sizeof(char *), 0);
    assert(sn_abi_v1_native_string_array_adopt(original, &view) == 0);
    assert(sn_abi_v1_native_string_array_data(view, &header) == 0 && header == original && header->elem_tag == SN_TAG_DEFAULT);
    sn_abi_v1_release(view);
    SnArray empty = {NULL, 0, 0, sizeof(char *), NULL, NULL, SN_TAG_DEFAULT};
    assert(sn_abi_v1_native_string_array_borrow(&empty, &view) == 0);
    assert(sn_abi_v1_native_string_array_copy(view, &copy) == 0);
    assert(sn_abi_v1_native_string_array_data(copy, &header) == 0 && header != &empty && header->len == 0);
    sn_abi_v1_release(view);
    sn_abi_v1_release(copy);
    puts("native string array views: pass");
    return 0;
}
