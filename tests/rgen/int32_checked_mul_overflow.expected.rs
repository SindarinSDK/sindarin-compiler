#![allow(dead_code, unused_mut, unused_variables, unused_parens)]

fn main() {
    let mut max: i32 = 2147483647;
    let mut two: i32 = 2;
    let mut overflow: i32 = (max).checked_mul(two).expect("checked arithmetic failed");
}
