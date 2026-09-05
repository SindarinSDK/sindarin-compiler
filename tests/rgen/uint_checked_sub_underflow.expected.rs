#![allow(dead_code, unused_mut, unused_variables, unused_parens)]

fn main() {
    let mut zero: u64 = 0;
    let mut one: u64 = 1;
    let mut underflow: u64 = (zero).checked_sub(one).expect("checked arithmetic failed");
}
