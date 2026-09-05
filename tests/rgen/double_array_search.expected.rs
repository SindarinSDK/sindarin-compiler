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
    let mut zero: f64 = 0.0;
    let mut negative_zero: f64 = (-0.0);
    let mut nan: f64 = (zero / zero);
    let mut values: Vec<f64> = vec![zero, 1.5, nan, 1.5];
    println!("{}", { let __sn_array = &(values); let __sn_array_search = (1.5 as f64).to_bits(); __sn_array.iter().any(|__sn_item| __sn_item.to_bits() == __sn_array_search) });
    println!("{}", { let __sn_array = &(values); let __sn_array_search = (1.5 as f64).to_bits(); __sn_array.iter().position(|__sn_item| __sn_item.to_bits() == __sn_array_search).map(|__sn_index| __sn_index as i64).unwrap_or(-1) });
    println!("{}", { let __sn_array = &(values); let __sn_array_search = (negative_zero as f64).to_bits(); __sn_array.iter().any(|__sn_item| __sn_item.to_bits() == __sn_array_search) });
    println!("{}", { let __sn_array = &(values); let __sn_array_search = (negative_zero as f64).to_bits(); __sn_array.iter().position(|__sn_item| __sn_item.to_bits() == __sn_array_search).map(|__sn_index| __sn_index as i64).unwrap_or(-1) });
    println!("{}", { let __sn_array = &(values); let __sn_array_search = (nan as f64).to_bits(); __sn_array.iter().any(|__sn_item| __sn_item.to_bits() == __sn_array_search) });
    println!("{}", { let __sn_array = &(values); let __sn_array_search = (nan as f64).to_bits(); __sn_array.iter().position(|__sn_item| __sn_item.to_bits() == __sn_array_search).map(|__sn_index| __sn_index as i64).unwrap_or(-1) });
}
