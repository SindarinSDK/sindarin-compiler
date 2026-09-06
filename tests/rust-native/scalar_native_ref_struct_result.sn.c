static long long tracker_alive = 0;

__sn__Tracker *tracker_create(long long id) {
    __sn__Tracker *value = calloc(1, sizeof(*value));
    value->__rc__ = 1;
    value->__sn__id = id;
    tracker_alive++;
    return value;
}

void tracker_dispose(__sn__Tracker *value) {
    (void)value;
    tracker_alive--;
}

long long tracker_get_id(__sn__Tracker *value) {
    return value->__sn__id;
}

bool tracker_same(__sn__Tracker *first, __sn__Tracker *second) {
    return first == second;
}

long long tracker_alive_count(void) {
    return tracker_alive;
}
