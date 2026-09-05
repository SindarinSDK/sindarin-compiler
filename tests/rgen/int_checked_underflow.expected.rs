#![allow(dead_code, unused_mut, unused_variables, unused_parens)]

fn main() {
    let mut min_base: i64 = (-9223372036854775807);
    let mut one: i64 = 1;
    let mut min: i64 = (min_base).checked_sub(one).expect("checked arithmetic failed");
    let mut underflow: i64 = (min).checked_sub(one).expect("checked arithmetic failed");
}
