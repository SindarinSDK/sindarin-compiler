char *mutate(SnArray *values) { ((char **)values->data)[0][0]='b'; return strdup("b"); }
