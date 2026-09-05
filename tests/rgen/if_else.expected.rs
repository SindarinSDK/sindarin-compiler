#![allow(dead_code, unused_mut, unused_variables, unused_parens)]

fn main() {
    let mut true_branch: i64 = 0;
    if true {
        (true_branch = 10);
    } else {
        (true_branch = 100);
    }
    println!("{}", true_branch);
    let mut false_branch: i64 = 0;
    if false {
        (false_branch = 100);
    } else {
        (false_branch = 20);
    }
    println!("{}", false_branch);
    let mut no_else: i64 = 30;
    if false {
        (no_else = 100);
    }
    println!("{}", no_else);
}
