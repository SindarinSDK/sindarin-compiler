#ifndef SN_PACKAGE_LINK_H
#define SN_PACKAGE_LINK_H
/* An explicit artifact/option is passed as one linker token. Ordinary library
 * names retain their legacy -l translation and configuration overrides. */
char *package_link_token(const char *value);
#endif
