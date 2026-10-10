#include "sn_abi.h"
#include <assert.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>

static atomic_int initialized, cleaned, entered;
static SnAbiStatus initialize(SnAbiPackage *package, uintptr_t context)
{
    assert(context == 42);
    assert(atomic_fetch_add(&initialized, 1) == 0);
    SnAbiPackageCall *nested = NULL;
    assert(sn_abi_v1_package_begin(package, &nested) == 0);
    assert(sn_abi_v1_package_shutdown(package) == SN_ABI_PACKAGE_BUSY);
    sn_abi_v1_package_end(nested);
    return 0;
}
static void cleanup(SnAbiPackage *package, uintptr_t context)
{
    assert(context == 42 && atomic_fetch_add(&cleaned, 1) == 0);
    SnAbiPackageCall *preserved = (SnAbiPackageCall *)(uintptr_t)1;
    assert(sn_abi_v1_package_begin(package, &preserved) == SN_ABI_PACKAGE_CLOSED);
    assert(preserved == (SnAbiPackageCall *)(uintptr_t)1);
    assert(sn_abi_v1_package_shutdown(package) == 0);
}
static void *worker(void *raw)
{
    SnAbiPackageCall *call = NULL;
    assert(sn_abi_v1_package_begin(raw, &call) == 0);
    assert(atomic_load(&initialized) == 1);
    atomic_fetch_add(&entered, 1);
    sn_abi_v1_package_end(call);
    return NULL;
}
static void *closer(void *raw)
{
    assert(sn_abi_v1_package_shutdown(raw) == 0);
    return NULL;
}
static SnAbiStatus failed(SnAbiPackage *package, uintptr_t context)
{
    (void)package; (void)context;
    atomic_fetch_add(&initialized, 1);
    return 13;
}
int main(void)
{
    SnAbiInfo info;
    assert(sn_abi_v1_query(SN_ABI_V1_2_VERSION, SN_ABI_CAP_PACKAGE_LIFECYCLE, &info, sizeof(info)) == 0);
    assert(info.capabilities == 63);
    assert(sn_abi_v1_query(SN_ABI_V1_1_VERSION, SN_ABI_CAP_PACKAGE_LIFECYCLE, &info, sizeof(info)) == SN_ABI_UNSUPPORTED);
    assert(sn_abi_v1_package_new(initialize, cleanup, 42, NULL) == SN_ABI_INVALID_ARGUMENT);
    assert(sn_abi_v1_package_begin(NULL, NULL) == SN_ABI_INVALID_ARGUMENT);
    sn_abi_v1_package_end(NULL); sn_abi_v1_package_release(NULL);
    assert(sn_abi_v1_package_shutdown(NULL) == 0);
    SnAbiPackage *package = NULL;
    assert(sn_abi_v1_package_new(initialize, cleanup, 42, &package) == 0);
    SnAbiPackage *alias = sn_abi_v1_package_retain(package);
    pthread_t threads[16];
    for (int i = 0; i < 16; i++) assert(!pthread_create(&threads[i], NULL, worker, package));
    for (int i = 0; i < 16; i++) assert(!pthread_join(threads[i], NULL));
    assert(atomic_load(&initialized) == 1 && atomic_load(&entered) == 16);
    SnAbiPackageCall *active = NULL;
    assert(sn_abi_v1_package_begin(package, &active) == 0);
    assert(sn_abi_v1_package_shutdown(package) == SN_ABI_PACKAGE_BUSY);
    pthread_t closing;
    assert(!pthread_create(&closing, NULL, closer, package));
    for (;;) {
        SnAbiPackageCall *probe = NULL;
        SnAbiStatus status = sn_abi_v1_package_begin(package, &probe);
        if (status == SN_ABI_PACKAGE_CLOSED) break;
        assert(status == 0); sn_abi_v1_package_end(probe);
    }
    assert(atomic_load(&cleaned) == 0);
    sn_abi_v1_package_end(active);
    assert(!pthread_join(closing, NULL));
    assert(atomic_load(&cleaned) == 1);
    assert(sn_abi_v1_package_shutdown(package) == 0);
    sn_abi_v1_package_release(alias); sn_abi_v1_package_release(package);
    assert(atomic_load(&cleaned) == 1);
    atomic_store(&initialized, 0);
    assert(sn_abi_v1_package_new(failed, cleanup, 42, &package) == 0);
    SnAbiPackageCall *preserved = (SnAbiPackageCall *)(uintptr_t)1;
    assert(sn_abi_v1_package_begin(package, &preserved) == 13);
    assert(sn_abi_v1_package_begin(package, &preserved) == 13);
    assert(preserved == (SnAbiPackageCall *)(uintptr_t)1 && atomic_load(&initialized) == 1);
    sn_abi_v1_package_release(package); assert(atomic_load(&cleaned) == 1);
    atomic_store(&initialized, 0); atomic_store(&cleaned, 0);
    assert(sn_abi_v1_package_new(initialize, cleanup, 42, &package) == 0);
    assert(sn_abi_v1_package_begin(package, &active) == 0);
    sn_abi_v1_package_release(package); /* Active call holds the final credit. */
    assert(atomic_load(&cleaned) == 0); sn_abi_v1_package_end(active);
    assert(atomic_load(&cleaned) == 1);
    puts("package lifecycle: pass");
    return 0;
}
