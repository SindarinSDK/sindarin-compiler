#include "target/rust/rust_native_response.h"
#include <stdlib.h>
#include <string.h>

bool rust_native_write_windows_link_response(FILE *file, const char *options,
                                            const char *error_file)
{
    if (!file || !options || !error_file) return false;
    size_t length = strlen(options), suffix = strlen(error_file) + 5;
    if (length < suffix) return false;
    const char *redirect = options + length - suffix;
    if (strncmp(redirect, " 2>\"", 4) ||
        strncmp(redirect + 4, error_file, strlen(error_file)) || options[length - 1] != '"')
        return false;
    char *arguments = malloc(length - suffix + 1);
    if (!arguments) return false;
    memcpy(arguments, options, length - suffix);
    arguments[length - suffix] = '\0';
    bool ok = rust_native_write_windows_response(file, arguments);
    free(arguments);
    return ok;
}

bool rust_native_write_windows_response(FILE *file, const char *options)
{
    if (!file || !options) return false;
    char *word = malloc(strlen(options) + 1);
    if (!word) return false;
    const char *cursor = options;
    bool ok = true;
    while (*cursor && ok)
    {
        while (*cursor == ' ' || *cursor == '\t') cursor++;
        if (!*cursor) break;
        char *out = word;
        bool quoted = false;
        while (*cursor && (quoted || (*cursor != ' ' && *cursor != '\t')))
        {
            size_t slashes = 0;
            while (*cursor == '\\') { slashes++; cursor++; }
            if (*cursor == '"')
            {
                for (size_t i = 0; i < slashes / 2; i++) *out++ = '\\';
                if (slashes % 2) { *out++ = '"'; cursor++; }
                else if (quoted && cursor[1] == '"') { *out++ = '"'; cursor += 2; }
                else { quoted = !quoted; cursor++; }
            }
            else
            {
                for (size_t i = 0; i < slashes; i++) *out++ = '\\';
                if (*cursor && (quoted || (*cursor != ' ' && *cursor != '\t')))
                    *out++ = *cursor++;
            }
        }
        *out = '\0';
        ok = fputc('"', file) != EOF;
        for (const char *p = word; *p && ok; p++)
        {
            if (*p == '\\' || *p == '"') ok = fputc('\\', file) != EOF;
            if (ok) ok = fputc(*p, file) != EOF;
        }
        if (ok) ok = fputs("\"\n", file) >= 0;
    }
    free(word);
    return ok && !ferror(file);
}
