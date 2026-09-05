#![allow(dead_code, unused_mut, unused_variables, unused_parens)]

fn main() {
    let mut zero: u32 = 0;
    let mut one: u32 = 1;
    let mut underflow: u32 = (zero).checked_sub(one).expect("checked arithmetic failed");
}
