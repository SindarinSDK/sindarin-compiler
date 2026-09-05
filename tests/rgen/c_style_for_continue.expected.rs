#![allow(dead_code, unused_mut, unused_variables, unused_parens)]

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
    let mut odd_total: i64 = 0;
    {
        let mut i: i64 = 0;

        while (i < 6)
 {
            if ({ let __sn_left = i; let __sn_right = 2; __sn_checked_mod(__sn_left.checked_rem(__sn_right), __sn_right == 0) }
 == 0)
 {
        { { let __sn_place = &mut (i); let __sn_previous = *__sn_place; let __sn_next = __sn_checked(__sn_previous.checked_add(1), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_previous }; continue; }
    }
            (odd_total = __sn_checked((odd_total).checked_add(i), "Runtime error: integer overflow in addition")
);
            { let __sn_place = &mut (i); let __sn_previous = *__sn_place; let __sn_next = __sn_checked(__sn_previous.checked_add(1), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_previous };
        }
    }
    println!("{}", odd_total);
    let mut nested_total: i64 = 0;
    {
        let mut outer: i64 = 0;

        while (outer < 3)
 {
            let mut inner: i64 = 0;
            while (inner < 3)
 {
        { let __sn_place = &mut (inner); let __sn_previous = *__sn_place; let __sn_next = __sn_checked(__sn_previous.checked_add(1), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_previous };
        if (inner < 3)
 {
        continue;
    }
        (nested_total = __sn_checked((nested_total).checked_add(outer), "Runtime error: integer overflow in addition")
);
    }
            { let __sn_place = &mut (outer); let __sn_previous = *__sn_place; let __sn_next = __sn_checked(__sn_previous.checked_add(1), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_previous };
        }
    }
    println!("{}", nested_total);
    let mut pair_total: i64 = 0;
    {
        let mut row: i64 = 0;

        while (row < 3)
 {
            {
        let mut column: i64 = 0;

        while (column < 3)
 {
            if (row == column)
 {
        { { let __sn_place = &mut (column); let __sn_previous = *__sn_place; let __sn_next = __sn_checked(__sn_previous.checked_add(1), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_previous }; continue; }
    }
            { let __sn_place = &mut (pair_total); let __sn_previous = *__sn_place; let __sn_next = __sn_checked(__sn_previous.checked_add(1), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_previous };
            { let __sn_place = &mut (column); let __sn_previous = *__sn_place; let __sn_next = __sn_checked(__sn_previous.checked_add(1), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_previous };
        }
    }
            { let __sn_place = &mut (row); let __sn_previous = *__sn_place; let __sn_next = __sn_checked(__sn_previous.checked_add(1), "Runtime error: integer overflow in addition"); *__sn_place = __sn_next; __sn_previous };
        }
    }
    println!("{}", pair_total);
}

