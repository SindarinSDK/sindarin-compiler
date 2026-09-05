static uint32_t recorded_value;

long long c_int(long long value) { return value; }
long long c_long(long long value) { return value; }
int32_t c_int32(int32_t value) { return value; }
uint64_t c_uint(uint64_t value) { return value; }
uint32_t c_uint32(uint32_t value) { return value; }
unsigned char c_byte(unsigned char value) { return value; }
float c_float(float value) { return value; }
double c_double(double value) { return value; }
void c_record(uint32_t value) { recorded_value = value; }
uint32_t c_recorded(void) { return recorded_value; }
