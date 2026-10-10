#include "sn_abi.h"
extern SnAbiStatus sn_go_package_init(SnAbiPackage *, uintptr_t);
extern void sn_go_package_cleanup(SnAbiPackage *, uintptr_t);
SnAbiStatus sn_go_package_new(uintptr_t context, SnAbiPackage **out) {
    return sn_abi_v1_package_new(sn_go_package_init, sn_go_package_cleanup, context, out);
}
