#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <json-c/json.h>
#include <json-c/linkhash.h>
#include "../test_harness.h"
#include "target/rust/rust_record_parameter_guards.h"

static void test_rust_record_guard_shared_ancestor_resize(void)
{
    json_object *model = json_object_new_object();
    json_object *structs = json_object_new_array();
    json_object *record = json_object_new_object();
    json_object *unused = json_object_new_object();
    json_object *type = json_object_new_object();
    json_object *param = json_object_new_object();
    json_object *methods = json_object_new_array();
    json_object_object_add(model, "structs", structs);
    json_object_object_add(record, "name", json_object_new_string("Counter"));
    json_object_object_add(unused, "name", json_object_new_string("Unused"));
    json_object_array_add(structs, record);
    json_object_array_add(structs, unused);
    json_object_object_add(type, "name", json_object_new_string("Counter"));
    json_object_object_add(param, "type", type);
    json_object_object_add(param, "rust_native_record_parameter_rebind", json_object_new_boolean(true));
    json_object_array_add(methods, param);
    json_object_object_add(record, "methods", methods);
    /* Both references reach the same record's descendant. The extra guard key
     * must resize that ancestor table, reproducing the hosted crash condition. */
    json_object_object_add(model, "functions", json_object_get(methods));
    struct lh_table *table = json_object_get_object(record);
    int capacity = table->size;
    for (int i = 0; table->count < table->size * LH_LOAD_FACTOR; i++) {
        char key[40];
        snprintf(key, sizeof(key), "metadata_%d", i);
        json_object_object_add(record, key, json_object_new_int(i));
    }
    assert(table->size == capacity);
    rust_mark_record_parameter_guards(model);
    json_object *guard = NULL;
    assert(json_object_get_object(record)->size > capacity);
    assert(json_object_object_get_ex(record, "rust_native_record_parameter_guard_used", &guard));
    assert(json_object_get_boolean(guard));
    assert(!json_object_object_get_ex(unused, "rust_native_record_parameter_guard_used", &guard));
    rust_mark_record_parameter_guards(model);
    assert(json_object_get_object(record)->size > capacity);
    json_object_put(model);
}

static void test_rust_record_guard_main(void)
{
    TEST_RUN("rust record guard: shared ancestor table resize", test_rust_record_guard_shared_ancestor_resize);
}
