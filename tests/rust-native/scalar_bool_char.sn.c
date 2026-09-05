#include <stdbool.h>
#include <stdint.h>

bool cBoolIdentity(bool value) { return value; }
unsigned char cBoolBits(bool value) { return (unsigned char)value; }
uint32_t cBoolSize(void) { return (uint32_t)sizeof(bool); }
char cCharIdentity(char value) { return value; }
int32_t cCharPromotion(char value) { return (int32_t)value; }
int32_t cExpectedHighPromotion(void) { return (int32_t)(char)0xff; }
uint32_t cCharSize(void) { return (uint32_t)sizeof(char); }
