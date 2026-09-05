#![allow(dead_code, unused_mut, unused_variables, unused_parens)]

fn main() {
    let mut i_max: i32 = 2147483647;
    println!("{}", { let (__sn_byte_left, __sn_byte_right): (i32, i32) = (i_max, 1); __sn_byte_left.wrapping_add(__sn_byte_right) });
    let mut u32_max: u32 = 4294967295;
    let mut u32_one: u32 = 1;
    println!("{}", { let (__sn_byte_left, __sn_byte_right): (u32, u32) = (u32_max, 1); __sn_byte_left.wrapping_add(__sn_byte_right) });
    println!("{}", { let (__sn_byte_left, __sn_byte_right): (u32, u32) = (0, 1); __sn_byte_left.wrapping_sub(__sn_byte_right) });
    println!("{}", { let (__sn_byte_left, __sn_byte_right): (u32, u32) = (65536, 65536); __sn_byte_left.wrapping_mul(__sn_byte_right) });
    println!("{}", { let __sn_byte_operand: u32 = u32_one; __sn_byte_operand.wrapping_neg() });
    let mut u_half: u64 = 9223372036854775807;
    let mut u_max: u64 = { let (__sn_byte_left, __sn_byte_right): (u64, u64) = ({ let (__sn_byte_left, __sn_byte_right): (u64, u64) = (u_half, 2); __sn_byte_left.wrapping_mul(__sn_byte_right) }, 1); __sn_byte_left.wrapping_add(__sn_byte_right) };
    println!("{}", ({ let (__sn_byte_left, __sn_byte_right): (u64, u64) = (u_max, 1); __sn_byte_left.wrapping_add(__sn_byte_right) } as i64));
    println!("{}", ({ let (__sn_byte_left, __sn_byte_right): (u64, u64) = (0, 1); __sn_byte_left.wrapping_sub(__sn_byte_right) } as i64));
}
