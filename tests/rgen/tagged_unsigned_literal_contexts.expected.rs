#![allow(dead_code, unused_mut, unused_variables, unused_parens)]

fn main() {
    println!("{}", -(1 as i64));
    println!("{}", !(1 as i64));
    let mut negated: u32 = (-(1 as i64) as u32);
    let mut inverted: u32 = (!(1 as i64) as u32);
    println!("{}", negated);
    println!("{}", inverted);
    println!("{}", (-(1 as i64) as i64));
    println!("{}", (!(1 as i64) as i64));
    let mut wide_negated: u64 = (-(1 as i64) as u64);
    let mut wide_inverted: u64 = (!(1 as i64) as u64);
    println!("{}", (wide_negated as i64));
    println!("{}", (wide_inverted as i64));
}
