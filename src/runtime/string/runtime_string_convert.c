// src/runtime/runtime_string_convert.c
// Type Conversion Functions (rt_to_string_*)

/* ============================================================================
 * Type Conversion Functions (rt_to_string_*)
 *
 * IMMUTABLE STRINGS: All rt_to_string_* functions create immutable strings
 * via rt_arena_v2_strdup(). These functions can remain unchanged because:
 * 1. They create short, fixed-size strings that don't need appending
 * 2. The results are typically used in interpolation/concatenation
 * 3. No benefit to adding metadata for these small strings
 *
 * TODO: If profiling shows these are bottlenecks in hot loops, consider
 * caching common values (e.g., "true"/"false", small integers 0-99).
 * ============================================================================ */

static const char *null_str = "(null)";

char *rt_to_string_long(RtArenaV2 *arena, long long val)
{
    char buf[32];
    snprintf(buf, sizeof(buf), "%lld", val);
    RtHandleV2 *_h = rt_arena_v2_strdup(arena, buf); rt_handle_begin_transaction(_h); char *_r = (char *)_h->ptr; rt_handle_end_transaction(_h); return _r;
}

char *rt_to_string_double(RtArenaV2 *arena, double val)
{
    char buf[64];
    snprintf(buf, sizeof(buf), "%.5f", val);
    RtHandleV2 *_h = rt_arena_v2_strdup(arena, buf); rt_handle_begin_transaction(_h); char *_r = (char *)_h->ptr; rt_handle_end_transaction(_h); return _r;
}

char *rt_to_string_char(RtArenaV2 *arena, char val)
{
    char buf[2] = {val, '\0'};
    RtHandleV2 *_h = rt_arena_v2_strdup(arena, buf); rt_handle_begin_transaction(_h); char *_r = (char *)_h->ptr; rt_handle_end_transaction(_h); return _r;
}

char *rt_to_string_bool(RtArenaV2 *arena, int val)
{
    RtHandleV2 *_h = rt_arena_v2_strdup(arena, val ? "true" : "false"); rt_handle_begin_transaction(_h); char *_r = (char *)_h->ptr; rt_handle_end_transaction(_h); return _r;
}

char *rt_to_string_byte(RtArenaV2 *arena, unsigned char val)
{
    char buf[8];
    snprintf(buf, sizeof(buf), "%u", (unsigned int)val);
    RtHandleV2 *_h = rt_arena_v2_strdup(arena, buf); rt_handle_begin_transaction(_h); char *_r = (char *)_h->ptr; rt_handle_end_transaction(_h); return _r;
}

char *rt_to_string_string(RtArenaV2 *arena, const char *val)
{
    if (val == NULL) {
        return (char *)null_str;
    }
    RtHandleV2 *_h = rt_arena_v2_strdup(arena, val); rt_handle_begin_transaction(_h); char *_r = (char *)_h->ptr; rt_handle_end_transaction(_h); return _r;
}

char *rt_to_string_void(RtArenaV2 *arena)
{
    RtHandleV2 *_h = rt_arena_v2_strdup(arena, "void"); rt_handle_begin_transaction(_h); char *_r = (char *)_h->ptr; rt_handle_end_transaction(_h); return _r;
}

char *rt_to_string_pointer(RtArenaV2 *arena, RtHandleV2 *p)
{
    if (p == NULL) {
        RtHandleV2 *_h = rt_arena_v2_strdup(arena, "nil"); rt_handle_begin_transaction(_h); char *_r = (char *)_h->ptr; rt_handle_end_transaction(_h); return _r;
    }
    char buf[32];
    snprintf(buf, sizeof(buf), "%p", p->ptr);
    RtHandleV2 *_h = rt_arena_v2_strdup(arena, buf); rt_handle_begin_transaction(_h); char *_r = (char *)_h->ptr; rt_handle_end_transaction(_h); return _r;
}
