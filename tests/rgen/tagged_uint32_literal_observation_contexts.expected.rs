#![allow(dead_code, unused_mut, unused_variables, unused_parens)]

fn main() {
    println!("{}", ((-(1 as i64) as i64) == (4294967295 as i64)));
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", -(1 as i64))); __sn_interpolated });
    println!("{}", -(!(1 as i64) as i64));
    println!("{}", ((-(!(1 as i64) as i64) as i64) == (2 as i64)));
}
