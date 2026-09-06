__sn__Tracker *tracker_create(long long id) {
    __sn__Tracker *value = calloc(1, sizeof(*value));
    value->__rc__ = 1;
    value->__sn__id = id;
    return value;
}

void tracker_dispose(__sn__Tracker *value) {
    (void)value;
    printf("dispose\n");
}
