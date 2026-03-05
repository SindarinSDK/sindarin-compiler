/* Helper C functions for test_address_of.sn */

void c_mutate_int(long long *ptr) {
    *ptr = 99;
}

void c_double_value(long long *ptr) {
    *ptr = *ptr * 2;
}
