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

fn assign(value: &mut Vec<i64>) -> i64 {
    { let __sn_array_index = __sn_index((value).len(), 0); (value)[__sn_array_index] = 2; };
    return (value)[__sn_index((value).len(), 0)];
}

fn main() {
    let mut values: Vec<i64> = vec![1];
    println!("{}", assign(&mut (values)));
}
