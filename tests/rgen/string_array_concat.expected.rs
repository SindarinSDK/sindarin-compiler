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

fn main() {
    let mut left: Vec<String> = vec!["alpha".to_string(), "beta".to_string()];
    let mut right: Vec<String> = vec!["gamma".to_string(), "delta".to_string()];
    let mut combined: Vec<String> = { let __sn_array_left = &(left); let __sn_array_right = &(right); [__sn_array_left.as_slice(), __sn_array_right.as_slice()].concat() };
    println!("{}", (combined).len() as i64);
    println!("{}", { let __sn_separator_0 = &(",".to_string()); (combined).join(__sn_separator_0.as_str()) });
    { let __sn_array_index = __sn_index((left).len(), 0); (left)[__sn_array_index] = "changed-left".to_string(); };
    { let __sn_array_index = __sn_index((right).len(), 1); (right)[__sn_array_index] = "changed-right".to_string(); };
    println!("{}", { let __sn_separator_0 = &(",".to_string()); (left).join(__sn_separator_0.as_str()) });
    println!("{}", { let __sn_separator_0 = &(",".to_string()); (right).join(__sn_separator_0.as_str()) });
    println!("{}", { let __sn_separator_0 = &(",".to_string()); (combined).join(__sn_separator_0.as_str()) });
}
