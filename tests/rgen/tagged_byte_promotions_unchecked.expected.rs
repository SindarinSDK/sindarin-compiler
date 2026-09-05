#![allow(dead_code, unused_mut, unused_variables, unused_parens)]

fn main() {
    println!("0x{:02X}", ({ let (__sn_byte_left, __sn_byte_right): (i32, i32) = (255 as i32, 1 as i32); __sn_byte_left + __sn_byte_right } as u32));
    println!("0x{:02X}", (-(1 as i32) as u32));
    println!("0x{:02X}", (!(1 as i32) as u32));
    println!("{}", ({ let (__sn_byte_left, __sn_byte_right): (i32, i32) = (255 as i32, 1 as i32); __sn_byte_left + __sn_byte_right } as i32 == 0 as i32));
    let mut stored: u8 = { let (__sn_byte_left, __sn_byte_right): (i32, i32) = ({ let (__sn_byte_left, __sn_byte_right): (i32, i32) = (255 as i32, 1 as i32); __sn_byte_left + __sn_byte_right } as i32, 2 as i32); __sn_byte_left / __sn_byte_right } as u8;
    println!("0x{:02X}", (stored as u32));
}
