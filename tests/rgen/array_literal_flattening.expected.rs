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
    let mut mixed: Vec<i64> = { let mut __sn_array: Vec<i64> = Vec::new(); __sn_array.push(0); __sn_array.extend((1..4).collect::<Vec<i64>>()); __sn_array.push(10); __sn_array };
    println!("{}", (mixed).len() as i64);
    println!("{}", (mixed)[__sn_index((mixed).len(), 0)]);
    println!("{}", (mixed)[__sn_index((mixed).len(), 1)]);
    println!("{}", (mixed)[__sn_index((mixed).len(), (-2))]);
    println!("{}", (mixed)[__sn_index((mixed).len(), (-1))]);
    let mut source: Vec<i64> = vec![1, 2, 3];
    let mut spread: Vec<i64> = { let mut __sn_array: Vec<i64> = Vec::new(); __sn_array.push(0); __sn_array.extend((source).iter().cloned()); __sn_array.push(4); __sn_array };
    { let __sn_array_index = __sn_index((source).len(), 0); (source)[__sn_array_index] = 9; };
    println!("{}", (spread).len() as i64);
    println!("{}", (spread)[__sn_index((spread).len(), 1)]);
    let mut left: Vec<i64> = vec![5, 6];
    let mut right: Vec<i64> = vec![7, 8];
    let mut combined: Vec<i64> = { let mut __sn_array: Vec<i64> = Vec::new(); __sn_array.extend((left).iter().cloned()); __sn_array.extend((right).iter().cloned()); __sn_array };
    println!("{}", (combined).len() as i64);
    println!("{}", (combined)[__sn_index((combined).len(), 0)]);
    println!("{}", (combined)[__sn_index((combined).len(), (-1))]);
    let mut empty: Vec<i64> = vec![];
    let mut with_empty: Vec<i64> = { let mut __sn_array: Vec<i64> = Vec::new(); __sn_array.push((-1)); __sn_array.extend((empty).iter().cloned()); __sn_array.extend((2..4).collect::<Vec<i64>>()); __sn_array };
    println!("{}", (with_empty).len() as i64);
    println!("{}", (with_empty)[__sn_index((with_empty).len(), (-1))]);
    let mut names: Vec<String> = vec!["alpha".to_string(), "beta".to_string()];
    let mut copied_names: Vec<String> = { let mut __sn_array: Vec<String> = Vec::new(); __sn_array.extend((names).iter().cloned()); __sn_array.push("gamma".to_string()); __sn_array };
    { let __sn_array_index = __sn_index((names).len(), 0); (names)[__sn_array_index] = "changed".to_string(); };
    println!("{}", { let __sn_separator_0 = &(",".to_string()); (copied_names).join(__sn_separator_0.as_str()) });
}
