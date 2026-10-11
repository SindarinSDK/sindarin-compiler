#ifndef SN_RUST_NATIVE_RESPONSE_H
#define SN_RUST_NATIVE_RESPONSE_H
#include <stdbool.h>
#include <stdio.h>
/* Parse Windows CRT argument quoting and append GNU response-file arguments.
 * Both supported GCC-compatible C drivers read this response-file syntax. */
bool rust_native_write_windows_response(FILE *file, const char *options);
#endif
