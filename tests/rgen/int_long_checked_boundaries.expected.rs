#![allow(dead_code, unused_mut, unused_variables, unused_parens)]

fn main() {
    let mut int_max_minus_one: i64 = 9223372036854775806;
    let mut int_one: i64 = 1;
    let mut int_max: i64 = (int_max_minus_one).checked_add(int_one).expect("checked arithmetic failed");
    let mut int_min_base: i64 = (-9223372036854775807);
    let mut int_min: i64 = (int_min_base).checked_sub(int_one).expect("checked arithmetic failed");
    let mut int_mul_base: i64 = 4611686018427387903;
    let mut int_two: i64 = 2;
    let mut int_product: i64 = (int_mul_base).checked_mul(int_two).expect("checked arithmetic failed");
    let mut int_quotient: i64 = (int_min).checked_div(int_one).expect("checked arithmetic failed");
    let mut int_remainder: i64 = (int_min).checked_rem(int_one).expect("checked arithmetic failed");
    let mut long_max_minus_one: i64 = 9223372036854775806;
    let mut long_one: i64 = 1;
    let mut long_max: i64 = (long_max_minus_one).checked_add(long_one).expect("checked arithmetic failed");
    let mut long_min_base: i64 = (-9223372036854775807);
    let mut long_min: i64 = (long_min_base).checked_sub(long_one).expect("checked arithmetic failed");
    let mut long_mul_base: i64 = 4611686018427387903;
    let mut long_two: i64 = 2;
    let mut long_product: i64 = (long_mul_base).checked_mul(long_two).expect("checked arithmetic failed");
    let mut long_quotient: i64 = (long_min).checked_div(long_one).expect("checked arithmetic failed");
    let mut long_remainder: i64 = (long_min).checked_rem(long_one).expect("checked arithmetic failed");
    if ((((((((((int_max == 9223372036854775807) && ((int_min).checked_add(int_one).expect("checked arithmetic failed") == int_min_base)) && (int_product == int_max_minus_one)) && (int_quotient == int_min)) && (int_remainder == 0)) && (long_max == 9223372036854775807)) && ((long_min).checked_add(long_one).expect("checked arithmetic failed") == long_min_base)) && (long_product == long_max_minus_one)) && (long_quotient == long_min)) && (long_remainder == 0)) {
        println!("{}", "ok".to_string());
    } else {
        println!("{}", "wrong".to_string());
    }
}
