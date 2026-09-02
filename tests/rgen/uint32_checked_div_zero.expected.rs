#![allow(dead_code, unused_mut, unused_variables, unused_parens)]

fn main() {
    let mut numerator: u32 = 4294967295;
    let mut zero: u32 = 0;
    let mut quotient: u32 = (numerator).checked_div(zero).expect("checked arithmetic failed");
}
