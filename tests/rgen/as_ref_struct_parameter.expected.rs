#![allow(dead_code, unused_mut, unused_variables, unused_parens)]

#[derive(Clone, Copy, Debug, PartialEq)]
struct Point {
    x: i64,
}

fn increment(point: &mut Point) {
    ((point).x = ((point).x).checked_add(1).expect("checked arithmetic failed"));
}

fn increment_twice(point: &mut Point) {
    increment(&mut *(point));
    increment(&mut *(point));
}

fn main() {
    let mut point: Point = Point { x: 1 };
    increment_twice(&mut (point));
    println!("{}", (point).x);
}
