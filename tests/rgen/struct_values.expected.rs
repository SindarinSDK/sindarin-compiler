#![allow(dead_code, unused_mut, unused_variables, unused_parens)]

#[derive(Clone, Copy, Debug, PartialEq)]
struct Point {
    x: i64,
    y: i64,
}
#[derive(Clone, Debug, PartialEq)]
struct Label {
    text: String,
}
#[derive(Clone, Debug, PartialEq)]
struct Marker {
    point: Point,
    label: Label,
}

fn main() {
    let mut point: Point = Point { x: 10, y: 20 };
    println!("{}", (point).x);
    ((point).x = 30);
    println!("{}", (point).x);
    let mut copied: Point = point;
    ((copied).y = 40);
    println!("{}", (point).y);
    println!("{}", (copied).y);
    let mut label: Label = Label { text: "default".to_string() };
    let mut label_copy: Label = label.clone();
    ((label_copy).text = "changed".to_string());
    println!("{}", (label).text);
    println!("{}", (label_copy).text);
    let mut marker: Marker = Marker { point: point, label: label.clone() };
    (((marker).label).text = "nested".to_string());
    println!("{}", ((marker).point).x);
    println!("{}", (label).text);
    println!("{}", ((marker).label).text);
}
