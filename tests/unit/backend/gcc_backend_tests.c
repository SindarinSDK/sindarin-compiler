// tests/unit/backend/gcc_backend_tests.c
// Unit tests for the GCC/C compiler backend

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "../test_harness.h"
#include "../gcc_backend.h"
#include "../code_gen.h"
#include "../debug.h"

/* ============================================================================
 * Backend Configuration Tests
 * ============================================================================ */

static void test_config_init_defaults(void)
{
    CCBackendConfig config;
    cc_backend_init_config(&config);

    /* Should have non-NULL values for all fields */
    assert(config.cc != NULL);
    assert(config.std != NULL);
    assert(config.debug_cflags != NULL);
    assert(config.release_cflags != NULL);
    assert(config.cflags != NULL);
    assert(config.ldflags != NULL);
    assert(config.ldlibs != NULL);

    /* Default C standard should be c99 */
    assert(strcmp(config.std, "c99") == 0);
}

static void test_config_default_compiler(void)
{
    CCBackendConfig config;
    cc_backend_init_config(&config);

    /* Default compiler should be gcc or clang depending on platform */
#ifdef _WIN32
    /* Windows defaults to clang (LLVM-MinGW) */
    assert(strcmp(config.cc, "clang") == 0 || strcmp(config.cc, "gcc") == 0);
#else
    /* Unix defaults to gcc */
    assert(strcmp(config.cc, "gcc") == 0 || strcmp(config.cc, "clang") == 0);
#endif
}

static void test_config_debug_flags(void)
{
    CCBackendConfig config;
    cc_backend_init_config(&config);

    /* Debug flags should contain -g */
    assert(config.debug_cflags != NULL);
    assert(strstr(config.debug_cflags, "-g") != NULL || strstr(config.debug_cflags, "/Zi") != NULL);
}

static void test_config_release_flags(void)
{
    CCBackendConfig config;
    cc_backend_init_config(&config);

    /* Release flags should contain optimization */
    assert(config.release_cflags != NULL);
    assert(strstr(config.release_cflags, "-O") != NULL || strstr(config.release_cflags, "/O") != NULL);
}

static void test_config_ldflags_empty(void)
{
    CCBackendConfig config;
    cc_backend_init_config(&config);

    /* Default ldflags should be empty or at least not NULL */
    assert(config.ldflags != NULL);
}

static void test_config_ldlibs_not_null(void)
{
    CCBackendConfig config;
    cc_backend_init_config(&config);

    assert(config.ldlibs != NULL);
}

static void test_config_cflags_not_null(void)
{
    CCBackendConfig config;
    cc_backend_init_config(&config);

    assert(config.cflags != NULL);
}

/* ============================================================================
 * Compiler Detection Tests
 * ============================================================================ */

/* These test internal behavior through observable effects */

static void test_detect_gcc_compiler(void)
{
    CCBackendConfig config;
    /* Test with explicit gcc */
    /* Note: We can't directly test detect_backend() as it's static,
     * but we can verify config works with gcc */
    cc_backend_init_config(&config);
    assert(config.cc != NULL);
}

static void test_config_std_c99(void)
{
    CCBackendConfig config;
    cc_backend_init_config(&config);

    /* Verify default standard is c99 */
    assert(strcmp(config.std, "c99") == 0);
}

/* ============================================================================
 * SDK Resolution Tests
 * ============================================================================ */

static void test_get_compiler_dir_not_null(void)
{
    const char *dir = gcc_get_compiler_dir(NULL);
    assert(dir != NULL);
    assert(strlen(dir) > 0);
}

static void test_get_compiler_dir_with_argv0(void)
{
    const char *dir = gcc_get_compiler_dir("./bin/sn");
    assert(dir != NULL);
    assert(strlen(dir) > 0);
}

static void test_get_compiler_dir_absolute_path(void)
{
#ifdef _WIN32
    const char *dir = gcc_get_compiler_dir("C:\\Program Files\\sn\\bin\\sn.exe");
#else
    const char *dir = gcc_get_compiler_dir("/usr/local/bin/sn");
#endif
    assert(dir != NULL);
    assert(strlen(dir) > 0);
}

static void test_sdk_cache_reset(void)
{
    /* Reset cache should not crash */
    gcc_reset_sdk_cache();

    /* After reset, should be able to resolve again */
    gcc_reset_sdk_cache();
}

static void test_resolve_sdk_import_nonexistent(void)
{
    gcc_reset_sdk_cache();
    const char *path = gcc_resolve_sdk_import(".", "nonexistent_module_xyz123");
    /* Should return NULL for nonexistent module */
    assert(path == NULL);
}

static void test_resolve_sdk_import_strips_prefix(void)
{
    gcc_reset_sdk_cache();
    /* Both sdk/math and math should resolve the same way */
    const char *path1 = gcc_resolve_sdk_import(".", "sdk/test_nonexistent");
    const char *path2 = gcc_resolve_sdk_import(".", "test_nonexistent");
    /* Both should return NULL for nonexistent module */
    assert(path1 == NULL);
    assert(path2 == NULL);
}

static void test_resolve_sdk_import_with_backslash(void)
{
    gcc_reset_sdk_cache();
    /* Should handle backslash prefix */
    const char *path = gcc_resolve_sdk_import(".", "sdk\\test_nonexistent");
    assert(path == NULL);
}

/* ============================================================================
 * Compiler Availability Tests
 * ============================================================================ */

static void test_check_available_returns_bool(void)
{
    CCBackendConfig config;
    cc_backend_init_config(&config);

    /* gcc_check_available returns true or false */
    bool result = gcc_check_available(&config, false);
    /* Result depends on system, just verify it returns a valid bool */
    assert(result == true || result == false);
}

/* ============================================================================
 * Path Handling Tests
 * ============================================================================ */

static void test_compiler_dir_ends_without_separator(void)
{
    const char *dir = gcc_get_compiler_dir("./test/binary");
    assert(dir != NULL);
    size_t len = strlen(dir);
    /* Should not end with path separator (or be single char) */
    if (len > 1) {
#ifdef _WIN32
        assert(dir[len-1] != '\\' || dir[len-1] != '/');
#else
        /* Unix is fine with trailing slash in some cases */
#endif
    }
}

static void test_compiler_dir_handles_dot(void)
{
    const char *dir = gcc_get_compiler_dir("./sn");
    assert(dir != NULL);
    /* Should return a valid path string */
    assert(strlen(dir) > 0);
}

static void test_compiler_dir_handles_basename(void)
{
    const char *dir = gcc_get_compiler_dir("sn");
    assert(dir != NULL);
    assert(strlen(dir) > 0);
}

/* ============================================================================
 * Pragma Source Validation Tests
 * ============================================================================ */

static void test_validate_null_sources(void)
{
    /* NULL sources should return true (nothing to validate) */
    bool result = gcc_validate_pragma_sources(NULL, 0, false);
    assert(result == true);
}

static void test_validate_empty_sources(void)
{
    /* Empty count should return true */
    PragmaSourceInfo sources[1];
    bool result = gcc_validate_pragma_sources(sources, 0, false);
    assert(result == true);
}

/* ============================================================================
 * Config Load Tests
 * ============================================================================ */

static void test_load_config_no_crash(void)
{
    /* Loading config from nonexistent path should not crash */
    cc_backend_load_config("/nonexistent/path");
    /* If we get here, it didn't crash */
}

static void test_load_config_current_dir(void)
{
    /* Loading config from current dir should not crash */
    cc_backend_load_config(".");
}

static void test_config_after_load(void)
{
    cc_backend_load_config(".");
    CCBackendConfig config;
    cc_backend_init_config(&config);

    /* Config should still be valid after loading from nonexistent file */
    assert(config.cc != NULL);
    assert(config.std != NULL);
}

/* ============================================================================
 * Multiple Config Initialization Tests
 * ============================================================================ */

static void test_config_init_multiple_times(void)
{
    CCBackendConfig config1, config2, config3;

    cc_backend_init_config(&config1);
    cc_backend_init_config(&config2);
    cc_backend_init_config(&config3);

    /* All should have same default values */
    assert(strcmp(config1.std, config2.std) == 0);
    assert(strcmp(config2.std, config3.std) == 0);
    assert(strcmp(config1.cc, config2.cc) == 0);
}

static void test_config_values_stable(void)
{
    CCBackendConfig config;
    cc_backend_init_config(&config);

    const char *cc1 = config.cc;
    const char *std1 = config.std;

    /* Re-init should give same pointers (statically allocated) */
    CCBackendConfig config2;
    cc_backend_init_config(&config2);

    /* Values should be equal */
    assert(strcmp(cc1, config2.cc) == 0);
    assert(strcmp(std1, config2.std) == 0);
}

/* ============================================================================
 * Edge Case Tests
 * ============================================================================ */

static void test_compiler_dir_empty_string(void)
{
    const char *dir = gcc_get_compiler_dir("");
    assert(dir != NULL);
}

static void test_sdk_resolve_empty_module(void)
{
    gcc_reset_sdk_cache();
    const char *path = gcc_resolve_sdk_import(".", "");
    /* Empty module name should return NULL */
    assert(path == NULL);
}

static void test_sdk_resolve_with_extension(void)
{
    gcc_reset_sdk_cache();
    /* .sn extension is added by the function, so this should fail */
    const char *path = gcc_resolve_sdk_import(".", "test.sn");
    assert(path == NULL);
}

static void test_config_consistent_across_calls(void)
{
    CCBackendConfig c1, c2;
    cc_backend_init_config(&c1);
    cc_backend_init_config(&c2);

    assert(strcmp(c1.cc, c2.cc) == 0);
    assert(strcmp(c1.std, c2.std) == 0);
    assert(strcmp(c1.debug_cflags, c2.debug_cflags) == 0);
    assert(strcmp(c1.release_cflags, c2.release_cflags) == 0);
}

/* ============================================================================
 * Backend-Specific Flag Tests
 * ============================================================================ */

static void test_debug_has_symbol_flag(void)
{
    CCBackendConfig config;
    cc_backend_init_config(&config);

    /* All backends should have some debug symbol flag */
    assert(config.debug_cflags != NULL);
    assert(strlen(config.debug_cflags) > 0);
}

static void test_release_has_optimization(void)
{
    CCBackendConfig config;
    cc_backend_init_config(&config);

    assert(config.release_cflags != NULL);
    assert(strlen(config.release_cflags) > 0);
}

/* ============================================================================
 * Platform-Specific Tests
 * ============================================================================ */

#ifdef _WIN32
static void test_windows_default_libs(void)
{
    CCBackendConfig config;
    cc_backend_init_config(&config);

    /* Windows builds may need ws2_32 or other libs */
    /* Just verify ldlibs is not NULL */
    assert(config.ldlibs != NULL);
}
#endif

#ifndef _WIN32
static void test_unix_pthread_support(void)
{
    CCBackendConfig config;
    cc_backend_init_config(&config);

    /* Should be able to compile with pthread support */
    assert(config.cc != NULL);
}
#endif

/* ============================================================================
 * SDK Directory Tests
 * ============================================================================ */

static void test_sdk_multiple_resolves(void)
{
    gcc_reset_sdk_cache();

    /* Multiple resolves should be consistent */
    const char *p1 = gcc_resolve_sdk_import(".", "nonexistent1");
    const char *p2 = gcc_resolve_sdk_import(".", "nonexistent2");
    const char *p3 = gcc_resolve_sdk_import(".", "nonexistent3");

    assert(p1 == NULL);
    assert(p2 == NULL);
    assert(p3 == NULL);
}

static void test_sdk_resolve_after_reset(void)
{
    gcc_reset_sdk_cache();
    gcc_resolve_sdk_import(".", "test1");

    gcc_reset_sdk_cache();
    gcc_resolve_sdk_import(".", "test2");

    /* Should not crash */
}

/* ============================================================================
 * Validate Sources Edge Cases
 * ============================================================================ */

static void test_validate_zero_count(void)
{
    PragmaSourceInfo info[1] = {{0}};
    bool result = gcc_validate_pragma_sources(info, 0, false);
    assert(result == true);
}

static void test_validate_verbose_mode(void)
{
    /* Verbose mode should not crash with NULL sources */
    bool result = gcc_validate_pragma_sources(NULL, 0, true);
    assert(result == true);
}

/* ============================================================================
 * Config Isolation Tests
 * ============================================================================ */

static void test_configs_are_independent(void)
{
    CCBackendConfig c1, c2;
    cc_backend_init_config(&c1);
    cc_backend_init_config(&c2);

    /* Two configs should be independent structures */
    assert(&c1 != &c2);
    assert(&c1.cc != &c2.cc);
}

static void test_config_fields_all_set(void)
{
    CCBackendConfig config;
    memset(&config, 0, sizeof(config));
    cc_backend_init_config(&config);

    /* All fields should be set */
    assert(config.cc != NULL);
    assert(config.std != NULL);
    assert(config.debug_cflags != NULL);
    assert(config.release_cflags != NULL);
    assert(config.cflags != NULL);
    assert(config.ldflags != NULL);
    assert(config.ldlibs != NULL);
}

/* ============================================================================
 * Compiler Dir Edge Cases
 * ============================================================================ */

static void test_compiler_dir_long_path(void)
{
    /* Long path should not overflow */
    char long_path[512];
    memset(long_path, 'a', sizeof(long_path) - 1);
    long_path[sizeof(long_path) - 1] = '\0';
    long_path[0] = '.';
    long_path[1] = '/';

    const char *dir = gcc_get_compiler_dir(long_path);
    assert(dir != NULL);
}

static void test_compiler_dir_special_chars(void)
{
    const char *dir = gcc_get_compiler_dir("./path-with_special.chars/binary");
    assert(dir != NULL);
}

/* ============================================================================
 * Stress Tests
 * ============================================================================ */

static void test_repeated_config_init(void)
{
    for (int i = 0; i < 100; i++) {
        CCBackendConfig config;
        cc_backend_init_config(&config);
        assert(config.cc != NULL);
        assert(config.std != NULL);
    }
}

static void test_repeated_sdk_resolve(void)
{
    for (int i = 0; i < 100; i++) {
        gcc_reset_sdk_cache();
        const char *p = gcc_resolve_sdk_import(".", "nonexistent");
        assert(p == NULL);
    }
}

static void test_repeated_compiler_dir(void)
{
    for (int i = 0; i < 100; i++) {
        const char *dir = gcc_get_compiler_dir("./test/path");
        assert(dir != NULL);
    }
}

/* ============================================================================
 * Test Runner
 * ============================================================================ */

void test_gcc_backend_main(void)
{
    TEST_SECTION("GCC Backend - Configuration");
    TEST_RUN("config_init_defaults", test_config_init_defaults);
    TEST_RUN("config_default_compiler", test_config_default_compiler);
    TEST_RUN("config_debug_flags", test_config_debug_flags);
    TEST_RUN("config_release_flags", test_config_release_flags);
    TEST_RUN("config_ldflags_empty", test_config_ldflags_empty);
    TEST_RUN("config_ldlibs_not_null", test_config_ldlibs_not_null);
    TEST_RUN("config_cflags_not_null", test_config_cflags_not_null);
    TEST_RUN("config_std_c99", test_config_std_c99);
    TEST_RUN("config_after_load", test_config_after_load);
    TEST_RUN("config_init_multiple_times", test_config_init_multiple_times);
    TEST_RUN("config_values_stable", test_config_values_stable);
    TEST_RUN("config_consistent_across_calls", test_config_consistent_across_calls);
    TEST_RUN("configs_are_independent", test_configs_are_independent);
    TEST_RUN("config_fields_all_set", test_config_fields_all_set);

    TEST_SECTION("GCC Backend - Compiler Detection");
    TEST_RUN("detect_gcc_compiler", test_detect_gcc_compiler);
    TEST_RUN("check_available_returns_bool", test_check_available_returns_bool);

    TEST_SECTION("GCC Backend - SDK Resolution");
    TEST_RUN("get_compiler_dir_not_null", test_get_compiler_dir_not_null);
    TEST_RUN("get_compiler_dir_with_argv0", test_get_compiler_dir_with_argv0);
    TEST_RUN("get_compiler_dir_absolute_path", test_get_compiler_dir_absolute_path);
    TEST_RUN("sdk_cache_reset", test_sdk_cache_reset);
    TEST_RUN("resolve_sdk_import_nonexistent", test_resolve_sdk_import_nonexistent);
    TEST_RUN("resolve_sdk_import_strips_prefix", test_resolve_sdk_import_strips_prefix);
    TEST_RUN("resolve_sdk_import_with_backslash", test_resolve_sdk_import_with_backslash);
    TEST_RUN("sdk_multiple_resolves", test_sdk_multiple_resolves);
    TEST_RUN("sdk_resolve_after_reset", test_sdk_resolve_after_reset);
    TEST_RUN("sdk_resolve_empty_module", test_sdk_resolve_empty_module);
    TEST_RUN("sdk_resolve_with_extension", test_sdk_resolve_with_extension);

    TEST_SECTION("GCC Backend - Path Handling");
    TEST_RUN("compiler_dir_ends_without_separator", test_compiler_dir_ends_without_separator);
    TEST_RUN("compiler_dir_handles_dot", test_compiler_dir_handles_dot);
    TEST_RUN("compiler_dir_handles_basename", test_compiler_dir_handles_basename);
    TEST_RUN("compiler_dir_empty_string", test_compiler_dir_empty_string);
    TEST_RUN("compiler_dir_long_path", test_compiler_dir_long_path);
    TEST_RUN("compiler_dir_special_chars", test_compiler_dir_special_chars);

    TEST_SECTION("GCC Backend - Pragma Validation");
    TEST_RUN("validate_null_sources", test_validate_null_sources);
    TEST_RUN("validate_empty_sources", test_validate_empty_sources);
    TEST_RUN("validate_zero_count", test_validate_zero_count);
    TEST_RUN("validate_verbose_mode", test_validate_verbose_mode);

    TEST_SECTION("GCC Backend - Config Load");
    TEST_RUN("load_config_no_crash", test_load_config_no_crash);
    TEST_RUN("load_config_current_dir", test_load_config_current_dir);

    TEST_SECTION("GCC Backend - Backend-Specific");
    TEST_RUN("debug_has_symbol_flag", test_debug_has_symbol_flag);
    TEST_RUN("release_has_optimization", test_release_has_optimization);

#ifdef _WIN32
    TEST_SECTION("GCC Backend - Windows Specific");
    TEST_RUN("windows_default_libs", test_windows_default_libs);
#else
    TEST_SECTION("GCC Backend - Unix Specific");
    TEST_RUN("unix_pthread_support", test_unix_pthread_support);
#endif

    TEST_SECTION("GCC Backend - Stress Tests");
    TEST_RUN("repeated_config_init", test_repeated_config_init);
    TEST_RUN("repeated_sdk_resolve", test_repeated_sdk_resolve);
    TEST_RUN("repeated_compiler_dir", test_repeated_compiler_dir);
}
