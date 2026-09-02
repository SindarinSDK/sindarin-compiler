#![allow(dead_code, unused_mut, unused_variables, unused_parens)]

fn main() {
    let mut numerator: i32 = 1;
    let mut zero: i32 = 0;
    let mut remainder: i32 = (numerator).checked_rem(zero).expect("checked arithmetic failed");
}
