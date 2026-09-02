#![allow(dead_code, unused_mut, unused_variables, unused_parens)]

fn main() {
    let mut max: u8 = 255;
    let mut two: u8 = 2;
    let mut overflow: u8 = (max).checked_mul(two).expect("checked arithmetic failed");
}
