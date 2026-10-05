/* Count actual C vtable context destruction, including abandoned objects. */
static long long g_serial_live, g_serial_cleanups;
typedef struct { long long value; } SerialContext;
static SerialContext *serial_context(long long value) {
    SerialContext *ctx = malloc(sizeof(*ctx));
    if (!ctx) abort();
    ctx->value = value;
    g_serial_live++;
    return ctx;
}
static void serial_free_context(void *ctx) {
    if (ctx) { free(ctx); g_serial_live--; g_serial_cleanups++; }
}
static void serial_encoder_cleanup(__sn__Encoder *self) {
    serial_free_context(self->__sn__ctx);
    self->__sn__ctx = NULL;
}
static void serial_write_int(__sn__Encoder *self, const char *key, long long value) {
    (void)key;
    ((SerialContext *)self->__sn__ctx)->value = value;
}
static char *serial_result(__sn__Encoder *self) {
    char value[64];
    snprintf(value, sizeof(value), "v=%lld", ((SerialContext *)self->__sn__ctx)->value);
    serial_encoder_cleanup(self);
    return strdup(value);
}
static __sn__EncoderVTable serial_encoder_vt = { .writeInt = serial_write_int, .result = serial_result };
__sn__Encoder *serial_encoder(void) {
    __sn__Encoder *value = __sn__Encoder_alloc();
    value->__sn__vt = &serial_encoder_vt;
    value->__sn__ctx = serial_context(0);
    value->__sn__cleanup = serial_encoder_cleanup;
    return value;
}
static long long serial_read_int(__sn__Decoder *self, const char *key) {
    (void)key;
    return ((SerialContext *)self->__sn__ctx)->value;
}
static void serial_decoder_cleanup(__sn__Decoder *self) {
    serial_free_context(self->__sn__ctx);
    self->__sn__ctx = NULL;
}
static __sn__DecoderVTable serial_decoder_vt = { .readInt = serial_read_int };
__sn__Decoder *serial_decoder(long long input) {
    __sn__Decoder *value = __sn__Decoder_alloc();
    value->__sn__vt = &serial_decoder_vt;
    value->__sn__ctx = serial_context(input);
    value->__sn__cleanup = serial_decoder_cleanup;
    return value;
}
long long serial_read(__sn__Decoder *value) { return serial_read_int(value, "v"); }
long long serial_live(void) { return g_serial_live; }
long long serial_cleanups(void) { return g_serial_cleanups; }
__sn__Encoder *serial_nil_encoder(void) { return NULL; }
__sn__Decoder *serial_nil_decoder(void) { return NULL; }
