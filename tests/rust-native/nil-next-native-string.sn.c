char *absent(void) { return NULL; }
char *empty(void) { char *value = malloc(1); value[0] = 0; return value; }
bool is_absent(char *value) { return value == NULL; }
