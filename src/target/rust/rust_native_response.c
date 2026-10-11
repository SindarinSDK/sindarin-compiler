#include "target/rust/rust_native_response.h"
#include <stdlib.h>
#include <string.h>

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
