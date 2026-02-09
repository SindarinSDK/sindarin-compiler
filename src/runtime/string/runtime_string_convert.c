// src/runtime/runtime_string_convert.c
// Type Conversion Functions (rt_to_string_*)

/* ============================================================================
 * Type Conversion Functions (rt_to_string_*)
 *
 * IMMUTABLE STRINGS: All rt_to_string_* functions create immutable strings
 * via rt_arena_strdup(). These functions can remain unchanged because:
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
    return rt_arena_strdup(arena, buf);
}

char *rt_to_string_double(RtArenaV2 *arena, double val)
{
    char buf[64];
    snprintf(buf, sizeof(buf), "%.5f", val);
    return rt_arena_strdup(arena, buf);
}

char *rt_to_string_char(RtArenaV2 *arena, char val)
{
    char buf[2] = {val, '\0'};
    return rt_arena_strdup(arena, buf);
}

char *rt_to_string_bool(RtArenaV2 *arena, int val)
{
    return rt_arena_strdup(arena, val ? "true" : "false");
}

char *rt_to_string_byte(RtArenaV2 *arena, unsigned char val)
{
    char buf[8];
    snprintf(buf, sizeof(buf), "%u", (unsigned int)val);
    return rt_arena_strdup(arena, buf);
}

char *rt_to_string_string(RtArenaV2 *arena, const char *val)
{
    if (val == NULL) {
        return (char *)null_str;
    }
    return rt_arena_strdup(arena, val);
}

char *rt_to_string_void(RtArenaV2 *arena)
{
    return rt_arena_strdup(arena, "void");
}

char *rt_to_string_pointer(RtArenaV2 *arena, void *p)
{
    if (p == NULL) {
        return rt_arena_strdup(arena, "nil");
    }
    char buf[32];
    snprintf(buf, sizeof(buf), "%p", p);
    return rt_arena_strdup(arena, buf);
}
