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
    let mut words: Vec<String> = vec!["the".to_string(), "quick".to_string(), "fox".to_string()];
    let mut separator: String = " | ".to_string();
    println!("{}", { let __sn_separator_0 = &(separator); (words).join(__sn_separator_0.as_str()) });
    println!("{}", separator);
    println!("{}", { let __sn_separator_0 = &("".to_string()); (words).join(__sn_separator_0.as_str()) });
    let mut single: Vec<String> = vec!["alone".to_string()];
    println!("{}", { let __sn_separator_0 = &(",".to_string()); (single).join(__sn_separator_0.as_str()) });
    let mut empty: Vec<String> = vec![];
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("empty=|"); __sn_interpolated.push_str(&format!("{}", { let __sn_separator_0 = &(",".to_string()); (empty).join(__sn_separator_0.as_str()) })); __sn_interpolated.push_str("|"); __sn_interpolated });
}
