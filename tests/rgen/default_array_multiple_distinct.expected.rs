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

fn mutatePair(first: &mut Vec<i64>, second: &mut Vec<i64>) -> i64 {
    (first).push((second).len() as i64);
    (second).push((first).len() as i64);
    return __sn_checked_0((__sn_checked_0(((first).len() as i64).checked_mul(10), "Runtime error: integer overflow in multiplication")).checked_add((second).len() as i64), "Runtime error: integer overflow in addition");
}

fn observeAfterArrays(first: &mut Vec<i64>, second: &mut Vec<i64>, seen: i64) {
    (first).push(seen);
    (second).push((first).len() as i64);
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", seen)); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", (first).len() as i64)); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", (second).len() as i64)); __sn_interpolated });
}

fn main() {
    let mut first: Vec<i64> = vec![1];
    let mut second: Vec<i64> = vec![7, 8];
    println!("{}", mutatePair(&mut (first), &mut (second)));
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", (first).len() as i64)); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", (first)[__sn_index((first).len(), 1)])); __sn_interpolated });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", (second).len() as i64)); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", (second)[__sn_index((second).len(), 2)])); __sn_interpolated });
    { let __sn_array_call_arg_0 = (first).len() as i64; observeAfterArrays(&mut (first), &mut (second), __sn_array_call_arg_0) };
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", (first).len() as i64)); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", (first)[__sn_index((first).len(), 2)])); __sn_interpolated });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", (second).len() as i64)); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", (second)[__sn_index((second).len(), 3)])); __sn_interpolated });
}
