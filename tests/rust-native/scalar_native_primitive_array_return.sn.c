SnArray *make_array(void) { SnArray *a = sn_array_new(sizeof(long long), 0); long long x=4,y=5; sn_array_push(a,&x); sn_array_push(a,&y); return a; }
