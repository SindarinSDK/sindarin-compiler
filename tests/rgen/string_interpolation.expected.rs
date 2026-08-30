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

fn main() {
    let mut name: String = "world".to_string();
    let mut count: i64 = 3;
    let mut ratio: f64 = 2.5;
    let mut active: bool = true;
    let mut values: Vec<i64> = vec![1, 2, 3];
    let mut point: Point = Point { x: 7 };
    let mut message: String = { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("Hello "); __sn_interpolated.push_str(&format!("{}", name)); __sn_interpolated.push_str(": "); __sn_interpolated.push_str(&format!("{}", count)); __sn_interpolated.push_str(", "); __sn_interpolated.push_str(&format!("{:.5}", ratio)); __sn_interpolated.push_str(", "); __sn_interpolated.push_str(&format!("{}", active)); __sn_interpolated };
    println!("{}", message);
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("values="); __sn_interpolated.push_str(&format!("{:?}", values)); __sn_interpolated });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("point="); __sn_interpolated.push_str(&format!("{:?}", point)); __sn_interpolated });
}
