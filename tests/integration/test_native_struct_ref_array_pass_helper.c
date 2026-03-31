/* C helper for test_native_struct_ref_array_pass.sn
 * Verifies that arrays of native struct as ref elements arrive
 * with valid, dereferenceable element pointers on the C side. */

#include <stdio.h>

long long verify_single_widget(__sn__Widget *w, long long expected_id) {
    if (!w) {
        printf("FAIL: single widget is NULL\n");
        return 0;
    }
    printf("single widget: id=%lld, value=%lld\n", w->__sn__id, w->__sn__value);
    return (w->__sn__id == expected_id) ? 1 : 0;
}

long long verify_widget_array(SnArray *widgets, long long expected_count) {
    long long n = sn_array_length(widgets);
    printf("native count: %lld\n", n);
    if (n != expected_count) {
        printf("FAIL: expected %lld elements, got %lld\n", expected_count, n);
        return 0;
    }

    for (long long i = 0; i < n; i++) {
        __sn__Widget *w = *(__sn__Widget **)sn_array_get(widgets, i);
        if (!w) {
            printf("FAIL: widget[%lld] is NULL\n", i);
            return 0;
        }
        printf("widget[%lld]: id=%lld, value=%lld\n", i, w->__sn__id, w->__sn__value);
    }

    return 1;
}
