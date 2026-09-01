#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
    const char *capture = getenv("SN_FAKE_RUSTC_CAPTURE");
    const char *exit_env = getenv("SN_FAKE_RUSTC_EXIT");
    int exit_code = 0;

    if (exit_env != NULL && exit_env[0] != '\0')
        exit_code = atoi(exit_env);

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
