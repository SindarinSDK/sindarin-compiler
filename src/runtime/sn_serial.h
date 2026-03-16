#ifndef SN_SERIAL_H
#define SN_SERIAL_H

/*
 * Sindarin Serialization Runtime — Encoder and Decoder vtable structs.
 *
 * These are the C representations of the built-in Encoder and Decoder
 * types used by @serializable structs. Format libraries (JSON, XML, etc.)
 * provide concrete implementations by populating the vtable function pointers.
 */

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

/* Forward declarations */
typedef struct __sn__Encoder __sn__Encoder;
typedef struct __sn__Decoder __sn__Decoder;

/* ---- Encoder vtable ---- */

typedef struct {
    void (*writeStr)(__sn__Encoder *self, const char *key, const char *value);
    void (*writeInt)(__sn__Encoder *self, const char *key, long long value);
    void (*writeDouble)(__sn__Encoder *self, const char *key, double value);
    void (*writeBool)(__sn__Encoder *self, const char *key, long long value);
    void (*writeNull)(__sn__Encoder *self, const char *key);
    __sn__Encoder *(*beginObject)(__sn__Encoder *self, const char *key);
    __sn__Encoder *(*beginArray)(__sn__Encoder *self, const char *key);
    void (*end)(__sn__Encoder *self);
    void (*appendStr)(__sn__Encoder *self, const char *value);
    void (*appendInt)(__sn__Encoder *self, long long value);
    void (*appendDouble)(__sn__Encoder *self, double value);
    void (*appendBool)(__sn__Encoder *self, long long value);
    __sn__Encoder *(*appendObject)(__sn__Encoder *self);
    char *(*result)(__sn__Encoder *self);
} __sn__EncoderVTable;

struct __sn__Encoder {
    __sn__EncoderVTable *__sn__vt;
    void *__sn__ctx;
    void (*__sn__cleanup)(__sn__Encoder *self);
};

/* ---- Decoder vtable ---- */

typedef struct {
    char *(*readStr)(__sn__Decoder *self, const char *key);
    long long (*readInt)(__sn__Decoder *self, const char *key);
    double (*readDouble)(__sn__Decoder *self, const char *key);
    long long (*readBool)(__sn__Decoder *self, const char *key);
    long long (*hasKey)(__sn__Decoder *self, const char *key);
    __sn__Decoder *(*readObject)(__sn__Decoder *self, const char *key);
    __sn__Decoder *(*readArray)(__sn__Decoder *self, const char *key);
    long long (*length)(__sn__Decoder *self);
    __sn__Decoder *(*at)(__sn__Decoder *self, long long index);
    char *(*atStr)(__sn__Decoder *self, long long index);
    long long (*atInt)(__sn__Decoder *self, long long index);
    double (*atDouble)(__sn__Decoder *self, long long index);
    long long (*atBool)(__sn__Decoder *self, long long index);
} __sn__DecoderVTable;

struct __sn__Decoder {
    __sn__DecoderVTable *__sn__vt;
    void *__sn__ctx;
    void (*__sn__cleanup)(__sn__Decoder *self);
};

/* ---- Encoder memory management (as ref / pointer semantics) ---- */

static inline __sn__Encoder *__sn__Encoder_alloc(void) {
    return (__sn__Encoder *)calloc(1, sizeof(__sn__Encoder));
}

static inline void __sn__Encoder_release(__sn__Encoder **p) {
    if (*p) {
        if ((*p)->__sn__cleanup) (*p)->__sn__cleanup(*p);
        free(*p);
    }
    *p = NULL;
}

#define sn_auto_Encoder __attribute__((cleanup(__sn__Encoder_release)))
#define sn_auto_ref_Encoder __attribute__((cleanup(__sn__Encoder_release)))

static inline void __sn__Encoder_release_elem(void *p) {
    __sn__Encoder_release((__sn__Encoder **)p);
}

/* ---- Decoder memory management (as ref / pointer semantics) ---- */

static inline __sn__Decoder *__sn__Decoder_alloc(void) {
    return (__sn__Decoder *)calloc(1, sizeof(__sn__Decoder));
}

static inline void __sn__Decoder_release(__sn__Decoder **p) {
    if (*p) {
        if ((*p)->__sn__cleanup) (*p)->__sn__cleanup(*p);
        free(*p);
    }
    *p = NULL;
}

#define sn_auto_Decoder __attribute__((cleanup(__sn__Decoder_release)))
#define sn_auto_ref_Decoder __attribute__((cleanup(__sn__Decoder_release)))

static inline void __sn__Decoder_release_elem(void *p) {
    __sn__Decoder_release((__sn__Decoder **)p);
}

/* ---- Vtable dispatch macros ----
 *
 * The compiler generates method calls like __sn__Encoder_writeStr(enc, "key", val).
 * These macros expand them to vtable dispatch: enc->__sn__vt->writeStr(enc, "key", val).
 * Zero overhead — the C preprocessor inlines the indirection. */

#define __sn__Encoder_writeStr(__self, __key, __val)    (__self)->__sn__vt->writeStr((__self), (__key), (__val))
#define __sn__Encoder_writeInt(__self, __key, __val)    (__self)->__sn__vt->writeInt((__self), (__key), (__val))
#define __sn__Encoder_writeDouble(__self, __key, __val) (__self)->__sn__vt->writeDouble((__self), (__key), (__val))
#define __sn__Encoder_writeBool(__self, __key, __val)   (__self)->__sn__vt->writeBool((__self), (__key), (__val))
#define __sn__Encoder_writeNull(__self, __key)          (__self)->__sn__vt->writeNull((__self), (__key))
#define __sn__Encoder_beginObject(__self, __key)        (__self)->__sn__vt->beginObject((__self), (__key))
#define __sn__Encoder_beginArray(__self, __key)         (__self)->__sn__vt->beginArray((__self), (__key))
#define __sn__Encoder_end(__self)                       (__self)->__sn__vt->end((__self))
#define __sn__Encoder_appendStr(__self, __val)          (__self)->__sn__vt->appendStr((__self), (__val))
#define __sn__Encoder_appendInt(__self, __val)          (__self)->__sn__vt->appendInt((__self), (__val))
#define __sn__Encoder_appendDouble(__self, __val)       (__self)->__sn__vt->appendDouble((__self), (__val))
#define __sn__Encoder_appendBool(__self, __val)         (__self)->__sn__vt->appendBool((__self), (__val))
#define __sn__Encoder_appendObject(__self)              (__self)->__sn__vt->appendObject((__self))
#define __sn__Encoder_result(__self)                    (__self)->__sn__vt->result((__self))

#define __sn__Decoder_readStr(__self, __key)            (__self)->__sn__vt->readStr((__self), (__key))
#define __sn__Decoder_readInt(__self, __key)            (__self)->__sn__vt->readInt((__self), (__key))
#define __sn__Decoder_readDouble(__self, __key)         (__self)->__sn__vt->readDouble((__self), (__key))
#define __sn__Decoder_readBool(__self, __key)           (__self)->__sn__vt->readBool((__self), (__key))
#define __sn__Decoder_hasKey(__self, __key)             (__self)->__sn__vt->hasKey((__self), (__key))
#define __sn__Decoder_readObject(__self, __key)         (__self)->__sn__vt->readObject((__self), (__key))
#define __sn__Decoder_readArray(__self, __key)          (__self)->__sn__vt->readArray((__self), (__key))
#define __sn__Decoder_length(__self)                    (__self)->__sn__vt->length((__self))
#define __sn__Decoder_at(__self, __idx)                 (__self)->__sn__vt->at((__self), (__idx))
#define __sn__Decoder_atStr(__self, __idx)              (__self)->__sn__vt->atStr((__self), (__idx))
#define __sn__Decoder_atInt(__self, __idx)              (__self)->__sn__vt->atInt((__self), (__idx))
#define __sn__Decoder_atDouble(__self, __idx)           (__self)->__sn__vt->atDouble((__self), (__idx))
#define __sn__Decoder_atBool(__self, __idx)             (__self)->__sn__vt->atBool((__self), (__idx))

#endif
