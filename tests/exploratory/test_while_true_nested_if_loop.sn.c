#ifdef _WIN32
#include <windows.h>
void sn_sleep_ms(long long ms) {
    Sleep((DWORD)ms);
}
#else
#include <unistd.h>
void sn_sleep_ms(long long ms) {
    usleep((unsigned int)(ms * 1000));
}
#endif
