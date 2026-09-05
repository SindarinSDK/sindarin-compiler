static long long first_value = 11;
static long long second_value = 22;
static char character_value = 'Z';
static unsigned char raw_value = 0xff;
static void *opaque_value = &first_value;
static long long call_count;

long long *int_slot(void) {
    call_count++;
    return &first_value;
}

long long *second_int_slot(void) { return &second_value; }
char *char_slot(void) { return &character_value; }
void *void_slot(void) { return &raw_value; }
void **opaque_slot(void) { return &opaque_value; }
long long pointer_calls(void) { return call_count; }

long long *pass_int(long long *value) { return value; }
char *pass_char(char *value) { return value; }
void *pass_void(void *value) { return value; }
void **pass_opaque(void **value) { return value; }

bool same_int(long long *left, long long *right) { return left == right; }
bool same_char(char *left, char *right) { return left == right; }
bool same_void(void *left, void *right) { return left == right; }
bool same_opaque(void **left, void **right) { return left == right; }
