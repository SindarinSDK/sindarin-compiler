static pthread_mutex_t gate=PTHREAD_MUTEX_INITIALIZER;
long long invoke(__Closure__ *action, long long value) {
 pthread_mutex_lock(&gate); sn_closure_retain(action); pthread_mutex_unlock(&gate);
 long long result=((long long (*)(void *,long long))action->fn)(action,value);
 pthread_mutex_lock(&gate); void *owned=action; sn_closure_release(&owned); pthread_mutex_unlock(&gate);
 return result;
}
