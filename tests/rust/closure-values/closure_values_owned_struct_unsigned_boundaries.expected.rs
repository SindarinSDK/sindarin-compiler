#![allow(dead_code, unused_mut, unused_variables, unused_parens)]

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
#[derive(Clone, Copy, Debug, PartialEq)]
struct Boundaries {
    add: u32,
    subtract: u32,
    multiply: u32,
    divide: u32,
    modulo: u32,
    increment: u32,
    decrement: u32,
}

fn main() {
    let mut max: u32 = 4294967295;
    let mut state: Boundaries = Boundaries { add: max, subtract: 0, multiply: 2147483648, divide: max, modulo: max, increment: max, decrement: 0 };
    let mut mutate: __SnClosure<dyn Fn() -> bool> = { let (state, max, ) = (state.clone(), max.clone(), ); self::__SnClosure::<dyn Fn() -> bool>(std::rc::Rc::new(move || -> bool { let mut state = state.clone(); { let __sn_rhs = 1; let __sn_place = &mut ((state).add); let __sn_next = (*__sn_place).wrapping_add(__sn_rhs); *__sn_place = __sn_next; __sn_next };{ let __sn_rhs = 1; let __sn_place = &mut ((state).subtract); let __sn_next = (*__sn_place).wrapping_sub(__sn_rhs); *__sn_place = __sn_next; __sn_next };{ let __sn_rhs = 2; let __sn_place = &mut ((state).multiply); let __sn_next = (*__sn_place).wrapping_mul(__sn_rhs); *__sn_place = __sn_next; __sn_next };{ let __sn_rhs = 2; let __sn_place = &mut ((state).divide); let __sn_next = (*__sn_place).wrapping_div(__sn_rhs); *__sn_place = __sn_next; __sn_next };{ let __sn_rhs = 2; let __sn_place = &mut ((state).modulo); let __sn_next = (*__sn_place).wrapping_rem(__sn_rhs); *__sn_place = __sn_next; __sn_next };let mut incrementBefore: u32 = { let __sn_previous = (state).increment; (state).increment += 1; __sn_previous };let mut decrementBefore: u32 = { let __sn_previous = (state).decrement; (state).decrement -= 1; __sn_previous };let mut result: bool = (((state.clone()).add == 0) && ((state.clone()).subtract == max.clone()));(result = ((result && ((state.clone()).multiply == 0)) && ((state.clone()).divide == 2147483647)));(result = (result && ((state.clone()).modulo == 1)));(result = ((result && (incrementBefore == max.clone())) && ((state.clone()).increment == 0)));(result = ((result && (decrementBefore == 0)) && ((state.clone()).decrement == max.clone())));return result;})) }
;
    println!("{}", ((mutate.clone()).0)());
    println!("{}", ((mutate.clone()).0)());
    println!("{}", ((state).add == max));
}
