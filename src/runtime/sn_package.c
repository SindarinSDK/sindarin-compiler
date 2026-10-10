#include "sn_abi.h"
#include "sn_core.h"
#include <pthread.h>
#include <stdatomic.h>

enum PackageState { PACKAGE_NEW, PACKAGE_INITIALIZING, PACKAGE_READY,
                    PACKAGE_FAILED, PACKAGE_CLOSING, PACKAGE_CLOSED };
struct SnAbiPackage {
    _Atomic size_t credits;
    pthread_mutex_t mutex;
    pthread_cond_t changed;
    enum PackageState state;
    pthread_t owner;
    size_t active_calls;
    SnAbiStatus failure;
    SnAbiPackageInit initialize;
    SnAbiPackageCleanup cleanup;
    uintptr_t context;
};
struct SnAbiPackageCall {
    SnAbiPackage *package;
    pthread_t owner;
    struct SnAbiPackageCall *next;
};
static _Thread_local SnAbiPackageCall *package_calls;

static bool active_on_this_thread(SnAbiPackage *package)
{
    for (SnAbiPackageCall *call = package_calls; call; call = call->next)
        if (call->package == package) return true;
    return false;
}

SnAbiStatus sn_abi_v1_package_new(SnAbiPackageInit initialize, SnAbiPackageCleanup cleanup,
                                 uintptr_t context, SnAbiPackage **out)
{
    if (!out) return SN_ABI_INVALID_ARGUMENT;
    SnAbiPackage *package = sn_calloc(1, sizeof(*package));
    atomic_init(&package->credits, 1);
    if (pthread_mutex_init(&package->mutex, NULL)) { free(package); return SN_ABI_FOREIGN_ERROR; }
    if (pthread_cond_init(&package->changed, NULL)) {
        pthread_mutex_destroy(&package->mutex); free(package); return SN_ABI_FOREIGN_ERROR;
    }
    package->initialize = initialize; package->cleanup = cleanup; package->context = context;
    *out = package;
    return SN_ABI_OK;
}

SnAbiPackage *sn_abi_v1_package_retain(SnAbiPackage *package)
{
    if (!package) return NULL;
    size_t credits = atomic_load_explicit(&package->credits, memory_order_relaxed);
    for (;;) {
        if (!credits || credits == SIZE_MAX) abort();
        if (atomic_compare_exchange_weak_explicit(&package->credits, &credits, credits + 1,
                memory_order_relaxed, memory_order_relaxed)) return package;
    }
}

SnAbiStatus sn_abi_v1_package_begin(SnAbiPackage *package, SnAbiPackageCall **out)
{
    if (!package || !out) return SN_ABI_INVALID_ARGUMENT;
    pthread_mutex_lock(&package->mutex);
    while (package->state == PACKAGE_INITIALIZING && !pthread_equal(package->owner, pthread_self()))
        pthread_cond_wait(&package->changed, &package->mutex);
    if (package->state == PACKAGE_NEW) {
        package->state = PACKAGE_INITIALIZING; package->owner = pthread_self();
        pthread_mutex_unlock(&package->mutex);
        SnAbiStatus status = package->initialize ? package->initialize(package, package->context) : SN_ABI_OK;
        pthread_mutex_lock(&package->mutex);
        package->failure = status;
        package->state = status ? PACKAGE_FAILED : PACKAGE_READY;
        pthread_cond_broadcast(&package->changed);
    }
    if (package->state == PACKAGE_FAILED || package->state >= PACKAGE_CLOSING) {
        SnAbiStatus status = package->state == PACKAGE_FAILED ? package->failure : SN_ABI_PACKAGE_CLOSED;
        pthread_mutex_unlock(&package->mutex); return status;
    }
    if (package->active_calls == SIZE_MAX) abort();
    SnAbiPackageCall *call = sn_malloc(sizeof(*call));
    call->package = sn_abi_v1_package_retain(package); call->owner = pthread_self();
    call->next = package_calls; package_calls = call; package->active_calls++;
    pthread_mutex_unlock(&package->mutex);
    *out = call;
    return SN_ABI_OK;
}

void sn_abi_v1_package_end(SnAbiPackageCall *call)
{
    if (!call) return;
    if (!pthread_equal(call->owner, pthread_self())) abort();
    SnAbiPackageCall **slot = &package_calls;
    while (*slot && *slot != call) slot = &(*slot)->next;
    if (!*slot) abort();
    *slot = call->next;
    SnAbiPackage *package = call->package;
    pthread_mutex_lock(&package->mutex);
    package->active_calls--;
    if (!package->active_calls) pthread_cond_broadcast(&package->changed);
    pthread_mutex_unlock(&package->mutex);
    free(call); sn_abi_v1_package_release(package);
}

SnAbiStatus sn_abi_v1_package_shutdown(SnAbiPackage *package)
{
    if (!package) return SN_ABI_OK;
    pthread_mutex_lock(&package->mutex);
    if (active_on_this_thread(package) ||
        (package->state == PACKAGE_INITIALIZING && pthread_equal(package->owner, pthread_self()))) {
        pthread_mutex_unlock(&package->mutex); return SN_ABI_PACKAGE_BUSY;
    }
    while (package->state == PACKAGE_INITIALIZING || package->state == PACKAGE_CLOSING) {
        if (package->state == PACKAGE_CLOSING && pthread_equal(package->owner, pthread_self())) {
            pthread_mutex_unlock(&package->mutex); return SN_ABI_OK;
        }
        pthread_cond_wait(&package->changed, &package->mutex);
    }
    if (package->state == PACKAGE_CLOSED) { pthread_mutex_unlock(&package->mutex); return SN_ABI_OK; }
    bool initialized = package->state == PACKAGE_READY;
    package->state = PACKAGE_CLOSING; package->owner = pthread_self();
    while (package->active_calls) pthread_cond_wait(&package->changed, &package->mutex);
    pthread_mutex_unlock(&package->mutex);
    if (initialized && package->cleanup) package->cleanup(package, package->context);
    pthread_mutex_lock(&package->mutex);
    package->state = PACKAGE_CLOSED;
    pthread_cond_broadcast(&package->changed);
    pthread_mutex_unlock(&package->mutex);
    return SN_ABI_OK;
}

void sn_abi_v1_package_release(SnAbiPackage *package)
{
    if (!package || atomic_fetch_sub_explicit(&package->credits, 1, memory_order_acq_rel) != 1) return;
    if (sn_abi_v1_package_shutdown(package)) abort();
    pthread_cond_destroy(&package->changed); pthread_mutex_destroy(&package->mutex); free(package);
}
