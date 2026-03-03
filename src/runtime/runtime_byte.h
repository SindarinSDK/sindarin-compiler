#ifndef RUNTIME_BYTE_H
#define RUNTIME_BYTE_H

#include "arena/arena_v2.h"

/* ============================================================================
 * Byte Array Conversion Functions
 * ============================================================================
 * Functions for converting between byte arrays and string representations.
 * Includes UTF-8, Latin-1, hexadecimal, and Base64 encodings.
 * All functions use RtHandleV2* for both input and output.
 * ============================================================================ */

/* ============================================================================
 * Byte Array to String Conversions
 * ============================================================================ */

/* Convert byte array to string using UTF-8 decoding */
RtHandleV2 *rt_byte_array_to_string(RtArenaV2 *arena, RtHandleV2 *bytes_h);

/* Convert byte array to string using Latin-1 (ISO-8859-1) decoding */
RtHandleV2 *rt_byte_array_to_string_latin1(RtArenaV2 *arena, RtHandleV2 *bytes_h);

/* Convert byte array to hexadecimal string representation */
RtHandleV2 *rt_byte_array_to_hex(RtArenaV2 *arena, RtHandleV2 *bytes_h);

/* Convert byte array to Base64 string representation */
RtHandleV2 *rt_byte_array_to_base64(RtArenaV2 *arena, RtHandleV2 *bytes_h);

/* ============================================================================
 * String to Byte Array Conversions
 * ============================================================================ */

/* Convert string to byte array using UTF-8 encoding */
RtHandleV2 *rt_string_to_bytes(RtArenaV2 *arena, RtHandleV2 *str_h);

#endif /* RUNTIME_BYTE_H */
