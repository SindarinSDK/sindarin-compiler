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
#[derive(Clone, Debug, PartialEq)]
struct Holder {
    action: __SnClosure<dyn Fn(i64) -> i64>,
}

fn increment(x: i64) -> i64 {
    return __sn_checked((x).checked_add(1), "Runtime error: integer overflow in addition")
;
}

fn identity(action: __SnClosure<dyn Fn(i64) -> i64>) -> __SnClosure<dyn Fn(i64) -> i64> {
    return action.clone();
}

fn main() {
    let mut named: __SnClosure<dyn Fn(i64) -> i64> = self::__SnClosure::<dyn Fn(i64) -> i64>(std::rc::Rc::new(increment));
    let mut alias: __SnClosure<dyn Fn(i64) -> i64> = named.clone();
    let mut returned: __SnClosure<dyn Fn(i64) -> i64> = identity(alias.clone());
    { named = { self::__SnClosure::<dyn Fn(i64) -> i64>(std::rc::Rc::new(move |x: i64| -> i64 { __sn_checked((x).checked_add(100), "Runtime error: integer overflow in addition")
})) }
; named.clone() };
    print!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", ((alias.clone()).0)(1))); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", ((returned.clone()).0)(2))); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", ((named.clone()).0)(3))); __sn_interpolated.push_str("\n"); __sn_interpolated });
    let mut holder: Holder = Holder { action: alias.clone() };
    ((holder).action = returned.clone());
    print!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", (((holder).action.clone()).0)(4))); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", ((alias.clone()).0)(5))); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", ((returned.clone()).0)(6))); __sn_interpolated.push_str("\n"); __sn_interpolated });
    let mut actions: Vec<__SnClosure<dyn Fn(i64) -> i64>> = vec![{ self::__SnClosure::<dyn Fn(i64) -> i64>(std::rc::Rc::new(move |x: i64| -> i64 { x.clone()})) }
];
    { let __sn_array_index = __sn_index((actions).len(), 0); (actions)[__sn_array_index] = returned.clone(); };
    print!("{}", { let mut __sn_interpolated = String::new(); __sn_interpolated.push_str(&format!("{}", (({ let (__sn_functions, __sn_function_index) = (&(actions), 0); __sn_functions[__sn_index(__sn_functions.len(), __sn_function_index)].clone() }).0)(7))); __sn_interpolated.push_str(":"); __sn_interpolated.push_str(&format!("{}", ((returned.clone()).0)(8))); __sn_interpolated.push_str("\n"); __sn_interpolated });
}

