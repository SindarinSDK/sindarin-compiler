static __sn__Record *remembered_record;
void remember_record(__sn__Record *value) { remembered_record = value; }
__sn__Record saved_record(void) { return *remembered_record; }
unsigned char char_bits(char value) { return (unsigned char)value; }
