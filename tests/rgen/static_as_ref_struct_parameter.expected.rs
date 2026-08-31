#![allow(dead_code, unused_mut, unused_variables, unused_parens)]

#[derive(Clone, Copy, Debug, PartialEq)]
struct Point {
    x: i64,
}

impl Point {
    fn increment(point: &mut Point) {
        ((point).x = ((point).x).checked_add(1).expect("checked arithmetic failed"));
    }
    fn twice(point: &mut Point) {
        Point::increment(&mut *(point));
        Point::increment(&mut *(point));
    }
}

fn main() {
    let mut point: Point = Point { x: 1 };
    Point::twice(&mut (point));
    println!("{}", (point).x);
}
