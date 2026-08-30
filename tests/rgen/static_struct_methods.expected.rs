#![allow(dead_code, unused_mut, unused_variables, unused_parens)]

#[derive(Clone, Copy, Debug, PartialEq)]
struct Point {
    x: i64,
    y: i64,
}

impl Point {
    fn create(x: i64, y: i64) -> Point {
        return Point { x: x, y: y };
    }
    fn origin() -> Point {
        return Point { x: 0, y: 0 };
    }
}
#[derive(Clone, Debug, PartialEq)]
struct Label {
    text: String,
}

impl Label {
    fn wrap(value: String) -> Label {
        return Label { text: { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("["); __sn_interpolated.push_str(&format!("{}", value)); __sn_interpolated.push_str("]"); __sn_interpolated } };
    }
}

fn main() {
    let mut point: Point = Point::create(3, 4);
    let mut origin: Point = Point::origin();
    let mut source: String = "hello".to_string();
    let mut label: Label = Label::wrap(source.clone());
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("point="); __sn_interpolated.push_str(&format!("{}", (point).x)); __sn_interpolated.push_str(","); __sn_interpolated.push_str(&format!("{}", (point).y)); __sn_interpolated });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("origin="); __sn_interpolated.push_str(&format!("{}", (origin).x)); __sn_interpolated.push_str(","); __sn_interpolated.push_str(&format!("{}", (origin).y)); __sn_interpolated });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("label="); __sn_interpolated.push_str(&format!("{}", (label).text)); __sn_interpolated.push_str("; source="); __sn_interpolated.push_str(&format!("{}", source)); __sn_interpolated });
}
