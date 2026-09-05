#![allow(dead_code, unused_mut, unused_variables, unused_parens)]

fn main() {
    println!("{}", -(1 as i64));
    println!("{}", !(1 as i64));
    let mut negated: u32 = { let __sn_byte_operand: u32 = 1; __sn_byte_operand.wrapping_neg() };
    let mut inverted: u32 = { let __sn_byte_operand: u32 = 1; !__sn_byte_operand };
    println!("{}", negated);
    println!("{}", inverted);
    println!("{}", (-(1 as i64) as i64));
    println!("{}", (!(1 as i64) as i64));
    let mut wide_negated: u64 = { let __sn_byte_operand: u64 = 1; __sn_byte_operand.wrapping_neg() };
    let mut wide_inverted: u64 = { let __sn_byte_operand: u64 = 1; !__sn_byte_operand };
    println!("{}", (wide_negated as i64));
    println!("{}", (wide_inverted as i64));
}
