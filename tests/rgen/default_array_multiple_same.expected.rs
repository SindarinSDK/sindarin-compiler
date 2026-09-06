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

#[derive(Clone, Debug, PartialEq)]
struct Bag {
    values: Vec<i64>,
}

fn __sn_array_alias_call_0() {
    println!("{}", "source helper".to_string());
}

fn mutateAlias(first: &mut Vec<i64>, second: &mut Vec<i64>) -> i64 {
    (first).push(2);
    println!("{}", (second).len() as i64);
    (second).push(3);
    return (first).len() as i64;
}

fn mark(calls: &mut i64) -> i64 {
    { let __sn_place = &mut (*(calls)); let __sn_previous = *__sn_place; let __sn_next = __sn_checked_0(__sn_previous.checked_add(1), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_previous };
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str("mark:"); __sn_interpolated.push_str(&format!("{}", *(calls))); __sn_interpolated });
    return 4;
}

fn mutateMixed(first: &mut Vec<i64>, marker: i64, second: &mut Vec<i64>, third: &mut Vec<i64>) -> i64 {
    (first).push(marker);
    println!("{}", (second).len() as i64);
    (third).push((first).len() as i64);
    return __sn_checked_0((__sn_checked_0((__sn_checked_0(((first).len() as i64).checked_mul(100), "Runtime error: integer overflow in multiplication")).checked_add(__sn_checked_0(((second).len() as i64).checked_mul(10), "Runtime error: integer overflow in multiplication")), "Runtime error: integer overflow in addition")).checked_add((third).len() as i64), "Runtime error: integer overflow in addition");
}

fn main() {
    let mut values: Vec<i64> = vec![1];
    println!("{}", __sn_array_alias_call_1(&mut (values)));
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", (values).len() as i64)); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", (values)[__sn_index((values).len(), 1)])); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", (values)[__sn_index((values).len(), 2)])); __sn_interpolated });
    let mut calls: i64 = 0;
    let mut shared: Vec<i64> = vec![5];
    let mut distinct: Vec<i64> = vec![8];
    println!("{}", { let __sn_array_call_arg_0 = mark(&mut (calls)); __sn_array_alias_call_2(&mut (shared), __sn_array_call_arg_0, &mut (distinct)) });
    println!("{}", calls);
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", (shared).len() as i64)); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", (shared)[__sn_index((shared).len(), 1)])); __sn_interpolated });
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", (distinct).len() as i64)); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", (distinct)[__sn_index((distinct).len(), 1)])); __sn_interpolated });
    let mut bag: Bag = Bag { values: vec![1] };
    println!("{}", __sn_array_alias_call_3(&mut ((bag).values)));
    println!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", ((bag).values).len() as i64)); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", ((bag).values)[__sn_index(((bag).values).len(), 1)])); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", ((bag).values)[__sn_index(((bag).values).len(), 2)])); __sn_interpolated });
}

fn __sn_array_alias_call_1(first: &mut Vec<i64>) -> i64 {
    (first).push(2);
    println!("{}", (first).len() as i64);
    (first).push(3);
    return (first).len() as i64;
}

fn __sn_array_alias_call_2(first: &mut Vec<i64>, marker: i64, third: &mut Vec<i64>) -> i64 {
    (first).push(marker);
    println!("{}", (first).len() as i64);
    (third).push((first).len() as i64);
    return __sn_checked_0((__sn_checked_0((__sn_checked_0(((first).len() as i64).checked_mul(100), "Runtime error: integer overflow in multiplication")).checked_add(__sn_checked_0(((first).len() as i64).checked_mul(10), "Runtime error: integer overflow in multiplication")), "Runtime error: integer overflow in addition")).checked_add((third).len() as i64), "Runtime error: integer overflow in addition");
}

fn __sn_array_alias_call_3(first: &mut Vec<i64>) -> i64 {
    (first).push(2);
    println!("{}", (first).len() as i64);
    (first).push(3);
    return (first).len() as i64;
}
