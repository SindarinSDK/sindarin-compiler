bool same_array(SnArray *a, SnArray *b) { return a == b; }
void change_array(SnArray *a) { ((unsigned char *)a->data)[0] = 7; unsigned char next = 9; sn_array_push(a, &next); }
