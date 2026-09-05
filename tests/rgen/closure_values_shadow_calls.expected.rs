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

struct __SnClosure<F: ?Sized>(std::rc::Rc<F>);
impl<F: ?Sized> Clone for __SnClosure<F> {
    fn clone(&self) -> Self { Self(self.0.clone()) }
}
impl<F: ?Sized> std::fmt::Debug for __SnClosure<F> {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        f.write_str("<function>")
    }
}
impl<F: ?Sized> PartialEq for __SnClosure<F> {
    fn eq(&self, other: &Self) -> bool { std::rc::Rc::ptr_eq(&self.0, &other.0) }
}
fn action(x: i64) -> i64 {
    return __sn_checked((x).checked_add(1), "Runtime error: integer overflow in addition")
;
}

fn parameter(action: __SnClosure<dyn Fn(i64) -> i64>) -> i64 {
    return ((action.clone()).0)(2);
}

fn main() {
    println!("{}", action(1));
    if true {
        let mut action: __SnClosure<dyn Fn(i64) -> i64> = { self::__SnClosure::<dyn Fn(i64) -> i64>(std::rc::Rc::new(move |x: i64| -> i64 { __sn_checked((x).checked_add(10), "Runtime error: integer overflow in addition")
})) }
;
        println!("{}", ((action.clone()).0)(1));
        println!("{}", parameter(action.clone()));
        if true {
        let mut action: __SnClosure<dyn Fn(i64) -> i64> = { self::__SnClosure::<dyn Fn(i64) -> i64>(std::rc::Rc::new(move |x: i64| -> i64 { __sn_checked((x).checked_add(20), "Runtime error: integer overflow in addition")
})) }
;
        println!("{}", ((action.clone()).0)(1));
    }
        println!("{}", ((action.clone()).0)(1));
    }
    println!("{}", action(1));
    let mut values: Vec<__SnClosure<dyn Fn(i64) -> i64>> = vec![{ self::__SnClosure::<dyn Fn(i64) -> i64>(std::rc::Rc::new(move |x: i64| -> i64 { __sn_checked((x).checked_add(30), "Runtime error: integer overflow in addition")
})) }
];
    for mut action in (values).iter().cloned() {
        println!("{}", ((action.clone()).0)(1));
    }
    println!("{}", action(1));
    if true {
        let mut action: __SnClosure<dyn Fn(i64) -> i64> = { self::__SnClosure::<dyn Fn(i64) -> i64>(std::rc::Rc::new(move |x: i64| -> i64 { __sn_checked((x).checked_add(40), "Runtime error: integer overflow in addition")
})) }
;
        let mut result: i64 = match (1 as i64) {
        1 => {
            (((action.clone()).0)(1) as i64)
        },
        _ => {
            (((action.clone()).0)(2) as i64)
        },
    };
        println!("{}", result);
    }
    println!("{}", action(1));
    let mut action: __SnClosure<dyn Fn(i64) -> i64> = { self::__SnClosure::<dyn Fn(i64) -> i64>(std::rc::Rc::new(move |x: i64| -> i64 { __sn_checked((x).checked_add(50), "Runtime error: integer overflow in addition")
})) }
;
    println!("{}", ((action.clone()).0)(1));
}

