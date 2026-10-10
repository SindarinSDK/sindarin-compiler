/* Inputs are finite and truncate into int64_t; C defines no out-of-range result. */
long long double_integer_reference(double value) { return (long long)value; }
