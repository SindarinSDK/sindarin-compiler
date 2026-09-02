#![allow(dead_code, unused_mut, unused_variables, unused_parens)]

fn increment(value: &mut i64) -> i64 {
    return { let __sn_previous = *(value); *(value) += 1; __sn_previous };
}

fn decrement(value: &mut i64) -> i64 {
    return { let __sn_previous = *(value); *(value) -= 1; __sn_previous };
}

fn main() {
    let mut high: i64 = 9;
    let mut low: i64 = 5;
    println!("{}", increment(&mut (high)));
    println!("{}", decrement(&mut (low)));
}
