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
    let mut source: Vec<String> = vec!["alpha".to_string(), "beta".to_string()];
    let mut copied: Vec<String> = (source).clone();
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", (source).len() as i64)); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", (copied).len() as i64)); __sn_interpolated });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", (source)[__sn_index((source).len(), 0)])); __sn_interpolated.push_str(","); __sn_interpolated.push_str(&format!("{}", (source)[__sn_index((source).len(), 1)])); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", (copied)[__sn_index((copied).len(), 0)])); __sn_interpolated.push_str(","); __sn_interpolated.push_str(&format!("{}", (copied)[__sn_index((copied).len(), 1)])); __sn_interpolated });
    { let __sn_array_index = __sn_index((source).len(), 0); (source)[__sn_array_index] = "changed-source".to_string(); };
    { let __sn_array_index = __sn_index((copied).len(), 1); (copied)[__sn_array_index] = "changed-copy".to_string(); };
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", (source)[__sn_index((source).len(), 0)])); __sn_interpolated.push_str(","); __sn_interpolated.push_str(&format!("{}", (source)[__sn_index((source).len(), 1)])); __sn_interpolated });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", (copied)[__sn_index((copied).len(), 0)])); __sn_interpolated.push_str(","); __sn_interpolated.push_str(&format!("{}", (copied)[__sn_index((copied).len(), 1)])); __sn_interpolated });
}
