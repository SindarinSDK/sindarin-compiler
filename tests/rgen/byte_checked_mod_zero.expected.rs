#![allow(dead_code, unused_mut, unused_variables, unused_parens)]

fn main() {
    let mut numerator: u8 = 255;
    let mut zero: u8 = 0;
    let mut remainder: u8 = (numerator).checked_rem(zero).expect("checked arithmetic failed");
}
