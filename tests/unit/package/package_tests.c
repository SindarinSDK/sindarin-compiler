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
#include <json-c/json.h>

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

static const char *test_yaml_path(void)
{
#ifdef _WIN32
    static char path[1024];
    const char *temp_dir = getenv("TEMP");
    if (temp_dir == NULL || temp_dir[0] == '\0')
        temp_dir = ".";
    snprintf(path, sizeof(path), "%s\\sn_test_package.yaml", temp_dir);
    return path;
#else
    return "/tmp/sn_test_package.yaml";
#endif
}

static void cleanup_test_yaml(void)
{
    remove(test_yaml_path());
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

    bool write_success = package_yaml_write(test_yaml_path(), &config);
    assert(write_success == true);

    /* Parse it back */
    PackageConfig parsed = {0};
    bool parse_success = package_yaml_parse(test_yaml_path(), &parsed);
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

    bool write_success = package_yaml_write(test_yaml_path(), &config);
    assert(write_success == true);

    /* Parse it back */
    PackageConfig parsed = {0};
    bool parse_success = package_yaml_parse(test_yaml_path(), &parsed);
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

    bool write_success = package_yaml_write(test_yaml_path(), &config);
    assert(write_success == true);

    /* Add a dependency */
    PackageDependency dep = {0};
    strncpy(dep.name, "new-lib", sizeof(dep.name) - 1);
    strncpy(dep.git_url, "https://github.com/user/new-lib.git", sizeof(dep.git_url) - 1);
    strncpy(dep.tag, "v3.0", sizeof(dep.tag) - 1);

    bool add_success = package_yaml_add_dependency(test_yaml_path(), &dep);
    assert(add_success == true);

    /* Parse and verify */
    PackageConfig parsed = {0};
    bool parse_success = package_yaml_parse(test_yaml_path(), &parsed);
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

    bool write_success = package_yaml_write(test_yaml_path(), &config);
    assert(write_success == true);

    /* Update the dependency */
    PackageDependency dep = {0};
    strncpy(dep.name, "lib", sizeof(dep.name) - 1);
    strncpy(dep.git_url, "https://github.com/new/lib.git", sizeof(dep.git_url) - 1);
    strncpy(dep.tag, "v2.0", sizeof(dep.tag) - 1);

    bool add_success = package_yaml_add_dependency(test_yaml_path(), &dep);
    assert(add_success == true);

    /* Parse and verify - should still have 1 dependency but updated */
    PackageConfig parsed = {0};
    bool parse_success = package_yaml_parse(test_yaml_path(), &parsed);
    assert(parse_success == true);

    assert(parsed.dependency_count == 1);
    assert(strcmp(parsed.dependencies[0].name, "lib") == 0);
    assert(strcmp(parsed.dependencies[0].git_url, "https://github.com/new/lib.git") == 0);
    assert(strcmp(parsed.dependencies[0].tag, "v2.0") == 0);

    cleanup_test_yaml();
}

static void write_package_test_yaml(const char *text)
{
    FILE *f = fopen(test_yaml_path(), "w");
    assert(f != NULL);
    assert(fputs(text, f) >= 0);
    assert(fclose(f) == 0);
}

static void test_yaml_runtime_contract(void)
{
    PackageConfig config;
    const PackageRuntime runtimes[] = {PACKAGE_RUNTIME_C, PACKAGE_RUNTIME_RS, PACKAGE_RUNTIME_GO};
    const char *names[] = {"C", "RS", "GO"};
    for (int i = 0; i < 3; i++) {
        char text[128];
        snprintf(text, sizeof(text), "name: package\nruntime: %s\n", names[i]);
        write_package_test_yaml(text);
        assert(package_yaml_parse(test_yaml_path(), &config));
        assert(config.runtime == runtimes[i]);
        assert(strcmp(package_runtime_name(config.runtime), names[i]) == 0);
        assert(package_runtime_resolve(config.runtime, PACKAGE_RUNTIME_RS) == runtimes[i]);
        assert(package_yaml_write(test_yaml_path(), &config));
        memset(&config, 0, sizeof(config));
        assert(package_yaml_parse(test_yaml_path(), &config));
        assert(config.runtime == runtimes[i]);
        PackageDependency dep = {0};
        strcpy(dep.name, "extra");
        strcpy(dep.git_url, "https://example.com/extra.git");
        strcpy(dep.tag, "v1");
        assert(package_yaml_add_dependency(test_yaml_path(), &dep));
        strcpy(dep.tag, "v2");
        assert(package_yaml_add_dependency(test_yaml_path(), &dep));
        assert(package_yaml_parse(test_yaml_path(), &config));
        assert(config.runtime == runtimes[i]);
        assert(config.dependency_count == 1);
        assert(strcmp(config.dependencies[0].tag, "v2") == 0);
    }
    write_package_test_yaml("name: legacy\n");
    assert(package_yaml_parse(test_yaml_path(), &config));
    assert(config.runtime == PACKAGE_RUNTIME_INHERIT);
    assert(package_runtime_resolve(config.runtime, PACKAGE_RUNTIME_C) == PACKAGE_RUNTIME_C);
    assert(package_runtime_resolve(config.runtime, PACKAGE_RUNTIME_RS) == PACKAGE_RUNTIME_RS);
    assert(package_runtime_name(config.runtime) == NULL);
    assert(package_yaml_write(test_yaml_path(), &config));
    FILE *f = fopen(test_yaml_path(), "r");
    char text[256] = {0};
    assert(f != NULL);
    assert(fread(text, 1, sizeof(text) - 1, f) > 0);
    fclose(f);
    assert(strstr(text, "runtime") == NULL);
    cleanup_test_yaml();
}

static void test_yaml_runtime_errors(void)
{
    const char *invalid[] = {
        "runtime: rust\n", "runtime: c\n", "runtime: RSX\n", "runtime: null\n",
        "runtime: \"\"\n", "runtime: []\n", "runtime: {name: C}\n",
        "runtime: C\nruntime: RS\n", "runtime: RS\n---\nruntime: C\n",
        "runtime: \"C\\0junk\"\n", "runtime: [\n"
    };
    for (size_t i = 0; i < sizeof(invalid) / sizeof(invalid[0]); i++) {
        write_package_test_yaml(invalid[i]);
        PackageConfig config;
        assert(!package_yaml_parse(test_yaml_path(), &config));
        assert(config.runtime == PACKAGE_RUNTIME_INHERIT);
    }
    write_package_test_yaml("name: preserved\n");
    PackageConfig config = {0};
    config.runtime = (PackageRuntime)99;
    assert(!package_yaml_write(test_yaml_path(), &config));
    assert(package_yaml_parse(test_yaml_path(), &config));
    assert(strcmp(config.name, "preserved") == 0);
    cleanup_test_yaml();
}

static void test_yaml_nested_metadata(void)
{
    /* Nested unknown metadata must not choose a runtime or swallow later keys. */
    write_package_test_yaml("extension:\n  runtime: GO\n  nested: [C, RS]\n"
        "name: outer\ndependencies:\n  - name: dep\n    extra: {runtime: GO}\n"
        "    git: https://example.com/dep.git\n    branch: main\nruntime: RS\n");
    PackageConfig config;
    assert(package_yaml_parse(test_yaml_path(), &config));
    assert(strcmp(config.name, "outer") == 0);
    assert(config.runtime == PACKAGE_RUNTIME_RS);
    assert(config.dependency_count == 1);
    assert(strcmp(config.dependencies[0].git_url, "https://example.com/dep.git") == 0);
    assert(strcmp(config.dependencies[0].branch, "main") == 0);
    write_package_test_yaml("extension: {runtime: GO}\nname: legacy\n");
    assert(package_yaml_parse(test_yaml_path(), &config));
    assert(config.runtime == PACKAGE_RUNTIME_INHERIT);
    cleanup_test_yaml();
}

static const char *native_manifest_fixture =
    "name: mixed-native\nruntime: RS\n"
    "extension: &extra {note: must-survive, versions: [1, 2]}\n"
    "dependencies:\n  - name: old\n    git: https://example.com/old.git\n"
    "    tag: v1\n    extension: *extra\n"
    "native:\n  abi: 1.0\n  declarations: [src/api.sn]\n"
    "  builds:\n    - name: c_io\n      language: C\n      sources: [native/io.c]\n"
    "      include_dirs: [include]\n      libraries: [z]\n"
    "    - name: rs_codec\n      language: RS\n      sources: [native/lib.rs, native/value.rs]\n"
    "      entry: native/lib.rs\n"
    "    - name: go_codec\n      language: GO\n      sources: [native/go/main.go, native/go/go.mod]\n"
    "      module: native/go\n"
    "  bindings:\n    - declaration: src/api.sn::open\n      build: c_io\n"
    "      symbol: native_open\n      convention: C\n      failure: status\n"
    "      ownership: {parameters: {path: borrowed}, result: owned}\n"
    "    - declaration: src/api.sn::view\n      build: rs_codec\n"
    "      symbol: native_view\n      convention: C\n      failure: abort\n"
    "      ownership: {parameters: {self: borrowed}, result: borrowed, borrowed_from: self}\n"
    "    - declaration: src/api.sn::consume\n      build: go_codec\n"
    "      symbol: native_consume\n      convention: C\n      failure: status\n"
    "      ownership: {parameters: {input: owned, count: value}, result: value}\n";

static char *read_package_test_yaml(void)
{
    FILE *f = fopen(test_yaml_path(), "rb");
    assert(f && fseek(f, 0, SEEK_END) == 0);
    long size = ftell(f);
    assert(size >= 0 && fseek(f, 0, SEEK_SET) == 0);
    char *text = malloc((size_t)size + 1);
    assert(text && fread(text, 1, (size_t)size, f) == (size_t)size);
    text[size] = 0;
    fclose(f);
    return text;
}

static void test_native_manifest_plan_and_dependency_edit(void)
{
    write_package_test_yaml(native_manifest_fixture);
    json_object *before = NULL, *after = NULL;
    assert(package_yaml_native_plan(test_yaml_path(), &before));
    assert(before != NULL);
    json_object *builds = NULL, *bindings = NULL;
    assert(json_object_object_get_ex(before, "builds", &builds));
    assert(json_object_array_length(builds) == 3);
    assert(json_object_object_get_ex(before, "bindings", &bindings));
    assert(json_object_array_length(bindings) == 3);
    PackageConfig config;
    assert(package_yaml_parse(test_yaml_path(), &config));
    assert(config.has_native && config.runtime == PACKAGE_RUNTIME_RS);
    char *preserved = read_package_test_yaml();
    assert(!package_yaml_write(test_yaml_path(), &config));
    char *unchanged = read_package_test_yaml();
    assert(strcmp(preserved, unchanged) == 0);
    free(preserved); free(unchanged);
    PackageDependency dep = {0};
    strcpy(dep.name, "extra");
    strcpy(dep.git_url, "https://example.com/extra.git");
    strcpy(dep.tag, "v3");
    assert(package_yaml_add_dependency(test_yaml_path(), &dep));
    strcpy(dep.name, "old");
    dep.tag[0] = 0;
    strcpy(dep.branch, "main");
    assert(package_yaml_add_dependency(test_yaml_path(), &dep));
    assert(package_yaml_native_plan(test_yaml_path(), &after));
    assert(json_object_equal(before, after));
    assert(package_yaml_parse(test_yaml_path(), &config));
    assert(config.runtime == PACKAGE_RUNTIME_RS && config.has_native);
    assert(config.dependency_count == 2);
    assert(config.dependencies[0].tag[0] == 0);
    assert(strcmp(config.dependencies[0].branch, "main") == 0);
    preserved = read_package_test_yaml();
    assert(strstr(preserved, "must-survive") && strstr(preserved, "extension"));
    /* Verify the unknown dependency extension survives as well as the root. */
    const char *first = strstr(preserved, "extension:");
    assert(first && strstr(first + 1, "extension:"));
    free(preserved);
    json_object_put(before); json_object_put(after);
    write_package_test_yaml("dependencies: &original\n  - name: old\n"
        "    git: https://example.com/old.git\n    tag: v1\nextension: *original\n");
    assert(package_yaml_add_dependency(test_yaml_path(), &dep));
    assert(package_yaml_parse(test_yaml_path(), &config));
    assert(config.dependencies[0].tag[0] == 0);
    preserved = read_package_test_yaml();
    assert(strstr(preserved, "tag: v1")); /* the unrelated alias retains its value */
    free(preserved);
    write_package_test_yaml("name: legacy\n");
    after = (json_object *)(uintptr_t)1;
    assert(package_yaml_native_plan(test_yaml_path(), &after) && after == NULL);
    cleanup_test_yaml();
}

static void test_native_manifest_invalid_contracts(void)
{
    struct { const char *find; const char *replace; } invalid[] = {
        {"abi: 1.0", "abi: 2.0"}, {"abi: 1.0", "unknown: 1.0"},
        {"language: C", "language: Rust"}, {"sources: [native/io.c]", "sources: []"},
        {"sources: [native/io.c]", "sources: native/io.c"},
        {"include_dirs: [include]", "include_dirs: {}"},
        {"entry: native/lib.rs", "entry: nonexistent.rs"},
        {"module: native/go", "module: []"},
        {"name: rs_codec", "name: c_io"}, {"build: c_io", "build: missing"},
        {"convention: C", "convention: Rust"}, {"failure: status", "failure: unwind"},
        {"result: owned", "result: copied"}, {"path: borrowed", "path: retained"},
        {"borrowed_from: self", "borrowed_from: path"},
        {"borrowed_from: self", "borrowed_from: []"},
        {"parameters: {path: borrowed}", "parameters: []"},
        {"symbol: native_open", "symbol: \"\""},
        {"symbol: native_open", "symbol: native_open\n      symbol: other"},
        {"native:", "native: {abi: 1.0}\nnative:"},
        {"declarations: [src/api.sn]", "declarations: []"},
        {"declarations: [src/api.sn]", "declarations: [\"a\\0b\"]"},
        {"result: borrowed, borrowed_from: self", "result: borrowed"},
        {"result: borrowed, borrowed_from: self", "result: value, borrowed_from: self"},
        {"ownership: {parameters: {path: borrowed}, result: owned}", "extension: {}"},
        {"src/api.sn::view", "src/api.sn::open"},
    };
    for (size_t i = 0; i < sizeof(invalid) / sizeof(invalid[0]); i++) {
        const char *found = strstr(native_manifest_fixture, invalid[i].find);
        assert(found);
        size_t prefix = (size_t)(found - native_manifest_fixture);
        size_t size = strlen(native_manifest_fixture) + strlen(invalid[i].replace) + 1;
        char *text = malloc(size);
        assert(text);
        memcpy(text, native_manifest_fixture, prefix);
        strcpy(text + prefix, invalid[i].replace);
        strcpy(text + prefix + strlen(invalid[i].replace), found + strlen(invalid[i].find));
        write_package_test_yaml(text);
        json_object *out = (json_object *)(uintptr_t)1;
        assert(!package_yaml_native_plan(test_yaml_path(), &out));
        assert(out == (json_object *)(uintptr_t)1);
        PackageDependency dep = {0};
        strcpy(dep.name, "extra");
        assert(!package_yaml_add_dependency(test_yaml_path(), &dep));
        char *unchanged = read_package_test_yaml();
        assert(strcmp(text, unchanged) == 0);
        free(text); free(unchanged);
    }
    write_package_test_yaml("native: &cycle {abi: *cycle}\n");
    json_object *out = NULL;
    assert(!package_yaml_native_plan(test_yaml_path(), &out));
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
    TEST_RUN("yaml_runtime_contract", test_yaml_runtime_contract);
    TEST_RUN("yaml_runtime_errors", test_yaml_runtime_errors);
    TEST_RUN("yaml_nested_metadata", test_yaml_nested_metadata);
    TEST_RUN("native_manifest_plan_and_dependency_edit", test_native_manifest_plan_and_dependency_edit);
    TEST_RUN("native_manifest_invalid_contracts", test_native_manifest_invalid_contracts);
#endif
}
