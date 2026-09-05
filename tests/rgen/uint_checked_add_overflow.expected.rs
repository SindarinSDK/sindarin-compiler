#![allow(dead_code, unused_mut, unused_variables, unused_parens)]

fn main() {
    let mut half: u64 = 9223372036854775807;
    let mut two: u64 = 2;
    let mut one: u64 = 1;
    let mut max_minus_one: u64 = { let (__sn_byte_left, __sn_byte_right): (u64, u64) = (half, two); __sn_byte_left.wrapping_mul(__sn_byte_right) };
    let mut max: u64 = { let (__sn_byte_left, __sn_byte_right): (u64, u64) = (max_minus_one, one); __sn_byte_left.wrapping_add(__sn_byte_right) };
}
