bool same_array(SnArray *a, SnArray *b) { return a == b; }
void change_array(SnArray *a) { ((int *)a->data)[0] = 7; int next = 9; sn_array_push(a, &next); }
