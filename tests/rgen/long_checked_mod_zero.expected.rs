#![allow(dead_code, unused_mut, unused_variables, unused_parens)]

fn main() {
    let mut numerator: i64 = 1;
    let mut zero: i64 = 0;
    let mut remainder: i64 = (numerator).checked_rem(zero).expect("checked arithmetic failed");
}
