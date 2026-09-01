#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv)
{
    const char *capture = getenv("SN_FAKE_RUSTC_CAPTURE");
    const char *exit_env = getenv("SN_FAKE_RUSTC_EXIT");
    const char *version_exit_env = getenv("SN_FAKE_RUSTC_VERSION_EXIT");
    const char *build_exit_env = getenv("SN_FAKE_RUSTC_BUILD_EXIT");
    int exit_code = 0;

    if (exit_env != NULL && exit_env[0] != '\0')
        exit_code = atoi(exit_env);

    bool is_version_invocation = false;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--version") == 0) {
            is_version_invocation = true;
            break;
        }
    }

    if (is_version_invocation) {
        if (version_exit_env != NULL && version_exit_env[0] != '\0')
            exit_code = atoi(version_exit_env);
    } else {
        if (build_exit_env != NULL && build_exit_env[0] != '\0')
            exit_code = atoi(build_exit_env);
    }

    if (capture != NULL && capture[0] != '\0') {
        FILE *f = fopen(capture, "a");
        if (f != NULL) {
            fprintf(f, "BEGIN\n");
            for (int i = 0; i < argc; i++) {
                fprintf(f, "%s\n", argv[i]);
            }
            fprintf(f, "END\n");
            fclose(f);
        }
    }

    return exit_code;
}
