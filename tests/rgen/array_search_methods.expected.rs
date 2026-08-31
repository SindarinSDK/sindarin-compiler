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
    let mut numbers: Vec<i64> = vec![10, 20, 10];
    println!("{}", { let __sn_array = &(numbers); let __sn_array_search = &(20); __sn_array.contains(__sn_array_search) });
    println!("{}", { let __sn_array = &(numbers); let __sn_array_search = &(99); __sn_array.contains(__sn_array_search) });
    println!("{}", { let __sn_array = &(numbers); let __sn_array_search = &(10); __sn_array.iter().position(|__sn_item| __sn_item == __sn_array_search).map(|__sn_index| __sn_index as i64).unwrap_or(-1) });
    println!("{}", { let __sn_array = &(numbers); let __sn_array_search = &(99); __sn_array.iter().position(|__sn_item| __sn_item == __sn_array_search).map(|__sn_index| __sn_index as i64).unwrap_or(-1) });
    let mut names: Vec<String> = vec!["alpha".to_string(), "beta".to_string(), "alpha".to_string()];
    let mut needle: String = "beta".to_string();
    println!("{}", { let __sn_array = &(names); let __sn_array_search = &(needle); __sn_array.contains(__sn_array_search) });
    println!("{}", { let __sn_array = &(names); let __sn_array_search = &("alpha".to_string()); __sn_array.iter().position(|__sn_item| __sn_item == __sn_array_search).map(|__sn_index| __sn_index as i64).unwrap_or(-1) });
    println!("{}", { let __sn_array = &(names); let __sn_array_search = &("missing".to_string()); __sn_array.iter().position(|__sn_item| __sn_item == __sn_array_search).map(|__sn_index| __sn_index as i64).unwrap_or(-1) });
    println!("{}", needle);
    let mut flags: Vec<bool> = vec![true, false];
    println!("{}", { let __sn_array = &(flags); let __sn_array_search = &(false); __sn_array.contains(__sn_array_search) });
    println!("{}", { let __sn_array = &(flags); let __sn_array_search = &(true); __sn_array.iter().position(|__sn_item| __sn_item == __sn_array_search).map(|__sn_index| __sn_index as i64).unwrap_or(-1) });
    let mut bytes: Vec<u8> = vec![10, 20, 30];
    println!("{}", { let __sn_array = &(bytes); let __sn_array_search = &(20); __sn_array.contains(__sn_array_search) });
    println!("{}", { let __sn_array = &(bytes); let __sn_array_search = &(30); __sn_array.iter().position(|__sn_item| __sn_item == __sn_array_search).map(|__sn_index| __sn_index as i64).unwrap_or(-1) });
    let mut letters: Vec<char> = vec!['\u{61}', '\u{62}', '\u{61}'];
    println!("{}", { let __sn_array = &(letters); let __sn_array_search = &('\u{62}'); __sn_array.contains(__sn_array_search) });
    println!("{}", { let __sn_array = &(letters); let __sn_array_search = &('\u{7a}'); __sn_array.contains(__sn_array_search) });
    println!("{}", { let __sn_array = &(letters); let __sn_array_search = &('\u{61}'); __sn_array.iter().position(|__sn_item| __sn_item == __sn_array_search).map(|__sn_index| __sn_index as i64).unwrap_or(-1) });
    println!("{}", { let __sn_array = &(letters); let __sn_array_search = &('\u{7a}'); __sn_array.iter().position(|__sn_item| __sn_item == __sn_array_search).map(|__sn_index| __sn_index as i64).unwrap_or(-1) });
    let mut empty: Vec<i64> = vec![];
    println!("{}", { let __sn_array = &(empty); let __sn_array_search = &(1); __sn_array.contains(__sn_array_search) });
    println!("{}", { let __sn_array = &(empty); let __sn_array_search = &(1); __sn_array.iter().position(|__sn_item| __sn_item == __sn_array_search).map(|__sn_index| __sn_index as i64).unwrap_or(-1) });
}
