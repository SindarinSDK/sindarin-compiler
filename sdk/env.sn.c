/* ==============================================================================
 * sdk/env.sn.c - Self-contained Environment Implementation for Sindarin SDK
 * ==============================================================================
 * This file provides the C implementation for the SnEnvironment type.
 * It is compiled via #pragma source and linked with Sindarin code.
 * ============================================================================== */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* Include runtime arena for proper memory management */
#include "runtime/runtime_arena.h"

/* ============================================================================
 * Platform-Specific Includes
 * ============================================================================ */

#ifdef _WIN32
#include <windows.h>
#else
/* POSIX systems */
extern char **environ;
#endif

/* ============================================================================
 * RtEnvironment Type Definition (Static-only, never instantiated)
 * ============================================================================ */

typedef struct RtEnvironment {
    int _unused;  /* Placeholder - this struct is never instantiated */
} RtEnvironment;

/* ============================================================================
 * Array Metadata Structure (must match runtime)
 * ============================================================================ */

typedef struct {
    RtArena *arena;
    size_t size;
    size_t capacity;
} RtArrayMetadata;

/* ============================================================================
 * Helper Functions
 * ============================================================================ */

/* Create a string array in the arena */
static char **sn_create_string_array(RtArena *arena, size_t count)
{
    char **result = rt_arena_alloc(arena, sizeof(RtArrayMetadata) + count * sizeof(char *));
    if (result == NULL) {
        fprintf(stderr, "sn_create_string_array: allocation failed\n");
        exit(1);
    }

    RtArrayMetadata *meta = (RtArrayMetadata *)result;
    meta->arena = arena;
    meta->size = count;
    meta->capacity = count;

    return (char **)(meta + 1);
}

/* ============================================================================
 * Environment Variable Access Functions
 * ============================================================================ */

#ifdef _WIN32

/* Windows implementation using GetEnvironmentVariable */
char *sn_env_get_required(RtArena *arena, const char *name)
{
    if (arena == NULL || name == NULL) {
        fprintf(stderr, "RuntimeError: Environment variable name cannot be null\n");
        exit(1);
    }

    /* First call to get required buffer size */
    DWORD size = GetEnvironmentVariableA(name, NULL, 0);
    if (size == 0) {
        /* Variable not found */
        fprintf(stderr, "RuntimeError: Environment variable '%s' is not set\n", name);
        exit(1);
    }

    /* Allocate buffer in arena and get the value */
    char *buffer = rt_arena_alloc(arena, size);
    if (buffer == NULL) {
        fprintf(stderr, "sn_env_get_required: allocation failed\n");
        exit(1);
    }

    DWORD result = GetEnvironmentVariableA(name, buffer, size);
    if (result == 0 || result >= size) {
        fprintf(stderr, "RuntimeError: Failed to read environment variable '%s'\n", name);
        exit(1);
    }

    return buffer;
}

char *sn_env_get_default(RtArena *arena, const char *name, const char *default_value)
{
    if (arena == NULL || name == NULL) {
        return default_value ? rt_arena_strdup(arena, default_value) : NULL;
    }

    /* First call to get required buffer size */
    DWORD size = GetEnvironmentVariableA(name, NULL, 0);
    if (size == 0) {
        /* Variable not found, return default */
        if (default_value != NULL) {
            return rt_arena_strdup(arena, default_value);
        }
        return NULL;
    }

    /* Allocate buffer in arena and get the value */
    char *buffer = rt_arena_alloc(arena, size);
    if (buffer == NULL) {
        fprintf(stderr, "sn_env_get_default: allocation failed\n");
        exit(1);
    }

    DWORD result = GetEnvironmentVariableA(name, buffer, size);
    if (result == 0 || result >= size) {
        /* Error, return default */
        if (default_value != NULL) {
            return rt_arena_strdup(arena, default_value);
        }
        return NULL;
    }

    return buffer;
}

int sn_env_has(const char *name)
{
    if (name == NULL) {
        return 0;
    }
    DWORD size = GetEnvironmentVariableA(name, NULL, 0);
    return size > 0 ? 1 : 0;
}

char ***sn_env_all(RtArena *arena)
{
    if (arena == NULL) {
        return NULL;
    }

    /* Get the environment block */
    LPCH envStrings = GetEnvironmentStringsA();
    if (envStrings == NULL) {
        return NULL;
    }

    /* First pass: count entries */
    size_t count = 0;
    LPCH ptr = envStrings;
    while (*ptr) {
        /* Skip entries that start with '=' (Windows internal variables) */
        if (*ptr != '=') {
            count++;
        }
        ptr += strlen(ptr) + 1;
    }

    /* Create outer array to hold pairs */
    char ***result = rt_arena_alloc(arena, sizeof(RtArrayMetadata) + count * sizeof(char **));
    if (result == NULL) {
        FreeEnvironmentStringsA(envStrings);
        fprintf(stderr, "sn_env_all: allocation failed\n");
        exit(1);
    }

    /* Set up array metadata */
    RtArrayMetadata *meta = (RtArrayMetadata *)result;
    meta->arena = arena;
    meta->size = count;
    meta->capacity = count;
    result = (char ***)(meta + 1);

    /* Second pass: populate array */
    ptr = envStrings;
    size_t idx = 0;
    while (*ptr && idx < count) {
        if (*ptr != '=') {
            /* Find the '=' separator */
            char *eq = strchr(ptr, '=');
            if (eq != NULL) {
                size_t name_len = eq - ptr;

                /* Create pair array [name, value] */
                char **pair = sn_create_string_array(arena, 2);
                pair[0] = rt_arena_strndup(arena, ptr, name_len);
                pair[1] = rt_arena_strdup(arena, eq + 1);

                result[idx++] = pair;
            }
        }
        ptr += strlen(ptr) + 1;
    }

    FreeEnvironmentStringsA(envStrings);
    return result;
}

#else

/* POSIX implementation using getenv and environ */

char *sn_env_get_required(RtArena *arena, const char *name)
{
    if (arena == NULL || name == NULL) {
        fprintf(stderr, "RuntimeError: Environment variable name cannot be null\n");
        exit(1);
    }

    const char *value = getenv(name);
    if (value == NULL) {
        fprintf(stderr, "RuntimeError: Environment variable '%s' is not set\n", name);
        exit(1);
    }

    /* Copy to arena (getenv returns pointer to static storage) */
    return rt_arena_strdup(arena, value);
}

char *sn_env_get_default(RtArena *arena, const char *name, const char *default_value)
{
    if (arena == NULL || name == NULL) {
        return default_value ? rt_arena_strdup(arena, default_value) : NULL;
    }

    const char *value = getenv(name);
    if (value != NULL) {
        return rt_arena_strdup(arena, value);
    }

    /* Variable not set, return default */
    if (default_value != NULL) {
        return rt_arena_strdup(arena, default_value);
    }
    return NULL;
}

int sn_env_has(const char *name)
{
    if (name == NULL) {
        return 0;
    }
    return getenv(name) != NULL ? 1 : 0;
}

char ***sn_env_all(RtArena *arena)
{
    if (arena == NULL) {
        return NULL;
    }

    /* Count entries */
    size_t count = 0;
    for (char **e = environ; *e != NULL; e++) {
        count++;
    }

    /* Create outer array to hold pairs */
    char ***result = rt_arena_alloc(arena, sizeof(RtArrayMetadata) + count * sizeof(char **));
    if (result == NULL) {
        fprintf(stderr, "sn_env_all: allocation failed\n");
        exit(1);
    }

    /* Set up array metadata */
    RtArrayMetadata *meta = (RtArrayMetadata *)result;
    meta->arena = arena;
    meta->size = count;
    meta->capacity = count;
    result = (char ***)(meta + 1);

    /* Populate array */
    for (size_t i = 0; i < count; i++) {
        const char *entry = environ[i];
        const char *eq = strchr(entry, '=');

        /* Create pair array [name, value] */
        char **pair = sn_create_string_array(arena, 2);

        if (eq != NULL) {
            size_t name_len = eq - entry;
            pair[0] = rt_arena_strndup(arena, entry, name_len);
            pair[1] = rt_arena_strdup(arena, eq + 1);
        } else {
            /* Malformed entry (no '='), use empty value */
            pair[0] = rt_arena_strdup(arena, entry);
            pair[1] = rt_arena_strdup(arena, "");
        }

        result[i] = pair;
    }

    return result;
}

#endif
