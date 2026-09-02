#![allow(dead_code, unused_mut, unused_variables, unused_parens)]

fn main() {
    let mut zero: u8 = 0;
    let mut one: u8 = 1;
    let mut underflow: u8 = (zero).checked_sub(one).expect("checked arithmetic failed");
}
