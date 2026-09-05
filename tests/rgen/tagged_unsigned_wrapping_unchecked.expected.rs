#![allow(dead_code, unused_mut, unused_variables, unused_parens)]

fn main() {
    let mut u32_value: u32 = 4294967295;
    println!("{}", { let __sn_byte_place = &mut (u32_value); let __sn_byte_previous = *__sn_byte_place; *__sn_byte_place = __sn_byte_previous.wrapping_add(1); __sn_byte_previous });
    println!("{}", u32_value);
    { let (__sn_byte_rhs, __sn_byte_place): (u32, &mut u32) = (1, &mut (u32_value)); let __sn_byte_next = (*__sn_byte_place).wrapping_sub(__sn_byte_rhs); *__sn_byte_place = __sn_byte_next; __sn_byte_next };
    println!("{}", u32_value);
    let mut u_half: u64 = 9223372036854775807;
    let mut u_value: u64 = { let (__sn_byte_left, __sn_byte_right): (u64, u64) = ({ let (__sn_byte_left, __sn_byte_right): (u64, u64) = (u_half, 2); __sn_byte_left.wrapping_mul(__sn_byte_right) }, 1); __sn_byte_left.wrapping_add(__sn_byte_right) };
    println!("{}", ({ let __sn_byte_place = &mut (u_value); let __sn_byte_previous = *__sn_byte_place; *__sn_byte_place = __sn_byte_previous.wrapping_add(1); __sn_byte_previous } as i64));
    println!("{}", (u_value as i64));
    { let (__sn_byte_rhs, __sn_byte_place): (u64, &mut u64) = (1, &mut (u_value)); let __sn_byte_next = (*__sn_byte_place).wrapping_sub(__sn_byte_rhs); *__sn_byte_place = __sn_byte_next; __sn_byte_next };
    println!("{}", (u_value as i64));
}
