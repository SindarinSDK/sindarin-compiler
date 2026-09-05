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

fn observe_text(counter: &mut i64) -> String {
    { let __sn_rhs = 1; let __sn_place = &mut (*(counter)); let __sn_next = (*__sn_place).checked_add(__sn_rhs).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_next };
    return "called".to_string();
}

fn observe_values(counter: &mut i64) -> Vec<i64> {
    { let __sn_rhs = 1; let __sn_place = &mut (*(counter)); let __sn_next = (*__sn_place).checked_add(__sn_rhs).expect("checked arithmetic failed"); *__sn_place = __sn_next; __sn_next };
    return vec![1, 2];
}

fn main() {
    let mut type_string: i64 = 8i64;
    let mut type_array: i64 = 8i64;
    let mut type_nested_array: i64 = 8i64;
    let mut type_struct_array: i64 = 8i64;
    let mut text: String = "managed".to_string();
    let mut values: Vec<i64> = vec![1, 2];
    let mut rows: Vec<Vec<i64>> = vec![vec![1, 2]];
    let mut points: Vec<Point> = vec![Point { x: 1 }];
    let mut counter: i64 = 0;
    let mut expression_sizes: i64 = (((((8i64).checked_add(8i64).expect("checked arithmetic failed")).checked_add(8i64).expect("checked arithmetic failed")).checked_add(8i64).expect("checked arithmetic failed")).checked_add(8i64).expect("checked arithmetic failed")).checked_add(8i64).expect("checked arithmetic failed");
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", ((((type_string == 8) && (type_array == 8)) && (type_nested_array == 8)) && (type_struct_array == 8)))); __sn_interpolated.push_str(" "); __sn_interpolated.push_str(&format!("{}", (expression_sizes == 48))); __sn_interpolated.push_str(" "); __sn_interpolated.push_str(&format!("{}", (counter == 0))); __sn_interpolated });
}
