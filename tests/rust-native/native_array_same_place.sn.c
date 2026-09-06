void mutate_pair(SnArray *first, SnArray *second) {
    ((long long *)first->data)[0] = 5;
    ((long long *)second->data)[1] =
        first == second && ((long long *)second->data)[0] == 5 ? 6 : 7;
}
