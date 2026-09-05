#![allow(dead_code, unused_mut, unused_variables, unused_parens)]

fn main() {
    let mut min_base: i32 = (-2147483647);
    let mut one: i32 = 1;
    let mut minimum: i32 = (min_base).checked_sub(one).expect("checked arithmetic failed");
    let mut underflow: i32 = (minimum).checked_sub(one).expect("checked arithmetic failed");
}
