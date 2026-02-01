#include "lexer/lexer_scan.h"
#include "lexer/lexer_util.h"
#include "debug.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <stdint.h>

static char error_buffer[128];

/* Convert a hex character to its value (0-15), returns -1 if invalid */
static int hex_char_to_int(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return 10 + (c - 'a');
    if (c >= 'A' && c <= 'F') return 10 + (c - 'A');
    return -1;
}

/* Include split modules */
#include "lexer_scan_keyword.c"
#include "lexer_scan_number.c"
#include "lexer_scan_string.c"
#include "lexer_scan_pipe.c"
