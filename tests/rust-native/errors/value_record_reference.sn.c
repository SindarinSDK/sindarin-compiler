static __sn__Record *remembered_record;
void remember_record(__sn__Record *value) { remembered_record = value; }
long long read_record(void) { return remembered_record->__sn__count; }
