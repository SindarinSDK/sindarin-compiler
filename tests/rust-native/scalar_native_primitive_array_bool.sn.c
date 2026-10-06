bool same_array(SnArray *a, SnArray *b) { return a == b; }
void change_array(SnArray *a) { ((bool *)a->data)[0] = true; bool next = false; sn_array_push(a, &next); }
