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

fn makePipe() -> String {
    println!("{}", "made".to_string());
    return "first\nsecond\n\n".to_string();
}

fn forward(value: String) -> String {
    return value;
}

fn chooseIndex() -> i64 {
    println!("{}", "index".to_string());
    return 1;
}

fn main() {
    let mut source: String = makePipe();
    let mut lines: Vec<String> = { let __sn_string_value = &(source); let __sn_string_bytes = __sn_string_value.as_bytes(); let mut __sn_string_lines = Vec::new(); let mut __sn_string_start = 0usize; while __sn_string_start < __sn_string_bytes.len() { let mut __sn_string_end = __sn_string_start; while __sn_string_end < __sn_string_bytes.len() && __sn_string_bytes[__sn_string_end] != b'\n' && __sn_string_bytes[__sn_string_end] != b'\r' { __sn_string_end += 1; } __sn_string_lines.push(String::from_utf8(__sn_string_bytes[__sn_string_start..__sn_string_end].to_vec()).expect("splitLines produced invalid UTF-8")); __sn_string_start = __sn_string_end; if __sn_string_start < __sn_string_bytes.len() && __sn_string_bytes[__sn_string_start] == b'\r' && __sn_string_start + 1 < __sn_string_bytes.len() && __sn_string_bytes[__sn_string_start + 1] == b'\n' { __sn_string_start += 2; } else if __sn_string_start < __sn_string_bytes.len() { __sn_string_start += 1; } } __sn_string_lines };
    { let __sn_assert_condition = ((lines).len() as i64 == 3); let __sn_assert_message = "pipe lines preserved".to_string(); if !__sn_assert_condition { eprintln!("{}", __sn_assert_message); std::process::exit(1); } };
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", (lines).len() as i64)); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", (lines)[__sn_index((lines).len(), 0)])); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", (lines)[__sn_index((lines).len(), 1)])); __sn_interpolated });
    println!("{}", (source).contains(("first".to_string()).as_str()));
    println!("{}", { let __sn_array_value = { let __sn_string_value = &(makePipe()); let __sn_string_bytes = __sn_string_value.as_bytes(); let mut __sn_string_lines = Vec::new(); let mut __sn_string_start = 0usize; while __sn_string_start < __sn_string_bytes.len() { let mut __sn_string_end = __sn_string_start; while __sn_string_end < __sn_string_bytes.len() && __sn_string_bytes[__sn_string_end] != b'\n' && __sn_string_bytes[__sn_string_end] != b'\r' { __sn_string_end += 1; } __sn_string_lines.push(String::from_utf8(__sn_string_bytes[__sn_string_start..__sn_string_end].to_vec()).expect("splitLines produced invalid UTF-8")); __sn_string_start = __sn_string_end; if __sn_string_start < __sn_string_bytes.len() && __sn_string_bytes[__sn_string_start] == b'\r' && __sn_string_start + 1 < __sn_string_bytes.len() && __sn_string_bytes[__sn_string_start + 1] == b'\n' { __sn_string_start += 2; } else if __sn_string_start < __sn_string_bytes.len() { __sn_string_start += 1; } } __sn_string_lines }; let __sn_array_index = chooseIndex(); __sn_array_value[__sn_index(__sn_array_value.len(), __sn_array_index)].clone() });
    let mut __sn_string_value: String = "a\r\nb\n\nc\r".to_string();
    let mut __sn_array_value: Vec<String> = { let __sn_string_value = &(__sn_string_value); let __sn_string_bytes = __sn_string_value.as_bytes(); let mut __sn_string_lines = Vec::new(); let mut __sn_string_start = 0usize; while __sn_string_start < __sn_string_bytes.len() { let mut __sn_string_end = __sn_string_start; while __sn_string_end < __sn_string_bytes.len() && __sn_string_bytes[__sn_string_end] != b'\n' && __sn_string_bytes[__sn_string_end] != b'\r' { __sn_string_end += 1; } __sn_string_lines.push(String::from_utf8(__sn_string_bytes[__sn_string_start..__sn_string_end].to_vec()).expect("splitLines produced invalid UTF-8")); __sn_string_start = __sn_string_end; if __sn_string_start < __sn_string_bytes.len() && __sn_string_bytes[__sn_string_start] == b'\r' && __sn_string_start + 1 < __sn_string_bytes.len() && __sn_string_bytes[__sn_string_start + 1] == b'\n' { __sn_string_start += 2; } else if __sn_string_start < __sn_string_bytes.len() { __sn_string_start += 1; } } __sn_string_lines };
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", (__sn_array_value).len() as i64)); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", (__sn_array_value)[__sn_index((__sn_array_value).len(), 0)])); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", (__sn_array_value)[__sn_index((__sn_array_value).len(), 1)])); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", (__sn_array_value)[__sn_index((__sn_array_value).len(), 2)])); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", (__sn_array_value)[__sn_index((__sn_array_value).len(), 3)])); __sn_interpolated });
    println!("{}", ({ let __sn_array_value = { let __sn_string_value = &(forward(__sn_string_value.clone())); let __sn_string_bytes = __sn_string_value.as_bytes(); let mut __sn_string_lines = Vec::new(); let mut __sn_string_start = 0usize; while __sn_string_start < __sn_string_bytes.len() { let mut __sn_string_end = __sn_string_start; while __sn_string_end < __sn_string_bytes.len() && __sn_string_bytes[__sn_string_end] != b'\n' && __sn_string_bytes[__sn_string_end] != b'\r' { __sn_string_end += 1; } __sn_string_lines.push(String::from_utf8(__sn_string_bytes[__sn_string_start..__sn_string_end].to_vec()).expect("splitLines produced invalid UTF-8")); __sn_string_start = __sn_string_end; if __sn_string_start < __sn_string_bytes.len() && __sn_string_bytes[__sn_string_start] == b'\r' && __sn_string_start + 1 < __sn_string_bytes.len() && __sn_string_bytes[__sn_string_start + 1] == b'\n' { __sn_string_start += 2; } else if __sn_string_start < __sn_string_bytes.len() { __sn_string_start += 1; } } __sn_string_lines }; let __sn_array_index = 2; __sn_array_value[__sn_index(__sn_array_value.len(), __sn_array_index)].clone() }).len() as i64);
}
