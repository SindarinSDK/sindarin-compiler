/*
 * Native C harness for test_serializable_push_ownership.sn
 * Re-uses the same minimal JSON encoder/decoder from test_serializable.sn.c
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

/* ---- Growable buffer ---- */

typedef struct {
    char *data;
    size_t len, cap;
} Buf;

static Buf *buf_new(void) {
    Buf *b = calloc(1, sizeof(Buf));
    b->cap = 256;
    b->data = malloc(b->cap);
    b->data[0] = '\0';
    return b;
}

static void buf_ensure(Buf *b, size_t need) {
    if (b->len + need >= b->cap) {
        b->cap = (b->cap + need) * 2;
        b->data = realloc(b->data, b->cap);
    }
}

static void buf_cat(Buf *b, const char *s) {
    size_t n = strlen(s);
    buf_ensure(b, n);
    memcpy(b->data + b->len, s, n);
    b->len += n;
    b->data[b->len] = '\0';
}

static void buf_char(Buf *b, char c) {
    buf_ensure(b, 1);
    b->data[b->len++] = c;
    b->data[b->len] = '\0';
}

/* ---- JSON Encoder ---- */

typedef struct {
    Buf *buf;
    int first;
    int is_array;
    int is_root;
} JEnc;

static void je_comma(JEnc *j) {
    if (!j->first) buf_char(j->buf, ',');
    j->first = 0;
}

static void je_write_str(__sn__Encoder *self, const char *key, const char *val) {
    JEnc *j = (JEnc *)self->__sn__ctx;
    je_comma(j);
    buf_char(j->buf, '"'); buf_cat(j->buf, key); buf_cat(j->buf, "\":\"");
    for (const char *p = val; *p; p++) {
        if (*p == '"') buf_cat(j->buf, "\\\"");
        else if (*p == '\\') buf_cat(j->buf, "\\\\");
        else if (*p == '\n') buf_cat(j->buf, "\\n");
        else buf_char(j->buf, *p);
    }
    buf_char(j->buf, '"');
}

static void je_write_int(__sn__Encoder *self, const char *key, long long val) {
    JEnc *j = (JEnc *)self->__sn__ctx;
    je_comma(j);
    char tmp[128];
    snprintf(tmp, sizeof(tmp), "\"%s\":%lld", key, val);
    buf_cat(j->buf, tmp);
}

static void je_write_double(__sn__Encoder *self, const char *key, double val) {
    JEnc *j = (JEnc *)self->__sn__ctx;
    je_comma(j);
    char tmp[128];
    snprintf(tmp, sizeof(tmp), "\"%s\":%g", key, val);
    buf_cat(j->buf, tmp);
}

static void je_write_bool(__sn__Encoder *self, const char *key, long long val) {
    JEnc *j = (JEnc *)self->__sn__ctx;
    je_comma(j);
    char tmp[128];
    snprintf(tmp, sizeof(tmp), "\"%s\":%s", key, val ? "true" : "false");
    buf_cat(j->buf, tmp);
}

static void je_write_null(__sn__Encoder *self, const char *key) {
    JEnc *j = (JEnc *)self->__sn__ctx;
    je_comma(j);
    char tmp[128];
    snprintf(tmp, sizeof(tmp), "\"%s\":null", key);
    buf_cat(j->buf, tmp);
}

static __sn__Encoder *je_make_sub(Buf *buf, int is_array);

static __sn__Encoder *je_begin_object(__sn__Encoder *self, const char *key) {
    JEnc *j = (JEnc *)self->__sn__ctx;
    je_comma(j);
    char tmp[128];
    snprintf(tmp, sizeof(tmp), "\"%s\":{", key);
    buf_cat(j->buf, tmp);
    return je_make_sub(j->buf, 0);
}

static __sn__Encoder *je_begin_array(__sn__Encoder *self, const char *key) {
    JEnc *j = (JEnc *)self->__sn__ctx;
    je_comma(j);
    char tmp[128];
    snprintf(tmp, sizeof(tmp), "\"%s\":[", key);
    buf_cat(j->buf, tmp);
    return je_make_sub(j->buf, 1);
}

static void je_end(__sn__Encoder *self) {
    JEnc *j = (JEnc *)self->__sn__ctx;
    buf_char(j->buf, j->is_array ? ']' : '}');
    free(j);
    free(self);
}

static void je_append_str(__sn__Encoder *self, const char *val) {
    JEnc *j = (JEnc *)self->__sn__ctx;
    je_comma(j);
    buf_char(j->buf, '"');
    for (const char *p = val; *p; p++) {
        if (*p == '"') buf_cat(j->buf, "\\\"");
        else buf_char(j->buf, *p);
    }
    buf_char(j->buf, '"');
}

static void je_append_int(__sn__Encoder *self, long long val) {
    JEnc *j = (JEnc *)self->__sn__ctx;
    je_comma(j);
    char tmp[64];
    snprintf(tmp, sizeof(tmp), "%lld", val);
    buf_cat(j->buf, tmp);
}

static void je_append_double(__sn__Encoder *self, double val) {
    JEnc *j = (JEnc *)self->__sn__ctx;
    je_comma(j);
    char tmp[64];
    snprintf(tmp, sizeof(tmp), "%g", val);
    buf_cat(j->buf, tmp);
}

static void je_append_bool(__sn__Encoder *self, long long val) {
    JEnc *j = (JEnc *)self->__sn__ctx;
    je_comma(j);
    buf_cat(j->buf, val ? "true" : "false");
}

static __sn__Encoder *je_append_object(__sn__Encoder *self) {
    JEnc *j = (JEnc *)self->__sn__ctx;
    je_comma(j);
    buf_char(j->buf, '{');
    return je_make_sub(j->buf, 0);
}

static char *je_result(__sn__Encoder *self) {
    JEnc *j = (JEnc *)self->__sn__ctx;
    buf_char(j->buf, j->is_array ? ']' : '}');
    char *result = strdup(j->buf->data);
    free(j->buf->data);
    free(j->buf);
    free(j);
    self->__sn__ctx = NULL;
    return result;
}

static __sn__EncoderVTable je_vt = {
    .writeStr = je_write_str,
    .writeInt = je_write_int,
    .writeDouble = je_write_double,
    .writeBool = je_write_bool,
    .writeNull = je_write_null,
    .beginObject = je_begin_object,
    .beginArray = je_begin_array,
    .end = je_end,
    .appendStr = je_append_str,
    .appendInt = je_append_int,
    .appendDouble = je_append_double,
    .appendBool = je_append_bool,
    .appendObject = je_append_object,
    .result = je_result,
};

static __sn__Encoder *je_make_sub(Buf *buf, int is_array) {
    __sn__Encoder *enc = calloc(1, sizeof(__sn__Encoder));
    JEnc *j = calloc(1, sizeof(JEnc));
    j->buf = buf;
    j->first = 1;
    j->is_array = is_array;
    j->is_root = 0;
    enc->__sn__vt = &je_vt;
    enc->__sn__ctx = j;
    return enc;
}

__sn__Encoder *sn_test_json_encoder(void) {
    __sn__Encoder *enc = calloc(1, sizeof(__sn__Encoder));
    JEnc *j = calloc(1, sizeof(JEnc));
    j->buf = buf_new();
    j->first = 1;
    j->is_array = 0;
    j->is_root = 1;
    buf_char(j->buf, '{');
    enc->__sn__vt = &je_vt;
    enc->__sn__ctx = j;
    return enc;
}

/* ---- Minimal JSON Decoder ---- */

typedef enum { JN_OBJ, JN_ARR, JN_STR, JN_INT, JN_DOUBLE, JN_BOOL, JN_NULL } JNType;

typedef struct JNode JNode;
typedef struct { char *key; JNode *val; } JKV;

struct JNode {
    JNType type;
    union {
        struct { JKV *items; int count; } obj;
        struct { JNode **items; int count; } arr;
        char *str;
        long long ival;
        double dval;
        int bval;
    };
};

static const char *skip_ws(const char *p) { while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') p++; return p; }

static JNode *jparse(const char **pp);

static char *jparse_string(const char **pp) {
    const char *p = *pp;
    if (*p != '"') return NULL;
    p++;
    Buf *b = buf_new();
    while (*p && *p != '"') {
        if (*p == '\\') {
            p++;
            if (*p == '"') buf_char(b, '"');
            else if (*p == '\\') buf_char(b, '\\');
            else if (*p == 'n') buf_char(b, '\n');
            else if (*p == 't') buf_char(b, '\t');
            else buf_char(b, *p);
        } else {
            buf_char(b, *p);
        }
        p++;
    }
    if (*p == '"') p++;
    *pp = p;
    char *result = strdup(b->data);
    free(b->data);
    free(b);
    return result;
}

static JNode *jparse(const char **pp) {
    const char *p = skip_ws(*pp);
    JNode *n = calloc(1, sizeof(JNode));
    if (*p == '{') {
        n->type = JN_OBJ;
        p++; p = skip_ws(p);
        int cap = 8;
        n->obj.items = malloc(sizeof(JKV) * cap);
        while (*p && *p != '}') {
            if (n->obj.count >= cap) { cap *= 2; n->obj.items = realloc(n->obj.items, sizeof(JKV) * cap); }
            p = skip_ws(p);
            char *key = jparse_string(&p);
            p = skip_ws(p);
            if (*p == ':') p++;
            JNode *val = jparse(&p);
            n->obj.items[n->obj.count++] = (JKV){ key, val };
            p = skip_ws(p);
            if (*p == ',') p++;
        }
        if (*p == '}') p++;
    } else if (*p == '[') {
        n->type = JN_ARR;
        p++; p = skip_ws(p);
        int cap = 8;
        n->arr.items = malloc(sizeof(JNode *) * cap);
        while (*p && *p != ']') {
            if (n->arr.count >= cap) { cap *= 2; n->arr.items = realloc(n->arr.items, sizeof(JNode *) * cap); }
            n->arr.items[n->arr.count++] = jparse(&p);
            p = skip_ws(p);
            if (*p == ',') p++;
        }
        if (*p == ']') p++;
    } else if (*p == '"') {
        n->type = JN_STR;
        n->str = jparse_string(&p);
    } else if (*p == 't') {
        n->type = JN_BOOL; n->bval = 1; p += 4;
    } else if (*p == 'f') {
        n->type = JN_BOOL; n->bval = 0; p += 5;
    } else if (*p == 'n') {
        n->type = JN_NULL; p += 4;
    } else {
        char *end;
        double d = strtod(p, &end);
        int is_int = 1;
        for (const char *c = p; c < end; c++) {
            if (*c == '.' || *c == 'e' || *c == 'E') { is_int = 0; break; }
        }
        if (is_int) { n->type = JN_INT; n->ival = (long long)d; }
        else { n->type = JN_DOUBLE; n->dval = d; }
        p = end;
    }
    *pp = p;
    return n;
}

static JNode *jnode_get(JNode *n, const char *key) {
    if (!n || n->type != JN_OBJ) return NULL;
    for (int i = 0; i < n->obj.count; i++) {
        if (strcmp(n->obj.items[i].key, key) == 0) return n->obj.items[i].val;
    }
    return NULL;
}

typedef struct { JNode *node; } JDec;

static __sn__Decoder *jd_make(JNode *node);

static char *jd_read_str(__sn__Decoder *self, const char *key) {
    JDec *d = (JDec *)self->__sn__ctx;
    JNode *v = jnode_get(d->node, key);
    return (v && v->type == JN_STR) ? strdup(v->str) : strdup("");
}

static long long jd_read_int(__sn__Decoder *self, const char *key) {
    JDec *d = (JDec *)self->__sn__ctx;
    JNode *v = jnode_get(d->node, key);
    return (v && v->type == JN_INT) ? v->ival : 0;
}

static double jd_read_double(__sn__Decoder *self, const char *key) {
    JDec *d = (JDec *)self->__sn__ctx;
    JNode *v = jnode_get(d->node, key);
    if (v && v->type == JN_DOUBLE) return v->dval;
    if (v && v->type == JN_INT) return (double)v->ival;
    return 0.0;
}

static long long jd_read_bool(__sn__Decoder *self, const char *key) {
    JDec *d = (JDec *)self->__sn__ctx;
    JNode *v = jnode_get(d->node, key);
    return (v && v->type == JN_BOOL) ? v->bval : 0;
}

static long long jd_has_key(__sn__Decoder *self, const char *key) {
    JDec *d = (JDec *)self->__sn__ctx;
    return jnode_get(d->node, key) != NULL;
}

static __sn__Decoder *jd_read_object(__sn__Decoder *self, const char *key) {
    JDec *d = (JDec *)self->__sn__ctx;
    JNode *v = jnode_get(d->node, key);
    return jd_make(v);
}

static __sn__Decoder *jd_read_array(__sn__Decoder *self, const char *key) {
    JDec *d = (JDec *)self->__sn__ctx;
    JNode *v = jnode_get(d->node, key);
    return jd_make(v);
}

static long long jd_length(__sn__Decoder *self) {
    JDec *d = (JDec *)self->__sn__ctx;
    if (d->node && d->node->type == JN_ARR) return d->node->arr.count;
    return 0;
}

static __sn__Decoder *jd_at(__sn__Decoder *self, long long index) {
    JDec *d = (JDec *)self->__sn__ctx;
    if (d->node && d->node->type == JN_ARR && index < d->node->arr.count)
        return jd_make(d->node->arr.items[index]);
    return jd_make(NULL);
}

static char *jd_at_str(__sn__Decoder *self, long long index) {
    JDec *d = (JDec *)self->__sn__ctx;
    if (d->node && d->node->type == JN_ARR && index < d->node->arr.count) {
        JNode *v = d->node->arr.items[index];
        if (v->type == JN_STR) return strdup(v->str);
    }
    return strdup("");
}

static long long jd_at_int(__sn__Decoder *self, long long index) {
    JDec *d = (JDec *)self->__sn__ctx;
    if (d->node && d->node->type == JN_ARR && index < d->node->arr.count) {
        JNode *v = d->node->arr.items[index];
        if (v->type == JN_INT) return v->ival;
    }
    return 0;
}

static double jd_at_double(__sn__Decoder *self, long long index) {
    JDec *d = (JDec *)self->__sn__ctx;
    if (d->node && d->node->type == JN_ARR && index < d->node->arr.count) {
        JNode *v = d->node->arr.items[index];
        if (v->type == JN_DOUBLE) return v->dval;
        if (v->type == JN_INT) return (double)v->ival;
    }
    return 0.0;
}

static long long jd_at_bool(__sn__Decoder *self, long long index) {
    JDec *d = (JDec *)self->__sn__ctx;
    if (d->node && d->node->type == JN_ARR && index < d->node->arr.count) {
        JNode *v = d->node->arr.items[index];
        if (v->type == JN_BOOL) return v->bval;
    }
    return 0;
}

static __sn__DecoderVTable jd_vt = {
    .readStr = jd_read_str,
    .readInt = jd_read_int,
    .readDouble = jd_read_double,
    .readBool = jd_read_bool,
    .hasKey = jd_has_key,
    .readObject = jd_read_object,
    .readArray = jd_read_array,
    .length = jd_length,
    .at = jd_at,
    .atStr = jd_at_str,
    .atInt = jd_at_int,
    .atDouble = jd_at_double,
    .atBool = jd_at_bool,
};

static __sn__Decoder *jd_make(JNode *node) {
    __sn__Decoder *dec = calloc(1, sizeof(__sn__Decoder));
    JDec *d = calloc(1, sizeof(JDec));
    d->node = node;
    dec->__sn__vt = &jd_vt;
    dec->__sn__ctx = d;
    return dec;
}

__sn__Decoder *sn_test_json_decoder(const char *input) {
    const char *p = input;
    JNode *root = jparse(&p);
    return jd_make(root);
}

__sn__Encoder *sn_test_json_array_encoder(void) {
    __sn__Encoder *enc = calloc(1, sizeof(__sn__Encoder));
    JEnc *j = calloc(1, sizeof(JEnc));
    j->buf = buf_new();
    j->first = 1;
    j->is_array = 1;
    j->is_root = 1;
    buf_char(j->buf, '[');
    enc->__sn__vt = &je_vt;
    enc->__sn__ctx = j;
    return enc;
}
