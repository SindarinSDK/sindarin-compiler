#![allow(dead_code, unused_mut, unused_variables, unused_parens)]

#[derive(Clone, Copy, Debug, PartialEq)]
struct Point {
    x: i64,
    y: i64,
}

fn geo__origin() -> Point {
    return Point { x: 3, y: 4 };
}

fn main() {
    let mut point: Point = geo__origin();
    println!("{}", (point).x);
}
