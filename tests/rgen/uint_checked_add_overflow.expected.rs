#![allow(dead_code, unused_mut, unused_variables, unused_parens)]

fn main() {
    let mut half: u64 = 9223372036854775807;
    let mut two: u64 = 2;
    let mut one: u64 = 1;
    let mut max_minus_one: u64 = (half).checked_mul(two).expect("checked arithmetic failed");
    let mut max: u64 = (max_minus_one).checked_add(one).expect("checked arithmetic failed");
    let mut overflow: u64 = (max).checked_add(one).expect("checked arithmetic failed");
}
