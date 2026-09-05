#![allow(dead_code, unused_mut, unused_variables, unused_parens)]

#[derive(Clone, Copy, Debug, PartialEq)]
struct Point {
    x: i64,
}

impl Point {
    fn offset(&self, mut other: Point) -> i64 {
        let mut replacement: Point = Point { x: ((other).x).checked_add((self).x).expect("checked arithmetic failed") };
        (other = replacement);
        return (other).x;
    }
}

fn main() {
    let mut point: Point = Point { x: 1 };
    let mut other: Point = Point { x: 2 };
    println!("{}", (point).offset(other));
    println!("{}", (other).x);
    println!("{}", (point).x);
    println!("{}", (point).offset(point));
    println!("{}", (point).x);
}
