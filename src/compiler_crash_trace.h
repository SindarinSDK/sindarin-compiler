#ifndef SN_COMPILER_CRASH_TRACE_H
#define SN_COMPILER_CRASH_TRACE_H

/* Diagnostic-only tracing records the actual faulting compiler process. */
#if defined(__APPLE__) || defined(__linux__)
#include <execinfo.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void compiler_crash_trace(int signal_number)
{
    static const char heading[] = "\nSindarin compiler fatal-signal backtrace:\n";
    void *frames[64];
    (void)write(STDERR_FILENO, heading, sizeof(heading) - 1);
    int count = backtrace(frames, 64);
    backtrace_symbols_fd(frames, count, STDERR_FILENO);
    /* SA_RESETHAND restores the default action. Returning delivers this pending
     * signal with its original fatal status rather than converting it to a pass. */
    (void)kill(getpid(), signal_number);
}

static void compiler_install_crash_trace(void)
{
    const char *enabled = getenv("SN_COMPILER_BACKTRACE");
    if (!enabled || strcmp(enabled, "1") != 0) return;
    void *warmup[1];
    (void)backtrace(warmup, 1);
    struct sigaction action;
    memset(&action, 0, sizeof(action));
    action.sa_handler = compiler_crash_trace;
    action.sa_flags = SA_RESETHAND;
    sigemptyset(&action.sa_mask);
    (void)sigaction(SIGSEGV, &action, NULL);
    (void)sigaction(SIGABRT, &action, NULL);
}
#else
static void compiler_install_crash_trace(void) { }
#endif

#endif
