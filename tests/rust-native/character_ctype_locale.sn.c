#include <ctype.h>
#include <locale.h>

char ctype_value(long long value) { return (char)(unsigned char)value; }
long long ctype_reference(long long value, long long operation) {
    unsigned char byte = (unsigned char)value;
    switch (operation) {
        case 0: return (long long)(char)byte;
        case 1: return (long long)(char)toupper(byte);
        case 2: return (long long)(char)tolower(byte);
        case 3: return isdigit(byte) != 0;
        case 4: return isalpha(byte) != 0;
        case 5: return isspace(byte) != 0;
        case 6: return isalnum(byte) != 0;
        default: return -999;
    }
}
/* Both locales are required; a missing environment locale fails the fixture. */
bool ctype_locale(long long value) {
    return setlocale(LC_CTYPE, value == 0 ? "C" : "") != NULL;
}
