#![allow(dead_code, unused_mut, unused_variables, unused_parens)]

fn __sn_index(length: usize, index: i64) -> usize {
    let resolved = if index < 0 { length as i64 + index } else { index };
    if resolved < 0 || resolved >= length as i64 {
        panic!("array index out of bounds: {index}");
    }
    resolved as usize
}

fn __sn_insert_index(length: usize, index: i64) -> usize {
    let resolved = if index < 0 { length as i64 + index } else { index };
    if resolved < 0 || resolved > length as i64 {
        panic!("array insert index out of bounds: {index}");
    }
    resolved as usize
}

fn __sn_array_size(size: i64) -> usize {
    if size < 0 {
        panic!("array size cannot be negative: {size}");
    }
    size as usize
}

#[derive(Clone, Copy, Debug, PartialEq)]
struct Point {
    x: i64,
}
#[derive(Clone, Debug, PartialEq)]
struct Label {
    text: String,
}

fn main() {
    let mut names: Vec<String> = vec!["one".to_string()];
    for mut item in (names).iter().cloned() {
        (item = "two".to_string());
        println!("{}", ((item == "two".to_string()) && ((names)[__sn_index((names).len(), 0)] == "one".to_string())));
    }
    let mut rows: Vec<Vec<i64>> = vec![vec![1]];
    for mut item in (rows).iter().cloned() {
        (item = vec![2]);
        println!("{}", ((item == vec![2]) && ((rows)[__sn_index((rows).len(), 0)] == vec![1])));
    }
    let mut points: Vec<Point> = vec![Point { x: 1 }];
    for mut item in (points).iter().cloned() {
        (item = Point { x: 2 });
        println!("{}", (((item).x == 2) && (((points)[__sn_index((points).len(), 0)]).x == 1)));
    }
    let mut labels: Vec<Label> = vec![Label { text: "one".to_string() }];
    for mut item in (labels).iter().cloned() {
        (item = Label { text: "two".to_string() });
        println!("{}", (((item).text == "two".to_string()) && (((labels)[__sn_index((labels).len(), 0)]).text == "one".to_string())));
    }
}
