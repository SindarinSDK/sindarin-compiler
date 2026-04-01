/* Native helper for sync proxy test — provides millisecond sleep */
#include <time.h>

void sn_test_sleep_ms(long long ms) {
    struct timespec ts;
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (ms % 1000) * 1000000;
    nanosleep(&ts, NULL);
}
