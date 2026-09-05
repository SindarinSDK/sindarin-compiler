#![allow(dead_code, unused_mut, unused_variables, unused_parens)]

fn main() {
    let mut left: u32 = 65536;
    let mut right: u32 = 65536;
    let mut overflow: u32 = (left).checked_mul(right).expect("checked arithmetic failed");
}
