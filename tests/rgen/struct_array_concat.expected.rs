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
    y: i64,
}

fn main() {
    let mut left: Vec<Point> = vec![Point { x: 1, y: 10 }, Point { x: 2, y: 20 }];
    let mut right: Vec<Point> = vec![Point { x: 3, y: 30 }, Point { x: 4, y: 40 }];
    let mut combined: Vec<Point> = { let __sn_array_left = &(left); let __sn_array_right = &(right); [__sn_array_left.as_slice(), __sn_array_right.as_slice()].concat() };
    println!("{}", (combined).len() as i64);
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", ((combined)[__sn_index((combined).len(), 0)]).x)); __sn_interpolated.push_str(","); __sn_interpolated.push_str(&format!("{}", ((combined)[__sn_index((combined).len(), 0)]).y)); __sn_interpolated });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", ((combined)[__sn_index((combined).len(), 1)]).x)); __sn_interpolated.push_str(","); __sn_interpolated.push_str(&format!("{}", ((combined)[__sn_index((combined).len(), 1)]).y)); __sn_interpolated });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", ((combined)[__sn_index((combined).len(), 2)]).x)); __sn_interpolated.push_str(","); __sn_interpolated.push_str(&format!("{}", ((combined)[__sn_index((combined).len(), 2)]).y)); __sn_interpolated });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", ((combined)[__sn_index((combined).len(), 3)]).x)); __sn_interpolated.push_str(","); __sn_interpolated.push_str(&format!("{}", ((combined)[__sn_index((combined).len(), 3)]).y)); __sn_interpolated });
    { let __sn_array_index = __sn_index((left).len(), 0); (left)[__sn_array_index] = Point { x: 100, y: 1000 }; };
    { let __sn_array_index = __sn_index((right).len(), 1); (right)[__sn_array_index] = Point { x: 4000, y: 400 }; };
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", ((left)[__sn_index((left).len(), 0)]).x)); __sn_interpolated.push_str(","); __sn_interpolated.push_str(&format!("{}", ((right)[__sn_index((right).len(), 1)]).y)); __sn_interpolated });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", ((combined)[__sn_index((combined).len(), 0)]).x)); __sn_interpolated.push_str(","); __sn_interpolated.push_str(&format!("{}", ((combined)[__sn_index((combined).len(), 3)]).y)); __sn_interpolated });
}
