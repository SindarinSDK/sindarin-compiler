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

#[derive(Clone, Copy, Debug, PartialEq)]
struct Mutator {
    marker: i64,
}

impl Mutator {
    fn append(&self, values: &mut Vec<i64>) -> i64 {
        (values).push((self).marker);
        return (values).len() as i64;
    }
    fn appendStatic(values: &mut Vec<i64>, marker: i64) {
        (values).push(marker);
    }
}

fn forward(values: &mut Vec<i64>) {
    Mutator::appendStatic(&mut *(values), 7);
}

fn makeValues(calls: &mut i64) -> Vec<i64> {
    { let __sn_place = &mut (*(calls)); let __sn_previous = *__sn_place; let __sn_next = __sn_checked_0(__sn_previous.checked_add(1), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_previous };
    return vec![8];
}

fn main() {
    let mut values: Vec<i64> = vec![1];
    let mut mutator: Mutator = Mutator { marker: 5 };
    println!("{}", (mutator).append(&mut (values)));
    forward(&mut (values));
    println!("{}", (values).len() as i64);
    println!("{}", (values)[__sn_index((values).len(), 1)]);
    println!("{}", (values)[__sn_index((values).len(), 2)]);
    let mut calls: i64 = 0;
    Mutator::appendStatic(&mut (makeValues(&mut (calls))), 9);
    println!("{}", calls);
}
