static __Closure__ *saved;
void remember(__Closure__ *read) { if(saved) sn_closure_release((void **)&saved); saved=sn_closure_retain(read); }
long long invoke_saved(void) { return ((long long (*)(void *))saved->fn)(saved); }
void clear_saved(void) { sn_closure_release((void **)&saved); }
