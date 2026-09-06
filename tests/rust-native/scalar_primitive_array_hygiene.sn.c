long long collide(SnArray *values, long long left, long long right) {
    ((long long *)values->data)[0] += left + right - 1;
    long long added = 6;
    sn_array_push(values, &added);
    return ((long long *)values->data)[0] + left + right;
}
