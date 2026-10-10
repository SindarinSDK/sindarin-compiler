/* Observe the existing C conversion contract, including host char signedness. */
char primitive_char_reference(long long value) { return (char)value; }
