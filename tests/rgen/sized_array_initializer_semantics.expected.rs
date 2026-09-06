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

fn __sn_runtime_error_0(message: &'static str) -> ! {
    eprintln!("{}", message);
    std::process::exit(1);
}

fn __sn_checked_0<T>(value: Option<T>, message: &'static str) -> T {
    match value {
        Some(value) => value,
        None => __sn_runtime_error_0(message),
    }
}

fn __sn_checked_div_0<T>(value: Option<T>, divisor_is_zero: bool) -> T {
    __sn_checked_0(value, if divisor_is_zero {
        "panic: Division by zero"
    } else {
        "Runtime error: integer overflow in division"
    })
}

fn __sn_checked_mod_0<T>(value: Option<T>, divisor_is_zero: bool) -> T {
    __sn_checked_0(value, if divisor_is_zero {
        "panic: Modulo by zero"
    } else {
        "Runtime error: integer overflow in modulo"
    })
}

fn observeSize(calls: &mut i64, size: i64) -> i64 {
    { let __sn_place = &mut (*(calls)); let __sn_previous = *__sn_place; let __sn_next = __sn_checked_0(__sn_previous.checked_add(1), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_previous };
    print!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("S"); __sn_interpolated.push_str(&format!("{}", *(calls))); __sn_interpolated.push_str(" "); __sn_interpolated });
    return size;
}

fn observeInt(calls: &mut i64) -> i64 {
    { let __sn_place = &mut (*(calls)); let __sn_previous = *__sn_place; let __sn_next = __sn_checked_0(__sn_previous.checked_add(1), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_previous };
    print!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("I"); __sn_interpolated.push_str(&format!("{}", *(calls))); __sn_interpolated.push_str(" "); __sn_interpolated });
    return __sn_checked_0((*(calls)).checked_mul(10), "Runtime error: integer overflow in multiplication");
}

fn main() {
    let mut size_calls: i64 = 0;
    let mut int_calls: i64 = 0;
    let mut values: Vec<i64> = { let mut __sn_sized_array_1 = Vec::with_capacity(__sn_array_size(observeSize(&mut (size_calls), 2))); let mut __sn_sized_index_1 = 0usize; while __sn_sized_index_1 < __sn_array_size(observeSize(&mut (size_calls), 2)) { __sn_sized_array_1.push(observeInt(&mut (int_calls))); __sn_sized_index_1 += 1; } __sn_sized_array_1 }
;
    let mut strings: Vec<String> = { let mut __sn_sized_array_2 = Vec::with_capacity(__sn_array_size(2)); let mut __sn_sized_index_2 = 0usize; while __sn_sized_index_2 < __sn_array_size(2) { __sn_sized_array_2.push(("value".to_string()).clone()); __sn_sized_index_2 += 1; } __sn_sized_array_2 }
;
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("|"); __sn_interpolated.push_str(&format!("{}", size_calls)); __sn_interpolated.push_str(","); __sn_interpolated.push_str(&format!("{}", int_calls)); __sn_interpolated.push_str(","); __sn_interpolated.push_str(&format!("{}", (values)[__sn_index((values).len(), 0)])); __sn_interpolated.push_str(","); __sn_interpolated.push_str(&format!("{}", (values)[__sn_index((values).len(), 1)])); __sn_interpolated.push_str(","); __sn_interpolated.push_str(&format!("{}", (strings)[__sn_index((strings).len(), 0)])); __sn_interpolated.push_str(","); __sn_interpolated.push_str(&format!("{}", (strings)[__sn_index((strings).len(), 1)])); __sn_interpolated });
    let mut zero_size_calls: i64 = 0;
    let mut zero_default_calls: i64 = 0;
    let mut empty: Vec<i64> = { let mut __sn_sized_array_3 = Vec::with_capacity(__sn_array_size(observeSize(&mut (zero_size_calls), 0))); let mut __sn_sized_index_3 = 0usize; while __sn_sized_index_3 < __sn_array_size(observeSize(&mut (zero_size_calls), 0)) { __sn_sized_array_3.push(observeInt(&mut (zero_default_calls))); __sn_sized_index_3 += 1; } __sn_sized_array_3 }
;
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("|"); __sn_interpolated.push_str(&format!("{}", zero_size_calls)); __sn_interpolated.push_str(","); __sn_interpolated.push_str(&format!("{}", zero_default_calls)); __sn_interpolated.push_str(","); __sn_interpolated.push_str(&format!("{}", (empty).len() as i64)); __sn_interpolated });
    let mut seed: String = "seed".to_string();
    let mut copies: Vec<String> = { let mut __sn_sized_array_4 = Vec::with_capacity(__sn_array_size(2)); let mut __sn_sized_index_4 = 0usize; while __sn_sized_index_4 < __sn_array_size(2) { __sn_sized_array_4.push((seed).clone()); __sn_sized_index_4 += 1; } __sn_sized_array_4 }
;
    (seed = "changed".to_string());
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("|"); __sn_interpolated.push_str(&format!("{}", (copies)[__sn_index((copies).len(), 0)])); __sn_interpolated.push_str(","); __sn_interpolated.push_str(&format!("{}", (copies)[__sn_index((copies).len(), 1)])); __sn_interpolated.push_str(","); __sn_interpolated.push_str(&format!("{}", seed)); __sn_interpolated });
    let mut __sn_sized_array: i64 = 2;
    let mut __sn_sized_index: i64 = 7;
    let mut __sn_sized_array_0: i64 = 3;
    let mut __sn_sized_index_0: i64 = 8;
    let mut first: Vec<i64> = { let mut __sn_sized_array_5 = Vec::with_capacity(__sn_array_size(__sn_sized_array)); let mut __sn_sized_index_5 = 0usize; while __sn_sized_index_5 < __sn_array_size(__sn_sized_array) { __sn_sized_array_5.push(__sn_sized_index); __sn_sized_index_5 += 1; } __sn_sized_array_5 }
;
    let mut second: Vec<i64> = { let mut __sn_sized_array_6 = Vec::with_capacity(__sn_array_size(__sn_sized_array_0)); let mut __sn_sized_index_6 = 0usize; while __sn_sized_index_6 < __sn_array_size(__sn_sized_array_0) { __sn_sized_array_6.push(__sn_sized_index_0); __sn_sized_index_6 += 1; } __sn_sized_array_6 }
;
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("|"); __sn_interpolated.push_str(&format!("{}", (first)[__sn_index((first).len(), 0)])); __sn_interpolated.push_str(","); __sn_interpolated.push_str(&format!("{}", (first)[__sn_index((first).len(), 1)])); __sn_interpolated.push_str(","); __sn_interpolated.push_str(&format!("{}", (second)[__sn_index((second).len(), 0)])); __sn_interpolated.push_str(","); __sn_interpolated.push_str(&format!("{}", (second)[__sn_index((second).len(), 2)])); __sn_interpolated });
}
