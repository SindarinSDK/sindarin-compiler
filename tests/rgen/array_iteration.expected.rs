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

fn __sn_runtime_error(message: &'static str) -> ! {
    eprintln!("{}", message);
    std::process::exit(1);
}

fn __sn_checked<T>(value: Option<T>, message: &'static str) -> T {
    match value {
        Some(value) => value,
        None => __sn_runtime_error(message),
    }
}

fn __sn_checked_div<T>(value: Option<T>, divisor_is_zero: bool) -> T {
    __sn_checked(value, if divisor_is_zero {
        "panic: Division by zero"
    } else {
        "Runtime error: integer overflow in division"
    })
}

fn __sn_checked_mod<T>(value: Option<T>, divisor_is_zero: bool) -> T {
    __sn_checked(value, if divisor_is_zero {
        "panic: Modulo by zero"
    } else {
        "Runtime error: integer overflow in modulo"
    })
}

fn main() {
    let mut total: i64 = 0;
    {
        let mut i: i64 = 0;

        while (i < 5)
 {
            (total = __sn_checked(total.checked_add(i), "Runtime error: integer overflow in addition")
);
            { let __sn_place = &mut (i); let __sn_previous = *__sn_place; let __sn_next = __sn_checked(__sn_previous.checked_add(1), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_previous };
        }
    }
    println!("{}", total);
    let mut values: Vec<i64> = vec![2, 4, 6];
    for mut value in (values).iter().cloned() {
        (total = __sn_checked(total.checked_add(value), "Runtime error: integer overflow in addition")
);
    }
    println!("{}", total);
    if (total == 22)
 {
        println!("{}", "matched".to_string());
    }
    let mut countdown: i64 = 2;
    while (countdown > 0)
 {
        println!("{}", countdown);
        { let __sn_place = &mut (countdown); let __sn_previous = *__sn_place; let __sn_next = __sn_checked(__sn_previous.checked_sub(1), "Runtime error: integer overflow in subtraction"); *__sn_place = __sn_next; __sn_previous };
    }
    let mut range_values: Vec<i64> = (3..7).collect::<Vec<i64>>();
    println!("{}", (range_values)[__sn_index((range_values).len(), 0)]);
    println!("{}", (range_values)[__sn_index((range_values).len(), (-1))]);
    let mut range_total: i64 = 0;
    for mut value in ((1..5).collect::<Vec<i64>>()).iter().cloned() {
        (range_total = __sn_checked(range_total.checked_add(value), "Runtime error: integer overflow in addition")
);
    }
    println!("{}", range_total);
    let mut names: Vec<String> = vec!["one".to_string(), "two".to_string()];
    for mut name in (names).iter().cloned() {
        println!("{}", name);
    }
}

