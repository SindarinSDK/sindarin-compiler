static bool mutate(void *closure, char *value) { (void)closure; value[0]='H'; return true; }
void *make(void) { __Closure__ *header=calloc(1,sizeof(*header));header->fn=(void *)mutate;header->size=sizeof(*header);header->__rc__=1;return header; }
