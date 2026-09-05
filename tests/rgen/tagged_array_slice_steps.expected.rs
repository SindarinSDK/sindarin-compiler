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

fn marker() -> i64 {
    print!("{}", "marker\n".to_string());
    return 2;
}

fn main() {
    std::process::exit((|| -> i64 {
        let mut values: Vec<i64> = vec![0, 1, 2, 3, 4, 5];
        let mut default_step: Vec<i64> = { let __sn_array = &(values); let __sn_length = __sn_array.len() as i64; let mut __sn_start: i64 = 0; let mut __sn_end: i64 = __sn_length; if __sn_start < 0 { __sn_start += __sn_length; } if __sn_end < 0 { __sn_end += __sn_length; } if __sn_start < 0 { __sn_start = 0; } if __sn_end > __sn_length { __sn_end = __sn_length; } if __sn_start >= __sn_end { Vec::new() } else { __sn_array[__sn_start as usize..__sn_end as usize].to_vec() } };
        let mut positive_step: Vec<i64> = { let __sn_array = &(values); let __sn_length = __sn_array.len() as i64; let mut __sn_start: i64 = 1; let mut __sn_end: i64 = 5; if __sn_start < 0 { __sn_start += __sn_length; } if __sn_end < 0 { __sn_end += __sn_length; } if __sn_start < 0 { __sn_start = 0; } if __sn_end > __sn_length { __sn_end = __sn_length; } if __sn_start >= __sn_end { Vec::new() } else { __sn_array[__sn_start as usize..__sn_end as usize].to_vec() } };
        let mut negative_step: Vec<i64> = { let __sn_array = &(values); let __sn_length = __sn_array.len() as i64; let mut __sn_start: i64 = 1; let mut __sn_end: i64 = 5; if __sn_start < 0 { __sn_start += __sn_length; } if __sn_end < 0 { __sn_end += __sn_length; } if __sn_start < 0 { __sn_start = 0; } if __sn_end > __sn_length { __sn_end = __sn_length; } if __sn_start >= __sn_end { Vec::new() } else { __sn_array[__sn_start as usize..__sn_end as usize].to_vec() } };
        let mut zero_step: Vec<i64> = { let __sn_array = &(values); let __sn_length = __sn_array.len() as i64; let mut __sn_start: i64 = 0; let mut __sn_end: i64 = __sn_length; if __sn_start < 0 { __sn_start += __sn_length; } if __sn_end < 0 { __sn_end += __sn_length; } if __sn_start < 0 { __sn_start = 0; } if __sn_end > __sn_length { __sn_end = __sn_length; } if __sn_start >= __sn_end { Vec::new() } else { __sn_array[__sn_start as usize..__sn_end as usize].to_vec() } };
        let mut negative_bounds: Vec<i64> = { let __sn_array = &(values); let __sn_length = __sn_array.len() as i64; let mut __sn_start: i64 = (-4); let mut __sn_end: i64 = (-1); if __sn_start < 0 { __sn_start += __sn_length; } if __sn_end < 0 { __sn_end += __sn_length; } if __sn_start < 0 { __sn_start = 0; } if __sn_end > __sn_length { __sn_end = __sn_length; } if __sn_start >= __sn_end { Vec::new() } else { __sn_array[__sn_start as usize..__sn_end as usize].to_vec() } };
        let mut beyond_start: Vec<i64> = { let __sn_array = &(values); let __sn_length = __sn_array.len() as i64; let mut __sn_start: i64 = 8; let mut __sn_end: i64 = __sn_length; if __sn_start < 0 { __sn_start += __sn_length; } if __sn_end < 0 { __sn_end += __sn_length; } if __sn_start < 0 { __sn_start = 0; } if __sn_end > __sn_length { __sn_end = __sn_length; } if __sn_start >= __sn_end { Vec::new() } else { __sn_array[__sn_start as usize..__sn_end as usize].to_vec() } };
        let mut before_end: Vec<i64> = { let __sn_array = &(values); let __sn_length = __sn_array.len() as i64; let mut __sn_start: i64 = 0; let mut __sn_end: i64 = (-8); if __sn_start < 0 { __sn_start += __sn_length; } if __sn_end < 0 { __sn_end += __sn_length; } if __sn_start < 0 { __sn_start = 0; } if __sn_end > __sn_length { __sn_end = __sn_length; } if __sn_start >= __sn_end { Vec::new() } else { __sn_array[__sn_start as usize..__sn_end as usize].to_vec() } };
        let mut empty: Vec<i64> = { let __sn_array = &(values); let __sn_length = __sn_array.len() as i64; let mut __sn_start: i64 = 3; let mut __sn_end: i64 = 3; if __sn_start < 0 { __sn_start += __sn_length; } if __sn_end < 0 { __sn_end += __sn_length; } if __sn_start < 0 { __sn_start = 0; } if __sn_end > __sn_length { __sn_end = __sn_length; } if __sn_start >= __sn_end { Vec::new() } else { __sn_array[__sn_start as usize..__sn_end as usize].to_vec() } };
        print!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", (default_step).len() as i64)); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", (default_step)[__sn_index((default_step).len(), 0)])); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", (default_step)[__sn_index((default_step).len(), 5)])); __sn_interpolated.push_str("\n"); __sn_interpolated });
        print!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", (positive_step).len() as i64)); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", (positive_step)[__sn_index((positive_step).len(), 0)])); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", (positive_step)[__sn_index((positive_step).len(), 3)])); __sn_interpolated.push_str("\n"); __sn_interpolated });
        print!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", (negative_step).len() as i64)); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", (negative_step)[__sn_index((negative_step).len(), 0)])); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", (negative_step)[__sn_index((negative_step).len(), 3)])); __sn_interpolated.push_str("\n"); __sn_interpolated });
        print!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", (zero_step).len() as i64)); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", (zero_step)[__sn_index((zero_step).len(), 0)])); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", (zero_step)[__sn_index((zero_step).len(), 5)])); __sn_interpolated.push_str("\n"); __sn_interpolated });
        print!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", (negative_bounds).len() as i64)); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", (negative_bounds)[__sn_index((negative_bounds).len(), 0)])); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", (negative_bounds)[__sn_index((negative_bounds).len(), 2)])); __sn_interpolated.push_str("\n"); __sn_interpolated });
        print!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", (beyond_start).len() as i64)); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", (before_end).len() as i64)); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", (empty).len() as i64)); __sn_interpolated.push_str("\n"); __sn_interpolated });
        return 0;
        return 0;
    })() as i32);
}
