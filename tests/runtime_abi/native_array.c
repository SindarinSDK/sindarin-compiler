#include <assert.h>
#include <string.h>
#include "sn_array.h"
#include "sn_abi.h"

static unsigned released_rows, released_strings;
static SnAbiValue *consumed;
static void release_string(void *slot) { released_strings++; free(*(char **)slot); }
static void release_row(void *slot) { released_rows++; sn_array_free(*(SnArray **)slot); }
static void copy_consuming(const void *source, void *out)
{
    memcpy(out, source, sizeof(long long));
    if (consumed) { SnAbiValue *value = consumed; consumed = NULL; sn_abi_v1_release(value); }
}
static SnArray *row(void)
{
    SnArray *r = sn_array_new(sizeof(char *), 3); r->elem_tag = SN_TAG_STRING;
    r->elem_release = release_string;
    char *key = strdup("key"), *value = strdup("value"), *nil = NULL;
    sn_array_push(r, &key); sn_array_push(r, &value); sn_array_push(r, &nil);
    return r;
}
int main(void)
{
    const SnAbiNativeArrayType strings = {SN_ABI_ARRAY_STRING, 2};
    SnAbiInfo info;
    const uint64_t masks[] = {7,31,63,127,255,511,1023};
    for (uint32_t i = 0; i < 7; i++) {
        assert(sn_abi_v1_query(SN_ABI_V1_VERSION+i, 0, &info, sizeof(info)) == 0 && info.capabilities == masks[i]);
        SnAbiInfo saved = info;
        assert(sn_abi_v1_query(SN_ABI_V1_VERSION+i, SN_ABI_CAP_NATIVE_ARRAYS, &info, sizeof(info)) == SN_ABI_UNSUPPORTED);
        assert(!memcmp(&info, &saved, sizeof(info)));
    }
    assert(sn_abi_v1_query(SN_ABI_V1_7_VERSION, SN_ABI_CAP_NATIVE_ARRAYS, &info, sizeof(info)) == 0 && info.capabilities == 2047);
    SnAbiValue *view = (SnAbiValue *)(uintptr_t)1, *copy = NULL;
    SnArray *out = (SnArray *)(uintptr_t)1;
    assert(sn_abi_v1_native_array_borrow(NULL, strings, &view) == 0 && !view);
    assert(sn_abi_v1_native_array_data(NULL, strings, &out) == 0 && !out);
    assert(sn_abi_v1_native_array_take(&view, strings, &out) == 0 && !view && !out);
    assert(sn_abi_v1_native_array_type(NULL, NULL) == SN_ABI_INVALID_ARGUMENT);
    SnArray *outer = sn_array_new(sizeof(SnArray *), 2), *r = row(), *nil = NULL;
    outer->elem_tag = SN_TAG_ARRAY; outer->elem_release = release_row;
    sn_array_push(outer, &r); sn_array_push(outer, &nil);
    view = (SnAbiValue *)(uintptr_t)1;
    assert(sn_abi_v1_native_array_adopt(outer, (SnAbiNativeArrayType){10,0}, &view) == SN_ABI_INVALID_ARGUMENT && view == (SnAbiValue *)(uintptr_t)1);
    assert(sn_abi_v1_native_array_borrow(outer, (SnAbiNativeArrayType){10,33}, &view) == SN_ABI_INVALID_ARGUMENT);
    assert(sn_abi_v1_native_array_borrow(outer, (SnAbiNativeArrayType){11,2}, &view) == SN_ABI_INVALID_ARGUMENT);
    r->elem_size = 1;
    assert(sn_abi_v1_native_array_borrow(outer, strings, &view) == SN_ABI_WRONG_KIND && view == (SnAbiValue *)(uintptr_t)1);
    r->elem_size = sizeof(char *);
    r->cap = 1;
    assert(sn_abi_v1_native_array_adopt(outer, strings, &view) == SN_ABI_INVALID_ARGUMENT);
    r->cap = 3;
    assert(sn_abi_v1_native_array_borrow(outer, strings, &view) == 0);
    SnAbiNativeArrayType type = {0,0};
    assert(sn_abi_v1_native_array_type(view, &type) == 0 && type.rank == 2 && type.leaf_kind == 10);
    assert(sn_abi_v1_native_array_data(view, strings, &out) == 0 && out == outer);
    out = (SnArray *)(uintptr_t)1;
    assert(sn_abi_v1_native_array_data(view, (SnAbiNativeArrayType){5,2}, &out) == SN_ABI_WRONG_KIND && out == (SnArray *)(uintptr_t)1);
    assert(sn_abi_v1_native_array_take(&view, strings, &out) == SN_ABI_INVALID_ARGUMENT && out == (SnArray *)(uintptr_t)1);
    SnAbiValue *alias = sn_abi_v1_retain(view);
    ((char **)r->data)[1][0] = 'V';
    assert(sn_abi_v1_native_array_data(alias, strings, &out) == 0 && !strcmp(((char **)r->data)[1], "Value"));
    assert(sn_abi_v1_native_array_copy(view, &copy) == 0);
    SnArray *copied = NULL;
    assert(sn_abi_v1_native_array_data(copy, strings, &copied) == 0 && copied != outer);
    SnArray *copied_row = ((SnArray **)copied->data)[0];
    assert(copied_row != r && ((char **)copied_row->data)[0] != ((char **)r->data)[0]);
    assert(copied->elem_release == outer->elem_release && copied_row->elem_release == r->elem_release);
    assert(copied->elem_copy && copied_row->elem_copy && !outer->elem_copy && !r->elem_copy);
    sn_abi_v1_release(view); sn_abi_v1_release(alias); sn_array_free(outer);
    assert(released_rows == 2 && released_strings == 3);
    assert(!strcmp(((char **)copied_row->data)[1], "Value") && !((SnArray **)copied->data)[1]);
    SnArray *second = sn_array_copy(copied);
    assert(((SnArray **)second->data)[0] != copied_row);
    sn_array_free(second);
    alias = sn_abi_v1_retain(copy); out = (SnArray *)(uintptr_t)1;
    assert(sn_abi_v1_native_array_take(&copy, strings, &out) == SN_ABI_INVALID_ARGUMENT && out == (SnArray *)(uintptr_t)1);
    sn_abi_v1_release(alias);
    assert(sn_abi_v1_native_array_take(&copy, strings, &out) == 0 && !copy && out == copied);
    assert(out->elem_release == release_row); sn_array_free(out);
    assert(released_rows == 6 && released_strings == 9);
    const size_t widths[] = {8,4,8,4,1,1,4,8,1,sizeof(char *)};
    const enum SnElemTag tags[] = {SN_TAG_INT,SN_TAG_INT,SN_TAG_INT,SN_TAG_INT,SN_TAG_BYTE,
        SN_TAG_BOOL,SN_TAG_DOUBLE,SN_TAG_DOUBLE,SN_TAG_CHAR,SN_TAG_STRING};
    for (uint32_t kind = 1; kind <= 10; kind++) {
        SnAbiNativeArrayType shape = {kind,1};
        SnArray *leaf = sn_array_new(widths[kind-1],1);
        leaf->elem_tag = tags[kind-1];
        unsigned char zero[16] = {0}; sn_array_push(leaf, zero);
        assert(sn_abi_v1_native_array_adopt(leaf, shape, &view) == 0);
        assert(sn_abi_v1_native_array_data(view, shape, &out) == 0 && out == leaf);
        assert(sn_abi_v1_native_array_copy(view, &copy) == 0);
        sn_abi_v1_release(copy);
        out = (SnArray *)(uintptr_t)1;
        assert(sn_abi_v1_native_array_data(view, (SnAbiNativeArrayType){kind,2}, &out) == SN_ABI_WRONG_KIND && out == (SnArray *)(uintptr_t)1);
        leaf->len = -1;
        assert(sn_abi_v1_native_array_data(view, shape, &out) == SN_ABI_INVALID_ARGUMENT && out == (SnArray *)(uintptr_t)1);
        leaf->len = 1;
        if (kind == SN_ABI_ARRAY_BOOL) {
            *(uint8_t *)leaf->data = 2;
            assert(sn_abi_v1_native_array_data(view, shape, &out) == SN_ABI_INVALID_ARGUMENT);
            *(uint8_t *)leaf->data = 1;
        }
        sn_abi_v1_release(view);
    }
    /* Recursive tags detect a self-cycle without retaining or taking it. */
    SnArray *cycle = sn_array_new(sizeof(SnArray *),1); cycle->elem_tag = SN_TAG_ARRAY;
    sn_array_push(cycle, &cycle); view = (SnAbiValue *)(uintptr_t)1;
    assert(sn_abi_v1_native_array_adopt(cycle, strings, &view) == SN_ABI_WRONG_KIND && view == (SnAbiValue *)(uintptr_t)1);
    sn_array_free(cycle);
    /* Copying borrowed strings supplies cleanup on the new owned header. */
    SnArray *borrowed = sn_array_new(sizeof(char *),1); borrowed->elem_tag = SN_TAG_STRING;
    char *literal = "borrowed"; sn_array_push(borrowed, &literal);
    const SnAbiNativeArrayType flat_strings = {SN_ABI_ARRAY_STRING,1};
    assert(sn_abi_v1_native_array_borrow(borrowed, flat_strings, &view) == 0);
    assert(sn_abi_v1_native_array_copy(view, &copy) == 0);
    sn_abi_v1_release(view); sn_array_free(borrowed);
    assert(sn_abi_v1_native_array_data(copy, flat_strings, &out) == 0);
    assert(!strcmp(*(char **)out->data, "borrowed") && out->elem_release && out->elem_copy);
    sn_abi_v1_release(copy);
    const SnAbiNativeArrayType integers = {SN_ABI_ARRAY_INT64,1};
    SnArray *numbers = sn_array_new(8,1); numbers->elem_tag = SN_TAG_INT; numbers->elem_copy = copy_consuming;
    long long number = -17; sn_array_push(numbers, &number);
    assert(sn_abi_v1_native_array_adopt(numbers, integers, &view) == 0); consumed = view;
    assert(sn_abi_v1_native_array_copy(view, &copy) == 0 && !consumed);
    assert(sn_abi_v1_native_array_data(copy, integers, &out) == 0 && *(long long *)out->data == -17);
    assert(out->elem_copy == copy_consuming); sn_abi_v1_release(copy);
    puts("typed native array views: pass");
    return 0;
}
