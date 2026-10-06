bool same_array(SnArray *a, SnArray *b) { return a == b; }
void change_array(SnArray *a) { ((unsigned long long *)a->data)[0] = 7; unsigned long long next = 9; sn_array_push(a, &next); }
