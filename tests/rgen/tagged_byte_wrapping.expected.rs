#![allow(dead_code, unused_mut, unused_variables, unused_parens)]

#[derive(Clone, Copy, Debug, PartialEq)]
struct ByteBox {
    value: u8,
}

fn bump(value: &mut u8) -> u8 {
    let mut one: u8 = 1;
    return { let (__sn_byte_rhs, __sn_byte_place): (u8, &mut u8) = (one, &mut (*(value))); let __sn_byte_next = (*__sn_byte_place).wrapping_add(__sn_byte_rhs); *__sn_byte_place = __sn_byte_next; __sn_byte_next };
}

fn main() {
    let mut max: u8 = 255;
    let mut zero: u8 = 0;
    let mut one: u8 = 1;
    let mut two: u8 = 2;
    let mut added: u8 = { let (__sn_byte_left, __sn_byte_right): (u8, u8) = (max, one); __sn_byte_left.wrapping_add(__sn_byte_right) };
    let mut subtracted: u8 = { let (__sn_byte_left, __sn_byte_right): (u8, u8) = (zero, one); __sn_byte_left.wrapping_sub(__sn_byte_right) };
    let mut multiplied: u8 = { let (__sn_byte_left, __sn_byte_right): (u8, u8) = (max, two); __sn_byte_left.wrapping_mul(__sn_byte_right) };
    let mut negated: u8 = -(one as i32) as u8;
    let mut inverted: u8 = !(zero as i32) as u8;
    println!("{}", (added == 0));
    println!("{}", (subtracted == 255));
    println!("{}", (multiplied == 254));
    println!("{}", (negated == 255));
    println!("{}", (inverted == 255));
    let mut add: u8 = max;
    let mut sub: u8 = zero;
    let mut mul: u8 = max;
    let mut add_result: u8 = { let (__sn_byte_rhs, __sn_byte_place): (u8, &mut u8) = (one, &mut (add)); let __sn_byte_next = (*__sn_byte_place).wrapping_add(__sn_byte_rhs); *__sn_byte_place = __sn_byte_next; __sn_byte_next };
    let mut sub_result: u8 = { let (__sn_byte_rhs, __sn_byte_place): (u8, &mut u8) = (one, &mut (sub)); let __sn_byte_next = (*__sn_byte_place).wrapping_sub(__sn_byte_rhs); *__sn_byte_place = __sn_byte_next; __sn_byte_next };
    let mut mul_result: u8 = { let (__sn_byte_rhs, __sn_byte_place): (u8, &mut u8) = (two, &mut (mul)); let __sn_byte_next = (*__sn_byte_place).wrapping_mul(__sn_byte_rhs); *__sn_byte_place = __sn_byte_next; __sn_byte_next };
    println!("{}", (add_result == 0));
    println!("{}", (sub_result == 255));
    println!("{}", (mul_result == 254));
    let mut inc: u8 = max;
    let mut dec: u8 = zero;
    println!("{}", ({ let __sn_byte_place = &mut (inc); let __sn_byte_previous = *__sn_byte_place; *__sn_byte_place = __sn_byte_previous.wrapping_add(1); __sn_byte_previous } == 255));
    println!("{}", (inc == 0));
    println!("{}", ({ let __sn_byte_place = &mut (dec); let __sn_byte_previous = *__sn_byte_place; *__sn_byte_place = __sn_byte_previous.wrapping_sub(1); __sn_byte_previous } == 0));
    println!("{}", (dec == 255));
    let mut r#box: ByteBox = ByteBox { value: 255 };
    println!("{}", ({ let __sn_byte_place = &mut ((r#box).value); let __sn_byte_previous = *__sn_byte_place; *__sn_byte_place = __sn_byte_previous.wrapping_add(1); __sn_byte_previous } == 255));
    println!("{}", ((r#box).value == 0));
    let mut referenced: u8 = max;
    println!("{}", (bump(&mut (referenced)) == 0));
    println!("{}", (referenced == 0));
    let mut shifted: u8 = { let (__sn_byte_left, __sn_byte_right): (i32, i32) = (two as i32, 8 as i32); __sn_byte_left << (__sn_byte_right as u32) } as u8;
    println!("{}", (shifted == 0));
    let mut mask: u8 = 15;
    let mut anded: u8 = { let (__sn_byte_left, __sn_byte_right): (i32, i32) = (max as i32, mask as i32); __sn_byte_left & __sn_byte_right } as u8;
    let mut ored: u8 = { let (__sn_byte_left, __sn_byte_right): (i32, i32) = (zero as i32, mask as i32); __sn_byte_left | __sn_byte_right } as u8;
    let mut xored: u8 = { let (__sn_byte_left, __sn_byte_right): (i32, i32) = (max as i32, mask as i32); __sn_byte_left ^ __sn_byte_right } as u8;
    println!("{}", (anded == 15));
    println!("{}", (ored == 15));
    println!("{}", (xored == 240));
    let mut compound_shift: u8 = two;
    { let (__sn_byte_rhs, __sn_byte_place): (u8, &mut u8) = (8, &mut (compound_shift)); let __sn_byte_next = (*__sn_byte_place as u32).wrapping_shl(__sn_byte_rhs as u32) as u8; *__sn_byte_place = __sn_byte_next; __sn_byte_next };
    println!("{}", (compound_shift == 0));
    let mut compound_bits: u8 = 240;
    { let (__sn_byte_rhs, __sn_byte_place): (u8, &mut u8) = (mask, &mut (compound_bits)); let __sn_byte_next = *__sn_byte_place | __sn_byte_rhs; *__sn_byte_place = __sn_byte_next; __sn_byte_next };
    { let (__sn_byte_rhs, __sn_byte_place): (u8, &mut u8) = (255, &mut (compound_bits)); let __sn_byte_next = *__sn_byte_place ^ __sn_byte_rhs; *__sn_byte_place = __sn_byte_next; __sn_byte_next };
    println!("{}", (compound_bits == 0));
    let mut quotient: u8 = 240;
    let mut remainder: u8 = 240;
    { let __sn_rhs = two; let __sn_place = &mut (quotient); let __sn_next = (*__sn_place).checked_div(__sn_rhs).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_next };
    { let __sn_rhs = 7; let __sn_place = &mut (remainder); let __sn_next = (*__sn_place).checked_rem(__sn_rhs).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_next };
    println!("{}", (quotient == 120));
    println!("{}", (remainder == 2));
    let mut __sn_byte_left: u8 = max;
    let mut __sn_byte_right: u8 = one;
    let mut hygienic_binary: u8 = { let (__sn_byte_left, __sn_byte_right): (u8, u8) = (__sn_byte_left, __sn_byte_right); __sn_byte_left.wrapping_add(__sn_byte_right) };
    println!("{}", (hygienic_binary == 0));
    let mut __sn_byte_rhs: u8 = max;
    let mut __sn_byte_place: u8 = one;
    let mut hygienic_compound: u8 = { let (__sn_byte_rhs, __sn_byte_place): (u8, &mut u8) = (__sn_byte_place, &mut (__sn_byte_rhs)); let __sn_byte_next = (*__sn_byte_place).wrapping_add(__sn_byte_rhs); *__sn_byte_place = __sn_byte_next; __sn_byte_next };
    println!("{}", (hygienic_compound == 0));
}
