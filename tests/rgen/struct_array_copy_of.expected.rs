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
    let mut source: Vec<Point> = vec![Point { x: 1, y: 10 }, Point { x: 2, y: 20 }];
    let mut copied: Vec<Point> = (source).clone();
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", (source).len() as i64)); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", (copied).len() as i64)); __sn_interpolated });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", ((source)[__sn_index((source).len(), 0)]).x)); __sn_interpolated.push_str(","); __sn_interpolated.push_str(&format!("{}", ((source)[__sn_index((source).len(), 0)]).y)); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", ((source)[__sn_index((source).len(), 1)]).x)); __sn_interpolated.push_str(","); __sn_interpolated.push_str(&format!("{}", ((source)[__sn_index((source).len(), 1)]).y)); __sn_interpolated });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", ((copied)[__sn_index((copied).len(), 0)]).x)); __sn_interpolated.push_str(","); __sn_interpolated.push_str(&format!("{}", ((copied)[__sn_index((copied).len(), 0)]).y)); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", ((copied)[__sn_index((copied).len(), 1)]).x)); __sn_interpolated.push_str(","); __sn_interpolated.push_str(&format!("{}", ((copied)[__sn_index((copied).len(), 1)]).y)); __sn_interpolated });
    { let __sn_array_index = __sn_index((source).len(), 0); (source)[__sn_array_index] = Point { x: 100, y: 1000 }; };
    { let __sn_array_index = __sn_index((copied).len(), 1); (copied)[__sn_array_index] = Point { x: 200, y: 2000 }; };
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", ((source)[__sn_index((source).len(), 0)]).x)); __sn_interpolated.push_str(","); __sn_interpolated.push_str(&format!("{}", ((source)[__sn_index((source).len(), 1)]).y)); __sn_interpolated });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", ((copied)[__sn_index((copied).len(), 0)]).x)); __sn_interpolated.push_str(","); __sn_interpolated.push_str(&format!("{}", ((copied)[__sn_index((copied).len(), 1)]).y)); __sn_interpolated });
}
