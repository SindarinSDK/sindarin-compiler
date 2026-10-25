#ifndef SN_PACKAGE_LINK_H
#define SN_PACKAGE_LINK_H
#include <stdbool.h>
/* An explicit artifact/option is passed as one linker token. Ordinary library
 * names retain their legacy -l translation and configuration overrides. */
char *package_link_token(const char *value);
/* Keep explicit option/archive ordering, then resolve named dependencies after
 * package archives. Legacy invocations without archives retain their order. */
bool package_link_defer_named(const char *const *values, int count);
bool package_link_named(const char *value);
#endif
