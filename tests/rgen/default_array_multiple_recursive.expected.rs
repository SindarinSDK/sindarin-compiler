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

fn appendRecursive(first: &mut Vec<i64>, second: &mut Vec<i64>, remaining: i64) -> i64 {
    if (remaining == 0) {
        (second).push((first).len() as i64);
        return (second).len() as i64;
    }
    (first).push(remaining);
    return { let __sn_array_call_arg_0 = __sn_checked_0((remaining).checked_sub(1), "Runtime error: integer overflow in subtraction"); appendRecursive(&mut *(first), &mut *(second), __sn_array_call_arg_0) };
}

fn main() {
    let mut values: Vec<i64> = vec![1];
    println!("{}", { let __sn_array_call_arg_1 = 2; __sn_array_alias_call_0(&mut (values), __sn_array_call_arg_1) });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", (values).len() as i64)); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", (values)[__sn_index((values).len(), 1)])); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", (values)[__sn_index((values).len(), 2)])); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", (values)[__sn_index((values).len(), 3)])); __sn_interpolated });
}

fn __sn_array_alias_call_0(first: &mut Vec<i64>, remaining: i64) -> i64 {
    if (remaining == 0) {
        (first).push((first).len() as i64);
        return (first).len() as i64;
    }
    (first).push(remaining);
    return { let __sn_array_call_arg_2 = __sn_checked_0((remaining).checked_sub(1), "Runtime error: integer overflow in subtraction"); __sn_array_alias_call_0(&mut *(first), __sn_array_call_arg_2) };
}
