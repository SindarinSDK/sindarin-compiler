#![allow(dead_code, unused_mut, unused_variables, unused_parens)]

fn halve(mut value: i64) -> i64 {
    { let (__sn_rhs, __sn_place): (i64, &mut i64) = (2, &mut (value)); let __sn_next = *__sn_place / __sn_rhs; *__sn_place = __sn_next; __sn_next };
    return value;
}

fn main() {
    let mut caller: i64 = 9;
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", halve(caller))); __sn_interpolated.push_str("/"); __sn_interpolated.push_str(&format!("{}", caller)); __sn_interpolated });
}
