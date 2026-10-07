static SnArray *apply(void *closure, SnArray *values) { (void)closure; char *next=strdup("b");sn_array_push(values,&next);return sn_array_copy(values); }
void *make(void) { __Closure__ *header=calloc(1,sizeof(*header));header->fn=(void *)apply;header->size=sizeof(*header);header->__rc__=1;return header; }
