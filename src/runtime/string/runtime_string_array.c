// src/runtime/runtime_string_array.c
// String Array Helpers

/* ============================================================================
 * String Array Helpers
 * ============================================================================
 * These functions support creating and growing string arrays for split operations.
 * ============================================================================ */

char **rt_create_string_array(RtArenaV2 *arena, size_t initial_capacity)
{
    /* Allocate array with header storing length and capacity */
    size_t header_size = 2 * sizeof(size_t);
    size_t alloc_size = header_size + (initial_capacity + 1) * sizeof(char *);
    RtHandleV2 *block_h = rt_arena_v2_alloc(arena, alloc_size);
    if (block_h == NULL) return NULL;
    rt_handle_v2_pin(block_h);
    void *block = block_h->ptr;

    size_t *header = (size_t *)block;
    header[0] = 0;                  /* length */
    header[1] = initial_capacity;   /* capacity */

    char **arr = (char **)((char *)block + header_size);
    arr[0] = NULL;  /* NULL terminator */
    return arr;
}

char **rt_push_string_to_array(RtArenaV2 *arena, char **arr, char *str)
{
    size_t *header = (size_t *)((char *)arr - 2 * sizeof(size_t));
    size_t len = header[0];
    size_t cap = header[1];

    if (len >= cap) {
        /* Need to grow - allocate new array with double capacity */
        size_t new_cap = cap * 2;
        char **new_arr = rt_create_string_array(arena, new_cap);
        if (new_arr == NULL) return arr;

        /* Copy existing elements */
        for (size_t i = 0; i < len; i++) {
            new_arr[i] = arr[i];
        }
        size_t *new_header = (size_t *)((char *)new_arr - 2 * sizeof(size_t));
        new_header[0] = len;
        arr = new_arr;
        header = new_header;
    }

    arr[len] = str;
    arr[len + 1] = NULL;  /* NULL terminator */
    header[0] = len + 1;
    return arr;
}
