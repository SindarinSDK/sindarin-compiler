#ifndef SN_MINIMAL_H
#define SN_MINIMAL_H

/*
 * Sindarin Minimal Runtime — umbrella header.
 *
 * Generated C code includes this single header. It pulls in all
 * runtime modules in dependency order.
 */

#include "sn_core.h"      /* includes, OOM wrappers, cleanup macros */
#include "sn_thread.h"    /* SnThread, pthread helpers */
#include "sn_array.h"     /* SnArray, element operations, method macros */
#include "sn_string.h"    /* string operations, split, method macros */
#include "sn_byte.h"      /* byte array encoding (hex, base64, latin1) */
#include "sn_arith.h"     /* checked/unchecked arithmetic */
#include "sn_conv.h"      /* type conversions, comparisons, I/O */
#include "sn_reflect.h"   /* TypeInfo, FieldInfo for typeOf() */

#endif
