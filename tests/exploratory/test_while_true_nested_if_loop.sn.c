#include <unistd.h>

void sn_sleep_ms(long long ms) {
    usleep((unsigned int)(ms * 1000));
}
