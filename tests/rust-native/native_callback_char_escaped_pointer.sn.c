static char *saved;
bool remember(char *value, __Closure__ *read) { saved=value; return ((char (*)(void *))read->fn)(read) == *value; }
char read_saved(void) { return *saved; }
void clear_saved(void) { saved=NULL; }
