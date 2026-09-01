#![allow(dead_code, unused_mut, unused_variables, unused_parens)]

fn main() {
    std::process::exit((|| -> i64 {
        let mut x: i64 = 3;
        return 0;
    })() as i32);
}
