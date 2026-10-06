bool same_array(SnArray *a, SnArray *b) { return a == b; }
void change_array(SnArray *a) { ((unsigned int *)a->data)[0] = 7; unsigned int next = 9; sn_array_push(a, &next); }
