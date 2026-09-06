#![allow(dead_code, unused_mut, unused_variables, unused_parens)]

fn main() {
    let mut narrow: u32 = 0;
    let mut wide: u64 = 0;
    println!("{}", ((-(1 as i64) as u64) < (narrow as u64)));
    println!("{}", ((-(1 as i64) as u32) > (wide as u32)));
}
