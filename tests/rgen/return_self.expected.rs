#![allow(dead_code, unused_mut, unused_variables, unused_parens)]

#[derive(Clone, Copy, Debug, PartialEq)]
struct Point {
    x: i64,
    y: i64,
}

impl Point {
    fn snapshot(&self) -> Point {
        return self.clone();
    }
    fn shiftX(&mut self, amount: i64) -> Point {
        ((self).x = ((self).x).checked_add(amount).expect("checked arithmetic failed"));
        return self.clone();
    }
}

fn main() {
    let mut original: Point = Point { x: 1, y: 2 };
    let mut snapshot: Point = (original).snapshot();
    let mut shifted: Point = (original).shiftX(4);
    println!("{}", (original).x);
    println!("{}", (shifted).x);
    ((original).x = 9);
    ((original).y = 10);
    println!("{}", (snapshot).x);
    println!("{}", (snapshot).y);
    println!("{}", (shifted).x);
    println!("{}", (shifted).y);
    println!("{}", (original).x);
    println!("{}", (original).y);
}
