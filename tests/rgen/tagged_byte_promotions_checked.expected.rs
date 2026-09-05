#![allow(dead_code, unused_mut, unused_variables, unused_parens)]

fn main() {
    println!("0x{:02X}", ({ let (__sn_byte_left, __sn_byte_right): (u8, u8) = (255, 1); __sn_byte_left.wrapping_add(__sn_byte_right) } as u32));
    println!("0x{:02X}", (-(1 as i32) as u32));
    println!("0x{:02X}", (!(1 as i32) as u32));
    println!("{}", ({ let (__sn_byte_left, __sn_byte_right): (u8, u8) = (255, 1); __sn_byte_left.wrapping_add(__sn_byte_right) } == 0));
    let mut stored: u8 = ({ let (__sn_byte_left, __sn_byte_right): (u8, u8) = (255, 1); __sn_byte_left.wrapping_add(__sn_byte_right) }).checked_div(2).expect("checked arithmetic failed");
    println!("0x{:02X}", (stored as u32));
}
