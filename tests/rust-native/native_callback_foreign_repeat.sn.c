static __Closure__ *saved;
static long long apply(void *closure, long long value) { (void)closure; return value+7; }
void initialize(void) { saved=calloc(1,sizeof(*saved)); saved->fn=(void *)apply; saved->size=sizeof(*saved); saved->__rc__=1; }
void *get(void) { return sn_closure_retain(saved); }
void clear(void) { sn_closure_release((void **)&saved); }
