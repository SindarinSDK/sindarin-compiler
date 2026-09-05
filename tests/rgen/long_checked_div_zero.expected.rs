#![allow(dead_code, unused_mut, unused_variables, unused_parens)]

fn main() {
    let mut numerator: i64 = 1;
    let mut zero: i64 = 0;
    let mut quotient: i64 = (numerator).checked_div(zero).expect("checked arithmetic failed");
}
