#![allow(dead_code, unused_mut, unused_variables, unused_parens)]

fn bumpAndReturn(counter: &mut i64) -> i64 {
    { let __sn_rhs = 1; let __sn_place = &mut (*(counter)); let __sn_next = (*__sn_place).checked_add(__sn_rhs).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_next };
    return (-1);
}

fn main() {
    let mut negative: i64 = (-1);
    let mut as_long: i64 = (negative as i64);
    let mut as_uint: u64 = (negative as u64);
    let mut as_byte: u8 = (negative as u8);
    let mut half_uint: u64 = 9223372036854775807;
    let mut max_uint: u64 = ((half_uint).checked_mul(2).expect("checked arithmetic failed")).checked_add(1).expect("checked arithmetic failed");
    let mut long_value: i64 = 42;
    let mut as_int: i64 = (long_value as i64);
    let mut byte_value: u8 = 200;
    let mut widened: i64 = (byte_value as i64);
    let mut true_value: i64 = (true as i64);
    let mut false_value: i64 = (false as i64);
    let mut counter: i64 = 0;
    let mut called_byte: u8 = (bumpAndReturn(&mut (counter)) as u8);
    let mut called_value: i64 = (called_byte as i64);
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", (((((((as_long == (-1)) && (as_uint == max_uint)) && (as_byte == 255)) && (as_int == 42)) && (widened == 200)) && (true_value == 1)) && (false_value == 0)))); __sn_interpolated.push_str(" "); __sn_interpolated.push_str(&format!("{}", ((counter == 1) && (called_value == 255)))); __sn_interpolated });
}
