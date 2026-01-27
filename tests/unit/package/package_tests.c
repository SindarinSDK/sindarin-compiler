/* ==============================================================================
 * package_tests.c - Unit Tests for Package Manager
 * ==============================================================================
 * Tests URL parsing, name extraction, and YAML operations.
 * ============================================================================== */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/stat.h>
#include <unistd.h>

#include "../test_harness.h"
#include "../../../src/package.h"

/* ============================================================================
 * URL Parsing Tests
 * ============================================================================ */

static void test_parse_url_ref_with_tag(void)
{
    char url[PKG_MAX_URL_LEN];
    char ref[PKG_MAX_REF_LEN];

    bool has_ref = package_parse_url_ref(
        "https://github.com/user/repo.git@v1.0.0",
        url, ref
    );

    assert(has_ref == true);
    assert(strcmp(url, "https://github.com/user/repo.git") == 0);
    assert(strcmp(ref, "v1.0.0") == 0);
}

static void test_parse_url_ref_with_branch(void)
{
    char url[PKG_MAX_URL_LEN];
    char ref[PKG_MAX_REF_LEN];

    bool has_ref = package_parse_url_ref(
        "https://github.com/user/repo.git@main",
        url, ref
    );

    assert(has_ref == true);
    assert(strcmp(url, "https://github.com/user/repo.git") == 0);
    assert(strcmp(ref, "main") == 0);
}

static void test_parse_url_ref_no_ref(void)
{
    char url[PKG_MAX_URL_LEN];
    char ref[PKG_MAX_REF_LEN];

    bool has_ref = package_parse_url_ref(
        "https://github.com/user/repo.git",
        url, ref
    );

    assert(has_ref == false);
    assert(strcmp(url, "https://github.com/user/repo.git") == 0);
    assert(ref[0] == '\0');
}

static void test_parse_url_ref_ssh_with_tag(void)
{
    char url[PKG_MAX_URL_LEN];
    char ref[PKG_MAX_REF_LEN];

    bool has_ref = package_parse_url_ref(
        "git@github.com:user/repo.git@v2.0",
        url, ref
    );

    assert(has_ref == true);
    assert(strcmp(url, "git@github.com:user/repo.git") == 0);
    assert(strcmp(ref, "v2.0") == 0);
}

static void test_parse_url_ref_ssh_no_ref(void)
{
    char url[PKG_MAX_URL_LEN];
    char ref[PKG_MAX_REF_LEN];

    bool has_ref = package_parse_url_ref(
        "git@github.com:user/repo.git",
        url, ref
    );

    assert(has_ref == false);
    assert(strcmp(url, "git@github.com:user/repo.git") == 0);
}

/* ============================================================================
 * Name Extraction Tests
 * ============================================================================ */

static void test_extract_name_https(void)
{
    char name[PKG_MAX_NAME_LEN];

    bool success = package_extract_name(
        "https://github.com/user/my-library.git",
        name
    );

    assert(success == true);
    assert(strcmp(name, "my-library") == 0);
}

static void test_extract_name_ssh(void)
{
    char name[PKG_MAX_NAME_LEN];

    bool success = package_extract_name(
        "git@github.com:org/sn-utils.git",
        name
    );

    assert(success == true);
    assert(strcmp(name, "sn-utils") == 0);
}

static void test_extract_name_no_git_extension(void)
{
    char name[PKG_MAX_NAME_LEN];

    bool success = package_extract_name(
        "https://github.com/user/repo",
        name
    );

    assert(success == true);
    assert(strcmp(name, "repo") == 0);
}

static void test_extract_name_nested_path(void)
{
    char name[PKG_MAX_NAME_LEN];

    bool success = package_extract_name(
        "https://gitlab.com/group/subgroup/project.git",
        name
    );

    assert(success == true);
    assert(strcmp(name, "project") == 0);
}

/* ============================================================================
 * YAML Tests (require filesystem)
 * ============================================================================ */

#if SN_HAS_PACKAGE_MANAGER

static const char *TEST_YAML_PATH = "/tmp/sn_test_package.yaml";

static void cleanup_test_yaml(void)
{
    unlink(TEST_YAML_PATH);
}

static void test_yaml_write_and_parse(void)
{
    cleanup_test_yaml();

    /* Create a config and write it */
    PackageConfig config = {0};
    strncpy(config.name, "test-project", sizeof(config.name) - 1);
    strncpy(config.version, "1.0.0", sizeof(config.version) - 1);
    strncpy(config.author, "Test Author", sizeof(config.author) - 1);
    strncpy(config.description, "A test project", sizeof(config.description) - 1);
    strncpy(config.license, "MIT", sizeof(config.license) - 1);

    bool write_success = package_yaml_write(TEST_YAML_PATH, &config);
    assert(write_success == true);

    /* Parse it back */
    PackageConfig parsed = {0};
    bool parse_success = package_yaml_parse(TEST_YAML_PATH, &parsed);
    assert(parse_success == true);

    /* Verify fields */
    assert(strcmp(parsed.name, "test-project") == 0);
    assert(strcmp(parsed.version, "1.0.0") == 0);
    assert(strcmp(parsed.author, "Test Author") == 0);
    assert(strcmp(parsed.description, "A test project") == 0);
    assert(strcmp(parsed.license, "MIT") == 0);
    assert(parsed.dependency_count == 0);

    cleanup_test_yaml();
}

static void test_yaml_write_with_dependencies(void)
{
    cleanup_test_yaml();

    /* Create a config with dependencies */
    PackageConfig config = {0};
    strncpy(config.name, "my-app", sizeof(config.name) - 1);
    strncpy(config.version, "2.0.0", sizeof(config.version) - 1);

    /* Add first dependency */
    strncpy(config.dependencies[0].name, "utils", sizeof(config.dependencies[0].name) - 1);
    strncpy(config.dependencies[0].git_url, "https://github.com/user/utils.git", sizeof(config.dependencies[0].git_url) - 1);
    strncpy(config.dependencies[0].tag, "v1.2.0", sizeof(config.dependencies[0].tag) - 1);

    /* Add second dependency */
    strncpy(config.dependencies[1].name, "http", sizeof(config.dependencies[1].name) - 1);
    strncpy(config.dependencies[1].git_url, "git@github.com:org/http.git", sizeof(config.dependencies[1].git_url) - 1);
    strncpy(config.dependencies[1].branch, "main", sizeof(config.dependencies[1].branch) - 1);

    config.dependency_count = 2;

    bool write_success = package_yaml_write(TEST_YAML_PATH, &config);
    assert(write_success == true);

    /* Parse it back */
    PackageConfig parsed = {0};
    bool parse_success = package_yaml_parse(TEST_YAML_PATH, &parsed);
    assert(parse_success == true);

    /* Verify */
    assert(strcmp(parsed.name, "my-app") == 0);
    assert(parsed.dependency_count == 2);

    assert(strcmp(parsed.dependencies[0].name, "utils") == 0);
    assert(strcmp(parsed.dependencies[0].git_url, "https://github.com/user/utils.git") == 0);
    assert(strcmp(parsed.dependencies[0].tag, "v1.2.0") == 0);

    assert(strcmp(parsed.dependencies[1].name, "http") == 0);
    assert(strcmp(parsed.dependencies[1].git_url, "git@github.com:org/http.git") == 0);
    assert(strcmp(parsed.dependencies[1].branch, "main") == 0);

    cleanup_test_yaml();
}

static void test_yaml_add_dependency(void)
{
    cleanup_test_yaml();

    /* Create initial config */
    PackageConfig config = {0};
    strncpy(config.name, "test-app", sizeof(config.name) - 1);
    strncpy(config.version, "1.0.0", sizeof(config.version) - 1);

    bool write_success = package_yaml_write(TEST_YAML_PATH, &config);
    assert(write_success == true);

    /* Add a dependency */
    PackageDependency dep = {0};
    strncpy(dep.name, "new-lib", sizeof(dep.name) - 1);
    strncpy(dep.git_url, "https://github.com/user/new-lib.git", sizeof(dep.git_url) - 1);
    strncpy(dep.tag, "v3.0", sizeof(dep.tag) - 1);

    bool add_success = package_yaml_add_dependency(TEST_YAML_PATH, &dep);
    assert(add_success == true);

    /* Parse and verify */
    PackageConfig parsed = {0};
    bool parse_success = package_yaml_parse(TEST_YAML_PATH, &parsed);
    assert(parse_success == true);

    assert(parsed.dependency_count == 1);
    assert(strcmp(parsed.dependencies[0].name, "new-lib") == 0);
    assert(strcmp(parsed.dependencies[0].tag, "v3.0") == 0);

    cleanup_test_yaml();
}

static void test_yaml_update_dependency(void)
{
    cleanup_test_yaml();

    /* Create config with existing dependency */
    PackageConfig config = {0};
    strncpy(config.name, "test-app", sizeof(config.name) - 1);
    strncpy(config.dependencies[0].name, "lib", sizeof(config.dependencies[0].name) - 1);
    strncpy(config.dependencies[0].git_url, "https://github.com/old/lib.git", sizeof(config.dependencies[0].git_url) - 1);
    strncpy(config.dependencies[0].tag, "v1.0", sizeof(config.dependencies[0].tag) - 1);
    config.dependency_count = 1;

    bool write_success = package_yaml_write(TEST_YAML_PATH, &config);
    assert(write_success == true);

    /* Update the dependency */
    PackageDependency dep = {0};
    strncpy(dep.name, "lib", sizeof(dep.name) - 1);
    strncpy(dep.git_url, "https://github.com/new/lib.git", sizeof(dep.git_url) - 1);
    strncpy(dep.tag, "v2.0", sizeof(dep.tag) - 1);

    bool add_success = package_yaml_add_dependency(TEST_YAML_PATH, &dep);
    assert(add_success == true);

    /* Parse and verify - should still have 1 dependency but updated */
    PackageConfig parsed = {0};
    bool parse_success = package_yaml_parse(TEST_YAML_PATH, &parsed);
    assert(parse_success == true);

    assert(parsed.dependency_count == 1);
    assert(strcmp(parsed.dependencies[0].name, "lib") == 0);
    assert(strcmp(parsed.dependencies[0].git_url, "https://github.com/new/lib.git") == 0);
    assert(strcmp(parsed.dependencies[0].tag, "v2.0") == 0);

    cleanup_test_yaml();
}

#endif /* SN_HAS_PACKAGE_MANAGER */

/* ============================================================================
 * Test Main Entry Point
 * ============================================================================ */

void test_package_main(void)
{
    TEST_SECTION("Package Manager");

    /* URL Parsing Tests */
    TEST_RUN("parse_url_ref_with_tag", test_parse_url_ref_with_tag);
    TEST_RUN("parse_url_ref_with_branch", test_parse_url_ref_with_branch);
    TEST_RUN("parse_url_ref_no_ref", test_parse_url_ref_no_ref);
    TEST_RUN("parse_url_ref_ssh_with_tag", test_parse_url_ref_ssh_with_tag);
    TEST_RUN("parse_url_ref_ssh_no_ref", test_parse_url_ref_ssh_no_ref);

    /* Name Extraction Tests */
    TEST_RUN("extract_name_https", test_extract_name_https);
    TEST_RUN("extract_name_ssh", test_extract_name_ssh);
    TEST_RUN("extract_name_no_git_extension", test_extract_name_no_git_extension);
    TEST_RUN("extract_name_nested_path", test_extract_name_nested_path);

#if SN_HAS_PACKAGE_MANAGER
    /* YAML Tests (require libyaml) */
    TEST_RUN("yaml_write_and_parse", test_yaml_write_and_parse);
    TEST_RUN("yaml_write_with_dependencies", test_yaml_write_with_dependencies);
    TEST_RUN("yaml_add_dependency", test_yaml_add_dependency);
    TEST_RUN("yaml_update_dependency", test_yaml_update_dependency);
#endif
}
