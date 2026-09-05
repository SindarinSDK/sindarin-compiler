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
    let mut values: Vec<i64> = vec![0, 1, 2, 3, 4, 5];
    let mut middle: Vec<i64> = { let __sn_array = &(values); let __sn_length = __sn_array.len() as i64; let mut __sn_start: i64 = 1; let mut __sn_end: i64 = 4; if __sn_start < 0 { __sn_start += __sn_length; } if __sn_end < 0 { __sn_end += __sn_length; } if __sn_start < 0 { __sn_start = 0; } if __sn_end > __sn_length { __sn_end = __sn_length; } if __sn_start >= __sn_end { Vec::new() } else { __sn_array[__sn_start as usize..__sn_end as usize].to_vec() } };
    (middle).push(9);
    println!("{}", (values).len() as i64);
    println!("{}", (middle).len() as i64);
    println!("{}", (middle)[__sn_index((middle).len(), 0)]);
    println!("{}", (middle)[__sn_index((middle).len(), (-1))]);
    let mut prefix: Vec<i64> = { let __sn_array = &(values); let __sn_length = __sn_array.len() as i64; let mut __sn_start: i64 = 0; let mut __sn_end: i64 = 3; if __sn_start < 0 { __sn_start += __sn_length; } if __sn_end < 0 { __sn_end += __sn_length; } if __sn_start < 0 { __sn_start = 0; } if __sn_end > __sn_length { __sn_end = __sn_length; } if __sn_start >= __sn_end { Vec::new() } else { __sn_array[__sn_start as usize..__sn_end as usize].to_vec() } };
    let mut suffix: Vec<i64> = { let __sn_array = &(values); let __sn_length = __sn_array.len() as i64; let mut __sn_start: i64 = (-2); let mut __sn_end: i64 = __sn_length; if __sn_start < 0 { __sn_start += __sn_length; } if __sn_end < 0 { __sn_end += __sn_length; } if __sn_start < 0 { __sn_start = 0; } if __sn_end > __sn_length { __sn_end = __sn_length; } if __sn_start >= __sn_end { Vec::new() } else { __sn_array[__sn_start as usize..__sn_end as usize].to_vec() } };
    let mut full: Vec<i64> = { let __sn_array = &(values); let __sn_length = __sn_array.len() as i64; let mut __sn_start: i64 = 0; let mut __sn_end: i64 = __sn_length; if __sn_start < 0 { __sn_start += __sn_length; } if __sn_end < 0 { __sn_end += __sn_length; } if __sn_start < 0 { __sn_start = 0; } if __sn_end > __sn_length { __sn_end = __sn_length; } if __sn_start >= __sn_end { Vec::new() } else { __sn_array[__sn_start as usize..__sn_end as usize].to_vec() } };
    let mut clamped: Vec<i64> = { let __sn_array = &(values); let __sn_length = __sn_array.len() as i64; let mut __sn_start: i64 = (-20); let mut __sn_end: i64 = 20; if __sn_start < 0 { __sn_start += __sn_length; } if __sn_end < 0 { __sn_end += __sn_length; } if __sn_start < 0 { __sn_start = 0; } if __sn_end > __sn_length { __sn_end = __sn_length; } if __sn_start >= __sn_end { Vec::new() } else { __sn_array[__sn_start as usize..__sn_end as usize].to_vec() } };
    let mut empty_equal: Vec<i64> = { let __sn_array = &(values); let __sn_length = __sn_array.len() as i64; let mut __sn_start: i64 = 3; let mut __sn_end: i64 = 3; if __sn_start < 0 { __sn_start += __sn_length; } if __sn_end < 0 { __sn_end += __sn_length; } if __sn_start < 0 { __sn_start = 0; } if __sn_end > __sn_length { __sn_end = __sn_length; } if __sn_start >= __sn_end { Vec::new() } else { __sn_array[__sn_start as usize..__sn_end as usize].to_vec() } };
    let mut empty_reverse: Vec<i64> = { let __sn_array = &(values); let __sn_length = __sn_array.len() as i64; let mut __sn_start: i64 = 4; let mut __sn_end: i64 = 2; if __sn_start < 0 { __sn_start += __sn_length; } if __sn_end < 0 { __sn_end += __sn_length; } if __sn_start < 0 { __sn_start = 0; } if __sn_end > __sn_length { __sn_end = __sn_length; } if __sn_start >= __sn_end { Vec::new() } else { __sn_array[__sn_start as usize..__sn_end as usize].to_vec() } };
    let mut empty_end: Vec<i64> = { let __sn_array = &(values); let __sn_length = __sn_array.len() as i64; let mut __sn_start: i64 = 0; let mut __sn_end: i64 = (-20); if __sn_start < 0 { __sn_start += __sn_length; } if __sn_end < 0 { __sn_end += __sn_length; } if __sn_start < 0 { __sn_start = 0; } if __sn_end > __sn_length { __sn_end = __sn_length; } if __sn_start >= __sn_end { Vec::new() } else { __sn_array[__sn_start as usize..__sn_end as usize].to_vec() } };
    println!("{}", (prefix)[__sn_index((prefix).len(), (-1))]);
    println!("{}", (suffix)[__sn_index((suffix).len(), 0)]);
    println!("{}", (suffix)[__sn_index((suffix).len(), (-1))]);
    println!("{}", (full).len() as i64);
    println!("{}", (clamped).len() as i64);
    println!("{}", (empty_equal).len() as i64);
    println!("{}", (empty_reverse).len() as i64);
    println!("{}", (empty_end).len() as i64);
    let mut names: Vec<String> = vec!["alpha".to_string(), "beta".to_string(), "gamma".to_string()];
    let mut selected: Vec<String> = { let __sn_array = &(names); let __sn_length = __sn_array.len() as i64; let mut __sn_start: i64 = 1; let mut __sn_end: i64 = __sn_length; if __sn_start < 0 { __sn_start += __sn_length; } if __sn_end < 0 { __sn_end += __sn_length; } if __sn_start < 0 { __sn_start = 0; } if __sn_end > __sn_length { __sn_end = __sn_length; } if __sn_start >= __sn_end { Vec::new() } else { __sn_array[__sn_start as usize..__sn_end as usize].to_vec() } };
    { let __sn_array_index = __sn_index((names).len(), 1); (names)[__sn_array_index] = "changed".to_string(); };
    println!("{}", { let __sn_separator_0 = &(",".to_string()); (selected).join(__sn_separator_0.as_str()) });
}
