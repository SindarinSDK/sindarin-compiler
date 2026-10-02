static const char *saved;

void remember(char *value) {
    saved = value;
}

char saved_first(void) {
    return saved[0];
}
