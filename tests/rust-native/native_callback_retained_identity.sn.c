static __Closure__ *saved;
void remember(__Closure__ *read) { if(saved) sn_closure_release((void **)&saved); saved=sn_closure_retain(read); }
bool same_saved(__Closure__ *read) { return saved == read; }
void clear_saved(void) { sn_closure_release((void **)&saved); }
