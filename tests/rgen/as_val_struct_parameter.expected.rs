#![allow(dead_code, unused_mut, unused_variables, unused_parens)]

#[derive(Clone, Copy, Debug, PartialEq)]
struct Point {
    x: i64,
}

fn increment(mut point: Point) -> i64 {
    ((point).x = ((point).x).checked_add(1).expect("checked arithmetic failed"));
    return (point).x;
}

fn main() {
    let mut point: Point = Point { x: 1 };
    println!("{}", increment(point));
    println!("{}", (point).x);
}
