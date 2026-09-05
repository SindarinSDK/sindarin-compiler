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
    let mut left: Vec<i64> = vec![1, 2];
    let mut right: Vec<i64> = vec![3, 4];
    let mut combined: Vec<i64> = { let __sn_array_left = &(left); let __sn_array_right = &(right); [__sn_array_left.as_slice(), __sn_array_right.as_slice()].concat() };
    (combined).push(5);
    println!("{}", (left).len() as i64);
    println!("{}", (right).len() as i64);
    println!("{}", (combined).len() as i64);
    println!("{}", (combined)[__sn_index((combined).len(), 0)]);
    println!("{}", (combined)[__sn_index((combined).len(), (-1))]);
    let mut self_concat: Vec<i64> = { let __sn_array_left = &(left); let __sn_array_right = &(left); [__sn_array_left.as_slice(), __sn_array_right.as_slice()].concat() };
    println!("{}", (self_concat).len() as i64);
    println!("{}", (self_concat)[__sn_index((self_concat).len(), 2)]);
    let mut empty: Vec<i64> = vec![];
    let mut with_empty: Vec<i64> = { let __sn_array_left = &(empty); let __sn_array_right = &(right); [__sn_array_left.as_slice(), __sn_array_right.as_slice()].concat() };
    println!("{}", (with_empty).len() as i64);
    let mut first_flags: Vec<bool> = vec![true];
    let mut second_flags: Vec<bool> = vec![false];
    let mut flags: Vec<bool> = { let __sn_array_left = &(first_flags); let __sn_array_right = &(second_flags); [__sn_array_left.as_slice(), __sn_array_right.as_slice()].concat() };
    println!("{}", (flags)[__sn_index((flags).len(), 0)]);
    println!("{}", (flags)[__sn_index((flags).len(), 1)]);
}
