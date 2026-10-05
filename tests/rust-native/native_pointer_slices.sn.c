/* Backing byte memory is deliberately independent from the owned slice. */
static unsigned char pointer_bytes[8] = {65, 255, 0, 128, 10, 13, 66, 67};
static long long pointer_trace, pointer_live;
static long long pointer_calls, start_calls, end_calls;
static unsigned char *owned_bytes;
unsigned char *slice_pointer(void) { pointer_calls++; pointer_trace = pointer_trace * 10 + 1; return pointer_bytes; }
long long slice_start(void) { start_calls++; pointer_trace = pointer_trace * 10 + 2; return 1; }
long long slice_end(void) { end_calls++; pointer_trace = pointer_trace * 10 + 3; return 5; }
long long slice_order(void) { long long trace = pointer_trace; pointer_trace = 0; return trace; }
unsigned char *slice_base(void) { return pointer_bytes; }
unsigned char *slice_middle(void) { return pointer_bytes + 3; }
unsigned char *slice_owned(void) {
    unsigned char *value = malloc(sizeof(pointer_bytes));
    if (!value) abort();
    memcpy(value, pointer_bytes, sizeof(pointer_bytes));
    pointer_live++;
    owned_bytes = value;
    return value;
}
void slice_mutate(unsigned char *pointer) { pointer[1] = 7; }
void slice_dispose(unsigned char *pointer) { if (pointer) { free(pointer); pointer_live--; } }
long long slice_alive(void) { return pointer_live; }
void slice_mutate_owned(void) { slice_mutate(owned_bytes); }
void slice_dispose_owned(void) { slice_dispose(owned_bytes); owned_bytes = NULL; }
unsigned char *slice_nil(void) { return NULL; }
bool slice_once(void) { return pointer_calls == 1 && start_calls == 1 && end_calls == 1; }
long long slice_char_span(char value) { return 300 - (long long)value; }
