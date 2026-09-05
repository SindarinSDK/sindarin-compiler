#![allow(dead_code, unused_mut, unused_variables, unused_parens)]

fn main() {
    let mut numerator: u8 = 255;
    let mut zero: u8 = 0;
    let mut quotient: u8 = (numerator).checked_div(zero).expect("checked arithmetic failed");
}
