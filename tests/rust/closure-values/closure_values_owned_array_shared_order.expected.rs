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
fn invoke(callback: __SnClosure<dyn Fn() -> i64>) -> i64 {
    return ((callback.clone()).0)();
}

fn main() {
    let values: std::rc::Rc<std::cell::RefCell<Vec<i64>>> = std::rc::Rc::new(std::cell::RefCell::new(vec![7]));
    let calls: std::rc::Rc<std::cell::Cell<i64>> = std::rc::Rc::new(std::cell::Cell::new(0));
    let mut observe: __SnClosure<dyn Fn() -> i64> = { let (calls, values, ) = (calls.clone(), values.clone(), ); self::__SnClosure::<dyn Fn() -> i64>(std::rc::Rc::new(move || -> i64 { { let (__sn_rhs, __sn_cell) = (1, &calls); let __sn_previous = __sn_cell.get(); let __sn_next = __sn_checked_0(__sn_previous.checked_add(__sn_rhs), "Runtime error: integer overflow in addition"); __sn_cell.set(__sn_next); __sn_next };return __sn_checked_0((__sn_checked_0(((values.borrow().clone()).len() as i64).checked_mul(10), "Runtime error: integer overflow in multiplication")).checked_add(calls.get()), "Runtime error: integer overflow in addition");})) }
;
    let mut replace: __SnClosure<dyn Fn(__SnClosure<dyn Fn() -> i64>) -> i64> = { let (values, ) = (values.clone(), ); self::__SnClosure::<dyn Fn(__SnClosure<dyn Fn() -> i64>) -> i64>(std::rc::Rc::new(move |callback: __SnClosure<dyn Fn() -> i64>| -> i64 { { let (__sn_value, __sn_cell) = (vec![invoke(callback.clone()), invoke(callback.clone())], &values); __sn_cell.replace(__sn_value.clone()); __sn_value };return __sn_checked_0((__sn_checked_0(((values.borrow().clone())[__sn_index((values.borrow().clone()).len(), 0)]).checked_mul(100), "Runtime error: integer overflow in multiplication")).checked_add((values.borrow().clone())[__sn_index((values.borrow().clone()).len(), 1)]), "Runtime error: integer overflow in addition");})) }
;
    println!("{}", ((replace.clone()).0)(observe.clone().clone()));
    println!("{}", (values.borrow().clone())[__sn_index((values.borrow().clone()).len(), 0)]);
    println!("{}", (values.borrow().clone())[__sn_index((values.borrow().clone()).len(), 1)]);
    println!("{}", calls.get());
}
