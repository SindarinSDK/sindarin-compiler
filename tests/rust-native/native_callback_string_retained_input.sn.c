static char *saved;
static bool remember(void *closure, char *value) { (void)closure; saved=value; return value!=NULL; }
void *make(void) { __Closure__ *header=calloc(1,sizeof(*header));header->fn=(void *)remember;header->size=sizeof(*header);header->__rc__=1;return header; }
bool same_saved(char *value) { return saved==value; }
void clear_saved(void) { saved=NULL; }
